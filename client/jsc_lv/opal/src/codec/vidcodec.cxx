/*
 * vidcodec.cxx
 *
 * Uncompressed video handler
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23166 $
 * $Author: rjongbloed $
 * $Date: 2009-07-29 05:04:11 +0000 (Wed, 29 Jul 2009) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "vidcodec.h"
#endif

#include <opal/buildopts.h>

#if OPAL_VIDEO

#include <codec/vidcodec.h>

#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>

#define FRAME_WIDTH  PVideoDevice::CIF16Width
#define FRAME_HEIGHT PVideoDevice::CIF16Height
#define FRAME_RATE   30 // NTSC

const OpalVideoFormat & GetOpalRGB24()
{
  static const OpalVideoFormat RGB24(
    OPAL_RGB24,
    RTP_DataFrame::MaxPayloadType,
    NULL,
    FRAME_WIDTH, FRAME_HEIGHT,
    FRAME_RATE,
    24*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE  // Bandwidth
  );
  return RGB24;
}

const OpalVideoFormat & GetOpalRGB32()
{
  static const OpalVideoFormat RGB32(
    OPAL_RGB32,
    RTP_DataFrame::MaxPayloadType,
    NULL,
    FRAME_WIDTH, FRAME_HEIGHT,
    FRAME_RATE,
    32*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE  // Bandwidth
  );
  return RGB32;
}

const OpalVideoFormat & GetOpalYUV420P()
{
  static const OpalVideoFormat YUV420P(
    OPAL_YUV420P,
    RTP_DataFrame::MaxPayloadType,
    NULL,
    FRAME_WIDTH, FRAME_HEIGHT,
    FRAME_RATE,
    12*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE  // Bandwidth
  );
  return YUV420P;
}


#define new PNEW

/////////////////////////////////////////////////////////////////////////////

OpalVideoTranscoder::OpalVideoTranscoder(const OpalMediaFormat & inputMediaFormat,
                                         const OpalMediaFormat & outputMediaFormat)
  : OpalTranscoder(inputMediaFormat, outputMediaFormat)
  , inDataSize(10*1024)
  , outDataSize(10*1024)
  , videoEncoderMaxOutputSize(10*1024)
  , forceIFrame(true)
  , lastFrameWasIFrame(false)
	, m_totalFramesPerSec(0)
  , m_totalBytesPerSec(0)
  , m_LasttotalBytes(0)
	, m_LasttotalFrames(0)
  , m_totalFrames(0)
  , m_totalBytes(0)
  , m_keyFrames(0)
{
  acceptEmptyPayload = true;
}


static void SetFrameBytes(const OpalMediaFormat & fmt, const PString & widthOption, const PString & heightOption, PINDEX & size)
{
  int width  = fmt.GetOptionInteger(widthOption, PVideoFrameInfo::CIFWidth);
  int height = fmt.GetOptionInteger(heightOption, PVideoFrameInfo::CIFHeight);
  int newSize = PVideoDevice::CalculateFrameBytes(width, height, fmt);
  if (newSize > 0)
    size = newSize;
}


bool OpalVideoTranscoder::UpdateMediaFormats(const OpalMediaFormat & input, const OpalMediaFormat & output)
{
  PWaitAndSignal mutex(updateMutex);

  if (!OpalTranscoder::UpdateMediaFormats(input, output))
    return PFalse;

  SetFrameBytes(inputMediaFormat,  OpalVideoFormat::MaxRxFrameWidthOption(), OpalVideoFormat::MaxRxFrameHeightOption(), inDataSize);
  SetFrameBytes(outputMediaFormat, OpalVideoFormat::FrameWidthOption(),      OpalVideoFormat::FrameHeightOption(),      outDataSize);

  videoEncoderMaxOutputSize = outputMediaFormat.GetOptionInteger(OpalVideoFormat::MaxTxPacketSizeOption(), outDataSize);

  return PTrue;
}


PINDEX OpalVideoTranscoder::GetOptimalDataFrameSize(PBoolean input) const
{
  if (input)
    return inDataSize;

  if (outDataSize < maxOutputSize)
    return outDataSize;

  return maxOutputSize;
}


PBoolean OpalVideoTranscoder::ExecuteCommand(const OpalMediaCommand & command)
{
  if (PIsDescendant(&command, OpalVideoUpdatePicture)) {
    forceIFrame = true; // Reset when I-Frame is sent
    return PTrue;
  }

  return OpalTranscoder::ExecuteCommand(command);
}


PBoolean OpalVideoTranscoder::Convert(const RTP_DataFrame & /*input*/,
                                  RTP_DataFrame & /*output*/)
{
  return PFalse;
}


#if OPAL_STATISTICS
void OpalVideoTranscoder::GetStatistics(OpalMediaStatistics & statistics) const
{
  statistics.m_totalFrames = m_totalFrames;
  statistics.m_keyFrames   = m_keyFrames;
	statistics.m_nFramesPerSec   = m_totalFramesPerSec;
	statistics.m_nBytesPerSec   = m_totalBytesPerSec;
}
#endif


///////////////////////////////////////////////////////////////////////////////

PString OpalVideoUpdatePicture::GetName() const
{
  return "Update Picture";
}

PString OpalVideoUpdatePicture2::GetName() const
{
  return "Update Picture 2";
}

PString OpalTemporalSpatialTradeOff::GetName() const
{
  return "Temporal Spatial Trade Off";
}

PString OpalLostPartialPicture::GetName() const
{
  return "Lost Partial Picture";
}

PString OpalLostPicture::GetName() const
{
  return "Lost Picture";
}

#endif // OPAL_VIDEO

/////////////////////////////////////////////////////////////////////////////
