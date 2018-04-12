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

#include "bnet.h"
#include "aura.h"
#include "util.h"
#include "fileutil.h"
#include "config.h"
#include "socket.h"
#include "auradb.h"
#include "bncsutilinterface.h"
#include "bnetprotocol.h"
#include "map.h"
#include "gameprotocol.h"
#include "game.h"
#include "irc.h"
#include "includes.h"
#include "hash.h"

#include <algorithm>

using namespace std;

//
// CBNET
//

CBNET::CBNET(CAura* nAura, string nServer, string nServerAlias, string nCDKeyROC, string nCDKeyTFT, string nCountryAbbrev, string nCountry, uint32_t nLocaleID, string nUserName, string nUserPassword, string nFirstChannel, char nCommandTrigger, uint8_t nWar3Version, std::vector<uint8_t> nEXEVersion, std::vector<uint8_t> nEXEVersionHash, string nPasswordHashType, uint32_t nHostCounterID)
  : m_Aura(nAura),
    m_Socket(new CTCPClient()),
    m_Protocol(new CBNETProtocol()),
    m_BNCSUtil(new CBNCSUtilInterface(nUserName, nUserPassword)),
    m_EXEVersion(move(nEXEVersion)),
    m_EXEVersionHash(move(nEXEVersionHash)),
    m_Server(move(nServer)),
    m_CDKeyROC(nCDKeyROC),
    m_CDKeyTFT(nCDKeyTFT),
    m_CountryAbbrev(move(nCountryAbbrev)),
    m_Country(move(nCountry)),
    m_UserName(nUserName),
    m_UserPassword(nUserPassword),
    m_FirstChannel(move(nFirstChannel)),
    m_PasswordHashType(move(nPasswordHashType)),
    m_LastDisconnectedTime(0),
    m_LastConnectionAttemptTime(0),
    m_LastNullTime(0),
    m_LastOutPacketTicks(0),
    m_LastAdminRefreshTime(GetTime()),
    m_LastBanRefreshTime(GetTime()),
    m_LastOutPacketSize(0),
    m_LocaleID(nLocaleID),
    m_HostCounterID(nHostCounterID),
    m_War3Version(nWar3Version),
    m_CommandTrigger(nCommandTrigger),
    m_Exiting(false),
    m_FirstConnect(true),
    m_WaitingToConnect(true),
    m_LoggedIn(false),
    m_InChat(false)
{
  if (m_PasswordHashType == "pvpgn" || m_EXEVersion.size() == 4 || m_EXEVersionHash.size() == 4)
  {
    m_PvPGN          = true;
    m_ReconnectDelay = 90;
  }
  else
  {
    m_PvPGN          = false;
    m_ReconnectDelay = 240;
  }

  if (!nServerAlias.empty())
    m_ServerAlias = nServerAlias;
  else
    m_ServerAlias = m_Server;

  m_CDKeyROC = nCDKeyROC;
  m_CDKeyTFT = nCDKeyTFT;

  // remove dashes and spaces from CD keys and convert to uppercase

  m_CDKeyROC.erase(remove(begin(m_CDKeyROC), end(m_CDKeyROC), '-'), end(m_CDKeyROC));
  m_CDKeyTFT.erase(remove(begin(m_CDKeyTFT), end(m_CDKeyTFT), '-'), end(m_CDKeyTFT));
  m_CDKeyROC.erase(remove(begin(m_CDKeyROC), end(m_CDKeyROC), ' '), end(m_CDKeyROC));
  m_CDKeyTFT.erase(remove(begin(m_CDKeyTFT), end(m_CDKeyTFT), ' '), end(m_CDKeyTFT));
  transform(begin(m_CDKeyROC), end(m_CDKeyROC), begin(m_CDKeyROC), ::toupper);
  transform(begin(m_CDKeyTFT), end(m_CDKeyTFT), begin(m_CDKeyTFT), ::toupper);

  if (m_CDKeyROC.size() != 26)
    Print("[BNET: " + m_ServerAlias + "] warning - your ROC CD key is not 26 characters long and is probably invalid");

  if (m_CDKeyTFT.size() != 26)
    Print("[BNET: " + m_ServerAlias + "] warning - your TFT CD key is not 26 characters long and is probably invalid");
}

CBNET::~CBNET()
{
  delete m_Socket;
  delete m_Protocol;
  delete m_BNCSUtil;
}

uint32_t CBNET::SetFD(void* fd, void* send_fd, int32_t* nfds)
{
  if (!m_Socket->HasError() && m_Socket->GetConnected())
  {
    m_Socket->SetFD(static_cast<fd_set*>(fd), static_cast<fd_set*>(send_fd), nfds);
    return 1;
  }

  return 0;
}

