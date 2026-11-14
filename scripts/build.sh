#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
WATCOM=${WATCOM:-"$HOME/opt/watcom"}
export WATCOM
export PATH="$WATCOM/binl:$PATH"
export INCLUDE="$WATCOM/h"
export LIB="$WATCOM/lib286"
export EDPATH="$WATCOM/eddat"
for tool in wcl wcc wlink; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Missing $tool: set WATCOM to an Open Watcom installation." >&2
        exit 1
    fi
done
if [ ! -d "$INCLUDE" ] || [ ! -d "$LIB" ]; then
    echo "Incomplete Open Watcom installation: expected h/ and lib286/." >&2
    exit 1
fi
mkdir -p "$ROOT/build" "$ROOT/mnt"
cd "$ROOT/build"
# Never leave a stale release executable after a failed compile/link.
rm -f PIX86.EXE "$ROOT/mnt/PIX86.EXE"
wcl -bt=dos -ms -0 -za -wx -w4 -os -k4096 \
    "$ROOT/src/main.c" "$ROOT/src/video.c" "$ROOT/src/mouse.c" \
    "$ROOT/src/keyboard.c" "$ROOT/src/canvas.c" "$ROOT/src/cursor.c" \
    -fm=PIX86.MAP -fe=PIX86.EXE
if [ ! -s PIX86.EXE ]; then
    echo "Open Watcom did not produce PIX86.EXE." >&2
    exit 1
fi
cp PIX86.EXE "$ROOT/mnt/PIX86.EXE"
echo "Built $ROOT/mnt/PIX86.EXE (16-bit real-mode DOS, small model)."
