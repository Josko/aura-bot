/*

   Copyright [2008] [Trevor Hogan]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   CODE PORTED FROM THE ORIGINAL GHOST PROJECT: http://ghost.pwner.org/

*/

#include "ghost.h"
#include "util.h"
#include "config.h"
#include "language.h"
#include "socket.h"
#include "ghostdb.h"
#include "bnet.h"
#include "map.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "game_base.h"

#include <string.h>
#include <time.h>

//
// CBaseGame
//

CBaseGame :: CBaseGame( CGHost *nGHost, CMap *nMap, uint16_t nHostPort, unsigned char nGameState, string &nGameName, string &nOwnerName, string &nCreatorName, string &nCreatorServer ) 


: m_GHost( nGHost ), m_Slots( nMap->GetSlots( ) ), m_Exiting( false ), m_Saving( false ), m_HostPort( nHostPort ), m_GameState( nGameState ), m_VirtualHostPID( 255 ), m_GProxyEmptyActions( 0 ), m_GameName( nGameName ), m_LastGameName( nGameName ), m_VirtualHostName( nGHost->m_VirtualHostName ), m_OwnerName( nOwnerName ), m_CreatorName( nCreatorName ), m_CreatorServer( nCreatorServer ), m_HCLCommandString( nMap->GetMapDefaultHCL( ) ), m_RandomSeed( GetTicks( ) ), m_HostCounter( nGHost->m_HostCounter++ ), m_Latency( nGHost->m_Latency ), m_SyncLimit( nGHost->m_SyncLimit ), m_SyncCounter( 0 ), m_GameTicks( 0 ), m_CreationTime( GetTime( ) ), m_LastPingTime( GetTime( ) ), m_LastRefreshTime( GetTime( ) ), m_LastDownloadTicks( GetTime( ) ), m_DownloadCounter( 0 ), m_LastDownloadCounterResetTicks( GetTicks( ) ), m_LastCountDownTicks( 0 ), m_CountDownCounter( 0 ), m_StartedLoadingTicks( 0 ), m_StartPlayers( 0 ), m_LastLagScreenResetTime( 0 ), m_LastActionSentTicks( 0 ), m_LastActionLateBy( 0 ), m_StartedLaggingTime( 0 ), m_LastLagScreenTime( 0 ),  m_LastReservedSeen( GetTime( ) ), m_StartedKickVoteTime( 0 ), m_GameOverTime( 0 ), m_LastPlayerLeaveTicks( 0 ), m_SlotInfoChanged( false ), m_Locked( false ), m_RefreshError( false ), m_RefreshRehosted( false ), m_MuteAll( false ), m_MuteLobby( false ), m_CountDownStarted( false ), m_GameLoading( false ), m_GameLoaded( false ), m_LoadInGame( nMap->GetMapLoadInGame( ) ), m_Lagging( false )
{
	m_Socket = new CTCPServer( );
	m_Protocol = new CGameProtocol( m_GHost );
	m_Map = new CMap( *nMap );

	// wait time of 1 minute  = 0 empty actions required
	// wait time of 2 minutes = 1 empty action required
	// etc...

	if( m_GHost->m_ReconnectWaitTime )
	{
		m_GProxyEmptyActions = m_GHost->m_ReconnectWaitTime - 1;

		// clamp to 9 empty actions (10 minutes)

		if( m_GProxyEmptyActions > 9 )
			m_GProxyEmptyActions = 9;
	}

	// start listening for connections

	if( !m_GHost->m_BindAddress.empty( ) )
		CONSOLE_Print( "[GAME: " + m_GameName + "] attempting to bind to address [" + m_GHost->m_BindAddress + "]" );

	if( m_Socket->Listen( m_GHost->m_BindAddress, m_HostPort ) )
		CONSOLE_Print( "[GAME: " + m_GameName + "] listening on port " + UTIL_ToString( m_HostPort ) );
	else
	{
		CONSOLE_Print( "[GAME: " + m_GameName + "] error listening on port " + UTIL_ToString( m_HostPort ) );
		m_Exiting = true;
	}
}

CBaseGame :: ~CBaseGame( )
{
	delete m_Socket;
	delete m_Protocol;
	delete m_Map;

	for( vector<CPotentialPlayer *> :: iterator i = m_Potentials.begin( ); i != m_Potentials.end( ); ++i )
		delete *i;

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		delete *i;

	while( !m_Actions.empty( ) )
	{
		delete m_Actions.front( );
		m_Actions.pop( );
	}
}

uint32_t CBaseGame :: GetNextTimedActionTicks( )
{
	// return the number of ticks (ms) until the next "timed action", which for our purposes is the next game update
	// the main GHost++ loop will make sure the next loop update happens at or before this value
	// note: there's no reason this function couldn't take into account the game's other timers too but they're far less critical
	// warning: this function must take into account when actions are not being sent (e.g. during loading or lagging)

	if( !m_GameLoaded || m_Lagging )
		return 50;

	uint32_t TicksSinceLastUpdate = GetTicks( ) - m_LastActionSentTicks;

	if( TicksSinceLastUpdate > m_Latency - m_LastActionLateBy )
		return 0;
	else
		return m_Latency - m_LastActionLateBy - TicksSinceLastUpdate;
}

uint32_t CBaseGame :: GetSlotsOccupied( )
{
	uint32_t NumSlotsOccupied = 0;

	for( vector<CGameSlot> :: iterator i = m_Slots.begin( ); i != m_Slots.end( ); ++i )
	{
		if( (*i).GetSlotStatus( ) == SLOTSTATUS_OCCUPIED )
			++NumSlotsOccupied;
	}

	return NumSlotsOccupied;
}

uint32_t CBaseGame :: GetSlotsOpen( )
{
	uint32_t NumSlotsOpen = 0;

	for( vector<CGameSlot> :: iterator i = m_Slots.begin( ); i != m_Slots.end( ); ++i )
	{
		if( (*i).GetSlotStatus( ) == SLOTSTATUS_OPEN )
			++NumSlotsOpen;
	}

	return NumSlotsOpen;
}

uint32_t CBaseGame :: GetNumPlayers( )
{
	return GetNumHumanPlayers( ) + m_FakePlayers.size( );
}

uint32_t CBaseGame :: GetNumHumanPlayers( )
{
	uint32_t NumHumanPlayers = 0;

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( !(*i)->GetLeftMessageSent( ) )
			++NumHumanPlayers;
	}

	return NumHumanPlayers;
}

string CBaseGame :: GetDescription( )
{
	string Description = m_GameName + " : " + m_OwnerName + " : " + UTIL_ToString( GetNumHumanPlayers( ) ) + "/" + UTIL_ToString( m_GameLoading || m_GameLoaded ? m_StartPlayers : m_Slots.size( ) );

	if( m_GameLoading || m_GameLoaded )
		Description += " : " + UTIL_ToString( ( m_GameTicks / 1000 ) / 60 ) + "m";
	else
		Description += " : " + UTIL_ToString( ( GetTime( ) - m_CreationTime ) / 60 ) + "m";

	return Description;
}

string CBaseGame :: GetPlayers( )
{
	string Players;
	
	for( unsigned int i = 0; i < m_Players.size( ); ++i )
	{
		if( !m_Players[i]->GetLeftMessageSent( ) )					
			Players += m_Players[i]->GetName( ) + ", ";
	}

        if( Players.size( ) > 2 )
            Players = Players.substr( 0, Players.size( ) - 2 );
		
	return Players;
}

unsigned int CBaseGame :: SetFD( void *fd, void *send_fd, int *nfds )
{
	int NumFDs = 0;

	if( m_Socket )
	{
		m_Socket->SetFD( (fd_set *)fd, (fd_set *)send_fd, nfds );
		++NumFDs;
	}

	for( vector<CPotentialPlayer *> :: iterator i = m_Potentials.begin( ); i != m_Potentials.end( ); ++i )
	{
		if( (*i)->GetSocket( ) )
		{
			(*i)->GetSocket( )->SetFD( (fd_set *)fd, (fd_set *)send_fd, nfds );
			++NumFDs;
		}
	}

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( (*i)->GetSocket( ) )
		{
			(*i)->GetSocket( )->SetFD( (fd_set *)fd, (fd_set *)send_fd, nfds );
			++NumFDs;
		}
	}

	return NumFDs;
}

