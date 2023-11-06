/*
 * sound.h
 *
 * Sound class.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */


///////////////////////////////////////////////////////////////////////////////
// PSound

#ifndef _PSOUND_WIN32
#define _PSOUND_WIN32

#include <mmsystem.h>
#ifndef _WIN32_WCE
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif
#endif

class PWaveFormat : public PObject
{
  PCLASSINFO(PWaveFormat, PObject)
  public:
    PWaveFormat();
    ~PWaveFormat();
    PWaveFormat(const PWaveFormat & fmt);
    PWaveFormat & operator=(const PWaveFormat & fmt);

    void PrintOn(ostream &) const;
    void ReadFrom(istream &);

    void SetFormat(unsigned numChannels, unsigned sampleRate, unsigned bitsPerSample);
    void SetFormat(const void * data, PINDEX size);

    PBoolean           SetSize   (PINDEX sz);
    PINDEX         GetSize   () const { return  size;       }
    void         * GetPointer() const { return  waveFormat; }
    WAVEFORMATEX * operator->() const { return  waveFormat; }
    WAVEFORMATEX & operator *() const { return *waveFormat; }
    operator   WAVEFORMATEX *() const { return  waveFormat; }

  protected:
    PINDEX         size;
    WAVEFORMATEX * waveFormat;
};


class PMultiMediaFile
{
  public:
    PMultiMediaFile();
    ~PMultiMediaFile();

    PBoolean CreateWaveFile(const PFilePath & filename,
                        const PWaveFormat & waveFormat,
                        DWORD dataSize);
    PBoolean OpenWaveFile(const PFilePath & filename,
                      PWaveFormat & waveFormat,
                      DWORD & dataSize);

    PBoolean Open(const PFilePath & filename, DWORD dwOpenFlags, LPMMIOINFO lpmmioinfo = NULL);
    PBoolean Close(UINT wFlags = 0);
    PBoolean Ascend(MMCKINFO & ckinfo, UINT wFlags = 0);
    PBoolean Descend(UINT wFlags, MMCKINFO & ckinfo, LPMMCKINFO lpckParent = NULL);
    PBoolean Read(void * data, PINDEX len);
    PBoolean CreateChunk(MMCKINFO & ckinfo, UINT wFlags = 0);
    PBoolean Write(const void * data, PINDEX len);

    DWORD GetLastError() const { return dwLastError; }

  protected:
    HMMIO hmmio;
    DWORD dwLastError;
};


class PWaveBuffer : public PBYTEArray
{
  PCLASSINFO(PWaveBuffer, PBYTEArray);
  private:
    PWaveBuffer(PINDEX sz = 0);
    ~PWaveBuffer();

    PWaveBuffer & operator=(const PSound & sound);

    DWORD Prepare(HWAVEOUT hWaveOut, PINDEX & count);
    DWORD Prepare(HWAVEIN hWaveIn);
    DWORD Release();

    void PrepareCommon(PINDEX count);

    HWAVEOUT hWaveOut;
    HWAVEIN  hWaveIn;
    WAVEHDR  header;

  friend class PSoundChannelWin32;
};

PARRAY(PWaveBufferArray, PWaveBuffer);


class PSoundChannelWin32: public PSoundChannel
{
 public:
    PSoundChannelWin32();
    void Construct();
    PSoundChannelWin32(const PString &device,
                     PSoundChannel::Directions dir,
                     unsigned numChannels,
                     unsigned sampleRate,
                     unsigned bitsPerSample);
    ~PSoundChannelWin32();
    static PStringArray GetDeviceNames(PSoundChannel::Directions = Player);
    PBoolean Open(
      const PString & device,
      Directions dir,
      unsigned numChannels,
      unsigned sampleRate,
      unsigned bitsPerSample
    );
    PBoolean Setup();
    PBoolean Close();
    PBoolean IsOpen() const;
    PBoolean Write(const void * buf, PINDEX len);
    PBoolean Read(void * buf, PINDEX len);
    PBoolean SetFormat(unsigned numChannels,
                   unsigned sampleRate,
                   unsigned bitsPerSample);
    unsigned GetChannels() const;
    unsigned GetSampleRate() const;
    unsigned GetSampleSize() const;
    PBoolean SetBuffers(PINDEX size, PINDEX count);
    PBoolean GetBuffers(PINDEX & size, PINDEX & count);
    PBoolean PlaySound(const PSound & sound, PBoolean wait);
    PBoolean PlayFile(const PFilePath & filename, PBoolean wait);
    PBoolean HasPlayCompleted();
    PBoolean WaitForPlayCompletion();
    PBoolean RecordSound(PSound & sound);
    PBoolean RecordFile(const PFilePath & filename);
    PBoolean StartRecording();
    PBoolean IsRecordBufferFull();
    PBoolean AreAllRecordBuffersFull();
    PBoolean WaitForRecordBufferFull();
    PBoolean WaitForAllRecordBuffersFull();
    PBoolean Abort();
    PBoolean SetVolume(unsigned newVal);
    PBoolean GetVolume(unsigned &devVol);
    PBoolean SetMute(bool mute);
    PBoolean GetMute(bool & mute);

  public:
    // Overrides from class PChannel
    virtual PString GetName() const;
      // Return the name of the channel.

      
    PString GetErrorText(ErrorGroup group = NumErrorGroups) const;
    // Get a text form of the last error encountered.

    PBoolean SetFormat(const PWaveFormat & format);

    PBoolean Open(const PString & device, Directions dir,const PWaveFormat & format);
    // Open with format other than PCM

  protected:
    PString      deviceName;
    Directions   direction;
    HWAVEIN      hWaveIn;
    HWAVEOUT     hWaveOut;
    HMIXER       hMixer;
    MIXERCONTROL volumeControl;
    MIXERCONTROL muteControl;
    HANDLE       hEventDone;
    PWaveFormat  waveFormat;
    bool         opened;

    PWaveBufferArray buffers;
    PINDEX           bufferIndex;
    PINDEX           bufferByteOffset;
    PMutex           bufferMutex;

  private:
    PBoolean OpenDevice(unsigned id);
    PBoolean GetDeviceID(const PString & device, Directions dir, unsigned& id);
};


#endif

// End Of File ///////////////////////////////////////////////////////////////
