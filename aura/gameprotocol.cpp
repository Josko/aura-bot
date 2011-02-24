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
#include "crc32.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "game.h"

//
// CGameProtocol
//

CGameProtocol::CGameProtocol( CAura *nAura ) : m_Aura( nAura )
{

}

CGameProtocol::~CGameProtocol( )
{

}

///////////////////////
// RECEIVE FUNCTIONS //
///////////////////////

CIncomingJoinPlayer *CGameProtocol::RECEIVE_W3GS_REQJOIN( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED W3GS_REQJOIN" );
  // DEBUG_Print( data );

  // 2 bytes                    -> Header
  // 2 bytes                    -> Length
  // 4 bytes                    -> Host Counter (Game ID)
  // 4 bytes                    -> Entry Key (used in LAN)
  // 1 byte                     -> ???
  // 2 bytes                    -> Listen Port
  // 4 bytes                    -> Peer Key
  // null terminated string			-> Name
  // 4 bytes                    -> ???
  // 2 bytes                    -> InternalPort (???)
  // 4 bytes                    -> InternalIP

  if ( ValidateLength( data ) && data.size( ) >= 20 )
  {
    uint32_t HostCounter = UTIL_ByteArrayToUInt32( data, false, 4 );
    uint32_t EntryKey = UTIL_ByteArrayToUInt32( data, false, 8 );
    BYTEARRAY Name = UTIL_ExtractCString( data, 19 );

    if ( !Name.empty( ) && data.size( ) >= Name.size( ) + 30 )
    {
      BYTEARRAY InternalIP = BYTEARRAY( data.begin( ) + Name.size( ) + 26, data.begin( ) + Name.size( ) + 30 );
      return new CIncomingJoinPlayer( HostCounter, EntryKey, string( Name.begin( ), Name.end( ) ), InternalIP );
    }
  }

  return NULL;
}

uint32_t CGameProtocol::RECEIVE_W3GS_LEAVEGAME( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED W3GS_LEAVEGAME" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> Reason

  if ( ValidateLength( data ) && data.size( ) >= 8 )
    return UTIL_ByteArrayToUInt32( data, false, 4 );

  return 0;
}

bool CGameProtocol::RECEIVE_W3GS_GAMELOADED_SELF( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED W3GS_GAMELOADED_SELF" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length

  if ( ValidateLength( data ) )
    return true;

  return false;
}

CIncomingAction *CGameProtocol::RECEIVE_W3GS_OUTGOING_ACTION( BYTEARRAY data, unsigned char PID )
{
  // DEBUG_Print( "RECEIVED W3GS_OUTGOING_ACTION" );
  // DEBUG_Print( data );

  // 2 bytes                -> Header
  // 2 bytes                -> Length
  // 4 bytes                -> CRC
  // remainder of packet		-> Action

  if ( PID != 255 && ValidateLength( data ) && data.size( ) >= 8 )
  {
    BYTEARRAY CRC = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );
    BYTEARRAY Action = BYTEARRAY( data.begin( ) + 8, data.end( ) );
    return new CIncomingAction( PID, CRC, Action );
  }

  return NULL;
}

uint32_t CGameProtocol::RECEIVE_W3GS_OUTGOING_KEEPALIVE( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED W3GS_OUTGOING_KEEPALIVE" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 1 byte           -> ???
  // 4 bytes					-> CheckSum??? (used in replays)

  if ( ValidateLength( data ) && data.size( ) == 9 )
    return UTIL_ByteArrayToUInt32( data, false, 5 );

  return 0;
}

