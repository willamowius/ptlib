/*
 * pchannel.cxx
 *
 * Operating System utilities.
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

#include <ctype.h>


#define new PNEW


///////////////////////////////////////////////////////////////////////////////
// PChannel

PChannelStreamBuffer::PChannelStreamBuffer(PChannel * chan)
  : channel(PAssertNULL(chan))
{
}


PBoolean PChannelStreamBuffer::SetBufferSize(PINDEX newSize)
{
  return input.SetSize(newSize) && output.SetSize(newSize);
}


streambuf::int_type PChannelStreamBuffer::overflow(int_type c)
{
  if (pbase() == NULL) {
    char * p = output.GetPointer(1024);
    setp(p, p+output.GetSize());
  }

  int bufSize = pptr() - pbase();
  if (bufSize > 0) {
    setp(pbase(), epptr());
    if (!channel->Write(pbase(), bufSize))
      return EOF;
  }

  if (c != EOF) {
    *pptr() = (char)c;
    pbump(1);
  }

  return 0;
}


streambuf::int_type PChannelStreamBuffer::underflow()
{
  if (eback() == NULL) {
    char * p = input.GetPointer(1024);
    char * e = p+input.GetSize();
    setg(p, e, e);
  }

  if (gptr() != egptr())
    return (BYTE)*gptr();

  if (!channel->Read(eback(), egptr() - eback()) ||
                                  channel->GetErrorCode() != PChannel::NoError)
    return EOF;

  PINDEX count = channel->GetLastReadCount();
  char * p = egptr() - count;
  memmove(p, eback(), count);
  setg(eback(), p, egptr());
  return (BYTE)*p;
}


int PChannelStreamBuffer::sync()
{
  int inAvail = egptr() - gptr();
  if (inAvail > 0) {
    setg(eback(), egptr(), egptr());
    if (PIsDescendant(channel, PFile))
      ((PFile *)channel)->SetPosition(-inAvail, PFile::Current);
  }

  if (pptr() > pbase())
    return overflow();

  return 0;
}


PChannelStreamBuffer::pos_type PChannelStreamBuffer::seekoff(off_type off, ios_base::seekdir dir, ios_base::openmode)
{
  sync();
  if (PIsDescendant(channel, PFile)) {
    PFile * file = (PFile *)channel;
    file->SetPosition((off_t)off, (PFile::FilePositionOrigin)dir);
    return file->GetPosition();
  }

  // If we have an input stream and the buffer is empty then force a read so
  // we can seek ahead.
  if (egptr() == gptr()) {
    int c = underflow();
    if (c == EOF)
      return EOF;
  }

  while (off-- > 0) {
    if (sbumpc() == EOF)
      return EOF;
  }
    
  return egptr() - gptr();
}


PChannelStreamBuffer::pos_type PChannelStreamBuffer::seekpos(pos_type pos, ios_base::openmode mode)
{
  return seekoff(pos, ios_base::beg, mode);
}


#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

PChannel::PChannel()
  : iostream(new PChannelStreamBuffer(this)),
    readTimeout(PMaxTimeInterval), writeTimeout(PMaxTimeInterval)
{
  os_handle = -1;
  memset(lastErrorCode, 0, sizeof(lastErrorCode));
  memset(lastErrorNumber, 0, sizeof(lastErrorNumber));
  lastReadCount = lastWriteCount = 0;
  Construct();
}

#ifdef _MSC_VER
#pragma warning(default:4355)
#endif


PChannel::~PChannel()
{
  flush();
  Close();
  delete (PChannelStreamBuffer *)rdbuf();
#ifndef _WIN32
  init(NULL);
#endif
}

PObject::Comparison PChannel::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PChannel), PInvalidCast);
  int h1 = GetHandle();
  int h2 = ((const PChannel&)obj).GetHandle();
  if (h1 < h2)
    return LessThan;
  if (h1 > h2)
    return GreaterThan;
  return EqualTo;
}


PINDEX PChannel::HashFunction() const
{
  return GetHandle()%97;
}


PBoolean PChannel::IsOpen() const
{
  return os_handle != -1;
}

PINDEX PChannel::GetLastReadCount() const
{ 
  return lastReadCount; 
}

PINDEX PChannel::GetLastWriteCount() const
{ 
  return lastWriteCount; 
}

int PChannel::ReadChar()
{
  BYTE c;
  PBoolean retVal = Read(&c, 1);
  return (retVal && lastReadCount == 1) ? c : -1;
}


int PChannel::ReadCharWithTimeout(PTimeInterval & timeout)
{
  SetReadTimeout(timeout);
  PTimeInterval startTick = PTimer::Tick();
  int c;
  if ((c = ReadChar()) < 0) // Timeout or aborted
    return -1;
  timeout -= PTimer::Tick() - startTick;
  return c;
}


PBoolean PChannel::ReadBlock(void * buf, PINDEX len)
{
  char * ptr = (char *)buf;
  PINDEX numRead = 0;

  while (numRead < len && Read(ptr+numRead, len - numRead))
    numRead += lastReadCount;

  lastReadCount = numRead;

  return lastReadCount == len;
}


PString PChannel::ReadString(PINDEX len)
{
  PString str;

  if (len == P_MAX_INDEX) {
    PINDEX l = 0;
    for (;;) {
      char * p = l + str.GetPointer(l+1000+1);
      if (!Read(p, 1000))
        break;
      l += lastReadCount;
    }
    str.SetSize(l+1);

    /*Need to put in a null at the end to allow for MSDOS/Win32 text files
      which returns fewer bytes than actually read as it shrinks the data into
      the removed carriage returns, so it actually changes the buffer beyond
      what it indicated. */
    str[l] = '\0';
  }
  else {
    if (!ReadBlock(str.GetPointer(len+1), len))
      return PString::Empty();
  }

  return str;
}


