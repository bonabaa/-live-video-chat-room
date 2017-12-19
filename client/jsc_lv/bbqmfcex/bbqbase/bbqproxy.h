
#ifndef _BBQ_PROXY_H_
#define _BBQ_PROXY_H_

//
// BBQProxy is a special built-in feature of BBQMsgTerminal, which can forward UDP/TCP data between 2 VFON clients.
//
// Any application derived from BBQMsgTerminal can work as BBQProxy, 
// including VServer, MCU, even user's VFON client, if this feature is enabled.
//
// Valid BBQProxy must have a real internet IP address, so that it can be accessed by clients behind NAT.
//
// BBQProxy must register to VServer with this feature flag on, and VServer maintains a dynamic table of active proxies.
//

#include <ptlib.h>
#include <ptlib/sockets.h>
#include <ptclib/pssl.h>

#include <list>
#include <map>

#include "sfidmsg.h"

#ifndef _WIN32
//chen yuan 
//#include "pthread2.h"
//
#endif

//#if defined(_WIN32) && defined(FD_SETSIZE)
//#if FD_SETSIZE == 64
//#error fd_set size limit is too small, please define a larger FD_SETSIZE before including <winsock2.h>
//#endif
//#endif

#define	BBQPROXY_TCPPORT				4522
#define	BBQPROXY_UDPPORT_MIN			6000
#define BBQPROXY_UDPPORT_MAX			9000

#define BBQPROXY_CHANNEL_MAX			600
#define BBQPROXY_CHANNELBUFFER_MAX		409600	// 400 KBytes, per channel
#define BBQPROXY_CHANNEL_IDLE			60		// 60 seconds

#define BBQPROXY_ANONYMOUSTCP_MAX		64
#define BBQPROXY_ANONYMOUSTCP_IDLE		30		// 30 seconds

#define BBQPROXY_BANDWIDTH_MAX			102400	// 100 Mbps
#define BBQPROXY_STAT_TIME				3600	// 1 hour

enum em_ChannelTypeExt
{
  em_DataChannelForConference=1, em_DataChannelForViewer=0
};
typedef struct BBQPROXY_CONNECT {
	uint32				nChannelId;
	uint32				uid;
	uint32				uidPeer;
	uint32				cookie;

	uint32				statusCode;
	uint32				useSSL;

	char				groupName[ 32 ];	// PCCW_IVC, PCCW_CVG, PCCW_xxx
  char        emChannelType;//0:datachannel for conference,1 data channel for viewer
	char				reserved[ 7 ];
} BBQPROXY_CONNECT;	// 64 bytes

#define BBQPROXY_BUF_MAX		(4069 + sizeof(SIM_MESSAGE) * 2) // should be more than 4K + 2 * sizeof(SIM_MESSAGE)

class BBQProxy : public PThread
{
	PCLASSINFO( BBQProxy, PThread );

public:
	enum PROXY_TYPE {
		PROXY_NONE,
		PROXY_UDP,
		PROXY_TCP,
		PROXY_SSL,
	};

	static PString TypeToString( int n );

	class Entry {
		BBQProxy &				m_owner;
	public:
		Entry(BBQProxy & owner);
		~Entry();

		uint32					m_nChannelId;		// channel Id
		uint32					m_uid;			
		uint32					m_cookie;			// for security checking

		Entry *					m_peer;
		uint32					m_peer_uid;

		MS_TIME					m_tInit;			// init time
		MS_TIME					m_timestamp;		// for idle checking

		bool					m_bChannelSource;	// channel source, from this to peer

		bool					m_bIdentified;		// already identified by sending connect request
		bool					m_bConnected;		// channel already connected by both client

		DWORD					m_nGigaBytesSendByThis;
		DWORD					m_nBytesSendByThis;		// total bytes sent till now

		DWORD					m_nBytesSendLast;		// total bytes sent some times ago
		MS_TIME					m_nBytesSendLastTime;	// the time to record total bytes, to calculate bps

		PROXY_TYPE				m_nProxyType;

		PUDPSocket *			m_pUdpSock;
		PTCPSocket *			m_pTcpSock;
		PSSLChannel *			m_pSSLChannel;

