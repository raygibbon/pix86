#!/bin/sh
# Run the instrumented harness in both video modes; keep raw PIT-unit results.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DOSBOX_X=${DOSBOX_X:-dosbox-x}
PROGRAM=${1:-PERFDOS}
case "$PROGRAM" in PERFDOS|BASEPERF) ;; *) echo 'Expected PERFDOS or BASEPERF' >&2; exit 1;; esac
SCRATCH=$(mktemp -d "$ROOT/mnt/PFXXXXXX")
NAME=$(basename "$SCRATCH")
mkdir -p "$ROOT/build/adaptive"
cat > "$SCRATCH/RUNPERF.BAT" <<DOS
@echo off
..\\$PROGRAM.EXE > VGA.TXT
..\\$PROGRAM.EXE /HIRES > VBE.TXT
exit
DOS
"$DOSBOX_X" -conf "$ROOT/tests/dosbox.conf" \
    -c "mount c $ROOT/mnt" -c 'c:' -c "cd $NAME" -c RUNPERF.BAT
for mode in VGA VBE; do
    cp "$SCRATCH/$mode.TXT" "$ROOT/build/adaptive/$PROGRAM-$mode.TXT"
    rg '^PASS:|^PROFILE timer=' "$SCRATCH/$mode.TXT"
    if rg '^FAIL:' "$SCRATCH/$mode.TXT"; then exit 1; fi
done
rm -r "$SCRATCH"
