/*
 * gsm0610mf.cxx
 *
 * GSM 06.10 Full Rate Media Format descriptions
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
#include <h323/h323caps.h>
#include <asn/h245.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H323
class H323_GSM0610Capability : public H323AudioCapability
{
  public:
    virtual PObject * Clone() const
    {
      return new H323_GSM0610Capability(*this);
    }

    virtual unsigned GetSubType() const
    {
      return H245_AudioCapability::e_gsmFullRate;
    }

    virtual PString GetFormatName() const
    {
      return OpalGSM0610;
    }

    virtual void SetTxFramesInPacket(unsigned frames)
    {
      H323AudioCapability::SetTxFramesInPacket(frames > 7 ? 7 : frames);
    }

    virtual PBoolean OnSendingPDU(H245_AudioCapability & pdu, unsigned packetSize) const
    {
      pdu.SetTag(H245_AudioCapability::e_gsmFullRate);

      H245_GSMAudioCapability & gsm = pdu;
      gsm.m_audioUnitSize = packetSize*33;
      return true;
    }

    virtual PBoolean OnReceivedPDU(const H245_AudioCapability & pdu, unsigned & packetSize)
    {
      if (pdu.GetTag() != H245_AudioCapability::e_gsmFullRate)
        return false;

      const H245_GSMAudioCapability & gsm = pdu;
      packetSize = gsm.m_audioUnitSize/33;
      if (packetSize == 0)
        packetSize = 1;
      return true;
    }
};
#endif // OPAL_H323


const OpalAudioFormat & GetOpalGSM0610()
{
  static const OpalAudioFormat GSM0610(OPAL_GSM0610, RTP_DataFrame::GSM, "GSM",  33, 160, 7, 4, 7, 8000 );

#if OPAL_H323
  static H323CapabilityFactory::Worker<H323_GSM0610Capability> GSM0610_Factory(OPAL_GSM0610, true);
#endif // OPAL_H323

  return GSM0610;
}


// End of File ///////////////////////////////////////////////////////////////
