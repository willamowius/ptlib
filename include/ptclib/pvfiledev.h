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

#ifndef PTLIB_PVFILEDEV_H
#define PTLIB_PVFILEDEV_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>

#if P_VIDEO
#if P_VIDFILE

#include <ptlib.h>
#include <ptlib/video.h>
#include <ptlib/vconvert.h>
#include <ptclib/pvidfile.h>
#include <ptclib/delaychan.h>


///////////////////////////////////////////////////////////////////////////////////////////
//
// This class defines a video capture (input) device that reads video from a raw YUV file
//

class PVideoInputDevice_YUVFile : public PVideoInputDevice
{
  PCLASSINFO(PVideoInputDevice_YUVFile, PVideoInputDevice);
  public:
    enum {
      Channel_PlayAndClose     = 0,
      Channel_PlayAndRepeat    = 1,
      Channel_PlayAndKeepLast  = 2,
      Channel_PlayAndShowBlack = 3,
      ChannelCount             = 4
    };

    /** Create a new file based video input device.
    */
    PVideoInputDevice_YUVFile();

    /** Destroy video input device.
    */
    virtual ~PVideoInputDevice_YUVFile();


    /**Open the device given the device name.
      */
    PBoolean Open(
      const PString & deviceName,   /// Device name to open
      PBoolean startImmediate = true    /// Immediately start device
    );

    /**Determine of the device is currently open.
      */
    PBoolean IsOpen() ;

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

    virtual PStringArray GetDeviceNames() const
      { return GetInputDeviceNames(); }

    /**Retrieve a list of Device Capabilities
      */
    static bool GetDeviceCapabilities(
      const PString & /*deviceName*/, ///< Name of device
      Capabilities * /*caps*/         ///< List of supported capabilities
    ) { return false; }

    /**Get the maximum frame size in bytes.

       Note a particular device may be able to provide variable length
       frames (eg motion JPEG) so will be the maximum size of all frames.
      */
    virtual PINDEX GetMaxFrameBytes();

    /**Grab a frame. 

       There will be a delay in returning, as specified by frame rate.
      */
    virtual PBoolean GetFrameData(
      BYTE * buffer,                 /// Buffer to receive frame
      PINDEX * bytesReturned = NULL  /// Optional bytes returned.
    );

    /**Grab a frame.

       Do not delay according to the current frame rate.
      */
    virtual PBoolean GetFrameDataNoDelay(
      BYTE * buffer,                 /// Buffer to receive frame
      PINDEX * bytesReturned = NULL  /// OPtional bytes returned.
    );


    /**Set the video format to be used.

       Default behaviour sets the value of the videoFormat variable and then
       returns the IsOpen() status.
    */
    virtual PBoolean SetVideoFormat(
      VideoFormat videoFormat   /// New video format
    );

    /**Get the number of video channels available on the device.

       Default behaviour returns 1.
    */
    virtual int GetNumChannels() ;

    /**Set the video channel to be used on the device. Channels have the following meanings:
        0 (default) = play file and close device
        1           = play file and repeat
        2           = play file and replay last frame
        3           = play file and display black frame

       Default behaviour sets the value of the channelNumber variable and then
       returns the IsOpen() status.
    */
    virtual PBoolean SetChannel(
         int channelNumber  /// New channel number for device.
    );
    
    /**Set the colour format to be used.

       Default behaviour sets the value of the colourFormat variable and then
       returns the IsOpen() status.
    */
    virtual PBoolean SetColourFormat(
      const PString & colourFormat   // New colour format for device.
    );
    
    /**Set the video frame rate to be used on the device.

       Default behaviour sets the value of the frameRate variable and then
       return the IsOpen() status.
    */
    virtual PBoolean SetFrameRate(
      unsigned rate  /// Frames per second
    );
         
    /**Get the minimum & maximum size of a frame on the device.

       Default behaviour returns the value 1 to UINT_MAX for both and returns
       false.
    */
    virtual PBoolean GetFrameSizeLimits(
      unsigned & minWidth,   /// Variable to receive minimum width
      unsigned & minHeight,  /// Variable to receive minimum height
      unsigned & maxWidth,   /// Variable to receive maximum width
      unsigned & maxHeight   /// Variable to receive maximum height
    ) ;

    /**Set the frame size to be used.

       Default behaviour sets the frameWidth and frameHeight variables and
       returns the IsOpen() status.
    */
    virtual PBoolean SetFrameSize(
      unsigned width,   /// New width of frame
      unsigned height   /// New height of frame
    );

   
 protected:
   PVideoFile   * m_file;
   PAdaptiveDelay m_pacing;
   unsigned       m_frameRateAdjust;
   bool           m_opened;
};


///////////////////////////////////////////////////////////////////////////////////////////
//
// This class defines a video display (output) device that writes video to a raw YUV file
//

class PVideoOutputDevice_YUVFile : public PVideoOutputDevice
{
  PCLASSINFO(PVideoOutputDevice_YUVFile, PVideoOutputDevice);

  public:
    /** Create a new video output device.
     */
    PVideoOutputDevice_YUVFile();

    /** Destroy video output device.
     */
    virtual ~PVideoOutputDevice_YUVFile();

    /**Get a list of all of the drivers available.
      */
    static PStringArray GetOutputDeviceNames();

    virtual PStringArray GetDeviceNames() const
      { return GetOutputDeviceNames(); }

    /**Open the device given the device name.
      */
    virtual PBoolean Open(
      const PString & deviceName,   /// Device name to open
      PBoolean startImmediate = true    /// Immediately start device
    );

    /**Start the video device I/O.
      */
    PBoolean Start();

    /**Stop the video device I/O capture.
      */
    PBoolean Stop();

    /**Close the device.
      */
    virtual PBoolean Close();

    /**Determine if the device is currently open.
      */
    virtual PBoolean IsOpen();

    /**Set the colour format to be used.

       Default behaviour sets the value of the colourFormat variable and then
       returns the IsOpen() status.
    */
    virtual PBoolean SetColourFormat(
      const PString & colourFormat   // New colour format for device.
    );
    
    /**Get the maximum frame size in bytes.

       Note a particular device may be able to provide variable length
       frames (eg motion JPEG) so will be the maximum size of all frames.
      */
    virtual PINDEX GetMaxFrameBytes();

    /**Set a section of the output frame buffer.
      */
    virtual PBoolean SetFrameData(
      unsigned x,
      unsigned y,
      unsigned width,
      unsigned height,
      const BYTE * data,
      PBoolean endFrame = true
    );

  protected:  
   PVideoFile * m_file;
   bool         m_opened;
};


#endif // P_VIDFILE
#endif

#endif // PTLIB_PVFILEDEV_H


// End Of File ///////////////////////////////////////////////////////////////
