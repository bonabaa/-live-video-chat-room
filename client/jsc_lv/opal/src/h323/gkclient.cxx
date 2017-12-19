/*
 * gkclient.cxx
 *
 * Gatekeeper client protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1999-2000 Equivalence Pty. Ltd.
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
 * Portions of this code were written with the assisance of funding from
 * iFace In, http://www.iface.com
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23771 $
 * $Author: rjongbloed $
 * $Date: 2009-11-17 01:24:47 +0000 (Tue, 17 Nov 2009) $
 */

#include <ptlib.h>

#include <opal/buildopts.h>
#if OPAL_H323

#ifdef __GNUC__
#pragma implementation "gkclient.h"
#endif

#include <h323/gkclient.h>

#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/h323pdu.h>
#include <h323/h323rtp.h>

#if OPAL_H460
#include <h460/h4601.h>
#endif

#define new PNEW


static PTimeInterval AdjustTimeout(unsigned seconds)
{
  // Allow for an incredible amount of system/network latency
  static unsigned TimeoutDeadband = 5; // seconds

  return PTimeInterval(0, seconds > TimeoutDeadband
                              ? (seconds - TimeoutDeadband)
                              : TimeoutDeadband);
}


/////////////////////////////////////////////////////////////////////////////

H323Gatekeeper::H323Gatekeeper(H323EndPoint & ep, H323Transport * trans)
  : H225_RAS(ep, trans)
#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif
  , highPriorityMonitor(*this, HighPriority)
  , lowPriorityMonitor(*this, LowPriority)
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
  , requestMutex(1, 1)
  , authenticators(ep.CreateAuthenticators())
#if OPAL_H460
  , features(ep.GetFeatureSet())
#endif
{
  alternatePermanent = PFalse;
  discoveryComplete = PFalse;
  registrationFailReason = UnregisteredLocally;

  pregrantMakeCall = pregrantAnswerCall = RequireARQ;

  autoReregister = PTrue;
  reregisterNow = PFalse;
  requiresDiscovery = PFalse;

  timeToLive.SetNotifier(PCREATE_NOTIFIER(TickleMonitor));
  infoRequestRate.SetNotifier(PCREATE_NOTIFIER(TickleMonitor));

  willRespondToIRR = PFalse;
  monitorStop = PFalse;

  monitor = PThread::Create(PCREATE_NOTIFIER(MonitorMain), "GkMonitor");
  
#if OPAL_H460
  features->AttachEndPoint(&ep);
  features->LoadFeatureSet(H460_Feature::FeatureRas);
#endif
}


H323Gatekeeper::~H323Gatekeeper()
{
  if (monitor != NULL) {
    monitorStop = PTrue;
    monitorTickle.Signal();
    monitor->WaitForTermination();
    delete monitor;
    monitor = NULL;
  }

  StopChannel();

#ifdef H323_H460
  delete features;
#endif
}


PString H323Gatekeeper::GetName() const
{
  PStringStream s;
  s << *this;
  return s;
}


PBoolean H323Gatekeeper::DiscoverAny()
{
  gatekeeperIdentifier = PString();
  return StartGatekeeper(H323TransportAddress());
}


PBoolean H323Gatekeeper::DiscoverByName(const PString & identifier)
{
  gatekeeperIdentifier = identifier;
  return StartGatekeeper(H323TransportAddress());
}


PBoolean H323Gatekeeper::DiscoverByAddress(const H323TransportAddress & address)
{
  gatekeeperIdentifier = PString();
  return StartGatekeeper(address);
}


PBoolean H323Gatekeeper::DiscoverByNameAndAddress(const PString & identifier,
                                              const H323TransportAddress & address)
{
  gatekeeperIdentifier = identifier;
  return StartGatekeeper(address);
}


static PBoolean WriteGRQ(H323Transport & transport, void * param)
{
  H323RasPDU & pdu = *(H323RasPDU *)param;

  OpalEndPoint & endpoint = transport.GetEndPoint();

  H323TransportAddress address = transport.GetLocalAddress();

  // Get interface we are about to send PDU out on
  PIPSocket::Address localAddress;
  WORD localPort;
  if (!address.GetIpAndPort(localAddress, localPort))
    return false;

  // Check if interface is used by signalling channel listeners
  OpalTransportAddressArray interfaces = endpoint.GetInterfaceAddresses(true, &transport);
  bool notInterface = true;
  for (PINDEX i = 0; i < interfaces.GetSize(); ++i) {
    PIPSocket::Address ifAddress;
    if (interfaces[i].GetIpAddress(ifAddress) && ifAddress == localAddress) {
      notInterface = false;
      break;
    }
  }
  if (notInterface) {
    PTRACE(3, "RAS\tNot sending GRQ on " << localAddress << " as no signalling chanel is listening there.");
    return true;
  }

  // We also have to use the translated address if one is specified
  PIPSocket::Address remoteAddress;
  if (transport.GetRemoteAddress().GetIpAddress(remoteAddress) &&
      endpoint.GetManager().TranslateIPAddress(localAddress, remoteAddress))
    address = H323TransportAddress(localAddress, localPort);

  // Adjust out outgoing local address in PDU
  H225_GatekeeperRequest & grq = pdu;
  address.SetPDU(grq.m_rasAddress);
  return pdu.Write(transport);
}


bool H323Gatekeeper::StartGatekeeper(const H323TransportAddress & initialAddress)
{
  if (PAssertNULL(transport) == NULL)
    return false;

  PAssert(!transport->IsRunning(), "Cannot do initial discovery on running RAS channel");

  H323TransportAddress address = initialAddress;
  if (address.IsEmpty())
    address = "udp$*:1719";

  if (!transport->ConnectTo(address))
    return false;

  if (!StartChannel())
    return false;

  reregisterNow = true;

  // Make sure transctor (socket read) thread has started, before kicking off GRQ
  timeToLive.SetInterval(500);
  return true;
}


bool H323Gatekeeper::DiscoverGatekeeper()
{
  discoveryComplete = false;
  
  H323RasPDU pdu;
  Request request(SetupGatekeeperRequest(pdu), pdu);
  
  H323TransportAddress address = transport->GetRemoteAddress();
  request.responseInfo = &address;
  
  requestsMutex.Wait();
  requests.SetAt(request.sequenceNumber, &request);
  requestsMutex.Signal();
  
  request.Poll(*this, endpoint.GetGatekeeperRequestRetries(), endpoint.GetGatekeeperRequestTimeout());
  
  requestsMutex.Wait();
  requests.SetAt(request.sequenceNumber, NULL);
  requestsMutex.Signal();
  
  return discoveryComplete;
}


PBoolean H323Gatekeeper::WriteTo(H323TransactionPDU & pdu, 
                                 const H323TransportAddressArray & addresses, 
                                 PBoolean callback)
{
  PWaitAndSignal mutex(transport->GetWriteMutex());

  if (discoveryComplete || pdu.GetPDU().GetTag() != H225_RasMessage::e_gatekeeperRequest)
    return H323Transactor::WriteTo(pdu, addresses, callback);

  PString oldInterface = transport->GetInterface();
  bool ok = transport->WriteConnect(WriteGRQ, &pdu.GetPDU());
  transport->SetInterface(oldInterface);

  PTRACE_IF(1, !ok, "RAS\tError writing discovery PDU: " << transport->GetErrorText());

  return ok;
}


