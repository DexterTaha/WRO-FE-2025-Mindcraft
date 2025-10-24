<a name="top"></a>

# 🤖 WRO-FE 2025 Mindcraft Robot System Documentation

![Hardware Architecture](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/schemes/Hardware%20Architecture.png)

> Comprehensive documentation for the WRO Future Engineers 2025 robot designed by **Team Mindcraft**. This robot integrates a Raspberry Pi 4B and ESP 32 with vision processing, sensor fusion, and custom power management to achieve autonomous navigation and obstacle avoidance.

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 📌 Table of Contents

> [!TIP]
> Click the arrow below 👇 to expand the **Table of Contents**.  
> Every item is a clickable link to a section in this README.

<details>
<summary><b>📂 Table of Contents</b></summary>

1. [🧩 Hardware Overview](#hardware-overview)  
2. [🗺️ Wiring Diagram](#wiring-diagram)  
3. [🛠️ About Fritzing](#about-fritzing)  
4. [🔋 Power Management](#power-management)  
5. [📡 Communication Architecture](#communication-architecture)  
6. [🧠 Component Responsibilities](#component-responsibilities)  
7. [🔁 System Data Flow](#system-data-flow)  
8. [📌 Pin Mapping (Pico)](#pin-mapping-pico)  
9. [🖼️ Media](#media)  
10. [✅ Status](#status)

</details>

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 🧩 Hardware Overview <a id="hardware-overview"></a>

| Code      | Component                   | Role                                            |
| --------- | --------------------------- | ----------------------------------------------- |
| `0x00`    | **Raspberry Pi 4B**         | Image processing, LIDAR handling, high-level AI |
| `0x01`    | **ESP 32**                  | Low-level control (motors, steering, sensors)   |
| `0x02`    | **RP LIDAR C1**             | Long-range obstacle detection (UART)            |
| `0x03`    | **BNO055 IMU**              | Orientation and motion sensing (I2C)            |
| `0x04`    | **PiCamera 3 Wide**         | Real-time image input (CSI to Pi)               |
| `0x05`    | **DC Motors + Encoders**    | Robot propulsion and feedback                   |
| `0x06`    | **Wheels**                  | Differential drive system                       |
| `0x07`    | **TB1266FNG Driver**        | Controls brushed DC motors                      |
| `0x08`    | **Servo Motor (180°)**      | Front wheel steering system                     |
| `0x09`    | **LiPo Battery 3S (11.1V)** | Main power supply                               |
| `0x10`    | **IMAX B6AC Charger**       | Recharges LiPo safely                           |
| `0x11–13` | **Voltage Regulator**        | Custom voltage regulation                       |
| `0x14`    | **Tactile Button**          | Manual start/stop input                         |
| `0x15`    | **Buzzer**                  | System feedback (tones/melody)                  |
| `0x19`    | **RGB LED**                 | Status indication                               |

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 🗺️ Wiring Diagram <a id="wiring-diagram"></a>

A visual schematic is available in the repo:

![Software System Architecture](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/schemes/Software%20System%20Architecture.png)
![View Circuit Image](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/schemes/circuit_bb.png)
📌 **High-resolution version** of the full circuit:
🔗 [View Circuit Image](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/schemes/circuit_bb.png)

📥 **Download the Fritzing (.fzz) project file:**
🔗 [Download circuit.fzz from GitHub](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/schemes/circuit.fzz)

👉 **Local Download (.fzz)**: <button style="padding: 10px 16px; font-size: 16px; background-color: #28a745; color: white; border: none; border-radius: 5px; cursor: pointer;" onclick="window.location.href='/mnt/data/dbd61b93-c1bc-48fd-b559-80db10cae37b.png'">Download Circuit File (.fzz)</button>

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 🛠️ About Fritzing <a id="about-fritzing"></a>

**Fritzing** is an open-source hardware design tool that makes it easy to create electronics schematics and PCB layouts using a visual breadboard-style interface. It's ideal for makers, students, and robotics teams.

🔧 **Download Fritzing**:
👉 [https://fritzing.org/download](https://fritzing.org/download)

> [!NOTE]
> *Available for Windows, macOS, and Linux.*


Fritzing helps document projects like this one by providing a clear and modifiable graphical circuit overview for prototyping, testing, and presentation.

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 🔋 Power Management <a id="power-management"></a>

### ➤ LiPo Battery (3S, 11.1V, 2200mAh)

* **Source** for all components
* Split via regulators for subsystem needs

### ➤ Custom Regulator Circuit

| Regulator      | Output Voltage | Connected Components   |
| -------------- | -------------- | ---------------------- |
| **First one**  | 5V             | ESP 32, BNO055, PI 4B  |
| **Second one** | 6V             | Servo motor            |
| **Third one**  | 9V             | Motor driver TB1266FNG |


> [!NOTE]
> Proper **ground plane** shared across devices

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 📡 Communication Architecture <a id="communication-architecture"></a>

| Protocol | Devices                            | Direction             |
| -------- | ---------------------------------- | --------------------- |
| **I2C**  | PI 4B ↔ BNO055                     | Input                 |
| **I2C**  | PI 4B ↔ ESP 32                        | Pi sends commands     |
| **UART** | PI 4B ↔ LIDAR                         | Serial stream (TX/RX) |
| **PWM**  | ESP 32 → Servo Motor               | Output only           |
| **PWM**  | ESP 32 → Servo + Motor Driver      | Output                |
| **GPIO** | ESP 32 ← Button, ESP 32 → Buzzer/LED | Input/Output          |

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 🧠 Component Responsibilities <a id="component-responsibilities"></a>

### 🟪 Raspberry Pi 4B

* AI & CV (OpenCV + camera)
* LIDAR data parsing
* Sends steering/speed commands to ESP 32

### 🟦 ESP 32

* Interprets I2C commands from Pi
* Handles:

  * PWM motor driving
  * Steering servo
  * RGB LED status
  * Button for manual control

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 🔁 System Data Flow <a id="system-data-flow"></a>

```plaintext
[Camera + LIDAR + IMU]
       ↓
[Raspberry Pi 4B]  ←→  [ESP 32]
       ↓                     ↓
[AI Decision]         [PWM + GPIO Control]
       ↓                     ↓
      MOVEMENT ←────────────┘
```

* Raspberry Pi handles vision & sensing.
* ESP 32 executes precise low-level actions.

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 📌 Pin Mapping (Pico) <a id="pin-mapping-pico"></a>

| Component / Function        | GPIO Pin |
| --------------------------- | :------: |
| PWB (Motor or Power Switch) |    14    |
| BI2 (Motor Input 2)         |    12    |
| BI1 (Motor Input 1)         |    26    |
| STBY (Motor Driver Standby) |    25    |
| SERVO PWM Signal            |    27    |
| ENCODER B Phase             |    32    |
| ENCODER A Phase             |    33    |
| PUSH BUTTON                 |    19    |
| BUZZER                      |    4     |

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 🖼️ Media <a id="media"></a>

![Robot Photo](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/870c603c6b6652d93f061301c1c845767c204ded/v-photos/real%20images/top-view.JPG)

> Robot front view showing camera, LIDAR, custom regulators, and modular stacking.

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## ✅ Status <a id="status"></a>

* ✅ Fully integrated sensors and power system
* ✅ Custom MicroPython code on ESP 32 for modular control
* ✅ Camera + LIDAR interfaced and tested

<p align="right">
  <a href="#top">
    <img src="https://img.shields.io/badge/-BACK_TO_TOP-151515?style=flat-square" alt="Back to top" />
  </a>
</p>
