#!/usr/bin/env python3
"""Convert the first-party DOS profile logs to CSV on stdout."""
import csv
import re
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1] / 'build' / 'adaptive'
pattern = re.compile(
    r'BENCH (\d+)x(\d+) (\d+)% ([^/]+)/([^ ]+) z(\d+) '
    r'strategy=(\d+) n=(\d+) pit=(\d+) us/frame=(\d+) rects=(\d+)')
writer = csv.writer(sys.stdout, lineterminator='\n')
writer.writerow(['build', 'mode', 'width', 'height', 'percent', 'pattern',
                 'distribution', 'zoom', 'strategy', 'samples', 'pit_units',
                 'microseconds_per_frame', 'rectangles'])
for mode in ('VGA', 'VBE'):
    for build in ('BASEPERF', 'PERFDOS'):
        source = root / (build + '-' + mode + '.TXT')
        if source.exists():
            for match in pattern.finditer(source.read_text()):
                writer.writerow([build, mode] + list(match.groups()))