bool CBNET::Update(void* fd, void* send_fd)
{
  const int64_t Ticks = GetTicks(), Time = GetTime();

  // we return at the end of each if statement so we don't have to deal with errors related to the order of the if statements
  // that means it might take a few ms longer to complete a task involving multiple steps (in this case, reconnecting) due to blocking or sleeping
  // but it's not a big deal at all, maybe 100ms in the worst possible case (based on a 50ms blocking time)

  if (m_Socket->HasError())
  {
    // the socket has an error

    Print2("[BNET: " + m_ServerAlias + "] disconnected from battle.net due to socket error");
    Print("[BNET: " + m_ServerAlias + "] waiting " + to_string(m_ReconnectDelay) + " seconds to reconnect");
    m_BNCSUtil->Reset(m_UserName, m_UserPassword);
    m_Socket->Reset();
    m_LastDisconnectedTime = Time;
    m_LoggedIn             = false;
    m_InChat               = false;
    m_WaitingToConnect     = true;
    return m_Exiting;
  }

  if (m_Socket->GetConnected())
  {
    // the socket is connected and everything appears to be working properly

    m_Socket->DoRecv(static_cast<fd_set*>(fd));

    // extract as many packets as possible from the socket's receive buffer and put them in the m_Packets queue

    string*              RecvBuffer      = m_Socket->GetBytes();
    std::vector<uint8_t> Bytes           = CreateByteArray((uint8_t*)RecvBuffer->c_str(), RecvBuffer->size());
    uint32_t             LengthProcessed = 0;

    CIncomingGameHost*  GameHost;
    CIncomingChatEvent* ChatEvent;

    // a packet is at least 4 bytes so loop as long as the buffer contains 4 bytes

    while (Bytes.size() >= 4)
    {
      // byte 0 is always 255

      if (Bytes[0] == BNET_HEADER_CONSTANT)
      {
        // bytes 2 and 3 contain the length of the packet

        const uint16_t             Length = static_cast<uint16_t>(Bytes[3] << 8 | Bytes[2]);
        const std::vector<uint8_t> Data   = std::vector<uint8_t>(begin(Bytes), begin(Bytes) + Length);

        if (Bytes.size() >= Length)
        {
          switch (Bytes[1])
          {
            case CBNETProtocol::SID_NULL:
              // warning: we do not respond to NULL packets with a NULL packet of our own
              // this is because PVPGN servers are programmed to respond to NULL packets so it will create a vicious cycle of useless traffic
              // official battle.net servers do not respond to NULL packets

              m_Protocol->RECEIVE_SID_NULL(Data);
              break;

            case CBNETProtocol::SID_GETADVLISTEX:
              GameHost = m_Protocol->RECEIVE_SID_GETADVLISTEX(Data);

              if (GameHost)
                Print("[BNET: " + m_ServerAlias + "] joining game [" + GameHost->GetGameName() + "]");

              delete GameHost;
              break;

            case CBNETProtocol::SID_ENTERCHAT:
              if (m_Protocol->RECEIVE_SID_ENTERCHAT(Data))
              {
                Print("[BNET: " + m_ServerAlias + "] joining channel [" + m_FirstChannel + "]");
                m_InChat = true;
                m_Socket->PutBytes(m_Protocol->SEND_SID_JOINCHANNEL(m_FirstChannel));
              }

              break;

            case CBNETProtocol::SID_CHATEVENT:
              ChatEvent = m_Protocol->RECEIVE_SID_CHATEVENT(Data);

              if (ChatEvent)
                ProcessChatEvent(ChatEvent);

              delete ChatEvent;
              break;

            case CBNETProtocol::SID_CHECKAD:
              m_Protocol->RECEIVE_SID_CHECKAD(Data);
              break;

            case CBNETProtocol::SID_STARTADVEX3:
              if (m_Protocol->RECEIVE_SID_STARTADVEX3(Data))
              {
                m_InChat = false;
              }
              else
              {
                Print("[BNET: " + m_ServerAlias + "] startadvex3 failed");
                m_Aura->EventBNETGameRefreshFailed(this);
              }

              break;

            case CBNETProtocol::SID_PING:
              m_Socket->PutBytes(m_Protocol->SEND_SID_PING(m_Protocol->RECEIVE_SID_PING(Data)));
              break;

            case CBNETProtocol::SID_AUTH_INFO:

              if (m_Protocol->RECEIVE_SID_AUTH_INFO(Data))
              {
                if (m_BNCSUtil->HELP_SID_AUTH_CHECK(m_Aura->m_Warcraft3Path, m_CDKeyROC, m_CDKeyTFT, m_Protocol->GetValueStringFormulaString(), m_Protocol->GetIX86VerFileNameString(), m_Protocol->GetClientToken(), m_Protocol->GetServerToken(), m_War3Version))
                {
                  // override the exe information generated by bncsutil if specified in the config file
                  // apparently this is useful for pvpgn users

                  if (m_EXEVersion.size() == 4)
                  {
                    Print("[BNET: " + m_ServerAlias + "] using custom exe version bnet_custom_exeversion = " + to_string(m_EXEVersion[0]) + " " + to_string(m_EXEVersion[1]) + " " + to_string(m_EXEVersion[2]) + " " + to_string(m_EXEVersion[3]));
                    m_BNCSUtil->SetEXEVersion(m_EXEVersion);
                  }

                  if (m_EXEVersionHash.size() == 4)
                  {
                    Print("[BNET: " + m_ServerAlias + "] using custom exe version hash bnet_custom_exeversionhash = " + to_string(m_EXEVersionHash[0]) + " " + to_string(m_EXEVersionHash[1]) + " " + to_string(m_EXEVersionHash[2]) + " " + to_string(m_EXEVersionHash[3]));
                    m_BNCSUtil->SetEXEVersionHash(m_EXEVersionHash);
                  }

                  Print("[BNET: " + m_ServerAlias + "] attempting to auth as Warcraft III: The Frozen Throne");

                  m_Socket->PutBytes(m_Protocol->SEND_SID_AUTH_CHECK(m_Protocol->GetClientToken(), m_BNCSUtil->GetEXEVersion(), m_BNCSUtil->GetEXEVersionHash(), m_BNCSUtil->GetKeyInfoROC(), m_BNCSUtil->GetKeyInfoTFT(), m_BNCSUtil->GetEXEInfo(), "Aura"));
                }
                else
                {
                  Print("[BNET: " + m_ServerAlias + "] logon failed - bncsutil key hash failed (check your Warcraft 3 path and cd keys), disconnecting");
                  m_Socket->Disconnect();
                }
              }

              break;

            case CBNETProtocol::SID_AUTH_CHECK:
              if (m_Protocol->RECEIVE_SID_AUTH_CHECK(Data))
              {
                // cd keys accepted

                Print("[BNET: " + m_ServerAlias + "] cd keys accepted");
                m_BNCSUtil->HELP_SID_AUTH_ACCOUNTLOGON();
                m_Socket->PutBytes(m_Protocol->SEND_SID_AUTH_ACCOUNTLOGON(m_BNCSUtil->GetClientKey(), m_UserName));
              }
              else
              {
                // cd keys not accepted

                switch (ByteArrayToUInt32(m_Protocol->GetKeyState(), false))
                {
                  case CBNETProtocol::KR_ROC_KEY_IN_USE:
                    Print("[BNET: " + m_ServerAlias + "] logon failed - ROC CD key in use by user [" + m_Protocol->GetKeyStateDescription() + "], disconnecting");
                    break;

                  case CBNETProtocol::KR_TFT_KEY_IN_USE:
                    Print("[BNET: " + m_ServerAlias + "] logon failed - TFT CD key in use by user [" + m_Protocol->GetKeyStateDescription() + "], disconnecting");
                    break;

                  case CBNETProtocol::KR_OLD_GAME_VERSION:
                    Print("[BNET: " + m_ServerAlias + "] logon failed - game version is too old, disconnecting");
                    break;

                  case CBNETProtocol::KR_INVALID_VERSION:
                    Print("[BNET: " + m_ServerAlias + "] logon failed - game version is invalid, disconnecting");
                    break;

                  default:
                    Print("[BNET: " + m_ServerAlias + "] logon failed - cd keys not accepted, disconnecting");
                    break;
                }

                m_Socket->Disconnect();
              }

              break;

            case CBNETProtocol::SID_AUTH_ACCOUNTLOGON:
              if (m_Protocol->RECEIVE_SID_AUTH_ACCOUNTLOGON(Data))
              {
                Print("[BNET: " + m_ServerAlias + "] username [" + m_UserName + "] accepted");

                if (m_PasswordHashType == "pvpgn")
                {
                  // pvpgn logon

                  Print("[BNET: " + m_ServerAlias + "] using pvpgn logon type (for pvpgn servers only)");
                  m_BNCSUtil->HELP_PvPGNPasswordHash(m_UserPassword);
                  m_Socket->PutBytes(m_Protocol->SEND_SID_AUTH_ACCOUNTLOGONPROOF(m_BNCSUtil->GetPvPGNPasswordHash()));
                }
                else
                {
                  // battle.net logon

                  Print("[BNET: " + m_ServerAlias + "] using battle.net logon type (for official battle.net servers only)");
                  m_BNCSUtil->HELP_SID_AUTH_ACCOUNTLOGONPROOF(m_Protocol->GetSalt(), m_Protocol->GetServerPublicKey());
                  m_Socket->PutBytes(m_Protocol->SEND_SID_AUTH_ACCOUNTLOGONPROOF(m_BNCSUtil->GetM1()));
                }
              }
              else
              {
                Print("[BNET: " + m_ServerAlias + "] logon failed - invalid username, disconnecting");
                m_Socket->Disconnect();
              }

              break;

            case CBNETProtocol::SID_AUTH_ACCOUNTLOGONPROOF:
              if (m_Protocol->RECEIVE_SID_AUTH_ACCOUNTLOGONPROOF(Data))
              {
                // logon successful

                Print("[BNET: " + m_ServerAlias + "] logon successful");
                m_LoggedIn = true;
                m_Socket->PutBytes(m_Protocol->SEND_SID_NETGAMEPORT(m_Aura->m_HostPort));
                m_Socket->PutBytes(m_Protocol->SEND_SID_ENTERCHAT());
                m_Socket->PutBytes(m_Protocol->SEND_SID_FRIENDLIST());
                m_Socket->PutBytes(m_Protocol->SEND_SID_CLANMEMBERLIST());
              }
              else
              {
                Print("[BNET: " + m_ServerAlias + "] logon failed - invalid password, disconnecting");

                // try to figure out if the user might be using the wrong logon type since too many people are confused by this

                string Server = m_Server;
                transform(begin(Server), end(Server), begin(Server), ::tolower);

                if (m_PvPGN && (Server == "useast.battle.net" || Server == "uswest.battle.net" || Server == "asia.battle.net" || Server == "europe.battle.net"))
                  Print("[BNET: " + m_ServerAlias + R"(] it looks like you're trying to connect to a battle.net server using a pvpgn logon type, check your config file's "battle.net custom data" section)");
                else if (!m_PvPGN && (Server != "useast.battle.net" && Server != "uswest.battle.net" && Server != "asia.battle.net" && Server != "europe.battle.net"))
                  Print("[BNET: " + m_ServerAlias + R"(] it looks like you're trying to connect to a pvpgn server using a battle.net logon type, check your config file's "battle.net custom data" section)");

                m_Socket->Disconnect();
              }

              break;

            case CBNETProtocol::SID_FRIENDLIST:
              m_Friends = m_Protocol->RECEIVE_SID_FRIENDLIST(Data);
              break;

            case CBNETProtocol::SID_CLANMEMBERLIST:
              m_Clan = m_Protocol->RECEIVE_SID_CLANMEMBERLIST(Data);
              break;
          }

          LengthProcessed += Length;
          Bytes = std::vector<uint8_t>(begin(Bytes) + Length, end(Bytes));
        }
        else
          break;
      }
    }

    *RecvBuffer = RecvBuffer->substr(LengthProcessed);

    // check if at least one packet is waiting to be sent and if we've waited long enough to prevent flooding
    // this formula has changed many times but currently we wait 1 second if the last packet was "small", 3.5 seconds if it was "medium", and 4 seconds if it was "big"

    int64_t WaitTicks;

    if (m_LastOutPacketSize < 10)
      WaitTicks = 1300;
    else if (m_LastOutPacketSize < 100)
      WaitTicks = 3300;
    else
      WaitTicks = 4300;

    if (!m_OutPackets.empty() && Ticks - m_LastOutPacketTicks >= WaitTicks)
    {
      if (m_OutPackets.size() > 7)
        Print("[BNET: " + m_ServerAlias + "] packet queue warning - there are " + to_string(m_OutPackets.size()) + " packets waiting to be sent");

      m_Socket->PutBytes(m_OutPackets.front());
      m_LastOutPacketSize = m_OutPackets.front().size();
      m_OutPackets.pop();
      m_LastOutPacketTicks = Ticks;
    }

    // send a null packet every 60 seconds to detect disconnects

    if (Time - m_LastNullTime >= 60 && Ticks - m_LastOutPacketTicks >= 60000)
    {
      m_Socket->PutBytes(m_Protocol->SEND_SID_NULL());
      m_LastNullTime = Time;
    }

    m_Socket->DoSend(static_cast<fd_set*>(send_fd));
    return m_Exiting;
  }

  if (!m_Socket->GetConnected() && !m_Socket->GetConnecting() && !m_WaitingToConnect)
  {
    // the socket was disconnected

    Print2("[BNET: " + m_ServerAlias + "] disconnected from battle.net");
    m_LastDisconnectedTime = Time;
    m_BNCSUtil->Reset(m_UserName, m_UserPassword);
    m_Socket->Reset();
    m_LoggedIn         = false;
    m_InChat           = false;
    m_WaitingToConnect = true;
    return m_Exiting;
  }

  if (!m_Socket->GetConnecting() && !m_Socket->GetConnected() && (m_FirstConnect || (Time - m_LastDisconnectedTime >= m_ReconnectDelay)))
  {
    // attempt to connect to battle.net

    m_FirstConnect = false;
    Print("[BNET: " + m_ServerAlias + "] connecting to server [" + m_Server + "] on port 6112");

    if (!m_Aura->m_BindAddress.empty())
      Print("[BNET: " + m_ServerAlias + "] attempting to bind to address [" + m_Aura->m_BindAddress + "]");

    if (m_ServerIP.empty())
    {
      m_Socket->Connect(m_Aura->m_BindAddress, m_Server, 6112);

      if (!m_Socket->HasError())
      {
        m_ServerIP = m_Socket->GetIPString();
        Print("[BNET: " + m_ServerAlias + "] resolved and cached server IP address " + m_ServerIP);
      }
    }
    else
    {
      // use cached server IP address since resolving takes time and is blocking

      Print("[BNET: " + m_ServerAlias + "] using cached server IP address " + m_ServerIP);
      m_Socket->Connect(m_Aura->m_BindAddress, m_ServerIP, 6112);
    }

    m_WaitingToConnect          = false;
    m_LastConnectionAttemptTime = Time;
  }

  if (m_Socket->GetConnecting())
  {
    // we are currently attempting to connect to battle.net

    if (m_Socket->CheckConnect())
    {
      // the connection attempt completed

      Print2("[BNET: " + m_ServerAlias + "] connected");
      m_Socket->PutBytes(m_Protocol->SEND_PROTOCOL_INITIALIZE_SELECTOR());
      m_Socket->PutBytes(m_Protocol->SEND_SID_AUTH_INFO(m_War3Version, m_LocaleID, m_CountryAbbrev, m_Country));
      m_Socket->DoSend(static_cast<fd_set*>(send_fd));
      m_LastNullTime       = Time;
      m_LastOutPacketTicks = Ticks;

      while (!m_OutPackets.empty())
        m_OutPackets.pop();

      return m_Exiting;
    }
    else if (Time - m_LastConnectionAttemptTime >= 15)
    {
      // the connection attempt timed out (15 seconds)

      Print("[BNET: " + m_ServerAlias + "] connect timed out");
      Print("[BNET: " + m_ServerAlias + "] waiting 90 seconds to reconnect");
      m_Socket->Reset();
      m_LastDisconnectedTime = Time;
      m_WaitingToConnect     = true;
      return m_Exiting;
    }
  }

  return m_Exiting;
}

