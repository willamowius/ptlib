/*
 * ptts.cxx
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

#ifdef __GNUC__
#pragma implementation "ptts.h"
#endif

#include <ptlib.h>

#include "ptbuildopts.h"

#if P_TTS

#include <ptclib/ptts.h>

#include <ptlib/pipechan.h>
#include <ptclib/ptts.h>



#if P_SAPI

////////////////////////////////////////////////////////////
//
// Text to speech using Microsoft's Speech API (SAPI)

#ifdef _MSC_VER
  #pragma comment(lib, "sapi.lib")
  #pragma message("SAPI support enabled")
#endif

#ifndef _WIN32_DCOM
  #define _WIN32_DCOM 1
#endif

#include <ptlib/msos/ptlib/pt_atl.h>

#include <sphelper.h>


class PTextToSpeech_SAPI : public PTextToSpeech
{
    PCLASSINFO(PTextToSpeech_SAPI, PTextToSpeech);
  public:
    PTextToSpeech_SAPI();

    // overrides
    PStringArray GetVoiceList();
    PBoolean SetVoice(const PString & voice);

    PBoolean SetRate(unsigned rate);
    unsigned GetRate();

    PBoolean SetVolume(unsigned volume);
    unsigned GetVolume();

    PBoolean OpenFile(const PFilePath & fn);
    PBoolean OpenChannel(PChannel * channel);
    PBoolean IsOpen()     { return m_opened; }

    PBoolean Close();
    PBoolean Speak(const PString & str, TextType hint);

  protected:
    CComPtr<ISpVoice>  m_cpVoice;
    CComPtr<ISpStream> m_cpWavStream;
    bool               m_opened;
    PString            m_CurrentVoice;
};

PFACTORY_CREATE(PFactory<PTextToSpeech>, PTextToSpeech_SAPI, "Microsoft SAPI", false);


#define new PNEW


PTextToSpeech_SAPI::PTextToSpeech_SAPI()
  : m_opened(false)
{
  ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
  PTRACE(5, "TTS\tPTextToSpeech_SAPI constructed");
}


PBoolean PTextToSpeech_SAPI::OpenChannel(PChannel *)
{
  Close();
  return false;
}


PBoolean PTextToSpeech_SAPI::OpenFile(const PFilePath & fn)
{
  Close();

  HRESULT hr = m_cpVoice.CoCreateInstance(CLSID_SpVoice);
  if (FAILED(hr))
    return false;

  CSpStreamFormat wavFormat;
  wavFormat.AssignFormat(SPSF_8kHz16BitMono);

  PWideString wfn = fn;
  hr = SPBindToFile(wfn, SPFM_CREATE_ALWAYS, &m_cpWavStream, &wavFormat.FormatId(), wavFormat.WaveFormatExPtr()); 
  if (FAILED(hr)) {
    m_cpWavStream.Release();
    return false;
  }

  hr = m_cpVoice->SetOutput(m_cpWavStream, true);
  m_opened = SUCCEEDED(hr);
  return m_opened;
}


PBoolean PTextToSpeech_SAPI::Close()
{
  if (!m_opened)
    return false;

  m_cpVoice->WaitUntilDone(INFINITE);
  m_cpWavStream.Release();
  m_cpVoice.Release();

  m_opened = false;
  return true;
}


PBoolean PTextToSpeech_SAPI::Speak(const PString & text, TextType hint)
{
  if (!IsOpen())
    return false;

  PWideString wtext = text;

  // do various things to the string, depending upon the hint
  switch (hint) {
    case Digits:
      break;
    default:
      break;
  };

  HRESULT hr = S_OK;

  if (m_CurrentVoice != NULL && !m_CurrentVoice.IsEmpty()) {
    PTRACE(4, "SAPI\tTrying to set voice \"" << m_CurrentVoice << "\""
              " of voices: " << setfill(',') << GetVoiceList());

    //Enumerate voice tokens with attribute "Name=<specified voice>"
    CComPtr<IEnumSpObjectTokens> cpEnum;
    hr = SpEnumTokens(SPCAT_VOICES, m_CurrentVoice.AsUCS2(), NULL, &cpEnum);
    if (FAILED(hr)) {
      PTRACE(2, "SAPI\tSpEnumTokens failed: " << hr);
    }
    else {
      //Get the closest token
      CComPtr<ISpObjectToken> cpVoiceToken;
      hr = cpEnum->Next(1, &cpVoiceToken, NULL);
      if (FAILED(hr)) {
        PTRACE(2, "SAPI\tEnumerate next failed: " << hr);
      }
      else {
        //set the voice
        hr = m_cpVoice->SetVoice(cpVoiceToken);
        if (FAILED(hr)) {
          PTRACE(2, "SAPI\tSetVoice failed: " << hr);
        }
        else {
          PTRACE(4, "SAPI\tSetVoice(" << m_CurrentVoice << ") OK!");
        }
      }
    } 
  }

  PTRACE(4, "SAPI\tSpeaking...");
  hr = m_cpVoice->Speak(wtext, SPF_DEFAULT, NULL);
  if (SUCCEEDED(hr))
    return true;

  PTRACE(2, "SAPI\tError speaking text: " << hr);
  return false;
}


PStringArray PTextToSpeech_SAPI::GetVoiceList()
{
  PStringArray voiceList;

  CComPtr<ISpObjectToken> cpVoiceToken;
  CComPtr<IEnumSpObjectTokens> cpEnum;
  ULONG ulCount = 0;

  //Enumerate the available voices 
  HRESULT hr = SpEnumTokens(SPCAT_VOICES, NULL, NULL, &cpEnum);

  // Get the number of voices
  if (SUCCEEDED(hr)) {
    hr = cpEnum->GetCount(&ulCount);
    PTRACE(4, "SAPI\tFound " << ulCount << " voices..");
  }
  // Obtain a list of available voice tokens, set the voice to the token, and call Speak
  while (SUCCEEDED(hr) && ulCount--) {

    cpVoiceToken.Release();

    if (SUCCEEDED(hr))
      hr = cpEnum->Next(1, &cpVoiceToken, NULL );

    if (SUCCEEDED(hr)) {
      voiceList.AppendString("voice");
      PTRACE(4, "SAPI\tFound voice:" << cpVoiceToken);
    }
  } 

  return voiceList;
}

PBoolean PTextToSpeech_SAPI::SetVoice(const PString & voice)
{
  m_CurrentVoice = voice;
  return true;
}

PBoolean PTextToSpeech_SAPI::SetRate(unsigned)
{
  return false;
}

unsigned PTextToSpeech_SAPI::GetRate()
{
  return 0;
}

PBoolean PTextToSpeech_SAPI::SetVolume(unsigned)
{
  return false;
}

unsigned PTextToSpeech_SAPI::GetVolume()
{
  return 0;
}

#else

  #ifdef _MSC_VER
    #pragma message("SAPI support DISABLED")
  #endif

#endif // P_SAPI


#ifndef _WIN32_WCE

////////////////////////////////////////////////////////////
//
//  Generic text to speech using Festival
//

#undef new

class PTextToSpeech_Festival : public PTextToSpeech
{
  PCLASSINFO(PTextToSpeech_Festival, PTextToSpeech);
  public:
    PTextToSpeech_Festival();
    ~PTextToSpeech_Festival();

    // overrides
    PStringArray GetVoiceList();
    PBoolean SetVoice(const PString & voice);

    PBoolean SetRate(unsigned rate);
    unsigned GetRate();

    PBoolean SetVolume(unsigned volume);
    unsigned GetVolume();

    PBoolean OpenFile(const PFilePath & fn);
    PBoolean OpenChannel(PChannel * channel);
    PBoolean IsOpen()    { return opened; }

    PBoolean Close();
    PBoolean Speak(const PString & str, TextType hint);

  protected:
    PBoolean Invoke(const PString & str, const PFilePath & fn);

    PMutex mutex;
    PBoolean opened;
    PBoolean usingFile;
    PString text;
    PFilePath path;
    unsigned volume, rate;
};

#define new PNEW

PFACTORY_CREATE(PFactory<PTextToSpeech>, PTextToSpeech_Festival, "Festival", false);

PTextToSpeech_Festival::PTextToSpeech_Festival()
{
  PWaitAndSignal m(mutex);
  usingFile = opened = false;
  rate = 8000;
  volume = 100;
  PTRACE(4, "TTS\tPTextToSpeech_Festival constructed");
}


PTextToSpeech_Festival::~PTextToSpeech_Festival()
{
  PWaitAndSignal m(mutex);
}

PBoolean PTextToSpeech_Festival::OpenChannel(PChannel *)
{
  PWaitAndSignal m(mutex);

  Close();
  usingFile = false;
  opened = false;

  return true;
}


PBoolean PTextToSpeech_Festival::OpenFile(const PFilePath & fn)
{
  PWaitAndSignal m(mutex);

  Close();
  usingFile = true;
  path = fn;
  opened = true;

  PTRACE(3, "TTS\tWriting speech to " << fn);

  return true;
}

PBoolean PTextToSpeech_Festival::Close()
{
  PWaitAndSignal m(mutex);

  if (!opened)
    return true;

  PBoolean stat = false;

  if (usingFile)
    stat = Invoke(text, path);

  text = PString();

  opened = false;

  return stat;
}


PBoolean PTextToSpeech_Festival::Speak(const PString & ostr, TextType hint)
{
  PWaitAndSignal m(mutex);

  if (!IsOpen()) {
    PTRACE(2, "TTS\tAttempt to speak whilst engine not open");
    return false;
  }

  PString str = ostr;

  // do various things to the string, depending upon the hint
  switch (hint) {
    case Digits:
    default:
    ;
  };

  if (usingFile) {
    PTRACE(3, "TTS\tSpeaking " << ostr);
    text = text & str;
    return true;
  }

  PTRACE(1, "TTS\tStream mode not supported for Festival");

  return false;
}

PStringArray PTextToSpeech_Festival::GetVoiceList()
{
  PStringArray voiceList;
  voiceList.AppendString("default");
  return voiceList;
}

PBoolean PTextToSpeech_Festival::SetVoice(const PString & v)
{
  return v == "default";
}

PBoolean PTextToSpeech_Festival::SetRate(unsigned v)
{
  rate = v;
  return true;
}

unsigned PTextToSpeech_Festival::GetRate()
{
  return rate;
}

PBoolean PTextToSpeech_Festival::SetVolume(unsigned v)
{
  volume = v;
  return true;
}

unsigned PTextToSpeech_Festival::GetVolume()
{
  return volume;
}

PBoolean PTextToSpeech_Festival::Invoke(const PString & otext, const PFilePath & fname)
{
  PString text = otext;
  text.Replace('\n', ' ', true);
  text.Replace('\"', '\'', true);
  text.Replace('\\', ' ', true);
  text = "\"" + text + "\"";

  PString cmdLine = "echo " + text + " | ./text2wave -F " + PString(PString::Unsigned, rate) + " -otype riff > " + fname;

  PPipeChannel cmd;
  if (!cmd.Open(cmdLine, PPipeChannel::ReadWriteStd)) {
    PTRACE(1, "TTS\tCannot execute command " << cmd);
    return false;
  }

  PTRACE(3, "TTS\tCreating " << fname << " using " << cmdLine);
  cmd.Execute();
  int code = -1;
  code = cmd.WaitForTermination();
  if (code >= 0) {
    PTRACE(4, "TTS\tdata generated");
  } else {
    PTRACE(1, "TTS\tgeneration failed");
  }

  return code == 0;
}

#endif // _WIN32_WCE

#endif // P_TTS


// End Of File ///////////////////////////////////////////////////////////////
