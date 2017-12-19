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
#ifdef _WIN32
#define D_YYCODE_CORE   "IVoice"
const  char* YYCODE_CORE = D_YYCODE_CORE;
#endif
#include <codec/opalplugin.h>
//#include "celt/libcelt/celt.h"
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

/////////////////////////////////////////////////////////////////////////////

static void * IVoicecreate_encoder(const struct PluginCodec_Definition * codec)
{
 
  return 1;
}


static void * IVoicecreate_decoder(const struct PluginCodec_Definition * codec)
{
 

  return 1;
}


static void IVoicedestroy_encoder(const struct PluginCodec_Definition * codec, void * context)
{

}


static void IVoicedestroy_decoder(const struct PluginCodec_Definition * codec, void * context)
{;
}


static int IVoicecodec_encoder(const struct PluginCodec_Definition * codec,
                                                            void * context,
                                                      const void * fromPtr,
                                                        unsigned * fromLen,
                                                            void * toPtr,
                                                        unsigned * toLen,
                                                    unsigned int * flag)
{
 

  return TRUE; 
}


static int IVoicecodec_decoder(const struct PluginCodec_Definition * codec,
                                                            void * context,
                                                      const void * fromPtr,
                                                        unsigned * fromLen,
                                                            void * toPtr,
                                                        unsigned * toLen,
                                                    unsigned int * flag)
{
   

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

static struct PluginCodec_ControlDefn IVoicecodec_controls[] = {
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
    
    D_YYCODE_CORE/*"IVoice-32K"*/,                             // text decription
    "L16",                                  // source format
    D_YYCODE_CORE/*"IVoice-32K"*/,                             // destination format
    
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
    D_YYCODE_CORE,                                 // RTP payload name
    
    IVoicecreate_encoder,                    // create codec function
    IVoicedestroy_encoder,                   // destroy codec
    IVoicecodec_encoder,                     // encode/decode
    IVoicecodec_controls,                    // codec controls

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

    D_YYCODE_CORE/*"IVoice-32K"*/,                             // text decription
    D_YYCODE_CORE/* "IVoice-32K"*/,                             // source format
    "L16",                                   // destination format

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
    D_YYCODE_CORE,                                 // RTP payload name

    IVoicecreate_decoder,                    // create codec function
    IVoicedestroy_decoder,                   // destroy codec
    IVoicecodec_decoder,                     // encode/decode
    IVoicecodec_controls,                    // codec controls
    
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
//    IVoicecreate_encoder,                    // create codec function
//    IVoicedestroy_encoder,                   // destroy codec
//    IVoicecodec_encoder,                     // encode/decode
//    IVoicecodec_controls,                    // codec controls
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
//    IVoicecreate_decoder,                    // create codec function
//    IVoicedestroy_decoder,                   // destroy codec
//    IVoicecodec_decoder,                     // encode/decode
//    IVoicecodec_controls,                    // codec controls
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
//    IVoicecreate_encoder,                    // create codec function
//    IVoicedestroy_encoder,                   // destroy codec
//    IVoicecodec_encoder,                     // encode/decode
//    IVoicecodec_controls,                    // codec controls
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
//    IVoicecreate_decoder,                    // create codec function
//    IVoicedestroy_decoder,                   // destroy codec
//    IVoicecodec_decoder,                     // encode/decode
//    IVoicecodec_controls,                    // codec controls
//    
//    PluginCodec_H323Codec_NoH323,
//    NULL
//  }
};
 void* g_GetcodecdefPointerIVoice(int * count)
{
  *count = sizeof(CodecDefn)/ sizeof(struct PluginCodec_Definition);
  return &CodecDefn[0];
}

//PLUGIN_CODEC_IMPLEMENT_ALL(CELT, celtCodecDefn, PLUGIN_CODEC_VERSION_OPTIONS)

/////////////////////////////////////////////////////////////////////////////
