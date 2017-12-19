
#ifndef _CYQ_CLIENT_H
#define _CYQ_CLIENT_H
#include "stdlib.h"
#  ifdef DLL_EXPORTS

#define LPN_DLL_API __declspec(dllexport)
#define uint32 unsigned int
#define DWORD  unsigned int
#define uint16 unsigned short

#  else

#    define LPN_DLL_API __declspec(dllimport)
#define uint32 unsigned int
#define uint16 unsigned short

#  endif

#define REGISTER_PLUGIN(x) CClient::Register(x)
typedef struct IPSOCKADDR2 {
	uint32				ip;
	uint16				port;
}  IPSOCKADDR2;
////
typedef struct CZ_CHANNEL {
	void				* udpsock;				// not null if udp
	void				* tcpSock;			// not null if tcp
	void				* sslChannel;		// not null if tcp ssl
	struct tagInfo {
		bool				bIsProxied;			// is proxied, or P2P
		bool				bInitFromLocal;		// channel requested from local
		//uint32			nChannelId;			// channel id of init party

		uint32				peerServerId;		// the server that peer client login
		uint32				peerId;				// peer client id

		IPSOCKADDR2			peerWAN;			// WAN ip:port of peer client
		IPSOCKADDR2			peerLAN;			// LAN ip:port of peer client
		IPSOCKADDR2			thisLAN;			// local ip:port

		DWORD				dwLocalTimeBase;
		DWORD				dwLocalCount;
		DWORD				dwRemoteTimeBase;
		DWORD				dwRemoteCount;

		uint32				nLocalAppParam;		
		uint32				nRemoteAppParam;
	} info;

		uint32				from;		
		uint32				to;
  
	unsigned int addrbbqchannel;//closerequestchanne(addrbbqchannel )
} CZ_CHANNEL2;



class  LPN_DLL_API CClient {

public:
	CClient(   );
	virtual ~CClient();
  static bool Register(CClient *p);
  bool czConnect(const char * ip, int port, const char* strID, const char* strPassword);
  bool czHB();
  bool czHello();
  CZ_CHANNEL * RequestChannel( uint32 uid, CZ_CHANNEL *result, int type, int modes=0x01);
  bool CloseRequestChannel( unsigned int& p);
  bool DeAttachRequestChannel(unsigned int pChannel);
  bool IsLogin();
  bool czCreateRoom(const char* name, const char* members);
  bool czLeftRoom(const char* name );
  unsigned int UdpRecvFrom(void * buf, PINDEX len,unsigned int& addr, unsigned int&port, const unsigned int pChannel);
  unsigned int UdpWriteTo(const void * buf, PINDEX len, const unsigned int pChannel);
  int socketSelect(const unsigned int pChanneldata, const unsigned int pChannelControl, unsigned int timeout);
  int gGetErrorNumber( const unsigned int pChannel, int errorcode);
  bool GetFirewallInfo(unsigned int& local, unsigned int& wan);
  bool GetSocketWanInfoByBBQChannel(const unsigned int pChannel, unsigned int& ip,  unsigned int& port);
public:
  virtual bool  OnRecvChannelRequest(CZ_CHANNEL* ){ return false;};
  virtual void  OnRequestedChannelSetup( CZ_CHANNEL * pChannel, int nAppCode, const char * lpszAppString ){return;} ;
  virtual bool  OnRecvCallers(const char*callers,  unsigned int){ return false;};
  virtual bool  OnRecvPeerNat(unsigned int from, unsigned int to, int type, unsigned int ip, unsigned short port, unsigned int tag){return false;}
private:
  //BBQClient* m_client;
};


#endif // _CYQ_CLIENT_H

