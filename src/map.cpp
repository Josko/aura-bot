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

#include "map.h"
#include "aura.h"
#include "util.h"
#include "fileutil.h"
#include "crc32.h"
#include "sha1.h"
#include "config.h"
#include "gameslot.h"

#define __STORMLIB_SELF__
#include <StormLib.h>

#define ROTL(x, n) ((x) << (n)) | ((x) >> (32 - (n))) // this won't work with signed types
#define ROTR(x, n) ((x) >> (n)) | ((x) << (32 - (n))) // this won't work with signed types

using namespace std;

//
// CMap
//

CMap::CMap(CAura* nAura, CConfig* CFG, const string& nCFGFile)
  : m_Aura(nAura)
{
  Load(CFG, nCFGFile);
}

CMap::~CMap() = default;

std::vector<uint8_t> CMap::GetMapGameFlags() const
{
  uint32_t GameFlags = 0;

  // speed

  if (m_MapSpeed == MAPSPEED_SLOW)
    GameFlags = 0x00000000;
  else if (m_MapSpeed == MAPSPEED_NORMAL)
    GameFlags = 0x00000001;
  else
    GameFlags = 0x00000002;

  // visibility

  if (m_MapVisibility == MAPVIS_HIDETERRAIN)
    GameFlags |= 0x00000100;
  else if (m_MapVisibility == MAPVIS_EXPLORED)
    GameFlags |= 0x00000200;
  else if (m_MapVisibility == MAPVIS_ALWAYSVISIBLE)
    GameFlags |= 0x00000400;
  else
    GameFlags |= 0x00000800;

  // observers

  if (m_MapObservers == MAPOBS_ONDEFEAT)
    GameFlags |= 0x00002000;
  else if (m_MapObservers == MAPOBS_ALLOWED)
    GameFlags |= 0x00003000;
  else if (m_MapObservers == MAPOBS_REFEREES)
    GameFlags |= 0x40000000;

  // teams/units/hero/race

  if (m_MapFlags & MAPFLAG_TEAMSTOGETHER)
    GameFlags |= 0x00004000;

  if (m_MapFlags & MAPFLAG_FIXEDTEAMS)
    GameFlags |= 0x00060000;

  if (m_MapFlags & MAPFLAG_UNITSHARE)
    GameFlags |= 0x01000000;

  if (m_MapFlags & MAPFLAG_RANDOMHERO)
    GameFlags |= 0x02000000;

  if (m_MapFlags & MAPFLAG_RANDOMRACES)
    GameFlags |= 0x04000000;

  return CreateByteArray(GameFlags, false);
}

uint32_t CMap::GetMapGameType() const
{
  /* spec stolen from Strilanc as follows:

    Public Enum GameTypes As UInteger
        None = 0
        Unknown0 = 1 << 0 '[always seems to be set?]

        '''<summary>Setting this bit causes wc3 to check the map and disc if it is not signed by Blizzard</summary>
        AuthenticatedMakerBlizzard = 1 << 3
        OfficialMeleeGame = 1 << 5

    SavedGame = 1 << 9
        PrivateGame = 1 << 11

        MakerUser = 1 << 13
        MakerBlizzard = 1 << 14
        TypeMelee = 1 << 15
        TypeScenario = 1 << 16
        SizeSmall = 1 << 17
        SizeMedium = 1 << 18
        SizeLarge = 1 << 19
        ObsFull = 1 << 20
        ObsOnDeath = 1 << 21
        ObsNone = 1 << 22

        MaskObs = ObsFull Or ObsOnDeath Or ObsNone
        MaskMaker = MakerBlizzard Or MakerUser
        MaskType = TypeMelee Or TypeScenario
        MaskSize = SizeLarge Or SizeMedium Or SizeSmall
        MaskFilterable = MaskObs Or MaskMaker Or MaskType Or MaskSize
    End Enum

   */

  // note: we allow "conflicting" flags to be set at the same time (who knows if this is a good idea)
  // we also don't set any flags this class is unaware of such as Unknown0, SavedGame, and PrivateGame

  uint32_t GameType = 0;

  // maker

  if (m_MapFilterMaker & MAPFILTER_MAKER_USER)
    GameType |= MAPGAMETYPE_MAKERUSER;

  if (m_MapFilterMaker & MAPFILTER_MAKER_BLIZZARD)
    GameType |= MAPGAMETYPE_MAKERBLIZZARD;

  // type

  if (m_MapFilterType & MAPFILTER_TYPE_MELEE)
    GameType |= MAPGAMETYPE_TYPEMELEE;

  if (m_MapFilterType & MAPFILTER_TYPE_SCENARIO)
    GameType |= MAPGAMETYPE_TYPESCENARIO;

  // size

  if (m_MapFilterSize & MAPFILTER_SIZE_SMALL)
    GameType |= MAPGAMETYPE_SIZESMALL;

  if (m_MapFilterSize & MAPFILTER_SIZE_MEDIUM)
    GameType |= MAPGAMETYPE_SIZEMEDIUM;

  if (m_MapFilterSize & MAPFILTER_SIZE_LARGE)
    GameType |= MAPGAMETYPE_SIZELARGE;

  // obs

  if (m_MapFilterObs & MAPFILTER_OBS_FULL)
    GameType |= MAPGAMETYPE_OBSFULL;

  if (m_MapFilterObs & MAPFILTER_OBS_ONDEATH)
    GameType |= MAPGAMETYPE_OBSONDEATH;

  if (m_MapFilterObs & MAPFILTER_OBS_NONE)
    GameType |= MAPGAMETYPE_OBSNONE;

  return GameType;
}

