/*
 * sockets.cxx
 *
 * Berkley sockets classes.
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
#include <ptbuildopts.h>
#include <ptlib/sockets.h>

#include <ctype.h>

#ifdef P_VXWORKS
// VxWorks variant of inet_ntoa() allocates INET_ADDR_LEN bytes via malloc
// BUT DOES NOT FREE IT !!!  Use inet_ntoa_b() instead.
#define INET_ADDR_LEN      18
extern "C" void inet_ntoa_b(struct in_addr inetAddress, char *pString);
#endif // P_VXWORKS

#ifdef __NUCLEUS_PLUS__
#include <ConfigurationClass.h>
#endif

#if P_QOS

#ifdef _WIN32
#include <winbase.h>
#include <winreg.h>

#ifndef _WIN32_WCE

void CALLBACK CompletionRoutine(DWORD dwError,
                                DWORD cbTransferred,
                                LPWSAOVERLAPPED lpOverlapped,
                                DWORD dwFlags);
                                

#endif  // _WIN32_WCE
#endif  // _WIN32
#endif // P_QOS


#if !defined(P_MINGW) && !defined(P_CYGWIN)
  #if P_HAS_IPV6 || defined(AI_NUMERICHOST)
    #define HAS_GETADDRINFO 1
  #endif
#else
  #if WINVER > 0x500
    #define HAS_GETADDRINFO 1
  #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// PIPSocket::Address

static int g_defaultIpAddressFamily = PF_INET;  // PF_UNSPEC;   // default to IPV4
static bool g_suppressCanonicalName = false;

static PIPSocket::Address loopback4(127,0,0,1);
static PIPSocket::Address broadcast4(INADDR_BROADCAST);
static PIPSocket::Address any4(0,0,0,0);
static in_addr inaddr_empty;
#if P_HAS_IPV6
static int defaultIPv6ScopeId = 0; 

static PIPSocket::Address loopback6(16,(const BYTE *)"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\001");
static PIPSocket::Address broadcast6(16,(const BYTE *)"\377\002\0\0\0\0\0\0\0\0\0\0\0\0\0\001"); // IPV6 multicast address
static PIPSocket::Address any6(16,(const BYTE *)"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");

#define IPV6_PARAM(p) p
#else
#define IPV6_PARAM(p)
#endif


void PIPSocket::SetSuppressCanonicalName(bool suppress)
{
  g_suppressCanonicalName = suppress;
}


bool PIPSocket::GetSuppressCanonicalName()
{
  return g_suppressCanonicalName;
}


int PIPSocket::GetDefaultIpAddressFamily()
{
  return g_defaultIpAddressFamily;
}


void PIPSocket::SetDefaultIpAddressFamily(int ipAdressFamily)
{
  g_defaultIpAddressFamily = ipAdressFamily;
}


void PIPSocket::SetDefaultIpAddressFamilyV4()
{
  SetDefaultIpAddressFamily(PF_INET);
}


#if P_HAS_IPV6

void PIPSocket::SetDefaultIpAddressFamilyV6()
{
  SetDefaultIpAddressFamily(PF_INET6);
}

void PIPSocket::SetDefaultV6ScopeId(int scopeId) 
{
  defaultIPv6ScopeId = scopeId;
}; 

int PIPSocket::GetDefaultV6ScopeId()
{
  return defaultIPv6ScopeId;
}

PBoolean PIPSocket::IsIpAddressFamilyV6Supported()
{
  int s = ::socket(PF_INET6, SOCK_DGRAM, 0);
  if (s < 0)
    return PFalse;

#if _WIN32
  closesocket(s);
#else
  _close(s);
#endif
  return PTrue;
}

#endif


PIPSocket::Address PIPSocket::GetDefaultIpAny()
{
#if P_HAS_IPV6
  if (g_defaultIpAddressFamily != PF_INET)
    return any6;
#endif

  return any4;
}


#if P_HAS_IPV6

class Psockaddr
{
  public:
    Psockaddr() : ptr(&storage) { memset(&storage, 0, sizeof(storage)); }
    Psockaddr(const PIPSocket::Address & ip, WORD port);
    sockaddr* operator->() const { return addr; }
    operator sockaddr*()   const { return addr; }
    socklen_t GetSize() const;
    PIPSocket::Address GetIP() const;
    WORD GetPort() const;
  private:
    sockaddr_storage storage;
    union {
      sockaddr_storage * ptr;
      sockaddr         * addr;
      sockaddr_in      * addr4;
      sockaddr_in6     * addr6;
    };
};


Psockaddr::Psockaddr(const PIPSocket::Address & ip, WORD port)
 : ptr(&storage) 
{
  memset(&storage, 0, sizeof(storage));

  if (ip.GetVersion() == 6) {
    addr6->sin6_family = AF_INET6;
    addr6->sin6_addr = ip;
    addr6->sin6_port = htons(port);
    addr6->sin6_flowinfo = 0;
    addr6->sin6_scope_id = PIPSocket::GetDefaultV6ScopeId(); // Should be set to the right interface....
  }
  else {
    addr4->sin_family = AF_INET;
    addr4->sin_addr = ip;
    addr4->sin_port = htons(port);
  }
}


socklen_t Psockaddr::GetSize() const
{
  switch (addr->sa_family) {
    case AF_INET :
      return sizeof(sockaddr_in);
    case AF_INET6 :
      // RFC 2133 (Old IPv6 spec) size is 24
      // RFC 2553 (New IPv6 spec) size is 28
      return sizeof(sockaddr_in6);
    default :
      return sizeof(storage);
  }
}


PIPSocket::Address Psockaddr::GetIP() const
{
  switch (addr->sa_family) {
    case AF_INET :
      return addr4->sin_addr;
    case AF_INET6 :
      return addr6->sin6_addr;
    default :
      return 0;
  }
}


WORD Psockaddr::GetPort() const
{
  switch (addr->sa_family) {
    case AF_INET :
      return ntohs(addr4->sin_port);
    case AF_INET6 :
      return ntohs(addr6->sin6_port);
    default :
      return 0;
  }
}

#endif


#if (defined(_WIN32) || defined(WINDOWS)) && !defined(__NUCLEUS_MNT__)
static PWinSock dummyForWinSock; // Assure winsock is initialised
#endif

#if (defined(P_PTHREADS) && !defined(P_THREAD_SAFE_CLIB)) || defined(__NUCLEUS_PLUS__)
#define REENTRANT_BUFFER_LEN 1024
#endif


class PIPCacheData : public PObject
{
  PCLASSINFO(PIPCacheData, PObject)
  public:
    PIPCacheData(struct hostent * ent, const char * original);
#if HAS_GETADDRINFO
    PIPCacheData(struct addrinfo  * addr_info, const char * original);
    void AddEntry(struct addrinfo  * addr_info);
#endif
    const PString & GetHostName() const { return hostname; }
    const PIPSocket::Address & GetHostAddress() const { return address; }
    const PStringArray& GetHostAliases() const { return aliases; }
    PBoolean HasAged() const;
  private:
    PString            hostname;
    PIPSocket::Address address;
    PStringArray       aliases;
    PTime              birthDate;
};



PDICTIONARY(PHostByName_private, PCaselessString, PIPCacheData);

class PHostByName : PHostByName_private
{
  public:
    PBoolean GetHostName(const PString & name, PString & hostname);
    PBoolean GetHostAddress(const PString & name, PIPSocket::Address & address);
    PBoolean GetHostAliases(const PString & name, PStringArray & aliases);
  private:
    PIPCacheData * GetHost(const PString & name);
    PMutex mutex;
  friend void PIPSocket::ClearNameCache();
};

static PMutex creationMutex;
static PHostByName & pHostByName()
{
  PWaitAndSignal m(creationMutex);
  static PHostByName t;
  return t;
}

class PIPCacheKey : public PObject
{
  PCLASSINFO(PIPCacheKey, PObject)
  public:
    PIPCacheKey(const PIPSocket::Address & a)
      { addr = a; }

    PObject * Clone() const
      { return new PIPCacheKey(*this); }

    PINDEX HashFunction() const
      { return (addr[1] + addr[2] + addr[3])%41; }

  private:
    PIPSocket::Address addr;
};

PDICTIONARY(PHostByAddr_private, PIPCacheKey, PIPCacheData);

class PHostByAddr : PHostByAddr_private
{
  public:
    PBoolean GetHostName(const PIPSocket::Address & addr, PString & hostname);
    PBoolean GetHostAddress(const PIPSocket::Address & addr, PIPSocket::Address & address);
    PBoolean GetHostAliases(const PIPSocket::Address & addr, PStringArray & aliases);
  private:
    PIPCacheData * GetHost(const PIPSocket::Address & addr);
    PMutex mutex;
  friend void PIPSocket::ClearNameCache();
};

static PHostByAddr & pHostByAddr()
{
  PWaitAndSignal m(creationMutex);
  static PHostByAddr t;
  return t;
}

#define new PNEW


//////////////////////////////////////////////////////////////////////////////
// IP Caching

PIPCacheData::PIPCacheData(struct hostent * host_info, const char * original)
{
  if (host_info == NULL) {
    address = 0;
    return;
  }

  hostname = host_info->h_name;
  if (host_info->h_addr != NULL)
#ifndef _WIN32_WCE
    address = *(DWORD *)host_info->h_addr;
#else
    address = PIPSocket::Address(host_info->h_length, (const BYTE *)host_info->h_addr);
#endif
  aliases.AppendString(host_info->h_name);

  PINDEX i;
  for (i = 0; host_info->h_aliases[i] != NULL; i++)
    aliases.AppendString(host_info->h_aliases[i]);

  for (i = 0; host_info->h_addr_list[i] != NULL; i++) {
#ifndef _WIN32_WCE
    PIPSocket::Address ip(*(DWORD *)host_info->h_addr_list[i]);
#else
    PIPSocket::Address ip(host_info->h_length, (const BYTE *)host_info->h_addr_list[i]);
#endif
    aliases.AppendString(ip.AsString());
  }

  for (i = 0; i < aliases.GetSize(); i++)
    if (aliases[i] *= original)
      return;

  aliases.AppendString(original);
}


#if HAS_GETADDRINFO

PIPCacheData::PIPCacheData(struct addrinfo * addr_info, const char * original)
{
  PINDEX i;
  if (addr_info == NULL) {
    address = 0;
    return;
  }

  // Fill Host primary informations
  hostname = addr_info->ai_canonname; // Fully Qualified Domain Name (FQDN)
  if (g_suppressCanonicalName || hostname.IsEmpty())
    hostname = original;
  if (addr_info->ai_addr != NULL)
    address = PIPSocket::Address(addr_info->ai_family, addr_info->ai_addrlen, addr_info->ai_addr);

  // Next entries
  while (addr_info != NULL) {
    AddEntry(addr_info);
    addr_info = addr_info->ai_next;
  }

  // Add original as alias or allready added ?
  for (i = 0; i < aliases.GetSize(); i++) {
    if (aliases[i] *= original)
      return;
  }

  aliases.AppendString(original);
}


void PIPCacheData::AddEntry(struct addrinfo * addr_info)
{
  PINDEX i;

  if (addr_info == NULL)
    return;

  // Add canonical name
  PBoolean add_it = PTrue;
  for (i = 0; i < aliases.GetSize(); i++) {
    if (addr_info->ai_canonname != NULL && (aliases[i] *= addr_info->ai_canonname)) {
      add_it = PFalse;
      break;
    }
  }

  if (add_it && addr_info->ai_canonname != NULL)
    aliases.AppendString(addr_info->ai_canonname);

  // Add IP address
  PIPSocket::Address ip(addr_info->ai_family, addr_info->ai_addrlen, addr_info->ai_addr);
  add_it = PTrue;
  for (i = 0; i < aliases.GetSize(); i++) {
    if (aliases[i] *= ip.AsString()) {
      add_it = PFalse;
      break;
    }
  }

  if (add_it)
    aliases.AppendString(ip.AsString());
}

#endif // HAS_GETADDRINFO


static PTimeInterval GetConfigTime(const char * /*key*/, DWORD dflt)
{
  //PConfig cfg("DNS Cache");
  //return cfg.GetInteger(key, dflt);
  return dflt;
}


