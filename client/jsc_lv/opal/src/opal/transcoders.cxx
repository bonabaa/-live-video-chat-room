/*
 * transcoders.cxx
 *
 * Abstractions for converting media from one format to another.
 *
 * Open H323 Library
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * $Revision: 23968 $
 * $Author: rjongbloed $
 * $Date: 2010-01-22 09:12:11 +0000 (Fri, 22 Jan 2010) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "transcoders.h"
#endif

#include <opal/buildopts.h>

#include <opal/transcoders.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalMediaFormatPair::OpalMediaFormatPair(const OpalMediaFormat & inputFmt,
                                         const OpalMediaFormat & outputFmt)
  : inputMediaFormat(inputFmt),
    outputMediaFormat(outputFmt)
{
}


void OpalMediaFormatPair::PrintOn(ostream & strm) const
{
  strm << inputMediaFormat << "->" << outputMediaFormat;
}


PObject::Comparison OpalMediaFormatPair::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, OpalMediaFormatPair), PInvalidCast);
  const OpalMediaFormatPair & other = (const OpalMediaFormatPair &)obj;
  if (inputMediaFormat < other.inputMediaFormat)
    return LessThan;
  if (inputMediaFormat > other.inputMediaFormat)
    return GreaterThan;
  return outputMediaFormat.Compare(other.outputMediaFormat);
}


/////////////////////////////////////////////////////////////////////////////

OpalTranscoder::OpalTranscoder(const OpalMediaFormat & inputMediaFormat,
                               const OpalMediaFormat & outputMediaFormat)
  : OpalMediaFormatPair(inputMediaFormat, outputMediaFormat)
{
  maxOutputSize = 32768; // Just something, usually changed by OpalMediaPatch
  outputIsRTP = inputIsRTP = PFalse;
  acceptEmptyPayload = false;
  acceptOtherPayloads = false;
}


bool OpalTranscoder::UpdateMediaFormats(const OpalMediaFormat & input, const OpalMediaFormat & output)
{
  PWaitAndSignal mutex(updateMutex);

  if (input.IsValid()) {
    if (inputMediaFormat == input)
      inputMediaFormat = input;
    else if (!inputMediaFormat.Merge(input))
      return false;
  }

  if (output.IsValid()) {
    if (outputMediaFormat == output)
      outputMediaFormat = output;
    else if (!outputMediaFormat.Merge(output))
      return false;
  }

  return true;
}


PBoolean OpalTranscoder::ExecuteCommand(const OpalMediaCommand & /*command*/)
{
  return PFalse;
}


void OpalTranscoder::SetInstanceID(const BYTE * /*instance*/, unsigned /*instanceLen*/)
{
}


RTP_DataFrame::PayloadTypes OpalTranscoder::GetPayloadType(PBoolean input) const
{
  return (input ? inputMediaFormat : outputMediaFormat).GetPayloadType();
}


#if OPAL_STATISTICS
void OpalTranscoder::GetStatistics(OpalMediaStatistics & /*statistics*/) const
{
}
#endif


PBoolean OpalTranscoder::ConvertFrames(const RTP_DataFrame & input, RTP_DataFrameList & output)
{
  // make sure there is at least one output frame available
  if (output.IsEmpty())
    output.Append(new RTP_DataFrame(0, maxOutputSize));
  else {
    while (output.GetSize() > 1)
      output.RemoveAt(1);
  }

  // set the output timestamp and marker bit
  unsigned timestamp = input.GetTimestamp();
  unsigned inClockRate = inputMediaFormat.GetClockRate();
  unsigned outClockRate = outputMediaFormat.GetClockRate();
  if (inClockRate != outClockRate)
    timestamp = (unsigned)((PUInt64)timestamp*outClockRate/inClockRate);
  output.front().SetTimestamp(timestamp);
  output.front().SetMarker(input.GetMarker());

  // set the output payload type directly from the output media format
  // and the input payload directly from the input media format
  output.front().SetPayloadType(GetPayloadType(false));

  // do not transcode if no match
  RTP_DataFrame::PayloadTypes packetPayloadType = input.GetPayloadType();
  RTP_DataFrame::PayloadTypes formatPayloadType = inputMediaFormat.GetPayloadType();
  if (formatPayloadType != RTP_DataFrame::MaxPayloadType && packetPayloadType != formatPayloadType && input.GetPayloadSize() > 0) {
    PTRACE(2, "Opal\tExpected payload type " << formatPayloadType << ", but received " << packetPayloadType << ". Ignoring packet");
    output.RemoveAll();
    return PTrue;
  }

  return Convert(input, output.front());
}


