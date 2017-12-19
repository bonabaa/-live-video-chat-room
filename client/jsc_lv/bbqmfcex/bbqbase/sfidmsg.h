
/* -----------------------------------------------------------------------
  Softfoundry Videophone Calling ID Message Protocol --> SFIDMSG --> SIM

  UDP Channel Message -> UDM_xxx

  Softfoundry Broad Bandwidth Communication Message Protocol -> BBQ

------------------------------------------------------------------------- 

ChangeLog:

2003.8.4, Init code, SIM_xxx, ver 1.0, by Liming Xie;
2003.9.15, Add protocol for Udp Channel Message, UCM, by Liming Xie;
2003.9.15, Add reliable UDP protocol, RUDP, by Zhang Hongxing;
2003.10.10, Add Virtual TCP/UDP protocol, VTCP/VUDP, by Zhang Hongxing;
2003.11.3, Add user profile, contact list, block list
2003.11.24, Add user defined message, including text, add contact, reqest conversation

------------------------------------------------------------------------- */

#include "os_related.h"

#ifndef _SIM_PROTOCOL_
#define _SIM_PROTOCOL_

#pragma pack(1)

#define	SIM_VERCODE(ver, msgmax)	(((ver) & 0xffff) << 16 | ((msgmax) & 0xffff))
#define SIM_VER(vercode)			((vercode) >> 16)
#define SIM_MSGMAX(vercode)			((vercode) & 0xffff)

// bbq client send heartbeat to keep oline, default is 60 seconds,
// but it might differ in differrent firewall
#define	SIM_HEARTBEAT_INTERVAL		60		 

#define	SIM_PORT		5101		// default SIM port, "Remote Who Is", use both TCP & UDP

#define SIM_MAGIC		0x4E		// "N"		

typedef struct SFIDMSGHEADER {
	char  		magic;					// always SIM_MAGIC
	uint16		sync;					// sync identifier
	uint16		size;					// size of message body
	char  		reserved;					// always SIM_MAGIC
	uint16		id;						// message ID
} PACKED SFIDMSGHEADER, * PSFIDMSGHEADER;		// 8 bytes

// msg IDs used in message header
enum SIM_MSGID {
	SIM_NONE = 0,

	// directory service protocol, be implemented by XLM
	SIM_CS_HELLO,			// say hello, ask my ip:port
	SIM_SC_HELLO,			

	SIM_CS_LOGIN,			// login: provide product serial & reg key, and ID (optional), and ask for a cookie
	SIM_SC_LOGIN,				

	SIM_CS_LOCATE,			// locate status info about others
	SIM_SC_LOCATE,		

	SIM_CS_HEARTBEAT,		// keep online
	SIM_SC_HEARTBEAT,

	SIM_CS_LOGOUT,			// logoff
	SIM_CS_CHANGESTATUS,	// change status, including busy, invisible, normal, etc.
	SIM_SC_LOGINREPLACED,	// login was replaced from another place 

	SIM_CS_TESTFIREWALL,	// test firewall, send to port 1, echo from port 3
	SIM_SC_TESTFIREWALL,

	SIM_CS_SUBSCRIBE,		// subscribe VFON service
	SIM_SC_SUBSCRIBE,

// ------ compartible only for version before 2004.06.07 -------------------
	SIM_CS_OLDVERSIONINFO,		// query client version requirement
	SIM_SC_OLDVERSIONINFO,
// ------ compartible end --------------------------------------------------

	SIM_CS_QUERYSERVICE,		// query service status
	SIM_SC_QUERYSERVICE,

	SIM_CS_VERSIONINFO,		// query client version requirement
	SIM_SC_VERSIONINFO,
	
	SIM_CS_QUERYGKLIST,		// query gatekeeper server list
	SIM_SC_QUERYGKLIST,

