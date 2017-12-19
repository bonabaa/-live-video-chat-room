/*
 * vidinput_directx.cxx
 *
 * Classes to support streaming video input (grabbing) and output.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2007 Luc Saillard <luc@saillard.org>
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
 * The Initial Developer of the Original Code is Luc Saillard <luc@saillard.org>
 *
 * Contributor(s): Matthias Schneider <ma30002000@yahoo.de>
 *
 * $Revision: 22568 $
 * $Author: csoutheren $
 * $Date: 2009-05-08 04:41:22 +0000 (Fri, 08 May 2009) $
 */

#include <ptlib.h>

#if defined(P_DIRECTSHOW)

#include "ptlib/msos/ptlib/vidinput_directx.h"


#ifdef P_DIRECTSHOW_LIBRARY1
#pragma comment(lib, P_DIRECTSHOW_LIBRARY1)
#endif
#ifdef P_DIRECTSHOW_LIBRARY2
#pragma comment(lib, P_DIRECTSHOW_LIBRARY2)
#endif


PCREATE_VIDINPUT_PLUGIN(DirectShow);


#ifdef _WIN32_WCE

static const GUID MEDIASUBTYPE_IYUV = { 0x56555949, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };
#define CLSID_CaptureGraphBuilder2 CLSID_CaptureGraphBuilder

#pragma comment(lib, "strmbase.lib")
#pragma comment(lib, "mmtimer.lib")

#ifdef DEBUG
/* Only the release version is provided as a .lib file, so we need to
   make sure that the compilation does NOT have the extra fields/functions
   that are added when DEBUG version. */
#undef DEBUG
#include <streams.h>
#define DEBUG
#else
#include <streams.h>
#endif

class PSampleGrabber : public CBaseVideoRenderer
{
  public:
    PSampleGrabber(HRESULT * hr);

    virtual HRESULT CheckMediaType(const CMediaType *media);
    virtual HRESULT ShouldDrawSampleNow(IMediaSample *sample, REFERENCE_TIME *start, REFERENCE_TIME *stop);
    virtual HRESULT DoRenderSample(IMediaSample *sample);

    HRESULT GetCurrentBuffer(long *, long *);

  private:
    PMutex m_sampleMutex;
    long   m_sampleSize;
    BYTE * m_sampleData;
};

#else // _WIN32_WCE

#include <dshow.h>

#undef INTERFACE
#define INTERFACE ISampleGrabberCB
DECLARE_INTERFACE_(ISampleGrabberCB, IUnknown)
{
  STDMETHOD_(HRESULT, SampleCB)(THIS_ double, IMediaSample *) PURE;
  STDMETHOD_(HRESULT, BufferCB)(THIS_ double, BYTE *, long) PURE;
};

#undef INTERFACE
#define INTERFACE ISampleGrabber

DECLARE_INTERFACE_(ISampleGrabber,IUnknown)
{
  STDMETHOD_(HRESULT, SetOneShot)(THIS_ BOOL) PURE;
  STDMETHOD_(HRESULT, SetMediaType)(THIS_ AM_MEDIA_TYPE *) PURE;
  STDMETHOD_(HRESULT, GetConnectedMediaType)(THIS_ AM_MEDIA_TYPE *) PURE;
  STDMETHOD_(HRESULT, SetBufferSamples)(THIS_ BOOL) PURE;
  STDMETHOD_(HRESULT, GetCurrentBuffer)(THIS_ long *, long *) PURE;
  STDMETHOD_(HRESULT, GetCurrentSample)(THIS_ IMediaSample *) PURE;
  STDMETHOD_(HRESULT, SetCallback)(THIS_ ISampleGrabberCB *, long) PURE;
};

extern "C" {
  extern const CLSID CLSID_SampleGrabber;
  extern const IID IID_ISampleGrabber;
  extern const CLSID CLSID_NullRenderer;
};

#endif // _WIN32_WCE



static struct {
    const char * m_colourFormat;
    GUID         m_guid;
} const ColourFormat2GUID[] =
{
    { "Grey",    MEDIASUBTYPE_RGB8   },
    { "BGR32",   MEDIASUBTYPE_RGB32  }, /* Microsoft assumes that we are in little endian */
    { "BGR24",   MEDIASUBTYPE_RGB24  },
    { "RGB565",  MEDIASUBTYPE_RGB565 },
    { "RGB555",  MEDIASUBTYPE_RGB555 },
    { "YUV420P", MEDIASUBTYPE_IYUV   },  // aka I420
    { "YUV420P", MEDIASUBTYPE_YV12   },
    { "YUV411",  MEDIASUBTYPE_Y411   },
    { "YUV411P", MEDIASUBTYPE_Y41P   },
    { "YUV410P", MEDIASUBTYPE_YVU9   },
    { "YUY2",    MEDIASUBTYPE_YUY2   },
    { "MJPEG",   MEDIASUBTYPE_MJPG   },
    { "UYVY422", MEDIASUBTYPE_UYVY   },
};

// Some support functions/macros

#if PTRACING
static PString ErrorMessage(HRESULT hr)
{
  PVarString msg;
  DWORD dwMsgLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL,
                                 hr,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                 msg.GetPointer(1000), 999,
                                 NULL);
  if (dwMsgLen > 0)
    return msg;

