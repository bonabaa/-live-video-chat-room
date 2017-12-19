#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "rtp/zrtpudp.h"
#endif

#include <opal/buildopts.h>

#if OPAL_ZRTP

  #define BUILD_ZRTP_MUTEXES 1

  #ifdef _WIN32
  #define ZRTP_PLATFORM ZP_WIN32
  #endif

  #ifdef P_LINUX
  #define ZRTP_PLATFORM ZP_LINUX
  #endif

  #include "rtp/zrtpudp.h"

  extern "C" {
  #include <zrtp.h>
  };

class OpalLibZRTPConnectionInfo : public OpalZRTPConnectionInfo {
  public:
    OpalLibZRTPConnectionInfo();

    bool Open();
    virtual RTP_UDP * CreateRTPSession(OpalConnection & conn, unsigned sessionId, bool remoteIsNat);

    static PMutex globalMutex;
    static bool globalInited;
    static zrtp_global_ctx_t zrtpGlobal;

    zrtp_conn_ctx_t zrtpConnContext;
};

class OpalLibZRTPStreamInfo : public OpalZRTPStreamInfo {
  public:
    virtual bool Open();
};


PMutex OpalLibZRTPConnectionInfo::globalMutex;
bool OpalLibZRTPConnectionInfo::globalInited = false;
zrtp_global_ctx_t OpalLibZRTPConnectionInfo::zrtpGlobal;

//////////////////////////////////////////////////////////////////////

class OpalZrtp_UDP : public SecureRTP_UDP
{
  PCLASSINFO(OpalZrtp_UDP, SecureRTP_UDP);
  public:
    OpalZrtp_UDP(
      const PString & encoding,       ///<  identifies initial RTP encoding (RTP/AVP, UDPTL etc)
      bool audio,                     ///<  is audio RTP data
      unsigned id,                    ///<  Session ID for RTP channel
      PBoolean remoteIsNAT            ///<  TRUE is remote is behind NAT
    );

    virtual ~OpalZrtp_UDP();

    virtual PBoolean WriteZrtpData(RTP_DataFrame & frame);

    virtual SendReceiveStatus OnSendData(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnSendControl(RTP_ControlFrame & frame, PINDEX & len);
    virtual SendReceiveStatus OnReceiveControl(RTP_ControlFrame & frame);
    virtual DWORD GetOutgoingSSRC();

  protected:
    zrtp_conn_ctx_t	zrtpSession;
};


class OpalZrtpSecurityMode : public OpalSecurityMode
{
  PCLASSINFO(OpalZrtpSecurityMode, OpalSecurityMode);
};

class LibZrtpSecurityMode_Base : public OpalZrtpSecurityMode
{
    PCLASSINFO(LibZrtpSecurityMode_Base, OpalZrtpSecurityMode);
  public:
    LibZrtpSecurityMode_Base();
    ~LibZrtpSecurityMode_Base();

    RTP_UDP * CreateRTPSession(
      OpalRTPConnection & connection,     ///< Connection creating session (may be needed by secure connections)
      const RTP_Session::Params & options ///< Parameters to construct with session.
    );

    PBoolean Open();

    zrtp_profile_t * GetZrtpProfile();

    zrtp_conn_ctx_t	* zrtpSession;

