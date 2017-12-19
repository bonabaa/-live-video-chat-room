
#include "bbqbase.h"
#include "bbqsvr.h"
#include "cert_rsa_aes.h"
#include "yatengine.h"

#ifdef _WIN32
#include <ptclib\url.h>
#else
#include <url.h>
#endif

#include <openssl/evp.h>
#include <openssl/rand.h>

#define		MCC_NOTIFY_AES_CLIENT_VERSION		((1 << 24) + (9 << 16) + 51201)

//#define		GENERATE_LONG_KEY

static const char KEY_BBQServer[] = "LPNHome";

static const char KEY_PKICertificate[] = "PKICertificate";
static const char DEFAULT_PKICertificate[] = "server.crt.pem";

static const char KEY_PKIPrivateKey[] = "PKIPrivateKey";
static const char DEFAULT_PKIPrivateKey[] = "server.key.pem";

static const char KEY_PKIPrivateKeyPass[] = "PKIPrivateKeyPass";
static const char DEFAULT_PKIPrivateKeyPass[] = "";

static const char KEY_PKIPrivateKeyPassEnc[] = "PKIPrivateKeyPassEnc";
static const char DEFAULT_PKIPrivateKeyPassEnc[] = "";

static const char KEY_HeartbeatTimeout[] = "HeartbeatTimeout";
static const char KEY_IDMax[] = "IDMax";
static const char KEY_IDReserved[] = "IDReserved";
static const char KEY_DBType[] = "DBType";
static const char KEY_DBURL[] = "DBURL";
static const char KEY_IndexOnSerial[] = "IndexOnSerial";
static const char KEY_DisableSerialKeyCheck[] = "DisableSerialKeyCheck";
static const char KEY_FixedPCTrialServiceDays[] = "FixedPCTrialServiceDays";

static const char KEY_ServerType[] = "ServerType";
static const char KEY_MyIp[] = "MyIp";
static const char KEY_ExternalIp[] = "ExternalIp";
static const char KEY_AnotherServerIp[] = "AnotherServerIp";
static const char KEY_SecondInterlinkIp[] = "SecondInterlinkIp";

static const char KEY_UplinkServerIp[] = "UplinkServerIp";
static const char KEY_DomainId[] = "DomainId";
static const char KEY_DomainAlias[] = "DomainAlias";
static const char KEY_DomainPassword[] = "DomainPassword";

static const char KEY_UseTcpForServerLink[] = "UseTcpForServerLink";

static const char KEY_DisableSubscribe[] = "DisableSubscribe";
static const char KEY_AllowMoreIdPerPC[] = "AllowMoreIdPerPC";
static const char KEY_DisableBindUser[] = "DisableBindUser";

static const char KEY_DBWriteInOutLog[] = "DBWriteInOutLog";
static const char KEY_DBWriteConferenceLog[] = "DBWriteConferenceLog";

static const char KEY_DBCacheTime[] = "DBCacheTime";
static const char KEY_IgnoreDBError[] = "IgnoreDBError";
static const char KEY_WriteHeartbeatFile[] = "WriteHeartbeatFile";
static const char KEY_CheckDNSInHeartbeat[] = "CheckDNSInHeartbeat";
static const char KEY_WriteUserActionLog[] = "WriteUserActionLog";
static const char KEY_QoSWarningLevel[] = "QoSWarningLevel";
static const char KEY_QoSStatResetTime[] = "QoSStatResetTime";

static const char KEY_MeetingForecast[] = "MeetingForecast";
static const char KEY_MeetingForecastTime[] = "MeetingForecastTime";
static const char KEY_MeetingEndForecast[] = "MeetingEndForecast";
static const char KEY_MeetingEndForecastTime[] = "MeetingEndForecastTime";
static const char KEY_MeetingBeginNotify[] = "MeetingBeginNotify";

static const char KEY_UserActionLogFile[] = "UserActionLogFile";
static const char KEY_UserActionLogMaxSize[] = "UserActionLogMaxSize";

static const char DEFAULT_UserActionLogFile[] = "logs/useraction.log";
const int DEFAULT_UserActionLogMaxSize = SIZE_MB_1;

static const char KEY_PrepaidPolicy[] = "PrepaidPolicy";
static const char KEY_DenyCallBetweenGroup[] = "DenyCallBetweenGroup";

static const char KEY_CustomizeDefaultUser[] = "CustomizeDefaultUser";
static const char KEY_CustomizeNormalUser[] = "CustomizeNormalUser";
static const char KEY_CustomizeTrialUser[] = "CustomizeTrialUser";
static const char KEY_CustomizeFreeUser[] = "CustomizeFreeUser";

static const char KEY_FeaturesDefault[] = "FeaturesDefault";
static const char KEY_FeaturesTrial[] = "FeaturesTrial";
static const char KEY_FeaturesFree[] = "FeaturesFree";
static const char KEY_FeaturesPrepaid[] = "FeaturesPrepaid"; 

static const char KEY_EnableCUG[] = "EnableCUG";
static const char KEY_RunInASPMode[] = "RunInASPMode";
static const char KEY_AllowCUGCallPublic[] = "AllowCUGCallPublic";
static const char KEY_CUGHavePrivateBuddy[] = "CUGHavePrivateBuddy";
static const char KEY_HideIdTreeToClient[] = "HideIdTreeToClient";

static const char KEY_AllowGroupIdLoginAsUser[] = "AllowGroupIdLoginAsUser";
static const char KEY_SyncBuddyWithAuthGateway[] = "SyncBuddyWithAuthGateway";
static const char KEY_AuthGatewayBindTypes[] = "AuthGatewayBindTypes";

static const char KEY_WelcomeMsg[] = "WelcomeMsg";
static const char KEY_AcceptClientAppList[] = "AcceptClientAppList";

static const char KEY_Security[] = "Security";
static const char KEY_CallbackTrustIp[] = "CallbackTrustIp";

const unsigned int DEFAULT_FeaturesDefault = (
	BBQACLFLAG_MAXPEERS(15) 
	| BBQACLFLAG_AUDIO
	| BBQACLFLAG_TEXTCHAT
	| BBQACLFLAG_ALLOWVPROXY
	| BBQACLFLAG_ALLOWGK
	| BBQACLFLAG_VIDEO
	| BBQACLFLAG_MAXVIDEO(0)
	| BBQACLFLAG_SMS
	| BBQACLFLAG_FILE
	| BBQACLFLAG_WHITEBOARD
	| BBQACLFLAG_APPSHARING
	| BBQACLFLAG_ALLOWBETA 
	);
const unsigned int DEFAULT_FeaturesTrial = (
	BBQACLFLAG_MAXPEERS(15) 
	| BBQACLFLAG_AUDIO
	| BBQACLFLAG_TEXTCHAT
	| BBQACLFLAG_ALLOWVPROXY
	| BBQACLFLAG_ALLOWGK
	| BBQACLFLAG_VIDEO
	| BBQACLFLAG_MAXVIDEO(0)
	| BBQACLFLAG_SMS
	| BBQACLFLAG_FILE
	| BBQACLFLAG_WHITEBOARD
	| BBQACLFLAG_APPSHARING
	| BBQACLFLAG_ALLOWBETA 
	);
const unsigned int DEFAULT_FeaturesFree = (
	BBQACLFLAG_MAXPEERS(15) 
	| BBQACLFLAG_AUDIO
	| BBQACLFLAG_TEXTCHAT
	| BBQACLFLAG_ALLOWVPROXY
	| BBQACLFLAG_ALLOWGK
	| BBQACLFLAG_VIDEO
	| BBQACLFLAG_MAXVIDEO(0)
	| BBQACLFLAG_SMS
	| BBQACLFLAG_FILE
	| BBQACLFLAG_WHITEBOARD
	| BBQACLFLAG_APPSHARING
	| BBQACLFLAG_ALLOWBETA 
	);
const unsigned int DEFAULT_FeaturesPrepaid = (
	BBQACLFLAG_MAXPEERS(15) 
	| BBQACLFLAG_AUDIO
	| BBQACLFLAG_TEXTCHAT
	| BBQACLFLAG_ALLOWVPROXY
	| BBQACLFLAG_ALLOWGK
	| BBQACLFLAG_VIDEO
	| BBQACLFLAG_MAXVIDEO(0)
	| BBQACLFLAG_SMS
	| BBQACLFLAG_FILE
	| BBQACLFLAG_WHITEBOARD
	| BBQACLFLAG_APPSHARING
	| BBQACLFLAG_ALLOWBETA 
	);

#define			ONE_YEAR					31536000
#define			ONE_HOUR					3600
#define			TEN_MINUTES					600
#define			FIVE_MINUTES				300
#define			ONE_MINUTE					60
#define			TEN_SECONDS					10

static const char KEY_RunAsClusterCenter[] = "RunAsClusterCenter";
const BOOL DEFAULT_RunAsClusterCenter = FALSE;

const int DEFAULT_HeartbeatTimeout = SIM_HEARTBEAT_INTERVAL * 3;
const int DEFAULT_IDMax = 1000000;
static const char DEFAULT_DBType[] = "file";
static const char DEFAULT_DBURL[] = "users.db";
const BOOL DEFAULT_IndexOnSerial = FALSE;

const BOOL DEFAULT_DisableSerialKeyCheck = FALSE;
const int DEFAULT_FixedPCTrialServiceDays = 30;

const BOOL DEFAULT_DisableSubscribe = FALSE;
const BOOL DEFAULT_AllowMoreIdPerPC = FALSE;
const BOOL DEFAULT_DisableBindUser = FALSE;

const BOOL DEFAULT_DBWriteInOutLog = FALSE;
const BOOL DEFAULT_DBWriteConferenceLog = FALSE;

const int DEFAULT_DBCacheTime = ONE_MINUTE;
const BOOL DEFAULT_IgnoreDBError = FALSE;
const BOOL DEFAULT_WriteHeartbeatFile = FALSE;
const BOOL DEFAULT_CheckDNSInHeartbeat = FALSE;
const BOOL DEFAULT_WriteUserActionLog = FALSE;

const BOOL DEFAULT_MeetingForecast = TRUE;
const int DEFAULT_MeetingForecastTime = FIVE_MINUTES;
const BOOL DEFAULT_MeetingEndForecast = TRUE;
const int DEFAULT_MeetingEndForecastTime = FIVE_MINUTES;
const BOOL DEFAULT_MeetingBeginNotify = TRUE;

const BOOL DEFAULT_PrepaidPolicy = TRUE;
const BOOL DEFAULT_DenyCallBetweenGroup = FALSE;

const BOOL DEFAULT_CustomizeNormalUser = FALSE;
const BOOL DEFAULT_CustomizeTrialUser = FALSE;
const BOOL DEFAULT_CustomizeFreeUser = FALSE;

const BOOL DEFAULT_EnableCUG = FALSE;
const BOOL DEFAULT_RunInASPMode = FALSE;
const BOOL DEFAULT_AllowCUGCallPublic = FALSE;
const BOOL DEFAULT_CUGHavePrivateBuddy = FALSE;
const BOOL DEFAULT_HideIdTreeToClient = FALSE;

const BOOL DEFAULT_AllowGroupIdLoginAsUser = FALSE;
const BOOL DEFAULT_SyncBuddyWithAuthGateway = TRUE;

static const char DEFAULT_AuthGatewayBindTypes[] = "PCCW_;GAIA;ARCHIEVA;NCS";

static const char KEY_Terminal[] = "Terminal";

static const char KEY_TcpPortList[] = "TcpPortList";
static const char DEFAULT_TcpPortList[] = "5101";//,443,80";

static const char KEY_UdpPortList[] = "UdpPortList";
static const char DEFAULT_UdpPortList[] = "5101,5102,5103";

static const char KEY_TcpClientMax[] = "TcpClientMax";
const int DEFAULT_TcpClientMax = 4096;

static const char KEY_TcpIdleMax[] = "TcpIdleMax";
const int DEFAULT_TcpIdleMax = FIVE_MINUTES;

static const char KEY_HandlerThreads[] = "HandlerThreads";
const int DEFAULT_HandlerThreads = 16;

static const char KEY_RequestQueueMax[] = "RequestQueueMax";
const int DEFAULT_RequestQueueMax = 8192;

static const char KEY_MsgPostMax[] = "MsgPostMax";
const int DEFAULT_MsgPostMax = 4096;

static const char KEY_TcpListenerBacklog[] = "TcpListenerBacklog";
const int DEFAULT_TcpListenerBacklog = 5;

static const char KEY_UseTcpListenerThread[] = "UseTcpListenerThread";
const int DEFAULT_UseTcpListenerThread = FALSE;

static const char KEY_Client[] = "Client";

static const char KEY_VersionMajor[] = "VersionMajor";
static const char KEY_VersionMinor[] = "VersionMinor";
static const char KEY_VersionBuild[] = "VersionBuild";

static const char KEY_VersionNewMajor[] = "VersionNewMajor";
static const char KEY_VersionNewMinor[] = "VersionNewMinor";
static const char KEY_VersionNewBuild[] = "VersionNewBuild";

static const char KEY_DownloadURL[] = "DownloadURL";

const int DEFAULT_VersionMajor = 0;
const int DEFAULT_VersionMinor = 0;
const int DEFAULT_VersionBuild = 0;

const int DEFAULT_VersionNewMajor = 0;
const int DEFAULT_VersionNewMinor = 0;
const int DEFAULT_VersionNewBuild = 0;

static const char KEY_GKServers[] = "GKServers";
static const char KEY_GKServersCount[] = "Count";
static const char KEY_GKServer[] = "Server";

static const char KEY_ProxyServers[] = "ProxyServers";
static const char KEY_ProxyServersCount[] = "Count";
static const char KEY_ProxyServer[] = "Server";

static const char STR_AnotherAlive[] = "AnotherAlive";

BBQServer* BBQServer::m_pCurrentServer = NULL;

BEGIN_BBQ_MESSAGE_MAP( BBQServer, BBQMsgTerminal )

	BBQ_MESSAGE_MAP( SIM_CS_HELLO,			OnMsgClientSayHello )
	BBQ_MESSAGE_MAP( SIM_CS_TESTFIREWALL,	OnMsgClientTestFirewall )

	BBQ_MESSAGE_MAP( SIM_CS_HEARTBEAT,		OnMsgClientHeartbeat )
	BBQ_MESSAGE_MAP( SIM_CS_LOCATE,			OnMsgClientLocate )
	BBQ_MESSAGE_MAP( SIM_CS_LOGOUT,			OnMsgClientLogout )
	BBQ_MESSAGE_MAP( SIM_CS_CHANGESTATUS,	OnMsgClientChangeStatus )

// ------ compartible only for version before 2004.06.07 -------------------
	BBQ_MESSAGE_MAP( SIM_CS_OLDVERSIONINFO,	OnMsgClientQueryOldVersionInfo )
// ------ compartible end --------------------------------------------------

	BBQ_MESSAGE_MAP( SIM_CS_QUERYSERVICE,	OnMsgClientQueryService )
	BBQ_MESSAGE_MAP( SIM_CS_VERSIONINFO,	OnMsgClientQueryVersionInfo )

	BBQ_MESSAGE_MAP( UCM_CS_Q,				OnMsgClientRequestChannel )
	BBQ_MESSAGE_MAP( UCM_CS_A,				OnMsgClientResponseChannel )

	BBQ_MESSAGE_MAP( SIM_CS_UPDATEUSERDATA,		OnMsgClientUpdateUserData )
	BBQ_MESSAGE_MAP( SIM_CS_VIEWUSERDATA,		OnMsgClientViewUserData )
	BBQ_MESSAGE_MAP( SIM_CS_GETUSERTECHPARAM,	OnMsgClientGetUserTechParam )

	BBQ_MESSAGE_MAP( SIM_CS_UPDATECALLFWDINFO,	OnMsgClientUpdateCallForwardInfo )
	BBQ_MESSAGE_MAP( SIM_CS_GETCALLFWDINFO,		OnMsgClientGetCallForwardInfo )

	BBQ_MESSAGE_MAP( SIM_CS_LISTONLINE,			OnMsgClientListOnlineUser )
	BBQ_MESSAGE_MAP( SIM_CS_SEARCHUSER,			OnMsgClientSearchUser )

	BBQ_MESSAGE_MAP( SIM_CS_ADDCONTACT,			OnMsgClientAddUser )
	BBQ_MESSAGE_MAP( SIM_CS_DELETECONTACT,		OnMsgClientDeleteUser )
	BBQ_MESSAGE_MAP( SIM_CS_GETCONTACTLIST,		OnMsgClientGetUserList )

	BBQ_MESSAGE_MAP( SIM_CS_ADDBLOCK,			OnMsgClientAddUser )
	BBQ_MESSAGE_MAP( SIM_CS_DELETEBLOCK,		OnMsgClientDeleteUser )
	BBQ_MESSAGE_MAP( SIM_CS_GETBLOCKLIST,		OnMsgClientGetUserList )

	BBQ_MESSAGE_MAP( SIM_CS_UPLOADDATABLOCK,	OnMsgClientUploadDataBlock )
	BBQ_MESSAGE_MAP( SIM_CS_DOWNLOADDATABLOCK,	OnMsgClientDownloadDataBlock )

	BBQ_MESSAGE_MAP( SIM_CS_USERMSG,			OnMsgClientUserMsg )
	//BBQ_MESSAGE_MAP( SIM_SX_USERMSG,			OnMsgClientUserMsg )
	//BBQ_MESSAGE_MAP( SIM_XS_USERMSG,			OnMsgClientUserMsg )

	BBQ_MESSAGE_MAP( SIM_CS_QUERYGKLIST,		OnMsgClientQueryGKServerList )
	BBQ_MESSAGE_MAP( SIM_CS_QUERYPSLIST,		OnMsgClientQueryPeoxyServerList )

	BBQ_MESSAGE_MAP( UCM_SP_CREATE,				OnMsgCreateProxyChannel )
	BBQ_MESSAGE_MAP( UCM_SP_RELEASE,			OnMsgReleaseProxyChannel )

	BBQ_MESSAGE_MAP( UCM_PS_NOTIFY,				OnMsgProxyNotify )

	BBQ_MESSAGE_MAP( SIM_CS_SUBSCRIBE,			OnMsgClientSubscribe )

	BBQ_MESSAGE_MAP( SIM_CS_LOGIN_EX,			OnMsgClientLoginEx )
	BBQ_MESSAGE_MAP( SIM_CS_GET_PKI_CERT,		OnMsgClientDownloadServerPKICert )
	BBQ_MESSAGE_MAP( SIM_CS_LOGIN_EX_SECURE,	OnMsgClientLoginExSecure )

	BBQ_MESSAGE_MAP( SIM_CS_LOCATE_EX,			OnMsgClientLocateEx )
	BBQ_MESSAGE_MAP( SIM_SC_LOCATE_EX,			OnMsgServerLocateEx )
	BBQ_MESSAGE_MAP( SIM_CS_CONFERENCE,			OnMsgClientConference )
	BBQ_MESSAGE_MAP( SIM_CS_LOADSTRING,			OnMsgClientLoadString )

	BBQ_MESSAGE_MAP( SIM_CS_GETBINDINFO,		OnMsgClientGetBindInfo )

	BBQ_MESSAGE_MAP( SIM_CS_PROXYSTATUS,		OnMsgProxyStatus )

	BBQ_MESSAGE_MAP( SIM_CS_SERVERSTATUS,		OnMsgServerStatus )
	//BBQ_MESSAGE_MAP( SIM_SC_SERVERSTATUS,		OnMsgServerFeedbackStatus )

	BBQ_MESSAGE_MAP( SIM_CS_SERVERSWITCH,		OnMsgServerSwitch )

	BBQ_MESSAGE_MAP( SIM_CS_GETMCULIST,			OnMsgClientGetMCUList )

	BBQ_MESSAGE_MAP( SIM_CS_GETIDTREES,			OnMsgClientGetIdTrees )

	BBQ_MESSAGE_MAP( SIM_SX_UPDATESWITCHINFO,	OnMsgSXUpdateSwitchInfo )
	BBQ_MESSAGE_MAP( SIM_SC_LOGINREPLACED,		OnMsgXSLoginReplaced )

	BBQ_MESSAGE_MAP( SIM_SX_QSWITCHINFO,		OnMsgSXQuerySwitchInfo )
	BBQ_MESSAGE_MAP( SIM_XS_ASWITCHINFO,		OnMsgXSAnswerSwitchInfo )

	BBQ_MESSAGE_MAP( SIM_SX_QLOCATE,			OnMsgSXQueryLocateUser )
	BBQ_MESSAGE_MAP( SIM_XS_ALOCATE,			OnMsgXSAnswerLocateUser )

	BBQ_MESSAGE_MAP( SIM_SS_SERVER_UPDATE,		OnMsgSSServerUpdate )
	BBQ_MESSAGE_MAP( SIM_SS_DOMAIN_UPDATE,		OnMsgSSDomainUpdate )

	BBQ_MESSAGE_MAP( SIM_CS_MCC_NOTIFY,			OnMsgClientNotifyMeetingEvent )
	BBQ_MESSAGE_MAP( SIM_CS_MCC_INVITE,			OnMsgClientMeetingInvite )

	BBQ_MESSAGE_MAP( SIM_CS_NOTIFY_READY,		OnMsgClientNotifyReady )

	BBQ_MESSAGE_MAP( SIM_SX_BUDDYSTATUSNOTIFY,	OnMsgSXBuddyStatusNotify )
	BBQ_MESSAGE_MAP( SIM_XS_BUDDYSTATUSNOTIFY,	OnMsgXSBuddyStatusNotify )

	BBQ_MESSAGE_MAP( SIM_CS_AUTH_CONTACT,		OnMsgClientAuthContact )

	BBQ_MESSAGE_MAP( SIM_CS_IM,					OnMsgClientSendOfflineIM )
	BBQ_MESSAGE_MAP( SIM_CS_GET_IM,				OnMsgClientGetOfflineIM )
	BBQ_MESSAGE_MAP( SIM_CS_DEL_IM,				OnMsgClientDeleteOfflineIM )

	BBQ_MESSAGE_MAP( SIM_CS_ENTER_ROOM,				OnMsgClientEnterRoom )
	BBQ_MESSAGE_MAP( SIM_CS_LEFT_ROOM,				OnMsgClientLeftRoom )
	BBQ_MESSAGE_MAP( SIM_CS_SAVE_NATCLIENT,				OnMsgNotifyNatinfo )

END_BBQ_MESSAGE_MAP()

BBQServer* BBQServer::GetCurrentServer( void )
{
	return m_pCurrentServer;
}

BBQServer::UplinkThread::UplinkThread( BBQServer & owner )
: PThread(5000, NoAutoDeleteThread), m_owner(owner)
{
	m_owner.m_mutexLink.Wait();
	m_owner.m_listUplinkThreads.push_back( this );
	m_owner.m_mutexLink.Signal();

	Resume();
}

BBQServer::UplinkThread::~UplinkThread()
{
	if( ! IsTerminated() ) {
		if( GetThreadId() != GetCurrentThreadId() ) {
			if( IsSuspended() ) Resume();
			WaitForTermination();
		}
	}

	m_owner.m_mutexLink.Wait();
	m_owner.m_listUplinkThreads.remove( this );
	m_owner.m_mutexLink.Signal();
}

void BBQServer::UplinkThread::Main( void )
{
	m_owner.m_dwUplinkTickCalled = 0; //GetHostUpTimeInMs();

	while( ! m_owner.IsClosing() ) {
		MS_TIME msNow = GetHostUpTimeInMs();
		if( msNow >= m_owner.m_dwUplinkTickCalled + UPLINK_TICK_TIME ) {		// check uplink every 30 sec
			m_owner.m_dwUplinkTickCalled = msNow;
			m_owner.UplinkTick();
		}

		sleep_ms(100);
	}
}

// ======================== InterlinkThread =====================

BBQServer::InterlinkThread::InterlinkThread( BBQServer & owner )
: PThread(5000, NoAutoDeleteThread), m_owner(owner)
{
	m_owner.m_mutexLink.Wait();
	m_owner.m_listInterlinkThreads.push_back( this );
	m_owner.m_mutexLink.Signal();

	Resume();
}

BBQServer::InterlinkThread::~InterlinkThread()
{
	if( ! IsTerminated() ) {
		if( GetThreadId() != GetCurrentThreadId() ) {
			if( IsSuspended() ) Resume();
			WaitForTermination();
		}
	}

	m_owner.m_mutexLink.Wait();
	m_owner.m_listInterlinkThreads.remove( this );
	m_owner.m_mutexLink.Signal();
}

void BBQServer::InterlinkThread::Main( void )
{
	m_owner.m_dwInterlinkTickCalled = 0; //GetHostUpTimeInMs();

	while( ! m_owner.IsClosing() ) {
		MS_TIME msNow = GetHostUpTimeInMs();
		if( msNow >= m_owner.m_dwInterlinkTickCalled + INTERLINK_TICK_TIME ) {	// check interlink every 30 sec
			m_owner.m_dwInterlinkTickCalled = msNow;
			m_owner.InterlinkTick();
		}

		sleep_ms(100);
	}
}

// ======================== BBQServer ==========================

BBQServer::BBQServer()
: BBQMsgTerminal( 8192, 16 )
{
  for(int i=0;i< 20;i++)
  {
    LPNRRoomRecord record;
    record.m_nID = i;
    record.m_strName.sprintf("WoW %u", 10001+ i);
    record.m_vecMembers.clear();
    m_mapRooms[i] = record;
  }
	m_pCurrentServer = this;

#ifdef _DEBUG
	// a flag for test only, evoke proxy when each channel is requested, even it is not behind strict firewall
	m_bProxyAllRequestChannel = false;
#endif

	memset( & m_config, 0, sizeof(m_config) );
	m_config.nMeetingForecastTime = 300;

	memset( & m_status, 0, sizeof(m_status) );

	memset( & m_chanUplink[0], 0, sizeof(m_chanUplink[0]) );
	memset( & m_chanUplink[1], 0, sizeof(m_chanUplink[1]) );
	memset( & m_chanInterlink[0], 0, sizeof(m_chanInterlink[0]) );
	memset( & m_chanInterlink[1], 0, sizeof(m_chanInterlink[1]) );

	//m_config.heartbeat_timeout = 0;
	//m_config.db_type = m_config.db_url = NULL;
	//m_config.disable_serial_key_check = false;
	//m_config.subscribe_trial_time = 0;

	m_bInited = false;
	m_bSystemConfigLoadedFromDB = false;
	m_bDataChanged = false;

	m_pDB = NULL;
	m_nAutoSaveCounter = 0;

	// for dongle check and feature disable
	m_nCheckDongleCounter = 0;
	m_bDisableFeature = false;

	memset( & m_VersionInfo, 0, sizeof(m_VersionInfo) );

	m_config.nServerType = BBQServer::STANDALONE;
	m_status.nServerStatus = BBQServer::ACTIVE;

	m_nCountCounter = 0;

	m_status.tUpTime = 0;

	srand( (unsigned int)time( NULL ) );

	m_pzPKICertPEM = m_pzPKIPrivKeyPEM = NULL;
	m_pPKICertX509 = m_pPKIPrivKey = NULL;
}

void BBQServer::Shutdown( void )
{
	PLOG( m_pLogger, Info, "Shutting down server..." );

	if( m_status.nServerStatus != BBQServer::DOWN ) SwitchStatus( BBQServer::DOWN );

	// call this to stop all the related working threads, and socket layer
	BBQMsgTerminal::OnClose();

	// wait interlink thread
	while( ! m_listInterlinkThreads.empty() ) {
		InterlinkThread * th = m_listInterlinkThreads.front();
		m_listInterlinkThreads.pop_front();

		{
			// the thread to be waited will require the same mutex, so release here for a while
			//UnlockIt yield( m_mutexRequest );

			delete th;
		}
	}

	// wait the uplink thread
	while( ! m_listUplinkThreads.empty() ) {
		UplinkThread * th = m_listUplinkThreads.front();
		m_listUplinkThreads.pop_front();

		{
			// the thread to be waited will require the same mutex, so release here for a while
			//UnlockIt yield( m_mutexRequest );

			delete th;
		}
	}

	// clear the system config 
	m_dictSystemConfig.clear();

	if( m_config.db_type ) { 
		free( m_config.db_type ); 
		m_config.db_type = NULL; 
	}

	if( m_config.db_url ) { 
		free( m_config.db_url ); 
		m_config.db_url = NULL; 
	}

	{
		WriteLock lock( m_mutexRecord );

		// remove all meeting records
		{
			while( ! m_mlMeetingQueue.empty() ) {
				BBQ_MeetingInfo_Ext * p = m_mlMeetingQueue.front();
				m_mlMeetingQueue.pop_front();
				delete p;
			}

			//while( ! m_mlMeetingNotifying.empty() ) {
			//	BBQ_MeetingInfo_Ext * p = m_mlMeetingNotifying.front();
			//	m_mlMeetingNotifying.pop_front();
			//	delete p;
			//}

			while( ! m_mlMeetingList.empty() ) {
				BBQ_MeetingInfo_Ext * p = m_mlMeetingList.front();
				m_mlMeetingList.pop_front();
				delete p;
			}
		}

		// remove all domain records
		{
			DomainMap::iterator iter, eiter;
			for( iter = m_mapIdDomains.begin(), eiter = m_mapIdDomains.end(); iter != eiter; iter ++ ) {
				BBQDomainRecord * p = iter->second;
				iter->second = NULL;
				if(p) delete p;
			}
		}

		// remove all server records
		{
			ServerMap::iterator iter, eiter;
			for( iter = m_mapLocalServers.begin(), eiter = m_mapLocalServers.end(); iter != eiter; iter ++ ) {
				BBQServerRecord * p = iter->second;
				iter->second = NULL;
				if(p) delete p;
			}
		}

		// remove all MCU records
		{
			MCUMap::iterator iter, eiter;

			// remove all proxy record when destruct
			for( iter = m_mapMCU.begin(), eiter = m_mapMCU.end(); iter != eiter; iter ++ ) {
				MCURecord * pMCU = iter->second;
				iter->second = NULL;

				delete pMCU;
			}
			m_mapMCU.clear();
			m_status.nMCUCount = 0;
		}

		{
			ProxyMap::iterator iter, eiter;

			// remove all proxy record when destruct
			for( iter = m_mapProxy.begin(), eiter = m_mapProxy.end(); iter != eiter; iter ++ ) {
				BBQProxyRecord * pRec = iter->second;
				iter->second = NULL;

				delete pRec;
			}
			m_mapProxy.clear();
			m_status.nProxyCount = 0;

			for( iter = m_mapPureProxy.begin(), eiter = m_mapPureProxy.end(); iter != eiter; iter ++ ) {
				BBQProxyRecord * pRec = iter->second;
				iter->second = NULL;

				delete pRec;
			}
			m_mapPureProxy.clear();
			m_status.nPureProxyCount = 0;
		}

		{	// remove all record when destruct
			for( RecordMap::iterator iter = m_mapRecord.begin(), eiter = m_mapRecord.end(); iter != eiter; iter ++ ) {
				SFIDRecord * pRec = iter->second;
				iter->second = NULL;

				if( pRec ) DestroyRecord( pRec );
			}
			m_mapRecord.clear();

			m_dictHardwareId.clear();
			m_dictHandphone.clear();
			m_dictEmail.clear();
		}
	}

	cert_rsa_aes_cleanup();

	m_bInited = false;
}

void BBQServer::DestroyRecord( SFIDRecord * pRec )
{
	if( pRec->pServiceInfo ) delete pRec->pServiceInfo;
	if( pRec->pUserInfo ) delete pRec->pUserInfo;

	if( pRec->pFriendList ) delete pRec->pFriendList;
	if( pRec->pBlockList ) delete pRec->pBlockList;

	if( pRec->pBuddyReverseLink ) delete pRec->pBuddyReverseLink;
	if( pRec->pBuddyAddingList ) delete pRec->pBuddyAddingList;
	if( pRec->pWaitAuthList ) delete pRec->pWaitAuthList;

	if( pRec->pParentIdList ) delete pRec->pParentIdList;
	if( pRec->pChildIdList ) delete pRec->pChildIdList;

	if( pRec->pSessionList ) {
		DestroyVfonSessionList( pRec->pSessionList );
		pRec->pSessionList = NULL;
	}

	if( pRec->pMeetingList ) delete pRec->pMeetingList;

	if( pRec->pBuddyStatus ) delete pRec->pBuddyStatus;

	if( pRec->pCallForwardInfo ) delete pRec->pCallForwardInfo;
	if( pRec->pVoipItems ) delete pRec->pVoipItems;

	if( pRec->pXMLEXT ) free( pRec->pXMLEXT );
	if( pRec->pTagString ) free( pRec->pTagString );

	if( pRec->pAESKeyBytes ) free( pRec->pAESKeyBytes );
	
	if( pRec->pImList ) {
		while( pRec->pImList->size() > 0 ) {
			BBQOfflineIMMessage * im = pRec->pImList->front();
			pRec->pImList->pop_front();
			if( im ) {
				BBQOfflineIMMessageDELETE( im );
			}
		}
		
		delete pRec->pImList;
	}

	delete pRec;
}

void BBQServer::OnEventDBFail( void )
{
	int nErr = m_pDB ? m_pDB->GetLastErrorCode() : 0;
	PString strErr = m_pDB ? m_pDB->GetLastErrorMsg() : "null database object.";

	PString strMsg( PString::Printf, "Operation on DB fail, error: %d, %s.", nErr, (const char *)strErr );
	PLOG( m_pLogger, Fatal, strMsg );

	m_status.nDBStatus = BBQDatabase::ServerFail;

	SwitchStatus( BBQServer::DOWN );
}

//#define CATCH_DB_FAIL( statement ) \
//	if( m_pDB && (m_pDB->GetLastErrorCode() != BBQDatabase::ServerFail) ) { \
//		statement; \
//		if( m_pDB->GetLastErrorCode() == BBQDatabase::ServerFail ) { \
//			PString strMsg( PString::Printf, "DB operation fail at line %d of %s", __LINE__, __FILE__ ); \
//			PLOG( m_pLogger, Error, strMsg ); \
//			OnEventDBFail(); \
//		} \
//	} else { \
//		PTRACE( 1, "DB in fail status, skip DB operation." ); \
//		if( m_status.nServerStatus != BBQServer::DOWN ) { \
//			OnEventDBFail(); \
//		} \
//	}

#define CATCH_DB_FAIL( statement ) \
	if( m_pDB ) { \
		if( m_config.ignore_db_err || (m_pDB->GetLastErrorCode() != BBQDatabase::ServerFail) ) { \
			statement; \
			if( m_pDB->GetLastErrorCode() == BBQDatabase::ServerFail ) { \
				PString strMsg( PString::Printf, "DB operation fail at line %d: %s", __LINE__, m_pDB->GetLastErrorMsg() ); \
				PLOG( m_pLogger, Error, strMsg ); \
				if( ! m_config.ignore_db_err ) OnEventDBFail(); \
			} \
		} else { \
			PTRACE( 1, "DB in fail status, skip DB operation." ); \
			if( m_status.nServerStatus != BBQServer::DOWN ) OnEventDBFail(); \
		} \
	} else { \
		PTRACE( 1, "DB object is NULL, skip DB operation." ); \
		if( m_status.nServerStatus != BBQServer::DOWN ) OnEventDBFail(); \
	}

void BBQServer::DestroyVfonSessionList( VfonSessionList * pList )
{
	time_t now = time(NULL);

	while( pList->size() > 0 ) {
		VfonSession * pSession = pList->front();
		pList->pop_front();

		pSession->endTime = now;
		pSession->flags.dropOffline = 1;

		CATCH_DB_FAIL( 
			m_pDB->LogVfonSession( pSession ) 
		);

		delete pSession;
		m_status.nCurrentConfs --;
	}

	delete pList;
}

BBQServer::~BBQServer()
{
	BBQProxy * pProxy = BBQProxy::GetCurrentProxy();
	if( pProxy ) {
		pProxy->StopProxy();
	}

	m_pCurrentServer = NULL;

	CleanPKICertKey();

	if( m_bInited ) Shutdown();
}

void BBQServer::CheckDongleStatus( void )
{
	if( m_nCheckDongleCounter == 0 ) {
		bool bValidLicense = ( IsValidCopy() && (time(NULL) < GetExpireTime()) );

		if( m_config.write_hearbeat_file ) {
			PString path( "logs" );
			path += PDirectory::IsSeparator('\\') ? "\\" : "/";
			path += "BadLicense";

			if( bValidLicense ) {
				unlink( path );
			} else {
				PFileLog::TouchFile( path );
			}
		}

		m_bDisableFeature = ! bValidLicense;
	}

	m_nCheckDongleCounter ++;

	if( m_bDisableFeature ) {
		if( m_nCheckDongleCounter >= TEN_SECONDS ) m_nCheckDongleCounter = 0;
	} else {
		if( m_nCheckDongleCounter >= ONE_HOUR ) m_nCheckDongleCounter = 0;
	}
}

void BBQServer::CheckQosStatus( void )
{
	// touch or delete the flag file "BadQoS"
	PString path( "logs" );
	path += PDirectory::IsSeparator('\\') ? "\\" : "/";
	path += "BadQoS";
	uint32 nTotalCalls = m_status.nOkayCalls + m_status.nFailedCalls + m_status.nErrorCalls;
	if( m_status.nOkayCalls < nTotalCalls * m_config.QoS_warning_level ) {
		PFileLog::TouchFile( path );
	} else {
		unlink( path );
	}

	// inc the reset conter
	m_nQoSCounter ++;
	if( (m_config.QoS_stat_reset_time > 0) && (m_nQoSCounter >= m_config.QoS_stat_reset_time) ) {
		m_nQoSCounter = 0;
		m_status.nOkayCalls = m_status.nFailedCalls = m_status.nErrorCalls = 0;
	}
}

void BBQServer::CheckProxyStatus( void )
{
	ReadLock saferead( m_mutexRecord );

	time_t now = time(NULL);

	{
		for( ProxyMap::iterator it = m_mapProxy.begin(), et = m_mapProxy.end(); it != et; /*it ++*/ ) {
			BBQProxyRecord * p = it->second;
			if( p && (p->time + m_config.heartbeat_timeout < now) ) {
				// record timeout
				STL_ERASE( m_mapProxy, ProxyMap::iterator, it );
			} else {
				it ++;
			}
		}
	}
	{
		for( ProxyMap::iterator it = m_mapPureProxy.begin(), et = m_mapPureProxy.end(); it != et; /*it ++*/ ) {
			BBQProxyRecord * p = it->second;
			if( p && (p->time + m_config.heartbeat_timeout < now) ) {
				// record timeout
				STL_ERASE( m_mapPureProxy, ProxyMap::iterator, it );
			} else {
				it ++;
			}
		}
	}
}

void BBQServer::OnMsgConnectionClose( BBQMsgConnection * pConn ) 
{
	LockIt sf( m_mutexClosedTcp );

	if( pConn ) {
		PTCPSocket * pSock = pConn->GetSocket();
		if( pSock ) {
			int fd = pSock->GetHandle();
			if( fd > 0 ) m_listClosedTcp.push_back( fd );
		}
	}
}

void BBQServer::CheckUserStatus( void )
{
	time_t now = time(NULL);

	int nCounter = 0;
	for( RecordMap::iterator Iter = m_mapRecord.begin(), eIter = m_mapRecord.end(); Iter != eIter; Iter ++ ) {
		SFIDRecord * pRec = Iter->second;

		bool bOnline = ( (pRec->sessionInfo.cookie != 0) && (pRec->time + m_config.heartbeat_timeout > now) );

		if( pRec->sockChannel.socket > 0 ) { // let's check the tcp socket is valid
			LockIt sf( m_mutexClosedTcp );
			for( std::list<int>::iterator it = m_listClosedTcp.begin(); it != m_listClosedTcp.end(); /**/ ) {
				if( pRec->sockChannel.socket == * it ) {
					STL_ERASE( m_listClosedTcp, std::list<int>::iterator, it );

					bOnline = false;
					break;
				} else {
					it ++;
				}
			}			

			//fd_set fdsetRead;
			//FD_ZERO( & fdsetRead );
			//FD_SET( pRec->sockChannel.socket, & fdsetRead );
			//struct timeval tval = { 0, 0 };
			//int s = select( pRec->sockChannel.socket +1, & fdsetRead, NULL, NULL, &tval);

			//if( s < 0 ) bOnline = false;
		}

		if( bOnline ) {
			nCounter ++;

		} else if( pRec->sessionInfo.cookie ) { // if the user drop offline, but cookie not clear

			WriteLock safewrite( m_mutexRecord );
			UserOffline( pRec, true );

		} else {
			// user offline already, garbage record in memory, ignore
		}
	}

	// now clear all the closed fd
	{
		LockIt sf( m_mutexClosedTcp );
		while( m_listClosedTcp.size() > 0 ) {
			m_listClosedTcp.pop_front();
		}
	}

	m_status.nOnlineUsers = nCounter;
	m_status.nMaxUserCount = max( m_status.nMaxUserCount, m_status.nOnlineUsers );
}

bool BBQServer::InviteUserForMeeting( uint32 uid, const char * lpszMeetingId )
{
	WriteLock lock( m_mutexRecord );

	// find the meeting
	BBQ_MeetingInfo_Ext * pExt = NULL;
	if( m_mlMeetingList.size() > 0 ) {
		for( meetingext_list::iterator it = m_mlMeetingList.begin(), et = m_mlMeetingList.end(); it != et; it ++ ) {
			BBQ_MeetingInfo_Ext * p = * it;
			if( 0 == strcmp( lpszMeetingId, p->info.strMeetingId ) ) {

				pExt = p;
				break;
			}
		}
	}
	if( ! pExt ) return false; // meeting not found in list

	BBQ_MeetingInfo * pM = & pExt->info;

	// find the user
	SFIDRecord * pRec = FindRecordByID( uid );
	if( ! pRec ) return false;

	// keep the meeting info during meeting time
	if( ! pRec->pMeetingList ) pRec->pMeetingList = new MeetingList;
	MeetingList * pList = pRec->pMeetingList;
	for( MeetingList::iterator it = pList->begin(), et = pList->end(); it != et; /**/ ) {
		const BBQ_MeetingInfo_Ext * p = * it;
		if( 0 == strcmp( p->info.strMeetingId, pM->strMeetingId ) ) {
			STL_ERASE( * pList, MeetingList::iterator, it );
		} else {
			it ++;
		}
	}
	if( pExt->info.status == MCC_BEGIN ) pList->push_back( pExt );

	time_t now = time(NULL);
	// now send notification
	if( (pRec->sessionInfo.cookie != 0) && (pRec->time + m_config.heartbeat_timeout > now) ) {
		BBQ_MeetingInfo mInfo = * pM;
		mInfo.timestamp = now;
		mInfo.status = MCC_BEGIN;

		SIM_REQUEST req;
		SIM_REQINITCHAN( req, pRec->sockChannel, SIM_SC_MCC_NOTIFY, sizeof(SIMD_SC_MCC_NOTIFY) );
		SIMD_SC_MCC_NOTIFY * pQ = (SIMD_SC_MCC_NOTIFY *) & req.msg.simData;
		memset( pQ, 0, sizeof(*pQ) );
		pQ->tNowSenderTime = now;
		pQ->eventId = MCC_BEGIN;
		pQ->sessionInfo = pRec->sessionInfo;

		// TODO: encrypt msg if needed
		if( pRec->pAESKeyBytes && (pRec->techInfo.version >= MCC_NOTIFY_AES_CLIENT_VERSION ) ) {
			bool bDone = AES_encrypt( (unsigned char *) & mInfo, sizeof(mInfo),
				(unsigned char *) pRec->pAESKeyBytes, (unsigned char *) PI_SQRT2_STRING, pRec->nAESKeyBytes * 8,
				(unsigned char *) & pQ->meetingInfo );
			if( bDone ) {
				pQ->flags.is_encrypted = 1;
			} else {
				pQ->meetingInfo = mInfo;
			}
		} else {
			pQ->meetingInfo = mInfo;
		}

		PostMessage( & req, false, 2000, 10 );
	}

	return true;
}

bool BBQServer::NotifyClientMeetingEvent( BBQ_MeetingInfo_Ext * pMExt, uint32 nEventId, bool bAlsoNotifyMember )
{
	BBQ_MeetingInfo * pM = & pMExt->info;

	time_t now = time(NULL);

	uint32 nId = 0;
	SIM_CHANNEL chNotify;
	SIM_SESSIONINFO sessionInfo;
	memset( & chNotify, 0, sizeof(chNotify) );
	memset( & sessionInfo, 0, sizeof(sessionInfo) );

	if( pM->flags.bMCU && ( (nEventId == MCC_BEGIN) || (nEventId == MCC_END) ) ) {

		uint32 id = pM->nMcuId;		// pick the default MCU

		SFIDRecord * pRec = FindRecordByID( id );
		if( pRec && ( (pRec->sessionInfo.cookie != 0) && (pRec->time + m_config.heartbeat_timeout > now) ) ) {
			nId = id;
			chNotify = pRec->sockChannel;
			sessionInfo = pRec->sessionInfo;
	
		}
		
		if( nId == 0 ) { // specified MCU is not online right now

			// following action for non-cluster mode only, 
			//if( m_config.nServerType != CLUSTER ) 
			// for clustered mode, need to config all MCU to one vfon server
			{
				// we pick another online MCU for the conference now
				for( MCUMap::iterator i = m_mapMCU.begin(), e = m_mapMCU.end(); i != e; i ++ ) {
					uint32 id = i->first;
					pRec = FindRecordByID( id );
					if( pRec && ( (pRec->sessionInfo.cookie != 0) && (pRec->time + m_config.heartbeat_timeout > now) ) ) {
						nId = id;
						chNotify = pRec->sockChannel;
						sessionInfo = pRec->sessionInfo;
						break;
					}
				}

				// if we pick another mcu, we must make change to the DB record
				if( nId && ( nId != pM->nMcuId ) ) {
					pM->nMcuId = nId;
				}

			}
		}

		if( nId == 0 ) {
			PString strMsg( PString::Printf, "Specified MCU or member (Id:%u) not online on event %s for meeting (%s).", 
				pM->nMcuId, 
				(nEventId == MCC_BEGIN) ? "BEGIN" : "END",
				pM->strMeetingId );
			m_UserActionLog.Output( PLog::Info, strMsg );
		}
	}

	BBQ_MeetingInfo mInfo = * pM;
	mInfo.timestamp = now;
	mInfo.status = (uint8) nEventId;

	SIM_REQUEST req;
	SIM_REQINIT( req, 0, 0, 0, 0, 0, SIM_SC_MCC_NOTIFY, sizeof(SIMD_SC_MCC_NOTIFY) );
	SIMD_SC_MCC_NOTIFY * pQ = (SIMD_SC_MCC_NOTIFY *) & req.msg.simData;
	memset( pQ, 0, sizeof(*pQ) );
	pQ->tNowSenderTime = now;
	pQ->eventId = nEventId;
	pQ->meetingInfo = mInfo;
	switch( nEventId ) {
	case MCC_INIT:
		pQ->nEventTime = m_config.nMeetingForecastTime;
		break;
	case MCC_INITEND:
		pQ->nEventTime = m_config.nMeetingEndForecastTime;
		break;
	}

	if( nId ) { // send command to MCU
		pQ->sessionInfo = sessionInfo;
		req.channel = chNotify;
		PostMessage( & req, true );
		//PostMessage( & req, false );
		sleep_ms(0);

		// new format with user & role info
		uint16 nMsgDataSize = sizeof(SIMD_SC_MCC_NOTIFY_EX) + ( pMExt->size - sizeof(* pMExt) );
		uint16 nReqSize = SIM_DATASIZE_TO_REQSIZE( nMsgDataSize );
		SIM_REQUEST * pReq = NULL;
		SIM_REQUEST_NEW( pReq, nReqSize );
		if( pReq ) {
 			SIM_REQINITCHAN( * pReq, chNotify, SIM_SC_MCC_NOTIFY_EX, nMsgDataSize );
			SIMD_SC_MCC_NOTIFY_EX * pN = (SIMD_SC_MCC_NOTIFY_EX *) pReq->msg.simData;
			pN->sessionInfo = sessionInfo;
			pN->eventId = nEventId;
			pN->tNowSenderTime = now;
			pN->nEventTime = pQ->nEventTime;
			memcpy( & pN->meetingInfoExt, pMExt, pMExt->size );
			pN->meetingInfoExt.info.timestamp = now;
			PostMessage( pReq, true );
			//PostMessage( & pReq, false );

			SIM_REQUEST_DELETE( pReq );
		}

		PString strMsg( PString::Printf, "Meeting event command %s for meeting (%s) sent to MCU (%u).", 
				(nEventId == MCC_BEGIN) ? "BEGIN" : "END",
				pM->strMeetingId,
				nId );
		m_UserActionLog.Output( PLog::Info, strMsg );
	}

	UpdateUserMeetingRef( pMExt );
	
	if ( bAlsoNotifyMember ) {

		for( uint32 i=0; i<pMExt->nNumberOfUsers; i++ ) {

			uint32 id = pMExt->pUsers[i].user.userid;

		//for( int i=0; i<pM->nIdCount; i++ ) { 
		//	uint32 id = 0;
		//	if( pM->flags.bLocalIdList || (pM->nIdCount >= 32) ) {
		//		id = pM->localIdList[i];
		//	} else {
		//		id = pM->idList[i].userid;
		//	}

			SFIDRecord * pRec = FindRecordByID( id );
			if( pRec ) {
				//// keep the meeting info during meeting time
				//if( ! pRec->pMeetingList ) pRec->pMeetingList = new MeetingList;

				//MeetingList * pList = pRec->pMeetingList;
				//for( MeetingList::iterator it = pList->begin(), et = pList->end(); it != et; /**/ ) {
				//	const BBQ_MeetingInfo_Ext * p = * it;
				//	if( 0 == strcmp( p->info.strMeetingId, pM->strMeetingId ) ) {
				//		STL_ERASE( * pList, MeetingList::iterator, it );
				//	} else {
				//		it ++;
				//	}
				//}
				//if( pMExt->info.status == MCC_BEGIN ) pList->push_back( pMExt );

				// send information to client
				if ( (pRec->sessionInfo.cookie != 0) && (pRec->time + m_config.heartbeat_timeout > now) ) {
					req.channel = pRec->sockChannel;
					pQ->sessionInfo = pRec->sessionInfo;

					// TODO: encrypt msg if needed
					if( pRec->pAESKeyBytes && (pRec->techInfo.version >= MCC_NOTIFY_AES_CLIENT_VERSION ) ) {
						bool bDone = AES_encrypt( (unsigned char *) & mInfo, sizeof(mInfo),
							(unsigned char *) pRec->pAESKeyBytes, (unsigned char *) PI_SQRT2_STRING, pRec->nAESKeyBytes * 8,
							(unsigned char *) & pQ->meetingInfo );
						if( bDone ) {
							pQ->flags.is_encrypted = 1;
						} else {
							pQ->meetingInfo = mInfo;
						}
					} else {
						pQ->meetingInfo = mInfo;
					}

					PostMessage( & req, true, 2000, 10 );
				}
			}
		}
	} else {
		// empty id list, invalid data
	}

	return true;
}

bool BBQServer::DestroyMeetingInfo( BBQ_MeetingInfo_Ext * pMExt )
{
	// remove from compare list
	KeywordIndex::iterator iter = m_dictMeetingId.find( pMExt->info.strMeetingId );
	if( iter != m_dictMeetingId.end() ) m_dictMeetingId.erase( iter );

	pMExt->info.status = MCC_INVALID;

	UpdateUserMeetingRef( pMExt );

	BBQ_MeetingInfo_Ext_DELETE( pMExt );

	return true;
}

bool BBQServer::ClearMeetingInfo( const char * lpszMeetingId )
{
	WriteLock lock( m_mutexRecord );

	// not found
	if( m_dictMeetingId.find( lpszMeetingId ) == m_dictMeetingId.end() ) return false;

	bool bDone = false;

	meetingext_list * allmeetings[3] = { NULL, NULL, NULL };
	allmeetings[0] = & m_mlMeetingQueue;
	//allmeetings[1] = & m_mlMeetingNotifying;
	allmeetings[2] = & m_mlMeetingList;

	for( int i=0; i<sizeof(allmeetings)/sizeof(meetingext_list *); i++ ) {
		meetingext_list * pList = allmeetings[i];
		if( ! pList ) continue;

		for( meetingext_list::iterator it = pList->begin(), et = pList->end(); it != et; /*it ++*/ ) {
			BBQ_MeetingInfo_Ext * p = * it;
			if( 0 == strcmp( lpszMeetingId, p->info.strMeetingId ) ) {
				STL_ERASE( m_mlMeetingQueue, meetingext_list::iterator, it );

				p->info.status = MCC_END;

				if( m_config.write_useraction_log ) {
					PString strMsg( PString::Printf, 
						"Meeting (%s) cancelled, now send END command to MCU(%u).", 
						p->info.strMeetingId, 
						p->info.nMcuId );
					m_UserActionLog.Output( PLog::Info, strMsg );
				}

				// the meeting should be ended, 
				NotifyClientMeetingEvent( p, MCC_END, m_config.meeting_begin_notify );

				DestroyMeetingInfo( p );
				bDone = true;
			} else {
				it ++;
			}
		}
	}

	return bDone;
}

bool BBQServer::UpdateUserMeetingRef( const BBQ_MeetingInfo_Ext * pExt )
{
	WriteLock lock( m_mutexRecord );

	if( ! pExt ) return false;

	for( int i=0; i<pExt->nNumberOfUsers; i++ ) {
		uint32 id = pExt->pUsers[i].user.userid;

		SFIDRecord * pRec = FindRecordByID( id );
		if( pRec ) {
			if( ! pRec->pMeetingList ) pRec->pMeetingList = new MeetingList;
			MeetingList * pList = pRec->pMeetingList;
			for( MeetingList::iterator it = pList->begin(), et = pList->end(); it != et; /**/) {
				const BBQ_MeetingInfo_Ext * p = * it;
				if( 0 == strcmp( p->info.strMeetingId, pExt->info.strMeetingId ) ) {
					STL_ERASE( * pList, MeetingList::iterator, it );
				} else {
					it ++;
				}
			}
			if( pExt->info.status == MCC_BEGIN ) pList->push_back( pExt );
		}
	}

	return true;
}

bool BBQServer::LoadMeetingInfo( time_t t1, time_t t2, uint32 nRoomId, const char * lpszMeetingId  )
{
	// if server is in standby status, then do not load meeting info
	if( m_status.nServerStatus == BBQServer::STANDBY ) return false;

	WriteLock lock( m_mutexRecord );

	// load meeting booking info ( within 10 minutes )
	meetingext_list mlNew;
	if( m_pDB && (m_pDB->ReadConferenceBookingExt( mlNew, t1, t2, nRoomId, lpszMeetingId )) ) {
		bool bDataValid = true;

		// check and make sure new data must be sorted in order of begin time
		{
			meetingext_list::iterator it = mlNew.begin(), et = mlNew.end();
			time_t t = 0;
			for( ; it != et; it ++ ) {
				BBQ_MeetingInfo_Ext * p = * it;
				if( t <= p->info.tBeginTime ) {
					t = p->info.tBeginTime;

					if( p->info.nDuration >= ONE_YEAR ) p->info.nDuration = -1; // meeting longer than 1 year means no limit
				} else {
					bDataValid = false;
					break;
				}
			}
		}

		// merge new items into our queue
		if( bDataValid ) {
			time_t tNow = time( NULL );

			meetingext_list::iterator it = m_mlMeetingQueue.begin(), et = m_mlMeetingQueue.end();
			while( ! mlNew.empty() ) {
				BBQ_MeetingInfo_Ext * pNew = mlNew.front(); mlNew.pop_front();

				// check if the data is already loaded or not
				if( m_dictMeetingId.find( pNew->info.strMeetingId ) != m_dictMeetingId.end() ) {
					// already loaded, ignore
					BBQ_MeetingInfo_Ext_DELETE( pNew );
					continue;
				}
				
				if( pNew->info.status == MCC_INIT ) { // it's a new meeting
					// set a 256bit AES key for the new meeting
#if P_SSL
					RAND_bytes( (unsigned char *)  pNew->info.aesKey, 32 );
#else
					for( int j=0; j<32; j++ ) pNew->info.aesKey[j] = rand() & 0xFF;
#endif

					// seek valid position
					while( it != et ) {
						if( pNew->info.tBeginTime >= (*it)->info.tBeginTime ) it ++;
						else break;
					}
					if( it != et ) {
						m_mlMeetingQueue.insert( it, pNew );
					} else {
						m_mlMeetingQueue.push_back( pNew );
					}

					m_dictMeetingId[ pNew->info.strMeetingId ] = 1;

					m_UserActionLog.Output( PLog::Info, PString(PString::Printf, "\"%s(%s)\" loaded to meeting queue.", pNew->info.strMeetingName, pNew->info.strMeetingId) );
					
				} else if ( pNew->info.status == MCC_BEGIN ) { // a meeting that already started, (useful if vfon server crashed and restarted)

					m_mlMeetingList.push_back( pNew );
					m_dictMeetingId[ pNew->info.strMeetingId ] = 1;

					UpdateUserMeetingRef( pNew );

					m_UserActionLog.Output( PLog::Info, PString(PString::Printf, "\"%s(%s)\" loaded but already started.", pNew->info.strMeetingName, pNew->info.strMeetingId) );
				} else {
					m_UserActionLog.Output( PLog::Info, PString(PString::Printf, "\"%s(%s)\" loaded but status invalid, ignore..", pNew->info.strMeetingName, pNew->info.strMeetingId) );

					// unexpected status, ignore
					BBQ_MeetingInfo_Ext_DELETE( pNew );
				}

			}
		}
	}

	return true;
}

bool BBQServer::LoadNewOrChangeMeetingById( const char * lpszMeetingId  )
{
	// if server is in standby status, then do not load meeting info
	if( m_status.nServerStatus == BBQServer::STANDBY ) return false;

	WriteLock lock( m_mutexRecord );

	// load meeting booking info ( within 10 minutes )
	meetingext_list mlNew;
	if( m_pDB && (m_pDB->ReadConferenceBookingExt( mlNew, 0, 0, 0, lpszMeetingId )) ) {
		if( mlNew.size() > 0 ) {
			BBQ_MeetingInfo_Ext * pNew = mlNew.front(); mlNew.pop_front();

			if( pNew->info.nDuration >= ONE_YEAR ) pNew->info.nDuration = -1; // meeting longer than 1 year means no limit

			return AddOrChangeMeeting( pNew );
		}
	}

	return false;
}

bool BBQServer::ReNotifyMeetingById( const char * lpszMeetingId  )
{
	if( lpszMeetingId == NULL ) return false;

	WriteLock lock( m_mutexRecord );

	for( meetingext_list::iterator iter = m_mlMeetingList.begin(), eiter = m_mlMeetingList.end(); iter != eiter; iter ++ ) {
		BBQ_MeetingInfo_Ext * p = * iter;

		if( 0 == strcmp( p->info.strMeetingId, lpszMeetingId ) ) {
			NotifyClientMeetingEvent( p, MCC_BEGIN, m_config.meeting_begin_notify );
			return true;
		}
	}


	return false;
}

bool BBQServer::AddOrChangeMeeting( BBQ_MeetingInfo_Ext * pNew )
{
	//// for debug only
	//if( pNew == NULL ) {
	//	pNew = BBQ_MeetingInfo_Ext_NEW( 1, 1 );

	//	BBQ_MeetingInfo * pM = & pNew->info;

	//	PTime now;
	//	strcpy( pM->strMeetingId, now.AsString("yyyy_MM_dd_hh_mm_ss_uuu", PTime::Local) );
	//	strcpy( pM->strMeetingName, now.AsString("test_yyyy_MM_dd_hh_mm_ss_uuu", PTime::Local) );
	//	pM->tBeginTime = time(NULL);
	//	pM->nDuration = 300;
	//	pM->nMcuId = 118;
	//	pM->nRoomId = 3409;

	//	pM->flags.bMCU = 1;
	//	pM->flags.bAllowJoin = 1;
	//	pM->flags.bFreeStart = 1;
	//	pM->flags.bInitMuteAll = 1;

	//	pM->flags.bLocalIdList = 1;
	//	pM->nIdCount = 1;
	//	pM->localIdList[0] = 100176;

	//	pNew->pUsers[0].user.userid = 100176;
	//	pNew->pUsers[0].role_id = 1;
	//	pNew->pRoles[0].role_id = 1;
	//	pNew->pRoles[0].role_priv = 0x0a;
	//}

	WriteLock lock( m_mutexRecord );

	int nCurrentStage = MCC_INVALID;
	time_t tNow = time(NULL);

	// remove the old one first
	for( meetingext_list::iterator iter = m_mlMeetingQueue.begin(), eiter = m_mlMeetingQueue.end(); iter != eiter; /*iter++*/ ) {
		BBQ_MeetingInfo_Ext * p = * iter;

		if( 0 == strcmp( p->info.strMeetingId, pNew->info.strMeetingId ) ) {
			memcpy( pNew->info.aesKey, p->info.aesKey, 32 );

			nCurrentStage = 0;
			STL_ERASE( m_mlMeetingQueue, meetingext_list::iterator, iter );
			DestroyMeetingInfo( p );
		} else {
			iter ++;
		}
	}

	//for( meetingext_list::iterator iter = m_mlMeetingNotifying.begin(), eiter = m_mlMeetingNotifying.end(); iter != eiter; /*iter++*/ ) {
	//	BBQ_MeetingInfo_Ext * p = * iter;

	//	if( 0 == strcmp( p->info.strMeetingId, pNew->info.strMeetingId ) ) {
	//		memcpy( pNew->info.aesKey, p->info.aesKey, 32 );

	//		nCurrentStage = MCC_BEGIN;
	//		STL_ERASE( m_mlMeetingNotifying, meetingext_list::iterator, iter );
	//		DestroyMeetingInfo( p );
	//	} else {
	//		iter ++;
	//	}
	//}
	for( meetingext_list::iterator iter = m_mlMeetingList.begin(), eiter = m_mlMeetingList.end(); iter != eiter; /*iter++*/ ) {
		BBQ_MeetingInfo_Ext * p = * iter;

		if( 0 == strcmp( p->info.strMeetingId, pNew->info.strMeetingId ) ) {
			memcpy( pNew->info.aesKey, p->info.aesKey, 32 );

			nCurrentStage = MCC_BEGIN;
			STL_ERASE( m_mlMeetingList, meetingext_list::iterator, iter );
			DestroyMeetingInfo( p );
		} else {
			iter ++;
		}
	}

	if( nCurrentStage == MCC_BEGIN ) { // the meeting is already in progress

		NotifyClientMeetingEvent( pNew, MCC_BEGIN, m_config.meeting_begin_notify );

		m_mlMeetingList.push_back( pNew );

		m_dictMeetingId[ pNew->info.strMeetingId ] = 1;

	} else if( pNew->info.status == MCC_INIT ) { // it's a new meeting

		if( pNew->info.flags.bFreeStart /*pNew->nDuration == -1*/ ) {
			// free start meeting will not send forecast
		} else {
			if( pNew->info.tBeginTime <= tNow + m_config.nMeetingForecastTime ) {
				// scheduled meeting, send forecast to all members
				NotifyClientMeetingEvent( pNew, MCC_INIT, m_config.meeting_forecast );
				pNew->info.flags.bForecastSent = 1;
			}
		}

		// seek valid position
		meetingext_list::iterator it = m_mlMeetingQueue.begin(), et = m_mlMeetingQueue.end();
		while( it != et ) {
			if( pNew->info.tBeginTime >= (*it)->info.tBeginTime ) it ++;
			else break;
		}
		if( it != et ) {
			m_mlMeetingQueue.insert( it, pNew );
		} else {
			m_mlMeetingQueue.push_back( pNew );
		}

		m_dictMeetingId[ pNew->info.strMeetingId ] = 1;

	} else { // unexpected status, ignore
		BBQ_MeetingInfo_Ext_DELETE( pNew );
		return false;
	}

	return true;
}

void BBQServer::CheckMeetingStatus( void )
{
	WriteLock lock( m_mutexRecord );

	time_t tNow = time(NULL);

	// re-notify the meeting info if not handled
	{
		for( meetingext_list::iterator iter = m_mlMeetingQueue.begin(), eiter = m_mlMeetingQueue.end(); iter != eiter; /*iter++*/ ) {
			BBQ_MeetingInfo_Ext * p = * iter;
			BBQ_MeetingInfo * pInfo = & p->info;

			// check or send forecast msg
			if( pInfo->flags.bFreeStart /*pNew->nDuration == -1*/ ) {					// free start meeting will not send forecast
			} else if( pInfo->flags.bForecastSent ) {									// forecast already sent
			} else if( pInfo->tBeginTime <= tNow + m_config.nMeetingForecastTime ) {	// scheduled meeting, send forecast to all members
				NotifyClientMeetingEvent( p, MCC_INIT, m_config.meeting_forecast );
				pInfo->flags.bForecastSent = 1;
			}

			if( tNow < pInfo->tBeginTime ) { // meeting not start, skip to next one
				iter ++;
				continue;
			}

			// check validation
			bool bValid = (tNow < pInfo->tBeginTime+FIVE_MINUTES);
			
			if( bValid ) { // if the meeting did not finish yet
				NotifyClientMeetingEvent( p, MCC_BEGIN, m_config.meeting_begin_notify );
				iter ++;
			} else {
				pInfo->status = MCC_FAIL;
				CATCH_DB_FAIL(
					m_pDB->UpdateConferenceBookingExt( p );
				);

				if( m_config.write_useraction_log ) {
					PString strMsg( PString::Printf, 
						"Meeting (%s) assigned to MCU(%u), but failed to start in specified time.", 
						p->info.strMeetingId, 
						p->info.nMcuId );
					m_UserActionLog.Output( PLog::Info, strMsg );
				}

				// already reach meeting end time, give up this meeting now, and update DB record
				STL_ERASE( m_mlMeetingQueue, meetingext_list::iterator, iter );
				DestroyMeetingInfo( p );
			}
		}

		for( meetingext_list::iterator iter = m_mlMeetingList.begin(), eiter = m_mlMeetingList.end(); iter != eiter; /*iter ++*/ ) {
			BBQ_MeetingInfo_Ext * p = * iter;
			BBQ_MeetingInfo * pInfo = & p->info;

			bool bValid = false;
			bool bTerminated = false;

			if( pInfo->flags.bFreeStart ) {
				if( -1 == pInfo->nDuration ) {
					bValid = true;										// endless free start meeting should be notified by MCU only
				} else {
					bValid = ( tNow < pInfo->tBeginTime + pInfo->nDuration );	// schedule meeting in progress
					bTerminated = ( tNow > pInfo->tBeginTime + pInfo->nDuration + FIVE_MINUTES );
				}
			} else {
				if( -1 == pInfo->nDuration ) {
					bValid = true;
					bTerminated = false;
				} else {
					bValid = ( tNow < pInfo->tBeginTime + pInfo->nDuration );	// schedule meeting in progress
					bTerminated = ( tNow > pInfo->tBeginTime + pInfo->nDuration + FIVE_MINUTES );
				}
			}

			// send forecast before valid meeting is going to be ended
			if( bValid && (-1 != pInfo->nDuration) && (! pInfo->flags.bForecastSent) ) {
				bool bToEnd = ((tNow + m_config.nMeetingEndForecastTime) >= (pInfo->tBeginTime + pInfo->nDuration)); 
				if( bToEnd ) {
					if( m_config.write_useraction_log ) {
						PString strMsg( PString::Printf, 
							"Meeting (%s) will end soon, sending forecast to MCU(%u).", 
							p->info.strMeetingId, 
							p->info.nMcuId );
						m_UserActionLog.Output( PLog::Info, strMsg );
					}
					NotifyClientMeetingEvent( p, MCC_INITEND, m_config.meeting_end_forecast );
					pInfo->flags.bForecastSent = 1;
				}
			}

			if( bTerminated ) {
				if( m_config.write_useraction_log ) {
					PString strMsg( PString::Printf, 
						"Meeting (%s) on MCU (%u) already terminated 5 minutes ago, now clear.", 
						p->info.strMeetingId, 
						p->info.nMcuId );
					m_UserActionLog.Output( PLog::Info, strMsg );
				}

				STL_ERASE( m_mlMeetingList, meetingext_list::iterator, iter );

				pInfo->status = MCC_END;

				CATCH_DB_FAIL(
					m_pDB->UpdateConferenceBookingExt( p );
				);

				DestroyMeetingInfo( p );
			} else {
				iter ++;

				if( bValid ) {
					// in progress
				} else { 
					if( m_config.write_useraction_log ) {
						PString strMsg( PString::Printf, 
							"Meeting (%s) reaches end, now send END command to MCU(%u).", 
							p->info.strMeetingId, 
							p->info.nMcuId );
						m_UserActionLog.Output( PLog::Info, strMsg );
					}

					// the meeting should be ended, 
					// but shall I let MCU do it with it's own counter ?
					NotifyClientMeetingEvent( p, MCC_END, m_config.meeting_begin_notify );
				}
			}
		}
	}
}

void BBQServer::OnTick( void ) // called every 1 second
{
	// check dongle
	CheckDongleStatus();

	// inc counter
	m_nCountCounter ++;
	if( m_nCountCounter >= ONE_HOUR ) m_nCountCounter = 0;

	if( m_nCountCounter % TEN_SECONDS == 1 ) {

		// sometimes, DB is not ready when service started
		// DB connector will always retry connecting DB
		// and we should make sure the system config is also loaded after that
		if( ! m_bSystemConfigLoadedFromDB ) {
			if( m_pDB && m_pDB->IsDbReady() ) {
				m_bSystemConfigLoadedFromDB = LoadSystemConfigFromDB();
			}
		}
	}

	if( m_nCountCounter % ONE_MINUTE == 1 ) {

		//if( m_pDB && (m_pDB->GetLastErrorCode() == BBQDatabase::ServerFail) ) {
		//	if( m_pDB->Open( m_config.db_url ) ) {

		//		// after re-open DB, read in the system config data 
		//		LoadSystemConfigFromDB();
		//	}
		//}

		CheckProxyStatus();

		CheckMeetingStatus();
	}

	//if( m_config.nMeetingForecastTime < ONE_MINUTE ) m_config.nMeetingForecastTime = ONE_MINUTE;
	if( m_nCountCounter % m_config.nMeetingForecastTime == 1 ) {
		time_t tNow = time(NULL);
		LoadMeetingInfo( tNow, tNow + m_config.nMeetingForecastTime * 2, 0, NULL );
	}	

	if( m_nCountCounter % ONE_MINUTE == 1 || m_listClosedTcp.size() > 0 ) {
		CheckUserStatus();
	}

	if( m_nCountCounter % FIVE_MINUTES == 1 ) {
		CheckQosStatus();
	}

	// check user status
#ifdef ANTI_PACKET_LOSS
	m_nCidTimeCounter ++;
	if( m_nCidTimeCounter >= TEN_SECONDS ) {
		m_nCidTimeCounter = 0;

		// clear channel request history
		{
			LockIt sf( m_mutexCidTime );
			MS_TIME msCmp = GetHostUpTimeInMs() - 30000; // 30s ago
			for( UNIQUE_CID_TIME_MAP::iterator it = m_mapRequestCidTime.begin(), eit = m_mapRequestCidTime.end(); it != eit; /**/ ) {
				if( it->second < msCmp ) {
					//it = m_mapRequestCidTime.erase( it );
					STL_ERASE( m_mapRequestCidTime, UNIQUE_CID_TIME_MAP::iterator, it );
				} else {
					it ++;
				}
			}
			for( UNIQUE_CID_TIME_MAP::iterator it = m_mapReplyCidTime.begin(), eit = m_mapReplyCidTime.end(); it != eit; /**/ ) {
				if( it->second < msCmp ) {
					//it = m_mapReplyCidTime.erase( it );
					STL_ERASE( m_mapReplyCidTime, UNIQUE_CID_TIME_MAP::iterator, it );
				} else {
					it ++;
				}
			}
		}
	}
#endif

	// TODO: do other staffs
}

bool BBQServer::ConnectDatabase( BBQDatabase * pDB )
{
	WriteLock lock( m_mutexRecord );

	m_pDB = pDB;

	if( m_pDB ) {
		if( m_pDB->GetLastErrorCode() != BBQDatabase::ServerFail ) {
			; // ok, no error, go on
		} else {
			// DB connect failed, in most time VFON server cannot work.
			// but the VFON server will retry opening the database in OnTick() for error recovery
			;
		}
	} else {
		// null DB object, VFON server cannot work.
		OnEventDBFail();
	}

	return true;
}

void BBQServer::DisconnectDatabase( void )
{
	WriteLock lock( m_mutexRecord );

	m_pDB = NULL;
}

#define SIZE_8K		8192

bool BBQServer::LoadSystemConfigFromDB( void )
{
	WriteLock lock( m_mutexRecord );

	if( ! m_pDB ) return false; 
	if( ! m_pDB->IsDbReady() ) return false;

	// load config string from database: "SystemVariables" -> "MAIN;ACCOUNT;BUY;MCC;CHANGEEMAIL;..."
	char		buff[ SIZE_8K ];
	PString		strKey, strValue;
	if( m_pDB->ServerConfig_LoadString( "SystemVariables", buff, SIZE_8K ) ) {

		m_dictSystemConfig.clear();

		PString strValue = buff;
		PStringArray saKeys = strValue.Tokenise( ";" );
		int n = saKeys.GetSize();
		for( int i=0; i<n; i++ ) {
			strKey = saKeys[i].Trim();
			if( (strKey.GetLength() > 0) && m_pDB->ServerConfig_LoadString( strKey, buff, SIZE_8K ) ) {
				strValue = buff;
				m_dictSystemConfig[ strKey ] = strValue.Trim();
			}
		}
	}

	if( m_pDB->ServerConfig_LoadString( "SYSTEM_EXT", buff, SIZE_8K ) ) {
		m_strSystemExt = buff;
	}

	// read system config from db instead of local variable
#define DB_GET_BOOL( key, var ) \
	if( m_pDB->ServerConfig_LoadString( key, buff, SIZE_8K ) && (strlen(buff) > 0) ) { \
		var = strtol(buff,NULL,10) ? 1 : 0; \
	}
#define DB_GET_BOOL_REVERSE( key, var ) \
	if( m_pDB->ServerConfig_LoadString( key, buff, SIZE_8K ) && (strlen(buff) > 0) ) { \
		var = strtol(buff,NULL,10) ? 0 : 1; \
	}
#define DB_GET_INT32( key, var ) \
	if( m_pDB->ServerConfig_LoadString( key, buff, SIZE_8K ) && (strlen(buff) > 0) ) { \
		var = strtol(buff,NULL,10) ? 1 : 0; \
	}
#define DB_GET_UINT32( key, var ) \
	if( m_pDB->ServerConfig_LoadString( key, buff, SIZE_8K ) && (strlen(buff) > 0) ) { \
		uint32 a = 0; \
		sscanf( buff, "%lu", & a ); \
		var = a; \
	}
#define DB_GET_EXT_UINT32( key, var ) \
	if( m_pDB->ServerConfig_LoadString( key, buff, SIZE_8K ) && (strlen(buff) > 0) ) { \
		uint32 a = 0, b = 0, c = 0; \
		sscanf( buff, "%lu,%lu,%lu", & a, & b, & c ); \
		var.value[0] = a, var.value[1] = b, var.value[2] = c; \
	}

	DB_GET_BOOL( "POLICY_SUBSCRIBE_MULTIPLE_ID", m_config.allow_subscribe_more_id_per_pc );
	DB_GET_UINT32( "POLICY_SUBSCRIBE_START_ID", m_config.id_min );
	DB_GET_UINT32( "POLICY_SUBSCRIBE_TRIAL_DAYS", m_config.subscribe_trial_time );

	// for new subscriber
	//DB_GET_BOOL( "POLICY_SERVICE_NEW_CREATE", m_config.use_customized_flag_for_new );
	DB_GET_UINT32( "ACL_SERVICE_NEW_CREATE", m_config.flagsDefault.value );
	DB_GET_EXT_UINT32( "ACL_EXT_SERVICE_NEW_CREATE", m_config.extFlagsDefault );

	// for trial user
	DB_GET_BOOL_REVERSE( "POLICY_SERVICE_TRIAL", m_config.use_customized_flag_for_trial );
	DB_GET_UINT32( "ACL_SERVICE_TRIAL", m_config.flagsTrial.value );
	DB_GET_EXT_UINT32( "ACL_EXT_SERVICE_TRIAL", m_config.extFlagsTrial );

	// for expired free use
	DB_GET_BOOL_REVERSE( "POLICY_SERVICE_EXPIRED", m_config.use_customized_flag_for_free );
	DB_GET_UINT32( "ACL_SERVICE_EXPIRED", m_config.flagsFree.value );
	DB_GET_EXT_UINT32( "ACL_EXT_SERVICE_EXPIRED", m_config.extFlagsFree );

	// for prepaid normal user
	DB_GET_BOOL_REVERSE( "POLICY_SERVICE_PAID", m_config.use_customized_flag_for_normal );
	DB_GET_UINT32( "ACL_SERVICE_PAID", m_config.flagsPrepaid.value );
	DB_GET_EXT_UINT32( "ACL_EXT_SERVICE_PAID", m_config.extFlagsPrepaid );

	// CUG paramters
	DB_GET_BOOL( "CUG_MODE", m_config.enable_cug );
	DB_GET_BOOL( "CUG_MULTI_LAYERED", m_config.run_in_asp_mode );
	DB_GET_BOOL( "CUG_ALLOW_GROUPID_LOGIN", m_config.allow_group_id_login_as_user );
	DB_GET_BOOL( "CUG_HIDE_GROUP_TREE", m_config.hide_id_tree_to_client );
	DB_GET_BOOL( "CUG_ALLOW_PRIVATE_BUDDY", m_config.cug_have_private_buddy );
	DB_GET_BOOL( "CUG_CALL_PUBLIC_USERS", m_config.allow_cug_call_public );

	// CALLBACK_ADDR
	if( m_pDB->ServerConfig_LoadString( "CALLBACK_ADDR", buff, SIZE_8K ) && (strlen(buff) > 0) ) {
		m_strCallbackTrustIp = buff;
	}

	return true;
}

void BBQServer::CleanPKICertKey( void )
{
	if( m_pzPKICertPEM ) { free( m_pzPKICertPEM ); m_pzPKICertPEM=NULL; }
	if( m_pzPKIPrivKeyPEM ) { free( m_pzPKIPrivKeyPEM ); m_pzPKIPrivKeyPEM=NULL; }
	if( m_pPKICertX509 ) { PKI_free_cert( m_pPKICertX509 ); m_pPKICertX509=NULL; }
	if( m_pPKIPrivKey ) { PKI_free_key( m_pPKIPrivKey ); m_pPKIPrivKey=NULL; }
}

bool BBQServer::LoadPKICertKey( const char * certfile, const char * privkeyfile, const char * pass )
{
	CleanPKICertKey();

	PString errmsg = "";

	// load cert
	m_pzPKICertPEM = ReadTextFromFile( certfile, 8192 );
	if( m_pzPKICertPEM ) {
		m_pPKICertX509 = PKI_PEM_to_X509( m_pzPKICertPEM, strlen(m_pzPKICertPEM) );
		if( m_pPKICertX509 ) {

			// load priv key
			m_pzPKIPrivKeyPEM = ReadTextFromFile( privkeyfile, 8192 );
			if( m_pzPKIPrivKeyPEM ) {
				m_pPKIPrivKey = PKI_PEM_to_private_key( m_pzPKIPrivKeyPEM, strlen(m_pzPKIPrivKeyPEM), pass );
				if( m_pPKIPrivKey ) {
					if( PKI_check_X509_privkey( m_pPKICertX509, m_pPKIPrivKey ) ) {

						PLOG( m_pLogger, Info, "PKI certificate and private key loaded and checked." );
						return true;
					} else {
						errmsg = "Error: PKI certificate and private key not match.";
					}
				} else {
					errmsg = "Error: Failed to load PKI private key from PEM, please check the passphrase.";
				}
			} else {
				errmsg = PString( PString::Printf, "Error: Cannot open PEM file '%s' for PKI private key.", privkeyfile ); 
			}
		} else {
			errmsg = "Error: Invalid PEM file for PKI certificate.";
		}
	} else {
		errmsg = PString( PString::Printf, "Error: Cannot open PEM file '%s' for PKI certificate.", certfile );
	}

	PLOG( m_pLogger, Error, errmsg );
	CleanPKICertKey();

	return false;
}

bool BBQServer::Startup( PConfig & cfg )
{
	if( m_bInited ) return false;

	PString str;
	PStringArray strs;
	WORD nPort;
	int n, i;

	// must init openssl functions
	cert_rsa_aes_init();

	cfg.SetDefaultSection( KEY_BBQServer );

	// load PKI certificate if there is
	//if( ! cfg.HasKey( KEY_PKICertificate ) ) cfg.SetString( KEY_PKICertificate, DEFAULT_PKICertificate );
	//if( ! cfg.HasKey( KEY_PKIPrivateKey ) ) cfg.SetString( KEY_PKIPrivateKey, DEFAULT_PKIPrivateKey );
	//if( ! cfg.HasKey( KEY_PKIPrivateKeyPassEnc ) ) cfg.SetString( KEY_PKIPrivateKeyPassEnc, DEFAULT_PKIPrivateKeyPassEnc );
	//if( ! cfg.HasKey( KEY_PKIPrivateKeyPass ) ) cfg.SetString( KEY_PKIPrivateKeyPass, DEFAULT_PKIPrivateKeyPass );

	PString certFile = cfg.GetString(  KEY_PKICertificate, DEFAULT_PKICertificate );
	PString privkeyFile = cfg.GetString( KEY_PKIPrivateKey, DEFAULT_PKIPrivateKey );
	PString privkeyFile_passphraseEnc = cfg.GetString( KEY_PKIPrivateKeyPassEnc, DEFAULT_PKIPrivateKeyPassEnc );
	PString privkeyFile_passphrase;

	// if we have passphrase in enc form, we decrypt it
	int len = strlen( privkeyFile_passphraseEnc );
	if( len > 0 ) {
		privkeyFile_passphrase = AES_decrypt_pin( privkeyFile_passphraseEnc );
	}
	
	if( strlen(privkeyFile_passphrase) == 0 ) {
		privkeyFile_passphrase = cfg.GetString( KEY_PKIPrivateKeyPass, DEFAULT_PKIPrivateKeyPass );
	}

	if( PFile::Exists( certFile ) && PFile::Exists( privkeyFile ) ) {
		LoadPKICertKey( certFile, privkeyFile, privkeyFile_passphrase );
	} else {
		PLOG( m_pLogger, Info, PString(PString::Printf, "Server PKI certificate and private key files ('%s' and '%s') not found, skip.", (const char *)certFile, (const char *)privkeyFile ) );
	}

	m_config.nUserParamA = cfg.GetInteger( "UserParamA", 1 );
	m_config.nUserParamB = cfg.GetInteger( "UserParamB", 0 );

	// read config
	// if config does not exist, then create and initalize it to default values
	if( ! cfg.HasKey( KEY_HeartbeatTimeout ) ) cfg.SetInteger( KEY_HeartbeatTimeout, DEFAULT_HeartbeatTimeout );
	m_config.heartbeat_timeout = cfg.GetInteger( KEY_HeartbeatTimeout, DEFAULT_HeartbeatTimeout );

	if( ! cfg.HasKey( KEY_IDMax ) ) cfg.SetInteger( KEY_IDMax, DEFAULT_IDMax );
	if( ! cfg.HasKey( KEY_IDReserved ) ) cfg.SetInteger( KEY_IDReserved, DEFAULT_IDMax / 10 );
	m_config.id_max = cfg.GetInteger( KEY_IDMax, DEFAULT_IDMax );
	m_config.id_min = cfg.GetInteger( KEY_IDReserved, m_config.id_max / 10 );

	if( ! cfg.HasKey( KEY_RunAsClusterCenter ) ) cfg.SetInteger( KEY_RunAsClusterCenter, DEFAULT_RunAsClusterCenter );
	m_config.run_as_cluster_center = cfg.GetInteger( KEY_RunAsClusterCenter, DEFAULT_RunAsClusterCenter );

	//if( ! cfg.HasKey( KEY_DBURL ) ) cfg.SetString( KEY_DBURL, DEFAULT_DBURL );
	m_config.db_type = strdup( DEFAULT_DBType);//strdup( cfg.GetString( KEY_DBType, DEFAULT_DBType ) );
	m_config.db_url = strdup( DEFAULT_DBURL);//strdup( cfg.GetString( KEY_DBURL, DEFAULT_DBURL ) );

	// log related
	if( ! cfg.HasKey( KEY_WriteHeartbeatFile ) ) cfg.SetInteger( KEY_WriteHeartbeatFile, DEFAULT_WriteHeartbeatFile );
	m_config.write_hearbeat_file = cfg.GetInteger( KEY_WriteHeartbeatFile, DEFAULT_WriteHeartbeatFile );
	if( m_config.write_hearbeat_file ) {
		PString folder( "logs" );
		folder += PDirectory::IsSeparator('\\') ? "\\" : "/";

		unlink( folder +  STR_AnotherAlive );

		unlink( folder +  TypeToString( BBQServer::PRIMARY ) );
		unlink( folder +  TypeToString( BBQServer::SECONDARY ) );
		unlink( folder +  TypeToString( BBQServer::STANDALONE ) );
		unlink( folder +  TypeToString( BBQServer::CLUSTER ) );

		unlink( folder +  StatusToString( BBQServer::ACTIVE ) );
		unlink( folder +  StatusToString( BBQServer::STANDBY ) );
		unlink( folder +  StatusToString( BBQServer::DOWN ) );
	}

	if( ! cfg.HasKey( KEY_CheckDNSInHeartbeat ) ) cfg.SetInteger( KEY_CheckDNSInHeartbeat, DEFAULT_CheckDNSInHeartbeat );
	m_config.check_dns_in_heartbeat = cfg.GetInteger( KEY_CheckDNSInHeartbeat, DEFAULT_CheckDNSInHeartbeat );

	if( ! cfg.HasKey( KEY_WriteUserActionLog ) ) cfg.SetInteger( KEY_WriteUserActionLog, DEFAULT_WriteUserActionLog );
	m_config.write_useraction_log = cfg.GetInteger( KEY_WriteUserActionLog, DEFAULT_WriteUserActionLog );

	if( ! cfg.HasKey( KEY_DBCacheTime ) ) cfg.SetInteger( KEY_DBCacheTime, DEFAULT_DBCacheTime );
	m_config.nDbCacheTime = cfg.GetInteger( KEY_DBCacheTime, DEFAULT_DBCacheTime ) * 1000; // convert to ms

	if( ! cfg.HasKey( KEY_MeetingForecastTime ) ) cfg.SetInteger( KEY_MeetingForecastTime, DEFAULT_MeetingForecastTime );
	m_config.nMeetingForecastTime = cfg.GetInteger( KEY_MeetingForecastTime, DEFAULT_MeetingForecastTime );
	if( m_config.nMeetingForecastTime < ONE_MINUTE ) m_config.nMeetingForecastTime = ONE_MINUTE;

	if( ! cfg.HasKey( KEY_MeetingEndForecastTime ) ) cfg.SetInteger( KEY_MeetingEndForecastTime, DEFAULT_MeetingEndForecastTime );
	m_config.nMeetingEndForecastTime = cfg.GetInteger( KEY_MeetingEndForecastTime, DEFAULT_MeetingEndForecastTime );
	if( m_config.nMeetingEndForecastTime < ONE_MINUTE ) m_config.nMeetingEndForecastTime = ONE_MINUTE;

	if( ! cfg.HasKey( KEY_MeetingForecast ) ) cfg.SetInteger( KEY_MeetingForecast, DEFAULT_MeetingForecast );
	m_config.meeting_forecast = cfg.GetInteger( KEY_MeetingForecast, DEFAULT_MeetingForecast ) ? 1 : 0;

	if( ! cfg.HasKey( KEY_MeetingEndForecast ) ) cfg.SetInteger( KEY_MeetingEndForecast, DEFAULT_MeetingEndForecast );
	m_config.meeting_end_forecast = cfg.GetInteger( KEY_MeetingForecast, DEFAULT_MeetingEndForecast ) ? 1 : 0;

	if( ! cfg.HasKey( KEY_MeetingBeginNotify ) ) cfg.SetInteger( KEY_MeetingBeginNotify, DEFAULT_MeetingBeginNotify );
	m_config.meeting_begin_notify = cfg.GetInteger( KEY_MeetingBeginNotify, DEFAULT_MeetingBeginNotify ) ? 1 : 0;

	if( ! cfg.HasKey( KEY_IgnoreDBError ) ) cfg.SetInteger( KEY_IgnoreDBError, DEFAULT_IgnoreDBError );
	m_config.ignore_db_err = cfg.GetInteger( KEY_IgnoreDBError, DEFAULT_IgnoreDBError );

	if( ! cfg.HasKey( KEY_QoSWarningLevel ) ) cfg.SetInteger( KEY_QoSWarningLevel, 0 );
	m_config.QoS_warning_level = cfg.GetInteger( KEY_QoSWarningLevel, 0 ) % 100;	// convert to percent

	if( ! cfg.HasKey( KEY_QoSStatResetTime ) ) cfg.SetInteger( KEY_QoSStatResetTime, 0 );
	m_config.QoS_stat_reset_time = cfg.GetInteger( KEY_QoSStatResetTime, 0 ) * ONE_MINUTE / FIVE_MINUTES; // convert to 5 min unit

	if( false&&m_config.write_useraction_log ) {
		if( ! cfg.HasKey( KEY_UserActionLogFile ) ) cfg.SetString( KEY_UserActionLogFile, DEFAULT_UserActionLogFile );
		if( ! cfg.HasKey( KEY_UserActionLogMaxSize ) ) cfg.SetInteger( KEY_UserActionLogMaxSize, DEFAULT_UserActionLogMaxSize );

		PString strLogFile = cfg.GetString( KEY_UserActionLogFile, DEFAULT_UserActionLogFile );
		int nLogMaxSize = cfg.GetInteger( KEY_UserActionLogMaxSize,  DEFAULT_UserActionLogMaxSize );

		m_UserActionLog.Open( strLogFile, nLogMaxSize );
	}

	if( ! cfg.HasKey( KEY_DBWriteInOutLog ) ) cfg.SetInteger( KEY_DBWriteInOutLog, DEFAULT_DBWriteInOutLog );
	if( ! cfg.HasKey( KEY_DBWriteConferenceLog ) ) cfg.SetInteger( KEY_DBWriteConferenceLog, DEFAULT_DBWriteConferenceLog );

	m_config.dbwrite_loginout_log = cfg.GetInteger( KEY_DBWriteInOutLog, DEFAULT_DBWriteInOutLog );
	m_config.dbwrite_conference_log = cfg.GetInteger( KEY_DBWriteConferenceLog, DEFAULT_DBWriteConferenceLog );

	// policy related
	if( ! cfg.HasKey( KEY_IndexOnSerial ) ) cfg.SetBoolean( KEY_IndexOnSerial, DEFAULT_IndexOnSerial );
	m_config.index_serial = cfg.GetBoolean( KEY_IndexOnSerial, DEFAULT_IndexOnSerial );

	if( ! cfg.HasKey( KEY_DisableSerialKeyCheck ) ) cfg.SetBoolean( KEY_DisableSerialKeyCheck, DEFAULT_DisableSerialKeyCheck );
	m_config.disable_serial_key_check = cfg.GetBoolean( KEY_DisableSerialKeyCheck, DEFAULT_DisableSerialKeyCheck );

	if( ! cfg.HasKey( KEY_FixedPCTrialServiceDays ) ) cfg.SetInteger( KEY_FixedPCTrialServiceDays, DEFAULT_FixedPCTrialServiceDays );
	m_config.subscribe_trial_time = cfg.GetInteger( KEY_FixedPCTrialServiceDays, DEFAULT_FixedPCTrialServiceDays );

	if( ! cfg.HasKey( KEY_DisableSubscribe ) ) cfg.SetInteger( KEY_DisableSubscribe, DEFAULT_DisableSubscribe );
	if( ! cfg.HasKey( KEY_AllowMoreIdPerPC ) ) cfg.SetInteger( KEY_AllowMoreIdPerPC, DEFAULT_AllowMoreIdPerPC );
	if( ! cfg.HasKey( KEY_DisableBindUser ) ) cfg.SetInteger( KEY_DisableBindUser, DEFAULT_DisableBindUser );
	m_config.disable_subscribe = cfg.GetInteger( KEY_DisableSubscribe, DEFAULT_DisableSubscribe );
	m_config.allow_subscribe_more_id_per_pc = cfg.GetInteger( KEY_AllowMoreIdPerPC, DEFAULT_AllowMoreIdPerPC );
	m_config.disable_bind_user = cfg.GetInteger( KEY_DisableBindUser, DEFAULT_DisableBindUser );

	if( ! cfg.HasKey( KEY_CustomizeNormalUser ) ) cfg.SetInteger( KEY_CustomizeNormalUser, DEFAULT_CustomizeNormalUser );
	if( ! cfg.HasKey( KEY_CustomizeTrialUser ) ) cfg.SetInteger( KEY_CustomizeTrialUser, DEFAULT_CustomizeTrialUser );
	if( ! cfg.HasKey( KEY_CustomizeFreeUser ) ) cfg.SetInteger( KEY_CustomizeFreeUser, DEFAULT_CustomizeFreeUser );
	m_config.use_customized_flag_for_normal = cfg.GetInteger( KEY_CustomizeNormalUser, DEFAULT_CustomizeNormalUser );
	m_config.use_customized_flag_for_trial = cfg.GetInteger( KEY_CustomizeTrialUser, DEFAULT_CustomizeTrialUser );
	m_config.use_customized_flag_for_free = cfg.GetInteger( KEY_CustomizeFreeUser, DEFAULT_CustomizeFreeUser );

	if( ! cfg.HasKey( KEY_FeaturesDefault ) ) cfg.SetInteger( KEY_FeaturesDefault, DEFAULT_FeaturesDefault );
	m_config.flagsDefault.value = cfg.GetInteger( KEY_FeaturesDefault, DEFAULT_FeaturesDefault );

	if( ! cfg.HasKey( KEY_FeaturesTrial ) ) cfg.SetInteger( KEY_FeaturesTrial, DEFAULT_FeaturesTrial );
	m_config.flagsTrial.value = cfg.GetInteger( KEY_FeaturesTrial, DEFAULT_FeaturesTrial );

	if( ! cfg.HasKey( KEY_FeaturesFree ) ) cfg.SetInteger( KEY_FeaturesFree, DEFAULT_FeaturesFree );
	m_config.flagsFree.value = cfg.GetInteger( KEY_FeaturesFree, DEFAULT_FeaturesFree );

	if( ! cfg.HasKey( KEY_FeaturesPrepaid ) ) cfg.SetInteger( KEY_FeaturesPrepaid, DEFAULT_FeaturesPrepaid );
	m_config.flagsPrepaid.value = cfg.GetInteger( KEY_FeaturesPrepaid, DEFAULT_FeaturesPrepaid );

	if( ! cfg.HasKey( KEY_PrepaidPolicy ) ) cfg.SetInteger( KEY_PrepaidPolicy, DEFAULT_PrepaidPolicy );
	m_config.prepaid_policy = cfg.GetInteger( KEY_PrepaidPolicy, DEFAULT_PrepaidPolicy );

	if( ! cfg.HasKey( KEY_EnableCUG) ) cfg.SetInteger( KEY_EnableCUG, DEFAULT_EnableCUG );
	m_config.enable_cug = cfg.GetInteger( KEY_EnableCUG, DEFAULT_EnableCUG );

	if( ! cfg.HasKey( KEY_RunInASPMode ) ) cfg.SetInteger( KEY_RunInASPMode, DEFAULT_RunInASPMode );
	m_config.run_in_asp_mode = cfg.GetInteger( KEY_RunInASPMode, DEFAULT_RunInASPMode );

	if( ! cfg.HasKey( KEY_AllowCUGCallPublic ) ) cfg.SetInteger( KEY_AllowCUGCallPublic, DEFAULT_AllowCUGCallPublic );
	m_config.allow_cug_call_public = cfg.GetInteger( KEY_AllowCUGCallPublic, DEFAULT_AllowCUGCallPublic );

	if( ! cfg.HasKey( KEY_CUGHavePrivateBuddy ) ) cfg.SetInteger( KEY_CUGHavePrivateBuddy, DEFAULT_CUGHavePrivateBuddy );
	m_config.cug_have_private_buddy = cfg.GetInteger( KEY_CUGHavePrivateBuddy, DEFAULT_CUGHavePrivateBuddy );

	if( ! cfg.HasKey( KEY_HideIdTreeToClient ) ) cfg.SetInteger( KEY_HideIdTreeToClient, DEFAULT_HideIdTreeToClient );
	m_config.hide_id_tree_to_client = cfg.GetInteger( KEY_HideIdTreeToClient, DEFAULT_HideIdTreeToClient );

	if( ! cfg.HasKey( KEY_AllowGroupIdLoginAsUser ) ) cfg.SetInteger( KEY_AllowGroupIdLoginAsUser, DEFAULT_AllowGroupIdLoginAsUser );
	m_config.allow_group_id_login_as_user = cfg.GetInteger( KEY_AllowGroupIdLoginAsUser, DEFAULT_AllowGroupIdLoginAsUser );

	if( ! cfg.HasKey( KEY_SyncBuddyWithAuthGateway ) ) cfg.SetInteger( KEY_SyncBuddyWithAuthGateway, DEFAULT_SyncBuddyWithAuthGateway );
	m_config.sync_buddy_with_authgateway = cfg.GetInteger( KEY_SyncBuddyWithAuthGateway, DEFAULT_SyncBuddyWithAuthGateway );

	//if( ! cfg.HasKey( KEY_AuthGatewayBindTypes ) ) cfg.SetString( KEY_AuthGatewayBindTypes, DEFAULT_AuthGatewayBindTypes );
	str = cfg.GetString( KEY_AuthGatewayBindTypes, DEFAULT_AuthGatewayBindTypes );
	m_strsBindTypes = str.Tokenise( ",; " );

	//if( ! cfg.HasKey( KEY_WelcomeMsg ) ) cfg.SetString( KEY_WelcomeMsg, "" );
	m_strWelcomeMsg = cfg.GetString( KEY_WelcomeMsg, "LPN Group" );

	//if( ! cfg.HasKey( KEY_AcceptClientAppList ) ) cfg.SetString( KEY_AcceptClientAppList, "" );
	m_strAcceptClientAppList = cfg.GetString( KEY_AcceptClientAppList );

	DWORD ip = 0;
	PString iphost_string;
	const char * st = NULL;
	const char * p = NULL;

	// server address of this server
	PString strServerAddress = cfg.GetString( KEY_MyIp, "0.0.0.0" );
	if( strServerAddress.GetLength() == 0 ) strServerAddress = "0.0.0.0";
	{
		st = (const char *) strServerAddress;
		p = strstr( st, ":" );
		if( p ) {
			iphost_string = PString( st, (p - st) );
			m_config.thisServerAddress.port = (uint16)strtol( p+1, NULL, 0 );
		} else {
			iphost_string = st;
			m_config.thisServerAddress.port = SIM_PORT;
		}
		if( HostnameToIp( iphost_string, ip ) ) {
			PLOG( m_pLogger, Info, 
				PString(PString::Printf, "This server '%s' is resolved as: %s.", 
				(const char *) iphost_string,
				(const char *) ToString( ip ) ) );
		} else {
			PLOG( m_pLogger, Error,
				PString(PString::Printf, "Failed to resolve this server '%s' to IP address.", (const char *) iphost_string ) );
		}
	}

	m_config.dwListenIp = ip;

	// TODO: mapping to external IP
	strServerAddress = cfg.GetString( KEY_ExternalIp, "0.0.0.0" );
	if( strServerAddress.GetLength() == 0 ) strServerAddress = "0.0.0.0";
	if( 0 != strcmp("0.0.0.0", strServerAddress) ) {
		PString iphost_string;
		st = (const char *) strServerAddress;
		p = strstr( st, ":" );
		if( p ) {
			iphost_string = PString( st, (p - st) );
			m_config.thisServerAddress.port = (uint16)strtol( p+1, NULL, 0 );
		} else {
			iphost_string = st;
			m_config.thisServerAddress.port = SIM_PORT;
		}
		if( HostnameToIp( iphost_string, ip ) ) {
		} else {
		}
	}

	if( ip == 0 ) { // if not configured, or failed to resolve, then use local ip
		PIPSocket::Address local_addr;
		GetHostAddress( local_addr );
		ip = (DWORD) local_addr;
	}

	m_config.thisServerAddress.ip = ip;

	// server type
	//if( ! cfg.HasKey( KEY_ServerType ) ) cfg.SetString( KEY_ServerType, STR_STANDALONE );
	//m_config.nServerType = TypeToInt( cfg.GetString( KEY_ServerType, STR_STANDALONE ) );
	//if( m_config.nServerType == INVALID_TYPE ) m_config.nServerType = STANDALONE;
  m_config.nServerType = STANDALONE;
	switch( m_config.nServerType ) {
	case PRIMARY:
	case SECONDARY:
		if( m_status.nServerStatus != BBQServer::DOWN ) m_status.nServerStatus = STANDBY;	
		// the dual-system server is started as STANDBY, 
		// then they will try to connect to each other, and will switch status.
		break;
	case STANDALONE:
	case CLUSTER:
	default:
		if( m_status.nServerStatus != BBQServer::DOWN ) m_status.nServerStatus = ACTIVE;
	}

	// server address of another server
	ip = 0;
	strServerAddress = cfg.GetString( KEY_AnotherServerIp, "0.0.0.0" );
	{
		st = (const char *) strServerAddress;
		p = strstr( st, ":" );
		if( p ) {
			iphost_string = PString( st, (p - st) );
			m_config.anotherServerAddress.port = (uint16)strtol( p+1, NULL, 0 );
		} else {
			iphost_string = st;
			m_config.anotherServerAddress.port = SIM_PORT;
		}
		if( HostnameToIp( iphost_string, ip ) ) {
			PLOG( m_pLogger, Info, 
				PString(PString::Printf, "Interlink server '%s' is resolved as: %s.", 
				(const char *) iphost_string,
				(const char *) ToString( ip ) ) );
		} else {
			PLOG( m_pLogger, Error,
				PString(PString::Printf, "Failed to resolve interlink server '%s' to IP address.", (const char *) iphost_string ) );
		}
	}
	m_config.anotherServerAddress.ip = ip;

	// second server address of another server
	ip = 0;
	strServerAddress = cfg.GetString( KEY_SecondInterlinkIp, "0.0.0.0" );
	{
		st = (const char *) strServerAddress;
		p = strstr( st, ":" );
		if( p ) {
			iphost_string = PString( st, (p - st) );
			m_config.anotherServerAddress2.port = (uint16)strtol( p+1, NULL, 0 );
		} else {
			iphost_string = st;
			m_config.anotherServerAddress2.port = SIM_PORT;
		}
		if( HostnameToIp( iphost_string, ip ) ) {
			PLOG( m_pLogger, Info, 
				PString(PString::Printf, "Second interlink server '%s' is resolved as: %s.", 
				(const char *) iphost_string,
				(const char *) ToString( ip ) ) );
		} else {
			PLOG( m_pLogger, Error,
				PString(PString::Printf, "Failed to resolve second interlink server '%s' to IP address.", (const char *) iphost_string ) );
		}
	}
	m_config.anotherServerAddress2.ip = ip;

	// server address of uplink server
	ip = 0;
	{
		strServerAddress = cfg.GetString( KEY_UplinkServerIp, "0.0.0.0" );
		st = (const char *) strServerAddress;
		p = strstr( st, ":" );
		if( p ) {
			iphost_string = PString( st, (p - st) );
			m_config.uplinkServerAddress.port = (uint16)strtol( p+1, NULL, 0 );
		} else {
			iphost_string = st;
			m_config.uplinkServerAddress.port = SIM_PORT;
		}
		if( HostnameToIp( iphost_string, ip ) ) {
			PLOG( m_pLogger, Info, 
				PString(PString::Printf, "Uplink server '%s' is resolved as: %s.", 
				(const char *) iphost_string,
				(const char *) ToString( ip ) ) );
		} else {
			PLOG( m_pLogger, Error,
				PString(PString::Printf, "Failed to resolve uplink server '%s' to IP address.", (const char *) iphost_string ) );
		}
	}
	m_config.uplinkServerAddress.ip = ip;

	m_config.use_tcp_for_server_link = cfg.GetBoolean( KEY_UseTcpForServerLink, FALSE );

	// m_config.domainInfo
	m_config.domainInfo.dwId = cfg.GetInteger( KEY_DomainId, 0 );
	strcpy( m_config.domainInfo.strAlias, cfg.GetString( KEY_DomainAlias, "") );
	strcpy( m_config.domainInfo.strPassword, cfg.GetString( KEY_DomainPassword, "" ) );

	m_config.nUserMax = GetUserLimit();

	if( m_config.run_as_cluster_center ) {
		m_bbqSwitch.Initialize( m_config.id_min, m_config.id_max );
	}

	cfg.SetDefaultSection( KEY_Terminal );

	// bind and listen to several TCP ports
	//if( ! cfg.HasKey( KEY_TcpPortList ) ) cfg.SetString( KEY_TcpPortList, DEFAULT_TcpPortList );
	str = DEFAULT_TcpPortList;///cfg.GetString( KEY_TcpPortList, DEFAULT_TcpPortList );
  //chen yuan
  //str += ",4323";
	strs = str.Tokenise( ", ;" );
	n = strs.GetSize();	

	if( ! cfg.HasKey(KEY_TcpListenerBacklog) ) cfg.SetInteger( KEY_TcpListenerBacklog, DEFAULT_TcpListenerBacklog );
	int nTcpListenerBacklog = cfg.GetInteger( KEY_TcpListenerBacklog, DEFAULT_TcpListenerBacklog );
	BOOL bUseTcpListenerThread = cfg.GetBoolean( KEY_UseTcpListenerThread, DEFAULT_UseTcpListenerThread );

	for( i=0; i<n; i++ ) {
		nPort = (WORD) strtol( strs[i], NULL, 0 );
		if( m_config.dwListenIp ) {
			ListenTcp(nPort, nTcpListenerBacklog, bUseTcpListenerThread, m_config.dwListenIp );
		} else {
			ListenTcp(nPort, nTcpListenerBacklog, bUseTcpListenerThread, INADDR_ANY );
		}
		if( i == 0 ) m_nDefaultTcpPort = nPort;
	}

	// bind and listen to several UDP ports
	///if( ! cfg.HasKey( KEY_UdpPortList ) ) cfg.SetString( KEY_UdpPortList, DEFAULT_UdpPortList );
	str = DEFAULT_UdpPortList;///cfg.GetString( KEY_UdpPortList, DEFAULT_UdpPortList );
	strs = str.Tokenise( ", ;" );
	n = strs.GetSize();	

	for( i=0; i<n; i++ ) {
		nPort = (WORD) strtol( strs[i], NULL, 0 );
		if( m_config.dwListenIp ) {
			ListenUdp( nPort, m_config.dwListenIp );
		} else {
			ListenUdp( nPort, INADDR_ANY );
		}

		if( i == 0 ) m_nDefaultUdpPort = nPort;
		if( i == 0 ) m_nMinUdpPort = nPort;
		if( i == n-1 ) m_nMaxUdpPort = nPort;
	}

	// set some TCP related parameters
	if( ! cfg.HasKey( KEY_TcpClientMax ) ) cfg.SetInteger( KEY_TcpClientMax, DEFAULT_TcpClientMax );
	if( ! cfg.HasKey( KEY_TcpIdleMax ) ) cfg.SetInteger( KEY_TcpIdleMax, DEFAULT_TcpIdleMax );
	DWORD dwMaxIdle = cfg.GetInteger( KEY_TcpIdleMax, DEFAULT_TcpIdleMax );
	int nMaxTcpClient = cfg.GetInteger( KEY_TcpClientMax, DEFAULT_TcpClientMax );

	int	nMaxSocketSize = GetMaxSocketSize();
	if( nMaxTcpClient > nMaxSocketSize - 8 ) {
		nMaxTcpClient = nMaxSocketSize - 8;
	}

	SetConnectionMax( nMaxTcpClient );
	SetConnectionIdleMax( dwMaxIdle * 1000 );

	if( ! cfg.HasKey( KEY_HandlerThreads ) ) cfg.SetInteger( KEY_HandlerThreads, DEFAULT_HandlerThreads );
	n = cfg.GetInteger( KEY_HandlerThreads, DEFAULT_HandlerThreads );
	SetNumberOfHandlers( n );

	if( ! cfg.HasKey( KEY_RequestQueueMax ) ) cfg.SetInteger( KEY_RequestQueueMax, DEFAULT_RequestQueueMax );
	n = cfg.GetInteger( KEY_RequestQueueMax, DEFAULT_RequestQueueMax );
	this->m_nRequestQueueMax = n;

	if( ! cfg.HasKey( KEY_MsgPostMax ) ) cfg.SetInteger( KEY_MsgPostMax, DEFAULT_MsgPostMax );
	n = cfg.GetInteger( KEY_MsgPostMax, DEFAULT_MsgPostMax );
	this->m_nMsgPostMax = n;

	cfg.SetDefaultSection( KEY_Client );

	m_VersionInfo.client.majorRequired = cfg.GetInteger( KEY_VersionMajor, DEFAULT_VersionMajor );
	m_VersionInfo.client.minorRequired = cfg.GetInteger( KEY_VersionMinor, DEFAULT_VersionMinor );
	m_VersionInfo.client.buildRequired = cfg.GetInteger( KEY_VersionBuild, DEFAULT_VersionBuild );

	m_VersionInfo.client.majorNew = cfg.GetInteger( KEY_VersionNewMajor, DEFAULT_VersionNewMajor );
	m_VersionInfo.client.minorNew = cfg.GetInteger( KEY_VersionNewMinor, DEFAULT_VersionNewMinor );
	m_VersionInfo.client.buildNew = cfg.GetInteger( KEY_VersionNewBuild, DEFAULT_VersionNewBuild );

	m_strNewClientDownloadURL = cfg.GetString( KEY_DownloadURL, "" );

	m_VersionInfo.server.upTime = time(NULL);

	m_config.requiredVersion = CLIENT_VERCODE( m_VersionInfo.client.majorRequired, m_VersionInfo.client.minorRequired, m_VersionInfo.client.buildRequired );

	PLOG( m_pLogger, Info, 
		PString( PString::Printf, "Server started as: %s, initial status: %s.",
		(const char *) TypeToString(m_config.nServerType),
		(const char *) StatusToString(m_status.nServerStatus) ) );

	if( m_config.write_hearbeat_file ) {
		PString folder( "logs" );
		folder += PDirectory::IsSeparator('\\') ? "\\" : "/";
		PFileLog::TouchFile( folder +  TypeToString( m_config.nServerType ) );
		PFileLog::TouchFile( folder +  StatusToString( m_status.nServerStatus ) );
	}

	m_strCallbackTrustIp = cfg.GetString( KEY_Security, KEY_CallbackTrustIp, "127.0.0.1" );	// ip/mask, ip/mask ...

	// load global system config from database
	m_bSystemConfigLoadedFromDB = LoadSystemConfigFromDB();

	// load ip2country info
	MS_TIME msStart, msEnd;
	Ip2CountryUtility i2c;
	int ret = 0;

	const char csvFile[] = "ip2country.csv";
	const char i2cFile[] = "ip2country.i2c";
	const char countryFile[] = "country.txt";

	if( PFile::Exists( i2cFile ) && PFile::Exists( countryFile ) ) {
		msStart = BBQMsgTerminal::GetHostUpTimeInMs();
		ret = m_Ip2CountryUtility.ImportBinI2C( i2cFile );
		ret = m_Ip2CountryUtility.ImportCountryInfo( countryFile );
		msEnd =  BBQMsgTerminal::GetHostUpTimeInMs();
		PLOG( m_pLogger, Info,
			PString(PString::Printf, "Loaded ip-to-country info from binary cache file. %d records, %d countries, time used: %u ms.", 
			m_Ip2CountryUtility.GetIp2CountryRecordCount(), m_Ip2CountryUtility.GetCountryRecordCount(), (unsigned int)(msEnd - msStart) ) );
	} else if ( PFile::Exists( csvFile ) ) {
		msStart = BBQMsgTerminal::GetHostUpTimeInMs();
		ret = m_Ip2CountryUtility.ImportCSV( csvFile );
		msEnd =  BBQMsgTerminal::GetHostUpTimeInMs();

		if( ret == Ip2CountryUtility::Okay ) {
			PLOG( m_pLogger, Info,
				PString(PString::Printf, "Loaded ip-to-country info from CSV file. %d records, %d countries, time used: %u ms.", 
				m_Ip2CountryUtility.GetIp2CountryRecordCount(), m_Ip2CountryUtility.GetCountryRecordCount(), (unsigned int)(msEnd - msStart) ) );

			if( ! PFile::Exists( i2cFile ) ) {
				msStart = BBQMsgTerminal::GetHostUpTimeInMs();
				ret = m_Ip2CountryUtility.ExportBinI2C( i2cFile );
				msEnd =  BBQMsgTerminal::GetHostUpTimeInMs();
				PLOG( m_pLogger, Info, PString(PString::Printf, "Ip2Country binary cache file updated, time used: %u ms.", (unsigned int)(msEnd - msStart) ) );
			}
			
			if( ! PFile::Exists( countryFile ) ) {
				msStart = BBQMsgTerminal::GetHostUpTimeInMs();
				ret = m_Ip2CountryUtility.ExportCountryInfo( countryFile );
				msEnd =  BBQMsgTerminal::GetHostUpTimeInMs();
				PLOG( m_pLogger, Info, PString(PString::Printf, "Country info cache file updated, time used: %u ms.", (unsigned int)(msEnd - msStart) ) );
			}
		}
		if( ret != Ip2CountryUtility::Okay ) {
			PLOG( m_pLogger, Error, "Failed to load ip-to-country data." );
		}
	} else {
		PLOG( m_pLogger, Info, "ip-to-country data file 'ip2country.csv' not found, skip." );
	}

	LoadMeetingInfo();

	m_bInited = true;

	m_status.tUpTime = time(NULL);

	new InterlinkThread( * this );

	new UplinkThread( * this );

	return true;
}

void BBQServer::EnableSerialKeyCheck( bool bEnable )
{
	m_config.disable_serial_key_check = ! bEnable;

	PConfig cfg( KEY_BBQServer );
	cfg.SetBoolean( KEY_DisableSerialKeyCheck, m_config.disable_serial_key_check );
}

bool BBQServer::IsSerialKeyCheckEnabled( void )
{
	return (! m_config.disable_serial_key_check);
}

bool BBQServer::LoadLayeredLinkData( SFIDRecord * pRec ) // load parent id list and child id list from DB
{
	if( ! pRec ) return false;

	VFONID vid;
	vid.serverid = 0;
	vid.userid = pRec->sessionInfo.id;

	uint_list parentIds, childIds;

	//CATCH_DB_FAIL(
		if( ! m_pDB->ReadRelationIdList( vid, BBQDatabase::PARENT_ID, & parentIds ) ) return false;
		if( ! m_pDB->ReadRelationIdList( vid, BBQDatabase::CHILD_ID, & childIds ) ) return false;

		if( ! pRec->pParentIdList ) pRec->pParentIdList = new UserIdList;
		if( pRec->pParentIdList ) {
			pRec->pParentIdList->RemoveAll();
			for( uint_list::iterator it = parentIds.begin(), et = parentIds.end(); it != et; it ++ ) {
				if( * it ) pRec->pParentIdList->Add( * it );
			}
		}

		if( ! pRec->pChildIdList ) pRec->pChildIdList = new UserIdList;
		if( pRec->pChildIdList ) {
			pRec->pChildIdList->RemoveAll();
			for( uint_list::iterator it = childIds.begin(), et = childIds.end(); it != et; it ++ ) {
				if( * it ) pRec->pChildIdList->Add( * it );
			}
		}
		return true;
	//);

	return false;
}

bool BBQServer::SaveData( SFIDRecord * pRec, SaveEvent event, BBQUserValidFlags * pFlags )
{
	BBQUserFullRecord full;

	memset( & full, 0, sizeof(full) );

	full.uid = pRec->sessionInfo.id;

	if( pRec->pServiceInfo ) {
		full.service = * (pRec->pServiceInfo);
	}
	if( pRec->pUserInfo ) {
		full.userInfo = * (pRec->pUserInfo);
	}
	if( pRec->pCallForwardInfo ) {
		int n = min( pRec->pCallForwardInfo->count, MAX_CALLFWD_COUNT );
		//for( int i=0; i<n; i++ ) {
		//	full.callfwd[i] = pRec->pCallForwardInfo->callfwd[i];
		//}
		memcpy( & full.callfwd[0], & pRec->pCallForwardInfo->callfwd[0], sizeof(BBQCallForwardRecord) * n );
	}

	if( pRec->pVoipItems ) {
		memcpy( & full.voipinfo[0], & pRec->pVoipItems->items[0], sizeof(BBQVoIPInfo) * MAX_VOIP_COUNT );
	}
	if( pRec->pXMLEXT ) {
		strncpy( full.xmlext, pRec->pXMLEXT, XMLEXT_SIZE );
		full.xmlext[ XMLEXT_SIZE-1 ] = '\0';
	}

	if( pRec->pFriendList ) {
		pRec->pFriendList->ExportTo( full.friends, USERDATA_LIST_SIZE );
	}
	if( pRec->pBlockList ) {
		pRec->pBlockList->ExportTo( full.blocks, USERDATA_LIST_SIZE );
	}
	if( pRec->pBuddyAddingList ) {
		pRec->pBuddyAddingList->ExportTo( full.adding, USERDATA_LIST_SIZE );
	}
	if( pRec->pWaitAuthList ) {
		pRec->pWaitAuthList->ExportTo( full.waitauth, USERDATA_LIST_SIZE );
	}

	full.service.type = pRec->serviceType;
	//full.service.product_serial = pRec->productSerial;
	//memcpy( full.service.hardwareID, pRec->regKey, USERDATA_NAME_SIZE );

	full.service.provider = pRec->providerID;
	full.service.start_time = pRec->serviceStart;
	full.service.expire_time = pRec->serviceExpire;
	full.service.flags = pRec->flags;
	full.service.extFlags = pRec->extFlags;

	full.service.login_time = pRec->time;
	full.service.ip = pRec->sockChannel.peer.ip;
	full.service.statusCode = pRec->userStatus;

	full.param = pRec->techInfo;


	if( pFlags ) full.valid_flags = * pFlags;

	full.valid_flags.login = (event==SaveOnLogin) ? 1 : 0;
	full.valid_flags.logout = (event == SaveOnLogout) ? 1 : 0;

	CATCH_DB_FAIL(
		if( m_pDB->Write( & full ) ) {
			pRec->is_dirty = 0;
			return true;
		} 		
	);

	return false;
}

bool BBQServer::SaveData( bool bForceAll )
{
	if( ! m_pDB ) return false;

	ReadLock lock( m_mutexRecord );

	// save one by one
	int i = 0;
	for( RecordMap::iterator Iter = m_mapRecord.begin(), eIter = m_mapRecord.end(); Iter != eIter; Iter++ ) {
		SFIDRecord * pRec = Iter->second;

		if( bForceAll || pRec->is_dirty ) {
			if( SaveData( pRec, SaveOnChange ) ) i++;
		}
	}

	PTRACE( 1, i << " of " << m_mapRecord.size() << " records saved." );

	m_bDataChanged = false;

	return true;
}

bool BBQServer::GetRecords( SFIDRecord* & pRecArray, int & nGet, int & nTotal, uint32 from, uint32 nMaxCount, bool bOnlineOnly )
{
	ReadLock lock( m_mutexRecord );

	nTotal = bOnlineOnly ? m_status.nOnlineUsers : (int) m_mapRecord.size();
	if( from >= nTotal ) return false;

	if( nMaxCount == 0 ) nMaxCount = nTotal - from;
	if( ! nMaxCount ) return false;
	pRecArray = new SFIDRecord[ nMaxCount ];
	if( ! pRecArray ) return false;
	memset( pRecArray, 0, sizeof(SFIDRecord) * nMaxCount );

	nGet = 0;
	SFIDRecord * p = pRecArray;
	time_t now = time(NULL);
	uint32 i=0;
	for( RecordMap::iterator Iter = m_mapRecord.begin(), eIter = m_mapRecord.end(); Iter != eIter; Iter ++ ) {
		SFIDRecord * pRec = Iter->second;

		if( bOnlineOnly ) {
			bool bOnline = ( (pRec->sessionInfo.cookie != 0) && (pRec->time + m_config.heartbeat_timeout > now) );
			if( ! bOnline ) continue;
		}
		i ++;

		if( (from!=0) && (i<=from) ) continue;
		if( nGet >= nMaxCount ) break;

		memcpy( p, pRec, sizeof(SFIDRecord) );
		nGet ++;
		p ++;
	}

	return true;
}

SFIDRecord * BBQServer::SafeFindRecordBySerial( uint32 nProductSerial )
{
	ReadLock lock( m_mutexRecord );

	return FindRecordBySerial( nProductSerial );
}

SFIDRecord * BBQServer::SafeFindRecordByID( uint32 nCallerID )
{
	ReadLock lock( m_mutexRecord );

	return FindRecordByID( nCallerID );
}

// input request, get answer
bool BBQServer::OnMsgClientSayHello( const SIM_REQUEST * req )
{
	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_HELLO, sizeof(SIMD_SC_HELLO), req );

	SIMD_SC_HELLO * pA = (SIMD_SC_HELLO *) & reply.msg.simData;

	pA->versionCode = SIM_THIS_VERCODE;

	pA->clientIp = req->channel.peer.ip;
	pA->clientPort = req->channel.peer.port;

	pA->anotherServerIp = m_config.anotherServerAddress.ip;
	pA->anotherServerPort = m_config.anotherServerAddress.port;

	SendMessage( & reply );

	return true;
}

bool BBQServer::OnMsgClientTestFirewall( const SIM_REQUEST * req )
{
	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_TESTFIREWALL, sizeof(SIMD_SC_HELLO), req );
	reply.channel.local.port = m_nMaxUdpPort;

	SIMD_SC_HELLO * pA = (SIMD_SC_HELLO *) & reply.msg.simData;

	pA->versionCode = SIM_THIS_VERCODE;
	pA->clientIp = req->channel.peer.ip;
	pA->clientPort = req->channel.peer.port;

	SendMessage( & reply );

	return true;
}

bool BBQServer::RemoveProxyRecord( uint32 nId )
{
	WriteLock lock( m_mutexRecord );

	ProxyMap::iterator iter = m_mapProxy.find( nId );
	if( iter != m_mapProxy.end() ) {
		BBQProxyRecord * pProxy = iter->second;

		//m_mapProxy.erase( iter );
		STL_ERASE( m_mapProxy, ProxyMap::iterator, iter );

		if( pProxy->pTagString ) free( pProxy->pTagString );
		delete pProxy;


		m_status.nProxyCount = m_mapProxy.size();

		return true;
	}

	return false;
}

BBQProxyRecord * BBQServer::FindProxyById( uint32 nId )
{
	BBQProxyRecord * pRec = NULL;

	if( m_mapProxy.size() > 0 ) {
		ProxyMap::iterator iter = m_mapProxy.find( nId );
		if( iter != m_mapProxy.end() ) { // found in memory
			pRec = iter->second;
		}
	} 

	return pRec;
}

BBQProxyRecord * BBQServer::FindPureProxyByIp( uint32 ip )
{
	BBQProxyRecord * pRec = NULL;

	if( m_mapPureProxy.size() > 0 ) {
		ProxyMap::iterator iter = m_mapPureProxy.find( ip );
		if( iter != m_mapPureProxy.end() ) { // found in memory
			//uint32 n = iter->first;
			pRec = iter->second;
		}
	} 

	return pRec;
}

//
// here is a policy to select the best proxy for new requested channel 
//
//#define		BBQPROXY_POLICY
//#define		BBQPROXY_POLICY_ALLOW					// only allow some member user to use proxy
//#define		BBQPROXY_POLICY_SECURITY				// meet user's need for safe proxy

BBQProxyRecord * BBQServer::PickProxy( uint32 nFromId, uint32 nToId )
{
#ifdef BBQPROXY_POLICY_SECURITY
	BBQProxyRecord::Flags flags;
	flags.configured = 1;
	flags.certified = 1;
	flags.volunteer = 1;
	flags.client = 1;
#endif

#ifdef BBQPROXY_POLICY

	SFIDRecord * pRecFrom = FindRecordByID( nFromId );
	SFIDRecord * pRecTo = FindRecordByID( nToId );
	if( pRecFrom && pRecTo ) {

#ifdef BBQPROXY_POLICY_ALLOW
		// if both are not allow to use proxy, then deny to use
		if( (! pRecFrom->flags.allow_proxy) && (! pRecTo->flags.allow_proxy) ) return NULL;
#endif

#ifdef BBQPROXY_POLICY_SECURITY
		if( pRecFrom->pUserInfo && pRecTo->pUserInfo ) {
			if( pRecFrom->pUserInfo->security.only_use_safe_proxy || pRecTo->pUserInfo->security.only_use_safe_proxy ) {
				flags.configured = 1;
				flags.certified = 1;
				flags.volunteer = 0;
				flags.client = 0;
			}
		}
#endif
	}
#endif

	int nBestBandwidth = 0;
	BBQProxyRecord * pRecBestBandwidth = NULL;

	int nBestPercent = 0;
	int nBandwidthInBestPercent = 0;
	BBQProxyRecord * pRecBestPercent = NULL;

	time_t now = time(NULL);

	ProxyMap::iterator iter, e_iter;

	for( iter = m_mapProxy.begin(), e_iter = m_mapProxy.end(); iter != e_iter; iter ++ ) {
		BBQProxyRecord * p = iter->second;
		if( (p->time + m_config.heartbeat_timeout > now) // is online
			&& ( ! p->pTagString )
			&& (p->proxyMax >= p->proxySlot) && (p->proxySlot > 0) // has empty slot
#ifdef BBQPROXY_POLICY_SECURITY
			&& (p->flags.value & flags.value)
#endif
			) {

			int nBandwidth = 0, nPercent = 0;

			if( p->proxyBandwidthTotal ) {
				nBandwidth = p->proxyBandwidthTotal - p->proxyBandwidthUsed;
				nPercent = 100 * nBandwidth / p->proxyBandwidthTotal;
			} else {
				nBandwidth = 0;
				nPercent = 0;
			}

			if( nBandwidth >= nBestBandwidth ) {
				nBestBandwidth = nBandwidth;
				pRecBestBandwidth = p;
			}

			if( nPercent >= nBestPercent ) {
				nBestPercent = nPercent;
				nBandwidthInBestPercent = nBandwidth;
				pRecBestPercent = p;
			}
		}
	}

	for( iter = m_mapPureProxy.begin(), e_iter = m_mapPureProxy.end(); iter != e_iter; iter ++ ) {
		BBQProxyRecord * p = iter->second;
		if( (p->time + m_config.heartbeat_timeout > now) // is online
			&& ( ! p->pTagString )
			&& (p->proxyMax >= p->proxySlot) && (p->proxySlot > 0) // has empty slot
#ifdef BBQPROXY_POLICY_SECURITY
			&& (p->flags.value & flags.value)
#endif
			) {

			int nBandwidth = 0, nPercent = 0;

			if( p->proxyBandwidthTotal ) {
				nBandwidth = p->proxyBandwidthTotal - p->proxyBandwidthUsed;
				nPercent = 100 * nBandwidth / p->proxyBandwidthTotal;
			} else {
				nBandwidth = 0;
				nPercent = 0;
			}

			if( nBandwidth >= nBestBandwidth ) {
				nBestBandwidth = nBandwidth;
				pRecBestBandwidth = p;
			}

			if( nPercent >= nBestPercent ) {
				nBestPercent = nPercent;
				nBandwidthInBestPercent = nBandwidth;
				pRecBestPercent = p;
			}
		}
	}

	// select proxy with the best bandwidth
	return pRecBestBandwidth;
}

BBQProxyRecord * BBQServer::PickProxyByTag( const char * pFromTag, const char * pToTag )
{
	//PStringArray strs1 = PString( pFromTag ).Tokenise(",; ");
	//PStringArray strs2 = PString( pToTag ).Tokenise(",; ");

	// TODO: we will support multiple token in future
	// but currently, we support only single tag
	if( 0 != strcmp( pFromTag, pToTag ) ) return NULL;

	const char * pTag = pFromTag;
	
	time_t now = time(NULL);

	BBQProxyRecord * pRecBest = NULL;
	uint32 usageBest = 100;

	ProxyMap::iterator iter, e_iter;

	for( iter = m_mapProxy.begin(), e_iter = m_mapProxy.end(); iter != e_iter; iter ++ ) {
		BBQProxyRecord * p = iter->second;
		if( (p->time + m_config.heartbeat_timeout > now) // is online
			&& (p->proxyMax >= p->proxySlot) && (p->proxySlot > 0) // has empty slot
			&& p->pTagString // tagged only
#ifdef BBQPROXY_POLICY_SECURITY
			&& (p->flags.value & flags.value)
#endif
			) {

			// skip those proxy with too heavy loading
			uint32 usage = 100 * p->proxyBandwidthUsed / p->proxyBandwidthTotal;
			if( usage > 90 ) continue;

			if( 0 == strcmp( p->pTagString, pTag ) ) { // must match tag
				if( usage <= usageBest ) { // pick the one with less loading
					usageBest = usage;
					pRecBest = p;
				}
			}
		}
	}

	for( iter = m_mapPureProxy.begin(), e_iter = m_mapPureProxy.end(); iter != e_iter; iter ++ ) {
		BBQProxyRecord * p = iter->second;
		if( (p->time + m_config.heartbeat_timeout > now) // is online
			&& (p->proxyMax >= p->proxySlot) && (p->proxySlot > 0) // has empty slot
			&& p->pTagString // tagged only
#ifdef BBQPROXY_POLICY_SECURITY
			&& (p->flags.value & flags.value)
#endif
			) {

			// skip those proxy with too heavy loading
			uint32 usage = 100 * p->proxyBandwidthUsed / p->proxyBandwidthTotal;
			if( usage > 90 ) continue;

			if( 0 == strcmp( p->pTagString, pTag ) ) { // must match tag
				if( usage <= usageBest ) { // pick the one with less loading
					usageBest = usage;
					pRecBest = p;
				}
			}
		}
	}

	return pRecBest;
}

BBQProxyRecord * BBQServer::PickProxyByIp2Country( uint32 ipFrom, uint32 ipTo )
{
	time_t now = time(NULL);

	uint32 countryFrom = m_Ip2CountryUtility.Ip2Country( ipFrom );
	uint32 countryTo = m_Ip2CountryUtility.Ip2Country( ipTo );

	BBQProxyRecord * pRecBest = NULL;
	uint32 paramBest = 0;
	uint32 usageBest = 100;

	ProxyMap::iterator iter, e_iter;
	for( iter = m_mapProxy.begin(), e_iter = m_mapProxy.end(); iter != e_iter; iter ++ ) {
		BBQProxyRecord * p = iter->second;
		if( (p->time + m_config.heartbeat_timeout > now) // is online
			&& ( ! p->pTagString )
			&& (p->proxyMax >= p->proxySlot) && (p->proxySlot > 0) // has empty slot
#ifdef BBQPROXY_POLICY_SECURITY
			&& (p->flags.value & flags.value)
#endif
			) {

			// skip those proxy with too heavy loading
			uint32 usage = 100 * p->proxyBandwidthUsed / p->proxyBandwidthTotal;
			if( usage > 90 ) continue;

			uint32 param = 0;
			if( p->contryCode == countryFrom ) param += 10;
			if( p->contryCode == countryTo ) param += 20;

			if( paramBest < param ) {
				paramBest = param;
				usageBest = usage;
				pRecBest = p;
			} else if ( paramBest == param ) { // same country priority
				if( usage <= usageBest ) {
					paramBest = param;
					usageBest = usage;
					pRecBest = p;
				}
			}

		}
	}

	for( iter = m_mapPureProxy.begin(), e_iter = m_mapPureProxy.end(); iter != e_iter; iter ++ ) {
		BBQProxyRecord * p = iter->second;
		if( (p->time + m_config.heartbeat_timeout > now) // is online
			&& ( ! p->pTagString )
			&& (p->proxyMax >= p->proxySlot) && (p->proxySlot > 0) // has empty slot
#ifdef BBQPROXY_POLICY_SECURITY
			&& (p->flags.value & flags.value)
#endif
			) {

			// skip those proxy with too heavy loading
			uint32 usage = 100 * p->proxyBandwidthUsed / p->proxyBandwidthTotal;
			if( usage > 90 ) continue;

			uint32 param = 0;
			if( p->contryCode == countryFrom ) param += 10;
			if( p->contryCode == countryTo ) param += 20;

			if( paramBest < param ) {
				paramBest = param;
				usageBest = usage;
				pRecBest = p;
			} else if ( paramBest == param ) { // same country priority
				if( usage < usageBest ) {
					paramBest = param;
					usageBest = usage;
					pRecBest = p;
				}
			}

		}
	}

	return pRecBest;
}

SFIDRecord * BBQServer::FindRecordBySerial( uint32 nSerial )
{
	//if( m_config.index_serial ) { // use index to locate the record quickly
	//	PVOID pValue = NULL;
	//	if( m_indexRecordSerial.Get(nSerial, pValue) ) {
	//		SFIDRecord * p = (SFIDRecord *) pValue;
	//		if( p->productSerial == nSerial ) return p;
	//	}
	//} else {
	//	for( RecordIter Iter = m_listRecord.begin(), eIter = m_listRecord.end(); Iter != eIter; Iter ++ ) {
	//		SFIDRecord * p = * Iter;
	//		if( p->productSerial == nSerial ) return p;
	//	}
	//}

	return NULL;
}

bool BBQServer::UpdateRecordCache( SFIDRecord * pRec, BBQUserFullRecord * full )
{
	if( pRec->sessionInfo.id != full->uid ) return false;

	memcpy( pRec->password, full->userInfo.security.oldpass, USERDATA_NAME_SIZE );

	m_dictHardwareId[ PString(full->service.hardwareID) ] = pRec->sessionInfo.id;

	pRec->providerID = full->service.provider;

	pRec->serviceType = full->service.type;
	pRec->serviceStart = full->service.start_time;
	pRec->serviceExpire = full->service.expire_time;

	// we only do this when the ip in memory is zero, that means the user has not login,
	// or else, if the user login already, we must not reset the port to zero.
	if( pRec->sockChannel.peer.ip == 0 ) {
		pRec->sockChannel.peer.ip = full->service.ip;
		pRec->sockChannel.peer.port = 0;

		pRec->netServer = full->service.serverAddr;

		pRec->userStatus = full->service.statusCode;
		pRec->login_time = full->service.login_time;

		pRec->time = 0;

		pRec->techInfo = full->param;
	}

	* (pRec->pServiceInfo) = full->service;
	* (pRec->pUserInfo) = full->userInfo;

	pRec->pCallForwardInfo->count = 0;
	for( int i=0; i<MAX_CALLFWD_COUNT; i++ ) {
		if( full->callfwd[i].fwd_condition ) {
			pRec->pCallForwardInfo->callfwd[ pRec->pCallForwardInfo->count ] = full->callfwd[i];
			pRec->pCallForwardInfo->count ++;
			if( pRec->pCallForwardInfo->count >= CALLFWD_COUNT ) break;
		}
	}

	if( pRec->pVoipItems ) {
		memcpy( & pRec->pVoipItems->items[0], & full->voipinfo[0], sizeof(BBQVoIPInfo) * MAX_VOIP_COUNT );
	}
	if( pRec->pXMLEXT ) {
		free( pRec->pXMLEXT );
		pRec->pXMLEXT = NULL;
	}
	pRec->pXMLEXT = strdup( full->xmlext );

	pRec->pFriendList->ImportFrom( full->friends, USERDATA_LIST_SIZE );
	pRec->pBlockList->ImportFrom( full->blocks, USERDATA_LIST_SIZE );
	pRec->pBuddyAddingList->ImportFrom( full->adding, USERDATA_LIST_SIZE );
	pRec->pWaitAuthList->ImportFrom( full->waitauth, USERDATA_LIST_SIZE );

	pRec->is_dirty = 0;

	pRec->flags = full->service.flags;

	pRec->extFlags = full->service.extFlags;

	// if no login is set, immediately kick the user by clear its cookie
	if( pRec->flags.no_login ) {

		if( pRec->sessionInfo.cookie ) {
			// clear its online cookie
			pRec->sessionInfo.cookie = 0;

			// close the connection
			int fdTcp = pRec->sockChannel.socket;
			if( fdTcp ) closesocket( fdTcp );
		}
	}

	pRec->load_timestamp = GetHostUpTimeInMs();

	return true;
}

SFIDRecord * BBQServer::InsertRecordIntoMemoryCache( BBQUserFullRecord * full )
{
	WriteLock safewrite( m_mutexRecord );

	SFIDRecord * pRec = NULL;

	RecordMap::iterator iter = m_mapRecord.find( full->uid );
	if( iter != m_mapRecord.end() ) {
		pRec = iter->second;
	} else {
		pRec = new SFIDRecord;
		memset( pRec, 0, sizeof(SFIDRecord) );

		pRec->sessionInfo.id = full->uid;

		pRec->pServiceInfo = new BBQUserServiceInfo;
		memset( pRec->pServiceInfo, 0, sizeof(BBQUserServiceInfo) );

		pRec->pUserInfo = new BBQUserInfo;
		memset( pRec->pUserInfo, 0, sizeof(BBQUserInfo) );

		pRec->pCallForwardInfo = new BBQCallForwardInfo;
		memset( pRec->pCallForwardInfo, 0, sizeof(BBQCallForwardInfo) );

		pRec->pVoipItems = new BBQVoipItems;
		memset( pRec->pVoipItems, 0, sizeof(BBQVoipItems) );

		pRec->pFriendList = new UserIdList();
		pRec->pBlockList = new UserIdList();
		pRec->pBuddyAddingList = new UserIdList();
		pRec->pWaitAuthList = new UserIdList();

		pRec->pBuddyStatus = new UserStatusMap;

		m_mapRecord[ pRec->sessionInfo.id ] = pRec;

		pRec->pImList = new offlineim_list;
	}

	UpdateRecordCache( pRec, full );

	return pRec;
}

bool BBQServer::UpdateRecordCacheFromDB( SFIDRecord * pRec )
{
	if( pRec && m_pDB ) {
		BBQUserFullRecord full;
		memset( & full, 0, sizeof(full) );
		full.uid = pRec->sessionInfo.id;

		CATCH_DB_FAIL(
			if( m_pDB->Read( & full ) 
				&& UpdateRecordCache( pRec, & full ) ) {

				LoadLayeredLinkData( pRec );
				return true;
			}
		);
	}

	return false;
}

SFIDRecord * BBQServer::FindRecordByID( uint32 nID, bool bForceReload )
{
	SFIDRecord * pRec = NULL;

	if( ! bForceReload ) { // if not force reload, find it in memory cache first
		RecordMap::iterator iter = m_mapRecord.find( nID );
		if( iter != m_mapRecord.end() ) { // found in memory
			pRec = iter->second;

		}
	}
	
	if( ! pRec ) { //, we try loading it from database
		if( m_pDB ) {
			BBQUserFullRecord full;
			memset( & full, 0, sizeof(full) );
			full.uid = nID;

			CATCH_DB_FAIL(
				if( m_pDB->Read( & full ) ) {
					pRec = InsertRecordIntoMemoryCache( & full );

					// if record loaded, then also load link info
					if( pRec ) LoadLayeredLinkData( pRec );

					return pRec;
				}
			);

			return NULL;
		}
	}

	return pRec;
}

SFIDRecord * BBQServer::FindRecordByHWID( const char * pHWID )
{
	SFIDRecord * pRec = NULL;

	KeywordIndex::iterator keyiter = m_dictHardwareId.find( PString(pHWID) );
	if( keyiter != m_dictHardwareId.end() ) { // found in memory
		uint32 uid = keyiter->second;

		RecordMap::iterator iter = m_mapRecord.find( uid );
		if( iter != m_mapRecord.end() ) {
			pRec = iter->second;
		}

	}
	
	if( ! pRec ) { //, we try loading it from database
		if( m_pDB ) {
			BBQUserFullRecord full;
			memset( & full, 0, sizeof(full) );

			CATCH_DB_FAIL(
				if( m_pDB->ReadByKey( & full, pHWID, BBQDatabase::BY_HARDWAREID ) ) {
					pRec = InsertRecordIntoMemoryCache( & full );

					// if record loaded, then also load link info
					if( pRec ) LoadLayeredLinkData( pRec );
				}
			);

		}
	}
	return pRec;
}

SFIDRecord * BBQServer::CreateRecordByHWID( const char * pHWID )
{
	SFIDRecord * pRec = NULL;

	if( ! pRec ) { //, we try loading it from database
		if( m_pDB ) {
			BBQUserFullRecord full;
			memset( & full, 0, sizeof(BBQUserFullRecord) );

			full.service.type = SERVICE_FIXEDPCTRIAL;
			strcpy( full.service.hardwareID, pHWID );
			full.service.start_time = time(NULL);
			full.service.expire_time = full.service.start_time + m_config.subscribe_trial_time * 3600 * 24;

			CATCH_DB_FAIL(
				full.uid = m_pDB->CreateUser( & full );
				if( full.uid != 0 ) {
					pRec = InsertRecordIntoMemoryCache( & full );

					// new create node, no link yet.
				} else {
					// db error ?
				}
			);
		}
	}

	return pRec;
}

bool BBQServer::UserOffline( SFIDRecord * pRec, bool bDrop )
{
	if( m_config.write_useraction_log ) {
		PString strMsg( PString::Printf, "%d(%u) %s from %s.", 
      pRec->sessionInfo.id, 
      pRec->sessionInfo.cookie,
			bDrop ? "drop offline" : "logout",
			(const char *)ToString(pRec->sockChannel.peer.ip) );
		m_UserActionLog.Output( PLog::Info, strMsg );
	}
  uint32 uid = pRec->sessionInfo.id ;
	// if this client is a proxy, remove it from proxy list
	RemoveProxyRecord( pRec->sessionInfo.id );

	if( pRec->sockChannel.socket ) { 
		// this is a trick for TCP connection,
		// just ask the low level msg terminal to disconnect TCP 
		SIM_REQUEST kick;
		SIM_REQINITCHAN( kick, pRec->sockChannel, SIM_NONE, 0 );
		SendMessage( & kick );

		pRec->sockChannel.socket = 0;
	}else
  {
    // test the udp heart
		SIM_REQUEST kick;
		SIM_REQINITCHAN( kick, pRec->sockChannel, SIM_NONE, 0 );
		SendMessage( & kick );
  }

	if( pRec->pSessionList ) { 
		// if the user has vfon sessions, end them, and write to log
		DestroyVfonSessionList( pRec->pSessionList );
		pRec->pSessionList = NULL;
	}

	// when user logout, clear the buddy status info
	if( pRec->pBuddyStatus ) {
		pRec->pBuddyStatus->clear();
	}

	if( pRec->pImList ) {
		while( pRec->pImList->size() > 0 ) {
			BBQOfflineIMMessage * im = pRec->pImList->front();
			pRec->pImList->pop_front();
			if( im ) {
				BBQOfflineIMMessageDELETE( im );
			}
		}
	}

	pRec->sessionInfo.cookie = 0;
	pRec->actStatus = CLIENT_OFFLINE;

	// clear AES key
	if( pRec->pAESKeyBytes ) {
		free( pRec->pAESKeyBytes );
		pRec->pAESKeyBytes = NULL;
	}
	pRec->nAESKeyBytes = 0;

	BBQUserValidFlags flags;
	memset( & flags, 0, sizeof(flags) );
	flags.partial_valid = 1;
	flags.service = 1;
	flags.param = 1;
	SaveData( pRec, SaveOnLogout, & flags );

	if(m_status.nOnlineUsers > 0) m_status.nOnlineUsers --;

	// if it is MCU, remove from the list
	MCUMap::iterator mcu_iter = m_mapMCU.find( pRec->sessionInfo.id );
	if( mcu_iter != m_mapMCU.end() ) {
		MCURecord * pMCU = mcu_iter->second;
		if( pMCU ) delete pMCU;

		//mcu_iter = m_mapMCU.erase( mcu_iter );
		STL_ERASE( m_mapMCU, MCUMap::iterator, mcu_iter );

		m_status.nMCUCount = m_mapMCU.size();
	}

	// send report to central server if connected
	if( m_status.nInterlinkConnected ) {
		SIM_REQUEST report;
		for( int i=0; i<2; i++ ) {
			if( m_chanInterlink[i].peer.ip != 0 ) {

				SIM_REQINITCHAN( report, m_chanInterlink[i], SIM_SX_UPDATESWITCHINFO, sizeof(SIMD_SX_UPDATESWITCHINFO) );
				SIMD_SX_UPDATESWITCHINFO * pU = (SIMD_SX_UPDATESWITCHINFO *) & report.msg.simData;

				memset( pU, 0, sizeof(*pU) );

				pU->id = pRec->sessionInfo.id;
				pU->info.logout = bDrop ? 0 : 1;
				pU->info.dropoffline = bDrop ? 1 : 0;
				pU->info.is_tcp = (pRec->sockChannel.socket != 0) ? 1 : 0;
				pU->info.firewall_type = pRec->techInfo.firewall;

				pU->info.act_status = pRec->pServiceInfo->statusCode & 0xff;
				pU->info.set_status = (pRec->pServiceInfo->statusCode >> 8) & 0xff;

				//pU->info.addr.ip = 0;
				pU->info.addr.ip = m_config.thisServerAddress.ip;
				pU->info.addr.tcpPort = m_nDefaultTcpPort;
				pU->info.addr.udpPort = m_nDefaultUdpPort;

				pU->info.channel = pRec->sockChannel;

				SendMessage( & report );
			}
		}
	}

	// send message to buddy to notify that i am offline
	NotifyBuddyStatus( pRec );
  YTUnRegisterSIPUser(uid);
	return true;
}

bool BBQServer::OnMsgClientLogout( const SIM_REQUEST * req )
{
	const SIMD_CS_LOGOUT * pQ = (const SIMD_CS_LOGOUT *) & req->msg.simData;

	if( pQ->sessionInfo.id != 0 ) {
		WriteLock lock( m_mutexRecord );

		SFIDRecord * pRec = FindRecordByID( pQ->sessionInfo.id );
		if( pRec && (pRec->sessionInfo.cookie == pQ->sessionInfo.cookie) ) {
			
			UserOffline( pRec, false );
		}
	} else { // session.id == 0, it is a static proxy ?

		if( (req->channel.peer.ip == pQ->sessionInfo.cookie) ) { // yes, it is

			WriteLock lock( m_mutexRecord );
	
			ProxyMap::iterator iter = m_mapPureProxy.find( req->channel.peer.ip );
			if( iter != m_mapPureProxy.end() ) {
				BBQProxyRecord * pProxy = iter->second;

				//iter = m_mapPureProxy.erase( iter );
				STL_ERASE( m_mapPureProxy, ProxyMap::iterator, iter );

				if( pProxy->pTagString ) free( pProxy->pTagString );
				delete pProxy;

				m_status.nPureProxyCount = m_mapPureProxy.size();
			}
		}
	}

	return true;
}

bool BBQServer::OnMsgClientChangeStatus( const SIM_REQUEST * req )
{
	const SIMD_CS_CHANGESTATUS * pQ = (const SIMD_CS_CHANGESTATUS *) & req->msg.simData;

	WriteLock lock( m_mutexRecord );

	SFIDRecord * pRec = FindRecordByID( pQ->sessionInfo.id );
	if( pRec ) {
		if( pRec && (pRec->sessionInfo.cookie == pQ->sessionInfo.cookie) ) {
			// update login channel
			pRec->sockChannel = req->channel;

			if( (pRec->userStatus & 0xff00) != (pQ->statusCode & 0xff00) ) {
				pRec->is_dirty = 1;

				pRec->userStatus = pQ->statusCode;

				// after set status, write back to database immediately
				BBQUserValidFlags flags;
				memset( & flags, 0, sizeof(flags) );
				flags.partial_valid = 1;
				flags.service = 1;
				SaveData( pRec, SaveOnLogin, & flags );
			}
		}

		pRec->userStatus = pQ->statusCode;

		// if someone else already subscribe my notifying msg, now notify them
		NotifyBuddyStatus( pRec );
	}
	//if( m_indexRecordId.Get( pQ->sessionInfo.id, pValue ) && ((pRec = (SFIDRecord *) pValue) != NULL) &&
	//	(pRec->sessionInfo.id == pQ->sessionInfo.id) && (pRec->sessionInfo.cookie == pQ->sessionInfo.cookie) ) {
	//	pRec->statusCode = pQ->statusCode;
	//}

	return true;
}

bool BBQServer::OnMsgClientHeartbeat( const SIM_REQUEST * req )
{
	const SIMD_CS_HEARTBEAT * pQ = (const SIMD_CS_HEARTBEAT *) & req->msg.simData;

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_HEARTBEAT, 0, req );

	UserStatusMap * pUSM = NULL;

	PBytePack packedData, packedMsg;
	uint32 statusCode = SIMS_OK, nFriends = 0;

	if( pQ->sessionInfo.id != 0 ) {	// this is a common vfon client

		if( m_status.nServerStatus != ACTIVE ) {
			statusCode = SIMS_NOTAVAILABLE;
			packedData << (DWORD) m_config.anotherServerAddress.ip << (WORD)m_config.anotherServerAddress.port;
		} else {

			WriteLock lock( m_mutexRecord );

			SFIDRecord * pRec = FindRecordByID( pQ->sessionInfo.id );
			if( pRec && (pRec->sessionInfo.cookie == pQ->sessionInfo.cookie) ) {

				//pRec->sockChannel = req->channel;

				// this is the channel that the client send heartbeat
				// if it is different with login channel
				pRec->sockHeartbeat = req->channel;

				pRec->netLAN = pQ->addrLAN;
				pRec->is_dirty = 1;
				pRec->time = time(NULL);

				if( pQ->proxyMax ) { // if it has proxy engine started
					if( (pRec->sockChannel.peer.ip == pRec->netLAN.ip)	// only accept the WAN client as proxy
						|| pQ->proxyInfo.hasTag							// or else it must have tag for some special clients
						) {					 
						BBQProxyRecord * pProxy = NULL;

						ProxyMap::iterator iter = m_mapProxy.find( pQ->sessionInfo.id );
						if( iter != m_mapProxy.end() ) {
							pProxy = iter->second;
						} else {
							pProxy = new BBQProxyRecord;

							if( pProxy ) {
								memset( pProxy, 0, sizeof(BBQProxyRecord) );
								pProxy->sessionInfo = pQ->sessionInfo;
								pProxy->flags.client = 1;
								pProxy->startTime = time(NULL);

								m_mapProxy[ pQ->sessionInfo.id ] = pProxy;

								m_status.nProxyCount = m_mapProxy.size();
								m_status.nMaxProxyCount = max( m_status.nMaxProxyCount, m_status.nProxyCount );
							}
						}

						if( pProxy ) {
							pProxy->sockChannel = req->channel;
							pProxy->time = pRec->time;

							pProxy->proxyMax = pQ->proxyMax;
							pProxy->proxySlot = pQ->proxySlot;
							pProxy->proxyBandwidthUsed = pQ->proxyBandwidthUsed;
							pProxy->proxyBandwidthTotal = pQ->proxyBandwidthTotal;

							// record the ip & country code
							pProxy->ip = req->channel.peer.ip;
							pProxy->contryCode = m_Ip2CountryUtility.Ip2Country( req->channel.peer.ip );

							pProxy->serviceIp = pQ->proxyInfo.serviceIp;

							pProxy->hasTag = pQ->proxyInfo.hasTag;
						}
					}
				}

				if( pRec->userStatus == CLIENT_MCU ) { // update mcu info if this is a mcu
          if( m_config.write_useraction_log )
          {
            //output the  MCU heatbeat packet
            PString strMsg( PString::Printf, "Received the heartbeat from %u(%u).",   pQ->sessionInfo.id,  pQ->sessionInfo.cookie);
					  m_UserActionLog.Output( PLog::Info, strMsg );
          }
					MCUMap::iterator mcu_iter = m_mapMCU.find( pQ->sessionInfo.id );
					if( mcu_iter != m_mapMCU.end() ) {
						MCURecord * pMCU = mcu_iter->second;
						if( pMCU != NULL ) {
							pMCU->rooms = pQ->mcuInfo.rooms;
							pMCU->persons = pQ->mcuInfo.persons;
							pMCU->bps = pQ->mcuInfo.bps;
						}
					}

					// TODO: update status in central server in every heartbeat, for MCU only
					if( m_status.nInterlinkConnected ) {
						SIM_REQUEST report;
						for( int i=0; i<2; i++ ) {
							if( m_chanInterlink[i].peer.ip != 0 ) {

								SIM_REQINITCHAN( report, m_chanInterlink[i], SIM_SX_UPDATESWITCHINFO, sizeof(SIMD_SX_UPDATESWITCHINFO) );
								SIMD_SX_UPDATESWITCHINFO * pU = (SIMD_SX_UPDATESWITCHINFO *) & report.msg.simData;

								memset( pU, 0, sizeof(*pU) );
								pU->id = pRec->sessionInfo.id;
								//pU->info.login = 1;
								pU->info.is_tcp = (pRec->sockChannel.socket != 0) ? 1 : 0;
								pU->info.firewall_type = pRec->techInfo.firewall;
								//pU->info.status = pRec->userStatus & 0xff;
								pU->info.act_status = pRec->actStatus & 0xff;
								pU->info.set_status = pRec->setStatus & 0xff;

								// TODO: using this server external address ip
								//pU->info.addr.ip = 0;
								pU->info.addr.ip = m_config.thisServerAddress.ip;
								pU->info.addr.tcpPort = m_nDefaultTcpPort;
								pU->info.addr.udpPort = m_nDefaultUdpPort;

								pU->info.channel = pRec->sockChannel;

								SendMessage( & report );
							}
						}
					}
				}

				// TODO: 
				// we wish not support online friend list in heartbeat now,
				// to decrease resource requirement for FindRecordByID(...) action
				pUSM = pRec->pBuddyStatus;

				statusCode = SIMS_OK;
			} else {
				statusCode = SIMS_DENY;
			}
		}
	} else { // session.id == 0, it is a static proxy ?
		if( ( req->channel.peer.ip == LOCALHOST_IP )				// localhost
			|| ( IS_PRIVATE_IP(req->channel.peer.ip) )				// different host behind firewall
			|| ( req->channel.peer.ip == pQ->sessionInfo.cookie )	// public ip or direct
			|| pQ->proxyInfo.hasTag									// or else it must have tag for some special clients 
			) { // yes, it is

			WriteLock lock( m_mutexRecord );
	
			BBQProxyRecord * pProxy = NULL;

			ProxyMap::iterator iter = m_mapPureProxy.find( req->channel.peer.ip );
			if( iter != m_mapPureProxy.end() ) {
				pProxy = iter->second;
			} else {
				pProxy = new BBQProxyRecord;

				if( pProxy ) {
					memset( pProxy, 0, sizeof(BBQProxyRecord) );
					pProxy->sessionInfo = pQ->sessionInfo;
					pProxy->flags.volunteer = 1;
					pProxy->startTime = time(NULL);

					m_mapPureProxy[ req->channel.peer.ip ] = pProxy;

					m_status.nPureProxyCount = m_mapPureProxy.size();
					m_status.nMaxPureProxyCount = max( m_status.nMaxPureProxyCount, m_status.nPureProxyCount );
				}
			}

			if( pProxy ) {
				pProxy->sockChannel = req->channel;
				pProxy->time = time(NULL);

				pProxy->proxyMax = pQ->proxyMax;
				pProxy->proxySlot = pQ->proxySlot;
				pProxy->proxyBandwidthUsed = pQ->proxyBandwidthUsed;
				pProxy->proxyBandwidthTotal = pQ->proxyBandwidthTotal;

				pProxy->adminPort = pQ->proxyInfo.adminPort;
				pProxy->tcpPort = pQ->proxyInfo.tcpPort;
				pProxy->udpPortMin = pQ->proxyInfo.udpPortMin;
				pProxy->udpPortMax = pQ->proxyInfo.udpPortMax;

				// record the ip & country code
				pProxy->ip = req->channel.peer.ip;
				pProxy->contryCode = m_Ip2CountryUtility.Ip2Country( req->channel.peer.ip );

				pProxy->serviceIp = pQ->proxyInfo.serviceIp;

				pProxy->hasTag = pQ->proxyInfo.hasTag;
			}

			//packedData << ((DWORD) req->channel.peer.ip);

			statusCode = SIMS_OK;

		} else { // no, it is not

			statusCode = SIMS_DENY;

			PString strMsg( PString::Printf, "A vfon proxy attempt to connect from %s(%s), kicked.", 
				(const char *) ToString(req->channel.peer.ip), (const char *) ToString(pQ->sessionInfo.cookie) );
			PTRACE( 1, strMsg );
		}
	}

	if( statusCode == SIMS_OK ) {

		if( pUSM != NULL ) {
			nFriends = pUSM->size();
			for( UserStatusMap::iterator it = pUSM->begin(), et = pUSM->end(); it != et; it ++ ) {
				packedData << it->first << it->second;
			}
		}

		packedData << ((DWORD) req->channel.peer.ip);

		reply.msg.simHeader.size = sizeof(statusCode) + sizeof(nFriends) + packedData.Size();
		packedMsg << reply.channel << reply.msg.simHeader << statusCode << nFriends << packedData;
	} else {
		reply.msg.simHeader.size = sizeof(statusCode) + packedData.Size();
		packedMsg << reply.channel << reply.msg.simHeader << statusCode << packedData;
	}

	SendMessage( (SIM_REQUEST *) packedMsg.Data() );

	return true;
}

bool BBQServer::IsCallAllowed( uint32 nFrom, uint32 nTo )
{
	ReadLock lock( m_mutexRecord );

	SFIDRecord * pFrom = nFrom ? FindRecordByID( nFrom ) : NULL;
	if( pFrom ) {
		if( pFrom->flags.no_callout ) return false;
		if( pFrom->userStatus == CLIENT_MCU ) return true;
	}

	SFIDRecord * pTo = nTo ? FindRecordByID( nTo ) : NULL;
	if( pTo ) {
		if( pTo->userStatus == CLIENT_MCU ) return true;

		bool bToPublic = ( pTo && ( (! pTo->pParentIdList) || (pTo->pParentIdList->Count() == 0) ) && ( (! pTo->pChildIdList) || (pTo->pChildIdList->Count() == 0) ));
		bool bFromPublic = ( pFrom && ( (! pFrom->pParentIdList) || (pFrom->pParentIdList->Count() == 0) ) && ( (! pFrom->pChildIdList) || (pFrom->pChildIdList->Count() == 0) ));

		bool bToGroup = ( ! bToPublic );
		bool bFromGroup = ( ! bFromPublic );

		if( m_config.enable_cug ) {
			if( bToGroup ) {
				if( IsContainedInSameTree( nFrom, nTo ) ) return true;
				else return IsAllowedByMeAndParents( nTo, nFrom );

			} else if ( bFromGroup ) {
				if( ! m_config.allow_cug_call_public ) return false;
				else if( IsBlockedByMeAndParents( nTo, nFrom ) ) return false;
			}
		}
		
		{ // public user, or without CUG

			// check flags
			if( pTo->flags.no_callin ) return false;

			// check block list
			if( pTo->pBlockList && pTo->pBlockList->Contain(nFrom) ) return false;

			// check friend list
			if( pTo->pUserInfo ) {
				if( pTo->pUserInfo->security.allow_call == ALLOWCALL_BYNONE ) return false;

				if( pTo->pUserInfo->security.allow_call == ALLOWCALL_BYFRIEND ) {

          //CUG member check, If the client selected  'only allow friend to call me', the users could call each other in the same tree.
          //add by chenyuan begin
          if (!m_config.hide_id_tree_to_client)
          {
            uint_list uintlist;
			      RecordMap records;
			      if( GetRootUserNodes(nFrom, records ) ) {
				      for( RecordMap::iterator it = records.begin(), et = records.end(); it != et; it ++ ) {
					      uintlist.push_back( it->first );
				      }
			      }

			      // get all nodes in same CUG tree
			      records.clear();
			      if( GetSubUserRecords( uintlist, records ) ) {
				      for( RecordMap::iterator it = records.begin(), et = records.end(); it != et; it ++ ) {
                //check the result
                if (nTo == it->first)
                  return true;
              }
            }
          }
          //add by chenyuan end

					if( ! pTo->pFriendList ) return false;
					if( ! pTo->pFriendList->Contain(nFrom) ) return false;
				}
			}

			return true;
		}
	} else return false;

	return true;
}

bool BBQServer::OnMsgClientLocate( const SIM_REQUEST * req )
{
	const SIMD_CS_LOCATE * pQ = (const SIMD_CS_LOCATE *) & req->msg.simData;

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_LOCATE, sizeof(SIMD_SC_LOCATE), req );
	SIMD_SC_LOCATE * pA = (SIMD_SC_LOCATE *) & reply.msg.simData;
	memset( pA, 0, sizeof(*pA) );

	// TODO: validate call user's identity here
	// but, sometimes identity verifying is not needed, cause MCU might query from one user to another

	bool bAllowQuery = IsCallAllowed( pQ->sessionInfo.id, pQ->queryID );

	if( ! bAllowQuery ) {
		pA->statusCode = SIMS_NOTFOUND;
		SendMessage( & reply );
		return true;
	}

	bool bClusterConnected = ((m_config.nServerType == CLUSTER) && m_status.nInterlinkConnected && (m_chanInterlink[0].peer.ip != 0));

	if( bClusterConnected && (! pQ->thisServerOnly) ) {

		// for cluster, forward and ask the cluster central for status

		SIM_REQUEST fwd;
		SIM_REQINITCHAN( fwd, m_chanInterlink[0], SIM_SX_QLOCATE, sizeof(SIMD_SX_QLOCATE) );
		SIMD_SX_QLOCATE * pF = (SIMD_SX_QLOCATE *) & fwd.msg.simData;
		memset( pF, 0, sizeof(*pF) );
		pF->queryChannel = req->channel;
		pF->sessionInfo = pQ->sessionInfo;
		pF->id = pQ->queryID;

		SendMessage( & fwd );

	} else {

		// then locate in this server only
		ReadLock lock( m_mutexRecord );
		SFIDRecord * pRec = FindRecordByID( pQ->queryID );
		if( pRec != NULL ) {
			pA->queryID = pRec->sessionInfo.id;

			pA->addrLAN = pRec->netLAN;
			pA->addrWAN = pRec->sockChannel.peer;

			pA->company_id = pRec->pServiceInfo ? pRec->pServiceInfo->company_id : 0;

			pA->addrServer = pRec->netServer;
			pA->isTCP = pRec->sockChannel.socket ? 1 : 0;
			pA->firewallType = pRec->techInfo.firewall;

			// pA->channelServerSide = pRec->sockChannel;

			bool bAlwaysForward = false;
			if( pRec->pCallForwardInfo ) {
				for( int i=0; i<pRec->pCallForwardInfo->count; i++ ) {
					BBQCallForwardRecord * p = & pRec->pCallForwardInfo->callfwd[i];
					if( (p->fwd_condition == CALLFWD_ANY) && p->enabled ) {
						bAlwaysForward = true;
						break;
					}
				}
			}

			if( (! pRec->sessionInfo.cookie)
				|| (time(NULL) > pRec->time + m_config.heartbeat_timeout) ) {
				pA->statusCode = SIMS_TIMEOUT; // though timeout, the information may be still usable, user can have a try.

			} else if( bAlwaysForward ) {
				pA->statusCode = SIMS_NOTAVAILABLE;

			} else {
				pA->statusCode = SIMS_OK;
			}
		} else {
			pA->statusCode = SIMS_NOTFOUND;
		}

		SendMessage( & reply );
	}

	return true;
}
bool BBQServer::OnMsgNotifyNatinfo( const SIM_REQUEST * req )
{
 
	const SIMD_CS_SAVE_NATCLIENT * pQ = (const SIMD_CS_SAVE_NATCLIENT *) & req->msg.simData;

 

	SIM_REQUEST notify;

	ReadLock lock( m_mutexRecord );

  SFIDRecord * pTo = FindRecordByID( pQ->to  );

	if( pTo ) {
    //sender
    //strncpy(pA->calllist, pQ->roomlist, sizeof(pA->calllist) -1);
	  SIMD_SC_SAVE_NATCLIENT * pA = (SIMD_SC_SAVE_NATCLIENT *) & notify.msg.simData;
    SIM_REQINITCHAN( notify, pTo->sockChannel, SIM_SC_SAVE_NATCLIENT, sizeof(SIMD_SC_SAVE_NATCLIENT) );
    *pA = *pQ;
    
    SendMessage( & notify, true );
  }
	return true;
}


bool BBQServer::OnMsgClientRequestChannel( const SIM_REQUEST * req )
{
	// if no dongle or license expired, do not allow user login
	if( m_bDisableFeature ) return true;

	const UCMD_CS_Q * pQ = (const UCMD_CS_Q *) & req->msg.simData;

#ifdef ANTI_PACKET_LOSS
	// check request history, if already handled, just ingore
	{
		LockIt sf( m_mutexCidTime );

		UNIQUE_CID ucid;
		ucid.uid = pQ->sessionInfo.id;
		ucid.cid = pQ->channelInfo.cid;

		if( m_mapRequestCidTime.find( ucid ) != m_mapRequestCidTime.end() ) {
			// same channel already handled, ignore this request
			return true;
		} else {
			m_mapRequestCidTime[ ucid ] = GetHostUpTimeInMs();
		}
	}
#endif

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, UCM_SC_A, sizeof(UCMD_SC_A), req );
	UCMD_SC_A * pA = (UCMD_SC_A *) & reply.msg.simData;

	ReadLock lock( m_mutexRecord );

	SFIDRecord * pTo = FindRecordByID( pQ->channelInfo.uid );

	if( pTo ) {
		bool bAllowQuery = IsCallAllowed( pQ->sessionInfo.id, pQ->channelInfo.uid );
		if( bAllowQuery ) {
			SIM_REQUEST fwd;
			SIM_REQINITCHAN( fwd, pTo->sockHeartbeat, UCM_SC_FWD_Q, sizeof(UCMD_SC_FWD_Q) );
			UCMD_SC_FWD_Q * pF = (UCMD_SC_FWD_Q *) & fwd.msg.simData;
			* pF = * pQ;

			//pF->channelInfo = pQ->channelInfo;
			pF->callerChannel = req->channel;
			pF->channelInfo.uid = pQ->sessionInfo.id;
			pF->channelInfo.thisWAN = req->channel.peer;

			//if( req->channel.socket ) pF->channelInfo.tcp = 1;

			pF->sessionInfo = pTo->sessionInfo;
			//pF->statusText[ SIM_STRINGSIZEMAX - 1 ] = '\0';

			pF->extraInfo.caller.use_firewall_here = 1;
			pF->extraInfo.caller.firewall = pQ->channelInfo.firewall;

			// if both side are behind strict firewall, then give a proxy
			bool bTcp = (pF->callerChannel.socket != 0 );
			bool bBothStrictFirewall = (pTo->techInfo.firewall + pQ->channelInfo.firewall > 4);
			bool bWan2Lan = IS_PRIVATE_IP( pTo->sockHeartbeat.peer.ip ); // target is inside LAN, so ask for a proxy forwarding data
			bool bLan2WanLan = IS_PRIVATE_IP( req->channel.peer.ip ) && ( pTo->sockHeartbeat.peer.ip != pTo->techInfo.interface_ip );

			if( bTcp || bBothStrictFirewall /*|| bWan2Lan || bLan2WanLan*/ ) {
				pF->channelInfo.give_me_proxy = 1;
			}

			if( bTcp ) fwd.channel = pTo->sockChannel;

#ifdef ANTI_PACKET_LOSS
			// for UDP users, send 3 times to avoid packet loss
			int n = fwd.channel.socket ? 1 : 3;
			//			TODO: send 3 packets in future, but now cannot, because old client cannot handle it
			//			int n = fwd.channel.socket ? 1 : 1;
			for( int i=0; i<n; i++ ) { SendMessage( & fwd ); sleep_ms(0); }
#else
			SendMessage( & fwd );
#endif

			if( m_config.write_useraction_log ) {
				PString strMsg( PString::Printf, "requesting channel from %d to %d: forwarded (%s).", pQ->sessionInfo.id, pQ->channelInfo.uid, (pF->channelInfo.give_me_proxy ? "ask for proxy" : "") );
				m_UserActionLog.Output( PLog::Debug, strMsg );
			}

			return true;
		} else {
			pA->statusCode = SIMS_REFUSE;
		}
	} else {
		pA->statusCode = SIMS_NOTFOUND;
	}

	SendMessage( & reply );

	if( m_config.write_useraction_log ) {
		PString strMsg( PString::Printf, "requesting channel from %d to %d: %s.", pQ->sessionInfo.id, pQ->channelInfo.uid, SIM_STATUSCODENAME(pA->statusCode) );
		m_UserActionLog.Output( PLog::Debug, strMsg );
	}

	return true;
}

bool BBQServer::OnMsgClientResponseChannel( const SIM_REQUEST * req )
{
	const UCMD_CS_A * pQ = (const UCMD_CS_A *) & req->msg.simData;

#ifdef ANTI_PACKET_LOSS
	// check request history, if already handled, just ingore
	{
		LockIt sf( m_mutexCidTime );

		UNIQUE_CID ucid;
		ucid.uid = pQ->channelInfo.uid;
		ucid.cid = pQ->channelInfo.cid;

		if( m_mapReplyCidTime.find( ucid ) != m_mapReplyCidTime.end() ) {
			// same channel already handled, ignore this request
			return true;
		} else {
			m_mapReplyCidTime[ ucid ] = GetHostUpTimeInMs();
		}
	}
#endif

	ReadLock lock( m_mutexRecord );

	SFIDRecord * pFrom = FindRecordByID( pQ->channelInfo.uid ); // caller
	SFIDRecord * pTo = FindRecordByID( pQ->sessionInfo.id );	// callee

	int nCallerFirewall = pQ->extraInfo.caller.use_firewall_here ? pQ->extraInfo.caller.firewall : (pFrom ? pFrom->techInfo.firewall : 1);
	int nCalledFirewall = pQ->extraInfo.called.use_firewall_here ? pQ->extraInfo.called.firewall : pQ->channelInfo.firewall;

	bool bBothStrictFirewall = ( nCallerFirewall + nCalledFirewall > 4 );
	bool bTcp = (pQ->callerChannel.socket != 0) || (req->channel.socket != 0);
	bool bWan2Lan = IS_PRIVATE_IP( pTo->sockHeartbeat.peer.ip ); // target is inside LAN, so ask for a proxy forwarding data

	// TODO: check ACL rules here

	bool bNeedProxy = ( 
#ifdef _DEBUG
		m_bProxyAllRequestChannel || // for test only, force to use proxy for all request
#endif
		// if both side are behind different strict firewall
		bBothStrictFirewall ||
		// tcp login, maybe UDP blocked, so use proxy as an option
		bTcp || 
		// one of the user request use proxy anyway
		pQ->channelInfo.give_me_proxy ||
		// WAN user call LAN user
		bWan2Lan
		);

	bool bAllowProxy = (pFrom && pFrom->flags.allow_proxy) || (pTo && pTo->flags.allow_proxy);

	SIM_REQUEST fwd;
	SIM_REQINIT( fwd, 0, 0, 0, 0, 0, UCM_SC_A, sizeof(UCMD_SC_A) );
	
	if( pQ->callerChannel.local.port != 0 ) {
		fwd.channel = pQ->callerChannel;
	} else {
		if( pFrom ) {
			fwd.channel = pFrom->sockHeartbeat;
		} else {
			fwd.channel.local = req->channel.local;
			fwd.channel.peer = pQ->channelInfo.peerWAN;
		}
	}

	UCMD_SC_A * pA = (UCMD_SC_A *) & fwd.msg.simData;
	* pA = * pQ;

	pA->sessionInfo.id = pQ->channelInfo.uid;

	pA->channelInfo.uid = pQ->sessionInfo.id;
	pA->channelInfo.thisWAN = req->channel.peer;

	pA->channelInfo.give_me_proxy = bNeedProxy;

	PString strProxy = "vproxy: ";

	if( bNeedProxy && ( ! bAllowProxy ) ) {
		pA->statusCode = SIMS_FIREWALL;
	}

	// TODO: send 3 packets in future, but now cannot, because old client cannot handle it
#ifdef ANTI_PACKET_LOSS
	// for UDP users, send 3 times to avoid packet loss
	int n = fwd.channel.socket ? 1 : 3;
//	int n = fwd.channel.socket ? 1 : 1;
//	TODO: send 3 packets in future, but now cannot, because old client cannot handle it
	for( int i=0; i<n; i++ ) { SendMessage( & fwd ); sleep_ms(0); }
#else
	SendMessage( & fwd );
#endif

	if( bNeedProxy ) {

		if( bAllowProxy ) {

			SIM_REQUEST proxy;
			SIM_REQINIT( proxy, 0, 0, 0, LOCALHOST_IP, m_nDefaultUdpPort, UCM_SP_CREATE, sizeof(UCMD_PROXY_CREATE) );
			if( m_config.dwListenIp ) {
				proxy.channel.peer.ip = m_config.dwListenIp;
			}
			UCMD_PROXY_CREATE * pC = (UCMD_PROXY_CREATE *) & proxy.msg.simData;

			memset( pC, 0, sizeof(*pC) );

			pC->channelInfo = pQ->channelInfo;
			pC->channelInfo.tcp = pQ->channelInfo.tcp | (bTcp ? 1 : 0);

			pC->entry[0].uid = pQ->channelInfo.uid;
			pC->entry[1].uid = pQ->sessionInfo.id;

			pC->appCode = pQ->statusCode;
			strncpy( pC->appString, pQ->statusText, SIM_APPSTRINGSIZEMAX ); pC->appString[ SIM_APPSTRINGSIZEMAX -1 ] = '\0';
			pC->extraInfo.called = pQ->extraInfo.called;
			pC->extraInfo.caller = pQ->extraInfo.caller;
			pC->callerChannel = pQ->callerChannel;

			// BBQProxyRecord * pProxyRec = PickProxy( pQ->channelInfo.uid, pQ->sessionInfo.id );

			BBQProxyRecord * pProxyRec = NULL;

			// to pick proxy by private tags
			if( pFrom && pTo ) { // caller and callee must be local user that already login
				if( pFrom->pTagString && pFrom->pTagString[0] && pTo->pTagString && pTo->pTagString[0] ) { // they must have non-empty tags
					pProxyRec = PickProxyByTag( pFrom->pTagString, pTo->pTagString );
				}
			}

			if( ! pProxyRec ) {
				if( ( ! IS_PRIVATE_IP( pQ->callerChannel.peer.ip ) ) && ( ! IS_PRIVATE_IP( pQ->callerChannel.peer.ip ) ) ) { // both global ip
					pProxyRec = PickProxyByIp2Country( pQ->callerChannel.peer.ip, req->channel.peer.ip );
				} else {
					pProxyRec = PickProxy( pQ->channelInfo.uid, pQ->sessionInfo.id );
				}
			}

			if( pProxyRec ) {
				pProxyRec->pick ++;
				proxy.channel = pProxyRec->sockChannel;
				//SIM_REQINITCHAN( proxy, pProxyRec->sockChannel, UCM_SP_CREATE, sizeof(UCMD_PROXY_CREATE) );
				pC->sessionInfo = pProxyRec->sessionInfo;
				pC->callerChannel = pQ->callerChannel;
				SendMessage( & proxy );

				CountryCode cc;
				cc.nCode = pProxyRec->contryCode;
				cc.szCode[3] = '\0';

				strProxy += PString( PString::Printf, "picked external (%s, %s)", (const char *)BBQMsgTerminal::ToString(pProxyRec->ip), cc.szCode );
			} else {
				strProxy += "no external, ";

				// no external proxy is found. 
				BBQProxy * pProxy = BBQProxy::GetCurrentProxy();
				if( pProxy ) {
					BBQProxy::Config config;	pProxy->GetConfig( config );
					BBQProxy::Status status;	pProxy->GetStatus( status );

					if( config.nChannelMax > (status.nEntries/2) ) {
						//SIM_REQINIT( proxy, 0, req->channel.local.ip, req->channel.local.port, LOCALHOST_IP, m_nDefaultUdpPort, UCM_SP_CREATE, sizeof(UCMD_PROXY_CREATE) );
						SendMessage( & proxy );

						strProxy += "picked build-in";
					} else {
						// no empty proxy slot
						strProxy += "build-in too busy";
					}
				} else {
					// local proxy is not started
					strProxy += "build-in not started";
				}
			}
		} else {
			strProxy += "not allow";
		}
	} else {
		strProxy += "no need";
	}

	if( m_config.write_useraction_log ) {
		PString strMsg( PString::Printf, "response from %d to %d: forwarded, (%s, %s).", pQ->sessionInfo.id, pQ->channelInfo.uid, (bTcp ? "tcp" : "udp"), (const char *)strProxy );
		m_UserActionLog.Output( PLog::Debug, strMsg );
	}

	return true;
}

bool BBQServer::OnMsgClientUpdateUserData( const SIM_REQUEST * req )
{
	SIM_SESSIONINFO sessionInfo;
	BBQUserInfo		userInfo;

	try {
		PBytePack packedData( & req->msg.simData, req->msg.simHeader.size );
		packedData >> sessionInfo >> userInfo;
	} catch ( const char * ) {
		// bad request
		return true;
	}

	{
		SIM_REQUEST reply;
		SIM_REPLY_REQUEST( & reply, SIM_SC_UPDATEUSERDATA, sizeof(uint32), req );
		uint32 * pA = (uint32 *) & reply.msg.simData;

		WriteLock safe( m_mutexRecord );

		SFIDRecord * pRec = FindRecordByID( sessionInfo.id );

		if( pRec 
			&& (pRec->sessionInfo.cookie == sessionInfo.cookie) 
			&& ( (! userInfo.security.modify) || 
					(0 == memcmp(pRec->password, userInfo.security.oldpass, USERDATA_NAME_SIZE)) // md5, 32 bytes
					)
			) {
			if( ! pRec->pUserInfo ) {
				pRec->pUserInfo = new BBQUserInfo;
			}
			if( pRec->pUserInfo ) {
				* (pRec->pUserInfo) = userInfo;

				if( userInfo.security.modify ) {
					memcpy( pRec->password, userInfo.security.newpass, USERDATA_NAME_SIZE );
				} else { // keep the old password
					memcpy( pRec->pUserInfo->security.oldpass, pRec->password, USERDATA_NAME_SIZE );
				}

				// Changed by Wu Hailong
				// 2004-07-31
				//if( m_config.save_on_change && m_pDB ) {
				BBQUserValidFlags flags;
				memset( & flags, 0, sizeof(flags) );
				flags.partial_valid = 1;
				flags.userInfo = 1;
				bool bOk = SaveData( pRec, SaveOnChange, & flags );

				if( m_config.write_useraction_log ) {
					PString strMsg( PString::Printf, "user %d update the profile (password %s)", pRec->sessionInfo.id, (userInfo.security.modify ? "changed" : "not changed") );
					m_UserActionLog.Output( PLog::Info, strMsg );
				}
				//}
				// Changed end

				* pA = bOk ? SIMS_OK : SIMS_DENY;

				SendMessage( & reply );

				// profile change, including alias, should inform buddy to update
				NotifyBuddyStatus( pRec );

			} else {
				* pA = SIMS_SERVERERROR;
				SendMessage( & reply );
			}

		} else {
			* pA = SIMS_DENY;
			SendMessage( & reply );
		}
	}

	return true;
}

bool BBQServer::OnMsgClientViewUserData( const SIM_REQUEST * req )
{
	SIM_SESSIONINFO		sessionInfo;
	uint32				queryID;

	try {
		PBytePack packedData( & req->msg.simData, req->msg.simHeader.size );
		packedData >> sessionInfo >> queryID;
	} catch ( const char * ) {
		// bad request
		return true;
	}

	{
		SIM_REQUEST reply;
		SIM_REPLY_REQUEST( & reply, SIM_SC_VIEWUSERDATA, 0, req );

		uint32					statusCode;
		BBQUserInfo				userInfo;
		VfonIdBindInfo			bindInfo;
		BBQUserTechParamInfo	techInfo;
		memset( & userInfo, 0, sizeof(userInfo) );
		memset( & bindInfo, 0, sizeof(bindInfo) );
		memset( & techInfo, 0, sizeof(techInfo) );

		ReadLock safe( m_mutexRecord );

		SFIDRecord * pRec = FindRecordByID( sessionInfo.id );
		if( pRec && (pRec->sessionInfo.cookie == sessionInfo.cookie) ) {

			pRec = FindRecordByID( queryID );
			if( pRec ) {
				if( pRec->pServiceInfo ) {
					bindInfo = pRec->pServiceInfo->bindInfo;
				}
				if( pRec->pUserInfo ) {
					userInfo = * pRec->pUserInfo;
					
					// mask contact info is required
					if( sessionInfo.id != queryID ) {
						switch( pRec->pUserInfo->contact.privacy ) {
						case 0: // allow all
							break;
						case 1: // only allow friends
							// private buddy
							if( pRec->pFriendList && pRec->pFriendList->Contain(sessionInfo.id) ) break;
							// or CUG buddy
							if( IsContainedInSameTree( sessionInfo.id, queryID ) ) break;
							// else fall through
						case 2: // allow none
							memset( & userInfo.contact, 0, sizeof(userInfo.contact) );
							break;
						}
					}

					// always mask off passwords
					userInfo.security.modify = 0;
					memset( userInfo.security.oldpass, 0, sizeof(userInfo.security.oldpass) );
					memset( userInfo.security.newpass, 0, sizeof(userInfo.security.newpass) );
				}
				statusCode = SIMS_OK;
			} else {
				statusCode = SIMS_NOTFOUND;
			}
		} else {
			statusCode = SIMS_DENY;
		}

		PBytePack packedData, packedMsg;
		packedData << statusCode << queryID << userInfo << bindInfo;
		reply.msg.simHeader.size = packedData.Size();
		packedMsg << reply.channel << reply.msg.simHeader << packedData;

		SendMessage( (SIM_REQUEST *) packedMsg.Data() );
	}

	return true;
}

bool BBQServer::OnMsgClientGetUserTechParam( const SIM_REQUEST * req )
{
	SIM_SESSIONINFO		sessionInfo;
	uint32				queryID;

	try {
		PBytePack packedData( & req->msg.simData, req->msg.simHeader.size );
		packedData >> sessionInfo >> queryID;
	} catch ( const char * ) {
		// bad request
		return true;
	}

	{
		SIM_REQUEST reply;
		SIM_REPLY_REQUEST( & reply, SIM_SC_GETUSERTECHPARAM, 0, req );

		uint32					statusCode;
		BBQUserTechParamInfo	techInfo;
		memset( & techInfo, 0, sizeof(techInfo) );

		ReadLock safe( m_mutexRecord );

		SFIDRecord * pRec = FindRecordByID( sessionInfo.id );
		if( pRec && (pRec->sessionInfo.cookie == sessionInfo.cookie) ) {

			pRec = FindRecordByID( queryID );
			if( pRec ) {
				techInfo = pRec->techInfo;
				statusCode = SIMS_OK;
			} else {
				statusCode = SIMS_NOTFOUND;
			}
		} else {
			statusCode = SIMS_DENY;
		}

		PBytePack packedData, packedMsg;
		packedData << statusCode << queryID << techInfo;
		reply.msg.simHeader.size = packedData.Size();
		packedMsg << reply.channel << reply.msg.simHeader << packedData;

		SendMessage( (SIM_REQUEST *) packedMsg.Data() );
	}

	return true;
}

bool BBQServer::OnMsgClientUpdateCallForwardInfo( const SIM_REQUEST * req )
{
	SIMD_CS_UPDATECALLFWDINFO * pQ = (SIMD_CS_UPDATECALLFWDINFO *) & req->msg.simData[0];

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_UPDATECALLFWDINFO, sizeof(SIMD_SC_UPDATECALLFWDINFO), req );
	SIMD_SC_UPDATECALLFWDINFO * pA = (SIMD_SC_UPDATECALLFWDINFO *) & reply.msg.simData[0];
	memset( pA, 0, sizeof(*pA) );
	pA->sessionInfo = pQ->sessionInfo;
	pA->userID = pQ->userID;
	pA->statusCode = SIMS_OK;

	if( pQ->userID == pQ->sessionInfo.id ) {
		SFIDRecord * pRec = FindRecordByID( pQ->userID );
		if( pRec && pRec->pCallForwardInfo ) {
			if( pRec->sessionInfo.cookie == pQ->sessionInfo.cookie ) {

				//* pRec->pCallForwardInfo = pQ->callfwdinfo;
	
				pRec->pCallForwardInfo->count = 0;
				int n = min( pQ->callfwdinfo.count, CALLFWD_COUNT );
				for( int i=0; i<n; i++ ) {
					if( pQ->callfwdinfo.callfwd[i].fwd_condition ) {
						pRec->pCallForwardInfo->callfwd[ pRec->pCallForwardInfo->count ] = pQ->callfwdinfo.callfwd[i];
						pRec->pCallForwardInfo->count ++;
						if( pRec->pCallForwardInfo->count >= CALLFWD_COUNT ) break;
					}
				}

				BBQUserValidFlags flags;
				memset( & flags, 0, sizeof(flags) );
				flags.partial_valid = 1;
				flags.call_forward = 1;
				SaveData( pRec, SaveOnChange, & flags );

			} else {
				pA->statusCode = SIMS_DENY;
			}
		} else {
			pA->statusCode = SIMS_NOTFOUND;
		}
	} else {
		pA->statusCode = SIMS_DENY;
	}

	SendMessage( & reply );

	return true;
}

bool BBQServer::OnMsgClientGetCallForwardInfo( const SIM_REQUEST * req )
{
	SIMD_CS_GETCALLFWDINFO * pQ = (SIMD_CS_GETCALLFWDINFO *) & req->msg.simData[0];

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_GETCALLFWDINFO, sizeof(SIMD_SC_GETCALLFWDINFO), req );
	SIMD_SC_GETCALLFWDINFO * pA = (SIMD_SC_GETCALLFWDINFO *) & reply.msg.simData[0];
	memset( pA, 0, sizeof(*pA) );
	pA->sessionInfo = pQ->sessionInfo;
	pA->userID = pQ->userID;
	pA->statusCode = SIMS_OK;

	bool bAllowQuery = (pQ->sessionInfo.id == pQ->userID) || IsCallAllowed( pQ->sessionInfo.id, pQ->userID );
	if( bAllowQuery ) {
		SFIDRecord * pRec = FindRecordByID( pQ->userID );
		if( pRec && pRec->pCallForwardInfo ) {

			//pA->callfwdinfo = * pRec->pCallForwardInfo;
			time_t tNow = time(NULL);

			pA->callfwdinfo.count = 0;
			int n = min( pRec->pCallForwardInfo->count, CALLFWD_COUNT );
			for( int i=0; i<n; i++ ) {
				if( pRec->pCallForwardInfo->callfwd[i].fwd_condition ) {
					BBQCallForwardRecord * pCallFwd = & pA->callfwdinfo.callfwd[ pA->callfwdinfo.count ];
					* pCallFwd = pRec->pCallForwardInfo->callfwd[i];

					// if this is a group id, and set to forward any sub online node
					if( pCallFwd->pick_subid && pRec->pChildIdList && (pRec->pChildIdList->Count()>0) ) { 
						uint32 * pId = pRec->pChildIdList->Data();
						int m = pRec->pChildIdList->Count();

						if( m > 0 ) {
							// we start searching from a random position, to avoid always assigning task to same person
							int x = rand() % m;

							bool bFound = false;
							for( int j=x; j<m; j++ ) {
								SFIDRecord * pSubRec = FindRecordByID( pId[j] );
								if( pSubRec																		// found
									&& (pSubRec->time + m_config.heartbeat_timeout > tNow)						// online
									&& (pSubRec->actStatus == CLIENT_NORMAL) ) {								// normal status

									pCallFwd->vfonid.userid = pId[j];
									pCallFwd->vfonid.serverid = m_config.domainInfo.dwId;
									bFound = true;
									break;
								}
							}

							if( ! bFound ) {
								for( int j=0; j<x; j++ ) {
									SFIDRecord * pSubRec = FindRecordByID( pId[j] );
									if( pSubRec																		// found
										&& (pSubRec->time + m_config.heartbeat_timeout > tNow)						// online
										&& (pSubRec->actStatus == CLIENT_NORMAL) ) {								// normal status

										pCallFwd->vfonid.userid = pId[j];
										pCallFwd->vfonid.serverid = m_config.domainInfo.dwId;
										bFound = true;
										break;
									}
								}
							}
						}
					}

					pA->callfwdinfo.count ++;
					if( pA->callfwdinfo.count >= CALLFWD_COUNT ) break;
				}
			}

		} else {
			pA->statusCode = SIMS_NOTFOUND;
		}
	} else {
		pA->statusCode = SIMS_DENY;
	}

	SendMessage( & reply );

	return true;
}

#define LIST_USER_PAGE_SIZE		20

bool BBQServer::OnMsgClientListOnlineUser( const SIM_REQUEST * req )
{
	SIM_SESSIONINFO		sessionInfo;
	uint32				nStart;

	try {
		PBytePack packedData( & req->msg.simData, req->msg.simHeader.size );
		packedData >> sessionInfo >> nStart;
	} catch ( const char * ) {
		// bad request
		return true;
	}

	{
		SIM_REQUEST reply;
		SIM_REPLY_REQUEST( & reply, SIM_SC_LISTONLINE, 0, req );

		uint32 statusCode;
		uint32 nTotal = m_status.nOnlineUsers;
		uint32 nCount = 0;
		BBQUserBriefRecord	briefInfo;
		memset( & briefInfo, 0, sizeof(briefInfo) );

		PBytePack packedData, packedMsg;

		ReadLock safe( m_mutexRecord );

		SFIDRecord * pRec = FindRecordByID( sessionInfo.id );
		if( pRec 
			//&& (pRec->sessionInfo.cookie == sessionInfo.cookie) 
			&& (! pRec->flags.no_listonline) 
			) {

			time_t now = time(NULL);
			uint32 i = 0;
			for( RecordMap::iterator Iter = m_mapRecord.begin(), eIter = m_mapRecord.end(); Iter != eIter; Iter ++ ) {
				SFIDRecord * p = Iter->second;

				if( (p->sessionInfo.cookie == 0)					// already logout
					|| (p->userStatus == CLIENT_MCU)				// MCU
					|| (now > p->time + m_config.heartbeat_timeout)	// timeout
					|| (p->actStatus == CLIENT_OFFLINE)			// logout
					|| (p->actStatus == CLIENT_INVISIBLE)			// invisible
					|| (p->pUserInfo && p->pUserInfo->security.hide_in_onlinelist) // hide in online list
					|| (m_config.enable_cug && p->pParentIdList && (p->pParentIdList->Count()>0) ) // cug grouped user
					) continue;

				// parent is not empty, so it's a group in company
				if( p->pParentIdList && (pRec->pParentIdList->Count() > 0) ) {
					// for CUG (closed user group) purpose, people in company will not be listed.
					if( m_config.enable_cug ) continue;
				}

				// count on
				if( i++ < nStart ) continue;

				memset( & briefInfo, 0, sizeof(briefInfo) );
				briefInfo.uid = p->sessionInfo.id;
				if( p->pUserInfo ) {
					briefInfo.video = p->techInfo.video;
					briefInfo.audio = p->techInfo.audio;
					briefInfo.image = p->pUserInfo->basic.image;
					briefInfo.age = p->pUserInfo->basic.age;
					briefInfo.gender = p->pUserInfo->basic.gender;
					memcpy( briefInfo.alias, p->pUserInfo->basic.alias, USERDATA_NAME_SIZE );
					memcpy( briefInfo.city, p->pUserInfo->basic.city, USERDATA_NAME_SIZE );
					memcpy( briefInfo.state, p->pUserInfo->basic.state, USERDATA_NAME_SIZE );
				}
				packedData << briefInfo;
				nCount ++;

				// count off
				if( nCount >= LIST_USER_PAGE_SIZE ) break;
			}

			statusCode = SIMS_OK;
		} else {
			statusCode = SIMS_DENY;
		}

		reply.msg.simHeader.size = sizeof(statusCode) + sizeof(nTotal) + sizeof(nStart) + sizeof(nCount) + packedData.Size();
		packedMsg	<< reply.channel << reply.msg.simHeader 
					<< statusCode << nTotal << nStart << nCount << packedData;

		SendMessage( (SIM_REQUEST *) packedMsg.Data() );
	}

	return true;
}

bool BBQServer::OnMsgClientSearchUser( const SIM_REQUEST * req )
{
	return true;
}


bool BBQServer::OnMsgClientAddUser( const SIM_REQUEST * req )
{
	SIM_SESSIONINFO		sessionInfo;
	uint32				userID;

	bool				bFriend = (req->msg.simHeader.id == SIM_CS_ADDCONTACT);

	try {
		PBytePack packedData( & req->msg.simData, req->msg.simHeader.size );
		packedData >> sessionInfo >> userID;
	} catch ( const char * ) {
		// bad request
		return true;
	}

	{
		uint32 nResultCode = 0;
		uint32 nStatus = 0;

		WriteLock safe( m_mutexRecord );

		SFIDRecord * pRecFriend = FindRecordByID( userID );
		if( pRecFriend ) {
			SFIDRecord *pRec = FindRecordByID( sessionInfo.id );

			bool bAllow = true;

			if( pRec 
				&& (pRec->sessionInfo.cookie == sessionInfo.cookie)
				&& bAllow ) {

				if( bFriend ) {
					if( pRec->flags.no_private_buddylist ) {
						nResultCode = SIMS_DENY;
					} else {
						if( ! pRec->pFriendList ) {
							pRec->pFriendList = new UserIdList;
						}
						if( pRec->pFriendList ) {
							if( pRec->pFriendList->Count() < USERDATA_LIST_SIZE-1 ) {

								// remove from block list if in it
								if( pRec->pBlockList && pRec->pBlockList->Contain( userID ) ) {
									pRec->pBlockList->Remove( userID );
								}

								// add to friend list
								pRec->pFriendList->Add( userID );

								// add to wait auth list
								{
									if( ! pRecFriend->pWaitAuthList ) pRecFriend->pWaitAuthList = new UserIdList();
									if( pRecFriend->pWaitAuthList ) pRecFriend->pWaitAuthList->Add( pRec->sessionInfo.id );
									pRecFriend->is_dirty = 1;

									BBQUserValidFlags flags;
									memset( & flags, 0, sizeof(flags) );
									flags.partial_valid = 1;
									flags.friends = 1;
									SaveData( pRecFriend, SaveOnChange, & flags );
								}

								// add to buddy's notify list
								//if( ! pRecFriend->pBuddyReverseLink ) pRecFriend->pBuddyReverseLink = new UserIdList();
								//if( pRecFriend->pBuddyReverseLink ) pRecFriend->pBuddyReverseLink->Add( pRec->sessionInfo.id );

								pRec->is_dirty = 1;
								nResultCode = SIMS_OK;

								time_t offline = time(NULL) - m_config.heartbeat_timeout;
								if( (pRecFriend->sessionInfo.cookie == 0) || (pRecFriend->time < offline) ) pRecFriend->actStatus = CLIENT_OFFLINE;
								nStatus = pRecFriend->userStatus;

								if( pRecFriend->pFriendList->Contain( pRec->sessionInfo.id ) ) {
									// if both are buddy of each other, so they can see the status
									if( pRec->pBuddyStatus ) {
										(* pRec->pBuddyStatus)[ userID ] = nStatus;
									}
									if( pRecFriend->pBuddyStatus ) {
										(* pRecFriend->pBuddyStatus)[ pRec->sessionInfo.id ] = pRec->userStatus;
									}
								} else {
									// if i am not buddy of the guy, he will not tell me his status
									nStatus = CLIENT_OFFLINE;
								}

							} else {
								nResultCode = SIMS_DENY;
							}
						} else {
							nResultCode = SIMS_SERVERERROR;
						}
					}
				} else {
					if( ! pRec->pBlockList ) {
						pRec->pBlockList = new UserIdList;
					}
					if( pRec->pBlockList ) {
						if( pRec->pBlockList->Count() < USERDATA_LIST_SIZE-1 ) {

							// remove from friend list if in it
							if( pRec->pFriendList && pRec->pFriendList->Contain( userID ) ) {
								pRec->pFriendList->Remove( userID );
							}

							// remove from status notify list
							if( pRecFriend->pBuddyStatus ) {
								UserStatusMap::iterator it = pRecFriend->pBuddyStatus->find( pRec->sessionInfo.id );
								if( it != pRecFriend->pBuddyStatus->end() ) {
									STL_ERASE( (* pRecFriend->pBuddyStatus), UserStatusMap::iterator, it );
								}
							}

							// remove me from this notify list
							//if( pRecFriend->pBuddyReverseLink ) pRecFriend->pBuddyReverseLink->Remove( pRec->sessionInfo.id );

							// add to block list
							pRec->pBlockList->Add( userID );

							pRec->is_dirty = 1;
							nResultCode = SIMS_OK;
						} else {
							nResultCode = SIMS_DENY;
						}

					} else {
						nResultCode = SIMS_SERVERERROR;
					}
				}

				// save changes
				if( pRec->is_dirty ) {
					BBQUserValidFlags flags;
					memset( & flags, 0, sizeof(flags) );
					flags.partial_valid = 1;
					flags.friends = 1;
					flags.blocks = 1;
					SaveData( pRec, SaveOnChange, & flags );

					if( m_config.write_useraction_log ) {
						PString strMsg( PString::Printf, "user %d added %d to his %s list", pRec->sessionInfo.id, userID, (bFriend ? "buddy" : "block") );
						m_UserActionLog.Output( PLog::Info, strMsg );
					}
				}

				// NEW: send notifying message to client
				if( nResultCode == SIMS_OK ) {
					SIM_REQUEST notify;
					SIM_REQINITCHAN( notify, pRecFriend->sockChannel, SIM_SC_NOTIFY_CONTACT, sizeof(SIMD_SC_NOTIFY_CONTACT) );
					SIMD_SC_NOTIFY_CONTACT * pN = (SIMD_SC_NOTIFY_CONTACT *) & notify.msg.simData;
					pN->sessionInfo = pRecFriend->sessionInfo;
					pN->statusCode = SIMS_OK;
					pN->actionCode = req->msg.simHeader.id;
					pN->userId = sessionInfo.id;

					bool bOnline = ( pRecFriend->sessionInfo.cookie && (pRecFriend->time + m_config.heartbeat_timeout > time(NULL)) );

					if( bOnline ) {
						// post to the client directly
						PostMessage( & notify, true );

					} else { // send to domain server for forwarding
						if( m_status.nInterlinkConnected ) {
							for( int i=0; i<2; i++ ) {
								if( m_chanInterlink[i].peer.ip != 0 ) {
									notify.channel = m_chanInterlink[i];
									SendMessage( & notify );
								}
							}
						}
					}
				}

				// notify the target to change status display
				NotifyBuddyStatus( pRec, userID );
			} else {
				nResultCode = SIMS_DENY;
			}

		} else {
			nResultCode = SIMS_NOTFOUND;
		}

		// send back reply
		{
			SIM_REQUEST reply;
			SIM_REPLY_REQUEST( & reply, (uint16)(bFriend ? SIM_SC_ADDCONTACT : SIM_SC_ADDBLOCK), sizeof(uint32) * 2, req );
			uint32 * pA = (uint32 *) & reply.msg.simData;
			* pA ++ = nResultCode;
			* pA = nStatus;
			SendMessage( & reply );
		}

	}

	return true;
}

bool BBQServer::OnMsgClientDeleteUser( const SIM_REQUEST * req )
{
	SIM_SESSIONINFO		sessionInfo;
	uint32				userID;

	bool				bFriend = (req->msg.simHeader.id == SIM_CS_DELETECONTACT);

	try {
		PBytePack packedData( & req->msg.simData, req->msg.simHeader.size );
		packedData >> sessionInfo >> userID;
	} catch ( const char * ) {
		// bad request
		return true;
	}

	{
		uint32 nResultCode = 0;

		WriteLock safe( m_mutexRecord );

		SFIDRecord * pRec = FindRecordByID( sessionInfo.id );

		if( pRec && (pRec->sessionInfo.cookie == sessionInfo.cookie) ) {
			SFIDRecord * pRecFriend = FindRecordByID( userID );

			if( bFriend ) {
				if( pRec->pFriendList ) {
					pRec->pFriendList->Remove( userID );
					pRec->is_dirty = 1;

					// remove from status notify list
					if( pRec->pBuddyStatus ) {
						UserStatusMap::iterator it = pRec->pBuddyStatus->find( userID );
						if( it != pRec->pBuddyStatus->end() ) {
							STL_ERASE( (* pRec->pBuddyStatus), UserStatusMap::iterator, it );
						}
					}
					if( pRecFriend && pRecFriend->pBuddyStatus ) {
						UserStatusMap::iterator it = pRecFriend->pBuddyStatus->find( pRec->sessionInfo.id );
						if( it != pRecFriend->pBuddyStatus->end() ) {
							STL_ERASE( (* pRecFriend->pBuddyStatus), UserStatusMap::iterator, it );
						}
					}
				}
			} else {
				if( pRec->pBlockList ) {
					pRec->pBlockList->Remove( userID );
					pRec->is_dirty = 1;
				}
			}

			if( pRec->is_dirty ) {
				BBQUserValidFlags flags;
				memset( & flags, 0, sizeof(flags) );
				flags.partial_valid = 1;
				flags.friends = 1;
				flags.blocks = 1;
				SaveData( pRec, SaveOnChange, & flags );

				if( m_config.write_useraction_log ) {
					PString strMsg( PString::Printf, "user %d removed %d from his %s list", pRec->sessionInfo.id, userID, (bFriend ? "buddy" : "block") );
					m_UserActionLog.Output( PLog::Info, strMsg );
				}
			}

			// NEW: send notifying message to client
			if( pRecFriend ) {
				SIM_REQUEST notify;
				SIM_REQINITCHAN( notify, pRecFriend->sockChannel, SIM_SC_NOTIFY_CONTACT, sizeof(SIMD_SC_NOTIFY_CONTACT) );
				SIMD_SC_NOTIFY_CONTACT * pN = (SIMD_SC_NOTIFY_CONTACT *) & notify.msg.simData;
				pN->sessionInfo = pRecFriend->sessionInfo;
				pN->statusCode = SIMS_OK;
				pN->actionCode = req->msg.simHeader.id;
				pN->userId = sessionInfo.id;

				bool bOnline = ( pRecFriend->sessionInfo.cookie && (pRecFriend->time + m_config.heartbeat_timeout > time(NULL)) );

				if( bOnline ) {
					// post to the client directly
					PostMessage( & notify, true );

				} else { // send to domain server for forwarding
					if( m_status.nInterlinkConnected ) {
						for( int i=0; i<2; i++ ) {
							if( m_chanInterlink[i].peer.ip != 0 ) {
								notify.channel = m_chanInterlink[i];
								SendMessage( & notify );
							}
						}
					}
				}
			}

			// notify the target to change status display
			NotifyBuddyStatus( pRec, userID, CLIENT_OFFLINE );

			nResultCode = SIMS_OK;
		} else {
			nResultCode = SIMS_DENY;
		}

		SIM_REQUEST reply;
		SIM_REPLY_REQUEST( & reply, (uint16)(bFriend ? SIM_SC_DELETECONTACT : SIM_SC_DELETEBLOCK), sizeof(uint32), req );
		uint32 * pA = (uint32 *) & reply.msg.simData;
		* pA = nResultCode;
		SendMessage( & reply );

	}

	return true;
}

bool BBQServer::OnMsgClientGetUserList( const SIM_REQUEST * req )
{
	SIM_SESSIONINFO		sessionInfo;

	bool				bFriend = (req->msg.simHeader.id == SIM_CS_GETCONTACTLIST);

	try {
		PBytePack packedData( & req->msg.simData, req->msg.simHeader.size );
		packedData >> sessionInfo;
	} catch ( const char * ) {
		// bad request
		return true;
	}

	{
		SIM_REQUEST reply;
		SIM_REPLY_REQUEST( & reply, (uint16)(bFriend ? SIM_SC_GETCONTACTLIST : SIM_SC_GETBLOCKLIST), 0, req );

		uint32 statusCode;
		uint32 nCount = 0;
		PBytePack packedData, packedMsg;

		ReadLock safe( m_mutexRecord );

		SFIDRecord * pRec = FindRecordByID( sessionInfo.id );
		if( pRec && (pRec->sessionInfo.cookie == sessionInfo.cookie) ) {
			statusCode = SIMS_OK;
			if( bFriend ) {
				if( pRec->flags.no_private_buddylist ) {
					nCount = 0;
				} else if( pRec->pFriendList ) {
					DWORD t = PTimer::Tick().GetInterval();

					uint32 * pID = pRec->pFriendList->Data();
					int n = pRec->pFriendList->Count();
					if( n >= USERDATA_LIST_SIZE ) n = USERDATA_LIST_SIZE;
					time_t offline = time(NULL) - m_config.heartbeat_timeout;
					for( int i=0; i<n; i ++, pID ++ ) {
						pRec = FindRecordByID( *pID );
						if( pRec ) {
							if( (pRec->sessionInfo.cookie == 0) || (pRec->time < offline) ) pRec->actStatus = CLIENT_OFFLINE;
							packedData << (*pID) << pRec->userStatus;
							nCount ++;
						}
					}

					//DWORD ti = PTimer::Tick().GetInterval() - t;
					//PString strMsg( PString::Printf, "time used for contact list is %d ms.", ti );
					//PLOG( m_pLogger, Info, strMsg );

				} else {
					nCount = 0;
				}
			} else {
				if( pRec->pBlockList ) {
					nCount = pRec->pBlockList->Count();
					if( nCount >= USERDATA_LIST_SIZE ) nCount = USERDATA_LIST_SIZE;
					packedData.Pack( pRec->pBlockList->Data(), nCount * sizeof(uint32) );
				} else {
					nCount = 0;
				}
			}
		} else {
			statusCode = SIMS_DENY;
		}

		reply.msg.simHeader.size = sizeof(statusCode) + sizeof(nCount) + packedData.Size();
		packedMsg << reply.channel << reply.msg.simHeader << statusCode << nCount << packedData;

		SendMessage( (SIM_REQUEST *) packedMsg.Data() );
	}

	return true;
}

bool BBQServer::OnMsgClientUploadDataBlock( const SIM_REQUEST * req )
{
	const SIMD_DATABLOCK * pQ = (const SIMD_DATABLOCK *) & req->msg.simData;

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_UPLOADDATABLOCK, sizeof(SIMD_DATABLOCK), req );
	SIMD_DATABLOCK * pA = (SIMD_DATABLOCK *) & reply.msg.simData;
	* pA = * pQ;

	if( 0 == strcmp( pQ->key, "@MyTagString@" ) ) { // this is a special key

		if( pQ->sessionInfo.id != 0 ) { // named client or proxy
			SFIDRecord * pRec = FindRecordByID( pQ->sessionInfo.id );
			if( pRec && (pRec->sessionInfo.cookie == pQ->sessionInfo.cookie) ) {

				// update if different
				if( pRec->pTagString ) {
					if( 0 != strcmp( pRec->pTagString, pQ->data ) ) {
						free( pRec->pTagString );
						pRec->pTagString = NULL;
					}
				}
				if( (! pRec->pTagString) && pQ->data[0] ) pRec->pTagString = strdup( pQ->data );

				ProxyMap::iterator iter = m_mapProxy.find( pQ->sessionInfo.id );
				if( iter != m_mapProxy.end() ) {
					BBQProxyRecord * pProxy = iter->second;

					// update if different
					if( pProxy->pTagString ) {
						if( 0 != strcmp( pProxy->pTagString, pQ->data ) ) {
							free( pProxy->pTagString );
							pProxy->pTagString = NULL;
						}
					}
					if( (! pProxy->pTagString) && pQ->data[0] ) pProxy->pTagString = strdup( pQ->data );
				}

				pA->statusCode = SIMS_OK;
			} else {
				pA->statusCode = SIMS_DENY;
			}

		} else { // anonymous proxy

			ProxyMap::iterator iter = m_mapPureProxy.find( req->channel.peer.ip );
			if( iter != m_mapPureProxy.end() ) {
				BBQProxyRecord * pProxy = iter->second;

				// update if different
				if( pProxy->pTagString ) {
					if( 0 != strcmp( pProxy->pTagString, pQ->data ) ) {
						free( pProxy->pTagString );
						pProxy->pTagString = NULL;
					}
				}
				if( (! pProxy->pTagString) && pQ->data[0] ) pProxy->pTagString = strdup( pQ->data );

				pA->statusCode = SIMS_OK;
			} else {
				pA->statusCode = SIMS_DENY;
			}
		}

	} else {
		SFIDRecord * pRec = FindRecordByID( pQ->sessionInfo.id );
		if( pRec && (pRec->sessionInfo.cookie == pQ->sessionInfo.cookie) ) {
			VFONID vid;
			vid.serverid = m_config.domainInfo.dwId;
			vid.userid = pQ->sessionInfo.id;

			BBQDataBlock bb;
			bb.data_len = pQ->data_len;
			bb.data = (char *) & pQ->data[0];
			if( m_pDB->WriteClientPersonalData( vid, pQ->key, pQ->digest, & bb ) ) {
				pA->statusCode = SIMS_OK;
			} else {
				pA->statusCode = SIMS_SERVERERROR;
			}
		} else {
			pA->statusCode = SIMS_DENY;
		}
	}

	SendMessage( & reply );

	return true;
}

bool BBQServer::OnMsgClientDownloadDataBlock( const SIM_REQUEST * req )
{
	const SIMD_DATABLOCK * pQ = (const SIMD_DATABLOCK *) & req->msg.simData;

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_DOWNLOADDATABLOCK, sizeof(SIMD_DATABLOCK), req );
	SIMD_DATABLOCK * pA = (SIMD_DATABLOCK *) & reply.msg.simData;
	* pA = * pQ;
	pA->data_len = 0;

	SFIDRecord * pRec = FindRecordByID( pQ->sessionInfo.id );
	if( pRec && (pRec->sessionInfo.cookie == pQ->sessionInfo.cookie) ) {
		VFONID vid;
		vid.serverid = m_config.domainInfo.dwId;
		vid.userid = pQ->sessionInfo.id;

		BBQDataBlock bb;
		memset( & bb, 0, sizeof(bb) );
		if( m_pDB->ReadClientPersonalData( vid, pQ->key, pQ->digest, & bb ) ) {
			if( (bb.data_len > 0) && (bb.data != NULL ) ) {
				int nMsgBodySize = sizeof(SIMD_DATABLOCK) + bb.data_len;
				int nReqSize = SIM_DATASIZE_TO_REQSIZE( nMsgBodySize );
				SIM_REQUEST * r = NULL;
				SIM_REQUEST_NEW( r, nReqSize );
				if( r ) {
					SIM_REPLY_REQUEST( r, SIM_SC_DOWNLOADDATABLOCK, nMsgBodySize, req );
					pA = (SIMD_DATABLOCK *) & r->msg.simData;
					* pA = * pQ;

					pA->data_len = bb.data_len;
					memcpy( & pA->data[0], bb.data, bb.data_len );

					pA->statusCode = SIMS_OK;
					SendMessage( r ); 

					if( bb.data ) { delete[] bb.data; bb.data=NULL; }
					SIM_REQUEST_DELETE( r );
					return true;
				}
			}
			if( bb.data ) { delete[] bb.data; bb.data=NULL; }
			pA->statusCode = SIMS_OK;

		} else {
			pA->statusCode = SIMS_SERVERERROR;
		}
	} else {
		pA->statusCode = SIMS_DENY;
	}

	SendMessage( & reply );

	return true;
}

bool BBQServer::OnMsgClientUserMsg( const SIM_REQUEST * req )
{
	const SIMD_USERMSG * pQ = (const SIMD_USERMSG *) & req->msg.simData;

	SIM_REQUEST * tmp = NULL;
	SIM_REQUEST_CLONE( tmp, req );
	if( ! tmp ) return true;

	SIMD_USERMSG * pIn = (SIMD_USERMSG *) & tmp->msg.simData;

	ReadLock safe( m_mutexRecord );

	// decrypt the message, if it is encrypted
	if( pQ->flags.encrypted ) {
		SFIDRecord * pFrom = FindRecordByID( pQ->sessionInfo.id );
		if( pFrom && (pFrom->sessionInfo.cookie == pQ->sessionInfo.cookie) ) {

			switch( pQ->flags.encrypt_method ) {
			case BBQ_ENC_AES128:
				bool bDone = AES_decrypt( (unsigned char *) & pQ->msgData[ pQ->nPlainMsgLength ], sizeof(time_t) + pQ->nEncryptedMsgLength, 
					(const unsigned char *) pFrom->password, (const unsigned char *) & pQ->msgData[0], 128,
					(unsigned char *) & pIn->msgData[ pQ->nPlainMsgLength ] );
				if( ! bDone ) {
					if( tmp ) SIM_REQUEST_DELETE( tmp );
					return true;
				}
				break;
			}

		}
	}

	if( (pQ->toId.serverid == 0) || (pQ->toId.serverid == m_config.domainInfo.dwId) ) {
		SFIDRecord * pTo = FindRecordByID( pQ->toId.userid );
		if( pTo ) {
			time_t now = time(NULL);
			if( pTo->sessionInfo.cookie && (pTo->time + m_config.heartbeat_timeout > now) ) { // local online, so forward directly
				// TODO: forward the data to client
				SIM_REQUEST * f = NULL;
				SIM_REQUEST_CLONE( f, tmp );
				if( f ) {
					bool bDone = true;

					// encrypted before sending if needed
					SIMD_USERMSG * pOut = (SIMD_USERMSG *) & f->msg.simData;
					if( pQ->flags.encrypted ) {
						switch( pQ->flags.encrypt_method ) {
						case BBQ_ENC_AES128:
							bool bSucess = AES_encrypt( (unsigned char *) & pIn->msgData[ pIn->nPlainMsgLength ], sizeof(time_t) + pIn->nEncryptedMsgLength, 
								(const unsigned char *) pTo->password, (const unsigned char *) & pIn->msgData[0], 128,
								(unsigned char *) & pOut->msgData[ pQ->nPlainMsgLength ] );
							if( ! bSucess ) {
								bDone = false;
							}
							break;
						}
					}

					if( bDone ) {
						f->channel = pTo->sockChannel;
						SendMessage( f );
					}

					SIM_REQUEST_DELETE( f );
				}


			} else { // not online locally right now
				if( m_config.nServerType == BBQServer::CLUSTER ) { // clustered
					if ( m_status.nUplinkConnected ) { // 
					} else {
					}
				} else { // not clustered, standalone server, save to DB or ?
				}
			}
		} else { // no such user
		}
	} else { // not a local user, forward to uplinked server

	}

	if( tmp ) SIM_REQUEST_DELETE( tmp );
	return true;
}

int BBQServer::SendTextMessageToClient( uint32 nCallerID, uint32 nMsgID, const char * lpszURLs )
{
	int nLen = 0;
	if( lpszURLs == NULL || (nLen = strlen( lpszURLs )) >= TEXT_MESSAGE_MAX_LENGTH ) return SIMS_BADREQUEST;

	ReadLock safe( m_mutexRecord );

	SFIDRecord * pRec = FindRecordByID( nCallerID );
	if( pRec ) {
		time_t now = time(NULL);
		bool bOnline = ( (pRec->sessionInfo.cookie != 0) && (pRec->time + m_config.heartbeat_timeout > now) );
		if( ! bOnline ) {
			return SIMS_TIMEOUT;
		}

		SIM_REQUEST req;
		SIM_REQINITCHAN( req, pRec->sockChannel, nMsgID, nLen+1 );
		char * pQ = (char *) & req.msg.simData[0];
		memcpy( pQ, lpszURLs, nLen );
		pQ[nLen] = '\0';

		if( SendMessage( & req, true ) ) return SIMS_OK;
		else return SIMS_TIMEOUT;
	}

	return SIMS_NOTFOUND;
}

int BBQServer::BroadcastTextMessageToClients( const char * lpszText )
{
	int nLen = 0;
	if( lpszText == NULL || (nLen = strlen( lpszText )) >= TEXT_MESSAGE_MAX_LENGTH ) return SIMS_BADREQUEST;

	ReadLock safe( m_mutexRecord );
	time_t now = time(NULL);

	SIM_REQUEST req;
	SIM_REQINIT( req, 0, 0, 0, 0, 0, SIM_SC_BROADCASTTEXT, nLen+1 );
	char * pQ = (char *) & req.msg.simData[0];
	memcpy( pQ, lpszText, nLen );
	pQ[nLen] = '\0';

	for( RecordMap::iterator Iter = m_mapRecord.begin(), eIter = m_mapRecord.end(); Iter != eIter; Iter++ ) {
		SFIDRecord * pRec = Iter->second;
		bool bOnline = ( (pRec->sessionInfo.cookie != 0) && (pRec->time + m_config.heartbeat_timeout > now) );
		if( ! bOnline ) continue;

		req.channel = pRec->sockChannel;
		SendMessage( & req );
	}

	return SIMS_OK;
}

bool BBQServer::GetClientVersionInfo( SIMD_VERSIONINFO* pVerInfo, PString & strDownloadURL )
{
	pVerInfo->client = m_VersionInfo.client;

	pVerInfo->server = m_VersionInfo.server;

	pVerInfo->server.nowTime = time(NULL);
	pVerInfo->server.upTime = m_status.tUpTime;

	pVerInfo->server.userMax = m_status.nMaxUserCount;
	pVerInfo->server.userCount = m_status.nOnlineUsers;

	SIMD_SERVER_FEATURES * pF = & pVerInfo->server.features;

	pF->allow_cug_call_public = m_config.allow_cug_call_public;
	pF->disable_subscribe = m_config.disable_subscribe;
	pF->cug_have_private_buddy = m_config.cug_have_private_buddy;
	pF->enable_cug = m_config.enable_cug;
	pF->run_in_asp_mode = m_config.run_in_asp_mode;

	strDownloadURL = m_strNewClientDownloadURL;

	return true;
}

bool BBQServer::SetClientVersionInfo( const SIMD_VERSIONINFO* pVerInfo, PString & strDownloadURL )
{
	m_VersionInfo.client = pVerInfo->client;

	m_config.requiredVersion = CLIENT_VERCODE( m_VersionInfo.client.majorRequired, m_VersionInfo.client.minorRequired, m_VersionInfo.client.buildRequired );

	m_strNewClientDownloadURL = strDownloadURL;

	PConfig cfg( KEY_Client );

	//cfg.SetInteger( KEY_VersionMajor,m_VersionInfo.client.majorRequired );
	//cfg.SetInteger( KEY_VersionMinor, m_VersionInfo.client.minorRequired );
	//cfg.SetInteger( KEY_VersionBuild, m_VersionInfo.client.buildRequired );

	//cfg.SetInteger( KEY_VersionNewMajor, m_VersionInfo.client.majorNew );
	//cfg.SetInteger( KEY_VersionNewMinor, m_VersionInfo.client.minorNew );
	//cfg.SetInteger( KEY_VersionNewBuild, m_VersionInfo.client.buildNew );

	//cfg.SetString( KEY_DownloadURL, m_strNewClientDownloadURL );

	return true;
}

// ------ compartible only for version before 2004.06.07 -------------------
bool BBQServer::OnMsgClientQueryOldVersionInfo( const SIM_REQUEST * req )
{
	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_OLDVERSIONINFO, 0, req );

	SIMD_OLDVERSIONINFO inf;
	memset( & inf, 0, sizeof(inf) );

	inf.server.major = m_VersionInfo.server.major;
	inf.server.minor = m_VersionInfo.server.minor;
	inf.server.build = m_VersionInfo.server.build;
	inf.server.upTime = m_VersionInfo.server.upTime;
	inf.server.nowTime = time(NULL);
	inf.server.userMax = m_config.nUserMax; //m_VersionInfo.server.userMax;
	inf.server.userCount = m_status.nOnlineUsers; //m_VersionInfo.server.userCount;

	inf.client.major = m_VersionInfo.client.majorNew;
	inf.client.minor = m_VersionInfo.client.minorNew;
	inf.client.build = m_VersionInfo.client.buildNew;

	const char * lpszURL = (const char *) m_strNewClientDownloadURL;

	PBytePack packedData, packedMsg;
	packedData << inf << lpszURL;

	reply.msg.simHeader.size = packedData.Size();
	packedMsg << reply.channel << reply.msg.simHeader << packedData;

	SendMessage( (SIM_REQUEST *) packedMsg.Data() );

	return true;
}
// ------ compartible end --------------------------------------------------

bool BBQServer::OnMsgClientQueryVersionInfo( const SIM_REQUEST * req )
{
	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_VERSIONINFO, 0, req );

	m_VersionInfo.server.nowTime = time(NULL);
	m_VersionInfo.server.userMax = m_config.nUserMax; //m_VersionInfo.server.userMax;
	m_VersionInfo.server.userCount = m_status.nOnlineUsers * m_config.nUserParamA + m_config.nUserParamB; //m_VersionInfo.server.userCount;

	SIMD_SERVER_FEATURES * pF = & m_VersionInfo.server.features;

	pF->allow_cug_call_public = m_config.allow_cug_call_public;
	pF->disable_subscribe = m_config.disable_subscribe;
	pF->cug_have_private_buddy = m_config.cug_have_private_buddy;
	pF->enable_cug = m_config.enable_cug;
	pF->run_in_asp_mode = m_config.run_in_asp_mode;

	const char * lpszURL = (const char *) m_strNewClientDownloadURL;

	PBytePack packedData, packedMsg;
	packedData << m_VersionInfo << lpszURL;

	reply.msg.simHeader.size = packedData.Size();
	packedMsg << reply.channel << reply.msg.simHeader << packedData;

	SendMessage( (SIM_REQUEST *) packedMsg.Data() );

	return true;
}

bool BBQServer::Kick( uint32 uid )
{
  ReadLock safe( m_mutexRecord );

	SFIDRecord * pRec = FindRecordByID( uid );
	if( pRec ) {
		if( pRec->sessionInfo.cookie ) {
	    SIM_REQUEST kick;
	    SIM_REQINITCHAN( kick, pRec->sockChannel, SIM_NONE, 0 );
	    SendMessage( & kick );

			// clear its online cookie
			pRec->sessionInfo.cookie = 0;

			// close the connection
			int fdTcp = pRec->sockChannel.socket;
			if( fdTcp ) closesocket( fdTcp );
		}

		return true;
	}

	return false;
}

bool BBQServer::NotifyUserServiceChanged( uint32 uid )
{
	ReadLock safe( m_mutexRecord );
	SFIDRecord * pRec = FindRecordByID( uid );

	if( pRec ) {
		NotifyUserServiceChanged( pRec );
		return true;
	}

	return false;
}

int BBQServer::NotifyUserServiceChangedToAll( void )
{
	ReadLock safe( m_mutexRecord );

	time_t tNow = time(NULL);
	int n = 0;
	for( RecordMap::iterator Iter = m_mapRecord.begin(), eIter = m_mapRecord.end(); Iter != eIter; Iter ++ ) {
		SFIDRecord * pRec = Iter->second;

		if( pRec && pRec->sessionInfo.cookie && ( tNow < pRec->time + m_config.heartbeat_timeout ) ) {
			NotifyUserServiceChanged( pRec );
			n ++;
		}
	}

	return n;
}

bool BBQServer::UpdateAccessFlags( SFIDRecord * pRec )
{
	if( ! pRec ) return false;

	//if( pRec->flags.max_peers_per_conf ) {
	//	pRec->flags.max_peers_per_conf = min( pRec->flags.max_peers_per_conf, (m_config.nMaxPersonPerConference -1) & 0x0f );
	//} else {
	//	pRec->flags.max_peers_per_conf = (m_config.nMaxPersonPerConference -1) & 0x0f;
	//}

	if ( m_config.prepaid_policy 
		&& pRec->pServiceInfo 
		&& pRec->pServiceInfo->points > 0 ) { // has points, video is allowed
		if( m_config.use_customized_flag_for_normal ) {
			// skip modify

			pRec->flags.allow_proxy = 1;
		} else {
			if( ! pRec->flags.ignore_rule_and_use_as_it_is ) pRec->flags = m_config.flagsPrepaid;

			//pRec->flags.allow_h323gk = 1;
			pRec->flags.allow_proxy = 1;
			pRec->flags.allow_video = 1;
			pRec->flags.max_video_size = 0; // unlimited

			if( pRec->pServiceInfo->billing_status == BBQDatabase::ST_PRO ) {
				pRec->flags.pro_whiteboard = 
				pRec->flags.pro_appsharing = 
				pRec->flags.pro_sendfile = 1;
			}
		}
	} else {
		time_t now = time(NULL);
		bool bTrial = ( (pRec->serviceStart <= now) && (now <= pRec->serviceExpire) );

		if( bTrial ) {
			if( m_config.use_customized_flag_for_trial ) {
				// skip modify

				pRec->flags.allow_proxy = 1;
			} else {
				if( ! pRec->flags.ignore_rule_and_use_as_it_is ) pRec->flags = m_config.flagsTrial;
			}
		} else {
			if( m_config.use_customized_flag_for_free ) {
				// skip modify

				pRec->flags.allow_proxy = 1;
			} else {
				if( ! pRec->flags.ignore_rule_and_use_as_it_is ) pRec->flags = m_config.flagsFree;
			}
		}
	}

	bool bPublic = ( pRec && ( (! pRec->pParentIdList) || (pRec->pParentIdList->Count() == 0) ) && ( (! pRec->pChildIdList) || (pRec->pChildIdList->Count() == 0) ));

	if( bPublic ) pRec->flags.no_private_buddylist = 0;
	else if ( ! m_config.cug_have_private_buddy ) pRec->flags.no_private_buddylist = 1;
	else pRec->flags.no_private_buddylist = 0;

	return true;
}

bool BBQServer::NotifyUserServiceChanged( SFIDRecord * pRec, bool bUpdateGroup )
{
	time_t tNow = time(NULL);

	if( pRec && pRec->sessionInfo.cookie && ( tNow < pRec->time + m_config.heartbeat_timeout ) ) {

		uint32 statusCode =  SIMS_OK;

		BBQUserServiceInfo	serviceInfo;
		memset( & serviceInfo, 0, sizeof(serviceInfo) );

		UpdateAccessFlags( pRec );

		if ( pRec->pServiceInfo ) serviceInfo.points = pRec->pServiceInfo->points;

		serviceInfo.ip = pRec->sockChannel.peer.ip;
		serviceInfo.login_time = pRec->time;

		serviceInfo.provider = pRec->providerID;

		serviceInfo.type = pRec->serviceType;

		serviceInfo.start_time = pRec->serviceStart;
		time_t tExp = pRec->serviceStart + m_config.subscribe_trial_time * 3600 * 24;
		serviceInfo.expire_time = max( tExp, pRec->serviceExpire );

		serviceInfo.update_group = bUpdateGroup ? 1 : 0;

		serviceInfo.extFlags = pRec->pServiceInfo->extFlags;

		SIM_REQUEST req;
		SIM_REQINITCHAN( req, pRec->sockChannel, SIM_SC_QUERYSERVICE, 0 );

		PBytePack packedData, packedMsg;
		packedData << statusCode << pRec->flags << serviceInfo;

		req.msg.simHeader.size = packedData.Size();
		packedMsg << req.channel << req.msg.simHeader << packedData;

		SendMessage( (SIM_REQUEST *) packedMsg.Data() );

		return true;
	}

	return false;
}

bool BBQServer::OnMsgClientQueryService( const SIM_REQUEST * req )
{
	SIM_SESSIONINFO		sessionInfo;

	try {
		PBytePack packedData( & req->msg.simData, req->msg.simHeader.size );
		packedData >> sessionInfo;
	} catch ( const char * ) {
		// bad request
		return true;
	}

	{
		uint32 statusCode = 0;
		BBQACLFlag			aclFlag;
		BBQUserServiceInfo	serviceInfo;
		memset( & aclFlag, 0, sizeof(aclFlag) );
		memset( & serviceInfo, 0, sizeof(serviceInfo) );

		WriteLock safe( m_mutexRecord );
		SFIDRecord * pRec = FindRecordByID( sessionInfo.id );
		if( pRec && (pRec->sessionInfo.cookie == sessionInfo.cookie) ) {
			statusCode = SIMS_OK;

			bool bBillingCheck = false;

			// modify acl flags according to billing info
			BBQDatabase::BillingInfo binfo;
			memset( & binfo, 0, sizeof(binfo) );

			//VFONID id;
			//id.serverid = pRec->serverID;
			//id.userid = pRec->sessionInfo.id;
			//CATCH_DB_FAIL(
			//	bBillingCheck = m_pDB->CheckBillingStatus( id, & binfo );
			//);

			// ------------------------------------------------------------------
			// The first input parameter changed!
			// VFONID for previous, it's a BBQDatabase::BillingUserInfo now,
			// as we need a more parameter, the user bind type, while doing CheckBillingStatus
			// --- By HZW, 2005-03-23

			BBQDatabase::BillingUserInfo buserInfo;
			memset( & buserInfo, 0, sizeof(buserInfo) );
			buserInfo.vfonid.serverid = pRec->serverID;
			buserInfo.vfonid.userid = pRec->sessionInfo.id;
			if( pRec->pServiceInfo ) {
				strncpy( buserInfo.bindtype, pRec ->pServiceInfo ->bindInfo.bindType, USERDATA_NAME_SIZE);
			}
			CATCH_DB_FAIL(
				bBillingCheck = m_pDB->CheckBillingStatus( & buserInfo, & binfo );
			);
			// ------------------------------------------------------------------

			if( bBillingCheck ) {
				if( binfo.statusCode == BBQDatabase::ST_INVALID ) {
					binfo.points = 0;
				}
				if( pRec->pServiceInfo ) {
					pRec->pServiceInfo->billing_status = binfo.statusCode;
					pRec->pServiceInfo->points = binfo.points;
				}
			}

			UpdateAccessFlags( pRec );

			aclFlag = pRec->flags;

			serviceInfo.points = binfo.points;
			serviceInfo.ip = pRec->sockChannel.peer.ip;
			serviceInfo.login_time = pRec->time;
			serviceInfo.provider = pRec->providerID;

			serviceInfo.type = pRec->serviceType;
			serviceInfo.start_time = pRec->serviceStart;
			if( serviceInfo.type == SERVICE_FIXEDPCTRIAL ) {
				serviceInfo.expire_time = pRec->serviceStart + m_config.subscribe_trial_time * 3600 * 24;
				serviceInfo.expire_time = max( serviceInfo.expire_time, pRec->serviceExpire );
			} else {
				serviceInfo.expire_time = pRec->serviceExpire;
			}

		} else {
			statusCode = SIMS_DENY;
		}

		SIM_REQUEST reply;
		SIM_REPLY_REQUEST( & reply, SIM_SC_QUERYSERVICE, 0, req );

		PBytePack packedData, packedMsg;
		packedData << statusCode << aclFlag << serviceInfo;

		reply.msg.simHeader.size = packedData.Size();
		packedMsg << reply.channel << reply.msg.simHeader << packedData;

		SendMessage( (SIM_REQUEST *) packedMsg.Data() );
	}

	return true;
}

bool BBQServer::OnMsgClientQueryGKServerList( const SIM_REQUEST * req )
{
	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_QUERYGKLIST, 0, req );

	PBytePack packedData, packedMsg;

	PConfig cfg( KEY_GKServers);
	int nCount = cfg.GetInteger(KEY_GKServersCount);
	packedData << nCount;
	for(int i = 0; i < nCount; i++) {
		PString strKey(PString::Printf, "%s_%d", KEY_GKServer, i);
		PString strServer = cfg.GetString(strKey);
		PStringArray array = strServer.Tokenise(",");
		PString  strPort;
		if( array.GetSize() > 1 )  {
			strPort = array[1];
			strPort.Trim();
		}
		if( array.GetSize() > 0 )  {
			strServer = array[0];
		} else {
			strServer = "";
		}
		IPSOCKADDR ipAddr = {0, 0 };
		if( strPort.IsEmpty() ) {
			ipAddr.port = 1719;
		} else {
			ipAddr.port = (WORD) strPort.AsUnsigned();
		}
		if( strServer.IsEmpty() ) {
			ipAddr.ip = 0;
		} else {
			ipAddr.ip = ::inet_addr(strServer);
		}
		packedData << ipAddr.ip << ipAddr.port;
	}

	reply.msg.simHeader.size = packedData.Size();
	packedMsg << reply.channel << reply.msg.simHeader << packedData;

	SendMessage( (SIM_REQUEST *) packedMsg.Data() );

	return true;
}

bool BBQServer::OnMsgClientQueryPeoxyServerList( const SIM_REQUEST * req )
{
	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_QUERYPSLIST, 0, req );

	PBytePack packedData, packedMsg;

	PConfig cfg( KEY_ProxyServers);
	int nCount = cfg.GetInteger(KEY_ProxyServersCount);
	packedData << nCount;
	for(int i = 0; i < nCount; i++) {
		PString strKey(PString::Printf, "%s_%d", KEY_ProxyServer, i);
		PString strServer = cfg.GetString(strKey);
		PStringArray array = strServer.Tokenise(",");
		PString  strPort;
		if( array.GetSize() > 1 )  {
			strPort = array[1];
			strPort.Trim();
		}
		if( array.GetSize() > 0 )  {
			strServer = array[0];
		} else {
			strServer = "";
		}
		IPSOCKADDR ipAddr = {0, 0 };
		if( strPort.IsEmpty() ) {
			ipAddr.port = 1719;
		} else {
			ipAddr.port = (WORD) strPort.AsUnsigned();
		}
		if( strServer.IsEmpty() ) {
			ipAddr.ip = 0;
		} else {
			ipAddr.ip = ::inet_addr(strServer);
		}
		packedData << ipAddr.ip << ipAddr.port;
	}

	reply.msg.simHeader.size = packedData.Size();
	packedMsg << reply.channel << reply.msg.simHeader << packedData;

	SendMessage( (SIM_REQUEST *) packedMsg.Data() );

	return true;
}

bool BBQServer::OnMsgCreateProxyChannel( const SIM_REQUEST * req )
{
	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, UCM_PS_NOTIFY, sizeof(UCMD_PROXY_NOTIFY), req );

	const UCMD_PROXY_CREATE * pQ = (const UCMD_PROXY_CREATE *) & req->msg.simData;
	UCMD_PROXY_NOTIFY * pA = (UCMD_PROXY_NOTIFY *) & reply.msg.simData;

	// here, check session info for security
	// if( (pQ->sessionInfo.id != m_sessionInfo.id) || (pQ->sessionInfo.cookie != m_sessionInfo.cookie) ) return true;

	if( LOCALHOST_IP != req->channel.peer.ip ) {

		PTRACE( 1, "CreateProxyChannel request coming, but not from local, shall we deny it?" );

	}

	* pA = * pQ;

	BBQProxy::Entry * pEA = NULL, * pEB = NULL;
	BBQProxy * pProxy = BBQProxy::GetCurrentProxy();
	if( pProxy && 
		pProxy->CreateChannel( pQ->entry[0].uid, pQ->entry[1].uid, (pQ->channelInfo.tcp != 0), pEA, pEB ) && 
		pEA && pEB ) {
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
		pA->statusCode = SIMS_NOTAVAILABLE;
	}

	SendMessage( & reply );

	return true;
}

bool BBQServer::OnMsgReleaseProxyChannel( const SIM_REQUEST * req )
{
	const UCMD_PROXY_RELEASE * pQ = (const UCMD_PROXY_RELEASE *) & req->msg.simData;

	// here, check session info for security
	// if( (pQ->sessionInfo.id != m_sessionInfo.id) || (pQ->sessionInfo.cookie != m_sessionInfo.cookie) ) return true;

	BBQProxy * pProxy = BBQProxy::GetCurrentProxy();
	if( pProxy ) {
		pProxy->CutChannel( pQ->nChannelId );
	}

	return true;
}

bool BBQServer::OnMsgProxyNotify( const SIM_REQUEST * req )
{
	const UCMD_PROXY_NOTIFY * pQ = (const UCMD_PROXY_NOTIFY *) & req->msg.simData;

	if( m_config.write_useraction_log ) {
		PString strMsg( PString::Printf, "proxy notify for %d and %d: %s.", pQ->entry[0].uid, pQ->entry[1].uid, SIM_STATUSCODENAME(pQ->statusCode) );
		m_UserActionLog.Output( PLog::Debug, strMsg );
	}

	if( pQ->statusCode != SIMS_OK ) return true;

	WriteLock lock( m_mutexRecord );

	// add score of the proxy
	BBQProxyRecord * pProxy = NULL;

	if( pQ->sessionInfo.id ) pProxy = FindProxyById( pQ->sessionInfo.id );
	else pProxy = FindPureProxyByIp( req->channel.peer.ip );

	if( pProxy ) {
		pProxy->score ++;

		if( pQ->proxyMax ) {
			pProxy->proxyBandwidthTotal = pQ->proxyBandwidthTotal;
			pProxy->proxyBandwidthUsed = pQ->proxyBandwidthUsed;

			pProxy->proxyMax = pQ->proxyMax;
			pProxy->proxySlot = pQ->proxySlot;
		}
	}

	SIM_REQUEST msg;
	SIM_REQINIT( msg, 0, req->channel.local.ip, req->channel.local.port, 0, 0, UCM_SC_PROXYNOTIFY, sizeof(UCMD_PROXY_NOTIFYCLIENT) );
	UCMD_PROXY_NOTIFYCLIENT * pC = (UCMD_PROXY_NOTIFYCLIENT *) & msg.msg.simData;

	* pC = * pQ;

	if( pC->entry[0].addr.ip == 0 ) {
		if( req->channel.peer.ip == m_config.dwListenIp ) {
			pC->entry[0].addr.ip = pC->entry[1].addr.ip = LOCALHOST_IP;
		} else {
			pC->entry[0].addr.ip = pC->entry[1].addr.ip = req->channel.peer.ip;
		}
	}

	//if( LOCALHOST_IP == req->channel.peer.ip ) {
	//	if( ! IS_PRIVATE_IP(m_config.thisServerAddress.ip) ) {
	//		pC->entry[0].addr.ip = pC->entry[1].addr.ip = m_config.thisServerAddress.ip;
	//	}
	//} else {
	//	pC->entry[0].addr.ip = pC->entry[1].addr.ip = req->channel.peer.ip;
	//}

	PString strMsg( PString::Printf, "proxy feedback okay, now send proxy ip info (%s) to both clients.", (const char *) ToString( pC->entry[0].addr.ip ) );
	m_UserActionLog.Output( PLog::Info, strMsg );

	SFIDRecord * pRec = NULL;
	
	if( pQ->callerChannel.local.port != 0 ) {
		msg.channel = pQ->callerChannel;
	} else {
		// send to the client that channel request from
		pRec = FindRecordByID( pC->entry[0].uid );
		if( pRec ) {
			msg.channel = pRec->sockChannel;
		} else {
			msg.channel.peer = pC->channelInfo.peerWAN;
		}
	}

	pC->channelInfo.init = 1;

	int n = msg.channel.socket ? 1 : 3; // if udp, send 3 packet to avoid packet loss
//	int n = msg.channel.socket ? 1 : 1;
// TODO: send 3 packets in future, but now cannot, because old client cannot handle it
#ifdef ANTI_PACKET_LOSS
	for(int i=0; i<n; i++) { SendMessage( & msg ); sleep_ms(0); }
#else
	SendMessage( & msg );
#endif

	// send to the client that channel request to
	pRec = FindRecordByID( pC->entry[1].uid );
	if( pRec ) {
		msg.channel = pRec->sockChannel;
	} else {
		msg.channel.peer = pC->channelInfo.thisWAN;
	}
	pC->channelInfo.init = 0;

	n = msg.channel.socket ? 1 : 3; // if udp, send 3 packet to avoid packet loss
//	int n = msg.channel.socket ? 1 : 1;
// TODO: send 3 packets in future, but now cannot, because old client cannot handle it
#ifdef ANTI_PACKET_LOSS
	for(int i=0; i<n; i++) { SendMessage( & msg ); sleep_ms(0); }
#else
	SendMessage( & msg );
#endif

	return true;
}

/* 
input information and subscribe vfon service
Input info:
 (1) user info;
 (2) password;
 (3) hardware id;
 (4) cd-key if available;
 (5) etc.
Output info:
 (1) new Id;
 (2) free trial time;
*/
bool BBQServer::OnMsgClientSubscribe( const SIM_REQUEST * req )
{
	const SIMD_CS_SUBSCRIBE * pQ = (const SIMD_CS_SUBSCRIBE *) & req->msg.simData;

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_SUBSCRIBE, sizeof(SIMD_SC_SUBSCRIBE), req );
	SIMD_SC_SUBSCRIBE * pA = (SIMD_SC_SUBSCRIBE *) & reply.msg.simData;
	memset( pA, 0, sizeof(*pA) );

	if( m_bDisableFeature ) { // if no dongle or license expired, do not allow user login
		pA->statusCode = SIMS_SERVERERROR;
		SendMessage( & reply );
		return true;

	} if( m_config.disable_subscribe ) {
		pA->statusCode = SIMS_DENY;
		SendMessage( & reply );
		return true;
	}

	// make usre hardware id here is in valid length
	int nLen = 0;
	for( int i=0; i<USERDATA_NAME_SIZE; i++ ) {
		if( pQ->hardwareId[i] == '\0' ) { 
			nLen = i;
			break;
		}
	}
	if( nLen >= USERDATA_NAME_SIZE ) {
		pA->statusCode = SIMS_BADREQUEST;
		SendMessage( & reply );
		return true;
	}
	if( nLen <= 0 ) {
		if( ! m_config.allow_subscribe_more_id_per_pc ) {
			pA->statusCode = SIMS_BADREQUEST;
			SendMessage( & reply );
			return true;
		}
	}

	BBQUserFullRecord rec;
	memset( & rec, 0, sizeof(rec) );

	if( ! m_config.allow_subscribe_more_id_per_pc ) {
		bool bRecordRead = false;
		CATCH_DB_FAIL(
			bRecordRead = m_pDB->ReadByKey( & rec, pQ->hardwareId, BBQDatabase::BY_HARDWAREID );
		);

		if( bRecordRead ) {
			if( m_config.write_useraction_log ) {
				PString strMsg( PString::Printf, "User subscribe from %s, found same PC already subscribed id: %d, ignore.", (const char *) ToString( & req->channel.peer ), rec.uid );
				m_UserActionLog.Output( PLog::Info, strMsg );
			}

			// the hardware id already exists in our database
			pA->statusCode = SIMS_EXISTS;
		}
	}

	if( rec.uid == 0 ) {	// if no record loaded

		rec.userInfo = pQ->userInfo;

		if( ! m_config.allow_subscribe_more_id_per_pc ) strcpy( rec.service.hardwareID, pQ->hardwareId );

		if( strlen(pQ->cdKey) > 0 ) strcpy( rec.service.product_key, pQ->cdKey );

		rec.service.provider = pQ->providerID;

		rec.service.type = SERVICE_PAY;
		rec.service.start_time = time(NULL);
		rec.service.expire_time = rec.service.start_time + m_config.subscribe_trial_time * 3600 * 24;

		if( rec.userInfo.security.modify ) {
			memcpy( rec.userInfo.security.oldpass, rec.userInfo.security.newpass, USERDATA_NAME_SIZE );
		}

		BBQACLFLAG_SET( rec.service.flags, m_config.flagsDefault.value ); 
		rec.service.serverID = 0;
		rec.uid = 0;

		CATCH_DB_FAIL(
			rec.uid = m_pDB->CreateUser( & rec );
			if( rec.uid != 0 ) {
				SFIDRecord * pRec = InsertRecordIntoMemoryCache( & rec );

				// new created node, no link yet.

				UpdateAccessFlags( pRec );

				// when subscribe, reset the load timestamp to zero, 
				// so that force data reload when first time login, to avoid data mismatch.
				pRec->load_timestamp = 0;
				pRec->binfo_load_timestamp = 0;

				pA->statusCode = SIMS_OK;
			} else {
				PString strMsg( PString::Printf, "User subscribe from %s, but server failed to create user record in DB.", (const char *) ToString( & req->channel.peer ) );
				PLOG( m_pLogger, Debug, strMsg );
			}
		);
	}

	if( rec.uid != 0 ) {	// if no record loaded or created

		pA->vfonId.serverid = rec.service.serverID;
		pA->vfonId.userid = rec.uid;

		pA->tNowServerTime = time(NULL);
		pA->tStartTime = rec.service.start_time;
		pA->tExpireTime = rec.service.expire_time;

		pA->nPoints = rec.service.points;

		if( m_config.write_useraction_log ) {
			PString strMsg( PString::Printf, "User subscribe from %s, assign id: %d.", (const char *) ToString( & req->channel.peer ), rec.uid );
			m_UserActionLog.Output( PLog::Info, strMsg );
		}

	} else {
		// database create user failed.
		pA->statusCode = SIMS_DENY;
	}

	SendMessage( & reply );

	return true;
}

/*
Input info:
  (1) Id, password, hardware id;
  (2) Bind id, bind password;
  (3) etc.
Output info:
  (1) Status code, OK, or reason code;
  (2) SessionInfo: id & cookie;
  (3) etc.
*/
bool BBQServer::OnMsgClientLoginExSecure( const SIM_REQUEST * req )
{
	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_LOGIN_EX, sizeof(SIMD_SC_LOGIN_EX), req );
	SIMD_SC_LOGIN_EX * pA = (SIMD_SC_LOGIN_EX *) & reply.msg.simData;
	memset( pA, 0, sizeof(*pA) );

	if( m_bDisableFeature ) { // if no dongle or license expired, do not allow user login

		pA->statusCode = SIMS_SERVERERROR;
		SendMessage( & reply );

		return true;
	} else if ( m_status.nServerStatus != ACTIVE ) { // if server is standby or down, refuse login, and suggest client to another server

		pA->statusCode = SIMS_NOTAVAILABLE;
		pA->anotherServer = m_config.anotherServerAddress;
		SendMessage( & reply );

		return true;
	}

	const SIMD_SECRET * pSecret = (const SIMD_SECRET *) & req->msg.simData;
	if( pSecret->encrypted_data_size > 256 || pSecret->secret_size > 512 ) { // secret too large, must be sth wrong
		pA->statusCode = SIMS_BADREQUEST;
		SendMessage( & reply );
		return true;
	}

	char plain_key[ 512 ];	// now decypted secret key
	switch( pSecret->encrypt_method ) {
	case SECRET_PLAIN:
		memcpy( plain_key, pSecret->encrypted_data, pSecret->secret_size );
		break;
	case SECRET_RSA:
		if( m_pPKIPrivKey ) {
			int dout = PKI_rsa_private_decrypt( m_pPKIPrivKey, pSecret->encrypted_data, pSecret->encrypted_data_size, plain_key );
			if( dout != pSecret->secret_size ) {
				pA->statusCode = SIMS_BADREQUEST;
				SendMessage( & reply );
				return true;
			}
		} else {
			pA->statusCode = SIMS_NOTAVAILABLE;
			SendMessage( & reply );
			return true;
		}
		break;
	case SECRET_AES128:
	case SECRET_AES256:
	default:
		pA->statusCode = SIMS_NOTIMPLEMENTED;
		SendMessage( & reply );
		return true;
	}

	uint32 nSecretSize = sizeof(SIMD_SECRET) + pSecret->encrypted_data_size;
	uint32 nOffset = nSecretSize + sizeof(uint32);
	const uint32 * pLen = (const uint32 *) & req->msg.simData[ nSecretSize ];
	const SIMD_CS_LOGIN_EX * pQ_encrypted = (const SIMD_CS_LOGIN_EX *) & req->msg.simData[ nOffset ];

	if( * pLen != sizeof(SIMD_CS_LOGIN_EX) ) {
		pA->statusCode = SIMS_BADREQUEST;
		SendMessage( & reply );
		return true;
	}

	SIMD_CS_LOGIN_EX plain_login_ex;	// now decrypt login request 
	switch( pSecret->secret_purpose ) {
	case SECRET_PLAIN:
		memcpy( & plain_login_ex, pQ_encrypted, sizeof(SIMD_CS_LOGIN_EX) );
		break;
	case SECRET_AES128:
		if( (pSecret->secret_size == 16) && 
			AES_decrypt( (unsigned char *) pQ_encrypted, sizeof(SIMD_CS_LOGIN_EX), (unsigned char *) plain_key, (unsigned char *) PI_SQRT2_STRING, 128, (unsigned char *)  & plain_login_ex ) ) {
			// okay, now successfully decrypted

		} else {
			pA->statusCode = SIMS_BADREQUEST;
			SendMessage( & reply );
			return true;
		}
		break;
	case SECRET_AES256:
		if( (pSecret->secret_size == 32) && 
			AES_decrypt( (unsigned char *) pQ_encrypted, sizeof(SIMD_CS_LOGIN_EX), (unsigned char *) plain_key, (unsigned char *) PI_SQRT2_STRING, 256, (unsigned char *)  & plain_login_ex ) ) {
			// okay, now successfully decrypted

		} else {
			pA->statusCode = SIMS_BADREQUEST;
			SendMessage( & reply );
			return true;
		}
		break;
	case SECRET_RSA:
	default:
		pA->statusCode = SIMS_NOTIMPLEMENTED;
		SendMessage( & reply );
		return true;
	}

	return DoClientLoginEx( req, & plain_login_ex, reply, plain_key, pSecret->secret_size );
}

bool BBQServer::OnMsgClientLoginEx( const SIM_REQUEST * req )
{
	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_LOGIN_EX, sizeof(SIMD_SC_LOGIN_EX), req );
	SIMD_SC_LOGIN_EX * pA = (SIMD_SC_LOGIN_EX *) & reply.msg.simData;
	memset( pA, 0, sizeof(*pA) );
	
	if( m_bDisableFeature ) { // if no dongle or license expired, do not allow user login

		pA->statusCode = SIMS_SERVERERROR;
		SendMessage( & reply );

		return true;
	} else if ( m_status.nServerStatus != ACTIVE ) { // if server is standby or down, refuse login, and suggest client to another server

		pA->statusCode = SIMS_NOTAVAILABLE;
		pA->anotherServer = m_config.anotherServerAddress;
		SendMessage( & reply );

		return true;
	}

	const SIMD_CS_LOGIN_EX * pQ = (const SIMD_CS_LOGIN_EX *) & req->msg.simData;

	return DoClientLoginEx( req, pQ, reply );
}

bool BBQServer::DoClientLoginEx( const SIM_REQUEST * req, const SIMD_CS_LOGIN_EX * pQ, SIM_REQUEST & reply, char * secret_key, int key_size )
{
	SIMD_SC_LOGIN_EX * pA = (SIMD_SC_LOGIN_EX *) & reply.msg.simData;

	// before going on, check client version

	if ( pQ->onlineStatus == CLIENT_MCU ) {
		// mcu, do not check version

	} else if ( (strlen(this->m_strAcceptClientAppList) > 0) 
		&& (strstr(this->m_strAcceptClientAppList, pQ->clientApp) == NULL) ) { // check client app name, such as [MCU][VMEET][MHAVC] ...

		pA->statusCode = SIMS_DENY;
		pA->errCode = ERR_CLIENTAPP;
		SendMessage( & reply );
	} else if ( pQ->techParam.version < m_config.requiredVersion ) {

		pA->statusCode = SIMS_DENY;
		pA->errCode = ERR_VERSION;
		SendMessage( & reply );

		return true;
	} 
	SFIDRecord * pRec = NULL;

	BBQUserFullRecord rec;
	memset( & rec, 0, sizeof(rec) );

	BBQDatabase::UserAuthInfo authInfo;
	memset( & authInfo, 0, sizeof(authInfo) );

	BBQDatabase::UserAuthResult authResult;
	memset( & authResult, 0, sizeof(authResult) );

	BBQDatabase::BillingInfo binfo;
	memset( & binfo, 0, sizeof(binfo) );

	bool bAuthFromGateway = false;
	bool bAuthenticated = false;
	bool bRead = false;

	if( pQ->vfonId.userid == 0 ) {
		if( m_pDB ) {
			if( m_pDB->IsAuthGatewayReady() ) {

			} else { // if AuthGW is not ready, then try using the data in DB
				VFONID vid; vid.value = 0;
				if( m_pDB->GetIdByBindInfo( & pQ->bindInfo, & vid ) ) {

				}
			}
		}
	}

#define		ONE_MIN_IN_MS			60000			// default DB cache time

	if( pQ->vfonId.userid ) {

		// to avoid login attack, before we load from database, we check from memory cache, 
		// it will be auto reloaded if not found in memory.
		pRec = FindRecordByID( pQ->vfonId.userid, false );	

		MS_TIME msNow = GetHostUpTimeInMs();

		if( pRec ) {
			// if the record is loaded some time ago, it might be out of date, we force it reloaded
			if( msNow > pRec->load_timestamp + m_config.nDbCacheTime ) {
				/*pRec = */
        //mod by chenyuan begin
        SFIDRecord *pNew =NULL;
				pNew = FindRecordByID( pQ->vfonId.userid, true );
        if (!pNew )
        {
		      pA->statusCode = SIMS_NOTFOUND;
		      SendMessage( & reply );
          return false;
        }
        else
        {
          if (pRec != pNew)
          {
            pRec = pNew;
            PString strMsg( PString::Printf, "SFID cache was out of date. (%d)\n.",  pQ->vfonId.userid );
            PLOG( m_pLogger, Info, "warning:  " + strMsg );
          }
        }
        //mod by chenyuan end
				pRec->load_timestamp = msNow;
			}

			// also update the record of company id if needed
			if( pRec->pServiceInfo && (pRec->pServiceInfo->company_id != 0) ) {
				SFIDRecord * pCom = FindRecordByID( pRec->pServiceInfo->company_id, false );
				if( pCom ) {
					if( msNow - pCom->load_timestamp > m_config.nDbCacheTime ) {
						/*pRec = */
						FindRecordByID( pRec->pServiceInfo->company_id, true );
						pCom->load_timestamp = msNow;
					}
				}
			}

			if( msNow - pRec->binfo_load_timestamp > m_config.nDbCacheTime ) {

				CATCH_DB_FAIL(

					//VFONID id;
					//id.serverid = pRec->serverID;
					//id.userid = pRec->sessionInfo.id;
					//if( ! m_pDB->CheckBillingStatus( id, & binfo ) ) binfo.statusCode = BBQDatabase::ST_INVALID;

					// ------------------------------------------------------------------
					// The first input parameter changed!
					// VFONID for previous, it's a BBQDatabase::BillingUserInfo now,
					// as we need a more parameter, the user bind type, while doing CheckBillingStatus
					// --- By HZW, 2005-03-23
					BBQDatabase::BillingUserInfo buserInfo;
					memset( & buserInfo, 0, sizeof(buserInfo) );
					buserInfo.vfonid.serverid = pRec->serverID;
					buserInfo.vfonid.userid = pRec->sessionInfo.id;
					if( pRec->pServiceInfo ) {
						strncpy( buserInfo.bindtype, pRec ->pServiceInfo ->bindInfo.bindType, USERDATA_NAME_SIZE);
					}
					if( ! m_pDB->CheckBillingStatus( & buserInfo, & binfo ) ) binfo.statusCode = BBQDatabase::ST_INVALID;
					// ------------------------------------------------------------------

					// cache the billing info
					if( pRec->pServiceInfo ) {
						pRec->pServiceInfo->billing_status = binfo.statusCode;
						pRec->pServiceInfo->points = binfo.points;
					}
				);

				pRec->binfo_load_timestamp = msNow;

			} else {
				// read billing info from cache
				if ( pRec->pServiceInfo ) {				
					binfo.statusCode = pRec->pServiceInfo->billing_status;
					binfo.points = pRec->pServiceInfo->points;
				}
			}
		}

		if( pRec ) {
			bRead = true;
			bAuthenticated = ( 0 == memcmp(pQ->lpszPassword, pRec->password, USERDATA_NAME_SIZE ) );
		}

	} else if ( strlen(pQ->bindInfo.bindType) != 0 ) {

		// if ( m_pDB && m_pDB->IsAuthGatewayReady() ) { // AuthGW is ready

		// authenticate at third-party authentication gateway here, for example, PCCW
		strcpy( authInfo.uid, pQ->bindInfo.bindId );
		strcpy( authInfo.passwd, pQ->bindInfo.bindPass );
		strcpy( authInfo.bindtype, pQ->bindInfo.bindType );

		{
		CATCH_DB_FAIL(
			PTRACE( 1, "authenticating user via gateway... " );
			MS_TIME msStart = GetHostUpTimeInMs();
			bAuthFromGateway = m_pDB->Authenticate( & authInfo, & authResult ) && (authResult.status == BBQDatabase::AUTH_SUCCEED);
			// ---------------------------------------------
			// expect result:
			// ---------------------------------------------
			//bAuthFromGateway = true;
			//authResult.status = BBQDatabase::AUTH_SUCCEED;
			//authResult.type = BBQDatabase::SERVICE_VMEET;
			// ---------------------------------------------
			MS_TIME msEnd = GetHostUpTimeInMs();
			DWORD t = (DWORD)(msEnd - msStart);
			PString strMsg( PString::Printf, "time used for auth gateway: %d ms.", t );
			if( t > 5000 ) {
				PLOG( m_pLogger, Info, "warning: Too much " + strMsg );
			} else {
				PLOG( m_pLogger, Debug, strMsg );
			}
		);
		}

		// write log info of auth result
		PLOG( m_pLogger, Info, PString( PString::Printf, "user %s authenticated %s from gateway.", (bAuthFromGateway ? "okay" : "failed"), authInfo.uid ) );

		if( bAuthFromGateway ) {

			if( authResult.flag_balance ) {
				binfo.statusCode = (authResult.type == BBQDatabase::SERVICE_VMEET) ? BBQDatabase::ST_PRO : BBQDatabase::ST_BASIC;
				binfo.points = (int)(authResult.balance * 100);

				// cache the billing info
				if( pRec->pServiceInfo ) {
					pRec->pServiceInfo->billing_status = binfo.statusCode;
					pRec->pServiceInfo->points = binfo.points;
				}
			}

			if( authResult.flag_dayoftrial ) {
				pRec->pServiceInfo->expire_time = time(NULL) + 3600 * 24 * authResult.dayoftrial;
			}

			// search the db for the user with specified id
			CATCH_DB_FAIL(
				// the search will ignore type, this will allow PCCW_xxx call each other, but pls make sure the bindId is unique
				const char * bindTypeForSearch = ((strstr(pQ->bindInfo.bindType, "PCCW_") != NULL) ? "" : pQ->bindInfo.bindType);
				bRead = m_pDB->ReadByKey( & rec, bindTypeForSearch, pQ->bindInfo.bindId );
			);

			bool bBindUserCreated = false;

			// check bind type
			bool bMatchBindType = false;
			int nBindTypes = m_strsBindTypes.GetSize();
			for( int j=0; j<nBindTypes; j++ ) {
				if( strstr( pQ->bindInfo.bindType, m_strsBindTypes[ j ] ) != NULL ) {
					bMatchBindType = true;
					break;
				}
			}

			if( bMatchBindType ) { 
				
				if( bRead ) {
					// already found the user in db, and loaded into rec.

					// if no balance returned by auth gateway, then try read it from DB
					if( ! authResult.flag_balance ) {
						BBQDatabase::BillingUserInfo buserInfo;
						memset( & buserInfo, 0, sizeof(buserInfo) );
						buserInfo.vfonid.serverid = pRec->serverID;
						buserInfo.vfonid.userid = pRec->sessionInfo.id;
						strncpy( buserInfo.bindtype, pQ->bindInfo.bindType, USERDATA_NAME_SIZE);
						if( ! m_pDB->CheckBillingStatus( & buserInfo, & binfo ) ) binfo.statusCode = BBQDatabase::ST_INVALID;
						// ------------------------------------------------------------------

						// cache the billing info
						if( pRec->pServiceInfo ) {
							pRec->pServiceInfo->billing_status = binfo.statusCode;
							pRec->pServiceInfo->points = binfo.points;
						}
					}
				} else {
					// user not found, create new record with bind type and bind id

					// copy user name to alias
					char nickname[ USERDATA_NAME_SIZE ];
					memcpy( nickname, pQ->bindInfo.bindId, USERDATA_NAME_SIZE );
					nickname[ USERDATA_NAME_SIZE - 1 ] = '\0';

					// if the id is an email address, then pick the name part
					for( int i=0; i<USERDATA_NAME_SIZE; i++ ) {
						if( '@' == nickname[i] ) {	
							nickname[i] = 0;
							break;
						}
					}
					memcpy( rec.userInfo.basic.alias, nickname, USERDATA_NAME_SIZE );
					
					// copy password
					memcpy( rec.userInfo.security.newpass, pQ->bindInfo.bindPass, USERDATA_NAME_SIZE );
					memcpy( rec.userInfo.security.oldpass, rec.userInfo.security.newpass, USERDATA_NAME_SIZE );
					rec.userInfo.security.modify = 1;

					BBQACLFLAG_SET( rec.service.flags, m_config.flagsDefault.value ); 
					rec.service.serverID = 0;
					rec.uid = 0;

					rec.service.type = SERVICE_PREPAY;
					rec.service.start_time = time(NULL);
					rec.service.expire_time = rec.service.start_time + m_config.subscribe_trial_time * 3600 * 24;
					
					CATCH_DB_FAIL(
						WriteLock safe( m_mutexDB );

						rec.uid = m_pDB->CreateUser( & rec );
						if( rec.uid ) {
							PLOG( m_pLogger, Debug, PString( PString::Printf, "%s user created for authentication gateway.", (authResult.isTrialUser ? "trial" : "normal" ) ) );

							VFONID vfonid;
							vfonid.serverid = 0;
							vfonid.userid = rec.uid;
							if( m_pDB->BindByKey( vfonid, & pQ->bindInfo ) ) {
								rec.service.bindInfo = pQ->bindInfo;
							}

							bBindUserCreated = true;
							bRead = true;
						} else {
							PLOG( m_pLogger, Error, "database create user (for authentication gateway) failed." );
						}
					);
				}

				if( ! authResult.isTrialUser ) { 
					// normal user will never expire if authenticated.
					rec.service.start_time = time(NULL);
					rec.service.expire_time = rec.service.start_time + m_config.subscribe_trial_time * 3600 * 24;
				} else {
					// when will the trial user expire?
				}
			} else {}

			if( bRead ) {
				pRec = InsertRecordIntoMemoryCache( & rec );

				// if record loaded, then also load link info
				if( pRec ) LoadLayeredLinkData( pRec );

				// update the bindType with info sent from client, 
				// when conference log is written, this bindType will be put into commen field.
				if( pRec->pServiceInfo ) pRec->pServiceInfo->bindInfo = pQ->bindInfo;
				//if( pRec->pServiceInfo ) memcpy( pRec->pServiceInfo->bindInfo.bindType, pQ->bindInfo.bindType, USERDATA_NAME_SIZE );

				if( strstr(pQ->bindInfo.bindType, "PCCW_") != NULL ) {

					if( ! authResult.isTrialUser ) { 
						// if it is a normal user, enable the service according to authenticate result
						CATCH_DB_FAIL(
							VFONID vfonid;
							vfonid.serverid = 0;
							vfonid.userid = rec.uid;
							m_pDB->EnableUserVMeetService( vfonid, (authResult.type == BBQDatabase::SERVICE_VMEET) ); 
						);
					} else if( bBindUserCreated ) { // if it is a temp user, add invitor ID as friend each other
						SFIDRecord * pInvitorRec = FindRecordByID( authResult.inviterID.userid );
						if( pInvitorRec != NULL ) {
							// modify invitor's buddy list, and save immediately
							if( pInvitorRec->pBlockList ) pInvitorRec->pBlockList->Remove( rec.uid );
							if( pInvitorRec->pFriendList ) pInvitorRec->pFriendList->Add( rec.uid );

							BBQUserValidFlags flags;
							memset( & flags, 0, sizeof(flags) );
							flags.partial_valid = 1;
							flags.friends = 1;
							flags.blocks = 1;
							SaveData( pInvitorRec, SaveOnChange, & flags );

							// modify this buddy list, 
							if( pRec->pBlockList ) pRec->pBlockList->Remove( authResult.inviterID.userid );
							if( pRec->pFriendList ) pRec->pFriendList->Add( authResult.inviterID.userid );
							// no need to save here, it will be saved later.

							if( m_config.write_useraction_log ) {
								PString strMsg( PString::Printf, "trial user %d and his invitor %d add each other to buddy list.", authResult.inviterID.userid, rec.uid );
								m_UserActionLog.Output( PLog::Info, strMsg );
							}
						}
					}
				} else {}
			} else {}
		}

		bAuthenticated = bAuthFromGateway && bRead;

		// for bind user, assign the billing balance info
		binfo.statusCode = (authResult.type == BBQDatabase::SERVICE_VMEET) ? BBQDatabase::ST_PRO : BBQDatabase::ST_BASIC;
		binfo.points = 1;
		if( strstr(pQ->bindInfo.bindType, "ARCHIEVA") != NULL ) {
			binfo.points = (int) authResult.balance;
		}
	}

	if( ! pRec ) {
		if( m_config.write_useraction_log ) {
			PString strMsg( PString::Printf, "user attempt to login as %d from %s denied, cause no such record.", pQ->vfonId.userid, (const char *)BBQMsgTerminal::ToString( & req->channel.peer ) );
			m_UserActionLog.Output( PLog::Info, strMsg );
		}

		pA->statusCode = SIMS_NOTFOUND;
		SendMessage( & reply );

		if( authResult.pBuddyList ) { delete authResult.pBuddyList; authResult.pBuddyList = NULL; }
		return true;
	}

	if( ! bAuthenticated ) {
		if( m_config.write_useraction_log ) {
			m_UserActionLog.Output( PLog::Info, "user login attempt denied, due to auth fail. from: " + BBQMsgTerminal::ToString( & req->channel.peer ) ); 
		}

		pA->statusCode = SIMS_DENY;
		pA->errCode = ERR_AUTHFAIL;
		SendMessage( & reply );

		if( authResult.pBuddyList ) { delete authResult.pBuddyList; authResult.pBuddyList = NULL; }
		return true;
	}

	if ( pQ->onlineStatus == CLIENT_MCU ) {
		// do not check the user limitation for MCU Id
	} else {
#ifndef _DEBUG
		if( UserLimitForOnline() ) {
			if( m_status.nOnlineUsers >= GetUserLimit() || m_status.nOnlineUsers> 20 ) {
				if( m_config.write_useraction_log ) {
					m_UserActionLog.Output( PLog::Info, "reaching the maximal number of online users, user login attempt denied, from: " + BBQMsgTerminal::ToString( & req->channel.peer ) ); 
				}

				pA->statusCode = SIMS_DENY;
				pA->errCode = ERR_TOOMANY;
				SendMessage( & reply );

				if( authResult.pBuddyList ) { delete authResult.pBuddyList; authResult.pBuddyList = NULL; }
				return true;
			}

		} else { // the user limit is for register user, so we have to check the DB records
			int nNumberOfSmallerId = 0;

			CATCH_DB_FAIL(
				nNumberOfSmallerId = m_pDB->GetCountOfUserProfile( pRec->sessionInfo.id );
			);
			
			if( nNumberOfSmallerId >= GetUserLimit() ) {
				if( m_config.write_useraction_log ) {
					m_UserActionLog.Output( PLog::Info, "reaching the maximal number of registered users, user login attempt denied, from: " + BBQMsgTerminal::ToString( & req->channel.peer ) ); 
				}

				pA->statusCode = SIMS_DENY;
				pA->errCode = ERR_TOOMANY;
				SendMessage( & reply );

				if( authResult.pBuddyList ) { delete authResult.pBuddyList; authResult.pBuddyList = NULL; }
				return true;
			}
		}
#endif
	}

	// make sure the sessions list are empty, this might happen when user drop offline and login again
	if( pRec->pSessionList ) {
		DestroyVfonSessionList( pRec->pSessionList );
		pRec->pSessionList = NULL;
	}

	if( pRec->pServiceInfo ) {
		pRec->pServiceInfo->billing_status = binfo.statusCode;
		pRec->pServiceInfo->points = binfo.points;
	}

	UpdateAccessFlags( pRec );

	if( pRec->flags.no_login ) { // this user is forbidden to login by manual
		if( m_config.write_useraction_log ) {
			m_UserActionLog.Output( PLog::Info, "user login attempt denied, for being set as \"no login\". from: " + BBQMsgTerminal::ToString( & req->channel.peer ) ); 
		}

		pA->statusCode = SIMS_DENY;
		pA->errCode = ERR_FORBIDDEN;
		SendMessage( & reply );

		if( authResult.pBuddyList ) { delete authResult.pBuddyList; authResult.pBuddyList = NULL; }
		return true;
	}

	if( pRec->pServiceInfo && pRec->pServiceInfo->is_group && ( ! m_config.allow_group_id_login_as_user ) ) {
		pA->statusCode = SIMS_DENY;
		pA->errCode = ERR_ISGROUPID;
		SendMessage( & reply );

		if( authResult.pBuddyList ) { delete authResult.pBuddyList; authResult.pBuddyList = NULL; }
		return true;
	}

	// --------- now, handle accepted affairs ------------------

	// generate a random number as cookie
	uint32 uNewCookie = rand() * PTimer::Tick().GetInterval() * pRec->sessionInfo.id;
	if ( uNewCookie == 0 ) uNewCookie ++; // cookie == 0, means offline, so never use 0 as cookie

	if( pRec->sessionInfo.cookie ) {
		if( (pRec->sockChannel.peer.ip == req->channel.peer.ip) && (pRec->sockChannel.peer.port == req->channel.peer.port) ) {
			// from same client, it's re-login, do not send replaced message. and, do not change the cookie
		} else {
			// notify user that already login, the connection is being replaced from another place 
			SIM_REQUEST notify;
			SIM_REQINITCHAN( notify, pRec->sockChannel, SIM_SC_LOGINREPLACED, sizeof(SIMD_SC_LOGINREPLACED) );

			SIMD_SC_LOGINREPLACED * pN = (SIMD_SC_LOGINREPLACED *) & notify.msg.simData;
			pN->sessionInfo = pRec->sessionInfo;
			pN->addrReplaceBy = req->channel.peer;
			SendMessage( & notify );

			pRec->sessionInfo.cookie = uNewCookie;
		}
	} else {
		// only count those who was offline
		// do not count those who was online but re-login
		m_status.nOnlineUsers ++;
		m_status.nMaxUserCount = max( m_status.nMaxUserCount, m_status.nOnlineUsers );

		pRec->sessionInfo.cookie = uNewCookie;
	}

	time_t	tNow = time(NULL);

	// this is the main session channel, we send notifying msg to client over it
	pRec->sockChannel = req->channel;

	pRec->login_time = pRec->time = tNow;
	pRec->techInfo = pQ->techParam;
	pRec->netLAN.ip = pQ->techParam.interface_ip;

	//pRec->userStatus = pQ->onlineStatus;
	pRec->actStatus = pQ->onlineStatus & 0xff; // only accept activity status when login, cause the set part is stored in DB

	// give the same copy of cookie to user
	pA->sessionInfo = pRec->sessionInfo;
	pA->statusCode = SIMS_OK;
	pA->tNowServerTime = time(NULL);
	pA->tStartTime = pRec->serviceStart;
	pA->tExpireTime = pRec->serviceExpire;

	pA->nServerId = m_config.domainInfo.dwId;

	pA->nPoints = binfo.points;

	pA->accessFlags = pRec->flags;
	pA->accessFlags.temp_user = authResult.isTrialUser ? 1 : 0;

	pA->extAccessFlags = pRec->pServiceInfo->extFlags;

	pA->have_meeting = ( pRec->pMeetingList &&  ( pRec->pMeetingList->size() > 0 ) ) ? 1 : 0;
	//pA->have_offline_message = 0;
	//pA->being_asked_as_buddy = 0;
	pA->setStatus = pRec->setStatus;

	pA->nHiddenBuddyCount = authResult.invisible;

	// free old aes key
	{
		if( pRec->pAESKeyBytes ) {
			free( pRec->pAESKeyBytes );
			pRec->pAESKeyBytes = NULL;
		}
		pRec->nAESKeyBytes = 0;
	}

	if( key_size > 0 ) {
		pRec->pAESKeyBytes = (char *) malloc( key_size );
		if( pRec->pAESKeyBytes ) {
			memcpy( pRec->pAESKeyBytes, secret_key, key_size );
			pRec->nAESKeyBytes = key_size;
		}
	}

	if( m_config.write_useraction_log ) {
		PString strMsg( PString::Printf, "%d %s log in from %s", pRec->sessionInfo.id, key_size?"securely":"", (const char *)ToString( & pRec->sockChannel.peer ) );
		m_UserActionLog.Output( PLog::Info, strMsg );
	}

	// subscribe notifying message from my buddy list,
	// also check validity of the buddy list here
	bool bBuddyListChanged = false;
	if( pRec->pFriendList ) {

		int nAuthBuddy = authResult.pBuddyList ? authResult.pBuddyList->size() : 0;

		int nCount = pRec->pFriendList->Count();
		uint32 * pId = pRec->pFriendList->Data();

		int nRemoved = 0, nNew = 0, nExists = 0, nCreated = 0;

		uint32 tmp[ USERDATA_LIST_SIZE ];
		if( nCount >= USERDATA_LIST_SIZE ) nCount = USERDATA_LIST_SIZE;
		memcpy( & tmp[0], pId, nCount * sizeof(uint32) );

		for( int i=0; i<nCount; i++ ) {
			SFIDRecord * pBuddy = FindRecordByID( tmp[i] );
			
			bool bRemoveFromBuddyList = false;
			if( pBuddy ) {
				bool bFound = false;

				// if authenticate gateway return a buddy list, then we need to check it and add/remove
				if( pBuddy->pServiceInfo && authResult.pBuddyList ) {
					for( string_list::iterator it = authResult.pBuddyList->begin(), et = authResult.pBuddyList->end(); it != et; it ++ ) {
						if( 0 == strcasecmp( it->c_str(), pBuddy->pServiceInfo->bindInfo.bindId) ) { // already match
							authResult.pBuddyList->erase( it );
							bFound = true;
							break;
						}
					}
				}

				// this buddy is in our list, but not in list from auth gateway
				// we keep it, cause it's vfon added buddy
				if( ! bFound ) {
					if( authResult.pBuddyList && m_config.sync_buddy_with_authgateway ) {
						bRemoveFromBuddyList = true;
					}
				}
			} else {
				// the buddy is not found. bad record ? remove from my list then.
				bRemoveFromBuddyList = true;
			}

			if( bRemoveFromBuddyList ) {
				pRec->pFriendList->Remove( tmp[i] );
				bBuddyListChanged = true;
				nRemoved ++;
			} else if ( ! pBuddy->pBlockList->Contain( pRec->sessionInfo.id ) ) { // if in block list, the do not notify status 
				//if( ! pBuddy->pBuddyReverseLink ) pBuddy->pBuddyReverseLink = new UserIdList();
				//pBuddy->pBuddyReverseLink->Add( pRec->sessionInfo.id );
			}
		}

		// we still have some new guys in our auth buddy list, we will add them to our buddy list now.
		if( authResult.pBuddyList && (! authResult.pBuddyList->empty()) ) {
			for( string_list::iterator it = authResult.pBuddyList->begin(), et = authResult.pBuddyList->end(); it != et; it ++ ) {

				// ignore too long id for memory safety
				if( strlen( it->c_str() ) >= USERDATA_NAME_SIZE ) continue;

				// search the db for the user with specified id
				BBQUserFullRecord buddyFullInfo;
				memset( & buddyFullInfo, 0, sizeof(buddyFullInfo) );

				bool buddyRead = false;

				CATCH_DB_FAIL(
					// the search will ignore type, this will allow PCCW_xxx call each other, but pls make sure the bindId is unique
					buddyRead = m_pDB->ReadByKey( & buddyFullInfo, pQ->bindInfo.bindType, it->c_str() );
				);

				// if not found, then create
				if( buddyRead ) {
					nExists ++;
					// already exists
				} else {
					strcpy( buddyFullInfo.userInfo.basic.alias, it->c_str() );
					strcpy( buddyFullInfo.service.bindInfo.bindId, it->c_str() );
					strcpy( buddyFullInfo.service.bindInfo.bindType, pQ->bindInfo.bindType );

					// copy password
					memcpy( buddyFullInfo.userInfo.security.newpass, pQ->bindInfo.bindPass, USERDATA_NAME_SIZE );
					memcpy( buddyFullInfo.userInfo.security.oldpass, buddyFullInfo.userInfo.security.newpass, USERDATA_NAME_SIZE );
					buddyFullInfo.userInfo.security.modify = 1;

					BBQACLFLAG_SET( buddyFullInfo.service.flags, m_config.flagsDefault.value ); 
					buddyFullInfo.service.serverID = 0;
					buddyFullInfo.uid = 0;

					buddyFullInfo.service.type = SERVICE_PREPAY;
					buddyFullInfo.service.start_time = time(NULL);
					buddyFullInfo.service.expire_time = rec.service.start_time + m_config.subscribe_trial_time * 3600 * 24;

					CATCH_DB_FAIL(
						WriteLock safe( m_mutexDB );

						buddyFullInfo.uid = m_pDB->CreateUser( & buddyFullInfo );
						if( buddyFullInfo.uid ) {
							nCreated ++;
							//PLOG( m_pLogger, Debug, 
							//	  PString( PString::Printf, "%s user (%u) created for authentication gateway.", (authResult.isTrialUser ? "trial" : "normal" ), buddyFullInfo.uid ) 
							//	);

							VFONID vfonid;
							vfonid.serverid = 0;
							vfonid.userid = buddyFullInfo.uid;
							
							m_pDB->BindByKey( vfonid, & buddyFullInfo.service.bindInfo );

							buddyRead = true;
						} else {
							PLOG( m_pLogger, Error, "database create user (for authentication gateway) failed." );
						}
					);
				}

				if( buddyRead ) {
					SFIDRecord * pBuddy = InsertRecordIntoMemoryCache( & buddyFullInfo );

					// if record loaded, then also load link info
					if( pBuddy ) LoadLayeredLinkData( pBuddy );

					if( pBuddy ) {
						nNew ++;
						pRec->pFriendList->Add( buddyFullInfo.uid );
						bBuddyListChanged = true;
					}
				}

			}
		}

		if( authResult.pBuddyList ) {		
			PLOG( m_pLogger, Debug, 
				PString( PString::Printf, "for user %u, auth gateway return %d buddies, %d of %d removed from old buddy list, and %d new ones, %d exists, %d created.", 
				pRec->sessionInfo.id, nAuthBuddy, nRemoved, nCount, nNew, nExists, nCreated ) 
				);
		}
	}

	// check validity of the buddy list here
	if( pRec->pBlockList ) {
		int nCount = pRec->pBlockList->Count();
		uint32 * pId = pRec->pBlockList->Data();

		uint32 tmp[ USERDATA_LIST_SIZE ];
		if( nCount >= USERDATA_LIST_SIZE ) nCount = USERDATA_LIST_SIZE;
		memcpy( & tmp[0], pId, nCount * sizeof(uint32) );

		for( int i=0; i<nCount; i++ ) {
			SFIDRecord * pGuy = FindRecordByID( tmp[i] );
			if( pGuy ) {
				//
			} else {
				// the buddy is not found. bad record ? remove from my list ?
				pRec->pBlockList->Remove( tmp[i] );
			}
		}
	}

	// after login, write back to database immediately, so that the server location is known to global
	BBQUserValidFlags flags;
	memset( & flags, 0, sizeof(flags) );
	flags.partial_valid = 1;
	flags.service = 1;
	flags.param = 1;
	flags.friends = bBuddyListChanged ? 1 : 0;
	SaveData( pRec, SaveOnLogin, & flags );

	SendMessage( & reply );

	if( pRec->actStatus == CLIENT_MCU ) {
		MCUMap::iterator mcu_iter = m_mapMCU.find( pRec->sessionInfo.id );
		if( mcu_iter != m_mapMCU.end() ) {
			MCURecord * pMCU = mcu_iter->second;
			memset( pMCU, 0, sizeof(MCURecord) );
			pMCU->vfonId.userid = pRec->sessionInfo.id;
			//memcpy( pMCU->alias, pRec->pUserInfo->basic.alias, USERDATA_NAME_SIZE );
			//mcu_iter->second = TRUE;
		} else {
			MCURecord * pMCU = new MCURecord;
			memset( pMCU, 0, sizeof(MCURecord) );
			pMCU->vfonId.userid = pRec->sessionInfo.id;
			//memcpy( pMCU->alias, pRec->pUserInfo->basic.alias, USERDATA_NAME_SIZE );
			m_mapMCU[ pRec->sessionInfo.id ] = pMCU;
		}
		m_status.nMCUCount = m_mapMCU.size();
		m_status.nMaxMCUCount = max( m_status.nMCUCount, m_status.nMaxMCUCount );
	}

	m_bDataChanged = true;

	// update status in central server.
	if( m_status.nInterlinkConnected ) {
		SIM_REQUEST report;
		for( int i=0; i<2; i++ ) {
			if( m_chanInterlink[i].peer.ip != 0 ) {

				SIM_REQINITCHAN( report, m_chanInterlink[i], SIM_SX_UPDATESWITCHINFO, sizeof(SIMD_SX_UPDATESWITCHINFO) );
				SIMD_SX_UPDATESWITCHINFO * pU = (SIMD_SX_UPDATESWITCHINFO *) & report.msg.simData;

				memset( pU, 0, sizeof(*pU) );
				pU->id = pRec->sessionInfo.id;
				pU->info.login = 1;
				pU->info.is_tcp = (pRec->sockChannel.socket != 0) ? 1 : 0;
				pU->info.firewall_type = pRec->techInfo.firewall;
				//pU->info.status = pRec->userStatus & 0xff;
				pU->info.act_status = pRec->actStatus & 0xff;
				pU->info.set_status = pRec->setStatus & 0xff;

				// TODO: using this server external address ip
				//pU->info.addr.ip = 0;
				pU->info.addr.ip = m_config.thisServerAddress.ip;
				pU->info.addr.tcpPort = m_nDefaultTcpPort;
				pU->info.addr.udpPort = m_nDefaultUdpPort;

				pU->info.channel = pRec->sockChannel;

				SendMessage( & report );
			}
		}
	}

	// ask for buddy status update
	AskForBuddyStatus( pRec );

	// show my status to my buddies
	NotifyBuddyStatus( pRec );

	// re notify meeting info to MCU
	if( pRec->actStatus == CLIENT_MCU ) {
		m_UserActionLog.Output( PLog::Info, PString(PString::Printf, "MCU %d login, checking related meetings.", pRec->sessionInfo.id) );
		WriteLock lock( m_mutexRecord );
		for( meetingext_list::iterator iter = m_mlMeetingList.begin(), eiter = m_mlMeetingList.end(); iter != eiter; iter ++ ) {
			BBQ_MeetingInfo_Ext * p = * iter;
			if( p->info.nMcuId == pRec->sessionInfo.id ) {
				m_UserActionLog.Output( PLog::Info, PString(PString::Printf, "Started meeting \"%s\"(id=\"%s\") using MCU %d, now re-notify MCU.", p->info.strMeetingName, p->info.strMeetingId, pRec->sessionInfo.id) );
				NotifyClientMeetingEvent( p, MCC_BEGIN, false );
			}
		}
	}

	if( authResult.pBuddyList ) { delete authResult.pBuddyList; authResult.pBuddyList = NULL; }
	return true;
}

void BBQServer::AskForBuddyStatus( SFIDRecord * pRec, uint32 uidTarget )
{
	RecordMap	localOnline;
	UserIdList	otherIds;
	RecordMap	alwaysOffline;

	time_t	tTest = time(NULL) - m_config.heartbeat_timeout;

	if( uidTarget != 0 ) {

		SFIDRecord * p = FindRecordByID( uidTarget );
		if( (p->sessionInfo.cookie != 0) && (p->time > tTest) ) {
			// he is online on this server
			localOnline[ p->sessionInfo.id ] = p;
		} else {
			// he is offline, or on other server
			otherIds.Add( p->sessionInfo.id );
		}
		
	} else { // target not mentioned, so we will get all buddy & CUG buddy status update

		// clear all data, and we will re-fill
		if( pRec->pBuddyStatus ) {
			pRec->pBuddyStatus->clear();
		}

		// CUG buddies
		if( pRec->pParentIdList || pRec->pChildIdList ) {
			// get all root ids
			uint_list uintlist;
			RecordMap records;
			if( GetRootUserNodes( pRec->sessionInfo.id, records ) ) {
				for( RecordMap::iterator it = records.begin(), et = records.end(); it != et; it ++ ) {
					uintlist.push_back( it->first );
				}
			}

			// get all nodes in same CUG tree
			records.clear();
			if( GetSubUserRecords( uintlist, records ) ) {
				for( RecordMap::iterator it = records.begin(), et = records.end(); it != et; it ++ ) {
					SFIDRecord * p = it->second;
					if( p ) {
						if( (p->sessionInfo.cookie != 0) && (p->time > tTest) ) {
							// he is online on this server
							localOnline[ p->sessionInfo.id ] = p;
						} else {
							// he is offline, or on other server
							otherIds.Add( p->sessionInfo.id );
						}
					} else {
						// not found
					}
				}
			}
		}

		// private buddies
		if( pRec->pFriendList ) {
			int nCount = pRec->pFriendList->Count();
			uint32 * pId = pRec->pFriendList->Data();
			for( int i=0; i<nCount; i++, pId++ ) {
				if( *pId == pRec->sessionInfo.id ) continue;

				SFIDRecord * p = FindRecordByID( * pId );
				if( p ) {
					if( p->pFriendList && p->pFriendList->Contain(pRec->sessionInfo.id) ) { // I am buddy of this guy
						if( (p->sessionInfo.cookie != 0) && (p->time > tTest) ) {
							// he is online on this server
							localOnline[ p->sessionInfo.id ] = p;
						} else if ( p->setStatus != 0 ) { // the set status is set, so need to be regarded as online user
							localOnline[ p->sessionInfo.id ] = p;
						} else {
							// he is offline, or on other server
							otherIds.Add( p->sessionInfo.id );
						}
					} else { // I am not buddy of this guy
						alwaysOffline[ p->sessionInfo.id ] = p;
					}
				} else {
					// not found
				}
			}
		}
	}

	// now, fill all local online buddy status
	if( pRec->pBuddyStatus ) {
		for( RecordMap::iterator it = localOnline.begin(), et = localOnline.end(); it != et; it ++ ) {
			(* pRec->pBuddyStatus)[ it->first ] = (it->second) ->userStatus;
		}
	}

	// ignore always offline buddy
	// do nothing here

	// and, request domain server for other status 
	// send msg to domain exchange server, it will forward the notify msg 
	if( m_status.nInterlinkConnected && (otherIds.Count() > 0) ) {
		uint32 n = otherIds.Count();
		if( n > 1000 ) n = 1000;
		uint32 data_size = sizeof(SIMD_SX_BUDDYSTATUSNOTIFY) + sizeof(uint32) * n;
		SIM_REQUEST * pReq = NULL;
		SIM_REQUEST_NEW( pReq, sizeof(SIM_CHANNEL) + sizeof(SFIDMSGHEADER) + data_size );
		if( pReq ) {
			SIM_REQINIT( * pReq, 0, 0, 0, 0, 0, SIM_SX_BUDDYSTATUSNOTIFY, data_size );
			SIMD_SX_BUDDYSTATUSNOTIFY * pF = (SIMD_SX_BUDDYSTATUSNOTIFY *) & pReq->msg.simData;
			memset( pF, 0, sizeof(*pF) );

			//pF->info = pN->info;
			pF->info.peerId.userid = pRec->sessionInfo.id;
			pF->flags.get_status_list = 1;
			pF->n = n;
			memcpy( & pF->uidlist[0], otherIds.Data(), sizeof(uint32) * n );

			for( int i=0; i<2; i++ ) {
				if( m_chanInterlink[i].peer.ip != 0 ) {
					pReq->channel = m_chanInterlink[i];
					SendMessage( pReq );
				}
			}

			SIM_REQUEST_DELETE( pReq );
		}
	}
}

void BBQServer::NotifyBuddyStatus( SFIDRecord * pRec, uint32 uidTarget, uint32 nStatus )
{
	time_t	tTest = time(NULL) - m_config.heartbeat_timeout;

	if((pRec->sessionInfo.cookie == 0) || (pRec->time < tTest)) pRec->actStatus = CLIENT_OFFLINE;

	uint32 userStatus = (nStatus == CLIENT_NORMAL) ? pRec->userStatus : nStatus;

	SIM_REQUEST msg;
	SIM_REQINIT( msg, 0, 0, 0, 0, 0, SIM_SC_BUDDYSTATUSNOTIFY, sizeof(SIMD_SC_BUDDYSTATUSNOTIFY) );
	SIMD_SC_BUDDYSTATUSNOTIFY * pN = (SIMD_SC_BUDDYSTATUSNOTIFY *) & msg.msg.simData;
	memset( pN, 0, sizeof(*pN) );
	pN->info.peerId.userid = pRec->sessionInfo.id;
	pN->info.techInfo = pRec->techInfo;
	if( pRec->pUserInfo ) memcpy( pN->info.alias, pRec->pUserInfo->basic.alias, USERDATA_NAME_SIZE );
	if( pRec->pServiceInfo ) memcpy( pN->info.bindId, pRec->pServiceInfo->bindInfo.bindId, USERDATA_ADDRESS_SIZE );

	pN->info.onlineStatus = userStatus;

	RecordMap	localOnline;
	UserIdList	otherIds;

	if( uidTarget != 0 ) {

		if( pRec->pBlockList && pRec->pBlockList->Contain(uidTarget) ) {
			userStatus = CLIENT_OFFLINE;
		}

		pN->info.onlineStatus = userStatus;

		SFIDRecord * p = FindRecordByID( uidTarget );
		if( p ) {
			if( (p->sessionInfo.cookie != 0) && (p->time > tTest) ) {
				// he is online on this server
				localOnline[ p->sessionInfo.id ] = p;
			} else {
				// he is offline, or on other server
				otherIds.Add( p->sessionInfo.id );
			}
		} else { // no such user
		}
		
	} else { // target not mentioned, so we will notify everyone that can see my status

		// CUG buddies
		if( pRec->pParentIdList || pRec->pChildIdList ) {
			// get all root ids
			uint_list uintlist;
			RecordMap records;
			if( GetRootUserNodes( pRec->sessionInfo.id, records ) ) {
				for( RecordMap::iterator it = records.begin(), et = records.end(); it != et; it ++ ) {
					uintlist.push_back( it->first );
				}
			}

			// get all nodes in same CUG tree
			records.clear();
			if( GetSubUserRecords( uintlist, records ) ) {
				for( RecordMap::iterator it = records.begin(), et = records.end(); it != et; it ++ ) {
					// skip self
					//if( it->first == pRec->sessionInfo.id ) continue;

					// if the id is blocked, then skip
					if( pRec->pBlockList && pRec->pBlockList->Contain( it->first ) ) continue;

					SFIDRecord * p = it->second;
					if( p ) {
						if( (p->sessionInfo.cookie != 0) && (p->time > tTest) ) {
							// he is online on this server
							localOnline[ p->sessionInfo.id ] = p;
						} else {
							// he is offline, or on other server
							otherIds.Add( p->sessionInfo.id );
						}
					} else {
						// not found
					}
				}
			}
		}

		// private buddies
		if( pRec->pFriendList ) {
			int nCount = pRec->pFriendList->Count();
			uint32 * pId = pRec->pFriendList->Data();
			for( int i=0; i<nCount; i++, pId++ ) {
				//if( *pId == pRec->sessionInfo.id ) continue;

				SFIDRecord * p = FindRecordByID( * pId );
				if( p ) {
					if( (p->sessionInfo.cookie != 0) && (p->time > tTest) ) {
						// he is online on this server
						localOnline[ p->sessionInfo.id ] = p;
					} else {
						// he is offline, or on other server
						otherIds.Add( p->sessionInfo.id );
					}
				} else {
					// not found
				}
			}
		}
	}

	// send notify msg to local online buddies
	if( localOnline.size() > 0 ) {
		for( RecordMap::iterator it = localOnline.begin(), eit = localOnline.end(); it != eit; it ++ ) {
			//uint32 id = it->first;
			SFIDRecord * p = it->second;

			// update the user's status list, so that it can be retrieved by heartbeat
			if( p->pBuddyStatus ) {
				(* p->pBuddyStatus)[ pRec->sessionInfo.id ] = userStatus;
			}

			// send immediate notification
			msg.channel = p->sockChannel;
			pN->sessionInfo = p->sessionInfo;
			SendMessage( & msg );
		}
	}

	// send msg to domain exchange server, it will forward the notify msg 
	if( m_status.nInterlinkConnected && (otherIds.Count() > 0) ) {
		uint32 n = otherIds.Count();
		if( n > 1000 ) n = 1000;
		uint32 data_size = sizeof(SIMD_SX_BUDDYSTATUSNOTIFY) + sizeof(uint32) * n;
		SIM_REQUEST * pReq = NULL;
		SIM_REQUEST_NEW( pReq, sizeof(SIM_CHANNEL) + sizeof(SFIDMSGHEADER) + data_size );
		if( pReq ) {
			SIM_REQINIT( * pReq, 0, 0, 0, 0, 0, SIM_SX_BUDDYSTATUSNOTIFY, data_size );
			SIMD_SX_BUDDYSTATUSNOTIFY * pF = (SIMD_SX_BUDDYSTATUSNOTIFY *) & pReq->msg.simData;
			memset( pF, 0, sizeof(*pF) );

			pF->info = pN->info;
			pF->flags.notify_1by1 = 1;
			pF->n = n;
			memcpy( & pF->uidlist[0], otherIds.Data(), sizeof(uint32) * n );

			for( int i=0; i<2; i++ ) {
				if( m_chanInterlink[i].peer.ip != 0 ) {
					pReq->channel = m_chanInterlink[i];
					SendMessage( pReq );
				}
			}

			SIM_REQUEST_DELETE( pReq );
		}
	}
}

// in domain server 
bool BBQServer::OnMsgSXBuddyStatusNotify( const SIM_REQUEST * req )
{
	const SIMD_SX_BUDDYSTATUSNOTIFY * pQ = (const SIMD_SX_BUDDYSTATUSNOTIFY *) & req->msg.simData;

	if( pQ->flags.get_status_list ) { // ask for buddy status
		uint32 n = pQ->n;
		if( n > 1000 ) n = 1000;
		uint32 data_size = sizeof(SIMD_XS_BUDDYSTATUSNOTIFY) + sizeof(ONLINE_USER) * n;
		SIM_REQUEST * pReq = NULL;
		SIM_REQUEST_NEW( pReq, sizeof(SIM_CHANNEL) + sizeof(SFIDMSGHEADER) + data_size );
		if( pReq ) {
			SIM_REQINITCHAN( * pReq, req->channel, SIM_XS_BUDDYSTATUSNOTIFY, data_size );
			SIMD_SX_BUDDYSTATUSNOTIFY * pF = (SIMD_XS_BUDDYSTATUSNOTIFY *) & pReq->msg.simData;
			memset( pF, 0, sizeof(*pF) );

			pF->sessionInfo.id = pQ->info.peerId.userid;
			pF->info = pQ->info;
			pF->flags.get_status_list = 1;
			pF->n = n;

			SWITCHINFO si;
			int i=0, j=0;
			for( i=0, j=0; i<n; i++ ) {
				uint32 uid = pQ->uidlist[i];
				if( m_bbqSwitch.Get( uid, si ) ) {
					pF->onlinelist[i].id = uid;

					uint32 s = si.set_status;
					s <<= 8;
					s |= si.set_status;
					pF->onlinelist[i].status = s;
				}
			}

			SendMessage( pReq );

			SIM_REQUEST_DELETE( pReq );
		}

	}
	
	if( pQ->flags.notify_1by1 ) { // request forwarding
		SIM_REQUEST msg;
		SIM_REQINIT( msg, 0, 0, 0, 0, 0, SIM_XS_BUDDYSTATUSNOTIFY, sizeof(SIMD_XS_BUDDYSTATUSNOTIFY) );
		SIMD_XS_BUDDYSTATUSNOTIFY * pN = (SIMD_XS_BUDDYSTATUSNOTIFY *) & msg.msg.simData;
		memset( pN, 0, sizeof(*pN) );
		pN->info = pQ->info;
		pN->flags.notify_1by1 = 1;

		SWITCHINFO si;
		uint32 uid = pQ->info.peerId.userid;

		if( m_bbqSwitch.Get( uid, si ) ) {
			si.channel = req->channel;
			//si.status = pQ->info.onlineStatus;
			si.act_status = pQ->info.onlineStatus & 0xff;
			si.set_status = (pQ->info.onlineStatus >> 8) & 0xff;
			m_bbqSwitch.Put( uid, si );
		}

		for( int i=0; i<pQ->n; i++ ) {
			uid = pQ->uidlist[i];
			if( m_bbqSwitch.Get( uid, si ) ) {
				msg.channel = si.channel;
				pN->sessionInfo.id = uid; 
				SendMessage( & msg );
			}
		}
	}

	return true;
}

// in login server
bool BBQServer::OnMsgXSBuddyStatusNotify( const SIM_REQUEST * req )
{
	const SIMD_XS_BUDDYSTATUSNOTIFY * pQ = (const SIMD_XS_BUDDYSTATUSNOTIFY *) & req->msg.simData;

	SFIDRecord * pRec = FindRecordByID( pQ->sessionInfo.id );
	if( pRec ) {

		if( pQ->flags.get_status_list && pRec->pBuddyStatus ) { // update the status list
			for( int i=0; i<pQ->n; i++ ) {
				(* pRec->pBuddyStatus)[ pQ->onlinelist[i].id ] = pQ->onlinelist[i].status;
			}
		}

		if( pQ->flags.notify_1by1 ) {

			time_t	tTest = time(NULL) - m_config.heartbeat_timeout;
			if( (pRec->sessionInfo.cookie != 0) && (pRec->time > tTest) ) { // send notify msg if the user is online

				SIM_REQUEST msg;
				SIM_REQINIT( msg, 0, 0, 0, 0, 0, SIM_SC_BUDDYSTATUSNOTIFY, sizeof(SIMD_SC_BUDDYSTATUSNOTIFY) );
				SIMD_SC_BUDDYSTATUSNOTIFY * pN = (SIMD_SC_BUDDYSTATUSNOTIFY *) & msg.msg.simData;
				memset( pN, 0, sizeof(*pN) );

				// update the user's status list, so that it can be retrieved by heartbeat
				if( pRec->pBuddyStatus ) {
					(* pRec->pBuddyStatus)[ pQ->info.peerId.userid ] = pQ->info.onlineStatus;
				}

				// send immediate notification
				pN->sessionInfo = pRec->sessionInfo;
				pN->info = pQ->info;
				msg.channel = pRec->sockChannel;
				SendMessage( & msg );
			}
		}
	}

	return true;
}

/*
Input info:
  (1) session info; // optional for identity
  (2) null terminated string: name;
Output info:
  (1) statusCode;
  (2) null terminated string: value;
*/
bool BBQServer::OnMsgClientLoadString( const SIM_REQUEST * req )
{
	SIM_SESSIONINFO sessionInfo;
	char * lpszStringName = NULL;

	try {
		PBytePack packedData( & req->msg.simData, req->msg.simHeader.size );
		packedData >> sessionInfo >> lpszStringName;
	} catch ( const char * ) {
		// bad request
		return true;
	}

	if( lpszStringName ) {

		uint32	statusCode = SIMS_NOTFOUND;
		PString strStringValue = "";

		if( 0 == strcasecmp( lpszStringName, "ServerConfigXML" ) ) {
			// TODO: merge all the config string into a XML string
			strStringValue += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE CONFIG>\n<CONFIG>\n";

			// system config
			strStringValue += "<SYSTEM>\n";
			for( KeywordString::iterator it = m_dictSystemConfig.begin(), eit = m_dictSystemConfig.end(); it != eit; it ++ ) {
				strStringValue += "<VAR NAME=\"" + it->first + "\"" + " VALUE=\"" + URLTranslateString(it->second)  + "\"/>\n";
			}
			if( m_strSystemExt.GetLength() > 0 ) {
				strStringValue += "<EXT>\n" + m_strSystemExt + "</EXT>\n";
			}
			strStringValue += "</SYSTEM>\n";

			// voip info
			strStringValue += "<PRIVATE>\n";
			SFIDRecord * pRec = FindRecordByID( sessionInfo.id );
			if( pRec && pRec->pVoipItems ) {
				BBQVoIPInfo * p = & pRec->pVoipItems->items[0];
				for( int i=0; i<MAX_VOIP_COUNT; i++ ) {
					// <VOIP TYPE="SIP" ENABLE="TRUE/FALSE" SERVER="ip[:port];ip:port;..." E164="" OUTBOUND="ip:port;ip:port;..." PROPERTY="string[1024]" TTL="nnn"/>
					if( p[i].h323 ) {
						strStringValue += PString( PString::Printf, "<VOIP TYPE=\"H323\" ENABLE=\"%s\" TTL=\"%d\" E164=\"%s\" %s SERVER=\"%s\" OUTBOUND=\"%s\" PROPERTY=\"%s\"/>\n",
							(p[i].enabled ? "TRUE" : "FALSE"), p[i].ttl, 
							(const char *) URLTranslateString(p[i].e164), 
							p[i].other_password ? (const char *) PString( PString::Printf, "PWD=\"%s\"", (const char *) URLTranslateString(  p[i].password ) ) : "",
							(const char *) URLTranslateString(p[i].server_addr), 
							(const char *) URLTranslateString(p[i].outbound_addr), 
							(const char *) URLTranslateString(p[i].ext_property) 
							); 
					} else if ( p[i].sip ) {
						strStringValue += PString( PString::Printf, "<VOIP TYPE=\"SIP\" ENABLE=\"%s\" TTL=\"%d\" E164=\"%s\" %s SERVER=\"%s\" OUTBOUND=\"%s\" PROPERTY=\"%s\"/>\n",
							(p[i].enabled ? "TRUE" : "FALSE"), p[i].ttl, 
							(const char *) URLTranslateString(p[i].e164), 
							p[i].other_password ? (const char *) PString( PString::Printf, "PWD=\"%s\"", (const char *) URLTranslateString(  p[i].password ) ) : "",
							(const char *) URLTranslateString(p[i].server_addr), 
							(const char *) URLTranslateString(p[i].outbound_addr), 
							(const char *) URLTranslateString(p[i].ext_property) 
							);
					} else {
						// empty record, skip
					}
				}
				if( pRec->pXMLEXT ) {
					strStringValue += "<EXT>\n";
					strStringValue += pRec->pXMLEXT;
					strStringValue += "</EXT>\n";
				}
			}
			strStringValue += "</PRIVATE>\n";

			strStringValue += "</CONFIG>\n";

			statusCode = SIMS_OK;
		} else {
			PString strStringName = "CLIENT_";  strStringName += lpszStringName;

			CATCH_DB_FAIL(
				char	buf[ 1024 ] = "";
				if( m_pDB->ServerConfig_LoadString( strStringName, buf, 1024 ) ) {
					buf[ 1023 ] = '\0';
					statusCode = SIMS_OK;
					strStringValue = buf;
				}
			);
		}

		SIM_REQUEST reply;
		SIM_REPLY_REQUEST( & reply, SIM_SC_LOADSTRING, 0, req );

		const char * pStringValue = (const char *) strStringValue;

		PBytePack packedData, packedMsg;
		packedData << statusCode << pStringValue;
		reply.msg.simHeader.size = packedData.Size();
		packedMsg << reply.channel << reply.msg.simHeader << packedData;

		SendMessage( (SIM_REQUEST *) packedMsg.Data() );

		delete lpszStringName;
		lpszStringName = NULL;
	}

	return true;
}

/*
Input info:
  (1) session info;		// include from id & cookie
  (2) target Id;
  (2) start/stop;
  (3) conference id;
  (4) conference host's vfon id;
  (5) flags, (video, vmeet, mcu, etc.)
Output info:
  (1) ACK.
*/
bool BBQServer::OnMsgClientConference( const SIM_REQUEST * req )
{
	const SIMD_CS_CONFERENCE * pQ = (const SIMD_CS_CONFERENCE *) & req->msg.simData;

	// ingore if peer id is not specified.
	if( pQ->peerId.userid == 0 ) return true;

	WriteLock lock( m_mutexRecord );

	SFIDRecord * pRec = FindRecordByID( pQ->sessionInfo.id );
	if( pRec && (pRec->sessionInfo.cookie == pQ->sessionInfo.cookie) ) {
		if( ! pRec->pSessionList ) pRec->pSessionList = new VfonSessionList;

		bool bFound = false;

		VfonSessionList * pList = pRec->pSessionList;
		VfonSession * pSession = NULL;

		for( VfonSessionList::iterator iter = pList->begin(), eiter = pList->end(); iter != eiter; iter ++ ) {
			pSession = * iter;

			if( ( 0 == strcmp(pSession->ConferenceId, pQ->lpszConferenceId) ) 
				&& ( 0 == memcmp(& pSession->peerId, & pQ->peerId, sizeof(VFONID)) )
				&& ( 0 == strcmp(pSession->h323TargetId, pQ->h323TargetId) )
				) { // found 

				bFound = true;

				if( pSession->flags.start ) { // we only handle the sucessful calls

					if( pQ->flags.start ) { // modify the existing session flags, use method OR
						pSession->flags.value |= pQ->flags.value;
						//pSession->flags.isCaller |= pQ->flags.isCaller;
						//pSession->flags.usedVmeet |= pQ->flags.usedVmeet;
						//pSession->flags.usedVideo |= pQ->flags.usedVideo;
						//pSession->flags.usedMCU |= pQ->flags.usedMCU;

					} else { // this conference is over, now let's write to log
						pSession->errorCode = pQ->errCode;
						pSession->endTime = time(NULL);
						pSession->flags.dropOffline = 0;

						m_status.msConfTime += (pSession->endTime - pSession->beginTime);

						if( m_config.dbwrite_conference_log ) {

							CATCH_DB_FAIL(
								m_pDB->LogVfonSession( pSession );
							);

						} else {
							// if conference log is not required, then skip.
						}

						//iter = pList->erase( iter );
						STL_ERASE( (*pList), VfonSessionList::iterator, iter );
						delete pSession;
						m_status.nCurrentConfs --;

						switch( pQ->errCode ) {
						case 6: // caller abort
						case 7: // transport error
							m_status.nErrorCalls ++;
							break;
						//case 28: // channel fail
						//	m_status.nFailedCalls ++;
						//	break;
						default: // okay, or other
							m_status.nOkayCalls ++;
						}

						break;
					}

				} else { // this is a failed call, we did not record those yet

				}

				break; // we found it, then break the loop
			}
		}

		if( ! bFound ) {
			if( pQ->flags.start ) { // the session does not exist, it is a new session

				pSession = new VfonSession;
				memset( pSession, 0, sizeof(VfonSession) );

				pSession->localId.serverid = pRec->serverID;
				pSession->localId.userid = pRec->sessionInfo.id;
				pSession->peerId = pQ->peerId;
				strcpy( pSession->ConferenceId, pQ->lpszConferenceId );
				strcpy( pSession->h323TargetId, pQ->h323TargetId );	
				pSession->hostId = pQ->hostId;
				pSession->flags = pQ->flags;
				pSession->beginTime = time(NULL);

				// write user's ip info into comment field, for location trace
				if( pRec->sockChannel.peer.ip != pRec->netLAN.ip ) {
					// 32 bytes is just enough for "255.255.255.255,255.255.255.255"
					sprintf( pSession->comment, "%s,%s", (const char *) ToString(pRec->sockChannel.peer.ip), (const char *) ToString(pRec->netLAN.ip) );
				} else {
					sprintf( pSession->comment, "%s", (const char *) ToString(pRec->sockChannel.peer.ip) );
				}

				//if( pRec->pServiceInfo ) {
				//	memcpy( pSession->comment, pRec->pServiceInfo->bindInfo.bindType, USERDATA_NAME_SIZE );
				//}

				pList->push_back( pSession ); // insert to session list

				m_status.nCurrentConfs ++;
				if( m_status.nMaxConfs < m_status.nCurrentConfs ) m_status.nMaxConfs = m_status.nCurrentConfs;

			} else {

				switch( pQ->errCode ) {
				//case 7: // transport error
				//	m_status.nErrorCalls ++;
				//	break;
				case 28: // channel fail
					{
						SFIDRecord * pPeerRec = FindRecordByID( pQ->peerId.userid );
						if( pPeerRec && 
							(pPeerRec->sessionInfo.cookie != 0) && 
							(pPeerRec->time + m_config.heartbeat_timeout > time(NULL)) 
						) {
							// only the peer exists and online, we consider it as a failed call
							m_status.nFailedCalls ++;
						} else {
							m_status.nAbortedCalls ++;
						}
					}
					break;
				default: // okay, or other
					m_status.nAbortedCalls ++;
				}

			}
		}
	}

	// no reply is required, the lower level already send back an ACK.

	return true;
}

bool BBQServer::OnMsgClientGetBindInfo( const SIM_REQUEST * req )
{
	const SIMD_CS_GETBINDINFO * cs = (const SIMD_CS_GETBINDINFO *) & req->msg.simData;

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_GETBINDINFO, sizeof(SIMD_SC_GETBINDINFO), req );

	SIMD_SC_GETBINDINFO * sc = (SIMD_SC_GETBINDINFO *) & reply.msg.simData;
	memset( sc, 0, sizeof(*sc) );

	bool bDone = false;

	CATCH_DB_FAIL(
		if( cs->vfonId.userid != 0 ) {
			strcpy( sc->bindInfo.bindType, cs->bindInfo.bindType );
			bDone = m_pDB->GetBindInfoById( cs->vfonId, & sc->bindInfo );
		} else {
			BBQUserFullRecord rec;
			memset( & rec, 0, sizeof(rec) );
			bool bRead = false;
			if( 0 == strcmp( cs->bindInfo.bindType, "BY_EMAIL" ) ) {
				bRead = m_pDB->ReadByKey( & rec, cs->bindInfo.bindId, BBQDatabase::BY_EMAIL );
				if( bRead ) {
					sc->vfonId.userid = rec.uid;
					sc->vfonId.serverid = rec.service.serverID;
					bDone = true;
				}
			} else if( 0 == strcmp( cs->bindInfo.bindType, "BY_HANDPHONE" ) ) {
				bRead = m_pDB->ReadByKey( & rec, cs->bindInfo.bindId, BBQDatabase::BY_HANDPHONE );
				if( bRead ) {
					sc->vfonId.userid = rec.uid;
					sc->vfonId.serverid = rec.service.serverID;
					bDone = true;
				}
			} else if( 0 == strcmp( cs->bindInfo.bindType, "BY_HARDWAREID" ) ) {
				bRead = m_pDB->ReadByKey( & rec, cs->bindInfo.bindId, BBQDatabase::BY_HARDWAREID );
				if( bRead ) {
					sc->vfonId.userid = rec.uid;
					sc->vfonId.serverid = rec.service.serverID;
					bDone = true;
				}
			} else {
				// for PCCW users, we allow they search all PCCW_CVG, PCCW_IVC, etc.
				VfonIdBindInfo inf = cs->bindInfo;
				if( strstr(inf.bindType, "PCCW_") ) strcpy( inf.bindType, "" );
				bDone = m_pDB->GetIdByBindInfo( & inf, & sc->vfonId );
			}
		}
	);

	sc->statusCode = bDone ? SIMS_OK : SIMS_NOTFOUND;

	SendMessage( & reply );

	return true;
}

bool BBQServer::OnMsgProxyStatus( const SIM_REQUEST * req )
{
	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_PROXYSTATUS, sizeof(SIMD_SC_PROXYSTATUS), req );
	SIMD_SC_PROXYSTATUS * sc = (SIMD_SC_PROXYSTATUS *) & reply.msg.simData;
	memset( sc, 0, sizeof(*sc) );

	BBQProxy * pProxy = BBQProxy::GetCurrentProxy();
	if( pProxy != NULL ) {
		BBQProxy::Config config; pProxy->GetConfig( config );
		BBQProxy::Status status; pProxy->GetStatus( status );

		sc->statusCode = SIMS_OK;

		sc->Version.major = m_VersionInfo.server.major;
		sc->Version.minor = m_VersionInfo.server.minor;
		sc->Version.build = m_VersionInfo.server.build;

		sc->nUserMax = config.nUserMax;
		sc->tExpireTime = config.tExpireTime;

		sc->nTcpPort = config.nTcpPort;
		sc->nUdpPortMin = config.nUdpPortMin;
		sc->nUdpPortMax = config.nUdpPortMax;

		sc->nChannelMax = config.nChannelMax;
		sc->nBandwidthMax = config.nBandwidthMax;
		sc->nChannelBufferMax = config.nChannelBufferMax;

		sc->tNowTime = time(NULL);
		sc->tUpTime = status.tUpTime;

		sc->nChannels = status.nEntries/2;
		sc->nAverageBandwidth = status.nAverageBandwidth;
		sc->nBufferBytes = status.nBufferBytes;

	} else {
		sc->statusCode = SIMS_NOTAVAILABLE;
	}

	SendMessage( & reply );

	return true;
}

bool BBQServer::OnMsgServerFeedbackStatus( const SIM_REQUEST * req )
{
	const SIMD_SC_SERVERSTATUS * sc = (const SIMD_SC_SERVERSTATUS *) & req->msg.simData;

	switch( sc->link_id ) {
	case 1:		// interlink response
		{
		m_status.nAnotherServerType = sc->nServerType;
		m_status.nAnotherServerStatus = sc->nServerStatus;
		m_status.tAnotherServerUpTime = time(NULL) - (sc->tNowTime - sc->tUpTime);

		if( m_config.write_hearbeat_file ) {
			PString path( "logs" );
			path += PDirectory::IsSeparator('\\') ? "\\" : "/";
			path += STR_AnotherAlive;
			PFileLog::TouchFile( path );
		}

		if( ! m_status.nInterlinkConnected ) {
			PLOG( m_pLogger, Info, 
				PString( PString::Printf, "Interlink server is connected, and resported as: %s, %s.", 
				(const char *)TypeToString(m_status.nAnotherServerType),
				(const char *)StatusToString(m_status.nAnotherServerStatus)
				) );
		}

		m_status.nInterlinkConnected = TRUE;

		// check the server type, and solve type conflict if there is.
		switch( sc->nServerType ) {
		case BBQServer::PRIMARY:
			if( m_config.nServerType == PRIMARY ) {
				// error: both are primary, 
				// one of us should be degrade to secondary
				time_t tMyUptime = m_status.tUpTime;
				time_t tPeerUptime = sc->tUpTime - sc->tNowTime + time(NULL);
				if( tMyUptime > tPeerUptime ) SwitchType( BBQServer::SECONDARY );
			}

			// now check the status
			{
				switch( sc->nServerStatus ) {
				case BBQServer::ACTIVE:
					// peer is primary and active, check my status
					if( m_status.nServerStatus == ACTIVE ) {
						PLOG( m_pLogger, Info, "Interlink server is reported as PRIMARY and ACTIVE." );
						SwitchStatus( BBQServer::STANDBY );
					}
					break;
				case BBQServer::STANDBY:
					// peer is primary and standby, 
					// keep no change even if i am standby, peer will switch to active
					break;
				case BBQServer::DOWN:
					// another server is down
					if(m_status.nServerStatus == BBQServer::STANDBY) {
						PLOG( m_pLogger, Info, "Interlink server is reported as DOWN." );
						SwitchStatus( BBQServer::ACTIVE );
					}
					break;
				}
			}

			break;
		case BBQServer::SECONDARY:
			if( m_config.nServerType == SECONDARY ) {
				// error: both are secondary, 
				// one of us should be upgrade to primary
				time_t tMyUptime = m_status.tUpTime;
				time_t tPeerUptime = sc->tUpTime - sc->tNowTime + time(NULL);
				if( tMyUptime < tPeerUptime ) SwitchType( BBQServer::PRIMARY );
			}

			// now check the status
			{
				switch( sc->nServerStatus ) {
				case BBQServer::ACTIVE:
					// peer is secondary and active, 
					// keep no change even if i am active, peer will switch to standby
					break;
				case BBQServer::STANDBY:
					// peer is secondary and standby, i switch to active
					if( m_status.nServerStatus == BBQServer::STANDBY ) {
						PLOG( m_pLogger, Info, "Interlink server is reported as SECONDARY and STANDBY." );
						SwitchStatus( BBQServer::ACTIVE );
					}
					break;
				case BBQServer::DOWN:
					// another server is down
					if(m_status.nServerStatus == BBQServer::STANDBY) {
						PLOG( m_pLogger, Info, "Interlink server is reported as DOWN." );
						SwitchStatus( BBQServer::ACTIVE );
					}
					break;
				}
			}
			break;
		case BBQServer::CLUSTER:
			break;
		case BBQServer::STANDALONE:
			break;
		}
		}

		break;
	case 2:		// uplink response
		break;
	default:
		;
	}

	return true;
}

bool BBQServer::OnMsgServerStatus( const SIM_REQUEST * req )
{
	const SIMD_CS_SERVERSTATUS * pQ = (const SIMD_CS_SERVERSTATUS *) & req->msg.simData;

	SIMD_CS_SERVERSTATUS q;
	if( req->msg.simHeader.size == 0 ) {	// compartible to old protocol
		memset( & q, 0, sizeof(SIMD_CS_SERVERSTATUS) );
	} else {
		q = * pQ;
	}

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_SERVERSTATUS, sizeof(SIMD_SC_SERVERSTATUS), req );
	SIMD_SC_SERVERSTATUS * sc = (SIMD_SC_SERVERSTATUS *) & reply.msg.simData;
	memset( sc, 0, sizeof(*sc) );

	sc->link_id = q.link_id;

	switch( q.link_id ) {
	case 0:
		sc->statusCode = SIMS_OK;
		break;
	case 1:
	case 2:
		// check password to validate link
		sc->statusCode = SIMS_OK;
		break;
	default:
		;
	}

	sc->nUserMax = m_config.nUserMax;
	sc->tExpireTime = m_config.tExpireTime;
	sc->nServerType = m_config.nServerType;

	sc->nServerId = m_config.domainInfo.dwId;
	sc->nServerStatus = m_status.nServerStatus;

	sc->nUserCount = m_status.nOnlineUsers;
	sc->nMaxUserCount = m_status.nMaxUserCount;

	sc->nProxyCount = m_status.nProxyCount;
	sc->nMaxProxyCount = m_status.nMaxProxyCount;

	sc->nPureProxyCount = m_status.nPureProxyCount;
	sc->nMaxPureProxyCount = m_status.nMaxPureProxyCount;

	sc->tNowTime = time(NULL);
	sc->tUpTime = m_status.tUpTime;

	sc->Version.major = m_VersionInfo.server.major;
	sc->Version.minor = m_VersionInfo.server.minor;
	sc->Version.build = m_VersionInfo.server.build;
	
	sc->nInterlinkConnected = m_status.nInterlinkConnected;
	sc->anotherServerAddress = m_config.anotherServerAddress;

	sc->nUplinkConnected = m_status.nUplinkConnected;
	sc->uplinkServerAddress = m_config.uplinkServerAddress;

	SIMD_SERVER_FEATURES * pF = & sc->features;

	pF->allow_cug_call_public = m_config.allow_cug_call_public;
	pF->disable_subscribe = m_config.disable_subscribe;
	pF->cug_have_private_buddy = m_config.cug_have_private_buddy;
	pF->enable_cug = m_config.enable_cug;
	pF->run_in_asp_mode = m_config.run_in_asp_mode;

	SendMessage( & reply );

	return true;
}

PString BBQServer::TypeToString( uint32 nType )
{
	switch( nType ) {
	case BBQServer::STANDALONE:		return STR_STANDALONE;
	case BBQServer::CLUSTER:		return STR_CLUSTER;
	case BBQServer::PRIMARY:		return STR_PRIMARY;
	case BBQServer::SECONDARY:		return STR_SECONDARY;
	}

	return "INVALID_TYPE";
}

PString BBQServer::StatusToString( uint32 nStatus )
{
	switch( nStatus ) {
	case BBQServer::ACTIVE:		return STR_ACTIVE;
	case BBQServer::STANDBY:	return STR_STANDBY;
	case BBQServer::DOWN:		return STR_DOWN;
	}

	return "INVALID_STATUS";
}

uint32 BBQServer::TypeToInt( const char * strType )
{
	if( 0 == strcasecmp(strType, STR_STANDALONE) )		return BBQServer::STANDALONE;
	else if( 0 == strcasecmp(strType, STR_CLUSTER) )	return BBQServer::CLUSTER;
	else if( 0 == strcasecmp(strType, STR_PRIMARY) )	return BBQServer::PRIMARY;
	else if( 0 == strcasecmp(strType, STR_SECONDARY) )	return BBQServer::SECONDARY;

	return INVALID_TYPE;
}

uint32 BBQServer::StatusToInt( const char * strStatus )
{
	if( 0 == strcasecmp(strStatus, STR_ACTIVE) )		return BBQServer::ACTIVE;
	else if( 0 == strcasecmp(strStatus, STR_STANDBY) )	return BBQServer::STANDBY;
	else if( 0 == strcasecmp(strStatus, STR_DOWN) )		return BBQServer::DOWN;

	return INVALID_STATUS;
}

bool BBQServer::ValidateAdminPassword( const char * username, const char * password )
{
	return true;
}

bool BBQServer::OnMsgServerSwitch( const SIM_REQUEST * req )
{
	const SIMD_CS_SERVERSWITCH * cs = (const SIMD_CS_SERVERSWITCH *) & req->msg.simData;

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_SERVERSWITCH, sizeof(SIMD_SC_SERVERSWITCH), req );
	SIMD_SC_SERVERSWITCH * sc = (SIMD_SC_SERVERSWITCH *) & reply.msg.simData;
	memset( sc, 0, sizeof(*sc) );

	// check password here
	if( ValidateAdminPassword( cs->username, cs->password ) ) {
		if( m_config.nServerType == BBQServer::PRIMARY ) {
			PLOG( m_pLogger, Info, 
				PString( PString::Printf, 
				"Manual switch command from host: %s, username/password is valid, accept.", 
				(const char *)ToString( req->channel.peer.ip ) ) );

			switch( cs->serverType ) {
			case BBQServer::STANDALONE:
			case BBQServer::CLUSTER:
			case BBQServer::PRIMARY:
			case BBQServer::SECONDARY:
				if( m_config.nServerType != cs->serverType ) SwitchType( cs->serverType );
			break;
			}

			switch( cs->serverStatus ) {
			case BBQServer::ACTIVE:
			case BBQServer::STANDBY:
			case BBQServer::DOWN:
			if( m_status.nServerStatus != cs->serverStatus ) SwitchStatus( cs->serverStatus );
			break;
			}

			sc->statusCode = SIMS_OK;

			// if i am a primary or secondary server, i should notify the other to update status immediately
			if( (m_config.anotherServerAddress.ip != 0) && 
				(m_config.nServerType == BBQServer::PRIMARY || m_config.nServerType == BBQServer::SECONDARY) ) {

					SIM_REQUEST notify;
					SIM_REQINIT( notify, 0, req->channel.local.ip, req->channel.local.port,
						m_config.anotherServerAddress.ip, m_config.anotherServerAddress.port, 
						SIM_CS_SERVERSWITCH, sizeof(SIMD_CS_SERVERSWITCH) );

					SIMD_CS_SERVERSWITCH * ns = (SIMD_CS_SERVERSWITCH *) & req->msg.simData;
					memset( ns, 0, sizeof(SIMD_CS_SERVERSWITCH) );
					//ns->serverType = m_config.nServerType;
					//ns->serverStatus = m_status.nServerStatus;

					for(int i=0; i<3; i++) { SendMessage( & notify ); sleep_ms(0); }
				}
		} else {
			PLOG( m_pLogger, Info, 
				PString( PString::Printf, 
				"Manual switch command from host: %s, username/password is valid, but this is not a primary server, denied.", 
				(const char *)ToString( req->channel.peer.ip ) ) );

			sc->statusCode = SIMS_DENY;
		}
	} else if ( req->channel.peer.ip == m_config.anotherServerAddress.ip ) { 
		PLOG( m_pLogger, Info, 
			PString( PString::Printf, 
			"Received notifiy message: interlink server accepted manual switch command.", 
			(const char *)ToString( req->channel.peer.ip ) ) );

		// message from another server, ask me to update the status immediately
		m_dwInterlinkTickCalled = GetHostUpTimeInMs() - INTERLINK_TICK_TIME;

		// is the response required ?
		// return true;

	} else {
		PLOG( m_pLogger, Info, 
			PString( PString::Printf, 
			"Manual switch command from host: %s, username/password is invalid, deny.", 
			(const char *)ToString( req->channel.peer.ip ) ) );

		sc->statusCode = SIMS_DENY;
	}

	sc->serverType = m_config.nServerType;
	sc->serverStatus = m_status.nServerStatus;

	SendMessage( & reply );

	return true;
}

bool BBQServer::SwitchType( uint32 nType )
{
	PLOG( m_pLogger, Info, 
		PString( PString::Printf, "Server Type switch from %s to %s.", 
		(const char *) TypeToString(m_config.nServerType), 
		(const char *) TypeToString(nType) ) );

	PString folder( "logs" );
	folder += PDirectory::IsSeparator('\\') ? "\\" : "/";

	if( m_config.write_hearbeat_file ) {
		unlink( folder +  TypeToString( m_config.nServerType ) );
	}

	m_config.nServerType = nType;

	// TODO: switch server type, such a big affair, should we do something?

	if( m_config.write_hearbeat_file ) {
		PFileLog::TouchFile( folder +  TypeToString( m_config.nServerType ) );
	}

	return true;
}

bool BBQServer::SwitchStatus( uint32 nStatus )
{
	PLOG( m_pLogger, Info, 
		PString( PString::Printf, "Server Status switch from %s to %s.", 
		(const char *) StatusToString(m_status.nServerStatus), 
		(const char *) StatusToString(nStatus) ) );

	PString folder( "logs" );
	folder += PDirectory::IsSeparator('\\') ? "\\" : "/";

	if( m_config.write_hearbeat_file ) {
		unlink( folder + StatusToString( m_status.nServerStatus ) );
	}

	m_status.nServerStatus = nStatus;

	// call database interface, write the status into database, required by DB backup/restore app
	if( m_pDB && ( m_pDB->GetLastErrorCode() != BBQDatabase::ServerFail ) ) {
		m_pDB->EnableServerActivity( m_status.nServerStatus == BBQServer::ACTIVE );
	}

	if( m_config.write_hearbeat_file ) {
		PFileLog::TouchFile( folder +  StatusToString( m_status.nServerStatus ) );
	}

	return (m_status.nServerStatus == nStatus);
}

// this function will be called by a thread every 30 seconds
void BBQServer::InterlinkTick( void )
{
	// keep checking db status
	if( m_status.nServerStatus != BBQServer::DOWN ) {
		if( ! m_pDB ) {
			// no DB object, VFON server cannot work.
			OnEventDBFail();
		} else {
			if ( m_pDB->GetLastErrorCode() == BBQDatabase::ServerFail ) {
				if( m_config.ignore_db_err ) {
					// error, but ignore
				} else {
					OnEventDBFail();
				}
			} else {
				// no error
			}
		}
	}

	// touch heartbeat file, for Big Brother to monitor
	if( m_config.write_hearbeat_file ) {
		PString folder( "logs" );
		folder += PDirectory::IsSeparator('\\') ? "\\" : "/";

		PFileLog::TouchFile( folder +  TypeToString( m_config.nServerType ) );
		PFileLog::TouchFile( folder +  StatusToString( m_status.nServerStatus ) );
	}

	if( m_config.check_dns_in_heartbeat ) {
		PConfig cfg( KEY_BBQServer );
		PString strAnotherServerAddress = cfg.GetString( KEY_AnotherServerIp, "0.0.0.0" );
		PString iphost_string;
		DWORD ip = 0;
		const char * st = (const char *) strAnotherServerAddress;
		const char * p = strstr( st, ":" );
		if( p ) {
			iphost_string = PString( st, (p - st) );
			m_config.anotherServerAddress.port = strtol( p+1, NULL, 0 );
		} else {
			iphost_string = st;
			m_config.anotherServerAddress.port = SIM_PORT;
		}
		if( HostnameToIp( iphost_string, ip ) ) {
			if( ip != m_config.anotherServerAddress.ip ) {
				PLOG( m_pLogger, Info, PString(PString::Printf, "Interlink server '%s' is now resolved to: %s, changed.", (const char *) iphost_string, (const char *) ToString( ip ) ) );
				m_config.anotherServerAddress.ip = ip;
			} else {
				// no change
			}
		} else {
			// failed to resolve hostname
			if( 0 != m_config.anotherServerAddress.ip ) {
				PLOG( m_pLogger, Info, PString(PString::Printf, "Failed to resolve interlink server '%s' to IP.", (const char *) iphost_string ) );
				m_config.anotherServerAddress.ip = 0;
			} else {
				// no change
			}
		}
	}

	// another server is not configured
	if( (m_config.anotherServerAddress.ip == 0) && (m_config.anotherServerAddress2.ip == 0) ) return;

	// no interlink is needed for standalone server
	if( (m_config.nServerType == BBQServer::STANDALONE) ) return;

	// validate the interlink channel
	if( m_config.anotherServerAddress.ip != 0 ) {
		m_chanInterlink[0].peer = m_config.anotherServerAddress;
		if( m_config.use_tcp_for_server_link ) {
			if( ! ValidateTcpChannel( m_chanInterlink[0] ) ) return;
		} else {
			m_chanInterlink[0].local.ip = 0;
			m_chanInterlink[0].local.port = m_nDefaultUdpPort;
		}
	}

	// validate the second interlink channel
	//if( m_config.anotherServerAddress2.ip != 0 ) {
	//	m_chanInterlink[1].peer = m_config.anotherServerAddress2;
	//	if( m_config.use_tcp_for_server_link ) {
	//		if( ! ValidateTcpChannel( m_chanInterlink[1] ) ) return;
	//	} else {
	//		m_chanInterlink[1].local.ip = 0;
	//		m_chanInterlink[1].local.port = m_nDefaultUdpPort;
	//	}
	//}

	SIM_REQUEST req;

	// server -> server update
	{
		SIM_REQINITCHAN( req, m_chanUplink[0], SIM_SS_SERVER_UPDATE, sizeof(SIMD_SS_SERVER_UPDATE) );
		SIMD_SS_SERVER_UPDATE * pQ = (SIMD_SS_SERVER_UPDATE *) & req.msg.simData;
		memset( pQ, 0, sizeof(*pQ) );

		pQ->flags.uplink = 1;

		SIMD_SERVER_INFO * p = & pQ->server;

		p->timestamp = time(NULL);

		p->dwIp = 0;

		//p->nAdminPort = ;
		p->nTcpPort = m_nDefaultTcpPort;
		p->nUdpPortMin = m_nMinUdpPort;
		p->nUdpPortMax = m_nMaxUdpPort;

		//p->dwCookie = 0;

		p->tUpTime = m_status.tUpTime;

		p->nServerType = m_config.nServerType;
		p->nServerStatus = m_status.nServerStatus;

		p->nMaxUsers = m_config.nUserMax;
		p->nOnlineUsers = m_status.nOnlineUsers;
		p->nMCUServers = m_status.nMCUCount;
		p->nProxyServers = m_status.nProxyCount;

		SendMessage( & req, true );
	}

	// client -> server status
	SIM_REQINITCHAN( req, m_chanInterlink[0], SIM_CS_SERVERSTATUS, 0 );
	SIMD_CS_SERVERSTATUS * pQ = (SIMD_CS_SERVERSTATUS *) & req.msg.simData;
	pQ->link_type = INTERLINK;

	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_SERVERSTATUS, 2000, 5 ); // a bug that might make server thread deadlock
	if( reply ) {
		SIMD_SC_SERVERSTATUS * sc = (SIMD_SC_SERVERSTATUS *) & reply->msg.simData;

		m_status.nAnotherServerType = sc->nServerType;
		m_status.nAnotherServerStatus = sc->nServerStatus;
		m_status.tAnotherServerUpTime = time(NULL) - (sc->tNowTime - sc->tUpTime);

		if( m_config.write_hearbeat_file ) {
			PString path( "logs" );
			path += PDirectory::IsSeparator('\\') ? "\\" : "/";
			path += STR_AnotherAlive;
			PFileLog::TouchFile( path );
		}

		if( ! m_status.nInterlinkConnected ) {
			PLOG( m_pLogger, Info, 
				PString( PString::Printf, "Interlink server is connected, and resported as: %s, %s.", 
				(const char *)TypeToString(m_status.nAnotherServerType),
				(const char *)StatusToString(m_status.nAnotherServerStatus)
				) );
		}

		m_status.nInterlinkConnected = TRUE;

		// check the server type, and solve type conflict if there is.
		switch( sc->nServerType ) {
		case BBQServer::PRIMARY:
			if( m_config.nServerType == PRIMARY ) {
				// error: both are primary, 
				// one of us should be degrade to secondary
				time_t tMyUptime = m_status.tUpTime;
				time_t tPeerUptime = sc->tUpTime - sc->tNowTime + time(NULL);
				if( tMyUptime > tPeerUptime ) SwitchType( BBQServer::SECONDARY );
			}

			// now check the status
			{
				switch( sc->nServerStatus ) {
				case BBQServer::ACTIVE:
					// peer is primary and active, check my status
					if( m_status.nServerStatus == ACTIVE ) {
						PLOG( m_pLogger, Info, "Interlink server is reported as PRIMARY and ACTIVE." );
						SwitchStatus( BBQServer::STANDBY );
					}
					break;
				case BBQServer::STANDBY:
					// peer is primary and standby, 
					// keep no change even if i am standby, peer will switch to active
					break;
				case BBQServer::DOWN:
					// another server is down
					if(m_status.nServerStatus == BBQServer::STANDBY) {
						PLOG( m_pLogger, Info, "Interlink server is reported as DOWN." );
						SwitchStatus( BBQServer::ACTIVE );
					}
					break;
				}
			}

			break;
		case BBQServer::SECONDARY:
			if( m_config.nServerType == SECONDARY ) {
				// error: both are secondary, 
				// one of us should be upgrade to primary
				time_t tMyUptime = m_status.tUpTime;
				time_t tPeerUptime = sc->tUpTime - sc->tNowTime + time(NULL);
				if( tMyUptime < tPeerUptime ) SwitchType( BBQServer::PRIMARY );
			}

			// now check the status
			{
				switch( sc->nServerStatus ) {
				case BBQServer::ACTIVE:
					// peer is secondary and active, 
					// keep no change even if i am active, peer will switch to standby
					break;
				case BBQServer::STANDBY:
					// peer is secondary and standby, i switch to active
					if( m_status.nServerStatus == BBQServer::STANDBY ) {
						PLOG( m_pLogger, Info, "Interlink server is reported as SECONDARY and STANDBY." );
						SwitchStatus( BBQServer::ACTIVE );
					}
					break;
				case BBQServer::DOWN:
					// another server is down
					if(m_status.nServerStatus == BBQServer::STANDBY) {
						PLOG( m_pLogger, Info, "Interlink server is reported as DOWN." );
						SwitchStatus( BBQServer::ACTIVE );
					}
					break;
				}
			}
			break;
		case BBQServer::CLUSTER:
			break;
		case BBQServer::STANDALONE:
			break;
		}

		ReleaseMessage( reply );

	} else {
		// in first 1 minutes of server startup, the system might not be stable enough, ignore the result
		if( m_status.tUpTime + ONE_MINUTE > time(NULL) ) return;

		// no response from peer, he is down???
		if( m_status.nInterlinkConnected ) {
			PLOG( m_pLogger, Info, "Lost connection to interlink server." );

			if( m_config.write_hearbeat_file ) {
				PString path( "logs" );
				path += PDirectory::IsSeparator('\\') ? "\\" : "/";
				path += STR_AnotherAlive;
				unlink( path );
			}
		}

		m_status.nInterlinkConnected = FALSE;

		switch( m_status.nServerStatus ) {
		case BBQServer::ACTIVE:
			// i am already active, so ignore
			break;
		case BBQServer::STANDBY:
			PLOG( m_pLogger, Info, "Interlink server is assumed as DOWN, will do auto switch now." );
			SwitchStatus( BBQServer::ACTIVE );
			break;
		case BBQServer::DOWN:
			// i am down, and the backup cannot be reached, serious problem!!!!!!!!!!!
			break;
		}
	}
}

void BBQServer::UplinkTick( void )
{
	// only domain server do the uplink work, or else skip
	if( ! m_config.run_as_cluster_center ) return;

	// uplink server is not configured
	if( m_config.uplinkServerAddress.ip == 0 ) return;

	// validate the uplink channel
	m_chanUplink[0].peer = m_config.uplinkServerAddress;
	if( m_config.use_tcp_for_server_link ) {
		if( ! ValidateTcpChannel( m_chanUplink[0] ) ) return;
	} else {
		m_chanUplink[0].local.ip = 0;
		m_chanUplink[0].local.port = m_nDefaultUdpPort;
	}

	// update domain info to parent node
	SIM_REQUEST req;
	SIM_REQINITCHAN( req, m_chanUplink[0], SIM_CS_SERVERSTATUS, 0 );
	SendMessage( & req, true );

	SIM_REQINITCHAN( req, m_chanUplink[0], SIM_SS_DOMAIN_UPDATE, sizeof(SIMD_SS_DOMAIN_UPDATE) );
	SIMD_SS_DOMAIN_UPDATE * pQ = (SIMD_SS_DOMAIN_UPDATE *) & req.msg.simData;
	memset( pQ, 0, sizeof(*pQ) );

	pQ->flags.uplink = 1;

	pQ->domain = m_config.domainInfo;

	SIMD_SERVER_INFO * p = & pQ->server;

	p->timestamp = time(NULL);

	p->dwIp = 0;

	//p->nAdminPort = ;
	p->nTcpPort = m_nDefaultTcpPort;
	p->nUdpPortMin = m_nMinUdpPort;
	p->nUdpPortMax = m_nMaxUdpPort;

	//p->dwCookie = 0;

	p->tUpTime = m_status.tUpTime;

	p->nServerType = m_config.nServerType;
	p->nServerStatus = m_status.nServerStatus;

	p->nMaxUsers = m_config.nUserMax;
	p->nOnlineUsers = m_status.nOnlineUsers;
	p->nMCUServers = m_status.nMCUCount;
	p->nProxyServers = m_status.nProxyCount;

	SendMessage( & req, true );

	return;

	SIM_REQUEST * reply = RequestMessage( & req, SIM_SC_SERVERSTATUS, 3000, 10 );
	if( reply ) {
		m_status.nUplinkConnected = TRUE;

		SIMD_SC_SERVERSTATUS * sc = (SIMD_SC_SERVERSTATUS *) & reply->msg.simData;

		m_status.nUplinkServerType = sc->nServerType;
		m_status.nUplinkServerStatus = sc->nServerStatus;
		m_status.tUplinkServerUpTime = time(NULL) - (sc->tNowTime - sc->tUpTime);

		ReleaseMessage( reply );
	} else {
		// no response from peer, is the uplink server down ???
		m_status.nUplinkConnected = FALSE;
	}
}

bool BBQServer::OnMsgClientGetMCUList( const SIM_REQUEST * req )
{
	SIMD_CS_GETMCULIST * pQ = (SIMD_CS_GETMCULIST *) & req->msg.simData;

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_GETMCULIST, sizeof(SIMD_SC_GETMCULIST), req );
	SIMD_SC_GETMCULIST * pA = (SIMD_SC_GETMCULIST *) & reply.msg.simData;
	memset( pA, 0, sizeof(SIMD_SC_GETMCULIST) );

	{
		WriteLock lock( m_mutexRecord );

		MCUMap::iterator iter = m_mapMCU.begin(), eiter = m_mapMCU.end();
		int i = 0;
		while( iter != eiter ) {
			if( i >= pQ->nBase ) break;
			i ++;
			iter ++;
		}
		pA->nBase = i;
		pA->nTotal = m_mapMCU.size();

		i = 0;
		if( pA->nBase < pA->nTotal ) {
			while( iter != eiter ) {
				if( i >= 20 ) break;

				uint32 id = iter->first;
				MCURecord * pMCU = iter->second;
				SFIDRecord * pRec = FindRecordByID( id );
				if( pRec ) {
					pA->inMCU[i].vfonId.serverid = 0;
					pA->inMCU[i].vfonId.userid = id;
					memcpy( pA->inMCU[i].alias, pRec->pUserInfo->basic.alias, USERDATA_NAME_SIZE );
					pA->inMCU[i].rooms = pMCU->rooms;
					pA->inMCU[i].persons = pMCU->persons;
					pA->inMCU[i].bps = pMCU->bps;
					i ++;
				}

				iter ++;
			}

		}
		pA->nCount = i;
		reply.msg.simHeader.size = sizeof(SIMD_SC_GETMCULIST) + sizeof(MCURecord) * i;
	}

	SendMessage( & reply );

	return true;
}

bool BBQServer::OnMsgSXUpdateSwitchInfo( const SIM_REQUEST * req )
{
	SIMD_SX_UPDATESWITCHINFO * pQ = (SIMD_SX_UPDATESWITCHINFO *) req->msg.simData;

	// check pQ->sessionInfo here for authentication

	// if login server has not sent its IP, we assign it with what we see
	if( pQ->info.addr.ip == 0 ) pQ->info.addr.ip = req->channel.peer.ip;

	if( pQ->info.login ) {
		// notify old one to exit
		SWITCHINFO oldinfo;
		memset( & oldinfo, 0, sizeof(oldinfo) );
		if( m_bbqSwitch.Get( pQ->id, oldinfo ) ) {

			if( oldinfo.addr.ip && (oldinfo.addr.ip != pQ->info.addr.ip) ) {
				// login replaced
				SIM_REQUEST notify;
				SIM_REQINITCHAN( notify, oldinfo.channel, SIM_SC_LOGINREPLACED, sizeof(SIMD_XS_LOGINREPLACED) );
				SIMD_XS_LOGINREPLACED * pA = (SIMD_SX_UPDATESWITCHINFO *) & notify.msg.simData;
				* pA = * pQ;
				SendMessage( & notify );
			}

		}
	}

	// save the channel info, we will send notifying message later
	pQ->info.channel = req->channel;

	// put to the HUGE table
	m_bbqSwitch.Put( pQ->id, pQ->info );

	return true;
}

bool BBQServer::OnMsgXSLoginReplaced( const SIM_REQUEST * req )
{
	SIMD_XS_LOGINREPLACED * pQ = (SIMD_XS_LOGINREPLACED *) req->msg.simData;

	SFIDRecord * pRec = FindRecordByID( pQ->id );
	if( pRec ) {
		SIM_REQUEST notify;
		SIM_REQINITCHAN( notify, pRec->sockChannel, SIM_SC_LOGINREPLACED, sizeof(SIMD_SC_LOGINREPLACED) );

		SIMD_SC_LOGINREPLACED * pN = (SIMD_SC_LOGINREPLACED *) & notify.msg.simData;
		pN->sessionInfo = pRec->sessionInfo;
		pN->addrReplaceBy = pQ->info.channel.peer;

		SendMessage( & notify );
	}

	return true;
}

bool BBQServer::OnMsgSXQuerySwitchInfo( const SIM_REQUEST * req )
{
	return true;
}

bool BBQServer::OnMsgXSAnswerSwitchInfo( const SIM_REQUEST * req )
{
	return true;
}

// msg handler in cluster central server
bool BBQServer::OnMsgSXQueryLocateUser( const SIM_REQUEST * req )
{
	SIMD_SX_QLOCATE * pQ = (SIMD_SX_QLOCATE *) & req->msg.simData;

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_XS_ALOCATE, sizeof(SIMD_XS_ALOCATE), req );
	SIMD_XS_ALOCATE * pA = (SIMD_XS_ALOCATE *) & reply.msg.simData;

	* pA = * pQ;

	bool bFound = m_bbqSwitch.Get( pA->id, pA->info );
	pA->statusCode = bFound ? SIMS_EXISTS : SIMS_NOTFOUND;

	SendMessage( & reply );

	return true;
}

// msg handler in cluster login server, forward back to client
bool BBQServer::OnMsgXSAnswerLocateUser( const SIM_REQUEST * req )
{
	SIMD_XS_ALOCATE * pQ = (SIMD_XS_ALOCATE *) & req->msg.simData;

	SIM_REQUEST reply;
	SIM_REQINITCHAN( reply, pQ->queryChannel, SIM_SC_LOCATE, sizeof(SIMD_SC_LOCATE) );
	SIMD_SC_LOCATE * pA = (SIMD_SC_LOCATE *) & reply.msg.simData;
	memset( pA, 0, sizeof(*pA) );

	pA->queryID = pQ->id;

	//pA->addrLAN = pRec->netLAN;
	//pA->addrWAN = pRec->sockChannel.peer;

	pA->isTCP = pQ->info.is_tcp ? 1 : 0;
	pA->firewallType = pQ->info.firewall_type & 0x03;
	pA->addrServer.ip = pQ->info.addr.ip;
	pA->addrServer.port = pQ->info.is_tcp ? pQ->info.addr.tcpPort : pQ->info.addr.udpPort;

	pA->statusCode = pQ->statusCode;

	SendMessage( & reply );

	return true;
}

// ------------------------------- user tree features --------------------------------------
bool BBQServer::GetAllParentUserNodes( uint32 nId, RecordMap & result, RecordMap * pSearchedNodes, int depth )
{
	if( depth >= TREE_MAX_DEPTH ) return false;	// too depth, sth is wrong.

	if( result.find( nId ) != result.end() ) return true; // already in result list

	RecordMap * pSearched = NULL;

	// remember every nodes that we have searched, if already searched, then skip.
	if( pSearchedNodes ) {
		if( pSearchedNodes->find( nId ) != pSearchedNodes->end() ) return true;

		pSearched = pSearchedNodes;
	} else {
		pSearched = new RecordMap; 	// if no searched list, we create one
	}

	SFIDRecord * pRec = FindRecordByID( nId );
	(* pSearched)[ nId ] = pRec;

	if( pRec ) {
		if( pRec->pParentIdList ) {
			result[ nId ] = pRec;

			unsigned int n = pRec->pParentIdList->Count();
			if( n > 0 ) {
				uint32 * p = pRec->pParentIdList->Data();
				for( unsigned int i=0; i<n; i++, p++ ) {
					GetAllParentUserNodes( *p, result, pSearched, depth +1 );
				}
			}
		} else { // already reach root
			if( m_config.run_in_asp_mode ) {
				// when run CUG in ASP mode, the second layer is isolated group, the root is company.
				// so skip
			} else {
				result[ nId ] = pRec;
				// when run CUG in Enterprise mode, the root is isolated group
			}
		}
	}

	if( ! pSearchedNodes ) { // this is the starting call
		// created by me, so release before return
		if( pSearched ) { delete pSearched;  }

		// must contain myself
		if( pRec && (result.find( nId ) == result.end() ) ) result[ nId ] = pRec;
	}

	return true;
}


bool BBQServer::GetRootUserNodes( uint32 nId, RecordMap & result, RecordMap * pSearchedNodes, int depth )
{
	if( depth >= TREE_MAX_DEPTH ) {
		return false;	// too depth, sth is wrong.
	}

	if( result.find( nId ) != result.end() ) return true; // already in result list

	SFIDRecord * pRec = FindRecordByID( nId );
	if( NULL == pRec ) return false;  // id not exists

	RecordMap * pSearched = NULL;

	// remember every nodes that we have searched, if already searched, then skip.
	if( pSearchedNodes ) {
		if( pSearchedNodes->find( nId ) != pSearchedNodes->end() ) return false;

		pSearched = pSearchedNodes;
	} else {
		pSearched = new RecordMap; 	// if no searched list, we create one
	}

	(* pSearched)[ nId ] = pRec; // we record every node that we have searched

	bool bDone = false;

	//
	// CUG TODO: specified every group as closed or not
	//
	//if(  pRec->pServiceInfo && pRec->pServiceInfo->is_closed_group ) { // closed group never search up any more
	//	result[ nId ] = pRec;
	//	bDone = true;
	//} else if( pRec->pParentIdList && (pRec->pParentIdList->Count() > 0) ) { // have parents, non-root node
	//	unsigned int n = pRec->pParentIdList->Count(), i = 0;
	//	uint32 * p = pRec->pParentIdList->Data();
	//	for( i=0; i<n; i++, p++ ) {
	//		if( GetRootUserNodes( *p, result, pSearched, depth +1 ) ) bDone = true;
	//	}
	//} else { // root already
	//	result[ nId ] = pRec;
	//	bDone = true;
	//}

	if( pRec->pParentIdList && (pRec->pParentIdList->Count() > 0) ) { // have parents
		unsigned int n = pRec->pParentIdList->Count(), i = 0;
		uint32 * p = pRec->pParentIdList->Data();
		for( i=0; i<n; i++, p++ ) {
			if( GetRootUserNodes( *p, result, pSearched, depth +1 ) ) {
				bDone = true;
			} else { 
				// parent id is invalid or cannot be used
			}
		}
		//if( m_config.run_in_asp_mode ) {
		if( ! bDone ) { // we have parent nodes, but cannot be used, so this is isolated group node 
			result[ nId ] = pRec;
			bDone = true;
		}
		//}
	} else { 
		// no parent id, so this is the root node
		if( m_config.run_in_asp_mode && (depth > 1) ) {
			// search from close group up, deny
		} else {
			result[ nId ] = pRec;
			bDone = true;
		}
	}

	if( ! pSearchedNodes ) {
		if( pSearched ) { delete pSearched;  }
	}

	return bDone;
}

bool BBQServer::GetSubUserTree( uint_list & idList, BBQTrees & trees, int depth, uint32 idSearchStart ) // get the sub trees for these nodes
{
	if( depth >= TREE_MAX_DEPTH ) return false;	// too depth, sth is wrong.

	for( uint_list::iterator it = idList.begin(), et = idList.end(); it != et; it ++ ) {
		uint32 id = * it;

		// check the id in the tree, if already exist, just skip
		if( trees.m_mapTreeNodes.find( id ) != trees.m_mapTreeNodes.end() ) continue;

		SFIDRecord * pRec = FindRecordByID( id );
		if( pRec && pRec->pChildIdList ) {
			// 
			// CUG TODO: for a closed group, skip seek down, unless this is the start point
			//
			//if( pRec->pServiceInfo && pRec->pServiceInfo->is_closed_group && (depth>0) ) {
			//	// for a closed group, skip seek down, unless this is the start point
			//	continue;
			//} else {
			//	uint32 * p = pRec->pChildIdList->Data();
			//	int n = pRec->pChildIdList->Count(), i = 0;
			//	if( n > 0 ) {
			//		uint_list * pList = new uint_list; 
			//		if( pList ) {
			//			for( i=0; i < n; i++ ) pList->push_back( * p ++ );
			//			trees.m_mapTreeNodes[ id ] = pList;
			//			GetSubUserTree( * pList, trees, depth +1 );
			//		}
			//	}
			//}

			bool bRootNode = ! ( pRec->pParentIdList && ( pRec->pParentIdList->Count() > 0 ));
			uint32 * p = pRec->pChildIdList->Data();
			int n = pRec->pChildIdList->Count(), i = 0;
			if( n > 0 ) {
				uint_list listSearch; bool bSelfCUG = false;
				uint_list * pList = new uint_list; 
				if( pList ) {
					for( i=0; i < n; i++ ) {
						uint32 x = * p ++;
						pList->push_back( x );

						if( m_config.run_in_asp_mode && bRootNode ) {
							if( x == idSearchStart ) {
								listSearch.push_back( x );
								bSelfCUG = true;
							}
						} else {
							listSearch.push_back( x );
						}
					}
					trees.m_mapTreeNodes[ id ] = pList;
					GetSubUserTree( (bSelfCUG ? listSearch : (*pList) ), trees, depth +1 );
				}
			}
		}
	}

	return true;
}

bool BBQServer::GetSubUserRecords( uint_list & idList, RecordMap & records, int depth ) // get the sub trees for these nodes
{
	if( depth >= TREE_MAX_DEPTH ) return false;	// too depth, sth is wrong.

	for( uint_list::iterator it = idList.begin(), et = idList.end(); it != et; it ++ ) {
		uint32 id = * it;

		// check the id in the records, if already exist, just skip
		if( records.find( id ) != records.end() ) continue;

		SFIDRecord * pRec = FindRecordByID( id );
		if( pRec ) {
			records[ id ] = pRec;

			if( pRec->pChildIdList ) {
				int n = pRec->pChildIdList->Count();
				uint32 * p = pRec->pChildIdList->Data();
				if( n > 0 ) {
					uint_list idSubList;
					for( int i=0; i < n; i++ ) idSubList.push_back( * p ++ );

					GetSubUserRecords( idSubList, records, depth +1 );
				}
			}
		}
	}

	return true;
}

bool BBQServer::IsContainedInSameTree( uint32 nIdA, uint32 nIdB )
{
	RecordMap mapA, mapB;

	if( GetRootUserNodes( nIdA, mapA ) && GetRootUserNodes( nIdB, mapB ) ) {
		for( RecordMap::iterator it = mapA.begin(), et = mapA.end(); it != et; it ++ ) {
			if( mapB.find( it->first ) != mapB.end() ) return true;	// they have at least one same root node, so in the same tree
		}
	}

	return false;
}

bool BBQServer::IsMyParent( uint32 nMe, uint32 nGuest )
{
	RecordMap mapMe;
	if( GetAllParentUserNodes( nMe, mapMe ) ) {
		if( mapMe.find( nGuest ) != mapMe.end() ) return true;
	}

	return false;
}

bool BBQServer::IsBuddyOfMyParents( uint32 nMe, uint32 nGuest )
{
	RecordMap mapMe, mapGuest;
	if( GetAllParentUserNodes( nMe, mapMe ) && GetAllParentUserNodes( nGuest, mapGuest ) ) {
		for( RecordMap::iterator it = mapMe.begin(), et = mapMe.end(); it != et; it ++ ) {
			SFIDRecord * pParent = it->second;
			if( pParent && pParent->pFriendList ) {
				UserIdList * pList = pParent->pFriendList;

				// he is buddy of one of my parent
				if( pList->Contain( nGuest ) ) return true;

				for( RecordMap::iterator ig = mapGuest.begin(), eg = mapGuest.end(); ig != eg; ig ++ ) {
					// one of his parent is buddy of one of my parent
					if( pList->Contain( ig->first ) ) return true;
				}
			}
		}
	}

	return false;
}

bool BBQServer::IsBlockedByMeAndParents( uint32 nMe, uint32 nGuest )
{
	RecordMap mapMe, mapGuest;
	if( GetAllParentUserNodes( nMe, mapMe ) && GetAllParentUserNodes( nGuest, mapGuest ) ) {

		// allow guest call out or not
		for( RecordMap::iterator it = mapGuest.begin(), et = mapGuest.end(); it != et; it ++ ) {
			SFIDRecord * pParent = it->second;
			if( pParent && pParent->flags.no_callout ) return true;
		}

		// check block list first
		for( RecordMap::iterator it = mapMe.begin(), et = mapMe.end(); it != et; it ++ ) {
			SFIDRecord * pParent = it->second;

			if( pParent && pParent->flags.no_callin ) return true;

			if( pParent && pParent->pBlockList ) {
				UserIdList * pList = pParent->pBlockList;

				// he is blocked by one of my parent
				if( pList->Contain( nGuest ) ) return true;

				for( RecordMap::iterator ig = mapGuest.begin(), eg = mapGuest.end(); ig != eg; ig ++ ) {
					// one of his parent is blocked by one of my parent
					if( pList->Contain( ig->first ) ) return true;
				}
			}
		}
	}

	return false;
}

bool BBQServer::IsAllowedByMeAndParents( uint32 nMe, uint32 nGuest )
{
	RecordMap mapMe, mapGuest;
	if( GetAllParentUserNodes( nMe, mapMe ) && GetAllParentUserNodes( nGuest, mapGuest ) ) {

		// allow guest call out or not
		for( RecordMap::iterator it = mapGuest.begin(), et = mapGuest.end(); it != et; it ++ ) {
			SFIDRecord * pParent = it->second;
			if( pParent && pParent->flags.no_callout ) return false;
		}

		// check block list first
		for( RecordMap::iterator it = mapMe.begin(), et = mapMe.end(); it != et; it ++ ) {
			SFIDRecord * pParent = it->second;

			if( pParent && pParent->flags.no_callin ) return false;

			if( pParent && pParent->pBlockList ) {
				UserIdList * pList = pParent->pBlockList;

				// he is blocked by one of my parent
				if( pList->Contain( nGuest ) ) return false;

				for( RecordMap::iterator ig = mapGuest.begin(), eg = mapGuest.end(); ig != eg; ig ++ ) {
					// one of his parent is blocked by one of my parent
					if( pList->Contain( ig->first ) ) return false;
				}
			}
		}

		// then check buddy list
		for( RecordMap::iterator it = mapMe.begin(), et = mapMe.end(); it != et; it ++ ) {
			SFIDRecord * pParent = it->second;
			if( pParent && pParent->pFriendList ) {
				UserIdList * pList = pParent->pFriendList;

				// he is buddy of one of my parent
				if( pList->Contain( nGuest ) ) return true;

				for( RecordMap::iterator ig = mapGuest.begin(), eg = mapGuest.end(); ig != eg; ig ++ ) {
					// one of his parent is buddy of one of my parent
					if( pList->Contain( ig->first ) ) return true;
				}
			}
		}

	}

	return false;
}

bool BBQServer::GetRootUserTrees( uint32 nMe, BBQTrees & trees )
{
	trees.RemoveAll();

	RecordMap mapMe;
	if( GetRootUserNodes( nMe, mapMe ) && ( mapMe.size() > 0 ) ) {
		for( RecordMap::iterator it = mapMe.begin(), et = mapMe.end(); it != et; it ++ ) {
			trees.m_listRootValues.push_back( it->first );
		}
		GetSubUserTree( trees.m_listRootValues, trees, 0, nMe );

		return true;
	}

	return false;
}

bool BBQServer::OnMsgClientGetIdTrees( const SIM_REQUEST * req )
{
	const SIMD_CS_GETIDTREES * pQ = (const SIMD_CS_GETIDTREES *) & req->msg.simData;

	ReadLock safe( m_mutexRecord );

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_GETIDTREES, 0, req );

	uint32 statusCode = SIMS_OK;
	PBytePack packedData, packedMsg;

	if( m_config.hide_id_tree_to_client ) {
		// send an empty id tree
	} else {
		SFIDRecord * pRec = FindRecordByID( pQ->sessionInfo.id );
		if( pRec && ( pRec->sessionInfo.cookie == pQ->sessionInfo.cookie ) ) {
			BBQTrees trees;
			if( GetRootUserTrees( pQ->nId, trees ) ) {
				if( trees.Exists( pQ->nId ) ) {
					packedData << trees;
				} else {
					int err = 1; // get a invalid tree
					if( err == 1 ) err = 2;
				}
			} else {
				statusCode = SIMS_NOTFOUND;
			}
		} else {
			statusCode = SIMS_DENY;
		}
	}

	reply.msg.simHeader.size = sizeof(statusCode) + sizeof(pQ->nId) + packedData.Size();
	packedMsg << reply.channel << reply.msg.simHeader << statusCode << pQ->nId << packedData;

	SendMessage( (SIM_REQUEST *) packedMsg.Data() );

	return true;
}

int BBQServer::SyncBuddyRecordFromDB( uint32 uid )
{
	ReadLock safe( m_mutexRecord );

	int n = 0;

	SFIDRecord * pRec = FindRecordByID( uid, true );
	if( pRec ) {
		n ++;

		if( pRec->pFriendList ) {
			uint32 * p = pRec->pFriendList->Data();
			int nCount = pRec->pFriendList->Count();

			for( int i=0; i<nCount; i++ ) {
				SFIDRecord * pBuddy = FindRecordByID( *p, true );
				if( pBuddy ) {
					NotifyUserServiceChanged( pBuddy );
					n ++;
				}
				p++;
			}
		}
	}

	return n;
}

int BBQServer::SyncGroupRecordFromDB( uint32 uid )
{
	ReadLock safe( m_mutexRecord );

	int n = 0;

	uint_list idList;

	// get all root ids
	RecordMap records;
	if( GetRootUserNodes( uid, records ) ) {

		for( RecordMap::iterator it = records.begin(), et = records.end(); it != et; it ++ ) {
			idList.push_back( it->first );
		}
	}
	records.clear();

	if( GetSubUserRecords( idList, records ) ) {

		for( RecordMap::iterator it = records.begin(), et = records.end(); it != et; it ++ ) {
			//if( NULL != FindRecordByID( it->first, true ) ) n ++;
			SFIDRecord * pRec = it->second;
			if( pRec && UpdateRecordCacheFromDB( pRec ) ) {
				NotifyUserServiceChanged( pRec, true );
				n ++;
			}
		}
	}

	return n;
}

bool BBQServer::OnMsgSSServerUpdate( const SIM_REQUEST * req )
{
	WriteLock safe( m_mutexRecord );

	const SIMD_SS_SERVER_UPDATE * pQ = (const SIMD_SS_SERVER_UPDATE *) & req->msg.simData;

	DWORD dwIp = pQ->server.dwIp;
	if( dwIp == 0 ) dwIp = req->channel.peer.ip;

	// update local server list
	{
		BBQServerRecord * pServer = NULL;

		ServerMap::iterator fit = m_mapLocalServers.find( dwIp );
		if( fit != m_mapLocalServers.end() ) { // already in our list
			pServer = fit->second;
		} else { // it's a new server
			pServer = new BBQServerRecord;
			memset( pServer, 0, sizeof(*pServer) );
			m_mapLocalServers[ dwIp ] = pServer;
		}

		if( pServer ) {
			pServer->sockChannel = req->channel;
			pServer->server = pQ->server;
			pServer->server.dwIp = dwIp;

			if( m_config.run_as_cluster_center && pQ->flags.uplink ) {
				pServer->flags.direct_connect = 1;
			}
		}
	}

	// if this is the domain cluster center, the update info should be forwarded to other login servers
	if( m_config.run_as_cluster_center && pQ->flags.uplink ) {

		SIM_REQUEST forward;
		SIM_REQINITCHAN( forward, req->channel, SIM_SS_SERVER_UPDATE, sizeof(SIMD_SS_SERVER_UPDATE) );
		SIMD_SS_SERVER_UPDATE * pF = (SIMD_SS_SERVER_UPDATE *) & forward.msg.simData;
		* pF = * pQ;

		// change uplink to boradcast, so that the receiver will not loop this message again
		pF->flags.uplink = 0;
		pF->flags.broadcast = 1;

		// update the ip of server
		pF->server.dwIp = dwIp;

		for( ServerMap::iterator it = m_mapLocalServers.begin(), eit = m_mapLocalServers.end(); it != eit; it ++ ) {
			if( it->first != dwIp ) {
				BBQServerRecord * pServer = it->second;
				if( pServer && pServer->flags.direct_connect ) {
					forward.channel = pServer->sockChannel;
					SendMessage( & forward );
				}
			}
		}
	}

	return true;
}

bool BBQServer::GetDomainNeighborChannels( ChannelMap & chm )
{
	DomainMap::iterator it = m_mapIdDomains.begin(), et = m_mapIdDomains.end();
	for( ; it != et; it ++ ) {
		for( int i=0; i<2; i++ ) {
			SIM_CHANNEL ch = it->second->servers[i].sockChannel;
			uint32 ip = ch.peer.ip;
			if( (ip != 0) && (chm.find(ip) == chm.end()) ) {
				chm[ ip ] = ch;
			}
		}
	}

	return true;
}

bool BBQServer::OnMsgSSDomainUpdate( const SIM_REQUEST * req )	// update domain info, every 5 minute?
{
	WriteLock safe( m_mutexRecord );

	const SIMD_SS_DOMAIN_UPDATE * pQ = (const SIMD_SS_DOMAIN_UPDATE *) & req->msg.simData;

	// if the domain update info is from myself, then skip
	if( pQ->domain.dwId == m_config.domainInfo.dwId ) return true;

	SIM_REQUEST msg;
	SIM_REQINITCHAN( msg, req->channel, SIM_SS_DOMAIN_UPDATE, sizeof(SIMD_SS_DOMAIN_UPDATE) );
	SIMD_SS_DOMAIN_UPDATE * pA = (SIMD_SS_DOMAIN_UPDATE *) & msg.msg.simData;
	memset( pA, 0, sizeof(*pA) );

	if( pQ->flags.uplink && (pQ->domain.dwId != m_config.domainInfo.dwId) ) { // my direct child domain, so send back downlink msg

		// TODO: check password here if required
		//VfonDomainInfo vdInfo;
		//memset( & vdInfo, 0, sizeof(vdInfo) );
		//vdInfo.dwId = pQ->domain.dwId;
		//if( m_pDB->ReadVfonDomainInfo( & vdInfo ) ) {
		//	bool bValid = ( 0 == memcmp( vdInfo.strPassword, pQ->strPassword, USERDATA_NAME_SIZE ) );

		//	// return here if password is invalid.
		//}

		pA->flags.downlink = 1;

		pA->domain = m_config.domainInfo;

		SIMD_SERVER_INFO * p = & pA->server;

		p->dwIp = 0;

		//p->nAdminPort = ;
		p->nTcpPort = m_nDefaultTcpPort;
		p->nUdpPortMin = m_nMinUdpPort;
		p->nUdpPortMax = m_nMaxUdpPort;

		//p->dwCookie = 0;

		p->timestamp = time(NULL);
		p->tUpTime = m_status.tUpTime;

		p->nServerType = m_config.nServerType;
		p->nServerStatus = m_status.nServerStatus;

		p->nMaxUsers = m_config.nUserMax;
		p->nOnlineUsers = m_status.nOnlineUsers;
		p->nMCUServers = m_status.nMCUCount;
		p->nProxyServers = m_status.nProxyCount;

		SendMessage( & msg );
	}

	if( pQ->flags.downlink ) {
		m_config.domainInfo.dwParentId = pQ->domain.dwId;
	}

	SIMD_DOMAIN_INFO di = pQ->domain;
	if( pQ->flags.uplink && (pQ->domain.dwId != m_config.domainInfo.dwId) ) {
		// my direct child domain, so this is the parent domain of him
		di.dwParentId = m_config.domainInfo.dwId;
	}

	BBQDomainRecord * pDomain = NULL;
	DomainMap::iterator it = m_mapIdDomains.find( di.dwId );
	if( it != m_mapIdDomains.end() ) { // already in our list
		pDomain = it->second;
	} else {
		pDomain = new BBQDomainRecord;
		memset( pDomain, 0, sizeof(*pDomain) );
		m_mapIdDomains[ di.dwId ] = pDomain;
	}

	SIMD_SERVER_INFO si = pQ->server;

	// if the server has no ip info, it is direct connected to me, so i set the ip for it
	if( si.dwIp == 0 ) si.dwIp = req->channel.peer.ip;

	// convert the time to local time
	time_t tNow = time(NULL);
	if( tNow > si.timestamp ) si.tUpTime += (tNow - si.timestamp);
	else si.tUpTime -= (si.timestamp - tNow);
	si.timestamp = tNow;

	if( pDomain ) {
		pDomain->domain = di;

		bool bMatch = false;
		for( int i=0; i<2; i++ ) {
			if( pDomain->servers[i].server.dwIp == si.dwIp ) {
				pDomain->servers[i].server = si;
				pDomain->servers[i].sockChannel = req->channel;
				bMatch = true;
				break;
			}
		}

		if( ! bMatch ) { // if not found, then replace the older one
			BBQServerRecord * pS = ( pDomain->servers[0].server.timestamp <= pDomain->servers[1].server.timestamp ) ? (& pDomain->servers[0]) : (& pDomain->servers[1]);
			pS->server = si;
			pS->sockChannel = req->channel;
		}
	}

	// forward this update info to all other neighbor server.
	* pA = * pQ;
	pA->domain = di;
	pA->server = si;
	pA->flags.value = 0;
	pA->flags.broadcast = 1;

	ChannelMap chMap;
	GetDomainNeighborChannels( chMap );
	for( ChannelMap::iterator iter = chMap.begin(), eiter = chMap.end(); iter != eiter; iter ++ ) {
		msg.channel = iter->second;
		if( msg.channel.peer.ip == req->channel.peer.ip ) continue; // if this msg is from this channel, then skip
		SendMessage( & msg );
	}

	return true;
}

bool BBQServer::OnMsgSSDomainSync( const SIM_REQUEST * req )
{
	return true;
}

bool BBQServer::OnMsgSSDomainSyncData( const SIM_REQUEST * req )
{
	return true;
}

/*
Input info:
  (1) sessionInfo, might not in use;
  (2) vfon id, if vfon.id == 0 then use following bind info;
  (3) string, bind type, such as "VFONID", "PCCW_CVG", "PCCW_IVC", "Mobilephone", "Email", etc.
  (4) string, bind id, such "139123456", "user@host.com", etc.
Output info:
  (1) queried VFON id;
  (2) location;
  (3) other info;
*/

bool BBQServer::OnMsgClientLocateEx( const SIM_REQUEST * req )
{
	const SIMD_CS_LOCATE_EX * pQ = (const SIMD_CS_LOCATE_EX *) & req->msg.simData;

	SIM_CHANNEL chForward;
	memset( & chForward, 0, sizeof(chForward) );
	bool bForward = false;

	if( pQ->nPathNodeCount < SIM_LOCATE_EX_MAX ) {

		uint32 dwDomainId = pQ->vfonid.serverid;

		// if use domain alias, we convert to domain id
		if( dwDomainId == 0 ) {
			if( strlen(pQ->lpszDomainAlias) != 0 ) {
				KeywordIndex::iterator it = m_dictDomainAlias.find( pQ->lpszDomainAlias );
				if( it != m_dictDomainAlias.end() ) {
					dwDomainId = it->second;
				}
			}
		}

		if( (dwDomainId == 0) || (dwDomainId == m_config.domainInfo.dwId) ) { // yeah, it is current domain !!!

			VFONID vfonid = pQ->vfonid;

			if( vfonid.userid == 0 ) { // if userid == 0, then use bind id to find out

				// TODO: search keyword index in cluster server memory instead of searching DB ???

				BBQUserFullRecord rec;
				memset( & rec, 0, sizeof(rec) );
				CATCH_DB_FAIL(
					bool bRead = false;
					//if( 0 == strcmp( pQ->lpszBindType, "BY_EMAIL" ) ) {
					//	bRead = m_pDB->ReadByKey( & rec, pQ->lpszBindId, BBQDatabase::BY_EMAIL );
					//} else if( 0 == strcmp( pQ->lpszBindType, "BY_HANDPHONE" ) ) {
					//	bRead = m_pDB->ReadByKey( & rec, pQ->lpszBindId, BBQDatabase::BY_HANDPHONE );
					//} else if( 0 == strcmp( pQ->lpszBindType, "BY_HARDWAREID" ) ) {
					//	bRead = m_pDB->ReadByKey( & rec, pQ->lpszBindId, BBQDatabase::BY_HARDWAREID );
					//} else {
					//	// the search will ignore type, this will allow PCCW_xxx call each other
					//	// but pls make sure the bindId is unique
					//	const char * bindTypeForSearch = ((strstr(pQ->lpszBindType, "PCCW_") != NULL) ? "" : pQ->lpszBindType);
					//	bRead = m_pDB->ReadByKey( & rec, bindTypeForSearch, pQ->lpszBindId ); // find it in database by bind key string
					//}
					bRead = m_pDB->ReadByKey( & rec, "", pQ->lpszBindId );
					if( bRead ) { 
						vfonid.userid = rec.uid;
						vfonid.serverid = m_config.domainInfo.dwId;
					}
				);
			}

			if( m_config.run_as_cluster_center ) { // this is a domain server, find the login server and forward the request

				SWITCHINFO si;
				if( m_bbqSwitch.Get( vfonid.userid, si ) ) {
					ServerMap::iterator it = m_mapLocalServers.find( si.addr.ip );
					if( it != m_mapLocalServers.end() && it->second ) {
						chForward = it->second->sockChannel;
						bForward = true;
					}
				} 

			} 
			
			if( ! bForward ) { // this is a login server, find the record of user and return exact info

				SFIDRecord * pRec = FindRecordByID( vfonid.userid );
				if( pRec ) {
					SIM_REQUEST reply;
					SIM_REPLY_REQUEST( & reply, SIM_SC_LOCATE_EX, sizeof(SIMD_SC_LOCATE_EX), req );
					SIMD_SC_LOCATE_EX * pA = (SIMD_SC_LOCATE_EX *) & reply.msg.simData;
					memset( pA, 0, sizeof(*pA) );

					pA->statusCode = SIMS_OK;

					pA->vfonid.serverid = m_config.domainInfo.dwId;
					pA->vfonid.userid = vfonid.userid;

					pA->dwServerIp = 0;
					pA->nServerTcpPort = m_nDefaultTcpPort;
					pA->nServerUdpPort = m_nDefaultUdpPort;

					pA->techInfo = pRec->techInfo;

					pA->onlineStatus = (uint8)(pRec->userStatus & 0xff);
					pA->setStatus = (uint8)(pRec->setStatus & 0xff);
					pA->isTCP = pRec->sockChannel.socket ? 1 : 0;
					pA->addrWAN = pRec->sockChannel.peer;

					pA->timestamp = pRec->time;						// heartbeat time
					pA->tNowServerTime = time(NULL);				// server time now

					if( pQ->nPathNodeCount > 0 ) {					// copy path and return
						pA->nPathNodeCount = pQ->nPathNodeCount;
						for( int i=0; i<pQ->nPathNodeCount; i++ ) {
							pA->PathChannels[i] = pQ->PathChannels[i];
						}
					}

					SendMessage( & reply );
					return true;

				} // else fall through
			}

		} else { // find the neighbor to the domain, forward the request

			DomainMap::iterator it = m_mapIdDomains.find( dwDomainId );
			if( it != m_mapIdDomains.end() ) {

				//DomainMap::iterator f = it;
				//for( int i=0; i<SIM_LOCATE_EX_MAX; i++ ) {
				//	BBQDomainRecord * p = f->second;
				//	if( p->domain.dwParentId == m_config.domainInfo.dwId ) { // the domain is my sub domain, forward to direct connected one
				//		chForward = p->servers[0].sockChannel;
				//		bForward = true;
				//	}
				//}
				//
				//if( ! bForward ) { // if not child domain, then forward to parent domain
				//	chForward = m_chanUplink[0];
				//	bForward = true;
				//}

				// since we already record who told me the domain info, we just ask him
				BBQDomainRecord * p = it->second;
				chForward = ( p->servers[0].server.timestamp > p->servers[1].server.timestamp ) ? p->servers[0].sockChannel : p->servers[1].sockChannel;
				//chForward = it->second->servers[0].sockChannel;
				bForward = true;

			} // else fall through

		}
	}

	SIM_REQUEST msg;
	if( bForward ) {
		SIM_REQINITCHAN( msg, chForward, SIM_CS_LOCATE_EX, sizeof(SIMD_CS_LOCATE_EX) );
		SIMD_CS_LOCATE_EX * pF = (SIMD_CS_LOCATE_EX *) & msg.msg.simData;
		* pF = * pQ;
		pF->PathChannels[ pF->nPathNodeCount ] = req->channel;
		pF->nPathNodeCount ++;
	} else {
		SIM_REPLY_REQUEST( & msg, SIM_SC_LOCATE_EX, sizeof(SIMD_SC_LOCATE_EX), req );
		SIMD_SC_LOCATE_EX * pA = (SIMD_SC_LOCATE_EX *) & msg.msg.simData;
		memset( pA, 0, sizeof(*pA) );
		pA->statusCode = SIMS_NOTFOUND;
		if( pQ->nPathNodeCount > 0 ) {					// copy path and return
			pA->nPathNodeCount = pQ->nPathNodeCount;
			for( int i=0; i<pQ->nPathNodeCount; i++ ) {
				pA->PathChannels[i] = pQ->PathChannels[i];
			}
		}
	}
	SendMessage( & msg );

	return true;
}

bool BBQServer::OnMsgServerLocateEx( const SIM_REQUEST * req )
{
	const SIMD_SC_LOCATE_EX * pQ = (const SIMD_SC_LOCATE_EX *) & req->msg.simData;

	if( pQ->nPathNodeCount > 0 ) {

		SIM_CHANNEL ch = pQ->PathChannels[ pQ->nPathNodeCount-1 ];

		SIM_REQUEST msg;
		SIM_REQINITCHAN( msg, ch, SIM_SC_LOCATE_EX, sizeof(SIMD_SC_LOCATE_EX) );
		SIMD_SC_LOCATE_EX * pF = (SIMD_SC_LOCATE_EX *) & msg.msg.simData;
		* pF = * pQ;
		if( (pF->dwServerIp == 0) || (pF->dwServerIp == LOCALHOST_IP) ) pF->dwServerIp = req->channel.peer.ip;
		pF->nPathNodeCount --;

		SendMessage( & msg );
	}

	return true;
}

bool BBQServer::OnMsgClientNotifyReady( const SIM_REQUEST * req )
{
	const SIMD_CS_NOTIFY_READY * pQ = (const SIMD_CS_NOTIFY_READY *) & req->msg.simData;

	ReadLock safe( m_mutexRecord );

	SFIDRecord * pRec = FindRecordByID( pQ->sessionInfo.id );
	if( ! pRec ) return true;
	if( pRec->sessionInfo.cookie != pQ->sessionInfo.cookie ) return true;

	// now send welcome message
	{
		const char * lpszWelcomeMsg = (const char *) m_strWelcomeMsg;
		int nLen = strlen( lpszWelcomeMsg );
		if( nLen > 0 ) {
			if( nLen >= TEXT_MESSAGE_MAX_LENGTH ) nLen = TEXT_MESSAGE_MAX_LENGTH -1;

			SIM_REQUEST notify;
			SIM_REQINITCHAN( notify, pRec->sockChannel, SIM_SC_BROADCASTTEXT, nLen+1 );

			char * pQ = (char *) & notify.msg.simData[0];
			memcpy( pQ, lpszWelcomeMsg, nLen );
			pQ[nLen] = '\0';

			SendMessage( & notify );
		}
	}

	// now send the meeting notification
	MeetingList * pList = pRec->pMeetingList;
	if( pList && (pList->size()>0) ) {
		SIM_REQUEST notify;
		SIM_REQINITCHAN( notify, pRec->sockChannel, SIM_SC_MCC_NOTIFY, sizeof(SIMD_SC_MCC_NOTIFY) );
		SIMD_SC_MCC_NOTIFY * pQ = (SIMD_SC_MCC_NOTIFY *) & notify.msg.simData;
		memset( pQ, 0, sizeof(*pQ) );
		pQ->sessionInfo = pRec->sessionInfo;
		pQ->tNowSenderTime = time(NULL);
		for( MeetingList::iterator it = pList->begin(), et = pList->end(); it != et; it ++ ) {

			BBQ_MeetingInfo mInfo = (* it)->info;

			//const BBQ_MeetingInfo_Ext * p = * it;
			//pQ->eventId = p->info.status;
			// TODO: encrypt msg if needed
			//pQ->meetingInfo = p->info;

			pQ->eventId = mInfo.status;

			// TODO: encrypt msg if needed
			if( pRec->pAESKeyBytes && (pRec->techInfo.version >= MCC_NOTIFY_AES_CLIENT_VERSION ) ) {
				bool bDone = AES_encrypt( (unsigned char *) & mInfo, sizeof(mInfo),
					(unsigned char *) pRec->pAESKeyBytes, (unsigned char *) PI_SQRT2_STRING, pRec->nAESKeyBytes * 8,
					(unsigned char *) & pQ->meetingInfo );
				if( bDone ) {
					pQ->flags.is_encrypted = 1;
				} else {
					pQ->meetingInfo = mInfo;
				}
			} else {
				pQ->meetingInfo = mInfo;
			}

			PostMessage( & notify, false, 2000, 10 );
		}
	}

	// NEW: now send the friend adding notification
	if( pRec->pWaitAuthList && pRec->pWaitAuthList->Count() > 0 ) {
		SIM_REQUEST notify;
		SIM_REQINITCHAN( notify, pRec->sockChannel, SIM_SC_NOTIFY_CONTACT, sizeof(SIMD_SC_NOTIFY_CONTACT) );
		SIMD_SC_NOTIFY_CONTACT * pN = (SIMD_SC_NOTIFY_CONTACT *) & notify.msg.simData;
		pN->sessionInfo = pRec->sessionInfo;
		pN->statusCode = SIMS_OK;
		pN->actionCode = SIM_CS_ADDCONTACT;

		int n = pRec->pWaitAuthList->Count();
		if( n > 8 ) n = 8;
		uint32 * pId = pRec->pWaitAuthList->Data();
		for( int i=0; i<n; i++ ) {
			pN->userId = * pId ++;
			PostMessage( & notify );
			sleep_ms(0);
		}
	}

	// now check offline msg and send the notification
	if( ! pRec->pImList ) pRec->pImList = new offlineim_list;
	if( pRec->pImList ) {
		VFONID my_id;
		my_id.serverid = m_config.domainInfo.dwId;
		my_id.userid = pRec->sessionInfo.id;
		bool bDB = false;
		CATCH_DB_FAIL(
			bDB = m_pDB->ReadOfflineIM( * (pRec->pImList), my_id.value );
		);
		int n = pRec->pImList->size();
		if( bDB && n > 0 ) { // now send offline msg notification to client
			BBQOfflineIMMessage * im = pRec->pImList->front();
			int nMsgLen = sizeof(*im) + (im->length-1);
			int nData = sizeof(SIMD_IM) + (im->length-1);
			int nReq = SIM_DATASIZE_TO_REQSIZE( nData );

			SIM_REQUEST * pReq = NULL;
			SIM_REQUEST_NEW( pReq, nReq );
			if( pReq ) {
				SIM_REQINITCHAN( * pReq, pRec->sockChannel, SIM_SC_IM, nData );
				SIMD_IM * pQ = (SIMD_IM *) & pReq->msg.simData[0];
				pQ->nTotal = pRec->pImList->size();
				pQ->nIndex = 0;
				pQ->sessionInfo = pRec->sessionInfo;
				pQ->statusCode = SIMS_OK;
				memcpy( & pQ->imMsg, im, nMsgLen );
				PostMessage( pReq, true );

				SIM_REQUEST_DELETE( pReq );
			}
		}
	}

	return true;
}

PString duration_to_string( time_t t )
{
	int s, m, h;

	s = t % 60; t /= 60;
	m = t % 60; t /= 60;
	h = t;

	return PString( PString::Printf, "%d:%02d:%02d", h, m, s );
}

bool BBQServer::OnMsgClientMeetingInvite( const SIM_REQUEST * req )
{
	time_t now = time(NULL);

	const SIMD_CS_MCC_INVITE * pQ = (const SIMD_CS_MCC_INVITE *) & req->msg.simData;

	SIM_REQUEST reply;
	SIM_REQINITCHAN( reply, req->channel, SIM_SC_MCC_INVITE, sizeof(SIMD_SC_MCC_INVITE) );
	SIMD_SC_MCC_INVITE * pA = (SIMD_SC_MCC_INVITE *) & reply.msg.simData;
	memset( pA, 0, sizeof(*pA) );

	PString strMsg( PString::Printf, "client %d sending meeting invitation '%s' to %d ... ",
		pQ->sessionInfo.id, pQ->meetingInfo.strMeetingId, pQ->toNewId );

	SFIDRecord * pChairman = FindRecordByID( pQ->sessionInfo.id );
	if( pChairman 
		&& ( pChairman->sessionInfo.cookie == pQ->sessionInfo.cookie )
		&& ( pChairman->time + m_config.heartbeat_timeout > now ) 
		// && ( pChairman->actStatus == CLIENT_MCU ) // only allow MCU send meeting invite ?
		) {

		const BBQ_MeetingInfo * pM = & pQ->meetingInfo;

		WriteLock lock( m_mutexRecord );

		// find the user
		SFIDRecord * pRec = FindRecordByID( pQ->toNewId );
		if( pRec ) {

			// now send notification
			if( (pRec->sessionInfo.cookie != 0) && (pRec->time + m_config.heartbeat_timeout > now) ) {
				BBQ_MeetingInfo mInfo = * pM;
				mInfo.timestamp = now;
				mInfo.status = pQ->eventId;

				SIM_REQUEST req;
				SIM_REQINITCHAN( req, pRec->sockChannel, SIM_SC_MCC_NOTIFY, sizeof(SIMD_SC_MCC_NOTIFY) );
				SIMD_SC_MCC_NOTIFY * pQ = (SIMD_SC_MCC_NOTIFY *) & req.msg.simData;
				memset( pQ, 0, sizeof(*pQ) );
				pQ->tNowSenderTime = now;
				pQ->eventId = MCC_BEGIN;
				pQ->sessionInfo = pRec->sessionInfo;

				// TODO: encrypt msg if needed
				if( pRec->pAESKeyBytes && (pRec->techInfo.version >= MCC_NOTIFY_AES_CLIENT_VERSION ) ) {
					bool bDone = AES_encrypt( (unsigned char *) & mInfo, sizeof(mInfo),
						(unsigned char *) pRec->pAESKeyBytes, (unsigned char *) PI_SQRT2_STRING, pRec->nAESKeyBytes * 8,
						(unsigned char *) & pQ->meetingInfo );
					if( bDone ) {
						pQ->flags.is_encrypted = 1;
					} else {
						pQ->meetingInfo = mInfo;
					}
				} else {
					pQ->meetingInfo = mInfo;
				}

				PostMessage( & req, false, 2000, 10 );
			}

			// find the meeting and update user reference
			BBQ_MeetingInfo_Ext * pExt = NULL;
			if( m_mlMeetingList.size() > 0 ) {
				for( meetingext_list::iterator it = m_mlMeetingList.begin(), et = m_mlMeetingList.end(); it != et; it ++ ) {
					BBQ_MeetingInfo_Ext * p = * it;
					if( 0 == strcmp( pM->strMeetingId, p->info.strMeetingId ) ) {
						UpdateUserMeetingRef( p );
						break;
					}
				}
			}

			pA->statusCode = SIMS_OK;
			strMsg += "done.";
		} else {
			pA->statusCode = SIMS_NOTFOUND;
			strMsg += "user not found.";
		}
		
	} else {
		pA->statusCode = SIMS_DENY;	
		strMsg += "denied.";
	}

	SendMessage( & reply );
	m_UserActionLog.Output( PLog::Info, strMsg );

	return true;
}

bool BBQServer::OnMsgClientNotifyMeetingEvent( const SIM_REQUEST * req )
{
	const SIMD_CS_MCC_NOTIFY * pQ = (const SIMD_CS_MCC_NOTIFY *) & req->msg.simData;

	// invalid source, ignore
	if( pQ->meetingInfo.flags.bMCU && (pQ->meetingInfo.nMcuId != pQ->sessionInfo.id) ) return true;

	WriteLock lock( m_mutexRecord );

	SFIDRecord * pRec = FindRecordByID( pQ->sessionInfo.id );
	if( ! pRec ) return true;

	// confirm data is from MCU
	if( pQ->meetingInfo.flags.bMCU && ( pRec->actStatus != CLIENT_MCU) ) return true;

	// if not found, ignore
	if( m_dictMeetingId.find( pQ->meetingInfo.strMeetingId ) == m_dictMeetingId.end() ) return true;

	BBQ_MeetingInfo_Ext * pExt = NULL;
	
	meetingext_list * pFromList = NULL, * pToList = NULL;
	if( MCC_BEGIN == pQ->eventId) {
		pFromList = & m_mlMeetingQueue;
		pToList = & m_mlMeetingList;
	} else if ( MCC_END == pQ->eventId ) {
		pFromList = & m_mlMeetingList;
		pToList = NULL;
	} else {
		return true;
	}

	if( ! pFromList ) return true;

	for( meetingext_list::iterator it = pFromList->begin(), et = pFromList->end(); it != et; it ++ ) {
		BBQ_MeetingInfo_Ext * p = * it;
		if( 0 == strcmp(p->info.strMeetingId, pQ->meetingInfo.strMeetingId) ) {
			pExt = p;
			STL_ERASE( * pFromList, meetingext_list::iterator, it );
			break;
		}
	}
	
	if( ! pExt ) return true;
	
	pExt->info.status = pQ->eventId;

	// clear forecast flag before moving to another queue
	pExt->info.flags.bForecastSent = 0;

	//switch( pQ->eventId ) {
	//case MCC_BEGIN:
	//	pInfo->tBeginTime = time(NULL);	// actual begin time
	//	break;
	//case MCC_END:
	//	break;
	//}

	UpdateUserMeetingRef( pExt );

	// update info in DB
	CATCH_DB_FAIL(
		m_pDB->UpdateConferenceBookingExt( pExt );
	);

	if( pToList ) {
		if( m_config.write_useraction_log ) {
			PString strMsg( PString::Printf, 
				"Meeting (%s) starting confirmed by MCU(%u).", 
				pExt->info.strMeetingId, 
				pExt->info.nMcuId );
			m_UserActionLog.Output( PLog::Info, strMsg );
		}

		pToList->push_back( pExt );
	} else {
		time_t tNow = time(NULL);
		time_t nDuration = tNow - pExt->info.tBeginTime;
		if( m_config.write_useraction_log ) {
			PString strMsg( PString::Printf, 
				"Meeting (%s) ended at %s of %s, by MCU(%u).", 
				pExt->info.strMeetingId, 
				(const char *) duration_to_string( nDuration ), 
				(const char *) duration_to_string( pExt->info.nDuration ),
				pExt->info.nMcuId );
			m_UserActionLog.Output( PLog::Info, strMsg );
		}

		DestroyMeetingInfo( pExt );
	}

	return true;
}

bool BBQServer::OnMsgClientAuthContact( const SIM_REQUEST * req )
{
	const SIMD_CS_AUTH_CONTACT * pQ = (const SIMD_CS_AUTH_CONTACT *) & req->msg.simData;

	WriteLock lock( m_mutexRecord );

	SFIDRecord * pRec = FindRecordByID( pQ->sessionInfo.id );
	if( pRec ) {
		switch( pQ->actionCode ) {
		case SIM_CS_ADDCONTACT:
			if( pRec->pWaitAuthList ) {
				pRec->pWaitAuthList->Remove( pQ->userId );
				//pRec->pFriendList->Add( pQ->userId );
				pRec->is_dirty = 1;

				BBQUserValidFlags flags;
				memset( & flags, 0, sizeof(flags) );
				flags.partial_valid = 1;
				flags.friends = 1;
				SaveData( pRec, SaveOnChange, & flags );
			}
			break;
		}
	}

	return true;
}

bool BBQServer::OnMsgClientLogUserAction( const SIM_REQUEST * req )
{
	const SIMD_CS_LOGUSERACTION * pQ = (const SIMD_CS_LOGUSERACTION *) & req->msg.simData;

	WriteLock lock( m_mutexRecord );

	SFIDRecord * pRec = FindRecordByID( pQ->sessionInfo.id );
	if( pRec && ( pRec->sessionInfo.cookie == pQ->sessionInfo.cookie ) ) {
		
		// skip duplicated msg
		if( pQ->logInfo.msg_rand_id ) {
			if( pRec->last_msg_rand_id == pQ->logInfo.msg_rand_id ) return true;
			pRec->last_msg_rand_id = pQ->logInfo.msg_rand_id;
		}

		// write pQ->logInfo to DB
		CATCH_DB_FAIL( 
			m_pDB->WriteUserActionLog( & pQ->logInfo ); 
		);

	}

	return true;
}

bool BBQServer::OnMsgClientDownloadServerPKICert( const SIM_REQUEST * req )
{
	SIM_REQUEST * pReq = NULL, * pTemp = NULL;

	SIM_REQUEST reply;
	SIM_REQINITCHAN( reply, req->channel, SIM_SC_GET_PKI_CERT, sizeof(SIMD_SC_GET_PKI_CERT) );
	SIMD_SC_GET_PKI_CERT * pA = (SIMD_SC_GET_PKI_CERT *) & reply.msg.simData;
	memset( pA, 0, sizeof(*pA) );

	if( ! m_pzPKICertPEM ) {
		pA->statusCode = SIMS_NOTAVAILABLE;
		SendMessage( & reply );
		return true;
	}

	int n = strlen( m_pzPKICertPEM );
	int nMsgDataSize = sizeof(SIMD_SC_GET_PKI_CERT) + (n+1);
	if( nMsgDataSize >= SIM_DATABLOCKSIZEMAX ) {
		int nReqSize = SIM_DATASIZE_TO_REQSIZE( nMsgDataSize );
		SIM_REQUEST * pTemp = NULL;
		SIM_REQUEST_NEW( pTemp, nReqSize );
		if( ! pTemp ) return true;

		pReq = pTemp;
	} else {
		pReq = & reply;
	}

	SIM_REQINITCHAN( * pReq, req->channel, SIM_SC_GET_PKI_CERT, nMsgDataSize );
	pA = (SIMD_SC_GET_PKI_CERT *) & pReq->msg.simData;
	memset( pA, 0, nMsgDataSize );
	pA->statusCode = SIMS_OK;
	strcpy( pA->strFormat, "PEM" );
	strcpy( pA->strPEM, m_pzPKICertPEM );

	SendMessage( pReq );

	if( pTemp ) SIM_REQUEST_DELETE( pTemp );
	return true;
}

bool BBQServer::OnMsgClientSendOfflineIM( const SIM_REQUEST * req )
{
	SIMD_IM * pQ = (SIMD_IM *) & req->msg.simData[0];

	SIM_REQUEST reply;
	SIM_REQINITCHAN( reply, req->channel, SIM_SC_IM, sizeof(SIMD_IM) );
	SIMD_IM * pA = (SIMD_IM *) & reply.msg.simData;
	memset( pA, 0, sizeof(*pA) );
	pA->sessionInfo = pQ->sessionInfo;

	SFIDRecord * pFrom = FindRecordByID( pQ->sessionInfo.id );
	if( pFrom && ( pFrom->sessionInfo.cookie == pQ->sessionInfo.cookie ) ) {

		VFONID vfonid;
		vfonid.value = pQ->imMsg.to_vfonid;
		SFIDRecord * pTo = FindRecordByID( vfonid.userid );
		if( pTo ) {
			if( ! pTo->pImList ) pTo->pImList = new offlineim_list;
			if( pTo->pImList ) {
				// duplicate msg, ignore
				for( offlineim_list::iterator it = pTo->pImList->begin(), et = pTo->pImList->end(); it != et; it ++ ) {
					BBQOfflineIMMessage * im = * it;
					if( 0 == memcmp(im->sentence_id, pQ->imMsg.sentence_id, 36) ) return true; 
				}

				// deliver the offline msgs to target user and save to DB
				CATCH_DB_FAIL( 
					m_pDB->WriteOfflineIM( & pQ->imMsg ); 
				);

				// if the user is online, then send a notification
				time_t offline_time = time(NULL) - m_config.heartbeat_timeout;
				if( pTo->sessionInfo.cookie && (pTo->time > offline_time) ) {
					int nTotal = 1;

					// put to im inbox
					if( ! pTo->pImList ) pTo->pImList = new offlineim_list;
					if( pTo->pImList ) {
						BBQOfflineIMMessage * im = BBQOfflineIMMessageNEW( pQ->imMsg.length );
						if( im ) {
							memcpy( im, & pQ->imMsg, sizeof(*im) + (pQ->imMsg.length -1) );
							pTo->pImList->push_back( im );

							nTotal = pTo->pImList->size();
						}
					}

					// forward to target
					int nMsgLen = sizeof(pQ->imMsg) + (pQ->imMsg.length -1);
					int nData = sizeof(SIMD_IM) + (pQ->imMsg.length -1);
					int nReq = SIM_DATASIZE_TO_REQSIZE( nData );
					SIM_REQUEST * pReq = NULL;
					SIM_REQUEST_NEW( pReq, nReq );
					if( pReq ) {
						SIM_REQINITCHAN( * pReq, pTo->sockChannel, SIM_SC_IM, nData );
						SIMD_IM * pF = (SIMD_IM *) & pReq->msg.simData[0];
						pF->nTotal = nTotal;
						pF->nIndex = nTotal -1;
						pF->sessionInfo = pTo->sessionInfo;
						pF->statusCode = SIMS_OK;
						memcpy( & pF->imMsg, & pQ->imMsg, nMsgLen );
						PostMessage( pReq, true );

						SIM_REQUEST_DELETE( pReq );
					}
				}

				pA->statusCode = SIMS_OK;
			} else {
				pA->statusCode = SIMS_NOTFOUND;
			}
		} else {
			pA->statusCode = SIMS_NOTFOUND;
		}
	} else {
		pA->statusCode = SIMS_DENY;
	}

	SendMessage( & reply );
	return true;
}

bool BBQServer::OnMsgClientGetOfflineIM( const SIM_REQUEST * req )
{
	SIMD_IM * pQ = (SIMD_IM *) & req->msg.simData[0];

	SFIDRecord * pRec = FindRecordByID( pQ->sessionInfo.id );
	if( pRec && ( pRec->sessionInfo.cookie == pQ->sessionInfo.cookie ) ) {
		int n = pRec->pImList->size();

		// get offline msg from inbox
		if( pRec->pImList && (pQ->nIndex < n) ) {
			int i = 0;
			for( offlineim_list::iterator it = pRec->pImList->begin(), et = pRec->pImList->end(); it != et; it ++, i ++ ) {
				if( i < pQ->nIndex ) continue;
				if( i > pQ->nIndex ) break;

				BBQOfflineIMMessage * im = * it;

				int nMsgLen = sizeof(pQ->imMsg) + (im->length -1);
				int nData = sizeof(SIMD_IM) + (im->length -1);
				int nReq = SIM_DATASIZE_TO_REQSIZE( nData );

				SIM_REQUEST * pReq = NULL;
				SIM_REQUEST_NEW( pReq, nReq );
				if( pReq ) {
					SIM_REQINITCHAN( * pReq, req->channel, SIM_SC_GET_IM, nData );
					SIMD_IM * pQ = (SIMD_IM *) & pReq->msg.simData[0];
					pQ->nTotal = pRec->pImList->size();
					pQ->nIndex = i;
					pQ->sessionInfo = pRec->sessionInfo;
					pQ->statusCode = SIMS_OK;
					memcpy( & pQ->imMsg, im, nMsgLen );
					PostMessage( pReq, true );

					SIM_REQUEST_DELETE( pReq );
					return true;
				}
			}
		}
	}

	SIM_REQUEST reply;
	SIM_REQINITCHAN( reply, req->channel, SIM_SC_GET_IM, sizeof(SIMD_IM) );
	SIMD_IM * pA = (SIMD_IM *) & reply.msg.simData;
	memset( pA, 0, sizeof(*pA) );
	pA->sessionInfo = pQ->sessionInfo;
	pA->statusCode = SIMS_NOTFOUND;
	SendMessage( & reply );

	return true;
}

bool BBQServer::OnMsgClientDeleteOfflineIM( const SIM_REQUEST * req )
{
	SIMD_IM * pQ = (SIMD_IM *) & req->msg.simData[0];

	SIM_REQUEST reply;
	SIM_REQINITCHAN( reply, req->channel, SIM_SC_DEL_IM, sizeof(SIMD_IM) );
	SIMD_IM * pA = (SIMD_IM *) & reply.msg.simData;
	memset( pA, 0, sizeof(*pA) );
	pA->sessionInfo = pQ->sessionInfo;

	SFIDRecord * pRec = FindRecordByID( pQ->sessionInfo.id );
	if( pRec && ( pRec->sessionInfo.cookie == pQ->sessionInfo.cookie ) ) {
		// remove the offline msgs from DB and memory

		// delete the im in DB
		VFONID my_id;
		my_id.serverid = m_config.domainInfo.dwId;
		my_id.userid = pRec->sessionInfo.id;
		CATCH_DB_FAIL( 
			m_pDB->DeleteOfflineIM( my_id.value ); 
		);

		// delete the im in memory
		int n = 0;
		BBQOfflineIMMessage * im = NULL;
		while( pRec->pImList && pRec->pImList->size() > 0 ) {
			im = pRec->pImList->front();
			pRec->pImList->pop_front();

			BBQOfflineIMMessageDELETE( im );
			n ++;
		}

		pA->statusCode = SIMS_OK;
		pA->nTotal = n;
		pA->nIndex = 0;
	} else {
		pA->statusCode = SIMS_NOTFOUND;
	}

	SendMessage( & reply );
	return true;
}

bool BBQServer::YTUnRegisterSIPUser(const uint32 id)
{
  TelEngine::Message msg("user.unregister");
  //msg.setParam("module","regfile");
  char szID[32]={0};
  msg.setParam("number", ultoa(id, szID, 10));
  if (TelEngine::Engine::dispatch(&msg))
  {
    return true;
  }
  return false;
}
bool BBQServer::OnMsgClientEnterRoom( const SIM_REQUEST * req )
{
 
	const SIMD_CS_ENTER_ROOM * pQ = (const SIMD_CS_ENTER_ROOM *) & req->msg.simData;

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_ENTER_ROOM, sizeof(SIMD_SC_ENTER_ROOM), req );
	SIMD_SC_ENTER_ROOM * pA = (SIMD_SC_ENTER_ROOM *) & reply.msg.simData;

  std::vector<uint32> vecMembers;

  {
    LockIt lock(m_mutxRooms);
    RoomMap::iterator itor= m_mapRooms.find(pQ->nRoomID);
    if (itor != m_mapRooms.end())
    {
      vecMembers = itor->second.m_vecMembers ;
      itor->second.insertMember(pQ->sessionInfo.id);
      if (vecMembers.size() <=0 )
        return true;
    }else
      return true;
  }
	ReadLock lock( m_mutexRecord );

  SFIDRecord * pTo = FindRecordByID( pQ->sessionInfo.id  );
  PString strMembers="";
  for(std::vector<uint32>::iterator iter = vecMembers.begin();iter!= vecMembers.end();iter++)
  {
     
    {
      char a[32]={0};
      strMembers+= (ultoa( *iter, a, 10) ) + PString(";");
    }
  }

	if( pTo ) {
    //sender
    strncpy(pA->calllist, (const char*)strMembers, sizeof(pA->calllist) -1);
    //pA->caller =  pQ->sessionInfo.id;
    SendMessage( & reply );
  }


	return true;
}
bool BBQServer::OnMsgClientLeftRoom( const SIM_REQUEST * req )
{
 
	const SIMD_CS_LEFT_ROOM * pQ = (const SIMD_CS_LEFT_ROOM *) & req->msg.simData;

	SIM_REQUEST reply;
	SIM_REPLY_REQUEST( & reply, SIM_SC_LEFT_ROOM, sizeof(SIMD_SC_LEFT_ROOM), req );
	SIMD_SC_LEFT_ROOM * pA = (SIMD_SC_LEFT_ROOM *) & reply.msg.simData;


  {
    LockIt lock(m_mutxRooms);
    RoomMap::iterator itor= m_mapRooms.find(pQ->nRoomID);
    if (itor != m_mapRooms.end())
    {
      itor->second.removeMember(pQ->sessionInfo.id );
    }else
      return true;
  }
	ReadLock lock( m_mutexRecord );

  SFIDRecord * pTo = FindRecordByID( pQ->sessionInfo.id  );

	if( pTo ) {
    SendMessage( & reply );
  }

	return true;
}




