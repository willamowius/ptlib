/*
 * sound.cxx
 *
 * Sound driver implementation.
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
 * Contributor(s): Loopback feature: Philip Edelbrock <phil@netroedge.com>.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#pragma implementation "sound.h"

#include <ptlib.h>

#ifdef P_LINUX
#include <sys/soundcard.h>
#include <sys/time.h>
#endif

#ifdef P_FREEBSD
#include <machine/soundcard.h>
#endif

#if defined(P_OPENBSD) || defined(P_NETBSD)
#include <soundcard.h>
#endif


///////////////////////////////////////////////////////////////////////////////
// declare type for sound handle dictionary

class SoundHandleEntry : public PObject {

  PCLASSINFO(SoundHandleEntry, PObject)

  public:
    SoundHandleEntry();

    int handle;
    int direction;

    unsigned numChannels;
    unsigned sampleRate;
    unsigned bitsPerSample;
    unsigned fragmentValue;
    PBoolean isInitialised;
};

PDICTIONARY(SoundHandleDict, PString, SoundHandleEntry);

#define LOOPBACK_BUFFER_SIZE 5000
#define BYTESINBUF ((startptr<endptr)?(endptr-startptr):(LOOPBACK_BUFFER_SIZE+endptr-startptr))

static char buffer[LOOPBACK_BUFFER_SIZE];
static int  startptr, endptr;

PMutex PSoundChannel::dictMutex;

static SoundHandleDict & handleDict()
{
  static SoundHandleDict dict;
  return dict;
}

PSound::PSound(unsigned channels,
               unsigned samplesPerSecond,
               unsigned bitsPerSample,
               PINDEX   bufferSize,
               const BYTE * buffer)
{
  encoding = 0;
  numChannels = channels;
  sampleRate = samplesPerSecond;
  sampleSize = bitsPerSample;
  SetSize(bufferSize);
  if (buffer != NULL)
    memcpy(GetPointer(), buffer, bufferSize);
}


PSound::PSound(const PFilePath & filename)
{
  encoding = 0;
  numChannels = 1;
  sampleRate = 8000;
  sampleSize = 16;
  Load(filename);
}


PSound & PSound::operator=(const PBYTEArray & data)
{
  PBYTEArray::operator=(data);
  return *this;
}


void PSound::SetFormat(unsigned channels,
                       unsigned samplesPerSecond,
                       unsigned bitsPerSample)
{
  encoding = 0;
  numChannels = channels;
  sampleRate = samplesPerSecond;
  sampleSize = bitsPerSample;
  formatInfo.SetSize(0);
}


PBoolean PSound::Load(const PFilePath & /*filename*/)
{
  return PFalse;
}


PBoolean PSound::Save(const PFilePath & /*filename*/)
{
  return PFalse;
}


PBoolean PSound::Play()
{
  PSoundChannel channel(PSoundChannel::GetDefaultDevice(PSoundChannel::Player),
                        PSoundChannel::Player);
  if (!channel.IsOpen())
    return PFalse;

  return channel.PlaySound(*this, PTrue);
}


PBoolean PSound::PlayFile(const PFilePath & file, PBoolean wait)
{
  PSoundChannel channel(PSoundChannel::GetDefaultDevice(PSoundChannel::Player),
                        PSoundChannel::Player);
  if (!channel.IsOpen())
    return PFalse;

  return channel.PlayFile(file, wait);
}


///////////////////////////////////////////////////////////////////////////////

SoundHandleEntry::SoundHandleEntry()
{
  handle    = -1;
  direction = 0;
}

///////////////////////////////////////////////////////////////////////////////

PSoundChannel::PSoundChannel()
{
  Construct();
}


PSoundChannel::PSoundChannel(const PString & device,
                             Directions dir,
                             unsigned numChannels,
                             unsigned sampleRate,
                             unsigned bitsPerSample)
{
  Construct();
  Open(device, dir, numChannels, sampleRate, bitsPerSample);
}


void PSoundChannel::Construct()
{
  os_handle = -1;
}


PSoundChannel::~PSoundChannel()
{
  Close();
}


PStringArray PSoundChannel::GetDeviceNames(Directions /*dir*/)
{
  static const char * const devices[] = {
    "/dev/audio",
    "/dev/dsp",
    "/dev/dspW",
    "loopback"
  };

  return PStringArray(PARRAYSIZE(devices), devices);
}


