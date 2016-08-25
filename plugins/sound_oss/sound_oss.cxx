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

#pragma implementation "sound_oss.h"

#include "sound_oss.h"

PCREATE_SOUND_PLUGIN(OSS, PSoundChannelOSS);

///////////////////////////////////////////////////////////////////////////////
// declare type for sound handle dictionary

PDICTIONARY(SoundHandleDict, PString, SoundHandleEntry);

static PMutex dictMutex;

static SoundHandleDict & handleDict()
{
  static SoundHandleDict dict;
  return dict;
}

///////////////////////////////////////////////////////////////////////////////

SoundHandleEntry::SoundHandleEntry()
{
  handle    = -1;
  direction = 0;
}

///////////////////////////////////////////////////////////////////////////////

PSoundChannelOSS::PSoundChannelOSS()
{
  PSoundChannelOSS::Construct();
}


PSoundChannelOSS::PSoundChannelOSS(const PString & device,
                                        Directions dir,
                                          unsigned numChannels,
                                          unsigned sampleRate,
                                          unsigned bitsPerSample)
{
  Construct();
  Open(device, dir, numChannels, sampleRate, bitsPerSample);
}


void PSoundChannelOSS::Construct()
{
  os_handle = -1;
}


PSoundChannelOSS::~PSoundChannelOSS()
{
  Close();
}

static PBoolean IsNumericString(PString numbers) {
  // return true if 'numbers' contains only digits (0 to 9)
  // or if it contains digits followed by a '.'

  PBoolean isNumber = PFalse;
  for (PINDEX p = 0; p < numbers.GetLength(); p++) {
    if (isdigit(numbers[p])) {
      isNumber = PTrue;
    } else {
      return isNumber;
    }
  }
  return isNumber;
}

static void CollectSoundDevices(PDirectory devdir, POrdinalToString & dsp, POrdinalToString & mixer, PBoolean collect_with_names)
{
  if (!devdir.Open())
    return;

  do {
    PString filename = devdir.GetEntryName();
    PString devname = devdir + filename;
    if (devdir.IsSubDir())
      CollectSoundDevices(devname, dsp, mixer, collect_with_names);
    else {
      if (!collect_with_names) {
        // On Linux, look at the character device numbers
        PFileInfo info;
        if (devdir.GetInfo(info) &&info.type == PFileInfo::CharDevice) {
          struct stat s;
          if (lstat(devname, &s) == 0) {
            // OSS compatible audio major device numbers (OSS, SAM9407, etc)
            static const unsigned deviceNumbers[] = { 14, 145, 246 };
            for (PINDEX i = 0; i < PARRAYSIZE(deviceNumbers); i++) {
              if ((s.st_rdev >> 8) == deviceNumbers[i]) {
                PINDEX cardnum = (s.st_rdev >> 4) & 15;
                if ((s.st_rdev & 15) == 3)  // Digital audio minor device number
                  dsp.SetAt(cardnum, devname);
                else if ((s.st_rdev & 15) == 0) // Digital audio minor device number
                  mixer.SetAt(cardnum, devname);
              }
            }
          }
        }
      }
      else {
        // On Linux devfs systems, the major numbers can change dynamically.
        // On FreeBSD and other OSs, the major numbes are different to Linux.
        // So collect devices by looking for dsp(N) and mixer(N).
        // (or /dev/audio(N) and mixer(N) on NetBSD)
        // Notes. FreeBSD supports audio stream mixing. A single sound card
        // may have multiple /dev entries in the form /dev/dspN.M
        // eg /dev/dsp0.0 /dev/dsp0.1 /dev/dsp0.2 and /dev/dsp0.3
        // When adding these to the 'dsp' string array, only the first one
        // found is used.

#if defined (P_NETBSD) || defined (P_OPENBSD)
        // Look for audio on NetBSD
        if (filename == "audio") {
          dsp.SetAt(0, devname);
        }
        // Look for audioN. Insert at position cardnum + 1
        if ((filename.GetLength() > 5) && (filename.Left(5) == "audio")) {
          PString numbers = filename.Mid(5); // get everything after 'audio'
          if (IsNumericString(numbers)) {
            PINDEX cardnum = numbers.AsInteger();
            dsp.SetAt(cardnum+1, devname);
          }
        }
#else /* defined (P_NETBSD) || defined (P_OPENBSD) */
        // Look for dsp
        if (filename == "dsp") {
          dsp.SetAt(0, devname);
        }

        // Look for dspN entries. Insert at position N + 1
        // and look for dspN.M entries. Insert at position N + 1 (ignoring M)

        if ((filename.GetLength() > 3) && (filename.Left(3) == "dsp")) {
          PString numbers = filename.Mid(3); // get everything after 'dsp'
          if (IsNumericString(numbers)) {
            PINDEX cardnum = numbers.AsInteger(); //dspN.M is truncated to dspN.
            // If we have not yet inserted something for this cardnum, insert it
            if (dsp.GetAt(cardnum+1) == NULL) {
#if defined (P_FREEBSD)
              // in FreeBSD we might have different sound channels
              // created by sound(4) in devfs(5), like /dev/dsp0, /dev/dsp1, ...
              // see also 'cat /dev/sndstat',
              // and the end user should make its choice between the sound
              // devices, for example having a built-in micro in a webcam
              // and a headset micro
              devname = devdir + "dsp" + numbers;
              PTRACE(1, "OSS\tCollectSoundDevices FreeBSD devname set to devfs(5) name:" << devname );
#endif /* defined (P_FREEBSD) */
              dsp.SetAt(cardnum+1, devname);
            }
          }
        }
#endif /* (P_NETBSD) || defined (P_OPENBSD) */

        // Look for mixer
        if (filename == "mixer") {
          mixer.SetAt(0, devname);
        }
        // Look for mixerN. Insert at position cardnum + 1
        if ((filename.GetLength() > 5) && (filename.Left(5) == "mixer")) {
          PString numbers = filename.Mid(5); // get everything after 'mixer'
          if (IsNumericString(numbers)) {
            PINDEX cardnum = numbers.AsInteger();
            mixer.SetAt(cardnum+1, devname);
          }
        }
      }
    }
  } while (devdir.Next());
}


