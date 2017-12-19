/*
 * mediafmt.cxx
 *
 * Media Format descriptions
 *
 * Open H323 Library
 *
 * Copyright (c) 1999-2000 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23981 $
 * $Author: rjongbloed $
 * $Date: 2010-01-25 08:12:03 +0000 (Mon, 25 Jan 2010) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "mediafmt.h"
#pragma implementation "mediacmd.h"
#endif

#include <opal/buildopts.h>

#include <opal/mediafmt.h>
#include <opal/mediacmd.h>
#include <codec/opalplugin.h>
#include <codec/opalwavfile.h>
#include <codec/opalpluginmgr.h>
#include <ptlib/videoio.h>
#include <ptclib/cypher.h>

#define new PNEW


/////////////////////////////////////////////////////////////////////////////

#define AUDIO_FORMAT(name, rtpPayloadType, encodingName, frameSize, frameTime, rxFrames, txFrames, maxFrames, clock) \
  const OpalAudioFormat & GetOpal##name() \
  { \
    static const OpalAudioFormat name(OPAL_##name, RTP_DataFrame::rtpPayloadType, \
                                    encodingName, frameSize, frameTime, rxFrames, txFrames, maxFrames, clock); \
    return name; \
  }

AUDIO_FORMAT(PCM16,          MaxPayloadType, "",     16, 8,  240,  0, 256,  8000);
AUDIO_FORMAT(PCM16_16KHZ,    MaxPayloadType, "",     32,16,  240,  0, 256, 16000);
AUDIO_FORMAT(PCM16_32KHZ,    MaxPayloadType, "",     64,32,  240,  0, 256, 32000);
AUDIO_FORMAT(PCM16_48KHZ,    MaxPayloadType, "",     96,48,  240,  0, 256, 48000);
AUDIO_FORMAT(L16_MONO_8KHZ,  L16_Mono,       "L16",  16, 8,  240, 30, 256,  8000);
AUDIO_FORMAT(L16_MONO_16KHZ, L16_Mono,       "L16",  32,16,  240, 30, 256, 16000);
AUDIO_FORMAT(L16_MONO_32KHZ, L16_Mono,       "L16",  64,32,  240, 30, 256, 32000);
AUDIO_FORMAT(L16_MONO_48KHZ, L16_Mono,       "L16",  96,48,  240, 30, 256, 48000);
AUDIO_FORMAT(G711_ULAW_64K,  PCMU,           "PCMU",  8, 8,  240, 20, 256,  8000);
AUDIO_FORMAT(G711_ALAW_64K,  PCMA,           "PCMA",  8, 8,  240, 20, 256,  8000);


class OpalStereoAudioFormat : public OpalAudioFormat
{
public:
  OpalStereoAudioFormat(const char * fullName,
                        RTP_DataFrame::PayloadTypes rtpPayloadType,
                        const char * encodingName,
                        PINDEX   frameSize,
                        unsigned frameTime,
                        unsigned rxFrames,
                        unsigned txFrames,
                        unsigned maxFrames,
                        unsigned clockRate)
    : OpalAudioFormat(fullName, rtpPayloadType, encodingName, frameSize, frameTime, rxFrames, txFrames, maxFrames, clockRate)
  {
    SetOptionInteger(OpalAudioFormat::ChannelsOption(), 2);
  }
};


const OpalAudioFormat & GetOpalL16_STEREO_48KHZ()
{
  static OpalStereoAudioFormat stereo48k("Linear-16-Stereo-48kHz",    // name of the meda format
                                         RTP_DataFrame::DynamicBase,  // RTP payload code
                                         "L16",                       // encoding name
                                         16,                          // frame size in bytes
                                         48,                          // frame time (1 ms in clock units)
                                         20,                          // recommended rx frames/packet
                                         20,                          // recommended tx frames/packet
                                         50,                          // max tx frame size
                                         48000);                      // clock rate
  return stereo48k;
};


const OpalMediaFormat & GetOpalRFC2833()
{
  static struct OpalRFC2833MediaFormat : public OpalMediaFormat {
    OpalRFC2833MediaFormat()
      : OpalMediaFormat(OPAL_RFC2833,
                        "userinput",
                        (RTP_DataFrame::PayloadTypes)101,  // Set to this for Cisco compatibility
                        "telephone-event",
                        true,   // Needs jitter
                        32*(1000/50), // bits/sec  (32 bits every 50ms)
                        4,      // bytes/frame
                        150*8,  // 150 millisecond
                        OpalMediaFormat::AudioClockRate)
    {
      AddOption(new OpalMediaOptionString("FMTP", false, "0-16,32,36"));
    }
  } const RFC2833;
  return RFC2833;
}

const OpalMediaFormat & GetOpalCiscoNSE()
{
  static struct OpalCiscoNSEMediaFormat : public OpalMediaFormat {
    OpalCiscoNSEMediaFormat()
      : OpalMediaFormat(OPAL_CISCONSE,
                        "userinput",
                        (RTP_DataFrame::PayloadTypes)100,  // Set to this for Cisco compatibility
                        "NSE",
                        true,   // Needs jitter
                        32*(1000/50), // bits/sec  (32 bits every 50ms)
                        4,      // bytes/frame
                        150*8,  // 150 millisecond
                        OpalMediaFormat::AudioClockRate)
    {
      AddOption(new OpalMediaOptionString("FMTP", false, "192,193"));
    }
  } const CiscoNSE;
  return CiscoNSE;
}

static OpalMediaFormatList & GetMediaFormatsList()
{
  static class OpalMediaFormatListMaster : public OpalMediaFormatList
  {
    public:
      OpalMediaFormatListMaster()
      {
        DisallowDeleteObjects();
      }
  } registeredFormats;

  return registeredFormats;
}


static PMutex & GetMediaFormatsListMutex()
{
  static PMutex mutex;
  return mutex;
}


static void Clamp(OpalMediaFormatInternal & fmt1, const OpalMediaFormatInternal & fmt2, const PString & variableOption, const PString & minOption, const PString & maxOption)
{
  if (fmt1.FindOption(variableOption) == NULL)
    return;

  unsigned value    = fmt1.GetOptionInteger(variableOption, 0);
  unsigned minValue = fmt2.GetOptionInteger(minOption, 0);
  unsigned maxValue = fmt2.GetOptionInteger(maxOption, UINT_MAX);
  if (value < minValue) {
    PTRACE(4, "MediaFormat\tClamped media option \"" << variableOption << "\" from " << value << " to min " << minValue);
    fmt1.SetOptionInteger(variableOption, minValue);
  }
  else if (value > maxValue) {
    PTRACE(4, "MediaFormat\tClamped media option \"" << variableOption << "\" from " << value << " to max " << maxValue);
    fmt1.SetOptionInteger(variableOption, maxValue);
  }
}


/////////////////////////////////////////////////////////////////////////////

OpalMediaOption::OpalMediaOption(const PString & name)
  : m_name(name)
  , m_readOnly(false)
  , m_merge(NoMerge)
{
}


OpalMediaOption::OpalMediaOption(const char * name, bool readOnly, MergeType merge)
  : m_name(name)
  , m_readOnly(readOnly)
  , m_merge(merge)
{
  m_name.Replace("=", "_", true);
}


PObject::Comparison OpalMediaOption::Compare(const PObject & obj) const
{
  const OpalMediaOption * otherOption = PDownCast(const OpalMediaOption, &obj);
  if (otherOption == NULL)
    return GreaterThan;
  return m_name.Compare(otherOption->m_name);
}


bool OpalMediaOption::Merge(const OpalMediaOption & option)
{
  bool assign;
  switch (m_merge) {
    case MinMerge :
      assign = CompareValue(option) == GreaterThan;
      break;

    case MaxMerge :
      assign = CompareValue(option) == LessThan;
      break;

    case EqualMerge :
      if (CompareValue(option) == EqualTo)
        return true;
      PTRACE(2, "MediaFormat\tMerge of media option \"" << m_name << "\" failed, required to be equal.");
      return false;

    case NotEqualMerge :
      if (CompareValue(option) != EqualTo)
        return true;
      PTRACE(2, "MediaFormat\tMerge of media option \"" << m_name << "\" failed, required to be not equal.");
      return false;

    case AlwaysMerge :
      assign = CompareValue(option) != EqualTo;
      break;

    default :
      assign = false;
      break;
  }

  if (assign) {
    PTRACE(4, "MediaFormat\tChanged media option \"" << m_name << "\" from " << *this << " to " << option);
    Assign(option);
  }

  return true;
}


PString OpalMediaOption::AsString() const
{
  PStringStream strm;
  PrintOn(strm);
  return strm;
}


bool OpalMediaOption::FromString(const PString & value)
{
  PStringStream strm;
  strm = value;
  ReadFrom(strm);
  return !strm.fail();
}


///////////////////////////////////////

OpalMediaOptionEnum::OpalMediaOptionEnum(const char * name, bool readOnly)
  : OpalMediaOption(name, readOnly, EqualMerge)
  , m_value(0)
{
}


OpalMediaOptionEnum::OpalMediaOptionEnum(const char * name,
                                         bool readOnly,
                                         const char * const * enumerations,
                                         PINDEX count,
                                         MergeType merge,
                                         PINDEX value)
  : OpalMediaOption(name, readOnly, merge),
    m_enumerations(count, enumerations),
    m_value(value)
{
  if (m_value >= count)
    m_value = count;
}


PObject * OpalMediaOptionEnum::Clone() const
{
  return new OpalMediaOptionEnum(*this);
}


void OpalMediaOptionEnum::PrintOn(ostream & strm) const
{
  if (m_value < m_enumerations.GetSize())
    strm << m_enumerations[m_value];
  else
    strm << psprintf("<%u>", m_value); // Don't output direct to stream so width() works correctly
}


void OpalMediaOptionEnum::ReadFrom(istream & strm)
{
  m_value = m_enumerations.GetSize();

  PINDEX longestMatch = 0;

  PCaselessString str;
  while (strm.peek() != EOF) {
    str += (char)strm.get();

    PINDEX i;
    for (i = 0; i < m_enumerations.GetSize(); i++) {
      if (str == m_enumerations[i].Left(str.GetLength())) {
        longestMatch = i;
        break;
      }
    }
    if (i >= m_enumerations.GetSize()) {
      i = str.GetLength()-1;
      strm.putback(str[i]);
      str.Delete(i, 1);
      break;
    }
  }

  if (str == m_enumerations[longestMatch])
    m_value = longestMatch;
  else {
    for (PINDEX i = str.GetLength(); i > 0; )
      strm.putback(str[--i]);
    strm.setstate(ios::failbit);
  }
}


PObject::Comparison OpalMediaOptionEnum::CompareValue(const OpalMediaOption & option) const
{
  const OpalMediaOptionEnum * otherOption = PDownCast(const OpalMediaOptionEnum, &option);
  if (otherOption == NULL)
    return GreaterThan;

  if (m_value > otherOption->m_value)
    return GreaterThan;

  if (m_value < otherOption->m_value)
    return LessThan;

  return EqualTo;
}


void OpalMediaOptionEnum::Assign(const OpalMediaOption & option)
{
  const OpalMediaOptionEnum * otherOption = PDownCast(const OpalMediaOptionEnum, &option);
  if (otherOption != NULL)
    m_value = otherOption->m_value;
}


void OpalMediaOptionEnum::SetValue(PINDEX value)
{
  if (value < m_enumerations.GetSize())
    m_value = value;
  else {
    m_value = m_enumerations.GetSize();
    PTRACE(1, "MediaFormat\tIllegal value (" << value << ") for OpalMediaOptionEnum");
  }
}


///////////////////////////////////////

OpalMediaOptionString::OpalMediaOptionString(const char * name, bool readOnly)
  : OpalMediaOption(name, readOnly, NoMerge)
{
}


OpalMediaOptionString::OpalMediaOptionString(const char * name, bool readOnly, const PString & value)
  : OpalMediaOption(name, readOnly, NoMerge),
    m_value(value)
{
}


PObject * OpalMediaOptionString::Clone() const
{
  OpalMediaOptionString * newObj = new OpalMediaOptionString(*this);
  newObj->m_value.MakeUnique();
  return newObj;
}


void OpalMediaOptionString::PrintOn(ostream & strm) const
{
  strm << m_value;
}


void OpalMediaOptionString::ReadFrom(istream & strm)
{
  while (isspace(strm.peek())) // Skip whitespace
    strm.get();

  if (strm.peek() != '"')
    strm >> m_value; // If no '"' then read to end of line or eof.
  else {
    // If there was a '"' then assume it is a C style literal string with \ escapes etc
    // The following will set the bad bit if eof occurs before

    char c = ' ';
    PINDEX count = 0;
    PStringStream str;
    while (strm.peek() != EOF) {
      strm.get(c);
      str << c;

      // Keep reading till get a '"' that is not preceded by a '\' that is not itself preceded by a '\'
      if (c == '"' && count > 0 && (str[count] != '\\' || !(count > 1 && str[count-1] == '\\')))
        break;

      count++;
    }

    if (c != '"') {
      // No closing quote, add one and set fail bit.
      strm.setstate(ios::failbit);
      str << '"';
    }

    m_value = PString(PString::Literal, (const char *)str);
  }
}


PObject::Comparison OpalMediaOptionString::CompareValue(const OpalMediaOption & option) const
{
  const OpalMediaOptionString * otherOption = PDownCast(const OpalMediaOptionString, &option);
  if (otherOption == NULL)
    return GreaterThan;

  return m_value.Compare(otherOption->m_value);
}


void OpalMediaOptionString::Assign(const OpalMediaOption & option)
{
  const OpalMediaOptionString * otherOption = PDownCast(const OpalMediaOptionString, &option);
  if (otherOption != NULL) {
    m_value = otherOption->m_value;
    m_value.MakeUnique();
  }
}


void OpalMediaOptionString::SetValue(const PString & value)
{
  m_value = value;
  m_value.MakeUnique();
}


///////////////////////////////////////

OpalMediaOptionOctets::OpalMediaOptionOctets(const char * name, bool readOnly, bool base64)
  : OpalMediaOption(name, readOnly, NoMerge)
  , m_base64(base64)
{
}


OpalMediaOptionOctets::OpalMediaOptionOctets(const char * name, bool readOnly, bool base64, const PBYTEArray & value)
  : OpalMediaOption(name, readOnly, NoMerge)
  , m_value(value)
  , m_base64(base64)
{
}


OpalMediaOptionOctets::OpalMediaOptionOctets(const char * name, bool readOnly, bool base64, const BYTE * data, PINDEX length)
  : OpalMediaOption(name, readOnly, NoMerge)
  , m_value(data, length)
  , m_base64(base64)
{
}


PObject * OpalMediaOptionOctets::Clone() const
{
  OpalMediaOptionOctets * newObj = new OpalMediaOptionOctets(*this);
  newObj->m_value.MakeUnique();
  return newObj;
}


void OpalMediaOptionOctets::PrintOn(ostream & strm) const
{
  if (m_base64)
    strm << PBase64::Encode(m_value);
  else {
    streamsize width = strm.width();
    ios::fmtflags flags = strm.flags();
    char fill = strm.fill();

    int fillLength = width - m_value.GetSize()*2;
    if (fillLength > 0 && (flags&ios_base::adjustfield) == ios::right) {
      for (int i = 0; i < fillLength; i++)
        strm << fill;
    }

    strm << right << hex << setfill('0');
    for (PINDEX i = 0; i < m_value.GetSize(); i++)
      strm << setw(2) << (unsigned)m_value[i];

    if (fillLength > 0 && (flags&ios_base::adjustfield) == ios::left) {
      strm << setw(1);
      for (int i = 0; i < fillLength; i++)
        strm << fill;
    }

    strm.fill(fill);
    strm.flags(flags);
  }
}


void OpalMediaOptionOctets::ReadFrom(istream & strm)
{
  if (m_base64) {
    PString str;
    strm >> str;
    PBase64::Decode(str, m_value);
  }
  else {
    char pair[3];
    pair[2] = '\0';

    PINDEX count = 0;
    PINDEX nibble = 0;

    while (strm.peek() != EOF) {
      char ch = (char)strm.get();
      if (isxdigit(ch))
        pair[nibble++] = ch;
      else if (ch == ' ')
        pair[nibble++] = '0';
      else
        break;

      if (nibble == 2) {
        if (!m_value.SetMinSize(100*((count+1+99)/100)))
          break;
        m_value[count++] = (BYTE)strtoul(pair, NULL, 16);
        nibble = 0;
      }
    }

    // Report error if no legal hex, not empty is OK.
    if (count == 0 && !strm.eof())
      strm.setstate(ios::failbit);

    m_value.SetSize(count);
  }
}


PObject::Comparison OpalMediaOptionOctets::CompareValue(const OpalMediaOption & option) const
{
  const OpalMediaOptionOctets * otherOption = PDownCast(const OpalMediaOptionOctets, &option);
  if (otherOption == NULL)
    return GreaterThan;

  return m_value.Compare(otherOption->m_value);
}


void OpalMediaOptionOctets::Assign(const OpalMediaOption & option)
{
  const OpalMediaOptionOctets * otherOption = PDownCast(const OpalMediaOptionOctets, &option);
  if (otherOption != NULL) {
    m_value = otherOption->m_value;
    m_value.MakeUnique();
  }
}


void OpalMediaOptionOctets::SetValue(const PBYTEArray & value)
{
  m_value = value;
  m_value.MakeUnique();
}


void OpalMediaOptionOctets::SetValue(const BYTE * data, PINDEX length)
{
  m_value = PBYTEArray(data, length);
}


/////////////////////////////////////////////////////////////////////////////

const PString & OpalMediaFormat::NeedsJitterOption()   { static PString s = PLUGINCODEC_OPTION_NEEDS_JITTER;    return s; }
const PString & OpalMediaFormat::MaxFrameSizeOption()  { static PString s = PLUGINCODEC_OPTION_MAX_FRAME_SIZE;  return s; }
const PString & OpalMediaFormat::FrameTimeOption()     { static PString s = PLUGINCODEC_OPTION_FRAME_TIME;      return s; }
const PString & OpalMediaFormat::ClockRateOption()     { static PString s = PLUGINCODEC_OPTION_CLOCK_RATE;      return s; }
const PString & OpalMediaFormat::MaxBitRateOption()    { static PString s = PLUGINCODEC_OPTION_MAX_BIT_RATE;    return s; }
const PString & OpalMediaFormat::TargetBitRateOption() { static PString s = PLUGINCODEC_OPTION_TARGET_BIT_RATE; return s; }

#if OPAL_H323
const PString & OpalMediaFormat::MediaPacketizationOption()  { static PString s = PLUGINCODEC_MEDIA_PACKETIZATION;  return s; }
const PString & OpalMediaFormat::MediaPacketizationsOption() { static PString s = PLUGINCODEC_MEDIA_PACKETIZATIONS; return s; }
#endif

const PString & OpalMediaFormat::ProtocolOption()        { static PString s = "Protocol";           return s; }
const PString & OpalMediaFormat::MaxTxPacketSizeOption() { static PString s = "Max Tx Packet Size"; return s; }

OpalMediaFormat::OpalMediaFormat(OpalMediaFormatInternal * info)
  : m_info(NULL)
{
  Construct(info);
}


OpalMediaFormat::OpalMediaFormat(RTP_DataFrame::PayloadTypes pt, unsigned clockRate, const char * name, const char * protocol)
  : m_info(NULL)
{
  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  const OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  OpalMediaFormatList::const_iterator fmt = registeredFormats.FindFormat(pt, clockRate, name, protocol);
  if (fmt != registeredFormats.end())
    *this = *fmt;
}


OpalMediaFormat::OpalMediaFormat(const char * wildcard)
  : m_info(NULL)
{
  operator=(PString(wildcard));
}


OpalMediaFormat::OpalMediaFormat(const PString & wildcard)
  : m_info(NULL)
{
  operator=(wildcard);
}


OpalMediaFormat::OpalMediaFormat(const char * fullName,
                                 const OpalMediaType & mediaType,
                                 RTP_DataFrame::PayloadTypes pt,
                                 const char * en,
                                 PBoolean     nj,
                                 unsigned bw,
                                 PINDEX   fs,
                                 unsigned ft,
                                 unsigned cr,
                                 time_t ts)
{
  Construct(new OpalMediaFormatInternal(fullName, mediaType, pt, en, nj, bw, fs, ft,cr, ts));
}


void OpalMediaFormat::Construct(OpalMediaFormatInternal * info)
{
  if (info == NULL)
    return;

  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  OpalMediaFormatList::const_iterator fmt = registeredFormats.FindFormat(info->formatName);
  if (fmt != registeredFormats.end()) {
    *this = *fmt;
    delete info;
  }
  else {
    m_info = info;
    registeredFormats.OpalMediaFormatBaseList::Append(this);
  }
}


OpalMediaFormat & OpalMediaFormat::operator=(RTP_DataFrame::PayloadTypes pt)
{
  PWaitAndSignal m(_mutex);

  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  const OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  OpalMediaFormatList::const_iterator fmt = registeredFormats.FindFormat(pt);
  if (fmt == registeredFormats.end())
    *this = OpalMediaFormat();
  else if (this != &*fmt)
    *this = *fmt;

  return *this;
}


OpalMediaFormat & OpalMediaFormat::operator=(const char * wildcard)
{
  PWaitAndSignal m(_mutex);
  return operator=(PString(wildcard));
}


OpalMediaFormat & OpalMediaFormat::operator=(const PString & wildcard)
{
  PWaitAndSignal m(_mutex);
  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  const OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  OpalMediaFormatList::const_iterator fmt = registeredFormats.FindFormat(wildcard);
  if (fmt == registeredFormats.end())
    *this = OpalMediaFormat();
  else
    *this = *fmt;

  return *this;
}


void OpalMediaFormat::CloneContents(const OpalMediaFormat * c)
{
  PWaitAndSignal m(_mutex);

  m_info = (OpalMediaFormatInternal *)c->m_info->Clone();
  m_info->options.MakeUnique();
}


void OpalMediaFormat::CopyContents(const OpalMediaFormat & c)
{
  PWaitAndSignal m(_mutex);
  m_info = c.m_info;
}


void OpalMediaFormat::DestroyContents()
{
  PWaitAndSignal m(_mutex);
  delete m_info;
}


PObject * OpalMediaFormat::Clone() const
{
  PWaitAndSignal m(_mutex);
  return new OpalMediaFormat(*this);
}


PObject::Comparison OpalMediaFormat::Compare(const PObject & obj) const
{
  PWaitAndSignal m(_mutex);
  PAssert(PIsDescendant(&obj, OpalMediaFormat), PInvalidCast);
  const OpalMediaFormat & other = (const OpalMediaFormat &)obj;
  if (m_info == NULL)
    return other.m_info == NULL ? EqualTo : LessThan;
  if (other.m_info == NULL)
    return m_info == NULL ? EqualTo : GreaterThan;
  return m_info->formatName.Compare(other.m_info->formatName);
}


void OpalMediaFormat::PrintOn(ostream & strm) const
{
  PWaitAndSignal m(_mutex);
  if (m_info != NULL)
    strm << *m_info;
}


void OpalMediaFormat::ReadFrom(istream & strm)
{
  PWaitAndSignal m(_mutex);
  char fmt[100];
  strm >> fmt;
  operator=(fmt);
}


bool OpalMediaFormat::ToNormalisedOptions()
{
  PWaitAndSignal m(_mutex);
  MakeUnique();
  return m_info != NULL && m_info->ToNormalisedOptions();
}


bool OpalMediaFormat::ToCustomisedOptions()
{
  PWaitAndSignal m(_mutex);
  MakeUnique();
  return m_info != NULL && m_info->ToCustomisedOptions();
}


bool OpalMediaFormat::Merge(const OpalMediaFormat & mediaFormat)
{
  PWaitAndSignal m(_mutex);
  MakeUnique();
  return m_info != NULL && mediaFormat.m_info != NULL && m_info->Merge(*mediaFormat.m_info);
}


OpalMediaFormatList OpalMediaFormat::GetMatchingRegisteredMediaFormats(RTP_DataFrame::PayloadTypes rtpPayloadType,
                                                                       const unsigned clockRate,
                                                                       const char * rtpEncodingName,
                                                                       const char * protocol)
{
  OpalMediaFormatList matches;

  PWaitAndSignal mutex(GetMediaFormatsListMutex());

  const OpalMediaFormatList & registeredFormats = GetMediaFormatsList();
  for (OpalMediaFormatList::const_iterator format = registeredFormats.FindFormat(rtpPayloadType, clockRate, rtpEncodingName, protocol);
       format != registeredFormats.end();
         format = registeredFormats.FindFormat(rtpPayloadType, clockRate, rtpEncodingName, protocol, ++format))
  {//CHEN YUAN Avoid the dead loop chenyuan
    if (format!=registeredFormats.end()){
      matches += *format;break;
    }
  }

  return matches;
}


OpalMediaFormatList OpalMediaFormat::GetAllRegisteredMediaFormats()
{
  OpalMediaFormatList copy;
  GetAllRegisteredMediaFormats(copy);
  return copy;
}


void OpalMediaFormat::GetAllRegisteredMediaFormats(OpalMediaFormatList & copy)
{
  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  const OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  for (OpalMediaFormatList::const_iterator format = registeredFormats.begin(); format != registeredFormats.end(); ++format)
    copy += *format;
}


bool OpalMediaFormat::SetRegisteredMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  for (OpalMediaFormatList::iterator format = registeredFormats.begin(); format != registeredFormats.end(); ++format) {
    if (*format == mediaFormat) {
      /* Yes, this looks a little odd as we just did equality above and seem to
         be assigning the left hand side with exactly the same value. But what
         is really happening is the above only compares the name, and below
         copies all of the attributes (OpalMediaFormatOtions) across. */
      *format = mediaFormat;
      return true;
    }
  }

  return false;
}


