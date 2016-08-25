#
# unix.mak
#
# First part of make rules, included in ptlib.mak and pwlib.mak.
# Note: Do not put any targets in the file. This should defaine variables
#       only, as targets are all in common.mak
#
# Portable Windows Library
#
# Copyright (c) 1993-1998 Equivalence Pty. Ltd.
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and limitations
# under the License.
#
# The Original Code is Portable Windows Library.
#
# The Initial Developer of the Original Code is Equivalence Pty. Ltd.
#
# Portions are Copyright (C) 1993 Free Software Foundation, Inc.
# All Rights Reserved.
# 
# Contributor(s): ______________________________________.
#
# $Revision$
# $Author$
# $Date$
#

ifndef PTLIBDIR
	echo "No PTLIBDIR environment variable defined!"
	echo "You need to define PTLIBDIR!"
	echo "Try something like:"
	echo "PTLIBDIR = $(HOME)/ptlib"
	exit 1
endif

####################################################

# include generated build options file, then include it
include $(PTLIBDIR)/make/ptbuildopts.mak

STANDARD_TARGETS=\
opt         debug         both \
optdepend   debugdepend   bothdepend \
optshared   debugshared   bothshared \
optnoshared debugnoshared bothnoshared \
optclean    debugclean    clean \
release tagbuild

.PHONY: all $(STANDARD_TARGETS)


ifeq (,$(findstring $(OSTYPE),linux gnu FreeBSD OpenBSD NetBSD solaris beos Darwin Carbon AIX Nucleus VxWorks rtems QNX cygwin mingw))

default_target :
	@echo
	@echo ######################################################################
	@echo "Warning: OSTYPE=$(OSTYPE) support has not been confirmed.  This may"
	@echo "         be a new operating system not yet encountered, or more"
	@echo "         likely, the OSTYPE and MACHTYPE environment variables are"
	@echo "         set to unusual values. You may need to explicitly set these"
	@echo "         variables for the correct operation of this system."
	@echo
	@echo "         Currently supported OSTYPE names are:"
	@echo "              linux Linux linux-gnu mklinux"
	@echo "              gnu solaris Solaris SunOS"
	@echo "              FreeBSD OpenBSD NetBSD beos Darwin Carbon"
	@echo "              VxWorks rtems mingw"
	@echo
	@echo "              **********************************"
	@echo "              *** DO NOT IGNORE THIS MESSAGE ***"
	@echo "              **********************************"
	@echo
	@echo "         The system almost certainly will not compile! When you get"
	@echo "         it working please send patches to support@equival.com.au"
	@echo ######################################################################
	@echo

$(STANDARD_TARGETS) :: default_target

endif

####################################################

# Set default for shared library usage

ifndef P_SHAREDLIB
P_SHAREDLIB=1
endif

# -Wall must be at the start of the options otherwise
# any -W overrides won't have any effect
ifeq ($(USE_GCC),yes)
STDCCFLAGS += -Wall 
endif

ifdef RPM_OPT_FLAGS
STDCCFLAGS	+= $(RPM_OPT_FLAGS)
endif

ifneq ($(OSTYPE),rtems)
ifndef SYSINCDIR
SYSINCDIR := /usr/include
endif
endif


# Empty LD so getys set by appropriate platform below
LD=

####################################################

STDCCFLAGS += -Wformat -Wformat-security -D_FORTIFY_SOURCE=2 

ifeq ($(OSTYPE),linux)

ifeq ($(MACHTYPE),x86)
ifdef CPUTYPE
ifeq ($(CPUTYPE),crusoe)
STDCCFLAGS	+= -fomit-frame-pointer -fno-strict-aliasing -fno-common -pipe -mpreferred-stack-boundary=2 -march=i686 -malign-functions=0 
STDCCFLAGS      += -malign-jumps=0 -malign-loops=0
else
STDCCFLAGS	+= -mcpu=$(CPUTYPE)
endif
endif
endif

ifeq ($(MACHTYPE),ia64)
STDCCFLAGS     += -DP_64BIT
endif

ifeq ($(MACHTYPE),hppa64)
STDCCFLAGS     += -DP_64BIT
endif

ifeq ($(MACHTYPE),s390x)
STDCCFLAGS     += -DP_64BIT
endif

