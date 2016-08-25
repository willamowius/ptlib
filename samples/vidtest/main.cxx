/*
 * main.cxx
 *
 * PWLib application source file for vidtest
 *
 * Main program entry point.
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
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include "precompile.h"
#include "main.h"
#include "version.h"


PCREATE_PROCESS(VidTest);

#include  <ptlib/video.h>
#include  <ptlib/vconvert.h>
#include  <ptclib/vsdl.h>


VidTest::VidTest()
  : PProcess("PwLib Video Example", "vidtest", 1, 0, ReleaseCode, 0)
  , m_grabber(NULL)
  , m_display(NULL)
  , m_secondary(NULL)
{
}


void VidTest::Main()
{
  PArgList & args = GetArguments();

  args.Parse("h-help."
             "-input-driver:"
             "I-input-device:"
             "-input-format:"
             "-input-channel:"
             "-output-driver:"
             "O-output-device:"
             "T-time:"
#if PTRACING
             "o-output:"             "-no-output."
             "t-trace."              "-no-trace."
#endif
       );

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  if (args.HasOption('h')) {
    PError << "usage: vidtest [options] [descriptor ...]\n"
              "\n"
              "Available options are:\n"
              "   --help                 : print this help message.\n"
              "   --input-driver  drv    : video grabber driver.\n"
              "   -I --input-device dev  : video grabber device.\n"
              "   --input-format  fmt    : video grabber format (\"pal\"/\"ntsc\")\n"
              "   --input-channel num    : video grabber channel.\n"
              "   --output-driver drv    : video display driver to use.\n"
              "   -O --output-device dev : video display device to use.\n"
              "   -T --time              : time in seconds to run test, no command line\n"
#if PTRACING
              "   -o or --output file   : file name for output of log messages\n"       
              "   -t or --trace         : degree of verbosity in log (more times for more detail)\n"     
#endif
              "\n"
              "The dscriptor arguments are of the form:\n"
              "    [colour ':' ] size [ '@' rate][ \"/crop\" ].\n"
              "The size component is size is one of the prefined names \"qcif\", \"cif\", \"vga\"\n"
              "etc or WxH, e.g \"640x480\". The fmt string is the colour format such as\n"
              "\"RGB32\", \"YUV420P\" etc. The rate field is a simple integer from 1 to 100.\n"
              "The crop field is one of \"scale\", \"resize\" (synonym for \"scale\"), \"centre\",\n"
              "\"center\", \"topleft\" or \"crop\" (synonym for \"topleft\"). Note no spaces are\n"
              "allowed in the descriptor.\n"
              "\n"
              "If the physical device can do the specified formats (input device for first\n"
              "format, output device for last format) then no video converter will be used.\n"
              "If they cannot do the format natively, or there are intermediate formats, then\n"
              "video converters are provided.\n"
              "\n"
              " e.g. ./vidtest --input-device fake --input-channel 2 YUV420P/qcif" << endl << endl;
    return;
  }


  /////////////////////////////////////////////////////////////////////

  PString inputDriverName = args.GetOptionString("input-driver");
  PString inputDeviceName = args.GetOptionString("input-device");
  m_grabber = PVideoInputDevice::CreateOpenedDevice(inputDriverName, inputDeviceName, FALSE);
  if (m_grabber == NULL) {
    cerr << "Cannot use ";
    if (inputDriverName.IsEmpty() && inputDriverName.IsEmpty())
      cerr << "default ";
    cerr << "video grab";
    if (!inputDriverName)
      cerr << ", driver \"" << inputDriverName << '"';
    if (!inputDeviceName)
      cerr << ", device \"" << inputDeviceName << '"';
    else
      cerr << ", default device";
    cerr << ", device name must be one of:\n";
    PStringList devices = PVideoInputDevice::GetDriversDeviceNames("*");
    for (PINDEX i = 0; i < devices.GetSize(); i++)
      cerr << "   " << devices[i] << '\n';
    cerr << endl;
    return;
  }

  cout << "Grabber ";
  if (!inputDriverName.IsEmpty())
    cout << "driver \"" << inputDriverName << "\" and ";
  cout << "device \"" << m_grabber->GetDeviceName() << "\" opened." << endl;

  PVideoInputDevice::Capabilities caps;
  if (m_grabber->GetDeviceCapabilities(&caps)) {
    cout << "Grabber " << inputDeviceName << " capabilities." << endl;
    for (std::list<PVideoFrameInfo>::const_iterator r = caps.framesizes.begin(); r != caps.framesizes.end(); ++r)
      cout << "    " << r->GetColourFormat() << ' ' << r->GetFrameWidth() << 'x' << r->GetFrameHeight() << ' ' << r->GetFrameRate() << "fps\n";
    cout << endl;
  }
  else
    cout << "Input device " << inputDeviceName << " capabilities not available." << endl;
    
  if (args.HasOption("input-format")) {
    PVideoDevice::VideoFormat format;
    PCaselessString formatString = args.GetOptionString("input-format");
    if (formatString == "PAL")
      format = PVideoDevice::PAL;
    else if (formatString == "NTSC")
      format = PVideoDevice::NTSC;
    else if (formatString == "SECAM")
      format = PVideoDevice::SECAM;
    else if (formatString == "Auto")
      format = PVideoDevice::Auto;
    else {
      cerr << "Illegal video input format name \"" << formatString << '"' << endl;
      return;
    }
    if (!m_grabber->SetVideoFormat(format)) {
      cerr << "Video input device could not be set to format \"" << formatString << '"' << endl;
      return;
    }
  }
  cout << "Grabber input format set to " << m_grabber->GetVideoFormat() << endl;

  if (args.HasOption("input-channel")) {
    int videoInput = args.GetOptionString("input-channel").AsInteger();
    if (!m_grabber->SetChannel(videoInput)) {
      cerr << "Video input device could not be set to channel " << videoInput << endl;
    return;
  }
  }
  cout << "Grabber input channel set to " << m_grabber->GetChannel() << endl;


  /////////////////////////////////////////////////////////////////////

  PString outputDriverName = args.GetOptionString("output-driver");
  PString outputDeviceName = args.GetOptionString("output-device");
  m_display = PVideoOutputDevice::CreateOpenedDevice(outputDriverName, outputDeviceName, FALSE);
  if (m_display == NULL) {
    cerr << "Cannot use ";
    if (outputDriverName.IsEmpty() && outputDriverName.IsEmpty())
      cerr << "default ";
    cerr << "video display";
    if (!outputDriverName)
      cerr << ", driver \"" << outputDriverName << '"';
    if (!outputDeviceName)
      cerr << ", device \"" << outputDeviceName << '"';
    else
      cerr << ", default device";
    cerr << ", device name must be one of:\n";
    PStringList devices = PVideoOutputDevice::GetDriversDeviceNames("*");
    for (PINDEX i = 0; i < devices.GetSize(); i++)
      cerr << "   " << devices[i] << '\n';
    cerr << endl;
    return;
  }

  cout << "Display ";
  if (!outputDriverName.IsEmpty())
    cout << "driver \"" << outputDriverName << "\" and ";
  cout << "device \"" << m_display->GetDeviceName() << "\" opened." << endl;


  /////////////////////////////////////////////////////////////////////

  PVideoFrameInfo grabberInfo = *m_grabber;
  PVideoFrameInfo displayInfo = *m_display;

  if (args.GetCount() == 0)
    displayInfo.SetColourFormat(grabberInfo.GetColourFormat());
  else {
    if (!grabberInfo.Parse(args[0])) {
      cerr << "Could not parse argument \"" << args[0] << '"' << endl;
      return;
    }

    if (!displayInfo.Parse(args[args.GetCount()-1])) {
      cerr << "Could not parse argument \"" << args[args.GetCount()-1] << '"' << endl;
      return;
    }

    PVideoFrameInfo src = grabberInfo;
    PVideoFrameInfo dst;
    for (PINDEX i = 1; i < args.GetCount()-1; i++) {
      if (args[i-1] *= args[i])
        continue;

      if (!dst.Parse(args[i])) {
        cerr << "Could not parse argument \"" << args[i] << '"' << endl;
        return;
      }

      PColourConverter * converter = PColourConverter::Create(src, dst);
      if (converter == NULL) {
        cerr << "Could not create converter from " << src.GetColourFormat() << " to " << dst.GetColourFormat() << endl;
        return;
      }

      m_converters.Append(converter);
      src = dst;
    }
  }


  /////////////////////////////////////////////////////////////////////

  if (!m_grabber->SetColourFormatConverter(grabberInfo.GetColourFormat()) ) {
    cerr << "Video input device could not be set to colour format \"" << grabberInfo.GetColourFormat() << '"' << endl;
    return;
  }

  cout << "Grabber colour format set to " << m_grabber->GetColourFormat() << " (";
  if (grabberInfo.GetColourFormat() == m_grabber->GetColourFormat())
    cout << "native";
  else
    cout << "converted to " << grabberInfo.GetColourFormat();
  cout << ')' << endl;


  if (!m_grabber->SetFrameSizeConverter(grabberInfo.GetFrameWidth(), grabberInfo.GetFrameHeight(), grabberInfo.GetResizeMode())) {
    cerr << "Video input device could not be set to size " << grabberInfo.GetFrameWidth() << 'x' << grabberInfo.GetFrameHeight() << endl;
    return;
  }
  cout << "Grabber frame size set to " << m_grabber->GetFrameWidth() << 'x' << m_grabber->GetFrameHeight() << endl;


  if (!m_grabber->SetFrameRate(grabberInfo.GetFrameRate())) {
    cerr << "Video input device could not be set to frame rate " << grabberInfo.GetFrameRate() << endl;
    return;
  }
  cout << "Grabber frame rate set to " << m_grabber->GetFrameRate() << endl;


  /////////////////////////////////////////////////////////////////////

  if (!m_display->SetColourFormatConverter(displayInfo.GetColourFormat())) {
    cerr << "Video output device could not be set to colour format \"" << displayInfo.GetColourFormat() << '"' << endl;
    return;
  }

  cout << "Diaplay colour format set to " << m_display->GetColourFormat() << " (";
  if (displayInfo.GetColourFormat() == m_display->GetColourFormat())
    cout << "native";
  else
    cout << "converted from " << displayInfo.GetColourFormat();
  cout << ')' << endl;


  if  (!m_display->SetFrameSizeConverter(displayInfo.GetFrameWidth(), displayInfo.GetFrameHeight(), displayInfo.GetResizeMode())) {
    cerr << "Video output device could not be set to size " << displayInfo.GetFrameWidth() << 'x' << displayInfo.GetFrameHeight() << endl;
    return;
  }

  cout << "Display frame size set to " << m_display->GetFrameWidth() << 'x' << m_display->GetFrameHeight() << endl;


  /////////////////////////////////////////////////////////////////////

  PThread::Create(PCREATE_NOTIFIER(GrabAndDisplay), 0,
                  PThread::NoAutoDeleteThread, PThread::NormalPriority,
                  "GrabAndDisplay");

  if (args.HasOption('T'))
    PThread::Sleep(args.GetOptionString('T').AsUnsigned()*1000);
  else {
    // command line
    for (;;) {

      // display the prompt
      cout << "vidtest> " << flush;
      PCaselessString cmd;
      cin >> cmd;

      if (cin.eof() || cmd == "q" || cmd == "x" || cmd == "quit" || cmd == "exit")
        break;

      if (cmd == "stop") {
        if (!m_grabber->Stop())
          cout << "\nCould not stop video input device" << endl;
        continue;
      }

      if (cmd == "start") {
        if (!m_grabber->Start())
          cout << "\nCould not stop video input device" << endl;
        continue;
      }

      if (cmd == "secondary") {
        if (m_secondary != NULL) {
          delete m_secondary;
          m_secondary = NULL;
        }
        else {
          m_secondary = PVideoOutputDevice::CreateOpenedDevice(PString::Empty(), m_display->GetDeviceName());
          if (m_secondary == NULL)
            cout << "\nCould not start secondary video output device" << endl;
        }
        continue;
      }

      if (cmd == "fg") {
        if (!m_grabber->SetVFlipState(!m_grabber->GetVFlipState()))
          cout << "\nCould not toggle Vflip state of video input device" << endl;
        continue;
      }

      if (cmd == "fd") {
        if (!m_display->SetVFlipState(!m_display->GetVFlipState()))
          cout << "\nCould not toggle Vflip state of video output device" << endl;
        continue;
      }

      unsigned width, height;
      if (PVideoFrameInfo::ParseSize(cmd, width, height)) {
        m_pauseGrabAndDisplay.Signal();
        if  (!m_grabber->SetFrameSizeConverter(width, height))
          cout << "Video input device could not be set to size " << width << 'x' << height << endl;
        if  (!m_display->SetFrameSizeConverter(width, height))
          cout << "Video output device could not be set to size " << width << 'x' << height << endl;
        m_resumeGrabAndDisplay.Signal();
        continue;
      }

      cout << "Select:\n"
              "  stop   : Stop grabbing\n"
              "  start  : Start grabbing\n"
              "  fg     : Flip video input top to bottom\n"
              "  fd     : Flip video output top to bottom\n"
              "  qcif   : Set size of grab & display to qcif\n"
              "  cif    : Set size of grab & display to cif\n"
              "  WxH    : Set size of grab & display W by H\n"
              "  Q or X : Exit program\n" << endl;
    } // end for
  }

  cout << "Exiting." << endl;
  m_exitGrabAndDisplay.Signal();

  delete m_secondary;
  delete m_display;
  delete m_grabber;
}


void VidTest::GrabAndDisplay(PThread &, INT)
{
  std::vector<PBYTEArray> frames;
  unsigned frameCount = 0;
  bool oldGrabberState = true;
  bool oldDisplayState = true;
  bool oldSecondaryState = true;

  if (!m_grabber->Start()) {
    cout << "Could not start video grabber!" << endl;
    return;
  }

  if (!m_display->Start()) {
    cout << "Could not start video display!" << endl;
    return;
  }

  frames.resize((m_converters.GetSize()+1));

  PTimeInterval startTick = PTimer::Tick();
  while (!m_exitGrabAndDisplay.Wait(0)) {

    bool grabberState = m_grabber->GetFrame(frames.front());
    if (oldGrabberState != grabberState) {
      oldGrabberState = grabberState;
      cerr << "Frame grab " << (grabberState ? "restored." : "failed!") << endl;
    }

    unsigned width, height;
    m_grabber->GetFrameSize(width, height);

    for (PINDEX frameIndex = 0; frameIndex < m_converters.GetSize(); ++frameIndex) {
      if (m_converters[frameIndex].Convert(frames[frameIndex],
                                            frames[frameIndex+1].GetPointer(m_converters[frameIndex].GetMaxDstFrameBytes())))
        m_converters[frameIndex].GetDstFrameSize(width, height);
      else
        cerr << "Frame conversion failed!" << endl;
    }

    m_display->SetFrameSize(width, height);

    bool displayState = m_display->SetFrameData(0, 0, width, height, frames.back());
    if (oldDisplayState != displayState) {
      oldDisplayState = displayState;
      cerr << "Frame display " << (displayState ? "restored." : "failed!") << endl;
    }

    if (m_secondary != NULL) {
      m_secondary->SetFrameSize(width, height);

      displayState = m_secondary->SetFrameData(0, 0, width, height, frames.back());
      if (oldSecondaryState != displayState) {
        oldSecondaryState = displayState;
        cerr << "Secondary Frame display " << (displayState ? "restored." : "failed!") << endl;
      }
    }

    if (m_pauseGrabAndDisplay.Wait(0)) {
      m_pauseGrabAndDisplay.Acknowledge();
      m_resumeGrabAndDisplay.Wait();
    }

    frameCount++;
  }

  PTimeInterval duration = PTimer::Tick() - startTick;
  cout << frameCount << " frames over " << duration << " seconds at " << (frameCount*1000.0/duration.GetMilliSeconds()) << " fps." << endl;
  m_exitGrabAndDisplay.Acknowledge();
}



// End of File ///////////////////////////////////////////////////////////////
