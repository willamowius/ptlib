/*
 * socket.cxx
 *
 * Berkley sockets classes implementation
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

#pragma implementation "sockets.h"
#pragma implementation "socket.h"
#pragma implementation "ipsock.h"
#pragma implementation "udpsock.h"
#pragma implementation "tcpsock.h"
#pragma implementation "ipdsock.h"
#pragma implementation "ethsock.h"
#pragma implementation "qos.h"

#include <ptlib.h>
#include <ptlib/sockets.h>

#include <map>
#include <ptlib/pstring.h>

#if defined(SIOCGENADDR)
#define SIO_Get_MAC_Address SIOCGENADDR
#define  ifr_macaddr         ifr_ifru.ifru_enaddr
#elif defined(SIOCGIFHWADDR)
#define SIO_Get_MAC_Address SIOCGIFHWADDR
#define  ifr_macaddr         ifr_hwaddr.sa_data
#endif

#if defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_NETBSD) || defined(P_SOLARIS) || defined(P_MACOSX) || defined(P_MACOS) || defined(P_IRIX) || defined(P_VXWORKS) || defined(P_RTEMS) || defined(P_QNX)
#define ifr_netmask ifr_addr

#include <net/if_dl.h>

#if !defined(P_IPHONEOS)
#include <net/if_types.h>
#include <net/route.h>
#endif

#include <netinet/in.h>
#if !defined(P_QNX)  && !defined(P_IPHONEOS)
#include <netinet/if_ether.h>
#endif

#if defined(P_NETBSD)
#include <ifaddrs.h>
#endif

#define ROUNDUP(a) \
        ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

#endif

#if defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_NETBSD) || defined(P_MACOSX) || defined(P_MACOS) || defined(P_QNX)
#include <sys/sysctl.h>
#endif

#ifdef P_RTEMS
#include <bsp.h>
#endif

#ifdef P_BEOS
#include <posix/sys/ioctl.h> // for FIONBIO
#include <be/bone/net/if.h> // for ifconf
#include <be/bone/sys/sockio.h> // for SIOCGI*
#endif

#if defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_NETBSD) || defined(P_MACOSX) || defined(P_VXWORKS) || defined(P_RTEMS) || defined(P_QNX)
// Define _SIZEOF_IFREQ for platforms (eg OpenBSD) which do not have it.
#ifndef _SIZEOF_ADDR_IFREQ
#define _SIZEOF_ADDR_IFREQ(ifr) \
  ((ifr).ifr_addr.sa_len > sizeof(struct sockaddr) ? \
  (sizeof(struct ifreq) - sizeof(struct sockaddr) + \
  (ifr).ifr_addr.sa_len) : sizeof(struct ifreq))
#endif
#endif

int PX_NewHandle(const char *, int);

#ifdef P_VXWORKS
// VxWorks variant of inet_ntoa() allocates INET_ADDR_LEN bytes via malloc
// BUT DOES NOT FREE IT !!!  Use inet_ntoa_b() instead.
#define INET_ADDR_LEN      18
extern "C" void inet_ntoa_b(struct in_addr inetAddress, char *pString);
#endif // P_VXWORKS

//////////////////////////////////////////////////////////////////////////////
// P_fd_set

void P_fd_set::Construct()
{
  max_fd = PProcess::Current().GetMaxHandles();
  set = (fd_set *)malloc((max_fd+7)>>3);
}


void P_fd_set::Zero()
{
  if (PAssertNULL(set) != NULL)
    memset(set, 0, (max_fd+7)>>3);
}


//////////////////////////////////////////////////////////////////////////////

PSocket::~PSocket()
{
  os_close();
}

int PSocket::os_close()
{
  if (os_handle < 0)
    return -1;

  // send a shutdown to the other end
  ::shutdown(os_handle, 2);

  return PXClose();
}


static int SetNonBlocking(int fd)
{
  if (fd < 0)
    return -1;

  // Set non-blocking so we can use select calls to break I/O block on close
  int cmd = 1;
#if defined(P_VXWORKS)
  if (::ioctl(fd, FIONBIO, &cmd) == 0)
#else
  if (::ioctl(fd, FIONBIO, &cmd) == 0 && ::fcntl(fd, F_SETFD, 1) == 0)
#endif
    return fd;

  ::close(fd);
  return -1;
}


int PSocket::os_socket(int af, int type, int protocol)
{
  int fd = ::socket(af, type, protocol);

#if P_HAS_RECVMSG
  if ((fd != -1) && (type == SOCK_DGRAM)) {
    int v = 1;
    setsockopt(fd, IPPROTO_IP, IP_RECVERR, &v, sizeof(v));
  }
#endif

  // attempt to create a socket
  return SetNonBlocking(PX_NewHandle(GetClass(), fd));
}


PBoolean PSocket::os_connect(struct sockaddr * addr, PINDEX size)
{
  int val;
  do {
    val = ::connect(os_handle, addr, size);
  } while (val != 0 && errno == EINTR);
  if (val == 0 || errno != EINPROGRESS)
    return ConvertOSError(val);

  if (!PXSetIOBlock(PXConnectBlock, readTimeout))
    return PFalse;

  // A successful select() call does not necessarily mean the socket connected OK.
  int optval = -1;
  socklen_t optlen = sizeof(optval);
  getsockopt(os_handle, SOL_SOCKET, SO_ERROR, (char *)&optval, &optlen);
  if (optval != 0) {
    errno = optval;
    return ConvertOSError(-1);
  }

  return PTrue;
}


PBoolean PSocket::os_accept(PSocket & listener, struct sockaddr * addr, PINDEX * size)
{
  int new_fd;
  while ((new_fd = ::accept(listener.GetHandle(), addr, (socklen_t *)size)) < 0) {
    switch (errno) {
      case EINTR :
        break;

#if defined(E_PROTO)
      case EPROTO :
        PTRACE(3, "PTLib\tAccept on " << listener << " failed with EPROTO - retrying");
        break;
#endif

      case EWOULDBLOCK :
        if (listener.GetReadTimeout() > 0) {
          if (listener.PXSetIOBlock(PXAcceptBlock, listener.GetReadTimeout()))
            break;
          return SetErrorValues(listener.GetErrorCode(), listener.GetErrorNumber());
        }
        // Next case

      default :
        return ConvertOSError(-1, LastReadError);
    }
  }

  return ConvertOSError(os_handle = SetNonBlocking(new_fd));
}


#if !defined(P_PTHREADS) && !defined(P_MAC_MPTHREADS) && !defined(P_BEOS)

PChannel::Errors PSocket::Select(SelectList & read,
                                 SelectList & write,
                                 SelectList & except,
      const PTimeInterval & timeout)
{
  PINDEX i, j;
  PINDEX nextfd = 0;
  int maxfds = 0;
  Errors lastError = NoError;
  PThread * unblockThread = PThread::Current();
  
  P_fd_set fds[3];
  SelectList * list[3] = { &read, &write, &except };

  for (i = 0; i < 3; i++) {
    for (j = 0; j < list[i]->GetSize(); j++) {
      PSocket & socket = (*list[i])[j];
      if (!socket.IsOpen())
        lastError = NotOpen;
      else {
        int h = socket.GetHandle();
        fds[i] += h;
        if (h > maxfds)
          maxfds = h;
      }
      socket.px_selectMutex[i].Wait();
      socket.px_selectThread[i] = unblockThread;
    }
  }

  if (lastError == NoError) {
    P_timeval tval = timeout;
    int result = ::select(maxfds+1, 
                          (fd_set *)fds[0], 
                          (fd_set *)fds[1], 
                          (fd_set *)fds[2], 
                          tval);

    int osError;
    (void)ConvertOSError(result, lastError, osError);
  }

  for (i = 0; i < 3; i++) {
    for (j = 0; j < list[i]->GetSize(); j++) {
      PSocket & socket = (*list[i])[j];
      socket.px_selectThread[i] = NULL;
      socket.px_selectMutex[i].Signal();
      if (lastError == NoError) {
        int h = socket.GetHandle();
        if (h < 0)
          lastError = Interrupted;
        else if (!fds[i].IsPresent(h))
          list[i]->RemoveAt(j--);
      }
    }
  }

  return lastError;
}
                     
#else

PChannel::Errors PSocket::Select(SelectList & read,
                                 SelectList & write,
                                 SelectList & except,
                                 const PTimeInterval & timeout)
{
  PINDEX i, j;
  int maxfds = 0;
  Errors lastError = NoError;
  PThread * unblockThread = PThread::Current();
  int unblockPipe = unblockThread->unblockPipe[0];

  P_fd_set fds[3];
  SelectList * list[3] = { &read, &write, &except };

  for (i = 0; i < 3; i++) {
    for (j = 0; j < list[i]->GetSize(); j++) {
      PSocket & socket = (*list[i])[j];
      if (!socket.IsOpen())
        lastError = NotOpen;
      else {
        int h = socket.GetHandle();
        fds[i] += h;
        if (h > maxfds)
          maxfds = h;
      }
      socket.px_selectMutex[i].Wait();
      socket.px_selectThread[i] = unblockThread;
    }
  }

  int result = -1;
  if (lastError == NoError) {
    fds[0] += unblockPipe;
    if (unblockPipe > maxfds)
      maxfds = unblockPipe;

    P_timeval tval = timeout;
    do {
      result = ::select(maxfds+1, (fd_set *)fds[0], (fd_set *)fds[1], (fd_set *)fds[2], tval);
    } while (result < 0 && errno == EINTR);

    int osError;
    if (ConvertOSError(result, lastError, osError)) {
      if (fds[0].IsPresent(unblockPipe)) {
        PTRACE(6, "PWLib\tSelect unblocked fd=" << unblockPipe);
        BYTE ch;
        if (ConvertOSError(::read(unblockPipe, &ch, 1), lastError, osError))
          lastError = Interrupted;
      }
    }
  }

  for (i = 0; i < 3; i++) {
    for (j = 0; j < list[i]->GetSize(); j++) {
      PSocket & socket = (*list[i])[j];
      socket.px_selectThread[i] = NULL;
      socket.px_selectMutex[i].Signal();
      if (lastError == NoError) {
        int h = socket.GetHandle();
        if (h < 0)
          lastError = Interrupted;
        else if (!fds[i].IsPresent(h))
          list[i]->RemoveAt(j--);
      }
    }
  }

  return lastError;
}

#endif


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
    v.four.s_addr = dw;
  }

  return *this;
}


PIPSocket::Address::operator DWORD() const
{
  return version != 4 ? 0 : (DWORD)v.four.s_addr;
}

BYTE PIPSocket::Address::Byte1() const
{
  return *(((BYTE *)&v.four.s_addr)+0);
}

BYTE PIPSocket::Address::Byte2() const
{
  return *(((BYTE *)&v.four.s_addr)+1);
}

BYTE PIPSocket::Address::Byte3() const
{
  return *(((BYTE *)&v.four.s_addr)+2);
}

BYTE PIPSocket::Address::Byte4() const
{
  return *(((BYTE *)&v.four.s_addr)+3);
}

PIPSocket::Address::Address(BYTE b1, BYTE b2, BYTE b3, BYTE b4)
{
  version = 4;
  BYTE * p = (BYTE *)&v.four.s_addr;
  p[0] = b1;
  p[1] = b2;
  p[2] = b3;
  p[3] = b4;
}

PBoolean PIPSocket::IsLocalHost(const PString & hostname)
{
  if (hostname.IsEmpty())
    return PTrue;

  if (hostname *= "localhost")
    return PTrue;

  // lookup the host address using inet_addr, assuming it is a "." address
  Address addr = hostname;
  if (addr.IsLoopback())  // Is 127.0.0.1
    return PTrue;
  if (!addr.IsValid())
    return PFalse;

  if (!GetHostAddress(hostname, addr))
    return PFalse;

#if P_HAS_IPV6
  {
    FILE * file;
    int dummy;
    int addr6[16];
    char ifaceName[255];
    PBoolean found = PFalse;
    if ((file = fopen("/proc/net/if_inet6", "r")) != NULL) {
      while (!found && (fscanf(file, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x %x %x %x %x %255s\n",
              &addr6[0],  &addr6[1],  &addr6[2],  &addr6[3], 
              &addr6[4],  &addr6[5],  &addr6[6],  &addr6[7], 
              &addr6[8],  &addr6[9],  &addr6[10], &addr6[11], 
              &addr6[12], &addr6[13], &addr6[14], &addr6[15], 
             &dummy, &dummy, &dummy, &dummy, ifaceName) != EOF)) {
        Address ip6addr(
          psprintf("%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
              addr6[0],  addr6[1],  addr6[2],  addr6[3], 
              addr6[4],  addr6[5],  addr6[6],  addr6[7], 
              addr6[8],  addr6[9],  addr6[10], addr6[11], 
              addr6[12], addr6[13], addr6[14], addr6[15]
          )
        );
        found = (ip6addr *= addr);
      }
      fclose(file);
    }
    if (found)
      return PTrue;
  }
#endif

  // check IPV4 addresses
  PUDPSocket sock;
  
  PBYTEArray buffer;
  struct ifconf ifConf;

#if defined(P_NETBSD)
  struct ifaddrs *ifap, *ifa;

  PAssert(getifaddrs(&ifap) == 0, "getifaddrs failed");
  for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
#else
#ifdef SIOCGIFNUM
  int ifNum;
  PAssert(::ioctl(sock.GetHandle(), SIOCGIFNUM, &ifNum) >= 0, "could not do ioctl for ifNum");
  ifConf.ifc_len = ifNum * sizeof(ifreq);
#else
  ifConf.ifc_len = 100 * sizeof(ifreq); // That's a LOT of interfaces!
#endif

  ifConf.ifc_req = (struct ifreq *)buffer.GetPointer(ifConf.ifc_len);
  
  if (ioctl(sock.GetHandle(), SIOCGIFCONF, &ifConf) >= 0) {
    void * ifEndList = (char *)ifConf.ifc_req + ifConf.ifc_len;
    ifreq * ifName = ifConf.ifc_req;

    while (ifName < ifEndList) {
#endif
      struct ifreq ifReq;
#if !defined(P_NETBSD)
      memcpy(&ifReq, ifName, sizeof(ifreq));
#else
      memset(&ifReq, 0, sizeof(ifReq));
      strncpy(ifReq.ifr_name, ifa->ifa_name, sizeof(ifReq.ifr_name) - 1);
#endif
      
      if (ioctl(sock.GetHandle(), SIOCGIFFLAGS, &ifReq) >= 0) {
        int flags = ifReq.ifr_flags;
        if ((flags & IFF_UP) && ioctl(sock.GetHandle(), SIOCGIFADDR, &ifReq) >= 0) {
          sockaddr_in * sin = (sockaddr_in *)&ifReq.ifr_addr;
          PIPSocket::Address address = sin->sin_addr;
          if (addr *= address)
            return PTrue;
        }
      }
      
#if defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_MACOSX) || defined(P_VXWORKS) || defined(P_RTEMS) || defined(P_QNX)
      // move the ifName pointer along to the next ifreq entry
      ifName = (struct ifreq *)((char *)ifName + _SIZEOF_ADDR_IFREQ(*ifName));
#elif !defined(P_NETBSD)
      ifName++;
#endif
    }
#if !defined(P_NETBSD)
  }
#endif
  
  return PFalse;
}


////////////////////////////////////////////////////////////////
//
//  PTCPSocket
//
PBoolean PTCPSocket::Read(void * buf, PINDEX maxLen)

{
  lastReadCount = 0;

  // wait until select indicates there is data to read, or until
  // a timeout occurs
  if (!PXSetIOBlock(PXReadBlock, readTimeout))
    return PFalse;

  // attempt to read out of band data
  char buffer[32];
  int ooblen;
  while ((ooblen = ::recv(os_handle, buffer, sizeof(buffer), MSG_OOB)) > 0) 
    OnOutOfBand(buffer, ooblen);

  // attempt to read non-out of band data
  int r = ::recv(os_handle, (char *)buf, maxLen, 0);
  if (!ConvertOSError(r, LastReadError))
    return PFalse;

  lastReadCount = r;
  return lastReadCount > 0;
}


#if P_HAS_RECVMSG

PBoolean PSocket::os_recvfrom(
      void * buf,
      PINDEX len,
      int    flags,
      sockaddr * addr,
      PINDEX * addrlen)
{
  lastReadCount = 0;

  if (!PXSetIOBlock(PXReadBlock, readTimeout))
    return PFalse;

  msghdr readData;
  memset(&readData, 0, sizeof(readData));

  readData.msg_name       = addr;
  readData.msg_namelen    = *addrlen;

  iovec readVector;
  readVector.iov_base     = buf;
  readVector.iov_len      = len;
  readData.msg_iov        = &readVector;
  readData.msg_iovlen     = 1;

  char auxdata[50];
  readData.msg_control    = auxdata;
  readData.msg_controllen = sizeof(auxdata);

  // read a packet 
  int r = ::recvmsg(os_handle, &readData, flags);
  if (r == -1) {
    PTRACE(5, "PTLIB\trecvmsg returned error " << errno);
    ::recvmsg(os_handle, &readData, MSG_ERRQUEUE);
  }

  if (!ConvertOSError(r, LastReadError))
    return PFalse;

  lastReadCount = r;

  if (r >= 0) {
    struct cmsghdr * cmsg;
    for (cmsg = CMSG_FIRSTHDR(&readData); cmsg != NULL; cmsg = CMSG_NXTHDR(&readData,cmsg)) {
      if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_PKTINFO) {
        in_pktinfo * info = (in_pktinfo *)CMSG_DATA(cmsg);
        SetLastReceiveAddr(&info->ipi_spec_dst, sizeof(in_addr));
      }
    }
  }

  return lastReadCount > 0;
}

#else

PBoolean PSocket::os_recvfrom(
      void * buf,     // Data to be written as URGENT TCP data.
      PINDEX len,     // Number of bytes pointed to by <CODE>buf</CODE>.
      int    flags,
      sockaddr * addr, // Address from which the datagram was received.
      PINDEX * addrlen)
{
  lastReadCount = 0;

  if (!PXSetIOBlock(PXReadBlock, readTimeout))
    return PFalse;

  // attempt to read non-out of band data
  int r = ::recvfrom(os_handle, (char *)buf, len, flags, (sockaddr *)addr, (socklen_t *)addrlen);
  if (!ConvertOSError(r, LastReadError))
    return PFalse;

  lastReadCount = r;
  return lastReadCount > 0;
}

#endif


PBoolean PSocket::os_sendto(
      const void * buf,   // Data to be written as URGENT TCP data.
      PINDEX len,         // Number of bytes pointed to by <CODE>buf</CODE>.
      int flags,
      sockaddr * addr, // Address to which the datagram is sent.
      PINDEX addrlen)  
{
  lastWriteCount = 0;

  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF, LastWriteError);

  // attempt to read data
  int result;
  for (;;) {
    if (addr != NULL)
      result = ::sendto(os_handle, (char *)buf, len, flags, (sockaddr *)addr, addrlen);
    else
      result = ::send(os_handle, (char *)buf, len, flags);

    if (result > 0)
      break;

    if (errno != EWOULDBLOCK)
      return ConvertOSError(-1, LastWriteError);

    if (!PXSetIOBlock(PXWriteBlock, writeTimeout))
      return PFalse;
  }

#if !defined(P_PTHREADS) && !defined(P_MAC_MPTHREADS)
  PThread::Yield(); // Starvation prevention
#endif

  lastWriteCount = result;
  return ConvertOSError(0, LastWriteError);
}


PBoolean PSocket::Read(void * buf, PINDEX len)
{
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF, LastReadError);

  if (!PXSetIOBlock(PXReadBlock, readTimeout)) 
    return PFalse;

  int lastReadCount = ::recv(os_handle, (char *)buf, len, 0);
    return lastReadCount > 0;
  if (ConvertOSError(lastReadCount))
    return lastReadCount > 0;

  lastReadCount = 0;
  return PFalse;
}



//////////////////////////////////////////////////////////////////
//
//  PEthSocket
//

PEthSocket::PEthSocket(PINDEX, PINDEX, PINDEX)
{
  medium = MediumUnknown;
  filterMask = FilterDirected|FilterBroadcast;
  filterType = TypeAll;
  fakeMacHeader = PFalse;
  ipppInterface = PFalse;
}


PEthSocket::~PEthSocket()
{
  Close();
}


PBoolean PEthSocket::Connect(const PString & interfaceName)
{
  Close();

  fakeMacHeader = PFalse;
  ipppInterface = PFalse;

  if (strncmp("lo", interfaceName, 2) == 0)
    medium = MediumLoop;
  else if (strncmp("sl", interfaceName, 2) == 0 ||
           strncmp("wlan", interfaceName, 4) == 0 ||
           strncmp("ppp", interfaceName, 3) == 0) {
    medium = MediumWan;
    fakeMacHeader = PTrue;
  }
  else if (strncmp("ippp", interfaceName, 4) == 0) {
    medium = MediumWan;
    ipppInterface = PTrue;
  }
  else
    medium = Medium802_3;

#if defined(SIO_Get_MAC_Address) 
  PUDPSocket ifsock;
  struct ifreq ifr;
  ifr.ifr_addr.sa_family = AF_INET;
  strcpy(ifr.ifr_name, interfaceName);
  if (!ConvertOSError(ioctl(ifsock.GetHandle(), SIO_Get_MAC_Address, &ifr)))
    return PFalse;

  memcpy(&macAddress, ifr.ifr_macaddr, sizeof(macAddress));
#endif

  channelName = interfaceName;
  return OpenSocket();
}


PBoolean PEthSocket::OpenSocket()
{
#ifdef SOCK_PACKET
  if (!ConvertOSError(os_handle = os_socket(AF_INET, SOCK_PACKET, htons(filterType))))
    return PFalse;

  struct sockaddr addr;
  memset(&addr, 0, sizeof(addr));
  addr.sa_family = AF_INET;
  strcpy(addr.sa_data, channelName);
  if (!ConvertOSError(bind(os_handle, &addr, sizeof(addr)))) {
    os_close();
    os_handle = -1;
    return PFalse;
  }
#endif

  return PTrue;
}


PBoolean PEthSocket::Close()
{
  SetFilter(FilterDirected, filterType);  // Turn off promiscuous mode
  return PSocket::Close();
}


PBoolean PEthSocket::EnumInterfaces(PINDEX idx, PString & name)
{
  PUDPSocket ifsock;

  ifreq ifreqs[20]; // Maximum of 20 interfaces
  struct ifconf ifc;
  ifc.ifc_len = sizeof(ifreqs);
  ifc.ifc_buf = (caddr_t)ifreqs;
  if (!ConvertOSError(ioctl(ifsock.GetHandle(), SIOCGIFCONF, &ifc)))
    return PFalse;

  int ifcount = ifc.ifc_len/sizeof(ifreq);
  int ifidx;
  for (ifidx = 0; ifidx < ifcount; ifidx++) {
    if (strchr(ifreqs[ifidx].ifr_name, ':') == NULL) {
      ifreq ifr;
      strcpy(ifr.ifr_name, ifreqs[ifidx].ifr_name);
      if (ioctl(ifsock.GetHandle(), SIOCGIFFLAGS, &ifr) == 0 &&
          (ifr.ifr_flags & IFF_UP) != 0 &&
           idx-- == 0) {
        name = ifreqs[ifidx].ifr_name;
        return PTrue;
      }
    }
  }

  return PFalse;
}


PBoolean PEthSocket::GetAddress(Address & addr)
{
  if (!IsOpen())
    return PFalse;

  addr = macAddress;
  return PTrue;
}


PBoolean PEthSocket::EnumIpAddress(PINDEX idx,
                               PIPSocket::Address & addr,
                               PIPSocket::Address & net_mask)
{
  if (!IsOpen())
    return PFalse;

  PUDPSocket ifsock;
  struct ifreq ifr;
  ifr.ifr_addr.sa_family = AF_INET;
  if (idx == 0)
    strcpy(ifr.ifr_name, channelName);
  else
    sprintf(ifr.ifr_name, "%s:%u", (const char *)channelName, (int)(idx-1));
  if (!ConvertOSError(ioctl(os_handle, SIOCGIFADDR, &ifr)))
    return PFalse;

  sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
  addr = sin->sin_addr;

  if (!ConvertOSError(ioctl(os_handle, SIOCGIFNETMASK, &ifr)))
    return PFalse;

  net_mask = sin->sin_addr;
  return PTrue;
}


PBoolean PEthSocket::GetFilter(unsigned & mask, WORD & type)
{
  if (!IsOpen())
    return PFalse;

  ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, channelName);
  if (!ConvertOSError(ioctl(os_handle, SIOCGIFFLAGS, &ifr)))
    return PFalse;

  if ((ifr.ifr_flags&IFF_PROMISC) != 0)
    filterMask |= FilterPromiscuous;
  else
    filterMask &= ~FilterPromiscuous;

  mask = filterMask;
  type = filterType;
  return PTrue;
}


PBoolean PEthSocket::SetFilter(unsigned filter, WORD type)
{
  if (!IsOpen())
    return PFalse;

  if (filterType != type) {
    os_close();
    filterType = type;
    if (!OpenSocket())
      return PFalse;
  }

  ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, channelName);
  if (!ConvertOSError(ioctl(os_handle, SIOCGIFFLAGS, &ifr)))
    return PFalse;

  if ((filter&FilterPromiscuous) != 0)
    ifr.ifr_flags |= IFF_PROMISC;
  else
    ifr.ifr_flags &= ~IFF_PROMISC;

  if (!ConvertOSError(ioctl(os_handle, SIOCSIFFLAGS, &ifr)))
    return PFalse;

  filterMask = filter;

  return PTrue;
}


PEthSocket::MediumTypes PEthSocket::GetMedium()
{
  return medium;
}


PBoolean PEthSocket::ResetAdaptor()
{
  // No implementation
  return PTrue;
}


PBoolean PEthSocket::Read(void * buf, PINDEX len)
{
  static const BYTE macHeader[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 8, 0 };

  BYTE * bufptr = (BYTE *)buf;

  if (fakeMacHeader) {
    if (len <= (PINDEX)sizeof(macHeader)) {
      memcpy(bufptr, macHeader, len);
      lastReadCount = len;
      return PTrue;
    }

    memcpy(bufptr, macHeader, sizeof(macHeader));
    bufptr += sizeof(macHeader);
    len -= sizeof(macHeader);
  }

  for (;;) {
    sockaddr from;
    PINDEX fromlen = sizeof(from);
    if (!os_recvfrom(bufptr, len, 0, &from, &fromlen))
      return PFalse;

    if (channelName != from.sa_data)
      continue;

    if (ipppInterface) {
      if (lastReadCount <= 10)
        return PFalse;
      if (memcmp(bufptr+6, "\xff\x03\x00\x21", 4) != 0) {
        memmove(bufptr+sizeof(macHeader), bufptr, lastReadCount);
        lastReadCount += sizeof(macHeader);
      }
      else {
        memmove(bufptr+sizeof(macHeader), bufptr+10, lastReadCount);
        lastReadCount += sizeof(macHeader)-10;
      }
      memcpy(bufptr, macHeader, sizeof(macHeader));
      break;
    }

    if (fakeMacHeader) {
      lastReadCount += sizeof(macHeader);
      break;
    }

    if ((filterMask&FilterPromiscuous) != 0)
      break;

    if ((filterMask&FilterDirected) != 0 && macAddress == bufptr)
      break;

    static const Address broadcast;
    if ((filterMask&FilterBroadcast) != 0 && broadcast == bufptr)
      break;
  }

  return lastReadCount > 0;
}


PBoolean PEthSocket::Write(const void * buf, PINDEX len)
{
  sockaddr to;
  strcpy((char *)to.sa_data, channelName);
  return os_sendto(buf, len, 0, &to, sizeof(to)) && lastWriteCount >= len;
}


///////////////////////////////////////////////////////////////////////////////

PBoolean PIPSocket::GetGatewayAddress(Address & addr, int version)
{
  RouteTable table;
  if (GetRouteTable(table)) {
    for (PINDEX i = 0; i < table.GetSize(); i++) {
      if (table[i].GetNetwork() == 0) {
        addr = table[i].GetDestination();
        return PTrue;
      }
    }
  }
  return PFalse;
}



PString PIPSocket::GetGatewayInterface(int version)
{
  RouteTable table;
  if (GetRouteTable(table)) {
    for (PINDEX i = 0; i < table.GetSize(); i++) {
      if (table[i].GetNetwork() == 0)
        return table[i].GetInterface();
    }
  }
  return PString();
}

#if defined(P_LINUX) || defined (P_AIX)

PBoolean PIPSocket::GetRouteTable(RouteTable & table)
{
  table.RemoveAll();

  PString strLine;
  PTextFile procfile;

  if (procfile.Open("/proc/net/route", PFile::ReadOnly) && procfile.ReadLine(strLine)) {
    // Ignore heading line above
    while (procfile.ReadLine(strLine)) {
      char iface[20];
      uint32_t net_addr, dest_addr, net_mask;
      int flags, refcnt, use, metric;
      if (sscanf(strLine, "%s%x%x%x%u%u%u%x",
                 iface, &net_addr, &dest_addr, &flags, &refcnt, &use, &metric, &net_mask) == 8) {
        RouteEntry * entry = new RouteEntry(net_addr);
        entry->net_mask = net_mask;
        entry->destination = dest_addr;
        entry->interfaceName = iface;
        entry->metric = metric;
        table.Append(entry);
      }
    }
  }

#if P_HAS_IPV6
  if (procfile.Open("/proc/net/ipv6_route", PFile::ReadOnly)) {
    while (procfile.ReadLine(strLine)) {
      PStringArray tokens = strLine.Tokenise(" \t", false);
      if (tokens.GetSize() == 10) {
        // 0 = dest_addr
        // 1 = net_mask (cidr in hex)
        // 2 = src_addr
        // 4 = next_hop
        // 5 = metric
        // 6 = refcnt
        // 7 = use
        // 8 = flags
        // 9 = device name

        BYTE net_addr[16];
        for (size_t i = 0; i < sizeof(net_addr); ++i)
          net_addr[i] = tokens[0].Mid(i*2, 2).AsUnsigned(16);

        BYTE dest_addr[16];
        for (size_t i = 0; i < sizeof(dest_addr); ++i)
          dest_addr[i] = tokens[4].Mid(i*2, 2).AsUnsigned(16);

        RouteEntry * entry = new RouteEntry(Address(sizeof(net_addr), net_addr));
        entry->destination = Address(sizeof(dest_addr), dest_addr);
        entry->interfaceName = tokens[9];
        entry->metric = tokens[5].AsUnsigned(16);
		BYTE net_mask[16];
		bzero(net_mask, sizeof(net_mask));
		for(size_t i = 0; i < tokens[1].AsUnsigned(16) / 4; ++i)
			net_mask[i/2] = (i % 2 == 0) ? 0xf0 : 0xff;
        entry->net_mask = Address(sizeof(net_mask), net_mask);
        table.Append(entry);
      }
    }
  }
#endif

  return !table.IsEmpty();
}

#elif (defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_NETBSD) || defined(P_MACOSX) || defined(P_QNX)) && !defined(P_IPHONEOS)

PBoolean process_rtentry(struct rt_msghdr *rtm, char *ptr, unsigned long *p_net_addr,
                     unsigned long *p_net_mask, unsigned long *p_dest_addr, int *p_metric);
PBoolean get_ifname(int index, char *name);

PBoolean PIPSocket::GetRouteTable(RouteTable & table)
{
  int mib[6];
  size_t space_needed;
  char *limit, *buf, *ptr;
  struct rt_msghdr *rtm;

  InterfaceTable if_table;


  // Read the Routing Table
  mib[0] = CTL_NET;
  mib[1] = PF_ROUTE;
  mib[2] = 0;
  mib[3] = 0;
  mib[4] = NET_RT_DUMP;
  mib[5] = 0;

  if (sysctl(mib, 6, NULL, &space_needed, NULL, 0) < 0) {
    printf("sysctl: net.route.0.0.dump estimate");
    return PFalse;
  }

  if ((buf = (char *)malloc(space_needed)) == NULL) {
    printf("malloc(%lu)", (unsigned long)space_needed);
    return PFalse;
  }

  // read the routing table data
  if (sysctl(mib, 6, buf, &space_needed, NULL, 0) < 0) {
    printf("sysctl: net.route.0.0.dump");
    free(buf);
    return PFalse;
  }


  // Read the interface table
  if (!GetInterfaceTable(if_table)) {
    printf("Interface Table Invalid\n");
    return PFalse;
  }


  // Process the Routing Table data
  limit = buf + space_needed;
  for (ptr = buf; ptr < limit; ptr += rtm->rtm_msglen) {

    unsigned long net_addr, dest_addr, net_mask;
    int metric;
    char name[16];

    rtm = (struct rt_msghdr *)ptr;

    if ( process_rtentry(rtm,ptr, &net_addr, &net_mask, &dest_addr, &metric) ){

      RouteEntry * entry = new RouteEntry(net_addr);
      entry->net_mask = net_mask;
      entry->destination = dest_addr;
      if ( get_ifname(rtm->rtm_index,name) )
        entry->interfaceName = name;
      entry->metric = metric;
      table.Append(entry);

    } // end if

  } // end for loop

  free(buf);
  return PTrue;
}

PBoolean process_rtentry(struct rt_msghdr *rtm, char *ptr, unsigned long *p_net_addr,
                     unsigned long *p_net_mask, unsigned long *p_dest_addr, int *p_metric) {

  struct sockaddr_in *sa_in;

  unsigned long net_addr, dest_addr, net_mask;
  int metric;

  sa_in = (struct sockaddr_in *)(rtm + 1);


  // Check for zero length entry
  if (rtm->rtm_msglen == 0) {
    printf("zero length message\n");
    return PFalse;
  }

  if ((~rtm->rtm_flags&RTF_LLINFO)
#if defined(P_NETBSD) || defined(P_QNX)
        && (~rtm->rtm_flags&RTF_CLONED)     // Net BSD has flag one way
#elif !defined(P_OPENBSD) && !defined(P_FREEBSD)
        && (~rtm->rtm_flags&RTF_WASCLONED)  // MAC has it another
#else
                                            // Open/Free BSD does not have it at all!
#endif
     ) {

    //strcpy(name, if_table[rtm->rtm_index].GetName);

    net_addr=dest_addr=net_mask=metric=0;

    // NET_ADDR
    if(rtm->rtm_addrs&RTA_DST ) {
      if(sa_in->sin_family == AF_INET)
        net_addr = sa_in->sin_addr.s_addr;

      sa_in = (struct sockaddr_in *)((char *)sa_in + ROUNDUP(sa_in->sin_len));
    }

    // DEST_ADDR
    if(rtm->rtm_addrs&RTA_GATEWAY) {
      if(sa_in->sin_family == AF_INET)
        dest_addr = sa_in->sin_addr.s_addr;

      sa_in = (struct sockaddr_in *)((char *)sa_in + ROUNDUP(sa_in->sin_len));
    }

    // NETMASK
    if(rtm->rtm_addrs&RTA_NETMASK && sa_in->sin_len)
      net_mask = sa_in->sin_addr.s_addr;

    if( rtm->rtm_flags&RTF_HOST)
      net_mask = 0xffffffff;


    *p_metric = metric;
    *p_net_addr = net_addr;
    *p_dest_addr = dest_addr;
    *p_net_mask = net_mask;

    return PTrue;

  } else {
    return PFalse;
  }

}

PBoolean get_ifname(int index, char *name) {
  int mib[6];
  size_t needed;
  char *lim, *buf, *next;
  struct if_msghdr *ifm;
  struct  sockaddr_dl *sdl;

  mib[0] = CTL_NET;
  mib[1] = PF_ROUTE;
  mib[2] = 0;
  mib[3] = AF_INET;
  mib[4] = NET_RT_IFLIST;
  mib[5] = index;

  if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) {
    printf("ERR route-sysctl-estimate");
    return PFalse;
  }

  if ((buf = (char *)malloc(needed)) == NULL) {
    printf("ERR malloc");
    return PFalse;
  }

  if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) {
    printf("ERR actual retrieval of routing table");
    free(buf);
    return PFalse;
  }

  lim = buf + needed;

  next = buf;
  if (next < lim) {

    ifm = (struct if_msghdr *)next;

    if (ifm->ifm_type == RTM_IFINFO) {
      sdl = (struct sockaddr_dl *)(ifm + 1);
    } else {
      printf("out of sync parsing NET_RT_IFLIST\n");
      return PFalse;
    }
    next += ifm->ifm_msglen;

    strncpy(name, sdl->sdl_data, sdl->sdl_nlen);
    name[sdl->sdl_nlen] = '\0';

    free(buf);
    return PTrue;

  } else {
    free(buf);
    return PFalse;
  }

}


#elif defined(P_SOLARIS)

/* jpd@louisiana.edu - influenced by Merit.edu's Gated 3.6 routine: krt_rtread_sunos5.c */

