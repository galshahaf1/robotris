// --- Unified Robot Connection Wrapper (Serial & WiFi) ---

class RobotConnection {
  constructor() {
    this.connectionType = 'serial'; // 'serial' or 'wifi'
    this.wifiIp = '';
    this.isConnected = false;
    
    // Serial specific state
    this.port = null;
    this.reader = null;
    this.writer = null;
    this.inputStream = null;
    this.outputStream = null;
    this.keepReading = false;
    
    // Callbacks
    this.onConnect = () => {};
    this.onDisconnect = () => {};
    this.onLineReceived = (line) => {};
    this.onError = (err) => {};
  }

  isSerialSupported() {
    return 'serial' in navigator;
  }

  async connect(type = 'serial', ip = '') {
    this.connectionType = type;
    this.wifiIp = ip ? ip.trim() : '';

    if (this.connectionType === 'serial') {
      if (!this.isSerialSupported()) {
        throw new Error("Web Serial API is not supported in this browser. Please use Chrome, Edge or Opera.");
      }

      try {
        this.port = await navigator.serial.requestPort();
        await this.port.open({ baudRate: 9600 });
        
        this.keepReading = true;
        this.isConnected = true;
        
        // Setup text encoder and decoder streams
        const textDecoder = new TextDecoderStream();
        this.inputStream = this.port.readable.pipeTo(textDecoder.writable);
        this.reader = textDecoder.readable.getReader();

        const textEncoder = new TextEncoderStream();
        this.outputStream = textEncoder.readable.pipeTo(this.port.writable);
        this.writer = textEncoder.writable.getWriter();

        this.onConnect();
        
        // Start read loop (non-blocking)
        this.readLoop();
        
        // Auto request configurations
        await this.sendCommand("GET_CONFIGS");
      } catch (err) {
        this.onError(err);
        this.disconnect();
        throw err;
      }
    } 
    else if (this.connectionType === 'wifi') {
      if (!this.wifiIp) {
        throw new Error("אנא הזן כתובת IP תקינה עבור הארדואינו.");
      }
      
      try {
        // Test connection by requesting configs
        this.isConnected = true; 
        this.onConnect();
        await this.fetchWiFiConfigs();
      } catch (err) {
        this.isConnected = false;
        this.onError(err);
        this.disconnect();
        throw new Error("לא ניתן להתחבר לכתובת ה-IP המבוקשת. ודא שהארדואינו דולק, מחובר לאותה רשת WiFi וה-IP נכון.");
      }
    }
  }

  async fetchWiFiConfigs() {
    try {
      const response = await fetch(`http://${this.wifiIp}/configs`);
      if (!response.ok) throw new Error("HTTP error: " + response.status);
      const text = await response.text();
      
      const lines = text.split("\n");
      for (let line of lines) {
        line = line.trim();
        if (line) {
          this.onLineReceived(line);
        }
      }
    } catch (err) {
      this.onError(err);
      this.disconnect();
      throw err;
    }
  }

  async fetchWiFiStatus() {
    if (this.connectionType !== 'wifi' || !this.isConnected) return;
    try {
      const response = await fetch(`http://${this.wifiIp}/status`);
      if (response.ok) {
        const text = (await response.text()).trim();
        if (text) {
          this.onLineReceived(text);
        }
      }
    } catch (err) {
      console.warn("Status fetch failed", err);
    }
  }

  async readLoop() {
    let buffer = "";
    while (this.port && this.port.readable && this.keepReading) {
      try {
        const { value, done } = await this.reader.read();
        if (done) {
          break;
        }
        if (value) {
          buffer += value;
          let lines = buffer.split("\n");
          buffer = lines.pop(); // Keep the last incomplete line in buffer
          
          for (let line of lines) {
            line = line.trim();
            if (line) {
              this.onLineReceived(line);
            }
          }
        }
      } catch (err) {
        this.onError(err);
        break;
      }
    }
  }

  async sendCommand(command) {
    if (!this.isConnected) {
      console.warn("Connection not active. Command not sent: " + command);
      return false;
    }

    if (this.connectionType === 'serial') {
      if (!this.writer) return false;
      try {
        await this.writer.write(command + "\n");
        return true;
      } catch (err) {
        this.onError(err);
        return false;
      }
    } 
    else if (this.connectionType === 'wifi') {
      try {
        let url = `http://${this.wifiIp}`;
        if (command.startsWith("SET:")) {
          const parts = command.split(":");
          // Format: SET:<mode>:<speed>:<amplitude>:<centerOffset>:<phaseOffset>
          url += `/set?mode=${parts[1]}&speed=${parts[2]}&amp=${parts[3]}&offset=${parts[4]}&phase=${parts[5]}`;
        } else if (command.startsWith("MODE:")) {
          const modeVal = command.split(":")[1];
          url += `/mode?val=${modeVal}`;
        } else if (command.startsWith("SAVE:")) {
          const modeVal = command.split(":")[1];
          url += `/save?mode=${modeVal}`;
        } else if (command === "GET_CONFIGS") {
          await this.fetchWiFiConfigs();
          return true;
        } else {
          return false;
        }

        const response = await fetch(url);
        if (response.ok) {
          const responseText = (await response.text()).trim();
          if (responseText) {
            const lines = responseText.split("\n");
            for (let line of lines) {
              if (line.trim()) this.onLineReceived(line.trim());
            }
          }
          return true;
        }
        return false;
      } catch (err) {
        this.onError(err);
        return false;
      }
    }
  }

  async disconnect() {
    this.isConnected = false;
    this.keepReading = false;
    
    if (this.reader) {
      try {
        await this.reader.cancel();
      } catch (e) {}
      this.reader = null;
    }

    if (this.writer) {
      try {
        await this.writer.close();
      } catch (e) {}
      this.writer = null;
    }

    if (this.port) {
      try {
        await this.port.close();
      } catch (e) {}
      this.port = null;
    }

    this.onDisconnect();
  }
}

// Export global instance
window.serialConn = new RobotConnection();

