/*
 * Teeny - A tiny game console firmware for ESP32.
 *
 * Hardware (simple circuit):
 *   ESP32  +  SSD1306 OLED (I2C)  +  2-axis joystick with push button
 *
 * Connections:
 *   ESP32 GPIO21 (SDA)  -> OLED SDA
 *   ESP32 GPIO22 (SCL)  -> OLED SCL
 *   ESP32 3.3V          -> OLED VCC  / Joystick VCC
 *   ESP32 GND           -> OLED GND  / Joystick GND
 *   (optional) GPIO16   -> OLED RST
 *   ESP32 GPIO34        -> Joystick VRX (X axis)
 *   ESP32 GPIO35        -> Joystick VRY (Y axis)
 *   ESP32 GPIO32        -> Joystick SW  (button, other side to GND)
 *
 * Modes:
 *   - OLED detected  : draws to the OLED AND streams the screen to the PC
 *                      over USB-serial (use the PC client). Controlled by the
 *                      hardware joystick.
 *   - No OLED        : remote mode. Game runs on the ESP32, screen streamed
 *                      to the PC, keyboard controls it.
 *
 * Run the PC client:
 *   python client/teeny_client.py --port /dev/ttyUSB0   # Linux
 *   python client/teeny_client.py --port COM3           # Windows
 *
 * Flash with Arduino IDE (ESP32 board package) or PlatformIO.
 */

#include <Wire.h>
#include <cstring>

#include "config.h"
#include "Display.h"
#include "Input.h"
#include "Teeny.h"
#include "Snake.h"
#include "Breakout.h"
#include "Echo.h"
#include "Orbit.h"
#include "Ember.h"
#include "Mirror.h"

// Detect whether a real SSD1306 OLED is present on I2C.
static bool oledDetected() {
    Wire.beginTransmission(OLED_ADDR);
    if (Wire.endTransmission() == 0) return true;
    Wire.beginTransmission(0x3D);
    return (Wire.endTransmission() == 0);
}

Display   *gOled   = nullptr;   // OLED output (optional)
SerialDisplay *gSerial = nullptr; // PC output (always on)
Input     *gInput  = nullptr;
Teeny   *gOS      = nullptr;
int8_t     gMode   = MODE_HARDWARE;

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(200);

    Wire.begin(OLED_SDA, OLED_SCL);

    // The PC serial output is always active so you can play on your
    // computer whether or not the OLED is wired.
    gSerial = new SerialDisplay();

    if (oledDetected()) {
        gMode = MODE_HARDWARE;
        gOled = new OLEDDisplay();
        gOled->begin();
        gInput = new HWJoystick();
    } else {
        gMode = MODE_REMOTE;
        gInput = new SerialJoystick();
    }

    gInput->begin();

    // Teeny renders to the OLED when present, otherwise straight to serial.
    Display *primary = gOled ? static_cast<Display *>(gOled)
                             : static_cast<Display *>(gSerial);
    gOS = new Teeny(primary, gInput, gMode);
    gOS->addGame(new Snake());
    gOS->addGame(new Breakout());
    gOS->addGame(new Echo());
    gOS->addGame(new Orbit());
    gOS->addGame(new Ember());
    gOS->addGame(new Mirror());
    gOS->begin();
}

void loop() {
    static uint32_t lastFrame = 0;
    uint32_t now = millis();
    if (now - lastFrame < FRAME_MS) return;
    lastFrame = now;

    gInput->update();   // read joystick (HW or serial from PC)
    gOS->update();      // advance Teeny state machine
    gOS->draw();        // render to the primary display buffer

    // Push to OLED when wired.
    if (gOled) {
        gOled->display();
    }

    // Always stream the same frame to the PC client.
    if (gOled) {
        gSerial->copyFrom(gOled);   // copy OLED buffer -> serial buffer
    }
    gSerial->display();
}
