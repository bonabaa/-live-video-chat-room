/*
 * pvidchan.cxx
 *
 * Video Channel implementation.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): Derek Smithies (derek@indranet.co.nz)
 *
 * $Revision: 20949 $
 * $Author: ms30002000 $
 * $Date: 2008-09-14 09:02:41 +0000 (Sun, 14 Sep 2008) $
 */

#ifdef __GNUC__
#pragma implementation "video.h"
#endif

#include <ptlib.h>

#if P_VIDEO 

#include <ptlib/video.h>

PVideoChannel::PVideoChannel() 
{
  mpInput = NULL;
  mpOutput = NULL;
}


PVideoChannel::PVideoChannel(const PString & device,
                             Directions dir)
{
  mpInput = NULL;
  mpOutput = NULL;
  Open(device, dir);
}

PVideoChannel::~PVideoChannel()
{
  Close();
}


PStringArray PVideoChannel::GetDeviceNames(Directions /*dir*/)
{
  return PString("Video Channel Base");
}


PString PVideoChannel::GetDefaultDevice(Directions /*dir*/)
{
#if defined(P_FREEBSD) || defined(P_OPENBSD)
  return "/dev/bktr0";
#endif

#ifndef DEFAULT_VIDEO
     return "/dev/video0";
#else
  return DEFAULT_VIDEO;
#endif
}


PBoolean PVideoChannel::Open(const PString & dev,
                         Directions dir)
{
  PWaitAndSignal m(accessMutex);

  Close();

  deviceName = dev;
  direction = dir;
  
  return PTrue;
}



PBoolean PVideoChannel::Read(void * buf, PINDEX  len)
{
  PWaitAndSignal m(accessMutex);

  if (mpInput == NULL)  
    return PFalse;

  BYTE * dataBuf;
  PINDEX dataLen;
  dataBuf = (BYTE *)buf;
  dataLen = len;
  return mpInput->GetFrameData(dataBuf, &dataLen);

  // CHANGED  return PTrue;
}

PBoolean PVideoChannel::Write(const void * buf,  //image data to be rendered
                          PINDEX      /* len */)
{
  PWaitAndSignal m(accessMutex);

  if (mpOutput == NULL)
    return PFalse;


  if (mpInput == NULL) {
    PTRACE(6,"PVC\t::Write, frame size is "
              << mpOutput->GetFrameWidth() << "x" << mpOutput->GetFrameHeight() << 
              " VideoGrabber is unavailable");
    return mpOutput->SetFrameData(0, 0, mpOutput->GetFrameWidth(), mpOutput->GetFrameHeight(), (const BYTE *)buf, PTrue);
  }

  PTRACE(6,"PVC\t::Write, frame size is " 
               << mpInput->GetFrameWidth() << "x" << mpInput->GetFrameHeight() << 
               " VideoGrabber is source of size");
  return mpOutput->SetFrameData(0, 0,
        mpInput->GetFrameWidth(), mpInput->GetFrameHeight(),
           (const BYTE *)buf, PTrue);  
}

PBoolean PVideoChannel::Close()
{
  PWaitAndSignal m(accessMutex);

  CloseVideoReader();
  CloseVideoPlayer();

  return PTrue;
}

/*returns true if either input or output is open */
PBoolean PVideoChannel::IsOpen() const 
{
   PWaitAndSignal m(accessMutex);

   return (mpInput != NULL) || (mpOutput != NULL);
}


PString PVideoChannel::GetName() const
{
  return deviceName;
}

void PVideoChannel::AttachVideoPlayer(PVideoOutputDevice * device, PBoolean keepCurrent)
{
  PWaitAndSignal m(accessMutex);

  if (mpOutput && keepCurrent)
    PAssertAlways("Error: Attempt to add video player while one is already defined");
  
  CloseVideoPlayer();
   
  mpOutput = device;
}

void PVideoChannel::AttachVideoReader(PVideoInputDevice * device, PBoolean keepCurrent)
{
  PWaitAndSignal m(accessMutex);

  if ((mpInput != NULL) && keepCurrent)
    PAssertAlways("Error: Attempt to add video reader while one is already defined");
  
  CloseVideoReader();
  
  mpInput = device;
}

