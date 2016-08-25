/*
 * videoio1394avc.h
 *
 * This file is a based on videoio1394dc.h
 *
 * Portable Windows Library
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * Contributor(s): Georgi Georgiev <chutz@gg3.net>
 *
 * $Revision$
 * $Author$
 * $Date$
 */


#ifndef _PVIDEOIO1394AVC

#define _PVIDEOIO1394AVC

#ifdef __GNUC__
#pragma interface
#endif

#include <sys/utsname.h>
#include <libraw1394/raw1394.h>
#include <libavc1394/avc1394.h>
#include <libavc1394/rom1394.h>
#include <libraw1394/csr.h>
#include <libdv/dv.h>

#include <ptlib.h>
#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptlib/file.h>
#if !P_USE_INLINES
#include <ptlib/contain.inl>
#endif
#include <ptclib/delaychan.h>


/** This class defines a video input device that
    generates fictitous image data.
*/

class PVideoInputDevice_1394AVC : public PVideoInputDevice
{
    PCLASSINFO(PVideoInputDevice_1394AVC, PVideoInputDevice);
 public:
  /** Create a new video input device.
   */
    PVideoInputDevice_1394AVC();

    /**Close the video input device on destruction.
      */
    ~PVideoInputDevice_1394AVC();

    /**Open the device given the device name.
      */
    PBoolean Open(
      const PString & deviceName,   /// Device name to open
      PBoolean startImmediate = PTrue    /// Immediately start device
    );

    /**Determine of the device is currently open.
      */
    PBoolean IsOpen();

    /**Close the device.
      */
    PBoolean Close();

    /**Start the video device I/O.
      */
    PBoolean Start();

    /**Stop the video device I/O capture.
      */
    PBoolean Stop();

    /**Determine if the video device I/O capture is in progress.
      */
    PBoolean IsCapturing();

    /**Get a list of all of the drivers available.
      */
    static PStringArray GetInputDeviceNames();

    PStringArray GetDeviceNames() const
    { return GetInputDeviceNames(); }

    /**Get the maximum frame size in bytes.

       Note a particular device may be able to provide variable length
       frames (eg motion JPEG) so will be the maximum size of all frames.
      */
    PINDEX GetMaxFrameBytes();

    /**Grab a frame, after a delay as specified by the frame rate.
      */
    PBoolean GetFrameData(
      BYTE * buffer,                 /// Buffer to receive frame
      PINDEX * bytesReturned = NULL  /// OPtional bytes returned.
    );

    /**Grab a frame. Do not delay according to the current frame rate parameter.
      */
    PBoolean GetFrameDataNoDelay(
      BYTE * buffer,                 /// Buffer to receive frame
      PINDEX * bytesReturned = NULL  /// OPtional bytes returned.
    );


    /**Get the brightness of the image. 0xffff-Very bright.
     */
    int GetBrightness();

    /**Set brightness of the image. 0xffff-Very bright.
     */
    PBoolean SetBrightness(unsigned newBrightness);


    /**Get the whiteness of the image. 0xffff-Very white.
     */
    int GetWhiteness();

    /**Set whiteness of the image. 0xffff-Very white.
     */
    PBoolean SetWhiteness(unsigned newWhiteness);


    /**Get the colour of the image. 0xffff-lots of colour.
     */
    int GetColour();

    /**Set colour of the image. 0xffff-lots of colour.
     */
    PBoolean SetColour(unsigned newColour);


    /**Get the contrast of the image. 0xffff-High contrast.
     */
    int GetContrast();

    /**Set contrast of the image. 0xffff-High contrast.
     */
    PBoolean SetContrast(unsigned newContrast);


    /**Get the hue of the image. 0xffff-High hue.
     */
    int GetHue();

    /**Set hue of the image. 0xffff-High hue.
     */
    PBoolean SetHue(unsigned newHue);
    
    
    /**Return whiteness, brightness, colour, contrast and hue in one call.
     */
    PBoolean GetParameters (int *whiteness, int *brightness, 
				int *colour, int *contrast, int *hue);

    /**Get the minimum & maximum size of a frame on the device.
    */
    PBoolean GetFrameSizeLimits(
      unsigned & minWidth,   /// Variable to receive minimum width
      unsigned & minHeight,  /// Variable to receive minimum height
      unsigned & maxWidth,   /// Variable to receive maximum width
      unsigned & maxHeight   /// Variable to receive maximum height
    ) ;

    void ClearMapping();

    int GetNumChannels();
    PBoolean SetChannel(
         int channelNumber  /// New channel number for device.
    );
    PBoolean SetFrameRate(
      unsigned rate  /// Frames  per second
    );
    PBoolean SetVideoFormat(
      VideoFormat videoFormat   /// New video format
    );
    PBoolean SetFrameSize(
      unsigned width,   /// New width of frame
      unsigned height   /// New height of frame
    );
    PBoolean SetColourFormat(
      const PString & colourFormat   // New colour format for device.
    );


    /**Try all known video formats & see which ones are accepted by the video driver
     */
    PBoolean TestAllFormats();


 protected:

    raw1394handle_t handle;
    PBoolean is_capturing;
    PBoolean UseDMA;
    dv_decoder_t * dv_decoder;
    PINDEX frameBytes;
    int port;
    PAdaptiveDelay m_pacing;

    PBoolean SetupHandle();
};

int RawISOHandler (raw1394handle_t handle, int channel, size_t length, u_int32_t * data);

#endif


// End Of File ///////////////////////////////////////////////////////////////
