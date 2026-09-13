#include "tts.h"
#include <string.h>
#include "../../config.h"
#include "../core/board.h"
#include "../core/text_util.h"
#include "../ui/oled.h"
#include "../ui/ui_state.h"
#include "buzzer.h"

uint8_t speechVolume = 10;
uint8_t speechVolumePercent = VOLUME_DEFAULT_PERCENT;
bool volumeAnnouncementPending = false;
uint32_t lastVolumeChangeMs = 0;
uint32_t volumeOverlayUntilMs = 0;
bool ttsInterrupted = false;

// 把一个 Unicode 码点按 UTF-16BE（大端）追加到输出缓冲。
// BMP 字符直接写 2 字节；补充平面（>0xFFFF）用代理对写成 4 字节。
// 0xD800~0xDFFF 是代理区，单独出现即非法，直接拒绝。
bool appendCodePointAsUtf16Be(uint32_t codePoint, uint8_t *out, size_t capacity, size_t &outLen) {
  if (codePoint <= 0xFFFF) {
    if (codePoint >= 0xD800 && codePoint <= 0xDFFF) {
      return false;
    }
    if (outLen + 2 > capacity) {
      return false;
    }
    out[outLen++] = (uint8_t)((codePoint >> 8) & 0xFF);
    out[outLen++] = (uint8_t)(codePoint & 0xFF);
    return true;
  }

  if (codePoint > 0x10FFFF || outLen + 4 > capacity) {
    return false;
  }

  uint32_t value = codePoint - 0x10000;
  uint16_t high = 0xD800 | ((value >> 10) & 0x3FF);
  uint16_t low = 0xDC00 | (value & 0x3FF);
  out[outLen++] = (uint8_t)((high >> 8) & 0xFF);
  out[outLen++] = (uint8_t)(high & 0xFF);
  out[outLen++] = (uint8_t)((low >> 8) & 0xFF);
  out[outLen++] = (uint8_t)(low & 0xFF);
  return true;
}

// 从 UTF-8 字节流解码出下一个码点（1~4 字节变长）。面试可讲 UTF-8 编码规则：
// 首字节高位 0 -> ASCII；110xxxxx -> 2 字节；1110xxxx -> 3 字节；11110xxx -> 4 字节；
// 后续字节固定 10xxxxxx。这里还做了"最短编码/超范围/代理区"的合法性校验，防止坏输入。
bool decodeNextUtf8CodePoint(const uint8_t *bytes, size_t length, size_t &index, uint32_t &codePoint) {
  if (index >= length) {
    return false;
  }

  uint8_t first = bytes[index++];
  if ((first & 0x80) == 0) {
    codePoint = first;
    return true;
  }

  uint8_t needed = 0;
  uint32_t value = 0;
  if ((first & 0xE0) == 0xC0) {
    needed = 1;
    value = first & 0x1F;
    if (value == 0) {
      return false;
    }
  } else if ((first & 0xF0) == 0xE0) {
    needed = 2;
    value = first & 0x0F;
  } else if ((first & 0xF8) == 0xF0) {
    needed = 3;
    value = first & 0x07;
  } else {
    return false;
  }

  if (index + needed > length) {
    return false;
  }

  for (uint8_t i = 0; i < needed; ++i) {
    uint8_t next = bytes[index++];
    if ((next & 0xC0) != 0x80) {
      return false;
    }
    value = (value << 6) | (next & 0x3F);
  }

  if ((needed == 1 && value < 0x80) ||
      (needed == 2 && value < 0x800) ||
      (needed == 3 && value < 0x10000) ||
      value > 0x10FFFF ||
      (value >= 0xD800 && value <= 0xDFFF)) {
    return false;
  }

  codePoint = value;
  return true;
}

// 拼装并发送一条 SYN6288 合成帧。
// 帧结构：FD | 长度高字节 | 长度低字节 | 0x01 | 文本类型 | 文本数据 | XOR校验。
// 长度字段 = 文本字节数 + 3（命令 1 + 类型 1 + 校验 1 之外的数据长度约定）。
// 校验 = 从帧头到校验位之前所有字节的异或。
bool sendSyn6288Frame(const uint8_t *textBytes, size_t textLen, uint8_t textType) {
  if (textLen == 0 || textLen > SYN6288_MAX_TEXT_BYTES) {
    return false;
  }

  uint8_t frame[SYN6288_MAX_TEXT_BYTES + 6];
  size_t frameLen = textLen + 6;
  uint16_t dataLen = (uint16_t)(textLen + 3);
  frame[0] = SYN6288_FRAME_HEADER;
  frame[1] = (uint8_t)((dataLen >> 8) & 0xFF);
  frame[2] = (uint8_t)(dataLen & 0xFF);
  frame[3] = SYN6288_CMD_SYNTHESIS;
  frame[4] = textType;
  memcpy(&frame[5], textBytes, textLen);

  uint8_t checksum = 0;
  for (size_t i = 0; i < frameLen - 1; ++i) {
    checksum ^= frame[i];
  }
  frame[frameLen - 1] = checksum;

  syn6288Serial.write(frame, frameLen);
  syn6288Serial.flush();
  delay(10);
  return true;
}

