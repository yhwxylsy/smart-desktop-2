#pragma once
#include <Arduino.h>
#include "../../config.h"

// 蜂鸣器旋律播放（原 sketch L91-94、L300-321、L1985-2076 原样搬运）。
//
// 职责：无源蜂鸣器（PB9）的提示音和预置旋律（SUCCESS/ALERT/SCALE/STARTUP/BIRTHDAY）。
// 面试可讲：无源蜂鸣器本身不发声，要靠 MCU 给它一定频率的方波（tone()）。
// 旋律是"音符表 + 非阻塞播放器"：每个音符含频率和时长，updateMusicPlayer 用
// millis() 到点切换下一个音，不用 delay 阻塞。频率 0 表示休止符。
struct MelodyNote {
  uint16_t frequency;   // 0 = 休止
  uint16_t durationMs;
};

void stopMusic();
bool startMusic(const MelodyNote *melody, uint8_t length);
void updateMusicPlayer();
bool startMusicByPreset(String preset);
bool playBeep(uint16_t frequency, uint16_t durationMs);
