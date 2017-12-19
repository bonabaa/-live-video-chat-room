/* vidinput_directx2.cxx
 *
 * DirectShow2 Implementation for the H323Plus/Opal Project.
 *
 * Copyright (c) 2009 ISVO (Asia) Pte Ltd. All Rights Reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the General Public License (the  "GNU License"), in which case the
 * provisions of GNU License are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GNU License and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GNU License. If you do not delete
 * the provisions above, a recipient may use your version of this file
 * under either the MPL or the GNU License."
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * Initial work sponsored by Requestec Ltd.
 *
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: directshow.cxx,v $
 *
 *
 */


#include <ptlib.h>

#ifdef P_DIRECTSHOW2

#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptlib/msos/ptlib/vidinput_directx2.h>

#include <atlbase.h>
#include <windows.h>
#include <dshow.h>
#include <initguid.h>

#ifndef _WIN32_WCE

// Use this to avoid compile error in Qedit.h with DirectX SDK
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__

#pragma message("dxtrans.h is not required ignore warning. If compile error comment out #define dxtrans.h in Qedit.h in PSDK")
#include <Qedit.h>
#pragma warning(disable:4201)

#include <Ks.h>
#include <KsMedia.h>
#pragma warning(default:4201)
#endif

#ifdef _MSC_VER
#pragma comment(lib, P_DIRECTSHOW_LIBRARY1)
#ifndef _WIN32_WCE
#pragma comment(lib, P_DIRECTSHOW_LIBRARY2)
#endif
#endif

#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }

#ifdef _WIN32_WCE

#ifndef HAVE_CE_SAMPLEGRABBER
DEFINE_GUID(IID_ISampleGrabber, 0x6B652FFF, 0x11FE, 0x4fce, 0x92, 0xAD,0x02, 0x66, 0xB5, 0xD7, 0xC7, 0x8F);
DEFINE_GUID(CLSID_SampleGrabber, 0xC1F400A4, 0x3F08, 0x11d3, 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37);
DEFINE_GUID(IID_ISampleGrabberCB, 0x0579154A, 0x2B53, 0x4994, 0xB0, 0xD0, 0xE7, 0x73, 0x14, 0x8E, 0xFF, 0x85);
DEFINE_GUID(CLSID_NullRenderer, 0xC1F400A4, 0x3F08, 0x11d3, 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 );
DEFINE_GUID(CLSID_CameraName, 0xCB998A05, 0x122C, 0x4166, 0x84, 0x6A, 0x93, 0x3E, 0x4D, 0x7E, 0x3C, 0x86 );
#endif

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
#endif

//////////////////////////////////////////////////////////////////////////////////////

class PVideoInputDevice_DirectShow2_PluginServiceDescriptor : public PDevicePluginServiceDescriptor
{
  public:
    virtual PObject *   CreateInstance(int /*userData*/) const { return new PVideoInputDevice_DirectShow2; }
    virtual PStringArray GetDeviceNames(int /*userData*/) const { return PVideoInputDevice_DirectShow2::GetInputDeviceNames(); }
    virtual bool GetDeviceCapabilities(const PString & deviceName, void * caps) const
	        { return PVideoInputDevice_DirectShow2::GetDeviceCapabilities(deviceName,(PVideoInputDevice::Capabilities *)caps); }

} PVideoInputDevice_DirectShow2_descriptor;

PCREATE_PLUGIN(DirectShow2, PVideoInputDevice, &PVideoInputDevice_DirectShow2_descriptor);

////////////////////////////////////////////////////////////////////

PString FailedMSG(HRESULT hr) 
{ 
#ifndef _WIN32_WCE
	TCHAR error[1024]; 
	AMGetErrorText(hr,error,1024);
	return error;
#else
	return "Not Available";
#endif
}

HRESULT GetUnconnectedPin(
    IBaseFilter *pFilter,   // Pointer to the filter.
    PIN_DIRECTION PinDir,   // Direction of the pin to find.
    IPin **ppPin)           // Receives a pointer to the pin.
{
    *ppPin = 0;
    IEnumPins *pEnum = 0;
    IPin *pPin = 0;
    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        return hr;
    }
    while (pEnum->Next(1, &pPin, NULL) == S_OK)
    {
        PIN_DIRECTION ThisPinDir;
        pPin->QueryDirection(&ThisPinDir);
        if (ThisPinDir == PinDir)
        {
            IPin *pTmp = 0;
            hr = pPin->ConnectedTo(&pTmp);
            if (SUCCEEDED(hr))  // Already connected, not the pin we want.
            {
                pTmp->Release();
            }
            else  // Unconnected, this is the pin we want.
            {
                pEnum->Release();
                *ppPin = pPin;
                return S_OK;
            }
        }
        pPin->Release();
    }
    pEnum->Release();
    // Did not find a matching pin.
    return E_FAIL;
}

HRESULT ConnectFilters(
    IGraphBuilder *pGraph, // Filter Graph Manager.
    IPin *pOut,            // Output pin on the upstream filter.
    IBaseFilter *pDest)    // Downstream filter.
{
    if ((pGraph == NULL) || (pOut == NULL) || (pDest == NULL))
    {
        return E_POINTER;
    }

    // Find an input pin on the downstream filter.
    IPin *pIn = 0;
    HRESULT hr = GetUnconnectedPin(pDest, PINDIR_INPUT, &pIn);
    if (FAILED(hr))
    {
        return hr;
    }
    // Try to connect them.
    hr = pGraph->Connect(pOut, pIn);
    pIn->Release();
    return hr;
}

HRESULT ConnectFilters(
    IGraphBuilder *pGraph, 
    IBaseFilter *pSrc, 
    IPin *pIn)
{
    if ((pGraph == NULL) || (pSrc == NULL) || (pIn == NULL))
    {
        return E_POINTER;
    }

    // Find an output pin on the first filter.
    IPin *pOut = 0;
    HRESULT hr = GetUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
    if (FAILED(hr)) 
    {
        return hr;
    }
    hr = pGraph->Connect(pOut, pIn);
    pOut->Release();
    return hr;
}

