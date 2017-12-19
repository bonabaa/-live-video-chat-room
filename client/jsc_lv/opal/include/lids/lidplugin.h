/*
 * lidplugins.h
 *
 * Line Interface Device plugins handler
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (C) 2006 Post Increment
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21293 $
 * $Author: rjongbloed $
 * $Date: 2008-10-12 23:24:41 +0000 (Sun, 12 Oct 2008) $
 */

#ifndef OPAL_LIDS_LIDPLUGIN_H
#define OPAL_LIDS_LIDPLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

#ifdef _WIN32
#  ifdef PLUGIN_DLL_EXPORTS
#    define PLUGIN_DLL_API __declspec(dllexport)
#  else
#    define PLUGIN_DLL_API __declspec(dllimport)
#  endif

#else

#define PLUGIN_DLL_API

#endif

#ifdef PWLIB_PLUGIN_API_VERSION
#undef PWLIB_PLUGIN_API_VERSION
#endif
#define PWLIB_PLUGIN_API_VERSION 1

////////////////////////////////////////////////////////////////////////////////
//
//  LID Plugins

#define	PLUGIN_LID_VERSION 1    // initial version

typedef int PluginLID_Boolean;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif


typedef enum PluginLID_Errors {
  PluginLID_NoError = 0,
  PluginLID_UnimplementedFunction,
  PluginLID_BadContext,
  PluginLID_InvalidParameter,
  PluginLID_NoSuchDevice,
  PluginLID_DeviceOpenFailed,
  PluginLID_UsesSoundChannel,
  PluginLID_DeviceNotOpen,
  PluginLID_NoSuchLine,
  PluginLID_OperationNotAllowed,
  PluginLID_NoMoreNames,
  PluginLID_BufferTooSmall,
  PluginLID_UnsupportedMediaFormat,
  PluginLID_NoDialTone,
  PluginLID_LineBusy,
  PluginLID_NoAnswer,
  PluginLID_Aborted,
  PluginLID_InternalError,
  PluginLID_NumErrors
} PluginLID_Errors;


enum PluginLID_CallProgressTones {
  PluginLID_NoTone       = 0x00,   // indicates no tones
  PluginLID_DialTone     = 0x01,   // Dial tone
  PluginLID_RingTone     = 0x02,   // Ring indication tone
  PluginLID_BusyTone     = 0x04,   // Line engaged tone
  PluginLID_FastBusyTone = 0x08,   // fast busy tone
  PluginLID_ClearTone    = 0x10,   // Call failed/cleared tone (often same as busy tone)
  PluginLID_CNGTone      = 0x20,   // Fax CNG tone
  PluginLID_MwiTone      = 0x40,   // Message Waiting Tone
  PluginLID_AllTones     = 0x4f
};


typedef struct PluginLID_DialParams
{
  bool     m_requireTones;      ///< Require dial/ring tone to be detected
  unsigned m_dialToneTimeout;   ///< Time in msec to wait for a dial tone to be detected
  unsigned m_dialStartDelay;    ///< Time in msec to wait between the dial tone detection and dialing the dtmf
  unsigned m_progressTimeout;   ///< Time in msec to wait for a progress tone (ring, busy or connected) to be detected
  unsigned m_commaDelay;        ///< Time in msec to wait when a comma (',') is found in the dial string
} PluginLID_DialParams;