PStringArray PSoundChannelOSS::GetDeviceNames(Directions /*dir*/)
{
  // First locate sound cards. On Linux with devfs and on the other platforms
  // (eg FreeBSD), we search for filenames with dspN or mixerN.
  // On linux without devfs we scan all of the devices and look for ones
  // with major device numbers corresponding to OSS compatible drivers.

  POrdinalToString dsp, mixer;

#ifdef P_LINUX
  PDirectory devdir = "/dev/sound";
  if (devdir.Open()) {
    CollectSoundDevices("/dev/sound", dsp, mixer, PTrue); // use names (devfs)
  } else {
    CollectSoundDevices("/dev", dsp, mixer, PFalse); // use major numbers
  }
#else
  CollectSoundDevices("/dev", dsp, mixer, PTrue); // use names
#endif

  // Now we go through the collected devices and see if any have a phyisical reality
  PStringList devices;

  for (PINDEX i = 0; i < dsp.GetSize(); i++) {
    PINDEX cardnum = dsp.GetKeyAt(i);
    // Try and open mixer if have one as this is unlikely to fail for any
    // reason other than there not being a physical device
    if (mixer.Contains(cardnum)) {
      int fd = ::open(mixer[cardnum], O_RDONLY);
      if (fd >= 0) {
        // Do something with the mixer to be sure it is there
        int dummy;
        if (::ioctl(fd, SOUND_MIXER_READ_DEVMASK, &dummy) >= 0)
          devices.AppendString(dsp[cardnum]);
        ::close(fd);
      }
      else {

        // mixer failed but this could still be a valid dsp...
        // warning this is just a hack to make this work on strange mixer and dsp configurations
        // on my machine the first sound card registers 1 mixer and 2 dsp, so when my webcam
        // registers itself as dsp2 this test would fail...
        int fd = ::open(dsp[cardnum], O_RDONLY | O_NONBLOCK);
        if (fd >= 0 || errno == EBUSY) {
          devices.AppendString(dsp[cardnum]);
          ::close(fd);
        }
      }
    }
    else {
      // No mixer available, try and open it directly, this could fail if
      // the device happens to be open already
      int fd = ::open(dsp[cardnum], O_RDONLY | O_NONBLOCK);

      if (fd >= 0 || errno == EBUSY) {
        devices.AppendString(dsp[cardnum]);
        ::close(fd);
      }
    }
  }

  return devices;
}


