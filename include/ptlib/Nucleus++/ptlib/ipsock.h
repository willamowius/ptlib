/*
 * ipsock.h
 *
 * Internet Protocol socket I/O channel class.
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

#ifndef _PIPSOCKET

#ifdef __GNUC__
#pragma interface
#endif

#if !defined(__BEOS__) && !defined(__NUCLEUS_PLUS__)
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

// Crib from msos
#if defined(ENETUNREACH) || defined(EHOSTUNREACH) || defined(WSAENETUNREACH) || defined (WSAENETEUNREACH)
#pragma message ("ENET errors actually defined!!!")
#else
// I'm sure this is dodgy, but I'm just copying 'doze for the moment.

#define WSABASEERR              10000

#define WSAENETUNREACH          (WSABASEERR+51)
#define WSAEHOSTUNREACH         (WSABASEERR+65)

#define ENETUNREACH             (WSAENETUNREACH|0x40000000)
#define EHOSTUNREACH            (WSAEHOSTUNREACH|0x40000000)
#endif



///////////////////////////////////////////////////////////////////////////////
// PIPSocket

#include "../../ipsock.h"
};


ostream & operator << (ostream & strm, const PIPSocket::Address & addr);

#endif
