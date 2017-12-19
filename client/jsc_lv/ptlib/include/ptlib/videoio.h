/*
 * videoio.h
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
 * Contributor(s): Mark Cooke (mpc@star.sr.bham.ac.uk)
 *
 * $Revision: 23962 $
 * $Author: rjongbloed $
 * $Date: 2010-01-22 04:39:06 +0000 (Fri, 22 Jan 2010) $
 */


#ifndef PTLIB_PVIDEOIO_H
#define PTLIB_PVIDEOIO_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif
#include <ptbuildopts.h>

#if P_VIDEO

#include <ptlib/plugin.h>
#include <ptlib/pluginmgr.h>
#include <list>

class PColourConverter;


class PVideoFrameInfo : public PObject
{
  PCLASSINFO(PVideoFrameInfo, PObject);

  public:
    enum ResizeMode
    {
        eScale,
        eCropCentre,
        eCropTopLeft,
        eMaxResizeMode
    };

    enum StandardSizes {
      SQCIFWidth = 128,  SQCIFHeight = 96,
      QCIFWidth  = 176,  QCIFHeight  = 144,
      CIFWidth   = 352,  CIFHeight   = 288,
      CIF4Width  = 704,  CIF4Height  = 576,
      CIF16Width = 1408, CIF16Height = 1152,
      i480Width  = 640,  i480Height  = 480,
      p720Width  = 960,  p720Height  = 720,
      i1080Width = 1920, i1080Height = 1080,
      HDTVWidth  = 1920, HDTVHeight  = 1080,
      MaxWidth   = 1920, MaxHeight   = 1152
    };

    /// Construct video frame information
    PVideoFrameInfo();
    PVideoFrameInfo(
      unsigned        frameWidth,
      unsigned        frameHeight,
      const PString & colourFormat = "YUV420P",
      unsigned        frameRate = 15,
      ResizeMode      resizeMode = eScale
    );
    bool operator <(PVideoFrameInfo& item) /*ע�����ء�*/
    {
      if( frameWidth < item.frameWidth && frameHeight<item.frameHeight ){
        return true;
      }else if (frameWidth== item.frameWidth && frameHeight<item.frameHeight ){
        return true;
      }else if (frameWidth< item.frameWidth && frameHeight==item.frameHeight ){
        return true;
      }
      else
        return false;
    };
    /**Set the frame size to be used.

       Default behaviour sets the frameWidth and frameHeight variables and
       returns PTrue.
    */
    virtual PBoolean SetFrameSize(
      unsigned width,   ///< New width of frame
      unsigned height   ///< New height of frame
    );

    /**Get the frame size being used.

       Default behaviour returns the value of the frameWidth and frameHeight
       variable and returns PTrue.
    */
    virtual PBoolean GetFrameSize(
      unsigned & width,
      unsigned & height
    ) const;

    /** Get the width of the frame being used.

        Default behaviour returns the value of the frameWidth variable
    */
    virtual unsigned GetFrameWidth() const;

    /** Get the height of the frame being used.

        Default behaviour returns the value of the frameHeight variable
    */
    virtual unsigned GetFrameHeight() const;

    /**Set the video frame rate to be used on the device.

       Default behaviour sets the value of the frameRate variable and then
       returns PTrue.
    */
    virtual PBoolean SetFrameRate(
      unsigned rate  ///< Frames  per second
    );

    /**Get the video frame rate used on the device.

       Default behaviour returns the value of the frameRate variable.
    */
    virtual unsigned GetFrameRate() const;

    /**Set the colour format to be used.

       Default behaviour sets the value of the colourFormat variable and then
       returns PTrue if not an empty string.
    */
    virtual PBoolean SetColourFormat(
      const PString & colourFormat // New colour format for device.
    );

    /**Get the colour format to be used.

       Default behaviour returns the value of the colourFormat variable.
    */
    virtual const PString & GetColourFormat() const;

    /**Set the resize mode to be used.
    */
    void SetResizeMode(
      ResizeMode mode
    ) { if (resizeMode < eMaxResizeMode) resizeMode = mode; }

    /**Get the resize mode to be used.
    */
    ResizeMode GetResizeMode() const { return resizeMode; }

