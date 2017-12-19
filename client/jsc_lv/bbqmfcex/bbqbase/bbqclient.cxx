
#include "bbqbase.h"
#include "bbqclient.h"
#include "bbqproxy.h"
#include "cert_rsa_aes.h"
#include "bzip2/bzlib.h"

//#include "bzip2/bzlib.h"
#if P_SSL
#include <openssl/rand.h>
#endif
#define UDP_BUFFER_SIZE 32768

BBQClient* BBQClient::m_pCurrentClient = NULL;
const char* m_AESkey="abc_%%%333***333&&&%%ddd((sdsds34345354abc_%%%333***333&&&%%ddd((sdsds34345354abc_%%%333***333&&&%%ddd((sdsds34345354abc_%%%333***333&&&%%ddd((sdsds34345354abc_%%%333***333&&&%%ddd((sdsds34345354";

#define	CLIENT_REQUEST_TIMEOUT				5000		//10000
#define CLIENT_LOGINEX_TIMEOUT				5000
#define	CLIENT_SIMPLE_REQUEST_TIMEOUT		2000

#define	CLIENT_REQUEST_CHANNEL_FOR_TESTING_PROXY_MARK		10000//appCode
#ifdef _STREAM_LOG
#define BBQTRACE(level, args) \
			  PLOGEX(  Debug, args )

#else
#define BBQTRACE(level, args) \
			  PTRACE(  level, args )
#endif
 int BZ2_bzBuffToBuffDecompressEx( 
 PBYTEArray&         dest, 
      unsigned int* destLen0,
      char*         source, 
      unsigned int  sourceLen,
      int           small, 
      int           verbosity 
   )
{
	int nResult =0;
	unsigned int destLen= 0, nOutSize = 0;
		
	do {
		 
		nOutSize+= 10240;
		destLen = nOutSize;
	}
	while ((nResult =  BZ2_bzBuffToBuffDecompress((char*) dest.GetPointer(nOutSize), &destLen, source,sourceLen,  small, verbosity)) == BZ_OUTBUFF_FULL) ;
	*destLen0 = destLen;
	return nResult;
}
const char*GetnAppParam(uint32 param)
{
	switch(param)
	{
	case em_SIPDATA_CALL:
		return "data";
	case em_VIDEO_CALL:
	case em_YYMeeting_VIDEO_CALL:
		return "video";
	case em_VIDEO_CONTROL_CALL:
		return "videoControl";
	case em_AUDIO_CALL:
	case em_YYMeeting_AUDIO_CALL:
		return "audio";
	case em_AUDIO_CONTROL_CALL:
		return "audioControl";
	case em_IPP_CALL:
	case em_IPP_CALL_Ext:
		return "IPP";
	case em_P2P_FILE:
		return "File";
	};
	return "";
};
#define IP2STRING( ip ) (const char *)((PIPSocket::Address)(ip)).AsString()

#if PTRACING

#define BBQCHANNEL_LOCAL_TRACE(pChannel) \
	if( pChannel ) { \
		PString str( PString::Printf, \
			"BBQ %s channel requested from local host:\r\n" \
			"    Channel type: %s/%s\r\n" \
			"    Socket handle: %d\r\n" \
			"    Local: (%d)%d, (%s:%d) (%u,%x)\r\n" \
			"    Peer: (%d)%d, (%s:%d) LAN (%s:%d) (%u,%x)\r\n", \
			GetnAppParam(pChannel->info.nLocalAppParam ),\
			(pChannel->tcpSock ? "TCP" : "UDP"), \
			(pChannel->info.bIsProxied ? "Proxied" : "P2P"), \
			(pChannel->tcpSock ? pChannel->tcpSock->GetHandle() : (pChannel->sock ? pChannel->sock->GetHandle() : 0)), \
			m_nServerId, m_sessionInfo.id, \
			IP2STRING( pChannel->info.thisLAN.ip ),  pChannel->info.thisLAN.port, \
			pChannel->info.dwLocalCount, pChannel->info.dwLocalTimeBase, \
			pChannel->info.peerServerId, pChannel->info.peerId,  \
			IP2STRING( pChannel->info.peerWAN.ip ),  pChannel->info.peerWAN.port, \
			IP2STRING( pChannel->info.peerLAN.ip ),  pChannel->info.peerLAN.port, \
			pChannel->info.dwRemoteCount, pChannel->info.dwRemoteTimeBase \
			); \
 \
		PTRACE( 1, str ); \
	}
#define BBQCHANNEL_REMOTE_TRACE(pChannel) \
		PString str( PString::Printf, \
			"BBQ %s channel  requested from remote host:\r\n" \
			"    Channel type: %s/%s\r\n" \
			"    Socket handle: %d\r\n" \
			"    Local: (%d)%d, (%s:%d) (%u,%x)\r\n" \
			"    Peer: (%d)%d, (%s:%d) LAN (%s:%d) (%u,%x)\r\n", \
			GetnAppParam(pChannel->info.nRemoteAppParam ),\
			(pChannel->tcpSock ? "TCP" : "UDP"), \
			(pChannel->info.bIsProxied ? "Proxied" : "P2P"), \
			(pChannel->tcpSock ? pChannel->tcpSock->GetHandle() : (pChannel->sock ? pChannel->sock->GetHandle() : 0)), \
			m_nServerId, m_sessionInfo.id, \
			IP2STRING( pChannel->info.thisLAN.ip ),  pChannel->info.thisLAN.port, \
			pChannel->info.dwLocalCount, pChannel->info.dwLocalTimeBase, \
			pChannel->info.peerServerId, pChannel->info.peerId,  \
			IP2STRING( pChannel->info.peerWAN.ip ),  pChannel->info.peerWAN.port, \
			IP2STRING( pChannel->info.peerLAN.ip ),  pChannel->info.peerLAN.port, \
			pChannel->info.dwRemoteCount, pChannel->info.dwRemoteTimeBase \
			); \
 \
		PTRACE( 1, str );
#else

#define BBQCHANNEL_LOCAL_TRACE(pChannel) 
#define BBQCHANNEL_REMOTE_TRACE(pChannel)

#endif

BEGIN_BBQ_MESSAGE_MAP( BBQClient, BBQMsgTerminal )

	BBQ_MESSAGE_MAP( SIM_SC_LOGINREPLACED,	OnMsgLoginReplaced )

	BBQ_MESSAGE_MAP( UCM_CC_IDQUERY,		OnMsgIdQuery )

	BBQ_MESSAGE_MAP( UCM_CC_Q,				OnMsgChannelDirectRequest )
	BBQ_MESSAGE_MAP( UCM_CC_A,				OnMsgChannelDirectResponse )

	BBQ_MESSAGE_MAP( UCM_SC_FWD_Q,			OnMsgChannelRequest )
	BBQ_MESSAGE_MAP( UCM_SC_A,				OnMsgChannelRequestFeedback )

	BBQ_MESSAGE_MAP( UCM_CC_CON,			OnMsgChannelConnect )
	BBQ_MESSAGE_MAP( UCM_CC_CONCFM,			OnMsgChannelConnectConfirmed )

	BBQ_MESSAGE_MAP( SIM_SC_USERMSG,		OnMsgUserDefinedMessage )
	BBQ_MESSAGE_MAP( SIM_CS_USERMSG,		OnMsgUserDefinedMessage )

	BBQ_MESSAGE_MAP( SIM_SC_INVITEVISIT,	OnMsgTextMessage )
	BBQ_MESSAGE_MAP( SIM_SC_BROADCASTTEXT,	OnMsgTextMessage )
//	BBQ_MESSAGE_MAP( SIM_SC_SENDFILE,	OnMsgSendFileMessage )
	BBQ_MESSAGE_MAP( SIM_CS_SENDTEXT,		OnMsgTextMessage )
//	BBQ_MESSAGE_MAP( SIM_CC_SENDFILEP2P,		OnMsgSendFileP2P )

	BBQ_MESSAGE_MAP( SIM_SC_QUERYSERVICE,	OnMsgServiceChanged )

	BBQ_MESSAGE_MAP( UCM_SP_CREATE,			OnMsgCreateProxyChannel )
	BBQ_MESSAGE_MAP( UCM_SP_RELEASE,		OnMsgReleaseProxyChannel )

	BBQ_MESSAGE_MAP( UCM_SC_PROXYNOTIFY,	OnMsgBBQProxyNotify )
//	BBQ_MESSAGE_MAP( SIM_SC_DOWNLOADFILE_NOTIFY,	OnMsgDownloadFileNotify )

	BBQ_MESSAGE_MAP( SIM_SC_BUDDYSTATUSNOTIFY,	OnMsgBuddyStatusNotify )
	BBQ_MESSAGE_MAP( SIM_SC_SELFBUDDYSTATUSNOTIFY,	OnMsgSelfBuddyStatusNotify )

	BBQ_MESSAGE_MAP( SIM_SC_CONFERENCE,		OnMsgConferenceChangeByServer )

	BBQ_MESSAGE_MAP( SIM_SC_MCC_NOTIFY,		OnMsgMCCNotify )
	BBQ_MESSAGE_MAP( SIM_SC_MCC_NOTIFY_EX,	OnMsgMCCNotifyEx )

	BBQ_MESSAGE_MAP( SIM_SC_NOTIFY_CONTACT,	OnMsgNotifyContact )

	//BBQ_MESSAGE_MAP( SIM_SC_IM,				OnMsgNotifyOfflineIM )

  BBQ_MESSAGE_MAP( SIM_SC_ENTER_ROOM,				OnMsgNotifyEnterRoom )

	BBQ_MESSAGE_MAP( SIM_SC_IM_NOTIFY,		OnMsgNotifyOfflineIM )
	BBQ_MESSAGE_MAP( SIM_SC_KICK,			OnMsgNotifyKickUser )
	BBQ_MESSAGE_MAP( UCM_CM_CONNECT,			OnMsgRequestViewer )
	BBQ_MESSAGE_MAP( SIM_SC_MCC_CMD,			OnNotifyMCCEventCMD )
	BBQ_MESSAGE_MAP( SIM_SC_FREEGROUP_NOTIFY,			OnMsgFreeGroupCmd )
	BBQ_MESSAGE_MAP( SIM_SC_MEETCTRL,			OnMsgMeetingControl )

END_BBQ_MESSAGE_MAP()

BBQClient* BBQClient::GetCurrentClient( void )
{
	return m_pCurrentClient;
}

BBQClient::BBQChannelWait::BBQChannelWait( PUDPSocket * pSock, BBQClient & owner )
	: PSemaphore(0, 1), m_owner(owner) 
{
	m_pSock = pSock;
	m_pTcpSock = NULL;
	m_pSSLChannel = NULL;

	m_bIsProxied = false;

	m_bDone = false;
	m_bConnectingProxy = false;

	m_nAppCode = 0;

	memset( & info, 0, sizeof(info) );
	memset( & peerInfo, 0, sizeof(peerInfo) );

	memset( & extra, 0, sizeof(extra) );
	memset( & peerExtra, 0, sizeof(peerExtra) );

	m_msWaitBeginTime = GetHostUpTimeInMs();
}

BBQClient::BBQChannelWait::~BBQChannelWait()
{
	if( m_pSock ) {
		if( m_owner.DetachUdpListener( m_pSock ) ) delete m_pSock;
		m_pSock = NULL;
	}
	if( m_pTcpSock ) {
		delete m_pTcpSock;
		m_pTcpSock = NULL;
	}
	if( m_pSSLChannel ) {
		delete m_pSSLChannel;
		m_pSSLChannel = NULL;
	}
}

BBQ_CHANNEL* BBQClient::BBQChannelWait::Detach( void )
{
	BBQ_CHANNEL * pChannel = new BBQ_CHANNEL;
	memset( pChannel, 0, sizeof(BBQ_CHANNEL) );

	pChannel->info.peerId = this->info.uid;
	pChannel->info.peerWAN = this->info.peerWAN;
	pChannel->info.peerLAN = this->info.peerLAN;
	pChannel->info.thisLAN = this->info.thisLAN;

	pChannel->info.bIsProxied = this->m_bIsProxied;
	pChannel->info.bInitFromLocal = this->info.init;

	if( m_pSock ) {
		m_owner.DetachUdpListener( m_pSock );
		pChannel->sock = m_pSock;
		m_pSock = NULL;
	}

	if( m_pTcpSock ) {
		pChannel->tcpSock = m_pTcpSock;
		m_pTcpSock = NULL;
	}

	if( m_pSSLChannel ) {
		pChannel->sslChannel = m_pSSLChannel;
		m_pSSLChannel = NULL;
	}

	pChannel->info.peerServerId = this->peerExtra.nServerId;

	// assign param for zhx, used for judge channel conflict
	pChannel->info.dwLocalCount = this->extra.dwChannelId;
	pChannel->info.dwLocalTimeBase = this->extra.dwTimeBase;

	pChannel->info.dwRemoteCount = this->peerExtra.dwChannelId;
	pChannel->info.dwRemoteTimeBase = this->peerExtra.dwTimeBase;

	pChannel->info.nLocalAppParam = this->extra.nAppParam;
	pChannel->info.nRemoteAppParam = this->peerExtra.nAppParam;

	return pChannel;
}

BBQClient::BBQClient( unsigned int nMsgQueueMax, int nHandlers,const char* szServerIP )
	: BBQMsgTerminal(nMsgQueueMax, nHandlers)
	, m_bVerifyServerCert(false)
	, m_nServerCertVerifyStatus(0)
	, m_backupMyIP(0)
	, m_backupMyPort(0)
	, m_bSupportCompr(false)
{
#ifdef _DEBUG
	PString ss = "cc.yymeeting.com";//"210.83.86.134";//"60.190.38.102";//"218.25.208.10";n
  char out[256]={0};
  char out2[256]={0};
  this->BBQAES_encrypt((const char*)ss, ss.GetLength(), out);
  this->BBQAES_decrypt((const char*)out, strlen(out), out2);

#endif
#ifdef _WIN32	  
   m_addrLocalinterface=PIPSocket::GetRouteAddress(PString(szServerIP));
#else
   m_addrLocalinterface=  PString("0.0.0.0");
#endif

	m_pCurrentClient = this;

	m_nPortNext = m_nPortBase = 10000;
	m_nPortMax = 20000;//65535;

	m_errLastError = Okay;
	m_errLastLoginCode = 0;

	m_bLogin = false;
	m_bHeartbeatOkay = false;
	m_nLastStatusCode = CLIENT_NORMAL;

	m_bTcpLogin = false;
	memset( & m_chanLogin, 0, sizeof(m_chanLogin) );

	m_bUdpBlocked = false;
	memset( & m_chanHeartbeat, 0, sizeof(m_chanHeartbeat) );

	// must init openssl functions
	cert_rsa_aes_init();
	m_pX509_server_cert = NULL;
	m_lpszX509Text = NULL;
	m_pAESKeyBytes = NULL;
	m_nAESKeyBytes = 0;

	m_bForceBBQProxy = false;
	m_bForceTcp = false;

	memset( & m_lpszMyPassword[0], 0, USERDATA_NAME_SIZE );

	m_bBeingLogout = false;

	m_nFirewallType = NoFirewall;
	//m_chanLogin.local.ip = 0;
	//m_chanLogin.local.port = 0;
	//m_chanLogin.peer.ip = 0;
	//m_chanLogin.peer.port = 0;
	m_netWAN.ip = 0;
	m_netWAN.port = 0;

	m_nNetworkBaseDelay = 0;

	m_bServerTested = false;

	m_nHeartbeatCounter = 0;
	m_nHeartbeatTimeInterval = SIM_HEARTBEAT_INTERVAL;
	m_tHeartbeatOkayTime = 0;

	m_nKeepAliveTimeInterval = SIM_HEARTBEAT_INTERVAL;
	m_nAliveCounter = m_nKeepAliveTimeInterval / 2;

	m_nNextChannelId = 1;
	memset( & m_sessionInfo, 0, sizeof(m_sessionInfo) );

	m_nProviderID = 0;
	memset( & m_techInfo, 0, sizeof(m_techInfo) );

	BBQACLFLAG_SET( m_accessFlags, BBQACLFLAG_DEFAULT );

#ifdef CACHE_USERINFO
	m_strCacheIndexFile = "cache.idx";
	m_strCacheDataFile = "cache.dat";
#endif

	{
		// bind a default udp port on being created, to be used as main communication port
		PUDPSocket * pSock = NULL;
		WORD port;


    if( BindDynamicUdpPort( pSock, port ) ) {
			m_chanLogin.local.port = m_nDefaultUdpPort = port;
		}

		// NEW FEATURE: all client use tcp/4320 for direct connection
    ListenTcp( SIM_PORT -1 ,5 ,true, INADDR_ANY, false);
		SetConnectionMax( 64 );
		SetConnectionIdleMax( 300 * 1000 );

		// timeout for Connect is 10s
		//SetConnectTimeout( 100 * 1000 );
	}

	m_dwTimeBase = (DWORD) GetHostUpTimeInMs();
//	m_pTickThread->SetPriority(PThread::HighestPriority);
}

BBQClient::~BBQClient()
{
	BBQProxy * pProxy = BBQProxy::GetCurrentProxy();
	if( pProxy ) {
		pProxy->StopProxy();
	}

	m_pCurrentClient = NULL;

	Close();
}

uint32 BBQClient::StatusStringToInt( const char * str )
{
	if( 0 == strcmp(str, "Online") ) return CLIENT_NORMAL;
	if( 0 == strcmp(str, "Busy") ) return CLIENT_BUSY;
	if( 0 == strcmp(str, "On The Phone") ) return CLIENT_ONTHEPHONE;
	if( 0 == strcmp(str, "Away") ) return CLIENT_AWAY;
	if( 0 == strcmp(str, "Be Right Back") ) return CLIENT_BERIGHTBACK;
	if( 0 == strcmp(str, "Out To Lunch") ) return CLIENT_OUTLUNCH;
	if( 0 == strcmp(str, "Invisible") ) return CLIENT_INVISIBLE;
	if( 0 == strcmp(str, "In Conference") ) return CLIENT_INCONF;
	if( 0 == strcmp(str, "MCU") ) return CLIENT_MCU;
	if( 0 == strcmp(str, "Offline") ) return CLIENT_OFFLINE;

	return CLIENT_NORMAL;
}

const char * BBQClient::StatusIntToString( uint32 n )
{
	switch( n ) {
	case CLIENT_NORMAL: return "Online";
	case CLIENT_BUSY: return "Busy"; 
	case CLIENT_ONTHEPHONE: return "On The Phone";
	case CLIENT_AWAY: return "Away";
	case CLIENT_BERIGHTBACK: return "Be Right Back";
	case CLIENT_OUTLUNCH: return "Out To Lunch";
	case CLIENT_INVISIBLE: return "Invisible";
	case CLIENT_INCONF: return "In Conference";

	case CLIENT_MCU: return "MCU";
	case CLIENT_OFFLINE: return "Offline";
	}

	return "Unknown";
}

PString BBQClient::GetLastErrorString( void ) 
{
	switch( m_errLastError ) {
	case Okay:					return "Okay";
	case InvalidPortRange:		return "InvalidPortRange";
	case InvalidArgument:		return "InvalidArgument";
	case NoServerSpecified:		return "NoServerSpecified";
	case ServerNotFound:		return "ServerNotFound";
	case NetworkError:			return "NetworkError";
	case Timeout:				return "Timeout";
	case IDNotAvailable:		return "IDNotAvailable";
	case InvalidRegKey:			return "InvalidRegKey";
	case InvalidAlias:			return "InvalidAlias";
	case BothStrictFirewall:	return "BothStrictFirewall";
	case LocalOffline:			return "LocalOffline";
	case PeerNotFound:			return "PeerNotFound";
	case PeerOffline:			return "PeerOffline";
	case PeerDenied:			return "PeerDenied";
	case ServiceExpired:		return "ServiceExpired";
	case NetworkChanged:		return "NetworkChanged";
	case ServerError:			return "ServerError";
	case ServerSwitch:			return "ServerSwitch";
	}

	return "Undefined error";
}

void BBQClient::Close()
{
	if( m_bLogin ) Logout();

	OnClose(); // call this to stop all the related working threads

	// notify all the channel pWait thread to abort
	{
		LockIt safe( m_mutexChannelWait );
		cw_iterator iter = m_listChannelWaits.begin(), eiter = m_listChannelWaits.end();
		while( iter != eiter ) {
			BBQChannelWait * pWait = * iter;
			UCMD_CHANNELINFO * info = & pWait->info;
			if( info->init ) {
				pWait->m_bDone = false;
				pWait->Signal();
				iter ++;
			} else {
				delete pWait;
				iter = m_listChannelWaits.erase( iter );
			}
		}
	}

	{
		LockIt safe( m_mutexVars );
		while( m_listFriends.size() > 0 ) {
			ONLINE_USER * pUser = m_listFriends.front();
			m_listFriends.pop_front();
			delete pUser;
		}
	}

	if( m_pX509_server_cert ) {
		PKI_free_cert( m_pX509_server_cert );
		m_pX509_server_cert = NULL;
	}
	if( m_lpszX509Text ) {
		free( m_lpszX509Text );
		m_lpszX509Text = NULL;
	}
	if( m_pAESKeyBytes ) {
		free( m_pAESKeyBytes );
		m_pAESKeyBytes = NULL;
	}

	cert_rsa_aes_cleanup();
}

bool BBQClient::ReBindDefaultPort( void )
{
	// close the current port in use
	PUDPSocket * pSock = NULL;
	if( m_chanLogin.local.port && ( (pSock = FindUdpListener( m_chanLogin.local.port )) != NULL ) ) {
		if( DetachUdpListener( pSock ) ) delete pSock;
		m_chanLogin.local.port = 0;
	}

	// bind a new port
	WORD port;
	if( BindDynamicUdpPort( pSock, port ) ) {
		// get local ip:port to fill into the request
		m_chanHeartbeat.local.port = m_chanLogin.local.port = m_nDefaultUdpPort = port;

		// immediately update status, if client is currently login to server
		if( m_bLogin ) SendHeartbeat();
	} else {
		m_errLastError = InvalidPortRange;
		return false;
	}

	return true;
}

uint32 BBQClient::GetNetworkBaseDelay( void )
{
	return m_nNetworkBaseDelay;
}

uint32 BBQClient::SetNetworkBaseDelay( uint32 nDelayMs )
{
	uint32 old = m_nNetworkBaseDelay;

	m_nNetworkBaseDelay = nDelayMs;

	return old;
}

bool BBQClient::SetPortRange( WORD nPortBase, WORD nPortMax ) 
{
	m_nPortBase = min( nPortBase, nPortMax );
	m_nPortMax = max( nPortBase, nPortMax );

	if( m_nPortNext < m_nPortBase || m_nPortNext >= m_nPortMax) {
		m_nPortNext = m_nPortBase;
	}

	if( m_chanLogin.local.port ) {
		if( m_chanLogin.local.port < m_nPortBase || m_chanLogin.local.port >= m_nPortMax ) {
			return ReBindDefaultPort();
		}
	}

	return true;
}

bool BBQClient::BindDynamicUdpPort( PUDPSocket* & pSock, WORD & port, const PIPSocket::Address & bind )
{
	pSock = new PUDPSocket;
	
	if( m_nPortNext < m_nPortBase || m_nPortNext >= m_nPortMax)
		m_nPortNext = m_nPortBase;

	for( port = m_nPortNext; port < m_nPortMax; port ++ ) {
		if( pSock->Listen(m_addrLocalinterface, 0, port, PSocket::AddressIsExclusive) ) {
      pSock->Close();
		  if( pSock->Listen(bind, 0, port, PSocket::AddressIsExclusive) ) {
			  DWORD ip = (DWORD) bind;
			  PString strIp = ip ? ToString(ip) : "*";
			  PString strMsg( PString::Printf, "bind UDP port: %s:%d.", (const char *)strIp, port );
			  PLOG( m_pLogger, Debug, strMsg );

			  AttachUdpListener( pSock );
			  m_nPortNext = port +1;
			  return true;
		  } else {
  //#if defined(_WIN32) && !defined(_WIN32_WCE)
  //			int nError = GetLastError();
  //			if(nError>=PROXYBASEERR && nError<=PROXYLASTERR) {
  //				delete pSock;
  //				pSock = NULL;
  //				return false;
  //			}
  //#endif
		  }
    }
	}

	for( port = m_nPortBase; port < m_nPortNext; port ++ ) {
		if( pSock->Listen(bind, 0, port, PSocket::AddressIsExclusive) ) {
			AttachUdpListener( pSock );
			m_nPortNext = port +1;
			return true;
		} else {
//#if defined(_WIN32) && !defined(_WIN32_WCE)
//			int nError = GetLastError();
//			if(nError>=PROXYBASEERR && nError<=PROXYLASTERR) {
//				delete pSock;
//				pSock = NULL;
//				return false;
//			}
//#endif
		}
	}

	delete pSock;
	pSock = NULL;
	return false;
}

