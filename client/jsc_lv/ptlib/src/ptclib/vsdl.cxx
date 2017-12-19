/*
 * vsdl.cxx
 *
 * Classes to support video output via SDL
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2000 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23337 $
 * $Author: rjongbloed $
 * $Date: 2009-09-05 01:21:09 +0000 (Sat, 05 Sep 2009) $
 */

#ifdef __GNUC__
#pragma implementation "vsdl.h"
#endif

#include <ptlib.h>
#include <ptlib/vconvert.h>
#include <ptlib/pluginmgr.h>
#include <ptclib/vsdl.h>

#define new PNEW

#if P_SDL

extern "C" {

#include <SDL/SDL.h>

};

#ifdef _MSC_VER
#pragma comment(lib, P_SDL_LIBRARY)
#endif


class PVideoOutputDevice_SDL_PluginServiceDescriptor : public PDevicePluginServiceDescriptor
{
  public:
    virtual PObject *    CreateInstance(int /*userData*/) const { return new PVideoOutputDevice_SDL; }
    virtual PStringArray GetDeviceNames(int /*userData*/) const { return PString("SDL"); }
    virtual bool         ValidateDeviceName(const PString & deviceName, int /*userData*/) const { return deviceName.Find("SDL") == 0; }
} PVideoOutputDevice_SDL_descriptor;

PCREATE_PLUGIN(SDL, PVideoOutputDevice, &PVideoOutputDevice_SDL_descriptor);


///////////////////////////////////////////////////////////////////////

PVideoOutputDevice_SDL::PVideoOutputDevice_SDL()
{
  colourFormat = "YUV420P";

  sdlThread = NULL;
  updateOverlay = false;
  screen = NULL;
  overlay = NULL;
}


PVideoOutputDevice_SDL::~PVideoOutputDevice_SDL()
{ 
  Close();
}


PStringArray PVideoOutputDevice_SDL::GetDeviceNames() const
{
  return PString("SDL");
}


PBoolean PVideoOutputDevice_SDL::Open(const PString & name, PBoolean /*startImmediate*/)
{
  Close();

  deviceName = name;

  sdlThread = PThread::Create(PCREATE_NOTIFIER(SDLThreadMain), "SDL");

  sdlStarted.Wait();

  return screen != NULL;
}


PBoolean PVideoOutputDevice_SDL::IsOpen()
{
  return screen != NULL && overlay != NULL;
}


PBoolean PVideoOutputDevice_SDL::Close()
{
  if (IsOpen()) {
    sdlStop.Signal();
    sdlThread->WaitForTermination(1000);
    delete sdlThread;
  }

  return PTrue;
}


PBoolean PVideoOutputDevice_SDL::SetColourFormat(const PString & colourFormat)
{
  if (colourFormat *= "YUV420P")
    return PVideoOutputDevice::SetColourFormat(colourFormat);

  return PFalse;
}


PBoolean PVideoOutputDevice_SDL::SetFrameSize(unsigned width, unsigned height)
{
  {
    PWaitAndSignal m(mutex);

    if (width == frameWidth && height == frameHeight)
      return PTrue;

    if (!PVideoOutputDevice::SetFrameSize(width, height))
      return PFalse;
  }

  adjustSize.Signal();
  return IsOpen();
}


PINDEX PVideoOutputDevice_SDL::GetMaxFrameBytes()
{
  PWaitAndSignal m(mutex);
  return GetMaxFrameBytesConverted(CalculateFrameBytes(frameWidth, frameHeight, colourFormat));
}


PBoolean PVideoOutputDevice_SDL::SetFrameData(unsigned x, unsigned y,
                                          unsigned width, unsigned height,
                                          const BYTE * data,
                                          PBoolean endFrame) 
{
  PWaitAndSignal m(mutex);

  if (!IsOpen())
    return PFalse;

  if (x != 0 || y != 0 || width != frameWidth || height != frameHeight || !endFrame)
    return PFalse;

  ::SDL_LockYUVOverlay(overlay);

  PAssert(frameWidth == (unsigned)overlay->w && frameHeight == (unsigned)overlay->h, PLogicError);
  PINDEX pixelsFrame = frameWidth * frameHeight;
  PINDEX pixelsQuartFrame = pixelsFrame >> 2;

  const BYTE * dataPtr = data;

  PBYTEArray tempStore;
  if (converter != NULL) {
    converter->Convert(data, tempStore.GetPointer(pixelsFrame+2*pixelsQuartFrame));
    dataPtr = tempStore;
  }

  memcpy(overlay->pixels[0], dataPtr,                                  pixelsFrame);
  memcpy(overlay->pixels[1], dataPtr + pixelsFrame,                    pixelsQuartFrame);
  memcpy(overlay->pixels[2], dataPtr + pixelsFrame + pixelsQuartFrame, pixelsQuartFrame);

  ::SDL_UnlockYUVOverlay(overlay);

  updateOverlay = true;

  return PTrue;
}


