
#ifndef _BBQ_CLIENT_H
#define _BBQ_CLIENT_H

#include <ptclib/pssl.h>

#include "msgterminal.h"
#include "bbqidmsg.h"
#include "useridlist.h"
#include "bbqtree.h"
#include "md5.h"
#include "bbqmcc.h"
#include <list>
#define	D_USERDEFINEDMESSAGE_1	0xfffffff1
#define	D_USERDEFINEDMESSAGE_2	0xfffffff2// request speaker in  mcc ,user -->chairman
#define	D_USERDEFINEDMESSAGE_3	0xfffffff3// respone speaker in  mcc ,chairman --> user
#define	D_USERDEFINEDMESSAGE_4	0xfffffff4// do sync videopanel layout  
#define	D_USERDEFINEDMESSAGE_5	0xfffffff5// save cdrs to DB
#define	D_USERDEFINEDMESSAGE_6	0xfffffff6// client收到消息 弹出聊天框  

struct BBQPROXY_CONNECT;
typedef struct BBQ_CHANNEL {
	PUDPSocket				* sock;				// not null if udp
	PTCPSocket				* tcpSock;			// not null if tcp
	PSSLChannel				* sslChannel;		// not null if tcp ssl
	struct tagInfo {
		bool				bIsProxied;			// is proxied, or P2P
		bool				bInitFromLocal;		// channel requested from local
		//uint32			nChannelId;			// channel id of init party

		uint32				peerServerId;		// the server that peer client login
		uint32				peerId;				// peer client id

		IPSOCKADDR			peerWAN;			// WAN ip:port of peer client
		IPSOCKADDR			peerLAN;			// LAN ip:port of peer client
		IPSOCKADDR			thisLAN;			// local ip:port

		DWORD				dwLocalTimeBase;
		DWORD				dwLocalCount;
		DWORD				dwRemoteTimeBase;
		DWORD				dwRemoteCount;

		uint32				nLocalAppParam;		
		uint32				nRemoteAppParam;
	} info;

	PByteBlock				* pExtraData;

} BBQ_UDPCHANNEL, BBQ_TCPCHANNEL, BBQ_CHANNEL;

#define CHANNEL_SETUP_TIME_MAX	10000	// 10 seconds
class BBQClient;
// TODO: will replace UserStatusList with this new data structure
struct YYFILE
{
	BBQClient *m_Owner;
	char oringnalfilename[64];//文件名
	char oringnalfilenameInServer[96];//文件名
	uint32 filetotalsize;//创建的时间
	uint32 currectfiletotalsize;//创建的时间
	std::list<uint32>* currectseqs;//当前收到的包
	PString* m_pStrSavePath;
	short totalpages;//页码
	uint32 sendtime;//创建的时间
	uint32 m_nTo;//receiver
	uint32 m_nFrom;//sender
	char	page;//page in the file
	bool isOK;
	char* pData;//file data
	time_t lasttime;
	BBQ_CHANNEL* pChannel;
	YYFILE(BBQClient *p);
	~YYFILE();
};
typedef std::list<YYFILE*>						VfonP2PList;
typedef std::list<ONLINE_USER *>				UserStatusList;
typedef std::list<ONLINE_USER_EXT >				UserStatusListExt;
typedef std::list<ONLINE_USER *>::iterator		of_iterator;
typedef std::list<ONLINE_USER_EXT>::iterator	ext_of_iterator;

typedef std::list<IPSOCKADDR>					GKServerList;
typedef std::list<IPSOCKADDR>::iterator			GKSL_iterator;

typedef std::list<IPSOCKADDR>					ProxyServerList;
typedef std::list<IPSOCKADDR>::iterator			PSL_iterator;

typedef std::list<MCURecord *>					MCUList;


#ifdef ANTI_PACKET_LOSS
// map channel id to time stamp, clear older than 30s evey minute
typedef std::map<UNIQUE_CID, MS_TIME, less_than_unique_cid> UNIQUE_CID_TIME_MAP;
#endif

//#define CACHE_USERINFO

