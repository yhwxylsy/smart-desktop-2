#pragma once
#include <Arduino.h>
#include "../../config.h"

// OLED 显存/图元/驱动（原 sketch L943-1104、L1251-1402 原样搬运）。
//
// 职责：SSD1306 0.96" OLED（128x64，I2C）的底层驱动——显存 buffer、点/线/字符/中文绘制、
//       初始化与"分页刷新"。屏幕编排（render* / showOledText / updateSystemUi）在
//       oled_screens 模块，本模块只提供"画什么"和"怎么刷"的原子操作。
//
// 面试可讲两点：
//   1. 显存是"页式"的：128x64 分成 8 页，每页 8 行，一个字节对应一列 8 个像素，
//      oledSetPixel 用 `x + (y/8)*128` 定位字节、`1<<(y%8)` 定位位。
//   2. flushOledPage 是"每次只刷一页"的非阻塞刷新：OLED_FLUSH_INTERVAL_MS=4ms 刷一页，
//      8 页一轮约 32ms，避免一次性写满 1KB 阻塞 loop()，从而不拖累舵机/旋律等时序。
extern uint8_t oledAddress;
extern uint8_t oledBuffer[OLED_WIDTH * OLED_PAGES];
extern uint8_t oledFlushPage;
extern bool oledAvailable;
extern bool oledDirty;
extern bool oledRenderPending;
extern uint32_t lastUiFrameMs;
extern uint32_t lastOledFlushMs;
extern uint32_t oledFpsWindowMs;
extern uint16_t oledFrameCounter;
extern uint8_t oledFps;

void clearOledBuffer();
void oledSetPixel(uint8_t x, uint8_t y, bool on);
void oledFillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on);
void oledDrawChar(uint8_t x, uint8_t y, char ch, bool inverted);
void oledDrawText(uint8_t x, uint8_t y, const String &text, uint8_t maxChars, bool inverted);
void oledDrawTextCentered(const String &text, uint8_t y, uint8_t maxChars);
void oledDrawCjkTextCentered(const char *text, uint8_t y);
void oledDrawLine(int x0, int y0, int x1, int y1, bool on);
bool oledProbe(uint8_t address);
bool oledWriteCommand(uint8_t command);
bool oledWriteDataChunk(uint16_t offset, uint8_t length);
bool initializeOled();
void flushOledPage();