bool PVideoOutputDevice_SDL::InitialiseSDL()
{
  // initialise the SDL library
  if (::SDL_Init(SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE) < 0 ) {
    PTRACE(1, "VSDL\tCouldn't initialize SDL: " << ::SDL_GetError());
    return false;
  }

#ifdef _WIN32
  SDL_SetModuleHandle(GetModuleHandle(NULL));
#endif

  PString title = "Video Output";
  PINDEX pos = deviceName.Find("TITLE=\"");
  if (pos != P_MAX_INDEX) {
    pos += 6;
    PINDEX quote = deviceName.FindLast('"');
    PString quotedTitle = deviceName(pos, quote > pos ? quote : P_MAX_INDEX);
    title = PString(PString::Literal, quotedTitle);
  }
  ::SDL_WM_SetCaption(title, NULL);

  pos = deviceName.Find("X=");
  int x = pos != P_MAX_INDEX ? atoi(&deviceName[pos+2]) : 100;

  pos = deviceName.Find("Y=");
  int y = pos != P_MAX_INDEX ? atoi(&deviceName[pos+2]) : 100;

  PString winpos(PString::Printf, "SDL_VIDEO_WINDOW_POS=%i,%i", x, y);
  putenv(winpos.GetPointer());

  screen = ::SDL_SetVideoMode(frameWidth, frameHeight, 0, SDL_SWSURFACE /* | SDL_RESIZABLE */);
  if (screen == NULL) {
    PTRACE(1, "VSDL\tCouldn't create SDL screen: " << ::SDL_GetError());
    return false;
  }

  overlay = ::SDL_CreateYUVOverlay(frameWidth, frameHeight, SDL_IYUV_OVERLAY, screen);
  if (overlay == NULL) {
    PTRACE(1, "VSDL\tCouldn't create SDL overlay: " << ::SDL_GetError());
    return false;
  }

  return true;
}


bool PVideoOutputDevice_SDL::ProcessSDLEvents()
{
  if (screen == NULL || overlay == NULL) {
    PTRACE(6, "VSDL\t Screen and/or overlay not open, so dont process events");
    return false;
  }

  SDL_Event event;  
  while (::SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT : //User selected cross
        PTRACE(3, "VSDL\t user selected cross on window, close window");
        return false;

      case SDL_VIDEORESIZE :
        PTRACE(4, "VSDL\t Resize window to " << event.resize.w << " x " << event.resize.h);
    }
  }
  // Sleep for 25 milliseconds
  SDL_Delay(25);

  return true;
}


void PVideoOutputDevice_SDL::SDLThreadMain(PThread &, INT)
{
  InitialiseSDL();

  sdlStarted.Signal();

  PTRACE(4, "VSDL\tMain loop is underway, with SDL screen initialised");

  while (ProcessSDLEvents()) {
    if (sdlStop.Wait(0))
      break;

    PWaitAndSignal m(mutex);

    if (adjustSize.Wait(0)) {
      ::SDL_FreeYUVOverlay(overlay);
      overlay = NULL;

      screen = ::SDL_SetVideoMode(frameWidth, frameHeight, 0, SDL_SWSURFACE /* | SDL_RESIZABLE */);
      if (screen != NULL)
        overlay = ::SDL_CreateYUVOverlay(frameWidth, frameHeight, SDL_IYUV_OVERLAY, screen);

      adjustSize.Acknowledge();
    }

    if (updateOverlay) {
      SDL_Rect rect;
      rect.x = 0;
      rect.y = 0;
      rect.w = (Uint16)frameWidth;
      rect.h = (Uint16)frameHeight;
      ::SDL_DisplayYUVOverlay(overlay, &rect);
      updateOverlay = true;
    }
  }

  if (overlay != NULL) {
    ::SDL_FreeYUVOverlay(overlay);
    overlay = NULL;
  }

  if (screen != NULL) {
    ::SDL_FreeSurface(screen);
    screen = NULL;
  }

  ::SDL_Quit();

  sdlStop.Acknowledge();

  PTRACE(4, "VSDL\tEnd of sdl display loop");
}


#endif // P_SDL


// End of file ////////////////////////////////////////////////////////////////
