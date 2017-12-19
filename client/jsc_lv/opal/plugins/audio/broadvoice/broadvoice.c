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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./FloatingPoint/bvcommon/typedef.h"
#include "./FloatingPoint/bv32/bv32cnst.h"
#include "./FloatingPoint/bvcommon/bvcommon.h"
#include "./FloatingPoint/bv32/bv32strct.h"
#include "./FloatingPoint/bv32/bv32.h"
#include "./FloatingPoint/bvcommon/utility.h"
#if G192BITSTREAM
#include "g192.h"
#else
#include "./FloatingPoint/bv32/bitpack.h"
#endif

#include <codec/opalplugin.h>
//#include "celt/libcelt/celt.h"
#define D_BV32_BITPACK_20 20
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

typedef struct {
	struct BV32_Bit_Stream bs;
	struct BV32_Encoder_State cs;
	struct BV32_Decoder_State ds;
#if !G192BITSTREAM
   UWord8 PackedStream[20];
#endif
}stContext;
/////////////////////////////////////////////////////////////////////////////

static void * broadvoice_create_encoder(const struct PluginCodec_Definition * codec)
{
  stContext * celt = malloc(sizeof(stContext));
  memset(celt, 0, sizeof(stContext));
  if (celt == NULL)
    return NULL;
 
  Reset_BV32_Coder(&celt->cs);
  return celt;
}


static void * broadvoice_create_decoder(const struct PluginCodec_Definition * codec)
{
  stContext * celt = malloc(sizeof(stContext));
  memset(celt, 0, sizeof(stContext));
  if (celt == NULL)
    return NULL;
 
  Reset_BV32_Decoder(&celt->ds);

  return celt;
}


static void broadvoice_destroy_encoder(const struct PluginCodec_Definition * codec, void * context)
{
  //if (context==NULL) return;
  stContext * celt = (stContext *)context;
 
  free(celt);
  celt=NULL;
}


static void broadvoice_destroy_decoder(const struct PluginCodec_Definition * codec, void * context)
{
  stContext * celt = (stContext *)context;
  free(celt);celt=NULL;
}


static int broadvoice_codec_encoder(const struct PluginCodec_Definition * codec,
                                                            void * context,
                                                      const void * fromPtr,
                                                        unsigned * fromLen,
                                                            void * toPtr,
                                                        unsigned * toLen,
                                                    unsigned int * flag)
{
  stContext *celt = (stContext *)context;
  int byteCount=0, fromLenShort =(*fromLen)/2 ;
  int i= 0;
   
  if (*fromLen < codec->parm.audio.samplesPerFrame*sizeof(short))
    return FALSE;

 
#ifdef HAVE_CELT_0_4_OR_SOONER
  byteCount = celt_encode(celt->encoder_state, (celt_int16_t *)fromPtr, (char *)toPtr, celt->bytes_per_packet);
#elif HAVE_CELT_0_5_0_OR_0_6_0
  byteCount = celt_encode(celt->encoder_state, (celt_int16_t *)fromPtr, NULL, (char *)toPtr, celt->bytes_per_packet);
#else
  for(i=0;i< fromLenShort / FRSZ;i++){
	  short* pX = ((short*)fromPtr) + i * FRSZ;
	  BV32_Encode(&celt->bs, &celt->cs, pX); 
	  BV32_BitPack( celt->PackedStream, &celt->bs );
	  
	  memcpy(((unsigned char *)toPtr )+ byteCount, celt->PackedStream, D_BV32_BITPACK_20);
	  byteCount += 20;//celt_encode(celt->encoder_state, ((const celt_int16 *)fromPtr) +i * D_MAX_FRAME_SIZE,   D_MAX_FRAME_SIZE  /**fromLen*/, ((unsigned char *)toPtr )+ byteCount,  *toLen - byteCount);
  }
#endif
  if (byteCount < 0) {
	return 0;
  }
  *toLen = byteCount;
  //*fromLen = codec->parm.audio.samplesPerFrame*sizeof(short);

  return TRUE; 
}


