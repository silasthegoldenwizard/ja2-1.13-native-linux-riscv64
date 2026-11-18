#!/usr/bin/env python3
"""Check production soldier-table sentinel boundaries."""
# P710 native snapshot update: tests/native-core/check_soldier_sentinel_bounds.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_soldier_sentinel_bounds.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    minigame = (root / "Tactical" / "Minigame.cpp").read_text(encoding="latin-1")
    team_turns = (root / "Tactical" / "TeamTurns.cpp").read_text(encoding="latin-1")
    failures: list[str] = []
    checks = 0

    def require(condition: bool, message: str) -> None:
        nonlocal checks
        checks += 1
        if not condition:
            failures.append(message)

    require(
        "bMercID <= TOTAL_SOLDIERS" not in minigame,
        "minigame player scan includes the NOBODY sentinel",
    )
    require(
        not re.search(
            r"for\s*\(\s*pSoldier\s*=\s*MercPtrs\s*\[\s*bMercID\s*\].*?"
            r"\+\+pSoldier\s*\)",
            minigame,
            re.DOTALL,
        ),
        "minigame player scan walks from one MercPtrs pointee by pointer arithmetic",
    )
    require(
        bool(re.search(r"bMercID\s*<\s*TOTAL_SOLDIERS", minigame)),
        "minigame player scan has no strict soldier-table bound",
    )
    require(
        bool(re.search(
            r"GetSoldier\s*\(\s*&pSoldier\s*,\s*SoldierID\s*\(\s*bMercID\s*\)\s*\)",
            minigame,
        )),
        "minigame player scan does not resolve each live soldier independently",
    )
    require(
        "LATEST_INTERRUPT_GUY <= TOTAL_SOLDIERS" not in team_turns,
        "interrupt cleanup accepts the NOBODY sentinel as an array index",
    )
    require(
        bool(re.search(
            r"GetSoldier\s*\(\s*&pLatestInterruptSoldier\s*,\s*"
            r"SoldierID\s*\(\s*LATEST_INTERRUPT_GUY\s*\)\s*\)",
            team_turns,
        )),
        "interrupt cleanup does not resolve the latest soldier through the live-owner guard",
    )
    require(
        "pLatestInterruptSoldier->AdjustNoAPToFinishMove( TRUE )" in team_turns
        and "pLatestInterruptSoldier->flags.bTurningFromPronePosition" in team_turns,
        "interrupt cleanup still dereferences the unchecked numeric soldier ID",
    )

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(
            f"FAIL: {len(failures)} of {checks} soldier sentinel-bound checks failed",
            file=sys.stderr,
        )
        return 1

    print(f"PASS: {checks} soldier sentinel-bound checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
