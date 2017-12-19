
#ifndef _BBQ_USER_INFO_H
#define _BBQ_USER_INFO_H

#include "sfidmsg.h"

#include <time.h>

#include <list>
#include <map>

#pragma pack(1)

// added on June 22, 2004,
// for multi-domain or multi-server vfon system.
typedef union VFONID {
	struct {
		uint32				userid;		// low 32 bits
		uint32				serverid;	// high 32 bits
	};
	uint64					value;
} PACKED VFONID;

// ------------------- User Info, by Group -----------------

#define			USERDATA_NAME_SIZE			32
#define			USERDATA_PHONE_SIZE			16
#define			USERDATA_ADDRESS_SIZE		64
#define			USERDATA_COMMENT_SIZE		256
#define			USERDATA_LIST_SIZE			1000
#define			USERDATA_ALIAS_SIZE_EXT		128

typedef union BBQACLFlag {
	struct {
		// low bit
		unsigned int					ignore_rule_and_use_as_it_is : 1;	// ignore predefined general rules and use as it is loaded from DB

		unsigned int					max_peers_per_conf : 4;	// x+1 = participant per conference, 1~15 -> 2~16
		unsigned int					run_as_adware : 1;		// run with advertisement
		unsigned int					allow_audio : 1;
		unsigned int					allow_text_chat : 1;

		unsigned int					no_login : 1;				// to login
		unsigned int					no_callout : 1;				// to call out
		unsigned int					no_callin : 1;				// to be called
		unsigned int					no_listonline : 1;			// to list online user
		unsigned int					no_search : 1;				// to search user
		unsigned int					allow_proxy : 1;			// allow use proxy if behind strict firewall
		unsigned int					no_private_buddylist : 1;	// no private buddy list
		unsigned int					reserved_server_control : 1;	

		unsigned int					no_minimcu : 1;		// mini mcu feature
		unsigned int					no_stdh323 : 1;		// make standard H.323 call
		unsigned int					allow_h323gk : 1;	// register to gatekeeper
		unsigned int					allow_video : 1;	// allow user to do video phone
		unsigned int					temp_user : 1;		// temp user cannot make call by hand
		unsigned int					max_video_size : 2; // 0: ignore, 1: QCIF; 2: CIF; 3: VGA
		unsigned int					allow_sms : 1;		// 

		unsigned int					pro_sendfile : 1;		// send file
		unsigned int					pro_whiteboard : 1;		// white board
		unsigned int					pro_appsharing : 1;		// application sharing

		unsigned int					allow_using_beta : 1;	// allow using beta version of client
		unsigned int					max_peervideos_per_conf : 4;

		// high bit
	};
	uint32								value;
} PACKED BBQACLFlag;	// 32 bit, == uint32
enum emServicePolicy
{
	emServicePolicyVfon,
	emServicePolicyVmeet,
};
typedef union BBQACLFlagExt {
	struct {
		unsigned short					max_bandwidth_per_channel;	// max bandwdith bandwidth per channel, in Kbps, unlimited if zero
		unsigned int					no_mcu : 1;
		unsigned int					no_whiteboard_adv : 1;
		unsigned int					no_record : 1;
		unsigned int					bind_with_email : 1;
		unsigned int					bind_with_mobilephone : 1;
		unsigned int					iptv_broker_server : 1;
		unsigned int					iptv_streaming_server : 1;
		unsigned int					iptv_media_server : 1;
		unsigned int					allow_hd_video_D1 : 1;
		unsigned int					allow_hd_video_720P : 1;
		unsigned int					allow_hd_video_1080P : 1;
		unsigned int					padding : 5;
		char									service_policy;				// refer the emServicePolicy 

		char							reserved[ 7 ];				// reserved for future
	};
	uint32								value[ 3 ];
} PACKED BBQACLFlagExt;

#define		BBQACLFLAG_MAXPEERS(N)		((0x0f & (N)) << 1)
#define		BBQACLFLAG_ADWARE			0x00000020
#define		BBQACLFLAG_AUDIO			0x00000040
#define		BBQACLFLAG_TEXTCHAT			0x00000080

