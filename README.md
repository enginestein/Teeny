# Teeny 

Two tiny gaming OS implementations for microcontrollers:

| Platform | Directory | Build system | Client |
|----------|-----------|--------------|--------|
| ESP32    | `ESP32/`  | PlatformIO / Arduino | `client/teeny_client.py` |
| RP2040   | `RP2040/` | CMake + Pico SDK | `RP2040/host/framebuffer_viewer/viewer.py` |

Both run game logic on the microcontroller. Wire up an OLED and buttons and it
runs standalone. No hardware? It streams the screen to your PC over USB and
your keyboard controls it.

## Quick start

**ESP32:**
```bash
cd ESP32
./flash.sh    # compile + upload
./play.sh     # launch the PC viewer
```

**RP2040:**
```bash
cd RP2040
cmake -B build && cmake --build build -j
./tools/play.sh   # launch viewer
```

## Layout

```
Teeny/
├── client/          PC client (pygame) for ESP32 remote mode
├── ESP32/           ESP32 firmware (Arduino sketch)
└── RP2040/          RP2040 firmware (C, Pico SDK)
```

MIT — hack, remix, build more games on top of it.
