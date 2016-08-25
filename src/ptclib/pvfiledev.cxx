/*
 * pvfiledev.cxx
 *
 * Video file declaration
 *
 * Portable Windows Library
 *
 * Copyright (C) 2004 Post Increment
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
 * Craig Southeren <craigs@postincrement.com>
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
#pragma implementation "pvfiledev.h"
#endif

#include <ptlib.h>

#if P_VIDEO
#if P_VIDFILE

#include <ptlib/vconvert.h>
#include <ptclib/pvfiledev.h>
#include <ptlib/pfactory.h>
#include <ptlib/pluginmgr.h>
#include <ptlib/videoio.h>


static const char DefaultYUVFileName[] = "*.yuv";


#define new PNEW


///////////////////////////////////////////////////////////////////////////////
// PVideoInputDevice_YUVFile

class PVideoInputDevice_YUVFile_PluginServiceDescriptor : public PDevicePluginServiceDescriptor
{
  public:
    typedef PFactory<PVideoFile> FileTypeFactory_T;

    virtual PObject * CreateInstance(int /*userData*/) const
    {
      return new PVideoInputDevice_YUVFile;
    }
    virtual PStringArray GetDeviceNames(int /*userData*/) const
    {
      return PVideoInputDevice_YUVFile::GetInputDeviceNames();
    }
    virtual bool ValidateDeviceName(const PString & deviceName, int /*userData*/) const
    {
      PCaselessString adjustedDevice = deviceName;

      FileTypeFactory_T::KeyList_T keyList = FileTypeFactory_T::GetKeyList();
      FileTypeFactory_T::KeyList_T::iterator r;
      for (r = keyList.begin(); r != keyList.end(); ++r) {
        PString ext = *r;
        PINDEX extLen = ext.GetLength();
        PINDEX length = adjustedDevice.GetLength();
        if (length > (2+extLen) && adjustedDevice.NumCompare(PString(".") + ext + "*", 2+extLen, length-(2+extLen)) == PObject::EqualTo)
          adjustedDevice.Delete(length-1, 1);
        else if (length < (2+extLen) || adjustedDevice.NumCompare(PString(".") + ext, 1+extLen, length-(1+extLen)) != PObject::EqualTo)
          continue;
        if (PFile::Access(adjustedDevice, PFile::ReadOnly)) 
          return true;
        PTRACE(1, "Unable to access file '" << adjustedDevice << "' for use as a video input device");
        return false;
      }
      return false;
    }
} PVideoInputDevice_YUVFile_descriptor;

PCREATE_PLUGIN(YUVFile, PVideoInputDevice, &PVideoInputDevice_YUVFile_descriptor);



PVideoInputDevice_YUVFile::PVideoInputDevice_YUVFile()
  : m_file(NULL)
  , m_frameRateAdjust(0)
  , m_opened(false)
{
  SetColourFormat("YUV420P");
}


PVideoInputDevice_YUVFile::~PVideoInputDevice_YUVFile()
{
  Close();
}


PBoolean PVideoInputDevice_YUVFile::Open(const PString & devName, PBoolean /*startImmediate*/)
{
  Close();

  PFilePath fileName;
  if (devName != DefaultYUVFileName) {
    fileName = devName;
    PINDEX lastCharPos = fileName.GetLength()-1;
    if (fileName[lastCharPos] == '*') {
      fileName.Delete(lastCharPos, 1);
      SetChannel(Channel_PlayAndRepeat);
    }
  }
  else {
    PDirectory dir;
    if (dir.Open(PFileInfo::RegularFile|PFileInfo::SymbolicLink)) {
      do {
        if (dir.GetEntryName().Right(4) == (DefaultYUVFileName+1)) {
          fileName = dir.GetEntryName();
          break;
        }
      } while (dir.Next());
    }
    if (fileName.IsEmpty()) {
      PTRACE(1, "VidFileDev\tCannot find any file using " << dir << DefaultYUVFileName << " as video input device");
      return false;
    }
  }

  m_file = PFactory<PVideoFile>::CreateInstance("yuv");
  if (m_file == NULL || !m_file->Open(fileName, PFile::ReadOnly, PFile::MustExist)) {
    PTRACE(1, "VidFileDev\tCannot open file " << fileName << " as video input device");
    return false;
  }

  *static_cast<PVideoFrameInfo *>(this) = *static_cast<PVideoFrameInfo *>(m_file);

  deviceName = m_file->GetFilePath();
  m_opened = true;
  return true;    
}


PBoolean PVideoInputDevice_YUVFile::IsOpen() 
{
  return m_opened;
}


PBoolean PVideoInputDevice_YUVFile::Close()
{
  m_opened = false;

  PBoolean ok = m_file != NULL && m_file->Close();

  PThread::Sleep(1000/frameRate);

  delete m_file;
  m_file = NULL;

  return ok;
}


PBoolean PVideoInputDevice_YUVFile::Start()
{
  return true;
}