ifeq ($(MACHTYPE),x86_64)
STDCCFLAGS     += -DP_64BIT
LDLIBS		+= -lresolv
endif

ifeq ($(MACHTYPE),ppc64)
STDCCFLAGS     += -DP_64BIT
endif

ifeq ($(P_SHAREDLIB),1)
ifndef PROG
STDCCFLAGS	+= -fPIC -DPIC
endif # PROG
endif # P_SHAREDLIB


STATIC_LIBS	:= libstdc++.a libg++.a libm.a libc.a
SYSLIBDIR	:= $(shell $(PTLIBDIR)/make/ptlib-config --libdir)

endif # linux

####################################################

ifeq ($(OSTYPE),gnu)

ifeq ($(MACHTYPE),x86)
ifdef CPUTYPE
ifeq ($(CPUTYPE),crusoe)
STDCCFLAGS	+= -fomit-frame-pointer -fno-strict-aliasing -fno-common -pipe -mpreferred-stack-boundary=2 -march=i686 -malign-functions=0
STDCCFLAGS      += -malign-jumps=0 -malign-loops=0
else
STDCCFLAGS	+= -mcpu=$(CPUTYPE)
endif
endif
endif

ifeq ($(MACHTYPE),ia64)
STDCCFLAGS     += -DP_64BIT
endif

ifeq ($(MACHTYPE),x86_64)
STDCCFLAGS     += -DP_64BIT
LDLIBS		+= -lresolv
endif

ifeq ($(P_SHAREDLIB),1)
ifndef PROG
STDCCFLAGS	+= -fPIC -DPIC
endif # PROG
endif # P_SHAREDLIB


STATIC_LIBS	:= libstdc++.a libg++.a libm.a libc.a
SYSLIBDIR	:= $(shell $(PTLIBDIR)/make/ptlib-config --libdir)

endif # gnu

####################################################

ifeq ($(OSTYPE),FreeBSD)

ifeq ($(MACHTYPE),x86)
ifdef CPUTYPE
STDCCFLAGS	+= -mcpu=$(CPUTYPE)
endif
endif

ifeq ($(MACHTYPE),amd64)
STDCCFLAGS     += -DP_64BIT
endif

P_USE_RANLIB		:= 1
#STDCCFLAGS      += -DP_USE_PRAGMA		# migrated to configure

ifeq ($(P_SHAREDLIB),1)
ifndef PROG
STDCCFLAGS	+= -fPIC -DPIC
endif # PROG
endif # P_SHAREDLIB

endif # FreeBSD


####################################################

ifeq ($(OSTYPE),OpenBSD)

ifeq ($(MACHTYPE),x86)
#STDCCFLAGS	+= -m486
endif

LDLIBS		+= -lossaudio

P_USE_RANLIB		:= 1
#STDCCFLAGS      += -DP_USE_PRAGMA		# migrated to configure


endif # OpenBSD


####################################################

ifeq ($(OSTYPE),NetBSD)

ifeq ($(MACHTYPE),x86)
ifdef CPUTYPE
STDCCFLAGS	+= -mcpu=$(CPUTYPE)
endif
endif

ifeq ($(MACHTYPE),x86_64)
STDCCFLAGS	+= -DP_64BIT
endif

P_USE_RANLIB		:= 1
#STDCCFLAGS      += -DP_USE_PRAGMA		# migrated to configure

ifndef PROG
STDCCFLAGS	+= -fPIC -DPIC
endif # PROG

endif # NetBSD


####################################################

ifeq ($(OSTYPE),AIX)

STDCCFLAGS	+= -DP_AIX  
# -pedantic -g
# LDLIBS	+= -lossaudio

STDCCFLAGS	+= -mminimal-toc

#P_USE_RANLIB		:= 1
STDCCFLAGS      += -DP_USE_PRAGMA


endif # AIX


####################################################

ifeq ($(OSTYPE),sunos)

# Sparc Sun 4x, using gcc 2.7.2

P_USE_RANLIB		:= 1
REQUIRES_SEPARATE_SWITCH = 1
#STDCCFLAGS      += -DP_USE_PRAGMA	# migrated to configure

endif # sunos


####################################################

ifeq ($(OSTYPE),solaris)

#  Solaris (Sunos 5.x)

CFLAGS +=-DSOLARIS -D__inline=inline
CXXFLAGS +=-DSOLARIS -D__inline=inline

