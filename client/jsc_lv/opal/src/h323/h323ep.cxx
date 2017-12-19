/*
 * h323ep.cxx
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): Many thanks to Simon Horne.
 *
 * $Revision: 23224 $
 * $Author: rjongbloed $
 * $Date: 2009-08-20 04:26:44 +0000 (Thu, 20 Aug 2009) $
 */

#include <ptlib.h>

#include <opal/buildopts.h>
#if OPAL_H323

#ifdef __GNUC__
#pragma implementation "h323ep.h"
#endif

#include <h323/h323ep.h>

#include <ptclib/random.h>
#include <opal/call.h>
#include <h323/h323pdu.h>
#include <h323/gkclient.h>
#include <ptclib/url.h>
#include <ptclib/enum.h>
#include <ptclib/pils.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

H323EndPoint::H323EndPoint(OpalManager & manager)
  : OpalRTPEndPoint(manager, "h323", CanTerminateCall)
  , autoCallForward(true)
  , disableFastStart(false)
  , disableH245Tunneling(false)
  , disableH245inSetup(false)
  , m_bH245Disabled(false)
  , canDisplayAmountString(false)
  , canEnforceDurationLimit(true)
#if OPAL_H450
  , callIntrusionProtectionLevel(3) //H45011_CIProtectionLevel::e_fullProtection;
#endif
  , terminalType(e_TerminalOnly)
#if OPAL_H239
  , m_defaultH239Control(false)
#endif
  , clearCallOnRoundTripFail(false)
  , signallingChannelCallTimeout(0, 0, 1)  // Minutes
  , controlChannelStartTimeout(0, 0, 2)    // Minutes
  , endSessionTimeout(0, 10)               // Seconds
  , masterSlaveDeterminationTimeout(0, 30) // Seconds
  , masterSlaveDeterminationRetries(10)
  , capabilityExchangeTimeout(0, 30)       // Seconds
  , logicalChannelTimeout(0, 30)           // Seconds
  , requestModeTimeout(0, 30)              // Seconds
  , roundTripDelayTimeout(0, 10)           // Seconds
  , roundTripDelayRate(0, 0, 1)            // Minutes
  , gatekeeperRequestTimeout(0, 5)         // Seconds
  , gatekeeperRequestRetries(2)
  , rasRequestTimeout(0, 3)                // Seconds
  , rasRequestRetries(2)
  , registrationTimeToLive(0, 0, 10)       // Minutes
  , sendGRQ(true)
  , callTransferT1(0,10)                   // Seconds
  , callTransferT2(0,10)                   // Seconds
  , callTransferT3(0,10)                   // Seconds
  , callTransferT4(0,10)                   // Seconds
  , callIntrusionT1(0,30)                  // Seconds
  , callIntrusionT2(0,30)                  // Seconds
  , callIntrusionT3(0,30)                  // Seconds
  , callIntrusionT4(0,30)                  // Seconds
  , callIntrusionT5(0,10)                  // Seconds
  , callIntrusionT6(0,10)                   // Seconds
  , gatekeeper(NULL)
#if OPAL_H450
  , nextH450CallIdentity(0)
#endif
#if OPAL_H460
  , disableH460(false)
#endif
{
  defaultSignalPort = 1720; // Set port in OpalEndPoint class

  localAliasNames.AppendString(defaultLocalPartyName);

  secondaryConnectionsActive.DisallowDeleteObjects();

  manager.AttachEndPoint(this, "h323s");

  PTRACE(4, "H323\tCreated endpoint.");
}


H323EndPoint::~H323EndPoint()
{
}


void H323EndPoint::ShutDown()
{
  /* Unregister request needs/depends OpalEndpoint listeners object, so shut
     down the gatekeeper (if there was one) before cleaning up the OpalEndpoint
     object which kills the listeners. */
  RemoveGatekeeper();

  OpalEndPoint::ShutDown();
}


PString H323EndPoint::GetDefaultTransport() const
{
  return "tcp$"
#if OPAL_PTLIB_SSL
         ",tcps$:1300"
#endif
    ;
}

void H323EndPoint::SetEndpointTypeInfo(H225_EndpointType & info) const
{
  info.IncludeOptionalField(H225_EndpointType::e_vendor);
  SetVendorIdentifierInfo(info.m_vendor);

  switch (terminalType) {
    case e_TerminalOnly :
    case e_TerminalAndMC :
      info.IncludeOptionalField(H225_EndpointType::e_terminal);
      break;

    case e_GatewayOnly :
    case e_GatewayAndMC :
    case e_GatewayAndMCWithDataMP :
    case e_GatewayAndMCWithAudioMP :
    case e_GatewayAndMCWithAVMP :
      info.IncludeOptionalField(H225_EndpointType::e_gateway);
      if (SetGatewaySupportedProtocol(info.m_gateway.m_protocol))
        info.m_gateway.IncludeOptionalField(H225_GatewayInfo::e_protocol);
      break;

    case e_GatekeeperOnly :
    case e_GatekeeperWithDataMP :
    case e_GatekeeperWithAudioMP :
    case e_GatekeeperWithAVMP :
      info.IncludeOptionalField(H225_EndpointType::e_gatekeeper);
      break;

    case e_MCUOnly :
    case e_MCUWithDataMP :
    case e_MCUWithAudioMP :
    case e_MCUWithAVMP :
      info.IncludeOptionalField(H225_EndpointType::e_mcu);
      info.m_mc = true;
      if (SetGatewaySupportedProtocol(info.m_mcu.m_protocol))
        info.m_mcu.IncludeOptionalField(H225_McuInfo::e_protocol);
  }
}


