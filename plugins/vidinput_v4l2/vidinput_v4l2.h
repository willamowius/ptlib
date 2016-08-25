/*
 * vidinput_v4l2.h
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
 * The Original Code is Portable Windows Library (V4L plugin).
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Derek Smithies (derek@indranet.co.nz)
 *                 Mark Cooke (mpc@star.sr.bham.ac.uk)
 *                 Nicola Orru' <nigu@itadinanta.it>
 *
 * $Revision$
 * $Author$
 * $Date$
 */
#ifndef _PVIDEOIOV4L2
#define _PVIDEOIOV4L2


#include <sys/mman.h>
#include <sys/time.h>

#include <ptlib.h>
#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptclib/delaychan.h>

#include V4L2_HEADER

#ifndef V4L2_PIX_FMT_SBGGR8
#define V4L2_PIX_FMT_SBGGR8  v4l2_fourcc('B','A','8','1') /*  8  BGBG.. GRGR.. */
#endif

class PVideoInputDevice_V4L2: public PVideoInputDevice
{

  PCLASSINFO(PVideoInputDevice_V4L2, PVideoInputDevice);
private:
  PVideoInputDevice_V4L2(const PVideoInputDevice_V4L2& ){};
  PVideoInputDevice_V4L2& operator=(const PVideoInputDevice_V4L2& ){ return *this; };
public:
  PVideoInputDevice_V4L2();
  virtual ~PVideoInputDevice_V4L2();
  
  void ReadDeviceDirectory (PDirectory, POrdinalToString &);

  static PStringList GetInputDeviceNames();

  PStringArray GetDeviceNames() const
  { return GetInputDeviceNames(); }

  PBoolean Open(const PString &deviceName, PBoolean startImmediate);

  PBoolean IsOpen();

  PBoolean Close();

  PBoolean Start();
  PBoolean Stop();

  PBoolean IsCapturing();

  PINDEX GetMaxFrameBytes();

  PBoolean GetFrameData(BYTE*, PINDEX*);
  PBoolean GetFrameDataNoDelay(BYTE*, PINDEX*);

  PBoolean GetFrameSizeLimits(unsigned int&, unsigned int&,
			  unsigned int&, unsigned int&);

  PBoolean TestAllFormats();

  PBoolean SetFrameSize(unsigned int, unsigned int);
  PBoolean SetNearestFrameSize(unsigned int, unsigned int);
  PBoolean SetFrameRate(unsigned int);

  PBoolean GetParameters(int*, int*, int*, int*, int*);

  PBoolean SetColourFormat(const PString&);

  int GetControlCommon(unsigned int control, int *value);
  PBoolean SetControlCommon(unsigned int control, int newValue);

  int GetContrast();
  PBoolean SetContrast(unsigned int);
  int GetBrightness();
  PBoolean SetBrightness(unsigned int);
  int GetWhiteness();
  PBoolean SetWhiteness(unsigned int);
  int GetColour();
  PBoolean SetColour(unsigned int);
  int GetHue();
  PBoolean SetHue(unsigned int);

  PBoolean SetVideoChannelFormat(int, PVideoDevice::VideoFormat);
  PBoolean SetVideoFormat(PVideoDevice::VideoFormat);
  int GetNumChannels();
  PBoolean SetChannel(int);

  PBoolean NormalReadProcess(BYTE*, PINDEX*);

private:
  void ClearMapping();

  PBoolean SetMapping();

  PBoolean VerifyHardwareFrameSize(unsigned int & width, unsigned int & height);

  PBoolean QueueBuffers();

  PBoolean StartStreaming();
  void StopStreaming();

  struct v4l2_capability videoCapability;
  struct v4l2_streamparm videoStreamParm;
  PBoolean   canRead;
  PBoolean   canStream;
  PBoolean   canSelect;
  PBoolean   canSetFrameRate;
  PBoolean   isMapped;
#define NUM_VIDBUF 4
  BYTE * videoBuffer[NUM_VIDBUF];
  uint   videoBufferCount;
  uint   currentvideoBuffer;

  PMutex mmapMutex;                             /** Has MMAP frame buffers in use? */
  PBoolean isOpen;				/** Has the Video Input Device successfully been openend? */
  PBoolean areBuffersQueued;
  PBoolean isStreaming;

  int    videoFd;
  int    frameBytes;
  PBoolean   started;
  PAdaptiveDelay m_pacing;
};

#endif
