#include "Mirror.h"
#include "config.h"

Mirror::Mirror() {
    memset(_cell, 0, sizeof(_cell));
    _cx = _cy = 0;
    _emitRow = _tgtRow = 1;
    _lastMove = _lastTick = 0;
    _frameId = 0;
    _score = 0;
    _timeLeft = 60000;
    _lastSec = -1;
    _over = false;
    _hit = false;
    _pathLen = 0;
}

void Mirror::begin() {
    randomSeed(micros());
    _score = 0;
    _frameId = 0;
    _timeLeft = 60000;
    _lastSec = -1;
    _over = false;
    _lastMove = millis();
    _lastTick = millis();
    _generate();
}

void Mirror::_generate() {
    memset(_cell, 0, sizeof(_cell));

    int8_t rows[3] = {1, 2, 3};
    for (int i = 2; i > 0; i--) {
        int j = (int)random(i + 1);
        int8_t t = rows[i];
        rows[i] = rows[j];
        rows[j] = t;
    }

    int k = (random(2) == 0) ? 1 : 2;
    int8_t c1 = (int8_t)random(2, COLS - 2);
    int8_t c2 = 0;
    if (k == 2) c2 = (int8_t)random(c1 + 1, COLS - 1);

    int8_t mx[4], my[4];
    uint8_t sol[4];
    int nm = 0;

    uint8_t m1 = (rows[1] > rows[0]) ? 2 : 1;
    mx[nm] = c1; my[nm] = rows[0]; sol[nm++] = m1;
    mx[nm] = c1; my[nm] = rows[1]; sol[nm++] = m1;
    _emitRow = rows[0];
    _tgtRow = rows[1];

    if (k == 2) {
        uint8_t m2 = (rows[2] > rows[1]) ? 2 : 1;
        mx[nm] = c2; my[nm] = rows[1]; sol[nm++] = m2;
        mx[nm] = c2; my[nm] = rows[2]; sol[nm++] = m2;
        _tgtRow = rows[2];
    }

    bool anyWrong = false;
    for (int i = 0; i < nm; i++) {
        uint8_t v = (random(2) == 0) ? 1 : 2;
        _cell[my[i]][mx[i]] = v;
        if (v != sol[i]) anyWrong = true;
    }
    if (!anyWrong) {
        _cell[my[0]][mx[0]] = (_cell[my[0]][mx[0]] == 1) ? 2 : 1;
    }

    _cx = c1;
    _cy = rows[0];

    _trace();
    for (int tries = 0; tries < 8 && _hit; tries++) {
        _cell[my[0]][mx[0]] = (_cell[my[0]][mx[0]] == 1) ? 2 : 1;
        _trace();
    }
}

void Mirror::_trace() {
    int8_t x = 0;
    int8_t y = _emitRow;
    uint8_t dir = 0;   // 0 right, 1 down, 2 left, 3 up
    _pathLen = 0;
    _hit = false;

    for (int step = 0; step < MAX_PATH_PTS; step++) {
        if (_pathLen < MAX_PATH_PTS) {
            _pathX[_pathLen] = x;
            _pathY[_pathLen] = y;
            _pathLen++;
        }

        if (x == COLS - 1 && y == _tgtRow) {
            _hit = true;
            break;
        }

        uint8_t m = _cell[y][x];
        if (m == 1) {
            if (dir == 0) dir = 3;
            else if (dir == 3) dir = 0;
            else if (dir == 1) dir = 2;
            else if (dir == 2) dir = 1;
        } else if (m == 2) {
            if (dir == 0) dir = 1;
            else if (dir == 1) dir = 0;
            else if (dir == 2) dir = 3;
            else if (dir == 3) dir = 2;
        }

        if (dir == 0) x++;
        else if (dir == 1) y++;
        else if (dir == 2) x--;
        else y--;

        if (x < 0 || x >= COLS || y < 0 || y >= ROWS) break;
    }
}

