/*
 * main.cxx
 *
 * PWLib application source file for PluginTest
 *
 * Main program entry point.
 *
 * Copyright 2003 Equivalence
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <ptlib/pprocess.h>

#include <ptlib/pluginmgr.h>
#include <ptlib/sound.h>
#include <ptlib/video.h>
#include "main.h"

#include <math.h>

#ifndef M_PI
#define M_PI  3.1415926
#endif

PCREATE_PROCESS(PluginTest);

#define SAMPLES 64000  

PluginTest::PluginTest()
  : PProcess("Equivalence", "PluginTest", 1, 0, AlphaCode, 1)
{
}

void Usage()
{
  PError << "usage: plugintest [options]\n \n"
            "  -d dir      : Set the directory from which plugins are loaded\n"
            "  -s          : show the list of available PSoundChannel drivers\n"
            "  -l          : list all plugin drivers\n"
            "  -L          : list all plugin drivers using abstract factory interface\n"
            "  -a driver   : play test sound using specified driver and default device\n"
            "                Use \"default\" as driver to use default (first) driver\n"
            "                Can also specify device as first arg, or use \"list\" to list all devices\n"
            "  -A driver   : same as -a but uses abstract factory based routines\n"
            "  -t          : set trace level (can be set more than once)\n"
            "  -o fn       : write trace output to file\n"
            "  -h          : display this help message\n";
   return;
}

ostream & operator << (ostream & strm, const std::vector<PString> & vec)
{
  char separator = strm.fill();
  int width = strm.width();
  for (std::vector<PString>::const_iterator r = vec.begin(); r != vec.end(); ++r) {
    if (r != vec.begin() && separator != '\0')
      strm << separator;
    strm.width(width);
    strm << *r;
  }
  if (separator == '\n')
    strm << '\n';

  return strm;
}

template <class DeviceType>
void DisplayPluginTypes(const PString & type)
{
  cout << "   " << type << " : ";
  std::vector<std::string> services = PFactory<DeviceType>::GetKeyList();
  if (services.size() == 0)
    cout << "None available" << endl;
  else {
    for (size_t i = 0; i < services.size(); ++i) {
      if (i > 0)
        cout << ',';
      cout << services[i];
    }
    cout << endl;
  }
}


void PluginTest::Main()
{
  PArgList & args = GetArguments();

  args.Parse(
       "t-trace."              
       "o-output:"             
       "h-help."               
       "l-list."               
       "L-List."               
       "s-service:"   
       "S-sounddefault:"
       "a-audio:"
       "A-Audio:"
       "d-directory:"          
       );

  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);

  if (args.HasOption('d')) {
    PPluginManager & pluginMgr = PPluginManager::GetPluginManager();
    pluginMgr.LoadPluginDirectory(args.GetOptionString('d'));
  }

  if (args.HasOption('h')) {
    Usage();
    return;
  }

  if (args.HasOption('l')) {
    cout << "List available plugin types" << endl;
    PPluginManager & pluginMgr = PPluginManager::GetPluginManager();
    PStringList plugins = pluginMgr.GetPluginTypes();
    if (plugins.GetSize() == 0)
      cout << "No plugins loaded" << endl;
    else {
      cout << plugins.GetSize() << " plugin types available:" << endl;
      for (int i = 0; i < plugins.GetSize(); i++) {
        cout << "   " << plugins[i] << " : ";
        PStringList services = pluginMgr.GetPluginsProviding(plugins[i]);
        if (services.GetSize() == 0)
          cout << "None available" << endl;
        else
          cout << setfill(',') << services << setfill(' ') << endl;
      }
    }
    return;
  }

  if (args.HasOption('L')) {
    DisplayPluginTypes<PSoundChannel>("PSoundChannel");
    DisplayPluginTypes<PVideoInputDevice>("PVideoInputDevice");
    DisplayPluginTypes<PVideoOutputDevice>("PVideoOutputDevice");
    return;
  }

  if (args.HasOption('s')) {
    cout << "Available " << args.GetOptionString('s') << " :" <<endl;
    cout << "Sound plugin names = " << setfill(',') << PSoundChannel::GetDriverNames() << setfill(' ') << endl;
    return;
  }

  if (args.HasOption('S')) {
    cout << "Default sound device is \"" << PSoundChannel::GetDefaultDevice(PSoundChannel::Player) << "\"" << endl;
    return;
  }

  if (args.HasOption('a') || args.HasOption('A')) {
    PString driver;
    PBoolean useFactory = PFalse;
    if (args.HasOption('a'))
      driver = args.GetOptionString('a');
    else {
      driver = args.GetOptionString('A');
      useFactory = PTrue;
    }

    PStringList driverList;
    if (useFactory)
      driverList = PStringList::container<std::vector<std::string> >(PFactory<PSoundChannel>::GetKeyList());
    else
      driverList = PSoundChannel::GetDriverNames();

    if (driver *= "default") {
      if (driverList.GetSize() == 0) {
        cout << "No sound device drivers available\n";
        return;
      }
      driver = driverList[0];
    }
    else if (driver *= "list") {
      cout << "Drivers: " << setfill('\n') << driverList << endl;
      return;
    }


    PStringList deviceList = PSoundChannel::GetDeviceNames(driver, PSoundChannel::Player);
    if (deviceList.GetSize() == 0) {
      cout << "No devices for sound driver " << driver << endl;
      return;
    }

    PString device;

    if (args.GetCount() > 0) {
      device = args[0];
      if (driver *= "list") {
        cout << "Devices = " << deviceList << endl;
        return;
      }
    }
    else {
      device = deviceList[0];
    }
    
    cout << "Using sound driver" << driver << " with device " << device << endl;

    PSoundChannel * snd = PSoundChannel::CreateChannel(driver);
    if (snd == NULL) {
      cout << "Failed to create sound channel with " << driver << endl;
      return;
    }

    cout << "Opening sound driver " << driver << " with device " << device << endl;

    if (!snd->Open(device, PSoundChannel::Player)) {
      cout << "Failed to open sound driver " << driver  << " with device " << device << endl;
      return;
    }

    if (!snd->IsOpen()) {
      cout << "Sound device " << device << " not open" << endl;
      return;
    }

    if (!snd->SetBuffers(SAMPLES, 2)) {
      cout << "Failed to set samples to " << SAMPLES << " and 2 buffers. End program now." << endl;
      return;
    }

    snd->SetVolume(100);

    PWORDArray audio(SAMPLES);
    int i, pointsPerCycle = 8;
    int volume = 80;
    double angle;

    for (i = 0; i < SAMPLES; i++) {
      angle = M_PI * 2 * (double)(i % pointsPerCycle)/pointsPerCycle;
      if ((i % 4000) < 3000)
        audio[i] = (unsigned short) ((16384 * cos(angle) * volume)/100);
      else
        audio[i] = 0;
    }

    if (!snd->Write((unsigned char *)audio.GetPointer(), SAMPLES * 2)) {
      cout << "Failed to write  " << SAMPLES/8000  << " seconds of beep beep. End program now." << endl;
      return;
    }

    snd->WaitForPlayCompletion();
  }
}

// End of File ///////////////////////////////////////////////////////////////