PBoolean PIPCacheData::HasAged() const
{
  static PTimeInterval retirement = GetConfigTime("Age Limit", 300000); // 5 minutes
  PTime now;
  PTimeInterval age = now - birthDate;
  return age > retirement;
}


PBoolean PHostByName::GetHostName(const PString & name, PString & hostname)
{
  PIPCacheData * host = GetHost(name);

  if (host != NULL) {
    hostname = host->GetHostName();
    hostname.MakeUnique();
  }

  mutex.Signal();

  return host != NULL;
}


PBoolean PHostByName::GetHostAddress(const PString & name, PIPSocket::Address & address)
{
  PIPCacheData * host = GetHost(name);

  if (host != NULL)
    address = host->GetHostAddress();

  mutex.Signal();

  return host != NULL;
}


PBoolean PHostByName::GetHostAliases(const PString & name, PStringArray & aliases)
{
  PIPCacheData * host = GetHost(name);

  if (host != NULL)
    aliases = host->GetHostAliases();

  mutex.Signal();
  return host != NULL;
}


PIPCacheData * PHostByName::GetHost(const PString & name)
{
  mutex.Wait();

  PString key = name;
  PINDEX len = key.GetLength();

  // Check for a legal hostname as per RFC952
  // but drop the requirement for leading alpha as per RFC 1123
  if (key.IsEmpty() ||
      key.FindSpan("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.") != P_MAX_INDEX ||
      key[len-1] == '-') {
    PTRACE(3, "Socket\tIllegal RFC952 characters in DNS name \"" << key << '"');
    return NULL;
  }

  // We lowercase this way rather than toupper() as that is locale dependent, and DNS names aren't.
  for (PINDEX i = 0; i < len; i++) {
    if (key[i] >= 'a')
      key[i] &= 0x5f;
  }

  PIPCacheData * host = GetAt(key);
  int localErrNo = NO_DATA;

  if (host != NULL && host->HasAged()) {
    SetAt(key, NULL);
    host = NULL;
  }

  if (host == NULL) {
    mutex.Signal();

#if HAS_GETADDRINFO

    struct addrinfo *res = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    if (!g_suppressCanonicalName)
      hints.ai_flags = AI_CANONNAME;
    hints.ai_family = g_defaultIpAddressFamily;
    localErrNo = getaddrinfo((const char *)name, NULL , &hints, &res);
    if (localErrNo != 0 && g_defaultIpAddressFamily == AF_INET6) {
      hints.ai_family = AF_INET;
      localErrNo = getaddrinfo((const char *)name, NULL , &hints, &res);
    }
    host = new PIPCacheData(localErrNo != NETDB_SUCCESS ? NULL : res, name);
    if (res != NULL)
      freeaddrinfo(res);

#else // HAS_GETADDRINFO

    int retry = 3;
    struct hostent * host_info;

#ifdef P_AIX

    struct hostent_data ht_data;
    memset(&ht_data, 0, sizeof(ht_data));
    struct hostent hostEnt;
    do {
      host_info = &hostEnt;
      ::gethostbyname_r(name,
                        host_info,
                        &ht_data);
      localErrNo = h_errno;
    } while (localErrNo == TRY_AGAIN && --retry > 0);

#elif defined(P_RTEMS) || defined(P_CYGWIN) || defined(P_MINGW)

    host_info = ::gethostbyname(name);
    localErrNo = h_errno;

#elif defined P_VXWORKS

    struct hostent hostEnt;
    host_info = Vx_gethostbyname((char *)name, &hostEnt);
    localErrNo = h_errno;

#elif defined P_LINUX || defined(P_GNU_HURD)

    char buffer[REENTRANT_BUFFER_LEN];
    struct hostent hostEnt;
    do {
      if (::gethostbyname_r(name,
                            &hostEnt,
                            buffer, REENTRANT_BUFFER_LEN,
                            &host_info,
                            &localErrNo) == 0)
        localErrNo = NETDB_SUCCESS;
    } while (localErrNo == TRY_AGAIN && --retry > 0);

#elif (defined(P_PTHREADS) && !defined(P_THREAD_SAFE_CLIB)) || defined(__NUCLEUS_PLUS__)

    char buffer[REENTRANT_BUFFER_LEN];
    struct hostent hostEnt;
    do {
      host_info = ::gethostbyname_r(name,
                                    &hostEnt,
                                    buffer, REENTRANT_BUFFER_LEN,
                                    &localErrNo);
    } while (localErrNo == TRY_AGAIN && --retry > 0);

#else

    host_info = ::gethostbyname(name);
    localErrNo = h_errno;

#endif

    if (localErrNo != NETDB_SUCCESS || retry == 0)
      host_info = NULL;
    host = new PIPCacheData(host_info, name);

#endif //HAS_GETADDRINFO

    mutex.Wait();

    SetAt(key, host);
  }

  if (host->GetHostAddress().IsValid())
    return host;

  PTRACE(4, "Socket\tName lookup of \"" << name << "\" failed: errno=" << localErrNo);
  return NULL;
}


PBoolean PHostByAddr::GetHostName(const PIPSocket::Address & addr, PString & hostname)
{
  PIPCacheData * host = GetHost(addr);

  if (host != NULL) {
    hostname = host->GetHostName();
    hostname.MakeUnique();
  }

  mutex.Signal();
  return host != NULL;
}


PBoolean PHostByAddr::GetHostAddress(const PIPSocket::Address & addr, PIPSocket::Address & address)
{
  PIPCacheData * host = GetHost(addr);

  if (host != NULL)
    address = host->GetHostAddress();

  mutex.Signal();
  return host != NULL;
}


PBoolean PHostByAddr::GetHostAliases(const PIPSocket::Address & addr, PStringArray & aliases)
{
  PIPCacheData * host = GetHost(addr);

  if (host != NULL)
    aliases = host->GetHostAliases();

  mutex.Signal();
  return host != NULL;
}

PIPCacheData * PHostByAddr::GetHost(const PIPSocket::Address & addr)
{
  mutex.Wait();

  PIPCacheKey key = addr;
  PIPCacheData * host = GetAt(key);

  if (host != NULL && host->HasAged()) {
    SetAt(key, NULL);
    host = NULL;
  }

  if (host == NULL) {
    mutex.Signal();

    int retry = 3;
    int localErrNo = NETDB_SUCCESS;
    struct hostent * host_info;

#ifdef P_AIX

    struct hostent_data ht_data;
    struct hostent hostEnt;
    do {
      host_info = &hostEnt;
      ::gethostbyaddr_r((char *)addr.GetPointer(), addr.GetSize(),
                        PF_INET, 
                        host_info,
                        &ht_data);
      localErrNo = h_errno;
    } while (localErrNo == TRY_AGAIN && --retry > 0);

#elif defined P_RTEMS || defined P_CYGWIN || defined P_MINGW

    host_info = ::gethostbyaddr(addr.GetPointer(), addr.GetSize(), PF_INET);
    localErrNo = h_errno;

#elif defined P_VXWORKS

    struct hostent hostEnt;
    host_info = Vx_gethostbyaddr(addr.GetPointer(), &hostEnt);

#elif defined P_LINUX || defined(P_GNU_HURD)

    char buffer[REENTRANT_BUFFER_LEN];
    struct hostent hostEnt;
    do {
      ::gethostbyaddr_r(addr.GetPointer(), addr.GetSize(),
                        PF_INET, 
                        &hostEnt,
                        buffer, REENTRANT_BUFFER_LEN,
                        &host_info,
                        &localErrNo);
    } while (localErrNo == TRY_AGAIN && --retry > 0);

#elif (defined(P_PTHREADS) && !defined(P_THREAD_SAFE_CLIB)) || defined(__NUCLEUS_PLUS__)

    char buffer[REENTRANT_BUFFER_LEN];
    struct hostent hostEnt;
    do {
      host_info = ::gethostbyaddr_r(addr.GetPointer(), addr.GetSize(),
                                    PF_INET, 
                                    &hostEnt,
                                    buffer, REENTRANT_BUFFER_LEN,
                                    &localErrNo);
    } while (localErrNo == TRY_AGAIN && --retry > 0);

#else

    host_info = ::gethostbyaddr(addr.GetPointer(), addr.GetSize(), PF_INET);
    localErrNo = h_errno;

#if defined(_WIN32) || defined(WINDOWS)  // Kludge to avoid strange 95 bug
    extern PBoolean P_IsOldWin95();
    if (P_IsOldWin95() && host_info != NULL && host_info->h_addr_list[0] != NULL)
      host_info->h_addr_list[1] = NULL;
#endif

#endif

    mutex.Wait();

    if (localErrNo != NETDB_SUCCESS || retry == 0)
      return NULL;

    host = new PIPCacheData(host_info, addr.AsString());

    SetAt(key, host);
  }

  return host->GetHostAddress().IsValid() ? host : NULL;
}


