#include "bbqbase.h"
#include ".\bbqpingproxy.h"
#define C_TESCT_STRING "hello"
BBQPingProxy::BBQPingProxy(BBQClient* client)
	: m_isRunning(false)
	, m_pThread(NULL)
{
  PAssertNULL(client);
  m_nTestUid = 0;
  m_client = client;
	m_pReadBuf = new PByteBlock(0,0, 1024*10, 512*10);


}
#ifndef CLIENT_REQUEST_CHANNEL_FOR_TESTING_PROXY_MARK
#define	CLIENT_REQUEST_CHANNEL_FOR_TESTING_PROXY_MARK		10000//appCode
#endif

void BBQPingProxy::Close()
{
  if (m_isRunning)
  {
    m_isRunning = false;
  }
  if (m_nTestUid>0)
     m_client->BreakChannelRequesting(m_nTestUid, CLIENT_REQUEST_CHANNEL_FOR_TESTING_PROXY_MARK);

  LockIt lock(m_mutex);
  for(std::map<DWORD, stMapParam>::iterator itor= m_mapWithProxyStates.begin();itor!=m_mapWithProxyStates.end();itor++)
  {
    if (itor->second.local )
    {
      if (itor->second.local->sock)
        itor->second.local->sock->Close();
      if (itor->second.local->tcpSock)
        itor->second.local->tcpSock->Close();
    }
    if (itor->second.peer )
    {
      if (itor->second.peer->sock)
        itor->second.peer->sock->Close();
      if (itor->second.peer->tcpSock)
        itor->second.peer->tcpSock->Close();
    }
  }
}
BBQPingProxy::~BBQPingProxy(void)
{
  Close();
  m_csThread.Enter();
  if( m_pThread != NULL ) {
	m_pThread->WaitForTermination();
	  delete m_pThread;
	  m_pThread = NULL;
  }
  m_csThread.Leave();

  LockIt lock(m_mutex);
  for(std::map<DWORD, stMapParam>::iterator itor= m_mapWithProxyStates.begin();itor!=m_mapWithProxyStates.end();itor++)
  {
    if (itor->second.local )
    {
      m_client->ReleaseChannel(itor->second.local);
      itor->second.local =NULL;
    }if (itor->second.peer )
    {
      m_client->ReleaseChannel(itor->second.peer);
      itor->second.peer =NULL;
    }
  }
  m_mapWithProxyStates.clear();
	delete m_pReadBuf;
}
bool BBQPingProxy::GetProxyParam(const PIPSocket::Address  proxyIp,  stPingProxyParam& param)
{
  LockIt lock(m_mutex);
  if (m_mapWithProxyStates.find((DWORD) proxyIp)!= m_mapWithProxyStates.end())
  {
    param.nDelay = m_mapWithProxyStates[(DWORD) proxyIp].nDelay;
    param.nlostpercentage = m_mapWithProxyStates[(DWORD) proxyIp].nlostpercentage;
    return true; 
  }
  return false;
}
bool BBQPingProxy::SetCallbackChannel(const PIPSocket::Address  proxyIp, const BBQ_CHANNEL*  pChannel )
{
  PAssertNULL(pChannel);
  LockIt lock(m_mutex);
  m_mapWithProxyStates[(DWORD) proxyIp].peer =(BBQ_CHANNEL *) pChannel;
  return true;
}

