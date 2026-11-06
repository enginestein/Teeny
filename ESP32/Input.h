#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>
#include "config.h"

// ---------------------------------------------------------------------------
// Input abstraction.
//
// Normalised joystick values: x and y in -100..100, button boolean.
// direction(): 0=none, 1=up, 2=down, 3=left, 4=right (cardinal only).
//
// - HWJoystick     : reads the analog potentiometers + push button.
// - SerialJoystick : parses "JOY x y btn\n" lines from USB-serial
//                    (sent by the PC client in remote mode).
// ---------------------------------------------------------------------------
class Input {
public:
    Input();
    virtual ~Input() {}

    virtual bool begin() = 0;
    virtual void update() = 0;   // read raw inputs (non-blocking)

    int16_t x()  const { return _x; }
    int16_t y()  const { return _y; }
    bool button() const { return _btn; }
    bool pressed() const { return _pressed; }   // true on the edge of press
    uint8_t direction() const { return _dir; }

protected:
    int16_t  _x, _y;
    bool     _btn, _prevBtn, _pressed;
    uint8_t  _dir;

    void _computeDirection();
};

class HWJoystick : public Input {
public:
    HWJoystick();
    bool begin() override;
    void update() override;
};

class SerialJoystick : public Input {
public:
    SerialJoystick();
    bool begin() override { return true; }
    void update() override;   // parse "JOY x y btn\n" from Serial

private:
    char _line[32];
    uint8_t _lineLen;
    void _parseLine();
};

#endif
