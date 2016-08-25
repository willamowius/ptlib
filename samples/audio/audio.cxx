//
// audio.cxx
//
// Roger Hardiman
//
//
/*
 * audio.cxx
 *
 * PWLib application source file for audio testing.
 *
 * Main program entry point.
 *
 * Copyright 2005 Roger Hardiman
 *
 * Copied by Derek Smithies, 1)Add soundtest code from ohphone.
 *                           2)Add headers.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <ptlib/pprocess.h>
#include "version.h"
#include "audio.h"
#include <ptclib/pwavfile.h>

Audio::Audio()
  : PProcess("Roger Hardiman & Derek Smithies code factory", "audio",
             MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{

}

PCREATE_PROCESS(Audio)

void Audio::Main()
{
  PArgList & args = GetArguments();
  args.Parse("r.    "
       "f.    "	     
       "h.    "
#if PTRACING
             "o-output:"             "-no-output."
             "t-trace."              "-no-trace."
#endif
       "p:    "
       "v.    "
       "w:    "
       "s:    ");
 
  if (args.HasOption('h')) {
    cout << "usage: audio " 
         << endl
         << "     -r        : report available sound devices" << endl
         << "     -f        : do a full duplex sound test on a sound device" << endl
         << "     -s  dev   : use this device in full duplex test " << endl
         << "     -h        : get help on usage " << endl
         << "     -p file   : play audio from the file out the specified sound device" << endl
         << "     -v        : report program version " << endl
	 << "     -w file   : write the captured audio to this file" << endl
#if PTRACING
         << "  -t --trace   : Enable trace, use multiple times for more detail" << endl
         << "  -o --output  : File for trace output, default is stderr" << endl
#endif
         << endl;
    return;
  }


  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);

  if (args.HasOption('v')) {
    cout << endl
         << "Product Name: " <<  (const char *)GetName() << endl
         << "Manufacturer: " <<  (const char *)GetManufacturer() << endl
         << "Version     : " <<  (const char *)GetVersion(PTrue) << endl
         << "System      : " <<  (const char *)GetOSName() << '-'
         <<  (const char *)GetOSHardware() << ' '
         <<  (const char *)GetOSVersion() << endl
         << endl;
    return;
  }
  

  cout << "Audio Test Program\n\n";

  PSoundChannel::Directions dir;
  PStringArray namesPlay, namesRecord;
  namesPlay = PSoundChannel::GetDeviceNames(PSoundChannel::Player);
  namesRecord = PSoundChannel::GetDeviceNames(PSoundChannel::Recorder);

  if (args.HasOption('r')) {
      cout << "List of play devices\n";
      for (PINDEX i = 0; i < namesPlay.GetSize(); i++)
	  cout << "  \"" << namesPlay[i] << "\"\n";      
      cout << "The default play device is \"" 
	   << PSoundChannel::GetDefaultDevice(PSoundChannel::Player) << "\"\n";
      
      cout << "List of Record devices\n";
      for (PINDEX i = 0; i < namesRecord.GetSize(); i++)
	  cout << "  \"" << namesRecord[i] << "\"\n";      
      cout << "The default record device is \"" 
	   << PSoundChannel::GetDefaultDevice(PSoundChannel::Recorder) << "\"\n\n";
  }

  // Display the mixer settings for the default devices (or device if specified)
  {
      PSoundChannel sound;
      dir = PSoundChannel::Player;
      devName = args.GetOptionString('s');
      if (devName.IsEmpty())
	  devName = PSoundChannel::GetDefaultDevice(dir);
      
      if (sound.Open(devName, dir)) {     
	  unsigned int vol;
	  if (sound.GetVolume(vol))
	      cout << "Play volume is (for " << devName << ")" << vol << endl;
	  else
	      cout << "Play volume cannot be obtained (for " << devName << ")" << endl;      
	  sound.Close();
      } else
	  cerr << "Failed to open play device (" 
	       << devName << ") to get volume settings" << endl;
  }
  {
      PSoundChannel sound;
      dir = PSoundChannel::Recorder;
      devName = args.GetOptionString('s');
      if (devName.IsEmpty())
	  devName = PSoundChannel::GetDefaultDevice(dir);
      if (sound.Open(devName, dir)) {     
	  unsigned int vol;
	  if (sound.GetVolume(vol))
	      cout << "Record volume is (for " << devName << ")" << vol << endl;
	  else
	      cout << "Record volume cannot be obtained (for " << devName << ")" << endl;
	  sound.Close();
      } else
	  cerr << "Failed to open record device (" 
	       << devName << ") to get volume settings" << endl;
  }

  if (args.HasOption('f')) {
    devName = args.GetOptionString('s');
    if (devName.IsEmpty())
      devName = PSoundChannel::GetDefaultDevice(PSoundChannel::Player);

    PString capturedAudio = args.GetOptionString('w');

    if (namesPlay.GetStringsIndex(devName) == P_MAX_INDEX) {
      cout << "could not find " << devName << " in list of available play devices - abort test" << endl;
      return;
    }

    if (namesRecord.GetStringsIndex(devName) == P_MAX_INDEX) {
      cout << "could not find " << devName << " in list of available record devices - abort test" << endl;
      return;
    }

    PTRACE(3, "Audio\tTest device " << devName);
    
    TestAudioDevice device;
    device.Test(capturedAudio);
    return;
  }

  if (args.HasOption('p')) {
      PString playFileName = args.GetOptionString('p');
      if (playFileName.IsEmpty()) {
	  cerr << "The p option requires an arguement - the name of the file to play" << endl;
	  cerr << "Terminating" << endl;
	  return;
      }

      PWAVFile audioFile;
      BYTE buffer[500];
      if (!audioFile.Open(playFileName)) {
	  cerr << "Failed to open the file \"" << playFileName << "\" to read audio from." << endl;
	  return;
      }

      PSoundChannel sound;
      dir = PSoundChannel::Player;
      devName = args.GetOptionString('s');
      if (devName.IsEmpty())
	  devName = PSoundChannel::GetDefaultDevice(dir);
      
      if (sound.Open(devName, PSoundChannel::Player, 1, 8000, 16)) {
	  PTRACE(3, "Open sound device " << devName << " to put audio to");
	  cerr << devName << " opened fine for playing to" << endl;
      } else {
	  cerr << "Failed to open play device (" 
	       << devName << ") for putting audio to speaker" << endl;
	  return;
      }
      sound.SetBuffers(480, 3);
      
      PINDEX readCounter = 0;
      while (audioFile.Read(buffer, 480)) {
	  PTRACE(3, "Read buffer " << readCounter << " from audio file " << playFileName);
	  readCounter++;
	  if (!sound.Write(buffer, 480)) {
	      cerr << " failed to write a buffer to sound device. Ending" << endl;
	      return;
	  }
      }
	  
  }
#if PTRACING
  if (args.GetOptionCount('t') > 0) {
    PTrace::ClearOptions(0);
    PTrace::SetLevel(0);
  }
#endif

}

////////////////////////////////////////////////////////////////////////////////

TestAudioDevice::~TestAudioDevice()
{
  AllowDeleteObjects();
  access.Wait();
  RemoveAll();
  endNow = PTrue;
  access.Signal();
  PThread::Sleep(100);
}

void TestAudioDevice::Test(const PString & captureFileName)
{
   endNow = PFalse;
   PConsoleChannel console(PConsoleChannel::StandardInput);

   AllowDeleteObjects(PFalse);
   PTRACE(3, "Start operation of TestAudioDevice");

   TestAudioRead reader(*this, captureFileName);
   TestAudioWrite writer(*this);   


   PStringStream help;
   help << "Select:\n";
   help << "  X   : Exit program\n"
        << "  Q   : Exit program\n"
        << "  {}  : Increase/reduce record volume\n"
        << "  []  : Increase/reduce playback volume\n"
        << "  H   : Write this help out\n"
	<< "  R   : Report the number of 30 ms long sound samples processed\n";
   
   PThread::Sleep(100);
   if (reader.IsTerminated() || writer.IsTerminated()) {
     reader.Terminate();
     writer.Terminate();
     
     goto endAudioTest;
   }

  for (;;) {
    // display the prompt
    cout << "(testing sound device for full duplex) Command ? " << flush;

    // terminate the menu loop if console finished
    char ch = (char)console.peek();
    if (console.eof()) {
      cout << "\nConsole gone - menu disabled" << endl;
      goto endAudioTest;
    }

    console >> ch;
    PTRACE(3, "console in audio test is " << ch);
    switch (tolower(ch)) {
	case '{' : 
	    reader.LowerVolume();
	    break;
	case '}' :
	    reader.RaiseVolume();
	    break;
	case '[' :
	    writer.LowerVolume();
	    break;
	case ']' : 
	    writer.RaiseVolume();
	    break;
	case 'r' :
	    reader.ReportIterations();
	    writer.ReportIterations();
	    break;
	case 'q' :
	case 'x' :
	    goto endAudioTest;
	case 'h' :
	    cout << help ;
	    break;
        default:
	    ;
    }
  }

endAudioTest:
  endNow = PTrue;
  cout  << "end audio test" << endl;

  reader.WaitForTermination();
  writer.WaitForTermination();
}


PBYTEArray *TestAudioDevice::GetNextAudioFrame()
{
  PBYTEArray *data = NULL;

  while (data == NULL) {
    {
      PWaitAndSignal m(access);
      if (GetSize() > 30)
        data = (PBYTEArray *)RemoveAt(0);  
      if (endNow)
        return NULL;
    }

    if (data == NULL) {
      PThread::Sleep(30);
    }
  }
  
  return data;
}

void TestAudioDevice::WriteAudioFrame(PBYTEArray *data)
{
  PWaitAndSignal mutex(access);
  if (endNow) {
    delete data;
    return;
  }
  
  PTRACE(5, "Buffer\tNow put one frame on the que");
  Append(data);
  if (GetSize() > 50) {
    cout << "The audio reader thread is not working - exit now before memory is exhausted" << endl;
    endNow = PTrue;
  }
  return;
}

PBoolean TestAudioDevice::DoEndNow()
{
    return endNow;
}

//////////////////////////////////////////////////////////////////////

TestAudioRead::TestAudioRead(TestAudioDevice &master, const PString & _captureFileName)
    :TestAudio(master),
     captureFileName(_captureFileName)
{    
  PTRACE(3, "Reader\tInitiate thread for reading " );
  Resume();
}

void TestAudioRead::ReportIterations()
{
    cout << "Captured " << iterations << " frames of 480 bytes to the sound card" << endl;
}



void TestAudioRead::Main()
{
  if (!OpenAudio(PSoundChannel::Recorder)) {
    PTRACE(1, "TestAudioWrite\tFAILED to open read device");
    return;
  }
  PWAVFile audioFile;
  if (!captureFileName.IsEmpty()) {
      if (!audioFile.Open(captureFileName, PFile::WriteOnly, PFile::Create | PFile::Truncate))
	  cerr << "Cannot create the file " << captureFileName << " to write audio to" << endl;
  }

  PTRACE(3, "TestAduioRead\tSound device is now open, start running");

  while ((!controller.DoEndNow()) && keepGoing) {
    PBYTEArray *data = new PBYTEArray(480);
    sound.Read(data->GetPointer(), data->GetSize());
    iterations++;
    PTRACE(3, "TestAudioRead\t send one frame to the queue" << data->GetSize());
    PTRACE(5, "Written the frame " << endl << (*data));

    if (audioFile.IsOpen())
	audioFile.Write(data->GetPointer(), data->GetSize());

    controller.WriteAudioFrame(data);
  }
  
  audioFile.Close();
  PTRACE(3, "End audio read thread");
}

//////////////////////////////////////////////////////////////////////

TestAudioWrite::TestAudioWrite(TestAudioDevice &master)
   : TestAudio(master)
{
  PTRACE(3, "Reader\tInitiate thread for writing " );
  Resume();
}

void TestAudioWrite::ReportIterations()
{
    cout << "Written " << iterations << " frames of 480 bytes to the sound card" << endl;
}

void TestAudioWrite::Main()
{
  if (!OpenAudio(PSoundChannel::Player)) {
    PTRACE(1, "TestAudioWrite\tFAILED to open play device");
    return;
  }
  PTRACE(3, "TestAudioWrite\tSound device is now open, start running");    
  
  while ((!controller.DoEndNow()) && keepGoing) {
    PBYTEArray *data = controller.GetNextAudioFrame();
    PTRACE(3, "TestAudioWrite\tHave read one audio frame ");
    if (data != NULL) {
      sound.Write(data->GetPointer(), data->GetSize());
      iterations++;
      delete data;
    } else
      PTRACE(1, "testAudioWrite\t next audio frame is NULL");    
  }


  PTRACE(3, "End audio write thread");
}

//////////////////////////////////////////////////////////////

TestAudio::TestAudio(TestAudioDevice &master) 
    :PThread(1000, NoAutoDeleteThread),
     controller(master)
{
    iterations = 0;
    keepGoing = PTrue;
}

TestAudio::~TestAudio()
{
   sound.Close();
}


PBoolean TestAudio::OpenAudio(enum PSoundChannel::Directions dir)
{
  if (dir == PSoundChannel::Recorder) 
    name = "Recorder";
  else
    name = "Player";
  
  PThread::Current()->SetThreadName(name);
  PString devName = Audio::Current().GetTestDeviceName();
  PTRACE(3, "TestAudio\t open audio start for " << name << " and device name of " << devName);

  PTRACE(3, "Open audio device for " << name << " and device name of " << devName);
  if (!sound.Open(devName,
      dir,
      1, 8000, 16)) {
      cerr <<  "Test:: Failed to open sound device  for " << name << endl;
      cerr <<  "Please check that \"" << devName << "\" is a valid device name" << endl;
      PTRACE(3, "TestAudio\tFailed to open device for " << name << " and device name of " << devName);

    return PFalse;
  }
  
  currentVolume = 90;
  sound.SetVolume(currentVolume);
  
  sound.SetBuffers(480, 2);
  return PTrue;
}


void TestAudio::RaiseVolume()
{
   if ((currentVolume + 5) < 101)
     currentVolume += 5;
   sound.SetVolume(currentVolume);
   cout << name << " volume is " << currentVolume << endl;
   PTRACE(3, "TestAudio\tRaise volume for " << name << " to " << currentVolume);
}

void TestAudio::LowerVolume()
{
   if ((currentVolume - 5) >= 0)
     currentVolume -= 5;
   sound.SetVolume(currentVolume);
   cout << name << " volume is " << currentVolume << endl;
   PTRACE(3, "TestAudio\tLower volume for " << name << " to " << currentVolume);
}
////////////////////////////////////////////////////////////////////////////////

/* The comment below is magic for those who use emacs to edit this file. 
 * With the comment below, the tab key does auto indent to 4 spaces.     
 *
 *
 * Local Variables:
 * mode:c
 * c-basic-offset:4
 * End:
 */

// End of hello.cxx
