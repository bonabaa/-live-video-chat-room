// PVideoInputDevice_DirectShowEx.cpp: implementation of the PVideoInputDevice_DirectShowEx class.
//
//////////////////////////////////////////////////////////////////////

#include <ptlib.h>
#ifdef DIRECTSHOWEX

#if defined(P_DIRECTSHOW)

#include "vidinput_directshowex.h"

#ifdef P_DIRECTSHOW_LIBRARY1
#pragma comment(lib, P_DIRECTSHOW_LIBRARY1)
#endif
#ifdef P_DIRECTSHOW_LIBRARY2
#pragma comment(lib, P_DIRECTSHOW_LIBRARY2)
#endif

#include <atlbase.h>
#include <dshow.h>
#include <mmsystem.h>
#include <strmif.h>
#include <amvideo.h>    
#include <uuids.h>    
#include <control.h>

#include <ptlib/vconvert.h>
//#include "../self/aplux.h"
//#include "../bbqdata.h"
//#include "../Dongle/dinfo.h"
#include "Dvdmedia.h"
#include "bbqCameraUtils.h"
#include "bbqCamera/SfVideoWndMsg.h"
#include <ptclib/pxml.h> 


//#pragma comment(lib, "./Dongle/Spromeps.lib")
//#pragma comment(lib, "./Dongle/DRLib.lib")

//#define	SUPPORT_TIME_STAMP
//
//#define PROTECT_HD_MIN_WIDTH	960
//#define PROTECT_HD_MIN_HEIGHT	768

namespace PWLibStupidLinkerHacks {
   int loadDirectShowExStuff;
};

PCREATE_VIDINPUT_PLUGIN(DirectShowEx);

//class PVideoInputDevice_DirectShowEx_PluginServiceDescriptor : public PDevicePluginServiceDescriptor
//{
//  public:
//    virtual PObject *    CreateInstance(int /*userData*/) const { return new PVideoInputDevice_DirectShowEx; }
//
//    virtual PStringArray GetDeviceNames(int /*userData*/) const { return PVideoInputDevice_DirectShowEx::GetInputDeviceNames(); }
//
//    virtual bool         GetDeviceCapabilities(const PString & deviceName, void * caps) const
//      { return PVideoInputDevice_DirectShowEx::GetDeviceCapabilities(deviceName, (PVideoInputDevice::Capabilities *)caps); }
//
//    virtual bool ValidateDeviceName(const PString & deviceName, int /*userData*/) const
//    {
//      return  true;//(deviceName.Left(10) *= "appwindow:") && (FindWindow(NULL, deviceName.Mid(10)) != NULL); //changed by chenyuan
//    }
//
//
//} PVideoInputDevice_DirectShowEx_descriptor;
//
//PCREATE_PLUGIN(DirectShowEx, PVideoInputDevice, &PVideoInputDevice_DirectShowEx_descriptor);

static struct {
    unsigned dest_width, dest_height, device_width, device_height;
} prefResizeTable[] = {    
	{ 80,  72,     88,  72 },
	{ 176,  144,   180,  144 },
	{ 352,  288,   360,  288 },
    { 640, 480,    320, 240 },
    { 640, 480,    160, 120 },
    { 352, 288,    320, 240 },
    { 176, 144,    160, 120 },
    { 176, 144,    320, 240 },
    { 176, 144,    352, 288 },   //Generate small video when camera only does large.
};

void SplitIPCameraType(const PString& strName, PString& strIPType, PString& strIP)
{
	strIPType = "";
	strIP = "";
	int nPos = strName.Find("vfoncameratype://");
	if( nPos == 0 ) {
		int nPos1 = strName.Find(":", strlen("vfoncameratype://"));
		if( nPos1 >= nPos ) {
			strIPType = strName.Mid(strlen("vfoncameratype://"), nPos1-nPos - strlen("vfoncameratype://"));
			strIP = strName.Mid(nPos1+1);
		}
	}
}

