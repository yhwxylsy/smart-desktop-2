#pragma once
#include <Arduino.h>
#include "../../config.h"

// 文本工具（原 sketch L360-384、L1610-1621、L2324-2330 原样搬运）。
//
// 职责：一组无状态的小工具函数，供协议解析、TTS 编解码、OLED 展示复用。
// 都是纯函数，便于单元测试。
String upperCopy(const String &text);                  // 返回大写副本（不修改原串）
bool containsUpperToken(const String &text, const char *token);  // 忽略大小写判断是否含 token
String compactForDisplay(const String &text, uint8_t maxLen);    // 压短+清洗不可打印字符（用于屏显）
int hexNibble(char ch);                               // 十六进制字符 -> 数值，非法返回 -1
String nextColonField(const String &text, int &offset);         // 按 ':' 切下一个字段，空字段返回 "-"
