/*
 * videoio.cxx
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

#ifdef __GNUC__
#pragma implementation "videoio.h"
#endif 

#include <ptlib.h>

#if P_VIDEO

#include <ptlib/pluginmgr.h>
#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>


namespace PWLib {
  PFactory<PDevicePluginAdapterBase>::Worker< PDevicePluginAdapter<PVideoInputDevice> > vidinChannelFactoryAdapter("PVideoInputDevice", PTrue);
  PFactory<PDevicePluginAdapterBase>::Worker< PDevicePluginAdapter<PVideoOutputDevice> > vidoutChannelFactoryAdapter("PVideoOutputDevice", PTrue);
};

template <> PVideoInputDevice * PDevicePluginFactory<PVideoInputDevice>::Worker::Create(const PString & type) const
{
  return PVideoInputDevice::CreateDevice(type);
}

template <> PVideoOutputDevice * PDevicePluginFactory<PVideoOutputDevice>::Worker::Create(const PString & type) const
{
  return PVideoOutputDevice::CreateDevice(type);
}

///////////////////////////////////////////////////////////////////////////////

#if PTRACING
ostream & operator<<(ostream & strm, PVideoDevice::VideoFormat fmt)
{
  static const char * const VideoFormatNames[PVideoDevice::NumVideoFormats] = {
    "PAL",
    "NTSC",
    "SECAM",
    "Auto"
  };

  if (fmt < PVideoDevice::NumVideoFormats && VideoFormatNames[fmt] != NULL)
    strm << VideoFormatNames[fmt];
  else
    strm << "VideoFormat<" << (unsigned)fmt << '>';

  return strm;
}
#endif


//Colour format bit per pixel table.
// These are in rough order of colour gamut size and "popularity"
static struct {
  const char * colourFormat;
  unsigned     bitsPerPixel;
} colourFormatBPPTab[] = {
  { "MJPEG", 24 },//chen
  { "YUV420P", 12 },
  { "I420",    12 },
  { "IYUV",    12 },
  { "YUV420",  12 },
  { "RGB24",   24 },
  { "BGR24",   24 },
  { "RGB32",   32 },
  { "BGR32",   32 },
  { "YUY2",    16 },
  { "YUV422",  16 },
  { "YUV422P", 16 },
  { "YUV411",  12 },
  { "YUV411P", 12 },
  { "RGB565",  16 },
  { "RGB555",  16 },
  { "YUV410",  10 },
  { "YUV410P", 10 },
  { "Grey",     8 },
  { "GreyF",    8 },
  { "UYVY422", 16 },
  { "UYV444",  24 },
  { "SBGGR8",   8 }
};


template <class VideoDevice>
static VideoDevice * CreateDeviceWithDefaults(PString & adjustedDeviceName,
                                              const PString & driverName,
                                              PPluginManager * pluginMgr, int userData=0)
{
  if (adjustedDeviceName == "*")
    adjustedDeviceName.MakeEmpty();

  PString adjustedDriverName = driverName;
  if (adjustedDriverName == "*")
    adjustedDriverName.MakeEmpty();

  if (adjustedDeviceName.IsEmpty()) {
    if (adjustedDriverName.IsEmpty()) {
      PStringArray drivers = VideoDevice::GetDriverNames(pluginMgr);
      if (drivers.IsEmpty())
        return NULL;

      // Give precedence to drivers like camera grabbers, Window
      static const char * prioritisedDrivers[] = {
        "Window", "SDL", "DirectShow", "VideoForWindows", "V4L", "V4L2", "1394DC", "1394AVC", "BSDCAPTURE", "FakeVideo", "NULLOutput"
      };
      for (PINDEX i = 0; i < PARRAYSIZE(prioritisedDrivers); i++) {
        PINDEX driverIndex = drivers.GetValuesIndex(PString(prioritisedDrivers[i]));
        if (driverIndex != P_MAX_INDEX) {
          PStringArray devices = VideoDevice::GetDriversDeviceNames(drivers[driverIndex]);
          if (!devices.IsEmpty()) {
            adjustedDeviceName = devices[0];
            adjustedDriverName = drivers[driverIndex];
            break;
          }
        }
      }

      if (adjustedDriverName.IsEmpty())
        adjustedDriverName = drivers[0];
    }

    if (adjustedDeviceName.IsEmpty()) {
      PStringArray devices = VideoDevice::GetDriversDeviceNames(adjustedDriverName);
      if (devices.IsEmpty())
        return NULL;

      adjustedDeviceName = devices[0];
    }
  }

  return VideoDevice::CreateDeviceByName(adjustedDeviceName, adjustedDriverName, pluginMgr,  userData);
}


///////////////////////////////////////////////////////////////////////////////
// PVideoDevice

PVideoFrameInfo::PVideoFrameInfo()
  : frameWidth(CIFWidth)
  , frameHeight(CIFHeight)
  , frameRate(25)
  , colourFormat("YUV420P")
  , resizeMode(eScale)
{
}


PVideoFrameInfo::PVideoFrameInfo(unsigned        width,
                                 unsigned        height,
                                 const PString & format,
                                 unsigned        rate,
                                 ResizeMode      resize)
  : frameWidth(width)
  , frameHeight(height)
  , frameRate(rate)
  , colourFormat(format)
  , resizeMode(resize)
{
}


PBoolean PVideoFrameInfo::SetFrameSize(unsigned width, unsigned height)
{
  if (width < 8 || height < 8)
    return PFalse;
  frameWidth = width;
  frameHeight = height;
  return PTrue;
}


PBoolean PVideoFrameInfo::GetFrameSize(unsigned & width, unsigned & height) const
{
  width = frameWidth;
  height = frameHeight;
  return PTrue;
}


unsigned PVideoFrameInfo::GetFrameWidth() const
{
  unsigned w,h;
  GetFrameSize(w, h);
  return w;
}


unsigned PVideoFrameInfo::GetFrameHeight() const
{
  unsigned w,h;
  GetFrameSize(w, h);
  return h;
}


PBoolean PVideoFrameInfo::SetFrameRate(unsigned rate)
{
  if (rate < 1 || rate > 999)
    return PFalse;

  frameRate = rate;
  return PTrue;
}


unsigned PVideoFrameInfo::GetFrameRate() const
{
  return frameRate;
}


PBoolean PVideoFrameInfo::SetColourFormat(const PString & colourFmt)
{
  if (!colourFmt) {
    colourFormat = colourFmt.ToUpper();
    return PTrue;
  }

  for (PINDEX i = 0; i < PARRAYSIZE(colourFormatBPPTab); i++) {
    if (SetColourFormat(colourFormatBPPTab[i].colourFormat))
      return PTrue;
  }

  return PFalse;
}


const PString & PVideoFrameInfo::GetColourFormat() const
{
  return colourFormat;
}


PINDEX PVideoFrameInfo::CalculateFrameBytes(unsigned width, unsigned height,
                                              const PString & colourFormat)
{
  for (PINDEX i = 0; i < PARRAYSIZE(colourFormatBPPTab); i++) {
    if (colourFormat *= colourFormatBPPTab[i].colourFormat)
      return width * height * colourFormatBPPTab[i].bitsPerPixel/8;
  }
  return 0;
}
 

bool PVideoFrameInfo::Parse(const PString & str)
{
  PString newFormat = colourFormat;
  PINDEX formatOffset = str.Find(':');
  if (formatOffset == 0)
    return false;

  if (formatOffset == P_MAX_INDEX)
    formatOffset = 0;
  else
    newFormat = str.Left(formatOffset++);


  ResizeMode newMode = resizeMode;
  PINDEX resizeOffset = str.Find('/', formatOffset);
  if (resizeOffset != P_MAX_INDEX) {
    static struct {
      const char * name;
      ResizeMode   mode;
    } const ResizeNames[] = {
      { "scale",   eScale },
      { "resize",  eScale },
      { "centre",  eCropCentre },
      { "center",  eCropCentre },
      { "crop",    eCropTopLeft },
      { "topleft", eCropTopLeft }
    };

    PCaselessString crop = str.Mid(resizeOffset+1);
    PINDEX resizeIndex = 0;
    while (crop != ResizeNames[resizeIndex].name) {
      if (++resizeIndex >= PARRAYSIZE(ResizeNames))
        return false;
    }
    newMode = ResizeNames[resizeIndex].mode;
  }


  int newRate = frameRate;
  PINDEX rateOffset = str.Find('@', formatOffset);
  if (rateOffset == P_MAX_INDEX)
    rateOffset = resizeOffset;
  else {
    newRate = str.Mid(rateOffset+1).AsInteger();
    if (newRate < 1 || newRate > 100)
      return false;
  }

  if (!ParseSize(str(formatOffset, rateOffset-1), frameWidth, frameHeight))
    return false;

  colourFormat = newFormat;
  frameRate = newRate;
  resizeMode = newMode;
  return true;
}


static struct {
  const char * name;
  unsigned width;
  unsigned height;
} const SizeTable[] = {
    { "CIF",    PVideoDevice::CIFWidth,   PVideoDevice::CIFHeight   },
    { "QCIF",   PVideoDevice::QCIFWidth,  PVideoDevice::QCIFHeight  },
    { "SQCIF",  PVideoDevice::SQCIFWidth, PVideoDevice::SQCIFHeight },
    { "CIF4",   PVideoDevice::CIF4Width,  PVideoDevice::CIF4Height  },
    { "4CIF",   PVideoDevice::CIF4Width,  PVideoDevice::CIF4Height  },
    { "CIF16",  PVideoDevice::CIF16Width, PVideoDevice::CIF16Height },
    { "16CIF",  PVideoDevice::CIF16Width, PVideoDevice::CIF16Height },

    { "192x144",192,                      144                       },
    { "480x360",480,                      360                       },
    { "CCIR601",720,                      486                       },
    { "NTSC",   720,                      480                       },
    { "PAL",    768,                      576                       },
    { "HDTVP",  1280,                     720                       },
    { "HD720",  1280,                     720                       },
    { "HDTVI",  1920,                     1080                      },
    { "HD1080", 1920,                     1080                      },

    { "CGA",    320,                      240                       },
    { "VGA",    640,                      480                       },
    { "WVGA",   854,                      480                       },
    { "SVGA",   800,                      600                       },
    { "XGA",    1024,                     768                       },
    { "SXGA",   1280,                     1024                      },
    { "WSXGA",  1440,                     900                       },
    { "SXGA+",  1400,                     1050                      },
    { "WSXGA+", 1680,                     1050                      },
    { "UXGA",   1600,                     1200                      },
    { "WUXGA",  1920,                     1200                      },
    { "QXGA",   2048,                     1536                      },
    { "WQXGA",  2560,                     1600                      },
    { }
};

bool PVideoFrameInfo::ParseSize(const PString & str, unsigned & width, unsigned & height)
{
  for (int i = 0; SizeTable[i].name != NULL; i++) {
    if (str *= SizeTable[i].name) {
      width = SizeTable[i].width;
      height = SizeTable[i].height;
      return true;
    }
  }

  return sscanf(str, "%ux%u", &width, &height) == 2 && width > 0 && height > 0;
}


PString PVideoFrameInfo::AsString(unsigned width, unsigned height)
{
  for (int i = 0; SizeTable[i].name != NULL; i++) {
    if (SizeTable[i].width == width && SizeTable[i].height == height)
      return SizeTable[i].name;
  }

  return psprintf("%ux%u", width, height);
}


///////////////////////////////////////////////////////////////////////////////
// PVideoDevice

PVideoDevice::PVideoDevice()
{
  lastError = 0;

  videoFormat = Auto;
  channelNumber = -1;  // -1 will find the first working channel number.
  nativeVerticalFlip = PFalse;

  converter = NULL;
}

PVideoDevice::~PVideoDevice()
{
  if (converter)
    delete converter;
}


PVideoDevice::OpenArgs::OpenArgs()
  : pluginMgr(NULL),
    deviceName("#1"),
    videoFormat(Auto),
    channelNumber(0),
    colourFormat("YUV420P"),
    convertFormat(PTrue),
    rate(0),
    width(CIFWidth),
    height(CIFHeight),
    convertSize(PTrue),
    resizeMode(eScale),
    flip(PFalse),
    brightness(-1),
    whiteness(-1),
    contrast(-1),
    colour(-1),
    hue(-1)
{
}


PBoolean PVideoDevice::OpenFull(const OpenArgs & args, PBoolean startImmediate)
{
  if (args.deviceName[0] == '#') {
    PStringArray devices = GetDeviceNames();
    PINDEX id = args.deviceName.Mid(1).AsUnsigned();
    if (id == 0 || id > devices.GetSize())
      return PFalse;

    if (!Open(devices[id-1], PFalse))
      return PFalse;
  }
  else {
    if (!Open(args.deviceName, PFalse))
      return PFalse;
  }

  if (!SetVideoFormat(args.videoFormat))
    return PFalse;

  if (!SetChannel(args.channelNumber))
    return PFalse;

  if (args.convertFormat) {
    if (!SetColourFormatConverter(args.colourFormat))
      return PFalse;
  }
  else {
    if (!SetColourFormat(args.colourFormat))
      return PFalse;
  }

  if (args.rate > 0) {
    if (!SetFrameRate(args.rate))
      return PFalse;
  }

  if (args.convertSize) {
    if (!SetFrameSizeConverter(args.width, args.height, args.resizeMode))
      return PFalse;
  }
  else {
    if (!SetFrameSize(args.width, args.height))
      return PFalse;
  }

  if (!SetVFlipState(args.flip))
    return PFalse;

  if (args.brightness >= 0) {
    if (!SetBrightness(args.brightness))
      return PFalse;
  }

  if (args.whiteness >= 0) {
    if (!SetWhiteness(args.whiteness))
      return PFalse;
  }

  if (args.contrast >= 0) {
    if (!SetContrast(args.contrast))
      return PFalse;
  }

  if (args.colour >= 0) {
    if (!SetColour(args.colour))
      return PFalse;
  }

  if (args.hue >= 0) {
    if (!SetColour(args.hue))
      return PFalse;
  }

  if (startImmediate)
    return Start();

  return PTrue;
}


PBoolean PVideoDevice::Close()
{
  return PTrue;  
}


PBoolean PVideoDevice::Start()
{
  return PTrue;
}


PBoolean PVideoDevice::Stop()
{
  return PTrue;
}


PBoolean PVideoDevice::SetVideoFormat(VideoFormat videoFmt)
{
  videoFormat = videoFmt;
  return PTrue;
}


PVideoDevice::VideoFormat PVideoDevice::GetVideoFormat() const
{
  return videoFormat;
}


int PVideoDevice::GetNumChannels()
{
  return 1;
}


PBoolean PVideoDevice::SetChannel(int channelNum)
{
  if (channelNum < 0) { // Seek out the first available channel
    for (int c = 0; c < GetNumChannels(); c++) {
      if (SetChannel(c))
        return PTrue;
    }
    return PFalse;
  }

  if (channelNum >= GetNumChannels()) {
    PTRACE(2, "PVidDev\tSetChannel number (" << channelNum << ") too large.");
    return PFalse;
  }

  channelNumber = channelNum;
  return PTrue;
}


int PVideoDevice::GetChannel() const
{
  return channelNumber;
}


PBoolean PVideoDevice::SetColourFormatConverter(const PString & newColourFmt)
{
  PVideoFrameInfo src = *this;
  PVideoFrameInfo dst = *this;

  PString colourFmt = newColourFmt; // make copy, just in case newColourFmt is reference to member colourFormat

  if (converter != NULL) {
    if (CanCaptureVideo()) {
      if (converter->GetDstColourFormat() == colourFmt)
        return PTrue;
    }
    else {
      if (converter->GetSrcColourFormat() == colourFmt)
        return PTrue;
    }
    converter->GetSrcFrameInfo(src);
    converter->GetDstFrameInfo(dst);
    delete converter;
    converter = NULL;
  }
  
  if (!preferredColourFormat.IsEmpty()) {
    PTRACE(4,"PVidDev\tSetColourFormatConverter, want " << colourFmt << " trying " << preferredColourFormat);
    if (SetColourFormat(preferredColourFormat)) {
      if (CanCaptureVideo()) {
        PTRACE(4,"PVidDev\tSetColourFormatConverter set camera to native "<< preferredColourFormat);
        if (preferredColourFormat != colourFmt)
          src.SetColourFormat(preferredColourFormat);
      }
      else {
        PTRACE(4,"PVidDev\tSetColourFormatConverter set renderer to "<< preferredColourFormat);
        if (preferredColourFormat != colourFmt)
          dst.SetColourFormat(preferredColourFormat);
      }

      if (nativeVerticalFlip || src.GetColourFormat() != dst.GetColourFormat()) {
        converter = PColourConverter::Create(src, dst);
        if (converter != NULL) {
          converter->SetVFlipState(nativeVerticalFlip);
          return PTrue;
        }
      }
    }
  }
  
  if (SetColourFormat(colourFmt)) {
    if (nativeVerticalFlip) {
      src.SetColourFormat(colourFmt);
      dst.SetColourFormat(colourFmt);
      converter = PColourConverter::Create(src, dst);
      if (PAssertNULL(converter) == NULL)
        return PFalse;
      converter->SetVFlipState(nativeVerticalFlip);
    }

    PTRACE(3, "PVidDev\tSetColourFormatConverter success for native " << colourFmt);    
    return PTrue;
  }

  // check to see if we have detected this device before, and try the same combination worked last time
  if (CanCaptureVideo()) {
    PString previousColourFormat       = "BGR24";

    PTRACE(4,"PVidDev\tSetColourFormatConverter trying, want " << colourFmt << " trying " << previousColourFormat);
    if (SetColourFormat(previousColourFormat)) {
      src.SetColourFormat(previousColourFormat);
      dst.SetColourFormat(colourFmt);
      converter = PColourConverter::Create(src, dst);
      if (converter != NULL) {
        PTRACE(3, "PVidDev\tSetColourFormatConverter succeeded for " << colourFmt << " and device using " << previousColourFormat);
        converter->SetVFlipState(nativeVerticalFlip);
        return PTrue;
      }
    }
    PTRACE(4,"PVidDev\tSetColourFormatConverter " << previousColourFormat << " did not work");
  }
  
  /************************
    Eventually, need something more sophisticated than this, but for the
    moment pick the known colour formats that the device is very likely to
    support and then look for a conversion routine from that to the
    destination format.

    What we really want is some sort of better heuristic that looks at
    computational requirements of each converter and picks a pair of formats
    that the hardware supports and uses the least CPU.
  */

  PINDEX knownFormatIdx = 0;
  while (knownFormatIdx < PARRAYSIZE(colourFormatBPPTab)) {
    PString formatToTry = colourFormatBPPTab[knownFormatIdx].colourFormat;
    PTRACE(4,"PVidDev\tSetColourFormatConverter, want " << colourFmt << " trying " << formatToTry);
    if (SetColourFormat(formatToTry)) {
      if (CanCaptureVideo()) {
        PTRACE(4,"PVidDev\tSetColourFormatConverter set camera to "<< formatToTry);
        src.SetColourFormat(formatToTry);
        dst.SetColourFormat(colourFmt);
      }
      else {
        PTRACE(4,"PVidDev\tSetColourFormatConverter set renderer to "<< formatToTry);
        dst.SetColourFormat(formatToTry);
        src.SetColourFormat(colourFmt);
      }
      converter = PColourConverter::Create(src, dst);
      if (converter != NULL) {
        // set converter properties that depend on this color format
        PTRACE(3, "PVidDev\tSetColourFormatConverter succeeded for " << colourFmt << " and device using " << formatToTry);
        converter->SetVFlipState(nativeVerticalFlip);
        return PTrue;
      } 
    } 
    knownFormatIdx++;
  }

  PTRACE(2, "PVidDev\tSetColourFormatConverter  FAILED for " << colourFmt);
  return PFalse;
}


