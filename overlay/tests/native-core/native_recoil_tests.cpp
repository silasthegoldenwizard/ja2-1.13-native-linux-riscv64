/*
 * P710 native snapshot update: tests/native-core/native_recoil_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_recoil_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Actual NCTH counter-force calculation, with authored finite state only.
#include "Soldier Control.h"
#include "GameSettings.h"
#include "LOS.h"
#include "random.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

extern FLOAT CalcCounterForceChange(SOLDIERTYPE*, UINT32, FLOAT, FLOAT,
                                    FLOAT, FLOAT, UINT32);
extern bool is_client;
extern bool is_networked;

namespace {
unsigned checks = 0;
void expect(bool value, const char* why) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << why << '\n'; std::abort(); }
}
void check(SOLDIERTYPE& shooter, FLOAT offset, FLOAT recoil, FLOAT force,
           UINT32 bullets, FLOAT expected, bool positiveError = false) {
    guiPreRandomIndex = 0;
    guiPreRandomNums[1] = positiveError ? (RAND_MAX / 2 + 1) : 0;
    const auto result = CalcCounterForceChange(&shooter, 100, 10.0f,
                                                offset, recoil, force, bullets);
    expect(std::isfinite(result), "finite recoil change for finite authored state");
    expect(std::fabs(result - expected) < 0.00001f, "expected original recoil and mandatory error");
    expect(guiPreRandomIndex == 2, "both original mandatory-error random draws retained");
}
}

int main() {
    auto shooter = std::make_unique<SOLDIERTYPE>();
    shooter->bDoBurst = 1;
    shooter->ubAttackingHand = HANDPOS;
    gGameSettings.fOptions[TOPTION_REPORT_MISS_MARGIN] = FALSE;
    gGameCTHConstants.RECOIL_COUNTER_INCREMENT = 1;
    gGameCTHConstants.RECOIL_COUNTER_ACCURACY_MIN_ERROR = 0.1f;
    // Reuse the real deterministic multiplayer random path. Zero samples make
    // the added error -0.1 without replacing the production random function.
    is_networked = true; is_client = true;
    for (auto& value : guiPreRandomNums) value = 0;

    // Nonzero offset, recoil and force; the original coasting branch selects
    // zero change. Before repair this reproduces the actual line10791 NaN cast.
    check(*shooter, 2.0f, 1.0f, -2.0f, 1, -0.1f);
    // At the center, both stationary and moving muzzle states are valid.
    check(*shooter, 0.0f, 1.0f, -1.0f, 3, -0.1f);
    check(*shooter, -0.0f, -1.0f, 1.0f, 3, -0.1f);
    check(*shooter, 0.0f, 1.0f, 0.0f, 3, -1.1f);
    check(*shooter, 0.0f, -1.0f, 0.0f, 3, 0.9f);
    // Ideal CF can be zero even when the muzzle offset and recoil are nonzero.
    check(*shooter, 1.0f, -1.0f, 1.0f, 3, -1.1f);
    check(*shooter, -1.0f, 1.0f, -1.0f, 3, 0.9f);
    // Preserve ordinary positive and negative adjustment, including the cap.
    check(*shooter, 2.0f, 1.0f, -1.0f, 3, -1.1f);
    check(*shooter, -2.0f, -1.0f, 1.0f, 3, 0.9f);
    check(*shooter, 2.0f, 1.0f, 0.0f, 3, -1.1f);
    check(*shooter, -2.0f, -1.0f, 0.0f, 3, 0.9f);
    check(*shooter, 0.0f, 0.0f, 0.0f, 3, -0.1f);
    // Mandatory error still reaches the original directional clamps.
    check(*shooter, -0.05f, -1.0f, 1.0f, 3, 0.0f);
    check(*shooter, 0.05f, 1.0f, -1.0f, 3, -0.15f);
    check(*shooter, 0.0f, 1.0f, -1.0f, 3, 0.0f, true);
    check(*shooter, 0.05f, 1.0f, -1.0f, 3, 0.0f, true);
    check(*shooter, -0.05f, -1.0f, 1.0f, 3, 0.15f, true);
    // Also cover the source review's coasting case at the default increment.
    gGameCTHConstants.RECOIL_COUNTER_INCREMENT = 20;
    gGameCTHConstants.RECOIL_COUNTER_ACCURACY_MIN_ERROR = 0.2f;
    check(*shooter, 30.0f, 1.0f, -2.0f, 2, -4.0f);

    // NCTH area fire can have no soldier at the target grid and no computed
    // sight range.  Keep the normal visibility penalties and all conversions
    // finite instead of dividing by zero and casting infinity to UINT32.
    expect(AdjustCounterForceAccuracyForSight(80, 100, 100, 1) == 80,
           "ordinary counter-force sight scaling is preserved");
    expect(AdjustCounterForceAccuracyForSight(80, 100, 100, 2) == 40,
           "team-only sight keeps the original half-accuracy penalty");
    expect(AdjustCounterForceAccuracyForSight(80, 100, 100, 4) == 20,
           "unseen target keeps the original quarter-accuracy penalty");
    expect(AdjustCounterForceAccuracyForSight(80, 100, 0, 4) == 20,
           "targetless area fire uses physical range before visibility penalty");
    expect(AdjustCounterForceAccuracyForSight(80, 0, 0, 4) == 0,
           "zero-range targetless fire remains finite");
    expect(AdjustCounterForceAccuracyForSight(UINT32_MAX, UINT32_MAX, 1, 1) == 100,
           "large authored values clamp before unsigned conversion");
    is_networked = false; is_client = false;
    std::cout << checks << " original NCTH counter-force assertions passed\n";
}
