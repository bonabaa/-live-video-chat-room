/*
 * pvfiledev.cxx
 *
 * Video file declaration
 *
 * Portable Windows Library
 *
 * Copyright (C) 2004 Post Increment
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
 * The Initial Developer of the Original Code is
 * Craig Southeren <craigs@postincrement.com>
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23300 $
 * $Author: rjongbloed $
 * $Date: 2009-08-28 09:43:32 +0000 (Fri, 28 Aug 2009) $
 */

#ifdef __GNUC__
#pragma implementation "pvfiledev.h"
#endif

#include <ptlib.h>

#if P_VIDEO
#if P_VIDFILE

#include <ptlib/vconvert.h>
#include <ptclib/pvfiledev.h>
#include <ptlib/pfactory.h>
#include <ptlib/pluginmgr.h>
#include <ptlib/videoio.h>


static const char DefaultYUVFileName[] = "*.yuv";


#define new PNEW


///////////////////////////////////////////////////////////////////////////////
// PVideoInputDevice_YUVFile

class PVideoInputDevice_YUVFile_PluginServiceDescriptor : public PDevicePluginServiceDescriptor
{
  public:
    typedef PFactory<PVideoFile> FileTypeFactory_T;

    virtual PObject * CreateInstance(int /*userData*/) const
    {
      return new PVideoInputDevice_YUVFile;
    }
    virtual PStringArray GetDeviceNames(int /*userData*/) const
    {
      return PVideoInputDevice_YUVFile::GetInputDeviceNames();
    }
    virtual bool ValidateDeviceName(const PString & deviceName, int /*userData*/) const
    {
      PCaselessString adjustedDevice = deviceName;

      FileTypeFactory_T::KeyList_T keyList = FileTypeFactory_T::GetKeyList();
      FileTypeFactory_T::KeyList_T::iterator r;
      for (r = keyList.begin(); r != keyList.end(); ++r) {
        PString ext = *r;
        PINDEX extLen = ext.GetLength();
        PINDEX length = adjustedDevice.GetLength();
        if (length > (2+extLen) && adjustedDevice.NumCompare(PString(".") + ext + "*", 2+extLen, length-(2+extLen)) == PObject::EqualTo)
          adjustedDevice.Delete(length-1, 1);
        else if (length < (2+extLen) || adjustedDevice.NumCompare(PString(".") + ext, 1+extLen, length-(1+extLen)) != PObject::EqualTo)
          continue;
        if (PFile::Access(adjustedDevice, PFile::ReadOnly)) 
          return true;
        PTRACE(1, "Unable to access file '" << adjustedDevice << "' for use as a video input device");
        return false;
      }
      return false;
    }
} PVideoInputDevice_YUVFile_descriptor;

PCREATE_PLUGIN(YUVFile, PVideoInputDevice, &PVideoInputDevice_YUVFile_descriptor);



PVideoInputDevice_YUVFile::PVideoInputDevice_YUVFile()
{
  file = NULL;

  SetColourFormat("YUV420P");
  channelNumber = 0; 
  grabCount = 0;
  SetFrameRate(10);
}


PVideoInputDevice_YUVFile::~PVideoInputDevice_YUVFile()
{
  Close();
}


PBoolean PVideoInputDevice_YUVFile::Open(const PString & _deviceName, PBoolean /*startImmediate*/)
{
  Close();

  PFilePath fileName;
  if (_deviceName != DefaultYUVFileName) {
    fileName = _deviceName;
    PINDEX lastCharPos = fileName.GetLength()-1;
    if (fileName[lastCharPos] == '*') {
      fileName.Delete(lastCharPos, 1);
      SetChannel(Channel_PlayAndRepeat);
    }
  }
  else {
    PDirectory dir;
    if (dir.Open(PFileInfo::RegularFile|PFileInfo::SymbolicLink)) {
      do {
        if (dir.GetEntryName().Right(4) == (DefaultYUVFileName+1)) {
          fileName = dir.GetEntryName();
          break;
        }
      } while (dir.Next());
    }
    if (fileName.IsEmpty()) {
      PTRACE(1, "YUVFile\tCannot find any file using " << dir << DefaultYUVFileName << " as video input device");
      return PFalse;
    }
  }

  file = PFactory<PVideoFile>::CreateInstance("yuv");
  if (file == NULL || !file->Open(fileName, PFile::ReadOnly, PFile::MustExist)) {
    PTRACE(1, "YUVFile\tCannot open file " << fileName << " as video input device");
    return PFalse;
  }

  if (!file->IsUnknownFrameSize()) {
    unsigned width, height;
    file->GetFrameSize(width, height);
    SetFrameSize(width, height);
  }

  deviceName = file->GetFilePath();
  return PTrue;    
}


PBoolean PVideoInputDevice_YUVFile::IsOpen() 
{
  return file != NULL && file->IsOpen();
}


PBoolean PVideoInputDevice_YUVFile::Close()
{
  PBoolean ok = file != NULL && file->Close();

  delete file;
  file = NULL;

  return ok;
}


PBoolean PVideoInputDevice_YUVFile::Start()
{
  return PTrue;
}


