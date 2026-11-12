#ifndef MIRROR_H
#define MIRROR_H

#include <Arduino.h>
#include "Teeny.h"

// Laser puzzle. A beam leaves the emitter on the left. Rotate the mirrors
// with the button until the beam reaches the target on the right edge.
// Solve as many as you can before the clock runs out.
class Mirror : public Game {
public:
    Mirror();
    const char *name() override { return "MIRROR"; }
    void begin() override;
    void update(Input *input) override;
    void draw(Display *disp) override;
    bool isOver() override { return _over; }
    uint32_t score() override { return _score; }
    uint32_t frameId() override { return _frameId; }

private:
    static const int8_t COLS = 11;
    static const int8_t ROWS = 5;
    static const int8_t CELL = 10;
    static const int8_t OFFX = 9;
    static const int8_t OFFY = 12;
    static const int MAX_PATH_PTS = 400;

    // Cell contents: 0 = empty, 1 = '/', 2 = '\'
    uint8_t _cell[ROWS][COLS];
    int8_t _cx, _cy;
    int8_t _emitRow, _tgtRow;
    uint32_t _lastMove;
    uint32_t _lastTick;
    uint32_t _frameId;
    uint32_t _score;
    int32_t _timeLeft;
    int32_t _lastSec;
    bool _over;
    bool _hit;

    int8_t _pathX[MAX_PATH_PTS];
    int8_t _pathY[MAX_PATH_PTS];
    int16_t _pathLen;

    void _generate();
    void _trace();
};

#endif
