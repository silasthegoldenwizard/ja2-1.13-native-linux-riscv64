#!/usr/bin/env python3
"""Independent tiny PNG/Copy7z JPC resources, stdlib and existing 7z only."""
# P710 native notice preparation: tests/native-core/generate_png_fixtures.py
# New candidate test/build/documentation/metadata path.
# Native adaptation recorded in the 2026-11-12 checkpoint.
# Exact first-edit/creation date is not established; this is a checkpoint-record date.
# Scope: add independent synthetic PNG/Copy7z JPC fixture generation.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Notice inserted: 2026-11-12.
# Existing source bytes and original notices retained; no additional rights are granted.
# This record does not establish license applicability, compliance or distribution permission.

# P710 native snapshot update: tests/native-core/generate_png_fixtures.py
# Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-14.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib


def chunk(name, data):
    return struct.pack('>I', len(data)) + name + data + struct.pack('>I', zlib.crc32(name + data))


PALETTE = bytes([0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255]) + bytes(252 * 3)


def png(width, height, kind, data, offset=(-2, 3), palette=PALETTE, offset_unit=0, srgb=False):
    channels = {3: 1, 2: 3, 6: 4}[kind]
    rows = b''.join(b'\0' + data[y * width * channels:(y + 1) * width * channels] for y in range(height))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, kind, 0, 0, 0)) +
            (chunk(b'sRGB', b'\0') if srgb else b'') +
            (chunk(b'PLTE', palette) if kind == 3 else b'') +
            chunk(b'oFFs', struct.pack('>iiB', *offset, offset_unit)) + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=Path)
    parser.add_argument('--seven-zip', default='7z')
    args = parser.parse_args()
    root = args.directory
    root.mkdir(parents=True, exist_ok=True)
    indexed = png(4, 2, 3, bytes([0, 1, 1, 0, 2, 0, 3, 3]))
    rgb = png(3, 1, 2, bytes([255, 0, 0, 0, 255, 0, 0, 0, 255]))
    rgba = png(2, 1, 6, bytes([12, 34, 56, 78, 90, 123, 210, 255]))
    fixtures = {'indexed.png': indexed, 'rgb.png': rgb, 'rgba.png': rgba,
                'indexed_srgb.png': png(4, 2, 3, bytes([0, 1, 1, 0, 2, 0, 3, 3]), srgb=True),
                'surface_rgba.png': png(3, 1, 6, bytes([250, 120, 65, 0,
                                                       12, 34, 56, 78,
                                                       90, 123, 210, 255])),
                'surface_indexed_key.png': png(2, 1, 3, bytes([0, 1]),
                                               palette=bytes([255, 0, 0, 0, 255, 0]) + bytes(254 * 3)),
                'one.png': png(1, 1, 3, b'\0'),
                'micrometer.png': png(4, 2, 3, bytes([0, 1, 1, 0, 2, 0, 3, 3]), offset_unit=1),
                'trailing.png': indexed + chunk(b'oFFs', struct.pack('>iiB', 123, 456, 0)) + b'ignored trailer',
                'long.png': png(130, 1, 3, bytes([1] * 128 + [0, 0]))}
    for name, data in fixtures.items():
        (root / name).write_bytes(data)
    bad = {'bad_signature.png': b'not a png', 'bad_truncated.png': indexed[:-5],
           'bad_palette.png': png(1, 1, 3, b'\0', palette=bytes(3)),
           'bad_offset.png': png(1, 1, 3, b'\0', offset=(32768, 0)),
           'bad_crc.png': indexed[:-1] + bytes([indexed[-1] ^ 1])}
    huge = bytearray(rgb)
    huge[16:20] = struct.pack('>I', 0x80000000)
    bad['bad_dimension.png'] = bytes(huge)
    bad['bad_depth.png'] = rgb[:24] + b'\x10' + rgb[25:]
    # Valid PNG but unsupported gray representation must fail without a palette.
    gray_rows = zlib.compress(b'\0\x7f')
    bad['bad_gray.png'] = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB',1,1,8,0,0,0,0)) + chunk(b'IDAT', gray_rows) + chunk(b'IEND', b'')
    for name, data in bad.items():
        (root / name).write_bytes(data)
    xml = b'''<ImageData><SubImage index="0"><offset x="-7" y="8"/><AuxObjectData><ubWallOrientation>9</ubWallOrientation><ubNumberOfTiles>2</ubNumberOfTiles><usTileLocIndex>4660</usTileLocIndex><ubCurrentFrame>1</ubCurrentFrame><ubNumberOfFrames>2</ubNumberOfFrames><flags><AUX_FULL_TILE/><AUX_ANIMATED_TILE/><AUX_USES_LAND_Z/></flags></AuxObjectData></SubImage><SubImage index="2"><offset x="5"/></SubImage></ImageData>'''

    def archive(name, entries):
        target = (root / name).resolve()
        if target.exists():
            target.unlink()
        with tempfile.TemporaryDirectory(prefix='p710-png-') as temp:
            directory = Path(temp)
            for entry, content in entries.items():
                (directory / entry).write_bytes(content)
            subprocess.run([args.seven_zip, 'a', '-t7z', '-mx=0', '-ms=off', '-mhe=off', str(target), *entries], cwd=directory, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)

    frame2 = png(1, 1, 3, b'\x02', offset=(1, -1))
    frame10 = png(1, 1, 3, b'\x03', offset=(2, -3))
    entries = {'10.png': frame10, '0.png': indexed, '2.png': frame2, 'app.xml': xml}
    archive('frames.jpc.7z', entries)
    archive('single_rgb.jpc.7z', {'0.png': rgb})
    archive('single_rgba.jpc.7z', {'0.png': rgba, 'app.xml': b'<ImageData><SubImage index="0"><offset y="6"/></SubImage></ImageData>'})
    archive('single_indexed.jpc.7z', {'0.png': indexed})
    badxml = {'negative': b'<ImageData><SubImage index="-1"/></ImageData>',
              'past_end': b'<ImageData><SubImage index="3"/></ImageData>',
              'duplicate': b'<ImageData><SubImage index="0"/><SubImage index="0"/></ImageData>',
              'overflow': xml.replace(b'>4660<', b'>65536<'),
              'junk': xml.replace(b'>4660<', b'>12x<'),
              'frames': xml.replace(b'>2</ubNumberOfFrames>', b'>4</ubNumberOfFrames>'),
              'offset': xml.replace(b'x="-7"', b'x="-32769"'),
              'entity': b'<!DOCTYPE ImageData [<!ENTITY data "test">]><ImageData/>',
              'malformed': b'<ImageData><SubImage index="0">'}
    for label, content in badxml.items():
        archive('bad_xml_' + label + '.jpc.7z', {**entries, 'app.xml': content})
    archive('bad_names.jpc.7z', {'abc.png': indexed})
    archive('bad_duplicate_index.jpc.7z', {'0.png': indexed, '00.png': indexed})
    archive('bad_rgb_frames.jpc.7z', {'0.png': rgb, '1.png': rgb})
    archive('bad_empty.jpc.7z', {'note.txt': b'no image'})
    archive('bad_member.jpc.7z', {'0.png': indexed[:-5]})
    (root / 'bad_archive.jpc.7z').write_bytes(b'not 7z')
    (root / 'png_manifest.json').write_text(json.dumps({'independently_authored': True, 'valid_png': list(fixtures), 'malformed_png': list(bad), 'valid_jpc': 4, 'malformed_jpc': 15}, indent=2) + '\n')


if __name__ == '__main__':
    main()