#ifndef _WIN32_WCE
  dwMsgLen = AMGetErrorTextA(hr, msg.GetPointer(1000), 1000);
  if (dwMsgLen > 0)
    return msg;
#endif

#pragma warning(disable:4995)
  char hex[20];
  _snprintf(hex, sizeof(hex), "0x%08x", hr);
  return hex;
}
#endif // PTRACING


#if PTRACING
static bool CheckError(HRESULT hr, const char * fn)
{
  if (SUCCEEDED(hr))
    return false;

  PTRACE(1,"PVidDirectShow\tFunction \"" << fn << "\" failed : " << ErrorMessage(hr));
  return true;
}

#define CHECK_ERROR(fn, action) if (CheckError(fn, #fn)) action
#else
#define CHECK_ERROR(fn, action) if (FAILED(fn)) action
#endif

#define CHECK_ERROR_RETURN(fn) CHECK_ERROR(fn, return false)


class MediaTypePtr
{
    AM_MEDIA_TYPE * pointer;
  public:
    MediaTypePtr()
      : pointer(NULL)
    {
    }

    ~MediaTypePtr()
    {
      Release();
    }

    void Release()
    {
      if (pointer == NULL)
        return;

      if (pointer->cbFormat != 0) {
        CoTaskMemFree(pointer->pbFormat);
        pointer->cbFormat = 0;
        pointer->pbFormat = NULL;
      }

      if (pointer->pUnk != NULL) {
        // Uncessessary because pUnk should not be used, but safest.
        pointer->pUnk->Release();
        pointer->pUnk = NULL;
      }

      CoTaskMemFree(pointer);
    }

    operator AM_MEDIA_TYPE *()        const { return  pointer; }
    AM_MEDIA_TYPE & operator*()       const { return *pointer; }
    AM_MEDIA_TYPE** operator&()             { return &pointer; }
	AM_MEDIA_TYPE* operator->()       const { return  pointer; }

  private:
    MediaTypePtr(const MediaTypePtr &) { }
    void operator=(const MediaTypePtr &) { }
};



///////////////////////////////////////////////////////////////////////////////

#if 0

struct DirectShowInputDeviceInfo {
  Capabilities * capabilities;
  PThread * thread;
  PVideoInputDevice_DirectShow * device;
};

typedef std::map<std::string, DirectShowInputDeviceInfo> DirectShowInputDeviceInfoMap;

static DirectShowInputDeviceInfoMap directShowInputDeviceInfoMap;

static void InitialiseCache()
{
  bool PVideoInputDevice_DirectShow::EnumerateDeviceNames(PStringArray & devices)

}

#endif

///////////////////////////////////////////////////////////////////////////////

PVideoInputDevice_DirectShow::PVideoInputDevice_DirectShow()
  : m_isCapturing(false)
  , m_maxFrameBytes(0)
{
  PTRACE(4,"PVidDirectShow\tPVideoInputDevice_DirectShow: constructor");

  CoInitializeEx(NULL, COINIT_MULTITHREADED);
}


PVideoInputDevice_DirectShow::~PVideoInputDevice_DirectShow()
{
  Close();
  ::CoUninitialize();
  PTRACE(4,"PVidDirectShow\tPVideoInputDevice_DirectShow: destructor");

}


PStringArray PVideoInputDevice_DirectShow::GetInputDeviceNames()
{
  PStringArray devices;
  PVideoInputDevice_DirectShow instance;
  instance.EnumerateDeviceNames(devices);
  return devices;
}


PBoolean PVideoInputDevice_DirectShow::Open(const PString & devName, PBoolean startImmediate)
{
  PTRACE(4,"PVidDirectShow\tOpen(\"" << devName << "\", " << startImmediate << ')');

  Close();

  // Create the filter graph
  CHECK_ERROR_RETURN(CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **) &m_pGraph));

  // Create the capture graph builder
  CHECK_ERROR_RETURN(CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void **) &m_pBuilder));

  if (!CreateCaptureDevice(devName))
    return false;

  // Add Capture filter to our graph.
  CHECK_ERROR_RETURN(m_pGraph->AddFilter(m_pCapture, L"Video Capture"));

  // Attach the filter graph to the capture graph
  CHECK_ERROR_RETURN(m_pBuilder->SetFiltergraph(m_pGraph));

  // Obtain interfaces for media control and Video Window
  CHECK_ERROR_RETURN(m_pGraph->QueryInterface(IID_IMediaControl,(void **)&m_pMediaControl));

  if (!CreateGrabberHandler())
    return false;

  PTRACE(4,"PVidDirectShow\tStart of GetDeviceCapabilities for c(\"" << devName << "\"");

  Capabilities caps;
  if (GetDeviceCapabilities(&caps))
    *(PVideoFrameInfo *)this = caps.framesizes.front();

  PTRACE(4,"PVidDirectShow\tEnd of GetDeviceCapabilities for c(\"" << devName << "\"");

  if (startImmediate)
    return Start();

  deviceName = devName;

  return true;
}

PBoolean PVideoInputDevice_DirectShow::IsOpen()
{
    return m_pCapture != NULL;
}

