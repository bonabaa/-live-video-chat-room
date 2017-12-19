
#ifndef _BBQ_ID_MSG_H
#define _BBQ_ID_MSG_H

#include "sfidmsg.h"
#include "bytepack.h"
#include "bbquserinfo.h"
#include "bbqmcc.h"

SIMPLE_PACK( BBQUserTechParamInfo )

//PBytePack & operator<< (PBytePack & pack, BBQUserBasicInfo & ubi);
//PBytePack & operator>> (PBytePack & pack, BBQUserBasicInfo & ubi);
//
//PBytePack & operator<< (PBytePack & pack, BBQUserContactInfo & uci );
//PBytePack & operator>> (PBytePack & pack, BBQUserContactInfo & uci );
//
//PBytePack & operator<< (PBytePack & pack, BBQUserDetailedInfo & udi );
//PBytePack & operator>> (PBytePack & pack, BBQUserDetailedInfo & udi );
//
//PBytePack & operator<< (PBytePack & pack, BBQUserInfo & ui );
//PBytePack & operator>> (PBytePack & pack, BBQUserInfo & ui );

// security.oldpass & newpass is 32 bytes, fully used for MD5, 
// so cannot pack as null-terminated string, do a simple pack here
SIMPLE_PACK( BBQUserInfo )
SIMPLE_PACK( VfonIdBindInfo )

PBytePack & operator<< (PBytePack & pack, BBQSearchCondition & sc );
PBytePack & operator>> (PBytePack & pack, BBQSearchCondition & sc );

PBytePack & operator<< (PBytePack & pack, BBQSearchResult & sr );
PBytePack & operator>> (PBytePack & pack, BBQSearchResult & sr );
PBytePack & operator<< (PBytePack & pack, SIM_MESSAGE_COMPR::simCompress & sr );
PBytePack & operator>> (PBytePack & pack, SIM_MESSAGE_COMPR::simCompress & sr );

// ---------------------- common data structure ------------------------------------

#pragma pack(1)

#define SIM_KEYSIZEMAX			32
#define SIM_ALIASSIZEMAX		32
#define SIM_STRINGSIZEMAX		256
#define D_SENDFILE_PACKET_SIZE		1100

typedef struct SIM_SESSIONINFO {
	uint32		id;
	uint32		cookie;			// generated radomly
} PACKED SIM_SESSIONINFO;

SIMPLE_PACK( SIM_CHANNEL )
SIMPLE_PACK( SFIDMSGHEADER )
SIMPLE_PACK( SIM_SESSIONINFO )

SIMPLE_PACK( BBQACLFlag )
SIMPLE_PACK( BBQUserServiceInfo )

// ----------------------- Message ID dependent data structure ---------------------

typedef struct SIMD_SC_SIMPLE {
	uint32				statusCode;
} PACKED SIMD_SC_SIMPLE;

typedef struct SIMD_SC_COMMON {
	uint32				statusCode;
	char				statusText[ SIM_STRINGSIZEMAX ];	// NULL terminated string, length no more than 255
} PACKED SIMD_SC_COMMON;

typedef struct SIMD_CS_HELLO {
	uint32		versionCode;
} PACKED SIMD_CS_HELLO;

typedef struct SIMD_CS_KICK {
} PACKED SIMD_CS_KICK;

typedef struct SIMD_SC_KICK {
} PACKED SIMD_SC_KICK;

typedef struct SIMD_SC_HELLO {
	uint32		versionCode;
	// reported by server to client,
	// it's the WAN ip:port of client, seen by server.
	uint32		clientIp;
	uint16		clientPort;
	// added on June 22, 2004,
	// optional, might be 0, if non-zero, means another server address, for testing firewall only.
	uint32		anotherServerIp;
	uint32		anotherServerPort;
} PACKED SIMD_SC_HELLO;

typedef struct UCMD_CC_IDCONFIRM {
	uint32		statusCode;

	VFONID		vfonid;				// serverid, userid

	char		reserved[ 244 ];
} PACKED UCMD_CC_IDQUERY, UCMD_CC_IDCONFIRM;

typedef struct SIMD_SC_PROXYSTATUS {
	uint32		statusCode;

	struct {
		uint32	major, minor, build;
	} PACKED Version;

	uint32		nUserMax;
	time_t		tExpireTime;

	time_t		tNowTime, tUpTime;

	WORD		nTcpPort;
	WORD		nUdpPortMin, nUdpPortMax;

	int			nChannelMax;
	int			nChannelBufferMax;
	int			nBandwidthMax;					// in Kbps

	int			nChannels;
	int			nBufferBytes;
	int			nAverageBandwidth;				// in Kbps

	char		reserved[ 194 ];
} PACKED SIMD_SC_PROXYSTATUS;

typedef struct SIMD_CS_SERVERSTATUS {
	uint32		link_type;				// 0 - info, 1 - interlink, 2 - uplink

	uint32		link_id;				// also used as domain prefix
	char		link_name[ 64 ];		// also used as domain alias, for example, 0100176 @ wor-tel
	char		link_password[ 32 ];	// md5

	char		reserved[ 152 ];

} PACKED SIMD_CS_SERVERSTATUS; // 256 bytes

typedef struct SIMD_SERVER_FEATURES {
	unsigned int	disable_subscribe : 1;			// subscribe is disabled
	unsigned int	enable_cug : 1;					// cug is enabled
	unsigned int	run_in_asp_mode : 1;			// run in ASP mode, the group root node is company, the second layer node is isolated group
	unsigned int	allow_cug_call_public : 1;		// allow CUG users call public users
	unsigned int	cug_have_private_buddy : 1;		// allow CUG users have private buddy list
	unsigned int	padding : 27;
} PACKED SIMD_SERVER_FEATURES;

