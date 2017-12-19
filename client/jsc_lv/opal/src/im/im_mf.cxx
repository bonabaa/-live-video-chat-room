/*
 * im_mf.cxx
 *
 * Instant Messaging Media Format descriptions
 *
 * Open Phone Abstraction Library
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
 * The Original Code is Open Phone Abstraction Library
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23617 $
 * $Author: csoutheren $
 * $Date: 2009-10-08 08:11:53 +0000 (Thu, 08 Oct 2009) $
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_HAS_IM

#include <opal/mediafmt.h>
#include <opal/connection.h>
#include <opal/patch.h>
#include <im/im.h>
#include <im/msrp.h>
#include <im/sipim.h>
#include <im/rfc4103.h>
#include <rtp/rtp.h>

#define new PNEW

/////////////////////////////////////////////////////////////////////////////

#if OPAL_HAS_MSRP

OPAL_INSTANTIATE_MEDIATYPE(msrp, OpalMSRPMediaType);

/////////////////////////////////////////////////////////////////////////////


#define DECLARE_MSRP_ENCODING(title, encoding) \
class IM##title##OpalMSRPEncoding : public OpalMSRPEncoding \
{ \
}; \
static PFactory<OpalMSRPEncoding>::Worker<IM##title##OpalMSRPEncoding> worker_##IM##title##OpalMSRPEncoding(encoding, true); \

/////////////////////////////////////////////////////////////////////////////

DECLARE_MSRP_ENCODING(Text, "text/plain");
DECLARE_MSRP_ENCODING(CPIM, "message/cpim");
DECLARE_MSRP_ENCODING(HTML, "message/html");

const OpalMediaFormat & GetOpalMSRP() 
{ 
  static class IMMSRPMediaFormat : public OpalMediaFormat { 
    public: 
      IMMSRPMediaFormat() 
        : OpalMediaFormat(OPAL_MSRP, 
                          "msrp", 
                          RTP_DataFrame::MaxPayloadType, 
                          "+", 
                          false,  
                          1440, 
                          512, 
                          0, 
                          1000)            // as defined in RFC 4103 - good as anything else   
      { 
        PFactory<OpalMSRPEncoding>::KeyList_T types = PFactory<OpalMSRPEncoding>::GetKeyList();
        PFactory<OpalMSRPEncoding>::KeyList_T::iterator r;

        PString acceptTypes;
        for (r = types.begin(); r != types.end(); ++r) {
          if (!acceptTypes.IsEmpty())
            acceptTypes += " ";
          acceptTypes += *r;
        }

        OpalMediaOptionString * option = new OpalMediaOptionString("Accept Types", false, acceptTypes);
        option->SetMerge(OpalMediaOption::AlwaysMerge);
        AddOption(option);

        option = new OpalMediaOptionString("Path", false, "");
        option->SetMerge(OpalMediaOption::MaxMerge);
        AddOption(option);
      } 
  } const f; 
  return f; 
} 

#endif // OPAL_HAS_MSRP


//////////////////////////////////////////////////////////////////////////////////////////

#if OPAL_HAS_SIPIM

OPAL_INSTANTIATE_MEDIATYPE2(sipim, "sip-im", OpalSIPIMMediaType);

const OpalMediaFormat & GetOpalSIPIM() 
{ 
  static class IMSIPMediaFormat : public OpalMediaFormat { 
    public: 
      IMSIPMediaFormat() 
        : OpalMediaFormat(OPAL_SIPIM, 
                          "sip-im", 
                          RTP_DataFrame::MaxPayloadType, 
                          "+", 
                          false,  
                          1440, 
                          512, 
                          0, 
                          1000)     // as defined in RFC 4103 - good as anything else
      { 
        OpalMediaOptionString * option = new OpalMediaOptionString("URL", false, "");
        option->SetMerge(OpalMediaOption::NoMerge);
        AddOption(option);
      } 
  } const f; 
  return f; 
} 

#endif // OPAL_HAS_SIPIM

//////////////////////////////////////////////////////////////////////////////////////////

const OpalMediaFormat & GetOpalT140() 
{ 
  static class T140MediaFormat : public OpalMediaFormat { 
    public: 
      T140MediaFormat() 
        : OpalMediaFormat(OPAL_T140, 
                          "t140", 
                          RTP_DataFrame::DynamicBase, 
                          "text", 
                          false,  
                          1440, 
                          512, 
                          0, 
                          1000)    // as defined in RFC 4103
      { 
      } 
  } const f; 
  return f; 
} 

OPAL_INSTANTIATE_MEDIATYPE(t140, OpalT140MediaType);

//////////////////////////////////////////////////////////////////////////////////////////

RTP_IMFrame::RTP_IMFrame(const BYTE * data, PINDEX len, bool dynamic)
  : RTP_DataFrame(data, len, dynamic)
{
}


RTP_IMFrame::RTP_IMFrame()
{
  SetExtension(true);
  SetExtensionSizeDWORDs(0);
  SetPayloadSize(0);
}

RTP_IMFrame::RTP_IMFrame(const PString & contentType)
{
  SetExtension(true);
  SetExtensionSizeDWORDs(0);
  SetPayloadSize(0);
  SetContentType(contentType);
}

RTP_IMFrame::RTP_IMFrame(const PString & contentType, const T140String & content)
{
  SetExtension(true);
  SetExtensionSizeDWORDs(0);
  SetPayloadSize(0);
  SetContentType(contentType);
  SetContent(content);
}

void RTP_IMFrame::SetContentType(const PString & contentType)
{
  PINDEX newExtensionBytes  = contentType.GetLength();
  PINDEX newExtensionDWORDs = (newExtensionBytes + 3) / 4;
  PINDEX oldPayloadSize = GetPayloadSize();

  // adding an extension adds 4 bytes to the header,
  //  plus the number of 32 bit words needed to hold the extension
  if (!GetExtension()) {
    SetPayloadSize(4 + newExtensionDWORDs + oldPayloadSize);
    if (oldPayloadSize > 0)
      memcpy(GetPayloadPtr() + newExtensionBytes + 4, GetPayloadPtr(), oldPayloadSize);
  }

  // if content type has not changed, nothing to do
  else if (GetContentType() == contentType) 
    return;

  // otherwise copy the new extension in
  else {
    PINDEX oldExtensionDWORDs = GetExtensionSizeDWORDs();
    if (oldPayloadSize != 0) {
      if (newExtensionDWORDs <= oldExtensionDWORDs) {
        memcpy(GetExtensionPtr() + newExtensionBytes, GetPayloadPtr(), oldPayloadSize);
      }
      else {
        SetPayloadSize((newExtensionDWORDs - oldExtensionDWORDs)*4 + oldPayloadSize);
        memcpy(GetExtensionPtr() + newExtensionDWORDs*4, GetPayloadPtr(), oldPayloadSize);
      }
    }
  }
  
  // reset lengths
  SetExtensionSizeDWORDs(newExtensionDWORDs);
  memcpy(GetExtensionPtr(), (const char *)contentType, newExtensionBytes);
  SetPayloadSize(oldPayloadSize);
  if (newExtensionDWORDs*4 > newExtensionBytes)
    memset(GetExtensionPtr() + newExtensionBytes, 0, newExtensionDWORDs*4 - newExtensionBytes);
}


PString RTP_IMFrame::GetContentType() const
{
  if (!GetExtension() || (GetExtensionSizeDWORDs() == 0))
    return PString::Empty();

  const char * p = (const char *)GetExtensionPtr();
  return PString(p, strlen(p));
}

void RTP_IMFrame::SetContent(const T140String & text)
{
  SetPayloadSize(text.GetSize());
  memcpy(GetPayloadPtr(), (const BYTE *)text, text.GetSize());
}

bool RTP_IMFrame::GetContent(T140String & text) const
{
  if (GetPayloadSize() == 0) 
    text.SetSize(0);
  else 
    text = T140String((const BYTE *)GetPayloadPtr(), GetPayloadSize());
  return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

OpalIMMediaStream::OpalIMMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource                        ///<  Is a source stream
    )
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
{
}

bool OpalIMMediaStream::ReadPacket(RTP_DataFrame & /*packet*/)
{
  PAssertAlways("Cannot ReadData from OpalIMMediaStream");
  return false;
}

bool OpalIMMediaStream::WritePacket(RTP_DataFrame & frame)
{
  RTP_IMFrame imFrame(frame.GetPointer(), frame.GetSize());
  connection.OnReceiveInternalIM(mediaFormat, imFrame);
  return true;
}

#endif // OPAL_HAS_IM
