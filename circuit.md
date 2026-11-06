# GameOS - Circuit

A dead-simple one-board gaming handheld for the ESP32.

## Parts
- 1 x ESP32 dev board (ESP32-WROOM / ESP32-S2 / ESP32-S3, any)
- 1 x 0.96" or 1.33" SSD1306 OLED, I2C, 128x64
- 1 x Analog joystick module with push button (VRX, VRY, SW, VCC, GND)
- Jumper wires

## Wiring

| ESP32 pin | OLED   | Joystick |
|-----------|--------|----------|
| 3.3V      | VCC    | VCC      |
| GND       | GND    | GND      |
| GPIO21    | SDA    | -        |
| GPIO22    | SCL    | -        |
| GPIO16*   | RST*   | -        |
| GPIO34    | -      | VRX (X)  |
| GPIO35    | -      | VRY (Y)  |
| GPIO32    | -      | SW       |

\* OLED RST is optional (wire to GPIO16 if your module has a RST pin,
otherwise leave it unconnected; the driver toggles it only when wired).

Joystick SW: the module button connects SW to GND when pressed. The ESP32
pin uses `INPUT_PULLUP`, so no external resistor is needed.

## Remote mode (no hardware)
If you don't have the OLED and joystick wired, just connect the ESP32 to
your computer via USB. GameOS auto-detects the missing OLED and switches
to remote mode: the game runs on the ESP32, the screen is streamed over
serial, and your PC keyboard controls it.

    python client/gameos_client.py

See client/README.md for client options.