PString PSoundChannelOSS::GetDefaultDevice(Directions dir)
{
  // Normally /dev/dsp points to the default sound device. If this is not
  // present, probe /dev for sound devices and return the first detected device.
    // return the first dsp device detected
  PStringArray devicenames;
  devicenames = PSoundChannelOSS::GetDeviceNames(dir);
  return devicenames[0];
}

PBoolean PSoundChannelOSS::Open(const PString & _device,
                              Directions _dir,
                                unsigned _numChannels,
                                unsigned _sampleRate,
                                unsigned _bitsPerSample)
{

  Close();


  // lock the dictionary
  PWaitAndSignal mutex(dictMutex);

  // make the direction value 1 or 2
  int dir = _dir + 1;

  // if this device is in the dictionary
  if (handleDict().Contains(_device)) {

    SoundHandleEntry & entry = handleDict()[_device];

    // see if the sound channel is already open in this direction
    if ((entry.direction & dir) != 0) {
      return PFalse;
    }

    // flag this entry as open in this direction
    entry.direction |= dir;
    os_handle = entry.handle;

  } else {

    // this is the first time this device has been used
    // open the device in read/write mode always
    // open the device in non-blocking mode to avoid hang if already open
    os_handle = ::open((const char *)_device, O_RDWR | O_NONBLOCK);

    if ((os_handle < 0) && (errno != EWOULDBLOCK)) 
      return ConvertOSError(os_handle);

    // switch to blocking mode
    DWORD cmd = 0;
    ::ioctl(os_handle, FIONBIO, &cmd);

    // add the device to the dictionary
    SoundHandleEntry * entry = PNEW SoundHandleEntry;
    handleDict().SetAt(_device, entry); 

    // save the information into the dictionary entry
    entry->handle        = os_handle;
    entry->direction     = dir;
    entry->numChannels   = mNumChannels     = _numChannels;
    entry->sampleRate    = actualSampleRate = mSampleRate    = _sampleRate;
    entry->bitsPerSample = mBitsPerSample   = _bitsPerSample;
    entry->isInitialised = PFalse;
    entry->fragmentValue = 0x7fff0008;
    entry->resampleRate  = 0;
  }
   
  // save the direction and device
  direction     = _dir;
  device        = _device;
  isInitialised = PFalse;

  return PTrue;
}

PBoolean PSoundChannelOSS::Setup()
{
  PWaitAndSignal mutex(dictMutex);

  if (os_handle < 0) {
    PTRACE(6, "OSS\tSkipping setup of " << device << " as not open");
    return PFalse;
  }

  if (isInitialised) {
    PTRACE(6, "OSS\tSkipping setup of " << device << " as instance already initialised");
    return PTrue;
  }

  // the device must always be in the dictionary
  PAssertOS(handleDict().Contains(device));

  // get record for the device
  SoundHandleEntry & entry = handleDict()[device];

  // set default return status
  PBoolean stat = PTrue;

  // do not re-initialise initialised devices
  if (entry.isInitialised) {
    PTRACE(6, "OSS\tSkipping setup for " << device << " as already initialised");
    resampleRate = entry.resampleRate;

  } else {
    PTRACE(6, "OSS\tInitialising " << device << "(" << (void *)(&entry) << ")");


#if defined(P_LINUX)
    // enable full duplex (maybe).
    ::ioctl(os_handle, SNDCTL_DSP_SETDUPLEX, 0);
#endif

    stat = PFalse;

    // must always set paramaters in the following order:
    //   buffer paramaters
    //   sample format (number of bits)
    //   number of channels (mon/stereo)
    //   speed (sampling rate)

    int arg, val;

    // reset the device first so it will accept the new parms
    if (ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_RESET, &arg))) {

      // set the write fragment size (applies to sound output only)
      arg = val = entry.fragmentValue;
      ::ioctl(os_handle, SNDCTL_DSP_SETFRAGMENT, &arg); 

      mBitsPerSample = entry.bitsPerSample;
#if PBYTE_ORDER == PLITTLE_ENDIAN
      arg = val = (entry.bitsPerSample == 16) ? AFMT_S16_LE : AFMT_S8;
#else
      arg = val = (entry.bitsPerSample == 16) ? AFMT_S16_BE : AFMT_S8;
#endif
      if (ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_SETFMT, &arg)) || (arg != val)) {

        mNumChannels = entry.numChannels;
        arg = val = (entry.numChannels == 2) ? 1 : 0;
        if (ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_STEREO, &arg)) || (arg != val)) {

          resampleRate = entry.resampleRate;          
          mSampleRate = entry.sampleRate;
          arg = val = entry.sampleRate;
          if (ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_SPEED, &arg))) {
            stat = PTrue;

            // detect cases where the hardware can't do the actual rate we need, but can do a simple multiple
            if (arg != (int)entry.sampleRate) {
              if (((arg / entry.sampleRate) * entry.sampleRate) == (unsigned)arg) {
                PTRACE(3, "Resampling data at " << entry.sampleRate << " to match hardware rate of " << arg);
                resampleRate = entry.resampleRate = arg / entry.sampleRate;
              } else {
                PTRACE_IF(4, actualSampleRate != (unsigned)val, "Actual sample rate selected is " << actualSampleRate << ", not " << entry.sampleRate);
                actualSampleRate = arg;
              }
            }
          }
        }
      }

