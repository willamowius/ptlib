/*
 * sound_directsound.h
 *
 * DirectX Sound driver implementation.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2006-2007 Novacom, a division of IT-Optics
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
 * The Initial Developer of the Original DirectSound Code is 
 * Vincent Luba <vincent.luba@novacom.be>
 *
 * Contributor(s): Ted Szoczei, Nimajin Software Consulting
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef __DIRECTSOUND_H__
#define __DIRECTSOUND_H__


#include <ptlib.h>
#include <ptbuildopts.h>

#if defined(P_DIRECTSOUND)

#include <ptlib/sound.h>

#include <dsound.h>

#include <ptlib/msos/ptlib/pt_atl.h>

#ifdef _WIN32_WCE
#define LPDIRECTSOUND8 LPDIRECTSOUND
#define LPDIRECTSOUNDBUFFER8 LPDIRECTSOUNDBUFFER
#define LPDIRECTSOUNDCAPTURE8 LPDIRECTSOUNDCAPTURE
#define LPDIRECTSOUNDCAPTUREBUFFER8 LPDIRECTSOUNDCAPTUREBUFFER
#define DirectSoundCreate8 DirectSoundCreate
#define IID_IDirectSoundBuffer8 IID_IDirectSoundBuffer
#define DirectSoundCaptureCreate8 DirectSoundCaptureCreate
#define IID_IDirectSoundCaptureBuffer8 IID_IDirectSoundCaptureBuffer
#endif


class PSoundChannelDirectSound: public PSoundChannel
{
public:
  /**@name Construction */
  //@{
  /** Initialise with no device
   */
  PSoundChannelDirectSound();

  /** Initialise and open device
    */
  PSoundChannelDirectSound(const PString &device,
			     PSoundChannel::Directions dir,
			     unsigned numChannels,
			     unsigned sampleRate,
			     unsigned bitsPerSample);

  ~PSoundChannelDirectSound();
  //@}

  /**@name Open functions */
  //@{
    /**Get the name for the default sound device for this driver.
	   Note that a named device may not necessarily do both playing and
	   recording so the name returned with the <code>dir</code>
       parameter in each value is not necessarily the same.

       @return
       A platform dependent string for the sound player/recorder.
     */
    static PString GetDefaultDevice(
      Directions dir    // Sound I/O direction
    );

  /** Provides a list of detected devices human readable names
      Returns the names array of enumerated devices as PStringArray
   */
  static PStringArray GetDeviceNames(PSoundChannel::Directions);

  /** Open a device with format specifications
      Device name corresponds to Multimedia name (first 32 characters)
    */
  PBoolean Open(const PString & device,
            Directions dir,
            unsigned numChannels,
            unsigned sampleRate,
            unsigned bitsPerSample);

  PString GetName() const { return m_deviceName; }

  PBoolean IsOpen() const
  {
    return (activeDirection == Player)? (m_playbackDevice != NULL) : (m_captureDevice != NULL);
  }

  /** Stop the Read/Write wait
   */
  PBoolean Abort();

  /** Destroy device
   */
  PBoolean Close();
  //@}

  /**@name Channel set up functions */
  //@{
  /** Change the audio format
      Can be called while open, but Aborts I/O
    */
    PBoolean SetFormat(unsigned numChannels,
                       unsigned sampleRate,
                       unsigned bitsPerSample);

    unsigned GetChannels() const { return m_waveFormat.nChannels; }
    unsigned GetSampleRate() const { return m_waveFormat.nSamplesPerSec; }
    unsigned GetSampleSize() const { return m_waveFormat.wBitsPerSample; }
    unsigned GetSampleBlockSize() const { return m_waveFormat.nBlockAlign; }

  /** Configure the device's transfer buffers.
      Read and write functions wait for input or space (blocking thread)
	  in increments of buffer size.
	  Best to make size the same as the len to be given to Read or Write.
      Best performance requires count of 4
      Can be called while open, but Aborts I/O
    */
    PBoolean SetBuffers(PINDEX size, PINDEX count);

    PBoolean GetBuffers(PINDEX & size, PINDEX & count);

    /**Set the volume of the play/read process.
       The volume range is 0 == quiet, 100 == LOUDEST. The volume is a
       logarithmic scale mapped from the lowest gain possible on the device to
       the highest gain
        
       @return
       true if there were no errors.
    */
    PBoolean SetVolume (unsigned);

    /**Get the volume of the play/read process.
       The volume range is 0 == quiet, 100 == LOUDEST. The volume is a
       logarithmic scale mapped from the lowest gain possible on the device to
       the highest gain.

       @return
       true if there were no errors.
    */
    PBoolean GetVolume (unsigned &);
  //@}

  /**@name Error functions */
  //@{
    static PString GetErrorText(Errors lastError, int error);

	/** Get error message description.
        Return a string indicating the error message that may be displayed to
       the user. The error for the last I/O operation in this object is used.
	   Override of PChannel method.
       @return Operating System error description string.
     */
    virtual PString GetErrorText(
      ErrorGroup group = NumErrorGroups   ///< Error group to get
    ) const;
  //@}

  /**@name Play functions */
  //@{
  /** Write specified number of bytes from buf to playback device
      Blocks thread until all bytes have been transferred to device
    */
  PBoolean Write(const void * buf, PINDEX len);

  /** Resets I/O, changes audio format to match sound and configures the 
      device's transfer buffers into one huge buffer, into which the entire
	  sound is loaded and started playing.
	  Returns immediately when wait is false, so you can do other stuff while
	  sound plays.
    */
  PBoolean PlaySound(const PSound & sound, PBoolean wait);

  /** Resets I/O, changes audio format to match file and reconfigures the
      device's transfer buffers. Accepts .wav files. Wait refers to waiting 
	  for completion of last chunk.
    */
  PBoolean PlayFile(const PFilePath & filename, PBoolean wait);

  PBoolean HasPlayCompleted();
  PBoolean WaitForPlayCompletion();
  //@}

  /**@name Record functions */
  //@{
  /** Read specified number of bytes from capture device into buf
	  Blocks thread until number of bytes have been received
    */
  PBoolean Read(void * buf, PINDEX len);

  PBoolean RecordSound(PSound & sound);
  PBoolean RecordFile(const PFilePath & filename);
  PBoolean StartRecording();
  PBoolean IsRecordBufferFull();
  PBoolean AreAllRecordBuffersFull();
  PBoolean WaitForRecordBufferFull();
  PBoolean WaitForAllRecordBuffersFull();
  //@}

  /**@name Notification/Reporting functions */
  //@{
  /** notifier receives parameters (PSoundChannelDirectSound &, enum SOUNDNOTIFY_x)
   */
  void SetNotifier (PNotifier & notifier)
  {
	  m_notifier = notifier;
  }

  enum  // notification codes
  {
    SOUNDNOTIFY_NOTHING = 0,
    SOUNDNOTIFY_ERROR = 1,      // call GetError...
    SOUNDNOTIFY_UNDERRUN = 2,   // check GetxLost
    SOUNDNOTIFY_OVERRUN = 3     // check GetxLost
  };

  /** Total number of bytes/samples transferred between card and the world
   */
  unsigned __int64 GetBytesBuffered (void) const
  {
    return m_dsMoved;
  }
  
  unsigned __int64 GetSamplesBuffered (void) const
  {
    if (m_waveFormat.nBlockAlign == 0)
      return 0;

    return m_dsMoved / m_waveFormat.nBlockAlign;
  }

  /** Total number of bytes/samples moved between card and application by Read/Write.
      This is rarely the same as GetxBuffered after closing because usually the very
	  last written data is not played or the last captured data is not read.
   */
  unsigned __int64 GetBytesMoved (void) const
  {
    return m_moved;
  }
  
  unsigned __int64 GetSamplesMoved (void) const
  {
    if (m_waveFormat.nBlockAlign == 0)
      return 0;

    return m_moved / m_waveFormat.nBlockAlign;
  }

  /** Accumulated number of bytes/samples lost due to Write overrun / Read underrun
      Use GetxLost to find out how many samples were lost
      The value is set to 0 by Open, then it accumulates until Close
      Notifier(extra=OVERRUN) is called when this value changes for recording
      Notifier(extra=UNDERRUN) is called when this value changes for playback
      Notifier(extra=OVERRUN) is called by Player too, but it does not change this value,
	  Player OVERRUN just means that the player is waiting for space to write into.
   */
  unsigned __int64 GetBytesLost (void) const
  {
    return m_lost;
  }

  unsigned __int64 GetSamplesLost (void) const
  {
    if (m_waveFormat.nBlockAlign == 0)
      return 0;

    return m_lost / m_waveFormat.nBlockAlign;
  }
  //@}

