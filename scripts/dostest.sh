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
    "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/workpal.c" "$ROOT/src/editor.c" "$ROOT/src/anim.c" "$ROOT/src/keymap.c" \
    -fe="$ROOT/mnt/EDTEST.EXE"
# File adapter uses exclusive-create/stat/fdopen CRT extensions. Fault wrappers
# are linked only into this separate test executable, never PIX86.EXE.
wcc -bt=dos -ms -0 -ze -wx -w4 -os -I"$ROOT/src" \
    -Dfwrite=fault_write -Dfflush=fault_flush -Dfclose=fault_close \
    -Drename=fault_rename -Dremove=fault_remove \
    -fo=faultio.o "$ROOT/src/fileio.c"
wcl -bt=dos -ms -0 -ze -wx -w4 -os -k4096 -DIO_FAULTS -I"$ROOT/src" \
    "$ROOT/tests/testfile.c" faultio.o "$ROOT/src/project.c" \
    "$ROOT/src/editor.c" "$ROOT/src/anim.c" "$ROOT/src/keymap.c" "$ROOT/src/canvas.c" "$ROOT/src/tools.c" \
    "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/workpal.c" "$ROOT/src/dlginput.c" \
    -fe="$ROOT/mnt/IOTEST.EXE"
# Scripted UI events use real 16-bit arithmetic; hardware is replaced by mocks.
wcl -bt=dos -ms -0 -za -wx -w4 -os -k4096 -I"$ROOT/tests/stubs" -I"$ROOT/src" \
    "$ROOT/tests/testui.c" "$ROOT/src/textsize.c" "$ROOT/src/dlggeom.c" "$ROOT/src/menu.c" "$ROOT/src/menudraw.c" "$ROOT/src/uipaint.c" \
    "$ROOT/src/keymap.c" "$ROOT/src/dialog.c" "$ROOT/src/dlginput.c" \
    "$ROOT/src/editor.c" "$ROOT/src/anim.c"  "$ROOT/src/canvas.c" "$ROOT/src/tools.c" \
    "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/workpal.c" -fe="$ROOT/mnt/UITEST.EXE"
wcl -bt=dos -ms -0 -za -wx -w4 -os -k4096 -I"$ROOT/src" \
    "$ROOT/tests/testkeys.c" "$ROOT/src/keyboard.c" "$ROOT/src/keymap.c" \
    -fe="$ROOT/mnt/KBDTEST.EXE"
wcl -bt=dos -ms -0 -za -wx -w4 -os -k4096 -I"$ROOT/src" \
    "$ROOT/tests/testanim.c" "$ROOT/src/editor.c" "$ROOT/src/history.c" \
    "$ROOT/src/anim.c" "$ROOT/src/canvas.c" "$ROOT/src/tools.c" "$ROOT/src/view.c" \
    "$ROOT/src/workpal.c" "$ROOT/src/keymap.c" fileio.o "$ROOT/src/project.c" \
    -fe="$ROOT/mnt/ANIMTEST.EXE"
