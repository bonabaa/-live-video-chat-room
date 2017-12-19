/*
 * h501pdu.h
 *
 * H.501 protocol handler
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
 * $Revision: 21293 $
 * $Author: rjongbloed $
 * $Date: 2008-10-12 23:24:41 +0000 (Sun, 12 Oct 2008) $
 */

#ifndef OPAL_H323_H501PDU_H
#define OPAL_H323_H501PDU_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_H501

#include <ptlib/sockets.h>

#include <h323/transaddr.h>
#include <h323/h323trans.h>
#include <asn/h501.h>


class H323_AnnexG;


/**Wrapper class for the H501 Annex G channel.
 */
class H501PDU : public H501_Message, public H323TransactionPDU
{
  PCLASSINFO(H501PDU, H501_Message);

  public:
    H501PDU();

    // overrides from PObject
    virtual PObject * Clone() const;

    // overrides from H323TransactionPDU
    virtual PASN_Object & GetPDU();
    virtual PASN_Choice & GetChoice();
    virtual const PASN_Object & GetPDU() const;
    virtual const PASN_Choice & GetChoice() const;
    virtual unsigned GetSequenceNumber() const;
    virtual unsigned GetRequestInProgressDelay() const;
#if PTRACING
    virtual const char * GetProtocolName() const;
#endif
    virtual H323TransactionPDU * ClonePDU() const;
    virtual void DeletePDU();

    // new functions
    H501_ServiceRequest               & BuildServiceRequest              (unsigned seqnum, const H323TransportAddressArray & reply);
    H501_ServiceConfirmation          & BuildServiceConfirmation         (unsigned seqnum);
    H501_ServiceRejection             & BuildServiceRejection            (unsigned seqnum, unsigned reason);
    H501_ServiceRelease               & BuildServiceRelease              (unsigned seqnum);
    H501_DescriptorRequest            & BuildDescriptorRequest           (unsigned seqnum, const H323TransportAddressArray & reply);
    H501_DescriptorConfirmation       & BuildDescriptorConfirmation      (unsigned seqnum);
    H501_DescriptorRejection          & BuildDescriptorRejection         (unsigned seqnum, unsigned reason);
    H501_DescriptorIDRequest          & BuildDescriptorIDRequest         (unsigned seqnum, const H323TransportAddressArray & reply);
    H501_DescriptorIDConfirmation     & BuildDescriptorIDConfirmation    (unsigned seqnum);
    H501_DescriptorIDRejection        & BuildDescriptorIDRejection       (unsigned seqnum, unsigned reason);
    H501_DescriptorUpdate             & BuildDescriptorUpdate            (unsigned seqnum, const H323TransportAddressArray & reply);
    H501_DescriptorUpdateAck          & BuildDescriptorUpdateAck         (unsigned seqnum);
    H501_AccessRequest                & BuildAccessRequest               (unsigned seqnum, const H323TransportAddressArray & reply);
    H501_AccessConfirmation           & BuildAccessConfirmation          (unsigned seqnum);
    H501_AccessRejection              & BuildAccessRejection             (unsigned seqnum, int reason);
    H501_RequestInProgress            & BuildRequestInProgress           (unsigned seqnum, unsigned delay);
    H501_NonStandardRequest           & BuildNonStandardRequest          (unsigned seqnum, const H323TransportAddressArray & reply);
    H501_NonStandardConfirmation      & BuildNonStandardConfirmation     (unsigned seqnum);
    H501_NonStandardRejection         & BuildNonStandardRejection        (unsigned seqnum, unsigned reason);
    H501_UnknownMessageResponse       & BuildUnknownMessageResponse      (unsigned seqnum);
    H501_UsageRequest                 & BuildUsageRequest                (unsigned seqnum, const H323TransportAddressArray & reply);
    H501_UsageConfirmation            & BuildUsageConfirmation           (unsigned seqnum);
    H501_UsageIndicationConfirmation  & BuildUsageIndicationConfirmation (unsigned seqnum);
    H501_UsageIndicationRejection     & BuildUsageIndicationRejection    (unsigned seqnum, unsigned reason);
    H501_UsageRejection               & BuildUsageRejection              (unsigned seqnum);
    H501_ValidationRequest            & BuildValidationRequest           (unsigned seqnum, const H323TransportAddressArray & reply);
    H501_ValidationConfirmation       & BuildValidationConfirmation      (unsigned seqnum);
    H501_ValidationRejection          & BuildValidationRejection         (unsigned seqnum, unsigned reason);
    H501_AuthenticationRequest        & BuildAuthenticationRequest       (unsigned seqnum, const H323TransportAddressArray & reply);
    H501_AuthenticationConfirmation   & BuildAuthenticationConfirmation  (unsigned seqnum);
    H501_AuthenticationRejection      & BuildAuthenticationRejection     (unsigned seqnum, unsigned reason);

  protected:
    void BuildRequest(unsigned tag, unsigned seqnum, const H323TransportAddressArray & replyAddr);
    void BuildPDU(unsigned tag, unsigned seqnum);
};


#endif // OPAL_H501

#endif  // OPAL_H323_H501PDU_H


/////////////////////////////////////////////////////////////////////////////
