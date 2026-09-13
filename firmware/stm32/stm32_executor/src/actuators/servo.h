#pragma once
#include <Arduino.h>
#include "../../config.h"

// 舵机脉冲驱动（原 sketch L2167-2218 原样搬运）。
//
// 职责：用软件方式产生舵机的 50Hz PWM 控制脉冲（PB8）。
// 面试可讲：标准舵机用"脉宽"编码角度——20ms 周期里高电平 0.5~2.5ms 对应 0~180°。
// 这里没占用硬件定时器，而是 updateServoPulse 用 micros() 轮询翻转电平、软产生脉冲；
// 只在命令到达后的 SERVO_HOLD_MS(800ms) 内维持，之后自动释放，避免舵机持续受力。
bool parseServoAngle(const String &value, uint8_t &angle);
void setServoAngle(uint8_t angle);
void updateServoPulse();
