# Robotris Wing Motion Tuning System

This workspace contains a modular, parameter-driven system designed to control and fine-tune organic motions for a 4-servo robotic assembly (wings/shutters).

## Project Structure

```text
Robotris/
├── README.md                           # Developer & Agent documentation
├── Wing_Motion_Arduino/                # Arduino Project Source Code
│   ├── Wing_Motion_Arduino.ino         # Main loop and state machine
│   ├── Config.h                        # Structs, pins, and shared config
│   ├── Motions.h / Motions.cpp         # Physical motion calculation formulas
│   └── SerialHandler.h / .cpp          # Serial command parser
└── Tuning_GUI/                         # Web Serial Parameter Tuning Dashboard
    ├── index.html                      # Layout Structure
    ├── style.css                       # Premium Glassmorphic Styling
    ├── serial.js                       # Web Serial API Connection Wrapper
    └── app.js                          # UI State and synchronization logic
```

---

## Arduino Architecture (C++)

To allow easy extensions and prevent reading huge files, the Arduino codebase is modularized:
1. **[Config.h](file:///c:/Users/isa/Desktop/Robotris/Wing_Motion_Arduino/Config.h)**: Centralizes pin definitions (`servoPins`, `buttonPin`), global variables, the state enum (`SystemState`), and the `MotionParams` configuration struct.
2. **[Motions.h](file:///c:/Users/isa/Desktop/Robotris/Wing_Motion_Arduino/Motions.h) / [Motions.cpp](file:///c:/Users/isa/Desktop/Robotris/Wing_Motion_Arduino/Motions.cpp)**: Contains all math equations for the 7 movement modes. Instead of hardcoded constants, they read dynamically from the `motionConfigs` struct array.
3. **[SerialHandler.h](file:///c:/Users/isa/Desktop/Robotris/Wing_Motion_Arduino/SerialHandler.h) / [SerialHandler.cpp](file:///c:/Users/isa/Desktop/Robotris/Wing_Motion_Arduino/SerialHandler.cpp)**: Listens to incoming serial lines, parses commands, updates configurations, and acknowledges them.

---

## Communication Protocol (Serial)

All commands are newline-terminated (`\n`).

### 1. GUI requests Configs
* **Sent by GUI**: `GET_CONFIGS`
* **Arduino Response**: Sends a stream of current parameters for each mode, followed by a completion flag:
  ```text
  CFG:1:1500.00:40.00:70.00:0.80
  CFG:2:2000.00:20.00:140.00:0.30
  ...
  CFG:7:1000.00:0.00:90.00:0.00
  CFG_DONE
  ```

### 2. GUI updates parameters of a mode
* **Sent by GUI**: `SET:<mode>:<speed>:<amplitude>:<centerOffset>:<phaseOffset>`
  * *Example*: `SET:1:1200:50:80:0.5`
* **Arduino Response**: `ACK:SET:<mode>:<speed>:<amplitude>:<centerOffset>:<phaseOffset>`

### 3. GUI commands mode switch
* **Sent by GUI**: `MODE:<mode>`
  * *Example*: `MODE:3`
* **Arduino Response**: `ACK:MODE:<mode>`

---

## Web Serial GUI Dashboard

A premium dark-themed tuner interface built with **HTML5, CSS3, and JavaScript**. 
It leverages the **Web Serial API** allowing Chrome, Edge, and Opera browsers to communicate **directly with the Arduino Uno R4 Minima via USB** without installing any local server, Python environment, or drivers.

### How to use:
1. Open the [Tuning_GUI/index.html](file:///c:/Users/isa/Desktop/Robotris/Tuning_GUI/index.html) file directly in a Google Chrome or Microsoft Edge browser.
2. Click **Connect Arduino** and select the Serial port of your Arduino Uno R4.
3. Once connected, the interface automatically loads current values from the Arduino.
4. Drag the sliders to tune the motion speed, amplitude, offset, and phase differences. The robot moves immediately in response to the changes.