/////////////////////////////////////////////////////////////////////////////

OpalMediaFormatInternal::OpalMediaFormatInternal(const char * fullName,
                                                 const OpalMediaType & _mediaType,
                                                 RTP_DataFrame::PayloadTypes pt,
                                                 const char * en,
                                                 PBoolean     nj,
                                                 unsigned bw,
                                                 PINDEX   fs,
                                                 unsigned ft,
                                                 unsigned cr,
                                                 time_t ts)
  : formatName(fullName), mediaType(_mediaType), forceIsTransportable(false)
{
  codecVersionTime = ts;
  rtpPayloadType = pt;
  rtpEncodingName = en;

  if (nj)
    AddOption(new OpalMediaOptionBoolean(OpalMediaFormat::NeedsJitterOption(), true, OpalMediaOption::OrMerge, true));

  if (bw > 0)
    AddOption(new OpalMediaOptionUnsigned(OpalMediaFormat::MaxBitRateOption(), true, OpalMediaOption::MinMerge, bw, 100));

  if (fs > 0)
    AddOption(new OpalMediaOptionUnsigned(OpalMediaFormat::MaxFrameSizeOption(), true, OpalMediaOption::NoMerge, fs));

  if (ft > 0)
    AddOption(new OpalMediaOptionUnsigned(OpalMediaFormat::FrameTimeOption(), true, OpalMediaOption::NoMerge, ft));

  if (cr > 0)
    AddOption(new OpalMediaOptionUnsigned(OpalMediaFormat::ClockRateOption(), true, OpalMediaOption::NoMerge, cr));

  AddOption(new OpalMediaOptionString(OpalMediaFormat::ProtocolOption(), true));

  // assume non-dynamic payload types are correct and do not need deconflicting
  if (rtpPayloadType < RTP_DataFrame::DynamicBase || rtpPayloadType >= RTP_DataFrame::MaxPayloadType) {
    if (rtpPayloadType == RTP_DataFrame::MaxPayloadType && 
        rtpEncodingName.GetLength() > 0 &&
        rtpEncodingName[0] == '+') {
      forceIsTransportable = true;
      rtpEncodingName = rtpEncodingName.Mid(1);
    }
    return;
  }

  PWaitAndSignal mutex(GetMediaFormatsListMutex());
  OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

  // Search for conflicting RTP Payload Type, collecting in use payload types along the way
  bool inUse[RTP_DataFrame::MaxPayloadType];
  memset(inUse, 0, sizeof(inUse));

  OpalMediaFormat * match = NULL;
  for (OpalMediaFormatList::iterator format = registeredFormats.begin(); format != registeredFormats.end(); ++format) {
    RTP_DataFrame::PayloadTypes thisPayloadType = format->GetPayloadType();
    if (thisPayloadType == rtpPayloadType)
      match = &*format;
    if (thisPayloadType < RTP_DataFrame::MaxPayloadType)
      inUse[thisPayloadType] = true;
  }

  if (match == NULL)
    return; // No conflict

  // Determine next unused payload type, if all the dynamic ones are allocated then
  // we start downward toward the well known values.
  int nextUnused = RTP_DataFrame::DynamicBase;
  while (inUse[nextUnused]) {
    if (nextUnused < RTP_DataFrame::DynamicBase)
      --nextUnused;
    else if (++nextUnused >= RTP_DataFrame::MaxPayloadType)
      nextUnused = RTP_DataFrame::DynamicBase-1;
  }

  match->SetPayloadType((RTP_DataFrame::PayloadTypes)nextUnused);
}


