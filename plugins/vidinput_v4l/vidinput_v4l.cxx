/*
 * video4linux.cxx
 *
 * Classes to support streaming video input (grabbing) and output.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2000 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Derek Smithies (derek@indranet.co.nz)
 *                 Mark Cooke (mpc@star.sr.bham.ac.uk)
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#pragma implementation "vidinput_v4l.h"
#include <ptlib.h>
#include <ptlib/pstring.h>
#include <ptlib/pluginmgr.h>
#include "vidinput_v4l.h"
#include <sys/utsname.h>

PCREATE_VIDINPUT_PLUGIN(V4L);

///////////////////////////////////////////////////////////////////////////////
// Linux Video4Linux Driver Hints Tables.
//
// In an ideal API, we wouldn't need these hints on setup.  There are enough
// wrinkles it seems we have to provide a static list of hints for known
// issues.

#define HINT_CSWIN_ZERO_FLAGS               0x0001
#define HINT_CSPICT_ALWAYS_WORKS            0x0002  /// ioctl return value indicates pict was set ok.
#define HINT_CGPICT_DOESNT_SET_PALETTE      0x0004
#define HINT_HAS_PREF_PALETTE               0x0008  /// use this palette with this camera.
#define HINT_ALWAYS_WORKS_320_240           0x0010  /// Camera always  opens OK at this size.
#define HINT_ALWAYS_WORKS_640_480           0x0020  /// Camera always  opens OK at this size.
#define HINT_ONLY_WORKS_PREF_PALETTE        0x0040  /// Camera always (and only) opens at pref palette.
#define HINT_CGWIN_FAILS                    0x0080  /// ioctl VIDIOCGWIN always fails.
#define HINT_FORCE_LARGE_SIZE               0x0100  /// driver does not work in small video size.
#define HINT_FORCE_DEPTH_16                 0x0200  /// CPiA cameras return a wrong value for the depth, and if you try to use that wrong value, it fails.
#define HINT_FORCE_DBLBUF                   0x0400  /// Force double buffering on quickcam express

static struct {
  const char     *name_regexp;        // String used to match the driver name
  const char     *name;               // String used for ptrace output
  const char     *version;             // Apply the hint if kernel
                                // version < given version,
                                // 0 means always apply
  unsigned hints;               // Hint flags
  int      pref_palette;        // Preferred palette.
} driver_hints[] = {

    /**Most of usb web cameras from the spca5xx driver
     */
    
  { "^Broken sensor chipset that accept only 640x480$",
    "Broken sensor chipset that accept only 640x480",
    NULL,
    HINT_ALWAYS_WORKS_640_480|
    HINT_CGWIN_FAILS,
    0},

     /**Philips usb web cameras
       Native format is 420(P) so use it.
     */
    
  { "^Philips [0-9]+ webcam$",
    "Philips USB webcam",
    NULL,
    HINT_ALWAYS_WORKS_640_480,
    VIDEO_PALETTE_YUV420P },
  
  /**Brooktree based capture boards.

     The current bttv driver doesn't fail CSPICT calls with unsupported
     palettes.  It also doesn't return a useful value from CGPICT calls
     to readback the palette. Not needed anymore from 2.4.23
   */
    { "^BT8(4|7)(8|9)",
      "Brooktree BT848 and BT878 based capture boards",
      "2.4.23",
      HINT_CSWIN_ZERO_FLAGS |
      HINT_CSPICT_ALWAYS_WORKS |
      HINT_CGPICT_DOESNT_SET_PALETTE |
      HINT_HAS_PREF_PALETTE,
      VIDEO_PALETTE_YUV420P },

  /** Quickcam Communicate STX (spca5xx driver)
      Actually, it's not true that it needs VIDEO_PALETTE_YUV420P.
      But it wouldn't be reasonable to convert the pictures twice.
   */
  { "Logitech QuickCam Communicate S",
    "Logitech Quickcam Communicate STX (spca5xx driver)",
    NULL,
    HINT_ALWAYS_WORKS_320_240 |
    HINT_ALWAYS_WORKS_640_480 |
    HINT_HAS_PREF_PALETTE,
    VIDEO_PALETTE_YUV420P },
  
  /** Quickcam Express (qc-usb driver) */
  { "Logitech [USB Camera|QuickCam USB]",
    "Quickcam Express (qc-usb driver)",
    NULL,
    HINT_FORCE_DBLBUF,
    0},
  
  /** Sony Vaio Motion Eye camera
      Linux kernel 2.4.7 has meye.c driver module.
   */
  { "meye",
    "Sony Vaio Motion Eye Camera",
    NULL,
    HINT_CGPICT_DOESNT_SET_PALETTE |
    HINT_CSPICT_ALWAYS_WORKS       |
    HINT_ALWAYS_WORKS_320_240      |
    HINT_ALWAYS_WORKS_640_480      |
    HINT_CGWIN_FAILS               |
    HINT_ONLY_WORKS_PREF_PALETTE   |
    HINT_HAS_PREF_PALETTE,
    VIDEO_PALETTE_YUV422 },

  /** USB camera, which only works in large size.
   */
  { "Logitech USB Webcam",
    "Logitech USB Webcam which works in large size only",
    NULL,
    HINT_FORCE_LARGE_SIZE,
    VIDEO_PALETTE_YUV420P 
  },

  /** Creative VideoBlaster Webcam II USB
   */
  {"CPiA Camera",
   "CPIA which works with cpia and cpia_usb driver modules",
   NULL,
   HINT_FORCE_DEPTH_16 |
   HINT_ONLY_WORKS_PREF_PALETTE   |
   HINT_HAS_PREF_PALETTE,
   VIDEO_PALETTE_YUV422
  },

 /** Intel PC Pro Camera
 
 */
  { "SPCA50X USB Camera",
    "Intel PC Pro Camera uses the spca50x driver",
    NULL,
    HINT_ONLY_WORKS_PREF_PALETTE        |
    HINT_HAS_PREF_PALETTE,
    VIDEO_PALETTE_RGB24
  },


  /** Default device with no special settings
   */
  { "",
    "V4L Supported Device",
    0,
    0,
    0 }

};

