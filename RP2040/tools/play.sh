#!/usr/bin/env bash
# play.sh -- flash (optional) + launch the Teeny PC viewer with one command.
#
#   ./tools/play.sh            just open the viewer (kills stale ones first)
#   ./tools/play.sh --flash    build + flash the RP2040, then open the viewer
#   ./tools/play.sh --bg       detach the viewer and return to your prompt
#   ./tools/play.sh --scale 2  override the window zoom (default 4)
#
# The viewer runs in the foreground by default (ESC quits). Exit any already
# running viewer window before re-flashing: two processes on /dev/ttyACM0
# starve each other and the window freezes on a stale frame.

set -u

HERE="$(cd "$(dirname "$0")/.." && pwd)"
FLASH=0
BG=0
SCALE=4
PORT=""

args=("$@")
i=0
while [[ $i -lt $# ]]; do
    arg="${args[$i]}"
    case "${arg}" in
        --flash) FLASH=1 ;;
        --bg) BG=1 ;;
        --scale)
            i=$((i + 1))
            if [[ $i -ge $# ]]; then
                echo "error: --scale needs a value" >&2
                exit 1
            fi
            SCALE="${args[$i]}"
            ;;
        --scale=*) SCALE="${arg#--scale=}" ;;
        /*) PORT="${arg}" ;;
        *) echo "unknown argument: ${arg}" >&2; exit 1 ;;
    esac
    i=$((i + 1))
done

# Kill any stale viewer from a previous session (safe pattern: the literal
# "[f]" prevents this script's own process list line from matching).
STALE="$(ps -eo pid,args | grep '[f]ramebuffer_viewer/viewer.py' | awk '{print $1}')"
if [[ -n "${STALE}" ]]; then
    echo "killing stale viewer process(es): ${STALE}"
    kill -9 ${STALE} 2>/dev/null || true
    sleep 1
fi

if [[ "${FLASH}" == 1 ]]; then
    echo "==> building..."
    cmake --build "${HERE}/build" || { echo "build failed" >&2; exit 1; }
    echo "==> flashing..."
    timeout 90 "${HERE}/tools/flash.sh" || { echo "flash failed" >&2; exit 1; }
    # The board re-enumerates after the flash reset; give the port a moment.
    for _ in $(seq 1 10); do
        [[ -e /dev/ttyACM0 ]] && break
        sleep 1
    done
fi

if [[ -z "${PORT}" ]]; then
    PORT="/dev/ttyACM0"
fi

echo "==> launching viewer (ESC to quit)"
if [[ "${BG}" == 1 ]]; then
    cd "${HERE}"
    setsid nohup python3 host/framebuffer_viewer/viewer.py "${PORT}" "${SCALE}" \
        >"${HERE}/.viewer.log" 2>&1 < /dev/null &
    disown
    echo "viewer detached; log at .viewer.log"
else
    cd "${HERE}"
    exec python3 host/framebuffer_viewer/viewer.py "${PORT}" "${SCALE}"
fi