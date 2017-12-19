/*
 * OpalWavFile.cxx
 *
 * WAV file class with auto-PCM conversion
 *
 * OpenH323 Library
 *
 * Copyright (c) 2002 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23557 $
 * $Author: csoutheren $
 * $Date: 2009-09-30 07:34:00 +0000 (Wed, 30 Sep 2009) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "opalwavfile.h"
#endif

#include <codec/opalwavfile.h>

#include <codec/g711codec.h>



#define new PNEW


OpalWAVFile::OpalWAVFile(unsigned fmt)
  : PWAVFile(fmt)
{
  SetAutoconvert();
}


OpalWAVFile::OpalWAVFile(OpenMode mode, int opts, unsigned fmt)
  : PWAVFile(mode, opts, fmt)
{
  SetAutoconvert();
}


OpalWAVFile::OpalWAVFile(const PFilePath & name, 
                                  OpenMode mode,  /// Mode in which to open the file.
                                       int opts,  /// #OpenOptions enum# for open operation.
                                   unsigned fmt)  /// Type of WAV File to create
  : PWAVFile(name, mode, opts, fmt)
{
  SetAutoconvert();
}


/////////////////////////////////////////////////////////////////////////////////

class PWAVFileConverterXLaw : public PWAVFileConverter
{
  public:
    off_t GetPosition(const PWAVFile & file) const
    {
      off_t pos = file.RawGetPosition();
      return pos * 2;
    }

    PBoolean SetPosition(PWAVFile & file, off_t pos, PFile::FilePositionOrigin origin)
    {
      pos /= 2;
      return file.SetPosition(pos, origin);
    }

    unsigned GetSampleSize(const PWAVFile & /*file*/) const
    {
      return 16;
    }

    off_t GetDataLength(PWAVFile & file)
    {
      return file.RawGetDataLength() * 2;
    }

    PBoolean Read(PWAVFile & file, void * buf, PINDEX len)
    {
      // read the xLaw data
      PINDEX samples = (len / 2);
      BYTE * xlaw = (BYTE *)alloca(samples);
      if (!file.PFile::Read(xlaw, samples))
        return false;

      samples = PMIN(samples, file.PFile::GetLastReadCount());

      // convert to PCM
      short * pcmPtr = (short *)buf;
      for (PINDEX i = 0; i < samples; i++)
        *pcmPtr++ = (short)DecodeSample(xlaw[i]);

      // fake the lastReadCount
      file.SetLastReadCount(samples * 2);

      return true;
    }

    PBoolean Write(PWAVFile & file, const void * buf, PINDEX len)
    {
      PINDEX samples = (len / 2);
      BYTE * xlaw = (BYTE *)alloca(samples);

      // convert from PCM
      short * pcmPtr = (short *)buf;
      for (PINDEX i = 0; i < samples; i++)
        xlaw[i] = (BYTE)EncodeSample(*pcmPtr++);

      if (!file.PFile::Write(xlaw, samples))
        return false;

      // fake the lastWriteCount
      file.SetLastWriteCount(samples * 2);

      return true;
    }

    virtual int DecodeSample(int sample) = 0;
    virtual int EncodeSample(int sample) = 0;
};


class PWAVFileConverterULaw : public PWAVFileConverterXLaw
{
  public:
    unsigned GetFormat(const PWAVFile & /*file*/) const
    {
      return PWAVFile::fmt_uLaw;
    }

    int DecodeSample(int sample)
    {
      return Opal_G711_uLaw_PCM::ConvertSample(sample);
    }

    int EncodeSample(int sample)
    {
      return Opal_PCM_G711_uLaw::ConvertSample(sample);
    }
};


class PWAVFileConverterALaw : public PWAVFileConverterXLaw
{
  public:
    unsigned GetFormat(const PWAVFile & /*file*/) const
    {
      return PWAVFile::fmt_ALaw;
    }

    int DecodeSample(int sample)
    {
      return Opal_G711_ALaw_PCM::ConvertSample(sample);
    }

    int EncodeSample(int sample)
    {
      return Opal_PCM_G711_ALaw::ConvertSample(sample);
    }
};


PFACTORY_CREATE(PWAVFileConverterFactory, PWAVFileConverterULaw, PWAVFile::fmt_uLaw, false);
PWAVFileConverterFactory::Worker<PWAVFileConverterALaw> ALawConverter(PWAVFile::fmt_ALaw);


///////////////////////////////////////////////////////////////////////

