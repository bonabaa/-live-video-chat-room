
#ifdef _WIN32

// define max size of fd_set, so that can handle large number of sockets
#ifdef FD_SETSIZE
#error Important: do not use precompiled header file for this .cxx file
#else
#define FD_SETSIZE      4096		// default is 64
#endif /* FD_SETSIZE */

#endif

#include "bbqbase.h"
void  SetSocketOptions(int fd) {
	// Disable Nagle's algorithm
	int nodelayval = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *) &nodelayval, sizeof(int)))
  {
    PTRACE(3,"BBQMsgTerminal\t Error disabling Nagle's algorithm");
  }
}
#include <time.h>
#include <sys/timeb.h>
extern const char* m_AESkey;
bool BBQMsgTerminal::NeedAck( uint16 msg ) const 
{
  
	switch( msg )
	{
	case SIM_SC_FREEGROUP_NOTIFY:
	case SIM_CS_USERMSG:
	case SIM_CS_NOTIFY_READY:
	case SIM_CS_CONFERENCE:
	case SIM_CS_CHANGESTATUS:
	case SIM_CS_MCC_NOTIFY:
	case SIM_CS_AUTH_CONTACT:
	case UCM_CC_CONCFM:
	case UCM_CC_CON:
	case UCM_CS_Q:
	case UCM_CS_A:

	case SIM_SC_INVITEVISIT:
	case SIM_SC_BROADCASTTEXT:

	case SIM_SS_SERVER_UPDATE:
	case SIM_CS_SERVERSTATUS:
	case SIM_SS_DOMAIN_UPDATE:
	case SIM_CS_SERVERSWITCH:
	//case SIM_SC_SERVERSWITCH:
	case UCM_SC_PROXYNOTIFY:
	case UCM_SC_FWD_Q:
	//case SIM_SC_CONFERENCE:

	case UCM_SC_A:
	case SIM_SC_IM:
	case SIM_SC_IM_NOTIFY:
	case SIM_SC_GET_IM:
	case SIM_SC_MCC_NOTIFY:
	case SIM_SC_MCC_NOTIFY_EX:
	case SIM_SC_NOTIFY_CONTACT:

	case SIM_SC_QUERYSERVICE:
	case SIM_SC_KICK:
	case SIM_SC_BUDDYSTATUSNOTIFY:
	case SIM_SC_SELFBUDDYSTATUSNOTIFY:
	case SIM_CS_SENDTEXT:
  //case SIM_CS_CONFERENCE:
	case SIM_CS_MCC_CMD:
	case SIM_SC_MCC_CMD:
	case SIM_CS_MEETUPDATE:
	case SIM_SC_MEETCTRL:
		return true;
	}
	return false;
}

const char * SIM_STATUSCODENAME( int statusCode )
{
	const char * statusName = "unknown";

	switch( statusCode ) {
	case SIMS_OK:					statusName = "okay"; break;
	case SIMS_BADREQUEST:			statusName = "bad request"; break;
	case SIMS_NOTFOUND:				statusName = "not found"; break;
	case SIMS_DENY:					statusName = "deny"; break;
	case SIMS_NOTAVAILABLE:			statusName = "not available"; break;
	case SIMS_TIMEOUT:				statusName = "timeout"; break;
	case SIMS_SERVERERROR:			statusName = "server error"; break;
	case SIMS_UNKNOWNERROR:			statusName = "unknown error"; break;
	case SIMS_NOTIMPLEMENTED:		statusName = "not implemented"; break;
	case SIMS_NETWORKERROR:			statusName = "network error"; break;
	case SIMS_REFUSE:				statusName = "refuse"; break;
	case SIMS_FIREWALL:				statusName = "firewall problem"; break;
	case SIMS_NORES:				statusName = "no resource"; break;
	case SIMS_AUTHREQUIRED:			statusName = "auth required"; break;
	case SIMS_SERVICEEXPIRED:		statusName = "service expired"; break;
	}

	return statusName;
}

const char * SIM_MSGNAME( int msgid )
{
	switch( msgid ) {
	case SIM_NONE: return "SIM_NONE";
	case SIM_SC_FREEGROUP_NOTIFY: return "SIM_SC_FREEGROUP_NOTIFY";
	case SIM_CS_MEETCREATE: return "SIM_CS_MEETCREATE";

// directory service protocol, be implemented by XLM
	case SIM_CS_HELLO: return "SIM_CS_HELLO";			// say hello, ask my ip:port
	case SIM_SC_HELLO: return "SIM_SC_HELLO";			

	case SIM_CS_LOGIN: return "SIM_CS_LOGIN";			// login: provide product serial & reg key, and ID (optional), and ask for a cookie
	case SIM_SC_LOGIN: return "SIM_SC_LOGIN";				

	case SIM_CS_LOCATE: return "SIM_CS_LOCATE";			// locate status info about others
	case SIM_SC_LOCATE: return "SIM_SC_LOCATE";		

	case SIM_CS_HEARTBEAT: return "SIM_CS_HEARTBEAT";		// keep online
	case SIM_SC_HEARTBEAT: return "SIM_SC_HEARTBEAT";

	case SIM_CS_LOGOUT: return "SIM_CS_LOGOUT";			// logoff
	case SIM_CS_CHANGESTATUS: return "SIM_CS_CHANGESTATUS";	// change status, including busy, invisible, normal, etc.
	case SIM_SC_LOGINREPLACED: return "SIM_SC_LOGINREPLACED";	// login was replaced from another place 

	case SIM_CS_TESTFIREWALL: return "SIM_CS_TESTFIREWALL";	// test firewall, send to port 1, echo from port 3
	case SIM_SC_TESTFIREWALL: return "SIM_SC_TESTFIREWALL";

	case SIM_CS_SUBSCRIBE: return "SIM_CS_SUBSCRIBE";		// subscribe VFON service
	case SIM_SC_SUBSCRIBE: return "SIM_SC_SUBSCRIBE";

// ------ compartible only for version before 2004.06.07 -------------------
	case SIM_CS_OLDVERSIONINFO: return "SIM_CS_OLDVERSIONINFO";		// query client version requirement
	case SIM_SC_OLDVERSIONINFO: return "SIM_SC_OLDVERSIONINFO";
// ------ compartible end --------------------------------------------------

	case SIM_CS_QUERYSERVICE: return "SIM_CS_QUERYSERVICE";		// query service status
	case SIM_SC_QUERYSERVICE: return "SIM_SC_QUERYSERVICE";

	case SIM_CS_VERSIONINFO: return "SIM_CS_VERSIONINFO";		// query client version requirement
	case SIM_SC_VERSIONINFO: return "SIM_SC_VERSIONINFO";

	case SIM_CS_QUERYGKLIST: return "SIM_CS_QUERYGKLIST";		// query gatekeeper server list
	case SIM_SC_QUERYGKLIST: return "SIM_SC_QUERYGKLIST";

	case SIM_CS_QUERYPSLIST: return "SIM_CS_QUERYPSLIST";		// query proxy server list
	case SIM_SC_QUERYPSLIST: return "SIM_SC_QUERYPSLIST";

// 登录：
// (1) 必须用 ID、密码、硬件ID 登录；
// (2) 通用结构，可用于各种绑定信息登录，例如香港PCCW的CVG（CVG ID）、BB（email地址）、IVC（手机号码）等；

	case SIM_CS_LOGIN_EX: return "SIM_CS_LOGIN_EX";		// login with id, password, hardware id, other mapping alias, etc.
	case SIM_SC_LOGIN_EX: return "SIM_SC_LOGIN_EX";

// 查询ID：
// 可以通过查询，输入信息可以是VFON ID或者绑定信息，返回 VFON ID以及位置、防火墙等信息，可以用于呼叫等后续操作；
	case SIM_CS_LOCATE_EX: return "SIM_CS_LOCATE_EX";
	case SIM_SC_LOCATE_EX: return "SIM_SC_LOCATE_EX";

// 通话记录：
// (1) 通话开始和结束时，客户端报告到服务器，服务器内存中保持通话动态列表，并在通话结束时记录到LOG文件或者数据库中；
// (2) Video、Vmeet等高级功能的使用，在通话过程中的Heartbeat以及通话结束时，报告到服务器，记录时一并记录，用于billing时使用；

// report that this client begin/end a conference with another user, for server to manage/log conference sessions, response is ACK 
	case SIM_CS_CONFERENCE: return "SIM_CS_CONFERENCE";	

// server ask client to begin/end a conference with another user, for server to manage/cut conference sessions, response is ACK
	case SIM_SC_CONFERENCE: return "SIM_SC_CONFERENCE";

// 可定义的全局字符串：
// (1) 服务器可以手工设定一些字符串，Name --> String，客户端可以通过一个函数去获取；
// (2) 需要设定的例如：下载软件的网址；修改账号信息的网址以及格式；等等；
// (3) 为了防止客户端读取服务器其他字符串，客户端需要的数据名一律加前缀 CLIENT_xxx ，通讯时仅传递 xxx 部分。

	case SIM_CS_LOADSTRING: return "SIM_CS_LOADSTRING";	// get a pre-defined string from server (or global database)
	case SIM_SC_LOADSTRING: return "SIM_SC_LOADSTRING";	

	case SIM_CS_GETBINDINFO: return "SIM_CS_GETBINDINFO";	// Vfon Id <--> PCCW Info
	case SIM_SC_GETBINDINFO: return "SIM_SC_GETBINDINFO";

	case SIM_CS_PROXYSTATUS: return "SIM_CS_PROXYSTATUS";		// query proxy status
	case SIM_SC_PROXYSTATUS: return "SIM_SC_PROXYSTATUS";

	case SIM_CS_SERVERSTATUS: return "SIM_CS_SERVERSTATUS";	// query server status
	case SIM_SC_SERVERSTATUS: return "SIM_SC_SERVERSTATUS";

	case SIM_CS_SERVERSWITCH: return "SIM_CS_SERVERSWITCH";	// do server switch
	case SIM_SC_SERVERSWITCH: return "SIM_SC_SERVERSWITCH";

	case SIM_SC_BUDDYSTATUSNOTIFY: return "SIM_SC_BUDDYSTATUSNOTIFY";	// notify msg when buddy status changed

	case SIM_CS_GETMCULIST: return "SIM_CS_GETMCULIST";			// download mcu list from server
	case SIM_SC_GETMCULIST: return "SIM_SC_GETMCULIST";

	case SIM_SX_UPDATESWITCHINFO: return "SIM_SX_UPDATESWITCHINFO";	// update switch info to cluster center, S -- server, X -- exchange center

	case SIM_SX_QSWITCHINFO: return "SIM_SX_QSWITCHINFO";			// query cluster center for the switch info
	case SIM_XS_ASWITCHINFO: return "SIM_XS_ASWITCHINFO";

	case SIM_SX_QLOCATE: return "SIM_SX_QLOCATE";				// ask for the location of a vfon client
	case SIM_XS_ALOCATE: return "SIM_XS_ALOCATE";
	//case SIM_IM: return "SIM_IM";

	case SIM_SS_SERVER_UPDATE: return "SIM_SS_SERVER_UPDATE";			// update local server info, from clustered to center, or to other clustered
	case SIM_SS_SERVER_UPDATEECHO: return "SIM_SS_SERVER_UPDATEECHO";

	case SIM_SS_DOMAIN_UPDATE: return "SIM_SS_DOMAIN_UPDATE";			// update domain info, usually from child to parent
	case SIM_SS_DOMAIN_UPDATEECHO: return "SIM_SS_DOMAIN_UPDATEECHO";		// feedback, ok or fail

	case SIM_SS_DOMAIN_SYNC: return "SIM_SS_DOMAIN_SYNC";				// request domain to start sync the info of other domains
	case SIM_SS_DOMAIN_SYNCDATA: return "SIM_SS_DOMAIN_SYNCDATA";			// domain data (1 or more)

	case SIM_SX_BUDDYSTATUSNOTIFY: return "SIM_SX_BUDDYSTATUSNOTIFY";		// request domain server to notify buddy status 
	case SIM_XS_BUDDYSTATUSNOTIFY: return "SIM_XS_BUDDYSTATUSNOTIFY";		// buddy status notify from domain server to login server

	case SIM_CS_LOGUSERACTION: return "SIM_CS_LOGUSERACTION";			// write user action log to server DB for auditing purpose
	case SIM_SC_LOGUSERACTION: return "SIM_SC_LOGUSERACTION";

	case SIM_CS_GET_PKI_CERT: return "SIM_CS_GET_PKI_CERT";			// download server's digital certificate in PEM format
	case SIM_SC_GET_PKI_CERT: return "SIM_SC_GET_PKI_CERT";

	case SIM_CS_LOGIN_EX_SECURE: return "SIM_CS_LOGIN_EX_SECURE";			// login, with data encrypted

	case SIM_CS_MCC_INVITE: return "SIM_CS_MCC_INVITE";			// MCU invite client to meeting
	case SIM_SC_MCC_INVITE: return "SIM_SC_MCC_INVITE";

//SIM_CS_KICK,			// kick
	case SIM_SC_KICK: return "SIM_SC_KICK";		    //kick from server	


// reserved for message terminal layer use.
	case SIM_ACK: return "SIM_ACK";
	case SIM_SEGMENT: return "SIM_SEGMENT";

// protocol to setup UDP/TCP channel between terminals under help of BBQ server, be implemented by XLM
// _CS_ means Client -> Server,
// _SC_ means Server -> Client,
// _CC_ means Client -> Client,
	case UCM_V1MIN: return "UCM_V1MIN";

	case UCM_CC_FIRSTOUT: return "UCM_CC_FIRSTOUT";				// try out firewall, so that peer can enter

	case UCM_CS_Q: return "UCM_CS_Q";						// client <-> server, request/response
	case UCM_SC_FWD_Q: return "UCM_SC_FWD_Q";
	case UCM_CS_A: return "UCM_CS_A";
	case UCM_SC_A: return "UCM_SC_A";

	case UCM_CC_CON: return "UCM_CC_CON";						// udp connect
	case UCM_CC_CONCFM: return "UCM_CC_CONCFM";

	case UCM_SP_CREATE: return "UCM_SP_CREATE";					// server -> proxy, ask proxy to create/release a proxy channel for client
	case UCM_SP_RELEASE: return "UCM_SP_RELEASE";
	case UCM_PS_NOTIFY: return "UCM_PS_NOTIFY";					// proxy -> server, notify channel status change

	case UCM_SC_PROXYNOTIFY: return "UCM_SC_PROXYNOTIFY";				// server -> client, notify that proxy channel status changed, UP/DOWN

	case UCM_CC_IDQUERY: return "UCM_CC_IDQUERY";					// ask id
	case UCM_CC_IDCONFIRM: return "UCM_CC_IDCONFIRM";				// reply id

	case UCM_CC_Q: return "UCM_CC_Q";				// direct channel request
	case UCM_CC_A: return "UCM_CC_A";				// response of direct channel request

	case UCM_CP_CONNECT: return "UCM_CP_CONNECT";
	case UCM_PC_CONNECT: return "UCM_PC_CONNECT";

	case UCM_V1MAX: return "UCM_V1MAX";

// reliable UDP protocol, be implemented by ZHX
	case SIM_RUDP_DATA: return "SIM_RUDP_DATA";
	case SIM_RUDP_ACK: return "SIM_RUDP_ACK";
	case SIM_VTCP_DATA: return "SIM_VTCP_DATA";
	case SIM_VTCP_ACK: return "SIM_VTCP_ACK";
	case SIM_VUDP_DATA: return "SIM_VUDP_DATA";

// account user data 
	case SIM_CS_REGISTERUSER: return "SIM_CS_REGISTERUSER";		// register user's data, ask to create an account
	case SIM_SC_REGISTERUSER: return "SIM_SC_REGISTERUSER";

	case SIM_CS_ERASEUSER: return "SIM_CS_ERASEUSER";				// user erase its own account
	case SIM_SC_ERASEUSER: return "SIM_SC_ERASEUSER";

	case SIM_CS_UPDATEUSERDATA: return "SIM_CS_UPDATEUSERDATA";			// update user's data
	case SIM_SC_UPDATEUSERDATA: return "SIM_SC_UPDATEUSERDATA";

	case SIM_CS_UPDATECALLFWDINFO: return "SIM_CS_UPDATECALLFWDINFO";		// update user's call forward records
	case SIM_SC_UPDATECALLFWDINFO: return "SIM_SC_UPDATECALLFWDINFO";

	case SIM_CS_GETCALLFWDINFO: return "SIM_CS_GETCALLFWDINFO";			// get call forward info
	case SIM_SC_GETCALLFWDINFO: return "SIM_SC_GETCALLFWDINFO";

	case SIM_CS_UPLOADDATABLOCK: return "SIM_CS_UPLOADDATABLOCK";			// upload data block and stored in server
	case SIM_SC_UPLOADDATABLOCK: return "SIM_SC_UPLOADDATABLOCK";

	case SIM_CS_DOWNLOADDATABLOCK: return "SIM_CS_DOWNLOADDATABLOCK";		// download data block from server
	case SIM_SC_DOWNLOADDATABLOCK: return "SIM_SC_DOWNLOADDATABLOCK";

// yellow page
	case SIM_CS_VIEWUSERDATA: return "SIM_CS_VIEWUSERDATA";		// view user's data of specified ID
	case SIM_SC_VIEWUSERDATA: return "SIM_SC_VIEWUSERDATA";

	case SIM_CS_GETUSERTECHPARAM: return "SIM_CS_GETUSERTECHPARAM";		// get user tech param
	case SIM_SC_GETUSERTECHPARAM: return "SIM_SC_GETUSERTECHPARAM";

	case SIM_CS_LISTONLINE: return "SIM_CS_LISTONLINE";				// list online neighbours, (on the same server)
	case SIM_SC_LISTONLINE: return "SIM_SC_LISTONLINE";

	case SIM_CS_SEARCHUSER: return "SIM_CS_SEARCHUSER";				// search user according to specified filter
	case SIM_SC_SEARCHUSER: return "SIM_SC_SEARCHUSER";

	case SIM_SC_MCC_NOTIFY_EX: return "SIM_SC_MCC_NOTIFY_EX";			// notify client/mcu a MCC event
	case SIM_CS_MCC_NOTIFY_EX: return "SIM_CS_MCC_NOTIFY_EX";			// report to server that the client has begin/end a meeting

// relationship
	case SIM_CS_ADDCONTACT: return "SIM_CS_ADDCONTACT";		// add contact to friend list
	case SIM_SC_ADDCONTACT: return "SIM_SC_ADDCONTACT";

	case SIM_CS_DELETECONTACT: return "SIM_CS_DELETECONTACT";			// delete contact from friend list
	case SIM_SC_DELETECONTACT: return "SIM_SC_DELETECONTACT";

	case SIM_CS_GETCONTACTLIST: return "SIM_CS_GETCONTACTLIST";			// download contact list from server
	case SIM_SC_GETCONTACTLIST: return "SIM_SC_GETCONTACTLIST";

	case SIM_CS_ADDBLOCK: return "SIM_CS_ADDBLOCK";				// add user id to block list
	case SIM_SC_ADDBLOCK: return "SIM_SC_ADDBLOCK";

	case SIM_CS_DELETEBLOCK: return "SIM_CS_DELETEBLOCK";				// delete user id from block list
	case SIM_SC_DELETEBLOCK: return "SIM_SC_DELETEBLOCK";

	case SIM_CS_GETBLOCKLIST: return "SIM_CS_GETBLOCKLIST";			// download block list from server
	case SIM_SC_GETBLOCKLIST: return "SIM_SC_GETBLOCKLIST";

	case SIM_SC_NOTIFY_CONTACT: return "SIM_SC_NOTIFY_CONTACT";			// notify the user to be added or blocked, post and need ACK

	case SIM_CS_AUTH_CONTACT: return "SIM_CS_AUTH_CONTACT";			// the user to be added send auth to server, just need ACK to make sure data received
	case SIM_SC_AUTH_CONTACT: return "SIM_SC_AUTH_CONTACT";			// feedback to the user that adding buddy, post and need ACK

	case SIM_CS_GETIDTREES: return "SIM_CS_GETIDTREES";				// get id trees that contain this user, for CUG purpose
	case SIM_SC_GETIDTREES: return "SIM_SC_GETIDTREES";

	case SIM_SC_MCC_NOTIFY: return "SIM_SC_MCC_NOTIFY";				// notify client/mcu a MCC event
	case SIM_CS_MCC_NOTIFY: return "SIM_CS_MCC_NOTIFY";				// report to server that the client has begin/end a meeting

	case SIM_CS_NOTIFY_READY: return "SIM_CS_NOTIFY_READY";			// ready to receive notify message

// the following msg will be forward by server
	case SIM_CS_USERMSG: return "SIM_CS_USERMSG";			// send user written message, this msg will be stored in user's box if offline
	case SIM_SC_USERMSG: return "SIM_SC_USERMSG";

	case SIM_CS_IM: return "SIM_CS_IM";						// send im, stored in db if offline
	case SIM_SC_IM: return "SIM_SC_IM";

	case SIM_CS_GET_IM: return "SIM_CS_GET_IM";					// client get im from server
	case SIM_SC_GET_IM: return "SIM_SC_GET_IM";

	case SIM_CS_DEL_IM: return "SIM_CS_DEL_IM";					// request to delete the offline im
	case SIM_SC_DEL_IM: return "SIM_SC_DEL_IM";

	case SIM_SC_INVITEVISIT: return "SIM_SC_INVITEVISIT";		// invite a client to visit/call one or more target
	case SIM_SC_BROADCASTTEXT: return "SIM_SC_BROADCASTTEXT";
	case SIM_CS_PINGPROXY: return "SIM_CS_PINGPROXY";
	case SIM_CS_SENDTEXT: return "SIM_CS_SENDTEXT";

	case SIM_CS_GET_PKI_CERT_COMPR: return "SIM_CS_GET_PKI_CERT_COMPR";			
	case SIM_SC_GET_PKI_CERT_COMPR: return "SIM_SC_GET_PKI_CERT_COMPR";      
	case SIM_CS_GETCONTACTLIST_COMPR: return "SIM_CS_GETCONTACTLIST_COMPR";		 
	case SIM_SC_GETCONTACTLIST_COMPR: return "SIM_SC_GETCONTACTLIST_COMPR";    
	case SIM_CS_LOADSTRING_COMPR: return "SIM_CS_LOADSTRING_COMPR";	      
	case SIM_SC_LOADSTRING_COMPR: return "SIM_SC_LOADSTRING_COMPR";	      
	case SIM_CS_DOWNLOADDATABLOCK_COMPR: return "SIM_CS_DOWNLOADDATABLOCK_COMPR";  
	case SIM_SC_DOWNLOADDATABLOCK_COMPR: return "SIM_SC_DOWNLOADDATABLOCK_COMPR"; 
	case SIM_CS_GETIDTREES_COMPR: return "SIM_CS_GETIDTREES_COMPR"; 
	case SIM_SC_GETIDTREES_COMPR: return "SIM_SC_GETIDTREES_COMPR"; 
	case SIM_SC_SELFBUDDYSTATUSNOTIFY: return "SIM_SC_SELFBUDDYSTATUSNOTIFY"; 
	case SIM_SC_MEETCTRL: return "SIM_SC_MEETCTRL"; 
	}

	return "undefined";
}

