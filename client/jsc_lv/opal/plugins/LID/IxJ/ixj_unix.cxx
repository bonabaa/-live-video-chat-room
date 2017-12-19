/*
 * Quicknet IxJack Plugin LID for OPAL
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
 * $Revision: 20384 $
 * $Author: rjongbloed $
 * $Date: 2008-06-04 10:14:22 +0000 (Wed, 04 Jun 2008) $
 */

#define PLUGIN_DLL_EXPORTS
#include <lids/lidplugin.h>

#include <sys/time.h>
#include <math.h>


#define IsLineInvalid(line) (line < 1)

enum {
    POTSLine,
    PSTNLine
};

/////////////////////////////////////////////////////////////////////////////

class Context
{
  private:
      int hDriver;
#if 0
    class ExceptionInfo {
      public:
        int fd;

        bool hasRing;
        bool hookState;
        bool hasWink;
        bool hasFlash;
        char dtmf[16];
        int dtmfIn;
        int dtmfOut;
#ifdef IXJCTL_VMWI
        bool hasCid;
        PHONE_CID cid;
#endif
        bool filter[4];
        bool cadence[4];
        telephony_exception data;
        timeval lastHookChange;
    };

    static void SignalHandler(int sig);
    ExceptionInfo * GetException();
    int GetOSHandle() { return os_handle; }

    bool ConvertOSError(int err);

    static ExceptionInfo exceptionInfo[MaxIxjDevices];
    static PMutex        exceptionMutex;
    static bool          exceptionInit;

    AECLevels aecLevel;
    bool removeDTMF;
    PMutex toneMutex;
    bool tonePlaying;
    PTimer lastRingTime;
    bool pstnIsOffHook;
    bool gotWink;
    int  userPlayVol, userRecVol;

    int  savedPlayVol, savedRecVol;
    AECLevels savedAEC;

#ifdef IXJCTL_VMWI
    PHONE_CID callerIdInfo;
#endif

#endif

  public:
    PLUGIN_LID_CTOR()
    {
    }

    PLUGIN_LID_DTOR()
    {
      Close();
    }


