#pragma once
#include <Arduino.h>

// 将 UTF-8 文本编码为十六进制串（原 main.ino L166-176 原样搬运）
//
// 职责：把字符串每个字节转成大写十六进制（如 "你好" -> "E4BDA0E5A5BD"）。
// 面试可讲：为什么走 HEX——串口协议是文本行，中文 UTF-8 字节可能含不可见/控制字符，
// 直接拼进 `NET:TTSHEX:` 会破坏命令解析，转成 ASCII 十六进制就能安全传输。
String utf8Hex(const String &text);
