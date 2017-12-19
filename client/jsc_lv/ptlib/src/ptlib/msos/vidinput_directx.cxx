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
#include <strmif.h>
#include <atlbase.h>
#include <dshow.h>
#include <mmsystem.h>
#include <strmif.h>
#include <amvideo.h>    
#include <uuids.h>    
#include <control.h>
#include <ks.h>
#include <ksmedia.h>
#include "../../../../yImageEngine/yCvMgr.h"
#include <ptlib/msos/ptlib/vidinput_file.h>

struct DXVideoFormat
{ 
	WORD  bitCount;
	BOOL negHeight; 
	DWORD compression; 
	const GUID*  guid;
}; 

const GUID MEDIASUBTYPE_I420 = {
	MAKEFOURCC( 'I', '4', '2', '0' ), 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71};

//DEFINE_GUID(CLSID_MSMPJEG, //{301056D0-6DFF-11D2-9EEB-006008039E37}
//			0x301056D0, 0x6DFF, 0x11D2, 0x9E, 0xEB, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37);
  const GUID CLSID_MSMPJEG  = {0x301056D0, 0x6DFF, 0x11D2, 0x9E, 0xEB, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 };
  	                      //0x1dd5c4ae, 0x5456, 0x410a, 0xbd, 0xc3, 0x99, 0xfd, 0xdf, 0x81, 0xb0, 0x44);

static struct 
{
	const char * videoFormat; 
	DXVideoFormat format;
} FormatTable[] = {
	{ "RGB24",	{24,	FALSE,	BI_RGB,						&MEDIASUBTYPE_RGB24 } },
	{ "RGB24F",	{24,	TRUE,	BI_RGB,						&MEDIASUBTYPE_RGB24 } },
	{ "RGB32",	{32,	FALSE,	BI_RGB,						&MEDIASUBTYPE_RGB32 } },
	{ "RGB32F",	{32,	TRUE,	BI_RGB,						&MEDIASUBTYPE_RGB32 } },
	{ "Grey",	{8,		FALSE,	BI_RGB,						&MEDIASUBTYPE_RGB8 } },
	{ "Gray",	{8,		FALSE,	BI_RGB,						&MEDIASUBTYPE_RGB8 } },
	{ "GreyF",	{8,		TRUE,	BI_RGB,						&MEDIASUBTYPE_RGB8 } },
	{ "GrayF",	{8,		TRUE,	BI_RGB,						&MEDIASUBTYPE_RGB8 } },
	{ "RGB565",	{16,	FALSE,	BI_BITFIELDS,				&MEDIASUBTYPE_RGB565 } },
	{ "RGB565F",{16,	TRUE,	BI_BITFIELDS,				&MEDIASUBTYPE_RGB565 } },
	{ "RGB555",	{15,	FALSE,	BI_BITFIELDS,				&MEDIASUBTYPE_RGB555 } },
	{ "RGB555F",{15,	TRUE,	BI_BITFIELDS,				&MEDIASUBTYPE_RGB555 } },
	{ "YUV420",	{12,	FALSE,	MAKEFOURCC('I','Y','U','V'),&MEDIASUBTYPE_IYUV } },
	{ "YUV420P",{12,	FALSE,	MAKEFOURCC('I','Y','U','V'),&MEDIASUBTYPE_IYUV } },
	{ "IYUV",	{12,	FALSE,	MAKEFOURCC('I','Y','U','V'),&MEDIASUBTYPE_IYUV } },
	{ "I420",	{12,	FALSE,	MAKEFOURCC('I','4','2','0'),&MEDIASUBTYPE_I420 } },
	{ "YV12",	{12,	FALSE,	MAKEFOURCC('Y','V','1','2'),&MEDIASUBTYPE_YV12 } },
	{ "YUV422",	{16,	FALSE,	MAKEFOURCC('Y','U','Y','2'),&MEDIASUBTYPE_YUY2 } },
	{ "YUV422P",{16,	FALSE,	MAKEFOURCC('Y','U','Y','2'),&MEDIASUBTYPE_YUY2 } },
	{ "YUY2",	{16,	FALSE,	MAKEFOURCC('Y','U','Y','2'),&MEDIASUBTYPE_YUY2 } },
	{ "UYVY",	{16,	FALSE,	MAKEFOURCC('U','Y','V','Y'),&MEDIASUBTYPE_UYVY } },
	{ "YVYU",	{16,	FALSE,	MAKEFOURCC('Y','V','Y','U'),&MEDIASUBTYPE_YVYU } },
	{ "MJPEG",	{12,	FALSE,	MAKEFOURCC('M','J','P','G'),&MEDIASUBTYPE_MJPG } }
};

DXVideoFormat* WINAPI GetFormat( const char * videoFormat )
{
	if( videoFormat == NULL ) return NULL;

	for( int i=0; i < sizeof(FormatTable) / sizeof(FormatTable[0]); ++i ) {
		if( stricmp(videoFormat, FormatTable[i].videoFormat)==0 ) {
			return &FormatTable[i].format;
		}
	}
	return NULL;
} 
#ifdef P_DIRECTSHOW_LIBRARY1
#pragma comment(lib, P_DIRECTSHOW_LIBRARY1)
#endif
#ifdef P_DIRECTSHOW_LIBRARY2
#pragma comment(lib, P_DIRECTSHOW_LIBRARY2)
#endif
extern const char* D_CCVISION_HDCAPNAME  ;
//#define D_PIN_CAPTURE 

class PVideoFrameInfoSort
{
  public:
    bool operator()(PVideoFrameInfo const& f1, PVideoFrameInfo const& f2)
    {
      if (f1.GetFrameWidth() > f2.GetFrameWidth())
        return true;
      if (f1.GetFrameWidth() == f2.GetFrameWidth() && f1.GetFrameHeight() > f2.GetFrameHeight())
        return true;
      return false;
    }
};

class PVideoFrameInfoMatch
{
  public:
    bool operator()(PVideoFrameInfo& f1, PVideoFrameInfo& f2)
    {
      return (f1.GetFrameWidth()  == f2.GetFrameWidth()) &&
             (f1.GetFrameHeight() == f2.GetFrameHeight());
    }
};



PCREATE_VIDINPUT_PLUGIN(DirectShow2010);
  static const GUID MEDIASUBTYPE_HDYC = {0x43594448, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };
  //const GUID GUID_GFX  = {0x00000031, 0x4544, 0x0010, 0x80,0x00, 0x00, 0xaa, 0x00, 0x38,  0x9b, 0x71 };
  const GUID GUID_GFX  = {0x1dd5c4ae, 0x5456, 0x410a, 0xbd, 0xc3, 0x99, 0xfd, 0xdf, 0x81, 0xb0, 0x44 };
  	                      //0x1dd5c4ae, 0x5456, 0x410a, 0xbd, 0xc3, 0x99, 0xfd, 0xdf, 0x81, 0xb0, 0x44);
