
#ifdef _WIN32

//// define max size of fd_set, so that can handle large number of sockets
#ifdef FD_SETSIZE
#error Important: do not use precompiled header file for this .cxx file
#else
#define FD_SETSIZE      4096		// default is 64
#endif /* FD_SETSIZE */

#endif

#include "bbqbase.h"
#include "bbqproxy.h"

#define		FWD_WHOLE_MESSAGE
/* -----------------------------------------------------------

why forward whole message?

we must make sure every packet forwarded is useful. so, we always forward entire message. if the message is not a entire message,
we put it in our read buffer, and wait for next time.

-------------------------------------------------------------- */

//
// Bug info: 1 parameter (message packet size) was adjusted from 1.2KB to 4KB for SIP module compatibility, 
// but channel buffer in the proxy engine is not adjusted (8KB). 
// Then the buffer will be always too small and cause error. Now adjusted to 12KB, Okay now.
//


BBQProxy * BBQProxy::m_pCurrentProxy = NULL;

BBQProxy::Config BBQProxy::m_config;

PString BBQProxy::TypeToString( int n )
{
	switch( n ) {
	case PROXY_UDP: return "UDP";
	case PROXY_TCP: return "TCP";
	case PROXY_SSL: return "SSL";
	}

	return "NONE";
}

BBQProxy::Entry::Entry(BBQProxy & owner)
	: m_ReadBuf(NULL, 0, BBQPROXY_BUF_MAX, BBQPROXY_BUF_MAX), m_owner(owner)
{
	m_nChannelId = 0;
	m_uid = 0;
	m_cookie = 0;
	m_peer = NULL;
	
	m_timestamp = 0;

	m_bIdentified = false;
	m_bConnected = false;

	//m_bTCP = false;
	//m_pSock = NULL;

	m_nProxyType = PROXY_NONE;
	m_pUdpSock = NULL;
	m_pTcpSock = NULL;
	m_pSSLChannel = NULL;

	m_nPort = 0;
	memset( & m_addr, 0, sizeof(m_addr) );

	m_nQueueBytes = 0;

	m_fpTrafficLog = NULL;
	m_fpTrafficData = NULL;
}

BBQProxy::Entry::~Entry()
{
	// release the socket object
	if( m_pUdpSock ) delete m_pUdpSock;
	if( m_pTcpSock ) delete m_pTcpSock;
	if( m_pSSLChannel ) delete m_pSSLChannel;

	if( m_fpTrafficLog ) {
		if( m_bChannelSource ) fclose( m_fpTrafficLog );
		m_fpTrafficLog = NULL;
	}
	if( m_fpTrafficData ) {
		fclose( m_fpTrafficData );
		m_fpTrafficData = NULL;
	}
}

void BBQProxy::Entry::Close( void )
{
	switch( m_nProxyType ) {
	case PROXY_UDP:
		if( m_pUdpSock ) m_pUdpSock->Close();
		break;
	case PROXY_TCP:
		if( m_pTcpSock ) m_pTcpSock->Close();
		break;
	case PROXY_SSL:
		if( m_pSSLChannel ) m_pSSLChannel->Close();
		if( m_pTcpSock ) m_pTcpSock->Close();
		break;
	}
	m_ReadBuf.Clear();
	m_queueWrite.Clear();
	m_timestamp = 0;

	if( m_fpTrafficLog ) {
		if( m_bChannelSource ) fclose( m_fpTrafficLog );
		m_fpTrafficLog = NULL;
	}
	if( m_fpTrafficData ) {
		fclose( m_fpTrafficData );
		m_fpTrafficData = NULL;
	}
}

int BBQProxy::Entry::GetHandle( void )
{
	switch( m_nProxyType ) {
	case PROXY_UDP:
		if( m_pUdpSock ) return m_pUdpSock->GetHandle();
		break;
	case PROXY_TCP:
		if( m_pTcpSock ) return m_pTcpSock->GetHandle();
		break;
	case PROXY_SSL:
		if( m_pSSLChannel ) return m_pSSLChannel->GetHandle();
		break;
	}

	return -1;
}

inline int BBQProxy::Entry::Read( char * pBuf, int nReadBuf )
{
	bool bRead = false;
	int nRead = 0;

	int nBuf = nReadBuf;

#ifdef FWD_WHOLE_MESSAGE
	// maybe part of 1.999 message is in m_ReadBuf, so we must use less buffer for new data
	nBuf -= sizeof(SIM_MESSAGE) + sizeof(SIM_MESSAGE);
#endif

	switch( m_nProxyType ) {
	case PROXY_UDP:
		{
#ifdef NEVER_BLOCK_THREAD
			int fd = m_pUdpSock->GetHandle();
			DWORD available = 0;
			if( ::ioctlsocket( fd, FIONREAD, & available ) != 0 ) return nRead;
			if( available == 0 ) {
				fd_set readfds;
				FD_ZERO( & readfds );
				FD_SET( fd, & readfds );
				struct timeval tv = { 0, 0 };
				int selval = ::select(fd+1, & readfds, NULL, NULL, & tv);
				if( selval > 0 ) {
					// it is readable, let's do it
				} else if ( selval == 0 ) {
					// blocked, we will try in future
					return nRead;
				} else {
					// socket is error
					return nRead;
				}
				if( ::ioctlsocket( fd, FIONREAD, & available ) != 0 ) return nRead;
			}
			sockaddr_in sockAddr;
			socklen_t nlen = sizeof(sockAddr);
			int n = ::recvfrom( fd, pBuf, nBuf, 0, (sockaddr*)&sockAddr, &nlen );
			if( (sockAddr.sin_addr.s_addr == m_addr.ip) && (sockAddr.sin_port == htons( m_addr.port )) ) {
				if( n > 0 ) nRead = n;
			}
#else
			PIPSocket::Address addr; 
			WORD port;
			
			if( m_pUdpSock->ReadFrom( pBuf, nBuf, addr, port ) ) {
				if( (DWORD)addr == m_addr.ip && port == m_addr.port ) {
					nRead = m_pUdpSock->GetLastReadCount();
					
					return nRead;
				} // else // UDP data from other address, might be hacker's attack, drop it
			}
			/*
			int fd = m_pUdpSock->GetHandle();
			sockaddr_in sockAddr;
			socklen_t nlen = sizeof(sockAddr);
			nRead = ::recvfrom( fd, pBuf, nBuf, 0, (sockaddr*)&sockAddr, &nlen );
      */
			return nRead;
#endif
		}
		break;
	case PROXY_TCP:
		{
			bRead = m_pTcpSock->Read( pBuf, nBuf );
			nRead = m_pTcpSock->GetLastReadCount();
			if( bRead ) {
				// 
			} else {
				if( nRead > 0 ) {
					// impossible to reach here
				} else if( (nRead == 0) && ( m_pTcpSock->GetErrorCode( PChannel::LastReadError ) ==  PChannel::Timeout ) ) {
					// no data coming
				} else {
					// now do not flush, cause the channel is broken, data loss is reasonable
					//int n = m_peer->Flush();
					int nErrorCode = m_pTcpSock->GetErrorCode( PChannel::LastReadError );

					PString strMsg( PString::Printf, "TCP channel %d from %d closed, error: %d, %d, %s. now closing peer %d.",
						this->m_nChannelId,
						this->m_uid, 
						nErrorCode,
						m_pTcpSock->GetErrorNumber( PChannel::LastReadError ), 
						(const char *)m_pTcpSock->GetErrorText( PChannel::LastReadError ),
						this->m_peer->m_uid );
					PLOG( & m_owner.m_UserActionLog, Info, strMsg );

					this->m_owner.StatChannel( true, (nErrorCode!=0), this->m_tInit, BBQMsgTerminal::GetHostUpTimeInMs() );

					this->Close();
					m_peer->Close();
					this->m_timestamp = m_peer->m_timestamp = 0;
				}
			}
		}
		break;
	case PROXY_SSL:
		{
			bRead = m_pSSLChannel->Read( pBuf, nBuf );
			nRead = m_pSSLChannel->GetLastReadCount();
			if( bRead ) {
				// 
			} else {
				if( nRead > 0 ) {
					// impossible to reach here
				} else if( (nRead == 0) && ( m_pTcpSock->GetErrorCode( PChannel::LastReadError ) ==  PChannel::Timeout ) ) {
					// no data coming
				} else {
					int nErrorCode = m_pTcpSock->GetErrorCode( PChannel::LastReadError );

					PString strMsg( PString::Printf, "SSL channel %d from %d closed, error: %d, %d, %s. now closing peer %d.", 
						this->m_nChannelId, 
						this->m_uid, 
						nErrorCode,
						m_pTcpSock->GetErrorNumber( PChannel::LastReadError ),
						(const char *)m_pTcpSock->GetErrorText( PChannel::LastReadError ),
						this->m_peer->m_uid );
					PLOG( & m_owner.m_UserActionLog, Info, strMsg );

					this->Close();
					int n = m_peer->Flush();
					m_peer->Close();
					this->m_timestamp = m_peer->m_timestamp = 0;

					PTRACE( 1, n << " bytes flushed before closing channel to " << m_peer->m_uid << "." );
				}
			}
		}
		break;
	}

#ifdef FWD_WHOLE_MESSAGE
	if( nRead > 0 ) {
		if( m_ReadBuf.PushBack( pBuf, nRead ) ) {
			nRead = m_ReadBuf.PopMessage( pBuf, nReadBuf );
			if ( nRead == 0 ) {
				// no data come, just retry next time
			} else if ( nRead == -1 ) {
				// when bad data coming, will go here
				PString strMsg( PString::Printf, "Invalid packet header, bad data coming from %d. Closing channel.", m_uid );
				PLOG( & m_owner.m_UserActionLog, Info, strMsg );

				this->Close();
			} else if ( nRead == -2 ) {
				// when buffer too small, will go here, 
				// but it's impossible, cause our buffer is 8192, much larger than 2 packets
			}
		} else {
			// the buffer is not enough
			// too bad, we must meet some bad data that cannot make up a message
			PString strMsg( PString::Printf, "Forward buffer overflow, too large packet, bad data coming from %d. Closing channel.", m_uid );
			PLOG( & m_owner.m_UserActionLog, Info, strMsg );

			this->Close();
		}
	}
#endif

	if( nRead > 0 ) {
		switch( m_owner.m_config.nLogTraffic ) {
		case 1:
			if( m_fpTrafficLog != NULL ) {
				PTime now;
				PString str = now.AsString("yyyy/MM/dd hh:mm:ss.uuu", PTime::Local);
				str += PString( PString::Printf, ", read %d bytes from %d.\r\n", nRead, this->m_uid );
				fputs( str, m_fpTrafficLog );
			}
			break;
		}
	}

	return nRead;
}