bool BBQClient::TestServer( PIPSocket::Address svraddr, WORD svrport )
{
  return true;
	PIPSocket::Address local_addr;
	PIPSocket::Address wan_addr;
	FirewallType firewall;

	if( TestServer( local_addr, wan_addr, firewall, svraddr, svrport ) ) {
		m_chanLogin.local.ip = (DWORD) local_addr;
		m_netWAN.ip = (DWORD) wan_addr;
		m_nFirewallType = firewall;

		return true;
	}

	return false;
}
bool BBQClient::GetShowerOrder(PStringArray& arr ,unsigned int roomid)
{
	bool bDone = false;

	SIM_REQUEST * reply = NULL;
	{
		SIM_REQUEST req;
	  SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_MEETCTRL, sizeof(SIMD_CS_MEETCTRL) );
	  SIMD_CS_MEETCTRL * p = (SIMD_CS_MEETCTRL *) & req.msg.simData;
    memset(p, 0, sizeof(SIMD_CS_MEETCTRL));
    //strncpy(p->key, strInfoKey, sizeof(p->key));
    p->cmd=1002;
    sprintf(p->info, "%u;%u", roomid,roomid);
		reply = RequestMessage( & req, SIM_SC_MEETCTRL, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2);
	}

	//bool bDone = false;
  char pData= NULL ;
	if( reply ) {
    SIMD_SC_MEETCTRL * pA = (SIMD_SC_MEETCTRL *) & reply->msg.simData;
    arr = PString (pA->info).Tokenise(",");
    ReleaseMessage( reply );
  }

	return bDone;
}
bool BBQClient::BBQPing( PIPSocket::Address svraddr, WORD svrport )
{
	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINIT( req, 0, 0, m_nDefaultUdpPort, (DWORD) svraddr, svrport, SIM_CS_HELLO, sizeof(SIMD_CS_HELLO) );
	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_HELLO, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		ReleaseMessage( reply );
		bDone = true;
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::BBQPing( 
						PIPSocket::Address & myIp, WORD & myPort, 
						PIPSocket::Address svraddr, WORD svrport )
{
	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINIT( req, 0, 0, m_nDefaultUdpPort, (DWORD) svraddr, svrport, SIM_CS_HELLO, sizeof(SIMD_CS_HELLO) );
	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_HELLO, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_HELLO * pA = (SIMD_SC_HELLO *) & reply->msg.simData;
		myIp = (DWORD) pA->clientIp;
		myPort = pA->clientPort;
		ReleaseMessage( reply );
		bDone = true;
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::BBQPing( 
						PIPSocket::Address & anotherServerIp, WORD & anotherServerPort, 
						PIPSocket::Address & myIp, WORD & myPort, 
						PIPSocket::Address svraddr, WORD svrport )
{
	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINIT( req, 0, 0, m_nDefaultUdpPort, (DWORD) svraddr, svrport, SIM_CS_HELLO, sizeof(SIMD_CS_HELLO) );
	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_HELLO, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_HELLO * pA = (SIMD_SC_HELLO *) & reply->msg.simData;
		myIp = (DWORD) pA->clientIp;
		myPort = pA->clientPort;
		anotherServerIp = (DWORD) pA->anotherServerIp;
		anotherServerPort = pA->anotherServerPort;
		ReleaseMessage( reply );
		bDone = true;
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}



bool BBQClient::TestServer( PIPSocket::Address & local_addr, PIPSocket::Address & wan_addr, FirewallType & firewall,
						   PIPSocket::Address svraddr, WORD svrport )
{
	WORD wan_port[3] = {0,0,0};

	PIPSocket::Address target_addr = svraddr;
	WORD target_port = svrport;

	GetHostAddress( local_addr );

	wan_addr = 0;

	bool bFailed = false;

	// send a hello message, and get my wan ip:port info from server response
	SIM_REQUEST req;
//#define D_ONLY_VIDEO
#ifdef  D_ONLY_VIDEO
  return true;
#endif
	// test two ports
	for( WORD i=0; i<2; /**/ ) {
		SIM_REQINIT( req, 0, 0, m_nDefaultUdpPort, (DWORD) target_addr, target_port, SIM_CS_HELLO, sizeof(SIMD_CS_HELLO) );
		SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_HELLO, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
		if( reply ) {
			SIMD_SC_HELLO * pA = (SIMD_SC_HELLO *) & reply->msg.simData;

			PString strMsg( PString::Printf, "HELLO response: my WAN: %s:%d, Another server: %s:%d",
				(const char *) ToString( pA->clientIp ), (unsigned short)pA->clientPort, 
				(const char *) ToString( pA->anotherServerIp ), (unsigned short)pA->anotherServerPort );
			PLOG( m_pLogger, Debug, strMsg );
			//PTRACE( 1, strMsg );
			if( wan_addr == 0 ) 
				wan_addr = pA->clientIp;
			wan_port[i] = pA->clientPort;

			if( pA->anotherServerIp != 0 ) { // because some strict firewall need 2 remote hosts to check out
				target_addr = pA->anotherServerIp;
				target_port = pA->anotherServerPort;
				target_port ++;				// we use the second port, cause some server set another server to itself.
			} else {
				target_port ++;
			}

			ReleaseMessage( reply );

			i++;
			if( i > 1 ) break; // already done 2 tests successfully, it's okay.

		} else { // timeout and no reply

			if( (i > 0) && (target_addr != svraddr) ) {
				// if second test failed on the second server, we need to finish the second test on the first server.
				target_addr = svraddr;
				target_port = svrport +1;

			} else {
				m_errLastError = Timeout;
				bFailed = true;
				break;
			}
		}
	}

	if( bFailed ) return false;

	if( wan_port[0] != wan_port[1] ) {
		firewall = StrictPolicy;

	} else {
		SIM_REQINIT( req, 0, 0, m_nDefaultUdpPort, (DWORD) svraddr, svrport, SIM_CS_TESTFIREWALL, sizeof(SIMD_CS_HELLO) );
		SIM_NEXTSYNC( &req );

		SIM_REQUEST filter;
		SIM_REPLY_REQUEST( & filter, SIM_SC_TESTFIREWALL, 0, & req );
		filter.channel.peer.ip = 0; // any ip is okay
		filter.channel.peer.port = 0; // any port is okay

		// put into pWait list before send, cause sometimes the response is very fast
		MsgWait pWait( *this, & filter );

		SIM_REQUEST * reply = NULL;
		bool bWaited = true;
		int nTry = 3;

		do {
			//SendMessageInternal( & req );
			SendMessage(&req);
			bWaited = pWait.WaitRequest( reply, 1000 + m_nNetworkBaseDelay * 2 );
			nTry --;
		} while ( (! bWaited) && (nTry>0) );

#ifdef PTRACING
		if( reply ) {
			PString str( PString::Printf, "msg requested okay: %s, sync: %d, %d bytes,\n\t%s",
				SIM_MSGNAME( reply->msg.simHeader.id ), 
				reply->msg.simHeader.sync, 
				SIM_MSGSIZE(reply->msg),
				(const char *) ToString( & reply->channel ) );
			PTRACE( 1, str );
		} else {
			PTRACE( 1, (bWaited ? "msg request waited." : "msg request failed.") );
		}
#endif

		if( reply ) {
			SIMD_SC_HELLO * pA = (SIMD_SC_HELLO *) & reply->msg.simData;
			wan_port[2] = pA->clientPort;
			ReleaseMessage( reply );

			if( local_addr == wan_addr ) 
				firewall = NoFirewall;		// direct connection without any firewall
			else
				firewall = WeakPolicy;		// weak NAT

		} else {
			if( local_addr == wan_addr ) 
				firewall = MediumPolicy;	// direct connection, but with net adaptor interface firewall
			else
				firewall = MediumPolicy;	// NAT firewall
		}
	}

	PString str( PString::Printf, 
		"Test server done. Local ip: %s, WAN ip: %s, Firewall: %d",
		IP2STRING( local_addr ), 
		IP2STRING( wan_addr ),
		firewall );
	PLOG( m_pLogger, Debug, str );

	return true;
}

bool BBQClient::SetServerAddress( const PString & svraddr, WORD svrport, bool bTcp )
{
	if( m_bTcpLogin != bTcp ) {

		if( m_bTcpLogin ) {
			Logout();
		}

		m_bTcpLogin = bTcp;
		
		m_bServerTested = false;
	}

	if( bTcp ) {
		// we need not resolve the hostname to IP, cause sometimes we use proxy, proxy will do it.
		if( (m_strServerHostname != svraddr) || (m_chanLogin.peer.port != svrport) ) {
			//if( m_bLogin ) 
			Logout();
			m_bServerTested = false;
		}

	} else {
		DWORD svrIp = HostnameToIp( svraddr );

		if( (m_chanLogin.peer.ip != svrIp) || (m_chanLogin.peer.port != svrport) ) {
			//if( m_bLogin ) 
			Logout();
			m_bServerTested = false;
		}

		m_chanLogin.peer.ip = svrIp;
	}

	m_strServerHostname = svraddr;
	m_chanLogin.peer.port = svrport;

	m_bUdpBlocked = false;

	return true;
}

bool BBQClient::SetServerAddress( DWORD svraddr, WORD svrport, bool bTest )
{
	// set the hostname to ip
	m_strServerHostname = ToString( svraddr );

	if( (m_chanLogin.peer.ip != svraddr) || (m_chanLogin.peer.port != svrport) ) {
		//if( m_bLogin ) 
		Logout();
		m_bServerTested = false;
	}

	m_chanLogin.peer.ip = (DWORD) svraddr;
	m_chanLogin.peer.port = svrport;

	m_bUdpBlocked = false;

	if( bTest ) {
		PIPSocket::Address local_addr;
		PIPSocket::Address wan_addr;
		FirewallType firewall;
		if( TestServer( local_addr, wan_addr, firewall, svraddr, svrport ) ) {
			m_chanLogin.local.ip = (DWORD) local_addr;
			m_netWAN.ip = (DWORD) wan_addr;
			m_nFirewallType = firewall;

			m_bServerTested = true;
		} else {
			return false;
		}
	}

	return true;
}

bool BBQClient::QueryVersionInfo( SIMD_VERSIONINFO * pVersionInfo, PString & strDownloadURL )
{
	// check the connection before sending data, if not available, then reconnect
	if( m_bTcpLogin ) {
		if(! ValidateTcpConnection()) return false;
	}

	SIM_REQUEST * reply = NULL;
	{
		SIM_REQUEST req;
		//SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_VERSIONINFO, 0 );
		SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_VERSIONINFO, 0 );
		reply = RequestMessage( & req, SIM_SC_VERSIONINFO, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2);
	}

	bool bDone = false;

	if( reply ) {
		try {
			char * pURL = NULL;
			PBytePack re( & reply->msg.simData, reply->msg.simHeader.size );
			re >> (* pVersionInfo) >> pURL;
			if( pURL ) {
				strDownloadURL = pURL;
				delete[] pURL;
			}
			bDone = true;
		} catch ( const char * ) {
			// bad response data
			m_errLastError = NetworkError;
		}

		ReleaseMessage( reply );

	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::QueryStringToStringInfo(const char* strInfoKey, PString & strKeysandValues )
{
	// check the connection before sending data, if not available, then reconnect
	if( m_bTcpLogin ) {
		if(! ValidateTcpConnection()) return false;
	}

  if (!strInfoKey) return false;
	SIM_REQUEST * reply = NULL;
	{
		SIM_REQUEST req;
	  SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_STRINGS, sizeof(SIMD_CS_STRINGS) );
	  SIMD_CS_STRINGS * p = (SIMD_CS_STRINGS *) & req.msg.simData;
    memset(p, 0, sizeof(SIMD_CS_STRINGS));
    strncpy(p->key, strInfoKey, sizeof(p->key));

		reply = RequestMessage( & req, SIM_SC_STRINGS, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2);
	}

	bool bDone = false;
  char pData= NULL ;
	if( reply ) {
		try {
      SIMD_CS_STRINGS data;
			char * pData = NULL;
			PBytePack re( & reply->msg.simData, reply->msg.simHeader.size );
			re >> data >>pData;
      if(   pData ) {
				strKeysandValues = pData;
				delete[] pData;
			}
			bDone = true;
		} catch ( const char * ) {
			// bad response data
			m_errLastError = NetworkError;
		}

		ReleaseMessage( reply );

	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}


bool BBQClient::GetFirewallInfo( PIPSocket::Address & local_addr, PIPSocket::Address & wan_addr, FirewallType & firewall )
{
	if( m_bTcpLogin ) {
		if( ! m_bLogin ) return false;
	} else {
		if( ! m_bServerTested ) {
			if( TestServer( m_chanLogin.peer.ip, m_chanLogin.peer.port ) ) {
				m_bServerTested = true;
			} else {
				m_errLastError = ServerNotFound;
				return false;
			}
		}
	}

	GetHostAddress( local_addr );
	wan_addr = m_netWAN.ip;
	firewall = m_nFirewallType;

	return true;
}

void BBQClient::Logout( void )
{
	LockIt safe( m_mutexChannelWait );
	if( ! m_bLogin ) {
		if( m_chanLogin.socket ) {
			CloseMsgConnection( 0,0, m_chanLogin.socket );

			//BBQMsgConnection * pConn = FindMsgConnection( 0, 0, m_chanLogin.socket );
			//if( pConn ) {
			//	PTCPSocket * pSock = pConn->GetSocket();
			//	if( pSock ) pSock->Close();
			//} else {
			//	closesocket( m_chanLogin.socket );
			//}
			m_chanLogin.socket = 0;
		}

		m_bHeartbeatOkay = false;

		return;
	}

	m_bBeingLogout = true;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_LOGOUT, sizeof(SIMD_CS_LOGOUT) );
	SIMD_CS_LOGOUT * p = (SIMD_CS_LOGOUT *) & req.msg.simData;
	p->sessionInfo = m_sessionInfo;

	if( m_chanLogin.socket ) {
		if( m_bHeartbeatOkay ) SendMessage( & req, false );

		CloseMsgConnection( 0,0, m_chanLogin.socket );

		//BBQMsgConnection * pConn = FindMsgConnection( 0,0, m_chanLogin.socket );
		//if( pConn ) {
		//	//if( m_bHeartbeatOkay ) SendMessage( & req, true, 3000, 1 );
		//	if( m_bHeartbeatOkay ) SendMessage( & req, false );

		//	// close the channel
		//	PTCPSocket * pSock = pConn->GetSocket();
		//	if( pSock ) pSock->Close();
		//} else {
		//	closesocket( m_chanLogin.socket );
		//}

		closesocket( m_chanLogin.socket );
		m_chanLogin.socket = 0;

		PLOG( m_pLogger, Debug,  "Logout, Tcp mode, connection closed." );
		//PTRACE( 1, "Logout, Tcp mode, connection closed." );
	} else {
		//if( m_bHeartbeatOkay ) SendMessage( & req, true );
		if( m_bHeartbeatOkay ) SendMessage( & req, false );

		PLOG( m_pLogger, Debug, "Logout. Udp mode." );
		//PTRACE( 1, "Logout. Udp mode." );
	}

	// break all the message waiting thread
	BreakMessageWaiting();

	m_chanLogin.socket = 0;
	m_bLogin = false;
	m_nServerId = 0;
	m_bHeartbeatOkay = false;
	m_sessionInfo.cookie = 0;
	BBQACLFLAG_SET( m_accessFlags, BBQACLFLAG_DEFAULT );

	m_bBeingLogout = false;

	// clear AES key
	if( m_pAESKeyBytes ) {
		free( m_pAESKeyBytes );
		m_pAESKeyBytes = NULL;
	}
	m_nAESKeyBytes = 0;
#ifdef _WIN32
	// clear server PKI cert
	if( m_pX509_server_cert ) {
		PKI_free_cert( m_pX509_server_cert );
		m_pX509_server_cert = NULL;
	}
	if( m_lpszX509Text ) {
		free( m_lpszX509Text );
		m_lpszX509Text = NULL;
	}
#endif
	RemoveAllDomainServers();
}

bool BBQClient::ChangeStatus( uint32 statusCode, const char * lpszString )
{
	if( ! m_bLogin ) return false;

	m_nLastStatusCode = statusCode;

	// NOTICE!!! never call ValidateTcpChannel(), cause it will call this function

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_CHANGESTATUS, sizeof(SIMD_CS_CHANGESTATUS) );
	SIMD_CS_CHANGESTATUS * p = (SIMD_CS_CHANGESTATUS *) & req.msg.simData;
	p->sessionInfo = m_sessionInfo;
	p->statusCode = statusCode;
	strncpy( p->statusString, (lpszString ? lpszString : ""), sizeof(p->statusString)-1 );

	PostMessage( & req, true );
	// return SendMessage( & req, true );
	return true;
}

void BBQClient::SetTcpLogin( bool bTcp )
{
	if( m_bTcpLogin != bTcp ) {

		if( m_bTcpLogin ) {
			Logout();
		}

		m_bTcpLogin = bTcp;
		
		m_bServerTested = false;
	}

	if( ! m_bTcpLogin ) {
		if( m_chanLogin.peer.ip == 0 && (! m_strServerHostname.IsEmpty()) ) {
			m_chanLogin.peer.ip = HostnameToIp( m_strServerHostname );
		}
	}
}

bool BBQClient::IsTcpLogin( void )
{
	return m_bTcpLogin;
}

bool BBQClient::ValidateChannel( SIM_CHANNEL & chan )
{
	if( chan.socket ) {	 // check current TCP connection
		if( FindMsgConnection( 0, 0, chan.socket ) ) {
			fd_set fdsetRead;
			FD_ZERO( & fdsetRead );
			FD_SET( chan.socket, & fdsetRead );
			struct timeval tval = { 0, 0 };
			int s = select( chan.socket +1, & fdsetRead, NULL, NULL, &tval);

			if( s >= 0 ) return true;

		}
		// else reconnect

		int fd = Connect( (PIPSocket::Address)chan.peer.ip, chan.peer.port );
		if( fd != 0 ) {
			chan.socket = fd;

			return true;
		}
	} else { 
		// check UDP socket for sending
		return true;
	}
	
	m_errLastError = NetworkError;

	return false;
}

bool BBQClient::ValidateTcpConnection( void )
{
	bool bConnected = false;
	bool bReconnected = false;

	// check current connection
	if( m_chanLogin.socket && FindMsgConnection( 0,0,m_chanLogin.socket ) ) {

		fd_set fdsetRead;
		FD_ZERO( & fdsetRead );
		FD_SET( m_chanLogin.socket, & fdsetRead );
		struct timeval tval = { 0, 0 };
		int s = select( m_chanLogin.socket +1, & fdsetRead, NULL, NULL, &tval);

		if( s >= 0 ) bConnected = true;
	}
	
	if( ! bConnected ) { // else reconnect
		bReconnected = bConnected = ( (m_chanLogin.socket = Connect( m_strServerHostname, m_chanLogin.peer.port )) != 0 );
	}

	if( bConnected ) {
		BBQMsgConnection * pConn = FindMsgConnection( 0, 0, m_chanLogin.socket );
		if( pConn ) {
			PTCPSocket * pSock = pConn->GetSocket();
			if( pSock ) {
				PIPSocket::Address addr; WORD port;
				pSock->GetPeerAddress( addr, port );
				m_chanLogin.peer.ip = (DWORD) addr;
				m_chanLogin.peer.port = port;

				if( bReconnected && m_bLogin ) ChangeStatus( m_nLastStatusCode, "" );

				return true;
			}
		}
	}
    
	m_errLastError = NetworkError;
	return false;
}

bool BBQClient::Login( uint32 nSerialID, const char * strKey, uint32 & uid, uint32 nServiceType, uint32 nStatus )
{
	bool bDone = false;

	BBQACLFLAG_SET( m_accessFlags, BBQACLFLAG_DEFAULT );
	SIM_REQUEST req;

	if( m_bLogin ) Logout();

	// check the connection before sending data, if not available, then reconnect
	if( m_bTcpLogin ) {
		if(! ValidateTcpConnection()) return false;
	}

	if( m_bTcpLogin ) {
		if( ! m_bServerTested ) {
			// because UDP blocked, NAT traversal cannot be done, 
			// instead, a TCP channel will be setup with TCP proxy, server test is NOT required
			SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_HELLO, sizeof(SIMD_CS_HELLO) );
			SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_HELLO, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
			if( reply ) {
				SIMD_SC_HELLO * pA = (SIMD_SC_HELLO *) & reply->msg.simData;

				PIPSocket::Address local_addr, wan_addr;
				GetHostAddress( local_addr );
				wan_addr = pA->clientIp;

				m_chanLogin.local.ip = (DWORD) local_addr;
				m_netWAN.ip = (DWORD) wan_addr;
				m_nFirewallType = StrictPolicy;

				ReleaseMessage( reply );
			}
		}
	} else {
		if( ! m_bServerTested ) {
			if( TestServer( m_chanLogin.peer.ip, m_chanLogin.peer.port ) ) {
				m_bServerTested = true;
			} else {
				m_errLastError = ServerNotFound;
				return false;
			}
		}
	}

	m_techInfo.interface_ip = m_chanLogin.local.ip;
	m_techInfo.wan = (m_chanLogin.local.ip == m_netWAN.ip) ? 1 : 0;
	m_techInfo.firewall = (int) m_nFirewallType;

	SIMD_CS_LOGIN * pQ = (SIMD_CS_LOGIN *) & req.msg.simData;
	if( strKey && (strlen(strKey) < SIM_KEYSIZEMAX) ) {
		pQ->serviceType = nServiceType;
		pQ->favouriteID = uid;
		pQ->serialID = nSerialID;
		strcpy( pQ->keyPassword, strKey );
		pQ->statusCode = nStatus;
		pQ->providerID = m_nProviderID;
		pQ->techParam = m_techInfo;
	} else {
		m_errLastError = InvalidRegKey;
		return false;
	}

	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_LOGIN, sizeof(SIMD_CS_LOGIN) );
	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_LOGIN, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_LOGIN * pA = (SIMD_SC_LOGIN *) & reply->msg.simData;
		switch( pA->statusCode ) {
		case SIMS_OK:
			m_sessionInfo = pA->sessionInfo;
			m_bLogin = true;

			m_regInfo = * pQ;
			m_regInfo.favouriteID = m_sessionInfo.id;
			
			// send a heartbeat immediately after login
			// SendHeartbeat(); 

			uid = m_sessionInfo.id;
			m_accessFlags = pA->accessFlags;
			bDone = true;

#ifdef CACHE_USERINFO
			// load cache index
			{
				FILE * fp = NULL;
				if( (fp = fopen( m_strCacheIndexFile, "rb")) != NULL ) {
					m_userlistCache << fp;
					fclose( fp );
				}
			}
#endif

			break;
		case SIMS_SERVICEEXPIRED:
			m_errLastError = ServiceExpired;
			break;
		case SIMS_NOTAVAILABLE:
			m_errLastError = IDNotAvailable;
			break;
		case SIMS_BADREQUEST:
		case SIMS_DENY:
			m_errLastError = InvalidRegKey;
			break;
		}

		ReleaseMessage( reply );
	} else {
		m_errLastError = Timeout;
	}
	
	return bDone;
}

bool BBQClient::OnChannelRequesting( uint32 uid, int nAppCode, const char * lpszAppString, uint32 & nAppParam )
{
	// nAppParam 
	// incoming data is peer's, change it to my param

	return AllowChannelRequest( uid, nAppCode, lpszAppString );
}

bool BBQClient::AllowChannelRequest( uint32 uid, int nAppCode, const char * lpszAppString ) 
{
	return true; 
}

void BBQClient::OnRequestedChannelSetup( BBQ_CHANNEL * pChannel, int nAppCode, const char * lpszAppString ) 
{
	// keep connection alive for a while, so that peer can retrieve valid socket info
	if( pChannel->tcpSock ) sleep_ms(500);

	ReleaseChannel( pChannel );
}
void BBQClient::OnRequestedChannelSetupForTestingProxy( BBQ_CHANNEL * pChannel, int nAppCode, const char * lpszAppString ) 
{
	// keep connection alive for a while, so that peer can retrieve valid socket info
	if( pChannel->tcpSock ) sleep_ms(500);

	ReleaseChannel( pChannel );
}
void BBQClient::OnTick( void )
{
	BBQMsgTerminal::OnTick();

	// send heartbeat to server, to keep this connection online
	m_nAliveCounter ++;
  if( m_nAliveCounter >= m_nKeepAliveTimeInterval /*&& m_nLastStatusCode!= CLIENT_MCU*/) {
		m_nAliveCounter = 0;

		KeepAlive();
	}

	// send heartbeat to server, to keep this connection online
	m_nHeartbeatCounter ++;
  static int ncCount = 10;
	if( m_nHeartbeatCounter >= m_nHeartbeatTimeInterval/* && (m_nLastStatusCode!= CLIENT_MCU || ncCount-->0)*/) {
		m_nHeartbeatCounter = 0;

		SendHeartbeat();
	}

	// clean requesting channel waited if timeout
	{
		LockIt safe( m_mutexChannelWait ); 
		MS_TIME msCmp = GetHostUpTimeInMs() - (CHANNEL_SETUP_TIME_MAX * 30); // 5 min ago
		for( cw_iterator iter = m_listChannelWaits.begin(), eiter = m_listChannelWaits.end(); iter != eiter; /**/ ) {
			BBQChannelWait * pWait = * iter;
			if( pWait->m_msWaitBeginTime < msCmp ) {
				if( pWait->info.init ) {
					m_errLastError = Timeout;
					pWait->Signal();
				} else {
					PLOG( m_pLogger, Debug, "Removing unused channel in requesting state." );
					//PTRACE( 1, "Removing unused channel in requesting state." );
					delete pWait;
					//iter = m_listChannelWaits.erase( iter );
					STL_ERASE( m_listChannelWaits, cw_iterator, iter );
					continue;
				}
			}
			iter ++;
		}
	}

	// clear client-server channel if not used for a time
	{
		LockIt safe( m_mutexChannelWait );
		MS_TIME msCmp = GetHostUpTimeInMs() - (CHANNEL_SETUP_TIME_MAX * 30); // idle for 5 minute
		for( IPCHANMAP::iterator it = m_mapChanOtherServers.begin(), et = m_mapChanOtherServers.end(); it != et; /**/ ) {
			if( it->second.timestamp > msCmp ) {
				it ++;
			} else {
				SIM_CHANNEL chan = it->second.channel;
				//it = m_mapChanOtherServers.erase( it );
				STL_ERASE( m_mapChanOtherServers, IPCHANMAP::iterator, it );
				if( (chan.socket == 0) && (m_chanLogin.socket == 0) && (chan.local.port == m_chanLogin.local.port) ) {
					// skip, cause the UDP port is default one
				} else {
					EraseChannel( chan );
				}
			}
		}

	}

#ifdef ANTI_PACKET_LOSS
	// clear channel request history
	{
		LockIt sf( m_mutexCidTime );
		MS_TIME msCmp = GetHostUpTimeInMs() - 60000; // 1 min ago
		for( UNIQUE_CID_TIME_MAP::iterator it = m_mapCidTime.begin(), eit = m_mapCidTime.end(); it != eit; /**/ ) {
			if( it->second < msCmp ) {
				//it = m_mapCidTime.erase( it );
				STL_ERASE( m_mapCidTime, UNIQUE_CID_TIME_MAP::iterator, it );
			} else {
				it ++;
			}
		}
	}
#endif
}

uint32 BBQClient::GetFriendStatus( uint32 uid )
{
	LockIt safe( m_mutexVars );
	for( of_iterator iter = m_listFriends.begin(), eiter = m_listFriends.end(); iter != eiter; iter ++ ) {
		ONLINE_USER * pUser = * iter;

		if( pUser->id == uid ) {
			return pUser->status;
		}
	}

	return CLIENT_OFFLINE;
}

bool BBQClient::GetLocalProxyInfo( SIMD_CS_HEARTBEAT * pQ )
{
	// report dynamic status
	BBQProxy * pProxy = BBQProxy::GetCurrentProxy();
	if( pProxy ) {
		BBQProxy::Config config;		pProxy->GetConfig( config );
		BBQProxy::Status status;		pProxy->GetStatus( status );

		pQ->proxyMax = config.nChannelMax;
		pQ->proxySlot = config.nChannelMax - status.nEntries/2;

		pQ->proxyBandwidthTotal = config.nBandwidthMax;
		pQ->proxyBandwidthUsed = status.nAverageBandwidth;

		pQ->proxyInfo.adminPort = config.nAdminPort;
		pQ->proxyInfo.tcpPort = config.nTcpPort;
		pQ->proxyInfo.udpPortMin = config.nUdpPortMin;
		pQ->proxyInfo.udpPortMax = config.nUdpPortMax;

		pQ->proxyInfo.serviceIp = config.dwExternalIp;
	}

	return true;
}

bool BBQClient::GetLocalMcuInfo( SIMD_CS_HEARTBEAT * pQ )
{
	return false;
}

bool BBQClient::KeepAlive( void )
{
	if( ! m_bLogin ) return false;

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	// by default, we use login channel to send heartbeat
	SIM_CHANNEL chanHeartbeat = m_chanLogin;
	bool chLogin = false;
	// but, we might use the UDP channel especially for heartbeat
	if( m_bTcpLogin && (! m_bUdpBlocked) ) {
		chLogin = true;
		chanHeartbeat = m_chanHeartbeat;
	}

	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, chanHeartbeat, SIM_CS_HELLO, sizeof(SIMD_CS_HELLO) );
	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_HELLO, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_HELLO * pA = (SIMD_SC_HELLO *) & reply->msg.simData;
		DWORD myIp = (DWORD) pA->clientIp;
		uint16 myPort = pA->clientPort;
		if( m_backupMyIP != myIp ) {
			PTRACE(1, "My IP Address changed:" << myIp << ":" << myPort);
			m_nKeepAliveTimeInterval = SIM_HEARTBEAT_INTERVAL;
			m_backupMyIP = myIp;
			m_backupMyPort = myPort;
		} else if( m_backupMyPort == 0 ) {
			m_backupMyPort = myPort;
		} else if( m_backupMyPort != myPort ) {
			PTRACE(1, "My Port changed:" << myIp << ":" << myPort);
			m_backupMyPort = myPort;
			if( m_nKeepAliveTimeInterval >= 60 ) {
				m_nKeepAliveTimeInterval = 45;
			} else if( m_nKeepAliveTimeInterval >= 45 ) {
				m_nKeepAliveTimeInterval = 30;
			} else if( m_nKeepAliveTimeInterval >= 30 ) {
				m_nKeepAliveTimeInterval = 15;
			} else if( m_nKeepAliveTimeInterval >= 15 ) {
				m_nKeepAliveTimeInterval = 10;
			} else {
				m_nKeepAliveTimeInterval = 5;
			}
		}
		ReleaseMessage( reply );
		bDone = true;
	} else {
		PTRACE(1, "KeepAlive: Timeout: " << ToString(&m_chanLogin));
		m_errLastError = Timeout;
	}

	if( chLogin ) {
		SIM_CHANNEL chanHeartbeat = m_chanLogin;

		SIM_REQUEST req;
		SIM_REQINITCHAN( req, chanHeartbeat, SIM_CS_HELLO, sizeof(SIMD_CS_HELLO) );
		SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_HELLO, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
		if( reply ) {
		}
		ReleaseMessage( reply );
	}

	return bDone;
}

bool BBQClient::SendHeartbeat( void )
{
	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		m_bHeartbeatOkay = false;
		return false;
	}

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) {
			OnHeartbeatTimeout();
			m_bHeartbeatOkay = false;
			return false;
		}
	}

	DWORD ipSwitch = 0;
	WORD portSwitch = SIM_PORT;

	// by default, we use login channel to send heartbeat
	SIM_CHANNEL chanHeartbeat = m_chanLogin;

	// but, we might use the UDP channel especially for heartbeat
	if( m_bTcpLogin && (! m_bUdpBlocked) ) chanHeartbeat = m_chanHeartbeat;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, chanHeartbeat, SIM_CS_HEARTBEAT, sizeof(SIMD_CS_HEARTBEAT) );
	SIMD_CS_HEARTBEAT * pQ = (SIMD_CS_HEARTBEAT *) & req.msg.simData;
	memset( pQ, 0, sizeof(SIMD_CS_HEARTBEAT) );
	pQ->sessionInfo = m_sessionInfo;

	PIPSocket::Address local_addr;
	GetHostAddress( local_addr );
	pQ->addrLAN.ip = local_addr;
	pQ->addrLAN.port = m_nDefaultUdpPort;

	GetLocalProxyInfo( pQ );

	GetLocalMcuInfo( pQ );

	bool bDone = false;
	bool bFailLogout = true;
	bool bSessionExpired = true;
	//add by chenyuan begin
	if (m_arConferenceStatus.GetSize()>0)
	{
		PBytePack data;
		LockIt lock(m_mutexConferenceStatus);
		for(int i=0;i< m_arConferenceStatus.GetSize();i++)
		{
			const char * p = (const char*)m_arConferenceStatus[i];
			data << p;
		}
		if (data.Size() < SIM_DATABLOCKSIZEMAX - sizeof(req.msg.simHeader) - req.msg.simHeader.size)
		{
			//enable the opt
			uint32 nCount = m_arConferenceStatus.GetSize();
			pQ->vmeetInfo.hasConferenceStatusTag = 1;
			req.msg.simHeader.size += (data.Size()+sizeof(nCount));
			memcpy(&req.msg.simData[sizeof(SIMD_CS_HEARTBEAT) ], &nCount,  sizeof(nCount));
			memcpy(&req.msg.simData[sizeof(SIMD_CS_HEARTBEAT)+sizeof(nCount) ], data.Data(), data.Size());
		}
	}
	//add by chenyuan end
	DWORD timeout = 6000;
	uint32 statusCode = 0, nOnlineFriends = 0;
	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_HEARTBEAT, timeout + m_nNetworkBaseDelay * 2, 10 );
	if( reply ) {
		try {
			PBytePack re( & reply->msg.simData, reply->msg.simHeader.size );
			re >> statusCode;
			switch( statusCode ) {
			case SIMS_OK:
				{
					{
						uint32 nID, status;

						LockIt safe( m_mutexVars );
						for( of_iterator iter = m_listFriends.begin(), eiter = m_listFriends.end(); iter != eiter; iter ++ ) {
							ONLINE_USER * pUser = * iter;
							pUser->status = CLIENT_OFFLINE;
						}

						re >> nOnlineFriends;
						for( uint32 i=0; i<nOnlineFriends; i++ ) {
							re >> nID >> status;
							bool bFind = false;
							for( of_iterator iter = m_listFriends.begin(), eiter = m_listFriends.end(); iter != eiter; iter ++ ) {
								ONLINE_USER * pUser = * iter;
								if( pUser->id == nID ) {
									pUser->status = status;
									bFind = true;//break;
								}
							}
	//						if( iter == eiter ) {
							if( ! bFind ) {
								ONLINE_USER * pUser = new ONLINE_USER;
								pUser->id = nID;
								pUser->status = status;
								m_listFriends.push_back( pUser );
							}
							//m_userlistOnlineFriends.Add( nID );
						}
					}

					if( ! m_bTcpLogin ) { // if network changed, re-do network testing to get the new network info
						DWORD wan_ip = 0;
						re >> wan_ip;
						if( wan_ip != m_netWAN.ip ) {
							TestServer( m_chanLogin.peer.ip, m_chanLogin.peer.port );
						}
					}
				}
				bDone = true;
				m_tHeartbeatOkayTime = time(NULL);
				//OnHeartbeatFeedback( m_userlistOnlineFriends );
				{
					LockIt safe( m_mutexVars );//chy
					OnHeartbeatFeedback( m_listFriends );
				}
				break;
			case SIMS_NOTAVAILABLE:
				{
					re >> ipSwitch >> portSwitch;
					m_errLastError = ServerSwitch;
					bSessionExpired = false;
				}
				break;
			case SIMS_BADREQUEST:
				m_errLastError = InvalidAlias;
				break;
			case SIMS_DENY:
			default:
				m_errLastError = NetworkError;
				break;
			}
		} catch ( const char * ) {
			m_errLastError = NetworkError;
			bFailLogout = false;
		}

		ReleaseMessage( reply );

		if( ! bDone && bFailLogout ) {
			PString strMsg( PString::Printf, "Failed sending heartbeat: statusCode = %d", statusCode );
			PLOG( m_pLogger, Debug, strMsg );
			//PTRACE( 1, "Failed sending heartbeat: statusCode = " << statusCode );

			Logout();
      OnHeartbeatTimeout();
			if( bSessionExpired ) {
				OnSessionExpired();
			}
		}

		if( ipSwitch != 0 ) {
			OnNotifySwitchServerWhenHeartbeat( ipSwitch, (portSwitch ? portSwitch : SIM_PORT) );
		} else if( ! bDone && bFailLogout  && !bSessionExpired ) {
			PTRACE(1, "Switch server error!");
			OnSessionExpired();
		}

	} else {
		if( m_chanLogin.socket == 0 ) {
			m_errLastError = Timeout;

			// if timeout only, do not logout, we will retry heartbeat next time
			OnHeartbeatTimeout();
		} else {

			fd_set fdsetRead;
			FD_ZERO( & fdsetRead );
			FD_SET( m_chanLogin.socket, & fdsetRead );
			struct timeval tval = { 0, 0 };
			int s = select( m_chanLogin.socket +1, & fdsetRead, NULL, NULL, &tval);
			if( s < 0 ) {
				m_errLastError = Timeout;

				// if timeout only, do not logout, we will retry heartbeat next time
				OnHeartbeatTimeout();
			} else {
				TestUdpForTcpLogin();
				bDone = true;
			}
		}
	}

	m_bHeartbeatOkay = bDone;

	return bDone;
}

bool BBQClient::IsLogin( void )
{
	return (m_bLogin && m_bHeartbeatOkay);
}

bool BBQClient::PickOrCreateCSChannel( SIM_CHANNEL & chan, const PIPSocket::Address & svrIp, WORD svrPort, WORD localPort )
{
	LockIt safe( m_mutexChannelWait );

	memset( & chan, 0, sizeof(chan) );

	DWORD ip = (DWORD) svrIp;

	if( ip == m_chanLogin.peer.ip ) {
		chan = m_chanLogin;
	} else {
		IPCHANMAP::iterator it = m_mapChanOtherServers.find( ip );
		if( it != m_mapChanOtherServers.end() ) {
			chan = it->second.channel;
			it->second.timestamp = GetHostUpTimeInMs();
		} else {
			if( m_bTcpLogin ) {
				chan.socket = Connect( svrIp, svrPort );
				if( chan.socket ) {
					sockaddr_in address;
					socklen_t size = sizeof(address);
					if( 0 == ::getsockname(chan.socket,(struct sockaddr*)&address, &size)) {
						PIPSocket::Address addr = address.sin_addr;
						chan.local.ip = (DWORD) addr;
						chan.local.port = ntohs(address.sin_port);
					}

					chan.peer.ip = svrIp;
					chan.peer.port = svrPort;

					CS_CHAN_INFO info;
					info.channel = chan;
					info.timestamp = GetHostUpTimeInMs();

					m_mapChanOtherServers[ ip ] = info;
					return true;
				}
			} else {
				chan.socket = 0;
				chan.local.ip = 0;
				chan.local.port = localPort ? localPort : m_chanLogin.local.port;
				chan.peer.ip = svrIp;
				chan.peer.port = svrPort;

				CS_CHAN_INFO info;
				info.channel = chan;
				info.timestamp = GetHostUpTimeInMs();

				m_mapChanOtherServers[ ip ] = info;
				return true;
			}
		}

	}

	if( chan.socket ) { // validate this tcp socket
		fd_set fdsetRead;
		FD_ZERO( & fdsetRead );
		FD_SET( chan.socket, & fdsetRead );
		struct timeval tval = { 0, 0 };
		int s = select( chan.socket +1, & fdsetRead, NULL, NULL, &tval);
		if( s >= 0 ) {
			return true;
		} else {
			chan.socket = Connect( svrIp, svrPort );
			if( chan.socket ) return true;
			else return false;
		}
	} else {
		return true;
	}

	return false;
}

