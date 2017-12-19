

#include <codec/opalplugin.h>


#include <stdlib.h>
#include <string.h>


#ifdef _WIN32_WCE


static int encoderInUse = 0;
static int decoderInUse = 0;

#endif // _WIN32_WCE


#define SAMPLES_PER_FRAME    80
#define BYTES_PER_FRAME      10
#define MICROSECONDSPERFRAME 10000
#define BITS_PER_SECOND      8000

#define MY_VERSION PLUGIN_CODEC_VERSION_OPTIONS

static struct PluginCodec_information licenseInfo = {
  1073619586,                              // timestamp = Fri 09 Jan 2004 03:39:46 AM UTC = 

  "Craig Southeren, Post Increment",                           // source code author
  "1.0",                                                       // source code version
  "craigs@postincrement.com",                                  // source code email
  "http://www.postincrement.com",                              // source code URL
  "Copyright (C) 2004 by Post Increment, All Rights Reserved", // source code copyright
  "MPL 1.0",                                                   // source code license
  PluginCodec_License_MPL,                                     // source code license

  "G.CODEA",                                                    // codec description
  "VoiceAge Corporation",                                      // codec author
  NULL,                                                        // codec version
  NULL,                                                        // codec email
  "http://www.voiceage.com",                                   // codec URL
  "Copyright (C) 1995-2001 VoiceAge Corporation. All Rights Reserved",  // codec copyright information
  "NULL",                                                      // codec license
  PluginCodec_License_ResearchAndDevelopmentUseOnly            // codec license code
};

//static const char L16Desc[]  = { "L16" };

static const char iCodeDescr[]  = { "YCODE desc" };
static const char iCodeMediaFmt[]  = { "Y.CODE" };
static const char ianaName[]   = { "YCODE" };
#define IANACODE  0

static const char g711A_Descr[]  = { "G.711 desc" };
static const char g711A_MediaFmt[]  = { "Y.CODE711" };
static const char g711A_ianaName[]   = { "PCMA" };
#define g711A_IANACODE  8

static const char g729Descr[]  = { "G.729 desc" };
static const char g729MediaFmt[]  = { "Y.CODE729" };
static const char g729_ianaName[]   = { "G729" };
#define g729_IANACODE  18

static const char VADStr[] = "VAD";




/////////////////////////////////////////////////////////////////////////////

static void * create_encoder(const struct PluginCodec_Definition * codec)
{
#ifdef _WIN32_WCE

  void * context = malloc(E_IF_g729ab_queryBlockSize());
  if (context == NULL)
    return NULL;

  if (E_IF_g729ab_init(context) == 0)
    return context;

  free(context);
  return NULL;

#else

  //if (encoderInUse)
  //  return NULL;

  //va_g729a_init_encoder();
  //encoderInUse = 1;
  return (void *)1;

#endif
}


static int codec_encoder(const struct PluginCodec_Definition * codec, 
                                           void * context,
                                     const void * from, 
                                       unsigned * fromLen,
                                           void * to,         
                                       unsigned * toLen,
                                   unsigned int * flag)
{

#ifdef _WIN32_WCE
  {
    UWord8 buffer[BYTES_PER_FRAME+1];
    if (E_IF_g729ab_encode(context, (Word16 *)from, buffer, toLen, 0) != 0)
      return 0;
    memcpy(to, &buffer[1], BYTES_PER_FRAME);
  }
#else

  //va_g729a_encoder((short *)from, (unsigned char *)to);
    memcpy(to, from, *fromLen);

#endif

 // *fromLen = SAMPLES_PER_FRAME*2;
  *toLen   = *fromLen;

  return 1; 
}


static void destroy_encoder(const struct PluginCodec_Definition * codec, void * context)
{
#ifdef _WIN32_WCE
  free(context);
#else
//  encoderInUse = 0;
#endif
}


static void * create_decoder(const struct PluginCodec_Definition * codec)
{
#ifdef _WIN32_WCE

  void * context = malloc(D_IF_g729ab_queryBlockSize());
  if (context == NULL)
    return NULL;

  if (D_IF_g729ab_init(context) == 0)
    return context;

  free(context);
  return NULL;

#else

  return (void *)1;

#endif
}

static int codec_decoder(const struct PluginCodec_Definition * codec, 
                                           void * context,
                                     const void * from, 
                                       unsigned * fromLen,
                                           void * to,         
                                       unsigned * toLen,
                                   unsigned int * flag)
{


  //va_g729a_decoder((unsigned char *)from, (short *)to, 0);
	memcpy(to, from, *fromLen);

  //*fromLen = BYTES_PER_FRAME;
  *toLen =*fromLen;

  return 1;
}

