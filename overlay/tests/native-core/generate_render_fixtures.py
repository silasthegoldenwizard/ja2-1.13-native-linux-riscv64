#!/usr/bin/env python3
"""Authored ETRLE fixtures and independent raw/BMP scene verification."""
# P710 native notice preparation: tests/native-core/generate_render_fixtures.py
# New candidate test/build/documentation/metadata path.
# Native adaptation recorded in the 2026-11-12 checkpoint.
# Exact first-edit/creation date is not established; this is a checkpoint-record date.
# Scope: add authored ETRLE/font scene fixtures and independent raw/BMP verification.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Notice inserted: 2026-11-12.
# Existing source bytes and original notices retained; no additional rights are granted.
# This record does not establish license applicability, compliance or distribution permission.

# P710 native snapshot update: tests/native-core/generate_render_fixtures.py
# Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-14.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

import argparse
from pathlib import Path
import struct
from generate_image_fixtures import collection

# Literal encodings, not obtained from a renderer or an encoder of these grids.
A = bytes.fromhex('81 01 02 81 00 03 02 01 02 00 03 02 00 02 00')
B = bytes.fromhex('02 03 03 00 01 03 81 00 02 03 01 00')
SPACE = bytes.fromhex('82 00 82 00 82 00')
GLYPHS = [(3, 3, 1, 0, A), (2, 3, 0, 1, B), (2, 3, 0, 0, SPACE)]


def write(root, name, data):
    p = root / name
    if p.is_symlink():
        raise ValueError('symlink output leaf')
    p.write_bytes(data)


def scene():
    # Independent literal coordinate grids. None means no write; opaque zero
    # in colored A is black, while mono A's zero background is skipped.
    p = [[0x2104] * 32 for _ in range(20)]
    for y in range(1, 19):
        for x in range(1, 31):
            p[y][x] = 0x4208
    tile = [[0x001f] * 8, [0x001f] + [0xf800] * 6 + [0x001f],
            [0x001f] + [0xf800] * 6 + [0x001f], [0x001f] * 8]
    mono_a = [[None, 0xffff, None], [0xffff, 0x07e0, 0xffff],
              [0xffff, None, 0xffff]]
    mono_b = [[0xffff, 0xffff], [0xffff, None], [0xffff, 0x07e0]]
    color_b = [[0x001f, 0x001f], [0x001f, None], [0x001f, 0xf800]]
    color_a = [[None, 0x07e0, None], [0x07e0, 0xf800, 0x07e0],
               [0x07e0, 0, 0x07e0]]
    for ox, oy, grid in [(2, 3, tile), (13, 3, mono_a), (16, 4, mono_b),
                         (20, 11, color_b), (23, 10, color_a)]:
        for y, row in enumerate(grid):
            for x, c in enumerate(row):
                if c is not None:
                    p[oy+y][ox+x] = c
    return p


def verify(bmp):
    expected = scene()
    raw = bmp.with_suffix('.rgb565').read_bytes()
    assert len(raw) == 72 * 20, 'raw allocation size'
    checks = 1
    for y, row in enumerate(expected):
        for x, c in enumerate(row):
            assert struct.unpack_from('<H', raw, y*72+x*2)[0] == c, (x, y)
            checks += 1
        assert raw[y*72+64:(y+1)*72] == b'\xa5'*8, ('padding', y)
        checks += 1
    b = bmp.read_bytes()
    assert b[:2] == b'BM'
    offset = struct.unpack_from('<I', b, 10)[0]
    dib, w, h = struct.unpack_from('<Iii', b, 14)
    depth = struct.unpack_from('<H', b, 28)[0]
    compression = struct.unpack_from('<I', b, 30)[0]
    assert dib >= 40 and w == 256 and abs(h) == 160 and depth in (24, 32)
    assert compression in (0, 3)
    if compression == 3:
        assert struct.unpack_from('<III', b, 54) == (0xff0000, 0xff00, 0xff)
    stride = ((w*depth+31)//32)*4
    assert len(b) >= offset+stride*abs(h)
    checks += 5
    for y in range(abs(h)):
        by = y if h < 0 else h-1-y
        for x in range(w):
            blue, green, red = b[offset+by*stride+x*(depth//8):][:3]
            # Compare every channel's source precision, allowing SDL's 5/6-bit
            # expansion rounding without hiding any RGB565 pixel difference.
            actual = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3)
            assert actual == expected[y//8][x//8], ('BMP', x, y)
            checks += 1
    print(f'PASS: {checks} independent raw/padding/BMP assertions')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('output', type=Path)
    ap.add_argument('--verify', action='store_true', help='output is viewer BMP')
    a = ap.parse_args()
    if a.verify:
        verify(a.output)
        return
    root = a.output.absolute()
    if any(p.is_symlink() for p in (root, *root.parents)):
        ap.error('symlink output directory')
    root.mkdir(parents=True, exist_ok=True)
    write(root, 'font.sti', collection(GLYPHS))
    write(root, 'font_alt.sti', collection([(2, 3, 0, 1, B), GLYPHS[1], GLYPHS[2]]))
    write(root, 'bad_run.sti', collection([(3, 1, 0, 0, bytes.fromhex('84 00'))]))
    bad = bytearray(collection(GLYPHS))
    struct.pack_into('<I', bad, 64+768, 0xffffffff)
    write(root, 'bad_descriptor.sti', bad)
    print('Wrote four synthetic rendering STI fixtures')


if __name__ == '__main__':
    main()