#if PTRACING
      audio_buf_info info;
      ::ioctl(os_handle, SNDCTL_DSP_GETOSPACE, &info);
      PTRACE(4, "OSS\tOutput: fragments = " << info.fragments
                     << ", total frags = " << info.fragstotal
                     << ", frag size   = " << info.fragsize
                     << ", bytes       = " << info.bytes);

      ::ioctl(os_handle, SNDCTL_DSP_GETISPACE, &info);
      PTRACE(4, "OSS\tInput: fragments = " << info.fragments
                     << ", total frags = " << info.fragstotal
                     << ", frag size   = " << info.fragsize
                     << ", bytes       = " << info.bytes);
#endif
    }
  }

  // ensure device is marked as initialised
  isInitialised       = PTrue;
  entry.isInitialised = PTrue;

  return stat;
}

PBoolean PSoundChannelOSS::Close()
{
  // if the channel isn't open, do nothing
  if (os_handle < 0)
    return PTrue;

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

PBoolean PSoundChannelOSS::IsOpen() const
{
  return os_handle >= 0;
}

PBoolean PSoundChannelOSS::Write(const void * buf, PINDEX len)
{
  lastWriteCount = 0;

  if (!Setup() || os_handle < 0)
    return PFalse;

  if (resampleRate == 0) {
    while (!ConvertOSError(::write(os_handle, (void *)buf, len))) 
      if (GetErrorCode() != Interrupted)
        return PFalse;
    lastWriteCount += len;
  }

  else {
    // cut the data into 1K blocks and upsample it
    lastWriteCount = 0;
    BYTE resampleBuffer[1024];
    const BYTE * src    = (const BYTE *)buf;
    const BYTE * srcEnd = src + len;
    while (src < srcEnd) {

      // expand the data by the appropriate sample ratio
      BYTE * dst = resampleBuffer;
      const BYTE * srcStart = src;
      unsigned j;
       
      while ((src < srcEnd) && (dst < (resampleBuffer + sizeof(resampleBuffer) - resampleRate*2))) {
        for (j = 0; j < resampleRate; ++j) {
          memcpy(dst, src, 2);
          dst += 2 ;
        }
        src += 2;
      }
      lastWriteCount += src - srcStart;
      while (!ConvertOSError(::write(os_handle, resampleBuffer, dst - resampleBuffer))) {
        if (GetErrorCode() != Interrupted) 
          return PFalse;
      }
    }

  }

  return PTrue;
}

PBoolean PSoundChannelOSS::Read(void * buf, PINDEX len)
{
  lastReadCount = 0;

  if (!Setup() || os_handle < 0)
    return PFalse;

  if (resampleRate == 0) {

    PINDEX total = 0;
    while (total < len) {
      PINDEX bytes = 0;
      while (!ConvertOSError(bytes = ::read(os_handle, (void *)(((unsigned char *)buf) + total), len-total))) {
        if (GetErrorCode() != Interrupted) {
          PTRACE(6, "OSS\tRead failed");
          return PFalse;
        }
        PTRACE(6, "OSS\tRead interrupted");
      }
      total += bytes;
      if (total != len) {
        PTRACE(6, "OSS\tRead completed short - " << total << " vs " << len << ". Reading more data");
      }
    }
    lastReadCount = total;
  }

  else {

    // downsample the data

    BYTE * dst    = (BYTE *)buf;
    BYTE * dstEnd = dst + len;
    lastReadCount = 0;

    PBYTEArray resampleBuffer((1024 / resampleRate) * resampleRate);

    // downsample the data into 1K blocks 
    while (dst < dstEnd) {


      // calculate number of source bytes needed to fill the buffer
      PINDEX srcBytes = resampleRate * (dstEnd - dst);
      PINDEX bytes;

      {
        PINDEX bufLen = PMIN(resampleBuffer.GetSize(), srcBytes);
        while (!ConvertOSError(bytes = ::read(os_handle, resampleBuffer.GetPointer(), bufLen))) {
          if (GetErrorCode() != Interrupted) 
            return PFalse;
        }
      }

      // use an average, not just a single sample
      const BYTE * src = resampleBuffer;
      while ( ((src - resampleBuffer) < bytes) && (dst < dstEnd)) {
        int sample = 0;
        unsigned j;
        for (j = 0; j < resampleRate; ++j) {
          sample += *(PUInt16l *)src;
          src += 2;
        }
        *(PUInt16l *)dst = sample / resampleRate;
        dst +=2 ;
        lastReadCount += 2;
      }
    }
  }

  if (lastReadCount != len)
    PTRACE(6, "OSS\tRead completed short - " << lastReadCount << " vs " << len);
  else
    PTRACE(6, "OSS\tRead completed");

  return PTrue;
}


PBoolean PSoundChannelOSS::SetFormat(unsigned numChannels,
                              unsigned sampleRate,
                              unsigned bitsPerSample)
{
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF);

  // check parameters
  PAssert((bitsPerSample == 8) || (bitsPerSample == 16), PInvalidParameter);
  PAssert(numChannels >= 1 && numChannels <= 2, PInvalidParameter);

  // lock the dictionary
  PWaitAndSignal mutex(dictMutex);

  // the device must always be in the dictionary
  PAssertOS(handleDict().Contains(device));

  // get record for the device
  SoundHandleEntry & entry = handleDict()[device];

  if (entry.isInitialised) {
    if ((numChannels   != entry.numChannels) ||
        (sampleRate    != entry.sampleRate) ||
        (bitsPerSample != entry.bitsPerSample)) {
      PTRACE(6, "OSS\tTried to change read/write format without stopping");
      return PFalse;
    }
    return PTrue;
  }

  Abort();

  entry.numChannels   = numChannels;
  entry.sampleRate    = sampleRate;
  entry.bitsPerSample = bitsPerSample;
  entry.isInitialised  = PFalse;

  // mark this channel as uninitialised
  isInitialised = PFalse;

  return PTrue;
}