PBoolean PVideoInputDevice_DirectShow::Close()
{
  if (!IsOpen())
    return false;

  PTRACE(4,"PVidDirectShow\tClosing \"" << deviceName << '"');

  if (m_pMediaControl)
    m_pMediaControl->StopWhenReady();

  m_pMediaControl.Release();
#ifdef _WIN32_WCE
  delete m_pSampleGrabber;
#else
  m_pSampleGrabber.Release();
#endif
  m_pCapture.Release();
  m_pBuilder.Release();
  m_pGraph.Release();

  return true;
}


PBoolean PVideoInputDevice_DirectShow::Start()
{
  if (IsCapturing())
    return true;

  PTRACE(4,"PVidDirectShow\tStart()");

  // Start previewing video data
  CHECK_ERROR_RETURN(m_pMediaControl->Run());

  /* Even after a WaitForCompletion, the webcam is sometimes not available, so wait
     until the server give us a frame before returning */
  for (unsigned retry = 0; retry < 100; ++retry) {
    long cbBuffer;
    HRESULT hr = m_pSampleGrabber->GetCurrentBuffer(&cbBuffer, NULL);
    if (SUCCEEDED(hr) && cbBuffer > 0) {
      m_isCapturing = true;
      return true;
    }

    PTRACE_IF(2, FAILED(hr) && hr != VFW_E_WRONG_STATE, "PVidDirectShow\tError waiting for camera: " << ErrorMessage(hr));

    PThread::Sleep(100); /* Not available */
  }

  PTRACE(2, "PVidDirectShow\tTime out waiting for first frame.");
  return false;
}

PBoolean PVideoInputDevice_DirectShow::Stop()
{
  if (IsCapturing())
    return false;

  PTRACE(4,"PVidDirectShow\tStop()");

  if (m_pMediaControl)
    m_pMediaControl->StopWhenReady();

  m_isCapturing = false;

  return true;
}


PBoolean PVideoInputDevice_DirectShow::IsCapturing()
{
  return m_isCapturing;
}


PBoolean PVideoInputDevice_DirectShow::GetFrameData(BYTE * buffer, PINDEX * bytesReturned)
{
  m_pacing.Delay(1000/GetFrameRate());
  return GetFrameDataNoDelay(buffer, bytesReturned);
}


PBoolean PVideoInputDevice_DirectShow::GetFrameDataNoDelay(BYTE *destFrame, PINDEX * bytesReturned)
{
  long cbBuffer = m_maxFrameBytes;
/*	if (m_pSampleGrabber==NULL) 
		return PFalse*/;
  CHECK_ERROR_RETURN(m_pSampleGrabber->GetCurrentBuffer(&cbBuffer, NULL));
  if (converter != NULL) {
		if (converter->GetSrcFrameHeight() == this->GetFrameHeight() && converter->GetSrcFrameWidth() == this->GetFrameWidth())
		{
			CHECK_ERROR_RETURN(m_pSampleGrabber->GetCurrentBuffer(&cbBuffer, (long*)m_tempFrame.GetPointer(cbBuffer)));
			if (cbBuffer == m_maxFrameBytes)
				converter->Convert(m_tempFrame, destFrame, cbBuffer, bytesReturned);
		}else
		{
			PTRACE(1, "direct\t can not convert the src to dst");
			if (cbBuffer != m_maxFrameBytes) *bytesReturned=0;
			return PTrue;
		}
  }
  else {
    PAssert(cbBuffer <= m_maxFrameBytes, PLogicError);
    CHECK_ERROR_RETURN(m_pSampleGrabber->GetCurrentBuffer(&cbBuffer, (long*)destFrame));
    if (bytesReturned != NULL)
      *bytesReturned = cbBuffer;
  }

	if (cbBuffer != m_maxFrameBytes) *bytesReturned=0;
  return true;
}


PINDEX PVideoInputDevice_DirectShow::GetMaxFrameBytes()
{
  return GetMaxFrameBytesConverted(m_maxFrameBytes);
}


/*
 * Set the FrameRate, FrameSize, ...
 */

