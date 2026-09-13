#pragma once
#include <Arduino.h>
#include <WebSocketsClient.h>
#include "../../config.h"

// WebSocket 实时链路（原 main.ino L670-726、L815-841 原样搬运）。
//
// 职责：与后端保持 WebSocket 长连接，接收"命令下发/播报/心跳"三类消息并转发给 STM32。
// 面试可讲：相比 HTTP 轮询，WebSocket 是"服务端主动推"的全双工长连接，实时性更好；
// 这里设了重连间隔(5s) + 应用层心跳(15s ping/3s 超时)，断线能自动恢复。
// 消息协议是 JSON：`{"type":"stm32/commands","lines":[...]}` 下发命令、
// `{"type":"speak","text":"..."}` 触发播报、`{"type":"ping"}` 回 pong。
// webSocket / wsConnected / wsStarted 由本模块独占，供后端桥接与 CLI 共享。
extern WebSocketsClient webSocket;
extern bool wsConnected;
extern bool wsStarted;

void handleWsText(const uint8_t *payload, size_t length);
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);
void startWebSocket();
bool pauseWebSocketForMicUpload();
