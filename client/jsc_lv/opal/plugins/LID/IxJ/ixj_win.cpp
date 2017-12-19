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
 * $Log: ixj_win.cpp,v $
 * Revision 1.5  2007/06/29 02:49:42  rjongbloed
 * Added PString::FindSpan() function (strspn equivalent) with slightly nicer semantics.
 *
 * Revision 1.4  2006/11/05 05:04:46  rjongbloed
 * Improved the terminal LID line ringing, epecially for country emulation.
 *
 * Revision 1.3  2006/10/26 08:36:49  rjongbloed
 * Fixed DevStudio 2005 warning
 *
 * Revision 1.2  2006/10/25 22:26:15  rjongbloed
 * Changed LID tone handling to use new tone generation for accurate country based tones.
 *
 * Revision 1.1  2006/10/02 13:30:53  rjongbloed
 * Added LID plug ins
 *
 */

#include <ptlib.h>
#include <ptlib/pprocess.h>
#include <ptlib/sound.h>

#define PLUGIN_DLL_EXPORTS
#include <lids/lidplugin.h>
#include <lids/lid.h>

#include "QTIoctl.h"
#include "ixjDefs.h"


#define NEW_DRIVER_VERSION ((5<<24)|(5<<16)|141)

static enum {
  IsWindows9x,
  IsWindowsNT,
  IsWindows2k
} GetOperatingSystem()
{
  static OSVERSIONINFO version;
  if (version.dwOSVersionInfoSize == 0) {
    version.dwOSVersionInfoSize = sizeof(version);
    GetVersionEx(&version);
  }
  if (version.dwPlatformId != VER_PLATFORM_WIN32_NT)
    return IsWindows9x;
  if (version.dwMajorVersion < 5)
    return IsWindowsNT;
  return IsWindows2k;
}

enum { MaxIxjDevices = 10 };

enum {
  POTSLine,
  PSTNLine,
  NumLines
};

#define IsLineJACK() (dwCardType == 3)
#define IsLineInvalid(line) ((line) >= (unsigned)(IsLineJACK() ? 2 : 1))


static const struct {
  const char * mediaFormat;
  unsigned dspBitMask:3; // bit0=8020,bit1=8021,bit2=8022
  unsigned isG729:1;
  unsigned isG7231:1;
  unsigned vad:1;
  PINDEX frameSize;
  DWORD recordMode;
  DWORD recordRate;
  DWORD playbackMode;
  DWORD playbackRate;
} CodecInfo[] = {
  { OPAL_PCM16,         7, 0, 0, 0,   0, RECORD_MODE_16LINEAR,   0,                   PLAYBACK_MODE_16LINEAR,   0                     },
  { OPAL_G711_ULAW_64K, 7, 0, 0, 0,   0, RECORD_MODE_ULAW,       0,                   PLAYBACK_MODE_ULAW,       0                     },
  { OPAL_G711_ALAW_64K, 7, 0, 0, 0,   0, RECORD_MODE_ALAW,       0,                   PLAYBACK_MODE_ALAW,       0                     },
  { OPAL_G728,          2, 0, 0, 0,  20, RECORD_MODE_TRUESPEECH, RECORD_RATE_G728,    PLAYBACK_MODE_TRUESPEECH, PLAYBACK_RATE_G728    },
  { OPAL_G729,          6, 1, 0, 0,  10, RECORD_MODE_TRUESPEECH, RECORD_RATE_G729,    PLAYBACK_MODE_TRUESPEECH, PLAYBACK_RATE_G729    },
  { OPAL_G729AB,        6, 1, 0, 1,  10, RECORD_MODE_TRUESPEECH, RECORD_RATE_G729,    PLAYBACK_MODE_TRUESPEECH, PLAYBACK_RATE_G729    },

  // these two lines should be for the 5k3 codec, but this does not work properly in the driver so we lie
  { OPAL_G7231_5k3,     7, 0, 1, 0,  24, RECORD_MODE_TRUESPEECH, RECORD_RATE_G723_63, PLAYBACK_MODE_TRUESPEECH, PLAYBACK_RATE_G723_63 },
  { OPAL_G7231A_5k3,    7, 0, 1, 1,  24, RECORD_MODE_TRUESPEECH, RECORD_RATE_G723_63, PLAYBACK_MODE_TRUESPEECH, PLAYBACK_RATE_G723_63 },

  { OPAL_G7231_6k3,     7, 0, 1, 0,  24, RECORD_MODE_TRUESPEECH, RECORD_RATE_G723_63, PLAYBACK_MODE_TRUESPEECH, PLAYBACK_RATE_G723_63 },
  { OPAL_G7231A_6k3,    7, 0, 1, 1,  24, RECORD_MODE_TRUESPEECH, RECORD_RATE_G723_63, PLAYBACK_MODE_TRUESPEECH, PLAYBACK_RATE_G723_63 },

};

static PINDEX FindCodec(const char * mediaFormat)
{
  for (PINDEX codecType = 0; codecType < PARRAYSIZE(CodecInfo); codecType++) {
    if (strcmp(mediaFormat, CodecInfo[codecType].mediaFormat) == 0)
      return codecType;
  }

  return P_MAX_INDEX;
}


static struct {
  OpalLineInterfaceDevice::T35CountryCodes t35Code;
  DWORD                                    ixjCode;
} CountryInfo[] = {
  { OpalLineInterfaceDevice::UnitedStates,  COEFF_US },
  { OpalLineInterfaceDevice::Australia,     COEFF_AUSTRALIA },
  { OpalLineInterfaceDevice::Czechoslovakia,COEFF_CZECH },
  { OpalLineInterfaceDevice::France,        COEFF_FRANCE },
  { OpalLineInterfaceDevice::Germany,       COEFF_GERMANY },
  { OpalLineInterfaceDevice::Italy,         COEFF_ITALY },
  { OpalLineInterfaceDevice::Japan,         COEFF_JAPAN },
  { OpalLineInterfaceDevice::KoreaRepublic, COEFF_SOUTH_KOREA },
  { OpalLineInterfaceDevice::NewZealand,    COEFF_NEW_ZEALAND },
  { OpalLineInterfaceDevice::Norway,        COEFF_NORWAY },
  { OpalLineInterfaceDevice::Philippines,   COEFF_PHILIPPINES },
  { OpalLineInterfaceDevice::Poland,        COEFF_POLAND },
  { OpalLineInterfaceDevice::SouthAfrica,   COEFF_SOUTH_AFRICA },
  { OpalLineInterfaceDevice::Sweden,        COEFF_SWEDEN },
  { OpalLineInterfaceDevice::UnitedKingdom, COEFF_UK }
};



/////////////////////////////////////////////////////////////////////////////

class Context
{
  private:
    PStringArray  deviceNames;
    PString       deviceName;
    DWORD         dwCardType;
    HANDLE        hDriver;
    PMutex        readMutex, writeMutex;
    bool          readStopped, writeStopped;
    PINDEX        readFrameSize, writeFrameSize;
    PINDEX        readCodecType, writeCodecType;
    bool          lastHookState, currentHookState;
    PTimer        hookTimeout;
    bool          inRawMode;
    unsigned      enabledAudioLine;
    bool          exclusiveAudioMode;
    DWORD         countryCode;
    DWORD         driverVersion;
    PTimer        ringTimeout;
    DWORD         lastDTMFDigit;
    DWORD         lastFlashState;
    PTimeInterval toneSendCompletionTime;
    bool          vadEnabled;
    HANDLE        hReadEvent, hWriteEvent;

  public:
    PLUGIN_LID_CTOR()
    {
      hDriver = INVALID_HANDLE_VALUE;
      driverVersion = 0;
      readStopped = writeStopped = true;
      readFrameSize = writeFrameSize = 480;  // 30 milliseconds of 16 bit PCM data
      readCodecType = writeCodecType = P_MAX_INDEX;
      currentHookState = lastHookState = false;
      inRawMode = false;
      enabledAudioLine = UINT_MAX;
      exclusiveAudioMode = true;
      lastDTMFDigit = 0;
      hReadEvent = hWriteEvent = NULL;
    }

    PLUGIN_LID_DTOR()
    {
      Close();
    }


