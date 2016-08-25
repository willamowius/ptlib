# Makefile
#
# Make file for ptlib library
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
# Contributor(s): Matthias Schneider____________________.

ifndef PTLIBDIR
PTLIBDIR := $(HOME)/ptlib
endif

include $(PTLIBDIR)/make/unix.mak

OBJDIR	= $(PT_OBJDIR)
LIBDIR  = $(PT_LIBDIR)
TARGET  = $(LIBDIR)/$(PTLIB_FILE)
VERSION_FILE = $(PTLIBDIR)/version.h
REVISION_FILE = $(PTLIBDIR)/revision.h

##########################################

COMPONENT_SRC_DIR	= ptclib
COMMON_SRC_DIR		= ptlib/common
PLUGIN_DIR		= ../plugins
ifeq ($(OSTYPE),mingw)
PLATFORM_SRC_DIR        = ptlib/msos
else
PLATFORM_SRC_DIR        = ptlib/unix
endif
VPATH_CXX		:= $(PLATFORM_SRC_DIR) $(COMMON_SRC_DIR) $(COMPONENT_SRC_DIR) 

ifeq ($(OSTYPE),Darwin)
SED_EXTREGEX_FLAG		= -E
else
SED_EXTREGEX_FLAG		= -r
endif

# try and keep the order of modules in reverse order of usage so any
# global statics will be constructed/destructed in the correct order

ifdef HAS_SASL2
SOURCES	+= $(COMPONENT_SRC_DIR)/psasl.cxx 
endif

ifdef HAS_OPENLDAP
SOURCES	+= \
	$(COMPONENT_SRC_DIR)/pldap.cxx \
        $(COMPONENT_SRC_DIR)/pils.cxx
endif

ifdef HAS_OPENSSL
SOURCES += $(COMPONENT_SRC_DIR)/pssl.cxx 
endif

ifdef HAS_SDL
SOURCES += $(COMPONENT_SRC_DIR)/vsdl.cxx

$(OBJDIR)/vsdl.o: $(COMPONENT_SRC_DIR)/vsdl.cxx
	@if [ ! -d $(OBJDIR) ] ; then mkdir -p $(OBJDIR) ; fi
	$(Q_CC)$(CXX) -DP_FORCE_STATIC_PLUGIN $(STDCCFLAGS) $(CFLAGS) -c $< -o $@

endif

ifdef HAS_ODBC
SOURCES += $(COMPONENT_SRC_DIR)/podbc.cxx
endif

## VIDEO DRIVERS
## Note this is mostly handled by the plugin system

ifdef HAS_VIDEO

SOURCES	 += $(COMMON_SRC_DIR)/vfakeio.cxx \
            $(COMMON_SRC_DIR)/videoio.cxx \
	    $(COMMON_SRC_DIR)/vconvert.cxx \
	    $(COMMON_SRC_DIR)/pvidchan.cxx \
	    $(COMMON_SRC_DIR)/tinyjpeg.c \
	    $(COMMON_SRC_DIR)/jidctflt.c

$(OBJDIR)/vfakeio.o: $(COMMON_SRC_DIR)/vfakeio.cxx
	@if [ ! -d $(OBJDIR) ] ; then mkdir -p $(OBJDIR) ; fi
	$(Q_CC)$(CXX) -DP_FORCE_STATIC_PLUGIN $(STDCCFLAGS) $(CFLAGS) -c $< -o $@

ifdef HAS_SHM_VIDEO
SOURCES	+= $(PLATFORM_SRC_DIR)/shmvideo.cxx
endif

ifndef HAS_PLUGINS

ifeq ($(OSTYPE),linux)
VPATH_CXX	+= $(PLUGIN_DIR)/vidinput_v4l
SOURCES		+= $(PLUGIN_DIR)/vidinput_v4l/vidinput_v4l.cxx
endif

endif  # HAS_PLUGINS

ifdef HAS_VFW_CAPTURE
SOURCES		+= $(PLATFORM_SRC_DIR)/vfw.cxx
endif

