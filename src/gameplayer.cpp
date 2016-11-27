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

#include "gameplayer.h"
#include "aura.h"
#include "bnet.h"
#include "map.h"
#include "gameprotocol.h"
#include "gpsprotocol.h"
#include "game.h"

using namespace std;

//
// CPotentialPlayer
//

CPotentialPlayer::CPotentialPlayer(CGameProtocol* nProtocol, CGame* nGame, CTCPSocket* nSocket)
  : m_Protocol(nProtocol),
    m_Game(nGame),
    m_Socket(nSocket),
    m_IncomingJoinPlayer(nullptr),
    m_DeleteMe(false)
{
}

CPotentialPlayer::~CPotentialPlayer()
{
  if (m_Socket)
    delete m_Socket;

  delete m_IncomingJoinPlayer;
}

bool CPotentialPlayer::Update(void* fd)
{
  if (m_DeleteMe)
    return true;

  if (!m_Socket)
    return false;

  m_Socket->DoRecv((fd_set*)fd);

  // extract as many packets as possible from the socket's receive buffer and process them

  string*                   RecvBuffer      = m_Socket->GetBytes();
  std::vector<uint_fast8_t> Bytes           = CreateByteArray((uint_fast8_t*)RecvBuffer->c_str(), RecvBuffer->size());
  uint_fast32_t             LengthProcessed = 0;

  // a packet is at least 4 bytes so loop as long as the buffer contains 4 bytes

  while (Bytes.size() >= 4)
  {
    if (Bytes[0] == W3GS_HEADER_CONSTANT || Bytes[0] == GPS_HEADER_CONSTANT)
    {
      // bytes 2 and 3 contain the length of the packet

      const uint_fast16_t             Length = ByteArrayToUInt16(Bytes, false, 2);
      const std::vector<uint_fast8_t> Data   = std::vector<uint_fast8_t>(begin(Bytes), begin(Bytes) + Length);

      if (Bytes.size() >= Length)
      {
        if (Bytes[0] == W3GS_HEADER_CONSTANT && Bytes[1] == CGameProtocol::W3GS_REQJOIN)
        {
          delete m_IncomingJoinPlayer;
          m_IncomingJoinPlayer = m_Protocol->RECEIVE_W3GS_REQJOIN(Data);

          if (m_IncomingJoinPlayer)
            m_Game->EventPlayerJoined(this, m_IncomingJoinPlayer);

          // this is the packet which int32_terests us for now, the remainder is left for CGamePlayer

          LengthProcessed += Length;
          Bytes = std::vector<uint_fast8_t>(begin(Bytes) + Length, end(Bytes));
          break;
        }

        LengthProcessed += Length;
        Bytes = std::vector<uint_fast8_t>(begin(Bytes) + Length, end(Bytes));
      }
      else
        break;
    }
  }

  *RecvBuffer = RecvBuffer->substr(LengthProcessed);

  // don't call DoSend here because some other players may not have updated yet and may generate a packet for this player
  // also m_Socket may have been set to nullptr during ProcessPackets but we're banking on the fact that m_DeleteMe has been set to true as well so it'll short circuit before dereferencing

  return m_DeleteMe || !m_Socket->GetConnected() || m_Socket->HasError();
}

void CPotentialPlayer::Send(const std::vector<uint_fast8_t>& data) const
{
  if (m_Socket)
    m_Socket->PutBytes(data);
}

//
// CGamePlayer
//