typedef struct CS_CHAN_INFO {
	SIM_CHANNEL			channel;
	MS_TIME				timestamp;
} CS_CHAN_INFO;

class BBQClient : public BBQMsgTerminal
{
	PCLASSINFO( BBQClient, BBQMsgTerminal );

public:
	BBQClient( unsigned int nMsgQueueMax = 1024, int nHandlers = 64,const char* szServerIP=0 );
	virtual ~BBQClient();
	class BBQChannelWait : public PSemaphore
	{
		PCLASSINFO( BBQChannelWait, PSemaphore )

	public:
		// called by the wait thread
		BBQChannelWait( PUDPSocket * pSock, BBQClient & owner );
		virtual ~BBQChannelWait();

		BBQ_CHANNEL* Detach( void );

		BBQClient &			m_owner;

		UCMD_CHANNELINFO	info;
		UCMD_CHANNELINFO	peerInfo;

		UCMD_CHANNEL_EXTRA	extra;
		UCMD_CHANNEL_EXTRA	peerExtra;

		bool				m_bIsProxied;
		bool				m_bDone;
		bool				m_bConnectingProxy;

		PUDPSocket*			m_pSock;
		PTCPSocket*			m_pTcpSock;
		PSSLChannel*		m_pSSLChannel;

		int					m_nAppCode;
		PString				m_strAppString;

		MS_TIME				m_msWaitBeginTime;

	};
	
	BBQACLFlag		m_accessFlags;

	enum ErrorCode {
		Okay,
		InvalidPortRange,
		InvalidArgument,
		NoServerSpecified,
		ServerNotFound,
		NetworkError,
		Timeout,
		IDNotAvailable,
		InvalidRegKey,
		InvalidAlias,
		BothStrictFirewall,
		LocalOffline,
		PeerNotFound,
		PeerOffline,
		PeerDenied,
		ServiceExpired,
		NetworkChanged,
		ServerError,
		ServerSwitch,
		ServerCertError,
		ComprMsgError
	};

	ErrorCode GetLastErrorCode( void ) { 
		return m_errLastError; 
	}

	PString GetLastErrorString( void );

	int GetLastErrorLoginCode( void ) { 
		return m_errLastLoginCode; 
	}

	enum FirewallType {
		NoFirewall,
		WeakPolicy,
		MediumPolicy,
		StrictPolicy,
	};

	bool				m_bForceBBQProxy;			// force channel connect to peer using proxy, if no proxy, will failed setup channel
	bool				m_bForceTcp;				// force channel connect to peer direct TCP, might fail if peer port cannot access

	char				m_lpszMyPassword[ USERDATA_NAME_SIZE ];

protected:
	int						m_errLastLoginCode;

	static BBQClient *		m_pCurrentClient;

	uint32					m_nProviderID;
	BBQUserTechParamInfo	m_techInfo;

	WORD				m_nPortBase, m_nPortMax, m_nPortNext;

	ErrorCode			m_errLastError;
	PString				m_errLastErrorString;

	PString				m_strServerHostname;

	uint16				m_nDefaultUdpPort;

	// channel to my login server, might be tcp or udp
	SIM_CHANNEL			m_chanLogin;

	bool				m_bServerTested;
	FirewallType		m_nFirewallType;
	IPSOCKADDR			m_netWAN;

	bool				m_bUdpBlocked;
	SIM_CHANNEL			m_chanHeartbeat;	// udp preferred if udp is allowed

	// server PKI cert, public key for RSA encryption
	void *				m_pX509_server_cert;
	char *				m_lpszX509Text;

	char *				m_pAESKeyBytes;
	uint32				m_nAESKeyBytes;

	uint32				m_nNetworkBaseDelay;

	bool				m_bTcpLogin;			// set to true if use TCP login

	bool				m_bChannelSSL;			// force request SSL channel

	bool				m_bBeingLogout;

	// list of channels to other servers for query or requesting channel
	// map IP to ...
	typedef std::map<DWORD, CS_CHAN_INFO>	IPCHANMAP;
	IPCHANMAP			m_mapChanOtherServers;

	bool				m_bLogin;
	bool				m_bHeartbeatOkay;
	uint32				m_nLastStatusCode;