static void destroy_decoder(const struct PluginCodec_Definition * codec, void * context)
{
#ifdef _WIN32_WCE
  free(context);
#else
//  decoderInUse = 0;
#endif
}


static int get_codec_options(const struct PluginCodec_Definition * defn,
                                                            void * context, 
                                                      const char * name,
                                                            void * parm,
                                                        unsigned * parmLen)
{
  //if (parm == NULL || parmLen == NULL || *parmLen != sizeof(struct PluginCodec_Option **))
  //  return 0;

  //*(struct PluginCodec_Option const * const * *)parm = NoVADOptionTable; //defn->userData;
  return 1;
}


#if SUPPORT_VAD
static int set_codec_options(const struct PluginCodec_Definition * defn,
                                                            void * context,
                                                      const char * name, 
                                                            void * parm, 
                                                        unsigned * parmLen)
{
  const char * const * option;
  int vad;

  //if (context == NULL || parm == NULL || parmLen == NULL || *parmLen != sizeof(const char **))
  //  return 0;

  //for (option = (const char * const *)parm; *option != NULL; option += 2) {
  //  if (_stricmp(option[0], VADStr) == 0)
  //    vad = atoi(option[1]) != 0;
  //}

  return 1;
}
#endif


/////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_ControlDefn controlDefn[] = {
  { PLUGINCODEC_CONTROL_GET_CODEC_OPTIONS, get_codec_options },
#if SUPPORT_VAD
  { PLUGINCODEC_CONTROL_SET_CODEC_OPTIONS, set_codec_options },
#endif
  { NULL }
};

static struct PluginCodec_Definition CodecDefn[] =
{
  { 
    // encoder
    MY_VERSION,                         // codec API version
    &licenseInfo,                       // license information

    PluginCodec_MediaTypeAudio |        // audio codec
    PluginCodec_InputTypeRaw |          // raw input data
    PluginCodec_OutputTypeRaw |         // raw output data
		PluginCodec_RTPTypeDynamic,        // explicit RTP type

    iCodeDescr,                          // text decription
   iCodeMediaFmt /*L16Desc*/,
		iCodeMediaFmt,

    NULL,                     // user data

    8000,                               // samples per second
    BITS_PER_SECOND,                    // raw bits per second
    MICROSECONDSPERFRAME,               // microseconds per frame
    SAMPLES_PER_FRAME,                  // samples per frame
    10,                                 // bytes per frame
    6,                                  // recommended number of frames per packet
    24,                                 // maximum number of frames per packet
    IANACODE,                           // IANA RTP payload code
    ianaName,                           // RTP Payload name

    create_encoder,                     // create codec function
    destroy_encoder,                    // destroy codec
    codec_encoder,                      // encode/decode
    controlDefn,                        // codec controls

    PluginCodec_H323Codec_NoH323,    // h323CapabilityType
    NULL                                // h323CapabilityData
  },

  { 
    // decoder
    MY_VERSION,                         // codec API version
    &licenseInfo,                       // license information

    PluginCodec_MediaTypeAudio |        // audio codec
    PluginCodec_InputTypeRaw |          // raw input data
    PluginCodec_OutputTypeRaw |         // raw output data
		PluginCodec_RTPTypeDynamic,        // explicit RTP type

    iCodeDescr,                          // text decription
		iCodeMediaFmt,
    iCodeMediaFmt/*L16Desc*/,

    NULL,                     // user data

    8000,                               // samples per second
    BITS_PER_SECOND,                    // raw bits per second
    MICROSECONDSPERFRAME,               // microseconds per frame
    SAMPLES_PER_FRAME,                  // samples per frame
    BYTES_PER_FRAME,                    // bytes per frame
    6,                                  // recommended number of frames per packet
    24,                                 // maximum number of frames per packet
    IANACODE,                           // IANA RTP payload code
    ianaName,                           // RTP Payload name

    create_decoder,                     // create codec function
    destroy_decoder,                    // destroy codec
    codec_decoder,                      // encode/decode
    controlDefn,                        // codec controls

    PluginCodec_H323Codec_NoH323,    // h323CapabilityType 
    NULL                                // h323CapabilityData
  },

  { 
    // encoder
    MY_VERSION,                         // codec API version
    &licenseInfo,                       // license information

    PluginCodec_MediaTypeAudio |        // audio codec
    PluginCodec_InputTypeRaw |          // raw input data
    PluginCodec_OutputTypeRaw |         // raw output data
		PluginCodec_RTPTypeExplicit,        // explicit RTP type

    g711A_Descr,                          // text decription
   g711A_MediaFmt /*L16Desc*/,
		g711A_MediaFmt,

    NULL,                     // user data

    8000,                               // samples per second
    BITS_PER_SECOND,                    // raw bits per second
    MICROSECONDSPERFRAME,               // microseconds per frame
    SAMPLES_PER_FRAME,                  // samples per frame
    10,                                 // bytes per frame
    6,                                  // recommended number of frames per packet
    24,                                 // maximum number of frames per packet
    g711A_IANACODE,                           // IANA RTP payload code
    g711A_ianaName,                           // RTP Payload name

    create_encoder,                     // create codec function
    destroy_encoder,                    // destroy codec
    codec_encoder,                      // encode/decode
    controlDefn,                        // codec controls

    PluginCodec_H323Codec_NoH323,    // h323CapabilityType
    NULL                                // h323CapabilityData
  },

