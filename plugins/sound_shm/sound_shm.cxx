/*
 * sound_shm.cxx
 *
 * Sound driver implementation.
 *
 * usage sample
 *    simpleopal --sound-out shm h323:192.168.15.73
 * use sample code shm2wav to capture audio stream
 *    shm2wav mywav.wav
 *
 * Portable Windows Library
 *
 * Copyright (c) 2009 Matthias Kattanek
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
 * Contributor(s): /
 *
 * $Version: 100 $
 * $Revision$
 * $Author$
 * $Date$
 */

#pragma implementation "sound_shm.h"

#include "sound_shm.h"

//#define SHMDGB_PRINT 1

PCREATE_SOUND_PLUGIN(SHM, PSoundChannelSHM)


static PStringToOrdinal playback_devices;
static PStringToOrdinal capture_devices;
PMutex dictionaryMutex;

///////////////////////////////////////////////////////////////////////////////

#include "shmif.cpp"

#define ASHMVERSION "audioshm 073"

const char AUDIOMAP_SHMID[] = "shmmapid";
const char SEMAPHORE_NAME[] = "audioOPAL.shm";
#define FRAMEBUFFER 20
#define FRAMESIZE 640

#define SHMAP_SIZE FRAMEBUFFER*FRAMESIZE

#ifdef SHMDGB_PRINT
#define DPRINT(args) (printf args)
#else
#define DPRINT(args) //(noprintf args) 
#endif

typedef enum {
  AMS_UNKNOWN = -1,
  AMS_DEFAULT = 0,
  AMS_ACTIVE,
  AMS_EOD,
  AMS_ERROR,
} AudioMapStatus;

SHMIFmap myMap;
AudioMapStatus mapStatus = AMS_DEFAULT;

//!! initial framebytes set by opal are 160
//!!    to be then change to 640
//!!    this will make out memory map too small
//!!    !! for now we hardwire it to 640  
int
openMapping( int framebytes, int samplerate, int channels, int bitspersample )
{
   int rc = -1;
   memset( &myMap,0,sizeof(myMap) );     // initialize map data structure

   myMap.shmName = (char *)AUDIOMAP_SHMID;
   myMap.semName = (char *)SEMAPHORE_NAME;
   //DPRINT(("openMapping(): map=%s sem=%s\n", myMap.shmName,myMap.semName ));
   printf("audioshm:open(): sem=%s\n", myMap.semName );

   // set frame properties AUDIO
   myMap.samplerate = samplerate;        //48000;
   myMap.channels = channels;            //2;
   myMap.bitspersample = bitspersample;  //16
   myMap.format = 123;        //audioFormat (raw pcm);

   // create map
   rc = shmifOpen( &myMap, SHMT_PRODUCER, SHMAP_SIZE );
   if ( rc < 0 ) {
      mapStatus = AMS_ERROR;
      return rc;
   }

   mapStatus = AMS_ACTIVE;
   return 0;
}

int
PSoundChannelSHM::writeSample( void *aData, int aLen )
{
   int rc = -1;
   if ( mapStatus != AMS_ACTIVE ) {
      rc = openMapping( aLen, mSampleRate, mNumChannels, mBitsPerSample );
      //if ( rc < 0 ) return rc;
   }
   rc = shmifWrite( &myMap, aData,aLen );
   return rc;
}

int
closeMapping()
{
   shmifClose( &myMap );
   mapStatus = AMS_EOD;
   return 0;
}

///////////////////////////////////////////////////////////////////////////////

PSoundChannelSHM::PSoundChannelSHM()
{
//printf("ashm constructor\n");
  card_nr = 0;
  os_handle = NULL;
}


PSoundChannelSHM::PSoundChannelSHM (const PString &device,
                                          Directions dir,
                                            unsigned numChannels,
                                            unsigned sampleRate,
                                            unsigned bitsPerSample)
{
  card_nr = 0;
  os_handle = NULL;
//printf("ashm constructor2\n");
  Open (device, dir, numChannels, sampleRate, bitsPerSample);
}


