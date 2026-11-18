/*
 * P710 native snapshot update: tests/native-core/native_tactical_team_last_slot_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "Campaign Types.h"
#include "GameSettings.h"
#include "Isometric Utils.h"
#include "Items.h"
#include "Overhead.h"
#include "Soldier Init List.h"
#include "Weapons.h"
#include "soldier profile type.h"
#include "strategicmap.h"
#include "connect.h"

#include <iostream>

namespace {
unsigned checks = 0;
unsigned failures = 0;
unsigned prisonerSpawnRequests = 0;

void check(bool condition, const char* message)
{
    ++checks;
    if (!condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void setTeamRange(UINT8 team, UINT16 first, UINT16 last)
{
    gTacticalStatus.Team[team].bFirstID = SoldierID(first);
    gTacticalStatus.Team[team].bLastID = SoldierID(last);
    for (UINT16 slot = first; slot <= last; ++slot)
        MercPtrs[slot] = nullptr;
}

void placeSoldier(SOLDIERTYPE& soldier, UINT8 team, UINT16 slot, INT32 grid)
{
    soldier.ubID = SoldierID(slot);
    soldier.bTeam = team;
    soldier.bActive = TRUE;
    soldier.bInSector = TRUE;
    soldier.ubProfile = NO_PROFILE;
    soldier.stats.bLife = OKLIFE;
    soldier.sSectorX = 1;
    soldier.sSectorY = 1;
    soldier.bSectorZ = 0;
    soldier.sGridNo = grid;
    MercPtrs[slot] = &soldier;
}

void checkTeamInitialization()
{
    is_networked = false;
    gbPlayerNum = OUR_TEAM;
    gGameExternalOptions.ubGameMaximumNumberOfPlayerMercs = 2;
    gGameExternalOptions.ubGameMaximumNumberOfPlayerVehicles = 1;
    gGameExternalOptions.ubGameMaximumNumberOfEnemies = 2;
    gGameExternalOptions.ubGameMaximumNumberOfCreatures = 2;
    gGameExternalOptions.ubGameMaximumNumberOfRebels = 2;
    gGameExternalOptions.ubGameMaximumNumberOfCivilians = 2;

    check(InitOverhead(), "the real overhead initialization path succeeds");
    for (UINT8 team : {OUR_TEAM, ENEMY_TEAM, CIV_TEAM})
    {
        const SoldierID first = gTacticalStatus.Team[team].bFirstID;
        const SoldierID last = gTacticalStatus.Team[team].bLastID;
        SOLDIERTYPE* slot = nullptr;
        check(GetInactiveSoldierSlot(&slot, first) && slot->bTeam == team,
              "initialization assigns the first inactive slot to its team");
        slot = nullptr;
        check(GetInactiveSoldierSlot(&slot, last) && slot->bTeam == team,
              "initialization assigns the final inactive slot to its team");
    }

    const SoldierID finalPlayer = gTacticalStatus.Team[OUR_TEAM].bLastID;
    SOLDIERTYPE* slot = nullptr;
    MercPtrs[finalPlayer]->bActive = TRUE;
    check(!GetInactiveSoldierSlot(&slot, finalPlayer) && slot == nullptr,
          "an active record is not advertised as an available slot");
    MercPtrs[finalPlayer]->bActive = FALSE;
    MercPtrs[finalPlayer] = nullptr;
    check(!GetInactiveSoldierSlot(&slot, finalPlayer) && slot == nullptr,
          "missing storage is not advertised as an available slot");
    check(ShutdownOverhead(), "the initialized overhead state shuts down cleanly");
}

void checkPrisonerPlacement()
{
    constexpr UINT16 last = 81;
    SOLDIERTYPE civilian;
    setTeamRange(CIV_TEAM, last - 1, last);
    placeSoldier(civilian, CIV_TEAM, last, 500);
    gGameExternalOptions.fAllowPrisonerSystem = TRUE;
    gGameExternalOptions.ubGameMaximumNumberOfCivilians = 4;
    SectorInfo[SECTOR(1, 1)].uiNumberOfPrisonersOfWar[PRISONER_ADMIN] = 1;

    prisonerSpawnRequests = 0;
    SectorAddPrisonersofWar(1, 1, 0);
    check(prisonerSpawnRequests == 0, "a civilian in the final slot uses the remaining prisoner placement capacity");

    gGameExternalOptions.ubGameMaximumNumberOfCivilians = 5;
    civilian.usSoldierFlagMask |= SOLDIER_POW_PRISON;
    prisonerSpawnRequests = 0;
    SectorAddPrisonersofWar(1, 1, 0);
    check(prisonerSpawnRequests == 0, "a prisoner in the final civilian slot prevents a duplicate spawn");

    civilian.bActive = FALSE;
    prisonerSpawnRequests = 0;
    SectorAddPrisonersofWar(1, 1, 0);
    check(prisonerSpawnRequests == 1,
          "a retired flagged final slot does not suppress the required prisoner spawn");
    civilian.bActive = TRUE;

    MercPtrs[last] = nullptr;
    prisonerSpawnRequests = 0;
    SectorAddPrisonersofWar(1, 1, 0);
    check(prisonerSpawnRequests == 1, "room for an unplaced prisoner triggers one spawn request");
}

void checkPlayerProximity()
{
    constexpr UINT16 last = 2;
    constexpr INT32 grid = 500;
    SOLDIERTYPE mercenary;
    setTeamRange(OUR_TEAM, 0, last);
    placeSoldier(mercenary, OUR_TEAM, last, grid);

    check(GridNoNearPlayerMercs(grid, 1), "the final player slot contributes to the proximity check");
    mercenary.bInSector = FALSE;
    check(!GridNoNearPlayerMercs(grid, 1), "an out-of-sector mercenary does not count as nearby");
    MercPtrs[last] = nullptr;
}

void checkExternalFeeding()
{
    constexpr UINT16 first = 0;
    constexpr UINT16 last = 2;
    constexpr UINT16 weaponItem = 500;
    constexpr UINT16 beltItem = 501;
    constexpr UINT8 calibre = 1;
    constexpr UINT8 ammoType = 1;
    SOLDIERTYPE feeder;
    SOLDIERTYPE receiver;
    setTeamRange(OUR_TEAM, first, last);
    placeSoldier(feeder, OUR_TEAM, first, 500);
    placeSoldier(receiver, OUR_TEAM, last, 501);
    feeder.ubDirection = EAST;
    gGameExternalOptions.ubExternalFeeding = 1;

    Item[weaponItem].usItemClass = IC_GUN;
    Item[weaponItem].usItemFlag = BELT_FED;
    Weapon[weaponItem].ubCalibre = calibre;
    OBJECTTYPE& gun = receiver.inv[HANDPOS];
    gun.usItem = weaponItem;
    gun.ubNumberOfObjects = 1;
    gun[0]->data.gun.ubGunAmmoType = ammoType;
    gun[0]->data.gun.ubGunShotsLeft = 1;

    Item[beltItem].usItemClass = IC_AMMO;
    Item[beltItem].usItemFlag = AMMO_BELT;
    Item[beltItem].ubClassIndex = beltItem;
    Magazine[beltItem].ubCalibre = calibre;
    Magazine[beltItem].ubAmmoType = ammoType;
    OBJECTTYPE& belt = feeder.inv[HANDPOS];
    belt.usItem = beltItem;
    belt.ubNumberOfObjects = 1;
    belt[0]->data.ubShotsLeft = 10;

    SoldierID firstTarget = NOBODY, secondTarget = NOBODY;
    UINT16 firstGun = 0, firstAmmo = 0, secondGun = 0, secondAmmo = 0;
    const bool feeding = feeder.IsFeedingExternal(&firstTarget, &firstGun, &firstAmmo,
                                                   &secondTarget, &secondGun, &secondAmmo);
    check(feeding && firstTarget == receiver.ubID && firstGun == HANDPOS && firstAmmo == HANDPOS,
          "an ammunition feeder finds the recipient in the final team slot");

    placeSoldier(receiver, OUR_TEAM, first, 501);
    placeSoldier(feeder, OUR_TEAM, last, 500);
    check(GetExternalFeedingObject(&receiver, &gun) == &belt,
          "a recipient finds the ammunition belt held by the final teammate");
    feeder.bInSector = FALSE;
    check(GetExternalFeedingObject(&receiver, &gun) == nullptr,
          "an out-of-sector feeder cannot supply ammunition");
    MercPtrs[first] = nullptr;
    MercPtrs[last] = nullptr;
}

void checkRadioOperator(UINT8 team, SoldierID owner)
{
    constexpr UINT16 last = 61;
    constexpr UINT16 radioItem = 502;
    SOLDIERTYPE radioOperator;
    setTeamRange(team, last - 1, last);
    placeSoldier(radioOperator, team, last, 500);
    radioOperator.stats.ubSkillTraits[0] = RADIO_OPERATOR_NT;
    radioOperator.inv[HANDPOS].usItem = radioItem;
    radioOperator.inv[HANDPOS].ubNumberOfObjects = 1;
    Item[radioItem].usItemFlag = RADIO_SET;
    gGameOptions.fNewTraitSystem = TRUE;
    gSkillTraitValues.ubMaxNumberOfTraits = 1;
    gSkillTraitValues.ubNumberOfMajorTraitsAllowed = 1;
    gWorldSectorX = gWorldSectorY = 1;
    gbWorldSectorZ = 0;

    INT32 signalGrid = NOWHERE;
    check(GetRadioOperatorSignal(owner, &signalGrid) && signalGrid == radioOperator.sGridNo,
          team == MILITIA_TEAM ? "the final militia radio operator provides a signal"
                               : "the final enemy radio operator provides a signal");
    radioOperator.sSectorX = 2;
    signalGrid = NOWHERE;
    check(!GetRadioOperatorSignal(owner, &signalGrid) && signalGrid == NOWHERE,
          "a radio operator in a different sector produces no signal");
    MercPtrs[last] = nullptr;
}
}

// The linker redirects CreatePrisonerOfWar here to count placement requests.
extern "C" void __wrap__Z19CreatePrisonerOfWarv()
{
    ++prisonerSpawnRequests;
}

int main()
{
    checkTeamInitialization();
    checkPrisonerPlacement();
    checkPlayerProximity();
    checkExternalFeeding();
    checkRadioOperator(MILITIA_TEAM, SoldierID(0));
    checkRadioOperator(ENEMY_TEAM, SoldierID(1));

    if (failures != 0)
        return 1;

    std::cout << "PASS: " << checks << " final team-slot assertions\n";
    return 0;
}
