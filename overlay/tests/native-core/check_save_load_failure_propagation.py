#!/usr/bin/env python3
"""Check that LoadSavedGame propagates semantic loader failures."""
# P710 native snapshot update: tests/native-core/check_save_load_failure_propagation.py
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
        print("usage: check_save_load_failure_propagation.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    source = (root / "Ja2" / "SaveLoadGame.cpp").read_text(encoding="latin-1")
    start = source.find("BOOLEAN LoadSavedGame(")
    end = source.find("BOOLEAN SaveMercProfiles(", start)
    body = source[start:end] if start >= 0 and end >= 0 else ""
    if not body:
        print("FAIL: LoadSavedGame body was not found", file=sys.stderr)
        return 1

    loaders = (
        "LoadSquadInfoFromSavedGameFile",
        "LoadVehicleInformationFromSavedGame",
        "LoadMilitiaMovementInformationFromSavedGame",
        "LoadBulletStructureFromSavedGame",
    )
    failures = []
    for loader in loaders:
        if not re.search(rf"if\s*\(\s*!\s*{loader}\s*\(", body):
            failures.append(loader)

    if failures:
        for loader in failures:
            print(f"FAIL: LoadSavedGame ignores {loader} failure", file=sys.stderr)
        print(
            f"FAIL: {len(failures)} of {len(loaders)} top-level loader results are ignored",
            file=sys.stderr,
        )
        return 1

    print(f"PASS: {len(loaders)} top-level save-loader failures propagate")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
