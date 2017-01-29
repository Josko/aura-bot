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

#include "stats.h"
#include "aura.h"
#include "auradb.h"
#include "game.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "util.h"

using namespace std;

//
// CStats
//

CStats::CStats(CGame* nGame)
  : m_Game(nGame),
    m_Winner(0)
{
  Print("[STATS] using dota stats");

  for (auto& player : m_Players)
    player = nullptr;
}

CStats::~CStats()
{
  for (auto& player : m_Players)
  {
    if (player)
      delete player;
  }
}

bool CStats::ProcessAction(CIncomingAction* Action)
{
  uint32_t                    i          = 0;
  const std::vector<uint8_t>* ActionData = Action->GetAction();
  std::vector<uint8_t>        Data, Key, Value;

  // dota actions with real time replay data start with 0x6b then the nullptr terminated string "dr.x"
  // unfortunately more than one action can be sent in a single packet and the length of each action isn't explicitly represented in the packet
  // so we have to either parse all the actions and calculate the length based on the type or we can search for an identifying sequence
  // parsing the actions would be more correct but would be a lot more difficult to write for relatively little gain
  // so we take the easy route (which isn't always guaranteed to work) and search the data for the sequence "6b 64 72 2e 78 00" and hope it identifies an action

  do
  {
    if ((*ActionData)[i] == 0x6b && (*ActionData)[i + 1] == 0x64 && (*ActionData)[i + 2] == 0x72 && (*ActionData)[i + 3] == 0x2e && (*ActionData)[i + 4] == 0x78 && (*ActionData)[i + 5] == 0x00)
    {
      // we think we've found an action with real time replay data (but we can't be 100% sure)
      // next we parse out two nullptr terminated strings and a 4 byte int32_teger

      if (ActionData->size() >= i + 7)
      {
        // the first nullptr terminated string should either be the strings "Data" or "Global" or a player id in ASCII representation, e.g. "1" or "2"

        Data = ExtractCString(*ActionData, i + 6);

        if (ActionData->size() >= i + 8 + Data.size())
        {
          // the second nullptr terminated string should be the key

          Key = ExtractCString(*ActionData, i + 7 + Data.size());

          if (ActionData->size() >= i + 12 + Data.size() + Key.size())
          {
            // the 4 byte int32_teger should be the value

            Value                     = std::vector<uint8_t>(ActionData->begin() + i + 8 + Data.size() + Key.size(), ActionData->begin() + i + 12 + Data.size() + Key.size());
            const string   DataString = string(begin(Data), end(Data));
            const string   KeyString  = string(begin(Key), end(Key));
            const uint32_t ValueInt   = ByteArrayToUInt32(Value, false);

            //Print( "[STATS] " + DataString + ", " + KeyString + ", " + to_string( ValueInt ) );

            if (DataString == "Data")
            {
              // these are received during the game
              // you could use these to calculate killing sprees and double or triple kills (you'd have to make up your own time restrictions though)
              // you could also build a table of "who killed who" data

              if (KeyString.size() >= 5 && KeyString.compare(0, 4, "Hero") == 0)
              {
                // a hero died

                const string   VictimName   = KeyString.substr(4);
                const uint32_t KillerColour = ValueInt;
                const uint32_t VictimColour = stoul(VictimName);
                CGamePlayer*   Killer       = m_Game->GetPlayerFromColour(ValueInt);
                CGamePlayer*   Victim       = m_Game->GetPlayerFromColour(VictimColour);

                if (!m_Players[ValueInt])
                  m_Players[ValueInt] = new CDBDotAPlayer();

                if (!m_Players[VictimColour])
                  m_Players[VictimColour] = new CDBDotAPlayer();

                if (Victim)
                {
                  if (Killer)
                  {
                    // check for hero denies

                    if (!((KillerColour <= 5 && VictimColour <= 5) || (KillerColour >= 7 && VictimColour >= 7)))
                    {
                      // non-leaver killed a non-leaver

                      m_Players[KillerColour]->IncKills();
                      m_Players[VictimColour]->IncDeaths();
                    }
                  }
                  else
                  {
                    // Scourge/Sentinel/leaver killed a non-leaver

                    m_Players[VictimColour]->IncDeaths();
                  }
                }
              }
              else if (KeyString.size() >= 7 && KeyString.compare(0, 6, "Assist") == 0)
              {
                // check if the assist was on a non-leaver

                if (m_Game->GetPlayerFromColour(ValueInt))
                {
                  string         AssisterName   = KeyString.substr(6);
                  const uint32_t AssisterColour = stoul(AssisterName);

                  if (!m_Players[AssisterColour])
                    m_Players[AssisterColour] = new CDBDotAPlayer();

                  m_Players[AssisterColour]->IncAssists();
                }
              }
              else if (KeyString.size() >= 8 && KeyString.compare(0, 5, "Tower") == 0)
              {
                // a tower died

                if ((ValueInt >= 1 && ValueInt <= 5) || (ValueInt >= 7 && ValueInt <= 11))
                {
                  if (!m_Players[ValueInt])
                    m_Players[ValueInt] = new CDBDotAPlayer();

                  m_Players[ValueInt]->IncTowerKills();
                }
              }
              else if (KeyString.size() >= 6 && KeyString.compare(0, 3, "Rax") == 0)
              {
                // a rax died

                if ((ValueInt >= 1 && ValueInt <= 5) || (ValueInt >= 7 && ValueInt <= 11))
                {
                  if (!m_Players[ValueInt])
                    m_Players[ValueInt] = new CDBDotAPlayer();

                  m_Players[ValueInt]->IncRaxKills();
                }
              }
              else if (KeyString.size() >= 8 && KeyString.compare(0, 7, "Courier") == 0)
              {
                // a courier died

                if ((ValueInt >= 1 && ValueInt <= 5) || (ValueInt >= 7 && ValueInt <= 11))
                {
                  if (!m_Players[ValueInt])
                    m_Players[ValueInt] = new CDBDotAPlayer();

                  m_Players[ValueInt]->IncCourierKills();
                }
              }
            }
            else if (DataString == "Global")
            {
              // these are only received at the end of the game

              if (KeyString == "Winner")
              {
                // Value 1 -> sentinel
                // Value 2 -> scourge

                m_Winner = ValueInt;

                if (m_Winner == 1)
                  Print("[STATS: " + m_Game->GetGameName() + "] detected winner: Sentinel");
                else if (m_Winner == 2)
                  Print("[STATS: " + m_Game->GetGameName() + "] detected winner: Scourge");
                else
                  Print("[STATS: " + m_Game->GetGameName() + "] detected winner: " + to_string(ValueInt));
              }
            }
            else if (DataString.size() <= 2 && DataString.find_first_not_of("1234567890") == string::npos)
            {
              // these are only received at the end of the game

              const uint32_t ID = stoul(DataString);

              if ((ID >= 1 && ID <= 5) || (ID >= 7 && ID <= 11))
              {
                if (!m_Players[ID])
                {
                  m_Players[ID] = new CDBDotAPlayer();
                  m_Players[ID]->SetColour(ID);
                }

                // Key "3"		-> Creep Kills
                // Key "4"		-> Creep Denies
                // Key "7"		-> Neutral Kills
                // Key "id"     -> ID (1-5 for sentinel, 6-10 for scourge, accurate after using -sp and/or -switch)

                switch (KeyString[0])
                {
                  case '3':
                    m_Players[ID]->SetCreepKills(ValueInt);
                    break;

                  case '4':
                    m_Players[ID]->SetCreepDenies(ValueInt);
                    break;

                  case '7':
                    m_Players[ID]->SetNeutralKills(ValueInt);
                    break;

                  case 'i':
                    if (KeyString[1] == 'd')
                    {
                      // DotA sends id values from 1-10 with 1-5 being sentinel players and 6-10 being scourge players
                      // unfortunately the actual player colours are from 1-5 and from 7-11 so we need to deal with this case here

                      if (ValueInt >= 6)
                        m_Players[ID]->SetNewColour(ValueInt + 1);
                      else
                        m_Players[ID]->SetNewColour(ValueInt);
                    }

                    break;

                  default:
                    break;
                }
              }
            }

            i += 12 + Data.size() + Key.size();
          }
          else
            ++i;
        }
        else
          ++i;
      }
      else
        ++i;
    }
    else
      ++i;
  } while (ActionData->size() >= i + 6);

  return m_Winner != 0;
}

