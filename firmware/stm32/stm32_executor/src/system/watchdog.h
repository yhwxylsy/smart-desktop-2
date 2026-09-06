#pragma once
#include <Arduino.h>

// 独立看门狗（IWDG）封装。
//
// IWDG 由片内 LSI 低速时钟驱动，与 CPU 主时钟无关，因此主时钟跑飞或程序跑死在
// 某处死循环时仍能触发复位。它一旦启动就无法用软件关停，只能靠复位清除；
// 现场应急请用 -DWATCHDOG_ENABLED_BY_DEFAULT=0 重新编译整体停用。
//
// 约定：
//   setupWatchdog() —— 在 setup() 末尾、所有外设初始化完成之后调用一次。
//   feedWatchdog()  —— 在 loop() 末尾调用一次；两次调用间隔超过
//                      WATCHDOG_TIMEOUT_MS 就会复位。
void setupWatchdog();
void feedWatchdog();