ifdef HAS_DIRECTSHOW
SOURCES	+= $(PLATFORM_SRC_DIR)/directshow.cxx
$(OBJDIR)/directshow.o: $(PLATFORM_SRC_DIR)/directshow.cxx
	@if [ ! -d $(OBJDIR) ] ; then mkdir -p $(OBJDIR) ; fi
	$(Q_CC)$(CXX) -DP_FORCE_STATIC_PLUGIN $(STDCCFLAGS) $(CFLAGS) -c $< -o $@
endif

endif # HAS_VIDEO

## SOUND DRIVERS
## Note this is mostly handled by the plugin system

ifdef HAS_AUDIO

SOURCES += $(COMMON_SRC_DIR)/sound.cxx 

ifeq ($(OSTYPE),mingw)
SOURCES += $(PLATFORM_SRC_DIR)/sound_win32.cxx
$(OBJDIR)/sound_win32.o: $(PLATFORM_SRC_DIR)/sound_win32.cxx
	@if [ ! -d $(OBJDIR) ] ; then mkdir -p $(OBJDIR) ; fi
	$(Q_CC)$(CXX) -DP_FORCE_STATIC_PLUGIN $(STDCCFLAGS) $(CFLAGS) -c $< -o $@
endif

ifndef HAS_PLUGINS

ifeq ($(OSTYPE),linux)
VPATH_CXX	+= $(PLUGIN_DIR)/sound_oss
SOURCES		+= $(PLUGIN_DIR)/sound_oss/sound_oss.cxx
endif # linux

ifdef HAS_ALSA
VPATH_CXX	+= $(PLUGIN_DIR)/sound_alsa
SOURCES		+= $(PLUGIN_DIR)/sound_alsa/sound_alsa.cxx
endif  # HAS_ALSA

ifeq ($(OSTYPE),beos)
SOURCES	 += $(PLATFORM_SRC_DIR)/beaudio.cxx
endif  # beos
 
endif  # HAS_PLUGINS

# Although not an external plugin, the MacOSX
# CoreAudio sound driver still is in plugin form
# and requires the PPluginManager
ifeq ($(OSTYPE),Darwin)
SOURCES	+= $(PLATFORM_SRC_DIR)/maccoreaudio.cxx
endif # Darwin

endif  # HAS_AUDIO

## Various modules

SOURCES	+= $(COMPONENT_SRC_DIR)/pxml.cxx 

ifdef HAS_EXPAT

ifdef HAS_XMLRPC
SOURCES	+= \
	$(COMPONENT_SRC_DIR)/pxmlrpc.cxx \
	$(COMPONENT_SRC_DIR)/pxmlrpcs.cxx 
endif

ifdef HAS_SOAP
SOURCES	+= $(COMPONENT_SRC_DIR)/psoap.cxx 
endif

ifdef HAS_VXML
SOURCES	+= $(COMPONENT_SRC_DIR)/vxml.cxx 
endif

ifdef HAS_SASL2
SOURCES += \
	$(COMPONENT_SRC_DIR)/xmpp.cxx \
	$(COMPONENT_SRC_DIR)/xmpp_c2s.cxx \
	$(COMPONENT_SRC_DIR)/xmpp_muc.cxx \
	$(COMPONENT_SRC_DIR)/xmpp_roster.cxx
endif
endif # HAS_EXPAT

ifdef HAS_LUA
SOURCES	+= $(COMPONENT_SRC_DIR)/lua.cxx
endif

ifdef HAS_RESOLVER
SOURCES += \
	$(COMPONENT_SRC_DIR)/pdns.cxx \
	$(COMPONENT_SRC_DIR)/enum.cxx 
endif

ifdef HAS_TTS
SOURCES += $(COMPONENT_SRC_DIR)/ptts.cxx 
endif

ifdef HAS_ASN
SOURCES += \
	$(COMPONENT_SRC_DIR)/asner.cxx \
	$(COMPONENT_SRC_DIR)/pasn.cxx 
endif

ifdef HAS_SNMP
SOURCES += \
	$(COMPONENT_SRC_DIR)/snmpclnt.cxx \
	$(COMPONENT_SRC_DIR)/snmpserv.cxx \
	$(COMPONENT_SRC_DIR)/psnmp.cxx \
	$(COMPONENT_SRC_DIR)/snmp.cxx \
	$(COMPONENT_SRC_DIR)/rfc1155.cxx 
