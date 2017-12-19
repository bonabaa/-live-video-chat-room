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
 * Based on code by Simon Horne of ISVO (Asia) Pte Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#define _CRT_SECURE_NO_DEPRECATE
#include <windows.h>

#define PLUGIN_DLL_EXPORTS
#include <lids/lidplugin.h>

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <process.h>
#include <queue>

#include "CM_HID.h"


// SubClass Values
#define WM_HID_DEV_ADDED      WM_USER+0x1000
#define WM_HID_DEV_REMOVED    WM_USER+0x1001
#define WM_HID_KEY_DOWN       WM_USER+0x1002
#define WM_HID_KEY_UP         WM_USER+0x1003
#define WM_HID_VOLUME_DOWN    WM_USER+0x1004
#define WM_HID_VOLUME_UP      WM_USER+0x1005
#define WM_HID_PLAYBACK_MUTE  WM_USER+0x1006
#define WM_HID_RECORD_MUTE    WM_USER+0x1007
#define WM_HID_SHUTDOWN       WM_USER+0x1008


static HINSTANCE g_hInstance;


/////////////////////////////////////////////////////////////////////////////

class Context
{
  protected:
    HANDLE m_hThread;
    DWORD  m_threadId;
    HANDLE m_hStartedEvent;
    HWND   m_hWnd;

    bool   m_pluggedIn;
    bool   m_isOffHook;

    std::queue<char> m_queue;
    CRITICAL_SECTION m_mutex;

  public:
    PLUGIN_LID_CTOR()
    {
      m_hThread= NULL;
      m_threadId = 0;
      m_hStartedEvent = NULL;
      m_hWnd = NULL;

      m_pluggedIn = false;
      m_isOffHook = false;

      InitializeCriticalSection(&m_mutex);
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

      if (StartDeviceDetection(NULL, 0, 0, 0, 0, 0, 0, 0, 0) == 0)
        return PluginLID_NoMoreNames;

      CloseDevice();

      UINT numDevs = waveOutGetNumDevs();
      for (UINT i = 0; i < numDevs; i++) {
        WAVEOUTCAPS caps;
	waveOutGetDevCaps(i, &caps, sizeof(caps));
        if (strstr(caps.szPname, "USB Audio") != NULL) {
          if (bufsize <= strlen(caps.szPname))
            return PluginLID_BufferTooSmall;

          int pos = strlen(caps.szPname)-1;
          while (caps.szPname[pos] == ' ')
            caps.szPname[pos--] = '\0';
          strcpy(buffer, caps.szPname);
          return PluginLID_NoError;
        }
      }

      return PluginLID_NoMoreNames;
    }


    PLUGIN_FUNCTION_ARG1(Open,const char *,device)
    {
      Close();

      m_hStartedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
      if (m_hStartedEvent == NULL)
        return PluginLID_DeviceOpenFailed;

      m_hThread = (HANDLE)_beginthread(ThreadMain, 0, this);
      if (m_hThread == NULL)
        return PluginLID_DeviceOpenFailed;

      if (WaitForSingleObject(m_hStartedEvent, 5000) == WAIT_OBJECT_0)
        return PluginLID_UsesSoundChannel;

      Close();
      return PluginLID_DeviceOpenFailed;
    }


