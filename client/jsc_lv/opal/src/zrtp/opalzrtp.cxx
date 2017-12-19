#ifdef __GNUC__
#pragma implementation "zrtp/opalzrtp.h"
#endif

#include <zrtp.h>
#include <ptlib.h>
#include <opal/buildopts.h>
#include <zrtp/opalzrtp.h>
#include <rtp/zrtp.h>
#include <sip/sipcon.h>

namespace PWLibStupidLinkerHacks {
	int libZRTPLoader;
};

extern "C" {

void zrtp_get_cache_path(char *path, uint32_t length)
{
  strcat(path, "zrtp.cache");
}

};

class OpalZrtpDefault : public OpalZrtp {
  public:
	OpalZrtpDefault();
	virtual ~OpalZrtpDefault();

  protected:
	virtual unsigned char *DoGetZID();
	virtual zrtp_global_ctx *DoGetZrtpContext();
	virtual bool DoInit(char *name, char *zidFile);

  private:
	bool LoadZID(char *zidFile);

	zrtp_global_ctx 	*zrtpContext;
	zrtp_zid_t 			zid;
};

OpalZrtp *OpalZrtp::instance = NULL;
int OpalZrtp::isDefault = 0;
ZrtpEventProcessor *OpalZrtp::eventProcessor = 0;

bool OpalZrtp::Init(char *name, char *zidFile) {
	printf("Opal zrtp init\n");
	if(instance) {
		return false;
	}
	
	instance = new OpalZrtpDefault();
	printf("Create default zrtp instance\n");
	if(!instance->DoInit(name, zidFile)) {
		printf("zrtp instance initialization failed\n");
		delete instance;
		instance = NULL;
		return false;
	}
	printf("zrtp instance initialized successfully\n");
	isDefault = 1;
	return true;
}

bool OpalZrtp::Init(OpalZrtp *opalZrtp) {
	instance = opalZrtp;
	isDefault = 0;
	return true;
}

bool OpalZrtp::DeInit() {
	if (!instance) {
		return false;
	}

	if (isDefault) {
		delete instance;
	}
	instance = NULL;
	return true;
}

zrtp_global_ctx *OpalZrtp::GetZrtpContext() {
	if (!instance) {
		return NULL;
	}

	return instance->DoGetZrtpContext();
}

unsigned char *OpalZrtp::GetZID() {
	if (!instance) {
		return NULL;
	}
	
	return instance->DoGetZID();
} 

void OpalZrtp::SetEventProcessor(ZrtpEventProcessor *eventProcessor){
	eventProcessor = eventProcessor;
}

ZrtpEventProcessor * OpalZrtp::GetEventProcessor(){
	return eventProcessor;
}

OpalZrtp::~OpalZrtp() {
};

unsigned char *OpalZrtp::DoGetZID() {
	return NULL;
}

zrtp_global_ctx *OpalZrtp::DoGetZrtpContext() {
	return NULL;
}

bool OpalZrtp::DoInit(char *name, char *zidFile) {
	return false;
}

// -------------------------------------------------------------------------------

OpalZrtpDefault::OpalZrtpDefault()
:zrtpContext(NULL)
{

}

OpalZrtpDefault::~OpalZrtpDefault() {
    if (zrtpContext) {
		zrtp_down(zrtpContext);
		delete zrtpContext;
    }
}

bool OpalZrtpDefault::DoInit(char *name, char *zidFile) {
	if (zrtpContext)
		return false;

	zrtpContext = new zrtp_global_ctx_t;
	printf("Going to init library\n");
	if (zrtp_status_ok != zrtp_init(zrtpContext, name)) {
		delete zrtpContext;
		zrtpContext = NULL;
		printf("initialization of library failed\n");
		return false;
	}

	if (!LoadZID(zidFile)) {
		zrtp_down(zrtpContext);
		delete zrtpContext;
		zrtpContext = NULL;
		return false;
	}

    return true;
}