typedef struct SIMD_SC_SERVERSTATUS {
	uint32					statusCode;					// 4

	struct {
		uint32				major, minor, build;
	} PACKED Version;								// 12

	uint32					nUserMax;					// 4
	time_t					tExpireTime;				// 4

	time_t					tNowTime, tUpTime;			// 8

	uint32					nServerId;					// 4

	uint32					nServerType;				// 4
	uint32					nServerStatus;				// 4

	uint32					nUserCount, nMaxUserCount;		// 8
	uint32					nProxyCount, nMaxProxyCount;	// 8
	uint32					nPureProxyCount, nMaxPureProxyCount;	// 8

	uint16					nInterlinkConnected;					// 2
	IPSOCKADDR				anotherServerAddress;					// 6

	uint16					nUplinkConnected;						// 2
	IPSOCKADDR				uplinkServerAddress;					// 6

	uint32					link_id;

	SIMD_SERVER_FEATURES	features;

	char					reserved[ 174 ];
} PACKED SIMD_SC_SERVERSTATUS;	// 256

typedef struct SIMD_CS_SERVERSWITCH {

	uint32		serverType;			// STANDALONE, PRIMARY, SECONDARY, CLUSTER
	uint32		serverStatus;		// ACTIVE, STANDBY, DOWN

	char		username[ 32 ];		// null-terminated
	char		password[ 32 ];		// null-terminated, or full 32 chars MD5

	char		reserved[ 184 ];	

} PACKED SIMD_CS_SERVERSWITCH;

typedef struct SIMD_SC_SERVERSWITCH {

	uint32		statusCode;			// SIMS_OK to confirm, other for failure.

	uint32		serverType;			// type after switch
	uint32		serverStatus;		// status after switch

	IPSOCKADDR	anotherServerAddress;	// if i am standby or down now, pls connect to another server.

	char		reserved[ 238 ];

} PACKED SIMD_SC_SERVERSWITCH;

typedef struct SIMD_CS_LOGIN {
	uint32						serialID;
	char						keyPassword[ SIM_KEYSIZEMAX ];
	uint32						favouriteID;
	uint32						serviceType;
	uint32						statusCode;			// normal, or invisible
	BBQUserTechParamInfo		techParam;
	uint32						providerID;
} PACKED SIMD_CS_LOGIN;

typedef struct SIMD_SC_LOGIN {
	SIM_SESSIONINFO		sessionInfo;
	uint32				statusCode;
	BBQACLFlag			accessFlags;
	char				statusText[ SIM_STRINGSIZEMAX ];	// NULL terminated string, length no more than 255
} PACKED SIMD_SC_LOGIN;

typedef struct SIMD_CS_LOCATE {
	SIM_SESSIONINFO		sessionInfo;		// 8
	uint32				queryID;			// 4
	uint8				thisServerOnly;		// 1
	char				reserved[ 3 ];		
} PACKED SIMD_CS_LOCATE;

typedef struct SIMD_SC_LOCATE {
	uint32				queryID;

	uint32				company_id;

	char				reserved[ 4 ];
  WORD       tcplistenport;
  uint32     transportchannelmodes;
	IPSOCKADDR			addrServer;	
	uint8				isTCP;
	uint8				firewallType;

	SIM_CHANNEL			channelServerSide;		// for server-side only

	IPSOCKADDR			addrLAN;
	IPSOCKADDR			addrWAN;
	uint32				statusCode;

} PACKED SIMD_SC_LOCATE;


typedef struct SIMD_CS_HEARTBEAT {
	SIM_SESSIONINFO		sessionInfo;

	uint16				proxySlot;				// empty proxy channel slot, that can serve others. 
	uint16				proxyMax;				// max proxy channel.

	uint32				proxyBandwidthUsed;		// bandwidth used, in Kbps
	uint32				proxyBandwidthTotal;	// total bandwidth, in Kbps

	union {
		struct { // only used for MCU to report info
			uint16		rooms;
			uint16		persons;
			uint32		bps;
		}				mcuInfo;
		struct { // only used for VProxy to report info
			uint16		adminPort;
			uint16		tcpPort;
			uint16		udpPortMin;
			uint16		udpPortMax;
			uint32		serviceIp;
			uint16		hasTag;
		}				proxyInfo;
		struct {
			char			hasConferenceStatusTag;//if hasConferenceStatusTag is 1, then the packet contain a conference status value.
		}       vmeetInfo;
		char			reserved[ 26 ];
	}PACKED ;

	IPSOCKADDR			addrLAN;
} PACKED SIMD_CS_HEARTBEAT;

typedef struct SIMD_CS_LOGOUT {
	SIM_SESSIONINFO		sessionInfo;

} PACKED SIMD_CS_LOGOUT;

enum BBQ_CLIENT_STATUS {
	CLIENT_NORMAL,
	CLIENT_BUSY,
	CLIENT_ONTHEPHONE,
	CLIENT_AWAY,
	CLIENT_BERIGHTBACK,
	CLIENT_OUTLUNCH,
	CLIENT_INVISIBLE,
	CLIENT_INCONF,

	CLIENT_MCU = 0xFE,
	CLIENT_OFFLINE = 0xFF,
};

enum BBQ_CLIENTSETTING_STATUS {
	CLIENT_ANSWERMACHINE_ON = 0x01,
	CLIENT_CALLFORWARD_ON = 0x02,
	CLIENT_SMSALERT_ON = 0x04,
};

typedef struct SIMD_CS_CHANGESTATUS {
	SIM_SESSIONINFO			sessionInfo;
	uint32					statusCode;
	char					statusString[ SIM_STRINGSIZEMAX ];	// NULL terminated string, length no more than 255
} PACKED SIMD_CS_CHANGESTATUS;

typedef struct SIMD_SC_LOGINREPLACED {
	SIM_SESSIONINFO			sessionInfo;
	IPSOCKADDR				addrReplaceBy;
} PACKED SIMD_SC_LOGINREPLACED;

// ======================== Message to setup UDP channel between clients ==============

typedef struct UCMD_CHANNELINFO {
	IPSOCKADDR			peerWAN;
	IPSOCKADDR			peerLAN;
	IPSOCKADDR			thisWAN;
	IPSOCKADDR			thisLAN;

	uint32				uid;					// peer's uid
	uint32				cookie;					// channel cookie, for security checking

	unsigned int		init : 1;				// true means local is the one that request this channel
	unsigned int		invite : 1;				// true means local is accessable as weak
	unsigned int		firewall : 2;			// firewall type, 0,1,2,3
	unsigned int		tcp : 1;				// true for a tcp channel, when udp blocked
	unsigned int		ssl : 1;

	unsigned int		cid : 25;				// local channel id/counter

	unsigned int		give_me_proxy : 1;		// request server to allocate proxy
} PACKED UCMD_CHANNELINFO;