void BBQClient::ReleaseCSChannel( SIM_CHANNEL & chan )
{
	bool bIsLoginChannel = (( chan.socket != 0 ) && (chan.socket == m_chanLogin.socket))
					|| (( chan.socket == 0 ) && (chan.local.port == m_chanLogin.local.port));

	if( bIsLoginChannel ) {
		// it is the login channel, just keep it
	} else {
		IPCHANMAP::iterator it = m_mapChanOtherServers.find( chan.peer.ip );
		if( it != m_mapChanOtherServers.end() ) {
			//it = m_mapChanOtherServers.erase( it );
			STL_ERASE( m_mapChanOtherServers, IPCHANMAP::iterator, it );
			if( (chan.socket == 0) && (m_chanLogin.socket == 0) && (chan.local.port == m_chanLogin.local.port) ) {
				// skip, cause the UDP port is default one
			} else {
				EraseChannel( chan );
			}
		}
	}
}

bool BBQClient::Locate( SIMD_SC_LOCATE & info, uint32 uid, 
					   PIPSocket::Address svrIp, WORD svrPort, uint32 nFrom )
{
	bool bThisServerOnly = (info.addrServer.ip != 0);

	// if no server is specified, then use current server that I login
	DWORD dwServerIp = (DWORD) svrIp;
	if( dwServerIp == 0 ) {
		if( ! m_bLogin ) {
			m_errLastError = LocalOffline;
			return false;
		}

		svrIp = m_chanLogin.peer.ip;
		svrPort = m_chanLogin.peer.port;

		dwServerIp = m_chanLogin.peer.ip;
	}

	SIM_CHANNEL chan;
	if( ! PickOrCreateCSChannel( chan, svrIp, svrPort ) ) return false;

	// send request, and pWait for response
	SIM_REQUEST req;
	SIM_REQINITCHAN( req, chan, SIM_CS_LOCATE, sizeof(SIMD_CS_LOCATE) );

	SIMD_CS_LOCATE * pQ = (SIMD_CS_LOCATE *) & req.msg.simData;
	memset( pQ, 0, sizeof(pQ) );
	if( nFrom == 0 ) {
		pQ->sessionInfo = m_sessionInfo;
	} else {
		pQ->sessionInfo.id = nFrom;
		pQ->sessionInfo.cookie = 0;
	}
	pQ->queryID = uid;
	pQ->thisServerOnly = bThisServerOnly ? 1 : 0; // notify server that do not forward again

	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_LOCATE, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_LOCATE * pA = (SIMD_SC_LOCATE *) & reply->msg.simData;
		info = * pA;
		ReleaseMessage( reply );

		switch( info.statusCode ) {
		case SIMS_OK:
		case SIMS_TIMEOUT:
		case SIMS_NOTAVAILABLE:
			if( info.addrServer.ip == 0 ) info.addrServer.ip = svrIp;
			if( info.addrServer.port == 0 ) info.addrServer.port = svrPort;
			return true;

		case SIMS_EXISTS: // the id exists, but it is on another server
			if( (info.addrServer.ip != 0) && /*(info.addrServer.ip != dwServerIp) &&*/ (! bThisServerOnly) ) {
				// the queried uid is currently on another server, we query again
				return Locate( info, uid, info.addrServer.ip, info.addrServer.port );
			} else {
				return false;
			}

		case SIMS_NOTFOUND:	
		default:
			return false;
		}
	} else {
		m_errLastError = Timeout;
	}

	return false;
}

bool BBQClient::LocateIP( PIPSocket::Address & addr, PIPSocket::Address & addrLAN, uint32 uid, PIPSocket::Address svrIp, WORD svrPort )
{
	SIMD_SC_LOCATE info;
	memset( & info, 0, sizeof(info) ); // must set info.addrServer.ip = 0 before call locate()
	if( Locate( info, uid, svrIp, svrPort ) ) {

		switch( info.statusCode ) {
		case SIMS_OK:
			addr = info.addrWAN.ip;
			addrLAN = info.addrLAN.ip;
			return true;

		case SIMS_NOTFOUND:
			m_errLastError = PeerNotFound;
			break;
		case SIMS_TIMEOUT:
			m_errLastError = PeerOffline;
			break;
		case SIMS_DENY:
			m_errLastError = LocalOffline;
			break;
		case SIMS_NETWORKERROR:
			m_errLastError = PeerDenied;
		}

		return false;
	}

	return false;
}

void BBQClient::BreakChannelRequesting( uint32 uid, uint16 nAppCode, const char * lpszAppString )
{
	LockIt safe( m_mutexChannelWait );

	for( cw_iterator iter = m_listChannelWaits.begin(), eiter = m_listChannelWaits.end(); iter != eiter; iter ++ ) {
		BBQChannelWait * pWait = * iter;
		UCMD_CHANNELINFO * info = & pWait->info;
		if( info->init 
			&& ((info->uid == 0) || (info->uid == uid)) 
			&& ((nAppCode == 0) || (nAppCode == pWait->m_nAppCode)) 
			&& ((lpszAppString == NULL) || (0 == strcmp(lpszAppString, pWait->m_strAppString))) 
			) {
			PTRACE(1, "BreakChannelRequesting: " << uid << ":" << nAppCode << ":" << (lpszAppString == NULL ? "" : lpszAppString));
			pWait->m_bDone = false;
			pWait->Signal();
		}
	}
}

bool BBQClient::ConfirmLocationDirectly( uint32 uid, SIMD_SC_LOCATE & locationInfo )
{
	bool bDone = false;
  if (locationInfo.addrLAN.port ==0)
    locationInfo.addrLAN.port = locationInfo.addrWAN.port;
	SIM_REQUEST req;
	SIM_REQINIT( req, 0, 0, m_nDefaultUdpPort, locationInfo.addrLAN.ip, locationInfo.addrLAN.port,UCM_CC_IDQUERY, sizeof(UCMD_CC_IDQUERY) );
	UCMD_CC_IDQUERY * pQ = (UCMD_CC_IDQUERY *) & req.msg.simData;
	pQ->statusCode = 0;
	pQ->vfonid.serverid = 0;
	pQ->vfonid.userid = uid;
	DWORD dwTimeout = 500;
	SIM_REQUEST * reply = RequestMessage( & req, UCM_CC_IDCONFIRM, dwTimeout + m_nNetworkBaseDelay * 2, 6 );
	if( reply ) {
		UCMD_CC_IDCONFIRM * pA = (UCMD_CC_IDCONFIRM *) & reply->msg.simData;
		bDone = ( pA->statusCode == SIMS_OK );
		ReleaseMessage( reply );
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

BBQ_CHANNEL * BBQClient::DirectTcpConnect( uint32 uid, 
										  uint16 nAppCode, const char * lpszAppString, uint32 nAppParam,
										  DWORD timeout, PIPSocket::Address ip, WORD port, bool bHost )
{
	BBQ_CHANNEL * pChannel = NULL;
#define _OLD_DIRECTTCPCONNECT
#ifdef _OLD_DIRECTTCPCONNECT
	//BBQ_CHANNEL * pChannel = NULL;
  int fd = -1;
  if (IS_PRIVATE_IP( m_chanLogin.local.ip) == false && m_chanLogin.local.ip == m_netWAN.ip && bHost)
  {
    char pData[4+2]={0}; 
    memcpy(pData,&m_netWAN.ip, 4); 
    memcpy(pData+4,&port, 2);
    SendUserDefinedMessage(BBQ_ENC_PLAIN,  D_USERDEFINEDMESSAGE_1,sizeof(pData), pData, uid);
    int nTry = 500;
    //tell peer to try connect here
    while (--nTry>0)
    {
		{
			LockIt safe( m_mutexNet );
			BBQMsgConnection * pConn = FindMsgConnectionEx(ip, 0,  port, 0);//only match ip
			if (pConn&& pConn->GetSocket())   {
				fd = pConn->GetSocket()->GetHandle();break;
			}
		}
      Sleep(2);
    }
  }else
	  fd= Connect( ip, port, timeout );

#else
	PTCPSocket * tcpSock = new PTCPSocket();
	if( ! tcpSock ) return NULL;
    const int one = 1;
    if (setsockopt(tcpSock->GetHandle(), IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one)))
			{
        PTRACE(3, "setocket failed");
				//return VFalse;
			}
      fd= tcpSock->GetHandle();
      
#endif
	if( fd > 0 ) {
 //   const int one = 1;
 //   if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one)))
	//		{
 //       PTRACE(3, "setocket failed");
	//			//return VFalse;
	//		}
		SIM_REQUEST req;
		SIM_REQINIT( req, fd, 0, 0, ip, port, UCM_CC_Q, sizeof(UCMD_CC_Q) );
		UCMD_CC_Q * pQ = (UCMD_CC_Q *) & req.msg.simData;
		memset( pQ, 0, sizeof(*pQ) );
		pQ->sessionInfo = m_sessionInfo;
		pQ->channelInfo.init = 1;
		{
			LockIt sf( m_mutexChannelWait );
			pQ->channelInfo.cid = ++ m_nNextChannelId;
		}
		pQ->channelInfo.cookie = (PTimer::Tick().GetInterval() * rand() * m_nNextChannelId * uid);
		pQ->channelInfo.uid = uid;
		pQ->statusCode = nAppCode;
		strncpy( pQ->statusText, lpszAppString, SIM_APPSTRINGSIZEMAX ); pQ->statusText[ SIM_APPSTRINGSIZEMAX -1 ] = '\0';
		pQ->extraInfo.caller.dwTimeBase = m_dwTimeBase;
		pQ->extraInfo.caller.dwChannelId = pQ->channelInfo.cid;
		pQ->extraInfo.caller.nServerId = m_nServerId;
		pQ->extraInfo.caller.nAppParam = nAppParam;
		
		SIM_REQUEST * reply = RequestMessage( & req, UCM_CC_A, timeout, 1 );

		BBQMsgConnection * pConn = FindMsgConnection(0, 0, fd);
		if( pConn && DetachMsgConnection(pConn) ) {
			if( reply ) {
				UCMD_CC_A * pA = (UCMD_CC_A *) & reply->msg.simData;
				if( pA->statusCode == SIMS_OK ) {

					pChannel = new BBQ_CHANNEL;
					memset( pChannel, 0, sizeof(BBQ_CHANNEL) );

					pChannel->tcpSock = pConn->DetachSocket(); 
					pChannel->pExtraData = pConn->DetachExtraData();

					pChannel->info.peerId = uid;

					PIPSocket::Address addr; WORD port;
					pChannel->tcpSock->GetPeerAddress( addr, port );
					pChannel->info.peerWAN.ip = pChannel->info.peerLAN.ip = (DWORD) addr;
					pChannel->info.peerWAN.port = pChannel->info.peerLAN.port = port;

					pChannel->tcpSock->GetLocalAddress( addr, port );
					pChannel->info.thisLAN.ip = (DWORD) addr;
					pChannel->info.thisLAN.port = port;

					pChannel->info.bIsProxied = false;
					pChannel->info.bInitFromLocal = true;

					pChannel->info.dwLocalCount = pQ->extraInfo.caller.dwChannelId;
					pChannel->info.dwLocalTimeBase = pQ->extraInfo.caller.dwTimeBase;
					pChannel->info.nLocalAppParam = pQ->extraInfo.caller.nAppParam;

					pChannel->info.dwRemoteCount = pA->extraInfo.called.dwChannelId;
					pChannel->info.dwRemoteTimeBase = pA->extraInfo.called.dwTimeBase;
					pChannel->info.nRemoteAppParam = pA->extraInfo.called.nAppParam;

					pChannel->info.peerServerId = pQ->extraInfo.called.nServerId;
				}
				ReleaseMessage( reply );
			}
			delete pConn;
		}
	}

	return pChannel;
}

BBQ_CHANNEL * BBQClient::DirectUdpConnect( uint32 uid,
										  uint16 nAppCode, const char * lpszAppString, uint32 nAppParam,
										  DWORD timeout, PIPSocket::Address ip, WORD port )
{
	BBQ_CHANNEL * pChannel = NULL;

	// bind a port
	PUDPSocket * pSock = NULL;
	WORD nChannelPort = 0;
	if( ! BindDynamicUdpPort( pSock, nChannelPort ) ) {
		m_errLastError = InvalidPortRange;
		return NULL;
	}

	// init channel info and pWait object ------------------------------
	BBQChannelWait* pWait = new BBQChannelWait( pSock, *this );

	pWait->info.uid = uid;
	pWait->info.init = 1; // important flag, never change again

	// lock to ensure the id is unique
	{
		LockIt sf( m_mutexChannelWait );
		pWait->info.cid = ++ m_nNextChannelId;
	}

	PIPSocket::Address local_addr;	GetHostAddress( local_addr );
	pWait->info.thisLAN.ip = local_addr;
	pWait->info.thisLAN.port = nChannelPort;

	// invite, if this side is accessable ( no firewall or loose policy )
	pWait->info.firewall = m_nFirewallType & 0x3;
	pWait->info.invite = (pWait->info.firewall != StrictPolicy) ? 1 : 0;
	pWait->info.cookie = pWait->info.invite ? (PTimer::Tick().GetInterval() * rand() * m_nNextChannelId * uid) : 0;

	//pWait->info.tcp = (modes & TCP_PROXY) ? 1 : 0;
	//pWait->info.give_me_proxy = ((modes & UDP_PROXY) || (modes & TCP_PROXY)) ? 1 : 0;

	pWait->m_nAppCode = nAppCode;
	pWait->m_strAppString = lpszAppString;

	pWait->extra.dwChannelId = pWait->info.cid;
	pWait->extra.dwTimeBase = m_dwTimeBase;
	pWait->extra.nServerId = m_nServerId;
	pWait->extra.nAppParam = nAppParam;
	// ------------------------------------------------------------------

	// insert to pWait object list, before send message
	{
		LockIt sf( m_mutexChannelWait );
		m_listChannelWaits.push_back( pWait );
	}

	// if this client can be access by it's local ip, then send request directly to the client
	{
		SIM_REQUEST req;
		SIM_REQINIT( req, 0, 0, nChannelPort, ip, port, UCM_CC_Q, sizeof(UCMD_CC_Q) );
		UCMD_CC_Q * pQ = (UCMD_CC_Q *) & req.msg.simData;
		memset( pQ, 0, sizeof(*pQ) );
		pQ->sessionInfo = m_sessionInfo;
		pQ->channelInfo = pWait->info;
		pQ->statusCode = nAppCode;
		strncpy( pQ->statusText, lpszAppString, SIM_APPSTRINGSIZEMAX ); pQ->statusText[ SIM_APPSTRINGSIZEMAX -1 ] = '\0';
		pQ->extraInfo.caller = pWait->extra;
    //for(int i=0;i< 3;i++)
    {
		  SendMessage( & req );
    }
	}

	// now pWait the event success or failure
	if( pWait->Wait(timeout) && pWait->m_bDone ) {
		pChannel = pWait->Detach();
	}

	// now remove the pWait object from list
	{
		LockIt sf( m_mutexChannelWait );
		m_listChannelWaits.remove( pWait );
	}

	delete pWait;

	return pChannel;
}
BBQ_CHANNEL * BBQClient::RequestChannelForTestingProxy( uint32 uid, PIPSocket::Address proxyIp,bool bTestUdp,
										DWORD timeout,   PIPSocket::Address svrIp, WORD svrPort  )
{
  DWORD modes=0;
  if (bTestUdp)
  {
    modes =UDP_PROXY;
  }else
  {
    modes =TCP_PROXY;
  }
  return RequestChannel(uid, CLIENT_REQUEST_CHANNEL_FOR_TESTING_PROXY_MARK, (const char*)proxyIp.AsString(),0,timeout, modes, svrIp, svrPort, proxyIp);
}
BBQ_CHANNEL * BBQClient::RequestChannel( uint32 uid, 
										uint16 nAppCode, const char * lpszAppString, uint32 nAppParam,
										DWORD timeout, DWORD modes, PIPSocket::Address svrIp, WORD svrPort,  PIPSocket::Address proxyIp )
{
	BBQ_CHANNEL * pChannel = NULL;
	bool bForceTcp = m_bForceTcp;
	bool bForceBBQProxy = m_bForceBBQProxy;
	if( (modes & PROXY_FORCE) == PROXY_FORCE ) {
		bForceBBQProxy = true;
		modes &= ~PROXY_FORCE;
	}
	if( (modes & TCP_FORCE) == TCP_FORCE ) {
		bForceTcp = true;
		modes &= ~TCP_FORCE;
	}
	if( lpszAppString == NULL ) lpszAppString = "";
	if( strlen(lpszAppString) >= SIM_APPSTRINGSIZEMAX ) {
		m_errLastError = InvalidArgument;
		return NULL;
	}

	// if no server is specified, then use current server that I login
	DWORD dwServerIp = (DWORD) svrIp;
	if( dwServerIp == 0 ) {
		if( ! m_bLogin ) {
			m_errLastError = LocalOffline;
			return NULL;
		}

		svrIp = m_chanLogin.peer.ip;
		svrPort = m_chanLogin.peer.port;

		dwServerIp = m_chanLogin.peer.ip;
	}

	SIM_CHANNEL chan;
	if( ! PickOrCreateCSChannel( chan, svrIp, svrPort ) ) {
		BBQTRACE(1, "PickOrCreateCSChannel Error!!!!!");
		return NULL;
	}

	// locate the uid, and get important information for later use
	SIMD_SC_LOCATE locationInfo;
	memset( & locationInfo, 0, sizeof(locationInfo) ); // must set info.addrServer.ip = 0 before call locate()
	bool bLocated = false;
	if( Locate( locationInfo, uid, svrIp, svrPort ) ) {
		switch( locationInfo.statusCode ) {
		case SIMS_OK:
			bLocated = true;
			break;
		case SIMS_NOTAVAILABLE: // user set call forward for any reason
			if( (nAppCode != 323) && (nAppCode != 523) ) { // allowed if it's not a video conference
				bLocated = true;
			}
			break;
		}
	}

	if( bLocated ) {
		// if the user login from same host of the server using "localhost",
		// we must change the "127.0.0.1" to server true ip,
		// for example, when MCU install on the same host
		if( locationInfo.addrWAN.ip == LOCALHOST_IP ) {
			locationInfo.addrWAN.ip = m_chanLogin.peer.ip;
			if( ! bForceBBQProxy ) {
//				modes |= (UDP_P2P | TCP_P2P);
			}
		}
    //we use peer's setting  to confirm the channel's property
    if ( ( nAppCode == 323|| nAppCode == 523 ) && locationInfo.transportchannelmodes !=0 )
    {
      bForceTcp =  ((locationInfo.transportchannelmodes & TCP_FORCE) == TCP_FORCE);
      if ( bForceTcp!= m_bForceTcp )           
      {
        if (m_bForceTcp == false){
           BBQTRACE(3, "BBQClient\t Agree the peer use tcp channel to transfer data.");
        }
        else
          bForceTcp = m_bForceTcp;
      }
    }
	} else {
		switch( locationInfo.statusCode ) {
		case SIMS_NOTFOUND:		
			m_errLastError = PeerNotFound;	
			break;
		case SIMS_TIMEOUT:		
		case SIMS_NOTAVAILABLE:		
			m_errLastError = PeerOffline;
			break;
		case SIMS_NETWORKERROR:	
			m_errLastError = NetworkError;
			break;
		case 0:
			m_errLastError = Timeout;
			break;
		case SIMS_DENY:
		default:
			m_errLastError = PeerDenied;	
		}
		BBQTRACE( 1, "Locate " << (unsigned int) uid << " failed: " << locationInfo.statusCode << ", " << GetLastErrorString() );
		ReleaseCSChannel( chan );
		return NULL;
	}
	if( bForceTcp || m_bUdpBlocked) {
		modes = TCP_P2P | TCP_PROXY | TCP_PROXY_AUTO;

		modes &= ~(UDP_P2P | UDP_PROXY);
	}

	if( bForceBBQProxy ) {
		if( modes & (UDP_P2P | UDP_PROXY) ) modes |= UDP_PROXY;
		if( modes & (TCP_P2P | TCP_PROXY) ) modes |= TCP_PROXY;

		modes &= ~ (UDP_P2P | TCP_P2P);
	}

	// try udp direct connect, if peer is in same LAN or VPN
	if( modes & UDP_P2P ) {
		if( ConfirmLocationDirectly( uid, locationInfo ) ) {
			pChannel = DirectUdpConnect(uid, nAppCode, lpszAppString, nAppParam, CLIENT_SIMPLE_REQUEST_TIMEOUT  +500  /*timeout*/, locationInfo.addrLAN.ip, locationInfo.addrLAN.port);
			if( pChannel ) {
				ReleaseCSChannel( chan );
				BBQCHANNEL_LOCAL_TRACE(pChannel);
				return pChannel;
			}
		}
	}
	// try tcp direct connect, if peer has public ip or port mapped.
	if( modes & TCP_P2P ) {
		WORD nRemoteTCPListenPort = (locationInfo.tcplistenport != 0? locationInfo.tcplistenport: SIM_PORT -1);
		pChannel = DirectTcpConnect(uid, nAppCode, lpszAppString, nAppParam, 1000 + m_nNetworkBaseDelay * 2 /*timeout*/, locationInfo.addrLAN.ip, nRemoteTCPListenPort,  !(locationInfo.addrLAN.ip== locationInfo.addrWAN.ip && false==IS_PRIVATE_IP(locationInfo.addrWAN.ip)) );
		if( pChannel ) {
			ReleaseCSChannel( chan );
			BBQCHANNEL_LOCAL_TRACE(pChannel);
			return pChannel;
		}

		pChannel = DirectTcpConnect(uid, nAppCode, lpszAppString, nAppParam, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 /*timeout*/, locationInfo.addrWAN.ip,nRemoteTCPListenPort, !(locationInfo.addrLAN.ip== locationInfo.addrWAN.ip && false==IS_PRIVATE_IP(locationInfo.addrWAN.ip)));
		if( pChannel ) {
			ReleaseCSChannel( chan );
			BBQCHANNEL_LOCAL_TRACE(pChannel);
			return pChannel;
		}
	}

	// bind a port
	PUDPSocket * pSock = NULL;
	WORD nChannelPort = 0;
	if( ! BindDynamicUdpPort( pSock, nChannelPort ) ) {
		m_errLastError = InvalidPortRange;
		return NULL;
	}

	// init channel info and pWait object ------------------------------
	BBQChannelWait* pWait = new BBQChannelWait( pSock, *this );

	pWait->info.uid = uid;
	pWait->info.init = 1; // important flag, never change again

	// lock to ensure the channel id is unique
	{
		LockIt sf( m_mutexChannelWait );
		pWait->info.cid = ++ m_nNextChannelId;
	}

	PIPSocket::Address local_addr;	GetHostAddress( local_addr );
	pWait->info.thisLAN.ip = local_addr;
	pWait->info.thisLAN.port = nChannelPort;

	// init as zero, cause we dont know
	memset( & pWait->info.peerLAN, 0, sizeof(IPSOCKADDR) );
	memset( & pWait->info.peerWAN, 0, sizeof(IPSOCKADDR) );

	// invite, if this side is accessable ( no firewall or loose policy )
	pWait->info.firewall = m_nFirewallType & 0x3;
	pWait->info.invite = (pWait->info.firewall != StrictPolicy) ? 1 : 0;
	pWait->info.cookie = pWait->info.invite ? (PTimer::Tick().GetInterval() * rand() * m_nNextChannelId * uid) : 0;

	pWait->info.tcp = (modes & (TCP_PROXY | TCP_PROXY_AUTO)) ? 1 : 0;
	pWait->info.give_me_proxy = (bForceBBQProxy || (modes & UDP_PROXY) || (modes & TCP_PROXY)) ? 1 : 0;

	pWait->m_nAppCode = nAppCode;
	pWait->m_strAppString = lpszAppString;

	pWait->extra.dwChannelId = pWait->info.cid;
	pWait->extra.dwTimeBase = m_dwTimeBase;
	pWait->extra.nServerId = m_nServerId;
	pWait->extra.nAppParam = nAppParam;

	pWait->extra.use_firewall_here = 1;
	pWait->extra.firewall = m_nFirewallType & 0x3;
	// ------------------------------------------------------------------

	// insert to pWait object list, before send message
	{
		LockIt sf( m_mutexChannelWait );
		m_listChannelWaits.push_back( pWait );
	}

	SIM_REQUEST req;

	bool bSvrDiff = (0 != memcmp(& chan.peer, & locationInfo.addrServer, sizeof(chan.peer))); 
	
	SIM_CHANNEL chanForChan;
	memset( & chanForChan, 0, sizeof(chanForChan) );
	if( bSvrDiff ) {
		if( PickOrCreateCSChannel( chanForChan, locationInfo.addrServer.ip, locationInfo.addrServer.port ) ) {
			// start the conversation by sending a request to server
			SIM_REQINITCHAN( req, chanForChan, UCM_CS_Q, sizeof(UCMD_CS_Q) );
		} else {
			BBQTRACE(1, "PickOrCreateCSChannel Error!!!!!");
		}
	} else {
		// start the conversation by sending a request to server
		if( m_bTcpLogin && (! m_bUdpBlocked) ) {
			chan = m_chanHeartbeat;
		}
		SIM_REQINITCHAN( req, chan, UCM_CS_Q, sizeof(UCMD_CS_Q) );
	}

	if( m_bTcpLogin && pWait->info.tcp ) { // if ask for tcp proxy, we send via tcp login channel to avoid udp packet loss
		req.channel = m_chanLogin;
	}

	// for UDP, send from new port;
	// for TCP, send using the TCP login channel
	if( req.channel.socket == 0 ) req.channel.local.port = nChannelPort;
	//int fd = ( (dwServerIp == m_chanLogin.peer.ip) && (svrPort == m_chanLogin.peer.port) ) ? m_chanLogin.socket : 0;
	//SIM_REQINIT( req, fd, 0, nChannelPort, dwServerIp, svrPort, UCM_CS_Q, sizeof(UCMD_CS_Q) );
	UCMD_CS_Q * pQ = (UCMD_CS_Q *) & req.msg.simData;
	memset( pQ, 0, sizeof(*pQ) );
	pQ->sessionInfo = m_sessionInfo;
	pQ->channelInfo = pWait->info;
  pQ->proxyIp = proxyIp; //chenyuan
	pQ->statusCode = nAppCode;
	strncpy( pQ->statusText, lpszAppString, SIM_APPSTRINGSIZEMAX ); pQ->statusText[ SIM_APPSTRINGSIZEMAX -1 ] = '\0';
	pQ->extraInfo.caller = pWait->extra;

#ifdef ANTI_PACKET_LOSS
	//PostMessage( &req, true );
	// udp connection to server, we send 3 times to avoid packet loss

	if( req.channel.socket == 0 ) {
		for( int i=0; i<3; i++ ) { SendMessage( & req ); sleep_ms(1); }
	} else {
		SendMessage( & req );
	}
#else
	SendMessage( & req );
#endif

	// tcp connection to server, the data is forward very slowly than UDP, so we must pWait more time
	if( locationInfo.isTCP || req.channel.socket ) timeout *= 3;

	// now pWait the event success or failure
	if( pWait->Wait(timeout) ) {
		if( pWait->m_bDone ) {
			pChannel = pWait->Detach();
		} else {
			// m_errLastError has been changed, so do not change it
		}
	} else {
		BBQTRACE(1,"Failed to request channel, peer id is " << uid); 
		m_errLastError = Timeout;
	}

	//if( pWait->Wait(timeout) && pWait->m_bDone ) {
	//	pChannel = pWait->Detach();
	//} else {
	//	m_errLastError = Timeout;
	//}

	// now remove the pWait object from list
	{
		LockIt sf( m_mutexChannelWait );
		m_listChannelWaits.remove( pWait );
	}

	delete pWait;

	if( bSvrDiff ) ReleaseCSChannel( chanForChan );
	ReleaseCSChannel( chan );
  BBQTRACE(5,"requested channel.");

	BBQCHANNEL_LOCAL_TRACE(pChannel);
	return pChannel;
}


//BBQ_CHANNEL * BBQClient::RequestChannel( uint32 uid, 
//										uint16 nAppCode, const char * lpszAppString, uint32 nAppParam,
//										DWORD timeout, DWORD modes, PIPSocket::Address svrIp, WORD svrPort,  PIPSocket::Address proxyIp )
//{
//	BBQ_CHANNEL * pChannel = NULL;
//
//	if( lpszAppString == NULL ) lpszAppString = "";
//	if( strlen(lpszAppString) >= SIM_APPSTRINGSIZEMAX ) {
//		m_errLastError = InvalidArgument;
//		return NULL;
//	}
//
//	// if no server is specified, then use current server that I login
//	DWORD dwServerIp = (DWORD) svrIp;
//	if( dwServerIp == 0 ) {
//		if( ! m_bLogin ) {
//			m_errLastError = LocalOffline;
//			return NULL;
//		}
//
//		svrIp = m_chanLogin.peer.ip;
//		svrPort = m_chanLogin.peer.port;
//
//		dwServerIp = m_chanLogin.peer.ip;
//	}
//
//	SIM_CHANNEL chan;
//	if( ! PickOrCreateCSChannel( chan, svrIp, svrPort ) ) {
//		PTRACE(1, "PickOrCreateCSChannel Error!!!!!");
//		return NULL;
//	}
//
//	// locate the uid, and get important information for later use
//	SIMD_SC_LOCATE locationInfo;
//	memset( & locationInfo, 0, sizeof(locationInfo) ); // must set info.addrServer.ip = 0 before call locate()
//	bool bLocated = false;
//	if( Locate( locationInfo, uid, svrIp, svrPort ) ) {
//		switch( locationInfo.statusCode ) {
//		case SIMS_OK:
//			bLocated = true;
//			break;
//		case SIMS_NOTAVAILABLE: // user set call forward for any reason
//			if( (nAppCode != 323) && (nAppCode != 523) ) { // allowed if it's not a video conference
//				bLocated = true;
//			}
//			break;
//		}
//	}
//
//	if( bLocated ) {
//		// if the user login from same host of the server using "localhost",
//		// we must change the "127.0.0.1" to server true ip,
//		// for example, when MCU install on the same host
//		if( locationInfo.addrWAN.ip == LOCALHOST_IP ) {
//			locationInfo.addrWAN.ip = m_chanLogin.peer.ip;
//			if( ! m_bForceBBQProxy ) {
////				modes |= (UDP_P2P | TCP_P2P);
//			}
//		}
//	} else {
//		switch( locationInfo.statusCode ) {
//		case SIMS_NOTFOUND:		
//			m_errLastError = PeerNotFound;	
//			break;
//		case SIMS_TIMEOUT:		
//		case SIMS_NOTAVAILABLE:		
//			m_errLastError = PeerOffline;
//			break;
//		case SIMS_NETWORKERROR:	
//			m_errLastError = NetworkError;
//			break;
//		case 0:
//			m_errLastError = Timeout;
//			break;
//		case SIMS_DENY:
//		default:
//			m_errLastError = PeerDenied;	
//		}
//		PTRACE( 1, "Locate " << (unsigned int) uid << " failed: " << locationInfo.statusCode << ", " << GetLastErrorString() );
//		ReleaseCSChannel( chan );
//		return NULL;
//	}
//
//	if( m_bForceTcp ) {
//		modes = TCP_P2P | TCP_PROXY | TCP_PROXY_AUTO;
//
//		modes &= ~(UDP_P2P | UDP_PROXY);
//	}
//
//	if( m_bForceBBQProxy ) {
//		if( modes & (UDP_P2P | UDP_PROXY) ) modes |= UDP_PROXY;
//		if( modes & (TCP_P2P | TCP_PROXY) ) modes |= TCP_PROXY;
//
//		modes &= ~ (UDP_P2P | TCP_P2P);
//	}
//
//	// try udp direct connect, if peer is in same LAN or VPN
//	if( modes & UDP_P2P ) {
//		if( ConfirmLocationDirectly( uid, locationInfo ) ) {
//			pChannel = DirectUdpConnect(uid, nAppCode, lpszAppString, nAppParam, CLIENT_SIMPLE_REQUEST_TIMEOUT  + m_nNetworkBaseDelay * 6 /*timeout*/, locationInfo.addrLAN.ip, locationInfo.addrLAN.port);
//			if( pChannel ) {
//				ReleaseCSChannel( chan );
//				BBQCHANNEL_LOCAL_TRACE(pChannel);
//				return pChannel;
//			}
//		}
//	}
//
//	// try tcp direct connect, if peer has public ip or port mapped.
//	if( modes & TCP_P2P ) {
//		pChannel = DirectTcpConnect(uid, nAppCode, lpszAppString, nAppParam, 1000 + m_nNetworkBaseDelay * 2 /*timeout*/, locationInfo.addrLAN.ip, SIM_PORT -1,  !(locationInfo.addrLAN.ip== locationInfo.addrWAN.ip && false==IS_PRIVATE_IP(locationInfo.addrWAN.ip)));
//		if( pChannel ) {
//			ReleaseCSChannel( chan );
//			BBQCHANNEL_LOCAL_TRACE(pChannel);
//			return pChannel;
//		}
//
//		pChannel = DirectTcpConnect(uid, nAppCode, lpszAppString, nAppParam, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 /*timeout*/, locationInfo.addrWAN.ip, SIM_PORT -1,  !(locationInfo.addrLAN.ip== locationInfo.addrWAN.ip && false==IS_PRIVATE_IP(locationInfo.addrWAN.ip)));
//		if( pChannel ) {
//			ReleaseCSChannel( chan );
//			BBQCHANNEL_LOCAL_TRACE(pChannel);
//			return pChannel;
//		}
//	}
//
//	// bind a port
//	PUDPSocket * pSock = NULL;
//	WORD nChannelPort = 0;
//	if( ! BindDynamicUdpPort( pSock, nChannelPort ) ) {
//		m_errLastError = InvalidPortRange;
//		return NULL;
//	}
//
//	// init channel info and pWait object ------------------------------
//	BBQChannelWait* pWait = new BBQChannelWait( pSock, *this );
//
//	pWait->info.uid = uid;
//	pWait->info.init = 1; // important flag, never change again
//
//	// lock to ensure the channel id is unique
//	{
//		LockIt sf( m_mutexChannelWait );
//		pWait->info.cid = ++ m_nNextChannelId;
//	}
//
//	PIPSocket::Address local_addr;	GetHostAddress( local_addr );
//	pWait->info.thisLAN.ip = local_addr;
//	pWait->info.thisLAN.port = nChannelPort;
//
//	// init as zero, cause we dont know
//	memset( & pWait->info.peerLAN, 0, sizeof(IPSOCKADDR) );
//	memset( & pWait->info.peerWAN, 0, sizeof(IPSOCKADDR) );
//
//	// invite, if this side is accessable ( no firewall or loose policy )
//	pWait->info.firewall = m_nFirewallType & 0x3;
//	pWait->info.invite = (pWait->info.firewall != StrictPolicy) ? 1 : 0;
//	pWait->info.cookie = pWait->info.invite ? (PTimer::Tick().GetInterval() * rand() * m_nNextChannelId * uid) : 0;
//
//	pWait->info.tcp = (modes & (TCP_PROXY | TCP_PROXY_AUTO)) ? 1 : 0;
//	pWait->info.give_me_proxy = (m_bForceBBQProxy || (modes & UDP_PROXY) || (modes & TCP_PROXY)) ? 1 : 0;
//
//	pWait->m_nAppCode = nAppCode;
//	pWait->m_strAppString = lpszAppString;
//
//	pWait->extra.dwChannelId = pWait->info.cid;
//	pWait->extra.dwTimeBase = m_dwTimeBase;
//	pWait->extra.nServerId = m_nServerId;
//	pWait->extra.nAppParam = nAppParam;
//
//	pWait->extra.use_firewall_here = 1;
//	pWait->extra.firewall = m_nFirewallType & 0x3;
//	// ------------------------------------------------------------------
//
//	// insert to pWait object list, before send message
//	{
//		LockIt sf( m_mutexChannelWait );
//		m_listChannelWaits.push_back( pWait );
//	}
//
//	SIM_REQUEST req;
//
//	bool bSvrDiff = (0 != memcmp(& chan.peer, & locationInfo.addrServer, sizeof(chan.peer))); 
//	
//	SIM_CHANNEL chanForChan;
//	memset( & chanForChan, 0, sizeof(chanForChan) );
//	if( bSvrDiff ) {
//		if( PickOrCreateCSChannel( chanForChan, locationInfo.addrServer.ip, locationInfo.addrServer.port ) ) {
//			// start the conversation by sending a request to server
//			SIM_REQINITCHAN( req, chanForChan, UCM_CS_Q, sizeof(UCMD_CS_Q) );
//		} else {
//			PTRACE(1, "PickOrCreateCSChannel Error!!!!!");
//		}
//	} else {
//		// start the conversation by sending a request to server
//		if( m_bTcpLogin && (! m_bUdpBlocked) ) {
//			chan = m_chanHeartbeat;
//		}
//		SIM_REQINITCHAN( req, chan, UCM_CS_Q, sizeof(UCMD_CS_Q) );
//	}
//
//	if( m_bTcpLogin && pWait->info.tcp ) { // if ask for tcp proxy, we send via tcp login channel to avoid udp packet loss
//		req.channel = m_chanLogin;
//	}
//
//	// for UDP, send from new port;
//	// for TCP, send using the TCP login channel
//	if( req.channel.socket == 0 ) req.channel.local.port = nChannelPort;
//	//int fd = ( (dwServerIp == m_chanLogin.peer.ip) && (svrPort == m_chanLogin.peer.port) ) ? m_chanLogin.socket : 0;
//	//SIM_REQINIT( req, fd, 0, nChannelPort, dwServerIp, svrPort, UCM_CS_Q, sizeof(UCMD_CS_Q) );
//	UCMD_CS_Q * pQ = (UCMD_CS_Q *) & req.msg.simData;
//	memset( pQ, 0, sizeof(*pQ) );
//	pQ->sessionInfo = m_sessionInfo;
//	pQ->channelInfo = pWait->info;
//  pQ->proxyIp = proxyIp; //chenyuan
//	pQ->statusCode = nAppCode;
//	strncpy( pQ->statusText, lpszAppString, SIM_APPSTRINGSIZEMAX ); pQ->statusText[ SIM_APPSTRINGSIZEMAX -1 ] = '\0';
//	pQ->extraInfo.caller = pWait->extra;
//
//#ifdef ANTI_PACKET_LOSS
//	//PostMessage( &req, true );
//	// udp connection to server, we send 3 times to avoid packet loss
//
//	if( req.channel.socket == 0 ) {
//		for( int i=0; i<3; i++ ) { SendMessage( & req ); sleep_ms(1); }
//	} else {
//		SendMessage( & req );
//	}
//#else
//	SendMessage( & req );
//#endif
//
//	// tcp connection to server, the data is forward very slowly than UDP, so we must pWait more time
//	if( locationInfo.isTCP || req.channel.socket ) timeout *= 3;
//
//	// now pWait the event success or failure
//	if( pWait->Wait(timeout) ) {
//		if( pWait->m_bDone ) {
//			pChannel = pWait->Detach();
//		} else {
//			// m_errLastError has been changed, so do not change it
//		}
//	} else {
//		PTRACE(1,"Failed to request channel, peer id is " << uid); 
//		m_errLastError = Timeout;
//	}
//
//	//if( pWait->Wait(timeout) && pWait->m_bDone ) {
//	//	pChannel = pWait->Detach();
//	//} else {
//	//	m_errLastError = Timeout;
//	//}
//
//	// now remove the pWait object from list
//	{
//		LockIt sf( m_mutexChannelWait );
//		m_listChannelWaits.remove( pWait );
//	}
//
//	delete pWait;
//
//	if( bSvrDiff ) ReleaseCSChannel( chanForChan );
//	ReleaseCSChannel( chan );
//  PTRACE(5,"requested channel.");
//
//	BBQCHANNEL_LOCAL_TRACE(pChannel);
//	return pChannel;
//}
//
void BBQClient::ReleaseChannel( BBQ_CHANNEL * pChannel )
{
	if( pChannel ) {
		if( pChannel->sock ) 
		{
			PIPSocket::Address addr;
			WORD port =0 ;
			pChannel->sock->GetLocalAddress(addr, port);
			PTRACE(3, "YYClient ReleaseChannel addr is " <<  addr<< ":" <<port);
			delete pChannel->sock;
		}
		if( pChannel->tcpSock ) delete pChannel->tcpSock;
		if( pChannel->sslChannel ) delete pChannel->sslChannel;
		if( pChannel->pExtraData ) delete pChannel->pExtraData;
		delete pChannel;
	}
}

bool BBQClient::OnMsgLoginReplaced( const SIM_REQUEST * req )
{
	// ignore msg not from server, to avoid attack,
	// sometimes IP == 0 means the connection is through http-proxy, then we skip this compare.
	if( req->channel.peer.ip != 0 
		&& m_chanLogin.peer.ip != 0 
		&& memcmp( & req->channel.peer, & m_chanLogin.peer, sizeof(m_chanLogin.peer) ) != 0 ) return true;
	
	LockIt safe( m_mutexChannelWait );

	SIMD_SC_LOGINREPLACED * pN = (SIMD_SC_LOGINREPLACED *) & req->msg.simData;
	if( (pN->sessionInfo.id != m_sessionInfo.id) || (pN->sessionInfo.cookie != m_sessionInfo.cookie) ) return true;

	OnLoginReplaced( pN->addrReplaceBy.ip );

	Logout();

	return true;
}

bool BBQClient::OnMsgChannelRequest( const SIM_REQUEST * req )
{
	// ignore msg not from server, to avoid attack,
	// sometimes IP == 0 means the connection is through http-proxy, then we skip this compare.
	if( req->channel.peer.ip != 0 
		&& m_chanHeartbeat.peer.ip != 0 
		&& memcmp( & req->channel.peer, & m_chanHeartbeat.peer, sizeof(m_chanHeartbeat.peer) ) != 0 && 
		( m_chanLogin.peer.ip == 0 || memcmp( & req->channel.peer, & m_chanLogin.peer, sizeof(m_chanLogin.peer) ) != 0)  ) return true;

	const UCMD_SC_FWD_Q * pQ = (const UCMD_SC_FWD_Q *) & req->msg.simData;
	const UCMD_CHANNELINFO * pPeerInfo = & pQ->channelInfo;

#ifdef ANTI_PACKET_LOSS
	// check request history, if already handled, just ingore
	{
		LockIt sf( m_mutexCidTime );

		UNIQUE_CID ucid;
		ucid.uid = pPeerInfo->uid;
		ucid.cid = pPeerInfo->cid;
		
		if( m_mapCidTime.find( ucid ) != m_mapCidTime.end() ) {
			// same channel already handled, ignore this request
			return true;
		} else {
			m_mapCidTime[ ucid ] = GetHostUpTimeInMs(); //PTimer::Tick().GetInterval();
		}
	}
#endif

	PTRACE( 1, "channel request from: " << pPeerInfo->uid << "." << pPeerInfo->cid );

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, UCM_CS_A, sizeof(UCMD_CS_A), req );

	UCMD_CS_A * pA = (UCMD_CS_A *) & reply.msg.simData;

	* pA = * pQ;

	pA->callerChannel = pQ->callerChannel;
	pA->sessionInfo = pQ->sessionInfo;
	pA->statusCode = SIMS_OK;
	//pA->statusText[0] = '\0';

	bool bConnect = false;
	SIM_REQUEST con;

	uint32 nAppParam = pQ->extraInfo.caller.nAppParam;

	if( OnChannelRequesting( pQ->channelInfo.uid, pQ->statusCode, pQ->statusText, nAppParam ) ) {

		// bind a port
		PUDPSocket * pSock = NULL;
		WORD port;
		if( BindDynamicUdpPort( pSock, port ) ) {

			BBQChannelWait* pWait = new BBQChannelWait( pSock, *this );
			pWait->m_nAppCode = pQ->statusCode;
			pWait->m_strAppString = pQ->statusText;

			{
				LockIt sf( m_mutexChannelWait );
				pWait->extra.dwChannelId = ++ m_nNextChannelId;
			}
			pWait->extra.dwTimeBase = m_dwTimeBase;
			pWait->extra.nServerId = m_nServerId;
			pWait->extra.nAppParam = nAppParam;

			pWait->extra.use_firewall_here = 1;
			pWait->extra.firewall = m_nFirewallType & 0x3;

			pWait->peerExtra = pQ->extraInfo.caller;

			// init channel info
			UCMD_CHANNELINFO * info = & pWait->info;
			memset( info, 0, sizeof(*info) );

			info->uid = pPeerInfo->uid;
			info->cid = pPeerInfo->cid;
			info->tcp = pPeerInfo->tcp;
			info->give_me_proxy = m_bForceBBQProxy ? 1 : pPeerInfo->give_me_proxy;
			info->init = 0; // important flag, never change again
			info->firewall = m_nFirewallType & 0x3;

			PIPSocket::Address local_addr;
			GetHostAddress( local_addr );
			info->thisLAN.ip = local_addr;
			//info->thisLAN.ip = m_chanLogin.local.ip;
			info->thisLAN.port = port;

			info->peerLAN = pPeerInfo->thisLAN;
			info->peerWAN = pPeerInfo->thisWAN;

			if( pPeerInfo->invite ) {
				info->invite = 0;
				info->cookie = pPeerInfo->cookie;
			} else {
				info->invite = 1;
				info->cookie = (PTimer::Tick().GetInterval() * rand() * m_nNextChannelId * pPeerInfo->uid);
			}

			pA->channelInfo = * info;
			//pA->channelInfo.uid = m_sessionInfo.id;
			pA->extraInfo.called = pWait->extra;
			pA->extraInfo.caller = pWait->peerExtra;
			pA->statusCode = SIMS_OK;

			// we use the new port to response
			reply.channel.local.port = port;

			// insert into pWait list, but actually not pWait on it (because we are in the handler thread)
			{
				LockIt sf( m_mutexChannelWait );
				m_listChannelWaits.push_back( pWait );
			}

			SIM_REQINIT( con, 0, 0, port, 0, 0, UCM_CC_CON, sizeof(UCMD_CC_CON) );
			UCMD_CC_CON * pCon = (UCMD_CC_CON *) con.msg.simData;
			pCon->channelInfo = * info;
			pCon->channelInfo.uid = m_sessionInfo.id;
			pCon->statusCode = pWait->m_nAppCode;
			//strcpy( pCon->statusText, pQ->statusText );
			strncpy( pCon->statusText, pQ->statusText, SIM_APPSTRINGSIZEMAX ); pCon->statusText[ SIM_APPSTRINGSIZEMAX -1 ] = '\0';
			pCon->extraInfo.called = pWait->extra;
			pCon->extraInfo.caller = pWait->peerExtra;

			// Note: do not judge same ip any more, cause we have tried direct connect
			con.channel.peer = pPeerInfo->thisWAN;
			if( con.channel.peer.ip == LOCALHOST_IP ) {
				con.channel.peer.ip = req->channel.peer.ip;
			}

			if( info->give_me_proxy ) {
				// already ask for proxy, so do not connect directly
				bConnect = false;
			} else if( pPeerInfo->firewall == 3 ) { 
				// peer is strong firewall, connect will fail, 
				// but local NAT required this packet to open mapping port to peer host
				bConnect = true;
			} else {
				bConnect = true;
			}
		} else {
			pA->statusCode = SIMS_REFUSE;
		}

	} else {
		pA->statusCode = SIMS_REFUSE;
	}

#ifdef ANTI_PACKET_LOSS
	PostMessage( &reply, true );
	int n = ( reply.channel.socket ) ? 1 : 3;
	//for( int i=0; i<n; i++ ) { SendMessage( & reply ); sleep_ms(1); }
#else
	SendMessage( & reply );
#endif

	// if forced to use BBQProxy, then do not connect directly
	if( m_bForceBBQProxy ) bConnect = false;

	if( bConnect ) {
		// SendMessage( & con, true );
    for( int i=0; i<6; i++ ) {
      SendMessage(&con);PThread::Sleep(1);
    }
		// post instead of send, to avoid block of call back thread
		//PostMessage( & con, true, 500 ,10 ); 
	}

	return true;
}


