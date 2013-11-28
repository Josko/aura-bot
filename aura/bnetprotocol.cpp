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

#include "bnetprotocol.h"
#include "util.h"

using namespace std;

CBNETProtocol::CBNETProtocol()
  : m_ClientToken(BYTEARRAY {220, 1, 203, 7})
{

}

CBNETProtocol::~CBNETProtocol()
{

}

///////////////////////
// RECEIVE FUNCTIONS //
///////////////////////

bool CBNETProtocol::RECEIVE_SID_NULL(const BYTEARRAY &data)
{
  // DEBUG_Print( "RECEIVED SID_NULL" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length

  return ValidateLength(data);
}

CIncomingGameHost *CBNETProtocol::RECEIVE_SID_GETADVLISTEX(const BYTEARRAY &data)
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

  if (ValidateLength(data) && data.size() >= 8)
  {
    const BYTEARRAY GamesFound = BYTEARRAY(data.begin() + 4, data.begin() + 8);

    if (ByteArrayToUInt32(GamesFound, false) > 0 && data.size() >= 25)
    {
      const BYTEARRAY Port = BYTEARRAY(data.begin() + 18, data.begin() + 20);
      const BYTEARRAY IP = BYTEARRAY(data.begin() + 20, data.begin() + 24);
      const BYTEARRAY GameName = ExtractCString(data, 24);

      if (data.size() >= GameName.size() + 35)
      {
        const BYTEARRAY HostCounter =
        {
          ExtractHex(data, GameName.size() + 27, true),
          ExtractHex(data, GameName.size() + 29, true),
          ExtractHex(data, GameName.size() + 31, true),
          ExtractHex(data, GameName.size() + 33, true)
        };

        return new CIncomingGameHost(IP,
                                     ByteArrayToUInt16(Port, false),
                                     string(GameName.begin(), GameName.end()),
                                     HostCounter);
      }
    }
  }

  return nullptr;
}

bool CBNETProtocol::RECEIVE_SID_ENTERCHAT(const BYTEARRAY &data)
{
  // DEBUG_Print( "RECEIVED SID_ENTERCHAT" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // null terminated string	-> UniqueName

  if (ValidateLength(data) && data.size() >= 5)
  {
    m_UniqueName = ExtractCString(data, 4);
    return true;
  }

  return false;
}

CIncomingChatEvent *CBNETProtocol::RECEIVE_SID_CHATEVENT(const BYTEARRAY &data)
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

  if (ValidateLength(data) && data.size() >= 29)
  {
    const BYTEARRAY EventID = BYTEARRAY(data.begin() + 4, data.begin() + 8);
    // BYTEARRAY Ping = BYTEARRAY( data.begin( ) + 12, data.begin( ) + 16 );
    const BYTEARRAY User = ExtractCString(data, 28);
    const BYTEARRAY Message = ExtractCString(data, User.size() + 29);

    return new CIncomingChatEvent((CBNETProtocol::IncomingChatEvent)ByteArrayToUInt32(EventID, false),
                                  string(User.begin(), User.end()),
                                  string(Message.begin(), Message.end()));
  }

  return nullptr;
}

bool CBNETProtocol::RECEIVE_SID_CHECKAD(const BYTEARRAY &data)
{
  // DEBUG_Print( "RECEIVED SID_CHECKAD" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length

  return ValidateLength(data);
}

bool CBNETProtocol::RECEIVE_SID_STARTADVEX3(const BYTEARRAY &data)
{
  // DEBUG_Print( "RECEIVED SID_STARTADVEX3" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> Status

  if (ValidateLength(data) && data.size() >= 8)
  {
    const BYTEARRAY Status = BYTEARRAY(data.begin() + 4, data.begin() + 8);

    if (ByteArrayToUInt32(Status, false) == 0)
      return true;
  }

  return false;
}

BYTEARRAY CBNETProtocol::RECEIVE_SID_PING(const BYTEARRAY &data)
{
  // DEBUG_Print( "RECEIVED SID_PING" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> Ping

  if (ValidateLength(data) && data.size() >= 8)
    return BYTEARRAY(data.begin() + 4, data.begin() + 8);

  return BYTEARRAY();
}

