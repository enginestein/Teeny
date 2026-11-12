#ifndef ECHO_H
#define ECHO_H

#include <Arduino.h>
#include "Teeny.h"

// Sonar maze. The maze is dark: press the button to send out a ping that
// lights up nearby walls for a moment. Find the exit before time runs out.
class Echo : public Game {
public:
    Echo();
    const char *name() override { return "ECHO"; }
    void begin() override;
    void update(Input *input) override;
    void draw(Display *disp) override;
    bool isOver() override { return _over; }
    uint32_t score() override { return _score; }
    uint32_t frameId() override { return _frameId; }

private:
    static const int8_t COLS = 21;
    static const int8_t ROWS = 9;
    static const int8_t CELL = 6;
    static const int8_t OFFX = 1;
    static const int8_t OFFY = 9;

    static const uint16_t PING_MS = 900;
    static const uint16_t PING_COOLDOWN_MS = 1150;
    static const int8_t PING_RADIUS = 5;

    uint8_t _wall[ROWS][COLS];
    uint8_t _seen[ROWS][COLS];
    int8_t _stackX[COLS * ROWS];
    int8_t _stackY[COLS * ROWS];

    int8_t _px, _py;
    int8_t _ex, _ey;
    uint8_t _level;
    uint32_t _score;
    uint32_t _frameId;
    int32_t _timeLeft;
    int32_t _lastSec;
    uint32_t _lastTick;
    uint32_t _lastMove;
    uint32_t _lastPing;
    uint32_t _pingStart;
    bool _pingOn;
    bool _over;

    void _generate();
    void _reveal(int8_t cx, int8_t cy, int8_t r);
    bool _isWall(int8_t x, int8_t y) const;
};

#endif
