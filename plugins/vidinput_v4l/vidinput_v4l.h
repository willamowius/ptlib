
#include <sys/mman.h>
#include <sys/time.h>

#include <ptlib.h>
#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptclib/delaychan.h>

#include <linux/videodev.h>

class PVideoInputDevice_V4L: public PVideoInputDevice
{

public:
  PVideoInputDevice_V4L();
  ~PVideoInputDevice_V4L();

  static PStringList GetInputDeviceNames();

  PStringArray GetDeviceNames() const
  { return GetInputDeviceNames(); }

  PBoolean Open(const PString &deviceName, PBoolean startImmediate);

  PBoolean IsOpen();

  PBoolean Close();

  PBoolean Start();
  PBoolean Stop();

  PBoolean IsCapturing();

  PINDEX GetMaxFrameBytes();

  PBoolean GetFrameData(BYTE*, PINDEX*);
  PBoolean GetFrameDataNoDelay(BYTE*, PINDEX*);

  PBoolean GetFrameSizeLimits(unsigned int&, unsigned int&,
			  unsigned int&, unsigned int&);

  PBoolean TestAllFormats();

  PBoolean SetFrameSize(unsigned int, unsigned int);
  PBoolean SetFrameRate(unsigned int);
  PBoolean VerifyHardwareFrameSize(unsigned int, unsigned int);

  PBoolean GetParameters(int*, int*, int*, int*, int*);

  PBoolean SetColourFormat(const PString&);

  int GetContrast();
  PBoolean SetContrast(unsigned int);
  int GetBrightness();
  PBoolean SetBrightness(unsigned int);
  int GetWhiteness();
  PBoolean SetWhiteness(unsigned int);
  int GetColour();
  PBoolean SetColour(unsigned int);
  int GetHue();
  PBoolean SetHue(unsigned int);

  PBoolean SetVideoChannelFormat(int, PVideoDevice::VideoFormat);
  PBoolean SetVideoFormat(PVideoDevice::VideoFormat);
  int GetNumChannels();
  PBoolean SetChannel(int);

  PBoolean NormalReadProcess(BYTE*, PINDEX*);

  void ClearMapping();
  PBoolean RefreshCapabilities();

  PAdaptiveDelay m_pacing;

  int    videoFd;
  struct video_capability videoCapability;
  int    canMap;  // -1 = don't know, 0 = no, 1 = yes
  int    colourFormatCode;
  PINDEX hint_index;
  BYTE *videoBuffer;
  PINDEX frameBytes;
  
  PBoolean   pendingSync[2];
  
  int    currentFrame;
  struct video_mbuf frame;
  struct video_mmap frameBuffer[2];
  
};
