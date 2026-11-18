#!/usr/bin/env python3
"""Check transactional ownership while constructing strategic paths."""
# P710 native snapshot update: tests/native-core/check_strategic_path_construction.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def extract(source: str, start: str, end: str) -> str:
    begin = source.find(start)
    finish = source.find(end, begin + len(start)) if begin >= 0 else -1
    return source[begin:finish] if begin >= 0 and finish >= 0 else ""


def compact(source: str) -> str:
    return re.sub(r"\s+", " ", source)


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
        print("usage: check_strategic_path_construction.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (
        Path(sys.argv[1]) / "Strategic" / "Strategic Pathing.cpp"
    ).read_text(encoding="latin-1")
    build = compact(
        extract(source, "PathStPtr BuildAStrategicPath(", "BOOLEAN AddSectorToPathList(")
    )
    add = compact(
        extract(source, "BOOLEAN AddSectorToPathList(", "BOOLEAN TravelBetweenSectorsIsBlockedFromVehicle(")
    )
    if not build or not add:
        print("FAIL: strategic path construction functions were not found", file=sys.stderr)
        return 1

    null_branch = extract(add, "if (pNode==NULL)", "else")
    append_loop = extract(build, "while(iPathLength > iCount)", "pNode=pHeadOfPathList;")
    checks = [
        (
            "initial path allocation is checked before initialization",
            ordered(
                build,
                "pNode = (PathStPtr) MemAlloc(sizeof(PathSt));",
                "if ( pNode == NULL )",
                "return NULL;",
                "pNode->fSpeed=NORMAL_MVT;",
            ),
        ),
        (
            "a null path is rejected instead of allocating unreachable storage",
            "return FALSE;" in null_branch and "MemAlloc" not in null_branch,
        ),
        (
            "appended path allocation is checked before publication",
            ordered(
                add,
                "pTempNode = (PathStPtr) MemAlloc(sizeof(PathSt));",
                "if ( pTempNode == NULL )",
                "return FALSE;",
                "pNode->pNext=pTempNode;",
            ),
        ),
        (
            "failed path append clears the complete unpublished construction",
            re.search(
                r"if\(!AddSectorToPathList\(pHeadOfPathList, .*?\)\) "
                r"\{ ClearStrategicPathList\( pHeadOfPathList, -1 \); return NULL; \}",
                build,
            )
            is not None,
        ),
        (
            "the owned path head remains stable across every append",
            "pHeadOfPathList = pNode;" not in append_loop
            and "pNode = pHeadOfPathList;" in append_loop,
        ),
    ]

    failures = 0
    for label, passed in checks:
        print(f"{'PASS' if passed else 'FAIL'}: {label}")
        failures += not passed
    if failures:
        print(f"FAIL: {failures} of {len(checks)} strategic path construction checks failed")
        return 1
    print(f"PASS: {len(checks)} strategic path construction checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