/*
 * Change Colourspace AND FrameSize by looking if the resolution is supported by the hardware.
 *
 * For example a Logitech Pro 4000:
 *   Fmt[0] = (RGB24, 320x240, 30fps)
 *   Fmt[1] = (RGB24, 640x480, 15fps)
 *   Fmt[2] = (RGB24, 352x288, 30fps)
 *   Fmt[3] = (RGB24, 176x144, 30fps)
 *   Fmt[4] = (RGB24, 160x120, 30fps)
 *   Fmt[5] = (YUV420P, 320x240, 30fps)
 *   Fmt[6] = (YUV420P, 640x480, 15fps)
 *   Fmt[7] = (YUV420P, 352x288, 30fps)
 *   Fmt[8] = (YUV420P, 176x144, 30fps)
 *   Fmt[9] = (YUV420P, 160x120, 30fps)
 *   Fmt[10] = (IYUV, 320x240, 30fps)
 *   Fmt[11] = (IYUV, 640x480, 15fps)
 *   Fmt[12] = (IYUV, 352x288, 30fps)
 *   Fmt[13] = (IYUV, 176x144, 30fps)
 *   Fmt[14] = (IYUV, 160x120, 30fps)
 *
 * For example a Logitech Fusion that support MPJEG in hardware, doesn't return a MJPEG format :(
 *  Fmt[0] = (RGB24, 320x240, 15fps)
 *  Fmt[1] = (RGB24, 176x144, 30fps)
 *  Fmt[2] = (RGB24, 160x120, 30fps)
 *  Fmt[3] = (RGB24, 352x288, 30fps)
 *  Fmt[4] = (RGB24, 432x240, 30fps)
 *  Fmt[5] = (RGB24, 480x360, 30fps)
 *  Fmt[6] = (RGB24, 512x288, 30fps)
 *  Fmt[7] = (RGB24, 640x360, 30fps)
 *  Fmt[8] = (RGB24, 640x480, 15fps)
 *  Fmt[9] = (RGB24, 704x576, 15fps)
 *  Fmt[10] = (RGB24, 864x480, 15fps)
 *  Fmt[11] = (RGB24, 960x720, 15fps)
 *  Fmt[12] = (RGB24, 1024x576, 10fps)
 *  Fmt[13] = (RGB24, 1280x960, 7.5fps)
 *  Fmt[14] = (YUV420P, 320x240, 15fps)
 *  Fmt[15] = (YUV420P, 176x144, 30fps)
 *  Fmt[16] = (YUV420P, 160x120, 30fps)
 *  Fmt[17] = (YUV420P, 352x288, 30fps)
 *  Fmt[18] = (YUV420P, 432x240, 30fps)
 *  Fmt[19] = (YUV420P, 480x360, 30fps)
 *  Fmt[20] = (YUV420P, 512x288, 30fps)
 *  Fmt[21] = (YUV420P, 640x360, 30fps)
 *  Fmt[22] = (YUV420P, 640x480, 15fps)
 *  Fmt[23] = (YUV420P, 704x576, 15fps)
 *  Fmt[24] = (YUV420P, 864x480, 15fps)
 *  Fmt[25] = (YUV420P, 960x720, 15fps)
 *  Fmt[26] = (YUV420P, 1024x576, 10fps)
 *  Fmt[27] = (YUV420P, 1280x960, 7.5fps)
 */
PBoolean PVideoInputDevice_DirectShow::SetAllParameters(const PString & newColourFormat, int newWidth, int newHeight, int fps)
{
  PTRACE(4, "PVidDirectShow\tSetCameraFormat(\""
         << newColourFormat << "\", "
         << newWidth << 'x' << newHeight << ", "
         << fps <<"fps)");

  PComPtr<IAMStreamConfig> pStreamConfig;
  CHECK_ERROR_RETURN(m_pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE,
                                               &MEDIATYPE_Video,
                                               m_pCapture,
                                               IID_IAMStreamConfig,
                                               (void **)&pStreamConfig));

  int iCount, iSize;
  CHECK_ERROR_RETURN(pStreamConfig->GetNumberOfCapabilities(&iCount, &iSize));

  /* Sanity check: just to be sure that the Streamcaps is a VIDEOSTREAM and not AUDIOSTREAM */
  VIDEO_STREAM_CONFIG_CAPS scc;
  if (sizeof(scc) != iSize) {
    PTRACE(1, "PVidDirectShow\tBad Capapabilities (not a VIDEO_STREAM_CONFIG_CAPS)");
    return false;
  }

  for (int i = 0; i < iCount; i++) {
    MediaTypePtr pMediaFormat;
    CHECK_ERROR(pStreamConfig->GetStreamCaps(i, &pMediaFormat, (BYTE *)&scc), continue);

    if (!((pMediaFormat->formattype == FORMAT_VideoInfo) &&
          (pMediaFormat->cbFormat >= sizeof(VIDEOINFOHEADER)) &&
          (pMediaFormat->pbFormat != NULL)))
      continue;

    bool notInTable = true;
    for (int j = 0; j < sizeof(ColourFormat2GUID)/sizeof(ColourFormat2GUID[0]); j++) {
      if (pMediaFormat->subtype == ColourFormat2GUID[j].m_guid &&
           newColourFormat == ColourFormat2GUID[j].m_colourFormat) {
        notInTable = false;
        break;
      }
    }

    if (notInTable) {
      wchar_t guidName[256];
      if (StringFromGUID2(pMediaFormat->subtype, guidName, sizeof(guidName)) <= 0)
        continue; // Can't use this entry!
      if (newColourFormat != PString(guidName))
        continue;
    }

    VIDEOINFOHEADER & videoInfo = *(VIDEOINFOHEADER *)pMediaFormat->pbFormat;

    if (videoInfo.bmiHeader.biWidth != newWidth)
      continue;

    if (videoInfo.bmiHeader.biHeight != newHeight)
      continue;

    const int maxfps = (int)(10000000.0/videoInfo.AvgTimePerFrame);
    if (fps < maxfps)
      videoInfo.AvgTimePerFrame = (LONGLONG) (10000000.0 / (double)fps);

    /* We have match a goo format, Use it to change the format */
    PTRACE(4,"PVidDirectShow\tMatched setting "<< i);

    OAFilterState filterState = State_Stopped;
    HRESULT hr = m_pMediaControl->GetState(1000, &filterState);
    PTRACE_IF(1, FAILED(hr), "PVidDirectShow\tGetState failed: " << ErrorMessage(hr));
    m_pMediaControl->StopWhenReady();

    hr = pStreamConfig->SetFormat(pMediaFormat);
    if (FAILED(hr))
    {
      PTRACE(2, "PVidDirectShow\tFailed to setFormat: " << ErrorMessage(hr));
      if (hr != VFW_E_INVALIDMEDIATYPE)
        continue;

      PTRACE(3, "PVidDirectShow\tRetrying ...");
      bool was_capturing = m_isCapturing;
      Close();
      Open(deviceName, false);
      hr = pStreamConfig->SetFormat(pMediaFormat);
      if (FAILED(hr)) {
        PTRACE(1, "PVidDirectShow\tFailed to setFormat (Try #2 graph deconnected): " << ErrorMessage(hr));
        continue;
      }
      if (was_capturing)
        Start();
    }

    if (filterState==State_Running)
      m_pMediaControl->Run();
    else if (filterState==State_Paused)
      m_pMediaControl->Pause();

    nativeVerticalFlip = pMediaFormat->subtype == MEDIASUBTYPE_RGB32 ||
                         pMediaFormat->subtype == MEDIASUBTYPE_RGB24 ||
                         pMediaFormat->subtype == MEDIASUBTYPE_RGB565 ||
                         pMediaFormat->subtype == MEDIASUBTYPE_RGB555;

    PTRACE(3, "PVidDirectShow\tSet new parameters: \""
           << newColourFormat << "\", "
           << newWidth << 'x' << newHeight << ", "
           << fps <<"fps");
    return true;
  }

  PTRACE(1, "PVidDirectShow\tNo matching capability for \""
         << newColourFormat << "\", "
         << newWidth << 'x' << newHeight << ", "
         << fps <<"fps");
  return false;
}