#define SIM_MATCH( f, value )		( (f) == 0 || (f) == (value) )

#define SIM_ADDR_MATCH( f, addr )  ( SIM_MATCH( (f).ip, (addr).ip ) && SIM_MATCH( (f).port, (addr).port ) )

#define SIM_UDP_MATCH( f, req ) \
	( ( (f)->channel.socket == 0 ) && \
	SIM_ADDR_MATCH( (f)->channel.peer, (req)->channel.peer ) && \
	SIM_MATCH( (f)->channel.local.port, (req)->channel.local.port ) ) // UDP

#define SIM_TCP_MATCH( f, req ) \
	( ((f)->channel.socket != 0) && ((f)->channel.socket == (req)->channel.socket) )

#define SIM_ACKFILTER_MATCH( f, reply ) \
	(	(f != NULL) && \
		(SIM_UDP_MATCH(f, reply) || SIM_TCP_MATCH(f, reply)) && \
		( (reply)->msg.simHeader.id == SIM_ACK) && ( (f)->msg.simHeader.sync == (reply)->msg.simHeader.sync ) \
	)

#define SIM_ACKFILTER_MATCH_2( f, reply ) \
	(	(f != NULL) && \
		(SIM_UDP_MATCH(f, reply) || SIM_TCP_MATCH(f, reply)) && \
		( (reply)->msg.simHeader.id == SIM_ACK) && ( (f)->msg.simHeader.sync == (reply)->msg.simHeader.sync ) && \
		( (reply)->msg.simHeader.size < sizeof(SIMD_ACK) || (((SIMD_ACK*)(reply)->msg.simData)->ackid == (f)->msg.simHeader.id) ) \
	)

#define SIM_REQFILTER_MATCH( f, req ) \
	( (f == NULL) || \
	( ( SIM_TCP_MATCH( f, req ) || SIM_UDP_MATCH( f, req ) ) && \
	  SIM_MATCH( (f)->msg.simHeader.id, (req)->msg.simHeader.id ) \
	  && SIM_MATCH( (req)->msg.simHeader.sync, (f)->msg.simHeader.sync ) ) \
	)

void BBQMsgTerminal::ReleaseMessage( void * req )
{
	SIM_REQUEST_DELETE( req );
}

void BBQMsgTerminal::EraseChannel( SIM_CHANNEL & chan )
{
	if( chan.socket ) {
		BBQMsgConnection * pConn = FindMsgConnection(0,0,chan.socket);
		if( pConn && DetachMsgConnection( pConn ) ) {
			PTRACE(1, " Delete Message Connection: " << (DWORD)pConn << " -" << (DWORD)pConn->GetSocket());
			delete pConn;
		}
	} else {
		PUDPSocket * pSock = FindUdpListener( chan.local.port );
		if( pSock && DetachUdpListener(pSock) ) {
			delete pSock;
		}
	}
}


// ------------------ BBQMsgTerminal::SegSession -----------------------------
#ifdef SUPPORT_BIG_MESSAGE

#define BIG_MESSAGE_SESSION_TIME_MAX		10000	// 10 seconds

typedef struct SIMD_SEGMENT_HEADER {
	uint32		sync;
	uint16		byte_total, byte_offset;
	uint16		segment_total, segment_offset;
} SIMD_SEGMENT_HEADER;

BBQMsgTerminal::SegSession::SegSession( const SIM_REQUEST * reqseg )
{
	SIMD_SEGMENT_HEADER * pHeader = (SIMD_SEGMENT_HEADER *) & reqseg->msg.simData;

	//sync = 0;
	//byte_total = byte_left = 0;
	//segment_total = 0;
	//segment_flags = NULL;
	//req = NULL;
	timestamp = BBQMsgTerminal::GetHostUpTimeInMs();

	segment_total = pHeader->segment_total;
	byte_total = pHeader->byte_total;

	segment_flags = new char[ segment_total ];
	memset( segment_flags, 0, sizeof(char) * segment_total );

	SIM_REQUEST_NEW( req, sizeof(SIM_CHANNEL) + byte_total );
	req->channel = reqseg->channel;

	sync = pHeader->sync;
	byte_left = byte_total;

	// copy the data of the first packet
	WORD nBytes = reqseg->msg.simHeader.size - sizeof(SIMD_SEGMENT_HEADER);
	if( nBytes <= byte_left ) {
		char * pDest = ((char *) & req->msg) + pHeader->byte_offset;
		const char * pSrc = (char *) & reqseg->msg.simData[ sizeof(SIMD_SEGMENT_HEADER) ];
		memcpy( pDest, pSrc, nBytes );
		segment_flags[ pHeader->segment_offset ] = 1;
		byte_left -= nBytes;
	}
}

BBQMsgTerminal::SegSession::~SegSession()
{
	if( segment_flags ) {
		delete[] segment_flags;
	}
	if( req ) {
		SIM_REQUEST_DELETE( req );
	}
}

bool BBQMsgTerminal::SegSession::Match( const SIM_REQUEST * reqseg )
{
	SIMD_SEGMENT_HEADER * pHeader = (SIMD_SEGMENT_HEADER *) & reqseg->msg.simData;
	//if( pHeader->byte_offset >= pHeader->byte_total || pHeader->segment_offset >= pHeader->segment_total ) return false;

	if( (memcmp(& req->channel, & reqseg->channel, sizeof(SIM_CHANNEL) ) == 0) && (sync == pHeader->sync) ) {
		if( ! segment_flags[ pHeader->segment_offset ] ) {
			// copy the content of the packet
			WORD nBytes = reqseg->msg.simHeader.size - sizeof(SIMD_SEGMENT_HEADER);
			if( nBytes <= byte_left ) {
				char * pDest = ((char *) & req->msg) + pHeader->byte_offset;
				const char * pSrc = (char *) & reqseg->msg.simData[ sizeof(SIMD_SEGMENT_HEADER) ];
				memcpy( pDest, pSrc, nBytes );
				segment_flags[ pHeader->segment_offset ] = 1;
				byte_left -= nBytes;
			}
		}
		return true;
	}

	return false;
}

