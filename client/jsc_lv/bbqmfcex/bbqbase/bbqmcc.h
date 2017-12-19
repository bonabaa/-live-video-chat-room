
#ifndef _BBQ_MCC_H_
#define _BBQ_MCC_H_

#include "bbquserinfo.h"
#define D_MAX_MEETING_MEMBER 32

#pragma pack(1)
typedef struct BBQ_MeetingRoomInfo {

} PACKED BBQ_MeetingRoomInfo;
typedef struct BBQ_MeetingUserInfo {
	union {
		struct {
			unsigned int	benterroom : 2;						// will go through MCU
			unsigned int	bAllowSpeak : 1;//if allow speak

			unsigned int	padding : 29;
		} flags;
		uint32				value;
	} PACKED ;
   

} PACKED BBQ_MeetingUserInfo;
typedef struct BBQ_MeetingInfo {
	char				strMeetingId[ 64 ];			// unique session id, format: {room id}_{date time}, for example: 2201_20060101120000

	time_t				tBeginTime;					// meeting start time
	uint32				nDuration;

	union {
		struct {
			unsigned int	bMCU : 1;						// will go through MCU
			unsigned int	bAllowJoin : 1;					// allow new person to attend the meeting
			unsigned int	bPassword : 1;					// has password to protect
			unsigned int	bDismissWhenChairmanQuit : 1;	// dismiss the meeting 
			unsigned int	bFreeStart : 1;					// it is a free start meeting that start immediately
			unsigned int	bForecastSent : 1;				// forecast already sent to client

			unsigned int	bLocalIdList : 1;				// use localIdList to allow max 64 users
			
			unsigned int	bInitMuteAll : 1;				// mute all user when started

			unsigned int	bRecording : 1;					// notify MCU do recording for this meeting
			unsigned int	bProtectRecording : 1;			// do password protect for recording

			unsigned int	bAESTraffic : 2;				// do encryption for tranfering, 0: plain, 1: aes128, 2:aes256

			unsigned int	padding : 20;
		};
		uint32				value;
	} PACKED flags;


	uint32				nCompanyId;

	uint32				nRoomId;					// default meeting room for this meeting
	uint32				nMcuId;						// default MCU for this meeting

	char				strPassword[ 32 ];			// md5

	// predefined attendee, first one is the chairman
	uint32				nIdCount;
	//union {
	//	VFONID				idList[ 32 ];					// 8 * 32 = 256 byte, maximal 32 users ?
		uint32				localIdList[ D_MAX_MEETING_MEMBER ];				// 4 * 64 = 256 byte, maximal 64 users
	//};
  BBQ_MeetingUserInfo  localIdListInfo[D_MAX_MEETING_MEMBER];
	char				strMeetingName[ 256 ];
	char				strMeetingDes[ 512 ];

	unsigned short		max_bandwidth_per_channel;	// in Kbps

	union {
		struct {
			unsigned char	max_video_size : 2;				// 0: ignore, 1: QCIF; 2: CIF; 3: VGA
			unsigned char	max_no_video : 1;
			unsigned char	default_video_size : 2;
			unsigned char	default_no_video : 1;
			unsigned char	padding1 : 2;
		};
		unsigned char	video_param_value;
	}PACKED ;

	uint8			max_persons_in_conf;		// maxinum persons allow in this conference, if 0 then no limit (hard limit is 64)

	uint8			status;						// 0: MCC_INIT, 1: MCC_BEGIN, 2: MCC_END, 3: MCC_FAIL, 4, MCC_INITEND... 

	uint8			max_talker_in_conf;

	uint32			timestamp;					// DWORD time_t, a value that always inc for receiver to filter duplicated msg

	char			aesKey[ 32 ];				// aes key for encrypt traffic
	char			aesIv[ 16 ];				// iv for AES
    uint32    tmLeft;                          //time left for 'this->tBeginTime'
	uint8			max_video_in_conf;         //video of sum
	uint32		max_persons_in_conf_ex;    //extern max_persons_in_conf
	union {
		struct {
			unsigned char	record_chairman_only : 1;				// 0: ignore, 1: QCIF; 2: CIF; 3: VGA
			unsigned char	padding2 : 7;
		};
		unsigned char	record_flag;
	}PACKED ;

	// more optional properties of a meeting
	int command;
	char				reserved[ 12 ];

} PACKED BBQ_MeetingInfo;

