/*
 *
 *	bbqmanager.h
 *
 */

#ifndef _BBQ_MANAGER_H
#define _BBQ_MANAGER_H

#include <ptlib.h>

#include "bbqidmsg.h"
#include "msgterminal.h"
#include "ip2country.h"
#include "bbqidrecord.h"
#include "bbqtree.h"
#include "bbqdatabase.h"
#include "md5.h"
#include "bbqswitch.h"
#include "bbqmcc.h"

//#define  D_OLD_SWITCH

#ifdef ANTI_PACKET_LOSS
// map channel id to time stamp, clear older than 30s evey minute
typedef std::map<UNIQUE_CID, MS_TIME, less_than_unique_cid> UNIQUE_CID_TIME_MAP;
#endif

#define		UPLINK_TICK_TIME			30000			// 30s
#define		INTERLINK_TICK_TIME			10000			// 10s
typedef std::map<uint32, BBQProxyRecord *>	ProxyMap;
typedef std::map<uint32, BBQProxyRecord >	ProxyMapEx;

class BBQServer : public BBQMsgTerminal
{
	PCLASSINFO(BBQServer, BBQMsgTerminal)

	class UplinkThread : public PThread
	{
		PCLASSINFO( UplinkThread, PThread )
	private:
		BBQServer &	m_owner;
	public:
		UplinkThread( BBQServer & owner );
		~UplinkThread();

		virtual void Main( void );
	};

	class InterlinkThread : public PThread
	{
		PCLASSINFO( InterlinkThread, PThread )
	private:
		BBQServer &	m_owner;
	public:
		InterlinkThread( BBQServer & owner );
		~InterlinkThread();

		virtual void Main( void );
	};

	mutable PMutex									m_mutexLink;

	std::list<UplinkThread *>						m_listUplinkThreads;
	typedef std::list<UplinkThread *>::iterator		uplink_iterator;

	std::list<InterlinkThread *>					m_listInterlinkThreads;
	typedef std::list<InterlinkThread *>::iterator	interlink_iterator;

	MS_TIME											m_dwUplinkTickCalled;
	void UplinkTick( void );

	MS_TIME											m_dwInterlinkTickCalled;
	void InterlinkTick( void );

	bool SwitchType( uint32 nType );

	bool SwitchStatus( uint32 nStatus );

	friend class UplinkThread;
	friend class InterlinkThread;

public:
	BBQServer();
	~BBQServer();
  bool YTUnRegisterSIPUser(const uint32 id);
	static BBQServer* GetCurrentServer( void );

	// should connect to database before service startup, 
	// and disconnect from database after service shutdown.
	bool ConnectDatabase( BBQDatabase * pDB );
	void DisconnectDatabase( void );

	void CleanPKICertKey( void );
	bool LoadPKICertKey( const char * certfile, const char * privkeyfile, const char * pass = "" );
	virtual bool Startup( PConfig & cfg );
	void Shutdown( void );

	virtual		bool	IsValidCopy( void ) { return true; };
	virtual		int		GetUserLimit( void ) { return 0x7fffffff; };
	virtual		int		GetUserLimit720P( void ) { return 0x7fffffff; };
	virtual		int		GetUserLimit1080P( void ) { return 0x7fffffff; };
	virtual		int		GetBasicUserLimit( void ) { return 0x7fffffff; };
	virtual		int		GetAdvancedUserLimit( void ) { return 0x7fffffff; };
	virtual		int		GetBasicUserLimit2( void ) { return 0x7fffffff; };
	virtual		int		GetAdvancedUserLimit2( void ) { return 0x7fffffff; };
	virtual		bool	UserLimitForOnline( void ) { return true; };
	virtual		time_t	GetExpireTime( void ) { return time(NULL) + 3600 * 24 * 365; };

	enum TYPE {
		INVALID_TYPE	= 0,
		STANDALONE		= 1,
		CLUSTER			= 2,
		PRIMARY			= 3,
		SECONDARY		= 4,
	};

#define	STR_STANDALONE		"STANDALONE"
#define STR_CLUSTER			"CLUSTER"
#define STR_PRIMARY			"PRIMARY"
#define	STR_SECONDARY		"SECONDARY"

