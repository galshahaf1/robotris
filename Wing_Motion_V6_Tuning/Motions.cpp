#include "Motions.h"

// Define the global shared variables
Servo servos[4];
SystemState currentState = MODE_1_BREATHING;
float currentPos[4] = {90.0, 90.0, 90.0, 90.0};
float targetPos[4]  = {90.0, 90.0, 90.0, 90.0};

// Initialize the motion configurations with their default values
MotionParams motionConfigs[9] = {
  {0.0, 0.0, 0.0, 0.0},                              // Index 0 (unused)
  {1500.0, 40.0, 70.0, 0.8},                         // MODE_1_BREATHING
  {2000.0, 20.0, 140.0, 0.3},                        // MODE_2_COCOON
  {3000.0, 60.0, 20.0, 2.0},                         // MODE_3_CONFLICT
  {450.0, 90.0, 45.0, 1.5},                          // MODE_4_RIPPLE
  {1000.0, 9.0, 90.0, 1.0},                          // MODE_5_SHIVER (amplitude=9.0 degrees peak, speed=1000.0 scale)
  {2400.0, 60.0, 60.0, 400.0},                       // MODE_6_ROLL (speed=2400ms period, phaseOffset=400ms delay)
  {800.0, 180.0, 0.0, 200.0},                        // MODE_7_STADIUM_WAVE (speed=800ms swing S, amplitude=180 max, centerOffset=0 min, phaseOffset=200ms delay d)
  {0.0, 0.0, 90.0, 0.0}                              // MODE_8_SLEEP (centerOffset=90.0)
};

void calculateTargets(unsigned long time) {
  MotionParams params = motionConfigs[currentState];

  switch (currentState) {
    
    case MODE_1_BREATHING: {
      for (int i = 0; i < 4; i++) {
        float phaseOffset = i * params.phaseOffset; 
        float breath = (sin((time / params.speed) - phaseOffset) + 1.0) / 2.0; 
        targetPos[i] = params.centerOffset + (breath * params.amplitude);
      }
      break;
    }
      
    case MODE_2_COCOON: {
      for (int i = 0; i < 4; i++) {
        float phaseOffset = i * params.phaseOffset;
        float microBreath = (sin((time / params.speed) - phaseOffset) + 1.0) / 2.0;
        targetPos[i] = params.centerOffset + (microBreath * params.amplitude);
      }
      break;
    }
      
    case MODE_3_CONFLICT: {
      for (int i = 0; i < 4; i++) {
        float phaseOffset = i * params.phaseOffset;
        float slowOpen = (sin((time / params.speed) - phaseOffset) + 1.0) / 2.0;
        targetPos[i] = params.centerOffset + (slowOpen * params.amplitude);
      }
      break;
    }
      
    case MODE_4_RIPPLE: {
      for (int i = 0; i < 4; i++) {
        float distFromCenter = abs(1.5 - (float)i); 
        float phaseOffset = distFromCenter * params.phaseOffset; 
        float wave = (sin((time / params.speed) + phaseOffset) + 1.0) / 2.0;
        targetPos[i] = params.centerOffset + (wave * params.amplitude);
      }
      break;
    }
      
    case MODE_5_SHIVER: {
      float speedFactor = params.speed / 1000.0;
      if (speedFactor <= 0.01) speedFactor = 1.0;
      
      for (int i = 0; i < 4; i++) {
        // Slow, independent posture drift
        float slowDrift = sin(time / ((3000.0 - i * 400.0) * speedFactor)) * 6.0 * params.phaseOffset;
        
        // Unique high-frequency tremor wave combinations for each wing
        float t1 = time / ((12.0 + i * 3.5) * speedFactor);
        float t2 = time / ((7.0 + i * 1.8) * speedFactor);
        float jitter = (sin(t1) * 6.0) + (cos(t2) * 3.0);
        
        // Decentralized envelope
        float envSin = sin(time / ((800.0 + i * 250.0) * speedFactor));
        float envCos = cos(time / ((1800.0 + i * 350.0) * speedFactor));
        float envelope = sq((envSin * envCos + 1.0) / 2.0);
        
        // Scale the shivering effect using params.amplitude
        targetPos[i] = params.centerOffset + slowDrift + (jitter * envelope * (params.amplitude / 9.0));
      }
      break;
    }

    case MODE_6_ROLL: {
      float period = params.speed;
      float phaseDelay = params.phaseOffset;
      
      for (int i = 0; i < 4; i++) {
        float tShifted = (float)time - (i * phaseDelay);
        float tNormalized = fmod(tShifted, period);
        if (tNormalized < 0) tNormalized += period;
        tNormalized /= period;
        
        float triVal = 0.0;
        if (tNormalized < 0.5) {
          triVal = tNormalized * 2.0;
        } else {
          triVal = 2.0 - (tNormalized * 2.0);
        }
        
        targetPos[i] = params.centerOffset + (triVal * params.amplitude);
      }
      break;
    }

    case MODE_7_STADIUM_WAVE: {
      float S = params.speed;
      float d = params.phaseOffset;
      float maxVal = params.amplitude;
      float minVal = params.centerOffset;

      float T = 6.0 * d + 2.0 * S + 100.0;
      if (T <= 0) T = 1000.0;

      float t = fmod((double)time, T);
      if (t < 0) t += T;

      for (int i = 0; i < 4; i++) {
        float t_up_start = i * d;
        float t_up_end = t_up_start + S;
        float t_down_start = (6.0 - (float)i) * d + S;
        float t_down_end = t_down_start + S;

        if (t >= t_up_start && t < t_up_end) {
          float progress = (t - t_up_start) / S;
          targetPos[i] = minVal + progress * (maxVal - minVal);
        }
        else if (t >= t_up_end && t < t_down_start) {
          targetPos[i] = maxVal;
        }
        else if (t >= t_down_start && t < t_down_end) {
          float progress = (t - t_down_start) / S;
          targetPos[i] = maxVal - progress * (maxVal - minVal);
        }
        else {
          targetPos[i] = minVal;
        }
      }
      break;
    }

    case MODE_8_SLEEP: {
      for (int i = 0; i < 4; i++) {
        targetPos[i] = params.centerOffset;
      }
      break;
    }
  }
}

void moveServosSmoothly() {
  float currentEasing = easingFactorDefault;
  if (currentState == MODE_5_SHIVER) {
    currentEasing = easingFactorShiver;
  }
  for (int i = 0; i < 4; i++) {
    currentPos[i] = (currentPos[i] * (1.0 - currentEasing)) + (targetPos[i] * currentEasing);
    int microSec = 500 + (currentPos[i] * (2000.0 / 180.0));
    servos[i].writeMicroseconds(microSec);
  }
}
