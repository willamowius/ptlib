#
# ptlib.mak
#
# mak rules to be included in a ptlib application Makefile.
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

$(error No PTLIBDIR environment variable defined! \
You need to define PTLIBDIR! \
Try something like: \
PTLIBDIR = $(HOME)/ptlib)

endif

include $(PTLIBDIR)/make/unix.mak
include $(PTLIBDIR)/make/common.mak

# End of ptlib.mak
