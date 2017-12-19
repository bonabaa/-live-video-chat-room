
#include "bbqbase.h"

PByteBlock::PByteBlock ( const char * pData, int nLen, int nBufSize, int nMaxSize ) 
{
	m_pBuf = NULL;
	m_nBufSize = 0;
	m_nDataLen = 0;
	m_nMaxSize = nMaxSize;

	if( nBufSize < nLen ) nBufSize = nLen;
	if( nBufSize > 0 ) {
		m_pBuf = new char[ nBufSize ];
		if( m_pBuf ) {
			m_nBufSize = nBufSize;
			if( pData && (nLen > 0) ) {
				memcpy( m_pBuf, pData, nLen );
				m_nDataLen = nLen;
			}
		}
	}
}

PByteBlock::~PByteBlock () 
{
	if( m_pBuf ) delete m_pBuf;
}

bool PByteBlock::PushBack( const char * pData, int nLen ) 
{
  PWaitAndSignal lock(m_mutexReadBuf);
	int n = m_nDataLen + nLen;
	if( n > m_nMaxSize ) return false;
	if( n > m_nBufSize ) {
		char * pBuf = new char[ n ];
		if( ! pBuf ) return false;
		if( m_pBuf ) {
			if( m_nDataLen > 0 ) memcpy( pBuf, m_pBuf, m_nDataLen );
			delete m_pBuf;
		}
		m_pBuf = pBuf;
		m_nBufSize = n;
	}
	memcpy( m_pBuf + m_nDataLen, pData, nLen );
	m_nDataLen += nLen;
	return true;
}

int PByteBlock::PopFront( int nLen ) 
{
	if( m_pBuf && (m_nDataLen > nLen) ) {
		if(nLen>0) memmove( m_pBuf, m_pBuf + nLen, m_nDataLen - nLen );
		m_nDataLen -= nLen;
		return nLen;
	} else {
		m_nDataLen = 0;
	}

	return 0;
}

int PByteBlock::PopMessage( char* & pDest, int & nBuf, char* & pSrc, int & nDataLen, int nMaxMsg )
{
	if( pDest && pSrc ) {
		SIM_MESSAGE * pMsg = NULL;
		char * p = pSrc; int n = nDataLen; int m = nBuf;
		for( int ii=0; ii<nMaxMsg; ii++ ) {
			// check message header 
			if( n >= sizeof(SFIDMSGHEADER) ) {
				pMsg = (SIM_MESSAGE *) p;
				if( pMsg->simHeader.magic != SIM_MAGIC ) {
#if 1
					// bad data for message
					return -1;
#else
          while (nDataLen>0 ){
  				  nDataLen -= 1;
				    memmove( pSrc, pSrc + 1, nDataLen );
            pMsg = (SIM_MESSAGE *) p;
            if ( pMsg->simHeader.magic != SIM_MAGIC || pMsg->simHeader.size + sizeof(SFIDMSGHEADER)< nDataLen ){
              SIM_MESSAGE * pMsg2 = (SIM_MESSAGE *)(((char*)pMsg) + pMsg->simHeader.size + sizeof(SFIDMSGHEADER));
              if (pMsg2->simHeader.magic != SIM_MAGIC ) {
                continue;
              } 
            }
          }
				 

#endif
        }/*else if ( pMsg->simHeader.size + sizeof(SFIDMSGHEADER)< nDataLen ){
          SIM_MESSAGE * pMsg2 = (SIM_MESSAGE *)(((char*)pMsg) + pMsg->simHeader.size + sizeof(SFIDMSGHEADER));
          if (pMsg2->simHeader.magic != SIM_MAGIC ) {
  				  nDataLen -= 1;
				    memmove( pSrc, pSrc + 1, nDataLen );
           
					  continue;
          } 
        }*/

				int nMsgSize = sizeof(SFIDMSGHEADER) + pMsg->simHeader.size;

				if( m >= nMsgSize ) {
					if ( n >= nMsgSize ) {
						p += nMsgSize;
						n -= nMsgSize;
						m -= nMsgSize;
					} else {
						// we get the header, but the message body not fully arrive
						break;
					}
				} else { // buffer not enough for this msg
					if( m < nBuf ) {
						// already pick some message
						break;
					} else {
						// no message picked, the original buffer is too small
						return -2; 
					}
				}
			} else {
				// the message not fully arrive
				break;
			}
		}

		if( p > pSrc ) {
			int nLen = (int)( p - pSrc );
			if( (nLen <= nBuf) && (nLen <= nDataLen) ) {
				memcpy( pDest, pSrc, nLen );
				nDataLen -= nLen;
				memmove( pSrc, pSrc + nLen, nDataLen );
				return nLen;
			} else {
				// impossible according to the logic
			}

		} else return 0;
	}

	return -1;
}

bool PByteBlock::PushFront( const char * pData, int nLen ) 
{
	int n = m_nDataLen + nLen;
	if( n > m_nMaxSize ) return false;
	if( n > m_nBufSize ) {
		char * pBuf = new char[ n ];
		if( ! pBuf ) return false;
		if( m_pBuf ) {
			memcpy( pBuf, m_pBuf, m_nDataLen );
			delete m_pBuf;
		}
		m_pBuf = pBuf;
		m_nBufSize = n;
	}
	memmove( m_pBuf + nLen, m_pBuf, m_nDataLen );// move orginal data to back
	memcpy( m_pBuf, pData, nLen );
	m_nDataLen += nLen;
	return true;
}

void PByteBlock::Clear( void ) 
{
	m_nDataLen = 0;
}

PByteBlock::operator const char * () 
{
	return m_pBuf;
}

int PByteBlock::DataLen( void ) 
{
	return m_nDataLen;
}