#include <sys/stream.h>
#include <stropts.h>
#include <sys/tihdr.h>
#include <sys/tiuser.h>
#include <inet/common.h>
#include <inet/mib2.h>
#include <inet/ip.h>

#ifndef T_CURRENT
#define T_CURRENT       MI_T_CURRENT
#endif

PBoolean PIPSocket::GetRouteTable(RouteTable & table)
{
#define task_pagesize 512
    char buf[task_pagesize];  /* = task_block_malloc(task_pagesize);*/
    int flags;
    int j = 0;
    int  sd, i, rc;
    struct strbuf strbuf;
    struct T_optmgmt_req *tor = (struct T_optmgmt_req *) buf;
    struct T_optmgmt_ack *toa = (struct T_optmgmt_ack *) buf;
    struct T_error_ack  *tea = (struct T_error_ack *) buf;
    struct opthdr *req;

    sd = open("/dev/ip", O_RDWR);
    if (sd < 0) {
#ifdef SOL_COMPLAIN
      perror("can't open mib stream");
#endif
      goto Return;
    }

    strbuf.buf = buf;

    tor->PRIM_type = T_OPTMGMT_REQ;
    tor->OPT_offset = sizeof(struct T_optmgmt_req);
    tor->OPT_length = sizeof(struct opthdr);
    tor->MGMT_flags = T_CURRENT;
    req = (struct opthdr *) (tor + 1);
    req->level = MIB2_IP;    /* any MIB2_xxx value ok here */
    req->name = 0;
    req->len = 0;

    strbuf.len = tor->OPT_length + tor->OPT_offset;
    flags = 0;
    rc = putmsg(sd, &strbuf, (struct strbuf *) 0, flags);
    if (rc == -1) {
#ifdef SOL_COMPLAIN
      perror("putmsg(ctl)");
#endif
      goto Return;
    }
    /*
     * each reply consists of a ctl part for one fixed structure
     * or table, as defined in mib2.h.  The format is a T_OPTMGMT_ACK,
     * containing an opthdr structure.  level/name identify the entry,
     * len is the size of the data part of the message.
     */
    req = (struct opthdr *) (toa + 1);
    strbuf.maxlen = task_pagesize;
    while (++j) {
  flags = 0;
  rc = getmsg(sd, &strbuf, (struct strbuf *) 0, &flags);
  if (rc == -1) {
#ifdef SOL_COMPLAIN
    perror("getmsg(ctl)");
#endif
    goto Return;
  }
  if (rc == 0
      && strbuf.len >= (int)sizeof(struct T_optmgmt_ack)
      && toa->PRIM_type == T_OPTMGMT_ACK
      && toa->MGMT_flags == T_SUCCESS
      && req->len == 0) {
    errno = 0;    /* just to be darned sure it's 0 */
    goto Return;    /* this is EOD msg */
  }

  if (strbuf.len >= (int)sizeof(struct T_error_ack)
      && tea->PRIM_type == T_ERROR_ACK) {
      errno = (tea->TLI_error == TSYSERR) ? tea->UNIX_error : EPROTO;
#ifdef SOL_COMPLAIN
      perror("T_ERROR_ACK in mibget");
#endif
      goto Return;
  }
      
  if (rc != MOREDATA
      || strbuf.len < (int)sizeof(struct T_optmgmt_ack)
      || toa->PRIM_type != T_OPTMGMT_ACK
      || toa->MGMT_flags != T_SUCCESS) {
      errno = ENOMSG;
      goto Return;
  }

  if (req->level != MIB2_IP
#if P_SOLARIS > 7
      || req->name != MIB2_IP_ROUTE
#endif
           ) {  /* == 21 */
      /* If this is not the routing table, skip it */
    /* Note we don't bother with IPv6 (MIB2_IP6_ROUTE) ... */
      strbuf.maxlen = task_pagesize;
      do {
    rc = getmsg(sd, (struct strbuf *) 0, &strbuf, &flags);
      } while (rc == MOREDATA) ;
      continue;
  }

  strbuf.maxlen = (task_pagesize / sizeof (mib2_ipRouteEntry_t)) * sizeof (mib2_ipRouteEntry_t);
  strbuf.len = 0;
  flags = 0;
  do {
      rc = getmsg(sd, (struct strbuf * ) 0, &strbuf, &flags);
      
      switch (rc) {
      case -1:
#ifdef SOL_COMPLAIN
        perror("mibget getmsg(data) failed.");
#endif
        goto Return;

      default:
#ifdef SOL_COMPLAIN
        fprintf(stderr,"mibget getmsg(data) returned %d, strbuf.maxlen = %d, strbuf.len = %d",
            rc,
            strbuf.maxlen,
            strbuf.len);
#endif
        goto Return;

      case MOREDATA:
      case 0:
        {
    mib2_ipRouteEntry_t *rp = (mib2_ipRouteEntry_t *) strbuf.buf;
    mib2_ipRouteEntry_t *lp = (mib2_ipRouteEntry_t *) (strbuf.buf + strbuf.len);

    do {
      char name[256];
#ifdef SOL_DEBUG_RT
      printf("%s -> %s mask %s metric %d %d %d %d %d ifc %.*s type %d/%x/%x\n",
             inet_ntoa(rp->ipRouteDest),
             inet_ntoa(rp->ipRouteNextHop),
             inet_ntoa(rp->ipRouteMask),
             rp->ipRouteMetric1,
             rp->ipRouteMetric2,
             rp->ipRouteMetric3,
             rp->ipRouteMetric4,
             rp->ipRouteMetric5,
             rp->ipRouteIfIndex.o_length,
             rp->ipRouteIfIndex.o_bytes,
             rp->ipRouteType,
             rp->ipRouteInfo.re_ire_type,
             rp->ipRouteInfo.re_flags
        );
#endif
      if (rp->ipRouteInfo.re_ire_type & (IRE_BROADCAST|IRE_CACHE|IRE_LOCAL))
                    continue;
      RouteEntry * entry = new RouteEntry(rp->ipRouteDest);
      entry->net_mask = rp->ipRouteMask;
      entry->destination = rp->ipRouteNextHop;
                  unsigned len = rp->ipRouteIfIndex.o_length;
                  if (len >= sizeof(name))
                    len = sizeof(name)-1;
      strncpy(name, rp->ipRouteIfIndex.o_bytes, len);
      name[len] = '\0';
      entry->interfaceName = name;
      entry->metric =  rp->ipRouteMetric1;
      table.Append(entry);
    } while (++rp < lp) ;
        }
        break;
      }
  } while (rc == MOREDATA) ;
    }

 Return:
    i = errno;
    (void) close(sd);
    errno = i;
    /*task_block_reclaim(task_pagesize, buf);*/
    if (errno)
      return (PFalse);
    else
      return (PTrue);
}