	enum STATUS {
		INVALID_STATUS	= 0,
		ACTIVE			= 1,
		STANDBY			= 2,
		DOWN			= 3,
	};

#define	STR_ACTIVE		"ACTIVE"
#define STR_STANDBY		"STANDBY"
#define STR_DOWN		"DownForDB"

	enum LINK {
		NO_LINK			= 0,
		INTERLINK		= 1,
		UPLINK			= 2,
	};

	static PString TypeToString( uint32 nType );
	static PString StatusToString( uint32 nStatus );

	static uint32 TypeToInt( const char * strType );
	static uint32 StatusToInt( const char * strStatus );

	enum PROXY_PICK_POLICY {
		BY_BANDWIDTH	= 0,
		BY_COUNTRY		= 1,
		BY_IPVALUE		= 2,
		BY_GROUP		= 3,
	};

protected:
	// some log related
	PFileLog		m_UserActionLog;

protected:
  //chen yuan add 720PUser list begin
  std::list<unsigned int>  m_list720POnlineUsers; 
  std::list<unsigned int>  m_list1080POnlineUsers; 
	std::list<std::string> m_lCurrentParticipantsInAllConfs;//All participants in a conference will consume one license each. it is invoke in m_recordmutex

  mutable   PMutex m_mtlistSpecial;
  //end

	static BBQServer*	m_pCurrentServer;

	struct Config {

		char	*	db_type;
		char	*	db_url;
		char	*	bind_type_filter_string;

		uint32		nUserMax;
		time_t		tExpireTime;

		int			heartbeat_timeout;
		uint32		id_min;
		uint32		id_max;

		int			subscribe_trial_time;	// trial time for subscribed user

		uint32		nServerType;			// server type: STANDALONE, CLUSTER, PRIMARY, SECONDARY

		DWORD		dwListenIp;

		IPSOCKADDR	thisServerAddress;		// public ip of this server, must be set if server behind NAT

		IPSOCKADDR	anotherServerAddress;	// interlink server address
		IPSOCKADDR	anotherServerAddress2;	// second interlink server, used as central server

		IPSOCKADDR	uplinkServerAddress;	// uplink server address

		int			nMaxPersonPerConference; // parameter send to client, participant limit per conference

		uint32		requiredVersion;		 // required version checking

		uint32		QoS_warning_level;		// percent of good calls
		uint32		QoS_stat_reset_time;	// QoS stat reset time

		uint32		nDbCacheTime;			// to boost server performance, there is a cache time for DB reading
		uint32		nMeetingForecastTime;		// meeting forecast time 
		uint32		nMeetingForecastTimeExt;		// meeting forecast time ext
		uint32		nMeetingEndForecastTime;	// meeting end forecast time

		uint32		nProxyPickPolicy;		// policy of proxy picking

		uint32		nUserParamA, nUserParamB;	// cheat display = users * A + B, for TW asia pacific only

		BBQACLFlag	flagsDefault;
		BBQACLFlag	flagsTrial;
		BBQACLFlag	flagsFree;
		BBQACLFlag	flagsPrepaid;

		BBQACLFlagExt	extFlagsDefault;
		BBQACLFlagExt	extFlagsTrial;
		BBQACLFlagExt	extFlagsFree;
		BBQACLFlagExt	extFlagsPrepaid;

		SIMD_DOMAIN_INFO	domainInfo;

		struct {
			unsigned int	run_as_cluster_center	: 1;

			unsigned int	use_tcp_for_server_link	: 1;

			unsigned int	index_serial			: 1;
			unsigned int	disable_serial_key_check : 1;

			unsigned int	disable_subscribe		: 1;
			unsigned int	disable_bind_user		: 1;

			unsigned int	use_customized_flag_for_normal : 1;
			unsigned int	use_customized_flag_for_trial : 1;
			unsigned int	use_customized_flag_for_free : 1;//POLICY_SERVICE_EXPIRED

			unsigned int	dbwrite_loginout_log : 1;
			unsigned int	dbwrite_conference_log : 1;
			unsigned int	write_useraction_log : 1;
			unsigned int	write_hearbeat_file : 1;

			unsigned int	check_dns_in_heartbeat : 1;

