#!/usr/bin/env python3
"""Check analyzer-reported runtime objects are initialized on every path."""
# P710 native snapshot update: tests/native-core/check_runtime_object_initialization.py
# Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def section(source: str, start_text: str, end_text: str) -> str:
    start = source.find(start_text)
    end = source.find(end_text, start)
    return source[start:end] if start >= 0 and end >= 0 else ""


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_runtime_object_initialization.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    corpses = (root / "Tactical" / "Rotting Corpses.cpp").read_text(encoding="latin-1")
    soldiers = (root / "Tactical" / "Soldier Control.cpp").read_text(encoding="latin-1")
    corpse = section(corpses, "BOOLEAN AddCorpseFromObject(", "void GetBloodFromCorpse(")
    dims = section(soldiers, "void GetActualSoldierAnimDims(", "void GetActualSoldierAnimOffsets(")
    offsets = section(soldiers, "void GetActualSoldierAnimOffsets(", "void SetSoldierLocatorOffsets(")
    if not corpse or not dims or not offsets:
        print("FAIL: one or more runtime object functions were not found", file=sys.stderr)
        return 1

    dims_prefix = dims[: dims.find("usAnimSurface =")]
    offsets_prefix = offsets[: offsets.find("usAnimSurface =")]
    checks = (
        (
            bool(re.search(
                r"ROTTING_CORPSE_DEFINITION\s+Corpse\s*(?:=\s*)?\{\s*\}\s*;",
                corpse,
            )),
            "corpse flags and fields can begin with indeterminate data",
        ),
        (
            "*psHeight = (INT16)5;" in dims_prefix
            and "*psWidth = (INT16)5;" in dims_prefix,
            "animation dimensions can return without publishing defaults",
        ),
        (
            "*sOffsetX = (INT16)0;" in offsets_prefix
            and "*sOffsetY = (INT16)0;" in offsets_prefix
            and bool(re.search(
                r"if\s*\(\s*pSoldier->usAniFrame\s*>=\s*"
                r"gAnimSurfaceDatabase\[usAnimSurface\]\.hVideoObject->usNumberOfObjects\s*\)\s*"
                r"\{?\s*return\s*;",
                offsets,
                re.DOTALL,
            )),
            "animation offsets can index beyond the surface frame table",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} runtime object checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} runtime object initialization guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
