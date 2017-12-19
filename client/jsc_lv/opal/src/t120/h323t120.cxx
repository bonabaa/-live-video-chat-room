/*
 * h323t120.cxx
 *
 * H.323 T.120 logical channel establishment
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
 * Contributor(s): ______________________________________.
 *
 * $Revision: 19279 $
 * $Author: rjongbloed $
 * $Date: 2008-01-17 04:08:34 +0000 (Thu, 17 Jan 2008) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h323t120.h"
#endif

#include <opal/buildopts.h>

#if OPAL_T120DATA

#include <t120/h323t120.h>
#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/transaddr.h>
#include <t120/t120proto.h>
#include <t120/x224.h>
#include <asn/h245.h>


#define new PNEW

#define T120_MAX_BIT_RATE 825000


/////////////////////////////////////////////////////////////////////////////

H323_T120Capability::H323_T120Capability()
  : H323DataCapability(T120_MAX_BIT_RATE)
{
  dynamicPortCapability = PTrue;
}


PObject * H323_T120Capability::Clone() const
{
  return new H323_T120Capability(*this);
}


unsigned H323_T120Capability::GetSubType() const
{
  return H245_DataApplicationCapability_application::e_t120;
}


PString H323_T120Capability::GetFormatName() const
{
  return OPAL_T120;
}


H323Channel * H323_T120Capability::CreateChannel(H323Connection & connection,
                                                 H323Channel::Directions direction,
                                                 unsigned sessionID,
                                const H245_H2250LogicalChannelParameters *) const
{
  return new H323_T120Channel(connection, *this, direction, sessionID);
}


PBoolean H323_T120Capability::OnSendingPDU(H245_DataApplicationCapability & pdu) const
{
  pdu.m_application.SetTag(H245_DataApplicationCapability_application::e_t120);
  return OnSendingPDU((H245_DataProtocolCapability &)pdu.m_application);
}


PBoolean H323_T120Capability::OnSendingPDU(H245_DataMode & pdu) const
{
  pdu.m_application.SetTag(H245_DataMode_application::e_t120);
  return OnSendingPDU((H245_DataProtocolCapability &)pdu.m_application);
}


PBoolean H323_T120Capability::OnSendingPDU(H245_DataProtocolCapability & pdu) const
{
  pdu.SetTag(H245_DataProtocolCapability::e_separateLANStack);
  return PTrue;
}


PBoolean H323_T120Capability::OnReceivedPDU(const H245_DataApplicationCapability & cap)
{
  if (cap.m_application.GetTag() != H245_DataApplicationCapability_application::e_t120)
    return PFalse;

  const H245_DataProtocolCapability & dataCap = cap.m_application;

  return dataCap.GetTag() == H245_DataProtocolCapability::e_separateLANStack;
}


/////////////////////////////////////////////////////////////////////////////

H323_T120Channel::H323_T120Channel(H323Connection & connection,
                                   const H323Capability & capability,
                                   Directions direction,
                                   unsigned id)
  : H323DataChannel(connection, capability, direction, id)
{
  t120handler = NULL;
  PTRACE(3, "H323T120\tCreated logical channel for T.120");
}


void H323_T120Channel::Receive()
{
  HandleChannel();
}


void H323_T120Channel::Transmit()
{
  HandleChannel();
}


void H323_T120Channel::HandleChannel()
{
  PTRACE(2, "H323T120\tThread started.");

  if (t120handler == NULL) {
    PTRACE(1, "H323T120\tNo protocol handler, aborting thread.");
  }
  else if (transport == NULL && listener == NULL) {
    PTRACE(1, "H323T120\tNo listener or transport, aborting thread.");
  }
  else if (listener != NULL) {
    if ((transport = listener->Accept(30000)) != NULL)  // 30 second wait for connect back
      t120handler->Answer(*transport);
    else {
      PTRACE(1, "H323T120\tAccept failed, aborting thread.");
    }
  }
  else if (transport->IsOpen())
    t120handler->Originate(*transport);
  else {
    PTRACE(1, "H323T120\tConnect failed, aborting thread.");
  }

  connection.CloseLogicalChannelNumber(number);

  PTRACE(2, "H323T120\tThread ended");
}


PBoolean H323_T120Channel::OnSendingPDU(H245_OpenLogicalChannel & open) const
{
  if (!H323DataChannel::OnSendingPDU(open))
    return PFalse;

  if (!((H323_T120Channel*)this)->CreateListener()) {
    PTRACE(1, "H323T120\tCould not create listener");
    return PFalse;
  }

  PTRACE(3, "H323T120\tOnSendingPDU");

  open.IncludeOptionalField(H245_OpenLogicalChannel::e_separateStack);
  open.m_separateStack.IncludeOptionalField(H245_NetworkAccessParameters::e_distribution);
  open.m_separateStack.m_distribution.SetTag(H245_NetworkAccessParameters_distribution::e_unicast);
  open.m_separateStack.m_networkAddress.SetTag(H245_NetworkAccessParameters_networkAddress::e_localAreaAddress);
  H245_TransportAddress & h245addr = open.m_separateStack.m_networkAddress;
  H323TransportAddress h323addr = listener->GetLocalAddress(connection.GetControlChannel().GetLocalAddress());
  return h323addr.SetPDU(h245addr, endpoint.GetDefaultSignalPort());
}


void H323_T120Channel::OnSendOpenAck(const H245_OpenLogicalChannel & /*open*/,
                                     H245_OpenLogicalChannelAck & ack) const
{
  PTRACE(3, "H323T120\tOnSendOpenAck");

  if (listener != NULL || transport != NULL) {
    ack.IncludeOptionalField(H245_OpenLogicalChannelAck::e_separateStack);
    ack.m_separateStack.IncludeOptionalField(H245_NetworkAccessParameters::e_distribution);
    ack.m_separateStack.m_distribution.SetTag(H245_NetworkAccessParameters_distribution::e_unicast);
    ack.m_separateStack.m_networkAddress.SetTag(H245_NetworkAccessParameters_networkAddress::e_localAreaAddress);
    H245_TransportAddress & h245addr = ack.m_separateStack.m_networkAddress;

    H323TransportAddress h323addr;
    if (listener != NULL)
      h323addr = listener->GetLocalAddress(connection.GetControlChannel().GetLocalAddress());
    else
      h323addr = transport->GetLocalAddress();
    h323addr.SetPDU(h245addr, endpoint.GetDefaultSignalPort());
  }
}