typedef struct UCMD_CHANNEL_EXTRA {
	DWORD				dwTimeBase;				// local time base
	DWORD				dwChannelId;			// local counter for channel
	uint32				nServerId;				// local domain id

	uint32				nAppParam;				// parameter passed by upper calling

	unsigned int		use_firewall_here : 1;	// if non-zero, then use firewall here
	unsigned int		firewall : 2;
	unsigned int		padding : 29;

	char				reserved[ 12 ];
} PACKED UCMD_CHANNEL_EXTRA;

#define SIM_APPSTRINGSIZEMAX	384

typedef struct UCMD_CS {
	SIM_SESSIONINFO		sessionInfo;

	UCMD_CHANNELINFO	channelInfo;
	int					statusCode;
	char				statusText[ SIM_APPSTRINGSIZEMAX ];

	struct {
		UCMD_CHANNEL_EXTRA	caller;
		UCMD_CHANNEL_EXTRA	called;
	} PACKED extraInfo;

	SIM_CHANNEL			callerChannel;
  DWORD       proxyIp;//chen yuan
	char				reserved[ 44 ];

} PACKED UCMD_CS_Q, UCMD_CS_A, UCMD_SC_FWD_Q, UCMD_SC_A, UCMD_CC_Q, UCMD_CC_A;
typedef struct SIMD_CS_ENTER_ROOM {
	SIM_SESSIONINFO		sessionInfo;
	uint32 nRoomID;
} PACKED  SIMD_CS_ENTER_ROOM;
typedef struct SIMD_CS_LEFT_ROOM {
	SIM_SESSIONINFO		sessionInfo;
	uint32 nRoomID;
} PACKED SIMD_CS_LEFT_ROOM ;
typedef struct SIMD_SC_ENTER_ROOM {
	SIM_SESSIONINFO		sessionInfo;

	char				calllist[ 256 ];
	uint32        caller;
} PACKED SIMD_SC_ENTER_ROOM ;
typedef struct SIMD_SC_ENTER_ROOM_NOTIFY {
  uint32      whocamein_ID;
  char        whocamein_Alias[32];
} PACKED SIMD_SC_ENTER_ROOM_NOTIFY ;
typedef struct SIMD_SC_LEFT_ROOM {
	SIM_SESSIONINFO		sessionInfo;

	//char				calllist[ 256 ];
	//uint32        caller;
} PACKED SIMD_SC_LEFT_ROOM ;
typedef struct UCMD_CC {
	UCMD_CHANNELINFO	channelInfo;
	int					statusCode;
	char				statusText[ SIM_APPSTRINGSIZEMAX ];

	struct extraInfo {
		UCMD_CHANNEL_EXTRA	caller;
		UCMD_CHANNEL_EXTRA	called;
	} PACKED extraInfo;

	char				reserved[ 64 ];

} PACKED UCMD_CC_CON, UCMD_CC_CONCFM;

typedef struct UCMD_PROXY {
	SIM_SESSIONINFO		sessionInfo;

	// identity in proxy
	uint32				nChannelId;				// proxy's channel id
	uint32				cookieProxy;			// cookie created by proxy

	// identity in client
	struct {
		uint32			uid;
		IPSOCKADDR		addr;					// proxy ip:port for the client to connect
	} PACKED entry[ 2 ];		 

	UCMD_CHANNELINFO	channelInfo;

	uint16				proxySlot;				// empty proxy channel slot, that can serve others. 
	uint16				proxyMax;				// max proxy channel.

	uint32				proxyBandwidthUsed;		// bandwidth used, in Kbps
	uint32				proxyBandwidthTotal;	// total bandwidth, in Kbps

	char				reserved[ 8 ];			// reserved bytes

	int					statusCode;

	int					appCode;
	char				appString[ SIM_APPSTRINGSIZEMAX ];

	struct tagExtraInfo {
		UCMD_CHANNEL_EXTRA	caller;
		UCMD_CHANNEL_EXTRA	called;
	} PACKED extraInfo;

	SIM_CHANNEL			callerChannel;

	char				reserved2[ 48 ];

} PACKED UCMD_PROXY_CREATE, UCMD_PROXY_RELEASE, UCMD_PROXY_NOTIFY, UCMD_PROXY_NOTIFYCLIENT; 

typedef struct SIMD_CS_SUBSCRIBE {
	BBQUserInfo				userInfo;

	char					hardwareId[ USERDATA_NAME_SIZE ];
	char					cdKey[ USERDATA_ADDRESS_SIZE ];
	uint32					providerID;

	// optional, client can specify an id as his prefered id, 
	// if set to 0, server will assign an id for him.
	uint32					preferedId;

	struct {
		unsigned int			use_alias_as_bindkey : 1;
		unsigned int			padding : 31;
	} PACKED ;

	char					reserved[ 128 ];
} PACKED SIMD_CS_SUBSCRIBE; // 1024 bytes

typedef struct SIMD_SC_SUBSCRIBE {
	uint32					statusCode;

	VFONID					vfonId;

	time_t					tStartTime, tExpireTime, tNowServerTime;
	int						nPoints;
	
	char					reserved[ 100 ];
} PACKED SIMD_SC_SUBSCRIBE; // 256 bytes

enum {
	SECRET_PLAIN	= 0,
	SECRET_MD5		= 1,
	SECRET_SHA1		= 2,
	SECRET_AES128	= 3,
	SECRET_AES256	= 4,
	SECRET_RSA		= 5,
	SECRET_RSA_SIGN	= 6,
};

#define	PI_SQRT2_STRING		"3.1415926535897932384626433832795*1.1486983549970350067986269467779"
#define	PI_SQRT2_STRING_EX		"1.3215926535897932394626437232795*1.1486983549970350067986269467771"

