#!/usr/bin/env python3
"""Check vehicle-list growth preserves the live list on allocation failure."""
# P710 native snapshot update: tests/native-core/check_vehicle_allocation_guards.py
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
        print("usage: check_vehicle_allocation_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    source = (root / "Tactical" / "Vehicles.cpp").read_text(encoding="latin-1")
    start = source.find("INT32 AddVehicleToList(")
    end = source.find("BOOLEAN RemoveVehicleFromList(", start)
    body = source[start:end] if start >= 0 and end >= 0 else ""
    if not body:
        print("FAIL: AddVehicleToList body was not found", file=sys.stderr)
        return 1

    first_start = body.find("if( iCount == 0 )")
    first_end = body.find("if( ( iVehicleIdValue == -1 )", first_start)
    first = body[first_start:first_end] if first_start >= 0 and first_end >= 0 else ""
    alloc_pos = first.find("MemAlloc(")
    check_pos = first.find("pNewList == NULL")
    clear_pos = first.find("memset( pNewList")
    publish_pos = first.find("pVehicleList = pNewList")

    checks = (
        (
            0 <= alloc_pos < check_pos < clear_pos < publish_pos,
            "first vehicle allocation is used before a null check",
        ),
        (
            bool(re.search(
                r"pNewList\s*=\s*\(VEHICLETYPE\s*\*\)\s*MemRealloc\s*\(\s*pVehicleList.*?\)\s*;\s*"
                r"if\s*\(\s*pNewList\s*==\s*NULL\s*\)\s*return\s*\(\s*-1\s*\)\s*;\s*"
                r"pVehicleList\s*=\s*pNewList\s*;",
                body,
                re.DOTALL,
            ))
            and "pVehicleList = (VEHICLETYPE *) MemRealloc" not in body,
            "vehicle list reallocation can discard the live allocation on failure",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} vehicle allocation checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} vehicle allocation guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
