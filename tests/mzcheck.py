#!/usr/bin/env python3
"""Check the plain DOS MZ container; CPU execution is tested in DOSBox-X."""
from pathlib import Path
import struct
import sys

path = Path(sys.argv[1] if len(sys.argv) > 1 else "mnt/PIX86.EXE")
data = path.read_bytes()
if len(data) < 28 or data[:2] != b"MZ":
    raise SystemExit("FAIL: missing DOS MZ header")
words = struct.unpack_from("<14H", data)
_, last, pages, relocations, header_paras, minimum, _, ss, sp, _, ip, cs, table, overlay = words
header_size = header_paras * 16
file_size = (pages - 1) * 512 + (last or 512)
if not (28 <= header_size < len(data) == file_size):
    raise SystemExit("FAIL: inconsistent DOS image size")
if table < 28 or table + relocations * 4 > header_size or overlay:
    raise SystemExit("FAIL: invalid relocation table or overlay")
image_size = len(data) - header_size
if cs * 16 + ip >= image_size or ss * 16 + sp > image_size + minimum * 16:
    raise SystemExit("FAIL: entry point or stack outside DOS allocation")
if header_size >= 64:
    new_header = struct.unpack_from("<I", data, 0x3C)[0]
    if new_header and data[new_header:new_header + 2] in (b"PE", b"NE", b"LE", b"LX"):
        raise SystemExit("FAIL: extended executable header")
print(f"PASS: plain DOS MZ, {len(data)} bytes, {relocations} relocations; no extended header.")
