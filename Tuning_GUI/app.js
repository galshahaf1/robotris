// --- UI Logic and State Management ---

// Local state for all 8 modes (synchronized with Arduino)
const configs = {
  1: { speed: 1500, amplitude: 40, centerOffset: 70, phaseOffset: 0.8 },
  2: { speed: 1500, amplitude: 60, centerOffset: 60, phaseOffset: 0.0 },
  3: { speed: 2400, amplitude: 80, centerOffset: 40, phaseOffset: 400.0 },
  4: { speed: 450, amplitude: 90, centerOffset: 45, phaseOffset: 1.5 },
  5: { speed: 1000, amplitude: 6, centerOffset: 90, phaseOffset: 1.5 },
  6: { speed: 2400, amplitude: 60, centerOffset: 60, phaseOffset: 400.0 },
  7: { speed: 800, amplitude: 180, centerOffset: 0, phaseOffset: 200.0 },
  8: { speed: 1000, amplitude: 0, centerOffset: 90, phaseOffset: 0.0 }
};

const modeNames = {
  1: "Breathing (Normal)",
  2: "Sweep (Parallel)",
  3: "Conflict (Sweeping Wave)",
  4: "Ripple (Gentle Breeze)",
  5: "Shiver (Tremor)",
  6: "Roll (One-way Wave)",
  7: "Stadium Wave (Wave)",
  8: "Sleep (Rest)"
};

let currentSelectedMode = 1;
let isConnected = false;
let sendTimeout = null;

// DOM Elements
const connectBtn = document.getElementById("connect-btn");
const statusDot = document.getElementById("status-dot");
const statusText = document.getElementById("status-text");
const activeModeName = document.getElementById("active-mode-name");
const tunerTitle = document.getElementById("current-tuner-title");
const tunerControls = document.getElementById("tuner-controls");
const saveBtn = document.getElementById("save-btn");
const terminalLog = document.getElementById("terminal-log");

const sliderSpeed = document.getElementById("param-speed");
const sliderAmplitude = document.getElementById("param-amplitude");
const sliderCenter = document.getElementById("param-center");
const sliderPhase = document.getElementById("param-phase");

const valSpeed = document.getElementById("val-speed");
const valAmplitude = document.getElementById("val-amplitude");
const valCenter = document.getElementById("val-center");
const valPhase = document.getElementById("val-phase");

const unitSpeed = document.getElementById("unit-speed");
const unitPhase = document.getElementById("unit-phase");

// Helper: Log to terminal UI
function logToTerminal(message, type = "info") {
  const time = new Date().toLocaleTimeString();
  const div = document.createElement("div");
  div.className = `log-line ${type}`;
  div.textContent = `[${time}] ${message}`;
  terminalLog.appendChild(div);
  terminalLog.scrollTop = terminalLog.scrollHeight;
}

// Update the sliders to reflect a specific mode's configurations
function updateSlidersUI(mode) {
  const cfg = configs[mode];
  if (!cfg) return;

  // Custom bounds/units depending on mode
  if (mode === 3 || mode === 6 || mode === 7) {
    // Modes with millisecond-based phase offsets
    sliderPhase.min = 0;
    sliderPhase.max = 1000;
    sliderPhase.step = 10;
    unitPhase.textContent = "ms";
    if (mode === 3) {
      document.getElementById("desc-phase").textContent = "השהיית זמן במילי-שניות (דיליי) בין הכנפיים החיצוניות לפנימיות.";
    } else {
      document.getElementById("desc-phase").textContent = "השהיית זמן במילי-שניות בין כנף לכנף (יוצרת אפקט גל זורם מצד לצד).";
    }
  } else {
    // Normal sine modes use numeric multipliers
    sliderPhase.min = 0;
    sliderPhase.max = 5;
    sliderPhase.step = 0.1;
    unitPhase.textContent = "mult";
    document.getElementById("desc-phase").textContent = "התיאום בין הכנפיים - העיכוב וההשהיה בין כנף לכנף שיוצרים את אפקט הגל.";
  }

  // Sleep mode disables parameter tuning
  if (mode === 8) {
    tunerControls.classList.add("disabled-overlay");
    tunerTitle.textContent = `${modeNames[mode]} (No Parameters)`;
  } else {
    if (isConnected) {
      tunerControls.classList.remove("disabled-overlay");
    }
    tunerTitle.textContent = `Tuning Mode: ${modeNames[mode]}`;
  }

  sliderSpeed.value = cfg.speed;
  sliderAmplitude.value = cfg.amplitude;
  sliderCenter.value = cfg.centerOffset;
  sliderPhase.value = cfg.phaseOffset;

  valSpeed.textContent = cfg.speed;
  valAmplitude.textContent = cfg.amplitude;
  valCenter.textContent = cfg.centerOffset;
  valPhase.textContent = cfg.phaseOffset;
}

// Collect values from sliders and send to Arduino (Throttled)
function sendCurrentParams() {
  if (!isConnected) return;

  const mode = currentSelectedMode;
  const speed = parseFloat(sliderSpeed.value);
  const amplitude = parseFloat(sliderAmplitude.value);
  const centerOffset = parseFloat(sliderCenter.value);
  const phaseOffset = parseFloat(sliderPhase.value);

  // Update local model
  configs[mode] = { speed, amplitude, centerOffset, phaseOffset };

  // Clear any pending throttled message
  if (sendTimeout) clearTimeout(sendTimeout);

  // Throttle to prevent serial buffer overflow
  sendTimeout = setTimeout(async () => {
    const cmd = `SET:${mode}:${speed.toFixed(2)}:${amplitude.toFixed(2)}:${centerOffset.toFixed(2)}:${phaseOffset.toFixed(2)}`;
    await window.serialConn.sendCommand(cmd);
  }, 50);
}