void PVideoChannel::CloseVideoPlayer()
{
  PWaitAndSignal m(accessMutex);

  if (mpOutput != NULL)
    delete mpOutput;
  
  mpOutput = NULL;
}

void PVideoChannel::CloseVideoReader()
{
  PWaitAndSignal m(accessMutex);

  if (mpInput != NULL)
    delete mpInput;
  
  mpInput = NULL;
}

PINDEX  PVideoChannel::GetGrabHeight() 
{
   PWaitAndSignal m(accessMutex);
   if (mpInput != NULL)
     return mpInput->GetFrameHeight();
   else
     return 0;
}


PINDEX  PVideoChannel::GetGrabWidth()
{
   PWaitAndSignal m(accessMutex);

   if (mpInput != NULL)
     return mpInput->GetFrameWidth();
   else
     return 0;
}

PBoolean PVideoChannel::IsGrabberOpen()
{
  PWaitAndSignal m(accessMutex);

  if (mpInput != NULL)
    return mpInput->IsOpen();
  else
    return PFalse; 
}

PBoolean PVideoChannel::IsRenderOpen()      
{
  PWaitAndSignal m(accessMutex);

  if (mpOutput != NULL)
    return mpOutput->IsOpen();
  else
    return PFalse; 
}

PBoolean PVideoChannel::DisplayRawData(void *videoBuffer)
{
  PWaitAndSignal m(accessMutex);

  if ((mpOutput == NULL) || (mpInput == NULL))
    return PFalse;
  
  PINDEX length=0;

  int frameWidth  = GetGrabWidth();
  int frameHeight = GetGrabHeight();
  PTRACE(6,"Video\t data direct:: camera-->render, size " << frameWidth << "x" << frameHeight );
  
  SetRenderFrameSize(frameWidth, frameHeight);
  Read(videoBuffer, length);
  Write((const void *)videoBuffer, length);
  
  return PTrue;      
}

void  PVideoChannel::SetGrabberFrameSize(int _width, int _height)     
{ 
  PTRACE(6, "PVC\t Set Grabber frame size to " << _width << "x" << _height);
  PWaitAndSignal m(accessMutex);

  if (mpInput != NULL) {
    if ((GetGrabWidth() != _width) || (GetGrabHeight() != _height))
      mpInput->SetFrameSize((unsigned)_width, (unsigned)_height);
  } 
}

void  PVideoChannel::SetRenderFrameSize(int _width, int _height) 
{ 
  PTRACE(6, "PVC\t Set Renderer frame size to " << _width << "x" << _height);
  PWaitAndSignal m(accessMutex);

  if (mpOutput != NULL)
    mpOutput->SetFrameSize(_width, _height); 
}

PVideoInputDevice *PVideoChannel::GetVideoReader()
{
  return mpInput;
}

PVideoOutputDevice *PVideoChannel::GetVideoPlayer()
{
  return mpOutput;
}

PBoolean  PVideoChannel::Redraw(const void * frame) 
{ 
  PTRACE(6,"PVC\t::Redraw a frame");
  return Write(frame, 0);
}

PINDEX   PVideoChannel::GetRenderWidth()
{ 
  PWaitAndSignal m(accessMutex);

  if (mpOutput != NULL)
    return mpOutput->GetFrameWidth(); 

  return 0;
}

PINDEX  PVideoChannel::GetRenderHeight()
{
  PWaitAndSignal m(accessMutex);

 if (mpOutput != NULL)
  return mpOutput->GetFrameHeight(); 

 return 0;
}


void PVideoChannel::RestrictAccess()
{
  accessMutex.Wait();
}

void PVideoChannel::EnableAccess()
{
  accessMutex.Signal();
}


PBoolean PVideoChannel::ToggleVFlipInput()
{
  PWaitAndSignal m(accessMutex);

 if (mpOutput != NULL)
  return mpInput->SetVFlipState(mpInput->GetVFlipState()); 

 return PFalse;
}

#endif

///////////////////////////////////////////////////////////////////////////
// End of file