void
PSoundChannelSHM::Construct()
{
//printf("ashm construc()\n");

  int val = 0;
#if PBYTE_ORDER == PLITTLE_ENDIAN
  val = (mBitsPerSample == 16) ? 16 : 8;
#else
  val = (mBitsPerSample == 16) ? 16 : 8;
#endif

  frameBytes = (mNumChannels * (val / 8));

  storedPeriods = 4;
  storedSize = frameBytes * 3;

DPRINT(("ashm construc() frameBytes=%d 4x=storedSize=%d val=%d\n", frameBytes,storedSize,val ));

  card_nr = 0;
  os_handle = NULL;
  isInitialised = PFalse;
}


PSoundChannelSHM::~PSoundChannelSHM()
{
//printf("ashm destructor\n");
   Close();
}



void
PSoundChannelSHM::UpdateDictionary (Directions dir)
{
DPRINT(("ashm updateDict\n"));
}


PStringArray
PSoundChannelSHM::GetDeviceNames (Directions dir)
{
   PStringArray devices;
 
DPRINT(("ashm getdevicenames dir=%d\n", dir));
  
   if (dir == Recorder)
      devices += "noshm";
   else
      devices += "shm";
   return devices;
}


PString
PSoundChannelSHM::GetDefaultDevice(Directions dir)
{
DPRINT(("ashm getdefaultdevice \n"));
  PStringArray devicenames;
  devicenames = PSoundChannelSHM::GetDeviceNames (dir);

  return devicenames[0];
}


int prinCounter = 20;

PBoolean PSoundChannelSHM::Open (const PString & _device,
                              Directions _dir,
                              unsigned _numChannels,
                              unsigned _sampleRate,
                              unsigned _bitsPerSample)
{

DPRINT(("ashm Open %s: dir=%d :%s:\n", AIPMVERSION, _dir, (const unsigned char *)_device));
DPRINT(("ashm Open channel=%d sampleRate=%d bitsperSample=%d\n", 
                        _numChannels, _sampleRate, _bitsPerSample ));
  Close();

  prinCounter = 20;
  direction = _dir;
  mNumChannels = _numChannels;
  mSampleRate = _sampleRate;
  mBitsPerSample = _bitsPerSample;

  Construct();

  PWaitAndSignal m(device_mutex);

  if (_device != "shm") {
      DPRINT(("ashm Open() device %s not supported\n", (const unsigned char *)_device ));
      return PFalse;
  }
  /* save internal parameters */
  card_nr = -2;
  device = "default";
  os_handle = (void*)73;        //pretend we have a valid handle

  Setup ();
  //PTRACE (1, "ALSA\tDevice " << real_device_name << " Opened");

  return PTrue;
}


PBoolean PSoundChannelSHM::Setup()
{
  DPRINT(("ashm setup\n"));

  if (isInitialised) {
    printf("ashm Setup() Skipping setup of %s as instance already initialised", (const char *)device);
    return PTrue;
  }
 
  isInitialised = PTrue;
  return PTrue;
}


PBoolean PSoundChannelSHM::Close()
{
  DPRINT(("ashm close\n"));

  PWaitAndSignal m(device_mutex);

  /* if the channel isn't open, do nothing */
  if (!os_handle)
    return PFalse;
  DPRINT(("ashm close  oshandle=%d\n", (int)os_handle));
  closeMapping();
  os_handle = NULL;
  isInitialised = PFalse;
  return PTrue;
}

PBoolean PSoundChannelSHM::Write (const void *buf, PINDEX len)
{
  int bytesPerSec = mSampleRate * mNumChannels * mBitsPerSample /8;
  int delay = len * 1000 / bytesPerSec;
  if ( prinCounter ) {
     prinCounter--;
     DPRINT(("ashm write %d len=%d fb=%d delay=%d\n", prinCounter, len, frameBytes, delay));
  }

  lastWriteCount = 0;
  PWaitAndSignal m(device_mutex);

  if ((!isInitialised && !Setup()) || !len || !os_handle)
    return PFalse;

  writeSample( (void*)buf, len );

   m_Pacing.Delay(delay);
   lastWriteCount = 640;   //len;

  return PTrue;
}


PBoolean PSoundChannelSHM::Read (void * buf, PINDEX len)
{
  DPRINT(("ashm read\n"));

  printf("ashm read read not supported on device ipm\n");
  return PFalse;
  
  return PTrue;
}