/*
 * This is list of channel names that accept only fixed size resolution
 * The spca5xx driver store in the channel name, the name of the bridge,
 * not the sensor, so we use a second list to trim false positive.
 */
static const char *bridges_with_640x480_fixed_width[] = {
   "SPCA505",
   "SPCA506",
   "SPCA501",
   "SPCA504",
   "SPCA500", /* Only the LogitechClickSmart310 doesn't support the 640x480 */
   "SPCA504B",
   "SPCA504C",
   "SPCA536",
   "SN9C102", /* SENSOR_PAS106 and SENSOR_TAS5110 doesn't support the 640x480 */
   "ZC301-2", /* SENSOR_PAS106 doesn't support the 640x480 */
   "CX11646",
   "SN9CXXX",
   "MR97311",
   "VC0321",
};

static const char *sensors_with_352x288_fixed_width[] = {
   "Philips SPC200NC ",	/* Using the SENSOR_PAS106 sensor */
   "Philips SPC210NC (FB) ",
   "Philips SPC300NC ",
   "Creative NX",
   "Creative Instant P0620",
   "Creative Instant P0620D",
   "Sonix sn9c10x + Pas106 sensor",
   "Genius VideoCAM NB", /* Using the SENSOR_TAS5110 sensor */
   "Sweex SIF webcam",
   "Logitech ClickSmart 310", /* Using the SENSOR_HDCS1020 sensor */
};

#if 0
/* TODO: We need to do the same hack */
static const char *bridges_with_352x288_fixed_width[] = {
   "SPCA508", /* 352x288 */
   "SPCA561", /* 352x288 */
   "TV8532", /* 352x288 */
   "ET61XX51", /* 320x240 */
   //  "SPCA533", /* 464*480 */
   //  "PAC207BCA", /* 320x240 only */
};
#endif

#define HINT(h) ((driver_hints[hint_index].hints & h) ? PTrue : PFalse)
#define MAJOR(a) (int)((unsigned short) (a) >> 8)
#define MINOR(a) (int)((unsigned short) (a) & 0xFF) 
// this is used to get more userfriendly names:
class V4LNames : public PObject
{ 
   PCLASSINFO(V4LNames, PObject);
public:
  V4LNames() {/* nothing */};

  void Update ();
  
  PString GetUserFriendly(PString devName);

  PString GetDeviceName(PString userName);

  PStringList GetInputDeviceNames();

protected:
  void AddUserDeviceName(PString userName, PString devName);

  PString BuildUserFriendly(PString devname);

  void PopulateDictionary();

  void ReadDeviceDirectory(PDirectory devdir, POrdinalToString & vid);

  PMutex          mutex;
  PStringToString deviceKey;
  PStringToString userKey;
  PStringList     inputDeviceNames;
};

void
V4LNames::Update()
{
  PDirectory   procvideo("/proc/video/dev");
  PString      entry;
  PStringList  devlist;
  
  PWaitAndSignal m(mutex);
  inputDeviceNames.RemoveAll (); // flush the previous run
  if (procvideo.Exists()) {
    if (procvideo.Open(PFileInfo::RegularFile)) {
      do {
        entry = procvideo.GetEntryName();
        if ((entry.Left(5) == "video") || (entry.Left(7) == "capture")) {
          PString thisDevice = "/dev/video" + entry.Right(1);
          int videoFd = ::open((const char *)thisDevice, O_RDONLY | O_NONBLOCK);

          if ((videoFd > 0) || (errno == EBUSY)){
            PBoolean valid = PFalse;
            struct video_capability  videoCaps;
            if (ioctl(videoFd, VIDIOCGCAP, &videoCaps) >= 0 && (videoCaps.type & VID_TYPE_CAPTURE) != 0)
              valid = PTrue;
            if (videoFd >= 0)
              close(videoFd); 
            if (valid)
              inputDeviceNames += thisDevice;
          }
        }
      } while (procvideo.Next());
    }   
  } 
  if (inputDeviceNames.GetSize() == 0) {
    POrdinalToString vid;
    ReadDeviceDirectory("/dev/", vid);

    for (PINDEX i = 0; i < vid.GetSize(); i++) {
      PINDEX cardnum = vid.GetKeyAt(i);
      int fd = ::open(vid[cardnum], O_RDONLY | O_NONBLOCK);
      if ((fd >= 0) || (errno == EBUSY)) {
        if (fd >= 0)
          ::close(fd);
        inputDeviceNames += vid[cardnum];
      }
    }
  }
  PopulateDictionary();
}

void  V4LNames::ReadDeviceDirectory(PDirectory devdir, POrdinalToString & vid)
{
  if (!devdir.Open())
    return;

  do {
    PString filename = devdir.GetEntryName();
    PString devname = devdir + filename;
    if (devdir.IsSubDir())
      ReadDeviceDirectory(devname, vid);
    else {

      PFileInfo info;
      if (devdir.GetInfo(info) && info.type == PFileInfo::CharDevice) {
        struct stat s;
        if (lstat(devname, &s) == 0) {
#if defined(P_FREEBSD)
          // device numbers are irrelevant here, so we match on names instead
          if (filename.GetLength() <= 5 || filename.Left(5) != "video")
            continue;
          int num = atoi(filename.Mid(6));
          if (num < 0 || num > 63)
            continue;
          vid.SetAt(num, devname);
#else
          static const int deviceNumbers[] = { 81 };
          for (PINDEX i = 0; i < PARRAYSIZE(deviceNumbers); i++) {
            if (MAJOR(s.st_rdev) == deviceNumbers[i]) {
              PINDEX num = MINOR(s.st_rdev);
              if (num <= 63 && num >= 0) {
                vid.SetAt(num, devname);
              }
            }
          }
#endif
        }
      }
    }
  } while (devdir.Next());
}

