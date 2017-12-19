#ifndef OPAL_ZRTP_ZRTPEVENTPROC_H
#define OPAL_ZRTP_ZRTPEVENTPROC_H

#include <zrtp.h>
#include <opal/buildopts.h>
#include <opal/connection.h>

class ZrtpEventProcessor {
  public:
  	virtual ~ZrtpEventProcessor()  {}
    virtual void OnClear           (OpalConnection * /*connection*/, unsigned int /*sessionID*/) {}
    virtual void OnInitiatingSecure(OpalConnection * /*connection*/, unsigned int /*sessionID*/) {}
    virtual void OnPendingSecure   (OpalConnection * /*connection*/, unsigned int /*sessionID*/) {}
    virtual void OnPendingClear    (OpalConnection * /*connection*/, unsigned int /*sessionID*/) {}
    virtual void OnSecure          (OpalConnection * /*connection*/, unsigned int /*sessionID*/) {}
    virtual void OnError           (OpalConnection * /*connection*/, unsigned int /*sessionID*/) {}
    virtual void OnNoZrtp          (OpalConnection * /*connection*/, unsigned int /*sessionID*/) {}
    virtual void OnUnknownEvent    (OpalConnection * /*connection*/, unsigned int /*sessionID*/, zrtp_event_t /*event*/) {}
};

#endif // OPAL_ZRTP_ZRTPEVENTPROC_H
