/*
 * g726mf.cxx
 *
 * G.726 Media Format descriptions
 *
 * Open Phone Abstraction Library
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2008 Vox Lucida
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
 * The Initial Developer of the Original Code is Vox Lucida
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21962 $
 * $Author: rjongbloed $
 * $Date: 2009-01-29 01:06:50 +0000 (Thu, 29 Jan 2009) $
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#include <opal/mediafmt.h>
#include <h323/h323caps.h>
#include <asn/h245.h>


#define new PNEW


enum G726SubTypes {
  G726_40K = 0,
  G726_32K = 1,
  G726_24K = 2,
  G726_16K = 3
};

static const char * const G726Name[4] = {
  OPAL_G726_40K,
  OPAL_G726_32K,
  OPAL_G726_24K,
  OPAL_G726_16K
};

static const char * const G726IANA[4] = {
  "G726-40",
  "G726-32",
  "G726-24",
  "G726-16"
};

static unsigned const G726Bits[4] = {
  5,
  4,
  3,
  2
};


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H323

static const char * const G726OID[4] = {
  "0.0.7.726.1.0.40",
  "0.0.7.726.1.0.32",
  "0.0.7.726.1.0.24",
  "0.0.7.726.1.0.16"
};

template <G726SubTypes subtype>
class H323_G726Capability : public H323GenericAudioCapability
{
  public:
    H323_G726Capability()
      : H323GenericAudioCapability(G726OID[subtype])
    {
    }

    virtual PObject * Clone() const
    {
      return new H323_G726Capability(*this);
    }

    virtual PString GetFormatName() const
    {
      return G726Name[subtype];
    }
};

#define CAPABILITY(type) static H323CapabilityFactory::Worker<H323_G726Capability<type> > type##_Factory(G726Name[type], true);

#else
#define CAPABILITY(t)
#endif // OPAL_H323


/////////////////////////////////////////////////////////////////////////////

#define FORMAT(type) \
  const OpalAudioFormat & GetOpal##type() \
  { \
    static const OpalAudioFormat type##_Format(G726Name[type], RTP_DataFrame::DynamicBase,  G726IANA[type], G726Bits[type], 8, 240, 30, 256, 8000); \
    CAPABILITY(type); \
    return type##_Format; \
  }

FORMAT(G726_40K);
FORMAT(G726_32K);
FORMAT(G726_24K);
FORMAT(G726_16K);


// End of File ///////////////////////////////////////////////////////////////
