#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "../core/types.h"

// 麦克风采集（原 main.ino L146-152、L869-883、L892-935、L1360-1383 原样搬运）。
//
// 职责：用 ESP32S3 Sense 板载 PDM 麦克风（I2S 接口）录一段 16kHz 单声道 PCM，
//       包成 WAV 格式供上传做 ASR。默认由 MIC_PATH_ENABLED 开关控制，关掉则走
//       笔记本麦克风链路（答辩主链路）。
// 面试可讲：PDM 麦克风输出 1-bit 脉冲密度调制信号，I2S 外设以 PDM_MONO_MODE 采集成
// 16bit PCM；generateWavHeader 手拼 44 字节 WAV 头；conditionPcm16 做去直流 + 数字增益。
// micReady / micBusy 由采集与流水线共享，以 extern 暴露；去耦合阶段改为访问接口。
extern bool micReady;
extern bool micBusy;

bool initMicrophone();
void generateWavHeader(uint8_t *wavHeader, uint32_t wavSize, uint32_t sampleRate);
void conditionPcm16(uint8_t *pcmBuffer, size_t pcmSize);
MicCapture captureMicWav();
uint8_t *allocLargeBuffer(size_t size);