	SIM_CS_QUERYPSLIST,		// query proxy server list
	SIM_SC_QUERYPSLIST,

// 登录：
// (1) 必须用 ID、密码、硬件ID 登录；
// (2) 通用结构，可用于各种绑定信息登录，例如香港PCCW的CVG（CVG ID）、BB（email地址）、IVC（手机号码）等；

	SIM_CS_LOGIN_EX,		// login with id, password, hardware id, other mapping alias, etc.
	SIM_SC_LOGIN_EX,

// 查询ID：
// 可以通过查询，输入信息可以是VFON ID或者绑定信息，返回 VFON ID以及位置、防火墙等信息，可以用于呼叫等后续操作；
	SIM_CS_LOCATE_EX,
	SIM_SC_LOCATE_EX,

// 通话记录：
// (1) 通话开始和结束时，客户端报告到服务器，服务器内存中保持通话动态列表，并在通话结束时记录到LOG文件或者数据库中；
// (2) Video、Vmeet等高级功能的使用，在通话过程中的Heartbeat以及通话结束时，报告到服务器，记录时一并记录，用于billing时使用；

	// report that this client begin/end a conference with another user, for server to manage/log conference sessions, response is ACK 
	SIM_CS_CONFERENCE,	

	// server ask client to begin/end a conference with another user, for server to manage/cut conference sessions, response is ACK
	SIM_SC_CONFERENCE,

// 可定义的全局字符串：
// (1) 服务器可以手工设定一些字符串，Name --> String，客户端可以通过一个函数去获取；
// (2) 需要设定的例如：下载软件的网址；修改账号信息的网址以及格式；等等；
// (3) 为了防止客户端读取服务器其他字符串，客户端需要的数据名一律加前缀 CLIENT_xxx ，通讯时仅传递 xxx 部分。

	SIM_CS_LOADSTRING,	// get a pre-defined string from server (or global database)
	SIM_SC_LOADSTRING,	

	SIM_CS_GETBINDINFO,	// Vfon Id <--> PCCW Info
	SIM_SC_GETBINDINFO,

	SIM_CS_PROXYSTATUS,		// query proxy status
	SIM_SC_PROXYSTATUS,

	SIM_CS_SERVERSTATUS,	// query server status
	SIM_SC_SERVERSTATUS,

	SIM_CS_SERVERSWITCH,	// do server switch
	SIM_SC_SERVERSWITCH,

	SIM_SC_BUDDYSTATUSNOTIFY,	// notify msg when buddy status changed

	SIM_CS_GETMCULIST,			// download mcu list from server
	SIM_SC_GETMCULIST,

	SIM_SX_UPDATESWITCHINFO,	// update switch info to cluster center, S -- server, X -- exchange center

	SIM_SX_QSWITCHINFO,			// query cluster center for the switch info
	SIM_XS_ASWITCHINFO,

	SIM_SX_QLOCATE,				// ask for the location of a vfon client
	SIM_XS_ALOCATE,

	SIM_SS_SERVER_UPDATE,			// update local server info, from clustered to center, or to other clustered
	SIM_SS_SERVER_UPDATEECHO,

	SIM_SS_DOMAIN_UPDATE,			// update domain info, usually from child to parent
	SIM_SS_DOMAIN_UPDATEECHO,		// feedback, ok or fail

	SIM_SS_DOMAIN_SYNC,				// request domain to start sync the info of other domains
	SIM_SS_DOMAIN_SYNCDATA,			// domain data (1 or more)

	SIM_SX_BUDDYSTATUSNOTIFY,		// request domain server to notify buddy status 
	SIM_XS_BUDDYSTATUSNOTIFY,		// buddy status notify from domain server to login server

	SIM_CS_LOGUSERACTION,			// write user action log to server DB for auditing purpose
	SIM_SC_LOGUSERACTION,

	SIM_CS_GET_PKI_CERT,			// download server's digital certificate in PEM format
	SIM_SC_GET_PKI_CERT,

	SIM_CS_LOGIN_EX_SECURE,			// login, with data encrypted

