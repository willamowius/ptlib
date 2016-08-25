/*
 * sound.cxx
 *
 * Code for pluigns sound device
 *
 * Portable Windows Library
 *
 * Copyright (c) 2003 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): Craig Southeren
 *                 Snark at GnomeMeeting
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifdef __GNUC__
#pragma implementation "sound.h"
#endif

#include <ptlib.h>

#include <ptlib/sound.h>
#include <ptlib/pluginmgr.h>
#include <ptclib/delaychan.h>


static const char soundPluginBaseClass[] = "PSoundChannel";


template <> PSoundChannel * PDevicePluginFactory<PSoundChannel>::Worker::Create(const PString & type) const
{
  return PSoundChannel::CreateChannel(type);
}

typedef PDevicePluginAdapter<PSoundChannel> PDevicePluginSoundChannel;
PFACTORY_CREATE(PFactory<PDevicePluginAdapterBase>, PDevicePluginSoundChannel, "PSoundChannel", true);


PStringArray PSoundChannel::GetDriverNames(PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return pluginMgr->GetPluginsProviding(soundPluginBaseClass);
}


PStringArray PSoundChannel::GetDriversDeviceNames(const PString & driverName,
                                                  PSoundChannel::Directions dir,
                                                  PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return pluginMgr->GetPluginsDeviceNames(driverName, soundPluginBaseClass, dir);
}


PSoundChannel * PSoundChannel::CreateChannel(const PString & driverName, PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return (PSoundChannel *)pluginMgr->CreatePluginsDevice(driverName, soundPluginBaseClass, 0);
}


PSoundChannel * PSoundChannel::CreateChannelByName(const PString & deviceName,
                                                   PSoundChannel::Directions dir,
                                                   PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return (PSoundChannel *)pluginMgr->CreatePluginsDeviceByName(deviceName, soundPluginBaseClass, dir);
}


PSoundChannel * PSoundChannel::CreateOpenedChannel(const PString & driverName,
                                                   const PString & deviceName,
                                                   PSoundChannel::Directions dir,
                                                   unsigned numChannels,
                                                   unsigned sampleRate,
                                                   unsigned bitsPerSample,
                                                   PPluginManager * pluginMgr)
{
  PString adjustedDeviceName = deviceName;
  PSoundChannel * sndChan;
  if (driverName.IsEmpty() || driverName == "*") {
    if (deviceName.IsEmpty() || deviceName == "*")
      adjustedDeviceName = PSoundChannel::GetDefaultDevice(dir);
    sndChan = CreateChannelByName(adjustedDeviceName, dir, pluginMgr);
  }
  else {
    if (deviceName.IsEmpty() || deviceName == "*") {
      PStringArray devices = PSoundChannel::GetDriversDeviceNames(driverName, PSoundChannel::Player);
      if (devices.IsEmpty())
        return NULL;
      adjustedDeviceName = devices[0];
    }
    sndChan = CreateChannel(driverName, pluginMgr);
  }

  if (sndChan != NULL && sndChan->Open(adjustedDeviceName, dir, numChannels, sampleRate, bitsPerSample))
    return sndChan;

  delete sndChan;
  return NULL;
}


PStringArray PSoundChannel::GetDeviceNames(PSoundChannel::Directions dir, PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return pluginMgr->GetPluginsDeviceNames("*", soundPluginBaseClass, dir);
}


PString PSoundChannel::GetDefaultDevice(Directions dir)
{
#ifdef _WIN32
  RegistryKey registry("HKEY_CURRENT_USER\\Software\\Microsoft\\Multimedia\\Sound Mapper",
                       RegistryKey::ReadOnly);

  PString str;

  if (dir == Player) {
    if (registry.QueryValue("ConsoleVoiceComPlayback", str) )
      return str;
    if (registry.QueryValue("Playback", str))
      return str;
  }
  else {
    if (registry.QueryValue("ConsoleVoiceComRecord", str))
      return str;
    if (registry.QueryValue("Record", str))
      return str;
  }
#endif

  PStringArray devices = GetDeviceNames(dir);

  if (devices.GetSize() == 0)
    return PString::Empty();

  for (PINDEX i = 0; i < devices.GetSize(); ++i) {
    if (!(devices[i] *= "NULL"))
      return devices[i];
  }

  return devices[0];
}


PSoundChannel::PSoundChannel()
  : m_baseChannel(NULL)
  , activeDirection(Closed)
{
}

PSoundChannel::~PSoundChannel()
{
  delete m_baseChannel;
}


PSoundChannel::PSoundChannel(const PString & device,
                             Directions dir,
                             unsigned numChannels,
                             unsigned sampleRate,
                             unsigned bitsPerSample)
  : m_baseChannel(NULL)
  , activeDirection(dir)
{
  Open(device, dir, numChannels, sampleRate, bitsPerSample);
}


PBoolean PSoundChannel::Open(const PString & devSpec,
                         Directions dir,
                         unsigned numChannels,
                         unsigned sampleRate,
                         unsigned bitsPerSample)
{
  PString driver, device;
  PINDEX colon = devSpec.Find(':');
  if (colon == P_MAX_INDEX)
    device = devSpec;
  else {
    driver = devSpec.Left(colon);
    device = devSpec.Mid(colon+1).Trim();
  }

  m_baseMutex.StartWrite();

  delete m_baseChannel;
  activeDirection = dir;

  m_baseChannel = CreateOpenedChannel(driver, device, dir, numChannels, sampleRate, bitsPerSample);
  if (m_baseChannel == NULL && !driver.IsEmpty())
    m_baseChannel = CreateOpenedChannel(PString::Empty(), devSpec, dir, numChannels, sampleRate, bitsPerSample);

  m_baseMutex.EndWrite();

  return m_baseChannel != NULL;
}


PString PSoundChannel::GetName() const
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL ? m_baseChannel->GetName() : PString::Empty();
}


PBoolean PSoundChannel::IsOpen() const
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->PChannel::IsOpen();
}


PBoolean PSoundChannel::Close()
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel == NULL || m_baseChannel->Close();
}


int PSoundChannel::GetHandle() const
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel == NULL ? -1 : m_baseChannel->PChannel::GetHandle();
}


PBoolean PSoundChannel::Abort()
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel == NULL || m_baseChannel->Abort();
}


PBoolean PSoundChannel::SetFormat(unsigned numChannels, unsigned sampleRate, unsigned bitsPerSample)
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->SetFormat(numChannels, sampleRate, bitsPerSample);
}


unsigned PSoundChannel::GetChannels() const
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel == NULL ? 0 : m_baseChannel->GetChannels();
}


unsigned PSoundChannel::GetSampleRate() const
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel == NULL ? 0 : m_baseChannel->GetSampleRate();
}


unsigned PSoundChannel::GetSampleSize() const 
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel == NULL ? 0 : m_baseChannel->GetSampleSize();
}


PBoolean PSoundChannel::SetBuffers(PINDEX size, PINDEX count)
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->SetBuffers(size, count);
}


PBoolean PSoundChannel::GetBuffers(PINDEX & size, PINDEX & count)
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->GetBuffers(size, count);
}


PBoolean PSoundChannel::SetVolume(unsigned volume)
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->SetVolume(volume);
}


PBoolean PSoundChannel::GetMute(bool & mute)
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->GetMute(mute);
}


PBoolean PSoundChannel::SetMute(bool mute)
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->SetMute(mute);
}


PBoolean PSoundChannel::GetVolume(unsigned & volume)
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->GetVolume(volume);
}


PBoolean PSoundChannel::Write(const void * buf, PINDEX len)
{
  PAssert(activeDirection == Player, PLogicError);

  if (len == 0)
    return IsOpen();

  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->Write(buf, len);
}

PBoolean PSoundChannel::Write(const void * buf, PINDEX len, const void * /*mark*/)
{
  return Write(buf, len);
}

