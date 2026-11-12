#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include "config.h"

class Display {
public:
    Display();
    virtual ~Display() {}

    virtual bool begin() = 0;
    virtual void display() = 0;   // push buffer to hardware / serial

    void clear();
    void drawPixel(int16_t x, int16_t y, bool color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color);
    void drawChar(int16_t x, int16_t y, char c, bool color);
    void drawText(int16_t x, int16_t y, const char *s, bool color);

    int16_t width()  const { return SCREEN_W; }
    int16_t height() const { return SCREEN_H; }
    uint8_t *buffer() { return _buffer; }
    bool dirty() const { return _dirty; }

    // Copy another display's buffer into this one (for dual output).
    void copyFrom(const Display *other) {
        memcpy(_buffer, other->_buffer, sizeof(_buffer));
        _dirty = true;
    }

protected:
    uint8_t _buffer[(SCREEN_W * SCREEN_H) / 8];
    bool _dirty;

private:
    void setPixelRaw(uint8_t *buf, int16_t x, int16_t y, bool color);
};

// SSD1306 over I2C.
class OLEDDisplay : public Display {
public:
    OLEDDisplay();
    bool begin() override;
    void display() override;

private:
    void cmd(uint8_t c);
    void data(const uint8_t *d, size_t len);
};

// Streams the framebuffer to the PC client.
class SerialDisplay : public Display {
public:
    SerialDisplay();
    bool begin() override { return true; }
    void display() override;
};

#endif
