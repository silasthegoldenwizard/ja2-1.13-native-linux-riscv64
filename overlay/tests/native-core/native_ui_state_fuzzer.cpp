/*
 * P710 native snapshot update: tests/native-core/native_ui_state_fuzzer.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Stateful sanitizer fuzzing for production UI callbacks.
#include "GameSettings.h"
#include "Map Screen Interface.h"
#include "Map Screen Interface Map.h"
#include "Map Screen Interface Map Inventory.h"
#include "Overhead.h"
#include "Soldier Control.h"
#include "mousesystem.h"
#include <cstddef>
#include <cstdint>

void MapInvenPoolSlots(MOUSE_REGION*,INT32);

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,std::size_t size) {
    if (size<3) return 0;

    constexpr INT32 slotCount=40;
    MAP_INVENTORY_POOL_SLOT_COUNT=slotCount;
    const INT32 slot=data[0]%slotCount;
    iCurrentInventoryPoolPage=data[1]%5;
    pInventoryPoolList.clear();
    pInventoryPoolList.resize(data[2]%(slotCount*2+1));

    bSelectedInfoChar=0;
    gCharactersList[0].fValid=TRUE;
    gCharactersList[0].usSolID=SoldierID(0);
    auto& selected=Menptr[0];
    selected.initialize();
    selected.ubID=SoldierID(0);
    selected.bActive=TRUE;
    selected.stats.bLife=80;
    selected.sSectorX=sSelMapX=1;
    selected.sSectorY=sSelMapY=1;
    selected.bSectorZ=iCurrentMapSectorZ=0;
    selected.flags.fBetweenSectors=FALSE;
    MercPtrs[0]=&selected;

    const BOOLEAN oldSectorDescription=gGameExternalOptions.fSectorDesc;
    gGameExternalOptions.fSectorDesc=TRUE;
    MOUSE_REGION region{};
    region.UserData[0]=slot;
    MapInvenPoolSlots(&region,MSYS_CALLBACK_REASON_RBUTTON_UP);
    gGameExternalOptions.fSectorDesc=oldSectorDescription;
    return 0;
}
