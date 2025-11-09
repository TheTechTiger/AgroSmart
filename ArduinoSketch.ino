#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// Firebase project details
#define API_KEY ""
#define DATABASE_URL "https://agrosmart-XXXXXXX.firebaseio.com"

// DHT setup
#define DHTPIN D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Soil moisture sensor setup
#define SOIL_MOISTURE_PIN A0
#define MOISTURE_DRY 1024      // Value when completely dry (adjust based on your sensor)
#define MOISTURE_WET 300       // Value when completely wet (adjust based on your sensor)

// Relay setup
#define RELAY_PIN D3
#define MOISTURE_THRESHOLD_LOW 10   // Turn on relay when moisture is below this %
#define MOISTURE_THRESHOLD_HIGH 80  // Turn off relay when moisture is above this %

// SH1106 Display setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WiFiMulti object
ESP8266WiFiMulti wifiMulti;

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Display states
enum DisplayState {
  DISPLAY_INIT,
  DISPLAY_SCANNING,
  DISPLAY_CONNECTING_WIFI,
  DISPLAY_WIFI_INFO,
  DISPLAY_CONNECTING_FIREBASE,
  DISPLAY_SENSOR_DATA,
  DISPLAY_ERROR
};

// System data structure
struct SystemData {
  DisplayState currentState = DISPLAY_INIT;
  float temperature = 0.0;
  float humidity = 0.0;
  int soilMoistureRaw = 0;
  int soilMoisturePercent = 0;
  bool wifiConnected = false;
  bool firebaseConnected = false;
  bool relayState = false;
  String connectedSSID = "";
  String ipAddress = "";
  int rssi = 0;
  int networksFound = 0;
  String errorMessage = "";
  String statusMessage = "";
  unsigned long lastUpdate = 0;
  bool sensorDataValid = false;
};

SystemData systemData;

// Update intervals
unsigned long lastDisplayUpdate = 0;
unsigned long lastSensorRead = 0;
unsigned long lastFirebaseUpload = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 500;     // 500ms for smooth updates
const unsigned long SENSOR_READ_INTERVAL = 1000;      // 1 second for responsive sensor reading
const unsigned long FIREBASE_UPLOAD_INTERVAL = 5000;  // 5 seconds for Firebase uploads

void setup() {
  Serial.begin(74880);
  
  // Initialize relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Start with relay OFF (LOW = OFF for most relay modules)
  systemData.relayState = false;
  
  // Initialize display
  display.begin(0x3C, true); // Address 0x3C default, true for reset
  if(!display.getBuffer()) {
    Serial.println("SH1106 allocation failed");
    for(;;);
  }
  
  // Initial display
  systemData.currentState = DISPLAY_INIT;
  systemData.statusMessage = "System Starting...";
  updateDisplay();
  delay(2000);

  // Add multiple WiFi networks
  wifiMulti.addAP("Redmi Note 9 Pro Max", "TrinityXf");
  wifiMulti.addAP("AndroidAP_6194", "VinDiesel");
  wifiMulti.addAP("LOQ15IAX9", "12345678");

  // Scan for networks
  Serial.println("Scanning for available Wi-Fi networks...");
  systemData.currentState = DISPLAY_SCANNING;
  systemData.statusMessage = "Scanning WiFi Networks...";
  updateDisplay();
  
  int n = WiFi.scanNetworks();
  systemData.networksFound = n;
  
  if (n == 0) {
    Serial.println("No networks found.");
    systemData.currentState = DISPLAY_ERROR;
    systemData.errorMessage = "No WiFi Networks Found";
    systemData.statusMessage = "Check WiFi availability";
    updateDisplay();
    delay(3000);
  } else {
    Serial.println(String(n) + " networks found:");
    systemData.statusMessage = String(n) + " networks found";
    updateDisplay();
    delay(1000);
    
    for (int i = 0; i < n; ++i) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (RSSI: ");
      Serial.print(WiFi.RSSI(i));
      Serial.print(") ");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "Open" : "Encrypted");
      delay(10);
    }
  }

  // Connect to WiFi
  Serial.println("Connecting to Wi-Fi...");
  systemData.currentState = DISPLAY_CONNECTING_WIFI;
  systemData.statusMessage = "Connecting to WiFi...";
  updateDisplay();
  
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  
  // Update WiFi connection info
  systemData.wifiConnected = true;
  systemData.connectedSSID = WiFi.SSID();
  systemData.ipAddress = WiFi.localIP().toString();
  systemData.rssi = WiFi.RSSI();
  
  Serial.println(" connected!");
  Serial.print("Connected to: ");
  Serial.println(systemData.connectedSSID);
  Serial.print("IP Address: ");
  Serial.println(systemData.ipAddress);
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(systemData.rssi);
  Serial.println(" dBm");

  // Show WiFi info
  systemData.currentState = DISPLAY_WIFI_INFO;
  updateDisplay();
  delay(3000);

  // Configure Firebase
  Serial.println("Configuring Firebase...");
  systemData.currentState = DISPLAY_CONNECTING_FIREBASE;
  systemData.statusMessage = "Connecting to Firebase...";
  updateDisplay();
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase sign-in successful");
    systemData.firebaseConnected = true;
    systemData.statusMessage = "Firebase Connected!";
  } else {
    Serial.printf("Sign-up error: %s\n", config.signer.signupError.message.c_str());
    systemData.firebaseConnected = false;
    systemData.currentState = DISPLAY_ERROR;
    systemData.errorMessage = "Firebase Connection Failed";
    systemData.statusMessage = "Check API credentials";
  }
  
  updateDisplay();
  delay(2000);

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  dht.begin();
  
  // Switch to main sensor data display
  systemData.currentState = DISPLAY_SENSOR_DATA;
  systemData.lastUpdate = millis();
  
  Serial.println("System initialized with optimized timing:");
  Serial.println("- Sensor readings: Every 1 second (responsive)");
  Serial.println("- Display updates: Every 500ms (smooth)");
  Serial.println("- Firebase uploads: Every 5 seconds (efficient)");
  Serial.println("- Relay control: Immediate response");
  Serial.println("Relay ON: Soil moisture > 10% and < 30%");
  Serial.println("Relay OFF: Soil moisture <= 10% or >= 30%\n");
}