OpalTranscoder * OpalTranscoder::Create(const OpalMediaFormat & srcFormat,
                                        const OpalMediaFormat & destFormat,
                                                   const BYTE * instance,
                                                       unsigned instanceLen)
{
  OpalTranscoder * transcoder = OpalTranscoderFactory::CreateInstance(OpalTranscoderKey(srcFormat, destFormat));
  if (transcoder != NULL) {
    transcoder->SetInstanceID(instance, instanceLen); // Make sure this is done first
    transcoder->UpdateMediaFormats(srcFormat, destFormat);
  }

  return transcoder;
}


static bool MergeFormats(const OpalMediaFormatList & masterFormats,
                         const OpalMediaFormat & srcCapability,
                         const OpalMediaFormat & dstCapability,
                         OpalMediaFormat & srcFormat,
                         OpalMediaFormat & dstFormat)
{
  /* Do the required merges to get final media format.

     We start with the media options from the master list (if present) as a
     starting point. This represents the local users "desired" options. Then
     we merge in the remote users capabilities, determined from the lists passed
     to OpalTranscoder::SelectFormats(). We finally rmerge the two formats so
     common attributes are agreed upon.
  
     Encoder example:
         sourceMediaFormats = YUV420P[QCIF]                        (from PCSSEndPoint)
         sinkMediaFormats   = H.261[QCIF,CIF],H.263[QCIF,CIF,D,E]  (from remotes capabilities)
         masterMediaFormats = H.263[SQCIF,QCIF,CIF,D]              (from local capabilities)

       OpalTranscoder::SelectFormats above will locate the pair of capabilities:
         srcCapability = YUV420P[QCIF]
         dstCapability = H.263[QCIF,CIF,D,E]

       Then we get from the masterMediaFormats:
         srcFormat = YUV420P[QCIF]
         dstFormat = H.263[SQCIF,QCIF,CIF,D]

       Then merging in the respective capability to the format we get:
         srcFormat = YUV420P[QCIF]          <-- No change, srcCapability is identical
         dstFormat = H.263[QCIF,CIF,D]      <-- Drop SQCIF as remote can't do it
                                            <-- Do not add annex E as we can't do it

       Then merging src into dst and dst into src we get:
         srcFormat = YUV420P[QCIF]          <-- No change, dstFormat is superset
         dstFormat = H.263[QCIF,D]          <-- Drop to CIF as YUV420P can't do it
                                            <-- Annex D is left as YUV420P does not have
                                                this option at all.


     Decoder example:
         sourceMediaFormats = H.261[QCIF,CIF],H.263[QCIF,CIF,D,E]  (from remotes capabilities)
         sinkMediaFormats   = YUV420P[QCIF]                        (from PCSSEndPoint)
         masterMediaFormats = H.263[SQCIF,QCIF,CIF,D]              (from local capabilities)

       OpalTranscoder::SelectFormats above will locate the pair of capabilities:
         srcCapability = H.263[QCIF,CIF,D,E]
         dstCapability = YUV420P[QCIF]

       Then we get from the masterMediaFormats:
         dstFormat = H.263[SQCIF,QCIF,CIF,D]
         srcFormat = YUV420P[QCIF]

       Then merging in the respective capability to the format we get:
         srcFormat = H.263[QCIF,CIF,D]      <-- Drop SQCIF as remote can't do it
                                            <-- Do not add annex E as we can't do it
         dstFormat = YUV420P[QCIF]          <-- No change, srcCapability is identical

       Then merging in capabilities we get:
         srcFormat = H.263[QCIF,D]          <-- Drop to CIF as YUV420P can't do it
                                            <-- Annex D is left as YUV420P does not have
                                                this option at all.
         dstFormat = YUV420P[QCIF]          <-- No change, srcFormat is superset
   */

  OpalMediaFormatList::const_iterator masterFormat = masterFormats.FindFormat(srcCapability);
  if (masterFormat == masterFormats.end()) {
    srcFormat = srcCapability;
    PTRACE(5, "Opal\tInitial source format from capabilities:\n" << setw(-1) << srcFormat);
  }
  else {
    srcFormat = *masterFormat;
    PTRACE(5, "Opal\tInitial source format from master:\n" << setw(-1) << srcFormat);
    if (!srcFormat.Merge(srcCapability))
      return false;
  }

  masterFormat = masterFormats.FindFormat(dstCapability);
  if (masterFormat == masterFormats.end()) {
    dstFormat = dstCapability;
    PTRACE(5, "Opal\tInitial destination format from capabilities:\n" << setw(-1) << dstFormat);
  }
  else {
    dstFormat = *masterFormat;
    PTRACE(5, "Opal\tInitial destination format from master:\n" << setw(-1) << dstFormat);
    if (!dstFormat.Merge(dstCapability))
      return false;
  }

  if (!srcFormat.Merge(dstFormat))
    return false;

  if (!dstFormat.Merge(srcFormat))
    return false;

  return true;
}