    /** Get the number of bytes of an image, given a particular width, height and colour format.
      */
    PINDEX CalculateFrameBytes() const { return CalculateFrameBytes(frameWidth, frameHeight, colourFormat); }
    static PINDEX CalculateFrameBytes(
      unsigned width,               ///< WIdth of frame
      unsigned height,              ///< Height of frame
      const PString & colourFormat  ///< Colour format of frame
    );

    /** Parse a descriptor string for the video format.
        This is of the form [fmt ':' ] size [ '@' rate][ '/' crop ]. The size component
        is as for the ParseSize() function.

        The fmt string is the colour format such as "RGB32", "YUV420P" etc.

        The rate field is a simple integer from 1 to 100.

        The crop field is one of "scale", "resize" (synonym for "scale"),
        "centre", "center", "topleft" or "crop" (synonym for "topleft").

        Note no spaces are allowed in the descriptor.
      */
    bool Parse(
      const PString & str   ///< String to parse
    );

    /** Parse the standard size string names.
        This will parse a size desciption using either standard names: "qcif",
        "cif", "vga", "hd1080" etc or WxY form e.g. "640x480".
      */
    static bool ParseSize(
      const PString & str,  ///< String to parse
      unsigned & width,     ///< Resultant width
      unsigned & height     ///< Resulatant height
    );

    /**Get a width/height as a standard size string name.
      */
    static PString AsString(
      unsigned width,     ///< Width to convert
      unsigned height     ///< Height to convert
    );

  protected:
    unsigned   frameWidth;
    unsigned   frameHeight;
    unsigned   frameRate;
    PString    colourFormat;
    ResizeMode resizeMode;
};


class PVideoControlInfo : public PObject
{
  PCLASSINFO(PVideoControlInfo, PObject);

 public:

	 typedef enum {
        ControlPan,
        ControlTilt,
        ControlZoom
     } InputControlType;

 	static PString AsString(const InputControlType & type);

	  InputControlType type;
	  long					  min;
	  long					  max;
	  long					  step;
	  long					  def;
	  long					  flags;
	  long					  current;
};


/**This class defines a video Input device control (Camera controls PTZ)
*/

class PVideoInputControl : public PVideoControlInfo
{
    PCLASSINFO(PVideoInputControl, PVideoControlInfo);

public:
	~PVideoInputControl();

	virtual PBoolean Pan(long value, bool absolute = false );
	virtual PBoolean Tilt(long value, bool absolute = false);
	virtual PBoolean Zoom(long value, bool absolute = false);

	long GetPan();
	long GetTilt();
	long GetZoom();

	void Reset();
	void SetCurrentPosition(const InputControlType ctype, long current);

	typedef std::list<PVideoControlInfo> InputDeviceControls;

protected:
	PBoolean GetVideoControlInfo(const InputControlType ctype, PVideoControlInfo & control);
	PBoolean GetDefaultPosition(const InputControlType ctype, long & def);
	PBoolean GetCurrentPosition(const InputControlType ctype, long & current);

	std::list<PVideoControlInfo> m_info;
	PMutex ccmutex;

};

/**This class defines a video Input device Interactions (Remote Inputs/Controls)
*/
class PVideoInteractionInfo : public PObject
{
  PCLASSINFO(PVideoInteractionInfo, PObject);

 public:

	 typedef enum {
        InteractKey,		/// Register remote KeyPresses
        InteractMouse,		/// Register remote Mouse Movements\Clicks
        InteractNavigate,	/// Register remote Navigation commands
		InteractRTSP,		/// Register remote RTSP (Real Time Streaming Protocol) Inputs
		InteractOther		/// Register remote application specific Inputs
     } InputInteractType;

 	static PString AsString(const InputInteractType & type);

	InputInteractType type;
};