	SIMD_CS_LOGIN		m_regInfo;
	SIM_SESSIONINFO		m_sessionInfo;

	uint32				m_nServerId;

	// counter & time for heartbeat
	int					m_nHeartbeatCounter;
	int					m_nHeartbeatTimeInterval;
	time_t				m_tHeartbeatOkayTime;

	int					m_nAliveCounter;
	int					m_nKeepAliveTimeInterval;

	DWORD				m_dwTimeBase;

	// channel request
	mutable PMutex					m_mutexChannelWait;
	uint16							m_nNextChannelId;
	list<BBQChannelWait *>			m_listChannelWaits;
	typedef std::list<BBQChannelWait *>::iterator	cw_iterator;

#ifdef ANTI_PACKET_LOSS
	// this is used for skip duplicate request
	mutable PMutex					m_mutexCidTime;
	UNIQUE_CID_TIME_MAP				m_mapCidTime;
#endif

	mutable PMutex		m_mutexVars;
	UserStatusList		m_listFriends;
	UserIdList			m_userlistBlocks;
  std::map<uint32, BBQUserInfo> m_UserInfoCache;
#ifdef CACHE_USERINFO
	UserIdList			m_userlistCache;
	PString				m_strCacheIndexFile; // cache.idx
	PString				m_strCacheDataFile;	 // cache.dat
#endif

	bool				m_bVerifyServerCert;
	int					m_nServerCertVerifyStatus;
	bool				m_bSupportCompr;
	uint32				m_backupMyIP;
	uint16				m_backupMyPort;
	PStringArray  m_arConferenceStatus;
	PMutex        m_mutexConferenceStatus;

	std::map<uint32, IPSOCKADDR>	m_mapDomainServers;
	PCriticalSection				m_csDomainServers;
public:
  bool GetShowerOrder(PStringArray& arr ,unsigned int roomid);
	void AddDomainServer(uint32 serverId, IPSOCKADDR domain)
	{
		PEnterAndLeave cs(m_csDomainServers);
		m_mapDomainServers[serverId] = domain;
	}
	IPSOCKADDR GetDomainServer(uint32 serverId)
	{
		IPSOCKADDR domain;
		if( m_mapDomainServers.count(serverId) > 0 ) {
			domain = m_mapDomainServers[serverId];
		} else {
			domain.ip = 0;
			domain.port = SIM_PORT;
		}
		return domain;
	}
	void RemoveDomainServer(uint32 serverId)
	{
		PEnterAndLeave cs(m_csDomainServers);
		m_mapDomainServers.erase(serverId);
	}
	void RemoveAllDomainServers()
	{
		PEnterAndLeave cs(m_csDomainServers);
		m_mapDomainServers.clear();
	}

	void     SetCurrectConferenceStatus(const PStringArray& strValue){LockIt lock(m_mutexConferenceStatus); m_arConferenceStatus = strValue;};
	PStringArray     GetCurrectConferenceStatus( ){LockIt lock(m_mutexConferenceStatus);return m_arConferenceStatus;};
	int	GetServerCertVerifyStatus() { return m_nServerCertVerifyStatus; }
	void SetVerifyServerCert(bool bVerifyServerCert) { m_bVerifyServerCert = bVerifyServerCert; }
	bool IsVerifyServerCert() { return m_bVerifyServerCert; }

	void SetTcpLogin( bool bTcp = true );
	bool IsTcpLogin( void );

	void SetChannelSSL( bool bSSL = true ) { m_bChannelSSL = bSSL; }
	bool IsChannelSSL( void ) { return m_bChannelSSL; }

	static uint32 StatusStringToInt( const char * str );

	static const char * StatusIntToString( uint32 n );

	static BBQClient* GetCurrentClient( void );

	void Close();

	void SetTechInfo( const BBQUserTechParamInfo * pInfo, uint32 nProviderID = 0 );

	virtual bool BindDynamicUdpPort( PUDPSocket* & pSock, WORD & port, const PIPSocket::Address & bind =  INADDR_ANY );
	bool ReBindDefaultPort( void );
	bool SetPortRange( WORD nPortBase, WORD nPortMax );

