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

#include <utility>

#include "gameprotocol.h"
#include "aura.h"
#include "util.h"
#include "crc32.h"
#include "gameplayer.h"
#include "gameslot.h"
#include "game.h"

using namespace std;

//
// CGameProtocol
//

CGameProtocol::CGameProtocol(CAura* nAura)
  : m_Aura(nAura)
{
}

CGameProtocol::~CGameProtocol() = default;

///////////////////////
// RECEIVE FUNCTIONS //
///////////////////////

CIncomingJoinPlayer* CGameProtocol::RECEIVE_W3GS_REQJOIN(const std::vector<uint8_t>& data)
{
  // DEBUG_Print( "RECEIVED W3GS_REQJOIN" );
  // DEBUG_Print( data );

  // 2 bytes                    -> Header
  // 2 bytes                    -> Length
  // 4 bytes                    -> Host Counter (Game ID)
  // 4 bytes                    -> Entry Key (used in LAN)
  // 1 byte                     -> ???
  // 2 bytes                    -> Listen Port
  // 4 bytes                    -> Peer Key
  // null terminated string			-> Name
  // 4 bytes                    -> ???
  // 2 bytes                    -> InternalPort (???)
  // 4 bytes                    -> InternalIP

  if (ValidateLength(data) && data.size() >= 20)
  {
    const uint32_t             HostCounter = ByteArrayToUInt32(data, false, 4);
    const uint32_t             EntryKey    = ByteArrayToUInt32(data, false, 8);
    const std::vector<uint8_t> Name        = ExtractCString(data, 19);

    if (!Name.empty() && data.size() >= Name.size() + 30)
    {
      const std::vector<uint8_t> InternalIP = std::vector<uint8_t>(begin(data) + Name.size() + 26, begin(data) + Name.size() + 30);
      return new CIncomingJoinPlayer(HostCounter, EntryKey, string(begin(Name), end(Name)), InternalIP);
    }
  }

  return nullptr;
}

uint32_t CGameProtocol::RECEIVE_W3GS_LEAVEGAME(const std::vector<uint8_t>& data)
{
  // DEBUG_Print( "RECEIVED W3GS_LEAVEGAME" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> Reason

  if (ValidateLength(data) && data.size() >= 8)
    return ByteArrayToUInt32(data, false, 4);

  return 0;
}

bool CGameProtocol::RECEIVE_W3GS_GAMELOADED_SELF(const std::vector<uint8_t>& data)
{
  // DEBUG_Print( "RECEIVED W3GS_GAMELOADED_SELF" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length

  if (ValidateLength(data))
    return true;

  return false;
}

CIncomingAction* CGameProtocol::RECEIVE_W3GS_OUTGOING_ACTION(const std::vector<uint8_t>& data, uint8_t PID)
{
  // DEBUG_Print( "RECEIVED W3GS_OUTGOING_ACTION" );
  // DEBUG_Print( data );

  // 2 bytes                -> Header
  // 2 bytes                -> Length
  // 4 bytes                -> CRC
  // remainder of packet		-> Action

  if (PID != 255 && ValidateLength(data) && data.size() >= 8)
  {
    const std::vector<uint8_t> CRC    = std::vector<uint8_t>(begin(data) + 4, begin(data) + 8);
    const std::vector<uint8_t> Action = std::vector<uint8_t>(begin(data) + 8, end(data));
    return new CIncomingAction(PID, CRC, Action);
  }

  return nullptr;
}

uint32_t CGameProtocol::RECEIVE_W3GS_OUTGOING_KEEPALIVE(const std::vector<uint8_t>& data)
{
  // DEBUG_Print( "RECEIVED W3GS_OUTGOING_KEEPALIVE" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 1 byte           -> ???
  // 4 bytes					-> CheckSum

  if (ValidateLength(data) && data.size() == 9)
    return ByteArrayToUInt32(data, false, 5);

  return 0;
}

