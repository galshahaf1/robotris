#include "SerialHandler.h"
#include "Motions.h"
#include <EEPROM.h>

void handleSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("SET:")) {
      // Format: SET:<mode>:<speed>:<amplitude>:<centerOffset>:<phaseOffset>
      // Example: SET:1:1500.0:40.0:70.0:0.8
      
      int firstColon = command.indexOf(':');
      int secondColon = command.indexOf(':', firstColon + 1);
      int thirdColon = command.indexOf(':', secondColon + 1);
      int fourthColon = command.indexOf(':', thirdColon + 1);
      int fifthColon = command.indexOf(':', fourthColon + 1);
      
      if (firstColon != -1 && secondColon != -1 && thirdColon != -1 && fourthColon != -1 && fifthColon != -1) {
        int mode = command.substring(firstColon + 1, secondColon).toInt();
        float speed = command.substring(secondColon + 1, thirdColon).toFloat();
        float amplitude = command.substring(thirdColon + 1, fourthColon).toFloat();
        float centerOffset = command.substring(fourthColon + 1, fifthColon).toFloat();
        float phaseOffset = command.substring(fifthColon + 1).toFloat();
        
        if (mode >= 1 && mode <= 9) {
          motionConfigs[mode].speed = speed;
          motionConfigs[mode].amplitude = amplitude;
          motionConfigs[mode].centerOffset = centerOffset;
          motionConfigs[mode].phaseOffset = phaseOffset;
          
          Serial.print("ACK:SET:");
          Serial.print(mode);
          Serial.print(":");
          Serial.print(speed);
          Serial.print(":");
          Serial.print(amplitude);
          Serial.print(":");
          Serial.print(centerOffset);
          Serial.print(":");
          Serial.println(phaseOffset);
        } else {
          Serial.println("ERR:Invalid mode");
        }
      } else {
        Serial.println("ERR:Invalid SET format");
      }
    } 
    else if (command.startsWith("MODE:")) {
      // Format: MODE:<mode>
      // Example: MODE:2
      int colon = command.indexOf(':');
      if (colon != -1) {
        int mode = command.substring(colon + 1).toInt();
        if (mode >= 1 && mode <= 9) {
          if (mode == 9) {
            if (currentState != MODE_9_ONETIME) {
              currentOnetimeEndAngle = 90.0;
            }
            triggerOnetimeWave();
          }
          currentState = (SystemState)mode;
          Serial.print("ACK:MODE:");
          Serial.println(mode);
        } else {
          Serial.println("ERR:Invalid mode");
        }
      }
    }
    else if (command.startsWith("SAVE:")) {
      // Format: SAVE:<mode>
      // Example: SAVE:1
      int colon = command.indexOf(':');
      if (colon != -1) {
        int mode = command.substring(colon + 1).toInt();
        if (mode >= 1 && mode <= 9) {
          int addr = 1 + (mode - 1) * sizeof(MotionParams);
          EEPROM.put(addr, motionConfigs[mode]);
          
          Serial.print("ACK:SAVE:");
          Serial.println(mode);
        } else {
          Serial.println("ERR:Invalid mode");
        }
      }
    }
    else if (command.equals("GET_CONFIGS")) {
      // Send current configs to GUI
      for (int i = 1; i <= 9; i++) {
        Serial.print("CFG:");
        Serial.print(i);
        Serial.print(":");
        Serial.print(motionConfigs[i].speed);
        Serial.print(":");
        Serial.print(motionConfigs[i].amplitude);
        Serial.print(":");
        Serial.print(motionConfigs[i].centerOffset);
        Serial.print(":");
        Serial.println(motionConfigs[i].phaseOffset);
      }
      Serial.println("CFG_DONE");
    }
  }
}
