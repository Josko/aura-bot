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
#include "language.h"
#include "socket.h"
#include "bnet.h"
#include "map.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "game.h"

//
// CPotentialPlayer
//

CPotentialPlayer::CPotentialPlayer( CGameProtocol *nProtocol, CGame *nGame, CTCPSocket *nSocket ) : m_Protocol( nProtocol ), m_Game( nGame ), m_Socket( nSocket ), m_DeleteMe( false ), m_IncomingJoinPlayer( NULL )
{

}

CPotentialPlayer::~CPotentialPlayer( )
{
  if( m_Socket )
    delete m_Socket;
  
  delete m_IncomingJoinPlayer;
}

BYTEARRAY CPotentialPlayer::GetExternalIP( )
{
  if ( m_Socket )
    return m_Socket->GetIP( );

  unsigned char Zeros[] = { 0, 0, 0, 0 };

  return UTIL_CreateByteArray( Zeros, 4 );
}

string CPotentialPlayer::GetExternalIPString( )
{
  if ( m_Socket )
    return m_Socket->GetIPString( );

  return string( );
}

bool CPotentialPlayer::Update( void *fd )
{
  if ( m_DeleteMe )
    return true;

  if( !m_Socket )
    return false;

  m_Socket->DoRecv( (fd_set *) fd );

  // extract as many packets as possible from the socket's receive buffer and process them

  string *RecvBuffer = m_Socket->GetBytes( );
  BYTEARRAY Bytes = UTIL_CreateByteArray( (unsigned char *) RecvBuffer->c_str( ), RecvBuffer->size( ) );

  // a packet is at least 4 bytes so loop as long as the buffer contains 4 bytes

  while ( Bytes.size( ) >= 4 )
  {
    if ( Bytes[0] == W3GS_HEADER_CONSTANT )
    {
      // bytes 2 and 3 contain the length of the packet

      uint16_t Length = UTIL_ByteArrayToUInt16( Bytes, false, 2 );

      if ( Length >= 4 && Bytes.size( ) >= Length )
      {
        if ( Bytes[0] == W3GS_HEADER_CONSTANT && Bytes[1] == CGameProtocol::W3GS_REQJOIN )
        {
          delete m_IncomingJoinPlayer;
          m_IncomingJoinPlayer = m_Protocol->RECEIVE_W3GS_REQJOIN( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) );

          if ( m_IncomingJoinPlayer )
            m_Game->EventPlayerJoined( this, m_IncomingJoinPlayer );

          // this is the packet which interests us for now, the remainder is left for CGamePlayer

          *RecvBuffer = RecvBuffer->substr( Length );
          Bytes = BYTEARRAY( Bytes.begin( ) + Length, Bytes.end( ) );
          break;
        }

        *RecvBuffer = RecvBuffer->substr( Length );
        Bytes = BYTEARRAY( Bytes.begin( ) + Length, Bytes.end( ) );
      }
      else
        break;
    }
  }

  // don't call DoSend here because some other players may not have updated yet and may generate a packet for this player
  // also m_Socket may have been set to NULL during ProcessPackets but we're banking on the fact that m_DeleteMe has been set to true as well so it'll short circuit before dereferencing

  return m_DeleteMe || !m_Socket->GetConnected( ) || m_Socket->HasError( );
}

void CPotentialPlayer::Send( const BYTEARRAY &data )
{
  if( m_Socket )
    m_Socket->PutBytes( data );
}

//
// CGamePlayer
//

CGamePlayer::CGamePlayer( CGameProtocol *nProtocol, CGame *nGame, CTCPSocket *nSocket, unsigned char nPID, const string &nJoinedRealm, const string &nName, const BYTEARRAY &nInternalIP, bool nReserved ) : m_Protocol( nProtocol ), m_Game( nGame ), m_Socket( nSocket ), m_DeleteMe( false ), m_PID( nPID ), m_Name( nName ), m_InternalIP( nInternalIP ), m_JoinedRealm( nJoinedRealm ), m_LeftCode( PLAYERLEAVE_LOBBY ), m_SyncCounter( 0 ), m_JoinTime( GetTime( ) ), m_LastMapPartSent( 0 ), m_LastMapPartAcked( 0 ), m_FinishedLoadingTicks( 0 ), m_StartedLaggingTicks( 0 ), m_Spoofed( false ), m_Reserved( nReserved ), m_WhoisShouldBeSent( false ), m_WhoisSent( false ), m_DownloadAllowed( false ), m_DownloadStarted( false ), m_DownloadFinished( false ), m_FinishedLoading( false ), m_Lagging( false ), m_DropVote( false ), m_KickVote( false ), m_Muted( false ), m_LeftMessageSent( false )
{

}

