
#ifndef _MSG_TERMINAL_H
#define _MSG_TERMINAL_H

#include <ptlib.h>
#include <ptlib/sockets.h>

#include <list>
#include <map>

#include "log.h"
#include "sfidmsg.h"
#include "msgconnection.h"
#include "rwlock.h"
extern void  SetSocketOptions(int fd);
#define		SUPPORT_BIG_MESSAGE
/* ----------------------------------------------------------------------------------------------------
 * Why support BIG MESSAGE? 
 * 
 * UDP packet is limited to 1500 bytes (MTU) for most routers, and only 576 bytes for dial-up networking, 
 * including 20 bytes of IP header, 8 bytes of UDP header, so big message cannot be transfered by a single UDP packet.
 *
 * We cut a big message and pack into several UDP packets, and the receiver will merge them to a single one.
 * All segments of a message must be received in 10 seconds after the first packet arrive, 
 * or else the data will be dropped.
 *
 * A big message is limited to 64KB in current version. 
 *
 * However, too big message is not recommended, cause the bigger the message is, the more UDP packets 
 * will be used to transfer segments of this message, and the more possible to be lost while network busy.
 * ---------------------------------------------------------------------------------------------------- */

#define		ENCRYPT_MESSAGE
/* ----------------------------------------------------------------------------------------------------
 * Why ENCRYPT MESSAGE?
 *
 * Since some stupid NAT (Linksys) sometimes convert all ip:port data in packets even it is not IP header.
 * So, encrypt all data in msg data body before send, and decrypt after received.
 *
 * And, notice that this make all client/server not compartible with previous versions.
 *
 * ---------------------------------------------------------------------------------------------------- */

#define		USE_MAP_FOR_LIST
/* ----------------------------------------------------------------------------------------------------
 * Why use map for list?
 *
 * use <map> to locate object faster than <list>, when send message loop
 *
 * for tcp listeners, map<port, PTCPSocket *>
 * for udp listeners, map<port, PUDPSocket *>
 * for tcp connections, map<fd, BBQMsgConnection *>
 *
 * ---------------------------------------------------------------------------------------------------- */

#define		BLOCK_SOCKET_WHEN_QUEUE_FULL
/* ----------------------------------------------------------------------------------------------------
 * too many packet drop when queue full.
 * now, if queue full, skip read, block it in socket
 * ---------------------------------------------------------------------------------------------------- */

typedef struct SIM_MSGPOST 
{
	MS_TIME			start, next_try;
	DWORD			interval;
	int				total, counter;
	SIM_REQUEST		req;
} SIM_MSGPOST;

#define SIM_NEXTSYNC( req ) if( (req) ->msg.simHeader.sync == 0 ) { (req) ->msg.simHeader.sync = GetNextSync(); }

class BBQMsgTerminal : public PThread
{
	PCLASSINFO( BBQMsgTerminal, PThread )

#ifdef SUPPORT_BIG_MESSAGE
	class SegSession : public PObject 
	{
		PCLASSINFO( SegSession, PObject )
	private:
		uint32	sync;
		uint32	byte_total, byte_left;
		uint16	segment_total;
		char*	segment_flags;
		SIM_REQUEST * req;
	protected:
		MS_TIME	timestamp;
		friend class BBQMsgTerminal;
	public:
		SegSession( const SIM_REQUEST * reqseg );
		~SegSession();
		
		bool Match( const SIM_REQUEST * reqseg );
		SIM_REQUEST * GetRequest( void );
	};
#endif

	class MsgWait : public PSemaphore
	{
		PCLASSINFO( MsgWait, PSemaphore )
	private:
		BBQMsgTerminal & m_owner;

		const SIM_REQUEST * m_pFilter;
		SIM_REQUEST *	m_pReq;
		bool			m_bACK;
	public:
		// called by the wait thread
		MsgWait( BBQMsgTerminal & owner, const SIM_REQUEST * pFilter, bool bACK = false );
		~MsgWait();

		bool WaitRequest( SIM_REQUEST * & reply, DWORD timeout );
		bool MatchFilter( const SIM_REQUEST * req );
	};

	class TcpListenerThread : public PThread
	{
		PCLASSINFO( TcpListenerThread, PThread )
	private:
		BBQMsgTerminal &	m_owner;
		PTCPSocket *		m_pSock;
	public:
		TcpListenerThread( BBQMsgTerminal & owner, PTCPSocket * pSock );
		~TcpListenerThread();

		// call this function to make the thread to exit
		void Close();

		// note: we can create or notify thread to exit at any time, 
		// so that we can change the number of handlers dynamically

		virtual void Main( void );
	};