PString GetIPCameraUrl(LPCTSTR pszIPCamera, LPCTSTR pszIP)
{
	PString strPath = "";//::GetAppDataFile("ipcamera.xml");
	//PXML xml(PXML::NewLineAfterElement);
	PXML xml;
	xml.LoadFile((LPCTSTR)strPath);
	PXMLElement* rootElement= xml.GetRootElement();
	//BBQFramework* pFramework = ::GetBBQFramework();
	//BBQAccount* pAccount = pFramework->GetCurrentAccount();
	rootElement= xml.GetRootElement();
	for(PINDEX i = 0; i < rootElement->GetSize(); i++ ) 
	{
		PXMLObject* pObject = rootElement->GetElement(i);
		if( pObject && pObject->IsElement()  ) 
		{
			PXMLElement* pElement = (PXMLElement*)pObject;
			if(pElement->GetName() *= "IPCamera") 
			{
				PString str = pElement->GetAttribute("Type");
				PString strT = str;//::UTF8ToPlainText(str);
				if( strT == pszIPCamera ) {
					str = pElement->GetAttribute("Url");
					strT = str;//::UTF8ToPlainText(str);
					strT = "vfoncamera://" + strT;
					PString strIPCameraUrl;
          strIPCameraUrl= PString(PString::Printf, strT, pszIP);
					return strIPCameraUrl;
				}
			}
		}		
	}
	return "";
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//class PDXVideoInputThread : public CWinThread
//{
//public:
//    PDXVideoInputThread (PVideoInputDevice_DirectShowEx & dev, PBoolean bOpenOnly = FALSE)
//      : BBQThread("PDXVideoInputThread"), device(dev), m_bOpenOnly(bOpenOnly)
//      { 
//		m_bAutoDelete = FALSE;
//		CreateThread(); 
//	}
//	virtual PBoolean InitInstance()
//	{
//		BBQThread::InitInstance();
//		return device.Win32HandleCapture(m_bOpenOnly);
//	}
//	virtual PBoolean ExitInstance()
//	{
//		device.Win32CaptureThreadEnd();
//		return BBQThread::ExitInstance();
//	}
//  protected:
//    PVideoInputDevice_DirectShowEx & device;
//	PBoolean m_bOpenOnly;
//};


///////////////////////////////////////////////////////////////////////////////

class PDXCapStatus : public DXCAPSTATUS
{
  public:
    PDXCapStatus(HWND hWnd);
    PBoolean IsOK()
       { return uiImageWidth != 0; }
};


///////////////////////////////////////////////////////////////////////////////

class PDXVideoDeviceBitmap : PBYTEArray
{
  public:
    // the following method is replaced by PDXVideoDeviceBitmap(HWND hWnd, WORD bpp)
    // PDXVideoDeviceBitmap(unsigned width, unsigned height, const PString & fmt);
    //returns object with gray color pallet if needed for 8 bpp formats
    PDXVideoDeviceBitmap(HWND hWnd, WORD bpp);
    // does not build color pallet
    PDXVideoDeviceBitmap(HWND hWnd); 
    // apply video format to capture device
    PBoolean ApplyFormat(HWND hWnd);

    BITMAPINFO * operator->() const 
    { return (BITMAPINFO *)theArray; }
};



///////////////////////////////////////////////////////////////////////////////

#define INVALID_COMP 0xffffffff

static struct vfwComp {
  DWORD compression; // values for .biCompression field that *might* be accepted
  PBoolean  askForVFlip; // is it likely to need vertical flipping during conversion?
} vfwCompTable[] = {
  { BI_RGB,                       TRUE, }, //[0]
  { BI_BITFIELDS,                 TRUE, }, //[1]
  { mmioFOURCC('I','Y','U','V'), FALSE, }, //[2]
  { mmioFOURCC('Y','V','1','2'), FALSE, }, //[3], same as IYUV/I420 except that  U and V planes are switched
  { mmioFOURCC('Y','U','Y','2'), FALSE, }, //[4]
  { mmioFOURCC('U','Y','V','Y'), FALSE, }, //[5], Like YUY2 except for ordering
  { mmioFOURCC('Y','V','Y','U'), FALSE, }, //[6], Like YUY2 except for ordering
  { mmioFOURCC('Y','V','U','9'), FALSE, }, //[7]
  { mmioFOURCC('M','J','P','G'), FALSE, }, //[8]
  { mmioFOURCC('I','4','2','0'), FALSE, }, // [9] same as IYUV
  { BI_RGB,                       FALSE, }, //[10]
  { BI_BITFIELDS,                 FALSE, }, //[11]
  { INVALID_COMP, }, 
};

static struct { 
  const char * colourFormat; 
  WORD  bitCount;
  PBoolean negHeight;
  vfwComp *vfwComp; 
} FormatTable[] = {
  { "I420",    12, FALSE, &vfwCompTable[9], },
  { "YUY2",    16, FALSE, &vfwCompTable[4], },
  { "UYVY",    16, FALSE, &vfwCompTable[5], },
  { "YVYU",    16, FALSE, &vfwCompTable[6], },
  { "YV12",    12, FALSE, &vfwCompTable[3], },
  { "YUV420P", 12, FALSE, &vfwCompTable[2], },
  { "IYUV",    12, FALSE, &vfwCompTable[2], },
  { "RGB24F",  24, TRUE, &vfwCompTable[10], },
  { "RGB24",   24, FALSE,  &vfwCompTable[0], },
  { "RGB32F",  32, TRUE, &vfwCompTable[10], },
  { "RGB32",   32, FALSE,  &vfwCompTable[0], },
  { "Grey",     8, FALSE,  &vfwCompTable[0], },
  { "Gray",     8, FALSE,  &vfwCompTable[0], },
  { "GreyF",    8, TRUE, &vfwCompTable[10], },
  { "GrayF",    8, TRUE, &vfwCompTable[10], },
  { "RGB565",  16, TRUE,  &vfwCompTable[1], },
  { "RGB565F", 16, FALSE, &vfwCompTable[11], },
  { "RGB555",  15, TRUE,  &vfwCompTable[1], },
  { "RGB555F", 15, FALSE, &vfwCompTable[11], },
//  { "I420",    12, FALSE, &vfwCompTable[9], },
  { "YUV422",  16, FALSE, &vfwCompTable[4], },
  { "MJPEG",   12, FALSE, &vfwCompTable[8], },
  { NULL,},
};


static struct {
    unsigned device_width, device_height;
} winTestResTable[] = {
    { 176, 144 },
    { 352, 288 },
    { 320, 240 },
    { 160, 120 },
    { 640, 480 },
    { 704, 576 },
    {1024, 768 },
};

static struct { 
  const char * colourFormat; 
  WORD  bitCount;
  PBoolean negHeight;
  vfwComp *vfwComp; 
} PreferredFormatTable[] = {
  { "I420",    12, FALSE, &vfwCompTable[9], },
  { "YUY2",    16, FALSE, &vfwCompTable[4], },
  { "YV12",    12, FALSE, &vfwCompTable[3], },
  { "UYVY",    16, FALSE, &vfwCompTable[5], },
  { "IYUV",    12, FALSE, &vfwCompTable[2], },
  { "RGB24F",   24, TRUE,  &vfwCompTable[10], },
  { "RGB24",   24, FALSE,  &vfwCompTable[0], },
};

PDXVideoDeviceBitmap::PDXVideoDeviceBitmap(HWND hCaptureWindow)
{
	PINDEX sz = dxGetVideoFormatSize(hCaptureWindow);
	SetSize(sz);
	if (!dxGetVideoFormat(hCaptureWindow, theArray, sz)) { 
		PTRACE(1, "PVidInp\tdxGetVideoFormat(hCaptureWindow) failed - " << ::GetLastError());
		SetSize(0);
		return;
	}
}

PDXVideoDeviceBitmap::PDXVideoDeviceBitmap(HWND hCaptureWindow, WORD bpp)
{
	PINDEX sz = dxGetVideoFormatSize(hCaptureWindow);
	SetSize(sz);
	if (!dxGetVideoFormat(hCaptureWindow, theArray, sz)) { 
		PTRACE(1, "PVidInp\tdxGetVideoFormat(hCaptureWindow) failed - " << ::GetLastError());
		SetSize(0);
		return;
	}

	if (8 == bpp && bpp != ((BITMAPINFO*)theArray)->bmiHeader.biBitCount) {
		SetSize(sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*256);
		for (int i = 0; i < 256; i++)
			((BITMAPINFO*)theArray)->bmiColors[i].rgbBlue  = ((BITMAPINFO*)theArray)->bmiColors[i].rgbGreen = ((BITMAPINFO*)theArray)->bmiColors[i].rgbRed = (BYTE)i;
	}
	((BITMAPINFO*)theArray)->bmiHeader.biBitCount = bpp;
}

PBoolean PDXVideoDeviceBitmap::ApplyFormat(HWND hWnd) 
{
	((BITMAPINFO*)theArray)->bmiHeader.biPlanes = 1;
	((BITMAPINFO*)theArray)->bmiHeader.biSizeImage = 
	(((BITMAPINFO*)theArray)->bmiHeader.biHeight<0 ? -((BITMAPINFO*)theArray)->bmiHeader.biHeight : ((BITMAPINFO*)theArray)->bmiHeader.biHeight)
			*4*((((BITMAPINFO*)theArray)->bmiHeader.biBitCount * ((BITMAPINFO*)theArray)->bmiHeader.biWidth + 31)/32);
  return dxSetVideoFormat(hWnd, theArray, GetSize()) == TRUE;
}

///////////////////////////////////////////////////////////////////////////////

PDXCapStatus::PDXCapStatus(HWND hWnd)
{
	memset(this, 0, sizeof(*this));
	if (dxGetStatus(hWnd, this, sizeof(*this)))
		return;

	PTRACE(1, "PVidInp\tdxGetStatus: failed - " << ::GetLastError());
}


///////////////////////////////////////////////////////////////////////////////
// PVideoDevice

PVideoInputDevice_DirectShowEx::PVideoInputDevice_DirectShowEx()
{
#ifndef _WIN32_WCE
  CoInitialize(NULL);
#else
  CoInitializeEx(NULL,COINIT_MULTITHREADED);
#endif
  BBQCamera_SetTraceFile("c:\\cameradll.log");
  BBQCamera_SetTraceLevel(5);
	//captureThread = NULL;
	hCaptureWindow = NULL;
	isCapturingNow = PFalse;
	m_bRealStop = PFalse;

	m_nBuffersSize = 1;//clientVersionData.GetFileProfileInt("Camera","BufferSize", 1);
	m_hSemaphore  = ::CreateSemaphore(NULL, 0, m_nBuffersSize, NULL);

	m_bPause = PFalse;
}

PVideoInputDevice_DirectShowEx::~PVideoInputDevice_DirectShowEx()
{
	Close();
	CloseHandle(m_hSemaphore);
}
void PVideoInputDevice_DirectShowEx::HandleCapture(PThread &, INT)
{
  PBoolean initSucceeded = Win32HandleCapture();

  if (initSucceeded) {
    threadStarted.Signal();

    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0))
      ::DispatchMessage(&msg);
  }
  Win32CaptureThreadEnd();
  PTRACE(5, "PVidInp\tDisconnecting driver");
  //capDriverDisconnect(hCaptureWindow);
  //capSetUserData(hCaptureWindow, NULL);

  //capSetCallbackOnError(hCaptureWindow, NULL);
  //capSetCallbackOnVideoStream(hCaptureWindow, NULL);

  //PTRACE(5, "PVidInp\tDestroying VIDCAP window");
  //DestroyWindow(hCaptureWindow);
  //hCaptureWindow = NULL;

  // Signal the other thread we have completed, even if have error
  if (!initSucceeded)
    threadStarted.Signal();
}

