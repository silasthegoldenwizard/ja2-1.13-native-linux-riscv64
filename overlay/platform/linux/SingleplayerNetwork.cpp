/*
 * P710 native snapshot update: platform/linux/SingleplayerNetwork.cpp
 * Native Linux source/build/test adaptations captured in the November 20-21 save-load and crash-report checkpoint.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-21.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "connect.h"
#include "fresh_header.h"

// The original single-player engine shares multiplayer state and send hooks
// throughout gameplay code. Keep that ABI available without pulling RakNet or
// the client/server implementation into the default native build. All state
// remains disconnected, so guarded send hooks are deliberately inert.

bool isOwnTeamWipedOut = false;
bool is_connected = false;
bool is_connecting = false;
bool is_client = false;
bool is_server = false;
bool is_networked = false;
bool is_host = false;
bool recieved_settings = false;
bool recieved_transfer_settings = false;
bool is_game_started = false;
bool auto_retry = false;
bool getReal = false;
bool allowlaptop = false;
bool goahead = false;
bool status = false;
int numready = 0;

BOOLEAN fClientReceivedAllFiles = FALSE;

INT16 serverSyncClientsDirectory = 0;
int CLIENT_NUM = 0;
int giNumTries = 0;
int numreadyteams = 0;
int readyteamreg[10] = {};

UINT8 gEnemyEnabled = 0;
UINT8 gCreatureEnabled = 0;
UINT8 gMilitiaEnabled = 0;
UINT8 gCivEnabled = 0;
UINT8 cMaxClients = 0;
UINT8 cSameMercAllowed = 0;
UINT8 cGameType = 0;
UINT8 netbTeam = 0;
UINT16 ubID_prefix = 0;
FLOAT cDamageMultiplier = 1.0F;
UINT8 gRandomStartingEdge = 0;
UINT8 gRandomMercs = 0;
UINT8 gMaxEnemiesEnabled = 0;
UINT8 cAllowMercEquipment = 0;
UINT8 cMaxMercs = 0;
UINT8 cWeaponReadyBonus = 0;
UINT8 cDisableSpectatorMode = 0;
UINT8 cDisableMorale = 0;

STRING512 gCurrentTransferFilename = {};
INT32 gCurrentTransferBytes = 0;
INT32 gTotalTransferBytes = 0;
UINT32 crate_usMapPos = 0;

char cClientName[30] = {};
char cServerName[30] = {};
char client_names[4][30] = {};
int client_ready[4] = {};
int client_teams[4] = {};
int client_edges[5] = {};
int client_downloading[4] = {};
int client_progress[4] = {};

CHAR16 TeamNameStrings[][30] = {
	L"You", L"AI", L"Creature", L"Militia", L"Civilian", L"Player_Plan",
	L"Client #1", L"Client #2", L"Client #3", L"Client #4"
};

player_stats gMPPlayerStats[5] = {};

void start_battle() {}
void DropOffItemsInSector(UINT8) {}
void mp_help() {}
void mp_help2() {}
void grid_display() {}
void send_loaded() {}
void send_donegui(UINT8) {}
UINT8 numenemyLAN(UINT8, UINT8) { return 0; }
void connect_client() {}
void start_server() {}
void client_packet() {}
void server_packet() {}
void NetworkAutoStart() {}

void server_disconnect()
{
	is_server = false;
	is_networked = false;
	is_connected = false;
}

void client_disconnect()
{
	is_client = false;
	is_networked = false;
	is_connected = false;
}

void send_path(SOLDIERTYPE*, INT32, UINT16, BOOLEAN, BOOLEAN) {}
void send_stance(SOLDIERTYPE*, UINT8) {}
void send_dir(SOLDIERTYPE*, UINT16) {}
void send_fire(SOLDIERTYPE*, INT32) {}
void send_hit(EV_S_WEAPONHIT*) {}
void send_bullet(BULLET*, UINT16) {}
void send_hire(SoldierID, UINT8, INT16, BOOLEAN) {}
void send_dismiss(UINT16) {}
void send_gui_pos(SOLDIERTYPE*, FLOAT, FLOAT) {}
void send_gui_dir(SOLDIERTYPE*, UINT16) {}
void send_EndTurn(UINT8) {}
void send_AI(SOLDIERCREATE_STRUCT*) {}
void send_stop(EV_S_STOP_MERC*) {}
void send_interrupt(SOLDIERTYPE*) {}
void send_grenade(OBJECTTYPE*, float, float, float, float, float, float, float,
	UINT32, SoldierID, UINT8, UINT32, INT32, bool) {}
void send_grenade_result(float, float, float, INT32, SoldierID, INT32, bool) {}
void send_plant_explosive(SoldierID, UINT16, UINT8, UINT16, UINT32, UINT8, UINT32) {}
void send_detonate_explosive(UINT32, SoldierID) {}
void send_spreadeffect(INT32, UINT8, UINT16, SoldierID, BOOLEAN, INT8, INT32) {}
void send_newsmokeeffect(INT32, UINT16, INT8, SoldierID, INT32) {}
void send_gasdamage(SOLDIERTYPE*, UINT16, INT16, BOOLEAN, INT16, INT16, SoldierID) {}
void send_explosivedamage(SoldierID, SoldierID, INT32, INT16, INT16, UINT32, UINT16, INT16) {}
void send_disarm_explosive(UINT32, UINT32, SoldierID) {}
void OpenChatMsgBox() {}
void reapplySETTINGS() {}
void send_settings() {}
void send_mapchange() {}
void send_teamchange(int) {}
void send_edgechange(int) {}
bool can_teamchange() { return false; }
bool can_edgechange() { return false; }
void game_over() {}

void send_door(SOLDIERTYPE*, INT32, BOOLEAN) {}
void send_changestate(EV_S_CHANGESTATE*) {}
void send_death(SOLDIERTYPE*) {}
void send_hitstruct(EV_S_STRUCTUREHIT*) {}
void send_hitwindow(EV_S_WINDOWHIT*) {}
void send_miss(EV_S_MISS*) {}
void UpdateSoldierToNetwork(SOLDIERTYPE*) {}
void kick_player() {}
void overide_turn() {}
void overide_callback(UINT8) {}
void send_fireweapon(EV_S_FIREWEAPON*) {}
void end_interrupt(BOOLEAN) {}
void sendRT() {}
void startCombat(UINT8) {}
void intAI(SOLDIERTYPE*) {}
void teamwiped() {}
BOOLEAN check_status() { return FALSE; }
void send_heal(SOLDIERTYPE*) {}