bool CBaseGame :: Update( void *fd, void *send_fd )
{
	// update players

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); )
	{
		if( (*i)->Update( fd ) )
		{
			EventPlayerDeleted( *i );
			delete *i;
			i = m_Players.erase( i );
		}
		else
			++i;
	}

	for( vector<CPotentialPlayer *> :: iterator i = m_Potentials.begin( ); i != m_Potentials.end( ); )
	{
		if( (*i)->Update( fd ) )
		{
			// flush the socket (e.g. in case a rejection message is queued)

			if( (*i)->GetSocket( ) )
				(*i)->GetSocket( )->DoSend( (fd_set *)send_fd );

			delete *i;
			i = m_Potentials.erase( i );
		}
		else
			++i;
	}

	// create the virtual host player

	if( !m_GameLoading && !m_GameLoaded && GetNumPlayers( ) < 12 )
		CreateVirtualHost( );

	// unlock the game

	if( m_Locked && !GetPlayerFromName( m_OwnerName, false ) )
	{
		SendAllChat( m_GHost->m_Language->GameUnlocked( ) );
		m_Locked = false;
	}

	// ping every 5 seconds
	// changed this to ping during game loading as well to hopefully fix some problems with people disconnecting during loading
	// changed this to ping during the game as well

	if( GetTime( ) - m_LastPingTime >= 5 )
	{
		// note: we must send pings to players who are downloading the map because Warcraft III disconnects from the lobby if it doesn't receive a ping every ~90 seconds
		// so if the player takes longer than 90 seconds to download the map they would be disconnected unless we keep sending pings
		// todotodo: ignore pings received from players who have recently finished downloading the map

		SendAll( m_Protocol->SEND_W3GS_PING_FROM_HOST( ) );

		// we also broadcast the game to the local network every 5 seconds so we hijack this timer for our nefarious purposes
		// however we only want to broadcast if the countdown hasn't started
		// see the !sendlan code later in this file for some more information about how this works
		// todotodo: should we send a game cancel message somewhere? we'll need to implement a host counter for it to work

		if( !m_CountDownStarted )
		{
			// construct a fixed host counter which will be used to identify players from this "realm" (i.e. LAN)
			// the fixed host counter's 4 most significant bits will contain a 4 bit ID (0-15)
			// the rest of the fixed host counter will contain the 28 least significant bits of the actual host counter
			// since we're destroying 4 bits of information here the actual host counter should not be greater than 2^28 which is a reasonable assumption
			// when a player joins a game we can obtain the ID from the received host counter
			// note: LAN broadcasts use an ID of 0, battle.net refreshes use an ID of 1-10, the rest are unused

			uint32_t FixedHostCounter = m_HostCounter & 0x0FFFFFFF;

			// we send 12 for SlotsTotal because this determines how many PID's Warcraft 3 allocates
			// we need to make sure Warcraft 3 allocates at least SlotsTotal + 1 but at most 12 PID's
			// this is because we need an extra PID for the virtual host player (but we always delete the virtual host player when the 12th person joins)
			// however, we can't send 13 for SlotsTotal because this causes Warcraft 3 to crash when sharing control of units
			// nor can we send SlotsTotal because then Warcraft 3 crashes when playing maps with less than 12 PID's (because of the virtual host player taking an extra PID)
			// we also send 12 for SlotsOpen because Warcraft 3 assumes there's always at least one player in the game (the host)
			// so if we try to send accurate numbers it'll always be off by one and results in Warcraft 3 assuming the game is full when it still needs one more player
			// the easiest solution is to simply send 12 for both so the game will always show up as (1/12) players
			
			// note: the PrivateGame flag is not set when broadcasting to LAN (as you might expect)
			// note: we do not use m_Map->GetMapGameType because none of the filters are set when broadcasting to LAN (also as you might expect)

			uint32_t MapGameType = MAPGAMETYPE_UNKNOWN0;
			m_GHost->m_UDPSocket->Broadcast( 6112, m_Protocol->SEND_W3GS_GAMEINFO( m_GHost->m_LANWar3Version, UTIL_CreateByteArray( MapGameType, false ), m_Map->GetMapGameFlags( ), m_Map->GetMapWidth( ), m_Map->GetMapHeight( ), m_GameName, "Clan 007", 0, m_Map->GetMapPath( ), m_Map->GetMapCRC( ), 12, 12, m_HostPort, FixedHostCounter ) );
		}

		m_LastPingTime = GetTime( );
	}

	// refresh every 4 seconds

	if( !m_RefreshError && !m_CountDownStarted && m_GameState == GAME_PUBLIC && GetSlotsOpen( ) > 0 && GetTime( ) - m_LastRefreshTime >= 4 )
	{
		// send a game refresh packet to each battle.net connection

		bool Refreshed = false;

		for( vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
		{
			// don't queue a game refresh message if the queue contains more than 1 packet because they're very low priority

			if( (*i)->GetOutPacketsQueued( ) <= 1 )
			{
				(*i)->QueueGameRefresh( m_GameState, m_GameName, string( ), m_Map, m_HostCounter );
				Refreshed = true;
			}
		}

		m_LastRefreshTime = GetTime( );
	}

	// send more map data

	if( !m_GameLoading && !m_GameLoaded && GetTicks( ) - m_LastDownloadCounterResetTicks >= 1000 )
	{
		// hackhack: another timer hijack is in progress here
		// since the download counter is reset once per second it's a great place to update the slot info if necessary

		if( m_SlotInfoChanged )
			SendAllSlotInfo( );

		m_DownloadCounter = 0;
		m_LastDownloadCounterResetTicks = GetTicks( );
	}

	if( !m_GameLoading && !m_GameLoaded && GetTicks( ) - m_LastDownloadTicks >= 80 )
	{
		uint32_t Downloaders = 0;

		for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		{
			if( (*i)->GetDownloadStarted( ) && !(*i)->GetDownloadFinished( ) )
			{
				++Downloaders;

				if( m_GHost->m_MaxDownloaders > 0 && Downloaders > m_GHost->m_MaxDownloaders )
					break;

				// send up to 100 pieces of the map at once so that the download goes faster
				// if we wait for each MAPPART packet to be acknowledged by the client it'll take a long time to download
				// this is because we would have to wait the round trip time (the ping time) between sending every 1442 bytes of map data
				// doing it this way allows us to send at least 140 KB in each round trip interval which is much more reasonable
				// the theoretical throughput is [140 KB * 1000 / ping] in KB/sec so someone with 100 ping (round trip ping, not LC ping) could download at 1400 KB/sec
				// note: this creates a queue of map data which clogs up the connection when the client is on a slower connection (e.g. dialup)
				// in this case any changes to the lobby are delayed by the amount of time it takes to send the queued data (i.e. 140 KB, which could be 30 seconds or more)
				// for example, players joining and leaving, slot changes, chat messages would all appear to happen much later for the low bandwidth player
				// note: the throughput is also limited by the number of times this code is executed each second
				// e.g. if we send the maximum amount (140 KB) 10 times per second the theoretical throughput is 1400 KB/sec
				// therefore the maximum throughput is 1400 KB/sec regardless of ping and this value slowly diminishes as the player's ping increases
				// in addition to this, the throughput is limited by the configuration value bot_maxdownloadspeed
				// in summary: the actual throughput is MIN( 140 * 1000 / ping, 1400, bot_maxdownloadspeed ) in KB/sec assuming only one player is downloading the map

				uint32_t MapSize = UTIL_ByteArrayToUInt32( m_Map->GetMapSize( ), false );

				while( (*i)->GetLastMapPartSent( ) < (*i)->GetLastMapPartAcked( ) + 1442 * 100 && (*i)->GetLastMapPartSent( ) < MapSize )
				{
					if( (*i)->GetLastMapPartSent( ) == 0 )
					{
						// overwrite the "started download ticks" since this is the first time we've sent any map data to the player
						// prior to this we've only determined if the player needs to download the map but it's possible we could have delayed sending any data due to download limits

						(*i)->SetStartedDownloadingTicks( GetTicks( ) );
					}

					// limit the download speed if we're sending too much data
					// the download counter is the # of map bytes downloaded in the last second (it's reset once per second)

					if( m_GHost->m_MaxDownloadSpeed > 0 && m_DownloadCounter > m_GHost->m_MaxDownloadSpeed * 1024 )
						break;

					Send( *i, m_Protocol->SEND_W3GS_MAPPART( GetHostPID( ), (*i)->GetPID( ), (*i)->GetLastMapPartSent( ), m_Map->GetMapData( ) ) );
					(*i)->SetLastMapPartSent( (*i)->GetLastMapPartSent( ) + 1442 );
					m_DownloadCounter += 1442;
				}
			}
		}

		m_LastDownloadTicks = GetTicks( );
	}

	// countdown every 500 ms

	if( m_CountDownStarted && GetTicks( ) - m_LastCountDownTicks >= 500 )
	{
		if( m_CountDownCounter > 0 )
		{
			// we use a countdown counter rather than a "finish countdown time" here because it might alternately round up or down the count
			// this sometimes resulted in a countdown of e.g. "6 5 3 2 1" during my testing which looks pretty dumb
			// doing it this way ensures it's always "5 4 3 2 1" but each interval might not be *exactly* the same length

			SendAllChat( UTIL_ToString( m_CountDownCounter-- ) + ". . ." );
		}
		else if( !m_GameLoading && !m_GameLoaded )
			EventGameStarted( );

		m_LastCountDownTicks = GetTicks( );
	}

	// check if the lobby is "abandoned" and needs to be closed since it will never start

	if( !m_GameLoading && !m_GameLoaded && m_GHost->m_LobbyTimeLimit > 0 )
	{
		// check if there's a player with reserved status in the game

		for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		{
			if( (*i)->GetReserved( ) )
				m_LastReservedSeen = GetTime( );
		}

		// check if we've hit the time limit

		if( GetTime( ) - m_LastReservedSeen >= m_GHost->m_LobbyTimeLimit * 60 )
		{
			CONSOLE_Print( "[GAME: " + m_GameName + "] is over (lobby time limit hit)" );
			return true;
		}
	}

	// check if the game is loaded

	if( m_GameLoading )
	{
		bool FinishedLoading = true;

		for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		{
			FinishedLoading = (*i)->GetFinishedLoading( );

			if( !FinishedLoading )
				break;
		}

		if( FinishedLoading )
		{			
			m_LastActionSentTicks = GetTicks( );
			m_GameLoading = false;
			m_GameLoaded = true;
			EventGameLoaded( );
		}
		else
		{
			// reset the "lag" screen (the load-in-game screen) every 30 seconds

			if( m_LoadInGame && GetTime( ) - m_LastLagScreenResetTime >= 30 )
			{
				bool UsingGProxy = false;

				for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
				{
					if( (*i)->GetGProxy( ) )
						UsingGProxy = true;
				}

				for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
				{
					if( (*i)->GetFinishedLoading( ) )
					{
						// stop the lag screen

						for( vector<CGamePlayer *> :: iterator j = m_Players.begin( ); j != m_Players.end( ); ++j )
						{
							if( !(*j)->GetFinishedLoading( ) )
								Send( *i, m_Protocol->SEND_W3GS_STOP_LAG( *j, true ) );
						}

						// send an empty update
						// this resets the lag screen timer but creates a rather annoying problem
						// in order to prevent a desync we must make sure every player receives the exact same "desyncable game data" (updates and player leaves) in the exact same order
						// unfortunately we cannot send updates to players who are still loading the map, so we buffer the updates to those players (see the else clause a few lines down for the code)
						// in addition to this we must ensure any player leave messages are sent in the exact same position relative to these updates so those must be buffered too

						if( UsingGProxy && !(*i)->GetGProxy( ) )
						{
							// we must send empty actions to non-GProxy++ players
							// GProxy++ will insert these itself so we don't need to send them to GProxy++ players
							// empty actions are used to extend the time a player can use when reconnecting

							for( unsigned char j = 0; j < m_GProxyEmptyActions; ++j )
								Send( *i, m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );
						}

						Send( *i, m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );

						// start the lag screen

						Send( *i, m_Protocol->SEND_W3GS_START_LAG( m_Players, true ) );
					}
					else
					{
						// buffer the empty update since the player is still loading the map

						if( UsingGProxy && !(*i)->GetGProxy( ) )
						{
							// we must send empty actions to non-GProxy++ players
							// GProxy++ will insert these itself so we don't need to send them to GProxy++ players
							// empty actions are used to extend the time a player can use when reconnecting

							for( unsigned char j = 0; j < m_GProxyEmptyActions; ++j )
								(*i)->AddLoadInGameData( m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );
						}

						(*i)->AddLoadInGameData( m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );
					}
				}

				// Warcraft III doesn't seem to respond to empty actions

				/* if( UsingGProxy )
					m_SyncCounter += m_GProxyEmptyActions;

				m_SyncCounter++; */
				m_LastLagScreenResetTime = GetTime( );
			}
		}
	}

	// keep track of the largest sync counter (the number of keepalive packets received by each player)
	// if anyone falls behind by more than m_SyncLimit keepalives we start the lag screen

	if( m_GameLoaded )
	{
		// check if anyone has started lagging
		// we consider a player to have started lagging if they're more than m_SyncLimit keepalives behind

		if( !m_Lagging )
		{
			string LaggingString;

			for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
			{
				if( m_SyncCounter - (*i)->GetSyncCounter( ) > m_SyncLimit )
				{
					(*i)->SetLagging( true );
					(*i)->SetStartedLaggingTicks( GetTicks( ) );
					m_Lagging = true;
					m_StartedLaggingTime = GetTime( );

					if( LaggingString.empty( ) )
						LaggingString = (*i)->GetName( );
					else
						LaggingString += ", " + (*i)->GetName( );
				}
			}

			if( m_Lagging )
			{
				// start the lag screen

				CONSOLE_Print( "[GAME: " + m_GameName + "] started lagging on [" + LaggingString + "]" );
				SendAll( m_Protocol->SEND_W3GS_START_LAG( m_Players ) );

				// reset everyone's drop vote

				for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
					(*i)->SetDropVote( false );

				m_LastLagScreenResetTime = GetTime( );
			}
		}

		if( m_Lagging )
		{
			bool UsingGProxy = false;

			for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
			{
				if( (*i)->GetGProxy( ) )
					UsingGProxy = true;
			}

			uint32_t WaitTime = 60;

			if( UsingGProxy )
				WaitTime = ( m_GProxyEmptyActions + 1 ) * 60;

			if( GetTime( ) - m_StartedLaggingTime >= WaitTime )
				StopLaggers( m_GHost->m_Language->WasAutomaticallyDroppedAfterSeconds( UTIL_ToString( WaitTime ) ) );

			// we cannot allow the lag screen to stay up for more than ~65 seconds because Warcraft III disconnects if it doesn't receive an action packet at least this often
			// one (easy) solution is to simply drop all the laggers if they lag for more than 60 seconds
			// another solution is to reset the lag screen the same way we reset it when using load-in-game
			// this is required in order to give GProxy++ clients more time to reconnect

			if( GetTime( ) - m_LastLagScreenResetTime >= 60 )
			{
				for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
				{
					// stop the lag screen

					for( vector<CGamePlayer *> :: iterator j = m_Players.begin( ); j != m_Players.end( ); ++j )
					{
						if( (*j)->GetLagging( ) )
							Send( *i, m_Protocol->SEND_W3GS_STOP_LAG( *j ) );
					}

					// send an empty update
					// this resets the lag screen timer

					if( UsingGProxy && !(*i)->GetGProxy( ) )
					{
						// we must send additional empty actions to non-GProxy++ players
						// GProxy++ will insert these itself so we don't need to send them to GProxy++ players
						// empty actions are used to extend the time a player can use when reconnecting

						for( unsigned char j = 0; j < m_GProxyEmptyActions; ++j )
							Send( *i, m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );
					}

					Send( *i, m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );

					// start the lag screen

					Send( *i, m_Protocol->SEND_W3GS_START_LAG( m_Players ) );
				}

				m_LastLagScreenResetTime = GetTime( );
			}

			// check if anyone has stopped lagging normally
			// we consider a player to have stopped lagging if they're less than half m_SyncLimit keepalives behind

			for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
			{
				if( (*i)->GetLagging( ) && m_SyncCounter - (*i)->GetSyncCounter( ) < m_SyncLimit / 2 )
				{
					// stop the lag screen for this player

					CONSOLE_Print( "[GAME: " + m_GameName + "] stopped lagging on [" + (*i)->GetName( ) + "]" );
					SendAll( m_Protocol->SEND_W3GS_STOP_LAG( *i ) );
					(*i)->SetLagging( false );
					(*i)->SetStartedLaggingTicks( 0 );
				}
			}

			// check if everyone has stopped lagging

			bool Lagging = false;

			for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
			{
				if( (*i)->GetLagging( ) )
					Lagging = true;
			}

			m_Lagging = Lagging;

			// reset m_LastActionSentTicks because we want the game to stop running while the lag screen is up

			m_LastActionSentTicks = GetTicks( );

			// keep track of the last lag screen time so we can avoid timing out players

			m_LastLagScreenTime = GetTime( );
		}
	}

	// send actions every m_Latency milliseconds
	// actions are at the heart of every Warcraft 3 game but luckily we don't need to know their contents to relay them
	// we queue player actions in EventPlayerAction then just resend them in batches to all players here

	if( m_GameLoaded && !m_Lagging && GetTicks( ) - m_LastActionSentTicks >= m_Latency - m_LastActionLateBy )
		SendAllActions( );

	// expire the votekick

	if( !m_KickVotePlayer.empty( ) && GetTime( ) - m_StartedKickVoteTime >= 60 )
	{
		CONSOLE_Print( "[GAME: " + m_GameName + "] votekick against player [" + m_KickVotePlayer + "] expired" );
		SendAllChat( m_GHost->m_Language->VoteKickExpired( m_KickVotePlayer ) );
		m_KickVotePlayer.clear( );
		m_StartedKickVoteTime = 0;
	}

	// start the gameover timer if there's only one player left

	if( m_Players.size( ) == 1 && m_FakePlayers.size( ) == 0 && m_GameOverTime == 0 && ( m_GameLoading || m_GameLoaded ) )
	{
		CONSOLE_Print( "[GAME: " + m_GameName + "] gameover timer started (one player left)" );
		m_GameOverTime = GetTime( );
	}

	// finish the gameover timer

	if( m_GameOverTime != 0 && GetTime( ) - m_GameOverTime >= 60 )
	{
		bool AlreadyStopped = true;

		for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		{
			if( !(*i)->GetDeleteMe( ) )
			{
				AlreadyStopped = false;
				break;
			}
		}

		if( !AlreadyStopped )
		{
			CONSOLE_Print( "[GAME: " + m_GameName + "] is over (gameover timer finished)" );
			StopPlayers( "was disconnected (gameover timer finished)" );
		}
	}

	// end the game if there aren't any players left

	if( m_Players.empty( ) && ( m_GameLoading || m_GameLoaded ) )
	{
		if( !m_Saving )
		{
			CONSOLE_Print( "[GAME: " + m_GameName + "] is over (no players left)" );
			SaveGameData( );
			m_Saving = true;
		}
		else if( IsGameDataSaved( ) )
			return true;
	}

	// accept new connections

	if( m_Socket )
	{
		CTCPSocket *NewSocket = m_Socket->Accept( (fd_set *)fd );

		if( NewSocket )
		{
			if( m_GHost->m_TCPNoDelay )
				NewSocket->SetNoDelay( );

			m_Potentials.push_back( new CPotentialPlayer( m_Protocol, this, NewSocket ) );
		}

		if( m_Socket->HasError( ) )
			return true;
	}

	return m_Exiting;
}

