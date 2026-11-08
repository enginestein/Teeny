# ESP32 wiring

Parts:
- ESP32 dev board
- SSD1306 OLED (I2C, 128x64)
- Analog joystick module (VRX, VRY, SW, VCC, GND)

| ESP32 pin | Connects to |
|-----------|-------------|
| 3.3V | OLED VCC, Joystick VCC |
| GND | OLED GND, Joystick GND |
| GPIO21 | OLED SDA |
| GPIO22 | OLED SCL |
| GPIO34 | Joystick VRX (X axis) |
| GPIO35 | Joystick VRY (Y axis) |
| GPIO32 | Joystick SW (button) |

Wire the joystick button between GPIO32 and GND. The ESP32 internal pull-up
handles it — no external resistor needed.

No hardware? Just plug the ESP32 into your PC via USB and run `./play.sh`.
The screen streams over serial and your keyboard controls the game.
