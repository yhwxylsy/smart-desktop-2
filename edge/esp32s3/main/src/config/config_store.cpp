#include "config_store.h"
#include "../../config.h"
#include <Preferences.h>

Preferences prefs;

String wifiSsid;
String wifiPassword;
String serverHost;
String deviceToken = String(SMARTDESK_DEVICE_TOKEN);
uint16_t serverPort = 8082;
bool serverSecure = false;

String httpBase() {
  return String(serverSecure ? "https://" : "http://") + serverHost + ":" + String(serverPort);
}

String wsBase() {
  return String(serverSecure ? "wss://" : "ws://") + serverHost + ":" + String(serverPort);
}

String wsPath() {
  return "/api/realtime/ws?device_id=" + String(DEVICE_ID) + "&edge_id=" + String(EDGE_ID);
}

void saveConfig() {
  prefs.begin("smartdesk", false);
  prefs.putString("wifi_ssid", wifiSsid);
  prefs.putString("wifi_pass", wifiPassword);
  prefs.putString("server_host", serverHost);
  prefs.putUShort("server_port", serverPort);
  prefs.putBool("server_secure", serverSecure);
  prefs.putString("device_token", deviceToken);
  prefs.end();
}

void loadConfig() {
  prefs.begin("smartdesk", true);
  wifiSsid = prefs.getString("wifi_ssid", "");
  wifiPassword = prefs.getString("wifi_pass", "");
  serverHost = prefs.getString("server_host", "");
  serverPort = prefs.getUShort("server_port", 8082);
  serverSecure = prefs.getBool("server_secure", false);
  deviceToken = prefs.getString("device_token", SMARTDESK_DEVICE_TOKEN);
  prefs.end();
}

bool parseServerUrl(String value) {
  value.trim();
  bool secure = false;
  if (value.startsWith("https://")) {
    secure = true;
    value.remove(0, strlen("https://"));
  } else if (value.startsWith("wss://")) {
    secure = true;
    value.remove(0, strlen("wss://"));
  } else if (value.startsWith("http://")) {
    value.remove(0, strlen("http://"));
  } else if (value.startsWith("ws://")) {
    value.remove(0, strlen("ws://"));
  }
  int slash = value.indexOf('/');
  if (slash >= 0) {
    value = value.substring(0, slash);
  }
  int colon = value.lastIndexOf(':');
  if (colon >= 0) {
    serverHost = value.substring(0, colon);
    serverPort = (uint16_t)value.substring(colon + 1).toInt();
  } else {
    serverHost = value;
    serverPort = secure ? 443 : 8082;
  }
  serverHost.trim();
  serverSecure = secure;
  return serverHost.length() > 0 && serverPort > 0;
}
