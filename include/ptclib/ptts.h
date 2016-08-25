/*
 * ptts.h
 *
 * Text To Speech classes
 *
 * Portable Windows Library
 *
 * Copyright (c) 2002 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_PTTS_H
#define PTLIB_PTTS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptbuildopts.h>

#if P_TTS

#include <ptlib/pfactory.h>


class PTextToSpeech : public PObject
{
  PCLASSINFO(PTextToSpeech, PObject);
  public:
    enum TextType {
      Default,
      Literal,
      Digits,
      Number,
      Currency,
      Time,
      Date,
      DateAndTime,
      Phone,
      IPAddress,
      Duration,
      Spell
    };

    virtual PStringArray GetVoiceList() = 0;
    virtual PBoolean SetVoice(const PString & voice) = 0;

    virtual PBoolean SetRate(unsigned rate) = 0;
    virtual unsigned GetRate() = 0;

    virtual PBoolean SetVolume(unsigned volume) = 0;
    virtual unsigned GetVolume() = 0;

    virtual PBoolean OpenFile(const PFilePath & fn) = 0;
    virtual PBoolean OpenChannel(PChannel * chanel) = 0;
    virtual PBoolean IsOpen() = 0;

    virtual PBoolean Close() = 0;
    virtual PBoolean Speak(const PString & text, TextType hint = Default) = 0;
};

#if P_SAPI
  PFACTORY_LOAD(PTextToSpeech_SAPI);
#endif

#ifndef _WIN32_WCE
  PFACTORY_LOAD(PTextToSpeech_Festival);
#endif


#endif // P_TTS

#endif // PTLIB_PTTS_H


// End Of File ///////////////////////////////////////////////////////////////
