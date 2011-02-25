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

#ifndef BNETPROTOCOL_H
#define BNETPROTOCOL_H

//
// CBNETProtocol
//

#define BNET_HEADER_CONSTANT 255

class CIncomingGameHost;
class CIncomingChatEvent;

class CBNETProtocol
{
public:

  enum Protocol
  {
    SID_NULL                    = 0,    // 0x0
    SID_STOPADV                 = 2,    // 0x2
    SID_GETADVLISTEX            = 9,    // 0x9
    SID_ENTERCHAT               = 10,   // 0xA
    SID_JOINCHANNEL             = 12,   // 0xC
    SID_CHATCOMMAND             = 14,   // 0xE
    SID_CHATEVENT               = 15,   // 0xF
    SID_CHECKAD                 = 21,   // 0x15
    SID_STARTADVEX3             = 28,   // 0x1C
    SID_DISPLAYAD               = 33,   // 0x21
    SID_NOTIFYJOIN              = 34,   // 0x22
    SID_PING                    = 37,   // 0x25
    SID_LOGONRESPONSE           = 41,   // 0x29
    SID_NETGAMEPORT             = 69,   // 0x45
    SID_AUTH_INFO               = 80,   // 0x50
    SID_AUTH_CHECK              = 81,   // 0x51
    SID_AUTH_ACCOUNTLOGON       = 83,   // 0x53
    SID_AUTH_ACCOUNTLOGONPROOF  = 84,   // 0x54
    SID_WARDEN                  = 94,   // 0x5E
    SID_FRIENDSLIST             = 101,  // 0x65
    SID_FRIENDSUPDATE           = 102,  // 0x66
    SID_CLANMEMBERLIST          = 125,  // 0x7D
    SID_CLANMEMBERSTATUSCHANGE  = 127   // 0x7F
  };

  enum KeyResult
  {
    KR_GOOD = 0,
    KR_OLD_GAME_VERSION = 256,
    KR_INVALID_VERSION  = 257,
    KR_ROC_KEY_IN_USE   = 513,
    KR_TFT_KEY_IN_USE   = 529
  };

  enum IncomingChatEvent
  {
    EID_SHOWUSER            = 1,  // received when you join a channel (includes users in the channel and their information)
    EID_JOIN                = 2,  // received when someone joins the channel you're currently in
    EID_LEAVE               = 3,  // received when someone leaves the channel you're currently in
    EID_WHISPER             = 4,  // received a whisper message
    EID_TALK                = 5,  // received when someone talks in the channel you're currently in
    EID_BROADCAST           = 6,  // server broadcast
    EID_CHANNEL             = 7,  // received when you join a channel (includes the channel's name, flags)
    EID_USERFLAGS           = 9,  // user flags updates
    EID_WHISPERSENT         = 10, // sent a whisper message
    EID_CHANNELFULL         = 13, // channel is full
    EID_CHANNELDOESNOTEXIST = 14, // channel does not exist
    EID_CHANNELRESTRICTED   = 15, // channel is restricted
    EID_INFO                = 18, // broadcast/information message
    EID_ERROR               = 19, // error message
    EID_EMOTE               = 23, // emote
    EID_IRC                 = -1  // internal flag only
  };

private:
  BYTEARRAY m_ClientToken;          // set in constructor
  BYTEARRAY m_LogonType;            // set in RECEIVE_SID_AUTH_INFO
  BYTEARRAY m_ServerToken;          // set in RECEIVE_SID_AUTH_INFO
  BYTEARRAY m_MPQFileTime;          // set in RECEIVE_SID_AUTH_INFO
  BYTEARRAY m_IX86VerFileName;      // set in RECEIVE_SID_AUTH_INFO
  BYTEARRAY m_ValueStringFormula;   // set in RECEIVE_SID_AUTH_INFO
  BYTEARRAY m_KeyState;             // set in RECEIVE_SID_AUTH_CHECK
  BYTEARRAY m_KeyStateDescription;  // set in RECEIVE_SID_AUTH_CHECK
  BYTEARRAY m_Salt;                 // set in RECEIVE_SID_AUTH_ACCOUNTLOGON
  BYTEARRAY m_ServerPublicKey;      // set in RECEIVE_SID_AUTH_ACCOUNTLOGON
  BYTEARRAY m_UniqueName;           // set in RECEIVE_SID_ENTERCHAT

public:
  CBNETProtocol( );
  ~CBNETProtocol( );

  BYTEARRAY GetClientToken( )             { return m_ClientToken; }
  BYTEARRAY GetLogonType( )               { return m_LogonType; }
  BYTEARRAY GetServerToken( )             { return m_ServerToken; }
  BYTEARRAY GetMPQFileTime( )             { return m_MPQFileTime; }
  BYTEARRAY GetIX86VerFileName( )         { return m_IX86VerFileName; }
  string GetIX86VerFileNameString( )      { return string( m_IX86VerFileName.begin( ), m_IX86VerFileName.end( ) ); }
  BYTEARRAY GetValueStringFormula( )      { return m_ValueStringFormula; }
  string GetValueStringFormulaString( )   { return string( m_ValueStringFormula.begin( ), m_ValueStringFormula.end( ) ); }
  BYTEARRAY GetKeyState( )                { return m_KeyState; }
  string GetKeyStateDescription( )        { return string( m_KeyStateDescription.begin( ), m_KeyStateDescription.end( ) ); }
  BYTEARRAY GetSalt( )                    { return m_Salt; }
  BYTEARRAY GetServerPublicKey( )         { return m_ServerPublicKey; }
  BYTEARRAY GetUniqueName( )              { return m_UniqueName; }

