#pragma once
#include <Arduino.h>
#include <MFRC522.h>
#include "../../config.h"

// RC522 读卡器与扫卡上报（原 main.ino L1637-1952 原样搬运）。
//
// 职责：SPI 接口的 RC522（MFRC522）读卡，扫到卡后把 UID 上报后端做鉴权，并提示 STM32。
// 面试可讲两点：
//   1. 协议：MFRC522 走 SPI，读卡流程是"寻卡(WakeupA/IsNewCardPresent) -> 防冲突 ->
//      读序列号(ReadCardSerial) -> HaltA 停卡"；
//   2. 工程细节：pollRfid 做了"防重复触发"（同一 UID 在 RFID_REPEAT_SUPPRESS_MS 内只报一次）
//      和"读卡器健康自愈"（定期读寄存器，发现异常就重新初始化）。
extern MFRC522 rfid;

bool rfidVersionHealthy(byte version);
byte initializeRfidReader();
void printRfidStatus(bool probeCard);
String uidToString(MFRC522::Uid *uid);
bool rfidCardReady();
void pollRfid();
