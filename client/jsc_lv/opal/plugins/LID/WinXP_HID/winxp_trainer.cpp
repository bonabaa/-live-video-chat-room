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
 * $Revision: 19258 $
 * $Author: rjongbloed $
 * $Date: 2008-01-15 07:29:51 +0000 (Tue, 15 Jan 2008) $
 */

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <iomanip>
#include <vector>
#include <assert.h>

using namespace std;

#define _WIN32_WINNT 0x0501
#include <windows.h>

#include "winxp_hid.h"


HINSTANCE g_hInstance;
RID_DEVICE_INFO g_device;
std::vector<BYTE> g_keymap[NumKeys];
size_t g_CurrentKey;


void HandleRawInput(HRAWINPUT hRaw)
{
  UINT size;
  GetRawInputData(hRaw, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));

  RAWINPUT * raw = (RAWINPUT *)malloc(size);
  assert(raw != NULL);

  if (GetRawInputData(hRaw, RID_INPUT, raw, &size, sizeof(RAWINPUTHEADER)) != size)
    cerr << "GetRawInputData doesn't return correct size!" << endl;
  else if (raw->header.dwType != RIM_TYPEHID)
    cerr << "Raw input from unexpected device type!" << endl;
  else {
    RID_DEVICE_INFO info;
    UINT size = info.cbSize = sizeof(info);
    if ((int)GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICEINFO, &info, &size) < 0)
      cerr << "Could not get info on HID device!" << endl;
    else if (info.hid.dwVendorId == g_device.hid.dwVendorId && info.hid.dwProductId == g_device.hid.dwProductId) {
      BYTE * data = raw->data.hid.bRawData;
      for (DWORD i = 0; i < raw->data.hid.dwCount; i++) {
        cout << "Input detected: " << hex << setfill('0');
        for (DWORD b = 0; b < raw->data.hid.dwSizeHid; b++)
          cout << setw(2) << (unsigned)data[b] << ' ';
        cout << dec << setfill(' ');

        if (g_CurrentKey < NumKeys && g_keymap[g_CurrentKey].empty()) {
          g_keymap[g_CurrentKey].resize(raw->data.hid.dwSizeHid);
          memcpy(&g_keymap[g_CurrentKey][0], data, raw->data.hid.dwSizeHid);
          cout << "  -> " << Keys[g_CurrentKey].code;
        }
        cout << endl;

        data += raw->data.hid.dwSizeHid;
      }
    }
  }

  free(raw);
}


void HandleTick(HWND hWnd)
{
  if (g_CurrentKey >= NumKeys)
    return;

  static unsigned PromptDelay;
  static unsigned FlushDelay;
  if (!g_keymap[g_CurrentKey].empty() && ++FlushDelay > 6) {
    if (++g_CurrentKey >= NumKeys) {
      SendMessage(hWnd, WM_CLOSE, 0, 0);
      return;
    }
    FlushDelay = 0;
    PromptDelay = 0;
  }

  if (g_keymap[g_CurrentKey].empty() && PromptDelay-- == 0) {
    cout << "Press the " << Keys[g_CurrentKey].description << " (" << Keys[g_CurrentKey].code << ") key ..." << endl;
    PromptDelay = 25;
  }

  INPUT_RECORD input;
  DWORD count = 0;
  if (PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &input, 1, &count) && count > 0 &&
      ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &input, 1, &count) && count > 0 &&
      (input.EventType&KEY_EVENT) != 0 &&
       input.Event.KeyEvent.bKeyDown) {
    switch (input.Event.KeyEvent.uChar.AsciiChar) {
      case '\033' :
        SendMessage(hWnd, WM_CLOSE, 0, 0);
        break;

      case '\r' :
        g_CurrentKey++;
        PromptDelay = 0;
    }
  }
}


LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {
    case WM_CREATE :
      SetTimer(hWnd, 1, 200, NULL);
      break;

    case WM_INPUT :
      HandleRawInput((HRAWINPUT)lParam);
      break;

    case WM_TIMER :
      HandleTick(hWnd);
      break;

    case WM_CLOSE :
      DestroyWindow(hWnd);
      break;

    case WM_DESTROY :
      PostQuitMessage(0);
      break;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}


