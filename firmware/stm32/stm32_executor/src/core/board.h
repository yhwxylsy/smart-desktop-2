#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "../../config.h"

// 板级串口与回写（原 sketch L84-89、L323-326、L2619-2640 原样搬运）。
// 2026-09-06 接线调整：USART3(PB11/PB10) 与 ESP32S3 组成全双工链路，
// 收 NET:* 与发 BT:* 都走它；SYN6288 改由 PB3 软件串口驱动（单向、短帧）。
// 依据见 docs/HARDWARE_WIRING.md。全项目仅本模块持有这些对象，
// 其他模块经本头声明的引用访问。
extern SoftwareSerial syn6288Serial;     // RX=PB4(占位未用), TX=PB3 -> SYN6288 RXD
extern HardwareSerial usbConsole;        // PA3(RX), PA2(TX) -> USB/调试
extern HardwareSerial espCommandSerial;  // PB11(RX), PB10(TX) <-> ESP32S3 全双工

// ESP 上行活动时间戳：pollSerial 写入，遥测/屏幕读取。
extern unsigned long lastEspRxActivityMs;
// ESP 接收行缓冲：loop 持有并传给 pollSerial。
extern String espLine;

void writeBack(const String &line);
void pollSerial(Stream &stream, String &buffer, const char *source);
