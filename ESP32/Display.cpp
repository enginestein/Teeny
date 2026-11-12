#include "Display.h"
#include "config.h"
#include "font5x7.h"

#include <Wire.h>

Display::Display() {
    _dirty = false;
    memset(_buffer, 0, sizeof(_buffer));
}

void Display::clear() {
    memset(_buffer, 0, sizeof(_buffer));
    _dirty = true;
}

void Display::setPixelRaw(uint8_t *buf, int16_t x, int16_t y, bool color) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    uint16_t idx = (uint16_t)((y / 8) * SCREEN_W + x);
    uint8_t bit = (uint8_t)(y % 8);
    if (color) buf[idx] |=  (1 << bit);
    else       buf[idx] &= ~(1 << bit);
}

void Display::drawPixel(int16_t x, int16_t y, bool color) {
    setPixelRaw(_buffer, x, y, color);
    _dirty = true;
}

void Display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color) {
    for (int16_t dy = 0; dy < h; dy++) {
        for (int16_t dx = 0; dx < w; dx++) {
            setPixelRaw(_buffer, x + dx, y + dy, color);
        }
    }
    _dirty = true;
}

void Display::drawChar(int16_t x, int16_t y, char c, bool color) {
    const uint8_t *rows = font5x7Glyph(c);
    for (int16_t row = 0; row < 7; row++) {
        uint8_t bits = rows[row];
        for (int16_t col = 0; col < 5; col++) {
            if (bits & (1 << (4 - col))) {
                setPixelRaw(_buffer, x + col, y + row, color);
            }
        }
    }
    _dirty = true;
}

void Display::drawText(int16_t x, int16_t y, const char *s, bool color) {
    int16_t cx = x;
    while (*s) {
        drawChar(cx, y, *s, color);
        cx += 6;   // 5 px glyph + 1 px spacing
        s++;
    }
}

OLEDDisplay::OLEDDisplay() {}

void OLEDDisplay::cmd(uint8_t c) {
    Wire.beginTransmission(OLED_ADDR);
    Wire.write((uint8_t)0x00);   // control byte: Co=0, D/C=0 -> command
    Wire.write(c);
    Wire.endTransmission();
}

void OLEDDisplay::data(const uint8_t *d, size_t len) {
    size_t i = 0;
    while (i < len) {
        size_t chunk = len - i;
        if (chunk > 31) chunk = 31;   // keep each I2C transaction small
        Wire.beginTransmission(OLED_ADDR);
        Wire.write((uint8_t)0x40);    // control byte: Co=0, D/C=1 -> data
        for (size_t j = 0; j < chunk; j++) Wire.write(d[i + j]);
        Wire.endTransmission();
        i += chunk;
    }
}

bool OLEDDisplay::begin() {
    if (OLED_RST >= 0) {
        pinMode(OLED_RST, OUTPUT);
        digitalWrite(OLED_RST, LOW);
        delay(10);
        digitalWrite(OLED_RST, HIGH);
        delay(10);
    }

    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(400000);

    // SSD1306 init sequence (128x64)
    cmd(0xAE);            // display off
    cmd(0xD5); cmd(0x80); // clock div
    cmd(0xA8); cmd(0x3F); // multiplex 1/64
    cmd(0xD3); cmd(0x00); // display offset
    cmd(0x40 | 0x00);     // start line 0
    cmd(0x8D); cmd(0x14); // charge pump enable
    cmd(0x20); cmd(0x02); // page addressing mode
    cmd(0xA1);            // segment remap
    cmd(0xC8);            // com output scan direction
    cmd(0xDA); cmd(0x12); // com pins hardware config
    cmd(0x81); cmd(0xCF); // contrast
    cmd(0xD9); cmd(0xF1); // pre-charge
    cmd(0xDB); cmd(0x40); // vcomh
    cmd(0xA4);            // display all on resume
    cmd(0xA6);            // normal display
    cmd(0xAF);            // display on

    clear();
    display();
    return true;
}

void OLEDDisplay::display() {
    if (!_dirty) return;
    for (int16_t page = 0; page < 8; page++) {
        cmd(0xB0 | page);           // set page address
        cmd(0x21); cmd(0); cmd(127);// column start/end
        data(&_buffer[(uint16_t)page * 128], 128);
    }
    _dirty = false;
}

SerialDisplay::SerialDisplay() {}

void SerialDisplay::display() {
    // Send every frame, even when the screen hasn't changed, so the PC
    // client can resync after a missed packet.
    const uint8_t sync[4] = {0xAA, 0x55, 0xAA, 0x55};
    Serial.write(sync, 4);

    uint16_t len = (uint16_t)((SCREEN_W * SCREEN_H) / 8);
    Serial.write((uint8_t)(len & 0xFF));
    Serial.write((uint8_t)(len >> 8));

    uint8_t chk = 0;
    for (uint16_t i = 0; i < len; i++) {
        uint8_t b = _buffer[i];
        Serial.write(b);
        chk ^= b;
    }
    Serial.write(chk);

    _dirty = false;
}
