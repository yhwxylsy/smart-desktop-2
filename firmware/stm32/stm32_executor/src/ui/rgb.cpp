#include "rgb.h"
#include "../../config.h"
#include "ui_state.h"

static uint32_t lastRgbFrameMs = 0;

RgbState rgbState(bool red, bool green, bool blue) {
  RgbState state;
  state.red = red;
  state.green = green;
  state.blue = blue;
  return state;
}

void writeRgbRaw(bool red, bool green, bool blue) {
  digitalWrite(PIN_RGB_RED, red ? HIGH : LOW);
  digitalWrite(PIN_RGB_GREEN, green ? HIGH : LOW);
  digitalWrite(PIN_RGB_BLUE, blue ? HIGH : LOW);
}

void setRgb(bool red, bool green, bool blue) {
  uint32_t startedMs = millis();
  writeRgbRaw(red, green, blue);
  if (commandLightMeasureActive && !commandHadLight) {
    commandHadLight = true;
    commandLightResponseMs = millis() - startedMs;
  }
}

// 把 UI 状态机映射成"基础 RGB 颜色"：蓝=启动、青=聆听、黄=处理/播报/执行、
// 绿=就绪、红=锁定/错误。这是静态映射，动画闪烁由 updateRgbAnimation 在基础色上叠加。
RgbState uiMachineBaseRgb() {
  switch (uiMachineState) {
    case UI_STATE_BOOT:
      return rgbState(false, false, true);
    case UI_STATE_LISTENING:
      return rgbState(false, true, true);
    case UI_STATE_PROCESSING:
    case UI_STATE_SPEAKING:
    case UI_STATE_EXECUTING:
      return rgbState(true, true, false);
    case UI_STATE_READY:
      return rgbState(false, true, false);
    case UI_STATE_LOCKED:
    case UI_STATE_ERROR:
      return rgbState(true, false, false);
  }
  return rgbState(true, false, false);
}

// 灯效动画：每 RGB_FRAME_INTERVAL_MS(80ms) 推一帧，按状态机做呼吸/闪烁/双闪。
// 面试可讲：全程非阻塞——用 `(now - started) % 1000` 取相位做脉宽调制，不依赖 delay。
// 优先级：ACK 闪烁(ackFlashUntilMs) > 状态呼吸/闪烁。
void updateRgbAnimation() {
  uint32_t now = millis();
  if (now - lastRgbFrameMs < RGB_FRAME_INTERVAL_MS) {
    return;
  }
  lastRgbFrameMs = now;

  if (now < ackFlashUntilMs) {
    if (ackFlashOk) {
      bool flashOn = (((ackFlashUntilMs - now) / 130) & 0x01) == 0;
      writeRgbRaw(false, flashOn, false);
    } else {
      writeRgbRaw(true, false, false);
    }
    return;
  }

  uint16_t phase = (now - uiMachineStateStartedMs) % 1000;
  bool pulse = phase < 520;
  switch (uiMachineState) {
    case UI_STATE_BOOT:
      writeRgbRaw(false, false, pulse);
      break;
    case UI_STATE_LISTENING:
      writeRgbRaw(false, true, true);
      break;
    case UI_STATE_PROCESSING:
    case UI_STATE_SPEAKING:
    case UI_STATE_EXECUTING:
      writeRgbRaw(pulse, pulse, false);
      break;
    case UI_STATE_LOCKED:
    case UI_STATE_ERROR:
      {
        bool doubleBlink = phase < 140 || (phase >= 280 && phase < 420);
        writeRgbRaw(doubleBlink, false, false);
      }
      break;
    case UI_STATE_READY:
      writeRgbRaw(false, true, false);
      break;
  }
}