	uint32 GetNetworkBaseDelay( void );
	uint32 SetNetworkBaseDelay( uint32 nDelayMs );

	bool SetServerAddress( DWORD svraddr, WORD port = SIM_PORT, bool bTest = false );

	// another interface especially for client with proxy
	// to use HTTP-PROXY or SOCKS-PROXY, the proxy is responsible for DNS resolving, so must send hostname to proxy, 
	// and in this case, default server port is Tcp/443
	bool SetServerAddress( const PString & svraddr, WORD port, bool bTcp = false );

	bool QueryVersionInfo( SIMD_VERSIONINFO * pVersionInfo, PString & lpszDownloadURL );

	bool BBQPing( PIPSocket::Address svraddr, WORD svrport = SIM_PORT );

	bool BBQPing( PIPSocket::Address & myIp, WORD & myPort, 
						PIPSocket::Address svraddr, WORD svrport = SIM_PORT );

	bool BBQPing( PIPSocket::Address & anotherServerIp, WORD & anotherServerPort, 
						PIPSocket::Address & myIp, WORD & myPort, 
						PIPSocket::Address svraddr, WORD svrport = SIM_PORT );

	bool QueryServerStatus( SIMD_SC_SERVERSTATUS & out, PIPSocket::Address addr = 0, WORD port = SIM_PORT );
	bool QueryProxyStatus( SIMD_SC_PROXYSTATUS & out, PIPSocket::Address addr = 0, WORD port = SIM_PORT );

	bool CommandSwitchServer( SIMD_CS_SERVERSWITCH & in, SIMD_SC_SERVERSWITCH & out );
	bool CommandSwitchServerActive( const char * lpszUsername, const char * lpszPassword );

	bool TestServer( PIPSocket::Address & local_addr, PIPSocket::Address & wan_addr, FirewallType & firewall, PIPSocket::Address svraddr, WORD svrport = SIM_PORT ); 
	bool TestServer( PIPSocket::Address addr, WORD port = SIM_PORT );
	bool GetFirewallInfo( PIPSocket::Address & local, PIPSocket::Address & wan, FirewallType & firewall );

	bool PickOrCreateCSChannel( SIM_CHANNEL & chan, const PIPSocket::Address & svrIp, WORD svrPort, WORD localPort = 0 );
	void ReleaseCSChannel( SIM_CHANNEL & chan );

	bool ValidateChannel( SIM_CHANNEL & chan );
	bool ValidateTcpConnection( void );

	bool Login( uint32 nSerial, const char * strKey, uint32 & uid, uint32 nServiceType = SERVICE_SOFTWAREBOUNDLE, uint32 nStatus = CLIENT_NORMAL );
	virtual void Logout( void );
	bool ChangeStatus( uint32 statusCode, const char * lpszString );

	bool IsLogin( void );
	uint32 GetID( void ) { return m_sessionInfo.id; }

	virtual bool GetLocalProxyInfo( SIMD_CS_HEARTBEAT * pQ );	// vfon proxy pls override to fill pQ
	virtual bool GetLocalMcuInfo( SIMD_CS_HEARTBEAT * pQ );		// mcu pls override to fill pQ
	virtual bool SendHeartbeat( void );

	bool KeepAlive( void );

	bool UpdateUserData( BBQUserInfo & userInfo ,const PString & strAliasExt);

	bool UploadDataBlock( const char * key, const char * digest, uint32 data_len, const char* pData );
	bool DownloadDataBlock( const char * key, const char * digest, uint32 & data_len, char* & pData ); // after usage, must delete[] pData if non-zero.

	// offline instant message
	bool SendOfflineIM( const BBQOfflineIMMessage * pIM );
	virtual void OnNotifyOfflineIM( int nIndex, int nTotal, const BBQOfflineIMMessage * pIM ) {}; // override to handle the notification
	BBQOfflineIMMessage * GetOfflineIM( int nIndex ); // delete after use if not NULL
	bool DeleteOfflineIM( void );

	bool UpdateMyTagString( const char * pStr );
	bool QueryProxyTagString( PString & strTag, const PString & svraddr, WORD svrport );

