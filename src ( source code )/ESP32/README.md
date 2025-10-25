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

## Quick Start

- Build with PlatformIO (in VS Code): click PlatformIO Build or run `platformio run` from the project root.
- Upload to a connected board: use PlatformIO Upload or `platformio run --target upload`.
- Open serial monitor at the baud rate set in `src/main.cpp` (default 115200) to view logs and test output.

## Servo configuration (in `src/main.cpp`)

- `servoMin`, `servoMax` — safe angular range for the servo (defaults in code: 35..145).
- `servoMid` — computed center position.
- `servoStep`, `checkDelayMs` — control sweep resolution and speed (can be tuned for smoothness).

If you want runtime control, the project can be extended with a simple Serial command parser to call `steer(<percent>)`.
