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

#undef EBADF /* override definition in errno.h */
#define EBADF WSAEBADF
#undef EINTR /* override definition in errno.h */
#define EINTR WSAEINTR
#undef EINVAL /* override definition in errno.h */
#define EINVAL WSAEINVAL
#undef EWOULDBLOCK /* override definition in errno.h */
#define EWOULDBLOCK WSAEWOULDBLOCK
#undef EINPROGRESS /* override definition in errno.h */
#define EINPROGRESS WSAEINPROGRESS
#undef EALREADY /* override definition in errno.h */
#define EALREADY WSAEALREADY
#undef ENOTSOCK /* override definition in errno.h */
#define ENOTSOCK WSAENOTSOCK
#undef EDESTADDRREQ /* override definition in errno.h */
#define EDESTADDRREQ WSAEDESTADDRREQ
#undef EMSGSIZE /* override definition in errno.h */
#define EMSGSIZE WSAEMSGSIZE
#undef EPROTOTYPE /* override definition in errno.h */
#define EPROTOTYPE WSAEPROTOTYPE
#undef ENOPROTOOPT /* override definition in errno.h */
#define ENOPROTOOPT WSAENOPROTOOPT
#undef EPROTONOSUPPORT /* override definition in errno.h */
#define EPROTONOSUPPORT WSAEPROTONOSUPPORT
#define ESOCKTNOSUPPORT WSAESOCKTNOSUPPORT
#undef EOPNOTSUPP /* override definition in errno.h */
#define EOPNOTSUPP WSAEOPNOTSUPP
#define EPFNOSUPPORT WSAEPFNOSUPPORT
#undef EAFNOSUPPORT /* override definition in errno.h */
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#undef EADDRINUSE /* override definition in errno.h */
#define EADDRINUSE WSAEADDRINUSE
#undef EADDRNOTAVAIL /* override definition in errno.h */
#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
#undef ENETDOWN /* override definition in errno.h */
#define ENETDOWN WSAENETDOWN
#undef ENETUNREACH /* override definition in errno.h */
#define ENETUNREACH WSAENETUNREACH
#undef ENETRESET /* override definition in errno.h */
#define ENETRESET WSAENETRESET
#undef ECONNABORTED /* override definition in errno.h */
#define ECONNABORTED WSAECONNABORTED
#undef ECONNRESET /* override definition in errno.h */
#define ECONNRESET WSAECONNRESET
#undef ENOBUFS /* override definition in errno.h */
#define ENOBUFS WSAENOBUFS
#undef EISCONN /* override definition in errno.h */
#define EISCONN WSAEISCONN
#undef ENOTCONN /* override definition in errno.h */
#define ENOTCONN WSAENOTCONN
#define ESHUTDOWN WSAESHUTDOWN
#define ETOOMANYREFS WSAETOOMANYREFS
#undef ETIMEDOUT /* override definition in errno.h */
#define ETIMEDOUT WSAETIMEDOUT
#undef ECONNREFUSED /* override definition in errno.h */
#define ECONNREFUSED WSAECONNREFUSED
#undef ELOOP /* override definition in errno.h */
#define ELOOP WSAELOOP
#ifndef ENAMETOOLONG /* possible previous definition in errno.h */
#define ENAMETOOLONG WSAENAMETOOLONG
#endif
#define EHOSTDOWN WSAEHOSTDOWN
#undef EHOSTUNREACH /* override definition in errno.h */
#define EHOSTUNREACH WSAEHOSTUNREACH
#ifndef ENOTEMPTY /* possible previous definition in errno.h */
#define ENOTEMPTY WSAENOTEMPTY
#endif
#define EPROCLIM WSAEPROCLIM
#define EUSERS WSAEUSERS
#define EDQUOT WSAEDQUOT
#define ESTALE WSAESTALE
#define EREMOTE WSAEREMOTE
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

//extern int32_t GetLastError();
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
  SOCKET             m_Socket;
  struct sockaddr_in m_SIN;
  bool               m_HasError;
  int                m_Error;

  CSocket();
  CSocket(SOCKET nSocket, struct sockaddr_in nSIN);