/**This class defines a video device.
   This class is used to abstract the few parameters that are common to both\
   input and output devices.

   Example colour formats are:

     "Grey"     Simple 8 bit linear grey scale
     "Gray"     Synonym for Grey
     "RGB32"    32 bit RGB
     "RGB24"    24 bit RGB
     "RGB565"   16 bit RGB (6 bit green)
     "RGB555"   15 bit RGB
     "YUV422"   YUV 4:2:2 packed
     "YUV422P"  YUV 4:2:2 planar
     "YUV411"   YUV 4:1:1 packed
     "YUV411P"  YUV 4:1:1 planar
     "YUV420"   YUV 4:2:0 packed
     "YUV420P"  YUV 4:2:0 planar
     "YUV410"   YUV 4:1:0 packed
     "YUV410P"  YUV 4:1:0 planar
     "MJPEG"    Motion JPEG
     "UYVY422"  YUV 4:2:2 packed as U Y V Y U Y V Y ...
     "UYV444    YUV 4:4:4 packed as U Y V   U Y V   ...
                They are used in IEEE 1394 digital cameras. The specification
                is found at
http://www.1394ta.org/Download/Technology/Specifications/2000/IIDC_Spec_v1_30.pdf

 */
class PVideoDevice : public PVideoFrameInfo
{
  PCLASSINFO(PVideoDevice, PVideoFrameInfo);

  protected:
    /** Create a new video device (input or output).
     */
    PVideoDevice();


  public:
    /** Delete structures created by PVideoDevice(); 
     */
    virtual ~PVideoDevice();

    enum VideoFormat {
      PAL,
      NTSC,
      SECAM,
      Auto,
      NumVideoFormats
    };

    /**Get the device name of the open device.
      */
    const PString & GetDeviceName() const
      { return deviceName; }

    /**Get a list of all of the drivers available.
      */
    virtual PStringArray GetDeviceNames() const = 0;

    struct OpenArgs {
      OpenArgs();

      PPluginManager * pluginMgr;
      PString     driverName;
      PString     deviceName;
      VideoFormat videoFormat;
      int         channelNumber;
      PString     colourFormat;
      bool        convertFormat;
      unsigned    rate;
      unsigned    width;
      unsigned    height;
      bool        convertSize;
      ResizeMode  resizeMode;
      bool        flip;
      int         brightness;
      int         whiteness;
      int         contrast;
      int         colour;
      int         hue;
    };

    /**Open the device given the device name.
      */
    virtual PBoolean OpenFull(
      const OpenArgs & args,      ///< Parameters to set on opened device
      PBoolean startImmediate = PTrue  ///< Immediately start device
    );

    /**Open the device given the device name.
      */
    virtual PBoolean Open(
      const PString & deviceName,   ///< Device name to open
      PBoolean startImmediate = PTrue    ///< Immediately start device
    ) = 0;

    /**Determine if the device is currently open.
      */
    virtual PBoolean IsOpen() = 0;

    /**Close the device.
      */
    virtual PBoolean Close() = 0;

    /**Start the video device I/O capture.
      */
    virtual PBoolean Start() = 0;

    /**Stop the video device I/O capture.
      */
    virtual PBoolean Stop() = 0;


#if PTRACING
    friend ostream & operator<<(ostream &, VideoFormat);
#endif

    /**Set the video format to be used.

       Default behaviour sets the value of the videoFormat variable and then
       returns PTrue.
    */
    virtual PBoolean SetVideoFormat(
      VideoFormat videoFormat   ///< New video format
    );

    /**Get the video format being used.

       Default behaviour returns the value of the videoFormat variable.
    */
    virtual VideoFormat GetVideoFormat() const;

    /**Get the number of video channels available on the device.

       Default behaviour returns 1.
    */
    virtual int GetNumChannels();

    /**Set the video channel to be used on the device.
       The channel number is an integer from 0 to GetNumChannels()-1. The
       special value of -1 will find the first working channel number.

       Default behaviour sets the value of the channelNumber variable and then
       returns PTrue.
    */
    virtual PBoolean SetChannel(
      int channelNumber  ///< New channel number for device.
    );

    /**Get the video channel to be used on the device.

       Default behaviour returns the value of the channelNumber variable.
    */
    virtual int GetChannel() const;

    /**Set the colour format to be used, trying converters if available.

       This function will set the colour format on the device to one that
       is compatible with a registered converter, and install that converter
       so that the correct format is used.
    */
    virtual PBoolean SetColourFormatConverter(
      const PString & colourFormat // New colour format for device.
    );

    /**Get the video conversion vertical flip state.
       Default action is to return PFalse.
     */
    virtual PBoolean GetVFlipState();