endif

ifdef HAS_FTP
SOURCES += \
	$(COMPONENT_SRC_DIR)/ftpclnt.cxx \
	$(COMPONENT_SRC_DIR)/ftpsrvr.cxx \
	$(COMPONENT_SRC_DIR)/ftp.cxx 
endif

ifdef HAS_TELNET
SOURCES += $(COMPONENT_SRC_DIR)/telnet.cxx 
endif

ifdef HAS_STUN
SOURCES += $(COMPONENT_SRC_DIR)/pstun.cxx \
           $(COMPONENT_SRC_DIR)/pnat.cxx

$(OBJDIR)/pstun.o: $(COMPONENT_SRC_DIR)/pstun.cxx
	@if [ ! -d $(OBJDIR) ] ; then mkdir -p $(OBJDIR) ; fi
	$(Q_CC)$(CXX) -DP_FORCE_STATIC_PLUGIN $(STDCCFLAGS) $(CFLAGS) -c $< -o $@

endif

ifdef HAS_SOCKS
SOURCES += $(COMPONENT_SRC_DIR)/socks.cxx 
endif

ifdef HAS_PIPECHAN
ifeq ($(OSTYPE),mingw)
SOURCES += $(PLATFORM_SRC_DIR)/pipe.cxx 
SOURCES += $(COMMON_SRC_DIR)/pipechan.cxx 
else
SOURCES += $(PLATFORM_SRC_DIR)/pipechan.cxx 
endif
endif

ifdef HAS_REMCONN
SOURCES += $(PLATFORM_SRC_DIR)/remconn.cxx 
endif

ifdef HAS_WAVFILE
SOURCES += $(COMPONENT_SRC_DIR)/pwavfile.cxx \
           $(COMPONENT_SRC_DIR)/pwavfiledev.cxx

$(OBJDIR)/pwavfiledev.o: $(COMPONENT_SRC_DIR)/pwavfiledev.cxx
	@if [ ! -d $(OBJDIR) ] ; then mkdir -p $(OBJDIR) ; fi
	$(Q_CC)$(CXX) -DP_FORCE_STATIC_PLUGIN $(STDCCFLAGS) $(CFLAGS) -c $< -o $@
endif

ifdef HAS_DTMF
SOURCES += $(COMPONENT_SRC_DIR)/dtmf.cxx 
endif

ifdef HAS_SERIAL

ifeq ($(OSTYPE),mingw)
SOURCES += $(PLATFORM_SRC_DIR)/winserial.cxx \
     	   $(COMMON_SRC_DIR)/serial.cxx

else
SOURCES += $(PLATFORM_SRC_DIR)/serchan.cxx
endif

SOURCES += $(COMPONENT_SRC_DIR)/modem.cxx 
endif

ifdef HAS_POP3SMTP
SOURCES += \
	$(COMPONENT_SRC_DIR)/inetmail.cxx 
endif

ifdef HAS_URL
SOURCES += \
	$(COMPONENT_SRC_DIR)/url.cxx 
endif

ifdef HAS_HTTP
SOURCES += \
	$(COMPONENT_SRC_DIR)/http.cxx \
	$(COMPONENT_SRC_DIR)/httpclnt.cxx \
	$(COMPONENT_SRC_DIR)/html.cxx \
	$(COMPONENT_SRC_DIR)/httpsrvr.cxx
endif

ifdef HAS_HTTPFORMS
SOURCES += \
	$(COMPONENT_SRC_DIR)/httpform.cxx
endif #HAS_HTTPFORMS

ifdef HAS_HTTPSVC
SOURCES += \
	$(PLATFORM_SRC_DIR)/svcproc.cxx \
	$(COMPONENT_SRC_DIR)/httpsvc.cxx

ifdef HAS_OPENSSL
SOURCES += $(COMPONENT_SRC_DIR)/shttpsvc.cxx
endif #HAS_OPENSSL
endif #HAS_HTTPSVC

ifdef HAS_CONFIG_FILE
ifeq ($(OSTYPE),mingw)
SOURCES += $(PLATFORM_SRC_DIR)/wincfg.cxx 
else
SOURCES += $(PLATFORM_SRC_DIR)/config.cxx 
endif
endif

