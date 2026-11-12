#include "Orbit.h"
#include "config.h"

Orbit::Orbit() {
    for (int i = 0; i < MAX_AST; i++) _ast[i].active = false;
    _ang = 0.0f;
    _x = (float)CX;
    _y = (float)CY;
    _dir = 1;
    _ring = 1;
    _lastDir = 0;
    _lastMs = 0;
    _elapsed = 0;
    _nextSpawn = 0;
    _dodged = 0;
    _score = 0;
    _frameId = 0;
    _over = false;
}

void Orbit::begin() {
    randomSeed(micros());
    for (int i = 0; i < MAX_AST; i++) _ast[i].active = false;
    _ang = 0.0f;
    _dir = 1;
    _ring = 1;
    _lastDir = 0;
    _elapsed = 0;
    _dodged = 0;
    _score = 0;
    _frameId = 0;
    _over = false;
    _lastMs = millis();
    _nextSpawn = _lastMs + 800;
    _x = (float)CX + _radius();
    _y = (float)CY;
}

void Orbit::_spawn() {
    int slot = -1;
    for (int i = 0; i < MAX_AST; i++) {
        if (!_ast[i].active) { slot = i; break; }
    }
    if (slot < 0) return;

    Asteroid &a = _ast[slot];
    switch (random(4)) {
        case 0: a.x = (float)random(0, SCREEN_W); a.y = -6.0f; break;
        case 1: a.x = (float)random(0, SCREEN_W); a.y = (float)SCREEN_H + 6.0f; break;
        case 2: a.x = -6.0f; a.y = (float)random(0, SCREEN_H); break;
        default: a.x = (float)SCREEN_W + 6.0f; a.y = (float)random(0, SCREEN_H); break;
    }

    float tx = (float)CX + random(-16, 17);
    float ty = (float)CY + random(-16, 17);
    float dx = tx - a.x;
    float dy = ty - a.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) len = 1.0f;

    float speed = 58.0f + (_elapsed / 1000) * 1.8f;
    if (speed > 115.0f) speed = 115.0f;

    a.vx = dx / len * speed;
    a.vy = dy / len * speed;
    a.sz = (uint8_t)random(2, 5);
    a.active = true;
}

void Orbit::update(Input *input) {
    if (_over) return;

    uint32_t now = millis();
    uint32_t delta = now - _lastMs;
    _lastMs = now;
    if (delta > 100u) delta = 100u;
    _elapsed += delta;
    float dt = (float)delta / 1000.0f;

    uint8_t d = input->direction();
    if (d == 1 && _lastDir != 1 && _ring != 1) { _ring = 1; _frameId++; }
    if (d == 2 && _lastDir != 2 && _ring != 0) { _ring = 0; _frameId++; }
    _lastDir = d;

    if (input->pressed()) {
        _dir = -_dir;
        _frameId++;
    }

    float r = _radius();
    _ang += (float)_dir * (105.0f / r) * dt;
    while (_ang > 2.0f * PI) _ang -= 2.0f * PI;
    while (_ang < 0.0f) _ang += 2.0f * PI;
    _x = (float)CX + r * cosf(_ang);
    _y = (float)CY + r * sinf(_ang);

    if ((int32_t)(now - _nextSpawn) >= 0) {
        _spawn();
        uint32_t secs = _elapsed / 1000;
        uint32_t interval = secs < 30 ? 1150 - secs * 20 : 550;
        if (interval < 380) interval = 380;
        _nextSpawn = now + interval;
    }

    for (int i = 0; i < MAX_AST; i++) {
        Asteroid &a = _ast[i];
        if (!a.active) continue;
        a.x += a.vx * dt;
        a.y += a.vy * dt;

        float pcx = a.x - (float)CX;
        float pcy = a.y - (float)CY;
        if (pcx * pcx + pcy * pcy < (float)(PLANET_R + 1) * (PLANET_R + 1)) {
            a.active = false;
            continue;
        }

        if (a.x < -20.0f || a.x > SCREEN_W + 20.0f ||
            a.y < -20.0f || a.y > SCREEN_H + 20.0f) {
            a.active = false;
            _dodged++;
        }
    }

    for (int i = 0; i < MAX_AST; i++) {
        Asteroid &a = _ast[i];
        if (!a.active) continue;
        float dx = a.x - _x;
        float dy = a.y - _y;
        float rr = a.sz * 0.5f + 2.5f;
        if (dx * dx + dy * dy < rr * rr) {
            _over = true;
            _frameId++;
            return;
        }
    }

    _score = (_elapsed / 1000) * 10 + _dodged * 5;
    _frameId++;
}

void Orbit::draw(Display *d) {
    for (int ring = 0; ring < 2; ring++) {
        float r = ring ? 25.0f : 17.0f;
        for (int deg = 0; deg < 360; deg += 16) {
            float a = (float)deg * PI / 180.0f;
            int16_t x = (int16_t)((float)CX + r * cosf(a) + 0.5f);
            int16_t y = (int16_t)((float)CY + r * sinf(a) + 0.5f);
            if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H)
                d->drawPixel(x, y, true);
        }
    }

    for (int dy = -PLANET_R; dy <= PLANET_R; dy++) {
        int w = (int)sqrtf((float)(PLANET_R * PLANET_R - dy * dy));
        d->fillRect(CX - w, CY + dy, 2 * w + 1, 1, true);
    }

    for (int i = 0; i < MAX_AST; i++) {
        Asteroid &a = _ast[i];
        if (!a.active) continue;
        int16_t x0 = (int16_t)(a.x - a.sz * 0.5f);
        int16_t y0 = (int16_t)(a.y - a.sz * 0.5f);
        int16_t x1 = x0 + a.sz;
        int16_t y1 = y0 + a.sz;
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 > SCREEN_W) x1 = SCREEN_W;
        if (y1 > SCREEN_H) y1 = SCREEN_H;
        if (x1 > x0 && y1 > y0) d->fillRect(x0, y0, x1 - x0, y1 - y0, true);
    }

    int16_t sx = (int16_t)_x - 1;
    int16_t sy = (int16_t)_y - 1;
    if (sx < 0) sx = 0;
    if (sx > SCREEN_W - 3) sx = SCREEN_W - 3;
    if (sy < 0) sy = 0;
    if (sy > SCREEN_H - 3) sy = SCREEN_H - 3;
    d->fillRect(sx, sy, 3, 3, true);

    char buf[24];
    snprintf(buf, sizeof(buf), "SC:%u SEC:%u",
             (unsigned)_score, (unsigned)(_elapsed / 1000));
    d->drawText(2, 1, buf, true);
}
