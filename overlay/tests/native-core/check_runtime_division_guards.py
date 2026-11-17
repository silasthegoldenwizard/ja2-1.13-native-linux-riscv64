#!/usr/bin/env python3
"""Check production arithmetic boundaries reported by Clang analysis."""
# P710 native snapshot update: tests/native-core/check_runtime_division_guards.py
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
        print("usage: check_runtime_division_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    soldier = (root / "Tactical" / "Soldier Control.cpp").read_text(
        encoding="latin-1"
    )
    mail = (root / "Laptop" / "BobbyRMailOrder.cpp").read_text(
        encoding="latin-1"
    )
    dress = section(
        soldier,
        "UINT32 SOLDIERTYPE::SoldierDressWound(",
        "void SOLDIERTYPE::InternalReceivingSoldierCancelServices(",
    )
    multi = section(
        soldier,
        "BOOLEAN\tSOLDIERTYPE::UpdateMultiTurnAction(",
        "void\t SOLDIERTYPE::DropSectorEquipment(",
    )
    gold = section(mail, "void DrawOrderGoldRectangle", "UINT32\tCalcCostFromWeightOfPackage")
    if not dress or not multi or not gold:
        print("FAIL: one or more arithmetic function bodies were not found", file=sys.stderr)
        return 1

    checks = (
        (
            len(re.findall(r"if\s*\(\s*!uiPossible\s*\)\s*return\s*\(\s*0\s*\)\s*;", dress)) >= 2,
            "bandaging can reduce uiPossible to zero after its first check",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*bOverTurnAPS\s*>\s*0\s*&&\s*entireapcost\s*>\s*0\s*\)\s*"
                r"DeductPoints\s*\(",
                multi,
                re.DOTALL,
            )),
            "partial multi-turn action cost can divide by a zero full cost",
        ),
        (
            bool(re.search(r"if\s*\(\s*guidivisor\s*<=\s*1\s*\)\s*return\s*;", gold)),
            "Bobby Ray order scrollbar can divide by a zero divisor",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} arithmetic checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} runtime division guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