void V4LNames::PopulateDictionary()
{
  PINDEX i, j;
  PStringToString tempList;

  for (i = 0; i < inputDeviceNames.GetSize(); i++) {
    PString ufname = BuildUserFriendly(inputDeviceNames[i]);
    tempList.SetAt(inputDeviceNames[i], ufname);
  }

  //Now, we need to cope with the case where there are two video
  //devices available, which both have the same user friendly name.
  //Matching user friendly names have a (X) appended to the name.
  for (i = 0; i < tempList.GetSize(); i++) {
    PString userName = tempList.GetDataAt(i); 

    PINDEX matches = 1;    
    for (j = i + 1; j < tempList.GetSize(); j++) {
      if (tempList.GetDataAt(j) == userName) {
        matches++;
        PStringStream revisedUserName;
        revisedUserName << userName << " (" << matches << ")";
        tempList.SetDataAt(j, revisedUserName);
      }
    }
  }

  //At this stage, we have correctly modified the temp list of names.
  for (j = 0; j < tempList.GetSize(); j++)
    AddUserDeviceName(tempList.GetDataAt(j), tempList.GetKeyAt(j));  
}

PString V4LNames::GetUserFriendly(PString devName)
{
  PWaitAndSignal m(mutex);

  PString result= deviceKey(devName);
  if (result.IsEmpty())
    return devName;

  return result;
}

PString V4LNames::GetDeviceName(PString userName)
{
  PWaitAndSignal m(mutex);

  for (PINDEX i = 0; i < userKey.GetSize(); i++)
    if (userKey.GetKeyAt(i).Find(userName) != P_MAX_INDEX)
      return userKey.GetDataAt(i);

  return userName;
}

void V4LNames::AddUserDeviceName(PString userName, PString devName)
{
  PWaitAndSignal m(mutex);

  if (userName != devName) { // must be a real userName!
    userKey.SetAt(userName, devName);
    deviceKey.SetAt(devName, userName);
  } else { // we didn't find a good userName
    if (!deviceKey.Contains (devName)) { // never met before: fallback
      userKey.SetAt(userName, devName);
      deviceKey.SetAt(devName, userName);
    } // no else: we already know the pair
  }
}

PString V4LNames::BuildUserFriendly(PString devname)
{
  PString Result;

  int fd = ::open((const char *)devname, O_RDONLY);
  if(fd < 0) {
    return devname;
  }

  struct video_capability videocap;
  if (::ioctl(fd, VIDIOCGCAP, &videocap) < 0)  {
      ::close(fd);
      return devname;
    }
  
  ::close(fd);
  PString ufname(videocap.name);  

  return ufname;
}

/*
  There is a duplication in the list of names.
  Consequently, opening the device as "ov511++" or "/dev/video0" will work.
*/
PStringList V4LNames::GetInputDeviceNames()
{
  PWaitAndSignal m(mutex);
  PStringList result;
  for (PINDEX i = 0; i < inputDeviceNames.GetSize(); i++) {
    result += GetUserFriendly (inputDeviceNames[i]);
  }

  return result;
}

PMutex creationMutex;
static 
V4LNames & GetNames()
{
  PWaitAndSignal m(creationMutex);
  static V4LNames names;
  names.Update();
  return names;
} 

///////////////////////////////////////////////////////////////////////////////
// PVideoInputDevice_V4L

PVideoInputDevice_V4L::PVideoInputDevice_V4L()
{
  videoFd       = -1;
  hint_index    = PARRAYSIZE(driver_hints) - 1;

  canMap           = -1;
  for (int i=0; i<2; i++)
    pendingSync[i] = PFalse;
}

PVideoInputDevice_V4L::~PVideoInputDevice_V4L()
{
    Close();
}

/* From the spca5xx driver and gspca driver */
#ifndef VIDEO_PALETTE_RAW_JPEG
#define VIDEO_PALETTE_RAW_JPEG  20
#define VIDEO_PALETTE_JPEG 21
#endif

static struct {
  const char * colourFormat;
  int code;
} colourFormatTab[] = {
  { "Grey", VIDEO_PALETTE_GREY },  //Entries in this table correspond
  { "BGR32", VIDEO_PALETTE_RGB32 }, //(line by line) to those in the 
  { "BGR24", VIDEO_PALETTE_RGB24 }, // PVideoDevice ColourFormat table.
  { "RGB565", VIDEO_PALETTE_RGB565 },
  { "RGB555", VIDEO_PALETTE_RGB555 },
  { "YUV422", VIDEO_PALETTE_YUV422 },
  { "YUV422P", VIDEO_PALETTE_YUV422P },
  { "YUV411", VIDEO_PALETTE_YUV411 },
  { "YUV411P", VIDEO_PALETTE_YUV411P },
  { "YUV420", VIDEO_PALETTE_YUV420 },
  { "YUV420P", VIDEO_PALETTE_YUV420P },
  { "YUV410P", VIDEO_PALETTE_YUV410P },
  { "UYVY422", VIDEO_PALETTE_UYVY },
  { "MJPEG", VIDEO_PALETTE_JPEG }
};


