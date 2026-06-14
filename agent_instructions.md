# Instructions for AI Agents / Copilots

Welcome to the Robotris repository. If you are an AI agent paired with the developer to continue working on this codebase, please follow the guidelines below to ensure seamless Git operation and code consistency.

---

## 1. Git & Workspace Configuration

The local environment is a Windows machine running PowerShell. Due to system-wide environment path propagation delays, standard `git` commands might not be recognized directly on startup in the terminal.

### Git Path & Command Invocation:
* **Absolute Git Path**: `C:\Program Files\Git\cmd\git.exe`
* **Command Syntax**: Always invoke Git commands in PowerShell using the absolute path to prevent PATH issues:
  ```powershell
  & "C:\Program Files\Git\cmd\git.exe" status
  & "C:\Program Files\Git\cmd\git.exe" add .
  & "C:\Program Files\Git\cmd\git.exe" commit -m "Your commit message"
  & "C:\Program Files\Git\cmd\git.exe" push
  ```
* **Authentication**: Git credentials are cached locally on this machine and synced to the remote repository: `https://github.com/galshahaf1/robotris.git`. Pushing directly will succeed without prompting for login.

---

## 2. Mandatory Workflow Rules

1. **Auto Commit & Push**: You are **required** to commit and push all code changes to GitHub at the end of every task or feature addition.
2. **Modular C++ Structure**: Keep the Arduino code modularized inside `Wing_Motion_V6_Tuning/`. Do not consolidate it into a single huge `.ino` file. All equations must be stored in `Motions.cpp`, configurations in `Config.h`, and communication logic in `SerialHandler.cpp`.
3. **No Redundant UI Buttons**: The GUI sliders update parameters dynamically on drag. Avoid adding manual "Apply" buttons unless explicitly requested.

---

## 3. Communication Protocol (Serial)

All communication between the Web GUI and the Arduino R4 Minima happens over USB Serial (9600 Baud) using newline-terminated (`\n`) string messages:
* `GET_CONFIGS` -> Requests configuration sync from the board.
* `SET:<mode>:<speed>:<amplitude>:<centerOffset>:<phaseOffset>` -> Updates motion parameters.
* `MODE:<mode>` -> Switches the running movement mode.
* `SAVE:<mode>` -> Triggers EEPROM storage for the specified mode.
