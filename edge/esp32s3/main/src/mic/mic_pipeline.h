#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "../core/types.h"

// 麦克风采集 -> 上传流水线与自检（原 main.ino L885-890、L1385-1576 原样搬运）。
//
// 职责：把"录一段 WAV -> 上传后端做 ASR -> 取回识别文本"串成一条流水线，
//       并驱动 STM32 的 OLED/RGB 状态（LISTEN/THINK/IDLE/ERROR）同步变化。
// 面试可讲：captureAndUploadMic 是典型的状态机式业务流程——
//   前置检查（开关/忙/就绪/网络）-> 置忙 -> 采集 -> 上传 -> 按结果回写状态 -> 释放。
// micBusy 是互斥锁，防止重入；上传前会 pauseWebSocketForMicUpload 临时断开 WS，
// 因为大音频走 HTTP 分块上传更稳，传完再恢复 WS。
bool rejectMicPath(const char *trigger);
bool captureAndUploadMic(bool inject = true, const String &source = "esp32_mic");
bool captureAndUploadMicAfterCue(String cueText, bool inject = true, const String &source = "esp32_mic");
bool runMicSelfTest(String phrase);
String normalizeSelfTestText(String value);                                     // 自检比对用的文本归一化（去标点/空格/大小写）
bool selfTestTextMatches(const String &recognized, const String &expected);
void sendMicSelfTestTelemetry(const String &expected, const AsrUploadResult &result, bool passed);