PBoolean PVideoInputDevice_YUVFile::Stop()
{
  return PTrue;
}


PBoolean PVideoInputDevice_YUVFile::IsCapturing()
{
  return IsOpen();
}


PStringArray PVideoInputDevice_YUVFile::GetInputDeviceNames()
{

  return PString(DefaultYUVFileName);
}


PBoolean PVideoInputDevice_YUVFile::SetVideoFormat(VideoFormat newFormat)
{
  return PVideoDevice::SetVideoFormat(newFormat);
}


int PVideoInputDevice_YUVFile::GetNumChannels() 
{
  return ChannelCount;  
}


PBoolean PVideoInputDevice_YUVFile::SetChannel(int newChannel)
{
  return PVideoDevice::SetChannel(newChannel);
}

PBoolean PVideoInputDevice_YUVFile::SetColourFormat(const PString & newFormat)
{
  if (!(newFormat *= "YUV420P"))
    return PFalse;

  return PVideoDevice::SetColourFormat(newFormat);
}


PBoolean PVideoInputDevice_YUVFile::SetFrameRate(unsigned rate)
{
  // if the file does not know what frame rate it is, then set it
  if (file == NULL || !file->SetFrameRate(rate))
    return PFalse;

  return PVideoDevice::SetFrameRate(file->GetFrameRate());
}


PBoolean PVideoInputDevice_YUVFile::GetFrameSizeLimits(unsigned & minWidth,
                                           unsigned & minHeight,
                                           unsigned & maxWidth,
                                           unsigned & maxHeight) 
{
  unsigned width, height;
  if (file == NULL || !file->GetFrameSize(width, height))
    return PFalse;
  minWidth  = maxWidth  = width;
  minHeight = maxHeight = height;
  return PTrue;
}

PBoolean PVideoInputDevice_YUVFile::SetFrameSize(unsigned width, unsigned height)
{
  // if the file does not know what size it is, then set it
  if (file == NULL || (file->IsUnknownFrameSize() && !file->SetFrameSize(width, height)))
    return PFalse;

  file->GetFrameSize(frameWidth, frameHeight);

  videoFrameSize = CalculateFrameBytes(frameWidth, frameHeight, colourFormat);
  return videoFrameSize > 0 && width == frameWidth && height == frameHeight;
}


PINDEX PVideoInputDevice_YUVFile::GetMaxFrameBytes()
{
  return GetMaxFrameBytesConverted(videoFrameSize);
}


PBoolean PVideoInputDevice_YUVFile::GetFrameData(BYTE * buffer, PINDEX * bytesReturned)
{    
  pacing.Delay(1000/GetFrameRate());
  return GetFrameDataNoDelay(buffer, bytesReturned);
}

 
PBoolean PVideoInputDevice_YUVFile::GetFrameDataNoDelay(BYTE *destFrame, PINDEX * bytesReturned)
{
  if (file == NULL)
    return PFalse;

  grabCount++;

  BYTE * readBuffer = destFrame;

  if (converter != NULL)
    readBuffer = frameStore.GetPointer(videoFrameSize);

  if (file->IsOpen()) {
    if (!file->ReadFrame(readBuffer))
      file->Close();
  }

  if (!file->IsOpen()) {
    switch (channelNumber) {
      case Channel_PlayAndClose:
      default:
        return PFalse;

      case Channel_PlayAndRepeat:
	    file->Open(deviceName, PFile::ReadOnly, PFile::MustExist);
        if (!file->SetPosition(0) || !file->ReadFrame(readBuffer))
          return PFalse;
        break;

      case Channel_PlayAndKeepLast:
        break;

      case Channel_PlayAndShowBlack:
        FillRect(readBuffer, 0, 0, frameWidth, frameHeight, 0, 0, 0);
        break;
    }
  }

  if (converter == NULL) {
    if (bytesReturned != NULL)
      *bytesReturned = videoFrameSize;
  } else {
    converter->SetSrcFrameSize(frameWidth, frameHeight);
    if (!converter->Convert(readBuffer, destFrame, bytesReturned))
      return PFalse;
    if (bytesReturned != NULL)
      *bytesReturned = converter->GetMaxDstFrameBytes();
  }

  return PTrue;
}


void PVideoInputDevice_YUVFile::GrabBlankImage(BYTE *resFrame)
{
  // Change colour every second, cycle is:
  // black, red, green, yellow, blue, magenta, cyan, white
  int mask = grabCount/frameRate;
  FillRect(resFrame,
           0, 0, frameWidth, frameHeight, //Fill the whole frame with the colour.
           (mask&1) ? 255 : 0, // red
           (mask&2) ? 255 : 0, // green
           (mask&4) ? 255 : 0);//blue
}