PBoolean PVideoDevice::GetVFlipState()
{
  if (converter != NULL)
    return converter->GetVFlipState() ^ nativeVerticalFlip;

  return nativeVerticalFlip;
}


PBoolean PVideoDevice::SetVFlipState(PBoolean newVFlip)
{
  if (newVFlip && converter == NULL) {
    converter = PColourConverter::Create(*this, *this);
    if (PAssertNULL(converter) == NULL)
      return PFalse;
  }

  if (converter != NULL)
    converter->SetVFlipState(newVFlip ^ nativeVerticalFlip);

  return PTrue;
}


PBoolean PVideoDevice::GetFrameSizeLimits(unsigned & minWidth,
                                      unsigned & minHeight,
                                      unsigned & maxWidth,
                                      unsigned & maxHeight) 
{
  minWidth = minHeight = 1;
  maxWidth = maxHeight = UINT_MAX;
  return PFalse;
}


static struct {
    unsigned asked_width, asked_height, device_width, device_height;
} framesizeTab[] = {    
    { 704, 576,    640, 480 },
    { 640, 480,    704, 576 },
    { 640, 480,    352, 288 },

    { 352, 288,    704, 576 },
    { 352, 288,    640, 480 },
    { 352, 288,    352, 240 },
    { 352, 288,    320, 240 },
    { 352, 288,    176, 144 },
    { 352, 288,   1024, 576 }, /* High resolution need to be set at the end */
    { 352, 288,   1280, 960 },

    { 352, 240,    352, 288 },
    { 352, 240,    320, 240 },
    { 352, 240,    640, 480 },

    { 320, 240,    352, 288 },
    { 320, 240,    352, 240 },
    { 320, 240,    640, 480 },

    { 176, 144,    352, 288 },
    { 176, 144,    352, 240 },
    { 176, 144,    320, 240 },
    { 176, 144,    176, 120 },
    { 176, 144,    160, 120 },
    { 176, 144,    640, 480 },
    { 176, 144,   1024, 576 },
    { 176, 144,   1280, 960 }, /* High resolution need to be set at the end */

    { 176, 120,    352, 288 },
    { 176, 120,    352, 240 },
    { 176, 120,    320, 240 },
    { 176, 120,    176, 144 },
    { 176, 120,    160, 120 },
    { 176, 120,    640, 480 },

    { 160, 120,    352, 288 },
    { 160, 120,    352, 240 },
    { 160, 120,    320, 240 },
    { 160, 120,    176, 144 },
    { 160, 120,    176, 120 },
    { 160, 120,    640, 480 },
    { 192, 144,    192, 144 },
};

