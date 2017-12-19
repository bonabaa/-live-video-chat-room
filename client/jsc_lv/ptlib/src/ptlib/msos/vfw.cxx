/*
 * vfw.cxx
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
 * Contributor(s): ______________________________________.
 *                 Walter H Whitlock (twohives@nc.rr.com)
 *
 * $Revision: 23962 $
 * $Author: rjongbloed $
 * $Date: 2010-01-22 04:39:06 +0000 (Fri, 22 Jan 2010) $
 */

#include <ptlib.h>

#if P_VIDEO

#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptlib/pluginmgr.h>
#include <ptclib/delaychan.h>

#if P_VFW_CAPTURE

#ifdef _MSC_VER
#pragma comment(lib, "vfw32.lib")
#pragma comment(lib, "gdi32.lib")
#endif

    
#include <vfw.h>

#ifdef __MINGW32__

#define VHDR_DONE       0x00000001
#define VHDR_KEYFRAME   0x00000008
#define WM_USER 1024
#define WM_CAP_FIRSTA (WM_USER)

#define capSetCallbackOnError(hwnd, fpProc)        ((BOOL)::SendMessage(hwnd, WM_CAP_SET_CALLBACK_ERROR, 0, (LPARAM)(LPVOID)(fpProc)))
#define capSetCallbackOnFrame(hwnd, fpProc)        ((BOOL)::SendMessage(hwnd, WM_CAP_SET_CALLBACK_FRAME, 0, (LPARAM)(LPVOID)(fpProc)))
#define capSetCallbackOnVideoStream(hwnd, fpProc)  ((BOOL)::SendMessage(hwnd, WM_CAP_SET_CALLBACK_VIDEOSTREAM, 0, (LPARAM)(LPVOID)(fpProc)))
#define capGetUserData(hwnd)                       (::SendMessage(hwnd, WM_CAP_GET_USER_DATA, 0, 0))
#define capSetUserData(hwnd, lUser)                ((BOOL)::SendMessage(hwnd, WM_CAP_SET_USER_DATA, 0, (LPARAM)lUser))
#define capDriverConnect(hwnd, i)                  ((BOOL)::SendMessage(hwnd, WM_CAP_DRIVER_CONNECT, (WPARAM)(i), 0L))
#define capDriverDisconnect(hwnd)                  ((BOOL)::SendMessage(hwnd, WM_CAP_DRIVER_DISCONNECT, (WPARAM)0, 0L))
#define capDriverGetCaps(hwnd, s, wSize)           ((BOOL)::SendMessage(hwnd, WM_CAP_DRIVER_GET_CAPS, (WPARAM)(wSize), (LPARAM)(LPVOID)(LPCAPDRIVERCAPS)(s)))
#define capGetVideoFormat(hwnd, s, wSize)          ((DWORD)::SendMessage(hwnd, WM_CAP_GET_VIDEOFORMAT, (WPARAM)(wSize), (LPARAM)(LPVOID)(s)))
#define capGetVideoFormatSize(hwnd)                ((DWORD)::SendMessage(hwnd, WM_CAP_GET_VIDEOFORMAT, 0, NULL))
#define capSetVideoFormat(hwnd, s, wSize)          ((BOOL)::SendMessage(hwnd, WM_CAP_SET_VIDEOFORMAT, (WPARAM)(wSize), (LPARAM)(LPVOID)(s)))
#define capPreview(hwnd, f)                        ((BOOL)::SendMessage(hwnd, WM_CAP_SET_PREVIEW, (WPARAM)(BOOL)(f), 0L))
#define capGetStatus(hwnd, s, wSize)               ((BOOL)::SendMessage(hwnd, WM_CAP_GET_STATUS, (WPARAM)(wSize), (LPARAM)(LPVOID)(LPCAPSTATUS)(s)))
#define capGrabFrameNoStop(hwnd)                   ((BOOL)::SendMessage(hwnd, WM_CAP_GRAB_FRAME_NOSTOP, (WPARAM)0, (LPARAM)0L))
#define capCaptureSetSetup(hwnd, s, wSize)         ((BOOL)::SendMessage(hwnd, WM_CAP_SET_SEQUENCE_SETUP, (WPARAM)(wSize), (LPARAM)(LPVOID)(LPCAPTUREPARMS)(s)))
#define capCaptureGetSetup(hwnd, s, wSize)         ((BOOL)::SendMessage(hwnd, WM_CAP_GET_SEQUENCE_SETUP, (WPARAM)(wSize), (LPARAM)(LPVOID)(LPCAPTUREPARMS)(s)))

extern "C" {
#ifdef _MSC_VER
BOOL VFWAPI capGetDriverDescriptionA (WORD wDriverIndex, LPSTR lpszName,
              int cbName, LPSTR lpszVer, int cbVer);
#endif
}

#endif // __MINGW32


#define STEP_GRAB_CAPTURE 1


/**This class defines a video input device.
 */
class PVideoInputDevice_VideoForWindows : public PVideoInputDevice
{
  PCLASSINFO(PVideoInputDevice_VideoForWindows, PVideoInputDevice);

  public:
    /** Create a new video input device.
     */
    PVideoInputDevice_VideoForWindows();

    /**Close the video input device on destruction.
      */
    ~PVideoInputDevice_VideoForWindows() { Close(); }

    /** Is the device a camera, and obtain video
     */
    static PStringArray GetInputDeviceNames();

    virtual PStringArray GetDeviceNames() const
      { return GetInputDeviceNames(); }

    /**Retrieve a list of Device Capabilities
      */
    static bool GetDeviceCapabilities(
      const PString & /*deviceName*/, ///< Name of device
      Capabilities * /*caps*/         ///< List of supported capabilities
    ) { return false; }

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

    /**Set the video frame rate to be used on the device.

       Default behaviour sets the value of the frameRate variable and then
       returns PTrue.
    */
    virtual PBoolean SetFrameRate(
      unsigned rate  /// Frames  per second
    );