			unsigned int	ignore_db_err : 1;

			unsigned int	prepaid_policy : 1;

			unsigned int	enable_cug : 1;					// enable cug, all group access will be changed to called by buddy only.
			unsigned int	run_in_asp_mode : 1;			// run in ASP mode, the group root node is company, the second layer node is isolated group
			unsigned int	allow_cug_call_public : 1;		// allow CUG users call public users
			unsigned int	cug_have_private_buddy : 1;		// allow CUG users have private buddy list
			unsigned int	hide_id_tree_to_client : 1;		// hide id tree to client

			unsigned int	allow_subscribe_more_id_per_pc : 1;	// allow subscribe more id per PC

			unsigned int	meeting_forecast : 1;
			unsigned int	meeting_begin_notify : 1;

			unsigned int	allow_group_id_login_as_user : 1;		// allow group id login as user, else deny

			unsigned int	sync_buddy_with_authgateway : 1;

			unsigned int	meeting_end_forecast : 1;

			unsigned int	meeting_begin_notify_ext : 1; //允许提前进入会议
			unsigned int	allow_use_privateip_relpace_serviceip_forproxy : 1;//chenyuan
			unsigned int	enable_support_group : 1;//chenyuan

			unsigned int	flag_padding : 3;
		};

    uint32		nHeatbeatSpecialUserID ;	//  Special user test 
		//chen yuan
		uint32 KEY_IDC_WARNING_CPU_B:1;//enable the cpu opt in warning settings
		uint32 KEY_IDC_WARNING_CPU_V:7;//set the percent of the cpu opt in warning settings
		uint32 KEY_IDC_WARNING_MEM_B:1;
		uint32 KEY_IDC_WARNING_MEM_V:7;
		uint32 KEY_IDC_WARNING_NET_B:1;
		uint32 KEY_IDC_WARNING_NET_V:7;
		uint32 KEY_IDC_WARNING_VAILD_MONTHS:8; 
		//struct {
		//	uint32 m_nConcurrectUsersInAllConference;
		//} license;
	} m_config;

	struct Status {
		time_t		tUpTime;

		uint32		nServerStatus;				// server status flag
		uint32		nDBStatus;					// db status

		uint16		nInterlinkConnected;

		uint32		nAnotherServerType;
		uint32		nAnotherServerStatus;
		time_t		tAnotherServerUpTime;

		uint16		nUplinkConnected;

		uint32		nUplinkServerType;
		uint32		nUplinkServerStatus;
		time_t		tUplinkServerUpTime;

		uint32		nOnlineUsers, nMaxUserCount, nOnlineVFONUsers, nOnlineVMeetUsers;
    //chen yuan add 720PUser list begin
		int		nDongle720POnlineUsers, nDongle1080POnlineUsers;
    bool bIsValidCopy;                
    unsigned int nGetUserLimit;               
    unsigned int nGetExpireTime;              
    unsigned int nGetBasicUserLimit;          
    unsigned int nGetAdvancedUserLimit;       
    unsigned int nGetBasicUserLimit2;          
    unsigned int nGetAdvancedUserLimit2;       
    unsigned int nGetReadMeetingLimit;    ///* 0 for not limited*/   
    bool bUserLimitForOnline;                      
    //end
		uint32		nCurrentConfs, nMaxConfs;


		uint32		nProxyCount, nMaxProxyCount;
		uint32		nPureProxyCount, nMaxPureProxyCount;
		uint32		nMCUCount, nMaxMCUCount;

		uint32		nOkayCalls, nErrorCalls, nAbortedCalls, nFailedCalls;
		MS_TIME		msConfTime;
		char szUdpports[64];
		char szTcpports[64];
    Status()
    {
			memset(szUdpports, 0, sizeof(szUdpports));
			memset(szTcpports, 0, sizeof(szTcpports));
      bIsValidCopy = false;bUserLimitForOnline = true;
      nGetReadMeetingLimit=nGetBasicUserLimit2=nGetAdvancedUserLimit2= nGetAdvancedUserLimit = nGetBasicUserLimit = nGetExpireTime = nGetUserLimit = nDongle720POnlineUsers= nDongle1080POnlineUsers= 0;
    };
	} m_status;