//// {CF49D4E0-1115-11ce-B03A-0020AF0BA770}         AVI Decoder
//OUR_GUID_ENTRY(CLSID_AVIDec,
//0xcf49d4e0, 0x1115, 0x11ce, 0xb0, 0x3a, 0x0, 0x20, 0xaf, 0xb, 0xa7, 0x70)

 // {CF49D4E0-1115-11ce-B03A-0020AF0BA770}         AVI Decoder
//OUR_GUID_ENTRY(GUID_GFX,
//0x1dd5c4ae, 0x5456, 0x410a, 0xbd, 0xc3, 0x99, 0xfd, 0xdf, 0x81, 0xb0, 0x44)

//
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }
#endif


 void WINAPI BBQFreeMediaType(AM_MEDIA_TYPE& mt)
{
	if (mt.cbFormat != 0) {
		if( mt.pbFormat != 0 ) CoTaskMemFree((PVOID)mt.pbFormat);

		// Strictly unnecessary but tidier
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}
	if (mt.pUnk != NULL) {
		mt.pUnk->Release();
		mt.pUnk = NULL;
	}
}
void WINAPI BBQDeleteMediaType(AM_MEDIA_TYPE *pmt)
{
	// allow NULL pointers for coding simplicity

	if (pmt == NULL) {
		return;
	}

	BBQFreeMediaType(*pmt);
	CoTaskMemFree((PVOID)pmt);
}
HRESULT GetPin( IBaseFilter * pFilter, PIN_DIRECTION dirrequired, int iNum, IPin **ppPin)
{
	CComPtr< IEnumPins > pEnum;
	*ppPin = NULL;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if(FAILED(hr)) 
		return hr;

	ULONG ulFound;
	IPin *pPin;
	hr = E_FAIL;
	while(S_OK == pEnum->Next(1, &pPin, &ulFound))
	{
		PIN_DIRECTION pindir = (PIN_DIRECTION)3;
		pPin->QueryDirection(&pindir);
		if(pindir == dirrequired)
		{
			if(iNum == 0)
			{
//				pPin->AddRef();
				*ppPin = pPin;
				break;
			}
			iNum--;
		} // if
		pPin->Release();
	} // while

	return hr;
}


IPin * GetInPin( IBaseFilter * pFilter, int Num )
{
	IPin* pComPin = NULL;
	GetPin(pFilter, PINDIR_INPUT, Num, &pComPin);
	return pComPin;
}


IPin * GetOutPin( IBaseFilter * pFilter, int Num )
{
	IPin* pComPin = NULL;
	GetPin(pFilter, PINDIR_OUTPUT, Num, &pComPin);
	return pComPin;
}

IPin* GetOutVideoPin( IBaseFilter * pFilter, int iNum )
{
	IPin* pComPin = NULL;

	CComPtr< IEnumPins > pEnum;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if(FAILED(hr)) 
		return 0;

	ULONG ulFound;
	IPin *pPin;
	hr = E_FAIL;
	while(S_OK == pEnum->Next(1, &pPin, &ulFound))
	{
		PIN_DIRECTION pindir = (PIN_DIRECTION)3;
		pPin->QueryDirection(&pindir);
		if(pindir == PINDIR_OUTPUT)
		{
			BOOL bFind = FALSE;
			IEnumMediaTypes *pEnum1 = 0;
			pPin->EnumMediaTypes(&pEnum1);
			if( pEnum1 ) {
				AM_MEDIA_TYPE* pMediaType;
				while( !bFind && S_OK == pEnum1->Next(1, &pMediaType, &ulFound)) {
					if( pMediaType->majortype == MEDIATYPE_Video || pMediaType->majortype == MEDIATYPE_Interleaved || 	pMediaType->majortype == GUID_NULL ) {
						bFind = TRUE;
					}
					BBQDeleteMediaType(pMediaType);

				}
				pEnum1->Release();
			}
			if( bFind ) {
				if(iNum == 0)
				{
					pComPin = pPin;
					break;
				}
				iNum--;
			}
		} // if
		pPin->Release();
	} // while
	return pComPin;
}


 //

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
 { "UYVY422", MEDIASUBTYPE_HDYC   },
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

  PTRACE(1,"PVidDirectShow2010\tFunction \"" << fn << "\" failed : " << ErrorMessage(hr) <<",hr=" << hr   );
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

struct DirectShow2010InputDeviceInfo {
  Capabilities * capabilities;
  PThread * thread;
  PVideoInputDevice_DirectShow2010 * device;
};

typedef std::map<std::string, DirectShow2010InputDeviceInfo> DirectShow2010InputDeviceInfoMap;

static DirectShow2010InputDeviceInfoMap directShowInputDeviceInfoMap;

static void InitialiseCache()
{
  bool PVideoInputDevice_DirectShow2010::EnumerateDeviceNames(PStringArray & devices)

}

#endif

///////////////////////////////////////////////////////////////////////////////

PVideoInputDevice_DirectShow2010::PVideoInputDevice_DirectShow2010()
  : m_isCapturing(false),m_bVirtualCamera(false)
  , m_maxFrameBytes(0)
{
  PTRACE(4,"PVidDirectShow2010\tPVideoInputDevice_DirectShow2010: constructor");
//  m_bSetMediaFormat = false;
  CoInitializeEx(NULL, COINIT_MULTITHREADED);
#ifdef D_CVMGR
  m_cvMgr = new yCvMgr();
#endif
  m_color_YUV420P_BGR24= m_color_BGR24_YUV420P =0;
}


PVideoInputDevice_DirectShow2010::~PVideoInputDevice_DirectShow2010()
{
  Close();
  //::CoUninitialize();
  PTRACE(4,"PVidDirectShow2010\tPVideoInputDevice_DirectShow2010: destructor");
  #ifdef D_CVMGR
  delete m_cvMgr;
  m_cvMgr = NULL;
#endif
}


PStringArray PVideoInputDevice_DirectShow2010::GetInputDeviceNames()
{
  PStringArray devices;
  PVideoInputDevice_DirectShow2010 instance;
  instance.EnumerateDeviceNames(devices);
  return devices;
}

PBoolean PVideoInputDevice_DirectShow2010::OleCreateProperty(HWND hOwer)
{
   if (IsCapturing() == false)
     return true;
  ISpecifyPropertyPages *pSpec;
  CAUUID cauuid;

  HRESULT hr = m_pCapture->QueryInterface(IID_ISpecifyPropertyPages,
      (void **)&pSpec);
  if(hr == S_OK)
  {
    m_pCapture->Stop();
    //m_pMediaControl->StopWhenReady();
      hr = pSpec->GetPages(&cauuid);

      hr = OleCreatePropertyFrame(hOwer, 30, 30, NULL, 1,
          (IUnknown **)&m_pCapture, cauuid.cElems,
          (GUID *)cauuid.pElems, 0, 0, NULL);

      CoTaskMemFree(cauuid.pElems);
      m_pCapture->Run(5);
     // pSpec->Release();
     m_pMediaControl->Run();
  }
  else return false;

  return true;
}