ifeq ($(MACHTYPE),x86)
ifeq ($(USE_GCC),yes)
DEBUG_FLAG	:= -gstabs+
CFLAGS           += -DUSE_GCC
CXXFLAGS         += -DUSE_GCC
endif
endif

ENDLDLIBS	+= -lsocket -lnsl -ldl -lposix4

# Sparc Solaris 2.x, using gcc 2.x
#Brian CC		:= gcc

#P_USE_RANLIB		:= 1

#STDCCFLAGS      += -DP_USE_PRAGMA	# migrated to configure

STATIC_LIBS	:= libstdc++.a libg++.a 
SYSLIBDIR	:= /opt/openh323/lib

# Rest added by jpd@louisiana.edu, to get .so libs created!
ifndef DEBUG
ifeq ($(P_SHAREDLIB),1)
ifndef PROG
STDCCFLAGS	+= -fPIC -DPIC
endif # PROG
endif
endif

endif # solaris

####################################################

ifeq ($(OSTYPE),irix)

#  should work whith Irix 6.5

# IRIX using a gcc
CC		:= gcc
STDCCFLAGS	+= -DP_IRIX
LDLIBS		+= -lsocket -lnsl

STDCCFLAGS      += -DP_USE_PRAGMA

endif # irix


####################################################

ifeq ($(OSTYPE),beos)

SYSLIBS     += -lbe -lmedia -lgame -lroot -lsocket -lbind -ldl 
STDCCFLAGS	+= -DBE_THREADS -DP_USE_PRAGMA -Wno-multichar -Wno-format
LDLIBS		+= $(SYSLIBS)

MEMORY_CHECK := 0
endif # beos


####################################################

ifeq ($(OSTYPE),ultrix)

# R2000 Ultrix 4.2, using gcc 2.7.x
STDCCFLAGS	+= -DP_ULTRIX
STDCCFLAGS      += -DP_USE_PRAGMA
endif # ultrix


####################################################

ifeq ($(OSTYPE),hpux)
STDCCFLAGS      += -DP_USE_PRAGMA
# HP/UX 9.x, using gcc 2.6.C3 (Cygnus version)
STDCCFLAGS	+= -DP_HPUX9

endif # hpux


####################################################
 
ifeq ($(OSTYPE),Darwin)
 
# MacOS X or later / Darwin

CFLAGS		+= -fno-common -dynamic
LDFLAGS		+= -multiply_defined suppress
ENDLDLIBS	+= -lresolv -framework AudioToolbox -framework CoreAudio
ENDLDLIBS   += -framework AudioUnit -framework CoreServices
STDCCFLAGS  += -D__MACOSX__

# Quicktime support is still a long way off. But for development purposes,
# I am inluding the flags to allow QuickTime to be linked.
# Uncomment them if you wish, but it will do nothing for the time being.

#HAS_QUICKTIMEX := 1
#STDCCFLAGS     += -DHAS_QUICKTIMEX
#ENDLDLIBS      += -framework QuickTime
 
ifeq ($(MACHTYPE),x86)
STDCCFLAGS	+= -m486
endif

ARCHIVE			:= libtool -static -o
P_USE_RANLIB	:= 0

endif # Darwin

ifeq ($(OSTYPE),Carbon)

# MacOS 9 or X using Carbonlib calls

STDCCFLAGS	+= -DP_MACOS

# I'm having no end of trouble with the debug memory allocator.
MEMORY_CHECK    := 0

# Carbon is only available for full Mac OS X, not pure Darwin, so the only
# currently available architecture is PPC.
P_MAC_MPTHREADS := 1
STDCCFLAGS	+= -DP_MAC_MPTHREADS
LDLIBS		+= -prebind -framework CoreServices -framework QuickTime -framework Carbon
  
P_SHAREDLIB	:= 0 
P_USE_RANLIB	:= 1

endif # Carbon

####################################################

ifeq ($(OSTYPE),VxWorks)

ifeq ($(MACHTYPE),ARM)
STDCCFLAGS	+= -mcpu=arm8 -DCPU=ARMARCH4
endif

STDCCFLAGS	+= -DP_VXWORKS -DPHAS_TEMPLATES -DVX_TASKS
STDCCFLAGS	+= -DNO_LONG_DOUBLE

STDCCFLAGS	+= -Wno-multichar -Wno-format