	bool UpdateCallForwardInfo( BBQCallForwardInfo & callfwdinfo );
	bool GetCallForwardInfo( uint32 nId, BBQCallForwardInfo & callfwdinfo, PIPSocket::Address svrIp = 0, WORD svrPort = 0 );

	bool AddFriend( uint32 uid, const char * lpszText = NULL );
	bool RemoveFriend( uint32 uid );
	bool GetFriendList( UserStatusList & thelist, bool bForceRefresh = false ); 

	bool IsFriendOnline( uint32 uid );
	uint32	GetFriendStatus( uint32 uid );

	bool QueryServiceInfo( BBQACLFlag & aclFlag, BBQUserServiceInfo & serviceInfo );

	bool AddBlock( uint32 uid );
	bool RemoveBlock( uint32 uid );
	bool GetBlockList( UserIdList & list, bool bForceRefresh = false );

	void ClearCache( void );
	void SetCacheFilePath( PString strIndexFile, PString strDataFile );
	bool ViewUserData( uint32 uid, BBQUserInfo & userInfo, bool bForceRefresh = false );
	bool ViewUserDataEx( uint32 uid, BBQUserInfo & userInfo, VfonIdBindInfo & bindInfo,PStringArray & voipinfo,PString& strAlias, bool bForceRefresh );

	bool GetUserTechParam( BBQUserTechParamInfo & techInfo, uint32 uid = 0 );

	bool ListOnlineUser( uint32 nStart, PBytePack & pack );
	bool SearchUser( const BBQSearchCondition * pFilter, uint32 nStart, PBytePack & pack );

	// first time call, MUST init pRecs=NULL, nStart=0; 
	// after referrence, if pRecs == NULL, MUST delete[] pRecs; 
	bool ListOnlineUser( uint32 & nStart, uint32 & nTotal, uint32 & nCount, BBQUserBriefRecord* & pRecs );
	bool SearchUser( const BBQSearchCondition * pFilter, uint32 & nStart, uint32 & nTotal, uint32 & nCount, BBQUserBriefRecord* & pRecs );

	// send end-to-end user defined data block (encrypted or plain)
	bool SendUserDefinedMessage( uint32 nEncryptMethod, uint32 nMeaning, uint32 nLength, const char * pData, uint32 uid, uint32 serverid = 0, PIPSocket::Address svrIp = 0, WORD svrPort = SIM_PORT ); 

	bool RequestConversation( uint32 uid, uint32 nContentId, const char * lpszText );

	// --- find location of others ------------------------------------------------------
	// the default server is the same as the login,
	// but specifying another server is permitted, so that cross-domain locate & request is possible
	bool LocateIP( PIPSocket::Address & addr, PIPSocket::Address & addrLAN, uint32 uid, PIPSocket::Address svrIp = 0, WORD svrPort = SIM_PORT );
	bool Locate( SIMD_SC_LOCATE & info, uint32 uid, PIPSocket::Address svrIp = 0, WORD svrPort = SIM_PORT, uint32 nFrom = 0 );

	BBQ_CHANNEL * DirectUdpConnect( uint32 uid, uint16 nAppCode, const char * lpszAppString, uint32 nAppParam,
		DWORD timeout, 
		PIPSocket::Address ip, WORD port );

	BBQ_CHANNEL * DirectTcpConnect( uint32 uid, uint16 nAppCode, const char * lpszAppString, uint32 nAppParam,
		DWORD timeout,
		PIPSocket::Address ip, WORD port, bool bHost );

