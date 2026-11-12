#include "Teeny.h"
#include "config.h"

Teeny::Teeny(Display *disp, Input *input, int8_t mode) {
    _disp = disp;
    _input = input;
    _mode = mode;
    _numGames = 0;
    _state = STATE_BOOT;
    _menuSel = 0;
    _menuTop = 0;
    _menuDir = 0;
    _curGame = nullptr;
    _bootTime = 0;
    _lastSig = 0;
}

void Teeny::begin() {
    _bootTime = millis();
    _disp->clear();
    _disp->display();
}

void Teeny::addGame(Game *g) {
    if (_numGames < TEENY_MAX_GAMES) {
        _games[_numGames++] = g;
    }
}

uint32_t Teeny::_signature() {
    uint32_t sig = ((uint32_t)_state << 24) | ((uint32_t)_menuSel << 16);
    if (_curGame) {
        sig |= _curGame->frameId();
        if (_curGame->isOver()) sig |= 1u;
    }
    return sig;
}

void Teeny::update() {
    uint32_t now = millis();

    switch (_state) {
        case STATE_BOOT:
            if (now - _bootTime > 1500UL) {
                _state = STATE_MENU;
                _menuSel = 0;
                _menuTop = 0;
                _menuDir = 0;
                _lastSig = 0;   // force menu redraw
            }
            break;

        case STATE_MENU: {
            uint8_t dir = _input->direction();
            if (dir != _menuDir) {
                if (dir == 1 && _menuSel > 0) { _menuSel--; _lastSig = 0; }
                if (dir == 2 && _menuSel + 1 < _numGames) { _menuSel++; _lastSig = 0; }
                _menuDir = dir;
            }
            if (_menuSel < _menuTop) _menuTop = _menuSel;
            if (_menuSel >= _menuTop + 4) _menuTop = _menuSel - 3;
            if (_input->pressed() && _numGames > 0) {
                _curGame = _games[_menuSel];
                _curGame->begin();
                _state = STATE_GAME;
                _lastSig = 0;
            }
            break;
        }

        case STATE_GAME:
            _curGame->update(_input);
            if (_curGame->isOver()) {
                _state = STATE_GAMEOVER;
                _lastSig = 0;
            }
            break;

        case STATE_GAMEOVER:
            if (_input->pressed()) {
                _curGame = nullptr;
                _state = STATE_MENU;
                _menuDir = 0;
                _lastSig = 0;
            }
            break;
    }
}

void Teeny::draw() {
    uint32_t sig = _signature();
    if (sig == _lastSig) return;     // nothing changed -> skip redraw
    _lastSig = sig;

    _disp->clear();

    switch (_state) {
        case STATE_BOOT:   drawBoot();   break;
        case STATE_MENU:   drawMenu();   break;
        case STATE_GAME:   _curGame->draw(_disp); break;
        case STATE_GAMEOVER: drawGameOver(); break;
    }
}

void Teeny::drawBoot() {
    _disp->drawText(49, 26, "TEENY", true);
    _disp->drawText(38, 42, "BOOTING...", true);
    if (_mode == MODE_REMOTE) {
        _disp->drawText(22, 54, "REMOTE MODE (USB)", true);
    } else {
        _disp->drawText(26, 54, "HARDWARE MODE", true);
    }
}

void Teeny::drawMenu() {
    _disp->drawText(2, 1, "TEENY", true);

    for (uint8_t row = 0; row < 4; row++) {
        uint8_t i = _menuTop + row;
        if (i >= _numGames) break;
        bool sel = (i == _menuSel);
        int16_t y = 12 + row * 12;
        if (sel) {
            _disp->fillRect(2, y - 2, 124, 10, true);
            _disp->drawText(6, y, _games[i]->name(), false);
        } else {
            _disp->drawText(6, y, _games[i]->name(), true);
        }
    }
    if (_menuTop > 0) _disp->drawText(120, 1, "^", true);
    if (_menuTop + 4 < _numGames) _disp->drawText(120, 56, "v", true);
}

void Teeny::drawGameOver() {
    _disp->drawText(34, 20, "GAME OVER", true);
    char buf[32];
    snprintf(buf, sizeof(buf), "SCORE: %u", _curGame ? _curGame->score() : 0);
    _disp->drawText(34, 36, buf, true);
    _disp->drawText(18, 52, "BTN=BACK TO MENU", true);
}
