
#include "bbqbase.h"
#include "cyclient.h"
#include "bbqclient.h"
#include "stdafx.h"
// #include <ptlib\debstrm.h>

#define UDP_BUFFER_SIZE 32768

static void SetMinBufferSize(PUDPSocket & sock, int buftype)
{
  int sz = 0;
  if (sock.GetOption(buftype, sz)) {
    if (sz >= UDP_BUFFER_SIZE)
      return;
  }
  else {
    PTRACE(1, "RTP_UDP\tGetOption(" << buftype << ") failed: " << sock.GetErrorText());
  }

  if (!sock.SetOption(buftype, UDP_BUFFER_SIZE)) {
    PTRACE(1, "RTP_UDP\tSetOption(" << buftype << ") failed: " << sock.GetErrorText());
  }

  PTRACE_IF(1, !sock.GetOption(buftype, sz) && sz < UDP_BUFFER_SIZE, "RTP_UDP\tSetOption(" << buftype << ") failed, even though it said it succeeded!");
}
BBQClient* m_client;
class NullProcess : public PProcess
{
	PCLASSINFO(NullProcess,PProcess)
public:
	NullProcess():PProcess("","LPNGroup"){}
	void Main(){}
};


NullProcess * GetProcess()
{
	static NullProcess* _process=NULL;
  if (!_process)
    _process = new NullProcess();

	return _process;
}

#define  Status(x) PString(x)
extern CClient* g_clientExtern;
CClient::CClient(   )
{
  GetProcess();
#ifdef _DEBUG
   PTrace::Initialise(5,"DEBUGSTREAM",
                      
                     PTrace::Timestamp|PTrace::Thread/*|PTrace::FileAndLine*/);
   PTRACE(1,"ptrace enabled.");

#else
#endif
  m_client =NULL;
  m_client = new BBQClient(1024 );
  if (m_client)
  {
    m_client->SetNetworkBaseDelay(0);
    //m_client->SetTcpLogin(true);
  }
}
CClient::~CClient()
{
  delete m_client;
  m_client =NULL;
  delete GetProcess();
}
bool  CClient::Register(CClient *p)
{
  g_clientExtern = p;
  return true;
}
bool CClient::czHello()
{
  if (!m_client->IsLogin())
    return false;
  return m_client->SendHeartbeat();
}
bool CClient::IsLogin()
{
  return m_client->IsLogin();
}
bool CClient::czLeftRoom(const char* name )
{
  m_client->leftRoom(PString(name).AsUnsigned() );
  return true;
}

bool CClient::czHB()
{
  if (!m_client->IsLogin())
    return false;
  return m_client->SendHeartbeat();
}
bool CClient::czCreateRoom(const char* name, const char* members)
{
  if (!m_client->IsLogin())
    return false;

  PString ID = name;

  return m_client->requestRoom(ID.AsUnsigned() );
}
bool CClient::DeAttachRequestChannel(unsigned int pChannel)
{
  if (pChannel!=0)
  {
    BBQ_CHANNEL* p1 = (BBQ_CHANNEL*)pChannel;
    if (p1->sock)
      m_client->DetachUdpListener(p1->sock);
  }

  return true;
}
bool CClient::CloseRequestChannel( unsigned int& p)
{
  if (p!=0)
  {
    BBQ_CHANNEL* p1 = (BBQ_CHANNEL*)p;
    if (p1->sock)
      m_client->DetachUdpListener(p1->sock);

    m_client->ReleaseChannel(p1);
    //delete p1;
  }
  p=0;
  return true;
}
unsigned int CClient::UdpRecvFrom(void * buf, PINDEX len,unsigned int& addr, unsigned int&port, const unsigned int pChannel)
{
  if (pChannel)
  {
    BBQ_CHANNEL* p = (BBQ_CHANNEL*)pChannel;
    if(p->sock)
    {
      PIPSocket::Address addr1;
      WORD port1;
      SIM_REQUEST rxMsg;

      if(p->sock->ReadFrom(&rxMsg.msg, sizeof(rxMsg.msg), /*buf, len,*/ addr1, port1))
      {
        int nRtpPacketLen  = p->sock->GetLastReadCount() -sizeof(SFIDMSGHEADER);
        if (rxMsg.msg.simHeader.magic ==  SIM_MAGIC&& nRtpPacketLen== rxMsg.msg.simHeader.size)
        {
          if (nRtpPacketLen>0)
          {
            port = port1;
            addr = DWORD(addr1);
            //memset(&rxMsg,0,sizeof(rxMsg));
            SIMD_CS_PINGPROXY * pQ = (SIMD_CS_PINGPROXY *) & rxMsg.msg.simData[0];
            memcpy(buf, pQ->strdata, nRtpPacketLen );
          }
          return nRtpPacketLen/*p->sock->GetLastReadCount()*/;
        }else
          return 0;
      }
      else
        return 0;
    }
  }
  else
    return 0;
}