bool BBQPingProxy::TestPing(const PIPSocket::Address  proxyIp, const uint32 uid)
{
  m_nTestUid =  uid;
  DWORD dwproxyIp = (DWORD)proxyIp;
  if (!proxyIp.IsValid())
    return false;
  stMapParam param;
  m_isRunning = true;
  //BBQ_CHANNEL* pChannel=NULL;
  BBQ_CHANNEL* pChannel= m_client->RequestChannelForTestingProxy(uid, proxyIp);
  if (!pChannel && m_isRunning)
  {
    //test tcp
    pChannel= m_client->RequestChannelForTestingProxy(uid, proxyIp, false);
  }
  if (!pChannel)
    return false;

  param.local = pChannel;
  {
    LockIt lock(m_mutex);
    m_mapWithProxyStates[ dwproxyIp].local = param.local;
  }
  //we wait 1000*60() sec until onrequestchannel had been valid.
  int nTestCount=100;
  while(nTestCount-- >0 && m_isRunning)
  {
    {
      LockIt lock(m_mutex);
      if (m_mapWithProxyStates[dwproxyIp].peer )
        break;
    }
    Sleep(60);
  }
  {
    LockIt lock(m_mutex);
	if( m_mapWithProxyStates[dwproxyIp].peer == NULL ) {
		return false;
	}
  }
  INT param2 = (INT)((DWORD)proxyIp);
  m_csThread.Enter();
  if( m_pThread != NULL ) {
	m_pThread->WaitForTermination();
	delete m_pThread;
	m_pThread = NULL;
  }
  m_csThread.Leave();

  m_pThread = PThread::Create(PCREATE_NOTIFIER(OnTaskThread), (INT)(dwproxyIp ),
    PThread::NoAutoDeleteThread, 
    PThread::NormalPriority,
    "requestchan");
  
  m_csThread.Enter();
  if( m_pThread != NULL ) {
	m_pThread->WaitForTermination();
	  delete m_pThread;
	  m_pThread = NULL;
  }
  m_csThread.Leave();
   //m_syncThread.Wait();

  LockIt lock(m_mutex);
  if ( m_mapWithProxyStates.find( (DWORD) proxyIp ) != m_mapWithProxyStates.end() )
  {
    m_client->ReleaseChannel(m_mapWithProxyStates[(DWORD) proxyIp].peer);
    m_client->ReleaseChannel(m_mapWithProxyStates[(DWORD) proxyIp].local);
    m_mapWithProxyStates[(DWORD) proxyIp].peer =NULL;
    m_mapWithProxyStates[(DWORD) proxyIp].local =NULL;
  }
  return true;
}