bool OpalTranscoder::SelectFormats(const OpalMediaType & mediaType,
                                   const OpalMediaFormatList & srcFormats2,
                                   const OpalMediaFormatList & dstFormats2,
                                   const OpalMediaFormatList & allFormats,
                                   OpalMediaFormat & srcFormat,
                                   OpalMediaFormat & dstFormat)
{
  OpalMediaFormatList::const_iterator s, d;
#if 0
  //chenyuan softfoundry begin
  //按照主叫codecs list 順序來決定用那一個codec 溝通
  const OpalMediaFormatList & srcFormats = dstFormats2;
  const OpalMediaFormatList & dstFormats = srcFormats2;
  //chenyuan softfoundry end
#else
  const OpalMediaFormatList & srcFormats = srcFormats2;
  const OpalMediaFormatList & dstFormats = dstFormats2;

#endif
  // Search through the supported formats to see if can pass data
  // directly from the given format to a possible one with no transcoders.
  for (d = dstFormats.begin(); d != dstFormats.end(); ++d) {
    for (s = srcFormats.begin(); s != srcFormats.end(); ++s) {
      if (*s == *d && s->GetMediaType() == mediaType && MergeFormats(allFormats, *s, *d, srcFormat, dstFormat))
        return true;
    }
  }

  // Search for a single transcoder to get from a to b
  for (d = dstFormats.begin(); d != dstFormats.end(); ++d) {
    for (s = srcFormats.begin(); s != srcFormats.end(); ++s) {
      if (s->GetMediaType() == mediaType || d->GetMediaType() == mediaType) {
        OpalTranscoderKey search(*s, *d);
        OpalTranscoderList availableTranscoders = OpalTranscoderFactory::GetKeyList();
        for (OpalTranscoderIterator i = availableTranscoders.begin(); i != availableTranscoders.end(); ++i) {
          if (search == *i && MergeFormats(allFormats, *s, *d, srcFormat, dstFormat))
            return true;
        }
      }
    }
  }

  // Last gasp search for a double transcoder to get from a to b
  for (d = dstFormats.begin(); d != dstFormats.end(); ++d) {
    for (s = srcFormats.begin(); s != srcFormats.end(); ++s) {
      if (s->GetMediaType() == mediaType || d->GetMediaType() == mediaType) {
        OpalMediaFormat intermediateFormat;
        if (FindIntermediateFormat(*s, *d, intermediateFormat) &&
            MergeFormats(allFormats, *s, *d, srcFormat, dstFormat))
          return true;
      }
    }
  }

  return PFalse;
}


