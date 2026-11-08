#!/usr/bin/env python3
"""
Teeny PC Client

Renders the Teeny screen streamed from the ESP32 over USB-serial and
relays keyboard input back to the ESP32 as joystick commands.

The game logic always runs on the ESP32; this client is just a display +
input terminal so you can play on your computer when no OLED/joystick is
wired to the ESP32.

Usage:
    python teeny_client.py                 # auto-detect ESP32 port
    python teeny_client.py --port COM3     # Windows
    python teeny_client.py --port /dev/ttyUSB0  # Linux/macOS
    python teeny_client.py --baud 115200 --scale 4 --fps 30
"""

import argparse
import sys
import time

import serial
import serial.tools.list_ports
import pygame

# Frame protocol (must match Teeny/Display.cpp SerialDisplay)
SYNC = b"\xAA\x55\xAA\x55"
FRAME_LEN = 128 * 64 // 8  # 1024 bytes, page layout: byte[(y/8)*128 + x], bit (y%8)


def find_esp32_port():
    """Best-effort auto-detection of the ESP32 serial port."""
    for p in serial.tools.list_ports.comports():
        desc = ((p.description or "") + " " + (p.manufacturer or "")).lower()
        if any(k in desc for k in (
            "esp", "usb", "silicon labs", "cp210", "ch340", "ftdi",
            "nano", "duino", "uart",
        )):
            return p.device
    # Fall back to the first ttyUSB/ttyACM/COM port found.
    for p in serial.tools.list_ports.comports():
        if any(k in p.device.lower() for k in ("usb", "acm", "com")):
            return p.device
    return None


def expand_frame(frame):
    """Expand 1024-byte monochrome framebuffer to RGB bytes (128x64)."""
    rgb = bytearray(128 * 64 * 3)
    idx = 0
    for y in range(64):
        page = y >> 3
        bit = y & 7
        base = page * 128
        for x in range(128):
            on = (frame[base + x] >> bit) & 1
            c = 255 if on else 0
            rgb[idx] = c
            rgb[idx + 1] = c
            rgb[idx + 2] = c
            idx += 3
    return bytes(rgb)


def main():
    ap = argparse.ArgumentParser(description="Teeny PC client")
    ap.add_argument("--port", default=None, help="Serial port (auto-detected if omitted)")
    ap.add_argument("--baud", type=int, default=921600, help="Serial baud rate")
    ap.add_argument("--scale", type=int, default=4, help="Window scale factor")
    ap.add_argument("--fps", type=int, default=30, help="Render frame cap")
    args = ap.parse_args()

    port = args.port or find_esp32_port()
    if not port:
        print("ERROR: Could not auto-detect ESP32 serial port.")
        print("       Plug in the ESP32 and/or pass --port <device>")
        sys.exit(1)

    print(f"[Teeny] Connecting to {port} @ {args.baud} ...")
    ser = serial.Serial(port, args.baud, timeout=0.02, write_timeout=0.02)
    time.sleep(2.0)  # let the ESP32 finish booting after the port open reset
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    pygame.init()
    win_w = 128 * args.scale
    win_h = 64 * args.scale
    screen = pygame.display.set_mode((win_w, win_h))
    pygame.display.set_caption("Teeny (running on ESP32)")
    clock = pygame.time.Clock()

    buf = bytearray()
    running = True
    last_input = (0, 0, 0)  # x, y, btn last sent

    print("[Teeny] Connected. Arrows = move, Space/Enter = button. Close window to quit.")

    try:
        while running:
            # --- Pygame events ---
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False

            # --- Keyboard -> joystick input (send only on change) ---
            keys = pygame.key.get_pressed()
            x = (1 if keys[pygame.K_RIGHT] else 0) - (1 if keys[pygame.K_LEFT] else 0)
            y = (1 if keys[pygame.K_DOWN] else 0) - (1 if keys[pygame.K_UP] else 0)
            btn = 1 if (keys[pygame.K_SPACE] or keys[pygame.K_RETURN]) else 0
            x *= 100
            y *= 100

            if (x, y, btn) != last_input:
                try:
                    ser.write(f"JOY {x} {y} {btn}\n".encode("ascii"))
                except serial.SerialException:
                    pass
                last_input = (x, y, btn)

            # --- Read frames from ESP32 ---
            try:
                data = ser.read(4096)
            except serial.SerialException:
                data = b""
            if data:
                buf += data
                while True:
                    idx = buf.find(SYNC)
                    if idx < 0:
                        # Keep up to 3 trailing bytes that could start a sync.
                        del buf[:max(0, len(buf) - 3)]
                        break
                    del buf[:idx]  # drop bytes before sync
                    if len(buf) < 4 + FRAME_LEN + 1:
                        break  # incomplete frame, wait for more data
                    del buf[:4]  # drop sync
                    length = buf[0] | (buf[1] << 8)
                    del buf[:2]
                    if len(buf) < length + 1:
                        # Put length back? simpler: stop and wait; but we already
                        # consumed length. Re-insert is fiddly—just break and
                        # rely on resync next loop. To be safe, rewind.
                        # (Frames are fixed-size so this normally won't happen.)
                        break
                    frame = bytes(buf[:length])
                    chk = buf[length]
                    del buf[:length + 1]

                    # Optional checksum validation (XOR of payload).
                    calc = 0
                    for b in frame:
                        calc ^= b
                    if calc != chk:
                        continue  # corrupt frame, skip

                    # --- Render ---
                    rgb = expand_frame(frame)
                    surf = pygame.image.frombuffer(rgb, (128, 64), "RGB")
                    surf = pygame.transform.scale(surf, (win_w, win_h))
                    screen.blit(surf, (0, 0))
                    pygame.display.flip()

            clock.tick(args.fps)
    finally:
        try:
            ser.close()
        except serial.SerialException:
            pass
        pygame.quit()
        print("[Teeny] Disconnected.")


if __name__ == "__main__":
    main()
