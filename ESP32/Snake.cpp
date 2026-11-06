#include "Snake.h"
#include "config.h"

// ---------------------------------------------------------------------------
// Snake
// ---------------------------------------------------------------------------
Snake::Snake() {
    _len = 0;
    _fx = _fy = 0;
    _dx = 1; _dy = 0;
    _pendDx = 1; _pendDy = 0;
    _score = 0;
    _lastMove = 0;
    _moveDelay = 130UL;
    _frameId = 0;
    _gameOver = false;
}

void Snake::begin() {
    _len = 3;
    _sx[0] = 8;  _sy[0] = 8;
    _sx[1] = 7;  _sy[1] = 8;
    _sx[2] = 6;  _sy[2] = 8;
    _dx = 1; _dy = 0;
    _pendDx = 1; _pendDy = 0;
    _score = 0;
    _frameId = 0;
    _gameOver = false;
    _lastMove = millis();
    _placeFood();
}

void Snake::_placeFood() {
    // Pick a random empty cell.
    for (int tries = 0; tries < 100; tries++) {
        int8_t fx = (int8_t)(random(COLS));
        int8_t fy = (int8_t)(random(ROWS));
        bool onSnake = false;
        for (uint8_t i = 0; i < _len; i++) {
            if (_sx[i] == fx && _sy[i] == fy) { onSnake = true; break; }
        }
        if (!onSnake) { _fx = fx; _fy = fy; return; }
    }
    // Fallback: first empty cell found.
    for (int8_t y = 0; y < ROWS; y++) {
        for (int8_t x = 0; x < COLS; x++) {
            bool onSnake = false;
            for (uint8_t i = 0; i < _len; i++) {
                if (_sx[i] == x && _sy[i] == y) { onSnake = true; break; }
            }
            if (!onSnake) { _fx = x; _fy = y; return; }
        }
    }
}

void Snake::_drawCell(Display *d, int8_t gx, int8_t gy, bool color) {
    d->fillRect(gx * CELL, gy * CELL, CELL, CELL, color);
}

void Snake::_move() {
    int8_t hx = _sx[0] + _dx;
    int8_t hy = _sy[0] + _dy;

    // Wall collision
    if (hx < 0 || hx >= COLS || hy < 0 || hy >= ROWS) {
        _gameOver = true;
        _frameId++;
        return;
    }

    // Self collision
    for (uint8_t i = 0; i < _len; i++) {
        if (_sx[i] == hx && _sy[i] == hy) {
            _gameOver = true;
            _frameId++;
            return;
        }
    }

    // Shift body
    for (uint8_t i = _len; i > 0; i--) {
        _sx[i] = _sx[i - 1];
        _sy[i] = _sy[i - 1];
    }
    _sx[0] = hx;
    _sy[0] = hy;

    // Eat food?
    if (hx == _fx && hy == _fy) {
        if (_len < COLS * ROWS) _len++;
        _score += 10;
        _placeFood();
    }

    _frameId++;
}

void Snake::update(Input *input) {
    if (_gameOver) return;

    bool changed = false;

    // Apply pending direction from joystick (no 180 reversals).
    uint8_t dir = input->direction();
    if (dir == 1 && _dy == 0) { _pendDx = 0; _pendDy = -1; changed = true; }
    else if (dir == 2 && _dy == 0) { _pendDx = 0; _pendDy = 1; changed = true; }
    else if (dir == 3 && _dx == 0) { _pendDx = -1; _pendDy = 0; changed = true; }
    else if (dir == 4 && _dx == 0) { _pendDx = 1; _pendDy = 0; changed = true; }

    uint32_t now = millis();
    if (now - _lastMove >= _moveDelay) {
        _dx = _pendDx; _dy = _pendDy;
        _move();
        _lastMove = now;
        changed = true;
    }

    if (changed) _frameId++;
}

void Snake::draw(Display *d) {
    // Food
    _drawCell(d, _fx, _fy, true);
    // Snake body
    for (uint8_t i = 0; i < _len; i++) {
        bool head = (i == 0);
        _drawCell(d, _sx[i], _sy[i], true);
    }
    // Score
    char buf[16];
    snprintf(buf, sizeof(buf), "SC:%u", _score);
    d->drawText(2, 2, buf, true);
}
