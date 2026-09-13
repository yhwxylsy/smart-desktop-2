#pragma once
#include <Arduino.h>
#include "../../config.h"

// 协议解析（原 sketch L214-218、L328-358 原样搬运）。
//
// 职责：协议层最底层的"进"与"出"——把一行串口命令拆成结构化数据（parseLine），
//       再根据执行结果生成回执（sendAck）。
//
// 两种命令形态（面试可讲）：
//   1. 直接命令 `NET:FAN:ON`：调试/兼容用，回 `BT:OK` / `BT:ERR`。
//   2. 包装命令 `NET:CMD:<action_id>:NET:FAN:ON`：生产主链路用。后端给每个动作
//      分配唯一 action_id，STM32 执行后回 `BT:ACK:<action_id>:OK/ERR`，后端据此把
//      回执与它下发的动作一一对应——这是"指令->执行->ACK"异步闭环的关键。
//
// 设计要点：parseLine 只做"剥壳"，不判断命令是否合法；合法性交给 dispatcher 的
// 命令表去匹配。职责单一，便于用纯函数做单元测试（backend/tests）。
struct ParsedCommand {
  String actionId;   // 包装命令里的 action_id（直接命令为空）
  String command;    // 剥掉包装后的真实命令（如 NET:FAN:ON）
  bool wrapped;      // 是否为包装命令
};

ParsedCommand parseLine(String line);
void sendAck(const ParsedCommand &parsed, bool ok);