    /**Set the video conversion vertical flip state.
       Default action is to return PFalse.
     */
    virtual PBoolean SetVFlipState(
      PBoolean newVFlipState    ///< New vertical flip state
    );

    /**Get the minimum & maximum size of a frame on the device.

       Default behaviour returns the value 1 to UINT_MAX for both and returns
       PFalse.
    */
    virtual PBoolean GetFrameSizeLimits(
      unsigned & minWidth,   ///< Variable to receive minimum width
      unsigned & minHeight,  ///< Variable to receive minimum height
      unsigned & maxWidth,   ///< Variable to receive maximum width
      unsigned & maxHeight   ///< Variable to receive maximum height
    ) ;


    /**Set the frame size to be used, trying converters if available.

       If the device does not support the size, a set of alternate resolutions
       are attempted.  A converter is setup if possible.
    */
    virtual PBoolean SetFrameSizeConverter(
      unsigned width,  ///< New width of frame
      unsigned height, ///< New height of frame
      ResizeMode resizeMode = eMaxResizeMode ///< Mode to use if resizing is required.
    );

    /**Set the frame size to be used, trying converters if available.
       Function used for Backward compatibility only.
       If the device does not support the size, a set of alternate resolutions
       are attempted.  A converter is setup if possible.
    */
    virtual PBoolean SetFrameSizeConverter(
      unsigned width,                   ///< New width of frame
      unsigned height,                  ///< New height of frame
	  PBoolean  /*bScaleNotCrop*/           ///< Not used.
	  )  { return SetFrameSizeConverter(width,height,eScale); }


    /**Set the frame size to be used.

       Note that devices may not be able to produce the requested size, and
       this function will fail.  See SetFrameSizeConverter().

       Default behaviour sets the frameWidth and frameHeight variables and
       returns PTrue.
    */
    virtual PBoolean SetFrameSize(
      unsigned width,   ///< New width of frame
      unsigned height   ///< New height of frame
    );

    /**Get the frame size being used.

       Default behaviour returns the value of the frameWidth and frameHeight
       variable and returns PTrue.
    */
    virtual PBoolean GetFrameSize(
      unsigned & width,
      unsigned & height
    ) const;

    /**Get the maximum frame size in bytes.

       Note a particular device may be able to provide variable length
       frames (eg motion JPEG) so will be the maximum size of all frames.
      */
    virtual PINDEX GetMaxFrameBytes() = 0;

    
    /**Get the last error code. This is a platform dependent number.
      */
    int GetLastError() const { return lastError; }


    /** Is the device a camera, and obtain video
     */
    virtual PBoolean CanCaptureVideo() const = 0;

    /**Get the brightness of the image. 0xffff-Very bright. -1 is unknown.
     */
    virtual int GetBrightness();

    /**Set brightness of the image. 0xffff-Very bright.
     */
    virtual PBoolean SetBrightness(unsigned newBrightness);


    /**Get the whiteness of the image. 0xffff-Very white. -1 is unknown.
     */
    virtual int GetWhiteness();

    /**Set whiteness of the image. 0xffff-Very white.
     */
    virtual PBoolean SetWhiteness(unsigned newWhiteness);


    /**Get the colour of the image. 0xffff-lots of colour. -1 is unknown.
     */
    virtual int GetColour();

    /**Set colour of the image. 0xffff-lots of colour.
     */
    virtual PBoolean SetColour(unsigned newColour);


    /**Get the contrast of the image. 0xffff-High contrast. -1 is unknown.
     */
    virtual int GetContrast();

    /**Set contrast of the image. 0xffff-High contrast.
     */
    virtual PBoolean SetContrast(unsigned newContrast);


    /**Get the hue of the image. 0xffff-High hue. -1 is unknown.
     */
    virtual int GetHue();

    /**Set hue of the image. 0xffff-High hue.
     */
    virtual PBoolean SetHue(unsigned newHue);
    
    
    /**Return whiteness, brightness, colour, contrast and hue in one call.
     */
    virtual PBoolean GetParameters(
      int *whiteness,
      int *brightness,
      int *colour,
      int *contrast,
      int *hue
    );

    
    /** Set VideoFormat and VideoChannel in one ioctl
     */
    virtual PBoolean SetVideoChannelFormat (
      int channelNumber, 
      VideoFormat videoFormat
    );


