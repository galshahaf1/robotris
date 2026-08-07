#include "Config.h"
#include "Motions.h"
#include "SerialHandler.h"
#include <EEPROM.h>
#include <WiFiS3.h>
#include "Credentials.h"

bool lastButtonState = HIGH; 
unsigned long buttonPressTime = 0;
bool isPressing = false;

const byte EEPROM_MAGIC = 0xA6;

// WiFi settings
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
WiFiServer server(80);

void handleWiFiCommands() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  WiFiClient client = server.available();
  if (!client) return;
  
  // Read request headers
  String request = "";
  while (client.connected() && client.available()) {
    char c = client.read();
    request += c;
    if (c == '\n' && request.endsWith("\r\n\r\n")) {
      break;
    }
  }
  
  // Parse path
  int firstSpace = request.indexOf(' ');
  int secondSpace = request.indexOf(' ', firstSpace + 1);
  if (firstSpace == -1 || secondSpace == -1) {
    client.stop();
    return;
  }
  
  String method = request.substring(0, firstSpace);
  String path = request.substring(firstSpace + 1, secondSpace);
  
  String responseHeader = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/plain\r\n"
                          "Access-Control-Allow-Origin: *\r\n"
                          "Connection: close\r\n\r\n";
  String responseBody = "";
  
  if (path == "/configs") {
    for (int i = 1; i <= 8; i++) {
      responseBody += "CFG:" + String(i) + ":" + 
                      String(motionConfigs[i].speed, 2) + ":" + 
                      String(motionConfigs[i].amplitude, 2) + ":" + 
                      String(motionConfigs[i].centerOffset, 2) + ":" + 
                      String(motionConfigs[i].phaseOffset, 2) + "\n";
    }
    responseBody += "CFG_DONE\n";
  } 
  else if (path.startsWith("/set")) {
    int mode = -1;
    float speed = 0, amplitude = 0, centerOffset = 0, phaseOffset = 0;
    
    int modeIdx = path.indexOf("mode=");
    int speedIdx = path.indexOf("speed=");
    int ampIdx = path.indexOf("amp=");
    int offsetIdx = path.indexOf("offset=");
    int phaseIdx = path.indexOf("phase=");
    
    if (modeIdx != -1) mode = path.substring(modeIdx + 5).toInt();
    if (speedIdx != -1) speed = path.substring(speedIdx + 6).toFloat();
    if (ampIdx != -1) amplitude = path.substring(ampIdx + 4).toFloat();
    if (offsetIdx != -1) centerOffset = path.substring(offsetIdx + 7).toFloat();
    if (phaseIdx != -1) phaseOffset = path.substring(phaseIdx + 6).toFloat();
    
    if (mode >= 1 && mode <= 8) {
      motionConfigs[mode].speed = speed;
      motionConfigs[mode].amplitude = amplitude;
      motionConfigs[mode].centerOffset = centerOffset;
      motionConfigs[mode].phaseOffset = phaseOffset;
      responseBody = "ACK:SET:" + String(mode) + ":" + 
                     String(speed, 2) + ":" + 
                     String(amplitude, 2) + ":" + 
                     String(centerOffset, 2) + ":" + 
                     String(phaseOffset, 2) + "\n";
    }
  } 
  else if (path.startsWith("/mode")) {
    int valIdx = path.indexOf("val=");
    if (valIdx != -1) {
      int val = path.substring(valIdx + 4).toInt();
      if (val >= 1 && val <= 8) {
        currentState = (SystemState)val;
        responseBody = "ACK:MODE:" + String(val) + "\n";
      }
    }
  } 
  else if (path.startsWith("/save")) {
    int modeIdx = path.indexOf("mode=");
    if (modeIdx != -1) {
      int mode = path.substring(modeIdx + 5).toInt();
      if (mode >= 1 && mode <= 8) {
        int addr = 1 + (mode - 1) * sizeof(MotionParams);
        EEPROM.put(addr, motionConfigs[mode]);
        responseBody = "ACK:SAVE:" + String(mode) + "\n";
      }
    }
  } 
  else if (path == "/status") {
    responseBody = "ACK:MODE:" + String((int)currentState) + "\n";
  }
  
  client.print(responseHeader);
  client.print(responseBody);
  client.stop();
}

void setup() {
  Serial.begin(9600);
  
  // Attach 8 servos
  for (int i = 0; i < 8; i++) {
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
  
  // WiFi Setup (10s timeout)
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    server.begin();
  } else {
    Serial.println("\nWiFi Connection timed out. Running in Offline/Serial mode.");
  }
  
  Serial.println("System Started. Mode: 1 (Breathing)");
}

void loop() {
  unsigned long currentMillis = millis();

  // --- 1. Serial Commands Handling ---
  handleSerialCommands();

  // --- 1.5. WiFi Commands Handling ---
  handleWiFiCommands();

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

