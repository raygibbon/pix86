# Editor rendering performance

This audit covers the existing C89, 8086 real-mode rendering pipeline. It adds
optional profiling and focused incremental updates; it does not replace the
screen architecture or allocate a screen-sized buffer.

## Pipeline and findings

`main.c` polls input, advances the editor and passes `EDIT_*` invalidations to
`screen.c`. The screen dispatcher selects the affected painter. `render.c`
composes canvas cells, selection overlays, controls and timeline thumbnails;
`video.c` clips operations and dispatches to Mode 13h or the banked VBE driver.
The software cursor saves/restores its small footprint and is hidden only when
a protected repaint intersects it.

The highest-cost avoidable work found in the initial audit was:

1. Diagonal pencil/eraser segments repainted their entire bounding rectangle,
   including unchanged canvas cells.
2. Flood fill invalidated the complete canvas even though the fill queue already
   identified the changed cells.
3. Ordinary frame navigation took the broad canvas/status/timeline update path,
   repainting unchanged canvas areas and timeline thumbnails.

Stroke and fill paths now pass their changed cell indices to the shared cell
painter. Frame navigation compares visible frame pixels and transparency, uses
the existing incremental canvas renderer, and updates frame indicators without
repainting unchanged thumbnails. Strokes/fills refresh only the active thumbnail.
Structural timeline changes still repaint the timeline. Selection movement
uses its clipped damage region; modal and menu restoration repaints only covered
regions. Tool groups, status text, and timeline indicators use existing
selective painters rather than a new invalidation framework.

VGA rectangle fills use contiguous scanline writes. VBE rectangle fills now use
contiguous far-memory writes bounded by the current bank window. VBE clipping
and bank-boundary handling remain in the video driver. No application-wide
framebuffer, graphics library, or DOS extender is introduced.

## Host write-count evidence

The host framebuffer mock compares optimized work against full-render reference
images, so these counts check both the amount of work and pixel correctness.
The before values below were captured with the same deterministic operations
before the corresponding change; the current tests reproduce the after values.
Counts are framebuffer writes, not elapsed-time measurements.

| Operation | Before | After | Change |
|---|---:|---:|---:|
| Diagonal stroke segment | 14,641 | 1,210 | -91.7% |
| Full-canvas flood fill | 49,732 | 30,976 | -37.7% |
| Dense frame navigation | 50,932 | 32,176 | -36.8% |

Other current counts include 2,080 writes for a 16-cell fill, 1,321 for sparse
frame navigation, and 145 for a stroke with the timeline visible. A single
pixel operation remains 121 writes. Full-frame reference comparisons in the
host suite detect missing pixels and restoration artifacts.

## DOSBox rendering measurements

`tests/perfdos.c` performs 32 full redraws, 32 75%-changed frame diffs, 32
playback renders (one initial full render followed by 31 diffs), and a diagonal
stroke for 16×16, 32×32, and 64×64 documents. `scripts/profilebuild.sh` builds
the instrumented 16-bit small-model harness with Open Watcom. The runs below
used DOSBox 0.74-3, normal 386 emulation at 40,000 cycles, for both VGA 320×200
and VBE 640×480. DOSBox-X was unavailable in this environment.

The reported render totals are DOS `clock()` ticks over eight operations.
Open Watcom reports 1,000 ticks/second, but DOSBox/DOS clock advancement is
effectively about 55 ms per tick. The totals are coarse and noisy; they should
not be read as sub-millisecond timing or a precise frame-rate benchmark.

| Mode / size | Full redraw ×32 | 75%-changed diff ×32 | Playback ×32 |
|---|---:|---:|---:|
| VGA 16×16 | 165 ticks | 220 ticks | 219 ticks |
| VGA 32×32 | 495 ticks | 439 ticks | 439 ticks |
| VGA 64×64 | 1,538 ticks | 1,428 ticks | 1,483 ticks |
| VBE 16×16 | 1,703 ticks | 1,208 ticks | 1,208 ticks |
| VBE 32×32 | 3,131 ticks | 2,252 ticks | 2,361 ticks |
| VBE 64×64 | 5,987 ticks | 4,559 ticks | 4,614 ticks |

Incremental rendering substantially reduces framebuffer writes in both modes.
In this run, 75%-changed diffs were slower on VGA at 16×16, faster by about 11%
at 32×32 and about 7% at 64×64. On VBE they were faster by about 24–29% across
the measured sizes, consistent with avoiding expensive banked writes. Playback
renders remained close to the diff-only totals despite including one full
presentation. A separate 8-render run varied significantly, so these numbers
are directional rather than stable timing guarantees. Fewer writes alone do
not guarantee lower render time: sparse changes are the strongest fit for
incremental rendering, while dense updates can pay comparison overhead.

The DOS harness also reports primitive counts and VBE bank switches. The
32-render run recorded 41,338 VBE bank switches across the full workload. Its
mouse-poll-to-present sample spans input handling through the next screen
presentation, not the time from physical mouse movement. The coarse DOS clock
cannot establish physical input latency. The VGA and VBE driver suites exercise
bank-boundary access; physical DOS hardware was not available.

## Constraints and remaining limitations

The regular loop still uses the existing DOS `delay(10)` while idle and polls
the mouse once per loop; this keeps idle processing blocked rather than
busy-waiting, but does not provide sub-tick input latency. `cursor_show` avoids
work when the pointer has not moved, and canvas repaints protect only an
intersecting cursor. Actual mouse-arrival-to-screen latency was not measurable
in this environment.

The compact status-row frame-number strip is repainted when its active frame or
count changes. The larger timeline uses selective indicator/thumbnail updates.
The release image is close to the 64 KiB near-code limit, so no additional UI
state or generalized invalidation layer was introduced.

## Optional diagnostics and verification

Compile-time `PIX86_PROFILE` enables counters for framebuffer writes,
rectangles/scanlines, full and partial canvas paints, compared canvas cells,
timeline/UI operations, VBE bank changes, mouse polls, status characters, and
screen/input elapsed ticks. Without the define, the profiling macros compile
to no-ops; ordinary builds do not call the timer or update counters. Profiling
uses `clock()` only in the separate diagnostic build and does not change the
application timer, install an interrupt hook, or alter playback timing.

Run:

```sh
scripts/test.sh
scripts/build.sh
scripts/dostest.sh
scripts/profilebuild.sh
```

Run the DOS tests with `scripts/rundost.sh` when DOSBox-X is available. IOTEST
is run in a disposable directory by that script. The profile harness logs are
written under `mnt/` and are generated output, not source fixtures.