	SIM_CHANNEL		m_chanInterlink[2];
	SIM_CHANNEL		m_chanUplink[2];

	// version info for client
	SIMD_VERSIONINFO	m_VersionInfo;
	PString				m_strNewClientDownloadURL;
	PStringToString		m_strStringToString;// only safe read for mult thread  
  
	PString			m_strWelcomeMsg;

	PString			m_strAcceptClientAppList;	// [MCU][VMEET][MHAVC]...

	char *			m_pzPKICertPEM;				// pki cert pem string
	PBYTEArray  m_pzPKICertPEMCompr;				// pki cert pem string
	char *			m_pzPKIPrivKeyPEM;			// pki priv key pem string

	void *			m_pPKICertX509;				// X509 object
	void *			m_pPKIPrivKey;				// EVP_PKEY object

	PStringArray	m_strsBindTypes;

	bool		m_bDataChanged;

	uint32		m_nCountCounter;				// counter for check online users, every 5 minutes
	uint32		m_nAutoSaveCounter;				// counter for saving changed records
	uint32		m_nCheckDongleCounter;			// counter for checking dongle, every hour, normally 0 ~ 3599
	uint32		m_nQoSCounter;					// counter for QoS checking

	bool		m_bDisableFeature;				// disable main features, including login, request channel

	WORD		m_nMinUdpPort;
	WORD		m_nMaxUdpPort;

	WORD		m_nDefaultUdpPort;
	WORD		m_nDefaultTcpPort;

#ifdef _DEBUG
	// a flag for test only, evoke proxy when each channel is requested, even it is not behind strict firewall
	bool		m_bProxyAllRequestChannel;
#endif

	bool		m_bInited;
	bool		m_bSystemConfigLoadedFromDB;

	mutable PReadWriteMutex m_mutexDB;

	BBQDatabase			* m_pDB;

	mutable PReadWriteMutex m_mutexRecord;

	struct less_than_string {
		bool operator()(PString s1, PString s2) const
		{
			return strcmp(s1, s2) < 0;
		}
	};

	// ---------------------------------------------
	// vfon client record in memory cache
	//
  struct LPNRRoomRecord
  {
    PString m_strName;
    PString m_nID;
    std::vector<uint32> m_vecMembers;
    bool insertMember(uint32 member)
    {
      for(int i=0;i< m_vecMembers.size();i++)
      {
        if (m_vecMembers[i] == member)
          return true;
      }
      m_vecMembers.push_back(member);
      return true;
    }
    bool removeMember(uint32 member)
    {
      for(std::vector<uint32>::iterator iter = m_vecMembers.begin();iter!= m_vecMembers.end();iter++)
      {
        if (*iter == member) 
        {
          m_vecMembers.erase(iter);
          return true;
        }
      }

      return false;
    }
  };
  PMutex m_mutxRooms;
	typedef std::map<uint32, LPNRRoomRecord >		RoomMap;
	RoomMap					m_mapRooms;		// uid -> LPNRRoomRecord *
	typedef std::map<uint32, SFIDRecord *>		RecordMap;
	RecordMap					m_mapRecord;		// uid -> SFIDRecord *

	typedef std::map<PString, uint32, less_than_string>		KeywordIndex;
	KeywordIndex				m_dictHardwareId;	// keyword -> uid
	KeywordIndex				m_dictHandphone;
	KeywordIndex				m_dictEmail;

	typedef std::map<PString, PString, less_than_string>	KeywordString;
	KeywordString				m_dictSystemConfig;			// keyword -> string
	PString						m_strSystemExt;

	PString						m_strCallbackTrustIp;		// "ip/mask;ip/mask;..."

	mutable PMutex				m_mutexClosedTcp;
	std::list<int>				m_listClosedTcp;

	// ---------------------------------------------- 
	// bbqproxy record in memory cache
	//
	ProxyMap					m_mapProxy;		// vfon id --> proxy record mapping
	ProxyMap					m_mapPureProxy;	// ip address --> proxy record mapping

	// ----------------------------------------------
	// vfon MCU record
	typedef std::map<uint32, MCURecord *>	MCUMap;
	MCUMap							m_mapMCU;

