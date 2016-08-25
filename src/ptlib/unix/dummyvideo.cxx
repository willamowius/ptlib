/*
 * dummyvideo.cxx
 *
 * Classes to support streaming video input (grabbing) and output.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2001 Equivalence Pty. Ltd.
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

#pragma implementation "videoio.h"

#include <ptlib.h>
#include <ptlib/videoio.h>
#include <ptlib/vfakeio.h>
#include <ptlib/vconvert.h>

///////////////////////////////////////////////////////////////////////////////
// PVideoInputDevice

PVideoInputDevice::PVideoInputDevice()
{
}


PBoolean PVideoInputDevice::Open(const PString & devName, PBoolean startImmediate)
{
  return PFalse;    
}


PBoolean PVideoInputDevice::IsOpen() 
{
  return PFalse;    
}


PBoolean PVideoInputDevice::Close()
{
  return PFalse;    
}


PBoolean PVideoInputDevice::Start()
{
  return PFalse;
}


PBoolean PVideoInputDevice::Stop()
{
  return PFalse;
}


PBoolean PVideoInputDevice::IsCapturing()
{
  return PFalse;
}


PBoolean PVideoInputDevice::SetVideoFormat(VideoFormat newFormat)
{
  return PFalse;
}


int PVideoInputDevice::GetBrightness()
{
  return -1;
}


PBoolean PVideoInputDevice::SetBrightness(unsigned newBrightness)
{
  return PFalse;
}


int PVideoInputDevice::GetHue()
{
  return -1;
}


PBoolean PVideoInputDevice::SetHue(unsigned newHue)
{
  return PFalse;
}


int PVideoInputDevice::GetContrast()
{
  return -1;
}


PBoolean PVideoInputDevice::SetContrast(unsigned newContrast)
{
  return PFalse;
}


PBoolean PVideoInputDevice::GetParameters (int *whiteness, int *brightness,
                                       int *colour, int *contrast, int *hue)
{
  return PFalse;
}


int PVideoInputDevice::GetNumChannels() 
{
  return 0;
}


PBoolean PVideoInputDevice::SetChannel(int newChannel)
{
  return PFalse;
}


PBoolean PVideoInputDevice::SetColourFormat(const PString & newFormat)
{
  return PFalse;
}


PBoolean PVideoInputDevice::SetFrameRate(unsigned rate)
{
  return PFalse;
}


PBoolean PVideoInputDevice::GetFrameSizeLimits(unsigned & minWidth,
                                           unsigned & minHeight,
                                           unsigned & maxWidth,
                                           unsigned & maxHeight) 
{
  return PFalse;
}


PBoolean PVideoInputDevice::SetFrameSize(unsigned width, unsigned height)
{
  return PFalse;
}


PINDEX PVideoInputDevice::GetMaxFrameBytes()
{
  return 0;
}



PBoolean PVideoInputDevice::GetFrameData(BYTE * buffer, PINDEX * bytesReturned)
{
  return PFalse;
}


PBoolean PVideoInputDevice::GetFrameDataNoDelay(BYTE * buffer, PINDEX * bytesReturned)
{
  return PFalse;
}


void PVideoInputDevice::ClearMapping()
{
}

PBoolean PVideoInputDevice::VerifyHardwareFrameSize(unsigned width,
                                                unsigned height)
{
	// Assume the size is valid
	return PTrue;
}

PBoolean PVideoInputDevice::TestAllFormats()
{
  return PTrue;
}
    
// End Of File ///////////////////////////////////////////////////////////////