		FILE *					m_fpTrafficLog;
		FILE *					m_fpTrafficData;

		WORD					m_nPort;			// local UDP port to receive data
		IPSOCKADDR				m_addr;				// address, where to send/receve data, especially for the UDP channel

		PByteBlock				m_ReadBuf;			// buffer for read, used if we wanna read a whole message each time

		PByteBlock				m_queueWrite;
		int						m_nQueueBytes;
		
		////chen yuan
		char buf[ BBQPROXY_BUF_MAX ];

    bool bWithHeader;
		int						GetHandle( void );

		int						Write( const char * pBuf, int nLen );
		int						Read( char * pBuf, int nBuf );
		void					Close( void );

		int						Flush( void );
		int						ForwardToPeer( void );
		bool					Queue( const char * pBuf, int nLen );
		bool					HasDataInQueue( void );
		
	};

	class ForwardThread : public PThread
	{
		PCLASSINFO( ForwardThread, PThread )

	public:
		BBQProxy &				m_owner;
		bool					m_bExit;
		mutable PReadWriteMutex	m_mutex;
		//PMutex	m_mutex;
		std::list<Entry *>		m_listChannelEntry;

		ForwardThread( BBQProxy & owner );
		~ForwardThread();

		virtual void Main( void );
	};

	std::list<ForwardThread *>	m_listForwardThreads;
	typedef std::list<ForwardThread *>::iterator ft_iterator;

	typedef struct AnonymousTcp {
		PTCPSocket *			pSock;
		MS_TIME					timestamp;
	} AnonymousTcp;

	struct Config {
		struct {
			uint32	major, minor, build;
		} Version;

		uint32	nUserMax;
		time_t	tExpireTime;

		DWORD	dwListenIp;
		DWORD	dwExternalIp;

		WORD	nAdminPort;
		WORD	nTcpPort;
		WORD	nListenBacklogSize;
		WORD	nUdpPortMin, nUdpPortMax;

		int		nForwardThreads;				// how many thread to handle channel data forwarding

		int		nChannelMax;
		int		nChannelBufferMax;

		int		nBandwidthMax;					// in Kbps

		int		nAnonymousTcpMax;

		int		nAnonymousTcpIdle;
		int		nChannelIdle;

		int		nStatTime;						// max length of bandwidth stat list

		int		nLogLevel;
		int		nLogMaxSize;
		char	lpszLogPath[ 256 ];

		int		nLogTraffic;					// write traffic info of each channel into log, for troubleshooting only

		char	lpszSSLCertPath[ 256 ];
		char	lpszSSLKeyPath[ 256 ];
		char	lpszSSLKeyPassword[ 256 ];
	};

	struct Status {
		WORD	nNextUdpPort;
		uint32	nNextChannelId;

		int		nEntries;;
		int		nBufferBytes;

		int		nHistoryEntries;
		int		nHistoryBytes, nHistoryGigaBytes;

		int		nBytesLastFiveSecond;			// bytes IN/OUT in last 5 seconds
		int		nAverageBandwidth;				// in Kbps

		time_t	tUpTime;

		bool	bSSLInited;
	};

	struct SessionInfo {
		
		uint32					m_nChannelId;		// channel Id
		MS_TIME					m_tInit;			// init time
		MS_TIME					m_timestamp;		// for idle checking

		PROXY_TYPE				m_nProxyType;		// proxy type

		struct {
			uint32					m_uid;			// uid
			WORD					m_nPort;		// local UDP port to receive data
			IPSOCKADDR				m_addr;			// address, where to send/receve data, especially for the UDP channel
			int						m_nQueueBytes;
			int						m_nSendGigaBytes;
			int						m_nSendBytes;
			int						m_nSendBps;
		} entry[2];

	};

	typedef std::list<SessionInfo *>		SessionInfoList;
	typedef std::map<uint32, SessionInfo *>	SessionInfoMap;

protected:
	friend class Entry;
	friend class BBQClient;
	friend class BBQServer;

	static	BBQProxy *						m_pCurrentProxy;

