#include "i2c_bus.h"
#include <Wire.h>
#include "../../config.h"
#include "../core/board.h"
#include "../ui/oled.h"
#include "../ui/ui_state.h"

// 扫描 I2C 总线：对 1~0x77 每个地址发一次空写，endTransmission()==0 说明设备有 ACK。
// 这是最快确认 OLED(0x3D/0x3C) 和 AHT20(0x38) 是否在线的办法，结果打到 USB 口。
uint8_t scanI2cBus() {
  uint8_t found = 0;
  usbConsole.println("[I2C] scan start");
  for (uint8_t address = 1; address < 0x78; ++address) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      found++;
      usbConsole.print("[I2C] found 0x");
      if (address < 0x10) {
        usbConsole.print("0");
      }
      usbConsole.println(address, HEX);
    } else if (error == 4) {
      usbConsole.print("[I2C] unknown error at 0x");
      if (address < 0x10) {
        usbConsole.print("0");
      }
      usbConsole.println(address, HEX);
    }
  }
  if (found == 0) {
    usbConsole.println("[I2C] no devices found");
  }
  usbConsole.print("[I2C] scan done count=");
  usbConsole.println(found);
  return found;
}

// `NET:I2C?` 的 handler：先扫描，若 OLED 之前没就绪但在两个候选地址(0x3D/0x3C)
// 之一探测到了，就顺手把它重新初始化——让"拔插 OLED 后"能靠一条命令自愈。
bool handleI2cScanCommand() {
  uint8_t found = scanI2cBus();
  if (!oledAvailable && (oledProbe(OLED_ADDR_PRIMARY) || oledProbe(OLED_ADDR_FALLBACK))) {
    initializeOled();
    oledRenderPending = true;
  }
  uiEvent.detail = String("I2C FOUND ") + String(found);
  oledRenderPending = true;
  return found > 0;
}
