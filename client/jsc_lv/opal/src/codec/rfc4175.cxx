/*
 * rfc4175.cxx
 *
 * RFC4175 transport for uncompressed video
 *
 * Open Phone Abstraction Library
 *
 * Copyright (C) 2007 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 22444 $
 * $Author: rjongbloed $
 * $Date: 2009-04-20 23:49:06 +0000 (Mon, 20 Apr 2009) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "rfc4175.h"
#endif

#include <opal/buildopts.h>

#if OPAL_RFC4175

#include <ptclib/random.h>
#include <opal/mediafmt.h>
#include <codec/rfc4175.h>
#include <codec/opalplugin.h>


#define   FRAME_WIDTH   1920
#define   FRAME_HEIGHT  1080
#define   FRAME_RATE    60

#define   REASONABLE_UDP_PACKET_SIZE  800

class RFC4175VideoFormat : public OpalVideoFormat
{
  PCLASSINFO(RFC4175VideoFormat, OpalVideoFormat);
  public:
    RFC4175VideoFormat(
      const char * fullName,    ///<  Full name of media format
      const char * samplingName,
      unsigned int bandwidth
    );
};

const OpalVideoFormat & GetOpalRFC4175_YCbCr420()
{
  static const RFC4175VideoFormat RFC4175_YCbCr420(OPAL_RFC4175_YCbCr420, "YCbCr-4:2:0", (FRAME_WIDTH*FRAME_HEIGHT*3/2)*FRAME_RATE);
  return RFC4175_YCbCr420;
}


const OpalVideoFormat & GetOpalRFC4175_RGB()
{
  static const RFC4175VideoFormat RFC4175_RGB(OPAL_RFC4175_RGB, "RGB", FRAME_WIDTH*FRAME_HEIGHT*3*FRAME_RATE);
  return RFC4175_RGB;
}


/////////////////////////////////////////////////////////////////////////////

RFC4175VideoFormat::RFC4175VideoFormat(
      const char * fullName,    ///<  Full name of media format
      const char * samplingName,
      unsigned int bandwidth)
 : OpalVideoFormat(fullName, 
                   RTP_DataFrame::DynamicBase,
                   "raw",
                   FRAME_WIDTH, FRAME_HEIGHT,
                   FRAME_RATE,
                   bandwidth)
{
  OpalMediaOption * option;

#if OPAL_SIP
  // add mandatory fields
  option = FindOption(ClockRateOption());
  if (option != NULL)
    option->SetFMTPName("rate");

  option = FindOption(FrameWidthOption());
  if (option != NULL)
    option->SetFMTPName("width");

  option = FindOption(FrameHeightOption());
  if (option != NULL)
    option->SetFMTPName("height");
#endif // OPAL_SIP

  option = new OpalMediaOptionString("rfc4175_sampling", PTrue, samplingName);
#if OPAL_SIP
  option->SetFMTPName("sampling");
#endif // OPAL_SIP
  AddOption(option, PTrue);

  option = new OpalMediaOptionInteger("rfc4175_depth", PTrue, OpalMediaOption::NoMerge, 8);
#if OPAL_SIP
  option->SetFMTPName("depth");
#endif // OPAL_SIP
  AddOption(option, PTrue);

  option = new OpalMediaOptionString("rfc4175_colorimetry", PTrue, "BT601-5");
#if OPAL_SIP
  option->SetFMTPName("colorimetry");
#endif // OPAL_SIP
  AddOption(option, PTrue);
}

/////////////////////////////////////////////////////////////////////////////

OpalRFC4175Transcoder::OpalRFC4175Transcoder(      
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat  ///<  Output media format
)
 : OpalVideoTranscoder(inputMediaFormat, outputMediaFormat)
{
}

PINDEX OpalRFC4175Transcoder::RFC4175HeaderSize(PINDEX lines)
{ return 2 + lines*6; }

/////////////////////////////////////////////////////////////////////////////

OpalRFC4175Encoder::OpalRFC4175Encoder(      
  const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
  const OpalMediaFormat & outputMediaFormat  ///<  Output media format
) : OpalRFC4175Transcoder(inputMediaFormat, outputMediaFormat)
{
#ifdef _DEBUG
  extendedSequenceNumber = 0;
#else
  extendedSequenceNumber = PRandom::Number();
#endif

  maximumPacketSize      = REASONABLE_UDP_PACKET_SIZE;
}

void OpalRFC4175Encoder::StartEncoding(const RTP_DataFrame &)
{
}

PBoolean OpalRFC4175Encoder::ConvertFrames(const RTP_DataFrame & input, RTP_DataFrameList & _outputFrames)
{
  _outputFrames.RemoveAll();

  PAssert(sizeof(ScanLineHeader) == 6, "ScanLineHeader is not packed");

  // make sure the incoming frame is big enough for a frame header
  if (input.GetPayloadSize() < (int)(sizeof(PluginCodec_Video_FrameHeader))) {
    PTRACE(1,"RFC4175\tPayload of grabbed frame too small for frame header");
    return PFalse;
  }

  PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)input.GetPayloadPtr();
  if (header->x != 0 && header->y != 0) {
    PTRACE(1,"RFC4175\tVideo grab of partial frame unsupported");
    return PFalse;
  }

  // get information from frame header
  frameHeight       = header->height;
  frameWidth        = header->width;

  // make sure the incoming frame is big enough for the specified frame size
  if (input.GetPayloadSize() < (int)(sizeof(PluginCodec_Video_FrameHeader) + PixelsToBytes(frameWidth*frameHeight))) {
    PTRACE(1,"RFC4175\tPayload of grabbed frame too small for full frame");
    return PFalse;
  }

  srcTimestamp = input.GetTimestamp();

  StartEncoding(input);

  // save pointer to output data
  dstFrames = &_outputFrames;
  dstScanlineCounts.resize(0);

  // encode the full frame
  EncodeFullFrame();

  // grab the actual data
  EndEncoding();

  return PTrue;
}

void OpalRFC4175Encoder::EncodeFullFrame()
{
  // encode the scan lines
  unsigned y;
  for (y = 0; y < frameHeight; y += GetRowsPerPgroup())
    EncodeScanLineSegment(y, 0, frameWidth);
}

void OpalRFC4175Encoder::EncodeScanLineSegment(PINDEX y, PINDEX offs, PINDEX width)
{
  // add new packets until scan line segment is finished
  PINDEX endX = offs + width;
  PINDEX x = offs;
  while (x < endX) {

    PINDEX roomLeft = maximumPacketSize - dstPacketSize;

    // if current frame cannot hold at least one pgroup, then add a new frame
    if ((dstFrames->GetSize() == 0) || (roomLeft < (PINDEX)(sizeof(ScanLineHeader) + GetPgroupSize()))) {
      AddNewDstFrame();
      continue;
    }

    PINDEX maxPGroupsInPacket = (roomLeft - (PINDEX)sizeof(ScanLineHeader)) / GetPgroupSize();
    PINDEX pGroupsInScanLine = (endX - x) / GetColsPerPgroup();
    // calculate how many pixels we can add

    PINDEX pixelsToAdd, octetsToAdd;
    if (maxPGroupsInPacket  <= pGroupsInScanLine) {
      octetsToAdd = maxPGroupsInPacket * GetPgroupSize(); 
      pixelsToAdd = maxPGroupsInPacket * GetColsPerPgroup();
    } else {
      octetsToAdd = pGroupsInScanLine * GetPgroupSize();
      pixelsToAdd = endX - x;
    }

    // populate the scan line table
    dstScanLineTable->length = (WORD)octetsToAdd;
    dstScanLineTable->y      = (WORD)y;
    dstScanLineTable->offset = (WORD)x | 0x8000;

    // adjust pointer to scan line table and number of scan lines
    ++dstScanLineTable;
    ++dstScanLineCount;

    // adjust packet size
    dstPacketSize += sizeof(ScanLineHeader) + octetsToAdd;

    // adjust X offset
    x += pixelsToAdd;
  }
}

void OpalRFC4175Encoder::AddNewDstFrame()
{
  // complete the previous output frame (if any)
  FinishOutputFrame();

  // allocate a new output frame
  RTP_DataFrame * frame = new RTP_DataFrame(maximumPacketSize - RTP_DataFrame::MinHeaderSize);
  dstFrames->Append(frame);

  // initialise payload size for maximum size
  frame->SetPayloadType(outputMediaFormat.GetPayloadType());
  // initialise current output scanline count;
  dstScanLineCount = 0;
  dstPacketSize    = frame->GetHeaderSize() + 2;
  dstScanLineTable = (ScanLineHeader *)(frame->GetPayloadPtr() + 2);
}

void OpalRFC4175Encoder::FinishOutputFrame()
{
  if ((dstFrames->GetSize() > 0) && (dstScanLineCount > 0)) {

    // populate the frame fields
    RTP_DataFrame & dst = dstFrames->back();

    // set the end of scan line table bit
    --dstScanLineTable;
    dstScanLineTable->offset = ((WORD)dstScanLineTable->offset) & 0x7FFF;

    // set the timestamp and payload type
    dst.SetTimestamp(srcTimestamp);
    dst.SetPayloadType(outputMediaFormat.GetPayloadType());

    // set and increment the sequence number
    dst.SetSequenceNumber((WORD)(extendedSequenceNumber & 0xffff));
    *(PUInt16b *)dst.GetPayloadPtr() = (WORD)((extendedSequenceNumber >> 16) & 0xffff);
    ++extendedSequenceNumber;

    // set actual payload size
    dst.SetPayloadSize(dstPacketSize - dst.GetHeaderSize());

    // save scanline count
    dstScanlineCounts.push_back(dstScanLineCount);
  }
}

/////////////////////////////////////////////////////////////////////////////

OpalRFC4175Decoder::OpalRFC4175Decoder(      
  const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
  const OpalMediaFormat & outputMediaFormat  ///<  Output media format
) : OpalRFC4175Transcoder(inputMediaFormat, outputMediaFormat)
{
  inputFrames.AllowDeleteObjects();
  first = true;
  Initialise();
}


OpalRFC4175Decoder::~OpalRFC4175Decoder()
{
}


PBoolean OpalRFC4175Decoder::Initialise()
{
  frameWidth  = 0;
  frameHeight = 0;

  inputFrames.RemoveAll();
  scanlineCounts.resize(0);

  return PTrue;
}


PBoolean OpalRFC4175Decoder::ConvertFrames(const RTP_DataFrame & input, RTP_DataFrameList & output)
{
  output.RemoveAll();

  PAssert(sizeof(ScanLineHeader) == 6, "ScanLineHeader is not packed");

  // do quick sanity check on packet
  if (input.GetPayloadSize() < 2) {
    PTRACE(1,"RFC4175\tinput frame too small for header");
    return PFalse;
  }

  // get extended sequence number
  DWORD receivedSeqNo = input.GetSequenceNumber() | ((*(PUInt16b *)input.GetPayloadPtr()) << 16);

  PBoolean ok = PTrue;

  // special handling for first packet
  if (first) {
    lastSequenceNumber = receivedSeqNo;
    lastTimeStamp      = input.GetTimestamp();
    first = PFalse;
  } 
  else {
    // if timestamp changed, we lost the marker bit on the previous input frame
    // so, flush the output and change to the new timestamp
    if ((input.GetTimestamp() != lastTimeStamp) && (inputFrames.GetSize() > 0)) {
      PTRACE(2, "RFC4175\tDetected change of timestamp - marker bit lost");
      DecodeFrames(output);
    }
    lastTimeStamp = input.GetTimestamp();

    // if packet is out of sequence, determine if to ignore packet or accept it and update sequence number
    ++lastSequenceNumber;
    if (lastSequenceNumber != receivedSeqNo) {
      ok = receivedSeqNo > lastSequenceNumber;
      if (!ok && ((lastSequenceNumber - receivedSeqNo) > 0xfffffc00)) {
        ok = true;
        lastSequenceNumber = receivedSeqNo;
      }
      PTRACE(2, "RFC4175\t" << (ok ? "Accepting" : "Ignoring") << " out of order packet");
    }
  }

  // make a pass through the scan line table and update the overall frame width and height
  PINDEX lineCount = 0;
  if (ok) {

    ScanLineHeader * scanLinePtr = (ScanLineHeader *)(input.GetPayloadPtr() + 2);

    PBoolean lastLine = false;
    while (!lastLine && RFC4175HeaderSize(lineCount+1) < input.GetPayloadSize()) {

      // scan line length (in pgroups units)
      PINDEX lineLength = scanLinePtr->length / GetPgroupSize();

      // line number 
      WORD lineNumber = scanLinePtr->y & 0x7fff; 

      // pixel offset of scanline start
      WORD offset = scanLinePtr->offset;

      // detect if last scanline in table
      if (offset & 0x8000)
        offset &= 0x7fff;
      else
        lastLine = true;

      // update frame width and height
      PINDEX right = offset + lineLength * GetColsPerPgroup();
      if (right > frameWidth)
        frameWidth = right;
      PINDEX bottom = lineNumber + GetRowsPerPgroup();
      if (bottom > frameHeight)
        frameHeight = bottom;

      // count lines
      ++lineCount;

      // update scan line pointer
      ++scanLinePtr;
    }
  }

  // add the frame to the input frame list, if OK
  if (ok) {

    inputFrames.Append(input.Clone());

    scanlineCounts.push_back(lineCount);
  }

  // if marker set, decode the frames
  if (input.GetMarker()) 
    DecodeFrames(output);

  return PTrue;
}

/////////////////////////////////////////////////////////////////////////////

void Opal_YUV420P_to_RFC4175YCbCr420::StartEncoding(const RTP_DataFrame & input)
{
  // save pointers to input data
  srcYPlane    = input.GetPayloadPtr() + sizeof(PluginCodec_Video_FrameHeader);
  srcCbPlane   = srcYPlane  + (frameWidth * frameHeight);
  srcCrPlane   = srcCbPlane + (frameWidth * frameHeight / 4);
}

void Opal_YUV420P_to_RFC4175YCbCr420::EndEncoding()
{
  FinishOutputFrame();

  PTRACE(4, "RFC4175\tEncoded YUV420P input frame to " << dstFrames->GetSize() << " RFC4175 output frames in YCbCr420 format");

  PINDEX f= 0;
  for (RTP_DataFrameList::iterator output = dstFrames->begin(); output != dstFrames->end(); ++output,++f) {
    ScanLineHeader * hdrs = (ScanLineHeader *)(output->GetPayloadPtr() + 2);
    register BYTE * scanLineDataPtr = output->GetPayloadPtr() + 2 + dstScanlineCounts[f] * sizeof(ScanLineHeader);
    for (PINDEX i = 0; i < dstScanlineCounts[f]; ++i) {
      ScanLineHeader & hdr = hdrs[i];

      PINDEX x     = hdr.offset & 0x7fff;
      PINDEX y     = hdr.y & 0x7fff;
      unsigned len = (hdr.length / GetPgroupSize()) * GetColsPerPgroup();

      register BYTE * yPlane0  = srcYPlane  + (frameWidth * y + x);
      register BYTE * yPlane1  = yPlane0    + frameWidth;
      register BYTE * cbPlane  = srcCbPlane + (frameWidth * y / 4) + x / 2;
      register BYTE * crPlane  = srcCrPlane + (frameWidth * y / 4) + x / 2;

      unsigned p;
      for (p = 0; p < len; p += 2) {
        *scanLineDataPtr++ = *yPlane0++;
        *scanLineDataPtr++ = *yPlane0++;
        *scanLineDataPtr++ = *yPlane1++;
        *scanLineDataPtr++ = *yPlane1++;
        *scanLineDataPtr++ = *cbPlane++;
        *scanLineDataPtr++ = *crPlane++;
      }
    }
  } 

  // set marker bit on last frame
  if (dstFrames->GetSize() != 0) {
    RTP_DataFrame & dst = dstFrames->back();
    dst.SetMarker(PTrue);
  }
}

/////////////////////////////////////////////////////////////////////////////

PBoolean Opal_RFC4175YCbCr420_to_YUV420P::DecodeFrames(RTP_DataFrameList & output)
{
  if (inputFrames.GetSize() == 0) {
    PTRACE(4, "RFC4175\tNo input frames to decode");
    return PFalse;
  }

  //if (frameHeight != 144 || frameWidth != 176) {
  //  int s = inputFrames.GetSize();
  //  PTRACE(4, "not right frame " << s);
  //}

  PTRACE(4, "RFC4175\tDecoding output from " << inputFrames.GetSize() << " input frames");

  // allocate destination frame
  output.Append(new RTP_DataFrame(sizeof(PluginCodec_Video_FrameHeader) + PixelsToBytes(frameWidth*frameHeight)));
  RTP_DataFrame & outputFrame = output.back();
  outputFrame.SetMarker(PTrue);
  outputFrame.SetPayloadType(outputMediaFormat.GetPayloadType());

  // get pointer to header and payload
  PluginCodec_Video_FrameHeader * hdr = (PluginCodec_Video_FrameHeader *)outputFrame.GetPayloadPtr();
  hdr->x = 0;
  hdr->y = 0;
  hdr->width  = frameWidth;
  hdr->height = frameHeight;

  BYTE * payload    = OPAL_VIDEO_FRAME_DATA_PTR(hdr);
  BYTE * dstYPlane  = payload;
  BYTE * dstCbPlane = dstYPlane  + (frameWidth * frameHeight);
  BYTE * dstCrPlane = dstCbPlane + (frameWidth * frameHeight / 4);

  // pass through all of the input frames, and extract information
  PINDEX f = 0;
  for (RTP_DataFrameList::iterator source = inputFrames.begin(); source != inputFrames.end(); ++source,++f) {
    // scan through table
    PINDEX l;
    ScanLineHeader * tablePtr = (ScanLineHeader *)(source->GetPayloadPtr() + 2);

    BYTE * yuvData = source->GetPayloadPtr() + 2 + scanlineCounts[f] * sizeof(ScanLineHeader);

    for (l = 0; l < scanlineCounts[f]; ++l) {

      // scan line length (in pixels units)
      PINDEX width = (tablePtr->length / GetPgroupSize()) * GetColsPerPgroup();

      // line number 
      WORD y = tablePtr->y & 0x7fff; 

      // pixel offset of scanline start
      WORD x = tablePtr->offset & 0x7fff;

      ++tablePtr;

      // only convert lines on even boundaries
      if (
          ((y & 1) == 0) 
          ) {

        BYTE * yPlane0 = dstYPlane  + y * frameWidth + x;
        BYTE * yPlane1 = yPlane0    + frameWidth;
        BYTE * cbPlane = dstCbPlane + (y * frameWidth / 4) + x / 2;
        BYTE * crPlane = dstCrPlane + (y * frameWidth / 4) + x / 2;

        PINDEX i;
        for (i = 0; i < width; i += 2) {
          *yPlane0++ = *yuvData++;
          *yPlane0++ = *yuvData++;
          *yPlane1++ = *yuvData++;
          *yPlane1++ = *yuvData++;
          *cbPlane++ = *yuvData++;
          *crPlane++ = *yuvData++;
        }
      }
    }
  }

  // reinitialise the buffers
  Initialise();

  return PTrue;
}

/////////////////////////////////////////////////////////////////////////////

void Opal_RGB24_to_RFC4175RGB::StartEncoding(const RTP_DataFrame & input)
{
  // save pointer to input data
  rgbBase  = input.GetPayloadPtr() + sizeof(PluginCodec_Video_FrameHeader);
}

void Opal_RGB24_to_RFC4175RGB::EndEncoding()
{
 FinishOutputFrame();

  PTRACE(4, "RFC4175\tEncoded RGB24 input frame to " << dstFrames->GetSize() << " RFC4175 output frames in RGB format");

  BYTE* inPtr = rgbBase;

  PINDEX f = 0;
  for (RTP_DataFrameList::iterator output = dstFrames->begin(); output != dstFrames->end(); ++output,++f) {
    ScanLineHeader * hdrs = (ScanLineHeader *)(output->GetPayloadPtr() + 2);
    BYTE * scanLineDataPtr = output->GetPayloadPtr() + 2 + dstScanlineCounts[f] * sizeof (ScanLineHeader);
	
    for (PINDEX i = 0; i < dstScanlineCounts[f]; ++i) {
      ScanLineHeader & hdr = hdrs[i];

      memcpy(scanLineDataPtr, inPtr, (int)hdr.length); 
	  scanLineDataPtr+= hdr.length;
      inPtr	+= hdr.length; 

	}
  } 

  // set marker bit on last frame
  if (dstFrames->GetSize() != 0) {
    RTP_DataFrame & dst = dstFrames->back();
    dst.SetMarker(TRUE);
  }
}

/////////////////////////////////////////////////////////////////////////////

PBoolean Opal_RFC4175RGB_to_RGB24::DecodeFrames(RTP_DataFrameList & output)
{
  if (inputFrames.GetSize() == 0) {
    PTRACE(4, "RFC4175\tNo input frames to decode");
    return PFalse;
  }

  PTRACE(4, "RFC4175\tDecoding output from " << inputFrames.GetSize() << " input frames");

  // allocate destination frame
  output.Append(new RTP_DataFrame(sizeof(PluginCodec_Video_FrameHeader) + PixelsToBytes(frameWidth*frameHeight)));
  RTP_DataFrame & outputFrame = output.back();
  outputFrame.SetMarker(PTrue);

  // get pointer to header and payload
  PluginCodec_Video_FrameHeader * hdr = (PluginCodec_Video_FrameHeader *)outputFrame.GetPayloadPtr();
  hdr->x = 0;
  hdr->y = 0;
  hdr->width  = frameWidth;
  hdr->height = frameHeight;

  BYTE * rgbDest = OPAL_VIDEO_FRAME_DATA_PTR(hdr);

  // pass through all of the input frames, and extract information
  PINDEX f = 0;
  for (RTP_DataFrameList::iterator source = inputFrames.begin(); source != inputFrames.end(); ++source,++f) {
    // scan through table
    PINDEX l;
    ScanLineHeader * tablePtr = (ScanLineHeader *)(source->GetPayloadPtr() + 2);

    BYTE * rgbSource = source->GetPayloadPtr() + 2 + scanlineCounts[f] * sizeof(ScanLineHeader);

    for (l = 0; l < scanlineCounts[f]; ++l) {

      // scan line length
      PINDEX width = (tablePtr->length / GetPgroupSize()) * GetColsPerPgroup();

      // line number 
      WORD y = tablePtr->y & 0x7fff; 

      // pixel offset of scanline start
      WORD x = tablePtr->offset & 0x7fff;

      ++tablePtr;

      memcpy(rgbDest + (y * frameWidth + x) * 3, rgbSource, width * 3);

      rgbSource += width*3;
    }
  }

  // reinitialise the buffers
  Initialise();

  return PTrue;
}

#endif // OPAL_RFC4175


