/*
 * g7231mf.cxx
 *
 * G.723.1 Media Format descriptions
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


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H323

class H323_G7231Capability : public H323AudioCapability
{
  public:
    virtual PObject * Clone() const
    {
      return new H323_G7231Capability(*this);
    }

    virtual unsigned GetSubType() const
    {
      return H245_AudioCapability::e_g7231;
    }

    virtual PString GetFormatName() const
    {
      return OpalG7231_6k3;
    }

    virtual PBoolean OnSendingPDU(H245_AudioCapability & pdu, unsigned packetSize) const
    {
      pdu.SetTag(H245_AudioCapability::e_g7231);
      H245_AudioCapability_g7231 & g7231 = pdu;
      g7231.m_maxAl_sduAudioFrames = packetSize;
      g7231.m_silenceSuppression = GetMediaFormat().GetOptionBoolean("VAD");
      return true;
    }

    virtual PBoolean OnReceivedPDU(const H245_AudioCapability & pdu, unsigned & packetSize)
    {
      if (pdu.GetTag() != H245_AudioCapability::e_g7231)
        return false;

      const H245_AudioCapability_g7231 & g7231 = pdu;
      packetSize = g7231.m_maxAl_sduAudioFrames;
      GetWritableMediaFormat().SetOptionBoolean("VAD", g7231.m_silenceSuppression);
      return true;
    }
};

#define CAPABILITY(type) static H323CapabilityFactory::Worker<H323_G7231Capability> type##_Factory(OPAL_##type, true)

#else
#define CAPABILITY(t)
#endif // OPAL_H323


/////////////////////////////////////////////////////////////////////////////

class OpalG723Format : public OpalAudioFormat
{
  public:
    OpalG723Format(const char * variant)
      : OpalAudioFormat(variant, RTP_DataFrame::G7231, "G723", 24, 240,  8,  3, 256,  8000)
    {
      bool isAnnexA = strchr(variant, 'A') != NULL;
      static const char * const yesno[] = { "no", "yes" };
      OpalMediaOption * option = new OpalMediaOptionEnum("VAD", true, yesno, 2, OpalMediaOption::AndMerge, isAnnexA);
#if OPAL_SIP
      option->SetFMTPName("annexa");
      option->SetFMTPDefault("yes");
#endif
      AddOption(option);
    }
};

#define FORMAT(name) \
  const OpalAudioFormat & GetOpal##name() \
  { \
    static const OpalG723Format name##_Format(OPAL_##name); \
    CAPABILITY(name); \
    return name##_Format; \
  }

FORMAT(G7231_6k3);
FORMAT(G7231_5k3);
FORMAT(G7231A_6k3);
FORMAT(G7231A_5k3);


// End of File ///////////////////////////////////////////////////////////////
