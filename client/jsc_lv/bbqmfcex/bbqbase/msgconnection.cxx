
#include "bbqbase.h"

BBQMsgConnection::BBQMsgConnection( bool bIncoming, PTCPSocket * pSock, bool bAutoDelete )
{
	if( pSock ) {
		m_pSock = pSock;
	} else {
		m_pSock = new PTCPSocket;
	}
  //int fd= m_pSock->GetHandle();
  //SetSocketOptions(m_pSock->GetHandle());
//const int one = 1;
//			if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one)))
//			{
//        PTRACE(3, "setocket failed");
//				//return VFalse;
//			}
	m_bIncoming = bIncoming;
	m_dwPrivateData = 0;

	m_nReadBytes = 0;
	m_msLastActionTime = BBQMsgTerminal::GetHostUpTimeInMs();

	m_bAutoDelete = bAutoDelete;
}

void BBQMsgConnection::UpdateTimestamp( void )
{
	m_msLastActionTime = BBQMsgTerminal::GetHostUpTimeInMs();
}


bool BBQMsgConnection::IsIdle( DWORD s )
{
	//return ( BBQMsgTerminal::GetHostUpTimeInMs() - m_msLastActionTime > 70000 );
	return ( BBQMsgTerminal::GetHostUpTimeInMs() - m_msLastActionTime > s );
}

BBQMsgConnection::~BBQMsgConnection()
{
	if( m_pSock ) {
		delete m_pSock;
	}

	while( m_QueueWrite.size() > 0 ) {
		PByteBlock * pdata = m_QueueWrite.front();
		m_QueueWrite.pop_front();
		delete pdata;
	}
}

//std::basic_ios

bool BBQMsgConnection::PickMessageFromBuffer( SIM_MESSAGE * msg )
{
	if( m_nReadBytes >= sizeof(msg->simHeader) ) {
		SIM_MESSAGE * src = (SIM_MESSAGE *) m_ReadBuf;
			if( src->simHeader.magic != SIM_MAGIC ) { // it's a bad connection, disconnect
				PTRACE( 1, "warning: message header MAGIC is error. Bad connection? Disconnect." ); 
        msg->simHeader.id = 10000;
        msg->simHeader.size =0;
        m_nReadBytes =0;
         return false;
				//m_pSock->Close(); chenyuan
				//return false;
			}
			int msgtotalsize = sizeof(src->simHeader) + src->simHeader.size;
			if( m_nReadBytes >= msgtotalsize ) {
				// read out a msg
				memcpy( msg, src, msgtotalsize );
				m_msLastActionTime = BBQMsgTerminal::GetHostUpTimeInMs();

				// move unused data for next time
				m_nReadBytes -= msgtotalsize;
				if( m_nReadBytes > 0 ) {
					memmove( m_ReadBuf, m_ReadBuf + msgtotalsize, m_nReadBytes );
				}

				return true;
			}
	}

	return false;
}

bool BBQMsgConnection::ReadMessage( SIM_MESSAGE * msg )
{
	// we should check in the buffer before read new data
	if( PickMessageFromBuffer( msg ) ) {
		// has data, so update timestamp
		m_msLastActionTime = BBQMsgTerminal::GetHostUpTimeInMs();
		return true;
	}

	// not enough data, we should fill the buffer by reading network
	int nBufLeft = MSGCONNECTION_READBUFSIZE - m_nReadBytes;
	if( m_pSock->Read( m_ReadBuf + m_nReadBytes, nBufLeft ) ) {
		// data coming, so update timestamp
		m_msLastActionTime = BBQMsgTerminal::GetHostUpTimeInMs();

		m_nReadBytes += m_pSock->GetLastReadCount();

		// now try again
		return PickMessageFromBuffer( msg );
	} else {
		// let outside handle the error
	}

	return false;
}

// non-block replacement of PTCPSocket::Write(...)
int BBQMsgConnection::Write( const char * pBuf, int nLen )
{
	int nWritten = 0;

	if( nLen > 0 ) {
		int fd = m_pSock->GetHandle();
		fd_set writefds;
		FD_ZERO( & writefds );
		FD_SET( fd, & writefds );
		struct timeval tv = { 0, 0 };
		int selval = ::select(fd+1, NULL, & writefds, NULL, & tv);
		if( selval > 0 ) {
			// it is writtable, let's do it
			nWritten = ::sendto(fd, pBuf, nLen, 0, NULL, 0);
			if( nWritten <= 0 ) nWritten = 0;
		} else if ( selval == 0 ) {
			// blocked, we will try in future
		} else {
			// socket is error
		}
	}

	return nWritten;
}

bool BBQMsgConnection::Queue( const char * pBuf, int nLen, bool bForce )
{
	if( ! bForce ) {
		if( m_QueueWrite.size() >= MSGCONNECTION_MSGQUEUEMAX ) {
			PTRACE( 1, "warning: msg connection queue too long, drop message." );
			return false;
		}
	}

	m_QueueWrite.push_back( new PByteBlock( pBuf, nLen ) );

	return true;
}

bool BBQMsgConnection::WriteMessage( const SIM_MESSAGE * msg, bool bBlockMode )
{
	int nLen = SIM_PMSGSIZE(msg);

	if( bBlockMode ) {
		if( ! FlushMessage( true ) ) {
			return false;
		}

		if( ! m_pSock->Write( (const char *) msg, nLen ) ) {
			return false;
		}
	} else {
		if( ! FlushMessage( false ) ) 
			return Queue( (const char *) msg, nLen );

		int nWritten = Write( (const char *) msg, nLen );

		if( nWritten < nLen ) 
			return Queue( (const char *)msg + nWritten, (nLen - nWritten), (nWritten > 0)  );
	}

	m_msLastActionTime = BBQMsgTerminal::GetHostUpTimeInMs();

	return true;
}

bool BBQMsgConnection::FlushMessage( bool bBlockMode )
{
	while( m_QueueWrite.size() > 0 ) {
		PByteBlock *pdata = m_QueueWrite.front();
		int nLen = pdata->DataLen();

		if( bBlockMode ) {
			if( m_pSock->Write( (const char *)(* pdata), nLen ) ) {

				// some data is written out, so update the timestamp
				m_msLastActionTime = BBQMsgTerminal::GetHostUpTimeInMs();

				m_QueueWrite.pop_front();
				delete pdata;

				continue;
			} break;
		} else {
			int nWritten = Write( (const char *)(* pdata), nLen );
			if( nWritten == 0 ) return false;

			// some data is written out, so update the timestamp
			m_msLastActionTime = BBQMsgTerminal::GetHostUpTimeInMs();

			if( nWritten == nLen ) {
				m_QueueWrite.pop_front();
				delete pdata;
				continue;
			} else if( nWritten > 0 ) {
				pdata->PopFront( nWritten );
				return false;
			}
		}
	}

	return true;
}

