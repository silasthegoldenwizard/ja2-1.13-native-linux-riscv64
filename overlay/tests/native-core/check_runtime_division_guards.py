#!/usr/bin/env python3
"""Check production arithmetic boundaries reported by Clang analysis."""
# P710 native snapshot update: tests/native-core/check_runtime_division_guards.py
# Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

# P710 native snapshot update: tests/native-core/check_runtime_division_guards.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def section(source: str, start_text: str, end_text: str) -> str:
    start = source.find(start_text)
    end = source.find(end_text, start)
    return source[start:end] if start >= 0 and end >= 0 else ""


def function_body(source: str, marker: str) -> str:
    start = source.rfind(marker)
    if start < 0:
        return ""
    opening = source.find("{", start)
    if opening < 0:
        return ""
    depth = 0
    for pos in range(opening, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1 : pos]
    return ""


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
    weapons = (root / "Tactical" / "Weapons.cpp").read_text(encoding="latin-1")
    points = (root / "Tactical" / "Points.cpp").read_text(encoding="latin-1")
    interface_items = (root / "Tactical" / "Interface Items.cpp").read_text(
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
    damage = function_body(soldier, "UINT8 SOLDIERTYPE::SoldierTakeDamage(")
    base_effect = function_body(weapons, "FLOAT CalcNewChanceToHitBaseEffectBonus(")
    aim_effect = function_body(weapons, "FLOAT CalcNewChanceToHitAimEffectBonus(")
    chance = function_body(weapons, "UINT32 CalcNewChanceToHitGun(")
    split_crate = function_body(interface_items, "void TransformationMenuPopup_SplitCrate(")
    pocket_ammo = function_body(interface_items, "void addAmmoToPocketPopup(")
    deduct_ammo = function_body(points, "void DeductAmmo(")
    if not all((dress, multi, gold, damage, base_effect, aim_effect, chance,
                split_crate, pocket_ammo, deduct_ammo)):
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
        (
            "__max(1,APBPConstants[AP_GET_WOUNDED_DIVISOR])"
            in re.sub(r"\s+", "", damage),
            "wound AP loss can divide by a zero configured divisor",
        ),
        (
            all(
                "gGameExternalOptions.ubMaxSuppressionShock != 0" in body
                and bool(re.search(
                    r"100\.0f\s*\*\s*\(FLOAT\)pSoldier->aiData\.bShock\s*/\s*"
                    r"\(FLOAT\)gGameExternalOptions\.ubMaxSuppressionShock",
                    body,
                ))
                for body in (base_effect, aim_effect)
            ),
            "new chance-to-hit shock effects can divide by a zero maximum",
        ),
        (
            bool(re.search(
                r"100\.0f\s*/\s*__max\s*\(\s*1\s*,\s*APBPConstants\[AP_MAXIMUM\]\s*\)",
                chance,
            )),
            "weapon handling loses fractional AP-maximum scaling",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*usMagazineSize\s*==\s*0\s*\)\s*return\s*;",
                split_crate,
            )),
            "ammo-crate splitting can divide by a zero magazine size",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*magSize\s*==\s*0\s*\)\s*continue\s*;",
                pocket_ammo,
            )),
            "ammo-pocket capacity can divide by a zero magazine size",
        ),
        (
            "__max(1,static_cast<INT32>(Magazine[Item[(*pObjUsed)[0]->data.gun.usGunAmmoItem].ubClassIndex].ubMagSize))"
            in re.sub(r"\s+", "", deduct_ammo),
            "campaign ammunition accounting can divide by a zero magazine size",
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
