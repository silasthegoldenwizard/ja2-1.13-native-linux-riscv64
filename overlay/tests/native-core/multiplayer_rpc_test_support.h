/*
 * P710 native snapshot update: tests/native-core/multiplayer_rpc_test_support.h
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#pragma once

#include "fresh_header.h"
#include "raknet/RakNetTypes.h"

struct MultiplayerDetonatePacket
{
	SoldierID ubID;
	UINT32 uiWorldItemIndex;
	UINT8 ubMPTeamIndex;
	UINT32 uiPreRandomIndex;
};

struct MultiplayerDisarmPacket
{
	UINT32 uiWorldItemIndex;
	UINT8 ubMPTeamIndex;
	SoldierID ubID;
	UINT32 sGridNo;
	UINT32 uiPreRandomIndex;
};

void recieveDETONATEEXPLOSIVE( RPCParameters *rpcParameters );
void recieveDISARMEXPLOSIVE( RPCParameters *rpcParameters );
void recieveDEATH( RPCParameters *rpcParameters );
void sendDEATH( RPCParameters *rpcParameters );

template<typename Packet>
void InitializeMultiplayerRPCParameters( RPCParameters& parameters, Packet& packet, BitSize_t bitCount = sizeof( Packet ) * 8 )
{
	parameters.input = reinterpret_cast<unsigned char *>( &packet );
	parameters.numberOfBitsOfData = bitCount;
}
