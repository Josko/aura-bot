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

#ifndef AURA_BNET_H_
#define AURA_BNET_H_

#include "includes.h"

#include <queue>

//
// CBNET
//

class CAura;
class CTCPClient;
class CBNCSUtilInterface;
class CBNETProtocol;
class CGameProtocol;
class CGame;
class CIncomingChatEvent;
class CDBBan;
class CIRC;
class CMap;

class CBNET
{
public:
  CAura* m_Aura;

private:
  CTCPClient*                      m_Socket;                    // the connection to battle.net
  CBNETProtocol*                   m_Protocol;                  // battle.net protocol
  CBNCSUtilInterface*              m_BNCSUtil;                  // the interface to the bncsutil library (used for logging into battle.net)
  std::queue<std::vector<uint_fast8_t>> m_OutPackets;                // queue of outgoing packets to be sent (to prevent getting kicked for flooding)
  std::vector<std::string>         m_Friends;                   // std::vector of friends
  std::vector<std::string>         m_Clan;                      // std::vector of clan members
  std::vector<uint_fast8_t>             m_EXEVersion;                // custom exe version for PvPGN users
  std::vector<uint_fast8_t>             m_EXEVersionHash;            // custom exe version hash for PvPGN users
  std::string                      m_Server;                    // battle.net server to connect to
  std::string                      m_ServerIP;                  // battle.net server to connect to (the IP address so we don't have to resolve it every time we connect)
  std::string                      m_ServerAlias;               // battle.net server alias (short name, e.g. "USEast")
  std::string                      m_CDKeyROC;                  // ROC CD key
  std::string                      m_CDKeyTFT;                  // TFT CD key
  std::string                      m_CountryAbbrev;             // country abbreviation
  std::string                      m_Country;                   // country
  std::string                      m_UserName;                  // battle.net username
  std::string                      m_UserPassword;              // battle.net password
  std::string                      m_FirstChannel;              // the first chat channel to join upon entering chat (note: store the last channel when entering a game)
  std::string                      m_CurrentChannel;            // the current chat channel
  std::string                      m_IRC;                       // IRC channel we're sending the message to
  std::string                      m_PasswordHashType;          // password hash type for PvPGN users
  int64_t                          m_LastDisconnectedTime;      // GetTime when we were last disconnected from battle.net
  int64_t                          m_LastConnectionAttemptTime; // GetTime when we last attempted to connect to battle.net
  int64_t                          m_LastNullTime;              // GetTime when the last null packet was sent for detecting disconnects
  int64_t                          m_LastOutPacketTicks;        // GetTicks when the last packet was sent for the m_OutPackets queue
  int64_t                          m_LastAdminRefreshTime;      // GetTime when the admin list was last refreshed from the database
  int64_t                          m_LastBanRefreshTime;        // GetTime when the ban list was last refreshed from the database
  int64_t                          m_ReconnectDelay;            // interval between two consecutive connect attempts
  uint32_t                         m_LastOutPacketSize;         // byte size of the last packet we sent from the m_OutPackets queue
  uint32_t                         m_LocaleID;                  // see: http://msdn.microsoft.com/en-us/library/0h88fahh%28VS.85%29.aspx
  uint32_t                         m_HostCounterID;             // the host counter ID to identify players from this realm
  uint_fast8_t                          m_War3Version;               // custom warcraft 3 version for PvPGN users
  char                             m_CommandTrigger;            // the character prefix to identify commands
  bool                             m_Exiting;                   // set to true and this class will be deleted next update
  bool                             m_FirstConnect;              // if we haven't tried to connect to battle.net yet
  bool                             m_WaitingToConnect;          // if we're waiting to reconnect to battle.net after being disconnected
  bool                             m_LoggedIn;                  // if we've logged into battle.net or not
  bool                             m_InChat;                    // if we've entered chat or not (but we're not necessarily in a chat channel yet
  bool                             m_PvPGN;                     // if this BNET connection is actually a PvPGN

public:
  CBNET(CAura* nAura, std::string nServer, std::string nServerAlias, std::string nCDKeyROC, std::string nCDKeyTFT, std::string nCountryAbbrev, std::string nCountry, uint32_t nLocaleID, std::string nUserName, std::string nUserPassword, std::string nFirstChannel, char nCommandTrigger, uint_fast8_t nWar3Version, std::vector<uint_fast8_t> nEXEVersion, std::vector<uint_fast8_t> nEXEVersionHash, std::string nPasswordHashType, uint32_t nHostCounterID);
  ~CBNET();
  CBNET(CBNET&) = delete;

  inline bool                 GetExiting() const { return m_Exiting; }
  inline std::string          GetServer() const { return m_Server; }
  inline std::string          GetServerAlias() const { return m_ServerAlias; }
  inline std::string          GetCDKeyROC() const { return m_CDKeyROC; }
  inline std::string          GetCDKeyTFT() const { return m_CDKeyTFT; }
  inline std::string          GetUserName() const { return m_UserName; }
  inline std::string          GetUserPassword() const { return m_UserPassword; }
  inline std::string          GetFirstChannel() const { return m_FirstChannel; }
  inline std::string          GetCurrentChannel() const { return m_CurrentChannel; }
  inline char                 GetCommandTrigger() const { return m_CommandTrigger; }
  inline std::vector<uint_fast8_t> GetEXEVersion() const { return m_EXEVersion; }
  inline std::vector<uint_fast8_t> GetEXEVersionHash() const { return m_EXEVersionHash; }
  inline std::string          GetPasswordHashType() const { return m_PasswordHashType; }
  inline uint32_t             GetHostCounterID() const { return m_HostCounterID; }
  inline bool                 GetLoggedIn() const { return m_LoggedIn; }
  inline bool                 GetInChat() const { return m_InChat; }
  inline uint32_t             GetOutPacketsQueued() const { return m_OutPackets.size(); }
  inline bool                 GetPvPGN() const { return m_PvPGN; }

  // processing functions

  uint32_t SetFD(void* fd, void* send_fd, int32_t* nfds);
  bool Update(void* fd, void* send_fd);
  void ProcessChatEvent(const CIncomingChatEvent* chatEvent);

  // functions to send packets to battle.net

  void SendGetFriendsList();
  void SendGetClanList();
  void QueueEnterChat();
  void QueueChatCommand(const std::string& chatCommand);
  void QueueChatCommand(const std::string& chatCommand, const std::string& user, bool whisper, const std::string& irc);
  void QueueGameCreate(uint_fast8_t state, const std::string& gameName, CMap* map, uint32_t hostCounter);
  void QueueGameRefresh(uint_fast8_t state, const std::string& gameName, CMap* map, uint32_t hostCounter);
  void QueueGameUncreate();

  void UnqueueGameRefreshes();

  // other functions

  bool IsAdmin(std::string name);
  bool IsRootAdmin(std::string name);
  CDBBan* IsBannedName(std::string name);
  void HoldFriends(CGame* game);
  void HoldClan(CGame* game);

private:
  std::vector<std::string> MapFilesMatch(std::string pattern);
  std::vector<std::string> ConfigFilesMatch(std::string pattern);
};

#endif // AURA_BNET_H_
