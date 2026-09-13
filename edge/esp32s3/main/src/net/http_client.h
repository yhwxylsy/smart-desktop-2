#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "../../config.h"

// HTTP 后端客户端（原 main.ino L484-540 原样搬运）。
//
// 职责：对后端 REST API 的统一 POST/GET JSON 封装。
// 面试可讲：根据 configStore::secure() 在 HTTP 与 HTTPS(WiFiClientSecure) 间切换，
// 并在请求头里带上 X-Device-Token 做设备鉴权（后端用它区分合法设备流量）。
// 端点路径串保留在各自调用方模块，本模块只提供通用 post/get。
bool postJson(const String &path, const String &body, String *responseOut = nullptr);
bool getJson(const String &path, String *responseOut = nullptr);