PBoolean PVideoInputDevice_DirectShow::SetFrameSize(unsigned newWidth, unsigned newHeight)
{
  PTRACE(4,"PVidDirectShow\tSetFrameSize(" << newWidth << ", " << newHeight << ")");

  if (!SetAllParameters(colourFormat, newWidth, newHeight, frameRate))
    return false;

  PTRACE(3,"PVidDirectShow\tSetFrameSize " << newWidth << "x" << newHeight << " is suported in hardware");

  if (!PVideoDevice::SetFrameSize(newWidth, newHeight))
    return false;

  m_maxFrameBytes = CalculateFrameBytes(frameWidth, frameHeight, colourFormat);
  PTRACE(4,"PVidDirectShow\tset frame size " << newWidth << "x" << newHeight << " frame bytes=" << m_maxFrameBytes);
  return true;
}


PBoolean PVideoInputDevice_DirectShow::SetFrameRate(unsigned rate)
{
  PTRACE(4, "PVidDirectShow\tSetFrameRate("<<rate<<"fps)");

  if (rate < 1)
    rate = 1;
  else if (rate > 50)
    rate = 50;

  if (!SetAllParameters(colourFormat, frameWidth, frameHeight, rate))
    return false;

  return PVideoDevice::SetFrameRate(rate);
}

PBoolean PVideoInputDevice_DirectShow::SetColourFormat(const PString & colourFmt)
{
  PTRACE(4,"PVidDirectShow\tSetColourFormat("<<colourFmt<<")");

  if (!SetAllParameters(colourFmt, frameWidth, frameHeight, frameRate))
    return false;

  if (!PVideoDevice::SetColourFormat(colourFmt))
    return false;

  return true;
}


int PVideoInputDevice_DirectShow::GetControlCommon(long control)
{
  PComPtr<IAMVideoProcAmp> pVideoProcAmp;
  CHECK_ERROR(m_pCapture->QueryInterface(IID_IAMVideoProcAmp, (void **)&pVideoProcAmp), return -1);

  long minimum, maximum, stepping, def, flags;
  CHECK_ERROR(pVideoProcAmp->GetRange(control, &minimum, &maximum, &stepping, &def, &flags), return -1);

  long value;
  CHECK_ERROR(pVideoProcAmp->Get(control, &value, &flags), return -1);

  if (flags == VideoProcAmp_Flags_Auto)
    return -1;

  return ((value - minimum) * 65536) / (maximum-minimum);
}


int PVideoInputDevice_DirectShow::GetBrightness()
{
  return GetControlCommon(VideoProcAmp_Brightness);
}

int PVideoInputDevice_DirectShow::GetWhiteness()
{
  return GetControlCommon(VideoProcAmp_Gamma);
}

int PVideoInputDevice_DirectShow::GetColour()
{
  return GetControlCommon(VideoProcAmp_Saturation);
}

int PVideoInputDevice_DirectShow::GetContrast()
{
  return GetControlCommon(VideoProcAmp_Contrast);
}

int PVideoInputDevice_DirectShow::GetHue()
{
  return GetControlCommon(VideoProcAmp_Hue);
}

PBoolean PVideoInputDevice_DirectShow::GetParameters(int *whiteness, int *brightness, int *colour, int *contrast, int *hue)
{
  if (!IsOpen())
    return false;

  *whiteness  = GetWhiteness();
  *brightness = GetBrightness();
  *colour     = GetColour();
  *contrast   = GetContrast();
  *hue        = GetHue();

  return true;
}


