/*

   Copyright [2010] [Josko Nikolic]

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

#include "aura.h"
#include "util.h"
#include "config.h"
#include "socket.h"
#include "auradb.h"
#include "bnet.h"
#include "map.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "game.h"
#include "stats.h"
#include "irc.h"

#include <ctime>
#include <cmath>
#include <cstdlib>

//
// sorting classes
//

class CGamePlayerSortAscByPing
{
public:

  bool operator( ) ( CGamePlayer *Player1, CGamePlayer *Player2 ) const
  {
    return Player1->GetPing( false ) < Player2->GetPing( false );
  }
};

class CGamePlayerSortDescByPing
{
public:

  bool operator( ) ( CGamePlayer *Player1, CGamePlayer *Player2 ) const
  {
    return Player1->GetPing( false ) > Player2->GetPing( false );
  }
};

//
// CGame
//

CGame::CGame( CAura *nAura, CMap *nMap, uint16_t nHostPort, unsigned char nGameState, string &nGameName, string &nOwnerName, string &nCreatorName, string &nCreatorServer ) : m_Aura( nAura ), m_DBBanLast( NULL ), m_GameID( 0 ), m_Slots( nMap->GetSlots( ) ), m_Exiting( false ), m_Saving( false ), m_HostPort( nHostPort ), m_GameState( nGameState ), m_VirtualHostPID( 255 ), m_GameName( nGameName ), m_LastGameName( nGameName ), m_VirtualHostName( nAura->m_VirtualHostName ), m_OwnerName( nOwnerName ), m_CreatorName( nCreatorName ), m_CreatorServer( nCreatorServer ), m_HCLCommandString( nMap->GetMapDefaultHCL( ) ), m_MapPath(nMap->GetMapPath( )), m_RandomSeed( GetTicks( ) ), m_HostCounter( nAura->m_HostCounter++ ), m_EntryKey( rand( ) ), m_Latency( nAura->m_Latency ), m_SyncLimit( nAura->m_SyncLimit ), m_SyncCounter( 0 ), m_GameTicks( 0 ), m_CreationTime( GetTime( ) ), m_LastPingTime( GetTime( ) ), m_LastRefreshTime( GetTime( ) ), m_LastDownloadTicks( GetTime( ) ), m_DownloadCounter( 0 ), m_LastDownloadCounterResetTicks( GetTicks( ) ), m_LastCountDownTicks( 0 ), m_CountDownCounter( 0 ), m_StartedLoadingTicks( 0 ), m_StartPlayers( 0 ), m_LastLagScreenResetTime( 0 ), m_LastActionSentTicks( 0 ), m_LastActionLateBy( 0 ), m_StartedLaggingTime( 0 ), m_LastLagScreenTime( 0 ), m_LastReservedSeen( GetTime( ) ), m_StartedKickVoteTime( 0 ), m_GameOverTime( 0 ), m_LastPlayerLeaveTicks( 0 ), m_SlotInfoChanged( false ), m_Locked( false ), m_RefreshError( false ), m_MuteAll( false ), m_MuteLobby( false ), m_CountDownStarted( false ), m_GameLoading( false ), m_GameLoaded( false ), m_Lagging( false ), m_Desynced( false )
{
  m_Socket = new CTCPServer( );
  m_Protocol = new CGameProtocol( m_Aura );
  m_Map = new CMap( *nMap );

  // wait time of 1 minute  = 0 empty actions required
  // wait time of 2 minutes = 1 empty action required...

  m_GProxyEmptyActions = m_Aura->m_ReconnectWaitTime - 1;

  // clamp to 9 empty actions (10 minutes)

  if ( m_GProxyEmptyActions > 9 )
    m_GProxyEmptyActions = 9;

  // start listening for connections

  if ( !m_Aura->m_BindAddress.empty( ) )
    Print( "[GAME: " + m_GameName + "] attempting to bind to address [" + m_Aura->m_BindAddress + "]" );

  if ( m_Socket->Listen( m_Aura->m_BindAddress, m_HostPort ) )
    Print( "[GAME: " + m_GameName + "] listening on port " + UTIL_ToString( m_HostPort ) );
  else
  {
    Print2( "[GAME: " + m_GameName + "] error listening on port " + UTIL_ToString( m_HostPort ) );    
    m_Exiting = true;
  }

  if ( m_Map->GetMapType( ) == "dota" )
    m_Stats = new CStats( this );
  else
    m_Stats = NULL;
}

CGame::~CGame( )
{
  delete m_Socket;
  delete m_Protocol;
  delete m_Map;

  for ( vector<CPotentialPlayer *> ::iterator i = m_Potentials.begin( ); i != m_Potentials.end( ); ++i )
    delete *i;

  for ( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
    delete *i;

  while ( !m_Actions.empty( ) )
  {
    delete m_Actions.front( );
    m_Actions.pop( );
  }

  // store the CDBGamePlayers in the database

  if ( m_GameID )
  {
    for ( vector<CDBGamePlayer *> ::iterator i = m_DBGamePlayers.begin( ); i != m_DBGamePlayers.end( ); ++i )
      m_Aura->m_DB->GamePlayerAdd( m_GameID, (*i)->GetName( ), (*i)->GetIP( ), (*i)->GetSpoofed( ), (*i)->GetSpoofedRealm( ), (*i)->GetReserved( ), (*i)->GetLoadingTime( ), (*i)->GetLeft( ), (*i)->GetLeftReason( ), (*i)->GetTeam( ), (*i)->GetColour( ) );

    // store the stats in the database

    if ( m_Stats )
      m_Stats->Save( m_Aura, m_Aura->m_DB, m_GameID );
  }
  else
    Print( "[GAME: " + m_GameName + "] unable to save player/stats data to database" );

  for ( vector<CDBBan *> ::iterator i = m_DBBans.begin( ); i != m_DBBans.end( ); ++i )
    delete *i;

  for ( vector<CDBGamePlayer *> ::iterator i = m_DBGamePlayers.begin( ); i != m_DBGamePlayers.end( ); ++i )
    delete *i;

  delete m_Stats;
}

uint32_t CGame::GetNextTimedActionTicks( ) const
{
  // return the number of ticks (ms) until the next "timed action", which for our purposes is the next game update
  // the main Aura++ loop will make sure the next loop update happens at or before this value
  // note: there's no reason this function couldn't take into account the game's other timers too but they're far less critical
  // warning: this function must take into account when actions are not being sent (e.g. during loading or lagging)

  if ( !m_GameLoaded || m_Lagging )
    return 50;

  uint32_t TicksSinceLastUpdate = GetTicks( ) - m_LastActionSentTicks;

  if ( TicksSinceLastUpdate > m_Latency - m_LastActionLateBy )
    return 0;
  else
    return m_Latency - m_LastActionLateBy - TicksSinceLastUpdate;
}

uint32_t CGame::GetSlotsOccupied( ) const
{
  uint32_t NumSlotsOccupied = 0;

  for ( vector<CGameSlot> ::const_iterator i = m_Slots.begin( ); i != m_Slots.end( ); ++i )
  {
    if ( (*i).GetSlotStatus( ) == SLOTSTATUS_OCCUPIED )
      ++NumSlotsOccupied;
  }

  return NumSlotsOccupied;
}

uint32_t CGame::GetSlotsOpen( ) const
{
  uint32_t NumSlotsOpen = 0;

  for ( vector<CGameSlot> ::const_iterator i = m_Slots.begin( ); i != m_Slots.end( ); ++i )
  {
    if ( (*i).GetSlotStatus( ) == SLOTSTATUS_OPEN )
      ++NumSlotsOpen;
  }

  return NumSlotsOpen;
}

uint32_t CGame::GetNumPlayers( ) const
{
  return GetNumHumanPlayers( ) + m_FakePlayers.size( );
}

uint32_t CGame::GetNumHumanPlayers( ) const
{
  uint32_t NumHumanPlayers = 0;

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    if ( !(*i)->GetLeftMessageSent( ) )
      ++NumHumanPlayers;
  }

  return NumHumanPlayers;
}

string CGame::GetDescription( ) const
{
  string Description = m_GameName + " : " + m_OwnerName + " : " + UTIL_ToString( GetNumHumanPlayers( ) ) + "/" + UTIL_ToString( m_GameLoading || m_GameLoaded ? m_StartPlayers : m_Slots.size( ) );

  if ( m_GameLoading || m_GameLoaded )
    Description += " : " + UTIL_ToString( ( m_GameTicks / 1000 ) / 60 ) + "m";
  else
    Description += " : " + UTIL_ToString( ( GetTime( ) - m_CreationTime ) / 60 ) + "m";

  return Description;
}

string CGame::GetPlayers( ) const
{
  string Players;

  for ( unsigned int i = 0; i < m_Players.size( ); ++i )
  {
    if ( !m_Players[i]->GetLeftMessageSent( ) )
      Players += m_Players[i]->GetName( ) + ", ";
  }

  if ( Players.size( ) > 2 )
    Players = Players.substr( 0, Players.size( ) - 2 );

  return Players;
}

unsigned int CGame::SetFD( void *fd, void *send_fd, int *nfds )
{
  unsigned int NumFDs = 0;

  if ( m_Socket )
  {
    m_Socket->SetFD( (fd_set *) fd, (fd_set *) send_fd, nfds );
    ++NumFDs;
  }

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
      (*i)->GetSocket( )->SetFD( (fd_set *) fd, (fd_set *) send_fd, nfds );
      ++NumFDs;
  }

  for ( vector<CPotentialPlayer *> ::const_iterator i = m_Potentials.begin( ); i != m_Potentials.end( ); ++i )
  {
    if ( (*i)->GetSocket( ) )
    {
      (*i)->GetSocket( )->SetFD( (fd_set *) fd, (fd_set *) send_fd, nfds );
      ++NumFDs;
    }
  }

  return NumFDs;
}

bool CGame::Update( void *fd, void *send_fd )
{
  uint32_t Time = GetTime( ), Ticks = GetTicks( );

  // ping every 5 seconds
  // changed this to ping during game loading as well to hopefully fix some problems with people disconnecting during loading
  // changed this to ping during the game as well

  if ( Time - m_LastPingTime >= 5 )
  {
    // note: we must send pings to players who are downloading the map because Warcraft III disconnects from the lobby if it doesn't receive a ping every ~90 seconds
    // so if the player takes longer than 90 seconds to download the map they would be disconnected unless we keep sending pings

    SendAll( m_Protocol->SEND_W3GS_PING_FROM_HOST( ) );

    // we also broadcast the game to the local network every 5 seconds so we hijack this timer for our nefarious purposes
    // however we only want to broadcast if the countdown hasn't started
    // see the !sendlan code later in this file for some more information about how this works

    if ( !m_CountDownStarted )
    {
      // construct a fixed host counter which will be used to identify players from this "realm" (i.e. LAN)
      // the fixed host counter's 4 most significant bits will contain a 4 bit ID (0-15)
      // the rest of the fixed host counter will contain the 28 least significant bits of the actual host counter
      // since we're destroying 4 bits of information here the actual host counter should not be greater than 2^28 which is a reasonable assumption
      // when a player joins a game we can obtain the ID from the received host counter
      // note: LAN broadcasts use an ID of 0, battle.net refreshes use an ID of 1-10, the rest are unused

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

      m_Aura->m_UDPSocket->Broadcast( 6112, m_Protocol->SEND_W3GS_GAMEINFO( m_Aura->m_LANWar3Version, UTIL_CreateByteArray( (uint32_t) MAPGAMETYPE_UNKNOWN0, false ), m_Map->GetMapGameFlags( ), m_Map->GetMapWidth( ), m_Map->GetMapHeight( ), m_GameName, "Clan 007", 0, m_Map->GetMapPath( ), m_Map->GetMapCRC( ), 12, 12, m_HostPort, m_HostCounter & 0x0FFFFFFF, m_EntryKey ) );
    }

    m_LastPingTime = Time;
  }

  // update players

  for ( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); )
  {
    if ( (*i)->Update( fd ) )
    {
      EventPlayerDeleted(*i);
      delete *i;
      i = m_Players.erase( i );
    }
    else
      ++i;
  }

  for ( vector<CPotentialPlayer *> ::iterator i = m_Potentials.begin( ); i != m_Potentials.end( ); )
  {
    if ( (*i)->Update( fd ) )
    {
      // flush the socket (e.g. in case a rejection message is queued)

      if ( (*i)->GetSocket( ) )
        (*i)->GetSocket( )->DoSend( (fd_set *) send_fd );

      delete *i;
      i = m_Potentials.erase( i );
    }
    else
      ++i;
  }

  // keep track of the largest sync counter (the number of keepalive packets received by each player)
  // if anyone falls behind by more than m_SyncLimit keepalives we start the lag screen

  if ( m_GameLoaded )
  {
    // check if anyone has started lagging
    // we consider a player to have started lagging if they're more than m_SyncLimit keepalives behind

    if ( !m_Lagging )
    {
      string LaggingString;

      for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
      {
        if ( m_SyncCounter - (*i)->GetSyncCounter( ) > m_SyncLimit )
        {
          (*i)->SetLagging( true );
          (*i)->SetStartedLaggingTicks( Ticks );
          m_Lagging = true;
          m_StartedLaggingTime = Time;

          if ( LaggingString.empty( ) )
            LaggingString = (*i)->GetName( );
          else
            LaggingString += ", " + (*i)->GetName( );
        }
      }

      if ( m_Lagging )
      {
        // start the lag screen

        Print( "[GAME: " + m_GameName + "] started lagging on [" + LaggingString + "]" );
        SendAll( m_Protocol->SEND_W3GS_START_LAG( m_Players ) );

        // reset everyone's drop vote

        for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
          (*i)->SetDropVote( false );

        m_LastLagScreenResetTime = Time;
      }
    }

    if ( m_Lagging )
    {
      bool UsingGProxy = false;

      for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
      {
        if ( (*i)->GetGProxy( ) )   
        {   
          UsingGProxy = true;
          break;  
        }   
      }
      
      uint32_t WaitTime = 60;
      
      if ( UsingGProxy )
        WaitTime = ( m_GProxyEmptyActions + 1 ) * 60;

      if ( Time - m_StartedLaggingTime >= WaitTime )
        StopLaggers( "was automatically dropped after " + UTIL_ToString( WaitTime ) + " seconds" );

      // we cannot allow the lag screen to stay up for more than ~65 seconds because Warcraft III disconnects if it doesn't receive an action packet at least this often
      // one (easy) solution is to simply drop all the laggers if they lag for more than 60 seconds
      // another solution is to reset the lag screen the same way we reset it when using load-in-game

      if ( Time - m_LastLagScreenResetTime >= 60 )
      {
        for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
        {
          // stop the lag screen

          for ( vector<CGamePlayer *> ::const_iterator j = m_Players.begin( ); j != m_Players.end( ); ++j )
          {
            if ( (*j)->GetLagging( ) )
              Send( *i, m_Protocol->SEND_W3GS_STOP_LAG(*j) );
          }

          // send an empty update
          // this resets the lag screen timer

          if ( UsingGProxy && !(*i)->GetGProxy( ) )
          {
            // we must send additional empty actions to non-GProxy++ players
            // GProxy++ will insert these itself so we don't need to send them to GProxy++ players
            // empty actions are used to extend the time a player can use when reconnecting
            
            for ( unsigned char j = 0; j < m_GProxyEmptyActions; ++j )
              Send( *i, m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );
          }

          Send( *i, m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );

          // start the lag screen

          Send( *i, m_Protocol->SEND_W3GS_START_LAG( m_Players ) );
        }

        // Warcraft III doesn't seem to respond to empty actions

        m_LastLagScreenResetTime = Time;
      }

      // check if anyone has stopped lagging normally
      // we consider a player to have stopped lagging if they're less than half m_SyncLimit keepalives behind

      for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
      {
        if ( (*i)->GetLagging( ) && m_SyncCounter - (*i)->GetSyncCounter( ) < m_SyncLimit / 2 )
        {
          // stop the lag screen for this player

          Print( "[GAME: " + m_GameName + "] stopped lagging on [" + (*i)->GetName( ) + "]" );
          SendAll( m_Protocol->SEND_W3GS_STOP_LAG(*i) );
          (*i)->SetLagging( false );
          (*i)->SetStartedLaggingTicks( 0 );
        }
      }

      // check if everyone has stopped lagging

      bool Lagging = false;

      for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
      {
        if ( (*i)->GetLagging( ) )
        {
          Lagging = true;
          break;
        }
      }

      m_Lagging = Lagging;

      // reset m_LastActionSentTicks because we want the game to stop running while the lag screen is up

      m_LastActionSentTicks = Ticks;

      // keep track of the last lag screen time so we can avoid timing out players

      m_LastLagScreenTime = Time;
    }
  }

  // send actions every m_Latency milliseconds
  // actions are at the heart of every Warcraft 3 game but luckily we don't need to know their contents to relay them
  // we queue player actions in EventPlayerAction then just resend them in batches to all players here

  if ( m_GameLoaded && !m_Lagging && Ticks - m_LastActionSentTicks >= m_Latency - m_LastActionLateBy )
    SendAllActions( );

  // end the game if there aren't any players left

  if ( m_Players.empty( ) && ( m_GameLoading || m_GameLoaded ) )
  {
    if ( !m_Saving )
    {
      Print( "[GAME: " + m_GameName + "] is over (no players left)" );
      Print( "[GAME: " + m_GameName + "] saving game data to database" );

      m_Saving = true;

      m_GameID = m_Aura->m_DB->GameAdd( m_Aura->m_BNETs.size( ) == 1 ? m_Aura->m_BNETs[0]->GetServer( ) : string( ), m_MapPath, m_GameName, m_OwnerName, m_GameTicks / 1000, m_GameState, m_CreatorName, m_CreatorServer );
    }
    else if ( m_GameID )
      return true;
  }

  // check if the game is loaded

  if ( m_GameLoading )
  {
    bool FinishedLoading = true;

    for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
    {
      FinishedLoading = (*i)->GetFinishedLoading( );

      if ( !FinishedLoading )
        break;
    }

    if ( FinishedLoading )
    {
      m_LastActionSentTicks = Ticks;
      m_GameLoading = false;
      m_GameLoaded = true;
      EventGameLoaded( );
    }
  }

  // start the gameover timer if there's only one player left

  if ( m_Players.size( ) == 1 && m_FakePlayers.empty( ) && m_GameOverTime == 0 && ( m_GameLoading || m_GameLoaded ) )
  {
    Print( "[GAME: " + m_GameName + "] gameover timer started (one player left)" );
    m_GameOverTime = Time;
  }

  // finish the gameover timer

  if ( m_GameOverTime != 0 && Time - m_GameOverTime >= 60 )
  {
    for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
    {
      if ( !(*i)->GetDeleteMe( ) )
      {
        Print( "[GAME: " + m_GameName + "] is over (gameover timer finished)" );
        StopPlayers( "was disconnected (gameover timer finished)" );
        break;
      }
    }
  }

  // expire the votekick

  if ( !m_KickVotePlayer.empty( ) && Time - m_StartedKickVoteTime >= 60 )
  {
    Print( "[GAME: " + m_GameName + "] votekick against player [" + m_KickVotePlayer + "] expired" );
    SendAllChat( "A votekick against player [" + m_KickVotePlayer + "] has expired" );
    m_KickVotePlayer.clear( );
    m_StartedKickVoteTime = 0;
  }

  if ( m_GameLoaded )
    return m_Exiting;

  // refresh every 3 seconds

  if ( !m_RefreshError && !m_CountDownStarted && m_GameState == GAME_PUBLIC && GetSlotsOpen( ) > 0 && Time - m_LastRefreshTime >= 3 )
  {
    // send a game refresh packet to each battle.net connection

    for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
    {
      // don't queue a game refresh message if the queue contains more than 1 packet because they're very low priority

      if ( (*i)->GetOutPacketsQueued( ) <= 1 )
      {
        (*i)->QueueGameRefresh( m_GameState, m_GameName, m_Map, m_HostCounter );
      }
    }

    m_LastRefreshTime = Time;
  }

  // send more map data

  if ( !m_GameLoading && !m_GameLoaded && Ticks - m_LastDownloadCounterResetTicks >= 1000 )
  {
    // hackhack: another timer hijack is in progress here
    // since the download counter is reset once per second it's a great place to update the slot info if necessary

    if ( m_SlotInfoChanged )
      SendAllSlotInfo( );

    m_DownloadCounter = 0;
    m_LastDownloadCounterResetTicks = Ticks;
  }

  if ( !m_GameLoading && Ticks - m_LastDownloadTicks >= 100 )
  {
    uint32_t Downloaders = 0;

    for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
    {
      if ( (*i)->GetDownloadStarted( ) && !(*i)->GetDownloadFinished( ) )
      {
        ++Downloaders;

        if ( m_Aura->m_MaxDownloaders > 0 && Downloaders > m_Aura->m_MaxDownloaders )
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

        while ( (*i)->GetLastMapPartSent( ) < (*i)->GetLastMapPartAcked( ) + 1442 * 100 && (*i)->GetLastMapPartSent( ) < MapSize )
        {
          if ( (*i)->GetLastMapPartSent( ) == 0 )
          {
            // overwrite the "started download ticks" since this is the first time we've sent any map data to the player
            // prior to this we've only determined if the player needs to download the map but it's possible we could have delayed sending any data due to download limits

            (*i)->SetStartedDownloadingTicks( Ticks );
          }

          // limit the download speed if we're sending too much data
          // the download counter is the # of map bytes downloaded in the last second (it's reset once per second)

          if ( m_Aura->m_MaxDownloadSpeed > 0 && m_DownloadCounter > m_Aura->m_MaxDownloadSpeed * 1024 )
            break;

          Send( *i, m_Protocol->SEND_W3GS_MAPPART( GetHostPID( ), (*i)->GetPID( ), (*i)->GetLastMapPartSent( ), m_Map->GetMapData( ) ) );
          (*i)->SetLastMapPartSent( (*i)->GetLastMapPartSent( ) + 1442 );
          m_DownloadCounter += 1442;
        }
      }
    }

    m_LastDownloadTicks = Ticks;
  }

  // countdown every 500 ms

  if ( m_CountDownStarted && Ticks - m_LastCountDownTicks >= 500 )
  {
    if ( m_CountDownCounter > 0 )
    {
      // we use a countdown counter rather than a "finish countdown time" here because it might alternately round up or down the count
      // this sometimes resulted in a countdown of e.g. "6 5 3 2 1" during my testing which looks pretty dumb
      // doing it this way ensures it's always "5 4 3 2 1" but each interval might not be *exactly* the same length

      SendAllChat( UTIL_ToString( m_CountDownCounter-- ) + ". . ." );
    }
    else if ( !m_GameLoading && !m_GameLoaded )
      EventGameStarted( );

    m_LastCountDownTicks = Ticks;
  }

  // check if the lobby is "abandoned" and needs to be closed since it will never start

  if ( !m_GameLoading && !m_GameLoaded && m_Aura->m_LobbyTimeLimit > 0 )
  {
    // check if there's a player with reserved status in the game

    for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
    {
      if ( (*i)->GetReserved( ) )
      {
        m_LastReservedSeen = Time;
        break;
      }
    }

    // check if we've hit the time limit

    if ( Time - m_LastReservedSeen > m_Aura->m_LobbyTimeLimit * 60 )
    {
      Print( "[GAME: " + m_GameName + "] is over (lobby time limit hit)" );

      for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
      {
        if ( !(*i)->GetPvPGN( ) && (*i)->GetSpam( ) )
        {
          (*i)->SetSpam( );
        }
      }

      return true;
    }
  }

  // create the virtual host player

  if ( !m_GameLoading && !m_GameLoaded && GetNumPlayers( ) < 12 )
    CreateVirtualHost( );

  // unlock the game

  if ( m_Locked && !GetPlayerFromName( m_OwnerName, false ) )
  {
    SendAllChat( "Game unlocked. All admins can run game commands" );
    m_Locked = false;
  }

  // accept new connections

  if ( m_Socket )
  {
    CTCPSocket *NewSocket = m_Socket->Accept( (fd_set *) fd );

    if ( NewSocket )
      m_Potentials.push_back( new CPotentialPlayer( m_Protocol, this, NewSocket ) );

    if ( m_Socket->HasError( ) )
      return true;
  }

  return m_Exiting;
}

void CGame::UpdatePost( void *send_fd )
{
  // we need to manually call DoSend on each player now because CGamePlayer :: Update doesn't do it
  // this is in case player 2 generates a packet for player 1 during the update but it doesn't get sent because player 1 already finished updating
  // in reality since we're queueing actions it might not make a big difference but oh well

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    (*i)->GetSocket( )->DoSend( (fd_set *) send_fd );
  }

  for ( vector<CPotentialPlayer *> ::const_iterator i = m_Potentials.begin( ); i != m_Potentials.end( ); ++i )
  {
    if ( (*i)->GetSocket( ) )
      (*i)->GetSocket( )->DoSend( (fd_set *) send_fd );
  }
}

void CGame::Send( CGamePlayer *player, const BYTEARRAY &data )
{
  if ( player )
    player->Send( data );
}

void CGame::Send( unsigned char PID, const BYTEARRAY &data )
{
  Send( GetPlayerFromPID( PID ), data );
}

void CGame::Send( const BYTEARRAY &PIDs, const BYTEARRAY &data )
{
  for ( unsigned int i = 0; i < PIDs.size( ); ++i )
    Send( PIDs[i], data );
}

void CGame::SendAll( const BYTEARRAY &data )
{
  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
    (*i)->Send( data );
}

void CGame::SendChat( unsigned char fromPID, CGamePlayer *player, const string &message )
{
  // send a private message to one player - it'll be marked [Private] in Warcraft 3

  if ( player )
  {
    if ( !m_GameLoading && !m_GameLoaded )
    {
      if ( message.size( ) > 254 )
        Send( player, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, UTIL_CreateByteArray( player->GetPID( ) ), 16, BYTEARRAY( ), message.substr( 0, 254 ) ) );
      else
        Send( player, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, UTIL_CreateByteArray( player->GetPID( ) ), 16, BYTEARRAY( ), message ) );
    }
    else
    {
      unsigned char ExtraFlags[] = { 3, 0, 0, 0 };

      // based on my limited testing it seems that the extra flags' first byte contains 3 plus the recipient's colour to denote a private message

      unsigned char SID = GetSIDFromPID( player->GetPID( ) );

      if ( SID < m_Slots.size( ) )
        ExtraFlags[0] = 3 + m_Slots[SID].GetColour( );

      if ( message.size( ) > 127 )
        Send( player, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, UTIL_CreateByteArray( player->GetPID( ) ), 32, UTIL_CreateByteArray( ExtraFlags, 4 ), message.substr( 0, 127 ) ) );
      else
        Send( player, m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, UTIL_CreateByteArray( player->GetPID( ) ), 32, UTIL_CreateByteArray( ExtraFlags, 4 ), message ) );
    }
  }
}

void CGame::SendChat( unsigned char fromPID, unsigned char toPID, const string &message )
{
  SendChat( fromPID, GetPlayerFromPID( toPID ), message );
}

void CGame::SendChat( CGamePlayer *player, const string &message )
{
  SendChat( GetHostPID( ), player, message );
}

void CGame::SendChat( unsigned char toPID, const string &message )
{
  SendChat( GetHostPID( ), toPID, message );
}

void CGame::SendAllChat( unsigned char fromPID, const string &message )
{
  // send a public message to all players - it'll be marked [All] in Warcraft 3

  if ( GetNumHumanPlayers( ) > 0 )
  {
    Print( "[GAME: " + m_GameName + "] [Local]: " + message );

    if ( !m_GameLoading && !m_GameLoaded )
    {
      if ( message.size( ) > 254 )
        SendAll( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetPIDs( ), 16, BYTEARRAY( ), message.substr( 0, 254 ) ) );
      else
        SendAll( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetPIDs( ), 16, BYTEARRAY( ), message ) );
    }
    else
    {
      if ( message.size( ) > 127 )
        SendAll( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetPIDs( ), 32, UTIL_CreateByteArray( (uint32_t) 0, false ), message.substr( 0, 127 ) ) );
      else
        SendAll( m_Protocol->SEND_W3GS_CHAT_FROM_HOST( fromPID, GetPIDs( ), 32, UTIL_CreateByteArray( (uint32_t) 0, false ), message ) );
    }
  }
}

void CGame::SendAllChat( const string &message )
{
  SendAllChat( GetHostPID( ), message );
}

void CGame::SendAllSlotInfo( )
{
  if ( !m_GameLoading && !m_GameLoaded )
  {
    SendAll( m_Protocol->SEND_W3GS_SLOTINFO( m_Slots, m_RandomSeed, m_Map->GetMapLayoutStyle( ), m_Map->GetMapNumPlayers( ) ) );
    m_SlotInfoChanged = false;
  }
}

void CGame::SendVirtualHostPlayerInfo( CGamePlayer *player )
{
  if ( m_VirtualHostPID == 255 )
    return;

  BYTEARRAY IP;
  IP.push_back( 0 );
  IP.push_back( 0 );
  IP.push_back( 0 );
  IP.push_back( 0 );

  Send( player, m_Protocol->SEND_W3GS_PLAYERINFO( m_VirtualHostPID, m_VirtualHostName, IP, IP ) );
}

void CGame::SendFakePlayerInfo( CGamePlayer *player )
{
  if ( m_FakePlayers.empty( ) )
    return;

  BYTEARRAY IP;
  IP.push_back( 0 );
  IP.push_back( 0 );
  IP.push_back( 0 );
  IP.push_back( 0 );

  for ( vector<unsigned char> ::const_iterator i = m_FakePlayers.begin( ); i != m_FakePlayers.end( ); ++i )
  {
    Send( player, m_Protocol->SEND_W3GS_PLAYERINFO( *i, "Troll[" + UTIL_ToString(*i) + "]", IP, IP ) );
  }
}

void CGame::SendAllActions( )
{
  bool UsingGProxy = false;

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    if ( (*i)->GetGProxy( ) )
    {   
      UsingGProxy = true;
      break;
    }
  }

  m_GameTicks += m_Latency;
  
  if ( UsingGProxy )
  {
    // we must send empty actions to non-GProxy++ players
    // GProxy++ will insert these itself so we don't need to send them to GProxy++ players
    // empty actions are used to extend the time a player can use when reconnecting

    for ( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
    {
      if ( !(*i)->GetGProxy( ) )
      {
        for ( unsigned char j = 0; j < m_GProxyEmptyActions; ++j )
          Send( *i, m_Protocol->SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *>( ), 0 ) );
      }
    }
  }

  ++m_SyncCounter;

  // we aren't allowed to send more than 1460 bytes in a single packet but it's possible we might have more than that many bytes waiting in the queue

  if ( !m_Actions.empty( ) )
  {
    // we use a "sub actions queue" which we keep adding actions to until we reach the size limit
    // start by adding one action to the sub actions queue

    queue<CIncomingAction *> SubActions;
    CIncomingAction *Action = m_Actions.front( );
    m_Actions.pop( );
    SubActions.push( Action );
    uint32_t SubActionsLength = Action->GetLength( );

    while ( !m_Actions.empty( ) )
    {
      Action = m_Actions.front( );
      m_Actions.pop( );

      // check if adding the next action to the sub actions queue would put us over the limit (1452 because the INCOMING_ACTION and INCOMING_ACTION2 packets use an extra 8 bytes)

      if ( SubActionsLength + Action->GetLength( ) > 1452 )
      {
        // we'd be over the limit if we added the next action to the sub actions queue
        // so send everything already in the queue and then clear it out
        // the W3GS_INCOMING_ACTION2 packet handles the overflow but it must be sent *before* the corresponding W3GS_INCOMING_ACTION packet

        SendAll( m_Protocol->SEND_W3GS_INCOMING_ACTION2( SubActions ) );

        while ( !SubActions.empty( ) )
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

    while ( !SubActions.empty( ) )
    {
      delete SubActions.front( );
      SubActions.pop( );
    }
  }
  else
    SendAll( m_Protocol->SEND_W3GS_INCOMING_ACTION( m_Actions, m_Latency ) );

  uint32_t Ticks = GetTicks( );
  uint32_t ActualSendInterval = Ticks - m_LastActionSentTicks;
  uint32_t ExpectedSendInterval = m_Latency - m_LastActionLateBy;
  m_LastActionLateBy = ActualSendInterval - ExpectedSendInterval;

  if ( m_LastActionLateBy > m_Latency )
  {
    // something is going terribly wrong - Aura++ is probably starved of resources
    // print a message because even though this will take more resources it should provide some information to the administrator for future reference
    // other solutions - dynamically modify the latency, request higher priority, terminate other games, ???

    // this program is SO FAST, I've yet to see this happen *coolface*

    Print( "[GAME: " + m_GameName + "] warning - the latency is " + UTIL_ToString( m_Latency ) + "ms but the last update was late by " + UTIL_ToString( m_LastActionLateBy ) + "ms" );
    m_LastActionLateBy = m_Latency;
  }

  m_LastActionSentTicks = Ticks;
}

void CGame::EventPlayerDeleted( CGamePlayer *player )
{
  Print( "[GAME: " + m_GameName + "] deleting player [" + player->GetName( ) + "]: " + player->GetLeftReason( ) );

  m_LastPlayerLeaveTicks = GetTicks( );

  // in some cases we're forced to send the left message early so don't send it again

  if ( player->GetLeftMessageSent( ) )
    return;

  if ( m_GameLoaded )
    SendAllChat( player->GetName( ) + " " + player->GetLeftReason( ) + "." );

  if ( player->GetLagging( ) )
    SendAll( m_Protocol->SEND_W3GS_STOP_LAG( player ) );

  // tell everyone about the player leaving

  SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( player->GetPID( ), player->GetLeftCode( ) ) );

  // abort the countdown if there was one in progress

  if ( m_CountDownStarted && !m_GameLoading && !m_GameLoaded )
  {
    SendAllChat( "Countdown aborted!" );
    m_CountDownStarted = false;
  }

  // abort the votekick

  if ( !m_KickVotePlayer.empty( ) )
    SendAllChat( "A votekick against player [" + m_KickVotePlayer + "] has been cancelled" );

  m_KickVotePlayer.clear( );
  m_StartedKickVoteTime = 0;

  // record everything we need to know about the player for storing in the database later
  // since we haven't stored the game yet (it's not over yet!) we can't link the gameplayer to the game
  // see the destructor for where these CDBGamePlayers are stored in the database
  // we could have inserted an incomplete record on creation and updated it later but this makes for a cleaner interface

  if ( m_GameLoading || m_GameLoaded )
  {
    // TODO: since we store players that crash during loading it's possible that the stats classes could have no information on them
    // that could result in a DBGamePlayer without a corresponding DBDotAPlayer - just be aware of the possibility

    unsigned char SID = GetSIDFromPID( player->GetPID( ) );
    unsigned char Team = 255;
    unsigned char Colour = 255;

    if ( SID < m_Slots.size( ) )
    {
      Team = m_Slots[SID].GetTeam( );
      Colour = m_Slots[SID].GetColour( );
    }

    m_DBGamePlayers.push_back( new CDBGamePlayer( 0, 0, player->GetName( ), player->GetExternalIPString( ), player->GetSpoofed( ) ? 1 : 0, player->GetSpoofedRealm( ), player->GetReserved( ) ? 1 : 0, player->GetFinishedLoading( ) ? player->GetFinishedLoadingTicks( ) - m_StartedLoadingTicks : 0, m_GameTicks / 1000, player->GetLeftReason( ), Team, Colour ) );

    // also keep track of the last player to leave for the !banlast command

    for ( vector<CDBBan *> ::const_iterator i = m_DBBans.begin( ); i != m_DBBans.end( ); ++i )
    {
      if ( (*i)->GetName( ) == player->GetName( ) )
        m_DBBanLast = *i;
    }
  }
}

void CGame::EventPlayerDisconnectTimedOut( CGamePlayer *player )
{
  if ( player->GetGProxy( ) && m_GameLoaded )
  {
    if ( !player->GetGProxyDisconnectNoticeSent( ) )
    {
      SendAllChat( player->GetName( ) + " " + "has lost the connection (timed out) but is using GProxy++ and may reconnect" );
      player->SetGProxyDisconnectNoticeSent( true );
    }

    if ( GetTime( ) - player->GetLastGProxyWaitNoticeSentTime( ) >= 20 )
    {
      uint32_t TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60 - ( GetTime( ) - m_StartedLaggingTime );

      if ( TimeRemaining > ( (uint32_t) m_GProxyEmptyActions + 1 ) * 60 )
        TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60;

      SendAllChat( player->GetPID( ), "Please wait for me to reconnect (" + UTIL_ToString( TimeRemaining ) + " seconds remain)" );
      player->SetLastGProxyWaitNoticeSentTime( GetTime( ) );
    }

    return;
  }

  // not only do we not do any timeouts if the game is lagging, we allow for an additional grace period of 10 seconds
  // this is because Warcraft 3 stops sending packets during the lag screen
  // so when the lag screen finishes we would immediately disconnect everyone if we didn't give them some extra time

  if ( GetTime( ) - m_LastLagScreenTime >= 10 )
  {
    player->SetDeleteMe( true );
    player->SetLeftReason( "has lost the connection (timed out)" );
    player->SetLeftCode( PLAYERLEAVE_DISCONNECT );

    if ( !m_GameLoading && !m_GameLoaded )
      OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
  }
}

void CGame::EventPlayerDisconnectSocketError( CGamePlayer *player )
{
  if ( player->GetGProxy( ) && m_GameLoaded )
  {
    if ( !player->GetGProxyDisconnectNoticeSent( ) )
    {
      SendAllChat( player->GetName( ) + " " + "has lost the connection (connection error - " + player->GetSocket( )->GetErrorString( ) + ") but is using GProxy++ and may reconnect" );
      player->SetGProxyDisconnectNoticeSent( true );
    } 

    if ( GetTime( ) - player->GetLastGProxyWaitNoticeSentTime( ) >= 20 )
    {
      uint32_t TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60 - ( GetTime( ) - m_StartedLaggingTime );
      
      if ( TimeRemaining > ( (uint32_t) m_GProxyEmptyActions + 1 ) * 60 )
        TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60;

      SendAllChat( player->GetPID( ), "Please wait for me to reconnect (" + UTIL_ToString( TimeRemaining ) + " seconds remain)" );
      player->SetLastGProxyWaitNoticeSentTime( GetTime( ) );
    } 

    return;
  }

  player->SetDeleteMe( true );
  player->SetLeftReason( "has lost the connection (connection error - " + player->GetSocket( )->GetErrorString( ) + ")" );
  player->SetLeftCode( PLAYERLEAVE_DISCONNECT );

  if ( !m_GameLoading && !m_GameLoaded )
    OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
}

void CGame::EventPlayerDisconnectConnectionClosed( CGamePlayer *player )
{
  if ( player->GetGProxy( ) && m_GameLoaded )
  {
    if ( !player->GetGProxyDisconnectNoticeSent( ) )
    {
      SendAllChat( player->GetName( ) + " " + "has lost the connection (connection closed by remote host) but is using GProxy++ and may reconnect" );
      player->SetGProxyDisconnectNoticeSent( true );
    } 

    if ( GetTime( ) - player->GetLastGProxyWaitNoticeSentTime( ) >= 20 )
    {
      uint32_t TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60 - ( GetTime( ) - m_StartedLaggingTime );

      if ( TimeRemaining > ( (uint32_t) m_GProxyEmptyActions + 1 ) * 60 )
        TimeRemaining = ( m_GProxyEmptyActions + 1 ) * 60;

      SendAllChat( player->GetPID( ), "Please wait for me to reconnect (" + UTIL_ToString( TimeRemaining ) + " seconds remain)" );
      player->SetLastGProxyWaitNoticeSentTime( GetTime( ) );
    } 

    return;
  }

  player->SetDeleteMe( true );
  player->SetLeftReason( "has lost the connection (connection closed by remote host)" );
  player->SetLeftCode( PLAYERLEAVE_DISCONNECT );

  if ( !m_GameLoading && !m_GameLoaded )
    OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
}

void CGame::EventPlayerJoined( CPotentialPlayer *potential, CIncomingJoinPlayer *joinPlayer )
{
  // check the new player's name

  if ( joinPlayer->GetName( ).empty( ) || joinPlayer->GetName( ).size( ) > 15 || joinPlayer->GetName( ) == m_VirtualHostName || GetPlayerFromName( joinPlayer->GetName( ), false ) || joinPlayer->GetName( ).find( " " ) != string::npos || joinPlayer->GetName( ).find( "|" ) != string::npos )
  {
    Print( "[GAME: " + m_GameName + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] invalid name (taken, invalid char, spoofer, too long)" );
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

  if ( HostCounterID == 0 )
  {
    // check if the player joining via LAN knows the entry key

    if ( joinPlayer->GetEntryKey( ) != m_EntryKey )
    {
      Print( "[GAME: " + m_GameName + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] is trying to join the game over LAN but used an incorrect entry key" );
      potential->Send( m_Protocol->SEND_W3GS_REJECTJOIN( REJECTJOIN_WRONGPASSWORD ) );
      potential->SetDeleteMe( true );
      return;
    }
  }
  else
  {
    for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
    {
      if ( (*i)->GetHostCounterID( ) == HostCounterID )
      {
        JoinedRealm = (*i)->GetServer( );
        break;
      }
    }
  }

  // check if the new player's name is banned but only if bot_banmethod is not 0
  // this is because if bot_banmethod is 0 and we announce the ban here it's possible for the player to be rejected later because the game is full
  // this would allow the player to spam the chat by attempting to join the game multiple times in a row

  for ( vector<CBNET *> ::iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
  {
    if ( (*i)->GetServer( ) == JoinedRealm )
    {
      CDBBan *Ban = (*i)->IsBannedName( joinPlayer->GetName( ) );

      if ( Ban )
      {
        Print( "[GAME: " + m_GameName + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] is banned" );

        if ( m_IgnoredNames.find( joinPlayer->GetName( ) ) == m_IgnoredNames.end( ) )
        {
          SendAllChat( joinPlayer->GetName( ) + " is trying to join the game but is banned" );
          SendAllChat( "User [" + Ban->GetName( ) + "] was banned on server [" + Ban->GetServer( ) + "] on " + Ban->GetDate( ) + " by [" + Ban->GetAdmin( ) + "] because [" + Ban->GetReason( ) + "]" );
          m_IgnoredNames.insert( joinPlayer->GetName( ) );
        }

        // let banned players "join" the game with an arbitrary PID then immediately close the connection
        // this causes them to be kicked back to the chat channel on battle.net

        vector<CGameSlot> Slots = m_Map->GetSlots( );
        potential->Send( m_Protocol->SEND_W3GS_SLOTINFOJOIN( 1, potential->GetSocket( )->GetPort( ), potential->GetExternalIP( ), Slots, 0, m_Map->GetMapLayoutStyle( ), m_Map->GetMapNumPlayers( ) ) );
        potential->SetDeleteMe( true );

        delete Ban;
        return;
      }
    }
  }


  // check if the player is an admin or root admin on any connected realm for determining reserved status
  // we can't just use the spoof checked realm like in EventPlayerBotCommand because the player hasn't spoof checked yet

  bool AnyAdminCheck = m_Aura->m_DB->AdminCheck( joinPlayer->GetName( ) ) || m_Aura->m_DB->RootAdminCheck( joinPlayer->GetName( ) );
  bool Reserved = IsReserved( joinPlayer->GetName( ) ) || AnyAdminCheck || IsOwner( joinPlayer->GetName( ) );

  // try to find a slot

  unsigned char SID = 255;

  // try to find an empty slot

  SID = GetEmptySlot( false );

  if ( SID == 255 && Reserved )
  {
    // a reserved player is trying to join the game but it's full, try to find a reserved slot

    SID = GetEmptySlot( true );

    if ( SID != 255 )
    {
      CGamePlayer *KickedPlayer = GetPlayerFromSID( SID );

      if ( KickedPlayer )
      {
        KickedPlayer->SetDeleteMe( true );
        KickedPlayer->SetLeftReason( "was kicked to make room for a reserved player [" + joinPlayer->GetName( ) + "]" );
        KickedPlayer->SetLeftCode( PLAYERLEAVE_LOBBY );

        // send a playerleave message immediately since it won't normally get sent until the player is deleted which is after we send a playerjoin message
        // we don't need to call OpenSlot here because we're about to overwrite the slot data anyway

        SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( KickedPlayer->GetPID( ), KickedPlayer->GetLeftCode( ) ) );
        KickedPlayer->SetLeftMessageSent( true );
      }
    }
  }

  if ( SID == 255 && IsOwner( joinPlayer->GetName( ) ) )
  {
    // the owner player is trying to join the game but it's full and we couldn't even find a reserved slot, kick the player in the lowest numbered slot
    // updated this to try to find a player slot so that we don't end up kicking a computer

    SID = 0;

    for ( unsigned char i = 0; i < m_Slots.size( ); ++i )
    {
      if ( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[i].GetComputer( ) == 0 )
      {
        SID = i;
        break;
      }
    }

    CGamePlayer *KickedPlayer = GetPlayerFromSID( SID );

    if ( KickedPlayer )
    {
      KickedPlayer->SetDeleteMe( true );
      KickedPlayer->SetLeftReason( "was kicked to make room for the owner player [" + joinPlayer->GetName( ) + "]" );
      KickedPlayer->SetLeftCode( PLAYERLEAVE_LOBBY );

      // send a playerleave message immediately since it won't normally get sent until the player is deleted which is after we send a playerjoin message
      // we don't need to call OpenSlot here because we're about to overwrite the slot data anyway

      SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( KickedPlayer->GetPID( ), KickedPlayer->GetLeftCode( ) ) );
      KickedPlayer->SetLeftMessageSent( true );
    }
  }

  if ( SID >= m_Slots.size( ) )
  {
    potential->Send( m_Protocol->SEND_W3GS_REJECTJOIN( REJECTJOIN_FULL ) );
    potential->SetDeleteMe( true );
    return;
  }

  // we have a slot for the new player
  // make room for them by deleting the virtual host player if we have to

  if ( GetNumPlayers( ) >= 11 )
    DeleteVirtualHost( );

  // turning the CPotentialPlayer into a CGamePlayer is a bit of a pain because we have to be careful not to close the socket
  // this problem is solved by setting the socket to NULL before deletion and handling the NULL case in the destructor
  // we also have to be careful to not modify the m_Potentials vector since we're currently looping through it

  Print( "[GAME: " + m_GameName + "] player [" + joinPlayer->GetName( ) + "|" + potential->GetExternalIPString( ) + "] joined the game" );
  CGamePlayer *Player = new CGamePlayer( potential, GetNewPID( ), JoinedRealm, joinPlayer->GetName( ), joinPlayer->GetInternalIP( ), Reserved );

  // consider LAN players to have already spoof checked since they can't
  // since so many people have trouble with this feature we now use the JoinedRealm to determine LAN status

  if ( JoinedRealm.empty( ) )
    Player->SetSpoofed( true );

  Player->SetWhoisShouldBeSent( AnyAdminCheck );
  m_Players.push_back( Player );
  potential->SetSocket( NULL );
  potential->SetDeleteMe( true );

  if ( m_Map->GetMapOptions( ) & MAPOPT_CUSTOMFORCES )
    m_Slots[SID] = CGameSlot( Player->GetPID( ), 255, SLOTSTATUS_OCCUPIED, 0, m_Slots[SID].GetTeam( ), m_Slots[SID].GetColour( ), m_Slots[SID].GetRace( ) );
  else
  {
    if ( m_Map->GetMapFlags( ) & MAPFLAG_RANDOMRACES )
      m_Slots[SID] = CGameSlot( Player->GetPID( ), 255, SLOTSTATUS_OCCUPIED, 0, 12, 12, SLOTRACE_RANDOM );
    else
      m_Slots[SID] = CGameSlot( Player->GetPID( ), 255, SLOTSTATUS_OCCUPIED, 0, 12, 12, SLOTRACE_RANDOM | SLOTRACE_SELECTABLE );

    // try to pick a team and colour
    // make sure there aren't too many other players already

    unsigned char NumOtherPlayers = 0;

    for ( unsigned char i = 0; i < m_Slots.size( ); ++i )
    {
      if ( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[i].GetTeam( ) != 12 )
        ++NumOtherPlayers;
    }

    if ( NumOtherPlayers < m_Map->GetMapNumPlayers( ) )
    {
      if ( SID < m_Map->GetMapNumPlayers( ) )
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

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    if ( !(*i)->GetLeftMessageSent( ) && *i != Player )
    {
      // send info about the new player to every other player

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

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    if ( Player != *i && Player->GetExternalIPString( ) == (*i)->GetExternalIPString( ) )
    {
      if ( Others.empty( ) )
        Others = (*i)->GetName( );
      else
        Others += ", " + (*i)->GetName( );
    }
  }

  if ( !Others.empty( ) )
    SendAllChat( "Player [" + joinPlayer->GetName( ) + "] has the same IP address as: " + Others );

  // abort the countdown if there was one in progress

  if ( m_CountDownStarted && !m_GameLoading && !m_GameLoaded )
  {
    SendAllChat( "Countdown aborted!" );
    m_CountDownStarted = false;
  }

  // auto lock the game

  if ( m_Aura->m_AutoLock && !m_Locked && IsOwner( joinPlayer->GetName( ) ) )
  {
    SendAllChat( "Game locked. Only the game owner and root admins can run game commands" );
    m_Locked = true;
  }
}

void CGame::EventPlayerLeft( CGamePlayer *player, uint32_t reason )
{
  // this function is only called when a player leave packet is received, not when there's a socket error, kick, etc...

  player->SetDeleteMe( true );

  if ( reason == PLAYERLEAVE_GPROXY )
    player->SetLeftReason( "was unrecoverably dropped from GProxy++" );   
  else    
    player->SetLeftReason( "has left the game voluntarily" );
  
  player->SetLeftCode( PLAYERLEAVE_LOST );

  if ( !m_GameLoading && !m_GameLoaded )
    OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
}

void CGame::EventPlayerLoaded( CGamePlayer *player )
{
  Print( "[GAME: " + m_GameName + "] player [" + player->GetName( ) + "] finished loading in " + UTIL_ToString( (float) ( player->GetFinishedLoadingTicks( ) - m_StartedLoadingTicks ) / 1000.f, 2 ) + " seconds" );


  SendAll( m_Protocol->SEND_W3GS_GAMELOADED_OTHERS( player->GetPID( ) ) );
}

void CGame::EventPlayerAction( CGamePlayer *player, CIncomingAction *action )
{
  m_Actions.push( action );

  // check for players saving the game and notify everyone

  if ( !action->GetAction( )->empty( ) && ( *action->GetAction( ) )[0] == 6 )
  {
    Print( "[GAME: " + m_GameName + "] player [" + player->GetName( ) + "] is saving the game" );
    SendAllChat( "Player [" + player->GetName( ) + "] is saving the game" );
  }

  // give the stats class a chance to process the action

  if ( m_Stats && action->GetAction( )->size( ) >= 6 && m_Stats->ProcessAction( action ) && m_GameOverTime == 0 )
  {
    Print( "[GAME: " + m_GameName + "] gameover timer started (stats class reported game over)" );
    m_GameOverTime = GetTime( );
  }
}

void CGame::EventPlayerKeepAlive( CGamePlayer *player )
{
  // check for desyncs

  uint32_t FirstCheckSum = player->GetCheckSums( )->front( );

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    if ( (*i)->GetCheckSums( )->empty( ) )
      return;

    if ( !m_Desynced && (*i)->GetCheckSums( )->front( ) != FirstCheckSum )
    {
      m_Desynced = true;
      Print( "[GAME: " + m_GameName + "] desync detected" );
      SendAllChat( "Warning! Desync detected!" );
      SendAllChat( "Warning! Desync detected!" );
      SendAllChat( "Warning! Desync detected!" );
    }
  }

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
    (*i)->GetCheckSums( )->pop( );
}

void CGame::EventPlayerChatToHost( CGamePlayer *player, CIncomingChatPlayer *chatPlayer )
{
  if ( chatPlayer->GetFromPID( ) == player->GetPID( ) )
  {
    if ( chatPlayer->GetType( ) == CIncomingChatPlayer::CTH_MESSAGE || chatPlayer->GetType( ) == CIncomingChatPlayer::CTH_MESSAGEEXTRA )
    {
      // relay the chat message to other players

      bool Relay = !player->GetMuted( );
      BYTEARRAY ExtraFlags = chatPlayer->GetExtraFlags( );

      // calculate timestamp

      string MinString = UTIL_ToString( ( m_GameTicks / 1000 ) / 60 );
      string SecString = UTIL_ToString( ( m_GameTicks / 1000 ) % 60 );

      if ( MinString.size( ) == 1 )
        MinString.insert( 0, "0" );

      if ( SecString.size( ) == 1 )
        SecString.insert( 0, "0" );

      if ( !ExtraFlags.empty( ) )
      {
        if ( ExtraFlags[0] == 1 )
        {
          Print( "[GAME: " + m_GameName + "] (" + MinString + ":" + SecString + ") [Allies] [" + player->GetName( ) + "]: " + chatPlayer->GetMessage( ) );
        }
        else if ( ExtraFlags[0] == 0 )
        {
          // this is an ingame [All] message, print it to the console

          Print( "[GAME: " + m_GameName + "] (" + MinString + ":" + SecString + ") [All] [" + player->GetName( ) + "]: " + chatPlayer->GetMessage( ) );

          // don't relay ingame messages targeted for all players if we're currently muting all
          // note that commands will still be processed even when muting all because we only stop relaying the messages, the rest of the function is unaffected

          if ( m_MuteAll )
            Relay = false;
        }
        else if ( ExtraFlags[0] == 2 )
        {
          // this is an ingame [Obs/Ref] message, print it to the console

          Print( "[GAME: " + m_GameName + "] (" + MinString + ":" + SecString + ") [Obs/Ref] [" + player->GetName( ) + "]: " + chatPlayer->GetMessage( ) );
        }
      }
      else
      {
        // this is a lobby message, print it to the console

        Print( "[GAME: " + m_GameName + "] [Lobby] [" + player->GetName( ) + "]: " + chatPlayer->GetMessage( ) );

        if ( m_MuteLobby )
          Relay = false;
      }

      // handle bot commands

      string Message = chatPlayer->GetMessage( );

      if ( !Message.empty( ) && ( Message[0] == m_Aura->m_CommandTrigger || Message[0] == '/' ) )
      {
        // extract the command trigger, the command, and the payload
        // e.g. "!say hello world" -> command: "say", payload: "hello world"

        string Command, Payload;
        string::size_type PayloadStart = Message.find( " " );

        if ( PayloadStart != string::npos )
        {
          Command = Message.substr( 1, PayloadStart - 1 );
          Payload = Message.substr( PayloadStart + 1 );
        }
        else
          Command = Message.substr( 1 );

        transform( Command.begin( ), Command.end( ), Command.begin( ), (int(* )(int) )tolower );

        // don't allow EventPlayerBotCommand to veto a previous instruction to set Relay to false
        // so if Relay is already false (e.g. because the player is muted) then it cannot be forced back to true here

        EventPlayerBotCommand( player, Command, Payload );
        Relay = false;
      }

      if ( Relay )
        Send( chatPlayer->GetToPIDs( ), m_Protocol->SEND_W3GS_CHAT_FROM_HOST( chatPlayer->GetFromPID( ), chatPlayer->GetToPIDs( ), chatPlayer->GetFlag( ), chatPlayer->GetExtraFlags( ), chatPlayer->GetMessage( ) ) );
    }
    else if ( chatPlayer->GetType( ) == CIncomingChatPlayer::CTH_TEAMCHANGE && !m_CountDownStarted )
      EventPlayerChangeTeam( player, chatPlayer->GetByte( ) );
    else if ( chatPlayer->GetType( ) == CIncomingChatPlayer::CTH_COLOURCHANGE && !m_CountDownStarted )
      EventPlayerChangeColour( player, chatPlayer->GetByte( ) );
    else if ( chatPlayer->GetType( ) == CIncomingChatPlayer::CTH_RACECHANGE && !m_CountDownStarted )
      EventPlayerChangeRace( player, chatPlayer->GetByte( ) );
    else if ( chatPlayer->GetType( ) == CIncomingChatPlayer::CTH_HANDICAPCHANGE && !m_CountDownStarted )
      EventPlayerChangeHandicap( player, chatPlayer->GetByte( ) );
  }
}

bool CGame::EventPlayerBotCommand( CGamePlayer *player, string &command, string &payload )
{
  string User = player->GetName( ), Command = command, Payload = payload;

  bool AdminCheck = false, RootAdminCheck = false;

  for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
  {
    if ( ( (*i)->GetServer( ) == player->GetSpoofedRealm( ) || player->GetJoinedRealm( ).empty( ) ) && (*i)->IsRootAdmin( User ) )
    {
      RootAdminCheck = true;
      AdminCheck = true;
      break;
    }
  }

  if ( !RootAdminCheck )
  {
    for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
    {
      if ( ( (*i)->GetServer( ) == player->GetSpoofedRealm( ) || player->GetJoinedRealm( ).empty( ) ) && (*i)->IsAdmin( User ) )
      {
        AdminCheck = true;
        break;
      }
    }
  }

  if ( player->GetSpoofed( ) && ( AdminCheck || RootAdminCheck || IsOwner( User ) ) )
  {
    Print( "[GAME: " + m_GameName + "] admin [" + User + "] sent command [" + Command + "] with payload [" + Payload + "]" );

    if ( !m_Locked || RootAdminCheck || IsOwner( User ) )
    {
      /*****************
       * ADMIN COMMANDS *
       ******************/

      //
      // !PING
      //

      if ( Command == "ping" || Command == "p" )
      {
        // kick players with ping higher than payload if payload isn't empty
        // we only do this if the game hasn't started since we don't want to kick players from a game in progress

        uint32_t Kicked = 0, KickPing = 0;

        if ( !m_GameLoading && !m_GameLoaded && !Payload.empty( ) )
          KickPing = UTIL_ToUInt32( Payload );

        // copy the m_Players vector so we can sort by descending ping so it's easier to find players with high pings

        vector<CGamePlayer *> SortedPlayers = m_Players;
        sort( SortedPlayers.begin( ), SortedPlayers.end( ), CGamePlayerSortDescByPing( ) );
        string Pings;

        for ( vector<CGamePlayer *> ::const_iterator i = SortedPlayers.begin( ); i != SortedPlayers.end( ); ++i )
        {
          Pings += (*i)->GetName( );
          Pings += ": ";

          if ( (*i)->GetNumPings( ) > 0 )
          {
            Pings += UTIL_ToString( (*i)->GetPing( m_Aura->m_LCPings ) );

            if ( !m_GameLoaded && !m_GameLoading && !(*i)->GetReserved( ) && KickPing > 0 && (*i)->GetPing( m_Aura->m_LCPings ) > KickPing )
            {
              (*i)->SetDeleteMe( true );
              (*i)->SetLeftReason( "was kicked for excessive ping " + UTIL_ToString( (*i)->GetPing( m_Aura->m_LCPings ) ) + " > " + UTIL_ToString( KickPing ) );
              (*i)->SetLeftCode( PLAYERLEAVE_LOBBY );
              OpenSlot( GetSIDFromPID( (*i)->GetPID( ) ), false );
              ++Kicked;
            }

            Pings += "ms";
          }
          else
            Pings += "N/A";

          if ( i != SortedPlayers.end( ) - 1 )
            Pings += ", ";

          if ( ( m_GameLoading || m_GameLoaded ) && Pings.size( ) > 100 )
          {
            // cut the text into multiple lines ingame

            SendAllChat( Pings );
            Pings.clear( );
          }
        }

        if ( !Pings.empty( ) )
          SendAllChat( Pings );

        if ( Kicked > 0 )          
          SendAllChat( "Kicking " + UTIL_ToString( Kicked ) + " players with pings greater than " + UTIL_ToString( KickPing ) );          
      }

        //
        // !FROM
        //

      else if ( Command == "from" || Command == "f" )
      {
        string Froms;

        for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
        {
          // we reverse the byte order on the IP because it's stored in network byte order

          Froms += (*i)->GetName( );
          Froms += ": (";
          Froms += m_Aura->m_DB->FromCheck( UTIL_ByteArrayToUInt32( (*i)->GetExternalIP( ), true ) );
          Froms += ")";

          if ( i != m_Players.end( ) - 1 )
            Froms += ", ";

          if ( ( m_GameLoading || m_GameLoaded ) && Froms.size( ) > 100 )
          {
            // cut the text into multiple lines ingame

            SendAllChat( Froms );
            Froms.clear( );
          }
        }

        if ( !Froms.empty( ) )
          SendAllChat( Froms );
      }

        //
        // !BANLAST
        //

      else if ( ( Command == "banlast" || Command == "bl" ) && m_GameLoaded && !m_Aura->m_BNETs.empty( ) && m_DBBanLast )
      {
        m_Aura->m_DB->BanAdd( m_DBBanLast->GetServer( ), m_DBBanLast->GetName( ), m_DBBanLast->GetIP( ), m_GameName, User, Payload );
        SendAllChat( "Player [" + m_DBBanLast->GetName( ) + "] was banned by player [" + User + "] on server [" + m_DBBanLast->GetServer( ) + "]" );
      }

        //
        // !CLOSE (close slot)
        //

      else if ( ( Command == "close" || Command == "c" ) && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
      {
        // close as many slots as specified, e.g. "5 10" closes slots 5 and 10

        stringstream SS;
        SS << Payload;

        while ( !SS.eof( ) )
        {
          uint32_t SID;
          SS >> SID;

          if ( SS.fail( ) )
          {
            Print( "[GAME: " + m_GameName + "] bad input to close command" );
            break;
          }
          else
            CloseSlot( (unsigned char) ( SID - 1 ), true );
        }
      }

        //
        // !END
        //

      else if ( ( Command == "end" || Command == "e" ) && m_GameLoaded )
      {
        Print( "[GAME: " + m_GameName + "] is over (admin ended game)" );
        StopPlayers( "was disconnected (admin ended game)" );
      }

        //
        // !HCL
        //

      else if ( Command == "hcl" && !m_CountDownStarted )
      {
        if ( !Payload.empty( ) )
        {
          if ( Payload.size( ) <= m_Slots.size( ) )
          {
            string HCLChars = "abcdefghijklmnopqrstuvwxyz0123456789 -=,.";

            if ( Payload.find_first_not_of( HCLChars ) == string::npos )
            {
              m_HCLCommandString = Payload;
              SendAllChat( "Setting HCL command string to [" + m_HCLCommandString + "]" );
            }
            else
              SendAllChat( "Unable to set HCL command string because it contains invalid characters" );
          }
          else
            SendAllChat( "Unable to set HCL command string because it's too long" );
        }
        else
          SendAllChat( "The HCL command string is [" + m_HCLCommandString + "]" );
      }

        //
        // !HOLD (hold a slot for someone)
        //

      else if ( Command == "hold" && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
      {
        // hold as many players as specified, e.g. "Varlock Kilranin" holds players "Varlock" and "Kilranin"

        stringstream SS;
        SS << Payload;

        while ( !SS.eof( ) )
        {
          string HoldName;
          SS >> HoldName;

          if ( SS.fail( ) )
          {
            Print( "[GAME: " + m_GameName + "] bad input to hold command" );
            break;
          }
          else
          {
            SendAllChat( "Added player [" + HoldName + "] to the hold list" );
            AddToReserved( HoldName );
          }
        }
      }

        //
        // !KICK (kick a player)
        //

      else if ( ( Command == "kick" || Command == "k" ) && !Payload.empty( ) )
      {
        CGamePlayer *LastMatch = NULL;
        uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

        if ( Matches == 0 )
          SendChat( player, "Unable to kick player [" + Payload + "]. No matches found" );
        else if ( Matches == 1 )
        {
          LastMatch->SetDeleteMe( true );
          LastMatch->SetLeftReason( "was kicked by player [" + User + "]" );

          if ( !m_GameLoading && !m_GameLoaded )
            LastMatch->SetLeftCode( PLAYERLEAVE_LOBBY );
          else
            LastMatch->SetLeftCode( PLAYERLEAVE_LOST );

          if ( !m_GameLoading && !m_GameLoaded )
            OpenSlot( GetSIDFromPID( LastMatch->GetPID( ) ), false );
        }
        else
          SendChat( player, "Unable to kick player [" + Payload + "]. Found more than one match" );
      }

        //
        // !LATENCY (set game latency)
        //

      else if ( Command == "latency" || Command == "l" )
      {
        if ( Payload.empty( ) )
          SendAllChat( "The game latency is " + UTIL_ToString( m_Latency ) + " ms" );
        else
        {
          m_Latency = UTIL_ToUInt32( Payload );

          if ( m_Latency <= 25 )
          {
            m_Latency = 25;
            SendAllChat( "Setting game latency to the minimum of 25 ms" );
          }
          else if ( m_Latency >= 250 )
          {
            m_Latency = 250;
            SendAllChat( "Setting game latency to the maximum of 250 ms" );
          }
          else
            SendAllChat( "Setting game latency to " + UTIL_ToString( m_Latency ) + " ms" );
        }
      }

        //
        // !OPEN (open slot)
        //

      else if ( ( Command == "open" || Command == "o" ) && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
      {
        // open as many slots as specified, e.g. "5 10" opens slots 5 and 10

        stringstream SS;
        SS << Payload;

        while ( !SS.eof( ) )
        {
          uint32_t SID;
          SS >> SID;

          // subtract one due to index starting at 0

          --SID;

          if ( SS.fail( ) )
          {
            Print( "[GAME: " + m_GameName + "] bad input to open command" );
            break;
          }
          else
          {
            // check if the slots is occupied by a fake player, if yes delete him

            bool Fake = false;

            if ( SID < m_Slots.size( ) )
            {
              for ( vector<unsigned char> ::iterator i = m_FakePlayers.begin( ); i != m_FakePlayers.end( ); ++i )
              {
                if ( m_Slots[SID].GetPID( ) == (*i) )
                {
                  Fake = true;
                  m_Slots[SID] = CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, m_Slots[SID].GetTeam( ), m_Slots[SID].GetColour( ), m_Slots[SID].GetRace( ) );
                  SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( *i, PLAYERLEAVE_LOBBY ) );
                  m_FakePlayers.erase( i );
                  SendAllSlotInfo( );
                  break;
                }
              }
            }

            // not a fake player

            if ( !Fake )
              OpenSlot( (unsigned char) SID, true );
          }
        }
      }



        //
        // !PRIV (rehost as private game)
        //

      else if ( Command == "priv" && !Payload.empty( ) && !m_CountDownStarted )
      {
        if ( Payload.length( ) < 31 )
        {
          Print( "[GAME: " + m_GameName + "] trying to rehost as private game [" + Payload + "]" );
          SendAllChat( "Trying to rehost as private game [" + Payload + "]. Please wait, this will take several seconds" );
          m_GameState = GAME_PRIVATE;
          m_LastGameName = m_GameName;
          m_GameName = Payload;
          m_HostCounter = m_Aura->m_HostCounter++;
          m_RefreshError = false;

          for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
          {
            // unqueue any existing game refreshes because we're going to assume the next successful game refresh indicates that the rehost worked
            // this ignores the fact that it's possible a game refresh was just sent and no response has been received yet
            // we assume this won't happen very often since the only downside is a potential false positive

            (*i)->UnqueueGameRefreshes( );
            (*i)->QueueGameUncreate( );
            (*i)->QueueEnterChat( );

            // we need to send the game creation message now because private games are not refreshed

            (*i)->QueueGameCreate( m_GameState, m_GameName, m_Map, m_HostCounter );

            if ( !(*i)->GetPvPGN( ) )
              (*i)->QueueEnterChat( );
          }

          m_CreationTime = GetTime( );
          m_LastRefreshTime = GetTime( );
        }
        else
          SendAllChat( "Unable to create game [" + Payload + "]. The game name is too long (the maximum is 31 characters)" );
      }

        //
        // !PUB (rehost as public game)
        //

      else if ( Command == "pub" && !Payload.empty( ) && !m_CountDownStarted )
      {
        if ( Payload.length( ) < 31 )
        {
          Print( "[GAME: " + m_GameName + "] trying to rehost as public game [" + Payload + "]" );
          SendAllChat( "Trying to rehost as public game [" + Payload + "]. Please wait, this will take several seconds" );
          m_GameState = GAME_PUBLIC;
          m_LastGameName = m_GameName;
          m_GameName = Payload;
          m_HostCounter = m_Aura->m_HostCounter++;
          m_RefreshError = false;

          for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
          {
            // unqueue any existing game refreshes because we're going to assume the next successful game refresh indicates that the rehost worked
            // this ignores the fact that it's possible a game refresh was just sent and no response has been received yet
            // we assume this won't happen very often since the only downside is a potential false positive

            (*i)->UnqueueGameRefreshes( );
            (*i)->QueueGameUncreate( );
            (*i)->QueueEnterChat( );

            // the game creation message will be sent on the next refresh
          }

          m_CreationTime = m_LastRefreshTime = GetTime( );
        }
        else
          SendAllChat( "Unable to create game [" + Payload + "]. The game name is too long (the maximum is 31 characters)" );
      }

        //
        // !START
        //

      else if ( ( Command == "start" || Command == "s" ) && !m_CountDownStarted )
      {
        // if the player sent "!start force" skip the checks and start the countdown
        // otherwise check that the game is ready to start

        if ( Payload == "force" )
          StartCountDown( true );
        else
        {
          if ( GetTicks( ) - m_LastPlayerLeaveTicks >= 2000 )
            StartCountDown( false );
          else
            SendAllChat( "Countdown aborted because someone left the game less than two seconds ago!" );
        }
      }

        //
        // !SWAP (swap slots)
        //

      else if ( ( Command == "swap" || Command == "sw" ) && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
      {
        uint32_t SID1, SID2;
        stringstream SS;
        SS << Payload;
        SS >> SID1;

        if ( SS.fail( ) )
          Print( "[GAME: " + m_GameName + "] bad input #1 to swap command" );
        else
        {
          if ( SS.eof( ) )
            Print( "[GAME: " + m_GameName + "] missing input #2 to swap command" );
          else
          {
            SS >> SID2;

            if ( SS.fail( ) )
              Print( "[GAME: " + m_GameName + "] bad input #2 to swap command" );
            else
              SwapSlots( (unsigned char) ( SID1 - 1 ), (unsigned char) ( SID2 - 1 ) );
          }
        }
      }

        //
        // !SYNCLIMIT
        //

      else if ( Command == "synclimit" || Command == "sl" )
      {
        if ( Payload.empty( ) )
          SendAllChat( "The sync limit is " + UTIL_ToString( m_SyncLimit ) + " packets" );
        else
        {
          m_SyncLimit = UTIL_ToUInt32( Payload );

          if ( m_SyncLimit <= 40 )
          {
            m_SyncLimit = 40;
            SendAllChat( "Setting sync limit to the minimum of 40 packets" );
          }
          else if ( m_SyncLimit >= 120 )
          {
            m_SyncLimit = 120;
            SendAllChat( "Setting sync limit to the maximum of 120 packets" );
          }
          else
            SendAllChat( "Setting sync limit to " + UTIL_ToString( m_SyncLimit ) + " packets" );
        }
      }

        //
        // !UNHOST
        //

      else if ( ( Command == "unhost" || Command == "uh" ) && !m_CountDownStarted )
      {
        for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
        {
          if ( (*i)->GetSpam( ) )
          {
            (*i)->SetSpam( );
          }
        }

        m_Exiting = true;
      }

        //
        // !SPAM
        //

      else if ( Command == "spam" && !m_CountDownStarted && m_GameState == GAME_PRIVATE && m_Map->GetMapType( ) == "dota" )
      {
        for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
        {
          if ( !(*i)->GetPvPGN( ) )
          {
            (*i)->SetSpam( );
          }
        }
      }

        //
        // !HANDICAP
        //

      else if ( ( Command == "handicap" || Command == "h" ) && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
      {
        // extract the slot and the handicap
        // e.g. "1 50" -> slot: "1", handicap: "50"

        uint32_t Slot, Handicap;
        stringstream SS;
        SS << Payload;
        SS >> Slot;

        if ( SS.fail( ) )
          Print( "[GAME: " + m_GameName + "] bad input #1 to handicap command" );
        else
        {
          if ( SS.eof( ) )
            Print( "[GAME: " + m_GameName + "] missing input #2 to handicap command" );
          else
          {
            SS >> Handicap;

            if ( SS.fail( ) )
              Print( "[GAME: " + m_GameName + "] bad input #2 to handicap command" );
            else
            {
              unsigned char SID = (unsigned char) ( Slot - 1 );

              if ( SID < m_Slots.size( ) )
              {
                if ( m_Slots[SID].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED )
                {
                  m_Slots[SID].SetHandicap( (unsigned char) Handicap );
                  SendAllSlotInfo( );
                }
              }
            }
          }
        }
      }

        //
        // !DOWNLOAD
        // !DL
        //

      else if ( ( Command == "download" || Command == "dl" ) && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
      {
        CGamePlayer *LastMatch = NULL;
        uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

        if ( Matches == 0 )
          SendChat( player, "Unable to start download for player [" + Payload + "]. No matches found" );
        else if ( Matches == 1 )
        {
          if ( !LastMatch->GetDownloadStarted( ) && !LastMatch->GetDownloadFinished( ) )
          {
            unsigned char SID = GetSIDFromPID( LastMatch->GetPID( ) );

            if ( SID < m_Slots.size( ) && m_Slots[SID].GetDownloadStatus( ) != 100 )
            {
              // inform the client that we are willing to send the map

              Print( "[GAME: " + m_GameName + "] map download started for player [" + LastMatch->GetName( ) + "]" );
              Send( LastMatch, m_Protocol->SEND_W3GS_STARTDOWNLOAD( GetHostPID( ) ) );
              LastMatch->SetDownloadAllowed( true );
              LastMatch->SetDownloadStarted( true );
              LastMatch->SetStartedDownloadingTicks( GetTicks( ) );
            }
          }
        }
        else
          SendAllChat( "Unable to start download for player [" + Payload + "]. Found more than one match" );
      }

        //
        // !DOWNLOADS
        //

      else if ( ( Command == "downloads" || Command == "dls" ) && !Payload.empty( ) )
      {
        uint32_t Downloads = UTIL_ToUInt32( Payload );

        if ( Downloads == 0 )
        {
          SendAllChat( "Map downloads disabled" );
          m_Aura->m_AllowDownloads = 0;
        }
        else if ( Downloads == 1 )
        {
          SendAllChat( "Map downloads enabled" );
          m_Aura->m_AllowDownloads = 1;
        }
        else if ( Downloads == 2 )
        {
          SendAllChat( "Conditional map downloads enabled" );
          m_Aura->m_AllowDownloads = 2;
        }
      }

        //
        // !DROP
        //

      else if ( Command == "drop" && m_GameLoaded )
        StopLaggers( "lagged out (dropped by admin)" );

        //
        // !MUTE
        //

      else if ( Command == "mute" )
      {
        CGamePlayer *LastMatch = NULL;
        uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

        if ( Matches == 0 )
          SendChat( player, "Unable to mute/unmute player [" + Payload + "]. No matches found" );
        else if ( Matches == 1 )
        {
          SendAllChat( "Player [" + LastMatch->GetName( ) + "] was muted by player [" + User + "]" );
          LastMatch->SetMuted( true );
        }
        else
          SendChat( player, "Unable to mute/unmute player [" + Payload + "]. Found more than one match" );
      }

        //
        // !MUTEALL
        //

      else if ( Command == "muteall" && m_GameLoaded )
      {
        SendAllChat( "Global chat muted (allied and private chat is unaffected)" );
        m_MuteAll = true;
      }

        //
        // !ABORT (abort countdown)
        // !A
        //

        // we use "!a" as an alias for abort because you don't have much time to abort the countdown so it's useful for the abort command to be easy to type

      else if ( ( Command == "abort" || Command == "a" ) && m_CountDownStarted && !m_GameLoading && !m_GameLoaded )
      {
        SendAllChat( "Countdown aborted!" );
        m_CountDownStarted = false;
      }

        //
        // !ADDBAN
        // !BAN
        //

      else if ( ( Command == "addban" || Command == "ban" ) && !Payload.empty( ) && !m_Aura->m_BNETs.empty( ) )
      {
        // extract the victim and the reason
        // e.g. "Varlock leaver after dying" -> victim: "Varlock", reason: "leaver after dying"

        string Victim, Reason;
        stringstream SS;
        SS << Payload;
        SS >> Victim;

        if ( !SS.eof( ) )
        {
          getline( SS, Reason );
          string::size_type Start = Reason.find_first_not_of( " " );

          if ( Start != string::npos )
            Reason = Reason.substr( Start );
        }

        if ( m_GameLoaded )
        {
          string VictimLower = Victim;
          transform( VictimLower.begin( ), VictimLower.end( ), VictimLower.begin( ), (int(* )(int) )tolower );
          uint32_t Matches = 0;
          CDBBan *LastMatch = NULL;

          // try to match each player with the passed string (e.g. "Varlock" would be matched with "lock")
          // we use the m_DBBans vector for this in case the player already left and thus isn't in the m_Players vector anymore

          for ( vector<CDBBan *> ::const_iterator i = m_DBBans.begin( ); i != m_DBBans.end( ); ++i )
          {
            string TestName = (*i)->GetName( );
            transform( TestName.begin( ), TestName.end( ), TestName.begin( ), (int(* )(int) )tolower );

            if ( TestName.find( VictimLower ) != string::npos )
            {
              ++Matches;
              LastMatch = *i;

              // if the name matches exactly stop any further matching

              if ( TestName == VictimLower )
              {
                Matches = 1;
                break;
              }
            }
          }

          if ( Matches == 0 )
            SendChat( player, "Unable to ban player [" + Victim + "]. No matches found" );
          else if ( Matches == 1 )
          {
            m_Aura->m_DB->BanAdd( LastMatch->GetServer( ), LastMatch->GetName( ), LastMatch->GetIP( ), m_GameName, User, Reason );
            SendAllChat( "Player [" + LastMatch->GetName( ) + "] was banned by player [" + User + "] on server [" + LastMatch->GetServer( ) + "]" );
          }
          else
            SendChat( player, "Unable to ban player [" + Victim + "]. Found more than one match" );
        }
        else
        {
          CGamePlayer *LastMatch = NULL;
          uint32_t Matches = GetPlayerFromNamePartial( Victim, &LastMatch );

          if ( Matches == 0 )
            SendChat( player, "Unable to ban player [" + Victim + "]. No matches found" );
          else if ( Matches == 1 )
          {
            m_Aura->m_DB->BanAdd( LastMatch->GetJoinedRealm( ), LastMatch->GetName( ), LastMatch->GetExternalIPString( ), m_GameName, User, Reason );
            SendAllChat( "Player [" + LastMatch->GetJoinedRealm( ) + "] was banned by player [" + LastMatch->GetName( ) + "] on server [" + User + "]" );
          }
          else
            SendChat( player, "Unable to ban player [" + Victim + "]. Found more than one match" );
        }
      }


        //
        // !CHECK
        //

      else if ( Command == "check" )
      {
        if ( !Payload.empty( ) )
        {
          CGamePlayer *LastMatch = NULL;
          uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

          if ( Matches == 0 )
            SendChat( player, "Unable to check player [" + Payload + "]. No matches found" );
          else if ( Matches == 1 )
          {
            bool LastMatchAdminCheck = false;

            for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
            {
              if ( ( (*i)->GetServer( ) == LastMatch->GetSpoofedRealm( ) || LastMatch->GetJoinedRealm( ).empty( ) ) && (*i)->IsAdmin( LastMatch->GetName( ) ) )
              {
                LastMatchAdminCheck = true;
                break;
              }
            }

            bool LastMatchRootAdminCheck = false;

            for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
            {
              if ( ( (*i)->GetServer( ) == LastMatch->GetSpoofedRealm( ) || LastMatch->GetJoinedRealm( ).empty( ) ) && (*i)->IsRootAdmin( LastMatch->GetName( ) ) )
              {
                LastMatchRootAdminCheck = true;
                break;
              }
            }

            SendAllChat( "Checked player [" + LastMatch->GetName( ) + "]. Ping: " + ( LastMatch->GetNumPings( ) > 0 ? UTIL_ToString( LastMatch->GetPing( m_Aura->m_LCPings ) ) + "ms" : "N/A" ) + ", From: " + m_Aura->m_DB->FromCheck( UTIL_ByteArrayToUInt32( LastMatch->GetExternalIP( ), true ) ) + ", Admin: " + ( LastMatchAdminCheck || LastMatchRootAdminCheck ? "Yes" : "No" ) + ", Owner: " + ( IsOwner( LastMatch->GetName( ) ) ? "Yes" : "No" ) + ", Spoof Checked: " + ( LastMatch->GetSpoofed( ) ? "Yes" : "No" ) + ", Realm: " + ( LastMatch->GetJoinedRealm( ).empty( ) ? "LAN" : LastMatch->GetJoinedRealm( ) ) + ", Reserved: " + ( LastMatch->GetReserved( ) ? "Yes" : "No" ) );
          }
          else
            SendChat( player, "Unable to check player [" + Payload + "]. Found more than one match" );
        }
        else
          SendAllChat( "Checked player [" + User + "]. Ping: " + ( player->GetNumPings( ) > 0 ? UTIL_ToString( player->GetPing( m_Aura->m_LCPings ) ) + "ms" : "N/A" ) + ", From: " + m_Aura->m_DB->FromCheck( UTIL_ByteArrayToUInt32( player->GetExternalIP( ), true ) ) + ", Admin: " + ( AdminCheck || RootAdminCheck ? "Yes" : "No" ) + ", Owner: " + ( IsOwner( User ) ? "Yes" : "No" ) + ", Spoof Checked: " + ( player->GetSpoofed( ) ? "Yes" : "No" ) + ", Realm: " + ( player->GetJoinedRealm( ).empty( ) ? "LAN" : player->GetJoinedRealm( ) ) + ", Reserved: " + ( player->GetReserved( ) ? "Yes" : "No" ) );
      }

        //
        // !CHECKBAN
        //

      else if ( Command == "checkban" && !Payload.empty( ) && !m_Aura->m_BNETs.empty( ) )
      {
        for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
        {
          CDBBan *Ban = m_Aura->m_DB->BanCheck( (*i)->GetServer( ), Payload, string( ) );

          if ( Ban )
          {
            SendAllChat( "User [" + Payload + "] was banned on server [" + (*i)->GetServer( ) + "] on " + Ban->GetDate( ) + " by [" + Ban->GetAdmin( ) + "] because [" + Ban->GetReason( ) + "]" );
            delete Ban;
          }
          else
            SendAllChat( "User [" + Payload + "] is not banned on server [" + (*i)->GetServer( ) + "]" );            
        }
      }

        //
        // !CLEARHCL
        //

      else if ( Command == "clearhcl" && !m_CountDownStarted )
      {
        m_HCLCommandString.clear( );
        SendAllChat( "Clearing HCL command string" );
      }

        //
        // !STATUS
        //

      else if ( Command == "status" )
      {
        string message = "Status: ";

        for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
          message += (*i)->GetServer( ) + ( (*i)->GetLoggedIn( ) ? " [Online], " : " [Offline], " );

        SendAllChat( message + m_Aura->m_IRC->m_Server + ( !m_Aura->m_IRC->m_WaitingToConnect ? " [Online]" : " [Offline]" ) );
      }

        //
        // !SENDLAN
        //

      else if ( Command == "sendlan" && !Payload.empty( ) && !m_CountDownStarted )
      {
        // extract the ip and the port
        // e.g. "1.2.3.4 6112" -> ip: "1.2.3.4", port: "6112"

        string IP;
        uint32_t Port = 6112;
        stringstream SS;
        SS << Payload;
        SS >> IP;

        if ( !SS.eof( ) )
          SS >> Port;

        if ( SS.fail( ) )
          Print( "[GAME: " + m_GameName + "] bad inputs to sendlan command" );
        else
        {
            // construct a fixed host counter which will be used to identify players from this "realm" (i.e. LAN)
                    // the fixed host counter's 4 most significant bits will contain a 4 bit ID (0-15)
                    // the rest of the fixed host counter will contain the 28 least significant bits of the actual host counter
                    // since we're destroying 4 bits of information here the actual host counter should not be greater than 2^28 which is a reasonable assumption
                    // when a player joins a game we can obtain the ID from the received host counter
                    // note: LAN broadcasts use an ID of 0, battle.net refreshes use an ID of 1-10, the rest are unused

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

          m_Aura->m_UDPSocket->SendTo( IP, Port, m_Protocol->SEND_W3GS_GAMEINFO( m_Aura->m_LANWar3Version, UTIL_CreateByteArray( (uint32_t) MAPGAMETYPE_UNKNOWN0, false ), m_Map->GetMapGameFlags( ), m_Map->GetMapWidth( ), m_Map->GetMapHeight( ), m_GameName, "Clan 007", 0, m_Map->GetMapPath( ), m_Map->GetMapCRC( ), 12, 12, m_HostPort, m_HostCounter & 0x0FFFFFFF, m_EntryKey ) );
        }
      }

        //
        // !OWNER (set game owner)
        //

      else if ( Command == "owner" )
      {
        if ( RootAdminCheck || IsOwner( User ) || !GetPlayerFromName( m_OwnerName, false ) )
        {
          if ( !Payload.empty( ) )
          {
            SendAllChat( "Setting game owner to [" + Payload + "]" );
            m_OwnerName = Payload;
          }
          else
          {
            SendAllChat( "Setting game owner to [" + User + "]" );
            m_OwnerName = User;
          }
        }
        else
          SendAllChat( "Unable to set game owner because you are not the owner and the owner is in the game. The owner is [" + m_OwnerName + "]" );
      }

        //
        // !SAY
        //

      else if ( Command == "say" && !Payload.empty( ) )
      {
        for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
          (*i)->QueueChatCommand( Payload );
      }

        //
        // !CLOSEALL
        //

      else if ( Command == "closeall" && !m_GameLoading && !m_GameLoaded )
        CloseAllSlots( );

        //
        // !COMP (computer slot)
        //

      else if ( Command == "comp" && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
      {
        // extract the slot and the skill
        // e.g. "1 2" -> slot: "1", skill: "2"

        uint32_t Slot;
        stringstream SS;
        SS << Payload;
        SS >> Slot;

        if ( SS.fail( ) )
          Print( "[GAME: " + m_GameName + "] bad input #1 to comp command" );
        else
        {
          uint32_t Skill;
          
          if ( !SS.eof( ) )
            SS >> Skill;

          if ( SS.fail( ) )
            Print( "[GAME: " + m_GameName + "] bad input #2 to comp command" );
          else
            ComputerSlot( (unsigned char) ( Slot - 1 ), (unsigned char) Skill, true );
        }
      }

        //
        // !COMPCOLOUR (computer colour change)
        //

      else if ( Command == "compcolour" && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
      {
        // extract the slot and the colour
        // e.g. "1 2" -> slot: "1", colour: "2"

        uint32_t Slot, Colour;
        stringstream SS;
        SS << Payload;
        SS >> Slot;

        if ( SS.fail( ) )
          Print( "[GAME: " + m_GameName + "] bad input #1 to compcolour command" );
        else
        {
          if ( SS.eof( ) )
            Print( "[GAME: " + m_GameName + "] missing input #2 to compcolour command" );
          else
          {
            SS >> Colour;

            if ( SS.fail( ) )
              Print( "[GAME: " + m_GameName + "] bad input #2 to compcolour command" );
            else
            {
              unsigned char SID = (unsigned char) ( Slot - 1 );

              if ( !( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS ) && Colour < 12 && SID < m_Slots.size( ) )
              {
                if ( m_Slots[SID].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[SID].GetComputer( ) == 1 )
                  ColourSlot( SID, Colour );
              }
            }
          }
        }
      }

        //
        // !COMPHANDICAP (computer handicap change)
        //

      else if ( Command == "comphandicap" && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
      {
        // extract the slot and the handicap
        // e.g. "1 50" -> slot: "1", handicap: "50"

        uint32_t Slot, Handicap;
        stringstream SS;
        SS << Payload;
        SS >> Slot;

        if ( SS.fail( ) )
          Print( "[GAME: " + m_GameName + "] bad input #1 to comphandicap command" );
        else
        {
          if ( SS.eof( ) )
            Print( "[GAME: " + m_GameName + "] missing input #2 to comphandicap command" );
          else
          {
            SS >> Handicap;

            if ( SS.fail( ) )
              Print( "[GAME: " + m_GameName + "] bad input #2 to comphandicap command" );
            else
            {
              unsigned char SID = (unsigned char) ( Slot - 1 );

              if ( !( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS ) && ( Handicap == 50 || Handicap == 60 || Handicap == 70 || Handicap == 80 || Handicap == 90 || Handicap == 100 ) && SID < m_Slots.size( ) )
              {
                if ( m_Slots[SID].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[SID].GetComputer( ) == 1 )
                {
                  m_Slots[SID].SetHandicap( (unsigned char) Handicap );
                  SendAllSlotInfo( );
                }
              }
            }
          }
        }
      }

        //
        // !FAKEPLAYER
        //

      else if ( ( Command == "fakeplayer" || Command == "fp" ) && !m_CountDownStarted )
        CreateFakePlayer( );

        //
        // !DELETEFAKE

      else if ( ( Command == "deletefake" || Command == "deletefakes" || Command == "df" || Command == "deletefakes" ) && !m_FakePlayers.empty( ) && !m_CountDownStarted )
        DeleteFakePlayers( );

        //
        // !FPPAUSE
        //

      else if ( ( Command == "fppause" || Command == "fpp" ) && m_FakePlayers.size( ) && m_GameLoaded && !m_FakePlayers.empty( ) )
      {
        BYTEARRAY CRC, Action;
        Action.push_back( 1 );
        m_Actions.push( new CIncomingAction( m_FakePlayers[rand( ) % m_FakePlayers.size( )], CRC, Action ) );
      }

        //
        // !FPRESUME
        //

      else if ( ( Command == "fpresume" || Command == "fpr" ) && m_FakePlayers.size( ) && m_GameLoaded )
      {
        BYTEARRAY CRC, Action;
        Action.push_back( 2 );
        m_Actions.push( new CIncomingAction( m_FakePlayers[0], CRC, Action ) );
      }

        //
        // !SP
        //

      else if ( Command == "sp" && !m_CountDownStarted )
      {
        SendAllChat( "Shuffling players" );
        ShuffleSlots( );
      }

        //
        // !LOCK
        //

      else if ( Command == "lock" && ( RootAdminCheck || IsOwner( User ) ) )
      {
        SendAllChat( "Game locked. Only the game owner and root admins can run game commands" );
        m_Locked = true;
      }

        //
        // !OPENALL
        //

      else if ( Command == "openall" && !m_GameLoading && !m_GameLoaded )
        OpenAllSlots( );

        //
        // !UNLOCK
        //

      else if ( Command == "unlock" && ( RootAdminCheck || IsOwner( User ) ) )
      {
        SendAllChat( "Game unlocked. All admins can run game commands" );
        m_Locked = false;
      }

        //
        // !UNMUTE
        //

      else if ( Command == "unmute" )
      {
        CGamePlayer *LastMatch = NULL;
        uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

        if ( Matches == 0 )
          SendChat( player, "Unable to mute/unmute player [" + Payload + "]. No matches found" );
        else if ( Matches == 1 )
        {
          SendAllChat( "Player [" + LastMatch->GetName( ) + "] was unmuted by player [" + User + "]" );
          LastMatch->SetMuted( false );
        }
        else
          SendChat( player, "Unable to mute/unmute player [" + Payload + "]. Found more than one match" );
      }

        //
        // !UNMUTEALL
        //

      else if ( Command == "unmuteall" && m_GameLoaded )
      {
        SendAllChat( "Global chat unmuted" );
        m_MuteAll = false;
      }



        //
        // !VOTECANCEL
        //

      else if ( Command == "votecancel" && !m_KickVotePlayer.empty( ) )
      {
        SendAllChat( "A votekick against player [" + m_KickVotePlayer + "] has been cancelled" );
        m_KickVotePlayer.clear( );
        m_StartedKickVoteTime = 0;
      }

        //
        // !W
        //

      else if ( Command == "w" && !Payload.empty( ) )
      {
        // extract the name and the message
        // e.g. "Varlock hello there!" -> name: "Varlock", message: "hello there!"

        string Name, Message;
        string::size_type MessageStart = Payload.find( " " );

        if ( MessageStart != string::npos )
        {
          Name = Payload.substr( 0, MessageStart );
          Message = Payload.substr( MessageStart + 1 );

          for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
            (*i)->QueueChatCommand( Message, Name, true, string( ) );
        }
      }

        //
        // !WHOIS
        //

      else if ( Command == "whois" && !Payload.empty( ) )
      {
        for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
          (*i)->QueueChatCommand( "/whois " + Payload );
      }

        //
        // !COMPRACE (computer race change)
        //

      else if ( Command == "comprace" && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
      {
        // extract the slot and the race
        // e.g. "1 human" -> slot: "1", race: "human"

        uint32_t Slot;
        string Race;
        stringstream SS;
        SS << Payload;
        SS >> Slot;

        if ( SS.fail( ) )
          Print( "[GAME: " + m_GameName + "] bad input #1 to comprace command" );
        else
        {
          if ( SS.eof( ) )
            Print( "[GAME: " + m_GameName + "] missing input #2 to comprace command" );
          else
          {
            getline( SS, Race );
            string::size_type Start = Race.find_first_not_of( " " );

            if ( Start != string::npos )
              Race = Race.substr( Start );

            transform( Race.begin( ), Race.end( ), Race.begin( ), (int(* )(int) )tolower );
            unsigned char SID = (unsigned char) ( Slot - 1 );

            if ( !( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS ) && !( m_Map->GetMapFlags( ) & MAPFLAG_RANDOMRACES ) && SID < m_Slots.size( ) )
            {
              if ( m_Slots[SID].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[SID].GetComputer( ) == 1 )
              {
                if ( Race == "human" )
                {
                  m_Slots[SID].SetRace( SLOTRACE_HUMAN | SLOTRACE_SELECTABLE );
                  SendAllSlotInfo( );
                }
                else if ( Race == "orc" )
                {
                  m_Slots[SID].SetRace( SLOTRACE_ORC | SLOTRACE_SELECTABLE );
                  SendAllSlotInfo( );
                }
                else if ( Race == "night elf" )
                {
                  m_Slots[SID].SetRace( SLOTRACE_NIGHTELF | SLOTRACE_SELECTABLE );
                  SendAllSlotInfo( );
                }
                else if ( Race == "undead" )
                {
                  m_Slots[SID].SetRace( SLOTRACE_UNDEAD | SLOTRACE_SELECTABLE );
                  SendAllSlotInfo( );
                }
                else if ( Race == "random" )
                {
                  m_Slots[SID].SetRace( SLOTRACE_RANDOM | SLOTRACE_SELECTABLE );
                  SendAllSlotInfo( );
                }
                else
                  Print( "[GAME: " + m_GameName + "] unknown race [" + Race + "] sent to comprace command" );
              }
            }
          }
        }
      }

        //
        // !COMPTEAM (computer team change)
        //

      else if ( Command == "compteam" && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
      {
        // extract the slot and the team
        // e.g. "1 2" -> slot: "1", team: "2"

        uint32_t Slot, Team;
        stringstream SS;
        SS << Payload;
        SS >> Slot;

        if ( SS.fail( ) )
          Print( "[GAME: " + m_GameName + "] bad input #1 to compteam command" );
        else
        {
          if ( SS.eof( ) )
            Print( "[GAME: " + m_GameName + "] missing input #2 to compteam command" );
          else
          {
            SS >> Team;

            if ( SS.fail( ) )
              Print( "[GAME: " + m_GameName + "] bad input #2 to compteam command" );
            else
            {
              unsigned char SID = (unsigned char) ( Slot - 1 );

              if ( !( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS ) && Team < 12 && SID < m_Slots.size( ) )
              {
                if ( m_Slots[SID].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[SID].GetComputer( ) == 1 )
                {
                  m_Slots[SID].SetTeam( (unsigned char) ( Team - 1 ) );
                  SendAllSlotInfo( );
                }
              }
            }
          }
        }
      }

        //
        // !VIRTUALHOST
        //

      else if ( Command == "virtualhost" && !Payload.empty( ) && Payload.size( ) <= 15 && !m_CountDownStarted )
      {
        DeleteVirtualHost( );
        m_VirtualHostName = Payload;
      }
    }
    else
    {
      Print( "[GAME: " + m_GameName + "] admin command ignored, the game is locked" );
      SendChat( player, "Only the game owner and root admins can run game commands when the game is locked" );
    }
  }
  else
  {
    if ( !player->GetSpoofed( ) )
      Print( "[GAME: " + m_GameName + "] non-spoofchecked user [" + User + "] sent command [" + Command + "] with payload [" + Payload + "]" );
    else
      Print( "[GAME: " + m_GameName + "] non-admin [" + User + "] sent command [" + Command + "] with payload [" + Payload + "]" );
  }

  /*********************
   * NON ADMIN COMMANDS *
   *********************/

  //
  // !CHECKME
  //

  if ( Command == "checkme" )
    SendChat( player, "Checked player [" + User + "]. Ping: " + ( player->GetNumPings( ) > 0 ? UTIL_ToString( player->GetPing( m_Aura->m_LCPings ) ) + "ms" : "N/A" ) + ", From: " + m_Aura->m_DB->FromCheck( UTIL_ByteArrayToUInt32( player->GetExternalIP( ), true ) ) + ", Admin: " + ( AdminCheck || RootAdminCheck ? "Yes" : "No" ) + ", Owner: " + ( IsOwner( User ) ? "Yes" : "No" ) + ", Spoof Checked: " + ( player->GetSpoofed( ) ? "Yes" : "No" ) + ", Realm: " + ( player->GetJoinedRealm( ).empty( ) ? "LAN" : player->GetJoinedRealm( ) ) + ", Reserved: " + ( player->GetReserved( ) ? "Yes" : "No" ) );  

    //
    // !STATS
    //

  else if ( Command == "stats" )
  {
    string StatsUser = User;

    if ( !Payload.empty( ) )
      StatsUser = Payload;

    if ( !StatsUser.empty( ) && StatsUser.size( ) < 16 && StatsUser[0] != '/' )
    {
      CDBGamePlayerSummary *GamePlayerSummary = m_Aura->m_DB->GamePlayerSummaryCheck( StatsUser );

      if ( GamePlayerSummary )
      {
        if ( player->GetSpoofed( ) && ( m_Aura->m_DB->AdminCheck( player->GetSpoofedRealm( ), User ) || RootAdminCheck || IsOwner( User ) ) )
          SendAllChat( "[" + StatsUser + "] has played " + UTIL_ToString( GamePlayerSummary->GetTotalGames( ) ) + " games with this bot. Average loading time: " + UTIL_ToString( (float) GamePlayerSummary->GetAvgLoadingTime( ) / 1000, 2 ) + " seconds. Average stay: " + UTIL_ToString( GamePlayerSummary->GetAvgLeftPercent( ) ) + " percent" );
        else
          SendChat( player, "[" + StatsUser + "] has played " + UTIL_ToString( GamePlayerSummary->GetTotalGames( ) ) + " games with this bot. Average loading time: " + UTIL_ToString( (float) GamePlayerSummary->GetAvgLoadingTime( ) / 1000, 2 ) + " seconds. Average stay: " + UTIL_ToString( GamePlayerSummary->GetAvgLeftPercent( ) ) + " percent" );

        delete GamePlayerSummary;
      }
      else
      {
        if ( player->GetSpoofed( ) && ( m_Aura->m_DB->AdminCheck( player->GetSpoofedRealm( ), User ) || RootAdminCheck || IsOwner( User ) ) )
          SendAllChat( "[" + StatsUser + "] hasn't played any games here" );
        else
          SendChat( player, "[" + StatsUser + "] hasn't played any games here" );
      }
    }
  }

    //
    // !STATSDOTA
    // !SD
    //

  else if ( Command == "statsdota" || Command == "sd" )
  {
    string StatsUser = User;

    if ( !Payload.empty( ) )
      StatsUser = Payload;

    if ( !StatsUser.empty( ) && StatsUser.size( ) < 16 && StatsUser[0] != '/' )
    {
      CDBDotAPlayerSummary *DotAPlayerSummary = m_Aura->m_DB->DotAPlayerSummaryCheck( StatsUser );

      if ( DotAPlayerSummary )
      {
        const string Summary = StatsUser + " - " + UTIL_ToString( DotAPlayerSummary->GetTotalGames( ) ) + " games (W/L: " + UTIL_ToString( DotAPlayerSummary->GetTotalWins( ) ) + "/" + UTIL_ToString( DotAPlayerSummary->GetTotalLosses( ) ) + ") Hero K/D/A: " + UTIL_ToString( DotAPlayerSummary->GetTotalKills( ) ) + "/" + UTIL_ToString( DotAPlayerSummary->GetTotalDeaths( ) ) + "/" + UTIL_ToString( DotAPlayerSummary->GetTotalAssists( ) ) + " (" + UTIL_ToString( DotAPlayerSummary->GetAvgKills( ), 2 ) + "/" + UTIL_ToString( DotAPlayerSummary->GetAvgDeaths( ), 2 ) + "/" + UTIL_ToString( DotAPlayerSummary->GetAvgAssists( ), 2 ) + ") Creep K/D/N: " + UTIL_ToString( DotAPlayerSummary->GetTotalCreepKills( ) ) + "/" + UTIL_ToString( DotAPlayerSummary->GetTotalCreepDenies( ) ) + "/" + UTIL_ToString( DotAPlayerSummary->GetTotalNeutralKills( ) ) + " (" + UTIL_ToString( DotAPlayerSummary->GetAvgCreepKills( ), 2 ) + "/" + UTIL_ToString( DotAPlayerSummary->GetAvgCreepDenies( ), 2 ) + "/" + UTIL_ToString( DotAPlayerSummary->GetAvgNeutralKills( ), 2 ) +") T/R/C: " + UTIL_ToString( DotAPlayerSummary->GetTotalTowerKills( ) ) + "/" + UTIL_ToString( DotAPlayerSummary->GetTotalRaxKills( ) ) + "/" + UTIL_ToString( DotAPlayerSummary->GetTotalCourierKills( ) );

        if ( player->GetSpoofed( ) && ( m_Aura->m_DB->AdminCheck( player->GetSpoofedRealm( ), User ) || RootAdminCheck || IsOwner( User ) ) )
          SendAllChat( Summary );
        else
          SendChat( player, Summary );

        delete DotAPlayerSummary;
      }
      else
      {
        if ( player->GetSpoofed( ) && ( m_Aura->m_DB->AdminCheck( player->GetSpoofedRealm( ), User ) || RootAdminCheck || IsOwner( User ) ) )
          SendAllChat( "[" + StatsUser + "] hasn't played any DotA games here" );
        else
          SendChat( player, "[" + StatsUser + "] hasn't played any DotA games here" );
      }
    }
  }

    //
    // !VERSION
    //

  else if ( Command == "version" )
  {
    SendChat( player, "Version: Aura " + m_Aura->m_Version );
  }

    //
    // !VOTEKICK
    //

  else if ( Command == "votekick" && !Payload.empty( ) )
  {
    if ( !m_KickVotePlayer.empty( ) )
      SendChat( player, "Unable to start votekick. Another votekick is in progress" );
    else if ( m_Players.size( ) == 2 )
      SendChat( player, "Unable to start votekick. There aren't enough players in the game for a votekick" );
    else
    {
      CGamePlayer *LastMatch = NULL;
      uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

      if ( Matches == 0 )
        SendChat( player, "Unable to votekick player [" + Payload + "]. No matches found" );
      else if ( Matches == 1 )
      {
        if ( LastMatch->GetReserved( ) )
          SendChat( player, "Unable to votekick player [" + LastMatch->GetName( ) + "]. That player is reserved and cannot be votekicked" );
        else
        {
          m_KickVotePlayer = LastMatch->GetName( );
          m_StartedKickVoteTime = GetTime( );

          for ( vector<CGamePlayer *> ::iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
            (*i)->SetKickVote( false );

          player->SetKickVote( true );
          Print( "[GAME: " + m_GameName + "] votekick against player [" + m_KickVotePlayer + "] started by player [" + User + "]" );
          SendAllChat( "Player [" + User + "] voted to kick player [" + LastMatch->GetName( ) + "]. " + UTIL_ToString( (uint32_t) ceil( ( GetNumHumanPlayers( ) - 1 ) * (float) m_Aura->m_VoteKickPercentage / 100 ) - 1 ) + " more votes are needed to pass" );          
          SendAllChat( "Type " + string( 1, m_Aura->m_CommandTrigger ) + "yes to vote" );
        }
      }
      else
        SendChat( player, "Unable to votekick player [" + Payload + "]. Found more than one match" );
    }
  }

    //
    // !YES
    //

  else if ( Command == "yes" && !m_KickVotePlayer.empty( ) && player->GetName( ) != m_KickVotePlayer && !player->GetKickVote( ) )
  {
    player->SetKickVote( true );
    uint32_t Votes = 0, VotesNeeded = (uint32_t) ceil( ( GetNumHumanPlayers( ) - 1 ) * (float) m_Aura->m_VoteKickPercentage / 100 );

    for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
    {
      if ( (*i)->GetKickVote( ) )
        ++Votes;
    }

    if ( Votes >= VotesNeeded )
    {
      CGamePlayer *Victim = GetPlayerFromName( m_KickVotePlayer, true );

      if ( Victim )
      {
        Victim->SetDeleteMe( true );
        Victim->SetLeftReason( "was kicked by vote" );

        if ( !m_GameLoading && !m_GameLoaded )
          Victim->SetLeftCode( PLAYERLEAVE_LOBBY );
        else
          Victim->SetLeftCode( PLAYERLEAVE_LOST );

        if ( !m_GameLoading && !m_GameLoaded )
          OpenSlot( GetSIDFromPID( Victim->GetPID( ) ), false );

        Print( "[GAME: " + m_GameName + "] votekick against player [" + m_KickVotePlayer + "] passed with " + UTIL_ToString( Votes ) + "/" + UTIL_ToString( GetNumHumanPlayers( ) ) + " votes" );
        SendAllChat( "A votekick against player [" + m_KickVotePlayer + "] has passed" );
      }
      else
        SendAllChat( "Error votekicking player [" + m_KickVotePlayer + "]" );

      m_KickVotePlayer.clear( );
      m_StartedKickVoteTime = 0;
    }
    else
      SendAllChat( "Player [" + User + "] voted to kick player [" + m_KickVotePlayer + "]. " + UTIL_ToString( VotesNeeded - Votes ) + " more votes are needed to pass" );
  }

  return true;
}

void CGame::EventPlayerChangeTeam( CGamePlayer *player, unsigned char team )
{
  // player is requesting a team change

  if ( m_Map->GetMapOptions( ) & MAPOPT_CUSTOMFORCES )
  {
    unsigned char oldSID = GetSIDFromPID( player->GetPID( ) );
    unsigned char newSID = GetEmptySlot( team, player->GetPID( ) );
    SwapSlots( oldSID, newSID );
  }
  else
  {
    if ( team > 12 )
      return;

    if ( team == 12 )
    {
      if ( m_Map->GetMapObservers( ) != MAPOBS_ALLOWED && m_Map->GetMapObservers( ) != MAPOBS_REFEREES )
        return;
    }
    else
    {
      if ( team >= m_Map->GetMapNumPlayers( ) )
        return;

      // make sure there aren't too many other players already

      unsigned char NumOtherPlayers = 0;

      for ( unsigned char i = 0; i < m_Slots.size( ); ++i )
      {
        if ( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[i].GetTeam( ) != 12 && m_Slots[i].GetPID( ) != player->GetPID( ) )
          ++NumOtherPlayers;
      }

      if ( NumOtherPlayers >= m_Map->GetMapNumPlayers( ) )
        return;
    }

    unsigned char SID = GetSIDFromPID( player->GetPID( ) );

    if ( SID < m_Slots.size( ) )
    {
      m_Slots[SID].SetTeam( team );

      if ( team == 12 )
      {
        // if they're joining the observer team give them the observer colour

        m_Slots[SID].SetColour( 12 );
      }
      else if ( m_Slots[SID].GetColour( ) == 12 )
      {
        // if they're joining a regular team give them an unused colour

        m_Slots[SID].SetColour( GetNewColour( ) );
      }

      SendAllSlotInfo( );
    }
  }
}

void CGame::EventPlayerChangeColour( CGamePlayer *player, unsigned char colour )
{
  // player is requesting a colour change

  if ( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS )
    return;

  if ( colour > 11 )
    return;

  unsigned char SID = GetSIDFromPID( player->GetPID( ) );

  if ( SID < m_Slots.size( ) )
  {
    // make sure the player isn't an observer

    if ( m_Slots[SID].GetTeam( ) == 12 )
      return;

    ColourSlot( SID, colour );
  }
}

void CGame::EventPlayerChangeRace( CGamePlayer *player, unsigned char race )
{
  // player is requesting a race change

  if ( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS )
    return;

  if ( m_Map->GetMapFlags( ) & MAPFLAG_RANDOMRACES )
    return;

  if ( race != SLOTRACE_HUMAN && race != SLOTRACE_ORC && race != SLOTRACE_NIGHTELF && race != SLOTRACE_UNDEAD && race != SLOTRACE_RANDOM )
    return;

  unsigned char SID = GetSIDFromPID( player->GetPID( ) );

  if ( SID < m_Slots.size( ) )
  {
    m_Slots[SID].SetRace( race | SLOTRACE_SELECTABLE );
    SendAllSlotInfo( );
  }
}

void CGame::EventPlayerChangeHandicap( CGamePlayer *player, unsigned char handicap )
{
  // player is requesting a handicap change

  if ( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS )
    return;

  if ( handicap != 50 && handicap != 60 && handicap != 70 && handicap != 80 && handicap != 90 && handicap != 100 )
    return;

  unsigned char SID = GetSIDFromPID( player->GetPID( ) );

  if ( SID < m_Slots.size( ) )
  {
    m_Slots[SID].SetHandicap( handicap );
    SendAllSlotInfo( );
  }
}

void CGame::EventPlayerDropRequest( CGamePlayer *player )
{
  // TODO: check that we've waited the full 45 seconds

  if ( m_Lagging )
  {
    Print( "[GAME: " + m_GameName + "] player [" + player->GetName( ) + "] voted to drop laggers" );
    SendAllChat( "Player [" + player->GetName( ) + "] voted to drop laggers" );

    // check if at least half the players voted to drop

    int Votes = 0;

    for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
    {
      if ( (*i)->GetDropVote( ) )
        ++Votes;
    }

    if ( (float) Votes / m_Players.size( ) > 0.50f )
      StopLaggers( "lagged out (dropped by vote)" );
  }
}

void CGame::EventPlayerMapSize( CGamePlayer *player, CIncomingMapSize *mapSize )
{
  if ( m_GameLoading || m_GameLoaded )
    return;

  uint32_t MapSize = UTIL_ByteArrayToUInt32( m_Map->GetMapSize( ), false );

  bool Admin = m_Aura->m_DB->AdminCheck( player->GetName( ) ) || m_Aura->m_DB->RootAdminCheck( player->GetName( ) );

  if ( mapSize->GetSizeFlag( ) != 1 || mapSize->GetMapSize( ) != MapSize )
  {
    // the player doesn't have the map

    if ( Admin || m_Aura->m_AllowDownloads )
    {
      string *MapData = m_Map->GetMapData( );

      if ( !MapData->empty( ) )
      {
        if ( Admin || m_Aura->m_AllowDownloads == 1 || ( m_Aura->m_AllowDownloads == 2 && player->GetDownloadAllowed( ) ) )
        {
          if ( !player->GetDownloadStarted( ) && mapSize->GetSizeFlag( ) == 1 )
          {
            // inform the client that we are willing to send the map

            Print( "[GAME: " + m_GameName + "] map download started for player [" + player->GetName( ) + "]" );
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
  else if ( player->GetDownloadStarted( ) )
  {
    // calculate download rate

    float Seconds = (float) ( GetTicks( ) - player->GetStartedDownloadingTicks( ) ) / 1000.f;
    float Rate = (float) MapSize / 1024.f / Seconds;
    Print( "[GAME: " + m_GameName + "] map download finished for player [" + player->GetName( ) + "] in " + UTIL_ToString( Seconds, 1 ) + " seconds" );
    SendAllChat( "Player [" + player->GetName( ) + "] downloaded the map in " + UTIL_ToString( Seconds, 1 ) + " seconds (" + UTIL_ToString( Rate, 1 ) + " KB/sec)" );
    player->SetDownloadFinished( true );
    player->SetFinishedDownloadingTime( GetTime( ) );
  }

  unsigned char NewDownloadStatus = (unsigned char) ( (float) mapSize->GetMapSize( ) / MapSize * 100.f );
  unsigned char SID = GetSIDFromPID( player->GetPID( ) );

  if ( NewDownloadStatus > 100 )
    NewDownloadStatus = 100;

  if ( SID < m_Slots.size( ) )
  {
    // only send the slot info if the download status changed

    if ( m_Slots[SID].GetDownloadStatus( ) != NewDownloadStatus )
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

void CGame::EventPlayerPongToHost( CGamePlayer *player )
{
  // autokick players with excessive pings but only if they're not reserved and we've received at least 3 pings from them
  // also don't kick anyone if the game is loading or loaded - this could happen because we send pings during loading but we stop sending them after the game is loaded
  // see the Update function for where we send pings

  if ( !m_GameLoading && !m_GameLoaded && !player->GetDeleteMe( ) && !player->GetReserved( ) && player->GetNumPings( ) >= 3 && player->GetPing( m_Aura->m_LCPings ) > m_Aura->m_AutoKickPing )
  {
    // send a chat message because we don't normally do so when a player leaves the lobby

    SendAllChat( "Autokicking player [" + player->GetName( )  + "] for excessive ping of " + UTIL_ToString( player->GetPing( m_Aura->m_LCPings ) ) );
    player->SetDeleteMe( true );
    player->SetLeftReason( "was autokicked for excessive ping of " + UTIL_ToString( player->GetPing( m_Aura->m_LCPings ) ) );
    player->SetLeftCode( PLAYERLEAVE_LOBBY );
    OpenSlot( GetSIDFromPID( player->GetPID( ) ), false );
  }
}

void CGame::EventGameStarted( )
{
  Print( "[GAME: " + m_GameName + "] started loading with " + UTIL_ToString( GetNumHumanPlayers( ) ) + " players" );

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

  if ( !m_HCLCommandString.empty( ) )
  {
    if ( m_HCLCommandString.size( ) <= GetSlotsOccupied( ) )
    {
      string HCLChars = "abcdefghijklmnopqrstuvwxyz0123456789 -=,.";

      if ( m_HCLCommandString.find_first_not_of( HCLChars ) == string::npos )
      {
        unsigned char EncodingMap[256];
        unsigned char j = 0;

        for ( uint32_t i = 0; i < 256; ++i )
        {
          // the following 7 handicap values are forbidden

          if ( j == 0 || j == 50 || j == 60 || j == 70 || j == 80 || j == 90 || j == 100 )
            ++j;

          EncodingMap[i] = j++;
        }

        unsigned char CurrentSlot = 0;

        for ( string::const_iterator si = m_HCLCommandString.begin( ); si != m_HCLCommandString.end( ); ++si )
        {
          while ( m_Slots[CurrentSlot].GetSlotStatus( ) != SLOTSTATUS_OCCUPIED )
            ++CurrentSlot;

          unsigned char HandicapIndex = ( m_Slots[CurrentSlot].GetHandicap( ) - 50 ) / 10;
          unsigned char CharIndex = HCLChars.find( *si );
          m_Slots[CurrentSlot++].SetHandicap( EncodingMap[HandicapIndex + CharIndex * 6] );
        }

        SendAllSlotInfo( );
        Print( "[GAME: " + m_GameName + "] successfully encoded HCL command string [" + m_HCLCommandString + "]" );
      }
      else
        Print( "[GAME: " + m_GameName + "] encoding HCL command string [" + m_HCLCommandString + "] failed because it contains invalid characters" );
    }
    else
      Print( "[GAME: " + m_GameName + "] encoding HCL command string [" + m_HCLCommandString + "] failed because there aren't enough occupied slots" );
  }

  // send a final slot info update if necessary
  // this typically won't happen because we prevent the !start command from completing while someone is downloading the map
  // however, if someone uses !start force while a player is downloading the map this could trigger
  // this is because we only permit slot info updates to be flagged when it's just a change in download status, all others are sent immediately
  // it might not be necessary but let's clean up the mess anyway

  if ( m_SlotInfoChanged )
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

  for ( vector<unsigned char> ::const_iterator i = m_FakePlayers.begin( ); i != m_FakePlayers.end( ); ++i )
    SendAll( m_Protocol->SEND_W3GS_GAMELOADED_OTHERS(*i) );

  // record the starting number of players

  m_StartPlayers = GetNumHumanPlayers( );

  // close the listening socket

  delete m_Socket;
  m_Socket = NULL;

  // delete any potential players that are still hanging around

  for ( vector<CPotentialPlayer *> ::const_iterator i = m_Potentials.begin( ); i != m_Potentials.end( ); ++i )
    delete *i;

  m_Potentials.clear( );

  // delete the map data

  delete m_Map;
  m_Map = NULL;

  // move the game to the games in progress vector

  m_Aura->m_CurrentGame = NULL;
  m_Aura->m_Games.push_back( this );

  // and finally reenter battle.net chat

  for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
  {
    (*i)->QueueGameUncreate( );
    (*i)->QueueEnterChat( );

    if ( (*i)->GetSpam( ) )
    {
      (*i)->SetSpam( false );
      (*i)->SendJoinChannel( (*i)->GetFirstChannel( ) );
    }
  }

  // record everything we need to ban each player in case we decide to do so later
  // this is because when a player leaves the game an admin might want to ban that player
  // but since the player has already left the game we don't have access to their information anymore
  // so we create a "potential ban" for each player and only store it in the database if requested to by an admin

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    m_DBBans.push_back( new CDBBan( (*i)->GetJoinedRealm( ), (*i)->GetName( ), (*i)->GetExternalIPString( ), string( ), string( ), string( ), string( ) ) );
  }
}

void CGame::EventGameLoaded( )
{
  Print( "[GAME: " + m_GameName + "] finished loading with " + UTIL_ToString( GetNumHumanPlayers( ) ) + " players" );

  // send shortest, longest, and personal load times to each player

  CGamePlayer *Shortest = NULL;
  CGamePlayer *Longest = NULL;

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    if ( !Shortest || (*i)->GetFinishedLoadingTicks( ) < Shortest->GetFinishedLoadingTicks( ) )
      Shortest = *i;

    if ( !Longest || (*i)->GetFinishedLoadingTicks( ) > Longest->GetFinishedLoadingTicks( ) )
      Longest = *i;
  }

  if ( Shortest && Longest )
  {
    SendAllChat( "Shortest load by player [" + Shortest->GetName( ) + "] was " + UTIL_ToString( (float) ( Shortest->GetFinishedLoadingTicks( ) - m_StartedLoadingTicks ) / 1000.f, 2 ) + " seconds" );  
    SendAllChat( "Longest load by player [" + Longest->GetName( ) + "] was " + UTIL_ToString( (float) ( Longest->GetFinishedLoadingTicks( ) - m_StartedLoadingTicks ) / 1000.f, 2 ) + " seconds" );
  }

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
    SendChat( *i, "Your load time was " + UTIL_ToString( (float) ( (*i)->GetFinishedLoadingTicks( ) - m_StartedLoadingTicks ) / 1000.f, 2 ) + " seconds" );    
}

unsigned char CGame::GetSIDFromPID( unsigned char PID )
{
  if ( m_Slots.size( ) > 255 )
    return 255;

  for ( unsigned char i = 0; i < m_Slots.size( ); ++i )
  {
    if ( m_Slots[i].GetPID( ) == PID )
      return i;
  }

  return 255;
}

CGamePlayer *CGame::GetPlayerFromPID( unsigned char PID )
{
  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    if ( !(*i)->GetLeftMessageSent( ) && (*i)->GetPID( ) == PID )
      return *i;
  }

  return NULL;
}

CGamePlayer *CGame::GetPlayerFromSID( unsigned char SID )
{
  if ( SID < m_Slots.size( ) )
    return GetPlayerFromPID( m_Slots[SID].GetPID( ) );

  return NULL;
}

CGamePlayer *CGame::GetPlayerFromName( string name, bool sensitive )
{
  if ( !sensitive )
    transform( name.begin( ), name.end( ), name.begin( ), (int(* )(int) )tolower );

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    if ( !(*i)->GetLeftMessageSent( ) )
    {
      string TestName = (*i)->GetName( );

      if ( !sensitive )
        transform( TestName.begin( ), TestName.end( ), TestName.begin( ), (int(* )(int) )tolower );

      if ( TestName == name )
        return *i;
    }
  }

  return NULL;
}

uint32_t CGame::GetPlayerFromNamePartial( string name, CGamePlayer **player )
{
  transform( name.begin( ), name.end( ), name.begin( ), (int(* )(int) )tolower );
  uint32_t Matches = 0;
  *player = NULL;

  // try to match each player with the passed string (e.g. "Varlock" would be matched with "lock")

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    if ( !(*i)->GetLeftMessageSent( ) )
    {
      string TestName = (*i)->GetName( );
      transform( TestName.begin( ), TestName.end( ), TestName.begin( ), (int(* )(int) )tolower );

      if ( TestName.find( name ) != string::npos )
      {
        ++Matches;
        *player = *i;

        // if the name matches exactly stop any further matching

        if ( TestName == name )
        {
          Matches = 1;
          break;
        }
      }
    }
  }

  return Matches;
}

CGamePlayer *CGame::GetPlayerFromColour( unsigned char colour )
{
  for ( unsigned char i = 0; i < m_Slots.size( ); ++i )
  {
    if ( m_Slots[i].GetColour( ) == colour )
      return GetPlayerFromSID( i );
  }

  return NULL;
}

unsigned char CGame::GetNewPID( )
{
  // find an unused PID for a new player to use

  for ( unsigned char TestPID = 1; TestPID < 255; ++TestPID )
  {
    if ( TestPID == m_VirtualHostPID )
      continue;

    bool InUse = false;

    for ( vector<unsigned char> ::const_iterator i = m_FakePlayers.begin( ); i != m_FakePlayers.end( ); ++i )
    {
      if ( *i == TestPID )
      {
        InUse = true;
        break;
      }
    }

    if ( InUse )
      continue;

    for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
    {
      if ( !(*i)->GetLeftMessageSent( ) && (*i)->GetPID( ) == TestPID )
      {
        InUse = true;
        break;
      }
    }

    if ( !InUse )
      return TestPID;
  }

  // this should never happen

  return 255;
}

unsigned char CGame::GetNewColour( )
{
  // find an unused colour for a player to use

  for ( unsigned char TestColour = 0; TestColour < 12; ++TestColour )
  {
    bool InUse = false;

    for ( unsigned char i = 0; i < m_Slots.size( ); ++i )
    {
      if ( m_Slots[i].GetColour( ) == TestColour )
      {
        InUse = true;
        break;
      }
    }

    if ( !InUse )
      return TestColour;
  }

  // this should never happen

  return 12;
}

BYTEARRAY CGame::GetPIDs( )
{
  BYTEARRAY result;

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    if ( !(*i)->GetLeftMessageSent( ) )
      result.push_back( (*i)->GetPID( ) );
  }

  return result;
}

BYTEARRAY CGame::GetPIDs( unsigned char excludePID )
{
  BYTEARRAY result;

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    if ( !(*i)->GetLeftMessageSent( ) && (*i)->GetPID( ) != excludePID )
      result.push_back( (*i)->GetPID( ) );
  }

  return result;
}

unsigned char CGame::GetHostPID( )
{
  // return the player to be considered the host (it can be any player) - mainly used for sending text messages from the bot
  // try to find the virtual host player first

  if ( m_VirtualHostPID != 255 )
    return m_VirtualHostPID;

  // try to find the fakeplayer next

  if ( !m_FakePlayers.empty( ) )
    return m_FakePlayers[0];

  // try to find the owner player next

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    if ( !(*i)->GetLeftMessageSent( ) && IsOwner( (*i)->GetName( ) ) )
      return (*i )->GetPID( );
  }

  // okay then, just use the first available player

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    if ( !(*i)->GetLeftMessageSent( ) )
      return (*i )->GetPID( );
  }

  return 255;
}

unsigned char CGame::GetEmptySlot( bool reserved )
{
  if ( m_Slots.size( ) > 255 )
    return 255;

  // look for an empty slot for a new player to occupy
  // if reserved is true then we're willing to use closed or occupied slots as long as it wouldn't displace a player with a reserved slot

  for ( unsigned char i = 0; i < m_Slots.size( ); ++i )
  {
    if ( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN )
      return i;
  }

  if ( reserved )
  {
    // no empty slots, but since player is reserved give them a closed slot

    for ( unsigned char i = 0; i < m_Slots.size( ); ++i )
    {
      if ( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_CLOSED )
        return i;
    }

    // no closed slots either, give them an occupied slot but not one occupied by another reserved player
    // first look for a player who is downloading the map and has the least amount downloaded so far

    unsigned char LeastDownloaded = 100;
    unsigned char LeastSID = 255;

    for ( unsigned char i = 0; i < m_Slots.size( ); ++i )
    {
      CGamePlayer *Player = GetPlayerFromSID( i );

      if ( Player && !Player->GetReserved( ) && m_Slots[i].GetDownloadStatus( ) < LeastDownloaded )
      {
        LeastDownloaded = m_Slots[i].GetDownloadStatus( );
        LeastSID = i;
      }
    }

    if ( LeastSID != 255 )
      return LeastSID;

    // nobody who isn't reserved is downloading the map, just choose the first player who isn't reserved

    for ( unsigned char i = 0; i < m_Slots.size( ); ++i )
    {
      CGamePlayer *Player = GetPlayerFromSID( i );

      if ( Player && !Player->GetReserved( ) )
        return i;
    }
  }

  return 255;
}

unsigned char CGame::GetEmptySlot( unsigned char team, unsigned char PID )
{
  if ( m_Slots.size( ) > 255 )
    return 255;

  // find an empty slot based on player's current slot

  unsigned char StartSlot = GetSIDFromPID( PID );

  if ( StartSlot < m_Slots.size( ) )
  {
    if ( m_Slots[StartSlot].GetTeam( ) != team )
    {
      // player is trying to move to another team so start looking from the first slot on that team
      // we actually just start looking from the very first slot since the next few loops will check the team for us

      StartSlot = 0;
    }

    // find an empty slot on the correct team starting from StartSlot

    for ( unsigned char i = StartSlot; i < m_Slots.size( ); ++i )
    {
      if ( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN && m_Slots[i].GetTeam( ) == team )
        return i;
    }

    // didn't find an empty slot, but we could have missed one with SID < StartSlot
    // e.g. in the DotA case where I am in slot 4 (yellow), slot 5 (orange) is occupied, and slot 1 (blue) is open and I am trying to move to another slot

    for ( unsigned char i = 0; i < StartSlot; ++i )
    {
      if ( m_Slots[i].GetSlotStatus( ) == SLOTSTATUS_OPEN && m_Slots[i].GetTeam( ) == team )
        return i;
    }
  }

  return 255;
}

void CGame::SwapSlots( unsigned char SID1, unsigned char SID2 )
{
  if ( SID1 < m_Slots.size( ) && SID2 < m_Slots.size( ) && SID1 != SID2 )
  {
    CGameSlot Slot1 = m_Slots[SID1];
    CGameSlot Slot2 = m_Slots[SID2];

    if ( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS )
    {
      // don't swap the team, colour, race, or handicap
      m_Slots[SID1] = CGameSlot( Slot2.GetPID( ), Slot2.GetDownloadStatus( ), Slot2.GetSlotStatus( ), Slot2.GetComputer( ), Slot1.GetTeam( ), Slot1.GetColour( ), Slot1.GetRace( ), Slot2.GetComputerType( ), Slot1.GetHandicap( ) );
      m_Slots[SID2] = CGameSlot( Slot1.GetPID( ), Slot1.GetDownloadStatus( ), Slot1.GetSlotStatus( ), Slot1.GetComputer( ), Slot2.GetTeam( ), Slot2.GetColour( ), Slot2.GetRace( ), Slot1.GetComputerType( ), Slot2.GetHandicap( ) );
    }
    else
    {
      // swap everything

      if ( m_Map->GetMapOptions( ) & MAPOPT_CUSTOMFORCES )
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

void CGame::OpenSlot( unsigned char SID, bool kick )
{
  if ( SID < m_Slots.size( ) )
  {
    if ( kick )
    {
      CGamePlayer *Player = GetPlayerFromSID( SID );

      if ( Player )
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

void CGame::CloseSlot( unsigned char SID, bool kick )
{
  if ( SID < m_Slots.size( ) )
  {
    if ( kick )
    {
      CGamePlayer *Player = GetPlayerFromSID( SID );

      if ( Player )
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

void CGame::ComputerSlot( unsigned char SID, unsigned char skill, bool kick )
{
  if ( SID < m_Slots.size( ) && skill < 3 )
  {
    if ( kick )
    {
      CGamePlayer *Player = GetPlayerFromSID( SID );

      if ( Player )
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

void CGame::ColourSlot( unsigned char SID, unsigned char colour )
{
  if ( SID < m_Slots.size( ) && colour < 12 )
  {
    // make sure the requested colour isn't already taken

    bool Taken = false;
    unsigned char TakenSID = 0;

    for ( unsigned char i = 0; i < m_Slots.size( ); ++i )
    {
      if ( m_Slots[i].GetColour( ) == colour )
      {
        TakenSID = i;
        Taken = true;
      }
    }

    if ( Taken && m_Slots[TakenSID].GetSlotStatus( ) != SLOTSTATUS_OCCUPIED )
    {
      // the requested colour is currently "taken" by an unused (open or closed) slot
      // but we allow the colour to persist within a slot so if we only update the existing player's colour the unused slot will have the same colour
      // this isn't really a problem except that if someone then joins the game they'll receive the unused slot's colour resulting in a duplicate
      // one way to solve this (which we do here) is to swap the player's current colour into the unused slot

      m_Slots[TakenSID].SetColour( m_Slots[SID].GetColour( ) );
      m_Slots[SID].SetColour( colour );
      SendAllSlotInfo( );
    }
    else if ( !Taken )
    {
      // the requested colour isn't used by ANY slot

      m_Slots[SID].SetColour( colour );
      SendAllSlotInfo( );
    }
  }
}

void CGame::OpenAllSlots( )
{
  bool Changed = false;

  for ( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); ++i )
  {
    if ( (*i).GetSlotStatus( ) == SLOTSTATUS_CLOSED )
    {
      (*i).SetSlotStatus( SLOTSTATUS_OPEN );
      Changed = true;
    }
  }

  if ( Changed )
    SendAllSlotInfo( );
}

void CGame::CloseAllSlots( )
{
  bool Changed = false;

  for ( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); ++i )
  {
    if ( (*i).GetSlotStatus( ) == SLOTSTATUS_OPEN )
    {
      (*i).SetSlotStatus( SLOTSTATUS_CLOSED );
      Changed = true;
    }
  }

  if ( Changed )
    SendAllSlotInfo( );
}

void CGame::ShuffleSlots( )
{
  // we only want to shuffle the player slots
  // that means we need to prevent this function from shuffling the open/closed/computer slots too
  // so we start by copying the player slots to a temporary vector

  vector<CGameSlot> PlayerSlots;

  for ( vector<CGameSlot> ::const_iterator i = m_Slots.begin( ); i != m_Slots.end( ); ++i )
  {
    if ( (*i).GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && (*i).GetComputer( ) == 0 && (*i).GetTeam( ) != 12 )
      PlayerSlots.push_back(*i);
  }

  // now we shuffle PlayerSlots

  if ( m_Map->GetMapOptions( ) & MAPOPT_CUSTOMFORCES )
  {
    // rather than rolling our own probably broken shuffle algorithm we use random_shuffle because it's guaranteed to do it properly
    // so in order to let random_shuffle do all the work we need a vector to operate on
    // unfortunately we can't just use PlayerSlots because the team/colour/race shouldn't be modified
    // so make a vector we can use

    vector<unsigned char> SIDs;

    for ( unsigned char i = 0; i < PlayerSlots.size( ); ++i )
      SIDs.push_back( i );

    random_shuffle( SIDs.begin( ), SIDs.end( ) );

    // now put the PlayerSlots vector in the same order as the SIDs vector

    vector<CGameSlot> Slots;

    // as usual don't modify the team/colour/race

    for ( unsigned char i = 0; i < SIDs.size( ); ++i )
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

  vector<CGameSlot> ::const_iterator CurrentPlayer = PlayerSlots.begin( );
  vector<CGameSlot> Slots;

  for ( vector<CGameSlot> ::const_iterator i = m_Slots.begin( ); i != m_Slots.end( ); ++i )
  {
    if ( (*i).GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && (*i).GetComputer( ) == 0 && (*i).GetTeam( ) != 12 )
    {
      Slots.push_back( *CurrentPlayer );
      ++CurrentPlayer;
    }
    else
      Slots.push_back(*i);
  }

  m_Slots = Slots;

  // and finally tell everyone about the new slot configuration

  SendAllSlotInfo( );
}

void CGame::AddToSpoofed( const string &server, const string &name, bool sendMessage )
{
  CGamePlayer *Player = GetPlayerFromName( name, true );

  if ( Player )
  {
    Player->SetSpoofedRealm( server );
    Player->SetSpoofed( true );

    if ( sendMessage )
      SendChat( Player, "Spoof check accepted for [" + name + "] on server [" + server + "]" );
  }
}

void CGame::AddToReserved( string name )
{
  transform( name.begin( ), name.end( ), name.begin( ), (int(* )(int) )tolower );

  // check that the user is not already reserved

  for ( vector<string> ::const_iterator i = m_Reserved.begin( ); i != m_Reserved.end( ); ++i )
  {
    if ( *i == name )
      return;
  }

  m_Reserved.push_back( name );

  // upgrade the user if they're already in the game

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    string NameLower = (*i)->GetName( );
    transform( NameLower.begin( ), NameLower.end( ), NameLower.begin( ), (int(* )(int) )tolower );

    if ( NameLower == name )
      (*i)->SetReserved( true );
  }
}

bool CGame::IsOwner( string name )
{
  string OwnerLower = m_OwnerName;
  transform( name.begin( ), name.end( ), name.begin( ), (int(* )(int) )tolower );
  transform( OwnerLower.begin( ), OwnerLower.end( ), OwnerLower.begin( ), (int(* )(int) )tolower );
  return name == OwnerLower;
}

bool CGame::IsReserved( string name )
{
  transform( name.begin( ), name.end( ), name.begin( ), (int(* )(int) )tolower );

  for ( vector<string> ::const_iterator i = m_Reserved.begin( ); i != m_Reserved.end( ); ++i )
  {
    if ( *i == name )
      return true;
  }

  return false;
}

bool CGame::IsDownloading( )
{
  // returns true if at least one player is downloading the map

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    if ( (*i)->GetDownloadStarted( ) && !(*i)->GetDownloadFinished( ) )
      return true;
  }

  return false;
}

void CGame::StartCountDown( bool force )
{
  if ( !m_CountDownStarted )
  {
    if ( force )
    {
      m_CountDownStarted = true;
      m_CountDownCounter = 5;
    }
    else
    {
      // check if the HCL command string is short enough

      if ( m_HCLCommandString.size( ) > GetSlotsOccupied( ) )
      {
        SendAllChat( "The HCL command string is too long. Use 'force' to start anyway" );
        return;
      }

      // check if everyone has the map

      string StillDownloading;

      for ( vector<CGameSlot> ::const_iterator i = m_Slots.begin( ); i != m_Slots.end( ); ++i )
      {
        if ( (*i).GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && (*i).GetComputer( ) == 0 && (*i).GetDownloadStatus( ) != 100 )
        {
          CGamePlayer *Player = GetPlayerFromPID( (*i).GetPID( ) );

          if ( Player )
          {
            if ( StillDownloading.empty( ) )
              StillDownloading = Player->GetName( );
            else
              StillDownloading += ", " + Player->GetName( );
          }
        }
      }

      if ( !StillDownloading.empty( ) )
        SendAllChat( "Players still downloading the map: " + StillDownloading );

      // check if everyone has been pinged enough (3 times) that the autokicker would have kicked them by now
      // see function EventPlayerPongToHost for the autokicker code

      string NotPinged;

      for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
      {
        if ( !(*i)->GetReserved( ) && (*i)->GetNumPings( ) < 3 )
        {
          if ( NotPinged.empty( ) )
            NotPinged = (*i)->GetName( );
          else
            NotPinged += ", " + (*i)->GetName( );
        }
      }

      if ( !NotPinged.empty( ) )
        SendAllChat( "Players not yet pinged 3 times: " + NotPinged );

      // if no problems found start the game

      if ( StillDownloading.empty( ) && NotPinged.empty( ) )
      {
        m_CountDownStarted = true;
        m_CountDownCounter = 5;
      }
    }
  }
}

void CGame::StopPlayers( const string &reason )
{
  // disconnect every player and set their left reason to the passed string
  // we use this function when we want the code in the Update function to run before the destructor (e.g. saving players to the database)
  // therefore calling this function when m_GameLoading || m_GameLoaded is roughly equivalent to setting m_Exiting = true
  // the only difference is whether the code in the Update function is executed or not

  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    (*i)->SetDeleteMe( true );
    (*i)->SetLeftReason( reason );
    (*i)->SetLeftCode( PLAYERLEAVE_LOST );
  }
}

void CGame::StopLaggers( const string &reason )
{
  for ( vector<CGamePlayer *> ::const_iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
  {
    if ( (*i)->GetLagging( ) )
    {
      (*i)->SetDeleteMe( true );
      (*i)->SetLeftReason( reason );
      (*i)->SetLeftCode( PLAYERLEAVE_DISCONNECT );
    }
  }
}

void CGame::CreateVirtualHost( )
{
  if ( m_VirtualHostPID != 255 )
    return;

  m_VirtualHostPID = GetNewPID( );

  BYTEARRAY IP;
  IP.push_back( 0 );
  IP.push_back( 0 );
  IP.push_back( 0 );
  IP.push_back( 0 );

  SendAll( m_Protocol->SEND_W3GS_PLAYERINFO( m_VirtualHostPID, m_VirtualHostName, IP, IP ) );
}

void CGame::DeleteVirtualHost( )
{
  if ( m_VirtualHostPID == 255 )
    return;

  SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( m_VirtualHostPID, PLAYERLEAVE_LOBBY ) );
  m_VirtualHostPID = 255;
}

void CGame::CreateFakePlayer( )
{
  if ( m_FakePlayers.size( ) > 10 )
    return;

  unsigned char SID = GetEmptySlot( false );

  if ( SID < m_Slots.size( ) )
  {
    if ( GetNumPlayers( ) >= 11 )
      DeleteVirtualHost( );

    unsigned char FakePlayerPID = GetNewPID( );

    BYTEARRAY IP;
    IP.push_back( 0 );
    IP.push_back( 0 );
    IP.push_back( 0 );
    IP.push_back( 0 );

    SendAll( m_Protocol->SEND_W3GS_PLAYERINFO( FakePlayerPID, "Troll[" + UTIL_ToString( FakePlayerPID ) + "]", IP, IP ) );
    m_Slots[SID] = CGameSlot( FakePlayerPID, 100, SLOTSTATUS_OCCUPIED, 0, m_Slots[SID].GetTeam( ), m_Slots[SID].GetColour( ), m_Slots[SID].GetRace( ) );
    m_FakePlayers.push_back( FakePlayerPID );
    SendAllSlotInfo( );
  }
}

void CGame::DeleteFakePlayers( )
{
  if ( m_FakePlayers.empty( ) )
    return;

  for ( unsigned char i = 0; i < m_Slots.size( ); ++i )
  {
    for ( vector<unsigned char> ::const_iterator j = m_FakePlayers.begin( ); j != m_FakePlayers.end( ); ++j )
    {
      if ( m_Slots[i].GetPID( ) == *j )
      {
        m_Slots[i] = CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, m_Slots[i].GetTeam( ), m_Slots[i].GetColour( ), m_Slots[i].GetRace( ) );
        SendAll( m_Protocol->SEND_W3GS_PLAYERLEAVE_OTHERS( *j, PLAYERLEAVE_LOBBY ) );
      }
    }
  }

  m_FakePlayers.clear( );
  SendAllSlotInfo( );
}
