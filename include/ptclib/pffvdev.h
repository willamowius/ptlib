/*
 * pffvdev.cxx
 *
 * Video device for ffmpeg
 *
 * Portable Windows Library
 *
 * Copyright (C) 2008 Post Increment
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

#ifndef PTLIB_PFFVDEV_H
#define PTLIB_PFFVDEV_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>

#if P_VIDEO
#if P_FFVDEV

#include <ptlib/pipechan.h>
#include <ptclib/delaychan.h>

///////////////////////////////////////////////////////////////////////////////////////////
//
// This class defines a video capture (input) device that reads video from an FFMpeg command
//


class PVideoInputDevice_FFMPEG : public PVideoInputDevice
{
 PCLASSINFO(PVideoInputDevice_FFMPEG, PVideoInputDevice);
 public:
    /** Create a new file based video input device.
    */
    PVideoInputDevice_FFMPEG();

    /** Destroy video input device.
    */
    ~PVideoInputDevice_FFMPEG();


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

    /**Set the video channel to be used on the device. 
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

    void ClearMapping() { return ; }


 protected:
   unsigned       m_ffmpegFrameWidth;
   unsigned       m_ffmpegFrameHeight;
   unsigned       m_ffmpegFrameRate;
   unsigned       m_ffmpegFrameSize;
   PPipeChannel   m_command;
   PINDEX         m_videoFrameSize;
   unsigned       grabCount;
   PAdaptiveDelay pacing;
};


#endif // P_FFVDEV
#endif

#endif // PTLIB_PVFILEDEV_H


// End Of File ///////////////////////////////////////////////////////////////