//////////////////////////////////////////////////////////////////////////////
// P_fd_set

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif

P_fd_set::P_fd_set()
{
  Construct();
  Zero();
}


P_fd_set::P_fd_set(SOCKET fd)
{
  Construct();
  Zero();
  FD_SET(fd, set);
}


P_fd_set & P_fd_set::operator=(SOCKET fd)
{
  PAssert(fd < max_fd, PInvalidParameter);
  Zero();
  FD_SET(fd, set);
  return *this;
}


P_fd_set & P_fd_set::operator+=(SOCKET fd)
{
  PAssert(fd < max_fd, PInvalidParameter);
  FD_SET(fd, set);
  return *this;
}


P_fd_set & P_fd_set::operator-=(SOCKET fd)
{
  PAssert(fd < max_fd, PInvalidParameter);
  FD_CLR(fd, set);
  return *this;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif


//////////////////////////////////////////////////////////////////////////////
// P_timeval

P_timeval::P_timeval()
{
  tval.tv_usec = 0;
  tval.tv_sec = 0;
  infinite = PFalse;
}


P_timeval & P_timeval::operator=(const PTimeInterval & time)
{
  infinite = time == PMaxTimeInterval;
  tval.tv_usec = (long)(time.GetMilliSeconds()%1000)*1000;
  tval.tv_sec = time.GetSeconds();
  return *this;
}


//////////////////////////////////////////////////////////////////////////////
// PSocket

PSocket::PSocket()
{
  port = 0;
#if P_HAS_RECVMSG
  catchReceiveToAddr = PFalse;
#endif
}


PBoolean PSocket::Connect(const PString &)
{
  PAssertAlways("Illegal operation.");
  return PFalse;
}


PBoolean PSocket::Listen(unsigned, WORD, Reusability)
{
  PAssertAlways("Illegal operation.");
  return PFalse;
}


PBoolean PSocket::Accept(PSocket &)
{
  PAssertAlways("Illegal operation.");
  return PFalse;
}


PBoolean PSocket::SetOption(int option, int value, int level)
{
#ifdef _WIN32_WCE
  if(option == SO_RCVBUF || option == SO_SNDBUF || option == IP_TOS)
    return PTrue;
#endif

  return ConvertOSError(::setsockopt(os_handle, level, option,
                                     (char *)&value, sizeof(value)));
}


PBoolean PSocket::SetOption(int option, const void * valuePtr, PINDEX valueSize, int level)
{
  return ConvertOSError(::setsockopt(os_handle, level, option,
                                     (char *)valuePtr, valueSize));
}


PBoolean PSocket::GetOption(int option, int & value, int level)
{
  socklen_t valSize = sizeof(value);
  return ConvertOSError(::getsockopt(os_handle, level, option,
                                     (char *)&value, &valSize));
}


PBoolean PSocket::GetOption(int option, void * valuePtr, PINDEX valueSize, int level)
{
  return ConvertOSError(::getsockopt(os_handle, level, option,
                                     (char *)valuePtr, (socklen_t *)&valueSize));
}


PBoolean PSocket::Shutdown(ShutdownValue value)
{
  return ConvertOSError(::shutdown(os_handle, value));
}


WORD PSocket::GetProtocolByName(const PString & name)
{
#if !defined(__NUCLEUS_PLUS__) && !defined(P_VXWORKS)
  struct protoent * ent = getprotobyname(name);
  if (ent != NULL)
    return ent->p_proto;
#endif

  return 0;
}


PString PSocket::GetNameByProtocol(WORD proto)
{
#if !defined(__NUCLEUS_PLUS__) && !defined(P_VXWORKS)
  struct protoent * ent = getprotobynumber(proto);
  if (ent != NULL)
    return ent->p_name;
#endif

  return psprintf("%u", proto);
}


WORD PSocket::GetPortByService(const PString & serviceName) const
{
  return GetPortByService(GetProtocolName(), serviceName);
}


WORD PSocket::GetPortByService(const char * protocol, const PString & service)
{
  // if the string is a valid integer, then use integer value
  // this avoids stupid problems like operating systems that match service
  // names to substrings (like "2000" to "taskmaster2000")
  if (service.FindSpan("0123456789") == P_MAX_INDEX)
    return (WORD)service.AsUnsigned();

#if defined( __NUCLEUS_PLUS__ )
  PAssertAlways("PSocket::GetPortByService: problem as no ::getservbyname in Nucleus NET");
  return 0;
#elif defined(P_VXWORKS)
  PAssertAlways("PSocket::GetPortByService: problem as no ::getservbyname in VxWorks");
  return 0;
#else
  PINDEX space = service.FindOneOf(" \t\r\n");
  struct servent * serv = ::getservbyname(service(0, space-1), protocol);
  if (serv != NULL)
    return ntohs(serv->s_port);

  long portNum;
  if (space != P_MAX_INDEX)
    portNum = atol(service(space+1, P_MAX_INDEX));
  else if (isdigit(service[0]))
    portNum = atoi(service);
  else
    portNum = -1;

  if (portNum < 0 || portNum > 65535)
    return 0;

  return (WORD)portNum;
#endif
}


PString PSocket::GetServiceByPort(WORD port) const
{
  return GetServiceByPort(GetProtocolName(), port);
}


PString PSocket::GetServiceByPort(const char * protocol, WORD port)
{
#if !defined(__NUCLEUS_PLUS__) && !defined(P_VXWORKS)
  struct servent * serv = ::getservbyport(htons(port), protocol);
  if (serv != NULL)
    return PString(serv->s_name);
  else
#endif
    return PString(PString::Unsigned, port);
}


void PSocket::SetPort(WORD newPort)
{
  PAssert(!IsOpen(), "Cannot change port number of opened socket");
  port = newPort;
}


void PSocket::SetPort(const PString & service)
{
  PAssert(!IsOpen(), "Cannot change port number of opened socket");
  port = GetPortByService(service);
}


WORD PSocket::GetPort() const
{
  return port;
}


PString PSocket::GetService() const
{
  return GetServiceByPort(port);
}


int PSocket::Select(PSocket & sock1, PSocket & sock2)
{
  return Select(sock1, sock2, PMaxTimeInterval);
}


int PSocket::Select(PSocket & sock1,
                    PSocket & sock2,
                    const PTimeInterval & timeout)
{
  SelectList read, dummy1, dummy2;
  read += sock1;
  read += sock2;

  Errors lastError;
  int osError;
  if (!ConvertOSError(Select(read, dummy1, dummy2, timeout), lastError, osError))
    return lastError;

  switch (read.GetSize()) {
    case 0 :
      return 0;
    case 2 :
      return -3;
    default :
      return &read.front() == &sock1 ? -1 : -2;
  }
}


PChannel::Errors PSocket::Select(SelectList & read)
{
  SelectList dummy1, dummy2;
  return Select(read, dummy1, dummy2, PMaxTimeInterval);
}


PChannel::Errors PSocket::Select(SelectList & read, const PTimeInterval & timeout)
{
  SelectList dummy1, dummy2;
  return Select(read, dummy1, dummy2, timeout);
}


PChannel::Errors PSocket::Select(SelectList & read, SelectList & write)
{
  SelectList dummy1;
  return Select(read, write, dummy1, PMaxTimeInterval);
}


PChannel::Errors PSocket::Select(SelectList & read,
                                 SelectList & write,
                                 const PTimeInterval & timeout)
{
  SelectList dummy1;
  return Select(read, write, dummy1, timeout);
}


PChannel::Errors PSocket::Select(SelectList & read,
                                 SelectList & write,
                                 SelectList & except)
{
  return Select(read, write, except, PMaxTimeInterval);
}


//////////////////////////////////////////////////////////////////////////////
// PIPSocket

PIPSocket::PIPSocket()
{
}


void PIPSocket::ClearNameCache()
{
  pHostByName().mutex.Wait();
  pHostByName().RemoveAll();
  pHostByName().mutex.Signal();

  pHostByAddr().mutex.Wait();
  pHostByAddr().RemoveAll();
  pHostByAddr().mutex.Signal();

#if (defined(_WIN32) || defined(WINDOWS)) && !defined(__NUCLEUS_MNT__) // Kludge to avoid strange NT bug
  static PTimeInterval delay = GetConfigTime("NT Bug Delay", 0);
  if (delay != 0) {
    ::Sleep(delay.GetInterval());
    ::gethostbyname("www.microsoft.com");
  }
#endif
  PTRACE(4, "Socket\tCleared DNS cache.");
}


PString PIPSocket::GetName() const
{
#if P_HAS_IPV6

  Psockaddr sa;
  socklen_t size = sa.GetSize();
  if (getpeername(os_handle, sa, &size) == 0)
    return GetHostName(sa.GetIP()) + psprintf(":%u", sa.GetPort());

#else

  sockaddr_in address;
  socklen_t size = sizeof(address);
  if (getpeername(os_handle, (struct sockaddr *)&address, &size) == 0)
    return GetHostName(address.sin_addr) + psprintf(":%u", ntohs(address.sin_port));

#endif

  return PString::Empty();
}


PString PIPSocket::GetHostName()
{
  char name[100];
  if (gethostname(name, sizeof(name)-1) != 0)
    return "localhost";
  name[sizeof(name)-1] = '\0';
  return name;
}


PString PIPSocket::GetHostName(const PString & hostname)
{
  // lookup the host address using inet_addr, assuming it is a "." address
  Address temp = hostname;
  if (temp.IsValid())
    return GetHostName(temp);

  PString canonicalname;
  if (pHostByName().GetHostName(hostname, canonicalname))
    return canonicalname;

  return hostname;
}


PString PIPSocket::GetHostName(const Address & addr)
{
  if (!addr.IsValid())
    return addr.AsString();

  PString hostname;
  if (pHostByAddr().GetHostName(addr, hostname))
    return hostname;

#if P_HAS_IPV6
  if (addr.GetVersion() == 6)
    return '[' + addr.AsString() + ']';
#endif

  return addr.AsString();
}


PBoolean PIPSocket::GetHostAddress(Address & addr)
{
  return pHostByName().GetHostAddress(GetHostName(), addr);
}


PBoolean PIPSocket::GetHostAddress(const PString & hostname, Address & addr)
{
  if (hostname.IsEmpty())
    return PFalse;

  // Check for special case of "[ipaddr]"
  if (hostname[0] == '[') {
    PINDEX end = hostname.Find(']');
    if (end != P_MAX_INDEX) {
      if (addr.FromString(hostname(1, end-1)))
        return PTrue;
    }
  }

  // Assuming it is a "." address and return if so
  if (addr.FromString(hostname))
    return PTrue;

  // otherwise lookup the name as a host name
  return pHostByName().GetHostAddress(hostname, addr);
}


PStringArray PIPSocket::GetHostAliases(const PString & hostname)
{
  PStringArray aliases;

  // lookup the host address using inet_addr, assuming it is a "." address
  Address addr = hostname;
  if (addr.IsValid())
    pHostByAddr().GetHostAliases(addr, aliases);
  else
    pHostByName().GetHostAliases(hostname, aliases);

  return aliases;
}


PStringArray PIPSocket::GetHostAliases(const Address & addr)
{
  PStringArray aliases;

  pHostByAddr().GetHostAliases(addr, aliases);

  return aliases;
}


PString PIPSocket::GetLocalAddress()
{
  PStringStream str;
  Address addr;
  WORD port;
  if (GetLocalAddress(addr, port))
    str << addr << ':' << port;
  return str;
}


PBoolean PIPSocket::GetLocalAddress(Address & addr)
{
  WORD dummy;
  return GetLocalAddress(addr, dummy);
}


PBoolean PIPSocket::GetLocalAddress(PIPSocketAddressAndPort & addr)
{
  Address ip;
  WORD port;
  if (!GetLocalAddress(ip, port))
    return false;

  addr.SetAddress(ip, port);
  return true;
}


PBoolean PIPSocket::GetLocalAddress(Address & addr, WORD & portNum)
{
#if P_HAS_IPV6
  Address   addrv4;
  Address   peerv4;
  Psockaddr sa;
  socklen_t size = sa.GetSize();
  if (!ConvertOSError(::getsockname(os_handle, sa, &size)))
    return PFalse;

  addr = sa.GetIP();
  portNum = sa.GetPort();

  // If the remote host is an IPv4 only host and our interface if an IPv4/IPv6 mapped
  // Then return an IPv4 address instead of an IPv6
  if (GetPeerAddress(peerv4)) {
    if ((peerv4.GetVersion()==4)||(peerv4.IsV4Mapped())) {
      if (addr.IsV4Mapped()) {
        addr = Address(addr[12], addr[13], addr[14], addr[15]);
      }
    }
  }
  
#else

  sockaddr_in address;
  socklen_t size = sizeof(address);
  if (!ConvertOSError(::getsockname(os_handle,(struct sockaddr*)&address,&size)))
    return PFalse;

  addr = address.sin_addr;
  portNum = ntohs(address.sin_port);

#endif

  return PTrue;
}


PString PIPSocket::GetPeerAddress()
{
  PStringStream str;
  Address addr;
  WORD port;
  if (GetPeerAddress(addr, port))
    str << addr << ':' << port;
  return str;
}


PBoolean PIPSocket::GetPeerAddress(Address & addr)
{
  WORD portNum;
  return GetPeerAddress(addr, portNum);
}


PBoolean PIPSocket::GetPeerAddress(PIPSocketAddressAndPort & addr)
{
  Address ip;
  WORD port;
  if (!GetPeerAddress(ip, port))
    return false;

  addr.SetAddress(ip, port);
  return true;
}


PBoolean PIPSocket::GetPeerAddress(Address & addr, WORD & portNum)
{
#if P_HAS_IPV6

  Psockaddr sa;
  socklen_t size = sa.GetSize();
  if (!ConvertOSError(::getpeername(os_handle, sa, &size)))
    return PFalse;

  addr = sa.GetIP();
  portNum = sa.GetPort();

#else

  sockaddr_in address;
  socklen_t size = sizeof(address);
  if (!ConvertOSError(::getpeername(os_handle,(struct sockaddr*)&address,&size)))
    return PFalse;

  addr = address.sin_addr;
  portNum = ntohs(address.sin_port);

#endif

  return PTrue;
}


PString PIPSocket::GetLocalHostName()
{
  Address addr;

  if (GetLocalAddress(addr))
    return GetHostName(addr);

  return PString::Empty();
}


PString PIPSocket::GetPeerHostName()
{
  Address addr;

  if (GetPeerAddress(addr))
    return GetHostName(addr);

  return PString::Empty();
}


PBoolean PIPSocket::Connect(const PString & host)
{
  Address ipnum(host);
#if P_HAS_IPV6
  if (ipnum.IsValid() || GetHostAddress(host, ipnum))
    return Connect(GetDefaultIpAny(), 0, ipnum);
#else
  if (ipnum.IsValid() || GetHostAddress(host, ipnum))
    return Connect(INADDR_ANY, 0, ipnum);
#endif  
  return PFalse;
}


PBoolean PIPSocket::Connect(const Address & addr)
{
#if P_HAS_IPV6
  return Connect(GetDefaultIpAny(), 0, addr);
#else
  return Connect(INADDR_ANY, 0, addr);
#endif
}


PBoolean PIPSocket::Connect(WORD localPort, const Address & addr)
{
#if P_HAS_IPV6
  return Connect(GetDefaultIpAny(), localPort, addr);
#else
  return Connect(INADDR_ANY, localPort, addr);
#endif
}


PBoolean PIPSocket::Connect(const Address & iface, const Address & addr)
{
  return Connect(iface, 0, addr);
}


PBoolean PIPSocket::Connect(const Address & iface, WORD localPort, const Address & addr)
{
  // close the port if it is already open
  if (IsOpen())
    Close();

  // make sure we have a port
  PAssert(port != 0, "Cannot connect socket without setting port");

#if P_HAS_IPV6

  Psockaddr sa(addr, port);

  // attempt to create a socket with the right family
  if (!OpenSocket(sa->sa_family))
    return PFalse;

  if (localPort != 0 || iface.IsValid()) {
    Psockaddr bind_sa(iface, localPort);

    if (!SetOption(SO_REUSEADDR, 0)) {
      os_close();
      return PFalse;
    }
    
    if (!ConvertOSError(::bind(os_handle, bind_sa, bind_sa.GetSize()))) {
      os_close();
      return PFalse;
    }
  }
  
  // attempt to connect
  if (os_connect(sa, sa.GetSize()))
    return PTrue;
  
#else

  // attempt to create a socket
  if (!OpenSocket())
    return PFalse;

  // attempt to connect
  sockaddr_in sin;
  if (localPort != 0 || iface.IsValid()) {
    if (!SetOption(SO_REUSEADDR, 0)) {
      os_close();
      return PFalse;
    }
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = iface;
    sin.sin_port        = htons(localPort);       // set the port
    if (!ConvertOSError(::bind(os_handle, (struct sockaddr*)&sin, sizeof(sin)))) {
      os_close();
      return PFalse;
    }
  }

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port   = htons(port);  // set the port
  sin.sin_addr   = addr;
  if (os_connect((struct sockaddr *)&sin, sizeof(sin)))
    return PTrue;

#endif

  os_close();
  return PFalse;
}


PBoolean PIPSocket::Listen(unsigned queueSize, WORD newPort, Reusability reuse)
{
#if P_HAS_IPV6
  return Listen(GetDefaultIpAny(), queueSize, newPort, reuse);
#else
  return Listen(INADDR_ANY, queueSize, newPort, reuse);
#endif
}


PBoolean PIPSocket::Listen(const Address & bindAddr,
                       unsigned,
                       WORD newPort,
                       Reusability reuse)
{
  // make sure we have a port
  if (newPort != 0)
    port = newPort;

#if P_HAS_IPV6
  Psockaddr bind_sa(bindAddr, port); 
#endif

#if P_HAS_IPV6
  // Always close and re-open as the bindAddr address family might change.
  Close();

  // attempt to create a socket
  if (!OpenSocket(bind_sa->sa_family))
    return PFalse;
#else
  if (!IsOpen()) {
    if (!OpenSocket())
      return PFalse;
  }
#endif
  
#ifndef P_BEOS
#if P_HAS_IPV6
  if(bind_sa->sa_family != AF_INET6)
#endif
  {
   // attempt to listen
   if (!SetOption(SO_REUSEADDR, reuse == CanReuseAddress ? 1 : 0)) {
     os_close();
     return PFalse;
   }
  }
#endif // BEOS

#if P_HAS_IPV6

  if (ConvertOSError(::bind(os_handle, bind_sa, bind_sa.GetSize()))) {
    Psockaddr sa;
    socklen_t size = sa.GetSize();
    if (!ConvertOSError(::getsockname(os_handle, sa, &size)))
      return PFalse;

    port = sa.GetPort();

    if (!bindAddr.IsMulticast())
      return true;

    // Probably move this stuff into a separate method
    if (bindAddr.GetVersion() == 4) {
      struct ip_mreq  mreq;
      mreq.imr_multiaddr = in_addr(bindAddr);
      mreq.imr_interface.s_addr = htonl(INADDR_ANY);
      if (SetOption(IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq), IPPROTO_IP)) {
        PTRACE(4, "Socket\tJoined multicast group");
        return true;
      }
      PTRACE(1, "Socket\tFailed to join multicast group");
    }
    else {
      PTRACE(1, "Socket\tIPV6 Multicast join not implemented yet");
    }

    return false;
  }

#else

  // attempt to listen
  sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family      = AF_INET;
  sin.sin_addr.s_addr = bindAddr;
  sin.sin_port        = htons(port);       // set the port

#ifdef __NUCLEUS_NET__
  int bind_result;
  if (port == 0)
    bind_result = ::bindzero(os_handle, (struct sockaddr*)&sin, sizeof(sin));
  else
    bind_result = ::bind(os_handle, (struct sockaddr*)&sin, sizeof(sin));
  if (ConvertOSError(bind_result))
#else
  if (ConvertOSError(::bind(os_handle, (struct sockaddr*)&sin, sizeof(sin))))
#endif
  {
    socklen_t size = sizeof(sin);
    if (ConvertOSError(::getsockname(os_handle, (struct sockaddr*)&sin, &size))) {
      port = ntohs(sin.sin_port);

    if (!IN_MULTICAST(ntohl(sin.sin_addr.s_addr)))
        return true;

      struct ip_mreq  mreq;
      mreq.imr_multiaddr.s_addr = sin.sin_addr.s_addr;
      mreq.imr_interface.s_addr = htonl(INADDR_ANY);
      if (SetOption(IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq))) {
        PTRACE(4, "Socket\tJoined multicast group");
        return true;
      }
      PTRACE(1, "Socket\tFailed to join multicast group");
    }
  }

