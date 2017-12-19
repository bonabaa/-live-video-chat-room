/*
 * g729mf.cxx
 *
 * G.729 Media Format descriptions
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


enum G729SubTypes {
  G729   = 0,
  G729A  = 1,
  G729B  = 2,
  G729AB = 3
};

static const char * const G729Name[4] = {
  OPAL_G729,
  OPAL_G729A,
  OPAL_G729B,
  OPAL_G729AB
};


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H323

static const unsigned G729SubType[4] = {
  H245_AudioCapability::e_g729,
  H245_AudioCapability::e_g729AnnexA,
  H245_AudioCapability::e_g729wAnnexB,
  H245_AudioCapability::e_g729AnnexAwAnnexB
};

template <G729SubTypes subtype>
class H323_G729CapabilityTemplate : public H323AudioCapability
{
  public:
    virtual PObject * Clone() const
    {
      return new H323_G729CapabilityTemplate(*this);
    }

    virtual unsigned GetSubType() const
    {
      return G729SubType[subtype];
    }

    virtual PString GetFormatName() const
    {
      return G729Name[subtype];
    }
};

#define CAPABILITY(type) static H323CapabilityFactory::Worker<H323_G729CapabilityTemplate<type> > type##_Factory(G729Name[type], true)

#else
#define CAPABILITY(t)
#endif // OPAL_H323


/////////////////////////////////////////////////////////////////////////////

class OpalG729Format : public OpalAudioFormat
{
  public:
    OpalG729Format(const char * variant)
      : OpalAudioFormat(variant, RTP_DataFrame::G729, "G729", 10, 80, 24, 5, 256, 8000)
    {
      // As per RFC3555
      bool isAnnexB = strchr(variant, 'B') != NULL;
      static const char * const yesno[] = { "no", "yes" };
      OpalMediaOption * option = new OpalMediaOptionEnum("VAD", true, yesno, 2, OpalMediaOption::AndMerge, isAnnexB);
#if OPAL_SIP
      option->SetFMTPName("annexb");
      option->SetFMTPDefault("yes");
#endif
      AddOption(option);
    }
};


#define FORMAT(name) \
  const OpalAudioFormat & GetOpal##name() \
  { \
  static const OpalG729Format name##_Format(G729Name[name]); \
    CAPABILITY(name); \
    return name##_Format; \
  }

FORMAT(G729  );
FORMAT(G729A );
FORMAT(G729B );
FORMAT(G729AB);


// End of File ///////////////////////////////////////////////////////////////
