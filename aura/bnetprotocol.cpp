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
#include "bnetprotocol.h"

CBNETProtocol::CBNETProtocol( )
{
  unsigned char ClientToken[] = { 220, 1, 203, 7 };
  m_ClientToken = UTIL_CreateByteArray( ClientToken, 4 );
}

CBNETProtocol::~CBNETProtocol( )
{

}

///////////////////////
// RECEIVE FUNCTIONS //
///////////////////////

bool CBNETProtocol::RECEIVE_SID_NULL( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED SID_NULL" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length

  return ValidateLength( data );
}

CIncomingGameHost *CBNETProtocol::RECEIVE_SID_GETADVLISTEX( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED SID_GETADVLISTEX" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> GamesFound
  // if( GamesFound > 0 )
  //		10 bytes			-> ???
  //		2 bytes				-> Port
  //		4 bytes				-> IP
  //		null term string		-> GameName
  //		2 bytes				-> ???
  //		8 bytes				-> HostCounter

  if ( ValidateLength( data ) && data.size( ) >= 8 )
  {
    BYTEARRAY GamesFound = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );

    if ( UTIL_ByteArrayToUInt32( GamesFound, false ) > 0 && data.size( ) >= 25 )
    {
      BYTEARRAY Port = BYTEARRAY( data.begin( ) + 18, data.begin( ) + 20 );
      BYTEARRAY IP = BYTEARRAY( data.begin( ) + 20, data.begin( ) + 24 );
      BYTEARRAY GameName = UTIL_ExtractCString( data, 24 );

      if ( data.size( ) >= GameName.size( ) + 35 )
      {
        BYTEARRAY HostCounter;
        HostCounter.push_back( UTIL_ExtractHex( data, GameName.size( ) + 27, true ) );
        HostCounter.push_back( UTIL_ExtractHex( data, GameName.size( ) + 29, true ) );
        HostCounter.push_back( UTIL_ExtractHex( data, GameName.size( ) + 31, true ) );
        HostCounter.push_back( UTIL_ExtractHex( data, GameName.size( ) + 33, true ) );
        return new CIncomingGameHost( IP,
                UTIL_ByteArrayToUInt16( Port, false ),
                string( GameName.begin( ), GameName.end( ) ),
                HostCounter );
      }
    }
  }

  return NULL;
}

bool CBNETProtocol::RECEIVE_SID_ENTERCHAT( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED SID_ENTERCHAT" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // null terminated string	-> UniqueName

  if ( ValidateLength( data ) && data.size( ) >= 5 )
  {
    m_UniqueName = UTIL_ExtractCString( data, 4 );
    return true;
  }

  return false;
}

CIncomingChatEvent *CBNETProtocol::RECEIVE_SID_CHATEVENT( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED SID_CHATEVENT" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> EventID
  // 4 bytes					-> ???
  // 4 bytes					-> Ping
  // 12 bytes					-> ???
  // null terminated string	-> User
  // null terminated string	-> Message

  if ( ValidateLength( data ) && data.size( ) >= 29 )
  {
    BYTEARRAY EventID = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );
    // BYTEARRAY Ping = BYTEARRAY( data.begin( ) + 12, data.begin( ) + 16 );
    BYTEARRAY User = UTIL_ExtractCString( data, 28 );
    BYTEARRAY Message = UTIL_ExtractCString( data, User.size( ) + 29 );

    return new CIncomingChatEvent( ( CBNETProtocol::IncomingChatEvent )UTIL_ByteArrayToUInt32( EventID, false ),
                string( User.begin( ), User.end( ) ),
                string( Message.begin( ), Message.end( ) ) );
  }

  return NULL;
}

bool CBNETProtocol::RECEIVE_SID_CHECKAD( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED SID_CHECKAD" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length

  return ValidateLength( data );
}

bool CBNETProtocol::RECEIVE_SID_STARTADVEX3( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED SID_STARTADVEX3" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> Status

  if ( ValidateLength( data ) && data.size( ) >= 8 )
  {
    BYTEARRAY Status = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );

    if ( UTIL_ByteArrayToUInt32( Status, false ) == 0 )
      return true;
  }

  return false;
}