CIncomingChatPlayer *CGameProtocol::RECEIVE_W3GS_CHAT_TO_HOST( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED W3GS_CHAT_TO_HOST" );
  // DEBUG_Print( data );

  // 2 bytes              -> Header
  // 2 bytes              -> Length
  // 1 byte               -> Total
  // for( 1 .. Total )
  //		1 byte            -> ToPID
  // 1 byte               -> FromPID
  // 1 byte               -> Flag
  // if( Flag == 16 )
  //		null term string	-> Message
  // elseif( Flag == 17 )
  //		1 byte            -> Team
  // elseif( Flag == 18 )
  //		1 byte            -> Colour
  // elseif( Flag == 19 )
  //		1 byte            -> Race
  // elseif( Flag == 20 )
  //		1 byte            -> Handicap
  // elseif( Flag == 32 )
  //		4 bytes           -> ExtraFlags
  //		null term string	-> Message

  if ( ValidateLength( data ) )
  {
    unsigned int i = 5;
    unsigned char Total = data[4];

    if ( Total > 0 && data.size( ) >= i + Total )
    {
      BYTEARRAY ToPIDs = BYTEARRAY( data.begin( ) + i, data.begin( ) + i + Total );
      i += Total;
      unsigned char FromPID = data[i];
      unsigned char Flag = data[i + 1];
      i += 2;

      if ( Flag == 16 && data.size( ) >= i + 1 )
      {
        // chat message

        BYTEARRAY Message = UTIL_ExtractCString( data, i );
        return new CIncomingChatPlayer( FromPID, ToPIDs, Flag, string( Message.begin( ), Message.end( ) ) );
      }
      else if ( ( Flag >= 17 && Flag <= 20 ) && data.size( ) >= i + 1 )
      {
        // team/colour/race/handicap change request

        unsigned char Byte = data[i];
        return new CIncomingChatPlayer( FromPID, ToPIDs, Flag, Byte );
      }
      else if ( Flag == 32 && data.size( ) >= i + 5 )
      {
        // chat message with extra flags

        BYTEARRAY ExtraFlags = BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 );
        BYTEARRAY Message = UTIL_ExtractCString( data, i + 4 );
        return new CIncomingChatPlayer( FromPID, ToPIDs, Flag, string( Message.begin( ), Message.end( ) ), ExtraFlags );
      }
    }
  }

  return NULL;
}

bool CGameProtocol::RECEIVE_W3GS_SEARCHGAME( BYTEARRAY data, unsigned char war3Version )
{
  uint32_t ProductID = 1462982736; // "W3XP"
  uint32_t Version = war3Version;

  // DEBUG_Print( "RECEIVED W3GS_SEARCHGAME" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> ProductID
  // 4 bytes					-> Version
  // 4 bytes					-> ??? (Zero)

  if ( ValidateLength( data ) && data.size( ) >= 16 )
  {
    if ( UTIL_ByteArrayToUInt32( data, false, 4 ) == ProductID )
    {
      if ( UTIL_ByteArrayToUInt32( data, false, 8 ) == Version )
      {
        if ( UTIL_ByteArrayToUInt32( data, false, 12 ) == 0 )
          return true;
      }
    }
  }

  return false;
}

CIncomingMapSize *CGameProtocol::RECEIVE_W3GS_MAPSIZE( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED W3GS_MAPSIZE" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> ???
  // 1 byte           -> SizeFlag (1 = have map, 3 = continue download)
  // 4 bytes					-> MapSize

  if ( ValidateLength( data ) && data.size( ) >= 13 )
    return new CIncomingMapSize( data[8], UTIL_ByteArrayToUInt32( data, false, 9 ) );

  return NULL;
}

uint32_t CGameProtocol::RECEIVE_W3GS_MAPPARTOK( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED W3GS_MAPPARTOK" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 1 byte           -> SenderPID
  // 1 byte           -> ReceiverPID
  // 4 bytes					-> ???
  // 4 bytes					-> MapSize

  if ( ValidateLength( data ) && data.size( ) >= 14 )
    return UTIL_ByteArrayToUInt32( data, false, 10 );

  return 0;
}

uint32_t CGameProtocol::RECEIVE_W3GS_PONG_TO_HOST( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED W3GS_PONG_TO_HOST" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> Pong

  // the pong value is just a copy of whatever was sent in SEND_W3GS_PING_FROM_HOST which was GetTicks( ) at the time of sending
  // so as long as we trust that the client isn't trying to fake us out and mess with the pong value we can find the round trip time by simple subtraction
  // (the subtraction is done elsewhere because the very first pong value seems to be 1 and we want to discard that one)

  if ( ValidateLength( data ) && data.size( ) >= 8 )
    return UTIL_ByteArrayToUInt32( data, false, 4 );

  return 1;
}

