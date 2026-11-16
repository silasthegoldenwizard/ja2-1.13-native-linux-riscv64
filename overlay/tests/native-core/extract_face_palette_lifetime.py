"""Extract the production face-palette lifecycle routines."""
# P710 native snapshot update: tests/native-core/extract_face_palette_lifetime.py
# Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-16.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

from pathlib import Path
import sys


def extract(source: Path, declaration: str) -> str:
    text = source.read_text()
    marker = "\n" + declaration + "("
    assert text.count(marker) == 1, (source, declaration, text.count(marker))
    start = text.index(marker) + 1
    name_at = start + len(declaration)
    brace = text.index("{", name_at)
    depth = 0
    end = brace
    for end in range(brace, len(text)):
        if text[end] == "{":
            depth += 1
        elif text[end] == "}":
            depth -= 1
            if depth == 0:
                end += 1
                break
    else:
        raise AssertionError((source, declaration, "unterminated function"))
    line = text[:start].count("\n") + 1
    return f'#line {line} "{source}"\n{text[start:end]}\n'


faces, interface, output = map(Path, sys.argv[1:])
parts = [
    extract(faces, "void SetPalettes"),
    extract(interface, "BOOLEAN InitializeFaceGearGraphics"),
    extract(faces, "void GetXYForRightIconPlacement_FaceGear"),
]
output.write_text("\n".join(parts))
