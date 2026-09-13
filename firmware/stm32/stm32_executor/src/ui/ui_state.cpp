#include "ui_state.h"
#include "../../config.h"
#include "../core/board.h"
#include "../core/text_util.h"
#include "oled.h"

UiEventState uiEvent;
UiMachineState uiMachineState = UI_STATE_BOOT;
uint32_t uiMachineStateStartedMs = 0;
InfoScreen infoScreen = INFO_SCREEN_MAIN;
uint32_t infoOverlayUntilMs = 0;
uint32_t ackFlashUntilMs = 0;
bool ackFlashOk = true;
bool commandLightMeasureActive = false;
bool commandHadLight = false;
uint32_t commandLightResponseMs = 0;
bool currentLockOn = false;

const char *eventLabel(UiEventType type) {
  switch (type) {
    case UI_EVENT_BOOT: return "BOOT";
    case UI_EVENT_DEMO: return "UI DEMO";
    case UI_EVENT_LISTEN: return "LISTEN";
    case UI_EVENT_THINK: return "THINK";
    case UI_EVENT_ACTION: return "ACTION";
    case UI_EVENT_ACK: return "ACK";
    case UI_EVENT_UART: return "UART";
    case UI_EVENT_I2C: return "I2C SCAN";
    case UI_EVENT_TELEMETRY: return "TELEMETRY";
    case UI_EVENT_OLED: return "OLED";
    case UI_EVENT_TTS: return "SYN6288 TTS";
    case UI_EVENT_FAN_ON: return "FAN ON";
    case UI_EVENT_FAN_OFF: return "FAN OFF";
    case UI_EVENT_BEEP: return "BEEP";
    case UI_EVENT_MUSIC: return "MUSIC";
    case UI_EVENT_LOCK_ON: return "LOCK ON";
    case UI_EVENT_LOCK_OFF: return "LOCK OFF";
    case UI_EVENT_AI_BUSY: return "AI BUSY";
    case UI_EVENT_AI_IDLE: return "AI IDLE";
    case UI_EVENT_AI_OFF: return "AI OFF";
    case UI_EVENT_SERVO: return "SERVO";
    case UI_EVENT_RFID: return "RFID";
    case UI_EVENT_ERROR: return "ERROR";
  }
  return "EVENT";
}

void enterUiMachineState(UiMachineState state, uint32_t startedMs) {
  uiMachineState = state;
  uiMachineStateStartedMs = startedMs;
  oledRenderPending = true;
}

// 事件 -> 状态迁移。面试可讲两点：
//   1. 守卫条件：LOCKED 态只放行 LOCK_OFF/ERROR/AI_OFF，其它事件被吞掉——
//      这保证了"锁屏"语义，RFID 未授权时即使有心跳/遥测也不会误解锁或改状态。
//   2. 多个事件归并到同一目标态（如 ACK/LOCK_OFF/AI_IDLE 都回 READY），
//      让"回到就绪"只写一次，避免状态爆炸。
void transitionUiMachineForEvent(UiEventType type, uint32_t startedMs) {
  if (uiMachineState == UI_STATE_LOCKED &&
      type != UI_EVENT_LOCK_OFF && type != UI_EVENT_ERROR && type != UI_EVENT_AI_OFF) {
    return;
  }

  switch (type) {
    case UI_EVENT_BOOT:
      enterUiMachineState(UI_STATE_BOOT, startedMs);
      break;
    case UI_EVENT_LOCK_ON:
      enterUiMachineState(UI_STATE_LOCKED, startedMs);
      break;
    case UI_EVENT_LOCK_OFF:
    case UI_EVENT_ACK:
    case UI_EVENT_AI_IDLE:
      enterUiMachineState(UI_STATE_READY, startedMs);
      break;
    case UI_EVENT_LISTEN:
      enterUiMachineState(UI_STATE_LISTENING, startedMs);
      break;
    case UI_EVENT_THINK:
    case UI_EVENT_AI_BUSY:
      enterUiMachineState(UI_STATE_PROCESSING, startedMs);
      break;
    case UI_EVENT_TTS:
      enterUiMachineState(UI_STATE_SPEAKING, startedMs);
      break;
    case UI_EVENT_ACTION:
    case UI_EVENT_FAN_ON:
    case UI_EVENT_FAN_OFF:
    case UI_EVENT_BEEP:
    case UI_EVENT_MUSIC:
    case UI_EVENT_SERVO:
    case UI_EVENT_DEMO:
      enterUiMachineState(UI_STATE_EXECUTING, startedMs);
      break;
    case UI_EVENT_ERROR:
    case UI_EVENT_AI_OFF:
      enterUiMachineState(UI_STATE_ERROR, startedMs);
      break;
    default:
      break;
  }
}

