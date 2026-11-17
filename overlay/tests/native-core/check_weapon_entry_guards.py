#!/usr/bin/env python3
"""Check weapon-calculation entry points reject missing required objects."""
# P710 native snapshot update: tests/native-core/check_weapon_entry_guards.py
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
        print("usage: check_weapon_entry_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    weapons = (root / "Tactical" / "Weapons.cpp").read_text(encoding="latin-1")
    impact = section(weapons, "INT32 BulletImpact(", "INT32 HTHImpact(")
    hth = section(weapons, "UINT32 CalcChanceHTH(", "UINT32 CalcChanceToStab(")
    thrown = section(weapons, "UINT32 CalcThrownChanceToHit(", "void ChangeWeaponMode(")
    if not impact or not hth or not thrown:
        print("FAIL: one or more weapon function bodies were not found", file=sys.stderr)
        return 1

    checks = (
        (
            bool(re.search(
                r"if\s*\(\s*pTarget\s*==\s*NULL\s*\|\|\s*"
                r"\(\s*pBullet\s*==\s*NULL\s*&&\s*pFirer\s*==\s*NULL\s*\)\s*\)\s*"
                r"return\s*\(?\s*0\s*\)?\s*;",
                impact,
                re.DOTALL,
            )),
            "BulletImpact can dereference a missing target or missing impact source",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*pAttacker\s*==\s*NULL\s*\|\|\s*pDefender\s*==\s*NULL\s*\)\s*"
                r"return\s*\(?\s*0\s*\)?\s*;",
                hth,
                re.DOTALL,
            )),
            "CalcChanceHTH can dereference a missing attacker or defender",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*pSoldier\s*==\s*NULL\s*\)\s*"
                r"return\s*\(?\s*0\s*\)?\s*;",
                thrown,
                re.DOTALL,
            )),
            "CalcThrownChanceToHit can dereference a missing soldier",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} weapon entry checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} weapon entry guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
