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
    "$ROOT/src/tools.c" "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/workpal.c" \
    "$ROOT/src/editor.c" "$ROOT/src/keymap.c" "$ROOT/src/layout.c" -o "$ROOT/build/edtest"
"$ROOT/build/edtest"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror \
    -I"$ROOT/src" "$ROOT/tests/render.c" "$ROOT/src/render.c" \
    "$ROOT/src/canvas.c" "$ROOT/src/tools.c" "$ROOT/src/history.c" \
    "$ROOT/src/view.c" "$ROOT/src/workpal.c" "$ROOT/src/editor.c" "$ROOT/src/region.c" "$ROOT/src/keymap.c" "$ROOT/src/layout.c" -o "$ROOT/build/rentest"
"$ROOT/build/rentest"
mkdir -p "$ROOT/build/iotest"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -I"$ROOT/src" \
    -Dfwrite=fault_write -Dfflush=fault_flush -Dfclose=fault_close \
    -Drename=fault_rename -Dremove=fault_remove \
    -c "$ROOT/src/fileio.c" -o "$ROOT/build/faultio.o"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -DIO_FAULTS \
    -I"$ROOT/src" "$ROOT/tests/testfile.c" "$ROOT/build/faultio.o" \
    "$ROOT/src/project.c" "$ROOT/src/editor.c" "$ROOT/src/canvas.c" \
    "$ROOT/src/tools.c" "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/workpal.c" \
    "$ROOT/src/dlginput.c" "$ROOT/src/keymap.c" "$ROOT/src/layout.c" -o "$ROOT/build/filetest"
(cd "$ROOT/build/iotest" && "$ROOT/build/filetest")
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror \
    -I"$ROOT/tests/stubs" -I"$ROOT/src" "$ROOT/tests/testui.c" \
    "$ROOT/src/textsize.c" "$ROOT/src/dlggeom.c" "$ROOT/src/menu.c" "$ROOT/src/menudraw.c" "$ROOT/src/uipaint.c" "$ROOT/src/keymap.c" \
    "$ROOT/src/dialog.c" "$ROOT/src/dlginput.c" "$ROOT/src/editor.c" \
    "$ROOT/src/canvas.c" "$ROOT/src/tools.c" "$ROOT/src/history.c" \
    "$ROOT/src/view.c" "$ROOT/src/workpal.c" "$ROOT/src/layout.c" -o "$ROOT/build/uitest"
"$ROOT/build/uitest"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -I"$ROOT/src" \
    "$ROOT/tests/testfont.c" "$ROOT/src/font.c" "$ROOT/src/textsize.c" "$ROOT/src/layout.c" -o "$ROOT/build/fonttest"
"$ROOT/build/fonttest"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -Dmain=pix86_run \
    -I"$ROOT/tests/stubs" -I"$ROOT/src" -c "$ROOT/src/main.c" -o "$ROOT/build/apptest.o"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror \
    -I"$ROOT/tests/stubs" -I"$ROOT/src" "$ROOT/tests/testpaint.c" "$ROOT/build/apptest.o" "$ROOT/src/ui.c" \
    "$ROOT/src/font.c" "$ROOT/src/textsize.c" "$ROOT/src/layout.c" "$ROOT/src/region.c" \
    "$ROOT/src/render.c" "$ROOT/src/screen.c" "$ROOT/src/cursor.c" \
    "$ROOT/src/menu.c" "$ROOT/src/menudraw.c" "$ROOT/src/uipaint.c" "$ROOT/src/keymap.c" \
    "$ROOT/src/dialog.c" "$ROOT/src/dlggeom.c" "$ROOT/src/dlginput.c" \
    "$ROOT/src/editor.c" "$ROOT/src/canvas.c" "$ROOT/src/tools.c" \
    "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/workpal.c" "$ROOT/src/fileio.c" \
    "$ROOT/src/project.c" -o "$ROOT/build/painttest"
mkdir -p "$ROOT/build/painttest-files"
(cd "$ROOT/build/painttest-files" && "$ROOT/build/painttest")
