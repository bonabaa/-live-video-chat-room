
#ifndef _BYTE_BLOCK_H_
#define _BYTE_BLOCK_H_

#include <stdio.h>
#include <stdlib.h>
#include <ptlib.h>

class PByteBlock {

protected:
	int		m_nMaxSize;
	char *	m_pBuf;
	int		m_nBufSize;
	int		m_nDataLen;
	PMutex m_mutexReadBuf;
public:
	PByteBlock ( const char * pData = NULL, int nLen = 0, int nBufSize = 0, int nMaxSize = 65536 );
	~PByteBlock ();

	bool PushBack( const char * pData, int nLen );

	bool PushFront( const char * pData, int nLen );

	int PopFront( int nLen );

	inline int PopMessage( SIM_MESSAGE * msg ) {
    PWaitAndSignal lock(m_mutexReadBuf);
		char * pDest = (char *) msg;
		int nBuf = sizeof(SIM_MESSAGE);
		return PopMessage( pDest, nBuf, m_pBuf, m_nDataLen, 1 );
	}

	inline int PopMessage( char* pDest, int nBuf ) {
		return PopMessage( pDest, nBuf, m_pBuf, m_nDataLen );
	}

	void Clear( void );

	operator const char * ();
	int DataLen( void );

	// return bytes copied out,
	// return 0 if not enough data for 1 message
	// return -1 if the data is invalid,
	// return -2 if the buffer out is too small for 1 message
	static int PopMessage( char* & pDest, int & nBuf, char* & pSrc, int & nDataLen, int nMaxMsg = 256 );

};

#endif // _BYTE_BLOCK_H_
