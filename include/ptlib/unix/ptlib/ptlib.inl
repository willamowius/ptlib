/*
 * ptlib.inl
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

#if defined(P_LINUX) || defined(P_GNU_HURD)
#if (__GNUC_MINOR__ < 7 && __GNUC__ <= 2)
#include <localeinfo.h>
#else
#define P_USE_LANGINFO
#endif
#elif defined(P_HPUX9)
#define P_USE_LANGINFO
#elif defined(P_SUN4)
#endif

#ifdef P_USE_LANGINFO
#include <langinfo.h>
#endif

PINLINE PProcessIdentifier PProcess::GetCurrentProcessID()
{
#ifdef P_VXWORKS
  return PThread::Current().PX_threadId;
#else
  return getpid();
#endif // P_VXWORKS
}

///////////////////////////////////////////////////////////////////////////////

PINLINE unsigned PTimer::Resolution()
{
#if defined(P_SUN4)
  return 1000;
#elif defined(P_RTEMS)
  rtems_interval ticks_per_sec; 
  rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND, &ticks_per_sec); 
  return (unsigned)(1000/ticks_per_sec);
#else
  return (unsigned)(1000/CLOCKS_PER_SEC);
#endif
}

///////////////////////////////////////////////////////////////////////////////

PINLINE PBoolean PDirectory::IsRoot() const
  { return IsSeparator((*this)[0]) && ((*this)[1] == '\0'); }

PINLINE PDirectory PDirectory::GetRoot() const
  { return PString(PDIR_SEPARATOR); }

PINLINE PBoolean PDirectory::IsSeparator(char ch)
  { return ch == PDIR_SEPARATOR; }

PINLINE PBoolean PDirectory::Change(const PString & p)
  { return chdir((char *)(const char *)p) == 0; }

///////////////////////////////////////////////////////////////////////////////

PINLINE PString PFilePath::GetVolume() const
  { return PString::Empty(); }

///////////////////////////////////////////////////////////////////////////////

PINLINE PBoolean PFile::Remove(const PFilePath & name, PBoolean)
  { return unlink((char *)(const char *)name) == 0; }

PINLINE PBoolean PFile::Remove(const PString & name, PBoolean)
  { return unlink((char *)(const char *)name) == 0; }

///////////////////////////////////////////////////////////////////////////////

PINLINE PString PChannel::GetName() const
  { return channelName; }

#ifdef BE_THREADS

PINLINE PThreadIdentifier PThread::GetThreadId() const
  { return mId; }

#else // !BE_THREADS

#ifndef VX_TASKS
PINLINE PThreadIdentifier PThread::GetCurrentThreadId()
  { return ::pthread_self(); }
#else
PINLINE PThreadIdentifier PThread::GetCurrentThreadId()
  { return ::taskIdSelf(); }
#endif // !VX_TASKS

#endif // BE_THREADS

// End Of File ///////////////////////////////////////////////////////////////
