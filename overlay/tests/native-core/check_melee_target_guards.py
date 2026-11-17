#!/usr/bin/env python3
"""Check production melee target pointer boundaries."""
# P710 native snapshot update: tests/native-core/check_melee_target_guards.py
# Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def body(source: str, name: str) -> str:
    match = re.search(
        rf"void\s+SOLDIERTYPE::{name}\s*\([^)]*\)\s*\{{(?P<body>.*?)\n\}}",
        source,
        re.DOTALL,
    )
    return match.group("body") if match else ""


def guarded(function: str) -> bool:
    return bool(re.search(
        r"GetSoldier\s*\(\s*&pTSoldier\s*,\s*usSoldierIndex\s*\)\s*;\s*"
        r"if\s*\(\s*pTSoldier\s*==\s*NULL\s*\)\s*\{?\s*return\s*;",
        function,
        re.DOTALL,
    ))


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_melee_target_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Tactical" / "Soldier Control.cpp").read_text(
        encoding="latin-1"
    )
    blade = body(source, "EVENT_SoldierBeginBladeAttack")
    punch = body(source, "EVENT_SoldierBeginPunchAttack")
    if not blade or not punch:
        print("FAIL: melee attack bodies were not found", file=sys.stderr)
        return 1

    checks = (
        (guarded(blade), "blade attack can use a missing target soldier"),
        (guarded(punch), "punch attack can use a missing target soldier"),
    )
    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} melee target checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} melee target pointer guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