    /**Set the frame size to be used.

       Note that devices may not be able to produce the requested size, and
       this function will fail.  See SetFrameSizeConverter().

       Default behaviour sets the frameWidth and frameHeight variables and
       returns PTrue.
    */
    virtual PBoolean SetFrameSize(
      unsigned width,   /// New width of frame
      unsigned height   /// New height of frame
    );

    /**Get the maximum frame size in bytes.

       Note a particular device may be able to provide variable length
       frames (eg motion JPEG) so will be the maximum size of all frames.
      */
    virtual PINDEX GetMaxFrameBytes();

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

  protected:

   /**Check the hardware can do the asked for size.

       Note that not all cameras can provide all frame sizes.
     */
    virtual PBoolean VerifyHardwareFrameSize(unsigned width, unsigned height);

    PDECLARE_NOTIFIER(PThread, PVideoInputDevice_VideoForWindows, HandleCapture);

    static LRESULT CALLBACK ErrorHandler(HWND hWnd, int id, LPCSTR err);
    LRESULT HandleError(int id, LPCSTR err);
    static LRESULT CALLBACK VideoHandler(HWND hWnd, LPVIDEOHDR vh);
    LRESULT HandleVideo(LPVIDEOHDR vh);
    PBoolean InitialiseCapture();

    PThread     * captureThread;
    PSyncPoint    threadStarted;

    HWND          hCaptureWindow;
    PMutex        operationMutex;

    PSyncPoint    frameAvailable;
    LPBYTE        lastFramePtr;
    unsigned      lastFrameSize;
    PMutex        lastFrameMutex;
    bool          isCapturingNow;
    PAdaptiveDelay m_Pacing;
};


///////////////////////////////////////////////////////////////////////////////

class PCapStatus : public CAPSTATUS
{
  public:
    PCapStatus(HWND hWnd);
    PBoolean IsOK()
       { return uiImageWidth != 0; }
};

///////////////////////////////////////////////////////////////////////////////

