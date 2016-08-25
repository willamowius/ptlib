/*
 * esdaudio.cxx
 *
 * Sound driver implementation to use ESound.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is
 * Shawn Pai-Hsiang Hsiao <shawn@eecs.harvard.edu>
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#pragma implementation "sound.h"

#include <ptlib.h>
#include <esd.h>

#pragma implementation "sound_esd.h"

#include "sound_esd.h"

PCREATE_SOUND_PLUGIN(ESD, PSoundChannelESD);

///////////////////////////////////////////////////////////////////////////////

PSoundChannelESD::PSoundChannelESD()
{
  Construct();
}


PSoundChannelESD::PSoundChannelESD(const PString & device,
                             Directions dir,
                             unsigned numChannels,
                             unsigned sampleRate,
                             unsigned bitsPerSample)
{
  Construct();
  Open(device, dir, numChannels, sampleRate, bitsPerSample);
}


void PSoundChannelESD::Construct()
{
}


PSoundChannelESD::~PSoundChannelESD()
{
  Close();
}


PStringArray PSoundChannelESD::GetDeviceNames(Directions /*dir*/)
{
  PStringArray array;

  array[0] = "ESound";

  return array;
}


PString PSoundChannelESD::GetDefaultDevice(Directions /*dir*/)
{
  return "ESound";
}

PBoolean PSoundChannelESD::Open(const PString & device,
                         Directions dir,
                         unsigned numChannels,
                         unsigned sampleRate,
                         unsigned bitsPerSample)
{
  int bits, channels, rate, mode, func;
  esd_format_t format;
  char *host = NULL, *name = NULL;

  Close();

  // make sure we have proper bits per sample
  switch (bitsPerSample) {
  case 16:
    bits = ESD_BITS16;
    break;
  case 8:
    bits = ESD_BITS8;
    break;
  default:
    return (PFalse);
  }

  // make sure we have proper number of channels
  switch (numChannels) {
  case 2:
    channels = ESD_STEREO;
    break;
  case 1:
    channels = ESD_MONO;
    break;
  default:
    return (PFalse);
  }

  rate = sampleRate;
  mode = ESD_STREAM;

  // a separate stream for Player and Recorder
  switch (dir) {
  case Recorder:
    func = ESD_RECORD;
    break;
  case Player:
    func = ESD_PLAY;
    break;
  default:
    return (PFalse);
  }

  format = bits | channels | mode | func;
  if (dir == Recorder) 
    os_handle = esd_record_stream_fallback( format, rate, host, name );
  else
    os_handle = esd_play_stream_fallback( format, rate, host, name );

  if ( os_handle <= 0 ) 
    return (PFalse);

  return SetFormat(numChannels, sampleRate, bitsPerSample);
}

PBoolean PSoundChannelESD::SetVolume(unsigned newVal)
{
  if (os_handle <= 0)  //Cannot set volume in loop back mode.
    return PFalse;
  
  return PFalse;
}

PBoolean  PSoundChannelESD::GetVolume(unsigned &devVol)
{
  if (os_handle <= 0)  //Cannot get volume in loop back mode.
    return PFalse;
  
  devVol = 0;
  return PFalse;
}
  


PBoolean PSoundChannelESD::Close()
{
  /* I think there is a bug here. We should be testing for loopback mode
   * and NOT calling PChannel::Close() when we are in loopback mode.
   * (otherwise we close file handle 0 which is stdin)
   */

  return PChannel::Close();
}


PBoolean PSoundChannelESD::SetFormat(unsigned numChannels,
                              unsigned sampleRate,
                              unsigned bitsPerSample)
{
  PAssert(numChannels >= 1 && numChannels <= 2, PInvalidParameter);
  PAssert(bitsPerSample == 8 || bitsPerSample == 16, PInvalidParameter);

  return PTrue;
}


PBoolean PSoundChannelESD::SetBuffers(PINDEX size, PINDEX count)
{
  Abort();

  PAssert(size > 0 && count > 0 && count < 65536, PInvalidParameter);

  return PTrue;
}


PBoolean PSoundChannelESD::GetBuffers(PINDEX & size, PINDEX & count)
{
  return PTrue;
}


PBoolean PSoundChannelESD::Write(const void * buf, PINDEX len)
{
  int rval;

  if (os_handle >= 0) {

    // Sends data to esd.
    rval = ::write(os_handle, buf, len);
    if (rval > 0) {
#if defined(P_MACOSX)
      // Mac OS X's esd has a big input buffer so we need to simulate
      // writing data out at the correct rate.
      writeDelay.Delay(len/16);
#endif
      return (PTrue);
    }
  }

  return PFalse;
}


PBoolean PSoundChannelESD::PlaySound(const PSound & sound, PBoolean wait)
{
  Abort();

  if (!Write((const BYTE *)sound, sound.GetSize()))
    return PFalse;

  if (wait)
    return WaitForPlayCompletion();

  return PTrue;
}


PBoolean PSoundChannelESD::PlayFile(const PFilePath & filename, PBoolean wait)
{
  return PTrue;
}


PBoolean PSoundChannelESD::HasPlayCompleted()
{
  return PFalse;
}


PBoolean PSoundChannelESD::WaitForPlayCompletion()
{
  return PTrue;
}


PBoolean PSoundChannelESD::Read(void * buf, PINDEX len)
{
  if (os_handle < 0) 
    return PFalse;

  lastReadCount = 0;
  // keep looping until we have read 'len' bytes
  while (lastReadCount < len) {
    int retval = ::read(os_handle, ((char *)buf)+lastReadCount, len-lastReadCount);
    if (retval <= 0) return PFalse;
    lastReadCount += retval;
  }
  return (PTrue);
}


PBoolean PSoundChannelESD::RecordSound(PSound & sound)
{
  return PTrue;
}


PBoolean PSoundChannelESD::RecordFile(const PFilePath & filename)
{
  return PTrue;
}


PBoolean PSoundChannelESD::StartRecording()
{
  return (PTrue);
}


PBoolean PSoundChannelESD::IsRecordBufferFull()
{
  return (PTrue);
}


PBoolean PSoundChannelESD::AreAllRecordBuffersFull()
{
  return PTrue;
}


PBoolean PSoundChannelESD::WaitForRecordBufferFull()
{
  if (os_handle < 0) {
    return PFalse;
  }

  return PXSetIOBlock(PXReadBlock, readTimeout);
}


PBoolean PSoundChannelESD::WaitForAllRecordBuffersFull()
{
  return PFalse;
}


PBoolean PSoundChannelESD::Abort()
{
  return PTrue;
}


// End of file

