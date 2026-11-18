#!/usr/bin/env python3
"""Check that helicopter repair scheduling uses the repair charge."""
# P710 native snapshot update: tests/native-core/check_enemy_heli_repair_cost.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def function_body(source: str, name: str) -> str:
    match = re.search(rf"^void\s+{name}\s*\([^)]*\)\s*\{{(.*?)^\}}", source, re.M | re.S)
    return re.sub(r"\s+", "", match[1]) if match else ""


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_enemy_heli_repair_cost.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Strategic" / "ASD.cpp").read_text(encoding="latin-1")
    repair = function_body(source, "UpdateEnemyHeliRepair")
    schedule = function_body(source, "UpdateEnemyHeli")
    cost = (
        "(gGameExternalOptions.gEnemyHeliMaxHP-heli.hp)"
        "*gGameExternalOptions.gEnemyHeliCostPerRepair1HP"
    )
    if f"INT32cost={cost};AddStrategicAIResources(ASD_MONEY,-cost);" not in repair:
        print("FAIL: repair does not charge for all missing HP", file=sys.stderr)
        return 1

    funded_repair = (
        f"if({cost}<=gASDResource[ASD_MONEY]){{"
        "heli.flagmask|=ENEMYHELI_REPAIRING;"
        "AddStrategicEvent(EVENT_ENEMY_HELI_REPAIR,"
    )
    if funded_repair not in schedule:
        print("FAIL: repair scheduling must compare the full charge with available money", file=sys.stderr)
        return 1

    print("PASS: enemy-helicopter repair scheduling checks the full charge")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
