#!/usr/bin/env python3
"""Check transactional allocation ownership in the original word wrappers."""
# P710 native snapshot update: tests/native-core/check_wordwrap_ownership.py
# Native Linux source/build/test adaptations captured in the accepted stream-loader and word-wrap ownership checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def extract(source: str, start_marker: str, end_marker: str) -> str:
    start = source.find(start_marker)
    end = source.find(end_marker, start + len(start_marker))
    return source[start:end] if start >= 0 and end >= 0 else ""


def compact(source: str) -> str:
    return re.sub(r"\s+", " ", source)


def matches(source: str, pattern: str) -> bool:
    return re.search(pattern, source, re.DOTALL) is not None


def ordered(source: str, *snippets: str) -> bool:
    position = 0
    for snippet in snippets:
        position = source.find(snippet, position)
        if position < 0:
            return False
        position += len(snippet)
    return True


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_wordwrap_ownership.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    source = (root / "Utils" / "WordWrap.cpp").read_text(encoding="utf-8-sig")
    helper = compact(
        extract(
            source,
            "static BOOLEAN AppendWrappedString(",
            "void UseSingleCharWordsForWordWrap",
        )
    )
    single = compact(
        extract(
            source,
            "WRAPPED_STRING *LineWrapForSingleCharWords(",
            "WRAPPED_STRING *LineWrap(",
        )
    )
    normal = compact(
        extract(
            source,
            "WRAPPED_STRING *LineWrap(",
            "UINT16 DisplayWrappedString(",
        )
    )
    if not single or not normal:
        print("FAIL: word-wrap functions were not found", file=sys.stderr)
        return 1

    checks = [
        (
            "append helper validates the node allocation before dereference",
            matches(
                helper,
                r"pNewString = \(WRAPPED_STRING \*\) MemAlloc.*?if\s*\( pNewString == NULL \).*?return\( FALSE \);",
            ),
        ),
        (
            "append helper releases the node when string allocation fails",
            matches(
                helper,
                r"pNewString->sString = \(STR16\) MemAlloc.*?if\s*\( pNewString->sString == NULL \).*?MemFree\( pNewString \);.*?return\( FALSE \);",
            ),
        ),
        (
            "append helper publishes only a fully initialized node",
            ordered(
                helper,
                "wcscpy( pNewString->sString, pString );",
                "pNewString->pNextWrappedString = NULL;",
                "pWrappedString->pNextWrappedString = pNewString;",
                "return( TRUE );",
            ),
        ),
        (
            "single-character wrapping validates caller outputs before use",
            matches(
                single,
                r"if\s*\( pusLineWidthIfWordIsWiderThenWidth == NULL \|\| pString == NULL \).*?return\( NULL \);.*?\*pusLineWidthIfWordIsWiderThenWidth",
            ),
        ),
        (
            "normal wrapping validates caller outputs before use",
            matches(
                normal,
                r"if\s*\( pusLineWidthIfWordIsWiderThenWidth == NULL \|\| pString == NULL \).*?return\( NULL \);.*?\*pusLineWidthIfWordIsWiderThenWidth",
            ),
        ),
        (
            "single-character wrapping uses two checked append operations",
            single.count("if ( !AppendWrappedString(") == 2,
        ),
        (
            "normal wrapping uses four checked append operations",
            normal.count("if ( !AppendWrappedString(") == 4,
        ),
        (
            "single-character append failures release the complete partial list",
            single.count("DeleteWrappedString( FirstWrappedString.pNextWrappedString );")
            == 2,
        ),
        (
            "normal append failures release the complete partial list",
            normal.count("DeleteWrappedString( FirstWrappedString.pNextWrappedString );")
            == 4,
        ),
        (
            "word-wrap functions no longer allocate nodes outside the helper",
            "MemAlloc( sizeof(WRAPPED_STRING)" not in single
            and "MemAlloc(sizeof(WRAPPED_STRING)" not in single
            and "MemAlloc( sizeof(WRAPPED_STRING)" not in normal
            and "MemAlloc(sizeof(WRAPPED_STRING)" not in normal,
        ),
    ]

    failures = 0
    for label, passed in checks:
        if passed:
            print(f"PASS: {label}")
        else:
            failures += 1
            print(f"FAIL: {label}", file=sys.stderr)

    if failures:
        print(f"FAIL: {failures} word-wrap ownership checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} word-wrap ownership checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