bool Initialise()
{
  static const char WndClassName[] = "WinXP-HID-LID-Trainer";	

  // Register the main window class. 
  WNDCLASS wc;
  wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = (WNDPROC)WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = g_hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wc.lpszMenuName =  NULL;
  wc.lpszClassName = WndClassName;

  if (RegisterClass(&wc) == 0) {
    cerr << "Could not register window class!";
    return false;
  }

  HWND hWnd = CreateWindowEx(0, WndClassName, WndClassName,
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, 200, 150,
                             0, 0, g_hInstance, 0);
  if (hWnd == NULL) {
    cerr << "Could not create window!";
    return false;
  }

  if (!SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_PROCESSED_INPUT)) {
    cerr << "Could not set console mode!";
    return false;
  }

  UINT deviceCount = 0;
  if (GetRawInputDeviceList(NULL, &deviceCount, sizeof(RAWINPUTDEVICELIST)) == 0 && deviceCount != 0) {
    RAWINPUTDEVICELIST * rawDevices = (RAWINPUTDEVICELIST *)calloc(deviceCount, sizeof(RAWINPUTDEVICELIST));
    if (GetRawInputDeviceList(rawDevices, &deviceCount, sizeof(RAWINPUTDEVICELIST)) > 0) {
      for (UINT i = 0; i < deviceCount; i++) {
        if (rawDevices[i].dwType == RIM_TYPEHID) {
          UINT size = g_device.cbSize = sizeof(g_device);
          if ((int)GetRawInputDeviceInfo(rawDevices[i].hDevice, RIDI_DEVICEINFO, &g_device, &size) > 0 &&
                      (g_device.hid.usUsagePage == 11 ||  // Telephony device
                       g_device.hid.usUsagePage == 12))   // Some brain dead handsets say "Consumer Device"
          {
            RAWINPUTDEVICE input;
            input.dwFlags = RIDEV_PAGEONLY|RIDEV_INPUTSINK;
            input.usUsagePage = g_device.hid.usUsagePage;
            input.usUsage = 0;
            input.hwndTarget = hWnd;
            if (RegisterRawInputDevices(&input, 1, sizeof(input)) != 0)
              return true;

            cerr << "Could not start listening for device input!" << endl;
            return false;
          }
        }
      }
    }
  }

  cerr << "Could not find a suitable device." << endl;
  return false;
}


int main(int, char **)
{
  if (!Initialise())
    return 1;

  cout << "Detected device with vendor=0x"
       << hex << setfill('0') << setw(4) << g_device.hid.dwVendorId
       << " product=0x" << setw(4) << g_device.hid.dwProductId << setfill(' ') << dec
       << "\n"
          "\n"
          "Press ESC to terminate, ENTER to skip button if not supported by handset.\n"
          "When pressing handset buttons, make sure button is released quickly to\n"
          "avoid a key up event being detected as the next button code.\n"
       << endl;

  bool running = true;
  while (running) {
    MSG msg;
    switch (GetMessage(&msg, NULL, 0, 0)) {
      case -1 :
      case 0 :
        running = false;
        break;

      default :
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
  }

  std::vector<BYTE> mask;
  mask.resize(g_keymap[0].size());

  size_t last = 0;
  size_t c;
  for (c = 0; c < NumKeys; c++) {
    if (Keys[c].mandatory && g_keymap[c].empty()) {
      cerr << "Mandatory code " << Keys[c].code << " not set, aborting." << endl;
      return 1;
    }
    if (c > 0 && !g_keymap[c].empty()) {
      for (size_t b = 0; b < mask.size(); b++) {
        if (g_keymap[c][b] != g_keymap[last][b])
          mask[b] = 0xff;
      }
      last = c;
    }
  }

  char name[20];
  _snprintf(name, sizeof(name), SectionFmt, g_device.hid.dwVendorId, g_device.hid.dwProductId);

  cout << "Add to .ini file the following:\n\n"
       << '[' << name << "]\n"
       << Description << "=Replace this text with description of handset!\n"
       << AudioDevice << "=USB\n";

  for (c = 0; c < NumKeys; c++) {
    cout << Keys[c].code << '=' << hex << setfill('0');
    for (size_t b = 0; b < g_keymap[c].size(); b++) {
      if (mask[b] == 0)
        cout << "xx";
      else
        cout << setw(2) << (unsigned)g_keymap[c][b];
    }
    cout << dec << setfill(' ') << endl;
  }
}

