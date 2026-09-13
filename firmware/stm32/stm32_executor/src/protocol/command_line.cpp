#include "command_line.h"
#include <string.h>
#include "../../config.h"
#include "../core/board.h"
#include "../core/text_util.h"
#include "../ui/rgb.h"
#include "../ui/ui_state.h"
#include "dispatcher.h"
#include "protocol.h"

// 把一条命令映射成一个 UI 事件类型（UiEventType），供 OLED 状态机做界面切换。
// 注意：它只"归类"，不执行命令——执行在 dispatcher 的命令表里。
// 面试可讲：用命令字符串前缀做分类，本质是一个优先级顺序的 if 链；
// 开头用 containsUpperToken 先抓"任何位置含 RFID"的兜底，再精确匹配 UI/TTS/传感器等。
UiEventType classifyNetCommand(const String &command, const String &actionId) {
  if (containsUpperToken(command, "RFID") || containsUpperToken(actionId, "RFID")) {
    return UI_EVENT_RFID;
  }
  if (command == "NET:UI:LISTEN") {
    return UI_EVENT_LISTEN;
  }
  if (command == "NET:UI:THINK") {
    return UI_EVENT_THINK;
  }
  if (command == "NET:UI:ACTION") {
    return UI_EVENT_ACTION;
  }
  if (command == "NET:UI:ACK") {
    return UI_EVENT_ACK;
  }
  if (command == "NET:UI:OUTPUT") {
    return UI_EVENT_AI_IDLE;
  }
  if (command == "NET:UI:IDLE") {
    return UI_EVENT_AI_IDLE;
  }
  if (command == "NET:UI:ERROR") {
    return UI_EVENT_ERROR;
  }
  if (command == "NET:UI:DEMO") {
    return UI_EVENT_DEMO;
  }
  if (command == "NET:UI:DEMO:STOP" || command == "NET:UI:STATUS?") {
    return UI_EVENT_TELEMETRY;
  }
  if (command.startsWith("NET:UI:USER:")) {
    return UI_EVENT_RFID;
  }
  if (command.startsWith("NET:UI:")) {
    return UI_EVENT_ERROR;
  }
  if (command == "NET:UART?") {
    return UI_EVENT_UART;
  }
  if (command == "NET:I2C?") {
    return UI_EVENT_I2C;
  }
  if (command == "NET:TELEMETRY?") {
    return UI_EVENT_TELEMETRY;
  }
  if (command.startsWith("NET:RGB:")) {
    return UI_EVENT_TELEMETRY;
  }
  if (command == "NET:TTS:STOP" || command == "NET:AUDIO:STOP" ||
      command.startsWith("NET:TTS:") || command.startsWith("NET:TTSHEX:") ||
      command.startsWith("NET:VOLUME:")) {
    return UI_EVENT_TTS;
  }
  if (command.startsWith("NET:OLED:")) {
    return UI_EVENT_OLED;
  }
  if (command == "NET:BEEP") {
    return UI_EVENT_BEEP;
  }
  if (command.startsWith("NET:MUSIC:")) {
    return UI_EVENT_MUSIC;
  }
  if (command.startsWith("NET:FAN:ON")) {
    return UI_EVENT_FAN_ON;
  }
  if (command == "NET:FAN:OFF") {
    return UI_EVENT_FAN_OFF;
  }
  if (command == "NET:LOCK:ON") {
    return UI_EVENT_LOCK_ON;
  }
  if (command == "NET:LOCK:OFF") {
    return UI_EVENT_LOCK_OFF;
  }
  if (command == "NET:AI:BUSY") {
    return UI_EVENT_AI_BUSY;
  }
  if (command == "NET:AI:IDLE") {
    return UI_EVENT_AI_IDLE;
  }
  if (command == "NET:AI:OFF") {
    return UI_EVENT_AI_OFF;
  }
  if (command.startsWith("NET:SERVO:")) {
    return UI_EVENT_SERVO;
  }
  return UI_EVENT_ERROR;
}

