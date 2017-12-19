/*
 * Generic USB HID Plugin LID for OPAL
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
 * $Revision: 20659 $
 * $Author: rjongbloed $
 * $Date: 2008-08-13 01:00:29 +0000 (Wed, 13 Aug 2008) $
 */

#define _CRT_SECURE_NO_DEPRECATE
#define _WIN32_WINNT 0x0501
#include <windows.h>

#define PLUGIN_DLL_EXPORTS
#include <lids/lidplugin.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <process.h>
#include <queue>
#include <vector>

#include "winxp_hid.h"


static HINSTANCE g_hInstance;

char g_IniFile[MAX_PATH];
char DescriptionText[100] = "Generic USB HID using winXP Raw Input API";


/////////////////////////////////////////////////////////////////////////////

class Context
{
  protected:
    HANDLE m_hThread;
    DWORD  m_threadId;
    HANDLE m_hStartedEvent;
    HWND   m_hWnd;

    WORD   m_manufacturerId;
    WORD   m_productId;

    std::queue<char> m_queue;
    CRITICAL_SECTION m_mutex;

    bool   m_isOffHook;

    std::vector<BYTE> m_keymap[NumKeys];
    std::vector<BYTE> m_keymask[NumKeys];

  public:
    PLUGIN_LID_CTOR()
    {
      m_hThread= NULL;
      m_threadId = 0;
      m_hStartedEvent = NULL;
      m_hWnd = NULL;

      m_manufacturerId = 0;
      m_productId = 0;

      InitializeCriticalSection(&m_mutex);

      m_isOffHook = false;
    }

    PLUGIN_LID_DTOR()
    {
      Close();
      DeleteCriticalSection(&m_mutex);
    }


    PLUGIN_FUNCTION_ARG3(GetDeviceName,unsigned,index, char *,buffer, unsigned,bufsize)
    {
      if (buffer == NULL || bufsize < 4)
        return PluginLID_InvalidParameter;

      if (index >= 1)
        return PluginLID_NoMoreNames;

      UINT deviceCount = 0;
      if (GetRawInputDeviceList(NULL, &deviceCount, sizeof(RAWINPUTDEVICELIST)) != 0 || deviceCount == 0)
        return PluginLID_NoMoreNames;

      RAWINPUTDEVICELIST * rawDevices = (RAWINPUTDEVICELIST *)calloc(deviceCount, sizeof(RAWINPUTDEVICELIST));
      if (GetRawInputDeviceList(rawDevices, &deviceCount, sizeof(RAWINPUTDEVICELIST)) > 0) {
        for (UINT i = 0; i < deviceCount; i++) {
          if (rawDevices[i].dwType == RIM_TYPEHID) {
            RID_DEVICE_INFO info;
            UINT size = info.cbSize = sizeof(info);
            if ((int)GetRawInputDeviceInfo(rawDevices[i].hDevice, RIDI_DEVICEINFO, &info, &size) > 0 &&
                        (info.hid.usUsagePage == 11 ||  // Telephony device
                         info.hid.usUsagePage == 12))  // Some brain dead handsets say "Consumer Device"
            {
              char name[100];
              _snprintf(name, sizeof(name), SectionFmt, info.hid.dwVendorId&0xffff, info.hid.dwProductId&0xffff);

              if (GetPrivateProfileString(name, Description, NULL, DescriptionText, sizeof(DescriptionText), g_IniFile) > 0) {
                char audio[30];
                GetPrivateProfileString(name, AudioDevice, "USB", audio, sizeof(audio), g_IniFile);

                UINT numDevs = waveOutGetNumDevs();
                for (UINT i = 0; i < numDevs; i++) {
                  WAVEOUTCAPS caps;
                  waveOutGetDevCaps(i, &caps, sizeof(caps));
                  if (strstr(caps.szPname, audio) != NULL) {
                    free(rawDevices);

                    strcat(name, DevSeperatorStr "WindowsMultimedia:");
                    strcat(name, caps.szPname);

                    if (bufsize <= strlen(name))
                      return PluginLID_BufferTooSmall;

                    strcpy(buffer, name);
                    return PluginLID_NoError;
                  }
                }
              }
            }
          }
        }
      }

      free(rawDevices);
      return PluginLID_NoMoreNames;
    }


