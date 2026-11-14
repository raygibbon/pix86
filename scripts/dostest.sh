#!/bin/sh
# Compile the portable M2 suite with 16-bit integers for execution in DOS.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
WATCOM=${WATCOM:-"$HOME/opt/watcom"}
export WATCOM
export PATH="$WATCOM/binl:$PATH"
export INCLUDE="$WATCOM/h"
export LIB="$WATCOM/lib286"
mkdir -p "$ROOT/build" "$ROOT/mnt"
cd "$ROOT/build"
wcl -bt=dos -ms -0 -za -wx -w4 -os -k4096 -I"$ROOT/src" \
    "$ROOT/tests/testedit.c" "$ROOT/src/canvas.c" "$ROOT/src/tools.c" \
    "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/editor.c" \
    -fe="$ROOT/mnt/EDTEST.EXE"
