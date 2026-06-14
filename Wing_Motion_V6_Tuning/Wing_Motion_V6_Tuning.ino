#include "Config.h"
#include "Motions.h"
#include "SerialHandler.h"

bool lastButtonState = HIGH; 
unsigned long buttonPressTime = 0;
bool isPressing = false;

void setup() {
  Serial.begin(9600);
  
  // Attach servos
  for (int i = 0; i < 4; i++) {
    servos[i].attach(servoPins[i]);
  }
  
  // Setup button
  pinMode(buttonPin, INPUT_PULLUP); 
  
  Serial.println("System Started. Mode: 1 (Breathing)");
}

void loop() {
  unsigned long currentMillis = millis();

  // --- 1. Serial Commands Handling ---
  handleSerialCommands();

  // --- 2. Button State Management & Mode Switching ---
  bool currentButtonState = digitalRead(buttonPin);
  
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    buttonPressTime = currentMillis;
    isPressing = true;
  }
  
  if (lastButtonState == LOW && currentButtonState == HIGH && isPressing) {
    unsigned long pressDuration = currentMillis - buttonPressTime;
    isPressing = false;
    
    if (pressDuration >= 600) { 
      currentState = MODE_7_SLEEP;
      Serial.println("Mode: 7 (SLEEP_MODE)");
    } else if (pressDuration > 50) { 
      if (currentState == MODE_7_SLEEP) {
        currentState = MODE_1_BREATHING; 
      } else {
        int nextState = (int)currentState + 1;
        if (nextState > 6) nextState = 1; 
        currentState = (SystemState)nextState;
      }
      Serial.print("Mode changed to: ");
      Serial.println(currentState);
    }
  }
  lastButtonState = currentButtonState;

  // --- 3. Compute target values for current mode ---
  calculateTargets(currentMillis);

  // --- 4. Move servos using easing ---
  moveServosSmoothly();
  
  delay(10); 
}