bool BBQClient::OnMsgChannelRequestFeedback( const SIM_REQUEST * req )
{
	const UCMD_SC_A * pQ = (const UCMD_SC_A *) & req->msg.simData;
	const UCMD_CHANNELINFO * pPeerInfo = & pQ->channelInfo;

	// ignore if id not match, to avoid anonymous attack
	// if( pQ->sessionInfo.id != m_sessionInfo.id ) return true;

	bool bConnect = false;
	SIM_REQUEST con;
	UCMD_CHANNELINFO connectInfo, * info = NULL;
	memset( & connectInfo, 0, sizeof(connectInfo) );

	if( pQ->statusCode == SIMS_OK ) {
		// from my bind port, send connect message to the invited port
		LockIt safe( m_mutexChannelWait ); 
		for( cw_iterator iter = m_listChannelWaits.begin(), eiter = m_listChannelWaits.end(); iter != eiter; iter ++ ) {
			BBQChannelWait * pWait = * iter;
			info = & pWait->info;
			if( info->uid == pQ->channelInfo.uid && info->cid == pQ->channelInfo.cid ) {

				pWait->peerInfo = pQ->channelInfo;
				pWait->peerExtra = pQ->extraInfo.called;

				// copy useful info provided by peer & server
				if( info->cookie == 0 ) info->cookie = pQ->channelInfo.cookie;
				info->peerLAN = pQ->channelInfo.thisLAN;

				info->give_me_proxy = m_bForceBBQProxy ? 1 : pQ->channelInfo.give_me_proxy;

				// we only change it if not changed by peer connect msg, or else we ignore, cause connect msg is real source
				if( info->peerWAN.ip == 0 ) {
					info->peerWAN = pQ->channelInfo.thisWAN;
				}

				// send a connect message
				SIM_REQINIT( con, 0, 0, info->thisLAN.port, 0, 0, UCM_CC_CON, sizeof(UCMD_CC_CON) );
				UCMD_CC_CON * pCon = (UCMD_CC_CON *) & con.msg.simData;
				pCon->channelInfo = * info;
				pCon->channelInfo.uid = m_sessionInfo.id;

				// Note: do not judge same ip any more, cause we have tried direct connect
				con.channel.peer = pPeerInfo->thisWAN;

				if( con.channel.peer.ip == LOCALHOST_IP ) {
					con.channel.peer.ip = req->channel.peer.ip;
				}

				if( info->give_me_proxy ) {
					// already ask for proxy, so do not connect directly
					bConnect = false;
				} else if( pPeerInfo->firewall == 3 ) { 
					// peer is strong firewall, connect will fail, 
					// but local NAT required this packet to open mapping port to peer host
					bConnect = true;
				} else {
					bConnect = true;
				}

				if( bConnect ) connectInfo = * info;

				pCon->statusCode = pWait->m_nAppCode;
//				strcpy( pCon->statusText, pWait->m_strAppString );
				strncpy( pCon->statusText, pWait->m_strAppString, SIM_APPSTRINGSIZEMAX ); pCon->statusText[ SIM_APPSTRINGSIZEMAX -1 ] = '\0';

				break;
			}
		}

	} else {
		switch( pQ->statusCode ) {
		case SIMS_DENY:
			m_errLastError = LocalOffline;
			break;
		case SIMS_TIMEOUT:
			m_errLastError = PeerOffline;
			break;
		case SIMS_NOTFOUND:
			m_errLastError = PeerNotFound;
			break;
		case SIMS_FIREWALL:
			m_errLastError = BothStrictFirewall;
			break;
		case SIMS_REFUSE:
			m_errLastError = PeerDenied;
			break;
		default:
			m_errLastError = NetworkError;
		}

		LockIt safe( m_mutexChannelWait ); 
		for( cw_iterator iter = m_listChannelWaits.begin(), eiter = m_listChannelWaits.end(); iter != eiter; iter ++ ) {
			BBQChannelWait * pWait = * iter;
			UCMD_CHANNELINFO * info = & pWait->info;
			if( info->thisLAN.port == req->channel.local.port ) {
				pWait->m_bDone = false;
				pWait->Signal();
				//iter = m_listChannelWaits.erase( iter );
				STL_ERASE( m_listChannelWaits, cw_iterator, iter );
				PTRACE(1, "m_listChannelWaits Remove");
				break;
			}
			//iter ++;
		}
	}

	// if forced to use BBQProxy, then do not connect directly
	if( m_bForceBBQProxy ) bConnect = false;

	if( bConnect ) {
		// SendMessage( & con, true );
    for( int i=0; i<6; i++ ) {
      SendMessage(&con);PThread::Sleep(1);
    }
		// post instead of send, to avoid block of call back thread
		//PostMessage( & con, true ,500, 10); 
	}

	return true;
}

bool BBQClient::OnMsgChannelConnect( const SIM_REQUEST * req )
{
//	if( m_bForceTcp ) return true;
	if( m_bForceBBQProxy ) return true;

	const UCMD_CC_CON * pQ = (const UCMD_CC_CON *) & req->msg.simData;
	BBQ_CHANNEL * pChannel = NULL;
	int nAppCode = 0;
	PString strAppString;

	// send a confirm message
	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, UCM_CC_CONCFM, sizeof(UCMD_CC_CONCFM), req );
	UCMD_CC_CONCFM *pCfm = (UCMD_CC_CONCFM *) & reply.msg.simData;
	pCfm->channelInfo.uid = m_sessionInfo.id;
	pCfm->statusCode = pQ->statusCode;
	//strcpy( pCfm->statusText, pQ->statusText );
	strncpy( pCfm->statusText, pQ->statusText, SIM_APPSTRINGSIZEMAX ); pCfm->statusText[ SIM_APPSTRINGSIZEMAX -1 ] = '\0';

	// if i am the called one, find the wait object, and copy useful info provided by peer & server
	{
		LockIt sf( m_mutexChannelWait );
		for( cw_iterator iter = m_listChannelWaits.begin(), eiter = m_listChannelWaits.end(); iter != eiter; iter ++ ) {
			BBQChannelWait * pWait = * iter;
			UCMD_CHANNELINFO * info = & pWait->info;

			if( (info->init != pQ->channelInfo.init) &&
				(info->uid == pQ->channelInfo.uid) && 
				(info->cid == pQ->channelInfo.cid) && 
				(info->cookie == pQ->channelInfo.cookie) ) {

				info->peerWAN = req->channel.peer;
				info->peerLAN = pQ->channelInfo.thisLAN;

				// if already ask for proxy, then ignore direct connect
				if( info->give_me_proxy ) return true;

				pCfm->channelInfo = * info;
				pCfm->channelInfo.uid = m_sessionInfo.id;
			}
		}
	}

	if( SendMessage( & reply, true ,600, 8, false) ) {
    PTRACE(3, "client\t UCMD_CC_CONCFM ack OK");
		LockIt sf( m_mutexChannelWait );
		for( cw_iterator iter = m_listChannelWaits.begin(), eiter = m_listChannelWaits.end(); iter != eiter; /**/ ) {
			BBQChannelWait * pWait = * iter;
			UCMD_CHANNELINFO * info = & pWait->info;

			if( (info->init != pQ->channelInfo.init) &&
				(info->uid == pQ->channelInfo.uid) && 
				(info->cid == pQ->channelInfo.cid) && 
				(info->cookie == pQ->channelInfo.cookie) ) {

				STL_ERASE( m_listChannelWaits, cw_iterator, iter );
				//iter = m_listChannelWaits.erase( iter );

				if( pWait->info.init ) {
					pWait->m_bDone = true;
					pWait->Signal();
				} else {
					nAppCode = pWait->m_nAppCode;
					strAppString = pWait->m_strAppString;

					// move socket out of pWait object, before delete pWait object, 
					// or else socket will be delete in pWait object destructor
					pChannel = pWait->Detach();

					delete pWait;
				}

				break;
			} else {
				iter ++;
			}
		}
  }else{
    PTRACE(3, "client\t UCMD_CC_CONCFM ack failed");
  }
	if( pChannel ) {
		BBQCHANNEL_REMOTE_TRACE(pChannel);
		OnRequestedChannelSetup( pChannel, nAppCode, strAppString );
	}

	return true;
}

bool BBQClient::OnMsgChannelConnectConfirmed( const SIM_REQUEST * req )
{
//	if( m_bForceTcp ) return true;
	if( m_bForceBBQProxy ) return true;

	const UCMD_CC_CON * pQ = (const UCMD_CC_CON *) & req->msg.simData;
	BBQ_CHANNEL * pChannel = NULL;
	int nAppCode = 0;
	PString strAppString;

		// if i am the called one, find the wait object, and copy useful info provided by peer & server
	{
		LockIt sf( m_mutexChannelWait );
		for( cw_iterator iter = m_listChannelWaits.begin(), eiter = m_listChannelWaits.end(); iter != eiter; iter ++ ) {
			BBQChannelWait * pWait = * iter;
			UCMD_CHANNELINFO * info = & pWait->info;
			if( (info->init != pQ->channelInfo.init) &&
				(info->uid == pQ->channelInfo.uid) && 
				(info->cid == pQ->channelInfo.cid) && 
				(info->cookie == pQ->channelInfo.cookie) ) {

				info->peerWAN = req->channel.peer;
				info->peerLAN = pQ->channelInfo.thisLAN;

				//iter = m_listChannelWaits.erase( iter );
				STL_ERASE( m_listChannelWaits, cw_iterator, iter );

				if( pWait->info.init ) {
					pWait->m_bDone = true;
					pWait->Signal();
				} else {
					nAppCode = pWait->m_nAppCode;
					strAppString = pWait->m_strAppString;

					// move socket out of pWait object, before delete pWait object, 
					// or else socket will be delete in pWait object destructor
					pChannel = pWait->Detach();

					delete pWait;
				}

				break;
			}
		}
	}

	if( pChannel ) {
		BBQCHANNEL_REMOTE_TRACE(pChannel);

		OnRequestedChannelSetup( pChannel, nAppCode, strAppString );
	}

	return true;
}

bool BBQClient::UpdateUserData( BBQUserInfo & userInfo,const PString & strAliasExt)
{
	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		return false;
	}

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_UPDATEUSERDATA, 0 );

	PBytePack packedData, packedMsg;
	packedData << m_sessionInfo << userInfo;
	{
		const char* p =(const char*)strAliasExt;
		packedData << p;
	}
	req.msg.simHeader.size = packedData.Size();
	packedMsg << req.channel << req.msg.simHeader << packedData;

	SIM_REQUEST * reply = RequestMessage( (SIM_REQUEST *) packedMsg.Data(), SIM_SC_UPDATEUSERDATA, CLIENT_REQUEST_TIMEOUT );
	if( reply ) {
		uint32 * pA = (uint32 *) & reply->msg.simData;
		switch( * pA ) {
		case SIMS_OK:	
			bDone = true;
			break;
		case SIMS_DENY:
		default:
			m_errLastError = PeerDenied;
		}

		ReleaseMessage( reply );
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::UpdateMyTagString( const char * pStr )
{
	return UploadDataBlock( "@MyTagString@", "", strlen(pStr)+1, pStr );
}

bool BBQClient::UploadDataBlock( const char * key, const char * digest, uint32 data_len, const char* pData )
{
	if( ! pData ) return false;

	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		return false;
	}

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	bool bDone = false;

	int nMsgBodySize = sizeof(SIMD_DATABLOCK) + data_len;
	int nReqSize = SIM_DATASIZE_TO_REQSIZE( nMsgBodySize );

	SIM_REQUEST * req = NULL;
	SIM_REQUEST_NEW( req, nReqSize );
	if( ! req ) return false;

	SIM_REQINITCHAN( * req, m_chanLogin, SIM_CS_UPLOADDATABLOCK, nMsgBodySize );

	SIMD_DATABLOCK * pQ = (SIMD_DATABLOCK *) & req->msg.simData;
	memset( pQ, 0, nMsgBodySize );

	pQ->sessionInfo = m_sessionInfo;
	pQ->statusCode = SIMS_OK;
	strncpy( pQ->key, key, 64 );
	strncpy( pQ->digest, digest, 64 );
	memcpy( & pQ->data[0], pData, data_len );
	pQ->data_len = data_len;

	SIM_REQUEST * reply = RequestMessage( req, SIM_SC_UPLOADDATABLOCK, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_DATABLOCK * pA = (SIMD_DATABLOCK *) & reply->msg.simData;
		switch( pA->statusCode ) {
		case SIMS_OK:	
			bDone = true;
			break;
		case SIMS_DENY:
		default:
			m_errLastError = PeerDenied;
		}
		ReleaseMessage( reply );
	}

	SIM_REQUEST_DELETE( req );

	return bDone;
}

bool BBQClient::DownloadDataBlock( const char * key, const char * digest, uint32 & data_len, char* & pData )
{
	if( data_len != 0 ) return false;
	if( pData != NULL ) return false; // must be initialized to zero

	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		return false;
	}

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_DOWNLOADDATABLOCK, sizeof(SIMD_DATABLOCK) );
	SIMD_DATABLOCK * pQ = (SIMD_DATABLOCK *) & req.msg.simData;
	memset( pQ, 0, sizeof(SIMD_DATABLOCK) );

	pQ->sessionInfo = m_sessionInfo;
	pQ->statusCode = SIMS_OK;
	strncpy( pQ->key, key, 64 );
	strncpy( pQ->digest, digest, 64 );
	pQ->data_len = 0;
	SIMD_DATABLOCK * pA =NULL;
	SIM_MESSAGE_COMPR::simCompress ComprFlag;
	ComprFlag.data_compressed = 0;
#if 0
	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_DOWNLOADDATABLOCK, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
#else
	SIM_REQUEST * reply = NULL;
	if (!m_bSupportCompr)
	{
		reply = RequestMessage( & req, SIM_SC_DOWNLOADDATABLOCK, CLIENT_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
		if (reply)
		{
			pA = (SIMD_DATABLOCK *) & reply->msg.simData;
		}
	}
	else
	{
		req.msg.simHeader.id= SIM_CS_DOWNLOADDATABLOCK_COMPR;
		reply = RequestMessage( & req, SIM_SC_DOWNLOADDATABLOCK_COMPR, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );

		if (reply)
		{
			memcpy(&ComprFlag, reply->msg.simData, sizeof(SIM_MESSAGE_COMPR::simCompress));
			if (ComprFlag.data_compressed&& ComprFlag.data_compress_method == C_COMPRESS_BZIP2)
			{
				int nHeaderLen = sizeof(SIM_MESSAGE_COMPR::simCompress);
				int comprLen = reply->msg.simHeader.size -  nHeaderLen;
				PBYTEArray btUncompr;
				unsigned int uncomprLen=0;
				if (comprLen>0) 
				{
					if (BZ2_bzBuffToBuffDecompressEx(btUncompr , &uncomprLen ,((char*) reply->msg.simData)+ nHeaderLen, comprLen , 0, 0)== 0)
					{
						char* uncompr= (char*)btUncompr.GetPointer();
						//if the uncomprLen greater than  the length of reply, malloc the uncomprLen to use
						if (uncomprLen> sizeof(reply->msg.simData)-nHeaderLen ) 
						{
								int nReqSize = SIM_DATASIZE_TO_REQSIZE( uncomprLen+  nHeaderLen);
								SIM_REQUEST * pTemp = NULL;
								SIM_REQUEST_NEW( pTemp, nReqSize );
								if( ! pTemp ) BBQTRACE( 1, "No memory." );

								pTemp->channel= reply->channel;
								memcpy(&pTemp->msg, &reply->msg, sizeof(reply->msg.simHeader) + nHeaderLen);
								ReleaseMessage( reply );
								
								reply = pTemp;
						}
						
						//memcpy(&reply->msg.simData[sizeof(SIM_MESSAGE_COMPR::simCompress)]
						memcpy(&reply->msg.simData[0], uncompr, uncomprLen);
						pA = (SIMD_DATABLOCK *) & reply->msg.simData[0];
					}
					else
						BBQTRACE( 1, "An error occurred when uncompressed  the data." );

				}
				else
					BBQTRACE( 1, "Incorrect compressed data." );
			}
			else
				pA = (SIMD_DATABLOCK *) & reply->msg.simData[sizeof(SIM_MESSAGE_COMPR::simCompress)];
		}

	}
#endif
	if( reply && pA) {
		//SIMD_DATABLOCK * pA = (SIMD_DATABLOCK *) & reply->msg.simData;
		switch( pA->statusCode ) {
		case SIMS_OK:
			if( pA->data_len > 0 ) {
				pData = new char[ pA->data_len ];
				if( pData ) {
					memcpy( pData, & pA->data[0], pA->data_len );
					data_len = pA->data_len;
				}
			}
			bDone = true;
			break;
		case SIMS_DENY:
		default:
			m_errLastError = PeerDenied;
		}
		ReleaseMessage( reply );
	}

	return bDone;
}

