/*
 * contain.h
 *
 * Low level object and container definitions.
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

#include "pmachdep.h"
#include <unistd.h>
#include <ctype.h>
#include <limits.h>


///////////////////////////////////////////
//
//  define TRUE and FALSE for environments that don't have them
//

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif

#ifdef P_USE_INTEGER_BOOL
typedef int BOOL;
#endif

///////////////////////////////////////////
//
//  define a macro for declaring classes so we can bolt
//  extra things to class declarations
//

#define PEXPORT
#define PSTATIC


///////////////////////////////////////////
//
// define some basic types and their limits
//

typedef  int16_t  PInt16; // 16 bit
typedef uint16_t PUInt16; // 16 bit
typedef  int32_t  PInt32; // 32 bit
typedef uint32_t PUInt32; // 32 bit

#ifndef P_NEEDS_INT64
typedef   signed long long int  PInt64; // 64 bit
typedef unsigned long long int PUInt64; // 64 bit
#endif


// Integer type that is same size as a pointer type.
typedef intptr_t      INT;
typedef uintptr_t    UINT;

// Create "Windows" style definitions.

typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;

typedef void                    VOID;
typedef char                    CHAR;
typedef wchar_t                 WCHAR;
typedef signed char             SCHAR;
typedef unsigned char           UCHAR;
typedef short                   SHORT;
typedef signed short            SSHORT;
typedef unsigned short          USHORT;
typedef  int16_t                SWORD;
typedef uint16_t                UWORD;
typedef long                    LONG;
typedef signed long             SLONG;
typedef unsigned long           ULONG;

#if defined(__APPLE__)
typedef signed long int         SDWORD;
typedef unsigned long int       UDWORD;
#else
typedef int32_t                 SDWORD;
typedef uint32_t                UDWORD;
#endif

typedef float                   SFLOAT;
typedef double                  SDOUBLE;
typedef double                  LDOUBLE;

typedef void *                  PTR;
typedef void *                  LPVOID;
typedef CHAR *                  LPSTR;
typedef WCHAR *                 LPWSTR;
typedef const CHAR *            LPCSTR;
typedef const WCHAR *           LPCWSTR;
typedef DWORD *                 LPDWORD;
#define FAR

typedef signed short            RETCODE;
typedef void *                  HWND;

// For sqltypes.h, prevent it from redefining the above
#define ALLREADY_HAVE_WINDOWS_TYPE 1

typedef SCHAR SQLSCHAR;
typedef HWND SQLHWND;
#define SQL_API


///////////////////////////////////////////
// Type used for array indexes and sizes

typedef int PINDEX;
#define P_MAX_INDEX INT_MAX

inline PINDEX PABSINDEX(PINDEX idx) { return (idx < 0 ? -idx : idx)&P_MAX_INDEX; }
#define PASSERTINDEX(idx) PAssert((idx) >= 0, PInvalidArrayIndex)

///////////////////////////////////////////
//
// needed for STL
//
#if P_HAS_STL_STREAMS
#define __USE_STL__     1
// both gnu-c++ and stlport define __true_type normally this would be
// fine but until pwlib removes the evil using namespace std below,
// this is included here to ensure the types do not conflict.  Yes you
// get math when you don't want it but its one of the things in
// stlport that sources the native cmath and includes
// the gcc header bits/cpp_type_traits.h which has the conflicting type.
//
// the sooner the using namespace std below is removed the better.
// namespace pollution in headers is plain wrong!
// 
#include <cmath>
#endif

#define P_HAS_TYPEINFO  1

using namespace std;

