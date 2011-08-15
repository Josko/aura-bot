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

#ifndef IRC_H
#define IRC_H

#define LF ('\x0A')

class CAura;
class CTCPClient;

class CIRC
{
public:
  CAura *m_Aura;
  CTCPClient *m_Socket;
  vector<string> m_Channels;
  vector<string> m_RootAdmins;
  string m_Server;
  string m_ServerIP;
  string m_Nickname;
  string m_NicknameCpy;
  string m_Username;
  char m_CommandTrigger;
  string m_Password;
  uint16_t m_Port;
  bool m_Exiting;
  bool m_WaitingToConnect;
  bool m_OriginalNick;
  uint32_t m_LastConnectionAttemptTime;
  uint32_t m_LastPacketTime;
  uint32_t m_LastAntiIdleTime;

  CIRC( CAura *nAura, const string &nServer, const string &nNickname, const string &nUsername, const string &nPassword, const vector<string> &nChannels, const vector<string> &nRootAdmins,uint16_t nPort, char nCommandTrigger );
  ~CIRC( );

  unsigned int SetFD( void *fd, void *send_fd, int *nfds );
  bool Update( void *fd, void *send_fd );
  void ExtractPackets( );
  void SendIRC( const string &message );
  void SendMessageIRC( const string &message, const string &target );
};

#endif
