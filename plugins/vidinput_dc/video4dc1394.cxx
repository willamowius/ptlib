/*
 * README for IEEE 1394 digital camera support routine in PWLib
 * ------------------------------------------------------------
 * 
 * PWLib now contains preliminary support for the IEEE 1394 digital
 * cameras under Linux.
 * 
 * 
 * Supported cameras:
 * 
 * There are two kind of the specifications of IEEE 1394 digital video
 * cameras, one is called "digital camera" and another is "AV/C camera".
 * A digital camera sends uncompressed video data while an AV/C camera
 * sends compressed data. This file is for digital cameras.
 * We can find a list of supported digital cameras by the Linux
 * device driver at:
 * http://www.tele.ucl.ac.be/PEOPLE/DOUXCHAMPS/ieee1394/cameras/
 * 
 * 
 * Installation and Use:
 * 
 * To select your 1394 camera for video input device instead of
 * usual Video4Linux devices, specify "/dev/raw1394" or "/dev/video1394"
 * as the filename of video input device. For example "ohphone
 * --videoinput /dev/raw1394" should use your 1394 camera as video input.
 * 
 * "/dev/video1394" uses faster DMA transfer for video input.
 * 
 * If you use DEVFS, the filename for DMA transfer may be /dev/video1394/0.
 * 
 * Requirements for Installation:
 * 
 * You needs the following softwares to compile the 1394 camera support
 * module in PWLib.
 * 
 * - libdc1394 0.9.0 or later.
 * - Linux 2.4.19 or later, which is required by the above version of
 *   libdc1394
 * - libraw1394 0.9.0 or later
 * 
 * You cannot compile it with older versions of libdc1394.
 * 
 * Troubleshooting:
 * 
 * If this module does not work for you, please verify the following
 * items before sending email:
 * 
 * 1) Can you view image of your camera by other programs? A sample
 *    program called "grab_gray_image" is included in the example
 *    directory of libdc1394. Please run grab_gray_image and see what
 *    happens. You can also use Coriander instead.
 *    (http://www.tele.ucl.ac.be/PEOPLE/DOUXCHAMPS/ieee1394/coriander/).
 * 2) If you have make sure other programs can use the camera, but
 *    this module still does not work, please run the debbuging version
 *    of ohphone with option "-tttt -o log.txt". Examine the file "log.txt"
 *    and you may see what is wrong.
 * 
 * 
 * Problem Reports:
 * They should be send to Ryutaroh Matsumoto <ryutaroh@rmatsumoto.org>.
 * 
 * 
 * Acknowledgment:
 * R. Matsumoto thanks Dr. Derek Smithies for his kind support for making
 * this module.
 * 
 * 
 * Technical Notes for Programmers
 * ------------------------------------------------------------
 *
 * Test Environment:
 * This module was tested against:
 *
 * Pentium III  
 * Linux 2.4.19
 * libraw1394 0.9.0
 * libdc1394 0.9.0
 *
 * Irez StealthFire Camera (http://www.irez.com)
 * OrangeMicro iBot Camera (http://www.orangemicro.com)
 *
 *
 * Internal Structure:
 * This module has been tested against the ohphone and ekiga/GnomeMeeting
 * video phone programs. They use 352x288 and 176x144 resolutions in
 * YUV420P color format. So this module only supports these
 * resolutions and YUV420P.
 *
 * 1394 Digital Cameras has many resolutions and color formats. Among
 * them, this module uses:
 *  160x120 YUV(4:4:4)  for  176x144 PTlib resolution, and
 *  320x240 YUV(4:2:2)  for  352x288 PTlib resolution.
 * The bus speed is always set to P_DC1394_DEFAULT_SPEED (400 Mbps).
 * If transfer at P_DC1394_DEFAULT_SPEED is not supported by your
 * camera, this module does not capture images from yours. In such
 * a case please set P_DC1394_DEFAULT_SPEED to appropriate value.
 *
 * Conversion routines from above formats to YUV420P were added to
 * src/ptlib/common/vconvert.cxx
 *
 * ToDo or Bugs:
 * This module does not implement GetBrightness() etc.  1394 cameras
 * can do both automatic control and manual control of brightness
 * etc., and they are usually set to automatic
 * control. Get/SetBrightness() etc. cannot access manual/automatic
 * selection. So we cannot implement an interface that can fully
 * control all of 1394 camera features. I decided not to implement
 * controlling interface at all. Those features can be controlled by
 * Coriander program even when ohphone or GnomeMeeting is being used.
 * Please use Coriander.
 *
 * PVideoInputDevice_1394DC does not allow creation of two or more instances.
 *
 * The bus speed is always set to P_DC1394_DEFAULT_SPEED (400 Mbps).
 * If transfer at P_DC1394_DEFAULT_SPEED is not supported by your
 * camera, this module does not capture images from yours. In such
 * a case please set P_DC1394_DEFAULT_SPEED to appropriate value.
 *
 * Copyright:
 * Copyright (c) 2002 Ryutaroh Matsumoto <ryutaroh@rmatsumoto.org>
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
 *
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#pragma implementation "videoio1394dc.h"

#include <ptlib.h>
#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptlib/file.h>
#include "videoio1394dc.h"
#include <sys/utsname.h>

PCREATE_VIDINPUT_PLUGIN(1394DC);

#ifndef P_DC1394_DEFAULT_SPEED
#define P_DC1394_DEFAULT_SPEED  SPEED_400
#endif

//#define ESTIMATE_CAPTURE_PERFORMANCE

#ifdef ESTIMATE_CAPTURE_PERFORMANCE
// for debugging
static PInt64 start_time;
static int num_captured;
#endif


///////////////////////////////////////////////////////////////////////////////
// PVideoInput1394DC

PVideoInputDevice_1394DC::PVideoInputDevice_1394DC()
{
  handle = NULL;
  is_capturing = PFalse;
  capturing_duration = 10000; // arbitrary large value suffices
}

PVideoInputDevice_1394DC::~PVideoInputDevice_1394DC()
{
  Close();
}

#define OK 1
#define NG 0
static int kernel_version_ok(void)
{
  struct utsname utsbuf;
  unsigned major_ver, minor_ver, minorminor_ver;

  if (uname(&utsbuf) == -1)
    return NG;

  /* utsbuf.release looks like "2.4.19-pre8". */
  if (sscanf(utsbuf.release, "%u.%u.%u", &major_ver, &minor_ver,
             &minorminor_ver) < 3)
    return NG; /* Should we return OK? */

  if (major_ver >= 3)
    return OK;
  else if (major_ver <= 1)
    return NG;

  if (minor_ver >= 6)
    return OK;
  else if (minor_ver <= 3)
    return NG;
  else if (minor_ver == 4)
    return minorminor_ver >= 19;
  else /* if (minor_ver == 5) */
    return minorminor_ver >= 9;
}