CIncomingChatPlayer* CGameProtocol::RECEIVE_W3GS_CHAT_TO_HOST(const std::vector<uint8_t>& data)
{
  // DEBUG_Print( "RECEIVED W3GS_CHAT_TO_HOST" );
  // DEBUG_Print( data );

  // 2 bytes              -> Header
  // 2 bytes              -> Length
  // 1 byte               -> Total
  // for( 1 .. Total )
  //		1 byte            -> ToPID
  // 1 byte               -> FromPID
  // 1 byte               -> Flag
  // if( Flag == 16 )
  //		null term string	-> Message
  // elseif( Flag == 17 )
  //		1 byte            -> Team
  // elseif( Flag == 18 )
  //		1 byte            -> Colour
  // elseif( Flag == 19 )
  //		1 byte            -> Race
  // elseif( Flag == 20 )
  //		1 byte            -> Handicap
  // elseif( Flag == 32 )
  //		4 bytes           -> ExtraFlags
  //		null term string	-> Message

  if (ValidateLength(data))
  {
    uint32_t      i     = 5;
    const uint8_t Total = data[4];

    if (Total > 0 && data.size() >= i + Total)
    {
      const std::vector<uint8_t> ToPIDs = std::vector<uint8_t>(begin(data) + i, begin(data) + i + Total);
      i += Total;
      const uint8_t FromPID = data[i];
      const uint8_t Flag    = data[i + 1];
      i += 2;

      if (Flag == 16 && data.size() >= i + 1)
      {
        // chat message

        const std::vector<uint8_t> Message = ExtractCString(data, i);
        return new CIncomingChatPlayer(FromPID, ToPIDs, Flag, string(begin(Message), end(Message)));
      }
      else if ((Flag >= 17 && Flag <= 20) && data.size() >= i + 1)
      {
        // team/colour/race/handicap change request

        const uint8_t Byte = data[i];
        return new CIncomingChatPlayer(FromPID, ToPIDs, Flag, Byte);
      }
      else if (Flag == 32 && data.size() >= i + 5)
      {
        // chat message with extra flags

        const std::vector<uint8_t> ExtraFlags = std::vector<uint8_t>(begin(data) + i, begin(data) + i + 4);
        const std::vector<uint8_t> Message    = ExtractCString(data, i + 4);
        return new CIncomingChatPlayer(FromPID, ToPIDs, Flag, string(begin(Message), end(Message)), ExtraFlags);
      }
    }
  }

  return nullptr;
}

CIncomingMapSize* CGameProtocol::RECEIVE_W3GS_MAPSIZE(const std::vector<uint8_t>& data)
{
  // DEBUG_Print( "RECEIVED W3GS_MAPSIZE" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> ???
  // 1 byte           -> SizeFlag (1 = have map, 3 = continue download)
  // 4 bytes					-> MapSize

  if (ValidateLength(data) && data.size() >= 13)
    return new CIncomingMapSize(data[8], ByteArrayToUInt32(data, false, 9));

  return nullptr;
}

uint32_t CGameProtocol::RECEIVE_W3GS_PONG_TO_HOST(const std::vector<uint8_t>& data)
{
  // DEBUG_Print( "RECEIVED W3GS_PONG_TO_HOST" );
  // DEBUG_Print( data );

  // 2 bytes					-> Header
  // 2 bytes					-> Length
  // 4 bytes					-> Pong

  // the pong value is just a copy of whatever was sent in SEND_W3GS_PING_FROM_HOST which was GetTicks( ) at the time of sending
  // so as long as we trust that the client isn't trying to fake us out and mess with the pong value we can find the round trip time by simple subtraction
  // (the subtraction is done elsewhere because the very first pong value seems to be 1 and we want to discard that one)

  if (ValidateLength(data) && data.size() >= 8)
    return ByteArrayToUInt32(data, false, 4);

  return 1;
}

////////////////////
// SEND FUNCTIONS //
////////////////////