////////////////////
// SEND FUNCTIONS //
////////////////////

BYTEARRAY CGameProtocol::SEND_W3GS_PING_FROM_HOST( )
{
  BYTEARRAY packet;
  packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
  packet.push_back( W3GS_PING_FROM_HOST ); // W3GS_PING_FROM_HOST
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  UTIL_AppendByteArray( packet, GetTicks( ), false ); // ping value
  AssignLength( packet );

  // DEBUG_Print( "SENT W3GS_PING_FROM_HOST" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_SLOTINFOJOIN( unsigned char PID, BYTEARRAY port, BYTEARRAY externalIP, vector<CGameSlot> &slots, uint32_t randomSeed, unsigned char layoutStyle, unsigned char playerSlots )
{
  unsigned char Zeros[] = { 0, 0, 0, 0 };

  BYTEARRAY SlotInfo = EncodeSlotInfo( slots, randomSeed, layoutStyle, playerSlots );
  BYTEARRAY packet;

  if ( port.size( ) == 2 && externalIP.size( ) == 4 )
  {
    packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
    packet.push_back( W3GS_SLOTINFOJOIN ); // W3GS_SLOTINFOJOIN
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( 0 ); // packet length will be assigned later
    UTIL_AppendByteArray( packet, (uint16_t) SlotInfo.size( ), false ); // SlotInfo length
    UTIL_AppendByteArrayFast( packet, SlotInfo ); // SlotInfo
    packet.push_back( PID ); // PID
    packet.push_back( 2 ); // AF_INET
    packet.push_back( 0 ); // AF_INET continued...
    UTIL_AppendByteArray( packet, port ); // port
    UTIL_AppendByteArrayFast( packet, externalIP ); // external IP
    UTIL_AppendByteArray( packet, Zeros, 4 ); // ???
    UTIL_AppendByteArray( packet, Zeros, 4 ); // ???
    AssignLength( packet );
  }
  else
    Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_SLOTINFOJOIN" );

  // DEBUG_Print( "SENT W3GS_SLOTINFOJOIN" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_REJECTJOIN( uint32_t reason )
{
  BYTEARRAY packet;
  packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
  packet.push_back( W3GS_REJECTJOIN ); // W3GS_REJECTJOIN
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  UTIL_AppendByteArray( packet, reason, false ); // reason
  AssignLength( packet );

  // DEBUG_Print( "SENT W3GS_REJECTJOIN" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_PLAYERINFO( unsigned char PID, string name, BYTEARRAY externalIP, BYTEARRAY internalIP )
{
  unsigned char PlayerJoinCounter[] = { 2, 0, 0, 0 };
  unsigned char Zeros[] = { 0, 0, 0, 0 };

  BYTEARRAY packet;

  if ( !name.empty( ) && name.size( ) <= 15 && externalIP.size( ) == 4 && internalIP.size( ) == 4 )
  {
    packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
    packet.push_back( W3GS_PLAYERINFO ); // W3GS_PLAYERINFO
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( 0 ); // packet length will be assigned later
    UTIL_AppendByteArray( packet, PlayerJoinCounter, 4 ); // player join counter
    packet.push_back( PID ); // PID
    UTIL_AppendByteArrayFast( packet, name ); // player name
    packet.push_back( 1 ); // ???
    packet.push_back( 0 ); // ???
    packet.push_back( 2 ); // AF_INET
    packet.push_back( 0 ); // AF_INET continued...
    packet.push_back( 0 ); // port
    packet.push_back( 0 ); // port continued...
    UTIL_AppendByteArrayFast( packet, externalIP ); // external IP
    UTIL_AppendByteArray( packet, Zeros, 4 ); // ???
    UTIL_AppendByteArray( packet, Zeros, 4 ); // ???
    packet.push_back( 2 ); // AF_INET
    packet.push_back( 0 ); // AF_INET continued...
    packet.push_back( 0 ); // port
    packet.push_back( 0 ); // port continued...
    UTIL_AppendByteArrayFast( packet, internalIP ); // internal IP
    UTIL_AppendByteArray( packet, Zeros, 4 ); // ???
    UTIL_AppendByteArray( packet, Zeros, 4 ); // ???
    AssignLength( packet );
  }
  else
    Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_PLAYERINFO" );

  // DEBUG_Print( "SENT W3GS_PLAYERINFO" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_PLAYERLEAVE_OTHERS( unsigned char PID, uint32_t leftCode )
{
  BYTEARRAY packet;

  if ( PID != 255 )
  {
    packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
    packet.push_back( W3GS_PLAYERLEAVE_OTHERS ); // W3GS_PLAYERLEAVE_OTHERS
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( PID ); // PID
    UTIL_AppendByteArray( packet, leftCode, false ); // left code (see PLAYERLEAVE_ constants in gameprotocol.h)
    AssignLength( packet );
  }
  else
    Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_PLAYERLEAVE_OTHERS" );

  // DEBUG_Print( "SENT W3GS_PLAYERLEAVE_OTHERS" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_GAMELOADED_OTHERS( unsigned char PID )
{
  BYTEARRAY packet;

  if ( PID != 255 )
  {
    packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
    packet.push_back( W3GS_GAMELOADED_OTHERS ); // W3GS_GAMELOADED_OTHERS
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( PID ); // PID
    AssignLength( packet );
  }
  else
    Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_GAMELOADED_OTHERS" );

  // DEBUG_Print( "SENT W3GS_GAMELOADED_OTHERS" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_SLOTINFO( vector<CGameSlot> &slots, uint32_t randomSeed, unsigned char layoutStyle, unsigned char playerSlots )
{
  BYTEARRAY SlotInfo = EncodeSlotInfo( slots, randomSeed, layoutStyle, playerSlots );
  BYTEARRAY packet;
  packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
  packet.push_back( W3GS_SLOTINFO ); // W3GS_SLOTINFO
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  UTIL_AppendByteArray( packet, (uint16_t) SlotInfo.size( ), false ); // SlotInfo length
  UTIL_AppendByteArrayFast( packet, SlotInfo ); // SlotInfo
  AssignLength( packet );
  // DEBUG_Print( "SENT W3GS_SLOTINFO" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_COUNTDOWN_START( )
{
  BYTEARRAY packet;
  packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
  packet.push_back( W3GS_COUNTDOWN_START ); // W3GS_COUNTDOWN_START
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  AssignLength( packet );
  // DEBUG_Print( "SENT W3GS_COUNTDOWN_START" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_COUNTDOWN_END( )
{
  BYTEARRAY packet;
  packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
  packet.push_back( W3GS_COUNTDOWN_END ); // W3GS_COUNTDOWN_END
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  AssignLength( packet );
  // DEBUG_Print( "SENT W3GS_COUNTDOWN_END" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *> actions, uint16_t sendInterval )
{
  BYTEARRAY packet;
  packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
  packet.push_back( W3GS_INCOMING_ACTION ); // W3GS_INCOMING_ACTION
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  UTIL_AppendByteArray( packet, sendInterval, false ); // send interval

  // create subpacket

  if ( !actions.empty( ) )
  {
    BYTEARRAY subpacket;

    while ( !actions.empty( ) )
    {
      CIncomingAction *Action = actions.front( );
      actions.pop( );
      subpacket.push_back( Action->GetPID( ) );
      UTIL_AppendByteArray( subpacket, (uint16_t) Action->GetAction( )->size( ), false );
      UTIL_AppendByteArrayFast( subpacket, *Action->GetAction( ) );
    }

    // calculate crc (we only care about the first 2 bytes though)

    BYTEARRAY crc32 = UTIL_CreateByteArray( m_Aura->m_CRC->FullCRC( (unsigned char *) string( subpacket.begin( ), subpacket.end( ) ).c_str( ), subpacket.size( ) ), false );
    crc32.resize( 2 );

    // finish subpacket

    UTIL_AppendByteArrayFast( packet, crc32 ); // crc
    UTIL_AppendByteArrayFast( packet, subpacket ); // subpacket
  }

  AssignLength( packet );
  // DEBUG_Print( "SENT W3GS_INCOMING_ACTION" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_CHAT_FROM_HOST( unsigned char fromPID, BYTEARRAY toPIDs, unsigned char flag, BYTEARRAY flagExtra, string message )
{
  BYTEARRAY packet;

  if ( !toPIDs.empty( ) && !message.empty( ) && message.size( ) < 255 )
  {
    packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
    packet.push_back( W3GS_CHAT_FROM_HOST ); // W3GS_CHAT_FROM_HOST
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( toPIDs.size( ) ); // number of receivers
    UTIL_AppendByteArrayFast( packet, toPIDs ); // receivers
    packet.push_back( fromPID ); // sender
    packet.push_back( flag ); // flag
    UTIL_AppendByteArrayFast( packet, flagExtra ); // extra flag
    UTIL_AppendByteArrayFast( packet, message ); // message
    AssignLength( packet );
  }
  else
    Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_CHAT_FROM_HOST" );

  // DEBUG_Print( "SENT W3GS_CHAT_FROM_HOST" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_START_LAG( vector<CGamePlayer *> players )
{
  BYTEARRAY packet;

  unsigned char NumLaggers = 0;

  for ( vector<CGamePlayer *> ::iterator i = players.begin( ); i != players.end( ); ++i )
  {
    if ( ( *i )->GetLagging( ) )
      ++NumLaggers;
  }

  if ( NumLaggers > 0 )
  {
    packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
    packet.push_back( W3GS_START_LAG ); // W3GS_START_LAG
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( NumLaggers );

    for ( vector<CGamePlayer *> ::iterator i = players.begin( ); i != players.end( ); ++i )
    {
      if ( ( *i )->GetLagging( ) )
      {
        packet.push_back( ( *i )->GetPID( ) );
        UTIL_AppendByteArray( packet, GetTicks( ) - ( *i )->GetStartedLaggingTicks( ), false );
      }
    }

    AssignLength( packet );
  }
  else
    Print( "[GAMEPROTO] no laggers passed to SEND_W3GS_START_LAG" );

  // DEBUG_Print( "SENT W3GS_START_LAG" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_STOP_LAG( CGamePlayer *player )
{
  BYTEARRAY packet;
  packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
  packet.push_back( W3GS_STOP_LAG ); // W3GS_STOP_LAG
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( player->GetPID( ) );

  UTIL_AppendByteArray( packet, GetTicks( ) - player->GetStartedLaggingTicks( ), false );

  AssignLength( packet );
  // DEBUG_Print( "SENT W3GS_STOP_LAG" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_GAMEINFO( unsigned char war3Version, BYTEARRAY mapGameType, BYTEARRAY mapFlags, BYTEARRAY mapWidth, BYTEARRAY mapHeight, string gameName, string hostName, uint32_t upTime, string mapPath, BYTEARRAY mapCRC, uint32_t slotsTotal, uint32_t slotsOpen, uint16_t port, uint32_t hostCounter, uint32_t entryKey )
{
  unsigned char ProductID_TFT[] = { 80, 88, 51, 87 }; // "W3XP"
  unsigned char Version[] = { war3Version, 0, 0, 0 };
  unsigned char Unknown2[] = { 1, 0, 0, 0 };

  BYTEARRAY packet;

  if ( mapGameType.size( ) == 4 && mapFlags.size( ) == 4 && mapWidth.size( ) == 2 && mapHeight.size( ) == 2 && !gameName.empty( ) && !hostName.empty( ) && !mapPath.empty( ) && mapCRC.size( ) == 4 )
  {
    // make the stat string

    BYTEARRAY StatString;
    UTIL_AppendByteArrayFast( StatString, mapFlags );
    StatString.push_back( 0 );
    UTIL_AppendByteArrayFast( StatString, mapWidth );
    UTIL_AppendByteArrayFast( StatString, mapHeight );
    UTIL_AppendByteArrayFast( StatString, mapCRC );
    UTIL_AppendByteArrayFast( StatString, mapPath );
    UTIL_AppendByteArrayFast( StatString, hostName );
    StatString.push_back( 0 );
    StatString = UTIL_EncodeStatString( StatString );

    // make the rest of the packet

    packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
    packet.push_back( W3GS_GAMEINFO ); // W3GS_GAMEINFO
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( 0 ); // packet length will be assigned later

    UTIL_AppendByteArray( packet, ProductID_TFT, 4 ); // Product ID (TFT)
    UTIL_AppendByteArray( packet, Version, 4 ); // Version
    UTIL_AppendByteArray( packet, hostCounter, false ); // Host Counter
    UTIL_AppendByteArray( packet, entryKey, false ); // Entry Key
    UTIL_AppendByteArrayFast( packet, gameName ); // Game Name
    packet.push_back( 0 ); // ??? (maybe game password)
    UTIL_AppendByteArrayFast( packet, StatString ); // Stat String
    packet.push_back( 0 ); // Stat String null terminator (the stat string is encoded to remove all even numbers i.e. zeros)
    UTIL_AppendByteArray( packet, slotsTotal, false ); // Slots Total
    UTIL_AppendByteArrayFast( packet, mapGameType ); // Game Type
    UTIL_AppendByteArray( packet, Unknown2, 4 ); // ???
    UTIL_AppendByteArray( packet, slotsOpen, false ); // Slots Open
    UTIL_AppendByteArray( packet, upTime, false ); // time since creation
    UTIL_AppendByteArray( packet, port, false ); // port
    AssignLength( packet );
  }
  else
    Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_GAMEINFO" );

  // DEBUG_Print( "SENT W3GS_GAMEINFO" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_CREATEGAME( unsigned char war3Version )
{
  unsigned char ProductID_TFT[] = { 80, 88, 51, 87 }; // "W3XP"
  unsigned char Version[] = { war3Version, 0, 0, 0 };
  unsigned char HostCounter[] = { 1, 0, 0, 0 };

  BYTEARRAY packet;
  packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
  packet.push_back( W3GS_CREATEGAME ); // W3GS_CREATEGAME
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later

  UTIL_AppendByteArray( packet, ProductID_TFT, 4 ); // Product ID (TFT)
  UTIL_AppendByteArray( packet, Version, 4 ); // Version
  UTIL_AppendByteArray( packet, HostCounter, 4 ); // Host Counter
  AssignLength( packet );
  // DEBUG_Print( "SENT W3GS_CREATEGAME" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_REFRESHGAME( uint32_t players, uint32_t playerSlots )
{
  unsigned char HostCounter[] = { 1, 0, 0, 0 };

  BYTEARRAY packet;
  packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
  packet.push_back( W3GS_REFRESHGAME ); // W3GS_REFRESHGAME
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  UTIL_AppendByteArray( packet, HostCounter, 4 ); // Host Counter
  UTIL_AppendByteArray( packet, players, false ); // Players
  UTIL_AppendByteArray( packet, playerSlots, false ); // Player Slots
  AssignLength( packet );
  // DEBUG_Print( "SENT W3GS_REFRESHGAME" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_DECREATEGAME( )
{
  unsigned char HostCounter[] = { 1, 0, 0, 0 };

  BYTEARRAY packet;
  packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
  packet.push_back( W3GS_DECREATEGAME ); // W3GS_DECREATEGAME
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  UTIL_AppendByteArray( packet, HostCounter, 4 ); // Host Counter
  AssignLength( packet );
  // DEBUG_Print( "SENT W3GS_DECREATEGAME" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_MAPCHECK( string mapPath, BYTEARRAY mapSize, BYTEARRAY mapInfo, BYTEARRAY mapCRC, BYTEARRAY mapSHA1 )
{
  unsigned char Unknown[] = { 1, 0, 0, 0 };

  BYTEARRAY packet;

  if ( !mapPath.empty( ) && mapSize.size( ) == 4 && mapInfo.size( ) == 4 && mapCRC.size( ) == 4 && mapSHA1.size( ) == 20 )
  {
    packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
    packet.push_back( W3GS_MAPCHECK ); // W3GS_MAPCHECK
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( 0 ); // packet length will be assigned later
    UTIL_AppendByteArray( packet, Unknown, 4 ); // ???
    UTIL_AppendByteArrayFast( packet, mapPath ); // map path
    UTIL_AppendByteArrayFast( packet, mapSize ); // map size
    UTIL_AppendByteArrayFast( packet, mapInfo ); // map info
    UTIL_AppendByteArrayFast( packet, mapCRC ); // map crc
    UTIL_AppendByteArrayFast( packet, mapSHA1 ); // map sha1
    AssignLength( packet );
  }
  else
    Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_MAPCHECK" );

  // DEBUG_Print( "SENT W3GS_MAPCHECK" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_STARTDOWNLOAD( unsigned char fromPID )
{
  unsigned char Unknown[] = { 1, 0, 0, 0 };

  BYTEARRAY packet;
  packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
  packet.push_back( W3GS_STARTDOWNLOAD ); // W3GS_STARTDOWNLOAD
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  UTIL_AppendByteArray( packet, Unknown, 4 ); // ???
  packet.push_back( fromPID ); // from PID
  AssignLength( packet );
  // DEBUG_Print( "SENT W3GS_STARTDOWNLOAD" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_MAPPART( unsigned char fromPID, unsigned char toPID, uint32_t start, string *mapData )
{
  unsigned char Unknown[] = { 1, 0, 0, 0 };

  BYTEARRAY packet;

  if ( start < mapData->size( ) )
  {
    packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
    packet.push_back( W3GS_MAPPART ); // W3GS_MAPPART
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( toPID ); // to PID
    packet.push_back( fromPID ); // from PID
    UTIL_AppendByteArray( packet, Unknown, 4 ); // ???
    UTIL_AppendByteArray( packet, start, false ); // start position

    // calculate end position (don't send more than 1442 map bytes in one packet)

    uint32_t End = start + 1442;

    if ( End > mapData->size( ) )
      End = mapData->size( );

    // calculate crc

    BYTEARRAY crc32 = UTIL_CreateByteArray( m_Aura->m_CRC->FullCRC( (unsigned char *) mapData->c_str( ) + start, End - start ), false );
    UTIL_AppendByteArrayFast( packet, crc32 );

    // map data

    BYTEARRAY Data = UTIL_CreateByteArray( (unsigned char *) mapData->c_str( ) + start, End - start );
    UTIL_AppendByteArrayFast( packet, Data );
    AssignLength( packet );
  }
  else
    Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_MAPPART" );

  // DEBUG_Print( "SENT W3GS_MAPPART" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CGameProtocol::SEND_W3GS_INCOMING_ACTION2( queue<CIncomingAction *> actions )
{
  BYTEARRAY packet;
  packet.push_back( W3GS_HEADER_CONSTANT ); // W3GS header constant
  packet.push_back( W3GS_INCOMING_ACTION2 ); // W3GS_INCOMING_ACTION2
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // ??? (send interval?)
  packet.push_back( 0 ); // ??? (send interval?)

  // create subpacket

  if ( !actions.empty( ) )
  {
    BYTEARRAY subpacket;

    while ( !actions.empty( ) )
    {
      CIncomingAction *Action = actions.front( );
      actions.pop( );
      subpacket.push_back( Action->GetPID( ) );
      UTIL_AppendByteArray( subpacket, (uint16_t) Action->GetAction( )->size( ), false );
      UTIL_AppendByteArrayFast( subpacket, *Action->GetAction( ) );
    }

    // calculate crc (we only care about the first 2 bytes though)

    BYTEARRAY crc32 = UTIL_CreateByteArray( m_Aura->m_CRC->FullCRC( (unsigned char *) string( subpacket.begin( ), subpacket.end( ) ).c_str( ), subpacket.size( ) ), false );
    crc32.resize( 2 );

    // finish subpacket

    UTIL_AppendByteArrayFast( packet, crc32 ); // crc
    UTIL_AppendByteArrayFast( packet, subpacket ); // subpacket
  }

  AssignLength( packet );
  // DEBUG_Print( "SENT W3GS_INCOMING_ACTION2" );
  // DEBUG_Print( packet );
  return packet;
}

/////////////////////
// OTHER FUNCTIONS //
/////////////////////

void CGameProtocol::AssignLength( BYTEARRAY &content )
{
  // insert the actual length of the content array into bytes 3 and 4 (indices 2 and 3)

  BYTEARRAY LengthBytes;

  LengthBytes = UTIL_CreateByteArray( (uint16_t) content.size( ), false );
  content[2] = LengthBytes[0];
  content[3] = LengthBytes[1];
}

bool CGameProtocol::ValidateLength( BYTEARRAY &content )
{
  // verify that bytes 3 and 4 (indices 2 and 3) of the content array describe the length

  BYTEARRAY LengthBytes;

  LengthBytes.push_back( content[2] );
  LengthBytes.push_back( content[3] );

  if ( UTIL_ByteArrayToUInt16( LengthBytes, false ) == content.size( ) )
    return true;

  return false;
}

BYTEARRAY CGameProtocol::EncodeSlotInfo( vector<CGameSlot> &slots, uint32_t randomSeed, unsigned char layoutStyle, unsigned char playerSlots )
{
  BYTEARRAY SlotInfo;
  SlotInfo.push_back( (unsigned char) slots.size( ) ); // number of slots

  for ( unsigned int i = 0; i < slots.size( ); ++i )
    UTIL_AppendByteArray( SlotInfo, slots[i].GetByteArray( ) );

  UTIL_AppendByteArray( SlotInfo, randomSeed, false ); // random seed
  SlotInfo.push_back( layoutStyle ); // LayoutStyle (0 = melee, 1 = custom forces, 3 = custom forces + fixed player settings)
  SlotInfo.push_back( playerSlots ); // number of player slots (non observer)
  return SlotInfo;
}

//
// CIncomingJoinPlayer
//

CIncomingJoinPlayer::CIncomingJoinPlayer( uint32_t nHostCounter, uint32_t nEntryKey, const string &nName, BYTEARRAY &nInternalIP ) : m_HostCounter( nHostCounter ), m_EntryKey( nEntryKey ), m_Name( nName ), m_InternalIP( nInternalIP )
{

}

CIncomingJoinPlayer::~CIncomingJoinPlayer( )
{

}

//
// CIncomingAction
//

CIncomingAction::CIncomingAction( unsigned char nPID, BYTEARRAY &nCRC, BYTEARRAY &nAction ) : m_PID( nPID ), m_CRC( nCRC ), m_Action( nAction )
{

}

CIncomingAction::~CIncomingAction( )
{

}

//
// CIncomingChatPlayer
//

CIncomingChatPlayer::CIncomingChatPlayer( unsigned char nFromPID, BYTEARRAY &nToPIDs, unsigned char nFlag, const string &nMessage ) : m_Type( CTH_MESSAGE ), m_FromPID( nFromPID ), m_ToPIDs( nToPIDs ), m_Flag( nFlag ), m_Message( nMessage )
{

}

CIncomingChatPlayer::CIncomingChatPlayer( unsigned char nFromPID, BYTEARRAY &nToPIDs, unsigned char nFlag, const string &nMessage, BYTEARRAY &nExtraFlags ) : m_Type( CTH_MESSAGE ), m_FromPID( nFromPID ), m_ToPIDs( nToPIDs ), m_Flag( nFlag ), m_Message( nMessage ), m_ExtraFlags( nExtraFlags )
{

}

CIncomingChatPlayer::CIncomingChatPlayer( unsigned char nFromPID, BYTEARRAY &nToPIDs, unsigned char nFlag, unsigned char nByte ) : m_FromPID( nFromPID ), m_ToPIDs( nToPIDs ), m_Flag( nFlag ), m_Byte( nByte )
{
  if ( nFlag == 17 )
    m_Type = CTH_TEAMCHANGE;
  else if ( nFlag == 18 )
    m_Type = CTH_COLOURCHANGE;
  else if ( nFlag == 19 )
    m_Type = CTH_RACECHANGE;
  else if ( nFlag == 20 )
    m_Type = CTH_HANDICAPCHANGE;
}

CIncomingChatPlayer::~CIncomingChatPlayer( )
{

}

//
// CIncomingMapSize
//

CIncomingMapSize::CIncomingMapSize( unsigned char nSizeFlag, uint32_t nMapSize ) : m_SizeFlag( nSizeFlag ), m_MapSize( nMapSize )
{

}

CIncomingMapSize::~CIncomingMapSize( )
{

}
