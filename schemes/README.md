# 🤖 WRO-FE 2025 Mindcraft Robot System Documentation

> Comprehensive documentation for the WRO Future Engineers 2025 robot designed by **Team Mindcraft**. This robot integrates a Raspberry Pi 4B and Raspberry Pi Pico with vision processing, sensor fusion, and custom power management to achieve autonomous navigation and obstacle avoidance.

---

## 📌 Table of Contents

* [Hardware Overview](#hardware-overview)
* [Wiring Diagram](#wiring-diagram)
* [About Fritzing](#about-fritzing)
* [Download Fritzing](#download-fritzing)
* [Power Management](#power-management)
* [Communication Architecture](#communication-architecture)
* [Component Responsibilities](#component-responsibilities)
* [System Data Flow](#system-data-flow)
* [Pin Mapping](#pin-mapping)
* [Media](#media)

---

## 🧩 Hardware Overview

| Code      | Component                   | Role                                            |
| --------- | --------------------------- | ----------------------------------------------- |
| `0x00`    | **Raspberry Pi 4B**         | Image processing, LIDAR handling, high-level AI |
| `0x01`    | **Raspberry Pi Pico**       | Low-level control (motors, steering, sensors)   |
| `0x02`    | **RP LIDAR C1**             | Long-range obstacle detection (UART)            |
| `0x03`    | **BNO055 IMU**              | Orientation and motion sensing (I2C)            |
| `0x04`    | **PiCamera 3 Wide**         | Real-time image input (CSI to Pi)               |
| `0x05`    | **DC Motors + Encoders**    | Robot propulsion and feedback                   |
| `0x06`    | **Wheels**                  | Differential drive system                       |
| `0x07`    | **TB1266FNG Driver**        | Controls brushed DC motors                      |
| `0x08`    | **Servo Motor (180°)**      | Front wheel steering system                     |
| `0x09`    | **LiPo Battery 3S (11.1V)** | Main power supply                               |
| `0x10`    | **IMAX B6AC Charger**       | Recharges LiPo safely                           |
| `0x11–13` | **7805, 7806, 7809**        | Custom voltage regulation                       |
| `0x14`    | **Tactile Button**          | Manual start/stop input                         |
| `0x15`    | **Buzzer**                  | System feedback (tones/melody)                  |
| `0x19`    | **RGB LED**                 | Status indication                               |

---

## 🗺️ Wiring Diagram

A visual schematic is available in the repo:

![Robot Circuit Diagram](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/schemes/schemes.png)
![View Circuit Image](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/schemes/Circuit.png)
📌 **High-resolution version** of the full circuit:
🔗 [View Circuit Image](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/schemes/Circuit.png)

📥 **Download the Fritzing (.fzz) project file:**
🔗 [Download circuit.fzz from GitHub](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/schemes/circuit.fzz)

👉 **Local Download (.fzz)**: <button style="padding: 10px 16px; font-size: 16px; background-color: #28a745; color: white; border: none; border-radius: 5px; cursor: pointer;" onclick="window.location.href='/mnt/data/dbd61b93-c1bc-48fd-b559-80db10cae37b.png'">Download Circuit File (.fzz)</button>

---

## 🛠️ About Fritzing

**Fritzing** is an open-source hardware design tool that makes it easy to create electronics schematics and PCB layouts using a visual breadboard-style interface. It's ideal for makers, students, and robotics teams.

🔧 **Download Fritzing**:
👉 [https://fritzing.org/download](https://fritzing.org/download)

*Available for Windows, macOS, and Linux.*

Fritzing helps document projects like this one by providing a clear and modifiable graphical circuit overview for prototyping, testing, and presentation.

---

## 🔋 Power Management

### ➤ LiPo Battery (3S, 11.1V, 2200mAh)

* **Source** for all components
* Split via regulators for subsystem needs

### ➤ Custom Regulator Circuit

| Regulator | Output Voltage | Connected Components   |
| --------- | -------------- | ---------------------- |
| **7805**  | 5V             | Pico, BNO055, display  |
| **7806**  | 6V             | Servo motor            |
| **7809**  | 9V             | Motor driver TB1266FNG |

* **100 µF Capacitors (6x total)**: Stabilization at input/output
* Proper **ground plane** shared across devices

---

## 📡 Communication Architecture

| Protocol | Devices                          | Direction             |
| -------- | -------------------------------- | --------------------- |
| **I2C**  | Pico ↔ BNO055                    | Bidirectional         |
| **I2C**  | Pi ↔ Pico                        | Pi sends commands     |
| **UART** | Pi ↔ LIDAR                       | Serial stream (TX/RX) |
| **SPI**  | Pico → ST7789 Display            | Output only           |
| **PWM**  | Pico → Servo + Motor Driver      | Output                |
| **GPIO** | Pico ← Button, Pico → Buzzer/LED | Input/Output          |

---

## 🧠 Component Responsibilities

### 🟪 Raspberry Pi 4B

* AI & CV (OpenCV + camera)
* LIDAR data parsing
* Sends steering/speed commands to Pico
* GUI or ROS if needed

### 🟦 Raspberry Pi Pico

* Interprets I2C commands from Pi
* Handles:

  * PWM motor driving
  * Steering servo
  * RGB LED status
  * Button for manual control
  * Display output
* Feedback via serial or display

---

## 🔁 System Data Flow

```plaintext
[Camera + LIDAR]
       ↓
[Raspberry Pi 4B]  ←→  [Raspberry Pi Pico]
       ↓                     ↓
[AI Decision]         [PWM + GPIO Control]
       ↓                     ↓
      MOVEMENT ←────────────┘
```

* Raspberry Pi handles vision & sensing.
* Pico executes precise low-level actions.

---

## 📌 Pin Mapping (Pico)

| Pin     | Signal      | Connected Device |
| ------- | ----------- | ---------------- |
| GP0     | I2C0 SDA    | BNO055           |
| GP1     | I2C0 SCL    | BNO055           |
| GP2     | PWM         | Left Motor       |
| GP3     | PWM         | Right Motor      |
| GP4     | PWM         | Servo Motor      |
| GP5     | GPIO        | RGB LED Red      |
| GP6     | GPIO        | RGB LED Green    |
| GP7     | GPIO        | RGB LED Blue     |
| GP8     | Digital Out | Buzzer           |
| GP9     | Digital In  | Button           |
| GP10-13 | SPI         | ST7789 Display   |

---

## 🖼️ Media

![Robot Photo](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/870c603c6b6652d93f061301c1c845767c204ded/v-photos/real%20images/top-view.JPG)

> Robot front view showing camera, LIDAR, custom regulators, and modular stacking.

---

## ✅ Status

* ✅ Fully integrated sensors and power system
* ✅ Custom MicroPython code on Pico for modular control
* ✅ Camera + LIDAR interfaced and tested
* 🧪 Final tuning of motion control in progress

---
