#include "protocol.h"
#include "../../config.h"
#include "../core/board.h"

// 生成 ACK 回执。核心逻辑是"回执必须能指回原动作"：
// 包装命令带 action_id 时回 `BT:ACK:<id>:OK/ERR`，否则回裸的 `BT:OK`/`BT:ERR`。
void sendAck(const ParsedCommand &parsed, bool ok) {
  if (parsed.wrapped && parsed.actionId.length() > 0) {
    writeBack("BT:ACK:" + parsed.actionId + ":" + (ok ? "OK" : "ERR"));
  } else {
    writeBack(ok ? "BT:OK" : "BT:ERR");
  }
}

// 把一行命令解析成 ParsedCommand。
// 输入示例：`NET:CMD:act_123:NET:FAN:ON`
//   -> actionId = "act_123"，command = "NET:FAN:ON"，wrapped = true。
// 非包装命令（如 `NET:FAN:ON`）直接整行作为 command，wrapped = false。
// 注意：这里只剥壳，不做合法性校验；校验在 dispatcher 的命令表里完成。
ParsedCommand parseLine(String line) {
  line.trim();
  ParsedCommand parsed;
  parsed.actionId = "";
  parsed.command = line;
  parsed.wrapped = false;

  // 不是包装命令，直接整行返回。
  if (!line.startsWith("NET:CMD:")) {
    return parsed;
  }

  // 包装命令格式：NET:CMD:<action_id>:NET:<真实命令>
  int idStart = strlen("NET:CMD:");
  // 从 idStart 往后找第二个 ":NET:"，它前面的就是 action_id。
  int commandStart = line.indexOf(":NET:", idStart);
  if (commandStart < 0) {
    // 只有 NET:CMD: 前缀却缺 :NET:，视为非法，command 置空让上层报解析错误。
    parsed.command = "";
    return parsed;
  }

  parsed.wrapped = true;
  parsed.actionId = line.substring(idStart, commandStart);
  parsed.command = line.substring(commandStart + 1);  // 跳过 ':'
  return parsed;
}