PBoolean PVideoInputDevice_V4L::Open(const PString & devName, PBoolean startImmediate)
{
  struct utsname buf;
  PString version;
  
  uname (&buf);

  if (buf.release)
    version = PString (buf.release);

  Close();
  
  PTRACE(1,"PVideoInputDevice_V4L: trying to open "<< devName);

  // check if it is a userfriendly name, and if so, get the real device name

  PString deviceName = GetNames().GetDeviceName(devName);
  videoFd = ::open((const char *)deviceName, O_RDWR);
  if (videoFd < 0) {
    PTRACE(1,"PVideoInputDevice_V4L::Open failed : "<< ::strerror(errno));
    return PFalse;
  }
  
  // get the device capabilities
  if (!RefreshCapabilities()) {
    ::close (videoFd);
    videoFd = -1;
    return PFalse;
  }
  
  if ((videoCapability.type & VID_TYPE_CAPTURE) == 0) {
    PTRACE(1,"PVideoInputDevice_V4L:: device capablilities reports cannot capture");
    ::close (videoFd);
    videoFd = -1;
    return PFalse;
  }

  hint_index = PARRAYSIZE(driver_hints) - 1;
  PString driver_name(videoCapability.name);  

  // Scan the hint table, looking for regular expression matches with
  // drivers we hold hints for.
  PINDEX tbl;
  for (tbl = 0; tbl < PARRAYSIZE(driver_hints); tbl ++) {
    PRegularExpression regexp;
    regexp.Compile(driver_hints[tbl].name_regexp, PRegularExpression::Extended);

    if (driver_name.FindRegEx(regexp) != P_MAX_INDEX) {
      PTRACE(1,"PVideoInputDevice_V4L::Open: Found driver hints: " << driver_hints[tbl].name);
      PTRACE(1,"PVideoInputDevice_V4L::Open: format: " << driver_hints[tbl].pref_palette);

      if (driver_hints[tbl].version && !version.IsEmpty ()) {
        if (PString (version) < PString (driver_hints[tbl].version)) {
          PTRACE(1,"PVideoInputDevice_V4L::Open: Hints applied because kernel version less than " << driver_hints[tbl].version);
          hint_index = tbl;
          break;
        }
        else {
          PTRACE(1,"PVideoInputDevice_V4L::Open: Hints not applied because kernel version is not less than " << driver_hints[tbl].version);
        }
      }
      else {
        hint_index = tbl;
        break;
      }
    }
  }

  /*
   * Some drivers like the spca5xx ou the gspca returns OK for any resolution
   * between min-max. But the image is crop, and the user doesn't see his face
   * entirely.
   * The problem is the drive support more than 200 webcams, and we cannot add
   * them all to the list of driver_hints[], so we use the sensor description
   * to enable the hack. The channel name contains the name of the bridge, so
   * we have only 10 comp to be. But some webcams have a 640x480 bridge, and a
   * small sensor ...
   */
  if (hint_index >= PARRAYSIZE(driver_hints)-1) {
     struct video_channel channel;
     memset(&channel, 0, sizeof(struct video_channel));
     if (::ioctl(videoFd, VIDIOCGCHAN, &channel) == 0) {
	/* Only check if the called doesn't return an error */
	for (tbl = 0; tbl < PARRAYSIZE(bridges_with_640x480_fixed_width); tbl ++) {
	   if (strcmp(bridges_with_640x480_fixed_width[tbl], channel.name) == 0) {
	      PBoolean false_positive = PFalse;
	      unsigned int idx;
	      for (idx = 0; idx < PARRAYSIZE(sensors_with_352x288_fixed_width); idx++) {
		 if (strcmp(sensors_with_352x288_fixed_width[idx], videoCapability.name) == 0) {
		    false_positive = PTrue;
		    break;
		 }
	      }
	      if (false_positive == PFalse) {
		 PTRACE(1,"PVideoInputDevice_V4L::Open: Found fixed 640x480 sensor");
		 hint_index = 0;
		 break;
	      }
	   }
	}
     }
  }
 
  // Force double-buffering with buggy Quickcam driver.
  if (HINT (HINT_FORCE_DBLBUF)) {

#define QC_IOCTLBASE            220
#define VIDIOCQCGCOMPATIBLE     _IOR ('v',QC_IOCTLBASE+10,int)  /* Get enable workaround for bugs, bitfield */
#define VIDIOCQCSCOMPATIBLE     _IOWR('v',QC_IOCTLBASE+10,int)  /* Set enable workaround for bugs, bitfield */

    int reg = 2; /* enable double buffering */
    ::ioctl (videoFd, VIDIOCQCSCOMPATIBLE, &reg);
  }

    
  // set height and width
  frameHeight = PMIN (videoCapability.maxheight, QCIFHeight);
  frameWidth  = PMIN (videoCapability.maxwidth, QCIFWidth);
  

  // Init audio
  struct video_audio videoAudio;
  if (::ioctl(videoFd, VIDIOCGAUDIO, &videoAudio) >= 0 &&
                      (videoAudio.flags & VIDEO_AUDIO_MUTABLE) != 0) {
    videoAudio.flags &= ~VIDEO_AUDIO_MUTE;
    videoAudio.mode = VIDEO_SOUND_MONO;
    ::ioctl(videoFd, VIDIOCSAUDIO, &videoAudio);
    }

  SetVideoFormat(videoFormat);
  SetColourFormat(colourFormat);

  return PTrue;
}


PBoolean PVideoInputDevice_V4L::IsOpen() 
{
  return videoFd >= 0;
}


PBoolean PVideoInputDevice_V4L::Close()
{
  if (!IsOpen())
    return PFalse;


  // Mute audio
  struct video_audio videoAudio;
  if (::ioctl(videoFd, VIDIOCGAUDIO, &videoAudio) >= 0 &&
                      (videoAudio.flags & VIDEO_AUDIO_MUTABLE) != 0) {
    videoAudio.flags |= VIDEO_AUDIO_MUTE;
    ::ioctl(videoFd, VIDIOCSAUDIO, &videoAudio);
  }

  ClearMapping();
  ::close(videoFd);

  videoFd = -1;
  canMap  = -1;
  
  return PTrue;
}


PBoolean PVideoInputDevice_V4L::Start()
{
  return PTrue;
}


PBoolean PVideoInputDevice_V4L::Stop()
{
  return PTrue;
}


PBoolean PVideoInputDevice_V4L::IsCapturing()
{
  return IsOpen();
}


PStringList PVideoInputDevice_V4L::GetInputDeviceNames()
{
  return GetNames().GetInputDeviceNames();
}