    /**Set preferred native colour format from video capture device.
       Note empty == no preference.
     */
    void SetPreferredColourFormat(const PString & colourFmt) { preferredColourFormat = colourFmt; }

    /**Get preferred native colour format from video capture device.
       Returns empty == no preference
     */
    const PString & GetPreferredColourFormat() { return preferredColourFormat; }
    
  protected:
    PINDEX GetMaxFrameBytesConverted(PINDEX rawFrameBytes) const;

    PString      deviceName;
    int          lastError;
    VideoFormat  videoFormat;
    int          channelNumber;
    // Preferred native colour format from video input device, empty == no preference
    PString      preferredColourFormat;
    PBoolean         nativeVerticalFlip;

    PColourConverter * converter;
    PBYTEArray         frameStore;

    int          frameBrightness; // 16 bit entity, -1 is no value
    int          frameWhiteness;
    int          frameContrast;
    int          frameColour;
    int          frameHue;
};
struct stUserInfo
{
  PString callid;
  unsigned int uid;
};

/**This class defines a video output device.- typically, a window.
 */
class PVideoOutputDevice : public PVideoDevice
{
  PCLASSINFO(PVideoOutputDevice, PVideoDevice);

  public:
    virtual void SetUserYYID(const stUserInfo& ){};
    /** Create a new video output device.
     */
    PVideoOutputDevice();
    
    /**Close the video output device on destruction.
      */
    virtual ~PVideoOutputDevice() { Close(); };      

    /**Get the list of available video output drivers (plug-ins)
    */
    static PStringArray GetDriverNames(
      PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
    );

    /**Get video output devices that correspond to the specified driver name.
       If driverName is an empty string or the value "*" then this will return
       a list of unique device names across all of the available drivers. If
       two drivers have identical names for devices, then the string returned
       will be of the form driver+'\t'+device.
    */
    static PStringArray GetDriversDeviceNames(
      const PString & driverName,         ///< Name of driver
      PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
    );

    /**Create the video output device that corresponds to the specified driver name.
    */
    static PVideoOutputDevice * CreateDevice(
      const PString & driverName,         ///< Name of driver
      PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
    );

    /* Create the matching video output device that corresponds to the device name.

       This is typically used with the return values from GetDriversDeviceNames().
     */
    static PVideoOutputDevice *CreateDeviceByName(
      const PString & deviceName,         ///< Name of device
      const PString & driverName = PString::Empty(),  ///< Name of driver (if any)
      PPluginManager * pluginMgr = NULL,   ///< Plug in manager, use default if NULL
      int userdata=0  ///< Name of driver (if any)
   );

    /**Create an opened video output device that corresponds to the specified names.
       If the driverName parameter is an empty string or "*" then CreateDeviceByName
       is used with the deviceName parameter which is assumed to be a value returned
       from GetDriversDeviceNames().
    */
    static PVideoOutputDevice *CreateOpenedDevice(
      const PString & driverName,         ///< Name of driver
      const PString & deviceName,         ///< Name of device
      PBoolean startImmediate = PTrue,         ///< Immediately start display
      PPluginManager * pluginMgr = NULL ,  ///< Plug in manager, use default if NULL
     int userdata = 0   /// 
    );

    /**Create an opened video output device that corresponds to the specified arguments.
    */
    static PVideoOutputDevice *CreateOpenedDevice(
      const OpenArgs & args,              ///< Parameters to set on opened device
      PBoolean startImmediate = PTrue ,         ///< Immediately start display
     int userdata = 0   /// 
    );

    /**Close the device.
      */
    virtual PBoolean Close() { return PTrue; }

    /**Start the video device I/O display.
      */
    virtual PBoolean Start() { return PTrue; }

    /**Stop the video device I/O display.
      */
    virtual PBoolean Stop() { return PTrue; }

    /** Is the device a camera, and obtain video
     */
    virtual PBoolean CanCaptureVideo() const;

    /**Set a section of the output frame buffer.
      */
    virtual PBoolean SetFrameData(
      unsigned x,
      unsigned y,
      unsigned width,
      unsigned height,
      const BYTE * data,
      PBoolean endFrame = PTrue
    ) = 0;
    virtual PBoolean SetFrameData(
      unsigned x,
      unsigned y,
      unsigned width,
      unsigned height,
      const BYTE * data,
      PBoolean endFrame,
      unsigned flags
    );

