#!/usr/bin/env python3
"""Check autoresolve reinforcement and AIM biography default state."""
# P710 native snapshot update: tests/native-core/check_autoresolve_aim_state_guards.py
# Native Linux source/build/test adaptations captured in the accepted strategic runtime-state checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def extract(source: str, start_marker: str, end_marker: str) -> str:
    start = source.find(start_marker)
    end = source.find(end_marker, start + len(start_marker))
    return source[start:end] if start >= 0 and end >= 0 else ""


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_autoresolve_aim_state_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    autoresolve_source = (root / "Strategic" / "Auto Resolve.cpp").read_text(
        encoding="latin-1"
    )
    aim_source = (root / "Laptop" / "AimMembers.cpp").read_text(encoding="latin-1")

    autoresolve = extract(
        autoresolve_source,
        "static void AssociateEnemiesWithStrategicGroups()",
        "void CalculateSoldierCells(",
    )
    aim_update = extract(
        aim_source,
        "BOOLEAN\tUpdateMercInfo(void)\n{",
        "BOOLEAN LoadMercBioInfo(",
    )
    if not autoresolve or not aim_update:
        print("FAIL: one or more guarded production functions were not found", file=sys.stderr)
        return 1

    checks = []
    for name in (
        "ubISNumAdmins",
        "ubISNumTroops",
        "ubISNumElites",
        "ubISNumTanks",
        "ubISNumJeeps",
        "ubISNumRobots",
    ):
        checks.append(
            (
                f"autoresolve initializes {name}",
                bool(re.search(rf"\b{name}\s*=\s*0\s*;", autoresolve)),
            )
        )

    checks.extend(
        [
            (
                "AIM biography buffer starts empty",
                bool(
                    re.search(
                        r"CHAR16\s+MercInfoString\s*\[\s*SIZE_MERC_BIO_INFO\s*\]"
                        r"\s*=\s*\{\s*0\s*\}\s*;",
                        aim_update,
                    )
                ),
            ),
            (
                "AIM additional-info buffer starts empty",
                bool(
                    re.search(
                        r"CHAR16\s+AdditionalInfoString\s*\[\s*SIZE_MERC_BIO_INFO\s*\]"
                        r"\s*=\s*\{\s*0\s*\}\s*;",
                        aim_update,
                    )
                ),
            ),
        ]
    )

    failures = 0
    for label, passed in checks:
        if passed:
            print(f"PASS: {label}")
        else:
            failures += 1
            print(f"FAIL: {label}", file=sys.stderr)

    if failures:
        print(f"FAIL: {failures} autoresolve/AIM state checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} autoresolve/AIM state checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