void H323EndPoint::SetVendorIdentifierInfo(H225_VendorIdentifier & info) const
{
  SetH221NonStandardInfo(info.m_vendor);

  info.IncludeOptionalField(H225_VendorIdentifier::e_productId);
  info.m_productId = productInfo.vendor & productInfo.name;
  info.m_productId.SetSize(info.m_productId.GetSize()+2);

  info.IncludeOptionalField(H225_VendorIdentifier::e_versionId);
  info.m_versionId = productInfo.version + " (OPAL v" + OpalGetVersion() + ')';
  info.m_versionId.SetSize(info.m_versionId.GetSize()+2);
}


void H323EndPoint::SetH221NonStandardInfo(H225_H221NonStandard & info) const
{
  info.m_t35CountryCode = productInfo.t35CountryCode;
  info.m_t35Extension = productInfo.t35Extension;
  info.m_manufacturerCode = productInfo.manufacturerCode;
}


bool H323EndPoint::SetGatewaySupportedProtocol(H225_ArrayOf_SupportedProtocols & protocols) const
{
  PStringList prefixes;

  if (!OnSetGatewayPrefixes(prefixes))
    return false;

  PINDEX count = protocols.GetSize();
  protocols.SetSize(count+1);

  protocols[count].SetTag(H225_SupportedProtocols::e_h323);
  H225_H323Caps & caps = protocols[count];

  caps.IncludeOptionalField(H225_H323Caps::e_supportedPrefixes);
  H225_ArrayOf_SupportedPrefix & supportedPrefixes = caps.m_supportedPrefixes;	
  supportedPrefixes.SetSize(prefixes.GetSize());

  for (PINDEX i = 0; i < prefixes.GetSize(); i++)
    H323SetAliasAddress(prefixes[i], supportedPrefixes[i].m_prefix);

  return true;
}


bool H323EndPoint::OnSetGatewayPrefixes(PStringList & prefixes) const
{
	return false;
}


H323Capability * H323EndPoint::FindCapability(const H245_Capability & cap) const
{
  return GetCapabilities().FindCapability(cap);
}


H323Capability * H323EndPoint::FindCapability(const H245_DataType & dataType) const
{
  return GetCapabilities().FindCapability(dataType);
}


H323Capability * H323EndPoint::FindCapability(H323Capability::MainTypes mainType,
                                              unsigned subType) const
{
  return GetCapabilities().FindCapability(mainType, subType);
}


void H323EndPoint::AddCapability(H323Capability * capability)
{
  capabilities.Add(capability);
}


PINDEX H323EndPoint::SetCapability(PINDEX descriptorNum,
                                   PINDEX simultaneousNum,
                                   H323Capability * capability)
{
  return capabilities.SetCapability(descriptorNum, simultaneousNum, capability);
}


PINDEX H323EndPoint::AddAllCapabilities(PINDEX descriptorNum,
                                        PINDEX simultaneous,
                                        const PString & name)
{
  return capabilities.AddAllCapabilities(*this, descriptorNum, simultaneous, name);
}


void H323EndPoint::AddAllUserInputCapabilities(PINDEX descriptorNum,
                                               PINDEX simultaneous)
{
  H323_UserInputCapability::AddAllCapabilities(capabilities, descriptorNum, simultaneous);
}


void H323EndPoint::RemoveCapabilities(const PStringArray & codecNames)
{
  capabilities.Remove(codecNames);
}


void H323EndPoint::ReorderCapabilities(const PStringArray & preferenceOrder)
{
  capabilities.Reorder(preferenceOrder);
}


const H323Capabilities & H323EndPoint::GetCapabilities() const
{
  if (capabilities.GetSize() == 0) {
    capabilities.AddAllCapabilities(*this, P_MAX_INDEX, P_MAX_INDEX, "*");
    H323_UserInputCapability::AddAllCapabilities(capabilities, P_MAX_INDEX, P_MAX_INDEX);
  }

  return capabilities;
}


PBoolean H323EndPoint::UseGatekeeper(const PString & address,
                                 const PString & identifier,
                                 const PString & localAddress)
{
  if (gatekeeper != NULL) {
    PBoolean same = PTrue;

    if (!address && address != "*")
      same = gatekeeper->GetTransport().GetRemoteAddress().IsEquivalent(address);

    if (!same && !identifier)
      same = gatekeeper->GetIdentifier() == identifier;

    if (!same && !localAddress)
      same = gatekeeper->GetTransport().GetLocalAddress().IsEquivalent(localAddress);

    if (same) {
      PTRACE(3, "H323\tUsing existing gatekeeper " << *gatekeeper);
      return PTrue;
    }
  }

  H323Transport * transport = NULL;
  if (!localAddress.IsEmpty()) {
    H323TransportAddress iface(localAddress);
    PIPSocket::Address ip;
    WORD port = H225_RAS::DefaultRasUdpPort;
    if (iface.GetIpAndPort(ip, port))
      transport = new H323TransportUDP(*this, ip, port);
  }

  if (address.IsEmpty() || address == "*") {
    if (identifier.IsEmpty())
      return DiscoverGatekeeper(transport);
    else
      return LocateGatekeeper(identifier, transport);
  }
  else {
    if (identifier.IsEmpty())
      return SetGatekeeper(address, transport);
    else
      return SetGatekeeperZone(address, identifier, transport);
  }
}