#endif

  os_close();
  return PFalse;
}


#if P_HAS_IPV6

/// Check for v4 mapped in v6 address ::ffff:a.b.c.d
PBoolean PIPSocket::Address::IsV4Mapped() const
{
  if (version != 6)
    return PFalse;
  return IN6_IS_ADDR_V4MAPPED(&v.six) || IN6_IS_ADDR_V4COMPAT(&v.six);
}

/// Check for link-local address fe80::/10
PBoolean PIPSocket::Address::IsLinkLocal() const
{
  if (version != 6)
    return PFalse;
  return IN6_IS_ADDR_LINKLOCAL(&v.six);
}

#endif

const PIPSocket::Address & PIPSocket::Address::GetLoopback(int IPV6_PARAM(version))
{
#if P_HAS_IPV6
  if (version == 6)
    return loopback6;
#endif
  return loopback4;
}

const PIPSocket::Address & PIPSocket::Address::GetAny(int IPV6_PARAM(version))
{
#if P_HAS_IPV6
  if (version == 6)
    return any6;
#endif
  return any4;
}

const PIPSocket::Address PIPSocket::Address::GetBroadcast(int IPV6_PARAM(version))
{
#if P_HAS_IPV6
  if (version == 6)
    return broadcast6;
#endif
  return broadcast4;
}