PBoolean H323_T120Channel::OnReceivedPDU(const H245_OpenLogicalChannel & open,
                                     unsigned & errorCode)
{
  number = H323ChannelNumber(open.m_forwardLogicalChannelNumber, PTrue);

  PTRACE(3, "H323T120\tOnReceivedPDU for channel: " << number);

  H323EndPoint & endpoint = connection.GetEndPoint();

  t120handler = connection.CreateT120ProtocolHandler();
  if (t120handler == NULL) {
    PTRACE(1, "H323T120\tCould not create protocol handler");
    errorCode = H245_OpenLogicalChannelReject_cause::e_dataTypeNotAvailable;
    return PFalse;
  }

  PBoolean listen = connection.HadAnsweredCall();

  H323TransportAddress address;
  if (open.HasOptionalField(H245_OpenLogicalChannel::e_separateStack) &&
    open.m_separateStack.m_networkAddress.GetTag() == H245_NetworkAccessParameters_networkAddress::e_localAreaAddress) {
    address = (const H245_TransportAddress &)open.m_separateStack.m_networkAddress;
    if (open.m_separateStack.HasOptionalField(H245_NetworkAccessParameters::e_t120SetupProcedure))
      listen = open.m_separateStack.m_t120SetupProcedure.GetTag() ==
                            H245_NetworkAccessParameters_t120SetupProcedure::e_waitForCall;
  }

  if (listen) {
    if (!address)
      listener = address.CreateListener(endpoint, OpalTransportAddress::HostOnly);
    else {
      // No address specified, assume same IP as the transport and use default port
      PIPSocket::Address ip;
      if (!connection.GetControlChannel().GetLocalAddress().GetIpAddress(ip)) {
        PTRACE(1, "H323T120\tOnly IPv4 supported");
        errorCode = H245_OpenLogicalChannelReject_cause::e_separateStackEstablishmentFailed;
        return PFalse;
      }
      listener = new OpalListenerTCP(endpoint, ip, OpalT120Protocol::DefaultTcpPort, PFalse);
    }

    if (!listener->Open(NULL)) {
      PTRACE(1, "H323T120\tCould not open listener");
      errorCode = H245_OpenLogicalChannelReject_cause::e_separateStackEstablishmentFailed;
      return PFalse;
    }

    PTRACE(2, "H323T120\tCreated listener on " << listener->GetLocalAddress());
  }
  else {
    if (address.IsEmpty()) {
      // No address specified, assume same IP as the transport and use default port
      PIPSocket::Address ip;
      if (!connection.GetControlChannel().GetRemoteAddress().GetIpAddress(ip)) {
        PTRACE(1, "H323T120\tOnly IPv4 supported");
        errorCode = H245_OpenLogicalChannelReject_cause::e_separateStackEstablishmentFailed;
        return PFalse;
      }
      address = OpalTransportAddress(ip, OpalT120Protocol::DefaultTcpPort);
    }

    transport = address.CreateTransport(endpoint, OpalTransportAddress::FullTSAP);
    if (transport == NULL) {
      PTRACE(1, "H323T120\tCould not create transport");
      errorCode = H245_OpenLogicalChannelReject_cause::e_separateStackEstablishmentFailed;
      return PFalse;
    }

    transport->SetReadTimeout(10000); // 10 second wait for connect
    if (!transport->ConnectTo(address)) {
      PTRACE(1, "H323T120\tCould not connect to remote address: " << address);
      errorCode = H245_OpenLogicalChannelReject_cause::e_separateStackEstablishmentFailed;
      return PFalse;
    }

    PTRACE(2, "H323T120\tCreated transport from "
           << transport->GetLocalAddress() << " to " << transport->GetRemoteAddress());
  }

  return PTrue;
}


PBoolean H323_T120Channel::OnReceivedAckPDU(const H245_OpenLogicalChannelAck & /*ack*/)
{
  PTRACE(3, "H323T120\tOnReceivedAckPDU");

  t120handler = connection.CreateT120ProtocolHandler();
  if (t120handler == NULL) {
    PTRACE(1, "H323T120\tCould not create protocol handler");
    return PFalse;
  }

  return PTrue;
}

#endif // OPAL_T120DATA

/////////////////////////////////////////////////////////////////////////////
