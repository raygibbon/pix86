# Indexed BMP, RGBA PNG and animation export

File → Export opens one compact modal with a destination field, mode, sheet
layout and grid column count. Tab/Shift+Tab moves focus; arrows change options.
Enter exports and Escape cancels. Mouse clicks choose/cycle options or focus the
filename field; Export/Cancel activate their actions. Irrelevant sheet controls
are dimmed and skipped. Idle polling does not paint. Overlays consume held clicks
before drawing resumes. Export stops playback without changing the selected frame.

- Current frame writes the selected artwork frame at its native dimensions.
- All frames writes one image per frame, in animation order, starting at 0001.
- Sprite sheet places frames left-to-right then top-to-bottom. Horizontal uses
  one row; vertical one column; grid allows 1..16 columns.

No scaling, interpolation, separators, margins, grid, cursor, UI or other view
decoration is exported. A grid's unused final cells contain the current BG
index, including when columns exceed frame count. There is no spacing between
frames. Timings remain in the native project; BMP does not store animation timing.

## Destination names and replacement

Destinations must be DOS 8.3 paths with `.BMP` or `.PNG` matching the selected format
(supplied when omitted). Explicit mismatched extensions, device names and invalid paths are rejected. Numbered mode
uses the first **up to four** stem characters and four digits: `PLY.BMP` gives
`PLY0001.BMP`, `PLY0002.BMP`; `PLAYER.BMP` gives `PLAY0001.BMP`, etc. The directory
is preserved. The dialog result reports the actual final/failed destination.

All batch destinations are checked before writing. An existing file triggers
one confirmation for matching outputs; cancelling writes nothing. Each output
uses the existing exclusive TMP/BAK checked-write/flush/close/rename/rollback
transaction. Earlier completed outputs remain if a later one fails. The result
reports completed/total and the failed target; retained recovery paths are shown.
A retained-backup warning stops a batch after counting the installed file, so its
recovery information is not overwritten. A batch is not an atomic multi-file
transaction, and power-loss/concurrent-writer guarantees are unchanged.

Export neither saves nor renames the native project, nor advances its saved
baseline, changes pixel data, timings, selection or dirty state. The current
native filename is protected from direct replacement; `.P86` destinations are
always rejected. As with native DOS file handling, path comparisons use canonical
case/separators and do not resolve filesystem aliases. Exporting first completes
any held stroke at the normal command boundary.

## Original BMP encoding

`bmp.c` is an original ANSI C encoder, independent of DOS, GUI and frame logic.
It emits these fields explicitly in little endian, never serializing a C struct:

| Field | Value |
| --- | --- |
| File header | 14 bytes, `BM`, exact 32-bit size, zero reserved words |
| Pixel offset | 1078 (14 + 40 + 256 × 4) |
| DIB | 40-byte BITMAPINFOHEADER |
| Dimensions | Positive native image/sheet width and height |
| Planes / depth / compression | 1 / 8 / BI_RGB (0) |
| Image size | Four-byte-aligned stride × height |
| Palette count | 256 RGBQUAD entries, B/G/R/reserved-zero |
| Pixels | Unchanged indices, bottom row first, zero row-padding bytes |

The reserved palette byte is **not alpha**. Both P86 v1 and v2 treat all indices,
including black/index 0, as opaque colours. There is no transparency metadata to
flatten and no implicit transparency key. This export does not claim alpha.

For indices 0..15, VGA DAC channels are 4 × index. For the remaining indices,
q=index−16: R=12×(q mod 6), G=12×(floor(q/6) mod 6), and
B=12×(floor(q/36) mod 6), exactly matching `video.c`. Each six-bit channel becomes
round(DAC × 255 / 63), implemented as (DAC × 255 + 31) / 63. A channel of 60
therefore becomes 243, preserving the intended DAC intensity rather than
normalizing the brightest used colour to 255. Repeated RGB entries keep their
separate original indices.