bool OpalTranscoder::FindIntermediateFormat(const OpalMediaFormat & srcFormat,
                                            const OpalMediaFormat & dstFormat,
                                            OpalMediaFormat & intermediateFormat)
{
  intermediateFormat = OpalMediaFormat();

  OpalTranscoderList availableTranscoders = OpalTranscoderFactory::GetKeyList();
  for (OpalTranscoderIterator find1 = availableTranscoders.begin(); find1 != availableTranscoders.end(); ++find1) {
    if (find1->first == srcFormat) {
      if (find1->second == dstFormat)
        return true;
      for (OpalTranscoderIterator find2 = availableTranscoders.begin(); find2 != availableTranscoders.end(); ++find2) {
        if (find2->first == find1->second && find2->second == dstFormat) {
          OpalMediaFormat probableFormat = find1->second;
          if (probableFormat.Merge(srcFormat) && probableFormat.Merge(dstFormat)) {
            intermediateFormat = probableFormat;
            return true;
          }
        }
      }
    }
  }

  return PFalse;
}


OpalMediaFormatList OpalTranscoder::GetDestinationFormats(const OpalMediaFormat & srcFormat)
{
  OpalMediaFormatList list;

  OpalTranscoderList availableTranscoders = OpalTranscoderFactory::GetKeyList();
  for (OpalTranscoderIterator find = availableTranscoders.begin(); find != availableTranscoders.end(); ++find) {
    if (find->first == srcFormat)
      list += find->second;
  }

  return list;
}


OpalMediaFormatList OpalTranscoder::GetSourceFormats(const OpalMediaFormat & dstFormat)
{
  OpalMediaFormatList list;

  OpalTranscoderList availableTranscoders = OpalTranscoderFactory::GetKeyList();
  for (OpalTranscoderIterator find = availableTranscoders.begin(); find != availableTranscoders.end(); ++find) {
    if (find->second == dstFormat)
      list += find->first;
  }

  return list;
}


OpalMediaFormatList OpalTranscoder::GetPossibleFormats(const OpalMediaFormatList & formats)
{
  OpalMediaFormatList possibleFormats;

  // Run through the formats connection can do directly and calculate all of
  // the possible formats, including ones via a transcoder
  for (OpalMediaFormatList::const_iterator f = formats.begin(); f != formats.end(); ++f) {
    OpalMediaFormat format = *f;
    possibleFormats += format;
    OpalMediaFormatList srcFormats = GetSourceFormats(format);
    for (OpalMediaFormatList::iterator s = srcFormats.begin(); s != srcFormats.end(); ++s) {
      OpalMediaFormatList dstFormats = GetDestinationFormats(*s);
      if (dstFormats.GetSize() > 0) {
        possibleFormats += *s;

        for (OpalMediaFormatList::iterator d = dstFormats.begin(); d != dstFormats.end(); ++d) {
          if (d->IsValid())
            possibleFormats += *d;
        }
      }
    }
  }

  return possibleFormats;
}


/////////////////////////////////////////////////////////////////////////////

OpalFramedTranscoder::OpalFramedTranscoder(const OpalMediaFormat & inputMediaFormat,
                                           const OpalMediaFormat & outputMediaFormat,
                                           PINDEX inputBytes, PINDEX outputBytes)
  : OpalTranscoder(inputMediaFormat, outputMediaFormat)
{
  PINDEX framesPerPacket = outputMediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1);
  inputBytesPerFrame = inputBytes*framesPerPacket;
  outputBytesPerFrame = outputBytes*framesPerPacket;

  PINDEX inMaxTimePerFrame  = inputMediaFormat.GetOptionInteger(OpalAudioFormat::MaxFramesPerPacketOption()) * 
                              inputMediaFormat.GetOptionInteger(OpalAudioFormat::FrameTimeOption());
  PINDEX outMaxTimePerFrame = outputMediaFormat.GetOptionInteger(OpalAudioFormat::MaxFramesPerPacketOption()) * 
                              outputMediaFormat.GetOptionInteger(OpalAudioFormat::FrameTimeOption());

  PINDEX maxPacketTime = PMAX(inMaxTimePerFrame, outMaxTimePerFrame);

  maxOutputDataSize = outputBytesPerFrame * (maxPacketTime / outputMediaFormat.GetOptionInteger(OpalAudioFormat::FrameTimeOption()));
}


static unsigned GreatestCommonDivisor(unsigned a, unsigned b)
{
  return b == 0 ? a : GreatestCommonDivisor(b, a % b);
}


