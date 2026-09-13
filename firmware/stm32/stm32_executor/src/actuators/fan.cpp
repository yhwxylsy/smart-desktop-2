#include "fan.h"
#include "../../config.h"

uint8_t currentFanLevel = 0;
bool drv8833Connected = DRV8833_CONNECTED_BY_DEFAULT != 0;

// 停转：两个输入都拉到 0，让 H 桥两侧同时截止，电机彻底断电。
// 面试可讲：这里同时做 analogWrite(0) + digitalWrite(LOW) 是为了
// 既清掉 PWM 占空比，又确保引脚电平被硬拉到低，避免 PWM 停止后残留高电平。
void stopDrv8833() {
  currentFanLevel = 0;
  if (!drv8833Connected) {
    return;
  }
  analogWrite(PIN_DRV8833_IN1, 0);
  analogWrite(PIN_DRV8833_IN2, 0);
  digitalWrite(PIN_DRV8833_IN1, LOW);
  digitalWrite(PIN_DRV8833_IN2, LOW);
}

uint8_t drv8833FanDutyForLevel(uint8_t level) {
  if (level <= 1) {
    return DRV8833_FAN_LEVEL1_DUTY;
  }
  if (level == 2) {
    return DRV8833_FAN_LEVEL2_DUTY;
  }
  return DRV8833_FAN_LEVEL3_DUTY;
}

uint8_t fanLevelFromCommand(const String &command) {
  int lastColon = command.lastIndexOf(':');
  if (lastColon < 0 || lastColon + 1 >= (int)command.length()) {
    return 2;
  }
  int level = command.substring(lastColon + 1).toInt();
  if (level < 1) {
    return 1;
  }
  if (level > 3) {
    return 3;
  }
  return (uint8_t)level;
}

// 开风扇：IN1 拉低、IN2 打 PWM（当前接线这个方向才是正转）。
// 档位 1~3 对应 217/235/255 占空比——最低档也设 ~85%，因为占空比太低小风扇起不来。
bool driveFanOn(uint8_t level) {
  if (!drv8833Connected) {
    return false;
  }
  currentFanLevel = level < 1 ? 1 : (level > 3 ? 3 : level);
  uint8_t duty = drv8833FanDutyForLevel(level);
  // Current fan wiring spins the useful direction with IN2 PWM and IN1 LOW.
  analogWrite(PIN_DRV8833_IN1, 0);
  digitalWrite(PIN_DRV8833_IN1, LOW);
  analogWrite(PIN_DRV8833_IN2, duty);
  return true;
}