PBoolean PIPSocket::Address::IsAny() const
{
  return (!IsValid());
}


PIPSocket::Address::Address()
{
#if P_HAS_IPV6
  if (g_defaultIpAddressFamily == AF_INET6)
    *this = loopback6;
  else
#endif
    *this = loopback4;
}


PIPSocket::Address::Address(const PString & dotNotation)
{
  operator=(dotNotation);
}


PIPSocket::Address::Address(PINDEX len, const BYTE * bytes)
{
  switch (len) {
#if P_HAS_IPV6
    case 16 :
      version = 6;
      memcpy(&v.six, bytes, len);
      break;
#endif
    case 4 :
      version = 4;
      memcpy(&v.four, bytes, len);
      break;

    default :
      version = 0;
  }
}


PIPSocket::Address::Address(const in_addr & addr)
{
  version = 4;
  v.four = addr;
}


#if P_HAS_IPV6
PIPSocket::Address::Address(const in6_addr & addr)
{
  version = 6;
  v.six = addr;
}

#endif


// Create an IP (v4 or v6) address from a sockaddr (sockaddr_in, sockaddr_in6 or sockaddr_in6_old) structure
PIPSocket::Address::Address(const int ai_family, const int ai_addrlen, struct sockaddr *ai_addr)
{
  switch (ai_family) {
#if P_HAS_IPV6
    case AF_INET6:
      if (ai_addrlen < (int)sizeof(sockaddr_in6)) {
        PTRACE(1, "Socket\tsockaddr size too small (" << ai_addrlen << ")  for family " << ai_family);
        break;
      }

      version = 6;
      v.six = ((struct sockaddr_in6 *)ai_addr)->sin6_addr;
      //sin6_scope_id, should be taken into account for link local addresses
      return;
#endif
    case AF_INET: 
      if (ai_addrlen < (int)sizeof(sockaddr_in)) {
        PTRACE(1, "Socket\tsockaddr size too small (" << ai_addrlen << ")  for family " << ai_family);
        break;
      }

      version = 4;
      v.four = ((struct sockaddr_in  *)ai_addr)->sin_addr;
      return;

    default :
      PTRACE(1, "Socket\tIllegal family (" << ai_family << ") specified.");
  }

  version = 0;
}


#ifdef __NUCLEUS_NET__
PIPSocket::Address::Address(const struct id_struct & addr)
{
  operator=(addr);
}


PIPSocket::Address & PIPSocket::Address::operator=(const struct id_struct & addr)
{
  s_addr = (((unsigned long)addr.is_ip_addrs[0])<<24) +
           (((unsigned long)addr.is_ip_addrs[1])<<16) +
           (((unsigned long)addr.is_ip_addrs[2])<<8) +
           (((unsigned long)addr.is_ip_addrs[3]));
  return *this;
}
#endif
 

PIPSocket::Address & PIPSocket::Address::operator=(const in_addr & addr)
{
  version = 4;
  v.four = addr;
  return *this;
}

#if P_HAS_IPV6
PIPSocket::Address & PIPSocket::Address::operator=(const in6_addr & addr)
{
  version = 6;
  v.six = addr;
  return *this;
}
#endif


PObject::Comparison PIPSocket::Address::Compare(const PObject & obj) const
{
  const PIPSocket::Address & other = (const PIPSocket::Address &)obj;

  if (version < other.version)
    return LessThan;
  if (version > other.version)
    return GreaterThan;

#if P_HAS_IPV6
  if (version == 6) {
    int result = memcmp(&v.six, &other.v.six, sizeof(v.six));
    if (result < 0)
      return LessThan;
    if (result > 0)
      return GreaterThan;
    return EqualTo;
  }
#endif

  if ((DWORD)*this < other)
    return LessThan;
  if ((DWORD)*this > other)
    return GreaterThan;
  return EqualTo;
}

#if P_HAS_IPV6
bool PIPSocket::Address::operator*=(const PIPSocket::Address & addr) const
{
  if (version == addr.version)
    return operator==(addr);

  if (this->GetVersion() == 6 && this->IsV4Mapped()) 
    return PIPSocket::Address((*this)[12], (*this)[13], (*this)[14], (*this)[15]) == addr;
  else if (addr.GetVersion() == 6 && addr.IsV4Mapped()) 
    return *this == PIPSocket::Address(addr[12], addr[13], addr[14], addr[15]);
  return PFalse;
}

bool PIPSocket::Address::operator==(in6_addr & addr) const
{
  PIPSocket::Address a(addr);
  return Compare(a) == EqualTo;
}
#endif


bool PIPSocket::Address::operator==(in_addr & addr) const
{
  PIPSocket::Address a(addr);
  return Compare(a) == EqualTo;
}


bool PIPSocket::Address::operator==(DWORD dw) const
{
  if (dw == 0)
    return !IsValid();

  if (version == 4)
    return (DWORD)*this == dw;

  return *this == Address(dw);
}


PIPSocket::Address & PIPSocket::Address::operator=(const PString & dotNotation)
{
  FromString(dotNotation);
  return *this;
}


PString PIPSocket::Address::AsString(bool IPV6_PARAM(bracketIPv6)) const
{
#if defined(P_VXWORKS)
  char ipStorage[INET_ADDR_LEN];
  inet_ntoa_b(v.four, ipStorage);
  return ipStorage;    
#else
# if defined(P_HAS_IPV6)
  if (version == 6) {
    PString str;
    Psockaddr sa(*this, 0);
    PAssertOS(getnameinfo(sa, sa.GetSize(), str.GetPointer(1024), 1024, NULL, 0, NI_NUMERICHOST) == 0);
    PINDEX percent = str.Find('%'); // used for scoped address e.g. fe80::1%ne0, (ne0=network interface 0)
    if (percent != P_MAX_INDEX)
      str[percent] = '\0';
    str.MakeMinimumSize();
    if (bracketIPv6)
      return '[' + str + ']';
    return str;
  }
#endif // P_HAS_IPV6
# if defined(P_HAS_INET_NTOP)
  PString str;
  if (inet_ntop(AF_INET, (const void *)&v.four, str.GetPointer(INET_ADDRSTRLEN), INET_ADDRSTRLEN) == NULL)
    return PString::Empty();
  str.MakeMinimumSize();
  return str;
# else
  static PCriticalSection x;
  PWaitAndSignal m(x);
  return inet_ntoa(v.four);
#endif // P_HAS_INET_NTOP
#endif // P_VXWORKS
}