bool OpalFramedTranscoder::UpdateMediaFormats(const OpalMediaFormat & input, const OpalMediaFormat & output)
{
  if (!OpalTranscoder::UpdateMediaFormats(input, output))
    return false;

  unsigned framesPerPacket = outputMediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1);
  unsigned inFrameSize = inputMediaFormat.GetFrameSize();
  unsigned outFrameSize = outputMediaFormat.GetFrameSize();
  unsigned inFrameTime = inputMediaFormat.GetFrameTime();
  unsigned outFrameTime = outputMediaFormat.GetFrameTime();
  unsigned leastCommonMultiple = inFrameTime*outFrameTime/GreatestCommonDivisor(inFrameTime, outFrameTime);
  inputBytesPerFrame = leastCommonMultiple/inFrameTime*inFrameSize*framesPerPacket;
  outputBytesPerFrame = leastCommonMultiple/outFrameTime*outFrameSize*framesPerPacket;

  PINDEX inMaxTimePerFrame  = inputMediaFormat.GetOptionInteger(OpalAudioFormat::MaxFramesPerPacketOption()) * 
                              inputMediaFormat.GetOptionInteger(OpalAudioFormat::FrameTimeOption());
  PINDEX outMaxTimePerFrame = outputMediaFormat.GetOptionInteger(OpalAudioFormat::MaxFramesPerPacketOption()) * 
                              outputMediaFormat.GetOptionInteger(OpalAudioFormat::FrameTimeOption());

  PINDEX maxPacketTime = PMAX(inMaxTimePerFrame, outMaxTimePerFrame);

  maxOutputDataSize = outputBytesPerFrame * (maxPacketTime / outputMediaFormat.GetOptionInteger(OpalAudioFormat::FrameTimeOption()));

  return true;
}


PINDEX OpalFramedTranscoder::GetOptimalDataFrameSize(PBoolean input) const
{
  return input ? inputBytesPerFrame : outputBytesPerFrame;
}


PBoolean OpalFramedTranscoder::Convert(const RTP_DataFrame & input, RTP_DataFrame & output)
{
  if (inputIsRTP || outputIsRTP) {

    const BYTE * inputPtr;
    PINDEX inLen;
    if (inputIsRTP) {
      inputPtr = (const BYTE *)input;
      inLen    = input.GetHeaderSize() + input.GetPayloadSize(); 
    }
    else
    {
      inputPtr = input.GetPayloadPtr();
      inLen    = input.GetPayloadSize(); 
    }

    BYTE * outputPtr;
    PINDEX outLen;
    output.SetPayloadSize(outputBytesPerFrame);
    if (outputIsRTP) {
      outputPtr = output.GetPointer();
      outLen    = output.GetSize();
    }
    else
    {
      outputPtr = output.GetPayloadPtr();
      outLen    = outputBytesPerFrame;
    }

    if (!ConvertFrame(inputPtr, inLen, outputPtr, outLen))
      return PFalse;

    if (!outputIsRTP)
      output.SetPayloadSize(outLen);
    else if (outLen <= RTP_DataFrame::MinHeaderSize)
      output.SetPayloadSize(0);
    else if (outLen <= output.GetHeaderSize())
      output.SetPayloadSize(0);
    else 
      output.SetPayloadSize(outLen - output.GetHeaderSize());

    return PTrue;
  }

  const BYTE * inputPtr = input.GetPayloadPtr();
  PINDEX inputLength = input.GetPayloadSize();

  if (inputLength == 0) {
    output.SetPayloadSize(outputBytesPerFrame);
    return ConvertSilentFrame (output.GetPayloadPtr());
  }

  // set maximum output payload size
  if (!output.SetPayloadSize(maxOutputDataSize))
    return PFalse;

  BYTE * outputPtr = output.GetPayloadPtr();

  PINDEX outLen = 0;

  while (inputLength > 0) {

    PINDEX consumed = inputLength; // PMIN(inputBytesPerFrame, inputLength);
    PINDEX created  = output.GetPayloadSize() - outLen;

    if (!ConvertFrame(inputPtr, consumed, outputPtr, created))
      return PFalse;

    if (consumed == 0 && created == 0)
      break;

    outputPtr   += created;
    outLen      += created;
    inputPtr    += consumed;
    inputLength -= consumed;
  }

  // set actual output payload size
  output.SetPayloadSize(outLen);

  return PTrue;
}

