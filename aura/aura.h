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

#ifndef AURA_H
#define AURA_H

#include "includes.h"

//
// CAura
//

class CUDPSocket;
class CTCPServer;
class CTCPSocket;
class CCRC32;
class CSHA1;
class CBNET;
class CGame;
class CAuraDB;
class CLanguage;
class CMap;
class CConfig;
class CIRC;

class CAura
{
public:
  CIRC *m_IRC;
  CUDPSocket *m_UDPSocket;                  // a UDP socket for sending broadcasts and other junk (used with !sendlan)
  CCRC32 *m_CRC;                            // for calculating CRC's
  CSHA1 *m_SHA;                             // for calculating SHA1's
  vector<CBNET *> m_BNETs;                  // all our battle.net connections (there can be more than one)
  CGame *m_CurrentGame;                     // this game is still in the lobby state
  vector<CGame *> m_Games;                  // these games are in progress
  CAuraDB *m_DB;                            // database
  CLanguage *m_Language;                    // language
  CMap *m_Map;                              // the currently loaded map
  bool m_Exiting;                           // set to true to force aura to shutdown next update (used by SignalCatcher)
  bool m_Enabled;                           // set to false to prevent new games from being created
  string m_Version;                         // Aura++ version string
  uint32_t m_HostCounter;                   // the current host counter (a unique number to identify a game, incremented each time a game is created)
  bool m_Ready;                             // indicates if there's lacking configuration info so we can quit
  string m_LanguageFile;                    // config value: language file
  string m_Warcraft3Path;                   // config value: Warcraft 3 path
  string m_BindAddress;                     // config value: the address to host games on
  uint16_t m_HostPort;                      // config value: the port to host games on
  uint32_t m_MaxGames;                      // config value: maximum number of games in progress
  char m_CommandTrigger;                    // config value: the command trigger inside games
  string m_MapCFGPath;                      // config value: map cfg path
  string m_MapPath;                         // config value: map path
  string m_VirtualHostName;                 // config value: virtual host name
  bool m_AutoLock;                          // config value: auto lock games when the owner is present
  uint32_t m_AllowDownloads;                // config value: allow map downloads or not
  uint32_t m_MaxDownloaders;                // config value: maximum number of map downloaders at the same time
  uint32_t m_MaxDownloadSpeed;              // config value: maximum total map download speed in KB/sec
  bool m_LCPings;                           // config value: use LC style pings (divide actual pings by two)
  uint32_t m_AutoKickPing;                  // config value: auto kick players with ping higher than this
  uint32_t m_LobbyTimeLimit;                // config value: auto close the game lobby after this many minutes without any reserved players
  uint32_t m_Latency;                       // config value: the latency (by default)
  uint32_t m_SyncLimit;                     // config value: the maximum number of packets a player can fall out of sync before starting the lag screen (by default)
  uint32_t m_VoteKickPercentage;            // config value: percentage of players required to vote yes for a votekick to pass
  string m_DefaultMap;                      // config value: default map (map.cfg)
  unsigned char m_LANWar3Version;           // config value: LAN warcraft 3 version

  CAura( CConfig *CFG );
  ~CAura( );

  // processing functions

  bool Update( );

  // events

  void EventBNETGameRefreshFailed( CBNET *bnet );
  void EventGameDeleted( CGame *game );

  // other functions

  void ReloadConfigs( );
  void SetConfigs( CConfig *CFG );
  void ExtractScripts( );
  void LoadIPToCountryData( );
  void CreateGame( CMap *map, unsigned char gameState, string gameName, string ownerName, string creatorName, string creatorServer, bool whisper );
  
  bool GetReady( )
  {
    return m_Ready;
  }
};

#endif
