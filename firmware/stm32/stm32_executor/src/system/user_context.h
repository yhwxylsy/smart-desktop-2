#pragma once
#include <Arduino.h>
#include "../../config.h"

// 用户上下文（原 sketch L276-278、L2332-2346 原样搬运）。
//
// 职责：保存"当前是谁在用设备"——用户 ID、RFID 卡 UID、模式（STUDY/…）。
// 数据来自后端下发的 `NET:UI:USER:<id>:<uid>:<mode>`，用于 OLED 展示和交互提示。
// 注意：这里只做展示层缓存，真正的鉴权在后端/ESP32 侧完成，STM32 不负责安全判定。
extern String currentUserId;
extern String currentCardUid;
extern String currentUserMode;

bool handleUserContextCommand(const String &payload);