class PWAVFileConverterPlugin : public PWAVFileConverter
{
  public:
    PWAVFileConverterPlugin()
    {
      PAssertAlways(PLogicError); // required to compile but should never be used.
    }

    PWAVFileConverterPlugin(
      unsigned wavFormatCode,
      const OpalMediaFormat & fmt,
      const PBYTEArray & /*extendedHeader*/
    )
      : m_wavFormatCode(wavFormatCode)
      , m_mediaFormat(fmt)
      , m_encoder(NULL)
      , m_decoder(NULL)
      , m_partialOffset(0)
      , m_partialCount(0)
    {
    }

    ~PWAVFileConverterPlugin()
    {
      delete m_encoder;
      delete m_decoder;
    }

    unsigned GetFormat(const PWAVFile & /*file*/) const
    {
      return m_wavFormatCode;
    }

    off_t GetPosition(const PWAVFile & file) const
    {
      return file.RawGetPosition()*m_mediaFormat.GetFrameTime()/m_mediaFormat.GetFrameSize();
    }

    PBoolean SetPosition(PWAVFile & file, off_t pos, PFile::FilePositionOrigin origin)
    {
      return file.SetPosition(pos*m_mediaFormat.GetFrameSize()/m_mediaFormat.GetFrameTime(), origin);
    }

    unsigned GetSampleSize(const PWAVFile & /*file*/) const
    {
      return 16;
    }

    off_t GetDataLength(PWAVFile & file)
    {
      return file.RawGetDataLength()*m_mediaFormat.GetFrameTime()/m_mediaFormat.GetFrameSize();
    }

    PBoolean Read(PWAVFile & file, void * buf, PINDEX len)
    {
      if (m_decoder == NULL && (m_decoder = OpalTranscoder::Create(m_mediaFormat, OpalPCM16)) == NULL)
        return false;

      if (m_partialCount == 0) {
        m_encodedFrame.SetPayloadSize(m_mediaFormat.GetFrameSize());
        if (!file.PFile::Read(m_encodedFrame.GetPayloadPtr(), m_encodedFrame.GetPayloadSize()))
          return false;

        if (!m_decoder->Convert(m_encodedFrame, m_decodedFrame))
          return false;

        m_partialCount = m_decodedFrame.GetPayloadSize();
        m_partialOffset = 0;
      }

      PINDEX toCopy = std::min(len, m_partialCount);
      memcpy(buf, m_decodedFrame.GetPayloadPtr()+m_partialOffset, toCopy);
      file.SetLastReadCount(toCopy);
      m_partialOffset += toCopy;
      m_partialCount -= toCopy;
      return true;
    }

    PBoolean Write(PWAVFile & file, const void * buf, PINDEX len)
    {
      if (m_encoder == NULL && (m_encoder = OpalTranscoder::Create(OpalPCM16, m_mediaFormat)) == NULL)
        return false;

      while (len > 0) {
        PINDEX bytesSoFar = m_decodedFrame.GetPayloadSize();
        PINDEX spaceLeft = m_mediaFormat.GetFrameTime()*2 - bytesSoFar;

        if (spaceLeft == 0) {
          if (!m_encoder->Convert(m_decodedFrame, m_encodedFrame))
            return false;

          if (!file.PFile::Write(m_encodedFrame.GetPayloadPtr(), m_encodedFrame.GetPayloadSize()))
            return false;

          spaceLeft = m_mediaFormat.GetFrameTime()*2;
          bytesSoFar = 0;
        }

        PINDEX toCopy = std::min(len, spaceLeft);
        m_decodedFrame.SetPayloadSize(bytesSoFar+toCopy);
        memcpy(m_decodedFrame.GetPayloadPtr()+bytesSoFar, buf, toCopy);
        file.SetLastWriteCount(toCopy);
        len -= toCopy;
      }

      return true;
    }

  private:
    unsigned         m_wavFormatCode;
    OpalMediaFormat  m_mediaFormat;
    OpalTranscoder * m_encoder;
    OpalTranscoder * m_decoder;
    RTP_DataFrame    m_encodedFrame;
    RTP_DataFrame    m_decodedFrame;
    PINDEX           m_partialOffset;
    PINDEX           m_partialCount;
};


class PWAVFileFormatPlugin : public PWAVFileFormat
{
  public:
    PWAVFileFormatPlugin()
    {
      PAssertAlways(PLogicError); // required to compile but should never be used.
    }

