
#include <string.h>
#include <ptlib/md5.h>

#include "bbqdatabase.h"

void MD5Encryption( char *pBuf, const char *pSrc)
{
	MD5 ctx;
	unsigned char digest[16];

	//char src[256];
	//memset( src, 0, 256);
	//strcpy( src, pSrc );
	//ctx.update( (unsigned char*)src, strlen(pSrc));

	ctx.update( (unsigned char *) pSrc, strlen(pSrc) );
	ctx.finalize();
	ctx.raw_digest( digest);

	for( int i = 0; i < 16; i++ )
	{
		sprintf( pBuf+i*2, "%02x", digest[i]);
	}
}

