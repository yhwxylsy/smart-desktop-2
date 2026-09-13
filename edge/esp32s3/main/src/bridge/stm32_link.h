#pragma once
#include <Arduino.h>
#include "../../config.h"

// STM32 串口链路（原 main.ino L64-65、L231-251、L1578-1590、L1830-1871 原样搬运）。
//
// 职责：ESP32S3 <-> STM32 的串口桥接。物理上两根线：
//   stm32Tx(UART1, GPIO6)  -> STM32 PB11 下发 NET:*；stm32Rx(UART2, GPIO44) <- STM32 PB10 收 BT:*。
// 面试可讲：
//   1. 收发各用一个独立硬件 UART（而不是一个串口全双工），这样接收永远不被发送阻塞，
//      收到 STM32 的 ACK/遥测能即时处理；
//   2. lastAckMs 记录"最近一次收到 STM32 ACK/PONG 的时间戳"，心跳模块据此判断 uart_ok；
//   3. sendUartKeepalive 周期性发 NET:UART? 探测 STM32 是否还活着（收不到就表示链路可能断了）。
// 双 HardwareSerial 实例与轮询由本模块独占。
// lastAckMs 被心跳模块读取，以 extern 暴露；去耦合阶段将改为访问接口。
extern HardwareSerial stm32Tx;
extern HardwareSerial stm32Rx;
extern unsigned long lastAckMs;

String stm32LogLine(const String &line);
void sendToStm32(const String &line, uint16_t postDelayMs = 80);
void sendUartKeepalive();
void pollStm32();