PBoolean PVideoInputDevice_YUVFile::Stop()
{
  return true;
}


PBoolean PVideoInputDevice_YUVFile::IsCapturing()
{
  return IsOpen();
}


PStringArray PVideoInputDevice_YUVFile::GetInputDeviceNames()
{

  return PString(DefaultYUVFileName);
}


PBoolean PVideoInputDevice_YUVFile::SetVideoFormat(VideoFormat newFormat)
{
  return PVideoDevice::SetVideoFormat(newFormat);
}


int PVideoInputDevice_YUVFile::GetNumChannels() 
{
  return ChannelCount;  
}


PBoolean PVideoInputDevice_YUVFile::SetChannel(int newChannel)
{
  return PVideoDevice::SetChannel(newChannel);
}


PBoolean PVideoInputDevice_YUVFile::SetColourFormat(const PString & newFormat)
{
  return (colourFormat *= newFormat);
}


PBoolean PVideoInputDevice_YUVFile::SetFrameRate(unsigned rate)
{
  // Set file, if it will change, if not convert in GetFrameData
  if (m_file != NULL)
    m_file->SetFrameRate(rate);

  return PVideoDevice::SetFrameRate(rate);
}


PBoolean PVideoInputDevice_YUVFile::GetFrameSizeLimits(unsigned & minWidth,
                                           unsigned & minHeight,
                                           unsigned & maxWidth,
                                           unsigned & maxHeight) 
{
  if (m_file == NULL) {
    PTRACE(2, "VidFileDev\tCannot get frame size limits, no file opened.");
    return false;
  }

  unsigned width, height;
  if (!m_file->GetFrameSize(width, height))
    return false;

  minWidth  = maxWidth  = width;
  minHeight = maxHeight = height;
  return true;
}


PBoolean PVideoInputDevice_YUVFile::SetFrameSize(unsigned width, unsigned height)
{
  if (m_file == NULL) {
    PTRACE(2, "VidFileDev\tCannot set frame size, no file opened.");
    return false;
  }

  return m_file->SetFrameSize(width, height) && PVideoDevice::SetFrameSize(width, height);
}


PINDEX PVideoInputDevice_YUVFile::GetMaxFrameBytes()
{
  return GetMaxFrameBytesConverted(m_file->GetFrameBytes());
}


PBoolean PVideoInputDevice_YUVFile::GetFrameData(BYTE * buffer, PINDEX * bytesReturned)
{
  m_pacing.Delay(1000/frameRate);

  if (!m_opened || PAssertNULL(m_file) == NULL) {
    PTRACE(5, "VidFileDev\tAbort GetFrameData, closed.");
    return false;
  }

  off_t frameNumber = m_file->GetPosition();

  unsigned fileRate = m_file->GetFrameRate();
  if (fileRate > frameRate) {
    m_frameRateAdjust += fileRate;
    while (m_frameRateAdjust > frameRate) {
      m_frameRateAdjust -= frameRate;
      ++frameNumber;
    }
    --frameNumber;
  }
  else if (fileRate < frameRate) {
    if (m_frameRateAdjust < frameRate)
      m_frameRateAdjust += fileRate;
    else {
      m_frameRateAdjust -= frameRate;
      --frameNumber;
    }
  }

  PTRACE(6, "VidFileDev\tPlaying frame number " << frameNumber);
  m_file->SetPosition(frameNumber);

  return GetFrameDataNoDelay(buffer, bytesReturned);
}


PBoolean PVideoInputDevice_YUVFile::GetFrameDataNoDelay(BYTE * frame, PINDEX * bytesReturned)
{
  if (!m_opened || PAssertNULL(m_file) == NULL) {
    PTRACE(5, "VidFileDev\tAbort GetFrameDataNoDelay, closed.");
    return false;
  }

  BYTE * readBuffer = converter != NULL ? frameStore.GetPointer(m_file->GetFrameBytes()) : frame;

  if (m_file->IsOpen()) {
    if (!m_file->ReadFrame(readBuffer))
      m_file->Close();
  }

  if (!m_file->IsOpen()) {
    switch (channelNumber) {
      case Channel_PlayAndClose:
      default:
        PTRACE(4, "VidFileDev\tCompleted play and close of " << m_file->GetFilePath());
        return false;

      case Channel_PlayAndRepeat:
        m_file->Open(deviceName, PFile::ReadOnly, PFile::MustExist);
        if (!m_file->SetPosition(0)) {
          PTRACE(2, "VidFileDev\tCould not rewind " << m_file->GetFilePath());
          return false;
        }
        if (!m_file->ReadFrame(readBuffer))
          return false;
        break;

      case Channel_PlayAndKeepLast:
        PTRACE(4, "VidFileDev\tCompleted play and keep last of " << m_file->GetFilePath());
        break;

      case Channel_PlayAndShowBlack:
        PTRACE(4, "VidFileDev\tCompleted play and show black of " << m_file->GetFilePath());
        PColourConverter::FillYUV420P(0, 0,
                                      frameWidth, frameHeight,
                                      frameWidth, frameHeight,
                                      readBuffer,
                                      100, 100, 100);
        break;
    }
  }

  if (converter == NULL) {
    if (bytesReturned != NULL)
      *bytesReturned = m_file->GetFrameBytes();
  }
  else {
    converter->SetSrcFrameSize(frameWidth, frameHeight);
    if (!converter->Convert(readBuffer, frame, bytesReturned)) {
      PTRACE(2, "VidFileDev\tConversion failed with " << *converter);
      return false;
    }

    if (bytesReturned != NULL)
      *bytesReturned = converter->GetMaxDstFrameBytes();
  }

  return true;
}