PBoolean PChannel::WriteString(const PString & str)
{
  PINDEX len = str.GetLength();
  PINDEX written = 0;
  while (written < len) {
    if (!Write((const char *)str + written, len - written)) {
      lastWriteCount += written;
      return PFalse;
    }
    written += lastWriteCount;
  }
  lastWriteCount = written;
  return PTrue;
}


PBoolean PChannel::ReadAsync(void * buf, PINDEX len)
{
  PBoolean retVal = Read(buf, len);
  OnReadComplete(buf, lastReadCount);
  return retVal;
}


void PChannel::OnReadComplete(void *, PINDEX)
{
}


PBoolean PChannel::WriteChar(int c)
{
  PAssert(c >= 0 && c < 256, PInvalidParameter);
  char buf = (char)c;
  return Write(&buf, 1);
}


PBoolean PChannel::WriteAsync(const void * buf, PINDEX len)
{
  PBoolean retVal = Write(buf, len);
  OnWriteComplete(buf, lastWriteCount);
  return retVal;
}


void PChannel::OnWriteComplete(const void *, PINDEX)
{
}


PBoolean PChannel::SetBufferSize(PINDEX newSize)
{
  return ((PChannelStreamBuffer *)rdbuf())->SetBufferSize(newSize);
}


enum {
  NextCharEndOfString = -1,
  NextCharDelay = -2,
  NextCharSend = -3,
  NextCharWait = -4
};


static int HexDigit(char c)
{
  if (!isxdigit(c))
    return 0;

  int hex = c - '0';
  if (hex < 10)
    return hex;

  hex -= 'A' - '9' - 1;
  if (hex < 16)
    return hex;

  return hex - ('a' - 'A');
}