static struct FormatTableEntry { 
  const char * colourFormat; 
  WORD  bitCount;
  PBoolean  negHeight; // MS documentation suggests that negative height will request
                  // top down scan direction from video driver
                  // HOWEVER, not all drivers honor this request
  DWORD compression; 
} FormatTable[] = {
  { "BGR32",   32, TRUE,  BI_RGB },
  { "BGR24",   24, TRUE,  BI_RGB },
  { "Grey",     8, TRUE,  BI_RGB },
  { "Gray",     8, TRUE,  BI_RGB },

  { "RGB565",  16, TRUE,  BI_BITFIELDS },
  { "RGB555",  15, TRUE,  BI_BITFIELDS },

  // http://support.microsoft.com/support/kb/articles/q294/8/80.asp
  { "YUV420P", 12, FALSE, mmioFOURCC('I','Y','U','V') },
  { "IYUV",    12, FALSE, mmioFOURCC('I','Y','U','V') }, // Synonym for IYUV
  { "I420",    12, FALSE, mmioFOURCC('I','4','2','0') }, // Synonym for IYUV
  { "YV12",    12, FALSE, mmioFOURCC('Y','V','1','2') }, // same as IYUV except that U and V planes are switched
  { "YUV422",  16, FALSE, mmioFOURCC('Y','U','Y','2') },
  { "YUY2",    16, FALSE, mmioFOURCC('Y','U','Y','2') },
  { "UYVY",    16, FALSE, mmioFOURCC('U','Y','V','Y') }, // Like YUY2 except for ordering
  { "YVYU",    16, FALSE, mmioFOURCC('Y','V','Y','U') }, // Like YUY2 except for ordering
  { "YVU9",    16, FALSE, mmioFOURCC('Y','V','U','9') },
  { "MJPEG",   12, FALSE, mmioFOURCC('M','J','P','G') },
  { NULL },
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

///////////////////////////////////////////////////////////////////////////////

class PVideoDeviceBitmap : PBYTEArray
{
  public:
    // the following method is replaced by PVideoDeviceBitmap(HWND hWnd, WORD bpp)
    // PVideoDeviceBitmap(unsigned width, unsigned height, const PString & fmt);
    //returns object with gray color pallet if needed for 8 bpp formats
    PVideoDeviceBitmap(HWND hWnd, WORD bpp);
    // does not build color pallet
    PVideoDeviceBitmap(HWND hWnd); 
    // apply video format to capture device
    PBoolean ApplyFormat(HWND hWnd, const FormatTableEntry & formatTableEntry);

    BITMAPINFO * operator->() const 
    { return (BITMAPINFO *)theArray; }
};

PVideoDeviceBitmap::PVideoDeviceBitmap(HWND hCaptureWindow)
{
  PINDEX sz = capGetVideoFormatSize(hCaptureWindow);
  SetSize(sz);
  if (!capGetVideoFormat(hCaptureWindow, theArray, sz)) { 
    PTRACE(1, "PVidInp\tcapGetVideoFormat(hCaptureWindow) failed - " << ::GetLastError());
    SetSize(0);
    return;
  }
}

PVideoDeviceBitmap::PVideoDeviceBitmap(HWND hCaptureWindow, WORD bpp)
{
  PINDEX sz = capGetVideoFormatSize(hCaptureWindow);
  SetSize(sz);
  if (!capGetVideoFormat(hCaptureWindow, theArray, sz)) { 
    PTRACE(1, "PVidInp\tcapGetVideoFormat(hCaptureWindow) failed - " << ::GetLastError());
    SetSize(0);
    return;
  }

  if (8 == bpp && bpp != ((BITMAPINFO*)theArray)->bmiHeader.biBitCount) {
    SetSize(sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*256);
    RGBQUAD * bmiColors = ((BITMAPINFO*)theArray)->bmiColors;
    for (int i = 0; i < 256; i++)
      bmiColors[i].rgbBlue  = bmiColors[i].rgbGreen = bmiColors[i].rgbRed = (BYTE)i;
  }
  ((BITMAPINFO*)theArray)->bmiHeader.biBitCount = bpp;
}

PBoolean PVideoDeviceBitmap::ApplyFormat(HWND hWnd, const FormatTableEntry & formatTableEntry)
{
  // NB it is necessary to set the biSizeImage value appropriate to frame size
  // assume bmiHeader.biBitCount has already been set appropriatly for format
  BITMAPINFO & bmi = *(BITMAPINFO*)theArray;
  bmi.bmiHeader.biPlanes = 1;

  int height = bmi.bmiHeader.biHeight<0 ? -bmi.bmiHeader.biHeight : bmi.bmiHeader.biHeight;
  bmi.bmiHeader.biSizeImage = height*4*((bmi.bmiHeader.biBitCount * bmi.bmiHeader.biWidth + 31)/32);

  // set .biHeight according to .negHeight value
  if (formatTableEntry.negHeight)
    bmi.bmiHeader.biHeight = -height; 

#if PTRACING
  PTimeInterval startTime = PTimer::Tick();
#endif

  if (capSetVideoFormat(hWnd, theArray, GetSize())) {
    PTRACE(3, "PVidInp\tcapSetVideoFormat succeeded: "
            << PString(formatTableEntry.colourFormat) << ' '
            << bmi.bmiHeader.biWidth << "x" << bmi.bmiHeader.biHeight
            << " sz=" << bmi.bmiHeader.biSizeImage << " time=" << (PTimer::Tick() - startTime));
    return PTrue;
  }

  if (formatTableEntry.negHeight) {
    bmi.bmiHeader.biHeight = height; 
    if (capSetVideoFormat(hWnd, theArray, GetSize())) {
      PTRACE(3, "PVidInp\tcapSetVideoFormat succeeded: "
              << PString(formatTableEntry.colourFormat) << ' '
              << bmi.bmiHeader.biWidth << "x" << bmi.bmiHeader.biHeight
              << " sz=" << bmi.bmiHeader.biSizeImage << " time=" << (PTimer::Tick() - startTime));
      return PTrue;
    }
  }

  PTRACE(1, "PVidInp\tcapSetVideoFormat failed: "
          << (formatTableEntry.colourFormat != NULL ? formatTableEntry.colourFormat : "NO-COLOUR-FORMAT") << ' '
          << bmi.bmiHeader.biWidth << "x" << bmi.bmiHeader.biHeight
          << " sz=" << bmi.bmiHeader.biSizeImage << " time=" << (PTimer::Tick() - startTime)
          << " - lastError=" << ::GetLastError());
  return PFalse;
}

///////////////////////////////////////////////////////////////////////////////

PCapStatus::PCapStatus(HWND hWnd)
{
  memset(this, 0, sizeof(*this));
  if (capGetStatus(hWnd, this, sizeof(*this)))
    return;

  PTRACE(1, "PVidInp\tcapGetStatus: failed - " << ::GetLastError());
}


///////////////////////////////////////////////////////////////////////////////
// PVideoInputDevice_VideoForWindows

PCREATE_VIDINPUT_PLUGIN(VideoForWindows);

PVideoInputDevice_VideoForWindows::PVideoInputDevice_VideoForWindows()
{
  captureThread = NULL;
  hCaptureWindow = NULL;
  lastFramePtr = NULL;
  lastFrameSize = 0;
  isCapturingNow = PFalse;
}

PBoolean PVideoInputDevice_VideoForWindows::Open(const PString & devName, PBoolean startImmediate)
{
  Close();

  operationMutex.Wait();

  deviceName = devName;

  captureThread = PThread::Create(PCREATE_NOTIFIER(HandleCapture), "VidIn");

  operationMutex.Signal();
  threadStarted.Wait();

  PWaitAndSignal mutex(operationMutex);

  if (hCaptureWindow == NULL) {
    delete captureThread;
    captureThread = NULL;
    return PFalse;
  }

  if (startImmediate)
    return Start();

  return PTrue;
}


PBoolean PVideoInputDevice_VideoForWindows::IsOpen() 
{
  return hCaptureWindow != NULL;
}


PBoolean PVideoInputDevice_VideoForWindows::Close()
{
  PWaitAndSignal mutex(operationMutex);

  if (!IsOpen())
    return PFalse;
 
  Stop();

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

  return PTrue;
}


PBoolean PVideoInputDevice_VideoForWindows::Start()
{
  PWaitAndSignal mutex(operationMutex);

  if (IsCapturing())
    return PTrue;

#if STEP_GRAB_CAPTURE
  isCapturingNow = PTrue;
  return capGrabFrameNoStop(hCaptureWindow);
#else
  if (capCaptureSequenceNoFile(hCaptureWindow)) {
    PCapStatus status(hCaptureWindow);
    isCapturingNow = status.fCapturingNow;
    return isCapturingNow;
  }

  lastError = ::GetLastError();
  PTRACE(1, "PVidInp\tcapCaptureSequenceNoFile: failed - " << lastError);
  return PFalse;
#endif
}


PBoolean PVideoInputDevice_VideoForWindows::Stop()
{
  PWaitAndSignal mutex(operationMutex);

  if (!IsCapturing())
    return PFalse;
  isCapturingNow = PFalse;
#if STEP_GRAB_CAPTURE
  return IsOpen() && frameAvailable.Wait(1000);
#else
  if (capCaptureStop(hCaptureWindow))
    return PTrue;

  lastError = ::GetLastError();
  PTRACE(1, "PVidInp\tcapCaptureStop: failed - " << lastError);
  return PFalse;
#endif
}


PBoolean PVideoInputDevice_VideoForWindows::IsCapturing()
{
  return isCapturingNow;
}


PBoolean PVideoInputDevice_VideoForWindows::SetColourFormat(const PString & colourFmt)
{
  PWaitAndSignal mutex(operationMutex);

  if (!IsOpen())
    return PVideoDevice::SetColourFormat(colourFmt); // Not open yet, just set internal variables

  PBoolean running = IsCapturing();
  if (running)
    Stop();

  PString oldFormat = colourFormat;

  if (!PVideoDevice::SetColourFormat(colourFmt))
    return PFalse;

  PINDEX i = 0;
  while (FormatTable[i].colourFormat != NULL && !(colourFmt *= FormatTable[i].colourFormat))
    i++;

  PVideoDeviceBitmap bi(hCaptureWindow, FormatTable[i].bitCount);

  if (FormatTable[i].colourFormat != NULL)
    bi->bmiHeader.biCompression = FormatTable[i].compression;
  else if (colourFmt.GetLength() == 4)
    bi->bmiHeader.biCompression = mmioFOURCC(colourFmt[0],colourFmt[1],colourFmt[2],colourFmt[3]);
  else {
    bi->bmiHeader.biCompression = 0xffffffff; // Indicate invalid colour format
    PVideoDevice::SetColourFormat(oldFormat);
    return PFalse;
  }

  // set frame width and height
  bi->bmiHeader.biWidth = frameWidth;
  bi->bmiHeader.biHeight = frameHeight;
  if (!bi.ApplyFormat(hCaptureWindow, FormatTable[i])) {
    lastError = ::GetLastError();
    PVideoDevice::SetColourFormat(oldFormat);
    return PFalse;
  }

  // Didn't do top down, tell everything we are up side down
  nativeVerticalFlip = FormatTable[i].negHeight && bi->bmiHeader.biHeight > 0;

  if (running)
    return Start();

  return PTrue;
}


PBoolean PVideoInputDevice_VideoForWindows::SetFrameRate(unsigned rate)
{
  PWaitAndSignal mutex(operationMutex);

  if (!PVideoDevice::SetFrameRate(rate))
    return PFalse;

  if (!IsOpen())
    return PTrue; // Not open yet, just set internal variables

  PBoolean running = IsCapturing();
  if (running)
    Stop();

  CAPTUREPARMS parms;
  memset(&parms, 0, sizeof(parms));

  if (!capCaptureGetSetup(hCaptureWindow, &parms, sizeof(parms))) {
    lastError = ::GetLastError();
    PTRACE(1, "PVidInp\tcapCaptureGetSetup: failed - " << lastError);
    return PFalse;
  }

  // keep current (default) framerate if 0==frameRate   
  if (0 != frameRate)
    parms.dwRequestMicroSecPerFrame = 1000000 / frameRate;
  parms.fMakeUserHitOKToCapture = PFalse;
  parms.wPercentDropForError = 100;
  parms.fCaptureAudio = PFalse;
  parms.fAbortLeftMouse = PFalse;
  parms.fAbortRightMouse = PFalse;
  parms.fLimitEnabled = PFalse;

  if (!capCaptureSetSetup(hCaptureWindow, &parms, sizeof(parms))) {
    lastError = ::GetLastError();
    PTRACE(1, "PVidInp\tcapCaptureSetSetup: failed - " << lastError);
    return PFalse;
  }
    
  if (running)
    return Start();

  return PTrue;
}


PBoolean PVideoInputDevice_VideoForWindows::SetFrameSize(unsigned width, unsigned height)
{
  PWaitAndSignal mutex(operationMutex);

  if (!IsOpen())
    return PVideoDevice::SetFrameSize(width, height); // Not open yet, just set internal variables

  PBoolean running = IsCapturing();
  if (running)
    Stop();

  PVideoDeviceBitmap bi(hCaptureWindow); 
  PTRACE(5, "PVidInp\tChanging frame size from "
         << bi->bmiHeader.biWidth << 'x' << bi->bmiHeader.biHeight << " to " << width << 'x' << height);

  PINDEX i = 0;
  while (FormatTable[i].colourFormat != NULL && !(colourFormat *= FormatTable[i].colourFormat))
    i++;

  bi->bmiHeader.biWidth = width;
  bi->bmiHeader.biHeight = height;
  if (!bi.ApplyFormat(hCaptureWindow, FormatTable[i])) {
    lastError = ::GetLastError();
    return PFalse;
  }

  // Didn't do top down, tell everything we are up side down
  nativeVerticalFlip = FormatTable[i].negHeight && bi->bmiHeader.biHeight > 0;

  // verify that the driver really took the frame size
  if (!VerifyHardwareFrameSize(width, height)) 
    return PFalse; 

  // frameHeight must be positive regardlesss of what the driver says
  if (0 > (int)height) 
    height = (unsigned)-(int)height;

  if (!PVideoDevice::SetFrameSize(width, height))
    return PFalse;

  if (running)
    return Start();

  return PTrue;
}


PBoolean PVideoInputDevice_VideoForWindows::TestAllFormats()
{
  PBoolean running = IsCapturing();
  if (running)
    Stop();

  for (PINDEX prefFormatIdx = 0; FormatTable[prefFormatIdx].colourFormat != NULL; prefFormatIdx++) {
    PVideoDeviceBitmap bi(hCaptureWindow, FormatTable[prefFormatIdx].bitCount); 
    bi->bmiHeader.biCompression = FormatTable[prefFormatIdx].compression;
    for (PINDEX prefResizeIdx = 0; prefResizeIdx < PARRAYSIZE(winTestResTable); prefResizeIdx++) {
      bi->bmiHeader.biWidth = winTestResTable[prefResizeIdx].device_width;
      bi->bmiHeader.biHeight = winTestResTable[prefResizeIdx].device_height;
      bi.ApplyFormat(hCaptureWindow, FormatTable[prefFormatIdx]);
    } // for prefResizeIdx
  } // for prefFormatIdx

  if (running)
    return Start();

  return PTrue;
}


//return PTrue if absolute value of height reported by driver 
//  is equal to absolute value of current frame height AND
//  width reported by driver is equal to current frame width
PBoolean PVideoInputDevice_VideoForWindows::VerifyHardwareFrameSize(unsigned width, unsigned height)
{
  PCapStatus status(hCaptureWindow);

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


PStringArray PVideoInputDevice_VideoForWindows::GetInputDeviceNames()
{
  PStringArray devices;

  for (WORD devId = 0; devId < 10; devId++) {
    char name[100];
    char version[200];
    if (capGetDriverDescription(devId, name, sizeof(name), version, sizeof(version)))
      devices.AppendString(name);
  }

  return devices;
}


PINDEX PVideoInputDevice_VideoForWindows::GetMaxFrameBytes()
{
  PWaitAndSignal mutex(operationMutex);

  if (!IsOpen())
    return 0;

  return GetMaxFrameBytesConverted(PVideoDeviceBitmap(hCaptureWindow)->bmiHeader.biSizeImage);
}


PBoolean PVideoInputDevice_VideoForWindows::GetFrameData(BYTE * buffer, PINDEX * bytesReturned)
{
  // Some camera drivers ignore the frame rate set in the CAPTUREPARMS structure,
  // so we have a fail safe delay here.
  m_Pacing.Delay(1000/GetFrameRate());
  return GetFrameDataNoDelay(buffer, bytesReturned);
}


PBoolean PVideoInputDevice_VideoForWindows::GetFrameDataNoDelay(BYTE * buffer, PINDEX * bytesReturned)
{
  if (!frameAvailable.Wait(1000)) {
    PTRACE(1, "PVidInp\tTimeout waiting for frame grab!");
    return PFalse;
  }

  bool retval = false;

  lastFrameMutex.Wait();

  if (lastFramePtr != NULL) {
    if (NULL != converter)
      retval = converter->Convert(lastFramePtr, buffer, bytesReturned);
    else {
      memcpy(buffer, lastFramePtr, lastFrameSize);
      if (bytesReturned != NULL)
        *bytesReturned = lastFrameSize;
      retval = true;
    }
  }

  lastFrameMutex.Signal();

#if STEP_GRAB_CAPTURE
  if (isCapturingNow)
    capGrabFrameNoStop(hCaptureWindow);
#endif

  return retval;
}


LRESULT CALLBACK PVideoInputDevice_VideoForWindows::ErrorHandler(HWND hWnd, int id, LPCSTR err)
{
  if (hWnd == NULL)
    return PFalse;

  return ((PVideoInputDevice_VideoForWindows *)capGetUserData(hWnd))->HandleError(id, err);
}


LRESULT PVideoInputDevice_VideoForWindows::HandleError(int id, LPCSTR PTRACE_PARAM(err))
{
  if (id != 0) {
    PTRACE(1, "PVidInp\tErrorHandler: [id="<< id << "] " << err);
  }

  return PTrue;
}


LRESULT CALLBACK PVideoInputDevice_VideoForWindows::VideoHandler(HWND hWnd, LPVIDEOHDR vh)
{
  if (hWnd == NULL || capGetUserData(hWnd) == NULL)
    return PFalse;

  return ((PVideoInputDevice_VideoForWindows *)capGetUserData(hWnd))->HandleVideo(vh);
}


LRESULT PVideoInputDevice_VideoForWindows::HandleVideo(LPVIDEOHDR vh)
{
  if ((vh->dwFlags&(VHDR_DONE|VHDR_KEYFRAME)) != 0) {
    lastFrameMutex.Wait();
    lastFramePtr = vh->lpData;
    lastFrameSize = vh->dwBytesUsed;
    if (lastFrameSize == 0)
      lastFrameSize = vh->dwBufferLength;
    lastFrameMutex.Signal();
    frameAvailable.Signal();
  }

  return PTrue;
}


PBoolean PVideoInputDevice_VideoForWindows::InitialiseCapture()
{
  if ((hCaptureWindow = capCreateCaptureWindow("Capture Window",
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

  capSetCallbackOnError(hCaptureWindow, ErrorHandler);

#if STEP_GRAB_CAPTURE
  if (!capSetCallbackOnFrame(hCaptureWindow, VideoHandler)) { //} balance braces
#else
  if (!capSetCallbackOnVideoStream(hCaptureWindow, VideoHandler)) {
#endif
    lastError = ::GetLastError();
    PTRACE(1, "PVidInp\tcapSetCallbackOnVideoStream failed - " << lastError);
    return PFalse;
  }

  WORD devId;

#if PTRACING
  if (PTrace::CanTrace(4)) { // list available video capture drivers
    ostream & trace = PTrace::Begin(5, __FILE__, __LINE__);
    trace << "PVidInp\tEnumerating available video capture drivers:\n";
    for (devId = 0; devId < 10; devId++) { 
      char name[100];
      char version[200];
      if (capGetDriverDescription(devId, name, sizeof(name), version, sizeof(version)) ) 
        trace << "  Video device[" << devId << "] = " << name << ", " << version << '\n';
    }
    trace << PTrace::End;
  }
#endif

  if (deviceName.GetLength() == 1 && isdigit(deviceName[0]))
    devId = (WORD)(deviceName[0] - '0');
  else {
    for (devId = 0; devId < 10; devId++) {
      char name[100];
      char version[200];
      if (capGetDriverDescription(devId, name, sizeof(name), version, sizeof(version)) &&
          (deviceName *= name))
        break;
    }
  }

  capSetUserData(hCaptureWindow, this);

  // Use first driver available.
  if (!capDriverConnect(hCaptureWindow, devId)) {
    lastError = ::GetLastError();
    PTRACE(1, "PVidInp\tcapDriverConnect failed - " << lastError);
    return PFalse;
  }

  CAPDRIVERCAPS driverCaps;
  memset(&driverCaps, 0, sizeof(driverCaps));
  if (!capDriverGetCaps(hCaptureWindow, &driverCaps, sizeof(driverCaps))) {
    lastError = ::GetLastError();
    PTRACE(1, "PVidInp\tcapGetDriverCaps failed - " << lastError);
    return PFalse;
  }

  PTRACE(6, "PVidInp\tEnumerating CAPDRIVERCAPS values:\n"
            "  driverCaps.wDeviceIndex           = " << driverCaps.wDeviceIndex        << "\n"
            "  driverCaps.fHasOverlay            = " << driverCaps.fHasOverlay         << "\n"
            "  driverCaps.fHasDlgVideoSource     = " << driverCaps.fHasDlgVideoSource  << "\n"
            "  driverCaps.fHasDlgVideoFormat     = " << driverCaps.fHasDlgVideoFormat  << "\n"
            "  driverCaps.fHasDlgVideoDisplay    = " << driverCaps.fHasDlgVideoDisplay << "\n"
            "  driverCaps.fCaptureInitialized    = " << driverCaps.fCaptureInitialized << "\n"
            "  driverCaps.fDriverSuppliesPalettes= " << driverCaps.fDriverSuppliesPalettes);
  
/*
  if (driverCaps.fHasOverlay)
    capOverlay(hCaptureWindow, PTrue);
  else {
    capPreviewRate(hCaptureWindow, 66);
    capPreview(hCaptureWindow, PTrue);
  }
*/
   
  capPreview(hCaptureWindow, PFalse);

#if PTRACING
  //if (PTrace::CanTrace(6))
  //  TestAllFormats(); // list acceptable formats and frame resolutions for video capture driver
#endif
  
  return SetFrameRate(frameRate) && SetColourFormatConverter(colourFormat.IsEmpty() ? PString("YUV420P") : colourFormat);
}


void PVideoInputDevice_VideoForWindows::HandleCapture(PThread &, INT)
{
  PBoolean initSucceeded = InitialiseCapture();

  if (initSucceeded) {
    threadStarted.Signal();

    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0))
      ::DispatchMessage(&msg);
  }

  PTRACE(5, "PVidInp\tDisconnecting driver");
  capDriverDisconnect(hCaptureWindow);
  capSetUserData(hCaptureWindow, NULL);

  capSetCallbackOnError(hCaptureWindow, NULL);
  capSetCallbackOnVideoStream(hCaptureWindow, NULL);

  PTRACE(5, "PVidInp\tDestroying VIDCAP window");
  DestroyWindow(hCaptureWindow);
  hCaptureWindow = NULL;

  // Signal the other thread we have completed, even if have error
  if (!initSucceeded)
    threadStarted.Signal();
}

#endif // P_VFW_CAPTURE


///////////////////////////////////////////////////////////////////////////////
// PVideoOutputDevice_Window

/**This class defines a video output device for RGB in a frame store.
 */
class PVideoOutputDevice_Window : public PVideoOutputDeviceRGB
{
  PCLASSINFO(PVideoOutputDevice_Window, PVideoOutputDeviceRGB);

  public:
    /** Create a new video output device.
     */
    PVideoOutputDevice_Window();
		virtual	int  GetHWND(){return (int)m_hWnd;};

    /** Destroy a video output device.
     */
    ~PVideoOutputDevice_Window();

    /**Open the device given the device name.
      */
    virtual PBoolean Open(
      const PString & deviceName,   /// Device name (filename base) to open
      PBoolean startImmediate = PTrue    /// Immediately start device
    );

    /**Determine if the device is currently open.
      */
    virtual PBoolean IsOpen();

    /**Close the device.
      */
    virtual PBoolean Close();

    /**Start the video device I/O display.
      */
    virtual PBoolean Start();

    /**Stop the video device I/O display.
      */
    virtual PBoolean Stop();

    /**Get a list of all of the devices available.
      */
    static PStringArray GetOutputDeviceNames();

    /**Get a list of all of the devices available.
      */
    virtual PStringArray GetDeviceNames() const
    { return GetOutputDeviceNames(); }

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

    /**Get the video conversion vertical flip state.
       Default action is to return PFalse.
     */
    virtual PBoolean GetVFlipState();

    /**Set the video conversion vertical flip state.
       Default action is to return PFalse.
     */
    virtual PBoolean SetVFlipState(
      PBoolean newVFlipState    /// New vertical flip state
    );

    /**Set the frame size to be used.

       Note that devices may not be able to produce the requested size, and
       this function will fail.  See SetFrameSizeConverter().

       Default behaviour sets the frameWidth and frameHeight variables and
       returns PTrue.
    */
    virtual PBoolean SetFrameSize(
      unsigned width,   /// New width of frame
      unsigned height   /// New height of frame
    );

    /**Set a section of the output frame buffer.
      */
    virtual PBoolean FrameComplete();

    /**Get the position of the output device, where relevant. For devices such as
       files, this always returns zeros. For devices such as Windows, this is the
       position of the window on the screen.
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

    LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

  protected:
    PDECLARE_NOTIFIER(PThread, PVideoOutputDevice_Window, HandleDisplay);
    void SetWindowSize();
    void Draw(HDC hDC);

    HWND       m_hWnd;
    PThread  * m_thread;
    PMutex     m_openCloseMutex;
    PSyncPoint m_started;
    BITMAPINFO m_bitmap;
    bool       m_flipped;
    POINT      m_lastPosition;
    SIZE       m_fixedSize;
};


#define DEFAULT_STYLE (WS_POPUP|WS_BORDER|WS_SYSMENU|WS_CAPTION)
#define DEFAULT_TITLE "Video Output"

static bool ParseWindowDeviceName(const PString & deviceName, DWORD * dwStylePtr = NULL, HWND * hWndParentPtr = NULL)
{
  if (deviceName.Find("MSWIN") != 0)
    return false;

  PINDEX pos = deviceName.Find("STYLE=");
  DWORD dwStyle = pos == P_MAX_INDEX ? DEFAULT_STYLE : strtoul(((const char *)deviceName)+pos+6, NULL, 0);
  if ((dwStyle&(WS_POPUP|WS_CHILD)) == 0) {
    PTRACE(1, "VidOut\tWindow must be WS_POPUP or WS_CHILD window.");
    return false;
  }

  HWND hWndParent = NULL;
  pos = deviceName.Find("PARENT=");
  if (pos != P_MAX_INDEX) {
    hWndParent = (HWND)strtoul(((const char *)deviceName)+pos+7, NULL, 0);
    if (!::IsWindow(hWndParent)) {
      PTRACE(2, "VidOut\tIllegal parent window " << hWndParent << " specified.");
      hWndParent = NULL;
    }
  }

  // Have parsed out style & parent, see if legal combination
  if (hWndParent == NULL && (dwStyle&WS_POPUP) == 0) {
    PTRACE(1, "VidOut\tWindow must be WS_POPUP if parent window not specified.");
    return false;
  }

  if (dwStylePtr != NULL)
    *dwStylePtr = dwStyle;

  if (hWndParentPtr != NULL)
    *hWndParentPtr = hWndParent;

  return true;
}


class PVideoOutputDevice_Window_PluginServiceDescriptor : public PDevicePluginServiceDescriptor
{
  public:
    virtual PObject *    CreateInstance(int /*userData*/) const { return PNEW PVideoOutputDevice_Window; }
    virtual PStringArray GetDeviceNames(int /*userData*/) const { return PVideoOutputDevice_Window::GetOutputDeviceNames(); }
    virtual bool         ValidateDeviceName(const PString & deviceName, int /*userData*/) const { return ParseWindowDeviceName(deviceName); }
} PVideoOutputDevice_Window_descriptor;

PCREATE_PLUGIN(Window, PVideoOutputDevice, &PVideoOutputDevice_Window_descriptor);


///////////////////////////////////////////////////////////////////////////////
// PVideoOutputDeviceRGB

PVideoOutputDevice_Window::PVideoOutputDevice_Window()
{
  m_hWnd = NULL;
  m_thread = NULL;
  m_flipped = PFalse;
  m_lastPosition.x = 0;
  m_lastPosition.y = 0;
  m_fixedSize.cx = 0;
  m_fixedSize.cy = 0;

  m_bitmap.bmiHeader.biSize = sizeof(m_bitmap.bmiHeader);
  m_bitmap.bmiHeader.biWidth = frameWidth;
  m_bitmap.bmiHeader.biHeight = -(int)frameHeight;
  m_bitmap.bmiHeader.biPlanes = 1;
  m_bitmap.bmiHeader.biBitCount = 32;
  m_bitmap.bmiHeader.biCompression = BI_RGB;
  m_bitmap.bmiHeader.biXPelsPerMeter = 0;
  m_bitmap.bmiHeader.biYPelsPerMeter = 0;
  m_bitmap.bmiHeader.biClrImportant = 0;
  m_bitmap.bmiHeader.biClrUsed = 0;
  m_bitmap.bmiHeader.biSizeImage = frameStore.GetSize();
}


PVideoOutputDevice_Window::~PVideoOutputDevice_Window()
{
  Close();
}


PStringArray PVideoOutputDevice_Window::GetOutputDeviceNames()
{
  return psprintf("MSWIN STYLE=0x%08X TITLE=\"%s\"", DEFAULT_STYLE, DEFAULT_TITLE);
}


PBoolean PVideoOutputDevice_Window::Open(const PString & name, PBoolean startImmediate)
{
  Close();

  m_openCloseMutex.Wait();

  deviceName = name;

  m_thread = PThread::Create(PCREATE_NOTIFIER(HandleDisplay), "VidOut");

  m_openCloseMutex.Signal();
  m_started.Wait();

  PWaitAndSignal m(m_openCloseMutex);

  return startImmediate ? Start() : m_hWnd != NULL;
}


PBoolean PVideoOutputDevice_Window::IsOpen()
{
  return m_hWnd != NULL;
}


PBoolean PVideoOutputDevice_Window::Close()
{
  PWaitAndSignal m(m_openCloseMutex);

  if (m_hWnd == NULL)
      return PFalse;

  SendMessage(m_hWnd, WM_CLOSE, 0, 0);
  m_thread->WaitForTermination(3000);
  delete m_thread;
  m_thread = NULL;
  return PTrue;
}


PBoolean PVideoOutputDevice_Window::Start()
{
  PWaitAndSignal m(m_openCloseMutex);

  if (m_hWnd == NULL)
    return PFalse;
  
  ShowWindow(m_hWnd, SW_SHOW);
  return PTrue;
}


PBoolean PVideoOutputDevice_Window::Stop()
{
  PWaitAndSignal m(m_openCloseMutex);

  if (m_hWnd != NULL)
    return ShowWindow(m_hWnd, SW_HIDE);

  return PFalse;
}


PBoolean PVideoOutputDevice_Window::SetColourFormat(const PString & colourFormat)
{
  PWaitAndSignal m(mutex);

  if (((colourFormat *= "BGR24") || (colourFormat *= "BGR32")) &&
                PVideoOutputDeviceRGB::SetColourFormat(colourFormat)) {
    m_bitmap.bmiHeader.biBitCount = (WORD)(bytesPerPixel*8);
    m_bitmap.bmiHeader.biSizeImage = frameStore.GetSize();
    return PTrue;
  }

  return PFalse;
}


PBoolean PVideoOutputDevice_Window::GetVFlipState()
{
  return m_flipped;
}


PBoolean PVideoOutputDevice_Window::SetVFlipState(PBoolean newVFlip)
{
  m_flipped = newVFlip;
  m_bitmap.bmiHeader.biHeight = m_flipped ? frameHeight : -(int)frameHeight;
  return PTrue;
}


PBoolean PVideoOutputDevice_Window::SetFrameSize(unsigned width, unsigned height)
{
  {
    PWaitAndSignal m(mutex);

    if (width == frameWidth && height == frameHeight)
      return PTrue;

    if (!PVideoOutputDeviceRGB::SetFrameSize(width, height))
      return PFalse;

    m_bitmap.bmiHeader.biWidth = frameWidth;
    m_bitmap.bmiHeader.biHeight = m_flipped ? frameHeight : -(int)frameHeight;
    m_bitmap.bmiHeader.biSizeImage = frameStore.GetSize();
  }

  // Must be outside of mutex
  SetWindowSize();

  return true;
}


void PVideoOutputDevice_Window::SetWindowSize()
{
  if (m_hWnd == NULL)
    return;

  RECT rect;
  rect.top = 0;
  rect.left = 0;
  rect.bottom = m_fixedSize.cy > 0 ? m_fixedSize.cy : frameHeight;
  rect.right = m_fixedSize.cx > 0 ? m_fixedSize.cx : frameWidth;
  ::AdjustWindowRectEx(&rect, GetWindowLong(m_hWnd, GWL_STYLE), false, GetWindowLong(m_hWnd, GWL_EXSTYLE));
  ::SetWindowPos(m_hWnd, HWND_TOP, 0, 0, rect.right-rect.left, rect.bottom-rect.top, SWP_NOMOVE);
}


PBoolean PVideoOutputDevice_Window::FrameComplete()
{
  PWaitAndSignal m(mutex);

  if (m_hWnd == NULL)
    return PFalse;

  HDC hDC = GetDC(m_hWnd);
  Draw(hDC);
  ReleaseDC(m_hWnd, hDC);

  return PTrue;
}


PBoolean PVideoOutputDevice_Window::GetPosition(int & x, int & y) const
{
  x = m_lastPosition.x;
  y = m_lastPosition.y;
  return PTrue;
}


bool PVideoOutputDevice_Window::SetPosition(int x, int y)
{
  if (m_hWnd != NULL) {
    RECT rect;
    rect.top = y;
    rect.left = x;
    rect.bottom = y;
    rect.right = x;
    ::AdjustWindowRectEx(&rect, GetWindowLong(m_hWnd, GWL_STYLE), false, GetWindowLong(m_hWnd, GWL_EXSTYLE));
    ::SetWindowPos(m_hWnd, HWND_TOP, x+(x-rect.left), y, 0, 0, SWP_NOSIZE);
  }

  return true;
}


void PVideoOutputDevice_Window::Draw(HDC hDC)
{
  RECT rect;
  GetClientRect(m_hWnd, &rect);

  int result;
  if (frameWidth == (unsigned)rect.right && frameHeight == (unsigned)rect.bottom)
    result = SetDIBitsToDevice(hDC,
                               0, 0, frameWidth, frameHeight,
                               0, 0, 0, frameHeight,
                               frameStore.GetPointer(), &m_bitmap, DIB_RGB_COLORS);
  else
    result = StretchDIBits(hDC,
                           0, 0, rect.right, rect.bottom,
                           0, 0, frameWidth, frameHeight,
                           frameStore.GetPointer(), &m_bitmap, DIB_RGB_COLORS, SRCCOPY);

  if (result == 0) {
    lastError = ::GetLastError();
    PTRACE(2, "VidOut\tDrawing image failed, error=" << lastError);
  }
}


static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (uMsg == WM_CREATE)
    SetWindowLong(hWnd, 0, (LONG)((LPCREATESTRUCT)lParam)->lpCreateParams);

  PVideoOutputDevice_Window * vodw = (PVideoOutputDevice_Window *)GetWindowLong(hWnd, 0);
  if (vodw != NULL)
    return vodw->WndProc(uMsg, wParam, lParam);

  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


static int GetTokenValue(const PString & deviceName, const char * token, int defaultValue)
{
  PINDEX pos = deviceName.Find(token);
  if (pos == P_MAX_INDEX)
    return defaultValue;

  pos += strlen(token);
  return deviceName.Mid(pos).AsInteger();
}


void PVideoOutputDevice_Window::HandleDisplay(PThread &, INT)
{
#ifndef _WIN32_WCE
  static const char wndClassName[] = "PVideoOutputDevice_Window";
#else
  static LPCWSTR wndClassName = L"PVideoOutputDevice_Window";
#endif

  static bool needRegistration = true;
  if (needRegistration) {
    needRegistration = false;

    WNDCLASS wndClass;
    memset(&wndClass, 0, sizeof(wndClass));
    wndClass.style = CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
    wndClass.lpszClassName = wndClassName;
    wndClass.lpszClassName = wndClassName;
    wndClass.lpfnWndProc = ::WndProc;
    wndClass.cbWndExtra = sizeof(this);
    PAssertOS(RegisterClass(&wndClass));
  }

  DWORD dwStyle;
  HWND hParent;
  if (ParseWindowDeviceName(deviceName, &dwStyle, &hParent)) {
    PString title = DEFAULT_TITLE;

    PINDEX pos = deviceName.Find("TITLE=\"");
    if (pos != P_MAX_INDEX) {
      pos += 6;
      PINDEX quote = deviceName.FindLast('"');
      PString quotedTitle = deviceName(pos, quote > pos ? quote : P_MAX_INDEX);
      title = PString(PString::Literal, quotedTitle);
    }

    m_lastPosition.x = GetTokenValue(deviceName, "X=", CW_USEDEFAULT);
    m_lastPosition.y = GetTokenValue(deviceName, "Y=", CW_USEDEFAULT);
    m_fixedSize.cx   = GetTokenValue(deviceName, "WIDTH=", 0);
    m_fixedSize.cy   = GetTokenValue(deviceName, "HEIGHT=", 0);

    if (m_lastPosition.x == CW_USEDEFAULT && m_lastPosition.y == CW_USEDEFAULT) {
      if (hParent != NULL) {
        RECT rect;
        GetWindowRect(hParent, &rect);
        m_lastPosition.x = (rect.right + rect.left - frameWidth)/2;
        m_lastPosition.y = (rect.bottom + rect.top - frameHeight)/2;
      }
      else {
        m_lastPosition.x = (GetSystemMetrics(SM_CXSCREEN) - frameWidth)/2;
        m_lastPosition.y = (GetSystemMetrics(SM_CYSCREEN) - frameHeight)/2;
      }
    }

    PVarString windowTitle = title;
    m_hWnd = CreateWindow(wndClassName,
                          windowTitle, 
                          dwStyle,
                          m_lastPosition.x, m_lastPosition.y, frameWidth, frameHeight,
                          hParent, NULL, GetModuleHandle(NULL), this);
    SetWindowSize();
  }

  m_started.Signal();

  if (m_hWnd != NULL) {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
      DispatchMessage(&msg);
  }
}


LRESULT PVideoOutputDevice_Window::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  PWaitAndSignal m(mutex);

  switch (uMsg) {
    case WM_PAINT :
      {
        PAINTSTRUCT paint;
        HDC hDC = BeginPaint(m_hWnd, &paint);
        Draw(hDC);
        EndPaint(m_hWnd, &paint);
        break;
      }

    case WM_MOVE :
      if (m_hWnd != NULL) {
        RECT rect;
        GetWindowRect(m_hWnd, &rect);
        m_lastPosition.x = rect.left;
        m_lastPosition.y = rect.top;
      }
      break;

    case WM_CLOSE :
      DestroyWindow(m_hWnd);
      m_hWnd = NULL;
      break;

    case WM_DESTROY:
      PostThreadMessage(GetCurrentThreadId(), WM_QUIT, 0, 0);
      break;
  }
  return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}

#endif // P_VIDEO



// End Of File ///////////////////////////////////////////////////////////////
