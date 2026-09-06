#include "watchdog.h"
#include <IWatchdog.h>
#include "../../config.h"
#include "../core/board.h"

static bool watchdogArmed = false;

void setupWatchdog() {
#if WATCHDOG_ENABLED_BY_DEFAULT
  // isReset(true)：读取本次上电是否由看门狗引起，并顺带清除标志，避免下次误判。
  if (IWatchdog.isReset(true)) {
    usbConsole.println("[WDT] previous reset came from the watchdog (loop() stalled)");
  }
  IWatchdog.begin(WATCHDOG_TIMEOUT_MS);
  watchdogArmed = true;
  usbConsole.print("[WDT] armed timeout_ms=");
  usbConsole.println((unsigned long)WATCHDOG_TIMEOUT_MS);
#else
  usbConsole.println("[WDT] disabled (WATCHDOG_ENABLED_BY_DEFAULT=0)");
#endif
}

void feedWatchdog() {
#if WATCHDOG_ENABLED_BY_DEFAULT
  if (watchdogArmed) {
    IWatchdog.reload();
  }
#endif
}
