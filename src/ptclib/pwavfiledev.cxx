/*
 * pwavfiledev.cxx
 *
 * Implementation of sound file device
 *
 * Portable Windows Library
 *
 * Copyright (C) 2007 Post Increment
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
 * Robert Jongbloed <robertj@postincrement.com>
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifdef __GNUC__
#pragma implementation "pwavfiledev.h"
#endif

#include <ptlib.h>
#include <ptclib/pwavfiledev.h>


class PSoundChannel_WAVFile_PluginServiceDescriptor : public PDevicePluginServiceDescriptor
{
  public:
    virtual PObject * CreateInstance(int /*userData*/) const
    {
        return new PSoundChannel_WAVFile;
    }
    virtual PStringArray GetDeviceNames(int userData) const
    {
        return PSoundChannel_WAVFile::GetDeviceNames((PSoundChannel::Directions)userData);
    }
    virtual bool ValidateDeviceName(const PString & deviceName, int userData) const
    {
      PFilePath pathname = deviceName;
      if (pathname.GetTitle().IsEmpty())
        return false;

      PINDEX last = pathname.GetLength()-1;
      if (userData == PSoundChannel::Recorder && pathname[last] == '*')
        pathname.Delete(last, 1);

      if (pathname.GetType() != ".wav")
        return false;

      if (userData == PSoundChannel::Recorder)
        return PFile::Access(pathname, PFile::ReadOnly);

      if (PFile::Exists(pathname))
        return PFile::Access(pathname, PFile::WriteOnly);

      return PFile::Access(pathname.GetDirectory(), PFile::WriteOnly);
    }
} PSoundChannel_WAVFile_descriptor;

PCREATE_PLUGIN(WAVFile, PSoundChannel, &PSoundChannel_WAVFile_descriptor);


#define new PNEW


///////////////////////////////////////////////////////////////////////////////

PSoundChannel_WAVFile::PSoundChannel_WAVFile()
  : m_autoRepeat(false)
  , m_sampleRate(8000)
  , m_bufferSize(2)
  , m_samplePosition(P_MAX_INDEX)
{
}


PSoundChannel_WAVFile::PSoundChannel_WAVFile(const PString & device,
                                             Directions dir,
                                             unsigned numChannels,
                                             unsigned sampleRate,
                                             unsigned bitsPerSample)
  : m_autoRepeat(false)
{
  Open(device, dir, numChannels, sampleRate, bitsPerSample);
}


PSoundChannel_WAVFile::~PSoundChannel_WAVFile()
{
  Close();
}


PString PSoundChannel_WAVFile::GetName() const
{
  return m_WAVFile.GetFilePath();
}


PStringArray PSoundChannel_WAVFile::GetDeviceNames(Directions)
{
  PStringArray devices;
  devices.AppendString("*.wav");
  return devices;
}


PBoolean PSoundChannel_WAVFile::Open(const PString & device,
                                 Directions dir,
                                 unsigned numChannels,
                                 unsigned sampleRate,
                                 unsigned bitsPerSample)
{
  Close();

  if (dir == PSoundChannel::Player) {
    SetFormat(numChannels, sampleRate, bitsPerSample);
    if (m_WAVFile.Open(device, PFile::WriteOnly))
      return true;
    SetErrorValues(m_WAVFile.GetErrorCode(), m_WAVFile.GetErrorNumber());
    return false;
  }

  PString adjustedDevice = device;
  PINDEX lastCharPos = adjustedDevice.GetLength()-1;
  if (adjustedDevice[lastCharPos] == '*') {
    adjustedDevice.Delete(lastCharPos, 1);
    m_autoRepeat = true;
  }

  if (!m_WAVFile.Open(adjustedDevice, PFile::ReadOnly)) {
    SetErrorValues(m_WAVFile.GetErrorCode(), m_WAVFile.GetErrorNumber());
    return false;
  }

  m_sampleRate = sampleRate;

  if (m_WAVFile.GetChannels() == numChannels &&
      m_sampleRate >= 8000 &&
      m_WAVFile.GetSampleSize() == bitsPerSample)
    return true;

  Close();

  SetErrorValues(BadParameter, EINVAL);
  return false;
}


PBoolean PSoundChannel_WAVFile::IsOpen() const
{ 
  return m_WAVFile.IsOpen();
}

PBoolean PSoundChannel_WAVFile::SetFormat(unsigned numChannels,
                                      unsigned sampleRate,
                                      unsigned bitsPerSample)
{
  m_WAVFile.SetChannels(numChannels);
  m_WAVFile.SetSampleRate(sampleRate);
  m_WAVFile.SetSampleSize(bitsPerSample);

  return PTrue;
}


