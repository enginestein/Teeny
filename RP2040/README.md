# Teeny (RP2040)

A tiny game console that runs entirely on an RP2040 (SparkFun Thing Plus).
The PC is optional — plug in an OLED and buttons and it runs standalone.
No OLED? It streams the screen to your PC over USB instead.

## What it is

- 8 games: Snake, Breakout, Invaders, Pong, Tetris, Runner, Pacmate, Flappy
- 160x144 color framebuffer
- Fixed 60 Hz game loop
- Runs from microSD card if you have one, works fine without one

## Modes

**Hardware mode** — OLED + buttons wired to the RP2040. The game runs on the
board, drawn on the OLED, controlled by the buttons.

**Remote mode** — No OLED/buttons. The RP2040 streams the screen to a pygame
window on your PC over USB, and your keyboard controls the game.

The board picks the mode automatically at boot: if it finds an SSD1306 on I2C
it uses the OLED, otherwise it streams to USB.

## Hardware connections

| RP2040 GPIO | Connects to |
|-------------|-------------|
| GP4 | OLED SDA (I2C data) |
| GP5 | OLED SCL (I2C clock) |
| GP16 | Button UP |
| GP17 | Button DOWN |
| GP18 | Button LEFT |
| GP19 | Button RIGHT |
| GP20 | Button A |
| GP21 | Button B |
| GP22 | Button START |
| GP26 | Button SELECT |

Wire each button between its GPIO and GND. The RP2040's internal pull-up
does the rest — no external resistors needed. Wire the OLED to 3.3V and GND
too.

If you don't wire anything, just connect the RP2040 to your PC via USB and
run the viewer from `host/framebuffer_viewer/viewer.py`.

## Build & flash

```
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -B build
cmake --build build -j
```

Output: `build/teeny.uf2`.

Flash by holding the BOOT button, plugging in USB, and copying the UF2 onto
the `RPI-RP2` drive. Or use `tools/flash.sh` if your board still runs DeckOS.