CGamePlayer::CGamePlayer(CPotentialPlayer* potential, uint_fast8_t nPID, string nJoinedRealm, string nName, std::vector<uint_fast8_t> nInternalIP, bool nReserved)
  : m_Protocol(potential->m_Protocol),
    m_Game(potential->m_Game),
    m_Socket(potential->GetSocket()),
    m_InternalIP(std::move(nInternalIP)),
    m_JoinedRealm(std::move(nJoinedRealm)),
    m_Name(std::move(nName)),
    m_TotalPacketsSent(0),
    m_TotalPacketsReceived(1),
    m_LeftCode(PLAYERLEAVE_LOBBY),
    m_SyncCounter(0),
    m_JoinTime(GetTime()),
    m_LastMapPartSent(0),
    m_LastMapPartAcked(0),
    m_FinishedLoadingTicks(0),
    m_StartedLaggingTicks(0),
    m_LastGProxyWaitNoticeSentTime(0),
    m_GProxyReconnectKey(GetTicks()),
    m_LastGProxyAckTime(0),
    m_PID(nPID),
    m_Spoofed(false),
    m_Reserved(nReserved),
    m_WhoisShouldBeSent(false),
    m_WhoisSent(false),
    m_DownloadAllowed(false),
    m_DownloadStarted(false),
    m_DownloadFinished(false),
    m_FinishedLoading(false),
    m_Lagging(false),
    m_DropVote(false),
    m_KickVote(false),
    m_Muted(false),
    m_LeftMessageSent(false),
    m_GProxy(false),
    m_GProxyDisconnectNoticeSent(false),
    m_DeleteMe(false)
{
}

CGamePlayer::~CGamePlayer()
{
  delete m_Socket;
}

uint_fast32_t CGamePlayer::GetPing(bool LCPing) const
{
  // just average all the pings in the vector, nothing fancy

  if (m_Pings.empty())
    return 0;

  uint_fast32_t AvgPing = 0;

  for (const auto& ping : m_Pings)
    AvgPing += ping;

  AvgPing /= m_Pings.size();

  if (LCPing)
    return AvgPing / 2;
  else
    return AvgPing;
}

