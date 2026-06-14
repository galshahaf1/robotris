#include "Config.h"
#include "Motions.h"
#include "SerialHandler.h"
#include <EEPROM.h>

bool lastButtonState = HIGH; 
unsigned long buttonPressTime = 0;
bool isPressing = false;

const byte EEPROM_MAGIC = 0xA5;

void setup() {
  Serial.begin(9600);
  
  // Attach servos
  for (int i = 0; i < 4; i++) {
    servos[i].attach(servoPins[i]);
  }
  
  // Setup button
  pinMode(buttonPin, INPUT_PULLUP); 
  
  // Read/Initialize configurations from EEPROM
  if (EEPROM.read(0) == EEPROM_MAGIC) {
    for (int i = 1; i <= 8; i++) {
      int addr = 1 + (i - 1) * sizeof(MotionParams);
      EEPROM.get(addr, motionConfigs[i]);
    }
    Serial.println("Loaded configurations from EEPROM.");
  } else {
    // Write defaults to EEPROM
    for (int i = 1; i <= 8; i++) {
      int addr = 1 + (i - 1) * sizeof(MotionParams);
      EEPROM.put(addr, motionConfigs[i]);
    }
    EEPROM.write(0, EEPROM_MAGIC);
    Serial.println("Initialized default configurations in EEPROM.");
  }
  
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
      currentState = MODE_8_SLEEP;
      Serial.println("Mode: 8 (SLEEP_MODE)");
    } else if (pressDuration > 50) { 
      if (currentState == MODE_8_SLEEP) {
        currentState = MODE_1_BREATHING; 
      } else {
        int nextState = (int)currentState + 1;
        if (nextState > 7) nextState = 1; 
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
