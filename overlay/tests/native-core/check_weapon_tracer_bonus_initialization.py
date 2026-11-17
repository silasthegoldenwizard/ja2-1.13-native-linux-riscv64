#!/usr/bin/env python3
"""Check the old-CTH tracer path resets its reused bonus accumulator."""
# P710 native snapshot update: tests/native-core/check_weapon_tracer_bonus_initialization.py
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
        print("usage: check_weapon_tracer_bonus_initialization.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    source = (root / "Tactical" / "Weapons.cpp").read_text(encoding="latin-1")
    start = source.find("UINT32 CalcChanceToHitGun(")
    end = source.find("UINT32 AICalcChanceToHitGun(", start)
    body = source[start:end] if start >= 0 and end >= 0 else ""
    if not body:
        print("FAIL: CalcChanceToHitGun body was not found", file=sys.stderr)
        return 1

    tracer = re.search(
        r"if\s*\(\s*iTracersFired\s*>\s*0\s*\)\s*\{(?P<body>.*?)"
        r"iChance\s*\+=\s*iBonus\s*;",
        body,
        re.DOTALL,
    )
    if not tracer or not re.search(
        r"iBonus\s*=\s*0\s*;\s*if\s*\(\s*sTotalAutofirePenalty\s*>\s*0\s*\)",
        tracer.group("body"),
        re.DOTALL,
    ):
        print(
            "FAIL: tracer CTH can add a stale or uninitialized iBonus when recoil penalty is zero",
            file=sys.stderr,
        )
        return 1

    print("PASS: old-CTH tracer bonus is initialized before use")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
