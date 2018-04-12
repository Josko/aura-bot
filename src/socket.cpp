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

#include <cstring>

#include "aura.h"
#include "util.h"
#include "socket.h"
#include "includes.h"

using namespace std;

#ifndef WIN32
int32_t GetLastError()
{
  return errno;
}
#endif

//
// CSocket
//

CSocket::CSocket()
  : m_Socket(INVALID_SOCKET),
    m_HasError(false),
    m_Error(0)
{
  memset(&m_SIN, 0, sizeof(m_SIN));
}

CSocket::CSocket(SOCKET nSocket, struct sockaddr_in nSIN)
  : m_Socket(nSocket),
    m_SIN(nSIN),
    m_HasError(false),
    m_Error(0)
{
}

CSocket::~CSocket()
{
  if (m_Socket != INVALID_SOCKET)
    closesocket(m_Socket);
}

string CSocket::GetErrorString() const
{
  if (!m_HasError)
    return "NO ERROR";

  switch (m_Error)
  {
    case EWOULDBLOCK:
      return "EWOULDBLOCK";
    case EINPROGRESS:
      return "EINPROGRESS";
    case EALREADY:
      return "EALREADY";
    case ENOTSOCK:
      return "ENOTSOCK";
    case EDESTADDRREQ:
      return "EDESTADDRREQ";
    case EMSGSIZE:
      return "EMSGSIZE";
    case EPROTOTYPE:
      return "EPROTOTYPE";
    case ENOPROTOOPT:
      return "ENOPROTOOPT";
    case EPROTONOSUPPORT:
      return "EPROTONOSUPPORT";
    case ESOCKTNOSUPPORT:
      return "ESOCKTNOSUPPORT";
    case EOPNOTSUPP:
      return "EOPNOTSUPP";
    case EPFNOSUPPORT:
      return "EPFNOSUPPORT";
    case EAFNOSUPPORT:
      return "EAFNOSUPPORT";
    case EADDRINUSE:
      return "EADDRINUSE";
    case EADDRNOTAVAIL:
      return "EADDRNOTAVAIL";
    case ENETDOWN:
      return "ENETDOWN";
    case ENETUNREACH:
      return "ENETUNREACH";
    case ENETRESET:
      return "ENETRESET";
    case ECONNABORTED:
      return "ECONNABORTED";
    case ENOBUFS:
      return "ENOBUFS";
    case EISCONN:
      return "EISCONN";
    case ENOTCONN:
      return "ENOTCONN";
    case ESHUTDOWN:
      return "ESHUTDOWN";
    case ETOOMANYREFS:
      return "ETOOMANYREFS";
    case ETIMEDOUT:
      return "ETIMEDOUT";
    case ECONNREFUSED:
      return "ECONNREFUSED";
    case ELOOP:
      return "ELOOP";
    case ENAMETOOLONG:
      return "ENAMETOOLONG";
    case EHOSTDOWN:
      return "EHOSTDOWN";
    case EHOSTUNREACH:
      return "EHOSTUNREACH";
    case ENOTEMPTY:
      return "ENOTEMPTY";
    case EUSERS:
      return "EUSERS";
    case EDQUOT:
      return "EDQUOT";
    case ESTALE:
      return "ESTALE";
    case EREMOTE:
      return "EREMOTE";
    case ECONNRESET:
      return "Connection reset by peer";
  }

  return "UNKNOWN ERROR (" + to_string(m_Error) + ")";
}

void CSocket::SetFD(fd_set* fd, fd_set* send_fd, int* nfds)
{
  if (m_Socket == INVALID_SOCKET)
    return;

  FD_SET(m_Socket, fd);
  FD_SET(m_Socket, send_fd);

#ifndef WIN32
  if (m_Socket > *nfds)
    *nfds = m_Socket;
#endif
}

void CSocket::Allocate(int type)
{
  m_Socket = socket(AF_INET, type, 0);

  if (m_Socket == INVALID_SOCKET)
  {
    m_HasError = true;
    m_Error    = GetLastError();
    Print("[SOCKET] error (socket) - " + GetErrorString());
    return;
  }
}

void CSocket::Reset()
{
  if (m_Socket != INVALID_SOCKET)
    closesocket(m_Socket);

  m_Socket = INVALID_SOCKET;
  memset(&m_SIN, 0, sizeof(m_SIN));
  m_HasError = false;
  m_Error    = 0;
}

//
// CTCPSocket
//

