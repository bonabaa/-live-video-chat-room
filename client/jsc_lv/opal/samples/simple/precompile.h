

#include <ptlib.h>
#include <ptlib/pprocess.h>
#include <ptlib/videoio.h>
#include <ptlib/sound.h>
#include <ptlib/video.h>
#include <ptclib/vsdl.h>
#include <opal/transcoders.h>
#include <codec/opalplugin.h>
#include <codec/opalpluginmgr.h>
#include <codec/vidcodec.h>

#include <opal/buildopts.h>

#if OPAL_IAX2
#include <iax2/iax2.h>
#endif

#if OPAL_SIP
#include <sip/sip.h>
#endif

#if OPAL_H323
#include <h323/h323.h>
#include <h323/gkclient.h>
#endif

#if OPAL_FAX
#include <t38/t38proto.h>
#endif

#include <t38/t38proto.h>

#include <opal/transcoders.h>
#include <lids/lidep.h>
#include <ptclib/pstun.h>
#include <ptlib/config.h>