unsigned int CClient::UdpWriteTo(const void * buf, PINDEX len, const unsigned int pChannel)
{
  if (pChannel)
  {
    BBQ_CHANNEL* p = (BBQ_CHANNEL*)pChannel;
    if(p->sock)
    {
	    SIM_REQUEST req;
      SIM_REQINIT(req, 0, 0, p->info.thisLAN.port, p->info.peerWAN.ip, p->info.peerWAN.port, SIM_CS_PINGPROXY, len/*sizeof(SIMD_CS_PINGPROXY)*/ );
	    SIMD_CS_PINGPROXY * pQ = (SIMD_CS_PINGPROXY *) & req.msg.simData[0];
	    //memset( pQ, 0, sizeof(*pQ) );
      memcpy(pQ->strdata, buf, len);
      if(p->sock->WriteTo(&req.msg,  /*req.msg.simHeader.size*/len + sizeof(SFIDMSGHEADER), p->info.peerWAN.ip, p->info.peerWAN.port ))
      {
        return p->sock->GetLastWriteCount();
      }
      else
        return 0;
    }
  }
  else
    return 0;
}
int CClient::socketSelect(const unsigned int pChanneldata, const unsigned int pChannelControl, unsigned int timeout)
{
  if (pChanneldata/*&& pChannelControl*/)
  {
    BBQ_CHANNEL* p = (BBQ_CHANNEL*)pChanneldata;
    //BBQ_CHANNEL* p2 = (BBQ_CHANNEL*)pChannelControl;
    if(p->sock/*&& p2->sock*/)
    {
      //PSocket::SelectList lst;
      //lst.Append(p->sock);
      return PIPSocket::Select(*(p->sock), *(p->sock), timeout);
    }
  }
  return 0;
}
int CClient::gGetErrorNumber( const unsigned int pChannel,int errorcode)
{ 
  if (pChannel)
  {
    BBQ_CHANNEL* p = (BBQ_CHANNEL*)pChannel;
    if(p->sock)
    {
      {
        return p->sock->GetErrorNumber((PChannel::ErrorGroup)errorcode);
      }
    }
  }
  else
    return 0;
}
CZ_CHANNEL * CClient::RequestChannel( uint32 uid, CZ_CHANNEL *result, int type, int modes /*UDP_P2P*/)
{
  BBQ_CHANNEL* p=m_client->RequestChannel(  uid, type, "hello, the world.", type, 15000, modes );
  if (p)
  {
    //memcpy(result, &(p->info), sizeof(p->info) );;
    result->udpsock = p->sock;
    result->info.thisLAN.ip = p->info.thisLAN.ip;
    result->info.thisLAN.port = p->info.thisLAN.port;
    result->info.peerWAN.port = p->info.peerWAN.port; 
    result->info.peerWAN.ip = p->info.peerWAN.ip;
    result->info.peerLAN.ip = p->info.peerLAN.ip;
    result->info.peerLAN.port = p->info.peerLAN.port;
    result->addrbbqchannel = (unsigned int)p;
    if (p->sock)
    {
      SetMinBufferSize(*(p->sock),    SO_RCVBUF);
      SetMinBufferSize(*(p->sock),    SO_SNDBUF);
    }

		if( p->sslChannel ) {
			// do nothing
		} else if( p->tcpSock ) {
			//m_pChannel->tcpSock->SetWriteTimeout( m_owner.m_loadparam->tcp_channel_block_timeout * 1000 ); // 30s

			//m_pConn = new BBQMsgConnection(false, m_pChannel->tcpSock, false);
			//if( m_pConn ) {
			//	m_owner.AttachMsgConnection( m_pConn );
			//}
		} else if ( p->sock ) {
			//m_client->AttachUdpListener( p->sock );
      for(int i=0;i< 4;i++)
      {
        char buf[24] ={0};
        sprintf(buf, "LPN%u,%u", i, i*10);
        p->sock->WriteTo(buf, strlen(buf), p->info.peerWAN.ip, p->info.peerWAN.port);
      }
		}
    p->sock = NULL;
    m_client->ReleaseChannel(p);
    return result;
  }else
    return NULL;
}
bool CClient::GetFirewallInfo(unsigned int& localip, unsigned int& wanip)
{
  BBQClient::FirewallType type;
  PIPSocket::Address local, wan;
  if (m_client->GetFirewallInfo(local, wan, type))
  {
    localip = (DWORD)local;
    wanip = (DWORD)wan;
  }else{
    return false;

  }

  return true;
}