PString PSoundChannel::GetDefaultDevice(Directions /*dir*/)
{
  return "/dev/dsp";
}


PBoolean PSoundChannel::Open(const PString & _device,
                              Directions _dir,
                                unsigned _numChannels,
                                unsigned _sampleRate,
                                unsigned _bitsPerSample)
{
  Close();

  // lock the dictionary
  dictMutex.Wait();

  // make the direction value 1 or 2
  int dir = _dir + 1;

  // if this device in in the dictionary
  if (handleDict().Contains(_device)) {

    SoundHandleEntry & entry = handleDict()[_device];

    // see if the sound channel is already open in this direction
    if ((entry.direction & dir) != 0) {
      dictMutex.Signal();
      return PFalse;
    }

    // flag this entry as open in this direction
    entry.direction |= dir;
    os_handle = entry.handle;

  } else {

    // this is the first time this device has been used
    // open the device in read/write mode always
    if (_device == "loopback") {
      startptr = endptr = 0;
      os_handle = 0; // Use os_handle value 0 to indicate loopback, cannot ever be stdin!
    }
    else if (!ConvertOSError(os_handle = ::open((const char *)_device, O_RDWR))) {
      dictMutex.Signal();
      return PFalse;
    }

    // add the device to the dictionary
    SoundHandleEntry * entry = PNEW SoundHandleEntry;
    handleDict().SetAt(_device, entry); 

    // save the information into the dictionary entry
    entry->handle        = os_handle;
    entry->direction     = dir;
    entry->numChannels   = _numChannels;
    entry->sampleRate    = _sampleRate;
    entry->bitsPerSample = _bitsPerSample;
    entry->isInitialised = PFalse;
    entry->fragmentValue = 0x7fff0008;
  }
   
   
  // unlock the dictionary
  dictMutex.Signal();

  // save the direction and device
  direction     = _dir;
  device        = _device;
  isInitialised = PFalse;

  return PTrue;
}

PBoolean PSoundChannel::Setup()
{
  if (os_handle < 0)
    return PFalse;

  if (isInitialised)
    return PTrue;

  // lock the dictionary
  dictMutex.Wait();

  // the device must always be in the dictionary
  PAssertOS(handleDict().Contains(device));

  // get record for the device
  SoundHandleEntry & entry = handleDict()[device];

  PBoolean stat = PFalse;
  if (entry.isInitialised)  {
    isInitialised = PTrue;
    stat          = PTrue;
  } else if (device == "loopback")
    stat = PTrue;
  else {

  // must always set paramaters in the following order:
  //   buffer paramaters
  //   sample format (number of bits)
  //   number of channels (mon/stereo)
  //   speed (sampling rate)

    int arg, val;

    // reset the device first so it will accept the new parms

#ifndef P_AIX
    if (ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_RESET, &arg))) {

      arg = val = entry.fragmentValue;
      //if (ConvertOSError(ioctl(os_handle, SNDCTL_DSP_SETFRAGMENT, &arg)) || (arg != val)) {
      ::ioctl(os_handle, SNDCTL_DSP_SETFRAGMENT, &arg); {

        arg = val = (entry.bitsPerSample == 16) ? AFMT_S16_LE : AFMT_S8;
        if (ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_SETFMT, &arg)) || (arg != val)) {

          arg = val = (entry.numChannels == 2) ? 1 : 0;
          if (ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_STEREO, &arg)) || (arg != val)) {

            arg = val = entry.sampleRate;
            if (ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_SPEED, &arg)) || (arg != val)) 
              stat = PTrue;
          }
        }
      }
    }
#endif

  }

  entry.isInitialised = PTrue;
  isInitialised       = PTrue;

  dictMutex.Signal();

  return stat;
}

PBoolean PSoundChannel::Close()
{
  // if the channel isn't open, do nothing
  if (os_handle < 0)
    return PTrue;

  if (os_handle == 0) {
    os_handle = -1;
    return PTrue;
  }

  // the device must be in the dictionary
  dictMutex.Wait();
  SoundHandleEntry * entry;
  PAssert((entry = handleDict().GetAt(device)) != NULL, "Unknown sound device \"" + device + "\" found");

  // modify the directions bit mask in the dictionary
  entry->direction ^= (direction+1);

  // if this is the last usage of this entry, then remove it
  if (entry->direction == 0) {
    handleDict().RemoveAt(device);
    dictMutex.Signal();
    return PChannel::Close();
  }

  // flag this channel as closed
  dictMutex.Signal();
  os_handle = -1;
  return PTrue;
}

