#ifndef ORBIT_H
#define ORBIT_H

#include <Arduino.h>
#include "Teeny.h"

// One-button orbit dodger. You circle a planet on one of two rings while
// asteroids fall in from every side. Press the button to reverse direction.
class Orbit : public Game {
public:
    Orbit();
    const char *name() override { return "ORBIT"; }
    void begin() override;
    void update(Input *input) override;
    void draw(Display *disp) override;
    bool isOver() override { return _over; }
    uint32_t score() override { return _score; }
    uint32_t frameId() override { return _frameId; }

private:
    static const int CX = 64;
    static const int CY = 38;
    static const int PLANET_R = 7;
    static const int MAX_AST = 14;

    struct Asteroid {
        float x, y, vx, vy;
        uint8_t sz;
        bool active;
    };

    Asteroid _ast[MAX_AST];
    float _ang;
    float _x, _y;
    int8_t _dir;
    int8_t _ring;
    int8_t _lastDir;
    uint32_t _lastMs;
    uint32_t _elapsed;
    uint32_t _nextSpawn;
    uint32_t _dodged;
    uint32_t _score;
    uint32_t _frameId;
    bool _over;

    float _radius() const { return _ring ? 25.0f : 17.0f; }
    void _spawn();
};

#endif
