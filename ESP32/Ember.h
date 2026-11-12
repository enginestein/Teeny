#ifndef EMBER_H
#define EMBER_H

#include <Arduino.h>
#include "Teeny.h"

// Fire spreads across the grid every tick. Move with the joystick and press
// the button to splash a 3x3 patch of water. Put every fire out to clear a
// wave. If fire reaches your cell, it's over.
class Ember : public Game {
public:
    Ember();
    const char *name() override { return "EMBER"; }
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

    uint8_t _fire[ROWS][COLS];
    int8_t _px, _py;
    uint8_t _wave;
    uint32_t _score;
    uint32_t _frameId;
    uint32_t _lastMove;
    uint32_t _lastSpread;
    uint32_t _lastSplash;
    uint32_t _spreadMs;
    bool _over;

    int _fireCount() const;
    void _seedFire(int count);
    void _spread();
    void _splash();
};

#endif