PBoolean PSoundChannel::Write(const void * buf, PINDEX len)
{
  if (!Setup())
    return PFalse;

  if (os_handle > 0) {
    while (!ConvertOSError(::write(os_handle, (void *)buf, len)))
      if (GetErrorCode() != Interrupted)
        return PFalse;
    return PTrue;
  }

  int index = 0;

  while (len > 0) {
    len--;
    buffer[endptr++] = ((char *)buf)[index++];
    if (endptr == LOOPBACK_BUFFER_SIZE)
      endptr = 0;
    while (((startptr - 1) == endptr) || ((endptr==LOOPBACK_BUFFER_SIZE - 1) && (startptr==0))) {
      usleep(5000);
    }
  }
  return PTrue;
}

PBoolean PSoundChannel::Read(void * buf, PINDEX len)
{
  if (!Setup())
    return PFalse;

  if (os_handle > 0) {
    while (!ConvertOSError(::read(os_handle, (void *)buf, len)))
      if (GetErrorCode() != Interrupted)
        return PFalse;
    return PTrue;
  }

  int index = 0;

  while (len > 0) {
    while (startptr == endptr)
      usleep(5000);
    len--;
    ((char *)buf)[index++]=buffer[startptr++];
    if (startptr == LOOPBACK_BUFFER_SIZE)
      startptr = 0;
  } 
  return PTrue;
}


PBoolean PSoundChannel::SetFormat(unsigned numChannels,
                              unsigned sampleRate,
                              unsigned bitsPerSample)
{
  if (os_handle < 0) {
    lastError = NotOpen;
    return PFalse;
  }

  // check parameters
  PAssert((bitsPerSample == 8) || (bitsPerSample == 16), PInvalidParameter);
  PAssert(numChannels >= 1 && numChannels <= 2, PInvalidParameter);

  Abort();

  // lock the dictionary
  dictMutex.Wait();

  // the device must always be in the dictionary
  PAssertOS(handleDict().Contains(device));

  // get record for the device
  SoundHandleEntry & entry = handleDict()[device];

  entry.numChannels   = numChannels;
  entry.sampleRate    = sampleRate;
  entry.bitsPerSample = bitsPerSample;
  entry.isInitialised  = PFalse;

  // unlock dictionary
  dictMutex.Signal();

  // mark this channel as uninitialised
  isInitialised = PFalse;

  return PTrue;
}


PBoolean PSoundChannel::SetBuffers(PINDEX size, PINDEX count)
{
  if (os_handle < 0) {
    lastError = NotOpen;
    return PFalse;
  }

  Abort();

  PAssert(size > 0 && count > 0 && count < 65536, PInvalidParameter);
  int arg = 1;
  while (size > (PINDEX)(1 << arg))
    arg++;

  arg |= count << 16;

  // lock the dictionary
  dictMutex.Wait();

  // the device must always be in the dictionary
  PAssertOS(handleDict().Contains(device));

  // get record for the device
  SoundHandleEntry & entry = handleDict()[device];

  // set information in the common record
  entry.fragmentValue = arg;
  entry.isInitialised = PFalse;

  // flag this channel as not initialised
  isInitialised       = PFalse;

  dictMutex.Signal();

  return PTrue;
}


PBoolean PSoundChannel::GetBuffers(PINDEX & size, PINDEX & count)
{
  if (os_handle < 0) {
    lastError = NotOpen;
    return PFalse;
  }

  // lock the dictionary
  dictMutex.Wait();

  // the device must always be in the dictionary
  PAssertOS(handleDict().Contains(device));

  SoundHandleEntry & entry = handleDict()[device];

  int arg = entry.fragmentValue;

  dictMutex.Signal();

  count = arg >> 16;
  size = 1 << (arg&0xffff);
  return PTrue;
}


PBoolean PSoundChannel::PlaySound(const PSound & sound, PBoolean wait)
{
  if (os_handle < 0) {
    lastError = NotOpen;
    return PFalse;
  }

  Abort();

  if (!Write((const BYTE *)sound, sound.GetSize()))
    return PFalse;

  if (wait)
    return WaitForPlayCompletion();

  return PTrue;
}


