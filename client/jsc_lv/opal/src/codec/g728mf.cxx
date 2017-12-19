/*
 * g728mf.cxx
 *
 * G.728 Media Format descriptions
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
class H323_G728Capability : public H323AudioCapability
{
  public:
    virtual PObject * Clone() const
    {
      return new H323_G728Capability(*this);
    }

    virtual unsigned GetSubType() const
    {
      return H245_AudioCapability::e_g728;
    }

    virtual PString GetFormatName() const
    {
      return OpalG728;
    }
};
#endif // OPAL_H323


const OpalAudioFormat & GetOpalG728()
{
  static const OpalAudioFormat G728_Format(OPAL_G728, RTP_DataFrame::G728,  "G728", 5, 20, 100, 10, 256, 8000);

#if OPAL_H323
  static H323CapabilityFactory::Worker<H323_G728Capability> G728_Factory(OPAL_G728, true);
#endif // OPAL_H323

  return G728_Format;
}


// End of File ///////////////////////////////////////////////////////////////