PBoolean PVideoDevice::SetFrameSizeConverter(unsigned width, unsigned height_old, ResizeMode resizeMode)
{
#ifdef _WIN32
  unsigned height = height_old;//(height_old==360?352:height_old);
#else
  unsigned height = (height_old==360?352:height_old);
#endif
  if (SetFrameSize(width, height)) {
    if (nativeVerticalFlip && converter == NULL) {
      converter = PColourConverter::Create(*this, *this);
      if (PAssertNULL(converter) == NULL)
        return PFalse;
    }
    if (converter != NULL) {
      converter->SetFrameSize(frameWidth, frameHeight);
      converter->SetVFlipState(nativeVerticalFlip);
    }
    return PTrue;
  }

  // Try and get the most compatible physical frame size to convert from/to
  PINDEX i;
  for (i = 0; i < PARRAYSIZE(framesizeTab); i++) {
    if (framesizeTab[i].asked_width == width && framesizeTab[i].asked_height == height &&
        SetFrameSize(framesizeTab[i].device_width, framesizeTab[i].device_height))
      break;
  }
  if (i >= PARRAYSIZE(framesizeTab)) {
    // Failed to find a resolution the device can do so far, so try
    // using the maximum width and height it claims it can do.
    unsigned minWidth, minHeight, maxWidth, maxHeight;
    if (GetFrameSizeLimits(minWidth, minHeight, maxWidth, maxHeight))
      SetFrameSize(maxWidth, maxHeight);
  }

  // Now create the converter ( if not already exist)
  if (converter == NULL) {
    PVideoFrameInfo src = *this;
    PVideoFrameInfo dst = *this;
    if (CanCaptureVideo())
      dst.SetFrameSize(width, height);
    else
      src.SetFrameSize(width, height);
    PTRACE(3,"PVidDev\tColour converter Preparing from " << width << 'x' << height );
    
    dst.SetResizeMode(resizeMode);
    converter = PColourConverter::Create(src, dst);
    if (converter == NULL) {
      PTRACE(1, "PVidDev\tSetFrameSizeConverter Colour converter creation failed");
      return PFalse;
    }
  }
  else
  {
    if (CanCaptureVideo())
      converter->SetDstFrameSize(width, height);
    else
      converter->SetSrcFrameSize(width, height);
    converter->SetResizeMode(resizeMode);
  }

  PTRACE(3,"PVidDev\tColour converter used from " << converter->GetSrcFrameWidth() << 'x' << converter->GetSrcFrameHeight() << " [" << converter->GetSrcColourFormat() << "]" << " to " << converter->GetDstFrameWidth() << 'x' << converter->GetDstFrameHeight() << " [" << converter->GetDstColourFormat() << "]");

  return PTrue;
}