	enum ChannelMode {
		UDP_P2P		= 0x01,
		TCP_P2P		= 0x02,
		UDP_PROXY	= 0x04,
		TCP_PROXY	= 0x08,
		UDP_PROXY_AUTO = 0x10,
		TCP_PROXY_AUTO = 0x20,
		PROXY_FORCE = 0x40,
		TCP_FORCE = 0x80,
	};
  virtual BBQ_CHANNEL * RequestChannelForTestingProxy( uint32 uid, PIPSocket::Address proxyIp,bool bTestUdp = true,
										  DWORD timeout = CHANNEL_SETUP_TIME_MAX,  PIPSocket::Address svrIp = 0, WORD svrPort = SIM_PORT );
	// this will block until request okay or failed.
	BBQ_CHANNEL * RequestChannel( uint32 uid, uint16 nAppCode, const char * lpszAppString, uint32 nAppParam,
		DWORD timeout = CHANNEL_SETUP_TIME_MAX,
		DWORD modes = UDP_P2P | UDP_PROXY_AUTO, 
		PIPSocket::Address svrIp = 0, WORD svrPort = SIM_PORT,  PIPSocket::Address proxyIp =0);

	// call this to break the above request process
	void BreakChannelRequesting( uint32 uid = 0, uint16 nAppCode = 0, const char * lpszAppString = NULL );

	// after use the requested channel, call this to release it.
	void ReleaseChannel( BBQ_CHANNEL * pChannel );

	// get gatekeeper servers list
	bool GetGKServerList(GKServerList& list);

	// get proxy servers list
	bool GetProxyList(ProxyServerList& list);

	bool Subscribe( const SIMD_CS_SUBSCRIBE * in, SIMD_SC_SUBSCRIBE * out );

	bool TestUdpForTcpLogin( void );

	bool LoginEx( const SIMD_CS_LOGIN_EX * in, SIMD_SC_LOGIN_EX * out  );
	bool EnterRoom( const uint32 roomid   );
	bool leftRoom( const uint32 roomid   );

	bool DownloadServerPKICert( void );
	const char * GetServerPKICertText( void );

	bool LoginExSecure( const SIMD_CS_LOGIN_EX * in, SIMD_SC_LOGIN_EX * out, uint16 nDataSecureType = SECRET_AES256, uint16 nKeySecureType = SECRET_RSA  );

	bool NotifyReady( void );

	bool MeetingInvite( uint32 uid, const BBQ_MeetingInfo * pM );

	bool LocateEx( const SIMD_CS_LOCATE_EX * in, SIMD_SC_LOCATE_EX * out );

	bool ConfirmLocationDirectly( uint32 uid, SIMD_SC_LOCATE & locationInfo );

	bool NotifyServerConferenceChange( const SIMD_CS_CONFERENCE * in );

	bool LogUserActionToServer( const VfonUserActionLog * in );

	bool DownloadString( const PString & strName, PString & strValue );

	bool GetBindInfoByVfonId( VFONID vfonid, VfonIdBindInfo * bindInfo );

	bool GetVfonIdByBindInfo( const VfonIdBindInfo * bindInfo, VFONID * pId );

	bool GetMCUList( std::list<MCURecord *> & listMCUs );

	bool GetIdTrees( BBQTrees & trees );

	// override this function, follow server's notify, to cut one or more vfon session in the conference
	virtual void ConferenceChangeByServer( const SIMD_CS_CONFERENCE * in ) {};

	void NotifyMCCBegin( const BBQ_MeetingInfo * pInfo );
	void NotifyMCCEnd( const BBQ_MeetingInfo * pInfo );	

	bool ConfirmContactActionBy( uint32 byId, uint32 nAction, bool bAccept );
	bool GetMCCInfo( const PString & strMeetingID  , SIMD_SC_GET_MCC* pData);
  bool QueryStringToStringInfo(const char* strInfoKey, PString & strKeysandValues );
	void SetSupportCompressMsg(bool bCompr){m_bSupportCompr = bCompr;};
	bool IsSupportCompressMsg( ){return m_bSupportCompr;};
  virtual PString NotifyMCCEventCMD( const int roomid,uint32 nEventID,const char* strDoctitle, const char* httpURL);
 
  uint16       m_nHttplistenPortForMCU;

protected:

	void NotifyMCCEvent( const BBQ_MeetingInfo * pInfo, uint32 nEventId );

	DECLARE_BBQ_MESSAGE_MAP()

	// called when the connetion is closing (by local) or closed (by peer)
	virtual void OnMsgConnectionClose( BBQMsgConnection * pConn );

	virtual void OnTick( void );