PBoolean OpalFramedTranscoder::ConvertFrame(const BYTE * inputPtr, PINDEX & /*consumed*/, BYTE * outputPtr, PINDEX & /*created*/)
{
  return ConvertFrame(inputPtr, outputPtr);
}

PBoolean OpalFramedTranscoder::ConvertFrame(const BYTE * /*inputPtr*/, BYTE * /*outputPtr*/)
{
  return PFalse;
}

PBoolean OpalFramedTranscoder::ConvertSilentFrame(BYTE *dst)
{
  memset(dst, 0, outputBytesPerFrame);
  return PTrue;
}

/////////////////////////////////////////////////////////////////////////////

OpalStreamedTranscoder::OpalStreamedTranscoder(const OpalMediaFormat & inputMediaFormat,
                                               const OpalMediaFormat & outputMediaFormat,
                                               unsigned inputBits,
                                               unsigned outputBits)
  : OpalTranscoder(inputMediaFormat, outputMediaFormat)
{
  inputBitsPerSample = inputBits;
  outputBitsPerSample = outputBits;
}


PINDEX OpalStreamedTranscoder::GetOptimalDataFrameSize(PBoolean input) const
{
  // For streamed codecs a "frame" is one milliseconds worth of data
  PINDEX size = outputMediaFormat.GetOptionInteger(input ? OpalAudioFormat::TxFramesPerPacketOption()
                                                         : OpalAudioFormat::RxFramesPerPacketOption(), 1);

  size *= outputMediaFormat.GetClockRate()/1000;            // Convert to milliseconds
  size *= input ? inputBitsPerSample : outputBitsPerSample; // Total bits
  size = (size+7)/8;                                        // Total bytes

  return size > 0 ? size : 1;
}


