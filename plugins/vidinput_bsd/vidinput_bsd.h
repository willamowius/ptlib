#ifndef _PVIDEOIOBSDCAPTURE

#define _PVIDEOIOBSDCAPTURE

#ifdef __GNUC__   
#pragma interface
#endif

#include <sys/mman.h>

#include <ptlib.h>
#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptclib/delaychan.h>

#if defined(P_FREEBSD)
#include <sys/param.h>
# if __FreeBSD_version >= 502100
#include <dev/bktr/ioctl_meteor.h>
# else
#include <machine/ioctl_meteor.h>
# endif
#endif

#if defined(P_OPENBSD) || defined(P_NETBSD)
#if P_OPENBSD >= 200105
#include <dev/ic/bt8xx.h>
#elif P_NETBSD >= 105000000
#include <dev/ic/bt8xx.h>
#else
#include <i386/ioctl_meteor.h>
#endif
#endif

#if !P_USE_INLINES
#include <ptlib/contain.inl>
#endif


class PVideoInputDevice_BSDCAPTURE : public PVideoInputDevice
{

  PCLASSINFO(PVideoInputDevice_BSDCAPTURE, PVideoInputDevice);

public:
  PVideoInputDevice_BSDCAPTURE();
  ~PVideoInputDevice_BSDCAPTURE();

  PBoolean Open(
    const PString &deviceName,
    PBoolean startImmediate = PTrue
  );

  PBoolean IsOpen();

  PBoolean Close();

  PBoolean Start();
  PBoolean Stop();

  PBoolean IsCapturing();

  static PStringList GetInputDeviceNames();

  PStringArray GetDeviceNames() const
  { return GetInputDeviceNames(); }

  PINDEX GetMaxFrameBytes();

//  PBoolean GetFrame(
//    PBYTEArray & frame
//  );
  PBoolean GetFrameData(
    BYTE * buffer,
    PINDEX * bytesReturned = NULL
  );
  PBoolean GetFrameDataNoDelay(
    BYTE * buffer,
    PINDEX * bytesReturned = NULL
  );

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
//  int GetWhiteness();
//  PBoolean SetWhiteness(unsigned int);
//  int GetColour();
//  PBoolean SetColour(unsigned int);
  int GetHue();
  PBoolean SetHue(unsigned int);

//  PBoolean SetVideoChannelFormat(int, PVideoDevice::VideoFormat);
  PBoolean SetVideoFormat(PVideoDevice::VideoFormat);
  int GetNumChannels();
  PBoolean SetChannel(int);

  PBoolean NormalReadProcess(BYTE*, PINDEX*);

  void ClearMapping();

  struct video_capability
  {
      int channels;   /* Num channels */
      int maxwidth;   /* Supported width */
      int maxheight;  /* And height */
      int minwidth;   /* Supported width */
      int minheight;  /* And height */
  };

  int    videoFd;
  struct video_capability videoCapability;
  int    canMap;  // -1 = don't know, 0 = no, 1 = yes
  BYTE * videoBuffer;
  PINDEX frameBytes;
  int    mmap_size;
  PAdaptiveDelay m_pacing;
 
};

#endif