PBoolean PIPSocket::Address::FromString(const PString & ipAndInterface)
{
  version = 0;
  memset(&v, 0, sizeof(v));

  PINDEX percent = ipAndInterface.Find('%');
  PString dotNotation = ipAndInterface.Left(percent);
  if (!dotNotation.IsEmpty()) {
#if P_HAS_IPV6

    // Find out if string is in brackets [], as in ipv6 address
    PINDEX lbracket = dotNotation.Find('[');
    PINDEX rbracket = dotNotation.Find(']', lbracket);
    if (lbracket != P_MAX_INDEX && rbracket != P_MAX_INDEX)
      dotNotation = dotNotation(lbracket+1, rbracket-1);

    struct addrinfo *res = NULL;
    struct addrinfo hints = { AI_NUMERICHOST, PF_UNSPEC }; // Could be IPv4: x.x.x.x or IPv6: x:x:x:x::x

    if (getaddrinfo((const char *)dotNotation, NULL , &hints, &res) == 0) {
      if (res->ai_family == PF_INET6) {
        // IPv6 addr
        version = 6;
        struct sockaddr_in6 * addr_in6 = (struct sockaddr_in6 *)res->ai_addr;
        v.six = addr_in6->sin6_addr;
      } else {
        // IPv4 addr
        version = 4;
        struct sockaddr_in * addr_in = (struct sockaddr_in *)res->ai_addr;
        v.four = addr_in->sin_addr;
      }
      if (res != NULL)
        freeaddrinfo(res);
      return IsValid();
    }

#else //P_HAS_IPV6

    DWORD iaddr;
    if (dotNotation.FindSpan("0123456789.") == P_MAX_INDEX &&
                      (iaddr = ::inet_addr((const char *)dotNotation)) != (DWORD)INADDR_NONE) {
      version = 4;
      v.four.s_addr = iaddr;
      return true;
    }

#endif
  }

  if (percent == P_MAX_INDEX)
    return false;

  PString iface = ipAndInterface.Mid(percent+1);
  if (iface.IsEmpty())
    return false;

  PIPSocket::InterfaceTable interfaceTable;
  if (!PIPSocket::GetInterfaceTable(interfaceTable))
    return false;

  for (PINDEX i = 0; i < interfaceTable.GetSize(); i++) {
    if (interfaceTable[i].GetName().NumCompare(iface) == EqualTo) {
      *this = interfaceTable[i].GetAddress();
      return true;
    }
  }

  return false;

}


PIPSocket::Address::operator PString() const
{
  return AsString();
}


PIPSocket::Address::operator in_addr() const
{
  if (version != 4)
    return inaddr_empty;

  return v.four;
}


#if P_HAS_IPV6
PIPSocket::Address::operator in6_addr() const
{
  if (version != 6)
    return any6.v.six;

  return v.six;
}
#endif


BYTE PIPSocket::Address::operator[](PINDEX idx) const
{
  PASSERTINDEX(idx);
#if P_HAS_IPV6
  if (version == 6) {
    PAssert(idx <= 15, PInvalidParameter);
    return v.six.s6_addr[idx];
  }
#endif

  PAssert(idx <= 3, PInvalidParameter);
  return ((BYTE *)&v.four)[idx];
}


ostream & operator<<(ostream & s, const PIPSocket::Address & a)
{
  return s << a.AsString();
}

istream & operator>>(istream & s, PIPSocket::Address & a)
{
/// Not IPv6 ready !!!!!!!!!!!!!
  char dot1, dot2, dot3;
  unsigned b1, b2, b3, b4;
  s >> b1;
  if (!s.fail()) {
    if (s.peek() != '.')
      a = htonl(b1);
    else {
      s >> dot1 >> b2 >> dot2 >> b3 >> dot3 >> b4;
      if (!s.fail() && dot1 == '.' && dot2 == '.' && dot3 == '.')
        a = PIPSocket::Address((BYTE)b1, (BYTE)b2, (BYTE)b3, (BYTE)b4);
    }
  }
  return s;
}


PINDEX PIPSocket::Address::GetSize() const
{
  switch (version) {
#if P_HAS_IPV6
    case 6 :
      return 16;
#endif

    case 4 :
      return 4;
  }

  return 0;
}


PBoolean PIPSocket::Address::IsValid() const
{
  switch (version) {
#if P_HAS_IPV6
    case 6 :
      return memcmp(&v.six, &any6.v.six, sizeof(v.six)) != 0;
#endif

    case 4 :
      return (DWORD)*this != INADDR_ANY;
  }
  return PFalse;
}


PBoolean PIPSocket::Address::IsLoopback() const
{
#if P_HAS_IPV6
  if (version == 6)
    return IN6_IS_ADDR_LOOPBACK(&v.six);
#endif
  return Byte1() == 127;
}


PBoolean PIPSocket::Address::IsBroadcast() const
{
#if P_HAS_IPV6
  if (version == 6) // In IPv6, no broadcast exist. Only multicast
    return *this == broadcast6; // multicast address as as substitute
#endif

  return *this == broadcast4;
}


PBoolean PIPSocket::Address::IsMulticast() const
{
#if P_HAS_IPV6
  if (version == 6)
    return IN6_IS_ADDR_MULTICAST(&v.six);
#endif

  return IN_MULTICAST(ntohl(v.four.s_addr));
}


PBoolean PIPSocket::Address::IsRFC1918() const 
{ 
#if P_HAS_IPV6
  if (version == 6) {
    if (IN6_IS_ADDR_LINKLOCAL(&v.six) || IN6_IS_ADDR_SITELOCAL(&v.six))
      return PTrue;
    if (IsV4Mapped())
      return PIPSocket::Address((*this)[12], (*this)[13], (*this)[14], (*this)[15]).IsRFC1918();
  }
#endif
  return (Byte1() == 10)
          ||
          (
            (Byte1() == 172)
            &&
            (Byte2() >= 16) && (Byte2() <= 31)
          )
          ||
          (
            (Byte1() == 192) 
            &&
            (Byte2() == 168)
          );
}

PIPSocket::InterfaceEntry::InterfaceEntry()
  : m_ipAddress(0, NULL)
  , m_netMask(0, NULL)
{
}

PIPSocket::InterfaceEntry::InterfaceEntry(const PString & name,
                                          const Address & address,
                                          const Address & mask,
                                          const PString & macAddress)
  : m_name(name.Trim())
  , m_ipAddress(address)
  , m_netMask(mask)
  , m_macAddress(macAddress)
{
  SanitiseName(m_name);
}


void PIPSocket::InterfaceEntry::SanitiseName(PString & name)
{
  /* HACK!!
     At various points in PTLib (and OPAL) the interface name is used in
     situations where there can be confusion in parsing. For example a URL can
     be http:://[::%interface]:2345 and a ] in interface name blows it to
     pieces. Similarly, "ip:port" style description which can be used a LOT,
     will also fail. At this late stage it is too hard to change all the other
     places, so we hack the fairly rare cases by translating those special
     characters.
   */ 
  name.Replace('[', '{', true);
  name.Replace(']', '}', true);
  name.Replace(':', ';', true);
}


void PIPSocket::InterfaceEntry::PrintOn(ostream & strm) const
{
  strm << m_ipAddress;
  if (!m_macAddress)
    strm << " <" << m_macAddress << '>';
  if (!m_name)
    strm << " (" << m_name << ')';
}


#ifdef __NUCLEUS_NET__
PBoolean PIPSocket::GetInterfaceTable(InterfaceTable & table)
{
    InterfaceEntry *IE;
    list<IPInterface>::iterator i;
    for(i=Route4Configuration->Getm_IPInterfaceList().begin();
            i!=Route4Configuration->Getm_IPInterfaceList().end();
            i++)
    {
        char ma[6];
        for(int j=0; j<6; j++) ma[j]=(*i).Getm_macaddr(j);
        IE = new InterfaceEntry((*i).Getm_name().c_str(), (*i).Getm_ipaddr(), ma );
        if(!IE) return false;
        table.Append(IE);
    }
    return true;
}
#endif

PBoolean PIPSocket::GetNetworkInterface(PIPSocket::Address & addr)
{
  PIPSocket::InterfaceTable interfaceTable;
  if (PIPSocket::GetInterfaceTable(interfaceTable)) {
    PINDEX i;
    for (i = 0; i < interfaceTable.GetSize(); ++i) {
      PIPSocket::Address localAddr = interfaceTable[i].GetAddress();
      if (!localAddr.IsLoopback() && (!localAddr.IsRFC1918() || !addr.IsRFC1918()))
        addr = localAddr;
    }
  }
  return addr.IsValid();
}


