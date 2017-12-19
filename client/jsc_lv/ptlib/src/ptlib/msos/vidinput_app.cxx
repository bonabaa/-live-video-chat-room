/* vidinput_app.cxx
 *
 *
 * Application Implementation for the PTLib Project.
 *
 * Copyright (c) 2007 ISVO (Asia) Pte Ltd. All Rights Reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 *
 *
 * Contributor(s): Craig Southeren, Post Increment (C) 2008
 *
 * $Revision: 20385 $
 * $Author: rjongbloed $
 * $Date: 2008-06-04 10:40:38 +0000 (Wed, 04 Jun 2008) $
 */


#include <ptlib.h>

#if P_APPSHARE

#include <ptlib/vidinput_app.h>
#include <ptlib/vconvert.h>
extern bool  g_CanTransmitVideo;

namespace PWLibStupidLinkerHacks {
   int loadAppShareStuff;
};
static const char TopWindowPrefix[] = "TopWindow:";
static const char SceenCapturePrefix[] = "Screen Capture";
static const PINDEX TopWindowPrefixLen = sizeof(TopWindowPrefix)-1;

//////////////////////////////////////////////////////////////////////////////////////
#if 0
class PVideoInputDevice_Application_PluginServiceDescriptor : public PDevicePluginServiceDescriptor
{
  public:
    virtual PObject *    CreateInstance(int /*userData*/) const { return new PVideoInputDevice_Application; }

    virtual PStringArray GetDeviceNames(int /*userData*/) const { return PVideoInputDevice_Application::GetInputDeviceNames(); }

    virtual bool         GetDeviceCapabilities(const PString & deviceName, void * caps) const
      { return PVideoInputDevice_Application::GetDeviceCapabilities(deviceName, (PVideoInputDevice::Capabilities *)caps); }

    virtual bool ValidateDeviceName(const PString & deviceName, int /*userData*/) const
    {
	#if 0
				return (deviceName.Left(10) *= "appwindow:") && (FindWindow(NULL, deviceName.Mid(10)) != NULL) ;
	#else
				return (deviceName *= "Desktop") ||((deviceName.Left(10) *= "appwindow:") && (FindWindow(NULL, deviceName.Mid(10)) != NULL))||
							((deviceName.Left(TopWindowPrefixLen) *= TopWindowPrefix) && (FindWindow(NULL, deviceName.Mid(TopWindowPrefixLen)) != NULL));
	#endif
    }


} PVideoInputDevice_Application_descriptor;
PCREATE_VIDINPUT_PLUGIN(Application);
#else
PCREATE_VIDINPUT_PLUGIN(Application/*, PVideoInputDevice, &PVideoInputDevice_Application_descriptor*/);
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Input device

PVideoInputDevice_Application::PVideoInputDevice_Application()
{
  m_hWnd                = NULL;
  m_client              = true;
  memset(&m_rectCapture, 0, sizeof(m_rectCapture));
  SetColourFormat("BGR24"/*32*/);
  SetFrameRate(10);
	m_nFullWidth=GetSystemMetrics(SM_CXSCREEN); m_nFullHeight=GetSystemMetrics(SM_CYSCREEN); 
  m_bDsiablesync =true;
}

PVideoInputDevice_Application::~PVideoInputDevice_Application()
{
  Close();
}

PStringArray PVideoInputDevice_Application::GetDeviceNames() const
{ 
  return GetInputDeviceNames(); 
}
//static BOOL CALLBACK AddWindowName(HWND hWnd, LPARAM userData)
//{
//  CHAR name[200];
//  if (IsWindowVisible(hWnd) && GetWindowText(hWnd, name, sizeof(name))) {
//    PStringArray * names = (PStringArray *)userData;
//    names->AppendString(PString(TopWindowPrefix) + name);
//  }
//  return TRUE;
//}

PStringArray PVideoInputDevice_Application::GetInputDeviceNames()
{
  PStringArray names;

  names += SceenCapturePrefix;

  //::EnumWindows(AddWindowName, (LPARAM)&names);

  return names;
}

PBoolean PVideoInputDevice_Application::GetDeviceCapabilities(const PString & /*deviceName*/, Capabilities * /*caps*/)  
{ 
  return false; 
}