typedef struct SIMD_SECRET {
	uint16					secret_purpose;			// SECRET_PLAIN, SECRET_AES, etc.
	uint16					secret_size;				// plain size in bytes, AES 128bit: 16 bytes, AES 256 bit: 32 bytes				
	uint16					encrypt_method;			// SECRET_PLAIN, SECRET_AES, etc.
	uint16					encrypted_data_size;
	char					encrypted_data[ 1 ];
} SIMD_SECRET;

typedef struct SIMD_SC_GET_PKI_CERT	{
	uint32					statusCode;
	char					strFormat[ 32 ];	// usually PEM
	char					strPEM[1];			// null terminated string
} SIMD_CS_GET_PKI_CERT, SIMD_SC_GET_PKI_CERT;

typedef struct SIMD_CS_LOGIN_EX {
	VFONID					vfonId;
	char					lpszPassword[ USERDATA_NAME_SIZE ];
	char					lpszHardwareId[ USERDATA_NAME_SIZE ];	// optional, to limit access from only 1 PC

	VfonIdBindInfo			bindInfo;

	BBQUserTechParamInfo	techParam;

	uint32					onlineStatus;

	char					clientApp[ 32 ]; // [MCU], [VMEET], [MHAVC] ...
} PACKED SIMD_CS_LOGIN_EX; // 512 bytes

enum {
	ERR_NONE		= 0,
	ERR_CLIENTAPP	= 1,
	ERR_VERSION		= 2,
	ERR_AUTHFAIL	= 3,
	ERR_TOOMANY		= 4,
	ERR_FORBIDDEN	= 5,
	ERR_ISGROUPID	= 6,
	ERR_NOLOGINMATCH		= 7,//use mcu id with vmeet to login than is not allow
	ERR_REPLACEFROMOTHERPLACE		= 8,// 
	//chenyuan
	ERR_DENY_LIMIT_CONCURRENT_TOTAL,
	ERR_DENY_LIMIT_CONCURRENT_VMEETS,
	ERR_DENY_LIMIT_REGISTER_TOTAL,
	ERR_DENY_LIMIT_REGISTER_VMEETS,
	ERR_DENY_LIMIT_CONF_PEOPLE_TOTAL,
	ERR_EXPIRED	= 0xF0,
};

typedef struct SIMD_SC_LOGIN_EX {
	uint32					statusCode;

	SIM_SESSIONINFO			sessionInfo;

	BBQACLFlag				accessFlags;

	time_t					tStartTime, tExpireTime, tNowServerTime;
	int						nPoints;

	IPSOCKADDR				anotherServer;	// 6 bytes, if return SIMS_NOTAVAILABLE, turn to ip:port of another server.

	uint32					nServerId;

	BBQACLFlagExt			extAccessFlags;	// 12 bytes

	struct {
		unsigned int		being_asked_as_buddy : 1;
		unsigned int		have_offline_message : 1;
		unsigned int		have_meeting : 1;
		unsigned int		padding : 5;
		unsigned int		setStatus : 8;
		unsigned int		padding2 : 16;
	} PACKED ;

	uint16					nHiddenBuddyCount;

	uint16					errCode;	// error code if login failed

	char					reserved[ 4 ];
} PACKED SIMD_SC_LOGIN_EX;

#define	SIM_LOCATE_EX_MAX_OLD	16
#define	SIM_LOCATE_EX_MAX	10

typedef struct SIMD_CS_LOCATE_EX_OLD {
	SIM_SESSIONINFO		sessionInfo;								// 8

	// if vfon.serverid == 0, then use the domain alias to find out the domain id at first
	// if vfon.id == 0, then using the bind information to get vfon id at first
	VFONID				vfonid;										// 8, 

	char				lpszBindType[ USERDATA_NAME_SIZE ];			// 32
	char				lpszBindId[ USERDATA_ADDRESS_SIZE ];		// 64
	char				lpszDomainAlias[ USERDATA_ADDRESS_SIZE ];	// 64, domian alias, if empty, local domain

	char				reserved[ 76 ];								// reserved for future expanding
	
	uint32				nPathNodeCount;								// 4
	SIM_CHANNEL			PathChannels[ SIM_LOCATE_EX_MAX_OLD ];			// 16 * N

} PACKED SIMD_CS_LOCATE_EX_OLD;	// 256 + 16 * N
typedef struct SIMD_CS_LOCATE_EX {
	SIM_SESSIONINFO		sessionInfo;								// 8

	// if vfon.serverid == 0, then use the domain alias to find out the domain id at first
	// if vfon.id == 0, then using the bind information to get vfon id at first
	VFONID				vfonid;										// 8, 

	char				lpszBindType[ USERDATA_NAME_SIZE ];			// 32
	char				lpszBindId[ USERDATA_ADDRESS_SIZE ];		// 64
	char				lpszDomainAlias[ USERDATA_ADDRESS_SIZE ];	// 64, domian alias, if empty, local domain

	char				reserved[ 76 ];								// reserved for future expanding
	
	uint32				nPathNodeCount;		
	// client donot use, it used by server to server
	SIM_CHANNEL_WITH_SEQ			PathChannels[ SIM_LOCATE_EX_MAX ];			// 16 * N
	//char szStatusString[96];

} PACKED SIMD_CS_LOCATE_EX;	// 256 + 16 * N
typedef struct SIMD_SC_LOCATE_EX {
	uint32					statusCode;			// 4

	VFONID					vfonid;				// 8, if vfon.id == 0, then using the bind information to get vfon id at first

	uint32					dwServerIp;			// 4
	uint16					nServerTcpPort;		// 2
	uint16					nServerUdpPort;		// 2

	BBQUserTechParamInfo	techInfo;			// 16

	uint8					onlineStatus;		// 1
	uint8					isTCP;				// 1
	IPSOCKADDR				addrWAN;			// 6

	time_t					timestamp;			// 8
	time_t					tNowServerTime;		// 8

	uint8					setStatus;			// 1

	char					reserved[ 27 ];

	uint32					nPathNodeCount;		// 4
	SIM_CHANNEL_WITH_SEQ			PathChannels[ SIM_LOCATE_EX_MAX ];			// 16 * N
	char szStatusString[96];
	char		alias[ USERDATA_NAME_SIZE ];
} PACKED SIMD_SC_LOCATE_EX;		// 128 + N * 16
typedef struct SIMD_SC_LOCATE_EX_OLD {
	uint32					statusCode;			// 4

	VFONID					vfonid;				// 8, if vfon.id == 0, then using the bind information to get vfon id at first

	uint32					dwServerIp;			// 4
	uint16					nServerTcpPort;		// 2
	uint16					nServerUdpPort;		// 2

	BBQUserTechParamInfo	techInfo;			// 16

	uint8					onlineStatus;		// 1
	uint8					isTCP;				// 1
	IPSOCKADDR				addrWAN;			// 6

	time_t					timestamp;			// 8
	time_t					tNowServerTime;		// 8

	uint8					setStatus;			// 1

	char					reserved[ 27 ];

	uint32					nPathNodeCount;		// 4
	SIM_CHANNEL				PathChannels[ SIM_LOCATE_EX_MAX_OLD ];	// 16

} PACKED SIMD_SC_LOCATE_EX_OLD;		// 128 + N * 16
enum em_Conf
{
  em_conf_enter=0, em_conf_left=1
};
typedef struct SIMD_CS_CONFERENCE {
	SIM_SESSIONINFO		sessionInfo;			// 8
  uint32        roomid;
	uint32				ActionUId;					// 4
	char				lpszConferenceId[ 64 ];	// 64, UUID for conference
	//char				h323TargetId[ 64 ];		// 64, if toId is zero, should use this H.323 target id, example: 8621114@210.22.176.194:1719
	//VFONID				hostId;					// 8, conference host's id
	//VfonSessionFlag		flags;					// 4, start/stop, usedVideo, usedVmeet, usedMCU, dropOffline, etc.
	uint32			action;				// 4, see em_Conf

	//char				CallId[ 64 ];				// 64
} PACKED SIMD_CS_CONFERENCE, SIMD_SC_CONFERENCE;

