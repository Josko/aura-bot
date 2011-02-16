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

#ifndef GAMEPROTOCOL_H
#define GAMEPROTOCOL_H

//
// CGameProtocol
//

#define W3GS_HEADER_CONSTANT			247

#define GAME_NONE                   0		// this case isn't part of the protocol, it's for internal use only
#define GAME_FULL                   2
#define GAME_PUBLIC                16
#define GAME_PRIVATE               17

#define GAMETYPE_CUSTOM             1
#define GAMETYPE_BLIZZARD           9

#define PLAYERLEAVE_DISCONNECT			1
#define PLAYERLEAVE_LOST            7
#define PLAYERLEAVE_LOSTBUILDINGS		8
#define PLAYERLEAVE_WON             9
#define PLAYERLEAVE_DRAW           10
#define PLAYERLEAVE_OBSERVER       11
#define PLAYERLEAVE_LOBBY          13

#define REJECTJOIN_FULL             9
#define REJECTJOIN_STARTED         10
#define REJECTJOIN_WRONGPASSWORD   27

#include "gameslot.h"

class CGamePlayer;
class CIncomingJoinPlayer;
class CIncomingAction;
class CIncomingChatPlayer;
class CIncomingMapSize;

class CGameProtocol
{
public:
  CAura *m_Aura;

  enum Protocol
  {
    W3GS_PING_FROM_HOST     = 1,  // 0x01
    W3GS_SLOTINFOJOIN       = 4,  // 0x04
    W3GS_REJECTJOIN         = 5,  // 0x05
    W3GS_PLAYERINFO         = 6,  // 0x06
    W3GS_PLAYERLEAVE_OTHERS = 7,  // 0x07
    W3GS_GAMELOADED_OTHERS  = 8,  // 0x08
    W3GS_SLOTINFO           = 9,  // 0x09
    W3GS_COUNTDOWN_START    = 10, // 0x0A
    W3GS_COUNTDOWN_END      = 11, // 0x0B
    W3GS_INCOMING_ACTION    = 12, // 0x0C
    W3GS_CHAT_FROM_HOST     = 15, // 0x0F
    W3GS_START_LAG          = 16, // 0x10
    W3GS_STOP_LAG           = 17, // 0x11
    W3GS_HOST_KICK_PLAYER   = 28, // 0x1C
    W3GS_REQJOIN            = 30, // 0x1E
    W3GS_LEAVEGAME          = 33, // 0x21
    W3GS_GAMELOADED_SELF    = 35, // 0x23
    W3GS_OUTGOING_ACTION    = 38, // 0x26
    W3GS_OUTGOING_KEEPALIVE = 39, // 0x27
    W3GS_CHAT_TO_HOST       = 40, // 0x28
    W3GS_DROPREQ            = 41, // 0x29
    W3GS_SEARCHGAME         = 47, // 0x2F (UDP/LAN)
    W3GS_GAMEINFO           = 48, // 0x30 (UDP/LAN)
    W3GS_CREATEGAME         = 49, // 0x31 (UDP/LAN)
    W3GS_REFRESHGAME        = 50, // 0x32 (UDP/LAN)
    W3GS_DECREATEGAME       = 51, // 0x33 (UDP/LAN)
    W3GS_CHAT_OTHERS        = 52, // 0x34
    W3GS_PING_FROM_OTHERS   = 53, // 0x35
    W3GS_PONG_TO_OTHERS     = 54, // 0x36
    W3GS_MAPCHECK           = 61, // 0x3D
    W3GS_STARTDOWNLOAD      = 63, // 0x3F
    W3GS_MAPSIZE            = 66, // 0x42
    W3GS_MAPPART            = 67, // 0x43
    W3GS_MAPPARTOK          = 68, // 0x44
    W3GS_MAPPARTNOTOK       = 69, // 0x45 - just a guess, received this packet after forgetting to send a crc in W3GS_MAPPART (f7 45 0a 00 01 02 01 00 00 00)
    W3GS_PONG_TO_HOST       = 70, // 0x46
    W3GS_INCOMING_ACTION2   = 72  // 0x48 - received this packet when there are too many actions to fit in W3GS_INCOMING_ACTION
  };

