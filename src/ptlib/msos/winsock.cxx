/*
 * winsock.cxx
 *
 * WINSOCK implementation of Berkley sockets.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <ptlib/sockets.h>

#include <svcguid.h>


#ifndef _WIN32_WCE
  #include <nspapi.h>
  #include <wsipx.h>

  #ifdef _MSC_VER
    #include <wsnwlink.h>

    #pragma comment(lib, "ws2_32.lib")

    #if P_HAS_IPV6
      #pragma message("IPv6 support enabled")
    #else
      #pragma message("IPv6 support DISABLED")
    #endif
    #if P_QOS
      #pragma message("QoS support enabled")
    #else
      #pragma message("QoS support DISABLED")
    #endif

  #else

    #define IPX_PTYPE 0x4000
    #define NS_DEFAULT 0

    #ifndef SVCID_NETWARE
    #define SVCID_NETWARE(_SapId) {(0x000B << 16)|(_SapId),0,0,{0xC0,0,0,0,0,0,0,0x46}}
    #endif /* SVCID_NETWARE */

    #define SVCID_FILE_SERVER SVCID_NETWARE(0x4)

  #endif

#endif // !_WIN32_WCE


//////////////////////////////////////////////////////////////////////////////
// PWinSock

PWinSock::PWinSock()
{
  WSADATA winsock;

#if 0 // old WinSock version check
  PAssert(WSAStartup(0x101, &winsock) == 0, POperatingSystemError);
  PAssert(LOBYTE(winsock.wVersion) == 1 &&
          HIBYTE(winsock.wVersion) == 1, POperatingSystemError);

#endif

  // ensure we support QoS
  PAssert(WSAStartup(0x0202, &winsock) == 0, POperatingSystemError);
  PAssert(LOBYTE(winsock.wVersion) >= 1 &&
          HIBYTE(winsock.wVersion) >= 1, POperatingSystemError);
}


PWinSock::~PWinSock()
{
  WSACleanup();
}


PBoolean PWinSock::OpenSocket()
{
  return PFalse;
}


const char * PWinSock::GetProtocolName() const
{
  return NULL;
}


//////////////////////////////////////////////////////////////////////////////
// P_fd_set

void P_fd_set::Construct()
{
  max_fd = UINT_MAX;
  set = (fd_set *)malloc(sizeof(fd_set));
}


void P_fd_set::Zero()
{
  if (PAssertNULL(set) != NULL)
    FD_ZERO(set);
}


//////////////////////////////////////////////////////////////////////////////
// PSocket

PSocket::~PSocket()
{
  Close();
}


PBoolean PSocket::Read(void * buf, PINDEX len)
{
  flush();
  lastReadCount = 0;

  if (len == 0)
    return SetErrorValues(BadParameter, EINVAL, LastReadError);

  os_recvfrom((char *)buf, len, 0, NULL, NULL);
  return lastReadCount > 0;
}


PBoolean PSocket::Write(const void * buf, PINDEX len)
{
  flush();
  return os_sendto(buf, len, 0, NULL, 0) && lastWriteCount >= len;
}


PBoolean PSocket::Close()
{
  if (!IsOpen())
    return PFalse;
  flush();
  return ConvertOSError(os_close());
}


int PSocket::os_close()
{
  clear();
  SOCKET s = os_handle;
  os_handle = -1;
  return closesocket(s);
}


int PSocket::os_socket(int af, int type, int proto)
{
  return ::socket(af, type, proto);
}