Factual format references, consulted 2026-11-15:
[Microsoft Bitmap Storage](https://learn.microsoft.com/en-us/windows/win32/gdi/bitmap-storage),
[BITMAPINFOHEADER](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader),
[RGBQUAD](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-rgbquad).
Only published field/row specifications were used; no sample implementation,
third-party codec, library, reference-editor code or assets were imported.
The new source remains original Pix86 code under the repository MIT licence.

## Streaming and bounds

`export.c` maps immutable Animation data to bounded spans. `bmp.c` uses a
128-byte pixel buffer plus a 54-byte header and four-byte palette entry; even a
whole scanline is never allocated. Width/height, alignment, image bytes and total
file bytes are checked with unsigned-long arithmetic before writing. The encoder
accepts positive dimensions up to 32767 with BMP 32-bit byte limits; document
sheets additionally obey Pix86's frame/size limits. Grid columns are deliberately
not clamped to frame count. A 64×64 document with four frames and 16 columns
produces a 1024×64, 66,614-byte BMP, crossing 64 KiB via small I/O calls.

The editor reuses its existing export/native I/O scratch snapshot; there is no
additional image heap or static pixel allocation. Final Watcom measurements:
EXE 54,204 bytes; near code 49,032 bytes; DGROUP 38,784 bytes including 4096-byte
stack; linked image 87,840 bytes. The seven animation allocations remain 114,968
bytes, so linked + reserved heap is 202,808 bytes before runtime/loader overhead.

## Verification (2026-11-15)

Nine strict C89 host suites and nine ASan/UBSan suites pass. Watcom application
and DOS tests build with zero warnings under `-wx -we -w4`. All six DOSBox-X
suites pass: EDTEST, IOTEST, UITEST, KBDTEST, ANIMTEST and BMPTEST. BMPTEST uses
an independent C decoder to check exact header fields, palette, all indices,
frame order, all layouts, padding, unused grid cells, DOS names, overwrite,
preflight collision, partial batch/disk-full, failed flush/close, old-target
preservation, size validation, >64-KiB streams and unchanged dirty/history state.
IOTEST retains native replacement/rollback fault coverage through the shared
transaction. UITEST/paint tests cover export keyboard/mouse controls, disabled
focus skipping, cancel, overwrite default/approval, idle zero writes and exact
restoration with one startup clear.

`python3 tests/bmpcheck.py build/bmptest-files` independently opens the generated
fixtures with Pillow and compares every index, dimension and all 256 palette
entries. Pillow is an external test tool only, not an application dependency.
ImageMagick also identifies the 8-bit BMP3 outputs correctly. The separately
built original `BMPVIEW.EXE` diagnostic decodes/displays a native exported BMP
in DOSBox-X; it is not linked into PIX86.EXE. It supports 8-bit BI_RGB images up
to 320×200. Run it from a disposable mounted directory as `..\BMPVIEW ONE.BMP`;
any key returns to text mode. Larger outputs are checked by streaming BMPTEST.

Actual native mouse/keyboard acceptance opens an original three-frame fixture,
exports every mode (including a two-column grid), verifies all pixels with
Pillow, cancels/retries overwrite, and confirms the native project is byte-for-byte
unchanged. Stationary export-dialog captures one second apart are identical.

[Export modal at native 320×200](EXPORT.PNG) · [Unfiltered 2×](EXPORT2X.PNG) ·
[Independent DOS BMP viewer](BMPVIEW.PNG).

Native FreeDOS/MS-DOS, physical disks/VGA/mouse hardware and a physically full
disk remain untested. Disk-full is injected at the fwrite boundary. PNG/GIF,
alpha, custom palettes, spacing options and animation timing export are absent.

## Transparency and streaming PNG

The dialog adds Format, Alpha and Background rows. BMP always shows deliberate
Flatten; PNG defaults to Preserve and can select Flatten. Background is a palette
index used only for flattening. Irrelevant rows are dimmed and skipped. Switching
format updates a recognized filename extension. Every mode supports both formats.

BMP retains indexed bytes for opaque pixels and replaces transparent pixels with
the selected flatten index. Its palette is the fixed VGA DAC palette converted
by (component*255+31)/63. Normal indexed BMP has no alpha. The low-level exporter
rejects masked BMP Preserve rather than silently losing transparency. Unused BMP
sheet cells use the flatten background. PNG Preserve emits RGBA with alpha 0 or
255 from the independent mask; hidden index RGB is retained. Unused PNG sheet
cells are transparent (their RGB is document BG). PNG Flatten uses the chosen
background and alpha 255. Preview/checker/grid/onion/UI are never export inputs.

`png.c` is an original ANSI C writer, independent of DOS graphics/input. It uses
RGBA8, filter 0, stored DEFLATE blocks, bounded IDAT chunks, computed CRC32 and
Adler32. Buffers hold at most 32 RGBA pixels (128 bytes); no complete row or sheet
allocation is required. Output size/dimension arithmetic is checked before writing;
all writes are bounded across 64 KiB segment boundaries. Stored compression is
intentionally larger than compressed PNG but needs no third-party library.

Specifications consulted, without importing source or assets:
[W3C PNG](https://www.w3.org/TR/png-3/),
[zlib RFC 1950](https://www.rfc-editor.org/rfc/rfc1950.html), and
[DEFLATE RFC 1951](https://www.rfc-editor.org/rfc/rfc1951.html).
The existing Microsoft BMP specification citations above remain authoritative for
BMP. No new dependencies, attribution or redistribution obligations are introduced.