	//virtual void OnHeartbeatFeedback( UserIdList & onlineList ) {};
	virtual void OnHeartbeatFeedback( UserStatusList & onlineList ) {};
	virtual void OnHeartbeatTimeout( void ) {};
	virtual void OnSessionExpired( void ) {};
	virtual void OnLoginReplaced( PIPSocket::Address who ) {};

	virtual void OnNotifySwitchServerWhenLogin( PIPSocket::Address another, WORD port = SIM_PORT ) {};
	virtual void OnNotifySwitchServerWhenHeartbeat( PIPSocket::Address another, WORD port = SIM_PORT ) {};

	// for MCC, invitor of the conference will receive the notifying message
	// vmeet client or old version of MCU please override this one
	virtual void OnNotifyMCCEvent( const BBQ_MeetingInfo * pInfo, uint32 nEvent, uint32 nEventTime ) {};// MCC_INIT, MCC_BEGIN, MCC_INITEND, MCC_END, MCC_FAIL

	// only MCU will receive this message, cause it will contain all the info of roles and users in a conference
	// new version of MCU please override this one
	virtual void OnNotifyMCCEventExt( const BBQ_MeetingInfo_Ext * pMExt, uint32 nEvent, uint32 nEventTime ) {};// MCC_INIT, MCC_BEGIN, MCC_INITEND, MCC_END, MCC_FAIL

	virtual void OnUserDefinedMessageArrive( time_t tServerTime, uint32 nMeaning, uint32 nLength, const char * pData, VFONID fromId, const char * lpszAlias ) ;

	virtual bool OnRequestConversation( uint32 uid, uint32 nContentId, const char * lpszText ) { return true; };

	virtual bool AllowChannelRequest( uint32 uid, int nAppCode, const char * lpszAppString );

	virtual bool OnChannelRequesting( uint32 uid, int nAppCode, const char * lpszAppString, uint32 & nAppParam );

	// function must be overrided to handle event
	// after use pChannel, override function must call ReleaseChannel()
	// but never call OnRequestedChannelSetup again.
	virtual void OnRequestedChannelSetup( BBQ_CHANNEL * pChannel, int nAppCode, const char * lpszAppString );

	virtual void OnRequestedChannelSetupForTestingProxy( BBQ_CHANNEL * pChannel, int nAppCode, const char * lpszAppString );

	virtual void OnTextMessage( uint32 nMsgID, const char * lpszString ) {};

	virtual void OnServiceChanged( const BBQACLFlag * pAclFlags, const BBQUserServiceInfo * pServiceInfo ) {};

	virtual void OnBuddyStatusChanged( const BBQBriefNotifyInfo * pInfo ) {};

	virtual void OnBuddyStatusChanged( const BBQBriefNotifyInfo * pInfo, const char* szStatusString ) { OnBuddyStatusChanged(pInfo); };

	virtual void OnBuddyStatusChanged( const BBQBriefNotifyInfo * pInfo, const char* szStatusString, const char* szAlias ) {   };

	virtual void OnNotifyContactActionBy( uint32 byId, uint32 nAction );

	virtual void OnNotifyKickUser( uint32 uid);

	virtual void OnSelfBuddyStatusChanged( UserStatusListExt& list ) { };
private:

	// ---------- message handlers ----------------------------------------
	bool OnMsgLoginReplaced( const SIM_REQUEST * req );

	bool OnMsgChannelRequest( const SIM_REQUEST * req );
	bool OnMsgChannelRequestFeedback( const SIM_REQUEST * req );

	bool OnMsgChannelConnect( const SIM_REQUEST * req );
	bool OnMsgChannelConnectConfirmed( const SIM_REQUEST * req );

	bool OnMsgUserDefinedMessage( const SIM_REQUEST * req );
	bool OnMsgRequestConversation( const SIM_REQUEST * req );

	bool OnMsgTextMessage( const SIM_REQUEST * req );
	bool OnMsgSendFileP2P( const SIM_REQUEST * req );
	bool OnMsgRequestViewer( const SIM_REQUEST * req );
  bool OnMsgFreeGroupCmd( const SIM_REQUEST * req );
  bool OnMsgMeetingControl( const SIM_REQUEST * req );

