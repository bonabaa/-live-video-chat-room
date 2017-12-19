/*
 * g7222mf.cxx
 *
 * GSM-AMR Media Format descriptions
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
 * $Revision: 21964 $
 * $Author: rjongbloed $
 * $Date: 2009-01-29 02:07:30 +0000 (Thu, 29 Jan 2009) $
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#include <opal/mediafmt.h>
#include <codec/opalplugin.h>
#include <h323/h323caps.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

class OpalG7222Format : public OpalAudioFormatInternal
{
  public:
    OpalG7222Format()
      : OpalAudioFormatInternal(OPAL_G7222, RTP_DataFrame::DynamicBase, "AMR-WB",  33, 160, 1, 1, 1, 8000, 0)
    {
      OpalMediaOption * option = new OpalMediaOptionInteger("Initial Mode", false, OpalMediaOption::MinMerge, 7);
#if OPAL_SIP
      option->SetFMTPName("mode");
      option->SetFMTPDefault("0");
#endif
#if OPAL_H323
      OpalMediaOption::H245GenericInfo info;
      info.ordinal = 1;
      info.mode = OpalMediaOption::H245GenericInfo::NonCollapsing;
      info.excludeTCS = info.excludeOLC = true;
      option->SetH245Generic(info);
#endif
      AddOption(option);

#if OPAL_H323
      option = FindOption(OpalAudioFormat::RxFramesPerPacketOption());
      if (option != NULL) {
        info.ordinal = 0; // All other fields the same as for the mode
        info.excludeTCS = false;
        info.excludeReqMode = true;
        option->SetH245Generic(info);
      }
#endif

      AddOption(new OpalMediaOptionString(PLUGINCODEC_MEDIA_PACKETIZATIONS, true, "RFC3267,RFC4867"));
    }
};


#if OPAL_H323
class H323_G7222Capability : public H323GenericAudioCapability
{
  public:
    H323_G7222Capability()
      : H323GenericAudioCapability(OpalPluginCodec_Identifer_G7222)
    {
    }

    virtual PObject * Clone() const
    {
      return new H323_G7222Capability(*this);
    }

    virtual PString GetFormatName() const
    {
      return OpalG7222;
    }
};
#endif // OPAL_H323



const OpalMediaFormat & GetOpalG7222()
{
  static OpalMediaFormat const G7222_Format(new OpalG7222Format);

#if OPAL_H323
  static H323CapabilityFactory::Worker<H323_G7222Capability> G7222_Factory(OPAL_G7222, true);
#endif // OPAL_H323

  return G7222_Format;
}



// End of File ///////////////////////////////////////////////////////////////