CTCPSocket::CTCPSocket()
  : CSocket(),
    m_LastRecv(GetTime()),
    m_Connected(false)
{
  Allocate(SOCK_STREAM);

// make socket non blocking

#ifdef WIN32
  int32_t iMode = 1;
  ioctlsocket(m_Socket, FIONBIO, (u_long FAR*)&iMode);
#else
  fcntl(m_Socket, F_SETFL, fcntl(m_Socket, F_GETFL) | O_NONBLOCK);
#endif

  // disable Nagle's algorithm

  int32_t OptVal = 1;
  setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&OptVal), sizeof(int32_t));
}

CTCPSocket::CTCPSocket(SOCKET nSocket, struct sockaddr_in nSIN)
  : CSocket(nSocket, nSIN),
    m_LastRecv(GetTime()),
    m_Connected(true)
{
// make socket non blocking

#ifdef WIN32
  int32_t iMode = 1;
  ioctlsocket(m_Socket, FIONBIO, (u_long FAR*)&iMode);
#else
  fcntl(m_Socket, F_SETFL, fcntl(m_Socket, F_GETFL) | O_NONBLOCK);
#endif
}

CTCPSocket::~CTCPSocket()
{
  if (m_Socket != INVALID_SOCKET)
    closesocket(m_Socket);
}

void CTCPSocket::Reset()
{
  CSocket::Reset();
  Allocate(SOCK_STREAM);

  m_Connected = false;
  m_RecvBuffer.clear();
  m_SendBuffer.clear();
  m_LastRecv = GetTime();

// make socket non blocking

#ifdef WIN32
  int32_t iMode = 1;
  ioctlsocket(m_Socket, FIONBIO, (u_long FAR*)&iMode);
#else
  fcntl(m_Socket, F_SETFL, fcntl(m_Socket, F_GETFL) | O_NONBLOCK);
#endif
}

void CTCPSocket::DoRecv(fd_set* fd)
{
  if (m_Socket == INVALID_SOCKET || m_HasError || !m_Connected)
    return;

  if (FD_ISSET(m_Socket, fd))
  {
    // data is waiting, receive it

    char    buffer[1024];
    int32_t c = recv(m_Socket, buffer, 1024, 0);

    if (c > 0)
    {
      // success! add the received data to the buffer

      m_RecvBuffer += string(buffer, c);
      m_LastRecv = GetTime();
    }
    else if (c == SOCKET_ERROR && GetLastError() != EWOULDBLOCK)
    {
      // receive error

      m_HasError = true;
      m_Error    = GetLastError();
      Print("[TCPSOCKET] error (recv) - " + GetErrorString());
      return;
    }
    else if (c == 0)
    {
      // the other end closed the connection

      Print("[TCPSOCKET] closed by remote host");
      m_Connected = false;
    }
  }
}

void CTCPSocket::DoSend(fd_set* send_fd)
{
  if (m_Socket == INVALID_SOCKET || m_HasError || !m_Connected || m_SendBuffer.empty())
    return;

  if (FD_ISSET(m_Socket, send_fd))
  {
    // socket is ready, send it

    int32_t s = send(m_Socket, m_SendBuffer.c_str(), static_cast<int32_t>(m_SendBuffer.size()), MSG_NOSIGNAL);

    if (s > 0)
    {
      // success! only some of the data may have been sent, remove it from the buffer

      m_SendBuffer = m_SendBuffer.substr(s);
    }
    else if (s == SOCKET_ERROR && GetLastError() != EWOULDBLOCK)
    {
      // send error

      m_HasError = true;
      m_Error    = GetLastError();
      Print("[TCPSOCKET] error (send) - " + GetErrorString());
      return;
    }
  }
}

void CTCPSocket::Disconnect()
{
  if (m_Socket != INVALID_SOCKET)
    shutdown(m_Socket, SHUT_RDWR);

  m_Connected = false;
}

//
// CTCPClient
//

CTCPClient::CTCPClient()
  : CTCPSocket(),
    m_Connecting(false)
{
}

CTCPClient::~CTCPClient()
{
}

void CTCPClient::Reset()
{
  CTCPSocket::Reset();
  m_Connecting = false;
}

void CTCPClient::Disconnect()
{
  if (m_Socket != INVALID_SOCKET)
    shutdown(m_Socket, SHUT_RDWR);

  m_Connected  = false;
  m_Connecting = false;
}

