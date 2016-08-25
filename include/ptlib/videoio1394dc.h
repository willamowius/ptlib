/*
 * videoio1394dc.h
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
 * Classes to support streaming video input from IEEE 1394 cameras.
 * Detailed explanation can be found at src/ptlib/unix/video4dc1394.cxx
 *
 * $Revision$
 * $Author$
 * $Date$
 */


#ifndef PTLIB_VIDEOIO1394DC_H
#define PTLIB_VIDEOIO1394DC_H

#ifdef __GNUC__
#pragma interface
#endif

#include <libraw1394/raw1394.h>
#include <libdc1394/dc1394_control.h>

/** This class defines a video input device that
    generates fictitous image data.
*/
class PVideoInput1394DcDevice : public PVideoInputDevice
{
    PCLASSINFO(PVideoInput1394DcDevice, PVideoInputDevice);
 public:
  /** Create a new video input device.
   */
    PVideoInput1394DcDevice();

    /**Close the video input device on destruction.
      */
    ~PVideoInput1394DcDevice();

    /**Open the device given the device name.
      */
    PBoolean Open(
      const PString & deviceName,   /// Device name to open
      PBoolean startImmediate = true    /// Immediately start device
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
    static PStringList GetInputDeviceNames();

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

    /**Set the frame size to be used, trying converters if available.

       If the device does not support the size, a set of alternate resolutions
       are attempted.  A converter is setup if possible.
    */
    PBoolean SetFrameSizeConverter(
      unsigned width,        /// New width of frame
      unsigned height,       /// New height of frame
      PBoolean     bScaleNotCrop /// Scale or crop/pad preference
    );

    /**Set the colour format to be used, trying converters if available.

       This function will set the colour format on the device to one that
       is compatible with a registered converter, and install that converter
       so that the correct format is used.
    */
    PBoolean SetColourFormatConverter(
      const PString & colourFormat // New colour format for device.
    );


 protected:
    raw1394handle_t handle;
    PBoolean is_capturing;
    PBoolean UseDMA;
    nodeid_t * camera_nodes;
    int numCameras;
    dc1394_cameracapture camera;
    int capturing_duration;
    PString      desiredColourFormat;
    unsigned     desiredFrameWidth;
    unsigned     desiredFrameHeight;
};


#endif // PTLIB_VIDEOIO1394DC_H


// End Of File ///////////////////////////////////////////////////////////////
