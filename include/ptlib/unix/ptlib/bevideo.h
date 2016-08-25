
#include <ptlib.h>
#include <ptlib/video.h>
#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>

class VideoConsumer;
class BMediaRoster;
class PVideoInputThread;
#include <MediaNode.h>

/**This class defines a BeOS video input device.
 */
class PVideoInputDevice_BeOSVideo : public PVideoInputDevice
{
  PCLASSINFO(PVideoInputDevice_BeOSVideo, PVideoInputDevice);

  public:
    /** Create a new video input device.
     */
    PVideoInputDevice_BeOSVideo();

    /**Close the video input device on destruction.
      */
    ~PVideoInputDevice_BeOSVideo() { Close(); }

    /** Is the device a camera, and obtain video
     */
    static PStringList GetInputDeviceNames();

    virtual PStringList GetDeviceNames() const
      { return GetInputDeviceNames(); }

    /**Open the device given the device name.
      */
    virtual PBoolean Open(
      const PString & deviceName,   /// Device name to open
      PBoolean startImmediate = PTrue    /// Immediately start device
    );

    /**Determine if the device is currently open.
      */
    virtual PBoolean IsOpen();

    /**Close the device.
      */
    virtual PBoolean Close();

    /**Start the video device I/O.
      */
    virtual PBoolean Start();

    /**Stop the video device I/O capture.
      */
    virtual PBoolean Stop();

    /**Determine if the video device I/O capture is in progress.
      */
    virtual PBoolean IsCapturing();

    /**Get the maximum frame size in bytes.

       Note a particular device may be able to provide variable length
       frames (eg motion JPEG) so will be the maximum size of all frames.
      */
    virtual PINDEX GetMaxFrameBytes();

    /**Grab a frame.
      */
    virtual PBoolean GetFrame(
      PBYTEArray & frame
    );

    /**Grab a frame, after a delay as specified by the frame rate.
      */
    virtual PBoolean GetFrameData(
      BYTE * buffer,                 /// Buffer to receive frame
      PINDEX * bytesReturned = NULL  /// OPtional bytes returned.
    );

    /**Grab a frame. Do not delay according to the current frame rate parameter.
      */
    virtual PBoolean GetFrameDataNoDelay(
      BYTE * buffer,                 /// Buffer to receive frame
      PINDEX * bytesReturned = NULL  /// OPtional bytes returned.
    );


    /**Try all known video formats & see which ones are accepted by the video driver
     */
    virtual PBoolean TestAllFormats();

  public:
    virtual PBoolean SetColourFormat(const PString & colourFormat);
    virtual PBoolean SetFrameRate(unsigned rate);
    virtual PBoolean SetFrameSize(unsigned width, unsigned height);

    friend PVideoInputThread;
  private:
    status_t StartNodes();
    void StopNodes();

  protected:
    BMediaRoster* fMediaRoster;
    VideoConsumer* fVideoConsumer;

    media_output fProducerOut;
    media_input fConsumerIn;

    media_node fTimeSourceNode;
    media_node fProducerNode;

    port_id fPort;

    PBoolean          isCapturingNow;
    PVideoInputThread* captureThread;
    
};