SIM_REQUEST * BBQMsgTerminal::SegSession::GetRequest( void )
{
	if( ! byte_left ) {
		SIM_REQUEST * p = req;
		req = NULL;
		return p;
	}

	return NULL;
}

#endif // SUPPORT_BIG_MESSAGE

// ------------------ BBQMsgTerminal::MsgWait --------------------------------

bool BBQMsgTerminal::MsgWait::MatchFilter( const SIM_REQUEST * req ) 
{
	if( m_bACK ) {
		return SIM_ACKFILTER_MATCH_2( m_pFilter, req );

	} else if( SIM_REQFILTER_MATCH(m_pFilter, req) ) {
		SIM_REQUEST_CLONE( m_pReq, req );
		return true;
	}
	return false;
}

bool BBQMsgTerminal::MsgWait::WaitRequest( SIM_REQUEST * & reply, DWORD timeout ) 
{
	// the terminal service thread must keep running and dispatching message, 
	// so it is not allowed to wait on its own
	if( m_owner.GetCurrentThreadId() == m_owner.GetThreadId() ) {
		cerr << "Logical error, never wait message in message loop thread.\n";
#if defined(_MSC_VER) && defined(_DEBUG) && !defined(_WIN32_WCE)
		__asm int 3;
#endif
		_exit(1);
	}

	bool bDone = (FALSE != Wait( timeout ? timeout : PMaxTimeInterval ));

	reply = m_pReq;
	m_pReq = NULL;

	return bDone; 
}

BBQMsgTerminal::MsgWait::MsgWait( BBQMsgTerminal & owner, const SIM_REQUEST * pFilter, bool bACK ) 
: PSemaphore(0, 1), m_owner(owner), m_pFilter(pFilter), m_bACK(bACK) 
{ 
	m_pReq = NULL;

	m_owner.m_mutexMsgWait.Wait();
	m_owner.m_listMsgWaits.push_back( this );
	m_owner.m_mutexMsgWait.Signal();
}

BBQMsgTerminal::MsgWait::~MsgWait() 
{
	m_owner.m_mutexMsgWait.Wait();
	m_owner.m_listMsgWaits.remove( this );
	m_owner.m_mutexMsgWait.Signal();

	SIM_REQUEST_DELETE( m_pReq );
}

// ----------------------------- BBQMsgTerminal::TcpListenerThread ----------------------

BBQMsgTerminal::TcpListenerThread::TcpListenerThread( BBQMsgTerminal & owner, PTCPSocket * pSock )
: PThread(8192, NoAutoDeleteThread,PThread::HighestPriority ), m_owner(owner), m_pSock(pSock)
{
	{
		LockIt safe( m_owner.m_mutexRequest );

	#ifdef USE_MAP_FOR_LIST
		WORD port = pSock->GetPort();
		tth_iterator iter = m_owner.m_listTcpListenerThreads.find( port );
		if( iter != m_owner.m_listTcpListenerThreads.end() ) {
			if( VAR_AT(iter) ) {
				TcpListenerThread * th = VAR_AT(iter);
				VAR_AT(iter) = NULL;

				th->Close();
				{
					UnlockIt yield( m_owner.m_mutexRequest );
					delete th;
				}
			}
			VAR_AT(iter) = this;
		} else {
			m_owner.m_listTcpListenerThreads[ port ] = this;
		}
	#else
		m_owner.m_listTcpListenerThreads.push_back( this );
	#endif
	}

	Resume();
}

BBQMsgTerminal::TcpListenerThread::~TcpListenerThread()
{
	if( ! IsTerminated() ) {
		if( GetThreadId() != GetCurrentThreadId() ) {
			if( IsSuspended() ) Resume();
			WaitForTermination();
		}
	}

	{
		LockIt safe( m_owner.m_mutexRequest );
		for( tth_iterator iter = m_owner.m_listTcpListenerThreads.begin(), eiter = m_owner.m_listTcpListenerThreads.end(); iter != eiter; iter ++ ) {
			if( this == VAR_AT(iter) ) {
				//m_owner.m_listTcpListenerThreads.erase( iter );
				STL_ERASE( m_owner.m_listTcpListenerThreads, tth_iterator, iter );
				break;
			}
		}
		if( m_pSock ) {
			delete m_pSock;
			m_pSock = NULL;
		}
	}
}

void BBQMsgTerminal::TcpListenerThread::Main( void )
{
	PTRACE( 3, "Tcp listener thread start." );

	int fd = m_pSock->GetHandle();
	fd_set fdsetRead;

	while( m_pSock && m_pSock->IsOpen() ) {

		FD_ZERO( & fdsetRead );
		FD_SET( fd, & fdsetRead );

		// do a select to wait I/O happen
		struct timeval tval = { 0x7fffffff, 0 };
		int ret = ::select( fd+1, & fdsetRead, NULL, NULL, &tval);
		if( ret < 0 ) {
			PTRACE_SOCKET_ERROR();
			continue;
		} else if( ret == 0 ) {
			continue;
		}

		// listen in block mode, 
		// wait for any connection, the put to msgconnection list
		if( ! m_owner.AcceptNewConnection( m_pSock ) ) {
			Sleep(1);
		}
	}

	PTRACE( 3, "Tcp listener thread exit." );
}

void BBQMsgTerminal::TcpListenerThread::Close( void )
{
	if( m_pSock ) m_pSock->Close();
}

// ----------------------------- BBQMsgTerminal::HandlerThread ----------------------

BBQMsgTerminal::HandlerThread::HandlerThread( BBQMsgTerminal & owner )
: PThread(20480, NoAutoDeleteThread, PThread::HighestPriority ), m_owner(owner), m_bClose(false)
{
	m_owner.m_mutexRequest.Wait();
	m_owner.m_listRequestHandlers.push_back( this );
	m_owner.m_mutexRequest.Signal();

	Resume();
}

BBQMsgTerminal::HandlerThread::~HandlerThread()
{
	if( ! IsTerminated() ) {
		if( GetThreadId() != GetCurrentThreadId() ) {
			if( IsSuspended() ) Resume();
			WaitForTermination();
		}
	}

	m_owner.m_mutexRequest.Wait();
	m_owner.m_listRequestHandlers.remove( this );
	m_owner.m_mutexRequest.Signal();
}

void BBQMsgTerminal::HandlerThread::Close( void )
{
	m_bClose = true;
}

void BBQMsgTerminal::HandlerThread::Main( void )
{
	PTRACE( 3, "Request handler thread start." );

	do {
		if( m_owner.IsClosing() || m_bClose ) break;

		// pick from queue, wait if there is no message temprorarily
		SIM_REQUEST * req = m_owner.GetRequestFromQueue();

		if( req ) {
			m_owner.OnMessageArrive( req );

			m_owner.ReleaseMessage( req );
		}
	} while ( true );

	PTRACE( 3, "Request handler thread exit." );
}

bool BBQMsgTerminal::PutRequestToQueue( SIM_REQUEST * req )
{
	{
		LockIt safe( m_mutexRequest );
		m_listRequests.push_back( req );
		if( m_nWaitingHandlers > 0 ) m_semRequest.Signal();
	}

	Yield();

	return true;
}

bool BBQMsgTerminal::CopyRequestToQueue( const SIM_REQUEST * req )
{
	{
		LockIt safe( m_mutexRequest );

		if( m_listRequests.size() >= m_nRequestQueueMax ) {
			//PTRACE( 1, "warning: msg drop due to queue full." );
			PLOG( m_pLogger, Debug, "warning: msg drop due to queue full." );
			return false;
		}

		SIM_REQUEST * clone = NULL;
		SIM_REQUEST_CLONE( clone, req );
		m_listRequests.push_back( clone );

		if( m_nWaitingHandlers > 0 ) m_semRequest.Signal();
	}

	Yield();

	return true;
}

SIM_REQUEST * BBQMsgTerminal::GetRequestFromQueue( void )
{
	LockIt safe( m_mutexRequest );

	SIM_REQUEST * req = NULL;

	if( m_listRequests.empty() ) {
		m_nWaitingHandlers ++;
		m_mutexRequest.Signal(); // unlock before wait

		m_semRequest.Wait();

		m_mutexRequest.Wait();	// lock again
		m_nWaitingHandlers --;
	}

	if( m_listRequests.size() > 0 ) {
		req = m_listRequests.front();
		m_listRequests.pop_front();
	} else {
		req = NULL;
	}

	return req;
}

// -------------------------- BBQMsgTerminal::BBQTickThread --------------

BBQMsgTerminal::TickThread::TickThread( BBQMsgTerminal & owner )
: PThread(10000, NoAutoDeleteThread,PThread::HighestPriority ), m_owner(owner)
{
	Resume();
}

BBQMsgTerminal::TickThread::~TickThread()
{
	if( ! IsTerminated() ) {
		if( GetThreadId() != GetCurrentThreadId() ) {
			if( IsSuspended() ) Resume();
			WaitForTermination();
		}
	}
}

void BBQMsgTerminal::TickThread::Main( void )
{
	MS_TIME msTickCalled = GetHostUpTimeInMs();

	while( ! m_owner.IsClosing() ) {
		MS_TIME msNow = BBQMsgTerminal::GetHostUpTimeInMs();
		if( msNow >= msTickCalled + 1000 ) {
			msTickCalled = msNow;
			m_owner.OnTick();
		} else if ( msNow < msTickCalled ) {
			msTickCalled = msNow;
			PString strMsg( PString::Printf, "OnTick() timer error, adjust now!" );
			PLOG( m_owner.m_pLogger, Error, strMsg );
		}

		sleep_ms(100);
	}
}


// ---------------------------------- BBQMsgTerminal -----------------------------

BBQMsgTerminal::BBQMsgTerminal( unsigned int nMsgQueueMax, int nHandlers, bool bEncryptMessage )
: PThread( 20000, NoAutoDeleteThread, PThread::HighestPriority ), m_semRequest(0,256)
{
    
    srand( (unsigned int)time( NULL ) );

	startup_socket();

//#ifdef _WIN32
//	WSADATA wsaData; 
//	int nRet = WSAStartup(MAKEWORD(2,1), &wsaData);
//	if ( nRet != 0 )
//	{
////		TRACE( "WSAStartup Error:%d\n", GetLastError() );
//	}
//#endif

	m_pLogger = NULL;
  bEncryptMessage = true;
#ifdef ENCRYPT_MESSAGE
	m_bEncryptMessage = bEncryptMessage;
	m_nEncBufferSize = sizeof(SIM_MESSAGE);
	m_pEncBuffer = (char *) malloc( m_nEncBufferSize );
#endif

	m_dwConnectTimeout = 30000;//0x7fffffff;//PMaxTimeInterval.GetInterval();

	m_nMsgPostMax = 44096*10;

	m_bClosing = false;
	m_nConnectionMax = 0;
	m_dwConnectionIdleMax = 0;

	m_nRequestQueueMax = nMsgQueueMax;
	m_nWaitingHandlers = 0;

	m_nSyncCounter = 0;

#ifdef SUPPORT_BIG_MESSAGE
	m_nSegSyncSerial = 0;
	//m_nUDPDataLimit = SIM_DATABLOCKSIZEMAX; // for common routers
	m_nUDPDataLimit = 1400; // for common routers
	//m_nUDPDataLimit = 548; // for dialup networking
	//m_nUDPDataLimit = 128; // for debug & test only
#endif

	for( int i=0; i<nHandlers; i++ ) {
		new HandlerThread( *this );
	}

	m_pTickThread = new TickThread( *this );

	Resume();
}

void BBQMsgTerminal::SetNumberOfHandlers( int n )
{
	int m = m_listRequestHandlers.size();
	if( n > m ) {
		n -= m;
		for( int i=0; i<n; i++ ) {
			new HandlerThread( *this );
		}
	} else if ( n < m ) {
		{
			LockIt safe( m_mutexRequest );

			for( int i=0; i<m; i++ ) {
				// give signal to the waiting thread to go on and abort
				m_semRequest.Signal();
			}

			m -= n;
			for( reqh_iterator iter = m_listRequestHandlers.begin(), eiter = m_listRequestHandlers.end(); iter != eiter; /**/ ) {
				HandlerThread * th = * iter;
				//iter = m_listRequestHandlers.erase( iter );
				STL_ERASE( m_listRequestHandlers, reqh_iterator, iter );
				if( th ) {
					th->Close();

					// now yield and wait the thread to exit, then delete it
					{
						UnlockIt safe( m_mutexRequest );
						delete th;
					}

				}
				m --;
				if( m == 0 ) break;
			}

		}
	}
}

BBQMsgTerminal::~BBQMsgTerminal()
{
	OnClose();

	Destruct();

	cleanup_socket();
//#ifdef _WIN32
//	::WSACleanup();
//#endif
}