#define		BBQACLFLAG_NOLOGIN			0x00000100
#define		BBQACLFLAG_NOCALLOUT		0x00000200
#define		BBQACLFLAG_NOCALLIN			0x00000400
#define		BBQACLFLAG_NOLISTONLINE		0x00000800
#define		BBQACLFLAG_NOSEARCH			0x00001000
#define		BBQACLFLAG_ALLOWVPROXY		0x00002000

#define		BBQACLFLAG_NOMINIMCU		0x00010000
#define		BBQACLFLAG_NOSTD323			0x00020000
#define		BBQACLFLAG_ALLOWGK			0x00040000
#define		BBQACLFLAG_VIDEO			0x00080000
#define		BBQACLFLAG_TEMPUSER			0x00100000
#define		BBQACLFLAG_MAXVIDEO(N)		((0x3 & (N)) << 21)
#define		BBQACLFLAG_SMS				0x00800000

#define		BBQACLFLAG_FILE				0x01000000
#define		BBQACLFLAG_WHITEBOARD		0x02000000
#define		BBQACLFLAG_APPSHARING		0x04000000
#define		BBQACLFLAG_ALLOWBETA		0x08000000

#define		BBQACLFLAG_DEFAULT			0x0
#define		BBQACLFLAG_SET(var,x)		( (var).value = (x) )
#define		BBQACLFLAG_VALUE(var)		( (var).value )

// technical parameter, which will help auto performance adjustment
typedef struct BBQUserTechParamInfo {
	uint32					interface_ip;		// network interface ip address
	uint32					version;			// version of client software
	unsigned int			bandwidth;			// Kbps
	unsigned int			clienttype : 8;			// refer the BBQVfonUserActionLogclienttype
	unsigned int			client_private_param : 6;	// client private parameter			
	unsigned int			proto_h323 : 1;		// support h.323 or not
	unsigned int			proto_sip : 1;		// support sip or not
	unsigned int			video : 1;			// 0: no, 1: yes
	unsigned int			audio : 1;			// 0: no, 1: yes
	unsigned int			wan : 1;			// 0: lan, 1: wan
	unsigned int			firewall: 2;		// 0: none, 1: weak, 2: medium, 3: strong
	unsigned int			os: 3;				// 0: windows, 1: linux, 2: mac, 3: other
	unsigned int			osver : 8;			// os version
} PACKED BBQUserTechParamInfo;	// 16 bytes

typedef struct BBQUserBasicInfo {
	uint8		image;
	char		alias[ USERDATA_NAME_SIZE ];
	uint8		age;							// convert to date while put to database
	uint8		gender;							// 1: male, 2: female, 0 for unknown
	char		country[ USERDATA_NAME_SIZE ];
	char		state[ USERDATA_NAME_SIZE ];
	char		city[ USERDATA_NAME_SIZE ];
} PACKED BBQUserBasicInfo;

typedef struct bBQUserContactInfo {
	char		email[ USERDATA_ADDRESS_SIZE ];
	char		address[ USERDATA_ADDRESS_SIZE ];
	char		postcode[ USERDATA_PHONE_SIZE ];
	char		phone[ USERDATA_PHONE_SIZE ];
	char		handphone[ USERDATA_PHONE_SIZE ];
	uint8		privacy;			// 0: all, 1: friends only, 2: none
} PACKED BBQUserContactInfo;

typedef struct BBQUserDetailedInfo {
	char		realname[ USERDATA_NAME_SIZE ];
	char		school[ USERDATA_NAME_SIZE ];
	char		occupation[ USERDATA_NAME_SIZE ];
	uint8		attribute;			// 1~12 for rat to pig, 0 for unknown
	uint8		bloodtype;			// 1~4 for O, A, B, AB, 5~6 for other, 0 for unknown
	uint8		constellation;		// 1~12 for stella, 0 for unknown
	char		webpage[ USERDATA_ADDRESS_SIZE ];
	char		comment[ USERDATA_COMMENT_SIZE ];
} PACKED BBQUserDetailedInfo;