MEMORY_CHECK := 0

STDCCFLAGS      += -DP_USE_PRAGMA

LD		= ld
LDFLAGS		+= --split-by-reloc 65535 -r 

endif # VxWorks

 
####################################################

ifeq ($(OSTYPE),rtems)

SYSLIBDIR	:= $(RTEMS_MAKEFILE_PATH)/lib
SYSINCDIR	:= $(RTEMS_MAKEFILE_PATH)/lib/include

LDFLAGS		+= -B$(SYSLIBDIR)/ -specs=bsp_specs -qrtems
STDCCFLAGS	+= -B$(SYSLIBDIR)/ -specs=bsp_specs -ansi -fasm -qrtems

ifeq ($(CPUTYPE),mcpu32)
STDCCFLAGS	+= -mcpu32
LDFLAGS		+= -mcpu32 
endif

ifeq ($(CPUTYPE),mpc860)
STDCCFLAGS	+= -mcpu=860
LDFLAGS		+= -mcpu=860
endif

STDCCFLAGS	+= -DP_RTEMS -DP_HAS_SEMAPHORES

P_SHAREDLIB	:= 0

endif # rtems

####################################################

ifeq ($(OSTYPE),QNX)

ifeq ($(MACHTYPE),x86)
STDCCFLAGS	+= -Wc,-m486
endif

STDCCFLAGS	+= -DP_QNX -DP_HAS_RECURSIVE_MUTEX=1 -DFD_SETSIZE=1024
LDLIBS		+= -lasound
ENDLDLIBS       += -lsocket -lstdc++

P_USE_RANLIB	:= 1
STDCCFLAGS      += -DP_USE_PRAGMA

ifeq ($(P_SHAREDLIB),1)
ifeq ($(USE_GCC),yes)
STDCCFLAGS      += -shared
else
ifeq ($(OSTYPE),solaris)
STDCCFLAGS      += -G
endif
endif

endif

endif # QNX6

####################################################

ifeq ($(OSTYPE),Nucleus)

# Nucleus using gcc
STDCCFLAGS	+= -msoft-float -nostdinc $(DEBUGFLAGS)
STDCCFLAGS	+= -D__NUCLEUS_PLUS__ -D__ppc -DWOT_NO_FILESYSTEM -DPLUS \
		   -D__HAS_NO_FLOAT -D__USE_STL__ \
                   -D__USE_STD__ \
		   -D__NUCLEUS_NET__ -D__NEWLIB__ \
		   -DP_USE_INLINES=0
ifndef WORK
WORK		= ${HOME}/work
endif
ifndef NUCLEUSDIR
NUCLEUSDIR	= ${WORK}/embedded/os/Nucleus
endif
ifndef STLDIR
STLDIR		= ${WORK}/embedded/packages/stl-3.2-stream
endif
STDCCFLAGS	+= -I$(NUCLEUSDIR)/plus \
		-I$(NUCLEUSDIR)/plusplus \
		-I$(NUCLEUSDIR)/net \
		-I$(NUCLEUSDIR) \
		-I$(PTLIBDIR)/include/ptlib/Nucleus++ \
		-I$(WORK)/embedded/libraries/socketshim/BerkleySockets \
		-I${STLDIR} \
		-I/usr/local/powerpc-motorola-eabi/include \
		-I${WORK}/embedded/libraries/configuration

UNIX_SRC_DIR	= $(PTLIBDIR)/src/ptlib/Nucleus++
MEMORY_CHECK	=	0
endif # Nucleus

####################################################

#ifeq ($(OSTYPE),mingw)
#LDFLAGS += -enable-runtime-pseudo-reloc -fatal-warning
#endif # mingw

###############################################################################
#
# Make sure some things are defined
#

ifndef INSTALL
INSTALL := install
endif

ifndef LD
LD = $(CXX)
endif

ifndef AR
AR := ar
endif

ifndef ARCHIVE
  ifdef P_USE_RANLIB
    ARCHIVE := $(AR) rc
  else
    ARCHIVE := $(AR) rcs
  endif
endif

ifndef RANLIB
RANLIB := ranlib
endif


# Further configuration

ifndef DEBUG_FLAG
DEBUG_FLAG	:=  -g
endif

