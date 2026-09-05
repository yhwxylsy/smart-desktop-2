#include "dispatcher.h"
#include <string.h>
#include "../../config.h"
#include "../actuators/fan.h"
#include "../actuators/servo.h"
#include "../audio/buzzer.h"
#include "../audio/tts.h"
#include "../core/board.h"
#include "../core/text_util.h"
#include "../sensors/telemetry.h"
#include "../sensors/ultrasonic.h"
#include "../system/i2c_bus.h"
#include "../system/ui_demo.h"
#include "../system/user_context.h"
#include "../ui/oled.h"
#include "../ui/oled_screens.h"
#include "../ui/rgb.h"
#include "../ui/ui_state.h"

bool executeNetCommand(const String &command) {
  if (command == "NET:UI:LISTEN" ||
      command == "NET:UI:THINK" ||
      command == "NET:UI:ACTION" ||
      command == "NET:UI:ACK" ||
      command == "NET:UI:OUTPUT" ||
      command == "NET:UI:IDLE" ||
      command == "NET:UI:ERROR") {
    return true;
  }

  if (command.startsWith("NET:UI:USER:")) {
    return handleUserContextCommand(command.substring(strlen("NET:UI:USER:")));
  }

  if (command == "NET:UI:DEMO") {
    startUiDemo();
    return true;
  }

  if (command == "NET:UI:DEMO:STOP") {
    stopUiDemo();
    return true;
  }

  if (command == "NET:UI:STATUS?") {
    String line = "BT:UI:";
    line += "demo=";
    line += uiDemoActive ? "ON" : "OFF";
    line += ",oled=";
    line += oledAvailable ? "OK" : "MISS";
    line += ",addr=0x";
    line += String(oledAddress, HEX);
    line += ",screen=";
    line += String((uint8_t)infoScreen);
    line += ",fps=";
    line += String(oledFps);
    line += ",volume_pct=";
    line += String(speechVolumePercent);
    writeBack(line);
    return true;
  }

  if (command == "NET:UART?") {
    writeBack("BT:PONG:" + String(millis()));
    return true;
  }

  if (command == "NET:I2C?") {
    return handleI2cScanCommand();
  }

  if (command == "NET:TELEMETRY?") {
    sendTelemetrySnapshot();
    return true;
  }

  if (command == "NET:RGB:STATUS?") {
    writeBack(buildRgbStatusLine());
    return true;
  }

  if (command == "NET:RGB:LEGEND?") {
    writeBack("BT:RGB:LEGEND:GREEN=ready,CYAN=object_or_tracking,YELLOW=env_watch,RED=sensor_or_too_close,BLUE=waiting");
    return true;
  }

  if (command == "NET:RGB:MODE:SENSOR") {
    rgbSensorMode = true;
    updateLatestRgbStatus();
    writeBack("BT:RGB:MODE:SENSOR");
    return true;
  }

  if (command == "NET:RGB:MODE:EVENT") {
    rgbSensorMode = false;
    writeBack("BT:RGB:MODE:EVENT");
    return true;
  }

  if (command == "NET:ULTRASONIC:ON") {
    ultrasonicEnabled = true;
    writeBack("BT:ULTRASONIC:ON");
    return true;
  }

  if (command == "NET:ULTRASONIC:OFF") {
    ultrasonicEnabled = false;
    digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
    writeBack("BT:ULTRASONIC:OFF");
    return true;
  }

  if (command == "NET:TTS:STOP" || command == "NET:AUDIO:STOP") {
    return stopSpeechOutput();
  }

  if (command.startsWith("NET:TTS:")) {
    return speakText(command.substring(strlen("NET:TTS:")));
  }

  if (command.startsWith("NET:TTSHEX:")) {
    return speakHexText(command.substring(strlen("NET:TTSHEX:")));
  }

  if (command.startsWith("NET:VOLUME:")) {
    return setSpeechVolume(command.substring(strlen("NET:VOLUME:")), false);
  }

  if (command.startsWith("NET:OLED:")) {
    if (!oledAvailable) {
      initializeOled();
    }
    showOledText(command.substring(strlen("NET:OLED:")));
    return true;
  }

  if (command.startsWith("NET:RFID:")) {
    uiEvent.detail = String("RFID ") + compactForDisplay(command.substring(strlen("NET:RFID:")), 15);
    oledRenderPending = true;
    return true;
  }

  if (command == "NET:BEEP") {
    playBeep(2200, 120);
    return true;
  }

  if (command.startsWith("NET:MUSIC:")) {
    return startMusicByPreset(command.substring(strlen("NET:MUSIC:")));
  }

  if (command == "NET:MOTOR:OFF") {
    stopDrv8833();
    return true;
  }

  if (command.startsWith("NET:FAN:ON")) {
    return driveFanOn(fanLevelFromCommand(command));
  }

  if (command == "NET:FAN:OFF") {
    stopDrv8833();
    return true;
  }

  if (command == "NET:LOCK:ON") {
    currentLockOn = true;
    setRgb(true, false, false);
    return true;
  }

  if (command == "NET:LOCK:OFF") {
    currentLockOn = false;
    setRgb(false, true, false);
    return true;
  }

  if (command == "NET:AI:BUSY") {
    setRgb(true, true, false);
    return true;
  }

  if (command == "NET:AI:IDLE") {
    setRgb(false, true, false);
    return true;
  }

  if (command == "NET:AI:OFF") {
    setRgb(false, false, false);
    return true;
  }

  if (command.startsWith("NET:SERVO:")) {
    uint8_t angle = 0;
    if (!parseServoAngle(command.substring(strlen("NET:SERVO:")), angle)) {
      usbConsole.print("[SERVO] invalid angle ");
      usbConsole.println(command.substring(strlen("NET:SERVO:")));
      return false;
    }
    setServoAngle(angle);
    usbConsole.print("[SERVO] ");
    usbConsole.println(angle);
    return true;
  }

  usbConsole.print("[ERR] unsupported command: ");
  usbConsole.println(command);
  return false;
}