PBoolean PVideoInputDevice_DirectShow::SetControlCommon(long control, int newValue)
{
  PTRACE(4, "PVidDirectShow\tSetControl() = " << newValue);

  PComPtr<IAMVideoProcAmp> pVideoProcAmp;
  CHECK_ERROR_RETURN(m_pCapture->QueryInterface(IID_IAMVideoProcAmp, (void **)&pVideoProcAmp));

  long minimum, maximum, stepping, def, flags;
  CHECK_ERROR_RETURN(pVideoProcAmp->GetRange(control, &minimum, &maximum, &stepping, &def, &flags));

  HRESULT hr;
  if (newValue == -1)
    hr = pVideoProcAmp->Set(control, 0, VideoProcAmp_Flags_Auto);
  else
  {
    long scaled = minimum + ((maximum-minimum) * newValue) / 65536;
    hr = pVideoProcAmp->Set(control, scaled, VideoProcAmp_Flags_Manual);
  }
  PTRACE_IF(2, FAILED(hr), "PVidDirectShow\tFailed to setRange interface on " << control << " : " << ErrorMessage(hr));

  return true;
}

PBoolean PVideoInputDevice_DirectShow::SetBrightness(unsigned newBrightness)
{
  return SetControlCommon(VideoProcAmp_Brightness, newBrightness);
}

PBoolean PVideoInputDevice_DirectShow::SetColour(unsigned newColour)
{
  return SetControlCommon(VideoProcAmp_Saturation, newColour);
}

PBoolean PVideoInputDevice_DirectShow::SetContrast(unsigned newContrast)
{
  return SetControlCommon(VideoProcAmp_Contrast, newContrast);
}

PBoolean PVideoInputDevice_DirectShow::SetHue(unsigned newHue)
{
  return SetControlCommon(VideoProcAmp_Hue, newHue);
}

PBoolean PVideoInputDevice_DirectShow::SetWhiteness(unsigned newWhiteness)
{
  return SetControlCommon(VideoProcAmp_Gamma, newWhiteness);
}


PBoolean PVideoInputDevice_DirectShow::GetDeviceCapabilities(const PString & deviceName, Capabilities * caps)
{
  PVideoInputDevice_DirectShow instance;
  return instance.Open(deviceName, PFalse) && instance.GetDeviceCapabilities(caps);
}


bool PVideoInputDevice_DirectShow::GetDeviceCapabilities(Capabilities * caps) const
{
  PComPtr<IAMStreamConfig> pStreamConfig;
  CHECK_ERROR_RETURN(m_pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE,
                                               &MEDIATYPE_Video,
                                               m_pCapture,
                                               IID_IAMStreamConfig,
                                               (void **)&pStreamConfig));

  int iCount, iSize;
  CHECK_ERROR_RETURN(pStreamConfig->GetNumberOfCapabilities(&iCount, &iSize));

  /* Sanity check: just to be sure that the Streamcaps is a VIDEOSTREAM and not AUDIOSTREAM */
  VIDEO_STREAM_CONFIG_CAPS scc;
  if (sizeof(scc) != iSize) {
    PTRACE(1, "PVidDirectShow\tBad Capapabilities (not a  VIDEO_STREAM_CONFIG_CAPS)");
    return false;
  }

  bool foundOne = false;

  for (int i = 0; i < iCount; i++) {
    MediaTypePtr pMediaFormat;
    CHECK_ERROR(pStreamConfig->GetStreamCaps(i, &pMediaFormat, (BYTE *)&scc), continue);

    if ((pMediaFormat->formattype == FORMAT_VideoInfo)     &&
        (pMediaFormat->cbFormat >= sizeof(VIDEOINFOHEADER)) &&
        (pMediaFormat->pbFormat != NULL))
    {
      VIDEOINFOHEADER & videoInfo = *(VIDEOINFOHEADER *)pMediaFormat->pbFormat;

      PVideoFrameInfo frameInfo;
      frameInfo.SetFrameSize(videoInfo.bmiHeader.biWidth, videoInfo.bmiHeader.biHeight);
      frameInfo.SetFrameRate((unsigned)(10000000.0/videoInfo.AvgTimePerFrame));

      bool notInTable = true;
      for (int j = 0; j < sizeof(ColourFormat2GUID)/sizeof(ColourFormat2GUID[0]); j++) {
        if (pMediaFormat->subtype == ColourFormat2GUID[j].m_guid) {
          frameInfo.SetColourFormat(ColourFormat2GUID[j].m_colourFormat);
          notInTable = false;
          break;
        }
      }

      if (notInTable) {
        wchar_t guidName[256];
        if (StringFromGUID2(pMediaFormat->subtype, guidName, sizeof(guidName)) <= 0)
          continue; // Can't use this entry!
        frameInfo.SetColourFormat(PString(guidName));
      }

      if (caps != NULL)
        caps->framesizes.push_back(frameInfo);

      PTRACE(4,"PVidDirectShow\tFmt["<< i << "] = ("
             << frameInfo.GetColourFormat() << ", "
             << frameInfo.GetFrameWidth() << "x" << frameInfo.GetFrameHeight() << ", "
             << frameInfo.GetFrameRate() << "fps)");

      foundOne = true;
    }
  }

  return foundOne;
}