	SIM_CS_MCC_INVITE,			// MCU invite client to meeting
	SIM_SC_MCC_INVITE,

	//SIM_CS_KICK,			// kick
	SIM_SC_KICK,		    //kick from server	

	
	// reserved for message terminal layer use.
	SIM_ACK = 100,
	SIM_SEGMENT,

	// protocol to setup UDP/TCP channel between terminals under help of BBQ server, be implemented by XLM
	// _CS_ means Client -> Server,
	// _SC_ means Server -> Client,
	// _CC_ means Client -> Client,
	UCM_V1MIN = 200,

	UCM_CC_FIRSTOUT,				// try out firewall, so that peer can enter

	UCM_CS_Q,						// client <-> server, request/response
	UCM_SC_FWD_Q,
	UCM_CS_A,
	UCM_SC_A,

	UCM_CC_CON,						// udp connect
	UCM_CC_CONCFM,

	UCM_SP_CREATE,					// server -> proxy, ask proxy to create/release a proxy channel for client
	UCM_SP_RELEASE,
	UCM_PS_NOTIFY,					// proxy -> server, notify channel status change

	UCM_SC_PROXYNOTIFY,				// server -> client, notify that proxy channel status changed, UP/DOWN

	UCM_CC_IDQUERY,					// ask id
	UCM_CC_IDCONFIRM,				// reply id

	UCM_CC_Q,				// direct channel request
	UCM_CC_A,				// response of direct channel request

	UCM_CP_CONNECT,
	UCM_PC_CONNECT,

	UCM_V1MAX,

	// reliable UDP protocol, be implemented by ZHX
	SIM_RUDP_DATA = 400,
	SIM_RUDP_ACK,
	SIM_VTCP_DATA,
	SIM_VTCP_ACK,
	SIM_VUDP_DATA,

	// account user data 
	SIM_CS_REGISTERUSER = 600,		// register user's data, ask to create an account
	SIM_SC_REGISTERUSER,

	SIM_CS_ERASEUSER,				// user erase its own account
	SIM_SC_ERASEUSER,

	SIM_CS_UPDATEUSERDATA,			// update user's data
	SIM_SC_UPDATEUSERDATA,

	SIM_CS_UPDATECALLFWDINFO,		// update user's call forward records
	SIM_SC_UPDATECALLFWDINFO,

	SIM_CS_GETCALLFWDINFO,			// get call forward info
	SIM_SC_GETCALLFWDINFO,

	SIM_CS_UPLOADDATABLOCK,			// upload data block and stored in server
	SIM_SC_UPLOADDATABLOCK,

	SIM_CS_DOWNLOADDATABLOCK,		// download data block from server
	SIM_SC_DOWNLOADDATABLOCK,

	// yellow page
	SIM_CS_VIEWUSERDATA = 620,		// view user's data of specified ID
	SIM_SC_VIEWUSERDATA,

	SIM_CS_GETUSERTECHPARAM,		// get user tech param
	SIM_SC_GETUSERTECHPARAM,

	SIM_CS_LISTONLINE,				// list online neighbours, (on the same server)
	SIM_SC_LISTONLINE,

	SIM_CS_SEARCHUSER,				// search user according to specified filter
	SIM_SC_SEARCHUSER,

	SIM_SC_MCC_NOTIFY_EX,			// notify client/mcu a MCC event
	SIM_CS_MCC_NOTIFY_EX,			// report to server that the client has begin/end a meeting

	SIM_CS_LISTONLINE_EX,				// list online neighbours, (on the same server)
	SIM_SC_LISTONLINE_EX,
	// relationship
	SIM_CS_ADDCONTACT = 640,		// add contact to friend list
	SIM_SC_ADDCONTACT,

	SIM_CS_DELETECONTACT,			// delete contact from friend list
	SIM_SC_DELETECONTACT,