HRESULT ConnectFilters(
    IGraphBuilder *pGraph, 
    IBaseFilter *pSrc, 
    IBaseFilter *pDest)
{
    if ((pGraph == NULL) || (pSrc == NULL) || (pDest == NULL))
    {
        return E_POINTER;
    }

    // Find an output pin on the first filter.
    IPin *pOut = 0;
    HRESULT hr = GetUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
    if (FAILED(hr)) 
    {
        return hr;
    }
    hr = ConnectFilters(pGraph, pOut, pDest);
    pOut->Release();
    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
// Video Input Functions

PString SetName(const PString & name, const PStringList & names)
{
	
	PStringStream DisplayName;
	
	DisplayName << name;
	int j = 0;
	for (int i=0; i < names.GetSize(); i++) {
		if (names[i].Left(name.GetLength()) == name)
			   j++;
	}

   if (j > 0)
	  DisplayName << " {" << j << "}";

   return DisplayName;
}

PString ParseName(const PString & name, PINDEX & devNo)
{
	
	if (strspn(name, "{") < strlen(name)) {
		PINDEX start = name.Find("{");
	    PINDEX end = name.Find("}");

		devNo = name.Mid(start+1,end - start-1).AsInteger();
		return name.Left(start).Trim();
	} else {
	  devNo = 0;
	  return name;
	}
}


PBoolean BindFilter(const PString & DeviceName, IBaseFilter **pFilter)
{
	if (DeviceName.GetLength() == 0) {
		PTRACE(6,"DShow\tUnable to Bind to empty device");
		return FALSE;
	}

	PINDEX devNo = 0;
	PString name = ParseName(DeviceName,devNo);

#ifndef _WIN32_WCE
    // enumerate all video capture devices
	CComPtr<ICreateDevEnum> pCreateDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
			  IID_ICreateDevEnum, (void**)&pCreateDevEnum);

	if (FAILED(hr)) {
		PTRACE(6,"DShow\tUnable to Enumerate system devices");
		return FALSE;
	}

    CComPtr<IEnumMoniker> pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
								&pEm, 0);
	if (FAILED(hr)) {
		PTRACE(6,"DShow\tUnable to Enumerate Video Input devices");
		return FALSE;
	}

    pEm->Reset();
    ULONG cFetched;
    IMoniker *pM;
	int index = 0;
	bool found = false;
    while(hr = pEm->Next(1, &pM, &cFetched), hr==S_OK, !found)
    {
		IPropertyBag *pBag;
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if(SUCCEEDED(hr)) 
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName", &var, NULL);
			if (SUCCEEDED(hr)) 
			{
			   TCHAR str[2048];		
			   WideCharToMultiByte(CP_ACP,0,var.bstrVal, -1, str, 2048, NULL, NULL);
				
			   if (PString(var.bstrVal).Trim() == name) {
				if (index == devNo) {
					pM->BindToObject(0, 0, IID_IBaseFilter, (void**)pFilter);
					found=true;
				}
				SysFreeString(var.bstrVal);
				index++;
			   }
			}
			SAFE_RELEASE(pBag);
		}
		SAFE_RELEASE(pM);
    } 
#else

#endif

	PTRACE(6,"DShow\tDevice " << DeviceName << " bound to Filter");
	return TRUE;
}

// Check to see the orientation of the captured image

void GetVideoInfoParameters(
    const VIDEOINFOHEADER *pvih, // Pointer to the format header.
    BYTE  * const pbData,   // Pointer to the first address in the buffer.
    DWORD *pdwWidth,        // Returns the width in pixels.
    DWORD *pdwHeight,       // Returns the height in pixels.
    LONG  *plStrideInBytes, // Add this to a row to get the new row down.
    BYTE **ppbTop,          // Returns a pointer to the first byte in the top row of pixels.
    bool bYuv               // Is this a YUV format? (true = YUV, false = RGB)
    )
{
    LONG lStride;

    //  For 'normal' formats, biWidth is in pixels. 
    //  Expand to bytes and round up to a multiple of 4.
    if ((pvih->bmiHeader.biBitCount != 0) &&
        (0 == (7 & pvih->bmiHeader.biBitCount))) 
    {
        lStride = (pvih->bmiHeader.biWidth * (pvih->bmiHeader.biBitCount / 8) + 3) & ~3;
    } 
    else   // Otherwise, biWidth is in bytes.
    {
        lStride = pvih->bmiHeader.biWidth;
    }

    //  If rcTarget is empty, use the whole image.
    if (IsRectEmpty(&pvih->rcTarget)) 
    {
        *pdwWidth = (DWORD)pvih->bmiHeader.biWidth;
        *pdwHeight = (DWORD)(abs(pvih->bmiHeader.biHeight));
        
        if (pvih->bmiHeader.biHeight < 0 || bYuv)   // Top-down bitmap. 
        {
            *plStrideInBytes = lStride; // Stride goes "down".
            *ppbTop           = pbData; // Top row is first.
        } 
        else        // Bottom-up bitmap.
        {
            *plStrideInBytes = -lStride;    // Stride goes "up".
            // Bottom row is first.
            *ppbTop = pbData + lStride * (*pdwHeight - 1);  
        }
    } 
    else   // rcTarget is NOT empty. Use a sub-rectangle in the image.
    {
        *pdwWidth = (DWORD)(pvih->rcTarget.right - pvih->rcTarget.left);
        *pdwHeight = (DWORD)(pvih->rcTarget.bottom - pvih->rcTarget.top);
        
        if (pvih->bmiHeader.biHeight < 0 || bYuv)   // Top-down bitmap.
        {
            // Same stride as above, but first pixel is modified down
            // and over by the target rectangle.
            *plStrideInBytes = lStride;     
            *ppbTop = pbData +
                     lStride * pvih->rcTarget.top +
                     (pvih->bmiHeader.biBitCount * pvih->rcTarget.left) / 8;
        } 
        else  // Bottom-up bitmap.
        {
            *plStrideInBytes = -lStride;
            *ppbTop = pbData +
                     lStride * (pvih->bmiHeader.biHeight - pvih->rcTarget.top - 1) +
                     (pvih->bmiHeader.biBitCount * pvih->rcTarget.left) / 8;
        }
    }
}

