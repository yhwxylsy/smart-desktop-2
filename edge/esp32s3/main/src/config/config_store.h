#pragma once
#include <Arduino.h>
#include <Preferences.h>

// 配置状态与 NVS 读写（原 main.ino L32-37、L67-77、L79-130 原样搬运）
// 纯搬运阶段以 extern 暴露；去耦合阶段将改为 configStore 命名空间访问器
extern String wifiSsid;
extern String wifiPassword;
extern String serverHost;
extern String deviceToken;
extern uint16_t serverPort;
extern bool serverSecure;
extern Preferences prefs;

void saveConfig();
void loadConfig();
bool parseServerUrl(String value);
String httpBase();
String wsBase();
String wsPath();