PBoolean H323EndPoint::SetGatekeeper(const PString & address, H323Transport * transport)
{
  H323TransportAddress h323addr(address, H225_RAS::DefaultRasUdpPort, "udp");
  return InternalCreateGatekeeper(transport) && gatekeeper->DiscoverByAddress(h323addr);
}


PBoolean H323EndPoint::SetGatekeeperZone(const PString & address,
                                     const PString & identifier,
                                     H323Transport * transport)
{
  H323TransportAddress h323addr(address, H225_RAS::DefaultRasUdpPort, "udp");
  return InternalCreateGatekeeper(transport) && gatekeeper->DiscoverByNameAndAddress(identifier, h323addr);
}


PBoolean H323EndPoint::LocateGatekeeper(const PString & identifier, H323Transport * transport)
{
  return InternalCreateGatekeeper(transport) && gatekeeper->DiscoverByName(identifier);
}


PBoolean H323EndPoint::DiscoverGatekeeper(H323Transport * transport)
{
  return InternalCreateGatekeeper(transport) && gatekeeper->DiscoverAny();
}


bool H323EndPoint::InternalCreateGatekeeper(H323Transport * transport)
{
  RemoveGatekeeper(H225_UnregRequestReason::e_reregistrationRequired);

  if (transport == NULL)
    transport = new H323TransportUDP(*this);

  gatekeeper = CreateGatekeeper(transport);
  if (gatekeeper == NULL)
    return false;

  gatekeeper->SetPassword(gatekeeperPassword, gatekeeperUsername);
  return true;
}


H323Gatekeeper * H323EndPoint::CreateGatekeeper(H323Transport * transport)
{
  return new H323Gatekeeper(*this, transport);
}


PBoolean H323EndPoint::IsRegisteredWithGatekeeper() const
{
  if (gatekeeper == NULL)
    return PFalse;

  return gatekeeper->IsRegistered();
}


PBoolean H323EndPoint::RemoveGatekeeper(int reason)
{
  PBoolean ok = PTrue;

  if (gatekeeper == NULL)
    return ok;

  ClearAllCalls();

  if (gatekeeper->IsRegistered()) // If we are registered send a URQ
    ok = gatekeeper->UnregistrationRequest(reason);

  delete gatekeeper;
  gatekeeper = NULL;

  return ok;
}


void H323EndPoint::SetGatekeeperPassword(const PString & password, const PString & username)
{
  gatekeeperUsername = username;
  gatekeeperPassword = password;

  if (gatekeeper != NULL) {
    gatekeeper->SetPassword(gatekeeperPassword, gatekeeperUsername);
    if (gatekeeper->IsRegistered()) // If we are registered send a URQ
      gatekeeper->UnregistrationRequest(H225_UnregRequestReason::e_reregistrationRequired);
    gatekeeper->RegistrationRequest();
  }
}

void H323EndPoint::OnGatekeeperConfirm()
{
}

void H323EndPoint::OnGatekeeperReject()
{
}

void H323EndPoint::OnRegistrationConfirm()
{
}

void H323EndPoint::OnRegistrationReject()
{
}

H235Authenticators H323EndPoint::CreateAuthenticators()
{
  H235Authenticators authenticators;

  PFactory<H235Authenticator>::KeyList_T keyList = PFactory<H235Authenticator>::GetKeyList();
  PFactory<H235Authenticator>::KeyList_T::const_iterator r;
  for (r = keyList.begin(); r != keyList.end(); ++r)
    authenticators.Append(PFactory<H235Authenticator>::CreateInstance(*r));

  PTRACE(3, "H323\tAuthenticator list is size " << (int)authenticators.GetSize());

  return authenticators;
}


PSafePtr<OpalConnection> H323EndPoint::MakeConnection(OpalCall & call,
                                                 const PString & remoteParty,
                                                          void * userData,
                                                    unsigned int options,
                                 OpalConnection::StringOptions * stringOptions)
{
  if (listeners.IsEmpty())
    return NULL;

  PTRACE(3, "H323\tMaking call to: " << remoteParty);
  return InternalMakeCall(call,
                          PString::Empty(),
                          PString::Empty(),
                          UINT_MAX,
                          remoteParty,
                          userData,
                          options,
                          stringOptions);
}


