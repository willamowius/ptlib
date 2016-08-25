
#include <ptlib.h>
#include <ptlib/sound.h>
#include <ptlib/socket.h>

//#if !P_USE_INLINES
//#include <ptlib/contain.inl>
//#endif

#ifdef P_SOLARIS
#include <sys/soundcard.h>
#endif

#ifdef P_LINUX
#include <sys/soundcard.h>
#endif

#ifdef P_FREEBSD
#if P_FREEBSD >= 500000
#include <sys/soundcard.h>
#else
#include <machine/soundcard.h>
#endif
#endif

#if defined(P_OPENBSD) || defined(P_NETBSD)
#include <soundcard.h>
#endif

#include <pulse/stream.h>

class PSoundChannelPulse: public PSoundChannel
{
 public:
    PSoundChannelPulse();
    void Construct();
    PSoundChannelPulse(const PString &device,
                     PSoundChannel::Directions dir,
                     unsigned numChannels,
                     unsigned sampleRate,
		     unsigned bitsPerSample);
    ~PSoundChannelPulse();
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
    PBoolean Abort()   { return PTrue; }
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

    PINDEX bufferSize;
    PINDEX bufferCount;

  pa_sample_spec ss;

  pa_stream *s;
  const void* record_data;
  size_t record_len;

  // This locks all access of this channel to this device. Thus, there
  // is only one activity hitting this instance of sound. if this
  // sound channel instance is used for read, it does not interfere
  // with anything related to write
  PMutex deviceMutex;
};