    /**Get the position of the output device, where relevant. For devices such as
       files, this always returns zeros. For devices such as Windows, this is the
       position of the window on the screen.
       
       Returns: TRUE if the position is available.
      */
    virtual PBoolean GetPosition(
      int & x,  // X position of device surface
      int & y   // Y position of device surface
    ) const;

    /**Set the position of the output device, where relevant. For devices such as
       files, this does nothing. For devices such as Windows, this sets the
       position of the window on the screen.
       
       Returns: TRUE if the position can be set.
      */
    virtual bool SetPosition(
      int x,  // X position of device surface
      int y   // Y position of device surface
    );
};


/**This class defines a video output device for RGB in a frame store.
 */
class PVideoOutputDeviceRGB : public PVideoOutputDevice
{
  PCLASSINFO(PVideoOutputDeviceRGB, PVideoOutputDevice);

  public:
    /** Create a new video output device.
     */
    PVideoOutputDeviceRGB();
		//virtual	int  GetHWND(){return 0;};
    /**Set the colour format to be used.
       Note that this function does not do any conversion. If it returns PTrue
       then the video device does the colour format in native mode.

       To utilise an internal converter use the SetColourFormatConverter()
       function.

       Default behaviour sets the value of the colourFormat variable and then
       returns PTrue.
    */
    virtual PBoolean SetColourFormat(
      const PString & colourFormat // New colour format for device.
    );

    /**Set the frame size to be used.

       Note that devices may not be able to produce the requested size, and
       this function will fail.  See SetFrameSizeConverter().

       Default behaviour sets the frameWidth and frameHeight variables and
       returns PTrue.
    */
    virtual PBoolean SetFrameSize(
      unsigned width,   ///< New width of frame
      unsigned height   ///< New height of frame
    );

    /**Get the maximum frame size in bytes.

       Note a particular device may be able to provide variable length
       frames (eg motion JPEG) so will be the maximum size of all frames.
      */
    virtual PINDEX GetMaxFrameBytes();

    /**Set a section of the output frame buffer.
      */
    virtual PBoolean SetFrameData(
      unsigned x,
      unsigned y,
      unsigned width,
      unsigned height,
      const BYTE * data,
      PBoolean endFrame = PTrue
    );

    /**Indicate frame may be displayed.
      */
    virtual PBoolean FrameComplete() = 0;

  protected:
    PMutex     mutex;
    PINDEX     bytesPerPixel;
    PINDEX     scanLineWidth;
    bool       swappedRedAndBlue;
};


#ifdef SHOULD_BE_MOVED_TO_PLUGIN

/**This class defines a video output device which outputs to a series of PPM files.
 */
class PVideoOutputDevicePPM : public PVideoOutputDeviceRGB
{
  PCLASSINFO(PVideoOutputDevicePPM, PVideoOutputDeviceRGB);

  public:
    /** Create a new video output device.
     */
    PVideoOutputDevicePPM();

    /**Open the device given the device name.
      */
    virtual PBoolean Open(
      const PString & deviceName,   ///< Device name (filename base) to open
      PBoolean startImmediate = PTrue    ///< Immediately start device
    );

    /**Determine if the device is currently open.
      */
    virtual PBoolean IsOpen();

    /**Close the device.
      */
    virtual PBoolean Close();

    /**Get a list of all of the drivers available.
      */
    virtual PStringArray GetDeviceNames() const;

    /**Indicate frame may be displayed.
      */
    virtual PBoolean EndFrame();

  protected:
    unsigned   frameNumber;
};

#endif // SHOULD_BE_MOVED_TO_PLUGIN


/**This class defines a video input device.
 */
class PVideoInputDevice : public PVideoDevice
{
  PCLASSINFO(PVideoInputDevice, PVideoDevice);

  public:
    virtual PBoolean SetFrameSize2(
      unsigned width,   ///< New width of frame
      unsigned height   ///< New height of frame
      ){
        return PVideoDevice::SetFrameSize(width, height);//chy 
      };

    /** Create a new video input device.
     */
    //PVideoInputDevice();

