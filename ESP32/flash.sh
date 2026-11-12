#!/usr/bin/env bash
# Build and upload Teeny over PlatformIO.
#   ./flash.sh                auto-detect the port
#   ./flash.sh /dev/ttyUSB0   use a specific port
set -euo pipefail

cd "$(dirname "$0")"

if ! command -v platformio >/dev/null 2>&1; then
  echo "platformio is required. Install it with:" >&2
  echo "    pip install platformio" >&2
  exit 1
fi

PORT="${1:-}"

echo "==> Building Teeny (ESP32)"
platformio run

if [[ -n "${PORT}" ]]; then
  echo "==> Uploading to ${PORT}"
  platformio run --target upload --upload-port "${PORT}"
else
  echo "==> Uploading (auto-detect port)"
  platformio run --target upload
fi

echo
echo "Done. Start the viewer with:  ./play.sh"