PBoolean PSoundChannel::PlayFile(const PFilePath & filename, PBoolean wait)
{
  if (os_handle < 0) {
    lastError = NotOpen;
    return PFalse;
  }

  PFile file(filename, PFile::ReadOnly);
  if (!file.IsOpen())
    return PFalse;

  for (;;) {
    BYTE buffer[256];
    if (!file.Read(buffer, 256))
      break;
    PINDEX len = file.GetLastReadCount();
    if (len == 0)
      break;
    if (!Write(buffer, len))
      break;
  }

  file.Close();

  if (wait)
    return WaitForPlayCompletion();

  return PTrue;
}


PBoolean PSoundChannel::HasPlayCompleted()
{
  if (os_handle < 0) {
    lastError = NotOpen;
    return PFalse;
  }

  if (os_handle == 0)
    return BYTESINBUF <= 0;

#ifndef P_AIX
  audio_buf_info info;
  if (!ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_GETOSPACE, &info)))
    return PFalse;

  return info.fragments == info.fragstotal;
#else
  return 0;
#endif

}


PBoolean PSoundChannel::WaitForPlayCompletion()
{
  if (os_handle < 0) {
    lastError = NotOpen;
    return PFalse;
  }

  if (os_handle == 0) {
    while (BYTESINBUF > 0)
      usleep(1000);
    return PTrue;
  }
#ifndef P_AIX
  return ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_SYNC, NULL));
#else
  return 0;
#endif
}


PBoolean PSoundChannel::RecordSound(PSound & sound)
{
  if (os_handle < 0) {
    lastError = NotOpen;
    return PFalse;
  }

  return PFalse;
}


PBoolean PSoundChannel::RecordFile(const PFilePath & filename)
{
  if (os_handle < 0) {
    lastError = NotOpen;
    return PFalse;
  }

  return PFalse;
}


PBoolean PSoundChannel::StartRecording()
{
  if (os_handle < 0) {
    lastError = NotOpen;
    return PFalse;
  }

  if (os_handle == 0)
    return PTrue;

  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(os_handle, &fds);

  struct timeval timeout;
  memset(&timeout, 0, sizeof(timeout));

  return ConvertOSError(::select(1, &fds, NULL, NULL, &timeout));
}


PBoolean PSoundChannel::IsRecordBufferFull()
{
  if (os_handle < 0) {
    lastError = NotOpen;
    return PFalse;
  }

  if (os_handle == 0)
    return (BYTESINBUF > 0);

#ifndef P_AIX
  audio_buf_info info;
  if (!ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_GETISPACE, &info)))
    return PFalse;

  return info.fragments > 0;
#else
  return 0;
#endif
}


PBoolean PSoundChannel::AreAllRecordBuffersFull()
{
  if (os_handle < 0) {
    lastError = NotOpen;
    return PFalse;
  }

  if (os_handle == 0)
    return (BYTESINBUF == LOOPBACK_BUFFER_SIZE);

#ifndef P_AIX
  audio_buf_info info;
  if (!ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_GETISPACE, &info)))
    return PFalse;

  return info.fragments == info.fragstotal;
#else
  return 0;
#endif
}


PBoolean PSoundChannel::WaitForRecordBufferFull()
{
  if (os_handle < 0) {
    lastError = NotOpen;
    return PFalse;
  }

  return PXSetIOBlock(PXReadBlock, readTimeout);
}


PBoolean PSoundChannel::WaitForAllRecordBuffersFull()
{
  return PFalse;
}


PBoolean PSoundChannel::Abort()
{
  if (os_handle == 0) {
    startptr = endptr = 0;
    return PTrue;
  }

#ifndef P_AIX
  return ConvertOSError(ioctl(os_handle, SNDCTL_DSP_RESET, NULL));
#else
  return 0;
#endif
}

PBoolean PSoundChannel::SetVolume(unsigned newVolume)
{
  cerr << __FILE__ << "PSoundChannel :: SetVolume called in error. Please fix"<<endl;
  return PFalse;
}

PBoolean  PSoundChannel::GetVolume(unsigned & volume)
{
 cerr << __FILE__ << "PSoundChannel :: GetVolume called in error. Please fix"<<endl;
  return PFalse;
}



// End of file
