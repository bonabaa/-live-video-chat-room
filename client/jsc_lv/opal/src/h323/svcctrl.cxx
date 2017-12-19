/*
 * svcctrl.cxx
 *
 * H.225 Service Control protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * $Revision: 19279 $
 * $Author: rjongbloed $
 * $Date: 2008-01-17 04:08:34 +0000 (Thu, 17 Jan 2008) $
 */

#include <ptlib.h>

#include <opal/buildopts.h>
#if OPAL_H323

#ifdef __GNUC__
#pragma implementation "svcctrl.h"
#endif

#include <h323/svcctrl.h>

#include <h323/h323ep.h>
#include <h323/h323pdu.h>
#include <asn/h248.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

H323ServiceControlSession::H323ServiceControlSession()
{
}


PString H323ServiceControlSession::GetServiceControlType() const
{
  return GetClass();
}


/////////////////////////////////////////////////////////////////////////////

H323HTTPServiceControl::H323HTTPServiceControl(const PString & u)
  : url(u)
{
}


H323HTTPServiceControl::H323HTTPServiceControl(const H225_ServiceControlDescriptor & contents)
{
  OnReceivedPDU(contents);
}


PBoolean H323HTTPServiceControl::IsValid() const
{
  return !url.IsEmpty();
}


PString H323HTTPServiceControl::GetServiceControlType() const
{
  return url;
}


PBoolean H323HTTPServiceControl::OnReceivedPDU(const H225_ServiceControlDescriptor & contents)
{
  if (contents.GetTag() != H225_ServiceControlDescriptor::e_url)
    return PFalse;

  const PASN_IA5String & pdu = contents;
  url = pdu;
  return PTrue;
}


PBoolean H323HTTPServiceControl::OnSendingPDU(H225_ServiceControlDescriptor & contents) const
{
  contents.SetTag(H225_ServiceControlDescriptor::e_url);
  PASN_IA5String & pdu = contents;
  pdu = url;

  return PTrue;
}


void H323HTTPServiceControl::OnChange(unsigned type,
                                      unsigned sessionId,
                                      H323EndPoint & endpoint,
                                      H323Connection * /*connection*/) const
{
  PTRACE(3, "SvcCtrl\tOnChange HTTP service control " << url);

  endpoint.OnHTTPServiceControl(type, sessionId, url);
}


/////////////////////////////////////////////////////////////////////////////

H323H248ServiceControl::H323H248ServiceControl()
{
}


H323H248ServiceControl::H323H248ServiceControl(const H225_ServiceControlDescriptor & contents)
{
  OnReceivedPDU(contents);
}


PBoolean H323H248ServiceControl::OnReceivedPDU(const H225_ServiceControlDescriptor & contents)
{
  if (contents.GetTag() != H225_ServiceControlDescriptor::e_signal)
    return PFalse;

  const H225_H248SignalsDescriptor & pdu = contents;

  H248_SignalsDescriptor signal;
  if (!pdu.DecodeSubType(signal))
    return PFalse;

  return OnReceivedPDU(signal);
}


PBoolean H323H248ServiceControl::OnSendingPDU(H225_ServiceControlDescriptor & contents) const
{
  contents.SetTag(H225_ServiceControlDescriptor::e_signal);
  H225_H248SignalsDescriptor & pdu = contents;

  H248_SignalsDescriptor signal;

  pdu.EncodeSubType(signal);

  return OnSendingPDU(signal);
}


PBoolean H323H248ServiceControl::OnReceivedPDU(const H248_SignalsDescriptor & descriptor)
{
  for (PINDEX i = 0; i < descriptor.GetSize(); i++) {
    if (!OnReceivedPDU(descriptor[i]))
      return PFalse;
  }

  return PTrue;
}


PBoolean H323H248ServiceControl::OnSendingPDU(H248_SignalsDescriptor & descriptor) const
{
  PINDEX last = descriptor.GetSize();
  descriptor.SetSize(last+1);
  return OnSendingPDU(descriptor[last]);
}


/////////////////////////////////////////////////////////////////////////////

H323CallCreditServiceControl::H323CallCreditServiceControl(const PString & amt,
                                                           PBoolean m,
                                                           unsigned dur)
  : amount(amt),
    mode(m),
    durationLimit(dur)
{
}


H323CallCreditServiceControl::H323CallCreditServiceControl(const H225_ServiceControlDescriptor & contents)
{
  OnReceivedPDU(contents);
}


PBoolean H323CallCreditServiceControl::IsValid() const
{
  return !amount || durationLimit > 0;
}


PBoolean H323CallCreditServiceControl::OnReceivedPDU(const H225_ServiceControlDescriptor & contents)
{
  if (contents.GetTag() != H225_ServiceControlDescriptor::e_callCreditServiceControl)
    return PFalse;

  const H225_CallCreditServiceControl & credit = contents;

  if (credit.HasOptionalField(H225_CallCreditServiceControl::e_amountString))
    amount = credit.m_amountString;

  if (credit.HasOptionalField(H225_CallCreditServiceControl::e_billingMode))
    mode = credit.m_billingMode.GetTag() == H225_CallCreditServiceControl_billingMode::e_debit;
  else
    mode = PTrue;

  if (credit.HasOptionalField(H225_CallCreditServiceControl::e_callDurationLimit))
    durationLimit = credit.m_callDurationLimit;
  else
    durationLimit = 0;

  return PTrue;
}


PBoolean H323CallCreditServiceControl::OnSendingPDU(H225_ServiceControlDescriptor & contents) const
{
  contents.SetTag(H225_ServiceControlDescriptor::e_callCreditServiceControl);
  H225_CallCreditServiceControl & credit = contents;

  if (!amount) {
    credit.IncludeOptionalField(H225_CallCreditServiceControl::e_amountString);
    credit.m_amountString = amount;

    credit.IncludeOptionalField(H225_CallCreditServiceControl::e_billingMode);
    credit.m_billingMode.SetTag(mode ? H225_CallCreditServiceControl_billingMode::e_debit
                                     : H225_CallCreditServiceControl_billingMode::e_credit);
  }

  if (durationLimit > 0) {
    credit.IncludeOptionalField(H225_CallCreditServiceControl::e_callDurationLimit);
    credit.m_callDurationLimit = durationLimit;
    credit.IncludeOptionalField(H225_CallCreditServiceControl::e_enforceCallDurationLimit);
    credit.m_enforceCallDurationLimit = PTrue;
  }

  return !amount || durationLimit > 0;
}


void H323CallCreditServiceControl::OnChange(unsigned /*type*/,
                                             unsigned /*sessionId*/,
                                             H323EndPoint & endpoint,
                                             H323Connection * connection) const
{
  PTRACE(3, "SvcCtrl\tOnChange Call Credit service control "
         << amount << (mode ? " debit " : " credit ") << durationLimit);

  endpoint.OnCallCreditServiceControl(amount, mode);
  if (durationLimit > 0 && connection != NULL)
    connection->SetEnforcedDurationLimit(durationLimit);
}


#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////
