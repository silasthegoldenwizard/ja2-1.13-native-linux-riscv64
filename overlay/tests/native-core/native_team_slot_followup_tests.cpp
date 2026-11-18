/*
 * P710 native snapshot update: tests/native-core/native_team_slot_followup_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Exercise retained team-slot consumers with sparse or stale runtime ownership.
#include "Overhead.h"
#include "Boxing.h"
#include "GameSettings.h"
#include "GameInitOptionsScreen.h"
#include "Strategic Transport Groups.h"
#include "mapscreen.h"

#include <cstring>
#include <iostream>

extern void HandleMilitiaCommand();

namespace {
unsigned checks = 0;
const char* failure = nullptr;

void check(bool value, const char* message)
{
	++checks;
	if (!value && failure == nullptr)
	{
		failure = message;
	}
}

void saveBoxingState(SoldierID (&ids)[NUM_BOXERS], BOOLEAN (&fought)[NUM_BOXERS])
{
	for (UINT8 index = 0; index < NUM_BOXERS; ++index)
	{
		ids[index] = gubBoxerID[index];
		fought[index] = gfBoxerFought[index];
	}
}

void restoreBoxingState(const SoldierID (&ids)[NUM_BOXERS], const BOOLEAN (&fought)[NUM_BOXERS])
{
	for (UINT8 index = 0; index < NUM_BOXERS; ++index)
	{
		gubBoxerID[index] = ids[index];
		gfBoxerFought[index] = fought[index];
	}
}

void setSingleBoxer(SoldierID id)
{
	for (UINT8 index = 0; index < NUM_BOXERS; ++index)
	{
		gubBoxerID[index] = index == 0 ? id : SoldierID(NOBODY);
		gfBoxerFought[index] = FALSE;
	}
}

void runStaleBoxerScenario()
{
	SoldierID previousIDs[NUM_BOXERS];
	BOOLEAN previousFought[NUM_BOXERS];
	saveBoxingState(previousIDs, previousFought);
	SOLDIERTYPE* const previousSlot = MercPtrs[10];

	setSingleBoxer(SoldierID(10));
	MercPtrs[10] = nullptr;
	check(BoxerAvailable() == FALSE, "a stale boxer ID is not available");
	check(BoxersAvailable() == 0, "a stale boxer ID is not counted");
	check(PickABoxer() == FALSE, "a stale boxer ID cannot be selected");

	MercPtrs[10] = previousSlot;
	restoreBoxingState(previousIDs, previousFought);
}

void runSparseBoxingPlayerScenario()
{
	const UINT8 previousPlayer = gbPlayerNum;
	const SoldierID previousFirst = gTacticalStatus.Team[0].bFirstID;
	const SoldierID previousLast = gTacticalStatus.Team[0].bLastID;
	SOLDIERTYPE* const previousSlot0 = MercPtrs[0];
	SOLDIERTYPE* const previousSlot10 = MercPtrs[10];
	SoldierID previousIDs[NUM_BOXERS];
	BOOLEAN previousFought[NUM_BOXERS];
	saveBoxingState(previousIDs, previousFought);

	SOLDIERTYPE liveBoxer;
	liveBoxer.initialize();
	liveBoxer.ubID = SoldierID(10);
	liveBoxer.bActive = TRUE;
	liveBoxer.bInSector = TRUE;
	liveBoxer.stats.bLife = OKLIFE;

	gbPlayerNum = 0;
	gTacticalStatus.Team[0].bFirstID = SoldierID(0);
	gTacticalStatus.Team[0].bLastID = SoldierID(0);
	MercPtrs[0] = nullptr;
	MercPtrs[10] = &liveBoxer;
	setSingleBoxer(SoldierID(10));
	check(AnotherFightPossible() == FALSE,
	      "an empty player-team slot cannot provide another boxer");

	MercPtrs[0] = previousSlot0;
	MercPtrs[10] = previousSlot10;
	restoreBoxingState(previousIDs, previousFought);
	gTacticalStatus.Team[0].bFirstID = previousFirst;
	gTacticalStatus.Team[0].bLastID = previousLast;
	gbPlayerNum = previousPlayer;
}

void runSparseMilitiaCommandScenario()
{
	const UINT8 previousPlayer = gbPlayerNum;
	const SoldierID previousFirst = gTacticalStatus.Team[0].bFirstID;
	const SoldierID previousLast = gTacticalStatus.Team[0].bLastID;
	SOLDIERTYPE* const previousSlot = MercPtrs[0];

	gbPlayerNum = 0;
	gTacticalStatus.Team[0].bFirstID = SoldierID(0);
	gTacticalStatus.Team[0].bLastID = SoldierID(0);
	MercPtrs[0] = nullptr;
	HandleMilitiaCommand();
	check(true, "an empty player-team slot is ignored by militia command handling");

	MercPtrs[0] = previousSlot;
	gTacticalStatus.Team[0].bFirstID = previousFirst;
	gTacticalStatus.Team[0].bLastID = previousLast;
	gbPlayerNum = previousPlayer;
}

void runSparseMapCharacterScenario()
{
	const UINT8 previousPlayer = gbPlayerNum;
	const SoldierID previousFirst = gTacticalStatus.Team[0].bFirstID;
	const SoldierID previousLast = gTacticalStatus.Team[0].bLastID;
	SOLDIERTYPE* const previousSlot = MercPtrs[0];

	gbPlayerNum = 0;
	gTacticalStatus.Team[0].bFirstID = SoldierID(0);
	gTacticalStatus.Team[0].bLastID = SoldierID(0);
	MercPtrs[0] = nullptr;
	ReBuildCharactersList();
	check(true, "an empty player-team slot is ignored by the map character rebuild");

	MercPtrs[0] = previousSlot;
	gTacticalStatus.Team[0].bFirstID = previousFirst;
	gTacticalStatus.Team[0].bLastID = previousLast;
	gbPlayerNum = previousPlayer;
}

void runSparseTransportInventoryScenario()
{
	const BOOLEAN previousEnabled = gGameExternalOptions.fStrategicTransportGroupsEnabled;
	const SoldierID previousPlayerFirst = gTacticalStatus.Team[OUR_TEAM].bFirstID;
	const SoldierID previousPlayerLast = gTacticalStatus.Team[OUR_TEAM].bLastID;
	const SoldierID previousEnemyFirst = gTacticalStatus.Team[ENEMY_TEAM].bFirstID;
	const SoldierID previousEnemyLast = gTacticalStatus.Team[ENEMY_TEAM].bLastID;
	SOLDIERTYPE* const previousPlayerSlot = MercPtrs[0];
	SOLDIERTYPE* const previousEnemySlot = MercPtrs[20];
	const UINT16 previousKillsPerPoint =
		zDiffSetting[gGameOptions.ubDifficultyLevel].iNumKillsPerProgressPoint;

	gGameExternalOptions.fStrategicTransportGroupsEnabled = TRUE;
	gTacticalStatus.Team[OUR_TEAM].bFirstID = SoldierID(0);
	gTacticalStatus.Team[OUR_TEAM].bLastID = SoldierID(0);
	gTacticalStatus.Team[ENEMY_TEAM].bFirstID = SoldierID(20);
	gTacticalStatus.Team[ENEMY_TEAM].bLastID = SoldierID(20);
	MercPtrs[0] = nullptr;
	MercPtrs[20] = nullptr;
	zDiffSetting[gGameOptions.ubDifficultyLevel].iNumKillsPerProgressPoint = 1;
	UpdateTransportGroupInventory();
	check(true, "empty player and enemy slots are ignored by transport inventory updates");

	MercPtrs[0] = previousPlayerSlot;
	MercPtrs[20] = previousEnemySlot;
	gTacticalStatus.Team[OUR_TEAM].bFirstID = previousPlayerFirst;
	gTacticalStatus.Team[OUR_TEAM].bLastID = previousPlayerLast;
	gTacticalStatus.Team[ENEMY_TEAM].bFirstID = previousEnemyFirst;
	gTacticalStatus.Team[ENEMY_TEAM].bLastID = previousEnemyLast;
	zDiffSetting[gGameOptions.ubDifficultyLevel].iNumKillsPerProgressPoint = previousKillsPerPoint;
	gGameExternalOptions.fStrategicTransportGroupsEnabled = previousEnabled;
}
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "usage: native_team_slot_followup_tests MODE\n";
		return 2;
	}

	if (std::strcmp(argv[1], "stale-boxer") == 0)
		runStaleBoxerScenario();
	else if (std::strcmp(argv[1], "boxing-player") == 0)
		runSparseBoxingPlayerScenario();
	else if (std::strcmp(argv[1], "militia-command") == 0)
		runSparseMilitiaCommandScenario();
	else if (std::strcmp(argv[1], "map-characters") == 0)
		runSparseMapCharacterScenario();
	else if (std::strcmp(argv[1], "transport-inventory") == 0)
		runSparseTransportInventoryScenario();
	else
	{
		std::cerr << "unknown mode: " << argv[1] << '\n';
		return 2;
	}

	if (failure != nullptr)
	{
		std::cerr << "FAIL after " << checks << " assertions: " << failure << '\n';
		return 1;
	}

	std::cout << "PASS: " << checks << " team-slot follow-up assertions\n";
	return 0;
}
