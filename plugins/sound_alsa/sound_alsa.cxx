/*
 * sound_alsa.cxx
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
 * The Initial Developer of the Original ALSA Code is
 * Damien Sandras <dsandras@seconix.com>
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): /
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#pragma implementation "sound_alsa.h"

#include "sound_alsa.h"
#include <ptclib/pwavfile.h>

PCREATE_SOUND_PLUGIN(ALSA, PSoundChannelALSA)


static PStringToOrdinal playback_devices;
static PStringToOrdinal capture_devices;
PMutex dictionaryMutex;

///////////////////////////////////////////////////////////////////////////////

PSoundChannelALSA::PSoundChannelALSA()
{
  card_nr = 0;
  os_handle = NULL;
}


PSoundChannelALSA::PSoundChannelALSA(const PString & device,
                                          Directions dir,
                                            unsigned numChannels,
                                            unsigned sampleRate,
                                            unsigned bitsPerSample)
{
  card_nr = 0;
  os_handle = NULL;
  Open(device, dir, numChannels, sampleRate, bitsPerSample);
}


void PSoundChannelALSA::Construct()
{
//  enum _snd_pcm_format val;
//
//#if PBYTE_ORDER == PLITTLE_ENDIAN
//  val = (mBitsPerSample == 16) ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_U8;
//#else
//  val = (mBitsPerSample == 16) ? SND_PCM_FORMAT_S16_BE : SND_PCM_FORMAT_U8;
//#endif
//
  frameBytes = 2;
  m_bufferSize = 320; // 20 ms worth of 8kHz data
  m_bufferCount = 2;  // double buffering

  card_nr = 0;
  os_handle = NULL;
  isInitialised = false;
}


PSoundChannelALSA::~PSoundChannelALSA()
{
  Close();
}


void PSoundChannelALSA::UpdateDictionary(Directions dir)
{
  PWaitAndSignal mutex(dictionaryMutex);

  PStringToOrdinal & devices = dir == Recorder ? capture_devices : playback_devices;
  devices.RemoveAll();

  int cardNum = -1;
  if (snd_card_next(&cardNum) < 0 || cardNum < 0)
    return;  // No sound card found

  snd_ctl_card_info_t * info = NULL;
  snd_ctl_card_info_alloca(&info);

  snd_pcm_info_t * pcminfo = NULL;
  snd_pcm_info_alloca(&pcminfo);

  do {
    char card_id[32];
    snprintf(card_id, sizeof(card_id), "hw:%d", cardNum);

    snd_ctl_t * handle = NULL;
    if (snd_ctl_open(&handle, card_id, 0) == 0) {
      snd_ctl_card_info(handle, info);

      int dev = -1;
      for (;;) {
        snd_ctl_pcm_next_device(handle, &dev);
        if (dev < 0)
          break;

        snd_pcm_info_set_device(pcminfo, dev);
        snd_pcm_info_set_subdevice(pcminfo, 0);
        snd_pcm_info_set_stream(pcminfo, dir == Recorder ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK);

        if (snd_ctl_pcm_info(handle, pcminfo) >= 0) {
          char * rawName = NULL;
          snd_card_get_name(cardNum, &rawName);
          if (rawName != NULL) {
            int disambiguator = 1;
            PString uniqueName = rawName;
            while (devices.Contains(uniqueName)) {
              uniqueName = rawName;
              uniqueName.sprintf(" (%d)", disambiguator++);
            }

            devices.SetAt(uniqueName, cardNum);

#ifdef snd_card_get_name_allocates_on_the_heap
            /*snd_card_get_name is unclear on if the pointer returned is
              static or on the heap. A web search shows that about half
              the implemetations out there do the free() and half don't!

              As it is causing at least one user a problem with "Block XXXX
              not in heap!" errors being output to stderr, we take it out and
              suffer the memory leak, if there is one ....
             */
            free(rawName);
#endif
          }
        }
      }
      snd_ctl_close(handle);
    }

    snd_card_next(&cardNum);
  } while (cardNum >= 0);
}


PStringArray PSoundChannelALSA::GetDeviceNames(Directions dir)
{
  PStringArray devices;

  UpdateDictionary(dir);

  if (dir == Recorder) {
    if (capture_devices.GetSize() > 0)
      devices += "Default";
    for (PINDEX i = 0 ; i < capture_devices.GetSize() ; i++)
      devices += capture_devices.GetKeyAt(i);
  }
  else {
    if (playback_devices.GetSize() > 0)
      devices += "Default";
    for (PINDEX i = 0 ; i < playback_devices.GetSize() ; i++)
      devices += playback_devices.GetKeyAt(i);
  }

  return devices;
}


