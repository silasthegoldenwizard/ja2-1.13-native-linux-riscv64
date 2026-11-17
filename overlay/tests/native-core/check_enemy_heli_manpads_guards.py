#!/usr/bin/env python3
"""Check production enemy-helicopter MANPADS pointer boundaries."""
# P710 native snapshot update: tests/native-core/check_enemy_heli_manpads_guards.py
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
        print("usage: check_enemy_heli_manpads_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Strategic" / "ASD.cpp").read_text(
        encoding="latin-1"
    )
    start = source.find("void EnemyHeliMANPADSCheck(")
    end = source.find("void EnemyHeliCheckPlayerKnowledge(", start)
    if start < 0 or end < 0:
        print("FAIL: EnemyHeliMANPADSCheck body was not found", file=sys.stderr)
        return 1
    body = source[start:end]

    checks = (
        (
            bool(re.search(
                r"for\s*\(\s*uiCnt\s*=\s*0\s*;.*?\+\+uiCnt\s*\)\s*\{\s*"
                r"pSoldier\s*=\s*MercPtrs\s*\[\s*uiCnt\s*\]\s*;",
                body,
                re.DOTALL,
            )),
            "the MANPADS loop does not resolve each merc slot independently",
        ),
        (
            bool(re.search(
                r"pSoldier\s*&&\s*pSoldier->bActive\s*&&.*?"
                r"!pSoldier->bSectorZ\s*&&\s*\(\s*\(.*?"
                r"pSoldier->sSectorY\s*==\s*heli_y\s*-\s*1\s*\)\s*\)",
                body,
                re.DOTALL,
            )),
            "adjacent-sector tests can bypass the live-merc checks",
        ),
        (
            body.count("pAttachment == NULL") >= 2
            and "pAttachment->exists" not in body,
            "a launcher without a grenade or bomb can dereference a null attachment",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} MANPADS checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} enemy-helicopter MANPADS pointer guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