    PLUGIN_FUNCTION_ARG1(Open,const char *,device)
    {
      Close();

      char * slash;
      m_manufacturerId = (WORD)strtoul(device, &slash, 16);
      if (*slash != IdSeperatorChar)
        return PluginLID_NoSuchDevice;

      char * backslash;
      m_productId = (WORD)strtoul(slash+1, &backslash, 16);
      if (*backslash != DevSeperatorChar)
        return PluginLID_NoSuchDevice;

      char name[20];
      _snprintf(name, sizeof(name), SectionFmt, m_manufacturerId, m_productId);

      if (GetPrivateProfileString(name, Description, NULL, DescriptionText, sizeof(DescriptionText), g_IniFile) <= 0)
        return PluginLID_NoSuchDevice;

      for (int i = 0; i < NumKeys; i++) {
        char item[2];
        item[0] = Keys[i].code;
        item[1] = '\0';
        char hexData[100];
        int len = GetPrivateProfileString(name, item, NULL, hexData, sizeof(hexData), g_IniFile)/2;
        m_keymap[i].resize(len);
        m_keymask[i].resize(len);
        for (int j = 0; j < len; j++) {
          char hexByte[3];
          hexByte[0] = hexData[j*2];
          hexByte[1] = hexData[j*2+1];
          hexByte[2] = '\0';
          if (strcmp(hexByte, "xx") == 0) {
            m_keymap[i][j] = 0;
            m_keymask[i][j] = 0;
          }
          else {
            m_keymap[i][j] = (BYTE)strtoul(hexByte, NULL, 16);
            m_keymask[i][j] = 0xff;
          }
        }
      }

      m_hStartedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
      if (m_hStartedEvent == NULL)
        return PluginLID_DeviceOpenFailed;

      m_hThread = (HANDLE)_beginthread(ThreadMain, 0, this);
      if (m_hThread == NULL)
        return PluginLID_DeviceOpenFailed;

      if (WaitForSingleObject(m_hStartedEvent, 5000) == WAIT_OBJECT_0 && m_hWnd != NULL)
        return PluginLID_UsesSoundChannel;

      Close();
      return PluginLID_DeviceOpenFailed;
    }