PBoolean PVideoInputDevice_DirectShow2010::Open(const PString & devName, PBoolean startImmediate)
{
  PTRACE(4,"PVidDirectShow2010\tOpen(\"" << devName << "\", " << startImmediate << ')');
  deviceName= devName;
  Close();

  // Create the filter graph
  CHECK_ERROR_RETURN(CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **) &m_pGraph));

  // Create the capture graph builder
  CHECK_ERROR_RETURN(CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void **) &m_pBuilder));

  if (!CreateCaptureDevice(devName))
    return false;

  // Add Capture filter to our graph.
  CHECK_ERROR_RETURN(m_pGraph->AddFilter(m_pCapture, L"Video Capture"));//chenyuanAddFilter
  if (this->deviceName == D_CCVISION_HDCAPNAME ){
  //begin
    CHECK_ERROR_RETURN(CoCreateInstance(GUID_GFX, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pDeinterlacer));

    CHECK_ERROR_RETURN(m_pGraph->AddFilter(m_pDeinterlacer, L"Deinterlace"));

    CHECK_ERROR_RETURN(CoCreateInstance(CLSID_AVIDec, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pAVI));
    CHECK_ERROR_RETURN(m_pGraph->AddFilter(m_pAVI, L"AVIdecoder"));

    CHECK_ERROR_RETURN(CoCreateInstance(CLSID_SmartTee, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pSmartTee));
    CHECK_ERROR_RETURN(m_pGraph->AddFilter(m_pSmartTee, L"Smart Tee"));

    CHECK_ERROR_RETURN(CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pNullRender));
    CHECK_ERROR_RETURN(m_pGraph->AddFilter(m_pNullRender, L"NULL render"));
  }
#ifdef D_PIN_CAPTURE
   
  //begin
     CHECK_ERROR_RETURN(CoCreateInstance(CLSID_MjpegDec/*CLSID_MSMPJEG*/, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pMJPEGFiler));
    CHECK_ERROR_RETURN(m_pGraph->AddFilter(m_pMJPEGFiler, L"MJPEG"));

    CHECK_ERROR_RETURN(CoCreateInstance(CLSID_Colour , NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pColorFiler));
    CHECK_ERROR_RETURN(m_pGraph->AddFilter(m_pColorFiler, L"color"));
	/*	if( !m_pMJPEGFiler ) {
				HRESULT hr = m_pMJPEGFiler->CoCreateInstance(CLSID_MSMPJEG);
				if( FAILED(hr) ) {
					m_pMJPEGFiler = 0;
				}
			}*/

    CHECK_ERROR_RETURN(CoCreateInstance(CLSID_AVIDec, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pAVI));
    CHECK_ERROR_RETURN(m_pGraph->AddFilter(m_pAVI, L"AVIdecoder"));

    CHECK_ERROR_RETURN(CoCreateInstance(CLSID_SmartTee, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pSmartTee));
    CHECK_ERROR_RETURN(m_pGraph->AddFilter(m_pSmartTee, L"Smart Tee"));

    CHECK_ERROR_RETURN(CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pNullRender));
    CHECK_ERROR_RETURN(m_pGraph->AddFilter(m_pNullRender, L"NULL render"));
#endif

  //end
  // Attach the filter graph to the capture graph
  CHECK_ERROR_RETURN(m_pBuilder->SetFiltergraph(m_pGraph));

  // Obtain interfaces for media control and Video Window
  CHECK_ERROR_RETURN(m_pGraph->QueryInterface(IID_IMediaControl,(void **)&m_pMediaControl));

  //propety
  //ISpecifyPropertyPages *pSpec;
  //CAUUID cauuid;
  //HRESULT hr = m_pCapture->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpec);
  //if(SUCCEEDED(hr))
  //{
  //  hr = pSpec->GetPages(&cauuid);
  //  if(SUCCEEDED(hr) && cauuid.cElems > 0)
  //  {
  //      //AppendMenu(hMenuSub,MF_STRING,MENU_DIALOG0+zz, TEXT("Video Capture Filter..."));
  //      //gcap.iVCapDialogPos = zz++;
  //      CoTaskMemFree(cauuid.pElems);
  //  }
  //  pSpec->Release();
  //}

  if (!CreateGrabberHandler()){
    m_bVirtualCamera = true;
    return true;//false;

  }
    
  PTRACE(4,"PVidDirectShow2010\tStart of GetDeviceCapabilities for c(\"" << devName << "\"");

  Capabilities caps;
   (GetDeviceCapabilities(&caps));
    

  PTRACE(4,"PVidDirectShow2010\tEnd of GetDeviceCapabilities for c(\"" << devName << "\"");

  if (startImmediate)
    return Start();

  deviceName = devName;

  return true;
}

PBoolean PVideoInputDevice_DirectShow2010::IsOpen()
{
    return m_pCapture != NULL;
}

PBoolean PVideoInputDevice_DirectShow2010::Close()
{
  if (!IsOpen())
    return false;

  PTRACE(4,"PVidDirectShow2010\tClosing \"" << deviceName << '"');

  if (m_pMediaControl)
    m_pMediaControl->StopWhenReady();

  m_pMediaControl.Release();
#ifdef _WIN32_WCE
  delete m_pSampleGrabber;
#else
  m_pSampleGrabber.Release();
#endif
  m_pCapture.Release();
  m_pDeinterlacer.Release();
  m_pMJPEGFiler.Release();
  m_pAVI.Release();
  m_pSmartTee.Release();
  m_pBuilder.Release();
  m_pGraph.Release();

  return true;
}


PBoolean PVideoInputDevice_DirectShow2010::Start()
{
   if (m_bVirtualCamera) 
     return true;
  if (IsCapturing())
    return true;

  PTRACE(4,"PVidDirectShow2010\tStart()");

  // Start previewing video data
  CHECK_ERROR_RETURN(m_pMediaControl->Run());

  /* Even after a WaitForCompletion, the webcam is sometimes not available, so wait
     until the server give us a frame before returning */
  for (unsigned retry = 0; retry < 200; ++retry) {
    long cbBuffer;
    HRESULT hr = m_pSampleGrabber->GetCurrentBuffer(&cbBuffer, NULL);
    if (SUCCEEDED(hr) && cbBuffer > 0) {
      m_isCapturing = true;
      return true;
    }

    PTRACE_IF(2, FAILED(hr) && hr != VFW_E_WRONG_STATE, "PVidDirectShow2010\tError waiting for camera: " << ErrorMessage(hr));

    PThread::Sleep(50); /* Not available */
  }

  PTRACE(2, "PVidDirectShow2010\tTime out waiting for first frame.");
  return false;
}

