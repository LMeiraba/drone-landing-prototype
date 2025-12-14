# 🛬 ESP32 Autonomous Drone Landing Pad (Prototype)

## 📌 Overview
This project demonstrates a **prototype autonomous landing pad for drones** using an **ESP32 microcontroller**.  
The system detects an approaching drone using distance sensors and provides **visual and audio guidance** to assist in safe landing.  
A **real-time web dashboard** hosted on the ESP32 allows live monitoring on a mobile device without page refresh.

This project is intended for **educational and demonstration purposes** and focuses on low-cost hardware and simple implementation.

---

## 🎯 Features
- ESP32-based control system  
- Ultrasonic and IR sensor-based distance detection  
- 60-LED WS2812B programmable LED ring for visual guidance  
- Buzzer indication on successful landing  
- Real-time web dashboard (HTML, CSS, JavaScript)  
- Mobile-friendly UI with live updates  
- Lightweight 3D-printed helipad base  

---

## 🧰 Hardware Components
- ESP32 Development Board  
- Ultrasonic Sensor (HC-SR04)  
- IR Distance Sensor  
- WS2812B 60-LED Ring  
- Active Buzzer  
- 5V Power Supply / Power Bank  
- Connecting Wires  
- 3D-Printed Helipad Base (PLA)  

---

## 🔌 System Working
1. The ultrasonic and IR sensors continuously measure the distance of the drone from the landing pad.  
2. The ESP32 selects the minimum distance for improved reliability.  
3. Based on distance:
   - **Idle**: LEDs off  
   - **Approaching**: Rotating orange LED animation  
   - **Landing**: Green LEDs with buzzer alert  
4. Sensor data is served as a JSON API.  
5. The web dashboard fetches data periodically and updates the UI in real time.

---

## 🌐 Web Dashboard
- Hosted directly on ESP32  
- Live updates using JavaScript `fetch()`  
- No page refresh required  
- Displays:
  - System status  
  - Ultrasonic distance  
  - IR distance  
  - Final selected distance  

---

## 🖨 3D Printed Helipad
- Material: PLA  
- Diameter: ~26 cm  
- Thickness: 5 mm  
- Designed using OpenSCAD  
- Lightweight and easy to fabricate  

---

## 🛠 Software & Libraries
- Arduino IDE  
- ESP32 Board Package  
- Adafruit NeoPixel Library  

---

## ⚠️ Disclaimer
This project is a **prototype** and is **not intended for real-world drone landing applications**.  
It is designed purely for learning, experimentation, and academic demonstrations.

---

## 🚀 Future Improvements
- Camera-based precision landing  
- Communication with drone flight controller  
- Battery voltage monitoring  
- Weather-resistant enclosure  
- GPS-assisted landing support  

---

## 📄 License
This project is open-source and intended for educational use.