#elif defined(P_VXWORKS)

PBoolean PIPSocket::GetRouteTable(RouteTable & table)
{
  PAssertAlways("PIPSocket::GetRouteTable()");
  for(;;){
    char iface[20];
    unsigned long net_addr, dest_addr, net_mask;
    int  metric;
    RouteEntry * entry = new RouteEntry(net_addr);
    entry->net_mask = net_mask;
    entry->destination = dest_addr;
    entry->interfaceName = iface;
    entry->metric = metric;
    table.Append(entry);
    return PTrue;
  }
}

#else // unsupported platform

#if 0 
PBoolean PIPSocket::GetRouteTable(RouteTable & table)
{
        // Most of this code came from the source code for the "route" command 
        // so it should work on other platforms too. 
        // However, it is not complete (the "address-for-interface" function doesn't exist) and not tested! 
        
        route_table_req_t reqtable; 
        route_req_t *rrtp; 
        int i,ret; 
        
        ret = get_route_table(&reqtable); 
        if (ret < 0) 
        { 
                return PFalse; 
        } 
        
        for (i=reqtable.cnt, rrtp = reqtable.rrtp;i>0;i--, rrtp++) 
        { 
                //the datalink doesn't save addresses/masks for host and default 
                //routes, so the route_req_t may not be filled out completely 
                if (rrtp->flags & RTF_DEFAULT) { 
                        //the IP default route is 0/0 
                        ((struct sockaddr_in *)&rrtp->dst)->sin_addr.s_addr = 0; 
                        ((struct sockaddr_in *)&rrtp->mask)->sin_addr.s_addr = 0; 
        
                } else if (rrtp->flags & RTF_HOST) { 
                        //host routes are addr/32 
                        ((struct sockaddr_in *)&rrtp->mask)->sin_addr.s_addr = 0xffffffff; 
                } 
        
            RouteEntry * entry = new RouteEntry(/* address_for_interface(rrtp->iface) */); 
            entry->net_mask = rrtp->mask; 
            entry->destination = rrtp->dst; 
            entry->interfaceName = rrtp->iface; 
            entry->metric = rrtp->refcnt; 
            table.Append(entry); 
        } 
        
        free(reqtable.rrtp); 
                
        return PTrue; 
#endif // 0

PBoolean PIPSocket::GetRouteTable(RouteTable & table)
{
#warning Platform requires implemetation of GetRouteTable()
  return PFalse;
}
#endif



#ifdef P_HAS_NETLINK

#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/genetlink.h>

#include <memory.h>
#include <errno.h>

class NetLinkRouteTableDetector : public PIPSocket::RouteTableDetector
{
  public:
    int m_fdLink;
    int m_fdCancel[2];

    NetLinkRouteTableDetector()
    {
      m_fdLink = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

      if (m_fdLink != -1) {
        struct sockaddr_nl sanl;
        memset(&sanl, 0, sizeof(sanl));
        sanl.nl_family = AF_NETLINK;
        sanl.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;

        bind(m_fdLink, (struct sockaddr *)&sanl, sizeof(sanl));
      }

      if (pipe(m_fdCancel) == -1)
        m_fdCancel[0] = m_fdCancel[1] = -1;

      PTRACE(3, "PTLIB\tOpened NetLink socket");
    }

    ~NetLinkRouteTableDetector()
    {
      if (m_fdLink != -1)
        close(m_fdLink);
      if (m_fdCancel[0] != -1)
        close(m_fdCancel[0]);
      if (m_fdCancel[1] != -1)
        close(m_fdCancel[1]);
    }

    bool Wait(const PTimeInterval & timeout)
    {
      if (m_fdCancel[0] == -1)
        return false;

      bool ok = true;
      while (ok) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(m_fdCancel[0], &fds);

        struct timeval tval;
        struct timeval * ptval = NULL;
        if (m_fdLink != -1) {
          tval.tv_sec  = timeout.GetMilliSeconds() / 1000;
          tval.tv_usec = (timeout.GetMilliSeconds() % 1000) * 1000;
          ptval = &tval;

          FD_SET(m_fdLink, &fds);
        }

        int result = select(std::max(m_fdLink, m_fdCancel[0])+1, &fds, NULL, NULL, ptval);
        if (result < 0)
          return false;
        if (result == 0)
          return true;

        if (FD_ISSET(m_fdCancel[0], &fds))
          return false;

        struct sockaddr_nl snl;
        char buf[4096];
        struct iovec iov = { buf, sizeof buf };
        struct msghdr msg = { (void*)&snl, sizeof snl, &iov, 1, NULL, 0, 0};

        int status = recvmsg(m_fdLink, &msg, 0);
        if (status < 0)
          return false;

        for (struct nlmsghdr * nlmsg = (struct nlmsghdr *)buf;
             NLMSG_OK(nlmsg, (unsigned)status);
             nlmsg = NLMSG_NEXT(nlmsg, status)) {
          if (nlmsg->nlmsg_len < sizeof(struct nlmsghdr))
            break;

          switch (nlmsg->nlmsg_type) {
            case RTM_NEWADDR :
            case RTM_DELADDR :
              PTRACE(3, "PTLIB\tInterface table change detected via NetLink");
              return true;
          }
        }
      }
      return false;
    }

    void Cancel()
    {
      PAssert(write(m_fdCancel[1], "", 1) == 1, POperatingSystemError);
    }
};

