/*
 * P710 native snapshot update: tests/native-core/native_facility_runtime_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted schedule, mini-event, and facility checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "Campaign Types.h"
#include "Facilities.h"
#include "Overhead.h"

#include <iostream>

namespace {
unsigned checks = 0;
unsigned failures = 0;

void check(bool condition, const char* message)
{
    ++checks;
    if (condition) return;
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}
}

int main()
{
    check(GetSectorModifier(nullptr, FACILITY_PERFORMANCE_MOD) == 0,
          "a missing soldier has no sector modifier");

    SOLDIERTYPE* soldier = &Menptr[0];
    soldier->sSectorX = 1;
    soldier->sSectorY = 1;
    soldier->bAssignment = 0;
    check(GetSectorModifier(soldier, FACILITY_CANTINA_MOD) == 100,
          "cantina percentage defaults to neutral without a facility");
    check(GetSectorModifier(soldier, FACILITY_PRISON_MOD) == 100,
          "prison percentage defaults to neutral without a facility");
    check(GetSectorModifier(soldier, UINT8_MAX) == 0,
          "an unknown sector modifier has a defined result");

    soldier->sSectorX = 0;
    check(GetSectorModifier(soldier, FACILITY_PERFORMANCE_MOD) == 0,
          "an invalid sector is rejected before indexing facility locations");
    soldier->sSectorX = 1;

    check(FacilityRiskResult(nullptr, RISK_STRENGTH, 0, 0) == 0,
          "a missing soldier cannot produce a facility risk result");
    check(FacilityRiskResult(soldier, NUM_RISKS, 0, 0) == 0,
          "an invalid risk type is rejected before indexing risk data");
    check(FacilityRiskResult(soldier, RISK_STRENGTH,
                             static_cast<UINT8>(MAX_NUM_FACILITY_TYPES), 0) == 0,
          "an invalid facility type is rejected before indexing facility data");
    check(FacilityRiskResult(soldier, RISK_STRENGTH, 0,
                             NUM_FACILITY_ASSIGNMENTS) == 0,
          "an invalid assignment type is rejected before indexing assignment data");

    if (failures) {
        std::cerr << "FAIL: " << failures << " of " << checks
                  << " facility runtime assertions\n";
        return 1;
    }
    std::cout << "PASS: " << checks << " facility runtime assertions\n";
    return 0;
}