inline int BBQProxy::Entry::Write( const char * pBuf, int nLen )
{
#define NEVER_BLOCK_THREAD

	int nWritten = 0;

	switch( m_nProxyType ) {
	case PROXY_UDP:
		{
#ifdef NEVER_BLOCK_THREAD
			int fd = m_pUdpSock->GetHandle();
			sockaddr_in sockAddr;
			sockAddr.sin_family = AF_INET;
			sockAddr.sin_addr.s_addr = m_addr.ip; //htonl( m_addr.ip );
			sockAddr.sin_port = htons( m_addr.port );
			nWritten = ::sendto(fd, pBuf, nLen, 0, (struct sockaddr *)&sockAddr, sizeof(sockAddr));
			//sleep_ms(0);
			if( nWritten < 0 ) nWritten = 0;
#else
			PIPSocket::Address addr = m_addr.ip;
			WORD port = m_addr.port;
			if( m_pUdpSock->WriteTo( pBuf, nLen, addr, port ) ) {
				//sleep_ms(0);
				nWritten = m_pUdpSock->GetLastWriteCount();
				if( nWritten < 0 ) nWritten = 0;
			}
#endif
		}
		break;
	case PROXY_TCP:
		{

#ifdef NEVER_BLOCK_THREAD
			// PTCPSocket::Write(...) will retry until all data sent out, that will block the thread,
			// we cannot block the thread, cause we have only 1 thread to do this job,
			// so we write a function to replace it.
			int fd = m_pTcpSock->GetHandle();
			fd_set writefds;
			FD_ZERO( & writefds );
			FD_SET( fd, & writefds );
			struct timeval tv = { 0, 0 };
			int selval = ::select(fd+1, NULL, & writefds, NULL, & tv);
			if( selval > 0 ) {
				// it is writtable, let's do it
				nWritten = ::sendto(fd, pBuf, nLen, 0, NULL, 0);
				if( nWritten < 0 ) nWritten = 0;
			} else if ( selval == 0 ) {
				// blocked, we will try in future
			} else {
				// socket is error
			}
#else
			if( m_pTcpSock->Write( pBuf, nLen ) ) {
				nWritten = m_pTcpSock->GetLastWriteCount();
			}
#endif
		}
		break;
	case PROXY_SSL:
		{
			if( m_pSSLChannel->Write( pBuf, nLen ) ) {
				nWritten = m_pSSLChannel->GetLastWriteCount();
				if( nWritten < 0 ) nWritten = 0;
			}
		}
		break;
	}

	if( nWritten > 0 ) {
		switch( m_owner.m_config.nLogTraffic ) {
		case 2:
			if( m_fpTrafficData != NULL ) {
				fwrite( pBuf, sizeof(char), nLen, m_fpTrafficData );
			}
		case 1:
			if( m_fpTrafficLog != NULL ) {
				PTime now;
				PString str = now.AsString("yyyy/MM/dd hh:mm:ss.uuu", PTime::Local);
				str += PString( PString::Printf, ", write %d bytes to %d.\r\n", nWritten, this->m_uid );
				fputs( str, m_fpTrafficLog );
			}
		case 0:
			break;
		}
	}

	return nWritten;
}

int BBQProxy::Entry::ForwardToPeer( void )
{
	int nTotalWrite = 0;

	// we must flush all data already in buffer
	if( m_peer->HasDataInQueue() ) {
		nTotalWrite = m_peer->Flush();
		if( m_peer->HasDataInQueue() ) return nTotalWrite; // not all data in peer buffer flushed, keep other data blocked.
	}

	//char buf[ BBQPROXY_BUF_MAX ];
	int nRead = 0, nWrite = 0;

	MS_TIME msNow = BBQMsgTerminal::GetHostUpTimeInMs();

	while( (nRead = Read( buf, BBQPROXY_BUF_MAX )) > 0 ) {
		// update the timestamp once data read in
		this->m_timestamp = m_peer->m_timestamp = msNow;
    
		m_nBytesSendByThis += nRead;
		if( m_nBytesSendByThis > 0x40000000 ) {
			m_nGigaBytesSendByThis ++;
			m_nBytesSendByThis &= 0x3fffffff;
		}
    
		nWrite = m_peer->Write( buf, nRead );
		nTotalWrite += nWrite;
    
		if( nWrite == nRead ) {
			// whole block forwarded, the socket is writable, let's go on
		} else {
			// not all data written, let's put the left data in buffer
			m_peer->Queue( buf + nWrite, (nRead - nWrite) );
			//printf("nWrite[%d] != nRead[%d] ", nRead, nWrite);

			break;
		}
	}

	return nTotalWrite;
}

bool BBQProxy::Entry::HasDataInQueue( void )
{
	return (m_queueWrite.DataLen() > 0);
}

bool BBQProxy::Entry::Queue( const char * pBuf, int nLen )
{
	m_queueWrite.PushBack( pBuf, nLen );

	m_nQueueBytes += nLen;

	return true;
}

int BBQProxy::Entry::Flush( void )
{
	int nTotalWrite = 0;

	int nSizeToWrite = m_queueWrite.DataLen();
	if( nSizeToWrite > 0 ) {
		int nSizeWritten = Write( (const char *)m_queueWrite, nSizeToWrite );
		//printf("BBQProxy::Entry::Flush=[%d][%d]\n",nSizeToWrite, nSizeWritten);
		if( nSizeWritten > 0 ) {

			// update the timestamp once data write out
			this->m_timestamp = m_peer->m_timestamp = BBQMsgTerminal::GetHostUpTimeInMs();

			m_queueWrite.PopFront( nSizeWritten );
			m_nQueueBytes -= nSizeWritten;

			nTotalWrite += nSizeWritten;
		}
	}

	return nTotalWrite;
}

// ----------------------------------------------------------------------------------------

BBQProxy::BBQProxy()
	: PThread( 20000, NoAutoDeleteThread )
{
	m_pCurrentProxy = this;

	m_bClosing = false;
	m_bRunning = false;

	memset( & m_status, 0, sizeof(m_status) );
	m_status.tUpTime = time(NULL);
	m_status.nNextUdpPort = m_config.nUdpPortMin;

	memset( & m_stat, 0, sizeof(m_stat) );

	m_pTcpListener = NULL;
}

BBQProxy::~BBQProxy()
{
	OnClose();

	m_pCurrentProxy = NULL;
}

void BBQProxy::OnClose( void )
{
	if( m_bClosing ) return;

	// flag to notify the main loop to exit
	m_bClosing = true;

	// wait the thread to terminate
	if( ! IsTerminated() ) {
		if( GetThreadId() != GetCurrentThreadId() ) {
			if( IsSuspended() ) Resume();
			WaitForTermination();
		}
	}

	WriteLock safe( m_mutexNet );

	if( m_pTcpListener ) {
		delete m_pTcpListener;
	}

	while( m_listChannelEntry.size() > 0 ) {
		Entry * pEntry = m_listChannelEntry.front();
		m_listChannelEntry.pop_front();

		delete pEntry;
	}

	while( m_listForwardThreads.size() > 0 ) {
		ForwardThread * th = m_listForwardThreads.front();
		m_listForwardThreads.pop_front();

		delete th;
	}

	m_bClosing = false;
}

