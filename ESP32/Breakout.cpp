#include "Breakout.h"
#include "config.h"

// ---------------------------------------------------------------------------
// Breakout
// ---------------------------------------------------------------------------
Breakout::Breakout() {
    _rows = 0;
    _level = 0;
    _px = 0;
    _bx = _by = 0;
    _pb = _py = 0;
    _bvx = _bvy = 0;
    _lives = 3;
    _score = 0;
    _state = STATE_SERVE;
    _frameId = 0;
    _lastTick = 0;
    _moveDelay = 22UL;   // ~45 physics ticks per second
    for (int r = 0; r < MAX_ROWS; r++)
        for (int c = 0; c < COLS; c++)
            _brick[r][c] = 0;
}

void Breakout::_generateLevel() {
    _level++;
    _rows = (int8_t)random(3, 6);   // 3, 4 or 5 rows

    // Clear grid.
    for (int r = 0; r < MAX_ROWS; r++)
        for (int c = 0; c < COLS; c++)
            _brick[r][c] = 0;

    int placed = 0;
    for (int r = 0; r < _rows; r++) {
        for (int c = 0; c < COLS; c++) {
            // ~70% chance a brick exists at this cell.
            if (random(100) < 70) {
                uint8_t strength = 1;
                if (random(100) < 18) strength = 2;   // tougher brick
                if (random(100) < 4)  strength = 3;   // very tough brick
                _brick[r][c] = strength;
                placed++;
            }
        }
    }

    // Guarantee at least one brick so the level is playable.
    if (placed == 0) {
        _brick[_rows / 2][COLS / 2] = 1;
    }
}

void Breakout::_resetBall() {
    _bx = _px + PADDLE_W / 2 - BALL_SZ / 2;
    _by = SCREEN_H - PADDLE_H - BALL_SZ - 1;
    _bvx = 0;
    _bvy = 0;
    _pb = _bx; _py = _by;
}

void Breakout::_launchBall() {
    // Random horizontal direction, always moving up.
    _bvx = (random(2) == 0) ? -2 : 2;
    if (_bvx == 0) _bvx = 1;
    _bvy = -2;
    _pb = _bx; _py = _by;
}

void Breakout::begin() {
    randomSeed(micros());
    _level = 0;
    _lives = 3;
    _score = 0;
    _state = STATE_SERVE;
    _frameId = 0;
    _lastTick = millis();
    _px = (SCREEN_W - PADDLE_W) / 2;
    _generateLevel();
    _resetBall();
}

void Breakout::_movePaddle(int8_t dir) {
    int8_t nx = _px + dir * 3;
    if (nx < 0) nx = 0;
    if (nx > SCREEN_W - PADDLE_W) nx = SCREEN_W - PADDLE_W;
    if (nx != _px) {
        _px = nx;
        _changed();
        // Carry the ball with the paddle while serving.
        if (_state == STATE_SERVE) {
            _bx = _px + PADDLE_W / 2 - BALL_SZ / 2;
        }
    }
}

void Breakout::_checkWalls() {
    if (_bx <= 0) { _bx = 0; _bvx = abs(_bvx); _changed(); }
    if (_bx + BALL_SZ >= SCREEN_W) { _bx = SCREEN_W - BALL_SZ; _bvx = -abs(_bvx); _changed(); }
    if (_by <= 0) { _by = 0; _bvy = abs(_bvy); _changed(); }
}

void Breakout::_checkPaddle() {
    int8_t paddleY = SCREEN_H - PADDLE_H - 2;
    // Ball moving down and about to hit paddle top.
    if (_bvy > 0 && _py + BALL_SZ <= paddleY && _by + BALL_SZ > paddleY) {
        if (_bx + BALL_SZ > _px && _bx < _px + PADDLE_W) {
            _by = paddleY - BALL_SZ;
            _bvy = -abs(_bvy);
            // Angle based on where it hit the paddle (center = straight up).
            int8_t hit = (_bx + BALL_SZ / 2) - (_px + PADDLE_W / 2);
            if (hit < -6) { _bvx = -2; }
            else if (hit > 6) { _bvx = 2; }
            else if (hit == 0) { _bvx = 0; }
            _changed();
        }
    }
}