#define			MCC_INVALID		0xff
#define			MCC_INIT		0
#define			MCC_BEGIN		1
#define			MCC_END			2
#define			MCC_FAIL		3
#define			MCC_INITEND		4
#define			MCC_CHANGE		5

// add unlimited number and user privilege

typedef struct BBQMeetingUserInfo {
	VFONID				user;
	uint32				role_id;
} BBQMeetingUserInfo;

typedef struct BBQPeerMeetingInfo {
	uint32	nMcuId;
	char		strMeetingId[ 64 ];
}BBQPeerMeetingInfo;
using std::list;
typedef list<BBQPeerMeetingInfo> BBQPeerMeetingInfoList;

typedef struct BBQMeetingRoleInfo {
	uint32				role_id;
	union {
		uint64			role_priv;
		struct {
			uint32		priv_basic;			// low 32 bits
			uint32		priv_ext;			// high 32 bits
		};
	};
} BBQMeeetingRoleInfo;

// user privilege in conference

//enum ConferenceRightType
//{
//	// Manager
//	ENDUEROLE_RIGHT				= 0x01000000,
//	INVITE_RIGHT				= 0x02000000,
//	KICKOUT_RIGHT				= 0x04000000,
//	EXTEND_RIGHT				= 0x08000000,
//	EDUERIGHT_RIGHT				= 0x10000000,
////	WITHDRAWRIGHT_RIGHT			= 0x20000000,
//
//	// AV
//	SENDAUDIO_RIGHT				= 0x00000001, 
//	SENDVIDEO_RIGHT				= 0x00000002,
//	RECEIVEAUDIO_RIGHT			= 0x00000004, 
//	RECEIVEVIDEO_RIGHT			= 0x00000008,
//	RECORD_RIGHT				= 0x00000100,
//
//	// Data
//	DATA_RIGHT					= 0x00000200,
//
//	OPENWB_RIGHT				= 0x00000400,
//	PRESENTATION_RIGHT			= 0x00000800,
//	VIEWWB_RIGHT				= 0x00001000,
//	DRAWWB_RIGHT				= 0x00002000,
//	PAGEWB_RIGHT				= 0x00004000,
//
//	OPENSHARE_RIGHT				= 0x00008000,
//	VIEWSHARE_RIGHT				= 0x00010000,
//	CONTROLSHARE_RIGHT			= 0x00020000,
//
//	SENDFILE_RIGHT				= 0x00040000,
//};


typedef struct BBQ_MeetingInfo_Ext {
	uint32					size; // actual size of this structure, should be initialized when malloc the buffer
	// = sizeof(this) 
	// + sizeof(BBQMeetingRoleInfo) * nNumberOfRoles
	// + sizeof(BBQMeetingUserInfo) * nNumberOfUsers
	// + sizeof(BBQPeerMeetingInfo) * nNumberOfPeerMeetings
	// + sizeof(BBQPeerMeetingInfo) * nNumberOfOtherMeetings

	BBQ_MeetingInfo			info;

	// note: 
	// the following pointers should always point to inside of the buffer
	// after memcpy or network transferring, the pointers should be fixed immediately
	//
	uint32					nNumberOfRoles;
	BBQMeetingRoleInfo *	pRoles;				// = (BBQMeetingRoleInfo *) & data[0]

	uint32					nNumberOfUsers;
	BBQMeetingUserInfo *	pUsers;				// = (BBQMeetingUserInfo *) & data[ sizeof(BBQMeetingRoleInfo) * nNumberOfRoles ]

	uint32					nNumberOfPeerMeetings;
	BBQPeerMeetingInfo					 *	pPeerMeetings;				// = (BBQPeerMeetingInfo *) & data[ sizeof(BBQMeetingRoleInfo) * nNumberOfRoles + sizeof(BBQMeetingUserInfo) * nNumberOfUsers ]

	uint32					nNumberOfOtherMeetings;
	BBQPeerMeetingInfo					 *	pOtherMeetings;				// = (BBQPeerMeetingInfo *) & data[ sizeof(BBQMeetingRoleInfo) * nNumberOfRoles + sizeof(BBQMeetingUserInfo) * nNumberOfUsers + sizeof(BBQPeerMeetingInfo) * nNumberOfPeerMeetings]

	//char					reserved[ 32 ];		// reserved for other possible data
	char					reserved[ 16 ];		// reserved for other possible data

	char					data[1];

} PACKED BBQ_MeetingInfo_Ext;