void CBaseGame :: UpdatePost( void *send_fd )
{
	// we need to manually call DoSend on each player now because CGamePlayer :: Update doesn't do it
	// this is in case player 2 generates a packet for player 1 during the update but it doesn't get sent because player 1 already finished updating
	// in reality since we're queueing actions it might not make a big difference but oh well

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( (*i)->GetSocket( ) )
			(*i)->GetSocket( )->DoSend( (fd_set *)send_fd );
	}

	for( vector<CPotentialPlayer *> :: iterator i = m_Potentials.begin( ); i != m_Potentials.end( ); ++i )
	{
		if( (*i)->GetSocket( ) )
			(*i)->GetSocket( )->DoSend( (fd_set *)send_fd );
	}
}

void CBaseGame :: Send( CGamePlayer *player, const BYTEARRAY &data )
{
	if( player )
		player->Send( data );
}

void CBaseGame :: Send( unsigned char PID, const BYTEARRAY &data )
{
	Send( GetPlayerFromPID( PID ), data );
}

void CBaseGame :: Send( const BYTEARRAY &PIDs, const BYTEARRAY &data )
{
	for( unsigned int i = 0; i < PIDs.size( ); ++i )
		Send( PIDs[i], data );
}

void CBaseGame :: SendAll( const BYTEARRAY &data )
{
	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		(*i)->Send( data );
}

void CBaseGame :: SendChat( unsigned char fromPID, CGamePlayer *player, const string &message )
{
	// send a private message to one player - it'll be marked [Private] in Warcraft 3

	if( player )
	{
		if( !m_GameLoading && !m_GameLoaded )
		{
			if( message.size( ) > 254 )
				Send( player, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, UTIL_CreateByteArray( player->GetPID( ) ), 16, BYTEARRAY( ), message.substr( 0, 254 ) ) );
			else
				Send( player, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, UTIL_CreateByteArray( player->GetPID( ) ), 16, BYTEARRAY( ), message ) );
		}
		else
		{
			unsigned char ExtraFlags[] = { 3, 0, 0, 0 };

			// based on my limited testing it seems that the extra flags' first byte contains 3 plus the recipient's colour to denote a private message

			unsigned char SID = GetSIDFromPID( player->GetPID( ) );

			if( SID < m_Slots.size( ) )
				ExtraFlags[0] = 3 + m_Slots[SID].GetColour( );

			if( message.size( ) > 127 )
				Send( player, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, UTIL_CreateByteArray( player->GetPID( ) ), 32, UTIL_CreateByteArray( ExtraFlags, 4 ), message.substr( 0, 127 ) ) );
			else
				Send( player, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, UTIL_CreateByteArray( player->GetPID( ) ), 32, UTIL_CreateByteArray( ExtraFlags, 4 ), message ) );
		}
	}
}

void CBaseGame :: SendChat( unsigned char fromPID, unsigned char toPID, const string &message )
{
	SendChat( fromPID, GetPlayerFromPID( toPID ), message );
}

void CBaseGame :: SendChat( CGamePlayer *player, const string &message )
{
	SendChat( GetHostPID( ), player, message );
}

void CBaseGame :: SendChat( unsigned char toPID, const string &message )
{
	SendChat( GetHostPID( ), toPID, message );
}

void CBaseGame :: SendAllChat( unsigned char fromPID, const string &message )
{
	// send a public message to all players - it'll be marked [All] in Warcraft 3

	if( GetNumHumanPlayers( ) > 0 )
	{
		CONSOLE_Print( "[GAME: " + m_GameName + "] [Local]: " + message );

		if( !m_GameLoading && !m_GameLoaded )
		{
			if( message.size( ) > 254 )
				SendAll( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetPIDs( ), 16, BYTEARRAY( ), message.substr( 0, 254 ) ) );
			else
				SendAll( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetPIDs( ), 16, BYTEARRAY( ), message ) );
		}
		else
		{
			if( message.size( ) > 127 )
				SendAll( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetPIDs( ), 32, UTIL_CreateByteArray( (uint32_t)0, false ), message.substr( 0, 127 ) ) );
			else
				SendAll( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetPIDs( ), 32, UTIL_CreateByteArray( (uint32_t)0, false ), message ) );
		}
	}
}

void CBaseGame :: SendAllChat( const string &message )
{
	SendAllChat( GetHostPID( ), message );
}

void CBaseGame :: SendAllSlotInfo( )
{
	if( !m_GameLoading && !m_GameLoaded )
	{
		SendAll( m_Protocol->SEND_W3GS_SLOTINFO( m_Slots, m_RandomSeed, m_Map->GetMapLayoutStyle( ), m_Map->GetMapNumPlayers( ) ) );
		m_SlotInfoChanged = false;
	}
}

void CBaseGame :: SendVirtualHostPlayerInfo( CGamePlayer *player )
{
	if( m_VirtualHostPID == 255 )
		return;

	BYTEARRAY IP;
	IP.push_back( 0 );
	IP.push_back( 0 );
	IP.push_back( 0 );
	IP.push_back( 0 );
	Send( player, m_Protocol->SEND_W3GS_PLAYERINFO( m_VirtualHostPID, m_VirtualHostName, IP, IP ) );
}

void CBaseGame :: SendFakePlayerInfo( CGamePlayer *player )
{
	if( m_FakePlayers.empty( ) )
		return;

	BYTEARRAY IP;
	IP.push_back( 0 );
	IP.push_back( 0 );
	IP.push_back( 0 );
	IP.push_back( 0 );

	srand( GetTicks( ) );

	for( vector<unsigned char> :: iterator i = m_FakePlayers.begin( ); i != m_FakePlayers.end( ); ++i )
	{
		Send( player, m_Protocol->SEND_W3GS_PLAYERINFO( *i, "Fake[" + UTIL_ToString( *i) + "]", IP, IP ) );
	}	
}

void CBaseGame :: SendAllActions( )
{
	bool UsingGProxy = false;

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( (*i)->GetGProxy( ) )
			UsingGProxy = true;
	}

	m_GameTicks += m_Latency;

	if( UsingGProxy )
	{
		// we must send empty actions to non-GProxy++ players
		// GProxy++ will insert these itself so we don't need to send them to GProxy++ players
		// empty actions are used to extend the time a player can use when reconnecting

		for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		{
			if( !(*i)->GetGProxy( ) )
			{
				for( unsigned char j = 0; j < m_GProxyEmptyActions; ++j )
					Send( *i, m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );
			}
		}
	}

	// Warcraft III doesn't seem to respond to empty actions

	/* if( UsingGProxy )
		m_SyncCounter += m_GProxyEmptyActions; */

	++m_SyncCounter;

	// we aren't allowed to send more than 1460 bytes in a single packet but it's possible we might have more than that many bytes waiting in the queue

	if( !m_Actions.empty( ) )
	{
		// we use a "sub actions queue" which we keep adding actions to until we reach the size limit
		// start by adding one action to the sub actions queue

		queue<CIncomingAction *> SubActions;
		CIncomingAction *Action = m_Actions.front( );
		m_Actions.pop( );
		SubActions.push( Action );
		uint32_t SubActionsLength = Action->GetLength( );

		while( !m_Actions.empty( ) )
		{
			Action = m_Actions.front( );
			m_Actions.pop( );

			// check if adding the next action to the sub actions queue would put us over the limit (1452 because the INCOMING_ACTION and INCOMING_ACTION2 packets use an extra 8 bytes)

			if( SubActionsLength + Action->GetLength( ) > 1452 )
			{
				// we'd be over the limit if we added the next action to the sub actions queue
				// so send everything already in the queue and then clear it out
				// the W3GS_INCOMING_ACTION2 packet handles the overflow but it must be sent *before* the corresponding W3GS_INCOMING_ACTION packet

				SendAll( m_Protocol->SEND_W3GS_INCOMING_ACTION2( SubActions ) );

				while( !SubActions.empty( ) )
				{
					delete SubActions.front( );
					SubActions.pop( );
				}

				SubActionsLength = 0;
			}

			SubActions.push( Action );
			SubActionsLength += Action->GetLength( );
		}

		SendAll( m_Protocol->SEND_W3GS_INCOMING_ACTION( SubActions, m_Latency ) );

		while( !SubActions.empty( ) )
		{
			delete SubActions.front( );
			SubActions.pop( );
		}
	}
	else
	{
		SendAll( m_Protocol->SEND_W3GS_INCOMING_ACTION( m_Actions, m_Latency ) );
	}

	uint32_t ActualSendInterval = GetTicks( ) - m_LastActionSentTicks;
	uint32_t ExpectedSendInterval = m_Latency - m_LastActionLateBy;
	m_LastActionLateBy = ActualSendInterval - ExpectedSendInterval;

	if( m_LastActionLateBy > m_Latency )
	{
		// something is going terribly wrong - GHost++ is probably starved of resources
		// print a message because even though this will take more resources it should provide some information to the administrator for future reference
		// other solutions - dynamically modify the latency, request higher priority, terminate other games, ???

		CONSOLE_Print( "[GAME: " + m_GameName + "] warning - the latency is " + UTIL_ToString( m_Latency ) + "ms but the last update was late by " + UTIL_ToString( m_LastActionLateBy ) + "ms" );
		m_LastActionLateBy = m_Latency;
	}

	m_LastActionSentTicks = GetTicks( );
}

void CBaseGame :: EventPlayerDeleted( CGamePlayer *player )
{
	CONSOLE_Print( "[GAME: " + m_GameName + "] deleting player [" + player->GetName( ) + "]: " + player->GetLeftReason( ) );

	m_LastPlayerLeaveTicks = GetTicks( );

	// in some cases we're forced to send the left message early so don't send it again

	if( player->GetLeftMessageSent( ) )
		return;

	if( m_GameLoaded )
		SendAllChat( player->GetName( ) + " " + player->GetLeftReason( ) + "." );

	if( player->GetLagging( ) )
		SendAll( m_Protocol->SEND_W3GS_STOP_LAG( player ) );


	if( m_LoadInGame && m_GameLoading )
	{
		// we must buffer player leave messages when using "load in game" to prevent desyncs
		// this ensures the player leave messages are correctly interleaved with the empty updates sent to each player

		for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		{
			if( (*i)->GetFinishedLoading( ) )
			{
				if( !player->GetFinishedLoading( ) )
					Send( *i, m_Protocol->SEND_W3GS_STOP_LAG( player ) );

				Send( *i, m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( player->GetPID( ), player->GetLeftCode( ) ) );
			}
			else
				(*i)->AddLoadInGameData( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( player->GetPID( ), player->GetLeftCode( ) ) );
		}
	}
	else
	{
		// tell everyone about the player leaving

		SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( player->GetPID( ), player->GetLeftCode( ) ) );
	}


	// abort the countdown if there was one in progress

	if( m_CountDownStarted && !m_GameLoading && !m_GameLoaded )
	{
		SendAllChat( m_GHost->m_Language->CountDownAborted( ) );
		m_CountDownStarted = false;
	}

	// abort the votekick

	if( !m_KickVotePlayer.empty( ) )
		SendAllChat( m_GHost->m_Language->VoteKickCancelled( m_KickVotePlayer ) );

	m_KickVotePlayer.clear( );
	m_StartedKickVoteTime = 0;
}

void CBaseGame :: EventPlayerDisconnectTimedOut( CGamePlayer *player )
{
	if( player->GetGProxy( ) && m_GameLoaded )
	{
		if( !player->GetGProxyDisconnectNoticeSent( ) )
		{
			SendAllChat( player->GetName( ) + " " + m_GHost->m_Language->HasLostConnectionTimedOutGProxy( ) + "." );
			player->SetGProxyDisconnectNoticeSent( true );
		}

		if( GetTime( ) - player->GetLastGProxyWaitNoticeSentTime( ) >= 20 )
		{
			uint32_t TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60 - ( GetTime( ) - m_StartedLaggingTime );

			if( TimeRemaining > ( (uint32_t)m_GProxyEmptyActions + 1 ) * 60 )
				TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60;

			SendAllChat( player->GetPID( ), m_GHost->m_Language->WaitForReconnectSecondsRemain( UTIL_ToString( TimeRemaining ) ) );
			player->SetLastGProxyWaitNoticeSentTime( GetTime( ) );
		}

		return;
	}

	// not only do we not do any timeouts if the game is lagging, we allow for an additional grace period of 10 seconds
	// this is because Warcraft 3 stops sending packets during the lag screen
	// so when the lag screen finishes we would immediately disconnect everyone if we didn't give them some extra time

	if( GetTime( ) - m_LastLagScreenTime >= 10 )
	{
		player->SetDeleteMe( true );
		player->SetLeftReason( m_GHost->m_Language->HasLostConnectionTimedOut( ) );
		player->SetLeftCode( PLAYERLEAVE_DISCONNECT );

		if( !m_GameLoading && !m_GameLoaded )
			OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
	}
}

void CBaseGame :: EventPlayerDisconnectPlayerError( CGamePlayer *player )
{
	// at the time of this comment there's only one player error and that's when we receive a bad packet from the player
	// since TCP has checks and balances for data corruption the chances of this are pretty slim

	player->SetDeleteMe( true );
	player->SetLeftReason( m_GHost->m_Language->HasLostConnectionPlayerError( player->GetErrorString( ) ) );
	player->SetLeftCode( PLAYERLEAVE_DISCONNECT );

	if( !m_GameLoading && !m_GameLoaded )
		OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
}