PIPSocket::RouteTableDetector * PIPSocket::CreateRouteTableDetector()
{
  return new NetLinkRouteTableDetector();
}

#elif defined(P_IPHONEOS)

#include <netdb.h>
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <SystemConfiguration/SystemConfiguration.h>
#include <SystemConfiguration/SCNetworkReachability.h>

#define kSCNetworkReachabilityOptionNodeName	CFSTR("nodename")

/*!
	@constant kSCNetworkReachabilityOptionServName
	@discussion A CFString that will be passed to getaddrinfo(3).  An acceptable
		value is either a decimal port number or a service name listed in
		services(5).
 */
#define kSCNetworkReachabilityOptionServName	CFSTR("servname")

/*!
	@constant kSCNetworkReachabilityOptionHints
	@discussion A CFData wrapping a "struct addrinfo" that will be passed to
		getaddrinfo(3).  The caller can supply any of the ai_family,
		ai_socktype, ai_protocol, and ai_flags structure elements.  All
		other elements must be 0 or the null pointer.
 */
#define kSCNetworkReachabilityOptionHints	CFSTR("hints")

class ReachabilityRouteTableDetector : public PIPSocket::RouteTableDetector
{
	SCNetworkReachabilityRef	target_async;

   public:
	SCNetworkReachabilityRef _setupReachability(SCNetworkReachabilityContext *context)
	{
		struct sockaddr_in		sin;
		struct sockaddr_in6		sin6;
		SCNetworkReachabilityRef	target	= NULL;

		bzero(&sin, sizeof(sin));
		sin.sin_len    = sizeof(sin);
		sin.sin_family = AF_INET;

		bzero(&sin6, sizeof(sin6));
		sin6.sin6_len    = sizeof(sin6);
		sin6.sin6_family = AF_INET6;

		const char *anchor = "apple.com";

		if (inet_aton(anchor, &sin.sin_addr) == 1) {

			target = SCNetworkReachabilityCreateWithAddress(NULL, (struct sockaddr *)&sin);
			
		} else if (inet_pton(AF_INET6, anchor, &sin6.sin6_addr) == 1) {
			char	*p;

			p = strchr(anchor, '%');
			if (p != NULL) {
				sin6.sin6_scope_id = if_nametoindex(p + 1);
			}

			target = SCNetworkReachabilityCreateWithAddress(NULL, (struct sockaddr *)&sin6);
			
		} else {
			
		target = SCNetworkReachabilityCreateWithName(NULL, anchor);
				
#if	!TARGET_OS_IPHONE
		if (CFDictionaryGetCount(options) > 0) {

			target = SCNetworkReachabilityCreateWithOptions(NULL, options);

		} else {

			SCPrint(TRUE, stderr, CFSTR("Must specify nodename or servname\n"));
			return NULL;
		}
		CFRelease(options);
#endif			
		}

		return target;
	}
	  