	SIM_CS_GETCONTACTLIST,			// download contact list from server
	SIM_SC_GETCONTACTLIST,

	SIM_CS_ADDBLOCK,				// add user id to block list
	SIM_SC_ADDBLOCK,

	SIM_CS_DELETEBLOCK,				// delete user id from block list
	SIM_SC_DELETEBLOCK,

	SIM_CS_GETBLOCKLIST,			// download block list from server
	SIM_SC_GETBLOCKLIST,

	SIM_SC_NOTIFY_CONTACT,			// notify the user to be added or blocked, post and need ACK

	SIM_CS_AUTH_CONTACT,			// the user to be added send auth to server, just need ACK to make sure data received
	SIM_SC_AUTH_CONTACT,			// feedback to the user that adding buddy, post and need ACK

	SIM_CS_GETIDTREES,				// get id trees that contain this user, for CUG purpose
	SIM_SC_GETIDTREES,

	SIM_SC_MCC_NOTIFY,				// notify client/mcu a MCC event
	SIM_CS_MCC_NOTIFY,				// report to server that the client has begin/end a meeting

	SIM_CS_NOTIFY_READY,			// ready to receive notify message

	// the following msg will be forward by server
	SIM_CS_USERMSG = 660,			// send user written message, this msg will be stored in user's box if offline
	SIM_SC_USERMSG,

	SIM_CS_IM,						// send im, stored in db if offline
	SIM_SC_IM_NOTIFY,

	SIM_CS_GET_IM,					// client get im from server
	SIM_SC_GET_IM,

	SIM_CS_DEL_IM,					// request to delete the offline im
	SIM_SC_DEL_IM,

	SIM_SC_GET_MCC,				// notify client/mcu a MCC event
	SIM_CS_GET_MCC,				// report to server that the client has begin/end a meeting

	SIM_SC_IM,

  SIM_SC_INVITEVISIT = 680,		// invite a client to visit/call one or more target
	SIM_SC_BROADCASTTEXT,
	SIM_CS_ENTER_ROOM,					// request to delete the offline im
	SIM_SC_ENTER_ROOM,
	SIM_CS_LEFT_ROOM,					// request to delete the offline im
	SIM_SC_LEFT_ROOM,

	SIM_CS_PINGPROXY,
	SIM_CS_SENDTEXT,
	SIM_CS_STRINGS,		// query client version requirement
	SIM_SC_STRINGS,
	SIM_SC_SELFBUDDYSTATUSNOTIFY,// all online buddy's status string notify to client (including self)
	//add by chen yuan,for compress message
	//对于压缩处理msg的流程如下，客户端发送消息ID末尾含有COMPR到server,server接受到COMPR msg发现数据给client，当发送数据长度大于 1400后压缩该数据然后发送，否则直接发送消息
	SIM_CS_GET_PKI_CERT_COMPR=1000,			// download server's digital certificate in PEM format
	SIM_SC_GET_PKI_CERT_COMPR,
	SIM_CS_GETCONTACTLIST_COMPR,			// download contact list from server
	SIM_SC_GETCONTACTLIST_COMPR,
	SIM_CS_LOADSTRING_COMPR,	// get a pre-defined string from server (or global database)
	SIM_SC_LOADSTRING_COMPR,	
	SIM_CS_DOWNLOADDATABLOCK_COMPR,		// download data block from server
	SIM_SC_DOWNLOADDATABLOCK_COMPR,
	SIM_CS_GETIDTREES_COMPR,				// get id trees that contain this user, for CUG purpose
	SIM_SC_GETIDTREES_COMPR,
	SIM_SX_UPDATESWITCHINFOEX,
	SIM_XS_BUDDYSTATUSNOTIFYLIST,	//notify the msg to all domain link vfon server,when received it ,send it to sender and sendt sender's state to online people.
	SIM_CS_MEETCREATE,
	SIM_SC_MEETCREATE,
  SIM_CS_GETCALLFWDINFO_COMPR,
  SIM_SC_GETCALLFWDINFO_COMPR,
  SIM_CS_GETGATEWAYLIST_COMPR,			// download gateway list from server
	SIM_SC_GETGATEWAYLIST_COMPR,
  SIM_CS_GET_MCC_COMPR,
  SIM_SC_GET_MCC_COMPR,