bool OpalZrtpDefault::LoadZID(char *zidFile) {
	FILE* zidf = fopen(zidFile, "r+");
	if (zidf) {
		fseek(zidf, 0, SEEK_END);
		if (sizeof(zrtp_zid_t) == ftell(zidf)) {
			//if size of the file is correct
			fseek(zidf, 0, SEEK_SET);
			fread(zid, sizeof(zrtp_zid_t), 1, zidf);
			fclose(zidf);
			return true;
		}
	}

	// lets generate new zid
	zrtp_randstr(zrtpContext, zid, sizeof(zrtp_zid_t));
	// and write it to the file
	zidf = fopen(zidFile, "w");
	if (!zidf)
		return false;
	fwrite(zid, sizeof(zrtp_zid_t), 1, zidf);
	fclose(zidf);
	return true;
}

zrtp_global_ctx *OpalZrtpDefault::DoGetZrtpContext() {
    return zrtpContext;
}

unsigned char *OpalZrtpDefault::DoGetZID() {
    return zid;
} 

extern "C" {

int zrtp_send_rtp(const zrtp_stream_ctx_t* stream_ctx, char* packet, unsigned int length) {
	OpalZrtp_UDP * rtpSession = (OpalZrtp_UDP *)stream_ctx->stream_usr_data;
	if (NULL == rtpSession)
		return zrtp_status_write_fail;

	RTP_DataFrame frame((BYTE *)packet, length);
	// we use special method for sending, without packets preprocessing
	return rtpSession->WriteZrtpData(frame) ? zrtp_status_ok : zrtp_status_write_fail;
}

//TODO: make this related with internal Opal's library logger
void zrtp_print_log(log_level_t level, const char* format, ...) {
	FILE *f;
	
	va_list arg;
	va_start(arg, format);
	f = fopen("/tmp/opal_zrtp.log", "a");
//  PTRACE(level+1, "libZRTP\t" + psprintf(format, arg));
	vfprintf(f, format, arg);
	 vprintf(format, arg);
	fclose(f);
	va_end( arg );
}

void zrtp_event_callback(zrtp_event_t event, zrtp_stream_ctx_t *ctx) {
	OpalZrtp_UDP *opalSession;
	SIPConnection *opalConnection;
	
	ZrtpEventProcessor *eventProcessor = OpalZrtp::GetEventProcessor();
	if(NULL == eventProcessor){
		return;
	}
	opalSession = (OpalZrtp_UDP*) ctx->stream_usr_data;
	opalConnection = (SIPConnection*) ctx->_session_ctx->ctx_usr_data;
	if(NULL == opalConnection || NULL == opalSession){
		return;
	}
	
	switch(event){
		case ZRTP_EVENT_IS_CLEAR:
			eventProcessor->OnClear(opalConnection, opalSession->GetSessionID());
			break;
		
		case ZRTP_EVENT_IS_INITIATINGSECURE:
			eventProcessor->OnInitiatingSecure(opalConnection, opalSession->GetSessionID());
			break;
			
		case ZRTP_EVENT_IS_PENDINGSECURE:
			eventProcessor->OnPendingSecure(opalConnection, opalSession->GetSessionID());
			break;
			
		case ZRTP_EVENT_IS_PENDINGCLEAR:
			eventProcessor->OnPendingClear(opalConnection, opalSession->GetSessionID());
			break;
			
		case ZRTP_EVENT_IS_SECURE:
			eventProcessor->OnSecure(opalConnection, opalSession->GetSessionID());
			break;
			
		case ZRTP_EVENT_ERROR:
			eventProcessor->OnError(opalConnection, opalSession->GetSessionID());
			break;
			
		case ZRTP_EVENT_NO_ZRTP:
			eventProcessor->OnNoZrtp(opalConnection, opalSession->GetSessionID());
			break;
			
		default:
			eventProcessor->OnUnknownEvent(opalConnection, opalSession->GetSessionID(), event);
			break;
	}
}

void zrtp_play_alert(zrtp_stream_ctx_t * stream_ctx) {
    stream_ctx->need_play_alert = zrtp_play_no;
}

} //extern "C"