unsigned H323Gatekeeper::SetupGatekeeperRequest(H323RasPDU & request)
{
  if (PAssertNULL(transport) == NULL)
    return 0;

  H225_GatekeeperRequest & grq = request.BuildGatekeeperRequest(GetNextSequenceNumber());

  H323TransportAddress rasAddress = transport->GetLocalAddress();
  rasAddress.SetPDU(grq.m_rasAddress);

  endpoint.SetEndpointTypeInfo(grq.m_endpointType);

  grq.IncludeOptionalField(H225_GatekeeperRequest::e_endpointAlias);
  H323SetAliasAddresses(endpoint.GetAliasNames(), grq.m_endpointAlias);

  if (!gatekeeperIdentifier) {
    grq.IncludeOptionalField(H225_GatekeeperRequest::e_gatekeeperIdentifier);
    grq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  grq.IncludeOptionalField(H225_GatekeeperRequest::e_supportsAltGK);

  H225_RAS::OnSendGatekeeperRequest(request, grq);

  discoveryComplete = PFalse;

  return grq.m_requestSeqNum;
}


void H323Gatekeeper::OnSendGatekeeperRequest(H225_GatekeeperRequest & grq)
{
  H225_RAS::OnSendGatekeeperRequest(grq);

  for (H235Authenticators::iterator iterAuth = authenticators.begin(); iterAuth != authenticators.end(); ++iterAuth) {
    if (iterAuth->SetCapability(grq.m_authenticationCapability, grq.m_algorithmOIDs)) {
      grq.IncludeOptionalField(H225_GatekeeperRequest::e_authenticationCapability);
      grq.IncludeOptionalField(H225_GatekeeperRequest::e_algorithmOIDs);
    }
  }
}


PBoolean H323Gatekeeper::OnReceiveGatekeeperConfirm(const H225_GatekeeperConfirm & gcf)
{
  if (!H225_RAS::OnReceiveGatekeeperConfirm(gcf))
    return PFalse;

  for (H235Authenticators::iterator iterAuth = authenticators.begin(); iterAuth != authenticators.end(); ++iterAuth) {
    if (iterAuth->UseGkAndEpIdentifiers())
      iterAuth->SetRemoteId(gatekeeperIdentifier);
  }

  if (gcf.HasOptionalField(H225_GatekeeperConfirm::e_authenticationMode) &&
      gcf.HasOptionalField(H225_GatekeeperConfirm::e_algorithmOID)) {
    for (H235Authenticators::iterator iterAuth = authenticators.begin(); iterAuth != authenticators.end(); ++iterAuth)
      iterAuth->Enable(iterAuth->IsCapability(gcf.m_authenticationMode, gcf.m_algorithmOID));
  }

  {
    PWaitAndSignal mutex(transport->GetWriteMutex());

    H323TransportAddress locatedAddress(gcf.m_rasAddress, "udp");
    if (!transport->SetRemoteAddress(locatedAddress)) {
      PTRACE(2, "RAS\tInvalid gatekeeper discovery address: \"" << locatedAddress << '"');
      return PFalse;
    }

    transport->SetInterface(transport->GetLastReceivedInterface());

    PTRACE(3, "RAS\tGatekeeper discovered at: "
           << transport->GetRemoteAddress()
           << " (if=" << transport->GetLocalAddress() << ')');
  }

  if (gcf.HasOptionalField(H225_GatekeeperConfirm::e_alternateGatekeeper))
    SetAlternates(gcf.m_alternateGatekeeper, PFalse);

  endpoint.OnGatekeeperConfirm();

  discoveryComplete = PTrue;
  return PTrue;
}


PBoolean H323Gatekeeper::OnReceiveGatekeeperReject(const H225_GatekeeperReject & grj)
{
  if (!H225_RAS::OnReceiveGatekeeperReject(grj))
    return PFalse;

  if (grj.HasOptionalField(H225_GatekeeperReject::e_altGKInfo)) {
    SetAlternates(grj.m_altGKInfo.m_alternateGatekeeper,
                  grj.m_altGKInfo.m_altGKisPermanent);

    if (lastRequest->responseInfo != NULL) {
      H323TransportAddress & gkAddress = *(H323TransportAddress *)lastRequest->responseInfo;
      gkAddress = alternates[0].rasAddress;
    }
  }

  endpoint.OnGatekeeperReject();

  return PTrue;
}


bool H323Gatekeeper::SetListenerAddresses(H225_ArrayOf_TransportAddress & pdu)
{
  H323TransportAddressArray listeners = endpoint.GetInterfaceAddresses(true, transport);
  if (listeners.IsEmpty())
    return false;

  for (PINDEX i = 0; i < listeners.GetSize(); i++) {
    if (listeners[i].GetProto() != "tcp")
      continue;

    H225_TransportAddress pduAddr;
    if (!listeners[i].SetPDU(pduAddr))
      continue;

    // Check for already have had that address.
    PINDEX pos, lastPos = pdu.GetSize();
    for (pos = 0; pos < lastPos; pos++) {
      if (pdu[pos] == pduAddr)
        break;
    }

    if (pos >= lastPos) {
      // Put new listener into array
      pdu.SetSize(lastPos+1);
      pdu[lastPos] = pduAddr;
    }
  }

  return pdu.GetSize() > 0;
}


PBoolean H323Gatekeeper::RegistrationRequest(PBoolean autoReg, PBoolean didGkDiscovery)
{
  if (PAssertNULL(transport) == NULL)
    return PFalse;

  autoReregister = autoReg;

  H323RasPDU pdu;
  H225_RegistrationRequest & rrq = pdu.BuildRegistrationRequest(GetNextSequenceNumber());

  // the discoveryComplete flag of the RRQ is not to be confused with the
  // discoveryComplete member variable of this instance...
  rrq.m_discoveryComplete = didGkDiscovery;

  rrq.m_rasAddress.SetSize(1);
  H323TransportAddress rasAddress = transport->GetLocalAddress();
  
  // We do have to use the translated address if specified
  PIPSocket::Address localAddress, remoteAddress;
  WORD localPort;
  
  if (rasAddress.GetIpAndPort(localAddress, localPort) &&
      transport->GetRemoteAddress().GetIpAddress(remoteAddress) &&
      transport->GetEndPoint().GetManager().TranslateIPAddress(localAddress, remoteAddress))
    rasAddress = H323TransportAddress(localAddress, localPort);
  
  rasAddress.SetPDU(rrq.m_rasAddress[0]);

  if (!SetListenerAddresses(rrq.m_callSignalAddress)) {
    PTRACE(1, "RAS\tCannot register with Gatekeeper without a H323Listener!");
    return false;
  }

  endpoint.SetEndpointTypeInfo(rrq.m_terminalType);
  endpoint.SetVendorIdentifierInfo(rrq.m_endpointVendor);

  rrq.IncludeOptionalField(H225_RegistrationRequest::e_terminalAlias);
  H323SetAliasAddresses(endpoint.GetAliasNames(), rrq.m_terminalAlias);

  PStringArray aliasNamePatterns = endpoint.GetAliasNamePatterns();
  if(aliasNamePatterns.GetSize() > 0) { //set patterns,
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_terminalAliasPattern);
    rrq.m_terminalAliasPattern.Append(new H225_AddressPattern);
    H225_AddressPattern &addressPattern = rrq.m_terminalAliasPattern[0];
    
    for( int i = 0; i < aliasNamePatterns.GetSize(); i++){
      PStringArray nameRange = aliasNamePatterns[i].Tokenise('-', FALSE);
      if (nameRange.GetSize() == 2 &&
          nameRange[0].FindSpan("1234567890*#") == P_MAX_INDEX &&
          nameRange[1].FindSpan("1234567890*#") == P_MAX_INDEX) {
        addressPattern.SetTag(H225_AddressPattern::e_range);
        H225_AddressPattern_range &addressPattern_range = addressPattern;

        addressPattern_range.m_startOfRange.SetTag(H225_PartyNumber::e_e164Number);
        H225_PublicPartyNumber &start= addressPattern_range.m_startOfRange;
        start.m_publicNumberDigits = nameRange[0];
        start.m_publicTypeOfNumber.SetTag(H225_PublicTypeOfNumber::e_unknown ); 

        addressPattern_range.m_endOfRange.SetTag(H225_PartyNumber::e_e164Number);
        H225_PublicPartyNumber &end= addressPattern_range.m_endOfRange;
        end.m_publicNumberDigits = nameRange[1];
        end.m_publicTypeOfNumber.SetTag(H225_PublicTypeOfNumber::e_unknown ); 
      }
      else { //a wildcard
        addressPattern.SetTag(H225_AddressPattern::e_wildcard);
        H225_AliasAddress &aliasAddress = addressPattern;
        H323SetAliasAddress(aliasNamePatterns[i], aliasAddress, H225_AliasAddress::e_dialedDigits);
      }
    }
  }

  rrq.m_willSupplyUUIEs = PTrue;
  rrq.IncludeOptionalField(H225_RegistrationRequest::e_usageReportingCapability);
  rrq.m_usageReportingCapability.IncludeOptionalField(H225_RasUsageInfoTypes::e_startTime);
  rrq.m_usageReportingCapability.IncludeOptionalField(H225_RasUsageInfoTypes::e_endTime);
  rrq.m_usageReportingCapability.IncludeOptionalField(H225_RasUsageInfoTypes::e_terminationCause);
  rrq.IncludeOptionalField(H225_RegistrationRequest::e_supportsAltGK);

  if (!gatekeeperIdentifier) {
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_gatekeeperIdentifier);
    rrq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  if (!endpointIdentifier.IsEmpty()) {
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_endpointIdentifier);
    rrq.m_endpointIdentifier = endpointIdentifier;
  }

  PTimeInterval ttl = endpoint.GetGatekeeperTimeToLive();
  if (ttl > 0) {
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_timeToLive);
    rrq.m_timeToLive = (int)ttl.GetSeconds();
  }

  if (endpoint.CanDisplayAmountString()) {
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_callCreditCapability);
    rrq.m_callCreditCapability.IncludeOptionalField(H225_CallCreditCapability::e_canDisplayAmountString);
    rrq.m_callCreditCapability.m_canDisplayAmountString = PTrue;
  }

  if (endpoint.CanEnforceDurationLimit()) {
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_callCreditCapability);
    rrq.m_callCreditCapability.IncludeOptionalField(H225_CallCreditCapability::e_canEnforceDurationLimit);
    rrq.m_callCreditCapability.m_canEnforceDurationLimit = PTrue;
  }

  if (IsRegistered()) { // send lightweight RRQ
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_keepAlive);
    rrq.m_keepAlive = PTrue;
  }

  Request request(rrq.m_requestSeqNum, pdu);
  if (MakeRequest(request))
    return PTrue;

  PTRACE(3, "RAS\tFailed registration of " << endpointIdentifier << " with " << gatekeeperIdentifier);
  switch (request.responseResult) {
    case Request::RejectReceived :
      switch (request.rejectReason) {
        case H225_RegistrationRejectReason::e_discoveryRequired :
          // If have been told by GK that we need to discover it again, set flag
          // for next register done by timeToLive handler to do discovery
          requiresDiscovery = PTrue;
          // Do next case

        case H225_RegistrationRejectReason::e_fullRegistrationRequired :
          registrationFailReason = GatekeeperLostRegistration;
          // Set timer to retry registration
          reregisterNow = PTrue;
          monitorTickle.Signal();
          break;

        // Onse below here are permananent errors, so don't try again
        case H225_RegistrationRejectReason::e_invalidCallSignalAddress :
          registrationFailReason = InvalidListener;
          break;

        case H225_RegistrationRejectReason::e_duplicateAlias :
          registrationFailReason = DuplicateAlias;
          break;

        case H225_RegistrationRejectReason::e_securityDenial :
          registrationFailReason = SecurityDenied;
          break;

        default :
          registrationFailReason = (RegistrationFailReasons)(request.rejectReason|RegistrationRejectReasonMask);
          break;
      }
      break;

    case Request::BadCryptoTokens :
      registrationFailReason = SecurityDenied;
      break;

    default :
      registrationFailReason = TransportError;
      break;
  }

  return PFalse;
}