	SIM_SS_ROUTE_MSG=2000,	// update switch info to cluster center 

	SIM_SS_UPDATEUSERINFO,
	SIM_SS_REQ_LOCATE_USER,
	SIM_SS_RES_LOCATE_USER,	//notify the msg to all domain link vfon server,when received it ,send it to sender and sendt sender's state to online people.
  SIM_CS_LOGOUT_IPTV=3000,
	SIM_CS_HEARTBEAT_IPTV,		// keep online
	SIM_SC_HEARTBEAT_IPTV,

	SIM_SC_MCC_CMD,				// notify bbqsvr a MCC command
	SIM_CS_MCC_CMD,				//  notify bbqsvr a MCC command
  SIM_CS_MEETUPDATE,
  SIM_SC_FREEGROUP_NOTIFY,
  UCM_CM_CONNECT=4000,
  UCM_MC_CONNECT,
  SIM_CC_SIPPACKET,
  SIM_CS_MEETUPDATE_EX,
  SIM_CS_MEETCTRL,
	SIM_SC_MEETCTRL,
	SIM_THISMAX,

	SIM_MAX = 65535,
};
extern const char * SIM_MSGNAME( int msgid );

// msg status codes in response msg returned by server
enum SIM_STATUSCODE {
	SIMS_OK,
	SIMS_BADREQUEST,
	SIMS_NOTFOUND,
	SIMS_DENY,
	SIMS_NOTAVAILABLE,
	SIMS_TIMEOUT,
	SIMS_SERVERERROR,
	SIMS_UNKNOWNERROR,
	SIMS_NOTIMPLEMENTED,
	SIMS_NETWORKERROR,
	SIMS_REFUSE,
	SIMS_FIREWALL,
	SIMS_NORES,
	SIMS_AUTHREQUIRED,
	SIMS_SERVICEEXPIRED,
	SIMS_EXISTS,
	SIMS_COMPRESSERROR,
	SIMS_SERVERBUSYSERROR,
	SIMS_LOGOUT,
};

extern const char * SIM_STATUSCODENAME( int statusCode );

// ============ SIM data structures =====================

// Important Note: all the ip & port transfered are network order, 
// should convert to host order before display.

#define SIM_THIS_VERCODE		SIM_VERCODE(1, SIM_THISMAX)

#define SIM_DATABLOCKSIZEMAX	4088	// 4096 - 8

// =====================================================================================
// this type is only for space to store data, 
// actual data to send should be: sizeof(msgHeader) + sizeof(msgBody)
typedef struct SIM_MESSAGE {
	SFIDMSGHEADER		simHeader;
	char				simData[ SIM_DATABLOCKSIZEMAX ];
} PACKED SIM_BLOCK, SIM_MESSAGE;
#define C_COMPRESS_BZIP2 1 //bzip2,
#define C_COMPR_FLAG_SIZE (sizeof(SIM_MESSAGE_COMPR::simCompress))
typedef struct SIM_MESSAGE_COMPR {
	SFIDMSGHEADER		simHeader;
	struct simCompress{
		unsigned short	data_compressed : 1;
		unsigned short	data_compress_method : 7; //C_COMPRESS_BZIP2:bzip2
		unsigned short	padding : 8;
	} simCompress_flags;//simHeader::size = sizeof (simCompress_flags) + length of simData
	char				simDataCompr[ SIM_DATABLOCKSIZEMAX ];
} PACKED SIM_BLOCK_COMPR, SIM_MESSAGE_COMPR;

