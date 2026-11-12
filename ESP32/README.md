# Teeny (ESP32)

Tiny game console on an ESP32. Six games, all played with one joystick and
one button. Wire up an OLED and joystick and it runs on the board. Don't wire
anything? It streams the screen to your PC over USB and your keyboard
controls it.

## Hardware

| ESP32 pin | OLED   | Joystick |
|-----------|--------|----------|
| 3.3V      | VCC    | VCC      |
| GND       | GND    | GND      |
| GPIO21    | SDA    | –        |
| GPIO22    | SCL    | –        |
| GPIO34    | –      | VRX (X)  |
| GPIO35    | –      | VRY (Y)  |
| GPIO32    | –      | SW (button) |

Wire the joystick button between GPIO32 and GND. The ESP32's internal pull-up
does the rest.

## Build & flash

```
pip install platformio
./flash.sh                # compile + upload (auto-detect port)
./flash.sh /dev/ttyUSB0   # specific serial port
```

Or open `Teeny.ino` in the Arduino IDE and upload.

## Play on your PC (no OLED/joystick)

Plug the ESP32 into your computer via USB and run:

```
./play.sh                 # auto-detect port
./play.sh /dev/ttyUSB0    # specific port
```

Controls: arrow keys move, Space/Enter is the button.

## Games

Move up/down in the menu and press the button to play.

| Game | How to play |
|------|-------------|
| Snake | joystick steers |
| Breakout | left/right moves the paddle, button serves |
| Echo | dark maze; joystick moves, button sends a sonar ping |
| Orbit | button reverses direction, up/down changes orbit |
| Ember | joystick moves, button splashes water on the fire |
| Mirror | joystick picks a mirror, button rotates it; hit the target with the beam |

MIT — hack, remix, build more games on top of it.