PBoolean PVideoInputDevice_Application::Open(const PString & dn, PBoolean  startImmediate )
{
  Close();
  m_strDeviceName = deviceName= dn;
  m_client = false;
  //if (startImmediate == false)
  //  return true;

 
  return true;
}

PBoolean PVideoInputDevice_Application::IsOpen()
{
  return m_hWnd != NULL;
}

PBoolean PVideoInputDevice_Application::Close()
{
  m_syncVideoReady.Signal();
  if (!IsOpen())
    return false;

  m_hWnd = NULL;
  return true;
}

PBoolean PVideoInputDevice_Application::Start()
{
 RECT rect;
  memset(&rect, 0, sizeof(rect));  // needed to avoid compiler warning

  if (m_hWnd == NULL) {
    if (deviceName.Left(10) *= "appwindow:") {
      m_hWnd = FindWindow(NULL, deviceName.Mid(10));
      if (m_hWnd != NULL) {
        ::GetWindowRect(m_hWnd, &rect);
        SetFrameSize(rect.right-rect.left, rect.bottom-rect.top);
      }
    }else if (deviceName.Left(TopWindowPrefixLen) *= TopWindowPrefix)
			m_hWnd = FindWindow(NULL, deviceName.Mid(TopWindowPrefixLen));
		else if (deviceName *= "desktop")
			m_hWnd = GetDesktopWindow();
		else if (deviceName*= SceenCapturePrefix)
		{
			PString deviceName2 = "appwindow:Program Manager";
			if (deviceName2.Left(10) *= "appwindow:") {
				m_hWnd = FindWindow(NULL, deviceName2.Mid(10));
				if (m_hWnd != NULL) {
					::GetWindowRect(m_hWnd, &rect);
					SetFrameSize(rect.right-rect.left, rect.bottom-rect.top);
				}
			}
		}

  }

  if (m_hWnd == NULL) {
    PTRACE(4,"AppInput/tOpen Fail no Window to capture specified!");
    return false;
  }

  return true;
}

PBoolean PVideoInputDevice_Application::Stop()
{
  m_hWnd= NULL;
  return true;
}

PBoolean PVideoInputDevice_Application::IsCapturing()
{
  return IsOpen();
}
 
PBoolean PVideoInputDevice_Application::SetVideoFormat(VideoFormat newFormat)
{
  return PVideoDevice::SetVideoFormat(newFormat);
}

PBoolean PVideoInputDevice_Application::SetColourFormat(const PString & colourFormat)
{
  if ((colourFormat *= "BGR32") || (colourFormat *= "BGR24")  )
    return PVideoDevice::SetColourFormat(colourFormat);

  return PFalse;
}

PBoolean PVideoInputDevice_Application::SetFrameRate(unsigned Rate)
{
  return PVideoDevice::SetFrameRate(2/*Rate*/);
}

PBoolean PVideoInputDevice_Application::SetFrameSize(unsigned width, unsigned height)
{
 RECT DesktoPrect; /* temporary variable */
  /* Next function copies the dimensions of 
  the bounding rectangle of the desktop */

 #if 0
  m_rectCapture.bottom =m_rectCapture.top+ height;
  m_rectCapture.right  = m_rectCapture.left + width;
#else
  HWND hDesktop =  ::GetDesktopWindow();
  ::GetWindowRect( hDesktop,&DesktoPrect);
  //if (m_bFirstSetFrameSize){
  //  m_bFirstSetFrameSize= false;
  //  m_rectCapture.left   = DesktoPrect.right- DesktoPrect.left - width;
  //  m_rectCapture.right  = DesktoPrect.bottom-DesktoPrect.top - height;
  //}else{
  //  //m_rectCapture.left   = DesktoPrect.right- DesktoPrect.left - width;
  //  //m_rectCapture.right  = DesktoPrect.bottom-DesktoPrect.top - height;
  //}
  m_rectCapture.right  = m_rectCapture.left + width;
  m_rectCapture.bottom = m_rectCapture.top+ height;
#endif
  return PVideoDevice::SetFrameSize(width, height);
}

PINDEX PVideoInputDevice_Application::GetMaxFrameBytes()
{
  return GetMaxFrameBytesConverted(CalculateFrameBytes(frameWidth, frameHeight, colourFormat));
}