static int GetNextChar(const PString & command,
                                    PINDEX & pos, PTimeInterval * time = NULL)
{
  int temp;

  if (command[pos] == '\0')
    return NextCharEndOfString;

  if (command[pos] != '\\')
    return command[pos++];

  switch (command[++pos]) {
    case '\0' :
      return NextCharEndOfString;

    case 'a' : // alert (ascii value 7)
      pos++;
      return 7;

    case 'b' : // backspace (ascii value 8)
      pos++;
      return 8;

    case 'f' : // formfeed (ascii value 12)
      pos++;
      return 12;

    case 'n' : // newline (ascii value 10)
      pos++;
      return 10;

    case 'r' : // return (ascii value 13)
      pos++;
      return 13;

    case 't' : // horizontal tab (ascii value 9)
      pos++;
      return 9;

    case 'v' : // vertical tab (ascii value 11)
      pos++;
      return 11;

    case 'x' : // followed by hh  where nn is hex number (ascii value 0xhh)
      if (isxdigit(command[++pos])) {
        temp = HexDigit(command[pos++]);
        if (isxdigit(command[pos]))
          temp += HexDigit(command[pos++]);
        return temp;
      }
      return command[pos];

    case 's' :
      pos++;
      return NextCharSend;

    case 'd' : // ns  delay for n seconds/milliseconds
    case 'w' :
      temp = command[pos] == 'd' ? NextCharDelay : NextCharWait;
      long milliseconds = 0;
      while (isdigit(command[++pos]))
        milliseconds = milliseconds*10 + command[pos] - '0';
      if (milliseconds <= 0)
        milliseconds = 1;
      if (command[pos] == 'm')
        pos++;
      else {
        milliseconds *= 1000;
        if (command[pos] == 's')
          pos++;
      }
      if (time != NULL)
        *time = milliseconds;
      return temp;
  }

  if (command[pos] < '0' || command[pos] > '7')
    return command[pos++];

  // octal number
  temp = command[pos++] - '0';
  if (command[pos] < '0' || command[pos] > '7')
    return temp;

  temp += command[pos++] - '0';
  if (command[pos] < '0' || command[pos] > '7')
    return temp;

  temp += command[pos++] - '0';
  return temp;
}


PBoolean PChannel::ReceiveCommandString(int nextChar,
                            const PString & reply, PINDEX & pos, PINDEX start)
{
  if (nextChar != GetNextChar(reply, pos)) {
    pos = start;
    return PFalse;
  }

  PINDEX dummyPos = pos;
  return GetNextChar(reply, dummyPos) < 0;
}


PBoolean PChannel::SendCommandString(const PString & command)
{
  abortCommandString = PFalse;

  int nextChar;
  PINDEX sendPosition = 0;
  PTimeInterval timeout;
  SetWriteTimeout(10000);

  while (!abortCommandString) { // not aborted
    nextChar = GetNextChar(command, sendPosition, &timeout);
    switch (nextChar) {
      default :
        if (!WriteChar(nextChar))
          return PFalse;
        break;

      case NextCharEndOfString :
        return PTrue;  // Success!!

      case NextCharSend :
        break;

      case NextCharDelay : // Delay in send
        PThread::Sleep(timeout);
        break;

      case NextCharWait : // Wait for reply
        PINDEX receivePosition = sendPosition;
        if (GetNextChar(command, receivePosition) < 0) {
          SetReadTimeout(timeout);
          while (ReadChar() >= 0)
            if (abortCommandString) // aborted
              return PFalse;
        }
        else {
          receivePosition = sendPosition;
          do {
            if (abortCommandString) // aborted
              return PFalse;
            if ((nextChar = ReadCharWithTimeout(timeout)) < 0)
              return PFalse;
          } while (!ReceiveCommandString(nextChar,
                                     command, receivePosition, sendPosition));
//          nextChar = GetNextChar(command, receivePosition);
          sendPosition = receivePosition;
        }
    }
  }

  return PFalse;
}


PBoolean PChannel::Shutdown(ShutdownValue)
{
  return PFalse;
}


bool PChannel::SetLocalEcho(bool /*localEcho*/)
{
  return IsOpen();
}

bool PChannel::FlowControl(const void * /*flowData*/)
{
    return false;
}

PChannel * PChannel::GetBaseReadChannel() const
{
  return (PChannel *)this;
}


PChannel * PChannel::GetBaseWriteChannel() const
{
  return (PChannel *)this;
}


PString PChannel::GetErrorText(ErrorGroup group) const
{
  return GetErrorText(lastErrorCode[group], lastErrorNumber[group]);
}