    PWAVFileFormatPlugin(
      unsigned wavFormatCode,
      const OpalMediaFormat & fmt,
      const PBYTEArray & extendedHeader
    )
      : m_wavFormatCode(wavFormatCode)
      , m_mediaFormat(fmt)
      , m_extendedHeader(extendedHeader)
    {
    }

    void CreateHeader(PWAV::FMTChunk & wavFmtChunk, PBYTEArray & extendedHeader)
    {
      wavFmtChunk.hdr.len       = sizeof(wavFmtChunk) - sizeof(wavFmtChunk.hdr) + m_extendedHeader.GetSize();
      wavFmtChunk.format        = (WORD)m_wavFormatCode;
      wavFmtChunk.numChannels   = 1;
      wavFmtChunk.sampleRate    = m_mediaFormat.GetClockRate();
      wavFmtChunk.bitsPerSample = 0;
      extendedHeader            = m_extendedHeader;

      UpdateHeader(wavFmtChunk, extendedHeader);
    }

    void UpdateHeader(PWAV::FMTChunk & wavFmtChunk, PBYTEArray & /*extendedHeader*/)
    {
      wavFmtChunk.bytesPerSample = (WORD)(wavFmtChunk.numChannels*m_mediaFormat.GetFrameSize());
      wavFmtChunk.bytesPerSec    = m_mediaFormat.GetFrameSize()*wavFmtChunk.sampleRate/m_mediaFormat.GetFrameTime();
    }

    unsigned GetFormat() const
    {
      return m_wavFormatCode;
    }

    PString GetDescription() const
    {
      return m_mediaFormat;
    }

    PString GetFormatString() const
    {
      return m_mediaFormat;
    }

  private:
    unsigned         m_wavFormatCode;
    OpalMediaFormat  m_mediaFormat;
    PBYTEArray       m_extendedHeader;
};


static BYTE const MSGSM_ExtendedHeader[] = { 0x02, 0x00, 0x40, 0x01 };

static struct PWAVFilePluginValidFormat {
  const char * m_name;
  unsigned     m_code;
  const BYTE * m_extendedHeaderData;
  PINDEX       m_extendedHeaderSize;
} const ValidFormats[] = {
  { "MS-GSM",         PWAVFile::fmt_GSM,  MSGSM_ExtendedHeader, sizeof(MSGSM_ExtendedHeader) },
  { OPAL_G728,        PWAVFile::fmt_G728 },
  { OPAL_G729,        PWAVFile::fmt_G729 },
  { }
};

template <class Factory, class Instance>
class PWAVFilePluginFactory : public PObject, public Factory::WorkerBase
{
  public:
    PWAVFilePluginFactory(const typename Factory::Key_T & key, const PWAVFilePluginValidFormat & info)
      : m_wavFormatCode(info.m_code)
      , m_mediaFormat(info.m_name)
      , m_extendedHeader(info.m_extendedHeaderData, info.m_extendedHeaderSize)
    {
      Factory::WorkerBase::isDynamic = true;
      Factory::Register(key, this);
    }

    virtual typename Factory::Abstract_T * Create(const typename Factory::Key_T & /*key*/) const
    {
      return new Instance(m_wavFormatCode, m_mediaFormat, m_extendedHeader);
    }

  protected:
    unsigned         m_wavFormatCode;
    OpalMediaFormat  m_mediaFormat;
    PBYTEArray       m_extendedHeader;
};


bool OpalWAVFile::AddMediaFormat(const OpalMediaFormat & mediaFormat)
{
  for (const PWAVFilePluginValidFormat * fmt = ValidFormats; fmt->m_name != NULL; ++fmt) {
    if (mediaFormat == fmt->m_name) {
      PWAVFileFormat * formatHandler = PWAVFileFormatByFormatFactory::CreateInstance(mediaFormat.GetName());
      if (formatHandler == NULL) {
        // Deleted by factor infrastructure
        new PWAVFilePluginFactory<PWAVFileFormatByIDFactory, PWAVFileFormatPlugin>(fmt->m_code, *fmt);
        new PWAVFilePluginFactory<PWAVFileFormatByFormatFactory, PWAVFileFormatPlugin>(fmt->m_name, *fmt);
      }

      new PWAVFilePluginFactory<PWAVFileConverterFactory, PWAVFileConverterPlugin>(fmt->m_code, *fmt);
      return true;
    }
  }

  return false;
}


///////////////////////////////////////////////////////////////////////