bool CBNETProtocol::RECEIVE_SID_AUTH_INFO(const BYTEARRAY &data)
{
  // DEBUG_Print( "RECEIVED SID_AUTH_INFO" );
  // DEBUG_Print( data );

  // 2 bytes				    -> Header
  // 2 bytes				    -> Length
  // 4 bytes				    -> LogonType
  // 4 bytes				    -> ServerToken
  // 4 bytes				    -> ???
  // 8 bytes				    -> MPQFileTime
  // null terminated string	    -> IX86VerFileName
  // null terminated string	    -> ValueStringFormula

  if (ValidateLength(data) && data.size() >= 25)
  {
    m_LogonType = BYTEARRAY(data.begin() + 4, data.begin() + 8);
    m_ServerToken = BYTEARRAY(data.begin() + 8, data.begin() + 12);
    m_MPQFileTime = BYTEARRAY(data.begin() + 16, data.begin() + 24);
    m_IX86VerFileName = ExtractCString(data, 24);
    m_ValueStringFormula = ExtractCString(data, m_IX86VerFileName.size() + 25);
    return true;
  }

  return false;
}

bool CBNETProtocol::RECEIVE_SID_AUTH_CHECK(const BYTEARRAY &data)
{
  // DEBUG_Print( "RECEIVED SID_AUTH_CHECK" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> KeyState
  // null terminated string	    -> KeyStateDescription

  if (ValidateLength(data) && data.size() >= 9)
  {
    m_KeyState = BYTEARRAY(data.begin() + 4, data.begin() + 8);
    m_KeyStateDescription = ExtractCString(data, 8);

    if (ByteArrayToUInt32(m_KeyState, false) == KR_GOOD)
      return true;
  }

  return false;
}

bool CBNETProtocol::RECEIVE_SID_AUTH_ACCOUNTLOGON(const BYTEARRAY &data)
{
  // DEBUG_Print( "RECEIVED SID_AUTH_ACCOUNTLOGON" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> Status
  // if( Status == 0 )
  //		32 bytes			-> Salt
  //		32 bytes			-> ServerPublicKey

  if (ValidateLength(data) && data.size() >= 8)
  {
    const BYTEARRAY status = BYTEARRAY(data.begin() + 4, data.begin() + 8);

    if (ByteArrayToUInt32(status, false) == 0 && data.size() >= 72)
    {
      m_Salt = BYTEARRAY(data.begin() + 8, data.begin() + 40);
      m_ServerPublicKey = BYTEARRAY(data.begin() + 40, data.begin() + 72);
      return true;
    }
  }

  return false;
}

bool CBNETProtocol::RECEIVE_SID_AUTH_ACCOUNTLOGONPROOF(const BYTEARRAY &data)
{
  // DEBUG_Print( "RECEIVED SID_AUTH_ACCOUNTLOGONPROOF" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> Status

  if (ValidateLength(data) && data.size() >= 8)
  {
    uint32_t Status = ByteArrayToUInt32(BYTEARRAY(data.begin() + 4, data.begin() + 8), false);

    if (Status == 0 || Status == 0xE)
      return true;
  }

  return false;
}

vector<string> CBNETProtocol::RECEIVE_SID_FRIENDLIST(const BYTEARRAY &data)
{
  // DEBUG_Print( "RECEIVED SID_FRIENDSLIST" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 1 byte					    -> Total
  // for( 1 .. Total )
  //		null term string	-> Account
  //		1 byte				-> Status
  //		1 byte				-> Area
  //		4 bytes				-> ???
  //		null term string	-> Location

  vector<string> Friends;

  if (ValidateLength(data) && data.size() >= 5)
  {
    uint32_t i = 5;
    uint8_t Total = data[4];

    while (Total > 0)
    {
      --Total;

      if (data.size() < i + 1)
        break;

      const BYTEARRAY Account = ExtractCString(data, i);
      i += Account.size() + 1;

      if (data.size() < i + 7)
        break;

      i += 6;
      i += ExtractCString(data, i).size() + 1;

      Friends.push_back(string(Account.begin(), Account.end()));
    }
  }

  return Friends;
}

