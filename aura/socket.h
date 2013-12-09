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

#ifndef AURA_SOCKET_H_
#define AURA_SOCKET_H_

#include "util.h"

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

typedef int32_t SOCKET;

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#define closesocket close

extern int32_t GetLastError();
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
  struct sockaddr_in m_SIN;

private:
  std::string m_RecvBuffer;
  std::string m_SendBuffer;
  uint32_t m_LastRecv;

protected:
  SOCKET m_Socket;
  int32_t m_Error;
  bool m_HasError;
  bool m_Connected;

public:
  CTCPSocket();
  CTCPSocket(SOCKET nSocket, struct sockaddr_in nSIN);
  ~CTCPSocket();

  std::string GetErrorString() const;
  inline BYTEARRAY GetPort() const                        { return CreateByteArray(m_SIN.sin_port, false); }
  inline BYTEARRAY GetIP() const                          { return CreateByteArray((uint32_t) m_SIN.sin_addr.s_addr, false); }
  inline std::string GetIPString() const                       { return inet_ntoa(m_SIN.sin_addr); }
  inline std::string *GetBytes()                               { return &m_RecvBuffer; }
  inline int32_t GetError() const                             { return m_Error; }
  inline uint32_t GetLastRecv() const                     { return m_LastRecv; }
  inline bool HasError() const                            { return m_HasError; }
  inline bool GetConnected() const                        { return m_Connected; }

  void SetFD(fd_set *fd, fd_set *send_fd, int32_t *nfds);
  void Reset();
  inline void PutBytes(const std::string &bytes)              { m_SendBuffer += bytes; }
  inline void PutBytes(const BYTEARRAY &bytes)           { m_SendBuffer += std::string(begin(bytes), end(bytes)); }

  inline void ClearRecvBuffer()                           { m_RecvBuffer.clear(); }
  inline void SubstrRecvBuffer(uint32_t i)           { m_RecvBuffer = m_RecvBuffer.substr(i); }
  inline void ClearSendBuffer()                           { m_SendBuffer.clear(); }

  void DoRecv(fd_set *fd);
  void DoSend(fd_set *send_fd);
  void Disconnect();
};

//
// CTCPClient
//

class CTCPClient
{
protected:
  struct sockaddr_in m_SIN;

private:
  std::string m_RecvBuffer;
  std::string m_SendBuffer;

protected:
  SOCKET m_Socket;
  int32_t m_Error;
  bool m_HasError;
  bool m_Connected;
  bool m_Connecting;

public:
  CTCPClient();
  ~CTCPClient();

  std::string GetErrorString() const;
  inline BYTEARRAY GetPort() const                        { return CreateByteArray(m_SIN.sin_port, false); }
  inline std::string GetIPString() const                       { return inet_ntoa(m_SIN.sin_addr); }
  inline std::string *GetBytes()                               { return &m_RecvBuffer; }
  inline int32_t GetError() const                             { return m_Error; }
  inline bool HasError() const                            { return m_HasError; }
  inline bool GetConnected() const                        { return m_Connected; }
  inline bool GetConnecting() const                       { return m_Connecting; }

  void SetFD(fd_set *fd, fd_set *send_fd, int32_t *nfds);
  void Reset();
  inline void PutBytes(const std::string &bytes)              { m_SendBuffer += bytes; }
  inline void PutBytes(const BYTEARRAY &bytes)           { m_SendBuffer += std::string(begin(bytes), end(bytes)); }

  bool CheckConnect();
  inline void ClearRecvBuffer()                           { m_RecvBuffer.clear(); }
  inline void SubstrRecvBuffer(uint32_t i)           { m_RecvBuffer = m_RecvBuffer.substr(i); }
  inline void ClearSendBuffer()                           { m_SendBuffer.clear(); }
  void DoRecv(fd_set *fd);
  void DoSend(fd_set *send_fd);
  void Disconnect();
  void Connect(const std::string &localaddress, const std::string &address, uint16_t port);
};

//
// CTCPServer
//

class CTCPServer
{
protected:
  struct sockaddr_in m_SIN;
  SOCKET m_Socket;
  int32_t m_Error;
  bool m_HasError;

public:
  CTCPServer();
  ~CTCPServer();

  std::string GetErrorString() const;
  inline bool HasError() const                            { return m_HasError; }
  inline int32_t GetError() const                             { return m_Error; }

  bool Listen(const std::string &address, uint16_t port);
  void SetFD(fd_set *fd, fd_set *send_fd, int32_t *nfds);
  CTCPSocket *Accept(fd_set *fd);
};

//
// CUDPSocket
//

class CUDPSocket
{
protected:
  struct sockaddr_in m_SIN;
  struct in_addr m_BroadcastTarget;
  SOCKET m_Socket;
  int32_t m_Error;
  bool m_HasError;

public:
  CUDPSocket();
  ~CUDPSocket();

  std::string GetErrorString() const;
  inline BYTEARRAY GetPort() const                        { return CreateByteArray(m_SIN.sin_port, false); }
  inline std::string GetIPString() const                       { return inet_ntoa(m_SIN.sin_addr); }
  inline bool HasError() const                            { return m_HasError; }
  inline int32_t GetError() const                             { return m_Error; }

  bool SendTo(struct sockaddr_in sin, const BYTEARRAY &message);
  bool SendTo(const std::string &address, uint16_t port, const BYTEARRAY &message);
  bool Broadcast(uint16_t port, const BYTEARRAY &message);

  void SetFD(fd_set *fd, fd_set *send_fd, int32_t *nfds);
  void Allocate(int32_t type);
  void Reset();
  void SetBroadcastTarget(const std::string &subnet);
  void SetDontRoute(bool dontRoute);
};

#endif  // AURA_SOCKET_H_
