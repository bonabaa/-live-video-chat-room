/*
 * CELT Codec Plugin for Opal
 *
 * Based on the GSM-AMR one
 */

#define _CRT_NONSTDC_NO_DEPRECATE 1
#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "plugin-config.h"

#include <codec/opalplugin.h>
#include "celt/libcelt/celt.h"

#ifndef FALSE
#define FALSE 0
#endif
//#define D_CUSTOM_CELT 
#ifndef TRUE
#define TRUE (!FALSE)
#endif

#if defined(_WIN32) || defined(_WIN32_WCE)
  #define STRCMPI  _strcmpi
#else
  #define STRCMPI  strcasecmp
#endif

/*Disable some warnings on VC++*/
#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif
//#define SAMPLING_RATE 48000
#define D_MAX_FRAME_SIZE 60
#define D_PACKETSIZE 43
//
//#define D_MAX_FRAME_BYTES 960
//#define D_MAX_FRAME_SIZE 256
//#define D_CELT_DECODE_ORANGNAL 1

// this is what we hand back when we are asked to create an encoder
typedef struct
{
  CELTDecoder *decoder_state;
  CELTEncoder *encoder_state;
  CELTMode    *mode;
  int frame_size;
  int bytes_per_packet;
} CELTContext;


/////////////////////////////////////////////////////////////////////////////

static int init_mode(CELTContext *celt, const struct PluginCodec_Definition * codec)
{
  int error = 0;

#if defined (HAVE_CELT_0_4_OR_SOONER) || defined (HAVE_CELT_0_5_0_OR_0_6_0)
  celt->mode = celt_mode_create(codec->sampleRate, 1, codec->parm.audio.samplesPerFrame, &error);
#else
  celt->mode = celt_mode_create(codec->sampleRate,D_MAX_FRAME_SIZE /*codec->parm.audio.samplesPerFrame*2*/,&error);
#endif
  if (celt->mode == NULL) {
    return FALSE;
  }

  //celt_mode_info(celt->mode, CELT_GET_FRAME_SIZE, &celt->frame_size);
  // celt_mode_info(mode, CELT_GET_NB_CHANNELS, &channels);
   //celt_mode_info(celt->mode,1500, &celt->frame_size);
  celt->bytes_per_packet =D_PACKETSIZE;// (codec->bitsPerSec * celt->frame_size/codec->sampleRate + 4) / 8;

  return TRUE;
}

static void * celt_create_encoder(const struct PluginCodec_Definition * codec)
{
  CELTContext * celt = malloc(sizeof(CELTContext));
  memset(celt, 0, sizeof(CELTContext));
  if (celt == NULL)
    return NULL;
#ifdef D_CUSTOM_CELT
  if (init_mode(celt, codec) == FALSE) {
    free(celt);
    return NULL;
  }
#else
  if (init_mode(celt, codec) == FALSE) {
    free(celt);
    return NULL;
  }
#endif
#if defined (HAVE_CELT_0_4_OR_SOONER) || defined (HAVE_CELT_0_5_0_OR_0_6_0)
  celt->encoder_state = celt_encoder_create(celt->mode);
#else
    celt->encoder_state = celt_encoder_create_custom(celt->mode, 1, NULL );
    //if (celt->encoder_state)
    //      celt_encoder_ctl(celt->encoder_state,CELT_SET_COMPLEXITY(7));

#endif
  if (celt->encoder_state == NULL ) {
    celt_mode_destroy(celt->mode);
    free(celt);celt=NULL;
    return NULL;
  }

  return celt;
}


static void * celt_create_decoder(const struct PluginCodec_Definition * codec)
{
  CELTContext * celt = malloc(sizeof(CELTContext));
  memset(celt, 0, sizeof(CELTContext));
  if (celt == NULL)
    return NULL;
#ifdef D_CUSTOM_CELT
  if (init_mode(celt, codec) == FALSE) {
    free(celt);
    return NULL;
  }
#else
  if (init_mode(celt, codec) == FALSE) {
    free(celt);
    return NULL;
  }
#endif
#if defined (HAVE_CELT_0_4_OR_SOONER) || defined (HAVE_CELT_0_5_0_OR_0_6_0)
  celt->decoder_state = celt_decoder_create(celt->mode->s);
#else
  #ifdef D_CUSTOM_CELT
    celt->decoder_state = celt_decoder_create_custom(celt->mode, 1, NULL);
  #else
    celt->decoder_state = celt_decoder_create_custom(celt->mode, 1, NULL);
  #endif
#endif
  if (celt->decoder_state == NULL ) {
    celt_mode_destroy(celt->mode);
    free(celt);
    return NULL;
  }

  return celt;
}


