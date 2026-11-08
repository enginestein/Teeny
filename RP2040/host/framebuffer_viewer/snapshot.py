#!/usr/bin/env python3
"""
snapshot.py -- grab one complete framebuffer frame from the RP2040 and dump it.

Headless check used to verify the framebuffer transport byte-for-byte without
a GUI. Writes the raw 160x144 RGB565 frame to a PPM (P6, RGB, 4x nearest
scaled) and prints basic content stats so the pattern can be sanity-checked.

Usage:
    python3 host/framebuffer_viewer/snapshot.py [PORT] [OUT.ppm]
"""

import os
import sys
import time

import serial

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from teeny.protocol import (  # noqa: E402
    DISPLAY_HEIGHT,
    DISPLAY_WIDTH,
    PKT_FRAME,
    PKT_FRAME_END,
    PKT_FRAME_RLE,
    FramedParser,
    decode_rle_row,
)


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
    out = sys.argv[2] if len(sys.argv) > 2 else "snapshot.ppm"
    scale = int(sys.argv[3]) if len(sys.argv) > 3 else 4

    ser = serial.Serial(port, 115200, timeout=0.5, dsrdtr=True)
    parser = FramedParser()
    rows = {}
    deadline = time.time() + 10

    print(f"waiting for a complete frame on {port}...", file=sys.stderr)
    while time.time() < deadline:
        data = ser.read(8192)
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
                    pixels = bytearray(DISPLAY_WIDTH * DISPLAY_HEIGHT * 2)
                    for y in range(DISPLAY_HEIGHT):
                        off = y * DISPLAY_WIDTH * 2
                        pixels[off : off + DISPLAY_WIDTH * 2] = rows[y]
                    write_ppm(out, pixels, scale)
                    report(pixels)
                    print(f"wrote {out} (drops={parser.drops})", file=sys.stderr)
                    ser.close()
                    return
    print("no complete frame received (is the console connected?)", file=sys.stderr)
    sys.exit(1)


def write_ppm(path, pixels, scale):
    w, h = DISPLAY_WIDTH, DISPLAY_HEIGHT
    # Convert RGB565 -> RGB888.
    rgb = bytearray()
    for i in range(0, len(pixels), 2):
        v = pixels[i] | (pixels[i + 1] << 8)
        r = (v >> 11) & 0x1F
        g = (v >> 5) & 0x3F
        b = v & 0x1F
        rgb += bytes([(r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)])
    with open(path, "wb") as f:
        f.write(f"P6\n{w * scale} {h * scale}\n255\n".encode())
        for y in range(h):
            row = rgb[y * w * 3 : (y + 1) * w * 3]
            for _ in range(scale):
                for x in range(w):
                    for _ in range(scale):
                        f.write(row[x * 3 : x * 3 + 3])


def report(pixels):
    n_px = len(pixels) // 2
    vals = set()
    for i in range(0, len(pixels), 2):
        v = pixels[i] | (pixels[i + 1] << 8)
        vals.add(v)
        if len(vals) > 16:
            break
    print(f"frame: {n_px} px, distinct-colour sample: {[hex(v) for v in sorted(vals)[:16]]}", file=sys.stderr)


if __name__ == "__main__":
    main()