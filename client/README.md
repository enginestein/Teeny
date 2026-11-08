# GameOS PC Client

Renders the ESP32's streamed screen in a pygame window and relays your
keyboard back to the board. Game logic runs on the ESP32; this is just the
display and controller.

## Requirements

```
pip install -r requirements.txt
```
(pyserial, pygame)

## Usage

```
python gameos_client.py                    # auto-detect port
python gameos_client.py --port /dev/ttyUSB0   # Linux
python gameos_client.py --port COM3                # Windows
python gameos_client.py --port /dev/cu.usbmodem... # macOS
```

## Controls

| Key | Action |
|-----|--------|
| Arrow keys | Move / select |
| Space / Enter | Button |
| Close window | Quit |

## Troubleshooting

- **No port found**: plug in the ESP32, check `ls /dev/ttyUSB*`, pass `--port`.
- **Blank window**: make sure `--baud` matches `SERIAL_BAUD` in the sketch (default 921600).
- **Permission denied (Linux)**: `sudo usermod -aG dialout $USER`, then log out and back in.