PBoolean H323EndPoint::NewIncomingConnection(OpalTransport * transport)
{
  PTRACE(3, "H225\tAwaiting first PDU");
  transport->SetReadTimeout(15000); // Await 15 seconds after connect for first byte
  H323SignalPDU pdu;
  if (!pdu.Read(*transport)) {
    PTRACE(1, "H225\tFailed to get initial Q.931 PDU, connection not started.");
    return PTrue;
  }

  unsigned callReference = pdu.GetQ931().GetCallReference();
  PTRACE(3, "H225\tIncoming call, first PDU: callReference=" << callReference);

  // Get a new (or old) connection from the endpoint, calculate token
  PString token = transport->GetRemoteAddress();
  token.sprintf("/%u", callReference);

  PSafePtr<H323Connection> connection = FindConnectionWithLock(token);

  if (connection == NULL) {
    // Get new instance of a call, abort if none created
    OpalCall * call = manager.InternalCreateCall();
    if (call != NULL)
      connection = CreateConnection(*call, token, NULL, *transport, PString::Empty(), PString::Empty(), &pdu);

    if (!AddConnection(connection)) {
      PTRACE(1, "H225\tEndpoint could not create connection, "
                "sending release complete PDU: callRef=" << callReference);

      H323SignalPDU releaseComplete;
      Q931 &q931PDU = releaseComplete.GetQ931();
      q931PDU.BuildReleaseComplete(callReference, PTrue);
      releaseComplete.m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_releaseComplete);

      H225_ReleaseComplete_UUIE &release = releaseComplete.m_h323_uu_pdu.m_h323_message_body;
      release.m_protocolIdentifier.SetValue(psprintf("0.0.8.2250.0.%u", H225_PROTOCOL_VERSION));

      H225_Setup_UUIE &setup = pdu.m_h323_uu_pdu.m_h323_message_body;
      if (setup.HasOptionalField(H225_Setup_UUIE::e_callIdentifier)) {
        release.IncludeOptionalField(H225_Setup_UUIE::e_callIdentifier);
        release.m_callIdentifier = setup.m_callIdentifier;
      }

      // Set the cause value
      q931PDU.SetCause(Q931::TemporaryFailure);

      // Send the PDU
      releaseComplete.Write(*transport);

      return PTrue;
    }
  }

  PTRACE(3, "H323\tCreated new connection: " << token);
  connection->AttachSignalChannel(token, transport, PTrue);

  if (connection->HandleSignalPDU(pdu)) {
    // All subsequent PDU's should wait forever
    transport->SetReadTimeout(PMaxTimeInterval);
    connection->HandleSignallingChannel();
  }
  else {
    connection->ClearCall(H323Connection::EndedByTransportFail);
    PTRACE(1, "H225\tSignal channel stopped on first PDU.");
  }

  return PFalse;
}


H323Connection * H323EndPoint::CreateConnection(OpalCall & call,
                                                const PString & token,
                                                void * /*userData*/,
                                                OpalTransport & /*transport*/,
                                                const PString & alias,
                                                const H323TransportAddress & address,
                                                H323SignalPDU * /*setupPDU*/,
                                                unsigned options,
                                                OpalConnection::StringOptions * stringOptions)
{
  return new H323Connection(call, *this, token, alias, address, options, stringOptions);
}


PBoolean H323EndPoint::SetupTransfer(const PString & oldToken,
                                 const PString & callIdentity,
                                 const PString & remoteParty,
                                 void * userData)
{
  // Make a new connection
  PSafePtr<OpalConnection> otherConnection =
    GetConnectionWithLock(oldToken, PSafeReference);
  if (otherConnection == NULL) {
    return PFalse;
  }

  OpalCall & call = otherConnection->GetCall();

  call.CloseMediaStreams();

  PTRACE(3, "H323\tTransferring call to: " << remoteParty);
  PBoolean ok = InternalMakeCall(call, oldToken, callIdentity, UINT_MAX, remoteParty, userData) != NULL;
  call.OnReleased(*otherConnection);
  otherConnection->Release(OpalConnection::EndedByCallForwarded);

  return ok;
}


H323Connection * H323EndPoint::InternalMakeCall(OpalCall & call,
                                           const PString & existingToken,
#if OPAL_H450
                                           const PString & callIdentity,
                                                  unsigned capabilityLevel,
#else
                                           const PString & ,
                                                  unsigned ,
#endif
                                           const PString & remoteParty,
                                                    void * userData,
                                              unsigned int options,
                           OpalConnection::StringOptions * stringOptions)
{
  OpalConnection::StringOptions localStringOptions;
  if (stringOptions == NULL)
    stringOptions = &localStringOptions;

  PString alias;
  H323TransportAddress address;
  if (!ParsePartyName(remoteParty, alias, address, stringOptions)) {
    PTRACE(2, "H323\tCould not parse \"" << remoteParty << '"');
    return NULL;
  }

  // Restriction: the call must be made on the same local interface as the one
  // that the gatekeeper is using.
  H323Transport * transport;
  if (gatekeeper != NULL)
    transport = gatekeeper->GetTransport().GetLocalAddress().CreateTransport(
                                          *this, OpalTransportAddress::Streamed);
  else if (!stringOptions->Contains(OPAL_OPT_INTERFACE))
    transport = address.CreateTransport(*this, OpalTransportAddress::NoBinding);
  else {
    OpalTransportAddress localInterface = (*stringOptions)[OPAL_OPT_INTERFACE];
    transport = localInterface.CreateTransport(*this, OpalTransportAddress::HostOnly);
  }

  if (transport == NULL) {
    PTRACE(1, "H323\tInvalid transport in \"" << remoteParty << '"');
    return NULL;
  }

  inUseFlag.Wait();

  PString newToken;
  if (existingToken.IsEmpty()) {
    do {
      newToken = psprintf("localhost/%u", Q931::GenerateCallReference());
    } while (connectionsActive.Contains(newToken));
  }

  H323Connection * connection = CreateConnection(call, newToken, userData, *transport, alias, address, NULL, options, stringOptions);
  if (!AddConnection(connection)) {
    PTRACE(1, "H225\tEndpoint could not create connection, aborting setup.");
    return NULL;
  }

  inUseFlag.Signal();

  connection->AttachSignalChannel(newToken, transport, PFalse);

#if OPAL_H450
  if (!callIdentity) {
    if (capabilityLevel == UINT_MAX)
      connection->HandleTransferCall(existingToken, callIdentity);
    else {
      connection->HandleIntrudeCall(existingToken, callIdentity);
      connection->IntrudeCall(capabilityLevel);
    }
  }
#endif

  PTRACE(3, "H323\tCreated new connection: " << newToken);

  // See if we are starting an outgoing connection as first in a call
  if (call.GetConnection(0) == (OpalConnection*)connection || !existingToken.IsEmpty())
    connection->SetUpConnection();

  return connection;
}


