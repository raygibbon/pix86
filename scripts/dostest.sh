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
wcl -bt=dos -ms -0 -za -wx -we -w4 -os -k4096 -I"$ROOT/src" \
    "$ROOT/tests/testedit.c" "$ROOT/src/canvas.c" "$ROOT/src/tools.c" \
    "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" "$ROOT/src/select.c" "$ROOT/src/workpal.c" "$ROOT/src/editor.c" "$ROOT/src/anim.c" "$ROOT/src/keymap.c" \
    -fe="$ROOT/mnt/EDTEST.EXE"
# File adapter uses exclusive-create/stat/fdopen CRT extensions. Fault wrappers
# are linked only into this separate test executable, never PIX86.EXE.
wcc -bt=dos -ms -0 -ze -wx -we -w4 -os -I"$ROOT/src" \
    -Dfwrite=fault_write -Dfflush=fault_flush -Dfclose=fault_close \
    -Drename=fault_rename -Dremove=fault_remove \
    -fo=faultio.o "$ROOT/src/fileio.c"
wcl -bt=dos -ms -0 -ze -wx -we -w4 -os -k4096 -DIO_FAULTS -I"$ROOT/src" \
    "$ROOT/tests/testfile.c" faultio.o "$ROOT/src/project.c" \
    "$ROOT/src/editor.c" "$ROOT/src/anim.c" "$ROOT/src/keymap.c" "$ROOT/src/canvas.c" "$ROOT/src/tools.c" \
    "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" "$ROOT/src/select.c" "$ROOT/src/workpal.c" "$ROOT/src/dlginput.c" \
    -fe="$ROOT/mnt/IOTEST.EXE"
# Scripted UI events use real 16-bit arithmetic; hardware is replaced by mocks.
wcl -bt=dos -ms -0 -za -wx -we -w4 -os -k4096 -I"$ROOT/tests/stubs" -I"$ROOT/src" \
    "$ROOT/src/ui.c" "$ROOT/tests/testui.c" "$ROOT/src/textsize.c" "$ROOT/src/dlggeom.c" "$ROOT/src/menu.c" "$ROOT/src/menudraw.c" "$ROOT/src/uipaint.c" \
    "$ROOT/src/keymap.c" "$ROOT/src/dialog.c" "$ROOT/src/dlginput.c" \
    "$ROOT/src/editor.c" "$ROOT/src/anim.c"  "$ROOT/src/canvas.c" "$ROOT/src/tools.c" \
    "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" "$ROOT/src/select.c" "$ROOT/src/workpal.c" -fe="$ROOT/mnt/UITEST.EXE"
wcl -bt=dos -ms -0 -za -wx -we -w4 -os -k4096 -I"$ROOT/src" \
    "$ROOT/tests/testkeys.c" "$ROOT/src/keyboard.c" "$ROOT/src/keymap.c" \
    -fe="$ROOT/mnt/KBDTEST.EXE"
# Independent regular file adapter and allocation-fault editor/history objects.
wcc -bt=dos -ms -0 -ze -wx -we -w4 -os -fo=fileio.o "$ROOT/src/fileio.c"
wcc -bt=dos -ms -0 -za -wx -we -w4 -os -D_fmalloc=fault_alloc -D_ffree=fault_free \
    -fo=failedit.o "$ROOT/src/editor.c"
wcc -bt=dos -ms -0 -za -wx -we -w4 -os -D_fmalloc=fault_alloc -D_ffree=fault_free \
    -fo=failhist.o "$ROOT/src/history.c"
wcl -bt=dos -ms -0 -za -wx -we -w4 -os -k4096 -I"$ROOT/src" \
    -DALLOC_FAULTS "$ROOT/tests/testanim.c" failedit.o failhist.o \
    "$ROOT/src/anim.c" "$ROOT/src/canvas.c" "$ROOT/src/tools.c" "$ROOT/src/view.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" "$ROOT/src/select.c" \
    "$ROOT/src/workpal.c" "$ROOT/src/keymap.c" fileio.o "$ROOT/src/project.c" \
    -fe="$ROOT/mnt/ANIMTEST.EXE"