void Breakout::_checkBricks() {
    // Ball bbox: [_bx,_bx+BALL_SZ] x [_by,_by+BALL_SZ]
    int8_t bLeft = _bx, bRight = _bx + BALL_SZ;
    int8_t bTop = _by, bBottom = _by + BALL_SZ;

    for (int r = 0; r < _rows; r++) {
        for (int c = 0; c < COLS; c++) {
            uint8_t strength = _brick[r][c];
            if (strength == 0) continue;

            int8_t brLeft = c * BRICK_W + 2;
            int8_t brRight = brLeft + BRICK_W - 1;
            int8_t brTop = 3 + r * BRICK_H;
            int8_t brBottom = brTop + BRICK_H - 1;

            // Rect intersection.
            if (bRight <= brLeft || bLeft >= brRight + 1 ||
                bBottom <= brTop || bTop >= brBottom + 1) continue;

            // Side detection using previous ball position.
            if (_py + BALL_SZ <= brTop) { _bvy = -abs(_bvy); }
            else if (_py >= brBottom + 1) { _bvy = abs(_bvy); }
            else if (_pb + BALL_SZ <= brLeft) { _bvx = -abs(_bvx); }
            else if (_pb >= brRight + 1) { _bvx = abs(_bvx); }
            else { _bvy = -_bvy; }

            // Damage brick.
            if (strength > 1) {
                _brick[r][c] = strength - 1;
            } else {
                _brick[r][c] = 0;
            }
            _score += 10;
            _changed();
            return;   // one brick collision per tick
        }
    }
}

void Breakout::_tick() {
    _pb = _bx; _py = _by;
    _bx += _bvx;
    _by += _bvy;

    _checkWalls();
    _checkPaddle();
    _checkBricks();

    // Ball missed the paddle -> lose a life.
    if (_by > SCREEN_H) {
        _lives--;
        if (_lives <= 0) {
            _state = STATE_OVER;
        } else {
            _state = STATE_SERVE;
            _resetBall();
        }
        _changed();
    }

    // Level cleared.
    if (_state == STATE_PLAYING && _anyBricksLeft() == false) {
        _state = STATE_CLEAR;
        _changed();
    }
}

bool Breakout::_anyBricksLeft() {
    for (int r = 0; r < _rows; r++)
        for (int c = 0; c < COLS; c++)
            if (_brick[r][c] != 0) return true;
    return false;
}

void Breakout::update(Input *input) {
    uint32_t now = millis();

    // Paddle moves every frame (responsive).
    uint8_t dir = input->direction();
    if (dir == 3) _movePaddle(-1);   // left
    if (dir == 4) _movePaddle(1);    // right

    // Button actions.
    if (input->pressed()) {
        if (_state == STATE_SERVE) {
            _launchBall();
            _state = STATE_PLAYING;
            _changed();
        } else if (_state == STATE_CLEAR) {
            _generateLevel();
            _resetBall();
            _state = STATE_SERVE;
            _changed();
        } else if (_state == STATE_OVER) {
            begin();   // restart
        }
    }

    // Physics tick.
    if (_state == STATE_PLAYING && now - _lastTick >= _moveDelay) {
        _tick();
        _lastTick = now;
    }
}

void Breakout::_drawBrick(Display *d, int8_t gx, int8_t gy) {
    d->fillRect(gx * BRICK_W + 2, 3 + gy * BRICK_H, BRICK_W - 1, BRICK_H - 1, true);
}

void Breakout::draw(Display *d) {
    // Bricks.
    for (int r = 0; r < _rows; r++) {
        for (int c = 0; c < COLS; c++) {
            if (_brick[r][c] != 0) _drawBrick(d, c, r);
        }
    }

    // Paddle.
    d->fillRect(_px, SCREEN_H - PADDLE_H - 2, PADDLE_W, PADDLE_H, true);

    // Ball.
    d->fillRect(_bx, _by, BALL_SZ, BALL_SZ, true);

    // HUD.
    char buf[32];
    snprintf(buf, sizeof(buf), "SC:%-5u LV:%u  B:%u", _score, _level, _lives);
    d->drawText(2, 2, buf, true);

    // State overlays.
    if (_state == STATE_SERVE) {
        d->drawText(36, 26, "BREAKOUT", true);
        d->drawText(18, 42, "BTN=SERVE BALL", true);
    } else if (_state == STATE_CLEAR) {
        d->drawText(36, 26, "CLEARED!", true);
        d->drawText(18, 42, "BTN=NEXT LEVEL", true);
    } else if (_state == STATE_OVER) {
        d->drawText(36, 26, "GAME OVER", true);
        snprintf(buf, sizeof(buf), "SCORE: %u", _score);
        d->drawText(40, 42, buf, true);
        d->drawText(18, 54, "BTN=RESTART", true);
    }
}
