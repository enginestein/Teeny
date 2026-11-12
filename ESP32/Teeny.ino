/*
 * Teeny - game console firmware for ESP32.
 *
 * Wiring:
 *   OLED        SDA=GPIO21, SCL=GPIO22, VCC=3.3V, GND=GND
 *   Joystick    VRX=GPIO34, VRY=GPIO35, SW=GPIO32 (to GND)
 *
 * With an OLED it runs standalone. Without one it streams the screen over
 * USB serial for client/teeny_client.py.
 *
 * Flash with PlatformIO or the Arduino IDE.
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