#if OPAL_H450

void H323EndPoint::TransferCall(const PString & token, 
                                const PString & remoteParty,
                                const PString & callIdentity)
{
  PSafePtr<H323Connection> connection = FindConnectionWithLock(token);
  if (connection != NULL)
    connection->TransferCall(remoteParty, callIdentity);
}


void H323EndPoint::ConsultationTransfer(const PString & primaryCallToken,   
                                        const PString & secondaryCallToken)
{
  PSafePtr<H323Connection> secondaryCall = FindConnectionWithLock(secondaryCallToken);
  if (secondaryCall != NULL)
    secondaryCall->ConsultationTransfer(primaryCallToken);
}


void H323EndPoint::HoldCall(const PString & token, PBoolean localHold)
{
  PSafePtr<H323Connection> connection = FindConnectionWithLock(token);
  if (connection != NULL)
    connection->HoldCall(localHold);
}


PBoolean H323EndPoint::IntrudeCall(const PString & remoteParty,
                               unsigned capabilityLevel,
                               void * userData)
{
  // Get new instance of a call, abort if none created
  OpalCall * call = manager.InternalCreateCall();
  if (call == NULL)
    return false;

  return InternalMakeCall(*call, PString::Empty(), PString::Empty(), capabilityLevel, remoteParty, userData) != NULL;
}

#endif

PBoolean H323EndPoint::OnSendConnect(H323Connection & /*connection*/,
                                 H323SignalPDU & /*connectPDU*/
                                )
{
  return PTrue;
}

PBoolean H323EndPoint::OnSendSignalSetup(H323Connection & /*connection*/,
                                     H323SignalPDU & /*setupPDU*/)
{
  return PTrue;
}

PBoolean H323EndPoint::OnSendCallProceeding(H323Connection & /*connection*/,
                                        H323SignalPDU & /*callProceedingPDU*/)
{
  return PTrue;
}

void H323EndPoint::OnReceivedInitiateReturnError()
{
}

