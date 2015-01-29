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

#include "includes.h"

#define SLOTSTATUS_OPEN     0
#define SLOTSTATUS_CLOSED   1
#define SLOTSTATUS_OCCUPIED 2

#define SLOTRACE_HUMAN      1
#define SLOTRACE_ORC        2
#define SLOTRACE_NIGHTELF   4
#define SLOTRACE_UNDEAD     8
#define SLOTRACE_RANDOM     32
#define SLOTRACE_SELECTABLE 64

#define SLOTCOMP_EASY       0
#define SLOTCOMP_NORMAL     1
#define SLOTCOMP_HARD       2

//
// CGameSlot
//

class CGameSlot
{
private:
  uint8_t m_PID;            // player id
  uint8_t m_DownloadStatus; // download status (0% to 100%)
  uint8_t m_SlotStatus;     // slot status (0 = open, 1 = closed, 2 = occupied)
  uint8_t m_Computer;       // computer (0 = no, 1 = yes)
  uint8_t m_Team;           // team
  uint8_t m_Colour;         // colour
  uint8_t m_Race;           // race (1 = human, 2 = orc, 4 = night elf, 8 = undead, 32 = random, 64 = selectable)
  uint8_t m_ComputerType;   // computer type (0 = easy, 1 = human or normal comp, 2 = hard comp)
  uint8_t m_Handicap;       // handicap

public:
  explicit CGameSlot(BYTEARRAY &n);
  CGameSlot(uint8_t nPID, uint8_t nDownloadStatus, uint8_t nSlotStatus, uint8_t nComputer, uint8_t nTeam, uint8_t nColour, uint8_t nRace, uint8_t nComputerType = 1, uint8_t nHandicap = 100);
  ~CGameSlot();

  inline uint8_t GetPID() const                           { return m_PID; }
  inline uint8_t GetDownloadStatus() const                { return m_DownloadStatus; }
  inline uint8_t GetSlotStatus() const                    { return m_SlotStatus; }
  inline uint8_t GetComputer() const                      { return m_Computer; }
  inline uint8_t GetTeam() const                          { return m_Team; }
  inline uint8_t GetColour() const                        { return m_Colour; }
  inline uint8_t GetRace() const                          { return m_Race; }
  inline uint8_t GetComputerType() const                  { return m_ComputerType; }
  inline uint8_t GetHandicap() const                      { return m_Handicap; }
  BYTEARRAY GetByteArray() const;

  inline void SetPID(uint8_t nPID)                       { m_PID = nPID; }
  inline void SetDownloadStatus(uint8_t nDownloadStatus) { m_DownloadStatus = nDownloadStatus; }
  inline void SetSlotStatus(uint8_t nSlotStatus)         { m_SlotStatus = nSlotStatus; }
  inline void SetComputer(uint8_t nComputer)             { m_Computer = nComputer; }
  inline void SetTeam(uint8_t nTeam)                     { m_Team = nTeam; }
  inline void SetColour(uint8_t nColour)                 { m_Colour = nColour; }
  inline void SetRace(uint8_t nRace)                     { m_Race = nRace; }
  inline void SetComputerType(uint8_t nComputerType)     { m_ComputerType = nComputerType; }
  inline void SetHandicap(uint8_t nHandicap)             { m_Handicap = nHandicap; }
};

#endif  // AURA_GAMESLOT_H_
