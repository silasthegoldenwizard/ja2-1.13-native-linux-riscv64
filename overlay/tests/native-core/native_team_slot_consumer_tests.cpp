/*
 * P710 native snapshot update: tests/native-core/native_team_slot_consumer_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_team_slot_consumer_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted vehicle roster checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_team_slot_consumer_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted ammo crate roster checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_team_slot_consumer_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted laptop roster checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Exercise periodic and strategic team consumers with genuinely empty slots.
#include "Overhead.h"
#include "Assignments.h"
#include "Drugs And Alcohol.h"
#include "Militia Control.h"
#include "Map Screen Interface.h"
#include "Merc Hiring.h"
#include "Morale.h"
#include "personnel.h"
#include "PATHAI.H"
#include "Queen Command.h"
#include "Strategic Merc Handler.h"
#include "Campaign Types.h"
#include "Campaign.h"
#include "Game Clock.h"
#include "Items.h"
#include "SaveLoadMap.h"
#include "Soldier Add.h"
#include "TileDat.h"
#include "Vehicles.h"
#include "Weapons.h"
#include "World Items.h"
#include "strategic.h"
#include "strategicmap.h"
#include "worlddef.h"
#include "worldman.h"

#include <cstring>
#include <iostream>

extern INT8 CountMilitiaTrainersInSoldiersSector(SOLDIERTYPE* pSoldier, UINT8 ubMilitiaType);
extern void HandleNaturalHealing();
extern void HandleDoctorsInSector(INT16 sX, INT16 sY, INT8 bZ);
extern void HandleDoctorMilitia();
extern void UpdatePatientsWhoAreDoneHealing();
extern UINT8 GetNumberThatCanBeDoctored(SOLDIERTYPE* pDoctor, BOOLEAN fThisHour,
	BOOLEAN fSkipKitCheck, BOOLEAN fSkipSkillCheck, BOOLEAN fCheckForSurgery);
extern void HandleRepairmenInSector(INT16 sX, INT16 sY, INT8 bZ);
extern SOLDIERTYPE* GetRobotSoldier();
extern void HandleDiseaseDiagnosis();
extern void HandleStrategicDiseaseAndBurial();
extern void HandleTrainingInSector(INT16 sMapX, INT16 sMapY, INT8 bZ);
extern void HandleRadioScanInSector(INT16 sMapX, INT16 sMapY, INT8 bZ);
extern void HandleSpreadingPropagandaInSector(INT16 sMapX, INT16 sMapY, INT8 bZ);
extern void HandlePrisonerProcessingInSector(INT16 sMapX, INT16 sMapY, INT8 bZ);
extern BOOLEAN IsAnythingAroundForSoldierToClean(SOLDIERTYPE* pSoldier);
extern void HandleAnyStatChangesAfterAttack();
extern void HourlyGatheringInformation();
extern void LoadCharacters();
extern BOOLEAN CheckForLosingEndOfBattle();
extern UINT8 FindBestMilitiaTrainingLeadershipInSector(
	INT16 sMapX, INT16 sMapY, INT8 bMapZ, UINT8 ubMilitiaType);
extern INT32 GetNumberOfMercsOnPlayersTeam();
extern INT32 GetTotalDailyCostOfCurrentTeam();
extern INT32 GetLowestDailyCostOfCurrentTeam();
extern INT32 GetHighestDailyCostOfCurrentTeam();
extern SoldierID GetIdOfMercWithHighestStat(INT32 iStat);
extern SoldierID GetIdOfMercWithLowestStat(INT32 iStat);
extern INT32 GetAvgStatOfCurrentTeamStat(INT32 iStat);
extern void ObliterateSector();
extern void HandleFortification();
extern INT16 MakeAutomaticSurgeryOnAllPatients(SOLDIERTYPE* pDoctor);
extern void HandleTacticalAmmoCrates(UINT8 magType);
extern BOOLEAN CreateLevelNode(LEVELNODE**);
extern UINT32 guiLevelNodes;

namespace {
unsigned checks = 0;
const char* failure = nullptr;
unsigned trainingAwardCalls = 0;
SOLDIERTYPE* lastTrainingStudent = nullptr;

void check(bool value, const char* message)
{
	++checks;
	if (!value && failure == nullptr)
	{
		failure = message;
	}
}

struct TeamSlotState
{
	UINT8 team;
	SoldierID first;
	SoldierID last;
	SoldierID slot;
	SOLDIERTYPE* owner;

	TeamSlotState(UINT8 teamValue, UINT16 slotValue)
		: team(teamValue),
		  first(gTacticalStatus.Team[teamValue].bFirstID),
		  last(gTacticalStatus.Team[teamValue].bLastID),
		  slot(slotValue),
		  owner(MercPtrs[slotValue])
	{
		gTacticalStatus.Team[team].bFirstID = slot;
		gTacticalStatus.Team[team].bLastID = slot;
		MercPtrs[slotValue] = nullptr;
	}

	~TeamSlotState()
	{
		MercPtrs[slot.i] = owner;
		gTacticalStatus.Team[team].bFirstID = first;
		gTacticalStatus.Team[team].bLastID = last;
	}
};

struct PeacefulSectorState
{
	INT16 x;
	INT16 y;
	BOOLEAN enemyControlled;
	INT8 townId;
	SECTORINFO sector;
	BOOLEAN enemyInSector;
	UINT32 tacticalFlags;

	PeacefulSectorState(INT16 xValue, INT16 yValue)
		: x(xValue),
		  y(yValue),
		  enemyControlled(StrategicMap[CALCULATE_STRATEGIC_INDEX(xValue, yValue)].fEnemyControlled),
		  townId(StrategicMap[CALCULATE_STRATEGIC_INDEX(xValue, yValue)].bNameId),
		  sector(SectorInfo[SECTOR(xValue, yValue)]),
		  enemyInSector(gTacticalStatus.fEnemyInSector),
		  tacticalFlags(gTacticalStatus.uiFlags)
	{
		StrategicMap[CALCULATE_STRATEGIC_INDEX(x, y)].fEnemyControlled = FALSE;
		StrategicMap[CALCULATE_STRATEGIC_INDEX(x, y)].bNameId = 1;
		SectorInfo[SECTOR(x, y)].ubNumAdmins = 0;
		SectorInfo[SECTOR(x, y)].ubNumTroops = 0;
		SectorInfo[SECTOR(x, y)].ubNumElites = 0;
		SectorInfo[SECTOR(x, y)].ubNumCreatures = 0;
		SectorInfo[SECTOR(x, y)].ubNumRobots = 0;
		SectorInfo[SECTOR(x, y)].ubNumJeeps = 0;
		SectorInfo[SECTOR(x, y)].ubNumTanks = 0;
		gTacticalStatus.fEnemyInSector = FALSE;
		gTacticalStatus.uiFlags &= ~INCOMBAT;
	}

	~PeacefulSectorState()
	{
		StrategicMap[CALCULATE_STRATEGIC_INDEX(x, y)].fEnemyControlled = enemyControlled;
		StrategicMap[CALCULATE_STRATEGIC_INDEX(x, y)].bNameId = townId;
		SectorInfo[SECTOR(x, y)] = sector;
		gTacticalStatus.fEnemyInSector = enemyInSector;
		gTacticalStatus.uiFlags = tacticalFlags;
	}
};

void runHourlyDrugScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	HourlyDrugUpdate();
	check(true, "hourly drug updates ignore empty player slots");
}

void runRemoveMilitiaScenario()
{
	TeamSlotState empty(MILITIA_TEAM, 40);
	RemoveMilitiaFromTactical();
	check(true, "militia removal ignores empty militia slots");
}

void runNumFreeSlotsScenario()
{
	TeamSlotState empty(ENEMY_TEAM, 20);
	check(NumFreeSlots(ENEMY_TEAM) == 0,
	      "missing enemy storage is not advertised as a usable tactical slot");
	SOLDIERTYPE storage;
	storage.initialize();
	storage.bActive = FALSE;
	MercPtrs[20] = &storage;
	check(NumFreeSlots(ENEMY_TEAM) == 1,
	      "an existing inactive enemy record counts as a free tactical slot");
	storage.bActive = TRUE;
	check(NumFreeSlots(ENEMY_TEAM) == 0,
	      "an active enemy record does not count as a free tactical slot");
}

void runMilitiaTrainerScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	SOLDIERTYPE subject;
	subject.initialize();
	subject.bActive = TRUE;
	subject.stats.bLife = 100;
	subject.sSectorX = 9;
	subject.sSectorY = 1;
	subject.bSectorZ = 0;
	check(CountMilitiaTrainersInSoldiersSector(&subject, 0) == 0,
	      "an empty player slot is not counted as a militia trainer");
}

void runBestMilitiaLeadershipScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	check(FindBestMilitiaTrainingLeadershipInSector(9, 1, 0, TOWN_MILITIA) == 0,
	      "best militia leadership ignores an empty player slot");
}

void runHourlyInformationScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	HourlyGatheringInformation();
	check(true, "hourly information gathering ignores an empty player slot");
}

void runLoadCharactersScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	const UINT8 previousPlayer = gbPlayerNum;
	gbPlayerNum = OUR_TEAM;
	LoadCharacters();
	gbPlayerNum = previousPlayer;
	check(true, "map character loading ignores an empty player slot");
}

void runBattleLossCheckScenario()
{
	TeamSlotState emptyMilitia(MILITIA_TEAM, 40);
	TeamSlotState emptyPlayer(OUR_TEAM, 0);
	const UINT8 previousPlayer = gbPlayerNum;
	gbPlayerNum = OUR_TEAM;
	check(CheckForLosingEndOfBattle() == TRUE,
	      "an empty player and militia roster has no capable combatant");
	gbPlayerNum = previousPlayer;
}

void runGlobalMoraleScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	const INT8 savedPlayerTeam = gbPlayerNum;
	gbPlayerNum = OUR_TEAM;
	HandleMoraleEvent(nullptr, MORALE_TOWN_LIBERATED, 9, 1, 0);
	gbPlayerNum = savedPlayerTeam;
	check(true, "global morale events ignore empty player slots");
}

void runSpecificMoraleScenario()
{
	HandleMoraleEvent(nullptr, MORALE_CLAUSTROPHOBE_UNDERGROUND, 9, 1, 0);
	check(true, "soldier-specific morale events reject a missing owner");
}

void runMercDailyScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	const UINT32 savedDay = guiDay;
	guiDay = 2;
	MercDailyUpdate();
	guiDay = savedDay;
	check(true, "daily merc updates ignore empty player slots");
}

void runIsMercOnTeamScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	check(IsMercOnTeam(0, FALSE, FALSE) == FALSE,
	      "an empty player slot does not contain a merc profile");
}

void runSoldierIDFromMercScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	check(GetSoldierIDFromMercID(0) == NOBODY,
	      "an empty player slot does not resolve to a soldier ID");
}

void runNaturalHealingScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	const UINT8 previousPlayer = gbPlayerNum;
	gbPlayerNum = OUR_TEAM;
	HandleNaturalHealing();
	gbPlayerNum = previousPlayer;
	check(true, "natural healing ignores an empty player-team slot");
}

void runAssignmentCountScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	check(FindNumberInSectorWithAssignment(9, 1, DOCTOR) == 0,
	      "assignment counts ignore an empty player-team slot");
}

void runDoctorsInSectorScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	HandleDoctorsInSector(9, 1, 0);
	check(true, "sector doctor updates ignore an empty player-team slot");
}

void runDoctorMilitiaScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	const UINT8 previousPlayer = gbPlayerNum;
	gbPlayerNum = OUR_TEAM;
	HandleDoctorMilitia();
	gbPlayerNum = previousPlayer;
	check(true, "militia doctor updates ignore an empty player-team slot");
}

void runPatientsDoneHealingScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	UpdatePatientsWhoAreDoneHealing();
	check(true, "patient completion updates ignore an empty player-team slot");
}

void runDoctorableCountScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	SOLDIERTYPE doctor;
	doctor.initialize();
	check(GetNumberThatCanBeDoctored(&doctor, HEALABLE_EVER, FALSE, FALSE, FALSE) == 0,
	      "doctorable-patient counts ignore an empty player-team slot");
}

void runDoctorLookupScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	SOLDIERTYPE patient;
	patient.initialize();
	check(AnyDoctorWhoCanHealThisPatient(&patient, HEALABLE_EVER) == nullptr,
	      "doctor lookup ignores an empty player-team slot");
}

void runRepairmenInSectorScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	HandleRepairmenInSector(9, 1, 0);
	check(true, "sector repair updates ignore an empty player-team slot");
}

void runRobotLookupScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	check(GetRobotSoldier() == nullptr,
	      "robot lookup ignores an empty player-team slot");
}

void runPropagandaBlockScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	check(HandlePropagandaBlockingBadNewsInTown(1, 100) == 100,
	      "propaganda blocking ignores an empty player-team slot");
}

void runDiseaseDiagnosisScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	const BOOLEAN previousDisease = gGameExternalOptions.fDisease;
	gGameExternalOptions.fDisease = TRUE;
	HandleDiseaseDiagnosis();
	gGameExternalOptions.fDisease = previousDisease;
	check(true, "disease diagnosis ignores an empty player-team slot");
}

void runTrainingPointsScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	SOLDIERTYPE student;
	student.initialize();
	student.stats.bWisdom = 50;
	student.stats.bMarksmanship = 50;
	student.bBreath = 100;
	student.bBreathMax = 100;
	student.bSectorZ = 1;

	const INT32 previousMinimum = gGameExternalOptions.ubTrainingSkillMin;
	const INT32 previousMaximum = gGameExternalOptions.ubTrainingSkillMax;
	const INT32 previousDivisor = gGameExternalOptions.ubSelfTrainingDivisor;
	gGameExternalOptions.ubTrainingSkillMin = 0;
	gGameExternalOptions.ubTrainingSkillMax = 100;
	gGameExternalOptions.ubSelfTrainingDivisor = 1000;
	UINT16 maximumPoints = 0;
	GetSoldierStudentPts(&student, MARKSMANSHIP, &maximumPoints);
	gGameExternalOptions.ubTrainingSkillMin = previousMinimum;
	gGameExternalOptions.ubTrainingSkillMax = previousMaximum;
	gGameExternalOptions.ubSelfTrainingDivisor = previousDivisor;
	check(true, "training-point lookup ignores an empty player-team slot");
}

void runSectorListScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	InitSectorsWithSoldiersList();
	BuildSectorsWithSoldiersList();
	check(true, "sector-presence rebuild ignores an empty player-team slot");
}

void runSamOperatorScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	SoldierID operatorID = NOBODY;
	GetBestSAMOperatorCTH_Player(9, 1, 0, &operatorID);
	check(operatorID == NOBODY,
	      "SAM operator lookup ignores an empty player-team slot");
}

void runFortificationScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	HandleFortification();
	check(true, "fortification updates ignore an empty player-team slot");
}

void runAutomaticSurgeryScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	SOLDIERTYPE doctor;
	doctor.initialize();
	doctor.bTeam = OUR_TEAM;
	check(MakeAutomaticSurgeryOnAllPatients(&doctor) == 0,
	      "automatic surgery ignores an empty player-team slot");
}

void runMilitiaTrainingRecordScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	RecordNumMilitiaTrainedForMercs(9, 1, 0, 1);
	check(true, "militia-training records ignore an empty player-team slot");
}

void runTrainingInSectorScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	PeacefulSectorState peaceful(9, 1);
	const UINT8 previousPlayer = gbPlayerNum;
	gbPlayerNum = OUR_TEAM;
	HandleTrainingInSector(9, 1, 0);
	gbPlayerNum = previousPlayer;
	check(true, "hourly training ignores an empty player-team slot");
}

void runSmartTrainingRestScenario()
{
	const SoldierID savedFirst = gTacticalStatus.Team[OUR_TEAM].bFirstID;
	const SoldierID savedLast = gTacticalStatus.Team[OUR_TEAM].bLastID;
	SOLDIERTYPE* savedZero = MercPtrs[0];
	SOLDIERTYPE* savedOne = MercPtrs[1];
	const UINT8 savedPlayer = gbPlayerNum;
	const UINT8 savedRest = gGameExternalOptions.ubSmartTrainingRest;
	const INT32 savedMinutes = gGameExternalOptions.ubMinutesForAssignmentToCount;
	const INT32 savedMinimum = gGameExternalOptions.ubTrainingSkillMin;
	const INT32 savedMaximum = gGameExternalOptions.ubTrainingSkillMax;
	const INT32 savedDivisor = gGameExternalOptions.ubSelfTrainingDivisor;

	SOLDIERTYPE student;
	student.initialize();
	student.ubID = SoldierID(0);
	student.bTeam = OUR_TEAM;
	student.bActive = TRUE;
	student.sSectorX = 9;
	student.sSectorY = 1;
	student.bSectorZ = 0;
	student.bTrainStat = MARKSMANSHIP;
	student.stats.bWisdom = 50;
	student.stats.bMarksmanship = 50;
	student.bBreath = 100;
	student.bBreathMax = 100;
	student.flags.fMercAsleep = FALSE;
	student.uiLastAssignmentChangeMin = 0;

	gTacticalStatus.Team[OUR_TEAM].bFirstID = SoldierID(0);
	gTacticalStatus.Team[OUR_TEAM].bLastID = SoldierID(1);
	MercPtrs[0] = &student;
	MercPtrs[1] = nullptr;
	gbPlayerNum = OUR_TEAM;
	gGameExternalOptions.ubMinutesForAssignmentToCount = 0;
	gGameExternalOptions.ubTrainingSkillMin = 0;
	gGameExternalOptions.ubTrainingSkillMax = 100;
	gGameExternalOptions.ubSelfTrainingDivisor = 1000;
	PeacefulSectorState peaceful(9, 1);

	for (UINT8 mode : {UINT8(1), UINT8(3)})
	{
		gGameExternalOptions.ubSmartTrainingRest = mode;
		student.bAssignment = TRAIN_SELF;
		trainingAwardCalls = 0;
		lastTrainingStudent = nullptr;
		HandleTrainingInSector(9, 1, 0);
		check(trainingAwardCalls == 1 && lastTrainingStudent == &student,
		      "a solo self-trainer receives points in smart-rest mode 1 or 3");

		student.bAssignment = TRAIN_BY_OTHER;
		trainingAwardCalls = 0;
		lastTrainingStudent = nullptr;
		HandleTrainingInSector(9, 1, 0);
		check(trainingAwardCalls == 0,
		      "an unpaired student still waits in smart-rest mode 1 or 3");
	}

	gGameExternalOptions.ubSmartTrainingRest = savedRest;
	gGameExternalOptions.ubMinutesForAssignmentToCount = savedMinutes;
	gGameExternalOptions.ubTrainingSkillMin = savedMinimum;
	gGameExternalOptions.ubTrainingSkillMax = savedMaximum;
	gGameExternalOptions.ubSelfTrainingDivisor = savedDivisor;
	gbPlayerNum = savedPlayer;
	MercPtrs[0] = savedZero;
	MercPtrs[1] = savedOne;
	gTacticalStatus.Team[OUR_TEAM].bFirstID = savedFirst;
	gTacticalStatus.Team[OUR_TEAM].bLastID = savedLast;
}

void runRadioScanScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	PeacefulSectorState peaceful(9, 1);
	const UINT8 previousPlayer = gbPlayerNum;
	gbPlayerNum = OUR_TEAM;
	HandleRadioScanInSector(9, 1, 0);
	gbPlayerNum = previousPlayer;
	check(true, "radio scanning ignores an empty player-team slot");
}

void runStrategicDiseaseScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	const UINT8 previousPlayer = gbPlayerNum;
	const BOOLEAN previousDisease = gGameExternalOptions.fDisease;
	const BOOLEAN previousStrategicDisease = gGameExternalOptions.fDiseaseStrategic;
	gbPlayerNum = OUR_TEAM;
	gGameExternalOptions.fDisease = TRUE;
	gGameExternalOptions.fDiseaseStrategic = TRUE;
	HandleStrategicDiseaseAndBurial();
	gGameExternalOptions.fDiseaseStrategic = previousStrategicDisease;
	gGameExternalOptions.fDisease = previousDisease;
	gbPlayerNum = previousPlayer;
	check(true, "strategic disease and burial ignores an empty player-team slot");
}

void runSpreadingPropagandaScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	PeacefulSectorState peaceful(9, 1);
	const UINT8 previousPlayer = gbPlayerNum;
	gbPlayerNum = OUR_TEAM;
	HandleSpreadingPropagandaInSector(9, 1, 0);
	gbPlayerNum = previousPlayer;
	check(true, "propaganda spreading ignores an empty player-team slot");
}

void runPrisonerProcessingScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	PeacefulSectorState peaceful(9, 1);
	const UINT8 previousPlayer = gbPlayerNum;
	const BOOLEAN previousPrisoners = gGameExternalOptions.fAllowPrisonerSystem;
	const UINT16 previousFacilityTypes = NUM_FACILITY_TYPES;
	const FACILITYLOCATIONS previousLocation = gFacilityLocations[SECTOR(9, 1)][0];
	const UINT16 previousLimit =
		gFacilityTypes[0].AssignmentData[FAC_INTERROGATE_PRISONERS].usPrisonBaseLimit;
	gbPlayerNum = OUR_TEAM;
	gGameExternalOptions.fAllowPrisonerSystem = TRUE;
	NUM_FACILITY_TYPES = 1;
	gFacilityLocations[SECTOR(9, 1)][0].fFacilityHere = TRUE;
	gFacilityTypes[0].AssignmentData[FAC_INTERROGATE_PRISONERS].usPrisonBaseLimit = 1;
	SectorInfo[SECTOR(9, 1)].uiNumberOfPrisonersOfWar[PRISONER_ADMIN] = 1;
	HandlePrisonerProcessingInSector(9, 1, 0);
	gFacilityTypes[0].AssignmentData[FAC_INTERROGATE_PRISONERS].usPrisonBaseLimit = previousLimit;
	gFacilityLocations[SECTOR(9, 1)][0] = previousLocation;
	NUM_FACILITY_TYPES = previousFacilityTypes;
	gGameExternalOptions.fAllowPrisonerSystem = previousPrisoners;
	gbPlayerNum = previousPlayer;
	check(true, "prisoner processing ignores an empty player-team slot");
}

void runRepairPeerLookupScenario()
{
	const SoldierID previousFirst = gTacticalStatus.Team[OUR_TEAM].bFirstID;
	const SoldierID previousLast = gTacticalStatus.Team[OUR_TEAM].bLastID;
	SOLDIERTYPE* const previousOwner0 = MercPtrs[0];
	SOLDIERTYPE* const previousOwner1 = MercPtrs[1];
	SOLDIERTYPE repairer;
	repairer.initialize();
	repairer.bActive = TRUE;
	repairer.bTeam = OUR_TEAM;
	repairer.stats.bLife = 100;
	repairer.sSectorX = 9;
	repairer.sSectorY = 1;
	repairer.bSectorZ = 0;
	gTacticalStatus.Team[OUR_TEAM].bFirstID = SoldierID(0);
	gTacticalStatus.Team[OUR_TEAM].bLastID = SoldierID(1);
	MercPtrs[0] = &repairer;
	MercPtrs[1] = nullptr;
	check(IsAnythingAroundForSoldierToClean(&repairer) == FALSE,
	      "repair and cleaning lookup ignores an empty peer slot");
	MercPtrs[1] = previousOwner1;
	MercPtrs[0] = previousOwner0;
	gTacticalStatus.Team[OUR_TEAM].bLastID = previousLast;
	gTacticalStatus.Team[OUR_TEAM].bFirstID = previousFirst;
}

void runPostAttackStatsScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	HandleAnyStatChangesAfterAttack();
	check(true, "post-attack stat updates ignore an empty player-team slot");
}

void runPersonnelRosterScenario()
{
	TeamSlotState empty(OUR_TEAM, 0);
	check(GetNumberOfMercsOnPlayersTeam() == 0,
	      "personnel count ignores an empty player-team slot");
	check(GetTotalDailyCostOfCurrentTeam() == 0,
	      "personnel total cost ignores an empty player-team slot");
	check(GetLowestDailyCostOfCurrentTeam() == 0,
	      "personnel lowest cost ignores an empty player-team slot");
	check(GetHighestDailyCostOfCurrentTeam() == 0,
	      "personnel highest cost ignores an empty player-team slot");
	check(GetIdOfMercWithHighestStat(0) == NOBODY,
	      "personnel highest-stat lookup ignores an empty player-team slot");
	check(GetIdOfMercWithLowestStat(0) == NOBODY,
	      "personnel lowest-stat lookup ignores an empty player-team slot");
	check(GetAvgStatOfCurrentTeamStat(0) == 0,
	      "personnel average ignores an empty player-team slot");
}

void runMercTeamCountScenario()
{
	const UINT8 previousPlayer = gbPlayerNum;
	const SoldierID previousFirst = gTacticalStatus.Team[OUR_TEAM].bFirstID;
	const SoldierID previousLast = gTacticalStatus.Team[OUR_TEAM].bLastID;
	SOLDIERTYPE* const previousOwner0 = MercPtrs[0];
	SOLDIERTYPE* const previousOwner1 = MercPtrs[1];

	SOLDIERTYPE soldier;
	soldier.initialize();
	soldier.ubID = SoldierID(1);
	soldier.bTeam = OUR_TEAM;
	soldier.bActive = TRUE;
	soldier.stats.bLife = 100;

	gbPlayerNum = OUR_TEAM;
	gTacticalStatus.Team[OUR_TEAM].bFirstID = SoldierID(0);
	gTacticalStatus.Team[OUR_TEAM].bLastID = SoldierID(1);
	MercPtrs[0] = nullptr;
	MercPtrs[1] = &soldier;
	check(NumberOfMercsOnPlayerTeam() == 1,
	      "team count finds a live mercenary after an empty first slot");

	MercPtrs[0] = &soldier;
	MercPtrs[1] = nullptr;
	check(NumberOfMercsOnPlayerTeam() == 1,
	      "team count skips an empty later slot");

	MercPtrs[1] = previousOwner1;
	MercPtrs[0] = previousOwner0;
	gTacticalStatus.Team[OUR_TEAM].bLastID = previousLast;
	gTacticalStatus.Team[OUR_TEAM].bFirstID = previousFirst;
	gbPlayerNum = previousPlayer;
}

void runPersonnelEntryRosterScenario()
{
	const SoldierID previousFirst = gTacticalStatus.Team[OUR_TEAM].bFirstID;
	const SoldierID previousLast = gTacticalStatus.Team[OUR_TEAM].bLastID;
	SOLDIERTYPE* const previousOwner0 = MercPtrs[0];
	SOLDIERTYPE* const previousOwner1 = MercPtrs[1];

	SOLDIERTYPE soldier;
	soldier.initialize();
	soldier.ubID = SoldierID(1);
	soldier.bTeam = OUR_TEAM;
	soldier.bActive = TRUE;
	soldier.stats.bLife = 100;

	gTacticalStatus.Team[OUR_TEAM].bFirstID = SoldierID(0);
	gTacticalStatus.Team[OUR_TEAM].bLastID = SoldierID(1);
	MercPtrs[0] = nullptr;
	MercPtrs[1] = &soldier;
	check(RefreshPersonnelCurrentTeamList() == 1,
	      "personnel entry finds a live mercenary after an empty first slot");

	MercPtrs[1] = nullptr;
	check(RefreshPersonnelCurrentTeamList() == 0,
	      "personnel entry accepts an entirely empty roster");

	MercPtrs[1] = previousOwner1;
	MercPtrs[0] = previousOwner0;
	gTacticalStatus.Team[OUR_TEAM].bLastID = previousLast;
	gTacticalStatus.Team[OUR_TEAM].bFirstID = previousFirst;
}

void runObliterateSectorScenario()
{
	const SoldierID previousLast = gTacticalStatus.Team[OUR_TEAM].bLastID;
	SOLDIERTYPE* const previousOwner = MercPtrs[1];
	gTacticalStatus.Team[OUR_TEAM].bLastID = SoldierID(0);
	MercPtrs[1] = nullptr;
	ObliterateSector();
	MercPtrs[1] = previousOwner;
	gTacticalStatus.Team[OUR_TEAM].bLastID = previousLast;
	check(true, "sector obliteration ignores empty non-player slots");
}

void runVehicleRosterScenario()
{
	VEHICLETYPE vehicle{};
	vehicle.fValid = TRUE;
	vehicle.ubVehicleType = 0;

	VEHICLETYPE* const previousVehicles = pVehicleList;
	const UINT8 previousVehicleCount = ubNumberOfVehicles;
	SOLDIERTYPE* const previousOwner = MercPtrs[0];

	SOLDIERTYPE passenger;
	passenger.initialize();
	passenger.bTeam = OUR_TEAM;
	vehicle.pPassengers[0] = &passenger;

	pVehicleList = &vehicle;
	ubNumberOfVehicles = 1;
	MercPtrs[0] = nullptr;
	check(AddSoldierToVehicle(&passenger, 0) == FALSE,
	      "vehicle admission ignores an empty roster slot while checking capacity");

	MercPtrs[0] = previousOwner;
	ubNumberOfVehicles = previousVehicleCount;
	pVehicleList = previousVehicles;
}

void runAmmoCrateRosterScenario()
{
	constexpr UINT16 looseAmmoItem = 500;
	constexpr UINT16 crateItem = 501;

	const INVTYPE previousLooseItem = Item[looseAmmoItem];
	const INVTYPE previousCrateItem = Item[crateItem];
	const MAGTYPE previousLooseMagazine = Magazine[looseAmmoItem];
	const MAGTYPE previousCrateMagazine = Magazine[crateItem];
	const UINT32 previousMaxItems = gMAXITEMS_READ;
	const BOOLEAN previousEnemyInSector = gTacticalStatus.fEnemyInSector;
	const SoldierID previousFirst = gTacticalStatus.Team[OUR_TEAM].bFirstID;
	const SoldierID previousLast = gTacticalStatus.Team[OUR_TEAM].bLastID;
	const UINT8 previousMaxVehicles =
		gGameExternalOptions.ubGameMaximumNumberOfPlayerVehicles;
	SOLDIERTYPE* const previousOwner0 = MercPtrs[0];
	SOLDIERTYPE* const previousOwner1 = MercPtrs[1];
	MAP_ELEMENT* const previousWorld = gpWorldLevelData;
	const UINT32 previousLevelNodes = guiLevelNodes;
	const UINT32 previousTileCount = giNumberOfTiles;
	const UINT32 previousWorldItemCount = guiNumWorldItems;
	std::vector<WORLDITEM> previousWorldItems;
	previousWorldItems.swap(gWorldItems);

	Item[looseAmmoItem] = {};
	Item[looseAmmoItem].usItemClass = IC_AMMO;
	Item[looseAmmoItem].ubClassIndex = looseAmmoItem;
	Magazine[looseAmmoItem] = {};
	Magazine[looseAmmoItem].ubCalibre = 1;
	Magazine[looseAmmoItem].ubAmmoType = AMMO_REGULAR;
	Magazine[looseAmmoItem].ubMagType = AMMO_MAGAZINE;
	Magazine[looseAmmoItem].ubMagSize = 30;

	Item[crateItem] = {};
	Item[crateItem].usItemClass = IC_AMMO;
	Item[crateItem].ubClassIndex = crateItem;
	Magazine[crateItem] = {};
	Magazine[crateItem].ubCalibre = 1;
	Magazine[crateItem].ubAmmoType = AMMO_REGULAR;
	Magazine[crateItem].ubMagType = AMMO_CRATE;
	Magazine[crateItem].ubMagSize = 100;
	gMAXITEMS_READ = crateItem + 1;

	std::vector<MAP_ELEMENT> world(static_cast<std::size_t>(WORLD_MAX));
	gpWorldLevelData = world.data();
	guiLevelNodes = 0;
	giNumberOfTiles = 0;
	ApplyMapChangesToMapTempFile(FALSE);

	gWorldItems.clear();
	gWorldItems.resize(1);
	WORLDITEM& worldItem = gWorldItems[0];
	worldItem.fExists = TRUE;
	worldItem.sGridNo = 0;
	worldItem.ubLevel = 0;
	worldItem.usFlags = WORLD_ITEM_REACHABLE;
	worldItem.bVisible = TRUE;
	check(CreateAmmo(looseAmmoItem, &worldItem.object, 30),
	      "create loose ammunition for the production crate pass");
	guiNumWorldItems = 1;

	LEVELNODE* node = nullptr;
	check(CreateLevelNode(&node) && node != nullptr,
	      "create the production item-pool level node");
	ITEM_POOL* pool = static_cast<ITEM_POOL*>(MemAlloc(sizeof(ITEM_POOL)));
	check(pool != nullptr, "allocate the production item-pool record");
	*pool = {};
	pool->iItemIndex = 0;
	pool->sGridNo = 0;
	pool->ubLevel = 0;
	pool->bVisible = TRUE;
	pool->pLevelNode = node;
	node->uiFlags = LEVELNODE_ITEM;
	node->pItemPool = pool;
	world[0].pStructHead = node;
	world[0].uiFlags = MAPELEMENT_ITEMPOOL_PRESENT;

	SOLDIERTYPE soldier;
	soldier.initialize();
	soldier.ubID = SoldierID(1);
	soldier.bTeam = OUR_TEAM;
	soldier.bActive = TRUE;
	soldier.bInSector = TRUE;
	check(CreateAmmo(crateItem, &soldier.inv[0], 1),
	      "create the mercenary crate target");

	gTacticalStatus.fEnemyInSector = FALSE;
	gTacticalStatus.Team[OUR_TEAM].bFirstID = SoldierID(0);
	gTacticalStatus.Team[OUR_TEAM].bLastID = SoldierID(2);
	gGameExternalOptions.ubGameMaximumNumberOfPlayerVehicles = 1;
	MercPtrs[0] = nullptr;
	MercPtrs[1] = &soldier;

	const BOOLEAN pathAIInitialized = InitPathAI();
	check(pathAIInitialized == TRUE,
	      "initialize pathfinding for the production post-merge sort pass");
	if (pathAIInitialized)
	{
		HandleTacticalAmmoCrates(AMMO_CRATE);
		ShutDownPathAI();
	}
	check(!gWorldItems[0].fExists,
	      "the loose ammunition is consumed after the sparse roster scan");
	check(world[0].pStructHead == nullptr,
	      "the consumed world-item pool is removed");

	MercPtrs[0] = previousOwner0;
	MercPtrs[1] = previousOwner1;
	gGameExternalOptions.ubGameMaximumNumberOfPlayerVehicles = previousMaxVehicles;
	gTacticalStatus.Team[OUR_TEAM].bFirstID = previousFirst;
	gTacticalStatus.Team[OUR_TEAM].bLastID = previousLast;
	gTacticalStatus.fEnemyInSector = previousEnemyInSector;
	gMAXITEMS_READ = previousMaxItems;
	Item[looseAmmoItem] = previousLooseItem;
	Item[crateItem] = previousCrateItem;
	Magazine[looseAmmoItem] = previousLooseMagazine;
	Magazine[crateItem] = previousCrateMagazine;
	gWorldItems.clear();
	previousWorldItems.swap(gWorldItems);
	guiNumWorldItems = previousWorldItemCount;
	gpWorldLevelData = previousWorld;
	guiLevelNodes = previousLevelNodes;
	giNumberOfTiles = previousTileCount;
}

}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "usage: native_team_slot_consumer_tests MODE\n";
		return 2;
	}

	if (std::strcmp(argv[1], "hourly-drug") == 0)
		runHourlyDrugScenario();
	else if (std::strcmp(argv[1], "remove-militia") == 0)
		runRemoveMilitiaScenario();
	else if (std::strcmp(argv[1], "free-slots") == 0)
		runNumFreeSlotsScenario();
	else if (std::strcmp(argv[1], "militia-trainers") == 0)
		runMilitiaTrainerScenario();
	else if (std::strcmp(argv[1], "best-militia-leadership") == 0)
		runBestMilitiaLeadershipScenario();
	else if (std::strcmp(argv[1], "hourly-information") == 0)
		runHourlyInformationScenario();
	else if (std::strcmp(argv[1], "load-characters") == 0)
		runLoadCharactersScenario();
	else if (std::strcmp(argv[1], "battle-loss-check") == 0)
		runBattleLossCheckScenario();
	else if (std::strcmp(argv[1], "morale-global") == 0)
		runGlobalMoraleScenario();
	else if (std::strcmp(argv[1], "morale-specific") == 0)
		runSpecificMoraleScenario();
	else if (std::strcmp(argv[1], "merc-daily") == 0)
		runMercDailyScenario();
	else if (std::strcmp(argv[1], "merc-on-team") == 0)
		runIsMercOnTeamScenario();
	else if (std::strcmp(argv[1], "merc-id") == 0)
		runSoldierIDFromMercScenario();
	else if (std::strcmp(argv[1], "natural-healing") == 0)
		runNaturalHealingScenario();
	else if (std::strcmp(argv[1], "assignment-count") == 0)
		runAssignmentCountScenario();
	else if (std::strcmp(argv[1], "doctors-in-sector") == 0)
		runDoctorsInSectorScenario();
	else if (std::strcmp(argv[1], "doctor-militia") == 0)
		runDoctorMilitiaScenario();
	else if (std::strcmp(argv[1], "patients-done-healing") == 0)
		runPatientsDoneHealingScenario();
	else if (std::strcmp(argv[1], "doctorable-count") == 0)
		runDoctorableCountScenario();
	else if (std::strcmp(argv[1], "doctor-lookup") == 0)
		runDoctorLookupScenario();
	else if (std::strcmp(argv[1], "repairmen-in-sector") == 0)
		runRepairmenInSectorScenario();
	else if (std::strcmp(argv[1], "robot-lookup") == 0)
		runRobotLookupScenario();
	else if (std::strcmp(argv[1], "propaganda-block") == 0)
		runPropagandaBlockScenario();
	else if (std::strcmp(argv[1], "disease-diagnosis") == 0)
		runDiseaseDiagnosisScenario();
	else if (std::strcmp(argv[1], "training-points") == 0)
		runTrainingPointsScenario();
	else if (std::strcmp(argv[1], "sector-list") == 0)
		runSectorListScenario();
	else if (std::strcmp(argv[1], "sam-operator") == 0)
		runSamOperatorScenario();
	else if (std::strcmp(argv[1], "fortification") == 0)
		runFortificationScenario();
	else if (std::strcmp(argv[1], "automatic-surgery") == 0)
		runAutomaticSurgeryScenario();
	else if (std::strcmp(argv[1], "militia-training-record") == 0)
		runMilitiaTrainingRecordScenario();
	else if (std::strcmp(argv[1], "training-in-sector") == 0)
		runTrainingInSectorScenario();
	else if (std::strcmp(argv[1], "smart-training-rest") == 0)
		runSmartTrainingRestScenario();
	else if (std::strcmp(argv[1], "radio-scan") == 0)
		runRadioScanScenario();
	else if (std::strcmp(argv[1], "strategic-disease") == 0)
		runStrategicDiseaseScenario();
	else if (std::strcmp(argv[1], "spreading-propaganda") == 0)
		runSpreadingPropagandaScenario();
	else if (std::strcmp(argv[1], "prisoner-processing") == 0)
		runPrisonerProcessingScenario();
	else if (std::strcmp(argv[1], "repair-peer-lookup") == 0)
		runRepairPeerLookupScenario();
	else if (std::strcmp(argv[1], "post-attack-stats") == 0)
		runPostAttackStatsScenario();
	else if (std::strcmp(argv[1], "personnel-roster") == 0)
		runPersonnelRosterScenario();
	else if (std::strcmp(argv[1], "merc-team-count") == 0)
		runMercTeamCountScenario();
	else if (std::strcmp(argv[1], "personnel-entry-roster") == 0)
		runPersonnelEntryRosterScenario();
	else if (std::strcmp(argv[1], "obliterate-sector") == 0)
		runObliterateSectorScenario();
	else if (std::strcmp(argv[1], "vehicle-roster") == 0)
		runVehicleRosterScenario();
	else if (std::strcmp(argv[1], "ammo-crate-roster") == 0)
		runAmmoCrateRosterScenario();
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

	std::cout << "PASS: " << checks << " sparse team-consumer assertions\n";
	return 0;
}

extern "C" void __wrap__Z10StatChangeP11SOLDIERTYPEhth(
	SOLDIERTYPE* student, UINT8 stat, UINT16, UINT8 reason)
{
	if (stat == MARKAMT && reason == FROM_TRAINING)
	{
		++trainingAwardCalls;
		lastTrainingStudent = student;
	}
}