PBoolean H323Gatekeeper::OnReceiveRegistrationConfirm(const H225_RegistrationConfirm & rcf)
{
  if (!H225_RAS::OnReceiveRegistrationConfirm(rcf))
    return PFalse;

  registrationFailReason = RegistrationSuccessful;
  reregisterNow = false;

  endpointIdentifier = rcf.m_endpointIdentifier;
  PTRACE(3, "RAS\tRegistered " << endpointIdentifier << " with " << gatekeeperIdentifier);


  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_alternateGatekeeper))
    SetAlternates(rcf.m_alternateGatekeeper, PFalse);

  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_timeToLive))
    timeToLive = AdjustTimeout(rcf.m_timeToLive);
  else
    timeToLive = 0; // zero disables lightweight RRQ

  // At present only support first call signal address to GK
  if (rcf.m_callSignalAddress.GetSize() > 0)
    gkRouteAddress = rcf.m_callSignalAddress[0];

  willRespondToIRR = rcf.m_willRespondToIRR;

  pregrantMakeCall = pregrantAnswerCall = RequireARQ;
  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_preGrantedARQ)) {
    if (rcf.m_preGrantedARQ.m_makeCall)
      pregrantMakeCall = rcf.m_preGrantedARQ.m_useGKCallSignalAddressToMakeCall
                                                      ? PreGkRoutedARQ : PregrantARQ;
    if (rcf.m_preGrantedARQ.m_answerCall)
      pregrantAnswerCall = rcf.m_preGrantedARQ.m_useGKCallSignalAddressToAnswer
                                                      ? PreGkRoutedARQ : PregrantARQ;
    if (rcf.m_preGrantedARQ.HasOptionalField(H225_RegistrationConfirm_preGrantedARQ::e_irrFrequencyInCall))
      SetInfoRequestRate(AdjustTimeout(rcf.m_preGrantedARQ.m_irrFrequencyInCall));
    else
      ClearInfoRequestRate();
  }
  else
    ClearInfoRequestRate();

  // Remove the endpoint aliases that the gatekeeper did not like and add the
  // ones that it really wants us to be.
  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_terminalAlias)) {
    const PStringList & currentAliases = endpoint.GetAliasNames();
    PStringList aliasesToChange;
    PINDEX i, j;

    for (i = 0; i < rcf.m_terminalAlias.GetSize(); i++) {
      PString alias = H323GetAliasAddressString(rcf.m_terminalAlias[i]);
      if (!alias) {
        PStringList::const_iterator currentAlias;
        for (currentAlias = currentAliases.begin(); currentAlias != currentAliases.end(); ++currentAlias) {
          if (alias *= *currentAlias)
            break;
        }
        if (currentAlias == currentAliases.end())
          aliasesToChange.AppendString(alias);
      }
    }
    for (PStringList::iterator alias = aliasesToChange.begin(); alias != aliasesToChange.end(); ++alias) {
      PTRACE(3, "RAS\tGatekeeper add of alias \"" << *alias << '"');
      endpoint.AddAliasName(*alias);
    }

    aliasesToChange.RemoveAll();

    for (PStringList::const_iterator alias = currentAliases.begin(); alias != currentAliases.end(); ++alias) {
      for (j = 0; j < rcf.m_terminalAlias.GetSize(); j++) {
        if (*alias *= H323GetAliasAddressString(rcf.m_terminalAlias[j]))
          break;
      }
      if (j >= rcf.m_terminalAlias.GetSize())
        aliasesToChange.AppendString(*alias);
    }
    for (PStringList::iterator alias = aliasesToChange.begin(); alias != aliasesToChange.end(); ++alias) {
      PTRACE(3, "RAS\tGatekeeper removal of alias \"" << *alias << '"');
      endpoint.RemoveAliasName(*alias);
    }
  }

  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_serviceControl))
    OnServiceControlSessions(rcf.m_serviceControl, NULL);

  // NAT Detection with GNUGK
  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_nonStandardData)) {
    PString NATaddr = rcf.m_nonStandardData.m_data.AsString();
    if ((!NATaddr.IsEmpty()) && (NATaddr.Left(4) == "NAT="))
      endpoint.OnGatekeeperNATDetect(NATaddr.Right(NATaddr.GetLength()-4),endpointIdentifier,gkRouteAddress);
  }
  
  endpoint.OnRegistrationConfirm();

  return PTrue;
}


PBoolean H323Gatekeeper::OnReceiveRegistrationReject(const H225_RegistrationReject & rrj)
{
  if (!H225_RAS::OnReceiveRegistrationReject(rrj))
    return PFalse;

  if (rrj.HasOptionalField(H225_RegistrationReject::e_altGKInfo))
    SetAlternates(rrj.m_altGKInfo.m_alternateGatekeeper,
                  rrj.m_altGKInfo.m_altGKisPermanent);

  // Update registration fail reason from last request fail reason
  switch(lastRequest->rejectReason) {
	case H225_RegistrationRejectReason::e_duplicateAlias : registrationFailReason = DuplicateAlias; break;
	case H225_RegistrationRejectReason::e_securityDenial: registrationFailReason = SecurityDenied; break;
	default: ;
  }
  
  endpoint.OnRegistrationReject();

  return PTrue;
}


void H323Gatekeeper::RegistrationTimeToLive()
{
  PTRACE(3, "RAS\tTime To Live reregistration");
  
  PBoolean didGkDiscovery = false;

  if (!discoveryComplete) {
    timeToLive.SetInterval(0, 0, 1);
    if (endpoint.GetSendGRQ()) {
      if (!DiscoverGatekeeper()) {
        PTRACE_IF(2, !reregisterNow, "RAS\tDiscovery failed, retrying in 1 minute");
        return;
      }
      requiresDiscovery = false;
      didGkDiscovery = true;
    }
    else {
      PTRACE_IF(3, !requiresDiscovery, "RAS\tSkipping gatekeeper discovery for " << transport->GetRemoteAddress());
      discoveryComplete = true;
    }
  }

  if (requiresDiscovery) {
    PTRACE(3, "RAS\tRepeating discovery on gatekeepers request.");

    H323RasPDU pdu;
    Request request(SetupGatekeeperRequest(pdu), pdu);
    if (!MakeRequest(request) || !discoveryComplete) {
      PTRACE(2, "RAS\tRediscovery failed, retrying in 1 minute.");
      timeToLive = PTimeInterval(0, 0, 1);
      return;
    }

    requiresDiscovery = PFalse;
    didGkDiscovery = true;
  }

  if (!RegistrationRequest(autoReregister, didGkDiscovery)) {
    PTRACE_IF(2, !reregisterNow, "RAS\tTime To Live reregistration failed, retrying in 1 minute");
    timeToLive = PTimeInterval(0, 0, 1);
  }
}