#ifdef _WIN32_WCE

bool PVideoInputDevice_DirectShow::EnumerateDeviceNames(PStringArray & devices)
{
  GUID guidCamera = { 0xCB998A05, 0x122C, 0x4166, 0x84, 0x6A, 0x93, 0x3E, 0x4D, 0x7E, 0x3C, 0x86 };
  // Note about the above: The driver material doesn't ship as part of the SDK. This GUID is hardcoded
  // here to be able to enumerate the camera drivers and pass the name of the driver to the video capture filter

  DEVMGR_DEVICE_INFORMATION devInfo;
  devInfo.dwSize = sizeof(devInfo);

  HANDLE handle = FindFirstDevice(DeviceSearchByGuid, &guidCamera, &devInfo);
  if (handle == NULL) {
    PTRACE(1, "PVidDirectShow\tFindFirstDevice failed, error=" << ::GetLastError());
    return false;
  }

  do {
    if (devInfo.hDevice != NULL) {
      PString devName(devInfo.szLegacyName);
      devices.AppendString(devName);
      PTRACE(3, "PVidDirectShow\tFound capture device \""<< devName <<'"');
    }
  } while (FindNextDevice(handle, &devInfo));

  FindClose(handle);

  PTRACE_IF(2, devices.IsEmpty(), "PVidDirectShow\tNo video capture devices available.");

  return true;
}


class CPropertyBag : public IPropertyBag
{  
    struct VAR_LIST
    {
      VARIANT var;
      VAR_LIST *pNext;
      BSTR pBSTRName;
    }  * m_pVarList;
    LONG m_refCount;

  public:
    CPropertyBag()
       : m_refCount(1), m_pVarList(0)
    {
    }

    ~CPropertyBag()
    {
      VAR_LIST *pTemp = m_pVarList;
      while (pTemp != NULL) {
        VariantClear(&pTemp->var);
        SysFreeString(pTemp->pBSTRName);

        VAR_LIST * pDel = pTemp;
        pTemp = pTemp->pNext;
        delete pDel;
      }
    }

    HRESULT Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
    {
      VAR_LIST *pTemp = m_pVarList;
      while (pTemp != NULL) {
        if (0 == wcscmp(pszPropName, pTemp->pBSTRName))
          return VariantCopy(pVar, &pTemp->var);
        pTemp = pTemp->pNext;
      }

      return S_FALSE;
    }

    HRESULT Write(LPCOLESTR pszPropName, VARIANT *pVar)
    {
      VAR_LIST *pTemp = new VAR_LIST();
      if (pTemp == NULL)
        return E_OUTOFMEMORY;

      pTemp->pNext = m_pVarList;
      m_pVarList = pTemp;

      pTemp->pBSTRName = SysAllocString(pszPropName);

      VariantInit(&pTemp->var);
      return VariantCopy(&pTemp->var, pVar);
    }

    ULONG AddRef()
    {
      return InterlockedIncrement(&m_refCount);
    }

    ULONG Release()
    {
      ULONG ret = InterlockedDecrement(&m_refCount);
      if (ret == 0)
        delete this; 
      return ret;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv)
    {
      if (ppv == NULL) 
        return E_POINTER;

      if (riid != IID_IPropertyBag) {
        *ppv = 0;
        return E_NOINTERFACE;
      }

      *ppv = static_cast<IPropertyBag*>(this);	
      AddRef();
      return S_OK;
    }
};


bool PVideoInputDevice_DirectShow::CreateCaptureDevice(const PString & devName)
{
  // Create an instance of the video capture filter
  CHECK_ERROR_RETURN(CoCreateInstance(CLSID_VideoCapture, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void**)&m_pCapture));

  PComPtr<IPersistPropertyBag> pPropertyBag;
  CHECK_ERROR_RETURN(m_pCapture->QueryInterface(&pPropertyBag));

  VARIANT varName;
  varName.vt = VT_BSTR;
  varName.bstrVal = ::SysAllocString(devName.AsUCS2());

  CPropertyBag propBag;
  CHECK_ERROR_RETURN(propBag.Write(_T("VCapName"), &varName));
  CHECK_ERROR_RETURN(pPropertyBag->Load(&propBag, NULL));

  VariantClear(&varName);

  return true;
}


/* As WinCE does not have the ISampleGrabber component we have to fake it
   using a custom renderer. */

struct __declspec(  uuid("{71771540-2017-11cf-ae26-0020afd79767}")  ) CLSID_MySampleGrabber;


PSampleGrabber::PSampleGrabber(HRESULT * hr)
  : CBaseVideoRenderer(__uuidof(CLSID_MySampleGrabber), NAME("Frame Sample Grabber"), NULL, hr)
  , m_sampleSize(0)
  , m_sampleData(NULL)
{
}


HRESULT PSampleGrabber::CheckMediaType(const CMediaType *media)
{
  return *media->FormatType() == FORMAT_VideoInfo && IsEqualGUID(*media->Type(), MEDIATYPE_Video) ? S_OK : E_FAIL;
}


HRESULT PSampleGrabber::ShouldDrawSampleNow(IMediaSample *sample, REFERENCE_TIME *start, REFERENCE_TIME *stop)
{
  return S_OK; // disable dropping of frames
}