bool CheckOrientation(BYTE *pData, VIDEOINFOHEADER *pVih)
{
    DWORD dwWidth, dwHeight;
    long lStride;
    BYTE *pbTop;
    GetVideoInfoParameters(pVih, pData, &dwWidth, 
        &dwHeight, &lStride, &pbTop, false);

    if (lStride < 0)    /// Must always be positive stride ?
		return FALSE;
	else
		return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// Video sample Grabber Callback

// Implementation of CSampleGrabberCB object
static int skipFrames = 5;
class CSampleGrabberCB : public ISampleGrabberCB 
{

public:

	CSampleGrabberCB()
	{
	  bufferSize = 0;
	  pBuffer = NULL; 
	  cInd = 0;
	}

	~CSampleGrabberCB()
	{
	   bufferSize = 0;
	   if (pBuffer != NULL)
		   delete pBuffer;
	   frameready.Signal();
	}

    // Fake out any COM ref counting
    //
    STDMETHODIMP_(ULONG) AddRef() { return 2; }
    STDMETHODIMP_(ULONG) Release() { return 1; }

    // Fake out any COM QI'ing
    //
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
    {
        
        if( riid == IID_ISampleGrabberCB || riid == IID_IUnknown ) 
        {
            *ppv = (void *) static_cast<ISampleGrabberCB*> ( this );
            return NOERROR;
        }    

        return E_NOINTERFACE;
    }


    // We don't implement this one
    //
    STDMETHODIMP SampleCB( double SampleTime, IMediaSample * pSample )
    {
        return 0;
    }

    // The sample grabber is calling us back on its deliver thread.
    // This is NOT the main app thread!
    //
	STDMETHODIMP BufferCB( double dblSampleTime, BYTE * buffer, long size )
    {

       PTRACE(6,"DShow\tBuffer Received " << size);

	    if (cInd < skipFrames) {
		 cInd++;
		 PTRACE(5,"DShow\tFrame Skipped.");
		 return S_OK;
		}

		PWaitAndSignal m(mbuf);

		if (size == 0)
			return S_OK;

		if (bufferSize != size) {
			PTRACE(5,"DShow\tResizing buffer from " << bufferSize << " to " << size );
			bufferSize = size;
			if (pBuffer != NULL)
				delete pBuffer;
			pBuffer = new BYTE[bufferSize];
			memset(pBuffer,0,bufferSize);
		}

        if (!buffer)
            return E_POINTER;

		memcpy(pBuffer,buffer, bufferSize);

		frameready.Signal();
		
		return S_OK;
	}

	bool GetCurrentBuffer(BYTE & buffer, const long & expectedSize) 
	{
		frameready.Wait(2000);

		PWaitAndSignal m(mbuf);

		if (bufferSize == 0) { 
			PTRACE(4,"DShow\tGRAB FAILURE! Current Buffer Empty");
			return FALSE;
		}

		if (bufferSize != expectedSize) {
			PTRACE(4,"DShow\tError: Current Buffer wrong size " << bufferSize << " expecting " << expectedSize);
			return FALSE;
		}

	     memcpy(&buffer, pBuffer, bufferSize);
	   return TRUE;  
	}

protected:

	long bufferSize;
	BYTE * pBuffer;

	PMutex mbuf;
	PSyncPoint frameready;
	PINDEX cInd;

};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Input device

PVideoInputDevice_DirectShow2::PVideoInputDevice_DirectShow2()
{
 PTRACE(4,"DShow\tVideo Device Instance");

  CoInitializeEx(NULL,COINIT_MULTITHREADED);

	m_pGB = NULL;	
	m_pCap = NULL;
	m_pDF = NULL;
	m_pMC = NULL;
	m_pGrabber = NULL; 
	m_pGrabberCB = NULL;
	m_pNull = NULL;
	m_pCamOutPin = NULL;  
	m_pCC = NULL;
     
	colorGUID = MEDIASUBTYPE_NULL;
	m_psCurrent=Stopped;

	checkVFlip = FALSE;

    preferredColourFormat = "BGR24";  
//	preferredColourFormat = "YUV420P";
	frameRate = 25;
	pBufferSize = 0;
	pBuffer = NULL;

}

PVideoInputDevice_DirectShow2::~PVideoInputDevice_DirectShow2()
{
	Close();
}

PStringArray PVideoInputDevice_DirectShow2::GetInputDeviceNames()
{
   
 PTRACE(5,"DShow\tGetting Input Device Names");

 PStringArray Names;
#ifndef _WIN32_WCE
    // enumerate all video capture devices
	CComPtr<ICreateDevEnum> pCreateDevEnum;

  CoInitializeEx(NULL,COINIT_MULTITHREADED);

    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
			  IID_ICreateDevEnum, (void**)&pCreateDevEnum);
	
	if (FAILED(hr)) {
		PTRACE(6,"DShow\tUnable to Enumerate system devices");
		return Names;
	}

    CComPtr<IEnumMoniker> pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
								&pEm, 0);
	if (hr != NOERROR) {
		PTRACE(6,"DShow\tUnable to Enumerate Video Input devices");
		return Names;
	}

    pEm->Reset();
    ULONG cFetched;
    IMoniker *pM;
    while(hr = pEm->Next(1, &pM, &cFetched), hr==S_OK)
    {
		IPropertyBag *pBag;
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if(SUCCEEDED(hr)) 
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName", &var, NULL);
			if (SUCCEEDED(hr)) 
			{
				TCHAR str[2048];		
				
				WideCharToMultiByte(CP_ACP,0,var.bstrVal, -1, str, 2048, NULL, NULL);
				Names.AppendString(SetName(var.bstrVal,Names));
				SysFreeString(var.bstrVal);
			}
			SAFE_RELEASE(pBag);
		}
		SAFE_RELEASE(pM);
    }
#else
	HANDLE	handle = NULL;
	char szDeviceName[8];
	ZeroMemory( szDeviceName, 8 );

	DEVMGR_DEVICE_INFORMATION di;
	di.dwSize = sizeof(di);

	handle = FindFirstDevice( DeviceSearchByGuid, &CLSID_CameraName, &di );
	if (handle != NULL ) {
		do{
		  wcstombs( szDeviceName, di.szLegacyName, 8 );
		  Names.AppendString(szDeviceName);
		} while( FindNextDevice( handle, &di ) );
      FindClose( handle );
	}

#endif

    PTRACE(6,"DShow\tDevices enumerated");
	return Names;
}

struct {
    char *pwlib_format;
    GUID  media_format;
} formats[] =
{
    {(char*) "BGR32",   MEDIASUBTYPE_RGB32}, 
    {(char*) "BGR24",   MEDIASUBTYPE_RGB24}
#ifndef _WIN32_WCE
    ,{(char*) "YUV420P", MEDIASUBTYPE_IYUV}, 
    {(char*) "YUV420",  MEDIASUBTYPE_YUY2}
#endif
}; 

static const char *media_format_to_pwlib_format(const GUID guid)
{
    unsigned int i;
	const char* unknown = "Unknown";

    for (i=0; i<sizeof(formats)/sizeof(formats[0]); i++)
    {
	if (guid == formats[i].media_format)
	    return formats[i].pwlib_format;
    }
#ifndef _WIN32_WCE
    if (guid == MEDIASUBTYPE_CLPL)
	return "CLPL";
    else if (guid == MEDIASUBTYPE_YUYV)
	return "YUYV";
    else if (guid == MEDIASUBTYPE_IYUV)
	return "IYUV";
    else if (guid == MEDIASUBTYPE_YVU9)
	return "YVU9";
	else 
#endif
    if (guid == MEDIASUBTYPE_Y411)
	return "Y411";
    else if (guid == MEDIASUBTYPE_Y41P)
	return "Y41P";
    else if (guid == MEDIASUBTYPE_YUY2)
	return "YUY2";
    else if (guid == MEDIASUBTYPE_YVYU)
	return "YVYU";
    else if (guid == MEDIASUBTYPE_UYVY)
	return "UYVY";
    else if (guid == MEDIASUBTYPE_Y211)
	return "Y211";
    else if (guid == MEDIASUBTYPE_YV12)
	return "YV12";
    else if (guid == MEDIASUBTYPE_CLJR)
	return "CLJR";
    else if (guid == MEDIASUBTYPE_IF09)
	return "IF09";
    else if (guid == MEDIASUBTYPE_CPLA)
	return "CPLA";
    else if (guid == MEDIASUBTYPE_MJPG)
	return "MJPG";
    else if (guid == MEDIASUBTYPE_TVMJ)
	return "TVMJ";
    else if (guid == MEDIASUBTYPE_WAKE)
	return "WAKE";
    else if (guid == MEDIASUBTYPE_CFCC)
	return "CFCC";
    else if (guid == MEDIASUBTYPE_IJPG)
	return "IJPG";
    else if (guid == MEDIASUBTYPE_Plum)
	return "Plum";
    else if (guid == MEDIASUBTYPE_DVCS)
	return "DVCS";
    else if (guid == MEDIASUBTYPE_DVSD)
	return "DVSD";
    else if (guid == MEDIASUBTYPE_MDVF)
	return "MDVF";
    else if (guid == MEDIASUBTYPE_RGB1)
	return "RGB1";
    else if (guid == MEDIASUBTYPE_RGB4)
	return "RGB4";
    else if (guid == MEDIASUBTYPE_RGB8)
	return "RGB8";
    else if (guid == MEDIASUBTYPE_RGB565)
	return "RGB565";
    else if (guid == MEDIASUBTYPE_RGB555)
	return "RGB555";
    else if (guid == MEDIASUBTYPE_RGB24)
	return "BGR24";
    else if (guid == MEDIASUBTYPE_RGB32)
	return "BGR32";
#ifndef _WIN32_WCE
    else if (guid == MEDIASUBTYPE_IYUV)
	return "I420";
#endif
    else
	return unknown; 
}



