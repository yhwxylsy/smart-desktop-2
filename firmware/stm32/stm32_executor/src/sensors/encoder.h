#pragma once
#include <Arduino.h>
#include "../../config.h"

// 旋钮编码器（原 sketch L2127-2161 原样搬运）。
//
// 职责：轮询读取 EC11 旋转编码器（A=PA8 / B=PA9 / 按键 PB15），累计位置和增量，
//       并把它映射成"调节 SYN6288 音量"（每 4 个卡点变 10% 音量）。
// 面试可讲：用两相格雷码的"上一状态<<2 | 当前状态"查 TRANSITIONS 表解码方向，
//       纯查表、无中断、无 delay，既消抖又省 CPU。
// encoderPosition / encoderDeltaSinceTelemetry 供遥测采样读取，以 extern 暴露。
extern long encoderPosition;
extern long encoderDeltaSinceTelemetry;

uint8_t readEncoderState();
void initEncoder();
void updateEncoder();
