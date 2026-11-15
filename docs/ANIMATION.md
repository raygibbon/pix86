# UI and animation workspace plan

Base: main 6229285. Target remains 8086 real-mode DOS, small model, VGA 13h.
No reference editor code, glyphs, assets or layouts are used.

Implementation stages:
1. Refine original 6x8 glyphs and menu spacing; retain the nine-pixel menu bar.
2. Contrast-based pixel mesh (suppressed below 3x) and independent 8/16 guides.
3. Reserve 36 pixels within the drawing workspace for a collapsible timeline;
   recompute integer zoom and centre, preserving the sidebar/status.
4. Portable bounded animation model, existing active Canvas/tools, frame commands,
   scrolling timeline, looping elapsed-time playback and adjustable frame timing.
5. Whole-document bounded history and exact saved baseline; version 2 indexed
   animation I/O through the existing checked TMP/BAK replacement transaction.
6. Strict host tests, Watcom build, DOSBox-X native suites, visual captures and
   measured memory budget, followed by documentation and a PR against main.

Memory design: at most 16 frames and 16384 total pixel bytes (therefore at most
four 64x64 frames). Each document snapshot is less than 32 KiB. Allocate the live
animation, five undo states and saved baseline as separate far-heap blocks under
Watcom, ordinary allocations on hosts. No frame arrays in DGROUP. Keep a near
4096-byte active canvas for existing tools and bounded near I/O scratch. Allocate
all history/baseline capacity at initialization, so later edits cannot fail halfway
through an operation. Initialization failure releases partial allocations.

New/Open must validate fully before replacing the live state. Version 1 remains
exactly its published single-square-image format. Version 2 has an explicit frame
count, durations and sequential opaque indexed frames. All indices, including 0,
are opaque colours; transparency, onion skinning and export are deferred.

History snapshots contain all frames and timings. Navigation is not an action;
undo restores the frame associated with the edit. View/tool/playing state is not
persisted or dirty. Playback uses elapsed milliseconds, never polling counts;
menu/modal entry stops it, and playback blocks painting gestures.

Verification retains existing idle-write, exact overlay restoration, one startup
clear and Escape interception regressions. Additional tests cover rectangular
geometry, timing/wrap, structural undo, failure preservation and malformed v2.

## Implemented and verified (2026-11-15)

All planned stages above are implemented. Live/history/baseline storage uses a
portable Animation type, aliased by the native file codec. Frame timing adjusts
by 10 ms between 20 and 2000 ms. Playback loops and stops for overlays/editing;
BIOS clock resolution limits precision. Optional onion skin remains deferred.

Eight strict C89 host suites and all eight ASan/UBSan suites pass. Open Watcom
builds the real-mode application and five native suites with `-wx -we -w4`.
EDTEST, IOTEST, UITEST, KBDTEST and ANIMTEST execute and pass in DOSBox-X. Native
ANIMTEST additionally injects failure at each of seven allocations and confirms
partial allocations are freed. No application fault hooks are compiled in.

Mouse/keyboard acceptance opened an independently generated three-frame 32×24
fixture; toggled timeline and pixel/tile grid; opened Frame and Font sample;
played/stopped looping frames; then used timeline blank/duplicate/delete/select,
edited pixels and saved version 2. An independent host parser compared all four
frame records and timing bytes. After selecting another frame, Undo restored the
edited frame, and Redo restored the exact saved bytes. New 16×24 followed by Open
and re-save returned identical animation data. Stopped captures one second apart
are byte-identical. The original idle-write, overlay restoration, intercepted
Escape and one-startup-clear regressions remain passing; new paint tests compare
timeline open/closed restoration byte-for-byte and consume held control presses.

| Native VGA capture | Enlarged |
| --- | --- |
| [Timeline 320×200](ANIM.PNG) | [Unfiltered 2× DOSBox-X](ANIM2X.PNG) |
| [Pixel mesh + 8×8 guide](ANGRID.PNG) | |
| [Frame menu](ANMENU.PNG) | |
| [Original font diagnostic](ANFONT.PNG) | |

Captures use the same original generated sprite with black, white and saturated
background regions. Native crops use exact integer sampling; no filtering or
font scaling is used. Physical VGA, mouse/storage hardware and native DOS remain
outstanding. Animation stores opaque colours only; onion skin, transparency,
layers, PNG/GIF/sprite-sheet export and precision sub-BIOS-tick timing are absent.
