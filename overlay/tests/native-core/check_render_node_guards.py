#!/usr/bin/env python3
"""Check production reveal rendering node ownership."""
# P710 native snapshot update: tests/native-core/check_render_node_guards.py
# Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_render_node_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "TileEngine" / "Render Fun.cpp").read_text(
        encoding="latin-1"
    )
    match = re.search(
        r"void\s+SetGridNoRevealedFlag\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    )
    if not match:
        print("FAIL: SetGridNoRevealedFlag body was not found", file=sys.stderr)
        return 1
    body = match.group("body")

    guarded = bool(re.search(
        r"if\s*\(\s*pNode\s*&&\s*\(\s*pStructure->fFlags\s*&\s*"
        r"STRUCTURE_SLANTED_ROOF\s*\)\s*\)\s*\{.*?"
        r"pNode->uiFlags\s*\|=\s*LEVELNODE_HIDDEN\s*;",
        body,
        re.DOTALL,
    ))
    if not guarded:
        print("FAIL: slanted-roof reveal can hide a missing level node", file=sys.stderr)
        return 1

    print("PASS: slanted-roof reveal level node is guarded")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