int BBQProxy::PrepareSelect( fd_set * pfdsetRead, fd_set * pfdsetWrite )
{
	int fdmax = 0;

	//FD_ZERO( pfdsetRead );
	//FD_ZERO( pfdsetWrite );

	// tcp listener
	if( m_pTcpListener && m_pTcpListener->IsOpen() ) {
		int fd = m_pTcpListener->GetHandle();
		FD_SET( fd, pfdsetRead );
		if( fd > fdmax ) fdmax = fd;
	}

	// anonymous tcp connection
	if( m_listAnonymousTcp.size() > 0 ) {
		for( at_iterator i = m_listAnonymousTcp.begin(), e = m_listAnonymousTcp.end(); i != e; /**/ ) {
			AnonymousTcp * pA = * i;

			if( pA->pSock && pA->pSock->IsOpen() ) {
				int fd = pA->pSock->GetHandle();
				FD_SET( fd, pfdsetRead );
				if( fd > fdmax ) fdmax = fd;

				i ++;
			} else {
				delete pA;
				//i = m_listAnonymousTcp.erase( i );
				STL_ERASE( m_listAnonymousTcp, at_iterator, i );
			}
		}
	}

	return fdmax;
}

int BBQProxy::PrepareSelectForChannel( std::list<Entry *> & listEntries, fd_set * pfdsetRead, fd_set * pfdsetWrite )
{
	int fdmax = 0;

	// udp/tcp/ssl channels
	if( listEntries.size() > 0 ) {
		MS_TIME msNow = BBQMsgTerminal::GetHostUpTimeInMs();
		for( pe_iterator iter = listEntries.begin(), e_iter = listEntries.end(); iter != e_iter; /**/ ) {
			Entry * pEntry = * iter;

			if( msNow - pEntry->m_nBytesSendLastTime >= 2000 ) { // stat every 2 seconds
				pEntry->m_nBytesSendLastTime = msNow;
				pEntry->m_nBytesSendLast = pEntry->m_nBytesSendByThis;
			} else if ( pEntry->m_nBytesSendLast > pEntry->m_nBytesSendByThis ) {
                pEntry->m_nBytesSendLast = pEntry->m_nBytesSendByThis;
			}

			PChannel * pChannel = NULL;

			switch( pEntry->m_nProxyType ) {
			case PROXY_UDP:
				pChannel = pEntry->m_pUdpSock;
				break;
			case PROXY_TCP:
				pChannel = pEntry->m_pTcpSock;
				break;
			case PROXY_SSL:
				pChannel = pEntry->m_pSSLChannel;
				break;
			}

			if( pChannel ) {
				if( pChannel->IsOpen() ) {
					int fd = pChannel->GetHandle();

					// if this socket has data coming, we must check if peer is writable or not
					if( pEntry->m_peer ) {
						if( FD_ISSET( fd, pfdsetRead ) ) {
							int peerfd = pEntry->m_peer->GetHandle();
							if( peerfd > 0 ) {
								FD_SET( peerfd, pfdsetWrite );
								if( peerfd > fdmax ) fdmax = peerfd;
							//} else {
							//	FD_CLR( peerfd, pfdsetWrite );
							}
						}

					}

					FD_SET( fd, pfdsetRead );
					if( pEntry->HasDataInQueue() ) FD_SET( fd, pfdsetWrite );

					if( fd > fdmax ) fdmax = fd;

					iter ++;
				} else { // socket already closed.
					PString strMsg( PString::Printf, "Channel from %d already closed, removing.", pEntry->m_uid );
					PTRACE( 1, strMsg );

					//iter = listEntries.erase( iter );
					STL_ERASE( listEntries, pe_iterator, iter );
					delete pEntry;
					m_status.nEntries --;
				}
			} else {
				iter ++;
			}
		}
	}

	return fdmax;
}

bool BBQProxy::HandleTcpListenersEvent( fd_set * pfdsetRead, fd_set * pfdsetWrite )
{
	int fd = m_pTcpListener->GetHandle();
	if( FD_ISSET( fd, pfdsetRead ) ) { // new connection is coming

		PTCPSocket * pNewCon = new PTCPSocket();
		if( pNewCon->Accept( * m_pTcpListener ) ) {

			PIPSocket::Address addr, local_addr;
			WORD	port, local_port;
			pNewCon->GetPeerAddress( addr, port );
			pNewCon->GetLocalAddress( local_addr, local_port );
			//PTRACE( 1, "TCP proxy attempt from " << (PString)addr << ":" << port );

			pNewCon->SetReadTimeout( PTimeInterval(0) );
			pNewCon->SetWriteTimeout( PTimeInterval(0) );

			if( m_config.nAnonymousTcpMax && (m_listAnonymousTcp.size() >= m_config.nAnonymousTcpMax) ) { // queue full
				PString strMsg( PString::Printf, "Too many anonymous tcp connection, deny." );
				PLOG( & m_UserActionLog, Info, strMsg );

				pNewCon->Close();
				delete pNewCon;
			} else {
				AnonymousTcp * pA = new AnonymousTcp;
				pA->pSock = pNewCon;
				pA->timestamp = BBQMsgTerminal::GetHostUpTimeInMs();
				m_listAnonymousTcp.push_back( pA );
			}
		} else {
			delete pNewCon;
		}

		return true;
	}

	return false;
}

bool BBQProxy::AnonymousTcpMatchEntry( PTCPSocket * pSock, std::list<Entry *> & listEntries, SIM_MESSAGE * pmsg, bool bWithHeader )
{
	BBQPROXY_CONNECT * pData = (BBQPROXY_CONNECT *) & pmsg->simData;

	for( pe_iterator i = listEntries.begin(), e = listEntries.end(); i != e; i ++ ) {
		Entry * pEA = * i;
    //chen yuan
    //PLOG( & m_UserActionLog, Info, "tcp socket handle %u (%u)", pSock->GetHandle(), pEA->m_pTcpSock->GetHandle() );

 //   if (pSock&& pEA->m_pTcpSock && pSock->GetHandle() == pEA->m_pTcpSock->GetHandle())
 //     pEA->bWithHeader  = bWithHeader;

		if( (pEA->m_nProxyType == PROXY_TCP) && (! pEA->m_pTcpSock) && 
			(! pEA->m_bIdentified) && (! pEA->m_bConnected) &&
			(pEA->m_nChannelId == pData->nChannelId) && (pEA->m_cookie == pData->cookie) && 
			(pEA->m_uid == pData->uid ) && (pEA->m_peer->m_uid == pData->uidPeer ) ) {

        pEA->bWithHeader  = bWithHeader;

			// if SSL requested but not initialized, do not use SSL
			if( pData->useSSL ) {
				if( ! m_status.bSSLInited ) {
					pData->useSSL = 0;
				}
			}

			pData->statusCode = SIMS_OK;

			PIPSocket::Address addr, local_addr;
			WORD	port, local_port;
			pSock->GetPeerAddress( addr, port );
			pSock->GetLocalAddress( local_addr, local_port );

			pEA->m_pTcpSock = pSock;
			pEA->m_nPort = local_port;
			pEA->m_addr.ip = (DWORD) addr;
			pEA->m_addr.port = port;

			pEA->m_bIdentified = true;

			// create SSL channel if requested
			if( pData->useSSL ) {
				//pEA->m_pSSLChannel = new PSSLChannel(false, * pSock);
				pEA->m_nProxyType = PROXY_SSL;
			}

			MS_TIME	msNow = BBQMsgTerminal::GetHostUpTimeInMs();
			pEA->m_timestamp = msNow;

			if( pEA->m_peer->m_bIdentified ) {
				pEA->m_bConnected = true;
				pEA->m_peer->m_bConnected = true;

				Entry * pES = pEA->m_bChannelSource ? pEA : pEA->m_peer;

				// write a log if match
				PString strMsg( PString::Printf, "Setup, TCP (ch: %u, cookie: %u) (uid: %d -> %d) (%s <-> %d <-> %d <-> %s, ts: %u.",
					pES->m_nChannelId, pES->m_cookie,
					pES->m_uid, pES->m_peer->m_uid,
					(const char *) BBQMsgTerminal::ToString( & pES->m_addr ),
					pES->m_nPort,
					pES->m_peer->m_nPort,
					(const char *) BBQMsgTerminal::ToString( & pES->m_peer->m_addr ),
					msNow );
				PLOG( & m_UserActionLog, Info, strMsg );

				// now send back echo
				if(pEA->bWithHeader/*bWithHeader*/ ) {
					pEA->m_pTcpSock->Write( pmsg, sizeof(pmsg->simHeader) + sizeof(BBQPROXY_CONNECT) );
				} else {
					pEA->m_pTcpSock->Write( pData, sizeof(BBQPROXY_CONNECT) );
				}

				// now send back echo to another side
				pData->uid = pEA->m_peer_uid;
				pData->uidPeer = pEA->m_uid;
				if( pEA->m_peer->bWithHeader/*bWithHeader*/ ) {
					pEA->m_peer->m_pTcpSock->Write( pmsg, sizeof(pmsg->simHeader) + sizeof(BBQPROXY_CONNECT) );
				} else {
					pEA->m_peer->m_pTcpSock->Write( pData, sizeof(BBQPROXY_CONNECT) );
				}

				// if traffic need to be logged
				if( this->m_config.nLogTraffic >= 1 ) {
					PTime now;
					PString str = now.AsString("yyyyMMddhhmmss", PTime::Local);

					PString strFilepath( PString::Printf, "logs\\ch-%s-%d-%d-%d.txt", (const char *)str, pES->m_nChannelId, pES->m_uid, pES->m_peer->m_uid );
					FILE * fpLog = fopen( strFilepath, "w" );
					pES->m_fpTrafficLog = pES->m_peer->m_fpTrafficLog = fpLog;

					if( this->m_config.nLogTraffic >= 2 ) {
						PString strFilepath1( PString::Printf, "logs\\ch-%s-%d-%d.bin", (const char *)str, pES->m_nChannelId, pES->m_peer->m_uid );
						FILE * fpData1 = fopen( strFilepath1, "w" );
						pES->m_fpTrafficData = fpData1;

						PString strFilepath2( PString::Printf, "logs\\ch-%s-%d-%d.bin", (const char *)str, pES->m_nChannelId, pES->m_uid );
						FILE * fpData2 = fopen( strFilepath2, "w" );
						pES->m_peer->m_fpTrafficData = fpData2;
					}
				}
			}

			return true;
		}					
	}

	return false;
}

