AC_INIT(include/ptlib.h)

AC_CONFIG_SUBDIRS(plugins)

PKG_PROG_PKG_CONFIG()
if test -z "$PKG_CONFIG" ; then
  AC_MSG_ERROR(PTLib must have pkg-config installed, 1)
fi

AC_PROG_CC
AC_PROG_CXX

AC_LANG(C++)

m4_include(ptlib.m4)

dnl ########################################################################
dnl set the PREFIX accordingly
if test "x$prefix" = "xNONE"; then
   if test "x$PTLIBDIR" = "x"; then
      prefix="/usr/local"
   else
      prefix="$PTLIBDIR"
      AC_MSG_NOTICE(PTLIBDIR detected - setting install prefix to ${PTLIBDIR})
   fi
fi


dnl ########################################################################
dnl set the PTLIB directory to the current directory

if test "x$PTLIBDIR" = "x"; then
  PTLIBDIR=`pwd`
fi

AC_SUBST(PTLIBDIR)

dnl set the PTLib installation dir for ptlib-config
AC_SUBST(PTINSTDIR)


dnl ########################################################################
dnl extract the PTLib version
dnl MSWIN_DISPLAY version,Version
dnl MSWIN_VERSION version,PTLIB_MAJOR=MAJOR_VERSION
dnl MSWIN_VERSION version,PTLIB_MINOR=MINOR_VERSION
dnl MSWIN_VERSION version,PTLIB_BUILD=BUILD_NUMBER
dnl MSWIN_VERSION version,PTLIB_VERSION=VERSION

MAJOR_VERSION=`cat ${PTLIBDIR}/version.h | grep MAJOR_VERSION | cut -f3 -d' '`
MINOR_VERSION=`cat ${PTLIBDIR}/version.h | grep MINOR_VERSION | cut -f3 -d' '`
BUILD_NUMBER=`cat ${PTLIBDIR}/version.h | grep BUILD_NUMBER | cut -f3 -d' '`
PTLIB_VERSION="${MAJOR_VERSION}.${MINOR_VERSION}.${BUILD_NUMBER}"
BUILD_TYPE=`cat ${PTLIBDIR}/version.h | grep BUILD_TYPE | cut -f 3 -d ' ' | sed 's/BetaCode/-beta/' | sed 's/AlphaCode/-alpha/' | sed 's/ReleaseCode/\./'`

AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(BUILD_NUMBER)
AC_SUBST(PTLIB_VERSION)
AC_SUBST(BUILD_TYPE)
AC_SUBST(MAJOR_VERSION, $MAJOR_VERSION)
AC_SUBST(MINOR_VERSION, $MINOR_VERSION)
AC_SUBST(BUILD_NUMBER, $BUILD_NUMBER)

AC_MSG_NOTICE(PTLib version is ${PTLIB_VERSION});

AC_DEFINE_UNQUOTED([PTLIB_MAJOR],   [$MAJOR_VERSION], [Major version])
AC_DEFINE_UNQUOTED([PTLIB_MINOR],   [$MINOR_VERSION], [Minor version])
AC_DEFINE_UNQUOTED([PTLIB_BUILD],   [$BUILD_NUMBER], [Build number])
AC_DEFINE_UNQUOTED([PTLIB_VERSION], ["$PTLIB_VERSION"], [PTLib version])

dnl ########################################################################
dnl check for minimum sizing

AC_ARG_ENABLE(minsize,AS_HELP_STRING([--enable-minsize],[Minimise the size of the lib by disabling least used options]))

dnl these are the default configure settings
HAS_SAMPLES=no
default_tracing=yes
default_resolver=yes
default_openldap=yes
default_openssl=yes
default_expat=yes
default_sdl=yes
default_sasl=yes
default_mlib=no
HAS_PLUGINS=1
HAS_TTS=1
HAS_ASN=1
HAS_STUN=1
HAS_PIPECHAN=1
HAS_DTMF=1
HAS_WAVFILE=1
HAS_SOCKS=1
HAS_FTP=1
HAS_SNMP=1
HAS_TELNET=1
HAS_REMCONN=1
HAS_SERIAL=1
HAS_POP3SMTP=1
HAS_AUDIO=1
HAS_VIDEO=1
HAS_URL=1
HAS_HTTP=1
HAS_HTTPFORMS=1
HAS_HTTPSVC=1
HAS_SOAP=1
HAS_XMLRPC=1
HAS_JABBER=1
HAS_CONFIG_FILE=1
default_ipv6=no
HAS_QOS=1
HAS_VIDFILE=1
HAS_FFVDEV=1
HAS_APPSHARE=
HAS_SHM_VIDEO=1
HAS_VXML=1
HAS_LUA=1
HAS_EXCEPTIONS=
HAS_VFW_CAPTURE=

if test "${enable_minsize}x" = "yesx" ; then
  default_resolver=no
  default_openldap=no
  default_openssl=no
  default_expat=no
  default_sdl=no
  default_sasl=no
  default_mlib=no
  HAS_PLUGINS=1
  HAS_TTS=
  HAS_ASN=1
  HAS_STUN=1
  HAS_PIPECHAN=
  HAS_DTMF=
  HAS_WAVFILE=
  HAS_SOCKS=
  HAS_FTP=
  HAS_SNMP=
  HAS_TELNET=
  HAS_REMCONN=
  HAS_SERIAL=
  HAS_POP3SMTP=
  HAS_AUDIO=1
  HAS_VIDEO=1
  HAS_URL=1
  HAS_HTTP=1
  HAS_HTTPFORMS=
  HAS_HTTPSVC=
  HAS_SOAP=
  HAS_XMLRPC=
  HAS_JABBER=
  HAS_CONFIG_FILE=0
  default_ipv6=no
  HAS_QOS=
  HAS_VIDFILE=
  HAS_FFVDEV=
  HAS_APPSHARE=
  HAS_VXML=
  HAS_SHM_VIDEO=
  HAS_EXCEPTIONS=
  HAS_VFW_CAPTURE=
  HAS_LUA=
fi

dnl ########################################################################
dnl set the OSTYPE and MACHTYPE

dnl MSWIN_DISPLAY winver,Advanced Windows API
dnl MSWIN_DEFINE  winver,WINVER=0x502


AC_CANONICAL_TARGET()


TARGETDIR=unix
OSTYPE=
NEEDS_PTHREADS=yes

case "$target_os" in
  linux*)          OSTYPE=linux ;
                   OSRELEASE="\"`uname -r`\"";
		   OS_TAG="P_LINUX" ;
                   need_pragma=yes ;
		   AC_DEFINE(P_PTHREADS, 1, "PThreads found")
		   AC_CHECK_FUNC([swab], [AC_DEFINE([USE_SYSTEM_SWAB], [], [Use system swab()] )] )
                   ;;

  gnu*)            OSTYPE=gnu ;
                   OSRELEASE="\"`uname -r`\"";
		   OS_TAG="P_GNU_HURD" ;
                   need_pragma=yes ;
		   AC_DEFINE(P_PTHREADS, 1, "PThreads found")
		   AC_CHECK_FUNC([swab], [AC_DEFINE([USE_SYSTEM_SWAB], [], [Use system swab()] )] )
                   ;;

  freebsd*|kfreebsd*)       OSTYPE=FreeBSD ;
		   OS_TAG="P_FREEBSD" ;
                   if test "x$OSRELEASE" = "x"; then
                     OSRELEASE="`sysctl -n kern.osreldate`" ;
                   fi
                   need_pragma=yes ;
		   AC_DEFINE(P_PTHREADS, 1, "PThreads found")
		   AC_CHECK_FUNC([swab], [AC_DEFINE([USE_SYSTEM_SWAB], [], [Use system swab()] )] )
		   ;;

  openbsd* )       OSTYPE=OpenBSD ;
		   OS_TAG="P_OPENBSD" ;
                   OSRELEASE="`sysctl -n kern.osrevision`" ;
		   ENDLDLIBS="-lossaudio" ;
                   need_pragma=yes ;
		   AC_DEFINE(P_PTHREADS, 1, "PThreads found")
		   AC_CHECK_FUNC([swab], [AC_DEFINE([USE_SYSTEM_SWAB], [], [Use system swab()] )] )
		   ;;

  netbsd* )        OSTYPE=NetBSD ;
		   OS_TAG="P_NETBSD" ;
                   OSRELEASE="`/sbin/sysctl -n kern.osrevision`" ;
		   ENDLDLIBS="-lossaudio" ;
                   need_pragma=yes ;
		   AC_DEFINE(P_PTHREADS, 1, "PThreads found")
		   AC_CHECK_FUNC([swab], [AC_DEFINE([USE_SYSTEM_SWAB], [], [Use system swab()] )] )
		   ;;

  solaris* | sunos* ) OSTYPE=solaris ;
		   OS_TAG="P_SOLARIS" ;
                   OSRELEASE=`uname -r | sed "s/5\.//g"` ;
                   STDCCFLAGS="$STDCCFLAGS -D__inline=inline -DSOLARIS"
                   need_pragma=yes ;
		   AC_DEFINE(P_PTHREADS, 1, "PThreads found")
                   dnl According to Andre Schulze <as8@rncmm2.urz.tu-dresden.de>
                   dnl Solaris swab should not be used
                   PTLIB_CHECK_UPAD128( [AC_DEFINE(P_HAS_UPAD128_T)] )
                   PTLIB_CHECK_FDSIZE()
		   ;;

  darwin* )	   OSTYPE=Darwin ;
		   OS_TAG="P_MACOSX" ;
		   OS_MAJOR=`uname -r | sed 's/\..*$//'`
		   OS_MAJOR=`expr $OS_MAJOR \* 100`
		   OS_MINOR=[`uname -r | sed -e 's/[0-9][0-9]*\.//' -e 's/\..*$//'`]
		   OSRELEASE=`expr $OS_MAJOR + $OS_MINOR`
                   SHAREDLIBEXT="dylib";
		   AC_DEFINE(P_PTHREADS, 1, "PThreads found")
		   AC_CHECK_FUNC([swab], [AC_DEFINE([USE_SYSTEM_SWAB], [], [Use system swab()] )] )
		   ;;

  beos*)           OSTYPE=beos ;
                   OS_TAG="P_BEOS" ;
                   OSRELEASE=`uname -r` ;
                   STDCCFLAGS="$STDCCFLAGS -D__BEOS__"
                   AC_CHECK_FUNC([swab], [AC_DEFINE([USE_SYSTEM_SWAB], [], [Use system swab()] )] )
		   ;;

  cygwin*)         OSTYPE=cygwin ;
                   OS_TAG="P_CYGWIN" ;
                   OSRELEASE="`uname -r`";
                   AC_CHECK_FUNC([swab], [AC_DEFINE([USE_SYSTEM_SWAB], [], [Use system swab()] )] )
                   HAS_SHM_VIDEO=
		   ;;

  mingw*)	   OSTYPE=mingw ;
		   OS_TAG="P_MINGW" ;
		   OSRELEASE=`uname -r` ;
		   TARGETDIR=msos ;
		   SHAREDLIBEXT="dll" ;
		   STDCCFLAGS="$STDCCFLAGS -mms-bitfields" ;
		   ENDLDLIBS="-lwinmm -lwsock32 -lws2_32 -lsnmpapi -lmpr -lcomdlg32 -lgdi32 -lavicap32 -liphlpapi -lole32" ;
		   NEEDS_PTHREADS=no ;
		   AC_CHECK_FUNC([swab], [AC_DEFINE([USE_SYSTEM_SWAB], [], [Use system swab()] )] )
		   HAS_SHM_VIDEO=
		   ;;

  * )		   OSTYPE="$target_os" ;
		   OS_TAG="P_UNKNOWN_OS" ;
  		   OSRELEASE="\"${OSTYPE}_`uname -r`\"" ;
  		   AC_CHECK_FUNC([swab], [AC_DEFINE([USE_SYSTEM_SWAB], [], [Use system swab()] )] )
		   AC_MSG_WARN("OS $target_os not recognized - proceed with caution!") ;
		   ;;