bool CGamePlayer::Update(void* fd)
{
  const int_fast64_t Time = GetTime();

  // wait 4 seconds after joining before sending the /whois or /w
  // if we send the /whois too early battle.net may not have caught up with where the player is and return erroneous results

  if (m_WhoisShouldBeSent && !m_Spoofed && !m_WhoisSent && !m_JoinedRealm.empty() && Time - m_JoinTime >= 4)
  {
    for (auto& bnet : m_Game->m_Aura->m_BNETs)
    {
      if (bnet->GetServer() == m_JoinedRealm)
      {
        if (m_Game->GetGameState() == GAME_PUBLIC || bnet->GetPvPGN())
          bnet->QueueChatCommand("/whois " + m_Name);
        else if (m_Game->GetGameState() == GAME_PRIVATE)
          bnet->QueueChatCommand(R"(Spoof check by replying to this message with "sc" [ /r sc ])", m_Name, true, string());
      }
    }

    m_WhoisSent = true;
  }

  // check for socket timeouts
  // if we don't receive anything from a player for 30 seconds we can assume they've dropped
  // this works because in the lobby we send pings every 5 seconds and expect a response to each one
  // and in the game the Warcraft 3 client sends keepalives frequently (at least once per second it looks like)

  if (Time - m_Socket->GetLastRecv() >= 30)
    m_Game->EventPlayerDisconnectTimedOut(this);

  // GProxy++ acks

  if (m_GProxy && Time - m_LastGProxyAckTime >= 10)
  {
    m_Socket->PutBytes(m_Game->m_Aura->m_GPSProtocol->SEND_GPSS_ACK(m_TotalPacketsReceived));
    m_LastGProxyAckTime = Time;
  }

  m_Socket->DoRecv((fd_set*)fd);

  // extract as many packets as possible from the socket's receive buffer and process them

  string*                   RecvBuffer      = m_Socket->GetBytes();
  std::vector<uint_fast8_t> Bytes           = CreateByteArray((uint_fast8_t*)RecvBuffer->c_str(), RecvBuffer->size());
  uint_fast32_t             LengthProcessed = 0;

  // a packet is at least 4 bytes so loop as long as the buffer contains 4 bytes

  CIncomingAction*     Action;
  CIncomingChatPlayer* ChatPlayer;
  CIncomingMapSize*    MapSize;
  uint_fast32_t        Pong;

  while (Bytes.size() >= 4)
  {
    // bytes 2 and 3 contain the length of the packet

    const uint_fast16_t             Length = ByteArrayToUInt16(Bytes, false, 2);
    const std::vector<uint_fast8_t> Data   = std::vector<uint_fast8_t>(begin(Bytes), begin(Bytes) + Length);

    if (Bytes[0] == W3GS_HEADER_CONSTANT)
    {
      ++m_TotalPacketsReceived;

      if (Bytes.size() >= Length)
      {
        // byte 1 contains the packet ID

        switch (Bytes[1])
        {
          case CGameProtocol::W3GS_LEAVEGAME:
            m_Game->EventPlayerLeft(this, m_Protocol->RECEIVE_W3GS_LEAVEGAME(Data));
            break;

          case CGameProtocol::W3GS_GAMELOADED_SELF:
            if (m_Protocol->RECEIVE_W3GS_GAMELOADED_SELF(Data))
            {
              if (!m_FinishedLoading)
              {
                m_FinishedLoading      = true;
                m_FinishedLoadingTicks = GetTicks();
                m_Game->EventPlayerLoaded(this);
              }
            }

            break;

          case CGameProtocol::W3GS_OUTGOING_ACTION:
            Action = m_Protocol->RECEIVE_W3GS_OUTGOING_ACTION(Data, m_PID);

            if (Action)
              m_Game->EventPlayerAction(this, Action);

            // don't delete Action here because the game is going to store it in a queue and delete it later

            break;

          case CGameProtocol::W3GS_OUTGOING_KEEPALIVE:
            m_CheckSums.push(m_Protocol->RECEIVE_W3GS_OUTGOING_KEEPALIVE(Data));
            ++m_SyncCounter;
            m_Game->EventPlayerKeepAlive(this);
            break;

          case CGameProtocol::W3GS_CHAT_TO_HOST:
            ChatPlayer = m_Protocol->RECEIVE_W3GS_CHAT_TO_HOST(Data);

            if (ChatPlayer)
              m_Game->EventPlayerChatToHost(this, ChatPlayer);

            delete ChatPlayer;
            break;

          case CGameProtocol::W3GS_DROPREQ:
            if (!m_DropVote)
            {
              m_DropVote = true;
              m_Game->EventPlayerDropRequest(this);
            }

            break;

          case CGameProtocol::W3GS_MAPSIZE:
            MapSize = m_Protocol->RECEIVE_W3GS_MAPSIZE(Data);

            if (MapSize)
              m_Game->EventPlayerMapSize(this, MapSize);

            delete MapSize;
            break;

          case CGameProtocol::W3GS_PONG_TO_HOST:
            Pong = m_Protocol->RECEIVE_W3GS_PONG_TO_HOST(Data);

            // we discard pong values of 1
            // the client sends one of these when connecting plus we return 1 on error to kill two birds with one stone

            if (Pong != 1)
            {
              // we also discard pong values when we're downloading because they're almost certainly inaccurate
              // this statement also gives the player a 5 second grace period after downloading the map to allow queued (i.e. delayed) ping packets to be ignored

              if (!m_DownloadStarted || (m_DownloadFinished && GetTime() - m_FinishedDownloadingTime >= 5))
              {
                // we also discard pong values when anyone else is downloading if we're configured to

                if (!m_Game->IsDownloading())
                {
                  m_Pings.push_back(GetTicks() - Pong);

                  if (m_Pings.size() > 10)
                    m_Pings.erase(begin(m_Pings));
                }
              }
            }

            m_Game->EventPlayerPongToHost(this);
            break;
        }

        LengthProcessed += Length;
        Bytes = std::vector<uint_fast8_t>(begin(Bytes) + Length, end(Bytes));
      }
      else
        break;
    }
    else if (Bytes[0] == GPS_HEADER_CONSTANT)
    {
      if (Length >= 4)
      {
        if (Bytes.size() >= Length)
        {
          if (Bytes[1] == CGPSProtocol::GPS_ACK && Data.size() == 8)
          {
            const uint_fast32_t LastPacket             = ByteArrayToUInt32(Data, false, 4);
            const uint_fast32_t PacketsAlreadyUnqueued = m_TotalPacketsSent - m_GProxyBuffer.size();

            if (LastPacket > PacketsAlreadyUnqueued)
            {
              uint_fast32_t PacketsToUnqueue = LastPacket - PacketsAlreadyUnqueued;

              if (PacketsToUnqueue > m_GProxyBuffer.size())
                PacketsToUnqueue = m_GProxyBuffer.size();

              while (PacketsToUnqueue > 0)
              {
                m_GProxyBuffer.pop();
                --PacketsToUnqueue;
              }
            }
          }
          else if (Bytes[1] == CGPSProtocol::GPS_INIT)
          {
            m_GProxy = true;
            m_Socket->PutBytes(m_Game->m_Aura->m_GPSProtocol->SEND_GPSS_INIT(m_Game->m_Aura->m_ReconnectPort, m_PID, m_GProxyReconnectKey, m_Game->GetGProxyEmptyActions()));
            Print("[GAME: " + m_Game->GetGameName() + "] player [" + m_Name + "] is using GProxy++");
          }
        }

        LengthProcessed += Length;
        Bytes = std::vector<uint_fast8_t>(begin(Bytes) + Length, end(Bytes));
      }
      else
        break;
    }
  }

  *RecvBuffer = RecvBuffer->substr(LengthProcessed);

  // try to find out why we're requesting deletion
  // in cases other than the ones covered here m_LeftReason should have been set when m_DeleteMe was set

  if (m_Socket)
  {
    if (m_Socket->HasError())
    {
      m_Game->EventPlayerDisconnectSocketError(this);
      m_Socket->Reset();
    }
    else if (!m_Socket->GetConnected())
    {
      m_Game->EventPlayerDisconnectConnectionClosed(this);
      m_Socket->Reset();
    }
  }

  if (m_GProxy && m_Game->GetGameLoaded())
    return m_DeleteMe;

  if (m_DeleteMe)
    return true;

  if (m_Socket)
    return m_Socket->HasError() || !m_Socket->GetConnected();

  return false;
}