vector<string> CBNETProtocol::RECEIVE_SID_CLANMEMBERLIST(const BYTEARRAY &data)
{
  // DEBUG_Print( "RECEIVED SID_CLANMEMBERLIST" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> ???
  // 1 byte					    -> Total
  // for( 1 .. Total )
  //		null term string	-> Name
  //		1 byte				-> Rank
  //		1 byte				-> Status
  //		null term string	-> Location

  vector<string> ClanList;

  if (ValidateLength(data) && data.size() >= 9)
  {
    uint32_t i = 9;
    uint8_t Total = data[8];

    while (Total > 0)
    {
      --Total;

      if (data.size() < i + 1)
        break;

      const BYTEARRAY Name = ExtractCString(data, i);
      i += Name.size() + 1;

      if (data.size() < i + 3)
        break;

      i += 2;

      // in the original VB source the location string is read but discarded, so that's what I do here

      i += ExtractCString(data, i).size() + 1;
      ClanList.push_back(string(Name.begin(), Name.end()));
    }
  }

  return ClanList;
}

////////////////////
// SEND FUNCTIONS //
////////////////////

BYTEARRAY CBNETProtocol::SEND_PROTOCOL_INITIALIZE_SELECTOR()
{
  return BYTEARRAY {1};
}

BYTEARRAY CBNETProtocol::SEND_SID_NULL()
{
  return BYTEARRAY {BNET_HEADER_CONSTANT, SID_NULL, 4, 0};
}

BYTEARRAY CBNETProtocol::SEND_SID_STOPADV()
{
  return BYTEARRAY {BNET_HEADER_CONSTANT, SID_STOPADV, 4, 0};
}

