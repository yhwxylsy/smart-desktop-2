#include "encoder.h"
#include "../../config.h"
#include "../audio/tts.h"

long encoderPosition = 0;
long encoderDeltaSinceTelemetry = 0;

static uint8_t encoderLastState = 0;
static int8_t encoderVolumeTicks = 0;

uint8_t readEncoderState() {
  uint8_t state = 0;
  if (digitalRead(PIN_ENCODER_A) == HIGH) {
    state |= 0x01;
  }
  if (digitalRead(PIN_ENCODER_B) == HIGH) {
    state |= 0x02;
  }
  return state;
}

void initEncoder() {
  encoderLastState = readEncoderState();
}

// 轮询一次编码器。核心是 TRANSITIONS 状态转移表：
//   上一状态(2bit)<<2 | 当前状态(2bit) = 4bit 索引，查表得方向（+1/-1/0）。
// 只认"合法的单步格雷码跳变"，非法跳变（如同时跳两相）返回 0，天然消抖。
void updateEncoder() {
  static const int8_t TRANSITIONS[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0
  };
  uint8_t state = readEncoderState();
  uint8_t index = (encoderLastState << 2) | state;
  int8_t delta = TRANSITIONS[index];
  if (delta != 0) {
    encoderPosition += delta;
    encoderDeltaSinceTelemetry += delta;
    encoderVolumeTicks += delta;
    if (encoderVolumeTicks >= ENCODER_STEPS_PER_DETENT) {
      encoderVolumeTicks = 0;
      setSpeechVolume("UP", true);
    } else if (encoderVolumeTicks <= -ENCODER_STEPS_PER_DETENT) {
      encoderVolumeTicks = 0;
      setSpeechVolume("DOWN", true);
    }
  }
  encoderLastState = state;
}
