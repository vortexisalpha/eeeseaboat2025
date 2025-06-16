import axios from 'axios';

//update this to match Arduino's IP address given by serial monitor
const ARDUINO_BASE_URL = 'http://172.20.10.3';
const TIMEOUT_MS = 5000; // 5 second timeout for requests

export interface ArduinoResponse {
  status: string;
  message?: string;
}

class ArduinoService {
  private async sendMotorCommand(command: string): Promise<ArduinoResponse> {
    try {
      const response = await axios.get(
        `${ARDUINO_BASE_URL}/${command}`,
        { timeout: TIMEOUT_MS }
      );
      return response.data;
    } catch (error) {
      console.log('Error sending command to Arduino:', error);
      if (axios.isAxiosError(error)) {
        if (error.code === 'ECONNREFUSED') {
          throw new Error('Could not connect to Arduino. Please check if it is powered on and connected to the same network.');
        }
        if (error.code === 'ETIMEDOUT') {
          throw new Error('Request timed out. Please check if the Arduino is responding.');
        }
      }
      console.log(error);
      throw new Error('Failed to communicate with Arduino');
    }
  }

  private async sendToggleCommand(endpoint: string, state: boolean): Promise<ArduinoResponse> {
    try {
      const response = await axios.get(
        `${ARDUINO_BASE_URL}/${endpoint}?state=${state}`,
        { timeout: TIMEOUT_MS }
      );
      return response.data;
    } catch (error) {
      console.log('Error sending toggle command to Arduino:', error);
      if (axios.isAxiosError(error)) {
        if (error.code === 'ECONNREFUSED') {
          throw new Error('Could not connect to Arduino. Please check if it is powered on and connected to the same network.');
        }
        if (error.code === 'ETIMEDOUT') {
          throw new Error('Request timed out. Please check if the Arduino is responding.');
        }
      }
      throw new Error('Failed to communicate with Arduino');
    }
  }

  // Movement commands
  async moveUp(): Promise<ArduinoResponse> {
    return this.sendMotorCommand('up');
  }

  async moveDown(): Promise<ArduinoResponse> {
    return this.sendMotorCommand('down');
  }

  async moveLeft(): Promise<ArduinoResponse> {
    return this.sendMotorCommand('left');
  }

  async moveRight(): Promise<ArduinoResponse> {
    return this.sendMotorCommand('right');
  }

  async stop(): Promise<ArduinoResponse> {
    return this.sendMotorCommand('stop');
  }

  // Toggle controls
  async toggleUltrasonic(state: boolean): Promise<ArduinoResponse> {
    return this.sendToggleCommand('ultrasonic', state);
  }

  async toggleMagnetic(state: boolean): Promise<ArduinoResponse> {
    return this.sendToggleCommand('magnetic', state);
  }

  async toggleIR(state: boolean): Promise<ArduinoResponse> {
    return this.sendToggleCommand('IR', state);
  }

  async toggleRadiowaves(state: boolean): Promise<ArduinoResponse> {
    return this.sendToggleCommand('radiowaves', state);
  }

  // Stats endpoint
  async getStats(): Promise<{ stats: string[] }> {
    try {
      const response = await axios.get(`${ARDUINO_BASE_URL}/stats`, { timeout: TIMEOUT_MS });
      return response.data || { stats: [] };
    } catch (error) {
      // If endpoint doesn't exist or returns error, return empty stats array
      return { stats: [] };
    }
  }
}

export const arduinoService = new ArduinoService(); 