PBoolean PVideoDevice::SetFrameSize(unsigned width, unsigned height)
{
#if PTRACING
  unsigned oldWidth = frameWidth;
  unsigned oldHeight = frameHeight;
#endif

  unsigned minWidth, minHeight, maxWidth, maxHeight;
  GetFrameSizeLimits(minWidth, minHeight, maxWidth, maxHeight);

  if (width < minWidth)
    frameWidth = minWidth;
  else if (width > maxWidth)
    frameWidth = maxWidth;
  else
    frameWidth = width;

  if (height < minHeight)
    frameHeight = minHeight;
  else if (height > maxHeight)
    frameHeight = maxHeight;
  else
    frameHeight = height;

  if (converter != NULL) {
    if (!converter->SetSrcFrameSize(width, height) ||
        !converter->SetDstFrameSize(width, height)) {
      PTRACE(1, "PVidDev\tSetFrameSize with converter failed with " << width << 'x' << height);
      return PFalse;
    }
  }

  PTRACE_IF(2, oldWidth != frameWidth || oldHeight != frameHeight,
            "PVidDev\tSetFrameSize to " << frameWidth << 'x' << frameHeight);
  return PTrue;
}


PBoolean PVideoDevice::GetFrameSize(unsigned & width, unsigned & height) const
{
  // Channels get very upset at this not returning the output size.
  return converter != NULL ? converter->GetDstFrameSize(width, height) : PVideoFrameInfo::GetFrameSize(width, height);
}