// Get  the number of channels (mono/stereo) in the sound.
unsigned PSoundChannelOSS::GetChannels()   const
{
  return mNumChannels;
}

// Get the sample rate in samples per second.
unsigned PSoundChannelOSS::GetSampleRate() const
{
  return actualSampleRate;
}

// Get the sample size in bits per sample.
unsigned PSoundChannelOSS::GetSampleSize() const
{
  return mBitsPerSample;
}

PBoolean PSoundChannelOSS::SetBuffers(PINDEX size, PINDEX count)
{
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF);

  //PINDEX totalSize = size * count;

  //size = 16;
  //count = (totalSize + 15) / 16;

  PAssert(size > 0 && count > 0 && count < 65536, PInvalidParameter);
  int arg = 1;
  while (size > (PINDEX)(1 << arg))
    arg++;

  arg |= count << 16;

  // lock the dictionary
  PWaitAndSignal mutex(dictMutex);

  // the device must always be in the dictionary
  PAssertOS(handleDict().Contains(device));

  // get record for the device
  SoundHandleEntry & entry = handleDict()[device];

  if (entry.isInitialised) {
    if (entry.fragmentValue != (unsigned)arg) {
      PTRACE(6, "OSS\tTried to change buffers without stopping");
      return PFalse;
    }
    return PTrue;
  }

  Abort();

  // set information in the common record
  entry.fragmentValue = arg;
  entry.isInitialised = PFalse;

  // flag this channel as not initialised
  isInitialised       = PFalse;

  return PTrue;
}


