#!/usr/bin/env python3
"""Check LOS bullet entry points stop invalid owner and target states."""
# P710 native snapshot update: tests/native-core/check_los_bullet_guards.py
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
        print("usage: check_los_bullet_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    source = (root / "Tactical" / "LOS.cpp").read_text(encoding="latin-1")
    bullets_header = (root / "Tactical" / "Bullets.h").read_text(encoding="latin-1")
    bullets_source = (root / "Tactical" / "bullets.cpp").read_text(encoding="latin-1")
    weapons_source = (root / "Tactical" / "Weapons.cpp").read_text(encoding="latin-1")
    client_source = (root / "Multiplayer" / "client.cpp").read_text(encoding="latin-1")
    shield = section(source, "BOOLEAN DamageRiotShield_Bullet(", "BOOLEAN BulletHitMerc(")
    hit = section(source, "BOOLEAN BulletHitMerc(", "void BulletHitStructure(")
    structure = section(source, "INT32 HandleBulletStructureInteraction(", "INT32 CTGTHandleBulletStructureInteraction(")
    ctgt = section(source, "INT32 CTGTHandleBulletStructureInteraction(", "static BOOLEAN PositionAllowsHit(")
    fire = section(source, "INT8 FireBullet(", "INT8 FireBulletGivenTargetNCTH(")
    sway = section(source, "void CalcMuzzleSway(", "FLOAT CalcBulletDeviation(")
    impact = section(weapons_source, "INT32 BulletImpact(", "INT32 HTHImpact(")
    receive = section(client_source, "void recieveBULLET(", "void recieveSTATE(")
    if not shield or not hit or not structure or not ctgt or not fire or not sway or not impact or not receive:
        print("FAIL: one or more LOS bullet functions were not found", file=sys.stderr)
        return 1

    checks = (
        (
            bool(re.search(
                r"if\s*\(\s*pBullet\s*==\s*NULL\s*\)\s*\{?\s*"
                r"return\s*\(\s*TRUE\s*\)\s*;\s*\}?\s*"
                r"if\s*\(\s*pStructure\s*==\s*NULL\s*\)\s*\{\s*"
                r"RemoveBullet\s*\(\s*pBullet->iBullet\s*\)\s*;\s*return\s*\(\s*TRUE\s*\)\s*;",
                hit,
                re.DOTALL,
            )),
            "BulletHitMerc accepts a missing bullet or structure",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*fHasFirer\s*&&\s*pFirer\s*==\s*NULL\s*\)\s*"
                r"\{\s*RemoveBullet\s*\(\s*pBullet->iBullet\s*\)\s*;\s*"
                r"return\s*\(\s*TRUE\s*\)\s*;",
                hit,
                re.DOTALL,
            )),
            "BulletHitMerc dereferences an unresolved non-anonymous firer",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*pStructure->usStructureID\s*>=\s*MAX_NUM_SOLDIERS\s*\)\s*"
                r"\{\s*RemoveBullet.*?return\s*\(\s*TRUE\s*\)\s*;.*?"
                r"pTarget\s*=\s*MercPtrs\[\s*pStructure->usStructureID\s*\]\s*;\s*"
                r"if\s*\(\s*pTarget\s*==\s*NULL\s*\)\s*\{\s*RemoveBullet.*?"
                r"return\s*\(\s*TRUE\s*\)\s*;",
                hit,
                re.DOTALL,
            )),
            "BulletHitMerc accepts an invalid or empty mercenary structure slot",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*pBullet\s*==\s*NULL\s*\)\s*\{?\s*"
                r"return\s*\(\s*FALSE\s*\)\s*;",
                fire,
            ))
            and bool(re.search(
                r"if\s*\(\s*fHasFirer\s*&&\s*pBullet->pFirer\s*==\s*NULL\s*\)\s*"
                r"\{\s*if\s*\(\s*!fFake\s*\)\s*\{?\s*"
                r"RemoveBullet\s*\(\s*pBullet->iBullet\s*\)\s*;\s*\}?\s*"
                r"return\s*\(\s*FALSE\s*\)\s*;",
                fire,
                re.DOTALL,
            )),
            "FireBullet continues after a missing bullet or unresolved firer",
        ),
        (
            bool(re.search(r"FLOAT\s+dStartZ\s*=\s*0\.0f\s*;", sway)),
            "ground-shot limiting consumes dStartZ after a failed height lookup",
        ),
        (
            bool(re.search(r"UINT16\s+usAmmoType\s*;", bullets_header))
            and bool(re.search(
                r"CreateBullet\s*\([^)]*UINT16\s+fromItem\s*,\s*UINT16\s+usAmmoType\s*\)",
                bullets_header,
            ))
            and bool(re.search(r"pBullet->usAmmoType\s*=\s*usAmmoType\s*;", bullets_source)),
            "bullets do not retain their ammunition identity",
        ),
        (
            "ubAmmoType = pBullet->usAmmoType;" in shield
            and "ubAmmoType = pBullet->usAmmoType;" in hit
            and "ubAmmoType = pBullet->usAmmoType;" in structure
            and "pBullet->usAmmoType" in ctgt
            and ctgt.count("AmmoTypes[ubAmmoType]") >= 3,
            "anonymous bullets still depend on a live firer for collision ammunition",
        ),
        (
            bool(re.search(
                r"else\s+if\s*\(\s*!fFragment\s*\).*?"
                r"if\s*\(\s*pBullet\s*\).*?ubAmmoType\s*=\s*pBullet->usAmmoType\s*;",
                impact,
                re.DOTALL,
            )),
            "anonymous bullet damage does not use the retained ammunition type",
        ),
        (
            "const BOOLEAN fHasFirer = ubFirer != NOBODY;" in fire
            and bool(re.search(
                r"else\s+if\s*\(\s*!fHasFirer\s*\)\s*\{.*?"
                r"Weapon\[\s*pBullet->fromItem\s*\]\.ubBulletSpeed",
                fire,
                re.DOTALL,
            )),
            "anonymous bullets still enter firer-only launch-speed logic",
        ),
        (
            source.count("CreateBullet(") == 5
            and len(re.findall(r"CreateBullet\s*\([^;]+?,\s*ammotype\s*\)\s*;", source, re.DOTALL)) == 2
            and bool(re.search(
                r"CreateBullet\s*\(\s*ubOwner\s*,\s*false\s*,\s*usBulletFlags\s*,\s*"
                r"usExplosiveItem\s*,\s*Explosive\[Item\[usExplosiveItem\]\.ubClassIndex\]\.ubFragType\s*\)",
                source,
            )),
            "one or more bullet producers omit the ammunition type",
        ),
        (
            "netb->net_bullet.usAmmoType" in receive
            and "FireBullet( netb->net_bullet.ubFirerID, pBullet, FALSE );" in receive
            and "FireBullet( pFirer->ubID, pBullet, FALSE );" not in receive,
            "network bullets discard ammunition identity or dereference an anonymous firer",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} LOS bullet checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} LOS bullet guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
