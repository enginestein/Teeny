#!/usr/bin/env bash
# flash.sh -- flash build/teeny.uf2 onto the SparkFun Thing Plus RP2040.
#
# Two paths:
#   1. Teeny already installed  -> send a framed PKT_CMD_REQ("dfu"); the
#      firmware calls reset_usb_boot() and appears as RPI-RP2.
#   2. DeckOS still installed    -> send plain text "dfu" to its CDC shell.
#
# It then copies the UF2 file onto the bootloader's USB mass-storage drive.

set -eu

HERE="$(cd "$(dirname "$0")/.." && pwd)"
UF2="${HERE}/build/teeny.uf2"

if [[ ! -f "${UF2}" ]]; then
    echo "error: ${UF2} not found; run: cmake -B build && cmake --build build"
    exit 1
fi

PORT="${1:-/dev/ttyACM0}"

# Try the framed Teeny dfu command first.
python3 - "${PORT}" << 'PYEOF'
import serial, sys, time

port = sys.argv[1]

def cksum(payload):
    c = 0
    for b in payload:
        c ^= b
    return c

def frame(payload):
    n = len(payload)
    head = bytes([0x47, 0x10, (n >> 8) & 0xFF, n & 0xFF, 0x00])
    body = bytes(payload)
    return head + body + bytes([cksum(bytes([0x10, (n >> 8) & 0xFF, n & 0xFF, 0x00]) + body)])

# Lite liveness probe: frame magic 0x47 'reserved byte' is a valid Teeny frame
# header; a Teeny console answers a CMD_REQ probe with a CMD_RSP echo.
try:
    s = serial.Serial(port, 115200, timeout=1.0, dsrdtr=True)
except serial.SerialException as e:
    print(f"cannot open {port}: {e}")
    sys.exit(1)

time.sleep(0.3)
s.reset_input_buffer()

# Probe: send CMD_REQ "ping" (4 bytes); a Teeny console echoes it back as a
# CMD_RSP (0x11) frame. Heartbeat frames may interleave, so scan for the
# echo signature 0x47 0x11 rather than any 0x47-leading byte.
probe = frame(b"ping")
s.write(probe)
s.flush()
# The console floods the pipe with framebuffer scanlines while it runs, so a
# single read() may catch nothing but video rows. Drain and scan for the echo
# signature across a generous window instead.
deadline = time.time() + 2.0
echo_seen = False
while time.time() < deadline:
    chunk = s.read(4096)
    if not chunk:
        continue
    if b"\x47\x11" in chunk:
        echo_seen = True
        break
if echo_seen:
    print("Teeny detected; sending dfu")
    s.write(frame(b"dfu"))
    time.sleep(0.5)
else:
    print("no Teeny reply; assuming DeckOS, sending raw dfu")
    s.write(b"\rdfu\r")
    time.sleep(0.2)
PYEOF

# Wait for the bootloader's USB mass-storage drive.
DRIVE=""
for _ in $(seq 1 40); do
    DRIVE="$(find /media -maxdepth 3 -iname 'RPI-RP2' -type d 2>/dev/null | head -n1 || true)"
    if [[ -n "${DRIVE}" ]]; then
        break
    fi
    sleep 1
done

if [[ -z "${DRIVE}" ]]; then
    echo "error: RPI-RP2 drive did not appear (is the board in bootloader?)"
    exit 1
fi
echo "bootloader on ${DRIVE}"

sleep 1
cp "${UF2}" "${DRIVE}/"
echo "copied $(basename "${UF2}") -> ${DRIVE}"
sync || true
echo "flashing... device will reset automatically"