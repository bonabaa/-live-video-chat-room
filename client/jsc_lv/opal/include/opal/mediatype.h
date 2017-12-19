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
 * $Revision: 23938 $
 * $Author: rjongbloed $
 * $Date: 2010-01-14 05:06:54 +0000 (Thu, 14 Jan 2010) $
 */

#ifndef OPAL_OPAL_MEDIATYPE_H
#define OPAL_OPAL_MEDIATYPE_H

#include <ptbuildopts.h>
#include <ptlib/pfactory.h>
#include <opal/buildopts.h>

#ifdef P_USE_PRAGMA
#pragma interface
#endif


class OpalMediaTypeDefinition;
class OpalSecurityMode;
class OpalConnection;

////////////////////////////////////////////////////////////////////////////
//
//  define the factory used for keeping track of OpalMediaTypeDefintions
//
typedef PFactory<OpalMediaTypeDefinition> OpalMediaTypeFactory;
typedef OpalMediaTypeFactory::KeyList_T OpalMediaTypeList;


/** Define the type used to hold the media type identifiers, i.e. "audio", "video", "h.224", "fax" etc
  */
class OpalMediaType : public std::string     // do not make this PCaselessString as that type does not work as index for std::map etc
{
  public:
    OpalMediaType()
    { }

    virtual ~OpalMediaType()
    { }

    OpalMediaType(const std::string & str)
      : std::string(str) { }

    OpalMediaType(const char * str)
      : std::string(str) { }

    OpalMediaType(const PString & str)
      : std::string((const char *)str) { }

    static const OpalMediaType & Audio();
    static const OpalMediaType & Video();
    static const OpalMediaType & Fax();
    static const OpalMediaType & UserInput();
    static const OpalMediaType & MSRP();

    OpalMediaTypeDefinition * GetDefinition() const;
    static OpalMediaTypeDefinition * GetDefinition(const OpalMediaType & key);
    static OpalMediaTypeDefinition * GetDefinition(unsigned sessionId);

    static OpalMediaTypeFactory::KeyList_T GetList() { return OpalMediaTypeFactory::GetKeyList(); }

#if OPAL_SIP
    static OpalMediaType GetMediaTypeFromSDP(const std::string & key, const std::string & transport);
#endif  // OPAL_SIP

    enum AutoStartMode {
      // Do not change order of enum as useful for bitmasking rx/tx
      OfferInactive,
      Receive,
      Transmit,
      ReceiveTransmit,
      DontOffer,

      TransmitReceive = ReceiveTransmit
    };

    __inline friend AutoStartMode operator++(AutoStartMode & mode)                 { return (mode = (AutoStartMode)(mode+1)); }
    __inline friend AutoStartMode operator--(AutoStartMode & mode)                 { return (mode = (AutoStartMode)(mode-1)); }
    __inline friend AutoStartMode operator|=(AutoStartMode & m1, AutoStartMode m2) { return (m1 = (AutoStartMode)((m1 & ~DontOffer) | m2)); }
    __inline friend AutoStartMode operator-=(AutoStartMode & m1, AutoStartMode m2) { return (m1 = (AutoStartMode)((int)m1 & ~((int)m2|DontOffer))); }

    AutoStartMode GetAutoStart() const;
};


__inline ostream & operator << (ostream & strm, const OpalMediaType & mediaType)
{
  return strm << mediaType.c_str();
}


////////////////////////////////////////////////////////////////////////////
//
//  this class defines the functions needed to work with the media type, i.e. 
//
class OpalRTPConnection;
class RTP_UDP;

#if OPAL_SIP
class SDPMediaDescription;
class OpalTransportAddress;
#endif

class OpalMediaSession;

/** This class defines the type used to define the attributes of a media type
 */
class OpalMediaTypeDefinition
{
  public:
    /// Create a new media type definition
    OpalMediaTypeDefinition(
      const char * mediaType,          ///< name of the media type (audio, video etc)
      const char * sdpType,            ///< name of the SDP type 
      unsigned requiredSessionId = 0,  ///< Session ID to use, asserts if already in use
      OpalMediaType::AutoStartMode autoStart = OpalMediaType::DontOffer   ///< Default value for auto-start transmit & receive
    );

    // Needed to avoid gcc warning about classes with virtual functions and 
    //  without a virtual destructor
    virtual ~OpalMediaTypeDefinition() { }

    /** Get flags for media type can auto-start on call initiation.
      */
    OpalMediaType::AutoStartMode GetAutoStart() const { return m_autoStart; }

    /** Set flag for media type can auto-start receive on call initiation.
      */
    void SetAutoStart(OpalMediaType::AutoStartMode v) { m_autoStart = v; }
    void SetAutoStart(OpalMediaType::AutoStartMode v, bool on) { if (on) m_autoStart |= v; else m_autoStart -= v; }

    /** Indicate type uses RTP for transport.
        If false, then it uses a generic OpaMediaSession
      */
    virtual bool UsesRTP() const { return true; }