PBoolean H323Gatekeeper::UnregistrationRequest(int reason)
{
  if (PAssertNULL(transport) == NULL)
    return PFalse;

  PINDEX i;
  H323RasPDU pdu;
  H225_UnregistrationRequest & urq = pdu.BuildUnregistrationRequest(GetNextSequenceNumber());

  H323TransportAddress rasAddress = transport->GetLocalAddress();

  SetListenerAddresses(urq.m_callSignalAddress);

  urq.IncludeOptionalField(H225_UnregistrationRequest::e_endpointAlias);
  H323SetAliasAddresses(endpoint.GetAliasNames(), urq.m_endpointAlias);

  if (!gatekeeperIdentifier) {
    urq.IncludeOptionalField(H225_UnregistrationRequest::e_gatekeeperIdentifier);
    urq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  if (!endpointIdentifier.IsEmpty()) {
    urq.IncludeOptionalField(H225_UnregistrationRequest::e_endpointIdentifier);
    urq.m_endpointIdentifier = endpointIdentifier;
  }

  if (reason >= 0) {
    urq.IncludeOptionalField(H225_UnregistrationRequest::e_reason);
    urq.m_reason = reason;
  }

  Request request(urq.m_requestSeqNum, pdu);

  PBoolean requestResult = MakeRequest(request);

  for (i = 0; i < alternates.GetSize(); i++) {
    AlternateInfo & altgk = alternates[i];
    if (altgk.registrationState == AlternateInfo::IsRegistered) {
      Connect(altgk.rasAddress,altgk.gatekeeperIdentifier);
      UnregistrationRequest(reason);
    }
  }

  if (requestResult)
    return PTrue;

  switch (request.responseResult) {
    case Request::NoResponseReceived :
      registrationFailReason = TransportError;
      timeToLive = 0; // zero disables lightweight RRQ
      break;

    case Request::BadCryptoTokens :
      registrationFailReason = SecurityDenied;
      timeToLive = 0; // zero disables lightweight RRQ
      break;

    default :
      break;
  }

  return !IsRegistered();
}


PBoolean H323Gatekeeper::OnReceiveUnregistrationConfirm(const H225_UnregistrationConfirm & ucf)
{
  if (!H225_RAS::OnReceiveUnregistrationConfirm(ucf))
    return PFalse;

  registrationFailReason = UnregisteredLocally;
  timeToLive = 0; // zero disables lightweight RRQ

  return PTrue;
}


PBoolean H323Gatekeeper::OnReceiveUnregistrationRequest(const H225_UnregistrationRequest & urq)
{
  if (!H225_RAS::OnReceiveUnregistrationRequest(urq))
    return PFalse;

  PTRACE(3, "RAS\tUnregistration received");
  if (!urq.HasOptionalField(H225_UnregistrationRequest::e_gatekeeperIdentifier) ||
       urq.m_gatekeeperIdentifier.GetValue() != gatekeeperIdentifier) {
    PTRACE(2, "RAS\tInconsistent gatekeeperIdentifier!");
    return PFalse;
  }

  if (!urq.HasOptionalField(H225_UnregistrationRequest::e_endpointIdentifier) ||
       urq.m_endpointIdentifier.GetValue() != endpointIdentifier) {
    PTRACE(2, "RAS\tInconsistent endpointIdentifier!");
    return PFalse;
  }

  endpoint.ClearAllCalls(H323Connection::EndedByGatekeeper, PFalse);

  PTRACE(3, "RAS\tUnregistered, calls cleared");
  registrationFailReason = UnregisteredByGatekeeper;
  timeToLive = 0; // zero disables lightweight RRQ

  if (urq.HasOptionalField(H225_UnregistrationRequest::e_alternateGatekeeper))
    SetAlternates(urq.m_alternateGatekeeper, PFalse);

  H323RasPDU response(authenticators);
  response.BuildUnregistrationConfirm(urq.m_requestSeqNum);
  PBoolean ok = WritePDU(response);

  if (autoReregister) {
    PTRACE(4, "RAS\tReregistering by setting timeToLive");
    discoveryComplete = PFalse;
    reregisterNow = PTrue;
    monitorTickle.Signal();
  }

  return ok;
}


PBoolean H323Gatekeeper::OnReceiveUnregistrationReject(const H225_UnregistrationReject & urj)
{
  if (!H225_RAS::OnReceiveUnregistrationReject(urj))
    return PFalse;

  if (lastRequest->rejectReason != H225_UnregRejectReason::e_callInProgress) {
    registrationFailReason = UnregisteredLocally;
    timeToLive = 0; // zero disables lightweight RRQ
  }

  return PTrue;
}


PBoolean H323Gatekeeper::LocationRequest(const PString & alias,
                                     H323TransportAddress & address)
{
  PStringList aliases;
  aliases.AppendString(alias);
  return LocationRequest(aliases, address);
}


PBoolean H323Gatekeeper::LocationRequest(const PStringList & aliases,
                                     H323TransportAddress & address)
{
  if (PAssertNULL(transport) == NULL)
    return PFalse;

  H323RasPDU pdu;
  H225_LocationRequest & lrq = pdu.BuildLocationRequest(GetNextSequenceNumber());

  H323SetAliasAddresses(aliases, lrq.m_destinationInfo);

  if (!endpointIdentifier.IsEmpty()) {
    lrq.IncludeOptionalField(H225_LocationRequest::e_endpointIdentifier);
    lrq.m_endpointIdentifier = endpointIdentifier;
  }

  H323TransportAddress replyAddress = transport->GetLocalAddress();
  replyAddress.SetPDU(lrq.m_replyAddress);

  lrq.IncludeOptionalField(H225_LocationRequest::e_sourceInfo);
  H323SetAliasAddresses(endpoint.GetAliasNames(), lrq.m_sourceInfo);

  if (!gatekeeperIdentifier) {
    lrq.IncludeOptionalField(H225_LocationRequest::e_gatekeeperIdentifier);
    lrq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  Request request(lrq.m_requestSeqNum, pdu);
  request.responseInfo = &address;
  if (!MakeRequest(request))
    return PFalse;

  // sanity check the address - some Gks return address 0.0.0.0 and port 0
  PIPSocket::Address ipAddr;
  WORD port;
  return address.GetIpAndPort(ipAddr, port) && (port != 0);
}


H323Gatekeeper::AdmissionResponse::AdmissionResponse()
{
  rejectReason = UINT_MAX;

  gatekeeperRouted = PFalse;
  endpointCount = 1;
  transportAddress = NULL;
  accessTokenData = NULL;

  aliasAddresses = NULL;
  destExtraCallInfo = NULL;
}


struct AdmissionRequestResponseInfo {
  AdmissionRequestResponseInfo(
    H323Gatekeeper::AdmissionResponse & r,
    H323Connection & c
  ) : param(r), connection(c) { }

  H323Gatekeeper::AdmissionResponse & param;
  H323Connection & connection;
  unsigned allocatedBandwidth;
  unsigned uuiesRequested;
  PString      accessTokenOID1;
  PString      accessTokenOID2;
};


PBoolean H323Gatekeeper::AdmissionRequest(H323Connection & connection,
                                      AdmissionResponse & response,
                                      PBoolean ignorePreGrantedARQ)
{
  PBoolean answeringCall = connection.HadAnsweredCall();

  if (!ignorePreGrantedARQ) {
    switch (answeringCall ? pregrantAnswerCall : pregrantMakeCall) {
      case RequireARQ :
        break;
      case PregrantARQ :
        return PTrue;
      case PreGkRoutedARQ :
        if (gkRouteAddress.IsEmpty()) {
          response.rejectReason = UINT_MAX;
          return PFalse;
        }
        if (response.transportAddress != NULL)
          *response.transportAddress = gkRouteAddress;
        response.gatekeeperRouted = PTrue;
        return PTrue;
    }
  }

  H323RasPDU pdu;
  H225_AdmissionRequest & arq = pdu.BuildAdmissionRequest(GetNextSequenceNumber());

  arq.m_callType.SetTag(H225_CallType::e_pointToPoint);
  arq.m_endpointIdentifier = endpointIdentifier;
  arq.m_answerCall = answeringCall;
  arq.m_canMapAlias = PTrue; // Stack supports receiving a different number in the ACF 
                            // to the one sent in the ARQ
  arq.m_willSupplyUUIEs = PTrue;

  if (!gatekeeperIdentifier) {
    arq.IncludeOptionalField(H225_AdmissionRequest::e_gatekeeperIdentifier);
    arq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  PString destInfo = connection.GetRemotePartyName();
  arq.m_srcInfo.SetSize(1);
  if (answeringCall) {
    H323SetAliasAddress(destInfo, arq.m_srcInfo[0]);

    if (!connection.GetLocalPartyName()) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_destinationInfo);
      H323SetAliasAddresses(connection.GetLocalAliasNames(), arq.m_destinationInfo);
    }
  }
  else {
    H323SetAliasAddresses(connection.GetLocalAliasNames(), arq.m_srcInfo);
    if (response.transportAddress == NULL || destInfo != *response.transportAddress) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_destinationInfo);
      arq.m_destinationInfo.SetSize(1);
      H323SetAliasAddress(destInfo, arq.m_destinationInfo[0]);
    }
  }

  PTRACE(3, "RAS\tAdmissionRequest answering = " << answeringCall << " local alias name " << connection.GetLocalAliasNames());

  const H323Transport * signallingChannel = connection.GetSignallingChannel();
  if (answeringCall) {
    arq.IncludeOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress);
    H323TransportAddress signalAddress = signallingChannel->GetRemoteAddress();
    signalAddress.SetPDU(arq.m_srcCallSignalAddress);
    signalAddress = signallingChannel->GetLocalAddress();
    arq.IncludeOptionalField(H225_AdmissionRequest::e_destCallSignalAddress);
    signalAddress.SetPDU(arq.m_destCallSignalAddress);
  }
  else {
    if (signallingChannel != NULL && signallingChannel->IsOpen()) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress);
      H323TransportAddress signalAddress = signallingChannel->GetLocalAddress();
      signalAddress.SetPDU(arq.m_srcCallSignalAddress);
    }
    if (response.transportAddress != NULL && !response.transportAddress->IsEmpty()) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_destCallSignalAddress);
      response.transportAddress->SetPDU(arq.m_destCallSignalAddress);
    }
  }

  arq.m_bandWidth = connection.GetBandwidthAvailable();
  arq.m_callReferenceValue = connection.GetCallReference();
  arq.m_conferenceID = connection.GetConferenceIdentifier();
  arq.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  AdmissionRequestResponseInfo info(response, connection);
  info.accessTokenOID1 = connection.GetGkAccessTokenOID();
  PINDEX comma = info.accessTokenOID1.Find(',');
  if (comma == P_MAX_INDEX)
    info.accessTokenOID2 = info.accessTokenOID1;
  else {
    info.accessTokenOID2 = info.accessTokenOID1.Mid(comma+1);
    info.accessTokenOID1.Delete(comma, P_MAX_INDEX);
  }

  connection.OnSendARQ(arq);

  Request request(arq.m_requestSeqNum, pdu);
  request.responseInfo = &info;

  if (!authenticators.IsEmpty()) {
    pdu.Prepare(arq.m_tokens, H225_AdmissionRequest::e_tokens,
                arq.m_cryptoTokens, H225_AdmissionRequest::e_cryptoTokens);

    H235Authenticators adjustedAuthenticators;
    if (connection.GetAdmissionRequestAuthentication(arq, adjustedAuthenticators)) {
      PTRACE(3, "RAS\tAuthenticators credentials replaced with \""
             << setfill(',') << adjustedAuthenticators << setfill(' ') << "\" during ARQ");

      for (H235Authenticators::iterator iterAuth = adjustedAuthenticators.begin(); iterAuth != adjustedAuthenticators.end(); ++iterAuth) {
        if (iterAuth->UseGkAndEpIdentifiers())
          iterAuth->SetRemoteId(gatekeeperIdentifier);
      }

      adjustedAuthenticators.PreparePDU(pdu,
                                        arq.m_tokens, H225_AdmissionRequest::e_tokens,
                                        arq.m_cryptoTokens, H225_AdmissionRequest::e_cryptoTokens);
      pdu.SetAuthenticators(adjustedAuthenticators);
    }
  }

  if (!MakeRequest(request)) {
    response.rejectReason = request.rejectReason;

    // See if we are registered.
    if (request.responseResult == Request::RejectReceived &&
        response.rejectReason != H225_AdmissionRejectReason::e_callerNotRegistered &&
        response.rejectReason != H225_AdmissionRejectReason::e_invalidEndpointIdentifier)
      return PFalse;

    PTRACE(2, "RAS\tEndpoint has become unregistered during ARQ from gatekeeper " << gatekeeperIdentifier);

    // Have been told we are not registered (or gk offline)
    switch (request.responseResult) {
      case Request::NoResponseReceived :
        registrationFailReason = TransportError;
        response.rejectReason = UINT_MAX;
        break;

      case Request::BadCryptoTokens :
        registrationFailReason = SecurityDenied;
        response.rejectReason = H225_AdmissionRejectReason::e_securityDenial;
        break;

      default :
        registrationFailReason = GatekeeperLostRegistration;
    }

    // If we are not registered and auto register is set ...
    if (!autoReregister)
      return PFalse;

    // Then immediately reregister.
    if (!RegistrationRequest(autoReregister))
      return PFalse;

    // Reset the gk info in ARQ
    arq.m_endpointIdentifier = endpointIdentifier;
    if (!gatekeeperIdentifier) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_gatekeeperIdentifier);
      arq.m_gatekeeperIdentifier = gatekeeperIdentifier;
    }
    else
      arq.RemoveOptionalField(H225_AdmissionRequest::e_gatekeeperIdentifier);

    // Is new request so need new sequence number as well.
    arq.m_requestSeqNum = GetNextSequenceNumber();
    request.sequenceNumber = arq.m_requestSeqNum;

    if (!MakeRequest(request)) {
      response.rejectReason = request.responseResult == Request::RejectReceived
                                                ? request.rejectReason : UINT_MAX;
     
      return PFalse;
    }
  }

  connection.SetBandwidthAvailable(info.allocatedBandwidth);
  connection.SetUUIEsRequested(info.uuiesRequested);

  return PTrue;
}


