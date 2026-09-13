#pragma once
#include <Arduino.h>
#include "../../config.h"

// 华为云 IoTDA MQTT 客户端（原 main.ino L69-76、L253-482 整块搬运）。
//
// 职责：对接华为云 IoTDA 平台（可选，默认关闭）。通过 MQTT 上报设备属性、订阅命令下发。
// 面试可讲三个点：
//   1. 鉴权：IoTDA 的设备接入用 HMAC-SHA256(timestamp, secret) 当 MQTT password，
//      clientId 按 `<device_id>_0_0_<timestamp>` 规范拼接；
//   2. 主题：属性上报 `$oc/devices/<id>/sys/properties/report`，命令订阅
//      `$oc/devices/<id>/sys/commands/#`，响应回 `.../commands/response/request_id=...`；
//   3. 编译隔离：整文件在 SMARTDESK_IOTDA_ENABLED=0 时被 #if 整体剔除，不影响默认链路。
// 整文件在 SMARTDESK_IOTDA_ENABLED 关闭时不参与编译，运行行为完全不变。
#if SMARTDESK_IOTDA_ENABLED
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#if SMARTDESK_IOTDA_PORT == 1883
extern WiFiClient iotdaClient;
#else
extern WiFiClientSecure iotdaClient;
#endif
extern PubSubClient iotdaMqtt;

void iotdaLoop();
void publishTelemetryToIotda(JsonObject sensors);
#endif