PObject * OpalMediaFormatInternal::Clone() const
{
  PWaitAndSignal m1(media_format_mutex);
  return new OpalMediaFormatInternal(*this);
}


bool OpalMediaFormatInternal::Merge(const OpalMediaFormatInternal & mediaFormat)
{
  PTRACE(4, "MediaFormat\tMerging " << mediaFormat << " into " << *this);

  PWaitAndSignal m1(media_format_mutex);
  PWaitAndSignal m2(mediaFormat.media_format_mutex);

  for (PINDEX i = 0; i < options.GetSize(); i++) {
    OpalMediaOption & opt = options[i];
    PString name = opt.GetName();
    OpalMediaOption * option = mediaFormat.FindOption(opt.GetName());
    if (option == NULL) {
      PTRACE_IF(2, formatName == mediaFormat.formatName, "MediaFormat\tCannot merge unmatched option " << opt.GetName());
    }
    else 
    {
      PAssert(option->GetName() == opt.GetName(), "find returned bad name");
      if (!opt.Merge(*option))
        return false;
    }
  }

  return true;
}


bool OpalMediaFormatInternal::ToNormalisedOptions()
{
  return true;
}


bool OpalMediaFormatInternal::ToCustomisedOptions()
{
  return true;
}


bool OpalMediaFormatInternal::IsValid() const
{
  return rtpPayloadType < RTP_DataFrame::IllegalPayloadType && !formatName.IsEmpty();
}


