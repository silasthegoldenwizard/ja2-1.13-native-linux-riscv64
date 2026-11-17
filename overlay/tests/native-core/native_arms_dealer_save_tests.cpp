/*
 * P710 native snapshot update: tests/native-core/native_arms_dealer_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Legacy arms-dealer loader lifecycle with synthetic bounded data only.
#include "Arms Dealer Init.h"
#include "BufferMan.h"
#include "GameVersion.h"
#include "SaveLoadGame.h"
#include <cstdlib>
#include <iostream>
#include <vector>

extern BOOLEAN NativeTestLoadArmsDealerInventoryFromSavedGameFile(CHAR8** ppBuffer);

int main()
{
    const UINT32 priorVersion = guiCurrentSaveGameVersion;
    guiCurrentSaveGameVersion = 55;
    std::vector<CHAR8> bytes(NUM_ORIGINAL_ARMS_DEALERS * sizeof(OLD_ARMS_DEALER_STATUS_101), 0);
    gpLoadBufferStart = bytes.data();
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    CHAR8* cursor = bytes.data();
    const BOOLEAN loaded = NativeTestLoadArmsDealerInventoryFromSavedGameFile(&cursor);
    const bool passed = !loaded && gfBufferLoadFailed && cursor == bytes.data() + bytes.size();
    ShutDownArmsDealers();
    gpLoadBufferStart = NULL;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    guiCurrentSaveGameVersion = priorVersion;
    if (!passed)
    {
        std::cerr << "FAIL: truncated legacy dealer inventory must reject at the exact boundary\n";
        return 1;
    }
    std::cout << "PASS: truncated legacy dealer inventory rejected transactionally\n";
    return 0;
}
