/*
 * tlib.cxx
 *
 * Miscelaneous class implementation
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

#define _OSUTIL_CXX

//#define SIGNALS_DEBUG

#pragma implementation "args.h"
#pragma implementation "pprocess.h"
#pragma implementation "thread.h"
#pragma implementation "semaphor.h"
#pragma implementation "mutex.h"
#pragma implementation "critsec.h"
#pragma implementation "psync.h"
#pragma implementation "syncpoint.h"
#pragma implementation "syncthrd.h"

#include "ptlib.h"
#include <ptlib/pprocess.h>

#ifdef P_VXWORKS
#include <sys/times.h>
#include <time.h>
#include <hostLib.h>
#include <remLib.h>
#include <taskLib.h>
#include <intLib.h>
#else
#include <sys/time.h>
#include <pwd.h>
#include <grp.h>
#endif // P_VXWORKS
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#if defined(P_LINUX) || defined(P_GNU_HURD)
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/mman.h>
#endif

#if defined(P_LINUX) || defined(P_SUN4) || defined(P_SOLARIS) || defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_NETBSD) || defined(P_MACOSX) || defined(P_MACOS) || defined (P_AIX) || defined(P_BEOS) || defined(P_IRIX) || defined(P_QNX) || defined(P_GNU_HURD)
#include <sys/utsname.h>
#define  HAS_UNAME
#elif defined(P_RTEMS)
extern "C" {
#include <sys/utsname.h>
}
#define  HAS_UNAME
#endif

#include "uerror.h"

#if defined(P_HPUX9)
#define  SELECT(p1,p2,p3,p4,p5)    select(p1,(int *)(p2),(int *)(p3),(int *)(p4),p5)
#else
#define  SELECT(p1,p2,p3,p4,p5)    select(p1,p2,p3,p4,p5)
#endif

#if defined(P_SUN4)
extern "C" void bzero(void *, int);
extern "C" int select(int width,
      fd_set *readfds,
      fd_set *writefds,
      fd_set *exceptfds,
      struct timeval *timeout);
#endif

#ifdef P_BEOS
#include "OS.h"
#endif


PString PProcess::GetOSClass()
{
#ifndef P_BEOS
  return PString("Unix");
#elif defined P_VXWORKS
  return PString("VxWorks");
#else
  return PString("Be Inc.");
#endif
}

PString PProcess::GetOSName()
{
#if defined(HAS_UNAME)
  struct utsname info;
  uname(&info);
#ifdef P_SOLARIS
  return PString(info.sysname) & info.release;
#else
  return PString(info.sysname);
#endif
#elif defined(P_VXWORKS)
  return PString::Empty();
#else
#warning No GetOSName specified
  return PString("Unknown");
#endif
}

PString PProcess::GetOSHardware()
{
#if defined(HAS_UNAME)
  struct utsname info;
  uname(&info);
  return PString(info.machine);
#elif defined(P_VXWORKS)
  return PString(sysModel());
#else
#warning No GetOSHardware specified
  return PString("unknown");
#endif
}

PString PProcess::GetOSVersion()
{
#if defined(HAS_UNAME)
  struct utsname info;
  uname(&info);
#ifdef P_SOLARIS
  return PString(info.version);
#else
  return PString(info.release);
#endif
#elif defined(P_VXWORKS)
  return PString(sysBspRev());
#else
#warning No GetOSVersion specified
  return PString("?.?");
#endif
}

bool PProcess::IsOSVersion(unsigned major, unsigned minor, unsigned build)
{
#if defined(HAS_UNAME)
  struct utsname info;
  uname(&info);
  unsigned maj, min, bld;
  sscanf(
#ifdef P_SOLARIS
         info.version
#else
         info.release
#endif
         , "%u.%u.%u", &maj, &min, &bld);
  if (maj < major)
    return false;
  if (maj > major)
    return true;

  if (min < minor)
    return false;
  if (min > minor)
    return true;

  return bld >= build;
#elif defined(P_VXWORKS)
  return sysBspRev() >= major;
#else
  return true;
#endif
}


PDirectory PProcess::GetOSConfigDir()
{
#ifdef P_VXWORKS
  return "./";
#else
  return "/etc";
#endif // P_VXWORKS
}

PDirectory PProcess::PXGetHomeDir ()

{
#ifdef P_VXWORKS
  return "./";
#else
  PString dest;
  char *ptr;
  struct passwd *pw = NULL;

#if defined(P_PTHREADS) && !defined(P_THREAD_SAFE_CLIB)
  struct passwd pwd;
  char buffer[1024];
#if defined (P_LINUX) || defined(P_AIX) || defined(P_IRIX) || (__GNUC__>=3 && defined(P_SOLARIS)) || defined(P_RTEMS) || defined(P_GNU_HURD)
  ::getpwuid_r(geteuid(), &pwd,
               buffer, 1024,
               &pw);
#else
  pw = ::getpwuid_r(geteuid(), &pwd, buffer, 1024);
#endif
#else
  pw = ::getpwuid(geteuid());
#endif

  if (pw != NULL && pw->pw_dir != NULL) 
    dest = pw->pw_dir;
  else if ((ptr = getenv ("HOME")) != NULL) 
    dest = ptr;
  else 
    dest = ".";

  if (dest.GetLength() > 0 && dest[dest.GetLength()-1] != '/')
    dest += "/";

  return dest;
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
// PProcess
//
// Return the effective user name of the process, eg "root" etc.

PString PProcess::GetUserName() const

{
#ifdef P_VXWORKS

  char pnamebuf[1024];
  int len = 1024;
  STATUS gethostresult;
  gethostresult =::gethostname(pnamebuf,len);  
  if (gethostresult == OK)
    return PString(pnamebuf,len);
  else
    return PString("VxWorks");

#else

#if defined(P_PTHREADS) && !defined(P_THREAD_SAFE_CLIB)
  struct passwd pwd;
  char buffer[1024];
  struct passwd * pw = NULL;
#if defined (P_LINUX) || defined (P_AIX) || defined(P_IRIX) || (__GNUC__>=3 && defined(P_SOLARIS)) || defined(P_RTEMS) || defined(P_GNU_HURD)
  ::getpwuid_r(geteuid(), &pwd, buffer, 1024, &pw);
#else
  pw = ::getpwuid_r(geteuid(), &pwd, buffer, 1024);
#endif
#else
  struct passwd * pw = ::getpwuid(geteuid());
#endif

  char * ptr;
  if (pw != NULL && pw->pw_name != NULL)
    return PString(pw->pw_name);
  else if ((ptr = getenv("USER")) != NULL)
    return PString(ptr);
  else
    return PString("user");
#endif // P_VXWORKS
}


PBoolean PProcess::SetUserName(const PString & username, PBoolean permanent)
{
#ifdef P_VXWORKS
  PAssertAlways("PProcess::SetUserName - not implemented for VxWorks");
  return PFalse;
#else
  if (username.IsEmpty())
    return seteuid(getuid()) != -1;

  int uid = -1;

  if (username[0] == '#') {
    PString s = username.Mid(1);
    if (s.FindSpan("1234567890") == P_MAX_INDEX)
      uid = s.AsInteger();
  }
  else {
#if defined(P_PTHREADS) && !defined(P_THREAD_SAFE_CLIB)
    struct passwd pwd;
    char buffer[1024];
    struct passwd * pw = NULL;
#if defined (P_LINUX) || defined (P_AIX) || defined(P_IRIX) || (__GNUC__>=3 && defined(P_SOLARIS)) || defined(P_RTEMS) || defined(P_GNU_HURD)
    ::getpwnam_r(username, &pwd, buffer, 1024, &pw);
#else
    pw = ::getpwnam_r(username, &pwd, buffer, 1024);
#endif
#else
    struct passwd * pw = ::getpwnam(username);
#endif

    if (pw != NULL && pw->pw_name != NULL)
      uid = pw->pw_uid;
    else {
      if (username.FindSpan("1234567890") == P_MAX_INDEX)
       uid = username.AsInteger();
    }
  }

  if (uid < 0)
    return PFalse;

  if (permanent)
    return setuid(uid) != -1;
    
  return seteuid(uid) != -1;
#endif // P_VXWORKS
}


///////////////////////////////////////////////////////////////////////////////
//
// PProcess
//
// Return the effective group name of the process, eg "wheel" etc.

PString PProcess::GetGroupName() const

{
#ifdef P_VXWORKS

  return PString("VxWorks");

#else

#if defined(P_PTHREADS) && !defined(P_THREAD_SAFE_CLIB)
  struct group grp;
  char buffer[1024];
  struct group * gr = NULL;
#if defined (P_LINUX) || defined (P_AIX) || defined(P_IRIX) || (__GNUC__>=3 && defined(P_SOLARIS)) || defined(P_RTEMS) || defined(P_GNU_HURD)
  ::getgrgid_r(getegid(), &grp, buffer, 1024, &gr);
#else
  gr = ::getgrgid_r(getegid(), &grp, buffer, 1024);
#endif
#else
  struct group * gr = ::getgrgid(getegid());
#endif

  char * ptr;
  if (gr != NULL && gr->gr_name != NULL)
    return PString(gr->gr_name);
  else if ((ptr = getenv("GROUP")) != NULL)
    return PString(ptr);
  else
    return PString("group");
#endif // P_VXWORKS
}


PBoolean PProcess::SetGroupName(const PString & groupname, PBoolean permanent)
{
#ifdef P_VXWORKS
  PAssertAlways("PProcess::SetGroupName - not implemented for VxWorks");
  return PFalse;
#else
  if (groupname.IsEmpty())
    return setegid(getgid()) != -1;

  int gid = -1;

  if (groupname[0] == '#') {
    PString s = groupname.Mid(1);
    if (s.FindSpan("1234567890") == P_MAX_INDEX)
      gid = s.AsInteger();
  }
  else {
#if defined(P_PTHREADS) && !defined(P_THREAD_SAFE_CLIB)
    struct group grp;
    char buffer[1024];
    struct group * gr = NULL;
#if defined (P_LINUX) || defined (P_AIX) || defined(P_IRIX) || (__GNUC__>=3 && defined(P_SOLARIS)) || defined(P_RTEMS) || defined(P_GNU_HURD)
    ::getgrnam_r(groupname, &grp, buffer, 1024, &gr);
#else
    gr = ::getgrnam_r(groupname, &grp, buffer, 1024);
#endif
#else
    struct group * gr = ::getgrnam(groupname);
#endif

    if (gr != NULL && gr->gr_name != NULL)
      gid = gr->gr_gid;
    else {
      if (groupname.FindSpan("1234567890") == P_MAX_INDEX)
       gid = groupname.AsInteger();
    }
  }

  if (gid < 0)
    return PFalse;

  if (permanent)
    return setgid(gid) != -1;
    
  return setegid(gid) != -1;
#endif // P_VXWORKS
}

extern PProcess * PProcessInstance;;

PString PX_GetThreadName(pthread_t id)
{
  if (PProcessInstance != NULL) {
    PWaitAndSignal m(PProcessInstance->m_activeThreadMutex);
    PThread * thread = PProcessInstance->m_activeThreads[id];
    return thread->GetThreadName();
  }
  return psprintf("%08x", id);
}

void PProcess::PXShowSystemWarning(PINDEX num)
{
  PXShowSystemWarning(num, "");
}

void PProcess::PXShowSystemWarning(PINDEX num, const PString & str)
{
  PProcess::Current()._PXShowSystemWarning(num, str);
}

void PProcess::_PXShowSystemWarning(PINDEX code, const PString & str)
{
  PError << "PWLib " << GetOSClass() << " error #" << code << '-' << str << endl;
}

void PXSignalHandler(int sig)
{
#ifdef SIGNALS_DEBUG
  fprintf(stderr,"\nSIGNAL<%u>\n",sig);
#endif

  PProcess & process = PProcess::Current();
  process.pxSignals |= 1 << sig;
  process.PXOnAsyncSignal(sig);
#if defined(P_MAC_MPTHREADS)
  process.SignalTimerChange();
#elif defined(P_PTHREADS)
  // Inform house keeping thread we have a signal to be processed
  process.SignalTimerChange();
#endif
  signal(sig, PXSignalHandler);
}

void PProcess::PXCheckSignals()
{
  if (pxSignals == 0)
    return;

#ifdef SIGNALS_DEBUG
  fprintf(stderr,"\nCHKSIG<%x>\n",pxSignals);
#endif

  for (int sig = 0; sig < 32; sig++) {
    int bit = 1 << sig;
    if ((pxSignals&bit) != 0) {
      pxSignals &= ~bit;
      PXOnSignal(sig);
    }
  }
}


void SetSignals(void (*handler)(int))
{
#ifdef SIGNALS_DEBUG
  fprintf(stderr,"\nSETSIG<%x>\n",(INT)handler);
#endif

  if (handler == NULL)
    handler = SIG_DFL;

#ifdef SIGHUP
  signal(SIGHUP, handler);
#endif
#ifdef SIGINT
  signal(SIGINT, handler);
#endif
#ifdef SIGUSR1
  signal(SIGUSR1, handler);
#endif
#ifdef SIGUSR2
  signal(SIGUSR2, handler);
#endif
#ifdef SIGPIPE
  signal(SIGPIPE, handler);
#endif
#ifdef SIGTERM
  signal(SIGTERM, handler);
#endif
#ifdef SIGWINCH
  signal(SIGWINCH, handler);
#endif
#ifdef SIGPROF
  signal(SIGPROF, handler);
#endif
}


void PProcess::PXOnAsyncSignal(int sig)
{
#ifdef SIGNALS_DEBUG
  fprintf(stderr,"\nASYNCSIG<%u>\n",sig);
#endif

  switch (sig) {
    case SIGINT:
    case SIGTERM:
      if (OnInterrupt(sig == SIGTERM))
        break;
      // Do next case

    case SIGHUP:
      raise(SIGKILL);
      break;
    default:
      return;
  }
}

void PProcess::PXOnSignal(int sig)
{
#ifdef _DEBUG
#ifdef SIGNALS_DEBUG
  fprintf(stderr,"\nSYNCSIG<%u>\n",sig);
#endif
  if (sig == 28) {
#if PMEMORY_CHECK
    PBoolean oldIgnore = PMemoryHeap::SetIgnoreAllocations(PTrue);
    static PMemoryHeap::State state;
    PMemoryHeap::GetState(state);
#endif
    PStringStream strm;
    m_activeThreadMutex.Wait();
    strm << "===============\n"
         << m_activeThreads.size() << " active threads\n";
    for (ThreadMap::iterator it = m_activeThreads.begin(); it != m_activeThreads.end(); ++it)
      strm << "  " << *it->second << "\n";
#if PMEMORY_CHECK
    strm << "---------------\n";
    PMemoryHeap::DumpObjectsSince(state, strm);
    PMemoryHeap::GetState(state);
#endif
    strm << "===============\n";
    m_activeThreadMutex.Signal();
    fprintf(stderr, "%s", strm.GetPointer());
#if PMEMORY_CHECK
    PMemoryHeap::SetIgnoreAllocations(oldIgnore);
#endif
  }
#endif
}

void PProcess::CommonConstruct()
{
  // Setup signal handlers
  pxSignals = 0;

  if (!m_library)
    SetSignals(&PXSignalHandler);

#if !defined(P_VXWORKS) && !defined(P_RTEMS)
  // initialise the timezone information
  tzset();
#endif

#ifdef P_CONFIG_FILE
  CreateConfigFilesDictionary();
#endif
}

void PProcess::CommonDestruct()
{
#ifdef P_CONFIG_FILE
  delete configFiles;
#endif
  configFiles = NULL;

  if (!m_library)
    SetSignals(NULL);
}

// rtems fixes
#ifdef P_RTEMS

extern "C" {
#include <netinet/in.h>
#include <rtems/rtems_bsdnet.h>
  

int socketpair(int d, int type, int protocol, int sv[2])
{
    static int port_count = IPPORT_USERRESERVED;
    int s;
    int addrlen;
    struct sockaddr_in addr1, addr2;
    static int network_status = 1;
    

    if (network_status>0)
    {
        printf("\"Network\" initializing!\n");
        network_status = rtems_bsdnet_initialize_network();
  if (network_status == 0)
      printf("\"Network\" initialized!\n");
  else
  {
      printf("Error: %s\n", strerror(errno));
      return -1;
  }
    }

    /* prepare sv */
    sv[0]=sv[1]=-1;

    /* make socket */
    s = socket( d, type, protocol);
    if (s<0) 
        return -1;

    memset(&addr1, 0, sizeof addr1);
    addr1.sin_family = d;
    addr1.sin_port = htons(++port_count);
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(s, (struct sockaddr *)&addr1, sizeof addr1) < 0) 
    {
  close(s);
        return -1;
    }
    if (listen(s, 2) < 0 ) 
    {
  close(s);
        return -1;
    }
    
    sv[0] = socket(d, type, protocol);
    if (sv[0] < 0) 
    {
  close(s);
        return -1;
    }
    
    memset(&addr2, 0, sizeof addr2);
    addr2.sin_family = d;
    addr2.sin_port = htons(++port_count);
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(sv[0], (struct sockaddr *)&addr2, sizeof addr2) < 0)
    {
  close(s);
  close(sv[0]);
  sv[0]=-1;
        return -1;
    }
    if (connect(sv[0], (struct sockaddr *)&addr1, sizeof addr1) < 0)
    {
  close(s);
  close(sv[0]);
  sv[0]=-1;
        return -1;
    }
    
    sv[1] = accept(s, (struct sockaddr *)&addr2, &addrlen);
    if (sv[1] < 0)
    {
  close(s);
  close(sv[0]);
  sv[0]=-1;
        return -1;
    }

    close(s);
    return 0;
}

