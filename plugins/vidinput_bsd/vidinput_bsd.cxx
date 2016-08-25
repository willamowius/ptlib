/*
 * video4bsd.cxx
 *
 * Classes to support streaming video input (grabbing) and output.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2000 Equivalence Pty. Ltd.
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
 * Contributor(s): Roger Hardiman <roger@freebsd.org>
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#pragma implementation "vidinput_bsd.h"

#include "vidinput_bsd.h"

PCREATE_VIDINPUT_PLUGIN(BSDCAPTURE);

///////////////////////////////////////////////////////////////////////////////
// PVideoInputBSDCAPTURE

PVideoInputDevice_BSDCAPTURE::PVideoInputDevice_BSDCAPTURE()
{
  videoFd     = -1;
  canMap      = -1;
}

PVideoInputDevice_BSDCAPTURE::~PVideoInputDevice_BSDCAPTURE()
{
  Close();
}

PBoolean PVideoInputDevice_BSDCAPTURE::Open(const PString & devName, PBoolean startImmediate)
{
  if (IsOpen())
  Close();

  deviceName = devName;
  videoFd = ::open((const char *)devName, O_RDONLY);
  if (videoFd < 0) {
    videoFd = -1;
    return PFalse;
  }
 
  // fill in a device capabilities structure
  videoCapability.minheight = 32;
  videoCapability.minwidth  = 32;
  videoCapability.maxheight = 768;
  videoCapability.maxwidth  = 576;
  videoCapability.channels  = 5;

  // set height and width
  frameHeight = videoCapability.maxheight;
  frameWidth  = videoCapability.maxwidth;
  
  // select the specified input
  if (!SetChannel(channelNumber)) {
    ::close (videoFd);
    videoFd = -1;
    return PFalse;
  } 
  
  // select the video format (eg PAL, NTSC)
  if (!SetVideoFormat(videoFormat)) {
    ::close (videoFd);
    videoFd = -1;
    return PFalse;
  }
 
  // select the colpur format (eg YUV420, or RGB)
  if (!SetColourFormat(colourFormat)) {
    ::close (videoFd);
    videoFd = -1;
    return PFalse;
  }

  // select the image size
  if (!SetFrameSize(frameWidth, frameHeight)) {
    ::close (videoFd);
    videoFd = -1;
    return PFalse;
  }

  return PTrue;    
}


PBoolean PVideoInputDevice_BSDCAPTURE::IsOpen() 
{
    return videoFd >= 0;
}


PBoolean PVideoInputDevice_BSDCAPTURE::Close()
{
  if (!IsOpen())
    return PFalse;

  ClearMapping();
  ::close(videoFd);
  videoFd = -1;
  canMap  = -1;
  
  return PTrue;
}

PBoolean PVideoInputDevice_BSDCAPTURE::Start()
{
  return PTrue;
}


PBoolean PVideoInputDevice_BSDCAPTURE::Stop()
{
  return PTrue;
}


PBoolean PVideoInputDevice_BSDCAPTURE::IsCapturing()
{
  return IsOpen();
}


PStringList PVideoInputDevice_BSDCAPTURE::GetInputDeviceNames()
{
  PStringList list;

  if (PFile::Exists("/dev/bktr0"))
    list.AppendString("/dev/bktr0");
  if (PFile::Exists("/dev/bktr1"))
    list.AppendString("/dev/bktr1");
  if (PFile::Exists("/dev/meteor0"))
    list.AppendString("/dev/meteor0");
  if (PFile::Exists("/dev/meteor1"))
    list.AppendString("/dev/meteor1");

  return list;
}


PBoolean PVideoInputDevice_BSDCAPTURE::SetVideoFormat(VideoFormat newFormat)
{
  if (!PVideoDevice::SetVideoFormat(newFormat))
    return PFalse;

  // set channel information
  static int fmt[4] = { METEOR_FMT_PAL, METEOR_FMT_NTSC,
                        METEOR_FMT_SECAM, METEOR_FMT_AUTOMODE };
  int format = fmt[newFormat];

  // set the information
  if (::ioctl(videoFd, METEORSFMT, &format) >= 0)
    return PTrue;

  // setting the format failed. Fall back trying other standard formats

  if (newFormat != Auto)
    return PFalse;

  if (SetVideoFormat(PAL))
    return PTrue;
  if (SetVideoFormat(NTSC))
    return PTrue;
  if (SetVideoFormat(SECAM))
    return PTrue;

  return PFalse;  
}


int PVideoInputDevice_BSDCAPTURE::GetNumChannels() 
{
  return videoCapability.channels;
}


PBoolean PVideoInputDevice_BSDCAPTURE::SetChannel(int newChannel)
{
  if (!PVideoDevice::SetChannel(newChannel))
    return PFalse;

  // set channel information
  static int chnl[5] = { METEOR_INPUT_DEV0, METEOR_INPUT_DEV1,
                         METEOR_INPUT_DEV2, METEOR_INPUT_DEV3,
                         METEOR_INPUT_DEV_SVIDEO };
  int channel = chnl[newChannel];

  // set the information
  if (::ioctl(videoFd, METEORSINPUT, &channel) < 0)
    return PFalse;

  return PTrue;
}


PBoolean PVideoInputDevice_BSDCAPTURE::SetColourFormat(const PString & newFormat)
{
  if (!PVideoDevice::SetColourFormat(newFormat))
    return PFalse;

  ClearMapping();

  frameBytes = CalculateFrameBytes(frameWidth, frameHeight, colourFormat);

  return PTrue;

}


PBoolean PVideoInputDevice_BSDCAPTURE::SetFrameRate(unsigned rate)
{
  if (!PVideoDevice::SetFrameRate(rate))
    return PFalse;

  return PTrue;
}


PBoolean PVideoInputDevice_BSDCAPTURE::GetFrameSizeLimits(unsigned & minWidth,
                                           unsigned & minHeight,
                                           unsigned & maxWidth,
                                           unsigned & maxHeight) 
{
  if (!IsOpen())
    return PFalse;

  minWidth  = videoCapability.minwidth;
  minHeight = videoCapability.minheight;
  maxWidth  = videoCapability.maxwidth;
  maxHeight = videoCapability.maxheight;
  return PTrue;

}


PBoolean PVideoInputDevice_BSDCAPTURE::SetFrameSize(unsigned width, unsigned height)
{
  if (!PVideoDevice::SetFrameSize(width, height))
    return PFalse;
  
  ClearMapping();

  frameBytes = CalculateFrameBytes(frameWidth, frameHeight, colourFormat);
  
  return PTrue;
}


PINDEX PVideoInputDevice_BSDCAPTURE::GetMaxFrameBytes()
{
  return GetMaxFrameBytesConverted(frameBytes);
}


PBoolean PVideoInputDevice_BSDCAPTURE::GetFrameData(BYTE * buffer, PINDEX * bytesReturned)
{
  m_pacing.Delay(1000/GetFrameRate());
  return GetFrameDataNoDelay(buffer,bytesReturned);
}


PBoolean PVideoInputDevice_BSDCAPTURE::GetFrameDataNoDelay(BYTE * buffer, PINDEX * bytesReturned)
{

  // Hack time. It seems that the Start() and Stop() functions are not
  // actually called, so we will have to initialise the frame grabber
  // here on the first pass through this GetFrameData() function

  if (canMap < 0) {

    struct meteor_geomet geo;
    geo.rows = frameHeight;
    geo.columns = frameWidth;
    geo.frames = 1;
    geo.oformat = METEOR_GEO_YUV_422 | METEOR_GEO_YUV_12;

    // Grab even field (instead of interlaced frames) where possible to stop
    // jagged interlacing artifacts. NTSC is 640x480, PAL/SECAM is 768x576.
    if (  ((PVideoDevice::GetVideoFormat() == PAL) && (frameHeight <= 288))
       || ((PVideoDevice::GetVideoFormat() == SECAM) && (frameHeight <= 288))
       || ((PVideoDevice::GetVideoFormat() == NTSC) && (frameHeight <= 240)) ){
        geo.oformat |=  METEOR_GEO_EVEN_ONLY;
    }

    // set the new geometry
    if (ioctl(videoFd, METEORSETGEO, &geo) < 0) {
      return PFalse;
    }

    mmap_size = frameBytes;
    videoBuffer = (BYTE *)::mmap(0, mmap_size, PROT_READ, 0, videoFd, 0);
    if (videoBuffer < 0) {
      return PFalse;
    } else {
      canMap = 1;
    }
 
    // put the grabber into continuous capture mode
    int mode =  METEOR_CAP_CONTINOUS;
    if (ioctl(videoFd, METEORCAPTUR, &mode) < 0 ) {
      return PFalse;
    }
  }


  // Copy a snapshot of the image from the mmap buffer
  // Really there should be some synchronisation here to avoid tearing
  // in the image, but we will worry about that later

  if (converter != NULL)
    return converter->Convert(videoBuffer, buffer, bytesReturned);

  memcpy(buffer, videoBuffer, frameBytes);

  if (bytesReturned != NULL)
    *bytesReturned = frameBytes;

  
  return PTrue;
}


void PVideoInputDevice_BSDCAPTURE::ClearMapping()
{
  if (canMap == 1) {

    // better stop grabbing first
    // Really this should be in the Stop() function, but that is
    // not actually called anywhere.

    int mode =  METEOR_CAP_STOP_CONT;
    ioctl(videoFd, METEORCAPTUR, &mode);

    if (videoBuffer != NULL)
      ::munmap(videoBuffer, mmap_size);

    canMap = -1;
    videoBuffer = NULL;
  }
}

PBoolean PVideoInputDevice_BSDCAPTURE::VerifyHardwareFrameSize(unsigned width,
                                                unsigned height)
{
	// Assume the size is valid
	return PTrue;
}


int PVideoInputDevice_BSDCAPTURE::GetBrightness()
{
  if (!IsOpen())
    return -1;

  unsigned char data;
  if (::ioctl(videoFd, METEORGBRIG, &data) < 0)
    return -1;
  frameBrightness = (data << 8);

  return frameBrightness;
}

int PVideoInputDevice_BSDCAPTURE::GetContrast()
{
  if (!IsOpen())
    return -1;

  unsigned char data;
  if (::ioctl(videoFd, METEORGCONT, &data) < 0)
    return -1;
  frameContrast = (data << 8);

 return frameContrast;
}

int PVideoInputDevice_BSDCAPTURE::GetHue()
{
  if (!IsOpen())
    return -1;

  char data;
  if (::ioctl(videoFd, METEORGHUE, &data) < 0)
    return -1;
  frameHue = ((data + 128) << 8);

  return frameHue;
}

PBoolean PVideoInputDevice_BSDCAPTURE::SetBrightness(unsigned newBrightness)
{
  if (!IsOpen())
    return PFalse;

  unsigned char data = (newBrightness >> 8); // rescale for the ioctl
  if (::ioctl(videoFd, METEORSBRIG, &data) < 0)
    return PFalse;

  frameBrightness=newBrightness;
  return PTrue;
}

PBoolean PVideoInputDevice_BSDCAPTURE::SetContrast(unsigned newContrast)
{
  if (!IsOpen())
    return PFalse;

  unsigned char data = (newContrast >> 8); // rescale for the ioctl
  if (::ioctl(videoFd, METEORSCONT, &data) < 0)
    return PFalse;

  frameContrast = newContrast;
  return PTrue;
}

PBoolean PVideoInputDevice_BSDCAPTURE::SetHue(unsigned newHue)
{
  if (!IsOpen())
    return PFalse;

  char data = (newHue >> 8) - 128; // ioctl takes a signed char
  if (::ioctl(videoFd, METEORSHUE, &data) < 0)
    return PFalse;

  frameHue=newHue;
  return PTrue;
}

PBoolean PVideoInputDevice_BSDCAPTURE::GetParameters (int *whiteness, int *brightness,
                                      int *colour, int *contrast, int *hue)
{
  if (!IsOpen())
    return PFalse;

  unsigned char data;
  char signed_data;

  if (::ioctl(videoFd, METEORGBRIG, &data) < 0)
    return -1;
  *brightness = (data << 8);

  if (::ioctl(videoFd, METEORGCONT, &data) < 0)
    return -1;
  *contrast = (data << 8);

  if (::ioctl(videoFd, METEORGHUE, &signed_data) < 0)
    return -1;
  *hue = ((data + 128) << 8);

  // The bktr driver does not have colour or whiteness ioctls
  // so set them to the current global values
  *colour     = frameColour;
  *whiteness  = frameWhiteness;

  // update the global settings
  frameBrightness = *brightness;
  frameContrast   = *contrast;
  frameHue        = *hue;

  return PTrue;
}

PBoolean PVideoInputDevice_BSDCAPTURE::TestAllFormats()
{
  return PTrue;
}
// End Of File ///////////////////////////////////////////////////////////////
