# GameOS PC Client

Renders the GameOS screen (streamed from the ESP32 over USB-serial) on your
computer and relays keyboard input back to the ESP32 so you can play.

The game logic always runs on the ESP32; this client is only a display +
input terminal.

## Requirements

```bash
pip install -r requirements.txt
```

Requires: `pyserial`, `pygame`.

## Usage

```bash
# Auto-detect ESP32 serial port
python gameos_client.py

# Choose a port manually
python gameos_client.py --port /dev/ttyUSB0     # Linux
python gameos_client.py --port COM3             # Windows
python gameos_client.py --port /dev/cu.usbserial...  # macOS

# Options
python gameos_client.py --port /dev/ttyUSB0 --baud 115200 --scale 4 --fps 30
```

Options:
- `--port`   Serial port (auto-detected if omitted)
- `--baud`   Baud rate (default 921600, must match `SERIAL_BAUD` in the sketch)
- `--scale`  Window scale factor (default 4 → 512x256 window)
- `--fps`    Render cap (default 30)

## Controls

| Key | Action |
|-----|--------|
| Arrow keys | Move / select (Snake: steer; Breakout: move paddle) |
| Space / Enter | Button (play / confirm; Breakout: serve ball / next level / restart) |
| Close window | Quit |

## Troubleshooting

- **Client says no port found**: plug in the ESP32, check `ls /dev/ttyUSB*` / `ls /dev/cu.usb*` / Device Manager, and pass `--port`.
- **Blank window / no frames**: make sure `--baud` matches the sketch (`SERIAL_BAUD`, default 921600). Some cheap USB-UART bridges don't support 921600 — if so, change `SERIAL_BAUD` to `115200` in `GameOS/config.h` and pass `--baud 115200` (frame rate will drop to ~11 fps, still playable for Snake).
- **Permissions (Linux)**: add your user to the `dialout` group or run with `sudo`.