PBoolean PVideoInputDevice_V4L::SetVideoFormat(VideoFormat newFormat)
{
  if (!PVideoDevice::SetVideoFormat(newFormat)) {
    PTRACE(1,"PVideoDevice::SetVideoFormat\t failed");
    return PFalse;
  }

  // The channel and format are both set at the same time with one ioctl().
  // Get the channel information (to check if channel is valid)
  // Note: If the channel is -1, we need to search for the first valid channel
  if (channelNumber == -1) {
    if (!SetChannel(channelNumber)){
      PTRACE(1,"PVideoDevice::Cannot set default channel in SetVideoFormat");
      return PFalse;
    }
  }

  struct video_channel channel;
  channel.channel = channelNumber;
  if (::ioctl(videoFd, VIDIOCGCHAN, &channel) < 0) {
    PTRACE(1,"VideoInputDevice Get Channel info failed : "<< ::strerror(errno));    
    return PFalse;
  }
  
  // set channel information
  static int fmt[4] = { VIDEO_MODE_PAL, VIDEO_MODE_NTSC, 
                          VIDEO_MODE_SECAM, VIDEO_MODE_AUTO };
  channel.norm = fmt[newFormat];

  // set the information
  if (::ioctl(videoFd, VIDIOCSCHAN, &channel) >= 0) {
    // format change might affect frame size limits; grab them again
    RefreshCapabilities();
    return PTrue;
  }

  PTRACE(1,"VideoInputDevice SetChannel failed : "<< ::strerror(errno));  

  if (newFormat != Auto)
    return PFalse;

  if (SetVideoFormat(PAL))
    return PTrue;
  if (SetVideoFormat(NTSC))
    return PTrue;
  if (SetVideoFormat(SECAM))
    return PTrue;

  return PFalse;
}


int PVideoInputDevice_V4L::GetNumChannels() 
{
  /* If Opened, return the capability value, else 1 as in videoio.cxx */
  if (IsOpen ())
    return videoCapability.channels;
  else
    return 1;
}


PBoolean PVideoInputDevice_V4L::SetChannel(int newChannel)
{
  if (!PVideoDevice::SetChannel(newChannel))
    return PFalse;

  // get channel information (to check if channel is valid)
  struct video_channel channel;
  channel.channel = channelNumber;
  if (::ioctl(videoFd, VIDIOCGCHAN, &channel) < 0) {
    PTRACE(1,"VideoInputDevice:: Get info on channel " << channelNumber << " failed : "<< ::strerror(errno));    
    return PFalse;
  }
  
  // set channel information
  channel.channel = channelNumber;

  // set the information
  if (::ioctl(videoFd, VIDIOCSCHAN, &channel) < 0) {
    PTRACE(1,"VideoInputDevice:: Set info on channel " << channelNumber << " failed : "<< ::strerror(errno));    
    return PFalse;
  }
  
  // it's unlikely that channel change would affect frame size limits,
  // but grab them again all the same
  RefreshCapabilities();
  return PTrue;
}


PBoolean PVideoInputDevice_V4L::SetVideoChannelFormat (int newNumber, VideoFormat videoFormat) 
{
  if (!PVideoDevice::SetChannel(newNumber))
    return PFalse;

  if (!PVideoDevice::SetVideoFormat(videoFormat)) {
    PTRACE(1,"PVideoDevice::SetVideoFormat\t failed");
    return PFalse;
  }

  static int fmt[4] = { VIDEO_MODE_PAL, VIDEO_MODE_NTSC, 
                          VIDEO_MODE_SECAM, VIDEO_MODE_AUTO };

  // select the specified input and video format
  // get channel information (to check if channel is valid)
  struct video_channel channel;
  channel.channel = channelNumber;
  if (::ioctl(videoFd, VIDIOCGCHAN, &channel) < 0) {
    PTRACE(1,"VideoInputDevice Get Channel info failed : "<< ::strerror(errno));    

    return PFalse;
  }
  
  // set channel information
  channel.norm = fmt[videoFormat];
  channel.channel = channelNumber;

  // set the information
  if (::ioctl(videoFd, VIDIOCSCHAN, &channel) < 0) {
    PTRACE(1,"VideoInputDevice SetChannel failed : "<< ::strerror(errno));  

    return PFalse;
  }

  // format change may affect frame size limits
  RefreshCapabilities();
  return PTrue;
}

PBoolean PVideoInputDevice_V4L::SetColourFormat(const PString & newFormat)
{
  PINDEX colourFormatIndex = 0;
  while (newFormat != colourFormatTab[colourFormatIndex].colourFormat) {
    colourFormatIndex++;
    if (colourFormatIndex >= PARRAYSIZE(colourFormatTab))
      return PFalse;
  }

  if (!PVideoDevice::SetColourFormat(newFormat))
    return PFalse;

  ClearMapping();

  // get current picture information
  struct video_picture pictureInfo;
  if (::ioctl(videoFd, VIDIOCGPICT, &pictureInfo) < 0) {
    PTRACE(1,"PVideoInputDevice_V4L::Get pict info failed : "<< ::strerror(errno));
    return PFalse;
  }
  
  // set colour format
  colourFormatCode = colourFormatTab[colourFormatIndex].code;
  pictureInfo.palette = colourFormatCode;
  if (HINT (HINT_FORCE_DEPTH_16))
    pictureInfo.depth = 16;

  // set the information
  if (::ioctl(videoFd, VIDIOCSPICT, &pictureInfo) < 0) {
    PTRACE(1,"PVideoInputDevice_V4L::Set pict info failed : "<< ::strerror(errno));
    PTRACE(1,"PVideoInputDevice_V4L:: used code of "<<colourFormatCode);
    PTRACE(1,"PVideoInputDevice_V4L:: palette: "<<colourFormatTab[colourFormatIndex].colourFormat);
    return PFalse;
  }
  

  // Driver only (and always) manages to set the colour format  with call to VIDIOCSPICT.
  if( (HINT(HINT_ONLY_WORKS_PREF_PALETTE) ) &&                   
      ( colourFormatCode == driver_hints[hint_index].pref_palette) ) {
    PTRACE(3,"PVideoInputDevice_V4L:: SetColourFormat succeeded with "<<newFormat);
    return PTrue;
  }

  // Some drivers always return success for CSPICT, and can't
  // read the current palette back in CGPICT.  We can't do much
  // more than just check to see if there is a preferred palette,
  // and fail if the request isn't the preferred palette.
  
  if (HINT(HINT_CSPICT_ALWAYS_WORKS) &&
      HINT(HINT_CGPICT_DOESNT_SET_PALETTE) &&
      HINT(HINT_HAS_PREF_PALETTE)) {
      if (colourFormatCode != driver_hints[hint_index].pref_palette)
        return PFalse;
  }

  // Some V4L drivers can't use CGPICT to check for errors.
  if (!HINT(HINT_CGPICT_DOESNT_SET_PALETTE)) {
    if (::ioctl(videoFd, VIDIOCGPICT, &pictureInfo) < 0) {
      PTRACE(1,"PVideoInputDevice_V4L::Get pict info failed : "<< ::strerror(errno));
      return PFalse;
    }
    
    if (pictureInfo.palette != colourFormatCode)
      return PFalse;
  }
  
  // set the new information
  return SetFrameSizeConverter(frameWidth, frameHeight);
}


