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
// CTCPSocket
//

class CTCPSocket
{
protected:
  SOCKET m_Socket;
  struct sockaddr_in m_SIN;
  int m_Error;
  bool m_HasError;
  bool m_Connected;

private:
  string m_RecvBuffer;
  string m_SendBuffer;
  uint32_t m_LastRecv;

public:
  CTCPSocket( );
  CTCPSocket( SOCKET nSocket, struct sockaddr_in nSIN );
  ~CTCPSocket( );

  BYTEARRAY GetPort( ) const;
  BYTEARRAY GetIP( ) const;
  string GetIPString( ) const;
  string GetErrorString( ) const;
  string *GetBytes( )								{ return &m_RecvBuffer; }
  int GetError( ) const							{ return m_Error; }
  uint32_t GetLastRecv( ) const		  { return m_LastRecv; }
  bool HasError( ) const						{ return m_HasError; }
  bool GetConnected( ) const				{ return m_Connected; }

  void SetFD( fd_set *fd, fd_set *send_fd, int *nfds );
  void Reset( );
  void PutBytes( const string &bytes );
  void PutBytes( const BYTEARRAY &bytes );

  void ClearRecvBuffer( )										{ m_RecvBuffer.clear( ); }
  void SubstrRecvBuffer( unsigned int i )		{ m_RecvBuffer = m_RecvBuffer.substr( i ); }
  void ClearSendBuffer( )										{ m_SendBuffer.clear( ); }

  void DoRecv( fd_set *fd );
  void DoSend( fd_set *send_fd );
  void Disconnect( );
};

//
// CTCPClient
//

class CTCPClient
{
protected:
  SOCKET m_Socket;
  struct sockaddr_in m_SIN;
  int m_Error;
  bool m_HasError;
  bool m_Connected;
  bool m_Connecting;

private:
  string m_RecvBuffer;
  string m_SendBuffer;

public:
  CTCPClient( );
  ~CTCPClient( );

  BYTEARRAY GetPort( ) const;
	BYTEARRAY GetIP( ) const;
	string GetIPString( ) const;
	string GetErrorString( ) const;
	string *GetBytes( )								{ return &m_RecvBuffer; }
	int GetError( ) const							{ return m_Error; }
	bool HasError( ) const						{ return m_HasError; }
	bool GetConnected( ) const				{ return m_Connected; }
  bool GetConnecting( ) const			  { return m_Connecting; }

  void SetFD( fd_set *fd, fd_set *send_fd, int *nfds );
  void Reset( );
  void PutBytes( const string &bytes );
  void PutBytes( const BYTEARRAY &bytes );

  bool CheckConnect( );
  void ClearRecvBuffer( )										{ m_RecvBuffer.clear( ); }
  void SubstrRecvBuffer( unsigned int i )		{ m_RecvBuffer = m_RecvBuffer.substr( i ); }
  void ClearSendBuffer( )										{ m_SendBuffer.clear( ); }
  void FlushRecv( fd_set *fd );
  void DoRecv( fd_set *fd );
  void DoSend( fd_set *send_fd );
  void SetNoDelay( );
  void Disconnect( );
  void Connect( const string &localaddress, const string &address, uint16_t port );
};

//
// CTCPServer
//

class CTCPServer
{
protected:
  SOCKET m_Socket;
  struct sockaddr_in m_SIN;
  int m_Error;
  bool m_HasError;

public:
  CTCPServer( );
  ~CTCPServer( );

  string GetErrorString( ) const;
  bool HasError( ) const										{ return m_HasError; }
  int GetError( ) const											{ return m_Error; }

  bool Listen( const string &address, uint16_t port );
  void SetFD( fd_set *fd, fd_set *send_fd, int *nfds );
  CTCPSocket *Accept( fd_set *fd );
};

//
// CUDPSocket
//

class CUDPSocket
{
protected:
  SOCKET m_Socket;
  struct sockaddr_in m_SIN;
  struct in_addr m_BroadcastTarget;
  int m_Error;
  bool m_HasError;

public:
  CUDPSocket( );
  ~CUDPSocket( );

  BYTEARRAY GetPort( ) const;
	BYTEARRAY GetIP( ) const;
	string GetIPString( ) const;
	string GetErrorString( ) const;
  bool HasError( ) const										{ return m_HasError; }
  int GetError( ) const							{ return m_Error; }
  
  bool SendTo( struct sockaddr_in sin, const BYTEARRAY &message );
  bool SendTo( const string &address, uint16_t port, const BYTEARRAY &message );
  bool Broadcast( uint16_t port, const BYTEARRAY &message );

  void SetFD( fd_set *fd, fd_set *send_fd, int *nfds );
  void Allocate( int type );
  void Reset( );
  void SetBroadcastTarget( const string &subnet );
  void SetDontRoute( bool dontRoute );
};

#endif
