#!/usr/bin/env bash
# play.sh -- launch the GameOS PC viewer for the ESP32 version.
#
#   ./play.sh                 auto-detect ESP32 serial port and open viewer
#   ./play.sh /dev/ttyUSB0    use a specific serial port
#
# The viewer is a pygame window that renders the ESP32's streamed framebuffer
# and relays keyboard input back to the board. Game logic runs on the ESP32;
# this client is only a display + input terminal.
#
# Prerequisites:
#   pip install -r ../client/requirements.txt   (pyserial, pygame)
set -euo pipefail

cd "$(dirname "$0")"

if ! command -v python3 >/dev/null 2>&1; then
  echo "python3 is required." >&2
  exit 1
fi

if ! python3 -c 'import serial, pygame' >/dev/null 2>&1; then
  echo "pyserial and pygame are required. Install them with:" >&2
  echo "    pip install -r ../client/requirements.txt" >&2
  exit 1
fi

PORT="${1:-}"
if [[ -n "${PORT}" ]]; then
  exec python3 ../client/gameos_client.py --port "${PORT}"
else
  exec python3 ../client/gameos_client.py
fi