PBoolean H323EndPoint::ParsePartyName(const PString & remoteParty,
                                            PString & alias,
                               H323TransportAddress & address,
                       OpalConnection::StringOptions * stringOptions)
{
  PURL url(remoteParty, GetPrefixName()); // Parses as per RFC3508

  if (stringOptions != NULL)
    stringOptions->ExtractFromURL(url);

#if OPAL_PTLIB_DNS

  // if there is no gatekeeper, try altarnate address lookup methods
  if (gatekeeper == NULL) {
    PString hostname = url.GetHostName();

    // No host, so lets try ENUM on the username part
    if (hostname.IsEmpty()) {
      PString username = url.GetUserName();
      // make sure the number has only digits and +
      if (username.FindSpan("+0123456789") == P_MAX_INDEX) {
        PString newName;
        if (PDNS::ENUMLookup(username, "E2U+h323", newName)) {
          PTRACE(4, "H323\tENUM converted remote party " << username << " to " << newName);
          url.Parse(newName, GetPrefixName());
        }
      }
    }

    // If it is a valid IP address then can't be a domain so do not try SRV record lookup
    if (!hostname.IsEmpty()) {
      PIPSocket::Address ip = hostname;
      if (!ip.IsValid()) {
        PIPSocketAddressAndPortVector addresses;
        if (PDNS::LookupSRV(hostname, "_h323._tcp", url.GetPort(), addresses) && !addresses.empty()) {
          // Only use first entry
          url.SetHostName(addresses[0].GetAddress().AsString());
          url.SetPort(addresses[0].GetPort());
        }
      }
    }
  }

#endif

  alias = url.GetUserName();

  address = url.GetHostName();
  if (!address && url.GetPort() != 0)
    address.sprintf(":%u", url.GetPort());

  if (alias.IsEmpty() && address.IsEmpty()) {
    PTRACE(1, "H323\tAttempt to use invalid URL \"" << remoteParty << '"');
    return PFalse;
  }

  PBoolean gatekeeperSpecified = PFalse;
  PBoolean gatewaySpecified = PFalse;

  PCaselessString type = url.GetParamVars()("type");

  if (url.GetScheme() == "callto") {
    // Do not yet support ILS
    if (type == "directory") {
#if OPAL_PTLIB_LDAP
      PString server = url.GetHostName();
      if (server.IsEmpty())
        server = GetDefaultILSServer();
      if (server.IsEmpty())
        return PFalse;

      PILSSession ils;
      if (!ils.Open(server, url.GetPort())) {
        PTRACE(1, "H323\tCould not open ILS server at \"" << server
               << "\" - " << ils.GetErrorText());
        return PFalse;
      }

      PILSSession::RTPerson person;
      if (!ils.SearchPerson(alias, person)) {
        PTRACE(1, "H323\tCould not find "
               << server << '/' << alias << ": " << ils.GetErrorText());
        return PFalse;
      }

      if (!person.sipAddress.IsValid()) {
        PTRACE(1, "H323\tILS user " << server << '/' << alias
               << " does not have a valid IP address");
        return PFalse;
      }

      // Get the IP address
      address = H323TransportAddress(person.sipAddress);

      // Get the port if provided
      for (PINDEX i = 0; i < person.sport.GetSize(); i++) {
        if (person.sport[i] != 1503) { // Dont use the T.120 port
          address = H323TransportAddress(person.sipAddress, person.sport[i]);
          break;
        }
      }

      alias = PString::Empty(); // No alias for ILS lookup, only host
      return PTrue;
#else
      return PFalse;
#endif
    }

    if (url.GetParamVars().Contains("gateway"))
      gatewaySpecified = PTrue;
  }
  else if (url.GetScheme() == "h323") {
    if (type == "gw")
      gatewaySpecified = PTrue;
    else if (type == "gk")
      gatekeeperSpecified = PTrue;
    else if (!type) {
      PTRACE(1, "H323\tUnsupported host type \"" << type << "\" in h323 URL");
      return PFalse;
    }
  }

  // User explicitly asked fo a look up to a gk
  if (gatekeeperSpecified) {
    if (alias.IsEmpty()) {
      PTRACE(1, "H323\tAttempt to use explict gatekeeper without alias!");
      return PFalse;
    }

    if (address.IsEmpty()) {
      PTRACE(1, "H323\tAttempt to use explict gatekeeper without address!");
      return PFalse;
    }

    H323TransportAddress gkAddr = address;
    PTRACE(3, "H323\tLooking for \"" << alias << "\" on gatekeeper at " << gkAddr);

    H323Gatekeeper * gk = CreateGatekeeper(new H323TransportUDP(*this));

    PBoolean ok = gk->DiscoverByAddress(gkAddr);
    if (ok) {
      ok = gk->LocationRequest(alias, address);
      if (ok) {
        PTRACE(3, "H323\tLocation Request of \"" << alias << "\" on gk " << gkAddr << " found " << address);
      }
      else {
        PTRACE(1, "H323\tLocation Request failed for \"" << alias << "\" on gk " << gkAddr);
      }
    }
    else {
      PTRACE(1, "H323\tLocation Request discovery failed for gk " << gkAddr);
    }

    delete gk;

    return ok;
  }

  // User explicitly said to use a gw, or we do not have a gk to do look up
  if (gatekeeper == NULL || gatewaySpecified) {
    /* If URL did not have an '@' as per RFC3508 we get an alias but no
       address, but user said to use gw, or we do not have a gk to do a lookup
       so we MUST have a host. So, we swap the alias into the address field
       and blank out the alias. */
    if (address.IsEmpty()) {
      PStringArray transports = GetDefaultTransport().Tokenise(',');
      address = H323TransportAddress(alias, GetDefaultSignalPort(), transports[0]);
      alias = PString::Empty();
    }
    return PTrue;
  }

  // We have a gatekeeper and user provided a host, all done
  if (!address)
    return PTrue;

  // We have a gk and user did not explicitly supply a host, so lets
  // do a check to see if it is an IP address 
  if (alias.FindRegEx("^[0-9][0-9]*\\.[0-9][0-9]*\\.[0-9][0-9]*\\.[0-9][0-9]*") != P_MAX_INDEX) {
    PINDEX colon = alias.Find(':');
    WORD port = GetDefaultSignalPort();
    if (colon != P_MAX_INDEX) {
      port = (WORD)alias.Mid(colon+1).AsInteger();
      alias = alias.Left(colon);
    }
    PIPSocket::Address ip(alias);
    if (ip.IsValid()) {
      alias = PString::Empty();
      address = H323TransportAddress(ip, port);
    }
  }

  return PTrue;
}


PSafePtr<H323Connection> H323EndPoint::FindConnectionWithLock(const PString & token, PSafetyMode mode)
{
  PSafePtr<H323Connection> connnection = PSafePtrCast<OpalConnection, H323Connection>(GetConnectionWithLock(token, mode));
  if (connnection != NULL)
    return connnection;

  for (PSafePtr<OpalConnection> conn(connectionsActive, mode); conn != NULL; ++conn) {
    connnection = PSafePtrCast<OpalConnection, H323Connection>(conn);
    if(connnection != NULL) {      //cast success
      if (connnection->GetCallIdentifier().AsString() == token)
        return connnection;
      if (connnection->GetConferenceIdentifier().AsString() == token)
        return connnection;
    }
  }

  return NULL;
}


