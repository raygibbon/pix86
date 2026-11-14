# Pix86 project format (.P86), version 1

This is an original Pix86 specification. It contains one indexed image and the
foreground/background selections. No external file-format code, fixture or
reference-editor assets were used. All test files are generated from this
specification by original tests.

## Encoding

The file is exactly a 24-byte header followed by width × height pixel bytes.
Integers are unsigned, explicitly **little endian**, regardless of C integer
size or structure padding. Offsets are zero-based. No native C struct is written.

| Offset | Bytes | Field | Version 1 requirement |
| ---: | ---: | --- | --- |
| 0 | 4 | Signature | Hex `50 38 36 1A` (ASCII P86 followed by Ctrl+Z) |
| 4 | 2 | Version | 1 |
| 6 | 2 | Header length | 24 |
| 8 | 2 | Width | 16, 32 or 64 |
| 10 | 2 | Height | Equal to width |
| 12 | 1 | Foreground index | 0–255 |
| 13 | 1 | Background index | 0–255 |
| 14 | 2 | Flags | Zero |
| 16 | 4 | Payload length | Exactly width × height |
| 20 | 4 | Reserved | Zero |
| 24 | width × height | Indexed pixels | Row-major, top row first, left to right |

Pixel (x,y) is at offset `24 + y * width + x`, for 0 ≤ x < width and
0 ≤ y < height. There is no compression, stride padding, terminator, checksum,
RGB palette block, transparency flag or trailer. Index zero is an ordinary
colour, not a transparency sentinel. Foreground/background are selections,
not a replacement for any pixel byte.

The only supported total lengths are 280, 1048 and 4120 bytes. Readers must
validate signature, version, header length, flags, reserved bytes, dimensions,
payload length and exact end of file. Reject unsupported versions, non-square
or unsupported dimensions, truncation and extra trailing bytes before applying
anything to the live document. Version 1 deliberately rejects nonzero unknown
fields instead of guessing how to interpret them.

## Implicit fixed palette

Version 1 uses Pix86's fixed VGA DAC palette; the file stores indices only.
DAC channels below range from 0 to 63; current entries use 0..60:

- For index i in 0..15: red = green = blue = 4 × i.
- For index i in 16..255, let q = i − 16:
  red = 12 × (q mod 6), green = 12 × (floor(q / 6) mod 6),
  blue = 12 × (floor(q / 36) mod 6).

Some RGB entries repeat; their indices remain distinct and must be preserved.
A display reader may scale each DAC channel to its output range. Custom palette
editing is outside version 1.

## Minimal example

A blank 16×16 image with foreground 15 and background 0 has this header:

```text
50 38 36 1A 01 00 18 00 10 00 10 00 0F 00 00 00
00 01 00 00 00 00 00 00
```

It is followed by 256 zero bytes. An independent reader can parse each integer
as the sum of byte[n] × 256^n, then read exactly the declared payload.

## Future extension

The explicit version, header length, payload length, flags and reserved space
leave a clear boundary for future format revisions. A later version may define
frame/chunk tables or a different payload. Version 1 contains **no animation
data** and must not be silently reinterpreted as a multi-frame format. Future
writers must change the version when changing these semantics; current Pix86
will reject such files cleanly and retain the existing artwork.

## Filenames and replacement

The application supplies `.P86` when the filename has no extension, accepts DOS
8.3 path components, and uses uppercase canonical names. The payload itself
never stores a filename. Other explicit 1–3 character extensions are accepted,
but format detection always uses the header, not the extension.

Saving writes an exclusively created `P86nnnnn.TMP` in the target directory,
checks write, flush and close, and only then attempts installation. Replacing
an existing target requires explicit UI confirmation. The old file is renamed
to a free `P86nnnnn.BAK`, then the completed temporary file is renamed to the
target. The backup is removed only after installation succeeds. A failed
installation attempts to restore the backup; if rollback also fails, both
recovery files are kept and their paths displayed. Do not delete them: the BAK
holds the old file and the TMP holds the completed new file. If only backup
cleanup fails, saving succeeded and the old backup is reported as retained.

This is a single-writer DOS strategy, not a concurrent-writer transaction or a
power-failure-atomic filesystem protocol. A reset between renames can leave a
BAK/TMP needing manual recovery. Pix86 does not automatically remove stale
recovery files or overwrite a colliding temporary/backup name. There are 100
candidate names per suffix; exhausting them fails safely. Read-only, inaccessible
or non-file targets fail without truncating them. Filesystem/media failures
that prevent rollback are reported explicitly; no software can guarantee a
rename will succeed on failing media.