void BBQMsgTerminal::OnClose(void)
{
	// close all sockets, so that they can make the blocked threads exit
	{
		// notify all the listener threads close socket and exit
		// stop all the listener thread
		{
			// wait all the handler thread to exit, and delete the object
			LockIt safe( m_mutexRequest );
			for( tth_iterator iter = m_listTcpListenerThreads.begin(), eiter = m_listTcpListenerThreads.end(); iter != eiter; /**/ ) {
				TcpListenerThread * tth = VAR_AT(iter);
				//iter = m_listTcpListenerThreads.erase(iter);
				STL_ERASE( m_listTcpListenerThreads, tth_iterator, iter );

				if( tth ) tth->Close();
				{
					// the thread to be waited will require the same mutex, so release here for a while
					UnlockIt yield( m_mutexRequest );
					delete tth;
				}
			}
		}

		LockIt safe( m_mutexNet );
		{
			for( tcp_iterator iter = m_listTcpListeners.begin(), eiter = m_listTcpListeners.end(); iter != eiter; iter ++ ) {
				PTCPSocket * pSock = VAR_AT(iter);
				if( pSock ) pSock->Close();
			}
		}
		{
			for( udp_iterator iter = m_listUdpListeners.begin(), eiter = m_listUdpListeners.end(); iter != eiter; iter ++ ) {
				PUDPSocket * pSock = VAR_AT(iter);
				if( pSock ) pSock->Close();
			}
		}
		{
			for( con_iterator iter = m_listMsgConnections.begin(), eiter = m_listMsgConnections.end(); iter != eiter; iter ++ ) {
				BBQMsgConnection * pConn = VAR_AT(iter);
				if( pConn ) {
					PTCPSocket * pSock = pConn->GetSocket();
					if( pSock ) pSock->Close();
				}
			}
		}
	}

	if( m_bClosing ) return;

	// flag to notify the main loop to exit
	m_bClosing = true;

	// wait the thread to terminate
	if( ! IsTerminated() ) {
		if( GetThreadId() != GetCurrentThreadId() ) {
			if( IsSuspended() ) Resume();
			WaitForTermination();
		}
	}

	// wait the tick thread to terminate
	if( m_pTickThread ) {
		delete m_pTickThread;
		m_pTickThread = NULL;
	}

	// notify those who are waiting to abort
	{
		LockIt safe( m_mutexMsgWait );

		while( ! m_listMsgWaits.empty() ) {
			MsgWait * wait = m_listMsgWaits.front();
			m_listMsgWaits.pop_front();

			wait->Signal(); 
			// no need to delete this object, the thread who r waiting will delete it
		}
	}

	// stop all the request handler thread
	{
		LockIt safe( m_mutexRequest );

		// clear the msg queue
		while( ! m_listRequests.empty() ) {
			SIM_REQUEST * req = m_listRequests.front();
			m_listRequests.pop_front();

			SIM_REQUEST_DELETE( req );
		}

		// notify all the waiting thread to abort
		for( int i = m_listRequestHandlers.size(); i>0; i-- ) {
			m_semRequest.Signal();
		}

		// wait all the handler thread to exit, and delete the object
		while( ! m_listRequestHandlers.empty() ) {
			HandlerThread * ht = m_listRequestHandlers.front();
			m_listRequestHandlers.pop_front();
			{
				// the thread to be waited will require the same mutex, so release here for a while
				UnlockIt yield( m_mutexRequest );
				delete ht;
			}
		}
	}

}

void BBQMsgTerminal::Destruct( void )
{
	// delete all network interface
	{
		LockIt safe( m_mutexNet );

		{
			for( tcp_iterator iter = m_listTcpListeners.begin(), eiter = m_listTcpListeners.end(); iter != eiter; /**/ ) {
				PTCPSocket * pSock = VAR_AT(iter);
				if( pSock ) delete pSock;
				//iter = m_listTcpListeners.erase( iter );
				STL_ERASE( m_listTcpListeners, tcp_iterator, iter );
			}
		}
		{
			for( udp_iterator iter = m_listUdpListeners.begin(), eiter = m_listUdpListeners.end(); iter != eiter; /**/ ) {
				PUDPSocket * pSock = VAR_AT(iter);
				if( pSock ) delete pSock;
				//iter = m_listUdpListeners.erase( iter );
				STL_ERASE( m_listUdpListeners, udp_iterator, iter );
			}
		}
		{
			for( con_iterator iter = m_listMsgConnections.begin(), eiter = m_listMsgConnections.end(); iter != eiter; /**/ ) {
				BBQMsgConnection * pConn = VAR_AT(iter);
				if( pConn ) {
					PTRACE(1, " Delete Message Connection: " << (DWORD)pConn << " -" << (DWORD)pConn->GetSocket());
					delete pConn;
				}
				//iter = m_listMsgConnections.erase( iter );
				STL_ERASE( m_listMsgConnections, con_iterator, iter );
			}
		}
	}

#ifdef SUPPORT_BIG_MESSAGE
	{
		// TODO: clean outdated segment sessions 
		LockIt safe( m_mutexMsgSegment );

		while( ! m_listMsgSegments.empty() ) {
			SegSession * pSession = m_listMsgSegments.front();
			m_listMsgSegments.pop_front();

			delete pSession;
		}
	}
#endif

	{
		LockIt safe( m_mutexNet );
		//LockIt safe( m_mutexMsgPost );
		while( m_listMsgPosts.size() > 0 ) {
			SIM_MSGPOST * pMsgPost = m_listMsgPosts.front();
			m_listMsgPosts.pop_front();

			// not create by new but malloc, so call free() instead of delete
			// delete pMsgPost;
			free( pMsgPost );
		}
	}

#ifdef ENCRYPT_MESSAGE
	{
		if( m_pEncBuffer ) free( m_pEncBuffer );
		m_pEncBuffer = NULL;
	}
#endif
}

bool BBQMsgTerminal::AttachTcpListener( PTCPSocket * pSock )
{
	LockIt safe( m_mutexNet );

	if( pSock->IsOpen() ) {
#ifdef USE_MAP_FOR_LIST
		WORD port = pSock->GetPort();
		tcp_iterator iter = m_listTcpListeners.find( port );
		if( iter != m_listTcpListeners.end() ) {
			if( VAR_AT(iter) ) delete VAR_AT(iter);
			VAR_AT(iter) = pSock;
		} else {
			m_listTcpListeners[ port ] = pSock;
		}
#else
		m_listTcpListeners.push_back( pSock );
#endif

		return true;
	}

	return false;
}

bool BBQMsgTerminal::DetachTcpListener( PTCPSocket * pSock )
{
	LockIt safe( m_mutexNet );

	for( tcp_iterator iter = m_listTcpListeners.begin(), eiter = m_listTcpListeners.end(); iter != eiter; iter ++ ) {
		if( VAR_AT(iter) == pSock ) {
			VAR_AT(iter) = NULL;
			return true;
		}
	}

	return false;
}

PTCPSocket * BBQMsgTerminal::FindTcpListener( WORD port )
{
	LockIt safe( m_mutexNet );

#ifdef USE_MAP_FOR_LIST
	tcp_iterator iter = m_listTcpListeners.find( port );
	if( iter != m_listTcpListeners.end() ) return VAR_AT(iter);
#else
	for( tcp_iterator iter = m_listTcpListeners.begin(), eiter = m_listTcpListeners.end(); iter != eiter; iter ++ ) {
		if( VAR_AT(iter) == NULL ) continue;

		PTCPSocket * pSock = VAR_AT(iter);
		if( (port == 0) || (port == pSock->GetPort()) ) {
			return pSock;
		}
	}
#endif

	return NULL;
}

PTCPSocket *  BBQMsgTerminal::ListenTcp( WORD port, unsigned int queueSize, bool bSeperateThread, const PIPSocket::Address & bind,bool bNonBlock )
{
	PTCPSocket * pSock = new PTCPSocket();

	DWORD ip = (DWORD) bind;
	PString strIp = bind ? BBQMsgTerminal::ToString(bind) : "*";
	if(	pSock->Listen( bind,  queueSize, port /*, PSocket::CanReuseAddress*/ ) ) {
	  int fd = pSock->GetHandle();
    if ( bNonBlock){
	  // set socket to non-block action
	    set_socket_nonblock( fd );//chenayuan
    }else
    { 
      SetSocketOptions(fd);
    }
		PString strMsg( PString::Printf, "Bind TCP port: %s:%d.", (const char *)strIp, port );
		//PTRACE( 1, "Bind TCP port: " << port );
		PLOG( m_pLogger, Debug, strMsg );
	} else {
		PString strMsg( PString::Printf, "TCP listen failed, port %s:%d seems in use.", (const char *)strIp, port );
		PLOG( m_pLogger, Error, strMsg );
		delete pSock;
		return NULL;
	}
	


//#ifdef _WIN32
//	DWORD fionbio = 1;
//	int ret = ::ioctlsocket( fd, FIONBIO, & fionbio );
//#else
//	int flags = fcntl( fd, F_GETFL, 0 );
//	int ret = fcntl( fd, F_SETFL, flags | O_NONBLOCK );
//	//int ret = fcntl( fd, F_SETFL, flags & (~O_NONBLOCK) );
//#endif

	if( bSeperateThread ) {
		new TcpListenerThread( *this, pSock );
	} else {
		AttachTcpListener( pSock );
	}

	return pSock;
}

void BBQMsgTerminal::StopListenTcp( WORD port )
{
	LockIt safe( m_mutexRequest );

	tth_iterator iter = m_listTcpListenerThreads.find( port );
	if( iter != m_listTcpListenerThreads.end() ) {
		TcpListenerThread * tth = VAR_AT(iter);
		if( tth ) tth->Close();

		// the thread to be waited will require the same mutex, so release here for a while
		UnlockIt yield( m_mutexRequest );
		delete tth;
	} else {
		PTCPSocket * pSock = FindTcpListener( port );
		if( pSock && DetachTcpListener(pSock) ) {
			delete pSock;
		}
	}
}

bool BBQMsgTerminal::AttachUdpListener( PUDPSocket * pSock )
{
	LockIt safe( m_mutexNet );

	if( pSock->IsOpen() ) {
		WORD port = pSock->GetPort();
		//PTRACE( 1, "Attaching UDP port: " << port );

		pSock->SetReadTimeout( PTimeInterval(0) );
		pSock->SetWriteTimeout( PTimeInterval(0) );

#ifdef USE_MAP_FOR_LIST
		udp_iterator iter = m_listUdpListeners.find( port );
		if( iter != m_listUdpListeners.end() ) {
			if( VAR_AT(iter) ) delete VAR_AT(iter);
			VAR_AT(iter) = pSock;
		} else {
			m_listUdpListeners[ port ] = pSock;
		}
#else
		m_listUdpListeners.push_back( pSock );
#endif

		return true;
	}

	return false;
}

bool BBQMsgTerminal::DetachUdpListener( PUDPSocket * pSock )
{
	if( ! pSock ) return false;

	LockIt safe( m_mutexNet );

	WORD port;
	port = pSock->GetPort();
	//PIPSocket::Address addr; 
	//pSock->GetLocalAddress( addr, port );
	//PTRACE( 1, "Detaching UDP port: " << port );
	PString strMsg( PString::Printf, "Detaching UDP port: %d.", port );
	PLOG( m_pLogger, Debug, strMsg );

	for( udp_iterator iter = m_listUdpListeners.begin(),eiter = m_listUdpListeners.end(); iter != eiter; iter ++ ) {
		if( VAR_AT(iter) == pSock ) {
			VAR_AT(iter) = NULL;
			return true;
		}
	}

	return false;
}

bool BBQMsgTerminal::ListenUdp( WORD port, const PIPSocket::Address & bind )
{
	PUDPSocket * pSock = new PUDPSocket();

	DWORD ip = (DWORD) bind;
	PString strIp = bind ? BBQMsgTerminal::ToString(bind) : "*";

	if(	pSock->Listen( bind, 0, port, PSocket::AddressIsExclusive ) && AttachUdpListener( pSock ) ) {
		PString strMsg( PString::Printf, "Bind UDP port: %s:%d.", (const char *)strIp, port );
		//PTRACE( 1, "Bind TCP port: " << port );
		PLOG( m_pLogger, Debug, strMsg );
	} else {
		PString strMsg( PString::Printf, "UDP listen failed, port %s:%d seems in use.", (const char *)strIp, port );
		PLOG( m_pLogger, Error, strMsg );
		delete pSock;
		return false;
	}

	return true;
}

void BBQMsgTerminal::StopListenUdp( WORD port )
{
	LockIt safe( m_mutexNet );

	udp_iterator iter = m_listUdpListeners.find( port );
	if( iter != m_listUdpListeners.end() ) {
		PUDPSocket * pSock = VAR_AT(iter);
		if( pSock ) pSock->Close();
	}
}

PUDPSocket * BBQMsgTerminal::FindUdpListener( WORD port )
{
	LockIt safe( m_mutexNet );

#ifdef USE_MAP_FOR_LIST
	udp_iterator iter = m_listUdpListeners.find( port );
	if( iter != m_listUdpListeners.end() ) return VAR_AT(iter);
#else
	for( udp_iterator iter = m_listUdpListeners.begin(), eiter = m_listUdpListeners.end(); iter != eiter; iter ++ ) {
		if( VAR_AT(iter) == NULL ) continue;

		PUDPSocket * pSock = VAR_AT(iter);
		if( (port == 0) || (port == pSock->GetPort()) ) {
			return pSock;
		}
	}
#endif

	return NULL;
}

bool BBQMsgTerminal::AttachMsgConnection( BBQMsgConnection * pConn )
{
	LockIt safe( m_mutexNet );

	PTCPSocket * pSock = pConn->GetSocket();

	if( pSock->IsOpen() ) {
		PTimeInterval timeout(0);
		pSock->SetReadTimeout(timeout);
		pSock->SetWriteTimeout(timeout);

#ifdef USE_MAP_FOR_LIST
		int fd = pSock->GetHandle();
		con_iterator iter = m_listMsgConnections.find( fd );
		if( iter != m_listMsgConnections.end() ) {
			if( VAR_AT(iter) ) {
				//PString strMsg( PString::Printf, "Delete Message Connection: %X - %X", (DWORD)(VAR_AT(iter)), (DWORD)(VAR_AT(iter))->GetSocket());
				//PLOG( m_pLogger, Debug, strMsg );
				PTRACE(1, " Delete Message Connection: " << (DWORD)(VAR_AT(iter)) << " -" << (DWORD)(VAR_AT(iter))->GetSocket());
				delete VAR_AT(iter);
			}
			VAR_AT(iter) = pConn;
		} else {
			m_listMsgConnections[ fd ] = pConn;
		}
#else
		m_listMsgConnections.push_back( pConn );
#endif

		//if( GetThreadId() != GetCurrentThreadId() ) {
		//	if( IsSuspended() ) Resume();
		//}

		return true;
	}

	return false;
}

bool BBQMsgTerminal::DetachMsgConnection( BBQMsgConnection * pConn )
{
	LockIt safe( m_mutexNet );

	for( con_iterator iter = m_listMsgConnections.begin(),eiter = m_listMsgConnections.end(); iter != eiter; iter ++ ) {
		if( VAR_AT(iter) == pConn ) {
			VAR_AT(iter) = NULL;
			return true;
		}
	}

	return false;
}

void BBQMsgTerminal::SetConnectTimeout( DWORD dwTime )
{
	m_dwConnectTimeout = dwTime;
}

