#!/usr/bin/env python3
"""Check AI action, movement, targeting, and NPC pointer boundaries."""
# P710 native snapshot update: tests/native-core/check_ai_runtime_guards.py
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
        print("usage: check_ai_runtime_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    aimain = (root / "TacticalAI" / "AIMain.cpp").read_text(encoding="latin-1")
    utils = (root / "TacticalAI" / "AIUtils.cpp").read_text(encoding="latin-1")
    attacks = (root / "TacticalAI" / "Attacks.cpp").read_text(encoding="latin-1")
    movement = (root / "TacticalAI" / "Movement.cpp").read_text(encoding="latin-1")
    npc = (root / "TacticalAI" / "NPC.cpp").read_text(encoding="latin-1")

    door = section(
        aimain,
        "case AI_ACTION_OPEN_OR_CLOSE_DOOR:",
        "case AI_ACTION_LOWER_GUN:",
    )
    roaming = section(utils, "INT16 RoamingRange(", "void RearrangePocket(")
    area = section(attacks, "BOOLEAN GetBestAoEGridNo(", "BOOLEAN GetFarthestOpponent(")
    move = section(
        movement,
        "INT32 InternalGoAsFarAsPossibleTowards(",
        "INT32 GoAsFarAsPossibleTowards(",
    )
    converse = section(npc, "void Converse(", "INT32 NPCConsiderInitiatingConv(")
    if not all((door, roaming, area, move, converse)):
        print("FAIL: one or more AI function bodies were not found", file=sys.stderr)
        return 1

    gift = section(
        converse,
        "else if ( pQuotePtr->usGiftItem != 0 )",
        "// Action before movement?",
    )
    actions = section(converse, "// Action before movement?", "// Movement?")

    checks = (
        (
            bool(re.search(
                r"if\s*\(\s*pStructure\s*==\s*NULL\s*\)\s*\{.*?"
                r"EndAIGuysTurn\s*\(\s*pSoldier\s*\)\s*;\s*"
                r"return\s*\(\s*FALSE\s*\)\s*;\s*\}",
                door,
                re.DOTALL,
            )),
            "door action dereferences a missing structure after cancellation",
        ),
        (
            bool(re.search(
                r"INT32\s+OppPosKnown\s*=\s*FALSE\s*;\s*"
                r"\*\s*pusFromGridNo\s*=\s*pSoldier->sGridNo\s*;\s*"
                r"if\s*\(\s*CREATURE_OR_BLOODCAT",
                roaming,
            )),
            "RoamingRange can return for a zombie without publishing an origin",
        ),
        (
            bool(re.search(
                r"else\s+if\s*\(\s*bPersOL\s*==\s*SEEN_LAST_TURN.*?"
                r"if\s*\(\s*SpacesAway.*?\)\s*>=\s*3\s*\)\s*"
                r"\{?\s*continue\s*;.*?sOpponentTile\s*\[\s*ubOpponentCnt\s*\]",
                area,
                re.DOTALL,
            )),
            "area targeting uses an uninitialized last-known opponent tile",
        ),
        (
            bool(re.search(
                r"INT16\s+bAPsLeft\s*=\s*pSoldier->bActionPoints\s*,\s*fPathFlags\s*;",
                move,
            )),
            "movement can read AP state after the global mode changes",
        ),
        (
            bool(re.search(
                r"pSoldier\s*=\s*FindSoldierByProfileID.*?"
                r"if\s*\(\s*pSoldier\s*\).*?FindObj\s*\(\s*pSoldier",
                gift,
                re.DOTALL,
            )),
            "NPC gift handling calls FindObj with a missing soldier",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*bInvPos\s*!=\s*NO_SLOT\s*\).*?"
                r"TalkingMenuGiveItem",
                gift,
                re.DOTALL,
            )),
            "NPC gift handling indexes the inventory after a disabled assertion",
        ),
        (
            len(re.findall(
                r"pSoldier\s*=\s*FindSoldierByProfileID.*?"
                r"if\s*\(\s*pSoldier\s*\)\s*\{.*?"
                r"ZEROTIMECOUNTER\s*\(\s*pSoldier->timeCounters\.AICounter\s*\)",
                actions,
                re.DOTALL,
            )) >= 2,
            "NPC action setup dereferences a missing soldier",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(
            f"FAIL: {len(failures)} of {len(checks)} AI runtime checks failed",
            file=sys.stderr,
        )
        return 1

    print(f"PASS: {len(checks)} AI runtime guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
