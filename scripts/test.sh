#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
mkdir -p "$ROOT/build"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror \
    -I"$ROOT/src" "$ROOT/tests/core.c" "$ROOT/src/canvas.c" \
    "$ROOT/src/cursor.c" -o "$ROOT/build/coretest"
"$ROOT/build/coretest"