	static void callout(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void *info)
	{
		struct tm	tm_now;
		struct timeval	tv_now;

		(void)gettimeofday(&tv_now, NULL);
		(void)localtime_r(&tv_now.tv_sec, &tm_now);

		PTRACE(1, psprintf("Reachability changed at: %2d:%02d:%02d.%03d, now it is %sreachable",
			tm_now.tm_hour,
			tm_now.tm_min,
			tm_now.tm_sec,
			tv_now.tv_usec / 1000,
			flags & kSCNetworkReachabilityFlagsReachable? "" : "not ")
			);

		ReachabilityRouteTableDetector* d = (ReachabilityRouteTableDetector*) info;	
		d->Cancel();
	}

	ReachabilityRouteTableDetector()
		: RouteTableDetector(),
		target_async(NULL)
	{
		SCNetworkReachabilityContext	context	= { 0, NULL, NULL, NULL, NULL };
		
		target_async = _setupReachability(&context);
		if (target_async == NULL) {
			PTRACE(1, psprintf("  Could not determine status: %s\n", SCErrorString(SCError())));
			return;
		}
		
		context.info = (void*) this;
		
		if (!SCNetworkReachabilitySetCallback(target_async, ReachabilityRouteTableDetector::callout, &context)) {
			PTRACE(1, psprintf("SCNetworkReachabilitySetCallback() failed: %s\n", SCErrorString(SCError())));
			return;
		}

		if (!SCNetworkReachabilityScheduleWithRunLoop(target_async, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode)) {
			PTRACE(1, psprintf("SCNetworkReachabilityScheduleWithRunLoop() failed: %s\n", SCErrorString(SCError()) ) );
			return;
		}
	}
   
