#!/bin/sh
# Build the interactive DOS harness after building the application.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
WATCOM=${WATCOM:-"$HOME/opt/watcom"}
export WATCOM
export PATH="$WATCOM/binl:$PATH"
export INCLUDE="$WATCOM/h"
export LIB="$WATCOM/lib286"
mkdir -p "$ROOT/build" "$ROOT/mnt"
cd "$ROOT/build"
wcl -bt=dos -ms -0 -ze -wx -we -w4 "$ROOT/tests/doscheck.c" \
    -fe="$ROOT/mnt/DOSCHECK.EXE"
