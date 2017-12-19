// PVideoInputDevice_DirectShowEx.h: interface for the PVideoInputDevice_DirectShowEx class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PDXVIDEOINPUTDEVICE_H__973EB45C_42B4_4C04_AFB4_33F15B0B5591__INCLUDED_)
#define AFX_PDXVIDEOINPUTDEVICE_H__973EB45C_42B4_4C04_AFB4_33F15B0B5591__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <ptlib.h>
#include <ptbuildopts.h>

#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptlib/pluginmgr.h>
#include <ptclib/delaychan.h>
#include <ptlib/thread.h> 

#ifdef __MINGW32__
#include <mingw_dshow_port.h>
#endif

#include <atlbase.h>
#include <dshow.h>
#include <mmsystem.h>
#include <strmif.h>
#include <amvideo.h>    
#include <uuids.h>    
#include <control.h>

#ifdef _WIN32_WCE
#include <amvideo.h>
#endif
#include "bbqCamera/SfVideoWndMsg.h"

class ISampleBuffer;

class PVideoInputDevice_DirectShowEx : public PVideoInputDevice  
{
	PCLASSINFO(PVideoInputDevice_DirectShowEx, PVideoInputDevice);
public:
	PVideoInputDevice_DirectShowEx();
	virtual ~PVideoInputDevice_DirectShowEx();

protected:
	virtual PBoolean Open(
		const PString & deviceName,   /// Device name to open
		PBoolean startImmediate = PTrue    /// Immediately start device
	);

	virtual PBoolean OpenOnly(
		const PString & deviceName,   /// Device name to open
		PBoolean startImmediate = PTrue    /// Immediately start device
	);
public:
    virtual PBoolean IsOpen();
    virtual PBoolean Close();

    virtual PBoolean Start();
    virtual PBoolean Stop();
    virtual PBoolean IsCapturing();

    virtual PStringArray GetDeviceNames() const;

    static PStringList GetInputDeviceNames();

    virtual PINDEX GetMaxFrameBytes();

    virtual PBoolean GetFrameData( BYTE * buffer, PINDEX * bytesReturned = NULL );
    virtual PBoolean GetFrameDataNoDelay( BYTE * buffer, PINDEX * bytesReturned );

	PBoolean IsSizeSupport(UINT nWidth, UINT nHeight);
    virtual PBoolean TestAllFormats();

	virtual DWORD GetError();

	PBoolean IsAplux();
	PBoolean GetCameraId(USHORT& vendorId, USHORT& productId);

	PBoolean HasOptionDialog(int nDialog);
	PBoolean ShowDialog(int nDialog, HWND hParent);
	int GetSupportedSize(SIZE* pSizes, int nCount);
	int GetSupportedFormat(DWORD* pFormats, int nCount);
protected:
  virtual PBoolean VerifyHardwareFrameSize(unsigned width, unsigned height);
  PDECLARE_NOTIFIER(PThread, PVideoInputDevice_DirectShowEx, HandleCapture);
public:
	virtual PBoolean SetTargetColourFormat(const PString & colourFormat, const PString & preferredFormat = "");
    virtual PBoolean SetColourFormat(const PString & colourFormat);
    virtual PBoolean SetFrameRate(unsigned rate);
    virtual PBoolean SetFrameSize(unsigned width, unsigned height);
    virtual PBoolean SetFrameSizeConverter(unsigned width, unsigned height, PBoolean bScaleNotCrop);
	virtual PBoolean GetSimliarSize(unsigned& nWidth, unsigned& nHeight);
	LRESULT DXHandleStatus(int nID, LPSTR lpStatusText);
protected:
    static LRESULT CALLBACK DXErrorHandler(HWND hWnd, int id, LPCSTR err);
    LRESULT DXHandleError(int id, LPCSTR err);
    static LRESULT CALLBACK DXVideoHandler(HWND hWnd, const ISampleBuffer* pData);
    LRESULT DXHandleVideo(const ISampleBuffer* pData);

    PBoolean Win32InitialiseCapture(PBoolean bOpenOnly = FALSE);
	PBoolean Win32HandleCapture(PBoolean bOpenOnly = PFalse);
	PBoolean Win32CaptureThreadEnd();

	PBoolean InnerStop();
	PBoolean m_bRealStop;
  PBoolean isCapturingNow; //add by chenyuan
  HWND hCaptureWindow; //add by chenyuan
  //PSyncPoint lastFrameMutex;//add by chenyuan
  PSyncPoint cameraStopMutex;//add by chenyuan
protected:
	//CWinThread*	win32captureThread;
  //PSyncPoint	win32threadStarted;
  PSyncPoint    threadStarted;

  PMutex        operationMutex;
  PThread     * captureThread;

	DWORD	m_lastError;

// Buffers
protected:
	int m_nBuffersSize;
	HANDLE m_hSemaphore;
	std::list<ISampleBuffer *>	m_lstBuffers;
	CComAutoCriticalSection	m_csBuffers;
	void DestroyBuffers();
public:
	void SetBuffersSize(int nBuffersSize);
public:
	virtual int IsPTZControlSupported();
	virtual int PTZControl(int nIndex, int nValue);
	virtual int PTZStringControl(const PString& strControlCommand);
	virtual int PTZWindow(LPSIZE size, LPRECT rect);
	
	int GetCameraProperties(LPTSTR lpszProperties, int nLen);
	PBoolean SetCameraProperties(LPCTSTR lpszProperties);

	PBoolean GetCameraProperty(LPCTSTR lpszProperty, LPTSTR pszValue);
	PBoolean SetCameraProperty(LPCTSTR lpszProperty, LPCTSTR lpszValue);
public:
	void SetPause(PBoolean bPause);
	PBoolean IsPause();
protected:
	PBoolean m_bPause;
public:
	friend class PDXVideoInputThread;
};

#endif // !defined(AFX_PDXVIDEOINPUTDEVICE_H__973EB45C_42B4_4C04_AFB4_33F15B0B5591__INCLUDED_)