typedef struct SIMD_CS_GETBINDINFO {
	uint32				statusCode;
	VFONID				vfonId;
	VfonIdBindInfo		bindInfo;
} PACKED SIMD_CS_GETBINDINFO, SIMD_SC_GETBINDINFO;

typedef struct SIMD_SC_BUDDYSTATUSNOTIFY {
	SIM_SESSIONINFO		sessionInfo;			// 8

	BBQBriefNotifyInfo	info;					// 124

	union {
		struct {
			unsigned int	notify_back : 1;		// ask for notify back
			unsigned int	get_status_list : 1;	// get the status of the listed id
			unsigned int	notify_1by1 : 1;		// notify the listed id one by one
			unsigned int	padding : 29;
		};
		unsigned int		value;
	} flags;									// 4

	char				szStatusString[ 96 ];		// 96
	char				reserved[ 12 ];		// 12

	uint32				n;						// 4
	union {
		uint32				uidlist[1];			// 4
		ONLINE_USER			onlinelist[1];		// 4
	};

} PACKED SIMD_SC_BUDDYSTATUSNOTIFY, SIMD_SX_BUDDYSTATUSNOTIFY, SIMD_XS_BUDDYSTATUSNOTIFY; // 256+
#define		BUDDYNOTIFY_NOTIFYBACK		0x0001
#define		BUDDYNOTIFY_GETSTATUSLIST	0x0002
#define		BUDDYNOTIFY_NOTIFY1BY1		0x0004
typedef struct SIMD_CS_GET_MCC  {
	SIM_SESSIONINFO		sessionInfo;
	char				strMeetingId[ 64 ];			// unique session id, format: {room id}_{date time}, for example: 2201_20060101120000
} PACKED SIMD_CS_GET_MCC;

typedef struct SIMD_SC_MCC_NOTIFY {
	SIM_SESSIONINFO		sessionInfo;

	uint32				eventId;				// 0:INIT, 1:BEGIN, 2:END, 3:FAIL, ...
	BBQ_MeetingInfo		meetingInfo;

	time_t				tNowSenderTime;

	uint32				nEventTime;				// seconds

	uint32				toNewId;				// to specified new user

	union {
		struct {
			unsigned int	is_encrypted : 1;	// this notify message is encrypted with client's login session key
			unsigned int	padding : 31;
		};
		unsigned int		value;
	} flags;

	char				reserved[ 104 ];
} PACKED SIMD_SC_MCC_NOTIFY, SIMD_CS_MCC_NOTIFY, SIMD_CS_NOTIFY_READY, SIMD_CS_MCC_INVITE, SIMD_SC_GET_MCC;

typedef struct SIMD_SC_MCC_NOTIFY_EX {
	uint32				size;
	SIM_SESSIONINFO		sessionInfo;

	uint32				eventId;				// 0:INIT, 1:BEGIN, 2:END, 3:FAIL, ...
	time_t				tNowSenderTime;
	uint32				nEventTime;				// seconds

	char				reserved[ 112 ];

	BBQ_MeetingInfo_Ext	meetingInfoExt;			// various length

} PACKED SIMD_SC_MCC_NOTIFY_EX, SIMD_CS_MCC_NOTIFY_EX, SIMD_CS_NOTIFY_READY_EX;

typedef struct SIMD_SC_GETMCULIST {
	uint32				nBase;
	uint32				nCount;
	uint32				nTotal;
	MCURecord			inMCU[1];			// more
} PACKED SIMD_SC_GETMCULIST, SIMD_CS_GETMCULIST;

// ------ compartible only for version before 2004.06.07 -------------------
typedef struct SIMD_OLDVERSIONINFO {
	struct {
		uint32		major, minor, build;
		time_t		nowTime, upTime;
		int			userMax, userCount;
	} PACKED server; // 64 bytes
	struct {
		uint32		major, minor, build;
	} PACKED client; // 64 bytes
} PACKED SIMD_OLDVERSIONINFO;		// 128 bytes
// ------ compartible end --------------------------------------------------