PBoolean PVideoInputDevice_1394DC::Open(const PString & devName, PBoolean startImmediate)
{
  if (!kernel_version_ok()) {
    PTRACE(0, "The Linux kernel version is too old.");
    return PFalse;
  }

  if (IsOpen()) {
    PTRACE(0, "You cannot open PVideoInputDevice_1394DC twice.");
    return PFalse;
  }

  if (devName == "/dev/raw1394")
    UseDMA = PFalse;
  // Don't forget /dev/video1394/0
  else if (strncmp(devName, "/dev/video1394", 14) == 0)
    UseDMA = PTrue;
  else {
    PTRACE(0, "devName must be /dev/raw1394 or /dev/video1394");
    return PFalse;
  }

  // See if devName is accessible.
  if (!PFile::Exists(devName)) {
    PTRACE(1, devName << " is not accessible.");
    return PFalse;
  }

  /*-----------------------------------------------------------------------
   *  Open ohci and asign handle to it
   *-----------------------------------------------------------------------*/
  handle = dc1394_create_handle(0);
  if (handle==NULL)
  {
    PTRACE(0, "Unable to aquire a raw1394 handle\ndid you insmod the drivers?\n");
    return PFalse;
  }

  /*-----------------------------------------------------------------------
   *  get the camera nodes and describe them as we find them
   *-----------------------------------------------------------------------*/
  int numNodes = raw1394_get_nodecount(handle);
  camera_nodes = dc1394_get_camera_nodes(handle,&numCameras,0);
  if (numCameras<1)
  {
    PTRACE(0, "no cameras found :(\n");
    dc1394_destroy_handle(handle);
    handle = NULL;
    return PFalse;
  }

  /*-----------------------------------------------------------------------
   *  to prevent the iso-transfer bug from raw1394 system, check if
   *  camera is highest node. For details see 
   *  http://linux1394.sourceforge.net/faq.html#DCbusmgmt
   *  and
   *  http://sourceforge.net/tracker/index.php?func=detail&aid=435107&group_id=8157&atid=108157
   *-----------------------------------------------------------------------*/
  for (int i=0; i<numCameras; i++) {
    if( camera_nodes[i] == numNodes-1) {
      PTRACE(0,"Sorry, your camera is the highest numbered node\n"
             "of the bus, and has therefore become the root node.\n"
             "The root node is responsible for maintaining \n"
             "the timing of isochronous transactions on the IEEE \n"
             "1394 bus.  However, if the root node is not cycle master \n"
             "capable (it doesn't have to be), then isochronous \n"
             "transactions will not work.  The host controller card is \n"
             "cycle master capable, however, most cameras are not.\n"
             "\n"
             "The quick solution is to add the parameter \n"
             "attempt_root=1 when loading the OHCI driver as a \n"
             "module.  So please do (as root):\n"
             "\n"
             "   rmmod ohci1394\n"
             "   insmod ohci1394 attempt_root=1\n"
             "\n"
             "for more information see the FAQ at \n"
             "http://linux1394.sourceforge.net/faq.html#DCbusmgmt\n"
             "\n");
      dc1394_destroy_handle(handle);
      handle = NULL;
      return PFalse;
    }
  }

  frameHeight = 240;
  frameWidth = 320;
  colourFormat = "UYVY422";
  capturing_duration = 10000; // arbitrary large value suffices
  deviceName = devName;

  // select the specified input and video format
  if (!SetChannel(channelNumber) ||
      !SetVideoFormat(videoFormat)) {
    PTRACE(1, "SetChannel() or SetVideoFormat() failed");
    Close();
    return PFalse;
  }

  if (startImmediate && !Start()) {
    Close();
    return PFalse;
  }

  // Verify the format that the card accept
  quadlet_t supported_framerates;
  supportedFormat = 0;
  if (dc1394_query_supported_framerates(handle, camera_nodes[channelNumber],
	FORMAT_VGA_NONCOMPRESSED, MODE_320x240_YUV422,
	&supported_framerates) == DC1394_SUCCESS) {
     supportedFormat |= DC1394_FORMAT_320x240;
  }

  if (dc1394_query_supported_framerates(handle, camera_nodes[channelNumber],
	FORMAT_VGA_NONCOMPRESSED, MODE_160x120_YUV444,
	&supported_framerates) == DC1394_SUCCESS) {
     supportedFormat |= DC1394_FORMAT_160x120;
  }
  
  PTRACE(3, "Successfully opended\n");
  return PTrue;
}


