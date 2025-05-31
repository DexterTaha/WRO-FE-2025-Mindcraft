# Robot Strategy for WRO Future Engineers

In this competition, our robot needs to figure out where to go and how to avoid obstacles using **sensors** like LiDAR and a camera. Here’s how it works:

---

## 1. **Understanding the Zones**

- There are special shapes on the board: two **X-shapes** and four **T-shapes**. These are called **Zones**.
- Our robot uses **4 sensors** to find out where these Zones are.
- After that, the robot locates itself based on these zones and gets ready to move!

![zones](https://github.com/DexterTaha/WRO-FE-2024-Mindcraft-International/blob/71f4f0625fcd301564ceb619e6b7f5b077dd08b7/images/zones.png)
---

## 2. **Looking for Obstacles**

- There are **24 possible places** where obstacles could be. We call them **P1, P2, P3, … up to P24**.
- The robot **pretends** these obstacles are there and checks their actual location using a **LiDAR sensor** (like a robot eye!).
- Then it uses a **camera** to figure out the color of the obstacles:
  - **Red** = Dangerous
  - **Green** = Safe
  - **None** = No obstacle
    
![obstacles](https://github.com/DexterTaha/WRO-FE-2024-Mindcraft-International/blob/9781a5c67df650d096feca76e06a3b82906b8b59/images/obstacles.png)

---

## 3. **Storing the Information**

- Each place, like P1, P2, etc., gets a value:
  - If there’s **no obstacle**, the value is **0**.
  - If there’s a **Green obstacle**, the value is **1**.
  - If there’s a **Red obstacle**, the value is **2**.
  
For example:
```powershell
P1 = 0 (no obstacle)
P2 = 2 (red obstacle)
p3 = 1 (green obstacle)
```


---

## 4. **Finding the Target**

- After knowing where the obstacles are, the robot uses its **LiDAR** to trace the path to the **target**.
- Then it moves carefully to avoid the obstacles and reach its goal!

![variables](https://github.com/DexterTaha/WRO-FE-2024-Mindcraft-International/blob/9ee2800d892db4f2527620af95ed6c12a5400082/images/variables.png)

This is the basic idea of how our robot works in the competition!
