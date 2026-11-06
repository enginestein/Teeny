#ifndef SNAKE_H
#define SNAKE_H

#include <Arduino.h>
#include "GameOS.h"

// ---------------------------------------------------------------------------
// Snake game for GameOS.
// Grid cells are 4x4 pixels -> 32 columns x 16 rows on a 128x64 screen.
// ---------------------------------------------------------------------------
class Snake : public Game {
public:
    Snake();
    const char *name() override { return "SNAKE"; }
    void begin() override;
    void update(Input *input) override;
    void draw(Display *disp) override;
    bool isOver() override { return _gameOver; }
    uint32_t score() override { return _score; }
    uint32_t frameId() override { return _frameId; }

private:
    static const int8_t CELL = 4;
    static const int8_t COLS = SCREEN_W / CELL;   // 32
    static const int8_t ROWS = SCREEN_H / CELL;   // 16

    int8_t _sx[COLS * ROWS];
    int8_t _sy[COLS * ROWS];
    uint8_t _len;
    int8_t _fx, _fy;        // food
    int8_t _dx, _dy;        // current direction
    uint8_t _pendDx, _pendDy; // pending direction (applied at next tick)
    uint32_t _score;
    uint32_t _lastMove;
    uint32_t _moveDelay;
    uint32_t _frameId;
    bool _gameOver;

    void _placeFood();
    void _move();
    void _drawCell(Display *d, int8_t gx, int8_t gy, bool color);
};

#endif