    /**Close the video input device on destruction.
      */
    ~PVideoInputDevice() { Close(); }

    /**Get the list of available video input drivers (plug-ins)
    */
    static PStringArray GetDriverNames(
      PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
    );

    /**Get video input devices that correspond to the specified driver name.
       If driverName is an empty string or the value "*" then this will return
       a list of unique device names across all of the available drivers. If
       two drivers have identical names for devices, then the string returned
       will be of the form driver+'\t'+device.
    */
    static PStringArray GetDriversDeviceNames(
      const PString & driverName,         ///< Name of driver
      PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
    );

    /**Create the video input device that corresponds to the specified driver name.
    */
    static PVideoInputDevice *CreateDevice(
      const PString & driverName,         ///< Name of driver
      PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
    );

    /* Create the matching video input device that corresponds to the device name.
       So, for "fake" return a device that will generate fake video.
       For "Phillips 680 webcam" (eg) will return appropriate grabber.
       Note that Phillips will return the appropriate grabber also.

       This is typically used with the return values from GetDriversDeviceNames().
     */
    static PVideoInputDevice *CreateDeviceByName(
      const PString & deviceName,         ///< Name of device
      const PString & driverName = PString::Empty(),  ///< Name of driver (if any)
      PPluginManager * pluginMgr = NULL ,  ///< Plug in manager, use default if NULL
      int userdata=0  ///< Name of driver (if any)
    );

    /**Create an opened video input device that corresponds to the specified names.
       If the driverName parameter is an empty string or "*" then CreateDeviceByName
       is used with the deviceName parameter which is assumed to be a value returned
       from GetDriversDeviceNames().
    */
    static PVideoInputDevice *CreateOpenedDevice(
      const PString & driverName,         ///< Name of driver
      const PString & deviceName,         ///< Name of device
      PBoolean startImmediate = PTrue,         ///< Immediately start grabbing
      PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
    );

    /**Create an opened video output device that corresponds to the specified arguments.
    */
    static PVideoInputDevice *CreateOpenedDevice(
      const OpenArgs & args,              ///< Parameters to set on opened device
      PBoolean startImmediate = PTrue          ///< Immediately start display
    );

	typedef struct {
	   std::list<PVideoFrameInfo> framesizes;
	   std::list<PVideoControlInfo> controls;
	   std::list<PVideoInteractionInfo> interactions;
	} Capabilities;

    /**Retrieve a list of Device Capabilities
      */
    virtual bool GetDeviceCapabilities(
      Capabilities * capabilities          ///< List of supported capabilities
    ) const { return GetDeviceCapabilities(GetDeviceName(), capabilities); }

    /**Retrieve a list of Device Capabilities for particular device
      */
    static PBoolean GetDeviceCapabilities(
      const PString & deviceName,           ///< Name of device
      Capabilities * capabilities,          ///< List of supported capabilities
      PPluginManager * pluginMgr = NULL     ///< Plug in manager, use default if NULL
    );

    /**Retrieve a list of Device Capabilities for a particular driver
      */
    static PBoolean GetDeviceCapabilities(
      const PString & deviceName,           ///< Name of device
      const PString & driverName,           ///< Device Driver
      Capabilities * caps,                  ///< List of supported capabilities
      PPluginManager * pluginMgr = NULL     ///< Plug in manager, use default if NULL
    );

    /**Open the device given the device name.
      */
    virtual PBoolean Open(
      const PString & deviceName,   ///< Device name to open
      PBoolean startImmediate = PTrue    ///< Immediately start device
    ) = 0;

    virtual PBoolean Close(
    ) { return PTrue; }

    /** Is the device a camera, and obtain video
     */
    virtual PBoolean CanCaptureVideo() const;
 
    /**Determine if the video device I/O capture is in progress.
      */
    virtual PBoolean IsCapturing() = 0;

    /**Grab a frame.
      */
    virtual PBoolean GetFrame(
      PBYTEArray & frame
    );

    /**Grab a frame, after a delay as specified by the frame rate.
      */
    virtual PBoolean GetFrameData(
      BYTE * buffer,                 ///< Buffer to receive frame
      PINDEX * bytesReturned,        ///< Optional bytes returned.
      unsigned int & flags           ///< optional flags returned
    );
    virtual PBoolean GetFrameData(
      BYTE * buffer,                 ///< Buffer to receive frame
      PINDEX * bytesReturned = NULL  ///< Optional bytes returned.
    ) = 0;

