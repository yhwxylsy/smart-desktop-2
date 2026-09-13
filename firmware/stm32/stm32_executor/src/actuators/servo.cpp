#include "servo.h"
#include "../../config.h"

static bool servoActive = false;
static bool servoPulseHigh = false;
static uint16_t servoPulseWidthUs = 1500;
static uint32_t servoPulseStartedUs = 0;
static uint32_t nextServoPulseUs = 0;
static uint32_t lastServoCommandMs = 0;

bool parseServoAngle(const String &value, uint8_t &angle) {
  String text = value;
  text.trim();
  if (text.length() == 0) {
    return false;
  }
  for (uint16_t i = 0; i < text.length(); ++i) {
    if (!isDigit(text.charAt(i))) {
      return false;
    }
  }
  long parsed = text.toInt();
  if (parsed < 0 || parsed > 180) {
    return false;
  }
  angle = (uint8_t)parsed;
  return true;
}

void setServoAngle(uint8_t angle) {
  servoPulseWidthUs = SERVO_MIN_PULSE_US +
                      ((uint32_t)(SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * angle) / 180;
  lastServoCommandMs = millis();
  nextServoPulseUs = micros();
  servoPulseHigh = false;
  servoActive = true;
  digitalWrite(PIN_SERVO, LOW);
}

// 软件 PWM 状态机（loop() 每轮调用）。逻辑：
//   高电平阶段：持续 servoPulseWidthUs 后拉低；
//   低电平阶段：等到下一个 20ms 周期起点再拉高。
// 全程用 micros() 计算，非阻塞；hold 超时后自动停止并释放舵机。
// 面试可讲：这就是"不占硬件定时器、用时间戳做 PWM"的做法，缺点是精度受 loop() 其它耗时影响——
// 这正是上一轮把串口改成硬件 UART、避免 bit-bang 长时间阻塞的动机之一。
void updateServoPulse() {
  if (!servoActive) {
    return;
  }

  uint32_t nowUs = micros();
  if (servoPulseHigh) {
    if ((uint32_t)(nowUs - servoPulseStartedUs) >= servoPulseWidthUs) {
      digitalWrite(PIN_SERVO, LOW);
      servoPulseHigh = false;
      nextServoPulseUs = servoPulseStartedUs + SERVO_PULSE_PERIOD_US;
    }
  } else if ((int32_t)(nowUs - nextServoPulseUs) >= 0) {
    digitalWrite(PIN_SERVO, HIGH);
    servoPulseStartedUs = nowUs;
    servoPulseHigh = true;
  }

  if (!servoPulseHigh && millis() - lastServoCommandMs > SERVO_HOLD_MS) {
    servoActive = false;
    digitalWrite(PIN_SERVO, LOW);
  }
}