public:
  ~CSocket();

  std::string                 GetErrorString() const;
  inline std::vector<uint8_t> GetPort() const { return CreateByteArray(m_SIN.sin_port, false); }
  inline std::vector<uint8_t> GetIP() const { return CreateByteArray(static_cast<uint32_t>(m_SIN.sin_addr.s_addr), false); }
  inline std::string          GetIPString() const { return inet_ntoa(m_SIN.sin_addr); }
  inline int32_t              GetError() const { return m_Error; }
  inline bool                 HasError() const { return m_HasError; }

  void SetFD(fd_set* fd, fd_set* send_fd, int32_t* nfds);
  void Reset();
  void Allocate(int type);
};

//
// CTCPSocket
//

class CTCPSocket : public CSocket
{
protected:
  std::string m_RecvBuffer;
  std::string m_SendBuffer;
  uint32_t    m_LastRecv;
  bool        m_Connected;

public:
  CTCPSocket();
  CTCPSocket(SOCKET nSocket, struct sockaddr_in nSIN);
  ~CTCPSocket();

  inline std::string* GetBytes() { return &m_RecvBuffer; }
  inline uint32_t     GetLastRecv() const { return m_LastRecv; }
  inline bool         GetConnected() const { return m_Connected; }

  inline void PutBytes(const std::string& bytes) { m_SendBuffer += bytes; }
  inline void PutBytes(const std::vector<uint8_t>& bytes) { m_SendBuffer += std::string(begin(bytes), end(bytes)); }

  inline void ClearRecvBuffer() { m_RecvBuffer.clear(); }
  inline void SubstrRecvBuffer(uint32_t i) { m_RecvBuffer = m_RecvBuffer.substr(i); }
  inline void                           ClearSendBuffer() { m_SendBuffer.clear(); }

  void DoRecv(fd_set* fd);
  void DoSend(fd_set* send_fd);
  void Disconnect();

  void Reset();
};

//
// CTCPClient
//

class CTCPClient final : public CTCPSocket
{
protected:
  bool m_Connecting;

public:
  CTCPClient();
  ~CTCPClient();

  inline std::string* GetBytes() { return &m_RecvBuffer; }
  inline bool         GetConnected() const { return m_Connected; }
  inline bool         GetConnecting() const { return m_Connecting; }

  void        Reset();
  inline void PutBytes(const std::string& bytes) { m_SendBuffer += bytes; }
  inline void PutBytes(const std::vector<uint8_t>& bytes) { m_SendBuffer += std::string(begin(bytes), end(bytes)); }

  bool        CheckConnect();
  inline void ClearRecvBuffer() { m_RecvBuffer.clear(); }
  inline void SubstrRecvBuffer(uint32_t i) { m_RecvBuffer = m_RecvBuffer.substr(i); }
  inline void                           ClearSendBuffer() { m_SendBuffer.clear(); }
  void DoRecv(fd_set* fd);
  void DoSend(fd_set* send_fd);
  void Disconnect();
  void Connect(const std::string& localaddress, const std::string& address, uint16_t port);
};

//
// CTCPServer
//

class CTCPServer final : public CTCPSocket
{
public:
  CTCPServer();
  ~CTCPServer();

  bool Listen(const std::string& address, uint16_t port);
  CTCPSocket* Accept(fd_set* fd);
};

//
// CUDPSocket
//

class CUDPSocket final : public CSocket
{
protected:
  struct in_addr m_BroadcastTarget;

public:
  CUDPSocket();
  ~CUDPSocket();

  bool SendTo(struct sockaddr_in sin, const std::vector<uint8_t>& message);
  bool SendTo(const std::string& address, uint16_t port, const std::vector<uint8_t>& message);
  bool Broadcast(uint16_t port, const std::vector<uint8_t>& message);

  void Reset();
  void SetBroadcastTarget(const std::string& subnet);
  void SetDontRoute(bool dontRoute);
};

#endif // AURA_SOCKET_H_
