/*
 * main.cxx - do wave file things.
 *
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

#include <ptlib.h>
#include <ptclib/pwavfile.h>
#include <ptclib/dtmf.h>
#include <ptlib/sound.h>
#include <ptlib/pprocess.h>

#define SAMPLES 64000  

class WAVFileTest : public PProcess
{
  public:
    WAVFileTest()
    : PProcess() { }
    void Main();
    void Create(PArgList & args);
    void Play(PArgList & args);
    void Record(PArgList & args);
};

PCREATE_PROCESS(WAVFileTest)


void WAVFileTest::Main()
{
  PArgList & args = GetArguments();
  args.Parse("hpr:c:F:C:R:d:D:v:");

  if (args.GetCount() > 0) {
    if (args.HasOption('c')) {
      Create(args);
      return;
    }

    if (args.HasOption('p')) {
      Play(args);
      return;
    }

    if (args.HasOption('r')) {
      Record(args);
      return;
    }
  }

  cout << "usage: wavfile { -r | -p | -c tones } [ options ] filename\n"
          "   -p          Play WAV file\n"
          "   -r time     Record WAV file for number of seconds\n"
          "   -c tones    Create WAV file from generated tones\n"
          "\n"
          "Options:\n"
          "   -F format   File format, e.g. \"PCM-16\" (create/record only)\n"
          "   -C channels Number of channels (record only)\n"
          "   -R rate     Sample rate (record only)\n"
          "   -d dev      Use device name for sound channel record/playback\n"
          "   -D drv      Use driver name for sound channel record/playback\n"
          "   -v vol      Set sound device to vol (0..100)\n"
       << endl;
}


void WAVFileTest::Create(PArgList & args)
{
  PWAVFile file(args[0], PFile::WriteOnly);
  if (!file.IsOpen()) {
    cout << "Cannot create wav file " << args[0] << endl;
    return;
  }

  if (args.HasOption('F'))
    file.SetFormat(args.GetOptionString('F'));

  if (args.HasOption('C'))
    file.SetChannels(args.GetOptionString('C').AsUnsigned());

  if (args.HasOption('R'))
    file.SetSampleRate(args.GetOptionString('R').AsUnsigned());

  PTones toneData(args.GetOptionString('c'), PTones::MaxVolume, file.GetSampleRate());
  file.Write((const short *)toneData, toneData.GetSize()*sizeof(short));
}


void WAVFileTest::Play(PArgList & args)
{
  PWAVFile file(args[0], PFile::ReadOnly, PFile::MustExist, PWAVFile::fmt_NotKnown);
  if (!file.IsOpen()) {
    cout << "Cannot open " << args[0] << endl;
    return;
  }

  PINDEX dataLen = file.GetDataLength();
  PINDEX hdrLen  = file.GetHeaderLength();
  PINDEX fileLen = file.GetLength();

  cout << "Format:       " << file.GetFormat() << " (" << file.GetFormatString() << ")" << "\n"
       << "Channels:     " << file.GetChannels() << "\n"
       << "Sample rate:  " << file.GetSampleRate() << "\n"
       << "Bytes/sec:    " << file.GetBytesPerSecond() << "\n"
       << "Bits/sample:  " << file.GetSampleSize() << "\n"
       << "\n"
       << "Hdr length :  " << hdrLen << endl
       << "Data length:  " << dataLen << endl
       << "File length:  " << fileLen << " (" << hdrLen + dataLen << ")" << endl
       << endl;

  PBYTEArray data;
  if (!file.Read(data.GetPointer(dataLen), dataLen) || (file.GetLastReadCount() != dataLen)) {
    cout << "error: cannot read " << dataLen << " bytes of WAV data" << endl;
    return;
  }

  PSoundChannel * sound = PSoundChannel::CreateOpenedChannel(args.GetOptionString('D'),
                                                             args.GetOptionString('d'),
                                                             PSoundChannel::Player,
                                                             file.GetChannels(),
                                                             file.GetSampleRate(),
                                                             file.GetSampleSize());
  if (sound == NULL) {
    cout << "Failed to create sound channel." << endl;
    return;
  }

  sound->SetVolume(args.GetOptionString('v', "50").AsUnsigned());

  if (!sound->SetBuffers(SAMPLES, 2)) {
    cout << "Failed to set samples to " << SAMPLES << " and 2 buffers. End program now." << endl;
    return;
  }

  if (!sound->Write((const BYTE *)data, data.GetSize())) {
    cout << "error: write to audio device failed" << endl;
    return;
  }

  sound->WaitForPlayCompletion();
  delete sound;
}


void WAVFileTest::Record(PArgList & args)
{
  PWAVFile file(args[0], PFile::WriteOnly);
  if (!file.IsOpen()) {
    cout << "Cannot open " << args[0] << endl;
    return;
  }

  if (args.HasOption('F'))
    file.SetFormat(args.GetOptionString('F'));

  if (args.HasOption('C'))
    file.SetChannels(args.GetOptionString('C').AsUnsigned());

  if (args.HasOption('R'))
    file.SetSampleRate(args.GetOptionString('R').AsUnsigned());

  PSoundChannel * sound = PSoundChannel::CreateOpenedChannel(args.GetOptionString('D'),
                                                             args.GetOptionString('d'),
                                                             PSoundChannel::Recorder,
                                                             file.GetChannels(),
                                                             file.GetSampleRate(),
                                                             file.GetSampleSize());
  if (sound == NULL) {
    cout << "Failed to create sound channel." << endl;
    return;
  }

  sound->SetVolume(args.GetOptionString('v', "50").AsUnsigned());

  PSimpleTimer timer(0, args.GetOptionString('r').AsUnsigned());
  cout << "Recording WAV file for " << timer << " seconds ..." << endl;
  while (timer.IsRunning()) {
    BYTE buffer[8192];
    if (!sound->Read(buffer, sizeof(buffer))) {
      cout << "Error reading sound channel: " << sound->GetErrorText() << endl;
      break;
    }
    if (!file.Write(buffer, sound->GetLastReadCount())) {
      cout << "Error writing WAV file: " << file.GetErrorText() << endl;
      break;
    }
  }

  delete sound;
}