void CBNET::ProcessChatEvent(const CIncomingChatEvent* chatEvent)
{
  CBNETProtocol::IncomingChatEvent Event   = chatEvent->GetChatEvent();
  bool                             Whisper = (Event == CBNETProtocol::EID_WHISPER);
  string                           User    = chatEvent->GetUser();
  string                           Message = chatEvent->GetMessage();

  // handle spoof checking for current game
  // this case covers whispers - we assume that anyone who sends a whisper to the bot with message "spoofcheck" should be considered spoof checked
  // note that this means you can whisper "spoofcheck" even in a public game to manually spoofcheck if the /whois fails

  if (Event == CBNETProtocol::EID_WHISPER && m_Aura->m_CurrentGame)
  {
    if (Message == "s" || Message == "sc" || Message == "spoofcheck")
    {
      m_Aura->m_CurrentGame->AddToSpoofed(m_Server, User, true);
      return;
    }
  }

  if (Event == CBNETProtocol::EID_IRC)
  {
    // extract the irc channel

    string::size_type MessageStart = Message.find(' ');

    m_IRC   = Message.substr(0, MessageStart);
    Message = Message.substr(MessageStart + 1);
  }
  else
    m_IRC.clear();

  if (Event == CBNETProtocol::EID_WHISPER || Event == CBNETProtocol::EID_TALK || Event == CBNETProtocol::EID_IRC)
  {
    if (Event == CBNETProtocol::EID_WHISPER)
      Print2("[WHISPER: " + m_ServerAlias + "] [" + User + "] " + Message);
    else
      Print("[LOCAL: " + m_ServerAlias + "] [" + User + "] " + Message);

    // handle bot commands

    if (!Message.empty() && Message[0] == m_CommandTrigger)
    {
      // extract the command trigger, the command, and the payload
      // e.g. "!say hello world" -> command: "say", payload: "hello world"

      string            Command, Payload;
      string::size_type PayloadStart = Message.find(' ');

      if (PayloadStart != string::npos)
      {
        Command = Message.substr(1, PayloadStart - 1);
        Payload = Message.substr(PayloadStart + 1);
      }
      else
        Command = Message.substr(1);

      transform(begin(Command), end(Command), begin(Command), ::tolower);

      const uint64_t CommandHash = HashCode(Command);

      if (IsAdmin(User) || IsRootAdmin(User))
      {
        Print("[BNET: " + m_ServerAlias + "] admin [" + User + "] sent command [" + Message + "]");

        /*****************
         * ADMIN COMMANDS *
         ******************/

        switch (CommandHash)
        {
          //
          // !MAP (load map file)
          //

          case HashCode("map"):
          {
            if (Payload.empty())
              QueueChatCommand("The currently loaded map/config file is: [" + m_Aura->m_Map->GetCFGFile() + "]", User, Whisper, m_IRC);
            else
            {
              if (!FileExists(m_Aura->m_MapPath))
              {
                Print("[BNET: " + m_ServerAlias + "] error listing maps - map path doesn't exist");
                QueueChatCommand("Error listing maps - map path doesn't exist", User, Whisper, m_IRC);
              }
              else
              {
                const vector<string> Matches = MapFilesMatch(Payload);

                if (Matches.empty())
                  QueueChatCommand("No maps found with that name", User, Whisper, m_IRC);
                else if (Matches.size() == 1)
                {
                  const string File = Matches.at(0);
                  QueueChatCommand("Loading map file [" + File + "]", User, Whisper, m_IRC);

                  // hackhack: create a config file in memory with the required information to load the map

                  CConfig MapCFG;
                  MapCFG.Set("map_path", R"(Maps\Download\)" + File);
                  MapCFG.Set("map_localpath", File);

                  if (File.find("DotA") != string::npos)
                    MapCFG.Set("map_type", "dota");

                  m_Aura->m_Map->Load(&MapCFG, File);

                  if (m_Aura->m_Map)
                  {
                    const char* ErrorMessage = m_Aura->m_Map->CheckValid();

                    if (ErrorMessage)
                      QueueChatCommand(std::string("Error while loading map: [") + ErrorMessage + "]", User, Whisper, m_IRC);
                  }
                }
                else
                {
                  string FoundMaps;

                  for (const auto& match : Matches)
                    FoundMaps += match + ", ";

                  QueueChatCommand("Maps: " + FoundMaps.substr(0, FoundMaps.size() - 2), User, Whisper, m_IRC);
                }
              }
            }

            break;
          }

          //
          // !UNHOST
          //

          case HashCode("unhost"):
          case HashCode("uh"):
          {
            if (m_Aura->m_CurrentGame)
            {
              if (m_Aura->m_CurrentGame->GetCountDownStarted())
                QueueChatCommand("Unable to unhost game [" + m_Aura->m_CurrentGame->GetDescription() + "]. The countdown has started, just wait a few seconds", User, Whisper, m_IRC);

              // if the game owner is still in the game only allow the root admin to unhost the game

              else if (m_Aura->m_CurrentGame->GetPlayerFromName(m_Aura->m_CurrentGame->GetOwnerName(), false) && !IsRootAdmin(User))
                QueueChatCommand("You can't unhost that game because the game owner [" + m_Aura->m_CurrentGame->GetOwnerName() + "] is in the lobby", User, Whisper, m_IRC);
              else
              {
                QueueChatCommand("Unhosting game [" + m_Aura->m_CurrentGame->GetDescription() + "]", User, Whisper, m_IRC);
                m_Aura->m_CurrentGame->SetExiting(true);
              }
            }
            else
              QueueChatCommand("Unable to unhost game. There is no game in the lobby", User, Whisper, m_IRC);

            break;
          }

          //
          // !PUB (host public game)
          //

          case HashCode("pub"):
          {
            if (!Payload.empty())
              m_Aura->CreateGame(m_Aura->m_Map, GAME_PUBLIC, Payload, User, User, this, Whisper);

            break;
          }

          //
          // !PRIV (host private game)
          //

          case HashCode("priv"):
          {
            if (!Payload.empty())
              m_Aura->CreateGame(m_Aura->m_Map, GAME_PRIVATE, Payload, User, User, this, Whisper);

            break;
          }

          //
          // !LOAD (load config file)
          //

          case HashCode("load"):
          {
            if (Payload.empty())
              QueueChatCommand("The currently loaded map/config file is: [" + m_Aura->m_Map->GetCFGFile() + "]", User, Whisper, m_IRC);
            else
            {
              if (!FileExists(m_Aura->m_MapCFGPath))
              {
                Print("[BNET: " + m_ServerAlias + "] error listing map configs - map config path doesn't exist");
                QueueChatCommand("Error listing map configs - map config path doesn't exist", User, Whisper, m_IRC);
              }
              else
              {
                const vector<string> Matches = ConfigFilesMatch(Payload);

                if (Matches.empty())
                  QueueChatCommand("No map configs found with that name", User, Whisper, m_IRC);
                else if (Matches.size() == 1)
                {
                  const string File = Matches.at(0);
                  QueueChatCommand("Loading config file [" + m_Aura->m_MapCFGPath + File + "]", User, Whisper, m_IRC);
                  CConfig MapCFG;
                  MapCFG.Read(m_Aura->m_MapCFGPath + File);
                  m_Aura->m_Map->Load(&MapCFG, m_Aura->m_MapCFGPath + File);
                }
                else
                {
                  string FoundMapConfigs;

                  for (const auto& match : Matches)
                    FoundMapConfigs += match + ", ";

                  QueueChatCommand("Maps configs: " + FoundMapConfigs.substr(0, FoundMapConfigs.size() - 2), User, Whisper, m_IRC);
                }
              }
            }

            break;
          }

          //
          // !ADDADMIN
          //

          case HashCode("addadmin"):
          {
            if (Payload.empty())
              break;

            if (IsRootAdmin(User))
            {
              if (IsAdmin(Payload))
                QueueChatCommand("Error. User [" + Payload + "] is already an admin on server [" + m_Server + "]", User, Whisper, m_IRC);
              else if (m_Aura->m_DB->AdminAdd(m_Server, Payload))
                QueueChatCommand("Added user [" + Payload + "] to the admin database on server [" + m_Server + "]", User, Whisper, m_IRC);
              else
                QueueChatCommand("Error adding user [" + Payload + "] to the admin database on server [" + m_Server + "]", User, Whisper, m_IRC);
            }
            else
              QueueChatCommand("You don't have access to that command", User, Whisper, m_IRC);

            break;
          }

          //
          // !ADDBAN
          // !BAN
          //

          case HashCode("addban"):
          case HashCode("ban"):
          {
            if (Payload.empty())
              break;

            // extract the victim and the reason
            // e.g. "Varlock leaver after dying" -> victim: "Varlock", reason: "leaver after dying"

            string       Victim, Reason;
            stringstream SS;
            SS << Payload;
            SS >> Victim;

            if (!SS.eof())
            {
              getline(SS, Reason);
              string::size_type Start = Reason.find_first_not_of(' ');

              if (Start != string::npos)
                Reason = Reason.substr(Start);
            }

            CDBBan* Ban = IsBannedName(Victim);

            if (Ban)
            {
              QueueChatCommand("Error. User [" + Victim + "] is already banned on server [" + m_Server + "]", User, Whisper, m_IRC);

              delete Ban;
            }
            else if (m_Aura->m_DB->BanAdd(m_Server, Victim, User, Reason))
              QueueChatCommand("Banned user [" + Victim + "] on server [" + m_Server + "]", User, Whisper, m_IRC);
            else
              QueueChatCommand("Error banning user [" + Victim + "] on server [" + m_Server + "]", User, Whisper, m_IRC);

            break;
          }

          //
          // !CHANNEL (change channel)
          //

          case HashCode("channel"):
          {
            if (Payload.empty())
              break;

            QueueChatCommand("/join " + Payload);
            break;
          }

          //
          // !CHECKADMIN
          //

          case HashCode("checkadmin"):
          {
            if (Payload.empty())
              break;

            if (IsRootAdmin(User))
            {
              if (IsAdmin(Payload))
                QueueChatCommand("User [" + Payload + "] is an admin on server [" + m_Server + "]", User, Whisper, m_IRC);
              else
                QueueChatCommand("User [" + Payload + "] is not an admin on server [" + m_Server + "]", User, Whisper, m_IRC);
            }
            else
              QueueChatCommand("You don't have access to that command", User, Whisper, m_IRC);

            break;
          }

          //
          // !CHECKBAN
          //

          case HashCode("checkban"):
          {
            if (Payload.empty())
              break;

            CDBBan* Ban = IsBannedName(Payload);

            if (Ban)
              QueueChatCommand("User [" + Payload + "] was banned on server [" + m_Server + "] on " + Ban->GetDate() + " by [" + Ban->GetAdmin() + "] because [" + Ban->GetReason() + "]", User, Whisper, m_IRC);
            else
              QueueChatCommand("User [" + Payload + "] is not banned on server [" + m_Server + "]", User, Whisper, m_IRC);

            delete Ban;
            break;
          }

          //
          // !CLOSE (close slot)
          //

          case HashCode("close"):
          {
            if (Payload.empty() || !m_Aura->m_CurrentGame)
              break;

            if (!m_Aura->m_CurrentGame->GetLocked())
            {
              // close as many slots as specified, e.g. "5 10" closes slots 5 and 10

              stringstream SS;
              SS << Payload;

              while (!SS.eof())
              {
                uint32_t SID;
                SS >> SID;

                if (SS.fail())
                {
                  Print("[BNET: " + m_ServerAlias + "] bad input to close command");
                  break;
                }
                else
                  m_Aura->m_CurrentGame->CloseSlot(static_cast<uint8_t>(SID - 1), true);
              }
            }
            else
              QueueChatCommand("This command is disabled while the game is locked", User, Whisper, m_IRC);

            break;
          }

          //
          // !CLOSEALL
          //

          case HashCode("closeall"):
          {
            if (!m_Aura->m_CurrentGame)
              break;

            if (!m_Aura->m_CurrentGame->GetLocked())
              m_Aura->m_CurrentGame->CloseAllSlots();
            else
              QueueChatCommand("This command is disabled while the game is locked", User, Whisper, m_IRC);

            break;
          }

          //
          // !COUNTADMINS
          //

          case HashCode("countadmins"):
          {
            if (IsRootAdmin(User))
            {
              uint32_t Count = m_Aura->m_DB->AdminCount(m_Server);

              if (Count == 0)
                QueueChatCommand("There are no admins on server [" + m_Server + "]", User, Whisper, m_IRC);
              else if (Count == 1)
                QueueChatCommand("There is 1 admin on server [" + m_Server + "]", User, Whisper, m_IRC);
              else
                QueueChatCommand("There are " + to_string(Count) + " admins on server [" + m_Server + "]", User, Whisper, m_IRC);
            }
            else
              QueueChatCommand("You don't have access to that command", User, Whisper, m_IRC);

            break;
          }

          //
          // !COUNTBANS
          //

          case HashCode("countbans"):
          {
            uint32_t Count = m_Aura->m_DB->BanCount(m_Server);

            if (Count == 0)
              QueueChatCommand("There are no banned users on server [" + m_Server + "]", User, Whisper, m_IRC);
            else if (Count == 1)
              QueueChatCommand("There is 1 banned user on server [" + m_Server + "]", User, Whisper, m_IRC);
            else
              QueueChatCommand("There are " + to_string(Count) + " banned users on server [" + m_Server + "]", User, Whisper, m_IRC);

            break;
          }

          //
          // !DELADMIN
          //

          case HashCode("deladmin"):
          {
            if (Payload.empty())
              break;

            if (IsRootAdmin(User))
            {
              if (!IsAdmin(Payload))
                QueueChatCommand("User [" + Payload + "] is not an admin on server [" + m_Server + "]", User, Whisper, m_IRC);
              else if (m_Aura->m_DB->AdminRemove(m_Server, Payload))
                QueueChatCommand("Deleted user [" + Payload + "] from the admin database on server [" + m_Server + "]", User, Whisper, m_IRC);
              else
                QueueChatCommand("Error deleting user [" + Payload + "] from the admin database on server [" + m_Server + "]", User, Whisper, m_IRC);
            }
            else
              QueueChatCommand("You don't have access to that command", User, Whisper, m_IRC);

            break;
          }

          //
          // !DELBAN
          // !UNBAN
          //

          case HashCode("delban"):
          case HashCode("unban"):
          {
            if (Payload.empty())
              break;

            if (m_Aura->m_DB->BanRemove(Payload))
              QueueChatCommand("Unbanned user [" + Payload + "] on all realms", User, Whisper, m_IRC);
            else
              QueueChatCommand("Error unbanning user [" + Payload + "] on all realms", User, Whisper, m_IRC);

            break;
          }

          //
          // !DOWNLOADS
          // !DLS
          //

          case HashCode("downloads"):
          case HashCode("dls"):
          {
            if (Payload.empty())
            {
              if (m_Aura->m_AllowDownloads == 0)
                QueueChatCommand("Map downloads disabled", User, Whisper, m_IRC);
              else if (m_Aura->m_AllowDownloads == 1)
                QueueChatCommand("Map downloads enabled", User, Whisper, m_IRC);
              else if (m_Aura->m_AllowDownloads == 2)
                QueueChatCommand("Conditional map downloads enabled", User, Whisper, m_IRC);

              break;
            }

            try
            {
              const uint32_t Downloads = stoul(Payload);

              if (Downloads == 0)
              {
                QueueChatCommand("Map downloads disabled", User, Whisper, m_IRC);
                m_Aura->m_AllowDownloads = 0;
              }
              else if (Downloads == 1)
              {
                QueueChatCommand("Map downloads enabled", User, Whisper, m_IRC);
                m_Aura->m_AllowDownloads = 1;
              }
              else if (Downloads == 2)
              {
                QueueChatCommand("Conditional map downloads enabled", User, Whisper, m_IRC);
                m_Aura->m_AllowDownloads = 2;
              }
            }
            catch (...)
            {
              // do nothing
            }

            break;
          }

          //
          // !END
          //

          case HashCode("end"):
          {
            if (Payload.empty())
              break;

            // TODO: what if a game ends just as you're typing this command and the numbering changes?

            try
            {
              const uint32_t GameNumber = stoul(Payload) - 1;

              if (GameNumber < m_Aura->m_Games.size())
              {
                // if the game owner is still in the game only allow the root admin to end the game

                if (m_Aura->m_Games[GameNumber]->GetPlayerFromName(m_Aura->m_Games[GameNumber]->GetOwnerName(), false) && !IsRootAdmin(User))
                  QueueChatCommand("You can't end that game because the game owner [" + m_Aura->m_Games[GameNumber]->GetOwnerName() + "] is still playing", User, Whisper, m_IRC);
                else
                {
                  QueueChatCommand("Ending game [" + m_Aura->m_Games[GameNumber]->GetDescription() + "]", User, Whisper, m_IRC);
                  Print("[GAME: " + m_Aura->m_Games[GameNumber]->GetGameName() + "] is over (admin ended game)");
                  m_Aura->m_Games[GameNumber]->StopPlayers("was disconnected (admin ended game)");
                }
              }
              else
                QueueChatCommand("Game number " + Payload + " doesn't exist", User, Whisper, m_IRC);
            }
            catch (...)
            {
              // do nothing
            }

            break;
          }

          //
          // !HOLD (hold a slot for someone)
          //

          case HashCode("hold"):
          {
            if (Payload.empty() || !m_Aura->m_CurrentGame)
              break;

            // hold as many players as specified, e.g. "Varlock Kilranin" holds players "Varlock" and "Kilranin"

            stringstream SS;
            SS << Payload;

            while (!SS.eof())
            {
              string HoldName;
              SS >> HoldName;

              if (SS.fail())
              {
                Print("[BNET: " + m_ServerAlias + "] bad input to hold command");
                break;
              }
              else
              {
                QueueChatCommand("Added player [" + HoldName + "] to the hold list", User, Whisper, m_IRC);
                m_Aura->m_CurrentGame->AddToReserved(HoldName);
              }
            }

            break;
          }

          //
          // !SENDLAN
          //

          case HashCode("sendlan"):
          {
            if (Payload.empty() || !m_Aura->m_CurrentGame || m_Aura->m_CurrentGame->GetCountDownStarted())
              break;

            // extract the ip and the port
            // e.g. "1.2.3.4 6112" -> ip: "1.2.3.4", port: "6112"

            string       IP;
            uint32_t     Port = 6112;
            stringstream SS;
            SS << Payload;
            SS >> IP;

            if (!SS.eof())
              SS >> Port;

            if (SS.fail())
              QueueChatCommand("Bad input to sendlan command", User, Whisper, m_IRC);
            else
            {
              // construct a fixed host counter which will be used to identify players from this "realm" (i.e. LAN)
              // the fixed host counter's 4 most significant bits will contain a 4 bit ID (0-15)
              // the rest of the fixed host counter will contain the 28 least significant bits of the actual host counter
              // since we're destroying 4 bits of information here the actual host counter should not be greater than 2^28 which is a reasonable assumption
              // when a player joins a game we can obtain the ID from the received host counter
              // note: LAN broadcasts use an ID of 0, battle.net refreshes use an ID of 1-10, the rest are unused

              // we send 12 for SlotsTotal because this determines how many PID's Warcraft 3 allocates
              // we need to make sure Warcraft 3 allocates at least SlotsTotal + 1 but at most 12 PID's
              // this is because we need an extra PID for the virtual host player (but we always delete the virtual host player when the 12th person joins)
              // however, we can't send 13 for SlotsTotal because this causes Warcraft 3 to crash when sharing control of units
              // nor can we send SlotsTotal because then Warcraft 3 crashes when playing maps with less than 12 PID's (because of the virtual host player taking an extra PID)
              // we also send 12 for SlotsOpen because Warcraft 3 assumes there's always at least one player in the game (the host)
              // so if we try to send accurate numbers it'll always be off by one and results in Warcraft 3 assuming the game is full when it still needs one more player
              // the easiest solution is to simply send 12 for both so the game will always show up as (1/12) players

              // note: the PrivateGame flag is not set when broadcasting to LAN (as you might expect)
              // note: we do not use m_Map->GetMapGameType because none of the filters are set when broadcasting to LAN (also as you might expect)

              m_Aura->m_UDPSocket->SendTo(IP, Port, m_Aura->m_CurrentGame->GetProtocol()->SEND_W3GS_GAMEINFO(m_Aura->m_LANWar3Version, CreateByteArray(static_cast<uint32_t>(MAPGAMETYPE_UNKNOWN0), false), m_Aura->m_CurrentGame->GetMap()->GetMapGameFlags(), m_Aura->m_CurrentGame->GetMap()->GetMapWidth(), m_Aura->m_CurrentGame->GetMap()->GetMapHeight(), m_Aura->m_CurrentGame->GetGameName(), "Clan 007", 0, m_Aura->m_CurrentGame->GetMap()->GetMapPath(), m_Aura->m_CurrentGame->GetMap()->GetMapCRC(), 12, 12, m_Aura->m_CurrentGame->GetHostPort(), m_Aura->m_CurrentGame->GetHostCounter() & 0x0FFFFFFF, m_Aura->m_CurrentGame->GetEntryKey()));
            }

            break;
          }

          //
          // !COUNTMAPS
          // !COUNTMAP
          //

          case HashCode("countmap"):
          case HashCode("countmaps"):
          {
#ifdef WIN32
            const auto Count = FilesMatch(m_Aura->m_MapPath, ".").size();
#else
            const auto Count = FilesMatch(m_Aura->m_MapPath, "").size();
#endif

            QueueChatCommand("There are currently [" + to_string(Count) + "] maps", User, Whisper, m_IRC);
            break;
          }

          //
          // !COUNTCFG
          // !COUNTCFGS
          //

          case HashCode("countcfg"):
          case HashCode("countcfgs"):
          {
#ifdef WIN32
            const auto Count = FilesMatch(m_Aura->m_MapCFGPath, ".").size();
#else
            const auto Count = FilesMatch(m_Aura->m_MapCFGPath, "").size();
#endif
            QueueChatCommand("There are currently [" + to_string(Count) + "] cfgs", User, Whisper, m_IRC);
            break;
          }

          //
          // !DELETECFG
          //

          case HashCode("deletecfg"):
          {
            if (Payload.empty() || !IsRootAdmin(User))
              break;

            if (Payload.find(".cfg") == string::npos)
              Payload.append(".cfg");

            if (!remove((m_Aura->m_MapCFGPath + Payload).c_str()))
              QueueChatCommand("Deleted [" + Payload + "]", User, Whisper, m_IRC);
            else
              QueueChatCommand("Removal failed", User, Whisper, m_IRC);

            break;
          }

          //
          // !DELETEMAP
          //

          case HashCode("deletemap"):
          {
            if (Payload.empty() || !IsRootAdmin(User))
              break;

            if (Payload.find(".w3x") == string::npos && Payload.find(".w3m") == string::npos)
              Payload.append(".w3x");

            if (!remove((m_Aura->m_MapPath + Payload).c_str()))
              QueueChatCommand("Deleted [" + Payload + "]", User, Whisper, m_IRC);
            else
              QueueChatCommand("Removal failed", User, Whisper, m_IRC);

            break;
          }

          //
          // !OPEN (open slot)
          //

          case HashCode("open"):
          {
            if (Payload.empty() || !m_Aura->m_CurrentGame)
              break;

            if (!m_Aura->m_CurrentGame->GetLocked())
            {
              // open as many slots as specified, e.g. "5 10" opens slots 5 and 10

              stringstream SS;
              SS << Payload;

              while (!SS.eof())
              {
                uint32_t SID;
                SS >> SID;

                if (SS.fail())
                {
                  Print("[BNET: " + m_ServerAlias + "] bad input to open command");
                  break;
                }
                else
                  m_Aura->m_CurrentGame->OpenSlot(static_cast<uint8_t>(SID - 1), true);
              }
            }
            else
              QueueChatCommand("This command is disabled while the game is locked", User, Whisper, m_IRC);

            break;
          }

          //
          // !OPENALL
          //

          case HashCode("openall"):
          {
            if (!m_Aura->m_CurrentGame)
              break;

            if (!m_Aura->m_CurrentGame->GetLocked())
              m_Aura->m_CurrentGame->OpenAllSlots();
            else
              QueueChatCommand("This command is disabled while the game is locked", User, Whisper, m_IRC);

            break;
          }

          //
          // !PRIVBY (host private game by other player)
          //

          case HashCode("privby"):
          {
            if (Payload.empty())
              break;

            // extract the owner and the game name
            // e.g. "Varlock dota 6.54b arem ~~~" -> owner: "Varlock", game name: "dota 6.54b arem ~~~"

            string            Owner, GameName;
            string::size_type GameNameStart = Payload.find(' ');

            if (GameNameStart != string::npos)
            {
              Owner    = Payload.substr(0, GameNameStart);
              GameName = Payload.substr(GameNameStart + 1);
              m_Aura->CreateGame(m_Aura->m_Map, GAME_PRIVATE, GameName, Owner, User, this, Whisper);
            }

            break;
          }

          //
          // !PUBBY (host public game by other player)
          //

          case HashCode("pubby"):
          {
            if (Payload.empty())
              break;

            // extract the owner and the game name
            // e.g. "Varlock dota 6.54b arem ~~~" -> owner: "Varlock", game name: "dota 6.54b arem ~~~"

            string            Owner, GameName;
            string::size_type GameNameStart = Payload.find(' ');

            if (GameNameStart != string::npos)
            {
              Owner    = Payload.substr(0, GameNameStart);
              GameName = Payload.substr(GameNameStart + 1);
              m_Aura->CreateGame(m_Aura->m_Map, GAME_PUBLIC, GameName, Owner, User, this, Whisper);
            }

            break;
          }

          //
          // !RELOAD
          //

          case HashCode("reload"):
          {
            if (IsRootAdmin(User))
            {
              QueueChatCommand("Reloading configuration files", User, Whisper, m_IRC);
              m_Aura->ReloadConfigs();
            }
            else
              QueueChatCommand("You don't have access to that command", User, Whisper, m_IRC);

            break;
          }

          //
          // !SAY
          //

          case HashCode("say"):
          {
            if (Payload.empty())
              break;

            if (IsRootAdmin(User) || Payload[0] != '/')
              QueueChatCommand(Payload);

            break;
          }

          //
          // !SAYGAME
          //

          case HashCode("saygame"):
          {
            if (Payload.empty())
              break;

            if (IsRootAdmin(User))
            {
              // extract the game number and the message
              // e.g. "3 hello everyone" -> game number: "3", message: "hello everyone"

              uint32_t     GameNumber;
              string       Message;
              stringstream SS;
              SS << Payload;
              SS >> GameNumber;

              if (SS.fail())
                Print("[BNET: " + m_ServerAlias + "] bad input #1 to saygame command");
              else
              {
                if (SS.eof())
                  Print("[BNET: " + m_ServerAlias + "] missing input #2 to saygame command");
                else
                {
                  getline(SS, Message);
                  string::size_type Start = Message.find_first_not_of(' ');

                  if (Start != string::npos)
                    Message = Message.substr(Start);

                  if (GameNumber - 1 < m_Aura->m_Games.size())
                    m_Aura->m_Games[GameNumber - 1]->SendAllChat("ADMIN: " + Message);
                  else
                    QueueChatCommand("Game number " + to_string(GameNumber) + " doesn't exist", User, Whisper, m_IRC);
                }
              }
            }
            else
              QueueChatCommand("You don't have access to that command", User, Whisper, m_IRC);

            break;
          }

          //
          // !SAYGAMES
          //

          case HashCode("saygames"):
          {
            if (Payload.empty())
              break;

            if (IsRootAdmin(User))
            {
              if (m_Aura->m_CurrentGame)
                m_Aura->m_CurrentGame->SendAllChat(Payload);

              for (auto& game : m_Aura->m_Games)
                game->SendAllChat("ADMIN: " + Payload);
            }
            else
            {
              if (m_Aura->m_CurrentGame)
                m_Aura->m_CurrentGame->SendAllChat(Payload);

              for (auto& game : m_Aura->m_Games)
                game->SendAllChat("ADMIN (" + User + "): " + Payload);
            }

            break;
          }

          //
          // !SP
          //

          case HashCode("sp"):
          {
            if (!m_Aura->m_CurrentGame || m_Aura->m_CurrentGame->GetCountDownStarted())
              break;

            if (!m_Aura->m_CurrentGame->GetLocked())
            {
              m_Aura->m_CurrentGame->SendAllChat("Shuffling players");
              m_Aura->m_CurrentGame->ShuffleSlots();
            }
            else
              QueueChatCommand("This command is disabled while the game is locked", User, Whisper, m_IRC);

            break;
          }

          //
          // !START
          // !S
          //

          case HashCode("s"):
          case HashCode("start"):
          {
            if (!m_Aura->m_CurrentGame || m_Aura->m_CurrentGame->GetCountDownStarted() || m_Aura->m_CurrentGame->GetNumHumanPlayers() == 0)
              break;

            if (!m_Aura->m_CurrentGame->GetLocked())
            {
              // if the player sent "!start force" skip the checks and start the countdown
              // otherwise check that the game is ready to start

              if (Payload == "force")
                m_Aura->m_CurrentGame->StartCountDown(true);
              else
                m_Aura->m_CurrentGame->StartCountDown(false);
            }
            else
              QueueChatCommand("This command is disabled while the game is locked", User, Whisper, m_IRC);

            break;
          }

          //
          // !SWAP (swap slots)
          //

          case HashCode("swap"):
          {
            if (Payload.empty() || !m_Aura->m_CurrentGame)
              break;

            if (!m_Aura->m_CurrentGame->GetLocked())
            {
              uint32_t     SID1, SID2;
              stringstream SS;
              SS << Payload;
              SS >> SID1;

              if (SS.fail())
                Print("[BNET: " + m_ServerAlias + "] bad input #1 to swap command");
              else
              {
                if (SS.eof())
                  Print("[BNET: " + m_ServerAlias + "] missing input #2 to swap command");
                else
                {
                  SS >> SID2;

                  if (SS.fail())
                    Print("[BNET: " + m_ServerAlias + "] bad input #2 to swap command");
                  else
                    m_Aura->m_CurrentGame->SwapSlots(static_cast<uint8_t>(SID1 - 1), static_cast<uint8_t>(SID2 - 1));
                }
              }
            }
            else
              QueueChatCommand("This command is disabled while the game is locked", User, Whisper, m_IRC);

            break;
          }

          //
          // !RESTART
          //

          case HashCode("restart"):
          {
            if ((!m_Aura->m_Games.size() && !m_Aura->m_CurrentGame) || Payload == "force")
            {
              m_Exiting = true;

              // gRestart is defined in aura.cpp

              extern bool gRestart;
              gRestart = true;
            }
            else
              QueueChatCommand("Games in progress, use !restart force", User, Whisper, m_IRC);

            break;
          }

          //
          // !W
          //

          case HashCode("w"):
          {
            if (Payload.empty())
              break;

            // extract the name and the message
            // e.g. "Varlock hello there!" -> name: "Varlock", message: "hello there!"

            string            Name;
            string            Message;
            string::size_type MessageStart = Payload.find(' ');

            if (MessageStart != string::npos)
            {
              Name    = Payload.substr(0, MessageStart);
              Message = Payload.substr(MessageStart + 1);

              for (auto& bnet : m_Aura->m_BNETs)
                bnet->QueueChatCommand(Message, Name, true, string());
            }

            break;
          }

          //
          // !DISABLE
          //

          case HashCode("disable"):
          {
            if (IsRootAdmin(User))
            {
              QueueChatCommand("Creation of new games has been disabled (if there is a game in the lobby it will not be automatically unhosted)", User, Whisper, m_IRC);
              m_Aura->m_Enabled = false;
            }
            else
              QueueChatCommand("You don't have access to that command", User, Whisper, m_IRC);

            break;
          }

          //
          // !ENABLE
          //

          case HashCode("enable"):
          {
            if (IsRootAdmin(User))
            {
              QueueChatCommand("Creation of new games has been enabled", User, Whisper, m_IRC);
              m_Aura->m_Enabled = true;
            }
            else
              QueueChatCommand("You don't have access to that command", User, Whisper, m_IRC);

            break;
          }

          //
          // !GETCLAN
          //

          case HashCode("getclan"):
          {
            SendGetClanList();
            QueueChatCommand("Updating the bot's internal clan list from battle.net..", User, Whisper, m_IRC);
            break;
          }

          //
          // !GETFRIENDS
          //

          case HashCode("getfriends"):
          {
            SendGetFriendsList();
            QueueChatCommand("Updating the bot's internal friends list from battle.net..", User, Whisper, m_IRC);
            break;
          }

          //
          // !EXIT
          // !QUIT
          //

          case HashCode("exit"):
          case HashCode("quit"):
          {
            if (IsRootAdmin(User))
            {
              if (Payload == "force")
                m_Exiting = true;
              else
              {
                if (m_Aura->m_CurrentGame || !m_Aura->m_Games.empty())
                  QueueChatCommand("At least one game is in the lobby or in progress. Use 'force' to shutdown anyway", User, Whisper, m_IRC);
                else
                  m_Exiting = true;
              }
            }
            else
              QueueChatCommand("You don't have access to that command", User, Whisper, m_IRC);

            break;
          }
        }
      }
      else
        Print("[BNET: " + m_ServerAlias + "] non-admin [" + User + "] sent command [" + Message + "]");

      /*********************
       * NON ADMIN COMMANDS *
       *********************/

      // don't respond to non admins if there are more than 3 messages already in the queue
      // this prevents malicious users from filling up the bot's chat queue and crippling the bot
      // in some cases the queue may be full of legitimate messages but we don't really care if the bot ignores one of these commands once in awhile
      // e.g. when several users join a game at the same time and cause multiple /whois messages to be queued at once

      if (IsAdmin(User) || IsRootAdmin(User) || m_OutPackets.size() < 3)
      {
        switch (CommandHash)
        {
          //
          // !STATS
          //

          case HashCode("stats"):
          {
            string StatsUser = User;

            if (!Payload.empty())
              StatsUser = Payload;

            // check for potential abuse

            if (StatsUser.size() < 16 && StatsUser[0] != '/')
            {
              CDBGamePlayerSummary* GamePlayerSummary = m_Aura->m_DB->GamePlayerSummaryCheck(StatsUser);

              if (GamePlayerSummary)
                QueueChatCommand("[" + StatsUser + "] has played " + to_string(GamePlayerSummary->GetTotalGames()) + " games with this bot. Average loading time: " + to_string(GamePlayerSummary->GetAvgLoadingTime()) + " seconds. Average stay: " + to_string(GamePlayerSummary->GetAvgLeftPercent()) + " percent", User, Whisper, m_IRC);
              else
                QueueChatCommand("[" + StatsUser + "] hasn't played any games with this bot yet", User, Whisper, m_IRC);

              delete GamePlayerSummary;
            }

            break;
          }

          //
          // !GETGAME
          // !G
          //

          case HashCode("g"):
          case HashCode("getgame"):
          {
            if (Payload.empty())
            {
              if (m_Aura->m_CurrentGame)
                QueueChatCommand("Game [" + m_Aura->m_CurrentGame->GetDescription() + "] is in the lobby and there are " + to_string(m_Aura->m_Games.size()) + "/" + to_string(m_Aura->m_MaxGames) + " other games in progress", User, Whisper, m_IRC);
              else
                QueueChatCommand("There is no game in the lobby and there are " + to_string(m_Aura->m_Games.size()) + "/" + to_string(m_Aura->m_MaxGames) + " other games in progress", User, Whisper, m_IRC);
              break;
            }

            try
            {
              const uint32_t GameNumber = stoul(Payload) - 1;

              if (GameNumber < m_Aura->m_Games.size())
                QueueChatCommand("Game number " + Payload + " is [" + m_Aura->m_Games[GameNumber]->GetDescription() + "]", User, Whisper, m_IRC);
              else
                QueueChatCommand("Game number " + Payload + " doesn't exist", User, Whisper, m_IRC);
            }
            catch (...)
            {
              // do nothing
            }

            break;
          }

          //
          // !GETGAMES
          // !G
          //

          case HashCode("getgames"):
          {
            if (m_Aura->m_CurrentGame)
              QueueChatCommand("Game [" + m_Aura->m_CurrentGame->GetDescription() + "] is in the lobby and there are " + to_string(m_Aura->m_Games.size()) + "/" + to_string(m_Aura->m_MaxGames) + " other games in progress", User, Whisper, m_IRC);
            else
              QueueChatCommand("There is no game in the lobby and there are " + to_string(m_Aura->m_Games.size()) + "/" + to_string(m_Aura->m_MaxGames) + " other games in progress", User, Whisper, m_IRC);

            break;
          }

          //
          // !GETPLAYERS
          // !GP
          //

          case HashCode("gp"):
          case HashCode("getplayers"):
          {
            if (Payload.empty())
              break;

            const int32_t GameNumber = stoi(Payload) - 1;

            if (-1 < GameNumber && GameNumber < static_cast<int32_t>(m_Aura->m_Games.size()))
              QueueChatCommand("Players in game [" + m_Aura->m_Games[GameNumber]->GetGameName() + "] are: " + m_Aura->m_Games[GameNumber]->GetPlayers(), User, Whisper, m_IRC);
            else if (GameNumber == -1 && m_Aura->m_CurrentGame)
              QueueChatCommand("Players in lobby [" + m_Aura->m_CurrentGame->GetGameName() + "] are: " + m_Aura->m_CurrentGame->GetPlayers(), User, Whisper, m_IRC);
            else
              QueueChatCommand("Game number " + Payload + " doesn't exist", User, Whisper, m_IRC);

            break;
          }

          //
          // !GETOBSERVERS
          // !GO
          //

          case HashCode("go"):
          case HashCode("getobservers"):
          {
            if (Payload.empty())
              break;

            const int32_t GameNumber = stoi(Payload) - 1;

            if (-1 < GameNumber && GameNumber < static_cast<int32_t>(m_Aura->m_Games.size()))
              QueueChatCommand("Observers in game [" + m_Aura->m_Games[GameNumber]->GetGameName() + "] are: " + m_Aura->m_Games[GameNumber]->GetObservers(), User, Whisper, m_IRC);
            else if (GameNumber == -1 && m_Aura->m_CurrentGame)
              QueueChatCommand("Observers in lobby [" + m_Aura->m_CurrentGame->GetGameName() + "] are: " + m_Aura->m_CurrentGame->GetObservers(), User, Whisper, m_IRC);
            else
              QueueChatCommand("Game number " + Payload + " doesn't exist", User, Whisper, m_IRC);

            break;
          }

          //
          // !STATSDOTA
          // !SD
          //

          case HashCode("sd"):
          case HashCode("statsdota"):
          {
            string StatsUser = User;

            if (!Payload.empty())
              StatsUser = Payload;

            // check for potential abuse

            if (!StatsUser.empty() && StatsUser.size() < 16 && StatsUser[0] != '/')
            {
              const CDBDotAPlayerSummary* DotAPlayerSummary = m_Aura->m_DB->DotAPlayerSummaryCheck(StatsUser);

              if (DotAPlayerSummary)
              {
                const string Summary = StatsUser + " - " + to_string(DotAPlayerSummary->GetTotalGames()) + " games (W/L: " + to_string(DotAPlayerSummary->GetTotalWins()) + "/" + to_string(DotAPlayerSummary->GetTotalLosses()) + ") Hero K/D/A: " + to_string(DotAPlayerSummary->GetTotalKills()) + "/" + to_string(DotAPlayerSummary->GetTotalDeaths()) + "/" + to_string(DotAPlayerSummary->GetTotalAssists()) + " (" + to_string(DotAPlayerSummary->GetAvgKills()) + "/" + to_string(DotAPlayerSummary->GetAvgDeaths()) + "/" + to_string(DotAPlayerSummary->GetAvgAssists()) + ") Creep K/D/N: " + to_string(DotAPlayerSummary->GetTotalCreepKills()) + "/" + to_string(DotAPlayerSummary->GetTotalCreepDenies()) + "/" + to_string(DotAPlayerSummary->GetTotalNeutralKills()) + " (" + to_string(DotAPlayerSummary->GetAvgCreepKills()) + "/" + to_string(DotAPlayerSummary->GetAvgCreepDenies()) + "/" + to_string(DotAPlayerSummary->GetAvgNeutralKills()) + ") T/R/C: " + to_string(DotAPlayerSummary->GetTotalTowerKills()) + "/" + to_string(DotAPlayerSummary->GetTotalRaxKills()) + "/" + to_string(DotAPlayerSummary->GetTotalCourierKills());

                QueueChatCommand(Summary, User, Whisper, m_IRC);

                delete DotAPlayerSummary;
              }
              else
                QueueChatCommand("[" + StatsUser + "] hasn't played any DotA games here", User, Whisper, m_IRC);
            }

            break;
          }

          //
          // !STATUS
          //

          case HashCode("status"):
          {
            string message = "Status: ";

            for (auto& bnet : m_Aura->m_BNETs)
              message += bnet->GetServer() + (bnet->GetLoggedIn() ? " [online], " : " [offline], ");

            message += m_Aura->m_IRC->m_Server + (!m_Aura->m_IRC->m_WaitingToConnect ? " [online]" : " [offline]");

            QueueChatCommand(message, User, Whisper, m_IRC);
            break;
          }

          //
          // !VERSION
          //

          case HashCode("version"):
          {
            QueueChatCommand("Version: Aura " + m_Aura->m_Version, User, Whisper, m_IRC);
            break;
          }
        }
      }
    }
  }
  else if (Event == CBNETProtocol::EID_CHANNEL)
  {
    // keep track of current channel so we can rejoin it after hosting a game

    Print("[BNET: " + m_ServerAlias + "] joined channel [" + Message + "]");

    m_CurrentChannel = Message;
  }
  else if (Event == CBNETProtocol::EID_INFO)
  {
    Print("[INFO: " + m_ServerAlias + "] " + Message);

    // extract the first word which we hope is the username
    // this is not necessarily true though since info messages also include channel MOTD's and such

    if (m_Aura->m_CurrentGame)
    {
      string            UserName;
      string::size_type Split = Message.find(' ');

      if (Split != string::npos)
        UserName = Message.substr(0, Split);
      else
        UserName = Message;

      if (m_Aura->m_CurrentGame->GetPlayerFromName(UserName, true))
      {
        // handle spoof checking for current game
        // this case covers whois results which are used when hosting a public game (we send out a "/whois [player]" for each player)
        // at all times you can still /w the bot with "spoofcheck" to manually spoof check

        if (Message.find("Throne in game") != string::npos || Message.find("currently in  game") != string::npos || Message.find("currently in private game") != string::npos)
        {
          // check both the current game name and the last game name against the /whois response
          // this is because when the game is rehosted, players who joined recently will be in the previous game according to battle.net
          // note: if the game is rehosted more than once it is possible (but unlikely) for a false positive because only two game names are checked

          if (Message.find(m_Aura->m_CurrentGame->GetGameName()) != string::npos || Message.find(m_Aura->m_CurrentGame->GetLastGameName()) != string::npos)
            m_Aura->m_CurrentGame->AddToSpoofed(m_Server, UserName, false);
          else
            m_Aura->m_CurrentGame->SendAllChat("Name spoof detected. The real [" + UserName + "] is in another game");

          return;
        }
      }
    }
  }
  else if (Event == CBNETProtocol::EID_ERROR)
  {
    Print("[ERROR: " + m_ServerAlias + "] " + Message);
  }
}