PIPSocket::Address PIPSocket::GetRouteInterfaceAddress(PIPSocket::Address remoteAddress)
{
  PIPSocket::InterfaceTable hostInterfaceTable;
  PIPSocket::GetInterfaceTable(hostInterfaceTable);

  PIPSocket::RouteTable hostRouteTable;
  PIPSocket::GetRouteTable(hostRouteTable);

  if (hostInterfaceTable.IsEmpty())
    return PIPSocket::GetDefaultIpAny();

  for (PINDEX IfaceIdx = 0; IfaceIdx < hostInterfaceTable.GetSize(); IfaceIdx++) {
    if (remoteAddress == hostInterfaceTable[IfaceIdx].GetAddress()) {
      PTRACE(5, "Socket\tRoute packet for " << remoteAddress
             << " over interface " << hostInterfaceTable[IfaceIdx].GetName()
             << "[" << hostInterfaceTable[IfaceIdx].GetAddress() << "]");
      return hostInterfaceTable[IfaceIdx].GetAddress();
    }
  }

  PIPSocket::RouteEntry * route = NULL;
  for (PINDEX routeIdx = 0; routeIdx < hostRouteTable.GetSize(); routeIdx++) {
    PIPSocket::RouteEntry & routeEntry = hostRouteTable[routeIdx];

    DWORD network = (DWORD) routeEntry.GetNetwork();
    DWORD mask = (DWORD) routeEntry.GetNetMask();

    if (((DWORD)remoteAddress & mask) == network) {
      if (route == NULL)
        route = &routeEntry;
      else if ((DWORD)routeEntry.GetNetMask() > (DWORD)route->GetNetMask())
        route = &routeEntry;
    }
  }

  if (route != NULL) {
    for (PINDEX IfaceIdx = 0; IfaceIdx < hostInterfaceTable.GetSize(); IfaceIdx++) {
      if (route->GetInterface() == hostInterfaceTable[IfaceIdx].GetName()) {
        PTRACE(5, "Socket\tRoute packet for " << remoteAddress
               << " over interface " << hostInterfaceTable[IfaceIdx].GetName()
               << "[" << hostInterfaceTable[IfaceIdx].GetAddress() << "]");
        return hostInterfaceTable[IfaceIdx].GetAddress();
      }
    }
  }

  return PIPSocket::GetDefaultIpAny();
}

//////////////////////////////////////////////////////////////////////////////
// PTCPSocket

PTCPSocket::PTCPSocket(WORD newPort)
{
  SetPort(newPort);
}


PTCPSocket::PTCPSocket(const PString & service)
{
  SetPort(service);
}


PTCPSocket::PTCPSocket(const PString & address, WORD newPort)
{
  SetPort(newPort);
  Connect(address);
}


PTCPSocket::PTCPSocket(const PString & address, const PString & service)
{
  SetPort(service);
  Connect(address);
}


PTCPSocket::PTCPSocket(PSocket & socket)
{
  Accept(socket);
}


PTCPSocket::PTCPSocket(PTCPSocket & tcpSocket)
{
  Accept(tcpSocket);
}


PObject * PTCPSocket::Clone() const
{
  return new PTCPSocket(port);
}


// By default IPv4 only adresses
PBoolean PTCPSocket::OpenSocket()
{
  return ConvertOSError(os_handle = os_socket(AF_INET, SOCK_STREAM, 0));
}


// ipAdressFamily should be AF_INET or AF_INET6
PBoolean PTCPSocket::OpenSocket(int ipAdressFamily) 
{
  return ConvertOSError(os_handle = os_socket(ipAdressFamily, SOCK_STREAM, 0));
}


const char * PTCPSocket::GetProtocolName() const
{
  return "tcp";
}


PBoolean PTCPSocket::Write(const void * buf, PINDEX len)
{
  flush();
  PINDEX writeCount = 0;

  while (len > 0) {
    if (!os_sendto(((char *)buf)+writeCount, len, 0, NULL, 0))
      return PFalse;
    writeCount += lastWriteCount;
    len -= lastWriteCount;
  }

  lastWriteCount = writeCount;
  return PTrue;
}


PBoolean PTCPSocket::Listen(unsigned queueSize, WORD newPort, Reusability reuse)
{
#if P_HAS_IPV6
  return Listen(GetDefaultIpAny(), queueSize, newPort, reuse);
#else
  return Listen(INADDR_ANY, queueSize, newPort, reuse);
#endif
}


PBoolean PTCPSocket::Listen(const Address & bindAddr,
                        unsigned queueSize,
                        WORD newPort,
                        Reusability reuse)
{
  if (PIPSocket::Listen(bindAddr, queueSize, newPort, reuse) &&
      ConvertOSError(::listen(os_handle, queueSize)))
    return PTrue;

  os_close();
  return PFalse;
}


PBoolean PTCPSocket::Accept(PSocket & socket)
{
  PAssert(PIsDescendant(&socket, PIPSocket), "Invalid listener socket");

#if P_HAS_IPV6

  Psockaddr sa;
  PINDEX size = sa.GetSize();
  if (!os_accept(socket, sa, &size))
    return PFalse;
    
#else

  sockaddr_in address;
  address.sin_family = AF_INET;
  PINDEX size = sizeof(address);
  if (!os_accept(socket, (struct sockaddr *)&address, &size))
    return PFalse;

#endif

  port = ((PIPSocket &)socket).GetPort();
  
  return PTrue;
}


PBoolean PTCPSocket::WriteOutOfBand(void const * buf, PINDEX len)
{
#ifdef __NUCLEUS_NET__
  PAssertAlways("WriteOutOfBand unavailable on Nucleus Plus");
  //int count = NU_Send(os_handle, (char *)buf, len, 0);
  int count = ::send(os_handle, (const char *)buf, len, 0);
#elif defined(P_VXWORKS)
  int count = ::send(os_handle, (char *)buf, len, MSG_OOB);
#else
  int count = ::send(os_handle, (const char *)buf, len, MSG_OOB);
#endif
  if (count < 0) {
    lastWriteCount = 0;
    return ConvertOSError(count, LastWriteError);
  }
  else {
    lastWriteCount = count;
    return PTrue;
  }
}


void PTCPSocket::OnOutOfBand(const void *, PINDEX)
{
}


//////////////////////////////////////////////////////////////////////////////
// PIPDatagramSocket

PIPDatagramSocket::PIPDatagramSocket()
{
}


PBoolean PIPDatagramSocket::ReadFrom(void * buf, PINDEX len,
                                 Address & addr, WORD & port)
{
  lastReadCount = 0;

#if P_HAS_IPV6

  Psockaddr sa;
  PINDEX size = sa.GetSize();
  bool ok = os_recvfrom(buf, len, 0, sa, &size);
  addr = sa.GetIP();
  port = sa.GetPort();

#else

  sockaddr_in sockAddr;
  PINDEX addrLen = sizeof(sockAddr);
  bool ok = os_recvfrom(buf, len, 0, (struct sockaddr *)&sockAddr, &addrLen);
  addr = sockAddr.sin_addr;
  port = ntohs(sockAddr.sin_port);

#endif

  return ok;
}


PBoolean PIPDatagramSocket::WriteTo(const void * buf, PINDEX len,
                                const Address & addr, WORD port)
{
  lastWriteCount = 0;

  PBoolean broadcast = addr.IsAny() || addr.IsBroadcast();
  if (broadcast) {
#ifdef P_BEOS
    PAssertAlways("Broadcast option under BeOS is not implemented yet");
    return PFalse;
#else
    if (!SetOption(SO_BROADCAST, 1))
      return PFalse;
#endif
  }
  
#ifdef P_MACOSX
  // Mac OS X does not treat 255.255.255.255 as a broadcast address (multihoming / ambiguity issues)
  // and such packets aren't sent to the layer 2 - broadcast address (i.e. ethernet ff:ff:ff:ff:ff:ff)
  // instead: send subnet broadcasts on all interfaces
  
  // TODO: Add IPv6 support
  
  PBoolean ok = PFalse;
  if (broadcast) {
    sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(port);
    
    InterfaceTable interfaces;
    if (GetInterfaceTable(interfaces)) {
      for (int i = 0; i < interfaces.GetSize(); i++) {
        InterfaceEntry & iface = interfaces[i];
        if (iface.GetAddress().IsLoopback()) {
          continue;
        }
        // create network address from address / netmask
        DWORD ifAddr = iface.GetAddress();
        DWORD netmask = iface.GetNetMask();
        DWORD bcastAddr = (ifAddr & netmask) + ~netmask;
        
        sockAddr.sin_addr.s_addr = bcastAddr;
        
        PBoolean result = os_sendto(buf, len, 0, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) != 0;
        
        ok = ok || result;
      }
    }
    
  } else {
    sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr = addr;
    sockAddr.sin_port = htons(port);
    ok = os_sendto(buf, len, 0, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) != 0;
  }
  
#else
  
#if P_HAS_IPV6
  
  Psockaddr sa(broadcast ? Address::GetBroadcast(addr.GetVersion()) : addr, port);
  PBoolean ok = os_sendto(buf, len, 0, sa, sa.GetSize()) != 0;
  
#else
  
  sockaddr_in sockAddr;
  sockAddr.sin_family = AF_INET;
  sockAddr.sin_addr = (broadcast ? Address::GetBroadcast() : addr);
  sockAddr.sin_port = htons(port);
  PBoolean ok = os_sendto(buf, len, 0, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) != 0;
  
#endif // P_HAS_IPV6
  
#endif // P_MACOSX

#ifndef P_BEOS
  if (broadcast)
    SetOption(SO_BROADCAST, 0);
#endif

  return ok && lastWriteCount >= len;
}


//////////////////////////////////////////////////////////////////////////////
// PUDPSocket

PUDPSocket::PUDPSocket(WORD newPort, int iAddressFamily)
{
  sendPort = 0;
  SetPort(newPort);
  OpenSocket(iAddressFamily);
}

PUDPSocket::PUDPSocket(PQoS * qos, WORD newPort, int iAddressFamily)
#if P_HAS_IPV6
  : sendAddress(iAddressFamily == AF_INET ? loopback4 : loopback6),
    lastReceiveAddress(iAddressFamily == AF_INET ? loopback4 : loopback6)
#else
  : sendAddress(loopback4),
    lastReceiveAddress(loopback4)
#endif
{
  if (qos != NULL)
      qosSpec = *qos;
  sendPort = 0;
  SetPort(newPort);
  OpenSocket(iAddressFamily);
}


PUDPSocket::PUDPSocket(const PString & service, PQoS * qos, int iAddressFamily)
#if P_HAS_IPV6
  : sendAddress(iAddressFamily == AF_INET ? loopback4 : loopback6),
    lastReceiveAddress(iAddressFamily == AF_INET ? loopback4 : loopback6)