bool OpalMediaFormatInternal::IsTransportable() const
{
  if (forceIsTransportable)
    return true;

  if (rtpPayloadType >= RTP_DataFrame::MaxPayloadType)
    return false;

  if (rtpPayloadType < RTP_DataFrame::LastKnownPayloadType)
    return true;

  return !rtpEncodingName.IsEmpty();
}


PStringToString OpalMediaFormatInternal::GetOptions() const
{
  PWaitAndSignal m1(media_format_mutex);
  PStringToString dict;
  for (PINDEX i = 0; i < options.GetSize(); i++)
    dict.SetAt(options[i].GetName(), options[i].AsString());
  return dict;
}


bool OpalMediaFormatInternal::GetOptionValue(const PString & name, PString & value) const
{
  PWaitAndSignal m(media_format_mutex);
  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  value = option->AsString();
  return true;
}


bool OpalMediaFormatInternal::SetOptionValue(const PString & name, const PString & value)
{
  PWaitAndSignal m(media_format_mutex);

  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  return option->FromString(value);
}


template <class OptionType, typename ValueType>
static ValueType GetOptionOfType(const OpalMediaFormatInternal & format, const PString & name, ValueType dflt)
{
  OpalMediaOption * option = format.FindOption(name);
  if (option == NULL)
    return dflt;

  OptionType * typedOption = dynamic_cast<OptionType *>(option);
  if (typedOption != NULL)
    return typedOption->GetValue();

  PTRACE(1, "MediaFormat\tInvalid type for getting option " << name << " in " << format);
  PAssertAlways(PInvalidCast);
  return dflt;
}


