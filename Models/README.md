<a name="top"></a>

# WRO Future Engineers Robot - Models

This section covers the **CAD modeling and manufacturing process** behind the development of our robot for the **WRO Future Engineers competition**, explaining our design tools, choices, and manufacturing techniques. The following highlights the key decisions in our process and why they benefit the project.

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 📌 Table of Contents

> [!TIP]
> Click the arrow below 👇 to expand the **Table of Contents**.  
> Every item is a clickable link to a section in this README.

<details>
<summary><b>📂 Table of Contents</b></summary>

1. [1. 🧩 Design Considerations Before CAD Modeling](#design-considerations-before-cad-modeling)
   - [1.1 Key Requirements](#key-requirements)
   - [1.2 Study and Planning](#study-and-planning)
2. [2. 🔁 Initial Robot Design and Iterations](#initial-robot-design-and-iterations)
   - [2.1 National Competition: LEGO Robot](#national-competition-lego-robot)
   - [2.2 First Version: Oversized DIY Robot](#first-version-oversized-diy-robot)
   - [2.3 Second Version: Lack of Differential System](#second-version-lack-of-differential-system)
   - [2.4 Third Version: Final Optimized Robot](#third-version-final-optimized-robot)
3. [3. 🧰 3D CAD Modeling - Onshape](#3d-cad-modeling-onshape)
   - [3.1 Why We Chose Onshape](#why-we-chose-onshape)
   - [3.2 Video of Onshape Demonstration](#video-of-onshape-demonstration)
   - [3.3 Screenshots of Our Robot 3D Model in Onshape](#screenshots-of-our-robot-3d-model-in-onshape)
   - [3.4 Other CAD Options Considered](#other-cad-options-considered)
   - [3.5 Onshape Advantages](#onshape-advantages)
   - [3.6 Learning Onshape](#learning-onshape)
4. [4. 🖨️ 3D Printing](#3d-printing)
   - [4.1 Why We Chose 3D Printing](#why-we-chose-3d-printing)
   - [4.2 Printer of Choice: Creality K1 Max](#printer-of-choice-creality-k1-max)
   - [4.3 3D Printed Robot Parts](#3d-printed-robot-parts)
   - [4.4 Video of Printing a Part](#video-of-printing-a-part)
   - [4.5 Why We Didn't Choose Laser Cutting or CNC Engraving](#why-we-didnt-choose-laser-cutting-or-cnc-engraving)
   - [4.6 Advantages of 3D Printing](#advantages-of-3d-printing)
5. [5. 🤖 Robot](#robot)
   - [5.1 Robot Assembly](#robot-assembly)
   - [5.2 Power System](#power-system)
   - [5.3 Steering System](#steering-system)
6. [6. 🧩 Robot Parts Details](#robot-parts-details)
7. [7. ✅ Conclusion](#conclusion)

</details>

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 1. 🧩 Design Considerations Before CAD Modeling <a id="design-considerations-before-cad-modeling"></a>

Before we began the CAD modeling process, we conducted a thorough study to establish the key requirements and considerations for our robot design. Our primary goal was to create a robot that is efficient, reliable, and optimized for the competition's challenges. The following points outline the main factors we considered:

### 1.1 Key Requirements <a id="key-requirements"></a>

- **Compact Size**: Our strategy required the robot to be smaller than a **20cm cube** to navigate efficiently through the competition environment.
- **Turning Radius**: The robot must be able to turn within an outside circle of maximum **40cm**, enabling it to maneuver in tight spaces.
- **Stability**: A **low center of gravity** was essential to ensure the robot's stability during movement and operations.
- **Weight Distribution**: **Equal weight distribution** across the robot prevents tipping and improves handling.
- **Traction**: High **friction on traction wheels** was necessary to prevent slipping and to provide better control.
- **Assembly Method**: All robot parts should be assembled using **screws**, **nuts**, and **zip ties**—**no glue**—to allow for easy modifications and repairs.
- **Ease of Modification**: The robot should be **sturdy yet easy to modify** and improve as needed.
- **Lightweight Design**: Keeping the robot as **lightweight as possible** enhances speed and reduces energy consumption.
- **Differential System**: Incorporating a **differential system** allows the robot to turn easily and smoothly.
- **Battery Safety**: The battery should be placed in a **safe location** to avoid damage from bumps or accidents, and to prevent hazards if the battery fails.
- **Component Safety**: Critical components should be **protected from potential battery issues**, such as leaks or explosions.
- **Ease of Reassembly**: The robot should be **easy to rebuild**, with parts designed to assemble in only **one way** to minimize errors.
- **Availability of Parts**: All parts should be **commonly available** in most markets to allow other teams to rebuild or service the robot if needed.

### 1.2 Study and Planning <a id="study-and-planning"></a>

We meticulously planned the robot's design to meet these requirements. By prioritizing a compact and stable structure, we ensured that the robot could navigate the competition course effectively. The decision to use common assembly methods and readily available parts not only facilitated our building process but also made our design accessible to others.

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 2. 🔁 Initial Robot Design and Iterations <a id="initial-robot-design-and-iterations"></a>

Our journey began with initial prototypes and several iterations to refine our robot design. Throughout this process, we learned valuable lessons that informed our final design.

### 2.1 National Competition: LEGO Robot <a id="national-competition-lego-robot"></a>

We participated in the **national competition** with a robot constructed using **LEGO components** due to their ease of use and availability. However, this version had several issues:

- **Size and Speed Issues**: The robot was **too big and slow**, exceeding the size limitations we had set and lacking the required speed.
- **Obstacle Challenges**: We couldn't manage to successfully complete the **obstacle challenge and parking**, which are critical parts of the competition.
- **Structural Limitations**: LEGO components did not provide the **sturdiness** and customization required for the competition's demands.

*Image of LEGO Robot Used in National Competition:*

[LEGO Robot](https://github-production-user-asset-6210df.s3.amazonaws.com/130682580/337490248-3f184c4a-aa2b-491f-9ec5-0fe420d42f31.gif?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAVCODYLSA53PQK4ZA%2F20241124%2Fus-east-1%2Fs3%2Faws4_request&X-Amz-Date=20241124T153606Z&X-Amz-Expires=300&X-Amz-Signature=f0453b3dcbf27a713996a15e16b4535ea7a080e4f0b3ac95c5329351e6ec7839&X-Amz-SignedHeaders=host)

### 2.2 First Version: Oversized DIY Robot <a id="first-version-oversized-diy-robot"></a>

After qualifying for the international stage, we decided to build a **100% DIY robot**, making all the mechanics and body ourselves while purchasing only the electronics. Our first DIY version had the following issues:

- **Size Issue**: The robot was **still too big**, not meeting our compact size requirement of fitting within a 20cm cube.
- **Weight Problems**: The larger size contributed to increased weight, affecting speed and maneuverability.
- **Inefficient Design**: The oversized structure made it difficult to navigate the course effectively.

*Link of First DIY Robot Version:*

[First DIY Robot](https://cad.onshape.com/documents/08595aa7e5b1cdab597252fc/w/cfc7e06246a86472db038f97/e/671ce5308fe71051d562260c)

### 2.3 Second Version: Lack of Differential System <a id="second-version-lack-of-differential-system"></a>

In the second iteration of our DIY robot, we attempted to reduce the size and weight but encountered new challenges:

- **No Differential System**: The robot **lacked a differential system**, making turning difficult and inefficient.
- **Poor Maneuverability**: The robot would **drift a lot** and took **150cm** to complete a full turn, which is unacceptable for tight course navigation.
- **Inefficient Turning Radius**: The large turning radius prevented the robot from handling obstacles and precise movements required in the competition.

*Link of Second DIY Robot Version:*

[Second DIY Robot](https://cad.onshape.com/documents/fbbf77d5a7d51852563af36e/w/afa3b15c16b20a57b3f67b6c/e/98887fbf0f458e0817d24d8b)

### 2.4 Third Version: Final Optimized Robot <a id="third-version-final-optimized-robot"></a>

For the third version, we made significant changes to address the previous shortcomings:

- **Incorporation of Differential System**: We added a **differential system** to allow smooth and efficient turning.
- **Hardware Upgrade**: We switched from using the **NVIDIA Jetson Nano** to the **Raspberry Pi 4 Model B**, which provided:

  - **Improved Compatibility**: Better compatibility with our control systems and peripherals.
  - **Weight Reduction**: The Raspberry Pi is **lighter**, contributing to our goal of a lightweight robot.
  - **Energy Efficiency**: Lower power consumption helped manage the robot's energy requirements.

- **Optimized Design**: The robot now fits within the **20cm cube** constraint, has a **low center of gravity**, and features **equal weight distribution**.
- **Enhanced Maneuverability**: With the differential system, the robot can turn efficiently within the required **40cm outside circle**.
- **Sturdy and Modular Assembly**: All parts are assembled using screws, nuts, and zip ties—no glue—making the robot sturdy yet easy to modify and improve.
- **Safety Features**: The battery is placed in a safe location to avoid damage from bumps or accidents, and critical components are protected from potential battery issues.

*Link of Third DIY Robot Version:*

[Third DIY Robot](https://cad.onshape.com/documents/1c6f1405e84d0c390333223c/w/c90b719cdc670bdbfb16a84e/e/7f48b1d9bd685347e4768d04)

This final version solved all the problems encountered in previous iterations, resulting in a robot that is compact, efficient, and competition-ready.

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 3. 🧰 3D CAD Modeling - Onshape <a id="3d-cad-modeling-onshape"></a>

To design and develop the robot, we used **Onshape**, a cloud-based CAD tool that allows for efficient collaboration and design flexibility. We had various options for CAD software, such as **Fusion 360** and **CATIA**, but we ultimately chose Onshape for several reasons:

### 3.1 Why We Chose Onshape <a id="why-we-chose-onshape"></a>

- **Cloud-Based**: Onshape operates entirely in the cloud, allowing our team to collaborate from any location. This was essential since team members were working from different places.
- **Collaborative Design**: Multiple team members can work on the same design simultaneously, ensuring faster iterations and feedback loops.
- **Free for Educational Use**: Being students representing Morocco, Onshape’s free educational plan provided us with full access to powerful design tools without extra costs.
- **No Hardware Constraints**: Onshape runs in a web browser, meaning we didn’t need high-end computers or complex software installations. This allowed everyone on the team to contribute regardless of their hardware limitations.

You can learn more about Onshape here: [Onshape Website](https://www.onshape.com/)

### 3.2 Video of Onshape Demonstration <a id="video-of-onshape-demonstration"></a>

[![Watch the video](https://img.icons8.com/color/452/play--v1.png)](https://github.com/user-attachments/assets/29b17698-be3b-4aab-8515-d6c245b77802)

### 3.3 Screenshots of Our Robot 3D Model in Onshape <a id="screenshots-of-our-robot-3d-model-in-onshape"></a>

![Robot Chassis Design]()

### 3.4 Other CAD Options Considered <a id="other-cad-options-considered"></a>

- **Fusion 360**: Hybrid cloud solution, limited collaboration, and high costs.
- **CATIA**: Too complex and expensive for our needs.

### 3.5 Onshape Advantages <a id="onshape-advantages"></a>

- **Real-Time Collaboration**: Multiple people can work on the design at the same time, accelerating the design process.
- **Accessible Anywhere**: Since it’s cloud-based, we could work on designs from any device with an internet connection.
- **Powerful CAD Tools**: Despite being browser-based, Onshape provides all the advanced CAD features we needed to design complex mechanical components for our robot.
- **Version Control**: We could easily track changes, revert to previous versions, and work on multiple iterations without losing progress.

### 3.6 Learning Onshape <a id="learning-onshape"></a>

We learned how to use Onshape through resources from the official Onshape website and a helpful playlist on YouTube.

- [Onshape Official Website](https://www.onshape.com/learn)
- [Onshape YouTube Playlist](https://youtube.com/playlist?list=PL4FdDkwWXT9p3IaT11JjJcnwnFWiHJuco&si=v0Z2kmiZvLGKHOFY)

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 4. 🖨️ 3D Printing <a id="3d-printing"></a>

Once our designs were finalized in Onshape, we needed to choose a manufacturing method. We considered several techniques like **laser cutting**, **CNC engraving**, and **3D printing**, but we ultimately decided to use **3D printing** due to its flexibility and ability to produce complex parts for our robot.

### 4.1 Why We Chose 3D Printing <a id="why-we-chose-3d-printing"></a>

- **Complex Geometries**: 3D printing allowed us to create complex and custom geometries that are difficult to achieve with laser cutting or CNC engraving.
- **Rapid Prototyping**: We could quickly print and test various parts, allowing us to make fast iterations on our robot design.
- **Cost-Effective**: 3D printing is often more affordable for one-off parts or prototypes compared to CNC engraving or laser cutting, which require more setup and material waste.
- **Less Material Waste**: 3D printing only uses the material needed for the part, reducing waste and lowering costs.

### 4.2 Printer of Choice: Creality K1 Max <a id="printer-of-choice-creality-k1-max"></a>

We selected the **Creality K1 Max** 3D printer for manufacturing our robot parts. This printer has several key advantages:

- **Large Build Volume**: The K1 Max provides a large build area, allowing us to print large components of the robot in a single go without having to divide them into smaller parts.
- **High Speed**: With fast print speeds, we were able to print our parts in a short amount of time, keeping up with the rapid pace of our project.
- **Precision**: The Creality K1 Max delivers high accuracy and detail in its prints, ensuring that our robot's mechanical parts fit together perfectly.
- **Ease of Use**: The printer is easy to set up and use, even for students, making it an ideal choice for quick prototyping.

Learn more about the Creality K1 Max here: [Creality K1 Max Printer](https://www.creality.com/products/creality-k1-max-3d-printer?spm=..404.header_1.1)

### 4.3 3D Printed Robot Parts <a id="3d-printed-robot-parts"></a>

![3D Printed Robot Parts](https://github.com/DexterTaha/WRO-FE-2024-Mindcraft-International/blob/main/images/Robot%20Creality%20Silcer.png)

### 4.4 Video of Printing a Part <a id="video-of-printing-a-part"></a>

[![Watch the video](https://img.icons8.com/color/452/play--v1.png)](https://github.com/user-attachments/assets/a05f5144-69ca-4180-a867-5e9cf91d8875)

*This video was made by us while printing the robot base of our robot.*

### 4.5 Why We Didn't Choose Laser Cutting or CNC Engraving <a id="why-we-didnt-choose-laser-cutting-or-cnc-engraving"></a>

- **Laser Cutting**: While laser cutting is great for creating flat, 2D parts, it is limited in producing complex 3D shapes and detailed mechanical components.
- **CNC Engraving**: CNC is excellent for metal and wood parts but involves more setup time and higher costs, especially for custom parts. It also generates more material waste, making it less ideal for our project, which requires many iterations.

### 4.6 Advantages of 3D Printing <a id="advantages-of-3d-printing"></a>

- **Customization**: We can easily make modifications to the design and print new parts within hours.
- **Quick Turnaround**: 3D printing enabled us to quickly prototype, test, and refine parts, significantly reducing development time.
- **Sustainable**: With 3D printing, there’s less waste of materials, aligning with our goal of sustainability for this project.

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 5. 🤖 Robot <a id="robot"></a>

### 5.1 Robot Assembly <a id="robot-assembly"></a>

|                   | Robot                                                         |
|-------------------|---------------------------------------------------------------|
| Robot Dimensions  | ![Robot Dimensions](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/Models/Drawing%20Dimensions%20Robot.png) |
| Robot Assembly    | ![Robot Assembly](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/Models/Drawing%20Parts%20Robot.png) |
| Robot View        | ![Robot View](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/Models/Drawing%20Views%20Robot.png) |

### 5.2 Power System <a id="power-system"></a>

|                   | Power System                                                 |
|-------------------|--------------------------------------------------------------|
| Power System Assembly | ![Power System Assembly]() |
| Power System View     | ![Power System View]() |

### 5.3 Steering System <a id="steering-system"></a>

|                   | Steering System                                              |
|-------------------|--------------------------------------------------------------|
| Steering System Assembly | ![Steering System Assembly]() |
| Steering System View     | ![Steering System View]() |

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 6. 🧩 Robot Parts Details <a id="robot-parts-details"></a>

| Part Name                           | Image                                                                                     | 3D File Link                                                                                       |
|-------------------------------------|-------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------|
| 0x01- Robot Base                    | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/raw/main/Models/Parts/0x01-%20Robot%20Base/Isometric%20Robot%20Base.png" alt="Robot Base" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x01-%20Robot%20Base) |
| 0x02- Second Layer                  | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/raw/main/Models/Parts/0x02-%20Second%20Layer/Isometric%20Second%20Layer.png" alt="Second Layer" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x02-%20Second%20Layer) |
| 0x03- Big Gear                      | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/raw/main/Models/Parts/0x03-%20Big%20Gear/Isometric%20Big%20Gear.png" alt="Big Gear" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x03-%20Big%20Gear) |
| 0x04- Small Gear                    | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/raw/main/Models/Parts/0x04-%20Small%20Gear/Isometric%20Small%20Gear.png" alt="Small Gear" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x04-%20Small%20Gear) |
| 0x05- Berring Support Left          | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/raw/main/Models/Parts/0x05-%20Berring%20Support%20Left/Isometric%20Berring%20Support%20Left.png" alt="Berring Support Left" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x05-%20Berring%20Support%20Left) |
| 0x06- Berring Support Right         | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/raw/main/Models/Parts/0x06-%20%20Berring%20Support%20Right/Isometric%20Berring%20Support%20Right.png" alt="Berring Support Right" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x06-%20%20Berring%20Support%20Right) |
| 0x07- Wheel Support                 | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/raw/main/Models/Parts/0x07-%20Wheel%20Support/Isometric%20Wheel%20Support.png" alt="Wheel Support" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x07-%20Wheel%20Support) |
| 0x08- Long Shaft                    | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/raw/main/Models/Parts/0x08-%20Long%20Shaft/Isometric%20Long%20Shaft.png" alt="Long Shaft" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x08-%20Long%20Shaft) |
| 0x09- Short Shaft                   | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/raw/main/Models/Parts/0x09-%20Short%20Shaft/Isometric%20Short%20Shaft.png" alt="Short Shaft" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x09-%20Short%20Shaft) |
| 0x10- Steering Conector             | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/raw/main/Models/Parts/0x10-%20Steering%20Conector/Isometric%20Steering%20Conector.png" alt="Steering Conector" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x10-%20Steering%20Conector) |
| 0x11- Steering Rack                 | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/raw/main/Models/Parts/0x11-%20Steering%20Rack/Isometric%20Steering%20Rack.png" alt="Steering Rack" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x11-%20Steering%20Rack) |
| 0x12- Front Support Camera          | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/raw/main/Models/Parts/0x12-%20Front%20Support%20Camera/Isometric%20Front%20Support%20Camera.png" alt="Front Support Camera" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x12-%20Front%20Support%20Camera) |
| 0x13- Back Support Camera           | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/raw/main/Models/Parts/0x13-%20Back%20Support%20Camera/Isometric%20Back%20Support%20Camera.png" alt="Back Support Camera" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x13-%20Back%20Support%20Camera) |
| 0x14- scwer shaft                   | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/Models/Parts/0x14-%20Screw%20shaft/Isometric%20Screw%20shaft.png" alt="scwer shaft" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x14-%20Screw%20shaft) |
| 0x15- Axle Clamp                    | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/raw/main/Models/Parts/0x15-%20Axle%20Clamp/Isometric%20Axle%20Clamp.png" alt="Axle Clamp" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x15-%20Axle%20Clamp) |
| 0x16- stand off 3mm                 | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/Models/Parts/0x16-%20Stand%20Off%203mm/Isometric%20Stand%20off%203mm.png" alt="stand off 3mm" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x16-%20Stand%20Off%203mm) |
| 0x17- stand off 2mm                 | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/Models/Parts/0x17-%20Stand%20Off%202mm/Isometric%20%20Stand%20off%202mm.png" alt="stand off 2mm" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x17-%20Stand%20Off%202mm) |
| 0x18- stand off 1mm                 | <img src="https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/blob/main/Models/Parts/0x18-%20Stand%20Off%201mm/Isometric%20Stand%20off%201mm.png" alt="stand off 1mm" width="300"/> | [Part Details](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft/tree/main/Models/Parts/0x18-%20Stand%20Off%201mm) |

Notes:
- I used HTML <img> tags with a width of 120px so images appear smaller in the table while keeping the original aspect ratio.
- If you'd like them even smaller or responsive, say a target width (e.g., 80, 100, 140) and I will update the table accordingly.

![-----------------------------------------------------](https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/rainbow.png)

## 7. ✅ Conclusion <a id="conclusion"></a>

For our **WRO Future Engineers robot project**, using **Onshape for CAD modeling** and the **Creality K1 Max for 3D printing** has been a critical part of our design and manufacturing process. These tools allowed us to work collaboratively, iterate quickly, and produce complex parts efficiently, all while staying within the constraints of an educational project.

Thank you for following our journey as we design a cutting-edge robot to represent **Morocco** on the global stage at the **World Robot Olympiad**!

For more information and to follow our progress, check out our [Project Link](https://github.com/DexterTaha/WRO-FE-2025-Mindcraft).

<p align="right">
  <a href="#top">
    <img src="https://img.shields.io/badge/-BACK_TO_TOP-151515?style=flat-square" alt="Back to top" />
  </a>
</p>