void CBaseGame :: EventPlayerDisconnectSocketError( CGamePlayer *player )
{
	if( player->GetGProxy( ) && m_GameLoaded )
	{
		if( !player->GetGProxyDisconnectNoticeSent( ) )
		{
			SendAllChat( player->GetName( ) + " " + m_GHost->m_Language->HasLostConnectionSocketErrorGProxy( player->GetSocket( )->GetErrorString( ) ) + "." );
			player->SetGProxyDisconnectNoticeSent( true );
		}

		if( GetTime( ) - player->GetLastGProxyWaitNoticeSentTime( ) >= 20 )
		{
			uint32_t TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60 - ( GetTime( ) - m_StartedLaggingTime );

			if( TimeRemaining > ( (uint32_t)m_GProxyEmptyActions + 1 ) * 60 )
				TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60;

			SendAllChat( player->GetPID( ), m_GHost->m_Language->WaitForReconnectSecondsRemain( UTIL_ToString( TimeRemaining ) ) );
			player->SetLastGProxyWaitNoticeSentTime( GetTime( ) );
		}

		return;
	}

	player->SetDeleteMe( true );
	player->SetLeftReason( m_GHost->m_Language->HasLostConnectionSocketError( player->GetSocket( )->GetErrorString( ) ) );
	player->SetLeftCode( PLAYERLEAVE_DISCONNECT );

	if( !m_GameLoading && !m_GameLoaded )
		OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
}

void CBaseGame :: EventPlayerDisconnectConnectionClosed( CGamePlayer *player )
{
	if( player->GetGProxy( ) && m_GameLoaded )
	{
		if( !player->GetGProxyDisconnectNoticeSent( ) )
		{
			SendAllChat( player->GetName( ) + " " + m_GHost->m_Language->HasLostConnectionClosedByRemoteHostGProxy( ) + "." );
			player->SetGProxyDisconnectNoticeSent( true );
		}

		if( GetTime( ) - player->GetLastGProxyWaitNoticeSentTime( ) >= 20 )
		{
			uint32_t TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60 - ( GetTime( ) - m_StartedLaggingTime );

			if( TimeRemaining > ( (uint32_t)m_GProxyEmptyActions + 1 ) * 60 )
				TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60;

			SendAllChat( player->GetPID( ), m_GHost->m_Language->WaitForReconnectSecondsRemain( UTIL_ToString( TimeRemaining ) ) );
			player->SetLastGProxyWaitNoticeSentTime( GetTime( ) );
		}

		return;
	}

	player->SetDeleteMe( true );
	player->SetLeftReason( m_GHost->m_Language->HasLostConnectionClosedByRemoteHost( ) );
	player->SetLeftCode( PLAYERLEAVE_DISCONNECT );

	if( !m_GameLoading && !m_GameLoaded )
		OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
}

void CBaseGame :: EventPlayerJoined( CPotentialPlayer *potential, CIncomingJoinPlayer *joinPlayer )
{
	// check if the new player's name is empty or too long

	if( joinPlayer->GetName( ).empty( ) || joinPlayer->GetName( ).size( ) > 15 || joinPlayer->GetName( ) == m_VirtualHostName || GetPlayerFromName( joinPlayer->GetName( ), false ) || joinPlayer->GetName( ).find( " " ) != string :: npos || joinPlayer->GetName( ).find( "|" ) != string :: npos  )
	{
		CONSOLE_Print( "[GAME: " + m_GameName + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] invalid name (taken, invalid chars, spoofer, too long...)" );
		potential->Send( m_Protocol->SEND_W3GS_REJECTJOIN( REJECTJOIN_FULL ) );
		potential->SetDeleteMe( true );
		return;
	}

	// identify their joined realm
	// this is only possible because when we send a game refresh via LAN or battle.net we encode an ID value in the 4 most significant bits of the host counter
	// the client sends the host counter when it joins so we can extract the ID value here
	// note: this is not a replacement for spoof checking since it doesn't verify the player's name and it can be spoofed anyway

	string JoinedRealm;
	uint32_t HostCounterID = joinPlayer->GetHostCounter( ) >> 28;

	// we use an ID value of 0 to denote joining via LAN, we don't have to set their joined realm.

	if( HostCounterID != 0 )
	{
		for( vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
		{
			if( (*i)->GetHostCounterID( ) == HostCounterID )
			{
				JoinedRealm = (*i)->GetServer( );
				break;
			}
		}
	}

	// check if the new player's name is banned but only if bot_banmethod is not 0
	// this is because if bot_banmethod is 0 and we announce the ban here it's possible for the player to be rejected later because the game is full
	// this would allow the player to spam the chat by attempting to join the game multiple times in a row

	if( m_GHost->m_BanMethod != 0 )
	{
		for( vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
		{
			if( (*i)->GetServer( ) == JoinedRealm )
			{
				CDBBan *Ban = (*i)->IsBannedName( joinPlayer->GetName( ) );

				if( Ban )
				{
					if( m_GHost->m_BanMethod == 1 || m_GHost->m_BanMethod == 3 )
					{
						CONSOLE_Print( "[GAME: " + m_GameName + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] is trying to join the game but is banned by name" );

						if( m_IgnoredNames.find( joinPlayer->GetName( ) ) == m_IgnoredNames.end( ) )
						{
							SendAllChat( m_GHost->m_Language->TryingToJoinTheGameButBannedByName( joinPlayer->GetName( ) ) );
							SendAllChat( m_GHost->m_Language->UserWasBannedOnByBecause( Ban->GetServer( ), Ban->GetName( ), Ban->GetDate( ), Ban->GetAdmin( ), Ban->GetReason( ) ) );
							m_IgnoredNames.insert( joinPlayer->GetName( ) );
						}

						// let banned players "join" the game with an arbitrary PID then immediately close the connection
						// this causes them to be kicked back to the chat channel on battle.net

						vector<CGameSlot> Slots = m_Map->GetSlots( );
						potential->Send( m_Protocol->SEND_W3GS_SLOTINFOJOIN( 1, potential->GetSocket( )->GetPort( ), potential->GetExternalIP( ), Slots, 0, m_Map->GetMapLayoutStyle( ), m_Map->GetMapNumPlayers( ) ) );
						potential->SetDeleteMe( true );
						return;
					}

					break;
				}
			}

			CDBBan *Ban = (*i)->IsBannedIP( potential->GetExternalIPString( ) );

			if( Ban )
			{
				if( m_GHost->m_BanMethod == 2 || m_GHost->m_BanMethod == 3 )
				{
					CONSOLE_Print( "[GAME: " + m_GameName + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] is trying to join the game but is banned by IP address" );

					if( m_IgnoredNames.find( joinPlayer->GetName( ) ) == m_IgnoredNames.end( ) )
					{
						SendAllChat( m_GHost->m_Language->TryingToJoinTheGameButBannedByIP( joinPlayer->GetName( ), potential->GetExternalIPString( ), Ban->GetName( ) ) );
						SendAllChat( m_GHost->m_Language->UserWasBannedOnByBecause( Ban->GetServer( ), Ban->GetName( ), Ban->GetDate( ), Ban->GetAdmin( ), Ban->GetReason( ) ) );
						m_IgnoredNames.insert( joinPlayer->GetName( ) );
					}

					// let banned players "join" the game with an arbitrary PID then immediately close the connection
					// this causes them to be kicked back to the chat channel on battle.net

					vector<CGameSlot> Slots = m_Map->GetSlots( );
					potential->Send( m_Protocol->SEND_W3GS_SLOTINFOJOIN( 1, potential->GetSocket( )->GetPort( ), potential->GetExternalIP( ), Slots, 0, m_Map->GetMapLayoutStyle( ), m_Map->GetMapNumPlayers( ) ) );
					potential->SetDeleteMe( true );
					return;
				}

				break;
			}
		}
	}

	// check if the player is an admin or root admin on any connected realm for determining reserved status
	// we can't just use the spoof checked realm like in EventPlayerBotCommand because the player hasn't spoof checked yet

	bool AnyAdminCheck = IsAdmin( joinPlayer->GetName( ) );
	bool Reserved = IsReserved( joinPlayer->GetName( ) ) || AnyAdminCheck || IsOwner( joinPlayer->GetName( ) );

	// try to find a slot

	unsigned char SID = 255;
	
	// try to find an empty slot

	SID = GetEmptySlot( false );

	if( SID == 255 && Reserved )
	{
		// a reserved player is trying to join the game but it's full, try to find a reserved slot

		SID = GetEmptySlot( true );
		
		if( SID != 255 )
		{
			CGamePlayer *KickedPlayer = GetPlayerFromSID( SID );

			if( KickedPlayer )
			{
				KickedPlayer->SetDeleteMe( true );
				KickedPlayer->SetLeftReason( m_GHost->m_Language->WasKickedForReservedPlayer( joinPlayer->GetName( ) ) );
				KickedPlayer->SetLeftCode( PLAYERLEAVE_LOBBY );

				// send a playerleave message immediately since it won't normally get sent until the player is deleted which is after we send a playerjoin message
				// we don't need to call OpenSlot here because we're about to overwrite the slot data anyway

				SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( KickedPlayer->GetPID( ), KickedPlayer->GetLeftCode( ) ) );
				KickedPlayer->SetLeftMessageSent( true );
			}
		}
	}

	if( SID == 255 && IsOwner( joinPlayer->GetName( ) ) )
	{
		// the owner player is trying to join the game but it's full and we couldn't even find a reserved slot, kick the player in the lowest numbered slot
		// updated this to try to find a player slot so that we don't end up kicking a computer

		SID = 0;

		for( unsigned char i = 0; i < m_Slots.size( ); ++i )
		{
			if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[i].GetComputer( ) == 0 )
			{
				SID = i;
				break;
			}
		}

		CGamePlayer *KickedPlayer = GetPlayerFromSID( SID );
		
		if( KickedPlayer )
		{
			KickedPlayer->SetDeleteMe( true );
			KickedPlayer->SetLeftReason( m_GHost->m_Language->WasKickedForOwnerPlayer( joinPlayer->GetName( ) ) );
			KickedPlayer->SetLeftCode( PLAYERLEAVE_LOBBY );

			// send a playerleave message immediately since it won't normally get sent until the player is deleted which is after we send a playerjoin message
			// we don't need to call OpenSlot here because we're about to overwrite the slot data anyway

			SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( KickedPlayer->GetPID( ), KickedPlayer->GetLeftCode( ) ) );
			KickedPlayer->SetLeftMessageSent( true );
		}
	}

	if( SID >= m_Slots.size( ) )
	{
		potential->Send( m_Protocol->SEND_W3GS_REJECTJOIN( REJECTJOIN_FULL ) );
		potential->SetDeleteMe( true );
		return;
	}

	// check if the new player's name is banned but only if bot_banmethod is 0
	// this is because if bot_banmethod is 0 we need to wait to announce the ban until now because they could have been rejected because the game was full
	// this would have allowed the player to spam the chat by attempting to join the game multiple times in a row

	if( m_GHost->m_BanMethod == 0 )
	{
		for( vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
		{
			if( (*i)->GetServer( ) == JoinedRealm )
			{
				CDBBan *Ban = (*i)->IsBannedName( joinPlayer->GetName( ) );

				if( Ban )
				{
					CONSOLE_Print( "[GAME: " + m_GameName + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] is using a banned name" );
					SendAllChat( m_GHost->m_Language->HasBannedName( joinPlayer->GetName( ) ) );
					SendAllChat( m_GHost->m_Language->UserWasBannedOnByBecause( Ban->GetServer( ), Ban->GetName( ), Ban->GetDate( ), Ban->GetAdmin( ), Ban->GetReason( ) ) );
					break;
				}
			}

			CDBBan *Ban = (*i)->IsBannedIP( potential->GetExternalIPString( ) );

			if( Ban )
			{
				CONSOLE_Print( "[GAME: " + m_GameName + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] is using a banned IP address" );
				SendAllChat( m_GHost->m_Language->HasBannedIP( joinPlayer->GetName( ), potential->GetExternalIPString( ), Ban->GetName( ) ) );
				SendAllChat( m_GHost->m_Language->UserWasBannedOnByBecause( Ban->GetServer( ), Ban->GetName( ), Ban->GetDate( ), Ban->GetAdmin( ), Ban->GetReason( ) ) );
				break;
			}
		}
	}

	// we have a slot for the new player
	// make room for them by deleting the virtual host player if we have to

	if( GetNumPlayers( ) >= 11 )
		DeleteVirtualHost( );

	// turning the CPotentialPlayer into a CGamePlayer is a bit of a pain because we have to be careful not to close the socket
	// this problem is solved by setting the socket to NULL before deletion and handling the NULL case in the destructor
	// we also have to be careful to not modify the m_Potentials vector since we're currently looping through it

	CONSOLE_Print( "[GAME: " + m_GameName + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] joined the game" );
	CGamePlayer *Player = new CGamePlayer( potential, GetNewPID( ), JoinedRealm, joinPlayer->GetName( ), joinPlayer->GetInternalIP( ), Reserved );

	// consider LAN players to have already spoof checked since they can't
	// since so many people have trouble with this feature we now use the JoinedRealm to determine LAN status

	if( JoinedRealm.empty( ) )
		Player->SetSpoofed( true );

	Player->SetWhoisShouldBeSent( AnyAdminCheck );
	m_Players.push_back( Player );
	potential->SetSocket( NULL );
	potential->SetDeleteMe( true );

	if( m_Map->GetMapOptions( ) & MAPOPT_CUSTOMFORCES )
		m_Slots[SID] = CGameSlot( Player->GetPID( ), 255, SLOTSTATUS_OCCUPIED, 0, m_Slots[SID].GetTeam( ), m_Slots[SID].GetColour( ), m_Slots[SID].GetRace( ) );
	else
	{
		if( m_Map->GetMapFlags( ) & MAPFLAG_RANDOMRACES )
			m_Slots[SID] = CGameSlot( Player->GetPID( ), 255, SLOTSTATUS_OCCUPIED, 0, 12, 12, SLOTRACE_RANDOM );
		else
			m_Slots[SID] = CGameSlot( Player->GetPID( ), 255, SLOTSTATUS_OCCUPIED, 0, 12, 12, SLOTRACE_RANDOM | SLOTRACE_SELECTABLE );

		// try to pick a team and colour
		// make sure there aren't too many other players already

		unsigned char NumOtherPlayers = 0;

		for( unsigned char i = 0; i < m_Slots.size( ); ++i )
		{
			if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[i].GetTeam( ) != 12 )
					++NumOtherPlayers;
		}

		if( NumOtherPlayers < m_Map->GetMapNumPlayers( ) )
		{
			if( SID < m_Map->GetMapNumPlayers( ) )
				m_Slots[SID].SetTeam( SID );
			else
				m_Slots[SID].SetTeam( 0 );

				m_Slots[SID].SetColour( GetNewColour( ) );
		}
	}	

	// send slot info to the new player
	// the SLOTINFOJOIN packet also tells the client their assigned PID and that the join was successful

	Player->Send( m_Protocol->SEND_W3GS_SLOTINFOJOIN( Player->GetPID( ), Player->GetSocket( )->GetPort( ), Player->GetExternalIP( ), m_Slots, m_RandomSeed, m_Map->GetMapLayoutStyle( ), m_Map->GetMapNumPlayers( ) ) );

	// send virtual host info and fake player info (if present) to the new player

	SendVirtualHostPlayerInfo( Player );
	SendFakePlayerInfo( Player );

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( !(*i)->GetLeftMessageSent( ) && *i != Player )
		{
			// send info about the new player to every other player

			if( (*i)->GetSocket( ) )
				(*i)->Send( m_Protocol->SEND_W3GS_PLAYERINFO( Player->GetPID( ), Player->GetName( ), Player->GetExternalIP( ), Player->GetInternalIP( ) ) );

			// send info about every other player to the new player
			Player->Send( m_Protocol->SEND_W3GS_PLAYERINFO( (*i)->GetPID( ), (*i)->GetName( ), (*i)->GetExternalIP( ), (*i)->GetInternalIP( ) ) );
		}
	}

	// send a map check packet to the new player

	Player->Send( m_Protocol->SEND_W3GS_MAPCHECK( m_Map->GetMapPath( ), m_Map->GetMapSize( ), m_Map->GetMapInfo( ), m_Map->GetMapCRC( ), m_Map->GetMapSHA1( ) ) );

	// send slot info to everyone, so the new player gets this info twice but everyone else still needs to know the new slot layout

	SendAllSlotInfo( );

	// check for multiple IP usage

	string Others;

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( Player != *i && Player->GetExternalIPString( ) == (*i)->GetExternalIPString( ) )
		{
			if( Others.empty( ) )
				Others = (*i)->GetName( );
			else
				Others += ", " + (*i)->GetName( );
		}
	}

	if( !Others.empty( ) )
		SendAllChat( m_GHost->m_Language->MultipleIPAddressUsageDetected( joinPlayer->GetName( ), Others ) );

	// abort the countdown if there was one in progress

	if( m_CountDownStarted && !m_GameLoading && !m_GameLoaded )
	{
		SendAllChat( m_GHost->m_Language->CountDownAborted( ) );
		m_CountDownStarted = false;
	}

	// auto lock the game

	if( m_GHost->m_AutoLock && !m_Locked && IsOwner( joinPlayer->GetName( ) ) )
	{
		SendAllChat( m_GHost->m_Language->GameLocked( ) );
		m_Locked = true;
	}
}