#else
  : sendAddress(loopback4),
    lastReceiveAddress(loopback4)
#endif
{
  if (qos != NULL)
      qosSpec = *qos;
  sendPort = 0;
  SetPort(service);
  OpenSocket(iAddressFamily);
}


PUDPSocket::PUDPSocket(const PString & address, WORD newPort)
{
  sendPort = 0;
  SetPort(newPort);
  Connect(address);
}


PUDPSocket::PUDPSocket(const PString & address, const PString & service)
{
  sendPort = 0;
  SetPort(service);
  Connect(address);
}


PBoolean PUDPSocket::ModifyQoSSpec(PQoS * qos)
{
  if (qos==NULL)
    return PFalse;

  qosSpec = *qos;
  return PTrue;
}

#if P_QOS
PQoS & PUDPSocket::GetQoSSpec()
{
  return qosSpec;
}
#endif //P_QOS

PBoolean PUDPSocket::ApplyQoS()
{
  char DSCPval = 0;
#ifndef _WIN32_WCE
  if (qosSpec.GetDSCP() < 0 ||
      qosSpec.GetDSCP() > 63) {
    if (qosSpec.GetServiceType() == SERVICETYPE_PNOTDEFINED)
      return PTrue;
    else {
      switch (qosSpec.GetServiceType()) {
        case SERVICETYPE_GUARANTEED:
          DSCPval = PQoS::guaranteedDSCP;
          break;
        case SERVICETYPE_CONTROLLEDLOAD:
          DSCPval = PQoS::controlledLoadDSCP;
          break;
        case SERVICETYPE_BESTEFFORT:
        default:
          DSCPval = PQoS::bestEffortDSCP;
          break;
      }
    }
  }
  else
    DSCPval = (char)qosSpec.GetDSCP();
#else
  DSCPval = 0x38;
  disableGQoS = PFalse;
#endif

#ifdef _WIN32
#if P_QOS
  if (disableGQoS)
    return PFalse;

#ifndef _WIN32_WCE
  PBoolean usesetsockopt = PFalse;

  OSVERSIONINFO versInfo;
  ZeroMemory(&versInfo,sizeof(OSVERSIONINFO));
  versInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  if (!(GetVersionEx(&versInfo)))
    usesetsockopt = PTrue;
  else {
    if (versInfo.dwMajorVersion < 5)
      usesetsockopt = PTrue;

    if (disableGQoS)
          return PFalse;

    PBoolean usesetsockopt = PFalse;

    if (versInfo.dwMajorVersion == 5 &&
        versInfo.dwMinorVersion == 0)
      usesetsockopt = PTrue;         //Windows 2000 does not always support QOS_DESTADDR
  }
#else
  PBoolean usesetsockopt = PTrue;
#endif

  PBoolean retval = PFalse;
  if (!usesetsockopt && sendAddress.IsValid() && sendPort != 0) {
    sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(sendPort);
    sa.sin_addr = sendAddress;
    memset(sa.sin_zero,0,8);

    char * inBuf = new char[2048];
    memset(inBuf,0,2048);
    DWORD bufLen = 0;
    PWinQoS wqos(qosSpec, (struct sockaddr *)(&sa), inBuf, bufLen);

    DWORD dummy = 0;
    int irval = WSAIoctl(os_handle, SIO_SET_QOS, inBuf, bufLen, NULL, 0, &dummy, NULL, NULL);

    delete[] inBuf;

    return irval == 0;
  }

  if (!usesetsockopt)
    return retval;

#endif  // P_QOS
#endif  // _WIN32

  unsigned int setDSCP = DSCPval<<2;

  int rv = 0;
  unsigned int curval = 0;
  socklen_t cursize = sizeof(curval);
  rv = ::getsockopt(os_handle,IPPROTO_IP, IP_TOS, (char *)(&curval), &cursize);
  if (curval == setDSCP)
    return PTrue;    //Required DSCP already set


  rv = ::setsockopt(os_handle, IPPROTO_IP, IP_TOS, (char *)&setDSCP, sizeof(setDSCP));

  if (rv != 0) {
    int err;
#ifdef _WIN32
    err = WSAGetLastError();
#else
    err = errno;
#endif
    PTRACE(1,"QOS\tsetsockopt failed with code " << err);
    return PFalse;
  }
    
  return PTrue;
}

PBoolean PUDPSocket::OpenSocketGQOS(int af, int type, int proto)
{
#if defined(_WIN32) && defined(P_QOS)
    
  //Try to find a QOS-enabled protocol
  DWORD bufferSize = 0;
  DWORD numProtocols = WSAEnumProtocols(proto != 0 ? &proto : NULL, NULL, &bufferSize);
  if (!ConvertOSError(numProtocols) && WSAGetLastError() != WSAENOBUFS) 
    return false;

  LPWSAPROTOCOL_INFO installedProtocols = (LPWSAPROTOCOL_INFO)(new BYTE[bufferSize]);
  numProtocols = WSAEnumProtocols(proto != 0 ? &proto : NULL, installedProtocols, &bufferSize);
  if (!ConvertOSError(numProtocols)) {
    delete[] installedProtocols;
    return false;
  }

  LPWSAPROTOCOL_INFO qosProtocol = installedProtocols;
  for (DWORD i = 0; i < numProtocols; qosProtocol++, i++) {
    if ((qosProtocol->dwServiceFlags1 & XP1_QOS_SUPPORTED) &&
        (qosProtocol->iSocketType == type) &&
        (qosProtocol->iAddressFamily == af)) {
      os_handle = WSASocket(af, type, proto, qosProtocol, 0, WSA_FLAG_OVERLAPPED);
      break;
    }
  }

  delete[] installedProtocols;

  if (!IsOpen())
    os_handle = WSASocket(af, type, proto, NULL, 0, WSA_FLAG_OVERLAPPED);

  return ConvertOSError(os_handle);

#else
  return ConvertOSError(os_handle = os_socket(af, type, proto));
#endif
}

#ifdef _WIN32
#if P_QOS

#define COULD_HAVE_QOS

static PBoolean CheckOSVersionFor(DWORD major, DWORD minor)
{
  OSVERSIONINFO versInfo;
  ZeroMemory(&versInfo,sizeof(OSVERSIONINFO));
  versInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  if (GetVersionEx(&versInfo)) {
    if (versInfo.dwMajorVersion > major ||
       (versInfo.dwMajorVersion == major && versInfo.dwMinorVersion >= minor))
      return PTrue;
  }
  return PFalse;
}

#endif // P_QOS
#endif // _WIN32

PBoolean PUDPSocket::OpenSocket()
{
#ifdef COULD_HAVE_QOS
  if (CheckOSVersionFor(5,1)) 
    return OpenSocketGQOS(AF_INET, SOCK_DGRAM, 0);
#endif

  return ConvertOSError(os_handle = os_socket(AF_INET,SOCK_DGRAM, 0));
}

PBoolean PUDPSocket::OpenSocket(int ipAdressFamily)
{
#ifdef COULD_HAVE_QOS
  if (CheckOSVersionFor(5,1)) 
    return OpenSocketGQOS(ipAdressFamily, SOCK_DGRAM, 0);
#endif

  return ConvertOSError(os_handle = os_socket(ipAdressFamily,SOCK_DGRAM, 0));
}

const char * PUDPSocket::GetProtocolName() const
{
  return "udp";
}


PBoolean PUDPSocket::Connect(const PString & address)
{
  sendPort = 0;
  return PIPDatagramSocket::Connect(address);
}


PBoolean PUDPSocket::Read(void * buf, PINDEX len)
{
  return PIPDatagramSocket::ReadFrom(buf, len, lastReceiveAddress, lastReceivePort);
}


PBoolean PUDPSocket::Write(const void * buf, PINDEX len)
{
  if (sendPort == 0)
    return PIPDatagramSocket::Write(buf, len);
  else
    return PIPDatagramSocket::WriteTo(buf, len, sendAddress, sendPort);
}


void PUDPSocket::SetSendAddress(const Address & newAddress, WORD newPort)
{
  sendAddress = newAddress;
  sendPort    = newPort;
  ApplyQoS();
}


void PUDPSocket::GetSendAddress(Address & address, WORD & port) const
{
  address = sendAddress;
  port    = sendPort;
}


PString PUDPSocket::GetSendAddress() const
{
  return sendAddress.AsString(true) + psprintf(":%u", sendPort);
}


void PUDPSocket::GetLastReceiveAddress(Address & address, WORD & port) const
{
  address = lastReceiveAddress;
  port    = lastReceivePort;
}


PString PUDPSocket::GetLastReceiveAddress() const
{
  return lastReceiveAddress.AsString(true) + psprintf(":%u", lastReceivePort);
}


PBoolean PUDPSocket::IsAlternateAddress(const Address &, WORD)
{
  return false;
}

PBoolean PUDPSocket::DoPseudoRead(int & /*selectStatus*/)
{
   return false;
}

//////////////////////////////////////////////////////////////////////////////

PBoolean PICMPSocket::OpenSocket(int)
{
  return PFalse;
}

//////////////////////////////////////////////////////////////////////////////

PBoolean PIPSocketAddressAndPort::Parse(const PString & str, WORD port, char separator)
{
  if (separator != '\0')
    m_separator = separator;

  PINDEX pos = str.Find(m_separator);
  if (pos != P_MAX_INDEX)
    port = (WORD)str.Mid(pos+1).AsUnsigned();

  if (port != 0)
    m_port = port;

  return PIPSocket::GetHostAddress(str.Left(pos), m_address) && m_port != 0;
}


void PIPSocketAddressAndPort::SetAddress(const PIPSocket::Address & addr, WORD port)
{
  m_address = addr;
  if (port != 0)
    m_port = port;
}


// End Of File ///////////////////////////////////////////////////////////////