bool PVideoInputDevice_DirectShow2::GetDeviceCapabilities(const PString & deviceName, PVideoInputDevice::Capabilities * caps)  
{ 
 
	IGraphBuilder           * t_pGB;
#ifndef _WIN32_WCE
    ICaptureGraphBuilder2   * t_pCap;
#else
    ICaptureGraphBuilder    * t_pCap;
#endif
    IBaseFilter             * t_pDF;
	IPin                    * t_pCamOutPin;

	IAMCameraControl        * t_pCC;
	PVideoControlInfo panInfo;
	PVideoControlInfo tiltInfo;
	PVideoControlInfo zoomInfo;

	bool success = FALSE;

// Get the interface for DirectShow's GraphBuilder
    HRESULT  hr=CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, 
                         IID_IGraphBuilder, (void **)&t_pGB);

    if(FAILED(hr)) {
		PTRACE(6,"DShow\tCannot initialise DirectShow GraphBuilder");
		SAFE_RELEASE(t_pGB);
		return FALSE;
	}

// Bind the WebCam Input
#ifndef _WIN32_WCE
	hr = CoCreateInstance (CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC_SERVER,
                           IID_ICaptureGraphBuilder2, (void **) &t_pCap);
#else
	hr = CoCreateInstance (CLSID_CaptureGraphBuilder , NULL, CLSCTX_INPROC_SERVER,
                           IID_ICaptureGraphBuilder2, (void **) &t_pCap);
#endif
		if (FAILED(hr)) {
		    PTRACE(6,"DShow\tCannot initialise DirectShow capture graph builder");
		      SAFE_RELEASE(t_pCap);
		      SAFE_RELEASE(t_pGB);
			return FALSE;
		}

	    t_pCap->SetFiltergraph(t_pGB);

// Bind Device Filter.  We know the device because the id was passed in
		if(!BindFilter(deviceName, &t_pDF)) {
			PTRACE(4,"DShow\tCannot bind webcam " << deviceName << " to filter");
		      SAFE_RELEASE(t_pDF);
		      SAFE_RELEASE(t_pCap);
		      SAFE_RELEASE(t_pGB);
			return FALSE;
		}

/// Check if Video Decoder
		IAMAnalogVideoDecoder *t_pVD;
		hr = t_pDF->QueryInterface(IID_IAMAnalogVideoDecoder, (void **)&t_pVD);
	    if (SUCCEEDED(hr)) {
			PTRACE(6,"DShow\t" << deviceName << " is a video decoding device.");
			  SAFE_RELEASE(t_pVD);
		      SAFE_RELEASE(t_pDF);
		      SAFE_RELEASE(t_pCap);
		      SAFE_RELEASE(t_pGB);
			return FALSE;
		}

		hr=t_pGB->AddFilter(t_pDF, L"Video Capture");
		if (FAILED(hr)) {
			PTRACE(4,"DShow\tCannot add video capture filter "  << deviceName );
		      SAFE_RELEASE(t_pDF);
		      SAFE_RELEASE(t_pCap);
		      SAFE_RELEASE(t_pGB);
		    return FALSE;
		}

// Get the Webcam output Pin 
		CComPtr<IEnumPins> pEnum;
		t_pDF->EnumPins(&pEnum);

		if (FAILED(hr)) {
		    PTRACE(4,"DShow\tCould not enumerate " << deviceName << " Output pins " << FailedMSG(hr));
		      SAFE_RELEASE(t_pDF);
		      SAFE_RELEASE(t_pCap);
		      SAFE_RELEASE(t_pGB);
			return FALSE;
		}

		hr = pEnum->Reset();
		hr = pEnum->Next(1, &t_pCamOutPin, NULL); 

		if (FAILED(hr)) {
		    PTRACE(4,"DShow\tUnable to find WebCam "  << deviceName << " Output pin " << FailedMSG(hr));
		      SAFE_RELEASE(t_pCamOutPin);
		      SAFE_RELEASE(t_pDF);
		      SAFE_RELEASE(t_pCap);
		      SAFE_RELEASE(t_pGB);
			return FALSE;
		}


// Query for camera controls
		hr = t_pDF->QueryInterface(IID_IAMCameraControl, (void **)&t_pCC);
	    if (FAILED(hr)) {
			PTRACE(6,"DShow\tWebCam " << deviceName << " does not support Camera Controls.");
			goto endcontroltest;
		} 
	// Retrieve information about the pan and tilt controls
		panInfo.type= PVideoControlInfo::ControlPan;
	hr = t_pCC->GetRange(CameraControl_Pan, &panInfo.min, &panInfo.max, &panInfo.step, &panInfo.def, &panInfo.flags);
	if (FAILED(hr)) {
			PTRACE(6,"DShow\tWebCam " << deviceName << " does not support Pan.");
	//		goto endcontroltest;
	} else {
		caps->controls.push_back(panInfo);
	}

	tiltInfo.type= PVideoControlInfo::ControlTilt;
	hr = t_pCC->GetRange(CameraControl_Tilt, &tiltInfo.min, &tiltInfo.max, &tiltInfo.step, &tiltInfo.def, &tiltInfo.flags);
	if (FAILED(hr)) {
			PTRACE(6,"DShow\tWebCam " << deviceName << " does not support Tilt.");
	//		goto endcontroltest;
	} else {
		caps->controls.push_back(tiltInfo);
	}

	zoomInfo.type= PVideoControlInfo::ControlZoom;
	hr = t_pCC->GetRange(CameraControl_Zoom, &zoomInfo.min, &zoomInfo.max, &zoomInfo.step, &zoomInfo.def, &zoomInfo.flags);
	if (FAILED(hr)) {
			PTRACE(6,"DShow\tWebCam " << deviceName << " does not support zoom.");
	//		goto endcontroltest;
	} else {
		caps->controls.push_back(zoomInfo);
	}


endcontroltest:

// Now we have the Output pin we can enumerate the capabilities
// WebCam
     IAMStreamConfig *pConfig = NULL;
     hr = t_pCamOutPin->QueryInterface(&pConfig);

	 if (FAILED(hr)) {
  		 SAFE_RELEASE(pConfig);
 		 SAFE_RELEASE(t_pCamOutPin);
		 SAFE_RELEASE(t_pCC);
		 SAFE_RELEASE(t_pDF);
		 SAFE_RELEASE(t_pCap);
		 SAFE_RELEASE(t_pGB);
		PTRACE(4,"DShow\tCannot Query webcam Configuration " << deviceName );
		return FALSE;
	 }

	int iCount = 0, iSize=0;
	hr = pConfig->GetNumberOfCapabilities(&iCount, &iSize);

	 if (FAILED(hr)) {
		PTRACE(6,"DShow\tCannot retrieve number of webcam capabilities " << deviceName );
  		 SAFE_RELEASE(pConfig);
 		 SAFE_RELEASE(t_pCamOutPin);
		 SAFE_RELEASE(t_pCC);
		 SAFE_RELEASE(t_pDF);
		 SAFE_RELEASE(t_pCap);
		 SAFE_RELEASE(t_pGB);
		return FALSE;
	 }

	if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS)) {
	  for (int iFormat = 0; iFormat < iCount; iFormat++)
	  {
		 VIDEO_STREAM_CONFIG_CAPS scc;
	         AM_MEDIA_TYPE *mt;
		 hr= pConfig->GetStreamCaps(iFormat,&mt,reinterpret_cast<BYTE*>(&scc));
		 if (SUCCEEDED(hr)) {
			   if ((mt->majortype == MEDIATYPE_Video) &&        
				((mt->subtype == MEDIASUBTYPE_RGB24) ||   //RGB24
				(mt->subtype == MEDIASUBTYPE_RGB32)     //RGB32
#ifndef _WIN32_WCE
				|| (mt->subtype == MEDIASUBTYPE_YUY2) ||     // YUV420
				(mt->subtype == MEDIASUBTYPE_IYUV)    // YUV420P 
#endif
				) &&
				(mt->cbFormat >= sizeof(VIDEOINFOHEADER)) &
				(mt->pbFormat != NULL)) {
                  // Valid Capability
					PVideoFrameInfo cap;
				      cap.SetFrameSize(scc.MaxOutputSize.cx,scc.MaxOutputSize.cy);
					  cap.SetColourFormat(media_format_to_pwlib_format(mt->subtype));
					  cap.SetFrameRate(10000000/(unsigned)scc.MinFrameInterval);
				   caps->framesizes.push_back(cap);
                   success = TRUE;
				}
		 }
	  }
	}

	 SAFE_RELEASE(t_pCC);
	 SAFE_RELEASE(t_pCamOutPin);
	 SAFE_RELEASE(t_pDF);
	 SAFE_RELEASE(t_pCap);
	 SAFE_RELEASE(t_pGB);
    return success; 
}

PBoolean PVideoInputDevice_DirectShow2::SetVideoFormat(IPin * pin) {

    PTRACE(6,"DShow\tSetting WebCam OutPut Video Format");

   if (pin == NULL) {
	   PTRACE(4,"DShow\tWebCam OutPut Pin is NULL!");
	   return FALSE;
   }

/// Check to see if WebCam
   IAMStreamConfig *pConfig = NULL;
   HRESULT hr;
   bool success = FALSE;

     hr = pin->QueryInterface(&pConfig);

	 if (FAILED(hr)) {
		PTRACE(4,"DShow\tCannot Query webcam Configuration");
		return FALSE;
	 }

	int iCount = 0, iSize=0;
	hr = pConfig->GetNumberOfCapabilities(&iCount, &iSize);

	 if (FAILED(hr)) {
		PTRACE(4,"DShow\tCannot retrieve number of webcam capabilities ");
		SAFE_RELEASE(pConfig);
		return FALSE;
	 }


	if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS)) {
	  for (int iFormat = 0; iFormat < iCount; iFormat++)
	  {
		 VIDEO_STREAM_CONFIG_CAPS scc;
	         AM_MEDIA_TYPE *mt;
		 hr= pConfig->GetStreamCaps(iFormat,&mt,reinterpret_cast<BYTE*>(&scc));
		 if (SUCCEEDED(hr)) {

			   if ((mt->majortype == MEDIATYPE_Video) &&        
				((mt->subtype == MEDIASUBTYPE_RGB24) ||   //RGB24
				(mt->subtype == MEDIASUBTYPE_RGB32)       //RGB32
#ifndef _WIN32_WCE
				|| (mt->subtype == MEDIASUBTYPE_YUY2) ||     // YUV420
				(mt->subtype == MEDIASUBTYPE_IYUV)
#endif			
				) &&    // YUV420P         
				(mt->cbFormat >= sizeof(VIDEOINFOHEADER)) &&
				(mt->pbFormat != NULL) && (!success)) {

				//Check if the device is capable of the video size required in Mode
				unsigned width = scc.MaxOutputSize.cx;
				unsigned height = scc.MaxOutputSize.cy;

				if ((frameWidth <= width) && (frameHeight <= height)) {
				// Set the Required size & frame rate
					PTRACE(5,"DShow\tChecking: w: " << width << " h: " << height << 
						                  " wanting w:" << frameWidth << " h: " << frameHeight );
					   
				VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER *)mt->pbFormat;

				if (frameWidth < width)
				  pVih->bmiHeader.biWidth= frameWidth;
				if (frameHeight < height) 
				  pVih->bmiHeader.biHeight= frameHeight;  

	           // Set the buffer size
				PString format = media_format_to_pwlib_format(mt->subtype);

				int xBufferSize = CalculateFrameBytes(frameWidth, frameHeight, format);

				pVih->bmiHeader.biSizeImage = xBufferSize;
						
				pVih->AvgTimePerFrame = 10000000 / frameRate;

				hr = pConfig->SetFormat(mt);

				 if (SUCCEEDED(hr)) {
				   PTRACE(5,"DShow\tWebcam Output Format Set to " << format);
				   success = TRUE;
				   break;
				 } else {
				   PTRACE(4,"DShow\tWebcam Output Format Not Set. " << format << " " << FailedMSG(hr));
				 }
			   } else {
			     PTRACE(4,"DShow\tWebcam does not support proposed Video: w: " << width << " h: " << height);
			   }
		   } else {
			PString colourFormat = "";
                        // Check the video format	

			if (mt->majortype == MEDIATYPE_Video) {
              colourFormat = media_format_to_pwlib_format(mt->subtype);

			//Get device video size
			  unsigned w = scc.MaxOutputSize.cx;
			  unsigned h = scc.MaxOutputSize.cy;
  
			  PTRACE(5,"DShow\tWebcam Video " << colourFormat << " w: " << w << " h: " << h );
			} else {
				PString format = "";

			  if (mt->majortype == MEDIATYPE_AnalogAudio)
				format = "Analog audio"; 
			  else if (mt->majortype == MEDIATYPE_AnalogVideo)
				format = "Analog video"; 
			  else if (mt->majortype == MEDIATYPE_Audio)
				format = "Audio";
			  else if (mt->majortype == MEDIATYPE_File)
				format = "File(Obsolete)"; 
			  else if (mt->majortype == MEDIATYPE_Interleaved)
				format = "Interleaved audio and video.(DV)"; 
			  else if (mt->majortype == MEDIATYPE_LMRT)
				format = "LMRT Obsolete";
			  else if (mt->majortype == MEDIATYPE_Midi)
				format = "MIDI format";
			  else if (mt->majortype == MEDIATYPE_MPEG2_PES)
				format = "MPEG-2 PES packets"; 
			  else if (mt->majortype == MEDIATYPE_ScriptCommand)
				format = "Data is a script command"; 
			  else if (mt->majortype == MEDIATYPE_Stream)
				format = "Byte stream";
			  else if (mt->majortype == MEDIATYPE_Text) 
				format = "Text";
			  else if (mt->majortype == MEDIATYPE_Timecode)
				format = "Timecode data";
			  else if (mt->majortype == MEDIATYPE_URL_STREAM)
				format = "URL Obsolete";
			  else
				format = "unknown";

              	          PTRACE(4,"DShow\tWebcam Unsupported Format " << format );
			}
		 }
			   
		   if(mt->pbFormat!=NULL && mt->cbFormat>0)
		   {
		     CoTaskMemFree(mt->pbFormat);
		     mt->pbFormat=NULL;
		     mt->cbFormat=0;
		   }
           } else {
		PTRACE(4,"DShow\tCannot access webcam video capability " << iFormat);
	   }
	}
    }

    SAFE_RELEASE(pConfig);

    return success;
}