template <class OptionType, typename ValueType>
static bool SetOptionOfType(OpalMediaFormatInternal & format, const PString & name, ValueType value)
{
  OpalMediaOption * option = format.FindOption(name);
  if (option == NULL)
    return false;

  OptionType * typedOption = dynamic_cast<OptionType *>(option);
  if (typedOption != NULL) {
    typedOption->SetValue(value);
    return true;
  }

  PTRACE(1, "MediaFormat\tInvalid type for setting option " << name << " in " << format);
  PAssertAlways(PInvalidCast);
  return false;
}


bool OpalMediaFormatInternal::GetOptionBoolean(const PString & name, bool dflt) const
{
  PWaitAndSignal m(media_format_mutex);
  const OpalMediaOptionEnum * optEnum = dynamic_cast<const OpalMediaOptionEnum *>(FindOption(name));
  if (optEnum != NULL && optEnum->GetEnumerations().GetSize() == 2)
    return optEnum->GetValue() != 0;

  return GetOptionOfType<OpalMediaOptionBoolean, bool>(*this, name, dflt);
}


bool OpalMediaFormatInternal::SetOptionBoolean(const PString & name, bool value)
{
  PWaitAndSignal m(media_format_mutex);
  OpalMediaOptionEnum * optEnum = dynamic_cast<OpalMediaOptionEnum *>(FindOption(name));
  if (optEnum != NULL && optEnum->GetEnumerations().GetSize() == 2) {
    optEnum->SetValue(value);
    return true;
  }

  return SetOptionOfType<OpalMediaOptionBoolean, bool>(*this, name, value);
}


int OpalMediaFormatInternal::GetOptionInteger(const PString & name, int dflt) const
{
  PWaitAndSignal m(media_format_mutex);
  OpalMediaOptionUnsigned * optUnsigned = dynamic_cast<OpalMediaOptionUnsigned *>(FindOption(name));
  if (optUnsigned != NULL)
    return optUnsigned->GetValue();

  return GetOptionOfType<OpalMediaOptionInteger, int>(*this, name, dflt);
}


bool OpalMediaFormatInternal::SetOptionInteger(const PString & name, int value)
{
  PWaitAndSignal m(media_format_mutex);
  OpalMediaOptionUnsigned * optUnsigned = dynamic_cast<OpalMediaOptionUnsigned *>(FindOption(name));
  if (optUnsigned != NULL) {
    optUnsigned->SetValue(value);
    return true;
  }

  return SetOptionOfType<OpalMediaOptionInteger, int>(*this, name, value);
}


double OpalMediaFormatInternal::GetOptionReal(const PString & name, double dflt) const
{
  PWaitAndSignal m(media_format_mutex);
  return GetOptionOfType<OpalMediaOptionReal, double>(*this, name, dflt);
}


bool OpalMediaFormatInternal::SetOptionReal(const PString & name, double value)
{
  PWaitAndSignal m(media_format_mutex);
  return SetOptionOfType<OpalMediaOptionReal, double>(*this, name, value);
}


PINDEX OpalMediaFormatInternal::GetOptionEnum(const PString & name, PINDEX dflt) const
{
  PWaitAndSignal m(media_format_mutex);
  return GetOptionOfType<OpalMediaOptionEnum, PINDEX>(*this, name, dflt);
}


bool OpalMediaFormatInternal::SetOptionEnum(const PString & name, PINDEX value)
{
  PWaitAndSignal m(media_format_mutex);
  return SetOptionOfType<OpalMediaOptionEnum, PINDEX>(*this, name, value);
}


PString OpalMediaFormatInternal::GetOptionString(const PString & name, const PString & dflt) const
{
  PWaitAndSignal m(media_format_mutex);
  return GetOptionOfType<OpalMediaOptionString, PString>(*this, name, dflt);
}


bool OpalMediaFormatInternal::SetOptionString(const PString & name, const PString & value)
{
  PWaitAndSignal m(media_format_mutex);
  return SetOptionOfType<OpalMediaOptionString, PString>(*this, name, value);
}


bool OpalMediaFormatInternal::GetOptionOctets(const PString & name, PBYTEArray & octets) const
{
  PWaitAndSignal m(media_format_mutex);
  OpalMediaOption * option = FindOption(name);
  if (option == NULL)
    return false;

  octets = PDownCast(OpalMediaOptionOctets, option)->GetValue();
  return true;
}


bool OpalMediaFormatInternal::SetOptionOctets(const PString & name, const PBYTEArray & octets)
{
  PWaitAndSignal m(media_format_mutex);
  return SetOptionOfType<OpalMediaOptionOctets, const PBYTEArray &>(*this, name, octets);
}