PBoolean PVideoInputDevice_DirectShowEx::Open(const PString & devName, PBoolean startImmediate)
{
	colourFormat = "";
	preferredColourFormat = "";
	if (converter)
		delete converter;
	converter = NULL;
    frameRate = 0;

	Close();

	PBoolean bRet = PTrue;
	deviceName = devName;
#if 0
	//win32captureThread = new PDXVideoInputThread(*this);
	win32threadStarted.Wait();
	if (hCaptureWindow == NULL) {
 /*   if (win32captureThread)
    {
      win32captureThread->WaitForTermination();
    }*/
////if(win32captureThread!=NULL && win32captureThread->m_hThread != NULL) {
////			win32captureThread->PostThreadMessage(WM_QUIT, 0, 0L);
//////			if( GetCurrentThreadId() == win32captureThread->m_nThreadID ) {
////				DWORD dwRet = ::WaitForSingleObject(win32captureThread->m_hThread, 50);
////				while(dwRet != WAIT_OBJECT_0) {
////					MSG msg;
////					if( ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) {
////						::TranslateMessage(&msg);
////						::DispatchMessage(&msg);
////					}
////					dwRet = ::WaitForSingleObject(win32captureThread->m_hThread, 50);
////				}
//////			} else {
//////				::WaitForSingleObject(win32captureThread->m_hThread, INFINITE);
//////			}
////		}
////		delete win32captureThread;
////		win32captureThread = NULL;
////		bRet = FALSE;
	}
#else
	//win32captureThread = NULL;
  captureThread = PThread::Create(PCREATE_NOTIFIER(HandleCapture), "VidIn");
  threadStarted.Wait();
  	if (hCaptureWindow == NULL) {
      delete captureThread;
      captureThread = NULL;

      bRet = PFalse;
    }

	//bRet = Win32HandleCapture();
#endif
	if( bRet ) {
		if (startImmediate)
			bRet = Start();
		else 
			bRet = PTrue;
	}
	return bRet;
}

PBoolean PVideoInputDevice_DirectShowEx::OpenOnly(const PString & devName, PBoolean startImmediate)
{
	colourFormat = "";
	preferredColourFormat = "";
	if (converter)
		delete converter;
	converter = NULL;
    frameRate = 0;

	Close();

	PBoolean bRet = PTrue;
	deviceName = devName;
#if 0
	//win32captureThread = new PDXVideoInputThread(*this, TRUE);
	win32threadStarted.Wait();
	if (hCaptureWindow == NULL) {
////		if(win32captureThread!=NULL && win32captureThread->m_hThread != NULL) {
////			win32captureThread->PostThreadMessage(WM_QUIT, 0, 0L);
//////			if( GetCurrentThreadId() == win32captureThread->m_nThreadID ) {
////				DWORD dwRet = ::WaitForSingleObject(win32captureThread->m_hThread, 50);
////				while(dwRet != WAIT_OBJECT_0) {
////					MSG msg;
////					if( ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) {
////						::TranslateMessage(&msg);
////						::DispatchMessage(&msg);
////					}
////					dwRet = ::WaitForSingleObject(win32captureThread->m_hThread, 50);
////				}
//////			} else {
//////				::WaitForSingleObject(win32captureThread->m_hThread, INFINITE);
//////			}
		}
		//delete win32captureThread;
		//win32captureThread = NULL;
		bRet = PFalse;
	}
#else
	//win32captureThread = NULL;
	bRet = Win32HandleCapture(PTrue);
#endif
	if( bRet ) {
		if (startImmediate)
			bRet = Start();
		else 
			bRet = PTrue;
	}
	return bRet;
}


PBoolean PVideoInputDevice_DirectShowEx::IsOpen() 
{
  return hCaptureWindow != NULL;
}


PBoolean PVideoInputDevice_DirectShowEx::Close()
{
	if (!IsOpen())
		return PFalse;
 
	Stop();
#if 0
////	if(win32captureThread!=NULL && win32captureThread->m_hThread != NULL) {
////		win32captureThread->PostThreadMessage(WM_QUIT, 0, 0L);
//////		if( GetCurrentThreadId() == win32captureThread->m_nThreadID ) {
////			DWORD dwRet = ::WaitForSingleObject(win32captureThread->m_hThread, 50);
////			while(dwRet != WAIT_OBJECT_0) {
////				MSG msg;
////				if( ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) {
////					::TranslateMessage(&msg);
////					::DispatchMessage(&msg);
////				}
////				dwRet = ::WaitForSingleObject(win32captureThread->m_hThread, 50);
////			}
//////		} else {
//////			::WaitForSingleObject(win32captureThread->m_hThread, INFINITE);
//////		}
////	}
////	delete win32captureThread;
////	win32captureThread = NULL;
#else
	//Win32CaptureThreadEnd(); //see HandleCapture
#endif

  ::PostThreadMessage(captureThread->GetThreadId(), WM_QUIT, 0, 0L);

  // Some brain dead drivers may hang so we provide a timeout.
  if (!captureThread->WaitForTermination(5000))
  {
      // Two things may happen if we are forced to terminate the capture thread:
      // 1. As the VIDCAP window is associated to that thread the OS itself will 
      //    close the window and release the driver
      // 2. the driver will not be released and we will not have video until we 
      //    terminate the process
      // Any of the two ios better than just hanging
      captureThread->Terminate();
      hCaptureWindow = NULL;
      PTRACE(1, "PVidInp\tCapture thread failed to stop. Terminated");
  }

  delete captureThread;
  captureThread = NULL;
  //
	colourFormat = "";
	preferredColourFormat = "";
	if (converter)
		delete converter;
	converter = NULL;
	lastError = 0;
    frameRate = 0;
	return PTrue;
}


PBoolean PVideoInputDevice_DirectShowEx::Start()
{
	if (IsCapturing())
		return PFalse;

//	frameAvailable.Wait(0);
	DestroyBuffers();

	if (dxCaptureStart(hCaptureWindow)) {
		isCapturingNow = PTrue;//status.fCapturingNow;
		m_bRealStop = PFalse;
		PTRACE(5, "PVideoInputDevice_DirectShowEx Capture has started");
		return isCapturingNow;
	}

	lastError = ::GetLastError();
	PTRACE(1, "PVidInp\tcapCaptureSequenceNoFile: failed - " << lastError);
	return PFalse;
}


PBoolean PVideoInputDevice_DirectShowEx::Stop()
{
	m_bPause = PFalse;
	m_bRealStop = PTrue;
	return InnerStop();
}

PBoolean PVideoInputDevice_DirectShowEx::InnerStop()
{
	if (!IsCapturing())
		return PFalse;

	isCapturingNow = FALSE;

	DestroyBuffers();

	PBoolean bRet;
	bRet = dxCaptureStop(hCaptureWindow);

	PTRACE(5, "PVideoInputDevice_DirectShowEx Capture has stopped");

//	if(bRet) 
//		cameraStopMutex.Wait();
	

	if(!bRet){
	  lastError = ::GetLastError();
	  PTRACE(1, "PVidInp\tcapCaptureStop: failed - " << lastError);
	  return PFalse;
	}
	return PTrue;
}


PBoolean PVideoInputDevice_DirectShowEx::IsCapturing()
{
	return isCapturingNow;
}

PBoolean PVideoInputDevice_DirectShowEx::IsSizeSupport(UINT nWidth, UINT nHeight)
{
	return dxTestFrameSize(hCaptureWindow, nWidth, nHeight);
}

int PVideoInputDevice_DirectShowEx::IsPTZControlSupported()
{
	if( hCaptureWindow == NULL ) return FALSE;
	return dxTestPTZControl(hCaptureWindow);
}

int PVideoInputDevice_DirectShowEx::PTZControl(int nIndex, int nValue)
{
	if( hCaptureWindow == NULL ) return FALSE;
	return dxPTZControl(hCaptureWindow, nIndex, nValue);
}

