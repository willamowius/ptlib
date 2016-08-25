#include <ptlib.h>
#include <ptlib/sound.h>

#define ALSA_PCM_NEW_HW_PARAMS_API 1
#include <alsa/asoundlib.h>


class PSoundChannelALSA : public PSoundChannel {
 public:
  PSoundChannelALSA();
  void Construct();
  PSoundChannelALSA(
    const PString &device,
    PSoundChannel::Directions dir,
    unsigned numChannels,
    unsigned sampleRate,
    unsigned bitsPerSample
  );
  ~PSoundChannelALSA();
  static PStringArray GetDeviceNames(PSoundChannel::Directions);
  static PString GetDefaultDevice(PSoundChannel::Directions);
  PBoolean Open(
    const PString & device,
    Directions dir,
    unsigned numChannels,
    unsigned sampleRate,
    unsigned bitsPerSample
  );
  PBoolean Setup();
  PBoolean Close();
  PBoolean Write(const void * buf, PINDEX len);
  PBoolean Read(void * buf, PINDEX len);
  PBoolean SetFormat(unsigned numChannels, unsigned sampleRate, unsigned bitsPerSample);
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
  PBoolean SetVolume(unsigned);
  PBoolean GetVolume(unsigned &);
  PBoolean IsOpen() const;

 private:
  static void UpdateDictionary(PSoundChannel::Directions);
  bool SetHardwareParams();
  PBoolean Volume(PBoolean, unsigned, unsigned &);

  PSoundChannel::Directions direction;
  PString device;
  unsigned mNumChannels;
  unsigned mSampleRate;
  unsigned mBitsPerSample;
  PBoolean isInitialised;

  snd_pcm_t *os_handle; /* Handle, different from the PChannel handle */
  int card_nr;

  PMutex device_mutex;

  PINDEX m_bufferSize;
  PINDEX m_bufferCount;

  /** Number of bytes in a ALSA frame. a frame may only be 4ms long*/
  int frameBytes; 
};
