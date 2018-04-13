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

#ifndef AURA_AURA_H_
#define AURA_AURA_H_

#include <cstdint>
#include <vector>
#include <string>

//
// CAura
//

class CUDPSocket;
class CTCPSocket;
class CTCPServer;
class CGPSProtocol;
class CCRC32;
class CSHA1;
class CBNET;
class CGame;
class CAuraDB;
class CMap;
class CConfig;
class CIRC;

class CAura
{
public:
  CIRC*                    m_IRC;
  CUDPSocket*              m_UDPSocket;                  // a UDP socket for sending broadcasts and other junk (used with !sendlan)
  CTCPServer*              m_ReconnectSocket;            // listening socket for GProxy++ reliable reconnects
  std::vector<CTCPSocket*> m_ReconnectSockets;           // std::vector of sockets attempting to reconnect (connected but not identified yet)
  CGPSProtocol*            m_GPSProtocol;                // class for gproxy protocol
  CCRC32*                  m_CRC;                        // for calculating CRC's
  CSHA1*                   m_SHA;                        // for calculating SHA1's
  std::vector<CBNET*>      m_BNETs;                      // all our battle.net connections (there can be more than one)
  CGame*                   m_CurrentGame;                // this game is still in the lobby state
  std::vector<CGame*>      m_Games;                      // these games are in progress
  CAuraDB*                 m_DB;                         // database
  CMap*                    m_Map;                        // the currently loaded map
  std::string              m_Version;                    // Aura++ version string
  std::string              m_MapCFGPath;                 // config value: map cfg path
  std::string              m_MapPath;                    // config value: map path
  std::string              m_VirtualHostName;            // config value: virtual host name
  std::string              m_LanguageFile;               // config value: language file
  std::string              m_Warcraft3Path;              // config value: Warcraft 3 path
  std::string              m_BindAddress;                // config value: the address to host games on
  std::string              m_DefaultMap;                 // config value: default map (map.cfg)
  uint32_t                 m_ReconnectWaitTime;          // config value: the maximum number of minutes to wait for a GProxy++ reliable reconnect
  uint32_t                 m_MaxGames;                   // config value: maximum number of games in progress
  uint32_t                 m_HostCounter;                // the current host counter (a unique number to identify a game, incremented each time a game is created)
  uint32_t                 m_AllowDownloads;             // config value: allow map downloads or not
  uint32_t                 m_MaxDownloaders;             // config value: maximum number of map downloaders at the same time
  uint32_t                 m_MaxDownloadSpeed;           // config value: maximum total map download speed in KB/sec
  uint32_t                 m_AutoKickPing;               // config value: auto kick players with ping higher than this
  uint32_t                 m_LobbyTimeLimit;             // config value: auto close the game lobby after this many minutes without any reserved players
  uint32_t                 m_Latency;                    // config value: the latency (by default)
  uint32_t                 m_SyncLimit;                  // config value: the maximum number of packets a player can fall out of sync before starting the lag screen (by default)
  uint32_t                 m_VoteKickPercentage;         // config value: percentage of players required to vote yes for a votekick to pass
  uint32_t                 m_NumPlayersToStartGameOver;  // config value: when this player count is reached, the game over timer will start
  uint16_t                 m_HostPort;                   // config value: the port to host games on
  uint16_t                 m_ReconnectPort;              // config value: the port to listen for GProxy++ reliable reconnects on
  uint8_t                  m_LANWar3Version;             // config value: LAN warcraft 3 version
  int32_t                  m_CommandTrigger;             // config value: the command trigger inside games
  bool                     m_Exiting;                    // set to true to force aura to shutdown next update (used by SignalCatcher)
  bool                     m_Enabled;                    // set to false to prevent new games from being created
  bool                     m_AutoLock;                   // config value: auto lock games when the owner is present
  bool                     m_Ready;                      // indicates if there's lacking configuration info so we can quit
  bool                     m_LCPings;                    // config value: use LC style pings (divide actual pings by two)

  explicit CAura(CConfig* CFG);
  ~CAura();
  CAura(CAura&) = delete;

  // processing functions

  bool Update();

  // events

  void EventBNETGameRefreshFailed(CBNET* bnet);
  void EventGameDeleted(CGame* game);

  // other functions

  void ReloadConfigs();
  void SetConfigs(CConfig* CFG);
  void ExtractScripts(const uint8_t War3Version);
  void LoadIPToCountryData();
  void CreateGame(CMap* map, uint8_t gameState, std::string gameName, std::string ownerName, std::string creatorName, CBNET* nCreatorServer, bool whisper);

  inline bool GetReady() const
  {
    return m_Ready;
  }
};

#endif // AURA_AURA_H_
