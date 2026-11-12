#ifndef BREAKOUT_H
#define BREAKOUT_H

#include <Arduino.h>
#include "Teeny.h"

// Breakout with procedurally generated brick layouts.
class Breakout : public Game {
public:
    Breakout();
    const char *name() override { return "BREAKOUT"; }
    void begin() override;
    void update(Input *input) override;
    void draw(Display *disp) override;
    bool isOver() override { return _state == STATE_OVER; }
    uint32_t score() override { return _score; }
    uint32_t frameId() override { return _frameId; }

private:
    enum State { STATE_SERVE, STATE_PLAYING, STATE_OVER, STATE_CLEAR };

    static const int8_t PADDLE_W = 26;
    static const int8_t PADDLE_H = 4;
    static const int8_t BALL_SZ = 3;

    static const int8_t BRICK_W = 12;
    static const int8_t BRICK_H = 7;
    static const int8_t COLS = 10;      // 10 x 12 = 120 px wide
    static const int8_t MAX_ROWS = 5;

    int8_t _rows;                       // rows in current level
    uint8_t _level;
    uint8_t _brick[MAX_ROWS][COLS];     // 0 = empty, else hits remaining

    int8_t _px;                         // paddle x
    int8_t _bx, _by;                    // ball pos
    int8_t _pb, _py;                    // ball prev pos (for side detection)
    int8_t _bvx, _bvy;                  // ball velocity
    int8_t _lives;
    uint32_t _score;
    uint8_t _state;
    uint32_t _frameId;
    uint32_t _lastTick;
    uint32_t _moveDelay;

    void _changed() { _frameId++; }

    void _generateLevel();
    void _resetBall();
    void _launchBall();
    void _tick();
    void _movePaddle(int8_t dir);
    void _checkWalls();
    void _checkPaddle();
    void _checkBricks();
    void _drawBrick(Display *d, int8_t gx, int8_t gy);
    bool _anyBricksLeft();
};

#endif
