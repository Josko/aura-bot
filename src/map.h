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

#ifndef AURA_MAP_H_
#define AURA_MAP_H_

#define MAPSPEED_SLOW 1
#define MAPSPEED_NORMAL 2
#define MAPSPEED_FAST 3

#define MAPVIS_HIDETERRAIN 1
#define MAPVIS_EXPLORED 2
#define MAPVIS_ALWAYSVISIBLE 3
#define MAPVIS_DEFAULT 4

#define MAPOBS_NONE 1
#define MAPOBS_ONDEFEAT 2
#define MAPOBS_ALLOWED 3
#define MAPOBS_REFEREES 4

#define MAPFLAG_TEAMSTOGETHER 1
#define MAPFLAG_FIXEDTEAMS 2
#define MAPFLAG_UNITSHARE 4
#define MAPFLAG_RANDOMHERO 8
#define MAPFLAG_RANDOMRACES 16

#define MAPOPT_HIDEMINIMAP 1 << 0
#define MAPOPT_MODIFYALLYPRIORITIES 1 << 1
#define MAPOPT_MELEE 1 << 2 // the bot cares about this one...
#define MAPOPT_REVEALTERRAIN 1 << 4
#define MAPOPT_FIXEDPLAYERSETTINGS 1 << 5 // and this one...
#define MAPOPT_CUSTOMFORCES 1 << 6        // and this one, the rest don't affect the bot's logic
#define MAPOPT_CUSTOMTECHTREE 1 << 7
#define MAPOPT_CUSTOMABILITIES 1 << 8
#define MAPOPT_CUSTOMUPGRADES 1 << 9
#define MAPOPT_WATERWAVESONCLIFFSHORES 1 << 11
#define MAPOPT_WATERWAVESONSLOPESHORES 1 << 12

#define MAPFILTER_MAKER_USER 1
#define MAPFILTER_MAKER_BLIZZARD 2

#define MAPFILTER_TYPE_MELEE 1
#define MAPFILTER_TYPE_SCENARIO 2

#define MAPFILTER_SIZE_SMALL 1
#define MAPFILTER_SIZE_MEDIUM 2
#define MAPFILTER_SIZE_LARGE 4

#define MAPFILTER_OBS_FULL 1
#define MAPFILTER_OBS_ONDEATH 2
#define MAPFILTER_OBS_NONE 4

#define MAPGAMETYPE_UNKNOWN0 1 // always set except for saved games?
#define MAPGAMETYPE_BLIZZARD 1 << 3
#define MAPGAMETYPE_MELEE 1 << 5
#define MAPGAMETYPE_SAVEDGAME 1 << 9
#define MAPGAMETYPE_PRIVATEGAME 1 << 11
#define MAPGAMETYPE_MAKERUSER 1 << 13
#define MAPGAMETYPE_MAKERBLIZZARD 1 << 14
#define MAPGAMETYPE_TYPEMELEE 1 << 15
#define MAPGAMETYPE_TYPESCENARIO 1 << 16
#define MAPGAMETYPE_SIZESMALL 1 << 17
#define MAPGAMETYPE_SIZEMEDIUM 1 << 18
#define MAPGAMETYPE_SIZELARGE 1 << 19
#define MAPGAMETYPE_OBSFULL 1 << 20
#define MAPGAMETYPE_OBSONDEATH 1 << 21
#define MAPGAMETYPE_OBSNONE 1 << 22

//
// CMap
//

#include <vector>
#include <string>
#include <cstdint>

class CAura;
class CGameSlot;
class CConfig;

class CMap
{
public:
  CAura* m_Aura;

private:
  std::vector<uint8_t>   m_MapSHA1;   // config value: map sha1 (20 bytes)
  std::vector<uint8_t>   m_MapSize;   // config value: map size (4 bytes)
  std::vector<uint8_t>   m_MapInfo;   // config value: map info (4 bytes) -> this is the real CRC
  std::vector<uint8_t>   m_MapCRC;    // config value: map crc (4 bytes) -> this is not the real CRC, it's the "xoro" value
  std::vector<uint8_t>   m_MapWidth;  // config value: map width (2 bytes)
  std::vector<uint8_t>   m_MapHeight; // config value: map height (2 bytes)
  std::vector<CGameSlot> m_Slots;
  std::string            m_CFGFile;
  std::string            m_MapPath;       // config value: map path
  std::string            m_MapType;       // config value: map type (for stats class)
  std::string            m_MapDefaultHCL; // config value: map default HCL to use (this should really be specified elsewhere and not part of the map config)
  std::string            m_MapLocalPath;  // config value: map local path
  std::string            m_MapData;       // the map data itself, for sending the map to players
  uint32_t               m_MapOptions;
  uint32_t               m_MapNumPlayers; // config value: max map number of players
  uint32_t               m_MapNumTeams;   // config value: max map number of teams
  uint8_t                m_MapSpeed;
  uint8_t                m_MapVisibility;
  uint8_t                m_MapObservers;
  uint8_t                m_MapFlags;
  uint8_t                m_MapFilterMaker;
  uint8_t                m_MapFilterType;
  uint8_t                m_MapFilterSize;
  uint8_t                m_MapFilterObs;
  bool                   m_Valid;

public:
  CMap(CAura* nAura, CConfig* CFG, const std::string& nCFGFile);
  ~CMap();

  inline bool                   GetValid() const { return m_Valid; }
  inline std::string            GetCFGFile() const { return m_CFGFile; }
  inline std::string            GetMapPath() const { return m_MapPath; }
  inline std::vector<uint8_t>   GetMapSize() const { return m_MapSize; }
  inline std::vector<uint8_t>   GetMapInfo() const { return m_MapInfo; }
  inline std::vector<uint8_t>   GetMapCRC() const { return m_MapCRC; }
  inline std::vector<uint8_t>   GetMapSHA1() const { return m_MapSHA1; }
  inline uint8_t                GetMapSpeed() const { return m_MapSpeed; }
  inline uint8_t                GetMapVisibility() const { return m_MapVisibility; }
  inline uint8_t                GetMapObservers() const { return m_MapObservers; }
  inline uint8_t                GetMapFlags() const { return m_MapFlags; }
  std::vector<uint8_t>          GetMapGameFlags() const;
  uint32_t                      GetMapGameType() const;
  inline uint32_t               GetMapOptions() const { return m_MapOptions; }
  uint8_t                       GetMapLayoutStyle() const;
  inline std::vector<uint8_t>   GetMapWidth() const { return m_MapWidth; }
  inline std::vector<uint8_t>   GetMapHeight() const { return m_MapHeight; }
  inline std::string            GetMapType() const { return m_MapType; }
  inline std::string            GetMapDefaultHCL() const { return m_MapDefaultHCL; }
  inline std::string            GetMapLocalPath() const { return m_MapLocalPath; }
  inline std::string*           GetMapData() { return &m_MapData; }
  inline uint32_t               GetMapNumPlayers() const { return m_MapNumPlayers; }
  inline uint32_t               GetMapNumTeams() const { return m_MapNumTeams; }
  inline std::vector<CGameSlot> GetSlots() const { return m_Slots; }

  void Load(CConfig* CFG, const std::string& nCFGFile);
  const char* CheckValid();
  uint32_t XORRotateLeft(uint8_t* data, uint32_t length);
};

#endif // AURA_MAP_H_