typedef struct SIMD_VERSIONINFO {
	struct {
		uint32					major, minor, build;
		time_t					nowTime, upTime;
		int						userMax, userCount;
		SIMD_SERVER_FEATURES	features;
		char					reserved[ 32 ];
	} PACKED server; // 64 bytes
	struct {
		uint32					majorRequired, minorRequired, buildRequired;
		uint32					majorNew, minorNew, buildNew;
		char					reserved[ 52 ];
	} PACKED client; // 64 bytes
	struct {
		uint32 nConcurrentTotal;
		uint32 nConcurrentVmeets;
		uint32 nRegisterTotal;
		uint32 nRegisterVmeets;
		uint32 nConcurrectUsersInAllConference;
	}PACKED donglelimit; // 128 bytes
	char					reserved[ 108 ];
} PACKED SIMD_VERSIONINFO;		// 128 bytes

SIMPLE_PACK( SIMD_OLDVERSIONINFO )
SIMPLE_PACK( SIMD_VERSIONINFO )

// update user's call forward info
typedef struct SIMD_CS_UPDATECALLFWDINFO {
	SIM_SESSIONINFO			sessionInfo;		// 8
	uint32					statusCode;			// 4

	uint32					userID;				// 4
	char					reserved[ 256 ];	// 256

	BBQCallForwardInfo		callfwdinfo;		// 4 + 1024

} PACKED SIMD_CS_UPDATECALLFWDINFO, SIMD_SC_UPDATECALLFWDINFO, SIMD_CS_GETCALLFWDINFO, SIMD_SC_GETCALLFWDINFO;

typedef struct SIMD_SC_NOTIFY_CONTACT {
	SIM_SESSIONINFO			sessionInfo;		// 8
	uint32					statusCode;			// 4

	uint32					actionCode;			// 4
	uint32					userId;				// 4

	char					reserved[ 44 ];
} SIMD_SC_NOTIFY_CONTACT, SIMD_CS_AUTH_CONTACT, SIMD_SC_AUTH_CONTACT, SIMD_SC_MCC_INVITE;	// 64 bytes

typedef struct SIMD_CS_LOGUSERACTION {
	SIM_SESSIONINFO			sessionInfo;		// 8
	VfonUserActionLog		logInfo;
	char					dataBlock[ 1 ];		// string content of the pointers in logInfo
} SIMD_CS_LOGUSERACTION;

typedef struct SIMD_CS_TAG {
	SIM_SESSIONINFO			sessionInfo;		// 8

	uint32					statusCode;			// 4
	uint32					actionCode;			// 4, 0:NONE, 1:UPDATE_TAG, 2:GET_TAG

	char					reserved[ 64 ];

	uint32					tagLength;
	char					tagString[1];		// null terminated tag string

} SIMD_CS_TAG, SIMD_SC_TAG;

/* --------------------- user account related message --------------------------------------
// the following msg data is transfered in PACKED mode, using PBytePack 

// client version info
SIM_CS_VERSIONINFO: {
};

SIM_SC_VERSIONINFO: {
	SIMD_VERSIONINFO	versionInfo;

	string		lpszDownloadURL;
};

// query service
SIM_CS_QUERYSERVICE: {
	SIM_SESSIONINFO		sessionInfo;
};
SIM_SC_QUERYSERVICE: {
	uint32				statusCode;

	BBQACLFlag			aclFlag;
	BBQUserServiceInfo	serviceInfo;
};

// register an login account
SIM_CS_REGISTERUSER: {
	BBQUserInfo			userInfo;
};
SIM_SC_REGISTERUSER: {
	uint32				statusCode;
	uint32				userID;
};

// update user's data
SIM_CS_UPDATEUSERDATA: {
	SIM_SESSIONINFO		sessionInfo;
	BBQUserInfo			userInfo;
};
SIM_SC_UPDATEUSERDATA: {
	uint32				statusCode;
};

// update user's call forward info
SIMD_CALLFWDINFO {
	SIM_SESSIONINFO			sessionInfo;
	uint32					statusCode;
	uint32					userID;
	uint32					nCount;
	BBQCallForwardRecord	records[4];
}

// view user's data of specified ID
SIM_CS_VIEWUSERDATA: {
	SIM_SESSIONINFO		sessionInfo;
	uint32				queryID;
};
SIM_SC_VIEWUSERDATA: {
	uint32					statusCode;
	uint32					queryID;
	BBQUserInfo				userInfo;
	BBQUserTechParamInfo	techInfo;
	VfonIdBindInfo			bindInfo;
};

// list online neighbours, (on the same server)
SIM_CS_LISTONLINE: {
	SIM_SESSIONINFO		sessionInfo;
	uint32				nStart;
};
SIMD_SC_LISTONLINE: {
	uint32				statusCode;
	uint32				nTotal;
	uint32				nStart;
	uint32				nCount;
	BBQUserBriefRecord	userInfo[ nCount ];
};

// search user (from database)
SIM_CS_SEARCHUSER: {
	SIM_SESSIONINFO		sessionInfo;
	BBQSearchCondition	filterSearch;
	uint32				nStart;
};
SIM_SC_SEARCHUSER: {
	uint32				statusCode;
	uint32				nTotal;
	uint32				nStart;
	uint32				nCount;
	BBQUserBriefRecord	userInfo[ nCount ];
};

// add/delete contact to friend list
SIM_CS_ADDCONTACT, SIM_CS_ADDBLOCK, SIM_CS_DELETECONTACT, SIM_CS_DELETEBLOCK: {
	SIM_SESSIONINFO		sessionInfo;
	uint32				userID;
};
SIM_SC_ADDCONTACT, SIM_SC_ADDBLOCK, SIM_SC_DELETECONTACT, SIM_SC_DELETEBLOCK: {
	uint32				statusCode;
};
SIM_CS_GETCONTACTLIST, SIM_CS_GETBLOCKLIST: {
	SIM_SESSIONINFO		sessionInfo;
};
SIM_SC_GETCONTACTLIST {
	uint32				statusCode;
	uint32				nCount;
	uint32				userList[ nCount * 2 ]; // id, status
};

SIM_SC_GETBLOCKLIST: {
	uint32				statusCode;
	uint32				nCount;
	uint32				userList[ nCount ];
};

----------------------------------------------------------- */