int PVideoInputDevice_DirectShowEx::PTZStringControl(const PString& strCommand)
{
	if( hCaptureWindow == NULL ) return FALSE;
	return dxPTZStringControl(hCaptureWindow, strCommand);
}

int PVideoInputDevice_DirectShowEx::PTZWindow(LPSIZE size, LPRECT rect)
{
	if( hCaptureWindow == NULL ) return FALSE;
	return dxPTZWindow(hCaptureWindow, size, rect);
}

PBoolean PVideoInputDevice_DirectShowEx::TestAllFormats() {
	PBoolean running = IsCapturing();
	if (running)
		InnerStop();

	for (PINDEX prefFormatIdx = 0; prefFormatIdx < PARRAYSIZE(FormatTable); prefFormatIdx++) {
		PDXVideoDeviceBitmap bi(hCaptureWindow, FormatTable[prefFormatIdx].bitCount); 
		for (PINDEX prefResizeIdx = 0; prefResizeIdx < PARRAYSIZE(winTestResTable); prefResizeIdx++) {
			if (FormatTable[prefFormatIdx].colourFormat != NULL)
				bi->bmiHeader.biCompression = FormatTable[prefFormatIdx].vfwComp->compression;
			else
				continue;

			bi->bmiHeader.biHeight = winTestResTable[prefResizeIdx].device_height;
			bi->bmiHeader.biWidth = winTestResTable[prefResizeIdx].device_width;

			// set .biHeight according to .negHeight value
			if (FormatTable[prefFormatIdx].negHeight && bi->bmiHeader.biHeight > 0)
				bi->bmiHeader.biHeight = -(int)bi->bmiHeader.biHeight; 
			else if (!FormatTable[prefFormatIdx].negHeight && bi->bmiHeader.biHeight < 0)
				bi->bmiHeader.biHeight = -(int)bi->bmiHeader.biHeight; 

			if (bi.ApplyFormat(hCaptureWindow)) {
				PTRACE(3, "PVidInp\tcapSetVideoFormat succeeded: "
					   << FormatTable[prefFormatIdx].colourFormat << ' '
					   << bi->bmiHeader.biWidth << "x" << bi->bmiHeader.biHeight
					   << " sz=" << bi->bmiHeader.biSizeImage);
			}
			else {
				PTRACE(1, "PVidInp\tcapSetVideoFormat failed: "
					   << FormatTable[prefFormatIdx].colourFormat << ' '
					   << bi->bmiHeader.biWidth << "x" << bi->bmiHeader.biHeight
					   << " sz=" << bi->bmiHeader.biSizeImage
					   << " - lastError=" << lastError);
			}
		} // for prefResizeIdx
	} // for prefFormatIdx

	if (running && !m_bRealStop)
		return Start();

	return PTrue;
}

DWORD PVideoInputDevice_DirectShowEx::GetError()
{
	return m_lastError;//dxGetLastError(hCaptureWindow);
}

PBoolean PVideoInputDevice_DirectShowEx::SetFrameRate(unsigned rate)
{
	if (!PVideoDevice::SetFrameRate(rate))
		return PFalse;

	PTRACE(5, "SetFrameRate");
//	PBoolean running = IsCapturing();
//	if (running)
//		InnerStop();

	if (!dxSetFrameRate(hCaptureWindow, rate)) {
//		lastError = ::GetLastError();
//		PTRACE(1, "PVidInp\tcapCaptureSetSetup: failed - " << lastError);
//		if (running && !m_bRealStop)
//			return Start();
		return PTrue;
	}
    
//	if (running && !m_bRealStop)
//		return Start();

	return PTrue;
}


PBoolean PVideoInputDevice_DirectShowEx::SetFrameSizeConverter(unsigned width, unsigned height, PBoolean bScaleNotCrop)
{
	PTRACE(5, "SetFrameSizeConverter");
	PBoolean running = IsCapturing();
	if (running)
		InnerStop();

	unsigned int nOldWidth, nOldHeight;
	GetFrameSize(nOldWidth, nOldHeight);

	if ( !SetFrameSize(width, height) ) {
		if (!converter)
			converter = PColourConverter::Create(colourFormat, colourFormat, width, height);
		if (!converter) {
			PTRACE(1, "PVidDev\tSetFrameSizeConverter Colour converter creation failed");
			goto setFrameSizeConverterError;
		}

		PTRACE(3,"PVidDev\tColour converter created for " << width << 'x' << height);

		PBoolean bSetSuccess = FALSE;
		PINDEX prefResizeIdx = 0;
		while (prefResizeIdx < PARRAYSIZE(prefResizeTable)) {
			if ((prefResizeTable[prefResizeIdx].dest_width == width) &&
				(prefResizeTable[prefResizeIdx].dest_height == height)) {

					if (SetFrameSize(prefResizeTable[prefResizeIdx].device_width,
						prefResizeTable[prefResizeIdx].device_height)) {
							PBoolean converterOK= converter->SetDstFrameSize(width, height, bScaleNotCrop);
							if (converterOK){
								frameWidth = width;
								frameHeight = height;
								PTRACE(4,"PVidDev\tSetFrameSizeConverter succceded for "
									<< prefResizeTable[prefResizeIdx].device_width << 'x'
									<< prefResizeTable[prefResizeIdx].device_height
									<< " --> " << width<< 'x' <<height);
								bSetSuccess = TRUE;
								break;
							}
							PTRACE(2,"PVidDev\tSetFrameSizeConverter FAILED for "
								<< prefResizeTable[prefResizeIdx].device_width << 'x'
								<< prefResizeTable[prefResizeIdx].device_height 
								<< " --> " << width << 'x' << height);	       
						}
				}    
				prefResizeIdx++;
		}
		//if( !bSetSuccess ) {
		//	// Failed to find a resolution the device can do so far, so try
		//	// using the maximum width and height it claims it can do.

		//	// QUESTION: DO WE WANT A MAX SIZE INSANITY CHECK HERE.

		//	unsigned minWidth, minHeight, maxWidth, maxHeight;
		//	GetFrameSizeLimits(minWidth, minHeight, maxWidth, maxHeight);

		//	if (SetFrameSize(maxWidth, maxHeight)){
		//		PTRACE(4,"PVidDev\tSuccess set hardware size to " << maxWidth << 'x' << maxHeight);
		//		if (converter->SetDstFrameSize(width, height, bScaleNotCrop)){
		//			PTRACE(3,"PVidDev\tSetFrameSizeConvert SUCCEEDED for " << width << 'x' << height);
		//			bSetSuccess = TRUE;
		//		}
		//	}
		//}
		if( !bSetSuccess ) {
			PTRACE(2,"PVidDev\tSetFrameSizeConverter FAILED for " << width << 'x' << height);
			goto setFrameSizeConverterError;
		}
	}
	if (running && !m_bRealStop)
		return Start();

	return PTrue;
setFrameSizeConverterError:
	SetFrameSize(nOldWidth, nOldHeight);
	if (running && !m_bRealStop)
		Start();

	return PFalse;
}