bool BBQClient::QueryProxyTagString( PString & strTag, const PString & svraddr, WORD svrport )
{
	DWORD svrIp = HostnameToIp( svraddr );
	if( ! svrIp ) return false;

	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINIT( req, 0, 0, 0, svrIp, svrport, SIM_CS_DOWNLOADDATABLOCK, sizeof(SIMD_DATABLOCK) );
	SIMD_DATABLOCK * pQ = (SIMD_DATABLOCK *) & req.msg.simData;
	memset( pQ, 0, sizeof(SIMD_DATABLOCK) );

	pQ->sessionInfo = m_sessionInfo;
	pQ->statusCode = SIMS_OK;
	strncpy( pQ->key, "@ProxyTagString@", 64 );
	strncpy( pQ->digest, "", 64 );
	pQ->data_len = 0;

	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_DOWNLOADDATABLOCK, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_DATABLOCK * pA = (SIMD_DATABLOCK *) & reply->msg.simData;
		switch( pA->statusCode ) {
		case SIMS_OK:
			if( pA->data_len > 0 ) {
				strTag = pA->data;
			}
			bDone = true;
			break;
		case SIMS_DENY:
		default:
			m_errLastError = PeerDenied;
		}
		ReleaseMessage( reply );
	}

	return bDone;
}

void BBQClient::SetTechInfo( const BBQUserTechParamInfo * pInfo, uint32 nProviderID )
{ 
	m_techInfo.audio = pInfo->audio;
	m_techInfo.video = pInfo->video;
	m_techInfo.bandwidth = pInfo->bandwidth;
	m_techInfo.clienttype = pInfo->clienttype;
	m_techInfo.os = pInfo->os;
	m_techInfo.osver = pInfo->osver;
	m_techInfo.version = pInfo->version;

	m_nProviderID = nProviderID;
}

bool BBQClient::GetUserTechParam( BBQUserTechParamInfo & techInfo, uint32 uid )
{
	if( uid == 0 ) {
		techInfo = m_techInfo;
		return true;
	}

	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		return false;
	}

	SIM_REQUEST * reply = NULL;
	{
		SIM_REQUEST req;
		SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_GETUSERTECHPARAM, 0 );
		PBytePack packedData, packedMsg;
		packedData << m_sessionInfo << uid;
		req.msg.simHeader.size = packedData.Size();
		packedMsg << req.channel << req.msg.simHeader << packedData;

		reply = RequestMessage( (SIM_REQUEST *) packedMsg.Data(), SIM_SC_GETUSERTECHPARAM, CLIENT_REQUEST_TIMEOUT );
	}

	bool bDone = false;

	if( reply ) {
		try {
			PBytePack re( & reply->msg.simData, reply->msg.simHeader.size );
			uint32 statusCode, queryID;
			re >> statusCode >> queryID >> techInfo;
			switch( statusCode ) {
			case SIMS_OK:
				m_errLastError = Okay;
				bDone = true;
				break;
			case SIMS_DENY:
				m_errLastError = LocalOffline;
				break;
			case SIMS_NOTFOUND:
			default:
				m_errLastError = PeerNotFound;
				break;
			}
		} catch ( const char * ) {
			// bad response data
			m_errLastError = NetworkError;
		}

		ReleaseMessage( reply );

		return bDone;
	} else {
		m_errLastError = Timeout;
	}

	return false;
}

void BBQClient::ClearCache( void )
{
#ifdef CACHE_USERINFO
	m_userlistCache.RemoveAll();

	PFile::Remove( PFilePath( m_strCacheIndexFile ), TRUE );
	PFile::Remove( PFilePath( m_strCacheDataFile ), TRUE );
#endif
}

void BBQClient::SetCacheFilePath( PString strIndexFile, PString strDataFile )
{
#ifdef CACHE_USERINFO
	ClearCache();

	m_strCacheIndexFile = strIndexFile;
	m_strCacheDataFile = strDataFile;
#endif
}

bool BBQClient::ViewUserData( uint32 uid, BBQUserInfo & userInfo, bool bForceRefresh )
{
	VfonIdBindInfo bindInfo;
	PStringArray sipinfo;
	PString strAlias;
	return ViewUserDataEx( uid, userInfo, bindInfo, sipinfo,strAlias, bForceRefresh );
}

bool BBQClient::ViewUserDataEx( uint32 uid, BBQUserInfo & userInfo, VfonIdBindInfo & bindInfo,PStringArray & voipinfo, PString& strAlias,bool bForceRefresh )
{
  int n= sizeof(BBQUserInfo);
	if (!bForceRefresh)
	{
    std::map<uint32, BBQUserInfo>::iterator iFound= m_UserInfoCache.find(uid);
		if (iFound!= m_UserInfoCache.end())
		{
			memcpy(&userInfo  ,& iFound->second, sizeof(userInfo ));
			return true;
		}/*else
			return false;*/
	}
#ifdef CACHE_USERINFO
	if( ! bForceRefresh ) {
		// try find data in cache;
		int nIndex = m_userlistCache.Index( uid );
		if( nIndex >= 0 ) {
			bool bDone = false;
			FILE * fp = NULL;
			if( (fp = fopen(m_strCacheDataFile, "rb")) != NULL ) {
				if( (fseek( fp, sizeof(BBQUserInfo) * nIndex, SEEK_SET ) == 0) &&
					(fread( & userInfo, sizeof(BBQUserInfo), 1, fp) == 1) ) {
					// get from cache
					bDone = true;
				}
				fclose(fp);
			}
			if( bDone ) return true;
		}
	}
#endif

	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		return false;
	}

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	SIM_REQUEST * reply = NULL;
	{
		SIM_REQUEST req;
		SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_VIEWUSERDATA, 0 );
		PBytePack packedData, packedMsg;
		packedData << m_sessionInfo << uid;
		req.msg.simHeader.size = packedData.Size();
		packedMsg << req.channel << req.msg.simHeader << packedData;

		reply = RequestMessage( (SIM_REQUEST *) packedMsg.Data(), SIM_SC_VIEWUSERDATA, CLIENT_REQUEST_TIMEOUT );
	}

	bool bDone = false;

	if( reply ) {
		try {
			PBytePack re( & reply->msg.simData, reply->msg.simHeader.size );
			uint32 statusCode, queryID, nCount=0;
			re >> statusCode; 
const int D_PACKET_OLD_LEN_1 = sizeof(statusCode) + sizeof(queryID)+ sizeof(userInfo) +sizeof(bindInfo);
const int D_PACKET_OLD_LEN_2 = sizeof(statusCode) + sizeof(queryID)+ sizeof(userInfo) +sizeof(bindInfo)+sizeof(nCount) ;
			switch( statusCode ) {
			case SIMS_OK:
				{
				if (/*sizeof(statusCode) + sizeof(queryID)+ sizeof(userInfo) +sizeof(bindInfo)*/D_PACKET_OLD_LEN_1 == reply->msg.simHeader.size )
				{
					re >> queryID >> userInfo >> bindInfo ;//received from old server
					m_errLastError = Okay;
					bDone = true;

				}
				else
				{
					re >> queryID >> userInfo >> bindInfo >> nCount;
          if (queryID<2000 ){
            strncpy(userInfo.contact.address, "address name", sizeof(userInfo.contact.address));
            strncpy(userInfo.detail.school, "school name", sizeof(userInfo.detail.school));
          }
					m_errLastError = Okay;
					bDone = true;
					int nReadvoipinfo =0;
					for(int i=0; i< nCount;i++)
					{
						char* p =NULL;
						re >> p;
						if (p)	{voipinfo.AppendString( p );nReadvoipinfo +=(strlen(p)+1); delete p;/*malloc in bytepack*/ }
					}
					if (D_PACKET_OLD_LEN_2 + nReadvoipinfo< reply->msg.simHeader.size  && reply->msg.simHeader.size > D_PACKET_OLD_LEN_1)
					{
						//Read extern alias
						char* p =NULL;
						re >> p;
						if (p) {strAlias = p;delete p;}

					}
				}
				// Catch the personnal info 
        std::map<uint32, BBQUserInfo>::iterator iFound= m_UserInfoCache.find(uid) ;
				if (iFound==m_UserInfoCache.end())
					m_UserInfoCache.insert(make_pair( uid, userInfo));
				else
					memcpy(&iFound->second, &userInfo, sizeof(userInfo) );
#ifdef CACHE_USERINFO
				// TODO: update data in cache
				{
					m_userlistCache.Add( uid );
					FILE * fp = NULL;
					if( (fp = fopen(m_strCacheIndexFile, "wb")) != NULL ) {
						fseek( fp, 0, SEEK_SET );
						m_userlistCache >> fp;
						fclose(fp);
					}

					int nIndex = m_userlistCache.Index( uid );
					if( nIndex >= 0 ) {
						if( (fp = fopen(m_strCacheDataFile, "ab+")) != NULL ) {
							if( (fseek( fp, sizeof(BBQUserInfo) * nIndex, SEEK_SET ) == 0) &&
								(fwrite(& userInfo, sizeof(BBQUserInfo), 1, fp) == 1) ) {
								// cache updated
							}
							fclose(fp);
						}
					}
				}
#endif
				}
				break;
			case SIMS_DENY:
				m_errLastError = LocalOffline;
				break;
			case SIMS_NOTFOUND:
			default:
				m_errLastError = PeerNotFound;
				break;
			}
		} catch ( const char * ) {
			// bad response data
			m_errLastError = NetworkError;
		}

		ReleaseMessage( reply );

		return bDone;
	} else {
		m_errLastError = Timeout;
	}

	return false;
}


bool BBQClient::AddFriend( uint32 uid, const char * lpszText )
{
	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		return false;
	}

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_ADDCONTACT, 0 );

	PBytePack packedData, packedMsg;
	packedData << m_sessionInfo << uid;
	req.msg.simHeader.size = packedData.Size();
	packedMsg << req.channel << req.msg.simHeader << packedData;

	bool bDone = false;

	SIM_REQUEST * reply = RequestMessage( (SIM_REQUEST *) packedMsg.Data(), SIM_SC_ADDCONTACT, CLIENT_REQUEST_TIMEOUT );
	if( reply ) {
		try {
			PBytePack re( & reply->msg.simData, reply->msg.simHeader.size );
			uint32 statusCode, status;
			re >> statusCode;
			switch( statusCode ) {
			case SIMS_OK:
				{
					re >> status;
					{
						LockIt safe( m_mutexVars );
						for( of_iterator iter = m_listFriends.begin(), eiter = m_listFriends.end(); iter != eiter; iter ++ ) {
							ONLINE_USER * pUser = * iter;
							if( pUser->id == uid ) {
								delete pUser;
								m_listFriends.remove( pUser );
								break;
							}
						}
						{
							ONLINE_USER * pUser = new ONLINE_USER;
							pUser->id = uid;
							pUser->status = status;
							m_listFriends.push_back( pUser );
						}
					}
					m_errLastError = Okay;
					bDone = true;
				}
				break;
			case SIMS_DENY:
				m_errLastError = PeerDenied;
				break;
			case SIMS_NOTFOUND:
			default:
				m_errLastError = PeerNotFound;
				break;
			}
		} catch ( const char * ) {
			// bad response data
			m_errLastError = NetworkError;
		}

		ReleaseMessage( reply );

	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::RemoveFriend( uint32 uid )
{
	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		return false;
	}

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_DELETECONTACT, 0 );

	PBytePack packedData, packedMsg;
	packedData << m_sessionInfo << uid;
	req.msg.simHeader.size = packedData.Size();
	packedMsg << req.channel << req.msg.simHeader << packedData;

	bool bDone = false;

	SIM_REQUEST * reply = RequestMessage( (SIM_REQUEST *) packedMsg.Data(), SIM_SC_DELETECONTACT, CLIENT_REQUEST_TIMEOUT );
	if( reply ) {
		uint32 * pA = (uint32 *) & reply->msg.simData;
		bDone = ( *pA == SIMS_OK );

		if( bDone ) {
			LockIt safe( m_mutexVars );
			for( of_iterator iter = m_listFriends.begin(), eiter = m_listFriends.end(); iter != eiter; iter ++ ) {
				ONLINE_USER * pUser = * iter;
				if( pUser->id == uid ) {
					delete pUser;
					m_listFriends.remove( pUser );
					break;
				}
			}
			//m_userlistFriends.Remove( uid );
		}

		ReleaseMessage( reply );
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::GetFriendList( UserStatusList & thelist, bool bForceRefresh ) 
{
	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		return false;
	}

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	bool bDone = false;
	PBytePack* reUncompr=NULL;//for uncompress UserStatusList

	if( bForceRefresh ) {

		SIM_REQUEST * reply = NULL;
		if (!m_bSupportCompr)
		{
			SIM_REQUEST req;
			SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_GETCONTACTLIST, 0 );
			PBytePack packedData, packedMsg;
			packedData << m_sessionInfo;
			req.msg.simHeader.size = packedData.Size();
			packedMsg << req.channel << req.msg.simHeader << packedData;

			reply = RequestMessage( (SIM_REQUEST *) packedMsg.Data(), SIM_SC_GETCONTACTLIST, CLIENT_REQUEST_TIMEOUT );
		}else
		{
			SIM_REQUEST req;

			SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_GETCONTACTLIST_COMPR, 0  );
			PBytePack packedData, packedMsg;
			packedData << m_sessionInfo;
			req.msg.simHeader.size = packedData.Size();
			//SIM_MESSAGE_COMPR::simCompress flag;
			//flag.data_compress_method =C_COMPRESS_BZIP2;
			//flag.data_compressed =1;

			packedMsg << req.channel << req.msg.simHeader /*<< flag */ << packedData;

			reply = RequestMessage( (SIM_REQUEST *) packedMsg.Data(), SIM_SC_GETCONTACTLIST_COMPR, CLIENT_REQUEST_TIMEOUT );
		}

		if( reply ) {
			try {
				PBytePack re( & reply->msg.simData, reply->msg.simHeader.size );
				uint32 statusCode, nCount = 0, nID, status;
#ifdef D_COMPR				
				if (reply->msg.simHeader.id == SIM_SC_GETCONTACTLIST_COMPR)
				{
					SIM_MESSAGE_COMPR::simCompress comprFlags;
					re >> comprFlags;
					if (comprFlags.data_compressed &&comprFlags.data_compress_method == C_COMPRESS_BZIP2 )
					{//compressed data
						int nHeaderLen = sizeof(SIM_MESSAGE_COMPR::simCompress ) +  sizeof(statusCode)  +  sizeof(nCount);
						int comprLen = reply->msg.simHeader.size -  nHeaderLen;
						char uncompr[1024*8]={0};
						unsigned int uncomprLen= sizeof(uncompr);
						if (comprLen>0) 
						{
							if (BZ2_bzBuffToBuffDecompress(uncompr , &uncomprLen ,((char*)re.Data())+ nHeaderLen, comprLen , 0, 0)== 0)
							{
								reUncompr = new PBytePack();//(uncompr, uncomprLen);
								reUncompr->Pack(uncompr, uncomprLen);
							}
							else
								PTRACE( 1, "An error occurred when uncompressed  the data." );

						}
						else
							PTRACE( 1, "Incorrect compressed data." );

					}else
							;//It is orignal data , continue to do
				}
#endif
				re >> statusCode;
				switch( statusCode ) {
				case SIMS_OK:
					{
						m_errLastError = Okay;
						bDone = true;

						LockIt safe( m_mutexVars );
						while( m_listFriends.size() > 0 ) {
							ONLINE_USER * pUser = m_listFriends.front();
							m_listFriends.pop_front();
							delete pUser;
						}
						while( thelist.size() > 0 ) {
							ONLINE_USER * pUser = thelist.front();
							thelist.pop_front();
							delete pUser;
						}

						re >> nCount;
						for( uint32 i=0; i<nCount; i++ ) {
							if (!reUncompr)
								re >> nID >> status;
							else
								*reUncompr>> nID >> status;

							bool bFind = false;
							for( of_iterator iter = m_listFriends.begin(), eiter = m_listFriends.end(); iter != eiter; iter ++ ) {
								ONLINE_USER * pUser = * iter;
								if( pUser && pUser->id == nID ) {
									bFind = true;
									break;
								}
							}
							if( !bFind ) {
								ONLINE_USER * pUser = new ONLINE_USER;
								pUser->id = nID;
								pUser->status = status;
								m_listFriends.push_back( pUser );

								ONLINE_USER * pUserCopy = new ONLINE_USER;
								pUserCopy->id = nID;
								pUserCopy->status = status;
								thelist.push_back( pUserCopy );
							}
						}
					}
					break;
				case SIMS_COMPRESSERROR:
						m_errLastError = ComprMsgError;
					break;
				case SIMS_DENY:
				default:
					m_errLastError = LocalOffline;
					break;
				}
				//
				ReleaseMessage( reply );	
			} catch ( const char * ) {
				// bad response data
				m_errLastError = NetworkError;
			}
		} else {
			m_errLastError = Timeout;
		}
	} else {
		bDone = true;

		LockIt safe( m_mutexVars );
		for( of_iterator iter = m_listFriends.begin(), eiter = m_listFriends.end(); iter != eiter; iter ++ ) {
			ONLINE_USER * pUser = * iter;
			ONLINE_USER * pUserCopy = new ONLINE_USER;
			pUserCopy->id = pUser->id;
			pUserCopy->status = pUser->status;
			thelist.push_back( pUserCopy );
		}
	}
	if (reUncompr) delete reUncompr;

	return bDone;
}

bool BBQClient::IsFriendOnline( uint32 uid )
{
	//return m_userlistOnlineFriends.Contain( uid );
	LockIt safe( m_mutexVars );
	for( of_iterator iter = m_listFriends.begin(), eiter = m_listFriends.end(); iter != eiter; iter ++ ) {
		ONLINE_USER * pUser = * iter;
		if( pUser->id == uid ) {
			if( pUser->status == CLIENT_OFFLINE || pUser->status == CLIENT_INVISIBLE ) return false;
			else return true;
		}
	}
	return false;
}

bool BBQClient::AddBlock( uint32 uid )
{
	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		return false;
	}

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_ADDBLOCK, 0 );

	PBytePack packedData, packedMsg;
	packedData << m_sessionInfo << uid;
	req.msg.simHeader.size = packedData.Size();
	packedMsg << req.channel << req.msg.simHeader << packedData;

	bool bDone = false;

	SIM_REQUEST * reply = RequestMessage( (SIM_REQUEST *) packedMsg.Data(), SIM_SC_ADDBLOCK, CLIENT_REQUEST_TIMEOUT );
	if( reply ) {
		uint32 * pA = (uint32 *) & reply->msg.simData;
		bDone = ( *pA == SIMS_OK );

		if( bDone ) {
			m_userlistBlocks.Add( uid );
		}

		ReleaseMessage( reply );
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::RemoveBlock( uint32 uid )
{
	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		return false;
	}

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_DELETEBLOCK, 0 );

	PBytePack packedData, packedMsg;
	packedData << m_sessionInfo << uid;
	req.msg.simHeader.size = packedData.Size();
	packedMsg << req.channel << req.msg.simHeader << packedData;

	bool bDone = false;

	SIM_REQUEST * reply = RequestMessage( (SIM_REQUEST *) packedMsg.Data(), SIM_SC_DELETEBLOCK, CLIENT_REQUEST_TIMEOUT );
	if( reply ) {
		uint32 * pA = (uint32 *) & reply->msg.simData;
		bDone = ( *pA == SIMS_OK );

		if( bDone ) {
			m_userlistBlocks.Remove( uid );
		}

		ReleaseMessage( reply );
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::GetBlockList( UserIdList & idlist, bool bForceRefresh )
{
	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		return false;
	}

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	bool bDone = false;

	if( bForceRefresh ) {
		SIM_REQUEST * reply = NULL;
		{
			SIM_REQUEST req;
			SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_GETBLOCKLIST, 0 );
			PBytePack packedData, packedMsg;
			packedData << m_sessionInfo;
			req.msg.simHeader.size = packedData.Size();
			packedMsg << req.channel << req.msg.simHeader << packedData;

			reply = RequestMessage( (SIM_REQUEST *) packedMsg.Data(), SIM_SC_GETBLOCKLIST, CLIENT_REQUEST_TIMEOUT );
		}

		if( reply ) {
			try {
				PBytePack re( & reply->msg.simData, reply->msg.simHeader.size );
				uint32 statusCode, nCount=0, nID;
				re >> statusCode;
				switch( statusCode ) {
				case SIMS_OK:
					{
						m_errLastError = Okay;
						bDone = true;
						m_userlistBlocks.RemoveAll();

						re >> nCount;
						for( uint32 i=0; i<nCount; i++ ) {
							re >> nID;
							m_userlistBlocks.Add( nID );
						}
					}
					break;
				case SIMS_DENY:
				default:
					m_errLastError = LocalOffline;
					break;
				}
			} catch ( const char * ) {
				// bad response data
				m_errLastError = NetworkError;
			}

			ReleaseMessage( reply );
		} else {
			m_errLastError = Timeout;
		}
	} else bDone = true;

	idlist.ImportFrom( m_userlistBlocks.Data(), m_userlistBlocks.Count() );

	return bDone;
}


bool BBQClient::SendUserDefinedMessage( uint32 nEncryptMethod, uint32 nMeaning, uint32 nLength, const char * pData, uint32 uid, uint32 serverid, PIPSocket::Address svrIp, WORD svrPort)
{
	// data block too big
	if( sizeof(SFIDMSGHEADER) + sizeof(SIMD_USERMSG) + 64 + sizeof(time_t) + nLength > sizeof(SIM_MESSAGE) ) {
		m_errLastError = InvalidArgument;
		return false;
	}

	//if( ! m_bLogin ) {
	//	m_errLastError = LocalOffline;
	//	return false;
	//}

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	DWORD dwServerIp = (DWORD) svrIp;
	if( dwServerIp == 0 ) {
		if( ! m_bLogin ) {
			m_errLastError = LocalOffline;
			return false;
		}

		svrIp = m_chanLogin.peer.ip;
		svrPort = m_chanLogin.peer.port;

		dwServerIp = m_chanLogin.peer.ip;
	}

	SIM_CHANNEL chan;
	if( ! PickOrCreateCSChannel( chan, svrIp, svrPort ) ) return false;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, chan, SIM_CS_USERMSG, 0 );

	SIMD_USERMSG * pQ = (SIMD_USERMSG *) & req.msg.simData[0];
	memset( pQ, 0, sizeof(*pQ) );
	pQ->sessionInfo = m_sessionInfo;
	pQ->statusCode = SIMS_OK;
	pQ->fromId.userid = m_sessionInfo.id;
	pQ->fromId.serverid = m_nServerId;
	pQ->toId.userid = uid;
	pQ->toId.serverid = serverid;

	pQ->nMsgMeaning = nMeaning;
	pQ->nExtraLength = 0;

	pQ->flags.encrypt_method = BBQ_ENC_PLAIN;

	// now do encryption if required
	if( nEncryptMethod != BBQ_ENC_PLAIN ) {
		bool bDone = false;

		switch( nEncryptMethod ) {
		case BBQ_ENC_AES128:
			{
				char * pBuf = new char[ sizeof(time_t) + pQ->nEncryptedMsgLength ];
				if( pBuf ) {
					memcpy( & pBuf[ 0 ], pData, nLength );
					* (time_t *) & pBuf[ nLength ] = time(NULL);

					char iv[ 32 ]; // 128 bit
#if P_SSL
					RAND_bytes( (unsigned char *) iv, 32 );
#else
					for(int i=0; i<32; i++) iv[i]=rand() & 0xFF; 
#endif
					memcpy( & pQ->msgData[0], & iv[0], sizeof(char) * 32 );
					bDone = AES_encrypt( (unsigned char *) pBuf, nLength + sizeof(time_t), 
						(const unsigned char *) & m_lpszMyPassword[0], (const unsigned char *) & pQ->msgData[0], 128,
						(unsigned char *) & pQ->msgData[ 32 ] );
					pQ->nPlainMsgLength = 32;
					pQ->nEncryptedMsgLength = nLength + sizeof(time_t);
	
					delete[] pBuf;
				}
			}
			break;
		//case BBQ_ENC_AES256:
		//	{
		//		char iv[ 64 ]; // 256 bit
		//		for(int i=0; i<64; i++) iv[i]=rand() & 0xFF; 
		//		memset( & pQ->msgData[0], 0, sizeof(char) * 64 );
		//		bDone = Encrypt_AES256( pData, nLength, lpszMyPassword, &vi[0], & pQ->msgData[ 64 ] );
		//		pQ->nPlainMsgLength = 64;
		//		pQ->nEncryptedMsgLength = nLength;
		//	}
			break;
		}

		if( bDone ) {
			pQ->flags.encrypted = 1;
			pQ->flags.encrypt_method = nEncryptMethod & 0xFF;
		} else {
			pQ->flags.encrypt_method = BBQ_ENC_PLAIN;
		}
	}

	if( pQ->flags.encrypt_method == BBQ_ENC_PLAIN ) {
		memcpy( & pQ->msgData[0], pData, nLength ); 
		* (time_t *) (& pQ->msgData[ nLength ]) = time(NULL);
		pQ->nPlainMsgLength = nLength;
		pQ->nEncryptedMsgLength = 0;
	}

	req.msg.simHeader.size = sizeof(SIMD_USERMSG) + pQ->nPlainMsgLength + sizeof(time_t) + pQ->nEncryptedMsgLength + pQ->nExtraLength;
  ReleaseCSChannel(chan);
	return SendMessage( & req, true );
}


bool BBQClient::OnMsgUserDefinedMessage( const SIM_REQUEST * req )
{
	const SIMD_USERMSG * pQ = (const SIMD_USERMSG *) & req->msg.simData;

	if( req->msg.simHeader.size != sizeof(SIMD_USERMSG) + pQ->nPlainMsgLength + sizeof(time_t) + pQ->nEncryptedMsgLength + pQ->nExtraLength ) {
		return true;
	}

	SIM_REQUEST * f = NULL;
	SIM_REQUEST_CLONE( f, req );
	if( ! f ) return true;

	SIMD_USERMSG * pF = (SIMD_USERMSG *) & f->msg.simData;

	if( pQ->flags.encrypted ) { // if encrypted, then decrypt
		switch( pQ->flags.encrypt_method ) {
		case BBQ_ENC_AES128:
			if( (pQ->nPlainMsgLength == 32) && (pQ->nEncryptedMsgLength > sizeof(time_t) ) ) {

				bool bDone = AES_decrypt( (unsigned char *) & pQ->msgData[ pQ->nPlainMsgLength ], pQ->nEncryptedMsgLength, 
					(const unsigned char *) & m_lpszMyPassword[0], (const unsigned char *) & pQ->msgData[0], 128,
					(unsigned char *) & pF->msgData[ 0 ] );

				if( bDone ) {
					pF->nPlainMsgLength = pQ->nEncryptedMsgLength - sizeof(time_t);
					pF->nEncryptedMsgLength = 0;
					pF->flags.encrypt_method = BBQ_ENC_PLAIN;
				} else { // failed to decrypt data, invalid
					if( f ) SIM_REQUEST_DELETE( f );
					return true;
				}
			}
			break;
		}
	}

	time_t tServerTime = * (time_t *) & pF->msgData[ pQ->nPlainMsgLength ];
	OnUserDefinedMessageArrive( tServerTime, pF->nMsgMeaning, pF->nPlainMsgLength, & pF->msgData[0], pF->fromId, pF->fromAlias );

	if( f ) SIM_REQUEST_DELETE( f );
	return true;
}

bool BBQClient::ListOnlineUser( uint32 nStart, PBytePack & pack )
{
	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		return false;
	}

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_LISTONLINE, 0 );

	PBytePack packedData, packedMsg;
	packedData << m_sessionInfo << nStart;
	req.msg.simHeader.size = packedData.Size();
	packedMsg << req.channel << req.msg.simHeader << packedData;

	SIM_REQUEST * reply = RequestMessage( (SIM_REQUEST *) packedMsg.Data(), SIM_SC_LISTONLINE, CLIENT_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		bool bDone = false;
		try {
			PBytePack re( & reply->msg.simData, reply->msg.simHeader.size );
			pack << re;

			uint32 statusCode, nTotal, nStartIndex, nCount;
			re >> statusCode >> nTotal >> nStartIndex >> nCount;
			switch( statusCode ) {
			case SIMS_OK:
				bDone = true;
				break;
			case SIMS_DENY:
			default:
				m_errLastError = LocalOffline;
				break;
			}
		} catch ( const char * ) {
			m_errLastError = NetworkError;
		}

		ReleaseMessage( reply );
		return bDone;
	} else {
		m_errLastError = Timeout;
	}

	return false;
}

bool BBQClient::ListOnlineUser( uint32 & nStart, uint32 & nTotal, uint32 & nCount, BBQUserBriefRecord* & pRecs )
{
	if( pRecs ) { 
		delete[] pRecs;
		pRecs = NULL;
	}

	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		return false;
	}

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_LISTONLINE, 0 );

	PBytePack packedData, packedMsg;
	packedData << m_sessionInfo << nStart;
	req.msg.simHeader.size = packedData.Size();
	packedMsg << req.channel << req.msg.simHeader << packedData;

	SIM_REQUEST * reply = RequestMessage( (SIM_REQUEST *) packedMsg.Data(), SIM_SC_LISTONLINE, CLIENT_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		bool bDone = false;
		try {
			PBytePack re( & reply->msg.simData, reply->msg.simHeader.size );
			uint32 statusCode;
			re >> statusCode; 
			switch( statusCode ) {
			case SIMS_OK:
				re >> nTotal >> nStart >> nCount;
				pRecs = new BBQUserBriefRecord[ nCount ];
				if( pRecs ) {
					for( int i=0; i<(int)nCount; i++ ) {
						re >> pRecs[i];
					}
				}
				nStart += nCount;
				bDone = true;
				break;
			case SIMS_DENY:
			default:
				nCount = 0;
				m_errLastError = LocalOffline;
				break;
			}
		} catch ( const char * ) {
			m_errLastError = NetworkError;
		}

		ReleaseMessage( reply );
		return bDone;
	} else {
		m_errLastError = Timeout;
	}

	return false;
}

bool BBQClient::SearchUser( const BBQSearchCondition * pFilter, uint32 nStart, PBytePack & pack )
{
	return false;
}

bool BBQClient::SearchUser( const BBQSearchCondition * pFilter, uint32 & nStart, uint32 & nTotal, uint32 & nCount, BBQUserBriefRecord* & pRecs )
{
	return false;
}

bool BBQClient::OnMsgTextMessage( const SIM_REQUEST * req )
{
	// check the msg source for security reason
	// if( req->channel.peer.ip != m_chanLogin.peer.ip ) return true;

	const char * lpszText = (const char *) & req->msg.simData;
	if( strlen( lpszText ) <= TEXT_MESSAGE_MAX_LENGTH ) {

#if PTRACING
		PString str( lpszText );
		PTRACE( 1, "\nText Message, msg id: " << req->msg.simHeader.id << "\n" << str << "\n" );
#endif

		OnTextMessage( req->msg.simHeader.id, lpszText );
	}

	return true;
}

bool BBQClient::QueryServiceInfo( BBQACLFlag & aclFlag, BBQUserServiceInfo & serviceInfo )
{
	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		return false;
	}

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_QUERYSERVICE, 0 );

	PBytePack packedData, packedMsg;
	packedData << m_sessionInfo;
	req.msg.simHeader.size = packedData.Size();
	packedMsg << req.channel << req.msg.simHeader << packedData;

	SIM_REQUEST * reply = RequestMessage( (SIM_REQUEST *) packedMsg.Data(), SIM_SC_QUERYSERVICE, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		bool bDone = false;
		try {
			PBytePack re( & reply->msg.simData, reply->msg.simHeader.size );

			uint32 statusCode;
			re >> statusCode;
			switch( statusCode ) {
			case SIMS_OK:
				re >> aclFlag >> serviceInfo;
				bDone = true;
				break;
			case SIMS_DENY:
			default:
				m_errLastError = LocalOffline;
				break;
			}
		} catch ( const char * ) {
			m_errLastError = NetworkError;
		}

		ReleaseMessage( reply );
		return bDone;
	} else {
		m_errLastError = Timeout;
	}

	return false;
}


bool BBQClient::OnMsgServiceChanged( const SIM_REQUEST * req )
{
	uint32 statusCode = 0;
	BBQACLFlag aclFlag;
	BBQUserServiceInfo serviceInfo;
	BBQACLFLAG_SET( aclFlag, BBQACLFLAG_DEFAULT );
	memset( & serviceInfo, 0, sizeof(serviceInfo) );

	try {
		PBytePack packedData( & req->msg.simData, req->msg.simHeader.size );
		packedData >> statusCode >> aclFlag >> serviceInfo;
	} catch ( const char * ) {
		// bad request
		return true;
	}

	if( statusCode == SIMS_OK ) {
		m_accessFlags = aclFlag;
		OnServiceChanged( & aclFlag, & serviceInfo );
	}

	return true;
}