bool BBQProxy::HandleAnonymousTcpEvent( fd_set * pfdsetRead, fd_set * pfdsetWrite )
{
	WriteLock safe( m_mutexNet );
	MS_TIME msNow = BBQMsgTerminal::GetHostUpTimeInMs();

	bool bSomethingDone = false;

	for( at_iterator iter = m_listAnonymousTcp.begin(), e_iter = m_listAnonymousTcp.end(); iter != e_iter; ) {
		AnonymousTcp * pA = * iter;
		PTCPSocket * pSock = pA->pSock;
		int fd = pSock->GetHandle();
		if( FD_ISSET( fd, pfdsetRead ) ) { // data is coming

			bSomethingDone = true;

			// no matter identity is yes or no, not anonymous any more, 
			// remove the node from the anonymous queue, but keep the socket object for later use
			//iter = m_listAnonymousTcp.erase( iter ); 
			STL_ERASE( m_listAnonymousTcp, at_iterator, iter );
			pA->pSock = NULL;
			delete pA;

			bool bWithHeader = false;
			SIM_MESSAGE msg;
			memset( & msg, 0, sizeof(msg) );
			bool bRead = pSock->Read( & msg, sizeof(msg.simHeader) );
			int nCount = pSock->GetLastReadCount();
			if( bRead && (nCount == sizeof(msg.simHeader)) ) {
				if( msg.simHeader.magic == SIM_MAGIC ) {
					bWithHeader = true;
          //
					if( (msg.simHeader.size <= SIM_DATABLOCKSIZEMAX) && (msg.simHeader.size >= sizeof(BBQPROXY_CONNECT)) ) {
						bRead = pSock->Read( & msg.simData, msg.simHeader.size );
						msg.simHeader.size = pSock->GetLastReadCount();
					} else {
						msg.simHeader.magic = 0;
					}
				} else {
					memmove( & msg.simData, & msg, sizeof(msg.simHeader) );
					int nLeft = sizeof(BBQPROXY_CONNECT) - sizeof(msg.simHeader);
					if( pSock->Read( & msg.simData[ sizeof(msg.simHeader) ], nLeft ) && (pSock->GetLastReadCount() == nLeft) ) {
						msg.simHeader.magic = SIM_MAGIC;
						msg.simHeader.id = UCM_CP_CONNECT;
						msg.simHeader.size = sizeof(msg.simHeader) + nLeft;
						msg.simHeader.sync = 0;
					} else {
						msg.simHeader.magic = 0;
					}
				}
			} else {
				msg.simHeader.magic = 0;
			}

			bool bValidConnectMsg = ( (msg.simHeader.magic == SIM_MAGIC) && (msg.simHeader.id == UCM_CP_CONNECT) && (msg.simHeader.size == sizeof(BBQPROXY_CONNECT)) );
			if( bValidConnectMsg ) {
				BBQPROXY_CONNECT * pData = (BBQPROXY_CONNECT *) & msg.simData;
				PString strMsg( PString::Printf, "uid %u connected tcp, seek to match channel: %d, cookie: %u, peer uid: %u.", 
					pData->uid,
					pData->nChannelId, pData->cookie,
					pData->uidPeer );
				PLOG( & m_UserActionLog, Info, strMsg );
			} else {
				PString strMsg( PString::Printf, "Bad tcp connection to proxy, closing. (read in %d bytes, expect: %d)", msg.simHeader.size, sizeof(BBQPROXY_CONNECT) );
				PLOG( & m_UserActionLog, Info, strMsg );

				delete pSock;
				continue;
			}

			// find the exact entry that match the request
			bool bMatch = false;
			bMatch = AnonymousTcpMatchEntry( pSock, m_listChannelEntry, & msg, bWithHeader );
      
			if( ! bMatch ) {
				for( ft_iterator it = m_listForwardThreads.begin(), et = m_listForwardThreads.end(); it != et; it ++ ) {
					ForwardThread * th = * it;

					WriteLock sf( th->m_mutex );
					bMatch = AnonymousTcpMatchEntry( pSock, th->m_listChannelEntry, & msg, bWithHeader );
					if( bMatch ) break;
				}
			}

			if( ! bMatch ) delete pSock;

		} else if ( msNow - pA->timestamp > m_config.nAnonymousTcpIdle ) { // no data coming and idle too long, kick
			//iter = m_listAnonymousTcp.erase( iter ); 
			STL_ERASE( m_listAnonymousTcp, at_iterator, iter );
			delete pA->pSock;
			delete pA;
		} else { // no data coming, but not idle too long, just wait again
			iter ++;
		}
	}

	return bSomethingDone;
}

