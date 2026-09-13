#pragma once
#include <Arduino.h>
#include "../../config.h"

// I2C 总线诊断（原 sketch L1256-1283、L1333-1342 原样搬运）。
//
// 职责：`NET:I2C?` 命令对应的 I2C 总线扫描，以及"扫描顺带把没探测到的 OLED 重新拉起"。
// 面试可讲：I2C 是 7 位地址，合法范围 1~0x77；scanI2cBus 逐个地址发一个空写事务，
// 用 endTransmission() 的返回值判断设备是否存在（0=有应答，4=其他错误）。
uint8_t scanI2cBus();
bool handleI2cScanCommand();
