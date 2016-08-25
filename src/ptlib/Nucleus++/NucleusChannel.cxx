/*
 * channel.cxx
 *
 * I/O channel classes implementation
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
 *
 * Copyright (c) 1999 ISDN Communications Ltd.
 *
 */

#include <ptlib.h>
#include <sys/socket.h>
#include "../common/pchannel.cxx"
#define new PNEW

PBoolean PChannel::Read(void *, PINDEX)
{
  PAssertAlways(PUnimplementedFunction);
  return PFalse;
}


PBoolean PChannel::Write(const void *, PINDEX)
{
  PAssertAlways(PUnimplementedFunction);
  return PFalse;
}

PBoolean PChannel::Write(const void * buf, PINDEX len, const void * /*mark*/)
{
  return Write(buf,len);
}

PBoolean PChannel::Close()
{
   ::shutdown(os_handle,2);
   os_handle=-1;
   return PFalse;
}


PString PChannel::GetErrorText(Errors, int osError)
{
  if (osError == 0)
    return PString();

  const char * err = strerror(osError);
  if (err != NULL)
    return err;

  return psprintf("Unknown error %d", osError);
}

PString PChannel::GetErrorText() const
{
  return GetErrorText(lastError, osError);
}

PBoolean PChannel::ConvertOSError(int err)
{
  return ConvertOSError(err, lastError, osError);
}

PBoolean PChannel::ConvertOSError(int err, Errors & lastError, int & osError)

{
  osError = (err >= 0) ? 0 : errno;

  switch (osError) {
    case 0 :
      lastError = NoError;
      return PTrue;

    case EINTR:
      lastError = Interrupted;
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

    case EFAULT:
    case EBADF:
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

PBoolean PChannel::PXSetIOBlock (int type, const PTimeInterval & timeout)
{
  return PXSetIOBlock(type, os_handle, timeout);
}

PBoolean PChannel::PXSetIOBlock (int type, int blockHandle, const PTimeInterval & timeout)
{
  if (blockHandle < 0) {
    lastError = NotOpen;
    return PFalse;
  }

  int stat = PThread::Current()->PXBlockOnIO(blockHandle, type, timeout);

  // if select returned < 0, then covert errno into lastError and return PFalse
  if (stat < 0)
    return ConvertOSError(stat);

  // if the select succeeded, then return PTrue
  if (stat > 0) 
    return PTrue;

  // otherwise, a timeout occurred so return PFalse
  lastError = Timeout;
  return PFalse;
}

int PChannel::PXClose()
{
  if (os_handle < 0)
    return -1;

  // make sure we don't have any problems
  int handle = os_handle;
  int stat;
  do {
    stat = shutdown(handle,2 );
  } while (stat == -1 && errno == EINTR);

  return stat;
}


///////////////////////////////////////////////////////////////////////////////

