#include "bbqbase.h"
#include ".\bbqclient2011.h"
#include "bbqbase.h"
#include "bbqclient.h"
#include "bbqproxy.h"
#include "cert_rsa_aes.h"
#include "bzip2/bzlib.h"
#if P_SSL
#include <openssl/rand.h>
#endif
#include <algorithm>
 #include <ptlib\debstrm.h>

BBQClient2011::BBQClient2011(void)
{
  m_pLogger = new PFileLog();
}

BBQClient2011::~BBQClient2011(void)
{
  delete m_pLogger;m_pLogger=NULL;
}
void BBQClient2011::OnTick( void )
{
	BBQMsgTerminal::OnTick();

	// send heartbeat to server, to keep this connection online
	m_nAliveCounter ++;
	if( m_nAliveCounter >= m_nKeepAliveTimeInterval ) {
		m_nAliveCounter = 0;

		//KeepAlive();
	}

	// send heartbeat to server, to keep this connection online
	m_nHeartbeatCounter ++;
	if( m_nHeartbeatCounter >= m_nHeartbeatTimeInterval ) {
		m_nHeartbeatCounter = 0;

    SendHeartbeat(SIM_CS_HEARTBEAT_IPTV);
	}
}
bool BBQClient2011::LoginSimple(const char* ip,bool bTcp, const char* alias )
{
  SIMD_CS_LOGIN_EX inInternal={0};
  SIMD_SC_LOGIN_EX outInternal={0};
  SIMD_CS_LOGIN_EX *in= &inInternal;
  SIMD_SC_LOGIN_EX * out= &outInternal ;
  PString strIP = ip;
	m_errLastLoginCode = 0;
  inInternal.techParam.clienttype = emClientTypeSimpleClient;
  strncpy(inInternal.bindInfo.bindId, alias, sizeof(inInternal.bindInfo.bindId)-1);
  //prepare to get a valid port
  PIPSocket::Address myAddr;
  WORD myPort;
  m_bTcpLogin = false;
  if (bTcp == false && BBQPing(myAddr, myPort,PIPSocket::Address( ip), SIM_PORT) ){
    SetServerAddress(ip, SIM_PORT);
  }else if ( SetServerAddress(strIP, 443, true) && ValidateTcpConnection()){
    m_bTcpLogin =true;
  }else if ( SetServerAddress(strIP, 80, true) && ValidateTcpConnection()){
    m_bTcpLogin =true;
  }else if ( SetServerAddress(strIP, 4321, true) && ValidateTcpConnection()){
    m_bTcpLogin =true;
  }else
    return false;
///////////////////////////////
  if( m_bLogin ) Logout();
	this->m_nLastStatusCode = in->onlineStatus;
  m_bServerTested = true;

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

void BBQClient2011::Logout( void )
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
  SIM_REQINITCHAN( req, m_chanLogin, SIM_CS_LOGOUT_IPTV, sizeof(SIMD_CS_LOGOUT) );
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
		//BBQTRACE( 1, "Logout, Tcp mode, connection closed." );
	} else {
		//if( m_bHeartbeatOkay ) SendMessage( & req, true );
		if( m_bHeartbeatOkay ) SendMessage( & req, false );

		PLOG( m_pLogger, Debug, "Logout. Udp mode." );
		//BBQTRACE( 1, "Logout. Udp mode." );
	}

	// break all the message waiting thread
	BreakMessageWaiting();

	m_chanLogin.socket = 0;
	m_bLogin = false;
	//m_nServerId = 0;
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

	// clear server PKI cert
	if( m_pX509_server_cert ) {
		PKI_free_cert( m_pX509_server_cert );
		m_pX509_server_cert = NULL;
	}
	if( m_lpszX509Text ) {
		free( m_lpszX509Text );
		m_lpszX509Text = NULL;
	}


}




void BBQClient2011::OnUserDefinedMessageArrive( time_t tServerTime, uint32 nMeaning, uint32 nLength, const char * pData, VFONID fromId, const char * lpszAlias ) 
{
  BBQClient::OnUserDefinedMessageArrive(tServerTime, nMeaning, nLength, pData, fromId, lpszAlias);
  if (m_OnUserDefineMsg){
    m_OnUserDefineMsg((SF_HANDLE)this, fromId.userid, pData, nLength);
    PTRACE(3,"BBQClient2011\t Received user define msg from "<< fromId.userid <<",msg length is  " <<  nLength );
  }else{
    PTRACE(3,"BBQClient2011\t error:client not set the call back to receive user define msg." );
  }
}
