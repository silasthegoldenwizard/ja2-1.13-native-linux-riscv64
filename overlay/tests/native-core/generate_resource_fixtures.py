#!/usr/bin/env python3
"""Author source-only PCX/TGA/image-chooser fixtures; never reads game data."""
# P710 native notice preparation: tests/native-core/generate_resource_fixtures.py
# New candidate test/build/documentation/metadata path.
# Native adaptation recorded in the 2026-11-12 checkpoint.
# Exact first-edit/creation date is not established; this is a checkpoint-record date.
# Scope: add source-only PCX/TGA/resource chooser fixture generation.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Notice inserted: 2026-11-12.
# Existing source bytes and original notices retained; no additional rights are granted.
# This record does not establish license applicability, compliance or distribution permission.

# P710 native snapshot update: tests/native-core/generate_resource_fixtures.py
# Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-14.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib
import generate_image_fixtures as sti


def pcx(encoded=bytes.fromhex('01 02 c2 03 c1 c1 c2 00 04'), *, width=3, height=2, stride=4):
    header = bytearray(128)
    header[:4] = bytes([10, 5, 1, 8])
    struct.pack_into('<HHHH', header, 4, 0, 0, width - 1, height - 1)
    header[65] = 1
    struct.pack_into('<HH', header, 66, stride, 1)
    return bytes(header) + encoded + b'\x0c' + sti.PALETTE


def tga(depth=24, descriptor=0, *, width=3, height=2):
    h = bytearray(18)
    h[0], h[2] = 3, 2
    struct.pack_into('<HH', h, 12, width, height)
    h[16:18] = bytes([depth, descriptor])
    # File order for default lower-left origin: blue/white/black then red/green/yellow.
    rgb = [(0, 0, 255), (255, 255, 255), (0, 0, 0), (255, 0, 0), (0, 255, 0), (255, 255, 0)]
    if descriptor & 0x20:
        rgb = rgb[3:] + rgb[:3]
    if descriptor & 0x10:
        rgb = rgb[2::-1] + rgb[:2:-1]
    pixels = (b''.join(bytes([b, g, r]) for r, g, b in rgb) if depth == 24 else
              b''.join(struct.pack('<H', ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3)) for r, g, b in rgb))
    return bytes(h) + b'ID!' + pixels


def png():
    def chunk(name, data):
        return struct.pack('>I', len(data)) + name + data + struct.pack('>I', zlib.crc32(name + data))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 2, 1, 8, 2, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress(bytes.fromhex('00 ff 00 00 00 ff 00'))) + chunk(b'IEND', b''))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output_dir', type=Path)
    root = parser.parse_args().output_dir.absolute()
    if any(p.is_symlink() for p in [root, *root.parents]):
        raise SystemExit('fixture root cannot traverse symlinks')
    root.mkdir(parents=True, exist_ok=True)
    fixtures = {'padded.pcx': pcx(), 'default.pcx': pcx(), 'UPPER.PCX': pcx(),
                'rgb24.tga': tga(), 'rgb16.tga': tga(16), 'top.tga': tga(descriptor=0x20),
                'right.tga': tga(descriptor=0x10), 'top_right.tga': tga(descriptor=0x30),
                'chooser.sti': sti.indexed(bytes([0, 1, 2, 3, 4, 5]), 3, 2),
                'chooser.png': png(), 'only.sti': sti.indexed(bytes([0, 1, 2, 3, 4, 5]), 3, 2),
                'only.png': png(), 'fallback.sti': sti.indexed(bytes([0, 1, 2, 3, 4, 5]), 3, 2),
                'direct.png': png(), 'invalid.png': b'invalid PNG',
                'invalid.sti': sti.indexed(bytes([0, 1, 2, 3, 4, 5]), 3, 2),
                'sprite.sti': sti.collection([(7, 5, -2, 1, sti.STREAM0), (3, 2, 2, -1, sti.STREAM1)])}
    # File chooser must choose an existing replacement even if its decode fails.
    fixtures['broken.sti'], fixtures['broken.png'] = fixtures['only.sti'], b'broken'
    p = fixtures['padded.pcx']
    fixtures.update({'bad_short.pcx': b'x', 'bad_palette.pcx': p[:-1],
                     'bad_marker.pcx': p[:-769] + b'\x00' + p[-768:],
                     'bad_zero_run.pcx': pcx(b'\xc0\x00'), 'bad_cross_row.pcx': pcx(b'\xc5\x01\xc3\x02'),
                     'bad_truncated_run.pcx': pcx(b'\xc4'), 'bad_underflow.pcx': pcx(b'\x01'),
                     'bad_overflow.pcx': pcx(b'\xc4\x01\xc5\x02'), 'bad_stride.pcx': pcx(stride=2),
                     'bad_trailing.pcx': pcx(bytes.fromhex('01 02 c2 03 c1 c1 c2 00 04 05')),
                     'bad_zero_size.tga': tga(width=0), 'bad_short.tga': tga()[:-1],
                     'bad_header.tga': tga()[:17]})
    for offset, value, name in [(2, 10, 'bad_rle.tga'), (16, 32, 'bad_depth.tga'), (17, 0xc0, 'bad_interleave.tga')]:
        b = bytearray(tga()); b[offset] = value; fixtures[name] = bytes(b)
    for name, data in fixtures.items():
        target = root / name
        if target.is_symlink():
            raise SystemExit('fixture leaf cannot be a symlink')
        target.write_bytes(data)
    manifest = {'synthetic_only': True, 'fixtures': {name: {'size': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
                                                   for name, data in fixtures.items()}}
    (root / 'resource_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Generated {len(fixtures)} synthetic resource fixtures in {root}')


if __name__ == '__main__':
    main()