void H323Gatekeeper::OnSendAdmissionRequest(H225_AdmissionRequest & /*arq*/)
{
  // Override default function as it sets crypto tokens and this is really
  // done by the AdmissionRequest() function.
}


static unsigned GetUUIEsRequested(const H225_UUIEsRequested & pdu)
{
  unsigned uuiesRequested = 0;

  if ((PBoolean)pdu.m_setup)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_setup);
  if ((PBoolean)pdu.m_callProceeding)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_callProceeding);
  if ((PBoolean)pdu.m_connect)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_connect);
  if ((PBoolean)pdu.m_alerting)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_alerting);
  if ((PBoolean)pdu.m_information)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_information);
  if ((PBoolean)pdu.m_releaseComplete)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_releaseComplete);
  if ((PBoolean)pdu.m_facility)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_facility);
  if ((PBoolean)pdu.m_progress)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_progress);
  if ((PBoolean)pdu.m_empty)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_empty);

  if (pdu.HasOptionalField(H225_UUIEsRequested::e_status) && (PBoolean)pdu.m_status)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_status);
  if (pdu.HasOptionalField(H225_UUIEsRequested::e_statusInquiry) && (PBoolean)pdu.m_statusInquiry)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_statusInquiry);
  if (pdu.HasOptionalField(H225_UUIEsRequested::e_setupAcknowledge) && (PBoolean)pdu.m_setupAcknowledge)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_setupAcknowledge);
  if (pdu.HasOptionalField(H225_UUIEsRequested::e_notify) && (PBoolean)pdu.m_notify)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_notify);

  return uuiesRequested;
}


static void ExtractToken(const AdmissionRequestResponseInfo & info,
                         const H225_ArrayOf_ClearToken & tokens,
                         PBYTEArray & accessTokenData)
{
  if (!info.accessTokenOID1 && tokens.GetSize() > 0) {
    PTRACE(4, "RAS\tLooking for OID " << info.accessTokenOID1 << " in ACF to copy.");
    for (PINDEX i = 0; i < tokens.GetSize(); i++) {
      if (tokens[i].m_tokenOID == info.accessTokenOID1) {
        PTRACE(4, "RAS\tLooking for OID " << info.accessTokenOID2 << " in token to copy.");
        if (tokens[i].HasOptionalField(H235_ClearToken::e_nonStandard) &&
            tokens[i].m_nonStandard.m_nonStandardIdentifier == info.accessTokenOID2) {
          PTRACE(4, "RAS\tCopying ACF nonStandard OctetString.");
          accessTokenData = tokens[i].m_nonStandard.m_data;
          break;
        }
      }
    }
  }
}


PBoolean H323Gatekeeper::OnReceiveAdmissionConfirm(const H225_AdmissionConfirm & acf)
{
  if (!H225_RAS::OnReceiveAdmissionConfirm(acf))
    return PFalse;

  AdmissionRequestResponseInfo & info = *(AdmissionRequestResponseInfo *)lastRequest->responseInfo;
  info.allocatedBandwidth = acf.m_bandWidth;
  if (info.param.transportAddress != NULL)
    *info.param.transportAddress = acf.m_destCallSignalAddress;

  info.param.gatekeeperRouted = acf.m_callModel.GetTag() == H225_CallModel::e_gatekeeperRouted;

  // Remove the endpoint aliases that the gatekeeper did not like and add the
  // ones that it really wants us to be.
  if (info.param.aliasAddresses != NULL &&
      acf.HasOptionalField(H225_AdmissionConfirm::e_destinationInfo)) {
    PTRACE(3, "RAS\tGatekeeper specified " << acf.m_destinationInfo.GetSize() << " aliases in ACF");
    *info.param.aliasAddresses = acf.m_destinationInfo;
  }

  if (acf.HasOptionalField(H225_AdmissionConfirm::e_uuiesRequested))
    info.uuiesRequested = GetUUIEsRequested(acf.m_uuiesRequested);

  if (info.param.destExtraCallInfo != NULL &&
      acf.HasOptionalField(H225_AdmissionConfirm::e_destExtraCallInfo))
    *info.param.destExtraCallInfo = acf.m_destExtraCallInfo;

  if (info.param.accessTokenData != NULL && acf.HasOptionalField(H225_AdmissionConfirm::e_tokens))
    ExtractToken(info, acf.m_tokens, *info.param.accessTokenData);

  if (info.param.transportAddress != NULL) {
    PINDEX count = 1;
    for (PINDEX i = 0; i < acf.m_alternateEndpoints.GetSize() && count < info.param.endpointCount; i++) {
      if (acf.m_alternateEndpoints[i].HasOptionalField(H225_Endpoint::e_callSignalAddress) &&
          acf.m_alternateEndpoints[i].m_callSignalAddress.GetSize() > 0) {
        info.param.transportAddress[count] = acf.m_alternateEndpoints[i].m_callSignalAddress[0];
        if (info.param.accessTokenData != NULL)
          ExtractToken(info, acf.m_alternateEndpoints[i].m_tokens, info.param.accessTokenData[count]);
        count++;
      }
    }
    info.param.endpointCount = count;
  }

  if (acf.HasOptionalField(H225_AdmissionConfirm::e_irrFrequency))
    SetInfoRequestRate(AdjustTimeout(acf.m_irrFrequency));
  willRespondToIRR = acf.m_willRespondToIRR;

  if (acf.HasOptionalField(H225_AdmissionConfirm::e_serviceControl))
    OnServiceControlSessions(acf.m_serviceControl, &info.connection);

  return PTrue;
}


PBoolean H323Gatekeeper::OnReceiveAdmissionReject(const H225_AdmissionReject & arj)
{
  if (!H225_RAS::OnReceiveAdmissionReject(arj))
    return PFalse;

  if (arj.HasOptionalField(H225_AdmissionConfirm::e_serviceControl))
    OnServiceControlSessions(arj.m_serviceControl,
              &((AdmissionRequestResponseInfo *)lastRequest->responseInfo)->connection);

  return PTrue;
}


static void SetRasUsageInformation(const H323Connection & connection, 
                                   H225_RasUsageInformation & usage)
{
  time_t time = connection.GetAlertingTime().GetTimeInSeconds();
  if (time != 0) {
    usage.IncludeOptionalField(H225_RasUsageInformation::e_alertingTime);
    usage.m_alertingTime = (unsigned)time;
  }

  time = connection.GetConnectionStartTime().GetTimeInSeconds();
  if (time != 0) {
    usage.IncludeOptionalField(H225_RasUsageInformation::e_connectTime);
    usage.m_connectTime = (unsigned)time;
  }

  time = connection.GetConnectionEndTime().GetTimeInSeconds();
  if (time != 0) {
    usage.IncludeOptionalField(H225_RasUsageInformation::e_endTime);
    usage.m_endTime = (unsigned)time;
  }
}


