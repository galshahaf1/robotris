#include <Servo.h>

// Create 8 servo objects
Servo servos[8];

// Define the 8 pins we are using
const int servoPins[8] = {5, 6, 7, 8, 9, 10, 11, 12};
const int buttonPin = 2; // Button pin (INPUT_PULLUP)

bool isRunning = false;
unsigned long lastMoveTime = 0;
int sweepState = 0; 
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

void toggleState() {
  isRunning = !isRunning;
  
  if (isRunning) {
    Serial.println("\n>>> Action: START");
    Serial.println("Attaching servos and starting sweep...");
    for (int i = 0; i < 8; i++) {
      servos[i].attach(servoPins[i]);
      servos[i].write(20);
    }
    lastMoveTime = millis();
    sweepState = 0;
  } else {
    Serial.println("\n>>> Action: STOP");
    Serial.println("Detaching servos (relaxing)...");
    for (int i = 0; i < 8; i++) {
      servos[i].detach();
    }
    Serial.println("Status: IDLE.");
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT_PULLUP);
  
  Serial.println("\n======================================");
  Serial.println("    8-SERVO EASY TOGGLE TEST BOARD    ");
  Serial.println("======================================");
  Serial.println("To toggle START / STOP:");
  Serial.println("  1. Press ENTER in Serial Monitor (or send anything)");
  Serial.println("  2. Or press the physical button on PIN 2");
  Serial.println("======================================");
  Serial.println("Status: IDLE (Waiting...)");
}

void loop() {
  // 1. Toggle on any Serial Input
  if (Serial.available() > 0) {
    // Clear buffer
    while(Serial.available() > 0) {
      Serial.read();
    }
    toggleState();
  }

  // 2. Toggle on Physical Button Press
  bool currentButtonReading = digitalRead(buttonPin);
  if (currentButtonReading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > 50) {
    // If the button state has changed and is now LOW (pressed)
    if (currentButtonReading == LOW && lastButtonState == HIGH) {
      toggleState();
      delay(200); // Simple debounce delay to prevent double triggers
    }
  }
  lastButtonState = currentButtonReading;

  // Non-blocking sweep logic
  if (isRunning) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastMoveTime >= 1500) {
      lastMoveTime = currentMillis;
      
      if (sweepState == 0) {
        Serial.println("Sweeping to 160 degrees...");
        for (int i = 0; i < 8; i++) {
          servos[i].write(160);
        }
        sweepState = 1;
      } else {
        Serial.println("Sweeping to 20 degrees...");
        for (int i = 0; i < 8; i++) {
          servos[i].write(20);
        }
        sweepState = 0;
      }
    }
  }
}