/*
 * Loopback interface
 */
extern int rtems_bsdnet_loopattach(rtems_bsdnet_ifconfig *);
static struct rtems_bsdnet_ifconfig loopback_config = {
    "lo0",                          /* name */
    rtems_bsdnet_loopattach,        /* attach function */

    NULL,                           /* link to next interface */

    "127.0.0.1",                    /* IP address */
    "255.0.0.0",                    /* IP net mask */
};

#include <bsp.h>
#warning Change lines below to match Your system settings

/*
 * Default network interface
 */
static struct rtems_bsdnet_ifconfig netdriver_config = {
    RTEMS_BSP_NETWORK_DRIVER_NAME,          /* name */
    RTEMS_BSP_NETWORK_DRIVER_ATTACH,        /* attach function */

    &loopback_config,                       /* link to next interface */

    "10.0.0.2",                             /* IP address */
    "255.255.255.0",                        /* IP net mask */

    NULL,                                   /* Driver supplies hardware address */
    0                                       /* Use default driver parameters */
};

/*
 * Network configuration
 */
struct rtems_bsdnet_config rtems_bsdnet_config = {
    &netdriver_config,

    NULL,                   /* no bootp function */

    1,                      /* Default network task priority */
    0,                      /* Default mbuf capacity */
    0,                      /* Default mbuf cluster capacity */

