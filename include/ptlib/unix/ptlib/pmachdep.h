/*
 * machdep.h
 *
 * Unix machine dependencies
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

#ifndef _PMACHDEP_H
#define _PMACHDEP_H

///////////////////////////////////////////////////////////////////////////////
#if defined(P_LINUX)

#include <paths.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/termios.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <dlfcn.h>

#define HAS_IFREQ

#if __GNU_LIBRARY__ < 6
#define P_LINUX_LIB_OLD
typedef int socklen_t;
#endif

#ifdef PPC
typedef size_t socklen_t;
#endif

///////////////////////////////////////////////////////////////////////////////
#elif defined(P_GNU_HURD)

#include <paths.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/termios.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <dlfcn.h>

#define HAS_IFREQ

///////////////////////////////////////////////////////////////////////////////
#elif defined(P_FREEBSD)

#if defined(P_PTHREADS)
#ifndef _THREAD_SAFE
#define _THREAD_SAFE
#endif
#define P_THREAD_SAFE_CLIB

#include <pthread.h>
#endif

#include <paths.h>
#include <errno.h>
#include <dlfcn.h>
#include <termios.h>
#include <sys/fcntl.h>
#include <sys/filio.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/signal.h>
#include <net/if.h>
#include <netinet/tcp.h>

/* socklen_t is defined in FreeBSD 3.4-STABLE, 4.0-RELEASE and above */
#if (P_FREEBSD <= 340000)
typedef int socklen_t;
#endif

#define HAS_IFREQ

///////////////////////////////////////////////////////////////////////////////
#elif defined(P_OPENBSD)

#if defined(P_PTHREADS)
#define _THREAD_SAFE
#define P_THREAD_SAFE_CLIB

#include <pthread.h>
#endif

#include <paths.h>
#include <errno.h>
#include <dlfcn.h>
#include <termios.h>
#include <sys/fcntl.h>
#include <sys/filio.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <net/if.h>
#include <netinet/tcp.h>

#define HAS_IFREQ

///////////////////////////////////////////////////////////////////////////////
#elif defined(P_NETBSD)

#if defined(P_PTHREADS)
#define _THREAD_SAFE
#define P_THREAD_SAFE_CLIB

#include <pthread.h>
#endif

#include <stdlib.h>
#include <paths.h>
#include <errno.h>
#include <dlfcn.h>
#include <termios.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/filio.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <net/if.h>
#include <netinet/tcp.h>

#define HAS_IFREQ

///////////////////////////////////////////////////////////////////////////////
#elif defined(P_SOLARIS)

#include <errno.h>
#include <sys/sockio.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/filio.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <dlfcn.h>
#include <net/if.h>
#include <sys/sockio.h>

#if !defined(P_HAS_UPAD128_T)
typedef union {
  long double _q;
  uint32_t _l[4];
} upad128_t;
#endif

#define INADDR_NONE     -1
#if P_SOLARIS < 7
typedef int socklen_t;
#endif

#define HAS_IFREQ

#if __GNUC__ < 3
extern "C" {

int ftime (struct timeb *);
pid_t wait3(int *status, int options, struct rusage *rusage);
int gethostname(char *, int);

};
#endif

///////////////////////////////////////////////////////////////////////////////
#elif defined (P_SUN4)

#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <net/if.h>
#include <sys/sockio.h>

#define HAS_IFREQ
#define raise(s)    kill(getpid(),s)

extern "C" {

char *mktemp(char *);
int accept(int, struct sockaddr *, int *);
int connect(int, struct sockaddr *, int);
int ioctl(int, int, void *);
int recv(int, void *, int, int);
int recvfrom(int, void *, int, int, struct sockaddr *, int *);
int select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
int sendto(int, const void *, int, int, const struct sockaddr *, int);
int send(int, const void *, int, int);
int shutdown(int, int);
int socket(int, int, int);
int vfork();
void bzero(void *, int);
void closelog();
void gettimeofday(struct timeval * tv, struct timezone * tz);
void openlog(const char *, int, int);
void syslog(int, char *, ...);
int setpgrp(int, int);
pid_t wait3(int *status, int options, struct rusage *rusage);
int bind(int, struct sockaddr *, int);
int listen(int, int);
int getsockopt(int, int, int, char *, int *);
int setsockopt(int, int, int, char *, int);
int getpeername(int, struct sockaddr *, int *);
int gethostname(char *, int);
int getsockname(int, struct sockaddr *, int *);
char * inet_ntoa(struct in_addr);

int ftime (struct timeb *);

struct hostent * gethostbyname(const char *);
struct hostent * gethostbyaddr(const char *, int, int);
struct servent * getservbyname(const char *, const char *);

#include <sys/termios.h>
#undef NL0
#undef NL1
#undef CR0
#undef CR1
#undef CR2
#undef CR3
#undef TAB0
#undef TAB1
#undef TAB2
#undef XTABS
#undef BS0
#undef BS1
#undef FF0
#undef FF1
#undef ECHO
#undef NOFLSH
#undef TOSTOP
#undef FLUSHO
#undef PENDIN
};


///////////////////////////////////////////////////////////////////////////////
#elif __BEOS__

#include <errno.h>
#include <termios.h>
#include <sys/socket.h>
#include <OS.h>
#include <cpp/stl.h>

#define SOCK_RAW 3 // raw-protocol interface, not suported in R4
#define PF_INET AF_INET
#define TCP_NODELAY 1
typedef int socklen_t;
#include <bone/arpa/inet.h>

