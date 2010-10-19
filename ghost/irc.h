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
	uint32_t m_LastPacketTime;

	CIRC( CGHost *nGHost, string nServer, string nNickname, string nUsername, string nPassword, vector<string> nChannels, uint16_t nPort, string nCommandTrigger, vector<string> nLocals );
	~CIRC( );

	unsigned int SetFD( void *fd, void *send_fd, int *nfds );
	bool Update( void *fd, void *send_fd );
	inline void ExtractPackets( );
	void SendIRC( const string &message );
	void SendDCC( const string &message );
	void PrivMsg( const string &message, const string &target );
	unsigned long ToInt( const string &s );
};

class CGHost;
class CIRC;
class CTCPClient;

class CDCC
{
public:
	CIRC *m_IRC;
	CTCPClient *m_Socket;
	string m_IP;
	uint16_t m_Port;
	const string m_Nickname;

	CDCC( CIRC *nIRC, const string nIP, const uint16_t nPort, const string nNickname );
	~CDCC( );

	unsigned int SetFD( void *fd, void *send_fd, int *nfds );
	void Update( void *fd, void *send_fd );
	void Connect( string IP, uint32_t Port );
};