static void celt_destroy_encoder(const struct PluginCodec_Definition * codec, void * context)
{
  //if (context==NULL) return;
  CELTContext * celt = (CELTContext *)context;
  if (celt&&celt->encoder_state){
    celt_encoder_destroy(celt->encoder_state);
    celt->encoder_state=NULL;
  }
  if (celt&& celt->mode){
    celt_mode_destroy(celt->mode);
    celt->mode=NULL;
  }
  free(celt);
  celt=NULL;
}


static void celt_destroy_decoder(const struct PluginCodec_Definition * codec, void * context)
{
  CELTContext * celt = (CELTContext *)context;
  if (context==NULL) return;
  if (celt->decoder_state)
    celt_decoder_destroy(celt->decoder_state);
  if (celt->mode)
    celt_mode_destroy(celt->mode);
  free(celt);
}


static int celt_codec_encoder(const struct PluginCodec_Definition * codec,
                                                            void * context,
                                                      const void * fromPtr,
                                                        unsigned * fromLen,
                                                            void * toPtr,
                                                        unsigned * toLen,
                                                    unsigned int * flag)
{
  CELTContext *celt = (CELTContext *)context;
  int byteCount=0, fromLenShort =(*fromLen)/2 ;
  int bCount = 0;int i= 0;
  if (context == NULL) {*toLen = 0;return TRUE;}
  
  if (*fromLen < codec->parm.audio.samplesPerFrame*sizeof(short))
    return FALSE;

  if (*toLen < celt->bytes_per_packet)
    return FALSE;

#ifdef HAVE_CELT_0_4_OR_SOONER
  byteCount = celt_encode(celt->encoder_state, (celt_int16_t *)fromPtr, (char *)toPtr, celt->bytes_per_packet);
#elif HAVE_CELT_0_5_0_OR_0_6_0
  byteCount = celt_encode(celt->encoder_state, (celt_int16_t *)fromPtr, NULL, (char *)toPtr, celt->bytes_per_packet);
#else
  for(i=0;i< fromLenShort / D_MAX_FRAME_SIZE;i++){
	 byteCount += celt_encode(celt->encoder_state, ((const celt_int16 *)fromPtr) +i * D_MAX_FRAME_SIZE,   D_MAX_FRAME_SIZE  /**fromLen*/, ((unsigned char *)toPtr )+ byteCount,  *toLen - byteCount);
  }
#endif
  if (byteCount < 0) {
	return 0;
  }
  *toLen = byteCount;
  //*fromLen = codec->parm.audio.samplesPerFrame*sizeof(short);

  return TRUE; 
}


static int celt_codec_decoder(const struct PluginCodec_Definition * codec,
                                                            void * context,
                                                      const void * fromPtr,
                                                        unsigned * fromLen,
                                                            void * toPtr,
                                                        unsigned * toLen,
                                                    unsigned int * flag)
{
  int nToLen =0;
  CELTContext *celt = (CELTContext *)context;
  if (context == NULL) {*toLen = 0;return TRUE;}
  if (*toLen < codec->parm.audio.samplesPerFrame*sizeof(short))
    return FALSE;

  if (*fromLen == 0)
    return FALSE;
  //*toLen = codec->parm.audio.samplesPerFrame*sizeof(short);
 
#if defined (HAVE_CELT_0_4_OR_SOONER) || defined (HAVE_CELT_0_5_0_OR_0_6_0)
  if (celt_decode(celt->decoder_state, (char *)fromPtr, *fromLen, (short *)toPtr) < 0)
    return 0;
#else
  if (( *toLen = celt_decode(celt->decoder_state, (unsigned char *)fromPtr, *fromLen, (short *)toPtr,*toLen/*D_MAX_FRAME_SIZE*/)) < 0){
    return 0;
  }
#endif


  return TRUE;
}

/* taken from Speex */
static int valid_for_sip(
      const struct PluginCodec_Definition * codec, 
      void * context, 
      const char * key, 
      void * parm, 
      unsigned * parmLen)
{
#if defined (HAVE_CELT_0_4_OR_SOONER) || defined (HAVE_CELT_0_5_0_OR_0_6_0)
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char *))
#else
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(unsigned char *))
#endif
    return 0;

  return (STRCMPI((const char *)parm, "sip") == 0) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_ControlDefn celt_codec_controls[] = {
  { "valid_for_protocol",       valid_for_sip },
  { NULL }
};

static struct PluginCodec_information licenseInfo = {
    // Fri Dec 13 2008, 23:37:31 CET =
    1229729851,

    "Stefan Knoblich, axsentis GmbH",                            // source code author
    "0.1",                                                       // source code version
    "s.knoblich@axsentis.de",                                    // source code email
    "http://oss.axsentis.de/",                                   // source code URL
    "Copyright (C) 2008 axsentis GmbH",                          // source code copyright
    "BSD license",                                               // source code license
    PluginCodec_License_BSD,                                     // source code license
    