// 时间驱动的"自动回退"：某些状态是瞬态，待够时间就回到 READY。
// 例如 BOOT 显示 1.8s、EXECUTING 显示 2.2s、SPEAKING 显示 7s（因为 SYN6288 的
// BUSY 引脚没接，播报结束只能靠固定时长估算）。这是"没有硬件反馈时用时间兜底"的典型做法。
void updateUiMachineState(uint32_t now) {
  uint32_t age = now - uiMachineStateStartedMs;
  if ((uiMachineState == UI_STATE_BOOT && age >= UI_BOOT_HOLD_MS) ||
      (uiMachineState == UI_STATE_EXECUTING && age >= UI_TRANSIENT_HOLD_MS) ||
      (uiMachineState == UI_STATE_SPEAKING && age >= UI_SPEAK_HOLD_MS)) {
    enterUiMachineState(UI_STATE_READY, now);
  }
}

void beginUiEvent(UiEventType type, const String &detail, const String &actionId, bool wrapped, uint32_t startedMs) {
  uiEvent.type = type;
  uiEvent.detail = detail;
  uiEvent.actionId = actionId;
  uiEvent.startedMs = startedMs;
  uiEvent.parseMs = 0;
  uiEvent.actionMs = 0;
  uiEvent.lightMs = 0;
  uiEvent.ackMs = 0;
  uiEvent.hasLight = false;
  uiEvent.ackSeen = false;
  uiEvent.ackOk = false;
  uiEvent.wrapped = wrapped;
  transitionUiMachineForEvent(type, startedMs);
  oledRenderPending = true;
}

void finishUiEvent(uint32_t parseMs, uint32_t actionMs, uint32_t ackMs, bool ok) {
  uiEvent.parseMs = parseMs;
  uiEvent.actionMs = actionMs;
  uiEvent.lightMs = commandLightResponseMs;
  uiEvent.hasLight = commandHadLight;
  uiEvent.ackMs = ackMs;
  uiEvent.ackSeen = true;
  uiEvent.ackOk = ok;
  ackFlashOk = ok;
  ackFlashUntilMs = ok ? 0 : millis() + RGB_ACK_FLASH_MS;
  if (!ok) {
    enterUiMachineState(UI_STATE_ERROR, millis());
  }
  oledRenderPending = true;
}

const char *uiMachineCode(UiMachineState state) {
  switch (state) {
    case UI_STATE_BOOT: return "S0 BOOT";
    case UI_STATE_LOCKED: return "S1 LOCKED";
    case UI_STATE_READY: return "S2 READY";
    case UI_STATE_LISTENING: return "S3 LISTEN";
    case UI_STATE_PROCESSING: return "S4 PROCESS";
    case UI_STATE_SPEAKING: return "S5 SPEAK";
    case UI_STATE_EXECUTING: return "S6 EXEC";
    case UI_STATE_ERROR: return "S7 ERROR";
  }
  return "S7 ERROR";
}

const char *uiMachineTitle(UiMachineState state) {
  switch (state) {
    case UI_STATE_BOOT: return "正在启动";
    case UI_STATE_LOCKED: return "已锁定";
    case UI_STATE_READY: return "准备就绪";
    case UI_STATE_LISTENING: return "正在聆听";
    case UI_STATE_PROCESSING: return "正在思考";
    case UI_STATE_SPEAKING: return "正在播报";
    case UI_STATE_EXECUTING: return "正在执行";
    case UI_STATE_ERROR: return "操作失败";
  }
  return "操作失败";
}

const char *uiMachineCompactTitle(UiMachineState state) {
  switch (state) {
    case UI_STATE_BOOT: return "BOOT SELF-CHECK";
    case UI_STATE_LOCKED: return "ACCESS LOCKED";
    case UI_STATE_READY: return "READY";
    case UI_STATE_LISTENING: return "LISTEN / PTT";
    case UI_STATE_PROCESSING: return "AI THINKING";
    case UI_STATE_SPEAKING: return "SPEAKING";
    case UI_STATE_EXECUTING: return "EXECUTING";
    case UI_STATE_ERROR: return "ERROR";
  }
  return "ERROR";
}

void logEventTiming(const char *source, const ParsedCommand &parsed, bool ok) {
  usbConsole.print("[EVT] source=");
  usbConsole.print(source);
  usbConsole.print(" event=");
  usbConsole.print(eventLabel(uiEvent.type));
  usbConsole.print(" action_id=");
  usbConsole.print(parsed.actionId.length() > 0 ? parsed.actionId : "-");
  usbConsole.print(" status=");
  usbConsole.print(ok ? "OK" : "ERR");
  usbConsole.print(" parse_ms=");
  usbConsole.print(uiEvent.parseMs);
  usbConsole.print(" action_ms=");
  usbConsole.print(uiEvent.actionMs);
  usbConsole.print(" light_ms=");
  if (uiEvent.hasLight) {
    usbConsole.print(uiEvent.lightMs);
  } else {
    usbConsole.print("n/a");
  }
  usbConsole.print(" ack_total_ms=");
  usbConsole.print(uiEvent.ackMs);
  usbConsole.print(" detail=");
  usbConsole.println(compactForDisplay(uiEvent.detail, 40));
}