protected:
  /** Repeatedly checks until there's space to fit buffer.
      Yields thread between checks.
	  Loop can be ended by calling Abort()(from another thread!)
    */
  PBoolean WaitForPlayBufferFree();

private:
  void Construct();

  PString m_deviceName;

  // pointer read/writes are atomic operations if aligned on 32 bits
  CComPtr<IDirectSoundCapture8>      m_captureDevice;
  CComPtr<IDirectSoundCaptureBuffer> m_captureBuffer;

  CComPtr<IDirectSound8>      m_playbackDevice;
  CComPtr<IDirectSoundBuffer> m_playbackBuffer;
  
  PBoolean SetBufferSections(PINDEX size, PINDEX count);

  PTimeInterval GetInterval(void);
  DWORD GetCyclesPassed(void);

  PBoolean OpenPlayback(LPCGUID deviceId);
  PBoolean OpenPlaybackBuffer (void);
  void ClosePlayback(void);

  /** Checks space available for writing audio to play.
	  Returns true if space enough for one buffer as set by SetBuffers.
	  Sets 'm_available' (space) and (possibly) m_movePos members for use by Write.
    */
  PBoolean CheckPlayBuffer (int & notification);

  PBoolean OpenCapture(LPCGUID deviceId);
  PBoolean OpenCaptureBuffer(void);
  void CloseCapture(void);

  /** Checks for input available from recorder
	  Returns true if enough input to fill one buffer as set by SetBuffers.
	  Sets 'm_available' (input) and (possibly) m_movePos members for use by Read.
    */
  PBoolean CheckCaptureBuffer(int & notification);

  PBoolean m_isStreaming;   // causes play to loop old audio when no new audio submitted
  PINDEX m_bufferSectionCount;
  PINDEX m_bufferSectionSize;
  PINDEX m_bufferSize;      // directSound buffer is divided into Count notification sections, each of Size
  DWORD m_movePos;          // byte offset from start of buffer to where we can write or read
  DWORD m_available;        // number of bytes space available to write into, or number of bytes available to read
  DWORD m_dsPos;			// DirectSound read/write byte position in buffer
  PTimeInterval m_tick;		// time of last buffer poll
  unsigned __int64 m_dsMoved;// total number of bytes transferred between card and the world
  unsigned __int64 m_moved; // total number of bytes moved between card and application by Read/Write
  unsigned __int64 m_lost;  // bytes lost due to Write overrun / Read underrun

  WAVEFORMATEX m_waveFormat;// audio format supplied to DirectSound

  enum // m_triggerEvent indeces
  {
    SOUNDEVENT_SOUND = 0, // triggered by DirectSound at buffer boundaries
    SOUNDEVENT_ABORT = 1  // triggered by Abort/Close
  };
  HANDLE m_triggerEvent[2];
  
  PMutex m_bufferMutex;     // prevents closing while active, protects xDevice and xBuffer members
                            // and also m_bufferSectionCount, m_bufferSectionSize & m_bufferSize
  HMIXER       m_mixer;     // for volume control
  MIXERCONTROL m_volumeControl;

  PBoolean OpenMixer(UINT waveDeviceID);
  void CloseMixer ();

  PNotifier m_notifier;     // hook for notification code handler
};

#endif // P_DIRECTSOUND

#endif  /* __DIRECTSOUND_H__ */
