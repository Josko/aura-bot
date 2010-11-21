
#define LF '\x0A'

class CGHost;
class CTCPClient;
class CDCC;

class CIRC
{
public: 
	CGHost *m_GHost;
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
	
	CIRC( CGHost *nGHost, string nServer, string nNickname, string nUsername, string nPassword, vector<string> nChannels, uint16_t nPort, string nCommandTrigger, vector<string> nLocals );
	~CIRC( );

	unsigned int SetFD( void *fd, void *send_fd, int *nfds );
	bool Update( void *fd, void *send_fd );
	inline void ExtractPackets( );
	void SendIRC( const string &message );
	void SendDCC( const string &message );
	void SendMessageIRC( const string &message, const string &target );
};

class CGHost;
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