    /**Grab a frame. Do not delay according to the current frame rate parameter.
      */
    virtual PBoolean GetFrameDataNoDelay(
      BYTE * buffer,                 ///< Buffer to receive frame
      PINDEX * bytesReturned,       ///< Optional bytes returned.
      unsigned int & flags           ///< optional flags returned
    );
    virtual PBoolean GetFrameDataNoDelay(
      BYTE * buffer,                 ///< Buffer to receive frame
      PINDEX * bytesReturned = NULL  ///< Optional bytes returned.
    ) = 0;

    /**Try all known video formats & see which ones are accepted by the video driver
     */
    virtual PBoolean TestAllFormats() = 0;
};


////////////////////////////////////////////////////////
//
// declare macros and structures needed for video input plugins
//

template <class className> class PVideoInputPluginServiceDescriptor : public PDevicePluginServiceDescriptor
{
  public:
    virtual PObject *    CreateInstance(int /*userData*/) const { return new className; }
    virtual PStringArray GetDeviceNames(int /*userData*/) const { return className::GetInputDeviceNames(); }
    virtual bool         GetDeviceCapabilities(const PString & deviceName, void * caps) const
      { return className::GetDeviceCapabilities(deviceName, (PVideoInputDevice::Capabilities *)caps); }
};

#define PCREATE_VIDINPUT_PLUGIN(name) \
  static PVideoInputPluginServiceDescriptor<PVideoInputDevice_##name> PVideoInputDevice_##name##_descriptor; \
  PCREATE_PLUGIN(name, PVideoInputDevice, &PVideoInputDevice_##name##_descriptor)

//PPLUGIN_STATIC_LOAD(FakeVideo, PVideoInputDevice);

#ifdef P_APPSHARE
  PPLUGIN_STATIC_LOAD(Application, PVideoInputDevice);
#endif

#if P_FFVDEV
  PPLUGIN_STATIC_LOAD(FFMPEG, PVideoInputDevice);
#endif

#if P_VIDFILE
  PPLUGIN_STATIC_LOAD(YUVFile, PVideoInputDevice);
#endif

#ifdef P_DIRECTSHOW
  PPLUGIN_STATIC_LOAD(DirectShow2010, PVideoInputDevice);
  PPLUGIN_STATIC_LOAD(DirectShowEx, PVideoInputDevice);
#endif

#ifdef P_DIRECTSHOW2
  PPLUGIN_STATIC_LOAD(DirectShow2, PVideoInputDevice);
#endif

////////////////////////////////////////////////////////
//
// declare macros and structures needed for video output plugins
//

template <class className> class PVideoOutputPluginServiceDescriptor : public PDevicePluginServiceDescriptor
{
  public:
    virtual PObject *    CreateInstance(int /*userData*/) const { return new className; }
    virtual PStringArray GetDeviceNames(int /*userData*/) const { return className::GetOutputDeviceNames(); }
};

#define PCREATE_VIDOUTPUT_PLUGIN(name) \
  static PVideoOutputPluginServiceDescriptor<PVideoOutputDevice_##name> PVideoOutputDevice_##name##_descriptor; \
  PCREATE_PLUGIN(name, PVideoOutputDevice, &PVideoOutputDevice_##name##_descriptor)

#if _WIN32
  PPLUGIN_STATIC_LOAD(Window, PVideoOutputDevice);
#endif

#if P_SDL
  PPLUGIN_STATIC_LOAD(SDL, PVideoOutputDevice);
#endif


////////////////////////////////////////////////////////
//
// declare classes needed for access to simple video font
//

class PVideoFont : public PObject
{
  PCLASSINFO(PVideoFont, PObject);
  public:
    enum {
      MAX_L_HEIGHT = 11
    };
    struct LetterData {
      char ascii;
      const char *line[MAX_L_HEIGHT];
    };

    static const LetterData * GetLetterData(char ascii);
};

#endif // P_VIDEO

#endif   // PTLIB_PVIDEOIO_H

// End Of File ///////////////////////////////////////////////////////////////
