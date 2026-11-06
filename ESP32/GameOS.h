#ifndef GAMEOS_H
#define GAMEOS_H

#include <Arduino.h>
#include "Display.h"
#include "Input.h"

// ---------------------------------------------------------------------------
// Game interface.  Every GameOS game derives from this.
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// GameOS core: boot screen -> menu -> game -> game-over state machine.
// ---------------------------------------------------------------------------
#define GAMEOS_MAX_GAMES 4

#define STATE_BOOT     0
#define STATE_MENU     1
#define STATE_GAME     2
#define STATE_GAMEOVER 3

class GameOS {
public:
    GameOS(Display *disp, Input *input, int8_t mode);
    void begin();
    void addGame(Game *g);
    void update();
    void draw();

private:
    Display  *_disp;
    Input    *_input;
    int8_t    _mode;
    Game     *_games[GAMEOS_MAX_GAMES];
    uint8_t   _numGames;
    uint8_t   _state;
    uint8_t   _menuSel;
    Game     *_curGame;
    uint32_t  _bootTime;
    uint32_t  _lastSig;

    uint32_t _signature();
    void drawBoot();
    void drawMenu();
    void drawGameOver();
};

#endif
