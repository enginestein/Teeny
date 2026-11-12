#include "Ember.h"
#include "config.h"

Ember::Ember() {
    memset(_fire, 0, sizeof(_fire));
    _px = COLS / 2;
    _py = ROWS / 2;
    _wave = 0;
    _score = 0;
    _frameId = 0;
    _lastMove = _lastSpread = _lastSplash = 0;
    _spreadMs = 650;
    _over = false;
}

void Ember::begin() {
    randomSeed(micros());
    memset(_fire, 0, sizeof(_fire));
    _px = COLS / 2;
    _py = ROWS / 2;
    _wave = 0;
    _score = 0;
    _frameId = 0;
    _over = false;
    _spreadMs = 650;
    uint32_t now = millis();
    _lastMove = _lastSpread = _lastSplash = now;
    _seedFire(3);
}

int Ember::_fireCount() const {
    int n = 0;
    for (int8_t y = 0; y < ROWS; y++)
        for (int8_t x = 0; x < COLS; x++)
            if (_fire[y][x]) n++;
    return n;
}

void Ember::_seedFire(int count) {
    int placed = 0;
    int guard = 0;
    while (placed < count && guard < 600) {
        guard++;
        int8_t x = (int8_t)random(COLS);
        int8_t y = (int8_t)random(ROWS);
        if (_fire[y][x]) continue;
        int8_t dx = x - _px, dy = y - _py;
        if (dx * dx + dy * dy < 16) continue;
        _fire[y][x] = 1;
        placed++;
    }
    if (placed == 0 && count > 0) {
        int8_t x = (_px < COLS / 2) ? COLS - 1 : 0;
        int8_t y = (_py < ROWS / 2) ? ROWS - 1 : 0;
        _fire[y][x] = 1;
    }
}

void Ember::_spread() {
    uint8_t next[ROWS][COLS];
    memcpy(next, _fire, sizeof(next));

    int chance = 18 + (int)_wave * 3;
    if (chance > 55) chance = 55;

    for (int8_t y = 0; y < ROWS; y++) {
        for (int8_t x = 0; x < COLS; x++) {
            if (!_fire[y][x]) continue;
            if ((int)random(100) >= chance) continue;

            int start = (int)random(4);
            for (int k = 0; k < 4; k++) {
                int d = (start + k) & 3;
                int8_t nx = x, ny = y;
                if (d == 0) nx++;
                else if (d == 1) ny++;
                else if (d == 2) nx--;
                else ny--;
                if (nx < 0 || nx >= COLS || ny < 0 || ny >= ROWS) continue;
                if (_fire[ny][nx] || next[ny][nx]) continue;
                next[ny][nx] = 1;
                break;
            }
        }
    }

    memcpy(_fire, next, sizeof(_fire));
}

void Ember::_splash() {
    int ext = 0;
    for (int8_t dy = -1; dy <= 1; dy++) {
        for (int8_t dx = -1; dx <= 1; dx++) {
            int8_t x = _px + dx, y = _py + dy;
            if (x < 0 || x >= COLS || y < 0 || y >= ROWS) continue;
            if (_fire[y][x]) {
                _fire[y][x] = 0;
                ext++;
            }
        }
    }
    _score += (uint32_t)ext * 10;
}

void Ember::update(Input *input) {
    if (_over) return;

    uint32_t now = millis();

    uint8_t dir = input->direction();
    if (dir != 0 && (uint32_t)(now - _lastMove) >= 95) {
        int8_t nx = _px, ny = _py;
        if (dir == 1) ny--;
        else if (dir == 2) ny++;
        else if (dir == 3) nx--;
        else if (dir == 4) nx++;
        if (nx >= 0 && nx < COLS && ny >= 0 && ny < ROWS && !_fire[ny][nx]) {
            _px = nx;
            _py = ny;
            _lastMove = now;
            _frameId++;
        }
    }

    if (input->pressed() && (uint32_t)(now - _lastSplash) >= 320) {
        _lastSplash = now;
        _splash();
        _frameId++;
    }

    if ((uint32_t)(now - _lastSpread) >= _spreadMs) {
        _lastSpread = now;
        _spread();
        _frameId++;
    }

    if (_fire[_py][_px]) {
        _over = true;
        _frameId++;
        return;
    }

    if (_fireCount() == 0) {
        _wave++;
        _score += 100;
        _spreadMs = 620 - (uint32_t)_wave * 35;
        if (_spreadMs < 220) _spreadMs = 220;
        _seedFire(3 + (int)_wave * 2);
        _frameId++;
    }
}

void Ember::draw(Display *d) {
    char buf[24];
    snprintf(buf, sizeof(buf), "EMBER W:%u S:%u",
             (unsigned)_wave, (unsigned)_score);
    d->drawText(2, 1, buf, true);

    for (int8_t y = 0; y < ROWS; y++) {
        for (int8_t x = 0; x < COLS; x++) {
            if (!_fire[y][x]) continue;
            d->fillRect(OFFX + x * CELL, OFFY + y * CELL, CELL - 1, CELL - 1, true);
        }
    }

    int16_t px = OFFX + _px * CELL;
    int16_t py = OFFY + _py * CELL;
    d->fillRect(px, py, CELL, 1, true);
    d->fillRect(px, py + CELL - 1, CELL, 1, true);
    d->fillRect(px, py, 1, CELL, true);
    d->fillRect(px + CELL - 1, py, 1, CELL, true);
    d->drawPixel(px + CELL / 2, py + CELL / 2, true);
}
