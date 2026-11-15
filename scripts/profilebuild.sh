#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
WATCOM=${WATCOM:-"$HOME/opt/watcom"}
export WATCOM
export PATH="$WATCOM/binl:$PATH"
export INCLUDE="$WATCOM/h"
export LIB="$WATCOM/lib286"
mkdir -p "$ROOT/build" "$ROOT/mnt"
cd "$ROOT/build"
wcl -bt=dos -ms -0 -za -wx -we -w4 -os -k4096 -DPIX86_PROFILE \
    -I"$ROOT/src" "$ROOT/tests/perfdos.c" "$ROOT/src/profile.c" \
    "$ROOT/src/video.c" "$ROOT/src/videovga.c" "$ROOT/src/videovbe.c" \
    "$ROOT/src/font.c" "$ROOT/src/textsize.c" "$ROOT/src/cursor.c" "$ROOT/src/mouse.c" \
    "$ROOT/src/canvas.c" "$ROOT/src/tools.c" "$ROOT/src/history.c" \
    "$ROOT/src/view.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" \
    "$ROOT/src/select.c" "$ROOT/src/workpal.c" "$ROOT/src/editor.c" \
    "$ROOT/src/anim.c" "$ROOT/src/render.c" "$ROOT/src/region.c" \
    "$ROOT/src/screen.c" "$ROOT/src/menu.c" "$ROOT/src/menudraw.c" \
    "$ROOT/src/uipaint.c" "$ROOT/src/keymap.c" \
    -fe="$ROOT/mnt/PERFDOS.EXE"
echo "Built $ROOT/mnt/PERFDOS.EXE (instrumented 16-bit DOS profile harness)."