bool OpalMediaFormatInternal::SetOptionOctets(const PString & name, const BYTE * data, PINDEX length)
{
  PWaitAndSignal m(media_format_mutex);
  return SetOptionOfType<OpalMediaOptionOctets, const PBYTEArray &>(*this, name, PBYTEArray(data, length));
}


bool OpalMediaFormatInternal::AddOption(OpalMediaOption * option, PBoolean overwrite)
{
  PWaitAndSignal m(media_format_mutex);
  if (PAssertNULL(option) == NULL)
    return false;

  PINDEX index = options.GetValuesIndex(*option);
  if (index != P_MAX_INDEX) {
    if (!overwrite) {
      delete option;
      return false;
    }

    options.RemoveAt(index);
  }

  options.Append(option);
  return true;
}


class OpalMediaOptionSearchArg : public OpalMediaOption
{
public:
  OpalMediaOptionSearchArg(const PString & name) : OpalMediaOption(name) { }
  virtual Comparison CompareValue(const OpalMediaOption &) const { return EqualTo; }
  virtual void Assign(const OpalMediaOption &) { }
};

OpalMediaOption * OpalMediaFormatInternal::FindOption(const PString & name) const
{
  PWaitAndSignal m(media_format_mutex);
  OpalMediaOptionSearchArg search(name);
  PINDEX index = options.GetValuesIndex(search);
  if (index == P_MAX_INDEX)
    return NULL;

  PAssert(options[index].GetName() == name, "OpalMediaOption name mismatch");

  return &options[index];
}


bool OpalMediaFormatInternal::IsValidForProtocol(const PString & protocol) const
{
  PWaitAndSignal m(media_format_mutex);

  // the protocol is only valid for SIP if the RTP name is not NULL
  if (protocol *= "sip")
    return (rtpEncodingName != NULL) || (forceIsTransportable);

  return true;
}


void OpalMediaFormatInternal::PrintOn(ostream & strm) const
{
  PINDEX i;
  PWaitAndSignal m(media_format_mutex);

  if (strm.width() != -1) {
    strm << formatName;
    return;
  }

  PINDEX TitleWidth = 20;
  for (i = 0; i < options.GetSize(); i++) {
    PINDEX width =options[i].GetName().GetLength();
    if (width > TitleWidth)
      TitleWidth = width;
  }

  strm << right << setw(TitleWidth) <<   "Format Name" << left << "       = " << formatName << '\n'
       << right << setw(TitleWidth) <<    "Media Type" << left << "       = " << mediaType << '\n'
       << right << setw(TitleWidth) <<  "Payload Type" << left << "       = " << rtpPayloadType << '\n'
       << right << setw(TitleWidth) << "Encoding Name" << left << "       = " << rtpEncodingName << '\n';
  for (i = 0; i < options.GetSize(); i++) {
    const OpalMediaOption & option = options[i];
    strm << right << setw(TitleWidth) << option.GetName() << " (R/" << (option.IsReadOnly() ? 'O' : 'W')
         << ") = " << left << setw(10) << option;

#if OPAL_SIP
    if (!option.GetFMTPName().IsEmpty())
      strm << "  FMTP name: " << option.GetFMTPName() << " (" << option.GetFMTPDefault() << ')';
#endif // OPAL_SIP

#if OPAL_H323
    const OpalMediaOption::H245GenericInfo & genericInfo = option.GetH245Generic();
    if (genericInfo.mode != OpalMediaOption::H245GenericInfo::None) {
      strm << "  H.245 Ordinal: " << genericInfo.ordinal
           << ' ' << (genericInfo.mode == OpalMediaOption::H245GenericInfo::Collapsing ? "Collapsing" : "Non-Collapsing");
      if (!genericInfo.excludeTCS)
        strm << " TCS";
      if (!genericInfo.excludeOLC)
        strm << " OLC";
      if (!genericInfo.excludeReqMode)
        strm << " RM";
    }
#endif // OPAL_H323

    // Show the type of the option: Boolean, Unsigned, String, etc.
    if (PIsDescendant(&option, OpalMediaOptionBoolean))
      strm << " Boolean";
    else if (PIsDescendant(&option, OpalMediaOptionUnsigned))
#if OPAL_H323
      switch (genericInfo.integerType) {
        default :
        case OpalMediaOption::H245GenericInfo::UnsignedInt :
          strm << " UnsignedInt";
          break;
        case OpalMediaOption::H245GenericInfo::Unsigned32 :
          strm << " Unsigned32";
          break;
        case OpalMediaOption::H245GenericInfo::BooleanArray :
          strm << " BooleanArray";
          break;
      }
#else
      strm << " UnsignedInt";
#endif // OPAL_H323
    else if (PIsDescendant(&option, OpalMediaOptionOctets))
      strm << " OctetString";
    else if (PIsDescendant(&option, OpalMediaOptionString))
      strm << " String";
    else if (PIsDescendant(&option, OpalMediaOptionEnum))
      strm << " Enum";
    else
      strm << " Unknown";

    strm << '\n';
  }
  strm << endl;
}

///////////////////////////////////////////////////////////////////////////////

const PString & OpalAudioFormat::RxFramesPerPacketOption() { static PString s = PLUGINCODEC_OPTION_RX_FRAMES_PER_PACKET; return s; }
const PString & OpalAudioFormat::TxFramesPerPacketOption() { static PString s = PLUGINCODEC_OPTION_TX_FRAMES_PER_PACKET; return s; }
const PString & OpalAudioFormat::MaxFramesPerPacketOption(){ static PString s = "Max Frames Per Packet"; return s; }
const PString & OpalAudioFormat::ChannelsOption()          { static PString s = "Channels"; return s; }

OpalAudioFormat::OpalAudioFormat(const char * fullName,
                                 RTP_DataFrame::PayloadTypes rtpPayloadType,
                                 const char * encodingName,
                                 PINDEX   frameSize,
                                 unsigned frameTime,
                                 unsigned rxFrames,
                                 unsigned txFrames,
                                 unsigned maxFrames,
                                 unsigned clockRate,
                                 time_t timeStamp)
{
  Construct(new OpalAudioFormatInternal(fullName,
                                        rtpPayloadType,
                                        encodingName,
                                        frameSize,
                                        frameTime,
                                        rxFrames,
                                        txFrames,
                                        maxFrames,
                                        clockRate,
                                        timeStamp));
}


OpalAudioFormatInternal::OpalAudioFormatInternal(const char * fullName,
                                                 RTP_DataFrame::PayloadTypes rtpPayloadType,
                                                 const char * encodingName,
                                                 PINDEX   frameSize,
                                                 unsigned frameTime,
                                                 unsigned rxFrames,
                                                 unsigned txFrames,
                                                 unsigned maxFrames,
                                                 unsigned clockRate,
                                                 time_t timeStamp)
  : OpalMediaFormatInternal(fullName,
                            "audio",
                            rtpPayloadType,
                            encodingName,
                            true,
                            8*frameSize*clockRate/frameTime,  // bits per second = 8*frameSize * framesPerSecond
                            frameSize,
                            frameTime,
                            clockRate,
                            timeStamp)
{
  if (rxFrames > 0)
    AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::RxFramesPerPacketOption(), false, OpalMediaOption::NoMerge,  rxFrames, 1, maxFrames));
  if (txFrames > 0)
    AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::TxFramesPerPacketOption(), false, OpalMediaOption::NoMerge, txFrames, 1, maxFrames));

  AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::MaxFramesPerPacketOption(), true,  OpalMediaOption::NoMerge,  maxFrames));
  AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::ChannelsOption(),           false, OpalMediaOption::NoMerge,  1, 1, 5));
}


