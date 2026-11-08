#!/usr/bin/env python3
"""Headless end-to-end check of the Teeny launcher + 8 games.

Drives the console through input frames (same wire format the viewer uses) and
asserts each state transition using the kernel's 1 Hz "app: ..." status line,
then verifies each game actually renders by inspecting one clean framebuffer.

Usage: tools/verify_games.py [PORT]
"""
import os
import re
import sys
import time

import serial

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "host"))
from teeny.protocol import (  # noqa: E402
    DISPLAY_HEIGHT,
    DISPLAY_WIDTH,
    PKT_FRAME,
    PKT_FRAME_END,
    PKT_FRAME_RLE,
    PKT_STATUS,
    FramedParser,
    decode_rle_row,
    encode,
    PKT_INPUT,
)

BTN = {"RIGHT": 0, "LEFT": 1, "UP": 2, "DOWN": 3, "A": 4, "B": 5, "START": 6, "SELECT": 7}
MASK = {k: 1 << v for k, v in BTN.items()}

PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
parser = FramedParser()


def pump(ser, seconds, sink=None):
    end = time.time() + seconds
    while time.time() < end:
        data = ser.read(65536)
        if not data:
            continue
        for p in parser.feed(data):
            if sink:
                sink(p)


def wait_for_app(ser, needle, timeout=6.0):
    """Block until a PKT_STATUS payload contains 'app: <needle>'. Returns it."""
    target = f"app: {needle}"
    end = time.time() + timeout
    while time.time() < end:
        got = []
        def sink(p):
            if p.type == PKT_STATUS:
                text = p.payload.decode("utf-8", "replace")
                if target in text:
                    got.append(text)
        pump(ser, 0.5, sink)
        if got:
            return got[-1]
    return None


def tap(ser, key, ms=140):
    ser.write(encode(PKT_INPUT, bytes([MASK[key]])))
    ser.flush()
    time.sleep(ms / 1000)
    ser.write(encode(PKT_INPUT, b"\x00"))
    ser.flush()


def launch(ser, idx, name):
    """Navigate to plate `idx` of the launcher and start it. The launcher
    selection is checked against the 1 Hz status line after every input, and a
    missed launch edge gets one extra A tap."""
    goto_sel(ser, idx)
    tap(ser, "A")
    if wait_for_app(ser, name, 6.0) is not None:
        return True
    tap(ser, "A")
    return wait_for_app(ser, name, 6.0) is not None


def goto_menu(ser):
    """Make sure the launcher is showing, wherever we are."""
    tap(ser, "SELECT")
    if wait_for_app(ser, "menu sel=", 3.0) is not None:
        return True
    tap(ser, "SELECT")
    return wait_for_app(ser, "menu sel=", 3.0) is not None


def goto_sel(ser, n, timeout=30.0):
    """Walk the launcher cursor to selection n, confirming via the 1 Hz status
    line after every tap (status cadence naturally paces the buttons)."""
    end = time.time() + timeout
    while time.time() < end:
        m = re.search(r"menu sel=(\d+)",
                      wait_for_app(ser, "menu sel=", 1.0) or "")
        cur = int(m.group(1)) if m else None
        if cur is None:
            continue
        if cur == n:
            return True
        tap(ser, "UP" if (cur - n) % 8 <= (n - cur) % 8 else "DOWN")
    return False


def grab_frame(ser, timeout=5.0):
    """Return the first complete framebuffer after a short settle, so games
    that animate (e.g. INVADERS marching) are sampled at their fresh state
    rather than some seconds in, without catching stale pre-launch frames."""
    settle = time.time() + 0.8
    while time.time() < settle:
        if ser.read(65536):
            continue
    rows = {}
    end = time.time() + timeout
    while time.time() < end:
        data = ser.read(65536)
        if not data:
            continue
        for p in parser.feed(data):
            if p.type == PKT_FRAME and len(p.payload) == 1 + DISPLAY_WIDTH * 2:
                rows[p.payload[0]] = p.payload[1:]
            elif p.type == PKT_FRAME_RLE:
                try:
                    rows[p.payload[0]] = decode_rle_row(p.payload)
                except ValueError:
                    rows.clear()
            elif p.type == PKT_FRAME_END:
                if len(rows) != DISPLAY_HEIGHT or set(rows) != set(range(DISPLAY_HEIGHT)):
                    rows.clear()
                else:
                    px = bytearray(DISPLAY_WIDTH * DISPLAY_HEIGHT * 2)
                    for y in range(DISPLAY_HEIGHT):
                        px[y * DISPLAY_WIDTH * 2:(y + 1) * DISPLAY_WIDTH * 2] = rows[y]
                    rows.clear()
                    return bytes(px)
    return None


def pxat(px, x, y):
    off = (y * DISPLAY_WIDTH + x) * 2
    return px[off] | (px[off + 1] << 8)


def count_row(px, y, color):
    return sum(1 for x in range(DISPLAY_WIDTH) if pxat(px, x, y) == color)