PBoolean H323Gatekeeper::DisengageRequest(const H323Connection & connection, unsigned reason)
{
  H323RasPDU pdu;
  H225_DisengageRequest & drq = pdu.BuildDisengageRequest(GetNextSequenceNumber());

  drq.m_endpointIdentifier = endpointIdentifier;
  drq.m_conferenceID = connection.GetConferenceIdentifier();
  drq.m_callReferenceValue = connection.GetCallReference();
  drq.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  drq.m_disengageReason.SetTag(reason);
  drq.m_answeredCall = connection.HadAnsweredCall();

  drq.IncludeOptionalField(H225_DisengageRequest::e_usageInformation);
  SetRasUsageInformation(connection, drq.m_usageInformation);

  drq.IncludeOptionalField(H225_DisengageRequest::e_terminationCause);
  drq.m_terminationCause.SetTag(H225_CallTerminationCause::e_releaseCompleteReason);
  Q931::CauseValues cause = H323TranslateFromCallEndReason(connection.GetCallEndReason(), drq.m_terminationCause);
  if (cause != Q931::ErrorInCauseIE) {
    drq.m_terminationCause.SetTag(H225_CallTerminationCause::e_releaseCompleteCauseIE);
    PASN_OctetString & rcReason = drq.m_terminationCause;
    rcReason.SetSize(2);
    rcReason[0] = 0x80;
    rcReason[1] = (BYTE)(0x80|cause);
  }

  if (!gatekeeperIdentifier) {
    drq.IncludeOptionalField(H225_DisengageRequest::e_gatekeeperIdentifier);
    drq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  Request request(drq.m_requestSeqNum, pdu);
  return MakeRequestWithReregister(request, H225_DisengageRejectReason::e_notRegistered);
}


PBoolean H323Gatekeeper::OnReceiveDisengageRequest(const H225_DisengageRequest & drq)
{
  if (!H225_RAS::OnReceiveDisengageRequest(drq))
    return PFalse;

  OpalGloballyUniqueID id = NULL;
  if (drq.HasOptionalField(H225_DisengageRequest::e_callIdentifier))
    id = drq.m_callIdentifier.m_guid;
  if (id == NULL)
    id = drq.m_conferenceID;

  H323RasPDU response(authenticators);
  PSafePtr<H323Connection> connection = endpoint.FindConnectionWithLock(id.AsString());
  if (connection == NULL)
    response.BuildDisengageReject(drq.m_requestSeqNum,
                                  H225_DisengageRejectReason::e_requestToDropOther);
  else {
    H225_DisengageConfirm & dcf = response.BuildDisengageConfirm(drq.m_requestSeqNum);

    dcf.IncludeOptionalField(H225_DisengageConfirm::e_usageInformation);
    SetRasUsageInformation(*connection, dcf.m_usageInformation);

    connection->Release(H323Connection::EndedByGatekeeper);
  }

  if (drq.HasOptionalField(H225_DisengageRequest::e_serviceControl))
    OnServiceControlSessions(drq.m_serviceControl, connection);

  return WritePDU(response);
}


PBoolean H323Gatekeeper::BandwidthRequest(H323Connection & connection,
                                      unsigned requestedBandwidth)
{
  H323RasPDU pdu;
  H225_BandwidthRequest & brq = pdu.BuildBandwidthRequest(GetNextSequenceNumber());

  brq.m_endpointIdentifier = endpointIdentifier;
  brq.m_conferenceID = connection.GetConferenceIdentifier();
  brq.m_callReferenceValue = connection.GetCallReference();
  brq.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  brq.m_bandWidth = requestedBandwidth;
  brq.IncludeOptionalField(H225_BandwidthRequest::e_usageInformation);
  SetRasUsageInformation(connection, brq.m_usageInformation);

  Request request(brq.m_requestSeqNum, pdu);
  
  unsigned allocatedBandwidth;
  request.responseInfo = &allocatedBandwidth;

  if (!MakeRequestWithReregister(request, H225_BandRejectReason::e_notBound))
    return PFalse;

  connection.SetBandwidthAvailable(allocatedBandwidth);
  return PTrue;
}


PBoolean H323Gatekeeper::OnReceiveBandwidthConfirm(const H225_BandwidthConfirm & bcf)
{
  if (!H225_RAS::OnReceiveBandwidthConfirm(bcf))
    return PFalse;

  if (lastRequest->responseInfo != NULL)
    *(unsigned *)lastRequest->responseInfo = bcf.m_bandWidth;

  return PTrue;
}


PBoolean H323Gatekeeper::OnReceiveBandwidthRequest(const H225_BandwidthRequest & brq)
{
  if (!H225_RAS::OnReceiveBandwidthRequest(brq))
    return PFalse;

  OpalGloballyUniqueID id = brq.m_callIdentifier.m_guid;
  PSafePtr<H323Connection> connection = endpoint.FindConnectionWithLock(id.AsString());

  H323RasPDU response(authenticators);
  if (connection == NULL)
    response.BuildBandwidthReject(brq.m_requestSeqNum,
                                  H225_BandRejectReason::e_invalidConferenceID);
  else {
    if (connection->SetBandwidthAvailable(brq.m_bandWidth))
      response.BuildBandwidthConfirm(brq.m_requestSeqNum, brq.m_bandWidth);
    else
      response.BuildBandwidthReject(brq.m_requestSeqNum,
                                    H225_BandRejectReason::e_insufficientResources);
  }

  return WritePDU(response);
}


void H323Gatekeeper::SetInfoRequestRate(const PTimeInterval & rate)
{
  if (rate < infoRequestRate.GetResetTime() || infoRequestRate.GetResetTime() == 0) {
    // Have to be sneaky here becuase we do not want to actually change the
    // amount of time to run on the timer.
    PTimeInterval timeToGo = infoRequestRate;
    infoRequestRate = rate;
    if (rate > timeToGo)
      infoRequestRate.PTimeInterval::operator=(timeToGo);
  }
}


void H323Gatekeeper::ClearInfoRequestRate()
{
  // Only reset rate to zero (disabled) if no calls present
  if (endpoint.GetConnectionCount())
    infoRequestRate = 0;
}


H225_InfoRequestResponse & H323Gatekeeper::BuildInfoRequestResponse(H323RasPDU & response,
                                                                    unsigned seqNum)
{
  H225_InfoRequestResponse & irr = response.BuildInfoRequestResponse(seqNum);

  endpoint.SetEndpointTypeInfo(irr.m_endpointType);
  irr.m_endpointIdentifier = endpointIdentifier;

  H323TransportAddress address = transport->GetLocalAddress();
  
  // We do have to use the translated address if specified
  PIPSocket::Address localAddress, remoteAddress;
  WORD localPort;
  
  if (address.GetIpAndPort(localAddress, localPort) &&
      transport->GetRemoteAddress().GetIpAddress(remoteAddress)) {

    OpalManager & manager = transport->GetEndPoint().GetManager();
    if (manager.TranslateIPAddress(localAddress, remoteAddress))
      address = H323TransportAddress(localAddress, localPort);
  }
  address.SetPDU(irr.m_rasAddress);

  SetListenerAddresses(irr.m_callSignalAddress);

  irr.IncludeOptionalField(H225_InfoRequestResponse::e_endpointAlias);
  H323SetAliasAddresses(endpoint.GetAliasNames(), irr.m_endpointAlias);

  return irr;
}


PBoolean H323Gatekeeper::SendUnsolicitedIRR(H225_InfoRequestResponse & irr,
                                        H323RasPDU & response)
{
  irr.m_unsolicited = PTrue;

  if (willRespondToIRR) {
    PTRACE(4, "RAS\tSending unsolicited IRR and awaiting acknowledgement");
    Request request(irr.m_requestSeqNum, response);
    return MakeRequest(request);
  }

  PTRACE(4, "RAS\tSending unsolicited IRR and without acknowledgement");
  response.SetAuthenticators(authenticators);
  return WritePDU(response);
}


static void AddInfoRequestResponseCall(H225_InfoRequestResponse & irr,
                                       const H323Connection & connection)
{
  irr.IncludeOptionalField(H225_InfoRequestResponse::e_perCallInfo);

  PINDEX sz = irr.m_perCallInfo.GetSize();
  if (!irr.m_perCallInfo.SetSize(sz+1))
    return;

  H225_InfoRequestResponse_perCallInfo_subtype & info = irr.m_perCallInfo[sz];

  info.m_callReferenceValue = connection.GetCallReference();
  info.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  info.m_conferenceID = connection.GetConferenceIdentifier();
  info.IncludeOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_originator);
  info.m_originator = !connection.HadAnsweredCall();

  H323_RTP_Session * session = connection.GetSessionCallbacks(H323Capability::DefaultAudioSessionID);
  if (session != NULL) {
    info.IncludeOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_audio);
    info.m_audio.SetSize(1);
    session->OnSendRasInfo(info.m_audio[0]);
  }

  session = connection.GetSessionCallbacks(H323Capability::DefaultVideoSessionID);
  if (session != NULL) {
    info.IncludeOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_video);
    info.m_video.SetSize(1);
    session->OnSendRasInfo(info.m_video[0]);
  }

  H323TransportAddress address = connection.GetControlChannel().GetLocalAddress();
  address.SetPDU(info.m_h245.m_sendAddress);
  address = connection.GetControlChannel().GetRemoteAddress();
  address.SetPDU(info.m_h245.m_recvAddress);

  info.m_callType.SetTag(H225_CallType::e_pointToPoint);
  info.m_bandWidth = connection.GetBandwidthUsed();
  info.m_callModel.SetTag(connection.IsGatekeeperRouted() ? H225_CallModel::e_gatekeeperRouted
                                                          : H225_CallModel::e_direct);

  info.IncludeOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_usageInformation);
  SetRasUsageInformation(connection, info.m_usageInformation);
}


