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

#ifndef AURA_GAME_H_
#define AURA_GAME_H_

#include "gameslot.h"

#include <set>
#include <queue>

//
// CGame
//

class CAura;
class CTCPServer;
class CGameProtocol;
class CPotentialPlayer;
class CGamePlayer;
class CMap;
class CIncomingJoinPlayer;
class CIncomingAction;
class CIncomingChatPlayer;
class CIncomingMapSize;
class CDBBan;
class CDBGamePlayer;
class CStats;
class CIRC;

class CGame
{
public:
  CAura* m_Aura;

protected:
  CTCPServer*                    m_Socket;                        // listening socket
  CDBBan*                        m_DBBanLast;                     // last ban for the !banlast command - this is a pointer to one of the items in m_DBBans
  std::vector<CDBBan*>           m_DBBans;                        // std::vector of potential ban data for the database
  CStats*                        m_Stats;                         // class to keep track of game stats such as kills/deaths/assists in dota
  CGameProtocol*                 m_Protocol;                      // game protocol
  std::vector<CGameSlot>         m_Slots;                         // std::vector of slots
  std::vector<CPotentialPlayer*> m_Potentials;                    // std::vector of potential players (connections that haven't sent a W3GS_REQJOIN packet yet)
  std::vector<CDBGamePlayer*>    m_DBGamePlayers;                 // std::vector of potential gameplayer data for the database
  std::vector<CGamePlayer*>      m_Players;                       // std::vector of players
  std::queue<CIncomingAction*>   m_Actions;                       // queue of actions to be sent
  std::vector<std::string>       m_Reserved;                      // std::vector of player names with reserved slots (from the !hold command)
  std::set<std::string>          m_IgnoredNames;                  // set of player names to NOT print ban messages for when joining because they've already been printed
  std::vector<uint8_t>           m_FakePlayers;                   // the fake player's PIDs (if present)
  CMap*                          m_Map;                           // map data
  std::string                    m_GameName;                      // game name
  std::string                    m_LastGameName;                  // last game name (the previous game name before it was rehosted)
  std::string                    m_VirtualHostName;               // host's name
  std::string                    m_OwnerName;                     // name of the player who owns this game (should be considered an admin)
  std::string                    m_CreatorName;                   // name of the player who created this game
  std::string                    m_CreatorServer;                 // battle.net server the player who created this game was on
  std::string                    m_KickVotePlayer;                // the player to be kicked with the currently running kick vote
  std::string                    m_HCLCommandString;              // the "HostBot Command Library" command std::string, used to pass a limited amount of data to specially designed maps
  std::string                    m_MapPath;                       // store the map path to save in the database on game end
  int64_t                        m_GameTicks;                     // ingame ticks
  int64_t                        m_CreationTime;                  // GetTime when the game was created
  int64_t                        m_LastPingTime;                  // GetTime when the last ping was sent
  int64_t                        m_LastRefreshTime;               // GetTime when the last game refresh was sent
  int64_t                        m_LastDownloadTicks;             // GetTicks when the last map download cycle was performed
  int64_t                        m_LastDownloadCounterResetTicks; // GetTicks when the download counter was last reset
  int64_t                        m_LastCountDownTicks;            // GetTicks when the last countdown message was sent
  int64_t                        m_StartedLoadingTicks;           // GetTicks when the game started loading
  int64_t                        m_LastActionSentTicks;           // GetTicks when the last action packet was sent
  int64_t                        m_LastActionLateBy;              // the number of ticks we were late sending the last action packet by
  int64_t                        m_StartedLaggingTime;            // GetTime when the last lag screen started
  int64_t                        m_LastLagScreenTime;             // GetTime when the last lag screen was active (continuously updated)
  int64_t                        m_LastReservedSeen;              // GetTime when the last reserved player was seen in the lobby
  int64_t                        m_StartedKickVoteTime;           // GetTime when the kick vote was started
  int64_t                        m_GameOverTime;                  // GetTime when the game was over
  int64_t                        m_LastPlayerLeaveTicks;          // GetTicks when the most recent player left the game
  int64_t                        m_LastLagScreenResetTime;        // GetTime when the "lag" screen was last reset
  int64_t                        m_RandomSeed;                    // the random seed sent to the Warcraft III clients
  uint32_t                       m_HostCounter;                   // a unique game number
  uint32_t                       m_EntryKey;                      // random entry key for LAN, used to prove that a player is actually joining from LAN
  uint32_t                       m_Latency;                       // the number of ms to wait between sending action packets (we queue any received during this time)
  uint32_t                       m_SyncLimit;                     // the maximum number of packets a player can fall out of sync before starting the lag screen
  uint32_t                       m_SyncCounter;                   // the number of actions sent so far (for determining if anyone is lagging)
  uint32_t                       m_DownloadCounter;               // # of map bytes downloaded in the last second
  uint32_t                       m_CountDownCounter;              // the countdown is finished when this reaches zero
  uint32_t                       m_StartPlayers;                  // number of players when the game started
  uint16_t                       m_HostPort;                      // the port to host games on
  uint8_t                        m_GameState;                     // game state, public or private
  uint8_t                        m_VirtualHostPID;                // host's PID
  uint8_t                        m_GProxyEmptyActions;            // empty actions used for gproxy protocol
  bool                           m_Exiting;                       // set to true and this class will be deleted next update
  bool                           m_Saving;                        // if we're currently saving game data to the database
  bool                           m_SlotInfoChanged;               // if the slot info has changed and hasn't been sent to the players yet (optimization)
  bool                           m_Locked;                        // if the game owner is the only one allowed to run game commands or not
  bool                           m_RefreshError;                  // if the game had a refresh error
  bool                           m_MuteAll;                       // if we should stop forwarding ingame chat messages targeted for all players or not
  bool                           m_MuteLobby;                     // if we should stop forwarding lobby chat messages
  bool                           m_CountDownStarted;              // if the game start countdown has started or not
  bool                           m_GameLoading;                   // if the game is currently loading or not
  bool                           m_GameLoaded;                    // if the game has loaded or not
  bool                           m_Lagging;                       // if the lag screen is active or not
  bool                           m_Desynced;                      // if the game has desynced or not

public:
  CGame(CAura* nAura, CMap* nMap, uint16_t nHostPort, uint8_t nGameState, std::string& nGameName, std::string& nOwnerName, std::string& nCreatorName, std::string& nCreatorServer);
  ~CGame();
  CGame(CGame&) = delete;