ifndef PTLIB_ALT
PLATFORM_TYPE = $(OSTYPE)_$(MACHTYPE)
else
PLATFORM_TYPE = $(OSTYPE)_$(PTLIB_ALT)_$(MACHTYPE)
endif

ifndef OBJ_SUFFIX
ifdef	DEBUG
OBJ_SUFFIX	:= _d
else
OBJ_SUFFIX	:=
endif # DEBUG
endif # OBJ_SUFFIX

ifndef STATICLIBEXT
STATICLIBEXT = a
endif

ifeq ($(P_SHAREDLIB),1)
LIB_SUFFIX	= $(SHAREDLIBEXT)
LIB_TYPE	=
else   
LIB_SUFFIX	= a 
LIB_TYPE	= _s
endif # P_SHAREDLIB

ifndef OBJDIR_SUFFIX
OBJDIR_SUFFIX = $(OBJ_SUFFIX)$(LIB_TYPE)
endif

ifndef INSTALL_DIR
INSTALL_DIR	= ${PREFIX}
endif

ifndef INSTALLBIN_DIR
INSTALLBIN_DIR	= $(INSTALL_DIR)/bin
endif

ifndef INSTALLLIB_DIR
INSTALLLIB_DIR	= $(INSTALL_DIR)/lib
endif


###############################################################################
#
# define some common stuff
#

ifneq ($(OSTYPE),solaris)
SHELL		:= /bin/sh
else
SHELL           := /bin/bash
endif

.SUFFIXES:	.cxx .prc 

# Required macro symbols

# Directories

ifdef PREFIX
UNIX_INC_DIR	= $(PREFIX)/include/ptlib/unix
else
UNIX_INC_DIR	= $(PTLIBDIR)/include/ptlib/unix
endif

ifndef UNIX_SRC_DIR
UNIX_SRC_DIR	= $(PTLIBDIR)/src/ptlib/unix
endif

PT_LIBDIR	= $(PTLIBDIR)/lib_$(PLATFORM_TYPE)

# set name of the PT library
PTLIB_BASE	= pt$(OBJ_SUFFIX)
PTLIB_FILE	= lib$(PTLIB_BASE)$(LIB_TYPE).$(LIB_SUFFIX)
PTLIB_DEBUG_FILE= lib$(PTLIB_BASE)_d$(LIB_TYPE).$(LIB_SUFFIX)
PT_OBJBASE	= obj$(OBJDIR_SUFFIX)
PT_OBJDIR	= $(PT_LIBDIR)/$(PT_OBJBASE)

###############################################################################
#
# Set up compiler flags and macros for debug/release versions
#

ifdef	DEBUG

ifndef MEMORY_CHECK
MEMORY_CHECK := 1
endif

STDCCFLAGS	+= $(DEBUG_FLAG) -D_DEBUG
LDFLAGS		+= $(DEBLDFLAGS)

else

STDCCFLAGS	+= -DNDEBUG

ifneq ($(OSTYPE),Darwin)
  ifeq ($(OSTYPE),solaris)
    ifeq ($(USE_GCC),yes)
      STDCCFLAGS	+= -O3
    else
      STDCCFLAGS	+= -xO3
    endif
  else
    STDCCFLAGS	+= -Os 
  endif
else
  STDCCFLAGS	+= -O2
endif

endif # DEBUG

# define ESDDIR variables if installed
ifdef  ESDDIR
STDCCFLAGS	+= -I$(ESDDIR)/include -DUSE_ESD=1
ENDLDLIBS	+= $(ESDDIR)/lib/libesd.a  # to avoid name conflicts
HAS_ESD		= 1
endif

# feature migrated to configure.in
# #define templates if available
# ifndef NO_PTLIB_TEMPLATES
# STDCCFLAGS	+= -DPHAS_TEMPLATES
# endif

# compiler flags for all modes
#STDCCFLAGS	+= -fomit-frame-pointer
#STDCCFLAGS	+= -fno-default-inline
#STDCCFLAGS     += -Woverloaded-virtual
#STDCCFLAGS     += -fno-implement-inlines

# add OS directory to include path
# STDCCFLAGS	+= -I$(UNIX_INC_DIR)  # removed CRS


# add library directory to library path and include the library
LDFLAGS		+= -L$(PT_LIBDIR)

LDLIBS		+= -l$(PTLIB_BASE)$(LIB_TYPE)

# End of unix.mak