void CTCPClient::Connect(const string& localaddress, const string& address, uint16_t port)
{
  if (m_Socket == INVALID_SOCKET || m_HasError || m_Connecting || m_Connected)
    return;

  if (!localaddress.empty())
  {
    struct sockaddr_in LocalSIN;
    memset(&LocalSIN, 0, sizeof(LocalSIN));
    LocalSIN.sin_family = AF_INET;

    if ((LocalSIN.sin_addr.s_addr = inet_addr(localaddress.c_str())) == INADDR_NONE)
      LocalSIN.sin_addr.s_addr = INADDR_ANY;

    LocalSIN.sin_port = htons(0);

    if (::bind(m_Socket, reinterpret_cast<struct sockaddr*>(&LocalSIN), sizeof(LocalSIN)) == SOCKET_ERROR)
    {
      m_HasError = true;
      m_Error    = GetLastError();
      Print("[TCPCLIENT] error (bind) - " + GetErrorString());
      return;
    }
  }

  // get IP address

  struct hostent* HostInfo;
  uint32_t        HostAddress;
  HostInfo = gethostbyname(address.c_str());

  if (!HostInfo)
  {
    m_HasError = true;
    // m_Error = h_error;
    Print("[TCPCLIENT] error (gethostbyname)");
    return;
  }

  memcpy(&HostAddress, HostInfo->h_addr, HostInfo->h_length);

  // connect

  m_SIN.sin_family      = AF_INET;
  m_SIN.sin_addr.s_addr = HostAddress;
  m_SIN.sin_port        = htons(port);

  if (connect(m_Socket, reinterpret_cast<struct sockaddr*>(&m_SIN), sizeof(m_SIN)) == SOCKET_ERROR)
  {
    if (GetLastError() != EINPROGRESS && GetLastError() != EWOULDBLOCK)
    {
      // connect error

      m_HasError = true;
      m_Error    = GetLastError();
      Print("[TCPCLIENT] error (connect) - " + GetErrorString());
      return;
    }
  }

  m_Connecting = true;
}

bool CTCPClient::CheckConnect()
{
  if (m_Socket == INVALID_SOCKET || m_HasError || !m_Connecting)
    return false;

  fd_set fd;
  FD_ZERO(&fd);
  FD_SET(m_Socket, &fd);

  struct timeval tv;
  tv.tv_sec  = 0;
  tv.tv_usec = 0;

// check if the socket is connected

#ifdef WIN32
  if (select(1, nullptr, &fd, nullptr, &tv) == SOCKET_ERROR)
#else
  if (select(m_Socket + 1, nullptr, &fd, nullptr, &tv) == SOCKET_ERROR)
#endif
  {
    m_HasError = true;
    m_Error    = GetLastError();
    return false;
  }

  if (FD_ISSET(m_Socket, &fd))
  {
    m_Connecting = false;
    m_Connected  = true;
    return true;
  }

  return false;
}

void CTCPClient::DoRecv(fd_set* fd)
{
  CTCPSocket::DoRecv(fd);
}

void CTCPClient::DoSend(fd_set* send_fd)
{
  CTCPSocket::DoSend(send_fd);
}

//
// CTCPServer
//

CTCPServer::CTCPServer()
  : CTCPSocket()
{
// make socket non blocking

#ifdef WIN32
  int32_t iMode = 1;
  ioctlsocket(m_Socket, FIONBIO, (u_long FAR*)&iMode);
#else
  fcntl(m_Socket, F_SETFL, fcntl(m_Socket, F_GETFL) | O_NONBLOCK);
#endif

  // set the socket to reuse the address in case it hasn't been released yet

  int32_t optval = 1;

#ifdef WIN32
  setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(int32_t));
#else
  setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int32_t));
#endif
}

CTCPServer::~CTCPServer()
{
}

bool CTCPServer::Listen(const string& address, uint16_t port)
{
  if (m_Socket == INVALID_SOCKET || m_HasError)
    return false;

  m_SIN.sin_family = AF_INET;

  if (!address.empty())
  {
    if ((m_SIN.sin_addr.s_addr = inet_addr(address.c_str())) == INADDR_NONE)
      m_SIN.sin_addr.s_addr = INADDR_ANY;
  }
  else
    m_SIN.sin_addr.s_addr = INADDR_ANY;

  m_SIN.sin_port = htons(port);

  if (::bind(m_Socket, reinterpret_cast<struct sockaddr*>(&m_SIN), sizeof(m_SIN)) == SOCKET_ERROR)
  {
    m_HasError = true;
    m_Error    = GetLastError();
    Print("[TCPSERVER] error (bind) - " + GetErrorString());
    return false;
  }

  // listen, queue length 8

  if (listen(m_Socket, 8) == SOCKET_ERROR)
  {
    m_HasError = true;
    m_Error    = GetLastError();
    Print("[TCPSERVER] error (listen) - " + GetErrorString());
    return false;
  }

  return true;
}