///////////////////////////////////////////////////////////////////////////////
// PVideoOutputDevice_YUVFile

class PVideoOutputDevice_YUVFile_PluginServiceDescriptor : public PDevicePluginServiceDescriptor
{
  public:
    virtual PObject * CreateInstance(int /*userData*/) const
    {
        return new PVideoOutputDevice_YUVFile;
    }
    virtual PStringArray GetDeviceNames(int /*userData*/) const
    {
        return PVideoOutputDevice_YUVFile::GetOutputDeviceNames();
    }
    virtual bool ValidateDeviceName(const PString & deviceName, int /*userData*/) const
    {
      return (deviceName.Right(4) *= ".yuv") && (!PFile::Exists(deviceName) || PFile::Access(deviceName, PFile::WriteOnly));
    }
} PVideoOutputDevice_YUVFile_descriptor;

PCREATE_PLUGIN(YUVFile, PVideoOutputDevice, &PVideoOutputDevice_YUVFile_descriptor);


PVideoOutputDevice_YUVFile::PVideoOutputDevice_YUVFile()
  : m_file(NULL)
  , m_opened(false)
{
}


PVideoOutputDevice_YUVFile::~PVideoOutputDevice_YUVFile()
{
  Close();
}


PBoolean PVideoOutputDevice_YUVFile::Open(const PString & devName, PBoolean /*startImmediate*/)
{
  PFilePath fileName;
  if (devName != DefaultYUVFileName)
    fileName = devName;
  else {
    unsigned unique = 0;
    do {
      fileName.Empty();
      fileName.sprintf("video%03u.yuv", ++unique);
    } while (PFile::Exists(fileName));
  }

  m_file = PFactory<PVideoFile>::CreateInstance("yuv");
  if (m_file == NULL || !m_file->Open(fileName, PFile::WriteOnly, PFile::Create|PFile::Truncate)) {
    PTRACE(1, "YUVFile\tCannot create file " << fileName << " as video output device");
    return false;
  }

  deviceName = m_file->GetFilePath();
  m_opened = true;
  return true;
}

PBoolean PVideoOutputDevice_YUVFile::Close()
{
  m_opened = false;

  PBoolean ok = m_file == NULL || m_file->Close();

  PThread::Sleep(10);

  delete m_file;
  m_file = NULL;

  return ok;
}

PBoolean PVideoOutputDevice_YUVFile::Start()
{
  return m_file != NULL && m_file->SetFrameSize(frameHeight, frameWidth);
}

PBoolean PVideoOutputDevice_YUVFile::Stop()
{
  return true;
}

PBoolean PVideoOutputDevice_YUVFile::IsOpen()
{
  return m_opened;
}


PStringArray PVideoOutputDevice_YUVFile::GetOutputDeviceNames()
{
  return PString(DefaultYUVFileName);
}


PBoolean PVideoOutputDevice_YUVFile::SetColourFormat(const PString & newFormat)
{
  return (newFormat *= "YUV420P") && PVideoDevice::SetColourFormat(newFormat);
}


PINDEX PVideoOutputDevice_YUVFile::GetMaxFrameBytes()
{
  return GetMaxFrameBytesConverted(CalculateFrameBytes(frameWidth, frameHeight, colourFormat));
}


PBoolean PVideoOutputDevice_YUVFile::SetFrameData(unsigned x, unsigned y,
                                              unsigned width, unsigned height,
                                              const BYTE * data,
                                              PBoolean /*endFrame*/)
{
  if (!m_opened || PAssertNULL(m_file) == NULL) {
    PTRACE(5, "VidFileDev\tAbort SetFrameData, closed.");
    return false;
  }

  if (x != 0 || y != 0 || width != frameWidth || height != frameHeight) {
    PTRACE(1, "YUVFile\tOutput device only supports full frame writes");
    return false;
  }

  if (!m_file->SetFrameSize(width, height))
    return false;

  if (converter == NULL)
    return m_file->WriteFrame(data);

  converter->Convert(data, frameStore.GetPointer(GetMaxFrameBytes()));
  return m_file->WriteFrame(frameStore);
}


#endif // P_VIDFILE
#endif

