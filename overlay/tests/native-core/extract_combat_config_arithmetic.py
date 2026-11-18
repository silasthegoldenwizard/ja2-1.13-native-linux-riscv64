"""Extract the current production combat arithmetic for the native test."""
# P710 native snapshot update: tests/native-core/extract_combat_config_arithmetic.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


import json
from pathlib import Path
import sys


def write_fragment(source, text, offset, end, output):
    line = text.count("\n", 0, offset) + 1
    output.write_text(
        f"#line {line} {json.dumps(str(source))}\n" + text[offset:end] + "\n",
        encoding="utf-8",
    )


def main():
    if len(sys.argv) != 3:
        print("Usage: extract_combat_config_arithmetic.py ENGINE_ROOT OUTPUT_DIR",
              file=sys.stderr)
        return 2

    root, output_dir = map(Path, sys.argv[1:])
    soldier = root / "Tactical" / "Soldier Control.cpp"
    weapons = root / "Tactical" / "Weapons.cpp"
    sources = {
        path: path.read_text(encoding="latin-1") for path in (soldier, weapons)
    }
    fragments = (
        (soldier, "UINT8 SOLDIERTYPE::SoldierTakeDamage(",
         "\t// Deduct breath AND APs!", "\t// ATE: if the robot, do not deduct",
         "wound_ap_cost.inc"),
        (weapons, "FLOAT CalcNewChanceToHitBaseEffectBonus(",
         "\t// SHOCK (SHOOTER)", "\treturn fBaseModifier;",
         "base_shock_modifier.inc"),
        (weapons, "FLOAT CalcNewChanceToHitAimEffectBonus(",
         "\t// SHOCK (SHOOTER)", "\treturn fAimModifier;",
         "aim_shock_modifier.inc"),
        (weapons, "UINT32 CalcNewChanceToHitGun(",
         "\t// Calculate how easy it is to handle the weapon",
         "\t// get bonus from weapon handling", "weapon_handling_difficulty.inc"),
    )
    output_dir.mkdir(parents=True, exist_ok=True)
    for path, signature, first, last, name in fragments:
        text = sources[path]
        start = text.rindex("\n" + signature) + 1
        end = text.index("\n}", start)
        offset = text.index(first, start, end)
        tail = text.index(last, offset, end)
        write_fragment(path, text, offset, tail, output_dir / name)

    text = sources[soldier]
    start = text.index("\nUINT8 SOLDIERTYPE::ShockLevelPercent(void)\n") + 1
    end = text.index("\n}", start) + len("\n}")
    write_fragment(soldier, text, start, end, output_dir / "shock_level_percent.inc")
    return 0


if __name__ == "__main__":
    sys.exit(main())