// 把命令压缩成一段短的可读摘要，用于 OLED 副屏和 USB 日志展示。
// 例如 `NET:TTSHEX:E4BDA0E5A5BD` -> "TTSHEX 6B"，避免把长 payload 直接怼上屏幕。
String commandPreview(const String &command) {
  if (command == "NET:TTS:STOP" || command == "NET:AUDIO:STOP") {
    return "TTS STOP";
  }
  if (command.startsWith("NET:UI:USER:")) {
    return String("USER ") + compactForDisplay(command.substring(strlen("NET:UI:USER:")), 14);
  }
  if (command.startsWith("NET:VOLUME:")) {
    return String("VOLUME ") + compactForDisplay(command.substring(strlen("NET:VOLUME:")), 4);
  }
  if (command.startsWith("NET:TTSHEX:")) {
    uint16_t hexChars = command.length() - strlen("NET:TTSHEX:");
    return String("TTSHEX ") + String(hexChars / 2) + "B";
  }
  if (command.startsWith("NET:TTS:")) {
    return String("TTS ") + compactForDisplay(command.substring(strlen("NET:TTS:")), 16);
  }
  if (command.startsWith("NET:OLED:")) {
    return String("OLED ") + compactForDisplay(command.substring(strlen("NET:OLED:")), 16);
  }
  if (command.startsWith("NET:FAN:ON")) {
    return "FAN ON";
  }
  if (command == "NET:FAN:OFF") {
    return "FAN OFF";
  }
  if (command == "NET:BEEP") {
    return "BEEP";
  }
  if (command.startsWith("NET:MUSIC:")) {
    return String("MUSIC ") + compactForDisplay(command.substring(strlen("NET:MUSIC:")), 12);
  }
  if (command.startsWith("NET:LOCK:")) {
    return command.substring(strlen("NET:"));
  }
  if (command.startsWith("NET:AI:")) {
    return command.substring(strlen("NET:"));
  }
  if (command.startsWith("NET:UI:")) {
    return command.substring(strlen("NET:"));
  }
  if (command == "NET:UART?") {
    return "UART PING";
  }
  if (command == "NET:I2C?") {
    return "I2C SCAN";
  }
  if (command == "NET:TELEMETRY?") {
    return "TELEMETRY";
  }
  if (command.startsWith("NET:RGB:")) {
    return String("RGB ") + compactForDisplay(command.substring(strlen("NET:RGB:")), 12);
  }
  if (command.startsWith("NET:ULTRASONIC:")) {
    return String("ULTRASONIC ") + compactForDisplay(command.substring(strlen("NET:ULTRASONIC:")), 10);
  }
  if (command.startsWith("NET:MOTOR:")) {
    return String("MOTOR ") + compactForDisplay(command.substring(strlen("NET:MOTOR:")), 12);
  }
  if (command.startsWith("NET:SERVO:")) {
    return String("SERVO ") + compactForDisplay(command.substring(strlen("NET:SERVO:")), 12);
  }
  return compactForDisplay(command, 20);
}

// 判断 text 的 index 处是否正好以 prefix 开头（逐字符比对，避免 substring 的堆开销）。
bool stringMatchesAt(const String &text, int index, const char *prefix) {
  size_t prefixLen = strlen(prefix);
  if (index < 0 || index + (int)prefixLen > (int)text.length()) {
    return false;
  }
  for (size_t i = 0; i < prefixLen; ++i) {
    if (text.charAt(index + i) != prefix[i]) {
      return false;
    }
  }
  return true;
}

// 判断 text[index] 是否是一个合法命令的起点。
// 面试可讲：这里有两个精妙点——
//   1. 用一张已知前缀表（prefixes[]）逐项匹配，而不是硬编码一堆 if；
//   2. `index>0 && 前一个字符是 ':'` 则直接否决——避免把包装命令
//      `NET:CMD:id:NET:FAN:ON` 里的第二个 ":NET:" 误判成新命令起点。
bool isKnownNetCommandStart(const String &text, int index) {
  if (index < 0 || index >= (int)text.length()) {
    return false;
  }
  if (index > 0 && text.charAt(index - 1) == ':') {
    return false;
  }

  static const char *prefixes[] = {
    "NET:CMD:",
    "NET:UART?",
    "NET:I2C?",
    "NET:TELEMETRY?",
    "NET:ULTRASONIC:",
    "NET:MOTOR:",
    "NET:TTS:",
    "NET:TTSHEX:",
    "NET:AUDIO:",
    "NET:VOLUME:",
    "NET:OLED:",
    "NET:BEEP",
    "NET:MUSIC:",
    "NET:FAN:",
    "NET:LOCK:",
    "NET:AI:",
    "NET:UI:",
    "NET:RGB:",
    "NET:SERVO:",
    "NET:RFID:"
  };

  for (uint8_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i) {
    if (stringMatchesAt(text, index, prefixes[i])) {
      return true;
    }
  }
  return false;
}

