/*
 * TigerJet USB HID Plugin LID for OPAL
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
 * Based on code by Simon Horne of ISVO (Asia) Pte Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: tj_win.cpp,v $
 * Revision 1.6  2006/11/12 03:35:15  rjongbloed
 * Fixed destruction race condition
 *
 * Revision 1.5  2006/11/05 05:04:46  rjongbloed
 * Improved the terminal LID line ringing, epecially for country emulation.
 *
 * Revision 1.4  2006/10/25 22:26:15  rjongbloed
 * Changed LID tone handling to use new tone generation for accurate country based tones.
 *
 * Revision 1.3  2006/10/22 12:08:51  rjongbloed
 * Major change so that sound card based LIDs, eg USB handsets. are handled in
 *   common code so not requiring lots of duplication.
 *
 * Revision 1.2  2006/10/16 09:46:49  rjongbloed
 * Fixed various MSVC 8 warnings
 *
 * Revision 1.1  2006/10/15 07:19:21  rjongbloed
 * Added first cut of TigerJet USB handset LID plug in
 *
 */

#define _CRT_SECURE_NO_DEPRECATE
#include <windows.h>

#define PLUGIN_DLL_EXPORTS
#include <lids/lidplugin.h>

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <process.h>
#include <assert.h>
#include <queue>

#include "TjIpSys.h"


/////////////////////////////////////////////////////////////////////////////

static HINSTANCE g_hInstance;


class CriticalSection : CRITICAL_SECTION
{
public:
  inline CriticalSection()  { InitializeCriticalSection(this); }
  inline ~CriticalSection() { DeleteCriticalSection(this); }
  inline void Enter()       { EnterCriticalSection(this); }
  inline void Leave()       { LeaveCriticalSection(this); }
};


/////////////////////////////////////////////////////////////////////////////

class Context
{
  protected:
    HMODULE          m_hDLL;
    TJIPSYSCALL      m_pTjIpSysCall;
    TjIpProductID    m_eProductID;
    bool             m_hasDTMF;
    bool             m_hasBeeper;
    bool             m_hasMixer;

    bool             m_isOffHook;
    bool             m_tonePlaying;

    HANDLE           m_hRingThread;
    HANDLE           m_hRingStopEvent;
    unsigned         m_ringFrequency;
    std::vector<unsigned> m_ringCadence;

    std::queue<char> m_Keys;
    CriticalSection  m_KeyMutex;


    static void StaticKeyCallback(void * context, UINT message, WPARAM wParam, LPARAM lParam)
    {
      ((Context *)context)->KeyCallback(message, wParam, lParam);
    }

    void KeyCallback(UINT message, WPARAM wParam, LPARAM lParam)
    {
      m_KeyMutex.Enter();

      switch (message) {
	case WM_TJIP_HID_KEY_RELEASED:
          Beep(m_ringFrequency);
          break;

	case WM_TJIP_HID_NEW_KEY:
          static struct {
            WPARAM code;
            char   ascii;
            int    frequency;
            int    hook; // 1 is off hook, -1 if on hook, 0 is no change
          } KeyInfo[] = {
            { 0xb0, '0', (941+1336)/2 }, // Average the DTMF frequencies
            { 0xb1, '1', (697+1209)/2 },
            { 0xb2, '2', (697+1336)/2 },
            { 0xb3, '3', (697+1477)/2 },
            { 0xb4, '4', (770+1209)/2 },
            { 0xb5, '5', (770+1336)/2 },
            { 0xb6, '6', (770+1477)/2 },
            { 0xb7, '7', (852+1209)/2 },
            { 0xb8, '8', (852+1336)/2 },
            { 0xb9, '9', (852+1477)/2 },
            { 0xba, '*', (941+1209)/2 },
            { 0xbb, '#', (941+1477)/2 },
            { 0x51, 'd', 900 }, // down
            { 0x52, 'u', 1400 }, // up
            { 0x2f, 'm', 500 }, // Mute
            { 0x2a, '\b', 2500 }, // Clear/Backspace key
            { 0x31, '\r', 1800, 1 }, // Enter (dial) key
            { 0x26, '\033', 700, -1 }  // Escape (hangup) key
          };
          for (int i = 0; i < sizeof(KeyInfo)/sizeof(KeyInfo[0]); i++) {
            if (wParam == KeyInfo[i].code) {
              Beep(KeyInfo[i].frequency);
              switch (KeyInfo[i].hook) {
                case 1 :
                  if (!m_isOffHook) {
                    m_isOffHook = true;
                    while (!m_Keys.empty())
                      m_Keys.pop();
                  }
                  break;

                case -1 :
                  m_isOffHook = false;
                  break;

                default :
                  m_Keys.push(KeyInfo[i].ascii);
              }
              break;
            }
          }
      }
      m_KeyMutex.Leave();
    }