PBoolean PVideoInputDevice_DirectShowEx::SetFrameSize(unsigned width, unsigned height)
{
	//if((width >= PROTECT_HD_MIN_WIDTH || height >= PROTECT_HD_MIN_HEIGHT)) {
	//	if( !(deviceName *= "vfonscreen://screen:")) {
	//		PBoolean bFind = PTrue;//GetDongleVideo(TRUE);
	//		if( !bFind ) {
	//			unsigned int nOldWidth, nOldHeight;
	//			GetFrameSize(nOldWidth, nOldHeight);
	//			if((nOldWidth >= PROTECT_HD_MIN_WIDTH || nOldHeight >= PROTECT_HD_MIN_HEIGHT)) {
	//				SIZE pSizes[10];
	//				int nCount = GetSupportedSize(pSizes, 10);
	//				if( nCount > 0 && (pSizes[0].cx != width || pSizes[0].cy != height) ) {
	//					SetFrameSize(pSizes[0].cx, pSizes[0].cy);
	//				}
	//			}
	//			return FALSE;
	//		}
	//	}
	//}
	PTRACE(5, "SetFrameSize");
	PBoolean running = IsCapturing();
	if (running)
		InnerStop();

	unsigned int nOldWidth, nOldHeight;
	GetFrameSize(nOldWidth, nOldHeight);

	if( !dxSetFrameSize(hCaptureWindow, width, height) ) {
		lastError = ::GetLastError();
		PTRACE(1, "PVidInp\tdxSetFrameSize failed: "
			<< colourFormat << ' '
			<< width << "x" << height
			<< " - lastError=" << lastError);
		goto setSizeError;
	}

	PTRACE(3, "PVidInp\tdxSetFrameSize succeeded: "
		 << colourFormat << ' '
		 << width << "x" << height);
  
	// verify that the driver really took the frame size
	if (!VerifyHardwareFrameSize(width, height)) 
		goto setSizeError; 

	// frameHeight must be positive regardlesss of what the driver says
	if (0 > (int)height) 
		height = (unsigned)-(int)height;

	if (!PVideoDevice::SetFrameSize(width, height))
		goto setSizeError;

	if (running && !m_bRealStop)
		return Start();

	return PTrue;

setSizeError:
	if( nOldWidth != width || nOldHeight != height ) {
		SetFrameSize(nOldWidth, nOldHeight);
	}
setSizeError1:
	if (running && !m_bRealStop)
		Start();

	return PFalse;
}

int PVideoInputDevice_DirectShowEx::GetSupportedSize(SIZE* pSizes, int nCount)
{
	if( hCaptureWindow == NULL ) return 0;
	int nRetCount = dxGetSupportedSizes(hCaptureWindow, pSizes, nCount);
	for(int i = 0; pSizes && i< nRetCount; ++i) {
		pSizes[i].cx = (pSizes[i].cx / 16) * 16;
		//if( pSizes[i].cx == 88 ) {
		//	pSizes[i].cx = 80;
		//}
	}
	if( pSizes != 0 && nRetCount > 0 && nRetCount < nCount - 1) 
	{
		if( 1 ) {//pSizes[nRetCount-1].cx > 1024 && pSizes[nRetCount-1].cy > 768 ) {
			int nAddedCount  = 0;
			SIZE sizes[5];
			sizes[nAddedCount].cx = pSizes[nRetCount-1].cx / 4;
			sizes[nAddedCount].cy = pSizes[nRetCount-1].cy / 4;
			if( (sizes[nAddedCount].cx % 16) == 0 ) {
				nAddedCount++;
			}
			sizes[nAddedCount].cx = pSizes[nRetCount-1].cx / 2;
			sizes[nAddedCount].cy = pSizes[nRetCount-1].cy / 2;
			if( (sizes[nAddedCount].cx % 16) == 0 ) {
				nAddedCount++;
			}
			// 176 * 144
			sizes[nAddedCount].cx = 176;
			sizes[nAddedCount].cy = 144;
			if( (sizes[nAddedCount].cx % 16) == 0 ) {
				nAddedCount++;
			}
			// 352 * 288
			sizes[nAddedCount].cx = 352;
			sizes[nAddedCount].cy = 288;
			if( (sizes[nAddedCount].cx % 16) == 0 ) {
				nAddedCount++;
			}

			// 704 * 576
			sizes[nAddedCount].cx = 704;
			sizes[nAddedCount].cy = 576;
			if( (sizes[nAddedCount].cx % 16) == 0 ) {
				nAddedCount++;
			}
//			sizes[2].cx = pSizes[nRetCount-1].cx * 2;
//			sizes[2].cy = pSizes[nRetCount-1].cy * 2;
			for( int l = 0; l < nAddedCount; ++l ) {
				int k = -1;
				for( int j = 0; j <nRetCount; j++ ){
					if( sizes[l].cx ==  pSizes[j].cx && sizes[l].cy ==  pSizes[j].cy ) {
						break;
					}
					if( sizes[l].cx > pSizes[j].cx ) {
						k = j;
					} else if( sizes[l].cx == pSizes[j].cx) {
						if( sizes[l].cy > pSizes[j].cy ) {
							k = j;
						}
					}
				}
				if( j == nRetCount) {
					for( int j = nRetCount-1; j >= k+1; j--) {
						pSizes[j+1].cx = pSizes[j].cx;
						pSizes[j+1].cy = pSizes[j].cy;
					}
					pSizes[k+1].cx = sizes[l].cx;
					pSizes[k+1].cy = sizes[l].cy;
					nRetCount++;
				}
			}
		
		}
		if(/*!GetDongleVideo(FALSE)*/ true && !(deviceName *= "vfonscreen://screen:"))
		{
			for(int m = 0; m< nRetCount;m++)
			{
				//if(pSizes[m].cx>=PROTECT_HD_MIN_WIDTH||pSizes[m].cy>=PROTECT_HD_MIN_HEIGHT)
				//{
				//	for(int n=m; n <nRetCount-1; ++n ) {
				//		pSizes[n].cx = pSizes[n+1].cx;
				//		pSizes[n].cy = pSizes[n+1].cy;
				//	}
				//	--nRetCount;
				//	--m;
				//}
			}
		}
	}
	return nRetCount;
}

int PVideoInputDevice_DirectShowEx::GetSupportedFormat(DWORD* pFormats, int nCount)
{
	if( hCaptureWindow == NULL ) return 0;
	return dxGetSupportedFormats(hCaptureWindow, pFormats, nCount);
}

//return TRUE if absolute value of height reported by driver 
//  is equal to absolute value of current frame height AND
//  width reported by driver is equal to current frame width
PBoolean PVideoInputDevice_DirectShowEx::VerifyHardwareFrameSize(unsigned width, unsigned height)
{
	PDXCapStatus status(hCaptureWindow);

	if (!status.IsOK())
		return PFalse;

	if (width != status.uiImageWidth)
		return PFalse;

	if (0 > (int)height)
		height = (unsigned)-(int)height;

	if (0 > (int)status.uiImageHeight)
		status.uiImageHeight = (unsigned)-(int)status.uiImageHeight;

	return (height == status.uiImageHeight);
}


PBoolean PVideoInputDevice_DirectShowEx::SetColourFormat(const PString & colourFmt)
{
	PBoolean running = IsCapturing();
	if (running)
		InnerStop();

	PString oldFormat = colourFormat;

	if (!PVideoDevice::SetColourFormat(colourFmt)) {
		return PFalse;
	}

	if( !dxSetStringVideoFormat(hCaptureWindow, (LPCTSTR)colourFmt) ) {
		lastError = ::GetLastError();
		PTRACE(1, "PVidInp\tdxSetStringVideoFormat failed: "
			   << colourFormat << ' '
			   << " - lastError=" << lastError);
		if( !oldFormat.IsEmpty() )
			PVideoDevice::SetColourFormat(oldFormat);
		return PFalse;
	}
	PINDEX i = 0;
	while (FormatTable[i].colourFormat != NULL && !(colourFmt *= FormatTable[i].colourFormat))
		i++;

	PTRACE(3, "PVidInp\tdxSetStringVideoFormat succeeded: "
		 << colourFormat); 

	if (converter) {
		converter->SetVFlipState(FormatTable[i].vfwComp->askForVFlip); 
		PTRACE(4, "PVidInp\tSetColourFormat(): converter.doVFlip set to " << converter->GetVFlipState());
	}
      
	if (running && !m_bRealStop)
		return Start();

	return PTrue;
}

