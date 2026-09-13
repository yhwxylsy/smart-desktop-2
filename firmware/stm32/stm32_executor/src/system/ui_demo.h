#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "../ui/ui_state.h"

// DEMO 自检（原 sketch L205-212、L286-298、L1698-1905 原样搬运）。
//
// 职责：`NET:UI:DEMO` 触发的本地硬件自检——按固定步骤依次点亮 OLED/RGB、播 TTS、
//       开风扇、响蜂鸣器、锁定/解锁、模拟 RFID，把每个执行器都过一遍。
// 面试可讲：用一张 UiDemoStep 表驱动流程（表驱动思想，和 dispatcher 一致），
//       每步有独立 durationMs，靠 millis() 非阻塞推进，不用 delay 串行等待。
struct UiDemoStep {
  UiEventType type;
  const char *detail;
  uint16_t durationMs;
  int8_t fanState;
  bool beep;
  const char *ttsHex;
};

// uiDemoActive 被 dispatch 的 NET:UI:STATUS? 读取，以 extern 暴露。
extern bool uiDemoActive;

void startUiDemo();
void stopUiDemo();
void updateUiDemo();
