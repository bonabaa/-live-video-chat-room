/*
 * t38proto.h
 *
 * T.38 protocol handler
 *
 * Open Phone Abstraction Library
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21293 $
 * $Author: rjongbloed $
 * $Date: 2008-10-12 23:24:41 +0000 (Sun, 12 Oct 2008) $
 */

#ifndef OPAL_T38_SIPT38_H
#define OPAL_T38_SIPT38_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_SIP

#include <sip/sdp.h>

#if OPAL_T38_CAPABILITY

/////////////////////////////////////////////////////////
//
//  SDP media description for fax media
//

class SDPFaxMediaDescription : public SDPMediaDescription
{
  PCLASSINFO(SDPFaxMediaDescription, SDPMediaDescription);
  public:
    SDPFaxMediaDescription(const OpalTransportAddress & address);
    virtual PCaselessString GetSDPTransportType() const;
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & portString);
    virtual PString GetSDPMediaType() const;
    virtual PString GetSDPPortList() const;
    virtual bool PrintOn(ostream & str, const PString & connectString) const;
    virtual void SetAttribute(const PString & attr, const PString & value);
    virtual void ProcessMediaOptions(SDPMediaFormat & sdpFormat, const OpalMediaFormat & mediaFormat);

  protected:
    PStringToString t38Attributes;
};

#endif // OPAL_T38_CAPABILITY

#endif // OPAL_SIP

#endif // OPAL_T38_SIPT38_H

