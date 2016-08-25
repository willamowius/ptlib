/*
 * main.h
 *
 * PWLib application header file for sound test.
 *
 *
 * $Revision$
 * $Author$
 * $Date$
 */
 
#ifndef _AUDIO_MAIN_H
#define _AUDIO_MAIN_H

#include <ptlib/sound.h>

class Audio : public PProcess
{
  PCLASSINFO(Audio, PProcess);

public:
  Audio();

  void Main();

  PString GetTestDeviceName() { return devName; }

    static Audio & Current()
        { return (Audio &)PProcess::Current(); }

 protected:
  PString devName;
};


/////////////////////////////////////////////////////////////////////////////
PDECLARE_LIST(TestAudioDevice, PBYTEArray *)
#if 0                                //This makes emacs bracket matching code happy.
{
#endif
 public:
  virtual ~TestAudioDevice();
  
  void Test(const PString & captureFileName);
  PBoolean DoEndNow();
  
  void WriteAudioFrame(PBYTEArray *data);
  PBYTEArray *GetNextAudioFrame();
  
 protected:
  PMutex access;
  PBoolean endNow;

};



class TestAudio : public PThread  
{
  PCLASSINFO(TestAudio, PThread)
  public:
    TestAudio(TestAudioDevice &master);
    virtual ~TestAudio();

    virtual void Terminate() { keepGoing = PFalse; }
    void LowerVolume();
    void RaiseVolume();
    
    void ReportIterations();

  protected:
    PString name;
    PBoolean OpenAudio(enum PSoundChannel::Directions dir);

    PINDEX             currentVolume;
    TestAudioDevice    &controller;
    PSoundChannel      sound;
    PBoolean               keepGoing;
    PINDEX             iterations;
};

class TestAudioRead : public TestAudio
{
    PCLASSINFO(TestAudioRead, TestAudio);
  public:
    TestAudioRead(TestAudioDevice &master, const PString & _captureFileName);
    
    void ReportIterations();

    void Main();
 protected:
    PString captureFileName;
};


class TestAudioWrite : public TestAudio
{
    PCLASSINFO(TestAudioWrite, TestAudio);
  public:
    TestAudioWrite(TestAudioDevice &master);

    void ReportIterations();
    
    void Main();
};




#endif  // _AUDIO_MAIN_H