PBoolean PVideoInputDevice_DirectShowEx::SetTargetColourFormat(const PString & colourFmt, const PString & preferredFormat)
{
	if( !preferredFormat.IsEmpty() && colourFmt != preferredFormat ) {
		if( SetColourFormat(preferredFormat) ) {
			colourFormat = colourFmt;
			return PTrue;
		}
	}
	if( !SetColourFormat(colourFmt) ) {
		int nCount = GetSupportedFormat(NULL, 0);
		if(nCount>0 ) {
			DWORD* pFormats = new DWORD[nCount];
			for(int i = 0; i < nCount; ++i ) {
				pFormats[i] = 0;
			}
			GetSupportedFormat(pFormats, nCount);
			for (PINDEX prefFormatIdx = 0; prefFormatIdx < PARRAYSIZE(PreferredFormatTable); prefFormatIdx++) {
				if( PreferredFormatTable[prefFormatIdx].colourFormat == NULL ) continue;
				if( colourFmt != PreferredFormatTable[prefFormatIdx].colourFormat ) {
					for( int i = 0; i < nCount; ++i ) {
						if( pFormats[i] == PreferredFormatTable[prefFormatIdx].vfwComp->compression ) {
							if( SetColourFormat(PreferredFormatTable[prefFormatIdx].colourFormat) ) {
								colourFormat = colourFmt;//PreferredFormatTable[prefFormatIdx].colourFormat;
								delete[] pFormats;
								return PTrue;
							}
						}
					} 
				}
			} // for prefFormatIdx

			for (PINDEX prefFormatIdx = 0; prefFormatIdx < PARRAYSIZE(PreferredFormatTable); prefFormatIdx++) {
				if( PreferredFormatTable[prefFormatIdx].colourFormat == NULL ) continue;
				if( colourFmt != PreferredFormatTable[prefFormatIdx].colourFormat ) {
					for( int i = 0; i < nCount; ++i ) {
						if( pFormats[i] == PreferredFormatTable[prefFormatIdx].vfwComp->compression ) {
							break;
						}
					}
					if( i == nCount ) {
						if( SetColourFormat(PreferredFormatTable[prefFormatIdx].colourFormat) ) {
							colourFormat = colourFmt;//PreferredFormatTable[prefFormatIdx].colourFormat;
							delete[] pFormats;
							return PTrue;
						}
					}
				}
			} // for prefFormatIdx
			delete[] pFormats;
		} else {
			for (PINDEX prefFormatIdx = 0; prefFormatIdx < PARRAYSIZE(PreferredFormatTable); prefFormatIdx++) {
				if( PreferredFormatTable[prefFormatIdx].colourFormat == NULL ) continue;
				if( colourFmt != PreferredFormatTable[prefFormatIdx].colourFormat ) {
					if( SetColourFormat(PreferredFormatTable[prefFormatIdx].colourFormat) ) {
						colourFormat = colourFmt;//PreferredFormatTable[prefFormatIdx].colourFormat;
						return PTrue;
					}
				}
			} // for prefFormatIdx
		}
		return PFalse;
	}
	return PTrue;
}

PStringArray PVideoInputDevice_DirectShowEx::GetDeviceNames() const
{
	return PVideoInputDevice_DirectShowEx::GetInputDeviceNames();
}

PStringList PVideoInputDevice_DirectShowEx::GetInputDeviceNames()
{
	PStringList list;

	for (WORD devId = 0; devId < 10; devId++) {
    char name[100]={0}; 
		char version[200]={0};
		if (dxGetDriverDescription(devId, name, sizeof(name), version, sizeof(version)))
			list.AppendString(name);
	}

	return list;
}


PINDEX PVideoInputDevice_DirectShowEx::GetMaxFrameBytes()
{
	if (!IsOpen())
		return 0;

	PINDEX size = PDXVideoDeviceBitmap(hCaptureWindow)->bmiHeader.biSizeImage;
	if (converter != NULL && size < converter->GetMaxDstFrameBytes())
		return converter->GetMaxDstFrameBytes();

	return size;
}

void PVideoInputDevice_DirectShowEx::SetPause(PBoolean bPause)
{
	m_bPause = bPause;
}

PBoolean PVideoInputDevice_DirectShowEx::IsPause()
{
	return m_bPause;
}

PBoolean PVideoInputDevice_DirectShowEx::GetFrameData(BYTE * buffer, PINDEX * bytesReturned)
{
	if(isCapturingNow) {
		if( m_bPause ) {
#if defined(SUPPORT_TIME_STAMP)
		if (bytesReturned != NULL) {
			ULONG timeStamp = (ULONG)PTimer::Tick().GetInterval();
			buffer += *bytesReturned;
			memcpy(buffer, &timeStamp, sizeof(ULONG));
		}
#endif
			Sleep(1000);
			return PTrue;
		}
		
		return GetFrameDataNoDelay(buffer, bytesReturned);

	} else {
		PTRACE(5, "PVideoInputDevice_DirectShowEx Capture has stopped");
#if defined(SUPPORT_TIME_STAMP)
		if (bytesReturned != NULL) {
			ULONG timeStamp = (ULONG)PTimer::Tick().GetInterval();
			buffer += *bytesReturned;
			memcpy(buffer, &timeStamp, sizeof(ULONG));
		}
#endif
		return PTrue;
	}
}


PBoolean PVideoInputDevice_DirectShowEx::GetFrameDataNoDelay(BYTE * buffer, PINDEX * bytesReturned)
{
	//PTRACE(1, "GetFrameData Wait: "<< PTimer::Tick() );
//	if (!frameAvailable.Wait(1000))
	if( ::WaitForSingleObject(m_hSemaphore, 1000) == WAIT_TIMEOUT ) {
#if defined(SUPPORT_TIME_STAMP)
		if (bytesReturned != NULL) {
			ULONG timeStamp = (ULONG)PTimer::Tick().GetInterval();
			buffer += *bytesReturned;
			memcpy(buffer, &timeStamp, sizeof(ULONG));
		}
#endif
		return PTrue;
	}

	ISampleBuffer* pData = NULL;
	m_csBuffers.Lock();
	if( m_lstBuffers.size() > 0 ) {
		pData = m_lstBuffers.front();
		m_lstBuffers.pop_front();
	}
	m_csBuffers.Unlock();

	if( pData ) {
		BYTE* framePtr = NULL;
		pData->GetSample()->GetPointer(&framePtr);
		int nSize = pData->GetSample()->GetSize();
		if( framePtr ) {
			//PString frameFormat = colourFormat;//(LPCTSTR)pData->GetVideoFormat();
      char cframeFormat[256]={0};
			pData->GetVideoFormat(cframeFormat, sizeof(cframeFormat));
      PString frameFormat = cframeFormat;
      if( frameFormat *= "I420" ) frameFormat = "YUV420P";
      if( colourFormat *= "I420" ) colourFormat = "YUV420P"; 
			if( frameFormat != colourFormat ) {
				unsigned nSrcWidth = 0, nSrcHeight = 0;
				PTRACE(5, "lastFrameFormat=" << frameFormat <<", colourFormat=" << colourFormat);
				if( converter != NULL && frameFormat != converter->GetSrcColourFormat() ) {
					PTRACE(1, "GetSrcColourFormat=" << converter->GetSrcColourFormat() << ", Now=" << frameFormat);
					converter->GetSrcFrameSize(nSrcWidth, nSrcHeight);
					delete converter;
					converter = NULL;
				}
				if( converter == NULL ) {
					dxGetFrameSize(hCaptureWindow, &nSrcWidth, &nSrcHeight);
					converter = PColourConverter::Create(frameFormat, colourFormat, frameWidth, frameHeight);
					PINDEX i = 0;
					while (FormatTable[i].colourFormat != NULL && !(frameFormat *= FormatTable[i].colourFormat))
						i++;
					if( (i < PARRAYSIZE(FormatTable)) && converter) {
						converter->SetVFlipState(FormatTable[i].vfwComp->askForVFlip); 
					}
				}
				if( converter ) {
					AM_MEDIA_TYPE *pmt;
					HRESULT hr = pData->GetSample()->GetMediaType(&pmt);
					if( SUCCEEDED(hr) && pmt) {
						if( pmt->formattype == FORMAT_VideoInfo ) {
							nSrcWidth = (((VIDEOINFOHEADER *) (pmt->pbFormat))->bmiHeader).biWidth;
							nSrcHeight = abs((((VIDEOINFOHEADER *) (pmt->pbFormat))->bmiHeader).biHeight);
						} else if( pmt->formattype == FORMAT_VideoInfo2 ) {
							nSrcWidth = (((VIDEOINFOHEADER2 *) (pmt->pbFormat))->bmiHeader).biWidth;
							nSrcHeight = abs((((VIDEOINFOHEADER2 *) (pmt->pbFormat))->bmiHeader).biHeight);
						}
						//PTRACE(1, "Converter = " << nSrcWidth << ", " << nSrcHeight);
						::BBQDeleteMediaType(pmt);
					}
					if( nSrcWidth != 0 && nSrcHeight != 0) {
						converter->SetSrcFrameSize(nSrcWidth, nSrcHeight);
						//converter->SetDstFrameSize(nSrcWidth, nSrcHeight, FALSE);
					}
				}
			} else {
				if( converter != NULL ) {
					delete converter;
					converter = NULL;
				}
			}
			//if (NULL != converter) {
			//	nSize = converter->GetMaxDstFrameBytes();
			//}			
			//if( bytesReturned != NULL && *bytesReturned < nSize) {
			//	pData->Release();
			//	*bytesReturned = nSize;
			//	m_lstBuffers.push_front(pData);
			//	::ReleaseSemaphore(m_hSemaphore, 1, NULL);
			//}

			if (NULL != converter) {
				converter->Convert(framePtr, buffer, &nSize);
			} else {
				if (bytesReturned != NULL && *bytesReturned != 0 ) {
					if( *bytesReturned >= nSize ) {
						memcpy(buffer, framePtr, nSize);
					} else {
						nSize = 0;
						//ASSERT(FALSE);
					}
				}else {
					memcpy(buffer, framePtr, nSize);
				}
			}
			if (bytesReturned != NULL)
				*bytesReturned = nSize;

#if defined(SUPPORT_TIME_STAMP)
			ULONG timeStamp = (ULONG)pData->GetSampleTime();
			buffer += nSize;
			memcpy(buffer, &timeStamp, sizeof(ULONG));
#endif
		} else {
			if (bytesReturned != NULL)
				*bytesReturned = 0;
		}
		pData->Release();
	} else {
		PTRACE(2, "NoData");
		if (bytesReturned != NULL)
			*bytesReturned = 0;
	}

#if PTRACING
	static DWORD first = timeGetTime();
	static int i = 0;
	DWORD end = timeGetTime();
	if(end-first >= 1000 ) {
		PString strText;
    strText.sprintf("Frames 3: %d", i);
    //strText = PString(PString::Printf,("Frames 3: %d\n", i));
		PTRACE(3, strText);
		i = 0;
		first = end;
	}
	i++;
#endif
	
	return PTrue;
}


