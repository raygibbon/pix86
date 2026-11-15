#!/bin/sh
# Run scripts/dostest.sh outputs in DOSBox-X. Requires an X display on Linux.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DOSBOX_X=${DOSBOX_X:-dosbox-x}
SCRATCH=$(mktemp -d "$ROOT/mnt/DTXXXXXX")
NAME=$(basename "$SCRATCH")
mkdir -p "$ROOT/build/dos-results"
cat > "$SCRATCH/RUNTEST.BAT" <<'DOS'
@echo off
..\EDTEST > ED.TXT
..\IOTEST > IO.TXT
..\UITEST > UI.TXT
..\KBDTEST > KBD.TXT
..\ANIMTEST > ANIM.TXT
..\BMPTEST > BMP.TXT
..\ALPHTEST > ALPHA.TXT
..\SELTEST > SEL.TXT
..\VIDTEST > VGA.TXT
..\VIDTEST HIRES > VBE.TXT
exit
DOS
"$DOSBOX_X" -conf "$ROOT/tests/dosbox.conf" \
    -c "mount c $ROOT/mnt" -c 'c:' -c "cd $NAME" -c RUNTEST.BAT
for suite in ED IO UI KBD ANIM BMP ALPHA SEL VGA VBE; do
    cp "$SCRATCH/$suite.TXT" "$ROOT/build/dos-results/$suite.TXT"
    rg '^PASS:' "$SCRATCH/$suite.TXT"
done
# The directory was created exclusively above and contains only test fixtures.
rm -r "$SCRATCH"
