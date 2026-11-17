/*
 * P710 native snapshot update: tests/native-core/native_enemy_temp_load_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Production-linked early-failure checks for preserved enemy-sector loading.
#include "Campaign Types.h"
#include "Enemy Soldier Save.h"
#include "strategicmap.h"
#include <iostream>
#include <stdexcept>

namespace {
unsigned checks = 0;

void check(bool condition, const char* why) {
    ++checks;
    if (!condition) throw std::runtime_error(why);
}

struct RestoredGlobals {
    UNDERGROUND_SECTORINFO* underground = gpUndergroundSectorInfoHead;
    INT16 sectorX = gWorldSectorX;
    INT16 sectorY = gWorldSectorY;
    INT8 sectorZ = gbWorldSectorZ;
    BOOLEAN restoring = gfRestoringEnemySoldiersFromTempFile;

    ~RestoredGlobals() {
        gpUndergroundSectorInfoHead = underground;
        gWorldSectorX = sectorX;
        gWorldSectorY = sectorY;
        gbWorldSectorZ = sectorZ;
        gfRestoringEnemySoldiersFromTempFile = restoring;
    }
};
}

int main() {
    try {
        RestoredGlobals restore;
        gpUndergroundSectorInfoHead = nullptr;
        gWorldSectorX = 1;
        gWorldSectorY = 1;
        gbWorldSectorZ = 1;
        gfRestoringEnemySoldiersFromTempFile = FALSE;

        check(!NewWayOfLoadingEnemySoldiersFromTempFile(),
              "missing underground metadata rejects the preserved-enemy load");
        check(!gfRestoringEnemySoldiersFromTempFile,
              "early load failure retires the restoring-enemy state");
        check(gpUndergroundSectorInfoHead == nullptr,
              "failed load leaves the underground list owner unchanged");

        std::cout << "PASS: " << checks
                  << " production enemy-temp early-failure assertions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " assertions: " << error.what() << '\n';
        return 1;
    }
}