	// ----------------------------------------------
	// local servers
	typedef std::map<uint32, BBQServerRecord *> ServerMap;		
	ServerMap					m_mapLocalServers;				// map ip to server record

	// ----------------------------------------------
	// global domain records
	typedef std::map<uint32, BBQDomainRecord *> DomainMap;	
	DomainMap					m_mapIdDomains;					// map domain id to domain record  , use m_mutexRecord to lock it 
	//PMutex						m_mapIdDomainsMutex;			// mutex for map domain id to domain record 
	KeywordIndex				m_dictDomainAlias;				// alias -> id mapping for fast search

	typedef std::map<uint32, SIM_CHANNEL> ChannelMap;			// map ip -> channel

	// -----------------------------------------------
	// MCC related
	KeywordIndex				m_dictMeetingId;				// map meeting id to 1/0, true/false

	meetingext_list				m_mlMeetingQueue;				// in order of start time
	//meetingext_list				m_mlMeetingNotifying;			// in order of start time
	meetingext_list				m_mlMeetingList;				// in order of end time
	// vfon client login to which server? all in this memory directory base
#ifdef D_OLD_SWITCH
	BBQSwitch					m_bbqSwitch;
#endif
#ifdef ANTI_PACKET_LOSS
	// this is used for skip duplicate request
	mutable PMutex					m_mutexCidTime;
	UNIQUE_CID_TIME_MAP				m_mapRequestCidTime;
	UNIQUE_CID_TIME_MAP				m_mapReplyCidTime;
	uint32							m_nCidTimeCounter;
#endif

protected:

	virtual void OnMsgConnectionClose( BBQMsgConnection * pConn );

	virtual void OnTick( void );

	DECLARE_BBQ_MESSAGE_MAP()

protected:

	bool LoadSystemConfigFromDB( void );

	virtual void OnEventDBFail( void );

	// proxy record management
	Ip2CountryUtility	m_Ip2CountryUtility;

	BBQProxyRecord * PickProxyByTag( const char * pFromTag, const char * pToTag );

	BBQProxyRecord * PickProxyByIp2Country( uint32 ipFrom, uint32 ipTo );

	BBQProxyRecord * PickProxy( uint32 nFromId, uint32 nToId );
	BBQProxyRecord * PickProxy(   DWORD dwIp);

	BBQProxyRecord * FindProxyById( uint32 nId );
	BBQProxyRecord * FindPureProxyByIp( uint32 ip );
	bool RemoveProxyRecord( uint32 nId );

	bool UpdateRecordCache( SFIDRecord * pRec, BBQUserFullRecord * full );
	bool UpdateRecordCacheFromDB( SFIDRecord * pRec );

	// vfon client reocrd management
	SFIDRecord * InsertRecordIntoMemoryCache( BBQUserFullRecord * full );

	SFIDRecord * FindRecordByID( uint32 nID, bool bForceReload = false );
	SFIDRecord * FindRecordByHWID( const char * pHWID );
	SFIDRecord * CreateRecordByHWID( const char * pHWID );

	SFIDRecord * FindRecordBySerial( uint32 nSerial );
	SFIDRecord * FindOrCreateRecordByHWID( const char * pHWID, uint32 nID = 0 );

	void AskForBuddyStatus( SFIDRecord * pRec, uint32 uidTarget = 0 );

	void NotifyBuddyStatus( SFIDRecord * pRec, uint32 uidTarget = 0, uint32 nStatus = CLIENT_NORMAL  );
	//void NotifyBuddyStatusEX( SFIDRecord * pRec, uint32 uidTargetID, uint32 uidTargetnStatus,  const char* statestring );
	bool NotifyUserServiceChanged( SFIDRecord * pRec, bool bUpdateGroup = false );

	void DestroyRecord( SFIDRecord * pRec );
	void DestroyVfonSessionList( VfonSessionList * pList );

	bool UpdateAccessFlags( SFIDRecord * pRec );

protected: // message handlers

