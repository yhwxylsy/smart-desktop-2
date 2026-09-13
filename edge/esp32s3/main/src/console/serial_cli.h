#pragma once
#include <Arduino.h>
#include "../../config.h"

// 串口 CLI（原 main.ino L1665-1828 原样搬运）。
//
// 职责：ESP32S3 的 USB 串口命令行，用于现场配置/调试（配 WiFi、改服务器、测麦克风/RFID 等）。
// 面试可讲：和 STM32 的 dispatcher 一样是"表驱动分发"——CLI_COMMANDS[] 存
// 前缀+精确标志+处理函数，handleSerialCommand 遍历匹配，新增命令只需加一行表项。
void handleSerialCommand(String line);
void pollUsbSerial();