PBoolean PSoundChannelOSS::GetBuffers(PINDEX & size, PINDEX & count)
{
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF);

  // lock the dictionary
  PWaitAndSignal mutex(dictMutex);

  // the device must always be in the dictionary
  PAssertOS(handleDict().Contains(device));

  SoundHandleEntry & entry = handleDict()[device];

  int arg = entry.fragmentValue;

  count = arg >> 16;
  size = 1 << (arg&0xffff);
  return PTrue;
}


PBoolean PSoundChannelOSS::PlaySound(const PSound & sound, PBoolean wait)
{
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF);

  Abort();

  if (!Write((const BYTE *)sound, sound.GetSize()))
    return PFalse;

  if (wait)
    return WaitForPlayCompletion();

  return PTrue;
}


PBoolean PSoundChannelOSS::PlayFile(const PFilePath & filename, PBoolean wait)
{
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF);

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


PBoolean PSoundChannelOSS::HasPlayCompleted()
{
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF);

  audio_buf_info info;
  if (!ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_GETOSPACE, &info)))
    return PFalse;

  return info.fragments == info.fragstotal;
}


PBoolean PSoundChannelOSS::WaitForPlayCompletion()
{
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF);

  return ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_SYNC, NULL));
}


PBoolean PSoundChannelOSS::RecordSound(PSound & sound)
{
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF);

  return PFalse;
}


PBoolean PSoundChannelOSS::RecordFile(const PFilePath & filename)
{
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF);

  return PFalse;
}


PBoolean PSoundChannelOSS::StartRecording()
{
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF);

  if (os_handle == 0)
    return PTrue;

  P_fd_set fds(os_handle);
  P_timeval instant;
  return ConvertOSError(::select(1, fds, NULL, NULL, instant));
}


PBoolean PSoundChannelOSS::IsRecordBufferFull()
{
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF);

  audio_buf_info info;
  if (!ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_GETISPACE, &info)))
    return PFalse;

  return info.fragments > 0;
}


PBoolean PSoundChannelOSS::AreAllRecordBuffersFull()
{
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF);

  audio_buf_info info;
  if (!ConvertOSError(::ioctl(os_handle, SNDCTL_DSP_GETISPACE, &info)))
    return PFalse;

  return info.fragments == info.fragstotal;
}


PBoolean PSoundChannelOSS::WaitForRecordBufferFull()
{
  if (os_handle < 0)
    return SetErrorValues(NotOpen, EBADF);

  return PXSetIOBlock(PXReadBlock, readTimeout);
}


PBoolean PSoundChannelOSS::WaitForAllRecordBuffersFull()
{
  return PFalse;
}


PBoolean PSoundChannelOSS::Abort()
{
  return ConvertOSError(ioctl(os_handle, SNDCTL_DSP_RESET, NULL));
}



PBoolean PSoundChannelOSS::SetVolume(unsigned newVal)
{
  if (os_handle <= 0)  //CAnnot set volume in loop back mode.
    return PFalse;

  int rc, deviceVol = (newVal << 8) | newVal;

  if (direction  == Player) 
    rc = ::ioctl(os_handle, MIXER_WRITE(SOUND_MIXER_VOLUME), &deviceVol);
   else 
    rc = ::ioctl(os_handle, MIXER_WRITE(SOUND_MIXER_MIC), &deviceVol);

  if (rc < 0) {
    PTRACE(1, "PSoundChannelOSS::SetVolume failed : " << ::strerror(errno));
    return PFalse;
  }

  return PTrue;
}

PBoolean  PSoundChannelOSS::GetVolume(unsigned &devVol)
{
  if (os_handle <= 0)  //CAnnot get volume in loop back mode.
    return PFalse;
  
  int vol, rc;
  if (direction == Player)
    rc = ::ioctl(os_handle, MIXER_READ(SOUND_MIXER_VOLUME), &vol);
  else
    rc = ::ioctl(os_handle, MIXER_READ(SOUND_MIXER_MIC), &vol);
  
  if (rc < 0) {
    PTRACE(1,  "PSoundChannelOSS::GetVolume failed : " << ::strerror(errno)) ;
    return PFalse;
  }
  
  devVol = vol & 0xff;
  return PTrue;
}
  


// End of file
