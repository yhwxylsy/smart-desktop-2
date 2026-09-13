#pragma once
#include <Arduino.h>
#include "../../config.h"

// Wi-Fi 连接与网络诊断（原 main.ino L728-813、L1592-1635 原样搬运）
//
// 职责：ESP32S3 的 WiFi 连接管理 + 网络诊断工具。
// 面试可讲 connectWifi 的加速技巧：先 scanTargetWifi 扫到目标 AP 的 BSSID 和信道，
// 再用 `WiFi.begin(ssid, pwd, channel, bssid)` 直接定向连接，跳过全信道扫描，
// 大幅缩短配网/重连时间；并开 autoReconnect + 关 sleep 保证实时链路的稳定。
bool scanTargetWifi(int32_t *channelOut, uint8_t bssidOut[6], int32_t *rssiOut, bool logResult = true);
void connectWifi();
void scanWifi();
void probeTcp(String target);  // 网络诊断：DNS 解析 + TCP 连通性测试
