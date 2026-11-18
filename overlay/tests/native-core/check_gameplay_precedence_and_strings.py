#!/usr/bin/env python3
"""Check small production expressions whose precedence or literals change behavior."""
# P710 native snapshot update: tests/native-core/check_gameplay_precedence_and_strings.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def compact(source: str) -> str:
    return re.sub(r"\s+", "", source)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_gameplay_precedence_and_strings.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    pmc = compact((root / "Laptop" / "PMC.cpp").read_text(encoding="latin-1"))
    weapons = compact((root / "Tactical" / "Weapons.cpp").read_text(encoding="latin-1"))
    soldier = compact(
        (root / "Tactical" / "Soldier Control.cpp").read_text(encoding="latin-1")
    )
    interface = compact(
        (root / "Tactical" / "Interface.cpp").read_text(encoding="latin-1")
    )
    interface_items = compact(
        (root / "Tactical" / "Interface Items.cpp").read_text(encoding="latin-1")
    )
    items = compact((root / "Tactical" / "Items.cpp").read_text(encoding="latin-1"))

    spotter_guard = (
        "if(gGameOptions.fNewTraitSystem&&"
        "(Weapon[usInHand].ubWeaponType==GUN_SN_RIFLE||"
        "Weapon[usInHand].ubWeaponType==GUN_RIFLE))"
    )
    queen_guard = (
        "if(usInHand==CREATURE_QUEEN_TENTACLES&&"
        "(pDefender->ubBodyType==LARVAE_MONSTER||"
        "pDefender->ubBodyType==INFANT_MONSTER))"
    )

    checks = (
        (
            "gPMCSectorNamesstr[count][63]=L'\\0';" in pmc
            and "gPMCSectorNamesstr[count][63]='/0';" not in pmc,
            "PMC sector names must end with a wide NUL character",
        ),
        (
            weapons.count(spotter_guard) == 2,
            "both old and new CTH paths must gate rifle spotter bonuses on new traits",
        ),
        (
            queen_guard in weapons,
            "queen-child protection must not affect infant monsters for every weapon",
        ),
        (
            "FLOATmodifier=0.5f+sLifeDeduct/100.0f;" in soldier,
            "open-wound infection scaling must preserve fractional damage",
        ),
        (
            "sprintf(zFilename,\"%s\\\\case%d.ogg\",zCommonName,Random(gCommonCase)+1);"
            in weapons
            and not re.search(r"(?<!s)printf\(zFilename,", weapons),
            "weapon case-sound selection must write the filename instead of printing it",
        ),
        (
            "&&(this->ubProfile!=NO_PROFILE)&&"
            "(gMercProfiles[this->ubProfile].Type==PROFILETYPE_RPC||"
            "gMercProfiles[this->ubProfile].Type==PROFILETYPE_NPC)" in soldier,
            "NPC/RPC wound bookkeeping must validate the attacker and profile before indexing",
        ),
        (
            "!InteractiveActionPossibleAtGridNo(" not in soldier
            and "InteractiveActionPossibleAtGridNo(this->sMTActionGridNo,"
            "this->pathing.bLevel,structindex)!=INTERACTIVE_STRUCTURE_HACKABLE"
            in soldier,
            "multi-turn hacking must compare the returned interaction type directly",
        ),
        (
            "if(gDisplayEnemyRoles&&"
            "(pSoldier->bTeam==ENEMY_TEAM||pSoldier->bTeam==CIV_TEAM))"
            in interface,
            "the enemy-role display option must gate both enemy and civilian symbols",
        ),
        (
            interface_items.count(
                "if(UsingNewInventorySystem()==true&&"
                "(guiCurrentScreen==GAME_SCREEN||guiCurrentScreen==SHOPKEEPER_SCREEN))"
            ) == 2
            and "UsingNewInventorySystem()==true&&guiCurrentScreen==GAME_SCREEN||"
            not in interface_items,
            "new-inventory description width must not leak into old-inventory shopkeeper UI",
        ),
        (
            "if(!pSoldier||pocket>=NUM_INV_SLOTS)" in interface_items,
            "ammunition popup callbacks must reject invalid inventory slots before indexing",
        ),
        (
            "if(Item[pAttachment->usItem].ubClassIndex>=LoadBearingEquipment.size())"
            in interface_items
            and "switch(LoadBearingEquipment[Item[pAttachment->usItem].ubClassIndex].lbeClass)"
            in interface_items,
            "LBE detachment must inspect the attachment class rather than the host item class",
        ),
        (
            all(
                f"if(pTeamSoldier->inv[usAmmoSlot{slot}].exists()&&"
                f"(Item[pTeamSoldier->inv[usAmmoSlot{slot}].usItem].usItemClass!=IC_AMMO||"
                f"pTeamSoldier->inv[usAmmoSlot{slot}][0]->data.ubShotsLeft>0))"
                in items
                for slot in (1, 2)
            ),
            "external feeding must not inspect an absent ammunition object",
        ),
        (
            items.count(
                "if(iter->exists()&&(!ItemIsDuckbill(iter->usItem)||"
                "(*pObj)[0]->data.gun.ubGunAmmoType==AMMO_BUCKSHOT))"
            ) == 2,
            "range bonuses must not inspect empty attachment entries",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} gameplay checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} gameplay precedence and string checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