PBoolean PVideoInputDevice_DirectShow2010::Stop()
{
  if (IsCapturing())
    return false;

  PTRACE(4,"PVidDirectShow2010\tStop()");

  if (m_pMediaControl)
    m_pMediaControl->StopWhenReady();

  m_isCapturing = false;

  return true;
}


PBoolean PVideoInputDevice_DirectShow2010::IsCapturing()
{
  return m_isCapturing;
}
enum{
  em_drawText,em_drawLine
};
 extern int g_nAudioInputLevel  ;
//extern  g_VideoMerge(int  e,BYTE* buffer,int w, int h, void* n );
PBoolean PVideoInputDevice_DirectShow2010::GetFrameData(BYTE * buffer, PINDEX * bytesReturned)
{

  m_pacing.Delay(1000/GetFrameRate());
  if (m_isCapturing ==false  ){
    PThread::Sleep(1000);
    *bytesReturned =0;
    return true;
  }
  PBoolean ret = GetFrameDataNoDelay(buffer, bytesReturned);
   if(ret&& *bytesReturned <=0 ){
    PThread::Sleep(100);
    return true;
  }
  if (g_nAudioInputLevel>1 &&*bytesReturned>0){
    PString src="YUV420P",dst = "BGR24";

   
    // m_color_YUV420P_BGR24, m_color_BGR24_YUV420P
      //yuv to bgr
    if (m_color_YUV420P_BGR24 == NULL){
      m_color_YUV420P_BGR24 = PColourConverter::Create(src , dst, this->GetFrameWidth(), this->GetFrameHeight() );
    }else if ( m_color_YUV420P_BGR24->GetSrcFrameWidth() != this->GetFrameWidth() || m_color_YUV420P_BGR24->GetSrcFrameHeight() != this->GetFrameHeight()){
      m_color_YUV420P_BGR24->SetSrcFrameSize(this->GetFrameWidth(), this->GetFrameHeight() );
      m_color_YUV420P_BGR24->SetDstFrameSize(this->GetFrameWidth(), this->GetFrameHeight() );

    }
    //dst bgr24 to yuv420 p
    if (m_color_BGR24_YUV420P == NULL){
      m_color_BGR24_YUV420P = PColourConverter::Create(dst,src ,  this->GetFrameWidth(), this->GetFrameHeight() );
    }else if ( m_color_BGR24_YUV420P->GetSrcFrameWidth() != this->GetFrameWidth() || m_color_BGR24_YUV420P->GetSrcFrameHeight() != this->GetFrameHeight()){
      m_color_BGR24_YUV420P->SetSrcFrameSize(this->GetFrameWidth(), this->GetFrameHeight() );
      m_color_BGR24_YUV420P->SetDstFrameSize(this->GetFrameWidth(), this->GetFrameHeight() );

    }



    int nReturn=0;
    
    if (m_colorYUV420P_BGR24Array.GetSize() < (*bytesReturned)*2)
      m_colorYUV420P_BGR24Array.SetSize((*bytesReturned)*2);
    BYTE* pdst =  m_colorYUV420P_BGR24Array.GetPointer();
    m_color_YUV420P_BGR24->Convert(buffer, pdst, *bytesReturned, &nReturn);
    //PFile f;
    //f.Open(PString("d:\\1.dat") );
    //f.Write( pdst, nReturn);
    //f.Close();

    ////
  #ifdef D_CVMGR
    m_cvMgr->SetAudioStatus(pdst, nReturn, this->GetFrameWidth(), this->GetFrameHeight(),g_nAudioInputLevel/* * GetFrameWidth()*0.5 /100 *//*g_nAudioInputLevel*/   );
#endif
    m_color_BGR24_YUV420P->Convert(pdst, buffer, nReturn, bytesReturned);
    //write video data to disk file

    //void SetAudioStatus( BYTE* buffer,int bufLen, int w, int h, int x);
    //g_VideoMerge( em_drawLine, buffer, this->GetFrameWidth(), this->GetFrameHeight(), n );
  }
  //if (m_cvMgr==NULL)
  //{
//#define D_WRTIEVIDEO_DISK
#ifdef D_WRTIEVIDEO_DISK
  //if (this->GetFrameWidth()==999)
  {
    PVideoInputDevice_CCFile::CCRecoreFile_Open(GetFrameHeight(), GetFrameWidth(), GetFrameRate(), GetFrameHeight()*GetFrameWidth()*3/2);
    PVideoInputDevice_CCFile::CCRecoreFile_Write( buffer,  GetFrameHeight()*GetFrameWidth()*3/2);
  }
#endif
  //}
    return ret;
}
PBoolean PVideoInputDevice_DirectShow2010::GetFrameSize(unsigned & width, unsigned & height) const
{
  // Channels get very upset at this not returning the output size.
#if 0
  return  PVideoFrameInfo::GetFrameSize(width, height);
#else
  return converter != NULL ? converter->GetDstFrameSize(width, height) : PVideoFrameInfo::GetFrameSize(width, height);
#endif
}