void CBaseGame :: EventPlayerLeft( CGamePlayer *player, uint32_t reason )
{
	// this function is only called when a player leave packet is received, not when there's a socket error, kick, etc...

	player->SetDeleteMe( true );

	if( reason == PLAYERLEAVE_GPROXY )
		player->SetLeftReason( m_GHost->m_Language->WasUnrecoverablyDroppedFromGProxy( ) );
	else
		player->SetLeftReason( m_GHost->m_Language->HasLeftVoluntarily( ) );

	player->SetLeftCode( PLAYERLEAVE_LOST );

	if( !m_GameLoading && !m_GameLoaded )
		OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
}

void CBaseGame :: EventPlayerLoaded( CGamePlayer *player )
{
	CONSOLE_Print( "[GAME: " + m_GameName + "] player [" + player->GetName( ) + "] finished loading in " + UTIL_ToString( (float)( player->GetFinishedLoadingTicks( ) - m_StartedLoadingTicks ) / 1000.f, 2 ) + " seconds" );

	if( m_LoadInGame )
	{
		// send any buffered data to the player now
		// see the Update function for more information about why we do this
		// this includes player loaded messages, game updates, and player leave messages

		queue<BYTEARRAY> *LoadInGameData = player->GetLoadInGameData( );

		while( !LoadInGameData->empty( ) )
		{
			Send( player, LoadInGameData->front( ) );
			LoadInGameData->pop( );
		}

		// start the lag screen for the new player

		bool FinishedLoading = true;

		for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		{
			FinishedLoading = (*i)->GetFinishedLoading( );

			if( !FinishedLoading )
				break;
		}

		if( !FinishedLoading )
			Send( player, m_Protocol->SEND_W3GS_START_LAG( m_Players, true ) );

		// remove the new player from previously loaded players' lag screens

		for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		{
			if( *i != player && (*i)->GetFinishedLoading( ) )
				Send( *i, m_Protocol->SEND_W3GS_STOP_LAG( player ) );
		}

		// send a chat message to previously loaded players

		for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		{
			if( *i != player && (*i)->GetFinishedLoading( ) )
				SendChat( *i, m_GHost->m_Language->PlayerFinishedLoading( player->GetName( ) ) );
		}

		if( !FinishedLoading )
			SendChat( player, m_GHost->m_Language->PleaseWaitPlayersStillLoading( ) );
	}
	else
		SendAll( m_Protocol->SEND_W3GS_GAMELOADED_OTHERS( player->GetPID( ) ) );
}

void CBaseGame :: EventPlayerAction( CGamePlayer *player, CIncomingAction *action )
{
	m_Actions.push( action );

	// check for players saving the game and notify everyone

	if( !action->GetAction( )->empty( ) && (*action->GetAction( ))[0] == 6 )
	{
		CONSOLE_Print( "[GAME: " + m_GameName + "] player [" + player->GetName( ) + "] is saving the game" );
		SendAllChat( m_GHost->m_Language->PlayerIsSavingTheGame( player->GetName( ) ) );
	}
}

void CBaseGame :: EventPlayerKeepAlive( CGamePlayer *player, uint32_t checkSum )
{
	// check for desyncs
	// however, it's possible that not every player has sent a checksum for this frame yet
	// first we verify that we have enough checksums to work with otherwise we won't know exactly who desynced

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( !(*i)->GetDeleteMe( ) && (*i)->GetCheckSums( )->empty( ) )
			return;
	}

	// now we check for desyncs since we know that every player has at least one checksum waiting

	bool FoundPlayer = false;
	uint32_t FirstCheckSum = 0;

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( !(*i)->GetDeleteMe( ) )
		{
			FoundPlayer = true;
			FirstCheckSum = (*i)->GetCheckSums( )->front( );
			break;
		}
	}

	if( !FoundPlayer )
		return;

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( !(*i)->GetDeleteMe( ) && (*i)->GetCheckSums( )->front( ) != FirstCheckSum )
		{
			CONSOLE_Print( "[GAME: " + m_GameName + "] desync detected" );
			SendAllChat( m_GHost->m_Language->DesyncDetected( ) );

			// try to figure out who desynced
			// this is complicated by the fact that we don't know what the correct game state is so we let the players vote
			// put the players into bins based on their game state

			map<uint32_t, vector<unsigned char> > Bins;

			for( vector<CGamePlayer *> :: iterator j = m_Players.begin( ); j != m_Players.end( ); ++j )
			{
				if( !(*j)->GetDeleteMe( ) )
					Bins[(*j)->GetCheckSums( )->front( )].push_back( (*j)->GetPID( ) );
			}

			uint32_t StateNumber = 1;
			map<uint32_t, vector<unsigned char> > :: iterator LargestBin = Bins.begin( );
			bool Tied = false;

			for( map<uint32_t, vector<unsigned char> > :: iterator j = Bins.begin( ); j != Bins.end( ); ++j )
			{
				if( (*j).second.size( ) > (*LargestBin).second.size( ) )
				{
					LargestBin = j;
					Tied = false;
				}
				else if( j != LargestBin && (*j).second.size( ) == (*LargestBin).second.size( ) )
					Tied = true;

				string Players;

				for( vector<unsigned char> :: iterator k = (*j).second.begin( ); k != (*j).second.end( ); ++k )
				{
					CGamePlayer *Player = GetPlayerFromPID( *k );

					if( Player )
					{
						if( Players.empty( ) )
							Players = Player->GetName( );
						else
							Players += ", " + Player->GetName( );
					}
				}

				SendAllChat( m_GHost->m_Language->PlayersInGameState( UTIL_ToString( StateNumber ), Players ) );
				++StateNumber;
			}

			FirstCheckSum = (*LargestBin).first;

			if( Tied )
			{
				// there is a tie, which is unfortunate
				// the most common way for this to happen is with a desync in a 1v1 situation
				// this is not really unsolvable since the game shouldn't continue anyway so we just kick both players
				// in a 2v2 or higher the chance of this happening is very slim
				// however, we still kick every player because it's not fair to pick one or another group
				// todotodo: it would be possible to split the game at this point and create a "new" game for each game state

				CONSOLE_Print( "[GAME: " + m_GameName + "] can't kick desynced players because there is a tie, kicking all players instead" );
				StopPlayers( m_GHost->m_Language->WasDroppedDesync( ) );
			}
			else
			{
				CONSOLE_Print( "[GAME: " + m_GameName + "] kicking desynced players" );

				for( map<uint32_t, vector<unsigned char> > :: iterator j = Bins.begin( ); j != Bins.end( ); ++j )
				{
					// kick players who are NOT in the largest bin
					// examples: suppose there are 10 players
					// the most common case will be 9v1 (e.g. one player desynced and the others were unaffected) and this will kick the single outlier
					// another (very unlikely) possibility is 8v1v1 or 8v2 and this will kick both of the outliers, regardless of whether their game states match

					if( (*j).first != (*LargestBin).first )
					{
						for( vector<unsigned char> :: iterator k = (*j).second.begin( ); k != (*j).second.end( ); ++k )
						{
							CGamePlayer *Player = GetPlayerFromPID( *k );

							if( Player )
							{
								Player->SetDeleteMe( true );
								Player->SetLeftReason( m_GHost->m_Language->WasDroppedDesync( ) );
								Player->SetLeftCode( PLAYERLEAVE_LOST );
							}
						}
					}
				}
			}

			// don't continue looking for desyncs, we already found one!

			break;
		}
	}

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( !(*i)->GetDeleteMe( ) )
			(*i)->GetCheckSums( )->pop( );
	}
}

void CBaseGame :: EventPlayerChatToHost( CGamePlayer *player, CIncomingChatPlayer *chatPlayer )
{
	if( chatPlayer->GetFromPID( ) == player->GetPID( ) )
	{
		if( chatPlayer->GetType( ) == CIncomingChatPlayer :: CTH_MESSAGE || chatPlayer->GetType( ) == CIncomingChatPlayer :: CTH_MESSAGEEXTRA )
		{
			// relay the chat message to other players

			bool Relay = !player->GetMuted( );
			BYTEARRAY ExtraFlags = chatPlayer->GetExtraFlags( );

			// calculate timestamp

			string MinString = UTIL_ToString( ( m_GameTicks / 1000 ) / 60 );
			string SecString = UTIL_ToString( ( m_GameTicks / 1000 ) % 60 );

			if( MinString.size( ) == 1 )
				MinString.insert( 0, "0" );

			if( SecString.size( ) == 1 )
				SecString.insert( 0, "0" );

			if( !ExtraFlags.empty( ) )
			{
				if( ExtraFlags[0] == 1 )
				{
					CONSOLE_Print( "[GAME: " + m_GameName + "] (" + MinString + ":" + SecString + ") [Allies] [" + player->GetName( ) + "]: " + chatPlayer->GetMessage( ) );
				}
				else if( ExtraFlags[0] == 0 )
				{
					// this is an ingame [All] message, print it to the console

					CONSOLE_Print( "[GAME: " + m_GameName + "] (" + MinString + ":" + SecString + ") [All] [" + player->GetName( ) + "]: " + chatPlayer->GetMessage( ) );

					// don't relay ingame messages targeted for all players if we're currently muting all
					// note that commands will still be processed even when muting all because we only stop relaying the messages, the rest of the function is unaffected

					if( m_MuteAll )
						Relay = false;
				}
				else if( ExtraFlags[0] == 2 )
				{
					// this is an ingame [Obs/Ref] message, print it to the console

					CONSOLE_Print( "[GAME: " + m_GameName + "] (" + MinString + ":" + SecString + ") [Obs/Ref] [" + player->GetName( ) + "]: " + chatPlayer->GetMessage( ) );
				}
			}
			else
			{
				// this is a lobby message, print it to the console

				CONSOLE_Print( "[GAME: " + m_GameName + "] [Lobby] [" + player->GetName( ) + "]: " + chatPlayer->GetMessage( ) );

				if( m_MuteLobby )
					Relay = false;
			}

			// handle bot commands

			string Message = chatPlayer->GetMessage( );

			if( !Message.empty( ) && ( Message[0] == m_GHost->m_CommandTrigger || Message[0] == '/' ) )
			{
				// extract the command trigger, the command, and the payload
				// e.g. "!say hello world" -> command: "say", payload: "hello world"

				string Command;
				string Payload;
				string :: size_type PayloadStart = Message.find( " " );

				if( PayloadStart != string :: npos )
				{
					Command = Message.substr( 1, PayloadStart - 1 );
					Payload = Message.substr( PayloadStart + 1 );
				}
				else
					Command = Message.substr( 1 );

				transform( Command.begin( ), Command.end( ), Command.begin( ), (int(*)(int))tolower );

				// don't allow EventPlayerBotCommand to veto a previous instruction to set Relay to false
				// so if Relay is already false (e.g. because the player is muted) then it cannot be forced back to true here

				EventPlayerBotCommand( player, Command, Payload );
				Relay = false;
			}

			if( Relay )
				Send( chatPlayer->GetToPIDs( ), m_Protocol->SEND_W3GS_CHAT_FROM_HOST( chatPlayer->GetFromPID( ), chatPlayer->GetToPIDs( ), chatPlayer->GetFlag( ), chatPlayer->GetExtraFlags( ), chatPlayer->GetMessage( ) ) );
		}
		else if( chatPlayer->GetType( ) == CIncomingChatPlayer :: CTH_TEAMCHANGE && !m_CountDownStarted )
			EventPlayerChangeTeam( player, chatPlayer->GetByte( ) );
		else if( chatPlayer->GetType( ) == CIncomingChatPlayer :: CTH_COLOURCHANGE && !m_CountDownStarted )
			EventPlayerChangeColour( player, chatPlayer->GetByte( ) );
		else if( chatPlayer->GetType( ) == CIncomingChatPlayer :: CTH_RACECHANGE && !m_CountDownStarted )
			EventPlayerChangeRace( player, chatPlayer->GetByte( ) );
		else if( chatPlayer->GetType( ) == CIncomingChatPlayer :: CTH_HANDICAPCHANGE && !m_CountDownStarted )
			EventPlayerChangeHandicap( player, chatPlayer->GetByte( ) );
	}
}

