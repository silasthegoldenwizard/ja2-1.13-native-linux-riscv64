"""Compile the current production routine with the reviewed synthetic fixture."""
# P710 native snapshot update: tests/native-core/extract_door_swap.py
# Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-15.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

from pathlib import Path
import sys

source, output = map(Path, sys.argv[1:])
text = source.read_text()
signature = "static STRUCTURE * InternalSwapStructureForPartner("
assert text.count(signature) == 1
start = text.index(signature)
end = text.index("\n}\n", start) + 2
output.write_text(f'#line {text[:start].count(chr(10)) + 1} "{source}"\n' + text[start:end] + "\n")