bool BBQClient::GetGKServerList(GKServerList& lstServer)
{
	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	lstServer.clear();

	SIM_REQUEST * reply = NULL;
	{
		SIM_REQUEST req;
		SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_QUERYGKLIST, 0 );
		reply = RequestMessage( & req, SIM_SC_QUERYGKLIST, CLIENT_SIMPLE_REQUEST_TIMEOUT  + m_nNetworkBaseDelay * 2);
	}

	bool bDone = false;

	if( reply ) {
		try {
			PBytePack re( & reply->msg.simData, reply->msg.simHeader.size );
			uint32 nCount;
			re >> nCount;
			IPSOCKADDR ipAddr;
			for(int i = 0; i< (int)nCount; i++) {
				re.Unpack(&ipAddr, sizeof(ipAddr));
				if( ipAddr.ip != 0 )
					lstServer.push_back(ipAddr);
			}
			bDone = true;
		} catch ( const char * ) {
			// bad response data
			m_errLastError = NetworkError;
		}

		ReleaseMessage( reply );

	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::GetProxyList(ProxyServerList& lstServer)
{
	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	lstServer.clear();

	SIM_REQUEST * reply = NULL;
	{
		SIM_REQUEST req;
		SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_QUERYPSLIST, 0 );
		reply = RequestMessage( & req, SIM_SC_QUERYPSLIST, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	}

	bool bDone = false;
	if( reply ) {
		try {
			PBytePack re( & reply->msg.simData, reply->msg.simHeader.size );
			uint32 nCount;
			re >> nCount;
			IPSOCKADDR ipAddr;
			for(int i = 0; i< (int)nCount; i++) {
				re.Unpack(&ipAddr, sizeof(ipAddr));
				if( ipAddr.ip != 0 )
					lstServer.push_back(ipAddr);
			}
			bDone = true;
		} catch ( const char * ) {
			// bad response data
			m_errLastError = NetworkError;
		}

		ReleaseMessage( reply );

	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

// called when the connetion is closing (by local) or closed (by peer)
void BBQClient::OnMsgConnectionClose( BBQMsgConnection * pConn )
{
	// TODO: 
	// we must known why connection is closed.
	// because lost connection will make client hangup current videoconference.

	if( m_chanLogin.socket == pConn->GetSocket()->GetHandle() ) {
		PTRACE( 1, "OnMsgConnectionClose() called, set login status as Offline." );

		m_nHeartbeatCounter = m_nHeartbeatTimeInterval;
//		m_bLogin = false;
//		m_nLoginSockFd = 0;
//		BBQACLFLAG_SET( m_accessFlags, BBQACLFLAG_DEFAULT );

		// OnSessionExpired();
		// do not call this to avoid dead lock,
		// send heartbeat will call it.
	}
}

bool BBQClient::OnMsgCreateProxyChannel( const SIM_REQUEST * req )
{
	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, UCM_PS_NOTIFY, sizeof(UCMD_PROXY_NOTIFY), req );

	const UCMD_PROXY_CREATE * pQ = (const UCMD_PROXY_CREATE *) & req->msg.simData;
	UCMD_PROXY_NOTIFY * pA = (UCMD_PROXY_NOTIFY *) & reply.msg.simData;

	// here, check session info for security
	// if( (pQ->sessionInfo.id != m_sessionInfo.id) || (pQ->sessionInfo.cookie != m_sessionInfo.cookie) ) return true;

	* pA = * pQ;

	BBQProxy::Entry * pEA = NULL, * pEB = NULL;
	BBQProxy * pProxy = BBQProxy::GetCurrentProxy();
	if( pProxy ) {
		if( pProxy->CreateChannel( pQ->entry[0].uid, pQ->entry[1].uid, (pQ->channelInfo.tcp != 0), pEA, pEB ) && pEA && pEB ) {
			pA->statusCode = SIMS_OK;

			pA->proxyMax = pProxy->m_config.nChannelMax;
			pA->proxySlot = pA->proxyMax - pProxy->m_status.nEntries/2;

			pA->proxyBandwidthTotal = pProxy->m_config.nBandwidthMax;
			pA->proxyBandwidthUsed = pProxy->m_status.nAverageBandwidth;

			pA->nChannelId = pEA->m_nChannelId;
			pA->cookieProxy = pEA->m_cookie;
			pA->entry[0].addr.ip = pA->entry[1].addr.ip = pProxy->m_config.dwExternalIp;
			//pA->entry[0].addr.ip = pA->entry[1].addr.ip = 0;
			if( pQ->channelInfo.tcp ) {
				pA->entry[0].addr.port = pA->entry[1].addr.port = BBQProxy::m_config.nTcpPort;
			} else {
				PIPSocket::Address addr; WORD port;
				switch( pEA->m_nProxyType ) {
					case BBQProxy::PROXY_NONE:
						break;
					case BBQProxy::PROXY_UDP:
						if( pEA->m_pUdpSock ) {
							pEA->m_pUdpSock->GetLocalAddress( addr, port );
							pA->entry[0].addr.port = port;
						}
						break;
					case BBQProxy::PROXY_TCP:
					case BBQProxy::PROXY_SSL:
						if( pEA->m_pTcpSock ) {
							pEA->m_pTcpSock->GetLocalAddress( addr, port );
							pA->entry[0].addr.port = port;
						}
						break;
				}
				switch( pEB->m_nProxyType ) {
					case BBQProxy::PROXY_NONE:
						break;
					case BBQProxy::PROXY_UDP:
						if( pEB->m_pUdpSock ) {
							pEB->m_pUdpSock->GetLocalAddress( addr, port );
							pA->entry[1].addr.port = port;
						}
						break;
					case BBQProxy::PROXY_TCP:
					case BBQProxy::PROXY_SSL:
						if( pEB->m_pTcpSock ) {
							pEB->m_pTcpSock->GetLocalAddress( addr, port );
							pA->entry[1].addr.port = port;
						}
						break;
				}
				//if( pEA->m_pSock ) {
				//	pEA->m_pSock->GetLocalAddress( addr, port );
				//	pA->entry[0].addr.port = port;
				//}
				//if( pEB->m_pSock ) {
				//	pEB->m_pSock->GetLocalAddress( addr, port );
				//	pA->entry[1].addr.port = port;
				//}
			}
		} else {
#if PTRACING
			PString str( PString::Printf, "Proxy create channel failed for %s (%d, %d).", 
				((pQ->channelInfo.tcp != 0) ? "TCP" : "UDP"), pQ->entry[0].uid, pQ->entry[1].uid );
			PTRACE( 1, str );
#endif
			pA->statusCode = SIMS_NOTAVAILABLE;
		}
	} else {
#if PTRACING
		PTRACE( 1, "Proxy not started. Create channel request ignored." );
#endif
		pA->statusCode = SIMS_NOTAVAILABLE;
	}

	SendMessage( & reply );

	return true;
}

bool BBQClient::OnMsgReleaseProxyChannel( const SIM_REQUEST * req )
{
	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, UCM_PS_NOTIFY, sizeof(UCMD_PROXY_NOTIFY), req );

	const UCMD_PROXY_RELEASE * pQ = (const UCMD_PROXY_RELEASE *) & req->msg.simData;
	UCMD_PROXY_NOTIFY * pA = (UCMD_PROXY_NOTIFY *) & reply.msg.simData;

	// here, check session info for security
	// if( (pQ->sessionInfo.id != m_sessionInfo.id) || (pQ->sessionInfo.cookie != m_sessionInfo.cookie) ) return true;

	* pA = * pQ;

	BBQProxy * pProxy = BBQProxy::GetCurrentProxy();
	if( pProxy && pProxy->CutChannel( pQ->nChannelId ) ) {
		pA->statusCode = SIMS_OK;
	} else {
		pA->statusCode = SIMS_NOTFOUND;
	}

	SendMessage( & reply );

	return true;
}

PTCPSocket * BBQClient::ConnectTcpProxyChannel( const SIM_REQUEST * req, PSSLChannel * & pSSLChannel  )
{
	const UCMD_PROXY_NOTIFYCLIENT * info = (const UCMD_PROXY_NOTIFYCLIENT *) & req->msg.simData;

	PTCPSocket * tcpSock = new PTCPSocket();
	if( ! tcpSock ) return NULL;
    const int one = 1;
    if (setsockopt(tcpSock->GetHandle(), IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one)))
			{
        PTRACE(3, "setocket failed");
				//return VFalse;
			}
	uint32 uidPeer;
	IPSOCKADDR addrProxy;
	if( info->channelInfo.init ) {
		addrProxy = info->entry[0].addr;
		if( (addrProxy.ip == 0) || (addrProxy.ip == LOCALHOST_IP) ) addrProxy.ip = req->channel.peer.ip;
		uidPeer = info->entry[1].uid;
	} else {
		addrProxy = info->entry[1].addr;
		if( (addrProxy.ip == 0) || (addrProxy.ip == LOCALHOST_IP) ) addrProxy.ip = req->channel.peer.ip;
		uidPeer = info->entry[0].uid;
	}

	tcpSock->SetPort( addrProxy.port );
#if PTRACING
	PString str( PString::Printf, "Connecting to proxy %s:%d ...", IP2STRING( addrProxy.ip ), addrProxy.port );
	PTRACE( 1, str );
#endif
	if( tcpSock->Connect( addrProxy.ip ? ((const PString&)(PIPSocket::Address) addrProxy.ip) : m_strServerHostname) ) {
#if PTRACING
		PTRACE( 1, "Connected." );
#endif

		// client will send msg with header, but must all servers upgrade to new for compartibility.
		// TODO: change it to true, in future when almost all servers have been upgrades
		bool bWithHeader = true;

		SIM_MESSAGE msg;
		msg.simHeader.magic = SIM_MAGIC;
		msg.simHeader.id = UCM_CP_CONNECT;
		msg.simHeader.size = sizeof(BBQPROXY_CONNECT);
		msg.simHeader.sync = 0;

		BBQPROXY_CONNECT * pData = (BBQPROXY_CONNECT *) & msg.simData;
		memset( pData, 0, sizeof(*pData) );
		pData->nChannelId = info->nChannelId;
		pData->cookie = info->cookieProxy;
		pData->uid = m_sessionInfo.id;
		pData->uidPeer = uidPeer;
		pData->useSSL = m_bChannelSSL ? 1 : 0;
    pData->emChannelType = em_DataChannelForConference;
		void * p = bWithHeader ? ((void *)(& msg)) : ((void *)pData);
		int nBytes = bWithHeader ? (sizeof(msg.simHeader)+sizeof(BBQPROXY_CONNECT)) : (sizeof(BBQPROXY_CONNECT));

		if( tcpSock->Write( p, nBytes ) && (tcpSock->GetLastWriteCount() == nBytes) ) {
			int fd = tcpSock->GetHandle();
			fd_set fdsetRead;
			FD_ZERO( & fdsetRead );
			FD_SET( fd, & fdsetRead );
			struct timeval tval = { 5, 0 };
      PTRACE(1, "select start" << PTime());
			int s = select( fd +1, & fdsetRead, NULL, NULL, &tval);
			//int s = tcpSock->Select( * tcpSock, PTimeInterval(0,3) );
      PTRACE(1, "select end" << PTime());
			if( s > 0 ) {
				memset( p, 0, nBytes );
				if( tcpSock->Read( p, nBytes ) && (tcpSock->GetLastReadCount() == nBytes) && (pData->statusCode == SIMS_OK) ) {
					if( pData->useSSL ) {
						//pSSLChannel = new PSSLChannel(true, * tcpSock);
					}
					return tcpSock;
				} else {
					PTRACE(1, "Status:" << pData->statusCode <<",ReadCount=" <<tcpSock->GetLastReadCount());
				}
			}
		}
	} 

#if PTRACING
	PTRACE( 1, "TCP Proxy Write Data or Read Data Error." );
#endif
	
	if( tcpSock ) delete tcpSock;

	return NULL;
}

PUDPSocket * BBQClient::ConnectUdpProxyChannel( const SIM_REQUEST * req )
{
	const UCMD_PROXY_NOTIFYCLIENT * info = (const UCMD_PROXY_NOTIFYCLIENT *) & req->msg.simData;

	uint32 uidPeer;
	IPSOCKADDR addrProxy;
	PUDPSocket * udpSock = NULL; 
	WORD port;
	if( info->channelInfo.init ) {
		addrProxy = info->entry[0].addr;
		if( (addrProxy.ip == 0) || (addrProxy.ip == LOCALHOST_IP) ) addrProxy.ip = req->channel.peer.ip;
		uidPeer = info->entry[1].uid;
		//port = info->channelInfo.thisLAN.port;
		//udpSock = FindUdpListener( port );
	} else {
		addrProxy = info->entry[1].addr;
		if( (addrProxy.ip == 0) || (addrProxy.ip == LOCALHOST_IP) ) addrProxy.ip = req->channel.peer.ip;
		uidPeer = info->entry[0].uid;
		//port = info->channelInfo.peerLAN.port;
		//udpSock = FindUdpListener( port );
	}

	if( BindDynamicUdpPort( udpSock, port ) && udpSock ) {
		DetachUdpListener( udpSock );

		// client will send msg with header, but must all servers upgrade to new for compartibility.
		// TODO: change it to true, in future when almost all servers have been upgrades
		bool bWithHeader = true;

		SIM_MESSAGE msg;
		msg.simHeader.magic = SIM_MAGIC;
		msg.simHeader.id = UCM_CP_CONNECT;
		msg.simHeader.size = sizeof(BBQPROXY_CONNECT);
		msg.simHeader.sync = 0;

		BBQPROXY_CONNECT * pData = (BBQPROXY_CONNECT *) & msg.simData;
		memset( pData, 0, sizeof(*pData) );
		pData->nChannelId = info->nChannelId;
		pData->cookie = info->cookieProxy;
		pData->uid = m_sessionInfo.id;
		pData->uidPeer = uidPeer;
		pData->useSSL = m_bChannelSSL ? 1 : 0;

		void * p = bWithHeader ? ((void *)(& msg)) : ((void *)pData);
		int nBytes = bWithHeader ? (sizeof(msg.simHeader)+sizeof(BBQPROXY_CONNECT)) : (sizeof(BBQPROXY_CONNECT));
		int nTimes = bWithHeader ? 5 : 1;

		for( int i=0; i<nTimes; i++ ) {
			udpSock->WriteTo( p, nBytes, addrProxy.ip, addrProxy.port );
			sleep_ms(1);
		}

		int fd = udpSock->GetHandle();
		fd_set fdsetRead;
		FD_ZERO( & fdsetRead );
		FD_SET( fd, & fdsetRead );
		struct timeval tval = { 5, 0 };
          PTRACE(1, "select start" << PTime());

		int s = select( fd +1, & fdsetRead, NULL, NULL, &tval);
		//int s = udpSock->Select( * udpSock, PTimeInterval(0,3) );
          PTRACE(1, "select end" << PTime());

		if( s > 0 ) {
			memset( p, 0, nBytes );
			PIPSocket::Address addr;
			if( udpSock->ReadFrom( p, nBytes, addr, port ))
			{
				if (udpSock->GetLastReadCount() == nBytes) 
				{
					if (pData->statusCode == SIMS_OK  ) 
					{
						return udpSock;
					}else
						PTRACE(1,"status code is  "<< pData->statusCode );
				}else
					PTRACE(1,"udpSock->GetLastReadCount()"<< udpSock->GetLastReadCount() );
			}else
				PTRACE(1,"udpSock->ReadFrom() is false" );
		}
		else
			PTRACE(1,"select return "<< s<<", the error code is "<< udpSock->GetErrorCode() );
		delete udpSock;
	}

#if PTRACING
	PTRACE( 1, "UDP Proxy Write Data or Read Data Error." );
#endif
		
	return NULL;
}

bool BBQClient::OnMsgBBQProxyNotify( const SIM_REQUEST * req )
{
	PTCPSocket * tcpSock = NULL;
	PUDPSocket * udpSock = NULL;
	PSSLChannel * sslChannel = NULL;

	const UCMD_PROXY_NOTIFYCLIENT * pQ = (const UCMD_PROXY_NOTIFYCLIENT *) & req->msg.simData;

	uint32 uidPeer;
	IPSOCKADDR addrProxy;
	if( pQ->channelInfo.init ) {
		addrProxy = pQ->entry[0].addr;
		if( (addrProxy.ip == 0) || (addrProxy.ip == LOCALHOST_IP) ) addrProxy.ip = req->channel.peer.ip;
		uidPeer = pQ->entry[1].uid;
	} else {
		addrProxy = pQ->entry[1].addr;
		if( (addrProxy.ip == 0) || (addrProxy.ip == LOCALHOST_IP) ) addrProxy.ip = req->channel.peer.ip;
		uidPeer = pQ->entry[0].uid;
	}

	// check our pWait list before connect, to avoid time waste
	{
		LockIt sf( m_mutexChannelWait );

		bool bFound = false;
		for( cw_iterator iter = m_listChannelWaits.begin(), eiter = m_listChannelWaits.end(); iter != eiter; iter ++ ) {
			BBQChannelWait * pWait = * iter;
			UCMD_CHANNELINFO * info = & pWait->info;
			if( (info->uid == uidPeer) && 
				(info->init == pQ->channelInfo.init) &&
				(info->cid == pQ->channelInfo.cid) && 
				(info->cookie == pQ->channelInfo.cookie) ) {

				if( pWait->m_bConnectingProxy ) return true;
				else pWait->m_bConnectingProxy = true;

				PTRACE( 1, PString(PString::Printf, "proxy notify: %u, %u, %u, %u. proxy cookie: %u", info->uid, info->init, info->cid, info->cookie, pQ->cookieProxy ) );

				bFound = true;
				break;
			}
		}
		if( ! bFound ) return true;
	}

	// now connect to proxy with given info
	if( pQ->channelInfo.tcp ) {
		// connect to the ip:port, send data, and pWait for response
		tcpSock = ConnectTcpProxyChannel( req, sslChannel );
	} else {
		// send UDP data to the ip:port, pWait for response
		udpSock = ConnectUdpProxyChannel( req );
	}

	bool bConnected = tcpSock || udpSock;

#if PTRACING
	PString str( PString::Printf, "Connecting to proxy channel: \n\ttype: %s\n\tchannel id (%d), \n\tfrom %d to %d, \n\tproxy address (%s:%d)\n\tproxy cookie (%d)\n\t%s.", 
		(pQ->channelInfo.tcp ? "TCP" : "UDP"),
		pQ->nChannelId,
		pQ->entry[0].uid, pQ->entry[1].uid, 
		IP2STRING( addrProxy.ip ), addrProxy.port,
		pQ->cookieProxy, 
		(bConnected ? "Connected" : "Failed")
		);
	PTRACE( 1, str );
#endif

	if( ! bConnected ) return true;

	BBQ_CHANNEL * pChannel = NULL;
	int nAppCode = 0;
	PString strAppString;
	bool bFound = false;

	m_mutexChannelWait.Wait();
	for( cw_iterator iter = m_listChannelWaits.begin(), eiter = m_listChannelWaits.end(); iter != eiter; iter ++ ) {
		BBQChannelWait * pWait = * iter;
		UCMD_CHANNELINFO * info = & pWait->info;

		if( (info->uid == uidPeer) && 
			(info->init == pQ->channelInfo.init) &&
			(info->cid == pQ->channelInfo.cid) && 
			(info->cookie == pQ->channelInfo.cookie) ) {

			PString strMsg( PString::Printf, "uid: %u, cid:%u, cookie: %u", info->uid, info->cid, info->cookie );
			PTRACE( 1, strMsg );

			bFound = true;
			// copy useful info provided by peer & server
			info->peerWAN = info->peerLAN = addrProxy;

			if( tcpSock || udpSock ) { // always true here

				//iter = m_listChannelWaits.erase( iter );
				STL_ERASE( m_listChannelWaits, cw_iterator, iter );

				// --- assign the socket to pWait object ---
				pWait->m_bIsProxied = true;

				if( pWait->m_pSock != udpSock ) {
					if( pWait->m_pSock ) {
						if( DetachUdpListener( pWait->m_pSock ) ) delete pWait->m_pSock;
						pWait->m_pSock = NULL;
					}

					pWait->m_pSock = udpSock;

					if( udpSock ) {
						PIPSocket::Address addr;
						WORD port;
						udpSock->GetLocalAddress( addr, port );
						pWait->info.thisLAN.port = port;
					}
				}

				if( pWait->m_pTcpSock != tcpSock ) {
					if( pWait->m_pTcpSock ) {
						// it's impossible
						delete pWait->m_pTcpSock;
						pWait->m_pTcpSock = NULL;
					}

					pWait->m_pTcpSock = tcpSock;

					pWait->m_pSSLChannel = sslChannel;

					PIPSocket::Address addr;
					WORD port;

					tcpSock->GetPeerAddress( addr, port );
					pWait->info.peerWAN.ip = (DWORD) addr;
					pWait->info.peerWAN.port = port;
					pWait->info.peerLAN = pWait->info.peerWAN;

					tcpSock->GetLocalAddress( addr, port );
					pWait->info.thisLAN.ip = (DWORD) addr;
					pWait->info.thisLAN.port = port;
				}
				// --- socket in pWait object is changed to new one already ---

				if( pWait->info.init ) { 
					// this channel is requested from local by another thread, 
					// now notify him to resume and retrieve this channel
					pWait->m_bDone = true;
					pWait->Signal();
				} else {
					nAppCode = pWait->m_nAppCode;
					strAppString = pWait->m_strAppString;

					// move socket out of pWait object, before delete pWait object, 
					// or else socket will be delete in pWait object destructor
					pChannel = pWait->Detach();
					delete pWait;
				}
			}

			break;
		}
	}
	m_mutexChannelWait.Signal();

	if( pChannel ) {
		BBQCHANNEL_REMOTE_TRACE(pChannel);
   
  //chen yuan
  if (nAppCode == CLIENT_REQUEST_CHANNEL_FOR_TESTING_PROXY_MARK)
  {
    OnRequestedChannelSetupForTestingProxy(pChannel, nAppCode, strAppString);
  }else
    OnRequestedChannelSetup( pChannel, nAppCode, strAppString );
	}

	if( ! bFound ) {
		if( udpSock ) delete udpSock;
		if( tcpSock ) delete tcpSock;
		if( sslChannel ) delete sslChannel;
	}

	return true;
}

bool BBQClient::OnMsgConferenceChangeByServer( const SIM_REQUEST * req )
{
	const SIMD_SC_CONFERENCE * pQ = (const SIMD_SC_CONFERENCE *) & req->msg.simData;

	// invalid message, ignore.
	//if( 0 != memcmp( & pQ->sessionInfo, & m_sessionInfo, sizeof(m_sessionInfo) ) ) return true;

	// call the overridable virtual function, the application will handle it
	ConferenceChangeByServer( pQ );

	return true;
}

bool BBQClient::Subscribe( const SIMD_CS_SUBSCRIBE * in, SIMD_SC_SUBSCRIBE * out )
{
	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_SUBSCRIBE, sizeof(SIMD_CS_SUBSCRIBE) );
	memcpy( & req.msg.simData, in, sizeof(SIMD_CS_SUBSCRIBE) );

	bool bDone = false;
	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_SUBSCRIBE, CLIENT_REQUEST_TIMEOUT );
	if( reply ) {
		SIMD_SC_SUBSCRIBE * p = (SIMD_SC_SUBSCRIBE *) & reply->msg.simData;
		switch( p->statusCode ) {
		case SIMS_OK:
		case SIMS_EXISTS:
			* out = * p;
			bDone = true;
			break;
		case SIMS_DENY:
			m_errLastError = PeerDenied;
			break;
		case SIMS_BADREQUEST:
		default:
			m_errLastError = InvalidArgument;
		}

		ReleaseMessage( reply );
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::MeetingInvite( uint32 uid, const BBQ_MeetingInfo * pM )
{
	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_MCC_INVITE, sizeof(SIMD_CS_MCC_INVITE) );
	SIMD_CS_MCC_INVITE * pQ = (SIMD_CS_MCC_INVITE *) & req.msg.simData;
	memset( pQ, 0, sizeof(SIMD_CS_MCC_INVITE) );

	pQ->sessionInfo = m_sessionInfo;
	pQ->meetingInfo = * pM;
	pQ->toNewId = uid;

	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_MCC_INVITE, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_MCC_INVITE * pA = (SIMD_SC_MCC_INVITE *) & reply->msg.simData;
		switch( pA->statusCode ) {
		case SIMS_OK:
			bDone = true;
			break;
		case SIMS_NOTFOUND:
			m_errLastError = PeerNotFound;
			break;
		case SIMS_DENY:
		default:
			m_errLastError = LocalOffline;
		}
		ReleaseMessage( reply );
	}

	return bDone;
}

bool BBQClient::NotifyReady( void )
{
	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_NOTIFY_READY, sizeof(SIMD_CS_NOTIFY_READY) );
	SIMD_CS_NOTIFY_READY * pQ = (SIMD_CS_NOTIFY_READY *) & req.msg.simData;
	memset( pQ, 0, sizeof(SIMD_CS_NOTIFY_READY) );
	pQ->sessionInfo = this->m_sessionInfo;
	//PostMessage( & req, true );  
	 return SendMessage( & req, true ); 
	return true;
}
 
bool BBQClient::LoginEx( const SIMD_CS_LOGIN_EX * in, SIMD_SC_LOGIN_EX * out )
{
	m_errLastLoginCode = 0;

	if( m_bLogin ) Logout();

	this->m_nLastStatusCode = in->onlineStatus;

	BBQACLFLAG_SET( m_accessFlags, BBQACLFLAG_DEFAULT );

	SIM_REQUEST req;

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;

		// query server, and get my LAN/WAN address
		{
			SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_HELLO, sizeof(SIMD_CS_HELLO) );
			SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_HELLO, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
			if( reply ) {
				SIMD_SC_HELLO * pA = (SIMD_SC_HELLO *) & reply->msg.simData;

				PIPSocket::Address local_addr, wan_addr;
				GetHostAddress( local_addr );
				wan_addr = pA->clientIp;

				m_chanLogin.local.ip = (DWORD) local_addr;
				m_netWAN.ip = (DWORD) wan_addr;
				m_nFirewallType = StrictPolicy;

				ReleaseMessage( reply );
			}
		}

		// before login, check the UDP is allowed for heartbeat or not
		TestUdpForTcpLogin();

	} else {
		m_chanHeartbeat = m_chanLogin;

		if( ! m_bServerTested ) {
			if( TestServer( m_chanLogin.peer.ip, m_chanLogin.peer.port ) ) 
      {

				m_bServerTested = true;
			} else {
				m_errLastError = ServerNotFound;
				return false;
			}
		}
	}

	m_techInfo.interface_ip = m_chanLogin.local.ip;
	m_techInfo.wan = (m_chanLogin.local.ip == m_netWAN.ip) ? 1 : 0;
	m_techInfo.firewall = (int) m_nFirewallType;

	bool bDone = false;

	SIM_REQUEST * reply = NULL;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_LOGIN_EX, sizeof(SIMD_CS_LOGIN_EX) );
	memcpy( & req.msg.simData, in, sizeof(SIMD_CS_LOGIN_EX) );

	SIMD_CS_LOGIN_EX * pIn =  (SIMD_CS_LOGIN_EX *) & req.msg.simData;
	pIn->techParam.firewall = (int) m_nFirewallType;
	pIn->techParam.interface_ip = m_chanLogin.local.ip;
	pIn->techParam.wan = (m_chanLogin.local.ip == m_netWAN.ip) ? 1 : 0;

	reply = RequestMessage( & req, SIM_SC_LOGIN_EX, CLIENT_LOGINEX_TIMEOUT );
	if( reply ) {
		SIMD_SC_LOGIN_EX * p = (SIMD_SC_LOGIN_EX *) & reply->msg.simData;
		switch( p->statusCode ) {
		case SIMS_OK:
			* out = * p;
			m_nServerId = out->nServerId;
			m_accessFlags = out->accessFlags;
			m_sessionInfo = out->sessionInfo;

			// store the correct password here, will use it as key to encrypt data
			// if the string is shorter than USERDATA_NAME_SIZE, will be filled with null characters
			strncpy( m_lpszMyPassword, in->lpszPassword, USERDATA_NAME_SIZE );

			m_bLogin = true;
			m_bHeartbeatOkay = true;
			bDone = true;
			break;
		case SIMS_NOTAVAILABLE:
			m_errLastError = ServerSwitch;
			OnNotifySwitchServerWhenLogin( p->anotherServer.ip, p->anotherServer.port );
			break;
		case SIMS_SERVERERROR:
			m_errLastError = ServerError;
			break;
		case SIMS_SERVICEEXPIRED:
			m_errLastError = ServiceExpired;
			break;
		case SIMS_DENY:
			m_errLastLoginCode = p->errCode;
			m_errLastError = InvalidRegKey;
			if( m_errLastLoginCode == ERR_FORBIDDEN ) {
				time_t nowTime = p->tNowServerTime;
				time_t expire_time = p->tExpireTime;
				int leftTimes = expire_time - nowTime;
				if( leftTimes <= 0 ) {
					m_errLastLoginCode = ERR_EXPIRED;
				}
			}
			break;
		case SIMS_NOTFOUND:
			m_errLastError = IDNotAvailable;
			break;
		case SIMS_BADREQUEST:
		default:
			m_errLastError = InvalidArgument;
		}

		ReleaseMessage( reply );
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

//extern void SaveCertToFile(const char* pszFile, void* pszContent);
extern int VerifyServerCert(void* peerCertificate, const char* hostname);

const char * BBQClient::GetServerPKICertText( void )
{
	return m_lpszX509Text;
}
bool BBQClient::EnterRoom(const uint32 roomid )
{
	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_ENTER_ROOM, sizeof(SIMD_CS_ENTER_ROOM) );
	SIMD_CS_ENTER_ROOM * pQ = (SIMD_CS_ENTER_ROOM *) & req.msg.simData;
	memset( pQ, 0, sizeof(SIMD_CS_ENTER_ROOM) );
  pQ->sessionInfo = m_sessionInfo;
  pQ->nRoomID =roomid;
	//strcpy( pQ->roomlist, members );

	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_ENTER_ROOM, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_ENTER_ROOM * pA = (SIMD_SC_ENTER_ROOM *) & reply->msg.simData;
    //start invited
//    OnRecvCallers(pA->calllist, pA->caller);
		ReleaseMessage( reply );
	} else 	{
		m_errLastError = NetworkError;
	}

	return bDone;
}


bool BBQClient::DownloadServerPKICert( void )
{
	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	bool bDone = false; SIMD_SC_GET_PKI_CERT * pA =NULL;SIM_REQUEST *reply=NULL;
	if (!m_bSupportCompr)
	{
		SIM_REQUEST req;
		SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_GET_PKI_CERT, sizeof(SIMD_CS_GET_PKI_CERT) );
		SIMD_CS_GET_PKI_CERT * pQ = (SIMD_CS_GET_PKI_CERT *) & req.msg.simData;
		memset( pQ, 0, sizeof(SIMD_CS_GET_PKI_CERT) );

		pQ->statusCode = SIMS_OK;
		strcpy( pQ->strFormat, "PEM" );

		reply = RequestMessage( & req, SIM_SC_GET_PKI_CERT, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
		if (reply)
		{
			pA =  (SIMD_SC_GET_PKI_CERT *) & reply->msg.simData ;
		}
	}else
	{
		SIM_REQUEST req;
		SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_GET_PKI_CERT_COMPR, sizeof(SIMD_CS_GET_PKI_CERT) );
		SIMD_CS_GET_PKI_CERT * pQ = (SIMD_CS_GET_PKI_CERT *) & req.msg.simData;
		memset( pQ, 0, sizeof(SIMD_CS_GET_PKI_CERT) );

		pQ->statusCode = SIMS_OK;
		strcpy( pQ->strFormat, "PEM" );

		SIM_REQUEST_COMPR *replyTemp = (SIM_REQUEST_COMPR *)RequestMessage( & req, SIM_SC_GET_PKI_CERT_COMPR, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
		if (replyTemp)
		{
			reply = ( SIM_REQUEST*)replyTemp;
			pA = (SIMD_SC_GET_PKI_CERT *) & replyTemp->msg.simDataCompr;
#ifdef D_COMPR

			if (pA && pA->statusCode ==SIMS_OK&& replyTemp->msg.simCompress_flags.data_compressed && replyTemp->msg.simCompress_flags.data_compress_method == C_COMPRESS_BZIP2)
			{
				char uncompr[8192]={0};
				unsigned int uncomprLen = sizeof(uncompr);
				int nPEMLen = reply->msg.simHeader.size - C_COMPR_FLAG_SIZE - sizeof(pA->statusCode) - sizeof(pA->strFormat) ;
				if (nPEMLen>0) 
				{
					if (BZ2_bzBuffToBuffDecompress(uncompr , &uncomprLen ,pA->strPEM, nPEMLen , 0,0)== 0)
					{
						strncpy(pA->strPEM, uncompr, 4000/*uncomprLen*/);
					}else
						PTRACE(1, "failed to decompress the msg" << SIM_MSGNAME(reply->msg.simHeader.id ));
				}else
						PTRACE(1, "Incorrect length of  PEM " << SIM_MSGNAME(reply->msg.simHeader.id ));

			}
			else
			{
				//network error? statusCode is not 0,?incorrect compress paramters? 
				//PTRACE(1, "Incurrect length of  PEM " << SIM_MSGNAME(reply->msg.simHeader.id ));
				//pA =  (SIMD_SC_GET_PKI_CERT *) & replyTemp->msg.simData ;
			}
#endif
		}
	}
	if( reply && pA) {
		switch( pA->statusCode ) {
		case SIMS_OK:
			// TODO: convert pem to x509
			if( m_pX509_server_cert ) {
				PKI_free_cert( m_pX509_server_cert );
				m_pX509_server_cert = NULL;
			}
			if( m_lpszX509Text ) {
				free( m_lpszX509Text );
				m_lpszX509Text = NULL;
			}
			m_pX509_server_cert = PKI_PEM_to_X509( pA->strPEM, strlen( pA->strPEM ) );
			//SaveCertToFile("c:\\1.cer", m_pX509_server_cert );
			if( m_pX509_server_cert ) {
				m_lpszX509Text = X509_to_text( m_pX509_server_cert );
				bDone = true;
			} else {
				m_errLastError = ServerError;
			}
			break;
		case SIMS_NOTAVAILABLE:
		default:
			m_errLastError = ServerError;
		}
		ReleaseMessage( reply );
	} else 	{
		m_errLastError = NetworkError;
	}

	return bDone;
}