void CBNET::SendGetFriendsList()
{
  if (m_LoggedIn)
    m_Socket->PutBytes(m_Protocol->SEND_SID_FRIENDLIST());
}

void CBNET::SendGetClanList()
{
  if (m_LoggedIn)
    m_Socket->PutBytes(m_Protocol->SEND_SID_CLANMEMBERLIST());
}

void CBNET::QueueEnterChat()
{
  if (m_LoggedIn)
    m_OutPackets.push(m_Protocol->SEND_SID_ENTERCHAT());
}

void CBNET::QueueChatCommand(const string& chatCommand)
{
  if (chatCommand.empty())
    return;

  if (m_LoggedIn)
  {
    if (m_OutPackets.size() <= 10)
    {
      Print("[QUEUED: " + m_ServerAlias + "] " + chatCommand);

      if (m_PvPGN)
        m_OutPackets.push(m_Protocol->SEND_SID_CHATCOMMAND(chatCommand.substr(0, 200)));
      else if (chatCommand.size() > 255)
        m_OutPackets.push(m_Protocol->SEND_SID_CHATCOMMAND(chatCommand.substr(0, 255)));
      else
        m_OutPackets.push(m_Protocol->SEND_SID_CHATCOMMAND(chatCommand));
    }
    else
      Print("[BNET: " + m_ServerAlias + "] too many (" + to_string(m_OutPackets.size()) + ") packets queued, discarding");
  }
}