uint8_t CMap::GetMapLayoutStyle() const
{
  // 0 = melee
  // 1 = custom forces
  // 2 = fixed player settings (not possible with the Warcraft III map editor)
  // 3 = custom forces + fixed player settings

  if (!(m_MapOptions & MAPOPT_CUSTOMFORCES))
    return 0;

  if (!(m_MapOptions & MAPOPT_FIXEDPLAYERSETTINGS))
    return 1;

  return 3;
}

void CMap::Load(CConfig* CFG, const string& nCFGFile)
{
  m_Valid   = true;
  m_CFGFile = nCFGFile;

  // load the map data

  m_MapLocalPath = CFG->GetString("map_localpath", string());
  m_MapData.clear();

  if (!m_MapLocalPath.empty())
    m_MapData = FileRead(m_Aura->m_MapPath + m_MapLocalPath);

  // load the map MPQ

  string MapMPQFileName = m_Aura->m_MapPath + m_MapLocalPath;
  HANDLE MapMPQ;
  bool   MapMPQReady = false;

#ifdef WIN32
  const wstring MapMPQFileNameW = wstring(begin(MapMPQFileName), end(MapMPQFileName));

  if (SFileOpenArchive(MapMPQFileNameW.c_str(), 0, MPQ_OPEN_FORCE_MPQ_V1, &MapMPQ))
#else
  if (SFileOpenArchive(MapMPQFileName.c_str(), 0, MPQ_OPEN_FORCE_MPQ_V1, &MapMPQ))
#endif
  {
    Print("[MAP] loading MPQ file [" + MapMPQFileName + "]");
    MapMPQReady = true;
  }
  else
    Print("[MAP] warning - unable to load MPQ file [" + MapMPQFileName + "]");

  // try to calculate map_size, map_info, map_crc, map_sha1

  std::vector<uint8_t> MapSize, MapInfo, MapCRC, MapSHA1;

  if (!m_MapData.empty())
  {
    m_Aura->m_SHA->Reset();

    // calculate map_size

    MapSize = CreateByteArray((uint32_t)m_MapData.size(), false);
    Print("[MAP] calculated map_size = " + ByteArrayToDecString(MapSize));

    // calculate map_info (this is actually the CRC)

    MapInfo = CreateByteArray((uint32_t)m_Aura->m_CRC->CalculateCRC((uint8_t*)m_MapData.c_str(), m_MapData.size()), false);
    Print("[MAP] calculated map_info = " + ByteArrayToDecString(MapInfo));

    // calculate map_crc (this is not the CRC) and map_sha1
    // a big thank you to Strilanc for figuring the map_crc algorithm out

    string CommonJ = FileRead(m_Aura->m_MapCFGPath + "common.j");

    if (CommonJ.empty())
      Print("[MAP] unable to calculate map_crc/sha1 - unable to read file [" + m_Aura->m_MapCFGPath + "common.j]");
    else
    {
      string BlizzardJ = FileRead(m_Aura->m_MapCFGPath + "blizzard.j");

      if (BlizzardJ.empty())
        Print("[MAP] unable to calculate map_crc/sha1 - unable to read file [" + m_Aura->m_MapCFGPath + "blizzard.j]");
      else
      {
        uint32_t Val = 0;

        // update: it's possible for maps to include their own copies of common.j and/or blizzard.j
        // this code now overrides the default copies if required

        bool OverrodeCommonJ   = false;
        bool OverrodeBlizzardJ = false;

        if (MapMPQReady)
        {
          HANDLE SubFile;

          // override common.j

          if (SFileOpenFileEx(MapMPQ, R"(Scripts\common.j)", 0, &SubFile))
          {
            uint32_t FileLength = SFileGetFileSize(SubFile, nullptr);

            if (FileLength > 0 && FileLength != 0xFFFFFFFF)
            {
              auto  SubFileData = new char[FileLength];
              DWORD BytesRead   = 0;

              if (SFileReadFile(SubFile, SubFileData, FileLength, &BytesRead, nullptr))
              {
                Print("[MAP] overriding default common.j with map copy while calculating map_crc/sha1");
                OverrodeCommonJ = true;
                Val             = Val ^ XORRotateLeft((uint8_t*)SubFileData, BytesRead);
                m_Aura->m_SHA->Update((uint8_t*)SubFileData, BytesRead);
              }

              delete[] SubFileData;
            }

            SFileCloseFile(SubFile);
          }
        }

        if (!OverrodeCommonJ)
        {
          Val = Val ^ XORRotateLeft((uint8_t*)CommonJ.c_str(), CommonJ.size());
          m_Aura->m_SHA->Update((uint8_t*)CommonJ.c_str(), CommonJ.size());
        }

        if (MapMPQReady)
        {
          HANDLE SubFile;

          // override blizzard.j

          if (SFileOpenFileEx(MapMPQ, R"(Scripts\blizzard.j)", 0, &SubFile))
          {
            uint32_t FileLength = SFileGetFileSize(SubFile, nullptr);

            if (FileLength > 0 && FileLength != 0xFFFFFFFF)
            {
              auto  SubFileData = new char[FileLength];
              DWORD BytesRead   = 0;

              if (SFileReadFile(SubFile, SubFileData, FileLength, &BytesRead, nullptr))
              {
                Print("[MAP] overriding default blizzard.j with map copy while calculating map_crc/sha1");
                OverrodeBlizzardJ = true;
                Val               = Val ^ XORRotateLeft((uint8_t*)SubFileData, BytesRead);
                m_Aura->m_SHA->Update((uint8_t*)SubFileData, BytesRead);
              }

              delete[] SubFileData;
            }

            SFileCloseFile(SubFile);
          }
        }

        if (!OverrodeBlizzardJ)
        {
          Val = Val ^ XORRotateLeft((uint8_t*)BlizzardJ.c_str(), BlizzardJ.size());
          m_Aura->m_SHA->Update((uint8_t*)BlizzardJ.c_str(), BlizzardJ.size());
        }

        Val = ROTL(Val, 3);
        Val = ROTL(Val ^ 0x03F1379E, 3);
        m_Aura->m_SHA->Update((uint8_t*)"\x9E\x37\xF1\x03", 4);

        if (MapMPQReady)
        {
          vector<string> FileList;
          FileList.emplace_back("war3map.j");
          FileList.emplace_back(R"(scripts\war3map.j)");
          FileList.emplace_back("war3map.w3e");
          FileList.emplace_back("war3map.wpm");
          FileList.emplace_back("war3map.doo");
          FileList.emplace_back("war3map.w3u");
          FileList.emplace_back("war3map.w3b");
          FileList.emplace_back("war3map.w3d");
          FileList.emplace_back("war3map.w3a");
          FileList.emplace_back("war3map.w3q");
          bool FoundScript = false;

          for (auto& fileName : FileList)
          {
            // don't use scripts\war3map.j if we've already used war3map.j (yes, some maps have both but only war3map.j is used)

            if (FoundScript && fileName == R"(scripts\war3map.j)")
              continue;

            HANDLE SubFile;

            if (SFileOpenFileEx(MapMPQ, fileName.c_str(), 0, &SubFile))
            {
              uint32_t FileLength = SFileGetFileSize(SubFile, nullptr);

              if (FileLength > 0 && FileLength != 0xFFFFFFFF)
              {
                auto  SubFileData = new char[FileLength];
                DWORD BytesRead   = 0;

                if (SFileReadFile(SubFile, SubFileData, FileLength, &BytesRead, nullptr))
                {
                  if (fileName == "war3map.j" || fileName == R"(scripts\war3map.j)")
                    FoundScript = true;

                  Val = ROTL(Val ^ XORRotateLeft((uint8_t*)SubFileData, BytesRead), 3);
                  m_Aura->m_SHA->Update((uint8_t*)SubFileData, BytesRead);
                }

                delete[] SubFileData;
              }

              SFileCloseFile(SubFile);
            }
          }

          if (!FoundScript)
            Print(R"([MAP] couldn't find war3map.j or scripts\war3map.j in MPQ file, calculated map_crc/sha1 is probably wrong)");

          MapCRC = CreateByteArray(Val, false);
          Print("[MAP] calculated map_crc = " + ByteArrayToDecString(MapCRC));

          m_Aura->m_SHA->Final();
          uint8_t SHA1[20];
          memset(SHA1, 0, sizeof(uint8_t) * 20);
          m_Aura->m_SHA->GetHash(SHA1);
          MapSHA1 = CreateByteArray(SHA1, 20);
          Print("[MAP] calculated map_sha1 = " + ByteArrayToDecString(MapSHA1));
        }
        else
          Print("[MAP] unable to calculate map_crc/sha1 - map MPQ file not loaded");
      }
    }
  }
  else
    Print("[MAP] no map data available, using config file for map_size, map_info, map_crc, map_sha1");

  // try to calculate map_width, map_height, map_slot<x>, map_numplayers, map_numteams, map_filtertype

  std::vector<uint8_t> MapWidth;
  std::vector<uint8_t> MapHeight;
  uint32_t             MapOptions    = 0;
  uint32_t             MapNumPlayers = 0;
  uint32_t             MapFilterType = MAPFILTER_TYPE_SCENARIO;
  uint32_t             MapNumTeams   = 0;
  vector<CGameSlot>    Slots;

  if (!m_MapData.empty())
  {
    if (MapMPQReady)
    {
      HANDLE SubFile;

      if (SFileOpenFileEx(MapMPQ, "war3map.w3i", 0, &SubFile))
      {
        uint32_t FileLength = SFileGetFileSize(SubFile, nullptr);

        if (FileLength > 0 && FileLength != 0xFFFFFFFF)
        {
          auto  SubFileData = new char[FileLength];
          DWORD BytesRead   = 0;

          if (SFileReadFile(SubFile, SubFileData, FileLength, &BytesRead, nullptr))
          {
            istringstream ISS(string(SubFileData, BytesRead));

            // war3map.w3i format found at http://www.wc3campaigns.net/tools/specs/index.html by Zepir/PitzerMike

            string   GarbageString;
            uint32_t FileFormat;
            uint32_t RawMapWidth;
            uint32_t RawMapHeight;
            uint32_t RawMapFlags;
            uint32_t RawMapNumPlayers;
            uint32_t RawMapNumTeams;

            ISS.read((char*)&FileFormat, 4); // file format (18 = ROC, 25 = TFT)

            if (FileFormat == 18 || FileFormat == 25)
            {
              ISS.seekg(4, ios::cur);            // number of saves
              ISS.seekg(4, ios::cur);            // editor version
              getline(ISS, GarbageString, '\0'); // map name
              getline(ISS, GarbageString, '\0'); // map author
              getline(ISS, GarbageString, '\0'); // map description
              getline(ISS, GarbageString, '\0'); // players recommended
              ISS.seekg(32, ios::cur);           // camera bounds
              ISS.seekg(16, ios::cur);           // camera bounds complements
              ISS.read((char*)&RawMapWidth, 4);  // map width
              ISS.read((char*)&RawMapHeight, 4); // map height
              ISS.read((char*)&RawMapFlags, 4);  // flags
              ISS.seekg(1, ios::cur);            // map main ground type

              if (FileFormat == 18)
                ISS.seekg(4, ios::cur); // campaign background number
              else if (FileFormat == 25)
              {
                ISS.seekg(4, ios::cur);            // loading screen background number
                getline(ISS, GarbageString, '\0'); // path of custom loading screen model
              }

              getline(ISS, GarbageString, '\0'); // map loading screen text
              getline(ISS, GarbageString, '\0'); // map loading screen title
              getline(ISS, GarbageString, '\0'); // map loading screen subtitle

              if (FileFormat == 18)
                ISS.seekg(4, ios::cur); // map loading screen number
              else if (FileFormat == 25)
              {
                ISS.seekg(4, ios::cur);            // used game data set
                getline(ISS, GarbageString, '\0'); // prologue screen path
              }

              getline(ISS, GarbageString, '\0'); // prologue screen text
              getline(ISS, GarbageString, '\0'); // prologue screen title
              getline(ISS, GarbageString, '\0'); // prologue screen subtitle

              if (FileFormat == 25)
              {
                ISS.seekg(4, ios::cur);            // uses terrain fog
                ISS.seekg(4, ios::cur);            // fog start z height
                ISS.seekg(4, ios::cur);            // fog end z height
                ISS.seekg(4, ios::cur);            // fog density
                ISS.seekg(1, ios::cur);            // fog red value
                ISS.seekg(1, ios::cur);            // fog green value
                ISS.seekg(1, ios::cur);            // fog blue value
                ISS.seekg(1, ios::cur);            // fog alpha value
                ISS.seekg(4, ios::cur);            // global weather id
                getline(ISS, GarbageString, '\0'); // custom sound environment
                ISS.seekg(1, ios::cur);            // tileset id of the used custom light environment
                ISS.seekg(1, ios::cur);            // custom water tinting red value
                ISS.seekg(1, ios::cur);            // custom water tinting green value
                ISS.seekg(1, ios::cur);            // custom water tinting blue value
                ISS.seekg(1, ios::cur);            // custom water tinting alpha value
              }

              ISS.read((char*)&RawMapNumPlayers, 4); // number of players
              uint32_t ClosedSlots = 0;

              for (uint32_t i = 0; i < RawMapNumPlayers; ++i)
              {
                CGameSlot Slot(0, 255, SLOTSTATUS_OPEN, 0, 0, 1, SLOTRACE_RANDOM);
                uint32_t  Colour;
                uint32_t  Status;
                uint32_t  Race;

                ISS.read((char*)&Colour, 4); // colour
                Slot.SetColour(Colour);
                ISS.read((char*)&Status, 4); // status

                if (Status == 1)
                  Slot.SetSlotStatus(SLOTSTATUS_OPEN);
                else if (Status == 2)
                {
                  Slot.SetSlotStatus(SLOTSTATUS_OCCUPIED);
                  Slot.SetComputer(1);
                  Slot.SetComputerType(SLOTCOMP_NORMAL);
                }
                else
                {
                  Slot.SetSlotStatus(SLOTSTATUS_CLOSED);
                  ++ClosedSlots;
                }

                ISS.read((char*)&Race, 4); // race

                if (Race == 1)
                  Slot.SetRace(SLOTRACE_HUMAN);
                else if (Race == 2)
                  Slot.SetRace(SLOTRACE_ORC);
                else if (Race == 3)
                  Slot.SetRace(SLOTRACE_UNDEAD);
                else if (Race == 4)
                  Slot.SetRace(SLOTRACE_NIGHTELF);
                else
                  Slot.SetRace(SLOTRACE_RANDOM);

                ISS.seekg(4, ios::cur);            // fixed start position
                getline(ISS, GarbageString, '\0'); // player name
                ISS.seekg(4, ios::cur);            // start position x
                ISS.seekg(4, ios::cur);            // start position y
                ISS.seekg(4, ios::cur);            // ally low priorities
                ISS.seekg(4, ios::cur);            // ally high priorities

                if (Slot.GetSlotStatus() != SLOTSTATUS_CLOSED)
                  Slots.push_back(Slot);
              }

              ISS.read((char*)&RawMapNumTeams, 4); // number of teams

              for (uint32_t i = 0; i < RawMapNumTeams; ++i)
              {
                uint32_t Flags;
                uint32_t PlayerMask;

                ISS.read((char*)&Flags, 4);      // flags
                ISS.read((char*)&PlayerMask, 4); // player mask

                for (uint8_t j = 0; j < 12; ++j)
                {
                  if (PlayerMask & 1)
                  {
                    for (auto& Slot : Slots)
                    {
                      if ((Slot).GetColour() == j)
                        (Slot).SetTeam(i);
                    }
                  }

                  PlayerMask >>= 1;
                }

                getline(ISS, GarbageString, '\0'); // team name
              }

              // the bot only cares about the following options: melee, fixed player settings, custom forces
              // let's not confuse the user by displaying erroneous map options so zero them out now

              MapOptions = RawMapFlags & (MAPOPT_MELEE | MAPOPT_FIXEDPLAYERSETTINGS | MAPOPT_CUSTOMFORCES);
              Print("[MAP] calculated map_options = " + to_string(MapOptions));
              MapWidth = CreateByteArray((uint16_t)RawMapWidth, false);
              Print("[MAP] calculated map_width = " + ByteArrayToDecString(MapWidth));
              MapHeight = CreateByteArray((uint16_t)RawMapHeight, false);
              Print("[MAP] calculated map_height = " + ByteArrayToDecString(MapHeight));
              MapNumPlayers = RawMapNumPlayers - ClosedSlots;
              Print("[MAP] calculated map_numplayers = " + to_string(MapNumPlayers));
              MapNumTeams = RawMapNumTeams;
              Print("[MAP] calculated map_numteams = " + to_string(MapNumTeams));

              uint32_t SlotNum = 1;

              for (auto& Slot : Slots)
              {
                Print("[MAP] calculated map_slot" + to_string(SlotNum) + " = " + ByteArrayToDecString((Slot).GetByteArray()));
                ++SlotNum;
              }

              if (MapOptions & MAPOPT_MELEE)
              {
                Print("[MAP] found melee map, initializing slots");

                // give each slot a different team and set the race to random

                uint8_t Team = 0;

                for (auto& Slot : Slots)
                {
                  (Slot).SetTeam(Team++);
                  (Slot).SetRace(SLOTRACE_RANDOM);
                }

                MapFilterType = MAPFILTER_TYPE_MELEE;
              }

              if (!(MapOptions & MAPOPT_FIXEDPLAYERSETTINGS))
              {
                // make races selectable

                for (auto& Slot : Slots)
                  (Slot).SetRace((Slot).GetRace() | SLOTRACE_SELECTABLE);
              }
            }
          }
          else
            Print("[MAP] unable to calculate map_options, map_width, map_height, map_slot<x>, map_numplayers, map_numteams - unable to extract war3map.w3i from MPQ file");

          delete[] SubFileData;
        }

        SFileCloseFile(SubFile);
      }
      else
        Print("[MAP] unable to calculate map_options, map_width, map_height, map_slot<x>, map_numplayers, map_numteams - couldn't find war3map.w3i in MPQ file");
    }
    else
      Print("[MAP] unable to calculate map_options, map_width, map_height, map_slot<x>, map_numplayers, map_numteams - map MPQ file not loaded");
  }
  else
    Print("[MAP] no map data available, using config file for map_options, map_width, map_height, map_slot<x>, map_numplayers, map_numteams");

  // close the map MPQ

  if (MapMPQReady)
    SFileCloseArchive(MapMPQ);

  m_MapPath = CFG->GetString("map_path", string());

  if (MapSize.empty())
    MapSize = ExtractNumbers(CFG->GetString("map_size", string()), 4);
  else if (CFG->Exists("map_size"))
  {
    Print("[MAP] overriding calculated map_size with config value map_size = " + CFG->GetString("map_size", string()));
    MapSize = ExtractNumbers(CFG->GetString("map_size", string()), 4);
  }

  m_MapSize = MapSize;

  if (MapInfo.empty())
    MapInfo = ExtractNumbers(CFG->GetString("map_info", string()), 4);
  else if (CFG->Exists("map_info"))
  {
    Print("[MAP] overriding calculated map_info with config value map_info = " + CFG->GetString("map_info", string()));
    MapInfo = ExtractNumbers(CFG->GetString("map_info", string()), 4);
  }

  m_MapInfo = MapInfo;

  if (MapCRC.empty())
    MapCRC = ExtractNumbers(CFG->GetString("map_crc", string()), 4);
  else if (CFG->Exists("map_crc"))
  {
    Print("[MAP] overriding calculated map_crc with config value map_crc = " + CFG->GetString("map_crc", string()));
    MapCRC = ExtractNumbers(CFG->GetString("map_crc", string()), 4);
  }

  m_MapCRC = MapCRC;

  if (MapSHA1.empty())
    MapSHA1 = ExtractNumbers(CFG->GetString("map_sha1", string()), 20);
  else if (CFG->Exists("map_sha1"))
  {
    Print("[MAP] overriding calculated map_sha1 with config value map_sha1 = " + CFG->GetString("map_sha1", string()));
    MapSHA1 = ExtractNumbers(CFG->GetString("map_sha1", string()), 20);
  }

  m_MapSHA1 = MapSHA1;

  if (CFG->Exists("map_filter_type"))
  {
    Print("[MAP] overriding calculated map_filter_type with config value map_filter_type = " + CFG->GetString("map_filter_type", string()));
    MapFilterType = CFG->GetInt("map_filter_type", MAPFILTER_TYPE_SCENARIO);
  }

  m_MapFilterType = MapFilterType;

  m_MapSpeed       = CFG->GetInt("map_speed", MAPSPEED_FAST);
  m_MapVisibility  = CFG->GetInt("map_visibility", MAPVIS_DEFAULT);
  m_MapObservers   = CFG->GetInt("map_observers", MAPOBS_NONE);
  m_MapFlags       = CFG->GetInt("map_flags", MAPFLAG_TEAMSTOGETHER | MAPFLAG_FIXEDTEAMS);
  m_MapFilterMaker = CFG->GetInt("map_filter_maker", MAPFILTER_MAKER_USER);
  m_MapFilterSize  = CFG->GetInt("map_filter_size", MAPFILTER_SIZE_LARGE);
  m_MapFilterObs   = CFG->GetInt("map_filter_obs", MAPFILTER_OBS_NONE);

  // TODO: it might be possible for MapOptions to legitimately be zero so this is not a valid way of checking if it wasn't parsed out earlier

  if (MapOptions == 0)
    MapOptions = CFG->GetInt("map_options", 0);
  else if (CFG->Exists("map_options"))
  {
    Print("[MAP] overriding calculated map_options with config value map_options = " + CFG->GetString("map_options", string()));
    MapOptions = CFG->GetInt("map_options", 0);
  }

  m_MapOptions = MapOptions;

  if (MapWidth.empty())
    MapWidth = ExtractNumbers(CFG->GetString("map_width", string()), 2);
  else if (CFG->Exists("map_width"))
  {
    Print("[MAP] overriding calculated map_width with config value map_width = " + CFG->GetString("map_width", string()));
    MapWidth = ExtractNumbers(CFG->GetString("map_width", string()), 2);
  }

  m_MapWidth = MapWidth;

  if (MapHeight.empty())
    MapHeight = ExtractNumbers(CFG->GetString("map_height", string()), 2);
  else if (CFG->Exists("map_height"))
  {
    Print("[MAP] overriding calculated map_height with config value map_height = " + CFG->GetString("map_height", string()));
    MapHeight = ExtractNumbers(CFG->GetString("map_height", string()), 2);
  }

  m_MapHeight     = MapHeight;
  m_MapType       = CFG->GetString("map_type", string());
  m_MapDefaultHCL = CFG->GetString("map_defaulthcl", string());

  if (MapNumPlayers == 0)
    MapNumPlayers = CFG->GetInt("map_numplayers", 0);
  else if (CFG->Exists("map_numplayers"))
  {
    Print("[MAP] overriding calculated map_numplayers with config value map_numplayers = " + CFG->GetString("map_numplayers", string()));
    MapNumPlayers = CFG->GetInt("map_numplayers", 0);
  }

  m_MapNumPlayers = MapNumPlayers;

  if (MapNumTeams == 0)
    MapNumTeams = CFG->GetInt("map_numteams", 0);
  else if (CFG->Exists("map_numteams"))
  {
    Print("[MAP] overriding calculated map_numteams with config value map_numteams = " + CFG->GetString("map_numteams", string()));
    MapNumTeams = CFG->GetInt("map_numteams", 0);
  }

  m_MapNumTeams = MapNumTeams;

  if (Slots.empty())
  {
    for (uint32_t Slot = 1; Slot <= 12; ++Slot)
    {
      string SlotString = CFG->GetString("map_slot" + to_string(Slot), string());

      if (SlotString.empty())
        break;

      std::vector<uint8_t> SlotData = ExtractNumbers(SlotString, 9);
      Slots.emplace_back(SlotData);
    }
  }
  else if (CFG->Exists("map_slot1"))
  {
    Print("[MAP] overriding slots");
    Slots.clear();

    for (uint32_t Slot = 1; Slot <= 12; ++Slot)
    {
      string SlotString = CFG->GetString("map_slot" + to_string(Slot), string());

      if (SlotString.empty())
        break;

      std::vector<uint8_t> SlotData = ExtractNumbers(SlotString, 9);
      Slots.emplace_back(SlotData);
    }
  }

  m_Slots = Slots;

  // if random races is set force every slot's race to random

  if (m_MapFlags & MAPFLAG_RANDOMRACES)
  {
    Print("[MAP] forcing races to random");

    for (auto& slot : m_Slots)
      slot.SetRace(SLOTRACE_RANDOM);
  }

  // force melee maps to have observer slots enabled by default

  if (m_MapFilterType & MAPFILTER_TYPE_MELEE && m_MapObservers == MAPOBS_NONE)
    m_MapObservers = MAPOBS_ALLOWED;

  // add observer slots

  if (m_MapObservers == MAPOBS_ALLOWED || m_MapObservers == MAPOBS_REFEREES)
  {
    Print("[MAP] adding " + to_string(12 - m_Slots.size()) + " observer slots");

    while (m_Slots.size() < 12)
      m_Slots.emplace_back(0, 255, SLOTSTATUS_OPEN, 0, 12, 12, SLOTRACE_RANDOM);
  }

  const char* ErrorMessage = CheckValid();

  if (ErrorMessage)
    Print(std::string("[MAP] ") + ErrorMessage);
}