PINDEX PVideoDevice::GetMaxFrameBytesConverted(PINDEX rawFrameBytes) const
{
  if (converter == NULL)
    return rawFrameBytes;

  PINDEX srcFrameBytes = converter->GetMaxSrcFrameBytes();
  PINDEX dstFrameBytes = converter->GetMaxDstFrameBytes();
  PINDEX convertedFrameBytes = PMAX(srcFrameBytes, dstFrameBytes);
  return PMAX(rawFrameBytes, convertedFrameBytes);
}


int PVideoDevice::GetBrightness()
{
  return frameBrightness;
}


PBoolean PVideoDevice::SetBrightness(unsigned newBrightness)
{
  frameBrightness = newBrightness;
  return PTrue;
}


int PVideoDevice::GetWhiteness()
{
  return frameWhiteness;
}


PBoolean PVideoDevice::SetWhiteness(unsigned newWhiteness)
{
  frameWhiteness = newWhiteness;
  return PTrue;
}


int PVideoDevice::GetColour()
{
  return frameColour;
}


PBoolean PVideoDevice::SetColour(unsigned newColour)
{
  frameColour=newColour;
  return PTrue;
}


int PVideoDevice::GetContrast()
{
  return frameContrast;
}


PBoolean PVideoDevice::SetContrast(unsigned newContrast)
{
  frameContrast=newContrast;
  return PTrue;
}