    PLUGIN_FUNCTION_ARG3(GetDeviceName,unsigned,index, char *,name, unsigned,size)
    {
      if (name == NULL || size < 2)
        return PluginLID_InvalidParameter;

      if (index == 0)
        GetDeviceNames();

      if (index >= (unsigned)deviceNames.GetSize())
        return PluginLID_NoMoreNames;

      strncpy(name, deviceNames[index], size-1);
      name[size-1] = '\0';
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(Open,const char *,device)
    {
      Close();

      PTRACE(3, "xJack\tOpening IxJ device \"" << device << '"');

      DWORD dwDeviceId = strtoul(device, NULL, 16);

      PString devicePath;
      const char * DevicePathPrefix = "\\\\.\\QTJACKDevice";

      switch (GetOperatingSystem()) {
        case IsWindows2k :
          DevicePathPrefix = "\\\\.\\QTIWDMDevice";
          // Flow into NT case

        case IsWindowsNT :
          if (dwDeviceId < 100) {
            devicePath = device;
            devicePath.Delete(devicePath.Find(' '), 1000);
            if (devicePath.FindSpan("0123456789") == P_MAX_INDEX)
              devicePath = DevicePathPrefix + devicePath;
          }
          else {
            GetDeviceNames();
            for (PINDEX dev = 0; dev < deviceNames.GetSize(); dev++) {
              PString thisDevice = deviceNames[dev];
              if (thisDevice.Find(device) != P_MAX_INDEX) {
                devicePath = thisDevice.Left(thisDevice.Find(' '));
                break;
              }
            }
          }
          timeBeginPeriod(2);
          break;

        case IsWindows9x :
          devicePath = "\\\\.\\Qtipj.vxd";
      }

      hDriver = CreateFile(devicePath,
                          GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                          NULL);
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceOpenFailed;

      if (GetOperatingSystem() == IsWindows9x) {
        DWORD dwResult = 0;
        if (!IoControl(IOCTL_Device_Open, dwDeviceId, &dwResult) || dwResult == 0) {
          CloseHandle(hDriver);
          hDriver = INVALID_HANDLE_VALUE;
          return PluginLID_NoSuchDevice;
        }
        deviceName = psprintf("%08X", dwDeviceId);
      }
      else {
        dwDeviceId = GetSerialNumber();
        if (dwDeviceId == 0) {
          CloseHandle(hDriver);
          hDriver = INVALID_HANDLE_VALUE;
          return PluginLID_NoSuchDevice;
        }

        deviceName = devicePath;
      }

      dwCardType = dwDeviceId >> 28;

      IoControl(IOCTL_Codec_SetKHz, 8000);
      IoControl(IOCTL_Idle_SetMasterGain, 15);
      IoControl(IOCTL_Filter_EnableDTMFDetect);
      IoControl(IOCTL_Speakerphone_AECOn);

      DWORD ver = 0;
      IoControl(IOCTL_VxD_GetVersion, 0, &ver);
      driverVersion = ((ver&0xff)<<24)|((ver&0xff00)<<8)|((ver>>16)&0xffff);

      PTRACE(2, "xJack\tOpened IxJ device \"" << deviceName << "\" version "
            << ((driverVersion>>24)&0xff  ) << '.'
            << ((driverVersion>>16)&0xff  ) << '.'
            << ( driverVersion     &0xffff));

      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG0(Close)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_NoError;

      RingLine(0, 0, NULL, 0);
      StopReading(0);
      StopWriting(0);
      SetLineToLineDirect(0, 1, true); // Back to pass through mode

      if (GetOperatingSystem() == IsWindows9x)
        IoControl(IOCTL_Device_Close);
      else
        timeEndPeriod(2);

      deviceName = PString();

      if (hReadEvent != NULL) {
        CloseHandle(hReadEvent);
        hReadEvent = NULL;
      }
      if (hWriteEvent != NULL) {
        CloseHandle(hWriteEvent);
        hWriteEvent = NULL;
      }

      BOOL ok = CloseHandle(hDriver);
      hDriver = INVALID_HANDLE_VALUE;
      return ok ? PluginLID_NoError : PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG1(GetLineCount, unsigned *,count)
    {
      if (count == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      *count = IsLineJACK() ? NumLines : 1;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineTerminal, unsigned,line, PluginLID_Boolean *,isTerminal)
    {
      if (isTerminal == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      *isTerminal = line == POTSLine;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(IsLinePresent, unsigned,line, PluginLID_Boolean,forceTest, PluginLID_Boolean *,present)
    {
      if (present == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (line != PSTNLine) {
        *present = false;
        return PluginLID_NoError;
      }

      int oldSlicState = -1;

      DWORD dwResult = 0;
      do {
        if (!IoControl(IOCTL_DevCtrl_GetLineTestResult, 0, &dwResult))
          return PluginLID_InternalError;
        if (dwResult == 0xffffffff || forceTest) {
          if (dwResult == LINE_TEST_OK) {
            IoControl(IOCTL_DevCtrl_GetPotsToSlic, 0, &dwResult);
            oldSlicState = dwResult;
          }
          IoControl(IOCTL_DevCtrl_LineTest);
          dwResult = LINE_TEST_TESTING;
          forceTest = false;
        }
      } while (dwResult == LINE_TEST_TESTING);

      if (oldSlicState >= 0)
        IoControl(IOCTL_DevCtrl_SetPotsToSlic, oldSlicState);

      *present = dwResult == LINE_TEST_OK;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineOffHook, unsigned,line, PluginLID_Boolean *,offHook)
    {
      if (offHook == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      DWORD dwResult = 0;

      if (line == PSTNLine) {
        if (!IoControl(IOCTL_DevCtrl_GetLineOnHook, 0, &dwResult))
          return PluginLID_InternalError;
        *offHook = dwResult == 0;
        return PluginLID_NoError;
      }

      PluginLID_Boolean present;
      if (IsLinePresent(PSTNLine, false, &present) != PluginLID_NoError)
        return PluginLID_InternalError;

      if (!IoControl(IsLineJACK() && present ? IOCTL_DevCtrl_GetLinePhoneOnHook : IOCTL_DevCtrl_GetOnHook, 0, &dwResult))
        return PluginLID_InternalError;

      bool newHookState = dwResult == 0;
      if (lastHookState != newHookState) {
        lastHookState = newHookState;
        hookTimeout = 250;
      }
      else {
        if (!hookTimeout.IsRunning())
          currentHookState = lastHookState;
      }

      *offHook = currentHookState;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetLineOffHook, unsigned,line, PluginLID_Boolean,newState)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (line != PSTNLine)
        return PluginLID_OperationNotAllowed;

      return IoControl(IOCTL_DevCtrl_LineSetOnHook, !newState) ? PluginLID_NoError : PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG2(HookFlash, unsigned,line, unsigned,flashTime)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (line != PSTNLine)
        return PluginLID_OperationNotAllowed;

      DWORD dwResult;
      if (!IoControl(IOCTL_DevCtrl_GetFlashState, 0, &dwResult))
        return PluginLID_InternalError;

      if (lastFlashState == dwResult)
        return PluginLID_InternalError;

      lastFlashState = dwResult;
      return dwResult != 0 ? PluginLID_NoError : PluginLID_InternalError;
    }


    //PLUGIN_FUNCTION_ARG2(HasHookFlash, unsigned,line, PluginLID_Boolean *,flashed)


    PLUGIN_FUNCTION_ARG2(IsLineRinging, unsigned,line, unsigned long *,cadence)
    {
      if (cadence == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (line != POTSLine)
        return PluginLID_OperationNotAllowed;

      if (ringTimeout.IsRunning()) {
        *cadence = 1;
        return PluginLID_NoError;
      }

      DWORD dwResult = 0;
      if (!IoControl(IOCTL_DevCtrl_LineGetRinging, 0, &dwResult))
        return PluginLID_InternalError;

      *cadence = dwResult != 0 ? 1 : 0;

      ringTimeout = 2500;
      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG4(RingLine, unsigned,line, unsigned,nCadence, const unsigned *,pattern, unsigned,frequency)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (line != PSTNLine)
        return PluginLID_OperationNotAllowed;

      if (nCadence == 0 || pattern == NULL) {
        switch (countryCode) {
          case COEFF_AUSTRALIA :
            static unsigned AusRing[] = { 200, 400, 200, 2000 };
            nCadence = PARRAYSIZE(AusRing);
            pattern = AusRing;
        }
      }

      IoControl(IOCTL_DevCtrl_SetPotsToSlic, 1);


      DWORD dwReturn, dwSize;
      DWORD cadenceArray[10];
      cadenceArray[0] = (nCadence+1)/2; // Number of pairs
      cadenceArray[1] = (nCadence&1) == 0; // If odd then repeat last entry
      unsigned i;
      for (i = 2; i < nCadence; i++)
        cadenceArray[i] = pattern[i-2];
      for (; i < PARRAYSIZE(cadenceArray); i++)
        cadenceArray[i] = 0;

      return IoControl(IOCTL_DevCtrl_SetRingCadence,
                      cadenceArray, sizeof(cadenceArray),
                      &dwReturn, sizeof(dwReturn), &dwSize) ? PluginLID_NoError : PluginLID_InternalError;
    }

    PLUGIN_FUNCTION_ARG3(IsLineDisconnected, unsigned,line, PluginLID_Boolean,checkForWink, PluginLID_Boolean *,disconnected)
    {
      if (disconnected == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (line != PSTNLine)
        return PluginLID_OperationNotAllowed;

      if (checkForWink) {
        DWORD dwResult = 0;
        if (IoControl(IOCTL_DevCtrl_GetLineCallerOnHook, 0, &dwResult) && dwResult != 0) {
          PTRACE(3, "xJack\tDetected wink, line disconnected.");
          *disconnected = true;
          return PluginLID_NoError;
        }
      }

      int tone;
      if (IsToneDetected(line, &tone) != PluginLID_NoError)
        return PluginLID_InternalError;

      *disconnected = (tone & PluginLID_BusyTone) != 0;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(SetLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean,connect)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line1) || IsLineInvalid(line2))
        return PluginLID_NoSuchLine;

      if (line1 == line2)
        return PluginLID_OperationNotAllowed;

      DWORD dwResult = 0;
      return IoControl(IOCTL_DevCtrl_SetPotsToSlic, connect ? 0 : 1, &dwResult) && dwResult != 0 ? PluginLID_NoError : PluginLID_InternalError;
    }
    
    PLUGIN_FUNCTION_ARG3(IsLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean *,connected)
    {
      if (connected == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line1) || IsLineInvalid(line2))
        return PluginLID_NoSuchLine;

      if (line1 == line2)
        return PluginLID_OperationNotAllowed;

      // The IOCTL_DevCtrl_GetPotsToSlic is broken unless the line test has been
      // performed and there is a PSTN line present.
      PluginLID_Boolean present;
      if (IsLinePresent(PSTNLine, false, &present) != PluginLID_NoError)
        return PluginLID_InternalError;

      if (!present)
        return PluginLID_OperationNotAllowed;

      DWORD dwResult = 1;
      if (!IoControl(IOCTL_DevCtrl_GetPotsToSlic, 0, &dwResult))
        return PluginLID_InternalError;

      *connected = dwResult == 0;
      return PluginLID_NoError;
    }



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
        return SetRawCodec(line) ? PluginLID_NoError : PluginLID_InternalError;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      StopReading(line);

      readCodecType = FindCodec(mediaFormat);
      if (readCodecType == P_MAX_INDEX) {
        PTRACE(1, "xJack\tUnsupported read codec requested: " << mediaFormat);
        return PluginLID_UnsupportedMediaFormat;
      }

      if (!writeStopped && readCodecType != writeCodecType) {
        PTRACE(1, "xJack\tAsymmetric codecs requested: "
                  "read=" << CodecInfo[readCodecType].mediaFormat
              << " write=" << CodecInfo[writeCodecType].mediaFormat);
        return PluginLID_UnsupportedMediaFormat;
      }

      PTRACE(3, "xJack\tSetReadFormat(" << CodecInfo[readCodecType].mediaFormat << ')');

      if (!IoControl(IOCTL_Codec_SetKHz, 8000))
        return PluginLID_InternalError;

      if (!IoControl(IOCTL_Record_SetBufferChannelLimit, 1))
        return PluginLID_InternalError;

      DWORD mode;
      do {
        if (!IoControl(IOCTL_Record_SetRECMODE, CodecInfo[readCodecType].recordMode))
          return PluginLID_InternalError;
        if (!IoControl(IOCTL_Record_GetRECMODE, 0, &mode))
          return PluginLID_InternalError;
        PTRACE_IF(3, mode != CodecInfo[readCodecType].recordMode,
                  "xJack\tSetRECMODE failed (" << mode << " -> " <<
                  CodecInfo[readCodecType].recordMode << "), retrying");
      } while (mode != CodecInfo[readCodecType].recordMode);

      DWORD rate;
      do {
        if (!IoControl(IOCTL_Record_SetRate, CodecInfo[readCodecType].recordRate))
          return PluginLID_InternalError;
        if (!IoControl(IOCTL_Record_GetRate, 0, &rate))
          return PluginLID_InternalError;
        PTRACE_IF(3, rate != CodecInfo[readCodecType].recordRate,
                  "xJack\tRecord_SetRate failed (" << rate << " -> " <<
                  CodecInfo[readCodecType].recordRate << "), retrying");
      } while (rate != CodecInfo[readCodecType].recordRate);

      readFrameSize = CodecInfo[readCodecType].frameSize;
      if (readFrameSize == 0) {
        DWORD frameWords;
        if (IoControl(IOCTL_Record_GetFrameSize, 0, &frameWords))
          readFrameSize = frameWords*2;
        else {
          PTRACE(1, "xJack\tCould not get record frame size.");
          return PluginLID_InternalError;
        }
      }

      SetVAD(line, CodecInfo[readCodecType].vad);

      if (!IoControl(driverVersion >= NEW_DRIVER_VERSION ? IOCTL_Record_Start
                                                        : IOCTL_Record_Start_Old))
        return PluginLID_InternalError;

      readStopped = false;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetWriteFormat, unsigned,line, const char *,mediaFormat)
    {
      if (mediaFormat == NULL)
        return SetRawCodec(line) ? PluginLID_NoError : PluginLID_InternalError;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      StopWriting(line);

      PWaitAndSignal mutex(writeMutex);

      IoControl(IOCTL_Playback_Stop);

      writeCodecType = FindCodec(mediaFormat);
      if (writeCodecType == P_MAX_INDEX) {
        PTRACE(1, "xJack\tUnsupported write codec requested: " << mediaFormat);
        return PluginLID_UnsupportedMediaFormat;
      }

      if (!readStopped && writeCodecType != readCodecType) {
        PTRACE(1, "xJack\tAsymmetric codecs requested: "
                  "read=" << CodecInfo[readCodecType].mediaFormat
              << " write=" << CodecInfo[writeCodecType].mediaFormat);
        return PluginLID_UnsupportedMediaFormat;
      }

      PTRACE(3, "xJack\tSetWriteFormat(" << CodecInfo[writeCodecType].mediaFormat << ')');

      if (!IoControl(IOCTL_Codec_SetKHz, 8000))
        return PluginLID_InternalError;

      if (!IoControl(IOCTL_Playback_SetBufferChannelLimit, 1))
        return PluginLID_InternalError;

      DWORD mode;
      do {
        if (!IoControl(IOCTL_Playback_SetPLAYMODE, CodecInfo[writeCodecType].playbackMode))
          return PluginLID_InternalError;
        if (!IoControl(IOCTL_Playback_GetPLAYMODE, 0, &mode))
          return PluginLID_InternalError;
        PTRACE_IF(2, mode != CodecInfo[writeCodecType].playbackMode,
                  "xJack\tSetPLAYMODE failed (" << mode << " -> " <<
                  CodecInfo[writeCodecType].playbackMode << "), retrying");
      } while (mode != CodecInfo[writeCodecType].playbackMode);

      DWORD rate;
      do {
        if (!IoControl(IOCTL_Playback_SetRate, CodecInfo[writeCodecType].playbackRate))
          return PluginLID_InternalError;
        if (!IoControl(IOCTL_Playback_GetRate, 0, &rate))
          return PluginLID_InternalError;
        PTRACE_IF(2, rate != CodecInfo[writeCodecType].playbackRate,
                  "xJack\tPlayback_SetRate failed (" << rate << " -> " <<
                  CodecInfo[writeCodecType].playbackRate << "), retrying");
      } while (rate != CodecInfo[writeCodecType].playbackRate);

      writeFrameSize = CodecInfo[writeCodecType].frameSize;
      if (writeFrameSize == 0) {
        DWORD frameWords;
        if (IoControl(IOCTL_Playback_GetFrameSize, 0, &frameWords))
          writeFrameSize = frameWords*2;
        else {
          PTRACE(1, "xJack\tCould not get playback frame size.");
          return PluginLID_InternalError;
        }
      }

      SetVAD(line, CodecInfo[writeCodecType].vad);

      if (!IoControl(driverVersion >= NEW_DRIVER_VERSION ? IOCTL_Playback_Start
                                                        : IOCTL_Playback_Start_Old))
        return PluginLID_InternalError;

      writeStopped = false;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(GetReadFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL || size == 0)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (readCodecType == P_MAX_INDEX)
        mediaFormat[0] = '\0';
      else {
        strncpy(mediaFormat, CodecInfo[readCodecType].mediaFormat, size-1);
        mediaFormat[size-1] = '\0';
      }
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(GetWriteFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL || size == 0)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (writeCodecType == P_MAX_INDEX)
        mediaFormat[0] = '\0';
      else {
        strncpy(mediaFormat, CodecInfo[writeCodecType].mediaFormat, size-1);
        mediaFormat[size-1] = '\0';
      }
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(StopReading, unsigned,line)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (inRawMode) {
        StopRawCodec(line);
        return PluginLID_NoError;
      }

      PTRACE(3, "xJack\tStopping read codec");

      readMutex.Wait();
      if (!readStopped) {
        readStopped = true;
        IoControl(IOCTL_Record_Stop);
      }
      readMutex.Signal();

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(StopWriting, unsigned,line)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (inRawMode) {
        StopRawCodec(line);
        return PluginLID_NoError;
      }

      PTRACE(3, "xJack\tStopping write codec");

      writeMutex.Wait();
      if (!writeStopped) {
        writeStopped = true;
        IoControl(IOCTL_Playback_Stop);
      }
      writeMutex.Signal();

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetReadFrameSize, unsigned,line, unsigned,frameSize)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (!inRawMode)
        return PluginLID_OperationNotAllowed;

      readFrameSize = frameSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetWriteFrameSize, unsigned,line, unsigned,frameSize)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (!inRawMode)
        return PluginLID_OperationNotAllowed;

      writeFrameSize = frameSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetReadFrameSize, unsigned,line, unsigned *,frameSize)
    {
      if (frameSize == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      *frameSize = readFrameSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetWriteFrameSize, unsigned,line, unsigned *,frameSize)
    {
      if (frameSize == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      *frameSize = writeFrameSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(ReadFrame, unsigned,line, void *,buffer, unsigned *,count)
    {
      if (buffer == NULL || count == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      *count = 0;

      PWaitAndSignal mutex(readMutex);
      if (readStopped)
        return PluginLID_OperationNotAllowed;

      DWORD dwBytesReturned = 0;
      if (inRawMode) {
        if (WaitForSingleObjectEx(hReadEvent, 1000, TRUE) != WAIT_OBJECT_0) {
          PTRACE(1, "xJack\tRead Timeout!");
          return PluginLID_InternalError;
        }
        IoControl(IOCTL_Fax_Read, NULL, 0, buffer, readFrameSize, &dwBytesReturned);
        *count = (unsigned)dwBytesReturned;
        return PluginLID_NoError;
      }

      bool reblockG729 = CodecInfo[readCodecType].isG729;
      WORD temp_frame_buffer[6];

      PWin32Overlapped overlap;
      if (!IoControl(IOCTL_Device_Read, NULL, 0,
                    reblockG729 ? temp_frame_buffer         : buffer,
                    reblockG729 ? sizeof(temp_frame_buffer) : readFrameSize,
                    &dwBytesReturned, &overlap))
        return PluginLID_InternalError;

      if (reblockG729) {
        switch (temp_frame_buffer[0]) {
          case 1 :
            memcpy(buffer, &temp_frame_buffer[1], 10);
            *count = 10;
            break;
          case 2 :
            if (CodecInfo[readCodecType].vad) {
              *(WORD *)buffer = temp_frame_buffer[1];
              *count = 2;
            }
            else {
              memset(buffer, 0, 10);
              *count = 10;
            }
            break;
          default : // Must be old driver
            memcpy(buffer, temp_frame_buffer, 10);
            *count = 10;
        }
      }
      else if (CodecInfo[readCodecType].isG7231) {
        // Pick out special cases for G.723.1 based codecs (variable length frames)
        static const PINDEX g723count[4] = { 24, 20, 4, 1 };
        *count = g723count[(*(BYTE *)buffer)&3];
      }
      else
        *count = (unsigned)dwBytesReturned;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG4(WriteFrame, unsigned,line, const void *,buffer, unsigned,count, unsigned *,written)
    {
      if (buffer == NULL || written == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      *written = count;

      PWaitAndSignal mutex(writeMutex);
      if (writeStopped)
        return PluginLID_OperationNotAllowed;

      DWORD dwResult = 0;
      DWORD dwBytesReturned = 0;

      if (inRawMode) {
        for (*written = 0; *written < count; *written += dwResult) {
          if (WaitForSingleObjectEx(hWriteEvent, 1000, TRUE) != WAIT_OBJECT_0) {
            PTRACE(1, "xJack\tWrite Timeout!");
            return PluginLID_InternalError;
          }
          IoControl(IOCTL_Fax_Write, ((BYTE *)buffer)+*written, count-*written,
                    &dwResult, sizeof(dwResult), &dwBytesReturned);
        }
        return PluginLID_NoError;
      }

      WORD temp_frame_buffer[12];
      PINDEX bytesToWrite;

      if (CodecInfo[writeCodecType].isG7231) {
        // Pick out special cases for G.723.1 based codecs (variable length frames)
        switch ((*(BYTE *)buffer)&3) {
          case 0 : // 6.3kbps rate frame
            *written = 24;
            break;
          case 1 : // 5.3kbps rate frame
            *written = 20;
            break;
          case 2 : // a Silence Insertion Descriptor
            memset(temp_frame_buffer, 0, sizeof(temp_frame_buffer));
            *(DWORD *)(temp_frame_buffer) = *(const DWORD *)buffer;
            buffer = temp_frame_buffer;
            *written = 4;
            break;
          case 3 : // repeat last CNG frame
            // Check for frame erasure command
            if (memcmp(buffer, "\xff\xff\xff\xff", 4) == 0)
              *written = 24;
            else {
              memset(temp_frame_buffer, 0, sizeof(temp_frame_buffer));
              temp_frame_buffer[0] = 3;
              buffer = temp_frame_buffer;
              *written = 1;
            }
            break;
        }
        bytesToWrite = 24;
      }
      else if (CodecInfo[writeCodecType].isG729) {
        if (count == 2) {
          PTRACE_IF(2, !CodecInfo[readCodecType].vad,
                    "xJack\tG.729B frame received, but not selected.");
          temp_frame_buffer[0] = 2;
          temp_frame_buffer[1] = *(const WORD *)buffer;
          memset(&temp_frame_buffer[2], 0, 8);
          *written = 2;
        }
        else {
          if (memcmp(buffer, "\0\0\0\0\0\0\0\0\0", 10) == 0) {
    #if 0
            memset(temp_frame_buffer, 0, 12);
    #else
            // We really should be sending a full frame of zeros here, but the codec
            // makes a clicking sound if you do so send annex B CNG frames instead.
            temp_frame_buffer[0] = 2;
            memset(&temp_frame_buffer[1], 0, 10);
    #endif
          }
          else {
            temp_frame_buffer[0] = 1;
            memcpy(&temp_frame_buffer[1], buffer, 10);
          }
          *written = 10;
        }
        buffer = temp_frame_buffer;
        bytesToWrite = 12;
      }
      else {
        bytesToWrite = writeFrameSize;
        *written = bytesToWrite;
      }

      if (count < *written) {
        PTRACE(1, "xJack\tWrite of too small a buffer");
        return PluginLID_InvalidParameter;
      }

      PWin32Overlapped overlap;
      return IoControl(IOCTL_Device_Write, (void *)buffer, bytesToWrite,
                      &dwResult, sizeof(dwResult), &dwBytesReturned, &overlap) ? PluginLID_NoError : PluginLID_InternalError;
    }



    PLUGIN_FUNCTION_ARG3(GetAverageSignalLevel, unsigned,line, PluginLID_Boolean,playback, unsigned *,signal)
    {
      if (signal == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      DWORD dwLevel;
      if (!IoControl(playback ? IOCTL_Playback_GetAvgPlaybackLevel
                              : IOCTL_Record_GetAvgRecordLevel,
                    0, &dwLevel))
        return PluginLID_InternalError;

      *signal = (unsigned)dwLevel;
      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG2(EnableAudio, unsigned,line, PluginLID_Boolean,enable)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      DWORD dwSource = ANALOG_SOURCE_SPEAKERPHONE;

      if (enable) {
        if (enabledAudioLine != line) {
          if (enabledAudioLine != UINT_MAX && exclusiveAudioMode) {
            PTRACE(3, "xJack\tEnableAudio on port when already enabled other port.");
            return PluginLID_OperationNotAllowed;
          }
          enabledAudioLine = line;
        }
        dwSource = line == POTSLine ? ANALOG_SOURCE_POTSPHONE : ANALOG_SOURCE_PSTNLINE;
      }
      else
        enabledAudioLine = UINT_MAX;

      if (!IsLineJACK())
        return IoControl(IOCTL_DevCtrl_SetAnalogSource, dwSource) ? PluginLID_NoError : PluginLID_InternalError;

      PluginLID_Boolean connected;
      if (IsLineToLineDirect(POTSLine, PSTNLine, &connected) != PluginLID_NoError)
        return PluginLID_InternalError;

      IoControl(IOCTL_DevCtrl_SetLineJackMode,
                    dwSource == ANALOG_SOURCE_PSTNLINE ? LINEJACK_MODE_LINEJACK
                                                        : LINEJACK_MODE_PHONEJACK);
      SetLineToLineDirect(POTSLine, PSTNLine, connected);

      if (dwSource != ANALOG_SOURCE_PSTNLINE) {
        if (!IoControl(IOCTL_DevCtrl_SetAnalogSource, dwSource))
          return PluginLID_InternalError;
      }

      InternalSetVolume(true, RecordMicrophone,    -1, dwSource != ANALOG_SOURCE_SPEAKERPHONE);
      InternalSetVolume(true, RecordPhoneIn,       -1, dwSource != ANALOG_SOURCE_POTSPHONE);
      InternalSetVolume(false,PlaybackPhoneOut,    -1, dwSource != ANALOG_SOURCE_POTSPHONE);
      InternalSetVolume(true, RecordPhoneLineIn,   -1, dwSource != ANALOG_SOURCE_PSTNLINE);
      InternalSetVolume(false,PlaybackPhoneLineOut,-1, dwSource != ANALOG_SOURCE_PSTNLINE);
      InternalSetVolume(false,PlaybackWave,        -1, false);

      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG2(IsAudioEnabled, unsigned,line, PluginLID_Boolean *,enable)
    {
      if (enable == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      *enable = enabledAudioLine == line;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetRecordVolume, unsigned,line, unsigned,volume)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (IsLineJACK()) {
        if (!InternalSetVolume(true,
                              line == POTSLine ? RecordPhoneIn : RecordPhoneLineIn,
                              volume,
                              -1))
          return PluginLID_InternalError;
      }

      return InternalSetVolume(true, RecordMaster, volume, -1)? PluginLID_NoError : PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG2(SetPlayVolume, unsigned,line, unsigned,volume)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (IsLineJACK()) {
        if (!InternalSetVolume(false,
                              line == POTSLine ? PlaybackPhoneOut : PlaybackPhoneLineOut,
                              volume,
                              -1))
          return PluginLID_InternalError;
      }

      return InternalSetVolume(false, PlaybackMaster, volume, -1) ? PluginLID_NoError : PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG2(GetRecordVolume, unsigned,line, unsigned *,volume)
    {
      if (volume == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      MIXER_LINE mixer;
      mixer.dwLineID = 0;

      DWORD dwSize = 0;
      if (!IoControl(IOCTL_Mixer_GetRecordLineControls,
                    &mixer, sizeof(mixer), &mixer, sizeof(mixer), &dwSize))
        return PluginLID_InternalError;

      if (mixer.dwLeftVolume > 65208) // 99.5%
        *volume = 100;
      else
        *volume = mixer.dwLeftVolume*100/65536;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetPlayVolume, unsigned,line, unsigned *,volume)
    {
      if (volume == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      MIXER_LINE mixer;
      mixer.dwLineID = 0;

      DWORD dwSize = 0;
      if (!IoControl(IOCTL_Mixer_GetPlaybackLineControls,
                    &mixer, sizeof(mixer), &mixer, sizeof(mixer), &dwSize))
        return PluginLID_InternalError;

      if (mixer.dwLeftVolume > 65208) // 99.5%
        *volume = 100;
      else
        *volume = mixer.dwLeftVolume*100/65536;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetAEC, unsigned,line, unsigned *,level)
    {
      if (level == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      DWORD dwLevel = 0;
      if (!IoControl(IOCTL_Speakerphone_GetAEC, 0, &dwLevel))
        return PluginLID_InternalError;

      *level = (unsigned)dwLevel;
      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG2(SetAEC, unsigned,line, unsigned,level)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return IoControl(IOCTL_Speakerphone_SetAEC, level) ? PluginLID_NoError : PluginLID_InternalError;
    }

    PLUGIN_FUNCTION_ARG2(GetVAD, unsigned,line, PluginLID_Boolean *,enable)
    {
      if (enable == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      *enable = vadEnabled;
      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG2(SetVAD, unsigned,line, PluginLID_Boolean,enable)
    {
      PTRACE(3, "xJack\tSet VAD " << (enable ? "on" : "off"));
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (!IoControl(enable ? IOCTL_Record_EnableVAD : IOCTL_Record_DisableVAD))
        return PluginLID_InternalError;

      vadEnabled = enable;
      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG4(GetCallerID, unsigned,line, char *,idString, unsigned,size, PluginLID_Boolean,full)
    {
      if (idString == NULL || size == 0)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (line != PSTNLine)
        return PluginLID_OperationNotAllowed;

      BYTE buffer[512];
      buffer[0] = 0;
      DWORD dwBytesReturned;
      if (!IoControl(IOCTL_DevCtrl_LineGetCallerID,
                    buffer, sizeof(buffer),
                    buffer, sizeof(buffer),
                    &dwBytesReturned))
        return PluginLID_InternalError;

      PTRACE_IF(3, buffer[0] != 0, "xJack\tCaller ID:\n"
                << hex << setprecision(2)
                << PBYTEArray(buffer, dwBytesReturned)
                << dec);

      PString name, timeStr, idStr;

      switch (buffer[0]) {
        case 4 : // Single message
          timeStr = PString((char *)&buffer[2], 8);
          idStr = PString((char *)&buffer[10], buffer[1]-8);
          break;

        case 128 : {
          PINDEX totalLength = buffer[1];
          PINDEX pos = 2;
          while (pos < totalLength) {
            switch (buffer[pos]) {
              case 1 :
                timeStr = PString((char *)&buffer[pos+2], buffer[pos+1]);
                break;
              case 2 :
                idStr = PString((char *)&buffer[pos+2], buffer[pos+1]);
                break;
              case 7 :
                name = PString((char *)&buffer[pos+2], buffer[pos+1]);
                break;
            }
            pos += buffer[pos+1]+2;
          }
          break;
        }

        default :
          return PluginLID_InternalError;
      }

      if (full && !timeStr.IsEmpty()) {
        PTime now;
        int minute = timeStr(6,7).AsUnsigned() % 60;
        int hour   = timeStr(4,5).AsUnsigned() % 24;
        int day    = timeStr(2,3).AsUnsigned();
        if (day < 1)
          day = 1;
        else if (day > 31)
          day = 31;
        int month  = timeStr(0,1).AsUnsigned();
        if (month < 1)
          month = 1;
        else if (month > 12)
          month = 12;

        PTime theTime(0, minute, hour, day, month, now.GetYear());
        idStr += '\t' + theTime.AsString(PTime::ShortDateTime) + '\t' + name;
      }

      strncpy(idString, idStr, size-1);
      idString[size-1] = '\0';
      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG2(SetCallerID, unsigned,line, const char *,idString)
    {
      if (idString == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (line != POTSLine)
        return PluginLID_OperationNotAllowed;

      PString name, number;
      PTime theTime;

      PStringArray fields = PString(idString).Tokenise('\t', true);
      switch (fields.GetSize()) {
        case 3 :
          name = fields[2];
        case 2 :
          theTime = PTime(fields[1]);
        case 1 :
        number = fields[0];
        break;
        default :
          return PluginLID_InvalidParameter;
      }

      PINDEX numberLength = number.GetLength();
      PINDEX nameLength = name.GetLength();

      char buffer[256];
      buffer[0] = 1;
      buffer[1] = '\x80';
      buffer[2] = (char)(14+numberLength+nameLength);
      buffer[3] = 1;
      buffer[4] = 8;
      sprintf(&buffer[5],
              "%02u%02u%02u%02u",
              theTime.GetMonth(),
              theTime.GetDay(),
              theTime.GetHour(),
              theTime.GetMinute());
      buffer[13] = 2;
      buffer[14] = (char)numberLength;
      strcpy(&buffer[15], number);
      buffer[15+numberLength] = 7;
      buffer[16+numberLength] = (char)nameLength;
      strcpy(&buffer[17+numberLength], name);

      DWORD dwReturn = 0;
      DWORD dwBytesReturned;
      return IoControl(IOCTL_FSK_SetMsgData,
                      buffer, 17+numberLength+nameLength,
                      &dwReturn, sizeof(dwReturn), &dwBytesReturned) ? PluginLID_NoError : PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG2(SendVisualMessageWaitingIndicator, unsigned,line, PluginLID_Boolean,on)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (line != POTSLine)
        return PluginLID_OperationNotAllowed;

      PluginLID_Boolean offHook;
      if (IsLineOffHook(line, &offHook) != PluginLID_NoError)
        return PluginLID_InternalError;

      if (offHook)
        return PluginLID_OperationNotAllowed;

      BYTE buffer[] = { 0, 130, 3, 11, 1, 0 };
      if (on)
        buffer[5] = 255;
      DWORD dwReturn = 0;
      DWORD dwBytesReturned;
      return IoControl(IOCTL_FSK_SetMsgData,
                      buffer, sizeof(buffer),
                      &dwReturn, sizeof(dwReturn), &dwBytesReturned) ? PluginLID_NoError : PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG4(PlayDTMF, unsigned,line, const char *,digits, unsigned,onTime, unsigned,offTime)
    {
      if (digits == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      while (*digits != '\0') {
        DWORD dwToneIndex;
        int digit = toupper(*digits++);
        switch (digit) {
          case '0' :
            dwToneIndex = IDLE_TONE_0;
            break;
          case '1' :
            dwToneIndex = IDLE_TONE_1;
            break;
          case '2' :
            dwToneIndex = IDLE_TONE_2;
            break;
          case '3' :
            dwToneIndex = IDLE_TONE_3;
            break;
          case '4' :
            dwToneIndex = IDLE_TONE_4;
            break;
          case '5' :
            dwToneIndex = IDLE_TONE_5;
            break;
          case '6' :
            dwToneIndex = IDLE_TONE_6;
            break;
          case '7' :
            dwToneIndex = IDLE_TONE_7;
            break;
          case '8' :
            dwToneIndex = IDLE_TONE_8;
            break;
          case '9' :
            dwToneIndex = IDLE_TONE_9;
            break;
          case '*' :
            dwToneIndex = IDLE_TONE_STAR;
            break;
          case '#' :
            dwToneIndex = IDLE_TONE_POUND;
            break;
          case 'A' :
            dwToneIndex = IDLE_TONE_A;
            break;
          case 'B' :
            dwToneIndex = IDLE_TONE_B;
            break;
          case 'C' :
            dwToneIndex = IDLE_TONE_C;
            break;
          case 'D' :
            dwToneIndex = IDLE_TONE_D;
            break;
          case ',' :
            dwToneIndex = IDLE_TONE_NOTONE;
            Sleep(2000);
            break;
          default :
            if ('E' <= digit && digit <= ('E' + 11))
              dwToneIndex = (digit - 'E') + 13;
            else {
              dwToneIndex = IDLE_TONE_NOTONE;
              Sleep(onTime+offTime);
            }
            break;
        }

        if (dwToneIndex != IDLE_TONE_NOTONE) {
          if (!InternalPlayTone(line, dwToneIndex, onTime, offTime, true))
            return PluginLID_InternalError;
        }
      }

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(ReadDTMF, unsigned,line, char *,digit)
    {
      if (digit == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      DWORD dwNewDigit;
      if (!IoControl(IOCTL_Filter_GetDTMFDigit, lastDTMFDigit, &dwNewDigit))
        return PluginLID_InternalError;

      if (dwNewDigit == 0 || dwNewDigit == lastDTMFDigit) {
        *digit = '\0';
        return PluginLID_NoError;
      }

      lastDTMFDigit = dwNewDigit;

      static char const dtmf[16] = {
        'D','1','2','3','4','5','6','7','8','9','*','0','#','A','B','C'
      };
      PTRACE(3, "xJack\tDetected DTMF tone: " << dtmf[dwNewDigit&0xf]);

      *digit = dtmf[dwNewDigit&0xf];
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetRemoveDTMF, unsigned,line, PluginLID_Boolean *,removeTones)
    {
      if (removeTones == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      DWORD result = false;
      if (!IoControl(IOCTL_Record_GetDisableOnDTMFDetect, 0, &result))
        return PluginLID_InternalError;

      *removeTones = result != 0;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetRemoveDTMF, unsigned,line, PluginLID_Boolean,removeTones)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return IoControl(IOCTL_Record_SetDisableOnDTMFDetect, removeTones) ? PluginLID_NoError : PluginLID_InternalError;
    }



    PLUGIN_FUNCTION_ARG2(IsToneDetected, unsigned,line, int *,tone)
    {
      if (tone == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      if (EnableAudio(line, true) != PluginLID_NoError)
        return PluginLID_InternalError;

      DWORD dwReturn = 0;
      if (!IoControl(IOCTL_Filter_IsToneCadenceValid, 0, &dwReturn))
        return PluginLID_InternalError;

      if (dwReturn != 0) 
        *tone = PluginLID_DialTone;
      else {
        dwReturn = 0;
        if (!IoControl(IOCTL_Filter_IsToneCadenceValid, 1, &dwReturn))
          return PluginLID_InternalError;
        if (dwReturn != 0) 
          *tone = PluginLID_RingTone;
        else {
          dwReturn = 0;
          if (!IoControl(IOCTL_Filter_IsToneCadenceValid, 2, &dwReturn))
            return PluginLID_InternalError;
          if (dwReturn != 0) 
            *tone = PluginLID_BusyTone;
          else {
            dwReturn = 0;
            if (!IoControl(IOCTL_Filter_IsToneCadenceValid, 3, &dwReturn))
              return PluginLID_InternalError;
            *tone = dwReturn != 0 ? PluginLID_CNGTone : PluginLID_NoTone;
          }
        }
      }

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
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      DWORD toneIndex;
      switch (tone) {
        case PluginLID_DialTone :
          toneIndex = 0;
          break;
        case PluginLID_RingTone :
          toneIndex = 1;
          break;
        case PluginLID_BusyTone :
          toneIndex = 2;
          break;
        case PluginLID_CNGTone :
          toneIndex = 3;
          break;
        default :
          PTRACE(1, "xJack\tCannot set filter for tone: " << tone);
          return PluginLID_InvalidParameter;
      }

      if (numCadences > 0) {
        if (onTimes == NULL || offTimes == NULL)
          return PluginLID_InvalidParameter;

        qthDetectToneCadence dtc;

        if (numCadences > PARRAYSIZE(dtc.element)) {
          PTRACE(1, "xJack\tToo many cadence elements: " << numCadences);
          return PluginLID_InvalidParameter;
        }


        dtc.ulFilter = toneIndex;
        dtc.ulNumElements = numCadences;
        dtc.type = QTH_DETECT_TONE_TYPE_ADD;
        dtc.term = QTH_DETECT_TONE_REPEAT_ALL;
        dtc.ulTolerance = 10;  // in %
        dtc.ulMinDetectLoops = 1;
        memset(dtc.element, 0, sizeof(dtc.element));
        for (unsigned i = 0; i < numCadences; i++) {
          dtc.element[i].ulOnTime = onTimes[i];
          dtc.element[i].ulOffTime = offTimes[i];
        }

        PTRACE(2, "xJack\tSetting cadence for tone index " << toneIndex
              << ", num=" << numCadences
              << ' ' << dtc.element[0].ulOnTime
              << '-' << dtc.element[0].ulOffTime);

        DWORD dwReturn = 0;
        DWORD dwBytesReturned;
        IoControl(IOCTL_Filter_DetectToneCadence, &dtc, sizeof(dtc),
                  &dwReturn, sizeof(dwReturn), &dwBytesReturned, false);
      }

      static struct FilterTableEntry {
        unsigned frequency1;
        unsigned frequency2;
        unsigned predefinedFilterSet;  // 0 = custom
        short    coefficients[19];
      } const FilterTable[] = {
        {  300, 640, 4 },
        {  300, 500, 5 },
        { 1100,1100, 6 },
        {  350, 350, 7 },
        {  400, 400, 8 },
        {  480, 480, 9 },
        {  440, 440, 10},
        {  620, 620, 11},
        {  425, 425, 0, 30850,-32534,-504,0,504,30831,-32669,24303,-22080,24303,30994,-32673, 1905, -1811, 1905,5,129,17,0xff5  },
        {  350, 440, 0, 30634,-31533,-680,0,680,30571,-32277,12894,-11945,12894,31367,-32379,23820,-23104,23820,7,159,21,0x0FF5 },
        {  400, 450, 0, 30613,-32031,-618,0,618,30577,-32491, 9612, -8935, 9612,31071,-32524,21596,-20667,21596,7,159,21,0x0FF5 },
      };

      FilterTableEntry match = { 0, 0, UINT_MAX };

      PINDEX i;

      // Look for exact match
      for (i = 0; i < PARRAYSIZE(FilterTable); i++) {
        if (frequency1  == FilterTable[i].frequency1 &&
            frequency2 == FilterTable[i].frequency2) {
          match = FilterTable[i];
          break;
        }
      }

      if (match.predefinedFilterSet == UINT_MAX) {
        // If single frequency, make a band out of it, +/- 5%
        if (frequency1 == frequency2) {
          frequency1  -= frequency1/20;
          frequency2 += frequency2/20;
        }

        // Try again looking for a band that is just a bit larger than required, no
        // more than twice the size required.
        for (i = 0; i < PARRAYSIZE(FilterTable); i++) {
          if (frequency1  > FilterTable[i].frequency1 &&
              frequency2 < FilterTable[i].frequency2 &&
              2*(frequency2 - frequency1) >
                      (FilterTable[i].frequency2 - FilterTable[i].frequency1)) {
            match = FilterTable[i];
            break;
          }
        }
      }

      if (match.predefinedFilterSet == UINT_MAX) {
        PTRACE(1, "xJack\tInvalid frequency for fixed filter sets: "
                << frequency1 << '-' << frequency2);
        return PluginLID_InvalidParameter;
      }

      struct {
        DWORD dwFilterNum;
        union {
          DWORD predefinedFilterSet;
          short coefficients[19];
        };
      } filterSet;
      PINDEX sizeOfFilterSet;

      if (match.predefinedFilterSet != 0) {
        filterSet.predefinedFilterSet = match.predefinedFilterSet;
        sizeOfFilterSet = sizeof(filterSet.dwFilterNum) + sizeof(filterSet.predefinedFilterSet);
      }
      else {
        memcpy(filterSet.coefficients, match.coefficients, sizeof(filterSet.coefficients));
        sizeOfFilterSet = sizeof(filterSet);
      }

      filterSet.dwFilterNum = toneIndex;

      PTRACE(2, "xJack\tSetting filter for tone index " << toneIndex
            << " freq: " << match.frequency1 << '-' << match.frequency2);

      DWORD dwReturn = 0;
      DWORD dwBytesReturned;
      return IoControl(IOCTL_Filter_ProgramFilter, &filterSet, sizeOfFilterSet,
                      &dwReturn, sizeof(dwReturn), &dwBytesReturned, FALSE) ? PluginLID_NoError : PluginLID_InternalError;
    }

    PLUGIN_FUNCTION_ARG2(PlayTone, unsigned,line, unsigned,tone)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      switch (tone) {
        case PluginLID_DialTone :
          return InternalPlayTone(line, IDLE_TONE_DIAL, 0, 0, false) ? PluginLID_NoError : PluginLID_InternalError;
        case PluginLID_RingTone :
          return InternalPlayTone(line, IDLE_TONE_RING, 0, 0, false) ? PluginLID_NoError : PluginLID_InternalError;
        case PluginLID_BusyTone :
          return InternalPlayTone(line, IDLE_TONE_BUSY, 0, 0, false) ? PluginLID_NoError : PluginLID_InternalError;
        case PluginLID_ClearTone :
          return InternalPlayTone(line, IDLE_TONE_BUSY, 0, 0, false) ? PluginLID_NoError : PluginLID_InternalError;
        default :
          return InternalPlayTone(line, IDLE_TONE_NOTONE, 0, 0, false) ? PluginLID_NoError : PluginLID_InternalError;
      }
    }

    PLUGIN_FUNCTION_ARG2(IsTonePlaying, unsigned,line, PluginLID_Boolean *,playing)
    {
      if (playing == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      *playing = PTimer::Tick() < toneSendCompletionTime;
      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG1(StopTone, unsigned,line)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      PTRACE(3, "xJack\tStopping tones");

      return IoControl(IOCTL_Idle_StopTone) ? PluginLID_NoError : PluginLID_InternalError;
    }

    //PLUGIN_FUNCTION_ARG4(DialOut, unsigned,line, const char *,number, PluginLID_Boolean,requireTones, unsigned,uiDialDelay)

    PLUGIN_FUNCTION_ARG2(GetWinkDuration, unsigned,line, unsigned *,winkDuration)
    {
      if (winkDuration == NULL)
        return PluginLID_InvalidParameter;

      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      DWORD level = 0;
      IoControl(IOCTL_DevCtrl_GetLineWinkDetTime, 0, &level);
      *winkDuration = (unsigned)level;
      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG2(SetWinkDuration, unsigned,line, unsigned,winkDuration)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      if (IsLineInvalid(line))
        return PluginLID_NoSuchLine;

      return IoControl(IOCTL_DevCtrl_SetLineWinkDetTime, winkDuration) ? PluginLID_NoError : PluginLID_InternalError;
    }

    PLUGIN_FUNCTION_ARG1(SetCountryCode, unsigned,country)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return PluginLID_DeviceNotOpen;

      // if a LineJack, the set the DAA coeffiecients
      if (!IsLineJACK())
        return PluginLID_OperationNotAllowed;

      if (country >= OpalLineInterfaceDevice::UnknownCountry)
        return PluginLID_InvalidParameter;

      PINDEX i;
      for (i = PARRAYSIZE(CountryInfo)-1; i > 0; i--) {
        if (CountryInfo[i].t35Code == countryCode)
          break;
      }

      PTRACE(2, "xJack\tSetting coefficient group for " << CountryInfo[i].t35Code);
      return IoControl(IOCTL_DevCtrl_SetCoefficientGroup, CountryInfo[i].ixjCode) ? PluginLID_NoError : PluginLID_InternalError;
    }

    PLUGIN_FUNCTION_ARG2(GetSupportedCountry, unsigned,index, unsigned *,countryCode)
    {
      if (countryCode == NULL)
        return PluginLID_InvalidParameter;

      if (index >= PARRAYSIZE(CountryInfo))
        return PluginLID_NoMoreNames;

      *countryCode = CountryInfo[index].t35Code;
      return PluginLID_NoError;
    }


  protected:
    void GetDeviceNames()
    {
      PINDEX i;

      const char * DevicePath = "\\\\.\\QTJACKDevice%u";

      switch (GetOperatingSystem()) {
        case IsWindows2k :
          DevicePath = "\\\\.\\QTIWDMDevice%u";
          // Fall into NT case

        case IsWindowsNT :
          for (i = 0; i < 100; i++) {
            PString devpath;
            devpath.sprintf(DevicePath, i);
            HANDLE hDriver = CreateFile(devpath,
                                        GENERIC_READ,
                                        FILE_SHARE_WRITE,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                        NULL);
            if (hDriver != INVALID_HANDLE_VALUE) {
              DWORD devId, bytesReturned;
              if (DeviceIoControl(hDriver, IOCTL_Device_GetSerialNumber,
                                  NULL, 0, &devId, sizeof(devId), &bytesReturned, NULL) &&
                                                        bytesReturned == sizeof(devId) && devId != 0)
                deviceNames.SetAt(deviceNames.GetSize(), new PCaselessString(devpath));
              CloseHandle(hDriver);
            }
          }
          break;

        case IsWindows9x :
          PStringArray devices = PSoundChannel::GetDeviceNames(PSoundChannel::Player);
          for (i = 0; i < devices.GetSize(); i++) {
            PString dev = devices[i];
            if (dev.Find("Internet") != P_MAX_INDEX &&
                  (dev.Find("JACK") != P_MAX_INDEX || dev.Find("PhoneCARD") != P_MAX_INDEX)) {
	      PINDEX lparen = dev.Find('(');
	      PINDEX rparen = dev.Find(')', lparen);
	      deviceNames.SetAt(deviceNames.GetSize(), new PCaselessString(dev(lparen+1, rparen-1)));
            }
          }
      }
    }

    DWORD GetSerialNumber()
    {
      if (GetOperatingSystem() == IsWindows9x)
        return deviceName.AsUnsigned(16);

      DWORD devId;
      if (IoControl(IOCTL_Device_GetSerialNumber, 0, &devId))
        return devId;

      return 0;
    }

    bool SetRawCodec(unsigned)
    {
      if (inRawMode)
        return true;

      PTRACE(3, "xJack\tSetRawCodec()");

      // Default to 30ms frames of 16 bit PCM data
      readFrameSize = 480;
      writeFrameSize = 480;

      if (hReadEvent == NULL)
        hReadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
      if (hWriteEvent == NULL)
        hWriteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

      HANDLE hEventArr[2];

      if (GetOperatingSystem() == IsWindows9x) {
        CHAR K32Path[MAX_PATH];
        HINSTANCE hK32;
        HANDLE (WINAPI *OpenVxDHandle)(HANDLE);

        GetSystemDirectory(K32Path, MAX_PATH);
        strcat(K32Path, "\\kernel32.dll");
        hK32 = LoadLibrary(K32Path);

        OpenVxDHandle = (HANDLE(WINAPI *)(HANDLE))GetProcAddress(hK32, "OpenVxDHandle");
        hEventArr[0] = OpenVxDHandle(hReadEvent);
        hEventArr[1] = OpenVxDHandle(hWriteEvent);
        FreeLibrary(hK32);
      }
      else
      {
        hEventArr[0] = hReadEvent;
        hEventArr[1] = hWriteEvent;
      }

      readMutex.Wait();
      writeMutex.Wait();

      DWORD dwReturn, dwBytesReturned;
      inRawMode = IoControl(IOCTL_Fax_Start,
                            hEventArr, sizeof(hEventArr),
                            &dwReturn, sizeof(dwReturn), &dwBytesReturned);
      readCodecType = writeCodecType = 0;
      readStopped = writeStopped = !inRawMode;

      readMutex.Signal();
      writeMutex.Signal();

      return inRawMode;
    }

    void StopRawCodec(unsigned line)
    {
      PTRACE(3, "xJack\tStopping raw codec");

      readMutex.Wait();
      writeMutex.Wait();
      readStopped = true;
      writeStopped = true;
      BOOL ok = IoControl(IOCTL_Fax_Stop);
      readMutex.Signal();
      writeMutex.Signal();

      inRawMode = false;
    }

    bool InternalSetVolume(bool record, unsigned id, int volume, int mute)
    {
      MIXER_LINE mixer;
      mixer.dwLineID = id;

      DWORD dwSize = 0;
      if (!IoControl(record ? IOCTL_Mixer_GetRecordLineControls
                            : IOCTL_Mixer_GetPlaybackLineControls,
                    &mixer, sizeof(mixer), &mixer, sizeof(mixer), &dwSize))
        return false;

      if (volume >= 0) {
        if (volume >= 100)
          mixer.dwRightVolume = 65535;
        else
          mixer.dwRightVolume = volume*65536/100;
        mixer.dwLeftVolume = mixer.dwRightVolume;
      }
      if (mute >= 0)
        mixer.dwMute = mute != 0;

      DWORD dwReturn;
      return IoControl(record ? IOCTL_Mixer_SetRecordLineControls
                              : IOCTL_Mixer_SetPlaybackLineControls,
                      &mixer, sizeof(mixer), &dwReturn, sizeof(dwReturn), &dwSize);
    }

    bool InternalPlayTone(unsigned line,
                          DWORD toneIndex,
                          DWORD onTime, DWORD offTime,
                          bool synchronous)
    {
      StopTone(line);

      PTRACE(3, "xJack\tPlaying tone: "
            << toneIndex << ' ' << onTime << ' ' << offTime << ' ' << synchronous);

      IDLE_TONE tone;

      tone.dwToneIndex = toneIndex;
      tone.dwToneOnPeriod = onTime;
      tone.dwToneOffPeriod = offTime;
      tone.dwDuration = tone.dwToneOnPeriod+tone.dwToneOffPeriod;
      tone.dwMasterGain = 15;

      DWORD dwReturn = 0;
      DWORD dwBytesReturned;
      if (!IoControl(IOCTL_Idle_PlayTone,
                    &tone, sizeof(tone),
                    &dwReturn, sizeof(dwReturn), &dwBytesReturned) ||
          dwBytesReturned != sizeof(dwReturn) ||
          dwReturn == 0)
        return false;

      toneSendCompletionTime = PTimer::Tick() + (int)tone.dwDuration - 1;
      if (synchronous)
        Sleep(tone.dwDuration);

      return true;
    }

    bool IoControl(DWORD dwIoControlCode,
                   DWORD inParam = 0,
                   DWORD * outParam = NULL)
    {
      DWORD dwDummy;
      if (outParam == NULL)
        outParam = &dwDummy;

      DWORD dwBytesReturned = 0;
      return IoControl(dwIoControlCode, &inParam, sizeof(DWORD),
                      outParam, sizeof(DWORD), &dwBytesReturned) &&
            dwBytesReturned == sizeof(DWORD);
    }

    bool IoControl(DWORD dwIoControlCode,
                   LPVOID lpInBuffer,
                   DWORD nInBufferSize,
                   LPVOID lpOutBuffer,
                   DWORD nOutBufferSize,
                   LPDWORD lpdwBytesReturned,
                   PWin32Overlapped * overlap = NULL)
    {
      if (hDriver == INVALID_HANDLE_VALUE)
        return false;

      DWORD newError = ERROR_SUCCESS;
      if (!DeviceIoControl(hDriver,
                          dwIoControlCode,
                          lpInBuffer,
                          nInBufferSize,
                          lpOutBuffer,
                          nOutBufferSize,
                          lpdwBytesReturned,
                          overlap)) {
        newError = ::GetLastError();
        while (newError == ERROR_IO_PENDING) {
          if (WaitForSingleObject(overlap->hEvent, 1000) != WAIT_OBJECT_0) {
            newError = ERROR_TIMEOUT;
            PTRACE(1, "xJack\tRead/Write Timeout!");
          }
          else if (GetOverlappedResult(hDriver, overlap, lpdwBytesReturned, FALSE))
            newError = ERROR_SUCCESS;
          else
            newError = ::GetLastError();
        }
      }

      PTRACE_IF(1, newError != ERROR_SUCCESS,
                "xJack\tError in DeviceIoControl, device=\"" << deviceName << "\", code=" << newError);

      return newError == ERROR_SUCCESS;
    }
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
    Context::SetCallerID,
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

class PluginLIDProcess : public PProcess
{
public:
  PluginLIDProcess()
    : PProcess("Open Phone Abstraction Library", "PluginLLID", 1, 0, ReleaseCode, 0)
  {
  }


  void Main()
  {
  }
};


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpArg)
{
  if (fdwReason == DLL_PROCESS_ATTACH) {
    PProcess::PreInitialise(0, NULL, NULL);
    static PluginLIDProcess myProcess;
  }
  return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
