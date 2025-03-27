#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <EEPROM.h>

// Wi-Fi credentials
const char* ssid = "Infinix HOT 10S";
const char* password = "12345678";

// Initialize LCD and Web Server
LiquidCrystal_I2C lcd(0x27, 16, 2);
ESP8266WebServer server(80);
DHT dht(D4, DHT11);

// Define component pins
#define soil A0     // Soil Moisture Sensor
#define PIR D5      // PIR Motion Sensor
#define RELAY_PIN_1 D3   // Relay (Water Pump)
#define PUSH_BUTTON_1 D7 // Emergency Button

// Soil moisture thresholds
#define MOISTURE_THRESHOLD_ON 30  // Turn on pump when moisture < 30%
#define MOISTURE_THRESHOLD_OFF 30 // Turn off pump when moisture >= 30%

// Global variables
int relay1State = LOW;
int pushButton1State = HIGH;
bool manualOverride = false;
int currentMoistureValue = 0;
unsigned long manualOverrideTime = 0;
const unsigned long manualOverrideDuration = 300000; // 5 minutes
bool lastPIRstate = LOW;

void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);

  // Read saved state from EEPROM
  relay1State = EEPROM.read(0);
  manualOverride = EEPROM.read(1);

  // Initialize hardware
  lcd.begin();
  lcd.backlight();
  pinMode(PIR, INPUT);
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(PUSH_BUTTON_1, INPUT_PULLUP);
  digitalWrite(RELAY_PIN_1, relay1State);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.println(WiFi.localIP());

  dht.begin();

  // Web server routes with CORS support
  server.on("/data", HTTP_GET, []() {
    handleData();
  });
  
  server.on("/control", HTTP_GET, []() {
    handleControl();
  });
  
  server.onNotFound([]() {
    handleNotFound();
  });
  
  server.begin();

  // Initial display
  lcd.setCursor(0, 0);
  lcd.print("IP:");
  lcd.print(WiFi.localIP());
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(5000); // Show IP for 5 seconds
  updateDisplay();
}

void loop() {
  server.handleClient();
  soilMoistureSensor();
  DHT11sensor();
  checkPhysicalButton();
  PIRsensor();
  delay(100);
}

void updateDisplay() {
  lcd.clear();
  
  // Line 1: Temperature and Humidity
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(dht.readTemperature(), 1);
  lcd.print("C H:");
  lcd.print(dht.readHumidity(), 1);
  lcd.print("%");
  
  // Line 2: Soil, Motion, Water status
  lcd.setCursor(0, 1);
  lcd.print("S:");
  if (currentMoistureValue < 100) lcd.print(" ");
  if (currentMoistureValue < 10) lcd.print(" ");
  lcd.print(currentMoistureValue);
  lcd.print("%");
  
  lcd.setCursor(8, 1);
  lcd.print("M:");
  lcd.print(digitalRead(PIR) ? "ON" : "OF");
  
  lcd.setCursor(13, 1);
  lcd.print("W:");
  lcd.print(relay1State ? "ON" : "OF");
}

void handleData() {
  // Add CORS headers
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");

  if (server.method() == HTTP_OPTIONS) {
    server.send(204);
    return;
  }

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  bool pirValue = digitalRead(PIR);
  
  String json = "{\"temp\":" + String(t) + 
                ",\"humid\":" + String(h) + 
                ",\"moisture\":" + String(currentMoistureValue) + 
                ",\"pir\":" + String(pirValue) + 
                ",\"relay\":" + String(relay1State) + 
                ",\"manual\":" + String(manualOverride) + "}";
                
  server.send(200, "application/json", json);
}

void handleControl() {
  // Add CORS headers
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");

  if (server.method() == HTTP_OPTIONS) {
    server.send(204);
    return;
  }

  String state = server.arg("state");
  state.toUpperCase();
  
  if (state == "ON") {
    relay1State = HIGH;
    manualOverride = false;
    server.send(200, "text/plain", "Pump turned ON");
  } 
  else if (state == "OFF") {
    relay1State = LOW;
    manualOverride = false;
    server.send(200, "text/plain", "Pump turned OFF");
  } 
  else if (state == "MANUAL") {
    relay1State = LOW;
    manualOverride = true;
    manualOverrideTime = millis();
    server.send(200, "text/plain", "Manual mode activated");
  }
  else {
    server.send(400, "text/plain", "Invalid command");
    return;
  }
  
  digitalWrite(RELAY_PIN_1, relay1State);
  saveState();
  updateDisplay();
}

void handleNotFound() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(404, "text/plain", "Not Found");
}

void soilMoistureSensor() {
  int rawValue = analogRead(soil);
  currentMoistureValue = map(rawValue, 0, 1023, 0, 100);
  currentMoistureValue = 100 - currentMoistureValue;

  // Check if manual override has expired
  if (manualOverride && (millis() - manualOverrideTime >= manualOverrideDuration)) {
    manualOverride = false;
    saveState();
    updateDisplay();
  }

  // Automatic control only when not in manual mode
  if (!manualOverride) {
    if (currentMoistureValue < MOISTURE_THRESHOLD_ON && relay1State == LOW) {
      relay1State = HIGH;
      digitalWrite(RELAY_PIN_1, relay1State);
      saveState();
      updateDisplay();
    } 
    else if (currentMoistureValue >= MOISTURE_THRESHOLD_OFF && relay1State == HIGH) {
      relay1State = LOW;
      digitalWrite(RELAY_PIN_1, relay1State);
      saveState();
      updateDisplay();
    }
  }
}

void DHT11sensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  // Display is updated in updateDisplay()
}

void PIRsensor() {
  bool pirValue = digitalRead(PIR);
  if (pirValue != lastPIRstate) {
    lastPIRstate = pirValue;
    updateDisplay();
  }
}

void checkPhysicalButton() {
  int currentButtonState = digitalRead(PUSH_BUTTON_1);
  
  if (currentButtonState == LOW && pushButton1State != LOW) {
    // Button pressed - toggle between manual and auto mode
    if (manualOverride) {
      // If already in manual mode, return to auto
      manualOverride = false;
    } else {
      // Enter manual mode and turn off pump
      manualOverride = true;
      manualOverrideTime = millis();
      relay1State = LOW;
    }
    
    digitalWrite(RELAY_PIN_1, relay1State);
    saveState();
    updateDisplay();
    delay(300); // Simple debounce
  }
  
  pushButton1State = currentButtonState;
}

void saveState() {
  EEPROM.write(0, relay1State);
  EEPROM.write(1, manualOverride);
  EEPROM.commit();
}