	// handle client request
	bool OnMsgClientSayHello( const SIM_REQUEST * pRQ );
	bool OnMsgClientTestFirewall( const SIM_REQUEST * pRQ );
	bool OnMsgClientHeartbeat( const SIM_REQUEST * pRQ );
	bool OnMsgClientLocate( const SIM_REQUEST * pRQ );
	bool OnMsgClientEnterRoom( const SIM_REQUEST * pRQ );
	bool OnMsgClientLeftrRoom( const SIM_REQUEST * pRQ );
	bool OnMsgClientLeftRoom( const SIM_REQUEST * pRQ );

	bool OnMsgClientLogout( const SIM_REQUEST * req );
	bool OnMsgClientChangeStatus( const SIM_REQUEST * req );

// ------ compartible only for version before 2004.06.07 -------------------
	bool OnMsgClientQueryOldVersionInfo( const SIM_REQUEST * req );
// ------ compartible end --------------------------------------------------

	bool OnMsgClientQueryVersionInfo( const SIM_REQUEST * req );
	bool OnMsgClientQueryService( const SIM_REQUEST * req );

	bool OnMsgClientRequestChannel( const SIM_REQUEST * req );
	bool OnMsgClientResponseChannel( const SIM_REQUEST * req );

	bool OnMsgClientRegisterUser( const SIM_REQUEST * req );

	bool OnMsgClientUpdateUserData( const SIM_REQUEST * req );
	bool OnMsgClientViewUserData( const SIM_REQUEST * req );
	bool OnMsgClientGetUserTechParam( const SIM_REQUEST * req );

	bool OnMsgClientUpdateCallForwardInfo( const SIM_REQUEST * req );
	bool OnMsgClientGetCallForwardInfo( const SIM_REQUEST * req );

	bool OnMsgClientListOnlineUser( const SIM_REQUEST * req );
	bool OnMsgClientSearchUser( const SIM_REQUEST * req );

	bool OnMsgClientAddUser( const SIM_REQUEST * req );
	bool OnMsgClientDeleteUser( const SIM_REQUEST * req );
	bool OnMsgClientGetUserList( const SIM_REQUEST * req );
	bool OnMsgClientGetUserListCompr( const SIM_REQUEST * req );

	bool OnMsgClientUploadDataBlock( const SIM_REQUEST * req );
	bool OnMsgClientDownloadDataBlock( const SIM_REQUEST * req );
	bool OnMsgClientDownloadDataBlockCompr( const SIM_REQUEST * req );

	bool OnMsgClientUserMsg( const SIM_REQUEST * req );

	bool OnMsgClientQueryGKServerList( const SIM_REQUEST * req );
	bool OnMsgClientQueryPeoxyServerList( const SIM_REQUEST * req );

	bool OnMsgProxyNotify( const SIM_REQUEST * req );

	// for proxy engine
	bool OnMsgCreateProxyChannel( const SIM_REQUEST * req );
	bool OnMsgReleaseProxyChannel( const SIM_REQUEST * req );

	bool OnMsgClientSubscribe( const SIM_REQUEST * req );

	bool DoClientLoginEx( const SIM_REQUEST * req, const SIMD_CS_LOGIN_EX * pQ, SIM_REQUEST & reply, char * secret_key = "", int key_size = 0 );
	bool OnMsgClientLoginEx( const SIM_REQUEST * req );

	bool OnMsgClientDownloadServerPKICert( const SIM_REQUEST * req );
	bool OnMsgClientDownloadServerPKICertCompr( const SIM_REQUEST * req );
	bool OnMsgClientLoginExSecure( const SIM_REQUEST * req );

	bool OnMsgClientLocateEx( const SIM_REQUEST * req );
	bool OnMsgServerLocateEx( const SIM_REQUEST * req );
	bool OnMsgClientConference( const SIM_REQUEST * req );

	bool OnMsgClientLoadString( const SIM_REQUEST * req );
	bool OnMsgClientLoadStringCompr( const SIM_REQUEST * req );
	bool OnMsgClientGetBindInfo( const SIM_REQUEST * req );

	// for status & admin
	bool OnMsgProxyStatus( const SIM_REQUEST * req );

	bool OnMsgServerStatus( const SIM_REQUEST * req );
	bool OnMsgServerFeedbackStatus( const SIM_REQUEST * req );

	bool OnMsgServerSwitch( const SIM_REQUEST * req );

	bool OnMsgClientGetMCUList( const SIM_REQUEST * req );