PINDEX PSoundChannel::GetLastWriteCount() const
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL ? m_baseChannel->GetLastWriteCount() : PChannel::GetLastWriteCount();
}


PBoolean PSoundChannel::PlaySound(const PSound & sound, PBoolean wait)
{
  PAssert(activeDirection == Player, PLogicError);
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->PlaySound(sound, wait);
}


PBoolean PSoundChannel::PlayFile(const PFilePath & file, PBoolean wait)
{
  PAssert(activeDirection == Player, PLogicError);
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->PlayFile(file, wait);
}


PBoolean PSoundChannel::HasPlayCompleted()
{
  PAssert(activeDirection == Player, PLogicError);
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->HasPlayCompleted();
}


PBoolean PSoundChannel::WaitForPlayCompletion() 
{
  PAssert(activeDirection == Player, PLogicError);
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->WaitForPlayCompletion();
}


PBoolean PSoundChannel::Read(void * buf, PINDEX len)
{
  PAssert(activeDirection == Recorder, PLogicError);

  if (len == 0)
    return IsOpen();

  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->Read(buf, len);
}


PINDEX PSoundChannel::GetLastReadCount() const
{
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL ? m_baseChannel->GetLastReadCount() : PChannel::GetLastReadCount();
}


PBoolean PSoundChannel::RecordSound(PSound & sound)
{
  PAssert(activeDirection == Recorder, PLogicError);
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->RecordSound(sound);
}


