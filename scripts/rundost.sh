#!/bin/sh
# Run scripts/dostest.sh outputs in DOSBox-X. Requires an X display on Linux.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DOSBOX_X=${DOSBOX_X:-dosbox-x}
SCRATCH=$(mktemp -d "$ROOT/mnt/DTXXXXXX")
NAME=$(basename "$SCRATCH")
mkdir -p "$ROOT/build/dos-results"
"$DOSBOX_X" -conf "$ROOT/tests/dosbox.conf" \
    -c "mount c $ROOT/mnt" -c 'c:' -c "cd $NAME" \
    -c '..\EDTEST > ED.TXT' -c '..\IOTEST > IO.TXT' \
    -c '..\UITEST > UI.TXT' -c '..\KBDTEST > KBD.TXT' \
    -c '..\ANIMTEST > ANIM.TXT' -c '..\BMPTEST > BMP.TXT' -c '..\ALPHTEST > ALPHA.TXT' -c exit
for suite in ED IO UI KBD ANIM BMP ALPHA; do
    cp "$SCRATCH/$suite.TXT" "$ROOT/build/dos-results/$suite.TXT"
    rg '^PASS:' "$SCRATCH/$suite.TXT"
done
# The directory was created exclusively above and contains only test fixtures.
rm -r "$SCRATCH"
