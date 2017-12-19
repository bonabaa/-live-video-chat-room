#ifndef OPAL_ZRTP_OPALZRTP_H
#define OPAL_ZRTP_OPALZRTP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <zrtp.h>
#include <opal/buildopts.h>
#include <zrtp/zrtpeventproc.h>

namespace PWLibStupidLinkerHacks {
  extern int libZRTPLoader;
};

class OpalZrtp {
  public:
	static bool Init(char *name, char *zidFile);
	static bool Init(OpalZrtp *opalZrtp);
	static bool DeInit();

	static zrtp_global_ctx *GetZrtpContext();
	static unsigned char *GetZID();
	static void SetEventProcessor(ZrtpEventProcessor *eventProcessor);
	static ZrtpEventProcessor * GetEventProcessor();

	virtual ~OpalZrtp();

  protected:
	virtual unsigned char *DoGetZID();
	virtual zrtp_global_ctx *DoGetZrtpContext();
	virtual bool DoInit(char *name, char *zidFile);
	
  private:
	static OpalZrtp *instance;
	static int		isDefault;
	static ZrtpEventProcessor *eventProcessor;
};



#endif // OPAL_ZRTP_OPALZRTP_H
