#!/usr/bin/env python3
"""
serial_console.py -- Teeny host console over USB CDC.

Reads the framed Teeny protocol from the console's serial port and prints
boot / status / log lines with per-packet counters. Also lets you type raw
payloads that are sent back to the console as PKT_CMD_REQ frames (Phase 0
ignores them, which itself proves the reverse link works).

Usage:
    python3 host/serial_console.py [PORT]
    e.g. python3 host/serial_console.py /dev/ttyACM0
"""

import sys
import time

import serial

sys.path.insert(0, __import__("os").path.dirname(__file__))
from teeny.protocol import FramedParser, PKT_NAMES  # noqa: E402

TYPE_TO_NAME_FMT = {
    0x01: "\033[1;33m[BOOT]\033[0m   ",
    0x02: "\033[1;32m[STATUS]\033[0m ",
    0x03: "\033[1;34m[LOG]\033[0m    ",
}


def find_port_args(argv):
    if len(argv) >= 2:
        return argv[1]
    return None


def main():
    port = find_port_args(sys.argv)
    if port is None:
        for candidate in ("/dev/ttyACM0", "/dev/ttyACM1"):
            try:
                s = serial.Serial(candidate, 115200, timeout=0.2, dsrdtr=True)
            except serial.SerialException:
                continue
            port = candidate
            print(f"opened {candidate}", file=sys.stderr)
            break
    if port is None:
        print("usage: serial_console.py [PORT]", file=sys.stderr)
        sys.exit(2)

    ser = serial.Serial(port, 115200, timeout=0.2, dsrdtr=True)
    parser = FramedParser()
    print("Teeny serial console. Ctrl-C to quit.", file=sys.stderr)

    try:
        while True:
            data = ser.read(4096)
            if data:
                for p in parser.feed(data):
                    if p.type in (0x20, 0x23, 0x24):
                        continue  # binary framebuffer traffic; the viewer handles it
                    label = TYPE_TO_NAME_FMT.get(p.type, f"[{p.name()}] ")
                    text = p.payload.decode("utf-8", "replace")
                    print(f"{label} seq={p.seq} {text}")
            else:
                time.sleep(0.01)
    except KeyboardInterrupt:
        pass
    finally:
        print(f"\ndrops={parser.drops} packets={parser.packets}", file=sys.stderr)


if __name__ == "__main__":
    main()