# Smart Load Management and Backup Protection System

**Department of Electrical & Electronic Engineering (EEE)**  
*Rajshahi University of Engineering & Technology (RUET), Bangladesh*[cite: 1]  
**Authors**: Md. Al Amin Islam & Team[cite: 1]

📄 **[View/Download Full Project Report (PDF)](report.pdf)**[cite: 1]

---

## 📌 Abstract
This project presents a low-cost, microcontroller-based battery monitoring and protection system built around the Arduino Uno platform[cite: 1]. The system continuously measures voltage and current delivered by a 12V, 7.5 Ah Sealed Lead-Acid (SLA) battery using an INA219 digital sensor[cite: 1]. When current exceeds a preset safety threshold, the system triggers a visual and audible alarm, displays a warning on a 16x2 LCD, and opens a relay to isolate the load and protect the battery[cite: 1].

---

## ✨ Key Features
- **Real-Time Sensing**: High-accuracy voltage and current acquisition via INA219 sensor over I2C[cite: 1].
- **Automatic Overload Isolation**: Disconnects load via relay when fault conditions occur[cite: 1].
- **Dual Visual Indicators**: 16x2 LCD for live numerical telemetry and a 10-LED bar-graph for status level[cite: 1].
- **Audible Warning**: Piezoelectric buzzer alerts during overcurrent conditions[cite: 1].
- **Manual Override**: Physical rocker switches and push buttons for individual channel switching and mode selection[cite: 1].

---

## 🛠️ Hardware Components

| Component | Quantity | Role / Function |
| :--- | :---: | :--- |
| **Arduino Uno (ATmega328P)** | 1 | Main controller processing sensor logic and driving outputs[cite: 1] |
| **INA219 Sensor** | 1 | I2C digital current and voltage monitoring module[cite: 1] |
| **4-Channel 5V Relay Module** | 1 | Switches DC load lines and provides emergency load shedding[cite: 1] |
| **16x2 Character LCD** | 1 | Live status display for voltage, current, and warnings[cite: 1] |
| **LED Bar-Graph** | 10 LEDs | Visual scale for load/battery level[cite: 1] |
| **Optocoupler (PC817)** | 1 | Grid presence detection for power source switching[cite: 1] |
| **12V, 7.5 Ah SLA Battery** | 1 | Main DC supply under test[cite: 1] |
| **Rocker Switches (SPST)** | 3 | Manual ON/OFF channel control switches[cite: 1] |

---

## ⚡ System Operation & Results

### Normal Operation vs Fault Detection
- **Normal State**: The LCD outputs live parameters (e.g., `V: 11.8V | C: 0.638A`) under steady load[cite: 1].
- **Fault State**: If current exceeds the limit, the system displays `WARNING! OVERCURRENT`, activates the alarm, and de-energizes the relay channel to isolate the load[cite: 1].

| Prototype Hardware Setup | Overcurrent Alarm Triggered |
| :---: | :---: |
| ![Hardware Setup](circuit_setup.jpg) | ![Warning Screen](warning_display.jpg) |

---

## 🚀 Future Improvements
- Migration from breadboard prototyping to a dedicated PCB layout[cite: 1].
- Implementation of Wi-Fi / Bluetooth (ESP32/HC-05) for mobile app monitoring and control[cite: 1].
- Implementation of deep-discharge and over-voltage protection algorithms[cite: 1].
