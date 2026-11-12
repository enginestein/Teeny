#!/usr/bin/env bash
# Open the PC viewer for the ESP32.
#   ./play.sh                 auto-detect the port
#   ./play.sh /dev/ttyUSB0    use a specific port
#
# Needs pyserial and pygame: pip install -r ../client/requirements.txt
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
  exec python3 ../client/teeny_client.py --port "${PORT}"
else
  exec python3 ../client/teeny_client.py
fi