void CGamePlayer::Send(const std::vector<uint_fast8_t>& data)
{
  // must start counting packet total from beginning of connection
  // but we can avoid buffering packets until we know the client is using GProxy++ since that'll be determined before the game starts
  // this prevents us from buffering packets for non-GProxy++ clients

  ++m_TotalPacketsSent;

  if (m_GProxy && m_Game->GetGameLoaded())
    m_GProxyBuffer.push(data);

  m_Socket->PutBytes(data);
}

void CGamePlayer::EventGProxyReconnect(CTCPSocket* NewSocket, uint_fast32_t LastPacket)
{
  delete m_Socket;
  m_Socket = NewSocket;
  m_Socket->PutBytes(m_Game->m_Aura->m_GPSProtocol->SEND_GPSS_RECONNECT(m_TotalPacketsReceived));

  const uint_fast32_t PacketsAlreadyUnqueued = m_TotalPacketsSent - m_GProxyBuffer.size();

  if (LastPacket > PacketsAlreadyUnqueued)
  {
    uint_fast32_t PacketsToUnqueue = LastPacket - PacketsAlreadyUnqueued;

    if (PacketsToUnqueue > m_GProxyBuffer.size())
      PacketsToUnqueue = m_GProxyBuffer.size();

    while (PacketsToUnqueue > 0)
    {
      m_GProxyBuffer.pop();
      --PacketsToUnqueue;
    }
  }

  // send remaining packets from buffer, preserve buffer

  queue<std::vector<uint_fast8_t>> TempBuffer;

  while (!m_GProxyBuffer.empty())
  {
    m_Socket->PutBytes(m_GProxyBuffer.front());
    TempBuffer.push(m_GProxyBuffer.front());
    m_GProxyBuffer.pop();
  }

  m_GProxyBuffer               = TempBuffer;
  m_GProxyDisconnectNoticeSent = false;
  m_Game->SendAllChat("Player [" + m_Name + "] reconnected with GProxy++!");
}