esac

MACHTYPE=
P_64BIT=0

case "$target_cpu" in
   x86 | i686 | i586 | i486 | i386 ) 
	AC_COMPILE_IFELSE(
          [AC_LANG_SOURCE([[int t = __amd64__;]])],
	  [[MACHTYPE=x86_64
	    P_64BIT=1]],
	  [[MACHTYPE=x86]]
        )
	PBYTE_ORDER=PLITTLE_ENDIAN
	;;

   x86_64 | amd64)
	MACHTYPE=$target_cpu
	P_64BIT=1
        LIB64=1
	;;

   alpha | alphaev56 | alphaev6 | alphaev67 | alphaev7)
	MACHTYPE=alpha
	P_64BIT=1
	;;

   hppa )
	MACHTYPE=hppa
	;;

   hppa64 )
	MACHTYPE=hppa64
	P_64BIT=1
        LIB64=1
	;;

   sparc | sparc64 )
	MACHTYPE=sparc
	P_64BIT=1
        LIB64=1
	;;

   ppc | powerpc )
	MACHTYPE=ppc
	;;

   ppc64 | powerpc64 )
	MACHTYPE=ppc64
	P_64BIT=1
        LIB64=1
	;;

   ia64 )
	MACHTYPE=ia64
	P_64BIT=1
	;;

   s390x )
	MACHTYPE=s390x
	P_64BIT=1
        LIB64=1
	;;

   s390 )
	MACHTYPE=s390
	;;

   * )
	MACHTYPE=$target_cpu
	AC_MSG_WARN("CPU $target_cpu not recognized - proceed with caution!")
	;;
esac


AC_ARG_ENABLE(force32, AS_HELP_STRING([--enable-force32],[Force 32-bit x86 build]))
if test "$MACHTYPE" = "x86_64" -a "${enable_force32}" = "yes" ; then
  if test "$OSTYPE" = "Darwin"; then
    STDCCFLAGS="$STDCCFLAGS -arch i386"
    LDFLAGS="$LDFLAGS -arch i386"
  else
    STDCCFLAGS="$STDCCFLAGS -m32"
    LDFLAGS="$LDFLAGS -m32"
  fi

  MACHTYPE=x86
  P_64BIT=0
  LIB64=0
  AC_MSG_NOTICE(Forcing 32 bit x86 compile)
fi


AC_MSG_NOTICE(OSTYPE set to $OSTYPE);
AC_MSG_NOTICE(OSRELEASE set to $OSRELEASE);
AC_MSG_NOTICE(MACHTYPE set to $MACHTYPE);

AC_SUBST(MACHTYPE,  $MACHTYPE)
AC_SUBST(OSTYPE,    $OSTYPE)
AC_SUBST(OSRELEASE, $OSRELEASE)
AC_SUBST(TARGETDIR, $TARGETDIR)

AC_DEFINE_UNQUOTED($OS_TAG, $OSRELEASE)

if test "$P_64BIT" = "1" ; then
  AC_DEFINE([P_64BIT], [1], [64bit architecture])
  STDCCFLAGS="$STDCCFLAGS -DP_64BIT"
fi

if test "$target_os" = "linux-gnulp" ; then
  AC_DEFINE([P_LPIA], [1], [Low Power Intel Architecture])
fi


dnl ########################################################################
dnl set LIBDIR accordingly
if test "$prefix" = "$PTLIBDIR"; then
   LIBDIR=${libdir}_${OSTYPE}_${MACHTYPE}
else
   LIBDIR=${libdir}
fi
AC_SUBST(LIBDIR)


dnl ########################################################################
dnl get the gcc version

USE_GCC=
USE_PCH=

if test x$GXX = xyes ; then
  USE_GCC=yes
  gcc_version=`$CXX -dumpversion`
  AC_MSG_NOTICE(gcc version is $gcc_version);
  GXX_MAJOR=`echo $gcc_version | sed 's/\..*$//'`
  GXX_MINOR=[`echo $gcc_version | sed -e 's/[0-9][0-9]*\.//' -e 's/\..*$//'`]
  GXX_PATCH=[`echo $gcc_version | sed -e 's/[0-9][0-9]*\.[0-9][0-9]*\.//' -e 's/\..*$//'`]

  dnl only enable pragma for gcc 4.0 and later
  AC_MSG_CHECKING(checking if pragma implementation should be used)
  if test ${GXX_MAJOR} -lt 4 ; then
    if test "${need_pragma}" = "yes" ; then
      STDCCFLAGS="$STDCCFLAGS -DP_USE_PRAGMA"
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
    fi
  else
    AC_MSG_RESULT(no)
  fi

  dnl only enable PCH for gcc 4.0 and later
  AC_MSG_CHECKING(checking if precompiled headers available)
  if test ${GXX_MAJOR} -lt 4 ; then
    enable_pch=no
  else
    AC_ARG_ENABLE(pch, AS_HELP_STRING([--enable-pch],[Enable precompiled headers]))
  fi
  if test "${enable_pch}" = "yes" ; then
    AC_MSG_RESULT(yes)
    USE_PCH=1
  else
    AC_MSG_RESULT(no)
  fi
else
  USE_GCC=no
fi

AC_SUBST(USE_GCC)
AC_SUBST(USE_PCH)

dnl ########################################################################
dnl check if precompiled headers available, or disabled

dnl ########################################################################
dnl set the shared library extension, if not already set

if test "x${SHAREDLIBEXT}" = "x" ; then
  SHAREDLIBEXT="so"
fi
AC_SUBST(SHAREDLIBEXT, $SHAREDLIBEXT)
AC_DEFINE([P_SHAREDLIBEXT], [$SHAREDLIBEXT], [shared library extension])

dnl ########################################################################
dnl check the endianness

dnl MSWIN_DISPLAY byteorder,Byte Order
dnl MSWIN_DEFINE  byteorder,PBYTE_ORDER=PLITTLE_ENDIAN

if test ${OSTYPE} != "Darwin" ; then  
  AC_C_BIGENDIAN()
  if test ${ac_cv_c_bigendian} = yes ; then
    PBYTE_ORDER=PBIG_ENDIAN
  else
    PBYTE_ORDER=PLITTLE_ENDIAN
  fi

  AC_DEFINE_UNQUOTED(PBYTE_ORDER, $PBYTE_ORDER, "byte order")
fi

AC_MSG_NOTICE(Default extension for C++ files is $ac_ext)

dnl ########################################################################
dnl look for compile flags
AC_LANG_PUSH(C++)

AC_MSG_CHECKING(if compiler accepts -felide-constructors)
AC_LANG_CONFTEST([AC_LANG_PROGRAM([[]],[[]])])
$CXX $CXXFLAGS -felide-constructors conftest.$ac_ext > /dev/null 2>&1
if test $? = 0 ; then
  AC_MSG_RESULT(yes)
  STDCXXFLAGS="$STDCXXFLAGS -felide-constructors"
else
  AC_MSG_RESULT(no)
fi

