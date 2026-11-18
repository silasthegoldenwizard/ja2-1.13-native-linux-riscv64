#!/usr/bin/env python3
"""Extract the production strategic path constructor and append helper."""
# P710 native snapshot update: tests/native-core/extract_strategic_path_construction.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


import json
from pathlib import Path
import sys


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: extract_strategic_path_construction.py ENGINE_ROOT OUTPUT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    output = Path(sys.argv[2])
    source = root / "Strategic" / "Strategic Pathing.cpp"
    text = source.read_text(encoding="latin-1")
    start = text.find("PathStPtr BuildAStrategicPath(")
    end = text.find("/*\nBOOLEAN TravelBetweenSectorsIsBlockedFromVehicle(", start)
    if start < 0 or end < 0:
        print("FAIL: strategic path constructor source was not found", file=sys.stderr)
        return 1

    line = text.count("\n", 0, start) + 1
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(
        f"#line {line} {json.dumps(str(source))}\n" + text[start:end] + "\n",
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
