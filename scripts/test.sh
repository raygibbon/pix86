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
    "$ROOT/src/tools.c" "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" "$ROOT/src/select.c" "$ROOT/src/workpal.c" \
    "$ROOT/src/editor.c" "$ROOT/src/anim.c" "$ROOT/src/keymap.c" -o "$ROOT/build/edtest"
"$ROOT/build/edtest"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -DPIX86_TEST \
    -I"$ROOT/src" "$ROOT/tests/render.c" "$ROOT/src/render.c" \
    "$ROOT/src/canvas.c" "$ROOT/src/tools.c" "$ROOT/src/history.c" \
    "$ROOT/src/view.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" "$ROOT/src/select.c" "$ROOT/src/workpal.c" "$ROOT/src/editor.c" "$ROOT/src/anim.c" "$ROOT/src/region.c" "$ROOT/src/keymap.c" -o "$ROOT/build/rentest"
"$ROOT/build/rentest"
mkdir -p "$ROOT/build/iotest"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -I"$ROOT/src" \
    -Dfwrite=fault_write -Dfflush=fault_flush -Dfclose=fault_close \
    -Drename=fault_rename -Dremove=fault_remove \
    -c "$ROOT/src/fileio.c" -o "$ROOT/build/faultio.o"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -DIO_FAULTS \
    -I"$ROOT/src" "$ROOT/tests/testfile.c" "$ROOT/build/faultio.o" \
    "$ROOT/src/project.c" "$ROOT/src/editor.c" "$ROOT/src/anim.c" "$ROOT/src/canvas.c" \
    "$ROOT/src/tools.c" "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" "$ROOT/src/select.c" "$ROOT/src/workpal.c" \
    "$ROOT/src/dlginput.c" "$ROOT/src/keymap.c" -o "$ROOT/build/filetest"
(cd "$ROOT/build/iotest" && "$ROOT/build/filetest")
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror \
    -I"$ROOT/tests/stubs" -I"$ROOT/src" "$ROOT/src/ui.c" "$ROOT/tests/testui.c" \
    "$ROOT/src/textsize.c" "$ROOT/src/dlggeom.c" "$ROOT/src/menu.c" "$ROOT/src/menudraw.c" "$ROOT/src/uipaint.c" "$ROOT/src/keymap.c" \
    "$ROOT/src/dialog.c" "$ROOT/src/dlginput.c" "$ROOT/src/editor.c" "$ROOT/src/anim.c" \
    "$ROOT/src/canvas.c" "$ROOT/src/tools.c" "$ROOT/src/history.c" \
    "$ROOT/src/view.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" "$ROOT/src/select.c" "$ROOT/src/workpal.c" -o "$ROOT/build/uitest"
"$ROOT/build/uitest"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -I"$ROOT/src" \
    "$ROOT/src/layout.c" "$ROOT/tests/testfont.c" "$ROOT/src/font.c" "$ROOT/src/textsize.c" -o "$ROOT/build/fonttest"
"$ROOT/build/fonttest"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -Dmain=pix86_run \
    -I"$ROOT/tests/stubs" -I"$ROOT/src" -c "$ROOT/src/main.c" -o "$ROOT/build/apptest.o"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror \
    -I"$ROOT/tests/stubs" -I"$ROOT/src" "$ROOT/tests/testpaint.c" "$ROOT/build/apptest.o" "$ROOT/src/ui.c" \
    "$ROOT/src/font.c" "$ROOT/src/textsize.c" "$ROOT/src/region.c" \
    "$ROOT/src/render.c" "$ROOT/src/screen.c" "$ROOT/src/cursor.c" \
    "$ROOT/src/menu.c" "$ROOT/src/menudraw.c" "$ROOT/src/uipaint.c" "$ROOT/src/keymap.c" \
    "$ROOT/src/dialog.c" "$ROOT/src/dlggeom.c" "$ROOT/src/dlginput.c" \
    "$ROOT/src/editor.c" "$ROOT/src/anim.c" "$ROOT/src/canvas.c" "$ROOT/src/tools.c" \
    "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" "$ROOT/src/select.c" "$ROOT/src/workpal.c" "$ROOT/src/fileio.c" \
    "$ROOT/src/project.c" "$ROOT/src/bmp.c" "$ROOT/src/export.c" "$ROOT/src/png.c" "$ROOT/src/colour.c" -o "$ROOT/build/painttest"