bool BBQClient::TestUdpForTcpLogin( void )
{
	PIPSocket::Address local_addr;
	PIPSocket::Address wan_addr;
	FirewallType firewall;

	if( TestServer( local_addr, wan_addr, firewall, m_chanLogin.peer.ip, SIM_PORT ) ) {
		m_bUdpBlocked = false;

		m_chanHeartbeat.socket = 0;
		m_chanHeartbeat.local.ip = (DWORD) local_addr;
		m_chanHeartbeat.local.port = m_nDefaultUdpPort;
		m_chanHeartbeat.peer.ip = m_chanLogin.peer.ip;
		m_chanHeartbeat.peer.port = SIM_PORT;

		m_nFirewallType = firewall;

	} else {
		m_bUdpBlocked = true;

		m_chanHeartbeat = m_chanLogin;
		m_nFirewallType = StrictPolicy;
	}

	return (! m_bUdpBlocked);
}

bool BBQClient::LoginExSecure( const SIMD_CS_LOGIN_EX * in, SIMD_SC_LOGIN_EX * out, uint16 nDataSecureType, uint16 nKeySecureType  )
{
	m_errLastLoginCode = 0;
	m_nServerCertVerifyStatus = 0;

	if( m_bLogin ) Logout();

	this->m_nLastStatusCode = in->onlineStatus;

	BBQACLFLAG_SET( m_accessFlags, BBQACLFLAG_DEFAULT );

	SIM_REQUEST req;

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;

		// query server, and get my LAN/WAN address
		{
			SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_HELLO, sizeof(SIMD_CS_HELLO) );
			SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_HELLO, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
			if( reply ) {
				SIMD_SC_HELLO * pA = (SIMD_SC_HELLO *) & reply->msg.simData;

				PIPSocket::Address local_addr, wan_addr;
				GetHostAddress( local_addr );
				wan_addr = pA->clientIp;

				m_chanLogin.local.ip = (DWORD) local_addr;
				m_netWAN.ip = (DWORD) wan_addr;
				m_nFirewallType = StrictPolicy;

				ReleaseMessage( reply );
			}
		}

		// before login, check the UDP is allowed for heartbeat or not
		TestUdpForTcpLogin();

	} else {
		m_chanHeartbeat = m_chanLogin;
		if( ! m_bServerTested ) {
			if( TestServer( m_chanLogin.peer.ip, m_chanLogin.peer.port ) ) {

				m_bServerTested = true;
			} else {
				m_errLastError = ServerNotFound;
				return false;
			}
		}
	}

	char plain_key[ 512 ];
	char secret[ 512 ];
	memset( plain_key, 0, sizeof(plain_key) );
	memset( secret, 0, sizeof(secret) );
	SIMD_SECRET * pSecret = (SIMD_SECRET *) & secret[ 0 ];

	pSecret->secret_purpose = nDataSecureType;
	switch( pSecret->secret_purpose ) {
	case SECRET_PLAIN:
		break;
	case SECRET_AES128:
		pSecret->secret_size = 128 / 8;	// 128 bit
#if P_SSL
		RAND_bytes( (unsigned char *) plain_key, pSecret->secret_size );
#else
		for( int i=0; i<pSecret->secret_size; i++ ) plain_key[i] = rand() & 0xFF;
#endif
		break;
	case SECRET_AES256:
		pSecret->secret_size = 256 / 8;	// 256 bit
#if P_SSL
		RAND_bytes( (unsigned char *) plain_key, pSecret->secret_size );
#else
		for( int i=0; i<pSecret->secret_size; i++ ) plain_key[i] = rand() & 0xFF;
#endif
		//memcpy( plain_key, "1234567890123456789012345678901234567890", pSecret->secret_size );
		break;
	}

	// now keep the AES key
	if( m_pAESKeyBytes ) {
		free( m_pAESKeyBytes );
		m_pAESKeyBytes = NULL;
	}
	m_nAESKeyBytes = 0;

	m_pAESKeyBytes = (char *) malloc( pSecret->secret_size );
	if( m_pAESKeyBytes ) {
		memcpy( m_pAESKeyBytes, plain_key, pSecret->secret_size );
		m_nAESKeyBytes = pSecret->secret_size;
	}

	pSecret->encrypt_method = nKeySecureType;
	switch( pSecret->encrypt_method ) {
	case SECRET_PLAIN:
		memcpy( & pSecret->encrypted_data[0], & plain_key[0], pSecret->secret_size );
		break;
	case SECRET_RSA:
		if( ! m_pX509_server_cert ) { // server cert not downloaded yet, now let's download it
			if( DownloadServerPKICert() ) {
				// server's public cert is downloaded successfully
			} else {
				return false;
			}
		}

		if( m_pX509_server_cert ) {
			// Verify Cert
			if( m_bVerifyServerCert )
			{
				int nStatus = VerifyServerCert(m_pX509_server_cert, m_strServerHostname);
				if( nStatus != 0 ) {
					m_nServerCertVerifyStatus = nStatus;
					m_errLastError = ServerCertError;
					return false;
				}
			}
			int outlen = PKI_rsa_public_encrypt( m_pX509_server_cert, plain_key, pSecret->secret_size, & pSecret->encrypted_data[0] );
			if( outlen > 0 ) {
				pSecret->encrypted_data_size = outlen;
			} else {
				// public encrypt fail
				return false;
			}
		} else {
			// not server cert
			return false;
		}
		break;
	}

	bool bDone = false;

	uint32 nSecretSize = sizeof(SIMD_SECRET) + pSecret->encrypted_data_size;
	uint32 nMsgDataSize = nSecretSize + sizeof(uint32) + sizeof(SIMD_CS_LOGIN_EX);
	if( nMsgDataSize > SIM_DATABLOCKSIZEMAX ) {
		m_errLastError = InvalidArgument;
		return false;
	}

	m_techInfo.interface_ip = m_chanLogin.local.ip;
	m_techInfo.wan = (m_chanLogin.local.ip == m_netWAN.ip) ? 1 : 0;
	m_techInfo.firewall = (int) m_nFirewallType;

	SIMD_CS_LOGIN_EX plain_login_ex = * in;
	plain_login_ex.techParam.firewall = (int) m_nFirewallType;
	plain_login_ex.techParam.interface_ip = m_chanLogin.local.ip;
	plain_login_ex.techParam.wan = (m_chanLogin.local.ip == m_netWAN.ip) ? 1 : 0;

	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_LOGIN_EX_SECURE, nMsgDataSize );
	memcpy( & req.msg.simData[ 0 ], pSecret, nSecretSize );
	uint32 * pLen = (uint32 *) & req.msg.simData[ nSecretSize ];
	uint32 nOffset = nSecretSize + sizeof(uint32);

	switch( pSecret->secret_purpose ) {
	case SECRET_PLAIN:
		memcpy( & req.msg.simData[ nOffset ], & plain_login_ex, sizeof(SIMD_CS_LOGIN_EX) );
		* pLen = sizeof(SIMD_CS_LOGIN_EX);
		break;
	case SECRET_AES128:
		if( AES_encrypt( (unsigned char *) & plain_login_ex, sizeof(SIMD_CS_LOGIN_EX), (unsigned char *) plain_key, (unsigned char *) PI_SQRT2_STRING, 128, (unsigned char *)  & req.msg.simData[ nOffset ] ) ) {
			* pLen = sizeof(SIMD_CS_LOGIN_EX);
		} else return false;
		break;
	case SECRET_AES256:
		if( AES_encrypt( (unsigned char *) & plain_login_ex, sizeof(SIMD_CS_LOGIN_EX), (unsigned char *) plain_key, (unsigned char *) PI_SQRT2_STRING, 256, (unsigned char *)  & req.msg.simData[ nOffset ] ) ) {
			* pLen = sizeof(SIMD_CS_LOGIN_EX);
		} else return false;
		break;
	default:
		return false;
	}

	SIM_REQUEST * reply = NULL;
	reply = RequestMessage( & req, SIM_SC_LOGIN_EX, CLIENT_LOGINEX_TIMEOUT );
	if( reply ) {
		SIMD_SC_LOGIN_EX * p = (SIMD_SC_LOGIN_EX *) & reply->msg.simData;
		* out = * p;
		switch( p->statusCode ) {
		case SIMS_OK:
			m_nServerId = out->nServerId;
			m_accessFlags = out->accessFlags;
			m_sessionInfo = out->sessionInfo;

			// store the correct password here, will use it as key to encrypt data
			// if the string is shorter than USERDATA_NAME_SIZE, will be filled with null characters
			strncpy( m_lpszMyPassword, in->lpszPassword, USERDATA_NAME_SIZE );

			m_bLogin = true;
			m_bHeartbeatOkay = true;
			bDone = true;
			break;
		case SIMS_NOTAVAILABLE:
			m_errLastError = ServerSwitch;
			OnNotifySwitchServerWhenLogin( p->anotherServer.ip, p->anotherServer.port );
			break;
		case SIMS_SERVERERROR:
			m_errLastError = ServerError;
			break;
		case SIMS_SERVICEEXPIRED:
			m_errLastError = ServiceExpired;
			break;
		case SIMS_DENY:
			m_errLastLoginCode = p->errCode;
			m_errLastError = InvalidRegKey;
			if( m_errLastLoginCode == ERR_FORBIDDEN ) {
				time_t nowTime = p->tNowServerTime;
				time_t expire_time = p->tExpireTime;
				int leftTimes = expire_time - nowTime;
				if( leftTimes <= 0 ) {
					m_errLastLoginCode = ERR_EXPIRED;
				}
			}
			break;
		case SIMS_NOTFOUND:
			m_errLastError = IDNotAvailable;
			break;
		case SIMS_BADREQUEST:
		default:
			m_errLastError = InvalidArgument;
		}

		ReleaseMessage( reply );
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::LocateEx( const SIMD_CS_LOCATE_EX * in, SIMD_SC_LOCATE_EX * out )
{
	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	bool bDone = false;
	SIM_REQUEST * reply = NULL;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_LOCATE_EX, sizeof(SIMD_CS_LOCATE_EX) );
	memcpy( & req.msg.simData, in, sizeof(SIMD_CS_LOCATE_EX) );

	reply = RequestMessage( & req, SIM_SC_LOCATE_EX, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_LOCATE_EX * p = (SIMD_SC_LOCATE_EX *) & reply->msg.simData;
		switch( p->statusCode ) {
		case SIMS_OK:
			if(req.msg.simHeader.size < sizeof(SIMD_SC_LOCATE_EX) )
			{
				memcpy(out, p, sizeof(SIMD_SC_LOCATE_EX_OLD));
			}else
				* out = * p;
			if( (out->dwServerIp == 0) || (out->dwServerIp == LOCALHOST_IP) ) out->dwServerIp = req.channel.peer.ip;
			if( (out->addrWAN.ip == 0) || (out->addrWAN.ip == LOCALHOST_IP) ) out->addrWAN.ip = req.channel.peer.ip;
			{
				uint32 serverid = in->vfonid.serverid;
				if( serverid != 0 ) {
					//m_csDomainServers.();
					IPSOCKADDR server;
					server.ip = out->dwServerIp;
					if( m_bTcpLogin ) {
						server.port = out->nServerTcpPort;
					} else {
						server.port = out->nServerUdpPort;
					}
					m_mapDomainServers[serverid] = server;
					//m_csDomainServers.Leave();
				}
			}
			bDone = true;
			break;
		case SIMS_NOTFOUND:
			m_errLastError = PeerNotFound;
			break;
		case SIMS_BADREQUEST:
		default:
			m_errLastError = InvalidArgument;
		}

		ReleaseMessage( reply );
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::NotifyServerConferenceChange( const SIMD_CS_CONFERENCE * in )
{
	if( ! m_bLogin ) {
		m_errLastError = LocalOffline;
		return false;
	}

	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_CONFERENCE, sizeof(SIMD_CS_CONFERENCE) );
	SIMD_CS_CONFERENCE * pQ = (SIMD_CS_CONFERENCE *) & req.msg.simData;

	memcpy( pQ, in, sizeof(SIMD_CS_CONFERENCE) );
	pQ->sessionInfo = m_sessionInfo;

	PostMessage( & req, true, 2000 + m_nNetworkBaseDelay * 2 );
	// return SendMessage( & req, true, 2000 + m_nNetworkBaseDelay * 2 );
	return true;
}

bool BBQClient::DownloadString( const PString & strName, PString & strValue )
{
	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	bool bDone = false;
	PBytePack packedData, packedMsg;
	SIM_REQUEST * reply=NULL;
	PBytePack* reUncompr=NULL;
	if (!m_bSupportCompr)
	{
		SIM_REQUEST req;
		SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_LOADSTRING, 0 );

		const char * lpszName = (const char *) strName;

		packedData << m_sessionInfo << lpszName;
		req.msg.simHeader.size = packedData.Size();
		packedMsg << req.channel << req.msg.simHeader << packedData;

		reply = RequestMessage( (SIM_REQUEST *) packedMsg.Data(), SIM_SC_LOADSTRING, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	}
	else
	{
		SIM_REQUEST req;
		SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_LOADSTRING_COMPR, 0);

		const char * lpszName = (const char *) strName;

		packedData << m_sessionInfo << lpszName;
		req.msg.simHeader.size = packedData.Size();
		packedMsg << req.channel << req.msg.simHeader << packedData;

		reply = RequestMessage( (SIM_REQUEST *) packedMsg.Data(), SIM_SC_LOADSTRING_COMPR, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	}
	if( reply ) {
		try {
			uint32 statusCode;

			PBytePack re( & reply->msg.simData, reply->msg.simHeader.size );
#ifdef D_COMPR

			if (reply->msg.simHeader.id ==SIM_SC_LOADSTRING_COMPR)
			{
				SIM_MESSAGE_COMPR::simCompress comprFlags;
				re >> comprFlags;
				if (comprFlags.data_compressed &&comprFlags.data_compress_method == C_COMPRESS_BZIP2 )
				{//compressed data
					int nHeaderLen = sizeof(SIM_MESSAGE_COMPR::simCompress ) +  sizeof(statusCode)  ;
					int comprLen = reply->msg.simHeader.size -  nHeaderLen;
					char uncompr[1024*8]={0};
					unsigned int uncomprLen= sizeof(uncompr);
					if (comprLen>0) 
					{
						if (BZ2_bzBuffToBuffDecompress(uncompr , &uncomprLen ,((char*)re.Data())+ nHeaderLen, comprLen , 0, 0)== 0)
						{
							//reUncompr = new PBytePack(uncompr, uncomprLen);
							reUncompr = new PBytePack();
							//(*reUncompr) << (const char *)uncompr;
							const char * pStringValue = (const char *) uncompr;
						*reUncompr << pStringValue;
					}
						else
							PTRACE( 1, "An error occurred when uncompressed  the data." );

					}
					else
						PTRACE( 1, "Incorrect compressed data." );

				}else
						;//It is orignal data , continue to do
			}

#endif
			re >> statusCode;
			
			switch( statusCode ) {
			case SIMS_OK: 
				{
					char * lpszValue = NULL;
					if (!reUncompr)
						re >> lpszValue;
					else
						*reUncompr>> lpszValue;

					if( lpszValue ) {
						strValue = lpszValue;
						delete[] lpszValue;
						bDone = true;
					}
				}
				break;
			default:
				m_errLastError = InvalidArgument;
			}
		} catch ( const char * ) {
			// bad response data
			m_errLastError = InvalidArgument;
		}
		
		ReleaseMessage( reply );
		//return bDone;
	} else {
		m_errLastError = Timeout;
	}
	if (reUncompr) delete reUncompr;
	return bDone;
}

bool BBQClient::GetBindInfoByVfonId( VFONID vfonid, VfonIdBindInfo * bindInfo )
{
	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_GETBINDINFO, sizeof(SIMD_CS_GETBINDINFO) );
	SIMD_CS_GETBINDINFO * cs = (SIMD_CS_GETBINDINFO *) req.msg.simData;
	memset( cs, 0, sizeof(*cs) );
	cs->vfonId = vfonid;

	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_GETBINDINFO, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_GETBINDINFO * sc = (SIMD_SC_GETBINDINFO *) reply->msg.simData;
		switch( sc->statusCode ) {
		case SIMS_OK: 
			* bindInfo = sc->bindInfo;
			bDone = true;
			break;
		default:
			m_errLastError = PeerNotFound;
		}
		ReleaseMessage( reply );
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::GetVfonIdByBindInfo( const VfonIdBindInfo * bindInfo, VFONID * pId )
{
	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_GETBINDINFO, sizeof(SIMD_CS_GETBINDINFO) );
	SIMD_CS_GETBINDINFO * cs = (SIMD_CS_GETBINDINFO *) req.msg.simData;
	memset( cs, 0, sizeof(*cs) );
	cs->bindInfo = * bindInfo;

	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_GETBINDINFO, CLIENT_SIMPLE_REQUEST_TIMEOUT  + m_nNetworkBaseDelay * 2);
	if( reply ) {
		SIMD_SC_GETBINDINFO * sc = (SIMD_SC_GETBINDINFO *) reply->msg.simData;
		switch( sc->statusCode ) {
		case SIMS_OK: 
			* pId = sc->vfonId;
			bDone = true;
			break;
		default:
			m_errLastError = PeerNotFound;
		}
		ReleaseMessage( reply );
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::OnMsgIdQuery( const SIM_REQUEST * req )
{
	const UCMD_CC_IDQUERY * cs = (const UCMD_CC_IDQUERY *) & req->msg.simData;

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, UCM_CC_IDCONFIRM, sizeof(UCMD_CC_IDCONFIRM), req );
	UCMD_CC_IDCONFIRM * sc = (UCMD_CC_IDCONFIRM *) & reply.msg.simData;
	memset( sc, 0, sizeof(*sc) );

	sc->statusCode = ((! m_bForceBBQProxy) && (cs->vfonid.userid == m_sessionInfo.id)) ? SIMS_OK : SIMS_NOTFOUND;

	sc->vfonid.serverid = 0;
	sc->vfonid.userid = m_sessionInfo.id;

	SendMessage( & reply );

	return true;
}

bool BBQClient::OnMsgChannelDirectRequest( const SIM_REQUEST * req )
{
	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, UCM_CC_A, sizeof(UCMD_CC_A), req );
	UCMD_CC_A * pA = (UCMD_CC_A *) & reply.msg.simData;

	// ignore msg not to me, to avoid attack
	const UCMD_CC_Q * pQ = (const UCMD_CC_Q *) & req->msg.simData;
	/*if( (pQ->channelInfo.uid != m_sessionInfo.id) || m_bForceBBQProxy) {
		pA->statusCode = SIMS_NOTFOUND;
		SendMessage( & reply );
		return true;
	}*/

	const UCMD_CHANNELINFO * pPeerInfo = & pQ->channelInfo;
	uint32		nPeerId = pQ->sessionInfo.id;

	pA->sessionInfo = pQ->sessionInfo;
	pA->channelInfo = pQ->channelInfo;
	pA->statusCode = SIMS_OK;
	pA->statusText[0] = '\0';

	SIM_REQUEST con;
	SIM_REPLY_REQUEST( & con, UCM_CC_CON, sizeof(UCMD_CC_CON), req );
	bool bConnect = false;

	uint32 nAppParam = pQ->extraInfo.caller.nAppParam;

	if( OnChannelRequesting( nPeerId, pQ->statusCode, pQ->statusText, nAppParam ) ) {

		if( req->channel.socket != 0 ) { // this is a TCP connection

			// okay, we agree
			pA->statusCode = SIMS_OK;

			{
				LockIt sf( m_mutexChannelWait );
				pA->extraInfo.called.dwChannelId = ++ m_nNextChannelId;
				pA->extraInfo.called.dwTimeBase = m_dwTimeBase;
				pA->extraInfo.called.nServerId = m_nServerId;
			}

			SendMessage( & reply );

			BBQMsgConnection * pConn = FindMsgConnection( 0, 0, req->channel.socket );
			if( pConn && DetachMsgConnection( pConn ) ) {

				BBQ_CHANNEL * pChannel = new BBQ_CHANNEL;
				memset( pChannel, 0, sizeof(BBQ_CHANNEL) );

				pChannel->tcpSock = pConn->DetachSocket(); 

				pChannel->info.peerId = nPeerId;

				PIPSocket::Address addr; WORD port;
				pChannel->tcpSock->GetPeerAddress( addr, port );
				pChannel->info.peerWAN.ip = pChannel->info.peerLAN.ip = (DWORD) addr;
				pChannel->info.peerWAN.port = pChannel->info.peerLAN.port = port;

				pChannel->tcpSock->GetLocalAddress( addr, port );
				pChannel->info.thisLAN.ip = (DWORD) addr;
				pChannel->info.thisLAN.port = port;

				pChannel->info.bIsProxied = false;
				pChannel->info.bInitFromLocal = false;

				pChannel->info.dwLocalCount = pA->extraInfo.called.dwChannelId;//pQ->extraInfo.called.dwChannelId;
				pChannel->info.dwLocalTimeBase = m_dwTimeBase;//pQ->extraInfo.called.dwTimeBase;
				pChannel->info.nLocalAppParam = nAppParam;

				pChannel->info.dwRemoteCount = pQ->extraInfo.caller.dwChannelId;
				pChannel->info.dwRemoteTimeBase = pQ->extraInfo.caller.dwTimeBase;
				pChannel->info.nRemoteAppParam = pQ->extraInfo.caller.nAppParam;

				pChannel->info.peerServerId = pQ->extraInfo.caller.nServerId;

				BBQCHANNEL_REMOTE_TRACE(pChannel);

				OnRequestedChannelSetup( pChannel, pQ->statusCode, pQ->statusText );
				delete pConn;
			}
			return true;
		}

		// bind a port
		PUDPSocket * pSock = NULL;
		WORD port;
		if( BindDynamicUdpPort( pSock, port ) ) {

			// create a pWait object
			BBQChannelWait* pWait = new BBQChannelWait( pSock, *this );
			pWait->info.uid = nPeerId;
			pWait->info.init = 0;
			pWait->info.cid = pPeerInfo->cid;
			pWait->info.firewall = (m_nFirewallType & 0x3);
			pWait->info.give_me_proxy = m_bForceBBQProxy ? 1 : pPeerInfo->give_me_proxy; // meaningless, because we go directly

			PIPSocket::Address local_addr; GetHostAddress( local_addr );
			pWait->info.thisLAN.ip = local_addr;
			pWait->info.thisLAN.port = port;

			pWait->info.peerLAN = pPeerInfo->thisLAN;
			pWait->info.peerWAN = req->channel.peer;

			if( pPeerInfo->invite ) {
				pWait->info.invite = 0;
			} else {
				pWait->info.invite = 1;
			}

			pWait->info.cookie = pPeerInfo->cookie;

			pWait->m_nAppCode = pQ->statusCode;
			pWait->m_strAppString = pQ->statusText;

			{
				LockIt sf( m_mutexChannelWait );
				pWait->extra.dwChannelId = ++ m_nNextChannelId;
			}
			pWait->extra.dwTimeBase = m_dwTimeBase;
			pWait->extra.nServerId = m_nServerId;
			pWait->extra.nAppParam = nAppParam;

			pWait->peerExtra = pQ->extraInfo.caller;
			// -----------------------------------------------------------
			{
				LockIt sf( m_mutexChannelWait );
				m_listChannelWaits.push_back( pWait );
			}

			// now send back response
			pA->statusCode = SIMS_OK;
			pA->sessionInfo = pQ->sessionInfo;
			pA->channelInfo = pWait->info;
			pA->channelInfo.uid = m_sessionInfo.id;
			pA->extraInfo.called = pWait->extra;
			pA->extraInfo.caller = pQ->extraInfo.caller;

			// we must use the same port to response, in case the sender is behind strict NAT
			// 

			UCMD_CC_CON * pCon = (UCMD_CC_CON *) con.msg.simData;
			pCon->channelInfo = pWait->info;
			pCon->channelInfo.uid = m_sessionInfo.id;
			pCon->statusCode = pWait->m_nAppCode;
			strncpy( pCon->statusText, pQ->statusText, SIM_APPSTRINGSIZEMAX ); pCon->statusText[ SIM_APPSTRINGSIZEMAX -1 ] = '\0';
			pCon->extraInfo.called = pA->extraInfo.called;
			pCon->extraInfo.caller = pA->extraInfo.caller;
			//strcpy( pCon->statusText, pQ->statusText );

			// send connect info to where i see it from
			// channel.peer = req->channel.peer;

			// now we connect using a new port
			con.channel.local.port = port;

			bConnect = true;
		} else {
			pA->statusCode = SIMS_REFUSE;
		}
	} else {
		pA->statusCode = SIMS_REFUSE;
	}

	SendMessage( & reply ); sleep_ms(1);

	if( bConnect ) {
		SendMessage( & con, true, 100, 8 );
	}

	return true;
}

bool BBQClient::OnMsgChannelDirectResponse( const SIM_REQUEST * req )
{
	if( m_bForceBBQProxy ) return true;

	const UCMD_CC_A * pQ = (const UCMD_CC_A *) & req->msg.simData;
	const UCMD_CHANNELINFO * pPeerInfo = & pQ->channelInfo;

	uint32		nPeerId = pQ->channelInfo.uid;

	// ignore if id not match, to avoid anonymous attack
	// if( pQ->sessionInfo.id != m_sessionInfo.id ) return true;

	SIM_REQUEST con;
	SIM_REPLY_REQUEST( & con, UCM_CC_CON, sizeof(UCMD_CC_CON), req );
	bool bConnect = false;
  PTRACE(3,  " nPeerId=" <<nPeerId << "statuscode=" <<  pQ->statusCode);
	if( pQ->statusCode == SIMS_OK ) {
		// from my bind port, send connect message to the invited port
		LockIt safe( m_mutexChannelWait ); 
		for( cw_iterator iter = m_listChannelWaits.begin(), eiter = m_listChannelWaits.end(); iter != eiter; iter ++ ) {
			BBQChannelWait * pWait = * iter;
			UCMD_CHANNELINFO * info = & pWait->info;
			if( info->init 
				&& (info->uid == nPeerId)
				&& (info->cid == pQ->channelInfo.cid) ) {

				pWait->peerInfo = pQ->channelInfo;
				pWait->peerExtra = pQ->extraInfo.called;

				// copy useful info provided by peer & server
				info->cookie = pPeerInfo->cookie;
				info->peerLAN = pPeerInfo->thisLAN;
				
				// bad: this is not the valid port for client-client connection
				//info->peerWAN = req->channel.peer;

				info->peerWAN.ip = req->channel.peer.ip;
				info->peerWAN.port = pPeerInfo->thisLAN.port;

				// send a connect message
				UCMD_CC_CON * pCon = (UCMD_CC_CON *) & con.msg.simData;
				pCon->channelInfo = * info;
				pCon->channelInfo.uid = m_sessionInfo.id;

				pCon->statusCode = pWait->m_nAppCode;
//				strcpy( pCon->statusText, pWait->m_strAppString );
				strncpy( pCon->statusText, pWait->m_strAppString, SIM_APPSTRINGSIZEMAX ); pCon->statusText[ SIM_APPSTRINGSIZEMAX -1 ] = '\0';

				// connect to the port specified by peer
				con.channel.peer = info->peerWAN;
				//con.channel.peer.port = pPeerInfo->thisLAN.port;

				bConnect = true;
        PTRACE(3, "bconnect = true, peerid"<< nPeerId);
				break;
			}
		}

	} else {
		switch( pQ->statusCode ) {
		//case SIMS_DENY:
		//case SIMS_TIMEOUT:
		//case SIMS_NOTFOUND:
		//case SIMS_FIREWALL:
		case SIMS_REFUSE:
			m_errLastError = PeerDenied;
			break;
		default:
			m_errLastError = NetworkError;
		}

		LockIt safe( m_mutexChannelWait ); 
		for( cw_iterator iter = m_listChannelWaits.begin(), eiter = m_listChannelWaits.end(); iter != eiter; /**/ ) {
			BBQChannelWait * pWait = * iter;
			UCMD_CHANNELINFO * info = & pWait->info;
			if( info->thisLAN.port == req->channel.local.port ) {
				pWait->m_bDone = false;
				pWait->Signal();
				//iter = m_listChannelWaits.erase( iter );
				STL_ERASE( m_listChannelWaits, cw_iterator, iter );
				break;
			} else {
				iter ++;
			}
		}
	}

	if( bConnect ) SendMessage( & con, true );

	return true;
}