	// mutex to access network interfaces
	mutable PReadWriteMutex					m_mutexNet;

	PTCPSocket *							m_pTcpListener;

	std::list<Entry *>						m_listChannelEntry;
	typedef std::list<Entry *>::iterator	pe_iterator;

	std::list<AnonymousTcp *>					m_listAnonymousTcp;
	typedef std::list<AnonymousTcp *>::iterator	at_iterator;

	static Config	m_config;


	std::list<int>	m_bwstat;

protected:

	bool	m_bRunning;
	bool	m_bClosing;

	// NOTE: about fd_set size limit of 64

	// Under Linux/Unix environment, (see man page of select, poll)
	// fd_set can only hold up to 64 file descriptors, use poll(...) instead of select(...) if more than that.

	// Under MS Windows environment, (see MSDN, Platform SDK: Windows Sockets 2)
	// four macros are defined in the header file Winsock2.h for manipulating and checking the descriptor sets. 
	// The variable FD_SETSIZE determines the maximum number of descriptors in a set. 
	// (The default value of FD_SETSIZE is 64, which can be modified by defining FD_SETSIZE to another value before including Winsock2.h.) 
	// Internally, socket handles in an fd_set structure are not represented as bit flags as in Berkeley Unix. 
	// Their data representation is opaque. Use of these macros will maintain software portability between different socket environments. 

	int	PrepareSelect( fd_set * pfdsetRead, fd_set * pfdsetWrite );
	bool HandleTcpListenersEvent( fd_set * pfdsetRead, fd_set * pfdsetWrite );
	bool HandleAnonymousTcpEvent( fd_set * pfdsetRead, fd_set * pfdsetWrite );

	bool AnonymousTcpMatchEntry( PTCPSocket * pSock, std::list<Entry *> & listEntries, SIM_MESSAGE * pmsg, bool bWithHeader );
	int	PrepareSelectForChannel( std::list<Entry *> & listEntries, fd_set * pfdsetRead, fd_set * pfdsetWrite );
	bool HandleChannelEvent( std::list<Entry *> & listEntries, fd_set * pfdsetRead, fd_set * pfdsetWrite 
		#ifndef _WIN32
		 ,int& ifds 
	  #endif
	   );
	int ValidateActiveChannel( std::list<Entry *> & listEntries );

	void LoopNetIO( void );
	// ------------------------------------------------
  //bWithHeader ,when recvieed the BBQPROXY_CONNECT, if bWithHeader== true ,contained a  SF header, else not  

public:
	static BBQProxy * GetCurrentProxy( void );
	static bool StartProxy( const Config * conf = NULL );
	static bool StopProxy( void );
	Status			m_status;

	BBQProxy();
	virtual ~BBQProxy();

	virtual void Main(void);

	// some log related
	PFileLog		m_UserActionLog;

	bool IsRunning( void ) { return m_bRunning; }

	bool SetConfig( Config & conf );

	// interface to get information
	bool GetConfig( Config & conf );
	bool GetStatus( Status & status );
	bool GetBandwidthStat( std::list<int> & stat );
	bool GetSessionInfoList( SessionInfoList & sessions );

	bool CreateChannel( uint32 uidA, uint32 uidB, bool bTCP, Entry * & pEntryA, Entry * & pEntryB );
	bool CutChannel( uint32 nChId );

protected:

	struct StatInfo {
		time_t	tTodayMin, tTodayMax;
		int		nUdp, nTcp, nBroken;
		int		nMinutes;
	} m_stat;

	bool StatChannel( bool bTcp, bool bBroken, MS_TIME msStart, MS_TIME msEnd );

protected:

	bool InternalGetStatus( std::list<Entry *> & listEntries, Status & status );
	bool InternalGetSessionInfoList( std::list<Entry *> & listEntries,  SessionInfoList & sessions );
	bool InternalCutChannel( std::list<Entry *> & listEntries, uint32 nChId );

	PUDPSocket * BindUdpPort( void );

	// call this in destructor, before any real destructing statement
	void OnClose(void);

	bool StartRunning( void );
	bool StopRunning( void );

};

#endif /* _BBQ_PROXY_H_ */