PBoolean PVideoInputDevice_1394DC::IsOpen() 
{
  return handle != NULL;
}


PBoolean PVideoInputDevice_1394DC::Close()
{
  if (IsOpen()) {
    if (IsCapturing())
      Stop();
    dc1394_destroy_handle(handle);
    handle = NULL;
    return PTrue;
  } else
    return PFalse;
}

PBoolean PVideoInputDevice_1394DC::Start()
{
  int dc1394_mode;
  if (!IsOpen()) return PFalse;
  if (is_capturing) return PTrue;
  if (frameWidth == 320 && frameHeight == 240)
    dc1394_mode = MODE_320x240_YUV422;
  else if (frameWidth == 160 && frameHeight == 120)
    dc1394_mode = MODE_160x120_YUV444;
  else {
    PTRACE(1, "Frame size is neither 320x240 or 160x120" << frameWidth << "x" << frameHeight);
    return PFalse;
  }
  PTRACE(1, deviceName << " " << channelNumber);

  quadlet_t supported_framerates;
  if (dc1394_query_supported_framerates(handle, camera_nodes[channelNumber],
          FORMAT_VGA_NONCOMPRESSED, dc1394_mode,
          &supported_framerates) != DC1394_SUCCESS) {
    PTRACE(1, "dc1394_query_supported_framerates() failed.");
    return PFalse;
  }

  int framerate;

  // supported_framerates seems always in the network byte order.
  if (supported_framerates & (1U << (31-5)))
    framerate = FRAMERATE_60;
  else if (supported_framerates & (1U << (31-4)))
    framerate = FRAMERATE_30;
  else if (supported_framerates & (1U << (31-3)))
    framerate = FRAMERATE_15;
  else if (supported_framerates & (1U << (31-2)))
    framerate = FRAMERATE_7_5;
  else if (supported_framerates & (1U << (31-1)))
    framerate = FRAMERATE_3_75;
  else if (supported_framerates & (1U << (31-0)))
    framerate = FRAMERATE_1_875;
  else {
    PTRACE(1, "Frame rate " << supported_framerates << " is not supported");
    return PFalse;
  }  

  // In order to compile the following line, you need libdc1394 0.9.0 or later.
  if ((UseDMA &&dc1394_dma_setup_capture(handle,camera_nodes[channelNumber],
                           0, /* channel of IEEE 1394 */ 
                           FORMAT_VGA_NONCOMPRESSED,
                           dc1394_mode,
                           P_DC1394_DEFAULT_SPEED,
                           framerate, 4, 1,
#ifdef NEW_DC_API
                           1,
#endif
                           deviceName,
      &camera)!=DC1394_SUCCESS) ||
      (!UseDMA && dc1394_setup_capture(handle,camera_nodes[channelNumber],
                           0, /* channel of IEEE 1394 */ 
                           FORMAT_VGA_NONCOMPRESSED,
                           dc1394_mode,
                           P_DC1394_DEFAULT_SPEED,
                           framerate,
       &camera)!=DC1394_SUCCESS))
  {
    PTRACE(0,"unable to setup camera-\n"
             "check " __FILE__ " to make sure\n"
             "that the video mode,framerate and format are\n"
             "supported by your camera\n");
    return PFalse;
  }

  /*-----------------------------------------------------------------------
   *  have the camera start sending us data
   *-----------------------------------------------------------------------*/
  if (dc1394_start_iso_transmission(handle,camera.node)
      !=DC1394_SUCCESS) 
  {
    PTRACE(0, "unable to start camera iso transmission\n");
    if (UseDMA)
      dc1394_dma_release_camera(handle,&camera);
    else
      dc1394_release_camera(handle,&camera);
    return PFalse;
  }
  is_capturing = PTrue;
#ifdef ESTIMATE_CAPTURE_PERFORMANCE
  PTime now;
  start_time = now.GetTimestamp();
  num_captured = 0;
#endif
  return PTrue;
}