    bool ReadRegister(unsigned regIndex, void * data, unsigned len = 1)
    {
      TJ_SEND_VENDOR_CMD vc;

      vc.vcCmd.direction = 1;	// read
      vc.vcCmd.bRequest = 4; // 320ns command pulse with auto incremented address
      vc.vcCmd.wValue = 0;
      vc.vcCmd.wIndex = regIndex;
      vc.vcCmd.wLength = len;
      vc.dwDataSize = len;
      vc.pDataBuf = data;

      return m_pTjIpSysCall(TJIP_TJ560VENDOR_COMMAND, 0, &vc) != 0;
    }

    bool WriteRegister(unsigned regIndex, const void * data, unsigned len)
    {
      TJ_SEND_VENDOR_CMD vc;

      vc.vcCmd.direction = 0;	// write
      vc.vcCmd.bRequest = 4; // 320ns command pulse with auto incremented address
      vc.vcCmd.wValue = 0;
      vc.vcCmd.wIndex = regIndex;
      vc.vcCmd.wLength = len;
      vc.vcCmd.bData   = 0x55;
      vc.dwDataSize = len;
      vc.pDataBuf = (PVOID)data;

      return m_pTjIpSysCall(TJIP_TJ560VENDOR_COMMAND, 0, &vc) != 0;
    }

    bool WriteRegister(unsigned regIndex, BYTE data)
    {
      return WriteRegister(regIndex, &data, 1);
    }

    void Beep(unsigned frequency)
    {
      if (!m_hasBeeper)
        return;

      if (frequency == 0) {
        WriteRegister(0x1a, 0x1);     // Turn Beeper off
        return;
      }

      int divisor = 46875 / frequency - 1;
      if (divisor < 1)
	divisor = 1;
      else if (divisor > 255)
	divisor = 255;

      WriteRegister(0x18, divisor); // set frequency
      WriteRegister(0x1a, 0x9);     // Turn Beeper on
    }



  public:
    PLUGIN_LID_CTOR()
    {
      m_eProductID = TJIP_NONE;
      m_hasDTMF = false;
      m_hasBeeper = false;
      m_hasMixer = false;
      m_isOffHook = false;
      m_tonePlaying = false;
      m_hRingThread = NULL;
      m_hRingStopEvent = NULL;
      m_ringFrequency = 0;

      m_hDLL = LoadLibrary("TjIpSys.dll");
      if (m_hDLL== NULL)
        return;

      m_pTjIpSysCall = (TJIPSYSCALL)GetProcAddress(m_hDLL, "TjIpSysCall");
      if (m_pTjIpSysCall == NULL)
        return;
      
      if (!m_pTjIpSysCall(TJIP_SYS_OPEN, (int)g_hInstance, &m_eProductID))
        return;

      TJ_CALLBACK_INFO info;
      info.fpCallback = StaticKeyCallback;
      info.context = this;
      m_pTjIpSysCall(TJIP_SET_CALLBACK, 0, &info);
    }

    PLUGIN_LID_DTOR()
    {
      Close();

      if (m_eProductID != TJIP_NONE) {
        TJ_CALLBACK_INFO info;
        info.fpCallback = NULL;
        info.context = NULL;
        m_pTjIpSysCall(TJIP_SET_CALLBACK, 0, &info);

        m_pTjIpSysCall(TJIP_SYS_CLOSE, 0, NULL);
      }

      if (m_hDLL != NULL)
        FreeLibrary(m_hDLL);
    }