PBoolean PVideoInputDevice_DirectShow2010::GetFrameDataNoDelay(BYTE *destFrame, PINDEX * bytesReturned)
{
  if ( m_bVirtualCamera){
    SetFrameRate(1);
    int nLen= frameWidth* frameHeight*3/2 ;
    if (nLen != m_arrVirtualData.GetSize()){
      m_arrVirtualData.SetSize(nLen);

      int npixels = frameWidth * frameHeight;
      int i=0;
      BYTE * p =m_arrVirtualData.GetPointer();
      memset(p, 0x0, frameWidth* frameHeight); // black
   
      memset(&p[frameWidth* frameHeight], 0x80, frameWidth/2* frameHeight/2);
      memset(&p[frameWidth* frameHeight+ frameWidth/2* frameHeight/2], 0x80, frameWidth/2* frameHeight/2);
  

      ////d = dest;
      //for (i=0; i < npixels; i++) 
      //  *d0++ = BLACK_Y;
      //for (i=0; i < npixels/4; i++)
      //  *d1++ = BLACK_U;
      //for (i=0; i < npixels/4; i++)
      //  *d2++ = BLACK_V;
     // memset(m_arrVirtualData.GetPointer(), 128, nLen);
    }
    memcpy(destFrame, m_arrVirtualData.GetPointer(), nLen);
    *bytesReturned= nLen;
    return true;
  }
  long cbBuffer = m_maxFrameBytes;
/*	if (m_pSampleGrabber==NULL) 
		return PFalse*/;

  /*CHECK_ERROR_RETURN*/
  if (m_pSampleGrabber->GetCurrentBuffer(&cbBuffer, NULL)!= S_OK ){
    PTRACE(1, "direct\t GetCurrentBuffer =0");
    * bytesReturned =0;
    return true;
  }
  if (converter != NULL) {
   // unsigned int w=0,h=0;
    //PVideoFrameInfo::GetFrameSize(w, h);
    if (converter->GetSrcFrameHeight() == frameHeight && converter->GetSrcFrameWidth() == frameWidth)
		{
     // memset(m_tempFrame.GetPointer(cbBuffer), 0,cbBuffer);
      if  (converter && converter->GetSrcColourFormat()!="MJPEG"){
			  CHECK_ERROR_RETURN(m_pSampleGrabber->GetCurrentBuffer(&cbBuffer, (long*)m_tempFrame.GetPointer(cbBuffer)));
        //m_pSampleGrabber->GetCurrentBuffer(&cbBuffer, (long*)m_tempFrame.GetPointer(cbBuffer));
			  if (cbBuffer == m_maxFrameBytes)
				  converter->Convert(m_tempFrame, destFrame, cbBuffer, bytesReturned);
      }
      else{//mjpeg
			  //CHECK_ERROR_RETURN(m_pSampleGrabber->GetCurrentBuffer(&cbBuffer, (long*)m_tempFrame.GetPointer(cbBuffer)));
        m_pSampleGrabber->GetCurrentBuffer(&cbBuffer, (long*)m_tempFrame.GetPointer(cbBuffer));
			  //if (cbBuffer == m_maxFrameBytes)
				  converter->Convert(m_tempFrame, destFrame, cbBuffer, bytesReturned);
          cbBuffer= m_maxFrameBytes;
      }
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


PINDEX PVideoInputDevice_DirectShow2010::GetMaxFrameBytes()
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
PBoolean PVideoInputDevice_DirectShow2010::SetAllParameters(const PString & newColourFormat, int newWidth, int newHeight, int fps)
{
  if (m_bVirtualCamera )
    return true;
  //fps=29;
 if ( newColourFormat=="MJPEG")
{
  fps=30;
}
//newWidth= 1920;newHeight=1080;
  PTRACE(4, "PVidDirectShow2010\tSetCameraFormat(\""
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
    PTRACE(1, "PVidDirectShow2010\tBad Capapabilities (not a VIDEO_STREAM_CONFIG_CAPS)");
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
    PTRACE(4,"PVidDirectShow2010\tMatched setting "<< i);

    OAFilterState filterState = State_Stopped;
    HRESULT hr = m_pMediaControl->GetState(1000, &filterState);
    PTRACE_IF(1, FAILED(hr), "PVidDirectShow2010\tGetState failed: " << ErrorMessage(hr));
    m_pMediaControl->StopWhenReady();
    if (deviceName == D_CCVISION_HDCAPNAME&& i>=8){
      hr = pStreamConfig->SetFormat(pMediaFormat);
      //m_bSetMediaFormat =true;
    }else if (deviceName != D_CCVISION_HDCAPNAME){
     // pMediaFormat->subtype =MEDIASUBTYPE_RGB24;
       hr = pStreamConfig->SetFormat(pMediaFormat);
    }
    if (FAILED(hr))
    {
      PTRACE(2, "PVidDirectShow2010\tFailed to setFormat: " << ErrorMessage(hr));
      if (hr != VFW_E_INVALIDMEDIATYPE)
        continue;

      PTRACE(3, "PVidDirectShow2010\tRetrying ...");
      bool was_capturing = m_isCapturing;
      Close();
      Open(deviceName, false);
       
      hr = pStreamConfig->SetFormat(pMediaFormat);
      if (FAILED(hr)) {
        PTRACE(1, "PVidDirectShow2010\tFailed to setFormat (Try #2 graph deconnected): " << ErrorMessage(hr));
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

    PTRACE(3, "PVidDirectShow2010\tSet new parameters: \""
           << newColourFormat << "\", "
           << newWidth << 'x' << newHeight << ", "
           << fps <<"fps");
    return true;
  }

  PTRACE(1, "PVidDirectShow2010\tNo matching capability for \""
         << newColourFormat << "\", "
         << newWidth << 'x' << newHeight << ", "
         << fps <<"fps");
  return false;
}


PBoolean PVideoInputDevice_DirectShow2010::SetFrameSize(unsigned newWidth, unsigned newHeight)
{

  //newWidth =1920;newHeight=1080;
  PTRACE(4,"PVidDirectShow2010\tSetFrameSize(" << newWidth << ", " << newHeight << ")");

  if (!SetAllParameters(colourFormat, newWidth, newHeight, frameRate))
    return false;

  PTRACE(3,"PVidDirectShow2010\tSetFrameSize " << newWidth << "x" << newHeight << " is suported in hardware");

  if (!PVideoDevice::SetFrameSize(newWidth, newHeight))
    return false;

  m_maxFrameBytes = CalculateFrameBytes(frameWidth, frameHeight, colourFormat);
  PTRACE(4,"PVidDirectShow2010\tset frame size " << newWidth << "x" << newHeight << " frame bytes=" << m_maxFrameBytes);
  return true;
}


PBoolean PVideoInputDevice_DirectShow2010::SetFrameRate(unsigned rate)
{
  PTRACE(4, "PVidDirectShow2010\tSetFrameRate("<<rate<<"fps)");

  if (rate < 1)
    rate = 1;
  else if (rate > 50)
    rate = 50;

  
  if (!SetAllParameters(colourFormat, frameWidth, frameHeight, rate))
  {
    if(this->deviceName != D_CCVISION_HDCAPNAME )
      return false;
  }
  return PVideoDevice::SetFrameRate(rate);
}

PBoolean PVideoInputDevice_DirectShow2010::SetColourFormat(const PString & colourFmt)
{
  
  PTRACE(4,"PVidDirectShow2010\tSetColourFormat("<<colourFmt<<")");

  if (!SetAllParameters(colourFmt, frameWidth, frameHeight, frameRate))
    return false;

  if (!PVideoDevice::SetColourFormat(colourFmt))
    return false;

  return true;
}


int PVideoInputDevice_DirectShow2010::GetControlCommon(long control)
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


int PVideoInputDevice_DirectShow2010::GetBrightness()
{
  return GetControlCommon(VideoProcAmp_Brightness);
}

int PVideoInputDevice_DirectShow2010::GetWhiteness()
{
  return GetControlCommon(VideoProcAmp_Gamma);
}

int PVideoInputDevice_DirectShow2010::GetColour()
{
  return GetControlCommon(VideoProcAmp_Saturation);
}

int PVideoInputDevice_DirectShow2010::GetContrast()
{
  return GetControlCommon(VideoProcAmp_Contrast);
}

int PVideoInputDevice_DirectShow2010::GetHue()
{
  return GetControlCommon(VideoProcAmp_Hue);
}

PBoolean PVideoInputDevice_DirectShow2010::GetParameters(int *whiteness, int *brightness, int *colour, int *contrast, int *hue)
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


PBoolean PVideoInputDevice_DirectShow2010::SetControlCommon(long control, int newValue)
{
  PTRACE(4, "PVidDirectShow2010\tSetControl() = " << newValue);

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
  PTRACE_IF(2, FAILED(hr), "PVidDirectShow2010\tFailed to setRange interface on " << control << " : " << ErrorMessage(hr));

  return true;
}

PBoolean PVideoInputDevice_DirectShow2010::SetBrightness(unsigned newBrightness)
{
  return SetControlCommon(VideoProcAmp_Brightness, newBrightness);
}

PBoolean PVideoInputDevice_DirectShow2010::SetColour(unsigned newColour)
{
  return SetControlCommon(VideoProcAmp_Saturation, newColour);
}

PBoolean PVideoInputDevice_DirectShow2010::SetContrast(unsigned newContrast)
{
  return SetControlCommon(VideoProcAmp_Contrast, newContrast);
}

PBoolean PVideoInputDevice_DirectShow2010::SetHue(unsigned newHue)
{
  return SetControlCommon(VideoProcAmp_Hue, newHue);
}

PBoolean PVideoInputDevice_DirectShow2010::SetWhiteness(unsigned newWhiteness)
{
  return SetControlCommon(VideoProcAmp_Gamma, newWhiteness);
}


PBoolean PVideoInputDevice_DirectShow2010::GetDeviceCapabilities(const PString & deviceName, Capabilities * caps)
{
  PVideoInputDevice_DirectShow2010 instance;
  return instance.Open(deviceName, PFalse) && instance.GetDeviceCapabilities(caps);
}

struct _equal_list_
{
    bool operator()(const PVideoFrameInfo& x1, const PVideoFrameInfo& x2)
    {
      if(x1.GetFrameWidth() == x2.GetFrameWidth() &&x1.GetFrameHeight() == x2.GetFrameHeight()){
            return true;
        }
        else
        {
            return false;
        }
    }
};
bool PVideoInputDevice_DirectShow2010::GetDeviceCapabilities(Capabilities * caps) const
{
  //if (m_curCapabilities.framesizes.size() >0 ) {
  //  *caps = m_curCapabilities;
  //  return true;
  //}
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
    PTRACE(1, "PVidDirectShow2010\tBad Capapabilities (not a  VIDEO_STREAM_CONFIG_CAPS)");
    return false;
  }

  bool foundOne = false;
//  PVideoFrameInfoArray fsizes;
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
      if ( frameInfo.GetFrameWidth() >=176&& (this->deviceName != D_CCVISION_HDCAPNAME ||    frameInfo.GetFrameWidth() == 1920 ))
     // if (caps != NULL && frameInfo.GetFrameWidth() == 1920)
      {
        
        caps->framesizes.push_back(frameInfo);

        PTRACE(4,"PVidDirectShow2010\tFmt["<< i << "] = ("
              << frameInfo.GetColourFormat() << ", "
              << frameInfo.GetFrameWidth() << "x" << frameInfo.GetFrameHeight() << ", "
              << frameInfo.GetFrameRate() << "fps)");
      }

      foundOne = true;
    }
  }
  //((Capabilities&)m_curCapabilities) =  (*caps);
 // caps->framesizes.unique(_equal_list
 /* if ( deviceName != D_CCVISION_HDCAPNAME){
    PVideoFrameInfo frameInfo;
    frameInfo.SetFrameSize(960 ,  540);
    caps->framesizes.push_back(frameInfo);
  }*/
  caps->framesizes.sort();
  caps->framesizes.unique(_equal_list_());

  //sort and uniqu
     

  return foundOne;
}


#ifdef _WIN32_WCE

bool PVideoInputDevice_DirectShow2010::EnumerateDeviceNames(PStringArray & devices)
{
  GUID guidCamera = { 0xCB998A05, 0x122C, 0x4166, 0x84, 0x6A, 0x93, 0x3E, 0x4D, 0x7E, 0x3C, 0x86 };
  // Note about the above: The driver material doesn't ship as part of the SDK. This GUID is hardcoded
  // here to be able to enumerate the camera drivers and pass the name of the driver to the video capture filter

  DEVMGR_DEVICE_INFORMATION devInfo;
  devInfo.dwSize = sizeof(devInfo);

  HANDLE handle = FindFirstDevice(DeviceSearchByGuid, &guidCamera, &devInfo);
  if (handle == NULL) {
    PTRACE(1, "PVidDirectShow2010\tFindFirstDevice failed, error=" << ::GetLastError());
    return false;
  }

  do {
    if (devInfo.hDevice != NULL) {
      PString devName(devInfo.szLegacyName);
      devices.AppendString(devName);
      PTRACE(3, "PVidDirectShow2010\tFound capture device \""<< devName <<'"');
    }
  } while (FindNextDevice(handle, &devInfo));

  FindClose(handle);

  PTRACE_IF(2, devices.IsEmpty(), "PVidDirectShow2010\tNo video capture devices available.");

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


bool PVideoInputDevice_DirectShow2010::CreateCaptureDevice(const PString & devName)
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


bool PVideoInputDevice_DirectShow2010::CreateGrabberHandler()
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
#if 0
  PComPtr<IPin> pGrabberPinIn;
  CHECK_ERROR_RETURN(m_pSampleGrabber->FindPin(L"In", &pGrabberPinIn));

  // Connect these two filters pins
  CHECK_ERROR_RETURN(m_pGraph->Connect(pCapturePinOut, pGrabberPinIn));
#else
  //chenyuan begin
   

  //end
#endif
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
	//USES_CONVERSION;

	//ICreateDevEnum *pCreateDevEnum;
	//HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
	//	IID_ICreateDevEnum, (void**)&pCreateDevEnum);
	//if (hr != NOERROR) {
	//	PTRACE(5, "Error Creating Device Enumerator");
	//	return FALSE;
	//}

	//IEnumMoniker *pEm;
	//hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
	//	&pEm, 0);
	//if (hr != NOERROR) {
	//	PTRACE(5, "Sorry, you have no video capture hardware");
	//	pCreateDevEnum->Release();
	//	return FALSE;
	//}

	// Create the system device enumerator
      CHECK_ERROR_RETURN(CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER/*CLSCTX_INPROC*/, IID_ICreateDevEnum, (void **)&m_pDevEnum));
      // Create an enumerator for the video capture devices
      CHECK_ERROR_RETURN(m_pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &m_pClassEnum, 0));

      if (m_pClassEnum != NULL)
        return true;

      PTRACE(2, "PVidDirectShow2010\tNo video capture device was detected.");
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

bool PVideoInputDevice_DirectShow2010::EnumerateDeviceNames(PStringArray & devices)
{
  PTRACE(4,"PVidDirectShow2010\tEnumerating Device Names");

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


bool PVideoInputDevice_DirectShow2010::CreateCaptureDevice(const PString & devName)
{
  PComEnumerator enumerator;
  if (!enumerator.Open())
    return false;

  while (enumerator.Next()) {
    if (enumerator.GetMonikerName() == devName) {
      // Bind Moniker to a filter object
      CHECK_ERROR_RETURN(enumerator.GetMoniker()->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_pCapture));
      PTRACE(4, "PVidDirectShow2010\tBound to device \""<< devName << '"');
      break;
    }
  }

  return true;
}

HRESULT PVideoInputDevice_DirectShow2010::SaveGraphToFile(TCHAR* sGraphFile)
{
	IStorage *          pStorage = NULL;
	IStream *           pStream = NULL;
	IPersistStream *    pPersistStream = NULL;
	HRESULT             hr = S_OK;

	if(m_pBuilder == NULL || sGraphFile == NULL)
		return E_FAIL;

	USES_CONVERSION;

	// Either Open or Create the *.GRF file
	hr = StgOpenStorage( T2W (sGraphFile), NULL, STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_DENY_WRITE, 
		NULL, NULL, &pStorage );
	if ( STG_E_FILENOTFOUND == hr )
		hr = StgCreateDocfile( T2W (sGraphFile), STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE , 
		NULL , &pStorage);
	if( FAILED(hr) ) return hr;

	hr = pStorage->CreateStream( L"ActiveMovieGraph", STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE, 
		NULL, NULL, &pStream );
	if( FAILED(hr) ) return hr;

	// Persist the stream, save & commit to disk
	hr = m_pBuilder->QueryInterface( IID_IPersistStream, (void **) &pPersistStream );
	if( FAILED(hr) ) return hr;

	hr = pPersistStream->Save(pStream, TRUE);
	if( FAILED(hr) ) return hr;

	hr = pStorage->Commit( STGC_DEFAULT );
	if( FAILED(hr) ) return hr;

	SAFE_RELEASE(pStorage);
	SAFE_RELEASE(pStream);
	SAFE_RELEASE(pPersistStream);

	return hr;
}
HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest)
{
    IPin *pOut = NULL;

    // Find an output pin on the first filter.
    HRESULT hr;// = FindUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
    if (SUCCEEDED(hr))
    {
        hr = ConnectFilters(pGraph, pSrc, pDest);
        pOut->Release();
    }
    return hr;
}
HRESULT PVideoInputDevice_DirectShow2010::PutResolutionFromDVDecoderPropertyPage( int displayPix)
{
	HRESULT hr;
	/* Obtain the dv docoder's IBaseFilter interface. */
	IEnumFilters *pEnum;
	hr = m_pGraph->EnumFilters(&pEnum);
	if( FAILED(hr) ) {
		PTRACE(5, "GetResolutionFromDVDecoderPropertyPage()::m_pGraph->EnumFilters failed. 0x"<< hex << hr <<dec);        
		return hr;
	}
	pEnum->Reset();
	ULONG cFetched;
	IBaseFilter *pFilter;
	while((hr = pEnum->Next(1, &pFilter, &cFetched))==S_OK)
	{
		FILTER_INFO info;
		hr = pFilter->QueryFilterInfo(&info);
		if( SUCCEEDED( hr) ) {
			PString str = info.achName;
			if( str.Find(L"DV Video Decoder") == 0 ) {
				IIPDVDec    *pIPDVDec = 0;
				hr = pFilter->QueryInterface(IID_IIPDVDec, reinterpret_cast<PVOID *>(&pIPDVDec));
				if( FAILED(hr) ) {
					PTRACE(5, "GetResolutionFromDVDecoderPropertyPage()::QI IIPDVDec failed. 0x"<< hex << hr <<dec);        
				}
				if( pIPDVDec ) {
					hr = pIPDVDec->put_IPDisplay(displayPix);
					if( FAILED(hr) ) {
						PTRACE(5, "GetResolutionFromDVDecoderPropertyPage()::pIPDVDec->put_IPDisplay failed. 0x"<< hex << hr <<dec);        
					} else {
						m_DVResolution = (_DVRESOLUTION)displayPix;
					}
					pIPDVDec->Release();
				}

			}
			if( info.pGraph ) info.pGraph->Release();
		}
		pFilter->Release();
	}
	pEnum->Release();
	return hr;
}