typedef struct BBQUserSecurityInfo {
	char		oldpass[ USERDATA_NAME_SIZE ];
	char		newpass[ USERDATA_NAME_SIZE ];
	union {
		struct {
			unsigned int			modify : 1;						// 0: do not change password, 1: change password
			unsigned int			allow_addcontact : 2;			// 0: allow anyone, 1: must authorize, 2: deny
			unsigned int			allow_call : 2;					// 0: allow anyone, 1: friend only, 2: deny
			unsigned int			hide_in_onlinelist : 1;			// 0: no, show me, 1: yes, hide me
			unsigned int			allow_leavetextmsg : 2;			// 0: allow anyone, 1: friend only, 2: deny
			unsigned int			allow_leavevideomsg : 2;		// 0: allow anyone, 1: friend only, 2: deny
			unsigned int			modify_servicetime : 1;			// 0: do not change, 1: changed
			unsigned int			only_use_safe_proxy : 1;		// 0: use any proxy possible, 1: use safe proxy only
			unsigned int			padding : 20;
		}; 
		uint32	flags;
	};
} PACKED BBQUserSecurityInfo;

enum BBQAllowAddContact {
	ALLOWADD_BYANYONE = 0,
	ALLOWADD_BYAUTH = 1,
	ALLOWADD_BYNONE = 2,
};

enum BBQAllowCall {
	ALLOWCALL_BYANYONE = 0, //anyone
	ALLOWCALL_BYFRIEND = 1, //friend
	ALLOWCALL_BYNONE = 2,   //forbidden anyone
};

typedef struct BBQUserInfo {
	BBQUserBasicInfo		basic;
	BBQUserContactInfo		contact;
	BBQUserDetailedInfo		detail;
	BBQUserSecurityInfo		security;
} PACKED BBQUserInfo; // 796 bytes

//------------------- Client Personal Data --------------------------
typedef struct BBQDataBlock {
	uint32		data_len;
	char *		data;
} BBQDataBlock;

enum BBQUserServiceType {

	SERVICE_SOFTWAREBOUNDLE,		// use serial:cdkey to login

	SERVICE_FIXEDPCTRIAL,			// use hardware id to register or login

	SERVICE_PHONETRIAL,				// use id:password to login
	SERVICE_PREPAY,
	SERVICE_PAY,

	// special client, VFON system component, 
	// maybe use id:password to login, maybe password is not required

	SERVICE_SUBVSERVER = 0x10,		// password is always required

	SERVICE_MCU = 0x20,				// password is not required, unless communication security is considered

	SERVICE_BBQPROXY = 0x40,		// password is not required, unless communication security is considered
};
enum BBQVfonUserActionLogActionType {
	ActionTypeUnknown,
	ActionTypeLogin,
	ActionTypeLogout,
	ActionTypecall,
	ActionTypecallokay,
	ActionTypecallfail,
	ActionTypesendtext,
	ActionTypesendfile,
	ActionTypeApp,
	ActionTypePresentation,
	ActionTypeMAX=1000,
};
enum BBQVfonUserActionLogclienttype {
	emClientTypeNormal,
	emClientTypeWeb,
};

#define	SERVICE_FIXEDPCTRIAL_TIME		30		// 30 days

typedef struct VfonIdBindInfo {
	char		bindType[ USERDATA_NAME_SIZE ];			// if not empty, then use bind info to login
	char		bindId[ USERDATA_ADDRESS_SIZE ];
	char		bindPass[ USERDATA_NAME_SIZE ];
} PACKED VfonIdBindInfo;

typedef struct BBQUserServiceInfo {
	uint32			type;
	uint32			product_serial;							// optional, for hardware boundle copies
	char			product_key[ USERDATA_ADDRESS_SIZE ];		
	// for fixed PC trial, this is hardware info, such as Hard Disk Id, or Net Adaptor MAC address;
	// for boundled service, this is CD key;
	// for others, this is super key, to protect regular password;

	time_t			start_time;								// period for free usage
	time_t			expire_time;

	uint32			provider;
	char			provider_name[ USERDATA_ADDRESS_SIZE ];

	time_t			login_time;
	uint32			ip;

	uint32			statusCode;			// 4

	BBQACLFlag		flags;				// 4, flags

	uint32			serverID;			// 4
	IPSOCKADDR		serverAddr;			// 6

	// newly added on 2004.9.22
	char			hardwareID[ USERDATA_NAME_SIZE ];		// bind id to a fixed PC

	VfonIdBindInfo	bindInfo;
	//char			bindType[ USERDATA_NAME_SIZE ];			// bind type name, "PCCW_CVG", "PCCW_BB", "PCCW_IVC", "PCCW_TEMP", etc.
	//char			bindKey[ USERDATA_ADDRESS_SIZE ];		// bind value, "12345", "who@user.com", etc.
	//char			bindPass[ USERDATA_NAME_SIZE ];			// bind password, or temp serial number.

	// optional, 
	// if there is a standalone billing system, use as cache only.
	int				points;									// money, 
	int				billing_status;							// billing status

	uint32			company_id;

	struct {
		unsigned int	is_group : 1;
		unsigned int	is_closed_group : 1;
		unsigned int	update_group : 1;
		unsigned int	is_mcu : 1;
		unsigned int	padding : 28;
	};

	BBQACLFlagExt	extFlags;			// 12

	char			reserved[ 40 ];

} PACKED BBQUserServiceInfo;

