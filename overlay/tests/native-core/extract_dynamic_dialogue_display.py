"""Extract the current production display and cleanup routines."""
# P710 native snapshot update: tests/native-core/extract_dynamic_dialogue_display.py
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
parts = []
for signature in ("DDBox::Destroy( )", "DDBox::Display( )"):
    assert text.count(signature) == 1
    start = text.index("void\n" + signature)
    end = text.index("\n}\n", start) + 2
    parts.append(f'#line {text[:start].count(chr(10)) + 1} "{source}"\n' + text[start:end])
output.write_text("\n\n".join(parts) + "\n")