PBoolean H323EndPoint::OnIncomingCall(H323Connection & connection,
                                  const H323SignalPDU & /*setupPDU*/,
                                  H323SignalPDU & /*alertingPDU*/)
{
  return connection.OnIncomingConnection(0, NULL);
}


PBoolean H323EndPoint::OnCallTransferInitiate(H323Connection & /*connection*/,
                                          const PString & /*remoteParty*/)
{
  return PTrue;
}


PBoolean H323EndPoint::OnCallTransferIdentify(H323Connection & /*connection*/)
{
  return PTrue;
}


void H323EndPoint::OnSendARQ(H323Connection & /*conn*/, H225_AdmissionRequest & /*arq*/)
{
}


OpalConnection::AnswerCallResponse
       H323EndPoint::OnAnswerCall(H323Connection & connection,
                                  const PString & caller,
                                  const H323SignalPDU & /*setupPDU*/,
                                  H323SignalPDU & /*connectPDU*/,
                                  H323SignalPDU & /*progressPDU*/)
{
  return connection.OnAnswerCall(caller);
}

OpalConnection::AnswerCallResponse
       H323EndPoint::OnAnswerCall(OpalConnection & connection,
       const PString & caller
)
{
  return OpalEndPoint::OnAnswerCall(connection, caller);
}

PBoolean H323EndPoint::OnAlerting(H323Connection & connection,
                              const H323SignalPDU & /*alertingPDU*/,
                              const PString & /*username*/)
{
  PTRACE(3, "H225\tReceived alerting PDU.");
  ((OpalConnection&)connection).OnAlerting();
  return PTrue;
}

PBoolean H323EndPoint::OnSendAlerting(H323Connection & PTRACE_PARAM(connection),
                                  H323SignalPDU & /*alerting*/,
                                  const PString & /*calleeName*/,   /// Name of endpoint being alerted.
                                  PBoolean /*withMedia*/                /// Open media with alerting
                                  )
{
  PTRACE(3, "H225\tOnSendAlerting conn = " << connection);
  return PTrue;
}

PBoolean H323EndPoint::OnSentAlerting(H323Connection & PTRACE_PARAM(connection))
{
  PTRACE(3, "H225\tOnSentAlerting conn = " << connection);
  return PTrue;
}

PBoolean H323EndPoint::OnConnectionForwarded(H323Connection & /*connection*/,
                                         const PString & /*forwardParty*/,
                                         const H323SignalPDU & /*pdu*/)
{
  return PFalse;
}


PBoolean H323EndPoint::ForwardConnection(H323Connection & connection,
                                     const PString & forwardParty,
                                     const H323SignalPDU & /*pdu*/)
{
  if (InternalMakeCall(connection.GetCall(),
                       connection.GetCallToken(),
                       PString::Empty(),
                       UINT_MAX,
                       forwardParty,
                       NULL) == NULL)
    return PFalse;

  connection.SetCallEndReason(H323Connection::EndedByCallForwarded);

  return PTrue;
}


void H323EndPoint::OnConnectionEstablished(H323Connection & /*connection*/,
                                           const PString & /*token*/)
{
}


PBoolean H323EndPoint::IsConnectionEstablished(const PString & token)
{
  PSafePtr<H323Connection> connection = FindConnectionWithLock(token);
  return connection != NULL && connection->IsEstablished();
}


PBoolean H323EndPoint::OnOutgoingCall(H323Connection & /*connection*/,
                                  const H323SignalPDU & /*connectPDU*/)
{
  PTRACE(3, "H225\tReceived connect PDU.");
  return PTrue;
}


void H323EndPoint::OnConnectionCleared(H323Connection & /*connection*/,
                                       const PString & /*token*/)
{
}


#if PTRACING
static void OnStartStopChannel(const char * startstop, const H323Channel & channel)
{
  const char * dir;
  switch (channel.GetDirection()) {
    case H323Channel::IsTransmitter :
      dir = "send";
      break;

    case H323Channel::IsReceiver :
      dir = "receiv";
      break;

    default :
      dir = "us";
      break;
  }

  PTRACE(3, "H323\t" << startstop << "ed "
                     << dir << "ing logical channel: "
                     << channel.GetCapability());
}
#endif


PBoolean H323EndPoint::OnStartLogicalChannel(H323Connection & /*connection*/,
                                         H323Channel & PTRACE_PARAM(channel))
{
#if PTRACING
  OnStartStopChannel("Start", channel);
#endif
  return PTrue;
}


void H323EndPoint::OnClosedLogicalChannel(H323Connection & /*connection*/,
                                          const H323Channel & PTRACE_PARAM(channel))
{
#if PTRACING
  OnStartStopChannel("Stopp", channel);
#endif
}


void H323EndPoint::OnRTPStatistics(const H323Connection & connection,
                                   const RTP_Session & session) const
{
  manager.OnRTPStatistics(connection, session);
}


void H323EndPoint::OnGatekeeperNATDetect(PIPSocket::Address /* publicAddr*/,
					 PString & /*gkIdentifier*/,
					 H323TransportAddress & /*gkRouteAddress*/)
{
}


