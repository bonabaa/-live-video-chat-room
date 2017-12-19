/*
 * mediatype.h
 *
 * Media Format Type descriptions
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2007 Post Increment and Hannes Friederich
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
 * The Original Code is OPAL
 *
 * The Initial Developer of the Original Code is Hannes Friederich and Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23686 $
 * $Author: rjongbloed $
 * $Date: 2009-10-20 04:19:11 +0000 (Tue, 20 Oct 2009) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "mediatype.h"
#endif

#include <opal/buildopts.h>

#include <opal/mediatype.h>
#include <opal/rtpconn.h>
#include <opal/call.h>
#include <opal/endpoint.h>
#include <opal/manager.h>

#if OPAL_H323
#include <h323/channels.h>
#endif


OPAL_INSTANTIATE_MEDIATYPE(audio, OpalAudioMediaType);

#if OPAL_VIDEO
OPAL_INSTANTIATE_MEDIATYPE(video, OpalVideoMediaType);
#endif

OPAL_INSTANTIATE_SIMPLE_MEDIATYPE_NO_SDP(userinput); 


typedef std::map<unsigned, OpalMediaTypeDefinition *> SessionIDToMediaTypeMap_T;
static SessionIDToMediaTypeMap_T & GetSessionIDToMediaTypeMap()
{
  static SessionIDToMediaTypeMap_T map;
  return map;
}

static PMutex & GetMapMutex()
{
  static PMutex mutex;
  return mutex;
}



///////////////////////////////////////////////////////////////////////////////

const OpalMediaType & OpalMediaType::Audio()     { static const OpalMediaType type = "audio";     return type; }
const OpalMediaType & OpalMediaType::Video()     { static const OpalMediaType type = "video";     return type; }
const OpalMediaType & OpalMediaType::Fax()       { static const OpalMediaType type = "fax";       return type; };
const OpalMediaType & OpalMediaType::UserInput() { static const OpalMediaType type = "userinput"; return type; };
const OpalMediaType & OpalMediaType::MSRP()		 { static const OpalMediaType type = "msrp"; return type; };


///////////////////////////////////////////////////////////////////////////////

OpalMediaTypeDefinition * OpalMediaType::GetDefinition() const
{
  return OpalMediaTypeFactory::CreateInstance(*this);
}


OpalMediaTypeDefinition * OpalMediaType::GetDefinition(const OpalMediaType & key)
{
  return OpalMediaTypeFactory::CreateInstance(key);
}


OpalMediaTypeDefinition * OpalMediaType::GetDefinition(unsigned sessionId)
{
  PWaitAndSignal mutex(GetMapMutex());
  SessionIDToMediaTypeMap_T & typeMap = GetSessionIDToMediaTypeMap();
  SessionIDToMediaTypeMap_T::iterator iter = typeMap.find(sessionId);
  return iter != typeMap.end() ? iter->second : NULL;
}


///////////////////////////////////////////////////////////////////////////////

OpalMediaTypeDefinition::OpalMediaTypeDefinition(const char * mediaType,
#if OPAL_SIP
                                                 const char * sdpType,
#else
                                                 const char *,
#endif
                                                 unsigned requiredSessionId,
                                                 OpalMediaType::AutoStartMode autoStart)
  : m_mediaType(mediaType)
  , m_autoStart(autoStart)
#if OPAL_SIP
  , m_sdpType(sdpType != NULL ? sdpType : "")
#endif
{
  PWaitAndSignal mutex(GetMapMutex());

  SessionIDToMediaTypeMap_T & typeMap = GetSessionIDToMediaTypeMap();

  if (requiredSessionId != 0 &&
      PAssert(typeMap.find(requiredSessionId) == typeMap.end(),
              "Cannot have multiple media types with same session ID"))
    m_defaultSessionId = requiredSessionId;
  else {
    // Allocate session ID
    m_defaultSessionId = 4; // Start after audio, video and data that are "special"
    while (typeMap.find(m_defaultSessionId) != typeMap.end())
      ++m_defaultSessionId;
  }

  typeMap[m_defaultSessionId] = this;
}


OpalMediaSession * OpalMediaTypeDefinition::CreateMediaSession(OpalConnection & /*conn*/, unsigned /* sessionID*/) const
{ 
  return NULL; 
}


RTP_UDP * OpalMediaTypeDefinition::CreateRTPSession(OpalRTPConnection & connection,
                                                    unsigned sessionID,
                                                    bool remoteIsNAT)
{
  RTP_Session::Params params;
  params.id = sessionID;
  params.encoding = GetRTPEncoding();
  params.isAudio = m_mediaType == OpalMediaType::Audio();
  params.remoteIsNAT = remoteIsNAT;

  return connection.GetEndPoint().GetManager().CreateRTPSession(params);
}


///////////////////////////////////////////////////////////////////////////////

OpalRTPAVPMediaType::OpalRTPAVPMediaType(const char * mediaType,
                                         const char * sdpType,
                                         unsigned requiredSessionId,
                                         OpalMediaType::AutoStartMode autoStart)
  : OpalMediaTypeDefinition(mediaType, sdpType, requiredSessionId, autoStart)
{
}

PString OpalRTPAVPMediaType::GetRTPEncoding() const
{
  return "rtp/avp";
}

OpalMediaSession * OpalRTPAVPMediaType::CreateMediaSession(OpalConnection & conn, unsigned sessionID) const
{
  return new OpalRTPMediaSession(conn, m_mediaType, sessionID, NULL);
}


///////////////////////////////////////////////////////////////////////////////

OpalAudioMediaType::OpalAudioMediaType()
  : OpalRTPAVPMediaType("audio", "audio", 1, OpalMediaType::ReceiveTransmit)
{
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

OpalVideoMediaType::OpalVideoMediaType()
  : OpalRTPAVPMediaType("video", "video", 2)
{ }

#endif // OPAL_VIDEO

///////////////////////////////////////////////////////////////////////////////
