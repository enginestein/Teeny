#!/usr/bin/env python3
"""
framebuffer_viewer/viewer.py -- Teeny development display.

Connects to the RP2040's USB CDC link, reassembles RGB565 framebuffer
scanlines, and shows them in a window scaled by an integer factor.

Usage:
    python3 host/framebuffer_viewer/viewer.py [PORT] [SCALE]
"""

import os
import queue
import sys
import threading
import time

import numpy as np
import pygame
import serial

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from teeny.protocol import (  # noqa: E402
    DISPLAY_HEIGHT, DISPLAY_WIDTH,
    PKT_FRAME, PKT_FRAME_END, PKT_FRAME_RLE, PKT_INPUT, PKT_STATUS,
    FramedParser, decode_rle_row, encode,
)

BTN_RIGHT, BTN_LEFT, BTN_UP, BTN_DOWN, BTN_A, BTN_B, BTN_START, BTN_SELECT = range(8)

KEYMAP = {
    pygame.K_UP: BTN_UP, pygame.K_DOWN: BTN_DOWN,
    pygame.K_LEFT: BTN_LEFT, pygame.K_RIGHT: BTN_RIGHT,
    pygame.K_w: BTN_UP, pygame.K_s: BTN_DOWN,
    pygame.K_a: BTN_LEFT, pygame.K_d: BTN_RIGHT,
    pygame.K_j: BTN_A, pygame.K_k: BTN_B,
    pygame.K_RETURN: BTN_START,
    pygame.K_LSHIFT: BTN_SELECT, pygame.K_RSHIFT: BTN_SELECT,
}


def send_input_mask(ser, mask):
    if ser is not None:
        ser.write(encode(PKT_INPUT, bytes([mask])))
        ser.flush()


def rgb565_bytes_to_surface(fb_bytes):
    a = np.frombuffer(fb_bytes, dtype="<u2").astype(np.int32).reshape(
        DISPLAY_HEIGHT, DISPLAY_WIDTH
    )
    r = (a >> 11) & 0x1F
    g = (a >> 5) & 0x3F
    b = a & 0x1F
    rgb = np.dstack([(r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)])
    rgb = rgb.astype(np.uint8)
    return pygame.image.frombuffer(
        np.ascontiguousarray(rgb), (DISPLAY_WIDTH, DISPLAY_HEIGHT), "RGB"
    )


def find_port():
    for candidate in ("/dev/ttyACM0", "/dev/ttyACM1"):
        try:
            s = serial.Serial(candidate, 115200, timeout=0.2, dsrdtr=True)
            s.close()
            return candidate
        except serial.SerialException:
            continue
    return None


def open_serial(port):
    return serial.Serial(port, 115200, timeout=0.5, dsrdtr=True)


def reader_thread(ser, q, parser):
    while True:
        data = ser.read(4096)
        if data:
            for p in parser.feed(data):
                q.put(p)


class FrameAssembler:
    def __init__(self):
        self.rows = {}
        self.pixels = bytearray(DISPLAY_WIDTH * DISPLAY_HEIGHT * 2)

    def add_row(self, y, data):
        if len(data) == DISPLAY_WIDTH * 2 and 0 <= y < DISPLAY_HEIGHT:
            self.rows[y] = data

    def complete(self):
        if len(self.rows) != DISPLAY_HEIGHT:
            return False
        if set(self.rows.keys()) != set(range(DISPLAY_HEIGHT)):
            self.rows.clear()
            return False
        for y in range(DISPLAY_HEIGHT):
            off = y * DISPLAY_WIDTH * 2
            self.pixels[off:off + DISPLAY_WIDTH * 2] = self.rows[y]
        self.rows.clear()
        return True


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else find_port()
    scale = int(sys.argv[2]) if len(sys.argv) > 2 else 4
    if port is None:
        print("usage: viewer.py [PORT] [SCALE]  (no serial device found)", file=sys.stderr)
        sys.exit(2)

    ser = open_serial(port)
    print(f"connected to {port}", file=sys.stderr)
    parser = FramedParser()
    q = queue.Queue()
    t = threading.Thread(target=reader_thread, args=(ser, q, parser), daemon=True)
    t.start()

    pygame.init()
    win_w, win_h = DISPLAY_WIDTH * scale, DISPLAY_HEIGHT * scale
    screen = pygame.display.set_mode((win_w, win_h))
    pygame.display.set_caption("Teeny framebuffer")

    assembler = FrameAssembler()
    surface = pygame.Surface((DISPLAY_WIDTH, DISPLAY_HEIGHT))
    surface.fill((0, 0, 0))
    frame_count = 0
    last_frame_time = time.monotonic()
    fps = 0.0
    got_frame = False
    button_mask = 0

    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                return
            if event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                pygame.quit()
                return
            if event.type in (pygame.KEYDOWN, pygame.KEYUP):
                btn = KEYMAP.get(event.key)
                if btn is not None:
                    bit = 1 << btn
                    if event.type == pygame.KEYDOWN:
                        button_mask |= bit
                    else:
                        button_mask &= ~bit
                    send_input_mask(ser, button_mask)
                elif event.type == pygame.KEYDOWN:
                    print(
                        "controls: arrows/WASD=move  J=A  K=B  "
                        "Enter=START  Shift=SELECT  ESC=quit",
                        file=sys.stderr,
                    )

        try:
            while True:
                pkt = q.get_nowait()
                if pkt.type == PKT_FRAME and len(pkt.payload) == 1 + DISPLAY_WIDTH * 2:
                    assembler.add_row(pkt.payload[0], pkt.payload[1:])
                elif pkt.type == PKT_FRAME_RLE:
                    try:
                        assembler.add_row(pkt.payload[0], decode_rle_row(pkt.payload))
                    except ValueError:
                        pass
                elif pkt.type == PKT_FRAME_END and assembler.complete():
                    surface = rgb565_bytes_to_surface(assembler.pixels)
                    got_frame = True
                elif pkt.type == PKT_STATUS:
                    print(pkt.payload.decode("utf-8", "replace"), file=sys.stderr)
        except queue.Empty:
            pass

        now = time.monotonic()
        if got_frame:
            got_frame = False
            frame_count += 1
            if now - last_frame_time >= 1.0:
                fps = frame_count / (now - last_frame_time)
                frame_count = 0
                last_frame_time = now
            pygame.display.set_caption(
                f"Teeny framebuffer  {fps:.1f} fps  (drops={parser.drops})"
            )

        scaled = pygame.transform.scale(surface, (win_w, win_h))
        screen.blit(scaled, (0, 0))
        pygame.display.flip()
        time.sleep(0.005)


if __name__ == "__main__":
    main()
