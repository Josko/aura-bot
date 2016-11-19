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

#ifndef AURA_BNETPROTOCOL_H_
#define AURA_BNETPROTOCOL_H_

//
// CBNETProtocol
//

#define BNET_HEADER_CONSTANT 255

#include <string>
#include <vector>

class CIncomingGameHost;
class CIncomingChatEvent;

class CBNETProtocol
{
public:
  enum Protocol
  {
    SID_NULL                   = 0,   // 0x0
    SID_STOPADV                = 2,   // 0x2
    SID_GETADVLISTEX           = 9,   // 0x9
    SID_ENTERCHAT              = 10,  // 0xA
    SID_JOINCHANNEL            = 12,  // 0xC
    SID_CHATCOMMAND            = 14,  // 0xE
    SID_CHATEVENT              = 15,  // 0xF
    SID_CHECKAD                = 21,  // 0x15
    SID_STARTADVEX3            = 28,  // 0x1C
    SID_DISPLAYAD              = 33,  // 0x21
    SID_NOTIFYJOIN             = 34,  // 0x22
    SID_PING                   = 37,  // 0x25
    SID_LOGONRESPONSE          = 41,  // 0x29
    SID_NETGAMEPORT            = 69,  // 0x45
    SID_AUTH_INFO              = 80,  // 0x50
    SID_AUTH_CHECK             = 81,  // 0x51
    SID_AUTH_ACCOUNTLOGON      = 83,  // 0x53
    SID_AUTH_ACCOUNTLOGONPROOF = 84,  // 0x54
    SID_WARDEN                 = 94,  // 0x5E
    SID_FRIENDLIST             = 101, // 0x65
    SID_FRIENDSUPDATE          = 102, // 0x66
    SID_CLANMEMBERLIST         = 125, // 0x7D
    SID_CLANMEMBERSTATUSCHANGE = 127  // 0x7F
  };

  enum KeyResult
  {
    KR_GOOD             = 0,
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
  std::vector<uint8_t> m_ClientToken;         // set in constructor
  std::vector<uint8_t> m_LogonType;           // set in RECEIVE_SID_AUTH_INFO
  std::vector<uint8_t> m_ServerToken;         // set in RECEIVE_SID_AUTH_INFO
  std::vector<uint8_t> m_MPQFileTime;         // set in RECEIVE_SID_AUTH_INFO
  std::vector<uint8_t> m_IX86VerFileName;     // set in RECEIVE_SID_AUTH_INFO
  std::vector<uint8_t> m_ValueStringFormula;  // set in RECEIVE_SID_AUTH_INFO
  std::vector<uint8_t> m_KeyState;            // set in RECEIVE_SID_AUTH_CHECK
  std::vector<uint8_t> m_KeyStateDescription; // set in RECEIVE_SID_AUTH_CHECK
  std::vector<uint8_t> m_Salt;                // set in RECEIVE_SID_AUTH_ACCOUNTLOGON
  std::vector<uint8_t> m_ServerPublicKey;     // set in RECEIVE_SID_AUTH_ACCOUNTLOGON
  std::vector<uint8_t> m_UniqueName;          // set in RECEIVE_SID_ENTERCHAT

public:
  CBNETProtocol();
  ~CBNETProtocol();

  inline std::vector<uint8_t> GetClientToken() const { return m_ClientToken; }
  inline std::vector<uint8_t> GetLogonType() const { return m_LogonType; }
  inline std::vector<uint8_t> GetServerToken() const { return m_ServerToken; }
  inline std::vector<uint8_t> GetMPQFileTime() const { return m_MPQFileTime; }
  inline std::vector<uint8_t> GetIX86VerFileName() const { return m_IX86VerFileName; }
  inline std::string          GetIX86VerFileNameString() const { return std::string(begin(m_IX86VerFileName), end(m_IX86VerFileName)); }
  inline std::vector<uint8_t> GetValueStringFormula() const { return m_ValueStringFormula; }
  inline std::string          GetValueStringFormulaString() const { return std::string(begin(m_ValueStringFormula), end(m_ValueStringFormula)); }
  inline std::vector<uint8_t> GetKeyState() const { return m_KeyState; }
  inline std::string          GetKeyStateDescription() const { return std::string(begin(m_KeyStateDescription), end(m_KeyStateDescription)); }
  inline std::vector<uint8_t> GetSalt() const { return m_Salt; }
  inline std::vector<uint8_t> GetServerPublicKey() const { return m_ServerPublicKey; }
  inline std::vector<uint8_t> GetUniqueName() const { return m_UniqueName; }

  // receive functions

