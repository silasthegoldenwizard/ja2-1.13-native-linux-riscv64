#!/usr/bin/env python3
"""Check selected production combat pointer boundaries."""
# P710 native snapshot update: tests/native-core/check_combat_pointer_guards.py
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
        print("usage: check_combat_pointer_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Tactical" / "Weapons.cpp").read_text(
        encoding="latin-1"
    )
    match = re.search(
        r"void\s+StructureHit\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    )
    if not match:
        print("FAIL: StructureHit body was not found", file=sys.stderr)
        return 1
    body = match.group("body")

    checks = (
        (
            bool(re.search(
                r"pBullet\s*=\s*GetBulletPtr\s*\([^)]*\)\s*;.*?"
                r"if\s*\(\s*pBullet\s*==\s*NULL\s*\)\s*\{?\s*return\s*;",
                body,
                re.DOTALL,
            )),
            "missing bullet lookup can reach a bullet dereference",
        ),
        (
            bool(re.search(
                r"pAttacker\s*=\s*ubAttackerID\s*;\s*"
                r"if\s*\(\s*pAttacker\s*!=\s*NULL\s*\).*?GetUsedWeapon",
                body,
                re.DOTALL,
            )),
            "missing attacker pointer can reach GetUsedWeapon",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*fStopped\s*&&\s*pAttacker\s*!=\s*NULL\s*\)"
                r"\s*\{\s*if\s*\(\s*pAttacker->ubOppNum\s*!=\s*NOBODY\s*\)",
                body,
                re.DOTALL,
            )),
            "missing attacker pointer can reach stopped-shot opponent handling",
        ),
        (
            bool(re.search(
                r"else\s*\{.*?ubHitVolume\s*=\s*Weapon\[usWeaponIndex\]\.ubHitVolume\s*;"
                r".*?if\s*\(\s*pObj\s*&&\s*pObj->exists\s*\(\s*\)\s*\)"
                r".*?ubAmmoType\s*=\s*\(\*pObj\)\[0\]->data\.gun\.ubGunAmmoType\s*;",
                body,
                re.DOTALL,
            )),
            "attackerless non-fragment bullet reads a missing weapon object",
        ),
        (
            "else if ( pObj && AmmoTypes[(*pObj)[0]->data.gun.ubGunAmmoType].explosionSize > 1)"
            in body,
            "attackerless rocket path reads a missing weapon object",
        ),
        (
            bool(re.search(
                r"else\s+if\s*\(\s*pAttacker\s*!=\s*NULL\s*&&\s*"
                r"!\s*ItemIsSingleShotRocketLauncher\s*\(usWeaponIndex\)\s*&&\s*"
                r"ItemIsRocketLauncher\s*\(usWeaponIndex\)\s*\)",
                body,
            )),
            "attackerless multi-shot rocket path reads a missing soldier inventory",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} combat pointer checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} StructureHit pointer guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