void CBNET::QueueChatCommand(const string& chatCommand, const string& user, bool whisper, const string& irc)
{
  if (chatCommand.empty())
    return;

  // if the destination is IRC send it there

  if (!irc.empty())
  {
    m_Aura->m_IRC->SendMessageIRC(chatCommand, irc);
    return;
  }

  // if whisper is true send the chat command as a whisper to user, otherwise just queue the chat command

  if (whisper)
  {
    QueueChatCommand("/w " + user + " " + chatCommand);
  }
  else
    QueueChatCommand(chatCommand);
}

void CBNET::QueueGameCreate(uint8_t state, const string& gameName, CMap* map, uint32_t hostCounter)
{
  if (m_LoggedIn && map)
  {
    if (!m_CurrentChannel.empty())
      m_FirstChannel = m_CurrentChannel;

    m_InChat = false;

    QueueGameRefresh(state, gameName, map, hostCounter);
  }
}

void CBNET::QueueGameRefresh(uint8_t state, const string& gameName, CMap* map, uint32_t hostCounter)
{
  if (m_LoggedIn && map)
  {
    // construct a fixed host counter which will be used to identify players from this realm
    // the fixed host counter's 4 most significant bits will contain a 4 bit ID (0-15)
    // the rest of the fixed host counter will contain the 28 least significant bits of the actual host counter
    // since we're destroying 4 bits of information here the actual host counter should not be greater than 2^28 which is a reasonable assumption
    // when a player joins a game we can obtain the ID from the received host counter
    // note: LAN broadcasts use an ID of 0, battle.net refreshes use an ID of 1-10, the rest are unused

    uint32_t MapGameType = map->GetMapGameType();
    MapGameType |= MAPGAMETYPE_UNKNOWN0;

    if (state == GAME_PRIVATE)
      MapGameType |= MAPGAMETYPE_PRIVATEGAME;

    // use an invalid map width/height to indicate reconnectable games

    std::vector<uint8_t> MapWidth;
    MapWidth.push_back(192);
    MapWidth.push_back(7);
    std::vector<uint8_t> MapHeight;
    MapHeight.push_back(192);
    MapHeight.push_back(7);

    m_OutPackets.push(m_Protocol->SEND_SID_STARTADVEX3(state, CreateByteArray(MapGameType, false), map->GetMapGameFlags(), MapWidth, MapHeight, gameName, m_UserName, 0, map->GetMapPath(), map->GetMapCRC(), map->GetMapSHA1(), ((hostCounter & 0x0FFFFFFF) | (m_HostCounterID << 28))));
  }
}