PBoolean PSocket::os_connect(struct sockaddr * addr, PINDEX size)
{
  if (readTimeout == PMaxTimeInterval)
    return ConvertOSError(::connect(os_handle, addr, size));

  DWORD fionbio = 1;
  if (!ConvertOSError(::ioctlsocket(os_handle, FIONBIO, &fionbio)))
    return PFalse;
  fionbio = 0;

  if (::connect(os_handle, addr, size) != SOCKET_ERROR)
    return ConvertOSError(::ioctlsocket(os_handle, FIONBIO, &fionbio));

  DWORD err = GetLastError();
  if (err != WSAEWOULDBLOCK) {
    ::ioctlsocket(os_handle, FIONBIO, &fionbio);
    SetLastError(err);
    return ConvertOSError(-1);
  }

  P_fd_set writefds = os_handle;
  P_fd_set exceptfds = os_handle;
  P_timeval tv;

  /* To avoid some strange behaviour on various windows platforms, do a zero
     timeout select first to pick up errors. Then do real timeout. */
  int selerr = ::select(1, NULL, writefds, exceptfds, tv);
  if (selerr == 0) {
    writefds = os_handle;
    exceptfds = os_handle;
    tv = readTimeout;
    selerr = ::select(1, NULL, writefds, exceptfds, tv);
  }

  switch (selerr) {
    case 1 :
      if (writefds.IsPresent(os_handle)) {
        // The following is to avoid a bug in Win32 sockets. The getpeername() function doesn't
        // work for some period of time after a connect, saying it is not connected yet!
        for (PINDEX failsafe = 0; failsafe < 1000; failsafe++) {
          sockaddr_in address;
          int sz = sizeof(address);
          if (::getpeername(os_handle, (struct sockaddr *)&address, &sz) == 0) {
            if (address.sin_port != 0)
              break;
          }
          ::Sleep(0);
        }

        err = 0;
      }
      else {
        // The following is to avoid a bug in Win32 sockets. The getsockopt() function
        // doesn't work for some period of time after a connect, saying no error!
        for (PINDEX failsafe = 0; failsafe < 1000; failsafe++) {
          int sz = sizeof(err);
          if (::getsockopt(os_handle, SOL_SOCKET, SO_ERROR, (char *)&err, &sz) == 0) {
            if (err != 0)
              break;
          }
          ::Sleep(0);
        }
        if (err == 0)
          err = WSAEFAULT; // Need to have something!
      }
      break;

    case 0 :
      err = WSAETIMEDOUT;
      break;

    default :
      err = GetLastError();
  }

  if (::ioctlsocket(os_handle, FIONBIO, &fionbio) == SOCKET_ERROR) {
    if (err == 0)
      err = GetLastError();
  }

  SetLastError(err);
  return ConvertOSError(err == 0 ? 0 : SOCKET_ERROR);
}


PBoolean PSocket::os_accept(PSocket & listener, struct sockaddr * addr, int * size)
{
  if (listener.GetReadTimeout() != PMaxTimeInterval) {
    P_fd_set readfds = listener.GetHandle();
    P_timeval tv = listener.GetReadTimeout();
    switch (select(0, readfds, NULL, NULL, tv)) {
      case 1 :
        break;
      case 0 :
        SetLastError(WSAETIMEDOUT);
        // Then return -1
      default :
        return ConvertOSError(-1);
    }
  }
  return ConvertOSError(os_handle = ::accept(listener.GetHandle(), addr, size));
}


PBoolean PSocket::os_recvfrom(void * buf,
                          PINDEX len,
                          int flags,
                          struct sockaddr * from,
                          PINDEX * fromlen)
{
  lastReadCount = 0;

  if (readTimeout != PMaxTimeInterval) {
    DWORD available;
    if (!ConvertOSError(ioctlsocket(os_handle, FIONREAD, &available), LastReadError))
      return PFalse;

    if (available == 0) {
      P_fd_set readfds = os_handle;
      P_timeval tv = readTimeout;
      int selval = ::select(0, readfds, NULL, NULL, tv);
      if (!ConvertOSError(selval, LastReadError))
        return PFalse;

      if (selval == 0)
        return SetErrorValues(Timeout, EAGAIN, LastReadError);

      if (!ConvertOSError(ioctlsocket(os_handle, FIONREAD, &available), LastReadError))
        return PFalse;
    }

    if (available > 0 && len > (PINDEX)available)
      len = available;
  }

  int recvResult = ::recvfrom(os_handle, (char *)buf, len, flags, from, fromlen);
  if (!ConvertOSError(recvResult, LastReadError))
    return PFalse;

  lastReadCount = recvResult;
  return PTrue;
}


PBoolean PSocket::os_sendto(const void * buf,
                        PINDEX len,
                        int flags,
                        struct sockaddr * to,
                        PINDEX tolen)
{
  lastWriteCount = 0;

  if (writeTimeout != PMaxTimeInterval) {
    P_fd_set writefds = os_handle;
    P_timeval tv = writeTimeout;
    int selval = ::select(0, NULL, writefds, NULL, tv);
    if (selval < 0)
      return PFalse;

    if (selval == 0) {
#ifndef _WIN32_WCE
      errno = EAGAIN;
#else
      SetLastError(EAGAIN);
#endif
      return PFalse;
    }
  }

  int sendResult = ::sendto(os_handle, (const char *)buf, len, flags, to, tolen);
  if (!ConvertOSError(sendResult, LastWriteError))
    return PFalse;

  if (sendResult == 0)
    return PFalse;

  lastWriteCount = sendResult;
  return PTrue;
}


