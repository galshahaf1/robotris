// --- Web Serial API Connection Wrapper ---

class WebSerialConnection {
  constructor() {
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

  isSupported() {
    return 'serial' in navigator;
  }

  async connect() {
    if (!this.isSupported()) {
      throw new Error("Web Serial API is not supported in this browser. Please use Chrome, Edge or Opera.");
    }

    try {
      this.port = await navigator.serial.requestPort();
      await this.port.open({ baudRate: 9600 });
      
      this.keepReading = true;
      
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
    if (!this.writer) {
      console.warn("Serial connection not active. Command not sent: " + command);
      return false;
    }
    try {
      await this.writer.write(command + "\n");
      return true;
    } catch (err) {
      this.onError(err);
      return false;
    }
  }

  async disconnect() {
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
window.serialConn = new WebSerialConnection();