BYTEARRAY CBNETProtocol::RECEIVE_SID_PING( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED SID_PING" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> Ping

  if ( ValidateLength( data ) && data.size( ) >= 8 )
    return BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );

  return BYTEARRAY( );
}

bool CBNETProtocol::RECEIVE_SID_LOGONRESPONSE( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED SID_LOGONRESPONSE" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> Status

  if ( ValidateLength( data ) && data.size( ) >= 8 )
  {
    BYTEARRAY Status = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );

    if ( UTIL_ByteArrayToUInt32( Status, false ) == 1 )
      return true;
  }

  return false;
}

bool CBNETProtocol::RECEIVE_SID_AUTH_INFO( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED SID_AUTH_INFO" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> LogonType
  // 4 bytes					-> ServerToken
  // 4 bytes					-> ???
  // 8 bytes					-> MPQFileTime
  // null terminated string	-> IX86VerFileName
  // null terminated string	-> ValueStringFormula

  if ( ValidateLength( data ) && data.size( ) >= 25 )
  {
    m_LogonType = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );
    m_ServerToken = BYTEARRAY( data.begin( ) + 8, data.begin( ) + 12 );
    m_MPQFileTime = BYTEARRAY( data.begin( ) + 16, data.begin( ) + 24 );
    m_IX86VerFileName = UTIL_ExtractCString( data, 24 );
    m_ValueStringFormula = UTIL_ExtractCString( data, m_IX86VerFileName.size( ) + 25 );
    return true;
  }

  return false;
}

bool CBNETProtocol::RECEIVE_SID_AUTH_CHECK( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED SID_AUTH_CHECK" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> KeyState
  // null terminated string	-> KeyStateDescription

  if ( ValidateLength( data ) && data.size( ) >= 9 )
  {
    m_KeyState = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );
    m_KeyStateDescription = UTIL_ExtractCString( data, 8 );

    if ( UTIL_ByteArrayToUInt32( m_KeyState, false ) == KR_GOOD )
      return true;
  }

  return false;
}

bool CBNETProtocol::RECEIVE_SID_AUTH_ACCOUNTLOGON( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED SID_AUTH_ACCOUNTLOGON" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> Status
  // if( Status == 0 )
  //		32 bytes			-> Salt
  //		32 bytes			-> ServerPublicKey

  if ( ValidateLength( data ) && data.size( ) >= 8 )
  {
    BYTEARRAY status = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );

    if ( UTIL_ByteArrayToUInt32( status, false ) == 0 && data.size( ) >= 72 )
    {
      m_Salt = BYTEARRAY( data.begin( ) + 8, data.begin( ) + 40 );
      m_ServerPublicKey = BYTEARRAY( data.begin( ) + 40, data.begin( ) + 72 );
      return true;
    }
  }

  return false;
}

bool CBNETProtocol::RECEIVE_SID_AUTH_ACCOUNTLOGONPROOF( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED SID_AUTH_ACCOUNTLOGONPROOF" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> Status

  if ( ValidateLength( data ) && data.size( ) >= 8 )
  {
    uint32_t Status = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 ), false );

    if ( Status == 0 || Status == 0xE )
      return true;
  }

  return false;
}

vector<string> CBNETProtocol::RECEIVE_SID_FRIENDSLIST( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED SID_FRIENDSLIST" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 1 byte					-> Total
  // for( 1 .. Total )
  //		null term string	-> Account
  //		1 byte				-> Status
  //		1 byte				-> Area
  //		4 bytes				-> ???
  //		null term string	-> Location

  vector<string> Friends;

  if ( ValidateLength( data ) && data.size( ) >= 5 )
  {
    unsigned int i = 5;
    unsigned char Total = data[4];

    while ( Total > 0 )
    {
      --Total;

      if ( data.size( ) < i + 1 )
        break;

      BYTEARRAY Account = UTIL_ExtractCString( data, i );

      Friends.push_back( string( Account.begin( ), Account.end( ) ) );
    }
  }

  return Friends;
}

