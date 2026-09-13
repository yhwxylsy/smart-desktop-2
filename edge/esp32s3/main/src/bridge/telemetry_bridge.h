#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "../net/heartbeat.h"

// 遥测桥接与白名单（原 main.ino L613-668 原样搬运）。
//
// 职责：把 STM32 上报的遥测 JSON（`BT:{...}`）做"字段白名单过滤"后转发给后端。
// 面试可讲：白名单 isAllowedTelemetryKey 只放行约定好的 20 个字段，既防止 STM32
// 侧未来误报垃圾字段污染后端数据，也统一了"设备 -> 云"的数据契约；
// 上报时还会补上 device_id/edge_id/voice_state 和本机 WiFi 信号强度。
// 复用 heartbeat 暴露的 voiceState。
bool isAllowedTelemetryKey(const char *key);
void sendTelemetryToBackend(String line);
