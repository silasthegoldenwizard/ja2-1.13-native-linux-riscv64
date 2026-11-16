#!/usr/bin/env python3
"""Create authored popup backgrounds, borders, and a bitmap font."""
# P710 native snapshot update: tests/native-core/generate_merc_popup_fixtures.py
# Native Linux source/build/test adaptations captured in the accepted virtual inventory and lifetime checkpoint dated 2026-11-16.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-16.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

import argparse
from pathlib import Path

from generate_image_fixtures import collection
from generate_render_fixtures import GLYPHS
from generate_resource_fixtures import pcx


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    root = parser.parse_args().output.absolute()
    if any(path.is_symlink() for path in (root, *root.parents)):
        parser.error("fixture output must not use symlinks")
    root.mkdir(parents=True, exist_ok=True)
    interface = root / "INTERFACE"
    if interface.is_symlink():
        parser.error("fixture interface directory must not be a symlink")
    interface.mkdir(exist_ok=True)
    # Independent literal background: every indexed sample is palette green.
    row = bytes([0xFF, 2]) * 5 + bytes([0xE3, 2])
    background = pcx(row * 207, width=350, height=207, stride=350)
    # Eight real ETRLE frames match the production popup border's frame indices.
    tile = bytes([10] + [1] * 10 + [0]) * 10
    border = collection([(10, 10, 0, 0, tile)] * 8)
    fixtures = {
        "font.sti": collection(GLYPHS),
        "INTERFACE/TactPopupBackground.pcx": background,
        "INTERFACE/TactPopupBackgroundMain.pcx": background,
        "INTERFACE/TactPopUp.sti": border,
        "INTERFACE/TactPopUpMain.sti": border,
    }
    for name, data in fixtures.items():
        output = root / name
        if output.is_symlink():
            parser.error("fixture output leaf must not be a symlink")
        output.write_bytes(data)
    print("Wrote five authored popup rendering fixtures")


if __name__ == "__main__":
    main()
