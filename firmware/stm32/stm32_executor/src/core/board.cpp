#include "board.h"
#include "../../config.h"
#include "../protocol/command_line.h"

SoftwareSerial syn6288Serial(PB4, PB3);  // RX pin is a placeholder; PB3 is the SYN6288 TX line.
HardwareSerial usbConsole(PA3, PA2);    // HardwareSerial(0); USB/debug console (SWD/USB CDC)
HardwareSerial espCommandSerial(PB11, PB10);  // HardwareSerial(3); full-duplex link to ESP32S3

unsigned long lastEspRxActivityMs = 0;
static uint32_t espEmptyDelimiterCount = 0;

// 所有 BT:* 上行（ACK、遥测、按键事件）都从这里出去。
// 2026-09-06 起走 USART3 硬件 TX：不再有软件串口 bit-bang 造成的 CPU 长时间占用。
void writeBack(const String &line) {
  usbConsole.println(line);
  espCommandSerial.println(line);
}

void pollSerial(Stream &stream, String &buffer, const char *source) {
  while (stream.available()) {
    char ch = (char)stream.read();
    if (strcmp(source, "ESP") == 0) {
      lastEspRxActivityMs = millis();
    }
    if (ch == '\n' || ch == '\r') {
      if (buffer.length() == 0 && strcmp(source, "ESP") == 0) {
        espEmptyDelimiterCount++;
        if (espEmptyDelimiterCount <= 5 || (espEmptyDelimiterCount % 20) == 0) {
          usbConsole.print("[ESP] empty delimiter count=");
          usbConsole.println(espEmptyDelimiterCount);
        }
        continue;
      }
      handleLine(buffer, source);
      buffer = "";
    } else {
      buffer += ch;
    }
  }
}