static PBoolean AddAllInfoRequestResponseCall(H225_InfoRequestResponse & irr,
                                          H323EndPoint & endpoint,
                                          const PStringList & tokens)
{
  PBoolean addedOne = PFalse;

  for (PStringList::const_iterator token = tokens.begin(); token != tokens.end(); ++token) {
    PSafePtr<H323Connection> connection = endpoint.FindConnectionWithLock(*token);
    if (connection != NULL) {
      AddInfoRequestResponseCall(irr, *connection);
      addedOne = PTrue;
    }
  }

  return addedOne;
}


void H323Gatekeeper::InfoRequestResponse()
{
  PStringList tokens = endpoint.GetAllConnections();
  if (tokens.IsEmpty())
    return;

  H323RasPDU response;
  H225_InfoRequestResponse & irr = BuildInfoRequestResponse(response, GetNextSequenceNumber());

  if (AddAllInfoRequestResponseCall(irr, endpoint, tokens))
    SendUnsolicitedIRR(irr, response);
}


void H323Gatekeeper::InfoRequestResponse(const H323Connection & connection)
{
  H323RasPDU response;
  H225_InfoRequestResponse & irr = BuildInfoRequestResponse(response, GetNextSequenceNumber());

  AddInfoRequestResponseCall(irr, connection);

  SendUnsolicitedIRR(irr, response);
}


void H323Gatekeeper::InfoRequestResponse(const H323Connection & connection,
                                         const H225_H323_UU_PDU & pdu,
                                         PBoolean sent)
{
  // Are unknown Q.931 PDU
  if (pdu.m_h323_message_body.GetTag() == P_MAX_INDEX)
    return;

  // Check mask of things to report on
  if ((connection.GetUUIEsRequested() & (1<<pdu.m_h323_message_body.GetTag())) == 0)
    return;

  PTRACE(3, "RAS\tSending unsolicited IRR for requested UUIE");

  // Report the PDU
  H323RasPDU response;
  H225_InfoRequestResponse & irr = BuildInfoRequestResponse(response, GetNextSequenceNumber());

  AddInfoRequestResponseCall(irr, connection);

  irr.m_perCallInfo[0].IncludeOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_pdu);
  irr.m_perCallInfo[0].m_pdu.SetSize(1);
  irr.m_perCallInfo[0].m_pdu[0].m_sent = sent;
  irr.m_perCallInfo[0].m_pdu[0].m_h323pdu = pdu;

  SendUnsolicitedIRR(irr, response);
}


PBoolean H323Gatekeeper::OnReceiveInfoRequest(const H225_InfoRequest & irq)
{
  if (!H225_RAS::OnReceiveInfoRequest(irq))
    return PFalse;

  H323RasPDU response(authenticators);
  H225_InfoRequestResponse & irr = BuildInfoRequestResponse(response, irq.m_requestSeqNum);

  if (irq.m_callReferenceValue == 0) {
    if (!AddAllInfoRequestResponseCall(irr, endpoint, endpoint.GetAllConnections())) {
      irr.IncludeOptionalField(H225_InfoRequestResponse::e_irrStatus);
      irr.m_irrStatus.SetTag(H225_InfoRequestResponseStatus::e_invalidCall);
    }
  }
  else {
    OpalGloballyUniqueID id = irq.m_callIdentifier.m_guid;
    PSafePtr<H323Connection> connection = endpoint.FindConnectionWithLock(id.AsString());
    if (connection == NULL) {
      irr.IncludeOptionalField(H225_InfoRequestResponse::e_irrStatus);
      irr.m_irrStatus.SetTag(H225_InfoRequestResponseStatus::e_invalidCall);
    }
    else {
      if (irq.HasOptionalField(H225_InfoRequest::e_uuiesRequested))
        connection->SetUUIEsRequested(::GetUUIEsRequested(irq.m_uuiesRequested));

      AddInfoRequestResponseCall(irr, *connection);
    }
  }

  if (!irq.HasOptionalField(H225_InfoRequest::e_replyAddress))
    return WritePDU(response);

  H323TransportAddress replyAddress = irq.m_replyAddress;
  if (replyAddress.IsEmpty())
    return PFalse;

  H323TransportAddress oldAddress = transport->GetRemoteAddress();
  if (oldAddress.IsEquivalent(replyAddress))
    return WritePDU(response);

  transport->GetWriteMutex().Wait();

  bool ok = transport->ConnectTo(replyAddress) && WritePDU(response);
  transport->ConnectTo(oldAddress);

  transport->GetWriteMutex().Signal();

  return ok;
}


PBoolean H323Gatekeeper::OnReceiveServiceControlIndication(const H225_ServiceControlIndication & sci)
{
  if (!H225_RAS::OnReceiveServiceControlIndication(sci))
    return PFalse;

  H323Connection * connection = NULL;

  if (sci.HasOptionalField(H225_ServiceControlIndication::e_callSpecific)) {
    OpalGloballyUniqueID id = sci.m_callSpecific.m_callIdentifier.m_guid;
    if (id.IsNULL())
      id = sci.m_callSpecific.m_conferenceID;
    connection = endpoint.FindConnectionWithLock(id.AsString());
  }

  OnServiceControlSessions(sci.m_serviceControl, connection);

  H323RasPDU response(authenticators);
  response.BuildServiceControlResponse(sci.m_requestSeqNum);
  return WritePDU(response);
}


void H323Gatekeeper::OnServiceControlSessions(const H225_ArrayOf_ServiceControlSession & serviceControl,
                                              H323Connection * connection)
{
  for (PINDEX i = 0; i < serviceControl.GetSize(); i++) {
    H225_ServiceControlSession & pdu = serviceControl[i];

    H323ServiceControlSession * session = NULL;
    unsigned sessionId = pdu.m_sessionId;

    if (serviceControlSessions.Contains(sessionId)) {
      session = &serviceControlSessions[sessionId];
      if (pdu.HasOptionalField(H225_ServiceControlSession::e_contents)) {
        if (!session->OnReceivedPDU(pdu.m_contents)) {
          PTRACE(2, "SvcCtrl\tService control for session has changed!");
          session = NULL;
        }
      }
    }

    if (session == NULL && pdu.HasOptionalField(H225_ServiceControlSession::e_contents)) {
      session = endpoint.CreateServiceControlSession(pdu.m_contents);
      serviceControlSessions.SetAt(sessionId, session);
    }

    if (session != NULL)
      endpoint.OnServiceControlSession(sessionId, pdu.m_reason.GetTag(), *session, connection);
  }
}


void H323Gatekeeper::OnTerminalAliasChanged()
{
  // Do a non-lightweight RRQ. Treat the GK as unregistered and immediately send a RRQ
  registrationFailReason = UnregisteredLocally;
  reregisterNow = TRUE;
  monitorTickle.Signal();
}


void H323Gatekeeper::SetPassword(const PString & password, 
                                 const PString & username)
{
  PString localId = username;
  if (localId.IsEmpty())
    localId = endpoint.GetLocalUserName();

  for (H235Authenticators::iterator iterAuth = authenticators.begin(); iterAuth != authenticators.end(); ++iterAuth) {
    iterAuth->SetLocalId(localId);
    iterAuth->SetPassword(password);
  }
}


void H323Gatekeeper::MonitorMain(PThread &, INT)
{
  PTRACE(4, "RAS\tBackground thread started");

  for (;;) {
    monitorTickle.Wait();
    if (monitorStop)
      break;

    if (reregisterNow || (!timeToLive.IsRunning() && timeToLive.GetResetTime() > 0)) {
      RegistrationTimeToLive();
      timeToLive.Reset();
    }

    if (!infoRequestRate.IsRunning() && infoRequestRate.GetResetTime() > 0) {
      InfoRequestResponse();
      infoRequestRate.Reset();
    }
  }

  PTRACE(4, "RAS\tBackground thread ended");
}


void H323Gatekeeper::TickleMonitor(PTimer &, INT)
{
  monitorTickle.Signal();
}


void H323Gatekeeper::SetAlternates(const H225_ArrayOf_AlternateGK & alts, PBoolean permanent)
{
  PINDEX i;

  if (!alternatePermanent)  {
    // don't want to replace alternates gatekeepers if this is an alternate and it's not permanent
    for (i = 0; i < alternates.GetSize(); i++) {
      if (transport->GetRemoteAddress().IsEquivalent(alternates[i].rasAddress) &&
          gatekeeperIdentifier == alternates[i].gatekeeperIdentifier)
        return;
    }
  }

  alternates.RemoveAll();
  for (i = 0; i < alts.GetSize(); i++) {
    AlternateInfo * alt = new AlternateInfo(alts[i]);
    if (alt->rasAddress.IsEmpty())
      delete alt;
    else
      alternates.Append(alt);
  }
  
  alternatePermanent = permanent;

  PTRACE(3, "RAS\tSet alternate gatekeepers:\n"
         << setfill('\n') << alternates << setfill(' '));
}