  CGameProtocol( CAura *nAura );
  ~CGameProtocol( );

  // receive functions

  CIncomingJoinPlayer *RECEIVE_W3GS_REQJOIN( BYTEARRAY data );
  uint32_t RECEIVE_W3GS_LEAVEGAME( BYTEARRAY data );
  bool RECEIVE_W3GS_GAMELOADED_SELF( BYTEARRAY data );
  CIncomingAction *RECEIVE_W3GS_OUTGOING_ACTION( BYTEARRAY data, unsigned char PID );
  uint32_t RECEIVE_W3GS_OUTGOING_KEEPALIVE( BYTEARRAY data );
  CIncomingChatPlayer *RECEIVE_W3GS_CHAT_TO_HOST( BYTEARRAY data );
  bool RECEIVE_W3GS_SEARCHGAME( BYTEARRAY data, unsigned char war3Version );
  CIncomingMapSize *RECEIVE_W3GS_MAPSIZE( BYTEARRAY data );
  uint32_t RECEIVE_W3GS_MAPPARTOK( BYTEARRAY data );
  uint32_t RECEIVE_W3GS_PONG_TO_HOST( BYTEARRAY data );

  // send functions

  BYTEARRAY SEND_W3GS_PING_FROM_HOST( );
  BYTEARRAY SEND_W3GS_SLOTINFOJOIN( unsigned char PID, const BYTEARRAY port, BYTEARRAY externalIP, vector<CGameSlot> &slots, uint32_t randomSeed, unsigned char layoutStyle, unsigned char playerSlots );
  BYTEARRAY SEND_W3GS_REJECTJOIN( uint32_t reason );
  BYTEARRAY SEND_W3GS_PLAYERINFO( unsigned char PID, string name, BYTEARRAY externalIP, BYTEARRAY internalIP );
  BYTEARRAY SEND_W3GS_PLAYERLEAVE_OTHERS( unsigned char PID, uint32_t leftCode );
  BYTEARRAY SEND_W3GS_GAMELOADED_OTHERS( unsigned char PID );
  BYTEARRAY SEND_W3GS_SLOTINFO( vector<CGameSlot> &slots, uint32_t randomSeed, unsigned char layoutStyle, unsigned char playerSlots );
  BYTEARRAY SEND_W3GS_COUNTDOWN_START( );
  BYTEARRAY SEND_W3GS_COUNTDOWN_END( );
  BYTEARRAY SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *> actions, uint16_t sendInterval );
  BYTEARRAY SEND_W3GS_INCOMING_ACTION2( queue<CIncomingAction *> actions );
  BYTEARRAY SEND_W3GS_CHAT_FROM_HOST( unsigned char fromPID, BYTEARRAY toPIDs, unsigned char flag, BYTEARRAY flagExtra, string message );
  BYTEARRAY SEND_W3GS_START_LAG( vector<CGamePlayer *> players );
  BYTEARRAY SEND_W3GS_STOP_LAG( CGamePlayer *player );
  BYTEARRAY SEND_W3GS_SEARCHGAME( unsigned char war3Version );
  BYTEARRAY SEND_W3GS_GAMEINFO( unsigned char war3Version, BYTEARRAY mapGameType, BYTEARRAY mapFlags, BYTEARRAY mapWidth, BYTEARRAY mapHeight, string gameName, string hostName, uint32_t upTime, string mapPath, BYTEARRAY mapCRC, uint32_t slotsTotal, uint32_t slotsOpen, uint16_t port, uint32_t hostCounter, uint32_t entryKey );
  BYTEARRAY SEND_W3GS_CREATEGAME( unsigned char war3Version );
  BYTEARRAY SEND_W3GS_REFRESHGAME( uint32_t players, uint32_t playerSlots );
  BYTEARRAY SEND_W3GS_DECREATEGAME( );
  BYTEARRAY SEND_W3GS_MAPCHECK( string mapPath, BYTEARRAY mapSize, BYTEARRAY mapInfo, BYTEARRAY mapCRC, BYTEARRAY mapSHA1 );
  BYTEARRAY SEND_W3GS_STARTDOWNLOAD( unsigned char fromPID );
  BYTEARRAY SEND_W3GS_MAPPART( unsigned char fromPID, unsigned char toPID, uint32_t start, string *mapData );

  // other functions

