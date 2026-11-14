#!/usr/bin/env python3
"""Authored nine-piece generic skins; no game assets or decoder output.

Each of the eight border pieces is exactly 3x2, index8 is a 1x1 fill.
Frame order is the original TL,T,TR,L,R,BL,B,BR,fill. Literal ETRLE
streams make each border distinguishable in independent C++ pixel grids.
"""
# P710 native snapshot update: tests/native-core/generate_generic_button_fixtures.py
# Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-14.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

from pathlib import Path
import argparse
from generate_image_fixtures import collection
from generate_render_fixtures import GLYPHS

PALETTE = bytes([0,0,0, 255,0,0, 0,255,0, 0,0,255, 255,255,0,
                 255,0,255, 0,255,255, 255,255,255, 128,128,128, 64,64,64])
BORDERS = [bytes.fromhex(s) for s in (
    '03 01 01 01 00 03 01 01 01 00',
    '03 02 02 02 00 03 02 02 02 00',
    '03 03 03 03 00 03 03 03 03 00',
    '03 04 04 04 00 03 04 04 04 00',
    '03 05 05 05 00 03 05 05 05 00',
    '03 06 06 06 00 03 06 06 06 00',
    '03 07 07 07 00 03 07 07 07 00',
    '03 08 08 08 00 03 08 08 08 00')]

def fixture(frames, palette=PALETTE):
    data=bytearray(collection(frames)); data[64:64+len(palette)]=palette; return data

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path); a=parser.parse_args()
    root=a.output.absolute()
    if any(p.is_symlink() for p in (root,*root.parents)): parser.error('symlink output directory')
    root.mkdir(parents=True,exist_ok=True)
    def write(name,data):
        leaf=root/name
        if leaf.is_symlink(): raise ValueError('symlink output leaf')
        leaf.write_bytes(data)
    frames=[(3,2,0,0,stream) for stream in BORDERS]+[(1,1,0,0,bytes.fromhex('01 09 00'))]
    write('generic_off.sti',fixture(frames))
    # All nonzero border entries white in the pressed/hilite/gray skin. The
    # fill pixel still comes from off skin, per original source contract.
    pale=bytes([0,0,0])+bytes([255,255,255])*9
    write('generic_on.sti',fixture(frames,pale))
    for name,palette in [('GENBUTN.STI',PALETTE),('GENBUTN2.STI',pale),('GENBUTN3.STI',pale),('GENBUTN4.STI',pale)]:
        write(name,fixture(frames,palette))
    write('generic_background.sti',fixture([(2,2,0,0,bytes.fromhex('02 01 03 00 02 03 01 00'))]))
    write('generic_icon.sti',fixture([(3,2,1,0,bytes.fromhex('03 01 01 01 00 01 01 81 01 01 00'))]))
    write('generic_short.sti',fixture(frames[:8]))
    write('generic_font.sti',collection(GLYPHS))
    print('Wrote ten authored generic/default button fixtures')

if __name__=='__main__':main()