bool CBaseGame :: EventPlayerBotCommand( CGamePlayer *player, string &command, string &payload )
{
	return true;
}

void CBaseGame :: EventPlayerChangeTeam( CGamePlayer *player, unsigned char team )
{
	// player is requesting a team change

	if( m_Map->GetMapOptions( ) & MAPOPT_CUSTOMFORCES )
	{
		unsigned char oldSID = GetSIDFromPID( player->GetPID( ) );
		unsigned char newSID = GetEmptySlot( team, player->GetPID( ) );
		SwapSlots( oldSID, newSID );
	}
	else
	{
		if( team > 12 )
			return;

		if( team == 12 )
		{
			if( m_Map->GetMapObservers( ) != MAPOBS_ALLOWED && m_Map->GetMapObservers( ) != MAPOBS_REFEREES )
				return;
		}
		else
		{
			if( team >= m_Map->GetMapNumPlayers( ) )
				return;

			// make sure there aren't too many other players already

			unsigned char NumOtherPlayers = 0;

			for( unsigned char i = 0; i < m_Slots.size( ); ++i )
			{
				if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[i].GetTeam( ) != 12 && m_Slots[i].GetPID( ) != player->GetPID( ) )
					++NumOtherPlayers;
			}

			if( NumOtherPlayers >= m_Map->GetMapNumPlayers( ) )
				return;
		}

		unsigned char SID = GetSIDFromPID( player->GetPID( ) );

		if( SID < m_Slots.size( ) )
		{
			m_Slots[SID].SetTeam( team );

			if( team == 12 )
			{
				// if they're joining the observer team give them the observer colour

				m_Slots[SID].SetColour( 12 );
			}
			else if( m_Slots[SID].GetColour( ) == 12 )
			{
				// if they're joining a regular team give them an unused colour

				m_Slots[SID].SetColour( GetNewColour( ) );
			}

			SendAllSlotInfo( );
		}
	}
}

void CBaseGame :: EventPlayerChangeColour( CGamePlayer *player, unsigned char colour )
{
	// player is requesting a colour change

	if( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS )
		return;

	if( colour > 11 )
		return;

	unsigned char SID = GetSIDFromPID( player->GetPID( ) );

	if( SID < m_Slots.size( ) )
	{
		// make sure the player isn't an observer

		if( m_Slots[SID].GetTeam( ) == 12 )
			return;

		ColourSlot( SID, colour );
	}
}

void CBaseGame :: EventPlayerChangeRace( CGamePlayer *player, unsigned char race )
{
	// player is requesting a race change

	if( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS )
		return;

	if( m_Map->GetMapFlags( ) & MAPFLAG_RANDOMRACES )
		return;

	if( race != SLOTRACE_HUMAN && race != SLOTRACE_ORC && race != SLOTRACE_NIGHTELF && race != SLOTRACE_UNDEAD && race != SLOTRACE_RANDOM )
		return;

	unsigned char SID = GetSIDFromPID( player->GetPID( ) );

	if( SID < m_Slots.size( ) )
	{
		m_Slots[SID].SetRace( race | SLOTRACE_SELECTABLE );
		SendAllSlotInfo( );
	}
}

void CBaseGame :: EventPlayerChangeHandicap( CGamePlayer *player, unsigned char handicap )
{
	// player is requesting a handicap change

	if( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS )
		return;

	if( handicap != 50 && handicap != 60 && handicap != 70 && handicap != 80 && handicap != 90 && handicap != 100 )
		return;

	unsigned char SID = GetSIDFromPID( player->GetPID( ) );

	if( SID < m_Slots.size( ) )
	{
		m_Slots[SID].SetHandicap( handicap );
		SendAllSlotInfo( );
	}
}

void CBaseGame :: EventPlayerDropRequest( CGamePlayer *player )
{
	// todotodo: check that we've waited the full 45 seconds

	if( m_Lagging )
	{
		CONSOLE_Print( "[GAME: " + m_GameName + "] player [" + player->GetName( ) + "] voted to drop laggers" );
		SendAllChat( m_GHost->m_Language->PlayerVotedToDropLaggers( player->GetName( ) ) );

		// check if at least half the players voted to drop

		uint32_t Votes = 0;

		for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		{
			if( (*i)->GetDropVote( ) )
				++Votes;
		}

		if( (float)Votes / m_Players.size( ) > 0.50f )
			StopLaggers( m_GHost->m_Language->LaggedOutDroppedByVote( ) );
	}
}

void CBaseGame :: EventPlayerMapSize( CGamePlayer *player, CIncomingMapSize *mapSize )
{
	if( m_GameLoading || m_GameLoaded )
		return;

	// todotodo: the variable names here are confusing due to extremely poor design on my part

	uint32_t MapSize = UTIL_ByteArrayToUInt32( m_Map->GetMapSize( ), false );

	bool Admin = IsAdmin( player->GetName( ) );

	if( mapSize->GetSizeFlag( ) != 1 || mapSize->GetMapSize( ) != MapSize )
	{
		// the player doesn't have the map

		if( Admin || m_GHost->m_AllowDownloads != 0 )
		{
			string *MapData = m_Map->GetMapData( );

			if( !MapData->empty( ) )
			{
				if( Admin || m_GHost->m_AllowDownloads == 1 || ( m_GHost->m_AllowDownloads == 2 && player->GetDownloadAllowed( ) ) )
				{
					if( !player->GetDownloadStarted( ) && mapSize->GetSizeFlag( ) == 1 )
					{
						// inform the client that we are willing to send the map

						CONSOLE_Print( "[GAME: " + m_GameName + "] map download started for player [" + player->GetName( ) + "]" );
						Send( player, m_Protocol->SEND_W3GS_STARTDOWNLOAD( GetHostPID( ) ) );
						player->SetDownloadStarted( true );
						player->SetStartedDownloadingTicks( GetTicks( ) );
					}
					else
						player->SetLastMapPartAcked( mapSize->GetMapSize( ) );
				}
			}
			else
			{
				player->SetDeleteMe( true );
				player->SetLeftReason( "doesn't have the map and there is no local copy of the map to send" );
				player->SetLeftCode( PLAYERLEAVE_LOBBY );
				OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
			}
		}
		else
		{
			player->SetDeleteMe( true );
			player->SetLeftReason( "doesn't have the map and map downloads are disabled" );
			player->SetLeftCode( PLAYERLEAVE_LOBBY );
			OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
		}
	}
	else if( player->GetDownloadStarted( ) )
	{
		// calculate download rate

		float Seconds = (float)( GetTicks( ) - player->GetStartedDownloadingTicks( ) ) / 1000.f;
		float Rate = (float)MapSize / 1024.f / Seconds;
		CONSOLE_Print( "[GAME: " + m_GameName + "] map download finished for player [" + player->GetName( ) + "] in " + UTIL_ToString( Seconds, 1 ) + " seconds" );
		SendAllChat( m_GHost->m_Language->PlayerDownloadedTheMap( player->GetName( ), UTIL_ToString( Seconds, 1 ), UTIL_ToString( Rate, 1 ) ) );
		player->SetDownloadFinished( true );
		player->SetFinishedDownloadingTime( GetTime( ) );
	}

	unsigned char NewDownloadStatus = (unsigned char)( (float)mapSize->GetMapSize( ) / MapSize * 100.f );
	unsigned char SID = GetSIDFromPID( player->GetPID( ) );

	if( NewDownloadStatus > 100 )
		NewDownloadStatus = 100;

	if( SID < m_Slots.size( ) )
	{
		// only send the slot info if the download status changed

		if( m_Slots[SID].GetDownloadStatus( ) != NewDownloadStatus )
		{
			m_Slots[SID].SetDownloadStatus( NewDownloadStatus );

			// we don't actually send the new slot info here
			// this is an optimization because it's possible for a player to download a map very quickly
			// if we send a new slot update for every percentage change in their download status it adds up to a lot of data
			// instead, we mark the slot info as "out of date" and update it only once in awhile (once per second when this comment was made)

			m_SlotInfoChanged = true;
		}
	}
}

void CBaseGame :: EventPlayerPongToHost( CGamePlayer *player, uint32_t pong )
{
	// autokick players with excessive pings but only if they're not reserved and we've received at least 3 pings from them
	// also don't kick anyone if the game is loading or loaded - this could happen because we send pings during loading but we stop sending them after the game is loaded
	// see the Update function for where we send pings

	if( !m_GameLoading && !m_GameLoaded && !player->GetDeleteMe( ) && !player->GetReserved( ) && player->GetNumPings( ) >= 3 && player->GetPing( m_GHost->m_LCPings ) > m_GHost->m_AutoKickPing )
	{
		// send a chat message because we don't normally do so when a player leaves the lobby

		SendAllChat( m_GHost->m_Language->AutokickingPlayerForExcessivePing( player->GetName( ), UTIL_ToString( player->GetPing( m_GHost->m_LCPings ) ) ) );
		player->SetDeleteMe( true );
		player->SetLeftReason( "was autokicked for excessive ping of " + UTIL_ToString( player->GetPing( m_GHost->m_LCPings ) ) );
		player->SetLeftCode( PLAYERLEAVE_LOBBY );
		OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
	}
}

void CBaseGame :: EventGameRefreshed( const string &server )
{
	if( m_RefreshRehosted )
	{
		// we're not actually guaranteed this refresh was for the rehosted game and not the previous one
		// but since we unqueue game refreshes when rehosting, the only way this can happen is due to network delay
		// it's a risk we're willing to take but can result in a false positive here

		SendAllChat( m_GHost->m_Language->RehostWasSuccessful( ) );
		m_RefreshRehosted = false;
	}
}