inline bool BBQProxy::HandleChannelEvent( std::list<Entry *> & listEntries, fd_set * pfdsetRead, fd_set * pfdsetWrite
#ifndef _WIN32	
	, int& ifds 
#endif
)
{
	MS_TIME msNow = BBQMsgTerminal::GetHostUpTimeInMs();

	bool bSomethingDone = false;

	for( pe_iterator iter = listEntries.begin(), e_iter = listEntries.end();
	 #ifndef _WIN32
	 	ifds > 0 && 
	 #endif 
	 iter != e_iter; iter ++
	  ) {
		Entry * pEA = * iter;

		if( pEA->m_pUdpSock || pEA->m_pTcpSock || pEA->m_pSSLChannel ) ; // yes, it's are valid
		else continue;	// no socket yet, those tcp that not come yet

		//int fdA = pEA->m_pSock->GetHandle();
		int fdA = pEA->GetHandle();
		if (fdA <0 )
			continue;

		if( FD_ISSET( fdA, pfdsetRead ) ) { // data is coming
			#ifndef _WIN32	
			--ifds;
			#endif
			if( pEA->m_bConnected ) { 

				//int fdB = pEA->m_peer->m_pSock->GetHandle();
				int fdB = pEA->m_peer->GetHandle();

				if( FD_ISSET( fdB, pfdsetWrite ) ) { // peer is writable
					int nWrite = pEA->ForwardToPeer();
					if( nWrite > 0 ) {
						m_status.nBytesLastFiveSecond += nWrite;
						m_status.nHistoryBytes += nWrite;
        
						bSomethingDone = true;
					}

				} else { // peer is not writable, skip
					// do nothing, just let the data blocked in socket

				}

			} else if ( pEA->m_nProxyType == PROXY_TCP || pEA->m_nProxyType == PROXY_SSL ) { // TCP, but peer is not connected yet, skip

				// do nothing, just let the data blocked in socket
				//PTRACE( 1, "TCP data coming from " << (unsigned int)pEA->m_uid << ", but peer not connected yet." );

			} else if ( ! pEA->m_bIdentified ) { // UDP port listeners, this is the connect request
				PUDPSocket * pSock = pEA->m_pUdpSock;
				PIPSocket::Address addr;
				WORD port, portLocal;
				pSock->GetLocalAddress( addr, portLocal );

				bool bWithHeader = false;
        pEA->bWithHeader = bWithHeader;
				SIM_MESSAGE msg;
				memset( & msg, 0, sizeof(msg) );
				bool bRead = pSock->ReadFrom( & msg, sizeof(msg.simHeader) + sizeof(BBQPROXY_CONNECT), addr, port );
				int nCount = pSock->GetLastReadCount();
				if( bRead ) {
					if( (nCount == sizeof(msg.simHeader) + sizeof(BBQPROXY_CONNECT)) && (msg.simHeader.magic == SIM_MAGIC) ) {
						bWithHeader = true;
            pEA->bWithHeader = bWithHeader;
						msg.simHeader.size = sizeof(BBQPROXY_CONNECT);
					} else if ( nCount == sizeof(BBQPROXY_CONNECT) ) {
						memmove( & msg.simData, & msg, sizeof(BBQPROXY_CONNECT) );
						msg.simHeader.magic = SIM_MAGIC;
						msg.simHeader.id = UCM_CP_CONNECT;
						msg.simHeader.size = sizeof(BBQPROXY_CONNECT);
						msg.simHeader.sync = 0;
					} else {
						msg.simHeader.magic = 0;
						msg.simHeader.size = nCount;
					}
				} else {
					msg.simHeader.magic = 0;
				}
				bool bValidConnectMsg = ( (msg.simHeader.magic == SIM_MAGIC) && (msg.simHeader.id == UCM_CP_CONNECT) && (msg.simHeader.size == sizeof(BBQPROXY_CONNECT)) );
				BBQPROXY_CONNECT * pData = (BBQPROXY_CONNECT *) & msg.simData;

				PString strMsg( PString::Printf, "uid %u connected udp, seek to match channel: %d, cookie: %u, peer uid: %u.", 
					pData->uid,
					pData->nChannelId, pData->cookie,
					pData->uidPeer );
				PLOG( & m_UserActionLog, Info, strMsg );

				if( bValidConnectMsg &&
					(! pEA->m_bIdentified) && (! pEA->m_bConnected) &&
					(pEA->m_nChannelId == pData->nChannelId) && (pEA->m_cookie == pData->cookie) && 
					(pEA->m_nPort == portLocal ) &&
					(pEA->m_uid == pData->uid ) && (pEA->m_peer->m_uid == pData->uidPeer) ) { // check data identity

					pEA->m_addr.ip = (DWORD) addr;
					pEA->m_addr.port = port;

					pEA->m_bIdentified = true;

					pEA->m_timestamp = msNow;

					// send back OK echo.
					pData->statusCode = SIMS_OK;

					if( pEA->m_peer->m_bIdentified ) {
						pEA->m_bConnected = true;
						pEA->m_peer->m_bConnected = true;

						Entry * pES = pEA->m_bChannelSource ? pEA : pEA->m_peer;

						// write a log that connection setup
						PString strMsg( PString::Printf, "Setup, UDP (ch: %u, cookie: %u) (uid: %d -> %d) (%s <-> %d <-> %d <-> %s, ts: %u.",
							pES->m_nChannelId, pES->m_cookie,
							pES->m_uid, pES->m_peer->m_uid,
							(const char *) BBQMsgTerminal::ToString( & pES->m_addr ),
							pES->m_nPort,
							pES->m_peer->m_nPort,
							(const char *) BBQMsgTerminal::ToString( & pES->m_peer->m_addr ),
							(DWORD)msNow );
						PLOG( & m_UserActionLog, Info, strMsg );

						// now send back to client confirmation msg
            if( pEA->bWithHeader/*bWithHeader*/ ) {
							msg.simHeader.id = UCM_PC_CONNECT;
							for( int i=0; i<3; i++ ) { // send 3 packets to avoid loss
								if ( pSock->WriteTo( & msg, sizeof(msg.simHeader) + sizeof(BBQPROXY_CONNECT), pEA->m_addr.ip, pEA->m_addr.port )){
									sleep_ms(1);
								}else
								{
									PString strMsg( PString::Printf, "Failed to Write to %s",(const char *) BBQMsgTerminal::ToString(pEA->m_addr.ip),  pEA->m_addr.port);
									PLOG( & m_UserActionLog, Info, strMsg );
								}
							}
						} else {
							pSock->WriteTo( pData, sizeof(BBQPROXY_CONNECT), pEA->m_addr.ip, pEA->m_addr.port );
						}

						// also send to another client
						pData->uid = pEA->m_peer->m_uid;
						pData->uidPeer = pEA->m_uid;
            if(pEA->m_peer->bWithHeader /*bWithHeader*/ ) {
							msg.simHeader.id = UCM_PC_CONNECT;
							for( int i=0; i<3; i++ ) { // send 3 packets to avoid loss
								if ( pEA->m_peer->m_pUdpSock->WriteTo( & msg, sizeof(msg.simHeader) + sizeof(BBQPROXY_CONNECT), pEA->m_peer->m_addr.ip, pEA->m_peer->m_addr.port )){
									sleep_ms(1);
								}else
								{
									PString strMsg( PString::Printf, "Failed to Write to %s",(const char *) BBQMsgTerminal::ToString( pEA->m_peer->m_addr.ip),  pEA->m_peer->m_addr.port);
									PLOG( & m_UserActionLog, Info, strMsg );
								}

							}
						} else {
							pEA->m_peer->m_pUdpSock->WriteTo( pData, sizeof(BBQPROXY_CONNECT), pEA->m_peer->m_addr.ip, pEA->m_peer->m_addr.port );
						}
					}

				} else { // bad data or bad request
					pData->statusCode = SIMS_BADREQUEST;

					if( pEA->bWithHeader/*bWithHeader*/ ) {
						msg.simHeader.id = UCM_PC_CONNECT;
						for( int i=0; i<3; i++ ) { // send 3 packets to avoid loss
							pSock->WriteTo( & msg, sizeof(msg.simHeader) + sizeof(BBQPROXY_CONNECT), addr, port );
							//sleep_ms(0);
						}
					} else {
						pSock->WriteTo( pData, sizeof(BBQPROXY_CONNECT), addr, port );
					}
				}

				bSomethingDone = true;

			} else { // UDP, but peer is not connected yet, skip

				// do nothing, just let the data blocked in socket
				//PTRACE( 1, "UDP data coming from " << (unsigned int)pEA->m_uid << ", but peer not connected yet." );

			}
		}
		if( FD_ISSET( fdA, pfdsetWrite ) ) {
			#ifndef _WIN32	
				--ifds;
			#endif
			int nWrite = pEA->Flush();
			if( nWrite > 0 ) {
				m_status.nBytesLastFiveSecond += nWrite;
				m_status.nHistoryBytes += nWrite;
      
				bSomethingDone = true;
			}
		}

	}

	return bSomethingDone;
}

int BBQProxy::ValidateActiveChannel( std::list<Entry *> & listEntries )
{
	MS_TIME msNow = BBQMsgTerminal::GetHostUpTimeInMs();

	for( pe_iterator iter = listEntries.begin(), e_iter = listEntries.end(); iter != e_iter; ) {
		Entry * pEntry = * iter;
		if( msNow - pEntry->m_timestamp >= m_config.nChannelIdle ) {

			// get rid of the node from the list
			//iter = listEntries.erase( iter );
			STL_ERASE( listEntries, pe_iterator, iter );

			if( pEntry->m_peer ) {
				// empty peer's pointer to avoid accessing deleted object
				pEntry->m_peer->m_peer = NULL;
				pEntry->m_peer->m_bConnected = false;

				// write log
				PString strMsg( PString::Printf, "%s channel %d (%d <--> %d) idle too long, close.",
					(const char *) TypeToString(pEntry->m_nProxyType), pEntry->m_nChannelId, pEntry->m_uid, pEntry->m_peer_uid );
				PLOG( & m_UserActionLog, Info, strMsg );

				StatChannel( (pEntry->m_nProxyType != PROXY_UDP), false, pEntry->m_tInit, BBQMsgTerminal::GetHostUpTimeInMs() );
			}

			// delete object
			pEntry->Close();
			delete pEntry;
			m_status.nEntries --;
		} else {
			iter ++;
		}
	}

	return listEntries.size();
}

#ifdef FD_SETSIZE
#if FD_SETSIZE == 64
#define FD_SETSIZE 4096
//#error large fd_set should be used to support large number of sockets.
#endif
#endif