typedef struct PluginLID_Definition
{
  unsigned int apiVersion;  // structure version

  // start of version 1 fields
  time_t timestamp;                 // creation time and date - obtain with command: date -u "+%c = %s"

  const char * name;                // LID name text
  const char * description;         // LID description text
  const char * manufacturer;        // LID manufacturer name
  const char * model;               // LID model name
  const char * revision;            // LID hardware revision number
  const char * manufacturerEmail;   // LID manufacturer email contact information
  const char * manufacturerURL;     // LID manufacturer web site

  const char * author;              // source code author
  const char * authorEmail;         // source code email contact information
  const char * authorURL;           // source code web site
  const char * copyright;           // source code copyright
  const char * license;             // source code license
  const char * version;             // source code version

  const void * userData;            // user data value

  void * (*Create)(const struct PluginLID_Definition * definition);
  void (*Destroy)(const struct PluginLID_Definition * definition,  void * context);

  PluginLID_Errors (*GetDeviceName)(void * context, unsigned index, char * name, unsigned size);
  PluginLID_Errors (*Open)(void * context, const char * device);
  PluginLID_Errors (*Close)(void * context);

  PluginLID_Errors (*GetLineCount)(void * context, unsigned * count);
  PluginLID_Errors (*IsLineTerminal)(void * context, unsigned line, PluginLID_Boolean * isTerminal);
  PluginLID_Errors (*IsLinePresent)(void * context, unsigned line, PluginLID_Boolean forceTest, PluginLID_Boolean * present);
  PluginLID_Errors (*IsLineOffHook)(void * context, unsigned line, PluginLID_Boolean * offHook);
  PluginLID_Errors (*SetLineOffHook)(void * context, unsigned line, PluginLID_Boolean newState);
  PluginLID_Errors (*HookFlash)(void * context, unsigned line, unsigned flashTime);
  PluginLID_Errors (*HasHookFlash)(void * context, unsigned line, PluginLID_Boolean * flashed);
  PluginLID_Errors (*IsLineRinging)(void * context, unsigned line, unsigned long * cadence);
  PluginLID_Errors (*RingLine)(void * context, unsigned line, unsigned nCadence, const unsigned * pattern, unsigned frequency);
  PluginLID_Errors (*IsLineDisconnected)(void * context, unsigned line, PluginLID_Boolean checkForWink, PluginLID_Boolean * disconnected);
  PluginLID_Errors (*SetLineToLineDirect)(void * context, unsigned line1, unsigned line2, PluginLID_Boolean connect);
  PluginLID_Errors (*IsLineToLineDirect)(void * context, unsigned line1, unsigned line2, PluginLID_Boolean * connected);

  PluginLID_Errors (*GetSupportedFormat)(void * context, unsigned index, char * mediaFormat, unsigned size);
  PluginLID_Errors (*SetReadFormat)(void * context, unsigned line, const char * mediaFormat);
  PluginLID_Errors (*SetWriteFormat)(void * context, unsigned line, const char * mediaFormat);
  PluginLID_Errors (*GetReadFormat)(void * context, unsigned line, char * mediaFormat, unsigned size);
  PluginLID_Errors (*GetWriteFormat)(void * context, unsigned line, char * mediaFormat, unsigned size);
  PluginLID_Errors (*StopReading)(void * context, unsigned line);
  PluginLID_Errors (*StopWriting)(void * context, unsigned line);
  PluginLID_Errors (*SetReadFrameSize)(void * context, unsigned line, unsigned frameSize);
  PluginLID_Errors (*SetWriteFrameSize)(void * context, unsigned line, unsigned frameSize);
  PluginLID_Errors (*GetReadFrameSize)(void * context, unsigned line, unsigned * frameSize);
  PluginLID_Errors (*GetWriteFrameSize)(void * context, unsigned line, unsigned * frameSize);
  PluginLID_Errors (*ReadFrame)(void * context, unsigned line, void * buffer, unsigned * count);
  PluginLID_Errors (*WriteFrame)(void * context, unsigned line, const void * buffer, unsigned count, unsigned * written);

  PluginLID_Errors (*GetAverageSignalLevel)(void * context, unsigned line, PluginLID_Boolean playback, unsigned * signal);

  PluginLID_Errors (*EnableAudio)(void * context, unsigned line, PluginLID_Boolean enable);
  PluginLID_Errors (*IsAudioEnabled)(void * context, unsigned line, PluginLID_Boolean * enable);
  PluginLID_Errors (*SetRecordVolume)(void * context, unsigned line, unsigned volume);
  PluginLID_Errors (*SetPlayVolume)(void * context, unsigned line, unsigned volume);
  PluginLID_Errors (*GetRecordVolume)(void * context, unsigned line, unsigned * volume);
  PluginLID_Errors (*GetPlayVolume)(void * context, unsigned line, unsigned * volume);

  PluginLID_Errors (*GetAEC)(void * context, unsigned line, unsigned * level);
  PluginLID_Errors (*SetAEC)(void * context, unsigned line, unsigned level);

  PluginLID_Errors (*GetVAD)(void * context, unsigned line, PluginLID_Boolean * enable);
  PluginLID_Errors (*SetVAD)(void * context, unsigned line, PluginLID_Boolean enable);

  PluginLID_Errors (*GetCallerID)(void * context, unsigned line, char * idString, unsigned size, PluginLID_Boolean full);
  PluginLID_Errors (*SetCallerID)(void * context, unsigned line, const char * idString);
  PluginLID_Errors (*SendVisualMessageWaitingIndicator)(void * context, unsigned line, PluginLID_Boolean on);

  PluginLID_Errors (*PlayDTMF)(void * context, unsigned line, const char * digits, unsigned onTime, unsigned offTime);
  PluginLID_Errors (*ReadDTMF)(void * context, unsigned line, char * digit);
  PluginLID_Errors (*GetRemoveDTMF)(void * context, unsigned line, PluginLID_Boolean * removeTones);
  PluginLID_Errors (*SetRemoveDTMF)(void * context, unsigned line, PluginLID_Boolean removeTones);

  PluginLID_Errors (*IsToneDetected)(void * context, unsigned line, int * tone);
  PluginLID_Errors (*WaitForToneDetect)(void * context, unsigned line, unsigned timeout, int * tone);
  PluginLID_Errors (*WaitForTone)(void * context, unsigned line, int tone, unsigned timeout);

  PluginLID_Errors (*SetToneParameters)(void * context, unsigned line,
                                        unsigned tone,
                                        unsigned lowFrequency,
                                        unsigned highFrequency,
                                        unsigned mixingMode,
                                        unsigned numCadences,
                                        const unsigned * onTimes,
                                        const unsigned * offTimes);
  PluginLID_Errors (*PlayTone)(void * context, unsigned line, unsigned tone);
  PluginLID_Errors (*IsTonePlaying)(void * context, unsigned line, PluginLID_Boolean * playing);
  PluginLID_Errors (*StopTone)(void * context, unsigned line);

  PluginLID_Errors (*DialOut)(void * context, unsigned line, const char * number, struct PluginLID_DialParams * params);

  PluginLID_Errors (*GetWinkDuration)(void * context, unsigned line, unsigned * winkDuration);
  PluginLID_Errors (*SetWinkDuration)(void * context, unsigned line, unsigned winkDuration);

  PluginLID_Errors (*SetCountryCode)(void * context, unsigned country);

  PluginLID_Errors (*GetSupportedCountry)(void * context, unsigned index, unsigned * countryCode);
  // end of version 1 fields

  PluginLID_Errors (*SetLineConnected)(void * context, unsigned line);
  PluginLID_Errors (*IsLineConnected)(void * context, unsigned line, PluginLID_Boolean * connected);

} PluginLID_Definition;


