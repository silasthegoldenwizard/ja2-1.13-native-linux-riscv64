#!/usr/bin/env python3
"""Check the production climb-point soldier boundary."""
# P710 native snapshot update: tests/native-core/check_climb_point_guards.py
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
        print("usage: check_climb_point_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "TileEngine" / "Buildings.cpp").read_text(
        encoding="latin-1"
    )
    match = re.search(
        r"INT32\s+FindClosestClimbPoint\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    )
    if not match:
        print("FAIL: FindClosestClimbPoint body was not found", file=sys.stderr)
        return 1
    body = match.group("body")

    guarded = bool(re.search(
        r"^\s*if\s*\(\s*pSoldier\s*==\s*NULL\s*\)\s*\{?\s*"
        r"return\s*\(\s*NOWHERE\s*\)\s*;",
        body,
        re.DOTALL,
    ))
    if not guarded:
        print("FAIL: a missing soldier can reach climb-point field reads", file=sys.stderr)
        return 1

    print("PASS: climb-point soldier pointer is guarded")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