int BBQMsgTerminal::Connect( PIPSocket::Address addr, WORD port, DWORD dwTimeout )
{
	BBQMsgConnection * pConn = new BBQMsgConnection( false );
	PTCPSocket * pSock = pConn->GetSocket();

	if( dwTimeout == 0 ) dwTimeout = m_dwConnectTimeout;
	pSock->SetReadTimeout( dwTimeout ); // set connect timeout

	pSock->SetPort( port );
	if( pSock->Connect( 0, addr ) && AttachMsgConnection( pConn ) ) {

		OnMsgConnectionOpen( pConn );
		return pSock->GetHandle();
	} else {
		delete pConn;
	}

	return 0;
}

int BBQMsgTerminal::Connect( const PString & addr, WORD port, DWORD dwTimeout )
{
	BBQMsgConnection * pConn = new BBQMsgConnection( false );
	PTCPSocket * pSock = pConn->GetSocket();

	if( dwTimeout == 0 ) dwTimeout = m_dwConnectTimeout;
	pSock->SetReadTimeout( dwTimeout ); // set connect timeout

	pSock->SetPort( port );
	if( pSock->Connect( addr ) && AttachMsgConnection( pConn ) ) {

		OnMsgConnectionOpen( pConn );
		return pSock->GetHandle();
	} else {
		delete pConn;
	}

	return 0;
}

bool BBQMsgTerminal::ValidateTcpChannel( SIM_CHANNEL & chan )
{
	// check current connection
	if( chan.socket ) {
		BBQMsgConnection * pConn = FindMsgConnection(0,0,chan.socket);
		if( pConn ) {
			fd_set fdsetRead;
			FD_ZERO( & fdsetRead );
			FD_SET( chan.socket, & fdsetRead );
			struct timeval tval = { 0, 0 };
			int s = select( chan.socket +1, & fdsetRead, NULL, NULL, &tval);
			if( s >= 0 ) return true;
			else {
				if( DetachMsgConnection( pConn ) ) {
					PTRACE(1, " Delete Message Connection: " << (DWORD)pConn << " -" << (DWORD)pConn->GetSocket());
					delete pConn;
				}
			}
		}
	}
	
	// else reconnect
	if( (chan.socket = Connect( (PIPSocket::Address) chan.peer.ip, chan.peer.port )) != 0 ) return true;
	
	return false;
}

void BBQMsgTerminal::CloseMsgConnection( DWORD ip, WORD port, int fd )
{
	LockIt safe( m_mutexNet );

	PIPSocket::Address peer_addr;
	WORD peer_port;

#ifdef USE_MAP_FOR_LIST
	if( fd > 0 ) {
		con_iterator iter = m_listMsgConnections.find( fd );
		if( iter != m_listMsgConnections.end() ) {
			BBQMsgConnection * pConn = VAR_AT(iter);
			if( pConn ) {
				PTCPSocket * pSock = pConn->GetSocket();
				if( pSock ) pSock->Close();
			}
		}
	} else 
#endif
	for( con_iterator iter = m_listMsgConnections.begin(), eiter = m_listMsgConnections.end(); iter != eiter; iter ++ ) {
		BBQMsgConnection * pConn = VAR_AT(iter);
		if( pConn ) {
			PTCPSocket * pSock = pConn->GetSocket();
			if( pSock ) {
				if( fd == pSock->GetHandle() ) {
					pSock->Close();
					return;
				} else if( pSock->GetPeerAddress( peer_addr, peer_port ) ) {
					if( (ip == peer_addr) && ( port == 0 || port == peer_port ) && ( fd == 0 || fd == pSock->GetHandle() ) ) {
						pSock->Close();
						return;
					}
				}
			}
		}
	}

	closesocket( fd );
}

BBQMsgConnection * BBQMsgTerminal::FindMsgConnection( DWORD ip, WORD port, int fd )
{
	LockIt safe( m_mutexNet );

	PIPSocket::Address peer_addr;
	WORD peer_port;

#ifdef USE_MAP_FOR_LIST
	if( fd > 0 ) {
		con_iterator iter = m_listMsgConnections.find( fd );
		if( iter != m_listMsgConnections.end() ) {
			return VAR_AT(iter);
		}
	} else 
#endif
	for( con_iterator iter = m_listMsgConnections.begin(), eiter = m_listMsgConnections.end(); iter != eiter; iter ++ ) {
		BBQMsgConnection * pConn = VAR_AT(iter);
		if( pConn ) {
			PTCPSocket * pSock = pConn->GetSocket();
			if( pSock ) {
				if( fd == pSock->GetHandle() ) {
					return pConn;
				} else if( pSock->GetPeerAddress( peer_addr, peer_port ) ) {
					if( (ip == peer_addr) && ( port == 0 || port == peer_port ) && ( fd == 0 || fd == pSock->GetHandle() ) ) {
						return pConn;
					}
				}
			}
		}
	}

	return NULL;
}


int BBQMsgTerminal::PrepareSelect( fd_set * pfdsetRead, fd_set * pfdsetWrite )
{
	LockIt safe( m_mutexNet );

	int fdmax = 0;

	FD_ZERO( pfdsetRead );
	FD_ZERO( pfdsetWrite );

	for( tcp_iterator iter = m_listTcpListeners.begin(), eiter = m_listTcpListeners.end(); iter != eiter; /**/ ) {
		PTCPSocket * pSock = VAR_AT(iter);

		if( pSock == NULL ) {
			//iter = m_listTcpListeners.erase( iter );
			STL_ERASE( m_listTcpListeners, tcp_iterator, iter );
			continue;
		} else if ( ! pSock->IsOpen() ) {
			delete pSock;
			//iter = m_listTcpListeners.erase( iter );
			STL_ERASE( m_listTcpListeners, tcp_iterator, iter );
			continue;
		}

		int fd = pSock->GetHandle();
		FD_SET( fd, pfdsetRead );
		if( fd > fdmax ) fdmax = fd;

		iter ++;
	}
	
	{
	for( udp_iterator iter = m_listUdpListeners.begin(), eiter = m_listUdpListeners.end(); iter != eiter; /**/ ) {
		PUDPSocket *pSock = VAR_AT(iter);

		if( pSock == NULL ) {
			//iter = m_listUdpListeners.erase( iter );
			STL_ERASE( m_listUdpListeners, udp_iterator, iter );
			continue;
		} else if ( ! pSock->IsOpen() ) {
			//delete pSock;
			//iter = m_listUdpListeners.erase( iter );
			iter ++;
			continue;
		}

		int fd = pSock->GetHandle();
		FD_SET( fd, pfdsetRead );
		if( fd > fdmax ) fdmax = fd;

		iter ++;
	}
	}
	{
	for( con_iterator iter = m_listMsgConnections.begin(), eiter = m_listMsgConnections.end(); iter != eiter; /**/ ) {
		BBQMsgConnection * pConn = VAR_AT(iter);

		if( pConn == NULL ) {
			//iter = m_listMsgConnections.erase( iter );
			STL_ERASE( m_listMsgConnections, con_iterator, iter );
			continue;
		} else {
			PTCPSocket * pSock = pConn->GetSocket();

			if( pSock->IsOpen() ) {
				// if idle check required, then kick those idle connection
				if( m_dwConnectionIdleMax && pConn->IsIdle(m_dwConnectionIdleMax) ) {

					PIPSocket::Address addr; WORD port;
					pSock->GetPeerAddress( addr, port );
					PString str( PString::Printf, "Connection %s %s:%d idle too long, kick.", (pConn->IsIncoming() ? "from" : "to"), (const char *) ToString((DWORD) addr), port );
					PLOG( m_pLogger, Info, str );

					if( OnMsgConnectionIdle( pConn ) ) {
						OnMsgConnectionClose( pConn );
						pSock->Close();
					}
				}
			}

			if( ! pSock->IsOpen() ) {
				if( pConn->m_bAutoDelete ) {
					//iter = m_listMsgConnections.erase( iter );
					STL_ERASE( m_listMsgConnections, con_iterator, iter );
					PTRACE(1, " Delete Message Connection: " << (DWORD)pConn << " -" << (DWORD)pConn->GetSocket());
					delete pConn;
					continue;
				} else {
					// it will be detach and delete by others
				}
			} else {
				int fd = pSock->GetHandle();
				FD_SET( fd, pfdsetRead );
				if( pConn->HasDataToWrite() ) FD_SET( fd, pfdsetWrite );
				if( fd > fdmax ) fdmax = fd;
			}

			iter ++;
		}
	}
	}
	return fdmax;
}

void BBQMsgTerminal::HandleMsgConnectionsEvent( fd_set * pfdsetRead, fd_set * pfdsetWrite )
{
	LockIt safe( m_mutexNet );

	for( con_iterator iter = m_listMsgConnections.begin(), eiter = m_listMsgConnections.end(); iter != eiter; iter ++ ) {
		BBQMsgConnection * pConn = VAR_AT(iter);

		if( ! pConn ) continue;

		PTCPSocket * pSock = pConn->GetSocket();
		int fd = pSock->GetHandle();

		// flush data to write if there is
		if( FD_ISSET( fd, pfdsetWrite ) ) {
			pConn->FlushMessage();
		}

#ifdef BLOCK_SOCKET_WHEN_QUEUE_FULL
		if( m_listRequests.size() >= m_nRequestQueueMax ) {
			// request queue already full,
			// we block the data in socket buffer.
			PTRACE( 1, "queue full!!!!!" );
			continue;
		}
#endif

		// receive data and handle message if there is
		if( FD_ISSET( fd, pfdsetRead ) ) {
			PIPSocket::Address		my_addr, peer_addr;
			WORD					my_port, peer_port;
			pSock->GetPeerAddress( peer_addr, peer_port );
			pSock->GetLocalAddress( my_addr, my_port );

			SIM_REQUEST req;
			req.channel.socket = fd;
			req.channel.peer.ip = (DWORD) peer_addr;
			req.channel.peer.port = peer_port;
			req.channel.local.ip = (DWORD) my_addr;
			req.channel.local.port = my_port;

			while( (VAR_AT(iter) == pConn) && pConn->ReadMessage( & req.msg ) ) {
#ifdef ENCRYPT_MESSAGE
				if( m_bEncryptMessage&&req.msg.simHeader.id!= UCM_CM_CONNECT ) {
					unsigned char myDec[1500]={0};
          AES_decrypt( (unsigned char *) req.msg.simData, req.msg.simHeader.size, (unsigned char *) m_AESkey, (unsigned char *) PI_SQRT2_STRING_EX, 128, (unsigned char *)  myDec ) ;
          memcpy(req.msg.simData, myDec, req.msg.simHeader.size);
				}
#endif
				OnMessageArriveInternal( & req );
			}
			
			if( VAR_AT(iter) == pConn ) {

				int rd = pSock->GetLastReadCount();
				int err = pSock->GetErrorCode( PChannel::LastReadError );

				if( rd > 0 ) {
					// impossible, because we read until no message arrive
				} else if( (rd==0) && (err == PChannel::Timeout) ) {
					// no data coming 
				} else {
					// read 0 bytes (closed), or other error 

					PIPSocket::Address addr; WORD port;
					pSock->GetPeerAddress( addr, port );

					MS_TIME msNow = GetHostUpTimeInMs();

					int nErr = pSock->GetErrorNumber( PChannel::LastReadError );
					PString strErr = pSock->GetErrorText( PChannel::LastReadError );
					PString strErrInfo = PString( PString::Printf, "error: %d, %d, %s", err, nErr, (const char *) strErr );

					PString strMsg( PString::Printf, "Connection %s %s:%d closed (idle time: %d ms), %s.", 
						(pConn->IsIncoming() ? "from" : "to"), (const char *)ToString( addr ), port, 
						(int)(msNow - pConn->GetLastActionTime()),
						(const char *) strErrInfo );
					//PTRACE( 1, strMsg );
					PLOG( m_pLogger, Info, strMsg );

					OnMsgConnectionClose( pConn );
					pSock->Shutdown(PChannel::ShutdownReadAndWrite); // PrepareSelect(...) will remove it from list
					pSock->Close(); // PrepareSelect(...) will remove it from list
				}
			}
		}
	}
}

void BBQMsgTerminal::HandleUdpListenersEvent( fd_set * pfdsetRead, fd_set * pfdsetWrite )
{
	LockIt safe( m_mutexNet );

	PIPSocket::Address		my_addr, peer_addr;
	WORD					my_port, peer_port;

	SIM_REQUEST req;
	SIM_CHANNEL * pChannel = & req.channel;
	SFIDMSGHEADER * pHeader = & req.msg.simHeader;


	for( udp_iterator iter = m_listUdpListeners.begin(), eiter = m_listUdpListeners.end(); iter != eiter; iter ++ ) {

		PUDPSocket * pSock = VAR_AT(iter);
		if( ! pSock ) continue;

		int fd = pSock->GetHandle();

#ifdef BLOCK_SOCKET_WHEN_QUEUE_FULL
		if( m_listRequests.size() >= m_nRequestQueueMax ) {
			// request queue already full,
			// we block the data in socket buffer.
			PTRACE( 1, "queue full!!!!!" );
			continue;
		}
#endif

		if( FD_ISSET( fd, pfdsetRead ) ) {

			while( (VAR_AT(iter) == pSock) && pSock->ReadFrom( & req.msg, sizeof(req.msg), peer_addr, peer_port ) ) {
				int n = pSock->GetLastReadCount();
				if( (n == (int)sizeof(SFIDMSGHEADER) + pHeader->size) && (pHeader->magic == SIM_MAGIC) ) {

					pSock->GetLocalAddress( my_addr, my_port );

					pChannel->socket = 0;
					pChannel->peer.ip = (DWORD) peer_addr;
					pChannel->peer.port = peer_port;
					pChannel->local.ip = 0;
					pChannel->local.port = my_port;

#ifdef ENCRYPT_MESSAGE
					if( m_bEncryptMessage ) {
						unsigned char myDec[1500]={0};
            AES_decrypt( (unsigned char *) req.msg.simData, req.msg.simHeader.size, (unsigned char *) m_AESkey, (unsigned char *) PI_SQRT2_STRING_EX, 128, (unsigned char *)  myDec ) ;
            memcpy(req.msg.simData, myDec, req.msg.simHeader.size);
					}
#endif

#ifdef SUPPORT_BIG_MESSAGE
					if( req.msg.simHeader.id == SIM_SEGMENT )
						OnMessageSegmentArriveInternal( & req );
					else
#endif
						OnMessageArriveInternal( & req );
				} else {
					// bad udp packet coming, drop
					PTRACE( 3, "bad incoming udp pakcet, drop." );
				}
			}
		}
	}
}