	bool OnMsgClientGetIdTrees( const SIM_REQUEST * req );
	bool OnMsgClientGetIdTreesCompr( const SIM_REQUEST * req );
	bool OnMsgXSLoginReplaced( const SIM_REQUEST * req );
	bool OnMsgXSAnswerSwitchInfo( const SIM_REQUEST * req );
	bool OnMsgXSAnswerLocateUser( const SIM_REQUEST * req );
	bool OnMsgXSQueryLocateUser( const SIM_REQUEST * req );
	bool OnMsgXSBuddyStatusNotify( const SIM_REQUEST * req );
#ifdef D_OLD_SWITCH
	bool OnMsgSXUpdateSwitchInfo( const SIM_REQUEST * req );
	bool OnMsgSXUpdateSwitchInfoEx( const SIM_REQUEST * req );

	bool OnMsgSXQuerySwitchInfo( const SIM_REQUEST * req );

	bool OnMsgSXQueryLocateUser( const SIM_REQUEST * req );
	bool OnMsgSXAnswerLocateUser( const SIM_REQUEST * req );
	bool OnMsgSXBuddyStatusNotify( const SIM_REQUEST * req );
#endif
	bool OnMsgSSServerUpdate( const SIM_REQUEST * req );

	bool OnMsgSSDomainUpdate( const SIM_REQUEST * req );

	bool OnMsgSSDomainSync( const SIM_REQUEST * req );
	bool OnMsgSSDomainSyncData( const SIM_REQUEST * req );

	bool OnMsgClientNotifyMeetingEvent( const SIM_REQUEST * req );
	bool OnMsgClientMeetingInvite( const SIM_REQUEST * req );

	bool OnMsgClientNotifyReady( const SIM_REQUEST * req );


	bool OnMsgClientAuthContact( const SIM_REQUEST * req );

	bool OnMsgClientLogUserAction( const SIM_REQUEST * req );

	bool OnMsgClientSendOfflineIM( const SIM_REQUEST * req );
	bool OnMsgClientGetOfflineIM( const SIM_REQUEST * req );
	bool OnMsgClientDeleteOfflineIM( const SIM_REQUEST * req );
	bool OnMsgClientGetMCCInfo( const SIM_REQUEST * req );
  bool OnMsgClientQueryStringToStringInfo( const SIM_REQUEST * req );
	bool OnMsgXSNotifyBuddyStatusNotifylist( const SIM_REQUEST * req );

protected:

	virtual PString URLTranslateString( const PString & str ) { return str; }

	virtual bool ValidateAdminPassword( const char * username, const char * password );

	enum SaveEvent {
		SaveOnChange,
		SaveOnLogin,
		SaveOnLogout,
	};

	bool SaveData( SFIDRecord * pRec, SaveEvent event, BBQUserValidFlags * pFlags = NULL );

protected:

	int SyncBuddyRecordFromDB( uint32 uid );

	int SyncGroupRecordFromDB( uint32 uid );

protected:	
	// ---------------------------------------------------------
	// node relationship link and tree info
	// ---------------------------------------------------------

	bool LoadLayeredLinkData( SFIDRecord * pRec ); // load parent id list and child id list from DB

	// recursing call, here using map just for a quick find
	bool GetAllParentUserNodes( uint32 nId, RecordMap & result, RecordMap * pSearchedNodes = NULL, int depth = 0 );

	// recursing call, here using map just for a quick find
	bool GetRootUserNodes( uint32 nId, RecordMap & result, RecordMap * pSearchedNodes = NULL, int depth = 0 );

	// recursing call, get the sub trees for these nodes
	bool GetSubUserTree( uint_list & idList, BBQTrees & trees, int depth = 0, uint32 idSearchStart = 0 ); 

	// get all the user record in sub tree
	bool GetSubUserRecords( uint_list & idList, RecordMap & records, int depth = 0 ); 

	bool IsMyParent( uint32 nMe, uint32 nGuest );
	bool IsContainedInSameTree( uint32 nIdA, uint32 nIdB ); 
	bool IsBuddyOfMyParents( uint32 nMe, uint32 nGuest );
	bool IsAllowedByMeAndParents( uint32 nMe, uint32 nGuest );
	bool IsBlockedByMeAndParents( uint32 nMe, uint32 nGuest );