	class HandlerThread : public PThread
	{
		PCLASSINFO( HandlerThread, PThread )
	private:
		BBQMsgTerminal &	m_owner;
		bool				m_bClose;
	public:
		HandlerThread( BBQMsgTerminal & owner );
		~HandlerThread();

		// call this function to make the thread to exit
		void Close();

		// note: we can create or notify thread to exit at any time, 
		// so that we can change the number of handlers dynamically

		virtual void Main( void );
	};

	class TickThread : public PThread
	{
		PCLASSINFO( TickThread, PThread )
	private:
		BBQMsgTerminal &	m_owner;
	public:
		TickThread( BBQMsgTerminal & owner );
		~TickThread();

		virtual void Main( void );
	};

protected:

	DWORD m_dwConnectTimeout;	// timeout value for Connect(...)

	// -------------------------------------------------------------------------------------
	// mutex to access logger
	mutable PMutex				m_mutexLog;
	PLog *						m_pLogger;

#define MSGLOG0( level, str ) \
	{ LockIt safe( m_mutexLog ); PLOG( m_pLogger, level, str ); }

#define MSGLOG( level, format, var ) \
	{ LockIt safe( m_mutexLog );  PLOG( m_pLogger, level, PString(PString::Printf, format, var) ); }

	// -------------------------------------------------------------------------------------
	// mutex to access network interfaces
	mutable PMutex									m_mutexNet;
	mutable PMutex									m_mutexP2pAck;

	DWORD											m_dwConnectionIdleMax;
	unsigned int									m_nConnectionMax;

	bool AcceptNewConnection( PTCPSocket * pSock );

#ifdef USE_MAP_FOR_LIST
	std::map<WORD, TcpListenerThread *>						m_listTcpListenerThreads;
	typedef std::map<WORD, TcpListenerThread *>::iterator	tth_iterator;

	std::map<WORD, PTCPSocket *>							m_listTcpListeners;
	typedef std::map<WORD, PTCPSocket *>::iterator			tcp_iterator;

	std::map<WORD, PUDPSocket *>							m_listUdpListeners;
	typedef std::map<WORD, PUDPSocket *>::iterator			udp_iterator;

	std::map<int, BBQMsgConnection *>						m_listMsgConnections;
	typedef std::map<int, BBQMsgConnection *>::iterator		con_iterator;

	#define	VAR_AT(x) ((x)->second)
#else
	std::list<WORD, TcpListenerThread *>					m_listTcpListenerThreads;
	typedef std::list<WORD, TcpListenerThread *>::iterator	tth_iterator;

	std::list<PTCPSocket *>									m_listTcpListeners;
	typedef std::list<PTCPSocket *>::iterator				tcp_iterator;

	std::list<PUDPSocket *>									m_listUdpListeners;
	typedef std::list<PUDPSocket *>::iterator				udp_iterator;

	std::list<BBQMsgConnection *>							m_listMsgConnections;
	typedef std::list<BBQMsgConnection *>::iterator			con_iterator;

	#define VAR_AT(x) (*(x))
#endif

#ifdef SUPPORT_BIG_MESSAGE
	// -------------------------------------------------------------------------------------
	// feature to send & receive big messages, split into multiple packets
	mutable PMutex									m_mutexMsgSegment;
	std::list<SegSession *>							m_listMsgSegments;
	typedef std::list<SegSession *>	::iterator		seg_iterator;

	uint32											m_nSegSyncSerial;
	WORD											m_nUDPDataLimit;
#endif

	// -------------------------------------------------------------------------------------
	// mutex to access msg post queue
	mutable PMutex									m_mutexMsgPost;

	// mutex to access msg wait queue
	mutable PMutex									m_mutexMsgWait;

	friend class MsgWait;

	std::list<MsgWait *>							m_listMsgWaits;
	typedef std::list<MsgWait *>::iterator			wait_iterator;

	std::list<SIM_MSGPOST *>						m_listMsgPosts;
	typedef std::list<SIM_MSGPOST *>::iterator		post_iterator;
	int												m_nMsgPostMax;

	// -------------------------------------------------------------------------------------
	// mutex, semaphore to access request and handler queue
	unsigned int									m_nRequestQueueMax;

	mutable PMutex									m_mutexRequest;

	mutable PSemaphore								m_semRequest;
	unsigned int									m_nWaitingHandlers;

	std::list<SIM_REQUEST *>						m_listRequests;
	typedef std::list<SIM_REQUEST *>::iterator		req_iterator;

	std::list<HandlerThread *>						m_listRequestHandlers;
	typedef std::list<HandlerThread *>::iterator	reqh_iterator;