void CBaseGame :: EventGameStarted( )
{
	CONSOLE_Print( "[GAME: " + m_GameName + "] started loading with " + UTIL_ToString( GetNumHumanPlayers( ) ) + " players" );

	// encode the HCL command string in the slot handicaps
	// here's how it works:
	//  the user inputs a command string to be sent to the map
	//  it is almost impossible to send a message from the bot to the map so we encode the command string in the slot handicaps
	//  this works because there are only 6 valid handicaps but Warcraft III allows the bot to set up to 256 handicaps
	//  we encode the original (unmodified) handicaps in the new handicaps and use the remaining space to store a short message
	//  only occupied slots deliver their handicaps to the map and we can send one character (from a list) per handicap
	//  when the map finishes loading, assuming it's designed to use the HCL system, it checks if anyone has an invalid handicap
	//  if so, it decodes the message from the handicaps and restores the original handicaps using the encoded values
	//  the meaning of the message is specific to each map and the bot doesn't need to understand it
	//  e.g. you could send game modes, # of rounds, level to start on, anything you want as long as it fits in the limited space available
	//  note: if you attempt to use the HCL system on a map that does not support HCL the bot will drastically modify the handicaps
	//  since the map won't automatically restore the original handicaps in this case your game will be ruined

	if( !m_HCLCommandString.empty( ) )
	{
		if( m_HCLCommandString.size( ) <= GetSlotsOccupied( ) )
		{
			string HCLChars = "abcdefghijklmnopqrstuvwxyz0123456789 -=,.";

			if( m_HCLCommandString.find_first_not_of( HCLChars ) == string :: npos )
			{
				unsigned char EncodingMap[256];
				unsigned char j = 0;

				for( uint32_t i = 0; i < 256; ++i )
				{
					// the following 7 handicap values are forbidden

					if( j == 0 || j == 50 || j == 60 || j == 70 || j == 80 || j == 90 || j == 100 )
						++j;

					EncodingMap[i] = j++;
				}

				unsigned char CurrentSlot = 0;

				for( string :: iterator si = m_HCLCommandString.begin( ); si != m_HCLCommandString.end( ); ++si )
				{
					while( m_Slots[CurrentSlot].GetSlotStatus( ) != SLOTSTATUS_OCCUPIED )
						++CurrentSlot;

					unsigned char HandicapIndex = ( m_Slots[CurrentSlot].GetHandicap( ) - 50 ) / 10;
					unsigned char CharIndex = HCLChars.find( *si );
					m_Slots[CurrentSlot++].SetHandicap( EncodingMap[HandicapIndex + CharIndex * 6] );
				}

				SendAllSlotInfo( );
				CONSOLE_Print( "[GAME: " + m_GameName + "] successfully encoded HCL command string [" + m_HCLCommandString + "]" );
			}
			else
				CONSOLE_Print( "[GAME: " + m_GameName + "] encoding HCL command string [" + m_HCLCommandString + "] failed because it contains invalid characters" );
		}
		else
			CONSOLE_Print( "[GAME: " + m_GameName + "] encoding HCL command string [" + m_HCLCommandString + "] failed because there aren't enough occupied slots" );
	}

	// send a final slot info update if necessary
	// this typically won't happen because we prevent the !start command from completing while someone is downloading the map
	// however, if someone uses !start force while a player is downloading the map this could trigger
	// this is because we only permit slot info updates to be flagged when it's just a change in download status, all others are sent immediately
	// it might not be necessary but let's clean up the mess anyway

	if( m_SlotInfoChanged )
		SendAllSlotInfo( );

	m_StartedLoadingTicks = GetTicks( );
	m_LastLagScreenResetTime = GetTime( );
	m_GameLoading = true;

	// since we use a fake countdown to deal with leavers during countdown the COUNTDOWN_START and COUNTDOWN_END packets are sent in quick succession
	// send a start countdown packet

	SendAll( m_Protocol->SEND_W3GS_COUNTDOWN_START( ) );

	// remove the virtual host player

	DeleteVirtualHost( );

	// send an end countdown packet

	SendAll( m_Protocol->SEND_W3GS_COUNTDOWN_END( ) );

	// send a game loaded packet for the fake player (if present)

	for( vector<unsigned char> :: iterator i = m_FakePlayers.begin( ); i != m_FakePlayers.end( ); ++i )
		SendAll( m_Protocol->SEND_W3GS_GAMELOADED_OTHERS( *i ) );

	// record the starting number of players

	m_StartPlayers = GetNumHumanPlayers( );

	// close the listening socket

	delete m_Socket;
	m_Socket = NULL;

	// delete any potential players that are still hanging around

	for( vector<CPotentialPlayer *> :: iterator i = m_Potentials.begin( ); i != m_Potentials.end( ); ++i )
		delete *i;

	m_Potentials.clear( );

	// delete the map data

	delete m_Map;
	m_Map = NULL;

	if( m_LoadInGame )
	{
		// buffer all the player loaded messages
		// this ensures that every player receives the same set of player loaded messages in the same order, even if someone leaves during loading
		// if someone leaves during loading we buffer the leave message to ensure it gets sent in the correct position but the player loaded message wouldn't get sent if we didn't buffer it now

		for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		{
			for( vector<CGamePlayer *> :: iterator j = m_Players.begin( ); j != m_Players.end( ); ++j )
				(*j)->AddLoadInGameData( m_Protocol->SEND_W3GS_GAMELOADED_OTHERS( (*i)->GetPID( ) ) );
		}
	}

	// move the game to the games in progress vector

	m_GHost->m_CurrentGame = NULL;
	m_GHost->m_Games.push_back( this );

	// and finally reenter battle.net chat

	for( vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
	{
		(*i)->QueueGameUncreate( );
		(*i)->QueueEnterChat( );
	}
}

void CBaseGame :: EventGameLoaded( )
{
	CONSOLE_Print( "[GAME: " + m_GameName + "] finished loading with " + UTIL_ToString( GetNumHumanPlayers( ) ) + " players" );

	// send shortest, longest, and personal load times to each player

	CGamePlayer *Shortest = NULL;
	CGamePlayer *Longest = NULL;

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( !Shortest || (*i)->GetFinishedLoadingTicks( ) < Shortest->GetFinishedLoadingTicks( ) )
			Shortest = *i;

		if( !Longest || (*i)->GetFinishedLoadingTicks( ) > Longest->GetFinishedLoadingTicks( ) )
			Longest = *i;
	}

	if( Shortest && Longest )
	{
		SendAllChat( m_GHost->m_Language->ShortestLoadByPlayer( Shortest->GetName( ), UTIL_ToString( (float)( Shortest->GetFinishedLoadingTicks( ) - m_StartedLoadingTicks ) / 1000, 2 ) ) );
		SendAllChat( m_GHost->m_Language->LongestLoadByPlayer( Longest->GetName( ), UTIL_ToString( (float)( Longest->GetFinishedLoadingTicks( ) - m_StartedLoadingTicks ) / 1000, 2 ) ) );
	}

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		SendChat( *i, m_GHost->m_Language->YourLoadingTimeWas( UTIL_ToString( (float)( (*i)->GetFinishedLoadingTicks( ) - m_StartedLoadingTicks ) / 1000, 2 ) ) );
}

unsigned char CBaseGame :: GetSIDFromPID( unsigned char PID )
{
	if( m_Slots.size( ) > 255 )
		return 255;

	for( unsigned char i = 0; i < m_Slots.size( ); ++i )
	{
		if( m_Slots[i].GetPID( ) == PID )
			return i;
	}

	return 255;
}

CGamePlayer *CBaseGame :: GetPlayerFromPID( unsigned char PID )
{
	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( !(*i)->GetLeftMessageSent( ) && (*i)->GetPID( ) == PID )
			return *i;
	}

	return NULL;
}

CGamePlayer *CBaseGame :: GetPlayerFromSID( unsigned char SID )
{
	if( SID < m_Slots.size( ) )
		return GetPlayerFromPID( m_Slots[SID].GetPID( ) );

	return NULL;
}

CGamePlayer *CBaseGame :: GetPlayerFromName( string name, bool sensitive )
{
	if( !sensitive )
		transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( !(*i)->GetLeftMessageSent( ) )
		{
			string TestName = (*i)->GetName( );

			if( !sensitive )
				transform( TestName.begin( ), TestName.end( ), TestName.begin( ), (int(*)(int))tolower );

			if( TestName == name )
				return *i;
		}
	}

	return NULL;
}

uint32_t CBaseGame :: GetPlayerFromNamePartial( string name, CGamePlayer **player )
{
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );
	uint32_t Matches = 0;
	*player = NULL;

	// try to match each player with the passed string (e.g. "Varlock" would be matched with "lock")

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( !(*i)->GetLeftMessageSent( ) )
		{
			string TestName = (*i)->GetName( );
			transform( TestName.begin( ), TestName.end( ), TestName.begin( ), (int(*)(int))tolower );

			if( TestName.find( name ) != string :: npos )
			{
				++Matches;
				*player = *i;

				// if the name matches exactly stop any further matching

				if( TestName == name )
				{
					Matches = 1;
					break;
				}
			}
		}
	}

	return Matches;
}

CGamePlayer *CBaseGame :: GetPlayerFromColour( unsigned char colour )
{
	for( unsigned char i = 0; i < m_Slots.size( ); ++i )
	{
		if( m_Slots[i].GetColour( ) == colour )
			return GetPlayerFromSID( i );
	}

	return NULL;
}

unsigned char CBaseGame :: GetNewPID( )
{
	// find an unused PID for a new player to use

	for( unsigned char TestPID = 1; TestPID < 255; ++TestPID )
	{
		if( TestPID == m_VirtualHostPID )
			continue;

		bool InUse = false;

		for( vector<unsigned char> :: iterator i = m_FakePlayers.begin( ); i != m_FakePlayers.end( ); ++i )
		{
			if( *i == TestPID )
			{
				InUse = true;
				break;
			}
		}

		if( InUse )
			continue;

		for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		{
			if( !(*i)->GetLeftMessageSent( ) && (*i)->GetPID( ) == TestPID )
			{
				InUse = true;
				break;
			}
		}

		if( !InUse )
			return TestPID;
	}

	// this should never happen

	return 255;
}

unsigned char CBaseGame :: GetNewColour( )
{
	// find an unused colour for a player to use

	for( unsigned char TestColour = 0; TestColour < 12; ++TestColour )
	{
		bool InUse = false;

		for( unsigned char i = 0; i < m_Slots.size( ); ++i )
		{
			if( m_Slots[i].GetColour( ) == TestColour )
			{
				InUse = true;
				break;
			}
		}

		if( !InUse )
			return TestColour;
	}

	// this should never happen

	return 12;
}

BYTEARRAY CBaseGame :: GetPIDs( )
{
	BYTEARRAY result;

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( !(*i)->GetLeftMessageSent( ) )
			result.push_back( (*i)->GetPID( ) );
	}

	return result;
}

BYTEARRAY CBaseGame :: GetPIDs( unsigned char excludePID )
{
	BYTEARRAY result;

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( !(*i)->GetLeftMessageSent( ) && (*i)->GetPID( ) != excludePID )
			result.push_back( (*i)->GetPID( ) );
	}

	return result;
}

unsigned char CBaseGame :: GetHostPID( )
{
	// return the player to be considered the host (it can be any player) - mainly used for sending text messages from the bot
	// try to find the virtual host player first

	if( m_VirtualHostPID != 255 )
		return m_VirtualHostPID;

	// try to find the fakeplayer next

	if( m_FakePlayers.size( ) )
		return m_FakePlayers[0];

	// try to find the owner player next

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( !(*i)->GetLeftMessageSent( ) && IsOwner( (*i)->GetName( ) ) )
			return (*i)->GetPID( );
	}

	// okay then, just use the first available player

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( !(*i)->GetLeftMessageSent( ) )
			return (*i)->GetPID( );
	}

	return 255;
}

unsigned char CBaseGame :: GetEmptySlot( bool reserved )
{
	if( m_Slots.size( ) > 255 )
		return 255;
	
	// look for an empty slot for a new player to occupy
	// if reserved is true then we're willing to use closed or occupied slots as long as it wouldn't displace a player with a reserved slot

	for( unsigned char i = 0; i < m_Slots.size( ); ++i )
	{
		if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN )
			return i;
	}

	if( reserved )
	{
		// no empty slots, but since player is reserved give them a closed slot

		for( unsigned char i = 0; i < m_Slots.size( ); ++i )
		{
			if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_CLOSED )
				return i;
		}

		// no closed slots either, give them an occupied slot but not one occupied by another reserved player
		// first look for a player who is downloading the map and has the least amount downloaded so far

		unsigned char LeastDownloaded = 100;
		unsigned char LeastSID = 255;

		for( unsigned char i = 0; i < m_Slots.size( ); ++i )
		{
			CGamePlayer *Player = GetPlayerFromSID( i );

			if( Player && !Player->GetReserved( ) && m_Slots[i].GetDownloadStatus( ) < LeastDownloaded )
			{
				LeastDownloaded = m_Slots[i].GetDownloadStatus( );
				LeastSID = i;
			}
		}

		if( LeastSID != 255 )
			return LeastSID;

		// nobody who isn't reserved is downloading the map, just choose the first player who isn't reserved

		for( unsigned char i = 0; i < m_Slots.size( ); ++i )
		{
			CGamePlayer *Player = GetPlayerFromSID( i );

			if( Player && !Player->GetReserved( ) )
				return i;
		}
	}

	return 255;
}

unsigned char CBaseGame :: GetEmptySlot( unsigned char team, unsigned char PID )
{
	if( m_Slots.size( ) > 255 )
		return 255;

	// find an empty slot based on player's current slot

	unsigned char StartSlot = GetSIDFromPID( PID );

	if( StartSlot < m_Slots.size( ) )
	{
		if( m_Slots[StartSlot].GetTeam( ) != team )
		{
			// player is trying to move to another team so start looking from the first slot on that team
			// we actually just start looking from the very first slot since the next few loops will check the team for us

			StartSlot = 0;
		}
		
		// find an empty slot on the correct team starting from StartSlot

		for( unsigned char i = StartSlot; i < m_Slots.size( ); ++i )
		{
			if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN && m_Slots[i].GetTeam( ) == team )
				return i;
		}

		// didn't find an empty slot, but we could have missed one with SID < StartSlot
		// e.g. in the DotA case where I am in slot 4 (yellow), slot 5 (orange) is occupied, and slot 1 (blue) is open and I am trying to move to another slot

		for( unsigned char i = 0; i < StartSlot; ++i )
		{
			if( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN && m_Slots[i].GetTeam( ) == team )
				return i;
		}
	}

	return 255;
}

void CBaseGame :: SwapSlots( unsigned char SID1, unsigned char SID2 )
{
	if( SID1 < m_Slots.size( ) && SID2 < m_Slots.size( ) && SID1 != SID2 )
	{
		CGameSlot Slot1 = m_Slots[SID1];
		CGameSlot Slot2 = m_Slots[SID2];

		if( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS )
		{
			// don't swap the team, colour, race, or handicap
			m_Slots[SID1] = CGameSlot( Slot2.GetPID( ), Slot2.GetDownloadStatus( ), Slot2.GetSlotStatus( ), Slot2.GetComputer( ), Slot1.GetTeam( ), Slot1.GetColour( ), Slot1.GetRace( ), Slot2.GetComputerType( ), Slot1.GetHandicap( ) );
			m_Slots[SID2] = CGameSlot( Slot1.GetPID( ), Slot1.GetDownloadStatus( ), Slot1.GetSlotStatus( ), Slot1.GetComputer( ), Slot2.GetTeam( ), Slot2.GetColour( ), Slot2.GetRace( ), Slot1.GetComputerType( ), Slot2.GetHandicap( ) );
		}
		else
		{
			// swap everything

			if( m_Map->GetMapOptions( ) & MAPOPT_CUSTOMFORCES )
			{
				// except if custom forces is set, then we don't swap teams...
				Slot1.SetTeam( m_Slots[SID2].GetTeam( ) );
				Slot2.SetTeam( m_Slots[SID1].GetTeam( ) );
			}

			m_Slots[SID1] = Slot2;
			m_Slots[SID2] = Slot1;
		}

		SendAllSlotInfo( );
	}
}

void CBaseGame :: OpenSlot( unsigned char SID, bool kick )
{
	if( SID < m_Slots.size( ) )
	{
		if( kick )
		{
			CGamePlayer *Player = GetPlayerFromSID( SID );

			if( Player )
			{
				Player->SetDeleteMe( true );
				Player->SetLeftReason( "was kicked when opening a slot" );
				Player->SetLeftCode( PLAYERLEAVE_LOBBY );
			}
		}

		CGameSlot Slot = m_Slots[SID];
		m_Slots[SID] = CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, Slot.GetTeam( ), Slot.GetColour( ), Slot.GetRace( ) ); 
		SendAllSlotInfo( );
	}
}