	~ReachabilityRouteTableDetector()
	{
		if(target_async != NULL)
			CFRelease(target_async);
	}

    bool Wait(const PTimeInterval & timeout)
    {
		m_cancel.Wait(timeout);
		return PTrue;
    }

    void Cancel()
    {
      m_cancel.Signal();
    }

  private:
    PSyncPoint m_cancel;
	PBoolean m_continue;
};

PIPSocket::RouteTableDetector * PIPSocket::CreateRouteTableDetector()
{
	return new ReachabilityRouteTableDetector();
}

#else // P_HAS_NETLINK, elif defined(P_IPHONEOS)

class DummyRouteTableDetector : public PIPSocket::RouteTableDetector
{
  public:
    bool Wait(const PTimeInterval & timeout)
    {
      return !m_cancel.Wait(timeout);
    }

    void Cancel()
    {
      m_cancel.Signal();
    }

  private:
    PSyncPoint m_cancel;
};


PIPSocket::RouteTableDetector * PIPSocket::CreateRouteTableDetector()
{
  return new DummyRouteTableDetector();
}

#endif // P_HAS_NETLINK, elif defined(P_IPHONEOS)


PBoolean PIPSocket::GetInterfaceTable(InterfaceTable & list, PBoolean includeDown)
{
  PUDPSocket sock;

  PBYTEArray buffer;
  struct ifconf ifConf;
  
#if defined(P_NETBSD)
  struct ifaddrs *ifap, *ifa;

  PAssert(getifaddrs(&ifap) == 0, "getifaddrs failed");

  for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
#else
  // HERE
#if defined(SIOCGIFNUM)
  int ifNum;
  PAssert(::ioctl(sock.GetHandle(), SIOCGIFNUM, &ifNum) >= 0, "could not do ioctl for ifNum");
  ifConf.ifc_len = ifNum * sizeof(ifreq);
#else
  ifConf.ifc_len = 100 * sizeof(ifreq); // That's a LOT of interfaces!
#endif

  ifConf.ifc_req = (struct ifreq *)buffer.GetPointer(ifConf.ifc_len);

  if (ioctl(sock.GetHandle(), SIOCGIFCONF, &ifConf) >= 0) {
    void * ifEndList = (char *)ifConf.ifc_req + ifConf.ifc_len;
    ifreq * ifName = ifConf.ifc_req;
    while (ifName < ifEndList) {
#endif
      struct ifreq ifReq;
#if !defined(P_NETBSD)
          memcpy(&ifReq, ifName, sizeof(ifreq));
#else
          memset(&ifReq, 0, sizeof(ifReq));
          strncpy(ifReq.ifr_name, ifa->ifa_name, sizeof(ifReq.ifr_name) - 1);
#endif

      if (ioctl(sock.GetHandle(), SIOCGIFFLAGS, &ifReq) >= 0) {
        int flags = ifReq.ifr_flags;
        if (includeDown || (flags & IFF_UP) != 0) {
          PString name(ifReq.ifr_name);

          PString macAddr;
#if defined(SIO_Get_MAC_Address)
          memcpy(&ifReq, ifName, sizeof(ifreq));
          if (ioctl(sock.GetHandle(), SIO_Get_MAC_Address, &ifReq) >= 0)
            macAddr = PEthSocket::Address((BYTE *)ifReq.ifr_macaddr);
#endif

#if !defined(P_NETBSD)
          memcpy(&ifReq, ifName, sizeof(ifreq));
#else
          memset(&ifReq, 0, sizeof(ifReq));
          strncpy(ifReq.ifr_name, ifa->ifa_name, sizeof(ifReq.ifr_name) - 1);
#endif

          if (ioctl(sock.GetHandle(), SIOCGIFADDR, &ifReq) >= 0) {

            sockaddr_in * sin = (sockaddr_in *)&ifReq.ifr_addr;
            PIPSocket::Address addr = sin->sin_addr;

#if !defined(P_NETBSD)
            memcpy(&ifReq, ifName, sizeof(ifreq));
#else
            memset(&ifReq, 0, sizeof(ifReq));
            strncpy(ifReq.ifr_name, ifa->ifa_name, sizeof(ifReq.ifr_name) - 1);
#endif

            if (ioctl(sock.GetHandle(), SIOCGIFNETMASK, &ifReq) >= 0) {
              PIPSocket::Address mask = 
#ifndef P_BEOS
    ((sockaddr_in *)&ifReq.ifr_netmask)->sin_addr;
#else
    ((sockaddr_in *)&ifReq.ifr_mask)->sin_addr;
#endif // !P_BEOS
              PINDEX i;
              for (i = 0; i < list.GetSize(); i++) {
#ifdef P_TORNADO
                if (list[i].GetName() == name &&
                    list[i].GetAddress() == addr)
                    if(list[i].GetNetMask() == mask)
#else
                if (list[i].GetName() == name &&
                    list[i].GetAddress() == addr &&
                    list[i].GetNetMask() == mask)
#endif
                  break;
              }
              if (i >= list.GetSize())
                list.Append(PNEW InterfaceEntry(name, addr, mask, macAddr));
            }
          }
        }
      }

#if defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_MACOSX) || defined(P_VXWORKS) || defined(P_RTEMS) || defined(P_QNX)
      // move the ifName pointer along to the next ifreq entry
      ifName = (struct ifreq *)((char *)ifName + _SIZEOF_ADDR_IFREQ(*ifName));
#elif !defined(P_NETBSD)
      ifName++;
#endif

    }
#if !defined(P_NETBSD)
  }
#endif

#if P_HAS_IPV6
  // build a table of IPV6 interface addresses
  // fe800000000000000202e3fffe1ee330 02 40 20 80     eth0
  // 00000000000000000000000000000001 01 80 10 80       lo
  FILE * file;
  int dummy;
  int addr[16];
  char ifaceName[255];
  if ((file = fopen("/proc/net/if_inet6", "r")) != NULL) {
    while (fscanf(file, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x %x %x %x %x %255s\n",
            &addr[0],  &addr[1],  &addr[2],  &addr[3], 
            &addr[4],  &addr[5],  &addr[6],  &addr[7], 
            &addr[8],  &addr[9],  &addr[10], &addr[11], 
            &addr[12], &addr[13], &addr[14], &addr[15], 
           &dummy, &dummy, &dummy, &dummy, ifaceName) != EOF) {
      BYTE bytes[16];
      for (PINDEX i = 0; i < 16; i++)
        bytes[i] = addr[i];

      PString macAddr;
#if defined(SIO_Get_MAC_Address)
      struct ifreq ifReq;
      memset(&ifReq, 0, sizeof(ifReq));
      strncpy(ifReq.ifr_name, ifaceName, sizeof(ifReq.ifr_name) - 1);
      if (ioctl(sock.GetHandle(), SIO_Get_MAC_Address, &ifReq) >= 0)
        macAddr = PEthSocket::Address((BYTE *)ifReq.ifr_macaddr);
#endif

      list.Append(PNEW InterfaceEntry(ifaceName, Address(16, bytes), Address::GetAny(6), macAddr));
    }
    fclose(file);
  }
#endif

  return PTrue;
}