	// can dynamically change number of handlers, by creating new thread or notify current thread to exit
	void	SetNumberOfHandlers( int n );

	bool PutRequestToQueue( SIM_REQUEST * req );
	bool CopyRequestToQueue( const SIM_REQUEST * req );
	SIM_REQUEST* GetRequestFromQueue( void );

	friend class HandlerThread;

	// -------------------------------------------------------------------------------------
	TickThread			* m_pTickThread;
	friend class TickThread;

private:

#ifdef ENCRYPT_MESSAGE
	bool						m_bEncryptMessage;
	char *						m_pEncBuffer;
	int							m_nEncBufferSize;
#endif

	SIM_MESSAGE *				EncryptMessage( const SIM_MESSAGE * plain );
	SIM_MESSAGE *				DecryptMessage( const SIM_MESSAGE * enc );

	bool		m_bClosing;

	mutable PMutex				m_mutexSyncCounter;
	uint16		m_nSyncCounter;

	// NOTE: about fd_set size limit of 64

	// Under Linux/Unix environment, (see man page of select, poll)
	// fd_set can only hold up to 64 file descriptors, use poll(...) instead of select(...) if more than that.

	// Under MS Windows environment, (see MSDN, Platform SDK: Windows Sockets 2)
	// four macros are defined in the header file Winsock2.h for manipulating and checking the descriptor sets. 
	// The variable FD_SETSIZE determines the maximum number of descriptors in a set. 
	// (The default value of FD_SETSIZE is 64, which can be modified by defining FD_SETSIZE to another value before including Winsock2.h.) 
	// Internally, socket handles in an fd_set structure are not represented as bit flags as in Berkeley Unix. 
	// Their data representation is opaque. Use of these macros will maintain software portability between different socket environments. 

	bool		m_bSkipReadWhenQueueFull;

	int	PrepareSelect( fd_set * pfdsetRead, fd_set * pfdsetWrite );
	void HandleMsgConnectionsEvent( fd_set * pfdsetRead, fd_set * pfdsetWrite );
	void HandleUdpListenersEvent( fd_set * pfdsetRead, fd_set * pfdsetWrite );
	void HandleTcpListenersEvent( fd_set * pfdsetRead, fd_set * pfdsetWrite );

	void LoopNetIO( void );
	// ------------------------------------------------

#ifdef SUPPORT_BIG_MESSAGE
	void OnMessageSegmentArriveInternal( const SIM_REQUEST * req );
#endif

	void OnMessageArriveInternal( const SIM_REQUEST * req );
	virtual void OnMessageArriveACK( const SIM_REQUEST * req )=0;

	bool HandleMessageWait( const SIM_REQUEST * req );

protected:
	//static char		m_AESkey[256];
	PIPSocket::Address				m_addrLocalinterface;

	int	GetMaxSocketSize( void );
	uint16 GetNextSync() {
		PWaitAndSignal lock( m_mutexSyncCounter );
		do 
		{
			m_nSyncCounter++;
		} while( m_nSyncCounter == 0 );
		return m_nSyncCounter;
	}
	virtual bool NeedAck( uint16 msg ) const;

	void Destruct( void );
	std::list<uint32> m_listP2pFileAcks;
public:
  BBQMsgConnection * FindMsgConnectionEx( DWORD ip, WORD port, WORD localport ,int fd );
	BBQMsgTerminal( unsigned int nMsgQueueMax = 1024, int nHandlers = 1, bool bEncryptMessage = true );
	virtual ~BBQMsgTerminal();

	bool IsClosing( void ) { return m_bClosing; }

	void SetLogger( PLog * pLogger ) { 
		m_pLogger = pLogger; 
	}

	static PString ToString( DWORD ip );
	static PString ToString( const IPSOCKADDR * pAddr );
	static PString ToString( const SIM_CHANNEL * pChannel, bool out = true );

	static DWORD HostnameToIp( const char * lpszHostname );
	static bool HostnameToIp( const char * lpszHostname, DWORD & ip );

	static DWORD GetHostUpTime( void ); // uptime in second
	static MS_TIME GetHostUpTimeInMs( void ); // uptime in ms

	// ------------------------------------------------------------------------
	// TCP listener interface
	bool AttachTcpListener( PTCPSocket * pSock );
	bool DetachTcpListener( PTCPSocket * pSock );
	PTCPSocket * FindTcpListener( WORD port = 0 );

	PTCPSocket *  ListenTcp( WORD port = 0, unsigned int queueSize = 5, bool bSeperateThread = true, const PIPSocket::Address & bind =  INADDR_ANY, bool bNonBlock= true );
	void StopListenTcp( WORD port );