    PLUGIN_FUNCTION_ARG3(GetDeviceName,unsigned,index, char *,name, unsigned,size)
    {
      if (name == NULL || size < 2)
        return PluginLID_InvalidParameter;

     // if (index >= (unsigned)deviceNames.GetSize())
     //   return PluginLID_NoMoreNames;

     // strncpy(name, deviceNames[index], size-1);
      name[size-1] = '\0';
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(Open,const char *,device)
    {
      Close();

      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG0(Close)
    {
      return PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG1(GetLineCount, unsigned *,count)
    {
      if (count == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

   //   *count = IsLineJACK() ? NumLines : 1;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineTerminal, unsigned,line, PluginLID_Boolean *,isTerminal)
    {
      if (isTerminal == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

    //  *isTerminal = line == POTSLine;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(IsLinePresent, unsigned,line, PluginLID_Boolean,forceTest, PluginLID_Boolean *,present)
    {
      if (present == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

     // *present = dwResult == LINE_TEST_OK;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineOffHook, unsigned,line, PluginLID_Boolean *,offHook)
    {
      if (offHook == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

     // *offHook = currentHookState;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetLineOffHook, unsigned,line, PluginLID_Boolean,newState)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (line != PSTNLine)
        return PluginLID_OperationNotAllowed;

      return PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG2(HookFlash, unsigned,line, unsigned,flashTime)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (line != PSTNLine)
        return PluginLID_OperationNotAllowed;

      return PluginLID_InternalError;
    }


    //PLUGIN_FUNCTION_ARG2(HasHookFlash, unsigned,line, PluginLID_Boolean *,flashed)


    PLUGIN_FUNCTION_ARG2(IsLineRinging, unsigned,line, unsigned long *,cadence)
    {
      if (cadence == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (line != POTSLine)
        return PluginLID_OperationNotAllowed;

      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG4(RingLine, unsigned,line, unsigned,nCadence, const unsigned *,pattern, unsigned,frequency)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (line != PSTNLine)
        return PluginLID_OperationNotAllowed;

      return PluginLID_InternalError;
    }

    PLUGIN_FUNCTION_ARG3(IsLineDisconnected, unsigned,line, PluginLID_Boolean,checkForWink, PluginLID_Boolean *,disconnected)
    {
      if (disconnected == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (line != PSTNLine)
        return PluginLID_OperationNotAllowed;

     // *disconnected = (tone & PluginLID_BusyTone) != 0;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(SetLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean,connect)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line1) || IsLineInvalid(line2))
        return PluginLID_NoSuchLine;

      if (line1 == line2)
        return PluginLID_OperationNotAllowed;

      return PluginLID_InternalError;
    }
    
    PLUGIN_FUNCTION_ARG3(IsLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean *,connected)
    {
      if (connected == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line1) || IsLineInvalid(line2))
        return PluginLID_NoSuchLine;

      if (line1 == line2)
        return PluginLID_OperationNotAllowed;

     // *connected = dwResult == 0;
      return PluginLID_NoError;
    }



    PLUGIN_FUNCTION_ARG3(GetSupportedFormat, unsigned,index, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL || size < 2)
        return PluginLID_InvalidParameter;

     // if (index >= sizeof(CodecInfo)/sizeof(CodecInfo))
     //   return PluginLID_NoMoreNames;

     // strncpy(mediaFormat, CodecInfo[index].mediaFormat, size-1);
      mediaFormat[size-1] = '\0';
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetReadFormat, unsigned,line, const char *,mediaFormat)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      StopReading(line);

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetWriteFormat, unsigned,line, const char *,mediaFormat)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      StopWriting(line);

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(GetReadFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL || size == 0)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(GetWriteFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL || size == 0)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(StopReading, unsigned,line)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(StopWriting, unsigned,line)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetReadFrameSize, unsigned,line, unsigned,frameSize)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

     // readFrameSize = frameSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetWriteFrameSize, unsigned,line, unsigned,frameSize)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

     // writeFrameSize = frameSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetReadFrameSize, unsigned,line, unsigned *,frameSize)
    {
      if (frameSize == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

     // *frameSize = readFrameSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetWriteFrameSize, unsigned,line, unsigned *,frameSize)
    {
      if (frameSize == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

     // *frameSize = writeFrameSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(ReadFrame, unsigned,line, void *,buffer, unsigned *,count)
    {
      if (buffer == NULL || count == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG4(WriteFrame, unsigned,line, const void *,buffer, unsigned,count, unsigned *,written)
    {
      if (buffer == NULL || written == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_InternalError;
    }



    PLUGIN_FUNCTION_ARG3(GetAverageSignalLevel, unsigned,line, PluginLID_Boolean,playback, unsigned *,signal)
    {
      if (signal == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG2(EnableAudio, unsigned,line, PluginLID_Boolean,enable)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG2(IsAudioEnabled, unsigned,line, PluginLID_Boolean *,enable)
    {
      if (enable == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

     // *enable = enabledAudioLine == line;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetRecordVolume, unsigned,line, unsigned,volume)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG2(SetPlayVolume, unsigned,line, unsigned,volume)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG2(GetRecordVolume, unsigned,line, unsigned *,volume)
    {
      if (volume == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetPlayVolume, unsigned,line, unsigned *,volume)
    {
      if (volume == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetAEC, unsigned,line, unsigned *,level)
    {
      if (level == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG2(SetAEC, unsigned,line, unsigned,level)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_InternalError;
    }

    PLUGIN_FUNCTION_ARG2(GetVAD, unsigned,line, PluginLID_Boolean *,enable)
    {
      if (enable == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

     // *enable = vadEnabled;
      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG2(SetVAD, unsigned,line, PluginLID_Boolean,enable)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

     // vadEnabled = enable;
      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG4(GetCallerID, unsigned,line, char *,idString, unsigned,size, PluginLID_Boolean,full)
    {
      if (idString == NULL || size == 0)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG2(SendVisualMessageWaitingIndicator, unsigned,line, PluginLID_Boolean,on)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (line != POTSLine)
        return PluginLID_OperationNotAllowed;

      return PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG4(PlayDTMF, unsigned,line, const char *,digits, unsigned,onTime, unsigned,offTime)
    {
      if (digits == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(ReadDTMF, unsigned,line, char *,digit)
    {
      if (digit == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

     // *digit = dtmf[dwNewDigit&0xf];
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetRemoveDTMF, unsigned,line, PluginLID_Boolean *,removeTones)
    {
      if (removeTones == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

     // *removeTones = result != 0;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetRemoveDTMF, unsigned,line, PluginLID_Boolean,removeTones)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_InternalError;
    }



    PLUGIN_FUNCTION_ARG2(IsToneDetected, unsigned,line, int *,tone)
    {
      if (tone == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (EnableAudio(line, TRUE) != PluginLID_NoError)
        return PluginLID_InternalError;

      return PluginLID_NoError;
    }


    //PLUGIN_FUNCTION_ARG3(WaitForToneDetect, unsigned,line, unsigned,timeout, int *,tone)
    //PLUGIN_FUNCTION_ARG3(WaitForTone, unsigned,line, int,tone, unsigned,timeout)

    PLUGIN_FUNCTION_ARG8(SetToneParameters, unsigned        ,line,
                                            unsigned        ,tone,
                                            unsigned        ,frequency1,
                                            unsigned        ,frequency2,
                                            unsigned        ,mode,
                                            unsigned        ,numCadences,
                                            const unsigned *,onTimes,
                                            const unsigned *,offTimes)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_InternalError;
    }

    PLUGIN_FUNCTION_ARG2(PlayTone, unsigned,line, unsigned,tone)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG2(IsTonePlaying, unsigned,line, PluginLID_Boolean *,playing)
    {
      if (playing == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

     // *playing = PTimer::Tick() < toneSendCompletionTime;
      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG1(StopTone, unsigned,line)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_InternalError;
    }

    //PLUGIN_FUNCTION_ARG4(DialOut, unsigned,line, const char *,number, PluginLID_Boolean,requireTones, unsigned,uiDialDelay)

    PLUGIN_FUNCTION_ARG2(GetWinkDuration, unsigned,line, unsigned *,winkDuration)
    {
      if (winkDuration == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG2(SetWinkDuration, unsigned,line, unsigned,winkDuration)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return PluginLID_InternalError;
    }

    PLUGIN_FUNCTION_ARG1(SetCountryCode, unsigned,country)
    {
      if (hDriver == -1)
        return PluginLID_DeviceNotOpen;

      // if a LineJack, the set the DAA coeffiecients
     // if (!IsLineJACK())
    //    return PluginLID_OperationNotAllowed;

      return PluginLID_InternalError;
    }

    PLUGIN_FUNCTION_ARG2(GetSupportedCountry, unsigned,index, unsigned *,countryCode)
    {
      if (countryCode == NULL)
        return PluginLID_InvalidParameter;

     // if (index >= PARRAYSIZE(CountryInfo))
     //   return PluginLID_NoMoreNames;

      //*countryCode = CountryInfo[index].t35Code;
      return PluginLID_NoError;
    }


  protected:
};


/////////////////////////////////////////////////////////////////////////////

static struct PluginLID_Definition definition[1] =
{

  { 
    // encoder
    PLUGIN_LID_VERSION,               // API version

    1083666706,                       // timestamp = Tue 04 May 2004 10:31:46 AM UTC = 

    "IxJ",                            // LID name text
    "Quicknet IxJack",                // LID description text
    "Quicknet Technologies",          // LID manufacturer name
    "IxJack",                         // LID model name
    "1.0",                            // LID hardware revision number
    "support@quicknet.net",           // LID email contact information
    "http://www.quicknet.net",        // LID web site

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
    Context::HookFlash,
    NULL,//Context::HasHookFlash,
    Context::IsLineRinging,
    Context::RingLine,
    Context::IsLineDisconnected,
    Context::SetLineToLineDirect,
    Context::IsLineToLineDirect,
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
    Context::GetAverageSignalLevel,
    Context::EnableAudio,
    Context::IsAudioEnabled,
    Context::SetRecordVolume,
    Context::SetPlayVolume,
    Context::GetRecordVolume,
    Context::GetPlayVolume,
    Context::GetAEC,
    Context::SetAEC,
    Context::GetVAD,
    Context::SetVAD,
    Context::GetCallerID,
    NULL,//Context::SetCallerID,
    Context::SendVisualMessageWaitingIndicator,
    Context::PlayDTMF,
    Context::ReadDTMF,
    Context::GetRemoveDTMF,
    Context::SetRemoveDTMF,
    Context::IsToneDetected,
    NULL,//Context::WaitForToneDetect,
    NULL,//Context::WaitForTone,
    Context::SetToneParameters,
    Context::PlayTone,
    Context::IsTonePlaying,
    Context::StopTone,
    NULL,//Context::DialOut,
    Context::GetWinkDuration,
    Context::SetWinkDuration,
    Context::SetCountryCode,
    Context::GetSupportedCountry
  }

};

PLUGIN_LID_IMPLEMENTATION(definition);


/////////////////////////////////////////////////////////////////////////////