	bool GetRootUserTrees( uint32 nId, BBQTrees & trees );

	// called in OnTick()
	virtual void CheckDongleStatus( void );
  bool CheckAndUpdateUserHDVideoFlags(const uint32 uid, BBQACLFlagExt* pAclExt);
  bool CheckAndRemoveUserHDVideoFlags(const uint32 uid );
  PString  GetUserHDVideoTypeString(const uint32 uid);
	void CheckProxyStatus( void );
	void CheckUserStatus( void );
	void CheckMeetingStatus( void );

	void CheckQosStatus( void );

	bool UserOffline( SFIDRecord * pRec, int bDrop = 0 );
  //type,0=add users,1=update conf time
	bool AddOrChangeMeeting( BBQ_MeetingInfo_Ext * pNew  ,int type=-1, void* tag=NULL);
	bool DestroyMeetingInfo( BBQ_MeetingInfo_Ext * pMExt );

	bool UpdateUserMeetingRef( const BBQ_MeetingInfo_Ext * pExt );
	bool ClearMeetingInfo( const char * lpszMeetingId );
	bool LoadMeetingInfo( time_t t1 = 0, time_t t2 = 0, uint32 nRoomId = 0, const char * lpszMeetingInfo = NULL );
	bool LoadNewOrChangeMeetingById( const char * lpszMeetingInfo = NULL );
	bool ReNotifyMeetingById( const char * lpszMeetingId = NULL );

	bool InviteUserForMeeting( uint32 uid, const char * lpszMeetingId );

  bool NotifyClientMeetingEvent( BBQ_MeetingInfo_Ext * pMExt, uint32 nEventId, bool bAlsoNotifyMember = false,uint32 nEventTag=0xffffffff/*ext nEventId*/,const  PIntArray*  NotifyMembers=NULL);
	void NotifyClientBuddiesStatuString(SFIDRecord* pRec);

protected:	
	// ---------------------------------------------------------
	// domain relationship link and tree info
	// ---------------------------------------------------------
	bool GetDomainNeighborChannels( ChannelMap & chm );

public:
	// -------------------- administraing interface -------------
	// save those records that have been updated after server up
	bool SaveData( bool bForceAll = false );

	// query record
	bool GetRecords( SFIDRecord* & pRecArray, int & nGet, int & nTotal, uint32 from, uint32 nMaxCount, bool bOnlineOnly = false );

	SFIDRecord * SafeFindRecordBySerial( uint32 nProductSerial );
	SFIDRecord * SafeFindRecordByID( uint32 nCallerID );

	bool Kick( uint32 uid );

	int NotifyUserServiceChangedToAll( void );
	bool NotifyUserServiceChanged( uint32 uid );

	int SendTextMessageToClient( uint32 nCallerID, uint32 nMsgID, const char * lpszURLs );
	int BroadcastTextMessageToClients( const char * lpszText );

	void EnableSerialKeyCheck( bool bEnable = true );
	bool IsSerialKeyCheckEnabled( void );

	bool GetClientVersionInfo( SIMD_VERSIONINFO* pVerInfo, PString & strDownloadURL );
	bool SetClientVersionInfo( const SIMD_VERSIONINFO* pVerInfo, PString & strDownloadURL );

	void SetServerVersionInfo( uint32 nMajor, uint32 nMinor, uint32 nBuild ) {
		m_VersionInfo.server.major = nMajor;
		m_VersionInfo.server.minor = nMinor;
		m_VersionInfo.server.build = nBuild;
	}

	virtual bool IsCallAllowed( uint32 nFrom, uint32 nTo );
	bool  IsOnlineUser(SFIDRecord* pRec)
	{
		time_t tNow;
		time(&tNow);
		return tNow - pRec->time <  m_config.heartbeat_timeout;
	}
	SIM_CHANNEL*   IsDomainChannelByPeer(const IPSOCKADDR&				peer);//need to lock the m_mutextrecord
	SIM_CHANNEL*   GetDomainChannelByID( uint32 id);
	bool IsValidDomainID( uint32 serverid);

};

#endif /* _BBQ_MANAGER_H */