bool BBQClient::QueryServerStatus( SIMD_SC_SERVERSTATUS & out, PIPSocket::Address svraddr, WORD svrport )
{
	if( svraddr == 0 ) {
		svraddr = m_chanLogin.peer.ip;
		svrport = m_chanLogin.peer.port;
	}

	bool bDone = false;

	SIM_REQUEST req;
	if( (svraddr == m_chanLogin.peer.ip) && (svrport == m_chanLogin.peer.port) ) {
		if( m_bTcpLogin ) {
			// check the connection before sending data, if not available, then reconnect
			if(! ValidateTcpConnection()) return false;
		}
		SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_SERVERSTATUS, 0 );
	} else {
		SIM_REQINIT( req, 0, 0, m_nDefaultUdpPort, (DWORD) svraddr, svrport, SIM_CS_SERVERSTATUS, 0 );
	}

	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_SERVERSTATUS, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_SERVERSTATUS * p = (SIMD_SC_SERVERSTATUS *) & reply->msg.simData;
		out = * p;

		ReleaseMessage( reply );
		bDone = true;
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::QueryProxyStatus( SIMD_SC_PROXYSTATUS & out, PIPSocket::Address svraddr, WORD svrport )
{
	if( svraddr == 0 ) {
		svraddr = m_chanLogin.peer.ip;
		svrport = m_chanLogin.peer.port;
	}

	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINIT( req, 0, 0, m_nDefaultUdpPort, (DWORD) svraddr, svrport, SIM_CS_PROXYSTATUS, 0 );
	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_PROXYSTATUS, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_PROXYSTATUS * p = (SIMD_SC_PROXYSTATUS *) & reply->msg.simData;
		out = * p;

		ReleaseMessage( reply );
		bDone = true;
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::CommandSwitchServer( SIMD_CS_SERVERSWITCH & in, SIMD_SC_SERVERSWITCH & out )
{
	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_SERVERSWITCH, sizeof(SIMD_CS_SERVERSWITCH) );
	SIMD_CS_SERVERSWITCH * pIn = (SIMD_CS_SERVERSWITCH *) & req.msg.simData;
	* pIn = in;

	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_SERVERSWITCH, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_SERVERSWITCH * pOut = (SIMD_SC_SERVERSWITCH *) & reply->msg.simData;
		out = * pOut;

		ReleaseMessage( reply );
		bDone = true;
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::CommandSwitchServerActive( const char * lpszUsername, const char * lpszPassword )
{
	SIMD_CS_SERVERSWITCH in;
	memset( & in, 0, sizeof(in) );
	memcpy( in.username, lpszUsername, sizeof(in.username) );
	memcpy( in.password, lpszPassword, sizeof(in.password) );
	in.serverType = BBQServer::INVALID_TYPE;
	in.serverStatus = BBQServer::ACTIVE;

	SIMD_SC_SERVERSWITCH out;
	memset( & out, 0, sizeof(out) );

	return ( CommandSwitchServer( in, out ) && (out.statusCode == SIMS_OK) && (out.serverStatus == BBQServer::ACTIVE) ); 
}

bool BBQClient::OnMsgBuddyStatusNotify( const SIM_REQUEST * req )
{
	const SIMD_SC_BUDDYSTATUSNOTIFY * pQ = (const SIMD_SC_BUDDYSTATUSNOTIFY *) & req->msg.simData;
	if( (pQ->sessionInfo.id != m_sessionInfo.id) || (pQ->sessionInfo.cookie != m_sessionInfo.cookie) ) return true;

	if (req->msg.simHeader.size == sizeof(SIMD_SC_BUDDYSTATUSNOTIFY) )
	{
		OnBuddyStatusChanged( & pQ->info,  pQ->szStatusString );
		//PAssertAlways( pQ->info.alias);
	}else if (req->msg.simHeader.size > sizeof(SIMD_SC_BUDDYSTATUSNOTIFY))
	{
const int  D_POS =sizeof(SIMD_SC_BUDDYSTATUSNOTIFY);
		//we try to get the alias
		int len=0;
		memcpy(&len, &req->msg.simData[ D_POS], sizeof(len));
		//PAssertAlways( "len");
		if (len >0 && len < USERDATA_ALIAS_SIZE_EXT)
		{
			char tmp[USERDATA_ALIAS_SIZE_EXT]={0};
			memcpy(tmp, &(req->msg.simData[ D_POS+ sizeof(len)]), len);
			OnBuddyStatusChanged( & pQ->info,  pQ->szStatusString, tmp );
		}else
			PTRACE(1, "bbqclient\t Received an error alias. len is" << len);
	}else
		PTRACE(1, "bbqclient\t Received an error alias. msg data len  is" << req->msg.simHeader.size);


	return true;
}
bool BBQClient::OnMsgSelfBuddyStatusNotify( const SIM_REQUEST * req )
{
	//const SIMD_SC_BUDDYSTATUSNOTIFY * pQ = (const SIMD_SC_BUDDYSTATUSNOTIFY *) & req->msg.simData;
	//if( (pQ->sessionInfo.id != m_sessionInfo.id) || (pQ->sessionInfo.cookie != m_sessionInfo.cookie) ) return true;
	
	UserStatusListExt list;
	if (req->msg.simHeader.size >0)
	{
		uint32 nCount =0;
		PBytePack data( &req->msg.simData[sizeof(nCount)] , req->msg.simHeader.size -sizeof(nCount));

		char *pValue=NULL;
		memcpy(&nCount,  &req->msg.simData[0], sizeof(nCount));
		for(int i=0;i< nCount;i++)
		{
			ONLINE_USER_EXT userstatus;
			userstatus.user.status = CLIENT_NORMAL;//default is online
			data >> userstatus.user.id;
			data >> pValue;
			if (pValue) 
			{
				strncpy( userstatus.strStatusString , pValue, sizeof( userstatus.strStatusString));
				list.push_back(userstatus);
				delete pValue;pValue=NULL;
			}
		} 
	}
	OnSelfBuddyStatusChanged( list );

	return true;
}

bool BBQClient::GetMCUList( MCUList & listMCUs )
{
	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	SIM_REQUEST * reply = NULL;
	{
		SIM_REQUEST req;
		SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_GETMCULIST, 0 );
		SIMD_SC_GETMCULIST * pQ = (SIMD_SC_GETMCULIST *) & req.msg.simData;
		//pQ->nBase = 0;
		memset( pQ, 0, sizeof(SIMD_CS_GETMCULIST) );
		reply = RequestMessage( & req, SIM_SC_GETMCULIST, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	}

	bool bDone = false;

	if( reply ) {
		SIMD_SC_GETMCULIST * pA = (SIMD_SC_GETMCULIST *) & reply->msg.simData;
		int nCount = min( pA->nCount, 20 );
		for( int i=0; i<nCount; i++ ) {
			MCURecord * p = new MCURecord;
			memcpy( p, & pA->inMCU[i], sizeof(MCURecord) );
			listMCUs.push_back( p );
		}

		ReleaseMessage( reply );

	} else {
		m_errLastError = Timeout;
	}

	return bDone;	
}

bool BBQClient::GetIdTrees( BBQTrees & trees )
{
	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	SIM_REQUEST * reply = NULL;
	if (!m_bSupportCompr)
	{
		SIM_REQUEST req;
		SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_GETIDTREES, sizeof(SIMD_CS_GETIDTREES) );
		SIMD_CS_GETIDTREES * pQ = (SIMD_CS_GETIDTREES *) & req.msg.simData;
		memset( pQ, 0, sizeof(SIMD_CS_GETIDTREES) );
		pQ->sessionInfo = m_sessionInfo;
		pQ->nId = 2012;//m_sessionInfo.id;
		reply = RequestMessage( & req, SIM_SC_GETIDTREES, CLIENT_REQUEST_TIMEOUT );
	}else
	{
		SIM_REQUEST req;

		SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_GETIDTREES_COMPR, sizeof(SIMD_CS_GETIDTREES)  );
		PBytePack packedData, packedMsg;
		SIMD_CS_GETIDTREES * pQ = (SIMD_CS_GETIDTREES *) & req.msg.simData;
		memset( pQ, 0, sizeof(SIMD_CS_GETIDTREES) );
		pQ->sessionInfo = m_sessionInfo;
		pQ->nId = 2012;//m_sessionInfo.id;
		reply = RequestMessage( & req, SIM_SC_GETIDTREES_COMPR, CLIENT_REQUEST_TIMEOUT );

	}

	bool bDone = false;

	if( reply ) {
		try {
			PBytePack re( & reply->msg.simData, reply->msg.simHeader.size );
			uint32 statusCode, queryID;
			PBytePack *reUncompr=NULL;
#ifdef D_COMPR

			if (reply->msg.simHeader.id == SIM_SC_GETIDTREES_COMPR)/*compress msg id*/
			{
				SIM_MESSAGE_COMPR::simCompress comprFlags;
				re >> comprFlags;
				if (comprFlags.data_compressed &&comprFlags.data_compress_method == C_COMPRESS_BZIP2 )
				{//compressed data
					int nHeaderLen = sizeof(SIM_MESSAGE_COMPR::simCompress ) +  sizeof(statusCode)  +  sizeof(queryID);
					int comprLen = reply->msg.simHeader.size -  nHeaderLen;
					char uncompr[1024*8]={0};
					unsigned int uncomprLen= sizeof(uncompr);
					if (comprLen>0) 
					{
						if (comprLen>0 && BZ2_bzBuffToBuffDecompress(uncompr , &uncomprLen ,((char*)re.Data())+ nHeaderLen, comprLen , 0, 0)== 0)
						{
							reUncompr = new PBytePack();//(uncompr, uncomprLen);
							reUncompr->Pack(uncompr, uncomprLen);
						}
						else
							PTRACE( 1, "An error occurred when uncompressed  the data." );

					}
					else
						PTRACE( 1, "Incorrect compressed data." );

				}else
						;//It is orignal data , continue to do
			}
#endif
			re >> statusCode >> queryID;
			switch( statusCode ) {
			case SIMS_OK:
				if( true/*queryID == m_sessionInfo.id*/ ) {
					m_errLastError = Okay;
					if( reply->msg.simHeader.size > 8 )
					{
						if (!reUncompr)
							re >> trees;
						else 
							*reUncompr >> trees;
					}
					bDone = true;
				} else {
					m_errLastError = ServerError;
				}
				break;
			case SIMS_DENY:
				m_errLastError = LocalOffline;
				break;
			case SIMS_NOTFOUND:
			default:
				m_errLastError = PeerNotFound;
				break;
			}
		} catch ( const char * ) {
			// bad response data
			m_errLastError = NetworkError;
		}

		ReleaseMessage( reply );

		return bDone;
	} else {
		m_errLastError = Timeout;
	}

	return bDone;	
}

bool BBQClient::OnMsgMCCNotify( const SIM_REQUEST * req )
{
	const SIMD_SC_MCC_NOTIFY * pQ = (const SIMD_SC_MCC_NOTIFY *) & req->msg.simData;

	// invalid message, ignore.
	//if( 0 != memcmp( & pQ->sessionInfo, & m_sessionInfo, sizeof(m_sessionInfo) ) ) return true;

	BBQ_MeetingInfo mInfo;
	if( this->m_pAESKeyBytes && pQ->flags.is_encrypted ) { // the meeting info is encrypted

		bool bDone = AES_decrypt( (unsigned char *) & pQ->meetingInfo, sizeof(mInfo), 
			(unsigned char *) this->m_pAESKeyBytes, (unsigned char *) PI_SQRT2_STRING, this->m_nAESKeyBytes * 8, 
			(unsigned char *) & mInfo );

	} else {
		mInfo = pQ->meetingInfo;
	}

	// convert to local time
	time_t tNow = time(NULL);
	if( tNow > pQ->tNowSenderTime ) mInfo.tBeginTime += (tNow - pQ->tNowSenderTime);
	else mInfo.tBeginTime -= (pQ->tNowSenderTime - tNow);


	OnNotifyMCCEvent( & mInfo, pQ->eventId, pQ->nEventTime );

	return true;
}

bool BBQClient::OnMsgMCCNotifyEx( const SIM_REQUEST * req )
{
	SIMD_SC_MCC_NOTIFY_EX * pQ = (SIMD_SC_MCC_NOTIFY_EX *) & req->msg.simData;

	// invalid message, ignore.
	if( 0 != memcmp( & pQ->sessionInfo, & m_sessionInfo, sizeof(m_sessionInfo) ) ) return true;

	BBQ_MeetingInfo_Ext * p = & pQ->meetingInfoExt;
	BBQ_MeetingInfo_Ext_FIX( p );

	int n = sizeof(req->msg.simHeader) + sizeof(*pQ) - sizeof(pQ->meetingInfoExt) + p->size;

	// convert to local time
	time_t tNow = time(NULL);
	if( tNow > pQ->tNowSenderTime ) p->info.tBeginTime += (tNow - pQ->tNowSenderTime);
	else p->info.tBeginTime -= (pQ->tNowSenderTime - tNow);

	OnNotifyMCCEventExt( p, pQ->eventId, pQ->nEventTime );

	return true;
}

void BBQClient::NotifyMCCEvent( const BBQ_MeetingInfo * pInfo, uint32 nEventId )
{
	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_MCC_NOTIFY, sizeof(SIMD_CS_MCC_NOTIFY) );

	SIMD_CS_MCC_NOTIFY * pQ = (SIMD_CS_MCC_NOTIFY *) & req.msg.simData;
	memset( pQ, 0, sizeof(*pQ) );
	pQ->sessionInfo = m_sessionInfo;
	pQ->eventId = nEventId;
	pQ->meetingInfo = * pInfo;
	SendMessage( & req, true );
}
extern WORD g_httpPort ;

PString BBQClient::NotifyMCCEventCMD( const int roomid,uint32 nEventID,const char* strDoctitle, const char* httpURL )
{
   
	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_MCC_CMD, sizeof(SIMD_CS_MCC_CMD)  );

	SIMD_CS_MCC_CMD * pQ = (SIMD_CS_MCC_CMD *) & req.msg.simData;

	memset( pQ, 0, sizeof(*pQ) );
	pQ->sessionInfo = m_sessionInfo;
  pQ->eventId = nEventID;
	pQ->roomid = roomid;
  if (nEventID ==em_MCC_Add_PPT_PAGE){
    pQ->nHttplistenPortForMCU = g_httpPort;//for mcu use 
    PBytePack pack;
    pack << strDoctitle << httpURL;
    memcpy(pQ->data, pack.Data(), pack.Size());
    req.msg.simHeader.size+= pack.Size();
	  SendMessage( & req, true );
  }else if(nEventID ==em_MCC_UserEntered_room){
	  SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_MCC_CMD, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	  if( reply ) {
	    SIMD_SC_MCC_CMD * pQ = (SIMD_SC_MCC_CMD *) & reply->msg.simData;
      if (pQ->state >0){
        PString strDocs;
        PBytePack pack(pQ->data,  reply->msg.simHeader.size- sizeof(SIMD_SC_MCC_CMD) );
        m_nHttplistenPortForMCU= pQ->nHttplistenPortForMCU;
        for(int i=0;i< pQ->state;i++){
          char *pTitle, *pkey;
          int nCount=0;
          PString strDoc;
          pack>> pTitle>> pkey >>nCount;
          strDoc.sprintf("%s|%s|%d,",pTitle ,pkey,nCount);
          delete pTitle;pTitle=NULL;
          delete pkey;pkey=NULL;

          strDocs+=strDoc;
        }
        return strDocs;
      }
      ReleaseMessage( reply );
    }

  }
  else if (nEventID ==em_MCC_Remove_Room){
    
	  PostMessage( & req, true ); 
  }
  else if (nEventID ==em_MCC_Add_participator){
    pQ->nActinuID = atol(strDoctitle);//id by add 
  
	  PostMessage( & req, true ); 
  }
  return "";
}
void BBQClient::NotifyMCCBegin( const BBQ_MeetingInfo * pInfo )
{
	NotifyMCCEvent( pInfo, MCC_BEGIN );
}

void BBQClient::NotifyMCCEnd( const BBQ_MeetingInfo * pInfo )
{
	NotifyMCCEvent( pInfo, MCC_END );
}

bool BBQClient::UpdateCallForwardInfo( BBQCallForwardInfo & callfwdinfo )
{
	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_UPDATECALLFWDINFO, sizeof(SIMD_CS_UPDATECALLFWDINFO) );
	SIMD_CS_UPDATECALLFWDINFO * pQ = (SIMD_CS_UPDATECALLFWDINFO *) & req.msg.simData[0];
	memset( pQ, 0, sizeof(*pQ) );
	pQ->sessionInfo = m_sessionInfo;
	pQ->callfwdinfo = callfwdinfo;
	pQ->userID = m_sessionInfo.id;
	pQ->statusCode = SIMS_OK;

	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_UPDATECALLFWDINFO, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_UPDATECALLFWDINFO * pA = (SIMD_SC_UPDATECALLFWDINFO *) & reply->msg.simData[0];
		switch( pA->statusCode ) {
		case SIMS_OK:	
			m_errLastError = Okay;
			bDone = true;
			break;
		case SIMS_DENY:
			m_errLastError = PeerDenied;
			break;
		default:
			m_errLastError = IDNotAvailable;
		}
		ReleaseMessage( reply );
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::GetCallForwardInfo( uint32 nId, BBQCallForwardInfo & callfwdinfo, PIPSocket::Address svrIp, WORD svrPort )
{
	bool bDone = false;

	// if no server is specified, then use current server that I login
	DWORD dwServerIp = (DWORD) svrIp;
	if( dwServerIp == 0 ) {
		if( ! m_bLogin ) {
			m_errLastError = LocalOffline;
			return false;
		}

		svrIp = m_chanLogin.peer.ip;
		svrPort = m_chanLogin.peer.port;

		dwServerIp = m_chanLogin.peer.ip;
	}

	SIM_CHANNEL chan;
	if( ! PickOrCreateCSChannel( chan, svrIp, svrPort ) ) return false;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, chan, SIM_CS_GETCALLFWDINFO, sizeof(SIMD_CS_GETCALLFWDINFO) );
	SIMD_CS_GETCALLFWDINFO * pQ = (SIMD_CS_GETCALLFWDINFO *) & req.msg.simData[0];
	memset( pQ, 0, sizeof(*pQ) );
	pQ->sessionInfo = m_sessionInfo;
	pQ->userID = nId;
	pQ->statusCode = SIMS_OK;

	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_GETCALLFWDINFO, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_GETCALLFWDINFO * pA = (SIMD_SC_GETCALLFWDINFO *) & reply->msg.simData[0];
		switch( pA->statusCode ) {
		case SIMS_OK:	
			callfwdinfo = pA->callfwdinfo;
			m_errLastError = Okay;
			bDone = true;
			break;
		case SIMS_DENY:
			m_errLastError = PeerDenied;
			break;
		default:
			m_errLastError = IDNotAvailable;
		}
		ReleaseMessage( reply );
	} else {
		m_errLastError = Timeout;
	}

	return bDone;
}

bool BBQClient::OnMsgNotifyContact( const SIM_REQUEST * req )
{
	const SIMD_SC_NOTIFY_CONTACT * pQ = (const SIMD_SC_NOTIFY_CONTACT *) & req->msg.simData;

	switch( pQ->actionCode ) {
	case SIM_CS_ADDCONTACT:
	case SIM_CS_DELETECONTACT:
	case SIM_CS_ADDBLOCK:
	case SIM_CS_DELETEBLOCK:
		OnNotifyContactActionBy( pQ->userId, pQ->actionCode );
		break;
	}

	return true;
}

bool BBQClient::ConfirmContactActionBy( uint32 byId, uint32 nAction, bool bAccept )
{
	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_AUTH_CONTACT, sizeof(SIMD_CS_AUTH_CONTACT) );
	SIMD_CS_AUTH_CONTACT * pQ = (SIMD_CS_AUTH_CONTACT *) & req.msg.simData[0];
	memset( pQ, 0, sizeof(*pQ) );
	pQ->sessionInfo = m_sessionInfo;
	pQ->userId = byId;
	pQ->actionCode = nAction;
	pQ->statusCode = bAccept ? SIMS_OK : SIMS_DENY;

	PostMessage( & req, true, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	// return SendMessage( & req, true, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	return true;
}

void BBQClient::OnNotifyContactActionBy( uint32 byId, uint32 nAction )
{
	PString strMsg( PString::Printf, "%s by %d.", SIM_MSGNAME(nAction), byId );

	PTRACE( 1, strMsg );
}

void BBQClient::OnNotifyKickUser( uint32 uid  )
{
	PString strMsg( PString::Printf, "Received kick message (id:%u) from server.", uid );

	PTRACE( 1, strMsg );
}

bool BBQClient::LogUserActionToServer( const VfonUserActionLog * in )
{
	// send data: session, structure, string contents
	int nDataSize = sizeof( SIMD_CS_LOGUSERACTION );
	nDataSize += in->lpszTarget ? (strlen(in->lpszTarget) +1) : 1;
	nDataSize += in->lpszContent ? (strlen(in->lpszContent) +1) : 1;
	nDataSize += in->lpszExtra ? (strlen(in->lpszExtra) +1) : 1;

	int nReqSize = nDataSize + sizeof(SIM_CHANNEL) + sizeof(SFIDMSGHEADER);

	SIM_REQUEST * req = NULL;
	SIM_REQUEST_NEW( req, nReqSize );
	if( ! req ) return false;

	SIM_REQINITCHAN( * req, m_chanLogin, SIM_CS_LOGUSERACTION, nDataSize );
	SIMD_CS_LOGUSERACTION * pQ = (SIMD_CS_LOGUSERACTION *) & req->msg.simData;

	pQ->sessionInfo = m_sessionInfo;
	pQ->logInfo = * in;
	pQ->logInfo.msg_rand_id = (rand() | 0x01);
	char * p = & pQ->dataBlock[0];
	char* pStart = p;
	int nLen = 0;
	if( in->lpszTarget ) {
		nLen = strlen( in->lpszTarget )+1;
		memcpy( p, in->lpszTarget, nLen );
	} else {
		nLen = 1;
		*p = '\0';
	}
	pQ->logInfo.lpszTarget = (char*)(p - pStart);
	p += nLen;

	if( in->lpszContent ) {
		nLen = strlen( in->lpszContent ) + 1;
		memcpy( p, in->lpszContent, nLen );
	} else {
		nLen = 1;
		*p = '\0';
	}
	pQ->logInfo.lpszContent = (char*)(p - pStart);
	p += nLen;

	if( in->lpszExtra ) {
		nLen = strlen( in->lpszExtra ) + 1;
		memcpy( p, in->lpszExtra, nLen );
	} else {
		nLen = 1;
		*p = '\0';
	}
	pQ->logInfo.lpszExtra = (char*)(p - pStart);
	p += nLen;

	bool bDone = false;
	SIM_REQUEST * reply = RequestMessage( req, SIM_SC_LOGUSERACTION, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		uint32 statusCode = * (uint32 *) & reply->msg.simData[0];
		switch( statusCode ) {
		case SIMS_OK: 
			bDone = true;
			break;
		}
		ReleaseMessage( reply );
	}

	SIM_REQUEST_DELETE( req );
	return bDone;
}

bool BBQClient::SendOfflineIM( const BBQOfflineIMMessage * pIM )
{
	// send data: session, structure, string contents
	int nDataSize = sizeof( SIMD_IM ) + pIM->length;
	int nReqSize = nDataSize + sizeof(SIM_CHANNEL) + sizeof(SFIDMSGHEADER);

	SIM_REQUEST * req = NULL;
	SIM_REQUEST_NEW( req, nReqSize );
	if( ! req ) return false;

	SIM_REQINITCHAN( * req, m_chanLogin, SIM_CS_IM, nDataSize );
	SIMD_IM * pQ = (SIMD_IM *) & req->msg.simData;

	pQ->sessionInfo = m_sessionInfo;
	pQ->statusCode = SIMS_OK;
	pQ->nIndex = 0;
	memcpy( & pQ->imMsg, pIM, sizeof(*pIM) + pIM->length - 1 );

	bool bDone = false;
	SIM_REQUEST * reply = RequestMessage( req, SIM_SC_IM, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_IM * p = (SIMD_IM *) & reply->msg.simData[0];
		switch( p->statusCode ) {
		case SIMS_OK: 
			bDone = true;
			break;
		}
		ReleaseMessage( reply );
	}

	SIM_REQUEST_DELETE( req );
	return bDone;
}

BBQOfflineIMMessage * BBQClient::GetOfflineIM( int nIndex ) // free() after use if not NULL
{
	BBQOfflineIMMessage * pIM = NULL;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_GET_IM, sizeof(SIMD_IM) );
	SIMD_IM * pQ = (SIMD_IM *) & req.msg.simData[0];
	pQ->sessionInfo = m_sessionInfo;
	pQ->statusCode = SIMS_OK;
	pQ->nIndex = nIndex;
	pQ->imMsg.length = 0;

	bool bDone = false;
	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_GET_IM, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_IM * p = (SIMD_IM *) & reply->msg.simData[0];
		switch( p->statusCode ) {
		case SIMS_OK: 
			bDone = true;
			int n = sizeof(BBQOfflineIMMessage) + p->imMsg.length -1;
			pIM = (BBQOfflineIMMessage *) malloc( n );
			if( pIM ) {
				memcpy( pIM, & p->imMsg, n );
			}
			break;
		}
		ReleaseMessage( reply );
	}
	return pIM;
}

bool BBQClient::DeleteOfflineIM( void )
{
	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_DEL_IM, sizeof(SIMD_IM) );
	SIMD_IM * pQ = (SIMD_IM *) & req.msg.simData[0];
	pQ->sessionInfo = m_sessionInfo;
	pQ->statusCode = SIMS_OK;
	pQ->nIndex = 0;
	pQ->imMsg.length = 0;

	bool bDone = false;
	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_DEL_IM, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_IM * p = (SIMD_IM *) & reply->msg.simData[0];
		switch( p->statusCode ) {
		case SIMS_OK: 
			bDone = true;
			break;
		}
		ReleaseMessage( reply );
	}
	return bDone;
}

bool BBQClient::OnMsgNotifyOfflineIM( const SIM_REQUEST * req )
{
	SIMD_IM * pQ = (SIMD_IM *) & req->msg.simData[0];

	if( m_sessionInfo.cookie == pQ->sessionInfo.cookie ) {
		OnNotifyOfflineIM( pQ->nIndex, pQ->nTotal, & pQ->imMsg );
	}

	return true;
}

bool BBQClient::OnMsgNotifyKickUser( const SIM_REQUEST * req )
{
	SIMD_SC_KICK * pQ = (SIMD_SC_KICK *) & req->msg.simData[0];

	OnNotifyKickUser(m_sessionInfo.id);

	return true;
}
bool BBQClient::GetMCCInfo( const PString & strMeetingID, SIMD_SC_GET_MCC* pData  )
{
	SIM_REQUEST req;
  SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_GET_MCC, sizeof(SIMD_CS_GET_MCC) );
	SIMD_CS_GET_MCC * pQ = (SIMD_CS_GET_MCC *) & req.msg.simData[0];
	pQ->sessionInfo = m_sessionInfo;
  strncpy(pQ->strMeetingId, strMeetingID, sizeof(pQ->strMeetingId));

	bool bDone = false;
	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_GET_MCC, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_GET_MCC * p = (SIMD_SC_GET_MCC *) & reply->msg.simData[0];
    if (p->eventId != MCC_FAIL)
    {
      memcpy(pData, p, sizeof(SIMD_SC_GET_MCC));
      bDone= true;
    }
		ReleaseMessage( reply );
	}
	return bDone;
}
void BBQClient::OnUserDefinedMessageArrive( time_t tServerTime, uint32 nMeaning, uint32 nLength, const char * pData, VFONID fromId, const char * lpszAlias ) 
{
  switch ( nMeaning)
  {
    case D_USERDEFINEDMESSAGE_1:
      {
          DWORD ip=0;WORD port=0;
        if (sizeof(ip) + sizeof(port) ==nLength )
        {
          memcpy(&ip, pData,sizeof(ip)); memcpy(&port, pData+sizeof(ip),sizeof(port));
          Connect(PIPSocket::Address( ip), port, 1000);
        }
      }
    break;
  };
}
bool BBQClient::leftRoom(const uint32 roomid )
{
	if( m_bTcpLogin ) {
		// check the connection before sending data, if not available, then reconnect
		if(! ValidateTcpConnection()) return false;
	}

	bool bDone = false;

	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_LEFT_ROOM, sizeof(SIMD_CS_LEFT_ROOM) );
	SIMD_CS_LEFT_ROOM * pQ = (SIMD_CS_LEFT_ROOM *) & req.msg.simData;
	memset( pQ, 0, sizeof(SIMD_CS_ENTER_ROOM) );
  pQ->sessionInfo = m_sessionInfo;
  pQ->nRoomID =roomid;
	//strcpy( pQ->roomlist, members );

	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_LEFT_ROOM, CLIENT_SIMPLE_REQUEST_TIMEOUT + m_nNetworkBaseDelay * 2 );
	if( reply ) {
		SIMD_SC_LEFT_ROOM * pA = (SIMD_SC_LEFT_ROOM *) & reply->msg.simData;
    //start invited
    //g_clientExtern->OnRecvCallers(pA->calllist, pA->caller);
		ReleaseMessage( reply );
	} else 	{
		m_errLastError = NetworkError;
	}

	return bDone;
}
bool BBQClient::OnMsgNotifyEnterRoom( const SIM_REQUEST * req )
{
	const SIMD_SC_ENTER_ROOM * pQ = (const SIMD_SC_ENTER_ROOM *) & req->msg.simData;


//  OnRecvCallers(pQ->calllist, pQ->caller);
	return true;
}

bool BBQClient::BBQAES_encrypt( const char * psrc, const unsigned int len, char* pOut )
{
  char out[1024]={0};
  
   if ( AES_encrypt( (unsigned char *) psrc, len, (unsigned char *) m_AESkey, (unsigned char *) PI_SQRT2_STRING_EX, 128, (unsigned char *)  out ))
   {
    PKI_encode((unsigned char *)out, len, (unsigned char *)pOut);
    return true;
   }

   return false;
}
bool BBQClient::BBQAES_decrypt( const char * psrc, const unsigned int len, char* pOut )
{
  char out[1024]={0};

   int len2= PKI_decode((unsigned char *)psrc, len, (unsigned char *)out);
   if( len>0 &&  AES_decrypt( (unsigned char *)out, len2, (unsigned char *) m_AESkey, (unsigned char *) PI_SQRT2_STRING_EX, 128, (unsigned char *)  pOut ) )
   {
     return true;
  }
   return false;
}
bool BBQClient::OnMsgDownloadFileNotify( const SIM_REQUEST * req )
{

	const SIMD_SC_DOWNLOADFILE_NOTIFY * pQ = (const SIMD_SC_DOWNLOADFILE_NOTIFY *) & req->msg.simData;
	if (pQ->sessionInfo.id != m_sessionInfo.id)
		return true;
	OnMsgDownloadFile(pQ->oringnalfilename, pQ->pages, pQ->sendtime, pQ->nFrom);

	PTRACE(1, "bbqclient\t we are downloading the file from "<<pQ->oringnalfilename<<".\n");
	return true;
}
bool BBQClient::OnMsgSendFileMessage( const SIM_REQUEST * req )
{
	// check the msg source for security reason
	// if( req->channel.peer.ip != m_chanLogin.peer.ip ) return true;
	const SIMD_SC_SENDFILE * pQ = (const SIMD_SC_SENDFILE *) & req->msg.simData;
	{
		OnMsgSendFileProcess(pQ->oringnalfilename, pQ->pages, pQ->sendtime, pQ->status, pQ->nTo);
	}
#ifdef _DEBUG
	uint32 status = pQ->status&0xff , nProcess= pQ->status >>16 & 0xff;
	PTRACE(1, "bbqclient\t we are sending the file  "<<pQ->oringnalfilename<< ". the process is " << nProcess<<"%.seq is " << pQ->seq);
#endif
	return true;
}
bool BBQClient::OnMsgSendFileP2P( const SIM_REQUEST * req )
{
	// check the msg source for security reason
	// if( req->channel.peer.ip != m_chanLogin.peer.ip ) return true;
	const SIMD_CC_SENDFILE * pQ = (const SIMD_CC_SENDFILE *) & req->msg.simData;
	{
		OnReceivingFileP2P(pQ);
	}
	return true;
}
bool BBQClient::OnMsgRequestViewer( const SIM_REQUEST * req )
{
  BBQPROXY_CONNECT *pQ = (BBQPROXY_CONNECT*) req->msg.simData;
	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, UCM_MC_CONNECT, sizeof(BBQPROXY_CONNECT), req );
  if (pQ->nChannelId == 0)   SendMessage(&reply);//only connect msg to reply

  OnRequestViewer(pQ->uid, pQ->uidPeer, req->channel.socket,   pQ );
	return true;
}
bool BBQClient::OnMsgFreeGroupCmd( const SIM_REQUEST * req )
{
  SIMD_SC_FREEGROUP_NOTIFY *pQ = (SIMD_SC_FREEGROUP_NOTIFY*) req->msg.simData;
  
  OnFreeGroupCmd(pQ  );
	return true;
}
bool BBQClient::OnMsgMeetingControl( const SIM_REQUEST * req )
{
  SIMD_SC_MEETCTRL *pQ = (SIMD_SC_MEETCTRL*) req->msg.simData;
  
  OnMeetingControl(pQ->info , pQ->cmd );
	return true;
}

bool BBQClient::OnNotifyMCCEventCMD( const SIM_REQUEST * req )
{
  SIMD_SC_MCC_CMD *pQ = (SIMD_SC_MCC_CMD*) req->msg.simData;
 
  OnNotifyMCCEventCMD(pQ->roomid, pQ->eventId,  pQ,req );
	return true;
}YYFILE::YYFILE(BBQClient *p)
{
	memset(this, 0, sizeof(YYFILE) );
	currectseqs = new std::list<uint32>();
	m_Owner = p;m_pStrSavePath = new PString();
}
YYFILE::~YYFILE()
{
	if (currectseqs) delete currectseqs;
	if (pData) delete pData;
	if (pChannel) 
	{
		if (pChannel->tcpSock) m_Owner->DetachTcpListener(pChannel->tcpSock);
		if (pChannel->sock) m_Owner->DetachUdpListener(pChannel->sock);
		m_Owner->ReleaseChannel(pChannel);
	}
	delete m_pStrSavePath;
}

 void BBQClient:: SendMeetingCtrl(int cmd, const char* msg)
 {
 		SIM_REQUEST * reply = NULL;
	{
		SIM_REQUEST req;
		//SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_VERSIONINFO, 0 );
		SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_MEETCTRL, sizeof(SIMD_CS_MEETCTRL) );
    SIMD_CS_MEETCTRL* pA =(SIMD_CS_MEETCTRL*) req.msg.simData;
    pA->sessionInfo = m_sessionInfo;
    pA->cmd= cmd;
    strncpy(pA->info, msg, sizeof(pA->info));
    for(int i=0;i<3;i++ )
		  SendMessage( & req);
	}
 }
  
