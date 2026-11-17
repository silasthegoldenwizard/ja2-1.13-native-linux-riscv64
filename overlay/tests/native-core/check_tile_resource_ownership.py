#!/usr/bin/env python3
"""Check rollback ownership for tile animation and surface construction."""
# P710 native snapshot update: tests/native-core/check_tile_resource_ownership.py
# Native Linux source/build/test adaptations captured in the accepted tile-resource ownership checkpoint dated 2026-11-17.
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
        print("usage: check_tile_resource_ownership.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    animation_source = (root / "TileEngine" / "Tile Animation.cpp").read_text(
        encoding="latin-1"
    )
    surface_source = (root / "TileEngine" / "Tile Surface.cpp").read_text(
        encoding="latin-1"
    )
    animation = compact(
        extract(
            animation_source,
            "ANITILE *CreateAnimationTile( ANITILE_PARAMS *pAniParams )",
            "void DeleteAniTiles( )",
        )
    )
    surface = compact(
        extract(
            surface_source,
            "TILE_IMAGERY *LoadTileSurface(",
            "void DeleteTileSurface(",
        )
    )
    if not animation or not surface:
        print("FAIL: tile construction functions were not found", file=sys.stderr)
        return 1

    add_calls = (
        "AddShadowToHead",
        "AddObjectToHead",
        "AddRoofToHead",
        "AddOnRoofToHead",
        "AddTopmostToHead",
    )
    node_cleanup = animation[
        animation.find("if ( pNode == NULL )") : animation.find(
            "// SET NEW TILE VALUES"
        )
    ]
    add_results_checked = all(
        matches(
            animation,
            rf"if \( {name}\( sGridNo, usTileIndex \) == FALSE \) .*? pNode = NULL",
        )
        for name in add_calls
    ) and ordered(
        node_cleanup,
        "if ( pNode == NULL )",
        "RemoveCachedTile( iCachedTile );",
        "MemFree( pNewAniNode );",
        "return( NULL );",
    )

    animation_allocation = animation[
        animation.find("pNewAniNode =") : animation.find(
            "if ( (uiFlags & ANITILE_EXISTINGTILE"
        )
    ]
    unsupported_level = animation[
        animation.find("default:") : animation.find("if ( pNode == NULL )")
    ]
    surface_allocation = surface[
        surface.find("pTileSurf = (PTILE_IMAGERY)") : surface.find(
            "// Set all values to zero"
        )
    ]
    aux_failure = surface[
        surface.find("if ( pTileSurf->pAuxData == NULL") : surface.find(
            "memcpy( pTileSurf->pAuxData"
        )
    ]

    checks = [
        (
            "animation parameters are validated before dereference",
            matches(
                animation,
                r"if \( pAniParams == NULL \) .*? return\( NULL \); .*? sGridNo = pAniParams->sGridNo",
            ),
        ),
        (
            "animation-node allocation is validated before initialization",
            ordered(
                animation_allocation,
                "pNewAniNode = (ANITILE *) MemAlloc",
                "if ( pNewAniNode == NULL )",
                "return( NULL );",
                "memset( pNewAniNode",
            ),
        ),
        (
            "cached-tile load failure releases the unpublished animation node",
            matches(
                animation,
                r"if \( iCachedTile == -1 \) .*? MemFree\( pNewAniNode \); .*? return\( NULL \);",
            ),
        ),
        (
            "existing-tile construction rejects a missing level node",
            matches(
                animation,
                r"if \( pGivenNode == NULL \) .*? MemFree\( pNewAniNode \); .*? return\( NULL \);",
            ),
        ),
        (
            "unsupported animation levels release cache and node ownership",
            ordered(
                unsupported_level,
                "default:",
                "RemoveCachedTile( iCachedTile );",
                "MemFree( pNewAniNode );",
                "return( NULL );",
            ),
        ),
        (
            "level-node insertion failures are checked before head selection",
            add_results_checked,
        ),
        (
            "tile-surface allocation failure releases all prior owners",
            ordered(
                surface_allocation,
                "pTileSurf = (PTILE_IMAGERY) MemAlloc",
                "if ( pTileSurf == NULL )",
                "FreeStructureFile( pStructureFileRef );",
                "DestroyImage( hImage );",
                "DeleteVideoObject( hVObject );",
                "return( NULL );",
            ),
        ),
        (
            "auxiliary-data allocation failure releases the tile-surface record",
            ordered(
                aux_failure,
                "if ( pTileSurf->pAuxData == NULL",
                "MemFree( pTileSurf );",
                "DestroyImage( hImage );",
                "DeleteVideoObject( hVObject );",
                "return( NULL );",
            ),
        ),
        (
            "structure-count failure releases the loaded structure file",
            matches(
                surface,
                r"pStructureFileRef == NULL \|\| hVObject->usNumberOfObjects != pStructureFileRef->usNumberOfStructures\) .*? if \( pStructureFileRef != NULL \) .*? FreeStructureFile\( pStructureFileRef \); .*? return\( NULL \);",
            ),
        ),
        (
            "Z-strip failure releases the loaded structure file",
            matches(
                surface,
                r"if \(fOk == FALSE\) .*? FreeStructureFile\( pStructureFileRef \); .*? return\( NULL \);",
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
        print(f"FAIL: {failures} tile-resource ownership checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} tile-resource ownership checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