#ifdef __cplusplus

#define PLUGIN_LID_CTOR() \
    static void * Create(const struct PluginLID_Definition * definition) { return new Context; } \
    Context()

#define PLUGIN_LID_DTOR() \
    static void Destroy(const struct PluginLID_Definition * definition,  void * context) { delete (Context *)context; } \
    ~Context()

#define PLUGIN_FUNCTION_ARG0(fn) \
     static PluginLID_Errors fn(void * context) { return context == NULL ? PluginLID_BadContext : ((Context *)context)->fn(); } \
            PluginLID_Errors fn(              )

#define PLUGIN_FUNCTION_ARG0(fn) \
     static PluginLID_Errors fn(void * context) { return context == NULL ? PluginLID_BadContext : ((Context *)context)->fn(); } \
            PluginLID_Errors fn(              )

#define PLUGIN_FUNCTION_ARG1(fn,                type1,var1) \
     static PluginLID_Errors fn(void * context, type1 var1) { return context == NULL ? PluginLID_BadContext : ((Context *)context)->fn(var1); } \
            PluginLID_Errors fn(                type1 var1)

#define PLUGIN_FUNCTION_ARG2(fn,                type1,var1, type2,var2) \
     static PluginLID_Errors fn(void * context, type1 var1, type2 var2) { return context == NULL ? PluginLID_BadContext : ((Context *)context)->fn(var1, var2); } \
            PluginLID_Errors fn(                type1 var1, type2 var2)

#define PLUGIN_FUNCTION_ARG3(fn,                type1,var1, type2,var2, type3,var3) \
     static PluginLID_Errors fn(void * context, type1 var1, type2 var2, type3 var3) { return context == NULL ? PluginLID_BadContext : ((Context *)context)->fn(var1, var2, var3); } \
            PluginLID_Errors fn(                type1 var1, type2 var2, type3 var3)

#define PLUGIN_FUNCTION_ARG4(fn,                type1,var1, type2,var2, type3,var3, type4,var4) \
     static PluginLID_Errors fn(void * context, type1 var1, type2 var2, type3 var3, type4 var4) { return context == NULL ? PluginLID_BadContext : ((Context *)context)->fn(var1, var2, var3, var4); } \
            PluginLID_Errors fn(                type1 var1, type2 var2, type3 var3, type4 var4)

#define PLUGIN_FUNCTION_ARG8(fn,                type1,var1, type2,var2, type3,var3, type4,var4, type5,var5, type6,var6, type7,var7, type8,var8) \
     static PluginLID_Errors fn(void * context, type1 var1, type2 var2, type3 var3, type4 var4, type5 var5, type6 var6, type7 var7, type8 var8) { return context == NULL ? PluginLID_BadContext : ((Context *)context)->fn(var1, var2, var3, var4, var5, var6, var7, var8); } \
            PluginLID_Errors fn(                type1 var1, type2 var2, type3 var3, type4 var4, type5 var5, type6 var6, type7 var7, type8 var8)

#endif // __cplusplus


#define PLUGIN_LID_API_VER_FN       PWLibPlugin_GetAPIVersion
#define PLUGIN_LID_API_VER_FN_STR   "PWLibPlugin_GetAPIVersion"

#define PLUGIN_LID_GET_LIDS_FN     OpalPluginLID_GetDefinitions
#define PLUGIN_LID_GET_LIDS_FN_STR "OpalPluginLID_GetDefinitions"


typedef struct PluginLID_Definition * (* PluginLID_GetDefinitionsFunction)(unsigned * /*count*/, unsigned /*version*/);


#define PLUGIN_LID_IMPLEMENTATION(defs) \
  extern "C" { \
    PLUGIN_DLL_API unsigned int PLUGIN_LID_API_VER_FN() { return PWLIB_PLUGIN_API_VERSION; } \
    PLUGIN_DLL_API PluginLID_Definition * PLUGIN_LID_GET_LIDS_FN(unsigned * count, unsigned version) \
      { *count = sizeof(defs)/sizeof(defs[0]); return defs; } \
  }


#ifdef __cplusplus
};
#endif

#endif // OPAL_LIDS_LIDPLUGIN_H
