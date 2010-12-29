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

#define LF ('\x0A')
#define CR ('\x0D')
#define SOH ('\x01')

class CAura;
class CTCPClient;
class CDCC;

class CIRC
{
public:
	CAura *m_Aura;
	CTCPClient *m_Socket;
	vector<CDCC *> m_DCC;
	vector<string> m_Locals;
	vector<string> m_Channels;
	string m_Server;
	string m_ServerIP;
	string m_Nickname;
	string m_NicknameCpy;
	string m_Username;
	string m_CommandTrigger;
	string m_Password;
	uint16_t m_Port;
	bool m_Exiting;
	bool m_WaitingToConnect;
	bool m_OriginalNick;
	uint32_t m_LastConnectionAttemptTime;
	uint32_t m_LastPacketTime;
	uint32_t m_LastAntiIdleTime;

	CIRC( CAura *nAura, string nServer, string nNickname, string nUsername, string nPassword, vector<string> nChannels, uint16_t nPort, string nCommandTrigger, vector<string> nLocals );
	~CIRC( );

	unsigned int SetFD( void *fd, void *send_fd, int *nfds );
	bool Update( void *fd, void *send_fd );
	inline void ExtractPackets( );
	void SendIRC( const string &message );
	void SendDCC( const string &message );
	void SendMessageIRC( const string &message, const string &target );
};

class CAura;
class CIRC;
class CTCPClient;

class CDCC
{
public:
	CTCPClient *m_Socket;
	string m_Nickname;
	CIRC *m_IRC;
	string m_IP;
	uint16_t m_Port;

	CDCC( CIRC *nIRC, string nIP, uint16_t nPort, const string &nNickname );
	~CDCC( );

	unsigned int SetFD( void *fd, void *send_fd, int *nfds );
	void Update( void *fd, void *send_fd );
	void Connect( const string &IP, uint16_t Port );
};