  { 
    // decoder
    MY_VERSION,                         // codec API version
    &licenseInfo,                       // license information

    PluginCodec_MediaTypeAudio |        // audio codec
    PluginCodec_InputTypeRaw |          // raw input data
    PluginCodec_OutputTypeRaw |         // raw output data
		PluginCodec_RTPTypeExplicit,        // explicit RTP type

    g711A_Descr,                          // text decription
		g711A_MediaFmt,
    g711A_MediaFmt/*L16Desc*/,

    NULL,                     // user data

    8000,                               // samples per second
    BITS_PER_SECOND,                    // raw bits per second
    MICROSECONDSPERFRAME,               // microseconds per frame
    SAMPLES_PER_FRAME,                  // samples per frame
    BYTES_PER_FRAME,                    // bytes per frame
    6,                                  // recommended number of frames per packet
    24,                                 // maximum number of frames per packet
    g711A_IANACODE,                           // IANA RTP payload code
    g711A_ianaName,                           // RTP Payload name

    create_decoder,                     // create codec function
    destroy_decoder,                    // destroy codec
    codec_decoder,                      // encode/decode
    controlDefn,                        // codec controls

    PluginCodec_H323Codec_NoH323,    // h323CapabilityType 
    NULL                                // h323CapabilityData
  },

  { 
    // encoder
    MY_VERSION,                         // codec API version
    &licenseInfo,                       // license information

    PluginCodec_MediaTypeAudio |        // audio codec
    PluginCodec_InputTypeRaw |          // raw input data
    PluginCodec_OutputTypeRaw |         // raw output data
    PluginCodec_RTPTypeExplicit,        // explicit RTP type

    g729Descr,                          // text decription
    g729MediaFmt,
    g729MediaFmt,

    NULL,                   // user data

    8000,                               // samples per second
    BITS_PER_SECOND,                    // raw bits per second
    MICROSECONDSPERFRAME,               // microseconds per frame
    SAMPLES_PER_FRAME,                  // samples per frame
    10,                                 // bytes per frame
    6,                                  // recommended number of frames per packet
    24,                                 // maximum number of frames per packet
    g729_IANACODE,                           // IANA RTP payload code
    g729_ianaName,                           // RTP Payload name

    create_encoder,                     // create codec function
    destroy_encoder,                    // destroy codec
    codec_encoder,                      // encode/decode
    controlDefn,                        // codec controls

    PluginCodec_H323Codec_NoH323, // h323CapabilityType
    NULL                                // h323CapabilityData
  },

  { 
    // decoder
    MY_VERSION,                         // codec API version
    &licenseInfo,                       // license information

    PluginCodec_MediaTypeAudio |        // audio codec
    PluginCodec_InputTypeRaw |          // raw input data
    PluginCodec_OutputTypeRaw |         // raw output data
    PluginCodec_RTPTypeExplicit,        // explicit RTP type

    g729Descr,                          // text decription
    g729MediaFmt,
    g729MediaFmt,

    NULL,                   // user data

    8000,                               // samples per second
    BITS_PER_SECOND,                    // raw bits per second
    MICROSECONDSPERFRAME,               // microseconds per frame
    SAMPLES_PER_FRAME,                  // samples per frame
    BYTES_PER_FRAME,                    // bytes per frame
    6,                                  // recommended number of frames per packet
    24,                                 // maximum number of frames per packet
    g729_IANACODE,                           // IANA RTP payload code
    g729_ianaName,                           // RTP Payload name

    create_decoder,                     // create codec function
    destroy_decoder,                    // destroy codec
    codec_decoder,                      // encode/decode
    controlDefn,                        // codec controls

    PluginCodec_H323Codec_NoH323,    // h323CapabilityType 
    NULL                                // h323CapabilityData
  },
};

 void* g_GetcodecdefPointerForYCode(int * count)
{
  *count = sizeof(CodecDefn)/ sizeof(struct PluginCodec_Definition);
  return &CodecDefn[0];
}

//PLUGIN_CODEC_IMPLEMENT_ALL(VoiceAgeYCODE, CodecDefn, MY_VERSION)

/////////////////////////////////////////////////////////////////////////////
