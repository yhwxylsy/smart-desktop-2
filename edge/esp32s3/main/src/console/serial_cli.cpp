#include "serial_cli.h"
#include <ArduinoJson.h>
#include "../../config.h"
#include "../config/config_store.h"
#include "../core/hex_util.h"
#include "../net/wifi_manager.h"
#include "../net/websocket_link.h"
#include "../rfid/rfid_reader.h"
#include "../bridge/stm32_link.h"
#include "../mic/mic_pipeline.h"

static String usbLine;

void handleSerialCommand(String line) {
  line.trim();
  if (line.isEmpty()) {
    return;
  }

  if (line.startsWith("CFG:WIFI:")) {
    String value = line.substring(strlen("CFG:WIFI:"));
    if (value == "SHOW") {
      Serial.print("[CFG] ssid=");
      Serial.print(wifiSsid);
      Serial.print(" server=");
      Serial.print(serverSecure ? "https://" : "http://");
      Serial.print(serverHost);
      Serial.print(":");
      Serial.println(serverPort);
    } else if (value == "SCAN") {
      scanWifi();
    } else {
      int comma = value.indexOf(',');
      if (comma < 0) {
        Serial.println("[CFG] use CFG:WIFI:<ssid>,<password>");
      } else {
        wifiSsid = value.substring(0, comma);
        wifiPassword = value.substring(comma + 1);
        saveConfig();
        Serial.println("[CFG] wifi saved");
        WiFi.disconnect(true);
        delay(200);
        connectWifi();
        startWebSocket();
      }
    }
    return;
  }

  if (line.startsWith("CFG:NET:TCP:")) {
    probeTcp(line.substring(strlen("CFG:NET:TCP:")));
    return;
  }

  if (line.startsWith("CFG:SERVER:")) {
    if (parseServerUrl(line.substring(strlen("CFG:SERVER:")))) {
      saveConfig();
      wsStarted = false;
      webSocket.disconnect();
      Serial.println("[CFG] server saved");
      startWebSocket();
    } else {
      Serial.println("[CFG] invalid server");
    }
    return;
  }

  if (line.startsWith("CFG:TOKEN:")) {
    deviceToken = line.substring(strlen("CFG:TOKEN:"));
    deviceToken.trim();
    saveConfig();
    Serial.println(deviceToken.length() > 0 ? "[CFG] device token saved" : "[CFG] device token cleared");
    return;
  }

  if (line == "CFG:RESET") {
    prefs.begin("smartdesk", false);
    prefs.clear();
    prefs.end();
    wifiSsid = "";
    wifiPassword = "";
    serverHost = "";
    deviceToken = String(SMARTDESK_DEVICE_TOKEN);
    serverSecure = false;
    Serial.println("[CFG] cleared");
    return;
  }

  if (line == "CFG:UART:PING") {
    sendToStm32("NET:UART?");
    return;
  }

  if (line == "CFG:RFID:STATUS") {
    printRfidStatus(true);
    return;
  }

  if (line == "CFG:RFID:RESET") {
    byte version = initializeRfidReader();
    Serial.printf("[RFID] reinitialized version=0x%02X\n", version);
    printRfidStatus(true);
    return;
  }

  if (line == "CFG:MIC:REC:CUE") {
    captureAndUploadMicAfterCue("");
    return;
  }

  if (line == "CFG:MIC:REC:CUE:ASRONLY") {
    captureAndUploadMicAfterCue("", false, "esp32_mic_asr_test");
    return;
  }

  if (line.startsWith("CFG:MIC:REC:CUE:")) {
    captureAndUploadMicAfterCue(line.substring(strlen("CFG:MIC:REC:CUE:")));
    return;
  }

  if (line == "CFG:MIC:REC:ASRONLY") {
    captureAndUploadMic(false, "esp32_mic_asr_test");
    return;
  }

  if (line == "CFG:MIC:REC") {
    captureAndUploadMic();
    return;
  }

  if (line.startsWith("CFG:MIC:SELFTEST")) {
    String phrase = "";
    if (line.startsWith("CFG:MIC:SELFTEST:")) {
      phrase = line.substring(strlen("CFG:MIC:SELFTEST:"));
    }
    runMicSelfTest(phrase);
    return;
  }

  if (line.startsWith("CFG:TTS:")) {
    sendToStm32("NET:TTSHEX:" + utf8Hex(line.substring(strlen("CFG:TTS:"))));
    return;
  }

  if (line.startsWith("CFG:OLED:")) {
    sendToStm32("NET:OLED:" + line.substring(strlen("CFG:OLED:")));
    return;
  }

  if (line.startsWith("CHAT:")) {
    JsonDocument doc;
    doc["type"] = "text";
    doc["text"] = line.substring(strlen("CHAT:"));
    String body;
    serializeJson(doc, body);
    if (wsConnected) {
      webSocket.sendTXT(body);
    } else {
      Serial.println("[CHAT] websocket not connected");
    }
    return;
  }

  Serial.println("[CFG] unknown command");
}

void pollUsbSerial() {
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\n' || ch == '\r') {
      handleSerialCommand(usbLine);
      usbLine = "";
    } else {
      usbLine += ch;
    }
  }
}