void BBQProxy::LoopNetIO( void )
{
	MS_TIME msLastTime = BBQMsgTerminal::GetHostUpTimeInMs();

	// prepare the select list, suspend running if no fd in the list
	fd_set fdsetRead, fdsetWrite;

	FD_ZERO( & fdsetRead );
	FD_ZERO( & fdsetWrite );

	while( m_bRunning && (! m_bClosing) ) {
		//sleep_ms(0);

#define FIVE_SECONDS_IN_MS	5000

		// remove the channels that idle for too long
		MS_TIME msNow = BBQMsgTerminal::GetHostUpTimeInMs();
		if( msNow - msLastTime >= FIVE_SECONDS_IN_MS ) {
			msLastTime = msNow;

			// write stat log at end of day
			time_t tNow = time(NULL);
			if( tNow > m_stat.tTodayMax ) {
				if( m_stat.tTodayMax != 0 ) { // now write stat info to log
					PString strMsg( PString::Printf, "today stat report: total time: %d minutes, total channels: %d (udp: %d, tcp: %d, broken: %d)",
						m_stat.nMinutes, (m_stat.nTcp + m_stat.nUdp), m_stat.nUdp, m_stat.nTcp, m_stat.nBroken );
					PLOG( & m_UserActionLog, Info, strMsg );

					m_stat.nTcp = 0;
					m_stat.nUdp = 0;
					m_stat.nBroken = 0;
					m_stat.nMinutes = 0;
				}

				struct tm tt = * localtime(& tNow);
				m_stat.tTodayMin = tNow - ((tt.tm_hour * 60 + tt.tm_min) * 60 + tt.tm_sec);
				m_stat.tTodayMax = m_stat.tTodayMin + 3600 * 24;
			}

			// remove those channels that idle for 60 seconds
			{
				WriteLock safe( m_mutexNet );
				ValidateActiveChannel( m_listChannelEntry );
			}

#define		ONE_GIGA		1073741824		// 1024 * 1024 * 1024

			if( m_status.nHistoryBytes > ONE_GIGA ) {
				m_status.nHistoryGigaBytes ++;
				m_status.nHistoryBytes /= ONE_GIGA;
			}

			// calculate average bandwidth in past 5 seconds
			m_status.nAverageBandwidth = m_status.nBytesLastFiveSecond / 640; // * 8 / 1024 / 5;
			m_status.nBytesLastFiveSecond = 0;

			// put current bandwidth to stat list
			m_bwstat.push_back( m_status.nAverageBandwidth );

			int nStatLength = m_config.nStatTime / 5;
			while( m_bwstat.size() > nStatLength ) m_bwstat.pop_front();
		}

		int fdmax = 0;
		{
			WriteLock safe( m_mutexNet );
			int fd = PrepareSelect( & fdsetRead, & fdsetWrite );
			if( fd > fdmax ) fdmax = fd;
			fd = PrepareSelectForChannel( m_listChannelEntry, & fdsetRead, & fdsetWrite );
			if( fd > fdmax ) fdmax = fd;
		}
		if( fdmax <= 0 ) {
			// impossible to go here, cause we at least have tcp port listener
			sleep_ms(1);
			continue;
		}

		// do a select to wait I/O happen
		struct timeval tval = { 0, 300000000 };
		int ret = ::select( fdmax+1, & fdsetRead, & fdsetWrite, NULL, &tval);
		if( ret < 0 ) {

#if defined(_WIN32) && defined(PTRACING) 
			int err = WSAGetLastError();
			const char * errString = "Unknown";
			switch( err ) {
			case WSANOTINITIALISED:		errString = "WSANOTINITIALISED";	break;
			case WSAEFAULT:				errString = "WSAEFAULT";			break;
			case WSAENETDOWN:			errString = "WSAENETDOWN";			break;
			case WSAEINVAL:				errString = "WSAEINVAL";			break;
			case WSAEINTR:				errString = "WSAEINTR";				break;
			case WSAEINPROGRESS:		errString = "WSAEINPROGRESS";		break;
			case WSAENOTSOCK:			errString = "WSAENOTSOCK";			break;
			}
			PTRACE(1, "Select() return error: " << errString);
#endif

			FD_ZERO( & fdsetRead );
			FD_ZERO( & fdsetWrite );
			sleep_ms(1);
			continue;
			//break;
		} else if( ret == 0 ) {
			FD_ZERO( & fdsetWrite );
			sleep_ms(1);
			continue;
		}

		// handle event, some event might be ignored if other condition not matches
		bool bSomethingDone = false;
		{
			WriteLock safe( m_mutexNet );
			if( HandleTcpListenersEvent( & fdsetRead, & fdsetWrite ) ) bSomethingDone = true;
			if( HandleAnonymousTcpEvent( & fdsetRead, & fdsetWrite ) ) bSomethingDone = true;
			if( HandleChannelEvent( m_listChannelEntry, & fdsetRead, & fdsetWrite 
				#ifndef _WIN32
					, ret 
				 #endif 
				 ) ) bSomethingDone = true;
		}
		// if nothing done in this cycle, we must wait some time, to avoid empty loop
		if( ! bSomethingDone ) {
			//sleep_ms(1); 
		}
		sleep_ms(10); 

		FD_ZERO( & fdsetWrite );
	}
}

void BBQProxy::Main( void )
{
	PTRACE( 1, "Proxy thread starting." );

	SetThreadName( "" );

	m_bRunning = true;

	LoopNetIO();

	m_bRunning = false;

	PTRACE( 1, "Proxy thread exiting." );
}

PUDPSocket * BBQProxy::BindUdpPort( void )
{
	PUDPSocket * pSock = new PUDPSocket();
	if( pSock ) {
		int nCurrentNextPort = m_status.nNextUdpPort;
		do {
			PBoolean bDone = pSock->Listen( (m_config.dwListenIp ? m_config.dwListenIp : INADDR_ANY),  0, m_status.nNextUdpPort, PSocket::AddressIsExclusive );

			PString strMsg( PString::Printf, "Bind UDP port: %d, %s.", m_status.nNextUdpPort, (bDone ? "okay." : "failed.") );
			PLOG( & m_UserActionLog, Info, strMsg );

			// try next port
			m_status.nNextUdpPort ++;
			if( m_status.nNextUdpPort > m_config.nUdpPortMax ) m_status.nNextUdpPort = m_config.nUdpPortMin;

			if( bDone ) return pSock;

		} while ( m_status.nNextUdpPort != nCurrentNextPort );

		delete pSock; // no valid port avaiable
	} 

	return NULL;
}

bool BBQProxy::CreateChannel( uint32 uidA, uint32 uidB, bool bTCP, Entry * & pEntryA, Entry * & pEntryB )
{
	WriteLock safe( m_mutexNet );

	PUDPSocket * pSA = NULL, * pSB = NULL;

	PIPSocket::Address addr;
	WORD portA, portB;
	portA = portB = m_config.nTcpPort;

	if( ! bTCP ) { // if it is UDP channel, bind 2 ports, to wait for the connect request
		pSA = BindUdpPort();
		pSB = BindUdpPort();
		if( pSA && pSB ) {
			// okay, we got a pair of UDP sockets
			pSA->GetLocalAddress( addr, portA );
			pSB->GetLocalAddress( addr, portB );

			pSA->SetReadTimeout( PTimeInterval(0) );
			pSB->SetReadTimeout( PTimeInterval(0) );

			pSA->SetWriteTimeout( PTimeInterval(0) );
			pSB->SetWriteTimeout( PTimeInterval(0) );

		} else {
			// fail to bind UDP ports
			if( pSA ) delete pSA;
			if( pSB ) delete pSB;

			PString strMsg( PString::Printf, "Failed to bind UDP port, when creating proxy channel, type: UDP (from %d to %d).", uidA, uidB );
			PLOG( & m_UserActionLog, Info, strMsg );

			return false;
		}
	}

	// create a pair of proxy entry, init them, and push to entry list
	pEntryA					= new Entry(*this);
	pEntryB					= new Entry(*this);
	pEntryA->m_peer			= pEntryB; 
	pEntryB->m_peer			= pEntryA;

	pEntryA->m_uid			= uidA;
	pEntryB->m_uid			= uidB;

	pEntryA->m_peer_uid		= uidB;
	pEntryB->m_peer_uid		= uidA;

	pEntryA->m_nChannelId	= pEntryB->m_nChannelId		= ++ m_status.nNextChannelId;

	MS_TIME	msNow = BBQMsgTerminal::GetHostUpTimeInMs();

	pEntryA->m_cookie		= pEntryB->m_cookie			= ((DWORD) msNow) * rand() * m_status.nNextChannelId; // 0 ~ RAND_MAX

	pEntryA->m_nProxyType	= pEntryB->m_nProxyType		= bTCP ? PROXY_TCP : PROXY_UDP;

	pEntryA->m_tInit		= pEntryB->m_tInit			=
	pEntryA->m_timestamp	= pEntryB->m_timestamp		= 
	pEntryA->m_nBytesSendLastTime = pEntryB->m_nBytesSendLastTime = msNow;

	pEntryA->m_nGigaBytesSendByThis = pEntryB->m_nGigaBytesSendByThis =
	pEntryA->m_nBytesSendByThis = pEntryB->m_nBytesSendByThis =
	pEntryA->m_nBytesSendLast = pEntryB->m_nBytesSendLast = 0;


	if( ! bTCP ) {
		pEntryA->m_pUdpSock = pSA;
		pEntryA->m_nPort = portA;

		pEntryB->m_pUdpSock = pSB;
		pEntryB->m_nPort = portB;
	}

	pEntryA->m_bChannelSource = true;
	pEntryB->m_bChannelSource = false;

	if( m_listForwardThreads.size() > 0 ) {
		// calculate the average entry number per threads
		int m = 0, n = 0;
		for( ft_iterator it = m_listForwardThreads.begin(), et = m_listForwardThreads.end(); it != et; it ++ ) {
			ForwardThread * th = * it;
			m += th->m_listChannelEntry.size();
			n ++;
		}
		int nAverage = m / n;

		// pick one threads, put the two entries into the list
		for( ft_iterator it = m_listForwardThreads.begin(), et = m_listForwardThreads.end(); it != et; it ++ ) {
			ForwardThread * th = * it;
			if( th->m_listChannelEntry.size() <= nAverage ) {
				WriteLock sf( th->m_mutex );
				th->m_listChannelEntry.push_back( pEntryA );
				th->m_listChannelEntry.push_back( pEntryB );
				break;
			}
		}
	} else {
		m_listChannelEntry.push_back( pEntryA );
		m_listChannelEntry.push_back( pEntryB );
	}

	m_status.nEntries += 2;
	m_status.nHistoryEntries += 2;

	PString strMsg( PString::Printf, 
		"Created, %s (ch: %u, cookie: %u), (uid: %d -> %d) (port: %d <-> %d), ts: %u.",
		(bTCP ? "TCP" : "UDP"), pEntryA->m_nChannelId, pEntryA->m_cookie, uidA, uidB, portA, portB, (DWORD)msNow );
	PLOG( & m_UserActionLog, Info, strMsg );

	return true;
}