    "computer.name",        /* Host name */
    "domain.name",          /* Domain name */
    "10.0.0.1",             /* Gateway */
    "10.0.0.1",             /* Log host */
    {"10.0.0.1" },          /* Name server(s) */
    {"10.0.0.1" },          /* NTP server(s) */

};

#define CONFIGURE_TEST_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_TEST_NEEDS_CLOCK_DRIVER
#define CONFIGURE_TEST_NEEDS_TIMER_DRIVER

#define CONFIGURE_MICROSECONDS_PER_TICK                         1000
#define CONFIGURE_TICKS_PER_TIMESLICE                             50

#define CONFIGURE_MAXIMUM_TASKS          rtems_resource_unlimited(50)
#define CONFIGURE_MAXIMUM_TIMERS         rtems_resource_unlimited(50)
#define CONFIGURE_MAXIMUM_SEMAPHORES     rtems_resource_unlimited(50)
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES rtems_resource_unlimited(50)
#define CONFIGURE_MAXIMUM_MUTEXES        rtems_resource_unlimited(50)

#define CONFIGURE_MAXIMUM_POSIX_THREADS                          500
#define CONFIGURE_MAXIMUM_POSIX_MUTEXES                          500
#define CONFIGURE_MAXIMUM_POSIX_CONDITION_VARIABLES              500
#define CONFIGURE_MAXIMUM_POSIX_KEYS                             500
#define CONFIGURE_MAXIMUM_POSIX_TIMERS                           500
#define CONFIGURE_MAXIMUM_POSIX_QUEUED_SIGNALS                   500
#define CONFIGURE_MAXIMUM_POSIX_MESSAGE_QUEUES                   500
#define CONFIGURE_MAXIMUM_POSIX_SEMAPHORES                       500

#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS                 500
#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM

#define CONFIGURE_POSIX_INIT_THREAD_TABLE
#define CONFIGURE_INIT_TASK_INITIAL_MODES (RTEMS_PREEMPT | RTEMS_TIMESLICE)

#ifdef DEBUG
#define STACK_CHECKER_ON
#endif

void* POSIX_Init(void*);
#define CONFIGURE_INIT
#include <confdefs.h>
}

#endif // P_RTEMS


//////////////////////////////////////////////////////////////////
//
//  Non-PTHREAD based routines
//

#if defined(P_MAC_MPTHREADS)
#include "tlibmpthrd.cxx"
#elif defined(P_PTHREADS)
#include "tlibthrd.cxx"
#elif defined(BE_THREADS)
#include "tlibbe.cxx"
#elif defined(VX_TASKS)
#include "tlibvx.cxx"
#endif