bool PVideoInputDevice_DirectShow2010::CreateGrabberHandler()
{
  // Create the Sample Grabber Filter.
  CHECK_ERROR_RETURN(CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pGrabberFilter));

  //Add the filter to the graph
  CHECK_ERROR_RETURN(m_pGraph->AddFilter(m_pGrabberFilter, L"Sample Grabber"));

  // Obtain interfaces for Sample Grabber
  CHECK_ERROR_RETURN(m_pGrabberFilter->QueryInterface(IID_ISampleGrabber, (void**)&m_pSampleGrabber));

  if (this->deviceName == D_CCVISION_HDCAPNAME){
   // Find the source's output pin and the renderer's input pin
    // Find the source's output pin and the renderer's input pin
    PComPtr<IPin> pCapturePinOut;
    
		  pCapturePinOut = GetOutPin(m_pCapture, 0);

  //ConnectFilters();
    //chenyuan begin
    PComPtr<IPin> pDeinterlacerIn; 
    CHECK_ERROR_RETURN(m_pDeinterlacer->FindPin(L"In", &pDeinterlacerIn));
  // SaveGraphToFile("c:\\2.grf");
    
    PComPtr<IPin> pDeinterlacerOut;
    CHECK_ERROR_RETURN(m_pDeinterlacer->FindPin(L"Out", &pDeinterlacerOut));
    PComPtr<IPin> pGrabberPinIn, pGrabberPinOut;
    pGrabberPinIn = GetInPin(m_pGrabberFilter,0);
  // CHECK_ERROR_RETURN(m_pGrabberFilter->FindPin(L"In", &pGrabberPinIn));
    CHECK_ERROR_RETURN(m_pGrabberFilter->FindPin(L"Out", &pGrabberPinOut));
    
  // CHECK_ERROR_RETURN(m_pGraph->Connect(pCapturePinOut, pDeinterlacerIn));
    //PComPtr<IPin> pDeinterlacerOut;
    //CHECK_ERROR_RETURN(m_pSampleGrabber->FindPin(L"Out", &pDeinterlacerOut));
    PComPtr<IPin> pAVIdecoderIn;
    CHECK_ERROR_RETURN(m_pAVI->FindPin(L"In", &pAVIdecoderIn));

    PComPtr<IPin> pAVIdecoderOut, pSmartTeeIn, pSmartTeeOut_0,pSmartTeeOut_1, pNULLRenderIn, pNULLRenderOut,pGrabBase;
    CHECK_ERROR_RETURN(m_pAVI->FindPin(L"Out", &pAVIdecoderOut));
    //CHECK_ERROR_RETURN(m_pSmartTee->FindPin(L"In", &pSmartTeeIn));
    //CHECK_ERROR_RETURN(m_pSmartTee->FindPin(L"Out", &pSmartTeeOut));
    pSmartTeeIn = GetInPin(m_pSmartTee,0);
    pSmartTeeOut_0 = GetOutPin(m_pSmartTee,0);
    pSmartTeeOut_1 = GetOutPin(m_pSmartTee,1);
    pNULLRenderIn = GetInPin(m_pNullRender,0);

   
  // pNULLRenderOut = GetOutPin(m_pNullRender,0);
    //CHECK_ERROR_RETURN(m_pNullRender->FindPin(L"In", &pNULLRenderIn));
  // CHECK_ERROR_RETURN(m_pNullRender->FindPin(L"Out", &pNULLRenderOut));

    CHECK_ERROR_RETURN(m_pGraph->Connect(pCapturePinOut, pDeinterlacerIn));
    //pAVIdecoderOut->Release();
    //CHECK_ERROR_RETURN(m_pGraph->Connect(pDeinterlacerOut, pAVIdecoderIn));
    
    //CHECK_ERROR_RETURN(m_pGraph->Connect(pAVIdecoderOut, pGrabberPinIn));

    
    CHECK_ERROR_RETURN(m_pGraph->Connect(pDeinterlacerOut,pSmartTeeIn ));
    //SaveGraphToFile("d:\\1_.grf");
    HRESULT h=m_pGraph->Render(pSmartTeeOut_0);
	  PutResolutionFromDVDecoderPropertyPage(8);
  	
	  //HRESULT hr = S_OK;// m_pSampleGrabber->SetMediaType( &VideoType ); // shouldn't fail
	  // 
  // h=(m_pGraph->Connect(pSmartTeeOut_0,pGrabberPinIn ));
  // CHECK_ERROR_RETURN(m_pGraph->Connect(pGrabberPinOut,pNULLRenderIn ));
      
   
    //(m_pGraph->Connect(pDeinterlacerOut, pGrabberPinIn));
    //SaveGraphToFile("c:\\22.grf");
    CHECK_ERROR_RETURN(m_pSampleGrabber->SetBufferSamples(true));
    CHECK_ERROR_RETURN(m_pSampleGrabber->SetOneShot(false));
 //HRESULT h=(m_pGraph->Render(pGrabberPinIn));
 //CHECK_ERROR_RETURN(m_pBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pCapture, m_pAVI, m_pGrabberFilter));
  // h= m_pBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pCapture, NULL, NULL);
  }else{
#ifdef D_PIN_CAPTURE
 // Find the source's output pin and the renderer's input pin
    // Find the source's output pin and the renderer's input pin
    PComPtr<IPin> pCapturePinOut;

    PComPtr<IPin>  pMJPEGFilerIn; 
    CHECK_ERROR_RETURN(m_pMJPEGFiler->FindPin(L"In", &pMJPEGFilerIn));
  // SaveGraphToFile("c:\\2.grf");
    
    PComPtr<IPin> pMJPEGFilerOut;
    CHECK_ERROR_RETURN(m_pMJPEGFiler->FindPin(L"Out", &pMJPEGFilerOut));
    PComPtr<IPin>  pcolorIn; 
    CHECK_ERROR_RETURN(m_pColorFiler->FindPin(L"In", &pcolorIn));
  // SaveGraphToFile("c:\\2.grf");
    
    PComPtr<IPin> pcolorOut;
    CHECK_ERROR_RETURN(m_pColorFiler->FindPin(L"Out", &pcolorOut));
    
		pCapturePinOut = GetOutPin(m_pCapture, 0);
 
    PComPtr<IPin> pGrabberPinIn, pGrabberPinOut;
    pGrabberPinIn = GetInPin(m_pGrabberFilter,0);
  // CHECK_ERROR_RETURN(m_pGrabberFilter->FindPin(L"In", &pGrabberPinIn));
  
    PComPtr<IPin> pAVIdecoderIn;
    CHECK_ERROR_RETURN(m_pAVI->FindPin(L"In", &pAVIdecoderIn));

    PComPtr<IPin> pAVIdecoderOut, pSmartTeeIn, pSmartTeeOut_0,pSmartTeeOut_1, pNULLRenderIn, pNULLRenderOut,pGrabBase;
    CHECK_ERROR_RETURN(m_pAVI->FindPin(L"Out", &pAVIdecoderOut));
    //CHECK_ERROR_RETURN(m_pSmartTee->FindPin(L"In", &pSmartTeeIn));
    //CHECK_ERROR_RETURN(m_pSmartTee->FindPin(L"Out", &pSmartTeeOut));
    pSmartTeeIn = GetInPin(m_pSmartTee,0);
    pSmartTeeOut_0 = GetOutPin(m_pSmartTee,0);
    pSmartTeeOut_1 = GetOutPin(m_pSmartTee,1);
    pNULLRenderIn = GetInPin(m_pNullRender,0);

    
 
    // CHECK_ERROR_RETURN(m_pGraph->Connect(pCapturePinOut, pDeinterlacerIn));
#if 1
    CHECK_ERROR_RETURN(m_pGraph->Connect(pCapturePinOut, pMJPEGFilerIn)); 
    CHECK_ERROR_RETURN(m_pGraph->Connect(pMJPEGFilerOut,pcolorIn ));
    CHECK_ERROR_RETURN(m_pGraph->Connect(pcolorOut,pSmartTeeIn ));
#else

     CHECK_ERROR_RETURN(m_pGraph->Connect(pCapturePinOut,pSmartTeeIn ));
    //CHECK_ERROR_RETURN(m_pGraph->Connect(pDeinterlacerOut,pSmartTeeIn ));
#endif
    //SaveGraphToFile("d:\\1_.grf");
    HRESULT h=m_pGraph->Render(pSmartTeeOut_0);
	  //PutResolutionFromDVDecoderPropertyPage(8);
  	
	 
    SaveGraphToFile("d:\\22.grf");
    CHECK_ERROR_RETURN(m_pSampleGrabber->SetBufferSamples(TRUE));
    CHECK_ERROR_RETURN(m_pSampleGrabber->SetOneShot(FALSE));
 //HRESULT h=(m_pGraph->Render(pGrabberPinIn));
 //CHECK_ERROR_RETURN(m_pBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pCapture, m_pAVI, m_pGrabberFilter));
  // h= m_pBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pCapture, NULL, NULL);
#else
    CHECK_ERROR_RETURN(m_pSampleGrabber->SetBufferSamples(true));
    CHECK_ERROR_RETURN(m_pSampleGrabber->SetOneShot(false));
     CHECK_ERROR_RETURN(m_pBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pCapture, m_pMJPEGFiler, m_pGrabberFilter));
  // CHECK_ERROR_RETURN(m_pBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pCapture, NULL, m_pGrabberFilter));
#endif
  }
   return true;
}

#endif // _WIN32_WCE


#endif /*P_DIRECTSHOW*/
