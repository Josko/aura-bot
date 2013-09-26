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

#ifndef GAME_H
#define GAME_H

#include "gameslot.h"

//
// CGame
//

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
  CAura *m_Aura;

protected:
  CTCPServer *m_Socket;                     // listening socket
  CDBBan *m_DBBanLast;                      // last ban for the !banlast command - this is a pointer to one of the items in m_DBBans
  vector<CDBBan *> m_DBBans;                // vector of potential ban data for the database
  CStats *m_Stats;                          // class to keep track of game stats such as kills/deaths/assists in dota
  CGameProtocol *m_Protocol;                // game protocol
  vector<CGameSlot> m_Slots;                // vector of slots
  vector<CPotentialPlayer *> m_Potentials;  // vector of potential players (connections that haven't sent a W3GS_REQJOIN packet yet)
  vector<CDBGamePlayer *> m_DBGamePlayers;  // vector of potential gameplayer data for the database
  vector<CGamePlayer *> m_Players;          // vector of players
  queue<CIncomingAction *> m_Actions;       // queue of actions to be sent
  vector<string> m_Reserved;                // vector of player names with reserved slots (from the !hold command)
  set<string> m_IgnoredNames;               // set of player names to NOT print ban messages for when joining because they've already been printed
  vector<unsigned char> m_FakePlayers;      // the fake player's PIDs (if present)
  CMap *m_Map;                              // map data
  string m_GameName;                        // game name
  string m_LastGameName;                    // last game name (the previous game name before it was rehosted)
  string m_VirtualHostName;                 // host's name
  string m_OwnerName;                       // name of the player who owns this game (should be considered an admin)
  string m_CreatorName;                     // name of the player who created this game
  string m_CreatorServer;                   // battle.net server the player who created this game was on
  string m_KickVotePlayer;                  // the player to be kicked with the currently running kick vote
  string m_HCLCommandString;                // the "HostBot Command Library" command string, used to pass a limited amount of data to specially designed maps
  string m_MapPath;                         // store the map path to save in the database on game end
  uint32_t m_RandomSeed;                    // the random seed sent to the Warcraft III clients
  uint32_t m_HostCounter;                   // a unique game number
  uint32_t m_EntryKey;                      // random entry key for LAN, used to prove that a player is actually joining from LAN
  uint32_t m_Latency;                       // the number of ms to wait between sending action packets (we queue any received during this time)
  uint32_t m_SyncLimit;                     // the maximum number of packets a player can fall out of sync before starting the lag screen
  uint32_t m_SyncCounter;                   // the number of actions sent so far (for determining if anyone is lagging)
  uint32_t m_GameTicks;                     // ingame ticks
  uint32_t m_CreationTime;                  // GetTime when the game was created
  uint32_t m_LastPingTime;                  // GetTime when the last ping was sent
  uint32_t m_LastRefreshTime;               // GetTime when the last game refresh was sent
  uint32_t m_LastDownloadTicks;             // GetTicks when the last map download cycle was performed
  uint32_t m_DownloadCounter;               // # of map bytes downloaded in the last second
  uint32_t m_LastDownloadCounterResetTicks; // GetTicks when the download counter was last reset
  uint32_t m_LastCountDownTicks;            // GetTicks when the last countdown message was sent
  uint32_t m_CountDownCounter;              // the countdown is finished when this reaches zero
  uint32_t m_StartedLoadingTicks;           // GetTicks when the game started loading
  uint32_t m_StartPlayers;                  // number of players when the game started
  uint32_t m_LastLagScreenResetTime;        // GetTime when the "lag" screen was last reset
  uint32_t m_LastActionSentTicks;           // GetTicks when the last action packet was sent
  uint32_t m_LastActionLateBy;              // the number of ticks we were late sending the last action packet by
  uint32_t m_StartedLaggingTime;            // GetTime when the last lag screen started
  uint32_t m_LastLagScreenTime;             // GetTime when the last lag screen was active (continuously updated)
  uint32_t m_LastReservedSeen;              // GetTime when the last reserved player was seen in the lobby
  uint32_t m_StartedKickVoteTime;           // GetTime when the kick vote was started
  uint32_t m_GameOverTime;                  // GetTime when the game was over
  uint32_t m_LastPlayerLeaveTicks;          // GetTicks when the most recent player left the game
  uint16_t m_HostPort;                      // the port to host games on
  unsigned char m_GameState;                // game state, public or private
  unsigned char m_VirtualHostPID;           // host's PID
  unsigned char m_GProxyEmptyActions;       // empty actions used for gproxy protocol
  bool m_Exiting;                           // set to true and this class will be deleted next update
  bool m_Saving;                            // if we're currently saving game data to the database
  bool m_SlotInfoChanged;                   // if the slot info has changed and hasn't been sent to the players yet (optimization)
  bool m_Locked;                            // if the game owner is the only one allowed to run game commands or not
  bool m_RefreshError;                      // if the game had a refresh error
  bool m_MuteAll;                           // if we should stop forwarding ingame chat messages targeted for all players or not
  bool m_MuteLobby;                         // if we should stop forwarding lobby chat messages
  bool m_CountDownStarted;                  // if the game start countdown has started or not
  bool m_GameLoading;                       // if the game is currently loading or not
  bool m_GameLoaded;                        // if the game has loaded or not
  bool m_Lagging;                           // if the lag screen is active or not
  bool m_Desynced;                          // if the game has desynced or not