void Mirror::update(Input *input) {
    if (_over) return;

    uint32_t now = millis();
    _timeLeft -= (int32_t)(now - _lastTick);
    _lastTick = now;
    if (_timeLeft <= 0) {
        _timeLeft = 0;
        _over = true;
        _frameId++;
        return;
    }

    uint8_t dir = input->direction();
    if (dir != 0 && (uint32_t)(now - _lastMove) >= 125) {
        int8_t nx = _cx, ny = _cy;
        if (dir == 1) ny--;
        else if (dir == 2) ny++;
        else if (dir == 3) nx--;
        else if (dir == 4) nx++;
        if (nx >= 0 && nx < COLS && ny >= 0 && ny < ROWS) {
            _cx = nx;
            _cy = ny;
            _lastMove = now;
            _frameId++;
        }
    }

    if (input->pressed()) {
        uint8_t &m = _cell[_cy][_cx];
        if (m != 0) {
            m = (m == 1) ? 2 : 1;
            _trace();
            _frameId++;
            if (_hit) {
                _score += 100;
                _timeLeft += 8000;
                _generate();
                _frameId++;
            }
        }
    }

    int32_t sec = (_timeLeft + 999) / 1000;
    if (sec != _lastSec) {
        _lastSec = sec;
        _frameId++;
    }
}

void Mirror::draw(Display *d) {
    char buf[24];
    snprintf(buf, sizeof(buf), "MIRROR S:%u T:%02u",
             (unsigned)_score, (unsigned)((_timeLeft + 999) / 1000));
    d->drawText(2, 1, buf, true);

    for (int16_t i = 0; i < _pathLen; i++) {
        int16_t x = OFFX + _pathX[i] * CELL + CELL / 2;
        int16_t y = OFFY + _pathY[i] * CELL + CELL / 2;
        if (i == 0) {
            d->drawPixel(x, y, true);
            continue;
        }
        int16_t px = OFFX + _pathX[i - 1] * CELL + CELL / 2;
        int16_t py = OFFY + _pathY[i - 1] * CELL + CELL / 2;
        if (px == x) {
            int16_t lo = (py < y) ? py : y;
            int16_t hi = (py < y) ? y : py;
            for (int16_t yy = lo; yy <= hi; yy++) d->drawPixel(x, yy, true);
        } else {
            int16_t lo = (px < x) ? px : x;
            int16_t hi = (px < x) ? x : px;
            for (int16_t xx = lo; xx <= hi; xx++) d->drawPixel(xx, y, true);
        }
    }

    {
        int16_t x = OFFX + CELL / 2;
        int16_t y = OFFY + _emitRow * CELL + CELL / 2;
        d->fillRect(x - 2, y - 2, 4, 4, true);
    }

    {
        int16_t x = OFFX + (COLS - 1) * CELL + CELL / 2;
        int16_t y = OFFY + _tgtRow * CELL + CELL / 2;
        d->fillRect(x - 4, y - 4, 8, 8, true);
        d->fillRect(x - 2, y - 2, 4, 4, false);
        d->drawPixel(x, y, true);
    }

    for (int8_t y = 0; y < ROWS; y++) {
        for (int8_t x = 0; x < COLS; x++) {
            uint8_t m = _cell[y][x];
            if (!m) continue;
            int16_t cx = OFFX + x * CELL + CELL / 2;
            int16_t cy = OFFY + y * CELL + CELL / 2;
            for (int i = -4; i <= 4; i++) {
                if (m == 1) d->drawPixel(cx + i, cy - i, true);
                else d->drawPixel(cx + i, cy + i, true);
            }
        }
    }

    {
        int16_t x = OFFX + _cx * CELL;
        int16_t y = OFFY + _cy * CELL;
        d->fillRect(x, y, CELL, 1, true);
        d->fillRect(x, y + CELL - 1, CELL, 1, true);
        d->fillRect(x, y, 1, CELL, true);
        d->fillRect(x + CELL - 1, y, 1, CELL, true);
    }
}