void CStats::Save(CAura* Aura, CAuraDB* DB)
{
  if (DB->Begin())
  {
    // since we only record the end game information it's possible we haven't recorded anything yet if the game didn't end with a tree/throne death
    // this will happen if all the players leave before properly finishing the game
    // the dotagame stats are always saved (with winner = 0 if the game didn't properly finish)
    // the dotaplayer stats are only saved if the game is properly finished

    uint32_t Players = 0;

    // check for invalid colours and duplicates
    // this can only happen if DotA sends us garbage in the "id" value but we should check anyway

    for (uint32_t i = 0; i < 12; ++i)
    {
      if (m_Players[i])
      {
        const uint32_t Colour = m_Players[i]->GetNewColour();

        if (!((Colour >= 1 && Colour <= 5) || (Colour >= 7 && Colour <= 11)))
        {
          Print("[STATS: " + m_Game->GetGameName() + "] discarding player data, invalid colour found");
          delete m_Players[i];
          m_Players[i] = nullptr;
          continue;
        }

        for (uint32_t j = i + 1; j < 12; ++j)
        {
          if (m_Players[j] && Colour == m_Players[j]->GetNewColour())
          {
            Print("[STATS: " + m_Game->GetGameName() + "] discarding player data, duplicate colour found");
            delete m_Players[j];
            m_Players[j] = nullptr;
          }
        }
      }
    }

    for (auto& player : m_Players)
    {
      if (player)
      {
        const uint32_t Colour = player->GetNewColour();
        const string   Name   = m_Game->GetDBPlayerNameFromColour(Colour);

        if (Name.empty())
          continue;

        uint8_t Win = 0;

        if ((m_Winner == 1 && Colour >= 1 && Colour <= 5) || (m_Winner == 2 && Colour >= 7 && Colour <= 11))
          Win = 1;
        else if ((m_Winner == 2 && Colour >= 1 && Colour <= 5) || (m_Winner == 1 && Colour >= 7 && Colour <= 11))
          Win = 2;

        Aura->m_DB->DotAPlayerAdd(Name, Win, player->GetKills(), player->GetDeaths(), player->GetCreepKills(), player->GetCreepDenies(), player->GetAssists(), player->GetNeutralKills(), player->GetTowerKills(), player->GetRaxKills(), player->GetCourierKills());
        ++Players;
      }
    }

    if (DB->Commit())
      Print("[STATS: " + m_Game->GetGameName() + "] saving " + to_string(Players) + " players");
    else
      Print("[STATS: " + m_Game->GetGameName() + "] unable to commit database transaction, data not saved");
  }
  else
    Print("[STATS: " + m_Game->GetGameName() + "] unable to begin database transaction, data not saved");
}
