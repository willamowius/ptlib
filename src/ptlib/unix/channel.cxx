/*
 * channel.cxx
 *
 * I/O channel classes implementation
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

#pragma implementation "channel.h"
#pragma implementation "indchan.h"

#include <ptlib.h>
#include <sys/ioctl.h>


#include "../common/pchannel.cxx"


#ifdef P_NEED_IOSTREAM_MUTEX
static PMutex iostreamMutex;
#define IOSTREAM_MUTEX_WAIT()   iostreamMutex.Wait();
#define IOSTREAM_MUTEX_SIGNAL() iostreamMutex.Signal();
#else
#define IOSTREAM_MUTEX_WAIT()
#define IOSTREAM_MUTEX_SIGNAL()
#endif


void PChannel::Construct()
{
  os_handle = -1;
  px_lastBlockType = PXReadBlock;
  px_readThread = NULL;
  px_writeThread = NULL;
  px_selectThread[0] = NULL;
  px_selectThread[1] = NULL;
  px_selectThread[2] = NULL;
}


///////////////////////////////////////////////////////////////////////////////
//
// PChannel::PXSetIOBlock
//   This function is used to perform IO blocks.
//   If the return value is PFalse, then the select call either
//   returned an error or a timeout occurred. The member variable lastError
//   can be used to determine which error occurred
//

PBoolean PChannel::PXSetIOBlock(PXBlockType type, const PTimeInterval & timeout)
{
  ErrorGroup group;
  switch (type) {
    case PXReadBlock :
      group = LastReadError;
      break;
    case PXWriteBlock :
      group = LastWriteError;
      break;
    default :
      group = LastGeneralError;
  }

  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF, group);

  PThread * blockedThread = PThread::Current();

  {
    PWaitAndSignal mutex(px_threadMutex);
    switch (type) {
      case PXWriteBlock :
        if (px_readThread != NULL && px_lastBlockType != PXReadBlock)
          return SetErrorValues(DeviceInUse, EBUSY, LastReadError);

        PTRACE(6, "PWLib\tBlocking on write.");
        px_writeMutex.Wait();
        px_writeThread = blockedThread;
        break;

      case PXReadBlock :
        if (px_readThread != NULL && px_lastBlockType == PXReadBlock)
          PAssertAlways(psprintf("Attempt to do simultaneous reads from multiple threads:"
                                 " os_handle=%i, thread ID=" PTHREAD_ID_FMT,
                                 os_handle, px_readThread->GetThreadId()));
        // Fall into default case

      default :
        if (px_readThread != NULL)
          return SetErrorValues(DeviceInUse, EBUSY, LastReadError);
        px_readThread = blockedThread;
        px_lastBlockType = type;
    }
  }

  int stat = blockedThread->PXBlockOnIO(os_handle, type, timeout);

  px_threadMutex.Wait();
  if (type != PXWriteBlock) {
    px_lastBlockType = PXReadBlock;
    px_readThread = NULL;
  }
  else {
    px_writeThread = NULL;
    px_writeMutex.Signal();
  }
  px_threadMutex.Signal();

  // if select returned < 0, then convert errno into lastError and return PFalse
  if (stat < 0)
    return ConvertOSError(stat, group);

  // if the select succeeded, then return PTrue
  if (stat > 0) 
    return PTrue;

  // otherwise, a timeout occurred so return PFalse
  return SetErrorValues(Timeout, ETIMEDOUT, group);
}


PBoolean PChannel::Read(void * buf, PINDEX len)
{
  lastReadCount = 0;

  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF, LastReadError);

  while ((lastReadCount = ::read(os_handle, buf, len)) < 0) {
    switch (errno) {
      case EINTR :
        break;

      case EWOULDBLOCK :
        if (readTimeout > 0) {
          if (PXSetIOBlock(PXReadBlock, readTimeout))
            break;
          return false;
        }
        // Next case

      default :
        return ConvertOSError(-1);
    }
  }

  return lastReadCount > 0;
}


PBoolean PChannel::Write(const void * buf, PINDEX len)
{
  lastWriteCount = 0;

  // if the os_handle isn't open, no can do
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF, LastWriteError);

  // flush the buffer before doing a write
  IOSTREAM_MUTEX_WAIT();
  flush();
  IOSTREAM_MUTEX_SIGNAL();

  while (len > 0) {

    int result;
    while ((result = ::write(os_handle, ((char *)buf)+lastWriteCount, len)) < 0) {
      switch (errno) {
        case EINTR :
          break;

        case EWOULDBLOCK :
          if (writeTimeout > 0) {
            if (PXSetIOBlock(PXWriteBlock, writeTimeout))
              break;
            return false;
          }
          // Next case

        default :
          return ConvertOSError(-1, LastReadError);
      }
    }

    lastWriteCount += result;
    len -= result;
  }

#if !defined(P_PTHREADS) && !defined(P_MAC_MPTHREADS)
  PThread::Yield(); // Starvation prevention
#endif

  // Reset all the errors.
  return ConvertOSError(0, LastWriteError);
}

PBoolean PChannel::Write(const void * buf, PINDEX len, const void * /*mark*/)
{
   return Write(buf,len);
}

