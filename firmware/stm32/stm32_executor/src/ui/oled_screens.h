#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "oled.h"
#include "ui_state.h"

// OLED 屏幕编排（原 sketch L1134-1249、L1344-1368、L1448-1457、L1907-1912 原样搬运）。
//
// 职责：把"状态机 + 传感器 + 用户上下文"翻译成屏幕画面。分两层：
//   - renderStateHeader/Body ：主屏——顶部反色状态条 + 状态标题 + 三行状态信息；
//   - renderInfoScreen       ：KEY1 循环切换的副屏（用户/链路/FPS/传感器/执行器）。
// updateSystemUi 是每帧的总入口：先推进状态机、再跑灯效、再按帧率重绘、最后分页 flush。
void renderStateHeader(UiMachineState state);
void renderStateBody(UiMachineState state);
void renderInfoScreen(uint32_t now);
void renderStatusScreen(uint32_t now);
void renderSystemOled();
void showOledText(const String &text);
void updateSystemUi();