    PLUGIN_FUNCTION_ARG3(GetDeviceName,unsigned,index, char *,buffer, unsigned,bufsize)
    {
      if (buffer == NULL || bufsize == 0)
        return PluginLID_InvalidParameter;

      if (m_hDLL== NULL)
        return PluginLID_InternalError;

      if (index > 0)
        return PluginLID_NoMoreNames;

      const char * searchName;
      switch (m_eProductID)
      {
        case TJIP_NONE :
          return PluginLID_NoSuchDevice;

	case TJIP_TJ560BPPG :
	case TJIP_TJ560BPPG_NO_EC :
	case TJIP_TJ560CPPG_NO_EC :
	case TJIP_TJ560BPPGPROSLIC :
	case TJIP_TJ560BPPGPROSLIC_NO_EC :
          searchName = "Personal PhoneGateway-USB";
          break;

        case TJIP_TJ320PHONE :
          searchName = "PCI Internet Phone";
          break;

        default :
          searchName = "USB Audio";
      }

      for (UINT id = 0; id < waveInGetNumDevs(); id++) {
        static const char wmm[] = "WindowsMultimedia:";
        WAVEINCAPS caps;
        if (waveInGetDevCaps(id, &caps, sizeof(caps)) == 0 && strstr(caps.szPname, searchName) != NULL) {
          if (bufsize <= strlen(caps.szPname)+sizeof(wmm))
            return PluginLID_BufferTooSmall;

          strcpy(buffer, wmm);
          strcat(buffer, caps.szPname);
          return PluginLID_NoError;
        }
      }

      return PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG1(Open,const char *,device)
    {
      Close();

      switch (m_eProductID)
      {
        case TJIP_TJ560BHANDSET_KEYPAD_HID :
	  WriteRegister(0x0b, 100);	// default is 48, now set to 100 ==> period = 100/2 = 50ms

          // Initialise beeper
          WriteRegister(0xa, 0x55);       // set GIO[3:0] all output as 0b0000
          WriteRegister(0x1a, 3);         // set GIO[8] to high

          BYTE value;
          ReadRegister(0x1a, &value);      // read back
          m_hasBeeper = (value & 0x2) == 0x2;

          WriteRegister(0x1a, m_hasBeeper ? 1 : 0);         // GIO8 as Beeper control output 0
          WriteRegister(0xa, 0);     // Restore GIO[3:0] setting

	  BYTE reg12, reg13;
	  ReadRegister(0x12, &reg12);
	  WriteRegister(0x12, reg12 & 0xdf);    // set AUX5 to low
	  ReadRegister(0x13, &reg13);
	  WriteRegister(0x13, reg13 | 0x20);    // set AUX5 to output
	  WriteRegister(0x12, reg12);           // restore original AUX data value
	  WriteRegister(0x13, reg13);           // restore original AUX control

	  WriteRegister(0xc, 0);
	  WriteRegister(0xd, 0);
	  WriteRegister(0xe, 0);
          break;
      }

      m_hasDTMF = m_pTjIpSysCall(TJIP_INIT_DTMF_TO_PPG, 0, (void *)device) != 0;

      TJ_MIXER_OPEN tjMixerOpen;
      memset(&tjMixerOpen, 0, sizeof(tjMixerOpen));
      strncpy(tjMixerOpen.szMixerName, device, sizeof(tjMixerOpen.szMixerName)-1);
      m_hasMixer = m_pTjIpSysCall(TJIP_OPEN_MIXER, 0, &tjMixerOpen) != 0;

      return PluginLID_UsesSoundChannel;
    }


    PLUGIN_FUNCTION_ARG0(Close)
    {
      RingLine(0, 0, NULL, 0);

      if (m_hasMixer) {
        if (!m_pTjIpSysCall(TJIP_CLOSE_MIXER, 0, NULL))
          OutputDebugString("Could not close mixer on TigerJet\n");
        m_hasMixer = false;
      }

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(GetLineCount, unsigned *,count)
    {
      if (count == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      *count = 1;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineTerminal, unsigned,line, PluginLID_Boolean *,isTerminal)
    {
      if (isTerminal == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      *isTerminal = true;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(IsLinePresent, unsigned,line, PluginLID_Boolean,forceTest, PluginLID_Boolean *,present)
    {
      if (present == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      *present = m_pTjIpSysCall(TJIP_CHECK_IS_TJ560DEVICE_PLUGGED, 0, NULL);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineOffHook, unsigned,line, PluginLID_Boolean *,offHook)
    {
      if (offHook == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      *offHook = m_isOffHook;
      return PluginLID_NoError;
    }


    //PLUGIN_FUNCTION_ARG2(SetLineOffHook, unsigned,line, PluginLID_Boolean,newState)
    //PLUGIN_FUNCTION_ARG2(HookFlash, unsigned,line, unsigned,flashTime)
    //PLUGIN_FUNCTION_ARG2(HasHookFlash, unsigned,line, PluginLID_Boolean *,flashed)
    //PLUGIN_FUNCTION_ARG2(IsLineRinging, unsigned,line, unsigned long *,cadence)

    static void CadenceThreadMain(void * arg)
    {
      ((Context *)arg)->CadenceThread();
    }

    void CadenceThread()
    {
      size_t cadenceIndex = 0;

      while (m_ringCadence.size() > 0 && WaitForSingleObject(m_hRingStopEvent, m_ringCadence[cadenceIndex]) == WAIT_TIMEOUT) {
        m_KeyMutex.Enter();
        if (++cadenceIndex >= m_ringCadence.size())
          cadenceIndex = 0;
        Beep((cadenceIndex&1) != 0 ? 0 : m_ringFrequency);
        m_KeyMutex.Leave();
      }
    }

    PLUGIN_FUNCTION_ARG4(RingLine, unsigned,line, unsigned,nCadence, const unsigned *,pattern, unsigned,frequency)
    {
      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (m_hRingThread != NULL) {
        SetEvent(m_hRingStopEvent);
        if (WaitForSingleObject(m_hRingThread, 5000) == WAIT_TIMEOUT)
          TerminateThread(m_hRingThread, -1);

        CloseHandle(m_hRingStopEvent);
        m_hRingStopEvent = NULL;
        m_hRingThread = NULL;
      }

      m_KeyMutex.Enter();

      m_ringCadence.assign(pattern, &pattern[nCadence]);
      m_ringFrequency = nCadence == 0 ? 0 : frequency;
      Beep(m_ringFrequency);

      if (nCadence > 1) {
        m_hRingStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hRingThread = (HANDLE)_beginthread(CadenceThreadMain, 0, this);
      }

      m_KeyMutex.Leave();

      return PluginLID_NoError;
    }

    //PLUGIN_FUNCTION_ARG3(IsLineDisconnected, unsigned,line, PluginLID_Boolean,checkForWink, PluginLID_Boolean *,disconnected)
    //PLUGIN_FUNCTION_ARG3(SetLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean,connect)
    //PLUGIN_FUNCTION_ARG3(IsLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean *,connected)
    //PLUGIN_FUNCTION_ARG3(GetSupportedFormat, unsigned,index, char *,mediaFormat, unsigned,size)
    //PLUGIN_FUNCTION_ARG2(SetReadFormat, unsigned,line, const char *,mediaFormat)
    //PLUGIN_FUNCTION_ARG2(SetWriteFormat, unsigned,line, const char *,mediaFormat)
    //PLUGIN_FUNCTION_ARG3(GetReadFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    //PLUGIN_FUNCTION_ARG3(GetWriteFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    //PLUGIN_FUNCTION_ARG1(StopReading, unsigned,line)
    //PLUGIN_FUNCTION_ARG1(StopWriting, unsigned,line)
    //PLUGIN_FUNCTION_ARG2(SetReadFrameSize, unsigned,line, unsigned,frameSize)
    //PLUGIN_FUNCTION_ARG2(SetWriteFrameSize, unsigned,line, unsigned,frameSize)
    //PLUGIN_FUNCTION_ARG2(GetReadFrameSize, unsigned,line, unsigned *,frameSize)
    //PLUGIN_FUNCTION_ARG2(GetWriteFrameSize, unsigned,line, unsigned *,frameSize)
    //PLUGIN_FUNCTION_ARG3(ReadFrame, unsigned,line, void *,buffer, unsigned *,count)
    //PLUGIN_FUNCTION_ARG4(WriteFrame, unsigned,line, const void *,buffer, unsigned,count, unsigned *,written)
    //PLUGIN_FUNCTION_ARG3(GetAverageSignalLevel, unsigned,line, PluginLID_Boolean,playback, unsigned *,signal)
    //PLUGIN_FUNCTION_ARG2(EnableAudio, unsigned,line, PluginLID_Boolean,enable)
    //PLUGIN_FUNCTION_ARG2(IsAudioEnabled, unsigned,line, PluginLID_Boolean *,enable)


    PLUGIN_FUNCTION_ARG2(SetRecordVolume, unsigned,line, unsigned,volume)
    {
      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (!m_hasMixer)
        return PluginLID_UnimplementedFunction;

      DWORD dwVolume = volume*32768/100;
      if (dwVolume > 32767)
        dwVolume = 32767;
      return m_pTjIpSysCall(TJIP_SET_WAVEIN_VOL, 0, (void *)dwVolume) ? PluginLID_NoError : PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG2(SetPlayVolume, unsigned,line, unsigned,volume)
    {
      if (volume > 100)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (!m_hasMixer)
        return PluginLID_UnimplementedFunction;

      DWORD dwVolume = volume*32768/100;
      if (dwVolume > 32767)
        dwVolume = 32767;
      return m_pTjIpSysCall(TJIP_SET_WAVEOUT_VOL, 0, (void *)dwVolume) ? PluginLID_NoError : PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG2(GetRecordVolume, unsigned,line, unsigned *,volume)
    {
      if (volume == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (!m_hasMixer)
        return PluginLID_UnimplementedFunction;

      DWORD dwVolume;
      if (!m_pTjIpSysCall(TJIP_GET_WAVEIN_VOL, 0, &dwVolume))
        return PluginLID_InternalError;

      *volume = dwVolume*100/32768;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetPlayVolume, unsigned,line, unsigned *,volume)
    {
      if (volume == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (!m_hasMixer)
        return PluginLID_UnimplementedFunction;

      DWORD dwVolume;
      if (!m_pTjIpSysCall(TJIP_GET_WAVEOUT_VOL, 0, &dwVolume))
        return PluginLID_InternalError;

      *volume = dwVolume*100/32768;
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

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (!m_hasDTMF)
        return PluginLID_UnimplementedFunction;

      while (*digits != '\0') {
        int nTone = 0;
        if (isdigit(*digits))
          nTone = *digits - '0';
        else if (*digits == '*')
          nTone = 10;
        else if (*digits == '#')
          nTone = 11;

        if (nTone > 0) {
          if (!m_pTjIpSysCall(TJIP_SEND_DTMF_TO_PPG, nTone, NULL))
            return PluginLID_InternalError;
        }
        digits++;
      }
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(ReadDTMF, unsigned,line, char *,digit)
    {
      if (digit == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      m_KeyMutex.Enter();

      if (m_Keys.size() == 0)
        *digit = '\0';
      else {
        *digit = m_Keys.front();
        m_Keys.pop();
      }

      m_KeyMutex.Leave();

      return PluginLID_NoError;
    }


    //PLUGIN_FUNCTION_ARG2(GetRemoveDTMF, unsigned,line, PluginLID_Boolean *,removeTones)
    //PLUGIN_FUNCTION_ARG2(SetRemoveDTMF, unsigned,line, PluginLID_Boolean,removeTones)
    //PLUGIN_FUNCTION_ARG2(IsToneDetected, unsigned,line, int *,tone)
    //PLUGIN_FUNCTION_ARG3(WaitForToneDetect, unsigned,line, unsigned,timeout, int *,tone)
    //PLUGIN_FUNCTION_ARG3(WaitForTone, unsigned,line, int,tone, unsigned,timeout)
    //PLUGIN_FUNCTION_ARG7(SetToneFilterParameters, unsigned        ,line,
    //                                              unsigned        ,tone,
    //                                              unsigned        ,lowFrequency,
    //                                              unsigned        ,highFrequency,
    //                                              unsigned        ,numCadences,
    //                                              const unsigned *,onTimes,
    //                                              const unsigned *,offTimes)


    PLUGIN_FUNCTION_ARG2(PlayTone, unsigned,line, unsigned,tone)
    {
      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (!m_hasDTMF)
        return PluginLID_UnimplementedFunction;

      int nTone = 0;
      switch (tone) {
        case PluginLID_RingTone :
          nTone = 101;
          break;
        case PluginLID_BusyTone :
          nTone = 102;
          break;
        case PluginLID_DialTone :
          nTone = 201;
          break;
        case PluginLID_FastBusyTone :
          nTone = 202;
          break;
        case PluginLID_ClearTone :
          nTone = 203;
          break;
      }

      if (nTone > 0) {
        if (!m_pTjIpSysCall(TJIP_SEND_DTMF_TO_PPG, nTone, NULL))
          return PluginLID_InternalError;
        m_tonePlaying = true;
      }
      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG2(IsTonePlaying, unsigned,line, PluginLID_Boolean *,playing)
    {
      if (playing == NULL)
        return PluginLID_InvalidParameter;

      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (!m_hasDTMF)
        return PluginLID_UnimplementedFunction;

      *playing = m_tonePlaying;
      return PluginLID_NoError;
    }

    PLUGIN_FUNCTION_ARG1(StopTone, unsigned,line)
    {
      if (m_eProductID == TJIP_NONE)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (!m_hasDTMF)
        return PluginLID_UnimplementedFunction;

      m_tonePlaying = false;
      return m_pTjIpSysCall(TJIP_SEND_DTMF_TO_PPG, -1, NULL) ? PluginLID_NoError : PluginLID_InternalError;
    }

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

    "TigerJet",                       // LID name text
    "TigerJet Network Tiger560",      // LID description text
    "TigerJet Network, Inc",          // LID manufacturer name
    "Tiger560",                       // LID model name
    "1.0",                            // LID hardware revision number
    "info@tjnet.com",                 // LID email contact information
    "http://www.tjnet,com",           // LID web site

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
    NULL,//Context::SetLineOffHook,
    NULL,//Context::HookFlash,
    NULL,//Context::HasHookFlash,
    NULL,//Context::IsLineRinging,
    Context::RingLine,
    NULL,//Context::IsLineDisconnected,
    NULL,//Context::SetLineToLineDirect,
    NULL,//Context::IsLineToLineDirect,
    NULL,//Context::GetSupportedFormat,
    NULL,//Context::SetReadFormat,
    NULL,//Context::SetWriteFormat,
    NULL,//Context::GetReadFormat,
    NULL,//Context::GetWriteFormat,
    NULL,//Context::StopReading,
    NULL,//Context::StopWriting,
    NULL,//Context::SetReadFrameSize,
    NULL,//Context::SetWriteFrameSize,
    NULL,//Context::GetReadFrameSize,
    NULL,//Context::GetWriteFrameSize,
    NULL,//Context::ReadFrame,
    NULL,//Context::WriteFrame,
    NULL,//Context::GetAverageSignalLevel,
    NULL,//Context::EnableAudio,
    NULL,//Context::IsAudioEnabled,
    NULL,//Context::SetRecordVolume,
    NULL,//Context::SetPlayVolume,
    NULL,//Context::GetRecordVolume,
    NULL,//Context::GetPlayVolume,
    NULL,//Context::GetAEC,
    NULL,//Context::SetAEC,
    NULL,//Context::GetVAD,
    NULL,//Context::SetVAD,
    NULL,//Context::GetCallerID,
    NULL,//Context::SetCallerID,
    NULL,//Context::SendVisualMessageWaitingIndicator,
    Context::PlayDTMF,
    Context::ReadDTMF,
    NULL,//Context::GetRemoveDTMF,
    NULL,//Context::SetRemoveDTMF,
    NULL,//Context::IsToneDetected,
    NULL,//Context::WaitForToneDetect,
    NULL,//Context::WaitForTone,
    NULL,//Context::SetToneFilterParameters,
    Context::PlayTone,
    Context::IsTonePlaying,
    Context::StopTone,
    NULL,//Context::DialOut,
    NULL,//Context::GetWinkDuration,
    NULL,//Context::SetWinkDuration,
    NULL,//Context::SetCountryCode,
    NULL,//Context::GetSupportedCountry
  }
};


PLUGIN_LID_IMPLEMENTATION(definition);


/////////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpArg)
{
  if (fdwReason == DLL_PROCESS_ATTACH) {
    g_hInstance = hinstDLL;
  }
  return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
