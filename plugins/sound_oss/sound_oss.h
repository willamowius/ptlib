
#include <ptlib.h>
#include <ptlib/sound.h>
#include <ptlib/socket.h>

//#if !P_USE_INLINES
//#include <ptlib/contain.inl>
//#endif

#if defined(P_LINUX) || defined(P_FREEBSD) || defined(P_NETBSD) || defined(P_SOLARIS)
#include <sys/soundcard.h>
#endif

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
    unsigned resampleRate;
};

class PSoundChannelOSS: public PSoundChannel
{
 public:
    PSoundChannelOSS();
    void Construct();
    PSoundChannelOSS(const PString &device,
                     PSoundChannel::Directions dir,
                     unsigned numChannels,
                     unsigned sampleRate,
		     unsigned bitsPerSample);
    ~PSoundChannelOSS();
    static PStringArray GetDeviceNames(PSoundChannel::Directions = Player);
    static PString GetDefaultDevice(PSoundChannel::Directions);
    PBoolean Open(const PString & _device,
              Directions _dir,
              unsigned _numChannels,
              unsigned _sampleRate,
              unsigned _bitsPerSample);
    PBoolean Setup();
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
    PBoolean SetVolume(unsigned newVal);
    PBoolean GetVolume(unsigned &devVol);

  protected:
    unsigned mNumChannels;
    unsigned mSampleRate;
    unsigned mBitsPerSample;
    unsigned actualSampleRate;
    Directions direction;
    PString device;
    PBoolean isInitialised;
    unsigned resampleRate;
};
