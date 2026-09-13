#include "ultrasonic.h"
#include "../../config.h"

bool ultrasonicEnabled = ULTRASONIC_ENABLED_BY_DEFAULT != 0;

// 测一次距离。流程：TRIG 拉低 2us -> 拉高 10us 触发 -> 等 ECHO 回波。
// pulseIn 测 ECHO 高电平脉宽（微秒），超时 ULTRASONIC_TIMEOUT_US(25ms) 返回 0 视为无回波。
// 面试可讲：/58.0f 是 HC-SR04 的换算系数（脉宽 58us ≈ 1cm，已含声速和往返/2）。
bool readDistanceCm(float &distanceCm) {
  if (!ultrasonicEnabled) {
    return false;
  }

  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);          // 10us 触发脉冲
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);

  uint32_t durationUs = pulseIn(PIN_ULTRASONIC_ECHO, HIGH, ULTRASONIC_TIMEOUT_US);
  if (durationUs == 0) {
    return false;                 // 超时无回波
  }

  distanceCm = durationUs / 58.0f;
  return true;
}