	bool OnNotifyMCCEventCMD( const SIM_REQUEST * req );
	bool OnMsgSendFileMessage( const SIM_REQUEST * req );

	bool OnMsgServiceChanged( const SIM_REQUEST * req );

	bool OnMsgBBQProxyNotify( const SIM_REQUEST * req );
	bool OnMsgDownloadFileNotify( const SIM_REQUEST * req );

	PTCPSocket * ConnectTcpProxyChannel( const SIM_REQUEST * req, PSSLChannel * & pSSLChannel );
	PUDPSocket * ConnectUdpProxyChannel( const SIM_REQUEST * req );

	// for proxy engine
	bool OnMsgCreateProxyChannel( const SIM_REQUEST * req );
	bool OnMsgReleaseProxyChannel( const SIM_REQUEST * req );

	bool OnMsgConferenceChangeByServer( const SIM_REQUEST * req );
	bool OnMsgNotifyEnterRoom( const SIM_REQUEST * req );
	bool OnMsgNotifyLeftRoom( const SIM_REQUEST * req );

	// optimize for direct connection
	bool OnMsgIdQuery( const SIM_REQUEST * req );

	bool OnMsgChannelDirectRequest( const SIM_REQUEST * req );
	bool OnMsgChannelDirectResponse( const SIM_REQUEST * req );

	bool OnMsgBuddyStatusNotify( const SIM_REQUEST * req );
	bool OnMsgSelfBuddyStatusNotify( const SIM_REQUEST * req );

	bool OnMsgMCCNotify( const SIM_REQUEST * req );
	bool OnMsgMCCNotifyEx( const SIM_REQUEST * req );

	bool OnMsgNotifyContact( const SIM_REQUEST * req );

	bool OnMsgNotifyOfflineIM(  const SIM_REQUEST * req );
	bool OnMsgNotifyKickUser(  const SIM_REQUEST * req );
  //chenyuan
  //virtual bool  OnRecvCallers(const char*callers,  unsigned int){return true;};
public:
  bool          BBQAES_encrypt( const char * psrc, const unsigned int len, char* pOut );
  static bool          BBQAES_decrypt( const char * psrc, const unsigned int len, char* pOut );
  virtual void OnMsgDownloadFile(const char * szURL, uint32 whichpage, uint32 sendtime, uint32 nFrom){};
  virtual void OnMsgSendFileProcess(const char * oringefilename, uint32 whichpage, uint32 sendtime, uint32 nPercent, uint32 nTo){};
  virtual void OnReceivingFileP2P(const SIMD_CC_SENDFILE* p){};
  void SetLocalInterface(	const PIPSocket::Address&				addr){ m_addrLocalinterface = addr;};
  virtual void OnRequestViewer(uint32 uidWantViewer, uint32 uidHostViewer, int nSocket, BBQPROXY_CONNECT* pData){};
  virtual void  OnNotifyMCCEventCMD( const int roomid,uint32 nEventID ,const  SIMD_SC_MCC_CMD* pCmd,const  SIM_REQUEST* ){};
  virtual void OnFreeGroupCmd( const SIMD_SC_FREEGROUP_NOTIFY * pQ ){};
  virtual void OnMeetingControl( const char* , int ){};
  void SendMeetingCtrl(int cmd,const char*);
  

};

#define CLIENT_VERCODE(major, minor, build) (((major & 0xff) << 24) | ((minor & 0xff) << 16) | (build & 0xffff ))
const char*GetnAppParam(uint32 param);
enum em_ChannelType{//RTP session Channel type 
  em_VIDEO_CALL=0, em_VIDEO_CONTROL_CALL=1,em_AUDIO_CALL=2, em_AUDIO_CONTROL_CALL=3,em_SIPDATA_CALL=4,em_P2P_FILE=5, em_IPP_CALL=6,em_YYMeeting_VIDEO_CALL=7,em_YYMeeting_AUDIO_CALL=8,em_IPP_CALL_Ext=9
};
#define D_VIDEO_CALL 0
#endif // _BBQ_CLIENT_H