  inline CMap*          GetMap() const { return m_Map; }
  inline CGameProtocol* GetProtocol() const { return m_Protocol; }
  inline uint32_t       GetEntryKey() const { return m_EntryKey; }
  inline uint16_t       GetHostPort() const { return m_HostPort; }
  inline uint8_t        GetGameState() const { return m_GameState; }
  inline uint8_t        GetGProxyEmptyActions() const { return m_GProxyEmptyActions; }
  inline std::string    GetGameName() const { return m_GameName; }
  inline std::string    GetLastGameName() const { return m_LastGameName; }
  inline std::string    GetVirtualHostName() const { return m_VirtualHostName; }
  inline std::string    GetOwnerName() const { return m_OwnerName; }
  inline std::string    GetCreatorName() const { return m_CreatorName; }
  inline std::string    GetCreatorServer() const { return m_CreatorServer; }
  inline uint32_t       GetHostCounter() const { return m_HostCounter; }
  inline int64_t        GetLastLagScreenTime() const { return m_LastLagScreenTime; }
  inline bool           GetLocked() const { return m_Locked; }
  inline bool           GetCountDownStarted() const { return m_CountDownStarted; }
  inline bool           GetGameLoading() const { return m_GameLoading; }
  inline bool           GetGameLoaded() const { return m_GameLoaded; }
  inline bool           GetLagging() const { return m_Lagging; }

  int64_t     GetNextTimedActionTicks() const;
  uint32_t    GetSlotsOccupied() const;
  uint32_t    GetSlotsOpen() const;
  uint32_t    GetNumPlayers() const;
  uint32_t    GetNumHumanPlayers() const;
  std::string GetDescription() const;
  std::string GetPlayers() const;
  std::string GetObservers() const;

  inline void SetExiting(bool nExiting) { m_Exiting = nExiting; }
  inline void SetRefreshError(bool nRefreshError) { m_RefreshError = nRefreshError; }

  // processing functions

  uint32_t SetFD(void* fd, void* send_fd, int32_t* nfds);
  bool Update(void* fd, void* send_fd);
  void UpdatePost(void* send_fd);

  // generic functions to send packets to players

  void Send(CGamePlayer* player, const BYTEARRAY& data);
  void Send(uint8_t PID, const BYTEARRAY& data);
  void Send(const BYTEARRAY& PIDs, const BYTEARRAY& data);
  void SendAll(const BYTEARRAY& data);