CGamePlayer::CGamePlayer( CPotentialPlayer *potential, unsigned char nPID, const string &nJoinedRealm, const string &nName, const BYTEARRAY &nInternalIP, bool nReserved ) : m_Protocol( potential->m_Protocol ), m_Game( potential->m_Game ), m_Socket( potential->GetSocket( ) ), m_DeleteMe( false ), m_PID( nPID ), m_Name( nName ), m_InternalIP( nInternalIP ), m_JoinedRealm( nJoinedRealm ), m_LeftCode( PLAYERLEAVE_LOBBY ), m_SyncCounter( 0 ), m_JoinTime( GetTime( ) ), m_LastMapPartSent( 0 ), m_LastMapPartAcked( 0 ), m_FinishedLoadingTicks( 0 ), m_StartedLaggingTicks( 0 ), m_Spoofed( false ), m_Reserved( nReserved ), m_WhoisShouldBeSent( false ), m_WhoisSent( false ), m_DownloadAllowed( false ), m_DownloadStarted( false ), m_DownloadFinished( false ), m_FinishedLoading( false ), m_Lagging( false ), m_DropVote( false ), m_KickVote( false ), m_Muted( false ), m_LeftMessageSent( false )
{

}

CGamePlayer::~CGamePlayer( )
{
    delete m_Socket;
}

BYTEARRAY CGamePlayer::GetExternalIP( )
{
    return m_Socket->GetIP( );
}

string CGamePlayer::GetExternalIPString( )
{
    return m_Socket->GetIPString( );
}

uint32_t CGamePlayer::GetPing( bool LCPing )
{
  // just average all the pings in the vector, nothing fancy

  if ( m_Pings.empty( ) )
    return 0;

  uint32_t AvgPing = 0;

  for ( unsigned int i = 0; i < m_Pings.size( ); ++i )
    AvgPing += m_Pings[i];

  AvgPing /= m_Pings.size( );

  if ( LCPing )
    return AvgPing / 2;
  else
    return AvgPing;
}

