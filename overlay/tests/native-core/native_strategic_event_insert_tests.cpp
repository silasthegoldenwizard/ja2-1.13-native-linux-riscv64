/*
 * P710 native snapshot update: tests/native-core/native_strategic_event_insert_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "Game Events.h"
#include "Game Clock.h"
#include "MemMan.h"

#include <cstring>
#include <iostream>

extern void ProcessPendingGameEvents(UINT32 uiAdjustment, UINT8 ubWarpCode);
extern UINT32 guiGameClock;

namespace {
STRATEGICEVENT* insertedEvent = nullptr;
unsigned executeCalls = 0;
}

extern "C" BOOLEAN WrapExecuteStrategicEvent(STRATEGICEVENT* current)
	asm("__wrap__Z21ExecuteStrategicEventP14STRATEGICEVENT");

extern "C" BOOLEAN WrapExecuteStrategicEvent(STRATEGICEVENT* current)
{
	++executeCalls;
	insertedEvent = static_cast<STRATEGICEVENT*>(MemAlloc(sizeof(STRATEGICEVENT)));
	if (insertedEvent == nullptr)
		return FALSE;
	std::memset(insertedEvent, 0, sizeof(*insertedEvent));
	insertedEvent->uiTimeStamp = current->uiTimeStamp + 1;
	insertedEvent->next = gpEventList;
	gpEventList = insertedEvent;
	return TRUE;
}

int main()
{
	gpEventList = static_cast<STRATEGICEVENT*>(MemAlloc(sizeof(STRATEGICEVENT)));
	if (gpEventList == nullptr)
	{
		std::cerr << "FAIL: could not allocate the current event\n";
		return 1;
	}
	std::memset(gpEventList, 0, sizeof(*gpEventList));
	gpEventList->uiTimeStamp = 10;
	gpEventList->ubEventType = ONETIME_EVENT;
	guiGameClock = 10;

	ProcessPendingGameEvents(0, WARPTIME_PROCESS_EVENTS_NORMALLY);

	bool passed = true;
	if (executeCalls != 1)
	{
		std::cerr << "FAIL: expected one event execution\n";
		passed = false;
	}
	if (gpEventList != insertedEvent)
	{
		std::cerr << "FAIL: callback-inserted head was not preserved\n";
		passed = false;
	}
	if (insertedEvent == nullptr || insertedEvent->next != nullptr)
	{
		std::cerr << "FAIL: processed event was not unlinked from the new head\n";
		passed = false;
	}

	if (insertedEvent != nullptr)
		MemFree(insertedEvent);
	insertedEvent = nullptr;
	gpEventList = nullptr;

	if (!passed)
		return 1;
	std::cout << "PASS: callback insertion preserves the strategic-event list\n";
	return 0;
}
