#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Initialize the Adafruit PWM Servo Driver library (default address is 0x40)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// We use raw 12-bit tick values (0 to 4095) for 50Hz frequency:
// 1ms pulse (usually 0 degrees)      -> ~205 ticks
// 1.5ms pulse (usually 90 degrees)   -> ~307 ticks
// 2ms pulse (usually 180 degrees)    -> ~410 ticks
// Let's expand the range slightly to match standard servo limits:
#define SERVOMIN  130 // Minimum tick length (approx 0.6ms)
#define SERVOMAX  490 // Maximum tick length (approx 2.4ms)

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10); // Wait for serial monitor

  Serial.println("\n=================================");
  Serial.println("PCA9685 Raw Register Test");
  Serial.println("=================================");

  // --- 1. Quick I2C Scan ---
  Wire.begin();
  Wire.beginTransmission(0x40);
  if (Wire.endTransmission() == 0) {
    Serial.println("SUCCESS: PCA9685 detected at 0x40.");
  } else {
    Serial.println("ERROR: PCA9685 NOT detected! Check SDA/SCL/GND/5V.");
    while (1) delay(100);
  }

  // --- 2. Initialize Driver ---
  pwm.begin();
  pwm.setPWMFreq(50); // Set standard 50Hz frequency
  delay(10);
  Serial.println("Driver initialized at 50Hz using raw values.");
  Serial.println("Sweep testing starts now...");
}

void loop() {
  Serial.println("\nSweeping each servo channel (0 to 7)...");
  
  for (int servoNum = 0; servoNum < 8; servoNum++) {
    Serial.print("Servo ");
    Serial.print(servoNum);
    
    // Move to Min
    Serial.print(" -> MIN");
    pwm.setPWM(servoNum, 0, SERVOMIN);
    delay(800);

    // Move to Center
    Serial.print(" -> CENTER");
    pwm.setPWM(servoNum, 0, (SERVOMIN + SERVOMAX) / 2);
    delay(800);

    // Move to Max
    Serial.print(" -> MAX");
    pwm.setPWM(servoNum, 0, SERVOMAX);
    delay(800);

    // Return to Min
    pwm.setPWM(servoNum, 0, SERVOMIN);
    delay(400);
    Serial.println(" [Done]");
  }

  Serial.println("Simultaneous movement test (all channels)...");
  // All to MAX
  for (int i = 0; i < 8; i++) {
    pwm.setPWM(i, 0, SERVOMAX);
  }
  delay(1500);

  // All to MIN
  for (int i = 0; i < 8; i++) {
    pwm.setPWM(i, 0, SERVOMIN);
  }
  delay(1500);

  Serial.println("Loop finished. Waiting 3 seconds before next run.");
  delay(3000);
}