AC_MSG_CHECKING(if linker accepts -Wreorder)
AC_LANG_CONFTEST([AC_LANG_PROGRAM([[]],[[]])])
$CXX $CXXFLAGS -Wreorder conftest.$ac_ext > /dev/null 2>&1
if test $? = 0 ; then
  AC_MSG_RESULT(yes)
  STDCXXFLAGS="$STDCXXFLAGS -Wreorder"
else
  AC_MSG_RESULT(no)
fi


dnl add additional information for the debugger to ensure the user can indeed
dnl debug coredumps and macros.

AC_MSG_CHECKING(if debug build accepts -g3 -ggdb -O0)
AC_LANG_CONFTEST([AC_LANG_PROGRAM([[]],[[]])])
$CXX  $CXXFLAGS  -g3 -ggdb -O0 conftest.$ac_ext > /dev/null 2>&1
if test $? = 0 ; then
  AC_MSG_RESULT(yes)
  DEBUG_FLAG="-g3 -ggdb -O0"
else
  AC_MSG_RESULT(no)
  DEBUG_FLAG="-g"
fi


AC_MSG_CHECKING(if compiler uses RTTI by default)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <typeinfo>]],
[[ class A
  {
    public:
      virtual void fred() { }
  };
  class B : public A
  {
    public:
      void fred() { }
  };
  B t;
  A * p = dynamic_cast<A *>(&t);
  return 0;
]])],DEFAULT_RTTI=1, DEFAULT_RTTI=0)
if test "$DEFAULT_RTTI" = "1" ; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  STDCXXFLAGS="$STDCCFLAGS -frtti"
fi

AC_LANG_POP(C++)

dnl ########################################################################
dnl check for the size of int
AC_CHECK_SIZEOF(int)

dnl ########################################################################
dnl check for long double

AC_C_LONG_DOUBLE
if test $ac_cv_c_long_double != yes ; then
  AC_DEFINE([PNO_LONG_DOUBLE], [], [No long double])
fi

dnl if test ${OSTYPE} = Darwin ; then
dnl   STDCCFLAGS="$STDCCFLAGS -Wno-long-double"
dnl fi

dnl ########################################################################
dnl we need flex, so we better make sure it's there
AC_PATH_PROG(FLEX, flex)
if test -z "$FLEX" ; then
        AC_MSG_ERROR(PTLib needs the GNU Project fast lexical analyzer generator flex, 1)
fi

dnl ########################################################################
dnl most systems have "ar", but some use "gar"
AC_CHECK_TOOL(AR, ar)
if test -z "$AR" ; then
  AC_CHECK_TOOL(AR, gar)
fi

dnl ########################################################################
dnl we need bison, so we better make sure it's there
AC_PATH_PROG(BISON, bison)
if test -z "$BISON" ; then
        AC_MSG_ERROR(PTLib needs the GNU Project parser generator bison, 1)
fi