bool BBQMsgTerminal::AcceptNewConnection( PTCPSocket * pSock )
{
	PTCPSocket * pNewSock = new PTCPSocket();
 // SetSocketOptions(pNewSock->GetHandle());
	if( pNewSock->Accept( * pSock ) ) {
    //pNewSock->SetOption(TCP_NODELAY, 1, IPPROTO_TCP);
    SetSocketOptions(pNewSock->GetHandle());
		if( m_nConnectionMax && (m_listMsgConnections.size() < m_nConnectionMax) ) {
      
			BBQMsgConnection * pNewCon = new BBQMsgConnection( true, pNewSock );
			AttachMsgConnection( pNewCon );
			OnMsgConnectionOpen( pNewCon );

			return true;

		} else {
			PIPSocket::Address addr; WORD port;
			pNewSock->GetPeerAddress( addr, port );
			PString strMsg( PString::Printf, "Connection reach max limit %d, deny new connection from %s:%d.", m_nConnectionMax, (const char *) ToString((DWORD)addr), port );
			PLOG( m_pLogger, Info, strMsg );

			delete pNewSock; // connection reach the max limit, deny

			return true;
		}

	} else {

		int errCode = pNewSock->GetErrorCode( PChannel::LastGeneralError );
		int errNumber = pNewSock->GetErrorNumber( PChannel::LastGeneralError );
		PString errString = pNewSock->GetErrorText( PChannel::LastGeneralError ); 

		delete pNewSock;	// accept failed
	}

	return false;
}

void BBQMsgTerminal::HandleTcpListenersEvent( fd_set * pfdsetRead, fd_set * pfdsetWrite )
{
	LockIt safe( m_mutexNet );

	for( tcp_iterator iter = m_listTcpListeners.begin(), eiter = m_listTcpListeners.end(); iter != eiter; iter ++ ) {
		PTCPSocket * pSock = VAR_AT(iter);
		if( pSock ) {
			int fd = pSock->GetHandle();
			if( FD_ISSET( fd, pfdsetRead ) ) AcceptNewConnection( pSock );
		}
	}
}

#ifdef FD_SETSIZE
#if FD_SETSIZE == 64
#error large fd_set should be used to support large number of sockets.
#endif
#endif

void BBQMsgTerminal::LoopNetIO( void )
{
	while( ! m_bClosing ) {
		sleep_ms(0);

		fd_set fdsetRead, fdsetWrite;

		// prepare the select list, suspend running if no fd in the list
		int fdmax = PrepareSelect( & fdsetRead, & fdsetWrite );
		if( fdmax <= 0 ) {
			// impossible, at least we have 1 or 2 port listener
			sleep_ms(1);
			continue;
		}

		// do a select to wait I/O happen
		struct timeval tval = { 0, 200000 };
		int ret = ::select( fdmax+1, & fdsetRead, & fdsetWrite, NULL, &tval);
		if( ret < 0 ) {
			PTRACE_SOCKET_ERROR();
			sleep_ms(1);
			continue;
		} else if( ret == 0 ) {
			continue;
		}

		// handle msg connections
		HandleMsgConnectionsEvent( & fdsetRead, & fdsetWrite );

		// handle udp listeners
		HandleUdpListenersEvent( & fdsetRead, & fdsetWrite );

		// handle tcp listeners
		HandleTcpListenersEvent( & fdsetRead, & fdsetWrite );

		//sleep_ms(0);
	}
}

void BBQMsgTerminal::Main( void )
{
	SetThreadName( "" );

	LoopNetIO();
}

#ifdef SUPPORT_BIG_MESSAGE
int BBQMsgTerminal::SetUdpPacketSizeLimit( int nBytes )
{
	WORD old_size = m_nUDPDataLimit + (20 + 8);

	WORD new_size = (WORD) (nBytes - (20 + 8));

	m_nUDPDataLimit = min( new_size, SIM_DATABLOCKSIZEMAX );

	return old_size;
}
#endif

PString BBQMsgTerminal::ToString( DWORD ip )
{
	return PString( inet_ntoa( * (in_addr*) & ip ) );
	//return (PString) (PIPSocket::Address) ip;
}

PString BBQMsgTerminal::ToString( const IPSOCKADDR * pAddr )
{
	return PString( PString::Printf, "%s:%d", inet_ntoa( * (in_addr*) & pAddr->ip ), pAddr->port );
	//return PString( PString::Printf, "%s:%d", (const char *) ToString(pAddr->ip), pAddr->port );
}

PString BBQMsgTerminal::ToString( const SIM_CHANNEL * pChannel, bool out )
{
	IPSOCKADDR local = pChannel->local, peer = pChannel->peer;

	if( pChannel->socket ) {
		struct sockaddr_in local_addr, peer_addr;
		socklen_t local_len = sizeof(local_addr), peer_len = sizeof(peer_addr);

		if( 0 == getsockname( pChannel->socket, (struct sockaddr *) & local_addr, & local_len ) ) {
			local.ip = local_addr.sin_addr.s_addr;
			local.port = ntohs( local_addr.sin_port );
		}

		if( 0 == getpeername( pChannel->socket, (struct sockaddr *) & peer_addr, & peer_len ) ) {
			peer.ip = peer_addr.sin_addr.s_addr;
			peer.port = ntohs( peer_addr.sin_port );
		}
	}

	return PString( PString::Printf, "%d (%s:%d) %s (%s:%d)",
		pChannel->socket, 
		(const char *) ToString(local.ip), local.port, 
		(out ? "-->" : "<--"),
		(const char *) ToString(peer.ip), peer.port );
}

DWORD BBQMsgTerminal::HostnameToIp( const char * lpszHostname )
{
	DWORD ip = 0;

	hostent * ent = gethostbyname( lpszHostname );
	if( ent && ent->h_addr_list[0] ) {
		struct in_addr first_ip = * ((struct in_addr *) ent->h_addr_list[0]);
		ip = first_ip.s_addr;
	}

	return ip;
}

bool BBQMsgTerminal::HostnameToIp( const char * lpszHostname, DWORD & ip )
{
	hostent * ent = gethostbyname( lpszHostname );
	if( ent && ent->h_addr_list[0] ) {
		struct in_addr first_ip = * ((struct in_addr *) ent->h_addr_list[0]);
		ip = first_ip.s_addr;

		return true;
	}

	return false;
}

#if 0
bool BBQMsgTerminal::SendMessageInternal( const SIM_CHANNEL * pChannel, const SIM_MESSAGE * msg )
{
	bool bDone = false;

	PAssert( msg->simHeader.sync, "msg->simHeader.sync must be set" );
	PTRACE( 3, PString( PString::Printf, "send msg: %s, sync: %d, %d bytes,\n\t%s.", 
		SIM_MSGNAME( msg->simHeader.id ), msg->simHeader.sync, SIM_PMSGSIZE(msg), 
		(const char *) ToString( pChannel, true ) ) );

	LockIt safe( m_mutexNet );

	if( pChannel->socket != 0 ) {
		BBQMsgConnection * pConn = NULL;

#ifdef USE_MAP_FOR_LIST
		con_iterator iter = m_listMsgConnections.find( pChannel->socket );
		if( iter != m_listMsgConnections.end() ) {
			pConn = VAR_AT(iter);
		}
#else
		for( con_iterator iter = m_listMsgConnections.begin(), eiter = m_listMsgConnections.end(); iter != eiter; iter ++ ) {
			pConn = VAR_AT(iter);
			if( ! pConn ) continue;
			PTCPSocket * pSock = pConn->GetSocket();
			if( pChannel->socket == pSock->GetHandle() ) {
				break;
			}
		}
#endif
		if( pConn ) {
			if( msg->simHeader.id == SIM_NONE ) {
				PTCPSocket * pSock = pConn->GetSocket();
				pSock->Close(); // PrepareSelect(...) will remove it from list
				bDone = true;
			} else {
				bDone = pConn->WriteMessage( msg );
			}
			return bDone;
		} else {
			//PTRACE( 1, "Error: specified TCP socket not found for sending message." );
			PLOG( m_pLogger, Debug, "Error: specified TCP socket not found for sending message." );
			return false;
		}
	} 
	
	else

	{
		PUDPSocket * pSock = NULL;

		if( pChannel->local.port != 0 ) {
#ifdef USE_MAP_FOR_LIST
			udp_iterator iter = m_listUdpListeners.find( pChannel->local.port );
			if( iter != m_listUdpListeners.end() ) {
				pSock = VAR_AT(iter);
			}
#else
			PIPSocket::Address		my_addr;
			WORD					my_port;
			for( udp_iterator iter = m_listUdpListeners.begin(), eiter = m_listUdpListeners.end(); iter != eiter; iter ++ ) {
				if( VAR_AT(iter) ) {
					PUDPSocket * p = VAR_AT(iter);
					p->GetLocalAddress( my_addr, my_port );
					if( pChannel->local.port == my_port ) {
						pSock = p;
						break;
					}
				}
			}
#endif
		} else {
			if( m_listUdpListeners.size() > 0 ) {
				udp_iterator iter = m_listUdpListeners.begin();
				pSock = VAR_AT(iter);
			}
		}

		if( pSock ) {
			WORD msg_size = SIM_PMSGSIZE(msg);

#ifdef SUPPORT_BIG_MESSAGE
			if( msg_size <= m_nUDPDataLimit ) {

				bDone = (FALSE != pSock->WriteTo( msg, msg_size, pChannel->peer.ip, pChannel->peer.port/*, TRUE*/ ));
      //ADD BY CHEN YUAN BEGIN
      if (!bDone)
      {
	      PTRACE( 3, PString( PString::Printf, "BBQMsgTerminal::SendMessageInternal returned false, (msgname=%s, msgsize=%d,  local.ip=%s, local.port=%d, peer.ip=%s, peer.port=%d) errcode=%d, errnumber=%d, errmsg=%s,\n   %s.", 
          SIM_MSGNAME(  msg->simHeader.id ),  msg_size, (const char *) ToString(pChannel->local.ip), pChannel->local.port, (const char *) ToString(pChannel->peer.ip),pChannel->peer.port, (int)pSock->GetErrorCode(), (int)pSock->GetErrorNumber(), (const char*)pSock->GetErrorText(), 
		      (const char *) ToString( pChannel, true ) ) );
      }
      //ADD BY CHEN YUAN END
      } else {

				WORD extra_size = sizeof(SIMD_SEGMENT_HEADER) + sizeof(SFIDMSGHEADER);
				WORD unit_size = m_nUDPDataLimit - extra_size;
				WORD n = ((msg_size-1) / unit_size) + 1;

				SIM_MESSAGE seg;
				SIM_MSGINIT( seg, SIM_SEGMENT, 0 );
				SIMD_SEGMENT_HEADER	* pHeader = (SIMD_SEGMENT_HEADER *) & seg.simData;
				pHeader->sync = m_nSegSyncSerial ++;
				pHeader->byte_total = msg_size;
				pHeader->segment_total = n;

				const char * pSrc = (const char *) msg;
				char * pDest = (char *) & seg.simData[ sizeof(SIMD_SEGMENT_HEADER) ];

				for( WORD i=0, j=0; i<n; i++ ) {
					WORD datasize = ((i<n-1) ? unit_size : (msg_size - unit_size * i ) /*(msg_size % unit_size)*/);
					memcpy( pDest, pSrc + j, datasize );
					seg.simHeader.size = sizeof(SIMD_SEGMENT_HEADER) + datasize;
					pHeader->segment_offset = i;
					pHeader->byte_offset = j;

					pSock->WriteTo( & seg, SIM_MSGSIZE(seg), pChannel->peer.ip, pChannel->peer.port/*, TRUE*/ );

					j = j + datasize;
				}
			}
#else
			bDone = (FALSE != pSock->WriteTo( msg, msg_size, pChannel->peer.ip, pChannel->peer.port/*, TRUE*/ ));
      //ADD BY CHEN YUAN BEGIN
      if (!bDone)
      {
	      PTRACE( 3, PString( PString::Printf, "BBQMsgTerminal::SendMessageInternal returned false, (msgname=%s, msgsize=%d, peer.ip=%s, peer.port=%d) errcode=%d, errnumber=%d, errmsg=%s,\n   %s.", 
          SIM_MSGNAME(  msg->simHeader.id ),  msg_size, (const char *) ToString(pChannel->peer.ip), pChannel->peer.port, (int)pSock->GetErrorCode(), (int)pSock->GetErrorNumber(), (const char*)pSock->GetErrorText(), 
		      (const char *) ToString( pChannel, true ) ) );
      }
      //ADD BY CHEN YUAN END
#endif // SUPPORT_BIG_MESSAGE
		}
	}

	return bDone;
}
#endif	// WZG: no one used

SIM_MESSAGE * BBQMsgTerminal::EncryptMessage( const SIM_MESSAGE * plain_msg )
{
	SIM_MESSAGE * msg = NULL;

#ifdef ENCRYPT_MESSAGE
	if( m_bEncryptMessage ) {
		int nBufSize = sizeof(plain_msg->simHeader) + plain_msg->simHeader.size;
		if( nBufSize > m_nEncBufferSize ) {
			if( m_pEncBuffer ) {
				free( m_pEncBuffer );
				m_pEncBuffer = NULL;
			}

			m_nEncBufferSize = nBufSize;
			m_pEncBuffer = (char *) malloc( m_nEncBufferSize );
		}

		msg = (SIM_MESSAGE *) m_pEncBuffer;
		if( ! msg ) return (SIM_MESSAGE *) plain_msg;

		msg->simHeader = plain_msg->simHeader;
		uint32 * psrc = (uint32 *) & plain_msg->simData;
		uint32 * pdest = (uint32 *) & msg->simData;
    //chen yuan add 
    AES_encrypt( (unsigned char *) psrc, plain_msg->simHeader.size, (unsigned char *) m_AESkey, (unsigned char *) PI_SQRT2_STRING_EX, 128, (unsigned char *)  pdest );
		//int n = (plain_msg->simHeader.size + 3) >> 2; // use 32 bit integer to do it faster
		//while( n -- > 0 ) {
		//	* pdest ++ = ~ (* psrc ++);
		//}
	} else {
		msg = (SIM_MESSAGE *) plain_msg;
	}
#else
	msg = (SIM_MESSAGE *) plain_msg;
#endif

	return msg;
}

