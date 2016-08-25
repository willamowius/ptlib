/*
 * contain.h
 *
 * General container classes.
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

#ifndef PTLIB_CONTAIN_H
#ifndef _WIN32_WCE
#error "Please remove pwlib\include\ptlib\msos from the tool include path" \
"and from the pre-processor options for this project"
#endif // !_WIN32_WCE
#endif

#ifndef PTLIB_MSOS_CONTAIN_H
#define PTLIB_MSOS_CONTAIN_H


#ifdef _MSC_VER

  #pragma warning(disable:4201)  // nonstandard extension: nameless struct/union
  #pragma warning(disable:4201)  // nonstandard extension: nameless struct/union
  #pragma warning(disable:4251)  // disable warning exported structs
  #pragma warning(disable:4511)  // default copy ctor not generated warning
  #pragma warning(disable:4512)  // default assignment op not generated warning
  #pragma warning(disable:4514)  // unreferenced inline removed
  #pragma warning(disable:4699)  // precompiled headers
  #pragma warning(disable:4702)  // disable warning about unreachable code
  #pragma warning(disable:4705)  // disable warning about statement has no effect
  #pragma warning(disable:4710)  // inline not expanded warning
  #pragma warning(disable:4711)  // auto inlining warning
  #pragma warning(disable:4786)  // identifier was truncated to '255' characters in the debug information
  #pragma warning(disable:4097)  // typedef synonym for class
  #pragma warning(disable:4800)  // forcing value to bool 'true' or 'false' (performance warning)

  #if !defined(__USE_STL__) && (_MSC_VER>=1300)
    #define __USE_STL__ 1
  #endif

  #if !defined(_CRT_SECURE_NO_DEPRECATE) && (_MSC_VER>=1400)
    #define _CRT_SECURE_NO_DEPRECATE 1
  #endif

  #if !defined(_CRT_NONSTDC_NO_WARNINGS) && (_MSC_VER>=1400)
    #define _CRT_NONSTDC_NO_WARNINGS 1
  #endif

#endif // _MSC_VER


///////////////////////////////////////////////////////////////////////////////
// Machine & Compiler dependent declarations

#ifndef WIN32
  #define WIN32  1
#endif

#ifndef _WIN32
  #define _WIN32  1
#endif

#if defined(_WINDOWS) || defined(_WIN32)

  // At least Windows 2000, configure.ac will generally uprate this
  #ifndef WINVER
    #define WINVER 0x0500
  #endif

  #if !defined(_WIN32_WINNT) && !defined(_WIN32_WCE)
    #define _WIN32_WINNT WINVER
  #endif

  #if !defined(_WIN32_WCE) && defined(_WIN32_WINNT) && (_WIN32_WINNT == 0x0500) && P_HAS_IPV6 && !defined(NTDDI_VERSION)
    #define NTDDI_VERSION NTDDI_WIN2KSP1
  #endif

  #ifndef STRICT
    #define STRICT
  #endif

  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif

  #include <windows.h>

  #undef DELETE   // Remove define from NT headers.


#else

  typedef unsigned char  BYTE;  //  8 bit unsigned integer quantity
  typedef unsigned short WORD;  // 16 bit unsigned integer quantity
  typedef unsigned long  DWORD; // 32 bit unsigned integer quantity
  typedef int            BOOL;  // type returned by expresion (i != j)

  #define TRUE 1
  #define FALSE 0

  #define NEAR __near

#endif


// Declaration for exported callback functions to OS
#if defined(_WIN32)
  #define PEXPORTED __stdcall
#elif defined(_WINDOWS)
  #define PEXPORTED WINAPI __export
#else
  #define PEXPORTED __far __pascal
#endif


// Declaration for static global variables (WIN16 compatibility)
#if defined(_WIN32)
  #define PSTATIC
#else
  #define PSTATIC __near
#endif


// Declaration for platform independent architectures
#define PCHAR8 PANSI_CHAR
#define PBYTE_ORDER PLITTLE_ENDIAN


// Declaration for integer that is the same size as a void *
#if defined(_WIN32)
  typedef int INT;
#else
  typedef long INT;   
#endif


// Declaration for signed integer that is 16 bits
typedef short PInt16;

// Declaration for signed integer that is 32 bits
typedef long PInt32;

#ifdef __MINGW32__
  #define __USE_STL__
  using namespace std;
  #define P_HAS_INT64
  typedef signed __int64 PInt64;
  typedef unsigned __int64 PUInt64;
#endif

// Declaration for 64 bit unsigned integer quantity
#if defined(_MSC_VER)

  #define P_HAS_INT64

  typedef signed __int64 PInt64;
  typedef unsigned __int64 PUInt64;

  #if _MSC_VER<1300

    class ostream;
    class istream;

    ostream & operator<<(ostream & s, PInt64 v);
    ostream & operator<<(ostream & s, PUInt64 v);

    istream & operator>>(istream & s, PInt64 & v);
    istream & operator>>(istream & s, PUInt64 & v);

  #endif

#endif


// Standard array index type (depends on compiler)
// Type used in array indexes especially that required by operator[] functions.
#if defined(_MSC_VER) || defined(__MINGW32__)

  #define PINDEX int
  #if defined(_WIN32) || defined(_WIN32_WCE)
    const PINDEX P_MAX_INDEX = 0x7fffffff;
  #else
    const PINDEX P_MAX_INDEX = 0x7fff;
  #endif
    inline PINDEX PABSINDEX(PINDEX idx) { return (idx < 0 ? -idx : idx)&P_MAX_INDEX; }
  #define PASSERTINDEX(idx) PAssert((idx) >= 0, PInvalidArrayIndex)

#else

  #define PINDEX unsigned
  #ifndef SIZEOF_INT
  # define SIZEOF_INT sizeof(int)
  #endif
  #if SIZEOF_INT == 4
     const PINDEX P_MAX_INDEX = 0xffffffff;
  #else
     const PINDEX P_MAX_INDEX = 0xffff;
  #endif
  #define PABSINDEX(idx) (idx)
  #define PASSERTINDEX(idx)

#endif

#ifndef _WIN32_WCE 

  #if _MSC_VER>=1400
    #define strcasecmp(s1,s2) _stricmp(s1,s2)
    #define strncasecmp(s1,s2,n) _strnicmp(s1,s2,n)
  #else
    #define strcasecmp(s1,s2) stricmp(s1,s2)
    #define strncasecmp(s1,s2,n) strnicmp(s1,s2,n)
    //#define _putenv ::putenv
    //#define _close ::close
    //#define _access ::access
  #endif

#endif // !_WIN32_WCE 


class PWin32Overlapped : public OVERLAPPED
{
  // Support class for overlapped I/O in Win32.
  public:
    PWin32Overlapped();
    ~PWin32Overlapped();
    void Reset();
};


enum { PWIN32ErrorFlag = 0x40000000 };

class PString;

class RegistryKey
{
  public:
    enum OpenMode {
      ReadOnly,
      ReadWrite,
      Create
    };
    RegistryKey(const PString & subkey, OpenMode mode);
    ~RegistryKey();

    BOOL EnumKey(PINDEX idx, PString & str);
    BOOL EnumValue(PINDEX idx, PString & str);
    BOOL DeleteKey(const PString & subkey);
    BOOL DeleteValue(const PString & value);
    BOOL QueryValue(const PString & value, PString & str);
    BOOL QueryValue(const PString & value, DWORD & num, BOOL boolean);
    BOOL SetValue(const PString & value, const PString & str);
    BOOL SetValue(const PString & value, DWORD num);
  private:
    HKEY key;
};

#ifndef _WIN32_WCE
  #define PDEFINE_WINMAIN(hInstance, hPrevInstance, lpCmdLine, nCmdShow) \
    int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
  #define PDEFINE_WINMAIN(hInstance, hPrevInstance, lpCmdLine, nCmdShow) \
    int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#endif
extern "C" PDEFINE_WINMAIN(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

// used by various modules to disable the winsock2 include to avoid header file problems
#ifndef P_KNOCKOUT_WINSOCK2

  #if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:4127 4706)
  #endif

  #include <winsock2.h> // Version 2 of windows socket
  #include <ws2tcpip.h> // winsock2 is not complete, ws2tcpip add some defines such as IP_TOS

  #if defined(_MSC_VER)
    #pragma warning(pop)
  #endif

  typedef int socklen_t;

#endif  // P_KNOCKOUT_WINSOCK2

#if defined(_MSC_VER) && !defined(_WIN32)
  extern "C" int __argc;
  extern "C" char ** __argv;
#endif

#ifdef __BORLANDC__
  #define __argc _argc
  #define __argv _argv
#endif

#undef Yield

typedef UINT  PThreadIdentifier;
typedef DWORD PProcessIdentifier;


#if defined(_MSC_VER)
  #pragma warning(disable:4201)
#endif

#include <malloc.h>
#include <mmsystem.h>


#ifndef _WIN32_WCE

  #ifdef _MSC_VER
    #include <crtdbg.h>
  #endif
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <errno.h>
  #include <io.h>
  #include <fcntl.h>
  #include <direct.h>
  #include <time.h>

#else

  #include <ptlib/wm/stdlibx.h>
  #include <ptlib/wm/errno.h>
  #include <ptlib/wm/sys/types.h>
  #if _WIN32_WCE < 0x500
    #include <ptlib/wm/time.h>
  #else
    #include <time.h>
  #endif

#ifndef MB_TASKMODAL
  #define MB_TASKMODAL MB_APPLMODAL
#endif

#endif

#define   P_HAS_TYPEINFO  1

// preload <string> and kill warnings
#if defined(_MSC_VER)
  #pragma warning(push)
  #include <yvals.h>    
  #pragma warning(disable:4100)
  #pragma warning(disable:4018)
  #pragma warning(disable:4663)
  #pragma warning(disable:4146)
  #pragma warning(disable:4244)
  #pragma warning(disable:4786)
#endif
#include <string>
#if defined(_MSC_VER)
  #pragma warning(pop)
#endif

// preload <vector> and kill warnings
#if defined(_MSC_VER)
  #pragma warning(push)
  #include <yvals.h>    
  #pragma warning(disable:4018)
  #pragma warning(disable:4663)
  #pragma warning(disable:4786)
#endif
#include <vector>
#if defined(_MSC_VER)
  #pragma warning(pop)
#endif

// preload <map> and kill warnings
#if defined(_MSC_VER)
  #pragma warning(push)
  #include <yvals.h>    
  #pragma warning(disable:4018)
  #pragma warning(disable:4663)
  #pragma warning(disable:4786)
#endif
#include <map>
#if defined(_MSC_VER)
  #pragma warning(pop)
#endif

// preload <utility> and kill warnings
#if defined(_MSC_VER)
  #pragma warning(push)
  #include <yvals.h>    
  #pragma warning(disable:4786)
#endif
#include <utility>
#if defined(_MSC_VER)
  #pragma warning(pop)
#endif

// preload <iterator> and kill warnings
#if defined(_MSC_VER)
  #pragma warning(push)
  #include <yvals.h>    
  #pragma warning(disable:4786)
#endif
#include <iterator>
#if defined(_MSC_VER)
  #pragma warning(pop)
#endif

// preload <algorithm> and kill warnings
#include <algorithm>

// preload <queue> and kill warnings
#if defined(_MSC_VER)
  #pragma warning(push)
  #include <yvals.h>    
  #pragma warning(disable:4284)
#endif
#include <queue>
#if defined(_MSC_VER)
  #pragma warning(pop)
#endif

// VS.net won't work without this :(
#if _MSC_VER>=1300
  using namespace std;
#endif

#if defined(_MSC_VER)
  #pragma warning(disable:4786)
#endif


#endif // PTLIB_MSOS_CONTAIN_H


// End Of File ///////////////////////////////////////////////////////////////