dnl ########################################################################
dnl check for number of parms to readdir
AC_MSG_CHECKING(if readdir_r has 2 parms)
AC_TRY_COMPILE([#define _POSIX_PTHREAD_SEMANTICS
                #include <dirent.h>],
               [DIR *s; struct dirent * e; readdir_r(s, e);], P_HAS_POSIX_READDIR_R=2, P_HAS_POSIX_READDIR_R=0)
if test ${P_HAS_POSIX_READDIR_R} = 2 ; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  AC_MSG_CHECKING(if readdir_r has 3 parms)
  AC_TRY_COMPILE([#include <dirent.h>],
                 [DIR *s; struct dirent * e; struct dirent * p; readdir_r(s, e, &p);], P_HAS_POSIX_READDIR_R=3, P_HAS_POSIX_READDIR_R=0)
  if test ${P_HAS_POSIX_READDIR_R} = 3 ; then
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
fi
AC_DEFINE_UNQUOTED(P_HAS_POSIX_READDIR_R, ${P_HAS_POSIX_READDIR_R})



dnl ########################################################################
dnl check for tracing
dnl

AC_ARG_ENABLE(tracing, AS_HELP_STRING([--disable-tracing],[Remove PTRACE and all trace logging]))

if test "x${enable_tracing}" = "xno" ; then
  AC_MSG_NOTICE(PTRACE and all trace logging removed)
  STDCCFLAGS="$STDCCFLAGS -DPTRACING=0 -DPASN_NOPRINTON -DPASN_LEANANDMEAN"
else
  AC_MSG_NOTICE(PTRACE and all trace logging included)
  STDCCFLAGS="$STDCCFLAGS -DPTRACING=1"
fi


dnl ########################################################################
dnl check for Windows-like integer BOOL
dnl
dnl MSWIN_DISPLAY ansi-bool,ANSI C++ bool
dnl MSWIN_DEFINE  ansi-bool,P_USE_STANDARD_CXX_BOOL

AC_ARG_ENABLE(ansi-bool,AS_HELP_STRING([--disable-ansi-bool],[Use Windows-like integer BOOL over ANSI C++ bool]))

if test "x${enable_ansi_bool}" = "xno" ; then
  AC_MSG_NOTICE(Using Windows-like integer BOOL for PBoolean: (PFalse=0 PTrue=1))
else
  AC_MSG_NOTICE(Using ANSI C++ bool for PBoolean: (PFalse=false PTrue=true))
  AC_DEFINE(P_USE_STANDARD_CXX_BOOL)
fi


dnl ########################################################################
dnl check for recvmsg

P_HAS_RECVMSG=
AC_MSG_CHECKING(for recvmsg)
AC_TRY_COMPILE([#include <sys/types.h>
                #include <sys/socket.h>
                #include <netinet/in.h>],
               [int t = SOL_IP; t = IP_PKTINFO; struct msghdr *msg; recvmsg(0, msg, 0);], P_HAS_RECVMSG=1)
if test "x${P_HAS_RECVMSG}" = "x1" ; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(P_HAS_RECVMSG, 1)
else
  AC_MSG_RESULT(no)
fi

dnl ########################################################################
dnl check for IPV6_ADD_MEMBERSHIP

P_HAS_IPV6_ADD_MEMBERSHIP=
AC_MSG_CHECKING(for IPV6_ADD_MEMBERSHIP)
AC_TRY_COMPILE([#include <netinet/in.h>],
               [int x = IPV6_ADD_MEMBERSHIP;],
               P_HAS_IPV6_ADD_MEMBERSHIP=1)
if test "x${P_HAS_IPV6_ADD_MEMBERSHIP}" = "x1" ; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(P_HAS_IPV6_ADD_MEMBERSHIP, 1)
else
  AC_MSG_RESULT(no)
fi

dnl ########################################################################
dnl check for NetLink support

P_HAS_NETLINK=0
AC_MSG_CHECKING(for NetLink support)
AC_TRY_COMPILE([#include <asm/types.h>
                #include <sys/socket.h>
                #include <linux/netlink.h>
                #include <linux/rtnetlink.h>
                #include <linux/genetlink.h>
                #include <memory.h>],
               [struct sockaddr_nl sa; memset(&sa, 0, sizeof(sa));
                sa.nl_family = AF_NETLINK; sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;
                int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);], P_HAS_NETLINK=1)
if test "${P_HAS_NETLINK}" = "1" ; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(P_HAS_NETLINK, 1)
else
  AC_MSG_RESULT(no)
fi

dnl ########################################################################
dnl check for streams types

P_HAS_STL_STREAMS=0
AC_MSG_CHECKING(if using STL streams)
AC_TRY_COMPILE([#include <iostream>],
               [int v = (int)std::ios_base::seekdir();], P_HAS_STL_STREAMS=1)
if test "${P_HAS_STL_STREAMS}" = 1 ; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi
AC_DEFINE_UNQUOTED(P_HAS_STL_STREAMS, ${P_HAS_STL_STREAMS})

dnl ########################################################################
dnl see if using GNU glibc++, because then we have atomic integer operations

AC_ARG_ENABLE(atomicity,AS_HELP_STRING([--disable-atomicity],[Do not use atomicity.h]))

P_HAS_ATOMIC_INT=0
P_NEEDS_GNU_CXX_NAMESPACE=0
AC_MSG_CHECKING(if atomic integer available)
if test "${enable_atomicity}" = "no" ; then
  AC_MSG_RESULT(disabled)
else
  AC_TRY_COMPILE([#include <bits/atomicity.h>],
                 [_Atomic_word val;], P_HAS_ATOMIC_INT=1,
                 AC_TRY_COMPILE([#include <ext/atomicity.h>],
                                [_Atomic_word val;], P_HAS_ATOMIC_INT=2)
  )
  if test ${P_HAS_ATOMIC_INT} != 0 ; then
    AC_MSG_RESULT(yes)
    AC_MSG_CHECKING(if __exchange_and_add is in __gnu_cxx namespace)
    if test ${P_HAS_ATOMIC_INT} = 1 ; then
      AC_TRY_COMPILE([#include <bits/atomicity.h>],
                     [_Atomic_word val; __gnu_cxx::__exchange_and_add(&val, 1)],
                     P_NEEDS_GNU_CXX_NAMESPACE=1)
    else
      P_HAS_ATOMIC_INT=1
      AC_TRY_COMPILE([#include <ext/atomicity.h>],
                     [_Atomic_word val; __gnu_cxx::__exchange_and_add(&val, 1)],
                     P_NEEDS_GNU_CXX_NAMESPACE=1)
    fi
    if test ${P_NEEDS_GNU_CXX_NAMESPACE} = 1 ; then
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
    fi
  else
    AC_MSG_RESULT(no)
  fi
fi

AC_DEFINE_UNQUOTED(P_HAS_ATOMIC_INT, ${P_HAS_ATOMIC_INT})
AC_DEFINE_UNQUOTED(P_NEEDS_GNU_CXX_NAMESPACE, ${P_NEEDS_GNU_CXX_NAMESPACE})

dnl ########################################################################
dnl check for Unix semaphores

# mingw implements itself the semaphores in src/ptlib/msos/win32.cxx
if test ${OSTYPE} != mingw ; then

dnl posix unnamed semaphore support in darwin/macosx is broken
if test ${OSTYPE} != Darwin ; then
  AC_TRY_COMPILE([#include <semaphore.h>],
                 [sem_t *s; sem_init(s, 0, 0);], P_HAS_SEMAPHORES=1)
  AC_MSG_CHECKING(if Unix semaphores are available)
  if test "${P_HAS_SEMAPHORES}x" != "x" ; then
    AC_DEFINE(P_HAS_SEMAPHORES)
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
fi

if test "${P_HAS_SEMAPHORES}x" = "x" ; then

  AC_TRY_COMPILE([#include <semaphore.h>],
                 [sem_t *s = sem_open("test", O_CREAT)],
                 P_HAS_NAMED_SEMAPHORES=1 )
  AC_MSG_CHECKING(if named semaphores are available)
  if test "${P_HAS_NAMED_SEMAPHORES}x" != "x"; then
    AC_DEFINE(P_HAS_NAMED_SEMAPHORES)
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
    AC_MSG_WARN(This host does not support semaphores which are essential)
    AC_MSG_WARN(to implement 'critical sections'. This feature was added)
    AC_MSG_WARN(in PWLib 1.7.0 and improves the stability of multithreaded)
    AC_MSG_WARN(programs. This feature has been disabled for this host)
  fi
fi

fi  # mingw

dnl ########################################################################
dnl check for pthreads library

dnl MSWIN_DISPLAY semaphore,Semaphore Support
dnl MSWIN_DEFINE  semaphore,P_HAS_SEMAPHORES

dnl Linux, Mac OS X use libpthread and -lpthread
dnl FreeBSD uses the -pthread compiler option and uses libc_r

AC_CHECK_LIB(pthread, pthread_create, HAS_PTHREADS=yes, HAS_PTHREADS=no)
if test ${HAS_PTHREADS} = yes ; then
  ENDLDLIBS="$ENDLDLIBS -lpthread"
  STDCCFLAGS="$STDCCFLAGS -D_REENTRANT"
else
  if test ${OSTYPE} = FreeBSD ; then
    HAS_PTHREADS=yes
    ENDLDLIBS="$ENDLDLIBS -pthread"
    STDCCFLAGS="$STDCCFLAGS -D_REENTRANT -pthread"
  fi
fi


if test ${HAS_PTHREADS} != yes -a ${NEEDS_PTHREADS} = yes; then
  AC_MSG_ERROR(must have pthreads!)
fi

dnl ########################################################################
dnl check for rt library

AC_CHECK_LIB(rt, clock_gettime, HAS_RT=yes, HAS_RT=no)
if test ${HAS_RT} = yes ; then
  ENDLDLIBS="$ENDLDLIBS -lrt"
fi


if test ${HAS_PTHREADS} != yes -a ${NEEDS_PTHREADS} = yes; then
  AC_MSG_ERROR(must have pthreads!)
fi

dnl ########################################################################
dnl check for XPG6 compliant pthreads (timed spinlocks)

if test ${HAS_PTHREADS} = yes ; then
  AC_MSG_CHECKING(if pthread_mutex_timedlock is available)
  AC_TRY_COMPILE([#include <pthread.h>],
                  [pthread_mutex_timedlock(NULL, NULL);], HAS_PTHREADS_XPG6=yes, HAS_PTHREADS_XPG6=no)
  if test ${HAS_PTHREADS_XPG6} = yes ; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(P_PTHREADS_XPG6, 1, "XPG6 compliant pthreads found")
  else
    AC_MSG_RESULT(no)
  fi

  AC_CHECK_LIB(pthread, sem_timedwait, HAS_SEMAPHORES_XPG6=yes, HAS_SEMAPHORES_XPG6=no)
  if test ${HAS_SEMAPHORES_XPG6} = yes ; then
    AC_MSG_CHECKING(if sem_timedwait is compilable)
    AC_TRY_COMPILE([#include <semaphore.h>],
                    [sem_t *s; const struct timespec * t; sem_timedwait(s, t);], HAS_SEMAPHORES_XPG6=yes, HAS_SEMAPHORES_XPG6=no)
    if test ${HAS_SEMAPHORES_XPG6} = yes ; then
      AC_DEFINE(P_HAS_SEMAPHORES_XPG6, 1, "Supports XPG6 Semaphores")
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
    fi
  fi
fi

dnl ########################################################################
dnl check for recursive mutexes
P_HAS_RECURSIVE_MUTEX=0
AC_MSG_CHECKING(if recursive mutexes are available)
AC_TRY_COMPILE([#include <pthread.h>],
               [pthread_mutexattr_t attr; pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);], P_HAS_RECURSIVE_MUTEX=1)
if test ${P_HAS_RECURSIVE_MUTEX} = 1 ; then
  AC_MSG_RESULT(PTHREAD_MUTEX_RECURSIVE_NP)
else
  AC_TRY_COMPILE([#include <pthread.h>],
	   [pthread_mutexattr_t attr; pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);], P_HAS_RECURSIVE_MUTEX=2)
  if test ${P_HAS_RECURSIVE_MUTEX} = 2 ; then
    AC_MSG_RESULT(PTHREAD_MUTEX_RECURSIVE)
  else
    AC_MSG_RESULT(no)
  fi
fi
AC_DEFINE_UNQUOTED(P_HAS_RECURSIVE_MUTEX, ${P_HAS_RECURSIVE_MUTEX})


dnl ########################################################################
dnl check if setpgrp takes non or two arguments
P_SETPGRP_SYSTEMV=0
P_SETPGRP_NOPARM=0
AC_MSG_CHECKING(if setpgrp takes no parameters)
AC_TRY_COMPILE([#include <unistd.h>],
               [pid_t p = setpgrp();], P_SETPGRP_NOPARM=1)
if test ${P_SETPGRP_NOPARM} = 1 ; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi
AC_DEFINE_UNQUOTED(P_SETPGRP_NOPARM, ${P_SETPGRP_NOPARM})


dnl ########################################################################
dnl check for regex library

dnl yes this test needs to be outside the if below
dnl because autoconf needs to initialise stuff. without this
dnl all sorts of strange things happen
AC_CHECK_HEADERS(regex.h, HAS_REGEX=yes)

AC_ARG_ENABLE(internalregex,
        AS_HELP_STRING([--enable-internalregex],[Force internal regex library]),
        internalregex=$enableval)
if test "$internalregex" = "yes" ; then
  AC_MSG_NOTICE(Forcing use of internal regex library)
else
  if test "$HAS_REGEX" = "yes" ; then
    AC_CHECK_FUNC(regcomp, HAS_REGEX=yes)
    if test "$HAS_REGEX" = "yes" ; then
      AC_SUBST(HAS_REGEX, 1)
      AC_DEFINE(P_REGEX, 1, "Regex library found")
      if test ${OSTYPE} = mingw ; then
        ENDLDLIBS="$ENDLDLIBS -lregex"
      fi
    fi
  fi
fi


dnl ########################################################################
dnl check for dynamic library code (dlopen and friends)

dnl MSWIN DLL Support,,P_DYNALINK

case "$target_os" in
  cygwin* | mingw* )
    PTLIB_SIMPLE_OPTION([plugins], [PLUGINS], [enable plugin support])
    AC_DEFINE(P_DYNALINK, 1, "Support for dynamic loading of libraries")
    ;;
  *)
    PTLIB_FIND_LIBDL([HAS_LIBDL=1], [HAS_LIBDL=])
    if test "x$HAS_LIBDL" = "x1" ; then
      AC_DEFINE(P_DYNALINK, 1, "Support for dynamic loading of libraries")
      dnl The rest is done further down...
    fi

    dnl #########################################################################
    dnl check to see if plugins enabled
    dnl MSWIN_DISPLAY plugins,Plugin Support
    dnl MSWIN_DEFINE  plugins,P_PLUGINS

    PTLIB_SIMPLE_OPTION([plugins], [PLUGINS], [enable plugin support], [HAS_LIBDL])
    ;;
esac

AC_MSG_CHECKING(where plugins are being installed)
AC_ARG_WITH([plugin-installdir],
             AS_HELP_STRING([--with-plugin-installdir=DIR],[Location where plugins are installed, starting at the lib dir]),
             [PLUGIN_DIR="$withval"],
             [PLUGIN_DIR="ptlib-${PTLIB_VERSION}"]
           )

EXPANDED_PLUGIN_DIR="${LIBDIR}/${PLUGIN_DIR}"
if test "x${exec_prefix}" = "xNONE" ; then
  if test "x${prefix}" = "xNONE" ; then
    EXPANDED_PLUGIN_DIR=`echo ${EXPANDED_PLUGIN_DIR} | sed s#\\${exec_prefix}#/usr/local#`
  else
    EXPANDED_PLUGIN_DIR=`echo ${EXPANDED_PLUGIN_DIR} | sed s#\\${exec_prefix}#${prefix}#`
  fi
else
  EXPANDED_PLUGIN_DIR=`echo ${EXPANDED_PLUGIN_DIR} | sed s#\\${exec_prefix}#${exec_prefix}#`
fi

case "$target_os" in
  cygwin* | mingw* )
    EXPANDED_PLUGIN_DIR="plugins"
  ;;
esac

AC_MSG_RESULT(${EXPANDED_PLUGIN_DIR})

AC_DEFINE_UNQUOTED([P_DEFAULT_PLUGIN_DIR], ["${EXPANDED_PLUGIN_DIR}"], [Default plugin directory])

DEV_PLUGIN_DIR="${PLUGIN_DIR}/devices"
AC_SUBST(DEV_PLUGIN_DIR)

dnl #########################################################################
dnl check to see if samples are enabled

PTLIB_SIMPLE_OPTION([samples], [SAMPLES], [enable samples build])

dnl ########################################################################
dnl look for DNS resolver code (must be done before OpenLDAP)
dnl on some systems, the resolver is in libc
dnl but needs resolv.h included which defines res_search as __res_search

dnl MSWIN_DISPLAY    resolver,DNS Resolver
dnl MSWIN_CHECK_FILE resolver,include\windns.h,P_DNS=1

AC_ARG_ENABLE([resolver],
	      AS_HELP_STRING([--disable-resolver],[disable DNS resolver support]),
	      [enable_resolver=$enableval],
	      [enable_resolver=$default_resolver]
	     )

HAS_RESOLVER=
if test "$enable_resolver" = "no" ; then
  AC_MSG_NOTICE(DNS resolver disabled)
else
  PTLIB_FIND_RESOLVER([HAS_RESOLVER=1], [HAS_RESOLVER=])

  if test "x${HAS_RESOLVER}" = "x1" ; then
    AC_DEFINE(P_DNS, 1, "resolver libraries found")
    if test "${HAS_RES_NINIT}z" != "z" ; then
      AC_DEFINE(P_HAS_RES_NINIT)
    fi
  fi
fi

AC_SUBST(HAS_RESOLVER)

dnl ########################################################################
dnl Look for SASL

dnl MSWIN_DISPLAY    sasl,Cyrus SASL
dnl MSWIN_CHECK_FILE sasl,include\sasl.h,P_SASL=1
dnl MSWIN_DIR_SYMBOL sasl,SASL_DIR
dnl MSWIN_CHECK_DIR  sasl,..\sasl\
dnl MSWIN_CHECK_DIR  sasl,..\cyrus-sasl\
dnl MSWIN_CHECK_DIR  sasl,..\external\sasl\
dnl MSWIN_CHECK_DIR  sasl,..\external\cyrus-sasl\
dnl MSWIN_CHECK_DIR  sasl,\sasl\
dnl MSWIN_CHECK_DIR  sasl,\cyrus-sasl\
dnl MSWIN_CHECK_DIR  sasl,c:\sasl\
dnl MSWIN_CHECK_DIR  sasl,c:\cyrus-sasl\
  
AC_ARG_ENABLE([sasl],
	      AS_HELP_STRING([--disable-sasl],[disable SASL support]),
	      [enable_sasl=$enableval],
	      [enable_sasl=$default_sasl]
	     )

if test "$enable_sasl" = "no" ; then
  AC_MSG_NOTICE(SASL disabled)
else 

  AC_CHECK_LIB([sasl2], [sasl_version], [HAS_SASL2=1], [HAS_SASL2=])
  AC_CHECK_LIB([sasl], [sasl_client_out], [HAS_SASL=1], [HAS_SASL=])

  PTLIB_CHECK_SASL_INCLUDE([
                            if test "x${SASL_HEADER}" = "xsasl" ; then
                              AC_DEFINE(P_HAS_SASL_SASL_H)
                            fi
                           ],
                           [
                            HAS_SASL=
                            HAS_SASL2=
                           ])

  if test "x${HAS_SASL2}" = "x1" ; then
    ENDLDLIBS="$ENDLDLIBS -lsasl2"
    AC_DEFINE_UNQUOTED(P_SASL,2)
  elif test "x${HAS_SASL}" = "x1" ; then
    ENDLDLIBS="$ENDLDLIBS -lsasl"
    AC_DEFINE_UNQUOTED(P_SASL,1)
  fi
fi dnl disable_Sasl
AC_SUBST(HAS_SASL2)
AC_SUBST(HAS_SASL)


dnl ########################################################################
dnl check for SUN mediaLib library

AC_ARG_ENABLE([mlib],
	      AS_HELP_STRING([--enable-mlib],[enable SUN mlib support]),
	      [enable_mlib=$enableval],
	      [enable_mlib=$default_mlib]
	     )
if test "$enable_mlib" = "no" ; then
  AC_MSG_NOTICE(SUN mlib disabled)
else 
  HAS_MEDIALIB=no
  AC_CHECK_LIB(mlib, mlib_VideoColorJFIFYCC2RGB420_Nearest, HAS_MEDIALIB=yes, HAS_MEDIALIB=no)
  if test $HAS_MEDIALIB = yes; then
    AC_MSG_NOTICE(SUN mlib enabled)
    AC_DEFINE(P_MEDIALIB)
    ENDLDLIBS="-lmlib $ENDLDLIBS"
  else
    AC_MSG_ERROR(SUN mlib not found, 1)
  fi
fi

dnl ########################################################################
dnl look for OpenLDAP (requires SASL)

dnl MSWIN_DISPLAY    openldap,Open LDAP
dnl MSWIN_IF_FEATURE openldap,sasl
dnl MSWIN_CHECK_FILE openldap,include\ldap.h|OpenLDAP,P_LDAP=1
dnl MSWIN_DIR_SYMBOL openldap,LDAP_DIR
dnl MSWIN_CHECK_DIR  openldap,..\openldap\
dnl MSWIN_CHECK_DIR  openldap,..\external\openldap\
dnl MSWIN_CHECK_DIR  openldap,\openldap\
dnl MSWIN_CHECK_DIR  openldap,c:\openldap\

AC_ARG_ENABLE([openldap],
	      AS_HELP_STRING([--disable-openldap],[disable OpenLDAP support]),
	      [enable_openldap=$enableval],
	      [enable_openldap=$default_openldap]
	     )

HAS_OPENLDAP=
if test "$enable_openldap" = "no" ; then
  AC_MSG_NOTICE(OpenLDAP disabled)
else 
  PTLIB_FIND_OPENLDAP([HAS_OPENLDAP=1], [HAS_OPENLDAP=])
  if test "x${HAS_OPENLDAP}" = "x1" ; then
    AC_DEFINE(P_LDAP, 1, "OpenLDAP found")
    ENDLDLIBS="$ENDLDLIBS $OPENLDAP_LIBS"
    STDCCFLAGS="$STDCCFLAGS $OPENLDAP_CFLAGS"
  fi
fi

AC_SUBST(HAS_OPENLDAP)

dnl ########################################################################
dnl look for OpenSSL
dnl check for environment variable, then assume standard system install
dnl this process is made incredibly more complex by the fact that RedHad 9
dnl is completely fscked. The default OpenSSL installation requires Kerberos,
dnl even when Kerberos is not installed, requiring an explicit #define to
dnl make it compile
dnl As if that is not enough, autoconf gets very confused if configure.in
dnl contains a AC_CHECK_HEADER directive that never gets used. So we have 
dnl to ensure that it is always called

dnl MSWIN_DISPLAY    openssl,Open SSL
dnl MSWIN_CHECK_FILE openssl,inc32\openssl\ssl.h,P_OSSL=1
dnl MSWIN_CHECK_FILE openssl,include\openssl\ssl.h,P_OSSL=2
dnl MSWIN_FIND_FILE  openssl,P_OSSL_AES,aes.h,...
dnl MSWIN_DIR_SYMBOL openssl,SSL_DIR
dnl MSWIN_CHECK_DIR  openssl,..\openssl\
dnl MSWIN_CHECK_DIR  openssl,..\external\openssl\
dnl MSWIN_CHECK_DIR  openssl,\openssl\
dnl MSWIN_CHECK_DIR  openssl,\libs\openssl\
dnl MSWIN_CHECK_DIR  openssl,\Program Files\OpenSSL\
dnl MSWIN_CHECK_DIR  openssl,\Program Files (x86)\OpenSSL\
dnl MSWIN_CHECK_DIR  openssl,c:\openssl\
dnl MSWIN_CHECK_DIR  openssl,c:\OpenSSL-Win32
dnl MSWIN_CHECK_DIR  openssl,c:\OpenSSL-Win64

AC_ARG_ENABLE([openssl],
	      AS_HELP_STRING([--disable-openssl],[disable OpenSSL support]),
	      [enable_openssl=$enableval],
	      [enable_openssl=$default_openssl]
	     )

if test "$enable_openssl" = "no" ; then
  AC_MSG_NOTICE(disabled)
else
  PKG_CHECK_MODULES([OPENSSL], 
                    [openssl], 
                    [
                     HAS_OPENSSL=1
                     AC_DEFINE(P_OSSL, 1, "OpenSSL found")
                     PTLIB_CHECK_OPENSSL_AES([
                                              AC_DEFINE([P_OSSL_AES], [1],[openssl with AES yes])
                                              HAS_OPENSSL_AES=1
                                            ])
                     PTLIB_OPENSSL_CONST( [AC_DEFINE([P_SSL_USE_CONST], [1], [SSL with const])] )
                     STDCCFLAGS="$STDCCFLAGS ${OPENSSL_CFLAGS}"
                     ENDLDLIBS="$ENDLDLIBS ${OPENSSL_LIBS}"
dnl                      PKG_REQUIRES="$PKG_REQUIRES ssl"
                    ], 
                    [HAS_OPENSSL=])
fi

AC_SUBST(HAS_OPENSSL)

dnl For now...
AC_SUBST(P_STATIC_ENDLDLIBS)
AC_SUBST(P_STATIC_LDFLAGS)



dnl ########################################################################
dnl check for variants of d2i_DHparams_bio from OpenSSL


AC_MSG_CHECKING(for d2i_DHparams_bio variants)
AC_TRY_COMPILE([#include <openssl/dh.h>
                #include <openssl/asn1.h>],
               [ BIO * in; d2i_DHparams_bio(in, NULL);],,P_d2i_DHparams_bio_OLD=1)
if test "x$P_d2i_DHparams_bio_OLD" = "x1" ; then
  AC_MSG_RESULT(old)
  AC_DEFINE(P_d2i_DHparams_bio_OLD, 1)
else
  AC_MSG_RESULT(new)
fi



dnl ########################################################################
dnl look for expat XML parser

dnl MSWIN_DISPLAY    expat,Expat XML
dnl MSWIN_CHECK_FILE expat,lib\expat.h,P_WINEXPAT=1
dnl MSWIN_CHECK_FILE expat,Source\lib\expat.h,P_WINEXPAT=2
dnl MSWIN_DIR_SYMBOL expat,EXPAT_DIR
dnl MSWIN_FIND_FILE  expat,P_EXPAT_LIBRARY_W32_DEBUG,expat.lib,Debug
dnl MSWIN_FIND_FILE  expat,P_EXPAT_LIBRARY_W32_RELEASE,expat.lib,Release
dnl MSWIN_FIND_FILE  expat,P_EXPAT_LIBRARY_W32_DEBUG,libexpat.lib,win32/bin/Debug
dnl MSWIN_FIND_FILE  expat,P_EXPAT_LIBRARY_W32_RELEASE,libexpat.lib,win32/bin/Release
dnl MSWIN_FIND_FILE  expat,P_EXPAT_LIBRARY_W32_DEBUG,libexpatMT.lib,win32/bin/Debug
dnl MSWIN_FIND_FILE  expat,P_EXPAT_LIBRARY_W32_RELEASE,libexpatMT.lib,win32/bin/Release
dnl MSWIN_FIND_FILE  expat,P_EXPAT_LIBRARY_WM5_DEBUG,expat.lib,wm5/Debug
dnl MSWIN_FIND_FILE  expat,P_EXPAT_LIBRARY_WM5_RELEASE,expat.lib,wm5/Release
dnl MSWIN_FIND_FILE  expat,P_EXPAT_LIBRARY_WM6_DEBUG,expat.lib,wm6/Debug
dnl MSWIN_FIND_FILE  expat,P_EXPAT_LIBRARY_WM6_RELEASE,expat.lib,wm6/Release
dnl MSWIN_FIND_FILE  expat,P_EXPAT_LIBRARY_V1,libexpat.lib,lib
dnl MSWIN_FIND_FILE  expat,P_EXPAT_LIBRARY_V2,libexpatMT.lib,bin
dnl MSWIN_CHECK_DIR  expat,..\expat\
dnl MSWIN_CHECK_DIR  expat,..\external\expat\
dnl MSWIN_CHECK_DIR  expat,\expat
dnl MSWIN_CHECK_DIR  expat,c:\expat\
dnl MSWIN_CHECK_DIR  expat,\libs\expat\
dnl MSWIN_CHECK_DIR  expat,\Program Files\Expat 2.*
dnl MSWIN_CHECK_DIR  expat,\Program Files (x86)\Expat 2.*

AC_ARG_ENABLE([expat],
	      AS_HELP_STRING([--disable-expat],[disable expat XML support]),
	      [enable_expat=$enableval],
	      [enable_expat=$default_expat]
	     )


if test "$enable_expat" = "no" ; then
  AC_MSG_NOTICE(Expat disabled)
else
  PTLIB_FIND_EXPAT([HAS_EXPAT=1], [HAS_EXPAT=])

  if test "x${HAS_EXPAT}" = "x1" ; then
    AC_DEFINE(P_OEXPAT, 1, "expat found")
    STDCCFLAGS="$STDCCFLAGS $EXPAT_CFLAGS"
    ENDLDLIBS="$ENDLDLIBS $EXPAT_LIBS"
  fi
fi
AC_SUBST(HAS_EXPAT)


dnl ########################################################################
dnl look for Lua library
dnl MSWIN_DISPLAY    lua,Lua interpreter
dnl MSWIN_CHECK_FILE lua,include\lua.h,P_LUA=1
dnl MSWIN_DIR_SYMBOL lua,LUA_DIR
dnl MSWIN_CHECK_DIR  lua,\Program Files\Lua\5.1
dnl MSWIN_CHECK_DIR  lua,\Program Files (x86)\Lua\5.1

AC_ARG_ENABLE([lua],
	      AS_HELP_STRING([--disable-lua],[disable Lua script support]),
	      [enable_lua=$enableval],
	      [enable_lua=$default_lua]
	     )

if test "$enable_lua" = "no" ; then
  AC_MSG_NOTICE(Lua disabled)
else
  PTLIB_FIND_LUA([HAS_LUA=1], [HAS_LUA=])

  if test "x${HAS_LUA}" = "x1" ; then
    AC_DEFINE(P_LUA, 1, "Lua found")
    STDCCFLAGS="$STDCCFLAGS $LUA_CFLAGS"
    ENDLDLIBS="$ENDLDLIBS $LUA_LIBS"
  fi
fi
AC_SUBST(HAS_LUA)




dnl ########################################################################
dnl look for IPV6 functions

dnl MSWIN_DISPLAY    ipv6,IPv6 Support
dnl MSWIN_CHECK_FILE ipv6,include\ws2tcpip.h,P_HAS_IPV6=1

AC_ARG_ENABLE(ipv6, AS_HELP_STRING([--enable-ipv6],[enable IPV6 support]))

if test "${enable_ipv6}z" = "z" ; then
  enable_ipv6=$default_ipv6
fi

HAS_IPV6=

if test "$enable_ipv6" = "no" ; then
  AC_MSG_NOTICE(IPV6 disabled)
else
  AC_MSG_CHECKING(for IPv6 support)
  AC_TRY_COMPILE([#include <sys/types.h>
                  #include <netinet/in.h>],
                  [struct sockaddr_in6 s; s.sin6_scope_id = 0;], HAS_IPV6=yes, HAS_IPV6=no)
  if test $HAS_IPV6 = no ; then
    AC_MSG_RESULT(no)
  else
    AC_TRY_COMPILE([#include <sys/socket.h>
                     #include <netdb.h>],
                     [getnameinfo(0, 0, 0, 0, 0, 0, 0);], HAS_IPV6=yes, HAS_IPV6=no)
    if test $HAS_IPV6 = no ; then
      AC_MSG_RESULT(no)
    else
      AC_MSG_RESULT(yes)
      AC_DEFINE(P_HAS_IPV6, 1, "IPV6 enabled")
      HAS_IPV6=1
    fi
  fi
fi
AC_SUBST(HAS_IPV6)

HAS_INET_NTOP=
AC_MSG_CHECKING(for inet_ntop)
AC_TRY_COMPILE([#include <sys/types.h>
                #include <netinet/in.h>
                #include <arpa/inet.h>],
                [int af; void * src; char * dst; socklen_t len; inet_ntop(af, src, dst, len);], HAS_INET_NTOP=yes, HAS_INET_NTOP=no)
if test $HAS_INET_NTOP = no ; then
  AC_MSG_RESULT(no)
else
  AC_DEFINE(P_HAS_INET_NTOP, 1)
  AC_MSG_RESULT(yes)
fi

dnl ########################################################################
dnl check for enabling config file support
dnl MSWIN_DISPLAY    configfile,Config file protocol support
dnl MSWIN_DEFINE     configfile,P_CONFIG_FILE

PTLIB_SIMPLE_OPTION([configfile], [CONFIG_FILE], [enable config file support])

dnl ########################################################################
dnl check for URL support
dnl MSWIN_DISPLAY    url,URL Support
dnl MSWIN_DEFINE     url,P_URL

PTLIB_SIMPLE_OPTION([url], [URL], [enable URL support])

dnl ########################################################################
dnl check for HTTP support
dnl MSWIN_DISPLAY    http,HTTP Support
dnl MSWIN_DEFINE     http,P_HTTP
dnl MSWIN_IF_FEATURE http,url

PTLIB_SIMPLE_OPTION([http], [HTTP], [enable HTTP support], [HAS_URL])

dnl ########################################################################
dnl check for HTTPFORMS support
dnl MSWIN_DISPLAY    httpforms,HTTP forms Support
dnl MSWIN_DEFINE     httpforms,P_HTTPFORMS
dnl MSWIN_IF_FEATURE httpforms,http&configfile

PTLIB_SIMPLE_OPTION([httpforms], [HTTPFORMS], [enable HTTP forms support], [HAS_HTTP], [HAS_CONFIG_FILE])


dnl ########################################################################
dnl check for HTTPSVC support
dnl MSWIN_DISPLAY    httpsvc,HTTP service Support
dnl MSWIN_DEFINE     httpsvc,P_HTTPSVC
dnl MSWIN_IF_FEATURE httpsvc,httpforms

PTLIB_SIMPLE_OPTION([httpsvc], [HTTPSVC], [enable HTTP server support], [HAS_HTTPFORMS])

dnl ########################################################################
dnl Jabber
dnl MSWIN_DISPLAY    jabber,Jabber
dnl MSWIN_DEFINE     jabber,P_JABBER
dnl MSWIN_IF_FEATURE jabber,expat

PTLIB_SIMPLE_OPTION([jabber], [JABBER], [enable Jabber support], [HAS_EXPAT])

dnl ########################################################################
dnl VXML 
dnl MSWIN_DISPLAY    vxml,VXML Support
dnl MSWIN_DEFINE     vxml,P_VXML
dnl MSWIN_IF_FEATURE vxml,http&expat

PTLIB_SIMPLE_OPTION([vxml], [VXML], [enable VXML support], [HAS_EXPAT], [HAS_HTTP])

dnl ########################################################################
dnl XMLRPC
dnl MSWIN_DISPLAY    xmlrpc,XMLRPC Support
dnl MSWIN_DEFINE     xmlrpc,P_XMLRPC
dnl MSWIN_IF_FEATURE xmlrpc,http&expat

PTLIB_SIMPLE_OPTION([xmlrpc], [XMLRPC], [enable XMLRPC support], [HAS_EXPAT], [HAS_HTTP])

dnl ########################################################################
dnl SOAP
dnl MSWIN_DISPLAY    soap,SOAP Support
dnl MSWIN_DEFINE     soap,P_SOAP
dnl MSWIN_IF_FEATURE soap,http&expat

PTLIB_SIMPLE_OPTION([soap], [SOAP], [enable SOAP support], [HAS_EXPAT], [HAS_HTTP])

dnl ########################################################################
dnl MSWIN_DISPLAY       atl,Active Template Library
dnl MSWIN_CHECK_FILE    atl,atlbase.h,P_ATL=1

dnl ########################################################################
dnl Speech
dnl MSWIN_DISPLAY       sapi,Speech API
dnl MSWIN_CHECK_FILE    sapi,include\sphelper.h,P_SAPI=1
dnl MSWIN_DEFINE        sapi,P_TTS


PTLIB_SIMPLE_OPTION([tts], [TTS], [enable Text-To-Speech server support])

dnl ########################################################################
dnl check for enabling ASN support
dnl MSWIN_DISPLAY    asn,ASN Support
dnl MSWIN_DEFINE     asn,P_ASN

PTLIB_SIMPLE_OPTION([asn], [ASN], [enable ASN decoding/encoding support])

dnl ########################################################################
dnl check for enabling STUN support
dnl MSWIN_DISPLAY    stun,STUN Support
dnl MSWIN_DEFINE     stun,P_STUN

PTLIB_SIMPLE_OPTION([stun], [STUN], [enable STUN support])

dnl ########################################################################
dnl check for enabling PIPECHAN support
dnl MSWIN_DISPLAY    pipechan,pipe channel Support
dnl MSWIN_DEFINE     pipechan,P_PIPECHAN

PTLIB_SIMPLE_OPTION([pipechan], [PIPECHAN], [enable pipechannel support])

dnl ########################################################################
dnl check for enabling DTMF support
dnl MSWIN_DISPLAY  dtmf,DTMF encoding/decoding support
dnl MSWIN_DEFINE   dtmf,P_DTMF

PTLIB_SIMPLE_OPTION([dtmf], [DTMF], [enable DTMF protocol support])

dnl ########################################################################
dnl check for enabling SOCKS support
dnl MSWIN_DISPLAY    socks,SOCKS protocol support
dnl MSWIN_DEFINE     socks,P_SOCKS

PTLIB_SIMPLE_OPTION([socks], [SOCKS], [enable SOCKS protocol support])

dnl ########################################################################
dnl check for enabling FTP support
dnl MSWIN_DISPLAY    ftp,FTP protocol support
dnl MSWIN_DEFINE     ftp,P_FTP

PTLIB_SIMPLE_OPTION([ftp], [FTP], [enable FTP protocol support])

dnl ########################################################################
dnl check for enabling SNMP support
dnl MSWIN_DISPLAY    snmp,SNMP protocol support
dnl MSWIN_DEFINE     snmp,P_SNMP

PTLIB_SIMPLE_OPTION([snmp], [SNMP], [enable SNMP protocol support])

dnl ########################################################################
dnl check for enabling TELNET support
dnl MSWIN_DISPLAY    telnet,Telnet protocol support
dnl MSWIN_DEFINE     telnet,P_TELNET

PTLIB_SIMPLE_OPTION([telnet], [TELNET], [enable TELNET protocol support])

dnl ########################################################################
dnl check for enabling REMCONN support
dnl MSWIN_DISPLAY    remconn,remote connection support
dnl MSWIN_DEFINE     remconn,P_REMCONN

PTLIB_SIMPLE_OPTION([remconn], [REMCONN], [enable remote connection support], [HAS_CONFIG_FILE])

dnl ########################################################################
dnl check for enabling serial port handling
dnl MSWIN_DISPLAY    serial,Serial Support
dnl MSWIN_DEFINE     serial,P_SERIAL

PTLIB_SIMPLE_OPTION([serial], [SERIAL], [enable serial port support])

dnl ########################################################################
dnl check for POP3/SMTP support
dnl MSWIN_DISPLAY    pop3smtp,POP3/SMTP Support
dnl MSWIN_DEFINE     pop3smtp,P_POP3SMTP

PTLIB_SIMPLE_OPTION([pop3smtp], [POP3SMTP], [enable POP3/SMTP support])

dnl ########################################################################
dnl check for sound support
dnl MSWIN_DISPLAY audio,Sound support
dnl MSWIN_DEFINE  audio,P_AUDIO

PTLIB_SIMPLE_OPTION([audio], [AUDIO], [enable audio support])

dnl ########################################################################
dnl check for enabling WAVFILE support
dnl MSWIN_DISPLAY    wavfile,WAV file support
dnl MSWIN_DEFINE     wavfile,P_WAVFILE

PTLIB_SIMPLE_OPTION([wavfile], [WAVFILE], [enable WAVFILE support], [HAS_AUDIO])


dnl ########################################################################
dnl look for QoS functions

dnl MSWIN_DISPLAY    qos,QoS Support
dnl MSWIN_CHECK_FILE qos,include\qossp.h|QOS_DESTADDR,P_QOS=1

PTLIB_SIMPLE_OPTION([qos], [QOS], [enable QOS support])

dnl ########################################################################
dnl check for video support
dnl MSWIN_DISPLAY    video,Video Support
dnl MSWIN_DEFINE     video,P_VIDEO

PTLIB_SIMPLE_OPTION([video], [VIDEO], [enable video device support])
if test "x${HAS_VIDEO}" != "x1" ; then
  AC_DEFINE([NO_VIDEO_CAPTURE], [], [No video capture])
fi

dnl ########################################################################
dnl check for enabling video file support
dnl MSWIN_DISPLAY    vidfile,Video file support
dnl MSWIN_DEFINE     vidfile,P_VIDFILE

PTLIB_SIMPLE_OPTION([vidfile], [VIDFILE], [enable video file support], [HAS_VIDEO])

dnl ########################################################################
dnl check for enabling FFMpeg video file support
dnl MSWIN_DISPLAY    ffvdev,FFMpeg video file support
dnl MSWIN_DEFINE     ffvdev,P_FFVDEV

PTLIB_SIMPLE_OPTION([ffvdev], [FFVDEV], [enable FFMpeg video file support], [HAS_FFVDEV], [HAS_PIPECHAN])

dnl ########################################################################
dnl check for enabling app share support
dnl MSWIN_DISPLAY    appshare,Application share support
dnl MSWIN_DEFINE     appshare,P_APPSHARE

PTLIB_SIMPLE_OPTION([appshare], [APPSHARE], [enable application sharing support], [HAS_VIDEO])

dnl ########################################################################
dnl enable shared memory video devices (static video plugin)

PTLIB_SIMPLE_OPTION([shmvideo], [SHM_VIDEO], [enable shared memory video devices])

dnl ########################################################################
dnl look for SDL, but only if we have pthreads

dnl MSWIN_DISPLAY    libsdl,Simple DirectMedia Layer
dnl MSWIN_CHECK_FILE libsdl,include\SDL.h,P_SDL=1
dnl MSWIN_CHECK_FILE libsdl,include\SDL\SDL.h,P_SDL=2
dnl MSWIN_DIR_SYMBOL libsdl,SDL_DIR
dnl MSWIN_FIND_FILE  libsdl,P_SDL_LIBRARY,SDL.lib,lib
dnl MSWIN_CHECK_DIR  libsdl,..\sdl*
dnl MSWIN_CHECK_DIR  libsdl,..\external\sdl*
dnl MSWIN_CHECK_DIR  libsdl,\sdl\
dnl MSWIN_CHECK_DIR  libsdl,c:\sdl\
dnl MSWIN_CHECK_DIR  libsdl,\Program Files (x86)\OSSBuild\GStreamer\v0.10.6\sdk

AC_ARG_ENABLE([sdl],
	      AS_HELP_STRING([--disable-sdl],[disable SDL support]),
	      [enable_sdl=$enableval],
	      [enable_sdl=$default_sdl]
	     )

HAS_SDL=
if test "x${HAS_VIDEO}" != "x1" ; then
    AC_MSG_NOTICE(SDL disabled due to disabled dependency HAS_VIDEO)
else
  if test "$enable_sdl" = "no" ; then
    AC_MSG_NOTICE(SDL disabled)
  else
    PKG_CHECK_MODULES([SDL], 
                      [sdl], 
                      [
                       HAS_SDL=1
                       AC_DEFINE(P_SDL, 1, "SDL found")
                       STDCCFLAGS="$STDCCFLAGS ${SDL_CFLAGS}"
                       LDFLAGS="$LDFLAGS `pkg-config --libs-only-L sdl`"
                       ENDLDLIBS="$ENDLDLIBS `pkg-config --libs-only-l sdl`"
                      ],[HAS_SDL=])
  fi
fi
AC_SUBST(HAS_SDL)


dnl ########################################################################
dnl check for video capture support.
dnl videodev.h on linux
dnl machine/ioctl_meteor on FreeBSD
dnl i386/ioctl_meteor on OpenBSD
dnl dev/ic/bt8xx on NetBSD

HAS_VIDEO_CAPTURE=
if test "x${HAS_VIDEO}" = "x1" ; then
  if test "x${HAS_PLUGINS}" = "x1" ; then
    HAS_VIDEO_CAPTURE=1
  fi
  case "$target_os" in
    cygwin* | mingw* )
  	  PTLIB_FIND_DIRECTX([HAS_VIDEO_CAPTURE=1
	 	              ENDLDLIBS="$ENDLDLIBS ${DIRECTX_LIBS}"
			      STDCCFLAGS="$STDCCFLAGS ${DIRECTX_INCLUDES}"
                              HAS_DIRECTSHOW=1
                              HAS_DIRECTSOUND=1
			      AC_DEFINE(P_DIRECTSHOW, 1)
			      AC_DEFINE(P_DIRECTX, 1)
			      AC_DEFINE(P_DIRECTSOUND, 1)
		             ])
		;;
  esac

  if test "${HAS_VIDEO_CAPTURE}z" = "z" ; then
    AC_MSG_NOTICE(No Video Capture support)
    AC_DEFINE([NO_VIDEO_CAPTURE], [], [No video capture])
  fi

  dnl these are needed to pass through to plugin configure
  AC_ARG_ENABLE(v4l,      [  --enable-v4l            enable V4L video support])
  AC_ARG_ENABLE(v4l2,     [  --enable-v4l2           enable V4L2 video support])
  AC_ARG_ENABLE(bsdvideo, [  --enable-bsdvideo       enable BSD video support])
  AC_ARG_ENABLE(avc,      [  --enable-avc            enable IEEE1394/AVC video support])
  AC_ARG_ENABLE(dc,       [  --enable-dc             enable IEEE1394/DC video support])
fi

AC_SUBST(HAS_DIRECTSHOW)
AC_SUBST(HAS_DIRECTSOUND)
AC_SUBST(HAS_VIDEO_CAPTURE)

PTLIB_SIMPLE_OPTION([vfw], [VFW_CAPTURE], [enable video for windows capture], [HAS_VIDEO])

if test "x${HAS_AUDIO}" = "x1" ; then

  dnl these are needed to pass through to the plugin configure program
  AC_ARG_ENABLE(alsa,  [  --enable-alsa           enable ALSA audio support],enable_alsa=$enableval)
  AC_ARG_ENABLE(oss,   [  --enable-oss            enable OSS audio support],enable_oss=$enableval)
  AC_ARG_ENABLE(pulse, [  --enable-pulse          enable PULSE audio support],enable_pulse=$enableval)
fi



dnl ########################################################################
dnl look for Direct Sound

dnl MSWIN_DISPLAY    dsound,Direct Sound Support
dnl MSWIN_CHECK_FILE dsound,include\dsound.h,P_DIRECTSOUND=1
dnl MSWIN_FIND_FILE  dsound,P_DIRECTSOUND_DXERR_H,dxerr.h,include
dnl MSWIN_FIND_FILE  dsound,P_DIRECTSOUND_LIB,dsound.lib,lib


dnl ########################################################################
dnl look for Direct Show (video)

dnl MSWIN_DISPLAY    dshow,Direct Show Support
dnl MSWIN_CHECK_FILE dshow,include\dshow.h,P_DIRECTSHOW=1
dnl MSWIN_FIND_FILE  dshow,P_DIRECTSHOW_QEDIT_H,Qedit.h,include


dnl ########################################################################
dnl look for MEMORY_CHECK enabled.

AC_ARG_ENABLE(memcheck,
       AS_HELP_STRING([--enable-memcheck],[enable leak testing code (off by default)]),
       memcheck=$enableval)

if test "$memcheck" = "yes" ; then
  AC_DEFINE(PMEMORY_CHECK, 1)
  AC_MSG_NOTICE(Memory checking enabled)
fi


dnl ########################################################################
dnl look for ODBC code

dnl MSWIN_DISPLAY    odbc,ODBC support
dnl MSWIN_CHECK_FILE odbc,include\odbcinst.h,P_ODBC=1

AC_ARG_ENABLE([odbc],
	      AS_HELP_STRING([--disable-odbc],[disable ODBC support]),
	      [enable_odbc=$enableval],
	      [enable_odbc=$default_odbc]
	     )

if test "$enable_odbc" = "no" ; then
  AC_MSG_NOTICE(ODBC disabled)
else
  PTLIB_FIND_ODBC([HAS_ODBC=1], [HAS_ODBC=])

  if test "x${HAS_ODBC}" = "x1" ; then
    AC_DEFINE(P_ODBC, 1, "ODBC found")
    STDCCFLAGS="$STDCCFLAGS $ODBC_CFLAGS"
    ENDLDLIBS="$ENDLDLIBS $ODBC_LIBS"
    AC_MSG_NOTICE(ODBC support enabled)
  fi
fi
AC_SUBST(HAS_ODBC)

dnl ########################################################################
dnl output make directives

dnl resolver needs to be at the end
if test "${HAS_RESOLVER:-unset}" != "unset" ; then
    ENDLDLIBS="$ENDLDLIBS ${RESOLVER_LIBS}"
dnl     PKG_LIBS="$PKG_LIBS ${RESOLVER_LIBS}"
fi

dnl dl needs to be at the end
if test "x${HAS_LIBDL}" = "x1" ; then
  ENDLDLIBS="$ENDLDLIBS $DL_LIBS"
dnl   PKG_LIBS="$PKG_LIBS $DL_LIBS"
  STDCCFLAGS="$STDCCFLAGS $DL_CFLAGS"
dnl   PKG_CFLAGS="$PKG_CFLAGS $DL_CFLAGS"
fi


dnl ########################################################################
dnl this reduces the size of all executables and libraries

case "$target_os" in
  solaris* | sunos* ) ;;
  *)
    PTLIB_SIMPLE_OPTION([exceptions], [EXCEPTIONS], [enable C++ exceptions])

    if test "x${HAS_EXCEPTIONS}" = "x1" ; then
      STDCCFLAGS="$STDCCFLAGS -fexceptions"
    else
      STDCCFLAGS="$STDCCFLAGS -fno-exceptions"
    fi
    ;;
esac


dnl Forward compatibility for pkg-config options
AC_DEFUN([PTLIB_NEW_OPTION],
         [
          if test "x${HAS_$1}" = "x1" ; then
            AC_SUBST([PTLIB_$1], [yes])
          else
            AC_SUBST([PTLIB_$1], [no])
          fi
         ]
        )

PTLIB_NEW_OPTION(IPV6)
PTLIB_NEW_OPTION(RESOLVER)
PTLIB_NEW_OPTION(OPENSSL)
PTLIB_NEW_OPTION(OPENSSL_AES)
PTLIB_NEW_OPTION(OPENLDAP)
PTLIB_NEW_OPTION(SASL)
PTLIB_NEW_OPTION(SASL2)
PTLIB_NEW_OPTION(EXPAT)
PTLIB_NEW_OPTION(REGEX)
PTLIB_NEW_OPTION(SDL)
PTLIB_NEW_OPTION(VIDEO_CAPTURE)

PTLIB_NEW_OPTION(TTS)
PTLIB_NEW_OPTION(ASN)
PTLIB_NEW_OPTION(STUN)
PTLIB_NEW_OPTION(PIPECHAN)
PTLIB_NEW_OPTION(DTMF)
PTLIB_NEW_OPTION(WAVFILE)
PTLIB_NEW_OPTION(SOCKS)
PTLIB_NEW_OPTION(FTP)
PTLIB_NEW_OPTION(SNMP)
PTLIB_NEW_OPTION(TELNET)
PTLIB_NEW_OPTION(REMCONN)
PTLIB_NEW_OPTION(SERIAL)
PTLIB_NEW_OPTION(POP3SMTP)
PTLIB_NEW_OPTION(AUDIO)
PTLIB_NEW_OPTION(VIDEO)
PTLIB_NEW_OPTION(SHM_VIDEO)

PTLIB_NEW_OPTION(VXML)
PTLIB_NEW_OPTION(JABBER)
PTLIB_NEW_OPTION(XMLRPC)
PTLIB_NEW_OPTION(SOAP)
PTLIB_NEW_OPTION(URL)
PTLIB_NEW_OPTION(HTTP)
PTLIB_NEW_OPTION(HTTPFORMS)
PTLIB_NEW_OPTION(HTTPSVC)
PTLIB_NEW_OPTION(CONFIG_FILE)
PTLIB_NEW_OPTION(VIDFILE)
PTLIB_NEW_OPTION(FFVDEV)
PTLIB_NEW_OPTION(ODBC)

dnl /Forward compatibility for pkg-config options

dnl ########################################################################
dnl final declarations

AC_SUBST(CC)
AC_SUBST(CXX)
AC_SUBST(STDCCFLAGS)
AC_SUBST(STDCXXFLAGS)
AC_SUBST(OPTSTDCCFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(ENDLDLIBS)
AC_SUBST(DEBUG_FLAG)
AC_SUBST(OSTYPE)
AC_SUBST(MACHTYPE)
AC_PROG_INSTALL

AC_CONFIG_FILES(make/ptbuildopts.mak)
AC_CONFIG_FILES([make/ptlib-config], [chmod a+x make/ptlib-config])
AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(ptlib_cfg.dxy)
AC_CONFIG_FILES(ptlib.pc)

dnl ########################################################################
dnl output header file
AC_CONFIG_HEADERS(include/ptbuildopts.h)

AC_OUTPUT()
