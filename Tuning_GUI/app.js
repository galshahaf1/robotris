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
  8: { speed: 1000, amplitude: 0, centerOffset: 90, phaseOffset: 0.0 },
  9: { speed: 800, amplitude: 180, centerOffset: 0, phaseOffset: 200.0 }
};

const modeNames = {
  1: "Breathing (Normal)",
  2: "Sweep (Parallel)",
  3: "Mirror (Symmetric Wave)",
  4: "Ripple (Gentle Breeze)",
  5: "Shiver (Tremor)",
  6: "Roll (One-way Wave)",
  7: "Stadium Wave (Wave)",
  8: "Sleep (Rest)",
  9: "One Time (Single Wave)"
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

// Connection Type Selector elements
const connectionType = document.getElementById("connection-type");
const wifiIpInput = document.getElementById("wifi-ip");
let wifiPollInterval = null;

// Initialize WiFi input visibility
wifiIpInput.style.display = "none";


const valSpeed = document.getElementById("val-speed");
const valAmplitude = document.getElementById("val-amplitude");
const valCenter = document.getElementById("val-center");
const valPhase = document.getElementById("val-phase");

const unitSpeed = document.getElementById("unit-speed");
const unitPhase = document.getElementById("unit-phase");

// Helper: Append log to terminal UI
function appendLogToDOM(message, type, timeStr) {
  const div = document.createElement("div");
  div.className = `log-line ${type}`;
  div.textContent = `[${timeStr}] ${message}`;
  terminalLog.appendChild(div);
  terminalLog.scrollTop = terminalLog.scrollHeight;
}

// Helper: Load logs from localStorage
function loadLogsFromStorage() {
  try {
    const savedLogs = localStorage.getItem("robot_logs");
    if (savedLogs) {
      const logs = JSON.parse(savedLogs);
      // Clear welcome message before rendering saved logs
      terminalLog.innerHTML = "";
      logs.forEach(logObj => {
        appendLogToDOM(logObj.message, logObj.type, logObj.time);
      });
    }
  } catch (err) {
    console.error("Failed to load logs from localStorage:", err);
  }
}

// Helper: Log to terminal UI, localStorage, and Python server (JSONL)
function logToTerminal(message, type = "info") {
  const now = new Date();
  const timeStr = now.toLocaleTimeString();
  const isoStr = now.toISOString();

  // 1. Render to screen
  appendLogToDOM(message, type, timeStr);

  const logObject = {
    timestamp: isoStr,
    time: timeStr,
    type: type,
    message: message
  };

  // 2. Persist in LocalStorage (keep last 150 items to save space)
  try {
    const savedLogs = localStorage.getItem("robot_logs");
    let logsList = savedLogs ? JSON.parse(savedLogs) : [];
    logsList.push(logObject);
    if (logsList.length > 150) {
      logsList.shift();
    }
    localStorage.setItem("robot_logs", JSON.stringify(logsList));
  } catch (err) {
    console.error("Failed to write to localStorage:", err);
  }

  // 3. Send to custom Python server to write into robot_log.jsonl
  fetch("/api/log", {
    method: "POST",
    headers: {
      "Content-Type": "application/json"
    },
    body: JSON.stringify(logObject)
  }).catch(err => {
    // Fail silently in browser console if not running via server (e.g. running offline as file://)
    console.debug("Could not log to Python server:", err.message);
  });
}

// Update the sliders to reflect a specific mode's configurations
function updateSlidersUI(mode) {
  const cfg = configs[mode];
  if (!cfg) return;

  // Custom bounds/units depending on mode
  if (mode === 3 || mode === 6 || mode === 7 || mode === 9) {
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
  if (currentSelectedMode === 9) return;

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
    if (mode === 9) {
      window.serialConn.sendCommand("TRIGGER");
    }
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

// Toggle IP input visibility on connection type change
connectionType.addEventListener("change", () => {
  if (connectionType.value === "wifi") {
    wifiIpInput.style.display = "inline-block";
  } else {
    wifiIpInput.style.display = "none";
  }
});

// Setup Connect Button click
connectBtn.addEventListener("click", async () => {
  if (isConnected) {
    logToTerminal("Disconnecting...");
    await window.serialConn.disconnect();
  } else {
    const type = connectionType.value;
    const ip = wifiIpInput.value;
    
    if (type === "serial") {
      logToTerminal("Requesting Serial Port connection...");
    } else {
      logToTerminal(`Requesting WiFi connection to ${ip}...`);
    }
    
    try {
      await window.serialConn.connect(type, ip);
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
  
  if (window.serialConn.connectionType === "wifi") {
    logToTerminal(`Connected successfully over WiFi to ${window.serialConn.wifiIp}. Syncing configurations...`, "success");
    // Start polling for status updates (heartbeat)
    wifiPollInterval = setInterval(() => {
      window.serialConn.fetchWiFiStatus();
    }, 1500);
  } else {
    logToTerminal("Connected successfully to Arduino via USB. Syncing configurations...", "success");
  }
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
  
  // Clear WiFi polling if active
  if (wifiPollInterval) {
    clearInterval(wifiPollInterval);
    wifiPollInterval = null;
  }
  
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

// Clear Log Button Click Listener
const clearLogBtn = document.getElementById("clear-log-btn");
if (clearLogBtn) {
  clearLogBtn.addEventListener("click", () => {
    // Clear screen DOM
    terminalLog.innerHTML = "";
    // Clear localStorage
    try {
      localStorage.removeItem("robot_logs");
    } catch (e) {}
    logToTerminal("Screen log cleared.", "info");
  });
}

// Initial state
selectMode(1);
loadLogsFromStorage();
