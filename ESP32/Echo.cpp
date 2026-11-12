#include "Echo.h"
#include "config.h"

Echo::Echo() {
    memset(_wall, 1, sizeof(_wall));
    memset(_seen, 0, sizeof(_seen));
    _px = _py = 1;
    _ex = COLS - 2;
    _ey = ROWS - 2;
    _level = 1;
    _score = 0;
    _frameId = 0;
    _timeLeft = 40000;
    _lastSec = -1;
    _lastTick = _lastMove = 0;
    _lastPing = 0;
    _pingStart = 0;
    _pingOn = false;
    _over = false;
}

void Echo::begin() {
    randomSeed(micros());
    _level = 1;
    _score = 0;
    _frameId = 0;
    _timeLeft = 40000;
    _over = false;
    _lastSec = -1;
    _lastMove = millis();
    _lastTick = millis();
    _lastPing = millis() - PING_COOLDOWN_MS;
    _pingStart = 0;
    _pingOn = false;
    _generate();
}

bool Echo::_isWall(int8_t x, int8_t y) const {
    if (x < 0 || x >= COLS || y < 0 || y >= ROWS) return true;
    return _wall[y][x] != 0;
}

void Echo::_reveal(int8_t cx, int8_t cy, int8_t r) {
    int8_t r2 = (int8_t)(r * r);
    for (int8_t y = 0; y < ROWS; y++) {
        for (int8_t x = 0; x < COLS; x++) {
            int8_t dx = x - cx, dy = y - cy;
            if (dx * dx + dy * dy <= r2) _seen[y][x] = 1;
        }
    }
}

void Echo::_generate() {
    for (int8_t y = 0; y < ROWS; y++) {
        for (int8_t x = 0; x < COLS; x++) {
            _wall[y][x] = 1;
            _seen[y][x] = 0;
        }
    }

    // Carve a perfect maze with a depth-first walk over the corridor cells.
    int sp = 0;
    _wall[1][1] = 0;
    _stackX[sp] = 1;
    _stackY[sp] = 1;
    sp++;

    while (sp > 0) {
        int8_t x = _stackX[sp - 1];
        int8_t y = _stackY[sp - 1];

        int8_t opts[4];
        int n = 0;
        if (x + 2 <= COLS - 2 && _wall[y][x + 2]) opts[n++] = 0;
        if (y + 2 <= ROWS - 2 && _wall[y + 2][x]) opts[n++] = 1;
        if (x - 2 >= 1 && _wall[y][x - 2]) opts[n++] = 2;
        if (y - 2 >= 1 && _wall[y - 2][x]) opts[n++] = 3;
        if (n == 0) {
            sp--;
            continue;
        }

        int8_t nx = x, ny = y;
        switch (opts[random(n)]) {
            case 0: _wall[y][x + 1] = 0; nx = x + 2; break;
            case 1: _wall[y + 1][x] = 0; ny = y + 2; break;
            case 2: _wall[y][x - 1] = 0; nx = x - 2; break;
            default: _wall[y - 1][x] = 0; ny = y - 2; break;
        }
        _wall[ny][nx] = 0;
        _stackX[sp] = nx;
        _stackY[sp] = ny;
        sp++;
    }

    _px = 1;
    _py = 1;
    _ex = COLS - 2;
    _ey = ROWS - 2;
    _reveal(_px, _py, 1);
}

void Echo::update(Input *input) {
    if (_over) return;

    uint32_t now = millis();
    _timeLeft -= (int32_t)(now - _lastTick);
    _lastTick = now;
    if (_timeLeft <= 0) {
        _timeLeft = 0;
        _over = true;
        _score = (uint32_t)(_level - 1) * 100;
        _frameId++;
        return;
    }

    bool pingOn = (uint32_t)(now - _pingStart) < PING_MS;
    if (pingOn != _pingOn) {
        _pingOn = pingOn;
        _frameId++;
    }

    if (input->pressed() && (uint32_t)(now - _lastPing) >= PING_COOLDOWN_MS) {
        _lastPing = now;
        _pingStart = now;
        _pingOn = true;
        _reveal(_px, _py, PING_RADIUS);
        _frameId++;
    }

    uint8_t dir = input->direction();
    if (dir != 0 && (uint32_t)(now - _lastMove) >= 110) {
        int8_t nx = _px, ny = _py;
        if (dir == 1) ny--;
        else if (dir == 2) ny++;
        else if (dir == 3) nx--;
        else if (dir == 4) nx++;

        if (!_isWall(nx, ny)) {
            _px = nx;
            _py = ny;
            _lastMove = now;
            _reveal(nx, ny, 1);
            _frameId++;

            if (nx == _ex && ny == _ey) {
                _level++;
                _timeLeft += 15000;
                _generate();
                _lastMove = now;
                _frameId++;
            }
        }
    }

    _score = (uint32_t)(_level - 1) * 100 + (uint32_t)(_timeLeft / 1000) * 5;

    int32_t sec = (_timeLeft + 999) / 1000;
    if (sec != _lastSec) {
        _lastSec = sec;
        _frameId++;
    }
}

void Echo::draw(Display *d) {
    char buf[24];
    snprintf(buf, sizeof(buf), "ECHO S:%u T:%02u",
             (unsigned)_score, (unsigned)((_timeLeft + 999) / 1000));
    d->drawText(2, 1, buf, true);

    for (int8_t y = 0; y < ROWS; y++) {
        for (int8_t x = 0; x < COLS; x++) {
            if (!_wall[y][x]) continue;
            int16_t px = OFFX + x * CELL;
            int16_t py = OFFY + y * CELL;
            int8_t dx = x - _px, dy = y - _py;
            bool near = _pingOn &&
                        (dx * dx + dy * dy <= PING_RADIUS * PING_RADIUS);
            if (near) {
                d->fillRect(px, py, CELL - 1, CELL - 1, true);
            } else if (_seen[y][x]) {
                d->drawPixel(px + CELL / 2, py + CELL / 2, true);
            }
        }
    }

    if (_seen[_ey][_ex]) {
        int16_t px = OFFX + _ex * CELL;
        int16_t py = OFFY + _ey * CELL;
        d->fillRect(px + 1, py + 1, CELL - 2, CELL - 2, true);
        d->fillRect(px + 2, py + 2, CELL - 4, CELL - 4, false);
    }

    int16_t px = OFFX + _px * CELL;
    int16_t py = OFFY + _py * CELL;
    d->fillRect(px + 1, py + 1, CELL - 2, CELL - 2, true);
}