// id => status code
typedef std::map<uint32, uint32>				UserStatusMap;

typedef struct ONLINE_USER {
	uint32			id;
	union {
		uint32		status;
		struct {
			unsigned int act_status : 8;
			unsigned int set_status : 8;
			unsigned int padding : 16;
		};
	};
} ONLINE_USER;
typedef struct ONLINE_USER_EXT {
	ONLINE_USER			user;
	char     strStatusString[256];
} ONLINE_USER_EXT;

enum {
	CALLFWD_NONE			= 0,
	CALLFWD_ANY				= 1,
	CALLFWD_BUSY			= 2,
	CALLFWD_NOANSWER		= 3,
	CALLFWD_NOTAVAILABLE	= 4,
};

#define		MAX_CALLFWD_COUNT		8
#define		CALLFWD_COUNT			4
#define		MAX_VOIP_COUNT			8
#define		XMLEXT_SIZE				1024

typedef struct BBQCallForwardRecord
{
	union {
		struct {
			unsigned int	fwd_condition : 16;	
			unsigned int	enabled : 1;
			unsigned int	pick_subid : 1;		// pick an idle sub id, useful working as call center mode, will return id in vfonid
			unsigned int	padding : 14;
		};
		uint32		value;					// 4 bytes, for saving to db
	};
	VFONID			vfonid;					// if userid == 0, then use following url
	char			fwd_url[ 128 ];			// http:, vfon:, h323:, sip:, phone:, etc.
	char			reserved[ 120 ];
} PACKED BBQCallForwardRecord;				// 256 bytes

typedef struct BBQCallForwardInfo {
	uint32					count;
	BBQCallForwardRecord	callfwd[ CALLFWD_COUNT ];
} PACKED BBQCallForwardInfo;

struct BBQUserValidFlags {
	unsigned int		partial_valid : 1;		// set to 1, if use following flags; if 0, all fields are valid

	unsigned int		userInfo : 1;
	unsigned int		service : 1;
	unsigned int		param : 1;
	unsigned int		friends : 1;
	unsigned int		blocks : 1;

	unsigned int		service_all : 1;		// force to write, include ACL, expire time

	unsigned int		padding : 20;

	unsigned int		voip_info : 1;	
	unsigned int		call_forward : 1;

	unsigned int		temp_user : 1;			// true if it is a temp user

	unsigned int		login : 1;				// do something when login
	unsigned int		logout : 1;				// do something when 
} PACKED ;

typedef struct BBQVoIPInfo {
	unsigned int		enabled : 1;
	unsigned int		other_password : 1;		// if non-zero, then should use following password to register
	unsigned int		h323 : 1;				// non-zero for h.323
	unsigned int		sip : 1;				// non-zero for sip
	unsigned int		padding : 28;			// reserved

	int					ttl;
	char				e164[ 64 ];
	char				password[ 64 ];
	char				server_addr[ USERDATA_COMMENT_SIZE ];		// ip[:port]
	char				outbound_addr[ USERDATA_COMMENT_SIZE ];	// ip[:port]
	char				ext_property[ USERDATA_COMMENT_SIZE ];	// other customized string
	char				auth_name[ USERDATA_ADDRESS_SIZE ];	// auth name
} PACKED BBQVoIPInfo;

