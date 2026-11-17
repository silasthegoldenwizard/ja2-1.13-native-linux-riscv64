#!/usr/bin/env python3
"""Check battle-entry soldier creation and lookup boundaries."""
# P710 native snapshot update: tests/native-core/check_battle_entry_soldier_guards.py
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


def guarded_creation_count(source: str) -> int:
    return len(re.findall(
        r"pSoldier\s*=\s*TacticalCreate[^;]+;\s*"
        r"Assert\s*\(\s*pSoldier\s*\)\s*;\s*"
        r"if\s*\(\s*!\s*pSoldier\s*\)\s*break\s*;",
        source,
    ))


def loop_resets_soldier(source: str) -> bool:
    return bool(re.search(
        r"while\s*\([^)]*\)\s*\{\s*pSoldier\s*=\s*(?:NULL|nullptr)\s*;",
        source,
    ))


def invalid_direction_returns(source: str) -> bool:
    return bool(re.search(
        r"default\s*:\s*AssertMsg\s*\([^;]+;\s*"
        r"(?:ResetCreatureAttackVariables\s*\(\s*\)\s*;\s*)?"
        r"return\s*;",
        source,
    ))


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_battle_entry_soldier_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    queen = (root / "Strategic" / "Queen Command.cpp").read_text(encoding="latin-1")
    creatures_source = (root / "Strategic" / "Creature Spreading.cpp").read_text(
        encoding="latin-1"
    )
    init_source = (root / "Tactical" / "Soldier Init List.cpp").read_text(
        encoding="latin-1"
    )

    enemies = section(queen, "void AddEnemiesToBattle(", "void AddMilitiaToBattle(")
    militia = section(queen, "void AddMilitiaToBattle(", "BOOLEAN SaveUnderGroundSectorInfoToSaveGame(")
    creatures = section(
        creatures_source,
        "static void AddCreaturesToBattle_Other(",
        "static void ChooseTownSectorToAttack(",
    )
    preparation = section(
        creatures_source,
        "BOOLEAN PrepareCreaturesForBattle()",
        "void CreatureNightPlanning()",
    )
    edge_militia = section(
        init_source,
        "void AddSoldierInitListMilitiaOnEdge(",
        "extern UINT32 GetWorldTotalMin",
    )
    prisoners = section(
        init_source,
        "void SectorAddPrisonersofWar(",
        "void SectorAddDownedPilot(",
    )
    list_cleanup = section(
        init_source,
        "void KillSoldierInitList()",
        "//These serialization functions",
    )
    list_sort = section(
        init_source,
        "void SortSoldierInitList()",
        "extern FLOAT gAmbushRadiusModifier",
    )
    if not all((
        enemies,
        militia,
        creatures,
        preparation,
        edge_militia,
        prisoners,
        list_cleanup,
        list_sort,
    )):
        print("FAIL: one or more battle-entry functions were not found", file=sys.stderr)
        return 1

    side_effect_assert = re.compile(
        r"Assert\s*\([^;\n]*(?<![!<>=])=(?!=)",
    )
    checks = (
        (
            not side_effect_assert.search(enemies + militia),
            "optimized builds remove soldier creation hidden inside Assert",
        ),
        (
            guarded_creation_count(enemies) == 6,
            "enemy creation does not guard all six allocation results",
        ),
        (
            loop_resets_soldier(enemies)
            and bool(re.search(
                r"if\s*\(\s*!\s*pSoldier\s*\)\s*break\s*;\s*"
                r"// HEADROCK HAM 3\.2",
                enemies,
            )),
            "enemy selection can reuse an old or undefined soldier pointer",
        ),
        (
            guarded_creation_count(militia) == 3,
            "militia creation does not guard all three allocation results",
        ),
        (
            loop_resets_soldier(militia)
            and bool(re.search(
                r"if\s*\(\s*!\s*pSoldier\s*\)\s*break\s*;\s*"
                r"// HEADROCK HAM 3\.2",
                militia,
            )),
            "militia selection can reuse an old or undefined soldier pointer",
        ),
        (
            invalid_direction_returns(enemies)
            and invalid_direction_returns(militia)
            and invalid_direction_returns(edge_militia)
            and invalid_direction_returns(creatures),
            "an invalid insertion direction continues after its disabled release assertion",
        ),
        (
            loop_resets_soldier(creatures)
            and bool(re.search(
                r"switch\s*\(\s*guCreatureAttackType\s*\).*?"
                r"if\s*\(\s*!\s*pSoldier\s*\)\s*break\s*;.*?"
                r"pSoldier->ubInsertionDirection",
                creatures,
                re.DOTALL,
            )),
            "creature creation can dereference a missing or undefined soldier",
        ),
        (
            "BOOLEAN fQueen = FALSE;" in preparation
            and bool(re.search(
                r"if\s*\(\s*ubCreatureHabitat\s*>=\s*"
                r"NUMBER_OF_CREATURE_COMPOSITIONS\s*\)\s*\{.*?"
                r"return\s+FALSE\s*;\s*\}.*?"
                r"gCreatureComposition\s*\[\s*ubCreatureHabitat\s*\]",
                preparation,
                re.DOTALL,
            )),
            "creature preparation can use an undefined queen flag or invalid composition index",
        ),
        (
            loop_resets_soldier(edge_militia)
            and bool(re.search(
                r"if\s*\(\s*!\s*pSoldier\s*\)\s*break\s*;\s*"
                r"// HEADROCK HAM 3\.2",
                edge_militia,
            )),
            "edge-militia selection can reuse an undefined soldier pointer",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*pTeamSoldier\s*&&\s*"
                r"\(\s*pTeamSoldier->usSoldierFlagMask\s*&\s*SOLDIER_POW_PRISON\s*\)\s*\)",
                prisoners,
            )),
            "prisoner counting dereferences an unresolved civilian slot",
        ),
        (
            "RemoveSoldierNodeFromInitList( gSoldierInitHead );" in list_cleanup
            and bool(re.search(
                r"if\s*\(\s*gSoldierInitHead\s*\)\s*"
                r"gSoldierInitHead->prev\s*=\s*NULL\s*;\s*"
                r"else\s*gSoldierInitTail\s*=\s*NULL\s*;",
                list_cleanup,
            )),
            "soldier-init teardown can retain or reuse a released tail",
        ),
        (
            bool(re.search(
                r"newHead\s*=\s*temp->next\s*;\s*"
                r"if\s*\(\s*!\s*newHead\s*\)\s*\{.*?"
                r"return\s*;\s*\}.*?gSoldierInitHead\s*=\s*newHead\s*;",
                list_sort,
                re.DOTALL,
            )),
            "soldier-init sorting can dereference a missing replacement head",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(
            f"FAIL: {len(failures)} of {len(checks)} battle-entry checks failed",
            file=sys.stderr,
        )
        return 1

    print(f"PASS: {len(checks)} battle-entry soldier guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