void loop() {
  // Check WiFi connection
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected, reconnecting...");
    systemData.wifiConnected = false;
    systemData.currentState = DISPLAY_CONNECTING_WIFI;
    systemData.statusMessage = "Reconnecting WiFi...";
    
    while (wifiMulti.run() != WL_CONNECTED) {
      updateDisplay();
      Serial.print(".");
      delay(1000);
    }
    
    // Update connection info
    systemData.wifiConnected = true;
    systemData.connectedSSID = WiFi.SSID();
    systemData.ipAddress = WiFi.localIP().toString();
    systemData.rssi = WiFi.RSSI();
    systemData.currentState = DISPLAY_SENSOR_DATA;
    
    Serial.println(" reconnected!");
    Serial.print("Connected to: ");
    Serial.println(systemData.connectedSSID);
  }

  // Read sensor data (every 1 second for responsive monitoring)
  if (millis() - lastSensorRead >= SENSOR_READ_INTERVAL) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int moistureRaw = analogRead(SOIL_MOISTURE_PIN);
    
    // Convert raw moisture reading to percentage
    int moisturePercent = map(moistureRaw, MOISTURE_DRY, MOISTURE_WET, 0, 100);
    moisturePercent = constrain(moisturePercent, 0, 100);

    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      systemData.sensorDataValid = false;
      systemData.currentState = DISPLAY_ERROR;
      systemData.errorMessage = "DHT Sensor Error";
      systemData.statusMessage = "Check DHT11 connection";
    } else {
      systemData.temperature = t;
      systemData.humidity = h;
      systemData.soilMoistureRaw = moistureRaw;
      systemData.soilMoisturePercent = moisturePercent;
      systemData.sensorDataValid = true;
      systemData.currentState = DISPLAY_SENSOR_DATA;
      systemData.lastUpdate = millis();
      
      // Control relay based on soil moisture (immediate response)
      controlRelay(moisturePercent);
      
      Serial.print("Sensors - Temp: ");
      Serial.print(t);
      Serial.print("°C, Humi: ");
      Serial.print(h);
      Serial.print("%, Soil: ");
      Serial.print(moisturePercent);
      Serial.print("% (");
      Serial.print(moistureRaw);
      Serial.print("), Relay: ");
      Serial.println(systemData.relayState ? "ON" : "OFF");
    }
    
    lastSensorRead = millis();
  }

  // Upload to Firebase (every 5 seconds to reduce cloud traffic)
  if (systemData.sensorDataValid && (millis() - lastFirebaseUpload >= FIREBASE_UPLOAD_INTERVAL)) {
    Serial.println("\n--- Firebase Upload ---");
    bool tempSuccess = false, humSuccess = false, moistSuccess = false, relaySuccess = false;
    
    if (Firebase.RTDB.setFloat(&fbdo, "/devices/nodemcu1/temperature", systemData.temperature)) {
      Serial.println("✓ Temperature uploaded: " + String(systemData.temperature));
      tempSuccess = true;
    } else {
      Serial.println("✗ Temperature upload error: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "/devices/nodemcu1/humidity", systemData.humidity)) {
      Serial.println("✓ Humidity uploaded: " + String(systemData.humidity));
      humSuccess = true;
    } else {
      Serial.println("✗ Humidity upload error: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, "/devices/nodemcu1/soilMoisture", systemData.soilMoisturePercent)) {
      Serial.println("✓ Soil Moisture uploaded: " + String(systemData.soilMoisturePercent) + "%");
      moistSuccess = true;
    } else {
      Serial.println("✗ Soil Moisture upload error: " + fbdo.errorReason());
    }

    // Upload relay status
    if (Firebase.RTDB.setBool(&fbdo, "/devices/nodemcu1/relayStatus", systemData.relayState)) {
      Serial.println("✓ Relay Status uploaded: " + String(systemData.relayState ? "ON" : "OFF"));
      relaySuccess = true;
    } else {
      Serial.println("✗ Relay Status upload error: " + fbdo.errorReason());
    }

    // Also upload raw moisture value for calibration purposes
    Firebase.RTDB.setInt(&fbdo, "/devices/nodemcu1/soilMoistureRaw", systemData.soilMoistureRaw);

    systemData.firebaseConnected = tempSuccess && humSuccess && moistSuccess;
    
    Serial.println("Firebase Status: " + String(systemData.firebaseConnected ? "Connected" : "Error"));
    Serial.println("------------------------\n");
    
    lastFirebaseUpload = millis();
  }

  // Update display
  if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    // Update WiFi info
    if (systemData.wifiConnected) {
      systemData.rssi = WiFi.RSSI();
    }
    
    updateDisplay();
    lastDisplayUpdate = millis();
  }

  delay(100);
}

