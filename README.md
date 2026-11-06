# GameOS

A tiny gaming operating system for the ESP32.  Simple circuit, two games
(Snake and Atari Breakout) with procedurally generated levels, and
automatic "remote mode" so you can play on your computer when the OLED/joystick aren't wired.

## Hardware

```
ESP32  +  SSD1306 OLED (I2C)  +  joystick (VRX/VRY/SW)
```

Full wiring: see `circuit.md`.

## How it works

1. On boot, GameOS probes I2C for an SSD1306 OLED.
2. **OLED found** → hardware mode: game renders on the OLED, controlled by
   the hardware joystick.
3. **No OLED** → remote mode: the ESP32 still runs the game, but streams
   the 128x64 screen over USB-serial to a PC client.  The PC renders it in
   a window and relays keyboard (arrows + space/enter) back to the ESP32
   as joystick input.  The game logic always runs on the ESP32.

## Flashing (Arduino IDE)

1. Install the ESP32 board package (File → Preferences → Additional Boards
   Manager URLs: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`).
2. Open `GameOS/GameOS.ino` in the Arduino IDE.
3. Tools → Board → ESP32 Arduino → ESP32 Dev Module.
4. Tools → Upload Speed → 921600 (match the client baud).
5. Select your ESP32 serial port and click Upload.

No extra libraries are required — the SSD1306 driver is self-contained
(raw I2C).

## Remote play (no OLED/joystick)

Just plug the ESP32 into your computer via USB and run:

```bash
cd client
pip install -r requirements.txt
python gameos_client.py
```

The client auto-detects the ESP32 serial port, or you can pass it manually:

```bash
python gameos_client.py --port /dev/ttyUSB0   # Linux
python gameos_client.py --port COM3           # Windows
python gameos_client.py --port /dev/cu.usbmodem...  # macOS
```

Controls (PC client):
- Arrow keys  → move snake
- Space / Enter → button (menu select / play)

## Project layout

```
GameOS/          Arduino sketch (ESP32 firmware)
  GameOS.ino     entry point + hardware auto-detect
  config.h       pins, baud, modes, timing
  Display.*      screen buffer + OLED + serial display drivers
  Input.*        HW joystick + serial joystick drivers
  GameOS.*       OS core: boot/menu/game/game-over state machine
  Snake.*        Snake game
  Breakout.*     Atari Breakout game (procedural levels)
  font5x7.*      tiny 5x7 bitmap font
client/          PC client (Python + pygame)
  gameos_client.py renders ESP32 screen, relays keyboard
circuit.md       wiring instructions
```

## License

MIT — hack, remix, build more games on top of it.
