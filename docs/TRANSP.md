# Transparency implementation plan

Preserve Mode 13h, 8086 small-model DOS, the existing Canvas/tools/history,
frame limits, file transactions, modal routing and dirty-region rendering.

1. Neutral continuous pixel grid, with independently toggled 8/16 guides.
2. Separate BG erase/fill selection, view-only preview colour and artwork.
3. Packed mask: 1 means transparent, independent of the indexed byte. Reserve
   2048 bytes per Animation and 512 active-frame bytes. Seven snapshots still
   fit below 32 KiB each; DGROUP increase is about 2.5 KiB, no new allocations.
   Pencil/fill make pixels opaque; transparent eraser preserves hidden indices.
   Blank-frame policy is explicit document metadata; switching it leaves existing
   pixels unchanged. Preview checkerboard/colour/link and eraser mode are UI state.
4. P86 v3 retains the 24-byte header. Byte 22 is default transparent-blank policy,
   byte 23 zero. Each frame has duration, index bytes, then packed mask. Old v1/v2
   load opaque. Mask/default policy participate in history and exact dirty state.
5. BMP requires an explicit flatten choice for masked pixels. PNG RGBA uses the
   same RGB conversion and per-pixel alpha, preserving black independently.
6. Original streaming PNG uses filter 0 and stored DEFLATE blocks, small IDAT
   chunks, computed CRC32 and Adler32, without a zlib dependency or large buffers.
7. Extend the existing export modal and document options, then host/native tests,
   independent PNG/BMP readers, low-flicker regressions, memory and screenshots.

No third-party source, assets or libraries are imported. PNG/zlib/DEFLATE field
contracts come from W3C PNG and RFC 1950/1951; implement independently.