CTCPSocket* CTCPServer::Accept(fd_set* fd)
{
  if (m_Socket == INVALID_SOCKET || m_HasError)
    return nullptr;

  if (FD_ISSET(m_Socket, fd))
  {
    // a connection is waiting, accept it

    struct sockaddr_in Addr;
    int32_t            AddrLen = sizeof(Addr);
    SOCKET             NewSocket;

#ifdef WIN32
    if ((NewSocket = accept(m_Socket, (struct sockaddr*)&Addr, &AddrLen)) != INVALID_SOCKET)
#else
    if ((NewSocket = accept(m_Socket, reinterpret_cast<struct sockaddr*>(&Addr), reinterpret_cast<socklen_t*>(&AddrLen))) != INVALID_SOCKET)
#endif
    {
      // success! return the new socket

      return new CTCPSocket(NewSocket, Addr);
    }
  }

  return nullptr;
}

//
// CUDPSocket
//

CUDPSocket::CUDPSocket()
  : CSocket()
{
  Allocate(SOCK_DGRAM);

  // enable broadcast support

  int32_t OptVal = 1;
  setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&OptVal), sizeof(int32_t));

  // set default broadcast target

  m_BroadcastTarget.s_addr = INADDR_BROADCAST;
}

CUDPSocket::~CUDPSocket()
{
}

bool CUDPSocket::SendTo(struct sockaddr_in sin, const std::vector<uint8_t>& message)
{
  if (m_Socket == INVALID_SOCKET || m_HasError)
    return false;

  const string MessageString = string(begin(message), end(message));

  if (sendto(m_Socket, MessageString.c_str(), MessageString.size(), 0, reinterpret_cast<struct sockaddr*>(&sin), sizeof(sin)) == -1)
    return false;

  return true;
}

bool CUDPSocket::SendTo(const string& address, uint16_t port, const std::vector<uint8_t>& message)
{
  if (m_Socket == INVALID_SOCKET || m_HasError)
    return false;

  // get IP address

  struct hostent* HostInfo;
  uint32_t        HostAddress;
  HostInfo = gethostbyname(address.c_str());

  if (!HostInfo)
  {
    m_HasError = true;
    // m_Error = h_error;
    Print("[UDPSOCKET] error (gethostbyname)");
    return false;
  }

  memcpy(&HostAddress, HostInfo->h_addr, HostInfo->h_length);
  struct sockaddr_in sin;
  sin.sin_family      = AF_INET;
  sin.sin_addr.s_addr = HostAddress;
  sin.sin_port        = htons(port);

  return SendTo(sin, message);
}

bool CUDPSocket::Broadcast(uint16_t port, const std::vector<uint8_t>& message)
{
  if (m_Socket == INVALID_SOCKET || m_HasError)
    return false;

  struct sockaddr_in sin;
  sin.sin_family      = AF_INET;
  sin.sin_addr.s_addr = m_BroadcastTarget.s_addr;
  sin.sin_port        = htons(port);

  const string MessageString = string(begin(message), end(message));

  if (sendto(m_Socket, MessageString.c_str(), MessageString.size(), 0, reinterpret_cast<struct sockaddr*>(&sin), sizeof(sin)) == -1)
  {
    Print("[UDPSOCKET] failed to broadcast packet (port " + to_string(port) + ", size " + to_string(MessageString.size()) + " bytes)");
    return false;
  }

  return true;
}

void CUDPSocket::SetBroadcastTarget(const string& subnet)
{
  if (subnet.empty())
  {
    Print("[UDPSOCKET] using default broadcast target");
    m_BroadcastTarget.s_addr = INADDR_BROADCAST;
  }
  else
  {
    // this function does not check whether the given subnet is a valid subnet the user is on
    // convert string representation of ip/subnet to in_addr

    Print("[UDPSOCKET] using broadcast target [" + subnet + "]");
    m_BroadcastTarget.s_addr = inet_addr(subnet.c_str());

    // if conversion fails, inet_addr( ) returns INADDR_NONE

    if (m_BroadcastTarget.s_addr == INADDR_NONE)
    {
      Print("[UDPSOCKET] invalid broadcast target, using default broadcast target");
      m_BroadcastTarget.s_addr = INADDR_BROADCAST;
    }
  }
}

void CUDPSocket::SetDontRoute(bool dontRoute)
{
  int32_t OptVal = 0;

  if (dontRoute)
    OptVal = 1;

  // don't route packets; make them ignore routes set by routing table and send them to the interface
  // belonging to the target address directly

  setsockopt(m_Socket, SOL_SOCKET, SO_DONTROUTE, reinterpret_cast<const char*>(&OptVal), sizeof(int32_t));
}

void CUDPSocket::Reset()
{
  CSocket::Reset();
  Allocate(SOCK_DGRAM);

  // enable broadcast support

  int32_t OptVal = 1;
  setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&OptVal), sizeof(int32_t));

  // set default broadcast target

  m_BroadcastTarget.s_addr = INADDR_BROADCAST;
}
