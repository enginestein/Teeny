# RP2040 connections

Wire an SSD1306 OLED and buttons to the RP2040 for standalone hardware mode.

| RP2040 GPIO | Connects to |
|-------------|-------------|
| GP4 | OLED SDA |
| GP5 | OLED SCL |
| GP16 | Button UP |
| GP17 | Button DOWN |
| GP18 | Button LEFT |
| GP19 | Button RIGHT |
| GP20 | Button A |
| GP21 | Button B |
| GP22 | Button START |
| GP26 | Button SELECT |

Buttons: one side to GPIO, other side to GND. Internal pull-ups are used.
OLED: also wire VCC to 3.3V and GND to GND.

Don't want hardware? Just plug the RP2040 into your PC via USB. It streams
the screen to the viewer on your computer and your keyboard controls it.
