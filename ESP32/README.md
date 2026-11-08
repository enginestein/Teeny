# GameOS (ESP32)

Tiny game console on an ESP32. Snake and Atari Breakout. Wire up an OLED and
joystick and it runs on the board. Don't wire anything? It streams the screen
to your PC over USB and your keyboard controls it.

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

Wire each button between its GPIO and GND. The ESP32's internal pull-up does
the rest.

## Build & flash

```
pip install platformio
./flash.sh                # compile + upload (auto-detect port)
./flash.sh /dev/ttyUSB0   # specific serial port
```

Or open `GameOS.ino` in the Arduino IDE and upload.

## Play on your PC (no OLED/joystick)

Plug the ESP32 into your computer via USB and run:

```
./play.sh                 # auto-detect port
./play.sh /dev/ttyUSB0    # specific port
```

Controls: arrow keys move, Space/Enter is the button.

## Games

- Snake
- Atari Breakout (procedural levels)

MIT — hack, remix, build more games on top of it.
