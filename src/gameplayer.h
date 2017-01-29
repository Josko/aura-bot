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

#ifndef AURA_GAMEPLAYER_H_
#define AURA_GAMEPLAYER_H_

#include "socket.h"

#include <queue>

class CTCPSocket;
class CGameProtocol;
class CGame;
class CIncomingJoinPlayer;

//
// CPotentialPlayer
//

class CPotentialPlayer
{
public:
  CGameProtocol* m_Protocol;
  CGame*         m_Game;

protected:
  // note: we permit m_Socket to be NULL in this class to allow for the virtual host player which doesn't really exist
  // it also allows us to convert CPotentialPlayers to CGamePlayers without the CPotentialPlayer's destructor closing the socket

  CTCPSocket*          m_Socket;
  CIncomingJoinPlayer* m_IncomingJoinPlayer;
  bool                 m_DeleteMe;

public:
  CPotentialPlayer(CGameProtocol* nProtocol, CGame* nGame, CTCPSocket* nSocket);
  ~CPotentialPlayer();

  inline CTCPSocket*          GetSocket() const { return m_Socket; }
  inline std::vector<uint8_t> GetExternalIP() const { return m_Socket->GetIP(); }
  inline std::string          GetExternalIPString() const { return m_Socket->GetIPString(); }
  inline bool                 GetDeleteMe() const { return m_DeleteMe; }
  inline CIncomingJoinPlayer* GetJoinPlayer() const { return m_IncomingJoinPlayer; }

  inline void SetSocket(CTCPSocket* nSocket) { m_Socket = nSocket; }
  inline void SetDeleteMe(bool nDeleteMe) { m_DeleteMe = nDeleteMe; }

  // processing functions

  bool Update(void* fd);

  // other functions

  void Send(const std::vector<uint8_t>& data) const;
};

//
// CGamePlayer
//

class CGamePlayer
{
public:
  CGameProtocol* m_Protocol;
  CGame*         m_Game;

protected:
  CTCPSocket* m_Socket; // note: we permit m_Socket to be NULL in this class to allow for the virtual host player which doesn't really exist

private:
  std::vector<uint8_t>             m_InternalIP;                   // the player's internal IP address as reported by the player when connecting
  std::vector<uint32_t>            m_Pings;                        // store the last few (10) pings received so we can take an average
  std::queue<uint32_t>             m_CheckSums;                    // the last few checksums the player has sent (for detecting desyncs)
  std::queue<std::vector<uint8_t>> m_GProxyBuffer;                 // buffer with data used with GProxy++
  std::string                      m_LeftReason;                   // the reason the player left the game
  std::string                      m_SpoofedRealm;                 // the realm the player last spoof checked :wq
  std::string                      m_JoinedRealm;                  // the realm the player joined on (probable, can be spoofed)
  std::string                      m_Name;                         // the player's name
  uint32_t                         m_TotalPacketsSent;             // the total number of packets sent to the player
  uint32_t                         m_TotalPacketsReceived;         // the total number of packets received from the player
  uint32_t                         m_LeftCode;                     // the code to be sent in W3GS_PLAYERLEAVE_OTHERS for why this player left the game
  uint32_t                         m_SyncCounter;                  // the number of keepalive packets received from this player
  int64_t                          m_JoinTime;                     // GetTime when the player joined the game (used to delay sending the /whois a few seconds to allow for some lag)
  uint32_t                         m_LastMapPartSent;              // the last mappart sent to the player (for sending more than one part at a time)
  uint32_t                         m_LastMapPartAcked;             // the last mappart acknowledged by the player
  int64_t                          m_StartedDownloadingTicks;      // GetTicks when the player started downloading the map
  int64_t                          m_FinishedDownloadingTime;      // GetTime when the player finished downloading the map
  int64_t                          m_FinishedLoadingTicks;         // GetTicks when the player finished loading the game
  int64_t                          m_StartedLaggingTicks;          // GetTicks when the player started laggin
  int64_t                          m_LastGProxyWaitNoticeSentTime; // GetTime when the last disconnection notice has been sent when using GProxy++
  uint32_t                         m_GProxyReconnectKey;           // the GProxy++ reconnect key
  int64_t                          m_LastGProxyAckTime;            // GetTime when we last acknowledged GProxy++ packet
  uint8_t                          m_PID;                          // the player's PID
  bool                             m_Spoofed;                      // if the player has spoof checked or not
  bool                             m_Reserved;                     // if the player is reserved (VIP) or not
  bool                             m_WhoisShouldBeSent;            // if a battle.net /whois should be sent for this player or not
  bool                             m_WhoisSent;                    // if we've sent a battle.net /whois for this player yet (for spoof checking)
  bool                             m_DownloadAllowed;              // if we're allowed to download the map or not (used with permission based map downloads)
  bool                             m_DownloadStarted;              // if we've started downloading the map or not
  bool                             m_DownloadFinished;             // if we've finished downloading the map or not
  bool                             m_FinishedLoading;              // if the player has finished loading or not
  bool                             m_Lagging;                      // if the player is lagging or not (on the lag screen)
  bool                             m_DropVote;                     // if the player voted to drop the laggers or not (on the lag screen)
  bool                             m_KickVote;                     // if the player voted to kick a player or not
  bool                             m_Muted;                        // if the player is muted or not
  bool                             m_LeftMessageSent;              // if the playerleave message has been sent or not
  bool                             m_GProxy;                       // if the player is using GProxy++
  bool                             m_GProxyDisconnectNoticeSent;   // if a disconnection notice has been sent or not when using GProxy++

protected:
  bool m_DeleteMe;

public:
  CGamePlayer(CPotentialPlayer* potential, uint8_t nPID, std::string nJoinedRealm, std::string nName, std::vector<uint8_t> nInternalIP, bool nReserved);
  ~CGamePlayer();