PBoolean PSoundChannelSHM::SetFormat (unsigned numChannels,
                                   unsigned sampleRate,
                                   unsigned bitsPerSample)
{
  DPRINT(("ashm setformat\n"));
  if (!os_handle)
    return SetErrorValues(NotOpen, EBADF);

  /* check parameters */
  PAssert((bitsPerSample == 8) || (bitsPerSample == 16), PInvalidParameter);
  PAssert(numChannels >= 1 && numChannels <= 2, PInvalidParameter);

  mNumChannels   = numChannels;
  mSampleRate    = sampleRate;
  mBitsPerSample = bitsPerSample;
 
  /* mark this channel as uninitialised */
  isInitialised = PFalse;

  return PTrue;
}


unsigned PSoundChannelSHM::GetChannels()   const
{
  return mNumChannels;
}


unsigned PSoundChannelSHM::GetSampleRate() const
{
  return mSampleRate;
}


unsigned PSoundChannelSHM::GetSampleSize() const
{
  return mBitsPerSample;
}


PBoolean PSoundChannelSHM::SetBuffers (PINDEX size, PINDEX count)
{
  storedPeriods = count;
  storedSize = size;

  isInitialised = PFalse;

  return PTrue;
}


PBoolean PSoundChannelSHM::GetBuffers(PINDEX & size, PINDEX & count)
{
  size = storedSize;
  count = storedPeriods;
  
  return PFalse;
}


PBoolean PSoundChannelSHM::PlaySound(const PSound & sound, PBoolean wait)
{
  DPRINT(("ashm playSound\n"));

  PINDEX pos = 0;
  PINDEX len = 0;
  char *buf = (char *) (const BYTE *) sound;

  if (!os_handle)
    return SetErrorValues(NotOpen, EBADF);

  len = sound.GetSize();
  do {

    if (!Write(&buf [pos], PMIN(320, len - pos)))
      return PFalse;
    pos += 320;
  } while (pos < len);

  if (wait)
    return WaitForPlayCompletion();

  return PTrue;
}


PBoolean PSoundChannelSHM::PlayFile(const PFilePath & filename, PBoolean wait)
{
  DPRINT(("ashm playFile\n"));
  BYTE buffer [512];
  
  if (!os_handle)
    return SetErrorValues(NotOpen, EBADF);

  PFile file (filename, PFile::ReadOnly);

  if (!file.IsOpen())
    return PFalse;

  for (;;) {

    if (!file.Read (buffer, 512))
      break;

    PINDEX len = file.GetLastReadCount();
    if (len == 0)
      break;
    if (!Write(buffer, len))
      break;
  }

  file.Close();

  if (wait)
    return WaitForPlayCompletion();

  return PTrue;
}


PBoolean PSoundChannelSHM::HasPlayCompleted()
{
  DPRINT(("ashm hasplayCompleted\n"));
  return PFalse;
}


PBoolean PSoundChannelSHM::WaitForPlayCompletion()
{
  DPRINT(("ashm wait4playCompleted\n"));
  return PFalse;
}


PBoolean PSoundChannelSHM::RecordSound(PSound & sound)
{
  return PFalse;
}


PBoolean PSoundChannelSHM::RecordFile(const PFilePath & filename)
{
  return PFalse;
}


PBoolean PSoundChannelSHM::StartRecording()
{
  return PFalse;
}


PBoolean PSoundChannelSHM::IsRecordBufferFull()
{
  return PTrue;
}


PBoolean PSoundChannelSHM::AreAllRecordBuffersFull()
{
  return PTrue;
}


PBoolean PSoundChannelSHM::WaitForRecordBufferFull()
{
  return PTrue;
}


PBoolean PSoundChannelSHM::WaitForAllRecordBuffersFull()
{
  return PFalse;
}


PBoolean PSoundChannelSHM::Abort()
{
  if (!os_handle)
    return PFalse;
  return PTrue;
}



PBoolean PSoundChannelSHM::SetVolume (unsigned newVal)
{
  unsigned i = 0;

  return Volume (PTrue, newVal, i);
}


PBoolean  PSoundChannelSHM::GetVolume(unsigned &devVol)
{
  return Volume (PFalse, 0, devVol);
}
  

PBoolean PSoundChannelSHM::IsOpen () const
{
  return (os_handle != NULL);
}


PBoolean PSoundChannelSHM::Volume (PBoolean set, unsigned set_vol, unsigned &get_vol)
{
    return PFalse;
}