PBoolean PVideoInputDevice_DirectShow2::Open(
      const PString & DeviceName, 
      PBoolean startImmediate     
    )
{

	Close();

	HRESULT hr;
// Get the interface for DirectShow's GraphBuilder

    hr=CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, 
                         IID_IGraphBuilder, (void **)&m_pGB);

    if(FAILED(hr)) {
		PTRACE(6,"DShow\tCannot initialise DirectShow GraphBuilder");
		return FALSE;
	}

// Bind the WebCam Input
		// Create the capture graph builder
#ifndef _WIN32_WCE
	hr = CoCreateInstance (CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC_SERVER,
                           IID_ICaptureGraphBuilder2, (void **) &m_pCap);
#else
	hr = CoCreateInstance (CLSID_CaptureGraphBuilder , NULL, CLSCTX_INPROC_SERVER,
                           IID_ICaptureGraphBuilder2, (void **) &m_pCap);
#endif
		if (FAILED(hr)) {
		    PTRACE(6,"DShow\tCannot initialise DirectShow capture graph builder");
			return FALSE;
		}

	    m_pCap->SetFiltergraph(m_pGB);

// Bind Device Filter.  We know the device because the id was passed in
		if(!BindFilter(DeviceName, &m_pDF)) {
			PTRACE(6,"DShow\tCannot bind webcam " << deviceName << " to filter");
			return FALSE;
		}

		hr=m_pGB->AddFilter(m_pDF, L"Video Capture");
		if (FAILED(hr)) {
			PTRACE(6,"DShow\tCannot add video capture filter");
		    return FALSE;
		}

// Get the Webcam output Pin 
		CComPtr<IEnumPins> pEnum;
		m_pDF->EnumPins(&pEnum);

		if (FAILED(hr)) {
		    PTRACE(6,"DShow\tCould not enumerate Output pins " << FailedMSG(hr));
			return FALSE;
		}

		hr = pEnum->Reset();
		hr = pEnum->Next(1, &m_pCamOutPin, NULL); 

		if (FAILED(hr)) {
		    PTRACE(6,"DShow\tUnable to find WebCam Output pin " << FailedMSG(hr));
			return FALSE;
		}

// Set the format of the Output pin of the webcam
		if (!SetVideoFormat(m_pCamOutPin)) {
			PTRACE(4,"DShow\tCannot Set the WebCam Video Format " << deviceName);
		    return FALSE;
		}

// Query for camera controls
	PVideoControlInfo info;
	hr = m_pDF->QueryInterface(IID_IAMCameraControl, (void **)&m_pCC);
	    if (FAILED(hr)) {
			PTRACE(6,"DShow\tWebCam " << deviceName << " does not support Camera Controls.");
			SAFE_RELEASE(m_pCC);
			m_pCC = NULL;
			goto endcontroltest;
		} 

	hr = m_pCC->GetRange(CameraControl_Pan, &info.min, &info.max, &info.step, &info.def, &info.flags);
	if (FAILED(hr)) {
			PTRACE(6,"DShow\tWebCam " << deviceName << " does not support Pan. Controls DISABLED");
			SAFE_RELEASE(m_pCC);
			m_pCC = NULL;
			goto endcontroltest;
	} 
	hr = m_pCC->GetRange(CameraControl_Tilt, &info.min, &info.max, &info.step, &info.def, &info.flags);
	if (FAILED(hr)) {
			PTRACE(6,"DShow\tWebCam " << deviceName << " does not support Tilt. Controls DISABLED");
			SAFE_RELEASE(m_pCC);
			m_pCC = NULL;
			goto endcontroltest;
	} 
	hr = m_pCC->GetRange(CameraControl_Zoom, &info.min, &info.max, &info.step, &info.def, &info.flags);
	if (FAILED(hr)) {
			PTRACE(6,"DShow\tWebCam " << deviceName << " does not support zoom. Controls DISABLED");
			SAFE_RELEASE(m_pCC);
			m_pCC = NULL;
			goto endcontroltest;
	}

	PTRACE(4,"DShow\tWebCam " << deviceName << " supports Camera Controls. Controls ENABLED");

endcontroltest:

// Buid the WebCam Sample Grabber
	PTRACE(6,"DShow\tBuilding Sample Grabber");
		IBaseFilter  *m_pGrab; 
		hr = CoCreateInstance(CLSID_SampleGrabber , NULL, CLSCTX_INPROC_SERVER,
							   IID_IBaseFilter, (void **) &m_pGrab);
		if (FAILED(hr)) {
		    PTRACE(6,"DShow\tCannot initialise Sample Grabber");
			return FALSE;
		}

		hr=m_pGB->AddFilter(m_pGrab, L"Sample Grabber");
		if (FAILED(hr)) {
			PTRACE(6,"DShow\tCannot add grabber filter");
		    return FALSE;
		}

		hr=m_pGrab->QueryInterface(IID_ISampleGrabber, (void **)&m_pGrabber);	
		if (FAILED(hr)) {
			PTRACE(6,"DShow\tCannot find Grabber Interface");
		    return FALSE;
		}

		   AM_MEDIA_TYPE mt;
			ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
			mt.majortype = MEDIATYPE_Video;
			mt.subtype = MEDIASUBTYPE_RGB24;
