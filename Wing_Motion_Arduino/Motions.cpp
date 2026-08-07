#include "Motions.h"
#include <math.h>

// Define the global shared variables
Servo servos[8];
SystemState currentState = MODE_1_BREATHING;
SystemState previousState = MODE_1_BREATHING; // fallback
float currentPos[8] = {90.0, 90.0, 90.0, 90.0, 90.0, 90.0, 90.0, 90.0};
float targetPos[8]  = {90.0, 90.0, 90.0, 90.0, 90.0, 90.0, 90.0, 90.0};
unsigned long motionStartTime = 0;

float waveStartAngle = 90.0;
float waveEndAngle = 180.0;
float currentOnetimeEndAngle = 90.0; // Initial reset state is 90

// Initialize the motion configurations with their default values
MotionParams motionConfigs[10] = {
  {0.0, 0.0, 0.0, 0.0},                              // Index 0 (unused)
  {1500.0, 40.0, 70.0, 0.8},                         // MODE_1_BREATHING
  {1500.0, 60.0, 60.0, 0.0},                         // MODE_2_SWEEP
  {2400.0, 80.0, 40.0, 400.0},                       // MODE_3_MIRROR (speed=2400ms period, amplitude=80deg range, centerOffset=40deg closed, phaseOffset=400ms delay)
  {450.0, 90.0, 45.0, 1.5},                          // MODE_4_RIPPLE
  {1000.0, 6.0, 90.0, 1.5},                          // MODE_5_SHIVER (speed=freq multiplier, amplitude=wiggle degrees, centerOffset=base pos, phaseOffset=variance)
  {2400.0, 60.0, 60.0, 400.0},                       // MODE_6_ROLL (speed=2400ms period, phaseOffset=400ms delay)
  {800.0, 180.0, 0.0, 200.0},                        // MODE_7_STADIUM_WAVE (speed=800ms swing S, amplitude=180 max, centerOffset=0 min, phaseOffset=200ms delay d)
  {0.0, 0.0, 90.0, 0.0},                             // MODE_8_SLEEP (centerOffset=90.0)
  {800.0, 180.0, 0.0, 200.0}                         // MODE_9_ONETIME (speed=800ms S, amplitude=180 max, centerOffset=0 min, phaseOffset=200ms d)
};