  bool RECEIVE_SID_NULL(const std::vector<uint8_t>& data);
  CIncomingGameHost* RECEIVE_SID_GETADVLISTEX(const std::vector<uint8_t>& data);
  bool RECEIVE_SID_ENTERCHAT(const std::vector<uint8_t>& data);
  CIncomingChatEvent* RECEIVE_SID_CHATEVENT(const std::vector<uint8_t>& data);
  bool RECEIVE_SID_CHECKAD(const std::vector<uint8_t>& data);
  bool RECEIVE_SID_STARTADVEX3(const std::vector<uint8_t>& data);
  std::vector<uint8_t> RECEIVE_SID_PING(const std::vector<uint8_t>& data);
  bool RECEIVE_SID_AUTH_INFO(const std::vector<uint8_t>& data);
  bool RECEIVE_SID_AUTH_CHECK(const std::vector<uint8_t>& data);
  bool RECEIVE_SID_AUTH_ACCOUNTLOGON(const std::vector<uint8_t>& data);
  bool RECEIVE_SID_AUTH_ACCOUNTLOGONPROOF(const std::vector<uint8_t>& data);
  std::vector<std::string> RECEIVE_SID_FRIENDLIST(const std::vector<uint8_t>& data);
  std::vector<std::string> RECEIVE_SID_CLANMEMBERLIST(const std::vector<uint8_t>& data);

  // send functions

  std::vector<uint8_t> SEND_PROTOCOL_INITIALIZE_SELECTOR();
  std::vector<uint8_t> SEND_SID_NULL();
  std::vector<uint8_t> SEND_SID_STOPADV();
  std::vector<uint8_t> SEND_SID_GETADVLISTEX(const std::string& gameName);
  std::vector<uint8_t> SEND_SID_ENTERCHAT();
  std::vector<uint8_t> SEND_SID_JOINCHANNEL(const std::string& channel);
  std::vector<uint8_t> SEND_SID_CHATCOMMAND(const std::string& command);
  std::vector<uint8_t> SEND_SID_CHECKAD();
  std::vector<uint8_t> SEND_SID_STARTADVEX3(uint8_t state, const std::vector<uint8_t>& mapGameType, const std::vector<uint8_t>& mapFlags, const std::vector<uint8_t>& mapWidth, const std::vector<uint8_t>& mapHeight, const std::string& gameName, const std::string& hostName, uint32_t upTime, const std::string& mapPath, const std::vector<uint8_t>& mapCRC, const std::vector<uint8_t>& mapSHA1, uint32_t hostCounter);
  std::vector<uint8_t> SEND_SID_NOTIFYJOIN(const std::string& gameName);
  std::vector<uint8_t> SEND_SID_PING(const std::vector<uint8_t>& pingValue);
  std::vector<uint8_t> SEND_SID_LOGONRESPONSE(const std::vector<uint8_t>& clientToken, const std::vector<uint8_t>& serverToken, const std::vector<uint8_t>& passwordHash, const std::string& accountName);
  std::vector<uint8_t> SEND_SID_NETGAMEPORT(uint16_t serverPort);
  std::vector<uint8_t> SEND_SID_AUTH_INFO(uint8_t ver, uint32_t localeID, const std::string& countryAbbrev, const std::string& country);
  std::vector<uint8_t> SEND_SID_AUTH_CHECK(const std::vector<uint8_t>& clientToken, const std::vector<uint8_t>& exeVersion, const std::vector<uint8_t>& exeVersionHash, const std::vector<uint8_t>& keyInfoROC, const std::vector<uint8_t>& keyInfoTFT, const std::string& exeInfo, const std::string& keyOwnerName);
  std::vector<uint8_t> SEND_SID_AUTH_ACCOUNTLOGON(const std::vector<uint8_t>& clientPublicKey, const std::string& accountName);
  std::vector<uint8_t> SEND_SID_AUTH_ACCOUNTLOGONPROOF(const std::vector<uint8_t>& clientPasswordProof);
  std::vector<uint8_t> SEND_SID_FRIENDLIST();
  std::vector<uint8_t> SEND_SID_CLANMEMBERLIST();

  // other functions

private:
  bool ValidateLength(const std::vector<uint8_t>& content);
};

//
// CIncomingGameHost
//

class CIncomingGameHost
{
private:
  std::string          m_GameName;
  std::vector<uint8_t> m_IP;
  std::vector<uint8_t> m_HostCounter;
  uint16_t             m_Port;

public:
  CIncomingGameHost(std::vector<uint8_t> nIP, uint16_t nPort, std::string nGameName, std::vector<uint8_t> nHostCounter);
  ~CIncomingGameHost();

  std::string                 GetIPString() const;
  inline std::vector<uint8_t> GetIP() const { return m_IP; }
  inline uint16_t             GetPort() const { return m_Port; }
  inline std::string          GetGameName() const { return m_GameName; }
  inline std::vector<uint8_t> GetHostCounter() const { return m_HostCounter; }
};

//
// CIncomingChatEvent
//

class CIncomingChatEvent
{
private:
  std::string                      m_User;
  std::string                      m_Message;
  CBNETProtocol::IncomingChatEvent m_ChatEvent;

public:
  CIncomingChatEvent(CBNETProtocol::IncomingChatEvent nChatEvent, std::string nUser, std::string nMessage);
  ~CIncomingChatEvent();

  inline CBNETProtocol::IncomingChatEvent GetChatEvent() const { return m_ChatEvent; }
  inline std::string                      GetUser() const { return m_User; }
  inline std::string                      GetMessage() const { return m_Message; }
};

#endif // AURA_BNETPROTOCOL_H_