ifdef HAS_VIDFILE
SOURCES += $(COMPONENT_SRC_DIR)/pvidfile.cxx \
	   $(COMPONENT_SRC_DIR)/pvfiledev.cxx 

$(OBJDIR)/pvfiledev.o: $(COMPONENT_SRC_DIR)/pvfiledev.cxx
	@if [ ! -d $(OBJDIR) ] ; then mkdir -p $(OBJDIR) ; fi
	$(Q_CC)$(CXX) -DP_FORCE_STATIC_PLUGIN $(STDCCFLAGS) $(CFLAGS) -c $< -o $@

endif

ifdef HAS_FFVDEV
SOURCES += $(COMPONENT_SRC_DIR)/pffvdev.cxx

$(OBJDIR)/pffvdev.o: $(COMPONENT_SRC_DIR)/pffvdev.cxx
	@if [ ! -d $(OBJDIR) ] ; then mkdir -p $(OBJDIR) ; fi
	$(Q_CC)$(CXX) -DP_FORCE_STATIC_PLUGIN $(STDCCFLAGS) $(CFLAGS) -c $< -o $@

endif

ifeq ($(OSTYPE),mingw)
SOURCES	+= \
        $(PLATFORM_SRC_DIR)/ptlib.cxx \
        $(PLATFORM_SRC_DIR)/icmp.cxx \
        $(PLATFORM_SRC_DIR)/winsock.cxx \
        $(PLATFORM_SRC_DIR)/win32.cxx \
        $(PLATFORM_SRC_DIR)/dllmain.cxx \
        $(PLATFORM_SRC_DIR)/ethsock.cxx \
        $(COMMON_SRC_DIR)/pchannel.cxx \
        $(COMMON_SRC_DIR)/pethsock.cxx \
        $(COMMON_SRC_DIR)/pconfig.cxx
else
SOURCES	+= \
	$(PLATFORM_SRC_DIR)/uicmp.cxx \
	$(PLATFORM_SRC_DIR)/socket.cxx \
	$(PLATFORM_SRC_DIR)/udll.cxx \
	$(PLATFORM_SRC_DIR)/channel.cxx \
	$(PLATFORM_SRC_DIR)/osutil.cxx \
	$(PLATFORM_SRC_DIR)/tlib.cxx \
	$(PLATFORM_SRC_DIR)/switch.cxx
endif

GETDATE_SOURCE = $(COMMON_SRC_DIR)/getdate.tab.c

SOURCES	+= \
	$(COMPONENT_SRC_DIR)/cli.cxx \
	$(COMPONENT_SRC_DIR)/threadpool.cxx \
	$(COMPONENT_SRC_DIR)/ipacl.cxx \
	$(COMPONENT_SRC_DIR)/qchannel.cxx \
	$(COMPONENT_SRC_DIR)/delaychan.cxx \
	$(COMPONENT_SRC_DIR)/memfile.cxx \
	$(COMPONENT_SRC_DIR)/cypher.cxx \
	$(COMPONENT_SRC_DIR)/random.cxx \
	$(COMPONENT_SRC_DIR)/inetprot.cxx \
	$(COMPONENT_SRC_DIR)/notifier_ext.cxx \
	$(COMPONENT_SRC_DIR)/vcard.cxx \
	$(COMPONENT_SRC_DIR)/guid.cxx \
	$(GETDATE_SOURCE) \
	$(PLATFORM_SRC_DIR)/assert.cxx \
	$(COMMON_SRC_DIR)/pluginmgr.cxx \
	$(COMMON_SRC_DIR)/sockets.cxx \
	$(COMMON_SRC_DIR)/psockbun.cxx \
	$(COMMON_SRC_DIR)/qos.cxx \
	$(COMMON_SRC_DIR)/ptime.cxx \
	$(COMMON_SRC_DIR)/syslog.cxx \
	$(COMMON_SRC_DIR)/osutils.cxx \
	$(COMMON_SRC_DIR)/safecoll.cxx \
	$(COMMON_SRC_DIR)/collect.cxx \
	$(COMMON_SRC_DIR)/contain.cxx \
	$(COMMON_SRC_DIR)/object.cxx   # must be last module