PBoolean PVideoInputDevice_V4L::SetFrameRate(unsigned rate)
{
  if (!PVideoDevice::SetFrameRate(rate))
    return PFalse;

  return PTrue;
}


PBoolean PVideoInputDevice_V4L::GetFrameSizeLimits(unsigned & minWidth,
                                           unsigned & minHeight,
                                           unsigned & maxWidth,
                                           unsigned & maxHeight) 
{
  if (!IsOpen())
    return PFalse;

  if(HINT(HINT_FORCE_LARGE_SIZE)) {
    videoCapability.maxheight = 288;
    videoCapability.maxwidth  = 352;
    videoCapability.minheight = 288;
    videoCapability.minwidth  = 352;
  }

  maxHeight = videoCapability.maxheight;
  maxWidth  = videoCapability.maxwidth;
  minHeight = videoCapability.minheight;
  minWidth  = videoCapability.minwidth;
    
  PTRACE(3,"PVideoInputDevice_V4L:\t GetFrameSizeLimits. "<<minWidth<<"x"<<minHeight<<" -- "<<maxWidth<<"x"<<maxHeight);
  
  return PTrue;
}


PBoolean PVideoInputDevice_V4L::SetFrameSize(unsigned width, unsigned height)
{
  PTRACE(5, "PVideoInputDevice_V4L\t SetFrameSize " << width <<"x"<<height << " Initiated.");
  if (!PVideoDevice::SetFrameSize(width, height)) {
    PTRACE(3,"PVideoInputDevice_V4L\t SetFrameSize "<<width<<"x"<<height<<" FAILED");
    return PFalse;
  }

  ClearMapping();
  
  if (!VerifyHardwareFrameSize(width, height)) {
    PTRACE(3,"PVideoInputDevice_V4L\t SetFrameSize failed for "<<width<<"x"<<height);
    PTRACE(3,"VerifyHardwareFrameSize failed.");
    return PFalse;
  }

  frameBytes = CalculateFrameBytes(frameWidth, frameHeight, colourFormat);
  
  return PTrue;
}


PINDEX PVideoInputDevice_V4L::GetMaxFrameBytes()
{
  return GetMaxFrameBytesConverted(frameBytes);
}


PBoolean PVideoInputDevice_V4L::GetFrameData(BYTE *buffer, PINDEX *bytesReturned)
{
  m_pacing.Delay(1000/GetFrameRate());
  return GetFrameDataNoDelay(buffer, bytesReturned);
}


PBoolean PVideoInputDevice_V4L::GetFrameDataNoDelay(BYTE * buffer, PINDEX * bytesReturned)
{
  if (canMap < 0) {
    //When canMap is < 0, it is the first use of GetFrameData. Check for memory mapping.
    if (::ioctl(videoFd, VIDIOCGMBUF, &frame) < 0) {
      canMap=0;
      PTRACE(3, "VideoGrabber " << deviceName << " cannot do memory mapping - GMBUF failed.");
      //This video device cannot do memory mapping.
    } else {
      videoBuffer = (BYTE *)::mmap(0, frame.size, PROT_READ|PROT_WRITE, MAP_SHARED, videoFd, 0);
     
      if (videoBuffer < 0) {
        canMap = 0;
        PTRACE(3, "VideoGrabber " << deviceName << " cannot do memory mapping - ::mmap failed.");
        //This video device cannot do memory mapping.
      } else {
        canMap = 1;

        frameBuffer[0].frame  = 0;
        frameBuffer[0].format = colourFormatCode;
        frameBuffer[0].width  = frameWidth;
        frameBuffer[0].height = frameHeight;

        frameBuffer[1].frame  = 1;
        frameBuffer[1].format = colourFormatCode;
        frameBuffer[1].width  = frameWidth;
        frameBuffer[1].height = frameHeight;

        currentFrame = 0;
        int ret;
        ret = ::ioctl(videoFd, VIDIOCMCAPTURE, &frameBuffer[currentFrame]);
        if (ret < 0) {
          PTRACE(1,"PVideoInputDevice_V4L::GetFrameData mcapture1 failed : " << ::strerror(errno));
          ClearMapping();  
          canMap = 0;
          //This video device cannot do memory mapping.
        }
        pendingSync[currentFrame] = PTrue;
      }
    }
  }

  if (canMap == 0) 
    {
      return NormalReadProcess(buffer, bytesReturned);
    }

  /*****************************
   * The xawtv package from http://bytesex.org/xawtv/index.html
   * contains a programming-FAQ by Gerd Knorr.
   * For streaming video with video4linux at the full frame rate 
   * (25 hz PAL, 30 hz NTSC) you need to, 
   *
   *   videoiomcapture frame 0                         (setup)
   *
   * loop:
   *   videoiomcapture frame 1   (returns immediately)
   *   videoiocsync    frame 0   (waits on the data)
   *  goto loop:
   *
   * the loop body could also have been:
   *   videoiomcapture frame 0   (returns immediately)
   *   videoiocsync    frame 1   (waits on the data)
   *  
   * The driver requires each mcapture has a corresponding sync. 
   * Thus, you use the pendingSync array.
   *
   * After the loop is finished, you need a videoiocsync 0.
   */

  // trigger capture of next frame in this buffer.
  // fallback to read() on errors.
  int ret = -1;
  
  ret = ::ioctl(videoFd, VIDIOCMCAPTURE, &frameBuffer[ 1 - currentFrame ]);
  if ( ret < 0 ) {
    PTRACE(1,"PVideoInputDevice_V4L::GetFrameData mcapture2 failed : " << ::strerror(errno));
    ClearMapping();
    canMap = 0;
    
    return NormalReadProcess(buffer, bytesReturned);
  }
  pendingSync[ 1 - currentFrame ] = PTrue;
  
  // device does support memory mapping, get data

  // wait for the frame to load. 
  ret = ::ioctl(videoFd, VIDIOCSYNC, &currentFrame);
  pendingSync[currentFrame] = PFalse;    
  if (ret < 0) {
    PTRACE(1,"PVideoInputDevice_V4L::GetFrameData csync failed : " << ::strerror(errno));
    ClearMapping();
    canMap = 0;
 
    return NormalReadProcess(buffer, bytesReturned);
  }
 
  // If converting on the fly do it from frame store to output buffer, otherwise do
  // straight copy.
  if (converter != NULL)
      converter->Convert(videoBuffer + frame.offsets[currentFrame], buffer, bytesReturned);
  else {
    memcpy(buffer, videoBuffer + frame.offsets[currentFrame], frameBytes);
    if (bytesReturned != NULL)
      *bytesReturned = frameBytes;
  }
  
  // change buffers
  currentFrame = 1 - currentFrame;

  return PTrue;
}

