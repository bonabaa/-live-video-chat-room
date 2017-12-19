#ifndef __SF_VIDEO_CAMERAUTILSG__H
#define __SF_VIDEO_CAMERAUTILSG__H

#include <ptlib.h>
#include <ptbuildopts.h>

#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptlib/pluginmgr.h>
#include <ptclib/delaychan.h>

#ifdef __MINGW32__
#include <mingw_dshow_port.h>
#endif

#include <windows.h>
#include "atlbase.h"
#include <ddraw.h>
#include <dshow.h>
#include <uuids.h>
#include <control.h>

#ifdef _WIN32_WCE
#include <amvideo.h>
#endif

#ifndef ASSERT
#define ASSERT(f)          (f)
#endif

BOOL WINAPI BBQIsEqualObject(IUnknown *pFirst, IUnknown *pSecond);
void WINAPI BBQFreeMediaType(AM_MEDIA_TYPE& mt);
void WINAPI BBQDeleteMediaType(AM_MEDIA_TYPE *pmt);
HRESULT WINAPI BBQCreateMediaType(AM_MEDIA_TYPE *pmtTarget, const AM_MEDIA_TYPE *pmtSource);
AM_MEDIA_TYPE * WINAPI BBQCreateMediaType(AM_MEDIA_TYPE const *pSrc);

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }
#endif


struct DXVideoFormat
{ 
	WORD  bitCount;
	BOOL negHeight; 
	DWORD compression; 
	const GUID*  guid;
}; 

IPin * GetInPin( IBaseFilter * pFilter, int Num );
IPin * GetOutPin( IBaseFilter * pFilter, int Num );
IPin* GetOutVideoPin( IBaseFilter * pFilter, int Num );
HRESULT GetPin( IBaseFilter * pFilter, PIN_DIRECTION dirrequired, int iNum, IPin **ppPin);

DXVideoFormat* WINAPI GetFormat( const char * videoFormat );
DXVideoFormat* WINAPI GetFormat(const BITMAPINFOHEADER& bmh);
PString WINAPI GetFormatString(const BITMAPINFOHEADER& bmh);
#endif 