ifneq ($(HAS_REGEX),1)
OBJS	= $(OBJDIR)/regcomp.o $(OBJDIR)/regexec.o $(OBJDIR)/regerror.o $(OBJDIR)/regfree.o
endif

CLEAN_FILES = $(GETDATE_SOURCE)


$(COMMON_SRC_DIR)/osutils.cxx: $(REVISION_FILE)

$(REVISION_FILE) : $(REVISION_FILE).in
ifeq ($(shell which svn 2> /dev/null),)
	sed -e "s/.WCREV./`sed -n -e 's/.*Revision: \([0-9]*\).*/\1/p' $<`/" $< > $@
else
ifeq ($(wildcard $(PTLIBDIR)/.svn),)
	sed -e "s/.WCREV./`sed -n -e 's/.*Revision: \([0-9]*\).*/\1/p' $<`/" $< > $@
else
	$(Q)sed "s/SVN_REVISION.*/SVN_REVISION `LC_ALL=C svn info | sed -n 's/Revision: //p'`/" $< > $@.tmp
	$(Q)if diff -q $@ $@.tmp >/dev/null 2>&1; then \
	  rm $@.tmp; \
	else \
	  mv -f $@.tmp $@; \
	  echo "Revision file updated to `sed $(SED_EXTREGEX_FLAG) -n 's/.*SVN_REVISION(.*)/\1/p' $@`" ; \
	fi

.PHONY: $(REVISION_FILE)
endif
endif

#######################################################

include ../make/common.mak

#######################################################

alllibs:
	$(MAKE) both
	$(MAKE) bothshared


$(OBJDIR)/regcomp.o: $(COMMON_SRC_DIR)/regex/regcomp.c
	$(Q_CC)$(CC) $(DEBUG_FLAG) $(OPTCCFLAGS) -c -DPOSIX_MISTAKE -I$(COMMON_SRC_DIR)/regex $(CFLAGS) -o $@ -c $<

$(OBJDIR)/regexec.o: $(COMMON_SRC_DIR)/regex/regexec.c
	$(Q_CC)$(CC) $(DEBUG_FLAG) $(OPTCCFLAGS) -c -DPOSIX_MISTAKE -I$(COMMON_SRC_DIR)/regex $(CFLAGS) -o $@ -c $<

$(OBJDIR)/regerror.o: $(COMMON_SRC_DIR)/regex/regerror.c
	$(Q_CC)$(CC) $(DEBUG_FLAG) $(OPTCCFLAGS) -c -DPOSIX_MISTAKE -I$(COMMON_SRC_DIR)/regex $(CFLAGS) -o $@ -c $<

$(OBJDIR)/regfree.o: $(COMMON_SRC_DIR)/regex/regfree.c
	$(Q_CC)$(CC) $(DEBUG_FLAG) $(OPTCCFLAGS) -c -DPOSIX_MISTAKE -I$(COMMON_SRC_DIR)/regex $(CFLAGS) -o $@ -c $<


ifdef REQUIRES_SEPARATE_SWITCH 
$(OBJDIR)/switch.o: $(PLATFORM_SRC_DIR)/switch.cxx
	@if [ ! -d $(OBJDIR) ] ; then mkdir -p $(OBJDIR) ; fi
	$(Q_CC)$(CXX) $(STDCCFLAGS) $(CFLAGS) -c $< -o $@
endif

$(OBJDIR)/getdate.tab.o: $(GETDATE_SOURCE)
	$(Q_CC)$(CC) -Wno-write-strings $(STDCCFLAGS) $(CFLAGS) -c $< -o $@

$(DEPDIR)/getdate.tab.dep: $(GETDATE_SOURCE)
	$(Q_CC)$(CC) $(STDCCFLAGS) -M $< >> $@

# needs bison 2.x
#$(GETDATE_SOURCE): $(COMMON_SRC_DIR)/getdate.y
#	bison $(COMMON_SRC_DIR)/getdate.y -o $(COMMON_SRC_DIR)/getdate.tab.c

#######################################################

LIB_BASENAME=$(PTLIB_BASE)
LIB_FILENAME=$(PTLIB_FILE)

include $(PTLIBDIR)/make/lib.mak

#######################################################

# End of Makefile