  uint32_t GetPing(bool LCPing) const;
  inline CTCPSocket*           GetSocket() const { return m_Socket; }
  inline std::vector<uint8_t>  GetExternalIP() const { return m_Socket->GetIP(); }
  inline std::string           GetExternalIPString() const { return m_Socket->GetIPString(); }
  inline bool                  GetDeleteMe() const { return m_DeleteMe; }
  inline uint8_t               GetPID() const { return m_PID; }
  inline std::string           GetName() const { return m_Name; }
  inline std::vector<uint8_t>  GetInternalIP() const { return m_InternalIP; }
  inline uint32_t              GetNumPings() const { return m_Pings.size(); }
  inline uint32_t              GetNumCheckSums() const { return m_CheckSums.size(); }
  inline std::queue<uint32_t>* GetCheckSums() { return &m_CheckSums; }
  inline std::string           GetLeftReason() const { return m_LeftReason; }
  inline std::string           GetSpoofedRealm() const { return m_SpoofedRealm; }
  inline std::string           GetJoinedRealm() const { return m_JoinedRealm; }
  inline uint32_t              GetLeftCode() const { return m_LeftCode; }
  inline uint32_t              GetSyncCounter() const { return m_SyncCounter; }
  inline int64_t               GetJoinTime() const { return m_JoinTime; }
  inline uint32_t              GetLastMapPartSent() const { return m_LastMapPartSent; }
  inline uint32_t              GetLastMapPartAcked() const { return m_LastMapPartAcked; }
  inline int64_t               GetStartedDownloadingTicks() const { return m_StartedDownloadingTicks; }
  inline int64_t               GetFinishedDownloadingTime() const { return m_FinishedDownloadingTime; }
  inline int64_t               GetFinishedLoadingTicks() const { return m_FinishedLoadingTicks; }
  inline int64_t               GetStartedLaggingTicks() const { return m_StartedLaggingTicks; }
  inline int64_t               GetLastGProxyWaitNoticeSentTime() const { return m_LastGProxyWaitNoticeSentTime; }
  inline uint32_t              GetGProxyReconnectKey() const { return m_GProxyReconnectKey; }
  inline bool                  GetGProxy() const { return m_GProxy; }
  inline bool                  GetGProxyDisconnectNoticeSent() const { return m_GProxyDisconnectNoticeSent; }
  inline bool                  GetSpoofed() const { return m_Spoofed; }
  inline bool                  GetReserved() const { return m_Reserved; }
  inline bool                  GetWhoisShouldBeSent() const { return m_WhoisShouldBeSent; }
  inline bool                  GetWhoisSent() const { return m_WhoisSent; }
  inline bool                  GetDownloadAllowed() const { return m_DownloadAllowed; }
  inline bool                  GetDownloadStarted() const { return m_DownloadStarted; }
  inline bool                  GetDownloadFinished() const { return m_DownloadFinished; }
  inline bool                  GetFinishedLoading() const { return m_FinishedLoading; }
  inline bool                  GetLagging() const { return m_Lagging; }
  inline bool                  GetDropVote() const { return m_DropVote; }
  inline bool                  GetKickVote() const { return m_KickVote; }
  inline bool                  GetMuted() const { return m_Muted; }
  inline bool                  GetLeftMessageSent() const { return m_LeftMessageSent; }
  inline void SetSocket(CTCPSocket* nSocket) { m_Socket = nSocket; }
  inline void SetDeleteMe(bool nDeleteMe) { m_DeleteMe = nDeleteMe; }
  inline void SetLeftReason(const std::string& nLeftReason) { m_LeftReason = nLeftReason; }
  inline void SetSpoofedRealm(const std::string& nSpoofedRealm) { m_SpoofedRealm = nSpoofedRealm; }
  inline void SetLeftCode(uint32_t nLeftCode) { m_LeftCode = nLeftCode; }
  inline void SetSyncCounter(uint32_t nSyncCounter) { m_SyncCounter = nSyncCounter; }
  inline void SetLastMapPartSent(uint32_t nLastMapPartSent) { m_LastMapPartSent = nLastMapPartSent; }
  inline void SetLastMapPartAcked(uint32_t nLastMapPartAcked) { m_LastMapPartAcked = nLastMapPartAcked; }
  inline void SetStartedDownloadingTicks(uint64_t nStartedDownloadingTicks) { m_StartedDownloadingTicks = nStartedDownloadingTicks; }
  inline void SetFinishedDownloadingTime(uint64_t nFinishedDownloadingTime) { m_FinishedDownloadingTime = nFinishedDownloadingTime; }
  inline void SetStartedLaggingTicks(uint64_t nStartedLaggingTicks) { m_StartedLaggingTicks = nStartedLaggingTicks; }
  inline void SetSpoofed(bool nSpoofed) { m_Spoofed = nSpoofed; }
  inline void SetReserved(bool nReserved) { m_Reserved = nReserved; }
  inline void SetWhoisShouldBeSent(bool nWhoisShouldBeSent) { m_WhoisShouldBeSent = nWhoisShouldBeSent; }
  inline void SetDownloadAllowed(bool nDownloadAllowed) { m_DownloadAllowed = nDownloadAllowed; }
  inline void SetDownloadStarted(bool nDownloadStarted) { m_DownloadStarted = nDownloadStarted; }
  inline void SetDownloadFinished(bool nDownloadFinished) { m_DownloadFinished = nDownloadFinished; }
  inline void SetLagging(bool nLagging) { m_Lagging = nLagging; }
  inline void SetDropVote(bool nDropVote) { m_DropVote = nDropVote; }
  inline void SetKickVote(bool nKickVote) { m_KickVote = nKickVote; }
  inline void SetMuted(bool nMuted) { m_Muted = nMuted; }
  inline void SetLeftMessageSent(bool nLeftMessageSent) { m_LeftMessageSent = nLeftMessageSent; }
  inline void SetGProxyDisconnectNoticeSent(bool nGProxyDisconnectNoticeSent) { m_GProxyDisconnectNoticeSent = nGProxyDisconnectNoticeSent; }
  inline void SetLastGProxyWaitNoticeSentTime(uint64_t nLastGProxyWaitNoticeSentTime) { m_LastGProxyWaitNoticeSentTime = nLastGProxyWaitNoticeSentTime; }

  // processing functions

  bool Update(void* fd);

  // other functions

  void Send(const std::vector<uint8_t>& data);
  void EventGProxyReconnect(CTCPSocket* NewSocket, uint32_t LastPacket);
};

#endif // AURA_GAMEPLAYER_H_
