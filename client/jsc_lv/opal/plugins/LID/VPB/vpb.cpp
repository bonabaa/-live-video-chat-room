/*
 * Voicetronix VPB Plugin LID for OPAL
 *
 * Copyright (C) 2006 Post Increment, All Rights Reserved
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
 * The Original Code is OPAL.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: vpb.cpp,v $
 * Revision 1.6  2006/11/05 05:04:46  rjongbloed
 * Improved the terminal LID line ringing, epecially for country emulation.
 *
 * Revision 1.5  2006/10/25 22:26:16  rjongbloed
 * Changed LID tone handling to use new tone generation for accurate country based tones.
 *
 * Revision 1.4  2006/10/16 09:46:49  rjongbloed
 * Fixed various MSVC 8 warnings
 *
 * Revision 1.3  2006/10/15 06:15:42  rjongbloed
 * Fixed windows/unix compatibility issue.
 *
 * Revision 1.2  2006/10/04 04:21:16  rjongbloed
 * Fixed VPB plug in build under linux
 *
 * Revision 1.1  2006/10/02 13:30:53  rjongbloed
 * Added LID plug ins
 *
 */

#define _CRT_SECURE_NO_DEPRECATE
#define PLUGIN_DLL_EXPORTS
#include <lids/lidplugin.h>

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <cstdio>

#include "vpbapi.h"

#if defined(_MSC_VER)
#pragma comment(lib, "libvpb.lib")
#endif


static const struct {
  const char *   mediaFormat;
  unsigned short mode;
} CodecInfo[] = {
  { "PCM-16",         VPB_LINEAR },
  { "G.711-uLaw-64k", VPB_MULAW },
  { "G.711-ALaw-64k", VPB_ALAW  },
};


class Context
{
  protected:
    unsigned m_uiLineCount;
    enum { MaxLineCount = 12 };

    struct LineState
    {
      PluginLID_Boolean Open(unsigned cardNumber, unsigned lineNumber)
      {
        /* +1 is used for driver 2.4.8*/
        handle = vpb_open(cardNumber, lineNumber);
        if (handle < 0)
          return FALSE;
        readFrameSize = writeFrameSize = 480;
        currentHookState = FALSE;
        vpb_sethook_sync(handle, VPB_ONHOOK);
        vpb_set_event_mask(handle, VPB_MRING | VPB_MTONEDETECT );
        return TRUE;
      }

      PluginLID_Boolean SetLineOffHook(PluginLID_Boolean newState)
      {
        try {
          if (vpb_sethook_sync(handle, newState ? VPB_OFFHOOK : VPB_ONHOOK) < 0)
            return FALSE;

          // clear DTMF buffer and event queue after changing hook state.
          vpb_flush_digits(handle);

          VPB_EVENT event;
          while (vpb_get_event_ch_async(handle, &event) == VPB_OK)
          {
          }
        }
        catch (VpbException v) {
          std::cerr << "VPB\tSetLineOffHook " << v.code << ", s = " << v.s << ", api func = " << v.api_function << std::endl;
          return FALSE;
        }

        currentHookState = newState;
        return TRUE;
      }

      int               handle;
      PluginLID_Boolean currentHookState;
      const char *      readFormat;
      const char *      writeFormat;
      size_t            readFrameSize, writeFrameSize;
    } lineState[MaxLineCount];

  public:
    PLUGIN_LID_CTOR()
    {
      m_uiLineCount = 0;
      memset(lineState, 0, sizeof(lineState));
    }

    PLUGIN_LID_DTOR()
    {
      Close();
    }


    PLUGIN_FUNCTION_ARG3(GetDeviceName,unsigned,index, char *,name, unsigned,size)
    {
      if (name == NULL || size < 3)
        return PluginLID_InvalidParameter;

      if (index > 99) // So sprintf later cannot possibly overflow buffer
        return PluginLID_NoMoreNames;

      try {
        int iHandle = vpb_open(index, 1 /* 0 for driver 3.01*/);
        if (iHandle >= 0) {
          int lineCount = vpb_get_ports_per_card(/*index*/);
          vpb_close(iHandle);
          if (lineCount > 0) {
            sprintf(name, "%u", index);
            return PluginLID_NoError;
          }
        }
      }
      catch (VpbException v) {
        std::cerr << "VPB\tOpalVpbDevice::GetLineCount Error code = "  << v.code << ", s = " << v.s << " api func = " << v.api_function << std::endl;
      }
      return PluginLID_NoMoreNames;
    }