// Function to control relay based on soil moisture
void controlRelay(int moisturePercent) {
  bool shouldRelayBeOn = (moisturePercent > MOISTURE_THRESHOLD_LOW && moisturePercent < MOISTURE_THRESHOLD_HIGH);
  
  if (shouldRelayBeOn && !systemData.relayState) {
    // Turn relay ON
    digitalWrite(RELAY_PIN, LOW);  // HIGH = ON for most relay modules
    systemData.relayState = true;
    Serial.println("RELAY TURNED ON - Soil moisture in irrigation range: " + String(moisturePercent) + "%");
  } else if (!shouldRelayBeOn && systemData.relayState) {
    // Turn relay OFF
    digitalWrite(RELAY_PIN, HIGH);   // LOW = OFF for most relay modules
    systemData.relayState = false;
    if (moisturePercent <= MOISTURE_THRESHOLD_LOW) {
      Serial.println("RELAY TURNED OFF - Soil too dry: " + String(moisturePercent) + "%");
    } else if (moisturePercent >= MOISTURE_THRESHOLD_HIGH) {
      Serial.println("RELAY TURNED OFF - Soil sufficiently moist: " + String(moisturePercent) + "%");
    }
  }
}

// Single modular display function
void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  
  switch (systemData.currentState) {
    case DISPLAY_INIT:
      renderInitScreen();
      break;
      
    case DISPLAY_SCANNING:
      renderStatusScreen("Scanning WiFi", systemData.statusMessage);
      break;
      
    case DISPLAY_CONNECTING_WIFI:
      renderStatusScreen("Connecting WiFi", systemData.statusMessage);
      break;
      
    case DISPLAY_WIFI_INFO:
      renderWiFiInfoScreen();
      break;
      
    case DISPLAY_CONNECTING_FIREBASE:
      renderStatusScreen("Firebase Setup", systemData.statusMessage);
      break;
      
    case DISPLAY_SENSOR_DATA:
      renderSensorDataScreen();
      break;
      
    case DISPLAY_ERROR:
      renderErrorScreen();
      break;
  }
  
  display.display();
}

void renderInitScreen() {
  display.setTextSize(2);
  display.setCursor(10, 15);
  display.println("AgroSmart");
  
  display.setTextSize(1);
  display.setCursor(25, 35);
  display.println("IoT System");
  
  display.setCursor(0, 50);
  display.println(systemData.statusMessage);
}

void renderStatusScreen(String title, String message) {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(title);
  
  display.drawLine(0, 12, 128, 12, SH110X_WHITE);
  
  display.setCursor(0, 20);
  display.println(message);
  
  // Show networks found if scanning
  if (systemData.currentState == DISPLAY_SCANNING && systemData.networksFound > 0) {
    display.setCursor(0, 35);
    display.print("Networks: ");
    display.println(systemData.networksFound);
  }
  
  // Add animation dots for connecting states
  if (systemData.currentState == DISPLAY_CONNECTING_WIFI || 
      systemData.currentState == DISPLAY_CONNECTING_FIREBASE) {
    int dots = (millis() / 500) % 4;
    display.setCursor(0, 45);
    display.print("Please wait");
    for (int i = 0; i < dots; i++) {
      display.print(".");
    }
  }
}

