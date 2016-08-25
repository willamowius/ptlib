/*
 * shmvideo.cxx
 *
 * This file contains the class hierarchy for both shared memory video
 * input and output devices.
 *
 * Copyright (c) 2003 Pai-Hsiang Hsiao
 * Copyright (c) 1998-2003 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#define P_FORCE_STATIC_PLUGIN

#include <ptlib.h>
#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptlib/unix/ptlib/shmvideo.h>

#if P_SHM_VIDEO

#ifdef P_MACOSX
namespace PWLibStupidLinkerHacks {
	int loadShmVideoStuff;
};

#endif

class PColourConverter;

static const char *
ShmKeyFileName()
{
  return "/dev/null";
}

PBoolean
PVideoOutputDevice_Shm::shmInit()
{
  semLock = sem_open(SEM_NAME_OF_OUTPUT_DEVICE,
		     O_RDWR, S_IRUSR|S_IWUSR, 0);

  if (semLock != (sem_t *)SEM_FAILED) {
    shmKey = ftok(ShmKeyFileName(), 0);
    if (shmKey >= 0) {
      shmId = shmget(shmKey, SHMVIDEO_BUFSIZE, 0666);
      if (shmId >= 0) {
        shmPtr = shmat(shmId, NULL, 0);
        if (shmPtr) {
          return PTrue;
        }
        else {
          PTRACE(1, "SHMV\t shmInit can not attach shared memory" << endl);
          shmctl(shmId, IPC_RMID, NULL);
          sem_close(semLock);
        }
      }
      else {
        PTRACE(1, "SHMV\t shmInit can not find the shared memory" << endl);
        sem_close(semLock);
      }
    }
    else {
      PTRACE(1, "SHMV\t shmInit can not create key for shared memory" << endl);
      sem_close(semLock);
    }
  }
  else {
    PTRACE(1, "SHMV\t shmInit can not create semaphore" << endl);
  }

  semLock = (sem_t *)SEM_FAILED;
  shmKey = -1;
  shmId = -1;
  shmPtr = NULL;

  return PFalse;
}

PVideoOutputDevice_Shm::PVideoOutputDevice_Shm()
{
	colourFormat = "RGB24";
	bytesPerPixel = 3;
	frameStore.SetSize(frameWidth * frameHeight * bytesPerPixel);
	
  semLock = (sem_t *)SEM_FAILED;
  shmKey = -1;
  shmId = -1;
  shmPtr = NULL;

  PTRACE(6, "SHMV\t Constructor of PVideoOutputDevice_Shm");
}

PBoolean PVideoOutputDevice_Shm::SetColourFormat(const PString & colourFormat)
{
	if( colourFormat == "RGB32")
		bytesPerPixel = 4;
	else if(colourFormat == "RGB24")
		bytesPerPixel = 3;
	else
		return false;
	
	return PVideoOutputDevice::SetColourFormat(colourFormat) && SetFrameSize(frameWidth, frameHeight);
}

PBoolean PVideoOutputDevice_Shm::SetFrameSize(unsigned width, unsigned height)
{
	if (!PVideoOutputDevice::SetFrameSize(width, height))
		return PFalse;
	
	return frameStore.SetSize(frameWidth*frameHeight*bytesPerPixel);
}

PINDEX PVideoOutputDevice_Shm::GetMaxFrameBytes()
{
	return frameStore.GetSize();
}

PBoolean PVideoOutputDevice_Shm::SetFrameData(unsigned x, unsigned y,
                                         unsigned width, unsigned height,
                                         const BYTE * data,
                                         PBoolean endFrame)
{
	if (x+width > frameWidth || y+height > frameHeight)
		return PFalse;
	
	if (x == 0 && width == frameWidth && y == 0 && height == frameHeight) {
		if (converter != NULL)
			converter->Convert(data, frameStore.GetPointer());
		else
			memcpy(frameStore.GetPointer(), data, height*width*bytesPerPixel);
	}
	else {
		if (converter != NULL) {
			PAssertAlways("Converted output of partial RGB frame not supported");
			return PFalse;
		}
		
		if (x == 0 && width == frameWidth)
			memcpy(frameStore.GetPointer() + y*width*bytesPerPixel, data, height*width*bytesPerPixel);
		else {
			for (unsigned dy = 0; dy < height; dy++)
				memcpy(frameStore.GetPointer() + ((y+dy)*width + x)*bytesPerPixel,
					   data + dy*width*bytesPerPixel, width*bytesPerPixel);
		}
	}
	
	if (endFrame)
		return EndFrame();
	
	return PTrue;
}

PBoolean
PVideoOutputDevice_Shm::Open(const PString & name,
			   PBoolean /*startImmediate*/)
{
  PTRACE(1, "SHMV\t Open of PVideoOutputDevice_Shm");

  Close();

  if (shmInit() == PTrue) {
    deviceName = name;
    return PTrue;
  }
  else {
    return PFalse;
  }
}