PBoolean H323Gatekeeper::MakeRequestWithReregister(Request & request, unsigned unregisteredTag)
{
  if (MakeRequest(request))
    return PTrue;

  if (request.responseResult == Request::RejectReceived &&
      request.rejectReason != unregisteredTag)
    return PFalse;

  PTRACE(2, "RAS\tEndpoint has become unregistered from gatekeeper " << gatekeeperIdentifier);

  // Have been told we are not registered (or gk offline)
  switch (request.responseResult) {
    case Request::NoResponseReceived :
      registrationFailReason = TransportError;
      break;

    case Request::BadCryptoTokens :
      registrationFailReason = SecurityDenied;
      break;

    default :
      registrationFailReason = GatekeeperLostRegistration;
  }

  // If we are not registered and auto register is set ...
  if (!autoReregister)
    return PFalse;

  reregisterNow = PTrue;
  monitorTickle.Signal();
  return PFalse;
}


void H323Gatekeeper::Connect(const H323TransportAddress & address,
                             const PString & gkid)
{
  if (transport == NULL)
    transport = CreateTransport(PIPSocket::GetDefaultIpAny());

  transport->SetRemoteAddress(address);
  transport->Connect();

  gatekeeperIdentifier = gkid;
}


PBoolean H323Gatekeeper::MakeRequest(Request & request)
{
  if (PAssertNULL(transport) == NULL)
    return PFalse;

  // Set authenticators if not already set by caller
  requestMutex.Wait();

  if (request.requestPDU.GetAuthenticators().IsEmpty())
    request.requestPDU.SetAuthenticators(authenticators);

  /* To be sure that the H323 Cleaner, H225 Caller or Monitor don't set the
     transport address of the alternate while the other is in timeout. We
     have to block the function */

  H323TransportAddress tempAddr = transport->GetRemoteAddress();
  PString tempIdentifier = gatekeeperIdentifier;
  
  PINDEX alt = 0;
  for (;;) {
    if (H225_RAS::MakeRequest(request)) {
      if (!alternatePermanent &&
            (transport->GetRemoteAddress() != tempAddr ||
             gatekeeperIdentifier != tempIdentifier))
        Connect(tempAddr, tempIdentifier);
      requestMutex.Signal();
      return PTrue;
    }
    
    if (request.responseResult != Request::NoResponseReceived &&
        request.responseResult != Request::TryAlternate) {
      // try alternate in those cases and see if it's successful
      requestMutex.Signal();
      return PFalse;
    }
    
    AlternateInfo * altInfo;
    PIPSocket::Address localAddress;
    WORD localPort;
    do {
      if (alt >= alternates.GetSize()) {
        if (!alternatePermanent && alt > 0) 
          Connect(tempAddr,tempIdentifier);
        requestMutex.Signal();
        return PFalse;
      }
      
      altInfo = &alternates[alt++];
      transport->GetLocalAddress().GetIpAndPort(localAddress,localPort);
      transport->CleanUpOnTermination();
      delete transport;

      transport = CreateTransport(localAddress,localPort);
      transport->SetRemoteAddress (altInfo->rasAddress);
      transport->Connect();
      gatekeeperIdentifier = altInfo->gatekeeperIdentifier;
      StartChannel();
    } while (altInfo->registrationState == AlternateInfo::RegistrationFailed);
    
    if (altInfo->registrationState == AlternateInfo::NeedToRegister) {
      altInfo->registrationState = AlternateInfo::RegistrationFailed;
      registrationFailReason = TransportError;
      discoveryComplete = PFalse;
      H323RasPDU pdu;
      Request req(SetupGatekeeperRequest(pdu), pdu);
      
      if (H225_RAS::MakeRequest(req)) {
        requestMutex.Signal(); // avoid deadlock...
        if (RegistrationRequest(autoReregister)) {
          altInfo->registrationState = AlternateInfo::IsRegistered;
          // The wanted registration is done, we can return
          if (request.requestPDU.GetChoice().GetTag() == H225_RasMessage::e_registrationRequest) {
        if (!alternatePermanent)
          Connect(tempAddr,tempIdentifier);
        return PTrue;
          }
        }
        requestMutex.Wait();
      }
    }
  }
}


H323Transport * H323Gatekeeper::CreateTransport(PIPSocket::Address binding, WORD port, PBoolean reuseAddr)
{
  return new H323TransportUDP(endpoint, binding, port, reuseAddr);
}


void H323Gatekeeper::OnAddInterface(const PIPSocket::InterfaceEntry & /*entry*/, PINDEX priority)
{
  if (priority != HighPriority)
    UpdateConnectionStatus();
  else {
    // special case if interface filtering is used: A new interface may 'hide' the old interface.
    // If this is the case, remove the transport interface and do a full rediscovery. 
    //
    // There is a race condition: If the transport interface binding is cleared AFTER
    // PMonitoredSockets::ReadFromSocket() is interrupted and starts listening again,
    // the transport will still listen on the old interface only. Therefore, clear the
    // socket binding BEFORE the monitored sockets update their interfaces.
    if (PInterfaceMonitor::GetInstance().HasInterfaceFilter()) {
      PString iface = transport->GetInterface();
      if (iface.IsEmpty()) // not connected.
        return;
      
      PIPSocket::Address addr;
      if (!transport->GetRemoteAddress().GetIpAddress(addr))
        return;
      
      PStringArray ifaces = lowPriorityMonitor.GetInterfaces(PFalse, addr);
      
      if (ifaces.GetStringsIndex(iface) == P_MAX_INDEX) { // original interface no longer available
        transport->SetInterface(PString::Empty());
      }
    }
  }
}


void H323Gatekeeper::OnRemoveInterface(const PIPSocket::InterfaceEntry & entry, PINDEX priority)
{
  if (priority == LowPriority) {
    UpdateConnectionStatus();
    return;
  }
  
  if (transport == NULL)
    return;
  
  PString iface = transport->GetInterface();
  if (iface.IsEmpty()) // not connected.
    return;
  
  if (PInterfaceMonitor::IsMatchingInterface(iface, entry)) {
    // currently used interface went down. make transport listen
    // on all available interfaces.
    transport->SetInterface(PString::Empty());
    PTRACE(3, "RAS\tInterface gatekeeper is bound to has gone down, restarting discovery");
  }
}


void H323Gatekeeper::UpdateConnectionStatus()
{
  // sanity check
  if (transport == NULL)
    return;
  
  PString iface = transport->GetInterface();
  if (!iface.IsEmpty()) // still connected
    return;
  
  // not connected anymore. so see if there is an interface available
  // for connecting to the remote address.
  PIPSocket::Address addr;
  if (!transport->GetRemoteAddress().GetIpAddress(addr))
    return;
  
  if (lowPriorityMonitor.GetInterfaces(false, addr).GetSize() > 0) {
    // at least one interface available, locate gatekeper
    discoveryComplete = PFalse;
    reregisterNow = PTrue;
    monitorTickle.Signal();
  }
}


PBoolean H323Gatekeeper::OnSendFeatureSet(unsigned pduType, H225_FeatureSet & message) const
{
#if OPAL_H460
  return features->SendFeature(pduType, message);
#else
  return endpoint.OnSendFeatureSet(pduType, message);
#endif
}


void H323Gatekeeper::OnReceiveFeatureSet(unsigned pduType, const H225_FeatureSet & message) const
{
#if OPAL_H460
  features->ReceiveFeature(pduType, message);
#else
  endpoint.OnReceiveFeatureSet(pduType, message);
#endif
}


/////////////////////////////////////////////////////////////////////////////

H323Gatekeeper::AlternateInfo::AlternateInfo(H225_AlternateGK & alt)
  : rasAddress(alt.m_rasAddress),
    gatekeeperIdentifier(alt.m_gatekeeperIdentifier.GetValue()),
    priority(alt.m_priority)
{
  registrationState = alt.m_needToRegister ? NeedToRegister : NoRegistrationNeeded;
}


H323Gatekeeper::AlternateInfo::~AlternateInfo ()
{

}


PObject::Comparison H323Gatekeeper::AlternateInfo::Compare(const PObject & obj)
{
  PAssert(PIsDescendant(&obj, H323Gatekeeper), PInvalidCast);
  unsigned otherPriority = ((const AlternateInfo & )obj).priority;
  if (priority < otherPriority)
    return LessThan;
  if (priority > otherPriority)
    return GreaterThan;
  return EqualTo;
}


void H323Gatekeeper::AlternateInfo::PrintOn(ostream & strm) const
{
  if (!gatekeeperIdentifier)
    strm << gatekeeperIdentifier << '@';

  strm << rasAddress;

  if (priority > 0)
    strm << ";priority=" << priority;
}

/////////////////////////////////////////////////////////////////////////////

H323Gatekeeper::InterfaceMonitor::InterfaceMonitor(H323Gatekeeper & _gk, PINDEX priority)
: PInterfaceMonitorClient(priority), gk(_gk)
{
}

void H323Gatekeeper::InterfaceMonitor::OnAddInterface(const PIPSocket::InterfaceEntry & entry)
{
  gk.OnAddInterface(entry, priority);
}

void H323Gatekeeper::InterfaceMonitor::OnRemoveInterface(const PIPSocket::InterfaceEntry & entry)
{
  gk.OnRemoveInterface(entry, priority);
}


#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////

