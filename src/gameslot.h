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

#ifndef AURA_GAMESLOT_H_
#define AURA_GAMESLOT_H_

#include <string>
#include <cstdint>
#include <vector>

#define SLOTSTATUS_OPEN 0
#define SLOTSTATUS_CLOSED 1
#define SLOTSTATUS_OCCUPIED 2

#define SLOTRACE_HUMAN 1
#define SLOTRACE_ORC 2
#define SLOTRACE_NIGHTELF 4
#define SLOTRACE_UNDEAD 8
#define SLOTRACE_RANDOM 32
#define SLOTRACE_SELECTABLE 64

#define SLOTCOMP_EASY 0
#define SLOTCOMP_NORMAL 1
#define SLOTCOMP_HARD 2

//
// CGameSlot
//

class CGameSlot
{
private:
  uint_fast8_t m_PID;            // player id
  uint_fast8_t m_DownloadStatus; // download status (0% to 100%)
  uint_fast8_t m_SlotStatus;     // slot status (0 = open, 1 = closed, 2 = occupied)
  uint_fast8_t m_Computer;       // computer (0 = no, 1 = yes)
  uint_fast8_t m_Team;           // team
  uint_fast8_t m_Colour;         // colour
  uint_fast8_t m_Race;           // race (1 = human, 2 = orc, 4 = night elf, 8 = undead, 32 = random, 64 = selectable)
  uint_fast8_t m_ComputerType;   // computer type (0 = easy, 1 = human or normal comp, 2 = hard comp)
  uint_fast8_t m_Handicap;       // handicap

public:
  explicit CGameSlot(const std::vector<uint_fast8_t>& n);
  CGameSlot(const uint_fast8_t nPID, const uint_fast8_t nDownloadStatus, const uint_fast8_t nSlotStatus, const uint_fast8_t nComputer, const uint_fast8_t nTeam, const uint_fast8_t nColour, const uint_fast8_t nRace, const uint_fast8_t nComputerType = 1, const uint_fast8_t nHandicap = 100);
  ~CGameSlot();

  inline uint_fast8_t              GetPID() const { return m_PID; }
  inline uint_fast8_t              GetDownloadStatus() const { return m_DownloadStatus; }
  inline uint_fast8_t              GetSlotStatus() const { return m_SlotStatus; }
  inline uint_fast8_t              GetComputer() const { return m_Computer; }
  inline uint_fast8_t              GetTeam() const { return m_Team; }
  inline uint_fast8_t              GetColour() const { return m_Colour; }
  inline uint_fast8_t              GetRace() const { return m_Race; }
  inline uint_fast8_t              GetComputerType() const { return m_ComputerType; }
  inline uint_fast8_t              GetHandicap() const { return m_Handicap; }
  inline std::vector<uint_fast8_t> GetByteArray() const { return std::vector<uint_fast8_t>{m_PID, m_DownloadStatus, m_SlotStatus, m_Computer, m_Team, m_Colour, m_Race, m_ComputerType, m_Handicap}; }

  inline void SetPID(uint_fast8_t nPID) { m_PID = nPID; }
  inline void SetDownloadStatus(uint_fast8_t nDownloadStatus) { m_DownloadStatus = nDownloadStatus; }
  inline void SetSlotStatus(uint_fast8_t nSlotStatus) { m_SlotStatus = nSlotStatus; }
  inline void SetComputer(uint_fast8_t nComputer) { m_Computer = nComputer; }
  inline void SetTeam(uint_fast8_t nTeam) { m_Team = nTeam; }
  inline void SetColour(uint_fast8_t nColour) { m_Colour = nColour; }
  inline void SetRace(uint_fast8_t nRace) { m_Race = nRace; }
  inline void SetComputerType(uint_fast8_t nComputerType) { m_ComputerType = nComputerType; }
  inline void SetHandicap(uint_fast8_t nHandicap) { m_Handicap = nHandicap; }
};

#endif // AURA_GAMESLOT_H_