PObject * OpalAudioFormatInternal::Clone() const
{
  PWaitAndSignal m(media_format_mutex);
  return new OpalAudioFormatInternal(*this);
}


bool OpalAudioFormatInternal::Merge(const OpalMediaFormatInternal & mediaFormat)
{
  PWaitAndSignal m1(media_format_mutex);
  PWaitAndSignal m2(mediaFormat.media_format_mutex);

  if (!OpalMediaFormatInternal::Merge(mediaFormat))
    return false;

  Clamp(*this, mediaFormat, OpalAudioFormat::TxFramesPerPacketOption(), PString::Empty(), OpalAudioFormat::RxFramesPerPacketOption());
  return true;
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

const PString & OpalVideoFormat::FrameWidthOption()               { static PString s = PLUGINCODEC_OPTION_FRAME_WIDTH;               return s; }
const PString & OpalVideoFormat::FrameHeightOption()              { static PString s = PLUGINCODEC_OPTION_FRAME_HEIGHT;              return s; }
const PString & OpalVideoFormat::MinRxFrameWidthOption()          { static PString s = PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH;        return s; }
const PString & OpalVideoFormat::MinRxFrameHeightOption()         { static PString s = PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT;       return s; }
const PString & OpalVideoFormat::MaxRxFrameWidthOption()          { static PString s = PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH;        return s; }
const PString & OpalVideoFormat::MaxRxFrameHeightOption()         { static PString s = PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT;       return s; }
const PString & OpalVideoFormat::TemporalSpatialTradeOffOption()  { static PString s = PLUGINCODEC_OPTION_TEMPORAL_SPATIAL_TRADE_OFF;return s; }
const PString & OpalVideoFormat::TxKeyFramePeriodOption()         { static PString s = PLUGINCODEC_OPTION_TX_KEY_FRAME_PERIOD;       return s; }
const PString & OpalVideoFormat::RateControlEnableOption()        { static PString s = "Rate Control Enable";                        return s; }
const PString & OpalVideoFormat::RateControllerOption()           { static PString s = "Rate Controller";                            return s; }
const PString & OpalVideoFormat::ContentRoleOption()              { static PString s = "Content Role";                               return s; }
const PString & OpalVideoFormat::ContentRoleMaskOption()          { static PString s = "Content Role Mask";                          return s; }

OpalVideoFormat::OpalVideoFormat(const char * fullName,
                                 RTP_DataFrame::PayloadTypes rtpPayloadType,
                                 const char * encodingName,
                                 unsigned maxFrameWidth,
                                 unsigned maxFrameHeight,
                                 unsigned maxFrameRate,
                                 unsigned maxBitRate,
                                 time_t timeStamp)
{
  Construct(new OpalVideoFormatInternal(fullName,
                                        rtpPayloadType,
                                        encodingName,
                                        maxFrameWidth,
                                        maxFrameHeight,
                                        maxFrameRate,
                                        maxBitRate,
                                        timeStamp));
}


OpalVideoFormatInternal::OpalVideoFormatInternal(const char * fullName,
                                                 RTP_DataFrame::PayloadTypes rtpPayloadType,
                                                 const char * encodingName,
                                                 unsigned maxFrameWidth,
                                                 unsigned maxFrameHeight,
                                                 unsigned maxFrameRate,
                                                 unsigned maxBitRate,
                                                 time_t timeStamp)
  : OpalMediaFormatInternal(fullName,
                            "video",
                            rtpPayloadType,
                            encodingName,
                            PFalse,
                            maxBitRate,
                            0,
                            OpalMediaFormat::VideoClockRate/maxFrameRate,
                            OpalMediaFormat::VideoClockRate,
                            timeStamp)
{
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::FrameWidthOption(),               false, OpalMediaOption::AlwaysMerge, PVideoFrameInfo::CIFWidth,   16,  32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::FrameHeightOption(),              false, OpalMediaOption::AlwaysMerge, PVideoFrameInfo::CIFHeight,  16,  32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::MinRxFrameWidthOption(),          false, OpalMediaOption::MaxMerge,    PVideoFrameInfo::SQCIFWidth, 16,  32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::MinRxFrameHeightOption(),         false, OpalMediaOption::MaxMerge,    PVideoFrameInfo::SQCIFHeight,16,  32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::MaxRxFrameWidthOption(),          false, OpalMediaOption::MinMerge,    maxFrameWidth,               16,  32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::MaxRxFrameHeightOption(),         false, OpalMediaOption::MinMerge,    maxFrameHeight,              16,  32767));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::TargetBitRateOption(),            false, OpalMediaOption::AlwaysMerge, maxBitRate,                  1000      ));
  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::TxKeyFramePeriodOption(),         false, OpalMediaOption::AlwaysMerge, 125,                         0,    1000));
  AddOption(new OpalMediaOptionBoolean (OpalVideoFormat::RateControlEnableOption(),        false, OpalMediaOption::NoMerge,     false                                  ));
  AddOption(new OpalMediaOptionUnsigned(OpalMediaFormat::MaxTxPacketSizeOption(),          false, OpalMediaOption::NoMerge,     8192,                        100,  8192));
  AddOption(new OpalMediaOptionString  (OpalVideoFormat::RateControllerOption(),           false                                                                       ));


  static const char * const RoleEnumerations[OpalVideoFormat::eNumRoles] = {
    "No Role",
    "Presentation",
    "Main",
    "Speaker",
    "Sign Language"
  };
  AddOption(new OpalMediaOptionEnum(OpalVideoFormat::ContentRoleOption(), false,
                                    RoleEnumerations, PARRAYSIZE(RoleEnumerations),
                                    OpalMediaOption::NoMerge));

  AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::ContentRoleMaskOption(),
                                        false, OpalMediaOption::AndMerge,
                                        0, 0, OpalVideoFormat::ContentRoleMask));

  // For video the max bit rate and frame rate is adjustable by user
  FindOption(OpalVideoFormat::MaxBitRateOption())->SetReadOnly(false);
  FindOption(OpalVideoFormat::FrameTimeOption())->SetReadOnly(false);
  FindOption(OpalVideoFormat::FrameTimeOption())->SetMerge(OpalMediaOption::MaxMerge);
}


PObject * OpalVideoFormatInternal::Clone() const
{
  PWaitAndSignal m(media_format_mutex);
  return new OpalVideoFormatInternal(*this);
}


bool OpalVideoFormatInternal::Merge(const OpalMediaFormatInternal & mediaFormat)
{
  PWaitAndSignal m(media_format_mutex);

  if (!OpalMediaFormatInternal::Merge(mediaFormat))
    return false;

  Clamp(*this, mediaFormat, OpalVideoFormat::TargetBitRateOption(), PString::Empty(),                          OpalMediaFormat::MaxBitRateOption());
  Clamp(*this, mediaFormat, OpalVideoFormat::FrameWidthOption(),    OpalVideoFormat::MinRxFrameWidthOption(),  OpalVideoFormat::MaxRxFrameWidthOption());
  Clamp(*this, mediaFormat, OpalVideoFormat::FrameHeightOption(),   OpalVideoFormat::MinRxFrameHeightOption(), OpalVideoFormat::MaxRxFrameHeightOption());

  return true;
}