inline BBQ_MeetingInfo_Ext * BBQ_MeetingInfo_Ext_NEW( int nRoles, int nUsers, int nPeerMeetings, int nOtherMeetings )
{
	int n = sizeof(BBQ_MeetingInfo_Ext) + sizeof(BBQMeetingRoleInfo) * nRoles + sizeof(BBQMeetingUserInfo) * nUsers + sizeof(BBQPeerMeetingInfo) * nPeerMeetings + sizeof(BBQPeerMeetingInfo) * nOtherMeetings;

	BBQ_MeetingInfo_Ext * p = (BBQ_MeetingInfo_Ext *) malloc( n );
	if( p ) {
		memset( p, 0, n );
		p->size = n;
		p->nNumberOfRoles = nRoles;
		p->nNumberOfUsers = nUsers;
		p->nNumberOfPeerMeetings = nPeerMeetings;
		p->nNumberOfOtherMeetings = nOtherMeetings;
		p->pRoles = (BBQMeetingRoleInfo *) & p->data[0];
		p->pUsers = (BBQMeetingUserInfo *) & p->data[ sizeof(BBQMeetingRoleInfo) * nRoles ];
		p->pPeerMeetings = (BBQPeerMeetingInfo *) & p->data[ sizeof(BBQMeetingRoleInfo) * nRoles + sizeof(BBQMeetingUserInfo) * nUsers];
		p->pOtherMeetings = (BBQPeerMeetingInfo *) & p->data[ sizeof(BBQMeetingRoleInfo) * nRoles + sizeof(BBQMeetingUserInfo) * nUsers + sizeof(BBQPeerMeetingInfo) * nPeerMeetings];
	}

	return p;
}

inline void BBQ_MeetingInfo_Ext_DELETE( BBQ_MeetingInfo_Ext * p )
{
	if( p ) {
		free( p );
		p = NULL;
	}
}

inline bool BBQ_MeetingInfo_Ext_FIX( BBQ_MeetingInfo_Ext * p )
{
	uint32 n = sizeof( * p );
	if( p->size < n ) return false;

	p->pRoles = (BBQMeetingRoleInfo *) & p->data[0];
	n += sizeof(BBQMeetingRoleInfo) * p->nNumberOfRoles;
	if( p->size < n ) return false;

	p->pUsers = (BBQMeetingUserInfo *) & p->data[ sizeof(BBQMeetingRoleInfo) * p->nNumberOfRoles ];
	n += sizeof(BBQMeetingUserInfo) * p->nNumberOfUsers;
	if( p->size < n ) return false;

	p->pPeerMeetings = (BBQPeerMeetingInfo *) & p->data[ sizeof(BBQMeetingRoleInfo) * p->nNumberOfRoles + sizeof(BBQMeetingUserInfo) * p->nNumberOfUsers];
	n += sizeof(BBQPeerMeetingInfo) * p->nNumberOfPeerMeetings;
	if( p->size < n ) return false;

	p->pOtherMeetings = (BBQPeerMeetingInfo *) & p->data[ sizeof(BBQMeetingRoleInfo) * p->nNumberOfRoles + sizeof(BBQMeetingUserInfo) * p->nNumberOfUsers + sizeof(BBQPeerMeetingInfo) * p->nNumberOfPeerMeetings];
	n += sizeof(BBQPeerMeetingInfo) * p->nNumberOfOtherMeetings;
	if( p->size < n ) return false;

	return true;
}

#pragma pack()

#endif //  _BBQ_MCC_H_
