/*
 * P710 native snapshot update: tests/native-core/native_squad_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_squad_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "Assignments.h"
#include "Auto Bandage.h"
#include "BufferMan.h"
#include "Food.h"
#include "interface Dialogue.h"
#include "Overhead.h"
#include "Quests.h"
#include "Soldier Profile.h"
#include "Strategic Movement.h"
#include "Strategic Status.h"
#include "Squads.h"
#include "Tactical Save.h"
#include "Vehicles.h"
#include "World Items.h"
#include "jascreens.h"
#include "screenids.h"
#include "strategicmap.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

extern INT8 SquadMovementGroups[NUMBER_OF_SQUADS];
extern BOOLEAN GetSoldiersInSquad(INT32, SOLDIERTYPE*[]);
extern SECTOR_EXT_DATA SectorExternalData[256][4];
extern UINT16 NumActiveAndConsciousTeamMembers(UINT8);

namespace {
using Bytes = std::vector<UINT8>;

struct SquadRecord {
    INT16 id;
    INT16 padding[5];
};

static_assert(sizeof(SquadRecord) == 12, "squad save record wire size changed");

unsigned checks = 0;
unsigned failures = 0;

void check(bool condition, const char* message)
{
    ++checks;
    if (condition) return;
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}

template<class T>
void append(Bytes& bytes, const T& value)
{
    const auto* begin = reinterpret_cast<const UINT8*>(&value);
    bytes.insert(bytes.end(), begin, begin + sizeof(value));
}

Bytes squadData(INT16 firstID, bool includeMovement = true)
{
    Bytes bytes;
    for (INT32 squad = 0; squad < NUMBER_OF_SQUADS; ++squad) {
        for (INT32 slot = 0; slot < NUMBER_OF_SOLDIERS_PER_SQUAD; ++slot) {
            SquadRecord record{};
            record.id = squad == 0 && slot == 0 ? firstID : -1;
            append(bytes, record);
        }
    }
    if (includeMovement) {
        for (INT32 squad = 0; squad < NUMBER_OF_SQUADS; ++squad) {
            const INT8 movement = static_cast<INT8>(squad + 1);
            append(bytes, movement);
        }
    }
    return bytes;
}

BOOLEAN load(Bytes& bytes, std::size_t& consumed)
{
    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;
    const BOOLEAN result = LoadSquadInfoFromSavedGameFile(&cursor);
    consumed = static_cast<std::size_t>(cursor - begin);
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    return result;
}

bool squadsAreEmpty()
{
    for (INT32 squad = 0; squad < NUMBER_OF_SQUADS; ++squad) {
        for (INT32 slot = 0; slot < NUMBER_OF_SOLDIERS_PER_SQUAD; ++slot) {
            if (Squad[squad][slot] != nullptr) return false;
        }
    }
    return true;
}

bool movementIsEmpty()
{
    for (const auto value : SquadMovementGroups) {
        if (value != 0) return false;
    }
    return true;
}

void clearState()
{
    auto empty = squadData(-1);
    std::size_t consumed = 0;
    check(load(empty, consumed), "empty squad table loads");
    check(consumed == empty.size(), "empty squad table consumes its exact extent");
    check(squadsAreEmpty(), "empty squad table clears all member pointers");
}

void invalidIDRegression(INT16 id)
{
    auto invalid = squadData(id);
    std::size_t consumed = 0;
    check(!load(invalid, consumed), "out-of-range squad member ID is rejected");
    check(!gfBufferLoadFailed,
          "invalid squad member ID is a semantic rejection, not a short read");
    check(consumed <= invalid.size(), "invalid-ID cursor remains in bounds");
    check(squadsAreEmpty(), "invalid member ID leaves no live squad pointers");
    check(movementIsEmpty(), "invalid member ID leaves no movement IDs");
}

void queryContractRegression()
{
    check(static_cast<const SOLDIERTYPE*>(NOBODY) == nullptr &&
              NOBODY.operator->() == nullptr,
          "NOBODY sentinel cannot index the runtime soldier table");
    check(SquadIsEmpty(-1), "negative squad index is treated as empty");
    check(SquadIsEmpty(NUMBER_OF_SQUADS),
          "past-end squad index is treated as empty");
    check(!IsThisSquadFull(-1), "invalid squad cannot be full");
    check(!IsCharacterInSquad(nullptr, 0),
          "null soldier is never a squad member");
    check(SlotCharacterIsInSquad(nullptr, 0) == -1,
          "null soldier has no squad slot");
    check(SquadCharacterIsIn(nullptr) == -1,
          "null soldier has no squad assignment");
    check(NumberOfPeopleInSquad(-1) == 0,
          "negative squad has no members");
    check(NumberOfNonEPCsInSquad(-1) == 0,
          "negative squad has no regular mercs");
    check(!IsRobotControllerInSquad(-1),
          "negative squad has no robot controller");
    check(NumberOfPlayerControllableMercsInSquad(-1) == 0,
          "negative squad has no controllable mercs");
    check(!DoesVehicleExistInSquad(-1),
          "negative squad has no vehicle");
    check(!IsThisSquadOnTheMove(-1),
          "negative squad is not moving");

    INT16 sectorX = -1;
    INT16 sectorY = -1;
    INT16 sectorZ = -1;
    check(!SectorSquadIsIn(-1, &sectorX, &sectorY, &sectorZ),
          "sector lookup rejects an invalid squad");
    check(!SectorSquadIsIn(0, nullptr, &sectorY, &sectorZ),
          "sector lookup rejects missing output storage");
    check(!GetSoldiersInSquad(0, nullptr),
          "squad copy rejects missing output storage");
    check(!IsSquadInSector(nullptr, 0),
          "sector membership rejects a missing soldier");
    check(!IsSquadInSector(&Menptr[0], NUMBER_OF_SQUADS),
          "sector membership rejects an invalid squad");

    auto& member = Menptr[0];
    member.initialize();
    member.ubID = SoldierID(0);
    member.bActive = TRUE;
    member.bAssignment = 0;
    member.sSectorX = 4;
    member.sSectorY = 5;
    member.bSectorZ = 1;
    member.flags.fBetweenSectors = FALSE;
    MercPtrs[0] = &member;
    Squad[0][NUMBER_OF_SOLDIERS_PER_SQUAD - 1] = &member;

    check(!SquadIsEmpty(0) && NumberOfPeopleInSquad(0) == 1,
          "valid sparse squad retains its member count");
    check(IsCharacterInSquad(&member, 0) &&
              SlotCharacterIsInSquad(&member, 0) ==
                  NUMBER_OF_SOLDIERS_PER_SQUAD - 1 &&
              SquadCharacterIsIn(&member) == 0,
          "valid sparse squad retains member lookup");
    sectorX = sectorY = sectorZ = -1;
    check(SectorSquadIsIn(0, &sectorX, &sectorY, &sectorZ) &&
              sectorX == 4 && sectorY == 5 && sectorZ == 1,
          "valid sparse squad returns its live sector");
    check(IsSquadInSector(&member, 0),
          "sector membership finds a live sparse squad member");
    SOLDIERTYPE* copied[NUMBER_OF_SOLDIERS_PER_SQUAD]{};
    check(GetSoldiersInSquad(0, copied) &&
              copied[NUMBER_OF_SOLDIERS_PER_SQUAD - 1] == &member,
          "valid sparse squad copies its member table");

    Squad[0][NUMBER_OF_SOLDIERS_PER_SQUAD - 1] = nullptr;
    MercPtrs[0] = nullptr;
}

void mutationContractRegression()
{
    check(!RemoveCharacterFromSquads(nullptr),
          "null soldier cannot be removed from empty squad slots");
    check(!RemoveCharacterFromASquad(nullptr, 0),
          "null soldier cannot be removed from a squad");
    check(!RemoveCharacterFromASquad(&Menptr[0], -1) &&
              !RemoveCharacterFromASquad(&Menptr[0], NUMBER_OF_SQUADS),
          "squad removal rejects invalid squad indices");
    check(!AddCharacterToSquad(nullptr, 0),
          "null soldier cannot be added to a squad");
    check(!AddCharacterToSquad(&Menptr[0], -1) &&
              !AddCharacterToSquad(&Menptr[0], NUMBER_OF_SQUADS),
          "squad insertion rejects invalid squad indices");
    check(!AddCharacterToAnySquad(nullptr),
          "null soldier cannot be added to any squad");
    check(AddCharacterToUniqueSquad(nullptr) == -1,
          "null soldier cannot receive a unique squad");
    check(!CopyPathOfSquadToCharacter(nullptr, 0) &&
              !CopyPathOfSquadToCharacter(&Menptr[0], -1),
          "path copy to soldier rejects invalid ownership");
    check(!CopyPathOfCharacterToSquad(nullptr, 0) &&
              !CopyPathOfCharacterToSquad(&Menptr[0], -1),
          "path copy from soldier rejects invalid ownership");
}

void movementContractRegression()
{
    UINT8 nextX = 9;
    UINT8 nextY = 9;
    UINT8 previousX = 9;
    UINT8 previousY = 9;
    UINT32 traverseTime = 9;
    UINT32 arriveTime = 9;

    GetSquadPosition(nullptr, &nextY, &previousX, &previousY,
                     &traverseTime, &arriveTime, 0);
    check(nextY == 9 && previousX == 9 && previousY == 9 &&
              traverseTime == 9 && arriveTime == 9,
          "squad position rejects incomplete output storage");

    GetSquadPosition(&nextX, &nextY, &previousX, &previousY,
                     &traverseTime, &arriveTime, NUMBER_OF_SQUADS);
    check(nextX == 0 && nextY == 0 && previousX == 0 && previousY == 0 &&
              traverseTime == 0 && arriveTime == 0,
          "invalid squad position returns an empty movement state");

    INT8 previousMovementGroups[NUMBER_OF_SQUADS]{};
    std::memcpy(previousMovementGroups, SquadMovementGroups,
                sizeof(previousMovementGroups));
    SetSquadPositionBetweenSectors(1, 2, 3, 4, 5, 6, NUMBER_OF_SQUADS);
    check(std::memcmp(previousMovementGroups, SquadMovementGroups,
                      sizeof(previousMovementGroups)) == 0,
          "invalid squad movement update leaves the movement table unchanged");

    INT16 locationX = 9;
    INT16 locationY = 9;
    INT8 locationZ = 9;
    GetLocationOfSquad(&locationX, &locationY, &locationZ, -1);
    GetLocationOfSquad(nullptr, nullptr, nullptr, 0);
    check(locationX == 9 && locationY == 9 && locationZ == 9,
          "invalid squad location returns without mutating outputs");
}

void movementGroupContractRegression()
{
    auto& vehicleSoldier = Menptr[0];
    vehicleSoldier.initialize();
    vehicleSoldier.ubID = SoldierID(0);
    vehicleSoldier.ubProfile = 0;
    vehicleSoldier.ubGroupID = 77;
    vehicleSoldier.bActive = TRUE;

    const UINT32 previousProfileType = gMercProfiles[0].Type;
    VEHICLETYPE* const previousVehicleList = pVehicleList;
    const UINT8 previousVehicleCount = ubNumberOfVehicles;

    gMercProfiles[0].Type = PROFILETYPE_VEHICLE;
    MercPtrs[0] = &vehicleSoldier;
    pVehicleList = nullptr;
    ubNumberOfVehicles = 0;

    const UINT8 previousSquadSize = gGameOptions.ubSquadSize;
    gGameOptions.ubSquadSize = NUMBER_OF_SOLDIERS_PER_SQUAD;
    check(!AddCharacterToSquad(&vehicleSoldier, 0) && SquadIsEmpty(0),
          "vehicle without a live record cannot enter a squad");

    CheckSquadMovementGroups();
    vehicleSoldier.bAssignment = VEHICLE;
    vehicleSoldier.iVehicleId = 0;
    check(!AddCharacterToSquad(&vehicleSoldier, 0) && SquadIsEmpty(0),
          "passenger with a missing vehicle cannot enter a squad");
    vehicleSoldier.bAssignment = 0;
    vehicleSoldier.iVehicleId = -1;

    Squad[0][0] = &vehicleSoldier;
    CheckSquadMovementGroups();
    check(vehicleSoldier.ubGroupID == 77,
          "missing vehicle record leaves its soldier group unchanged");

    VEHICLETYPE vehicle{};
    vehicle.fValid = TRUE;
    vehicle.ubProfileID = vehicleSoldier.ubProfile;
    vehicle.ubMovementGroup = 42;
    pVehicleList = &vehicle;
    ubNumberOfVehicles = 1;
    CheckSquadMovementGroups();
    check(vehicleSoldier.ubGroupID == vehicle.ubMovementGroup,
          "matching vehicle record supplies its movement group");

    Squad[0][0] = nullptr;
    MercPtrs[0] = nullptr;
    gMercProfiles[0].Type = previousProfileType;
    pVehicleList = previousVehicleList;
    ubNumberOfVehicles = previousVehicleCount;
    gGameOptions.ubSquadSize = previousSquadSize;
    RemoveAllGroups();
}

void sectorFlagTeamRangeRegression()
{
    const SoldierID previousFirstID = gTacticalStatus.Team[gbPlayerNum].bFirstID;
    const SoldierID previousLastID = gTacticalStatus.Team[gbPlayerNum].bLastID;
    const UINT32 previousSectorFlags = SectorInfo[SECTOR(1, 1)].uiFlags;
    const UINT8 previousVisitedToday =
        gStrategicStatus.ubNumNewSectorsVisitedToday;

    SectorInfo[SECTOR(1, 1)].uiFlags &= ~SF_ALREADY_VISITED;
    gTacticalStatus.Team[gbPlayerNum].bFirstID = SoldierID(0);
    gTacticalStatus.Team[gbPlayerNum].bLastID = NOBODY;

    check(SetSectorFlag(1, 1, 0, SF_ALREADY_VISITED),
          "visited-sector update accepts an empty runtime team range");
    check(GetSectorFlagStatus(1, 1, 0, SF_ALREADY_VISITED),
          "visited-sector update still publishes its flag");
    check(!SetSectorFlag(0, 1, 0, SF_ALREADY_VISITED) &&
              !ReSetSectorFlag(17, 1, 0, SF_ALREADY_VISITED) &&
              !GetSectorFlagStatus(1, 17, 0, SF_ALREADY_VISITED) &&
              !SetSectorFlag(1, 1, MAXIMUM_VALID_Z_COORDINATE + 1,
                             SF_ALREADY_VISITED),
          "sector-flag operations reject invalid map coordinates");

    gTacticalStatus.Team[gbPlayerNum].bFirstID = previousFirstID;
    gTacticalStatus.Team[gbPlayerNum].bLastID = previousLastID;
    SectorInfo[SECTOR(1, 1)].uiFlags = previousSectorFlags;
    gStrategicStatus.ubNumNewSectorsVisitedToday = previousVisitedToday;
}

void autoBandageTeamRangeRegression()
{
    const SoldierID previousFirstID = gTacticalStatus.Team[OUR_TEAM].bFirstID;
    const SoldierID previousLastID = gTacticalStatus.Team[OUR_TEAM].bLastID;

    auto& patient = Menptr[0];
    patient.initialize();
    patient.ubID = SoldierID(0);
    patient.bActive = TRUE;
    patient.flags.fBetweenSectors = TRUE;
    patient.bBleeding = 1;
    patient.stats.bLife = 1;
    patient.sSectorX = 9;
    patient.sSectorY = 10;

    MercPtrs[0] = &patient;
    MercPtrs[1] = nullptr;
    gTacticalStatus.Team[OUR_TEAM].bFirstID = SoldierID(0);
    gTacticalStatus.Team[OUR_TEAM].bLastID = SoldierID(1);

    check(GetBestRetreatingMercDoctor(&patient) == NOBODY,
          "retreat doctor lookup skips an empty runtime team slot");

    MercPtrs[0] = nullptr;
    SetRetreatBandaging(TRUE);
    HandleRetreatBandaging();
    check(!RetreatBandagingPending(),
          "retreat bandaging retires cleanly with an empty runtime team range");

    gTacticalStatus.Team[OUR_TEAM].bFirstID = previousFirstID;
    gTacticalStatus.Team[OUR_TEAM].bLastID = previousLastID;
    MercPtrs[0] = nullptr;
    MercPtrs[1] = nullptr;
}

void vehicleDriverTeamRangeRegression()
{
    const SoldierID previousFirstID = gTacticalStatus.Team[OUR_TEAM].bFirstID;
    const SoldierID previousLastID = gTacticalStatus.Team[OUR_TEAM].bLastID;

    auto& driver = Menptr[0];
    driver.initialize();
    driver.ubID = SoldierID(0);
    driver.bActive = TRUE;

    MercPtrs[0] = &driver;
    MercPtrs[1] = nullptr;
    gTacticalStatus.Team[OUR_TEAM].bFirstID = SoldierID(0);
    gTacticalStatus.Team[OUR_TEAM].bLastID = SoldierID(1);

    check(OnlythisCanDriveVehicle(&driver, 0),
          "vehicle driver lookup skips an empty runtime team slot");

    gTacticalStatus.Team[OUR_TEAM].bFirstID = previousFirstID;
    gTacticalStatus.Team[OUR_TEAM].bLastID = previousLastID;
    MercPtrs[0] = nullptr;
    MercPtrs[1] = nullptr;
}

void questFactTeamRangeRegression()
{
    const SoldierID previousFirstID = gTacticalStatus.Team[gbPlayerNum].bFirstID;
    const SoldierID previousLastID = gTacticalStatus.Team[gbPlayerNum].bLastID;

    gTacticalStatus.Team[gbPlayerNum].bFirstID = SoldierID(0);
    gTacticalStatus.Team[gbPlayerNum].bLastID = SoldierID(0);
    MercPtrs[0] = nullptr;

    check(!CheckPlayerHasHead(),
          "quest fact lookup skips an empty runtime player-team slot");

    gTacticalStatus.Team[gbPlayerNum].bFirstID = previousFirstID;
    gTacticalStatus.Team[gbPlayerNum].bLastID = previousLastID;
    MercPtrs[0] = nullptr;
}

void dialogueMedicalCostTeamRangeRegression()
{
    constexpr UINT8 doctorProfile = 77;
    const SoldierID previousFirstID = gTacticalStatus.Team[gbPlayerNum].bFirstID;
    const SoldierID previousLastID = gTacticalStatus.Team[gbPlayerNum].bLastID;
    SOLDIERTYPE* const previousDoctor = MercPtrs[2];

    auto& doctor = Menptr[2];
    doctor.initialize();
    doctor.ubID = SoldierID(2);
    doctor.bActive = TRUE;
    doctor.ubProfile = doctorProfile;
    doctor.sGridNo = 0;

    MercPtrs[0] = nullptr;
    MercPtrs[1] = nullptr;
    MercPtrs[2] = &doctor;
    gTacticalStatus.Team[gbPlayerNum].bFirstID = SoldierID(0);
    gTacticalStatus.Team[gbPlayerNum].bLastID = SoldierID(1);

    check(CalcMedicalCost(doctorProfile) == 10,
          "dialogue medical-cost lookup skips empty runtime team slots");

    gTacticalStatus.Team[gbPlayerNum].bFirstID = previousFirstID;
    gTacticalStatus.Team[gbPlayerNum].bLastID = previousLastID;
    MercPtrs[0] = nullptr;
    MercPtrs[1] = nullptr;
    MercPtrs[2] = previousDoctor;
}

void playerTeamCapacityRangeRegression()
{
    const SoldierID previousFirstID = gTacticalStatus.Team[gbPlayerNum].bFirstID;
    const SoldierID previousLastID = gTacticalStatus.Team[gbPlayerNum].bLastID;

    gTacticalStatus.Team[gbPlayerNum].bFirstID = SoldierID(0);
    gTacticalStatus.Team[gbPlayerNum].bLastID = SoldierID(0);
    MercPtrs[0] = nullptr;

    check(CountNonVehiclesOnPlayerTeam() == 0,
          "player-team capacity lookup skips an empty runtime slot");

    gTacticalStatus.Team[gbPlayerNum].bFirstID = previousFirstID;
    gTacticalStatus.Team[gbPlayerNum].bLastID = previousLastID;
    MercPtrs[0] = nullptr;
}

void overheadTeamRangeRegression()
{
    const SoldierID previousFirstID = gTacticalStatus.Team[OUR_TEAM].bFirstID;
    const SoldierID previousLastID = gTacticalStatus.Team[OUR_TEAM].bLastID;

    gTacticalStatus.Team[OUR_TEAM].bFirstID = SoldierID(0);
    gTacticalStatus.Team[OUR_TEAM].bLastID = SoldierID(0);
    MercPtrs[0] = nullptr;

    check(!TeamMemberNear(OUR_TEAM, 0, 1),
          "team proximity lookup skips an empty runtime slot");
    check(!TeamMemberNear(-1, 0, 1),
          "team proximity lookup rejects an invalid team index");
    check(NumActiveAndConsciousTeamMembers(OUR_TEAM) == 0,
          "active-team count skips an empty runtime slot");
    check(NumActiveAndConsciousTeamMembers(MAXTEAMS) == 0,
          "active-team count rejects an invalid team index");
    check(!CheckForPlayerTeamInMissionExit(),
          "mission-exit lookup skips an empty runtime slot");
    HandleTeamServices(OUR_TEAM);
    check(true, "team services tolerate an empty runtime team range");
    HandleTeamServices(MAXTEAMS);
    check(true, "team services reject an invalid team index");
    DoPOWPathChecks();
    check(true, "POW path checks tolerate an empty runtime player-team range");

    gTacticalStatus.Team[OUR_TEAM].bFirstID = previousFirstID;
    gTacticalStatus.Team[OUR_TEAM].bLastID = previousLastID;
    MercPtrs[0] = nullptr;
}

void overheadSelectionRangeRegression()
{
    const SoldierID previousFirstID = gTacticalStatus.Team[OUR_TEAM].bFirstID;
    const SoldierID previousLastID = gTacticalStatus.Team[OUR_TEAM].bLastID;

    auto& current = Menptr[1];
    current.initialize();
    current.ubID = SoldierID(1);
    current.bActive = TRUE;
    current.bInSector = TRUE;
    current.bTeam = OUR_TEAM;
    current.stats.bLife = 80;
    current.bAssignment = 0;

    MercPtrs[0] = nullptr;
    MercPtrs[1] = &current;
    MercPtrs[2] = nullptr;
    gTacticalStatus.Team[OUR_TEAM].bFirstID = SoldierID(0);
    gTacticalStatus.Team[OUR_TEAM].bLastID = SoldierID(2);

    check(FindNextActiveAndAliveMerc(&current, FALSE, TRUE) == current.ubID,
          "next-merc selection skips empty runtime team slots");
    check(FindPrevActiveAndAliveMerc(&current, FALSE, TRUE) == current.ubID,
          "previous-merc selection skips empty runtime team slots");

    MercPtrs[1] = nullptr;
    gTacticalStatus.Team[OUR_TEAM].bFirstID = SoldierID(0);
    gTacticalStatus.Team[OUR_TEAM].bLastID = SoldierID(0);
    check(LastActiveTeamMember(OUR_TEAM) == SoldierID(0),
          "last-active lookup tolerates an empty runtime team range");
    check(LastActiveTeamMember(MAXTEAMS) == NOBODY,
          "last-active lookup rejects an invalid team index");

    gTacticalStatus.Team[OUR_TEAM].bFirstID = previousFirstID;
    gTacticalStatus.Team[OUR_TEAM].bLastID = previousLastID;
    MercPtrs[0] = nullptr;
    MercPtrs[1] = nullptr;
    MercPtrs[2] = nullptr;
}

void sectorFillCanteensTeamRangeRegression()
{
    const UINT32 previousScreen = guiCurrentScreen;
    const UINT32 previousFlags = gTacticalStatus.uiFlags;
    const BOOLEAN previousEnemy = gTacticalStatus.fEnemyInSector;
    const SoldierID previousSelected = gusSelectedSoldier;
    const SoldierID previousFirstID = gTacticalStatus.Team[gbPlayerNum].bFirstID;
    const SoldierID previousLastID = gTacticalStatus.Team[gbPlayerNum].bLastID;
    const INT16 previousX = gWorldSectorX;
    const INT16 previousY = gWorldSectorY;
    const INT8 previousZ = gbWorldSectorZ;
    const UINT32 previousWorldItemCount = guiNumWorldItems;
    const UINT8 previousWater = SectorExternalData[SECTOR(1, 1)][0].usWaterType;
	std::vector<WORLDITEM> previousWorldItems;
	previousWorldItems.swap(gWorldItems);

    guiCurrentScreen = GAME_SCREEN;
    gTacticalStatus.uiFlags &= ~INCOMBAT;
    gTacticalStatus.fEnemyInSector = FALSE;
    gusSelectedSoldier = SoldierID(0);
    gWorldSectorX = 1;
    gWorldSectorY = 1;
    gbWorldSectorZ = 0;
    guiNumWorldItems = 1;
    SectorExternalData[SECTOR(1, 1)][0].usWaterType = WATER_DRINKABLE;
    gTacticalStatus.Team[gbPlayerNum].bFirstID = SoldierID(0);
    gTacticalStatus.Team[gbPlayerNum].bLastID = SoldierID(1);
    MercPtrs[0] = nullptr;
    MercPtrs[1] = nullptr;

    SectorFillCanteens();
    check(MercPtrs[0] == nullptr && MercPtrs[1] == nullptr,
          "sector canteen fill skips empty runtime team slots");
	check(gWorldItems.empty(),
	      "sector canteen fill clamps a stale world-item count to the live vector");

    guiCurrentScreen = previousScreen;
    gTacticalStatus.uiFlags = previousFlags;
    gTacticalStatus.fEnemyInSector = previousEnemy;
    gusSelectedSoldier = previousSelected;
    gTacticalStatus.Team[gbPlayerNum].bFirstID = previousFirstID;
    gTacticalStatus.Team[gbPlayerNum].bLastID = previousLastID;
    gWorldSectorX = previousX;
    gWorldSectorY = previousY;
    gbWorldSectorZ = previousZ;
    guiNumWorldItems = previousWorldItemCount;
	previousWorldItems.swap(gWorldItems);
    SectorExternalData[SECTOR(1, 1)][0].usWaterType = previousWater;
}

int finish()
{
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    if (failures) {
        std::cerr << "FAIL: " << failures << " of " << checks
                  << " squad-save lifecycle assertions\n";
        return 1;
    }
    std::cout << "PASS: " << checks
              << " squad-save lifecycle assertions\n";
    return 0;
}
}

int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--invalid-id") == 0) {
        invalidIDRegression(INT16_MAX);
        return finish();
    }

    clearState();

    auto valid = squadData(0);
    std::size_t consumed = 0;
    check(load(valid, consumed), "valid squad member loads");
    check(consumed == valid.size(), "valid squad data consumes its exact extent");
    check(Squad[0][0] == &Menptr[0], "valid member ID rebuilds the live soldier pointer");
    check(Squad[0][1] == nullptr, "empty member slot remains null");
    check(SquadMovementGroups[0] == 1 &&
              SquadMovementGroups[NUMBER_OF_SQUADS - 1] == NUMBER_OF_SQUADS,
          "movement IDs publish after the squad table");

    auto missingMovement = squadData(0, false);
    consumed = 0;
    check(!load(missingMovement, consumed), "missing movement table is rejected");
    check(gfBufferLoadFailed, "missing movement table marks the bounded load failure");
    check(consumed <= missingMovement.size(), "missing-movement cursor remains in bounds");
    check(squadsAreEmpty(), "missing movement table rolls back squad pointers");
    check(movementIsEmpty(), "missing movement table rolls back movement IDs");

    auto truncatedTable = squadData(-1);
    truncatedTable.resize(truncatedTable.size() / 2);
    consumed = 0;
    check(!load(truncatedTable, consumed), "truncated squad table is rejected");
    check(gfBufferLoadFailed, "truncated squad table marks the bounded load failure");
    check(consumed <= truncatedTable.size(), "truncated-table cursor remains in bounds");
    check(squadsAreEmpty(), "truncated squad table leaves no member pointers");
    check(movementIsEmpty(), "truncated squad table leaves no movement IDs");

    invalidIDRegression(INT16_MAX);
    invalidIDRegression(-2);
    clearState();
    queryContractRegression();
    clearState();
    mutationContractRegression();
    clearState();
    movementContractRegression();
    clearState();
    movementGroupContractRegression();
    clearState();
    sectorFlagTeamRangeRegression();
    clearState();
    autoBandageTeamRangeRegression();
    clearState();
    vehicleDriverTeamRangeRegression();
    clearState();
    questFactTeamRangeRegression();
    clearState();
    dialogueMedicalCostTeamRangeRegression();
    clearState();
    playerTeamCapacityRangeRegression();
    clearState();
    overheadTeamRangeRegression();
    clearState();
    overheadSelectionRangeRegression();
    clearState();
    sectorFillCanteensTeamRangeRegression();
    clearState();
    return finish();
}