  // functions to send packets to players

  void SendChat(uint8_t fromPID, CGamePlayer* player, const std::string& message);
  void SendChat(uint8_t fromPID, uint8_t toPID, const std::string& message);
  void SendChat(CGamePlayer* player, const std::string& message);
  void SendChat(uint8_t toPID, const std::string& message);
  void SendAllChat(uint8_t fromPID, const std::string& message);
  void SendAllChat(const std::string& message);
  void SendAllSlotInfo();
  void SendVirtualHostPlayerInfo(CGamePlayer* player);
  void SendFakePlayerInfo(CGamePlayer* player);
  void SendAllActions();

  // events
  // note: these are only called while iterating through the m_Potentials or m_Players std::vectors
  // therefore you can't modify those std::vectors and must use the player's m_DeleteMe member to flag for deletion

  void EventPlayerDeleted(CGamePlayer* player);
  void EventPlayerDisconnectTimedOut(CGamePlayer* player);
  void EventPlayerDisconnectSocketError(CGamePlayer* player);
  void EventPlayerDisconnectConnectionClosed(CGamePlayer* player);
  void EventPlayerJoined(CPotentialPlayer* potential, CIncomingJoinPlayer* joinPlayer);
  void EventPlayerLeft(CGamePlayer* player, uint32_t reason);
  void EventPlayerLoaded(CGamePlayer* player);
  void EventPlayerAction(CGamePlayer* player, CIncomingAction* action);
  void EventPlayerKeepAlive(CGamePlayer* player);
  void EventPlayerChatToHost(CGamePlayer* player, CIncomingChatPlayer* chatPlayer);
  bool EventPlayerBotCommand(CGamePlayer* player, std::string& command, std::string& payload);
  void EventPlayerChangeTeam(CGamePlayer* player, uint8_t team);
  void EventPlayerChangeColour(CGamePlayer* player, uint8_t colour);
  void EventPlayerChangeRace(CGamePlayer* player, uint8_t race);
  void EventPlayerChangeHandicap(CGamePlayer* player, uint8_t handicap);
  void EventPlayerDropRequest(CGamePlayer* player);
  void EventPlayerMapSize(CGamePlayer* player, CIncomingMapSize* mapSize);
  void EventPlayerPongToHost(CGamePlayer* player);

  // these events are called outside of any iterations

  void EventGameStarted();
  void EventGameLoaded();

  // other functions

  uint8_t GetSIDFromPID(uint8_t PID) const;
  CGamePlayer* GetPlayerFromPID(uint8_t PID) const;
  CGamePlayer* GetPlayerFromSID(uint8_t SID) const;
  CGamePlayer* GetPlayerFromName(std::string name, bool sensitive) const;
  uint32_t GetPlayerFromNamePartial(std::string name, CGamePlayer** player) const;
  std::string GetDBPlayerNameFromColour(uint8_t colour) const;
  CGamePlayer* GetPlayerFromColour(uint8_t colour) const;
  uint8_t   GetNewPID() const;
  uint8_t   GetNewColour() const;
  BYTEARRAY GetPIDs() const;
  BYTEARRAY GetPIDs(uint8_t excludePID) const;
  uint8_t GetHostPID() const;
  uint8_t GetEmptySlot(bool reserved) const;
  uint8_t GetEmptySlot(uint8_t team, uint8_t PID) const;
  void SwapSlots(uint8_t SID1, uint8_t SID2);
  void OpenSlot(uint8_t SID, bool kick);
  void CloseSlot(uint8_t SID, bool kick);
  void ComputerSlot(uint8_t SID, uint8_t skill, bool kick);
  void ColourSlot(uint8_t SID, uint8_t colour);
  void OpenAllSlots();
  void CloseAllSlots();
  void ShuffleSlots();
  void AddToSpoofed(const std::string& server, const std::string& name, bool sendMessage);
  void AddToReserved(std::string name);
  void RemoveFromReserved(std::string name);
  bool IsOwner(std::string name) const;
  bool IsReserved(std::string name) const;
  bool IsDownloading() const;
  void StartCountDown(bool force);
  void StopPlayers(const std::string& reason);
  void StopLaggers(const std::string& reason);
  void CreateVirtualHost();
  void DeleteVirtualHost();
  void CreateFakePlayer();
  void DeleteFakePlayers();
};

#endif // AURA_GAME_H_
