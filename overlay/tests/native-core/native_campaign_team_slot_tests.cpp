/*
 * P710 native snapshot update: tests/native-core/native_campaign_team_slot_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Exercise active-squad experience awards with sparse player-team ownership slots.
#include "Campaign.h"
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

	try
	{
		gbPlayerNum = 0;
		gTacticalStatus.Team[0].bFirstID = SoldierID(0);
		gTacticalStatus.Team[0].bLastID = SoldierID(1);

		SOLDIERTYPE liveSoldier;
		liveSoldier.initialize();
		liveSoldier.ubID = SoldierID(1);
		liveSoldier.bTeam = gbPlayerNum;
		liveSoldier.bActive = TRUE;
		liveSoldier.bInSector = FALSE;
		liveSoldier.stats.bLife = 50;

		MercPtrs[0] = nullptr;
		MercPtrs[1] = &liveSoldier;
		AwardExperienceBonusToActiveSquad(EXP_BONUS_MINIMUM);
		check(liveSoldier.bActive == TRUE && liveSoldier.bInSector == FALSE,
		      "a live owner after an empty team slot remains valid and unchanged");

		MercPtrs[0] = previousSlot0;
		MercPtrs[1] = previousSlot1;
		gTacticalStatus.Team[0].bFirstID = previousFirst;
		gTacticalStatus.Team[0].bLastID = previousLast;
		gbPlayerNum = previousPlayer;

		std::cout << "PASS: " << checks << " campaign team-slot assertions\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		MercPtrs[0] = previousSlot0;
		MercPtrs[1] = previousSlot1;
		gTacticalStatus.Team[0].bFirstID = previousFirst;
		gTacticalStatus.Team[0].bLastID = previousLast;
		gbPlayerNum = previousPlayer;

		std::cerr << "FAIL after " << checks << " assertions: " << error.what() << '\n';
		return 1;
	}
}
