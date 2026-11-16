"""Extract the current tactical-placement edge validation and copy block."""
# P710 native snapshot update: tests/native-core/extract_tactical_placement.py
# Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-16.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import sys


source, output = map(Path, sys.argv[1:])
text = source.read_text()


def line_number(offset: int) -> int:
    return text[:offset].count("\n") + 1


def function(signature: str) -> tuple[int, str]:
    assert text.count(signature) == 1
    start = text.index(signature)
    opening = text.index("{", start)
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return start, text[start : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


find_start, find_valid = function(
    "static void FindValidInsertionCode( UINT8 *pubStrategicInsertionCode )"
)
check_start, check_valid = function(
    "static void CheckForValidMapEdge( UINT8 *pubStrategicInsertionCode )"
)

block_marker = (
    "\t\t\tif( pSoldier->ubStrategicInsertionCode == "
    "INSERTION_CODE_PRIMARY_EDGEINDEX ||"
)
assert text.count(block_marker) == 1
block_start = text.index(block_marker)
block_end_marker = "\n\t\t\t// Flugente: campaign stats"
block_end = text.index(block_end_marker, block_start)
placement_block = text[block_start:block_end].rstrip()

output.write_text(
    f'#line {line_number(find_start)} "{source}"\n{find_valid}\n\n'
    f'#line {line_number(check_start)} "{source}"\n{check_valid}\n\n'
    "static void P710ApplyTacticalPlacementBlock()\n{\n"
    f'#line {line_number(block_start)} "{source}"\n{placement_block}\n'
    "}\n"
)
