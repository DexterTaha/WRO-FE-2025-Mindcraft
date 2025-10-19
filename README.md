<a name="top"></a>

  
# 🤖 MINDCRAFT WRO Future Engineers team

📌 This repository documents the ongoing research, design, and development of our self-driving robotic vehicle, engineered and programmed by Salmane Derdeb, Taha Taidi Laamiri, and Rayane Ghacha for the World Robot Olympiad (WRO) 2025 – Future Engineers Division. The project represents the fusion of embedded systems (ESP32 and Raspberry Pi), perception technologies (computer vision and LiDAR-based mapping), and intelligent control logic implemented in Python and C++. It showcases our continuous effort to build a robust, fully autonomous system capable of performing complex navigation and task-solving challenges with precision and adaptability.

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

<p align="center">
  <img alt="Project logo" src="https://img.shields.io/badge/Project-WRO--FE--2025--Mindcraft-4b5563?style=for-the-badge&logo=robot" />
</p>

[![GitHub Stars](https://img.shields.io/github/stars/DexterTaha/WRO-FE-2025-Mindcraft.svg)](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/stargazers)
![OS](https://img.shields.io/badge/OS-Linux-red?style=flat&logo=linux)
[![Made with Python](https://img.shields.io/badge/Made%20with-Python%203.8%2B-1f425f.svg?logo=python)](https://www.python.org/)
[![C++](https://img.shields.io/badge/C%2B%2B-ISO%20C%2B%2B-00599C?logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![Top language](https://img.shields.io/github/languages/top/DexterTaha/WRO-FE-2025-Mindcraft)](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft)
[![Repo size](https://img.shields.io/github/repo-size/DexterTaha/WRO-FE-2025-Mindcraft)](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft)
[![Build Status](https://img.shields.io/github/actions/workflow/status/DexterTaha/WRO-FE-2025-Mindcraft/ci.yml?branch=main)](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/actions)
[![Docs](https://img.shields.io/badge/Docs-MkDocs-blue?style=flat)](https://dextertaha.github.io/WRO-FE-2025-Mindcraft/)
[![License](https://img.shields.io/github/license/DexterTaha/WRO-FE-2025-Mindcraft)](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/LICENSE)
[![Open issues](https://img.shields.io/github/issues/DexterTaha/WRO-FE-2025-Mindcraft)](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/issues)
[![Contributors](https://img.shields.io/github/contributors/DexterTaha/WRO-FE-2025-Mindcraft)](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/graphs/contributors)
[![Maintained](https://img.shields.io/badge/Maintained-yes-green.svg)](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft)

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)


## Table of Contents
> [!TIP]
> Click the arrow below 👇 to expand the **Table of Contents**. All items are clickable links to folders or README.md files.

<details>
<summary>Table of Contents</summary>

1. [Top-level README](./README.md)

2. [Models (3D files & parts)](./Models/)  
   - [Models README](./Models/README.md)  
   - [Parts folder](./Models/Parts/)  
     - [0x00 - Robot Base](./Models/Parts/0x00-%20Robot%20Base/)  
     - [0x01 - Second Layer](./Models/Parts/0x01-%20Second%20Layer/)  
     - [0x02 - Wheel Support](./Models/Parts/0x02-%20Wheel%20Support/)  
     - [0x03 - Gear](./Models/Parts/0x03-%20Gear/)  
     - [0x04 - Berring Support Right](./Models/Parts/0x04-%20%20Berring%20Support%20Right/)  
     - [0x05 - stand off 2mm](./Models/Parts/0x05-%20stand%20off%202mm/)  
     - [0x06 - Short Shaft](./Models/Parts/0x06-%20Short%20Shaft/)  
     - [0x07 - Long Shaft](./Models/Parts/0x07-%20Long%20Shaft/)  
     - [0x08 - Axle Clamp](./Models/Parts/0x08-%20Axle%20Clamp/)  
     - [0x09 - stand off 1mm](./Models/Parts/0x09-%20stand%20off%201mm/)  
     - [0x10 - Big Gear](./Models/Parts/0x10-%20Big%20Gear/)  
     - [0x11 - Front Support Camera](./Models/Parts/0x11-%20Front%20Support%20Camera/)  
     - [0x12 - Berring Support Left](./Models/Parts/0x12-%20Berring%20Support%20Left/)  
     - [0x13 - stando ff3mm](./Models/Parts/0x13-%20stando%20ff3mm/)  
     - [0x14 - Back Support Camera](./Models/Parts/0x14-%20Back%20Support%20Camera/)  
     - [0x15 - Steering Conector](./Models/Parts/0x15-%20Steering%20Conector/)  
     - [0x16 - scwer shaft](./Models/Parts/0x16-%20scwer%20shaft/)  
     - [0x17 - Steering Rack](./Models/Parts/0x17-%20Steering%20Rack/)

3. [other (BOM & components)](./other/)  
   - [other README](./other/README.md)  
   - [BOM (Bill Of Materials) folder](./other/BOM(Bill%20Of%20Materials)/)  
     - [BOM README](./other/BOM(Bill%20Of%20Materials)/README.md)  
   - [component Details folder](./other/component%20Details/)  
     - [component Details README](./other/component%20Details/README.md)  
     - Components (subfolders) — each folder is clickable:
       - [0x00 - Raspberry Pi 4B](./other/component%20Details/0x00-Raspberry%20Pi%204B/)  
       - [0x01 - Raspberry pi pico](./other/component%20Details/0x01-Raspberry%20pi%20pico/)  
       - [0x02 - LIDAR](./other/component%20Details/0x02-LIDAR/)  
       - [0x03 - GYROSCOPE Sensor BMO055](./other/component%20Details/0x03-GYROSCOPE%20Sensor%20BMO055/)  
       - [0x04 - PiCamera 3 wide](./other/component%20Details/0x04-PiCamera%203%20wide/)  
       - [0x05 - DC Brushed Motor with Encoder](./other/component%20Details/0x05-DC%20Brushed%20Motor%20with%20Encoder/)  
       - [0x06 - Wheels](./other/component%20Details/0x06-Wheels/)  
       - [0x07 - TB1266FNG Motor Driver](./other/component%20Details/0x07-TB1266FNG%20Motor%20Driver/)  
       - [0x08 - Servo motor Metal Gear Box 180°](./other/component%20Details/0x08-Servo%20motor%20Metal%20Gear%20Box%20180%C2%B0/)  
       - [0x09 - Lipo 3S 2200mah 11.1V 50C](./other/component%20Details/0x09-Lipo%203S%202200mah%2011.1V%2050C/)  
       - [0x10 - IMAX B6AC V2](./other/component%20Details/0x10-IMAX%20B6AC%20V2/)  
       - [0x11 - 7806 Transistor](./other/component%20Details/0x11-7806%20Transistor/)  
       - [0x12 - 7805 Transistor](./other/component%20Details/0x12-7805%20Transistor/)  
       - [0x13 - 7809 Transistor](./other/component%20Details/0x13-7809%20Transistor/)  
       - [0x14 - Push button](./other/component%20Details/0x14-Push%20button/)  
       - [0x15 - Buzzer Alarm Batterie Lipo](./other/component%20Details/0x15-Buzzer%20Alarm%20Batterie%20Lipo/)  
       - [0x16 - Servo Tester](./other/component%20Details/0x16-Servo%20Tester/)  
       - [0x17 - Servobras](./other/component%20Details/0x17-Servobras/)  
       - [0x18 - SD card 64GB](./other/component%20Details/0x18-SD%20card%2064GB/)  
       - [0x19 - RBG Led](./other/component%20Details/0x19-RBG%20Led/)  
       - [0x20 - Switch](./other/component%20Details/0x20-Switch/)

4. [schemes (circuit files)](./schemes/)  
   - [schemes README](./schemes/README.md)

5. [src ( source code )](./src%20(%20source%20code%20)/)  
   - [PI4B folder](./src%20(%20source%20code%20)/PI4B/)  
   - [PICO folder](./src%20(%20source%20code%20)/PICO/)

6. [t-photos (team photos)](./t-photos/)  
   - [t-photos README](./t-photos/README.md)

7. [v-photos (vehicle photos)](./v-photos/)  
   - [v-photos README](./v-photos/README.md)

8. [videos](./videos/)  
   - [videos README](./videos/README.md)  
   - Subfolders:
     - [Obstacle Challenge](./videos/Obstacle%20Challenge/)  
     - [Open Challenge](./videos/Open%20Challenge/)  
     - [Project Presentation](./videos/Project%20Presentation/)  
     - [Robot Parts Printing](./videos/Robot%20Parts%20Printing/)  
     - [Sensors Test](./videos/Sensors%20Test/)

</details>

> [!NOTE]
>
> The folders, images and supplementary docs in this repository are provided as documentation and for reference only. They are not required to recreate the robot hardware, nor are they mandatory for judging. The files and media help explain our design and process but do not imply required parts or exact build steps for competition entry.

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 🌟 Highlights

- **Innovative mobility system:** Built on a custom-designed chassis engineered for balance, rigidity, and modularity. The vehicle features a **rear-wheel differential drive system** for propulsion and a **front-wheel Ackermann steering mechanism** controlled by a high-torque servo motor. DC motors is driven by a **TB6612FNG motor driver**, with encoder feedback for precise closed-loop speed and position control. A **buzzer** provides real-time debugging feedback, and a **push button** is used for controlled start and system activation.  

- **Optimized power and sensing setup:** Powered by a **12 V Li-ion 3C 2.2A battery system**, the design separates power lines for motors and logic circuits to ensure stability and reduce interference. The robot integrates an **RPLIDAR C1** for mapping and obstacle detection, a **Raspberry Pi Camera 3 Wide** for real-time vision color detection, and a **BNO055 IMU** for orientation sensing. All sensors are carefully placed for accurate perception and reliable navigation.  

- **Autonomous navigation and perception:** Combines **computer vision** and **LiDAR-based mapping** to perform environment recognition, obstacle avoidance, and adaptive path planning. Data fusion from LiDAR, camera, and IMU enables the robot to drive smoothly and make intelligent decisions in real time.  

- **Cross-platform architecture:** The system runs high-level perception and control on the **Raspberry Pi 4 (Python)** while the **ESP32 (C/C++)** handles low-level motor control, servo steering, and feedback loops. This distributed design ensures fast response, modularity, and efficient hardware utilization.  

- **Open-source and fully documented:** All **CAD models**, **wiring diagrams**, **source code**, and **build instructions** are openly available for others to learn from and replicate. Full documentation, including setup guides and engineering notes, is published through **MkDocs** at [dextertaha.github.io/WRO-FE-2025-Mindcraft](https://dextertaha.github.io/WRO-FE-2025-Mindcraft/).


![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

### 🌍 World Robot Olympiad (WRO)

> The World Robot Olympiad (WRO) is a prestigious international robotics competition that ignites the imaginations of students worldwide. It challenges participants to showcase their creativity, problem-solving skills, and technical prowess in designing and programming robots for a variety of tasks and challenges.
>
> One of the most dynamic categories within WRO is the Future Engineers category. Here, participants are tasked with developing innovative solutions to real-world problems using robotics and automation. This category serves as a breeding ground for future innovators, encouraging students to think critically and creatively, laying the groundwork for a new generation of engineers and technologists.
>
> This year, the Future Engineers category presents an exciting challenge: creating a self-driving car. This challenge pushes participants to explore the cutting edge of robotics, adding layers of complexity and innovation to an already thrilling competition.

🎥 <a href="https://www.youtube.com/watch?v=_J15lf6uhwo&t=2s" target="_blank" rel="noopener noreferrer">Watch the challenge explanation video</a>

Official rules: <a href="https://wro-association.org/wp-content/uploads/WRO-2025-Future-Engineers-Self-Driving-Cars-General-Rules.pdf" target="_blank" rel="noopener noreferrer">Download the WRO 2025 Future Engineers — Self-Driving Cars official rules (PDF)</a>


![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 📁 Project Structure

```
📦 WRO-FE-2025-Mindcraft
├── 📁 Models
├── 📁 docs
├── 📁 images
├── 📁 other
│   ├── 📁 BOM(Bill Of Materials
│   ├── 📁 team-photos
│   └── 📁 video
├── 📁 schemes
├── 📁 src ( source code )
├── 📁 t-photos
├── 📁 v-photos
├── 📁 videos
├── 📄 .gitignore
├── 📄 LICENSE
└── 📄 README.md
```


![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

# 🏁 WRO Future Engineers Competition

| **1.Mobility Management**                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Mobility management discussion should cover how the vehicle movements are managed. What motors are selected, how they are selected and implemented. A brief discussion regarding the vehicle chassis design /selection can be provided as well as the mounting of all components to the vehicle chassis/structure. The discussion may include engineering principles such as speed, torque, power etc. usage. Building or assembly instructions can be provided together with 3D CAD files to 3D print parts. |
| <a href="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/60c6af51964cac08a16972d11f31143172ebb7d1/Models" target="_blank" rel="noopener noreferrer">Robot Parts & Design</a>                                                                                                                                                                                                                                                                                                                                                                              |
| [Power System]()                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| [Sensing Units]()                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| [Steering System]()                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |

---

| **2.Power and Sense Management**                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Power and Sense management discussion should cover the power source for the vehicle as well as the sensors required to provide the vehicle with information to negotiate the different challenges. The discussion can include the reasons for selecting various sensors and how they are being used on the vehicle together with power consumption. The discussion could include a wiring diagram with BOM for the vehicle that includes all aspects of professional wiring diagrams. |
| [Bill of Materials (BOM)]()                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| <a href="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/8bc0357a20f934db1f0ee246594716c5e9e2c6fb/schemes" target="_blank" rel="noopener noreferrer">Schematics</a>                                                                                                                                                                                                                                                                                                                                                               |

---

| **3.Obstacle Management**                                                                                                                                                                                                 |
| :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Obstacle management discussion should include the strategy for the vehicle to negotiate the obstacle course for all the challenges. This could include flow diagrams, pseudo code and source code with detailed comments. |
| [Strategy]()                                                                                                                                                                                                              |
| [Arduino Functions]()                                                                                                                                                                                                     |
| [Open challenge]()                                                                                                                                                                                                        |
| [Dashboard Visualisation]()                                                                                                                                                                                               |
| [Map randomizer & score calculator]()                                                                                                                                                                                     |

---

| **4.Pictures – Team and Vehicle**                                                                                                                                                                                                                                                                                                                                                                                    |
| :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Pictures of the team and robot must be provided. The pictures of the robot must cover all sides of the robot, must be clear, in focus and show aspects of the mobility, power and sense, and obstacle management. Reference in the discussion sections 1, 2 and 3 can be made to these pictures. Team photo is necessary for judges to relate and identify the team during the local and international competitions. |
| <a href="https://github.com/DexterTaha/WRO-FE-2024-Mindcraft-International/tree/2bbf1b3f514550d7e91d4fa6e24702a49f4da584/v-photos" target="_blank" rel="noopener noreferrer">Vehicle Photos</a>                                                                                                                                                                                                                                                                           |
| <a href="https://github.com/DexterTaha/WRO-FE-2024-Mindcraft-International/tree/2bbf1b3f514550d7e91d4fa6e24702a49f4da584/t-photos" target="_blank" rel="noopener noreferrer">Team Members & Pictures</a>                                                                                                                                                                                                                                                                  |

---

| **5.Performance Videos**                                                                                                                                                                                                                            |
| :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| The performance videos must demonstrate the performance of the vehicle from start to finish for each challenge. The videos could include an overlay of commentary, titles or animations. The video could also include aspects of section 1, 2 or 3. |
| <a href="https://github.com/DexterTaha/WRO-FE-2024-Mindcraft-International/tree/2bbf1b3f514550d7e91d4fa6e24702a49f4da584/videos" target="_blank" rel="noopener noreferrer">Demonstration Videos</a>                                                                                                      |
| <a href="https://www.youtube.com/@MindcraftWRO-kw8vp" target="_blank" rel="noopener noreferrer">Youtube Channel</a>                                                                                                                                                                                      |

---

| **6.GitHub Utilization**                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Git and GitHub are available for opensource project management and file version control. As part of the design and development process, teams must use this platform to document their progress, coding development and share files. Judging the platform will include how complete the information provided is, how information is structured and how often commits were done. Teams can use this platform to provide additional information on their engineering design and coding of their vehicle as well. |
| <a href="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft" target="_blank" rel="noopener noreferrer">Repository Link</a>                                                                                                                                                                                                                                                                                                                                                                                                                                         |

---

| **7.Engineering Factor**                                                                                                      |
| :---------------------------------------------------------------------------------------------------------------------------- |
| Own Design and manufacturing of vehicle and components, with off the shelf electrical components, such as motors and sensors. |
| [Design Description]()                                                                                                        |


![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 👥 Team

[![Team Website](https://img.shields.io/badge/Website-Visit-brightgreen?style=for-the-badge&logo=web&logoColor=white)]()
[![Team Youtube](https://img.shields.io/badge/Youtube-%23FF0000.svg?style=for-the-badge&logo=Youtube&logoColor=white)](https://www.youtube.com/@MindcraftWRO-kw8vp)



![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 🧠 Our Centre

[![Centre Website](https://img.shields.io/badge/Website-Visit-brightgreen?style=for-the-badge&logo=web&logoColor=white)](https://www.mindcraft.ma/)
[![Centre Youtube](https://img.shields.io/badge/Youtube-%23FF0000.svg?style=for-the-badge&logo=Youtube&logoColor=white)](https://www.youtube.com/@Mindcraftma)
[![Facebook](https://img.shields.io/badge/Facebook-%231877F2.svg?style=for-the-badge&logo=Facebook&logoColor=white)](https://www.facebook.com/mindcraft.ma)
[![Instagram](https://img.shields.io/badge/Instagram-%23E4405F.svg?style=for-the-badge&logo=Instagram&logoColor=white)](https://www.instagram.com/mindcraft.ma)

Released 2025 by [DERDEB Salmane](https://github.com/salmane-derdeb) @ [Taha TAIDI LAAMIRI](https://github.com/DexterTaha) @ [Rayane GHACHA](https://github.com/Rayane-Ghacha)


![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## License
>You can check out the full license [here](https://github.com/IgorAntun/node-chat/blob/master/LICENSE)

This project is licensed under the terms of the **MIT** license.



![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

Like the work? 😍

Consider giving the repository a star 😎

<p align="right">
  <a href="#top">
    <img src="https://img.shields.io/badge/-BACK_TO_TOP-151515?style=flat-square" alt="Back to top" />
  </a>
</p>