public:
  CGame(CAura *nAura, CMap *nMap, uint16_t nHostPort, unsigned char nGameState, string &nGameName, string &nOwnerName, string &nCreatorName, string &nCreatorServer);
  ~CGame();

  inline CMap *GetMap() const                             { return m_Map; }
  inline CGameProtocol *GetProtocol() const               { return m_Protocol; }
  inline uint32_t GetEntryKey() const                     { return m_EntryKey; }
  inline uint16_t GetHostPort() const                     { return m_HostPort; }
  inline unsigned char GetGameState() const               { return m_GameState; }
  inline unsigned char GetGProxyEmptyActions() const      { return m_GProxyEmptyActions; }
  inline string GetGameName() const                       { return m_GameName; }
  inline string GetLastGameName() const                   { return m_LastGameName; }
  inline string GetVirtualHostName() const                { return m_VirtualHostName; }
  inline string GetOwnerName() const                      { return m_OwnerName; }
  inline string GetCreatorName() const                    { return m_CreatorName; }
  inline string GetCreatorServer() const                  { return m_CreatorServer; }
  inline uint32_t GetHostCounter() const                  { return m_HostCounter; }
  inline uint32_t GetLastLagScreenTime() const            { return m_LastLagScreenTime; }
  inline bool GetLocked() const                           { return m_Locked; }
  inline bool GetCountDownStarted() const                 { return m_CountDownStarted; }
  inline bool GetGameLoading() const                      { return m_GameLoading; }
  inline bool GetGameLoaded() const                       { return m_GameLoaded; }
  inline bool GetLagging() const                          { return m_Lagging; }

  uint32_t GetNextTimedActionTicks() const;
  uint32_t GetSlotsOccupied() const;
  uint32_t GetSlotsOpen() const;
  uint32_t GetNumPlayers() const;
  uint32_t GetNumHumanPlayers() const;
  string GetDescription() const;
  string GetPlayers() const;

  inline void SetExiting(bool nExiting)                      { m_Exiting = nExiting; }
  inline void SetRefreshError(bool nRefreshError)            { m_RefreshError = nRefreshError; }

  // processing functions

  unsigned int SetFD(void *fd, void *send_fd, int *nfds);
  bool Update(void *fd, void *send_fd);
  void UpdatePost(void *send_fd);

  // generic functions to send packets to players

  void Send(CGamePlayer *player, const BYTEARRAY &data);
  void Send(unsigned char PID, const BYTEARRAY &data);
  void Send(const BYTEARRAY &PIDs, const BYTEARRAY &data);
  void SendAll(const BYTEARRAY &data);

  // functions to send packets to players

  void SendChat(unsigned char fromPID, CGamePlayer *player, const string &message);
  void SendChat(unsigned char fromPID, unsigned char toPID, const string &message);
  void SendChat(CGamePlayer *player, const string &message);
  void SendChat(unsigned char toPID, const string &message);
  void SendAllChat(unsigned char fromPID, const string &message);
  void SendAllChat(const string &message);
  void SendAllSlotInfo();
  void SendVirtualHostPlayerInfo(CGamePlayer *player);
  void SendFakePlayerInfo(CGamePlayer *player);
  void SendAllActions();

  // events
  // note: these are only called while iterating through the m_Potentials or m_Players vectors
  // therefore you can't modify those vectors and must use the player's m_DeleteMe member to flag for deletion

  void EventPlayerDeleted(CGamePlayer *player);
  void EventPlayerDisconnectTimedOut(CGamePlayer *player);
  void EventPlayerDisconnectSocketError(CGamePlayer *player);
  void EventPlayerDisconnectConnectionClosed(CGamePlayer *player);
  void EventPlayerJoined(CPotentialPlayer *potential, CIncomingJoinPlayer *joinPlayer);
  void EventPlayerLeft(CGamePlayer *player, uint32_t reason);
  void EventPlayerLoaded(CGamePlayer *player);
  void EventPlayerAction(CGamePlayer *player, CIncomingAction *action);
  void EventPlayerKeepAlive(CGamePlayer *player);
  void EventPlayerChatToHost(CGamePlayer *player, CIncomingChatPlayer *chatPlayer);
  bool EventPlayerBotCommand(CGamePlayer *player, string &command, string &payload);
  void EventPlayerChangeTeam(CGamePlayer *player, unsigned char team);
  void EventPlayerChangeColour(CGamePlayer *player, unsigned char colour);
  void EventPlayerChangeRace(CGamePlayer *player, unsigned char race);
  void EventPlayerChangeHandicap(CGamePlayer *player, unsigned char handicap);
  void EventPlayerDropRequest(CGamePlayer *player);
  void EventPlayerMapSize(CGamePlayer *player, CIncomingMapSize *mapSize);
  void EventPlayerPongToHost(CGamePlayer *player);

  // these events are called outside of any iterations

  void EventGameStarted();
  void EventGameLoaded();

  // other functions

  unsigned char GetSIDFromPID(unsigned char PID);
  CGamePlayer *GetPlayerFromPID(unsigned char PID);
  CGamePlayer *GetPlayerFromSID(unsigned char SID);
  CGamePlayer *GetPlayerFromName(string name, bool sensitive);
  uint32_t GetPlayerFromNamePartial(string name, CGamePlayer **player);
  string GetDBPlayerNameFromColour(unsigned char colour) const;
  CGamePlayer *GetPlayerFromColour(unsigned char colour);
  unsigned char GetNewPID();
  unsigned char GetNewColour();
  BYTEARRAY GetPIDs();
  BYTEARRAY GetPIDs(unsigned char excludePID);
  unsigned char GetHostPID();
  unsigned char GetEmptySlot(bool reserved);
  unsigned char GetEmptySlot(unsigned char team, unsigned char PID);
  void SwapSlots(unsigned char SID1, unsigned char SID2);
  void OpenSlot(unsigned char SID, bool kick);
  void CloseSlot(unsigned char SID, bool kick);
  void ComputerSlot(unsigned char SID, unsigned char skill, bool kick);
  void ColourSlot(unsigned char SID, unsigned char colour);
  void OpenAllSlots();
  void CloseAllSlots();
  void ShuffleSlots();
  void AddToSpoofed(const string &server, const string &name, bool sendMessage);
  void AddToReserved(string name);
  bool IsOwner(string name);
  bool IsReserved(string name);
  bool IsDownloading();
  void StartCountDown(bool force);
  void StopPlayers(const string &reason);
  void StopLaggers(const string &reason);
  void CreateVirtualHost();
  void DeleteVirtualHost();
  void CreateFakePlayer();
  void DeleteFakePlayers();
};

#endif
