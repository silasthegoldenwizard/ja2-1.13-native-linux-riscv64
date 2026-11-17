#!/usr/bin/env python3
"""Check the Bobby Ray scroll-region ownership transition in production source."""
# P710 native snapshot update: tests/native-core/check_bobbyray_region_lifecycle.py
# Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import sys


def check(condition: bool, message: str, failures: list[str]) -> None:
    if not condition:
        failures.append(message)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_bobbyray_region_lifecycle.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Laptop" / "BobbyRMailOrder.cpp").read_text(
        encoding="latin-1"
    )
    start_marker = (
        "if (gfDrawGridColumnMouseRegion == FALSE || "
        "gSelectedGridScrollColumnRegionSize != guidivisor)"
    )
    start = source.find(start_marker)
    end = source.find("gfDrawGridColumnMouseRegion = TRUE;", start)
    if start < 0 or end < 0:
        print("FAIL: Bobby Ray scroll-region rebuild block was not found", file=sys.stderr)
        return 1

    block = source[start:end]
    deleted = block.find("delete [] gSelectedGridScrollColumnRegion;")
    reset = block.find("gSelectedGridScrollColumnRegion = NULL;", deleted)
    size_reset = block.find("gSelectedGridScrollColumnRegionSize = 0;", deleted)
    allocated = block.find("gSelectedGridScrollColumnRegion = new MOUSE_REGION[guidivisor];")
    first_use = block.find("MSYS_DefineRegion( &gSelectedGridScrollColumnRegion[i]")
    stale_size_guard = block.find(
        "if (gSelectedGridScrollColumnRegionSize != guidivisor)", deleted
    )
    size_publish = block.find(
        "gSelectedGridScrollColumnRegionSize = guidivisor;", allocated
    )

    failures: list[str] = []
    check(deleted >= 0, "old region array is released", failures)
    check(
        deleted < reset < allocated,
        "released region pointer is cleared before replacement allocation",
        failures,
    )
    check(
        deleted < size_reset < allocated,
        "released region count is cleared before replacement allocation",
        failures,
    )
    check(
        stale_size_guard < 0 or stale_size_guard > allocated,
        "replacement allocation does not depend on the released array's old size",
        failures,
    )
    check(
        allocated < size_publish < first_use,
        "replacement count is published before the array is used",
        failures,
    )

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        return 1

    print("PASS: 5 Bobby Ray scroll-region ownership invariants")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