    PLUGIN_FUNCTION_ARG0(Close)
    {
      if (m_hThread != NULL) {
        SendMessage(m_hWnd, WM_HID_SHUTDOWN, 0, 0);
        if (WaitForSingleObject(m_hThread, 5000) == WAIT_TIMEOUT)
          TerminateThread(m_hThread, -1);
      }

      if (m_hStartedEvent != NULL)
        CloseHandle(m_hStartedEvent);

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

      *present = m_pluggedIn;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineOffHook, unsigned,line, PluginLID_Boolean *,offHook)
    {
      if (offHook == NULL)
        return PluginLID_InvalidParameter;

      if (m_hWnd == NULL)
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

    PLUGIN_FUNCTION_ARG4(RingLine, unsigned,line, unsigned,nCadence, const unsigned *,pattern, unsigned,frequency)
    {
      if (m_hWnd == NULL)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      if (nCadence > 0)
        StartBuzzer();
      else
        StopBuzzer();
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

      if (m_hWnd == NULL)
        return PluginLID_DeviceNotOpen;

      if (line >= 1)
        return PluginLID_NoSuchLine;

      EnterCriticalSection(&m_mutex);

      if (m_queue.size() == 0)
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

      static const char WndClassName[] = "CM_HID";	 
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
      while (!m_queue.empty())
        m_queue.pop();

      int result = StartDeviceDetection(m_hWnd,
                                        WM_HID_DEV_ADDED,
                                        WM_HID_DEV_REMOVED,
                                        WM_HID_KEY_DOWN,
                                        WM_HID_KEY_UP,
                                        WM_HID_VOLUME_DOWN,
                                        WM_HID_VOLUME_UP,
                                        WM_HID_PLAYBACK_MUTE,
                                        WM_HID_RECORD_MUTE);

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

    void Enqueue(char digit)
    {
      EnterCriticalSection(&m_mutex);
      m_queue.push(digit);
      LeaveCriticalSection(&m_mutex);
    }

    void HandleMsg(UINT message, WPARAM wParam, LPARAM lParam)
    {
      switch (message) {
        case WM_HID_DEV_ADDED :
          m_pluggedIn = true;
          StartKeyScan();
          break;

        case WM_HID_DEV_REMOVED :
          m_pluggedIn = false;
          StopKeyScan();
          break;

        case WM_HID_KEY_DOWN: 
          switch (wParam) {
            case 1:
              Enqueue('1');
              break;
            case 2:
              Enqueue('4');
              break;
            case 3:
              Enqueue('7');
              break;
            case 4:
              Enqueue('*');
              break;
            case 5:
              Enqueue('2');
              break;
            case 6:
              Enqueue('5');
              break;
            case 7:
              Enqueue('8');
              break;
            case 8:
              Enqueue('0');
              break;
            case 9:
              Enqueue('3');
              break;
            case 10:
              Enqueue('6');
              break;
            case 11:
              Enqueue('9');
              break;
            case 12:
              Enqueue('#');
              break;
            case 13:
              m_isOffHook = true;  // Dial key
              break;
            case 14:
              m_isOffHook = false;;  // Stop Dial key
              break;
            case 15:
              Enqueue('l');  // left
              break;
            case 16:
              Enqueue('r');  // right
          }
          break;

        case WM_HID_KEY_UP: 
          // Do Nothing
          break;

        case WM_HID_VOLUME_DOWN: 
          if ((int)wParam == 1)
	    Enqueue('u');
          else
	    Enqueue('d');
          break;

        case WM_HID_VOLUME_UP: 
          // Do Nothing
          break;

        case WM_HID_PLAYBACK_MUTE: 
          break;

        case WM_HID_RECORD_MUTE: 
	  Enqueue('m');
          break;

        case WM_DEVICECHANGE: 
          HandleUsbDeviceChange(wParam, lParam); 
          break;

        case WM_HID_SHUTDOWN :
          CloseDevice();
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

    "CM-HID",                         // LID name text
    "USB handset for CM phones",      // LID description text
    "CM",                             // LID manufacturer name
    "CM-HID",                         // LID model name
    "1.0",                            // LID hardware revision number
    "",                               // LID email contact information
    "",                               // LID web site

    "Robert Jongbloed, Vox Lucida",                              // source code author
    "robertj@voxlucida.com.au",                                  // source code email
    "http://www.voxgratia.org",                                  // source code URL
    "Copyright (C) 2006-2008 by Vox Lucida, All Rights Reserved",// source code copyright
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


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
  if (fdwReason == DLL_PROCESS_ATTACH)
    g_hInstance = hinstDLL;
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