void H323EndPoint::OnHTTPServiceControl(unsigned /*opeartion*/,
                                        unsigned /*sessionId*/,
                                        const PString & /*url*/)
{
}


void H323EndPoint::OnCallCreditServiceControl(const PString & /*amount*/, PBoolean /*mode*/)
{
}


void H323EndPoint::OnServiceControlSession(unsigned type,
                                           unsigned sessionId,
                                           const H323ServiceControlSession & session,
                                           H323Connection * connection)
{
  session.OnChange(type, sessionId, *this, connection);
}

PBoolean H323EndPoint::OnConferenceInvite(const H323SignalPDU & /*setupPDU */)
{
  return PFalse;
}

PBoolean H323EndPoint::OnCallIndependentSupplementaryService(const H323SignalPDU & /* setupPDU */)
{
  return PFalse;
}

PBoolean H323EndPoint::OnNegotiateConferenceCapabilities(const H323SignalPDU & /* setupPDU */)
{
  return PFalse;
}

H323ServiceControlSession * H323EndPoint::CreateServiceControlSession(const H225_ServiceControlDescriptor & contents)
{
  switch (contents.GetTag()) {
    case H225_ServiceControlDescriptor::e_url :
      return new H323HTTPServiceControl(contents);

    case H225_ServiceControlDescriptor::e_callCreditServiceControl :
      return new H323CallCreditServiceControl(contents);
  }

  return NULL;
}


void H323EndPoint::SetDefaultLocalPartyName(const PString & name)
{
  SetLocalUserName(name);
  OpalEndPoint::SetDefaultLocalPartyName(name);
}


void H323EndPoint::SetLocalUserName(const PString & name)
{
  PAssert(!name, "Must have non-empty string in AliasAddress!");
  if (name.IsEmpty())
    return;

  localAliasNames.RemoveAll();
  localAliasNames.AppendString(name);
}


PBoolean H323EndPoint::AddAliasName(const PString & name)
{
  PAssert(!name, "Must have non-empty string in AliasAddress!");

  if (localAliasNames.GetValuesIndex(name) != P_MAX_INDEX)
    return PFalse;

  localAliasNames.AppendString(name);
  return PTrue;
}


PBoolean H323EndPoint::RemoveAliasName(const PString & name)
{
  PINDEX pos = localAliasNames.GetValuesIndex(name);
  if (pos == P_MAX_INDEX)
    return PFalse;

  PAssert(localAliasNames.GetSize() > 1, "Must have at least one AliasAddress!");
  if (localAliasNames.GetSize() < 2)
    return PFalse;

  localAliasNames.RemoveAt(pos);
  return PTrue;
}


bool H323EndPoint::AddAliasNamePattern(const PString & pattern)
{
  PAssert(!pattern, "Must have non-empty string in AddressPattern !");

  if (localAliasPatterns.GetValuesIndex(pattern) != P_MAX_INDEX)
    return false;

  localAliasPatterns.AppendString(pattern);
  return true;
}


PBoolean H323EndPoint::IsTerminal() const
{
  switch (terminalType) {
    case e_TerminalOnly :
    case e_TerminalAndMC :
      return PTrue;

    default :
      return PFalse;
  }
}


PBoolean H323EndPoint::IsGateway() const
{
  switch (terminalType) {
    case e_GatewayOnly :
    case e_GatewayAndMC :
    case e_GatewayAndMCWithDataMP :
    case e_GatewayAndMCWithAudioMP :
    case e_GatewayAndMCWithAVMP :
      return PTrue;

    default :
      return PFalse;
  }
}


PBoolean H323EndPoint::IsGatekeeper() const
{
  switch (terminalType) {
    case e_GatekeeperOnly :
    case e_GatekeeperWithDataMP :
    case e_GatekeeperWithAudioMP :
    case e_GatekeeperWithAVMP :
      return PTrue;

    default :
      return PFalse;
  }
}


PBoolean H323EndPoint::IsMCU() const
{
  switch (terminalType) {
    case e_MCUOnly :
    case e_MCUWithDataMP :
    case e_MCUWithAudioMP :
    case e_MCUWithAVMP :
      return PTrue;

    default :
      return PFalse;
  }
}


void H323EndPoint::TranslateTCPAddress(PIPSocket::Address & localAddr,
                                       const PIPSocket::Address & remoteAddr)
{
  manager.TranslateIPAddress(localAddr, remoteAddr);
}


PBoolean H323EndPoint::OnSendFeatureSet(unsigned id, H225_FeatureSet & featureSet)
{
#if OPAL_H460
  return features.SendFeature(id, featureSet);
#else
  return false;
#endif
}


void H323EndPoint::OnReceiveFeatureSet(unsigned id, const H225_FeatureSet & featureSet)
{
#if OPAL_H460
  features.ReceiveFeature(id, featureSet);
#endif
}


void H323EndPoint::LoadBaseFeatureSet()
{
#if OPAL_H460
  features.AttachEndPoint(this);
  features.LoadFeatureSet(H460_Feature::FeatureBase);
#endif
}


bool H323EndPoint::OnFeatureInstance(int instType, const PString & identifer)
{
  return true;
}


#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////