PChannel::Errors PSocket::Select(SelectList & read,
                                 SelectList & write,
                                 SelectList & except,
                                 const PTimeInterval & timeout)
{
  SelectList::iterator sock;
  P_fd_set readfds;
  for (sock = read.begin(); sock != read.end(); ++sock) {
    if (!sock->IsOpen())
      return NotOpen;
    readfds += sock->GetHandle();
  }

  P_fd_set writefds;
  for (sock = write.begin(); sock != write.end(); ++sock) {
    if (!sock->IsOpen())
      return NotOpen;
    writefds += sock->GetHandle();
  }

  P_fd_set exceptfds;
  for (sock = except.begin(); sock != except.end(); ++sock) {
    if (!sock->IsOpen())
      return NotOpen;
    exceptfds += sock->GetHandle();
  }

  P_timeval tval = timeout;
  int retval = select(INT_MAX, readfds, writefds, exceptfds, tval);

  Errors lastError;
  int osError;
  if (!ConvertOSError(retval, lastError, osError))
    return lastError;

  if (retval > 0) {
    sock = read.begin();
    while (sock != read.end()) {
      int h = sock->GetHandle();
      if (h < 0)
        return Interrupted;
      if (readfds.IsPresent(h))
        ++sock;
      else
        read.erase(sock++);
    }
    sock = write.begin();
    while ( sock != write.end()) {
      int h = sock->GetHandle();
      if (h < 0)
        return Interrupted;
      if (writefds.IsPresent(h))
        ++sock;
      else
        write.erase(sock++);
    }
    sock = except.begin();
    while ( sock != except.end()) {
      int h = sock->GetHandle();
      if (h < 0)
        return Interrupted;
      if (exceptfds.IsPresent(h))
        ++sock;
      else
        except.erase(sock++);
    }
  }
  else {
    read.RemoveAll();
    write.RemoveAll();
    except.RemoveAll();
  }

  return NoError;
}


PBoolean PSocket::ConvertOSError(int status, ErrorGroup group)
{
  Errors lastError;
  int osError;
  PBoolean ok = ConvertOSError(status, lastError, osError);
  SetErrorValues(lastError, osError, group);
  return ok;
}


PBoolean PSocket::ConvertOSError(int status, Errors & lastError, int & osError)
{
  if (status >= 0) {
    lastError = NoError;
    osError = 0;
    return PTrue;
  }

#ifdef _WIN32
  SetLastError(WSAGetLastError());
  return PChannel::ConvertOSError(-2, lastError, osError);
#else
  osError = WSAGetLastError();
  switch (osError) {
    case 0 :
      lastError = NoError;
      return PTrue;
    case WSAEWOULDBLOCK :
      lastError = Timeout;
      break;
    default :
      osError |= PWIN32ErrorFlag;
      lastError = Miscellaneous;
  }
  return PFalse;
#endif
}


//////////////////////////////////////////////////////////////////////////////
// PIPSocket::Address

PIPSocket::Address::Address(BYTE b1, BYTE b2, BYTE b3, BYTE b4)
{
  version = 4;
  v.four.S_un.S_un_b.s_b1 = b1;
  v.four.S_un.S_un_b.s_b2 = b2;
  v.four.S_un.S_un_b.s_b3 = b3;
  v.four.S_un.S_un_b.s_b4 = b4;
}


PIPSocket::Address::Address(DWORD dw)
{
  operator=(dw);
}


PIPSocket::Address & PIPSocket::Address::operator=(DWORD dw)
{
  if (dw == 0) {
    version = 0;
    memset(&v, 0, sizeof(v));
  }
  else {
    version = 4;
    v.four.S_un.S_addr = dw;
  }
  return *this;
}


PIPSocket::Address::operator DWORD() const
{
  return version != 4 ? 0 : v.four.S_un.S_addr;
}


BYTE PIPSocket::Address::Byte1() const
{
  return v.four.S_un.S_un_b.s_b1;
}


BYTE PIPSocket::Address::Byte2() const
{
  return v.four.S_un.S_un_b.s_b2;
}


BYTE PIPSocket::Address::Byte3() const
{
  return v.four.S_un.S_un_b.s_b3;
}


BYTE PIPSocket::Address::Byte4() const
{
  return v.four.S_un.S_un_b.s_b4;
}


//////////////////////////////////////////////////////////////////////////////
// PIPSocket

PBoolean P_IsOldWin95()
{
  static int state = -1;
  if (state < 0) {
    state = 1;
    OSVERSIONINFO info;
    info.dwOSVersionInfoSize = sizeof(info);
    if (GetVersionEx(&info)) {
      state = 0;
      if (info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && info.dwBuildNumber < 1000)
        state = 1;
    }
  }
  return state != 0;
}


PBoolean PIPSocket::IsLocalHost(const PString & hostname)
{
  if (hostname.IsEmpty())
    return PTrue;

  if (hostname *= "localhost")
    return PTrue;

  // lookup the host address using inet_addr, assuming it is a "." address
  PIPSocket::Address addr = hostname;
  if (addr.IsLoopback())  // Is 127.0.0.1 or ::1
    return PTrue;

  if (addr == 0) {
    if (!GetHostAddress(hostname, addr))
      return PFalse;
  }

  // Seb: Should check that it's really IPv4 aware.
  struct hostent * host_info = ::gethostbyname(GetHostName());

  if (P_IsOldWin95())
    return addr == *(struct in_addr *)host_info->h_addr_list[0];

  for (PINDEX i = 0; host_info->h_addr_list[i] != NULL; i++) {
#if P_HAS_IPV6
    if (host_info->h_length == 16) {
      if (addr == *(struct in6_addr *)host_info->h_addr_list[i])
        return PTrue;
    }
    else
#endif
    if (addr == *(struct in_addr *)host_info->h_addr_list[i])
      return PTrue;
  }
  return PFalse;
}

