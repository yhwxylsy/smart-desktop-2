#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "../ui/rgb.h"
#include "../ui/ui_state.h"
#include "protocol.h"

// 命令入口与粘包/前缀处理（原 sketch L593-681、L683-807、L2539-2617 原样搬运）。
//
// 职责：串口行缓冲 -> 命令的"预处理"层，包含三个面试高频点：
//   1. handleLine      ：真正从串口 buffer 进到业务的第一道门，负责把一行拆成可执行命令；
//                        重点在"脏前缀清洗 + 粘包拆分"（串口可能一次收到多条或带垃圾字节）。
//   2. classifyNetCommand / commandPreview ：把命令映射成 UI 事件类型 / 人类可读摘要，
//                        供 OLED 状态机和日志展示用，本身不改任何状态。
//   3. isKnownNetCommandStart / findKnownNetCommandStart ：按已知前缀在字符串里定位命令起点，
//                        是脏前缀清洗与粘包拆分的核心工具。
//
// 去耦合阶段将把 classifyNetCommand/commandPreview/isKnownNetCommandStart 前缀表
// 与 dispatcher 的命令分支合并为单一 command_table。
bool stringMatchesAt(const String &text, int index, const char *prefix);
bool isKnownNetCommandStart(const String &text, int index);
int findKnownNetCommandStart(const String &text, int fromIndex);
UiEventType classifyNetCommand(const String &command, const String &actionId);
String commandPreview(const String &command);
void handleLine(String line, const char *source);