void renderWiFiInfoScreen() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("WiFi Connected!");
  
  display.drawLine(0, 12, 128, 12, SH110X_WHITE);
  
  display.setCursor(0, 16);
  display.print("SSID: ");
  display.println(systemData.connectedSSID);
  
  display.setCursor(0, 28);
  display.print("IP: ");
  display.println(systemData.ipAddress);
  
  display.setCursor(0, 40);
  display.print("Signal: ");
  display.print(systemData.rssi);
  display.println(" dBm");
  
  renderSignalBars(90, 40);
}

void renderSensorDataScreen() {
  // Header
  display.setTextSize(1);
  display.setCursor(15, 0);
  display.println("AgroSmart Monitor");
  display.drawLine(0, 12, 128, 12, SH110X_WHITE);
  
  // Sensor data
  if (systemData.sensorDataValid) {
    display.setCursor(0, 16);
    display.print("Temp: ");
    display.print(systemData.temperature, 1);
    display.println("C");
    
    display.setCursor(0, 26);
    display.print("Humi: ");
    display.print(systemData.humidity, 1);
    display.println("%");
    
    display.setCursor(0, 36);
    display.print("Soil: ");
    display.print(systemData.soilMoisturePercent);
    display.println("%");
    
    // Visual soil moisture indicator
    renderMoistureBar(70, 36);
    
    // Relay status
    display.setCursor(0, 46);
    display.print("Pump: ");
    display.print(systemData.relayState ? "ON" : "OFF");
    
    // Add relay indicator
    if (systemData.relayState) {
      display.fillCircle(120, 48, 3, SH110X_WHITE);
    } else {
      display.drawCircle(120, 48, 3, SH110X_WHITE);
    }
  } else {
    display.setCursor(0, 16);
    display.println("Sensor: No Data");
  }
  
  // Connection status
  display.setCursor(0, 56);
  display.print("WiFi:");
  if (systemData.wifiConnected) {
    display.print("OK");
    renderSignalBars(100, 54);
  } else {
    display.print("X");
  }
  
  display.setCursor(64, 56);
  display.print("FB:");
  display.print(systemData.firebaseConnected ? "OK" : "X");
}

void renderErrorScreen() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ERROR");
  
  display.drawLine(0, 12, 128, 12, SH110X_WHITE);
  
  display.setCursor(0, 20);
  display.println(systemData.errorMessage);
  
  display.setCursor(0, 35);
  display.println(systemData.statusMessage);
  
  // Blinking error indicator
  if ((millis() / 1000) % 2 == 0) {
    display.fillRect(115, 2, 10, 8, SH110X_WHITE);
  }
}

void renderSignalBars(int x, int y) {
  int bars = map(systemData.rssi, -100, -30, 0, 4);
  bars = constrain(bars, 0, 4);
  
  for (int i = 0; i < 4; i++) {
    int barHeight = 2 + i * 2;
    if (i < bars) {
      display.fillRect(x + i * 6, y + 8 - barHeight, 4, barHeight, SH110X_WHITE);
    } else {
      display.drawRect(x + i * 6, y + 8 - barHeight, 4, barHeight, SH110X_WHITE);
    }
  }
}

void renderMoistureBar(int x, int y) {
  // Draw moisture level bar
  int barWidth = 48;
  int barHeight = 6;
  int fillWidth = map(systemData.soilMoisturePercent, 0, 100, 0, barWidth);
  
  // Draw border
  display.drawRect(x, y, barWidth, barHeight, SH110X_WHITE);
  
  // Fill based on moisture level
  if (fillWidth > 0) {
    display.fillRect(x + 1, y + 1, fillWidth - 2, barHeight - 2, SH110X_WHITE);
  }
  
  // Add threshold markers
  int lowThresholdX = map(MOISTURE_THRESHOLD_LOW, 0, 100, 0, barWidth);
  int highThresholdX = map(MOISTURE_THRESHOLD_HIGH, 0, 100, 0, barWidth);
  
  // Mark the irrigation zone (10-30%)
  display.drawLine(x + lowThresholdX, y - 1, x + lowThresholdX, y + barHeight, SH110X_WHITE);
  display.drawLine(x + highThresholdX, y - 1, x + highThresholdX, y + barHeight, SH110X_WHITE);
}