int PVideoDevice::GetHue()
{
  return frameHue;
}


PBoolean PVideoDevice::SetHue(unsigned newHue)
{
  frameHue=newHue;
  return PTrue;
}

    
PBoolean PVideoDevice::GetParameters (int *whiteness,
                                  int *brightness, 
                                  int *colour,
                                  int *contrast,
                                  int *hue)
{
  if (!IsOpen())
    return PFalse;

  *brightness = frameBrightness;
  *colour     = frameColour;
  *contrast   = frameContrast;
  *hue        = frameHue;
  *whiteness  = frameWhiteness;

  return PTrue;
}

PBoolean PVideoDevice::SetVideoChannelFormat (int newNumber, VideoFormat newFormat) 
{
  PBoolean err1, err2;

  err1 = SetChannel (newNumber);
  err2 = SetVideoFormat (newFormat);
  
  return (err1 && err2);
}

PStringArray PVideoDevice::GetDeviceNames() const
{
  return PStringArray();
}

////////////////////////////////////////////////////////////////////////////////////////////

PString PVideoControlInfo::AsString(const InputControlType & ctype)
{
	switch (ctype) {
		case ControlPan:
			return "Pan";
		case ControlTilt:
			return "Tilt";
		case ControlZoom:
			return "Zoom";
	}
	return PString();
}

////////////////////////////////////////////////////////////////////////////////////////////

PString PVideoInteractionInfo::AsString(const InputInteractType & ctype)
{
	switch (ctype) {
		case InteractKey:
			return "Remote Key Press";
		case InteractMouse:
			return "Remote Mouse Move/Click";
		case InteractNavigate:
			return "Remote Navigation";
		case InteractRTSP:
			return "Remote RTSP Commands";
		case InteractOther:
			return "Custom/Other";
	}
	return PString();
}

/////////////////////////////////////////////////////////////////////////////////////////////

PVideoInputControl::~PVideoInputControl()  
{ 
	Reset(); 
}

PBoolean PVideoInputControl::Pan(long /*value*/, bool /*absolute*/)  
{
	return false; 
}

PBoolean PVideoInputControl::Tilt(long /*value*/, bool /*absolute*/)  
{ 
	return false; 
}

PBoolean PVideoInputControl::Zoom(long /*value*/, bool /*absolute*/)  
{ 
	return false; 
}

long PVideoInputControl::GetPan()
{
	long position;
	if (GetCurrentPosition(ControlPan, position))
		  return position;

	return 0;
}
	
long PVideoInputControl::GetTilt()
{
	long position;
	if (GetCurrentPosition(ControlTilt, position))
		   return position;

    return 0;
}
	
long PVideoInputControl::GetZoom()
{
	long position;
	if (GetCurrentPosition(ControlZoom, position))
		   return position;

    return 0;
}

void PVideoInputControl::Reset()
{
	PTRACE(4,"CC\tResetting camera to default position.");

	long position;

	if (GetDefaultPosition(ControlPan, position))
		   Pan(position,true);

	if (GetDefaultPosition(ControlTilt, position))
		   Tilt(position,true);

	if (GetDefaultPosition(ControlZoom, position))
		   Zoom(position,true);

}

PBoolean PVideoInputControl::GetVideoControlInfo(const InputControlType ctype, PVideoControlInfo & control)
{
	 for (std::list<PVideoControlInfo>::iterator r = m_info.begin(); r != m_info.end(); ++r) {
		 if (r->type == ctype) {
			 control = *r;
			 return true;
		 }
	 }

	 return false;
}

PBoolean PVideoInputControl::GetDefaultPosition(const InputControlType ctype, long & def)
{
	 for (std::list<PVideoControlInfo>::const_iterator r = m_info.begin(); r != m_info.end(); ++r) {
		 if (r->type == ctype) {
			 def = r->def;
			 return true;
		 }
	 }
	 return false;
}

PBoolean PVideoInputControl::GetCurrentPosition(const InputControlType ctype, long & current)
{
	 for (std::list<PVideoControlInfo>::const_iterator r = m_info.begin(); r != m_info.end(); ++r) {
		 if (r->type == ctype) {
			 current = r->current;
			 return true;
		 }
	 }
	 return false;
}


void PVideoInputControl::SetCurrentPosition(const InputControlType ctype, long current)
{
	 for (std::list<PVideoControlInfo>::iterator r = m_info.begin(); r != m_info.end(); ++r) {
		 if (r->type == ctype) {
			 r->current = current;
			 break;
		 }
	 }
}




///////////////////////////////////////////////////////////////////////////////
// PVideoOutputDevice

PVideoOutputDevice::PVideoOutputDevice()
{
}


PBoolean PVideoOutputDevice::CanCaptureVideo() const
{
  return PFalse;
}


PBoolean PVideoOutputDevice::GetPosition(int &, int &) const
{
  return PFalse;
}


bool PVideoOutputDevice::SetPosition(int, int)
{
  return false;
}


///////////////////////////////////////////////////////////////////////////////
// PVideoOutputDeviceRGB

PVideoOutputDeviceRGB::PVideoOutputDeviceRGB()
{
  PTRACE(6, "RGB\t Constructor of PVideoOutputDeviceRGB");
#if 0
  colourFormat = "RGB24";
  bytesPerPixel = 3;
#else
  colourFormat = "RGB32";
  bytesPerPixel = 4;
#endif
  swappedRedAndBlue = false;
//  SetFrameSize(frameWidth, frameHeight);
}