PBoolean
PVideoOutputDevice_Shm::IsOpen()
{
  if (semLock != (sem_t *)SEM_FAILED) {
    return PTrue;
  }
  else {
    return PFalse;
  }
}

PBoolean
PVideoOutputDevice_Shm::Close()
{
  if (semLock != (sem_t *)SEM_FAILED) {
    shmdt(shmPtr);
    sem_close(semLock);
    shmPtr = NULL;
  }
  return PTrue;
}

PStringArray
PVideoOutputDevice_Shm::GetDeviceNames() const
{
  return PString("shm");
}

PBoolean
PVideoOutputDevice_Shm::EndFrame()
{
  long *ptr = (long *)shmPtr;

  if (semLock == (sem_t *)SEM_FAILED) {
    return PFalse;
  }

  if (bytesPerPixel != 3 && bytesPerPixel != 4) {
    PTRACE(1, "SHMV\t EndFrame() does not handle bytesPerPixel!={3,4}"<<endl);
    return PFalse;
  }

  if (frameWidth*frameHeight*bytesPerPixel > SHMVIDEO_FRAMESIZE) {
    return PFalse;
  }

  // write header info so the consumer knows what to expect
  ptr[0] = frameWidth;
  ptr[1] = frameHeight;
  ptr[2] = bytesPerPixel;

  PTRACE(1, "writing " << frameStore.GetSize() << " bytes" << endl);
  if (memcpy((char *)shmPtr+sizeof(long)*3,
             frameStore, frameStore.GetSize()) == NULL) {
    return PFalse;
  }

  sem_post(semLock);

  return PTrue;
}

///////////////////////////////////////////////////////////////////////////////

PCREATE_VIDINPUT_PLUGIN(Shm);

PBoolean
PVideoInputDevice_Shm::shmInit()
{
  semLock = sem_open(SEM_NAME_OF_INPUT_DEVICE,
		     O_RDWR, S_IRUSR|S_IWUSR, 0);

  if (semLock != (sem_t *)SEM_FAILED) {
    shmKey = ftok(ShmKeyFileName(), 100);
    if (shmKey >= 0) {
      shmId = shmget(shmKey, SHMVIDEO_BUFSIZE, 0666);
      if (shmId >= 0) {
        shmPtr = shmat(shmId, NULL, 0);
        if (shmPtr) {
          return PTrue;
        }
        else {
          PTRACE(1, "SHMV\t shmInit can not attach shared memory" << endl);
          shmctl(shmId, IPC_RMID, NULL);
          sem_close(semLock);
        }
      }
      else {
        PTRACE(1, "SHMV\t shmInit can not find the shared memory" << endl);
        sem_close(semLock);
      }
    }
    else {
      PTRACE(1, "SHMV\t shmInit can not create key for shared memory" << endl);
      sem_close(semLock);
    }
  }
  else {
    PTRACE(1, "SHMV\t shmInit can not create semaphore" << endl);
  }

  semLock = (sem_t *)SEM_FAILED;
  shmKey = -1;
  shmId = -1;
  shmPtr = NULL;

  return PFalse;
}

PVideoInputDevice_Shm::PVideoInputDevice_Shm()
{
  semLock = (sem_t *)SEM_FAILED;
  shmKey = -1;
  shmId = -1;
  shmPtr = NULL;

  PTRACE(4, "SHMV\t Constructor of PVideoInputDevice_Shm");
}

PBoolean
PVideoInputDevice_Shm::Open(const PString & name,
			  PBoolean /*startImmediate*/)
{
  PTRACE(1, "SHMV\t Open of PVideoInputDevice_Shm");

  Close();

  if (shmInit() == PTrue) {
    deviceName = name;
    return PTrue;
  }
  else {
    return PFalse;
  }
}

PBoolean
PVideoInputDevice_Shm::IsOpen()
{
  if (semLock != (sem_t *)SEM_FAILED) {
    return PTrue;
  }
  else {
    return PFalse;
  }
}

PBoolean
PVideoInputDevice_Shm::Close()
{
  if (semLock != (sem_t *)SEM_FAILED) {
    shmdt(shmPtr);
    sem_close(semLock);
    shmPtr = NULL;
  }
  return PTrue;
}