    "CELT (ultra-low delay audio codec)",                        // codec description
    "Jean-Marc Valin, Xiph Foundation.",                         // codec author
    "",                                                          // codec version
    "jean-marc.valin@hermes.usherb.ca",                          // codec email
    "http://www.celt-codec.org",                                 // codec URL
    "(C) 2008 Xiph.Org Foundation, All Rights Reserved",         // codec copyright information
    "Xiph BSD license",                                          // codec license
    PluginCodec_License_BSD                                      // codec license code
};

static struct PluginCodec_Definition CodecDefn[] = {
  /* 32KHz */
//  {
//    // encoder
//    PLUGIN_CODEC_VERSION_OPTIONS,           // codec API version
//    &licenseInfo,                           // license information
//
//    PluginCodec_MediaTypeAudio |            // audio codec
//    PluginCodec_InputTypeRaw |              // raw input data
//    PluginCodec_OutputTypeRaw |             // raw output data
//    PluginCodec_RTPTypeShared |
//    PluginCodec_RTPTypeDynamic,             // dynamic RTP type
//    
//    "CELT-32K",                             // text decription
//    "L16",                                  // source format
//    "CELT-32K",                             // destination format
//    
//    NULL,                                   // user data
//
//    32000,                                  // samples per second
//    32000,                                  // raw bits per second
//    10000,                                  // microseconds per frame
//    {{
//      320,                                  // samples per frame
//      40,                                   // bytes per frame
//      1,                                    // recommended number of frames per packet
//      1,                                    // maximum number of frames per packet
//    }},
//    0,                                      // IANA RTP payload code
//    "CELT",                                 // RTP payload name
//    
//    celt_create_encoder,                    // create codec function
//    celt_destroy_encoder,                   // destroy codec
//    celt_codec_encoder,                     // encode/decode
//    celt_codec_controls,                    // codec controls
//
//    PluginCodec_H323Codec_NoH323,
//    NULL
//  },
//  { 
//    // decoder
//    PLUGIN_CODEC_VERSION_OPTIONS,           // codec API version
//    &licenseInfo,                           // license information
//
//    PluginCodec_MediaTypeAudio |            // audio codec
//    PluginCodec_InputTypeRaw |              // raw input data
//    PluginCodec_OutputTypeRaw |             // raw output data
//    PluginCodec_RTPTypeShared |
//    PluginCodec_RTPTypeDynamic,             // dynamic RTP type
//
//#if 0	/* supported ???*/
//    PluginCodec_DecodeSilence,              // Can accept missing (empty) frames and generate silence
//#endif
//
//    "CELT-32K",                             // text decription
//    "CELT-32K",                             // source format
//    "L16",                                  // destination format
//
//    NULL,                                   // user data
//
//    32000,                                  // samples per second
//    32000,                                  // raw bits per second
//    10000,                                  // microseconds per frame
//    {{
//      320,                                  // samples per frame
//      40,                                   // bytes per frame
//      1,                                    // recommended number of frames per packet
//      1,                                    // maximum number of frames per packet
//    }},
//    0,                                      // IANA RTP payload code
//    "CELT",                                 // RTP payload name
//
//    celt_create_decoder,                    // create codec function
//    celt_destroy_decoder,                   // destroy codec
//    celt_codec_decoder,                     // encode/decode
//    celt_codec_controls,                    // codec controls
//    
//    PluginCodec_H323Codec_NoH323,
//    NULL
//  }
  //,
  /* 8 KHz */
//  {
//    // encoder
//    PLUGIN_CODEC_VERSION_OPTIONS,           // codec API version
//    &licenseInfo,                           // license information
//
//    PluginCodec_MediaTypeAudio |            // audio codec
//    PluginCodec_InputTypeRaw |              // raw input data
//    PluginCodec_OutputTypeRaw |             // raw output data
//    PluginCodec_RTPTypeShared |
//    PluginCodec_RTPTypeDynamic,             // dynamic RTP type
//    
//    "CELT",                             // text decription
//    "L16",                                  // source format
//    "CELT",                             // destination format
//    
//    NULL,                                   // user data
//
//    8000,                                  // samples per second
//    8000,                                  // raw bits per second
//    10000,                                  // microseconds per frame
//    {{
//      80,                                  // samples per frame
//      10,                                   // bytes per frame
//      1,                                    // recommended number of frames per packet
//      1,                                    // maximum number of frames per packet
//    }},
//    0,                                      // IANA RTP payload code
//    "CELT",                                 // RTP payload name
//    
//    celt_create_encoder,                    // create codec function
//    celt_destroy_encoder,                   // destroy codec
//    celt_codec_encoder,                     // encode/decode
//    celt_codec_controls,                    // codec controls
//
//    PluginCodec_H323Codec_NoH323,
//    NULL
//  },
//  { 
//    // decoder
//    PLUGIN_CODEC_VERSION_OPTIONS,           // codec API version
//    &licenseInfo,                           // license information
//
//    PluginCodec_MediaTypeAudio |            // audio codec
//    PluginCodec_InputTypeRaw |              // raw input data
//    PluginCodec_OutputTypeRaw |             // raw output data
//    PluginCodec_RTPTypeShared |
//    PluginCodec_RTPTypeDynamic,             // dynamic RTP type
//
//#if 0	/* supported ???*/
//    PluginCodec_DecodeSilence,              // Can accept missing (empty) frames and generate silence
//#endif
//
//    "CELT-",                             // text decription
//    "CELT-",                             // source format
//    "L16",                                  // destination format
//
//    NULL,                                   // user data
//
//     8000,                                  // samples per second
//    8000,                                  // raw bits per second
//    10000,                                  // microseconds per frame
//    {{
//      80,                                  // samples per frame
//      10,                                   // bytes per frame
//      1,                                    // recommended number of frames per packet
//      1,                                    // maximum number of frames per packet
//    }},
//    0,                                      // IANA RTP payload code
//    "CELT",                                 // RTP payload name
//
//    celt_create_decoder,                    // create codec function
//    celt_destroy_decoder,                   // destroy codec
//    celt_codec_decoder,                     // encode/decode
//    celt_codec_controls,                    // codec controls
//    
//    PluginCodec_H323Codec_NoH323,
//    NULL
//  }
  //,
  /* 48 KHz */
  {
    // encoder
    PLUGIN_CODEC_VERSION_OPTIONS,           // codec API version
    &licenseInfo,                           // license information

    PluginCodec_MediaTypeAudio |            // audio codec
    PluginCodec_InputTypeRaw |              // raw input data
    PluginCodec_OutputTypeRaw |             // raw output data
    PluginCodec_RTPTypeShared |
    PluginCodec_RTPTypeDynamic,             // dynamic RTP type
    
    "CELT-48K",                             // text decription
    "L16",                                  // source format
    "CELT-48K",                             // destination format
    
    NULL,                                   // user data

    48000,                                  // samples per second
    48000,                                  // raw bits per second
    10000,                                  // microseconds per frame
    {{
      480,                                  // samples per frame
      60,                                   // bytes per frame
      1,                                    // recommended number of frames per packet
      1,                                    // maximum number of frames per packet
    }},
    0,                                      // IANA RTP payload code
    "CELT",                                 // RTP payload name
    
    celt_create_encoder,                    // create codec function
    celt_destroy_encoder,                   // destroy codec
    celt_codec_encoder,                     // encode/decode
    celt_codec_controls,                    // codec controls

    PluginCodec_H323Codec_NoH323,
    NULL
  },
  { 
    // decoder
    PLUGIN_CODEC_VERSION_OPTIONS,           // codec API version
    &licenseInfo,                           // license information

    PluginCodec_MediaTypeAudio |            // audio codec
    PluginCodec_InputTypeRaw |              // raw input data
    PluginCodec_OutputTypeRaw |             // raw output data
    PluginCodec_RTPTypeShared |
    PluginCodec_RTPTypeDynamic,             // dynamic RTP type

#if 0	/* supported ???*/
    PluginCodec_DecodeSilence,              // Can accept missing (empty) frames and generate silence
#endif

    "CELT-48K",                             // text decription
    "CELT-48K",                             // source format
    "L16",                                  // destination format

    NULL,                                   // user data

    48000,                                  // samples per second
    48000,                                  // raw bits per second
    10000,                                  // microseconds per frame
    {{
      480,                                  // samples per frame
      60,                                   // bytes per frame
      1,                                    // recommended number of frames per packet
      1,                                    // maximum number of frames per packet
    }},
    0,                                      // IANA RTP payload code
    "CELT",                                 // RTP payload name

    celt_create_decoder,                    // create codec function
    celt_destroy_decoder,                   // destroy codec
    celt_codec_decoder,                     // encode/decode
    celt_codec_controls,                    // codec controls
    
    PluginCodec_H323Codec_NoH323,
    NULL
  }
};
 void* g_GetcodecdefPointerCelt(int * count)
{
  *count = sizeof(CodecDefn)/ sizeof(struct PluginCodec_Definition);
  return &CodecDefn[0];
}

//PLUGIN_CODEC_IMPLEMENT_ALL(CELT, celtCodecDefn, PLUGIN_CODEC_VERSION_OPTIONS)

/////////////////////////////////////////////////////////////////////////////
