#include "Input.h"
#include "config.h"

Input::Input() {
    _x = 0; _y = 0;
    _btn = false; _prevBtn = false; _pressed = false;
    _dir = 0;
}

void Input::_computeDirection() {
    _dir = 0;
    if (_y < -JOY_DEADZONE / 2)       _dir = 1; // up
    else if (_y > JOY_DEADZONE / 2)   _dir = 2; // down
    else if (_x < -JOY_DEADZONE / 2)  _dir = 3; // left
    else if (_x > JOY_DEADZONE / 2)   _dir = 4; // right
}

HWJoystick::HWJoystick() {}

bool HWJoystick::begin() {
    pinMode(JOY_VRX, INPUT);
    pinMode(JOY_VRY, INPUT);
    pinMode(JOY_SW, INPUT_PULLUP);
    analogReadResolution(10);   // 0..1023
    return true;
}

void HWJoystick::update() {
    int vx = analogRead(JOY_VRX);
    int vy = analogRead(JOY_VRY);

    // Map 0..1023 -> -100..100 (512 is center -> 0)
    int16_t x = (int16_t)(((long)(vx - 512) * 100) / 512);
    int16_t y = (int16_t)(((long)(vy - 512) * 100) / 512);
    if (x < -100) x = -100; if (x > 100) x = 100;
    if (y < -100) y = -100; if (y > 100) y = 100;

    _x = x;
    _y = y;

    bool btn = (digitalRead(JOY_SW) == LOW); // active-low button
    _pressed = (btn && !_prevBtn);
    _prevBtn = btn;
    _btn = btn;

    _computeDirection();
}

SerialJoystick::SerialJoystick() {
    _lineLen = 0;
    _line[0] = 0;
}

void SerialJoystick::_parseLine() {
    _line[_lineLen] = 0;
    int x = 0, y = 0, b = 0;
    if (sscanf(_line, "JOY %d %d %d", &x, &y, &b) == 3) {
        _x = (int16_t)x; if (_x < -100) _x = -100; if (_x > 100) _x = 100;
        _y = (int16_t)y; if (_y < -100) _y = -100; if (_y > 100) _y = 100;
        _btn = (b != 0);
    }
    _pressed = (_btn && !_prevBtn);
    _prevBtn = _btn;
    _computeDirection();
}

void SerialJoystick::update() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            if (_lineLen > 0) _parseLine();
            _lineLen = 0;
            _line[0] = 0;
        } else if (_lineLen < sizeof(_line) - 1) {
            _line[_lineLen++] = c;
        }
    }
}