void BBQPingProxy::OnTaskThread(PThread &, INT parm )
{
  m_mutex.Wait();
  DWORD proxyIp = (DWORD)parm;
  if (!proxyIp)
  {
    //why?

  }
  PSocket* sock1 = NULL;
  PSocket* sock2 = NULL;
  bool bIsTcp = (m_mapWithProxyStates[ proxyIp].local->tcpSock != NULL);
  if (bIsTcp)
  {
     sock1 = m_mapWithProxyStates[proxyIp].local->tcpSock;
     sock2 = m_mapWithProxyStates[ proxyIp].peer->tcpSock;
  }else
  {
     sock1 = m_mapWithProxyStates[ proxyIp].local->sock;
     sock2 = m_mapWithProxyStates[ proxyIp].peer->sock;
  } 
  m_mutex.Signal();

  if (sock1!=NULL && sock2!=NULL)
  {
    PTimeInterval timeout(0, 3);
  #define C_TEST_COUNT 10;
    int nTest=C_TEST_COUNT;
    int  nSucceedCount=0;
    time_t nDelay=0;
    PTimeInterval  nDelaySum(0,0);

  m_mutex.Wait();
    //
    DWORD dwLocalIp = m_mapWithProxyStates[ proxyIp].local->info.thisLAN.ip;
    DWORD wLocalPort = m_mapWithProxyStates[ proxyIp].local->info.thisLAN.port;
    DWORD dwPeerIp = m_mapWithProxyStates[ proxyIp].local->info.peerWAN.ip;
    DWORD wPeerPort = m_mapWithProxyStates[proxyIp].local->info.peerWAN.port;
	DWORD hSock = (m_mapWithProxyStates[ proxyIp].local->sock) ? 0 : m_mapWithProxyStates[ proxyIp].local->tcpSock->GetHandle();  
  m_mutex.Signal();


    //
#define D_PING_DATA_LEN sizeof(SIMD_CS_PINGPROXY)
    while(nTest-- && m_isRunning)
    {
      PTime nowbegin;

	    SIM_REQUEST req;
      SIM_REQINIT(req, hSock,0,wLocalPort, dwPeerIp, wPeerPort, SIM_CS_PINGPROXY, D_PING_DATA_LEN );
	    SIMD_CS_PINGPROXY * pQ = (SIMD_CS_PINGPROXY *) & req.msg.simData[0];
	    memset( pQ, 0, sizeof(*pQ) );
	    //pQ->sessionInfo = m_sessionInfo;
      strncpy(pQ->strdata, C_TESCT_STRING, strlen(C_TESCT_STRING) );
      SIM_REQUEST rxMsg;
      memset(&rxMsg,0,sizeof(rxMsg));
			bool bGotAPingMessge=false;

      if (bIsTcp)
      {
        //tcp
        if (!sock1->Write(&req.msg,  req.msg.simHeader.size + sizeof(SFIDMSGHEADER)) )
            continue;
				while (!bGotAPingMessge)
				{
					int nSelect = PSocket::Select(  *sock2, timeout);
					if (nSelect==-1)
					{
						sock2->Read(&rxMsg.msg,  sizeof(rxMsg.msg.simHeader) +  D_PING_DATA_LEN);
						int nRead = sock2->GetLastReadCount();
						char *pBuf= (char*)&rxMsg.msg;
						if( nRead > 0 ) {
							m_pReadBuf->PushBack( pBuf, nRead );
							if( true ) {
								nRead = m_pReadBuf->PopMessage(&rxMsg.msg  );
								if (nRead>0)
								{
									if (rxMsg.msg.simHeader.magic == SIM_MAGIC && rxMsg.msg.simHeader.id== SIM_CS_PINGPROXY)
										bGotAPingMessge = true;
									else
									{
										//m_pReadBuf->Clear();//reset buf
										//break;
									}
								}
								if ( nRead == 0 ) {
									// no data come, just retry next time
								} else if ( nRead == -1 ) {
									// when bad data coming, will go here
									PTRACE(1,"when bad data coming, will go here ");

									sock2->Close();
								} else if ( nRead == -2 ) {
									// when buffer too small, will go here, 
									// but it's impossible, cause our buffer is 8192, much larger than 2 packets
									PTRACE(1,"when buffer too small, will go here ");
									sock2->Close();

								}
							} else {
								// the buffer is not enough
								// too bad, we must meet some bad data that cannot make up a message
								//PString strMsg( PString::Printf, "Forward buffer overflow, too large packet, bad data coming from %d. Closing channel.", m_uid );
								//PLOG( & m_owner.m_UserActionLog, Info, strMsg );
								PTRACE(1,"Recv buffer overflow, ");
								sock2->Close();
							}
						}
					}
					else
						break;
				}
      }else
      {
        //udp
        if (! ((PUDPSocket*)sock1)->WriteTo(&req.msg,  req.msg.simHeader.size + sizeof(SFIDMSGHEADER), dwPeerIp, wPeerPort))
          continue;
				while (!bGotAPingMessge)
				{
					int nSelect = PSocket::Select(  *sock2, timeout);
					if (nSelect==-1)
					{

						IPSocket::Address addr;
						WORD port=0;
						if (((PUDPSocket*)sock2)->ReadFrom( &rxMsg.msg, sizeof(rxMsg.msg), addr, port) )
						{
							if (rxMsg.msg.simHeader.magic == SIM_MAGIC && rxMsg.msg.simHeader.id== SIM_CS_PINGPROXY)
							{
								bGotAPingMessge = true;
							}else
								;//continue;
						}else
						{
							break;
						}

					}else
						break;
				}
      }
      //verify the message.
			if (bGotAPingMessge)
			{
				pQ = (SIMD_CS_PINGPROXY *) & rxMsg.msg.simData[0];
				if (strncmp(pQ->strdata, C_TESCT_STRING, strlen(C_TESCT_STRING)) ==0)
				{
					PTime nowend;
					nDelaySum += nowend - nowbegin;
					nSucceedCount++;
				}
			}else
			{
				PTRACE(1, "BBQPING\t Got a invalid msg" << (SIM_MSGNAME(rxMsg.msg.simHeader.id)) );
			}
    }

  m_mutex.Wait();
    //fill the proxystate
  m_mapWithProxyStates[(DWORD) proxyIp].nDelay = nSucceedCount>0 ? nDelaySum.GetMilliSeconds()/nSucceedCount:-1 ;
  m_mapWithProxyStates[(DWORD) proxyIp].nlostpercentage = 1.0 - (float)nSucceedCount/(float)C_TEST_COUNT ;
  m_mutex.Signal();
  }
  m_syncThread.Signal();
}