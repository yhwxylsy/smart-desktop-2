// 智能桌面终端 STM32 执行器固件 —— 配置常量（纯搬运自原 sketch L8-43、L45-82、L120-130）
//
// 行为保持硬约束：协议字符串、所有时序默认值一律为原值，不得修改。
// 例外（2026-09-06 已评审通过的 USART3 全双工改造）：串口引脚分配与两端波特率
//   见下方"串口链路波特率"一节，回退方式也写在那里。
//   除此之外所有 PIN_* 引脚号仍是原值，不得修改。
// STM32duino 运行时（PBx 引脚宏、Stream、SoftwareSerial、Wire 等）由本文件优先包含。
//
// 编译：arduino-cli compile -b STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8

#pragma once
#include <Arduino.h>

// ---- 串口链路波特率 ----
// 2026-09-06 接线调整（详见 docs/HARDWARE_WIRING.md）：
//   USART3（PB11 RX / PB10 TX）与 ESP32S3 组成全双工链路，两端都是硬件 UART，因此上到 115200；
//   SYN6288 改由 PB3 软件串口驱动，保持 9600（SYN6288 出厂默认，且 bit-bang 在低速下更稳）。
// 背景：此前 PB10 硬件 TX 给了 SYN6288，而 600B 量级的 BT:* 上行却走了 PB3 软件串口，
//   每 4 秒约 630 ms 的 bit-bang 会长时间占用 CPU，拖累舵机脉冲、旋律节拍与编码器采样。
// 回退方式：若真机出现乱码或首字节脏前缀，两端同步把波特率改回 9600
//   （STM32 改本常量；ESP32 改 edge/esp32s3/main/config.h 的 STM32_UART_BAUD）。
//   只改一端会被 tools/verify_firmware_consistency.py 的 E 段判为不一致。
static const uint32_t ESP_UART_BAUD = 115200;  // USART3 全双工 <-> ESP32S3
static const uint32_t SYN6288_BAUD = 9600;     // PB3 software TX -> SYN6288 RXD
// 600B 遥测在 115200 下线上时间约 52 ms（改造前 9600 软件串口约 630 ms），已无需特殊处理。
// 若仍想让写入完全不占 loop()，用 -DSERIAL_TX_BUFFER_SIZE=1024 增大硬件串口 TX 缓冲
// （STM32duino core 默认 64 字节；当前 core 版本的 HardwareSerial 没有 setTxBufferSize()）。

// ---- 板级引脚（PIN_* 原样搬运）----
static const int PIN_BUZZER = PB9;
static const int PIN_DRV8833_IN1 = PA0;  // TIM2_CH1 in the Botelvdong DRV8833 example.
static const int PIN_DRV8833_IN2 = PA1;  // TIM2_CH2 in the Botelvdong DRV8833 example.
static const int PIN_RGB_BLUE = PA6;
static const int PIN_RGB_GREEN = PA7;
static const int PIN_RGB_RED = PB0;
static const int PIN_SERVO = PB8;
static const int PIN_NTC = PA4;
static const int PIN_POTENTIOMETER = PA5;
static const int PIN_ULTRASONIC_TRIG = PA11;
static const int PIN_ULTRASONIC_ECHO = PA10;
static const int PIN_ENCODER_A = PA8;
static const int PIN_ENCODER_B = PA9;
static const int PIN_ENCODER_BUTTON = PB15;
static const int PIN_TRACKING_SENSOR = PB14;
#ifndef DEMO_BUTTON_PIN
static const int PIN_DEMO_BUTTON = PB13;  // KEY2 on the Botelvdong STM32 learning kit.
#else
static const int PIN_DEMO_BUTTON = DEMO_BUTTON_PIN;
#endif
#ifndef INFO_BUTTON_PIN
static const int PIN_INFO_BUTTON = PB12;  // KEY1: OLED information screen switch.
#else
static const int PIN_INFO_BUTTON = INFO_BUTTON_PIN;
#endif

// ---- 默认开关（行为保持，仅当未外部覆盖时生效）----
#ifndef ULTRASONIC_ENABLED_BY_DEFAULT
#define ULTRASONIC_ENABLED_BY_DEFAULT 1
#endif

#ifndef DRV8833_CONNECTED_BY_DEFAULT
#define DRV8833_CONNECTED_BY_DEFAULT 1
#endif