PBoolean PVideoInputDevice_1394DC::Stop()
{
  if (IsCapturing()) {
    dc1394_stop_iso_transmission(handle,camera.node);
    if (UseDMA) {
    dc1394_dma_unlisten(handle, &camera);
    dc1394_dma_release_camera(handle,&camera);
    } else
      dc1394_release_camera(handle,&camera);
    is_capturing = PFalse;
    return PTrue;
  } else
    return PFalse;
}


PBoolean PVideoInputDevice_1394DC::IsCapturing()
{
  return is_capturing;
}

PStringList PVideoInputDevice_1394DC::GetInputDeviceNames()
{
  PStringList list;

  if (PFile::Exists("/dev/raw1394"))
    list.AppendString("/dev/raw1394");
 
  if (PFile::Exists("/dev/video1394/0")) {

    // DEVFS naming scheme
    for (int i=0; ; i++) {
      PString devname = PString("/dev/video1394/") + PString(i);
      if (PFile::Exists(devname))
        list.AppendString(devname);
      else
        break;
    }
  }
  else if (PFile::Exists("/dev/video1394"))
    /* traditional naming */
    list.AppendString("/dev/video1394");

  return list;
}


PBoolean PVideoInputDevice_1394DC::SetVideoFormat(VideoFormat newFormat)
{
  if (!PVideoDevice::SetVideoFormat(newFormat)) {
    PTRACE(3,"PVideoDevice::SetVideoFormat\t failed for format "<<newFormat);
    return PFalse;
  }
  return PTrue;
}

int PVideoInputDevice_1394DC::GetBrightness()
{
  return -1;
}


PBoolean PVideoInputDevice_1394DC::SetBrightness(unsigned newBrightness)
{
  return PFalse;
}


int PVideoInputDevice_1394DC::GetHue()
{
  return -1;
}


PBoolean PVideoInputDevice_1394DC::SetHue(unsigned newHue)
{
  return PFalse;
}


int PVideoInputDevice_1394DC::GetContrast()
{
  return -1;
}


PBoolean PVideoInputDevice_1394DC::SetContrast(unsigned newContrast)
{
  return PFalse;
}

PBoolean PVideoInputDevice_1394DC::SetColour(unsigned newColour) 
{
  return -1;
}