    PLUGIN_FUNCTION_ARG0(Close)
    {
      if (m_hThread != NULL) {
        if (m_hWnd != NULL)
          SendMessage(m_hWnd, WM_CLOSE, 0, 0);
        if (WaitForSingleObject(m_hThread, 5000) == WAIT_TIMEOUT)
          TerminateThread(m_hThread, -1);
        m_hThread = NULL;
      }

      if (m_hStartedEvent != NULL) {
        CloseHandle(m_hStartedEvent);
        m_hStartedEvent = NULL;
      }

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(GetLineCount, unsigned *,count)
    {
      if (count == NULL)
        return PluginLID_InvalidParameter;

      if (m_hWnd == 0)
        return PluginLID_DeviceNotOpen;

      *count = 1;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineTerminal, unsigned,line, PluginLID_Boolean *,isTerminal)
    {
      if (isTerminal == NULL)
        return PluginLID_InvalidParameter;

      if (m_hWnd == 0)
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

      if (m_hWnd == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      *present = true;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineOffHook, unsigned,line, PluginLID_Boolean *,offHook)
    {
      if (offHook == NULL)
        return PluginLID_InvalidParameter;

      if (m_hWnd == 0)
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
    //PLUGIN_FUNCTION_ARG4(RingLine, unsigned,line, unsigned,nCadence, const unsigned *,pattern, unsigned,frequency)
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
    //PLUGIN_FUNCTION_ARG2(SetRecordVolume, unsigned,line, unsigned,volume)
    //PLUGIN_FUNCTION_ARG2(SetPlayVolume, unsigned,line, unsigned,volume)
    //PLUGIN_FUNCTION_ARG2(GetRecordVolume, unsigned,line, unsigned *,volume)
    //PLUGIN_FUNCTION_ARG2(GetPlayVolume, unsigned,line, unsigned *,volume)
    //PLUGIN_FUNCTION_ARG2(GetAEC, unsigned,line, unsigned *,level)
    //PLUGIN_FUNCTION_ARG2(SetAEC, unsigned,line, unsigned,level)
    //PLUGIN_FUNCTION_ARG2(GetVAD, unsigned,line, PluginLID_Boolean *,enable)
    //PLUGIN_FUNCTION_ARG2(SetVAD, unsigned,line, PluginLID_Boolean,enable)
    //PLUGIN_FUNCTION_ARG4(GetCallerID, unsigned,line, char *,idString, unsigned,size, PluginLID_Boolean,full)
    //PLUGIN_FUNCTION_ARG2(SetCallerID, unsigned,line, const char *,idString)
    //PLUGIN_FUNCTION_ARG2(SendVisualMessageWaitingIndicator, unsigned,line, PluginLID_Boolean,on)
    //PLUGIN_FUNCTION_ARG4(PlayDTMF, unsigned,line, const char *,digits, unsigned,onTime, unsigned,offTime)


    PLUGIN_FUNCTION_ARG2(ReadDTMF, unsigned,line, char *,digit)
    {
      if (digit == NULL)
        return PluginLID_InvalidParameter;

      if (m_manufacturerId == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      EnterCriticalSection(&m_mutex);

      if (m_queue.empty())
        *digit = '\0';
      else {
        *digit = m_queue.front();
        m_queue.pop();
      }

      LeaveCriticalSection(&m_mutex);

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
    //PLUGIN_FUNCTION_ARG2(PlayTone, unsigned,line, unsigned,tone)
    //PLUGIN_FUNCTION_ARG2(IsTonePlaying, unsigned,line, PluginLID_Boolean *,playing)
    //PLUGIN_FUNCTION_ARG1(StopTone, unsigned,line)
    //PLUGIN_FUNCTION_ARG4(DialOut, unsigned,line, const char *,number, PluginLID_Boolean,requireTones, unsigned,uiDialDelay)
    //PLUGIN_FUNCTION_ARG2(GetWinkDuration, unsigned,line, unsigned *,winkDuration)
    //PLUGIN_FUNCTION_ARG2(SetWinkDuration, unsigned,line, unsigned,winkDuration)
    //PLUGIN_FUNCTION_ARG1(SetCountryCode, unsigned,country)
    //PLUGIN_FUNCTION_ARG2(GetSupportedCountry, unsigned,index, unsigned *,countryCode)

  protected:
    static void ThreadMain(void * arg)
    {
      ((Context *)arg)->Main();
    }

    static LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
      Context * context = (Context *)GetWindowLong(hWnd, 0);
      if (context != NULL)
        context->HandleMsg(message, wParam, lParam);
      return DefWindowProc(hWnd, message, wParam, lParam);
    }

    void Main()
    {
      m_threadId = GetCurrentThreadId();

      static const char WndClassName[] = "WinXP-HID-LID";	 
      static bool registered = false;
      if (!registered) {
        // Register the main window class. 
        WNDCLASS wc;
        wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = (WNDPROC)WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = sizeof(this);
        wc.hInstance = g_hInstance;
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszMenuName =  NULL;
        wc.lpszClassName = WndClassName;

        registered = RegisterClass(&wc) != 0;
      }

      m_hWnd = CreateWindowEx(0, WndClassName, WndClassName,
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 200, 150,
                              0, 0, g_hInstance, 0);
      if (m_hWnd == NULL) {
        SetEvent(m_hStartedEvent);
        return;
      }

      SetWindowLong(m_hWnd, 0, (LONG)this);

      if (!Register(true)) {
        DestroyWindow(m_hWnd);
        m_hWnd = NULL;
        SetEvent(m_hStartedEvent);
        return;
      }

      SetEvent(m_hStartedEvent);

      MSG msg;
      for (;;) {
        switch (GetMessage(&msg, NULL, 0, 0)) {
          case -1 :
          case 0 :
            return;
          default :
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
      }
    }

    bool Register(bool add)
    {
      RAWINPUTDEVICE device[2];
      device[0].dwFlags = add ? (RIDEV_PAGEONLY|RIDEV_INPUTSINK) : RIDEV_REMOVE;
      device[0].usUsagePage = 11; // Telephony
      device[0].usUsage = 0;      // Everything on page
      device[0].hwndTarget = m_hWnd;
      device[1] = device[0];
      device[1].usUsagePage = 12; // Some brain dead handsets say "Consumer device"
      if (RegisterRawInputDevices(device, sizeof(device)/sizeof(device[0]), sizeof(device[0])) != 0)
        return true;

      char str[200];
      _snprintf(str, sizeof(str), "WinXP-HID-LID: RegisterRawInputDevices error %u\r\n", GetLastError());
      OutputDebugString(str);
      return false;
    }

    void HandleRawInput(HRAWINPUT hRaw)
    {
      UINT size;
      GetRawInputData(hRaw, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));

      RAWINPUT * raw = (RAWINPUT *)malloc(size);
      if (raw == NULL) 
        return;

      if (GetRawInputData(hRaw, RID_INPUT, raw, &size, sizeof(RAWINPUTHEADER)) != size)
        OutputDebugString("WinXP-HID-LID: GetRawInputData doesn't return correct size!\n");
      else if (raw->header.dwType != RIM_TYPEHID)
        OutputDebugString("WinXP-HID-LID: Raw input from unexpected device type!\n");
      else {
        RID_DEVICE_INFO info;
        UINT size = info.cbSize = sizeof(info);
        if ((int)GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICEINFO, &info, &size) < 0)
          OutputDebugString("WinXP-HID-LID: Could not get info on HID device!\n");
        else if (m_manufacturerId == info.hid.dwVendorId && m_productId == info.hid.dwProductId) {
          BYTE * data = raw->data.hid.bRawData;
          for (DWORD i = 0; i < raw->data.hid.dwCount; i++) {
            EnterCriticalSection(&m_mutex);
#if 0
            char buffer[100];
            strcpy(buffer, "WinXP-HID-LID: ";
            for (DWORD b =0; b < raw->data.hid.dwSizeHid; b++)
              sprintf(&buffer[strlen(buffer)], "%02X ", data[b]);
            strcat(buffer, "\n");
            OutputDebugString(buffer);
#endif
            for (int c = 0; c < NumKeys; c++) {
              if (m_keymap[c].size() > 0 && m_keymap[c].size() <= raw->data.hid.dwSizeHid) {
                bool matched = true;
                for (size_t j = 0; j < m_keymap[c].size(); j++) {
                  if ((data[j] & m_keymask[c][j]) != m_keymap[c][j]) {
                    matched = false;
                    break;
                  }
                }
                if (matched) {
                  switch (Keys[c].code) {
                    case 'S' : // Start
                      m_isOffHook = true;
                      break;
                    case 'E' : // End
                      m_isOffHook = false;
                      break;
                    default :
                      m_queue.push(Keys[c].code);
                  }
                  break;
                }
              }
            }
            LeaveCriticalSection(&m_mutex);
            data += raw->data.hid.dwSizeHid;
          }
        }
      }

      free(raw);
    }

    void HandleMsg(UINT message, WPARAM wParam, LPARAM lParam)
    {
      switch (message) {
        case WM_INPUT:
          HandleRawInput((HRAWINPUT)lParam);
          break;

        //case WM_INPUT_DEVICE_CHANGE :
        //  break;

        case WM_CLOSE :
          Register(false);
          DestroyWindow(m_hWnd);
          break;

        case WM_DESTROY :
          PostQuitMessage(0);
          break;
      }
    }
};


/////////////////////////////////////////////////////////////////////////////

static struct PluginLID_Definition definition[1] =
{

  { 
    // encoder
    PLUGIN_LID_VERSION,               // API version

    1083666706,                       // timestamp = Tue 04 May 2004 10:31:46 AM UTC = 

    "WinXP-HID",                      // LID name text
    DescriptionText,                  // LID description text
    "Anyone",                         // LID manufacturer name
    "WinXP-HID",                      // LID model name
    "1.0",                            // LID hardware revision number
    "",                               // LID email contact information
    "",                               // LID web site

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
    NULL,//Context::RingLine,
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
    NULL,//Context::PlayDTMF,
    Context::ReadDTMF,
    NULL,//Context::GetRemoveDTMF,
    NULL,//Context::SetRemoveDTMF,
    NULL,//Context::IsToneDetected,
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


BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID)
{
  if (fdwReason == DLL_PROCESS_ATTACH)
    g_hInstance = hInstDLL;

  GetModuleFileName(hInstDLL, g_IniFile, sizeof(g_IniFile)-4);
  char * dll = strstr(g_IniFile, ".dll");
  if (dll != NULL)
    strcpy(dll, ".ini");
  else
    strcat(g_IniFile, ".ini");

  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