    /** Create a media session suitable for the media type.
      */
    virtual OpalMediaSession * CreateMediaSession(
      OpalConnection & connection,  ///< Connection media session is being created for
      unsigned         sessionID    ///< ID for the media session
    ) const;

    /** Get the string used for the RTP_FormatHandler PFactory which is used
        to create the RTP handler for the this media type
        possible values include "rtp/avp" and "udptl"

        Only valid if UsesRTP return true
      */
    virtual PString GetRTPEncoding() const = 0;

    /** Create an RTP session for this media format.
        By default, this will create a RTP_UDP session with the correct initial format

        Only valid if UsesRTP return true
      */
    virtual RTP_UDP * CreateRTPSession(
      OpalRTPConnection & conn,
      unsigned sessionID, 
      bool remoteIsNAT
    );

    /** Return the default session ID for this media type.
      */
    unsigned GetDefaultSessionId() const { return m_defaultSessionId; }

  protected:
    std::string m_mediaType;
    unsigned    m_defaultSessionId;
    OpalMediaType::AutoStartMode m_autoStart;

#if OPAL_SIP
  public:
    //
    //  return the SDP type for this media type
    //
    virtual std::string GetSDPType() const 
    { return m_sdpType; }

    //
    //  create an SDP media description entry for this media type
    //
    virtual SDPMediaDescription * CreateSDPMediaDescription(
      const OpalTransportAddress & localAddress
    ) = 0;

  protected:
    std::string m_sdpType;
#endif
};


////////////////////////////////////////////////////////////////////////////
//
//  define a macro for declaring a new OpalMediaTypeDefinition factory
//

#define OPAL_INSTANTIATE_MEDIATYPE2(title, name, cls) \
namespace OpalMediaTypeSpace { \
  static PFactory<OpalMediaTypeDefinition>::Worker<cls> static_##title##_##cls(name, true); \
}; \

#define OPAL_INSTANTIATE_MEDIATYPE(type, cls) \
  OPAL_INSTANTIATE_MEDIATYPE2(type, #type, cls) \


#ifdef SOLARIS
template <char * Type, char * sdp>
#else
template <char * Type, const char * sdp>
#endif
class SimpleMediaType : public OpalMediaTypeDefinition
{
  public:
    SimpleMediaType()
      : OpalMediaTypeDefinition(Type, sdp, 0)
    { }

    virtual ~SimpleMediaType()                     { }

    virtual RTP_UDP * CreateRTPSession(OpalRTPConnection & ,unsigned , bool ) { return NULL; }

    PString GetRTPEncoding() const { return PString::Empty(); } 

#if OPAL_SIP
  public:
    virtual SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & ) { return NULL; }
#endif
};


#define OPAL_INSTANTIATE_SIMPLE_MEDIATYPE(type, sdp) \
namespace OpalMediaTypeSpace { \
  char type##_type_string[] = #type; \
  char type##_sdp_string[] = #sdp; \
  typedef SimpleMediaType<type##_type_string, type##_sdp_string> type##_MediaType; \
}; \
OPAL_INSTANTIATE_MEDIATYPE(type, type##_MediaType) \

#define OPAL_INSTANTIATE_SIMPLE_MEDIATYPE_NO_SDP(type) OPAL_INSTANTIATE_SIMPLE_MEDIATYPE(type, "") 

////////////////////////////////////////////////////////////////////////////
//
//  common ancestor for audio and video OpalMediaTypeDefinitions
//

class OpalRTPAVPMediaType : public OpalMediaTypeDefinition {
  public:
    OpalRTPAVPMediaType(
      const char * mediaType, 
      const char * sdpType, 
      unsigned     requiredSessionId = 0,
      OpalMediaType::AutoStartMode autoStart = OpalMediaType::DontOffer
    );

    virtual PString GetRTPEncoding() const;

    OpalMediaSession * CreateMediaSession(OpalConnection & /*conn*/, unsigned /* sessionID*/) const;
};


class OpalAudioMediaType : public OpalRTPAVPMediaType {
  public:
    OpalAudioMediaType();

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
#endif
};


#if OPAL_VIDEO

class OpalVideoMediaType : public OpalRTPAVPMediaType {
  public:
    OpalVideoMediaType();

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
#endif
};

#endif // OPAL_VIDEO


#if OPAL_T38_CAPABILITY

class OpalFaxMediaType : public OpalMediaTypeDefinition 
{
  public:
    OpalFaxMediaType();

    PString GetRTPEncoding(void) const;
    RTP_UDP * CreateRTPSession(OpalRTPConnection & conn,
                               unsigned sessionID, bool remoteIsNAT);

    OpalMediaSession * CreateMediaSession(OpalConnection & conn, unsigned /* sessionID*/) const;

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
#endif
};

#endif // OPAL_T38_CAPABILITY


__inline OpalMediaType::AutoStartMode OpalMediaType::GetAutoStart() const { return GetDefinition()->GetAutoStart(); }


#endif // OPAL_OPAL_MEDIATYPE_H