unsigned PSoundChannel_WAVFile::GetChannels() const
{
  return m_WAVFile.GetChannels();
}


unsigned PSoundChannel_WAVFile::GetSampleRate() const
{
  return m_WAVFile.GetSampleRate();
}


unsigned PSoundChannel_WAVFile::GetSampleSize() const
{
  return m_WAVFile.GetSampleSize();
}


PBoolean PSoundChannel_WAVFile::Close()
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);

  m_WAVFile.Close();
  os_handle = -1;
  return PTrue;
}


PBoolean PSoundChannel_WAVFile::SetBuffers(PINDEX size, PINDEX /*count*/)
{
  m_bufferSize = size;
  return true;
}


PBoolean PSoundChannel_WAVFile::GetBuffers(PINDEX & size, PINDEX & count)
{
  size = m_bufferSize;
  count = 1;
  return true;
}


PBoolean PSoundChannel_WAVFile::Write(const void * data, PINDEX size)
{
  PBoolean ok = m_WAVFile.Write(data, size);
  lastWriteCount = m_WAVFile.GetLastWriteCount();
  m_Pacing.Delay(lastWriteCount*8/m_WAVFile.GetSampleSize()*1000/m_WAVFile.GetSampleRate());
  return ok;
}


PBoolean PSoundChannel_WAVFile::HasPlayCompleted()
{
  return PTrue;
}


PBoolean PSoundChannel_WAVFile::WaitForPlayCompletion()
{
  return PTrue;
}


PBoolean PSoundChannel_WAVFile::StartRecording()
{
  return PTrue;
}


PBoolean PSoundChannel_WAVFile::Read(void * data, PINDEX size)
{
  lastReadCount = 0;

  unsigned wavSampleRate = m_WAVFile.GetSampleRate();
  if (wavSampleRate < m_sampleRate) {
    // File has less samples than we want, so we need to interpolate
    unsigned iDutyCycle = m_sampleRate - wavSampleRate;
    short iSample = 0;
    short * pPCM = (short *)data;
    for (PINDEX count = 0; count < size; count += sizeof(short)) {
      iDutyCycle += wavSampleRate;
      if (iDutyCycle >= m_sampleRate) {
        iDutyCycle -= m_sampleRate;
        if (!ReadSample(iSample))
          return false;
      }
      *pPCM++ = iSample;
      lastReadCount += sizeof(short);
    }
  }
  else if (wavSampleRate > m_sampleRate) {
    // File has more samples than we want, so we need to throw some away
    unsigned iDutyCycle = 0;
    short iSample;
    short * pPCM = (short *)data;
    for (PINDEX count = 0; count < size; count += sizeof(short)) {
      do {
        if (!ReadSample(iSample))
          return false;
        iDutyCycle += m_sampleRate;
      } while (iDutyCycle < wavSampleRate);
      iDutyCycle -= wavSampleRate;
      *pPCM++ = iSample;
      lastReadCount += sizeof(short);
    }
  }
  else {
    if (!ReadSamples(data, size))
      return false;
    lastReadCount = m_WAVFile.GetLastReadCount();
  }

  m_Pacing.Delay(lastReadCount*8/m_WAVFile.GetSampleSize()*1000/m_sampleRate);
  return true;
}


bool PSoundChannel_WAVFile::ReadSample(short & sample)
{
  if (m_samplePosition >= m_sampleBuffer.GetSize()) {
    static const PINDEX BufferSize = 10000;
    if (!ReadSamples(m_sampleBuffer.GetPointer(BufferSize), BufferSize*sizeof(short)))
      return false;
    m_sampleBuffer.SetSize(m_WAVFile.GetLastReadCount()/sizeof(short));
    m_samplePosition = 0;
  }
  sample = m_sampleBuffer[m_samplePosition++];
  return true;
}


bool PSoundChannel_WAVFile::ReadSamples(void * data, PINDEX size)
{
  if (m_WAVFile.Read(data, size))
    return true;

  if (!m_autoRepeat)
    return false;

  m_WAVFile.SetPosition(0);
  return m_WAVFile.Read(data, size);
}


PBoolean PSoundChannel_WAVFile::IsRecordBufferFull()
{
  return PTrue;
}


PBoolean PSoundChannel_WAVFile::AreAllRecordBuffersFull()
{
  return PTrue;
}


PBoolean PSoundChannel_WAVFile::WaitForRecordBufferFull()
{
  return PTrue;
}


PBoolean PSoundChannel_WAVFile::WaitForAllRecordBuffersFull()
{
  return PTrue;
}


// End of File ///////////////////////////////////////////////////////////////

