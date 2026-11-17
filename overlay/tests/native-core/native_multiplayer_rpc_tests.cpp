/*
 * P710 native snapshot update: tests/native-core/native_multiplayer_rpc_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "multiplayer_rpc_test_support.h"

#include <cstdint>
#include <iostream>
#include <limits>

namespace
{
template<typename Packet>
void ExerciseMissingAndTruncatedPackets( void (*callback)( RPCParameters * ) )
{
	callback( NULL );

	RPCParameters parameters{};
	callback( &parameters );

	Packet packet{};
	parameters.input = reinterpret_cast<unsigned char *>( &packet );
	for ( BitSize_t bits = 0; bits < sizeof( Packet ) * 8; ++bits )
	{
		parameters.numberOfBitsOfData = bits;
		callback( &parameters );
	}
}
}

int main()
{
	ExerciseMissingAndTruncatedPackets<MultiplayerDetonatePacket>( recieveDETONATEEXPLOSIVE );
	ExerciseMissingAndTruncatedPackets<MultiplayerDisarmPacket>( recieveDISARMEXPLOSIVE );
	ExerciseMissingAndTruncatedPackets<death_struct>( recieveDEATH );
	ExerciseMissingAndTruncatedPackets<death_struct>( sendDEATH );

	MultiplayerDetonatePacket detonate{};
	detonate.ubID.i = std::numeric_limits<UINT16>::max();
	RPCParameters detonateParameters{};
	InitializeMultiplayerRPCParameters( detonateParameters, detonate );
	recieveDETONATEEXPLOSIVE( &detonateParameters );

	MultiplayerDisarmPacket disarm{};
	disarm.ubID.i = std::numeric_limits<UINT16>::max();
	RPCParameters disarmParameters{};
	InitializeMultiplayerRPCParameters( disarmParameters, disarm );
	recieveDISARMEXPLOSIVE( &disarmParameters );

	death_struct death{};
	death.soldier_id.i = std::numeric_limits<UINT16>::max();
	death.attacker_id.i = std::numeric_limits<UINT16>::max();
	RPCParameters deathParameters{};
	InitializeMultiplayerRPCParameters( deathParameters, death );
	recieveDEATH( &deathParameters );

	death.soldier_team = 0;
	death.attacker_team = 1;
	sendDEATH( &deathParameters );
	death.soldier_team = 1;
	death.attacker_team = 6;
	sendDEATH( &deathParameters );

	std::cout << "PASS: multiplayer RPC payload and soldier-ID boundaries\n";
	return 0;
}