mkdir -p "$ROOT/build/painttest-files"
(cd "$ROOT/build/painttest-files" && "$ROOT/build/painttest")
# Animation allocation failures are injected only into test objects.
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -I"$ROOT/src" \
    -Dmalloc=fault_alloc -Dfree=fault_free -c "$ROOT/src/editor.c" -o "$ROOT/build/failedit.o"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -I"$ROOT/src" \
    -Dmalloc=fault_alloc -Dfree=fault_free -c "$ROOT/src/history.c" -o "$ROOT/build/failhist.o"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -DALLOC_FAULTS -I"$ROOT/src" \
    "$ROOT/tests/testanim.c" "$ROOT/build/failedit.o" "$ROOT/build/failhist.o" \
    "$ROOT/src/anim.c" "$ROOT/src/canvas.c" "$ROOT/src/tools.c" "$ROOT/src/view.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" "$ROOT/src/select.c" \
    "$ROOT/src/workpal.c" "$ROOT/src/keymap.c" "$ROOT/src/fileio.c" "$ROOT/src/project.c" \
    -o "$ROOT/build/animtest"
mkdir -p "$ROOT/build/animtest-files"
(cd "$ROOT/build/animtest-files" && "$ROOT/build/animtest")
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -I"$ROOT/src" \
    -Dfwrite=bmp_fault_write -c "$ROOT/src/bmp.c" -o "$ROOT/build/faultbmp.o"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -I"$ROOT/src" \
    -Dfflush=bmp_fault_flush -Dfclose=bmp_fault_close -c "$ROOT/src/fileio.c" -o "$ROOT/build/bmpio.o"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -DBMP_FAULTS -I"$ROOT/src" \
    "$ROOT/tests/testbmp.c" "$ROOT/build/faultbmp.o" "$ROOT/build/bmpio.o" \
    "$ROOT/src/export.c" "$ROOT/src/png.c" "$ROOT/src/colour.c" "$ROOT/src/anim.c" "$ROOT/src/editor.c" "$ROOT/src/canvas.c" \
    "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" "$ROOT/src/select.c" "$ROOT/src/tools.c" "$ROOT/src/workpal.c" \
    "$ROOT/src/keymap.c" -o "$ROOT/build/bmptest"
mkdir -p "$ROOT/build/bmptest-files"
(cd "$ROOT/build/bmptest-files" && "$ROOT/build/bmptest")
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -I"$ROOT/src" \
    -Dfwrite=alpha_write -c "$ROOT/src/png.c" -o "$ROOT/build/faultpng.o"
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -DALPHA_FAULTS -I"$ROOT/src" \
    "$ROOT/tests/tstalpha.c" "$ROOT/build/faultpng.o" "$ROOT/src/export.c" "$ROOT/src/bmp.c" "$ROOT/src/colour.c" \
    "$ROOT/src/fileio.c" "$ROOT/src/editor.c" "$ROOT/src/history.c" "$ROOT/src/anim.c" \
    "$ROOT/src/canvas.c" "$ROOT/src/tools.c" "$ROOT/src/view.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" "$ROOT/src/select.c" "$ROOT/src/workpal.c" "$ROOT/src/keymap.c" \
    -o "$ROOT/build/alphatest"
mkdir -p "$ROOT/build/alphatest-files"
(cd "$ROOT/build/alphatest-files" && "$ROOT/build/alphatest")

"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -I"$ROOT/src" \
    "$ROOT/tests/testsel.c" "$ROOT/src/editor.c" "$ROOT/src/select.c" "$ROOT/src/layout.c" "$ROOT/src/flyout.c" \
    "$ROOT/src/history.c" "$ROOT/src/view.c" "$ROOT/src/workpal.c" "$ROOT/src/anim.c" \
    "$ROOT/src/canvas.c" "$ROOT/src/tools.c" "$ROOT/src/keymap.c" "$ROOT/src/fileio.c" -o "$ROOT/build/seltest"
mkdir -p "$ROOT/build/seltest-files"
(cd "$ROOT/build/seltest-files" && "$ROOT/build/seltest")
"${CC:-cc}" -std=c89 -pedantic -Wall -Wextra -Werror -I"$ROOT/src" \
    "$ROOT/tests/compact.c" "$ROOT/src/screen.c" "$ROOT/src/render.c" "$ROOT/src/ui.c" \
    "$ROOT/src/flyout.c" "$ROOT/src/layout.c" "$ROOT/src/view.c" "$ROOT/src/editor.c" \
    "$ROOT/src/select.c" "$ROOT/src/history.c" "$ROOT/src/anim.c" "$ROOT/src/canvas.c" "$ROOT/src/tools.c" \
    "$ROOT/src/workpal.c" "$ROOT/src/keymap.c" "$ROOT/src/menu.c" "$ROOT/src/menudraw.c" \
    "$ROOT/src/uipaint.c" "$ROOT/src/font.c" "$ROOT/src/textsize.c" "$ROOT/src/cursor.c" "$ROOT/src/region.c" \
    -o "$ROOT/build/cmptest"
"$ROOT/build/cmptest"
