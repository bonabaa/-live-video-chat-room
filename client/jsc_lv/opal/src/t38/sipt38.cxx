/*
 * t38proto.cxx
 *
 * T.38 protocol handler
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 1998-2002 Equivalence Pty. Ltd.
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
 * Contributor(s): Vyacheslav Frolov.
 *
 * $Revision: 23831 $
 * $Author: rjongbloed $
 * $Date: 2009-12-03 10:21:07 +0000 (Thu, 03 Dec 2009) $
 */

/*

There are two methods for signalling T.38 within a SIP session.
See T.38 Annex D for more information

UDPTL encapsulation (see T.38 section 9.1) over UDP is signalled as described in RFC 3362 as follows:

  m=image 5000 udptl t38
  a=T38FaxVersion=2
  a=T38FaxRateManagement=transferredTCF

RTP encapsulation (see T.38 section 9.2) is signalled as described in RFC 4612 as follows:

  m=audio 5000 RTP/AVP 96
  a=rtpmap:96 t38/8000
  a=fmtp:98 T38FaxVersion=2;T38FaxRateManagement=transferredTCF

Either, or both, can be used in a call

*/

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "sipt38.h"
#endif

#include <opal/buildopts.h>

#include <t38/sipt38.h>

#if OPAL_SIP
#if OPAL_T38_CAPABILITY

/////////////////////////////////////////////////////////////////////////////

SDPMediaDescription * OpalFaxMediaType::CreateSDPMediaDescription(const OpalTransportAddress & localAddress)
{
  return new SDPFaxMediaDescription(localAddress);
}

/////////////////////////////////////////////////////////////////////////////

SDPFaxMediaDescription::SDPFaxMediaDescription(const OpalTransportAddress & address)
  : SDPMediaDescription(address, OpalMediaType::Fax())
{
  t38Attributes.SetAt("T38FaxRateManagement", "transferredTCF");
  t38Attributes.SetAt("T38FaxVersion",        "0");
  //t38Attributes.SetAt("T38MaxBitRate",        "9600");
  //t38Attributes.SetAt("T38FaxMaxBuffer",      "200");
}

PCaselessString SDPFaxMediaDescription::GetSDPTransportType() const
{ 
  return "udptl";
}

PString SDPFaxMediaDescription::GetSDPMediaType() const 
{ 
  return "image"; 
}

SDPMediaFormat * SDPFaxMediaDescription::CreateSDPMediaFormat(const PString & portString)
{
  return new SDPMediaFormat(*this, RTP_DataFrame::DynamicBase, portString);
}


PString SDPFaxMediaDescription::GetSDPPortList() const
{
  PStringStream str;

  // output encoding names for non RTP
  SDPMediaFormatList::const_iterator format;
  for (format = formats.begin(); format != formats.end(); ++format)
    str << ' ' << format->GetEncodingName();

  return str;
}

bool SDPFaxMediaDescription::PrintOn(ostream & str, const PString & connectString) const
{
  // call ancestor
  if (!SDPMediaDescription::PrintOn(str, connectString))
    return false;

  // output options
  for (PINDEX i = 0; i < t38Attributes.GetSize(); i++) 
    str << "a=" << t38Attributes.GetKeyAt(i) << ":" << t38Attributes.GetDataAt(i) << "\r\n";

  return true;
}

void SDPFaxMediaDescription::SetAttribute(const PString & attr, const PString & value)
{
  if (attr.Left(3) *= "t38") {
    t38Attributes.SetAt(attr, value);
    return;
  }

  return SDPMediaDescription::SetAttribute(attr, value);
}

void SDPFaxMediaDescription::ProcessMediaOptions(SDPMediaFormat & /*sdpFormat*/, const OpalMediaFormat & mediaFormat)
{
  if (mediaFormat.GetMediaType() == OpalMediaType::Fax()) {
    for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); ++i) {
      const OpalMediaOption & option = mediaFormat.GetOption(i);
      if (option.GetName().Left(3) *= "t38") 
        t38Attributes.SetAt(option.GetName(), option.AsString());
    }
  }
}

/////////////////////////////////////////////////////////////////////////////

#endif // OPAL_T38_CAPABILITY
#endif // OPAL_SIP

