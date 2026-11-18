/*
 * P710 native snapshot update: tests/native-core/native_npc_dialogue_chain_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Exercise the production NPC-to-merc handoff when tactical visibility is stale.
#include "Dialogue Control.h"
#include "NPC.h"
#include "Overhead.h"
#include "Soldier Profile.h"
#include "opplist.h"

#include <cstdlib>
#include <iostream>

void TriggerClosestMercWhoCanSeeNPC(UINT8 ubNPC, NPCQuoteInfo* pQuotePtr);

namespace {
unsigned checks = 0;

void expect(bool value, const char* reason)
{
	++checks;
	if (!value)
	{
		std::cerr << "FAIL: " << reason << '\n';
		std::abort();
	}
}

void initializeSoldier(SOLDIERTYPE& soldier, UINT16 id, UINT8 profile, UINT8 team, INT32 grid)
{
	soldier.initialize();
	soldier.ubID = SoldierID(id);
	soldier.ubProfile = profile;
	soldier.bActive = TRUE;
	soldier.bInSector = TRUE;
	soldier.bTeam = team;
	soldier.bAssignment = 0;
	soldier.stats.bLife = 100;
	soldier.sGridNo = grid;
}

void resetDialogue()
{
	EmptyDialogueQueue();
	giNPCReferenceCount = 0;
}
}

int main()
{
	for (UINT16 id = 0; id < MAX_NUM_SOLDIERS; ++id)
	{
		MercPtrs[id] = &Menptr[id];
		Menptr[id].bActive = FALSE;
	}
	gbPlayerNum = 0;
	gTacticalStatus.Team[gbPlayerNum].bFirstID = SoldierID(0);
	gTacticalStatus.Team[gbPlayerNum].bLastID = SoldierID(1);

	SOLDIERTYPE first;
	SOLDIERTYPE second;
	SOLDIERTYPE miguel;
	initializeSoldier(first, 0, 0, gbPlayerNum, 1000);
	initializeSoldier(second, 1, 1, gbPlayerNum, 1010);
	initializeSoldier(miguel, 100, MIGUEL, CIV_TEAM, 1002);
	MercPtrs[0] = &first;
	MercPtrs[1] = &second;
	MercPtrs[100] = &miguel;

	NPCQuoteInfo quote{};
	quote.ubTriggerNPCRec = QUOTE_RESPONSE_TO_MIGUEL_SLASH_QUOTE_MERC_OR_RPC_LETGO;

	first.aiData.bOppList[miguel.ubID] = NOT_HEARD_OR_SEEN;
	second.aiData.bOppList[miguel.ubID] = NOT_HEARD_OR_SEEN;
	resetDialogue();
	TriggerClosestMercWhoCanSeeNPC(MIGUEL, &quote);
	expect(!DialogueQueueIsEmpty(),
	       "stale sector-entry visibility still queues the NPC-to-merc handoff");
	expect(giNPCReferenceCount == 1,
	       "the fallback queues Miguel's continuation-bearing merc response");

	first.flags.uiStatusFlags |= SOLDIER_GASSED;
	first.aiData.bOppList[miguel.ubID] = SEEN_CURRENTLY;
	resetDialogue();
	TriggerClosestMercWhoCanSeeNPC(MIGUEL, &quote);
	expect(!DialogueQueueIsEmpty(),
	       "an ineligible visible merc does not hide an eligible fallback merc");
	expect(giNPCReferenceCount == 1,
	       "the eligible fallback response retains Miguel's continuation event");

	resetDialogue();
	TriggerClosestMercWhoCanSeeNPC(MIGUEL, nullptr);
	expect(DialogueQueueIsEmpty() && giNPCReferenceCount == 0,
	       "a missing quote record leaves the dialogue queue unchanged");

	resetDialogue();
	TriggerClosestMercWhoCanSeeNPC(200, &quote);
	expect(DialogueQueueIsEmpty() && giNPCReferenceCount == 0,
	       "a missing NPC leaves the dialogue queue unchanged");

	resetDialogue();
	std::cout << checks << " NPC dialogue-chain assertions passed\n";
	return 0;
}