void CBNET::QueueGameUncreate()
{
  if (m_LoggedIn)
    m_OutPackets.push(m_Protocol->SEND_SID_STOPADV());
}

void CBNET::UnqueueGameRefreshes()
{
  queue<std::vector<uint8_t>> Packets;

  while (!m_OutPackets.empty())
  {
    // TODO: it's very inefficient to have to copy all these packets while searching the queue

    std::vector<uint8_t> Packet = m_OutPackets.front();
    m_OutPackets.pop();

    if (Packet.size() >= 2 && Packet[1] != CBNETProtocol::SID_STARTADVEX3)
      Packets.push(Packet);
  }

  m_OutPackets = Packets;
  Print("[BNET: " + m_ServerAlias + "] unqueued game refresh packets");
}

bool CBNET::IsAdmin(string name)
{
  transform(begin(name), end(name), begin(name), ::tolower);

  if (m_Aura->m_DB->AdminCheck(m_Server, name))
    return true;

  return false;
}

bool CBNET::IsRootAdmin(string name)
{
  transform(begin(name), end(name), begin(name), ::tolower);

  if (m_Aura->m_DB->RootAdminCheck(m_Server, name))
    return true;

  return false;
}

CDBBan* CBNET::IsBannedName(string name)
{
  transform(begin(name), end(name), begin(name), ::tolower);

  if (CDBBan* Ban = m_Aura->m_DB->BanCheck(m_Server, name))
    return Ban;

  return nullptr;
}

