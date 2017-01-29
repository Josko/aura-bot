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

#ifndef AURA_IRC_H_
#define AURA_IRC_H_

#include <vector>
#include <string>
#include <cstdint>

#define LF ('\x0A')

class CAura;
class CTCPClient;

class CIRC
{
public:
  CAura*                   m_Aura;
  CTCPClient*              m_Socket;
  std::vector<std::string> m_Channels;
  std::vector<std::string> m_RootAdmins;
  std::string              m_Server;
  std::string              m_ServerIP;
  std::string              m_Nickname;
  std::string              m_NicknameCpy;
  std::string              m_Username;
  std::string              m_Password;
  int64_t                  m_LastConnectionAttemptTime;
  int64_t                  m_LastPacketTime;
  int64_t                  m_LastAntiIdleTime;
  uint16_t                 m_Port;
  int8_t                   m_CommandTrigger;
  bool                     m_Exiting;
  bool                     m_WaitingToConnect;
  bool                     m_OriginalNick;

  CIRC(CAura* nAura, std::string nServer, const std::string& nNickname, const std::string& nUsername, std::string nPassword, std::vector<std::string> nChannels, std::vector<std::string> nRootAdmins, uint16_t nPort, int8_t nCommandTrigger);
  ~CIRC();
  CIRC(CIRC&) = delete;

  uint32_t SetFD(void* fd, void* send_fd, int32_t* nfds);
  bool Update(void* fd, void* send_fd);
  void ExtractPackets();
  void SendIRC(const std::string& message);
  void SendMessageIRC(const std::string& message, const std::string& target);
};

#endif // AURA_IRC_H_
