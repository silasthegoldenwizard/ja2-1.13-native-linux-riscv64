#!/usr/bin/env python3
"""Check that overhead item invalidation only uses initialized coordinates."""
# P710 native snapshot update: tests/native-core/check_overhead_item_invalidation.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_overhead_item_invalidation.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (
        Path(sys.argv[1]) / "TileEngine" / "overhead map.cpp"
    ).read_text(encoding="latin-1")
    start = source.find("void RenderOverheadOverlays()")
    end = source.find(
        "void RenderOverheadOverlays( INT16 sStartPointX_M", start + 1
    )
    if start < 0 or end < 0:
        print("FAIL: RenderOverheadOverlays body was not found", file=sys.stderr)
        return 1

    body = source[start:end]
    item_start = body.find("//ITEMS OVERLAY")
    if item_start < 0:
        print("FAIL: overhead item overlay was not found", file=sys.stderr)
        return 1
    item_body = re.sub(r"\s+", " ", body[item_start:])

    guarded = re.search(
        r"PixelDraw\( FALSE, sX, sY, usLineColor, pDestBuf \); "
        r"InvalidateRegion\( sX, sY, \(INT16\)\( sX \+ 1 \), "
        r"\(INT16\)\( sY \+ 1 \) \); \} "
        r"// Flugente: exploration",
        item_body,
    )
    invalidations = item_body.count(
        "InvalidateRegion( sX, sY, (INT16)( sX + 1 ), (INT16)( sY + 1 ) );"
    )
    if not guarded or invalidations != 1:
        print(
            "FAIL: overhead item invalidation can use coordinates that were not initialized",
            file=sys.stderr,
        )
        return 1

    print("PASS: overhead item invalidation is scoped to a successful pixel draw")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
