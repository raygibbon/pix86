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