void OpalMediaFormat::AdjustVideoArgs(PVideoDevice::OpenArgs & args) const
{
  args.width = GetOptionInteger(OpalVideoFormat::FrameWidthOption(), PVideoFrameInfo::QCIFWidth);
  args.height = GetOptionInteger(OpalVideoFormat::FrameHeightOption(), PVideoFrameInfo::QCIFHeight);
  unsigned maxRate = GetClockRate()/GetFrameTime();
  if (args.rate > maxRate)
    args.rate = maxRate;
}

#endif // OPAL_VIDEO

///////////////////////////////////////////////////////////////////////////////

OpalMediaFormatList::OpalMediaFormatList()
{
}


OpalMediaFormatList::OpalMediaFormatList(const OpalMediaFormat & format)
{
  *this += format;
}


OpalMediaFormatList & OpalMediaFormatList::operator+=(const OpalMediaFormat & format)
{
  MakeUnique();
  if (format.IsValid() && !HasFormat(format))
    OpalMediaFormatBaseList::Append(format.Clone());
  return *this;
}


OpalMediaFormatList & OpalMediaFormatList::operator+=(const OpalMediaFormatList & formats)
{
  MakeUnique();
  for (OpalMediaFormatList::const_iterator format = formats.begin(); format != formats.end(); ++format)
    *this += *format;
  return *this;
}


OpalMediaFormatList & OpalMediaFormatList::operator-=(const OpalMediaFormat & format)
{
  MakeUnique();
  OpalMediaFormatList::const_iterator fmt = FindFormat(format);
  if (fmt != end())
    erase(fmt);

  return *this;
}


OpalMediaFormatList & OpalMediaFormatList::operator-=(const OpalMediaFormatList & formats)
{
  MakeUnique();
  for (OpalMediaFormatList::const_iterator format = formats.begin(); format != formats.end(); ++format)
    *this -= *format;
  return *this;
}


void OpalMediaFormatList::Remove(const PStringArray & maskList)
{
  if (maskList.IsEmpty())
    return;

  PTRACE(4,"MediaFormat\tRemoving codecs " << setfill(',') << maskList);

  for (PINDEX i = 0; i < maskList.GetSize(); i++) {
    OpalMediaFormatList::const_iterator fmt;
    while ((fmt = FindFormat(maskList[i])) != end())
      erase(fmt);
  }
}


OpalMediaFormatList::const_iterator OpalMediaFormatList::FindFormat(RTP_DataFrame::PayloadTypes pt, unsigned clockRate, const char * name, const char * protocol, const_iterator iter) const
{
  // First look for a matching encoding name
  if (name != NULL && *name != '\0') {
    OpalMediaFormatList::const_iterator format ((iter == const_iterator()) ? begin() : iter);
    for (; format != end(); ++format) {
      // If encoding name matches exactly, then use it regardless of payload code.
      const char * otherName = format->GetEncodingName();
      if (otherName != NULL && strcasecmp(otherName, name) == 0 &&
          (clockRate == 0    || clockRate == format->GetClockRate()) && // if have clock rate, clock rate must match
          (protocol  == NULL || format->IsValidForProtocol(protocol))) // if protocol is specified, must be valid for the protocol
        return format;
    }
  }

  // Can't match by encoding name, try by known payload type.
  // Note we do two separate loops as it is possible (though discouraged) for
  // someone to override a standard payload type with another encoding name, so
  // have to search all formats by name before trying by number.
  if (pt < RTP_DataFrame::DynamicBase) {
    OpalMediaFormatList::const_iterator format ((iter == const_iterator()) ? begin() : iter);
    for (; format != end(); ++format) {
      if (format->GetPayloadType() == pt &&
          (clockRate == 0    || clockRate == format->GetClockRate()) && // if have clock rate, clock rate must match
          (protocol  == NULL || format->IsValidForProtocol(protocol))) // if protocol is specified, must be valid for the protocol
        return format;
    }
  }

  return end();
}


static bool WildcardMatch(const PCaselessString & str, const PStringArray & wildcards)
{
  if (wildcards.GetSize() == 1)
    return str == wildcards[0];

  PINDEX i;
  PINDEX last = 0;
  for (i = 0; i < wildcards.GetSize(); i++) {
    PString wildcard = wildcards[i];

    PINDEX next;
    if (wildcard.IsEmpty())
      next = last;
    else {
      next = str.Find(wildcard, last);
      if (next == P_MAX_INDEX)
        return false;
    }

    // Check for having * at beginning of search string
    if (i == 0 && next != 0 && !wildcard)
      return false;

    last = next + wildcard.GetLength();

    // Check for having * at end of search string
    if (i == wildcards.GetSize()-1 && !wildcard && last != str.GetLength())
      return false;
  }

  return true;
}


OpalMediaFormatList::const_iterator OpalMediaFormatList::FindFormat(const PString & search, const_iterator iter) const
{
  if (search.IsEmpty())
    return end();

  if (iter == const_iterator())
    iter = begin();

  bool negative = search[0] == '!';

  PString adjustedSearch = search.Mid(negative ? 1 : 0);
  if (adjustedSearch.IsEmpty())
    return end();

  if (adjustedSearch[0] == '@') {
    OpalMediaType searchType = adjustedSearch.Mid(1);
    while (iter != end()) {
      if ((iter->GetMediaType() == searchType) != negative)
        return iter;
      ++iter;
    }
  }
  else {
    PStringArray wildcards = adjustedSearch.Tokenise('*', true);
    while (iter != end()) {
      if (WildcardMatch(iter->m_info->formatName, wildcards) != negative)
        return iter;
      ++iter;
    }
  }

  return end();
}


void OpalMediaFormatList::Reorder(const PStringArray & order)
{
  DisallowDeleteObjects();
  PINDEX nextPos = 0;
  for (PINDEX i = 0; i < order.GetSize(); i++) {
    if (order[i][0] == '@') {
      OpalMediaType mediaType = order[i].Mid(1);
      PINDEX findPos = 0;
      while (findPos < GetSize()) {
        if ((*this)[findPos].GetMediaType() == mediaType) {
          if (findPos > nextPos)
            OpalMediaFormatBaseList::InsertAt(nextPos, RemoveAt(findPos));
          nextPos++;
        }
        findPos++;
      }
    }
    else {
      PStringArray wildcards = order[i].Tokenise('*', true);

      PINDEX findPos = 0;
      while (findPos < GetSize()) {
        if (WildcardMatch((*this)[findPos].m_info->formatName, wildcards)) {
          if (findPos > nextPos)
            OpalMediaFormatBaseList::InsertAt(nextPos, RemoveAt(findPos));
          nextPos++;
        }
        findPos++;
      }
    }
  }
  AllowDeleteObjects();
}

bool OpalMediaFormatList::HasType(const OpalMediaType & type, bool mustBeTransportable) const
{
  OpalMediaFormatList::const_iterator format;
  for (format = begin(); format != end(); ++format) {
    if (format->GetMediaType() == type && (!mustBeTransportable || format->IsTransportable()))
      return true;
  }

  return false;
}
unsigned int OpalMediaFormat::GetCodeDefPointer()
{ 
  if (m_info)
  {
    OpalPluginVideoFormatInternal *fmt = dynamic_cast<OpalPluginVideoFormatInternal *>(m_info);
    if (fmt)
    {
      return (unsigned int )fmt->codecDef;
    }
  }
  return 0;
}

// End of File ///////////////////////////////////////////////////////////////