PString PSoundChannelALSA::GetDefaultDevice(Directions dir)
{
  PStringArray devicenames = PSoundChannelALSA::GetDeviceNames(dir);
  if (devicenames.IsEmpty())
    return PString::Empty();
  return devicenames[0];
}


PBoolean PSoundChannelALSA::Open(const PString & devName,
                                      Directions dir,
                                        unsigned numChannels,
                                        unsigned sampleRate,
                                        unsigned bitsPerSample)
{
  Close();

  direction = dir;
  mNumChannels = numChannels;
  mSampleRate = sampleRate;
  mBitsPerSample = bitsPerSample;

  Construct();

  PWaitAndSignal m(device_mutex);

  PString real_device_name;
  if (devName == "Default") {
    real_device_name = "default";
    card_nr = -2;
  }
  else {
    PStringToOrdinal & devices = dir == Recorder ? capture_devices : playback_devices;
    if (devices.IsEmpty())
      UpdateDictionary(dir);

    POrdinalKey * index = devices.GetAt(devName);
    if (index == NULL) {
      PTRACE(1, "ALSA\tDevice not found");
      return false;
    }

    real_device_name = "plughw:" + PString(*index);
    card_nr = *index;
  }

  /* Open in NONBLOCK mode */
  if (snd_pcm_open(&os_handle,
                   real_device_name,
                   dir == Recorder ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK,
                   SND_PCM_NONBLOCK) < 0) {
    PTRACE(1, "ALSA\tOpen Failed");
    return false;
  }

  snd_pcm_nonblock(os_handle, 0);

  /* save internal parameters */
  device = real_device_name;

  Setup();
  PTRACE(3, "ALSA\tDevice " << device << " Opened");

  return true;
}