const char* CMap::CheckValid()
{
  // TODO: should this code fix any errors it sees rather than just warning the user?

  if (m_MapPath.empty() || m_MapPath.length() > 53)
  {
    m_Valid = false;
    return "invalid map_path detected";
  }

  if (m_MapPath.find('/') != string::npos)
    Print(R"(warning - map_path contains forward slashes '/' but it must use Windows style back slashes '\')");

  if (m_MapSize.size() != 4)
  {
    m_Valid = false;
    return "invalid map_size detected";
  }
  else if (!m_MapData.empty() && m_MapData.size() != ByteArrayToUInt32(m_MapSize, false))
  {
    m_Valid = false;
    return "invalid map_size detected - size mismatch with actual map data";
  }

  if (m_MapInfo.size() != 4)
  {
    m_Valid = false;
    return "invalid map_info detected";
  }

  if (m_MapCRC.size() != 4)
  {
    m_Valid = false;
    return "invalid map_crc detected";
  }

  if (m_MapSHA1.size() != 20)
  {
    m_Valid = false;
    return "invalid map_sha1 detected";
  }

  if (m_MapSpeed != MAPSPEED_SLOW && m_MapSpeed != MAPSPEED_NORMAL && m_MapSpeed != MAPSPEED_FAST)
  {
    m_Valid = false;
    return "invalid map_speed detected";
  }

  if (m_MapVisibility != MAPVIS_HIDETERRAIN && m_MapVisibility != MAPVIS_EXPLORED && m_MapVisibility != MAPVIS_ALWAYSVISIBLE && m_MapVisibility != MAPVIS_DEFAULT)
  {
    m_Valid = false;
    return "invalid map_visibility detected";
  }

  if (m_MapObservers != MAPOBS_NONE && m_MapObservers != MAPOBS_ONDEFEAT && m_MapObservers != MAPOBS_ALLOWED && m_MapObservers != MAPOBS_REFEREES)
  {
    m_Valid = false;
    return "invalid map_observers detected";
  }

  if (m_MapWidth.size() != 2)
  {
    m_Valid = false;
    return "invalid map_width detected";
  }

  if (m_MapHeight.size() != 2)
  {
    m_Valid = false;
    return "invalid map_height detected";
  }

  if (m_MapNumPlayers == 0 || m_MapNumPlayers > 12)
  {
    m_Valid = false;
    return "invalid map_numplayers detected";
  }

  if (m_MapNumTeams == 0 || m_MapNumTeams > 12)
  {
    m_Valid = false;
    return "invalid map_numteams detected";
  }

  if (m_Slots.empty() || m_Slots.size() > 12)
  {
    m_Valid = false;
    return "invalid map_slot<x> detected";
  }

  return nullptr;
}

uint32_t CMap::XORRotateLeft(uint8_t* data, uint32_t length)
{
  // a big thank you to Strilanc for figuring this out

  uint32_t i   = 0;
  uint32_t Val = 0;

  if (length > 3)
  {
    while (i < length - 3)
    {
      Val = ROTL(Val ^ ((uint32_t)data[i] + (uint32_t)(data[i + 1] << 8) + (uint32_t)(data[i + 2] << 16) + (uint32_t)(data[i + 3] << 24)), 3);
      i += 4;
    }
  }

  while (i < length)
  {
    Val = ROTL(Val ^ data[i], 3);
    ++i;
  }

  return Val;
}