std::vector<uint8_t> CGameProtocol::SEND_W3GS_PING_FROM_HOST()
{
  std::vector<uint8_t> packet = {W3GS_HEADER_CONSTANT, W3GS_PING_FROM_HOST, 8, 0};
  AppendByteArray(packet, GetTicks(), false); // ping value
  return packet;
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_SLOTINFOJOIN(uint8_t PID, const std::vector<uint8_t>& port, const std::vector<uint8_t>& externalIP, const vector<CGameSlot>& slots, uint32_t randomSeed, uint8_t layoutStyle, uint8_t playerSlots)
{
  std::vector<uint8_t> packet;

  if (port.size() == 2 && externalIP.size() == 4)
  {
    const uint8_t              Zeros[]  = {0, 0, 0, 0};
    const std::vector<uint8_t> SlotInfo = EncodeSlotInfo(slots, randomSeed, layoutStyle, playerSlots);
    packet.push_back(W3GS_HEADER_CONSTANT);                    // W3GS header constant
    packet.push_back(W3GS_SLOTINFOJOIN);                       // W3GS_SLOTINFOJOIN
    packet.push_back(0);                                       // packet length will be assigned later
    packet.push_back(0);                                       // packet length will be assigned later
    AppendByteArray(packet, static_cast<uint16_t>(SlotInfo.size()), false); // SlotInfo length
    AppendByteArrayFast(packet, SlotInfo);                     // SlotInfo
    packet.push_back(PID);                                     // PID
    packet.push_back(2);                                       // AF_INET
    packet.push_back(0);                                       // AF_INET continued...
    AppendByteArray(packet, port);                             // port
    AppendByteArrayFast(packet, externalIP);                   // external IP
    AppendByteArray(packet, Zeros, 4);                         // ???
    AppendByteArray(packet, Zeros, 4);                         // ???
    AssignLength(packet);
  }
  else
    Print("[GAMEPROTO] invalid parameters passed to SEND_W3GS_SLOTINFOJOIN");

  return packet;
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_REJECTJOIN(uint32_t reason)
{
  std::vector<uint8_t> packet = {W3GS_HEADER_CONSTANT, W3GS_REJECTJOIN, 8, 0};
  AppendByteArray(packet, reason, false); // reason
  return packet;
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_PLAYERINFO(uint8_t PID, const string& name, const std::vector<uint8_t>& externalIP, const std::vector<uint8_t>& internalIP)
{
  std::vector<uint8_t> packet;

  if (!name.empty() && name.size() <= 15 && externalIP.size() == 4 && internalIP.size() == 4)
  {
    const uint8_t PlayerJoinCounter[] = {2, 0, 0, 0};
    const uint8_t Zeros[]             = {0, 0, 0, 0};

    packet.push_back(W3GS_HEADER_CONSTANT);        // W3GS header constant
    packet.push_back(W3GS_PLAYERINFO);             // W3GS_PLAYERINFO
    packet.push_back(0);                           // packet length will be assigned later
    packet.push_back(0);                           // packet length will be assigned later
    AppendByteArray(packet, PlayerJoinCounter, 4); // player join counter
    packet.push_back(PID);                         // PID
    AppendByteArrayFast(packet, name);             // player name
    packet.push_back(1);                           // ???
    packet.push_back(0);                           // ???
    packet.push_back(2);                           // AF_INET
    packet.push_back(0);                           // AF_INET continued...
    packet.push_back(0);                           // port
    packet.push_back(0);                           // port continued...
    AppendByteArrayFast(packet, externalIP);       // external IP
    AppendByteArray(packet, Zeros, 4);             // ???
    AppendByteArray(packet, Zeros, 4);             // ???
    packet.push_back(2);                           // AF_INET
    packet.push_back(0);                           // AF_INET continued...
    packet.push_back(0);                           // port
    packet.push_back(0);                           // port continued...
    AppendByteArrayFast(packet, internalIP);       // internal IP
    AppendByteArray(packet, Zeros, 4);             // ???
    AppendByteArray(packet, Zeros, 4);             // ???
    AssignLength(packet);
  }
  else
    Print("[GAMEPROTO] invalid parameters passed to SEND_W3GS_PLAYERINFO");

  return packet;
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_PLAYERLEAVE_OTHERS(uint8_t PID, uint32_t leftCode)
{
  if (PID != 255)
  {
    std::vector<uint8_t> packet = {W3GS_HEADER_CONSTANT, W3GS_PLAYERLEAVE_OTHERS, 9, 0, PID};
    AppendByteArray(packet, leftCode, false); // left code (see PLAYERLEAVE_ constants in gameprotocol.h)
    return packet;
  }

  Print("[GAMEPROTO] invalid parameters passed to SEND_W3GS_PLAYERLEAVE_OTHERS");
  return std::vector<uint8_t>();
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_GAMELOADED_OTHERS(uint8_t PID)
{
  if (PID != 255)
    return std::vector<uint8_t>{W3GS_HEADER_CONSTANT, W3GS_GAMELOADED_OTHERS, 5, 0, PID};

  Print("[GAMEPROTO] invalid parameters passed to SEND_W3GS_GAMELOADED_OTHERS");

  return std::vector<uint8_t>();
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_SLOTINFO(vector<CGameSlot>& slots, uint32_t randomSeed, uint8_t layoutStyle, uint8_t playerSlots)
{
  const std::vector<uint8_t> SlotInfo     = EncodeSlotInfo(slots, randomSeed, layoutStyle, playerSlots);
  const uint16_t             SlotInfoSize = static_cast<uint16_t>(SlotInfo.size());

  std::vector<uint8_t> packet = {W3GS_HEADER_CONSTANT, W3GS_SLOTINFO, 0, 0};
  AppendByteArray(packet, SlotInfoSize, false); // SlotInfo length
  AppendByteArrayFast(packet, SlotInfo);        // SlotInfo
  AssignLength(packet);
  return packet;
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_COUNTDOWN_START()
{
  return std::vector<uint8_t>{W3GS_HEADER_CONSTANT, W3GS_COUNTDOWN_START, 4, 0};
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_COUNTDOWN_END()
{
  return std::vector<uint8_t>{W3GS_HEADER_CONSTANT, W3GS_COUNTDOWN_END, 4, 0};
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_INCOMING_ACTION(queue<CIncomingAction*> actions, uint16_t sendInterval)
{
  std::vector<uint8_t> packet = {W3GS_HEADER_CONSTANT, W3GS_INCOMING_ACTION, 0, 0};
  AppendByteArray(packet, sendInterval, false); // send int32_terval

  // create subpacket

  if (!actions.empty())
  {
    std::vector<uint8_t> subpacket;

    do
    {
      CIncomingAction* Action = actions.front();
      actions.pop();
      subpacket.push_back(Action->GetPID());
      AppendByteArray(subpacket, static_cast<uint16_t>(Action->GetAction()->size()), false);
      AppendByteArrayFast(subpacket, *Action->GetAction());
    } while (!actions.empty());

    // calculate crc (we only care about the first 2 bytes though)

    std::vector<uint8_t> crc32 = CreateByteArray(m_Aura->m_CRC->CalculateCRC((uint8_t*)string(begin(subpacket), end(subpacket)).c_str(), subpacket.size()), false);
    crc32.resize(2);

    // finish subpacket

    AppendByteArrayFast(packet, crc32);     // crc
    AppendByteArrayFast(packet, subpacket); // subpacket
  }

  AssignLength(packet);
  return packet;
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_CHAT_FROM_HOST(uint8_t fromPID, const std::vector<uint8_t>& toPIDs, uint8_t flag, const std::vector<uint8_t>& flagExtra, const string& message)
{
  if (!toPIDs.empty() && !message.empty() && message.size() < 255)
  {
    std::vector<uint8_t> packet = {W3GS_HEADER_CONSTANT, W3GS_CHAT_FROM_HOST, 0, 0, static_cast<uint8_t>(toPIDs.size())};
    AppendByteArrayFast(packet, toPIDs);    // receivers
    packet.push_back(fromPID);              // sender
    packet.push_back(flag);                 // flag
    AppendByteArrayFast(packet, flagExtra); // extra flag
    AppendByteArrayFast(packet, message);   // message
    AssignLength(packet);
    return packet;
  }

  Print("[GAMEPROTO] invalid parameters passed to SEND_W3GS_CHAT_FROM_HOST");
  return std::vector<uint8_t>();
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_START_LAG(vector<CGamePlayer*> players)
{
  uint8_t NumLaggers = 0;

  for (auto& player : players)
  {
    if ((player)->GetLagging())
      ++NumLaggers;
  }

  if (NumLaggers > 0)
  {
    std::vector<uint8_t> packet = {W3GS_HEADER_CONSTANT, W3GS_START_LAG, 0, 0, NumLaggers};

    for (auto& player : players)
    {
      if ((player)->GetLagging())
      {
        packet.push_back((player)->GetPID());
        AppendByteArray(packet, GetTicks() - (player)->GetStartedLaggingTicks(), false);
      }
    }

    AssignLength(packet);
    return packet;
  }

  Print("[GAMEPROTO] no laggers passed to SEND_W3GS_START_LAG");
  return std::vector<uint8_t>();
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_STOP_LAG(CGamePlayer* player)
{
  std::vector<uint8_t> packet = {W3GS_HEADER_CONSTANT, W3GS_STOP_LAG, 9, 0, player->GetPID()};
  AppendByteArray(packet, GetTicks() - player->GetStartedLaggingTicks(), false);
  return packet;
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_GAMEINFO(uint8_t war3Version, const std::vector<uint8_t>& mapGameType, const std::vector<uint8_t>& mapFlags, const std::vector<uint8_t>& mapWidth, const std::vector<uint8_t>& mapHeight, const string& gameName, const string& hostName, uint32_t upTime, const string& mapPath, const std::vector<uint8_t>& mapCRC, uint32_t slotsTotal, uint32_t slotsOpen, uint16_t port, uint32_t hostCounter, uint32_t entryKey)
{
  if (mapGameType.size() == 4 && mapFlags.size() == 4 && mapWidth.size() == 2 && mapHeight.size() == 2 && !gameName.empty() && !hostName.empty() && !mapPath.empty() && mapCRC.size() == 4)
  {
    const uint8_t Unknown2[] = {1, 0, 0, 0};

    // make the stat string

    std::vector<uint8_t> StatString;
    AppendByteArrayFast(StatString, mapFlags);
    StatString.push_back(0);
    AppendByteArrayFast(StatString, mapWidth);
    AppendByteArrayFast(StatString, mapHeight);
    AppendByteArrayFast(StatString, mapCRC);
    AppendByteArrayFast(StatString, mapPath);
    AppendByteArrayFast(StatString, hostName);
    StatString.push_back(0);
    StatString = EncodeStatString(StatString);

    // make the rest of the packet

    std::vector<uint8_t> packet = {W3GS_HEADER_CONSTANT, W3GS_GAMEINFO, 0, 0, 80, 88, 51, 87, war3Version, 0, 0, 0};
    AppendByteArray(packet, hostCounter, false); // Host Counter
    AppendByteArray(packet, entryKey, false);    // Entry Key
    AppendByteArrayFast(packet, gameName);       // Game Name
    packet.push_back(0);                         // ??? (maybe game password)
    AppendByteArrayFast(packet, StatString);     // Stat String
    packet.push_back(0);                         // Stat String null terminator (the stat string is encoded to remove all even numbers i.e. zeros)
    AppendByteArray(packet, slotsTotal, false);  // Slots Total
    AppendByteArrayFast(packet, mapGameType);    // Game Type
    AppendByteArray(packet, Unknown2, 4);        // ???
    AppendByteArray(packet, slotsOpen, false);   // Slots Open
    AppendByteArray(packet, upTime, false);      // time since creation
    AppendByteArray(packet, port, false);        // port
    AssignLength(packet);
    return packet;
  }

  Print("[GAMEPROTO] invalid parameters passed to SEND_W3GS_GAMEINFO");
  return std::vector<uint8_t>();
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_CREATEGAME(uint8_t war3Version)
{
  return std::vector<uint8_t>{W3GS_HEADER_CONSTANT, W3GS_CREATEGAME, 16, 0, 80, 88, 51, 87, war3Version, 0, 0, 0, 1, 0, 0, 0};
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_REFRESHGAME(uint32_t players, uint32_t playerSlots)
{
  std::vector<uint8_t> packet = {W3GS_HEADER_CONSTANT, W3GS_REFRESHGAME, 16, 0, 1, 0, 0, 0};
  AppendByteArray(packet, players, false);     // Players
  AppendByteArray(packet, playerSlots, false); // Player Slots
  return packet;
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_DECREATEGAME()
{
  return std::vector<uint8_t>{W3GS_HEADER_CONSTANT, W3GS_DECREATEGAME, 8, 0, 1, 0, 0, 0};
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_MAPCHECK(const string& mapPath, const std::vector<uint8_t>& mapSize, const std::vector<uint8_t>& mapInfo, const std::vector<uint8_t>& mapCRC, const std::vector<uint8_t>& mapSHA1)
{
  if (!mapPath.empty() && mapSize.size() == 4 && mapInfo.size() == 4 && mapCRC.size() == 4 && mapSHA1.size() == 20)
  {
    std::vector<uint8_t> packet = {W3GS_HEADER_CONSTANT, W3GS_MAPCHECK, 0, 0, 1, 0, 0, 0};
    AppendByteArrayFast(packet, mapPath); // map path
    AppendByteArrayFast(packet, mapSize); // map size
    AppendByteArrayFast(packet, mapInfo); // map info
    AppendByteArrayFast(packet, mapCRC);  // map crc
    AppendByteArrayFast(packet, mapSHA1); // map sha1
    AssignLength(packet);
    return packet;
  }

  Print("[GAMEPROTO] invalid parameters passed to SEND_W3GS_MAPCHECK");
  return std::vector<uint8_t>();
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_STARTDOWNLOAD(uint8_t fromPID)
{
  return std::vector<uint8_t>{W3GS_HEADER_CONSTANT, W3GS_STARTDOWNLOAD, 9, 0, 1, 0, 0, 0, fromPID};
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_MAPPART(uint8_t fromPID, uint8_t toPID, uint32_t start, const string* mapData)
{
  if (start < mapData->size())
  {
    std::vector<uint8_t> packet = {W3GS_HEADER_CONSTANT, W3GS_MAPPART, 0, 0, toPID, fromPID, 1, 0, 0, 0};
    AppendByteArray(packet, start, false); // start position

    // calculate end position (don't send more than 1442 map bytes in one packet)

    uint32_t End = start + 1442;

    if (End > mapData->size())
      End = mapData->size();

    // calculate crc

    const std::vector<uint8_t> crc32 = CreateByteArray(m_Aura->m_CRC->CalculateCRC((uint8_t*)mapData->c_str() + start, End - start), false);
    AppendByteArrayFast(packet, crc32);

    // map data

    const std::vector<uint8_t> data = CreateByteArray((uint8_t*)mapData->c_str() + start, End - start);
    AppendByteArrayFast(packet, data);
    AssignLength(packet);
    return packet;
  }

  Print("[GAMEPROTO] invalid parameters passed to SEND_W3GS_MAPPART");
  return std::vector<uint8_t>();
}

std::vector<uint8_t> CGameProtocol::SEND_W3GS_INCOMING_ACTION2(queue<CIncomingAction*> actions)
{
  std::vector<uint8_t> packet = {W3GS_HEADER_CONSTANT, W3GS_INCOMING_ACTION2, 0, 0, 0, 0};

  // create subpacket

  if (!actions.empty())
  {
    std::vector<uint8_t> subpacket;

    while (!actions.empty())
    {
      CIncomingAction* Action = actions.front();
      actions.pop();
      subpacket.push_back(Action->GetPID());
      AppendByteArray(subpacket, static_cast<uint16_t>(Action->GetAction()->size()), false);
      AppendByteArrayFast(subpacket, *Action->GetAction());
    }

    // calculate crc (we only care about the first 2 bytes though)

    std::vector<uint8_t> crc32 = CreateByteArray(m_Aura->m_CRC->CalculateCRC((uint8_t*)string(begin(subpacket), end(subpacket)).c_str(), subpacket.size()), false);
    crc32.resize(2);

    // finish subpacket

    AppendByteArrayFast(packet, crc32);     // crc
    AppendByteArrayFast(packet, subpacket); // subpacket
  }

  AssignLength(packet);
  return packet;
}

/////////////////////
// OTHER FUNCTIONS //
/////////////////////

bool CGameProtocol::ValidateLength(const std::vector<uint8_t>& content)
{
  // verify that bytes 3 and 4 (indices 2 and 3) of the content array describe the length

  return (static_cast<uint16_t>(content[3] << 8 | content[2]) == content.size());
}

std::vector<uint8_t> CGameProtocol::EncodeSlotInfo(const vector<CGameSlot>& slots, uint32_t randomSeed, uint8_t layoutStyle, uint8_t playerSlots)
{
  std::vector<uint8_t> SlotInfo;
  SlotInfo.push_back(static_cast<uint8_t>(slots.size())); // number of slots

  for (auto& slot : slots)
    AppendByteArray(SlotInfo, slot.GetByteArray());

  AppendByteArray(SlotInfo, randomSeed, false); // random seed
  SlotInfo.push_back(layoutStyle);              // LayoutStyle (0 = melee, 1 = custom forces, 3 = custom forces + fixed player settings)
  SlotInfo.push_back(playerSlots);              // number of player slots (non observer)
  return SlotInfo;
}

//
// CIncomingJoinPlayer
//

CIncomingJoinPlayer::CIncomingJoinPlayer(uint32_t nHostCounter, uint32_t nEntryKey, string nName, std::vector<uint8_t> nInternalIP)
  : m_Name(std::move(nName)),
    m_InternalIP(std::move(nInternalIP)),
    m_HostCounter(nHostCounter),
    m_EntryKey(nEntryKey)
{
}

CIncomingJoinPlayer::~CIncomingJoinPlayer() = default;

//
// CIncomingAction
//

CIncomingAction::CIncomingAction(uint8_t nPID, std::vector<uint8_t> nCRC, std::vector<uint8_t> nAction)
  : m_CRC(std::move(nCRC)),
    m_Action(std::move(nAction)),
    m_PID(nPID)
{
}

CIncomingAction::~CIncomingAction() = default;

//
// CIncomingChatPlayer
//

CIncomingChatPlayer::CIncomingChatPlayer(uint8_t nFromPID, std::vector<uint8_t> nToPIDs, uint8_t nFlag, string nMessage)
  : m_Message(std::move(nMessage)),
    m_ToPIDs(std::move(nToPIDs)),
    m_Type(CTH_MESSAGE),
    m_FromPID(nFromPID),
    m_Flag(nFlag),
    m_Byte(255)
{
}

CIncomingChatPlayer::CIncomingChatPlayer(uint8_t nFromPID, std::vector<uint8_t> nToPIDs, uint8_t nFlag, string nMessage, std::vector<uint8_t> nExtraFlags)
  : m_Message(std::move(nMessage)),
    m_ToPIDs(std::move(nToPIDs)),
    m_ExtraFlags(std::move(nExtraFlags)),
    m_Type(CTH_MESSAGE),
    m_FromPID(nFromPID),
    m_Flag(nFlag),
    m_Byte(255)
{
}

CIncomingChatPlayer::CIncomingChatPlayer(uint8_t nFromPID, std::vector<uint8_t> nToPIDs, uint8_t nFlag, uint8_t nByte)
  : m_ToPIDs(std::move(nToPIDs)),
    m_FromPID(nFromPID),
    m_Flag(nFlag),
    m_Byte(nByte)
{
  if (nFlag == 17)
    m_Type = CTH_TEAMCHANGE;
  else if (nFlag == 18)
    m_Type = CTH_COLOURCHANGE;
  else if (nFlag == 19)
    m_Type = CTH_RACECHANGE;
  else if (nFlag == 20)
    m_Type = CTH_HANDICAPCHANGE;
}

CIncomingChatPlayer::~CIncomingChatPlayer() = default;

//
// CIncomingMapSize
//

CIncomingMapSize::CIncomingMapSize(uint8_t nSizeFlag, uint32_t nMapSize)
  : m_MapSize(nMapSize),
    m_SizeFlag(nSizeFlag)
{
}

CIncomingMapSize::~CIncomingMapSize() = default;