bool PSoundChannelALSA::SetHardwareParams()
{
  PTRACE(4,"ALSA\tSetHardwareParams " << ((direction == Player) ? "Player" : "Recorder") << " channels=" << mNumChannels
	   << " sample rate=" << mSampleRate);

  if (!os_handle)
    return SetErrorValues(NotOpen, EBADF);

  enum _snd_pcm_format sndFormat;
#if PBYTE_ORDER == PLITTLE_ENDIAN
  sndFormat = (mBitsPerSample == 16) ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_U8;
#else
  sndFormat = (mBitsPerSample == 16) ? SND_PCM_FORMAT_S16_BE : SND_PCM_FORMAT_U8;
#endif

  frameBytes = (mNumChannels * (snd_pcm_format_width(sndFormat) / 8));

  if (frameBytes == 0)
    frameBytes = 2;
  int err;

  // Finally set the hardware parameters
  for (unsigned retry = 0; retry < 100; ++retry) {
    snd_pcm_hw_params_t *hw_params = NULL;
    snd_pcm_hw_params_alloca(&hw_params);

    if ((err = snd_pcm_hw_params_any(os_handle, hw_params)) < 0) {
      PTRACE(1, "ALSA\tCannot initialize hardware parameter structure: " << snd_strerror(err));
      return false;
    }


    if ((err = snd_pcm_hw_params_set_access(os_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
      PTRACE(1, "ALSA\tCannot set access type: " <<  snd_strerror(err));
      return false;
    }


    if ((err = snd_pcm_hw_params_set_format(os_handle, hw_params, sndFormat)) < 0) {
      PTRACE(1, "ALSA\tCannot set sample format: " << snd_strerror(err));
      return false;
    }


    if ((err = snd_pcm_hw_params_set_channels(os_handle, hw_params, mNumChannels)) < 0) {
      PTRACE(1, "ALSA\tCannot set channel count: " << snd_strerror(err));
      return false;
    }

    if ((err = snd_pcm_hw_params_set_rate_near(os_handle, hw_params, &mSampleRate, NULL)) < 0) {
      PTRACE(1, "ALSA\tCannot set sample rate: " << snd_strerror(err));
      return false;
    }

    int dir = 0;
    int totalBufferSize = m_bufferSize*m_bufferCount;
    snd_pcm_uframes_t desiredPeriodSize = m_bufferSize/frameBytes;

    /* use of get function (ie. snd_pcm_hw_params_get_period_size) and the check as done before was, in my opinion, was pretty unuseful
       because actually the set function (ie.snd_pcm_hw_params_set_period_size_near) returns the real set value
       in the argument passed (ie. desiredPeriodSize) */

    if ((err = snd_pcm_hw_params_set_period_size_near(os_handle, hw_params, &desiredPeriodSize, &dir)) < 0) {
       PTRACE(1, "ALSA\tCannot set period size: " << snd_strerror(err));
    }
    else {
       PTRACE(4, "ALSA\tSuccessfully set period size to " << desiredPeriodSize);
    }

    /* i experimented (3 different sound cards) that is better to rounds value to the nearest integer to avoid buffer underrun/overrun */
    unsigned desiredPeriods = (unsigned)(((float)totalBufferSize / (float)(desiredPeriodSize*frameBytes))+0.5);

    if (desiredPeriods < 2) desiredPeriods = 2;

    if ((err = (int) snd_pcm_hw_params_set_periods_near(os_handle, hw_params, &desiredPeriods, &dir)) < 0) {
      PTRACE(1, "ALSA\tCannot set periods to: " << snd_strerror(err));
    }
    else {
      PTRACE(4, "ALSA\tSuccessfully set periods to " << desiredPeriods);
    }

    if ((err = snd_pcm_hw_params(os_handle, hw_params)) >= 0) {
      PTRACE(4, "ALSA\tparameters set ok");
      isInitialised = true;
      return true;
    }

    if (err != -EAGAIN && err != -EBADFD)
      break;

    PTRACE(4, "ALSA\tRetrying after temporary error: " << snd_strerror(err));
    usleep(1000);
  }

  PTRACE(1, "ALSA\tCannot set parameters: " << snd_strerror(err));
  return false;
}

PBoolean PSoundChannelALSA::Setup()
{
  if (os_handle == NULL) {
    PTRACE(6, "ALSA\tSkipping setup of " << device << " as not open");
    return false;
  }

  if (isInitialised) {
    PTRACE(6, "ALSA\tSkipping setup of " << device << " as instance already initialised");
    return true;
  }

  return SetHardwareParams();
}


PBoolean PSoundChannelALSA::Close()
{
  PWaitAndSignal m(device_mutex);

  /* if the channel isn't open, do nothing */
  if (!os_handle)
    return false;

  PTRACE(3, "ALSA\tClosing " << device);
  snd_pcm_close(os_handle);
  os_handle = NULL;
  isInitialised = false;

  return true;
}


PBoolean PSoundChannelALSA::Write(const void *buf, PINDEX len)
{
  lastWriteCount = 0;

  PWaitAndSignal m(device_mutex);

  if ((!isInitialised && !Setup()) || !len || !os_handle)
    return false;

  int pos = 0, max_try = 0;
  const char * buf2 = (const char *)buf;
  do {
    /* the number of frames to read is the buffer length
    divided by the size of one frame */
    long r = snd_pcm_writei(os_handle, (char *) &buf2 [pos], len / frameBytes);

    if (r >= 0) {
      pos += r * frameBytes;
      len -= r * frameBytes;
      lastWriteCount += r * frameBytes;
    }
    else {
      PTRACE(5, "ALSA\tBuffer underrun detected. Recovering... ");
      if (r == -EPIPE) {    /* under-run */
        r = snd_pcm_prepare(os_handle);
        PTRACE_IF(1, r < 0, "ALSA\tCould not prepare device: " << snd_strerror(r));
      }
      else if (r == -ESTRPIPE) {
        PTRACE(5, "ALSA\tOutput suspended. Resuming... ");
        while ((r = snd_pcm_resume(os_handle)) == -EAGAIN)
          sleep(1);       /* wait until the suspend flag is released */

        if (r < 0) {
          r = snd_pcm_prepare(os_handle);
          PTRACE_IF(1, r < 0, "ALSA\tCould not prepare device: " << snd_strerror(r));
        }
      }
      else {
        PTRACE(1, "ALSA\tCould not write " << max_try << " " << len << " " << snd_strerror(r));
      }

      max_try++;
      if (max_try > 5)
        return false;
    }
  } while (len > 0);

  return true;
}


PBoolean PSoundChannelALSA::Read(void * buf, PINDEX len)
{
  lastReadCount = 0;

  PWaitAndSignal m(device_mutex);

  if ((!isInitialised && !Setup()) || !len || !os_handle)
    return false;

  memset((char *) buf, 0, len);

  int pos = 0, max_try = 0;
  char * buf2 = (char *)buf;

  do {
    /* the number of frames to read is the buffer length
    divided by the size of one frame */
    long r = snd_pcm_readi(os_handle, &buf2[pos],len/frameBytes);

    if (r >= 0) {
      pos += r * frameBytes;
      len -= r * frameBytes;
      lastReadCount += r * frameBytes;
    }
    else {
      if (r == -EPIPE) {    /* under-run */
	  snd_pcm_prepare(os_handle);
      }
      else if (r == -ESTRPIPE) {
        while ((r = snd_pcm_resume(os_handle)) == -EAGAIN)
          sleep(1);       /* wait until the suspend flag is released */

        if (r < 0)
          snd_pcm_prepare(os_handle);
      }

      PTRACE(1, "ALSA\tCould not read " << max_try << " " << len << " " << snd_strerror(r));

      max_try++;

      if (max_try > 5)
        return false;
    }
  } while (len > 0);

  return true;
}


PBoolean PSoundChannelALSA::SetFormat(unsigned numChannels,
                                      unsigned sampleRate,
                                      unsigned bitsPerSample)
{
  if (!os_handle)
    return SetErrorValues(NotOpen, EBADF);

  /* check parameters */
  PAssert((bitsPerSample == 8) || (bitsPerSample == 16), PInvalidParameter);
  PAssert(numChannels >= 1 && numChannels <= 2, PInvalidParameter);

  mNumChannels   = numChannels;
  mSampleRate    = sampleRate;
  mBitsPerSample = bitsPerSample;

  /* mark this channel as uninitialised */
  isInitialised = false;

  return true;
}


unsigned PSoundChannelALSA::GetChannels() const
{
  return mNumChannels;
}


unsigned PSoundChannelALSA::GetSampleRate() const
{
  return mSampleRate;
}


unsigned PSoundChannelALSA::GetSampleSize() const
{
  return mBitsPerSample;
}


PBoolean PSoundChannelALSA::SetBuffers(PINDEX size, PINDEX count)
{
  PTRACE(4,"ALSA\tSetBuffers direction=" <<
	         ((direction == Player) ? "Player" : "Recorder") << " size=" << size << " count=" << count);

  m_bufferSize = size;
  m_bufferCount = count;

  /* set actually new parameters */
  return SetHardwareParams();
}


PBoolean PSoundChannelALSA::GetBuffers(PINDEX & size, PINDEX & count)
{
  size = m_bufferSize;
  count = m_bufferCount;
  return true;
}


PBoolean PSoundChannelALSA::PlaySound(const PSound & sound, PBoolean wait)
{
  if (!os_handle)
    return SetErrorValues(NotOpen, EBADF);

  if (!Write((const BYTE *)sound, sound.GetSize()))
    return false;

  if (wait)
    return WaitForPlayCompletion();

  return true;
}


PBoolean PSoundChannelALSA::PlayFile(const PFilePath & filename, PBoolean wait)
{
  BYTE buffer [512];
  PTRACE(1, "ALSA\tPlayFile " << filename);

  if (!os_handle)
    return SetErrorValues(NotOpen, EBADF);

  /* use PWAVFile instead of PFile -> skips wav header bytes */

  PWAVFile file(filename, PFile::ReadOnly,PWAVFile::fmt_NotKnown);
  snd_pcm_prepare(os_handle);

  if (!file.IsOpen())
    return false;

  for (;;) {
    if (!file.Read(buffer, 512))
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

  return true;
}


PBoolean PSoundChannelALSA::HasPlayCompleted()
{
  if (!os_handle)
    return SetErrorValues(NotOpen, EBADF);

  return (snd_pcm_state(os_handle) != SND_PCM_STATE_RUNNING);
}


PBoolean PSoundChannelALSA::WaitForPlayCompletion()
{
  if (!os_handle)
    return SetErrorValues(NotOpen, EBADF);

  snd_pcm_drain(os_handle);

  return true;
}


PBoolean PSoundChannelALSA::RecordSound(PSound & sound)
{
  return false;
}


PBoolean PSoundChannelALSA::RecordFile(const PFilePath & filename)
{
  return false;
}


PBoolean PSoundChannelALSA::StartRecording()
{
  return false;
}


PBoolean PSoundChannelALSA::IsRecordBufferFull()
{
  return true;
}


PBoolean PSoundChannelALSA::AreAllRecordBuffersFull()
{
  return true;
}


PBoolean PSoundChannelALSA::WaitForRecordBufferFull()
{
  return true;
}


PBoolean PSoundChannelALSA::WaitForAllRecordBuffersFull()
{
  return false;
}


PBoolean PSoundChannelALSA::Abort()
{
  int r = 0;

  if (!os_handle)
    return false;

  PTRACE(4, "ALSA\tAborting " << device);
  if ((r = snd_pcm_drain(os_handle)) < 0) {
    PTRACE(1, "ALSA\tCannot abort" << snd_strerror(r));
    return false;
  }

  return true;
}



PBoolean PSoundChannelALSA::SetVolume(unsigned newVal)
{
  unsigned i = 0;
  return Volume(true, newVal, i);
}


PBoolean  PSoundChannelALSA::GetVolume(unsigned &devVol)
{
  return Volume(false, 0, devVol);
}


PBoolean PSoundChannelALSA::IsOpen() const
{
  return os_handle != NULL;
}


PBoolean PSoundChannelALSA::Volume(PBoolean set, unsigned set_vol, unsigned &get_vol)
{
  int err = 0;
  snd_mixer_t *handle;
  snd_mixer_elem_t *elem;
  snd_mixer_selem_id_t *sid;

  const char *play_mix_name [] = { "PCM", "Master", "Speaker", NULL };
  const char *rec_mix_name [] = { "Capture", "Mic", NULL };
  PString card_name;

  long pmin = 0, pmax = 0;
  long int vol = 0;
  int i = 0;

  if (!os_handle)
    return false;

  if (card_nr == -2)
    card_name = "default";
  else
    card_name = "hw:" + PString(card_nr);

  //allocate simple id
  snd_mixer_selem_id_alloca(&sid);

  //sets simple-mixer index and name
  snd_mixer_selem_id_set_index(sid, 0);

  if ((err = snd_mixer_open(&handle, 0)) < 0) {
    PTRACE(1, "ALSA\tMixer open error: " << snd_strerror(err));
    return false;
  }

  if ((err = snd_mixer_attach(handle, card_name)) < 0) {
    PTRACE(1, "ALSA\tMixer attach " << card_name << " error: " << snd_strerror(err));
    snd_mixer_close(handle);
    return false;
  }

  if ((err = snd_mixer_selem_register(handle, NULL, NULL)) < 0) {
    PTRACE(1, "ALSA\tMixer register error: " << snd_strerror(err));
    snd_mixer_close(handle);
    return false;
  }

  err = snd_mixer_load(handle);
  if (err < 0) {
    PTRACE(1, "ALSA\tMixer load error: " << snd_strerror(err));
    snd_mixer_close(handle);
    return false;
  }

  do {
    snd_mixer_selem_id_set_name(sid, (direction == Player)?play_mix_name[i]:rec_mix_name[i]);
    elem = snd_mixer_find_selem(handle, sid);
    i++;
  } while (!elem && ((direction == Player && play_mix_name[i] != NULL) || (direction == Recorder && rec_mix_name[i] != NULL)));

  if (!elem) {
    PTRACE(1, "ALSA\tUnable to find simple control.");
    snd_mixer_close(handle);
    return false;
  }

  if (set) {
    if (direction == Player) {
      snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
      vol = (set_vol * (pmax?pmax:31)) / 100;
      snd_mixer_selem_set_playback_volume_all(elem, vol);
    }
    else {
      snd_mixer_selem_get_capture_volume_range(elem, &pmin, &pmax);
      vol = (set_vol * (pmax?pmax:31)) / 100;
      snd_mixer_selem_set_capture_volume_all(elem, vol);
    }
    PTRACE(4, "ALSA\tSet volume to " << vol);
  }
  else {
    if (direction == Player) {
      snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
      snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &vol);
    }
    else {
      snd_mixer_selem_get_capture_volume_range(elem, &pmin, &pmax);
      snd_mixer_selem_get_capture_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &vol);
    }

    get_vol = (vol * 100) / (pmax?pmax:31);
    PTRACE(4, "ALSA\tGot volume " << vol);
  }

  snd_mixer_close(handle);

  return true;
}