void calculateTargets(unsigned long time) {
  MotionParams params = motionConfigs[currentState];

  switch (currentState) {
    
    case MODE_1_BREATHING: {
      for (int i = 0; i < 8; i++) {
        float phaseOffset = i * params.phaseOffset; 
        float breath = (sin((time / params.speed) - phaseOffset) + 1.0) / 2.0; 
        targetPos[i] = params.centerOffset + (breath * params.amplitude);
      }
      break;
    }
      
    case MODE_2_SWEEP: {
      float angle = (2.0 * M_PI * (float)time) / params.speed;
      float breath = (sin(angle) + 1.0) / 2.0;
      for (int i = 0; i < 8; i++) {
        targetPos[i] = params.centerOffset + (breath * params.amplitude);
      }
      break;
    }
      
    case MODE_3_MIRROR: {
      float T = params.speed;
      if (T <= 0.01) T = 1000.0;
      float d = params.phaseOffset;
      float maxVal = params.centerOffset + params.amplitude;
      float minVal = params.centerOffset;

      float angle = (2.0 * M_PI * (float)time) / T;
      float phaseAngle = (d / T) * 2.0 * M_PI;

      for (int i = 0; i < 8; i++) {
        // Calculate symmetric distance from the outer edge (0 and 7 are outermost)
        float distFromEdge = (i < 4) ? i : (7 - i);
        float breath = (cos(angle - distFromEdge * phaseAngle) + 1.0) / 2.0;

        // Apply mirroring: left side (0, 1, 2, 3) and right side (4, 5, 6, 7) move in opposite directions
        if (i < 4) {
          targetPos[i] = minVal + (breath * (maxVal - minVal));
        } else {
          targetPos[i] = maxVal - (breath * (maxVal - minVal));
        }
      }
      break;
    }
      
    case MODE_4_RIPPLE: {
      for (int i = 0; i < 8; i++) {
        // Ripple outward from center (between 3 and 4)
        float distFromCenter = fabs(3.5 - (float)i); 
        float phaseOffset = distFromCenter * params.phaseOffset; 
        float wave = (sin((time / params.speed) + phaseOffset) + 1.0) / 2.0;
        targetPos[i] = params.centerOffset + (wave * params.amplitude);
      }
      break;
    }
      
    case MODE_5_SHIVER: {
      float speedMult = params.speed / 1000.0;
      if (speedMult <= 0.01) speedMult = 1.0;
      
      for (int i = 0; i < 8; i++) {
        // High frequency waves shifted by phaseOffset for each wing
        float w1 = (time * speedMult * 0.05) + (i * params.phaseOffset * 7.3);
        float w2 = (time * speedMult * 0.12) + (i * params.phaseOffset * 13.7);
        float w3 = (time * speedMult * 0.23) + (i * params.phaseOffset * 19.1);
        
        float jitter = (sin(w1) * 0.5) + (cos(w2) * 0.3) + (sin(w3) * 0.2);
        
        targetPos[i] = params.centerOffset + (jitter * params.amplitude);
      }
      break;
    }

    case MODE_6_ROLL: {
      float period = params.speed;
      float phaseDelay = params.phaseOffset;
      
      for (int i = 0; i < 8; i++) {
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

      // Period scales to 14 delays for 8 servos (0 to 7)
      float T = 14.0 * d + 2.0 * S + 100.0;
      if (T <= 0) T = 1000.0;

      float t = fmod((float)time, T);
      if (t < 0) t += T;

      for (int i = 0; i < 8; i++) {
        float t_up_start = i * d;
        float t_up_end = t_up_start + S;
        float t_down_start = (14.0 - (float)i) * d + S;
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
      for (int i = 0; i < 8; i++) {
        targetPos[i] = params.centerOffset;
      }
      break;
    }

    case MODE_9_ONETIME: {
      float S = params.speed;
      float d = params.phaseOffset;
      unsigned long elapsed = time - motionStartTime;
      float totalDuration = S + (7.0 * d);
      
      if (elapsed <= totalDuration) {
        for (int i = 0; i < 8; i++) {
          float startTime = i * d;
          if (elapsed < startTime) {
            targetPos[i] = 90.0;
          } else if (elapsed < startTime + S) {
            float progress = (float)(elapsed - startTime) / S;
            targetPos[i] = 90.0 + progress * (waveEndAngle - 90.0);
          } else {
            targetPos[i] = waveEndAngle;
          }
        }
      } else {
        for (int i = 0; i < 8; i++) targetPos[i] = waveEndAngle;
        currentState = previousState;
        motionStartTime = 0;
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
  for (int i = 0; i < 8; i++) {
    currentPos[i] = (currentPos[i] * (1.0 - currentEasing)) + (targetPos[i] * currentEasing);
    int microSec = 500 + (currentPos[i] * (2000.0 / 180.0));
    servos[i].writeMicroseconds(microSec);
  }
}

void triggerOnetimeWave() {
  if (currentOnetimeEndAngle == 90.0) {
    // First run: from 90 to 180
    waveStartAngle = 90.0;
    waveEndAngle = 180.0;
    currentOnetimeEndAngle = 180.0;
  }
  else if (currentOnetimeEndAngle == 180.0) {
    // Second run: from 180 to 0
    waveStartAngle = 180.0;
    waveEndAngle = 0.0;
    currentOnetimeEndAngle = 0.0;
  }
  else if (currentOnetimeEndAngle == 0.0) {
    // Third run: from 0 to 180
    waveStartAngle = 0.0;
    waveEndAngle = 180.0;
    currentOnetimeEndAngle = 180.0;
  }
  motionStartTime = millis();
}

