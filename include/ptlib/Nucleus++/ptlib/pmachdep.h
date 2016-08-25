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

#ifdef __NUCLEUS_MNT__
#pragma message ("<netdb.h> not included")
#define P_PLATFORM_HAS_THREADS
// The windows version of errno.h, which this will find, should do for us -
// it contains lots of things from Unix!!!
#include <errno.h>
#else
#ifdef __NUCLEUS_PLUS__
#define P_PLATFORM_HAS_THREADS
#endif
#ifndef __NUCLEUS_PLUS__
#include <netdb.h>
#endif
#endif


#if defined(P_PTHREADS)
#define P_PLATFORM_HAS_THREADS
#ifndef __NUCLEUS_PLUS__
#include <pthread.h>
#endif
#endif

// If we're running effectively 'doze, then it's little endian.  If PoserPC,
// big endian.
#ifdef __NUCLEUS_MNT__
#define PBYTE_ORDER PLITTLE_ENDIAN
#else
//#define PBYTE_ORDER PBIG_ENDIAN
#endif

#if 0
#ifdef __NUCLEUS_PLUS__
// Other things
#define	INADDR_NONE	-1
#endif
#endif

#ifdef __NUCLEUS_MNT__
#define BREAKPOINT _asm int 3;
#endif
#ifdef __ppc
#define BREAKPOINT asm("sc");
#endif

#endif // _PMACHDEP_H

// End of file