	// ------------------------------------------------------------------------
	// UDP receive & send interface
	bool AttachUdpListener( PUDPSocket * pSock );
	bool DetachUdpListener( PUDPSocket * pSock );
	PUDPSocket * FindUdpListener( WORD port = 0 );

	bool ListenUdp( WORD port = 0, const PIPSocket::Address & bind =  INADDR_ANY );
	void StopListenUdp( WORD port );

#ifdef SUPPORT_BIG_MESSAGE
	int SetUdpPacketSizeLimit( int nBytes );
#endif

	// ------------------------------------------------------------------------
	// TCP connection
	bool AttachMsgConnection( BBQMsgConnection * pSock );
	bool DetachMsgConnection( BBQMsgConnection * pSock );

	void SetConnectTimeout( DWORD dwTimeout = 30000 ); // default 30 s
	int Connect( PIPSocket::Address addr, WORD port, DWORD dwTimeout = 0 ); // if 0, use global setting
	int Connect( const PString & addr, WORD port, DWORD dwTimeout = 0 ); // support proxied connection using hostname

	bool ValidateTcpChannel( SIM_CHANNEL & chan );

	BBQMsgConnection * FindMsgConnection( DWORD ip = 0, WORD port = 0, int fd = 0 );
	void CloseMsgConnection( DWORD ip = 0, WORD port = 0, int fd = 0 );

	void SetConnectionIdleMax( DWORD dwMax = 0 ) { m_dwConnectionIdleMax = dwMax; }
	void SetConnectionMax( unsigned int nMax = 0 ) { m_nConnectionMax = nMax; }

	// ----------------------------------------------------------------------------
	// return false if there is no proper channel to send the message
	// ( A LITTLE TRICK: the tcp msg connection will be disconnected if send a SIM_NONE msg )
	bool SendMessage( SIM_REQUEST * req, bool bWaitACK = false, DWORD timeout = 1000, int nRetry = 6, bool bAutoFillAync=true );

	// put this message into a queue to send in background by another thread
	void PostMessage( SIM_REQUEST * req, bool bWaitACK = false, DWORD timeout = 600, int nRetry = 6 );

	SIM_REQUEST * RequestMessage( SIM_REQUEST * req, uint16 msgFilter = 0, DWORD timeout = 0, int nRetry = 6 );

	// the terminal service thread must keep running and dispatching message, 
	// so it is not allowed to wait on its own
	SIM_REQUEST * WaitMessage( const SIM_REQUEST * filter = NULL, DWORD timeout = 0 );

	void BreakMessageWaiting( void );

	void ReleaseMessage( /*SIM_REQUEST*/void * req );

	void EraseChannel( SIM_CHANNEL & chan );

	virtual void Main(void);

	 void GetHostAddress(PIPSocket::Address& local_adr);
private:
	bool SendMessageInternal( const SIM_REQUEST * req );
#if 0
	bool SendMessageInternal( const SIM_CHANNEL * channel, const SIM_MESSAGE * msg );
#endif	//  Commented by WZG: not used
protected:
	// call this in destructor, before any real destructing statement
	void OnClose(void);

	// -------------------------------------------------------------------------------------
	// the following virtual functions are encouraged to override 
	// -------------------------------------------------------------------------------------

#define DECLARE_BBQ_MESSAGE_MAP() \
	virtual bool OnMessageArrive( const SIM_REQUEST * req );

#define BEGIN_BBQ_MESSAGE_MAP( thisClass, parentClass ) \
bool thisClass::OnMessageArrive( const SIM_REQUEST * req ) { \
	if( parentClass::OnMessageArrive( req ) ) return true; \
	switch( req->msg.simHeader.id ) { 

#define BBQ_MESSAGE_MAP(msgid, msgfunc) \
	case (msgid): return msgfunc( req );

#define END_BBQ_MESSAGE_MAP() \
	case SIM_NONE: return true; \
	} \
	return false; \
}

	// return true if the message is handled, default false
	virtual bool OnMessageArrive( const SIM_REQUEST * req );

	// connection max limit reached
	virtual void OnMsgConnectionMaxOverflow( BBQMsgConnection * pConn );

	// called when the connection is just open
	virtual void OnMsgConnectionOpen( BBQMsgConnection * pConn );

	// called when the connetion is closing (by local) or closed (by peer)
	virtual void OnMsgConnectionClose( BBQMsgConnection * pConn );

	// return true to kick this idle connection, default true
	virtual bool OnMsgConnectionIdle( BBQMsgConnection * pConn );

	// this function will be called every second
	virtual void OnTick( void );

};

#endif //  _MSG_TERMINAL_H
