/*

   Copyright [2008] [Trevor Hogan]

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

#ifndef SOCKET_H
#define SOCKET_H

#ifdef WIN32
 #include <winsock2.h>
 #include <errno.h>

 #define EADDRINUSE WSAEADDRINUSE
 #define EADDRNOTAVAIL WSAEADDRNOTAVAIL
 #define EAFNOSUPPORT WSAEAFNOSUPPORT
 #define EALREADY WSAEALREADY
 #define ECONNABORTED WSAECONNABORTED
 #define ECONNREFUSED WSAECONNREFUSED
 #define ECONNRESET WSAECONNRESET
 #define EDESTADDRREQ WSAEDESTADDRREQ
 #define EDQUOT WSAEDQUOT
 #define EHOSTDOWN WSAEHOSTDOWN
 #define EHOSTUNREACH WSAEHOSTUNREACH
 #define EINPROGRESS WSAEINPROGRESS
 #define EISCONN WSAEISCONN
 #define ELOOP WSAELOOP
 #define EMSGSIZE WSAEMSGSIZE
 // #define ENAMETOOLONG WSAENAMETOOLONG
 #define ENETDOWN WSAENETDOWN
 #define ENETRESET WSAENETRESET
 #define ENETUNREACH WSAENETUNREACH
 #define ENOBUFS WSAENOBUFS
 #define ENOPROTOOPT WSAENOPROTOOPT
 #define ENOTCONN WSAENOTCONN
 // #define ENOTEMPTY WSAENOTEMPTY
 #define ENOTSOCK WSAENOTSOCK
 #define EOPNOTSUPP WSAEOPNOTSUPP
 #define EPFNOSUPPORT WSAEPFNOSUPPORT
 #define EPROTONOSUPPORT WSAEPROTONOSUPPORT
 #define EPROTOTYPE WSAEPROTOTYPE
 #define EREMOTE WSAEREMOTE
 #define ESHUTDOWN WSAESHUTDOWN
 #define ESOCKTNOSUPPORT WSAESOCKTNOSUPPORT
 #define ESTALE WSAESTALE
 #define ETIMEDOUT WSAETIMEDOUT
 #define ETOOMANYREFS WSAETOOMANYREFS
 #define EUSERS WSAEUSERS
 #define EWOULDBLOCK WSAEWOULDBLOCK
#else
 #include <arpa/inet.h>
 #include <errno.h>
 #include <fcntl.h>
 #include <netdb.h>
 #include <netinet/in.h>
 #include <netinet/tcp.h>
 #include <sys/ioctl.h>
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <unistd.h>

 typedef int SOCKET;

 #define INVALID_SOCKET -1
 #define SOCKET_ERROR -1

 #define closesocket close

 extern int GetLastError( );
#endif

#ifndef INADDR_NONE
 #define INADDR_NONE -1
#endif

#ifndef MSG_NOSIGNAL
 #define MSG_NOSIGNAL 0
#endif

#ifdef WIN32
 #define SHUT_RDWR 2
#endif

//
// CSocket
//

class CSocket
{
protected:
	SOCKET m_Socket;
	struct sockaddr_in m_SIN;
	bool m_HasError;
	int m_Error;

public:
	CSocket( );
	CSocket( SOCKET nSocket, struct sockaddr_in nSIN );
	~CSocket( );

	virtual BYTEARRAY GetPort( );
	virtual BYTEARRAY GetIP( );
	virtual string GetIPString( );
	virtual bool HasError( )						{ return m_HasError; }
	virtual int GetError( )							{ return m_Error; }
	virtual string GetErrorString( );
	virtual void SetFD( fd_set *fd, fd_set *send_fd, int *nfds );
	virtual void Allocate( int type );
	virtual void Reset( );
};

//
// CTCPSocket
//

class CTCPSocket : public CSocket
{
protected:
	bool m_Connected;

private:
	string m_RecvBuffer;
	string m_SendBuffer;
	uint32_t m_LastRecv;
	uint32_t m_LastSend;

public:
	CTCPSocket( );
	CTCPSocket( SOCKET nSocket, struct sockaddr_in nSIN );
	virtual ~CTCPSocket( );

	virtual void Reset( );
	virtual bool GetConnected( )				{ return m_Connected; }
	virtual string *GetBytes( )					{ return &m_RecvBuffer; }
	virtual void PutBytes( const string &bytes );
	virtual void PutBytes( const BYTEARRAY &bytes );
	virtual void ClearRecvBuffer( )				{ m_RecvBuffer.clear( ); }
	virtual void ClearSendBuffer( )				{ m_SendBuffer.clear( ); }
	virtual uint32_t GetLastRecv( )				{ return m_LastRecv; }
	virtual uint32_t GetLastSend( )				{ return m_LastSend; }
	virtual void DoRecv( fd_set *fd );
	virtual void DoSend( fd_set *send_fd );
	virtual void Disconnect( );
	virtual void SetNoDelay( );
};

//
// CTCPClient
//

class CTCPClient : public CTCPSocket
{
protected:
	bool m_Connecting;

public:
	CTCPClient( );
	virtual ~CTCPClient( );

	virtual void Reset( );
	virtual void Disconnect( );
	virtual bool GetConnecting( )												{ return m_Connecting; }
	virtual void Connect( const string &localaddress, const string &address, uint16_t port );
	virtual bool CheckConnect( );
};

//
// CTCPServer
//

class CTCPServer : public CTCPSocket
{
public:
	CTCPServer( );
	virtual ~CTCPServer( );

	virtual bool Listen( const string &address, uint16_t port );
	virtual CTCPSocket *Accept( fd_set *fd );
};

//
// CUDPSocket
//

class CUDPSocket : public CSocket
{
protected:
	struct in_addr m_BroadcastTarget;
public:
	CUDPSocket( );
	virtual ~CUDPSocket( );

	virtual bool SendTo( struct sockaddr_in sin, const BYTEARRAY &message );
	virtual bool SendTo( const string &address, uint16_t port, const BYTEARRAY &message );
	virtual bool Broadcast( uint16_t port, const BYTEARRAY &message );
	virtual void SetBroadcastTarget( const string &subnet );
	virtual void SetDontRoute( bool dontRoute );
};

//
// CUDPServer
//

class CUDPServer : public CUDPSocket
{
public:
	CUDPServer( );
	virtual ~CUDPServer( );

	virtual bool Bind( struct sockaddr_in sin );
	virtual bool Bind( const string &address, uint16_t port );
	virtual void RecvFrom( fd_set *fd, struct sockaddr_in *sin, string *message );
};

#endif
