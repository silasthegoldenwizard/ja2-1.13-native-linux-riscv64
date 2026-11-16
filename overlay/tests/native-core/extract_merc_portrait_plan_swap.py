"""Extract the production portrait-record swap with its AI-plan lifetime rule."""
# P710 native snapshot update: tests/native-core/extract_merc_portrait_plan_swap.py
# Native Linux source/build/test adaptations captured in the accepted virtual inventory and lifetime checkpoint dated 2026-11-16.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-16.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

from pathlib import Path
import sys

source, output = map(Path, sys.argv[1:])
text = source.read_text()
signature = "static void SwapMercPortraitRecords( SOLDIERTYPE& source, SOLDIERTYPE& target )"
assert text.count(signature) == 1, "expected one production record-swap helper"
start = text.index(signature)
opening = text.index("{", start)
depth = 0
for end in range(opening, len(text)):
    if text[end] == "{":
        depth += 1
    elif text[end] == "}":
        depth -= 1
        if not depth:
            break
else:
    raise AssertionError("unterminated production record-swap helper")
call = "SwapMercPortraitRecords( Menptr[ ubSourceMerc ], Menptr[ ubTargetMerc ] );"
assert text.count(call) == 1, "actual portrait swap must use the tested helper"
assert "SOLDIERTYPE TempMenptr = Menptr[ ubSourceMerc ]" not in text, "early live-plan snapshot must stay removed"
line = text[:start].count("\n") + 1
output.write_text(f'#line {line} "{source}"\n{text[start:end + 1]}\n')
