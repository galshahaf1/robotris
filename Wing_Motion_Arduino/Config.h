#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Servo.h>

// --- Hardware Pins ---
const int servoPins[8] = {5, 6, 7, 8, 9, 10, 11, 12};
const int buttonPin = 2;

// --- Easing Configuration ---
const float easingFactorDefault = 0.08;
const float easingFactorShiver = 0.25;

// --- System State Enum ---
enum SystemState {
  MODE_1_BREATHING = 1,
  MODE_2_SWEEP = 2,
  MODE_3_MIRROR = 3,
  MODE_4_RIPPLE = 4,
  MODE_5_SHIVER = 5,
  MODE_6_ROLL = 6,
  MODE_7_STADIUM_WAVE = 7,
  MODE_8_SLEEP = 8,
  MODE_9_ONETIME = 9
};

// --- Motion Parameters Structure ---
struct MotionParams {
  float speed;         // Speed factor (divisor of time)
  float amplitude;     // Movement amplitude (multiplier)
  float centerOffset;  // Center angle offset (baseline)
  float phaseOffset;   // Phase offset between adjacent wings
};

// --- Global Shared Variables (Declared as extern) ---
extern Servo servos[8];
extern SystemState currentState;
extern SystemState previousState; // Store the mode prior to one‑time wave
extern float currentPos[8];
extern float targetPos[8];
extern MotionParams motionConfigs[10]; // Indices 1-9 correspond to SystemState values
extern unsigned long motionStartTime;

// Stateful One-Time Wave angles
extern float waveStartAngle;
extern float waveEndAngle;
extern float currentOnetimeEndAngle;

#endif // CONFIG_H

