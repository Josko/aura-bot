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

#ifndef MAP_H
#define MAP_H

#define MAPSPEED_SLOW                     1
#define MAPSPEED_NORMAL                   2
#define MAPSPEED_FAST                     3

#define MAPVIS_HIDETERRAIN                1
#define MAPVIS_EXPLORED                   2
#define MAPVIS_ALWAYSVISIBLE              3
#define MAPVIS_DEFAULT                    4

#define MAPOBS_NONE                       1
#define MAPOBS_ONDEFEAT                   2
#define MAPOBS_ALLOWED                    3
#define MAPOBS_REFEREES                   4

#define MAPFLAG_TEAMSTOGETHER             1
#define MAPFLAG_FIXEDTEAMS                2
#define MAPFLAG_UNITSHARE                 4
#define MAPFLAG_RANDOMHERO                8
#define MAPFLAG_RANDOMRACES               16

#define MAPOPT_HIDEMINIMAP                1 << 0
#define MAPOPT_MODIFYALLYPRIORITIES       1 << 1
#define MAPOPT_MELEE                      1 << 2  // the bot cares about this one...
#define MAPOPT_REVEALTERRAIN              1 << 4
#define MAPOPT_FIXEDPLAYERSETTINGS        1 << 5  // and this one...
#define MAPOPT_CUSTOMFORCES               1 << 6  // and this one, the rest don't affect the bot's logic
#define MAPOPT_CUSTOMTECHTREE             1 << 7
#define MAPOPT_CUSTOMABILITIES            1 << 8
#define MAPOPT_CUSTOMUPGRADES             1 << 9
#define MAPOPT_WATERWAVESONCLIFFSHORES    1 << 11
#define MAPOPT_WATERWAVESONSLOPESHORES    1 << 12

#define MAPFILTER_MAKER_USER              1
#define MAPFILTER_MAKER_BLIZZARD          2

#define MAPFILTER_TYPE_MELEE              1
#define MAPFILTER_TYPE_SCENARIO           2

#define MAPFILTER_SIZE_SMALL              1
#define MAPFILTER_SIZE_MEDIUM             2
#define MAPFILTER_SIZE_LARGE              4

#define MAPFILTER_OBS_FULL                1
#define MAPFILTER_OBS_ONDEATH             2
#define MAPFILTER_OBS_NONE                4

#define MAPGAMETYPE_UNKNOWN0              1       // always set except for saved games?
#define MAPGAMETYPE_BLIZZARD              1 << 3
#define MAPGAMETYPE_MELEE                 1 << 5
#define MAPGAMETYPE_SAVEDGAME             1 << 9
#define MAPGAMETYPE_PRIVATEGAME           1 << 11
#define MAPGAMETYPE_MAKERUSER             1 << 13
#define MAPGAMETYPE_MAKERBLIZZARD         1 << 14
#define MAPGAMETYPE_TYPEMELEE             1 << 15
#define MAPGAMETYPE_TYPESCENARIO          1 << 16
#define MAPGAMETYPE_SIZESMALL             1 << 17
#define MAPGAMETYPE_SIZEMEDIUM            1 << 18
#define MAPGAMETYPE_SIZELARGE             1 << 19
#define MAPGAMETYPE_OBSFULL               1 << 20
#define MAPGAMETYPE_OBSONDEATH            1 << 21
#define MAPGAMETYPE_OBSNONE               1 << 22

#include "gameslot.h"

//
// CMap
//

class CMap
{
public:
  CAura *m_Aura;

private:
  bool m_Valid;
  string m_CFGFile;
  string m_MapPath;               // config value: map path
  BYTEARRAY m_MapSize;            // config value: map size (4 bytes)
  BYTEARRAY m_MapInfo;            // config value: map info (4 bytes) -> this is the real CRC
  BYTEARRAY m_MapCRC;             // config value: map crc (4 bytes) -> this is not the real CRC, it's the "xoro" value
  BYTEARRAY m_MapSHA1;            // config value: map sha1 (20 bytes)
  unsigned char m_MapSpeed;
  unsigned char m_MapVisibility;
  unsigned char m_MapObservers;
  unsigned char m_MapFlags;
  unsigned char m_MapFilterMaker;
  unsigned char m_MapFilterType;
  unsigned char m_MapFilterSize;
  unsigned char m_MapFilterObs;
  uint32_t m_MapOptions;
  BYTEARRAY m_MapWidth;           // config value: map width (2 bytes)
  BYTEARRAY m_MapHeight;          // config value: map height (2 bytes)
  string m_MapType;               // config value: map type (for stats class)
  string m_MapDefaultHCL;         // config value: map default HCL to use (this should really be specified elsewhere and not part of the map config)
  string m_MapLocalPath;          // config value: map local path
  string m_MapData;               // the map data itself, for sending the map to players
  uint32_t m_MapNumPlayers;       // config value: max map number of players
  uint32_t m_MapNumTeams;         // config value: max map number of teams
  vector<CGameSlot> m_Slots;

public:
  CMap( CAura *nAura, CConfig *CFG, const string &nCFGFile );
  ~CMap( );

  inline bool GetValid( ) const                               { return m_Valid; }
  inline string GetCFGFile( ) const                           { return m_CFGFile; }
  inline string GetMapPath( ) const                           { return m_MapPath; }
  inline BYTEARRAY GetMapSize( ) const                        { return m_MapSize; }
  inline BYTEARRAY GetMapInfo( ) const                        { return m_MapInfo; }
  inline BYTEARRAY GetMapCRC( ) const                         { return m_MapCRC; }
  inline BYTEARRAY GetMapSHA1( ) const                        { return m_MapSHA1; }
  inline unsigned char GetMapSpeed( ) const                   { return m_MapSpeed; }
  inline unsigned char GetMapVisibility( ) const              { return m_MapVisibility; }
  inline unsigned char GetMapObservers( ) const               { return m_MapObservers; }
  inline unsigned char GetMapFlags( ) const                   { return m_MapFlags; }
  BYTEARRAY GetMapGameFlags( ) const;
  uint32_t GetMapGameType( ) const;
  inline uint32_t GetMapOptions( ) const                      { return m_MapOptions; }
  unsigned char GetMapLayoutStyle( ) const;
  inline BYTEARRAY GetMapWidth( ) const                       { return m_MapWidth; }
  inline BYTEARRAY GetMapHeight( ) const                      { return m_MapHeight; }
  inline string GetMapType( ) const                           { return m_MapType; }
  inline string GetMapDefaultHCL( ) const                     { return m_MapDefaultHCL; }
  inline string GetMapLocalPath( ) const                      { return m_MapLocalPath; }
  inline string *GetMapData( )                                { return &m_MapData; }
  inline uint32_t GetMapNumPlayers( ) const                   { return m_MapNumPlayers; }
  inline uint32_t GetMapNumTeams( ) const                     { return m_MapNumTeams; }
  inline vector<CGameSlot> GetSlots( ) const                  { return m_Slots; }

  void Load( CConfig *CFG, const string &nCFGFile );
  void CheckValid( );
  uint32_t XORRotateLeft( unsigned char *data, uint32_t length );
};

#endif