// Mode Selection Button Click
function selectMode(mode) {
  currentSelectedMode = mode;
  
  // Highlight active tab
  document.querySelectorAll(".mode-tab").forEach(tab => {
    if (parseInt(tab.dataset.mode) === mode) {
      tab.classList.add("active");
    } else {
      tab.classList.remove("active");
    }
  });

  updateSlidersUI(mode);

  if (isConnected) {
    // Command Arduino to switch to this mode immediately on selection
    window.serialConn.sendCommand(`MODE:${mode}`);
  }
}

// Bind Sliders input event
[sliderSpeed, sliderAmplitude, sliderCenter, sliderPhase].forEach(slider => {
  slider.addEventListener("input", (e) => {
    // Update numerical labels immediately for visual responsiveness
    const targetValId = "val-" + e.target.id.replace("param-", "");
    document.getElementById(targetValId).textContent = e.target.value;
    
    sendCurrentParams();
  });
});

// Setup Connect Button click
connectBtn.addEventListener("click", async () => {
  if (isConnected) {
    logToTerminal("Disconnecting...");
    await window.serialConn.disconnect();
  } else {
    logToTerminal("Requesting Serial Port connection...");
    try {
      await window.serialConn.connect();
    } catch (err) {
      logToTerminal("Connection failed: " + err.message, "error");
    }
  }
});



// Save Button Click (Sends SAVE command to Arduino)
saveBtn.addEventListener("click", () => {
  if (isConnected) {
    window.serialConn.sendCommand(`SAVE:${currentSelectedMode}`);
  }
});

// Setup Mode Tab click handlers
document.querySelectorAll(".mode-tab").forEach(tab => {
  tab.addEventListener("click", () => {
    const mode = parseInt(tab.dataset.mode);
    selectMode(mode);
  });
});

// --- Web Serial Callbacks Registration ---

window.serialConn.onConnect = () => {
  isConnected = true;
  connectBtn.textContent = "Disconnect";
  connectBtn.className = "btn btn-secondary";
  statusDot.className = "dot connected";
  statusText.textContent = "Connected";

  
  saveBtn.removeAttribute("disabled");
  
  if (currentSelectedMode !== 8) {
    tunerControls.classList.remove("disabled-overlay");
  }
  logToTerminal("Connected successfully to Arduino. Syncing configurations...", "success");
};

window.serialConn.onDisconnect = () => {
  isConnected = false;
  connectBtn.textContent = "Connect Arduino";
  connectBtn.className = "btn btn-primary";
  statusDot.className = "dot disconnected";
  statusText.textContent = "Disconnected";
  activeModeName.textContent = "-";

  saveBtn.setAttribute("disabled", "true");
  tunerControls.classList.add("disabled-overlay");
  
  // Remove running highlights
  document.querySelectorAll(".mode-tab").forEach(tab => {
    tab.classList.remove("running");
  });
  
  logToTerminal("Connection closed.");
};

window.serialConn.onLineReceived = (line) => {
  // Silent logs for heartbeat/config parsing, print warnings/acks clearly
  if (line.startsWith("CFG:")) {
    // Format: CFG:<mode>:<speed>:<amplitude>:<centerOffset>:<phaseOffset>
    const parts = line.split(":");
    if (parts.length === 6) {
      const mode = parseInt(parts[1]);
      configs[mode] = {
        speed: parseFloat(parts[2]),
        amplitude: parseFloat(parts[3]),
        centerOffset: parseFloat(parts[4]),
        phaseOffset: parseFloat(parts[5])
      };
      // If synced the current selected mode, update UI sliders
      if (mode === currentSelectedMode) {
        updateSlidersUI(mode);
      }
    }
  } else if (line === "CFG_DONE") {
    logToTerminal("Configurations fully synced from Arduino.", "success");
  } else if (line.startsWith("ACK:SET:")) {
    logToTerminal(`Setting updated: ${line}`, "success");
  } else if (line.startsWith("ACK:SAVE:")) {
    const mode = parseInt(line.split(":")[2]);
    logToTerminal(`Saved config for ${modeNames[mode] || mode} permanently to Arduino!`, "success");
  } else if (line.startsWith("ACK:MODE:")) {
    const mode = parseInt(line.split(":")[2]);
    activeModeName.textContent = modeNames[mode] || mode;
    logToTerminal(`Mode switched: ${modeNames[mode] || mode}`);
    
    // Highlight the running mode in the sidebar list
    document.querySelectorAll(".mode-tab").forEach(tab => {
      if (parseInt(tab.dataset.mode) === mode) {
        tab.classList.add("running");
      } else {
        tab.classList.remove("running");
      }
    });
  } else {
    // Normal print output from Arduino (like startup messages)
    logToTerminal(`Arduino: ${line}`);
  }
};

window.serialConn.onError = (err) => {
  logToTerminal(`Error: ${err.message}`, "error");
};

// Initial state
selectMode(1);
