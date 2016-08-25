/*
 * pwavfiledev.cxx
 *
 * Sound file device declaration
 *
 * Portable Windows Library
 *
 * Copyright (C) 2007 Post Increment
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
 * Robert Jongbloed <robertj@postincrement.com>
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_PWAVFILEDEV_H
#define PTLIB_PWAVFILEDEV_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>

#include <ptlib/sound.h>
#include <ptclib/pwavfile.h>
#include <ptclib/delaychan.h>

#if defined(P_WAVFILE)


///////////////////////////////////////////////////////////////////////////////////////////
//
// This class defines a sound channel device that reads audio from a raw WAV file
//

class PSoundChannel_WAVFile : public PSoundChannel
{
 PCLASSINFO(PSoundChannel_WAVFile, PSoundChannel);
 public:
    PSoundChannel_WAVFile();
    PSoundChannel_WAVFile(const PString &device,
                     PSoundChannel::Directions dir,
                     unsigned numChannels,
                     unsigned sampleRate,
                     unsigned bitsPerSample);
    ~PSoundChannel_WAVFile();
    static PStringArray GetDeviceNames(PSoundChannel::Directions = Player);
    PBoolean Open(
      const PString & device,
      Directions dir,
      unsigned numChannels,
      unsigned sampleRate,
      unsigned bitsPerSample
    );
    virtual PString GetName() const;
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
    PBoolean HasPlayCompleted();
    PBoolean WaitForPlayCompletion();
    PBoolean StartRecording();
    PBoolean IsRecordBufferFull();
    PBoolean AreAllRecordBuffersFull();
    PBoolean WaitForRecordBufferFull();
    PBoolean WaitForAllRecordBuffersFull();

protected:
    bool ReadSamples(void * data, PINDEX size);
    bool ReadSample(short & data);

    PWAVFile       m_WAVFile;
    PAdaptiveDelay m_Pacing;
    bool           m_autoRepeat;
    unsigned       m_sampleRate;
    PINDEX         m_bufferSize;
    PShortArray    m_sampleBuffer;
    PINDEX         m_samplePosition;
};


#endif // defined(P_WAVFILE)

#endif // PTLIB_PWAVFILEDEV_H


// End Of File ///////////////////////////////////////////////////////////////