#ifdef P_VXWORKS

int h_errno;

struct hostent * Vx_gethostbyname(char *name, struct hostent *hp)
{
  u_long addr;
  static char staticgethostname[100];

  hp->h_aliases = NULL;
  hp->h_addr_list[1] = NULL;
  if ((int)(addr = inet_addr(name)) != ERROR) {
    memcpy(staticgethostname, &addr, sizeof(addr));
    hp->h_addr_list[0] = staticgethostname;
    h_errno = SUCCESS;
    return hp;
  }
  memcpy(staticgethostname, &addr, sizeof (addr));
  hp->h_addr_list[0] = staticgethostname;
  h_errno = SUCCESS;
  return hp;
}

struct hostent * Vx_gethostbyaddr(char *name, struct hostent *hp)
{
  u_long addr;
  static char staticgethostaddr[100];

  hp->h_aliases = NULL;
  hp->h_addr_list = NULL;

  if ((int)(addr = inet_addr(name)) != ERROR) {
    char ipStorage[INET_ADDR_LEN];
    inet_ntoa_b(*(struct in_addr*)&addr, ipStorage);
    sprintf(staticgethostaddr,"%s",ipStorage);
    hp->h_name = staticgethostaddr;
    h_errno = SUCCESS;
  }
  else
  {
    printf ("_gethostbyaddr: not able to get %s\n",name);
    h_errno = NOTFOUND;
  }
  return hp;
}

#endif // P_VXWORKS


#include "../common/pethsock.cxx"

//////////////////////////////////////////////////////////////////////////////
// PUDPSocket

void PUDPSocket::EnableGQoS()
{
}

PBoolean PUDPSocket::SupportQoS(const PIPSocket::Address & )
{
  return PFalse;
}

///////////////////////////////////////////////////////////////////////////////

