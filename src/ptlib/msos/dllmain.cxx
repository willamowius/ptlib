/*
 * dllmain.cxx
 *
 * DLL main entry point for PTLib.dll and PWLib.dll
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

#ifdef _MSC_VER
#pragma warning(disable:4201 4514)
#endif

#include <ptbuildopts.h>

// Include header files for everything that uses factories or plugins
#include <ptlib.h>
#include <ptlib/plugin.h>
#include <ptlib/sound.h>
#include <ptlib/videoio.h>
#include <ptclib/pwavfile.h>
#include <ptclib/pvidfile.h>
#include <ptclib/ptts.h>


#ifndef _WIN32_WCE
HINSTANCE PDllInstance;
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
#else
HANDLE PDllInstance;
BOOL WINAPI DllMain(HANDLE hinstDLL, DWORD fdwReason, LPVOID)
#endif
{
  if (fdwReason == DLL_PROCESS_ATTACH)
    PDllInstance = hinstDLL;
  return TRUE;
}

