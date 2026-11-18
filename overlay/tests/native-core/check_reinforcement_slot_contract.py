#!/usr/bin/env python3
"""Check that reinforcement capacity and creation share one usable-slot rule."""
# P710 native snapshot update: tests/native-core/check_reinforcement_slot_contract.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def extract(source: str, start: str, end: str) -> str:
    begin = source.find(start)
    finish = source.find(end, begin + len(start)) if begin >= 0 else -1
    return source[begin:finish] if begin >= 0 and finish >= 0 else ""


def compact(source: str) -> str:
    return re.sub(r"\s+", " ", source)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_reinforcement_slot_contract.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    overhead = compact((root / "Tactical" / "Overhead.cpp").read_text(encoding="latin-1"))
    header = compact((root / "Tactical" / "Overhead.h").read_text(encoding="latin-1"))
    queen = compact((root / "Strategic" / "Queen Command.cpp").read_text(encoding="latin-1"))
    create = compact((root / "Tactical" / "Soldier Create.cpp").read_text(encoding="latin-1"))

    helper = extract(overhead, "BOOLEAN GetInactiveSoldierSlot(", "static BOOLEAN NextAIToHandle(")
    init = extract(overhead, "BOOLEAN InitOverhead( )", "BOOLEAN ShutdownOverhead( )")
    capacity = extract(queen, "UINT16 NumFreeSlots(", "BOOLEAN PrepareEnemyForSectorBattle(")
    selection = extract(create, "SOLDIERTYPE* TacticalCreateSoldier(", "BOOLEAN TacticalCopySoldierFromProfile(")
    if not all((helper, init, capacity, selection)):
        print("FAIL: reinforcement slot functions were not found", file=sys.stderr)
        return 1

    checks = [
        (
            "the inactive-slot predicate is shared through the tactical header",
            "BOOLEAN GetInactiveSoldierSlot( SOLDIERTYPE **ppSoldier, SoldierID usSoldierIndex );" in header,
        ),
        (
            "the predicate rejects missing storage and active records",
            "pSoldier == NULL || pSoldier->bActive" in helper
            and "*ppSoldier = pSoldier;" in helper,
        ),
        (
            "team initialization assigns metadata through inactive storage",
            "GetInactiveSoldierSlot( &pTeamMember, cnt2 )" in init,
        ),
        (
            "reinforcement capacity counts only usable inactive storage",
            "GetInactiveSoldierSlot( &pSoldier, i )" in capacity
            and "GetSoldier( &pSoldier, i )" not in capacity,
        ),
        (
            "soldier creation selects with the same usable-slot predicate",
            "GetInactiveSoldierSlot( &pTeamSoldier, cnt )" in selection
            and "if ( !pTeamSoldier->bActive )" not in selection,
        ),
    ]

    failures = 0
    for label, passed in checks:
        print(f"{'PASS' if passed else 'FAIL'}: {label}")
        failures += not passed
    if failures:
        print(f"FAIL: {failures} reinforcement slot-contract checks failed", file=sys.stderr)
        return 1
    print(f"PASS: {len(checks)} reinforcement slot-contract checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