bool sendSyn6288Command(uint8_t command) {
  uint8_t frame[5];
  frame[0] = SYN6288_FRAME_HEADER;
  frame[1] = 0x00;
  frame[2] = 0x01;
  frame[3] = command;
  frame[4] = frame[0] ^ frame[1] ^ frame[2] ^ frame[3];
  syn6288Serial.write(frame, sizeof(frame));
  syn6288Serial.flush();
  delay(6);
  return true;
}

// 播报 UTF-8 文本：先拼一段 SYN6288 音量控制指令 `[vN]`（N=0~16），
// 再把 UTF-8 逐码点转成 UTF-16BE，最后发 Unicode 合成帧。
// 面试可讲：音量不是单独一条指令，而是内嵌在播报文本里的控制标记，这是 SYN6288 的协议特点。
bool speakUtf8Bytes(const uint8_t *bytes, size_t length) {
  uint8_t unicodeBytes[SYN6288_MAX_TEXT_BYTES];
  size_t unicodeLen = 0;
  size_t index = 0;

  String volumeControl = String("[v") + String(speechVolume) + "]";
  for (unsigned int i = 0; i < volumeControl.length(); ++i) {
    if (!appendCodePointAsUtf16Be((uint8_t)volumeControl.charAt(i), unicodeBytes,
                                  sizeof(unicodeBytes), unicodeLen)) {
      return false;
    }
  }

  while (index < length) {
    uint32_t codePoint = 0;
    if (!decodeNextUtf8CodePoint(bytes, length, index, codePoint)) {
      return false;
    }
    if (!appendCodePointAsUtf16Be(codePoint, unicodeBytes, sizeof(unicodeBytes), unicodeLen)) {
      return false;
    }
  }

  return sendSyn6288Frame(unicodeBytes, unicodeLen, SYN6288_TYPE_UNICODE);
}

bool speakText(const String &text) {
  ttsInterrupted = false;
  return speakUtf8Bytes((const uint8_t *)text.c_str(), text.length());
}

bool speakHexText(const String &hexText) {
  ttsInterrupted = false;
  if (hexText.length() == 0 || (hexText.length() % 2) != 0) {
    return false;
  }

  uint8_t utf8Bytes[SYN6288_MAX_TEXT_BYTES];
  size_t utf8Len = 0;
  for (unsigned int i = 0; i < hexText.length(); i += 2) {
    int high = hexNibble(hexText.charAt(i));
    int low = hexNibble(hexText.charAt(i + 1));
    if (high < 0 || low < 0 || utf8Len >= sizeof(utf8Bytes)) {
      return false;
    }
    utf8Bytes[utf8Len++] = (uint8_t)((high << 4) | low);
  }
  return speakUtf8Bytes(utf8Bytes, utf8Len);
}

bool stopSpeechOutput() {
  stopMusic();
  volumeAnnouncementPending = false;
  ttsInterrupted = true;
  oledRenderPending = true;
  return sendSyn6288Command(SYN6288_CMD_STOP);
}

uint8_t volumeLevelFromPercent(uint8_t percent) {
  return (uint8_t)(((uint16_t)percent * SYN6288_MAX_VOLUME + 50) / 100);
}

uint8_t volumePercentFromLevel(uint8_t level) {
  return (uint8_t)((((uint16_t)level * 10 + SYN6288_MAX_VOLUME / 2) /
                    SYN6288_MAX_VOLUME) *
                   10);
}

bool setSpeechVolume(const String &value, bool announce) {
  (void)announce;
  String normalized = value;
  normalized.trim();
  normalized.toUpperCase();

  int percent = speechVolumePercent;
  if (normalized == "UP") {
    percent += VOLUME_STEP_PERCENT;
  } else if (normalized == "DOWN") {
    percent -= VOLUME_STEP_PERCENT;
  } else {
    if (normalized.length() == 0) {
      return false;
    }
    for (unsigned int i = 0; i < normalized.length(); ++i) {
      if (!isDigit(normalized.charAt(i))) {
        return false;
      }
    }
    int level = normalized.toInt();
    if (level < 0) level = 0;
    if (level > SYN6288_MAX_VOLUME) level = SYN6288_MAX_VOLUME;
    percent = volumePercentFromLevel((uint8_t)level);
  }

  if (percent < 0) percent = 0;
  if (percent > VOLUME_MAX_PERCENT) percent = VOLUME_MAX_PERCENT;
  speechVolumePercent = (uint8_t)percent;
  speechVolume = volumeLevelFromPercent(speechVolumePercent);
  uiEvent.detail = String("VOLUME ") + String(speechVolumePercent) + "%";
  oledRenderPending = true;
  lastVolumeChangeMs = millis();
  volumeOverlayUntilMs = lastVolumeChangeMs + SCREEN_OVERLAY_MS;
  volumeAnnouncementPending = false;
  return true;
}

void announceVolumeWhenSettled() {
  volumeAnnouncementPending = false;
}
