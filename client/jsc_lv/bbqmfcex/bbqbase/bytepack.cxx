
#include "bbqbase.h"

#define BYTEPACK_INIT_SIZE		4096
#define BYTEPACK_INC_SIZE		1024

PBytePack::PBytePack ( const void * pUnpackData, int nDataLen )
{
	if( pUnpackData ) { // readonly

		m_pUnpackPointer = m_pUnpackBuffer = (const char *) pUnpackData;
		m_pUnpackBufferEnd = m_pUnpackBuffer + nDataLen;

		m_pPackPointer = m_pPackBufferEnd = m_pPackBuffer = NULL;

	} else { // writable pack object

		m_pPackPointer = m_pPackBuffer = new char[ BYTEPACK_INIT_SIZE ];
		m_pPackBufferEnd = m_pPackBuffer + BYTEPACK_INIT_SIZE;

		m_pUnpackBufferEnd = m_pUnpackBuffer = m_pUnpackPointer = m_pPackBuffer;
	}
}

PBytePack::PBytePack()
{
	// writable pack object 

	m_pPackPointer = m_pPackBuffer = new char[ BYTEPACK_INIT_SIZE ];
	m_pPackBufferEnd = m_pPackBuffer + BYTEPACK_INIT_SIZE;

	m_pUnpackBufferEnd = m_pUnpackBuffer = m_pUnpackPointer = m_pPackBuffer;
}

PBytePack::~PBytePack()
{
	if( m_pPackBuffer != NULL ) {
		delete[] m_pPackBuffer;
	}
}

int PBytePack::Pack( const void * data, int len )
{
	if( ! m_pPackBuffer ) {
		throw "pack: readonly";
		return 0;
	}

	if( m_pPackPointer + len > m_pPackBufferEnd ) {
		int inc = ((len/BYTEPACK_INC_SIZE) + 1) * BYTEPACK_INC_SIZE;
		int bufsize = m_pPackBufferEnd - m_pPackBuffer + inc;
		char* tmp = new char[ bufsize ];
		if( ! tmp ) {
			throw "pack: no memory";
			return 0;
		}
		int datasize = m_pPackPointer - m_pPackBuffer;
		if( m_pPackBuffer ) {
			if( datasize > 0 ) memcpy( tmp, m_pPackBuffer, datasize );
			delete[] m_pPackBuffer;
		}

		// change the pack pointers
		m_pPackBuffer = tmp;
		m_pPackBufferEnd = m_pPackBuffer + bufsize;
		m_pPackPointer = m_pPackBuffer + datasize;

		// also change the unpack pointers
		m_pUnpackPointer = m_pPackBuffer + (m_pUnpackPointer - m_pUnpackBuffer);
		m_pUnpackBuffer = m_pPackBuffer;
		m_pUnpackBufferEnd = m_pPackPointer;
	}

	memcpy( m_pPackPointer, data, len );
	m_pPackPointer += len;
	m_pUnpackBufferEnd = m_pPackPointer;

	return len;
}

int PBytePack::Unpack( void * buf, int len )
{
	if( m_pUnpackBufferEnd - m_pUnpackPointer < len ) {
		throw "unpack: reach end";
		return 0;
	}

	if( len > 0 ) {
		memcpy( buf, m_pUnpackPointer, len );
		m_pUnpackPointer += len;
	}

	return len;
}

int PBytePack::PackString( const char * str )
{
	if( str ) {
		return Pack( str, strlen(str) + 1 );
	} else {
		return Pack( "", 1 );
	}
}

int PBytePack::UnpackString( char * buf, int bufsize )
{
	const char * p = m_pUnpackPointer;
	while( p<m_pUnpackBufferEnd && *p++ );
	if( p > m_pUnpackBufferEnd ) { 
		throw "unpack: reach end";
		return 0;
	}

	int len = p - m_pUnpackPointer;

	int copylen = min( len, bufsize );
	if( copylen > 0 ) {
		memcpy( buf, m_pUnpackPointer, copylen );
		buf[ copylen-1 ] = '\0';
	}
	m_pUnpackPointer += len;
	return copylen;
}

const void * PBytePack::Data( void )
{ 
	return m_pUnpackBuffer; 
}

int PBytePack::Size( void ) 
{ 
	return (int)(m_pUnpackBufferEnd - m_pUnpackBuffer);
}

int PBytePack::GetPackedData( void* buf, int bufsize )
{
	int n = m_pUnpackBufferEnd - m_pUnpackBuffer;
	n = min( n, bufsize );
	if( n > 0 ) {
		memcpy( buf, m_pUnpackBuffer, n );
	}
	return n;
}

// ------------------------------ Pack Utility functions ----------------------------

PBytePack & operator<< (PBytePack & pack, const char * & str)
{
	pack.Pack( & str, sizeof(str) );
	if( str ) {
		int len = strlen( str );
		pack << len;
		pack.Pack( str, len );
	}
	return pack;
}

PBytePack & operator>>(PBytePack & pack, char * & str)
{
	pack.Unpack( & str, sizeof(str) );
	if( str ) {
		int len = 0;
		pack >> len;
		if( len >= 0 ) {
			str = new char[ len + 1 ];
			pack.Unpack( str, len );
			str[ len ] = '\0';
		} else {
			throw "unpack: invalid string";
			str = NULL;
		}
	}
	return pack;
}

