
#ifndef _BYTE_PACK_H
#define _BYTE_PACK_H

#include <ptlib.h>

class PBytePack : public PObject
{
	PCLASSINFO( PBytePack, PObject )

private:
	char * m_pPackBuffer, * m_pPackBufferEnd, * m_pPackPointer;
	const char * m_pUnpackBuffer, * m_pUnpackBufferEnd, * m_pUnpackPointer;

public:
	PBytePack();
	PBytePack( const void * pUnpackData, int nDataLen );
	~PBytePack();

	// pack & unpack, len is the exact size of data to be packed or unpacked
	// return: == len, if successful, == 0, if fail.
	int Pack( const void * data, int len );
	int Unpack( void * buf, int len );

	// pack & unpack a null-terminated string, including the '\0' at end of string
	// if str == NULL, a empty string will be packed.
	// return: bytes packed or unpacked, == 0, if failed
	int PackString( const char * str );
	int UnpackString( char * buf, int bufsize );

	const void * Data( void );
	int Size( void );

	int GetPackedData( void* buf, int bufsize );
};

#define SIMPLE_PACK(type) \
	inline PBytePack & operator<< ( PBytePack & pack, const type & v ) { pack.Pack(&v,sizeof(v)); return pack; } \
	inline PBytePack & operator>> ( PBytePack & pack, type & v ) { pack.Unpack(&v,sizeof(v)); return pack; }

SIMPLE_PACK( char )
SIMPLE_PACK( unsigned char )
SIMPLE_PACK( short )
SIMPLE_PACK( unsigned short )
SIMPLE_PACK( int )
SIMPLE_PACK( unsigned int )
SIMPLE_PACK( long )
SIMPLE_PACK( unsigned long )

#if defined(_WIN32) && (_MSC_VER < 1300)
SIMPLE_PACK( uint8 )
SIMPLE_PACK( uint16 )
SIMPLE_PACK( uint32 )
#endif

PBytePack & operator<< (PBytePack & pack, const char * & str);
PBytePack & operator>>(PBytePack & pack, char * & str);

inline PBytePack & operator<< (PBytePack & pack, PBytePack & sub) { pack.Pack(sub.Data(),sub.Size()); return pack; }

#endif // _BYTE_PACK_H