int PVideoInputDevice_1394DC::GetColour()
{
  return -1;
}


PBoolean PVideoInputDevice_1394DC::SetWhiteness(unsigned newWhiteness) 
{
  return PFalse;
}

int PVideoInputDevice_1394DC::GetWhiteness()
{
  return -1;
}


PBoolean PVideoInputDevice_1394DC::GetParameters (int *whiteness, int *brightness,
                                       int *colour, int *contrast, int *hue)
{
  *whiteness = -1;
  *brightness = -1;
  *colour = -1;
  *hue = -1;
  return PFalse;
}


int PVideoInputDevice_1394DC::GetNumChannels() 
{
  return numCameras;
}


PBoolean PVideoInputDevice_1394DC::SetChannel(int newChannel)
{
  if (PVideoDevice::SetChannel(newChannel) == PFalse)
    return PFalse;
  if(IsCapturing()) {
    Stop();
    Start();
  }
  return PTrue;
}



PBoolean PVideoInputDevice_1394DC::SetFrameRate(unsigned rate)
{
  if (!PVideoDevice::SetFrameRate(rate))
    return PFalse;

  return PTrue;
}


PBoolean PVideoInputDevice_1394DC::GetFrameSizeLimits(unsigned & minWidth,
                                           unsigned & minHeight,
                                           unsigned & maxWidth,
                                           unsigned & maxHeight) 
{
  minWidth = 160;
  maxWidth = 320;
  minHeight = 120;
  maxHeight = 240;
  return PTrue;
}


PINDEX PVideoInputDevice_1394DC::GetMaxFrameBytes()
{
  return GetMaxFrameBytesConverted(frameBytes);
}


PBoolean PVideoInputDevice_1394DC::GetFrameDataNoDelay(BYTE * buffer, PINDEX * bytesReturned)
{
  if (!IsCapturing()) return PFalse;

  PTRACE(3, "We are going to single capture.\n");
  if ((UseDMA && dc1394_dma_single_capture(&camera)!=DC1394_SUCCESS) ||
      (!UseDMA && dc1394_single_capture(handle,&camera)!=DC1394_SUCCESS)){
    PTRACE(1, "dc1394_single_capture() failed.");
    return PFalse;
  }
  
  PTRACE(3, "single captured, try to convert\n");

  // If converting on the fly do it from frame store to output buffer, otherwise do
  // straight copy.
  if (converter != NULL)
    converter->Convert((const BYTE *)camera.capture_buffer, buffer, bytesReturned);
  else {
    PTRACE(1, "Converter must exist. Something goes wrong.");
    return PFalse;
  }

#ifdef ESTIMATE_CAPTURE_PERFORMANCE
  ++num_captured;
  PTime now;
  double capturing_time = (double)((now.GetTimestamp()-start_time))/1000000;
  ::fprintf(stderr, "time %f, num_captured=%d, fps=%f\n", capturing_time, num_captured, num_captured/capturing_time);
#endif

  if (UseDMA)
    dc1394_dma_done_with_buffer(&camera);
  return PTrue;
}

PBoolean PVideoInputDevice_1394DC::GetFrameData(BYTE * buffer, PINDEX * bytesReturned)
{
  m_pacing.Delay(1000/GetFrameRate());
  return GetFrameDataNoDelay(buffer,bytesReturned);
}


void PVideoInputDevice_1394DC::ClearMapping()
{
}


PBoolean PVideoInputDevice_1394DC::TestAllFormats()
{
  return PTrue;
}


PBoolean PVideoInputDevice_1394DC::SetColourFormat(const PString & newFormat)
{
  return newFormat == colourFormat;
}


PBoolean PVideoInputDevice_1394DC::SetFrameSize(unsigned width, unsigned height)
{
  if (width == 320 && height == 240) {
    if (!(supportedFormat & DC1394_FORMAT_320x240))
      return PFalse;
    colourFormat = "UYVY422";
  }
  else if (width == 160 && height == 120) {
    if (!(supportedFormat & DC1394_FORMAT_160x120))
      return PFalse;
    colourFormat = "UYV444";
  }
  else
    return PFalse;

  frameWidth = width;
  frameHeight = height;

  frameBytes = PVideoDevice::CalculateFrameBytes(frameWidth, frameHeight, colourFormat);

  if (IsCapturing()) {
    Stop();
    Start();
  }

  return PTrue;
}


    
// End Of File ///////////////////////////////////////////////////////////////