//This video device does not support memory mapping - so 
// use normal read process to extract a frame of video data.
PBoolean PVideoInputDevice_V4L::NormalReadProcess(BYTE *resultBuffer, PINDEX *bytesReturned)
{ 

   ssize_t ret;
   ret = -1;
   while (ret < 0) {

     ret = ::read(videoFd, resultBuffer, frameBytes);
     if ((ret < 0) && (errno == EINTR))
       continue;
    
      if (ret < 0) {
        PTRACE(1,"PVideoInputDevice_V4L::NormalReadProcess() failed");
        return PFalse;
      }      
    }

    if ((PINDEX)ret != frameBytes) {
      PTRACE(1,"PVideoInputDevice_V4L::NormalReadProcess() returned a short read");
      // Not a completely fatal. Maybe it should return PFalse instead of a partial
      // image though?
      // return PFalse;
    }
    
    if (converter != NULL)
      return converter->ConvertInPlace(resultBuffer, bytesReturned);

    if (bytesReturned != NULL)
      *bytesReturned = frameBytes;

    return PTrue;
}

void PVideoInputDevice_V4L::ClearMapping()
{
  if ((canMap == 1) && (videoBuffer != NULL)) {
    for (int i=0; i<2; i++) {
      if (pendingSync[i]) {
        int res = ::ioctl(videoFd, VIDIOCSYNC, &i);
        if (res < 0) 
          PTRACE(1,"PVideoInputDevice_V4L::GetFrameData csync failed : " << ::strerror(errno));
          pendingSync[i] = PFalse;    
        }
        ::munmap(videoBuffer, frame.size);
    }
  }
  
  canMap = -1;   
  videoBuffer = NULL;
}



PBoolean PVideoInputDevice_V4L::VerifyHardwareFrameSize(unsigned width, unsigned height)
{
  struct video_window vwin;

  if (HINT(HINT_FORCE_LARGE_SIZE)) {
    if(  (width==352) && (height==288) ) {
      PTRACE(3,"PVideoInputDevice_V4L\t VerifyHardwareFrameSize USB OK  352x288 ");
      return PTrue;
    } else {
      PTRACE(3,"PVideoInputDevice_V4L\t VerifyHardwareFrameSize USB FAIL "<<width<<"x"<<height);
      return PFalse;
    }
  }
    
  if (HINT(HINT_ALWAYS_WORKS_320_240) &&  (width==320) && (height==240) ) {
    PTRACE(3,"PVideoInputDevice_V4L\t VerifyHardwareFrameSize OK  for  320x240 ");
    return PTrue;
  }
    
  if (HINT(HINT_ALWAYS_WORKS_640_480) &&  (width==640) && (height==480) ) {
    PTRACE(3,"PVideoInputDevice_V4L\t VerifyHardwareFrameSize OK for 640x480 ");
    return PTrue;
  }
     
  if (HINT(HINT_CGWIN_FAILS)) {
    PTRACE(3,"PVideoInputDevice_V4L\t VerifyHardwareFrameSize fails for size "
            << width << "x" << height);
    return PFalse;
  }
  
  // Request current hardware frame size
  if (::ioctl(videoFd, VIDIOCGWIN, &vwin) < 0) {
    PTRACE(3,"PVideoInputDevice_V4L\t VerifyHardwareFrameSize VIDIOCGWIN1 error::" << ::strerror(errno));
    return PFalse;
  }

  // Request the width and height
  vwin.width  = width;
  vwin.height = height;
  
  // The only defined flags appear to be as status indicators
  // returned in the CGWIN call.  At least the bttv driver fails
  // when flags isn't zero.  Check the driver hints for clearing
  // the flags.
  if (HINT(HINT_CSWIN_ZERO_FLAGS)) {
    PTRACE(1,"PVideoInputDevice_V4L\t VerifyHardwareFrameSize: Clearing flags field");
    vwin.flags = 0;
  }
  
  ::ioctl(videoFd, VIDIOCSWIN, &vwin);
  
  // Read back settings to be careful about existing (broken) V4L drivers
  if (::ioctl(videoFd, VIDIOCGWIN, &vwin) < 0) {
    PTRACE(3,"PVideoInputDevice_V4L\t VerifyHardwareFrameSize VIDIOCGWIN2 error::" << ::strerror(errno));
    return PFalse;
  }
  
  if ((vwin.width != width) || (vwin.height != height)) {
    PTRACE(3,"PVideoInputDevice_V4L\t VerifyHardwareFrameSize Size mismatch.");
    return PFalse;
  }

  return PTrue;
}

