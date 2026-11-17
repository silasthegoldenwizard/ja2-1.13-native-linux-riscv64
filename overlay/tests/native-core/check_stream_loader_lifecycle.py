#!/usr/bin/env python3
"""Check transactional stdio handling in animation and shade-table loaders."""
# P710 native snapshot update: tests/native-core/check_stream_loader_lifecycle.py
# Native Linux source/build/test adaptations captured in the accepted stream-loader and word-wrap ownership checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def extract(source: str, start_marker: str, end_marker: str | None) -> str:
    start = source.find(start_marker)
    if start < 0:
        return ""
    if end_marker is None:
        return source[start:]
    end = source.find(end_marker, start + len(start_marker))
    return source[start:end] if end >= 0 else ""


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
        print("usage: check_stream_loader_lifecycle.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    animation_source = (root / "Ja2" / "aniviewscreen.cpp").read_text(
        encoding="latin-1"
    )
    lighting_source = (root / "TileEngine" / "lighting.cpp").read_text(
        encoding="latin-1"
    )
    animation_handle = compact(
        extract(
            animation_source,
            "UINT32  AniEditScreenHandle(void)",
            "static UINT16 GetAnimStateFromName",
        )
    )
    animation_loader = compact(
        extract(animation_source, "void BuildListFile( )", None)
    )
    animation_reset = compact(
        extract(
            animation_source,
            "static void ClearAnimationStateList( )",
            "static void CycleAnimations",
        )
    )
    shade_loader = compact(
        extract(
            lighting_source,
            "void LoadShadeTablesFromTextFile()",
            "// Debug variable",
        )
    )
    if not animation_handle or not animation_loader or not shade_loader:
        print("FAIL: stream loader functions were not found", file=sys.stderr)
        return 1

    second_pass_failure = animation_loader[
        animation_loader.find("if ( ferror( infoFile )") :
        animation_loader.find("pusStates = pNewStates")
    ]
    shade_failure = shade_loader[
        shade_loader.find("if ( fscanf") : shade_loader.find(
            "newShadeLevels[ i ][ j ]"
        )
    ]

    checks = [
        (
            "animation list cleanup has one complete owner reset",
            ordered(
                animation_reset,
                "if ( pusStates != NULL )",
                "MemFree( pusStates );",
                "pusStates = NULL;",
                "ubNumStates = 0;",
                "ubCurLoadedState = 0;",
                "fOKFiles = FALSE;",
            ),
        ),
        (
            "animation loader resets stale global state before opening",
            ordered(
                animation_loader,
                "ClearAnimationStateList( );",
                'fopen( "ANITEST.DAT", "rb")',
            ),
        ),
        (
            "animation lines are counted from successful reads",
            "while( !feof" not in animation_loader
            and matches(
                animation_loader,
                r"while\s*\(\s*fgets\( currFilename, sizeof\( currFilename \), infoFile \) != NULL\s*\)",
            ),
        ),
        (
            "animation line endings are trimmed without length underflow",
            'strcspn( currFilename, "\\r\\n" )' in animation_loader
            and "strlen( currFilename ) -1" not in animation_loader,
        ),
        (
            "empty animation entries are ignored",
            matches(
                animation_loader,
                r"if\s*\( currFilename\[ 0 \] == '\\0' \)\s*\{?\s*continue;",
            ),
        ),
        (
            "animation entry count is bounded to the published type",
            matches(
                animation_loader,
                r"if\s*\( numEntries >= UINT8_MAX \).*?fclose\( infoFile \);.*?return;",
            ),
        ),
        (
            "animation rewind failure closes the stream",
            matches(
                animation_loader,
                r"if\s*\( fseek\( infoFile, 0, SEEK_SET \) != 0 \).*?fclose\( infoFile \);.*?return;",
            ),
        ),
        (
            "animation-state allocation is validated before use",
            matches(
                animation_loader,
                r"pNewStates = \(UINT16 \*\) MemAlloc.*?if\s*\( pNewStates == NULL \).*?fclose\( infoFile \);.*?return;",
            ),
        ),
        (
            "unknown animation entries release local state and stream",
            matches(
                animation_loader,
                r"usState == 5555.*?MemFree\( pNewStates \);.*?fclose\( infoFile \);.*?return;",
            ),
        ),
        (
            "short or failed animation reads roll back local state",
            ordered(
                second_pass_failure,
                "if ( ferror( infoFile ) || cnt != numEntries )",
                "MemFree( pNewStates );",
                "fclose( infoFile );",
                "return;",
            ),
        ),
        (
            "animation state publishes only after the stream closes",
            ordered(
                animation_loader,
                "fclose( infoFile );",
                "pusStates = pNewStates;",
                "ubNumStates = (UINT8)cnt;",
                "fOKFiles = TRUE;",
            ),
        ),
        (
            "loaded-order toggle requires a nonempty published list",
            matches(
                animation_handle,
                r"InputEvent\.usParam == 'l'.*?if\s*\( fOKFiles && pusStates != NULL && ubNumStates > 0 \)",
            ),
        ),
        (
            "loaded-order rendering requires a nonempty published list",
            matches(
                animation_handle,
                r"if\s*\( fToggle2 && fOKFiles && pusStates != NULL && ubNumStates > 0 \)",
            ),
        ),
        (
            "page-up wraps at the exclusive animation count",
            matches(
                animation_handle,
                r"ubCurLoadedState\+\+;.*?if\s*\( ubCurLoadedState >= ubNumStates \).*?ubCurLoadedState = 0;",
            ),
        ),
        (
            "page-down wraps to the last valid animation index",
            matches(
                animation_handle,
                r"if\s*\( ubCurLoadedState == 0 \).*?ubCurLoadedState = \(UINT8\)\( ubNumStates - 1 \);.*?else.*?ubCurLoadedState--;",
            ),
        ),
        (
            "animation shutdown and escape use complete cleanup",
            animation_source.count("ClearAnimationStateList( );") >= 3,
        ),
        (
            "shade-table reads validate conversion and range",
            matches(
                shade_failure,
                r"if\s*\( fscanf\( fp, \"%d\", &num \) != 1 \|\| num < 0 \|\| num > UINT16_MAX \)",
            ),
        ),
        (
            "shade-table parse failure closes without publishing",
            matches(
                shade_loader,
                r"if\s*\( fscanf\( fp, \"%d\", &num \) != 1 \|\| num < 0 \|\| num > UINT16_MAX \).*?fclose\( fp \);.*?return;.*?newShadeLevels\[ i \]\[ j \]",
            ),
        ),
        (
            "shade-table values publish atomically after stream close",
            ordered(
                shade_loader,
                "UINT16 newShadeLevels[16][3];",
                "newShadeLevels[ i ][ j ] = (UINT16)num;",
                "fclose( fp );",
                "memcpy( gusShadeLevels, newShadeLevels, sizeof( gusShadeLevels ) );",
            ),
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
        print(f"FAIL: {failures} stream-loader lifecycle checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} stream-loader lifecycle checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