void PVideoInputDevice_YUVFile::FillRect(BYTE * frame,
                   int xPos, int initialYPos,
                   int rectWidth, int rectHeight,
                   int r, int g,  int b)
{
  //This routine fills a region of the video image with data. It is used as the central
  //point because one only has to add other image formats here.

  int yPos = initialYPos;

  int offset       = ( yPos * frameWidth ) + xPos;
  int colourOffset = ( (yPos * frameWidth) >> 2) + (xPos >> 1);

  int Y  =  ( 257 * r + 504 * g +  98 * b)/1000 + 16;
  int Cb =  (-148 * r - 291 * g + 439 * b)/1000 + 128;
  int Cr =  ( 439 * r - 368 * g -  71 * b)/1000 + 128;

  unsigned char * Yptr  = frame + offset;
  unsigned char * CbPtr = frame + (frameWidth * frameHeight) + colourOffset;
  unsigned char * CrPtr = frame + (frameWidth * frameHeight) + (frameWidth * frameHeight/4)  + colourOffset;

  int rr ;
  int halfRectWidth = rectWidth >> 1;
  int halfWidth     = frameWidth >> 1;
  
  for (rr = 0; rr < rectHeight;rr+=2) {
    memset(Yptr, Y, rectWidth);
    Yptr += frameWidth;
    memset(Yptr, Y, rectWidth);
    Yptr += frameWidth;

    memset(CbPtr, Cb, halfRectWidth);
    memset(CrPtr, Cr, halfRectWidth);

    CbPtr += halfWidth;
    CrPtr += halfWidth;
  }
}

///////////////////////////////////////////////////////////////////////////////
// PVideoOutputDevice_YUVFile

class PVideoOutputDevice_YUVFile_PluginServiceDescriptor : public PDevicePluginServiceDescriptor
{
  public:
    virtual PObject * CreateInstance(int /*userData*/) const
    {
        return new PVideoOutputDevice_YUVFile;
    }
    virtual PStringArray GetDeviceNames(int /*userData*/) const
    {
        return PVideoOutputDevice_YUVFile::GetOutputDeviceNames();
    }
    virtual bool ValidateDeviceName(const PString & deviceName, int /*userData*/) const
    {
      return (deviceName.Right(4) *= ".yuv") && (!PFile::Exists(deviceName) || PFile::Access(deviceName, PFile::WriteOnly));
    }
} PVideoOutputDevice_YUVFile_descriptor;

PCREATE_PLUGIN(YUVFile, PVideoOutputDevice, &PVideoOutputDevice_YUVFile_descriptor);


PVideoOutputDevice_YUVFile::PVideoOutputDevice_YUVFile()
{
  file = NULL;
}


PVideoOutputDevice_YUVFile::~PVideoOutputDevice_YUVFile()
{
  Close();
}


PBoolean PVideoOutputDevice_YUVFile::Open(const PString & _deviceName, PBoolean /*startImmediate*/)
{
  PFilePath fileName;
  if (_deviceName != DefaultYUVFileName)
    fileName = _deviceName;
  else {
    unsigned unique = 0;
    do {
      fileName.Empty();
      fileName.sprintf("video%03u.yuv", ++unique);
    } while (PFile::Exists(fileName));
  }

  file = PFactory<PVideoFile>::CreateInstance("yuv");
  if (file == NULL || !file->Open(fileName, PFile::WriteOnly, PFile::Create|PFile::Truncate)) {
    PTRACE(1, "YUVFile\tCannot create file " << fileName << " as video output device");
    return PFalse;
  }

  deviceName = file->GetFilePath();
  return PTrue;
}

PBoolean PVideoOutputDevice_YUVFile::Close()
{
  PBoolean ok = file == NULL || file->Close();

  delete file;
  file = NULL;

  return ok;
}

PBoolean PVideoOutputDevice_YUVFile::Start()
{
  return file != NULL && file->SetFrameSize(frameHeight, frameWidth);
}

PBoolean PVideoOutputDevice_YUVFile::Stop()
{
  return PTrue;
}

PBoolean PVideoOutputDevice_YUVFile::IsOpen()
{
  return file != NULL && file->IsOpen();
}


PStringArray PVideoOutputDevice_YUVFile::GetOutputDeviceNames()
{
  return PString(DefaultYUVFileName);
}


PBoolean PVideoOutputDevice_YUVFile::SetColourFormat(const PString & newFormat)
{
  if (!(newFormat *= "YUV420P"))
    return PFalse;

  return PVideoDevice::SetColourFormat(newFormat);
}


PINDEX PVideoOutputDevice_YUVFile::GetMaxFrameBytes()
{
  return GetMaxFrameBytesConverted(CalculateFrameBytes(frameWidth, frameHeight, colourFormat));
}


PBoolean PVideoOutputDevice_YUVFile::SetFrameData(unsigned x, unsigned y,
                                              unsigned width, unsigned height,
                                              const BYTE * data,
                                              PBoolean /*endFrame*/)
{
  if (x != 0 || y != 0 || width != frameWidth || height != frameHeight) {
    PTRACE(1, "YUVFile\tOutput device only supports full frame writes");
    return PFalse;
  }

  if (file == NULL || (file->IsUnknownFrameSize() && !file->SetFrameSize(width, height)))
    return PFalse;

  if (converter == NULL)
    return file->WriteFrame(data);

  converter->Convert(data, frameStore.GetPointer(GetMaxFrameBytes()));
  return file->WriteFrame(frameStore);
}


#endif // P_VIDFILE
#endif

