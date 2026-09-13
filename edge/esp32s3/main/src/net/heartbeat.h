#pragma once
#include <Arduino.h>
#include "../../config.h"

// 心跳与语音状态（原 main.ino L843-867 原样搬运）。
//
// 职责：周期性向后端上报设备心跳，让后端知道"这台设备还活着、UART 是否正常"。
// 面试可讲：uart_ok 不是"串口对象存在"，而是"最近 5 秒内收到过 STM32 的 ACK/PONG"——
// 用 lastAckMs 的时效性判断链路是否真正通畅，比布尔在线标志可靠得多。
// voiceState 被遥测桥接、麦克风流水线共享，故以 extern 暴露；
// 去耦合阶段将改为访问接口。
extern String voiceState;

void sendHeartbeat(bool force = false);
void setVoiceState(const String &state, bool sendNow = false);