//			mt.subtype = MEDIASUBTYPE_IYUV;
		 m_pGrabber->SetMediaType(&mt);
		if FAILED(hr) {
 			PTRACE(6,"DShow\tFailed to Set Grabber Media Type : " << FailedMSG(hr));
		    return FALSE;   
		}

		pBufferSize = CalculateFrameBytes(frameWidth, frameHeight, "RGB24");
        PTRACE(5,"DShow\tBuffer size set to " << pBufferSize);

		hr = m_pGrabber->SetBufferSamples(TRUE);
		if FAILED(hr) {
 			PTRACE(6,"DShow\tFailed to Set Grabber buffer samples : " << FailedMSG(hr));
		    return FALSE;   
		}

    PTRACE(6,"DShow\tSetting Sample Grabber Callback");
		m_pGrabberCB = new CSampleGrabberCB();
		hr = m_pGrabber->SetCallback(m_pGrabberCB,1);
		if FAILED(hr) {
 			PTRACE(6,"DShow\tFailed to Set Grabber Callback : " << FailedMSG(hr));
		    return FALSE;   
		}

    PTRACE(6,"DShow\tConnect Sample Grabber to WebCam");
		hr = ConnectFilters(m_pGB,m_pCamOutPin,m_pGrab);
		if FAILED(hr) {
 			PTRACE(6,"DShow\tFailed to connect webcam to Grabber filter : " << FailedMSG(hr));
		    return FALSE;   
		}

// Set the NULL Renderer
	PTRACE(6,"DShow\tBuilding NULL output filter");
	    hr = CoCreateInstance (CLSID_NullRenderer , NULL, CLSCTX_INPROC_SERVER,
							   IID_IBaseFilter, (void **) &m_pNull);
		if (FAILED(hr)) {
		    PTRACE(6,"DShow\tCannot initialise NULL Renderer");
			return FALSE;
		}

		hr=m_pGB->AddFilter(m_pNull, L"NULL Output");
		if (FAILED(hr)) {
			PTRACE(6,"DShow\tCannot add NULL output filter");
		    return FALSE;
		}

    PTRACE(6,"DShow\tConnect Null Render to Grab filter Pin");
		hr = ConnectFilters(m_pGB,m_pGrab,m_pNull);
		if (FAILED(hr)) {
			PTRACE(6,"DShow\tCannot Null Output Filter to transform filter " <<  FailedMSG(hr));
		    return FALSE;
		}  
		SAFE_RELEASE(m_pGrab);

// Starting/Stopping Video Interface.
        hr = m_pGB->QueryInterface(IID_IMediaControl, (void **)&m_pMC);
		if (FAILED(hr)) {
			PTRACE(6,"DShow\tCannot access Media control interface");
		    return FALSE;
		}

// always set BGR24 as that will be supported on almost all webcams.
	    SetColourFormat(preferredColourFormat);

   if (startImmediate) 
		Start();
		
	PTRACE(6,"DShow\tDevice " << DeviceName << " open.");
	deviceName = DeviceName;
	return TRUE;
}


PBoolean PVideoInputDevice_DirectShow2::IsOpen()
{
	return (m_psCurrent == Running);
}


PBoolean PVideoInputDevice_DirectShow2::Close()
{
	if (!IsOpen())
		return FALSE;

   PTRACE(6,"DShow\tTorn Down.");

// Stop WebCam Graph
	Stop();

// Release filters
	SAFE_RELEASE(m_pNull);
	SAFE_RELEASE(m_pGrabberCB);
	SAFE_RELEASE(m_pGrabber); 

// Release the WebCam and interfaces
	SAFE_RELEASE(m_pCC);
	SAFE_RELEASE(m_pMC);
	SAFE_RELEASE(m_pCamOutPin); 
	SAFE_RELEASE(m_pCap);
	SAFE_RELEASE(m_pDF);

// Relase DirectShow Graph
    SAFE_RELEASE(m_pGB); 

	if (pBuffer != NULL)
	       delete pBuffer;
	pBuffer = NULL;

	return TRUE;
}


PBoolean PVideoInputDevice_DirectShow2::Start()
{

    HRESULT hr;

	if (m_pMC == NULL)
		return FALSE;

	if (m_psCurrent == Running) {
        PTRACE(5, "DShow\tVideo is already started");
		return TRUE;
	}

	if((m_psCurrent == Paused) || (m_psCurrent == Stopped))
    {
        hr = m_pMC->Run();
 
		if SUCCEEDED(hr) {
			PTRACE(6,"DShow\tVideo Started.");
			m_psCurrent = Running;		
		    return TRUE;
		}
		PTRACE(4,"DShow\tUnable to Start Video. " << FailedMSG(hr));
    }

    PTRACE(4,"DShow\tUnable to Start Video. " << m_psCurrent );
	return FALSE;
}

PBoolean PVideoInputDevice_DirectShow2::Stop()
{
    HRESULT hr;

	if (m_pMC == NULL)
		return FALSE;

	if((m_psCurrent == Paused) || (m_psCurrent == Running))
    {
       hr = m_pMC->Stop();
		if SUCCEEDED(hr) {
		   PTRACE(6,"DShow\tVideo Stopped.");
           m_psCurrent = Stopped;
		   return TRUE;
		}
    }
	return FALSE;
}

PBoolean PVideoInputDevice_DirectShow2::IsCapturing()
{
	return (m_psCurrent == Running);
}

PBoolean PVideoInputDevice_DirectShow2::SetColourFormat(
      const PString & ColourFormat 
    )
{

  PTRACE(6,"DShow\tSetting Video Format " << ColourFormat);

  PBoolean running = IsCapturing();
  if (running)
    Stop();

  if (!PVideoDevice::SetColourFormat(ColourFormat))
    return FALSE;

  if (running) 
    return Start();


  return TRUE;
}

PBoolean PVideoInputDevice_DirectShow2::SetFrameRate(
      unsigned Rate 
    )
{
  if (frameRate == Rate)
	  return TRUE;

  PTRACE(5,"DShow\tSetting Video FrameRate " << Rate);

  if (!PVideoDevice::SetFrameRate(Rate))
    return FALSE;
  
  bool running = IsCapturing();
  if (running)
      Stop();

  /// Set the video Filter
  if ((m_pCamOutPin != NULL) && 
	  (!SetVideoFormat(m_pCamOutPin))) {
		  PTRACE(4,"DShow\tError setting frame rate " << Rate);
		  return FALSE;
	  }

  if (running)
	 Start();
	

	return TRUE;
}

PBoolean PVideoInputDevice_DirectShow2::SetFrameSize(
      unsigned Width,   
      unsigned Height   
    )
{

	if ((frameWidth == Width) && (frameHeight == Height))
		return TRUE;

	PTRACE(6,"DShow\tSetting Video FrameSize " << Width << " x " << Height);

    if (!PVideoDevice::SetFrameSize(Width, Height))
           return FALSE;

    bool stop = FALSE;
	if (IsCapturing()) { 
	  Stop();
	  stop = TRUE;
	}
        /// Set the video Filter
	if ((m_pCamOutPin != NULL) && 
		        (!SetVideoFormat(m_pCamOutPin))) {
	   PTRACE(4,"DShow\tUnable to set Video Format!");
	   return FALSE;
	} 

	if (stop)
	   Start();

	return TRUE;
}

PINDEX PVideoInputDevice_DirectShow2::GetMaxFrameBytes()
{
  return GetMaxFrameBytesConverted(CalculateFrameBytes(frameWidth, frameHeight, colourFormat));
}

PBoolean PVideoInputDevice_DirectShow2::GetFrameData(
      BYTE * buffer,                 
      PINDEX * bytesReturned   
    )
{
    return GetFrameDataNoDelay(buffer,bytesReturned);
}