bool BBQProxy::InternalCutChannel( std::list<Entry *> & listEntries, uint32 nChId )
{
	int n = 0;
	for( pe_iterator iter = listEntries.begin(), e_iter = listEntries.end(); iter != e_iter; /**/ ) {
		Entry * pEntry = * iter;
		if( pEntry->m_nChannelId == nChId ) {
			//iter = listEntries.erase( iter );
			STL_ERASE( listEntries, pe_iterator, iter );
			if( pEntry->m_peer ) {
				pEntry->m_peer->m_peer = NULL;
				pEntry->m_peer->m_bConnected = false;
			}
			pEntry->Close();
			delete pEntry;
			m_status.nEntries --;
			n ++;
		} else {
			iter ++;
		}
	}

	return (n > 0);
}

bool BBQProxy::CutChannel( uint32 nChId )
{
	WriteLock safe( m_mutexNet );

	bool bDone = InternalCutChannel( m_listChannelEntry, nChId );
	if( ! bDone ) {
		for( ft_iterator it = m_listForwardThreads.begin(), et = m_listForwardThreads.end(); it != et; it ++ ) {
			ForwardThread * th = * it;

			WriteLock sf( th->m_mutex );
			bDone = InternalCutChannel( th->m_listChannelEntry, nChId );
			if( bDone ) break;
		}
	}

	PString strMsg( PString::Printf, "Cutting channel %d, %s.", nChId, bDone ? "done." : "not found." );
	PLOG( & m_UserActionLog, Info, strMsg );

	return bDone;
}

bool BBQProxy::SetConfig( Config & conf )
{
	// if the tcp port is open, change to another port if different
	if( m_pTcpListener && (conf.nTcpPort != m_config.nTcpPort) ) {
		PTCPSocket * pNewListener = new PTCPSocket;
		DWORD ip = (conf.dwListenIp ? conf.dwListenIp : INADDR_ANY);
		if( pNewListener->Listen( ip, conf.nListenBacklogSize, conf.nTcpPort /*, PSocket::CanReuseAddress*/ ) ) {

			PString strIp = m_config.dwListenIp ? BBQMsgTerminal::ToString(m_config.dwListenIp) : "*";
			PString strMsg( PString::Printf, "Proxy TCP listen port: :%s:%d.", (const char *) strIp, conf.nTcpPort );
			PLOG( & m_UserActionLog, Info, strMsg );

			delete m_pTcpListener;
			m_pTcpListener = pNewListener;
		} else {
			PString strIp = m_config.dwListenIp ? BBQMsgTerminal::ToString(m_config.dwListenIp) : "*";
			PString strMsg( PString::Printf, "Proxy TCP listen fail at port: %s:%d.", (const char *) strIp, conf.nTcpPort );
			PLOG( & m_UserActionLog, Info, strMsg );

			delete pNewListener;
			return false;
		} 
	}

	m_config = conf;
	m_config.nUdpPortMin = min( conf.nUdpPortMin, conf.nUdpPortMax );
	m_config.nUdpPortMax = max( conf.nUdpPortMin, conf.nUdpPortMax );

	if( strlen(m_config.lpszSSLCertPath) && strlen(m_config.lpszSSLKeyPath) ) {
		//m_status.bSSLInited = SetServerCtx( m_config.lpszSSLCertPath, m_config.lpszSSLKeyPath, m_config.lpszSSLKeyPassword );
	}

	if( strlen(m_config.lpszLogPath) ) {
		m_UserActionLog.Open( m_config.lpszLogPath );
	}

	return true;
}

bool BBQProxy::GetConfig( Config & conf )
{
	conf = m_config;

	return true;
}

bool BBQProxy::InternalGetStatus( std::list<Entry *> & listEntries, Status & status )
{
	for( pe_iterator i = listEntries.begin(), e = listEntries.end(); i != e; i ++ ) {
		Entry * pE = * i;
		status.nBufferBytes += pE->m_nQueueBytes;
	}

	return true;
}

bool BBQProxy::GetStatus( Status & status )
{
	ReadLock safe( m_mutexNet );

	status = m_status;

	InternalGetStatus( m_listChannelEntry, status );

	for( ft_iterator it = m_listForwardThreads.begin(), et = m_listForwardThreads.end(); it != et; it ++ ) {
		ForwardThread * th = * it;

		ReadLock sf( th->m_mutex );
		InternalGetStatus( th->m_listChannelEntry, status );
	}

	return true;
}

bool BBQProxy::GetBandwidthStat( std::list<int> & stat )
{
	stat.clear();
	for( std::list<int>::iterator i = m_bwstat.begin(), e = m_bwstat.end(); i != e; i++ ) {
		stat.push_back( * i );
	}

	return true;
}

bool BBQProxy::GetSessionInfoList( SessionInfoList & sessions )
{
	ReadLock safe( m_mutexNet );

	InternalGetSessionInfoList( m_listChannelEntry, sessions );

	for( ft_iterator it = m_listForwardThreads.begin(), et = m_listForwardThreads.end(); it != et; it ++ ) {
		ForwardThread * th = * it;

		ReadLock sf( th->m_mutex );
		InternalGetSessionInfoList( th->m_listChannelEntry, sessions );
	}

	return true;
}

bool BBQProxy::InternalGetSessionInfoList( std::list<Entry *> & listEntries, SessionInfoList & sessions )
{
	MS_TIME msNow = BBQMsgTerminal::GetHostUpTimeInMs();

	for( pe_iterator iter = listEntries.begin(), e_iter = listEntries.end(); iter != e_iter; iter ++ ) {
		Entry * pEntry = * iter;

		if( pEntry->m_bConnected && pEntry->m_bChannelSource ) {
			SessionInfo * si = new SessionInfo;
			memset( si, 0, sizeof(*si) );

			si->m_nChannelId = pEntry->m_nChannelId;
			si->m_tInit = pEntry->m_tInit;
			si->m_timestamp = pEntry->m_timestamp;
			si->m_nProxyType = pEntry->m_nProxyType;

			for( int i=0; i<2; i++ ) {
				si->entry[i].m_uid = pEntry->m_uid;
				si->entry[i].m_nPort = pEntry->m_nPort;
				si->entry[i].m_addr = pEntry->m_addr;
				si->entry[i].m_nQueueBytes = pEntry->m_nQueueBytes;
				si->entry[i].m_nSendBytes = pEntry->m_nBytesSendByThis;
				si->entry[i].m_nSendGigaBytes = pEntry->m_nGigaBytesSendByThis;

				DWORD t = (DWORD)(msNow - pEntry->m_nBytesSendLastTime);
				si->entry[i].m_nSendBps = ((pEntry->m_nBytesSendByThis - pEntry->m_nBytesSendLast) * 1000) / (t ? t : 1);

				pEntry = pEntry->m_peer;
			}

			sessions.push_back( si );
		}
	}

	return true;
}

BBQProxy * BBQProxy::GetCurrentProxy( void )
{
	return m_pCurrentProxy;
}

