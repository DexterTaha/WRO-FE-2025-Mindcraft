<a name="top"></a>

# 🤖 MINDCRAFT — WRO Future Engineers (2025)

📌 This repository documents the research, engineering and software behind our self-driving vehicle built for the World Robot Olympiad (WRO) 2025 — Future Engineers division. The system is developed by DERDEB Salmane, Taha TAIDI LAAMIRI, and Rayane GHACHA and combines embedded systems (ESP32, Raspberry Pi), perception (camera + LiDAR), sensor fusion and control logic in Python and C++.

<p align="center">
  <img alt="Project logo" src="https://img.shields.io/badge/Project-WRO--FE--2025--Mindcraft-4b5563?style=for-the-badge&logo=robot" />
</p>

[![GitHub Stars](https://img.shields.io/github/stars/DexterTaha/WRO-FE-2025-Mindcraft.svg)](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/stargazers)
![OS](https://img.shields.io/badge/OS-Linux-red?style=flat&logo=linux)
[![Python](https://img.shields.io/badge/Made%20with-Python%203.8%2B-1f425f.svg?logo=python)](https://www.python.org/)
[![C++](https://img.shields.io/badge/C%2B%2B-ISO%20C%2B%2B-00599C?logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![Docs](https://img.shields.io/badge/Docs-MkDocs-blue?style=flat)](https://dextertaha.github.io/WRO-FE-2025-Mindcraft/)
[![License](https://img.shields.io/github/license/DexterTaha/WRO-FE-2025-Mindcraft)](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/LICENSE)

---

Contents
- Overview
- Highlights
- Project structure
- WRO requirements & mapping
- Getting started
- Documentation
- Contributing
- Team & Contact
- License

---

## Overview

MINDCRAFT is a compact autonomous vehicle designed to tackle the WRO Future Engineers self-driving car challenge. The architecture separates responsibilities across two compute platforms:

- High-level perception and planning: Raspberry Pi 4 (Python)
- Low-level control and actuation: ESP32 (C/C++)

Sensors include an RPLIDAR for 2D mapping, Raspberry Pi Camera for color/vision tasks, and a BNO055 IMU for orientation. Power is provided by a 12 V Li-ion pack; motor drivers and regulators isolate motor power from logic to reduce noise and ensure reliability.

---

## Highlights

- Modular, 3D-printed chassis with rear differential drive and front Ackermann steering.
- TB6612FNG-driven DC motors with encoder feedback for closed-loop speed control.
- Sensor fusion of LiDAR, camera, and IMU for mapping, obstacle avoidance, and adaptive path planning.
- Distributed software stack: perception/planning on Pi, control/servo on ESP32.
- Open CAD models, wiring diagrams, BOM, and source code — documented with MkDocs.

---

## Project structure

Top-level layout (abridged):

```
📦 WRO-FE-2025-Mindcraft
├── Models/                # CAD parts and assemblies
├── docs/                  # MkDocs site and documentation
├── images/
├── other/                 # BOM, component details, strategy
├── schemes/               # Wiring diagrams, power systems
├── src/ (source code)     # PI4B (Python), PICO/ESP32 (C/C++)
├── t-photos/              # Team photos
├── v-photos/              # Vehicle photos
├── videos/                # Demonstrations
├── .gitignore
├── LICENSE
└── README.md
```

Quick links:
- Models: https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models
- BOM & components: https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/other
- Schematics & wiring: https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/schemes/README.md
- Videos: https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/videos

---

## WRO mapping (how this repo satisfies WRO requirements)

1. Mobility management
   - Documented motor selection, torque/speed trade-offs, chassis design and mounting details.
   - See: Models and Models/README for design and assembly notes.

2. Power & sensor management
   - Isolated motor and logic rails, voltage regulators, and a detailed BOM.
   - See: other/BOM and schemes/README for wiring and power budgets.

3. Obstacle management
   - Strategy combining LiDAR mapping, vision-based object recognition, and IMU pose estimation.
   - See: other/strategy and src (ESP32 and PI4B folders) for code and algorithms.

4. Photos & videos
   - Team and multi-angle robot photos in t-photos and v-photos.
   - Demonstration videos in videos/ with challenge runs and tests.

5. GitHub usage
   - Source, docs, and version history are maintained in this repository; MkDocs site hosts the full documentation.

---

## Getting started (developer notes)

Prerequisites:
- Raspberry Pi 4 (Raspbian / Ubuntu)
- ESP32 toolchain (PlatformIO or ESP-IDF)
- Python 3.8+
- RPLIDAR / Pi Camera connected and tested

Quick steps:
1. Clone the repo:
   git clone https://github.com/DexterTaha/WRO-FE-2025-Mindcraft.git
2. Read docs/ to set up the Pi image and ESP32 firmware.
3. Install Python requirements (see docs or src/PI4B/requirements.txt).
4. Flash ESP32 firmware in src/ESP32/ (or PlatformIO project).
5. Run perception and control nodes as described in docs.

For step-by-step setup, troubleshooting and wiring diagrams, open the MkDocs site:
https://dextertaha.github.io/WRO-FE-2025-Mindcraft/

---

## Documentation

Full documentation (hardware, software, wiring, and build instructions) is published with MkDocs:
https://dextertaha.github.io/WRO-FE-2025-Mindcraft/

If something is missing or unclear, please open an issue with the "docs" label.

---

## Contributing

We welcome:
- Bug reports and issues
- Documentation improvements
- CAD/model fixes and 3D-printing notes
- Software improvements (Python or C/C++)

Please follow standard GitHub workflow:
- Fork → feature branch → PR with clear description and testing steps.
- Use the issue tracker to discuss larger changes first.

---

## Team & Contact

Released 2025 by:
- DERDEB Salmane — https://github.com/salmane-derdeb
- Taha TAIDI LAAMIRI — https://github.com/DexterTaha
- Rayane GHACHA — https://github.com/Rayane-Ghacha

Centre:
- Mindcraft — https://www.mindcraft.ma/
- YouTube: https://www.youtube.com/@Mindcraftma

Project channel:
- Team YouTube: https://www.youtube.com/@MindcraftWRO-kw8vp

---

## License

This project is released under the MIT License. See LICENSE for details:
https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/LICENSE

---

Like the project? Consider starring the repo ⭐

<p align="right">
  <a href="#top">
    <img src="https://img.shields.io/badge/-BACK_TO_TOP-151515?style=flat-square" alt="Back to top" />
  </a>
</p>