typedef struct BBQUserFullRecord {
	uint32					uid;

	BBQUserValidFlags		valid_flags;

	BBQUserInfo				userInfo;
	BBQUserServiceInfo		service;
	BBQUserTechParamInfo	param;

	uint32					friends[ USERDATA_LIST_SIZE ];	// zero-terminated
	uint32					blocks[ USERDATA_LIST_SIZE ];	// zero-terminated
	uint32					adding[ USERDATA_LIST_SIZE ];	// zero-terminated
	uint32					waitauth[ USERDATA_LIST_SIZE ];	// zero-terminated

	BBQCallForwardRecord	callfwd[ MAX_CALLFWD_COUNT ];	// call forward record

	BBQVoIPInfo				voipinfo[ MAX_VOIP_COUNT ];		// voip config info
	char					xmlext[ XMLEXT_SIZE ];			// customized string that put into XML profile
	char					userStatusString[ 96 ];				//personal info (UTF8)

	char					AliasExt[ USERDATA_ALIAS_SIZE_EXT ];
	char					reserved[ 32 ];

} PACKED BBQUserFullRecord;

typedef struct BBQVoipIems {
	uint32					count;
	BBQVoIPInfo				items[ MAX_CALLFWD_COUNT ];
} BBQVoipItems;

typedef struct BBQBriefNotifyInfo {
	VFONID					peerId;							// 8
	uint32					onlineStatus;					// 4
	char					alias[ USERDATA_NAME_SIZE ];	// 32
	char					bindId[ USERDATA_ADDRESS_SIZE ];// 64
	BBQUserTechParamInfo	techInfo;						// 16
} PACKED BBQBriefNotifyInfo; // 124

// ------------------- Search condition & result -----------

// for all the items, assign 0 or [0] to ignore
typedef struct BBQSearchCondition {
	// basic
	char						alias[ USERDATA_NAME_SIZE ];		
	char						email[ USERDATA_ADDRESS_SIZE ];		

	// advanced
	char						country[ USERDATA_NAME_SIZE ];		
	char						state[ USERDATA_NAME_SIZE ];		
	char						city[ USERDATA_NAME_SIZE ];			

	uint8						video;		// 0: ignore, & 0x1==1: yes, & 0x1==0: no
	uint8						audio;		// 0: ignore, & 0x1==1: yes, & 0x1==0: no

	char						school[ USERDATA_NAME_SIZE ];
	char						occupation[ USERDATA_NAME_SIZE ];

	uint8						gender;		// 0: ignore, & 0x1==1: male, & 0x1==0: female
	uint8						age_min;	// convert to time_t while query
	uint8						age_max;

} /*PACKED */BBQSearchCondition;

typedef struct BBQSearchResult {
	uint32						uid;
	uint8						image;
	uint8						gender;
	uint8						age;
	uint8						video;
	uint8						audio;
	char						alias[ USERDATA_NAME_SIZE ];
	char						state[ USERDATA_NAME_SIZE ];
	char						city[ USERDATA_NAME_SIZE ];
} /*PACKED */BBQSearchResult, BBQUserBriefRecord;

// ---------- VFON session -----------------------------------------

typedef union VfonSessionFlag {
	struct {
		unsigned int			start : 1;
		unsigned int			isCaller : 1;
		unsigned int			usedVideo : 1;
		unsigned int			usedVmeet :1;
		unsigned int			usedMCU : 1;
		unsigned int			dropOffline : 1;
		unsigned int			errChannelRequesting : 1;
		unsigned int			errTransport : 1;
		unsigned int			padding : 26;
	};
	uint32						value;
} PACKED VfonSessionFlag;

#define		VFONSESSIONFLAG_DEFAULT			0x0
#define		VFONSESSIONFLAG_SET(var,x)		( (var).value = (x) )
#define		VFONSESSIONFLAG_VALUE(var)		( (var).value )

typedef struct VfonSession {
	VFONID				localId, peerId;			// 16
	time_t				beginTime, endTime;			// 8
	VfonSessionFlag		flags;						// 4, isCaller, usedVideo, usedVmeet, usedMCU, dropOffline, etc.
	char				ConferenceId[ 64 ];			// 64
	char				h323TargetId[ 64 ];			// if toId is zero, should use this H.323 target id, example: 8621114@210.22.176.194:1719
	VFONID				hostId;						// 8, host of this conference
	uint32				errorCode;					// 4, error in this session
	char				comment[ USERDATA_NAME_SIZE ]; // 32, maybe bindType
	time_t			lasttime;//4, to keep alive conference session, it set in receive a heartbeat .
	char				CallId[ 64 ];				// 64
	char				reserved[ 20 ];				// 20
} PACKED VfonSession;	// 224 bytes

