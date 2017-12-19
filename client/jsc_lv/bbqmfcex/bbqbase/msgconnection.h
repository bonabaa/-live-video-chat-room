#ifndef _MSG_CONNECTION_H
#define _MSG_CONNECTION_H

#define USE_STD_LIST

#include <ptlib.h>
#include <ptlib/sockets.h>

#include <list>

#include "sfidmsg.h"

#include "byteblock.h"

#define MSGCONNECTION_READBUFSIZE	sizeof(SIM_MESSAGE)
#define MSGCONNECTION_MSGQUEUEMAX	32

class BBQMsgConnection : public PObject
{
	PCLASSINFO( BBQMsgConnection, PObject )

protected:

	PTCPSocket *		m_pSock;

	std::list<PByteBlock *>	m_QueueWrite;

	BYTE				m_ReadBuf[ MSGCONNECTION_READBUFSIZE ];
	int					m_nReadBytes;
	MS_TIME				m_msLastActionTime;

	bool				m_bIncoming;
	DWORD				m_dwPrivateData;

	bool				m_bAutoDelete;

	friend class BBQMsgTerminal;

public:

	void UpdateTimestamp( void );
	
	//
	// if flag "bAutoDelete" is on, 
	// when this msg connection is closed, it will be auto delete from msg terminal.
	//
	BBQMsgConnection( bool bIncoming = true, PTCPSocket * pSock = NULL, bool bAutoDelete = true );

	// 
	// pSock attached will also be deleted. If not hope so, detach it before destructor
	//
	~BBQMsgConnection();

	PTCPSocket * GetSocket( void ) {
		return m_pSock;
	}

	PTCPSocket * DetachSocket( void ) {
		PTCPSocket * pSock = m_pSock;
		m_pSock = NULL;
		return pSock;
	}

	PByteBlock * DetachExtraData( void ) {
		PByteBlock * pData = NULL;
		if( m_nReadBytes > 0 ) {
			pData = new PByteBlock( (const char *) m_ReadBuf, m_nReadBytes );
		}
		return pData;
	}

	void AttachSocket( PTCPSocket * pSock ) {
		if( m_pSock ) delete m_pSock;
		m_pSock = pSock;
	}

	int Write( const char * pBuf, int nLen );
	bool Queue( const char * pBuf, int nLen, bool bForce = false );

	bool PickMessageFromBuffer( SIM_MESSAGE * msg );
	bool ReadMessage( SIM_MESSAGE * msg );
	bool WriteMessage( const SIM_MESSAGE * msg, bool bBlockMode = false );

	bool FlushMessage( bool bBlockMode = false );

	bool HasDataToWrite( void ) { 
		return ( m_QueueWrite.size() > 0 ); 
	};

	MS_TIME GetLastActionTime( void ) { return m_msLastActionTime; }
	bool IsIdle( DWORD s = 300000 );

	bool IsIncoming( void ) { return m_bIncoming; }

	void SetPrivateData( DWORD dwData ) { m_dwPrivateData = dwData; }
	DWORD GetPrivateData( void ) { return m_dwPrivateData; }

};

#endif // _MSG_CONNECTION_H
