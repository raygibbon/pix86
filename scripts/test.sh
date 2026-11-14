#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
mkdir -p "$ROOT/build"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror \
    -I"$ROOT/src" "$ROOT/tests/core.c" "$ROOT/src/canvas.c" \
    "$ROOT/src/cursor.c" -o "$ROOT/build/coretest"
"$ROOT/build/coretest"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror \
    -I"$ROOT/src" "$ROOT/tests/testedit.c" "$ROOT/src/canvas.c" \
    "$ROOT/src/tools.c" "$ROOT/src/history.c" "$ROOT/src/view.c" \
    "$ROOT/src/editor.c" -o "$ROOT/build/edtest"
"$ROOT/build/edtest"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror \
    -I"$ROOT/src" "$ROOT/tests/render.c" "$ROOT/src/render.c" \
    "$ROOT/src/canvas.c" "$ROOT/src/tools.c" "$ROOT/src/history.c" \
    "$ROOT/src/view.c" "$ROOT/src/editor.c" -o "$ROOT/build/rentest"
"$ROOT/build/rentest"