//////////////////////////////////////////////////////////////////////////////
// PUDPSocket

PBoolean PUDPSocket::disableGQoS = PTrue;

void PUDPSocket::EnableGQoS()
{
  disableGQoS = PFalse;
}

#if P_QOS
PBoolean PUDPSocket::SupportQoS(const PIPSocket::Address & address)
{
  if (disableGQoS)
    return PFalse;

  if (!address.IsValid())
    return PFalse;

  // Check to See if OS supportive
    OSVERSIONINFO versInfo;
    ZeroMemory(&versInfo,sizeof(OSVERSIONINFO));
    versInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!(GetVersionEx(&versInfo)))
        return PFalse;
    else
    {
        if (versInfo.dwMajorVersion < 5)
            return PFalse;  // Not Supported in Windows

        if (versInfo.dwMajorVersion == 5 &&
            versInfo.dwMinorVersion == 0)
            return PFalse;         //Windows 2000 does not always support QOS_DESTADDR
    }

  // Need to put in a check to see if the NIC has 802.1p packet priority support 
  // This Requires access to the NIC driver and requires Windows DDK. To Be Done Sometime...
  
  // Get the name of the required NIC to check whether it supports 802.1p
  PString NICname =  PIPSocket::GetInterface(address);

  // For Now Assume it can.
  return PTrue;
}

#else

PBoolean PUDPSocket::SupportQoS(const PIPSocket::Address &)
{
  return PFalse;
}
#endif  // P_QOS


#if P_QOS

PWinQoS::~PWinQoS()
{
    delete sa;
}

PWinQoS::PWinQoS(PQoS & pqos, struct sockaddr * to, char * inBuf, DWORD & bufLen)
{
  QOS * qos = (QOS *)inBuf;
    
  if (pqos.GetTokenRate() == QOS_NOT_SPECIFIED)
    qos->SendingFlowspec.ServiceType = SERVICETYPE_BESTEFFORT;
  else
    qos->SendingFlowspec.ServiceType = pqos.GetServiceType();
    
  qos->SendingFlowspec.TokenRate = pqos.GetTokenRate();
  qos->SendingFlowspec.TokenBucketSize = pqos.GetTokenBucketSize();
  qos->SendingFlowspec.PeakBandwidth = pqos.GetPeakBandwidth();
  qos->SendingFlowspec.Latency = QOS_NOT_SPECIFIED;
  qos->SendingFlowspec.DelayVariation = QOS_NOT_SPECIFIED;
  qos->SendingFlowspec.MaxSduSize = QOS_NOT_SPECIFIED;
  qos->SendingFlowspec.MinimumPolicedSize = QOS_NOT_SPECIFIED;

  qos->ReceivingFlowspec.ServiceType = SERVICETYPE_BESTEFFORT|SERVICE_NO_QOS_SIGNALING;
  qos->ReceivingFlowspec.TokenRate = QOS_NOT_SPECIFIED;
  qos->ReceivingFlowspec.TokenBucketSize = QOS_NOT_SPECIFIED;
  qos->ReceivingFlowspec.PeakBandwidth = QOS_NOT_SPECIFIED;
  qos->ReceivingFlowspec.Latency = QOS_NOT_SPECIFIED;
  qos->ReceivingFlowspec.DelayVariation = QOS_NOT_SPECIFIED;
  qos->ReceivingFlowspec.MaxSduSize = QOS_NOT_SPECIFIED;
  qos->ReceivingFlowspec.MinimumPolicedSize = QOS_NOT_SPECIFIED;

  sa = new sockaddr;
  *sa = *to;

  QOS_DESTADDR qosdestaddr;
  qosdestaddr.ObjectHdr.ObjectType = QOS_OBJECT_DESTADDR;
  qosdestaddr.ObjectHdr.ObjectLength = sizeof(qosdestaddr);
  qosdestaddr.SocketAddress = sa;
  qosdestaddr.SocketAddressLength = sizeof(*sa);

  qos->ProviderSpecific.len = sizeof(qosdestaddr);
  qos->ProviderSpecific.buf = inBuf + sizeof(*qos);

  memcpy(inBuf+sizeof(*qos),&qosdestaddr,sizeof(qosdestaddr));
  bufLen = sizeof(*qos)+sizeof(qosdestaddr);
}

#endif // P_QOS


// End Of File ///////////////////////////////////////////////////////////////