PBoolean PSoundChannel::RecordFile(const PFilePath & file)
{
  PAssert(activeDirection == Recorder, PLogicError);
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->RecordFile(file);
}


PBoolean PSoundChannel::StartRecording()
{
  PAssert(activeDirection == Recorder, PLogicError);
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->StartRecording();
}


PBoolean PSoundChannel::IsRecordBufferFull() 
{
  PAssert(activeDirection == Recorder, PLogicError);
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->IsRecordBufferFull();
}


PBoolean PSoundChannel::AreAllRecordBuffersFull() 
{
  PAssert(activeDirection == Recorder, PLogicError);
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->AreAllRecordBuffersFull();
}


PBoolean PSoundChannel::WaitForRecordBufferFull() 
{
  PAssert(activeDirection == Recorder, PLogicError);
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->WaitForRecordBufferFull();
}


PBoolean PSoundChannel::WaitForAllRecordBuffersFull() 
{
  PAssert(activeDirection == Recorder, PLogicError);
  PReadWaitAndSignal mutex(m_baseMutex);
  return m_baseChannel != NULL && m_baseChannel->WaitForAllRecordBuffersFull();
}


const char * PSoundChannel::GetDirectionText(Directions dir)
{
  switch (dir) {
    case Player :
      return "Playback";
    case Recorder :
      return "Recording";
    case Closed :
      return "Closed";
  }

  return "<Unknown>";
}


///////////////////////////////////////////////////////////////////////////

#if !defined(_WIN32) && !defined(__BEOS__) && !defined(__APPLE__)

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
  return Play(PSoundChannel::GetDefaultDevice(PSoundChannel::Player));
}


PBoolean PSound::Play(const PString & device)
{

  PSoundChannel channel(device, PSoundChannel::Player);
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


#endif //_WIN32

///////////////////////////////////////////////////////////////////////////

static const PConstString NullAudio("Null Audio");

class PSoundChannelNull : public PSoundChannel
{
 PCLASSINFO(PSoundChannelNull, PSoundChannel);
 public:
    PSoundChannelNull()
      : m_sampleRate(0)
    {
    }

    PSoundChannelNull(
      const PString &device,
      PSoundChannel::Directions dir,
      unsigned numChannels,
      unsigned sampleRate,
      unsigned bitsPerSample
    ) : m_sampleRate(0)
    {
      Open(device, dir, numChannels, sampleRate, bitsPerSample);
    }

    static PStringArray GetDeviceNames(PSoundChannel::Directions = Player)
    {
      return NullAudio;
    }

    PBoolean Open(const PString &,
                  Directions dir,
                  unsigned numChannels,
                  unsigned sampleRate,
                  unsigned bitsPerSample)
    {
      activeDirection = dir;
      return SetFormat(numChannels, sampleRate, bitsPerSample);
    }

    virtual PString GetName() const
    {
      return NullAudio;
    }

    PBoolean Close()
    {
      m_sampleRate = 0;
      return true;
    }

    PBoolean IsOpen() const
    {
      return m_sampleRate > 0;
    }

    PBoolean Write(const void *, PINDEX len)
    {
      if (m_sampleRate <= 0)
        return false;

      lastWriteCount = len;
      m_Pacing.Delay(len/2*1000/m_sampleRate);
      return true;
    }

    PBoolean Read(void * buf, PINDEX len)
    {
      if (m_sampleRate <= 0)
        return false;

      memset(buf, 0, len);
      lastReadCount = len;
      m_Pacing.Delay(len/2*1000/m_sampleRate);
      return true;
    }

    PBoolean SetFormat(unsigned numChannels,
                   unsigned sampleRate,
                   unsigned bitsPerSample)
    {
      m_sampleRate = sampleRate;
      return numChannels == 1 && bitsPerSample == 16;
    }

    unsigned GetChannels() const
    {
      return 1;
    }

    unsigned GetSampleRate() const
    {
      return m_sampleRate;
    }

    unsigned GetSampleSize() const
    {
      return 16;
    }

    PBoolean SetBuffers(PINDEX, PINDEX)
    {
      return true;
    }

    PBoolean GetBuffers(PINDEX & size, PINDEX & count)
    {
      size = 2;
      count = 1;
      return true;
    }

protected:
    unsigned       m_sampleRate;
    PAdaptiveDelay m_Pacing;
};


PCREATE_SOUND_PLUGIN(NullAudio, PSoundChannelNull)
