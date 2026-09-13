#pragma once
#include <Arduino.h>
#include "../../config.h"

// 超声波测距（原 sketch L1965-1983 原样搬运）。
//
// 职责：HC-SR04 超声波测距。TRIG=PA11 发触发脉冲，ECHO=PA10 回波脉宽。
// 面试可讲：原理是"发 10us 高电平触发，然后测 ECHO 高电平持续时间"，距离 = 时间 × 声速/2。
// 代码用 pulseIn() 测脉宽，`/58.0f` 是声速换算的常数（微秒 -> 厘米）。
extern bool ultrasonicEnabled;

bool readDistanceCm(float &distanceCm);
