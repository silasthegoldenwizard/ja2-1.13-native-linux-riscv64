#!/usr/bin/env python3
"""Check transactional ownership while constructing the global tile database."""
# P710 native snapshot update: tests/native-core/check_tile_database_lifecycle.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
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
        print("usage: check_tile_database_lifecycle.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    tiledef_source = (root / "TileEngine" / "tiledef.cpp").read_text(
        encoding="latin-1"
    )
    tiledat_source = (root / "TileEngine" / "TileDat.cpp").read_text(
        encoding="latin-1"
    )
    header = (root / "TileEngine" / "tiledef.h").read_text(encoding="latin-1")
    world_source = (root / "TileEngine" / "worlddef.cpp").read_text(
        encoding="latin-1"
    )
    editor_source = (root / "Editor" / "editscreen.cpp").read_text(
        encoding="latin-1"
    )

    create = compact(
        extract(
            tiledef_source,
            "CreateTileDatabase( INT32 iTilesetID )",
            "void DeallocateTileDatabase( )",
        )
    )
    deallocate = compact(
        extract(
            tiledef_source,
            "void DeallocateTileDatabase( )",
            "BOOLEAN GetLandHeadType",
        )
    )
    allocate = compact(
        extract(
            tiledef_source,
            "BOOLEAN AllocateAnimTileData( TILE_ELEMENT *pTileElem, UINT8 ubNumFrames )",
            "void FreeAnimTileData( TILE_ELEMENT *pTileElem )",
        )
    )
    free_data = compact(
        extract(
            tiledef_source,
            "void FreeAnimTileData( TILE_ELEMENT *pTileElem )",
            None,
        )
    )
    specific = compact(
        extract(
            tiledat_source,
            "SetSpecificDatabaseValues(",
            "// Madd: sets the giNumberOfTiles variable",
        )
    )
    load_tileset = compact(
        extract(
            world_source,
            "BOOLEAN LoadMapTileset( INT32 iTilesetID )",
            "BOOLEAN SaveMapTileset( INT32 iTilesetID )",
        )
    )
    destroy_surfaces = compact(
        extract(
            world_source,
            "void DestroyTileSurfaces( )",
            "static void CompileWorldTerrainIDs",
        )
    )
    load_world = compact(
        extract(
            world_source,
            "BOOLEAN LoadWorld(const STR8 puiFilename",
            "BOOLEAN LoadWorldInfoForAutoResolve",
        )
    )
    create_new_map = compact(
        extract(
            editor_source,
            "void CreateNewMap()",
            "UINT32 ProcessEditscreenMessageBoxResponse()",
        )
    )
    if not all(
        (
            create,
            deallocate,
            allocate,
            free_data,
            specific,
            load_tileset,
            destroy_surfaces,
            load_world,
            create_new_map,
        )
    ):
        print("FAIL: tile-database lifecycle functions were not found", file=sys.stderr)
        return 1

    frame_failure = allocate[
        allocate.find("pNewAnimData->pusFrames =") : allocate.find(
            "pNewAnimData->ubNumFrames ="
        )
    ]
    checks = [
        (
            "configured tile database size is validated before construction",
            matches(
                create,
                r"expectedDatabaseSize = GetTileDatabaseExpectedSize\(\);.*?if \( giNumberOfTiles <= 0 \|\| giNumberOfTiles > NUMBEROFTILES \|\| expectedDatabaseSize == 0 \).*?return\( FALSE \);",
            ),
        ),
        (
            "tile-database builders publish failure-capable signatures",
            "BOOLEAN CreateTileDatabase( INT32 iTilesetID );" in header
            and "BOOLEAN SetSpecificDatabaseValues(" in header,
        ),
        (
            "animation allocation validates the destination and frame count",
            matches(
                allocate,
                r"if \( pTileElem == NULL \|\| ubNumFrames == 0 \).*?return\( FALSE \);",
            ),
        ),
        (
            "animation record allocation is checked before initialization",
            ordered(
                allocate,
                "pNewAnimData = (TILE_ANIMATION_DATA *)MemAlloc",
                "if ( pNewAnimData == NULL )",
                "return( FALSE );",
                "memset( pNewAnimData, 0, sizeof( TILE_ANIMATION_DATA ) );",
            ),
        ),
        (
            "frame allocation failure releases the unpublished record",
            ordered(
                frame_failure,
                "pNewAnimData->pusFrames = (UINT16 *)MemAlloc",
                "if ( pNewAnimData->pusFrames == NULL )",
                "MemFree( pNewAnimData );",
                "return( FALSE );",
            ),
        ),
        (
            "animation ownership publishes only after complete initialization",
            ordered(
                allocate,
                "pNewAnimData->ubNumFrames = ubNumFrames;",
                "if ( pTileElem->pAnimData != NULL )",
                "FreeAnimTileData( pTileElem );",
                "pTileElem->pAnimData = pNewAnimData;",
                "return( TRUE );",
            ),
        ),
        (
            "animation cleanup accepts a null element and clears published ownership",
            matches(
                free_data,
                r"if \( pTileElem == NULL \).*?return;.*?if \( pTileElem->pAnimData != NULL \).*?pTileElem->pAnimData->pusFrames = NULL;.*?MemFree\( pTileElem->pAnimData \);.*?pTileElem->pAnimData = NULL;",
            ),
        ),
        (
            "auxiliary animation allocation failure rolls back construction",
            matches(
                create,
                r"if \( !AllocateAnimTileData\( &TileElement, TileSurf->pAuxData\[cnt2\]\.ubNumberOfFrames \) \).*?return AbortTileDatabaseCreation\( &TileElement \);",
            ),
        ),
        (
            "animated-tile registry capacity is checked before publication",
            matches(
                create,
                r"if \( gusNumAnimatedTiles >= MAX_ANIMATED_TILES \).*?return AbortTileDatabaseCreation\( &TileElement \);.*?gusAnimatedTiles\[ gusNumAnimatedTiles \]",
            ),
        ),
        (
            "specific tile setup failure rolls back pending and published owners",
            matches(
                create,
                r"if \( !SetSpecificDatabaseValues\(.*?\) \).*?return AbortTileDatabaseCreation\( &TileElement \);",
            ),
        ),
        (
            "both tile database append sites reject capacity overflow",
            create.count("if ( gTileDatabaseSize >= expectedDatabaseSize || gTileDatabaseSize >= NUMBEROFTILES )")
            == 2
            and create.count("return AbortTileDatabaseCreation( &TileElement );") >= 5,
        ),
        (
            "tile database construction verifies the final published size",
            matches(
                create,
                r"if \( gTileDatabaseSize != expectedDatabaseSize \).*?DeallocateTileDatabase\( \);.*?return\( FALSE \);.*?return\( TRUE \);",
            ),
        ),
        (
            "all six specific animation allocations propagate failure",
            specific.count("if ( !AllocateAnimTileData( TileElement, 5 ) )") == 6
            and specific.count("return( FALSE );") >= 7
            and specific.strip().endswith("return( TRUE ); }"),
        ),
        (
            "database deallocation scans the complete backing array",
            "for( cnt = 0; cnt < NUMBEROFTILES; cnt++ )" in deallocate,
        ),
        (
            "tileset cache publication is transactional across failure and retry",
            matches(load_tileset, r"iTilesetID == giCurrentTilesetID && gTileDatabaseSize != 0")
            and ordered(
                load_tileset,
                "giCurrentTilesetID = -1;",
                "LoadTileSurfaces(",
                "DeallocateTileDatabase( );",
                "DestroyTileSurfaces( );",
                "return( FALSE );",
                "DeallocateTileDatabase( );",
                "if ( !CreateTileDatabase( iTilesetID ) )",
                "DestroyTileSurfaces( );",
                "return( FALSE );",
                "giCurrentTilesetID = iTilesetID;",
            ),
        ),
        (
            "surface-load failure retires the database before returning",
            matches(
                load_tileset,
                r"if \( !LoadTileSurfaces\(.*?\) \).*?DeallocateTileDatabase\( \);.*?DestroyTileSurfaces\( \);.*?return\( FALSE \);",
            ),
        ),
        (
            "surface destruction clears all reuse flags before a retry",
            ordered(
                destroy_surfaces,
                "DeleteTileSurface( gTileSurfaceArray[ uiLoop ] );",
                "gTileSurfaceArray[ uiLoop ] = NULL;",
                "memset( gbDefaultSurfaceUsed, 0, sizeof( gbDefaultSurfaceUsed ) );",
                "memset( gbSameAsDefaultSurfaceUsed, 0, sizeof( gbSameAsDefaultSurfaceUsed ) );",
                "memset( gbNewTileSurfaceLoaded, 0, sizeof( gbNewTileSurfaceLoaded ) );",
            ),
        ),
        (
            "map loading releases transient buffers when tileset loading fails",
            matches(
                load_world,
                r"if \( !LoadMapTileset\( iTilesetID \) \).*?MemFree\(pBufferHead\);.*?MemFree\(bCounts\);.*?SET_ERROR\(\"Could not load map: tile database construction failed\"\);.*?return FALSE;",
            ),
        ),
        (
            "editor map creation stops when the default tileset cannot load",
            matches(
                create_new_map,
                r"if \( !gfWorldLoaded && !LoadMapTileset\( 0 \) \).*?return;.*?NewWorld",
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
        print(
            f"FAIL: {failures} tile-database lifecycle checks failed",
            file=sys.stderr,
        )
        return 1

    print(f"PASS: {len(checks)} tile-database lifecycle checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
