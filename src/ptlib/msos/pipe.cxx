/*
 * pipe.cxx
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

PPipeChannel::PPipeChannel()
{
  hToChild = hFromChild = hStandardError = INVALID_HANDLE_VALUE;
}


#ifdef _WIN32_WCE
PBoolean PPipeChannel::PlatformOpen(const PString &, const PStringArray &, OpenMode, PBoolean, PBoolean, const PStringToString *)
{
  return PFalse;
}
#else
PBoolean PPipeChannel::PlatformOpen(const PString & subProgram,
                                const PStringArray & argumentList,
                                OpenMode mode,
                                PBoolean searchPath,
                                PBoolean stderrSeparate,
                                const PStringToString * environment)
{
  subProgName = subProgram;

  const char * prog = NULL;
  PStringStream cmdLine;
  if (searchPath)
    cmdLine << subProgram;
  else
    prog = subProgram;

  for (PINDEX i = 0; i < argumentList.GetSize(); i++) {
    cmdLine << ' ';
    if (argumentList[i].Find(' ') == P_MAX_INDEX)
      cmdLine << argumentList[i];
    else if (argumentList[i].Find('"') == P_MAX_INDEX)
      cmdLine << '"' << argumentList[i] << '"';
    else
      cmdLine << '\'' << argumentList[i] << '\'';
  }

  PCharArray envBuf;
  char * envStr = NULL;
  if (environment != NULL) {
    PINDEX size = 0;
    for (PINDEX e = 0; e < environment->GetSize(); e++) {
      PString str = environment->GetKeyAt(e) + '=' + environment->GetDataAt(e);
      PINDEX len = str.GetLength() + 1;
      envBuf.SetSize(size + len);
      memcpy(envBuf.GetPointer()+size, (const char *)str, len);
      size += len;
    }
    envStr = envBuf.GetPointer();
  }

  //
  // this code comes from http://msdn.microsoft.com/en-us/library/ms682499(VS.85).aspx
  //
  STARTUPINFO startup;
  memset(&startup, 0, sizeof(startup));
  startup.cb = sizeof(startup);
  startup.dwFlags = STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES;
  startup.wShowWindow = SW_HIDE;
  startup.hStdInput = INVALID_HANDLE_VALUE;
  startup.hStdOutput = INVALID_HANDLE_VALUE;
  startup.hStdError = INVALID_HANDLE_VALUE;

  SECURITY_ATTRIBUTES security;
  security.nLength = sizeof(security);
  security.lpSecurityDescriptor = NULL;
  security.bInheritHandle = TRUE;

  // ReadOnly means child has no stdin
  // otherwise create a pipe for us to send data to child
  if (mode == ReadOnly)
    hToChild = INVALID_HANDLE_VALUE;
  else {
    HANDLE writeEnd;
    PAssertOS(CreatePipe(&startup.hStdInput, &writeEnd, &security, 0));
    PAssertOS(SetHandleInformation(writeEnd, HANDLE_FLAG_INHERIT, 0));
    PAssertOS(DuplicateHandle(GetCurrentProcess(), writeEnd,
                              GetCurrentProcess(), &hToChild, 0, PFalse,
                              DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS));
  }

  // WriteOnly means child has no stdout
  // ReadWriteStd means child uses our stdout and stderr
  // otherwise, create a pipe to read stdout from child, and perhaps a seperate one for stderr too
  if (mode == WriteOnly)
    hFromChild = INVALID_HANDLE_VALUE;
  else if (mode == ReadWriteStd) {
    hFromChild = INVALID_HANDLE_VALUE;
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  }
  else {
    PAssertOS(CreatePipe(&hFromChild, &startup.hStdOutput, &security, 1));
    PAssertOS(SetHandleInformation(hFromChild, HANDLE_FLAG_INHERIT, 0));
    if (stderrSeparate) {
      PAssertOS(CreatePipe(&hStandardError, &startup.hStdError, &security, 1));
      PAssertOS(SetHandleInformation(hStandardError, HANDLE_FLAG_INHERIT, 0));
    }
    else {
      startup.hStdError = startup.hStdOutput;
      hStandardError = INVALID_HANDLE_VALUE;
    }
  }

  if (ConvertOSError(CreateProcess(prog, cmdLine.GetPointer(),
                                   NULL, NULL, PTrue, 0, envStr,
                                   NULL, &startup, &info) ? 0 : -2))
    os_handle = info.dwProcessId;
  else {
    if (hToChild != INVALID_HANDLE_VALUE)
      CloseHandle(hToChild);
    if (hFromChild != INVALID_HANDLE_VALUE)
      CloseHandle(hFromChild);
    if (hStandardError != INVALID_HANDLE_VALUE)
      CloseHandle(hStandardError);
  }

  if (startup.hStdInput != INVALID_HANDLE_VALUE)
    CloseHandle(startup.hStdInput);

  if (mode != ReadWriteStd) {
    if (startup.hStdOutput != INVALID_HANDLE_VALUE)
      CloseHandle(startup.hStdOutput);
    if (startup.hStdOutput != startup.hStdError)
      CloseHandle(startup.hStdError);
  }

  return IsOpen();
}
#endif // !_WIN32_WCE



PPipeChannel::~PPipeChannel()
{
  Close();
}


PBoolean PPipeChannel::IsOpen() const
{
  return os_handle != -1;
}


int PPipeChannel::GetReturnCode() const
{
  DWORD code;
  if (GetExitCodeProcess(info.hProcess, &code))
    return code != STILL_ACTIVE ? code : -2;

  ((PPipeChannel*)this)->ConvertOSError(-2);
  return -1;
}


PBoolean PPipeChannel::CanReadAndWrite()
{
  return PTrue;
}

PBoolean PPipeChannel::IsRunning() const
{
  return GetReturnCode() == -2;
}


int PPipeChannel::WaitForTermination()
{
  if (WaitForSingleObject(info.hProcess, INFINITE) == WAIT_OBJECT_0)
    return GetReturnCode();

  ConvertOSError(-2);
  return -1;
}


int PPipeChannel::WaitForTermination(const PTimeInterval & timeout)
{
  if (WaitForSingleObject(info.hProcess, timeout.GetInterval()) == WAIT_OBJECT_0)
    return GetReturnCode();

  ConvertOSError(-2);
  return -1;
}


PBoolean PPipeChannel::Kill(int signal)
{
  return ConvertOSError(TerminateProcess(info.hProcess, signal) ? 0 : -2);
}


PBoolean PPipeChannel::Read(void * buffer, PINDEX len)
{
  lastReadCount = 0;

  DWORD count = 0;

#ifndef _WIN32_WCE
  // Cannot use overlapped I/O with anonymous pipe.
  // So have all this hideous code. :-(

  if (readTimeout == PMaxTimeInterval) {
    if (!ConvertOSError(ReadFile(hFromChild, buffer, 1, &count, NULL) ? 0 : -2, LastReadError))
      return false;

    lastReadCount = 1;
    if (len == 1)
      return true;

    if (!PeekNamedPipe(hFromChild, NULL, 0, NULL, &count, NULL))
      return ConvertOSError(-2, LastReadError);

    if (count == 0)
      return true;

    ++((BYTE * &)buffer);
    --len;
  }
  else {
    PSimpleTimer timeout(readTimeout);
    for (;;) {
      if (!PeekNamedPipe(hFromChild, NULL, 0, NULL, &count, NULL))
        return ConvertOSError(-2, LastReadError);

      if (count > 0)
        break;

      if (timeout.HasExpired()) {
        SetErrorValues(Timeout, EAGAIN, LastReadError);
        return false;
      }

      Sleep(10);
    }
  }

  if (len > (PINDEX)count)
    len = count;
#endif

  if (!ConvertOSError(ReadFile(hFromChild, buffer, len, &count, NULL) ? 0 : -2, LastReadError))
    return false;

  lastReadCount += count;
  return lastReadCount > 0;
}
      

PBoolean PPipeChannel::Write(const void * buffer, PINDEX len)
{
  lastWriteCount = 0;
  DWORD count;
  if (!ConvertOSError(WriteFile(hToChild, buffer, len, &count, NULL) ? 0 : -2, LastWriteError))
    return PFalse;
  lastWriteCount = count;
  return lastWriteCount >= len;
}


PBoolean PPipeChannel::Close()
{
  if (IsOpen()) {
    os_handle = -1;
    if (hToChild != INVALID_HANDLE_VALUE)
      CloseHandle(hToChild);
    if (hFromChild != INVALID_HANDLE_VALUE)
      CloseHandle(hFromChild);
    if (hStandardError != INVALID_HANDLE_VALUE)
      CloseHandle(hStandardError);
    if (!TerminateProcess(info.hProcess, 1))
      return PFalse;
  }
  return PTrue;
}


PBoolean PPipeChannel::Execute()
{
  flush();
  clear();
  if (hToChild != INVALID_HANDLE_VALUE)
    CloseHandle(hToChild);
  hToChild = INVALID_HANDLE_VALUE;
  return IsRunning();
}


#ifdef _WIN32_WCE
PBoolean PPipeChannel::ReadStandardError(PString &, PBoolean)
{
  return PFalse;
}
#else
PBoolean PPipeChannel::ReadStandardError(PString & errors, PBoolean wait)
{
  DWORD available, bytesRead;
  if (!PeekNamedPipe(hStandardError, NULL, 0, NULL, &available, NULL))
    return ConvertOSError(-2, LastReadError);

  if (available != 0)
    return ConvertOSError(ReadFile(hStandardError,
                          errors.GetPointer(available+1), available,
                          &bytesRead, NULL) ? 0 : -2, LastReadError);

  if (!wait)
    return PFalse;

  char firstByte;
  if (!ReadFile(hStandardError, &firstByte, 1, &bytesRead, NULL))
    return ConvertOSError(-2, LastReadError);

  errors = firstByte;

  if (!PeekNamedPipe(hStandardError, NULL, 0, NULL, &available, NULL))
    return ConvertOSError(-2, LastReadError);

  if (available == 0)
    return PTrue;

  return ConvertOSError(ReadFile(hStandardError,
                        errors.GetPointer(available+2)+1, available,
                        &bytesRead, NULL) ? 0 : -2, LastReadError);
}
#endif // !_WIN32_WCE


// End Of File ///////////////////////////////////////////////////////////////
