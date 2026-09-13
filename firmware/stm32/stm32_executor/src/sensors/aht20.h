#pragma once
#include <Arduino.h>
#include "../../config.h"

// AHT20 温湿度（原 sketch L1914-1963 原样搬运）。
//
// 职责：通过 I2C 读取 AHT20 温湿度传感器（地址 0x38）。
// 面试可讲：AHT20 不是"读寄存器"型传感器，而是"发命令->等转换->读 7 字节原始值"
// 的命令式协议：
//   初始化 = 0xBE + 0x08 0x00；触发测量 = 0xAC + 0x33 0x00，之后要等 ~80ms 让片上
//   温湿度转换完成，再读 7 字节。原始数据是各 20bit 的定点数，需要按数据手册公式换算。
extern bool aht20Initialized;

bool initializeAht20();
bool readAht20(float &temperatureC, float &humidityPct);
