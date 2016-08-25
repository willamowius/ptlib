/*
 * pipechan.cxx
 *
 * Sub-process communicating with pipe I/O channel class
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
#include <ptlib/pipechan.h>

#include <ctype.h>


///////////////////////////////////////////////////////////////////////////////
// PPipeChannel

static PBoolean SplitArgs(const PString & cmdline,
                      PString & progName,
                      PStringArray & arguments)
{
  PArgList list = cmdline;
  if (list.GetCount() == 0)
    return PFalse;

  progName = list[0];

  arguments.SetSize(list.GetCount()-1);
  for (PINDEX i = 1; i < list.GetCount(); i++)
    arguments[i-1] = list[i];

  return PTrue;
}


PPipeChannel::PPipeChannel(const PString & subProgram,
                           OpenMode mode,
                           PBoolean searchPath,
                           PBoolean stderrSeparate)
{
  PString progName;
  PStringArray arguments;
  if (SplitArgs(subProgram, progName, arguments))
    PlatformOpen(progName, arguments, mode, searchPath, stderrSeparate, NULL);
}


PPipeChannel::PPipeChannel(const PString & subProgram,
                           const PStringArray & arguments,
                           OpenMode mode,
                           PBoolean searchPath,
                           PBoolean stderrSeparate)
{
  PlatformOpen(subProgram, arguments, mode, searchPath, stderrSeparate, NULL);
}


PPipeChannel::PPipeChannel(const PString & subProgram,
                           const PStringToString & environment,
                           OpenMode mode,
                           PBoolean searchPath,
                           PBoolean stderrSeparate)
{
  PString progName;
  PStringArray arguments;
  if (SplitArgs(subProgram, progName, arguments))
    PlatformOpen(progName, arguments, mode, searchPath, stderrSeparate, &environment);
}


PPipeChannel::PPipeChannel(const PString & subProgram,
                           const PStringArray & arguments,
                           const PStringToString & environment,
                           OpenMode mode,
                           PBoolean searchPath,
                           PBoolean stderrSeparate)
{
  PlatformOpen(subProgram, arguments, mode, searchPath, stderrSeparate, &environment);
}


PObject::Comparison PPipeChannel::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PPipeChannel), PInvalidCast);
  return subProgName.Compare(((const PPipeChannel &)obj).subProgName);
}


PString PPipeChannel::GetName() const
{
  return subProgName;
}


PBoolean PPipeChannel::Open(const PString & subProgram,
                        OpenMode mode,
                        PBoolean searchPath,
                        PBoolean stderrSeparate)
{
  PString progName;
  PStringArray arguments;
  if (!SplitArgs(subProgram, progName, arguments))
    return PFalse;
  return PlatformOpen(progName, arguments, mode, searchPath, stderrSeparate, NULL);
}


PBoolean PPipeChannel::Open(const PString & subProgram,
                        const PStringArray & arguments,
                        OpenMode mode,
                        PBoolean searchPath,
                        PBoolean stderrSeparate)
{
  return PlatformOpen(subProgram, arguments, mode, searchPath, stderrSeparate, NULL);
}


PBoolean PPipeChannel::Open(const PString & subProgram,
                        const PStringToString & environment,
                        OpenMode mode,
                        PBoolean searchPath,
                        PBoolean stderrSeparate)
{
  PString progName;
  PStringArray arguments;
  if (!SplitArgs(subProgram, progName, arguments))
    return PFalse;
  return PlatformOpen(progName, arguments, mode, searchPath, stderrSeparate, &environment);
}


PBoolean PPipeChannel::Open(const PString & subProgram,
                        const PStringArray & arguments,
                        const PStringToString & environment,
                        OpenMode mode,
                        PBoolean searchPath,
                        PBoolean stderrSeparate)
{
  return PlatformOpen(subProgram, arguments, mode, searchPath, stderrSeparate, &environment);
}


// End Of File ///////////////////////////////////////////////////////////////