SIM_MESSAGE * BBQMsgTerminal::DecryptMessage( const SIM_MESSAGE * enc_msg )
{
	SIM_MESSAGE * msg = NULL;

#ifdef ENCRYPT_MESSAGE
	if( m_bEncryptMessage ) {
		int nBufSize = sizeof(enc_msg->simHeader) + enc_msg->simHeader.size;
		if( nBufSize > m_nEncBufferSize ) {
			if( m_pEncBuffer ) {
				free( m_pEncBuffer );
				m_pEncBuffer = NULL;
			}

			m_nEncBufferSize = nBufSize;
			m_pEncBuffer = (char *) malloc( m_nEncBufferSize );
		}

		msg = (SIM_MESSAGE *) m_pEncBuffer;
		if( ! msg ) return (SIM_MESSAGE *) enc_msg;

		msg->simHeader = enc_msg->simHeader;
		uint32 * psrc = (uint32 *) & enc_msg->simData;
		uint32 * pdest = (uint32 *) & msg->simData;
		int n = (enc_msg->simHeader.size + 3) >> 2; // use 32 bit integer to do it faster
		while( n -- > 0 ) {
			* pdest ++ = ~ (* psrc ++);
		}
	} else {
		msg = (SIM_MESSAGE *) enc_msg;
	}
#else
	msg = (SIM_MESSAGE *) enc_msg;
#endif

	return msg;
}

// TODO: this function need to be optimized, 
// if there are lots of socket in the list, the for loop will be slow
// should use std::map<>
// for tcp, use std::map<fd, BBQMsgConnection *>
// for udp, use std::map<port, PUDPSocket *>

bool BBQMsgTerminal::SendMessageInternal( const SIM_REQUEST * plain_req )
{
	bool bDone = false;

	PAssert( plain_req->msg.simHeader.sync, "plain_req->msg.simHeader.sync must be set" );
	PTRACE( 3, PString( PString::Printf, "send msg: %s, sync: %d, %d bytes, \n\t%s",
		SIM_MSGNAME(plain_req->msg.simHeader.id), plain_req->msg.simHeader.sync, SIM_MSGSIZE(plain_req->msg), (const char *) ToString( & plain_req->channel, true ) ) );

	LockIt safe( m_mutexNet );

	const SIM_CHANNEL * pChannel = & plain_req->channel;

	if( pChannel->socket != 0 ) {
		BBQMsgConnection * pConn = NULL;

#ifdef USE_MAP_FOR_LIST
		con_iterator iter = m_listMsgConnections.find( pChannel->socket );
		if( iter != m_listMsgConnections.end() ) {
			pConn = VAR_AT(iter);
		}
#else
		for( con_iterator iter = m_listMsgConnections.begin(), eiter = m_listMsgConnections.end(); iter != eiter; iter ++ ) {
			pConn = VAR_AT(iter);
			if( ! pConn ) continue;
			PTCPSocket * pSock = pConn->GetSocket();
			if( pChannel->socket == pSock->GetHandle() ) {
				break;
			}
		}
#endif
		if( pConn ) {
			if( plain_req->msg.simHeader.id == SIM_NONE ) {
				PTCPSocket * pSock = pConn->GetSocket();
				pSock->Close(); // PrepareSelect(...) will remove it from list
				bDone = true;
			} else {
        if (UCM_MC_CONNECT != plain_req->msg.simHeader.id)
  				bDone = pConn->WriteMessage( EncryptMessage( & plain_req->msg ) );
        else
  				bDone = pConn->WriteMessage( ( & plain_req->msg ) );
			}
			return bDone;
		} else {
			//PTRACE( 1, "Error: specified TCP socket not found for sending message." );
			PLOG( m_pLogger, Debug, "Error: specified TCP socket not found for sending message." );
			return false;
		}
	}

	else

	{
		PUDPSocket * pSock = NULL;

		if( pChannel->local.port != 0 ) {
#ifdef USE_MAP_FOR_LIST
			udp_iterator iter = m_listUdpListeners.find( pChannel->local.port );
			if( iter != m_listUdpListeners.end() ) {
				pSock = VAR_AT(iter);
			}
#else
			PIPSocket::Address		my_addr;
			WORD					my_port;
			for( udp_iterator iter = m_listUdpListeners.begin(), eiter = m_listUdpListeners.end(); iter != eiter; iter ++ ) {
				if( VAR_AT(iter) ) {
					PUDPSocket * p = VAR_AT(iter);
					p->GetLocalAddress( my_addr, my_port );
					if( pChannel->local.port == my_port ) {
						pSock = p;
						break;
					}
				}
			}
#endif
		} else {
			if( m_listUdpListeners.size() > 0 ) {
				udp_iterator iter = m_listUdpListeners.begin();
				pSock = VAR_AT(iter);
			}
		}

		if( pSock ) {
			WORD msg_size = SIM_MSGSIZE( plain_req->msg );

#ifdef SUPPORT_BIG_MESSAGE
			if( msg_size <= m_nUDPDataLimit ) {

				bDone = (FALSE != pSock->WriteTo( EncryptMessage( & plain_req->msg ), msg_size, pChannel->peer.ip, pChannel->peer.port/*, TRUE*/ ));
      //ADD BY CHEN YUAN BEGIN
      if (!bDone)
      {
	      PTRACE( 3, PString( PString::Printf, "BBQMsgTerminal::SendMessageInternal returned false, (msgname=%s, msgsize=%d,  local.ip=%s, local.port=%d, peer.ip=%s, peer.port=%d) errcode=%d, errnumber=%d, errmsg=%s,\n   %s.", 
          SIM_MSGNAME(   plain_req->msg.simHeader.id ),  msg_size, (const char *) ToString(pChannel->local.ip), pChannel->local.port, (const char *) ToString(pChannel->peer.ip),pChannel->peer.port, (int)pSock->GetErrorCode(), (int)pSock->GetErrorNumber(), (const char*)pSock->GetErrorText(), 
		      (const char *) ToString( pChannel, true ) ) );
      }
      //ADD BY CHEN YUAN END
			} else {

				WORD extra_size = sizeof(SIMD_SEGMENT_HEADER) + sizeof(SFIDMSGHEADER);
				WORD unit_size = m_nUDPDataLimit - extra_size;
				WORD n = ((msg_size-1) / unit_size) + 1;

				SIM_MESSAGE seg;
				SIM_MSGINIT( seg, SIM_SEGMENT, 0 );
				SIMD_SEGMENT_HEADER	* pHeader = (SIMD_SEGMENT_HEADER *) & seg.simData;
				pHeader->sync = m_nSegSyncSerial ++;
				pHeader->byte_total = msg_size;
				pHeader->segment_total = n;

				const char * pSrc = (const char *) & plain_req->msg;
				char * pDest = (char *) & seg.simData[ sizeof(SIMD_SEGMENT_HEADER) ];

				bDone = true;
				for( WORD i=0, j=0; bDone && i<n; i++ ) {
					WORD datasize = ((i<n-1) ? unit_size : (msg_size - unit_size * i )/*(msg_size % unit_size)*/);
					memcpy( pDest, pSrc + j, datasize );
					seg.simHeader.size = sizeof(SIMD_SEGMENT_HEADER) + datasize;
					pHeader->segment_offset = i;
					pHeader->byte_offset = j;

					bDone = (FALSE != pSock->WriteTo( EncryptMessage(& seg), SIM_MSGSIZE(seg), pChannel->peer.ip, pChannel->peer.port/*, TRUE*/ ));

					j = j + datasize;
				}
			}
#else
			bDone = (FALSE != pSock->WriteTo( EncryptMessage(& plain_req->msg), msg_size, pChannel->peer.ip, pChannel->peer.port/*, TRUE*/ ));
      //ADD BY CHEN YUAN BEGIN
      if (!bDone)
      {
	      PTRACE( 3, PString( PString::Printf, "BBQMsgTerminal::SendMessageInternal returned false, (msgname=%s, msgsize=%d,  local.ip=%s, local.port=%d, peer.ip=%s, peer.port=%d) errcode=%d, errnumber=%d, errmsg=%s,\n   %s.", 
          SIM_MSGNAME(   plain_req->msg.simHeader.id ),  msg_size, (const char *) ToString(pChannel->local.ip), pChannel->local.port, (const char *) ToString(pChannel->peer.ip),pChannel->peer.port, (int)pSock->GetErrorCode(), (int)pSock->GetErrorNumber(), (const char*)pSock->GetErrorText(), 
		      (const char *) ToString( pChannel, true ) ) );
      }
      //ADD BY CHEN YUAN END
#endif // SUPPORT_BIG_MESSAGE
		}
	}
	return bDone;
}

bool BBQMsgTerminal::SendMessage( SIM_REQUEST * req, bool bWaitACK, DWORD timeout, int nTry , bool bAutoFillAync)
{
	if (bAutoFillAync)	SIM_NEXTSYNC( req );
	bool bDone = false;

	if( bWaitACK && req ->channel.socket == 0 ) {
		//if( ++ m_nSyncCounter == 0 ) m_nSyncCounter = 1;
		//req->msg.simHeader.sync = m_nSyncCounter;

		do {
			if( m_bClosing ) break;
      
     
			MsgWait wait( *this, req, true );
			SendMessageInternal( req );
      SendMessageInternal( req );
      SendMessageInternal( req );
			SIM_REQUEST * ack = NULL;
			bDone = wait.WaitRequest( ack, timeout );
			if(ack) { SIM_REQUEST_DELETE( ack ); }

			nTry --;
		} while( (! bDone) && (nTry > 0) );

		//req->msg.simHeader.sync = 0;
	} else {
		//req->msg.simHeader.sync = 0;
		bDone = SendMessageInternal( req );
	}

	return bDone;
}

void BBQMsgTerminal::PostMessage( SIM_REQUEST * req, bool bWaitACK, DWORD timeout, int nRetry )
{
	SIM_NEXTSYNC( req );
	// send immediately
	SendMessageInternal( req );

	if( bWaitACK && nRetry > 1 && req->channel.socket == 0 ) { // if the msg need a ACK, we put to queue

		LockIt safe( m_mutexNet );
		//LockIt safe( m_mutexMsgPost );

		if( (int)m_listMsgPosts.size() >= m_nMsgPostMax ) 
    {
        PString strMsg( PString::Printf, "warning: the post msg is full,we drop the msg.  %s msg type =%s, size=%d Post:MsgPostMax=%d ", (const char *) ToString( &  req->channel, true ),   SIM_MSGNAME(req->msg.simHeader.id),  SIM_MSGSIZE(req->msg), m_nMsgPostMax );
			  PLOG( m_pLogger, Info,  strMsg);
         return;
    }

		int nReqSize = SIM_DATASIZE_TO_REQSIZE( req->msg.simHeader.size );
		int nPostSize = sizeof(SIM_MSGPOST) - sizeof(SIM_REQUEST) + nReqSize;

		SIM_MSGPOST * pMsgPost = (SIM_MSGPOST *) malloc( nPostSize );
		if( ! pMsgPost ) return;

		//memset( pMsgPost, 0, nPostSize );
		pMsgPost->start = GetHostUpTimeInMs();
		pMsgPost->interval = timeout;
		pMsgPost->total = nRetry;
		pMsgPost->next_try = pMsgPost->start + pMsgPost->interval;
		pMsgPost->counter = 1;

		// because the size of request is not fixed, we cannot copy fixed size, just copy actual size
		//pMsgPost->req = * req;
		memcpy( & pMsgPost->req, req, nReqSize );

		// insert to proper position of the queue
		bool bInserted = false;
		for( post_iterator i = m_listMsgPosts.begin(), e = m_listMsgPosts.end(); i != e; i++ ) {
			SIM_MSGPOST * p = * i;
			if( p->next_try > pMsgPost->next_try ) { 
				m_listMsgPosts.insert( i, pMsgPost );
				bInserted = true;
				break;
			}
		}
		if( ! bInserted ) m_listMsgPosts.push_back( pMsgPost );

	}
}

SIM_REQUEST * BBQMsgTerminal::WaitMessage( const SIM_REQUEST * filter, DWORD timeout )
{
	MsgWait wait( *this, filter );

	SIM_REQUEST * reply = NULL;
	bool bWaited = wait.WaitRequest( reply, timeout );

#if PTRACING
	if( reply ) {
		PTRACE( 3, PString( PString::Printf, "msg waited: %s, sync: %d, %d bytes,\n\t%s.",
			SIM_MSGNAME(reply->msg.simHeader.id), 
			reply->msg.simHeader.sync,
			SIM_MSGSIZE(reply->msg),
			(const char *) ToString( & reply->channel, false )
			) );
	} else {
		PTRACE( 1, (bWaited ? "msg waited." : "msg wait timeout.") );
	}
#endif

	return reply;
}

SIM_REQUEST * BBQMsgTerminal::RequestMessage( SIM_REQUEST * req, uint16 msgFilter, DWORD timeout, int nRetry )
{
	SIM_NEXTSYNC( req );
	SIM_REQUEST filter;
	SIM_REPLY_REQUEST( & filter, msgFilter, 0, req );

	// put into wait list before send, cause sometimes the response is very fast
	MsgWait wait( *this, & filter );

	SIM_REQUEST * reply = NULL;
	bool bWaited = false;

	do {
		if( m_bClosing ) break;

		SendMessageInternal( req );
    SendMessageInternal( req );
		if( req->channel.socket ) {
			bWaited = wait.WaitRequest( reply, timeout * 3 );
			nRetry = 1;
		} else {
			bWaited = wait.WaitRequest( reply, timeout );
		}
	} while ( (! bWaited) && (-- nRetry) );

#if PTRACING
	if( reply ) {
		PString strMsg( PString::Printf, "msg requested: %s, sync: %d, %d bytes, %s.",
			SIM_MSGNAME(reply->msg.simHeader.id), 
			reply->msg.simHeader.sync,
			SIM_MSGSIZE(reply->msg),
			(const char *) ToString( & reply->channel, false )
			);
		//PTRACE( 3, strMsg );
		PLOG( m_pLogger, Debug3, strMsg );
	} else {
		PString strMsg( PString::Printf, "request msg for %s timeout.", SIM_MSGNAME(req->msg.simHeader.id) );
		//PTRACE( 1, strMsg );
		PLOG( m_pLogger, Debug, strMsg );
	}
#endif

	return reply;
}

void BBQMsgTerminal::BreakMessageWaiting( void )
{
	LockIt safe( m_mutexMsgWait );

	while( ! m_listMsgWaits.empty() ) {
		MsgWait * wait = m_listMsgWaits.front();
		m_listMsgWaits.pop_front();

		wait->Signal(); 
		// no need to delete this object, the thread who r waiting will delete it
	}
}


