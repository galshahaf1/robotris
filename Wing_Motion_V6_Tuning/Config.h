#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Servo.h>

// --- Hardware Pins ---
const int servoPins[4] = {9, 10, 11, 12};
const int buttonPin = 2;

// --- Easing Configuration ---
const float easingFactorDefault = 0.08;
const float easingFactorShiver = 0.25;

// --- System State Enum ---
enum SystemState {
  MODE_1_BREATHING = 1,
  MODE_2_COCOON = 2,
  MODE_3_CONFLICT = 3,
  MODE_4_RIPPLE = 4,
  MODE_5_SHIVER = 5,
  MODE_6_ROLL = 6,
  MODE_7_STADIUM_WAVE = 7,
  MODE_8_SLEEP = 8
};

// --- Motion Parameters Structure ---
struct MotionParams {
  float speed;         // Speed factor (divisor of time)
  float amplitude;     // Movement amplitude (multiplier)
  float centerOffset;  // Center angle offset (baseline)
  float phaseOffset;   // Phase offset between adjacent wings
};

// --- Global Shared Variables (Declared as extern) ---
extern Servo servos[4];
extern SystemState currentState;
extern float currentPos[4];
extern float targetPos[4];
extern MotionParams motionConfigs[9]; // Indices 1-8 correspond to SystemState values

#endif // CONFIG_H