typedef struct VfonUserActionLog {
	VFONID		idUser;
	uint32		nActionType;		// refer to BBQVfonUserActionLogActionType...
	uint32		nResult;			// 0 for unknown, 1 for okay, 2 for fail, if login same as statusCode
	char		strAlias[ 32 ];		// alias of user, might be phone number for call-in if uid = 0
	char		strAction[ 64 ];	// login, logout, call, callokay, callfail, sendtext, sendfile, etc
	time_t		tBegin;				// begin time or login time or logout time
	uint32		nDuration;			// duration of action, in sec
	char *		lpszTarget;			// id or phone number list, seperated by , 
	char *		lpszContent;		// encoding:text of chat (utf8:xXx), or description of file (name:xXx, size:nnn)
	char *		lpszExtra;			// extra info, alias, action, anything else
	uint32		msg_rand_id;		// for server to filter duplicated msg
	uint32		cookie;		// status ID ,see  SIM_STATUSCODE 
	uint32		errorid;		// login result error ID
	char			login_addr[ USERDATA_ADDRESS_SIZE ];// ip[:port]
	char		clienttype;	// clienttype refer the BBQVfonUserActionLogclienttype
	char		reserved[ 47 ];	// reserved
} VfonUserActionLog; // 256 bytes


typedef struct MCURecord {
	VFONID		vfonId;
	char		alias[ USERDATA_NAME_SIZE ];
	uint16		rooms;
	uint16		persons;
	uint32		bps;
} MCURecord;

typedef struct IPSERVICEADDR {
	uint32				ip;
	uint16				tcpPort;	// zero if not in use
	uint16				udpPort;	// zero if not in use
} PACKED IPSERVICEADDR;

typedef struct SWITCHINFO {
	IPSERVICEADDR		addr;				// 8 bytes
	struct {
		unsigned int	act_status : 8;			// 8 bit, char, user's activity status

		unsigned int	firewall_type : 2;	// 0: public, 1 ~ 3: weak, medium, strong
		unsigned int	is_tcp : 1;

		unsigned int	login : 1;
		unsigned int	logout : 1;
		unsigned int	dropoffline : 1;

		unsigned int	set_status : 8; // user's setting status

		unsigned int	padding : 10;
	};
	SIM_CHANNEL			channel;
	char				reserved[ 4 ];

} PACKED SWITCHINFO, PSWITCHINFO;	// 32 bytes

typedef struct BBQOfflineIMMessage {
	char				conf_id[37];		// Conference ID
	char				sentence_id[37];	// Sentence ID
	uint32				from_vfonid;		// From
	uint32				compressed;		// 0 not compress, 1 compressed
	uint32				to_vfonid;			// To
	int				to_vfonid_sub;			// To
	uint64				send_time;			// timestamp
	int					length;				// Sentence length
	char				data[1];			// Sentence content
} PACKED BBQOfflineIMMessage;


#define BBQOfflineIMMessageNEW(len)		(BBQOfflineIMMessage*) malloc( sizeof(BBQOfflineIMMessage) + ((len>0)?len:1) )
#define BBQOfflineIMMessageDELETE(p)	free( (BBQOfflineIMMessage *) p )

typedef std::list<BBQOfflineIMMessage *> offlineim_list;

typedef struct VfonDomainInfo {
	uint32					dwId;								// 4 bytes, as prefix or domain id for VFON id
	char					strAlias[ USERDATA_ADDRESS_SIZE ];	// 64 bytes
	char					strPassword[ USERDATA_NAME_SIZE ];	// 32 bytes, only keep local & child password for uplink
} VfonDomainInfo;

#define ANTI_PACKET_LOSS

#ifdef ANTI_PACKET_LOSS
// ----------------- Channel Id & Timestamp -------------------
typedef struct UNIQUE_CID {
	uint32		uid;	// user id
	uint32		cid;	// channel id
} PACKED UNIQUE_CID;

struct less_than_unique_cid {
	bool operator()(UNIQUE_CID s1, UNIQUE_CID s2) const
	{
		return memcmp(& s1, & s2, sizeof(UNIQUE_CID)) < 0;
	}
};
#endif

#pragma pack()

#endif // _BBQ_USER_INFO_H