  protected:
    // last element of each array mush be 0
    void Init(int *sas, int *pk, int *auth, int *cipher, int *hash);
    zrtp_profile_t *profile;
};

//////////////////////////////////////////////////////////////////////

OpalLibZRTPConnectionInfo * OpalLibZRTPConnectionInfo_Create()
{
  OpalLibZRTPConnectionInfo * zrtp = new OpalLibZRTPConnectionInfo();
  if (zrtp == NULL || !zrtp->Open()) {
    delete zrtp;
    return NULL;
  }

  return zrtp;
}

OpalLibZRTPConnectionInfo::OpalLibZRTPConnectionInfo()
{
}

bool OpalLibZRTPConnectionInfo::Open()
{
  // do the global init
  {
    PWaitAndSignal m(globalMutex);
    if (!globalInited) {
      if (zrtp_init(&zrtpGlobal, 
                    "Opal", 
#ifdef _DEBUG
                    ZRTP_LICENSE_MODE_UNLIMITED
#else
                    ZRTP_LICENSE_MODE_PASSIVE
#endif
      ) != zrtp_status_ok)
        return false;

      globalInited = true;
    }

    // do the per-session init
    if (zrtp_init_session_ctx(&zrtpConnContext, &zrtpGlobal, NULL, 0) != zrtp_status_ok) 
      return false;
  }

  return true;
}

RTP_UDP * OpalLibZRTPConnectionInfo::CreateRTPSession(OpalConnection & conn, unsigned sessionId, bool remoteIsNat)
{
  return new OpalZrtp_UDP(
  //OpalZRTPStreamInfo * zrtpStreamInfo = zrtpConnInfo->CreateStream();
  //if (zrtpStreamInfo != NULL) {

  return NULL;
}


//////////////////////////////////////////////////////////////////////

OpalZrtp_UDP::OpalZrtp_UDP(const Params & params)
  : SecureRTP_UDP(params)
  , zrtpStream(NULL)
{
  zrtp_conn_ctx_t	* zrtpSession;
}
 

OpalZrtp_UDP::~OpalZrtp_UDP() {
}

// this method is used for zrtp protocol packets sending in zrtp_send_rtp function, 
//	as we dont want to process them by lib once more time in OnSendData
// (by the way they will have incorrect CRC after setting new packets length in OnSendData)
PBoolean OpalZrtp_UDP::WriteZrtpData(RTP_DataFrame & frame) {
	if (shutdownWrite) {
		shutdownWrite = FALSE;
		return PFalse;
	}

	// Trying to send a PDU before we are set up!
	if (!remoteAddress.IsValid() || remoteDataPort == 0) {
		return PFalse; //libzrtp has to wait
	}

	while (!dataSocket->WriteTo(frame.GetPointer(), 
								frame.GetHeaderSize()+frame.GetPayloadSize(),
								remoteAddress, remoteDataPort))
    {
		switch (dataSocket->GetErrorNumber()) {
			case ECONNRESET :
			case ECONNREFUSED :
    			break;
			default:
				return PFalse;
		}
	}
	return PTrue;
}
 


RTP_UDP::SendReceiveStatus OpalZrtp_UDP::OnSendData(RTP_DataFrame & frame) {
	SendReceiveStatus stat = RTP_UDP::OnSendData(frame);
	if (stat != e_ProcessPacket) {
		return stat;
	}
 
	unsigned len = frame.GetHeaderSize() + frame.GetPayloadSize();
 
 	zrtp_status_t err = ::zrtp_process_rtp(zrtpStream, (char *)frame.GetPointer(), &len);
   
	if (err != zrtp_status_ok) {
		return RTP_Session::e_IgnorePacket;
	}
 
	frame.SetPayloadSize(len - frame.GetHeaderSize());
	return e_ProcessPacket;
}
 
RTP_UDP::SendReceiveStatus OpalZrtp_UDP::OnReceiveData(RTP_DataFrame & frame) {
	unsigned len = frame.GetHeaderSize() + frame.GetPayloadSize();
	zrtp_status_t err = ::zrtp_process_srtp(zrtpStream, (char *)frame.GetPointer(), &len);
 
	if (err != zrtp_status_ok) {
		return RTP_Session::e_IgnorePacket;
	}
	
	frame.SetPayloadSize(len - frame.GetHeaderSize());
 
	return RTP_UDP::OnReceiveData(frame);
}
 
RTP_UDP::SendReceiveStatus OpalZrtp_UDP::OnSendControl(RTP_ControlFrame & frame, PINDEX & transmittedLen) {
	SendReceiveStatus stat = RTP_UDP::OnSendControl(frame, transmittedLen);
	if (stat != e_ProcessPacket) {
		return stat;
	}

	frame.SetMinSize(transmittedLen + SRTP_MAX_TRAILER_LEN);
	 unsigned len = transmittedLen;

	zrtp_status_t err = ::zrtp_process_srtcp(zrtpStream, (char *)frame.GetPointer(), &len);
	if (err != zrtp_status_ok) {
		return RTP_Session::e_IgnorePacket;
	}
	
    transmittedLen = len;
 
	return e_ProcessPacket;
}
 
RTP_UDP::SendReceiveStatus OpalZrtp_UDP::OnReceiveControl(RTP_ControlFrame & frame) {
	unsigned len = frame.GetSize();
	zrtp_status_t err = ::zrtp_process_rtcp(zrtpStream, (char *)frame.GetPointer(), &len);
	if (err != zrtp_status_ok) {
		return RTP_Session::e_IgnorePacket;
	}
	
	frame.SetSize(len);
	return RTP_UDP::OnReceiveControl(frame);
}
 
DWORD OpalZrtp_UDP::GetOutgoingSSRC() {
	return syncSourceOut;
}

///////////////////////////////////////////////////////////////////////////////

#include <zrtp.h>

#define SRTP_MAX_TAG_LEN 12 
#define SRTP_MAX_TRAILER_LEN SRTP_MAX_TAG_LEN 

#define DECLARE_LIBZRTP_CRYPTO_ALG(name, sas, pk, auth, cipher, hash) \
class OpalZrtpSecurityMode_##name : public LibZrtpSecurityMode_Base \
{ \
  public: \
	OpalZrtpSecurityMode_##name() {\
		Init(sas, pk, auth, cipher, hash); \
	} \
	~OpalZrtpSecurityMode_##name() {\
		if (profile) delete profile; \
	} \
}; \
PFACTORY_CREATE(PFactory<OpalSecurityMode>, OpalZrtpSecurityMode_##name, "ZRTP|" #name, false);

DECLARE_LIBZRTP_CRYPTO_ALG( DEFAULT, NULL, NULL, NULL, NULL, NULL);
#if 0
DECLARE_LIBZRTP_CRYPTO_ALG( STRONGHOLD, 
						((int[]){ZRTP_SAS_BASE256, ZRTP_SAS_BASE32, 0}),
				       		((int[]){ZRTP_PKTYPE_PRESH, ZRTP_PKTYPE_DH4096, ZRTP_PKTYPE_DH3072, 0}),
				      	 	((int[]){ZRTP_ATL_HS80, ZRTP_ATL_HS32, 0}),
				       		((int[]){ZRTP_CIPHER_AES256, ZRTP_CIPHER_AES128, 0}),
				       		((int[]){ZRTP_HASH_SHA256, ZRTP_HASH_SHA128, 0}));
#endif

LibZrtpSecurityMode_Base::LibZrtpSecurityMode_Base() 
{
	Init(NULL, NULL, NULL, NULL, NULL);
  zrtpSession = new(zrtp_conn_ctx_t);
  ::zrtp_init_session_ctx(zrtpSession, OpalZrtp::GetZrtpContext(), OpalZrtp::GetZID());
  zrtpSession->ctx_usr_data = this;
}

LibZrtpSecurityMode_Base::~LibZrtpSecurityMode_Base() 
{
  if (zrtpSession){
    ::zrtp_done_session_ctx(zrtpSession);
    delete (zrtpSession);
  }
}
 
zrtp_profile_t* LibZrtpSecurityMode_Base::GetZrtpProfile(){
	return profile;
}

void LibZrtpSecurityMode_Base::Init(int *sas, int *pk, int *auth, int *cipher, int *hash) {
	int i;
	if (!sas || !pk || !auth || !cipher || !hash || !sas[0] || !pk[0] || !auth[0] || !cipher[0] || !hash[0] ) {
		profile = NULL; 
		// use default components values (if we use NULL pointer libzrtp will take default sets of components)
		printf("init zrtp security mode by default\n");
		return;
	}
	
	profile = new zrtp_profile_t;
	memset(profile, 0, sizeof(zrtp_profile_t));
    
	profile->active		= 1;
	profile->autosecure	= 1;
	profile->allowclear	= 0;
	profile->cache_ttl	= ZRTP_CACHE_DEFAULT_TTL;

	for (i = 0; sas[i]; i++)
		profile->sas_schemes[i] = sas[i];

	for (i = 0; pk[i]; i++)
		profile->pk_schemes[i] = pk[i];

	for (i = 0; auth[i]; i++)
		profile->auth_tag_lens[i] = auth[i];

	for (i = 0; cipher[i]; i++)
		profile->cipher_types[i] = cipher[i];

	for (i = 0; hash[i]; i++)
		profile->hash_schemes[i] = hash[i];
	
	printf("init zrtp security mode\n");
}
 
RTP_UDP * LibZrtpSecurityMode_Base::CreateRTPSession(OpalRTPConnection & connection,
                                                     const RTP_Session::Params & options)
{
  OpalZrtp_UDP * session = new OpalZrtp_UDP(options);
  session->SetSecurityMode(this);

  int ssrc = session->GetOutgoingSSRC();
  session->zrtpStream = ::zrtp_attach_stream(zrtpSession, ssrc);

  printf("attaching stream to sessionID %d\n", id);
  if (session->zrtpStream) {
    session->zrtpStream->stream_usr_data = session;
    zrtp_start_stream(session->zrtpStream);
  }

  return session;
}
 
PBoolean LibZrtpSecurityMode_Base::Open() 
{
	return PTrue;
}

#endif
