/*
 * osutil.cxx
 *
 * Operating System classes implementation
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

#include <ptlib.h>


#ifdef P_USE_LANGINFO
#pragma message ("H")
#include <langinfo.h>
#endif

#define	LINE_SIZE_STEP	100

#define	DEFAULT_FILE_MODE	(S_IRUSR|S_IWUSR|S_IROTH|S_IRGRP)

#define new PNEW

///////////////////////////////////////////////////////////////////////////////
// PConsoleChannel

PConsoleChannel::PConsoleChannel()
{
}


PConsoleChannel::PConsoleChannel(ConsoleType type)
{
  Open(type);
}


PBoolean PConsoleChannel::Open(ConsoleType type)
{
  switch (type) {
    case StandardInput :
      os_handle = 0;
      return PTrue;

    case StandardOutput :
      os_handle = 1;
      return PTrue;

    case StandardError :
      os_handle = 2;
      return PTrue;
  }

  return PFalse;
}


PString PConsoleChannel::GetName() const
{
  return "Console";
}

#ifdef __NUCLEUS_MNT__
PBoolean PConsoleChannel::Read(void * buffer, PINDEX length)
  {
  flush();
  cin >> (char *)buffer;
  PINDEX buflen =lastReadCount = strlen((char *)buffer);

  return buflen < length ? buflen : length;
  }


PBoolean PConsoleChannel::Write(const void * buffer, PINDEX length)
  {
  flush();
  cout << PString((const char *)buffer, length) << "\n";
  return PTrue;
  }
#endif

PBoolean PConsoleChannel::Close()
  {
  os_handle = -1;
  return PTrue;
  }