static int broadvoice_codec_decoder(const struct PluginCodec_Definition * codec,
                                                            void * context,
                                                      const void * fromPtr,
                                                        unsigned * fromLen,
                                                            void * toPtr,
                                                        unsigned * toLen,
                                                    unsigned int * flag)
{
   stContext *celt = (stContext *)context;
   int i=0;
   int byteCount=0;
   if (*toLen < codec->parm.audio.samplesPerFrame*sizeof(short))
    return FALSE;

  if (*fromLen == 0)
    return FALSE;
  //*toLen = codec->parm.audio.samplesPerFrame*sizeof(short);
 
#if defined (HAVE_CELT_0_4_OR_SOONER) || defined (HAVE_CELT_0_5_0_OR_0_6_0)
  if (celt_decode(celt->decoder_state, (char *)fromPtr, *fromLen, (short *)toPtr) < 0)
    return 0;
#else
   for(i=0;i< *fromLen / D_BV32_BITPACK_20;i++){
	  UWord8* p=((UWord8*)fromPtr) +D_BV32_BITPACK_20 *i; 
	  BV32_BitUnPack(p/* celt->PackedStream*/, &celt->bs );
   	  BV32_Decode(&celt->bs, &celt->ds, ((short*) toPtr) + i* FRSZ);          
	  //memcpy(((unsigned char *)toPtr )+ byteCount, PackedStream, 20);
	  (byteCount) += FRSZ*2;//celt_encode(celt->encoder_state, ((const celt_int16 *)fromPtr) +i * D_MAX_FRAME_SIZE,   D_MAX_FRAME_SIZE  /**fromLen*/, ((unsigned char *)toPtr )+ byteCount,  *toLen - byteCount);
  }
  if (byteCount  < 0){
    return 0;
  }
#endif
	*toLen = byteCount;

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

static struct PluginCodec_ControlDefn broadvoice_codec_controls[] = {
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
  {
    // encoder
    PLUGIN_CODEC_VERSION_OPTIONS,           // codec API version
    &licenseInfo,                           // license information

    PluginCodec_MediaTypeAudio |            // audio codec
    PluginCodec_InputTypeRaw |              // raw input data
    PluginCodec_OutputTypeRaw |             // raw output data
    PluginCodec_RTPTypeShared |
    PluginCodec_RTPTypeDynamic,             // dynamic RTP type
    
    "CELT-32K",                             // text decription
    "L16",                                  // source format
    "CELT-32K",                             // destination format
    
    NULL,                                   // user data

    32000,                                  // samples per second
    32000,                                  // raw bits per second
    10000,                                  // microseconds per frame
    {{
      320,                                  // samples per frame
      40,                                   // bytes per frame
      1,                                    // recommended number of frames per packet
      1,                                    // maximum number of frames per packet
    }},
    0,                                      // IANA RTP payload code
    "CELT",                                 // RTP payload name
    
    broadvoice_create_encoder,                    // create codec function
    broadvoice_destroy_encoder,                   // destroy codec
    broadvoice_codec_encoder,                     // encode/decode
    broadvoice_codec_controls,                    // codec controls

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

    "CELT-32K",                             // text decription
    "CELT-32K",                             // source format
    "L16",                                  // destination format

    NULL,                                   // user data

    32000,                                  // samples per second
    32000,                                  // raw bits per second
    10000,                                  // microseconds per frame
    {{
      320,                                  // samples per frame
      40,                                   // bytes per frame
      1,                                    // recommended number of frames per packet
      1,                                    // maximum number of frames per packet
    }},
    0,                                      // IANA RTP payload code
    "CELT",                                 // RTP payload name

    broadvoice_create_decoder,                    // create codec function
    broadvoice_destroy_decoder,                   // destroy codec
    broadvoice_codec_decoder,                     // encode/decode
    broadvoice_codec_controls,                    // codec controls
    
    PluginCodec_H323Codec_NoH323,
    NULL
  }
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
//    broadvoice_create_encoder,                    // create codec function
//    broadvoice_destroy_encoder,                   // destroy codec
//    broadvoice_codec_encoder,                     // encode/decode
//    broadvoice_codec_controls,                    // codec controls
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
//    broadvoice_create_decoder,                    // create codec function
//    broadvoice_destroy_decoder,                   // destroy codec
//    broadvoice_codec_decoder,                     // encode/decode
//    broadvoice_codec_controls,                    // codec controls
//    
//    PluginCodec_H323Codec_NoH323,
//    NULL
//  }
  //,
  /* 48 KHz */
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
//    "CELT-48K",                             // text decription
//    "L16",                                  // source format
//    "CELT-48K",                             // destination format
//    
//    NULL,                                   // user data
//
//    48000,                                  // samples per second
//    48000,                                  // raw bits per second
//    10000,                                  // microseconds per frame
//    {{
//      480,                                  // samples per frame
//      60,                                   // bytes per frame
//      1,                                    // recommended number of frames per packet
//      1,                                    // maximum number of frames per packet
//    }},
//    0,                                      // IANA RTP payload code
//    "CELT",                                 // RTP payload name
//    
//    broadvoice_create_encoder,                    // create codec function
//    broadvoice_destroy_encoder,                   // destroy codec
//    broadvoice_codec_encoder,                     // encode/decode
//    broadvoice_codec_controls,                    // codec controls
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
//    "CELT-48K",                             // text decription
//    "CELT-48K",                             // source format
//    "L16",                                  // destination format
//
//    NULL,                                   // user data
//
//    48000,                                  // samples per second
//    48000,                                  // raw bits per second
//    10000,                                  // microseconds per frame
//    {{
//      480,                                  // samples per frame
//      60,                                   // bytes per frame
//      1,                                    // recommended number of frames per packet
//      1,                                    // maximum number of frames per packet
//    }},
//    0,                                      // IANA RTP payload code
//    "CELT",                                 // RTP payload name
//
//    broadvoice_create_decoder,                    // create codec function
//    broadvoice_destroy_decoder,                   // destroy codec
//    broadvoice_codec_decoder,                     // encode/decode
//    broadvoice_codec_controls,                    // codec controls
//    
//    PluginCodec_H323Codec_NoH323,
//    NULL
//  }
};
 void* g_GetcodecdefPointerBroadVoice(int * count)
{
  *count = sizeof(CodecDefn)/ sizeof(struct PluginCodec_Definition);
  return &CodecDefn[0];
}

//PLUGIN_CODEC_IMPLEMENT_ALL(CELT, celtCodecDefn, PLUGIN_CODEC_VERSION_OPTIONS)

/////////////////////////////////////////////////////////////////////////////
