# Smart Plant Watering System

![Smart Plant Watering Dashboard]
![Web Dashboard ](https://github.com/user-attachments/assets/6c354af7-ad61-465a-bb92-10c19524b48b)


A microcontroller-based smart plant watering system built with an ESP8266, featuring automated watering based on soil moisture, motion detection alerts via EmailJS, and a custom web dashboard for monitoring and control.

## Overview

This project automates plant care by monitoring soil moisture, temperature, humidity, and motion using an ESP8266 microcontroller. It includes a web dashboard with gauges to visualize sensor data and buttons to manually control the water pump. When motion is detected near the plant (e.g., by a PIR sensor), an email alert is sent using EmailJS.

## Features

- **Automated Watering**: Turns the water pump on/off based on soil moisture thresholds (below 30% ON, above 30% OFF).
- **Sensor Monitoring**: Tracks temperature, humidity (DHT11), soil moisture, and motion (PIR sensor).
- **Web Dashboard**: Displays real-time sensor data with JustGage gauges and allows manual pump control.
- **Email Alerts**: Sends an email via EmailJS when motion is detected.
- **Manual Override**: Temporarily disables automation for 5 minutes via a physical button or web UI.
- **LCD Display**: Shows sensor readings and system status locally on a 16x2 I2C LCD.
- **Persistent State**: Saves pump and override states to EEPROM for reboot recovery.

## Hardware Requirements

- ESP8266 (e.g., NodeMCU)
- DHT11 Temperature & Humidity Sensor (D4)
- Soil Moisture Sensor (A0)
- PIR Motion Sensor (D5)
- Relay Module (D3)
- Push Button (D7)
- 16x2 I2C LCD (Address: 0x27)
- Jumper wires, breadboard, and power supply

## Software Requirements

- Arduino IDE (with ESP8266 board support)
- VS Code (with Live Server extension or Node.js)
- EmailJS account (free tier)
- Git (for cloning/uploading to GitHub)

## Setup Instructions
### 1. Hardware Setup
1. Connect components to the ESP8266:
   - DHT11: D4
   - Soil Moisture: A0
   - PIR: D5
   - Relay: D3
   - Push Button: D7 (with pull-up)
   - LCD: I2C pins (SDA, SCL)
2. Power the ESP8266 and ensure all sensors are functional.

### 2. Arduino Code
1. Install required libraries in Arduino IDE:
   - `ESP8266WiFi`
   - `ESP8266WebServer`
   - `LiquidCrystal_I2C`
   - `DHT sensor library by Adafruit`
2. Open `arduino/SmartPlant.ino` in Arduino IDE.
3. Update Wi-Fi credentials (`ssid`, `password`) if needed.
4. Upload the code to your ESP8266.
5. Open Serial Monitor (9600 baud) to note the IP address (e.g., `192.168.196.234`).

### 3. EmailJS Configuration
1. Sign up at [EmailJS](https://www.emailjs.com/).
2. Create an **Email Service** (e.g., `SmartPlantService`) and note the **Service ID**.
3. Create an **Email Template**:
   - Subject: `Smart Plant Alert: Motion Detected`
   - Body: `Motion detected near your plant! Time: {{time}}`
   - Note the **Template ID**.
4. Get your **Public Key** from the EmailJS dashboard.

### 4. Web Dashboard
1. Open `web/` in VS Code.
2. Update `script.js`:
   - Replace `ESP_IP` with your ESP8266 IP (e.g., `192.168.196.234`).
   - Replace EmailJS placeholders:
     - `YOUR_EMAILJS_USER_ID` (Public Key)
     - `YOUR_SERVICE_ID`
     - `YOUR_TEMPLATE_ID`
     - `your_email@example.com` (your email)
3. Run the dashboard:
   - **Live Server**: Right-click `index.html` > "Open with Live Server" (`http://127.0.0.1:5500`).
   - **Node.js**: Install dependencies (`npm init -y`, `npm install express`), create `server.js` (see below), and run `node server.js` (`http://localhost:3000`).