LRESULT CALLBACK PVideoInputDevice_DirectShowEx::DXErrorHandler(HWND hWnd, int id, LPCSTR err)
{
	if (hWnd == NULL)
		return FALSE;

	return ((PVideoInputDevice_DirectShowEx *)dxGetUserData(hWnd))->DXHandleError(id, err);
}


LRESULT PVideoInputDevice_DirectShowEx::DXHandleError(int id, LPCSTR err)
{
	if (id != 0) {
		PTRACE(1, "PVidInp\tErrorHandler: [id="<< id << "] " << err);
	}

	return TRUE;
}

LRESULT CALLBACK PVideoInputDevice_DirectShowEx::DXVideoHandler(HWND hWnd, const ISampleBuffer* pData)
{

	if (hWnd == NULL)
		return FALSE;

	return ((PVideoInputDevice_DirectShowEx *)dxGetUserData(hWnd))->DXHandleVideo(pData);
}


LRESULT PVideoInputDevice_DirectShowEx::DXHandleVideo(const ISampleBuffer* pInputData)
{
	//PTRACE(1, "HandleVideo Begin: "<< PTimer::Tick() );
	m_csBuffers.Lock();
	if( isCapturingNow ) {
		PBoolean bNeedReleaseSemaphore = TRUE;
		if( m_lstBuffers.size() >= m_nBuffersSize ) {
			ISampleBuffer* pData = m_lstBuffers.front();
			m_lstBuffers.pop_front();
			pData->Release();
			bNeedReleaseSemaphore = FALSE;
		}

		ISampleBuffer* pData = pInputData->Clone();
		m_lstBuffers.push_back(pData);

		if( bNeedReleaseSemaphore )
			::ReleaseSemaphore(m_hSemaphore, 1, NULL);
	}
	m_csBuffers.Unlock();

#if PTRACING
	static DWORD first = timeGetTime();
	static int i = 0;
	DWORD end = timeGetTime();
	if(end-first >= 1000 ) {
		PString strText;
    strText.sprintf("Frames 2: %d", i);
		//strText= PString(PString::Printf,("Frames 2: %d\n", i));
		PTRACE(3, strText);
		i = 0;
		first = end;
	}
	i++;
#endif

	return TRUE;
}


static LRESULT FAR PASCAL DXStatusHandle(HWND hWnd, int nID, LPSTR lpStatusText)
{
	PVideoInputDevice_DirectShowEx* pDevice = (PVideoInputDevice_DirectShowEx *)dxGetUserData(hWnd);
	return pDevice->DXHandleStatus(nID, lpStatusText);
}

LRESULT PVideoInputDevice_DirectShowEx::DXHandleStatus(int nID, LPSTR lpStatusText)
{
    if(nID==IDS_CAP_END){
    	cameraStopMutex.Signal();
    }
    PTRACE(1, lpStatusText);

    return (LRESULT) TRUE ;
}

PBoolean PVideoInputDevice_DirectShowEx::Win32InitialiseCapture(PBoolean bOpenOnly)
{
	if ((hCaptureWindow = dxCreateCaptureWindow("Capture Window",
									   WS_POPUP | WS_CAPTION,
									   CW_USEDEFAULT, CW_USEDEFAULT,
									   frameWidth + GetSystemMetrics(SM_CXFIXEDFRAME),
									   frameHeight + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFIXEDFRAME),
									   (HWND)0,
									   0)) == NULL) {
		lastError = ::GetLastError();
		PTRACE(1, "PVidInp\tcapCreateCaptureWindow failed - " << lastError);
		return PFalse;
	}

	dxSetCallbackOnError(hCaptureWindow, DXErrorHandler);
	dxSetCallbackOnStatus(hCaptureWindow, DXStatusHandle);
	if (!dxSetCallbackOnVideoStream(hCaptureWindow, DXVideoHandler)) {
		lastError = ::GetLastError();
		PTRACE(1, "PVidInp\tcapSetCallbackOnVideoStream failed - " << lastError);
		return PFalse;
	}

	WORD devId;
	if (PTrace::CanTrace(6)) { // list available video capture drivers
		PTRACE(5, "PVidInp\tEnumerating available video capture drivers");
		for (devId = 0; devId < 10; devId++) { 
      char name[100]={0};
      char version[200]={0};
		  if (dxGetDriverDescription(devId, name, sizeof(name), version, sizeof(version)) ) 
				PTRACE(5, "PVidInp\tVideo device[" << devId << "] = " << name << ", " << version);
		}
	}
	if (deviceName.GetLength() == 1 && isdigit(deviceName[0])) {
		devId = (WORD)(deviceName[0] - '0');
		if(!dxDriverConnect(hCaptureWindow,devId)) 
			return PFalse;
		
		char szName[256];
		dxDriverGetName(hCaptureWindow, szName, 255);
		deviceName = szName;
	}
	else {
		PString strDevice((LPCTSTR)deviceName);
		int nPos = strDevice.Find("vfoncameratype://");
		if( nPos == 0 ) {
			PString strIPType, strIP;
			SplitIPCameraType(strDevice, strIPType, strIP);
			strDevice = GetIPCameraUrl(strIPType, strIP);;
		}
		
		if(!dxDriverConnectByName(hCaptureWindow,(LPCTSTR)strDevice))  { 
			if( bOpenOnly ) {
				return PFalse;
			} else {
				for (devId = 0; devId < 10; devId++) {
					if(dxDriverConnect(hCaptureWindow,devId)) 
						break;  
				}
				if( devId == 10 ) return FALSE;
				char szName[256];
				dxDriverGetName(hCaptureWindow, szName, 255);
				deviceName = szName;
			}
		}
	}
	int nWidth, nHeight;
	if( dxGetFrameSize(hCaptureWindow, &nWidth, &nHeight) ) {
		frameWidth = nWidth;
		frameHeight = nHeight;
	}

	dxSetUserData(hCaptureWindow, this);

	dxPreview(hCaptureWindow, FALSE);