PBoolean PVideoInputDevice_Application::GetFrameData(
      BYTE * buffer,                 
      PINDEX * bytesReturned   
    )
{
  
  //if (  g_CanTransmitVideo ==false){
  //  *bytesReturned=0;
  // // m_bDsiablesync= true; 
  //// m_syncVideoReady.Wait();
  //  PThread::Sleep(1000);
  //  return true;
  //}
  //else if (g_CanTransmitVideo == true && m_bDsiablesync == true){
  // // m_syncVideoReady.Signal();m_bDsiablesync = false;
  //}
  if (IsCapturing()==false)
  {
    *bytesReturned=0;
    return true;
  }
    grabDelay.Delay(1000/GetFrameRate());
    return GetFrameDataNoDelay(buffer,bytesReturned);
}

static inline WORD GetNumberOfColours(WORD bitsPerPixel)
{
    // only 1, 4 and 8bpp bitmaps use palettes (well, they could be used with
    // 24bpp ones too but we don't support this as I think it's quite uncommon)
    return (WORD)(bitsPerPixel <= 8 ? 1 << bitsPerPixel : 0);
}
 PBoolean PVideoInputDevice_Application::GetFrameDataNoDelay(BYTE * buffer, PINDEX * bytesReturned)
{
  PWaitAndSignal m(lastFrameMutex);
#ifdef _WIN32
 Sleep(5);//CPU 100% in mediapatch
#endif
  //PTRACE(6,"AppInput\tGrabbing Frame");
  if (m_hWnd == NULL) return true;
 const RECT& _rect = m_rectCapture;

  // Get the client area of the window
  if (m_client) {

    //::GetClientRect(m_hWnd, &_rect);
    int width  = _rect.right - _rect.left;
    int height = _rect.bottom - _rect.top;
    if ((width != (int)frameWidth) || (height != (signed)frameHeight))
      PVideoDevice::SetFrameSize(width, height);

    POINT pt1;
    pt1.x = _rect.left;
    pt1.y = _rect.top;
    ::ClientToScreen(m_hWnd, &pt1);

    POINT pt2;
    pt2.x = _rect.right;             
    pt2.y = _rect.bottom;             
    ::ClientToScreen(m_hWnd, &pt2);

    //_rect.left   = pt1.x;             
    //_rect.top    = pt1.y;             
    //_rect.right  = pt2.x;             
    //_rect.bottom = pt2.y;  
  }
  else
  {
    //::GetWindowRect(m_hWnd, &_rect);
    //_rect.bottom = _rect.top + frameHeight;
    //_rect.right  = _rect.left + frameWidth;
  }

  bool retVal = true;

  if (!IsRectEmpty(&_rect)) {

    // create a DC for the screen and create
    // a memory DC compatible to screen DC
    HDC hScrDC = CreateDC("DISPLAY", NULL, NULL, NULL);
    HDC hMemDC = CreateCompatibleDC(hScrDC);

    // get borders of grab area in pixels
    int left   = _rect.left;     
    int top    = _rect.top;     
    int right  = _rect.right;     
    int bottom = _rect.bottom;

    // get size of screen in pixels
    //int xScrn = GetDeviceCaps(hScrDC, HORZRES);
    //int yScrn = GetDeviceCaps(hScrDC, VERTRES);

#if 0
    // make sure bitmap rectangle is visible
    if (left < 0)
      nX = 0;
    if (nY < 0)
      nY = 0;
    if (nX2 > xScrn)
      nX2 = xScrn;
    if (nY2 > yScrn)
      nY2 = yScrn;
#endif

    // get width and height of grab region
    int nWidth  = right - left;
    int nHeight = bottom - top;

    HBITMAP hBitMap;
    {
       // create a bitmap compatible with the screen DC
      HGDIOBJ _hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);
   
      // select new bitmap into memory DC
      HGDIOBJ _hOldBitmap = SelectObject(hMemDC, _hBitmap);
  
      // bitblt screen DC to memory DC     
			if (m_nFullWidth > nWidth&& m_nFullHeight>nHeight )
        BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC,_rect.left, _rect.top, SRCCOPY);
			else
				BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC, left, top, SRCCOPY);   
   
      POINT point;

      if ( GetCursorPos(&point) 
        && point.x > m_rectCapture.left  &&  point.x < m_rectCapture.right
         && point.y > m_rectCapture.top  &&  point.y < m_rectCapture.bottom)
      {
#if 0
       // http://msdn2.microsoft.com/en-us/library/ms648388(VS.85).aspx
        //CURSORINFO info;
        //GetCursorInfo(&info);
        HCURSOR hico =  GetCursor();// info.hCursor;
        if (hico== NULL){
          hico = 0;
        }
//        BOOL b= DrawIcon( hMemDC, point.x-m_rectCapture.left , point.y-m_rectCapture.top,hico);
        hico =0;
#else
        HCURSOR hCursor = GetCursor();  
     POINT ptCursor;  
     GetCursorPos(&ptCursor);  
     ICONINFO IconInfo = {0};  
     if(GetIconInfo(hCursor, &IconInfo))  
     {  
         ptCursor.x -= IconInfo.xHotspot;  
         ptCursor.y -= IconInfo.yHotspot;  
         if(NULL != IconInfo.hbmMask)  
             DeleteObject(IconInfo.hbmMask);  
         if(NULL != IconInfo.hbmColor)  
             DeleteObject(IconInfo.hbmColor);  
    }  
     DrawIconEx(hMemDC, ptCursor.x, ptCursor.y, hCursor, 0, 0, 0, NULL, DI_NORMAL | DI_COMPAT);  
#endif
      }
    

      // select old bitmap back into memory DC and get handle to     
      // bitmap of the screen
      hBitMap = (HBITMAP)SelectObject(hMemDC, _hOldBitmap);
    }

    // get the bitmap information
    BITMAP bitmap;
    if (GetObject(hBitMap, sizeof(BITMAP), (LPSTR)&bitmap) == 0) {
      PTRACE(2, "AppInput\tCould not get bitmap information");
      retVal = false;
    }
    else
    {
      // create a BITMAPINFO with enough room for the pixel data
      unsigned pixelOffs = sizeof(BITMAPINFOHEADER) + GetNumberOfColours(bitmap.bmBitsPixel) * sizeof(RGBQUAD);
      LPBITMAPINFO bitmapInfo = (LPBITMAPINFO)bitMapInfoStorage.GetPointer(pixelOffs + (bitmap.bmHeight * bitmap.bmWidthBytes));
      BITMAPINFOHEADER & bi = bitmapInfo->bmiHeader;
      memset(&bi, 0, sizeof(bi));

      bi.biSize        = sizeof(BITMAPINFOHEADER);
      bi.biWidth       = bitmap.bmWidth;
      bi.biHeight      = bitmap.bmHeight;
      bi.biPlanes      = GetDeviceCaps(hScrDC,   PLANES);/*1*/;
      bi.biBitCount    = bitmap.bmBitsPixel;
      bi.biCompression = BI_RGB;

      // get the pixel data
      int scanLines = GetDIBits(hMemDC, 
                                (HBITMAP)hBitMap, 
                                0, bitmap.bmHeight, 
                                (char *)bitmapInfo + pixelOffs,
                                bitmapInfo,
                                DIB_RGB_COLORS);

      if (scanLines == 0) {
        PTRACE(2, "AppInput\tFailed to convert image");
      } else {
        int srcPixelSize  = bitmap.bmBitsPixel / 8;
        int dstPixelSize  = (colourFormat == "BGR32") ? 4 : 3;

				if ( frameWidth != bitmap.bmWidth || frameHeight != bitmap.bmHeight )
				{
					*bytesReturned =0;
					return PTrue;
				}
        // convert from 24/32 bit to 24/32 bit, and invert top to bottom
        BYTE * src = (BYTE *)bitmapInfo + pixelOffs + (bitmap.bmHeight-1) * bitmap.bmWidthBytes;
        
        BYTE * dst = (converter == NULL) ? buffer : tempPixelBuffer.GetPointer(bitmap.bmHeight*bitmap.bmWidth * dstPixelSize);
        for (long y = 0; y < bitmap.bmHeight; ++y) {
          for (long x = 0; x < bitmap.bmWidth; ++x) {
 /*           dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];*/
            memcpy(dst, src, 3);
            if (dstPixelSize == 4)
              *(dst+3) = 0;//*dst++ = 0;
            src += srcPixelSize;
            dst += dstPixelSize;
          }
          src -= bitmap.bmWidth*srcPixelSize + bitmap.bmWidthBytes;
        }
        *bytesReturned = bitmap.bmHeight * bitmap.bmWidth * dstPixelSize;
				if (converter != NULL &&(bitmap.bmHeight == converter->GetSrcFrameHeight() && bitmap.bmWidth== converter->GetSrcFrameWidth() )&& !converter->Convert(tempPixelBuffer.GetPointer(), buffer, bytesReturned)) {
          PTRACE(2, "AppInput\tConverter failed");
          retVal = false;
        }
        //delete tempBuffer;
      }
    }

    DeleteObject(hBitMap); 
    DeleteDC(hScrDC);
    DeleteDC(hMemDC);
  }

  /////////////////////////////////////////////////////////////////////

  return retVal;
}