#define wait3(s, o, r) waitpid(-1, s, o)
int seteuid(uid_t euid);
int setegid(gid_t gid);

#define RTLD_NOW        0x2
extern "C" {
void *dlopen(const char *path, int mode);
int dlclose(void *handle);
void *dlsym(void *handle, const char *symbol);
};

///////////////////////////////////////////////////////////////////////////////
#elif defined (P_MACOSX) || defined(P_MACOS)
 
#if defined(P_PTHREADS)
  #ifndef _THREAD_SAFE
    #define _THREAD_SAFE
  #endif
  #define P_THREAD_SAFE_CLIB
  #include <pthread.h>
#endif
#if defined(P_MAC_MPTHREADS)
#include <CoreServices/CoreServices.h>
// Blasted Mac <CoreServices.h> comes with 17 years of crufty history
// crapping up the namespace, thankyouverymuch.  (What I really want is
// just Multiprocessing.h, but that drags in nearly as much crap and isn't
// readily available on Mac OS X.)
// So:  undefine the troublespots as they occur.
#undef nil // you morons.

// Open Transport and UNIX networking headers don't get along.  Why did
// Apple have to do this?  And what's worse, they are functionally equivalent
// #defines, Apple could have easily made the headers compatible.  But no.
#undef TCP_NODELAY
#undef TCP_MAXSEG
#endif // MPThreads

#include <paths.h>
#include <errno.h>
#include <termios.h>
#include <sys/fcntl.h>
#include <sys/filio.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/signal.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>

#if defined (P_MACOSX) && (P_MACOSX < 800)
typedef int socklen_t;
#endif
 
#define HAS_IFREQ
#define _POSIX_MONOTONIC_CLOCK


///////////////////////////////////////////////////////////////////////////////
#elif defined (P_AIX)

#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <dlfcn.h>
#include <net/if.h>
#include <strings.h>

#define HAS_IFREQ

///////////////////////////////////////////////////////////////////////////////
#elif defined (P_IRIX)

#include <errno.h>
#include <sys/sockio.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/filio.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <dlfcn.h>
#include <net/if.h>
#include <sys/sockio.h>

typedef int socklen_t;

///////////////////////////////////////////////////////////////////////////////
#elif defined (P_VXWORKS)

#include <taskLib.h>
#include <semLib.h>
#include <sysLib.h>
#include <time.h>
#include <ioLib.h>
#include <unistd.h>
#include <selectLib.h>
#include <inetLib.h>
#include <hostLib.h>
#include <ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <socklib.h>
#include <signal.h>

// Prevent conflict between net/mbuf.h and some ASN.1 header files
// VxWorks uses some #define m_data <to-something-else> constructions
#undef m_data
#undef m_type

#define HAS_IFREQ

#define _exit(i)   exit(i)

typedef int socklen_t;

extern int h_errno;

#define SUCCESS    0
#define NOTFOUND   1

struct hostent * Vx_gethostbyname(char *name, struct hostent *hp);
struct hostent * Vx_gethostbyaddr(char *name, struct hostent *hp);

#define strcasecmp strcmp

#define P_HAS_SEMAPHORES
#define _THREAD_SAFE
#define P_THREAD_SAFE_CLIB
typedef long PThreadIdentifier;
typedef long PProcessIdentifier;


///////////////////////////////////////////////////////////////////////////////
#elif defined(P_RTEMS)

#include <sys/termios.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <net/if.h>
typedef int socklen_t;
typedef int64_t         quad_t;
extern "C" {
  int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *tv);
  int strcasecmp(const char *, const char *);
  int strncasecmp(const char *, const char *, size_t);
  char* strdup(const char *);
}
#define PSETPGRP()  tcsetprgrp(0, 0)
#define wait3(s, o, r) waitpid(-1, s, o)
#define seteuid setuid
#define setegid setgid
#define HAS_IFREQ

///////////////////////////////////////////////////////////////////////////////
#elif defined(P_QNX)

#if defined(P_PTHREADS)
#define _THREAD_SAFE
#define P_THREAD_SAFE_CLIB

#include <pthread.h>
#include <resolv.h> /* for pthread's h_errno */
#endif

#include <stdlib.h>
#include <paths.h>
#include <errno.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/termio.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/tcp.h>

#define HAS_IFREQ

///////////////////////////////////////////////////////////////////////////////
#elif defined(P_CYGWIN)
#include <sys/termios.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

///////////////////////////////////////////////////////////////////////////////

// Other operating systems here

#else
#endif

///////////////////////////////////////////////////////////////////////////////

// includes common to all Unix variants

#include <netdb.h>
#include <dirent.h>
#include <limits.h>

#include <netinet/in.h>
#include <errno.h>
#include <sys/socket.h>
#ifndef P_VXWORKS
#include <sys/time.h>
#endif

#ifndef __BEOS__
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

typedef int SOCKET;
typedef pid_t PProcessIdentifier;

#ifndef PSETPGRP
#if P_SETPGRP_NOPARM
#define PSETPGRP()  setpgrp()
#else
#define PSETPGRP()  setpgrp(0, 0)
#endif
#endif

#ifdef P_PTHREADS

#include <pthread.h>
typedef pthread_t PThreadIdentifier;

#if defined(P_HAS_SEMAPHORES) || defined(P_HAS_NAMED_SEMAPHORES)
#include <semaphore.h>
#endif  // P_HAS_SEMPAHORES

#endif  // P_PTHREADS

#ifdef BE_THREADS
typedef thread_id PThreadIdentifier;
#endif // BE_THREADS

#endif // _PMACHDEP_H

// End of file
