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

#include "aura.h"
#include "gameslot.h"

using namespace std;

//
// CGameSlot
//

CGameSlot::CGameSlot(const std::vector<uint8_t>& n)
  : m_PID(0),
    m_DownloadStatus(255),
    m_SlotStatus(SLOTSTATUS_OPEN),
    m_Computer(0),
    m_Team(0),
    m_Colour(1),
    m_Race(SLOTRACE_RANDOM),
    m_ComputerType(SLOTCOMP_NORMAL),
    m_Handicap(100)
{
  const size_t size = n.size();

  if (size >= 7)
  {
    m_PID            = n[0];
    m_DownloadStatus = n[1];
    m_SlotStatus     = n[2];
    m_Computer       = n[3];
    m_Team           = n[4];
    m_Colour         = n[5];
    m_Race           = n[6];

    if (size >= 8)
    {
      m_ComputerType = n[7];

      if (size >= 9)
        m_Handicap = n[8];
    }
  }
}

CGameSlot::CGameSlot(const uint8_t nPID, const uint8_t nDownloadStatus, const uint8_t nSlotStatus, const uint8_t nComputer, const uint8_t nTeam, const uint8_t nColour, const uint8_t nRace, const uint8_t nComputerType, const uint8_t nHandicap)
  : m_PID(nPID),
    m_DownloadStatus(nDownloadStatus),
    m_SlotStatus(nSlotStatus),
    m_Computer(nComputer),
    m_Team(nTeam),
    m_Colour(nColour),
    m_Race(nRace),
    m_ComputerType(nComputerType),
    m_Handicap(nHandicap)
{
}

CGameSlot::~CGameSlot() = default;
