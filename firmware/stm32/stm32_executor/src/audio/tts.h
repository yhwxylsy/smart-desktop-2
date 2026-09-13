#pragma once
#include <Arduino.h>
#include "../../config.h"

// SYN6288 语音合成（原 sketch L120-136、L1459-1696 原样搬运）。
//
// 职责：把中文文本转成 SYN6288 的语音合成帧，从软件串口（PB3）发出去；并管理音量。
// 面试可讲三个核心：
//   1. SYN6288 帧格式：`0xFD + 数据长度(2B) + 命令(0x01合成/0x02停止) + 文本类型(0x03=Unicode)
//      + 文本数据 + XOR校验`，长度/校验都是协议规定，必须逐字节拼对。
//   2. 为什么做 UTF-8 -> UTF-16BE：后端下发的文本是 UTF-8，但 SYN6288 的 Unicode 模式
//      要的是 UTF-16 大端序。所以本地实现了一套 UTF-8 解码 + UTF-16BE 编码，避免在
//      STM32 上放 GBK 字表。
//   3. 音量：SYN6288 用文本内嵌的 `[vN]`（N=0~16）控制，代码把用户 0%~100% 音量
//      映射到 0~16 级，每档 10%。
// speechVolumePercent / volumeOverlayUntilMs 供屏幕层读取，以 extern 暴露。
extern uint8_t speechVolumePercent;
extern uint32_t volumeOverlayUntilMs;

bool appendCodePointAsUtf16Be(uint32_t codePoint, uint8_t *out, size_t capacity, size_t &outLen);
bool decodeNextUtf8CodePoint(const uint8_t *bytes, size_t length, size_t &index, uint32_t &codePoint);
bool sendSyn6288Frame(const uint8_t *textBytes, size_t textLen, uint8_t textType);
bool sendSyn6288Command(uint8_t command);
bool speakUtf8Bytes(const uint8_t *bytes, size_t length);
bool speakText(const String &text);
bool speakHexText(const String &hexText);
bool stopSpeechOutput();
uint8_t volumeLevelFromPercent(uint8_t percent);
uint8_t volumePercentFromLevel(uint8_t level);
bool setSpeechVolume(const String &value, bool announce);
// TODO(decouple): announceVolumeWhenSettled 为空函数且 volumeAnnouncementPending 只写不读，
// 属确证死代码，去耦合阶段删除并移除 loop 调用点。
void announceVolumeWhenSettled();