    PLUGIN_FUNCTION_ARG1(Open,const char *,device)
    {
      Close();

      int cardNumber = atoi(device);
      int iHandle = vpb_open(cardNumber, /* 0 for driver 3.01*/1);
      /* get the number of ports for this card */
      m_uiLineCount = vpb_get_ports_per_card(/*cardNumber*/); 
      vpb_close(iHandle);

      if (m_uiLineCount == 0)
        return PluginLID_NoSuchDevice;

      try {
        for(unsigned uiLineCount = 0; uiLineCount < m_uiLineCount; uiLineCount++)
          lineState[uiLineCount].Open(cardNumber, uiLineCount);
      }
      catch (VpbException v) {
        std::cerr << "VPB\tOpalVpbDevice::Open Error code = "  << v.code << ", s = " << v.s << " api func = " << v.api_function << std::endl;
        m_uiLineCount = 0;
        return PluginLID_DeviceOpenFailed;
      }

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG0(Close)
    {
      if (m_uiLineCount == 0)
        return PluginLID_NoError;

      try {
        for(unsigned uiLineCount = 0; uiLineCount < m_uiLineCount; uiLineCount++) {
          SetLineOffHook(uiLineCount, FALSE);
          vpb_close(lineState[uiLineCount].handle);
        }  
      }
      catch (VpbException v) {
        std::cerr << "VPB\tOpalVpbDevice::Close Error code = " << v.code << ", s = " << v.s << ", api func = " << v.api_function << std::endl;
      }
  
      m_uiLineCount = 0;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(GetLineCount, unsigned *,count)
    {
      if (count == NULL)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      *count = m_uiLineCount;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineTerminal, unsigned,line, PluginLID_Boolean *,isTerminal)
    {
      if (isTerminal == NULL)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      *isTerminal = FALSE;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(IsLinePresent, unsigned,line, PluginLID_Boolean,forceTest, PluginLID_Boolean *,present)
    {
      if (present == NULL)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      *present = TRUE;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineOffHook, unsigned,line, PluginLID_Boolean *,offHook)
    {
      if (offHook == NULL)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      *offHook = lineState[line].currentHookState;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetLineOffHook, unsigned,line, PluginLID_Boolean,newState)
    {
      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      return lineState[line].SetLineOffHook(newState) ? PluginLID_NoError : PluginLID_InternalError;
    }


    //PLUGIN_FUNCTION_ARG2(HookFlash, unsigned,line, unsigned,flashTime)
    //PLUGIN_FUNCTION_ARG2(HasHookFlash, unsigned,line, PluginLID_Boolean *,flashed)

    PLUGIN_FUNCTION_ARG2(IsLineRinging, unsigned,line, unsigned long *,cadence)
    {
      if (cadence == NULL)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      *cadence = 0; // Not ringing

      if (!lineState[line].currentHookState) {
        // DR 13/1/02 - Dont look at event queue here if off hook, as we will steal events 
        // that IsToneDetected may be looking for.
        VPB_EVENT event;
        if (vpb_get_event_ch_async(lineState[line].handle, &event) == VPB_OK && event.type == VPB_RING)
          *cadence = 1;
      }
      return PluginLID_NoError;
    }

    //PLUGIN_FUNCTION_ARG4(RingLine, unsigned,line, unsigned,nCadence, const unsigned *,pattern, unsigned,frequency)
    //PLUGIN_FUNCTION_ARG3(IsLineDisconnected, unsigned,line, PluginLID_Boolean,checkForWink, PluginLID_Boolean *,disconnected)
    //PLUGIN_FUNCTION_ARG3(SetLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean,connect)
    //PLUGIN_FUNCTION_ARG3(IsLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean *,connected)


    PLUGIN_FUNCTION_ARG3(GetSupportedFormat, unsigned,index, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL || size < 2)
        return PluginLID_InvalidParameter;

      if (index >= sizeof(CodecInfo)/sizeof(CodecInfo))
        return PluginLID_NoMoreNames;

      strncpy(mediaFormat, CodecInfo[index].mediaFormat, size-1);
      mediaFormat[size-1] = '\0';
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetReadFormat, unsigned,line, const char *,mediaFormat)
    {
      if (mediaFormat == NULL)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      for (int i = 0; i < sizeof(CodecInfo)/sizeof(CodecInfo); i++) {
        if (strcmp(mediaFormat, CodecInfo[i].mediaFormat) == 0) {
          if (vpb_record_buf_start(lineState[line].handle, CodecInfo[i].mode) < 0)
            return PluginLID_InternalError;
          lineState[line].readFormat = CodecInfo[i].mediaFormat;
          return PluginLID_NoError;
        }
      }

      return PluginLID_UnsupportedMediaFormat;
    }


    PLUGIN_FUNCTION_ARG2(SetWriteFormat, unsigned,line, const char *,mediaFormat)
    {
      if (mediaFormat == NULL)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      for (int i = 0; i < sizeof(CodecInfo)/sizeof(CodecInfo); i++) {
        if (strcmp(mediaFormat, CodecInfo[i].mediaFormat) == 0) {
          if (vpb_play_buf_start(lineState[line].handle, CodecInfo[i].mode) < 0)
            return PluginLID_InternalError;
          lineState[line].writeFormat = CodecInfo[i].mediaFormat;
          return PluginLID_NoError;
        }
      }

      return PluginLID_UnsupportedMediaFormat;
    }


    PLUGIN_FUNCTION_ARG3(GetReadFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL || size == 0)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      strncpy(mediaFormat, lineState[line].readFormat, size-1);
      mediaFormat[size-1] = '\0';
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(GetWriteFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL || size == 0)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      strncpy(mediaFormat, lineState[line].writeFormat, size-1);
      mediaFormat[size-1] = '\0';
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(StopReading, unsigned,line)
    {
      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      vpb_record_terminate(lineState[line].handle);
      vpb_record_buf_finish(lineState[line].handle);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(StopWriting, unsigned,line)
    {
      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      vpb_play_terminate(lineState[line].handle);
      vpb_play_buf_finish(lineState[line].handle);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetReadFrameSize, unsigned,line, unsigned,frameSize)
    {
      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      lineState[line].readFrameSize = frameSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetWriteFrameSize, unsigned,line, unsigned,frameSize)
    {
      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      lineState[line].writeFrameSize = frameSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetReadFrameSize, unsigned,line, unsigned *,frameSize)
    {
      if (frameSize == NULL)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      *frameSize = lineState[line].readFrameSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetWriteFrameSize, unsigned,line, unsigned *,frameSize)
    {
      if (frameSize == NULL)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      *frameSize = lineState[line].writeFrameSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(ReadFrame, unsigned,line, void *,buffer, unsigned *,count)
    {
      if (buffer == NULL || count == NULL)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      *count = lineState[line].readFrameSize;
      vpb_record_buf_sync(lineState[line].handle, (char *)buffer, (unsigned short)*count);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG4(WriteFrame, unsigned,line, const void *,buffer, unsigned,count, unsigned *,written)
    {
      if (buffer == NULL || written == NULL)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      *written = count;
      vpb_play_buf_sync(lineState[line].handle, (char *)buffer, (unsigned short)count);
      return PluginLID_NoError;
    }



    //PLUGIN_FUNCTION_ARG3(GetAverageSignalLevel, unsigned,line, PluginLID_Boolean,playback, unsigned *,signal)
    //PLUGIN_FUNCTION_ARG2(EnableAudio, unsigned,line, PluginLID_Boolean,enable)
    //PLUGIN_FUNCTION_ARG2(IsAudioEnabled, unsigned,line, PluginLID_Boolean *,enable)


    PLUGIN_FUNCTION_ARG2(SetRecordVolume, unsigned,line, unsigned,volume)
    {
      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      return vpb_record_set_gain(lineState[line].handle, (float)(volume/100.0*24.0-12.0)) >= 0 ? PluginLID_NoError : PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG2(SetPlayVolume, unsigned,line, unsigned,volume)
    {
      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      return vpb_play_set_gain(lineState[line].handle, (float)(volume/100.0*24.0-12.0)) >= 0 ? PluginLID_NoError : PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG2(GetRecordVolume, unsigned,line, unsigned *,volume)
    {
      if (volume == NULL)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      float gain;
      if (vpb_record_get_gain(lineState[line].handle, &gain) < 0)
        return PluginLID_InternalError;

      *volume = (unsigned)((gain+12.0)/24.0*100.0);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetPlayVolume, unsigned,line, unsigned *,volume)
    {
      if (volume == NULL)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      float gain;
      if (vpb_play_get_gain(lineState[line].handle, &gain) < 0)
        return PluginLID_InternalError;

      *volume = (unsigned)((gain+12.0)/24.0*100.0);
      return PluginLID_NoError;
    }



    //PLUGIN_FUNCTION_ARG2(GetAEC, unsigned,line, unsigned *,level)
    //PLUGIN_FUNCTION_ARG2(SetAEC, unsigned,line, unsigned,level)
    //PLUGIN_FUNCTION_ARG2(GetVAD, unsigned,line, PluginLID_Boolean *,enable)
    //PLUGIN_FUNCTION_ARG2(SetVAD, unsigned,line, PluginLID_Boolean,enable)
    //PLUGIN_FUNCTION_ARG4(GetCallerID, unsigned,line, char *,idString, unsigned,size, PluginLID_Boolean,full)
    //PLUGIN_FUNCTION_ARG2(SetCallerID, unsigned,line, const char *,idString)
    //PLUGIN_FUNCTION_ARG2(SendVisualMessageWaitingIndicator, unsigned,line, PluginLID_Boolean,on)

    PLUGIN_FUNCTION_ARG4(PlayDTMF, unsigned,line, const char *,digits, unsigned,onTime, unsigned,offTime)
    {
      if (digits == NULL)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      try {
        vpb_dial_sync(lineState[line].handle, (char *)digits);
        vpb_dial_sync(lineState[line].handle, (char*)",");
      }
      catch(VpbException v) {
        std::cerr << "VPB\tPlayDTMF Error code = "  << v.code << ", s = " << v.s << " api func = " << v.api_function << std::endl;
        return PluginLID_InternalError;
      }

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(ReadDTMF, unsigned,line, char *,digit)
    {
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetRemoveDTMF, unsigned,line, PluginLID_Boolean *,removeTones)
    {
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetRemoveDTMF, unsigned,line, PluginLID_Boolean,removeTones)
    {
      return PluginLID_NoError;
    }



    PLUGIN_FUNCTION_ARG2(IsToneDetected, unsigned,line, int *,tone)
    {
      if (tone == NULL)
        return PluginLID_InvalidParameter;

      if (m_uiLineCount == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_uiLineCount)
        return PluginLID_NoSuchLine;

      *tone = PluginLID_NoTone;

      VPB_EVENT event;
      try {
        if (vpb_get_event_ch_async(lineState[line].handle, &event) == VPB_NO_EVENTS)
          return PluginLID_NoError;
      }
      catch (VpbException v) {
        std::cerr << "VPB\tOpalVpbDevice::Open Error code = "  << v.code << ", s = " << v.s << " api func = " << v.api_function << std::endl;
        return PluginLID_InternalError;
      }
      
      if (event.type == VPB_RING) {
        *tone = PluginLID_RingTone;
        return PluginLID_NoError;
      }

      if (event.type != VPB_TONEDETECT)
        return PluginLID_NoError;

      switch (event.data) {
        case VPB_DIAL :
          *tone = PluginLID_DialTone;
          break;
        case VPB_RINGBACK :
          *tone = PluginLID_RingTone;
          break;
        case VPB_BUSY :
          *tone = PluginLID_BusyTone;
          break;
    #ifdef VPB_FASTBUSY
        case VPB_FASTBUSY :
          *tone = PluginLID_FastBusyTone;
          break;
    #endif
        case VPB_GRUNT :
          break;
    #ifdef VPB_MWI
        case VPB_MWI :
          *tone = PluginLID_MwiTone;
          break;
    #endif
        default:
          std::cerr << "VPB\tTone Detect: no a known tone." << event.data<< std::endl;
          return PluginLID_InternalError;
      }
      
      return PluginLID_NoError;
    }


    //PLUGIN_FUNCTION_ARG3(WaitForToneDetect, unsigned,line, unsigned,timeout, int *,tone)
    //PLUGIN_FUNCTION_ARG3(WaitForTone, unsigned,line, int,tone, unsigned,timeout)
    //PLUGIN_FUNCTION_ARG7(SetToneFilterParameters, unsigned        ,line,
    //                                              unsigned        ,tone,
    //                                              unsigned        ,lowFrequency,
    //                                              unsigned        ,highFrequency,
    //                                              unsigned        ,numCadences,
    //                                              const unsigned *,onTimes,
    //                                              const unsigned *,offTimes)
    //PLUGIN_FUNCTION_ARG2(PlayTone, unsigned,line, unsigned,tone)
    //PLUGIN_FUNCTION_ARG2(IsTonePlaying, unsigned,line, PluginLID_Boolean *,playing)
    //PLUGIN_FUNCTION_ARG1(StopTone, unsigned,line)
    //PLUGIN_FUNCTION_ARG4(DialOut, unsigned,line, const char *,number, PluginLID_Boolean,requireTones, unsigned,uiDialDelay)
    //PLUGIN_FUNCTION_ARG2(GetWinkDuration, unsigned,line, unsigned *,winkDuration)
    //PLUGIN_FUNCTION_ARG2(SetWinkDuration, unsigned,line, unsigned,winkDuration)
    //PLUGIN_FUNCTION_ARG1(SetCountryCode, unsigned,country)
    //PLUGIN_FUNCTION_ARG2(GetSupportedCountry, unsigned,index, unsigned *,countryCode)
};



/////////////////////////////////////////////////////////////////////////////

static struct PluginLID_Definition definition[1] =
{

  { 
    // encoder
    PLUGIN_LID_VERSION,               // API version

    1083666706,                       // timestamp = Tue 04 May 2004 10:31:46 AM UTC = 

    "VPB",                            // LID name text
    "Voicetronix VPB-4",              // LID description text
    "Voicetronix",                    // LID manufacturer name
    "VPB",                            // LID model name
    "1.0",                            // LID hardware revision number
    "peter@voicetronix.com.au",       // LID email contact information
    "http://www.voicetronix.com.au",  // LID web site

    "Robert Jongbloed, Post Increment",                          // source code author
    "robertj@postincrement.com",                                 // source code email
    "http://www.postincrement.com",                              // source code URL
    "Copyright (C) 2006 by Post Increment, All Rights Reserved", // source code copyright
    "MPL 1.0",                                                   // source code license
    "1.0",                                                       // source code version

    NULL,                             // user data value

    Context::Create,
    Context::Destroy,
    Context::GetDeviceName,
    Context::Open,
    Context::Close,
    Context::GetLineCount,
    Context::IsLineTerminal,
    Context::IsLinePresent,
    Context::IsLineOffHook,
    Context::SetLineOffHook,
    NULL, //Context::HookFlash,
    NULL, //Context::HasHookFlash,
    Context::IsLineRinging,
    NULL, //Context::RingLine,
    NULL, //Context::IsLineDisconnected,
    NULL, //Context::SetLineToLineDirect,
    NULL, //Context::IsLineToLineDirect,
    Context::GetSupportedFormat,
    Context::SetReadFormat,
    Context::SetWriteFormat,
    Context::GetReadFormat,
    Context::GetWriteFormat,
    Context::StopReading,
    Context::StopWriting,
    Context::SetReadFrameSize,
    Context::SetWriteFrameSize,
    Context::GetReadFrameSize,
    Context::GetWriteFrameSize,
    Context::ReadFrame,
    Context::WriteFrame,
    NULL,//Context::GetAverageSignalLevel,
    NULL,//Context::EnableAudio,
    NULL,//Context::IsAudioEnabled,
    Context::SetRecordVolume,
    Context::SetPlayVolume,
    Context::GetRecordVolume,
    Context::GetPlayVolume,
    NULL,//Context::GetAEC,
    NULL,//Context::SetAEC,
    NULL,//Context::GetVAD,
    NULL,//Context::SetVAD,
    NULL,//Context::GetCallerID,
    NULL,//Context::SetCallerID,
    NULL,//Context::SendVisualMessageWaitingIndicator,
    Context::PlayDTMF,
    Context::ReadDTMF,
    Context::GetRemoveDTMF,
    Context::SetRemoveDTMF,
    Context::IsToneDetected,
    NULL,//Context::WaitForToneDetect,
    NULL,//Context::WaitForTone,
    NULL,//Context::SetToneFilterParameters,
    NULL,//Context::PlayTone,
    NULL,//Context::IsTonePlaying,
    NULL,//Context::StopTone,
    NULL,//Context::DialOut,
    NULL,//Context::GetWinkDuration,
    NULL,//Context::SetWinkDuration,
    NULL,//Context::SetCountryCode,
    NULL,//Context::GetSupportedCountry
  }

};

PLUGIN_LID_IMPLEMENTATION(definition);

/////////////////////////////////////////////////////////////////////////////