def count_rect_nonblack(px, x0, y0, x1, y1):
    n = 0
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if pxat(px, x, y) != 0x0000:
                n += 1
    return n


def main():
    ser = serial.Serial(PORT, 115200, timeout=0.3, dsrdtr=True)
    time.sleep(0.5)
    ok = True

    def check(label, cond):
        nonlocal ok
        print(("PASS" if cond else "FAIL") + f"  {label}")
        ok = ok and cond

    check("boots into menu", wait_for_app(ser, "menu sel=0") is not None)

    tap(ser, "A")
    check("A launches SNAKE", wait_for_app(ser, "SNAKE ") is not None)

    tap(ser, "SELECT")
    check("SELECT returns to menu", wait_for_app(ser, "menu sel=0") is not None)

    check("breaks down to BREAKOUT", launch(ser, 1, "BREAKOUT"))
    px = grab_frame(ser)
    if px:
        reds = count_row(px, 22, 0xF800)
        paddle = count_row(px, 138, 0xFFFF)
        check(f"BREAKOUT renders (bricks red={reds} paddle={paddle})",
              reds >= 5 and paddle >= 5)
    else:
        check("BREAKOUT renders (frame)", False)

    tap(ser, "SELECT")
    check("back to menu", wait_for_app(ser, "menu sel=1") is not None)

    check("launches INVADERS", launch(ser, 2, "INVADERS"))
    px = grab_frame(ser)
    if px:
        invs = count_row(px, 26, 0x07E0)
        turret = count_row(px, 137, 0xFFFF)
        check(f"INVADERS renders (grid={invs} turret={turret})", invs >= 15 and turret >= 4)
    else:
        check("INVADERS renders (frame)", False)

    tap(ser, "SELECT")
    check("final back to menu", wait_for_app(ser, "menu sel=2") is not None)

    check("launches PONG", launch(ser, 3, "PONG"))
    px = grab_frame(ser)
    if px:
        p1 = pxat(px, 8, 70) == 0xFFFF
        p2 = pxat(px, 151, 70) == 0xFFFF
        bl = pxat(px, 77, 69) == 0xFFFF
        check(f"PONG renders (paddles+ball={p1 and p2 and bl})",
              p1 and p2 and bl)
    else:
        check("PONG renders (frame)", False)

    tap(ser, "SELECT")
    check("menu after PONG", wait_for_app(ser, "menu sel=3") is not None)

    check("launches TETRIS", launch(ser, 4, "TETRIS"))
    px = grab_frame(ser)
    if px:
        border = pxat(px, 44, 17) == 0x3186  # DKGRAY playfield border
        piece = count_rect_nonblack(px, 45, 23, 115, 40) >= 4
        check(f"TETRIS renders (border={border} piece={piece})", border and piece)
    else:
        check("TETRIS renders (frame)", False)

    tap(ser, "SELECT")
    check("menu after TETRIS", wait_for_app(ser, "menu sel=4") is not None)

    check("launches RUNNER", launch(ser, 5, "RUNNER"))
    px = grab_frame(ser)
    if px:
        # grass surface row + cyan hero (hero starts at camped left edge)
        grass = count_row(px, 114, 0x0540)
        hero = count_row(px, 102, 0x07FF)
        check(f"RUNNER renders (grass={grass} hero={hero})",
              grass >= 5 and hero >= 5)
    else:
        check("RUNNER renders (frame)", False)

    tap(ser, "SELECT")
    check("menu after RUNNER", wait_for_app(ser, "menu sel=5") is not None)

    check("launches PACMATE", launch(ser, 6, "PACMATE"))
    px = grab_frame(ser)
    if px:
        wall = count_row(px, 1, 0x001F)           # blue maze top border
        pyellow = sum(1 for y in range(DISPLAY_HEIGHT) for x in range(DISPLAY_WIDTH)
                      if pxat(px, x, y) == 0xFFE0)
        ghost = sum(1 for y in range(DISPLAY_HEIGHT) for x in range(DISPLAY_WIDTH)
                    if pxat(px, x, y) in (0xFC00, 0x07FF))
        check(f"PACMATE renders (wall={wall} player={pyellow} ghost={ghost})",
              wall >= 100 and pyellow >= 15 and ghost >= 6)
    else:
        check("PACMATE renders (frame)", False)

    tap(ser, "SELECT")
    check("menu after PACMATE", wait_for_app(ser, "menu sel=6") is not None)

    check("launches FLAPPY", launch(ser, 7, "FLAPPY"))
    px = grab_frame(ser)
    if px:
        bird = count_rect_nonblack(px, 30, 55, 40, 75)
        ground = count_row(px, 133, 0x07E0)
        check(f"FLAPPY renders (bird={bird} ground={ground})",
              bird >= 15 and ground >= 80)
    else:
        check("FLAPPY renders (frame)", False)

    tap(ser, "SELECT")
    check("menu after FLAPPY", wait_for_app(ser, "menu sel=7") is not None)

    ser.close()
    print("RESULT:", "PASS" if ok else "FAIL")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()