int findKnownNetCommandStart(const String &text, int fromIndex) {
  for (int i = fromIndex; i < (int)text.length(); ++i) {
    if (isKnownNetCommandStart(text, i)) {
      return i;
    }
  }
  return -1;
}

// 处理"单条"命令的完整流程：解析 -> 分类 -> 执行 -> ACK -> 时序日志。
// 面试可讲：这里用 millis() 打了三个时间戳（receivedMs/parseMs/actionMs），
// 把"解析耗时、动作耗时、端到端耗时"都记进 USB 日志，是排查链路延迟的手段。
void handleSingleCommandLine(String line, const char *source) {
  uint32_t receivedMs = millis();
  if (strcmp(source, "ESP") == 0) {
    lastEspRxActivityMs = receivedMs;
  }
  line.trim();
  if (line.length() == 0) {
    return;
  }

  usbConsole.print("[");
  usbConsole.print(source);
  usbConsole.print("] ");
  usbConsole.println(line);

  uint32_t parseStartedMs = millis();
  ParsedCommand parsed = parseLine(line);
  uint32_t parseMs = millis() - parseStartedMs;
  if (parsed.command.length() == 0) {
    commandLightMeasureActive = false;
    commandHadLight = false;
    commandLightResponseMs = 0;
    beginUiEvent(UI_EVENT_ERROR, "PARSE ERR", parsed.actionId, parsed.wrapped, receivedMs);
    sendAck(parsed, false);
    finishUiEvent(parseMs, 0, millis() - receivedMs, false);
    logEventTiming(source, parsed, false);
    return;
  }

  beginUiEvent(classifyNetCommand(parsed.command, parsed.actionId), commandPreview(parsed.command),
               parsed.actionId, parsed.wrapped, receivedMs);
  commandHadLight = false;
  commandLightResponseMs = 0;
  commandLightMeasureActive = true;
  RgbState cueRgb = uiMachineBaseRgb();
  setRgb(cueRgb.red, cueRgb.green, cueRgb.blue);
  uint32_t actionStartedMs = millis();
  bool ok = executeNetCommand(parsed.command);
  uint32_t actionMs = millis() - actionStartedMs;
  commandLightMeasureActive = false;
  sendAck(parsed, ok);
  finishUiEvent(parseMs, actionMs, millis() - receivedMs, ok);
  logEventTiming(source, parsed, ok);
}

// 命令入口：从串口收到"一行"后的统一处理。解决串口通信的两大经典问题——
//   1. 脏前缀：上行链路偶发把垃圾字节（如 0xFE）拼在命令前面，
//      这里用 findKnownNetCommandStart 找到第一个合法命令起点，把前面的脏字节丢弃。
//   2. 粘包：一次读到多条命令连在一起（如 `NET:FAN:ONNET:BEEP`），
//      这里循环按"下一个合法起点"拆段，逐条处理。
// 面试可讲：这是"以协议前缀做同步头"的容错思路，不需要固定帧长/校验也能自愈。
void handleLine(String line, const char *source) {
  line.trim();
  if (line.length() == 0) {
    return;
  }

  // 脏前缀清洗：第一个合法命令起点不在 0，说明前面有垃圾字节，整体前移丢弃。
  int firstStart = findKnownNetCommandStart(line, 0);
  if (firstStart > 0) {
    usbConsole.print("[WARN] dropping serial prefix bytes=");
    usbConsole.println(firstStart);
    line = line.substring(firstStart);
  }

  // 从 1 开始找第二个命令起点，找不到说明这一行只有一条命令。
  int nextStart = findKnownNetCommandStart(line, 1);
  if (nextStart < 0) {
    handleSingleCommandLine(line, source);
    return;
  }

  // 粘包拆分：一行里有多条命令，逐段切开分别处理。
  usbConsole.println("[WARN] split concatenated NET commands");
  int currentStart = 0;
  while (currentStart >= 0 && currentStart < (int)line.length()) {
    int followingStart = findKnownNetCommandStart(line, currentStart + 1);
    String segment = followingStart >= 0 ? line.substring(currentStart, followingStart) : line.substring(currentStart);
    segment.trim();
    if (segment.length() > 0) {
      handleSingleCommandLine(segment, source);
    }
    if (followingStart < 0) {
      break;
    }
    currentStart = followingStart;
  }
}