bool CClient::czConnect(const char * ip,const int port, const char* strID, const char* strPassword)
{
  if (m_client->SetServerAddress(ip, port))
  {
			MS_TIME msStart = BBQMsgTerminal::GetHostUpTimeInMs();

			uint32 major, minor, build;
			char* p = (char*) (const char *) "2.0";
			major = strtol( p, & p, 0 ); while(*p=='.') p++;
			minor = strtol( p, & p, 0 ); while(*p=='.') p++;
			build = strtol( p, & p, 0 );

			BBQUserTechParamInfo techInfo;
			memset( & techInfo, 0, sizeof(techInfo) );
			techInfo.version = ((major & 0xff) << 24) | ((minor & 0xff) << 16) | (build & 0xffff);
			m_client->SetTechInfo( & techInfo );

			SIMD_CS_LOGIN_EX in;
			SIMD_SC_LOGIN_EX out;
			memset( & in, 0, sizeof(in) );
			memset( & out, 0, sizeof(out) );
			in.vfonId.userid = strtol(strID,NULL,0);
			in.techParam = techInfo;
			MD5Encryption( in.lpszPassword, strPassword );

			PTRACE( 1, "MD5 password: " << in.lpszPassword );

			memcpy( in.lpszHardwareId, (const char *) "GetHardwareToken()", USERDATA_NAME_SIZE ); in.lpszHardwareId[ USERDATA_NAME_SIZE-1 ] = '\0';
			in.onlineStatus = CLIENT_NORMAL;//StatusStringToInt( strStatus );
			
			if( m_client->LoginEx( & in, & out ) ) {
				PTime t1( out.tStartTime ), t2( out.tExpireTime );
				//Status( PString( PString::Printf, "Okay, login to %s:%d.\r\n"
				//	"My Id: %d\r\n"
				//	"Free time: [%s, %s], %s.\r\n"
				//	"Points: %d\r\n"
				//	"Access flag: %04x\r\n", 
				//	(const char *)(PString)(PIPSocket::Address)(DWORD)m_dwIP, m_nPort,
				//	out.sessionInfo.id,
				//	(const char *)t1.AsString("yyyy/MM/dd hh:mm:ss"), (const char *)t2.AsString("yyyy/MM/dd hh:mm:ss"),
				//	(out.tNowServerTime < out.tExpireTime) ? "valid" : "expired",
				//	out.nPoints,
				//	out.accessFlags.value
				//	) );

				m_client->SendHeartbeat();
        return true;
			} else { 
				PString strReason = "";
				switch( out.errCode ) {
				case ERR_NONE: strReason = "ERR_NONE"; break;
				case ERR_CLIENTAPP: strReason = "ERR_CLIENTAPP"; break; 
				case ERR_VERSION: strReason = "ERR_VERSION"; break;
				case ERR_AUTHFAIL: strReason = "ERR_AUTHFAIL"; break;
				case ERR_TOOMANY: strReason = "ERR_TOOMANY"; break;
				case ERR_FORBIDDEN: strReason = "ERR_FORBIDDEN"; break;
				case ERR_ISGROUPID: strReason = "ERR_ISGROUPID"; break;
				}
				Status( PString( PString::Printf, "Login failed, error: %d, %s, %s.\r\n", 
					m_client->GetLastErrorCode(), 
					(const char *) m_client->GetLastErrorString(),
					(const char *) strReason ) );
			}

		}
    return false;
}
bool CClient::GetSocketWanInfoByBBQChannel(const unsigned int pChannel, unsigned int& ip,  unsigned int& port)
{
  if (pChannel)
  {
    BBQ_CHANNEL* p = (BBQ_CHANNEL*)pChannel;
    if(p->sock)
    {
      ip = p->info.peerWAN.ip;port = p->info.peerWAN.port;
      return true;
    }
  }
  
  return false;
}