BYTEARRAY CBNETProtocol::SEND_SID_GETADVLISTEX(const string &gameName)
{
  BYTEARRAY packet = {BNET_HEADER_CONSTANT, SID_GETADVLISTEX, 0, 0, 255, 3, 0, 0, 255, 3, 0, 0, 0, 0, 0, 1, 0, 0, 0};
  AppendByteArrayFast(packet, gameName);  // Game Name
  packet.push_back(0);                    // Game Password is NULL
  packet.push_back(0);                    // Game Stats is NULL
  AssignLength(packet);
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_ENTERCHAT()
{
  return BYTEARRAY {BNET_HEADER_CONSTANT, SID_ENTERCHAT, 6, 0, 0, 0};
}

BYTEARRAY CBNETProtocol::SEND_SID_JOINCHANNEL(const string &channel)
{
  BYTEARRAY packet = {BNET_HEADER_CONSTANT, SID_JOINCHANNEL, 0, 0};

  if (channel.size() > 0)
  {
    const uint8_t NoCreateJoin[] = { 2, 0, 0, 0 };
    AppendByteArray(packet, NoCreateJoin, 4);   // flags for no create join
  }
  else
  {
    const uint8_t FirstJoin[] = { 1, 0, 0, 0 };
    AppendByteArray(packet, FirstJoin, 4);   // flags for first join
  }

  AppendByteArrayFast(packet, channel);
  AssignLength(packet);

  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_CHATCOMMAND(const string &command)
{
  BYTEARRAY packet = {BNET_HEADER_CONSTANT, SID_CHATCOMMAND, 0, 0};
  AppendByteArrayFast(packet, command);   // Message
  AssignLength(packet);
  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_CHECKAD()
{
  return BYTEARRAY {BNET_HEADER_CONSTANT, SID_CHECKAD, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
}

BYTEARRAY CBNETProtocol::SEND_SID_STARTADVEX3(uint8_t state, const BYTEARRAY &mapGameType, const BYTEARRAY &mapFlags, const BYTEARRAY &mapWidth, const BYTEARRAY &mapHeight, const string &gameName, const string &hostName, uint32_t upTime, const string &mapPath, const BYTEARRAY &mapCRC, const BYTEARRAY &mapSHA1, uint32_t hostCounter)
{
  // TODO: sort out how GameType works, the documentation is horrendous

  string HostCounterString = ToHexString(hostCounter);

  if (HostCounterString.size() < 8)
    HostCounterString.insert((size_t) 0, (size_t) 8 - HostCounterString.size(), '0');

  HostCounterString = string(HostCounterString.rbegin(), HostCounterString.rend());

  BYTEARRAY packet;

  // make the stat string

  BYTEARRAY StatString;
  AppendByteArrayFast(StatString, mapFlags);
  StatString.push_back(0);
  AppendByteArrayFast(StatString, mapWidth);
  AppendByteArrayFast(StatString, mapHeight);
  AppendByteArrayFast(StatString, mapCRC);
  AppendByteArrayFast(StatString, mapPath);
  AppendByteArrayFast(StatString, hostName);
  StatString.push_back(0);
  AppendByteArrayFast(StatString, mapSHA1);
  StatString = EncodeStatString(StatString);

  if (mapGameType.size() == 4 && mapFlags.size() == 4 && mapWidth.size() == 2 && mapHeight.size() == 2 && !gameName.empty() && !hostName.empty() && !mapPath.empty() && mapCRC.size() == 4 && mapSHA1.size() == 20 && StatString.size() < 128 && HostCounterString.size() == 8)
  {
    // make the rest of the packet

    const uint8_t Unknown[] = { 255, 3, 0, 0 };
    const uint8_t CustomGame[] = { 0, 0, 0, 0 };

    packet.push_back(BNET_HEADER_CONSTANT);   // BNET header constant
    packet.push_back(SID_STARTADVEX3);   // SID_STARTADVEX3
    packet.push_back(0);   // packet length will be assigned later
    packet.push_back(0);   // packet length will be assigned later
    packet.push_back(state);   // State (16 = public, 17 = private, 18 = close)
    packet.push_back(0);   // State continued...
    packet.push_back(0);   // State continued...
    packet.push_back(0);   // State continued...
    AppendByteArray(packet, upTime, false);   // time since creation
    AppendByteArrayFast(packet, mapGameType);   // Game Type, Parameter
    AppendByteArray(packet, Unknown, 4);   // ???
    AppendByteArray(packet, CustomGame, 4);   // Custom Game
    AppendByteArrayFast(packet, gameName);   // Game Name
    packet.push_back(0);   // Game Password is NULL
    packet.push_back(98);   // Slots Free (ascii 98 = char 'b' = 11 slots free) - note: do not reduce this as this is the # of PID's Warcraft III will allocate
    AppendByteArrayFast(packet, HostCounterString, false);   // Host Counter
    AppendByteArrayFast(packet, StatString);   // Stat String
    packet.push_back(0);   // Stat String null terminator (the stat string is encoded to remove all even numbers i.e. zeros)
    AssignLength(packet);
  }
  else
    Print("[BNETPROTO] invalid parameters passed to SEND_SID_STARTADVEX3");

  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_NOTIFYJOIN(const string &gameName)
{
  BYTEARRAY packet = {BNET_HEADER_CONSTANT, SID_NOTIFYJOIN, 0, 0, 0, 0, 0, 0, 14, 0, 0, 0};
  AppendByteArrayFast(packet, gameName);  // Game Name
  packet.push_back(0);                    // Game Password is NULL
  AssignLength(packet);

  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_PING(const BYTEARRAY &pingValue)
{
  if (pingValue.size() == 4)
  {
    BYTEARRAY packet = {BNET_HEADER_CONSTANT, SID_PING, 0, 0};
    AppendByteArrayFast(packet, pingValue);   // Ping Value
    AssignLength(packet);
    return packet;
  }

  Print("[BNETPROTO] invalid parameters passed to SEND_SID_PING");
  return BYTEARRAY();
}

BYTEARRAY CBNETProtocol::SEND_SID_LOGONRESPONSE(BYTEARRAY clientToken, BYTEARRAY serverToken, BYTEARRAY passwordHash, string accountName)
{
  // TODO: check that the passed BYTEARRAY sizes are correct (don't know what they should be right now so I can't do this today)

  BYTEARRAY packet;
  packet.push_back(BNET_HEADER_CONSTANT);   // BNET header constant
  packet.push_back(SID_LOGONRESPONSE);   // SID_LOGONRESPONSE
  packet.push_back(0);   // packet length will be assigned later
  packet.push_back(0);   // packet length will be assigned later
  AppendByteArrayFast(packet, clientToken);   // Client Token
  AppendByteArrayFast(packet, serverToken);   // Server Token
  AppendByteArrayFast(packet, passwordHash);   // Password Hash
  AppendByteArrayFast(packet, accountName);   // Account Name
  AssignLength(packet);

  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_NETGAMEPORT(uint16_t serverPort)
{
  BYTEARRAY packet;
  packet.push_back(BNET_HEADER_CONSTANT);   // BNET header constant
  packet.push_back(SID_NETGAMEPORT);   // SID_NETGAMEPORT
  packet.push_back(0);   // packet length will be assigned later
  packet.push_back(0);   // packet length will be assigned later
  AppendByteArray(packet, serverPort, false);   // local game server port
  AssignLength(packet);

  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_AUTH_INFO(uint8_t ver, uint32_t localeID, string countryAbbrev, string country)
{
  const uint8_t ProtocolID[] = { 0, 0, 0, 0 };
  const uint8_t PlatformID[] = { 54, 56, 88, 73 }; // "IX86"
  const uint8_t ProductID_TFT[] = { 80, 88, 51, 87 }; // "W3XP"
  const uint8_t Version[] = { ver, 0, 0, 0 };
  const uint8_t Language[] = { 83, 85, 110, 101 }; // "enUS"
  const uint8_t LocalIP[] = { 127, 0, 0, 1 };
  const uint8_t TimeZoneBias[] = { 60, 0, 0, 0 }; // 60 minutes (GMT +0100) but this is probably -0100

  BYTEARRAY packet;
  packet.push_back(BNET_HEADER_CONSTANT);   // BNET header constant
  packet.push_back(SID_AUTH_INFO);   // SID_AUTH_INFO
  packet.push_back(0);   // packet length will be assigned later
  packet.push_back(0);   // packet length will be assigned later
  AppendByteArray(packet, ProtocolID, 4);   // Protocol ID
  AppendByteArray(packet, PlatformID, 4);   // Platform ID
  AppendByteArray(packet, ProductID_TFT, 4);   // Product ID (TFT)
  AppendByteArray(packet, Version, 4);   // Version
  AppendByteArray(packet, Language, 4);   // Language (hardcoded as enUS to ensure battle.net sends the bot messages in English)
  AppendByteArray(packet, LocalIP, 4);   // Local IP for NAT compatibility
  AppendByteArray(packet, TimeZoneBias, 4);   // Time Zone Bias
  AppendByteArray(packet, localeID, false);   // Locale ID
  AppendByteArray(packet, localeID, false);   // Language ID (copying the locale ID should be sufficient since we don't care about sublanguages)
  AppendByteArrayFast(packet, countryAbbrev);   // Country Abbreviation
  AppendByteArrayFast(packet, country);   // Country
  AssignLength(packet);

  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_AUTH_CHECK(BYTEARRAY clientToken, BYTEARRAY exeVersion, BYTEARRAY exeVersionHash, BYTEARRAY keyInfoROC, BYTEARRAY keyInfoTFT, string exeInfo, string keyOwnerName)
{
  BYTEARRAY packet;

  if (clientToken.size() == 4 && exeVersion.size() == 4 && exeVersionHash.size() == 4)
  {
    uint32_t NumKeys = 2;

    packet.push_back(BNET_HEADER_CONSTANT);   // BNET header constant
    packet.push_back(SID_AUTH_CHECK);   // SID_AUTH_CHECK
    packet.push_back(0);   // packet length will be assigned later
    packet.push_back(0);   // packet length will be assigned later
    AppendByteArrayFast(packet, clientToken);   // Client Token
    AppendByteArrayFast(packet, exeVersion);   // EXE Version
    AppendByteArrayFast(packet, exeVersionHash);   // EXE Version Hash
    AppendByteArray(packet, NumKeys, false);   // number of keys in this packet
    AppendByteArray(packet, (uint32_t) 0, false);   // boolean Using Spawn (32 bit)
    AppendByteArrayFast(packet, keyInfoROC);   // ROC Key Info
    AppendByteArrayFast(packet, keyInfoTFT);   // TFT Key Info
    AppendByteArrayFast(packet, exeInfo);   // EXE Info
    AppendByteArrayFast(packet, keyOwnerName);   // CD Key Owner Name
    AssignLength(packet);
  }
  else
    Print("[BNETPROTO] invalid parameters passed to SEND_SID_AUTH_CHECK");

  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_AUTH_ACCOUNTLOGON(BYTEARRAY clientPublicKey, string accountName)
{
  BYTEARRAY packet;

  if (clientPublicKey.size() == 32)
  {
    packet.push_back(BNET_HEADER_CONSTANT);   // BNET header constant
    packet.push_back(SID_AUTH_ACCOUNTLOGON);   // SID_AUTH_ACCOUNTLOGON
    packet.push_back(0);   // packet length will be assigned later
    packet.push_back(0);   // packet length will be assigned later
    AppendByteArrayFast(packet, clientPublicKey);   // Client Key
    AppendByteArrayFast(packet, accountName);   // Account Name
    AssignLength(packet);
  }
  else
    Print("[BNETPROTO] invalid parameters passed to SEND_SID_AUTH_ACCOUNTLOGON");

  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_AUTH_ACCOUNTLOGONPROOF(BYTEARRAY clientPasswordProof)
{
  BYTEARRAY packet;

  if (clientPasswordProof.size() == 20)
  {
    packet.push_back(BNET_HEADER_CONSTANT);   // BNET header constant
    packet.push_back(SID_AUTH_ACCOUNTLOGONPROOF);   // SID_AUTH_ACCOUNTLOGONPROOF
    packet.push_back(0);   // packet length will be assigned later
    packet.push_back(0);   // packet length will be assigned later
    AppendByteArrayFast(packet, clientPasswordProof);   // Client Password Proof
    AssignLength(packet);
  }
  else
    Print("[BNETPROTO] invalid parameters passed to SEND_SID_AUTH_ACCOUNTLOGON");

  return packet;
}

BYTEARRAY CBNETProtocol::SEND_SID_FRIENDLIST()
{
  return BYTEARRAY {BNET_HEADER_CONSTANT, SID_FRIENDLIST, 4, 0};
}

BYTEARRAY CBNETProtocol::SEND_SID_CLANMEMBERLIST()
{
  return BYTEARRAY {BNET_HEADER_CONSTANT, SID_CLANMEMBERLIST, 8, 0, 0, 0, 0, 0};
}

/////////////////////
// OTHER FUNCTIONS //
/////////////////////

bool CBNETProtocol::ValidateLength(const BYTEARRAY &content)
{
  // verify that bytes 3 and 4 (indices 2 and 3) of the content array describe the length

  return ((uint16_t)(content[3] << 8 | content[2]) == content.size());
}

//
// CIncomingGameHost
//

CIncomingGameHost::CIncomingGameHost(const BYTEARRAY &nIP, uint16_t nPort, const string &nGameName, const BYTEARRAY &nHostCounter)
  : m_GameName(nGameName),
    m_IP(nIP),
    m_HostCounter(nHostCounter),
    m_Port(nPort)
{

}

CIncomingGameHost::~CIncomingGameHost()
{

}

string CIncomingGameHost::GetIPString() const
{
  string Result;

  if (m_IP.size() >= 4)
  {
    for (uint32_t i = 0; i < 4; ++i)
    {
      Result += ToString((uint32_t) m_IP[i]);

      if (i < 3)
        Result += ".";
    }
  }

  return Result;
}

//
// CIncomingChatEvent
//

CIncomingChatEvent::CIncomingChatEvent(CBNETProtocol::IncomingChatEvent nChatEvent, const string &nUser, const string &nMessage)
  : m_User(nUser),
    m_Message(nMessage),
    m_ChatEvent(nChatEvent)
{

}

CIncomingChatEvent::~CIncomingChatEvent()
{

}
