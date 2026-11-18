/*
 * P710 native snapshot update: tests/native-core/native_disease_team_slot_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Exercise hourly disease processing with sparse player-team ownership slots.
#include "Disease.h"
#include "GameSettings.h"
#include "Overhead.h"

#include <iostream>
#include <stdexcept>

namespace {
unsigned checks = 0;

void check(bool value, const char* message)
{
	++checks;
	if (!value)
	{
		throw std::runtime_error(message);
	}
}
}

int main()
{
	const UINT8 previousPlayer = gbPlayerNum;
	const SoldierID previousFirst = gTacticalStatus.Team[0].bFirstID;
	const SoldierID previousLast = gTacticalStatus.Team[0].bLastID;
	SOLDIERTYPE* const previousSlot0 = MercPtrs[0];
	SOLDIERTYPE* const previousSlot1 = MercPtrs[1];
	const BOOLEAN previousDisease = gGameExternalOptions.fDisease;
	const BOOLEAN previousStrategic = gGameExternalOptions.fDiseaseStrategic;
	const BOOLEAN previousLimitations = gGameExternalOptions.fDiseaseSevereLimitations;
	const BOOLEAN previousOpinions = gGameExternalOptions.fDynamicOpinions;
	const DISEASE previousDiseaseDefinition = Disease[0];

	try
	{
		gbPlayerNum = 0;
		gTacticalStatus.Team[0].bFirstID = SoldierID(0);
		gTacticalStatus.Team[0].bLastID = SoldierID(1);
		gGameExternalOptions.fDisease = TRUE;
		gGameExternalOptions.fDiseaseStrategic = FALSE;
		gGameExternalOptions.fDiseaseSevereLimitations = FALSE;
		gGameExternalOptions.fDynamicOpinions = FALSE;

		Disease[0].sInfectionPtsGainPerHour = 1;
		Disease[0].sInfectionPtsOutbreak = 900;
		Disease[0].sInfectionPtsFull = 1000;
		Disease[0].usDiseaseProperties = 0;

		SOLDIERTYPE liveSoldier;
		liveSoldier.initialize();
		liveSoldier.ubID = SoldierID(1);
		liveSoldier.bTeam = gbPlayerNum;
		liveSoldier.bActive = TRUE;
		liveSoldier.stats.bLife = 0;
		liveSoldier.sSectorX = 1;
		liveSoldier.sSectorY = 1;
		liveSoldier.bSectorZ = 0;
		liveSoldier.sDiseasePoints[0] = 10;

		MercPtrs[0] = nullptr;
		MercPtrs[1] = &liveSoldier;
		HandleDisease();
		check(liveSoldier.sDiseasePoints[0] == 11,
		      "a live soldier after an empty team slot receives the hourly disease update");

		liveSoldier.bActive = FALSE;
		liveSoldier.sDiseasePoints[0] = 20;
		HandleDisease();
		check(liveSoldier.sDiseasePoints[0] == 20,
		      "an inactive owner in the team range is ignored");

		Disease[0] = previousDiseaseDefinition;
		gGameExternalOptions.fDisease = previousDisease;
		gGameExternalOptions.fDiseaseStrategic = previousStrategic;
		gGameExternalOptions.fDiseaseSevereLimitations = previousLimitations;
		gGameExternalOptions.fDynamicOpinions = previousOpinions;
		MercPtrs[0] = previousSlot0;
		MercPtrs[1] = previousSlot1;
		gTacticalStatus.Team[0].bFirstID = previousFirst;
		gTacticalStatus.Team[0].bLastID = previousLast;
		gbPlayerNum = previousPlayer;

		std::cout << "PASS: " << checks << " disease team-slot assertions\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		Disease[0] = previousDiseaseDefinition;
		gGameExternalOptions.fDisease = previousDisease;
		gGameExternalOptions.fDiseaseStrategic = previousStrategic;
		gGameExternalOptions.fDiseaseSevereLimitations = previousLimitations;
		gGameExternalOptions.fDynamicOpinions = previousOpinions;
		MercPtrs[0] = previousSlot0;
		MercPtrs[1] = previousSlot1;
		gTacticalStatus.Team[0].bFirstID = previousFirst;
		gTacticalStatus.Team[0].bLastID = previousLast;
		gbPlayerNum = previousPlayer;

		std::cerr << "FAIL after " << checks << " assertions: " << error.what() << '\n';
		return 1;
	}
}