PBoolean OpalStreamedTranscoder::Convert(const RTP_DataFrame & input,
                                     RTP_DataFrame & output)
{
  PINDEX i, bit, mask;

  PINDEX samples = input.GetPayloadSize()*8/inputBitsPerSample;
  PINDEX outputSize = samples*outputBitsPerSample/8;
  output.SetPayloadSize(outputSize);

  // The conversion algorithm for 5,3 & 2 bits per sample needs an extra
  // couple of bytes at the end of the buffer to avoid lots of conditionals
  output.SetMinSize(output.GetHeaderSize()+outputSize+2);

  const BYTE * inputBytes = input.GetPayloadPtr();
  const short * inputWords = (const short *)inputBytes;

  BYTE * outputBytes = output.GetPayloadPtr();
  short * outputWords = (short *)outputBytes;

  switch (inputBitsPerSample) {
    case 16 :
      switch (outputBitsPerSample) {
        case 16 :
          for (i = 0; i < samples; i++)
            *outputWords++ = (short)ConvertOne(*inputWords++);
          break;

        case 8 :
          for (i = 0; i < samples; i++)
            *outputBytes++ = (BYTE)ConvertOne(*inputWords++);
          break;

        case 4 :
          for (i = 0; i < samples; i++) {
            if ((i&1) == 0)
              *outputBytes = (BYTE)ConvertOne(*inputWords++);
            else
              *outputBytes++ |= (BYTE)(ConvertOne(*inputWords++) << 4);
          }
          break;

        case 5 :
        case 3 :
        case 2 :
          bit = 0;
          outputBytes[0] = 0;
          for (i = 0; i < samples; i++) {
            int converted = ConvertOne(*inputWords++);
            outputBytes[0] |= (BYTE)(converted << bit);
            outputBytes[1] |= (BYTE)(converted >> (8-bit));
            bit += outputBitsPerSample;
            if (bit >= 8) {
              outputBytes++;
              outputBytes[1] = 0;
              bit -= 8;
            }
          }
          break;

        default :
          PAssertAlways("Unsupported bit size");
          return PFalse;
      }
      break;

    case 8 :
      switch (outputBitsPerSample) {
        case 16 :
          for (i = 0; i < samples; i++)
            *outputWords++ = (short)ConvertOne(*inputBytes++);
          break;

        case 8 :
          for (i = 0; i < samples; i++)
            *outputBytes++ = (BYTE)ConvertOne(*inputBytes++);
          break;

        case 4 :
          for (i = 0; i < samples; i++) {
            if ((i&1) == 0)
              *outputBytes = (BYTE)ConvertOne(*inputBytes++);
            else
              *outputBytes++ |= (BYTE)(ConvertOne(*inputBytes++) << 4);
          }
          break;

        case 5 :
        case 3 :
        case 2 :
          bit = 0;
          outputBytes[0] = 0;
          for (i = 0; i < samples; i++) {
            int converted = ConvertOne(*inputBytes++);
            outputBytes[0] |= (BYTE)(converted << bit);
            outputBytes[1] |= (BYTE)(converted >> (8-bit));
            bit += outputBitsPerSample;
            if (bit >= 8) {
              outputBytes++;
              outputBytes[1] = 0;
              bit -= 8;
            }
          }
          break;

        default :
          PAssertAlways("Unsupported bit size");
          return PFalse;
      }
      break;

    case 4 :
      switch (outputBitsPerSample) {
        case 16 :
          for (i = 0; i < samples; i++)
            *outputWords++ = (short)ConvertOne((i&1) == 0 ? (*inputBytes & 15) : (*inputBytes++ >> 4));
          break;

        case 8 :
          for (i = 0; i < samples; i++)
            *outputBytes++ = (BYTE)ConvertOne((i&1) == 0 ? (*inputBytes & 15) : (*inputBytes++ >> 4));
          break;

        case 4 :
          for (i = 0; i < samples; i++) {
            if ((i&1) == 0)
              *outputBytes = (BYTE)ConvertOne(*inputBytes & 15);
            else
              *outputBytes++ |= (BYTE)(ConvertOne(*inputBytes++ >> 4) << 4);
          }
          break;

        default :
          PAssertAlways("Unsupported bit size");
          return PFalse;
      }
      break;

    case 5 :
    case 3 :
    case 2 :
      switch (outputBitsPerSample) {
        case 16 :
          bit = 0;
          mask = 0xff>>(8-inputBitsPerSample);
          for (i = 0; i < samples; i++) {
            *outputWords++ = (short)ConvertOne(((inputBytes[0]>>bit)|(inputBytes[1]<<(8-bit)))&mask);
            bit += inputBitsPerSample;
            if (bit >= 8) {
              ++inputBytes;
              bit -= 8;
            }
          }
          break;

        case 8 :
          bit = 0;
          mask = 0xff>>(8-inputBitsPerSample);
          for (i = 0; i < samples; i++) {
            *outputBytes++ = (BYTE)ConvertOne(((inputBytes[0]>>bit)|(inputBytes[1]<<(8-bit)))&mask);
            bit += outputBitsPerSample;
            if (bit >= 8) {
              ++inputBytes;
              bit -= 8;
            }
          }
          break;

        default :
          PAssertAlways("Unsupported bit size");
          return PFalse;
      }
      break;

    default :
      PAssertAlways("Unsupported bit size");
      return PFalse;
  }

  return PTrue;
}


/////////////////////////////////////////////////////////////////////////////

Opal_Linear16Mono_PCM::Opal_Linear16Mono_PCM()
  : OpalStreamedTranscoder(OpalL16_MONO_8KHZ, OpalPCM16, 16, 16)
{
}

int Opal_Linear16Mono_PCM::ConvertOne(int sample) const
{
  unsigned short tmp_sample = (unsigned short)sample;
  
#if PBYTE_ORDER==PLITTLE_ENDIAN
  return (tmp_sample>>8)|(tmp_sample<<8);
#else
  return tmp_sample;
#endif
}


/////////////////////////////////////////////////////////////////////////////

Opal_PCM_Linear16Mono::Opal_PCM_Linear16Mono()
  : OpalStreamedTranscoder(OpalPCM16, OpalL16_MONO_8KHZ, 16, 16)
{
}


int Opal_PCM_Linear16Mono::ConvertOne(int sample) const
{
  unsigned short tmp_sample = (unsigned short)sample;
  
#if PBYTE_ORDER==PLITTLE_ENDIAN
  return (tmp_sample>>8)|(tmp_sample<<8);
#else
  return tmp_sample;
#endif
}


/////////////////////////////////////////////////////////////////////////////