PBoolean PVideoInputDevice_DirectShow2::GetFrameDataNoDelay(
      BYTE * buffer,                 
      PINDEX * bytesReturned  
    )
{

   PWaitAndSignal m(lastFrameMutex);

   PTRACE(6,"DShow\tGrabbing Frame");

   bool retval = false;

   if (pBuffer == NULL)
        pBuffer = new BYTE[pBufferSize];

   bool success = ((CSampleGrabberCB *)m_pGrabberCB)->GetCurrentBuffer(*pBuffer,pBufferSize);

   if (!success) {
	   PTRACE(2,"DShow\tGrab the current buffer failed!");
	   return false;
   }

   PTRACE(6,"DShow\tBuffer obtained." << pBufferSize );

   if (pBuffer != NULL) {

    // Check orientation of picture and VFlip if required
	  if (!checkVFlip) {
	    PTRACE(5,"DShow\tChecking Image orientation");
		  HRESULT hr;
		  AM_MEDIA_TYPE mt;
		  hr = m_pGrabber->GetConnectedMediaType(&mt);
		  if (SUCCEEDED(hr)) {
			  if (!CheckOrientation(pBuffer, (VIDEOINFOHEADER *)mt.pbFormat)) {
                 SetVFlipState(TRUE);
				 PTRACE(2,"DShow\tImage wrong orientation VFlipped");
			  }
		  } else {
             PTRACE(2,"DShow\tFailed to determine Image orientation");
		  }
		checkVFlip = TRUE;
	  } 

	// Convert the image for output
	  if (NULL != converter) {
         retval = converter->Convert(pBuffer,buffer, bytesReturned);
         PTRACE(6,"DShow\tBuffer converted." << *bytesReturned );
	  } else {

         PTRACE(6,"DShow\tBuffer copied." << pBufferSize );
         memcpy(buffer, pBuffer, pBufferSize);
	     if (buffer != NULL)
		   *bytesReturned = pBufferSize;
	    retval = true;
	  }
   }

  PTRACE(6,"DShow\tBuffer Transcoded "  << retval);
  
  return retval;  
}


PBoolean PVideoInputDevice_DirectShow2::TestAllFormats()
{
	return TRUE;
}

PBoolean PVideoInputDevice_DirectShow2::VerifyHardwareFrameSize(
						unsigned /*width*/, 
						unsigned /*height*/
						)
{
	return TRUE;
}

PBoolean PVideoInputDevice_DirectShow2::SetChannel(int newChannel)
{

  return TRUE;
}

#ifndef _WIN32_WCE
PVideoInputControl * PVideoInputDevice_DirectShow2::GetVideoInputControls()  
{
	if (!m_pCC)
	    return NULL; 

	return new PVideoInputControl_DirectShow2(m_pCC);
}
#endif


///////////////////////////////////////////////////////////

#ifndef _WIN32_WCE

PVideoInputControl_DirectShow2::PVideoInputControl_DirectShow2(IAMCameraControl * _pCC)
     : t_pCC(_pCC)
{
	HRESULT  hr;
	PVideoControlInfo panInfo;
	PVideoControlInfo tiltInfo;
	PVideoControlInfo zoomInfo;

	// Retrieve information about the PTZ
	panInfo.type= ControlPan;
	hr = t_pCC->GetRange(CameraControl_Pan, &panInfo.min, &panInfo.max, &panInfo.step, &panInfo.def, &panInfo.flags);
    panInfo.current = panInfo.def;
	if (SUCCEEDED(hr)) m_info.push_back(panInfo);

	tiltInfo.type= ControlTilt;
	hr = t_pCC->GetRange(CameraControl_Tilt, &tiltInfo.min, &tiltInfo.max, &tiltInfo.step, &tiltInfo.def, &tiltInfo.flags);
    tiltInfo.current = tiltInfo.def;
	if (SUCCEEDED(hr)) m_info.push_back(tiltInfo);

    zoomInfo.type= ControlZoom;
	hr = t_pCC->GetRange(CameraControl_Zoom, &zoomInfo.min, &zoomInfo.max, &zoomInfo.step, &zoomInfo.def, &zoomInfo.flags);
    zoomInfo.current = zoomInfo.def;
	if (SUCCEEDED(hr)) m_info.push_back(zoomInfo);

}
	
bool PVideoInputControl_DirectShow2::Pan(long value, bool absolute)
{
	PWaitAndSignal m(ccmutex);

	HRESULT  hr;
	PVideoControlInfo control;
	if (GetVideoControlInfo(PVideoControlInfo::ControlPan, control)) {
		long flags;
		if (absolute)
			flags = KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE | KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
		else
			flags = KSPROPERTY_CAMERACONTROL_FLAGS_RELATIVE | KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;

	    hr = t_pCC->Set(CameraControl_Pan, value, flags);
	   if SUCCEEDED(hr) {
		   hr = t_pCC->Get(CameraControl_Pan, &control.current, &flags);
		   PTRACE(6,"CC\tSet Pan to " << control.current);
		   return true;
	   } else {
		   PTRACE(4, "CC\tFailed to pan to " << value);
	   }
	}
	return false;
}

bool PVideoInputControl_DirectShow2::Tilt(long value, bool absolute)
{
    PWaitAndSignal m(ccmutex);

	HRESULT  hr;
	PVideoControlInfo control;
	if (GetVideoControlInfo(PVideoControlInfo::ControlTilt, control)) {
		long flags;
		if (absolute)
			flags = KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE | KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
		else
			flags = KSPROPERTY_CAMERACONTROL_FLAGS_RELATIVE | KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;

	    hr = t_pCC->Set(CameraControl_Tilt, value, flags);
	   if SUCCEEDED(hr) {
		   hr = t_pCC->Get(CameraControl_Tilt, &control.current, &flags);
		   PTRACE(6,"CC\tSet tilt to " << control.current);
		   return true;
	   } else {
		   PTRACE(4, "CC\tFailed to tilt to " << value);
	   }
	}	
	return false;
}

bool PVideoInputControl_DirectShow2::Zoom(long value, bool absolute)
{
	PWaitAndSignal m(ccmutex);

	// 50 is 100% and 200 is 4x zoom
	if (absolute && ((value < 50) || (value > 200))) {
		PTRACE(4, "CC\tWrong zoom value received: " << value << " must be between 50 (1x) and 200 (4x).");
		return false;
	}

	HRESULT  hr;
	PVideoControlInfo control;
	if (GetVideoControlInfo(PVideoControlInfo::ControlZoom, control)) {
		long flags;

		flags = KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE | KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;

		if (!absolute) {
		     control.current = control.current + value;
		}

	   hr = t_pCC->Set(CameraControl_Zoom, control.current, flags);
	   if SUCCEEDED(hr) {
		   PTRACE(6,"CC\tSet Zoom to " << control.current);
		   SetCurrentPosition(PVideoControlInfo::ControlZoom, control.current);
		   return true;
	   } else {
		   PTRACE(4, "CC\tFailed to zoom to " << value);
	   }
	}
	return false;
}

#endif  // WCE

#endif  // P_DSHOW