// ---- 时序与硬件常量（原样搬运）----
static const uint32_t TELEMETRY_INTERVAL_MS = 4000;
static const uint32_t UI_FRAME_INTERVAL_MS = 80;
static const uint32_t RGB_FRAME_INTERVAL_MS = 80;
static const uint32_t OLED_FLUSH_INTERVAL_MS = 4;
static const uint32_t SERVO_PULSE_PERIOD_US = 20000;
static const uint32_t SERVO_HOLD_MS = 800;
static const uint16_t SERVO_MIN_PULSE_US = 500;
static const uint16_t SERVO_MAX_PULSE_US = 2500;
static const uint8_t AHT20_I2C_ADDRESS = 0x38;
static const uint8_t AHT20_CMD_INIT = 0xBE;
static const uint8_t AHT20_CMD_MEASURE = 0xAC;
static const uint32_t ULTRASONIC_TIMEOUT_US = 25000;
static const uint16_t ANALOG_RAW_MAX = 1023;
static const uint8_t DRV8833_FAN_LEVEL1_DUTY = 217;  // ~85%; lower duty may fail to start small fans.
static const uint8_t DRV8833_FAN_LEVEL2_DUTY = 235;  // ~92%.
static const uint8_t DRV8833_FAN_LEVEL3_DUTY = 255;  // Full speed.
static const uint16_t MUSIC_NOTE_GAP_MS = 35;
static const uint32_t UI_BOOT_HOLD_MS = 1800;
static const uint32_t UI_TRANSIENT_HOLD_MS = 2200;
static const uint32_t UI_SPEAK_HOLD_MS = 7000;
static const uint32_t RGB_ACK_FLASH_MS = 520;
static const uint8_t OLED_WIDTH = 128;
static const uint8_t OLED_HEIGHT = 64;
static const uint8_t OLED_PAGES = OLED_HEIGHT / 8;
static const uint8_t OLED_ADDR_PRIMARY = 0x3D;   // 0x7A in 8-bit datasheets.
static const uint8_t OLED_ADDR_FALLBACK = 0x3C;
static const uint8_t OLED_CHUNK_BYTES = 16;
static const uint8_t UI_PANEL_X = 4;
static const uint8_t UI_TEXT_X = 6;
static const uint8_t UI_RIGHT_MARGIN = 10;
static const uint8_t UI_PANEL_WIDTH = OLED_WIDTH - UI_PANEL_X - UI_RIGHT_MARGIN;
static const uint8_t UI_MAX_CHARS = (OLED_WIDTH - UI_TEXT_X - UI_RIGHT_MARGIN) / 6;
static const uint16_t UI_DEMO_DEFAULT_STEP_MS = 950;
static const uint32_t BUTTON_DEBOUNCE_MS = 35;
static const uint32_t KEY2_HOLD_START_MS = 600;
static const uint32_t KEY1_LONG_PRESS_MS = 700;
static const uint32_t SCREEN_OVERLAY_MS = 1400;
static const uint32_t TELEMETRY_ESP_QUIET_MS = 3000;

// ---- SYN6288 语音合成常量（原样搬运）----
static const uint8_t SYN6288_FRAME_HEADER = 0xFD;
static const uint8_t SYN6288_CMD_SYNTHESIS = 0x01;
static const uint8_t SYN6288_CMD_STOP = 0x02;
static const uint8_t SYN6288_TYPE_UNICODE = 0x03;
static const size_t SYN6288_MAX_TEXT_BYTES = 200;
static const uint8_t SYN6288_MAX_VOLUME = 16;
static const uint8_t VOLUME_DEFAULT_PERCENT = 60;
static const uint8_t VOLUME_MAX_PERCENT = 100;
static const uint8_t VOLUME_STEP_PERCENT = 10;
static const int8_t ENCODER_STEPS_PER_DETENT = 4;
static const uint32_t VOLUME_ANNOUNCE_DELAY_MS = 450;

// ---- 独立看门狗（IWDG，LSI 时钟，与 CPU 主时钟无关）----
// 目的：现场长时间通电时，若 I2C 总线挂死或 loop() 卡在某处，设备能自行复位而不是永久僵住。
// 注意：IWDG 一旦启动无法用软件关停，只能靠复位清除。
// 现场应急：用 -DWATCHDOG_ENABLED_BY_DEFAULT=0 重新编译即可整体停用。
#ifndef WATCHDOG_ENABLED_BY_DEFAULT
#define WATCHDOG_ENABLED_BY_DEFAULT 1
#endif
// 取值需大于单次 loop() 最坏耗时（遥测 + OLED + TTS 帧），同时小于用户能容忍的卡死时长。
static const uint32_t WATCHDOG_TIMEOUT_MS = 5000;
