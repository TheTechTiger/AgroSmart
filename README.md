
# 🌱 Smart Agriculture for Efficient Cultivation in Hilly Regions  
**SIH 25062 – Team Project**

## 📖 Overview
This project implements an **IoT‑based Smart Agriculture system** designed to optimize cultivation in hilly regions. Using **NodeMCU (ESP8266)**, the system monitors environmental conditions and automates irrigation. Data is uploaded to **Firebase Realtime Database**, enabling farmers to view live conditions and control sprinklers remotely via a mobile app.

The solution ensures efficient water usage, reduces manual intervention, and supports sustainable farming practices in challenging terrains.

---

## ⚙️ Features
- **Weather monitoring**: Real‑time temperature and humidity readings using DHT11 sensor.  
- **Soil moisture sensing**: Analog soil moisture sensor tracks soil hydration levels.  
- **Automated irrigation**: Relay‑controlled water pump activates when soil moisture drops below threshold.  
- **Cloud integration**: Sensor data uploaded to Firebase for remote access.  
- **Mobile app control**: Farmers can monitor conditions and manually control sprinklers via companion app.  
- **OLED display feedback**: SH1106 OLED shows live sensor data, WiFi/Firebase status, and pump state.  
- **Multi‑WiFi support**: ESP8266WiFiMulti ensures reliable connectivity across multiple networks.  

---

## 🛠️ Hardware Components
- **NodeMCU ESP8266** – Core microcontroller with WiFi.  
- **DHT11 sensor** – Measures temperature and humidity.  
- **Soil moisture sensor** – Detects soil hydration levels.  
- **Relay module** – Controls water pump/sprinkler system.  
- **SH1106 OLED display** – Displays system status and sensor data.  
- **Water pump & sprinklers** – Automated irrigation hardware.  

---

## 🧑‍💻 Software & Libraries
- **ESP8266WiFi / ESP8266WiFiMulti** – WiFi connectivity.  
- **Firebase ESP Client** – Firebase Realtime Database integration.  
- **DHT library** – Sensor data acquisition.  
- **Adafruit GFX & SH110X** – OLED display rendering.  

---

## 🔄 System Workflow
1. **Sensor data collection**: DHT11 and soil moisture sensor read values every second.  
2. **Relay control**: Pump turns ON when soil moisture < threshold, OFF when moisture > threshold.  
3. **Data upload**: Temperature, humidity, soil moisture, and pump status sent to Firebase every 5 seconds.  
4. **Mobile app interface**: Farmers view live data and control sprinklers remotely.  
5. **OLED feedback**: Local display shows sensor readings, WiFi/Firebase status, and pump activity.  

---

## 📊 Firebase Data Structure
```
/devices/nodemcu1/
    ├── temperature: <float>
    ├── humidity: <float>
    ├── soilMoisture: <int>
    ├── soilMoistureRaw: <int>
    ├── relayStatus: <bool>
```

---

## 🚀 Setup Instructions
1. **Hardware assembly**: Connect DHT11, soil moisture sensor, relay, and OLED to NodeMCU.  
2. **Firebase setup**: Create a Firebase project, enable Realtime Database, and obtain API key & database URL.  
3. **Code upload**: Flash the provided Arduino sketch to NodeMCU via Arduino IDE.  
4. **WiFi configuration**: Update WiFi credentials in code (`wifiMulti.addAP`).  
5. **App integration**: Connect mobile app to Firebase for live monitoring and control.  

---

## 📌 Project Resources
- 📱 [App Release (GitHub)](https://github.com/TheTechTiger/AgroSmart/releases/tag/v1.0.0)  
- 🎥 [Demo Video (YouTube)](https://www.youtube.com/watch?v=UTdf7FC17ng)  
- 🌐 [Landing Page](https://thetechtiger.github.io/AgroSmart/)  

---

## 🖼️ Project Images

### 📱 Android App Screenshot  
<p align="center">
  <img src="https://raw.githubusercontent.com/TheTechTiger/AgroSmart/refs/heads/main/images/app_SS1.jpg" alt="Android App Screenshot" width="400"/>
</p>

### 🔌 Connection Diagram (Made using Fritzing)  
<p align="center">
  <img src="https://raw.githubusercontent.com/TheTechTiger/AgroSmart/refs/heads/main/images/CircuitDiagram.png" alt="Connection Diagram" width="500"/>
</p>

### 🖥️ OLED Screen Display Sample  
<p align="center">
  <img src="https://raw.githubusercontent.com/TheTechTiger/AgroSmart/refs/heads/main/images/OLEDScreen.jpg" alt="OLED Screen Display" width="400"/>
</p>

### ⚙️ PCB Assembly (Built Project)  
<p align="center">
  <img src="https://raw.githubusercontent.com/TheTechTiger/AgroSmart/refs/heads/main/images/PCB_Assembly.png" alt="PCB Assembly" width="500"/>
</p>

---

## 🌍 Impact
This system addresses **water scarcity and manual labor challenges** in hilly agriculture by:  
- **Automating irrigation** based on soil conditions.  
- **Providing real‑time insights** into weather and soil health.  
- **Enabling remote control** of sprinklers via mobile app.  
- **Supporting sustainable farming** practices in difficult terrains.  
