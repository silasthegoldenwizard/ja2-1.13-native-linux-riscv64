#!/usr/bin/env python3
"""Write authored, synthetic quick-button STI fixtures. Native change 2026-11-12."""
# P710 native snapshot update: tests/native-core/generate_button_fixtures.py
# Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-15.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

import argparse
from pathlib import Path
from generate_image_fixtures import collection
from generate_render_fixtures import GLYPHS

# Literal ETRLE rows: transparent top-left, white border, colored middle.
# Five frames in original quick-image ordering: off, hover, on, on-hover, gray.
TOP = bytes.fromhex('81 07 06 06 06 06 06 06 06 00')
BOTTOM = bytes.fromhex('08 06 06 06 06 06 06 06 06 00')
BLUE = bytes.fromhex('08 06 03 03 03 03 03 03 06 00')
GREEN = bytes.fromhex('08 06 02 02 02 02 02 02 06 00')
RED = bytes.fromhex('08 06 01 01 01 01 01 01 06 00')
YELLOW = bytes.fromhex('08 06 04 04 04 04 04 04 06 00')
GRAY = bytes.fromhex('08 06 05 05 05 05 05 05 06 00')
PALETTE = bytes([0,0,0, 255,0,0, 0,255,0, 0,0,255,
                 255,255,0, 128,128,128, 255,255,255])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    root = args.output.absolute()
    if any(p.is_symlink() for p in (root, *root.parents)):
        parser.error('symlink output directory')
    root.mkdir(parents=True, exist_ok=True)

    def write(name, data):
        path = root / name
        if path.is_symlink():
            raise ValueError('symlink output leaf')
        path.write_bytes(data)

    data = bytearray(collection([(8, 6, 0, 0, TOP + middle * 4 + BOTTOM)
                                 for middle in (BLUE, GREEN, RED, YELLOW, GRAY)]))
    data[64:64 + len(PALETTE)] = PALETTE
    write('buttons.sti', data)
    # Font retains its existing rendering-fixture palette intentionally.
    write('font.sti', collection(GLYPHS))
    write('bad_buttons.sti', data[:63])
    print('Wrote three synthetic UI fixtures (buttons, font, short header)')


if __name__ == '__main__':
    main()
