/*
 * im_mf.h
 *
 * Media formats for Instant Messaging
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
 * $Revision: 23775 $
 * $Author: rjongbloed $
 * $Date: 2009-11-18 07:46:17 +0000 (Wed, 18 Nov 2009) $
 */

#ifndef OPAL_IM_IM_H
#define OPAL_IM_IM_H

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_HAS_IM

#include <opal/mediastrm.h>

#include <im/rfc4103.h>

class OpalIMMediaType : public OpalMediaTypeDefinition 
{
  public:
    OpalIMMediaType(
      const char * mediaType, ///< name of the media type (audio, video etc)
      const char * sdpType    ///< name of the SDP type 
    )
      : OpalMediaTypeDefinition(mediaType, sdpType, 0, OpalMediaType::ReceiveTransmit)
    { }

    PString GetRTPEncoding() const { return PString::Empty(); }
    RTP_UDP * CreateRTPSession(OpalRTPConnection & , unsigned , bool ) { return NULL; }
    virtual bool UsesRTP() const { return false; }
};

class RTP_IMFrame : public RTP_DataFrame
{
  public:
    RTP_IMFrame();
    RTP_IMFrame(const PString & contentType);
    RTP_IMFrame(const PString & contentType, const T140String & content);
    RTP_IMFrame(const BYTE * data, PINDEX len, PBoolean dynamic = PTrue);

    void SetContentType(const PString & contentType);
    PString GetContentType() const;

    void SetContent(const T140String & text);
    bool GetContent(T140String & text) const;

    PString AsString() const { return PString((const char *)GetPayloadPtr(), GetPayloadSize()); }
};

class OpalIMMediaStream : public OpalMediaStream
{
  public:
    OpalIMMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource                        ///<  Is a source stream
    );

    virtual PBoolean IsSynchronous() const         { return false; }
    virtual PBoolean RequiresPatchThread() const   { return false; }

    bool ReadPacket(RTP_DataFrame & packet);
    bool WritePacket(RTP_DataFrame & packet);
};

#endif // OPAL_HAS_IM

#endif // OPAL_IM_IM_H