PBoolean PChannel::ConvertOSError(int status, ErrorGroup group)
{
  Errors lastError;
  int osError;
  PBoolean ok = ConvertOSError(status, lastError, osError);
  SetErrorValues(lastError, osError, group);
  return ok;
}


PBoolean PChannel::SetErrorValues(Errors errorCode, int errorNum, ErrorGroup group)
{
  lastErrorCode[NumErrorGroups] = lastErrorCode[group] = errorCode;
  lastErrorNumber[NumErrorGroups] = lastErrorNumber[group] = errorNum;
  return errorCode == NoError;
}

#ifndef P_HAS_RECVMSG

PBoolean PChannel::Read(const VectorOfSlice & slices)
{
  PINDEX length = 0;

  VectorOfSlice::const_iterator r;
  for (r = slices.begin(); r != slices.end(); ++r) {
    PBoolean stat = Read(r->iov_base, r->iov_len);
    length        += lastReadCount;
    lastReadCount = length;
    if (!stat)
      return PFalse;
  }

  return PTrue;
}

PBoolean PChannel::Write(const VectorOfSlice & slices)
{
  PINDEX length = 0;

  VectorOfSlice::const_iterator r;
  for (r = slices.begin(); r != slices.end(); ++r) {
    PBoolean stat = Write(r->iov_base, r->iov_len);
    length        += lastWriteCount;
    lastWriteCount = length;
    if (!stat)
      return PFalse;
  }

  return PTrue;
}

#endif // P_HAS_RECVMSG

///////////////////////////////////////////////////////////////////////////////
// PIndirectChannel

PIndirectChannel::PIndirectChannel()
{
  readChannel = writeChannel = NULL;
  writeAutoDelete = readAutoDelete = PFalse;
}


PObject::Comparison PIndirectChannel::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PIndirectChannel), PInvalidCast);
  const PIndirectChannel & other = (const PIndirectChannel &)obj;
  return readChannel == other.readChannel &&
         writeChannel == other.writeChannel ? EqualTo : GreaterThan;
}


PString PIndirectChannel::GetName() const
{
  PReadWaitAndSignal mutex(channelPointerMutex);

  if (readChannel != NULL && readChannel == writeChannel)
    return readChannel->GetName();

  PStringStream name;

  name << "R<";
  if (readChannel != NULL)
    name << readChannel->GetName();
  name << "> T<";
  if (writeChannel != NULL)
    name << writeChannel->GetName();
  name << '>';

  return name;
}


PBoolean PIndirectChannel::Close()
{
  PBoolean retval = PTrue;

  flush();

  channelPointerMutex.StartRead();

  if (readChannel != NULL)
    retval = readChannel->Close();

  if (readChannel != writeChannel && writeChannel != NULL)
    retval = writeChannel->Close() && retval;

  channelPointerMutex.EndRead();

  channelPointerMutex.StartWrite();

  PChannel * r = readChannel;
  PChannel * w = writeChannel;

  readChannel = NULL;
  writeChannel = NULL;

  if (readAutoDelete)
    delete r;

  if (r != w && writeAutoDelete)
    delete w;

  channelPointerMutex.EndWrite();

  return retval;
}


PBoolean PIndirectChannel::IsOpen() const
{
  PReadWaitAndSignal mutex(channelPointerMutex);

  if (readChannel != NULL && readChannel == writeChannel)
    return readChannel->IsOpen();

  PBoolean returnValue = readChannel != NULL ? readChannel->IsOpen() : PFalse;

  if (writeChannel != NULL)
    returnValue = writeChannel->IsOpen() || returnValue;

  return returnValue;
}


PBoolean PIndirectChannel::Read(void * buf, PINDEX len)
{
  flush();

  PReadWaitAndSignal mutex(channelPointerMutex);

  if (readChannel == NULL) {
    SetErrorValues(NotOpen, EBADF, LastReadError);
    return PFalse;
  }

  readChannel->SetReadTimeout(readTimeout);
  PBoolean returnValue = readChannel->Read(buf, len);

  SetErrorValues(readChannel->GetErrorCode(LastReadError),
                 readChannel->GetErrorNumber(LastReadError),
                 LastReadError);
  lastReadCount = readChannel->GetLastReadCount();

  return returnValue;
}


