#include "aht20.h"
#include <Wire.h>
#include "../../config.h"

bool aht20Initialized = false;

bool initializeAht20() {
  Wire.beginTransmission(AHT20_I2C_ADDRESS);
  Wire.write(AHT20_CMD_INIT);
  Wire.write(0x08);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  delay(10);
  return true;
}

// 读取温湿度。注意"懒初始化"：首次调用若未初始化就先补一次 init。
// 每次 I2C 事务失败都把 aht20Initialized 置 false，让下次调用重新初始化——自愈。
bool readAht20(float &temperatureC, float &humidityPct) {
  if (!aht20Initialized) {
    aht20Initialized = initializeAht20();
  }
  if (!aht20Initialized) {
    return false;
  }

  // 触发一次测量（0xAC + 0x33 0x00），之后必须等约 80ms 让片上转换完成。
  Wire.beginTransmission(AHT20_I2C_ADDRESS);
  Wire.write(AHT20_CMD_MEASURE);
  Wire.write(0x33);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    aht20Initialized = false;
    return false;
  }

  delay(85);  // AHT20 转换等待（数据手册要求 ≥80ms）
  uint8_t received = Wire.requestFrom(AHT20_I2C_ADDRESS, (uint8_t)7);
  if (received != 7) {
    aht20Initialized = false;
    return false;
  }

  uint8_t buffer[7];
  for (uint8_t i = 0; i < 7; ++i) {
    buffer[i] = Wire.read();
  }
  // 状态字节 bit7 是 busy 标志：置 1 说明数据还没就绪，本次丢弃。
  if ((buffer[0] & 0x80) != 0) {
    return false;
  }

  // 从 7 字节里抠出两个 20bit 原始值（湿度在 buffer[1..3]，温度在 buffer[3..5]，buffer[3] 两者共享 4bit）。
  uint32_t rawHumidity = ((uint32_t)buffer[1] << 12) | ((uint32_t)buffer[2] << 4) | (buffer[3] >> 4);
  uint32_t rawTemperature = ((uint32_t)(buffer[3] & 0x0F) << 16) | ((uint32_t)buffer[4] << 8) | buffer[5];
  // 数据手册换算公式：湿度 = raw/2^20 * 100%，温度 = raw/2^20 * 200 - 50℃。
  humidityPct = (rawHumidity * 100.0f) / 1048576.0f;
  temperatureC = ((rawTemperature * 200.0f) / 1048576.0f) - 50.0f;
  return true;
}