bool CGamePlayer::Update( void *fd )
{
  uint32_t Time = GetTime( );

  // wait 4 seconds after joining before sending the /whois or /w
  // if we send the /whois too early battle.net may not have caught up with where the player is and return erroneous results

  if ( m_WhoisShouldBeSent && !m_Spoofed && !m_WhoisSent && !m_JoinedRealm.empty( ) && Time - m_JoinTime >= 4 )
  {
    for ( vector<CBNET *> ::iterator i = m_Game->m_Aura->m_BNETs.begin( ); i != m_Game->m_Aura->m_BNETs.end( ); ++i )
    {
      if ( ( *i )->GetServer( ) == m_JoinedRealm )
      {
        if ( m_Game->GetGameState( ) == GAME_PUBLIC || ( *i )->GetPvPGN( ) )
          ( *i )->QueueChatCommand( "/whois " + m_Name );
        else if ( m_Game->GetGameState( ) == GAME_PRIVATE )
          ( *i )->QueueChatCommand( m_Game->m_Aura->m_Language->SpoofCheckByReplying( ), m_Name, true, string( ) );
      }
    }

    m_WhoisSent = true;
  }

  // check for socket timeouts
  // if we don't receive anything from a player for 35 seconds we can assume they've dropped
  // this works because in the lobby we send pings every 5 seconds and expect a response to each one
  // and in the game the Warcraft 3 client sends keepalives frequently (at least once per second it looks like)

  if ( Time - m_Socket->GetLastRecv( ) >= 35 )
    m_Game->EventPlayerDisconnectTimedOut( this );

  m_Socket->DoRecv( (fd_set *) fd );

  // extract as many packets as possible from the socket's receive buffer and process them

  string *RecvBuffer = m_Socket->GetBytes( );
  BYTEARRAY Bytes = UTIL_CreateByteArray( (unsigned char *) RecvBuffer->c_str( ), RecvBuffer->size( ) );

  // a packet is at least 4 bytes so loop as long as the buffer contains 4 bytes

  CIncomingAction *Action = NULL;
  CIncomingChatPlayer *ChatPlayer = NULL;
  CIncomingMapSize *MapSize = NULL;
  uint32_t Pong = 0;

  while ( Bytes.size( ) >= 4 )
  {
    if ( Bytes[0] == W3GS_HEADER_CONSTANT )
    {
      // bytes 2 and 3 contain the length of the packet

      uint16_t Length = UTIL_ByteArrayToUInt16( Bytes, false, 2 );

      if ( Length >= 4 && Bytes.size( ) >= Length )
      {
        // byte 1 contains the packet ID

        switch ( Bytes[1] )
        {
          case CGameProtocol::W3GS_LEAVEGAME:
            m_Game->EventPlayerLeft( this );
            break;

          case CGameProtocol::W3GS_GAMELOADED_SELF:
            if ( m_Protocol->RECEIVE_W3GS_GAMELOADED_SELF( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) )
            {
              if ( !m_FinishedLoading )
              {
                m_FinishedLoading = true;
                m_FinishedLoadingTicks = GetTicks( );
                m_Game->EventPlayerLoaded( this );
              }
            }

            break;

          case CGameProtocol::W3GS_OUTGOING_ACTION:
            Action = m_Protocol->RECEIVE_W3GS_OUTGOING_ACTION( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ), m_PID );

            if ( Action )
              m_Game->EventPlayerAction( this, Action );

            // don't delete Action here because the game is going to store it in a queue and delete it later

            break;

          case CGameProtocol::W3GS_OUTGOING_KEEPALIVE:
            m_CheckSums.push( m_Protocol->RECEIVE_W3GS_OUTGOING_KEEPALIVE( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) );
            ++m_SyncCounter;
            m_Game->EventPlayerKeepAlive( this );
            break;

          case CGameProtocol::W3GS_CHAT_TO_HOST:
            ChatPlayer = m_Protocol->RECEIVE_W3GS_CHAT_TO_HOST( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) );

            if ( ChatPlayer )
              m_Game->EventPlayerChatToHost( this, ChatPlayer );

            delete ChatPlayer;
            ChatPlayer = NULL;
            break;

          case CGameProtocol::W3GS_DROPREQ:
            if ( !m_DropVote )
            {
              m_DropVote = true;
              m_Game->EventPlayerDropRequest( this );
            }

            break;

          case CGameProtocol::W3GS_MAPSIZE:
            MapSize = m_Protocol->RECEIVE_W3GS_MAPSIZE( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) );

            if ( MapSize )
              m_Game->EventPlayerMapSize( this, MapSize );

            delete MapSize;
            MapSize = NULL;
            break;

          case CGameProtocol::W3GS_PONG_TO_HOST:
            Pong = m_Protocol->RECEIVE_W3GS_PONG_TO_HOST( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) );

            // we discard pong values of 1
            // the client sends one of these when connecting plus we return 1 on error to kill two birds with one stone

            if ( Pong != 1 )
            {
              // we also discard pong values when we're downloading because they're almost certainly inaccurate
              // this statement also gives the player a 5 second grace period after downloading the map to allow queued (i.e. delayed) ping packets to be ignored

              if ( !m_DownloadStarted || ( m_DownloadFinished && GetTime( ) - m_FinishedDownloadingTime >= 5 ) )
              {
                // we also discard pong values when anyone else is downloading if we're configured to

                if ( !m_Game->IsDownloading( ) )
                {
                  m_Pings.push_back( GetTicks( ) - Pong );

                  if ( m_Pings.size( ) > 20 )
                    m_Pings.erase( m_Pings.begin( ) );
                }
              }
            }

            m_Game->EventPlayerPongToHost( this );
            break;
        }

        *RecvBuffer = RecvBuffer->substr( Length );
        Bytes = BYTEARRAY( Bytes.begin( ) + Length, Bytes.end( ) );
      }
      else
        break;
    }
  }

  // try to find out why we're requesting deletion
  // in cases other than the ones covered here m_LeftReason should have been set when m_DeleteMe was set

  if ( m_Socket->HasError( ) )
  {
    m_Game->EventPlayerDisconnectSocketError( this );
    m_Socket->Reset( );
    
    return true;
  }
  else if ( !m_Socket->GetConnected( ) )
  {
    m_Game->EventPlayerDisconnectConnectionClosed( this );
    m_Socket->Reset( );
    
    return true;
  }  
  
  return m_DeleteMe;
}

void CGamePlayer::Send( const BYTEARRAY &data )
{
  m_Socket->PutBytes( data );
}
