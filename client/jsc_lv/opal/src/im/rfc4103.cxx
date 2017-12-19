/*
 * rfc4103.cxx
 *
 * Implementation of RFC 4103 RTP Payload for Text Conversation
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2008 Post Increment
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
 * $Revision: 23632 $
 * $Author: csoutheren $
 * $Date: 2009-10-09 06:02:18 +0000 (Fri, 09 Oct 2009) $
 */

#ifdef __GNUC__
#pragma implementation "rfc4103.h"
#endif

#include <ptlib.h>
#include <opal/buildopts.h>

#include <im/rfc4103.h>

#if OPAL_HAS_RFC4103

#include <sip/sdp.h>

/////////////////////////////////////////////////////////////////////////////

RFC4103Context::RFC4103Context()
  : m_sequence(0)
  , m_baseTimeStamp(0)
{
}

RFC4103Context::RFC4103Context(const OpalMediaFormat & fmt)
  : m_mediaFormat(fmt)
  , m_sequence(0)
  , m_baseTimeStamp(0)
{
}

void RFC4103Context::SetMediaFormat(const OpalMediaFormat & fmt)
{
  m_mediaFormat = fmt;
}


RTP_DataFrameList RFC4103Context::ConvertToFrames(const PString & contentType, const T140String & content)
{
  DWORD ts = m_baseTimeStamp;
  ts += (DWORD)((PTime() - m_baseTime).GetMilliSeconds());

  RTP_DataFrameList frames;
  RTP_IMFrame * frame = new RTP_IMFrame(contentType, content);

  frame->SetPayloadType(m_mediaFormat.GetPayloadType());
  frame->SetMarker(true);
  frame->SetTimestamp(ts);
  frame->SetSequenceNumber(++m_sequence);

  frames.Append(frame);

  return frames;
}

/////////////////////////////////////////////////////////////////////////////


#endif // OPAL_HAS_RFC4103