#define SIM_MSGINIT(msg, msgid, datasize) \
	(msg).simHeader.magic = SIM_MAGIC; \
	(msg).simHeader.sync = 0; \
	(msg).simHeader.id = msgid; \
	(msg).simHeader.size = datasize; \
	memset( & (msg).simData, 0, datasize ); 

#define SIM_MSGINIT_COMPR(msg, msgid, datasize) \
	(msg).simHeader.magic = SIM_MAGIC; \
	(msg).simHeader.sync = 0; \
	(msg).simHeader.id = msgid; \
	(msg).simHeader.size = datasize + C_COMPR_FLAG_SIZE; \
	(msg).simCompress_flags.data_compressed = 0;  \
	(msg).simCompress_flags.data_compress_method = 0; \
	memset( & (msg).simDataCompr, 0, datasize ); 

#define	SIM_MSGSIZE(msg)	( sizeof(SFIDMSGHEADER) + (msg).simHeader.size ) 
#define SIM_PMSGSIZE(pmsg)	( sizeof(SFIDMSGHEADER) + (pmsg)->simHeader.size ) 

typedef struct IPSOCKADDR {
	uint32				ip;
	uint16				port;
} PACKED IPSOCKADDR;

typedef struct SIM_CHANNEL {
	int						socket; // non-zero for TCP, zero for UDP
	IPSOCKADDR				peer;	// ip:port
	IPSOCKADDR				local;	// 0:port
} PACKED SIM_CHANNEL;
typedef struct SIM_CHANNEL_WITH_SEQ {
	SIM_CHANNEL  channel;
	uint16				sync;// refer the simHeader::sync
} PACKED SIM_CHANNEL_WITH_SEQ;
typedef struct SIM_REQUEST {
	SIM_CHANNEL				channel;
	SIM_MESSAGE				msg;
} PACKED SIM_REQUEST;

typedef struct SIM_REQUEST_COMPR {
	SIM_CHANNEL				channel;
	SIM_MESSAGE_COMPR				msg;
} PACKED SIM_REQUEST_COMPR;

#define SIM_REQINITCHAN_COMPR(req, chan, msgid, msgdatasize, compresstype) \
	(req).channel = chan; \
	SIM_MSGINIT_COMPR( ((req).msg), (msgid), (msgdatasize  ) );\
	(req).msg.simCompress_flags.data_compressed  = 0; if (compresstype>0 ) (req).msg.simCompress_flags.data_compressed = 1;\
	(req).msg.simCompress_flags.data_compress_method = compresstype;

#define SIM_REQINIT_COMPR(req, sock, localip, localport, peerip, peerport, msgid, msgdatasize, compresstype) \
	(req).channel.socket = (sock); \
	(req).channel.local.ip = (localip); \
	(req).channel.local.port = (localport); \
	(req).channel.peer.ip = (peerip); \
	(req).channel.peer.port = (peerport); \
	SIM_MSGINIT_COMPR( ((req).msg), (msgid), (msgdatasize ) ); \
	(req).msg.simCompress_flags.data_compressed  = 0; if (compresstype>0 ) (req).msg.simCompress_flags.data_compressed = 1;\
	(req).msg.simCompress_flags.data_compress_method = compresstype;

#define SIM_REQINITCHAN(req, chan, msgid, msgdatasize) \
	(req).channel = chan; \
	SIM_MSGINIT( ((req).msg), (msgid), (msgdatasize) );

#define SIM_REQINIT(req, sock, localip, localport, peerip, peerport, msgid, msgdatasize) \
	(req).channel.socket = (sock); \
	(req).channel.local.ip = (localip); \
	(req).channel.local.port = (localport); \
	(req).channel.peer.ip = (peerip); \
	(req).channel.peer.port = (peerport); \
	SIM_MSGINIT( ((req).msg), (msgid), (msgdatasize) );