enum BBQ_ENC_METHOD {
	BBQ_ENC_PLAIN	= 0,
	BBQ_ENC_MD5		= 1,
	BBQ_ENC_AES128	= 2,
	BBQ_ENC_AES256	= 3,
};

enum BBQ_USERMSG_ID {
	BBQ_USERMSG_NONE = 0,
	BBQ_USERMSG_AESKEY = 0x10,
	BBQ_USERMSG_CHAT = 0x20,
};

// send a message to another user
typedef struct SIMD_USERMSG {
	SIM_SESSIONINFO		sessionInfo;
	uint32				statusCode;
	VFONID				fromId;
	VFONID				toId;
	char				fromAlias[ USERDATA_NAME_SIZE ];	// filled by server
	char				reserved[ 32 ];
	union {
		struct {
			unsigned int encrypt_method : 8;	// BBQ_ENC_METHOD
			unsigned int need_echo : 1;
			unsigned int encrypted : 1;			// data encrypted
			unsigned int padding : 22;
		};
		uint32			value;
	} flags;
	uint32				nMsgMeaning;
	uint32				nPlainMsgLength;
	uint32				nEncryptedMsgLength;
	uint32				nExtraLength;
	char				msgData[ 1 ];	// msgData[ nPlainMsgLength ] + time_t + msgData[ nEncryptedMsgLength ]
} PACKED SIMD_USERMSG;

typedef struct SIMD_IM {
	SIM_SESSIONINFO		sessionInfo;
	uint16				statusCode;
	uint16				conpressed;
	uint32				nIndex;
	uint32				nTotal;
	BBQOfflineIMMessage	imMsg;
} SIMD_IM; // sizeof() + imMsg.length

typedef struct SIMD_DATABLOCK {
	SIM_SESSIONINFO		sessionInfo;
	uint32				statusCode;
	struct {
		unsigned int	data_compressed : 1;
		unsigned int	data_compress_method : 7;
		unsigned int	padding : 24;
	} flags;
	char				reserved[ 60 ];
	char				key[ 64 ];
	char				digest[ 64 ];
	uint32				data_len;
	char				data[1];
} SIMD_DATABLOCK;
typedef struct SIMD_STRINGS {
	struct {
		unsigned int	data_compressed : 1;
		unsigned int	data_compress_method : 7;
		unsigned int	padding : 24;
	} flags;
	char				key[ 64 ];
} SIMD_STRINGS, SIMD_CS_STRINGS, SIMD_SC_STRINGS;
PBytePack & operator<< (PBytePack & pack, SIMD_SC_STRINGS & sr );
PBytePack & operator>> (PBytePack & pack, SIMD_SC_STRINGS & sr );

// invite a client to call one or more target

#define TEXT_MESSAGE_MAX_LENGTH		1024

typedef struct SIMD_SX_UPDATESWITCHINFO {
	uint32				statusCode;			// 4

	SIM_SESSIONINFO		sessionInfo;		// 8, for auth

	uint32				id;					// 4
	SWITCHINFO			info;				// 32

	SIM_CHANNEL			queryChannel;		// 20

	char				reserved[ 16 ];
} PACKED SIMD_SX_UPDATESWITCHINFO, SIMD_XS_LOGINREPLACED, SIMD_SX_QLOCATE, SIMD_XS_ALOCATE;
typedef struct SIMD_SX_UPDATESWITCHINFOEX {
	SIM_SESSIONINFO		sessionInfo;		// 8, for auth

	uint32				id;					// 4
	uint32				senderuseridstate;					// 4
	uint32				domainid;
	uint32        buddysonlinecount;
	uint32        opt;//0:login,1:logout,0xfff:changed state
	char				*pData;
	//SWITCHINFO			info;				// 32

	//SIM_CHANNEL			queryChannel;		// 20

	
} PACKED SIMD_SX_UPDATESWITCHINFOEX;
typedef struct SIMD_XS_BUDDYSTATUSNOTIFYLIST {
	SIM_SESSIONINFO		sessionInfo;		// 8, for auth

	uint32				senderuserid;					// 4
	uint32				senderuseridstate;					// 4
	char				  senderuseridstateString[96];					// 4
	uint32				fromdomainid;
	uint32        buddysonlinecount;
	//uint32        opt;//0:login,1:logout,0xfff:changed state
	char				*pData;
	//SWITCHINFO			info;				// 32

	//SIM_CHANNEL			queryChannel;		// 20

	
} PACKED SIMD_XS_BUDDYSTATUSNOTIFYLIST;

typedef struct SIMD_CS_GETIDTREES {
	SIM_SESSIONINFO			sessionInfo;
	uint32					nId;
	char					reserved[ 52 ];
} PACKED SIMD_CS_GETIDTREES;

// SIMD_SC_GETIDTREE { statusCode, nId, pack >> trees }

typedef struct SIMD_SERVER_INFO {
	time_t				timestamp;				// 8

	uint32				dwIp;

	uint16				nAdminPort;
	uint16				nTcpPort;
	uint16				nUdpPortMin;
	uint16				nUdpPortMax;

	uint32				dwCookie;

	time_t				tUpTime;				// 8, already convert to local host time

	uint32				nServerType;			// 4
	uint32				nServerStatus;			// 4

	uint32				nMaxUsers;				// 4
	uint32				nOnlineUsers;			// 4
	uint32				nProxyServers;			// 4
	uint32				nMCUServers;			// 4

	char				reserved[ 60 ];			// reserved for extension
} PACKED SIMD_SERVER_INFO; // 128 byte

typedef struct SIMD_DOMAIN_INFO {
	uint32					dwId;								// 4 bytes, as prefix or domain id for VFON id
	char					strAlias[ USERDATA_ADDRESS_SIZE ];	// 64 bytes
	char					strPassword[ USERDATA_NAME_SIZE ];	// 32 bytes, only keep local & child password for uplink

	uint32					dwParentId;							// 4

	char					reserved[ 24 ];
} PACKED SIMD_DOMAIN_INFO;	// 128
typedef struct SIMD_CC_YYMEETING_PACKET {
  uint32        uid;
  uint32        nCallType:8;// em_calltype
  uint32        reserved:24;
	char					strdata[ 1 ];	// 1 bytes
} PACKED SIMD_CC_YYMEETING_PACKET;	// 

