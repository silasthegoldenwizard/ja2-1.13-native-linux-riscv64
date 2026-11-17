/*
 * P710 native snapshot update: tests/native-core/native_multiplayer_rpc_fuzzer.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "multiplayer_rpc_test_support.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace
{
template<typename Packet>
void CopyRecipe( Packet& packet, const std::uint8_t *data, std::size_t size )
{
	if ( size > 2 )
		std::memcpy( &packet, data + 2, std::min( size - 2, sizeof( Packet ) ) );
}

template<typename Packet>
BitSize_t SelectedBitCount( const std::uint8_t *data, std::size_t size )
{
	if ( size < 2 )
		return 0;

	return static_cast<BitSize_t>( data[1] ) % ( sizeof( Packet ) * 8 + 1 );
}

template<typename Packet>
void Dispatch( void (*callback)( RPCParameters * ), Packet& packet, const std::uint8_t *data, std::size_t size )
{
	if ( size > 1 && ( data[1] & 0x40 ) != 0 )
	{
		callback( NULL );
		return;
	}

	RPCParameters parameters{};
	InitializeMultiplayerRPCParameters( parameters, packet, SelectedBitCount<Packet>( data, size ) );
	if ( size > 1 && ( data[1] & 0x80 ) != 0 )
		parameters.input = NULL;
	callback( &parameters );
}
}

extern "C" int LLVMFuzzerTestOneInput( const std::uint8_t *data, std::size_t size )
{
	if ( size == 0 || size > 4096 )
		return 0;

	switch ( data[0] & 3 )
	{
		case 0:
		{
			MultiplayerDetonatePacket packet{};
			CopyRecipe( packet, data, size );
			packet.ubID.i = std::numeric_limits<UINT16>::max();
			Dispatch( recieveDETONATEEXPLOSIVE, packet, data, size );
			break;
		}
		case 1:
		{
			MultiplayerDisarmPacket packet{};
			CopyRecipe( packet, data, size );
			packet.ubID.i = std::numeric_limits<UINT16>::max();
			Dispatch( recieveDISARMEXPLOSIVE, packet, data, size );
			break;
		}
		case 2:
		{
			death_struct packet{};
			CopyRecipe( packet, data, size );
			packet.soldier_id.i = std::numeric_limits<UINT16>::max();
			packet.attacker_id.i = std::numeric_limits<UINT16>::max();
			Dispatch( recieveDEATH, packet, data, size );
			break;
		}
		default:
		{
			death_struct packet{};
			CopyRecipe( packet, data, size );
			packet.soldier_team = ( size > 1 && ( data[1] & 1 ) != 0 ) ? 0 : 6;
			packet.attacker_team = ( size > 1 && ( data[1] & 2 ) != 0 ) ? 0 : 6;
			Dispatch( sendDEATH, packet, data, size );
			break;
		}
	}

	return 0;
}