//#define SIM_REQSIZE(req)	( sizeof(SIM_CHANNEL) + SIM_MSGSIZE(req.msg) )
#define SIM_PREQSIZE(req)	( sizeof(SIM_CHANNEL) + SIM_MSGSIZE(req->msg) )

#define SIM_REPLY_REQUEST( reply, msgid, msgsize, req ) \
{ \
	(reply)->channel = (req)->channel; \
	SIM_MSGINIT( ((reply)->msg), (msgid), (msgsize) ) \
	(reply)->msg.simHeader.sync = (req)->msg.simHeader.sync; \
}

#define SIM_REPLY_REQUEST_COMPR( reply, msgid, msgsize, req, compresstype ) \
{ \
	(reply)->channel = (req)->channel; \
	SIM_MSGINIT_COMPR( ((reply)->msg), (msgid), (msgsize) ) \
	(reply)->msg.simCompress_flags.data_compressed = 0; if (compresstype>0 ) (reply)->msg.simCompress_flags.data_compressed = 1;  \
	(reply)->msg.simCompress_flags.data_compress_method = compresstype; \
	(reply)->msg.simHeader.sync = (req)->msg.simHeader.sync; \
}

#define SIM_DATASIZE_TO_REQSIZE( size )  (sizeof(SIM_CHANNEL) + sizeof(SFIDMSGHEADER) + (size))

#define SIM_REQUEST_NEW( req, size ) \
{ \
	req = (SIM_REQUEST *) malloc( size ); \
	if( req ) { \
		memset( req, 0, size ); \
	} \
}
#define SIM_REQUEST_NEW_COMPR( req, size ) \
{ \
	req = (SIM_REQUEST_COMPR *) malloc( size+C_COMPR_FLAG_SIZE ); \
	if( req ) { \
		memset( req, 0, size+C_COMPR_FLAG_SIZE ); \
	} \
}

#define SIM_REQUEST_CLONE( clone, req ) \
{ \
	int _reqsize = SIM_PREQSIZE( req ); \
	int _bufsize = max( sizeof(SIM_REQUEST), _reqsize ); \
	clone = (SIM_REQUEST *) malloc( _bufsize ); \
	if( clone ) { \
		memcpy( clone, req, _reqsize ); \
	} \
}

#define SIM_REQUEST_DELETE( req )	\
	if(req) free(req)

#if defined(WIN32) && defined(PTRACING) 
#define		PTRACE_SOCKET_ERROR() \
{ \
			int err = WSAGetLastError(); \
			const char * errString = "Unknown";\
			switch( err ) {\
			case WSANOTINITIALISED:		errString = "WSANOTINITIALISED";	break;\
			case WSAEFAULT:				errString = "WSAEFAULT";			break;\
			case WSAENETDOWN:			errString = "WSAENETDOWN";			break;\
			case WSAEINVAL:				errString = "WSAEINVAL";			break;\
			case WSAEINTR:				errString = "WSAEINTR";				break;\
			case WSAEINPROGRESS:		errString = "WSAEINPROGRESS";		break;\
			case WSAENOTSOCK:			errString = "WSAENOTSOCK";			break;\
			}\
			PTRACE( 1, "Socket error: " << err << ", " << errString );\
}
#else
#define		PTRACE_SOCKET_ERROR()
#endif

#pragma pack()

// 127.0.0.1

#define	LOCALHOST_IP 0x100007f

// 192.168.x.x, 172.[16-31].x.x, 10.x.x.x

#define	IS_C_TYPE(x)	(((x) & 0xffff) == 0x0000a8c0)
#define IS_B_TYPE(x)	(((x) & 0xf0ff) == 0x000010ac)
#define IS_A_TYPE(x)	(((x) & 0xff) ==   0x0000000a)
#define IS_LOCALHOST(x) ((x) == LOCALHOST_IP)

#define IS_PRIVATE_IP(x)	( IS_C_TYPE(x) || IS_B_TYPE(x) || IS_A_TYPE(x) )

#endif /* _SIM_PROTOCOL_ */