PBoolean PVideoOutputDeviceRGB::SetColourFormat(const PString & colourFormat)
{
  PWaitAndSignal m(mutex);

  PINDEX newBytesPerPixel;

  if (colourFormat *= "RGB32") {
    newBytesPerPixel = 4;
    swappedRedAndBlue = false;
  }
  else if (colourFormat *= "BGR32") {
	  newBytesPerPixel = 4;
	  swappedRedAndBlue = true;
  }
#ifdef _WIN32
  else if (colourFormat *= "RGB24") {
    newBytesPerPixel = 3;
    swappedRedAndBlue = false;
  }
  else if (colourFormat *= "BGR24") {
    newBytesPerPixel = 3;
    swappedRedAndBlue = true;
  }
#endif
  else
    return PFalse;

  if (!PVideoOutputDevice::SetColourFormat(colourFormat))
    return PFalse;

  bytesPerPixel = newBytesPerPixel;
  scanLineWidth = ((frameWidth*bytesPerPixel+3)/4)*4;
  return frameStore.SetSize(frameHeight*scanLineWidth);
}


PBoolean PVideoOutputDeviceRGB::SetFrameSize(unsigned width, unsigned height)
{
  PWaitAndSignal m(mutex);

  if (frameWidth == width && frameHeight == height)
    return true;

  if (!PVideoOutputDevice::SetFrameSize(width, height))
    return PFalse;

  scanLineWidth = ((frameWidth*bytesPerPixel+3)/4)*4;
  return frameStore.SetSize(frameHeight*scanLineWidth);
}


PINDEX PVideoOutputDeviceRGB::GetMaxFrameBytes()
{
  PWaitAndSignal m(mutex);
  return GetMaxFrameBytesConverted(frameStore.GetSize());
}


PBoolean PVideoOutputDeviceRGB::SetFrameData(unsigned x, unsigned y,
                                         unsigned width, unsigned height,
                                         const BYTE * data,
                                         PBoolean endFrame)
{
  PWaitAndSignal m(mutex);

  if (x+width > frameWidth || y+height > frameHeight)
    return PFalse;

  if (x == 0 && width == frameWidth && y == 0 && height == frameHeight) {
    if (converter != NULL)
      converter->Convert(data, frameStore.GetPointer());
    else
      memcpy(frameStore.GetPointer(), data, height*scanLineWidth);
  }
  else {
    if (converter != NULL) {
      PAssertAlways("Converted output of partial RGB frame not supported");
      return PFalse;
    }

    if (x == 0 && width == frameWidth)
      memcpy(frameStore.GetPointer() + y*scanLineWidth, data, height*scanLineWidth);
    else {
      for (unsigned dy = 0; dy < height; dy++)
        memcpy(frameStore.GetPointer() + (y+dy)*scanLineWidth + x*bytesPerPixel,
               data + dy*width*bytesPerPixel, width*bytesPerPixel);
    }
  }

  if (endFrame)
    return FrameComplete();

  return PTrue;
}


///////////////////////////////////////////////////////////////////////////////
// PVideoOutputDevicePPM

#ifdef SHOULD_BE_MOVED_TO_PLUGIN

PVideoOutputDevicePPM::PVideoOutputDevicePPM()
{
  PTRACE(6, "PPM\t Constructor of PVideoOutputDevicePPM");
  frameNumber = 0;
}


PBoolean PVideoOutputDevicePPM::Open(const PString & name,
                                 PBoolean /*startImmediate*/)
{
  Close();

  PFilePath path = name;
  if (!PDirectory::Exists(path.GetDirectory()))
    return PFalse;

  if (path != psprintf(path, 12345))
    deviceName = path;
  else
    deviceName = path.GetDirectory() + path.GetTitle() + "%u" + path.GetType();

  return PTrue;
}


PBoolean PVideoOutputDevicePPM::IsOpen()
{
  return !deviceName;
}


PBoolean PVideoOutputDevicePPM::Close()
{
  deviceName.MakeEmpty();
  return PTrue;
}


PStringArray PVideoOutputDevicePPM::GetDeviceNames() const
{
  return PDirectory();
}


PBoolean PVideoOutputDevicePPM::EndFrame()
{
  PFile file;
  if (!file.Open(psprintf(deviceName, frameNumber++), PFile::WriteOnly)) {
    PTRACE(1, "PPMVid\tFailed to open PPM output file \""
           << file.GetName() << "\": " << file.GetErrorText());
    return PFalse;
  }

  file << "P6 " << frameWidth  << " " << frameHeight << " " << 255 << "\n";

  if (!file.Write(frameStore, frameStore.GetSize())) {
    PTRACE(1, "PPMVid\tFailed to write frame data to PPM output file " << file.GetName());
    return PFalse;
  }

  PTRACE(6, "PPMVid\tFinished writing PPM file " << file.GetName());
  return file.Close();
}

#endif // SHOULD_BE_MOVED_TO_PLUGIN


///////////////////////////////////////////////////////////////////////////////
// PVideoInputDevice

PBoolean PVideoInputDevice::CanCaptureVideo() const
{
  return PTrue;
}

static const char videoInputPluginBaseClass[] = "PVideoInputDevice";


PStringArray PVideoInputDevice::GetDriverNames(PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return pluginMgr->GetPluginsProviding(videoInputPluginBaseClass);
}


PStringArray PVideoInputDevice::GetDriversDeviceNames(const PString & driverName, PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return pluginMgr->GetPluginsDeviceNames(driverName, videoInputPluginBaseClass);
}