HRESULT PSampleGrabber::DoRenderSample(IMediaSample *sample)
{
  m_sampleMutex.Wait();

  m_sampleSize = sample->GetActualDataLength();
  sample->GetPointer(&m_sampleData);

  m_sampleMutex.Signal();

  return  S_OK;
}


HRESULT PSampleGrabber::GetCurrentBuffer(long * pSize, long * pData)
{
  m_sampleMutex.Wait();

  if (pSize != NULL)
    *pSize = m_sampleSize;

  if (pData != NULL)
    memcpy(pData, m_sampleData, m_sampleSize);

  m_sampleMutex.Signal();

  return S_OK;
}


bool PVideoInputDevice_DirectShow::CreateGrabberHandler()
{
  HRESULT hr = S_OK;
  PSampleGrabber * grabber = new PSampleGrabber(&hr);
  if (FAILED(hr)) {
    delete grabber;
    return false;
  }

  m_pSampleGrabber = grabber;

  CHECK_ERROR_RETURN(m_pGraph->AddFilter(dynamic_cast<IBaseFilter *>(grabber), L"Sampler"));

  // Find the source's output pin and the renderer's input pin
  PComPtr<IPin> pCapturePinOut;
  CHECK_ERROR_RETURN(m_pCapture->FindPin(L"Capture", &pCapturePinOut));

  PComPtr<IPin> pGrabberPinIn;
  CHECK_ERROR_RETURN(m_pSampleGrabber->FindPin(L"In", &pGrabberPinIn));

  // Connect these two filters pins
  CHECK_ERROR_RETURN(m_pGraph->Connect(pCapturePinOut, pGrabberPinIn));

  return true;
}


#else // _WIN32_WCE

class PComEnumerator
{
    PComPtr<ICreateDevEnum> m_pDevEnum;
    PComPtr<IEnumMoniker>   m_pClassEnum;
    PComPtr<IMoniker>       m_pMoniker;
  public:
    bool Open()
    {
      // Create the system device enumerator
      CHECK_ERROR_RETURN(CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum, (void **)&m_pDevEnum));
      // Create an enumerator for the video capture devices
      CHECK_ERROR_RETURN(m_pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &m_pClassEnum, 0));

      if (m_pClassEnum != NULL)
        return true;

      PTRACE(2, "PVidDirectShow\tNo video capture device was detected.");
      return false;
    }

    bool Next()
    {
      m_pMoniker.Release();

      ULONG cFetched;
      return m_pClassEnum->Next(1, &m_pMoniker, &cFetched) == S_OK;
    }

    IMoniker * GetMoniker() const { return m_pMoniker; }

    PString GetMonikerName()
    {
      PComPtr<IPropertyBag> pPropBag;
      CHECK_ERROR(m_pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag), return PString::Empty());

      // Find the description or friendly name.
      VARIANT varName;
      varName.vt = VT_BSTR;
      HRESULT hr = pPropBag->Read(L"Description", &varName, NULL);
      if (FAILED(hr))
        hr = pPropBag->Read(L"FriendlyName", &varName, NULL);
      if (FAILED(hr))
        return PString::Empty();

      PString name = varName.bstrVal;
      VariantClear(&varName);
      return name;
    }
};

bool PVideoInputDevice_DirectShow::EnumerateDeviceNames(PStringArray & devices)
{
  PTRACE(4,"PVidDirectShow\tEnumerating Device Names");

  PComEnumerator enumerator;
  if (!enumerator.Open())
    return false;

  while (enumerator.Next()) {
    PString name = enumerator.GetMonikerName();
    if (!name.IsEmpty())
      devices.AppendString(name);
  }

  return true;
}


bool PVideoInputDevice_DirectShow::CreateCaptureDevice(const PString & devName)
{
  PComEnumerator enumerator;
  if (!enumerator.Open())
    return false;

  while (enumerator.Next()) {
    if (enumerator.GetMonikerName() == devName) {
      // Bind Moniker to a filter object
      CHECK_ERROR_RETURN(enumerator.GetMoniker()->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_pCapture));
      PTRACE(4, "PVidDirectShow\tBound to device \""<< devName << '"');
      break;
    }
  }

  return true;
}


bool PVideoInputDevice_DirectShow::CreateGrabberHandler()
{
  // Create the Sample Grabber Filter.
  CHECK_ERROR_RETURN(CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pGrabberFilter));

  //Add the filter to the graph
  CHECK_ERROR_RETURN(m_pGraph->AddFilter(m_pGrabberFilter, L"Sample Grabber"));

  // Obtain interfaces for Sample Grabber
  CHECK_ERROR_RETURN(m_pGrabberFilter->QueryInterface(IID_ISampleGrabber, (void**)&m_pSampleGrabber));

  // Set some params
  CHECK_ERROR_RETURN(m_pSampleGrabber->SetBufferSamples(true));
  CHECK_ERROR_RETURN(m_pSampleGrabber->SetOneShot(false));

  // http://msdn2.microsoft.com/en-us/library/ms784859.aspx
  CHECK_ERROR_RETURN(m_pBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pCapture, NULL, m_pGrabberFilter));

  return true;
}

#endif // _WIN32_WCE


#endif /*P_DIRECTSHOW*/