vector<string> CBNETProtocol::RECEIVE_SID_CLANMEMBERLIST( BYTEARRAY data )
{
  // DEBUG_Print( "RECEIVED SID_CLANMEMBERLIST" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> ???
  // 1 byte					-> Total
  // for( 1 .. Total )
  //		null term string	-> Name
  //		1 byte				-> Rank
  //		1 byte				-> Status
  //		null term string	-> Location

  vector<string> ClanList;

  if ( ValidateLength( data ) && data.size( ) >= 9 )
  {
    unsigned int i = 9;
    unsigned char Total = data[8];

    while ( Total > 0 )
    {
      --Total;

      if ( data.size( ) < i + 1 )
        break;

      BYTEARRAY Name = UTIL_ExtractCString( data, i );
      ClanList.push_back( string( Name.begin( ), Name.end( ) ) );
    }
  }

  return ClanList;
}

////////////////////
// SEND FUNCTIONS //
////////////////////

BYTEARRAY CBNETProtocol::SEND_PROTOCOL_INITIALIZE_SELECTOR( )
{
  BYTEARRAY packet;
  packet.push_back( 1 );
  // DEBUG_Print( "SENT PROTOCOL_INITIALIZE_SELECTOR" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_NULL( )
{
  BYTEARRAY packet;
  packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
  packet.push_back( SID_NULL ); // SID_NULL
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  AssignLength( packet );
  // DEBUG_Print( "SENT SID_NULL" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_STOPADV( )
{
  BYTEARRAY packet;
  packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
  packet.push_back( SID_STOPADV ); // SID_STOPADV
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  AssignLength( packet );
  // DEBUG_Print( "SENT SID_STOPADV" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_GETADVLISTEX( string gameName )
{
  unsigned char MapFilter1[] = { 255, 3, 0, 0 };
  unsigned char MapFilter2[] = { 255, 3, 0, 0 };
  unsigned char MapFilter3[] = { 0, 0, 0, 0 };
  unsigned char NumGames[] = { 1, 0, 0, 0 };

  BYTEARRAY packet;
  packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
  packet.push_back( SID_GETADVLISTEX ); // SID_GETADVLISTEX
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  UTIL_AppendByteArray( packet, MapFilter1, 4 ); // Map Filter
  UTIL_AppendByteArray( packet, MapFilter2, 4 ); // Map Filter
  UTIL_AppendByteArray( packet, MapFilter3, 4 ); // Map Filter
  UTIL_AppendByteArray( packet, NumGames, 4 ); // maximum number of games to list
  UTIL_AppendByteArrayFast( packet, gameName ); // Game Name
  packet.push_back( 0 ); // Game Password is NULL
  packet.push_back( 0 ); // Game Stats is NULL
  AssignLength( packet );
  // DEBUG_Print( "SENT SID_GETADVLISTEX" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_ENTERCHAT( )
{
  BYTEARRAY packet;
  packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
  packet.push_back( SID_ENTERCHAT ); // SID_ENTERCHAT
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // Account Name is NULL on Warcraft III/The Frozen Throne
  packet.push_back( 0 ); // Stat String is NULL on CDKEY'd products
  AssignLength( packet );
  // DEBUG_Print( "SENT SID_ENTERCHAT" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_JOINCHANNEL( string channel )
{
  unsigned char NoCreateJoin[] = { 2, 0, 0, 0 };
  unsigned char FirstJoin[] = { 1, 0, 0, 0 };

  BYTEARRAY packet;
  packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
  packet.push_back( SID_JOINCHANNEL ); // SID_JOINCHANNEL
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later

  if ( channel.size( ) > 0 )
    UTIL_AppendByteArray( packet, NoCreateJoin, 4 ); // flags for no create join
  else
    UTIL_AppendByteArray( packet, FirstJoin, 4 ); // flags for first join

  UTIL_AppendByteArrayFast( packet, channel );
  AssignLength( packet );
  // DEBUG_Print( "SENT SID_JOINCHANNEL" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_CHATCOMMAND( string command )
{
  BYTEARRAY packet;
  packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
  packet.push_back( SID_CHATCOMMAND ); // SID_CHATCOMMAND
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  UTIL_AppendByteArrayFast( packet, command ); // Message
  AssignLength( packet );
  // DEBUG_Print( "SENT SID_CHATCOMMAND" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_CHECKAD( )
{
  unsigned char Zeros[] = { 0, 0, 0, 0 };

  BYTEARRAY packet;
  packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
  packet.push_back( SID_CHECKAD ); // SID_CHECKAD
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  UTIL_AppendByteArray( packet, Zeros, 4 ); // ???
  UTIL_AppendByteArray( packet, Zeros, 4 ); // ???
  UTIL_AppendByteArray( packet, Zeros, 4 ); // ???
  UTIL_AppendByteArray( packet, Zeros, 4 ); // ???
  AssignLength( packet );
  // DEBUG_Print( "SENT SID_CHECKAD" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_STARTADVEX3( unsigned char state, BYTEARRAY mapGameType, BYTEARRAY mapFlags, BYTEARRAY mapWidth, BYTEARRAY mapHeight, string gameName, string hostName, uint32_t upTime, string mapPath, BYTEARRAY mapCRC, BYTEARRAY mapSHA1, uint32_t hostCounter )
{
  // todotodo: sort out how GameType works, the documentation is horrendous

  /*

  Game type tag: (read W3GS_GAMEINFO for this field)
   0x00000001 - Custom
   0x00000009 - Blizzard/Ladder
  Map author: (mask 0x00006000) can be combined
   *0x00002000 - Blizzard
   0x00004000 - Custom
  Battle type: (mask 0x00018000) cant be combined
   0x00000000 - Battle
   *0x00010000 - Scenario
  Map size: (mask 0x000E0000) can be combined with 2 nearest values
   0x00020000 - Small
   0x00040000 - Medium
   *0x00080000 - Huge
  Observers: (mask 0x00700000) cant be combined
   0x00100000 - Allowed observers
   0x00200000 - Observers on defeat
   *0x00400000 - No observers
  Flags:
   0x00000800 - Private game flag (not used in game list)

   */

  unsigned char Unknown[] = { 255, 3, 0, 0 };
  unsigned char CustomGame[] = { 0, 0, 0, 0 };

  string HostCounterString = UTIL_ToHexString( hostCounter );

  if ( HostCounterString.size( ) < 8 )
    HostCounterString.insert( 0, 8 - HostCounterString.size( ), '0' );

  HostCounterString = string( HostCounterString.rbegin( ), HostCounterString.rend( ) );

  BYTEARRAY packet;

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
  UTIL_AppendByteArrayFast( StatString, mapSHA1 );
  StatString = UTIL_EncodeStatString( StatString );

  if ( mapGameType.size( ) == 4 && mapFlags.size( ) == 4 && mapWidth.size( ) == 2 && mapHeight.size( ) == 2 && !gameName.empty( ) && !hostName.empty( ) && !mapPath.empty( ) && mapCRC.size( ) == 4 && mapSHA1.size( ) == 20 && StatString.size( ) < 128 && HostCounterString.size( ) == 8 )
  {
    // make the rest of the packet

    packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
    packet.push_back( SID_STARTADVEX3 ); // SID_STARTADVEX3
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( state ); // State (16 = public, 17 = private, 18 = close)
    packet.push_back( 0 ); // State continued...
    packet.push_back( 0 ); // State continued...
    packet.push_back( 0 ); // State continued...
    UTIL_AppendByteArray( packet, upTime, false ); // time since creation
    UTIL_AppendByteArrayFast( packet, mapGameType ); // Game Type, Parameter
    UTIL_AppendByteArray( packet, Unknown, 4 ); // ???
    UTIL_AppendByteArray( packet, CustomGame, 4 ); // Custom Game
    UTIL_AppendByteArrayFast( packet, gameName ); // Game Name
    packet.push_back( 0 ); // Game Password is NULL
    packet.push_back( 98 ); // Slots Free (ascii 98 = char 'b' = 11 slots free) - note: do not reduce this as this is the # of PID's Warcraft III will allocate
    UTIL_AppendByteArrayFast( packet, HostCounterString, false ); // Host Counter
    UTIL_AppendByteArrayFast( packet, StatString ); // Stat String
    packet.push_back( 0 ); // Stat String null terminator (the stat string is encoded to remove all even numbers i.e. zeros)
    AssignLength( packet );
  }
  else
    Print( "[BNETPROTO] invalid parameters passed to SEND_SID_STARTADVEX3" );

  // DEBUG_Print( "SENT SID_STARTADVEX3" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_NOTIFYJOIN( string gameName )
{
  unsigned char ProductID[] = { 0, 0, 0, 0 };
  unsigned char ProductVersion[] = { 14, 0, 0, 0 }; // Warcraft III is 14

  BYTEARRAY packet;
  packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
  packet.push_back( SID_NOTIFYJOIN ); // SID_NOTIFYJOIN
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  UTIL_AppendByteArray( packet, ProductID, 4 ); // Product ID
  UTIL_AppendByteArray( packet, ProductVersion, 4 ); // Product Version
  UTIL_AppendByteArrayFast( packet, gameName ); // Game Name
  packet.push_back( 0 ); // Game Password is NULL
  AssignLength( packet );
  // DEBUG_Print( "SENT SID_NOTIFYJOIN" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_PING( BYTEARRAY pingValue )
{
  BYTEARRAY packet;

  if ( pingValue.size( ) == 4 )
  {
    packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
    packet.push_back( SID_PING ); // SID_PING
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( 0 ); // packet length will be assigned later
    UTIL_AppendByteArrayFast( packet, pingValue ); // Ping Value
    AssignLength( packet );
  }
  else
    Print( "[BNETPROTO] invalid parameters passed to SEND_SID_PING" );

  // DEBUG_Print( "SENT SID_PING" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_LOGONRESPONSE( BYTEARRAY clientToken, BYTEARRAY serverToken, BYTEARRAY passwordHash, string accountName )
{
  // todotodo: check that the passed BYTEARRAY sizes are correct (don't know what they should be right now so I can't do this today)

  BYTEARRAY packet;
  packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
  packet.push_back( SID_LOGONRESPONSE ); // SID_LOGONRESPONSE
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  UTIL_AppendByteArrayFast( packet, clientToken ); // Client Token
  UTIL_AppendByteArrayFast( packet, serverToken ); // Server Token
  UTIL_AppendByteArrayFast( packet, passwordHash ); // Password Hash
  UTIL_AppendByteArrayFast( packet, accountName ); // Account Name
  AssignLength( packet );
  // DEBUG_Print( "SENT SID_LOGONRESPONSE" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_NETGAMEPORT( uint16_t serverPort )
{
  BYTEARRAY packet;
  packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
  packet.push_back( SID_NETGAMEPORT ); // SID_NETGAMEPORT
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  UTIL_AppendByteArray( packet, serverPort, false ); // local game server port
  AssignLength( packet );
  // DEBUG_Print( "SENT SID_NETGAMEPORT" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_AUTH_INFO( unsigned char ver, uint32_t localeID, string countryAbbrev, string country )
{
  unsigned char ProtocolID[] = { 0, 0, 0, 0 };
  unsigned char PlatformID[] = { 54, 56, 88, 73 }; // "IX86"
  unsigned char ProductID_TFT[] = { 80, 88, 51, 87 }; // "W3XP"
  unsigned char Version[] = { ver, 0, 0, 0 };
  unsigned char Language[] = { 83, 85, 110, 101 }; // "enUS"
  unsigned char LocalIP[] = { 127, 0, 0, 1 };
  unsigned char TimeZoneBias[] = { 44, 1, 0, 0 }; // 300 minutes (GMT -0500)

  BYTEARRAY packet;
  packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
  packet.push_back( SID_AUTH_INFO ); // SID_AUTH_INFO
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  UTIL_AppendByteArray( packet, ProtocolID, 4 ); // Protocol ID
  UTIL_AppendByteArray( packet, PlatformID, 4 ); // Platform ID
  UTIL_AppendByteArray( packet, ProductID_TFT, 4 ); // Product ID (TFT)
  UTIL_AppendByteArray( packet, Version, 4 ); // Version
  UTIL_AppendByteArray( packet, Language, 4 ); // Language (hardcoded as enUS to ensure battle.net sends the bot messages in English)
  UTIL_AppendByteArray( packet, LocalIP, 4 ); // Local IP for NAT compatibility
  UTIL_AppendByteArray( packet, TimeZoneBias, 4 ); // Time Zone Bias
  UTIL_AppendByteArray( packet, localeID, false ); // Locale ID
  UTIL_AppendByteArray( packet, localeID, false ); // Language ID (copying the locale ID should be sufficient since we don't care about sublanguages)
  UTIL_AppendByteArrayFast( packet, countryAbbrev ); // Country Abbreviation
  UTIL_AppendByteArrayFast( packet, country ); // Country
  AssignLength( packet );
  // DEBUG_Print( "SENT SID_AUTH_INFO" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_AUTH_CHECK( BYTEARRAY clientToken, BYTEARRAY exeVersion, BYTEARRAY exeVersionHash, BYTEARRAY keyInfoROC, BYTEARRAY keyInfoTFT, string exeInfo, string keyOwnerName )
{
  BYTEARRAY packet;

  if ( clientToken.size( ) == 4 && exeVersion.size( ) == 4 && exeVersionHash.size( ) == 4 )
  {
    uint32_t NumKeys = 2;
    
    packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
    packet.push_back( SID_AUTH_CHECK ); // SID_AUTH_CHECK
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( 0 ); // packet length will be assigned later
    UTIL_AppendByteArrayFast( packet, clientToken ); // Client Token
    UTIL_AppendByteArrayFast( packet, exeVersion ); // EXE Version
    UTIL_AppendByteArrayFast( packet, exeVersionHash ); // EXE Version Hash
    UTIL_AppendByteArray( packet, NumKeys, false ); // number of keys in this packet
    UTIL_AppendByteArray( packet, (uint32_t) 0, false ); // boolean Using Spawn (32 bit)
    UTIL_AppendByteArrayFast( packet, keyInfoROC ); // ROC Key Info
    UTIL_AppendByteArrayFast( packet, keyInfoTFT ); // TFT Key Info
    UTIL_AppendByteArrayFast( packet, exeInfo ); // EXE Info
    UTIL_AppendByteArrayFast( packet, keyOwnerName ); // CD Key Owner Name
    AssignLength( packet );
  }
  else
    Print( "[BNETPROTO] invalid parameters passed to SEND_SID_AUTH_CHECK" );

  // DEBUG_Print( "SENT SID_AUTH_CHECK" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_AUTH_ACCOUNTLOGON( BYTEARRAY clientPublicKey, string accountName )
{
  BYTEARRAY packet;

  if ( clientPublicKey.size( ) == 32 )
  {
    packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
    packet.push_back( SID_AUTH_ACCOUNTLOGON ); // SID_AUTH_ACCOUNTLOGON
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( 0 ); // packet length will be assigned later
    UTIL_AppendByteArrayFast( packet, clientPublicKey ); // Client Key
    UTIL_AppendByteArrayFast( packet, accountName ); // Account Name
    AssignLength( packet );
  }
  else
    Print( "[BNETPROTO] invalid parameters passed to SEND_SID_AUTH_ACCOUNTLOGON" );

  // DEBUG_Print( "SENT SID_AUTH_ACCOUNTLOGON" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_AUTH_ACCOUNTLOGONPROOF( BYTEARRAY clientPasswordProof )
{
  BYTEARRAY packet;

  if ( clientPasswordProof.size( ) == 20 )
  {
    packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
    packet.push_back( SID_AUTH_ACCOUNTLOGONPROOF ); // SID_AUTH_ACCOUNTLOGONPROOF
    packet.push_back( 0 ); // packet length will be assigned later
    packet.push_back( 0 ); // packet length will be assigned later
    UTIL_AppendByteArrayFast( packet, clientPasswordProof ); // Client Password Proof
    AssignLength( packet );
  }
  else
    Print( "[BNETPROTO] invalid parameters passed to SEND_SID_AUTH_ACCOUNTLOGON" );

  // DEBUG_Print( "SENT SID_AUTH_ACCOUNTLOGONPROOF" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_FRIENDSLIST( )
{
  BYTEARRAY packet;
  packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
  packet.push_back( SID_FRIENDSLIST ); // SID_FRIENDSLIST
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  AssignLength( packet );
  // DEBUG_Print( "SENT SID_FRIENDSLIST" );
  // DEBUG_Print( packet );
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_CLANMEMBERLIST( )
{
  unsigned char Cookie[] = { 0, 0, 0, 0 };

  BYTEARRAY packet;
  packet.push_back( BNET_HEADER_CONSTANT ); // BNET header constant
  packet.push_back( SID_CLANMEMBERLIST ); // SID_CLANMEMBERLIST
  packet.push_back( 0 ); // packet length will be assigned later
  packet.push_back( 0 ); // packet length will be assigned later
  UTIL_AppendByteArray( packet, Cookie, 4 ); // cookie
  AssignLength( packet );
  // DEBUG_Print( "SENT SID_CLANMEMBERLIST" );
  // DEBUG_Print( packet );
  return packet;
}

/////////////////////
// OTHER FUNCTIONS //
/////////////////////

bool CBNETProtocol::AssignLength( BYTEARRAY &content )
{
  // insert the actual length of the content array into bytes 3 and 4 (indices 2 and 3)

  BYTEARRAY LengthBytes;

  if ( content.size( ) >= 4 && content.size( ) <= 65535 )
  {
    LengthBytes = UTIL_CreateByteArray( (uint16_t) content.size( ), false );
    content[2] = LengthBytes[0];
    content[3] = LengthBytes[1];
    return true;
  }

  return false;
}

bool CBNETProtocol::ValidateLength( BYTEARRAY &content )
{
  // verify that bytes 3 and 4 (indices 2 and 3) of the content array describe the length

  uint16_t Length;
  BYTEARRAY LengthBytes;

  if ( content.size( ) >= 4 && content.size( ) <= 65535 )
  {
    LengthBytes.push_back( content[2] );
    LengthBytes.push_back( content[3] );
    Length = UTIL_ByteArrayToUInt16( LengthBytes, false );

    if ( Length == content.size( ) )
      return true;
  }

  return false;
}

//
// CIncomingGameHost
//

CIncomingGameHost::CIncomingGameHost( BYTEARRAY &nIP, uint16_t nPort, string nGameName, BYTEARRAY &nHostCounter ) : m_IP( nIP ), m_Port( nPort ), m_GameName( nGameName ), m_HostCounter( nHostCounter )
{

}

CIncomingGameHost::~CIncomingGameHost( )
{

}

string CIncomingGameHost::GetIPString( )
{
  string Result;

  if ( m_IP.size( ) >= 4 )
  {
    for ( unsigned int i = 0; i < 4; ++i )
    {
      Result += UTIL_ToString( (unsigned int) m_IP[i] );

      if ( i < 3 )
        Result += ".";
    }
  }

  return Result;
}

//
// CIncomingChatEvent
//

CIncomingChatEvent::CIncomingChatEvent( CBNETProtocol::IncomingChatEvent nChatEvent, const string &nUser, const string &nMessage ) : m_ChatEvent( nChatEvent ), m_User( nUser ), m_Message( nMessage )
{

}

CIncomingChatEvent::~CIncomingChatEvent( )
{

}
