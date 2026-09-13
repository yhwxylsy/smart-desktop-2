#pragma once
#include <Arduino.h>
#include "../../config.h"

// DRV8833 风扇驱动（原 sketch L2078-2125 原样搬运）。
//
// 职责：用 DRV8833（H 桥电机驱动）驱动桌面小风扇，支持 3 档 PWM 调速。
// 面试可讲：DRV8833 有两个输入 IN1/IN2，控制 H 桥的通断来决定电机正反转；
// 本项目风扇是单向的，所以固定 IN1 拉低、IN2 打 PWM，只走一个方向。
// 注意 PB5 是板载继电器脚，但不是风扇输出——风扇走 DRV8833 的 PA0/PA1。
extern uint8_t currentFanLevel;
extern bool drv8833Connected;  // setup 依据此开关决定是否初始化 IN1/IN2 引脚

void stopDrv8833();
uint8_t drv8833FanDutyForLevel(uint8_t level);       // 档位(1~3) -> 占空比(217/235/255)
bool driveFanOn(uint8_t level);
uint8_t fanLevelFromCommand(const String &command);  // 从 `NET:FAN:ON:<1-3>` 解析档位