PVideoInputDevice * PVideoInputDevice::CreateDevice(const PString &driverName, PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return (PVideoInputDevice *)pluginMgr->CreatePluginsDevice(driverName, videoInputPluginBaseClass);
}


PVideoInputDevice * PVideoInputDevice::CreateDeviceByName(const PString & deviceName, const PString & driverName, PPluginManager * pluginMgr, int userData)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return (PVideoInputDevice *)pluginMgr->CreatePluginsDeviceByName(deviceName, videoInputPluginBaseClass,userData,driverName);
}


PBoolean PVideoInputDevice::GetDeviceCapabilities(const PString & deviceName, Capabilities * caps, PPluginManager * pluginMgr)
{
  return GetDeviceCapabilities(deviceName, "*", caps, pluginMgr);
}


PBoolean PVideoInputDevice::GetDeviceCapabilities(const PString & deviceName, const PString & driverName, Capabilities * caps, PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return pluginMgr->GetPluginsDeviceCapabilities(videoInputPluginBaseClass,driverName,deviceName, caps);
}



PVideoInputDevice * PVideoInputDevice::CreateOpenedDevice(const PString & driverName,
                                                          const PString & deviceName,
                                                          PBoolean startImmediate,
                                                          PPluginManager * pluginMgr )
{
  PString adjustedDeviceName = deviceName;
  PVideoInputDevice * device = CreateDeviceWithDefaults<PVideoInputDevice>(adjustedDeviceName, driverName, pluginMgr);
  if (device == NULL)
    return NULL;

  if (device->Open(adjustedDeviceName, startImmediate))
    return device;

  delete device;
  return NULL;
}


PVideoInputDevice * PVideoInputDevice::CreateOpenedDevice(const OpenArgs & args,
                                                          PBoolean startImmediate)
{
  OpenArgs adjustedArgs = args;
  PVideoInputDevice * device = CreateDeviceWithDefaults<PVideoInputDevice>(adjustedArgs.deviceName, args.driverName, NULL);
  if (device == NULL)
    return NULL;


  //PVideoDevice::SetFrameSize(args.width, args.height);//chy
  device->SetFrameSize2(args.width, args.height);
  if (device->OpenFull(adjustedArgs, startImmediate))
    return device;

  delete device;
  return NULL;
}

PBoolean PVideoInputDevice::GetFrame(PBYTEArray & frame)
{
  PINDEX returned;
  if (!GetFrameData(frame.GetPointer(GetMaxFrameBytes()), &returned))
    return PFalse;

  frame.SetSize(returned);
  return PTrue;
}

PBoolean PVideoInputDevice::GetFrameData(
  BYTE * buffer,
  PINDEX * bytesReturned,
  unsigned int & flags
)
{
  flags = 0;
  return GetFrameData(buffer, bytesReturned);
}

PBoolean PVideoInputDevice::GetFrameDataNoDelay(
  BYTE * buffer,
  PINDEX * bytesReturned,
  unsigned & flags
)
{
  flags = 0;
  return GetFrameDataNoDelay(buffer, bytesReturned);
}

PBoolean PVideoOutputDevice::SetFrameData(
      unsigned x,
      unsigned y,
      unsigned width,
      unsigned height,
      const BYTE * data,
      PBoolean endFrame,
      unsigned /*flags*/
)
{
  return SetFrameData(x, y, width, height, data, endFrame);
}





////////////////////////////////////////////////////////////////////////////////////////////

static const char videoOutputPluginBaseClass[] = "PVideoOutputDevice";


PStringArray PVideoOutputDevice::GetDriverNames(PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return pluginMgr->GetPluginsProviding(videoOutputPluginBaseClass);
}


PStringArray PVideoOutputDevice::GetDriversDeviceNames(const PString & driverName, PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return pluginMgr->GetPluginsDeviceNames(driverName, videoOutputPluginBaseClass);
}


PVideoOutputDevice * PVideoOutputDevice::CreateDevice(const PString & driverName, PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return (PVideoOutputDevice *)pluginMgr->CreatePluginsDevice(driverName, videoOutputPluginBaseClass);
}


PVideoOutputDevice * PVideoOutputDevice::CreateDeviceByName(const PString & deviceName, const PString & driverName, PPluginManager * pluginMgr, int userData)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return (PVideoOutputDevice *)pluginMgr->CreatePluginsDeviceByName(deviceName, videoOutputPluginBaseClass, userData, driverName);
}


PVideoOutputDevice * PVideoOutputDevice::CreateOpenedDevice(const PString &driverName,
                                                            const PString &deviceName,
                                                            PBoolean startImmediate,
                                                            PPluginManager * pluginMgr, int userData)
{
  PString adjustedDeviceName = deviceName;
  PVideoOutputDevice * device = CreateDeviceWithDefaults<PVideoOutputDevice>(adjustedDeviceName, driverName, pluginMgr, userData);
  if (device == NULL)
    return NULL;

  if (device->Open(adjustedDeviceName, startImmediate))
    return device;

  delete device;
  return NULL;
}


PVideoOutputDevice * PVideoOutputDevice::CreateOpenedDevice(const OpenArgs & args,
                                                            PBoolean startImmediate, int userData)
{
  OpenArgs adjustedArgs = args;
  PVideoOutputDevice * device = CreateDeviceWithDefaults<PVideoOutputDevice>(adjustedArgs.deviceName, args.driverName, NULL, userData);
  if (device == NULL)
    return NULL;

  if (device->OpenFull(adjustedArgs, startImmediate))
    return device;

  delete device;
  return NULL;
}

#endif // P_VIDEO

// End Of File ///////////////////////////////////////////////////////////////