PBoolean PIndirectChannel::Write(const void * buf, PINDEX len)
{
  flush();

  PReadWaitAndSignal mutex(channelPointerMutex);

  if (writeChannel == NULL) {
    SetErrorValues(NotOpen, EBADF, LastWriteError);
    return PFalse;
  }

  writeChannel->SetWriteTimeout(writeTimeout);
  PBoolean returnValue = writeChannel->Write(buf, len);

  SetErrorValues(writeChannel->GetErrorCode(LastWriteError),
                 writeChannel->GetErrorNumber(LastWriteError),
                 LastWriteError);

  lastWriteCount = writeChannel->GetLastWriteCount();

  return returnValue;
}


PBoolean PIndirectChannel::Shutdown(ShutdownValue value)
{
  PReadWaitAndSignal mutex(channelPointerMutex);

  if (readChannel != NULL && readChannel == writeChannel)
    return readChannel->Shutdown(value);

  PBoolean returnValue = readChannel != NULL ? readChannel->Shutdown(value) : PFalse;

  if (writeChannel != NULL)
    returnValue = writeChannel->Shutdown(value) || returnValue;

  return returnValue;
}


bool PIndirectChannel::SetLocalEcho(bool localEcho)
{
  PReadWaitAndSignal mutex(channelPointerMutex);
  return readChannel != NULL && readChannel->SetLocalEcho(localEcho);
}


PString PIndirectChannel::GetErrorText(ErrorGroup group) const
{
  if (readChannel != NULL)
    return readChannel->GetErrorText(group);

  if (writeChannel != NULL)
    return writeChannel->GetErrorText(group);

  return PChannel::GetErrorText(group);
}


PBoolean PIndirectChannel::Open(PChannel & channel)
{
  return Open(&channel, (PBoolean)PFalse);
}


PBoolean PIndirectChannel::Open(PChannel * channel, PBoolean autoDelete)
{
  return Open(channel, channel, autoDelete, autoDelete);
}


PBoolean PIndirectChannel::Open(PChannel * readChan,
                            PChannel * writeChan,
                            PBoolean autoDeleteRead,
                            PBoolean autoDeleteWrite)
{
  flush();

  channelPointerMutex.StartWrite();

  if (readChannel != NULL)
    readChannel->Close();

  if (readChannel != writeChannel && writeChannel != NULL)
    writeChannel->Close();

  if (readAutoDelete)
    delete readChannel;

  if (readChannel != writeChannel && writeAutoDelete)
    delete writeChannel;

  readChannel = readChan;
  readAutoDelete = autoDeleteRead;

  writeChannel = writeChan;
  writeAutoDelete = autoDeleteWrite;

  channelPointerMutex.EndWrite();

  return IsOpen() && OnOpen();
}


PBoolean PIndirectChannel::OnOpen()
{
  return PTrue;
}


bool PIndirectChannel::SetReadChannel(PChannel * channel, bool autoDelete, bool closeExisting)
{
  PWriteWaitAndSignal mutex(channelPointerMutex);

  if (closeExisting) {
    if (readAutoDelete)
      delete readChannel;
  }
  else {
    if (readChannel != NULL)
      return SetErrorValues(DeviceInUse, EEXIST);
  }

  readChannel = channel;
  readAutoDelete = autoDelete;

  return channel != NULL && channel->IsOpen();
}


bool PIndirectChannel::SetWriteChannel(PChannel * channel, bool autoDelete, bool closeExisting)
{
  PWriteWaitAndSignal mutex(channelPointerMutex);

  if (closeExisting) {
    if (writeAutoDelete)
      delete writeChannel;
  }
  else {
    if (writeChannel != NULL)
      return SetErrorValues(DeviceInUse, EEXIST);
  }

  writeChannel = channel;
  writeAutoDelete = autoDelete;

  return channel != NULL && channel->IsOpen();
}


