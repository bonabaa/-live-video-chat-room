
#include <ptlib.h>

#include "bbqCameraUtils.h"

const GUID MEDIASUBTYPE_I420 = {
	MAKEFOURCC( 'I', '4', '2', '0' ), 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71};

const GUID MEDIASUBTYPE_H264 = {
	MAKEFOURCC( 'H', '2', '6', '4' ), 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71};

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
	{ "MJPEG",	{12,	FALSE,	MAKEFOURCC('M','J','P','G'),&MEDIASUBTYPE_MJPG } },
	{ "H264",	{32,	FALSE,	MAKEFOURCC('H','2','6','4'),&MEDIASUBTYPE_H264 } },
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

DXVideoFormat* WINAPI GetFormat(const BITMAPINFOHEADER& bmh)
{
	for( int i=0; i < sizeof(FormatTable) / sizeof(FormatTable[0]); ++i ) {
		if( (bmh.biCompression == FormatTable[i].format.compression) && 
			(bmh.biBitCount == FormatTable[i].format.bitCount) &&
			( (FormatTable[i].format.negHeight && bmh.biHeight < 0) ||
			  (!FormatTable[i].format.negHeight && bmh.biHeight > 0) 
			 )
		   ){
			return &FormatTable[i].format;
		}
	}
	return NULL;
}

PString WINAPI GetFormatString(const BITMAPINFOHEADER& bmh)
{
	for( int i=0; i < sizeof(FormatTable) / sizeof(FormatTable[0]); ++i ) {
		if( (bmh.biCompression == FormatTable[i].format.compression) && 
			(bmh.biBitCount == FormatTable[i].format.bitCount) &&
			( (FormatTable[i].format.negHeight && bmh.biHeight < 0) ||
			  (!FormatTable[i].format.negHeight && bmh.biHeight > 0) 
			 )
		   ){
			return FormatTable[i].videoFormat;
		}
	}
	return "";
}

BOOL WINAPI BBQIsEqualObject(IUnknown *pFirst, IUnknown *pSecond)
{
	/*  Different objects can't have the same interface pointer for
	any interface
	*/
	if (pFirst == pSecond) {
		return TRUE;
	}
	/*  OK - do it the hard way - check if they have the same
	IUnknown pointers - a single object can only have one of these
	*/
	LPUNKNOWN pUnknown1;     // Retrieve the IUnknown interface
	LPUNKNOWN pUnknown2;     // Retrieve the other IUnknown interface
	HRESULT hr;              // General OLE return code

	ASSERT(pFirst);
	ASSERT(pSecond);

	/* See if the IUnknown pointers match */

	hr = pFirst->QueryInterface(IID_IUnknown,(void **) &pUnknown1);
	ASSERT(SUCCEEDED(hr));
	ASSERT(pUnknown1);

	hr = pSecond->QueryInterface(IID_IUnknown,(void **) &pUnknown2);
	ASSERT(SUCCEEDED(hr));
	ASSERT(pUnknown2);

	/* Release the extra interfaces we hold */

	pUnknown1->Release();
	pUnknown2->Release();
	return (pUnknown1 == pUnknown2);
}

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


//  Copy 1 media type to another

HRESULT WINAPI BBQCreateMediaType(AM_MEDIA_TYPE *pmtTarget, const AM_MEDIA_TYPE *pmtSource)
{
    //  We'll leak if we copy onto one that already exists - there's one
    //  case we can check like that - copying to itself.
    ASSERT(pmtSource != pmtTarget);
    *pmtTarget = *pmtSource;
    if (pmtSource->cbFormat != 0) {
        ASSERT(pmtSource->pbFormat != NULL);
        pmtTarget->pbFormat = (PBYTE)CoTaskMemAlloc(pmtSource->cbFormat);
        if (pmtTarget->pbFormat == NULL) {
            pmtTarget->cbFormat = 0;
            return E_OUTOFMEMORY;
        } else {
            CopyMemory((PVOID)pmtTarget->pbFormat, (PVOID)pmtSource->pbFormat,
                       pmtTarget->cbFormat);
        }
    }
    if (pmtTarget->pUnk != NULL) {
        pmtTarget->pUnk->AddRef();
    }

    return S_OK;
}

AM_MEDIA_TYPE * WINAPI BBQCreateMediaType(AM_MEDIA_TYPE const *pSrc)
{
    ASSERT(pSrc);

    // Allocate a block of memory for the media type

    AM_MEDIA_TYPE *pMediaType =
        (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));

    if (pMediaType == NULL) {
        return NULL;
    }
    // Copy the variable length format block

    HRESULT hr = BBQCreateMediaType(pMediaType,pSrc);
    if (FAILED(hr)) {
        CoTaskMemFree((PVOID)pMediaType);
        return NULL;
    }

    return pMediaType;
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

IPin* GetOutAudioPin( IBaseFilter * pFilter, int iNum )
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
					if( pMediaType->majortype == MEDIATYPE_Audio ) {
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