  // receive functions

  bool RECEIVE_SID_NULL( BYTEARRAY data );
  CIncomingGameHost *RECEIVE_SID_GETADVLISTEX( BYTEARRAY data );
  bool RECEIVE_SID_ENTERCHAT( BYTEARRAY data );
  CIncomingChatEvent *RECEIVE_SID_CHATEVENT( BYTEARRAY data );
  bool RECEIVE_SID_CHECKAD( BYTEARRAY data );
  bool RECEIVE_SID_STARTADVEX3( BYTEARRAY data );
  BYTEARRAY RECEIVE_SID_PING( BYTEARRAY data );
  bool RECEIVE_SID_LOGONRESPONSE( BYTEARRAY data );
  bool RECEIVE_SID_AUTH_INFO( BYTEARRAY data );
  bool RECEIVE_SID_AUTH_CHECK( BYTEARRAY data );
  bool RECEIVE_SID_AUTH_ACCOUNTLOGON( BYTEARRAY data );
  bool RECEIVE_SID_AUTH_ACCOUNTLOGONPROOF( BYTEARRAY data );
  vector<string> RECEIVE_SID_FRIENDSLIST( BYTEARRAY data );
  vector<string> RECEIVE_SID_CLANMEMBERLIST( BYTEARRAY data );

  // send functions

  BYTEARRAY SEND_PROTOCOL_INITIALIZE_SELECTOR( );
  BYTEARRAY SEND_SID_NULL( );
  BYTEARRAY SEND_SID_STOPADV( );
  BYTEARRAY SEND_SID_GETADVLISTEX( string gameName );
  BYTEARRAY SEND_SID_ENTERCHAT( );
  BYTEARRAY SEND_SID_JOINCHANNEL( string channel );
  BYTEARRAY SEND_SID_CHATCOMMAND( string command );
  BYTEARRAY SEND_SID_CHECKAD( );
  BYTEARRAY SEND_SID_STARTADVEX3( unsigned char state, BYTEARRAY mapGameType, BYTEARRAY mapFlags, BYTEARRAY mapWidth, BYTEARRAY mapHeight, string gameName, string hostName, uint32_t upTime, string mapPath, BYTEARRAY mapCRC, BYTEARRAY mapSHA1, uint32_t hostCounter );
  BYTEARRAY SEND_SID_NOTIFYJOIN( string gameName );
  BYTEARRAY SEND_SID_PING( BYTEARRAY pingValue );
  BYTEARRAY SEND_SID_LOGONRESPONSE( BYTEARRAY clientToken, BYTEARRAY serverToken, BYTEARRAY passwordHash, string accountName );
  BYTEARRAY SEND_SID_NETGAMEPORT( uint16_t serverPort );
  BYTEARRAY SEND_SID_AUTH_INFO( unsigned char ver, uint32_t localeID, string countryAbbrev, string country );
  BYTEARRAY SEND_SID_AUTH_CHECK( BYTEARRAY clientToken, BYTEARRAY exeVersion, BYTEARRAY exeVersionHash, BYTEARRAY keyInfoROC, BYTEARRAY keyInfoTFT, string exeInfo, string keyOwnerName );
  BYTEARRAY SEND_SID_AUTH_ACCOUNTLOGON( BYTEARRAY clientPublicKey, string accountName );
  BYTEARRAY SEND_SID_AUTH_ACCOUNTLOGONPROOF( BYTEARRAY clientPasswordProof );
  BYTEARRAY SEND_SID_FRIENDSLIST( );
  BYTEARRAY SEND_SID_CLANMEMBERLIST( );

  // other functions

private:
  void AssignLength( BYTEARRAY &content );
  bool ValidateLength( BYTEARRAY &content );
};

//
// CIncomingGameHost
//

class CIncomingGameHost
{
private:
  BYTEARRAY m_IP;
  uint16_t m_Port;
  string m_GameName;
  BYTEARRAY m_HostCounter;

public:
  CIncomingGameHost( BYTEARRAY &nIP, uint16_t nPort, string nGameName, BYTEARRAY &nHostCounter );
  ~CIncomingGameHost( );

  string GetIPString( );
  BYTEARRAY GetIP( )          { return m_IP; }
  uint16_t GetPort( )         { return m_Port; }
  string GetGameName( )       { return m_GameName; }
  BYTEARRAY GetHostCounter( ) { return m_HostCounter; }
};

//
// CIncomingChatEvent
//

class CIncomingChatEvent
{
private:
  CBNETProtocol::IncomingChatEvent m_ChatEvent;
  string m_User;
  string m_Message;

public:
  CIncomingChatEvent( CBNETProtocol::IncomingChatEvent nChatEvent, const string &nUser, const string &nMessage );
  ~CIncomingChatEvent( );

  CBNETProtocol::IncomingChatEvent GetChatEvent( )    { return m_ChatEvent; }
  string GetUser( )                                   { return m_User; }
  string GetMessage( )                                { return m_Message; }
};

#endif