PBoolean PVideoInputDevice_Shm::IsCapturing()
{
	return PTrue;
}

PINDEX PVideoInputDevice_Shm::GetMaxFrameBytes()
{
	return videoFrameSize;
}

PStringArray
PVideoInputDevice_Shm::GetInputDeviceNames()
{
  return PString("shm");
}

PBoolean
PVideoInputDevice_Shm::GetFrameSizeLimits(unsigned & minWidth,
					unsigned & minHeight,
					unsigned & maxWidth,
					unsigned & maxHeight) 
{
  minWidth  = 176;
  minHeight = 144;
  maxWidth  = 352;
  maxHeight =  288;

  return PTrue;
}

static void RGBtoYUV420PSameSize (const BYTE *, BYTE *, unsigned, PBoolean, 
                                  int, int);


#define rgbtoyuv(r, g, b, y, u, v) \
  y=(BYTE)(((int)30*r  +(int)59*g +(int)11*b)/100); \
  u=(BYTE)(((int)-17*r  -(int)33*g +(int)50*b+12800)/100); \
  v=(BYTE)(((int)50*r  -(int)42*g -(int)8*b+12800)/100); \



static void RGBtoYUV420PSameSize (const BYTE * rgb,
                                  BYTE * yuv,
                                  unsigned rgbIncrement,
                                  PBoolean flip, 
                                  int srcFrameWidth, int srcFrameHeight) 
{
  const unsigned planeSize = srcFrameWidth*srcFrameHeight;
  const unsigned halfWidth = srcFrameWidth >> 1;
  
  // get pointers to the data
  BYTE * yplane  = yuv;
  BYTE * uplane  = yuv + planeSize;
  BYTE * vplane  = yuv + planeSize + (planeSize >> 2);
  const BYTE * rgbIndex = rgb;

  for (int y = 0; y < (int) srcFrameHeight; y++) {
    BYTE * yline  = yplane + (y * srcFrameWidth);
    BYTE * uline  = uplane + ((y >> 1) * halfWidth);
    BYTE * vline  = vplane + ((y >> 1) * halfWidth);

    if (flip)
      rgbIndex = rgb + (srcFrameWidth*(srcFrameHeight-1-y)*rgbIncrement);

    for (int x = 0; x < (int) srcFrameWidth; x+=2) {
      rgbtoyuv(rgbIndex[0], rgbIndex[1], rgbIndex[2],*yline, *uline, *vline);
      rgbIndex += rgbIncrement;
      yline++;
      rgbtoyuv(rgbIndex[0], rgbIndex[1], rgbIndex[2],*yline, *uline, *vline);
      rgbIndex += rgbIncrement;
      yline++;
      uline++;
      vline++;
    }
  }
}


PBoolean PVideoInputDevice_Shm::GetFrame(PBYTEArray & frame)
{
	PINDEX returned;
	if (!GetFrameData(frame.GetPointer(GetMaxFrameBytes()), &returned))
		return PFalse;
	
	frame.SetSize(returned);
	return PTrue;
}

PBoolean
PVideoInputDevice_Shm::GetFrameData(BYTE * buffer, PINDEX * bytesReturned)
{    
  m_pacing.Delay(1000/GetFrameRate());

  return GetFrameDataNoDelay(buffer, bytesReturned);
}

PBoolean
PVideoInputDevice_Shm::GetFrameDataNoDelay (BYTE *buffer, PINDEX *bytesReturned)
{
  long *bufPtr = (long *)shmPtr;

  unsigned width = 0;
  unsigned height = 0;
  unsigned rgbIncrement = 4;

  GetFrameSize (width, height);

  bufPtr[0] = width;
  bufPtr[1] = height;

  if (semLock != (sem_t *)SEM_FAILED && sem_trywait(semLock) == 0) {
    if (bufPtr[0] == (long)width && bufPtr[1] == (long)height) {
      rgbIncrement = bufPtr[2];
      RGBtoYUV420PSameSize ((BYTE *)(bufPtr+3), buffer, rgbIncrement, PFalse, 
			    width, height);
	  
	  *bytesReturned = videoFrameSize;
      return PTrue;
    }
  }

  return PFalse;
}

PBoolean PVideoInputDevice_Shm::TestAllFormats()
{
	return PTrue;
}

PBoolean PVideoInputDevice_Shm::Start()
{
	return PTrue;
}

PBoolean PVideoInputDevice_Shm::Stop()
{
	return PTrue;
}

#endif
