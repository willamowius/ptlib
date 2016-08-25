/*
 * osutil.inl
 *
 * Operating System classes inline function implementation
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
 * $Id$
 */

#ifdef P_USE_LANGINFO
#include <langinfo.h>
#endif

PINLINE DWORD PProcess::GetProcessID() const
{
#ifdef __NUCLEUS_PLUS__
// Only one process
  return 0;
#else
  return (DWORD)getpid();
#endif
}

///////////////////////////////////////////////////////////////////////////////

PINLINE unsigned PTimer::Resolution()
#ifdef __NUCLEUS_PLUS__
  {
// Returns number of milliseconds per tick
#pragma message ("Timer tick hard-coded at 10 milliseconds in ptlib.inl")
  return 10;
  }
#elif defined(P_SUN4)
  { return 1000; }
#else
  { return (unsigned)(1000/CLOCKS_PER_SEC); }
#endif

///////////////////////////////////////////////////////////////////////////////


PINLINE void PTime::SetCurrentTime()
{
  theTime = time(NULL);
}


///////////////////////////////////////////////////////////////////////////////

PINLINE PBoolean PDirectory::IsRoot() const
#ifdef WOT_NO_FILESYSTEM
  { return PTrue;}
#else
  { return IsSeparator((*this)[0]) && ((*this)[1] == '\0'); }
#endif

PINLINE PBoolean PDirectory::IsSeparator(char ch)
  { return ch == PDIR_SEPARATOR; }

#ifdef WOT_NO_FILESYSTEM
PINLINE PBoolean PDirectory::Change(const PString &)
  { return PTrue;}

PINLINE PBoolean PDirectory::Exists(const PString & p)
  { return PFalse; }
#else
PINLINE PBoolean PDirectory::Change(const PString & p)
  { return chdir(p) == 0; }

PINLINE PBoolean PDirectory::Exists(const PString & p)
  { return access((const char *)p, 0) == 0; }
#endif

///////////////////////////////////////////////////////////////////////////////

PINLINE PString PFilePath::GetVolume() const
  { return PString::Empty(); }

///////////////////////////////////////////////////////////////////////////////

PINLINE PBoolean PFile::Exists(const PFilePath & name)
#ifdef WOT_NO_FILESYSTEM
  { return PFalse; }
#else
  { return access(name, 0) == 0; }
#endif

PINLINE PBoolean PFile::Remove(const PFilePath & name, PBoolean)
  { return unlink(name) == 0; }

///////////////////////////////////////////////////////////////////////////////

PINLINE void PChannel::Construct()
  { os_handle = -1; }

PINLINE PString PChannel::GetName() const
  { return channelName; }

// End Of File ///////////////////////////////////////////////////////////////
