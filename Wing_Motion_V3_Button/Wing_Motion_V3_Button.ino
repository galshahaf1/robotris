#include <Servo.h>

Servo servos[4];
const int servoPins[4] = {9, 10, 11, 12}; 

const int buttonPin = 2;
bool lastButtonState = HIGH; 
unsigned long buttonPressTime = 0;
bool isPressing = false;

// המספור הותאם ל-1 עד 5
enum SystemState {
  MODE_1_BREATHING = 1,
  MODE_2_COCOON = 2,
  MODE_3_CONFLICT = 3,
  MODE_4_RIPPLE = 4,
  MODE_5_SLEEP = 5
};

SystemState currentState = MODE_1_BREATHING;

float currentPos[4] = {90.0, 90.0, 90.0, 90.0}; 
float targetPos[4]  = {90.0, 90.0, 90.0, 90.0}; 

const float easingFactor = 0.08; 

void setup() {
  Serial.begin(9600);
  
  for (int i = 0; i < 4; i++) {
    servos[i].attach(servoPins[i]);
  }
  
  pinMode(buttonPin, INPUT_PULLUP); 
  Serial.println("System Started. Mode: 1 (Breathing)");
}

void loop() {
  unsigned long currentMillis = millis();

  // --- 1. ניהול מצבים (1 עד 4) ---
  bool currentButtonState = digitalRead(buttonPin);
  
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    buttonPressTime = currentMillis;
    isPressing = true;
  }
  
  if (lastButtonState == LOW && currentButtonState == HIGH && isPressing) {
    unsigned long pressDuration = currentMillis - buttonPressTime;
    isPressing = false;
    
    if (pressDuration >= 600) { 
      currentState = MODE_5_SLEEP;
      Serial.println("Mode: 5 (SLEEP_MODE)");
    } else if (pressDuration > 50) { 
      if (currentState == MODE_5_SLEEP) {
        currentState = MODE_1_BREATHING; 
      } else {
        int nextState = (int)currentState + 1;
        if (nextState > 4) nextState = 1; 
        currentState = (SystemState)nextState;
      }
      Serial.print("Mode changed to: ");
      Serial.println(currentState);
    }
  }
  lastButtonState = currentButtonState;

  // --- 2. חישוב מיקומי המטרה לפי המצב הנוכחי ---
  calculateTargets(currentMillis);

  // --- 3. הנעת המנועים ---
  moveServosSmoothly();
  
  delay(10); 
}

// ---------------------------------------------------------
// פונקציות עזר
// ---------------------------------------------------------

void calculateTargets(unsigned long time) {
  switch (currentState) {
    
    case MODE_1_BREATHING: {
      // נשימה רגועה במרכז
      for (int i = 0; i < 4; i++) {
        float phaseOffset = i * 0.8; 
        float breath = (sin((time / 1500.0) - phaseOffset) + 1.0) / 2.0; 
        targetPos[i] = 70.0 + (breath * 40.0); // טווח 70-110
      }
      break;
    }
      
    case MODE_2_COCOON: {
      // מסוגר, פונה הצידה, כמעט ללא דיליי (פועל כיחידה אחת מוגנת)
      for (int i = 0; i < 4; i++) {
        float phaseOffset = i * 0.3; // דיליי קטן מאוד
        float microBreath = (sin((time / 2000.0) - phaseOffset) + 1.0) / 2.0;
        targetPos[i] = 140.0 + (microBreath * 20.0); // טווח 140-160
      }
      break;
    }
      
    case MODE_3_CONFLICT: {
      // פתוח לרווחה להכנסת אוויר, תנועת גל ארוכה מאוד וא-סימטרית
      for (int i = 0; i < 4; i++) {
        float phaseOffset = i * 2.0; // דיליי ענק - יוצר תנועת גריפה
        float slowOpen = (sin((time / 3000.0) - phaseOffset) + 1.0) / 2.0;
        targetPos[i] = 20.0 + (slowOpen * 60.0); // טווח 20-80
      }
      break;
    }
      
    case MODE_4_RIPPLE: {
      // מרוכך ונעים יותר: רפרוף של משב רוח במקום טריקה
      for (int i = 0; i < 4; i++) {
        float distFromCenter = abs(1.5 - i); 
        float phaseOffset = distFromCenter * 1.5; 
        float wave = (sin((time / 450.0) + phaseOffset) + 1.0) / 2.0;
        targetPos[i] = 45.0 + (wave * 90.0); // טווח מצומצם ועדין: 45-135
      }
      break;
    }

    case MODE_5_SLEEP: {
      for (int i = 0; i < 4; i++) {
        targetPos[i] = 90.0;
      }
      break;
    }
  }
}

void moveServosSmoothly() {
  for (int i = 0; i < 4; i++) {
    currentPos[i] = (currentPos[i] * (1.0 - easingFactor)) + (targetPos[i] * easingFactor);
    int microSec = 500 + (currentPos[i] * (2000.0 / 180.0));
    servos[i].writeMicroseconds(microSec);
  }
}