private:
  bool AssignLength( BYTEARRAY &content );
  bool ValidateLength( BYTEARRAY &content );
  BYTEARRAY EncodeSlotInfo( vector<CGameSlot> &slots, uint32_t randomSeed, unsigned char layoutStyle, unsigned char playerSlots );
};

//
// CIncomingJoinPlayer
//

class CIncomingJoinPlayer
{
private:
  uint32_t m_HostCounter;
  uint32_t m_EntryKey;
  string m_Name;
  BYTEARRAY m_InternalIP;

public:
  CIncomingJoinPlayer( uint32_t nHostCounter, uint32_t nEntryKey, const string &nName, BYTEARRAY &nInternalIP );
  ~CIncomingJoinPlayer( );

  uint32_t GetHostCounter( )
  {
    return m_HostCounter;
  }

  uint32_t GetEntryKey( )
  {
    return m_EntryKey;
  }

  string GetName( )
  {
    return m_Name;
  }

  BYTEARRAY GetInternalIP( )
  {
    return m_InternalIP;
  }
};

//
// CIncomingAction
//

class CIncomingAction
{
private:
  unsigned char m_PID;
  BYTEARRAY m_CRC;
  BYTEARRAY m_Action;

public:
  CIncomingAction( unsigned char nPID, BYTEARRAY &nCRC, BYTEARRAY &nAction );
  ~CIncomingAction( );

  unsigned char GetPID( )
  {
    return m_PID;
  }

  BYTEARRAY GetCRC( )
  {
    return m_CRC;
  }

  BYTEARRAY *GetAction( )
  {
    return &m_Action;
  }

  uint32_t GetLength( )
  {
    return m_Action.size( ) + 3;
  }
};

//
// CIncomingChatPlayer
//

class CIncomingChatPlayer
{
public:

  enum ChatToHostType
  {
    CTH_MESSAGE         = 0,  // a chat message
    CTH_MESSAGEEXTRA    = 1,  // a chat message with extra flags
    CTH_TEAMCHANGE      = 2,  // a team change request
    CTH_COLOURCHANGE    = 3,  // a colour change request
    CTH_RACECHANGE      = 4,  // a race change request
    CTH_HANDICAPCHANGE  = 5   // a handicap change request
  };

private:
  ChatToHostType m_Type;
  unsigned char m_FromPID;
  BYTEARRAY m_ToPIDs;
  unsigned char m_Flag;
  string m_Message;
  unsigned char m_Byte;
  BYTEARRAY m_ExtraFlags;

public:
  CIncomingChatPlayer( unsigned char nFromPID, BYTEARRAY &nToPIDs, unsigned char nFlag, const string &nMessage );
  CIncomingChatPlayer( unsigned char nFromPID, BYTEARRAY &nToPIDs, unsigned char nFlag, const string &nMessage, BYTEARRAY &nExtraFlags );
  CIncomingChatPlayer( unsigned char nFromPID, BYTEARRAY &nToPIDs, unsigned char nFlag, unsigned char nByte );
  ~CIncomingChatPlayer( );

  ChatToHostType GetType( )
  {
    return m_Type;
  }

  unsigned char GetFromPID( )
  {
    return m_FromPID;
  }

  BYTEARRAY GetToPIDs( )
  {
    return m_ToPIDs;
  }

  unsigned char GetFlag( )
  {
    return m_Flag;
  }

  string GetMessage( )
  {
    return m_Message;
  }

  unsigned char GetByte( )
  {
    return m_Byte;
  }

  BYTEARRAY GetExtraFlags( )
  {
    return m_ExtraFlags;
  }
};

class CIncomingMapSize
{
private:
  unsigned char m_SizeFlag;
  uint32_t m_MapSize;

public:
  CIncomingMapSize( unsigned char nSizeFlag, uint32_t nMapSize );
  ~CIncomingMapSize( );

  unsigned char GetSizeFlag( )
  {
    return m_SizeFlag;
  }

  uint32_t GetMapSize( )
  {
    return m_MapSize;
  }
};

#endif
