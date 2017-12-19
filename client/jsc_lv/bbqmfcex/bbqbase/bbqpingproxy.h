#pragma once
#include <ptlib.h>
#include <map>

class BBQClient;
struct BBQ_CHANNEL;
struct stPingProxyParam
{
  unsigned nDelay; //ÑÓÊ±
  float nlostpercentage;//¶ª°üÂÊ

};
struct stMapParam:public stPingProxyParam
{
  BBQ_CHANNEL *local; //requestchannel
  BBQ_CHANNEL *peer;   //onrequestedchannel
  //bool bTcp;   // tcp or udp
  stMapParam()
  {
    memset(this,0, sizeof(stMapParam));
	nDelay =-1;
	nlostpercentage = 100;
  };
};
class BBQPingProxy
{
public:
  BBQPingProxy(BBQClient* client);
  ~BBQPingProxy(void);
  void Close();
  bool GetProxyParam(const PIPSocket::Address  proxyIp,  stPingProxyParam& param);
  // set onrequestedchannel pointer
  bool SetCallbackChannel(const PIPSocket::Address  proxyIp, const BBQ_CHANNEL*  pChannel );
  bool TestPing(const PIPSocket::Address  proxyIp, const uint32 uid);
protected:
  BBQClient* m_client;
  uint32 m_nTestUid;
  PMutex m_mutex;// mutex m_mapWithProxyStates
  std::map<DWORD, stMapParam> m_mapWithProxyStates;
  PSyncPoint  m_syncThread;//wait until taskthread exited.

  PDECLARE_NOTIFIER(PThread, BBQPingProxy, OnTaskThread);
  PThread* m_pThread;
  PCriticalSection m_csThread;
  bool m_isRunning;
	PByteBlock* m_pReadBuf;
};