PBoolean PVideoInputDevice_Application::TestAllFormats()
{
    return true;
}

PBoolean PVideoInputDevice_Application::SetChannel(int /*newChannel*/)
{
  return true;
}

void PVideoInputDevice_Application::AttachCaptureWindow(HWND _hwnd, bool _client)
{
    m_hWnd = _hwnd;
    m_client = _client;
}
PBoolean PVideoInputDevice_Application::SetFrameSizeConverter(unsigned width, unsigned height, ResizeMode resizeMode)
{
  //if (SetFrameSize(width, height)) {
  //  if (nativeVerticalFlip && converter == NULL) {
  //    converter = PColourConverter::Create(*this, *this);
  //    if (PAssertNULL(converter) == NULL)
  //      return PFalse;
  //  }
  //  if (converter != NULL) {
  //    converter->SetFrameSize(frameWidth, frameHeight);
  //    converter->SetVFlipState(nativeVerticalFlip);
  //  }
  //  return PTrue;
  //}

  // Try and get the most compatible physical frame size to convert from/to
  //PINDEX i;
  //for (i = 0; i < PARRAYSIZE(framesizeTab); i++) {
  //  if (framesizeTab[i].asked_width == width && framesizeTab[i].asked_height == height &&
  //      SetFrameSize(framesizeTab[i].device_width, framesizeTab[i].device_height))
  //    break;
  //}
  //if (i >= PARRAYSIZE(framesizeTab)) {
  //  // Failed to find a resolution the device can do so far, so try
  //  // using the maximum width and height it claims it can do.
  //  unsigned minWidth, minHeight, maxWidth, maxHeight;
  //  if (GetFrameSizeLimits(minWidth, minHeight, maxWidth, maxHeight))
  //    SetFrameSize(maxWidth, maxHeight);
  //}

  // Now create the converter ( if not already exist)
  if (converter == NULL) {
    PVideoFrameInfo src = *this;
    PVideoFrameInfo dst = *this;
    if (CanCaptureVideo()){
      dst.SetFrameSize(width, height);
     
    }else
      src.SetFrameSize(width, height);
    PTRACE(3,"PVidDev\tColour converter Preparing from " << width << 'x' << height );
    
    dst.SetResizeMode(resizeMode);
    converter = PColourConverter::Create(src, dst);
    converter->SetSrcFrameSize( GetFrameWidth(), GetFrameHeight());
    converter->SetDstFrameSize( width, height);
    if (converter == NULL) {
      PTRACE(1, "PVidDev\tSetFrameSizeConverter Colour converter creation failed");
      return PFalse;
    }
  }
  else
  {
    converter->SetSrcFrameSize( GetFrameWidth(), GetFrameHeight());
    if (CanCaptureVideo())
      converter->SetDstFrameSize(width, height);
    else
      converter->SetSrcFrameSize(width, height);
    converter->SetResizeMode(resizeMode);
  }

  PTRACE(3,"PVidDev\tColour converter used from " << converter->GetSrcFrameWidth() << 'x' << converter->GetSrcFrameHeight() << " [" << converter->GetSrcColourFormat() << "]" << " to " << converter->GetDstFrameWidth() << 'x' << converter->GetDstFrameHeight() << " [" << converter->GetDstColourFormat() << "]");

  return PTrue;
}
PBoolean PVideoInputDevice_Application::SetFrameSizeEx( int x, int y,int w, int h  )
{

  m_rectCapture.left =x;m_rectCapture.top = y;
  m_rectCapture.right = x+w;
  m_rectCapture.bottom = y+h;
return true;
}


#endif  // P_APPSHARE
