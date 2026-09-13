#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "../net/websocket_link.h"

// 后端命令桥接（原 main.ino L542-611、L1492-1494 原样搬运）。
//
// 职责：在后端 <-> STM32 之间搬运"命令下发"和"ACK/按键事件回传"。
// 面试可讲"双通道容错"：
//   1. 命令下发：优先走 WebSocket 实时推送；WS 未连时降级为 HTTP 轮询
//      （pollBackendCommands 定时 GET /api/hardware/commands）——保证没 WS 也能跑；
//   2. 回传：sendAckToBackend 先试 HTTP POST，失败再退 WS 发 `{"type":"ack"}`。
// 这样任何一条通道出问题，另一条都能兜住，答辩现场更稳。
// 复用 websocket_link 暴露的 webSocket / wsConnected。
void forwardCommandsFromJson(const String &jsonText);
void pollBackendCommands();
void sendAckToBackend(const String &line);
void sendButtonEventToBackend(const String &line);
void handleStm32ButtonEvent(const String &line);
