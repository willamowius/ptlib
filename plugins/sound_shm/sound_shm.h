#include <ptlib.h>
#include <ptlib/sound.h>
#include <ptclib/delaychan.h>

class PAudioDelay : public PObject
{
  PCLASSINFO(PAudioDelay, PObject);

  public:
    PAudioDelay();
    PBoolean Delay(int time);
    void Restart();
    int  GetError();

  protected:
    PTime  previousTime;
    PBoolean   firstTime;
    int    error;
};

#define MIN_HEADROOM    30
#define MAX_HEADROOM    60

class SoundHandleEntry : public PObject {

  PCLASSINFO(SoundHandleEntry, PObject)

  public:
    SoundHandleEntry();

    int handle;
    int direction;

    unsigned numChannels;
    unsigned sampleRate;
    unsigned bitsPerSample;
    unsigned fragmentValue;
    PBoolean isInitialised;
};

#define LOOPBACK_BUFFER_SIZE 5000
#define BYTESINBUF ((startptr<endptr)?(endptr-startptr):(LOOPBACK_BUFFER_SIZE+endptr-startptr))



class PSoundChannelSHM : public PSoundChannel {
 public:
  PSoundChannelSHM();
  void Construct();
  PSoundChannelSHM(const PString &device,
		   PSoundChannel::Directions dir,
		   unsigned numChannels,
		   unsigned sampleRate,
		   unsigned bitsPerSample);
  ~PSoundChannelSHM();
  static PStringArray GetDeviceNames(PSoundChannel::Directions);
  static PString GetDefaultDevice(PSoundChannel::Directions);
  PBoolean Open(const PString & _device,
       Directions _dir,
       unsigned _numChannels,
       unsigned _sampleRate,
       unsigned _bitsPerSample);
  PBoolean Setup();
  PBoolean Close();
  PBoolean Write(const void * buf, PINDEX len);
  int writeSample( void *aData, int aLen );
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
  PBoolean SetVolume (unsigned);
  PBoolean GetVolume (unsigned &);
  PBoolean IsOpen() const;

 private:

  static void UpdateDictionary(PSoundChannel::Directions);
  PBoolean Volume (PBoolean, unsigned, unsigned &);
  PSoundChannel::Directions direction;
  PString device;
  unsigned mNumChannels;
  unsigned mSampleRate;
  unsigned mBitsPerSample;
  PBoolean isInitialised;

  void *os_handle;
  int card_nr;

  PMutex device_mutex;
  PAdaptiveDelay m_Pacing;


  /**number of 30 (or 20) ms long sound intervals stored by ALSA. Typically, 2.*/
  PINDEX storedPeriods;

  /**Total number of bytes of audio stored by ALSA.  Typically, 2*480 or 960.*/
  PINDEX storedSize;

  /** Number of bytes in a ALSA frame. a frame may only be 4ms long*/
  int frameBytes; 

};