bool BBQProxy::StartProxy( const Config * conf )
{
	srand( (unsigned int)time( NULL ) );

	if( m_pCurrentProxy && m_pCurrentProxy->IsTerminated() ) {
		delete m_pCurrentProxy;
		m_pCurrentProxy = NULL;
	}

	if( conf ) {
		m_config = * conf;

		m_config.nUdpPortMin = min( conf->nUdpPortMin, conf->nUdpPortMax );
		m_config.nUdpPortMax = max( conf->nUdpPortMin, conf->nUdpPortMax );

		//m_config.nListenBacklogSize = min( m_config.nListenBacklogSize, 50 );
		//m_config.nListenBacklogSize = max( m_config.nListenBacklogSize, 2 );
	} else {
		m_config.nTcpPort = BBQPROXY_TCPPORT;
		m_config.nListenBacklogSize = 5;

		m_config.nUdpPortMin = BBQPROXY_UDPPORT_MIN;
		m_config.nUdpPortMax = BBQPROXY_UDPPORT_MAX;

		m_config.nChannelMax = BBQPROXY_CHANNEL_MAX;
		m_config.nChannelBufferMax = BBQPROXY_CHANNELBUFFER_MAX;
		m_config.nChannelIdle = BBQPROXY_CHANNEL_IDLE * 1000;

		m_config.nAnonymousTcpMax = BBQPROXY_ANONYMOUSTCP_MAX;
		m_config.nAnonymousTcpIdle = BBQPROXY_ANONYMOUSTCP_IDLE * 1000;

		m_config.nBandwidthMax = BBQPROXY_BANDWIDTH_MAX;
		m_config.nStatTime = BBQPROXY_STAT_TIME;
	}

	if( ! m_pCurrentProxy ) {
		m_pCurrentProxy = new BBQProxy();
		if( ! m_pCurrentProxy ) return false;
	}

	return m_pCurrentProxy->StartRunning();
}

bool BBQProxy::StopProxy( void )
{
	if( m_pCurrentProxy ) {
		m_pCurrentProxy->StopRunning();

		delete m_pCurrentProxy;
		m_pCurrentProxy = NULL;
	}

	return true;
}

bool BBQProxy::StartRunning( void )
{
	if( strlen(m_config.lpszLogPath) ) {
		m_UserActionLog.Open( m_config.lpszLogPath, m_config.nLogMaxSize );
		m_UserActionLog.SetLevel( m_config.nLogLevel );

#ifdef _DEBUG
		m_UserActionLog.SetLevel( PLog::Debug2 );
#endif
	}

	// if the tcp port is open, change to another port if different
	if( ! m_pTcpListener ) {
		m_pTcpListener = new PTCPSocket;
		DWORD ip = (m_config.dwListenIp ? m_config.dwListenIp : INADDR_ANY);
		while( ! m_pTcpListener->Listen( ip, m_config.nListenBacklogSize, m_config.nTcpPort /*, PSocket::CanReuseAddress*/ ) ) 
			m_config.nTcpPort ++;

		PString strIp = ip ? BBQMsgTerminal::ToString(ip) : "*";
		PString strMsg( PString::Printf, "Proxy TCP listen port: %s:%d.", (const char *) strIp, m_config.nTcpPort );
		PLOG( & m_UserActionLog, Info, strMsg );
	}

	// create forwarding threads
	for( int i=0; i<m_config.nForwardThreads; i++ ) {
		new ForwardThread( *this );
	}

	if( m_pCurrentProxy->IsSuspended() ) {
		m_pCurrentProxy->Resume();
	}

	PLOG( & m_UserActionLog, Info, "Proxy engine startup and running." );

	return true;
}

bool BBQProxy::StopRunning( void )
{
	// write stat log
	PString strMsg( PString::Printf, "today stat report: total time: %d minutes, total channels: %d (udp: %d, tcp: %d, broken: %d)",
		m_stat.nMinutes, (m_stat.nTcp + m_stat.nUdp), m_stat.nUdp, m_stat.nTcp, m_stat.nBroken );
	PLOG( & m_UserActionLog, Info, strMsg );

	// set flag to exit thread main loop
	m_bRunning = false;

	{
		ReadLock safe( m_mutexNet );
		if( m_pTcpListener ) {
			m_pTcpListener->Close();
		}
	}

	// wait the thread to terminate
	if( ! IsTerminated() ) {
		if( GetThreadId() != GetCurrentThreadId() ) {
			if( IsSuspended() ) Resume();
			WaitForTermination();
		}
	}

	return true;
}

BBQProxy::ForwardThread::ForwardThread( BBQProxy & owner )
: PThread(20480, NoAutoDeleteThread), m_owner(owner), m_bExit(false)
{
	{
		WriteLock safe( m_owner.m_mutexNet );
		m_owner.m_listForwardThreads.push_back( this );
	}
	Resume();
}

BBQProxy::ForwardThread::~ForwardThread()
{
	m_bExit = true;

	if( ! IsTerminated() ) {
		if( GetThreadId() != GetCurrentThreadId() ) {
			if( IsSuspended() ) Resume();
			WaitForTermination();
		}
	}

	{
		WriteLock safe( m_mutex );
		while( m_listChannelEntry.size() > 0 ) {
			Entry * pEntry = m_listChannelEntry.front();
			m_listChannelEntry.pop_front();

			delete pEntry;
		}
	}

	{
		WriteLock safe( m_owner.m_mutexNet );
		for( ft_iterator iter = m_owner.m_listForwardThreads.begin(), eiter = m_owner.m_listForwardThreads.end(); iter != eiter; iter ++ ) {
			if( this == *iter ) {
				//m_owner.m_listForwardThreads.erase( iter );
				STL_ERASE( m_owner.m_listForwardThreads, ft_iterator, iter );
				break;
			}
		}
	}

}

void BBQProxy::ForwardThread::Main( void )
{
	MS_TIME msLastTime = BBQMsgTerminal::GetHostUpTimeInMs();

	// prepare the select list, suspend running if no fd in the list
	fd_set fdsetRead, fdsetWrite;

	FD_ZERO( & fdsetRead );
	FD_ZERO( & fdsetWrite );

	while( ! m_bExit ) {
		 
		
		// remove the channels that idle for too long
		MS_TIME msNow = BBQMsgTerminal::GetHostUpTimeInMs();
		if( msNow - msLastTime >= FIVE_SECONDS_IN_MS ) {
			msLastTime = msNow;

			// remove those channels that idle for 60 seconds
			{
				WriteLock safe( m_mutex );
				m_owner.ValidateActiveChannel( m_listChannelEntry );
			}
		}

		int fdmax = 0;
		{
			WriteLock safe( m_mutex );
			fdmax = m_owner.PrepareSelectForChannel( m_listChannelEntry, & fdsetRead, & fdsetWrite );
		}
		if( fdmax <= 0 ) {
			// sleep to avoid empty loop
			//printf("fdmax=[%d]\n", fdmax);
			sleep_ms(1);
			continue;
		}

		// do a select to wait I/O happen
		struct timeval tval = { 0, 200000 };
		int ret = ::select( fdmax+1, & fdsetRead, & fdsetWrite, NULL, &tval);
		if( ret < 0 ) {

#if defined(_WIN32) && defined(PTRACING) 
			int err = WSAGetLastError();
			const char * errString = "Unknown";
			switch( err ) {
			case WSANOTINITIALISED:		errString = "WSANOTINITIALISED";	break;
			case WSAEFAULT:				errString = "WSAEFAULT";			break;
			case WSAENETDOWN:			errString = "WSAENETDOWN";			break;
			case WSAEINVAL:				errString = "WSAEINVAL";			break;
			case WSAEINTR:				errString = "WSAEINTR";				break;
			case WSAEINPROGRESS:		errString = "WSAEINPROGRESS";		break;
			case WSAENOTSOCK:			errString = "WSAENOTSOCK";			break;
			}
			PTRACE(1, "Select() return error: " << errString);
#else
			
#endif
			PTRACE(1, "Select() return error: ");
			//printf("Select() return error:[%d]\n", errno);

			FD_ZERO( & fdsetRead );
			FD_ZERO( & fdsetWrite );
			sleep_ms(1);
			continue;
			//break;
		} else if( ret == 0 ) {
			FD_ZERO( & fdsetWrite );
			//Sleep(PTimeInterval(1));
			sleep_ms(1);
			continue;
		}

		// handle event, some event might be ignored if other condition not matches
		bool bSomethingDone = false;
		{
			WriteLock safe( m_mutex );
			if( m_owner.HandleChannelEvent( m_listChannelEntry, & fdsetRead, & fdsetWrite 
				#ifndef _WIN32	
				, ret 
				#endif 
				) 
				) bSomethingDone = true;
		}
		// if nothing done in this cycle, we must wait some time, to avoid empty loop
		if( ! bSomethingDone ) {
			//sleep_ms(10); 
		}
		//Sleep(PTimeInterval(1));
		sleep_ms(10);
		FD_ZERO( & fdsetWrite );
	}
}

bool BBQProxy::StatChannel( bool bTcp, bool bBroken, MS_TIME msStart, MS_TIME msEnd )
{
	DWORD dwDuration = (DWORD)((msEnd - msStart) + 30000) / 60000;
	if( dwDuration > 0 ) {
		m_stat.nMinutes += dwDuration;
	}		

	if( bTcp ) {
		m_stat.nTcp ++;
		if( bBroken ) m_stat.nBroken ++;
	} else {
		m_stat.nUdp ++;
	}

	return true;
}