wcc -bt=dos -ms -0 -za -wx -we -w4 -os -Dfwrite=bmp_fault_write \
    -fo=faultbmp.o "$ROOT/src/bmp.c"
wcc -bt=dos -ms -0 -ze -wx -we -w4 -os -Dfflush=bmp_fault_flush -Dfclose=bmp_fault_close \
    -fo=bmpio.o "$ROOT/src/fileio.c"
wcl -bt=dos -ms -0 -za -wx -we -w4 -os -k4096 -DBMP_FAULTS -I"$ROOT/src" \
    "$ROOT/tests/testbmp.c" faultbmp.o bmpio.o "$ROOT/src/export.c" "$ROOT/src/png.c" "$ROOT/src/colour.c" \
    "$ROOT/src/editor.c" "$ROOT/src/history.c" "$ROOT/src/anim.c" "$ROOT/src/canvas.c" \
    "$ROOT/src/view.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" "$ROOT/src/select.c" "$ROOT/src/tools.c" "$ROOT/src/workpal.c" "$ROOT/src/keymap.c" \
    -fe="$ROOT/mnt/BMPTEST.EXE"
# Optional interactive independent 8-bit BMP viewer (not in PIX86.EXE).
wcl -bt=dos -ms -0 -ze -wx -we -w4 -os -k4096 -I"$ROOT/src" \
    "$ROOT/tests/bmpview.c" "$ROOT/src/layout.c" "$ROOT/src/videovga.c" "$ROOT/src/videovbe.c" "$ROOT/src/video.c" "$ROOT/src/font.c" "$ROOT/src/textsize.c" \
    -fe="$ROOT/mnt/BMPVIEW.EXE"
wcc -bt=dos -ms -0 -za -wx -we -w4 -os -Dfwrite=alpha_write -fo=faultpng.o "$ROOT/src/png.c"
wcl -bt=dos -ms -0 -za -wx -we -w4 -os -k4096 -DALPHA_FAULTS -I"$ROOT/src" \
    "$ROOT/tests/tstalpha.c" faultpng.o "$ROOT/src/export.c" "$ROOT/src/bmp.c" "$ROOT/src/colour.c" \
    fileio.o "$ROOT/src/editor.c" "$ROOT/src/history.c" "$ROOT/src/anim.c" \
    "$ROOT/src/canvas.c" "$ROOT/src/tools.c" "$ROOT/src/view.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" "$ROOT/src/select.c" "$ROOT/src/workpal.c" "$ROOT/src/keymap.c" \
    -fe="$ROOT/mnt/ALPHTEST.EXE"

wcl -bt=dos -ms -0 -za -wx -we -w4 -os -k4096 -I"$ROOT/src" \
    "$ROOT/tests/testsel.c" "$ROOT/src/editor.c" "$ROOT/src/select.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" \
    "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/workpal.c" "$ROOT/src/anim.c" \
    "$ROOT/src/canvas.c" "$ROOT/src/tools.c" "$ROOT/src/keymap.c" fileio.o -fe="$ROOT/mnt/SELTEST.EXE"

wcl -bt=dos -ms -0 -za -wx -we -w4 -os -k4096 -I"$ROOT/src" \
    "$ROOT/tests/testvideo.c" "$ROOT/src/video.c" "$ROOT/src/videovga.c" "$ROOT/src/videovbe.c" \
    -fe="$ROOT/mnt/VIDTEST.EXE"

# Read-only PIT diagnostics, including monotonicity against the BIOS clock.
wcl -bt=dos -ms -0 -za -wx -we -w4 -os -k4096 -I"$ROOT/src" \
    "$ROOT/tests/testtime.c" "$ROOT/src/timer.c" -fe="$ROOT/mnt/TIMETEST.EXE"
