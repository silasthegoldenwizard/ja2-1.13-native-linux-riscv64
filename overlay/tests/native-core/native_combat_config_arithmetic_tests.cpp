/*
 * P710 native snapshot update: tests/native-core/native_combat_config_arithmetic_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "core_types.h"
#include "native_crt.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string_view>

namespace {

// Minimal state used by the extracted arithmetic. Runtime types and min/max
// conversions come from the native engine headers.
enum { AP_GET_WOUNDED_DIVISOR, AP_MAXIMUM };
INT16 APBPConstants[2]{};

struct
{
    UINT8 ubMaxSuppressionShock;
} gGameExternalOptions{};

struct
{
    FLOAT BASE_SHOCK = -20.0f;
    FLOAT AIM_SHOCK = -50.0f;
} gGameCTHConstants;

struct SOLDIERTYPE
{
    struct { INT8 bShock; } aiData;
    UINT8 ShockLevelPercent(void);
};

using std::min; // The original Windows source used the unprefixed macro.
#include "shock_level_percent.inc"

INT16 woundedCost(INT16 sLifeDeduct, INT16 divisor, INT16 maximumAp)
{
    APBPConstants[AP_GET_WOUNDED_DIVISOR] = divisor;
    APBPConstants[AP_MAXIMUM] = maximumAp;
    INT16 sAPCost = 0;
#include "wound_ap_cost.inc"
    return sAPCost;
}

struct Modifiers { FLOAT base; FLOAT aim; };

Modifiers shockModifiers(INT8 shock, UINT8 maximumShock)
{
    gGameExternalOptions.ubMaxSuppressionShock = maximumShock;
    SOLDIERTYPE soldier{{shock}};
    SOLDIERTYPE* pSoldier = &soldier;
    FLOAT fBaseModifier = 10.0f;
    FLOAT fAimModifier = 15.0f;
#include "base_shock_modifier.inc"
#include "aim_shock_modifier.inc"
    return {fBaseModifier, fAimModifier};
}

struct OBJECTTYPE { INT16 readyTimeReduction; };

INT16 GetPercentReadyTimeAPReduction(OBJECTTYPE* obj)
{
    return obj->readyTimeReduction;
}

Modifiers handlingDifficulty(INT16 maximumAp)
{
    APBPConstants[AP_MAXIMUM] = maximumAp;
    OBJECTTYPE weapon{25};
    OBJECTTYPE* pInHand = &weapon;
    const UINT8 ubModifiedHandling = 24;
#include "weapon_handling_difficulty.inc"
    return {fGunBaseDifficulty, fGunAimDifficulty};
}

int failures = 0;
int checks = 0;

void check(bool condition, const char* message)
{
    ++checks;
    if (!condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool near(FLOAT actual, FLOAT expected)
{
    return std::isfinite(actual) && std::fabs(actual - expected) < 0.0001f;
}

void checkWoundCosts()
{
    check(woundedCost(19, 4, 100) == 4, "normal wounded AP cost");
    check(woundedCost(19, 4, 25) == 1, "integer wound cost at 25 AP");
    check(woundedCost(19, 4, 150) == 6, "integer wound cost at 150 AP");
    check(woundedCost(16, 0, 100) == 16, "zero wound divisor uses the minimum of one");
    check(woundedCost(16, -4, 25) == 4, "negative wound divisor uses the minimum of one");
    check(woundedCost(0, 0, 100) == 0, "no wound loses zero AP with a zero divisor");
}

void checkShockModifiers(INT8 shock, UINT8 maximumShock, Modifiers expected, const char* message)
{
    const Modifiers actual = shockModifiers(shock, maximumShock);
    check(near(actual.base, expected.base) && near(actual.aim, expected.aim), message);
}

void checkHandlingDifficulty(INT16 maximumAp, FLOAT expected, const char* message)
{
    const Modifiers actual = handlingDifficulty(maximumAp);
    check(near(actual.base, expected) && near(actual.aim, expected), message);
}

}

int main(int argc, char** argv)
{
    const std::string_view scenario = argc == 2 ? argv[1] : "";
    if (scenario == "wound")
    {
        checkWoundCosts();
    }
    else if (scenario == "shock")
    {
        checkShockModifiers(0, 30, {10.0f, 15.0f}, "a shooter without shock keeps both modifiers");
        checkShockModifiers(15, 0, {10.0f, 15.0f}, "disabled suppression shock keeps both modifiers finite");
        checkShockModifiers(1, 30, {9.333333f, 13.333333f}, "fractional shock preserves the original floating ratio");
        checkShockModifiers(15, 30, {0.0f, -10.0f}, "half shock applies half the base and aim penalties");
        checkShockModifiers(30, 30, {-10.0f, -35.0f}, "maximum shock applies both full penalties");
        checkShockModifiers(40, 30, {-16.666667f, -51.666667f}, "above-maximum shock preserves the original uncapped ratio");
    }
    else if (scenario == "handling")
    {
        checkHandlingDifficulty(25, 72.0f, "25 AP uses the existing fourfold handling difficulty");
        checkHandlingDifficulty(100, 18.0f, "100 AP preserves the ready-time-adjusted difficulty");
        checkHandlingDifficulty(60, 30.0f, "60 AP uses a fractional multiplier greater than one");
        checkHandlingDifficulty(125, 14.4f, "125 AP preserves a fractional handling penalty");
        checkHandlingDifficulty(250, 7.2f, "250 AP preserves a fractional handling penalty");
        checkHandlingDifficulty(0, 1800.0f, "zero maximum AP uses a nonzero fallback divisor");
    }
    else
    {
        std::cerr << "Usage: native_combat_config_arithmetic_tests wound|shock|handling\n";
        return 2;
    }

    std::cout << checks - failures << '/' << checks << " " << scenario << " checks passed\n";
    return failures != 0;
}
