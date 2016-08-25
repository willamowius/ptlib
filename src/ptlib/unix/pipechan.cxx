/*
 * pipechan.cxx
 *
 * Sub-process commuicating with pip I/O channel implementation
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

#pragma implementation "pipechan.h"

#include <ptlib.h>
#include <ptlib/pipechan.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#if defined(P_LINUX) || defined(P_SOLARIS)
#include <termio.h>
#endif

#include "../common/pipechan.cxx"

#if defined(P_MACOSX) && !defined(P_IPHONEOS)
#  include <crt_externs.h>
#  define environ (*_NSGetEnviron())
#else
extern char ** environ;
#endif


int PX_NewHandle(const char *, int);


////////////////////////////////////////////////////////////////
//
//  PPipeChannel
//

PPipeChannel::PPipeChannel()
{
  toChildPipe[0] = toChildPipe[1] = -1;
  fromChildPipe[0] = fromChildPipe[1] = -1;
  stderrChildPipe[0] = stderrChildPipe[1] = -1;
}


PBoolean PPipeChannel::PlatformOpen(const PString & subProgram,
                                const PStringArray & argumentList,
                                OpenMode mode,
                                PBoolean searchPath,
                                PBoolean stderrSeparate,
                                const PStringToString * environment)
{
#if defined(P_VXWORKS) || defined(P_RTEMS)
  PAssertAlways("PPipeChannel::PlatformOpen");
  return PFalse;
#else
  subProgName = subProgram;

  // setup the pipe to the child
  if (mode == ReadOnly)
    toChildPipe[0] = toChildPipe[1] = -1;
  else {
    PAssert(pipe(toChildPipe) == 0, POperatingSystemError);
    PX_NewHandle("PPipeChannel toChildPipe", PMAX(toChildPipe[0], toChildPipe[1]));
  }
 
  // setup the pipe from the child
  if (mode == WriteOnly || mode == ReadWriteStd)
    fromChildPipe[0] = fromChildPipe[1] = -1;
  else {
    PAssert(pipe(fromChildPipe) == 0, POperatingSystemError);
    PX_NewHandle("PPipeChannel fromChildPipe", PMAX(fromChildPipe[0], fromChildPipe[1]));
  }

  if (stderrSeparate)
    PAssert(pipe(stderrChildPipe) == 0, POperatingSystemError);
  else {
    stderrChildPipe[0] = stderrChildPipe[1] = -1;
    PX_NewHandle("PPipeChannel stderrChildPipe", PMAX(stderrChildPipe[0], stderrChildPipe[1]));
  }

  // Set up new environment if one specified.
  char ** exec_environ = environ;
  if (environment != NULL || !searchPath) {
    exec_environ = (char **)calloc(environment->GetSize()+1, sizeof(char*));
    for (PINDEX i = 0; i < environment->GetSize(); i++) {
      PString key(environment->GetKeyAt(i));
      if (searchPath || (key != "PATH")) {
        PString str = key + '=' + environment->GetDataAt(i);
        exec_environ[i] = strdup(str);
      }
    }
  }

  // fork to allow us to execute the child
#if defined(__BEOS__) || defined(P_IRIX)
  childPid = fork();
#else
  childPid = vfork();
#endif
  if (childPid < 0)
    return PFalse;

  if (childPid > 0) {
    // setup the pipe to the child
    if (toChildPipe[0] != -1) {
      ::close(toChildPipe[0]);
      toChildPipe[0] = -1;
    }

    if (fromChildPipe[1] != -1) {
      ::close(fromChildPipe[1]);
      fromChildPipe[1] = -1;
    }
 
    if (stderrChildPipe[1] != -1) {
      ::close(stderrChildPipe[1]);
      stderrChildPipe[1] = -1;
    }

    if (exec_environ != environ)
      free(exec_environ);
 
    os_handle = 0;
    return PTrue;
  }

  // the following code is in the child process

  // if we need to write to the child, make sure the child's stdin
  // is redirected
  if (toChildPipe[0] != -1) {
    ::close(STDIN_FILENO);
    if (::dup(toChildPipe[0]) == -1)
      return false;
    ::close(toChildPipe[0]);
    ::close(toChildPipe[1]);  
  } else {
    int fd = open("/dev/null", O_RDONLY);
    PAssertOS(fd >= 0);
    ::close(STDIN_FILENO);
    if (::dup(fd) == -1)
      return false;
    ::close(fd);
  }

  // if we need to read from the child, make sure the child's stdout
  // and stderr is redirected
  if (fromChildPipe[1] != -1) {
    ::close(STDOUT_FILENO);
    if (::dup(fromChildPipe[1]) == -1)
      return false;
    ::close(STDERR_FILENO);
    if (!stderrSeparate)
      if (::dup(fromChildPipe[1]) == -1)
        return false;
    ::close(fromChildPipe[1]);
    ::close(fromChildPipe[0]); 
  } else if (mode != ReadWriteStd) {
    int fd = ::open("/dev/null", O_WRONLY);
    PAssertOS(fd >= 0);
    ::close(STDOUT_FILENO);
    if (::dup(fd) == -1)
      return false;
    ::close(STDERR_FILENO);
    if (!stderrSeparate)
      if (::dup(fd) == -1)
        return false;
    ::close(fd);
  }

  if (stderrSeparate) {
    if (::dup(stderrChildPipe[1]) == -1)
      return false;
    ::close(stderrChildPipe[1]);
    ::close(stderrChildPipe[0]); 
  }

  // set the SIGINT and SIGQUIT to ignore so the child process doesn't
  // inherit them from the parent
  signal(SIGINT,  SIG_IGN);
  signal(SIGQUIT, SIG_IGN);

  // and set ourselves as out own process group so we don't get signals
  // from our parent's terminal (hopefully!)
  PSETPGRP();

  // setup the arguments, not as we are about to execl or exit, we don't
  // care about memory leaks, they are not real!
  char ** args = (char **)calloc(argumentList.GetSize()+2, sizeof(char *));
  args[0] = strdup(subProgName.GetTitle());
  PINDEX i;
  for (i = 0; i < argumentList.GetSize(); i++) 
    args[i+1] = strdup(argumentList[i].GetPointer());

  // run the program
  execve(subProgram, args, exec_environ);

  _exit(2);
  return PFalse;
#endif // P_VXWORKS || P_RTEMS
}


PBoolean PPipeChannel::Close()
{
  bool wasRunning = false;

  // close pipe from child
  if (fromChildPipe[0] != -1) {
    ::close(fromChildPipe[0]);
    fromChildPipe[0] = -1;
  }

  if (fromChildPipe[1] != -1) {
    ::close(fromChildPipe[1]);
    fromChildPipe[1] = -1;
  }

  // close pipe to child
  if (toChildPipe[0] != -1) {
    ::close(toChildPipe[0]);
    toChildPipe[0] = -1;
  }

  if (toChildPipe[1] != -1) {
    ::close(toChildPipe[1]);
    toChildPipe[1] = -1;
  }

  // close pipe to child
  if (stderrChildPipe[0] != -1) {
    ::close(stderrChildPipe[0]);
    stderrChildPipe[0] = -1;
  }

  if (stderrChildPipe[1] != -1) {
    ::close(stderrChildPipe[1]);
    stderrChildPipe[1] = -1;
  }

  // kill the child process
  if (IsRunning()) {
    wasRunning = true;
    PTRACE(4, "PipeChannel\tChild being sent SIGKILL");
    kill(childPid, SIGKILL);
    WaitForTermination();
  }

  // ensure this channel looks like it is closed
  os_handle = -1;
  childPid  = 0;

  return wasRunning;
}

PBoolean PPipeChannel::Read(void * buffer, PINDEX len)
{
  PAssert(IsOpen(), "Attempt to read from closed pipe");
  PAssert(fromChildPipe[0] != -1, "Attempt to read from write-only pipe");

  os_handle = fromChildPipe[0];
  PBoolean status = PChannel::Read(buffer, len);
  os_handle = 0;
  return status;
}

PBoolean PPipeChannel::Write(const void * buffer, PINDEX len)
{
  PAssert(IsOpen(), "Attempt to write to closed pipe");
  PAssert(toChildPipe[1] != -1, "Attempt to write to read-only pipe");

  os_handle = toChildPipe[1];
  PBoolean status = PChannel::Write(buffer, len);
  os_handle = 0;
  return status;
}

PBoolean PPipeChannel::Execute()
{
  flush();
  clear();
  if (toChildPipe[1] != -1) {
    ::close(toChildPipe[1]);
    toChildPipe[1] = -1;
  }
  return PTrue;
}


PPipeChannel::~PPipeChannel()
{
  Close();
}

int PPipeChannel::GetReturnCode() const
{
  return retVal;
}

PBoolean PPipeChannel::IsRunning() const
{
  if (childPid == 0)
    return PFalse;

#if defined(P_PTHREADS) || defined(P_MAC_MPTHREADS)

  int err;
  int status;
  if ((err = waitpid(childPid, &status, WNOHANG)) == 0)
    return PTrue;

  if (err != childPid)
    return PFalse;

  PPipeChannel * thisW = (PPipeChannel *)this;
  thisW->childPid = 0;

  if (WIFEXITED(status)) {
    thisW->retVal = WEXITSTATUS(status);
    PTRACE(2, "PipeChannel\tChild exited with code " << retVal);
  } else if (WIFSIGNALED(status)) {
    PTRACE(2, "PipeChannel\tChild was signalled with " << WTERMSIG(status));
    thisW->retVal = -1;
  } else if (WIFSTOPPED(status)) {
    PTRACE(2, "PipeChannel\tChild was stopped with " << WSTOPSIG(status));
    thisW->retVal = -1;
  } else {
    PTRACE(2, "PipeChannel\tChild was stopped with unknown status" << status);
    thisW->retVal = -1;
  }

  return PFalse;

#else
  return kill(childPid, 0) == 0;
#endif
}

int PPipeChannel::WaitForTermination()
{
  if (childPid == 0)
    return retVal;

  int err;

#if defined(P_PTHREADS) || defined(P_MAC_MPTHREADS)
  int status;
  do {
    err = waitpid(childPid, &status, 0);
    if (err == childPid) {
      childPid = 0;
      if (WIFEXITED(status)) {
        retVal = WEXITSTATUS(status);
        PTRACE(2, "PipeChannel\tChild exited with code " << retVal);
      } else if (WIFSIGNALED(status)) {
        PTRACE(2, "PipeChannel\tChild was signalled with " << WTERMSIG(status));
        retVal = -1;
      } else if (WIFSTOPPED(status)) {
        PTRACE(2, "PipeChannel\tChild was stopped with " << WSTOPSIG(status));
        retVal = -1;
      } else {
        PTRACE(2, "PipeChannel\tChild was stopped with unknown status" << status);
        retVal = -1;
      }
      return retVal;
    }
  } while (errno == EINTR);
#else
  if ((err = kill (childPid, 0)) == 0)
    return retVal = PThread::Current()->PXBlockOnChildTerminate(childPid, PMaxTimeInterval);
#endif

  ConvertOSError(err);
  return -1;
}

int PPipeChannel::WaitForTermination(const PTimeInterval & timeout)
{
  if (childPid == 0)
    return retVal;

  int err;

#if defined(P_PTHREADS) || defined(P_MAC_MPTHREADS)
  PAssert(timeout == PMaxTimeInterval, PUnimplementedFunction);
  int status;
  do {
    err = waitpid(childPid, &status, 0);
    if (err == childPid) {
      childPid = 0;
      if (WIFEXITED(status)) {
        retVal = WEXITSTATUS(status);
        PTRACE(2, "PipeChannel\tChild exited with code " << retVal);
      } else if (WIFSIGNALED(status)) {
        PTRACE(2, "PipeChannel\tChild was signalled with " << WTERMSIG(status));
        retVal = -1;
      } else if (WIFSTOPPED(status)) {
        PTRACE(2, "PipeChannel\tChild was stopped with " << WSTOPSIG(status));
        retVal = -1;
      } else {
        PTRACE(2, "PipeChannel\tChild was stopped with unknown status" << status);
        retVal = -1;
      }
      return retVal;
    }
  } while (errno == EINTR);
#else
  if ((err = kill (childPid, 0)) == 0)
    return retVal = PThread::Current()->PXBlockOnChildTerminate(childPid, timeout);
#endif

  ConvertOSError(err);
  return -1;
}

PBoolean PPipeChannel::Kill(int killType)
{
  PTRACE(4, "PipeChannel\tChild being sent signal " << killType);
  return ConvertOSError(kill(childPid, killType));
}

PBoolean PPipeChannel::CanReadAndWrite()
{
  return PTrue;
}


PBoolean PPipeChannel::ReadStandardError(PString & errors, PBoolean wait)
{
  PAssert(IsOpen(), "Attempt to read from closed pipe");
  PAssert(stderrChildPipe[0] != -1, "Attempt to read from write-only pipe");

  os_handle = stderrChildPipe[0];
  
  PBoolean status = PFalse;
#ifndef BE_BONELESS
  int available;
  if (ConvertOSError(ioctl(stderrChildPipe[0], FIONREAD, &available))) {
    if (available != 0)
      status = PChannel::Read(errors.GetPointer(available+1), available);
    else if (wait) {
      char firstByte;
      status = PChannel::Read(&firstByte, 1);
      if (status) {
        errors = firstByte;
        if (ConvertOSError(ioctl(stderrChildPipe[0], FIONREAD, &available))) {
          if (available != 0)
            status = PChannel::Read(errors.GetPointer(available+2)+1, available);
        }
      }
    }
  }
#endif

  os_handle = 0;
  return status;
}