typedef struct SIMD_SS_SERVER_UPDATE {
	uint32				statusCode;		// 4

	SIMD_SERVER_INFO	server;			// 128

	SIM_SESSIONINFO		sessionInfo;	// 8, server ip & cookie
	MS_TIME				msNow;
	char				strPassword[ USERDATA_NAME_SIZE ];	// 32 bytes, for security checking

	union {
		struct {
			unsigned int	uplink : 1;
			unsigned int	downlink : 1;
			unsigned int	broadcast : 1;
			unsigned int	padding : 29;
		};
		unsigned int		value;
	} PACKED flags;

	char				reserved[ 72 ];			
} PACKED SIMD_SS_SERVER_UPDATE; // 256

typedef struct SIMD_SS_DOMAIN_UPDATE {		
	uint32				statusCode;		// 4

	SIMD_DOMAIN_INFO	domain;	// 128
	SIMD_SERVER_INFO	server;	// 128

	SIM_SESSIONINFO		sessionInfo;	// 8, server ip & cookie
	MS_TIME				msNow;			// 8
	char				strPassword[ USERDATA_NAME_SIZE ];	// 32 bytes, for security checking

	union {
		struct {
			unsigned int	uplink : 1;
			unsigned int	downlink : 1;
			unsigned int	broadcast : 1;
			unsigned int	padding : 29;
		};
		unsigned int		value;
	} PACKED flags;

	char				reserved[ 200 ];
} PACKED SIMD_SS_DOMAIN_UPDATE; // 512

/* 
SIMD_SC_INVITEVISIT: {
	const char * lpszString; 
}; 
the string is in the format of "<URL>\r\n<URL>\r\n...", 
<URL> can be an <vf:ID>, or <tel:phone number>, or any other URL.
*/

typedef struct SIMD_ACK {
	uint16 ackid;
} PACKED SIMD_ACK;
enum em_MCC_Command{
  em_MCC_Add_PPT_PAGE =0/* mcu tell it to server,or server tell it to client */, em_MCC_UserEntered_room= 1,em_MCC_Remove_Room=2, em_MCC_Add_participator=3,
};
typedef struct SIMD_SC_MCC_CMD {
	SIM_SESSIONINFO		sessionInfo;
	uint32				roomid;				//  
	uint32				eventId;				// 0:httpurl
  uint16       state;
  uint16       nHttplistenPortForMCU;
	uint32				nActinuID;				// 0:httpurl
 	char				reserved[ 60 ];
  char         data[1];
} PACKED SIMD_SC_MCC_CMD, SIMD_CS_MCC_CMD ;

typedef struct SIMD_CS_SENDFILE {
	SIM_SESSIONINFO			sessionInfo;
	uint32 nTo;//receive person
	char oringnalfilename[64];//文件名
	uint32 filetotalsize;//创建的时间
	char pages;//页码
	uint32 sendtime;//创建的时间
	short seq;//序号
	short len;
	char data[1];
} PACKED SIMD_CS_SENDFILE, SIMD_CC_SENDFILE;
typedef struct SIMD_SC_SENDFILE {
	SIM_SESSIONINFO			sessionInfo;
	uint32 nTo;//receive person
	char oringnalfilename[64];//文件名
	uint32 filetotalsize;//创建的时间
	char pages;//页码
	uint32 sendtime;//创建的时间
	short seq;//序号
	uint32 status;
} PACKED SIMD_SC_SENDFILE;
typedef struct SIMD_SC_DOWNLOADFILE_NOTIFY {
	SIM_SESSIONINFO			sessionInfo;
	uint32 nFrom;//who send 
	char oringnalfilename[64];//文件名
	char pages;//页码
	uint32 filetotalsize;//创建的时间
	uint32 sendtime;//创建的时间
} PACKED SIMD_SC_DOWNLOADFILE_NOTIFY;
typedef struct SIMD_CS_MEETCREATE {
	SIM_SESSIONINFO			sessionInfo;
	BBQ_MeetingInfo     info;
} PACKED SIMD_CS_MEETCREATE;
typedef struct SIMD_SC_MEETCREATE {
	SIM_SESSIONINFO			sessionInfo;
	char				strMeetingId[ 64 ];
	uint32      nRoomID;
	uint32 state;
	//BBQ_MeetingInfo     info;
} PACKED SIMD_SC_MEETCREATE;
typedef struct SIMD_CS_MEETCTRL {
	SIM_SESSIONINFO			sessionInfo;
  int cmd;
	char     info[128];
} PACKED SIMD_CS_MEETCTRL, SIMD_SC_MEETCTRL;
typedef struct SIMD_CS_MEETUPDATE {
	SIM_SESSIONINFO			sessionInfo;
	uint32				nRoomID;
	uint32      nActiveMember;
	uint32      nDeActiveMember;
 
} PACKED SIMD_CS_MEETUPDATE;
typedef struct SIMD_SC_FREEGROUP_NOTIFY {
	SIM_SESSIONINFO			sessionInfo;
	uint32				command ;
	uint32      nGroupID;
	uint32 state;
	char			reservl[ 32 ];
} PACKED SIMD_SC_FREEGROUP_NOTIFY;

typedef struct SFCallRecord
{
  char m_szFromID[32];
  //char m_szFromAlias[32];
  char m_szToID[32];//call对方用户名
  char m_szToAlias[32];

  //char m_szName[256];    //call对方用户名
  char m_szAddress[48]; //server ip
  unsigned m_tBeginTime ;//通话启始时间
  unsigned m_tduration;  //通话时间长度
  int m_calledtype;// 参考 em_CalledType
  int m_emCalltype;//call type
  char m_szDialogID[128];
} SFCallRecord;

#pragma pack()

#endif // _BBQ_ID_MSG_H