#ifdef P_HAS_RECVMSG

PBoolean PChannel::Read(const VectorOfSlice & slices)
{
  lastReadCount = 0;

  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF, LastReadError);

  if (!PXSetIOBlock(PXReadBlock, readTimeout)) 
    return PFalse;

  if (ConvertOSError(lastReadCount = ::readv(os_handle, &slices[0], slices.size()), LastReadError))
    return lastReadCount > 0;

  lastReadCount = 0;
  return PFalse;
}

PBoolean PChannel::Write(const VectorOfSlice & slices)
{
  // if the os_handle isn't open, no can do
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF, LastWriteError);

  // flush the buffer before doing a write
  IOSTREAM_MUTEX_WAIT();
  flush();
  IOSTREAM_MUTEX_SIGNAL();

  int result;
  while ((result = ::writev(os_handle, &slices[0], slices.size())) < 0) {
    if (errno != EWOULDBLOCK)
      return ConvertOSError(-1, LastWriteError);

    if (!PXSetIOBlock(PXWriteBlock, writeTimeout))
      return PFalse;
  }

#if !defined(P_PTHREADS) && !defined(P_MAC_MPTHREADS)
  PThread::Yield(); // Starvation prevention
#endif

  // Reset all the errors.
  return ConvertOSError(0, LastWriteError);
}

#endif

PBoolean PChannel::Close()
{
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF);
  
  return ConvertOSError(PXClose());
}


static void AbortIO(PThread * & thread, PMutex & mutex)
{
  mutex.Wait();
  if (thread != NULL)
    thread->PXAbortBlock();
  mutex.Signal();

  while (thread != NULL)
    PThread::Yield();
}

int PChannel::PXClose()
{
  if (os_handle < 0)
    return -1;

  PTRACE(6, "PWLib\tClosing channel, fd=" << os_handle);

  // make sure we don't have any problems
  IOSTREAM_MUTEX_WAIT();
  flush();
  int handle = os_handle;
  os_handle = -1;
  IOSTREAM_MUTEX_SIGNAL();

#if !defined(P_PTHREADS) && !defined(BE_THREADS) && !defined(P_MAC_MPTHREADS) && !defined(VX_TASKS)
  // abort any I/O block using this os_handle
  PProcess::Current().PXAbortIOBlock(handle);

#ifndef BE_BONELESS
  DWORD cmd = 0;
  ::ioctl(handle, FIONBIO, &cmd);
#endif
#endif

  AbortIO(px_readThread, px_threadMutex);
  AbortIO(px_writeThread, px_threadMutex);
  AbortIO(px_selectThread[0], px_threadMutex);
  AbortIO(px_selectThread[1], px_threadMutex);
  AbortIO(px_selectThread[2], px_threadMutex);

  int stat;
  do {
    stat = ::close(handle);
  } while (stat == -1 && errno == EINTR);

  return stat;
}

PString PChannel::GetErrorText(Errors normalisedError, int osError /* =0 */)
{
  if (osError == 0) {
    if (normalisedError == NoError)
      return PString();

    static int const errors[NumNormalisedErrors] = {
      0, ENOENT, EEXIST, ENOSPC, EACCES, EBUSY, EINVAL, ENOMEM, EBADF, EAGAIN, EINTR,
      EMSGSIZE, EIO, 0x1000000
    };
    osError = errors[normalisedError];
  }

  if (osError == 0x1000000)
    return "High level protocol failure";

  const char * err = strerror(osError);
  if (err != NULL)
    return err;

  return psprintf("Unknown error %d", osError);
}


PBoolean PChannel::ConvertOSError(int err, Errors & lastError, int & osError)

{
  osError = (err >= 0) ? 0 : errno;

  switch (osError) {
    case 0 :
      lastError = NoError;
      return PTrue;

    case EMSGSIZE:
      lastError = BufferTooSmall;
      break;

    case EBADF:  // will get EBADF if a read/write occurs after closing. This must return Interrupted
    case EINTR:
      lastError = Interrupted;
      break;

    case EWOULDBLOCK :
    case ETIMEDOUT :
      lastError = Timeout;
      break;

    case EEXIST:
      lastError = FileExists;
      break;

    case EISDIR:
    case EROFS:
    case EACCES:
    case EPERM:
      lastError = AccessDenied;
      break;

#ifndef __BEOS__
    case ETXTBSY:
      lastError = DeviceInUse;
      break;
#endif

    case EFAULT:
    case ELOOP:
    case EINVAL:
      lastError = BadParameter;
      break;

    case ENOENT :
    case ENAMETOOLONG:
    case ENOTDIR:
      lastError = NotFound;
      break;

    case EMFILE:
    case ENFILE:
    case ENOMEM :
      lastError = NoMemory;
      break;

    case ENOSPC:
      lastError = DiskFull;
      break;

    default :
      lastError = Miscellaneous;
      break;
  }
  return PFalse;
}


///////////////////////////////////////////////////////////////////////////////