//	if (!SetFrameRate(frameRate))
	//if (!SetFrameRate(10))
	//	return FALSE;

	SetFrameRate(10);

	if (preferredColourFormat.IsEmpty()) {
		if( colourFormat.IsEmpty() )
			return PTrue;
		else {
			SetColourFormat(colourFormat);
			return PTrue;
		}
	}
	SetColourFormat(preferredColourFormat);
	return PTrue;
}

PBoolean PVideoInputDevice_DirectShowEx::Win32HandleCapture(PBoolean bOpenOnly)
{
	m_lastError = 0;
	PBoolean initSucceeded = PFalse;
	try {
		initSucceeded = Win32InitialiseCapture(bOpenOnly);

		if( !initSucceeded ) {
			if( hCaptureWindow != NULL ) 
				m_lastError = dxGetLastError(hCaptureWindow);
			Win32CaptureThreadEnd();
		}
	} catch(...) {
		Win32CaptureThreadEnd();
		initSucceeded = PFalse;
		m_lastError = ERROR_DXCAMERA_NONE;
	}
	threadStarted.Signal(); 


	return initSucceeded ;
}

PBoolean PVideoInputDevice_DirectShowEx::Win32CaptureThreadEnd()
{
	if( hCaptureWindow == NULL ) return PTrue;

	PTRACE(5, "PVidInp\tDisconnecting driver");
	dxSetUserData(hCaptureWindow, NULL);

	dxSetCallbackOnError(hCaptureWindow, NULL);
	dxSetCallbackOnVideoStream(hCaptureWindow, NULL);
	dxSetCallbackOnStatus(hCaptureWindow, NULL);

	dxDriverDisconnect(hCaptureWindow);
	PTRACE(5, "PVidInp\tDestroying VIDCAP window");
	DestroyWindow(hCaptureWindow);
	hCaptureWindow = NULL;

	/*********zhx**** *****************/
	DestroyBuffers();
	return PTrue;
}

PBoolean PVideoInputDevice_DirectShowEx::IsAplux()
{
	//USHORT vendorId, productId;
	//PBoolean bRet = GetCameraId(vendorId, productId);
	//if( bRet ) {
	//	bRet = (vendorId == APLUX_ID_VENDOR);
	//}
	//return bRet;
	return PFalse;
}

PBoolean PVideoInputDevice_DirectShowEx::GetCameraId(USHORT& vendorId, USHORT& productId)
{
	return dxGetCameraID(hCaptureWindow, &vendorId, &productId) == TRUE;
}

PBoolean PVideoInputDevice_DirectShowEx::HasOptionDialog(int nDialog)
{
	return dxHasDialog(hCaptureWindow, nDialog) == TRUE;
}

PBoolean PVideoInputDevice_DirectShowEx::ShowDialog(int nDialog, HWND hParent)
{
	return dxShowDialog(hCaptureWindow, nDialog, hParent) ==TRUE;
/*
	PDXCamera* pCamera = (PDXCamera*)dxGetKernel(hCaptureWindow);
	if( pCamera ) {
		return pCamera->ShowDialog(nDialog, hParent);
	}
	return FALSE;
	*/
}

void PVideoInputDevice_DirectShowEx::DestroyBuffers()
{

	//lastFrameMutex.Wait();
	//if(lastFramePtr)
	//	delete[] lastFramePtr;
	//lastFramePtr = NULL;
	//lastFrameSize = 0;
	//lastFrameMutex.Signal();
	m_csBuffers.Lock();
	while( m_lstBuffers.size() > 0 ) {
		ISampleBuffer* pData = m_lstBuffers.front();
		m_lstBuffers.pop_front();
		pData->Release();
		::WaitForSingleObject(m_hSemaphore, 0);
	}
	m_csBuffers.Unlock();
}

void PVideoInputDevice_DirectShowEx::SetBuffersSize(int nBuffersSize)
{
	ASSERT( nBuffersSize > 0 );
	m_csBuffers.Lock();
	m_nBuffersSize = nBuffersSize;
	if( m_nBuffersSize < 1 ) {
		m_nBuffersSize = 1;
	}
	while( m_lstBuffers.size() > nBuffersSize ) {
		ISampleBuffer* pData = m_lstBuffers.front();
		m_lstBuffers.pop_front();
		pData->Release();
	}
	ReleaseSemaphore(m_hSemaphore, 1, NULL);
	CloseHandle(m_hSemaphore);
	m_hSemaphore  = ::CreateSemaphore(NULL, m_lstBuffers.size(), m_nBuffersSize, NULL);
	m_csBuffers.Unlock();
}

PBoolean PVideoInputDevice_DirectShowEx::GetSimliarSize(unsigned& nWidth, unsigned& nHeight)
{
	PBoolean bRet = dxGetSimilarSize(hCaptureWindow, &nWidth, &nHeight);
	if( bRet ) {
		nWidth = (nWidth / 16) * 16;
		//if((nWidth >= PROTECT_HD_MIN_WIDTH || nHeight >= PROTECT_HD_MIN_HEIGHT)) {
		//	if( !(deviceName *= "vfonscreen://screen:")) {
		//		PBoolean bFind = PTrue;//GetDongleVideo(TRUE);
		//		if( !bFind ) {
		//			SIZE pSizes[10];
		//			int nCount = GetSupportedSize(pSizes, 10);
		//			if( nCount > 0 && (pSizes[0].cx != nWidth || pSizes[0].cy != nHeight) ) {
		//				nWidth = pSizes[0].cx;
		//				nHeight = pSizes[0].cy;
		//			}
		//		}
		//	}
		//}
	}
	return bRet;
}

int PVideoInputDevice_DirectShowEx::GetCameraProperties(LPTSTR pszProperties, int nLen)
{
	if( hCaptureWindow != NULL ) {
		return dxGetProperties(hCaptureWindow, pszProperties, nLen) ==TRUE;
	}
	return 0;
}

PBoolean PVideoInputDevice_DirectShowEx::SetCameraProperties(LPCTSTR lpszProperties)
{
	if( hCaptureWindow != NULL ) {
		return dxSetProperties(hCaptureWindow, lpszProperties) ==TRUE;
	}
	return PFalse;
}

PBoolean PVideoInputDevice_DirectShowEx::GetCameraProperty(LPCTSTR lpszProperty, LPTSTR pszValue)
{
	if( hCaptureWindow != NULL ) {
		if( stricmp(lpszProperty, "HighDefinition") == 0 ) {
			if( deviceName *= "vfonscreen://screen:" ) return FALSE;
			///if( !GetDongleVideo(FALSE) ) return FALSE;
		}
		return dxGetProperty(hCaptureWindow, lpszProperty, pszValue) ==TRUE;
	}
	return PFalse;
}

PBoolean PVideoInputDevice_DirectShowEx::SetCameraProperty(LPCTSTR lpszProperty, LPCTSTR lpszValue)
{
	if( hCaptureWindow != NULL ) {
		return dxSetProperty(hCaptureWindow, lpszProperty, lpszValue) == TRUE;
	}
	return PFalse;
}

#endif // P_DIRECTSHOW
#endif