int PVideoInputDevice_V4L::GetBrightness() 
{ 
  if (!IsOpen())
    return -1;

  struct video_picture vp;

  if (::ioctl(videoFd, VIDIOCGPICT, &vp) < 0)
    return -1;
  frameBrightness = vp.brightness;

  return frameBrightness; 
}


int PVideoInputDevice_V4L::GetWhiteness() 
{ 
  if (!IsOpen())
    return -1;

  struct video_picture vp;

  if (::ioctl(videoFd, VIDIOCGPICT, &vp) < 0)
    return -1;
  frameWhiteness = vp.whiteness;

  return frameWhiteness;
}

int PVideoInputDevice_V4L::GetColour() 
{ 
  if (!IsOpen())
    return -1;

  struct video_picture vp;

  if (::ioctl(videoFd, VIDIOCGPICT, &vp) < 0)
    return -1;
  frameColour = vp.colour;

  return frameColour; 
}



int PVideoInputDevice_V4L::GetContrast() 
{
  if (!IsOpen())
    return -1;

  struct video_picture vp;

  if (::ioctl(videoFd, VIDIOCGPICT, &vp) < 0)
    return -1;
  frameContrast = vp.contrast;

 return frameContrast; 
}

int PVideoInputDevice_V4L::GetHue() 
{
  if (!IsOpen())
    return -1;

  struct video_picture vp;

  if (::ioctl(videoFd, VIDIOCGPICT, &vp) < 0)
    return -1;
  frameHue = vp.hue;

  return frameHue; 
}

PBoolean PVideoInputDevice_V4L::SetBrightness(unsigned newBrightness) 
{ 
  if (!IsOpen())
    return PFalse;

  struct video_picture vp;

  if (::ioctl(videoFd, VIDIOCGPICT, &vp) < 0)
    return PFalse;

  vp.brightness = newBrightness;
  if (::ioctl(videoFd, VIDIOCSPICT, &vp) < 0)
    return PFalse;

  frameBrightness=newBrightness;
  return PTrue;
}
PBoolean PVideoInputDevice_V4L::SetWhiteness(unsigned newWhiteness) 
{ 
  if (!IsOpen())
    return PFalse;

  struct video_picture vp;

  if (::ioctl(videoFd, VIDIOCGPICT, &vp) < 0)
    return PFalse;

  vp.whiteness = newWhiteness;
  if (::ioctl(videoFd, VIDIOCSPICT, &vp) < 0)
    return PFalse;

  frameWhiteness = newWhiteness;
  return PTrue;
}

PBoolean PVideoInputDevice_V4L::SetColour(unsigned newColour) 
{ 
  if (!IsOpen())
    return PFalse;

  struct video_picture vp;

  if (::ioctl(videoFd, VIDIOCGPICT, &vp) < 0)
    return PFalse;

  vp.colour = newColour;
  if (::ioctl(videoFd, VIDIOCSPICT, &vp) < 0)
    return PFalse;

  frameColour = newColour;
  return PTrue;
}
PBoolean PVideoInputDevice_V4L::SetContrast(unsigned newContrast) 
{ 
  if (!IsOpen())
    return PFalse;

  struct video_picture vp;

  if (::ioctl(videoFd, VIDIOCGPICT, &vp) < 0)
    return PFalse;

  vp.contrast = newContrast;
  if (::ioctl(videoFd, VIDIOCSPICT, &vp) < 0)
    return PFalse;

  frameContrast = newContrast;
  return PTrue;
}

PBoolean PVideoInputDevice_V4L::SetHue(unsigned newHue) 
{
  if (!IsOpen())
    return PFalse;

  struct video_picture vp;

  if (::ioctl(videoFd, VIDIOCGPICT, &vp) < 0)
    return PFalse;

  vp.hue = newHue;
  if (::ioctl(videoFd, VIDIOCSPICT, &vp) < 0)
    return PFalse;

   frameHue=newHue; 
  return PTrue;
}

PBoolean PVideoInputDevice_V4L::GetParameters (int *whiteness, int *brightness, 
                                      int *colour, int *contrast, int *hue)
{
  if (!IsOpen())
    return PFalse;

  struct video_picture vp;

  if (::ioctl(videoFd, VIDIOCGPICT, &vp) < 0)
    {
      PTRACE(3, "GetParams bombs out!");
      return PFalse;
    }

  *brightness = vp.brightness;
  *colour     = vp.colour;
  *contrast   = vp.contrast;
  *hue        = vp.hue;
  *whiteness  = vp.whiteness;

  frameBrightness = *brightness;
  frameColour     = *colour;
  frameContrast   = *contrast;
  frameHue        = *hue;
  frameWhiteness  = *whiteness;
 
  return PTrue;
}

PBoolean PVideoInputDevice_V4L::TestAllFormats()
{
  return PTrue;
}

PBoolean PVideoInputDevice_V4L::RefreshCapabilities()
{
  if (::ioctl(videoFd, VIDIOCGCAP, &videoCapability) < 0)  {
    PTRACE(1,"PVideoInputV4lDevice:: get device capablilities failed : "<< ::strerror(errno));
    return PFalse;
  }
  return PTrue;
}


// End Of File ///////////////////////////////////////////////////////////////