void CBNET::HoldFriends(CGame* game)
{
  for (auto& friend_ : m_Friends)
    game->AddToReserved(friend_);
}

void CBNET::HoldClan(CGame* game)
{
  for (auto& clanmate : m_Clan)
    game->AddToReserved(clanmate);
}

vector<string> CBNET::MapFilesMatch(string pattern)
{
  transform(begin(pattern), end(pattern), begin(pattern), ::tolower);

  auto ROCMaps = FilesMatch(m_Aura->m_MapPath, ".w3m");
  auto TFTMaps = FilesMatch(m_Aura->m_MapPath, ".w3x");

  vector<string> MapList;
  MapList.insert(end(MapList), begin(ROCMaps), end(ROCMaps));
  MapList.insert(end(MapList), begin(TFTMaps), end(TFTMaps));

  vector<string> Matches;

  for (auto& mapName : MapList)
  {
    string lowerMapName(mapName);
    transform(begin(lowerMapName), end(lowerMapName), begin(lowerMapName), ::tolower);

    if (lowerMapName.find(pattern) != string::npos)
      Matches.push_back(mapName);
  }

  return Matches;
}

vector<string> CBNET::ConfigFilesMatch(string pattern)
{
  transform(begin(pattern), end(pattern), begin(pattern), ::tolower);

  vector<string> ConfigList = FilesMatch(m_Aura->m_MapCFGPath, ".cfg");

  vector<string> Matches;

  for (auto& cfgName : ConfigList)
  {
    string lowerCfgName(cfgName);
    transform(begin(lowerCfgName), end(lowerCfgName), begin(lowerCfgName), ::tolower);

    if (lowerCfgName.find(pattern) != string::npos)
      Matches.push_back(cfgName);
  }

  return Matches;
}