PChannel * PIndirectChannel::GetBaseReadChannel() const
{
  PReadWaitAndSignal mutex(channelPointerMutex);
  return readChannel != NULL ? readChannel->GetBaseReadChannel() : 0;
}


PChannel * PIndirectChannel::GetBaseWriteChannel() const
{
  PReadWaitAndSignal mutex(channelPointerMutex);
  return writeChannel != NULL ? writeChannel->GetBaseWriteChannel() : 0;
}


///////////////////////////////////////////////////////////////////////////////
// PFile

PFile::~PFile()
{
  Close();
}


PObject::Comparison PFile::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PFile), PInvalidCast);
  return path.Compare(((const PFile &)obj).path);
}


PBoolean PFile::Rename(const PString & newname, PBoolean force)
{
  Close();

  if (!ConvertOSError(Rename(path, newname, force) ? 0 : -1))
    return PFalse;

  path = path.GetDirectory() + newname;
  return PTrue;
}


PBoolean PFile::Close()
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);

  flush();

#ifdef WOT_NO_FILESYSTEM
  PBoolean ok = PTrue;
#else
  PBoolean ok = ConvertOSError(_close(os_handle));
#endif

  os_handle = -1;

  if (removeOnClose)
    Remove();

  return ok;
}


PBoolean PFile::Read(void * buffer, PINDEX amount)
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);

  flush();
#ifdef WOT_NO_FILESYSTEM
  lastReadCount = 0;
#else
  lastReadCount = _read(GetHandle(), buffer, amount);
#endif
  return ConvertOSError(lastReadCount, LastReadError) && lastReadCount > 0;
}


PBoolean PFile::Write(const void * buffer, PINDEX amount)
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);

  flush();
#ifdef WOT_NO_FILESYSTEM
  lastWriteCount = amount;
#else
  lastWriteCount = _write(GetHandle(), buffer, amount);
#endif
  return ConvertOSError(lastWriteCount, LastWriteError) && lastWriteCount >= amount;
}


PBoolean PFile::Open(const PFilePath & name, OpenMode  mode, int opts)
{
  Close();
  SetFilePath(name);
  return Open(mode, opts);
}


off_t PFile::GetLength() const
{
  if (!IsOpen())
    return -1;

#ifdef WOT_NO_FILESYSTEM
  return 0;
#else
  off_t pos = _lseek(GetHandle(), 0, SEEK_CUR);
  off_t len = _lseek(GetHandle(), 0, SEEK_END);
  PAssertOS(_lseek(GetHandle(), pos, SEEK_SET) != (off_t)-1);
  return len;
#endif
}


PBoolean PFile::IsEndOfFile() const
{
  if (!IsOpen())
    return true;

  ((PFile *)this)->flush();
  return GetPosition() >= GetLength();
}


PBoolean PFile::SetPosition(off_t pos, FilePositionOrigin origin)
{
#ifdef WOT_NO_FILESYSTEM
  return PTrue;
#else
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);

  return _lseek(GetHandle(), pos, origin) != (off_t)-1;
#endif
}


PBoolean PFile::Copy(const PFilePath & oldname, const PFilePath & newname, PBoolean force)
{
  PFile oldfile(oldname, ReadOnly);
  if (!oldfile.IsOpen())
    return PFalse;

  PFile newfile(newname,
                   WriteOnly, Create|Truncate|(force ? MustExist : Exclusive));
  if (!newfile.IsOpen())
    return PFalse;

  PCharArray buffer(10000);

  off_t amount = oldfile.GetLength();
  while (amount > 10000) {
    if (!oldfile.Read(buffer.GetPointer(), 10000))
      return PFalse;
    if (!newfile.Write((const char *)buffer, 10000))
      return PFalse;
    amount -= 10000;
  }

  if (!oldfile.Read(buffer.GetPointer(), (int)amount))
    return PFalse;
  if (!newfile.Write((const char *)buffer, (int)amount))
    return PFalse;

  return newfile.Close();
}


// End Of File ///////////////////////////////////////////////////////////////
