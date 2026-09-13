#pragma once
#include <Arduino.h>
#include "../../config.h"

// KEY1(信息)/KEY2(DEMO) 双按键（原 sketch L1767-1879 原样搬运）。
//
// 职责：轮询两个物理按键，做消抖 + 短按/长按识别，并把事件以 `BT:BTN:` 上行。
// 面试可讲：按键不做成中断，而是 loop() 轮询 + 软件消抖（BUTTON_DEBOUNCE_MS=35ms），
// 再叠加一个"短按/长按"状态机——按下计时超过阈值（KEY2 600ms / KEY1 700ms）判定为长按。
// KEY2 短按=终止播报，长按=进入电脑麦克风 PTT 录音；KEY1 短按=切屏，长按=回主屏。
extern String lastButtonEvent;
extern uint32_t lastButtonEventMs;

void writeButtonEvent(const String &event);
void updateKey2Button();
void updateInfoButton();