bool BBQMsgTerminal::HandleMessageWait( const SIM_REQUEST * req )
{
	LockIt safe( m_mutexMsgWait ); 

	wait_iterator eiter = m_listMsgWaits.end();
	wait_iterator iter = m_listMsgWaits.begin();
	while( iter != eiter ) {
		MsgWait * pWait = * iter;

		if( pWait->MatchFilter( req ) ) {

			//iter = m_listMsgWaits.erase( iter );
			STL_ERASE( m_listMsgWaits, wait_iterator, iter );
			pWait->Signal();

			return true;

			// the iter already moved to next, so skip iter ++
			continue;
		}

		iter ++;
	}

	return false;
}

#ifdef SUPPORT_BIG_MESSAGE
void BBQMsgTerminal::OnMessageSegmentArriveInternal( const SIM_REQUEST * reqseg )
{
	SIM_REQUEST * req = NULL;

	{
		LockIt safe( m_mutexMsgSegment );

		SegSession * pSession = NULL;

		seg_iterator iter = m_listMsgSegments.begin(), eiter = m_listMsgSegments.end();
		while( iter != eiter ) {
			SegSession * p = * iter;

			if( p->Match( reqseg ) ) {
				pSession = p;
				break;
			} else 
				iter ++;
		}

		if( ! pSession ) {
			pSession = new SegSession( reqseg );
			m_listMsgSegments.push_back( pSession );
		}

		if( (req = pSession->GetRequest()) != NULL ) {
			m_listMsgSegments.remove( pSession );
			delete pSession;
		}
	}

	if( req ) {
		OnMessageArriveInternal( req );
		SIM_REQUEST_DELETE( req );
	}
}
#endif

static DWORD GetCurrentTimeStamp()
{
	DWORD t;

#ifdef _WIN32
	struct _timeb tb;
	_ftime(&tb);
#else
	struct timeb tb;
	ftime( & tb );
#endif
	t = tb.time*1000+tb.millitm;
	return t;
}

DWORD BBQMsgTerminal::GetHostUpTime( void ) // uptime in second
{
#ifdef _WIN32
#if 1
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	DWORD s = (DWORD) (count.QuadPart / frequency.QuadPart);
#else
	static MS_TIME last = 0;
	static MS_TIME cycle_fix = 0;

	MS_TIME ms = GetTickCount() + cycle_fix;		// GetTickCount() is only a DWORD, will overflow every 49.7 days
	if( ms < last ) {
		cycle_fix += 0x100000000;
		ms += 0x100000000;
	}
	last = ms;
	DWORD s = ms / 1000;
#endif
#else
	DWORD s = 0;
	struct timeval tv;
	if( 0 == gettimeofday( & tv, NULL ) ) {
		s = tv.tv_sec;
	}
#endif

	return s;
}

MS_TIME BBQMsgTerminal::GetHostUpTimeInMs( void ) // uptime in ms
{
#ifdef _WIN32
#if 1 // disable this for microsoft bug report at http://support.microsoft.com/kb/274323/en-us
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	MS_TIME ms = (count.QuadPart / (frequency.QuadPart/1000));
#else
	static MS_TIME last = 0;
	static MS_TIME cycle_fix = 0;

	MS_TIME ms = GetTickCount() + cycle_fix;		// GetTickCount() is only a DWORD, will overflow every 49.7 days
	if( ms < last ) {
		cycle_fix += 0x100000000;
		ms += 0x100000000;
	}
	last = ms;
#endif
#else
	MS_TIME ms = 0;
	struct timeval tv;
	if( 0 == gettimeofday( & tv, NULL ) ) {
		ms = tv.tv_sec;
		ms = ms * 1000 + tv.tv_usec / 1000;
	}
#endif

	return ms;
}

void BBQMsgTerminal::OnMessageArriveInternal( const SIM_REQUEST * req )
{
	if (req->msg.simHeader.sync != 0) {
		if(req->msg.simHeader.id == SIM_ACK ) {
//#ifdef _DEBUG
//			PString str;  
//			str.sprintf("ack arrive: %s, sync: %d, %d bytes,\n\t%s.",
//				SIM_MSGNAME(req->msg.simHeader.id), req->msg.simHeader.sync, SIM_MSGSIZE(req->msg), 
//				(const char *) ToString( & req->channel, false ) );
//			PTRACE( 1, str);
//#endif
			// check the msg post list, and remove the one match this ACK
			{
				LockIt safe( m_mutexNet );
			//LockIt safe( m_mutexMsgPost );
				for( post_iterator i = m_listMsgPosts.begin(), e = m_listMsgPosts.end(); i != e; i++ ) {
					SIM_MSGPOST * p = * i;

					if( SIM_ACKFILTER_MATCH_2( & p->req, req ) ) {
						//i = m_listMsgPosts.erase( i );
						STL_ERASE( m_listMsgPosts, post_iterator, i );
						delete p;
						break;
					}
				}

			}
#if 1
			//if (((SIMD_ACK*)req->msg.simData)->ackid == SIM_CC_SENDFILEP2P)
			//	{
			//		OnMessageArriveACK(req);
			//		//LockIt safe( m_mutexP2pAck );
			//		//m_listP2pFileAcks.push_back( req->msg.simHeader.sync);
			//	}
#endif
			//if p2pfile  ack,we keep it to cal the 

		} else if( NeedAck( req ->msg.simHeader.id ) ) 
		{
			SIM_REQUEST ack;
			SIM_REPLY_REQUEST( & ack, SIM_ACK, sizeof(SIMD_ACK), req );
			SIMD_ACK* buf = (SIMD_ACK*)ack.msg.simData;
			buf->ackid = req->msg.simHeader.id;
			//if (req->msg.simHeader.id == SIM_CC_SENDFILEP2P)
			//{
			//	SIMD_CC_SENDFILE* p = (SIMD_CC_SENDFILE*)req->msg.simData;
			//	memcpy(ack.msg.simData +sizeof(SIMD_ACK), &( p->nTo), sizeof( p->nTo));
			//	memcpy(ack.msg.simData +sizeof(SIMD_ACK)+sizeof( p->nTo) , &( p->sendtime), sizeof( p->sendtime));
			//	ack.msg.simHeader.size+= (sizeof( p->nTo)+  sizeof( p->sendtime));
			//}
			//ack.msg.simHeader.sync = req->msg.simHeader.sync;
			SendMessageInternal( & ack );
		}
	}
	
	if( ! HandleMessageWait( req ) ) {

		PTRACE( 3, PString ( PString::Printf, "msg arrive: %s, sync: %d, %d bytes,\n\t%s. at time: %u, [queue size: %d]",
			SIM_MSGNAME(req->msg.simHeader.id), req->msg.simHeader.sync, SIM_MSGSIZE(req->msg), 
			(const char *) ToString( & req->channel, false ),
			GetCurrentTimeStamp(),
			m_listRequests.size() ) );

		if( m_listRequestHandlers.empty() )
			OnMessageArrive( req );
		else
			CopyRequestToQueue( req );

	}
}


// connection max limit reached
void BBQMsgTerminal::OnMsgConnectionMaxOverflow( BBQMsgConnection * pConn ) 
{
	PTRACE( 1, "Reaching connection max limit." );
}

	// called when the connection is just open
void BBQMsgTerminal::OnMsgConnectionOpen( BBQMsgConnection * pConn ) 
{
#if PTRACING
	PTCPSocket * pSock = pConn->GetSocket();
  SetSocketOptions(pSock->GetHandle());
	PIPSocket::Address addr; WORD port;
	pSock->GetPeerAddress( addr, port );
	PString str( PString::Printf, "Connection open %s %s:%d.", 
		(pConn->IsIncoming() ? "from" : "to"), (const char *) ToString( addr ), port );
	PTRACE( 1, str );
#endif
}

	// called when the connetion is closing (by local) or closed (by peer)
void BBQMsgTerminal::OnMsgConnectionClose( BBQMsgConnection * pConn ) 
{
}

	// return true to kick this idle connection
bool BBQMsgTerminal::OnMsgConnectionIdle( BBQMsgConnection * pConn ) 
{
	return true; 
}

	// return true if the message is handled
bool BBQMsgTerminal::OnMessageArrive( const SIM_REQUEST * req ) 
{
	return false; 
}

void BBQMsgTerminal::OnTick( void )
{
	MS_TIME msNow = GetHostUpTimeInMs();

#ifdef SUPPORT_BIG_MESSAGE
	// clean outdated segment sessions 
	{
		LockIt safe( m_mutexMsgSegment );

		seg_iterator iter = m_listMsgSegments.begin(), eiter = m_listMsgSegments.end();
		while( iter != eiter ) {
			SegSession * pSession = * iter;

			if( msNow > pSession->timestamp + BIG_MESSAGE_SESSION_TIME_MAX ) {
				//iter = m_listMsgSegments.erase( iter );
				STL_ERASE( m_listMsgSegments, seg_iterator, iter );
				delete pSession;
			} else {
				iter ++;
			}
		}
	}
#endif

	// clean outdated msg to post
	{
		LockIt safe( m_mutexNet );
		//LockIt safe( m_mutexMsgPost );

		while( m_listMsgPosts.size() > 0 ) {
			this->Yield();
			SIM_MSGPOST * pMsgPost = m_listMsgPosts.front();

			if( pMsgPost->next_try < msNow ) {
				// send the message
				SendMessageInternal( & pMsgPost->req );
				SIM_MSGPOST * msg = pMsgPost;
				//
	      PTRACE( 1, "Post msg "<<  SIM_MSGNAME(  msg->req.msg.simHeader.id ) );

				//
				pMsgPost->counter ++;
				m_listMsgPosts.pop_front();

				// check the counter
				if( pMsgPost->counter >= pMsgPost->total ) { // if reach max retry, remove it
					// not create by new but malloc, so call free()
					
          //PString strMsg( PString::Printf, "warning: the msg had droped, due to reach max retry.  %s msg type =%s, size=%d Post:total=%d ", (const char *) ToString( & pMsgPost->req.channel, true ),   SIM_MSGNAME(pMsgPost->req.msg.simHeader.id),  SIM_MSGSIZE(pMsgPost->req.msg), pMsgPost->total);
			    //PLOG( m_pLogger, Info,  strMsg);

					free( pMsgPost );

				} else { // re-insert to proper position
					pMsgPost->next_try += pMsgPost->interval;

					bool bInserted = false;
					for( post_iterator i = m_listMsgPosts.begin(), e = m_listMsgPosts.end(); i != e; i++ ) {
						SIM_MSGPOST * p = * i;
						if( p->next_try > pMsgPost->next_try ) {
							m_listMsgPosts.insert( i, pMsgPost );
							bInserted = true;
							break;
						}
					}
					if( ! bInserted ) m_listMsgPosts.push_back( pMsgPost );
				}
			} else {
				// all msg that reach time have been sent, skip those not reach send time, we will handle in next call to OnTick
        //PString strMsg( PString::Printf, "warning: all msg that reach time have been sent, skip those not reach send time, we will send it in next time.  %s msg type =%s, size=%d Post:total=%d ", (const char *) ToString( & pMsgPost->req.channel, true ),   SIM_MSGNAME(pMsgPost->req.msg.simHeader.id),  SIM_MSGSIZE(pMsgPost->req.msg), pMsgPost->total);
			  //PLOG( m_pLogger, Info,  strMsg);
				break;
			}
		}
	}
}

int	BBQMsgTerminal::GetMaxSocketSize( void )
{
	return FD_SETSIZE;
}

void BBQMsgTerminal::GetHostAddress(PIPSocket::Address& local_adr)
{
#if 1//chy
 local_adr = m_addrLocalinterface ;
#else
	PIPSocket::Address gateway;
	PIPSocket::InterfaceTable table;
	if( PIPSocket::GetGatewayAddress(gateway) && PIPSocket::GetInterfaceTable(table) ) {

		// first, search the IP with same subnet of default gateway
		DWORD gatewayadr = gateway;
		for( int i = 0; i < table.GetSize(); i++) {
			PIPSocket::InterfaceEntry& entry  = table[i];
			DWORD adr = entry.GetAddress();
			DWORD netmask = entry.GetNetMask();

			// ethernet IP
			if( netmask != 0 ) {
				if( ( adr & netmask ) == ( gatewayadr & netmask ) ) {
					local_adr = adr;
					return;
				}
			}
		}

#ifndef WIN32
		// second, search the IP with same interface name to default gateway, for linux/unix
		PString intf = PIPSocket::GetGatewayInterface();
		for( int i = 0; i < table.GetSize(); i++) {
			PIPSocket::InterfaceEntry& entry  = table[i];
			DWORD adr = entry.GetAddress();
			DWORD netmask = entry.GetNetMask();

			// named interface IP (not working under windows)
			PString in = entry.GetName();
			if( ! in.IsEmpty() ) {
				if( 0 == strcmp( in, intf ) ) {
					local_adr = adr;
					return;
				}
			}
		}
#endif

		// if still not found, try search IP that possibly be ADSL or sth.
		for( int i = 0; i < table.GetSize(); i++) {
			PIPSocket::InterfaceEntry& entry  = table[i];
			DWORD adr = entry.GetAddress();
			DWORD netmask = entry.GetNetMask();

			// PPPoE interface IP
			if( netmask == 0xffffffff ) {
				local_adr = adr;
				return;
			}
		}
	}

	PIPSocket::GetHostAddress(local_adr);
#endif
}
BBQMsgConnection * BBQMsgTerminal::FindMsgConnectionEx( DWORD ip, WORD port, WORD localport ,int fd )
{
	LockIt safe( m_mutexNet );

	PIPSocket::Address peer_addr;
	WORD peer_port;

#ifdef USE_MAP_FOR_LIST
	if( fd > 0 ) {
		con_iterator iter = m_listMsgConnections.find( fd );
		if( iter != m_listMsgConnections.end() ) {
			return VAR_AT(iter);
		}
	} else 
#endif
	for( con_iterator iter = m_listMsgConnections.begin(), eiter = m_listMsgConnections.end(); iter != eiter; iter ++ ) {
		BBQMsgConnection * pConn = VAR_AT(iter);
		if( pConn ) {
			PTCPSocket * pSock = pConn->GetSocket();
			if( pSock ) {
				if( fd == pSock->GetHandle() ) {
					return pConn;
				} else if( pSock->GetPeerAddress( peer_addr, peer_port ) ) {
          if( (ip == peer_addr) && (localport==0|| localport == pSock->GetPort()) && ( port == 0 || port == peer_port  ) && ( fd == 0 || fd == pSock->GetHandle() ) ) {
						return pConn;
					}
				}
			}
		}
	}

	return NULL;
}