void CBaseGame :: CloseSlot( unsigned char SID, bool kick )
{
	if( SID < m_Slots.size( ) )
	{
		if( kick )
		{
			CGamePlayer *Player = GetPlayerFromSID( SID );

			if( Player )
			{
				Player->SetDeleteMe( true );
				Player->SetLeftReason( "was kicked when closing a slot" );
				Player->SetLeftCode( PLAYERLEAVE_LOBBY );
			}
		}

		CGameSlot Slot = m_Slots[SID];
		m_Slots[SID] = CGameSlot( 0, 255, SLOTSTATUS_CLOSED, 0, Slot.GetTeam( ), Slot.GetColour( ), Slot.GetRace( ) ); 
		SendAllSlotInfo( );
	}
}

void CBaseGame :: ComputerSlot( unsigned char SID, unsigned char skill, bool kick )
{
	if( SID < m_Slots.size( ) && skill < 3 )
	{
		if( kick )
		{
			CGamePlayer *Player = GetPlayerFromSID( SID );

			if( Player )
			{
				Player->SetDeleteMe( true );
				Player->SetLeftReason( "was kicked when creating a computer in a slot" );
				Player->SetLeftCode( PLAYERLEAVE_LOBBY );
			}
		}

		CGameSlot Slot = m_Slots[SID];
		m_Slots[SID] = CGameSlot( 0, 100, SLOTSTATUS_OCCUPIED, 1, Slot.GetTeam( ), Slot.GetColour( ), Slot.GetRace( ), skill );
		SendAllSlotInfo( );
	}
}

void CBaseGame :: ColourSlot( unsigned char SID, unsigned char colour )
{
	if( SID < m_Slots.size( ) && colour < 12 )
	{
		// make sure the requested colour isn't already taken

		bool Taken = false;
		unsigned char TakenSID = 0;

		for( unsigned char i = 0; i < m_Slots.size( ); ++i )
		{
			if( m_Slots[i].GetColour( ) == colour )
			{
				TakenSID = i;
				Taken = true;
			}
		}

		if( Taken && m_Slots[TakenSID].GetSlotStatus( ) != SLOTSTATUS_OCCUPIED )
		{
			// the requested colour is currently "taken" by an unused (open or closed) slot
			// but we allow the colour to persist within a slot so if we only update the existing player's colour the unused slot will have the same colour
			// this isn't really a problem except that if someone then joins the game they'll receive the unused slot's colour resulting in a duplicate
			// one way to solve this (which we do here) is to swap the player's current colour into the unused slot

			m_Slots[TakenSID].SetColour( m_Slots[SID].GetColour( ) );
			m_Slots[SID].SetColour( colour );
			SendAllSlotInfo( );
		}
		else if( !Taken )
		{
			// the requested colour isn't used by ANY slot

			m_Slots[SID].SetColour( colour );
			SendAllSlotInfo( );
		}
	}
}

void CBaseGame :: OpenAllSlots( )
{
	bool Changed = false;

	for( vector<CGameSlot> :: iterator i = m_Slots.begin( ); i != m_Slots.end( ); ++i )
	{
		if( (*i).GetSlotStatus( ) == SLOTSTATUS_CLOSED )
		{
			(*i).SetSlotStatus( SLOTSTATUS_OPEN );
			Changed = true;
		}
	}

	if( Changed )
		SendAllSlotInfo( );
}

void CBaseGame :: CloseAllSlots( )
{
	bool Changed = false;

	for( vector<CGameSlot> :: iterator i = m_Slots.begin( ); i != m_Slots.end( ); ++i )
	{
		if( (*i).GetSlotStatus( ) == SLOTSTATUS_OPEN )
		{
			(*i).SetSlotStatus( SLOTSTATUS_CLOSED );
			Changed = true;
		}
	}

	if( Changed )
		SendAllSlotInfo( );
}

void CBaseGame :: ShuffleSlots( )
{
	// we only want to shuffle the player slots
	// that means we need to prevent this function from shuffling the open/closed/computer slots too
	// so we start by copying the player slots to a temporary vector

	vector<CGameSlot> PlayerSlots;

	for( vector<CGameSlot> :: iterator i = m_Slots.begin( ); i != m_Slots.end( ); ++i )
	{
		if( (*i).GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && (*i).GetComputer( ) == 0 && (*i).GetTeam( ) != 12 )
			PlayerSlots.push_back( *i );
	}

	// now we shuffle PlayerSlots

	if( m_Map->GetMapOptions( ) & MAPOPT_CUSTOMFORCES )
	{
		// rather than rolling our own probably broken shuffle algorithm we use random_shuffle because it's guaranteed to do it properly
		// so in order to let random_shuffle do all the work we need a vector to operate on
		// unfortunately we can't just use PlayerSlots because the team/colour/race shouldn't be modified
		// so make a vector we can use

		vector<unsigned char> SIDs;

		for( unsigned char i = 0; i < PlayerSlots.size( ); ++i )
			SIDs.push_back( i );

		random_shuffle( SIDs.begin( ), SIDs.end( ) );

		// now put the PlayerSlots vector in the same order as the SIDs vector

		vector<CGameSlot> Slots;

		// as usual don't modify the team/colour/race

		for( unsigned char i = 0; i < SIDs.size( ); ++i )
			Slots.push_back( CGameSlot( PlayerSlots[SIDs[i]].GetPID( ), PlayerSlots[SIDs[i]].GetDownloadStatus( ), PlayerSlots[SIDs[i]].GetSlotStatus( ), PlayerSlots[SIDs[i]].GetComputer( ), PlayerSlots[i].GetTeam( ), PlayerSlots[i].GetColour( ), PlayerSlots[i].GetRace( ) ) );

		PlayerSlots = Slots;
	}
	else
	{
		// regular game
		// it's easy when we're allowed to swap the team/colour/race!

		random_shuffle( PlayerSlots.begin( ), PlayerSlots.end( ) );
	}

	// now we put m_Slots back together again

	vector<CGameSlot> :: iterator CurrentPlayer = PlayerSlots.begin( );
	vector<CGameSlot> Slots;

	for( vector<CGameSlot> :: iterator i = m_Slots.begin( ); i != m_Slots.end( ); ++i )
	{
		if( (*i).GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && (*i).GetComputer( ) == 0 && (*i).GetTeam( ) != 12 )
		{
			Slots.push_back( *CurrentPlayer );
			++CurrentPlayer;
		}
		else
			Slots.push_back( *i );
	}

	m_Slots = Slots;

	// and finally tell everyone about the new slot configuration

	SendAllSlotInfo( );
}

void CBaseGame :: AddToSpoofed( string server, const string &name, bool sendMessage )
{
	CGamePlayer *Player = GetPlayerFromName( name, true );

	if( Player )
	{
		Player->SetSpoofedRealm( server );
		Player->SetSpoofed( true );

		if( sendMessage )
			SendAllChat( m_GHost->m_Language->SpoofCheckAcceptedFor( server, name ) );
	}
}

void CBaseGame :: AddToReserved( string name )
{
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );

	// check that the user is not already reserved

	for( vector<string> :: iterator i = m_Reserved.begin( ); i != m_Reserved.end( ); ++i )
	{
		if( *i == name )
			return;
	}

	m_Reserved.push_back( name );

	// upgrade the user if they're already in the game

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		string NameLower = (*i)->GetName( );
		transform( NameLower.begin( ), NameLower.end( ), NameLower.begin( ), (int(*)(int))tolower );

		if( NameLower == name )
			(*i)->SetReserved( true );
	}
}

bool CBaseGame :: IsOwner( string name )
{
	string OwnerLower = m_OwnerName;
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );
	transform( OwnerLower.begin( ), OwnerLower.end( ), OwnerLower.begin( ), (int(*)(int))tolower );
	return name == OwnerLower;
}

bool CBaseGame :: IsReserved( string name )
{
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );

	for( vector<string> :: iterator i = m_Reserved.begin( ); i != m_Reserved.end( ); ++i )
	{
		if( *i == name )
			return true;
	}

	return false;
}

bool CBaseGame :: IsDownloading( )
{
	// returns true if at least one player is downloading the map

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( (*i)->GetDownloadStarted( ) && !(*i)->GetDownloadFinished( ) )
			return true;
	}

	return false;
}

bool CBaseGame :: IsGameDataSaved( )
{
	return true;
}

void CBaseGame :: SaveGameData( )
{

}

void CBaseGame :: StartCountDown( bool force )
{
	if( !m_CountDownStarted )
	{
		if( force )
		{
			m_CountDownStarted = true;
			m_CountDownCounter = 5;
		}
		else
		{
			// check if the HCL command string is short enough

			if( m_HCLCommandString.size( ) > GetSlotsOccupied( ) )
			{
				SendAllChat( m_GHost->m_Language->TheHCLIsTooLongUseForceToStart( ) );
				return;
			}

			// check if everyone has the map

			string StillDownloading;

			for( vector<CGameSlot> :: iterator i = m_Slots.begin( ); i != m_Slots.end( ); ++i )
			{
				if( (*i).GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && (*i).GetComputer( ) == 0 && (*i).GetDownloadStatus( ) != 100 )
				{
					CGamePlayer *Player = GetPlayerFromPID( (*i).GetPID( ) );

					if( Player )
					{
						if( StillDownloading.empty( ) )
							StillDownloading = Player->GetName( );
						else
							StillDownloading += ", " + Player->GetName( );
					}
				}
			}

			if( !StillDownloading.empty( ) )
				SendAllChat( m_GHost->m_Language->PlayersStillDownloading( StillDownloading ) );

			// check if everyone has been pinged enough (3 times) that the autokicker would have kicked them by now
			// see function EventPlayerPongToHost for the autokicker code

			string NotPinged;

			for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
			{
				if( !(*i)->GetReserved( ) && (*i)->GetNumPings( ) < 3 )
				{
					if( NotPinged.empty( ) )
						NotPinged = (*i)->GetName( );
					else
						NotPinged += ", " + (*i)->GetName( );
				}
			}

			if( !NotPinged.empty( ) )
				SendAllChat( m_GHost->m_Language->PlayersNotYetPinged( NotPinged ) );

			// if no problems found start the game

			if( StillDownloading.empty( ) && NotPinged.empty( ) )
			{
				m_CountDownStarted = true;
				m_CountDownCounter = 5;
			}
		}
	}
}

void CBaseGame :: StopPlayers( string reason )
{
	// disconnect every player and set their left reason to the passed string
	// we use this function when we want the code in the Update function to run before the destructor (e.g. saving players to the database)
	// therefore calling this function when m_GameLoading || m_GameLoaded is roughly equivalent to setting m_Exiting = true
	// the only difference is whether the code in the Update function is executed or not

	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		(*i)->SetDeleteMe( true );
		(*i)->SetLeftReason( reason );
		(*i)->SetLeftCode( PLAYERLEAVE_LOST );
	}
}

void CBaseGame :: StopLaggers( string reason )
{
	for( vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
	{
		if( (*i)->GetLagging( ) )
		{
			(*i)->SetDeleteMe( true );
			(*i)->SetLeftReason( reason );
			(*i)->SetLeftCode( PLAYERLEAVE_DISCONNECT );
		}
	}
}

void CBaseGame :: CreateVirtualHost( )
{
	if( m_VirtualHostPID != 255 )
		return;

	m_VirtualHostPID = GetNewPID( );
	BYTEARRAY IP;
	IP.push_back( 0 );
	IP.push_back( 0 );
	IP.push_back( 0 );
	IP.push_back( 0 );
	SendAll( m_Protocol->SEND_W3GS_PLAYERINFO( m_VirtualHostPID, m_VirtualHostName, IP, IP ) );
}

void CBaseGame :: DeleteVirtualHost( )
{
	if( m_VirtualHostPID == 255 )
		return;

	SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( m_VirtualHostPID, PLAYERLEAVE_LOBBY ) );
	m_VirtualHostPID = 255;
}

void CBaseGame :: CreateFakePlayer( )
{
	if( m_FakePlayers.size( ) > 10 )
			return;

	unsigned char SID = GetEmptySlot( false );

	if( SID < m_Slots.size( ) )
	{
		if( GetNumPlayers( ) >= 11 )
			DeleteVirtualHost( );

		unsigned char FakePlayerPID = GetNewPID( );

		BYTEARRAY IP;
		IP.push_back( 0 );
		IP.push_back( 0 );
		IP.push_back( 0 );
		IP.push_back( 0 );

		
		SendAll( m_Protocol->SEND_W3GS_PLAYERINFO( FakePlayerPID, "Fake[" + UTIL_ToString( FakePlayerPID ) + "]", IP, IP ) );
		m_Slots[SID] = CGameSlot( FakePlayerPID, 100, SLOTSTATUS_OCCUPIED, 0, m_Slots[SID].GetTeam( ), m_Slots[SID].GetColour( ), m_Slots[SID].GetRace( ) );
		m_FakePlayers.push_back( FakePlayerPID );
		SendAllSlotInfo( );
	}
}

void CBaseGame :: DeleteFakePlayers( )
{
	if( m_FakePlayers.empty( ) )
		return;

	for( unsigned char i = 0; i < m_Slots.size( ); ++i )
	{
		for( vector<unsigned char> :: iterator j = m_FakePlayers.begin( ); j != m_FakePlayers.end( ); ++j )
		{
			if( m_Slots[i].GetPID( ) == *j )
			{
				m_Slots[i] = CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, m_Slots[i].GetTeam( ), m_Slots[i].GetColour( ), m_Slots[i].GetRace( ) );
				SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( *j, PLAYERLEAVE_LOBBY ) );
			}
		}
	}
		
	m_FakePlayers.clear( );
	SendAllSlotInfo( );
}

inline bool CBaseGame :: IsAdmin( string name )
{
	for( vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
	{
		if( (*i)->IsAdmin( name ) || (*i)->IsRootAdmin( name ) )
		{
			return true;
		}
	}

	return false;
}
