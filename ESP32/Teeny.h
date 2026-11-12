#ifndef TEENY_H
#define TEENY_H

#include <Arduino.h>
#include "Display.h"
#include "Input.h"

// Base class for a game.
class Game {
public:
    virtual ~Game() {}
    virtual const char *name() = 0;
    virtual void begin() = 0;
    virtual void update(Input *input) = 0;
    virtual void draw(Display *disp) = 0;     // assumes buffer already cleared
    virtual bool isOver() = 0;
    virtual uint32_t score() = 0;
    virtual uint32_t frameId() { return 0; }  // increments when visuals change
};

// Boot -> menu -> game -> game over.
#define TEENY_MAX_GAMES 8

#define STATE_BOOT     0
#define STATE_MENU     1
#define STATE_GAME     2
#define STATE_GAMEOVER 3

class Teeny {
public:
    Teeny(Display *disp, Input *input, int8_t mode);
    void begin();
    void addGame(Game *g);
    void update();
    void draw();

private:
    Display  *_disp;
    Input    *_input;
    int8_t    _mode;
    Game     *_games[TEENY_MAX_GAMES];
    uint8_t   _numGames;
    uint8_t   _state;
    uint8_t   _menuSel;
    uint8_t   _menuTop;
    uint8_t   _menuDir;
    Game     *_curGame;
    uint32_t  _bootTime;
    uint32_t  _lastSig;

    uint32_t _signature();
    void drawBoot();
    void drawMenu();
    void drawGameOver();
};

#endif
