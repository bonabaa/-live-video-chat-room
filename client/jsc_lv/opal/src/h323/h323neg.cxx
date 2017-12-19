/*
 * h323neg.cxx
 *
 * H.323 PDU definitions
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
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23567 $
 * $Author: rjongbloed $
 * $Date: 2009-10-01 06:20:36 +0000 (Thu, 01 Oct 2009) $
 */

#include <ptlib.h>

#include <opal/buildopts.h>
#if OPAL_H323

#ifdef __GNUC__
#pragma implementation "h323neg.h"
#endif

#include <h323/h323neg.h>

#include <ptclib/random.h>
#include <h323/h323ep.h>


#define new PNEW


///////////////////////////////////////////////////////////////////////////////

H245Negotiator::H245Negotiator(H323EndPoint & end, H323Connection & conn)
  : endpoint(end),
    connection(conn)
{
  replyTimer.SetNotifier(PCREATE_NOTIFIER(HandleTimeout));
}


void H245Negotiator::HandleTimeout(PTimer &, INT)
{
}


/////////////////////////////////////////////////////////////////////////////

H245NegMasterSlaveDetermination::H245NegMasterSlaveDetermination(H323EndPoint & end,
                                                                 H323Connection & conn)
  : H245Negotiator(end, conn)
{
  retryCount = 1;
  state = e_Idle;
  status = e_Indeterminate;
}


PBoolean H245NegMasterSlaveDetermination::Start(PBoolean renegotiate)
{
  PWaitAndSignal wait(mutex);

  if (state != e_Idle) {
    PTRACE(3, "H245\tMasterSlaveDetermination already in progress");
    return PTrue;
  }

  if (!renegotiate && IsDetermined())
    return PTrue;

  retryCount = 1;
  return Restart();
}


PBoolean H245NegMasterSlaveDetermination::Restart()
{
  PTRACE(3, "H245\tSending MasterSlaveDetermination");

  // Begin the Master/Slave determination procedure
  determinationNumber = PRandom::Number()%16777216;
  replyTimer = endpoint.GetMasterSlaveDeterminationTimeout();
  state = e_Outgoing;

  H323ControlPDU pdu;
  pdu.BuildMasterSlaveDetermination(endpoint.GetTerminalType(), determinationNumber);
  return connection.WriteControlPDU(pdu);
}


void H245NegMasterSlaveDetermination::Stop()
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tStopping MasterSlaveDetermination: state=" << state);

  if (state == e_Idle)
    return;

  replyTimer.Stop(false);
  state = e_Idle;
}


PBoolean H245NegMasterSlaveDetermination::HandleIncoming(const H245_MasterSlaveDetermination & pdu)
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tReceived MasterSlaveDetermination: state=" << state);

  if (state == e_Incoming) {
    replyTimer.Stop(false);
    state = e_Idle;
    return connection.OnControlProtocolError(H323Connection::e_MasterSlaveDetermination,
                                             "Duplicate MasterSlaveDetermination");
  }

  replyTimer = endpoint.GetMasterSlaveDeterminationTimeout();

  // Determine the master and slave
  MasterSlaveStatus newStatus;
  if (pdu.m_terminalType < (unsigned)endpoint.GetTerminalType())
    newStatus = e_DeterminedMaster;
  else if (pdu.m_terminalType > (unsigned)endpoint.GetTerminalType())
    newStatus = e_DeterminedSlave;
  else {
    DWORD moduloDiff = (pdu.m_statusDeterminationNumber - determinationNumber)&0xffffff;
    if (moduloDiff == 0 || moduloDiff == 0x800000)
      newStatus = e_Indeterminate;
    else if (moduloDiff < 0x800000)
      newStatus = e_DeterminedMaster;
    else
      newStatus = e_DeterminedSlave;
  }

  H323ControlPDU reply;

  if (newStatus != e_Indeterminate) {
    PTRACE(3, "H245\tMasterSlaveDetermination: local is "
                  << (newStatus == e_DeterminedMaster ? "master" : "slave"));
    reply.BuildMasterSlaveDeterminationAck(newStatus == e_DeterminedMaster);
    state = state == e_Outgoing ? e_Incoming : e_Idle;
    status = newStatus;
  }
  else if (state == e_Outgoing) {
    retryCount++;
    if (retryCount < endpoint.GetMasterSlaveDeterminationRetries())
      return Restart(); // Try again

    replyTimer.Stop(false);
    state = e_Idle;
    return connection.OnControlProtocolError(H323Connection::e_MasterSlaveDetermination,
                                             "Retries exceeded");
  }
  else {
    reply.BuildMasterSlaveDeterminationReject(H245_MasterSlaveDeterminationReject_cause::e_identicalNumbers);
  }

  return connection.WriteControlPDU(reply);
}


PBoolean H245NegMasterSlaveDetermination::HandleAck(const H245_MasterSlaveDeterminationAck & pdu)
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tReceived MasterSlaveDeterminationAck: state=" << state);

  if (state == e_Idle)
    return PTrue;

  replyTimer = endpoint.GetMasterSlaveDeterminationTimeout();

  MasterSlaveStatus newStatus;
  if (pdu.m_decision.GetTag() == H245_MasterSlaveDeterminationAck_decision::e_master)
    newStatus = e_DeterminedMaster;
  else
    newStatus = e_DeterminedSlave;

  H323ControlPDU reply;
  if (state == e_Outgoing) {
    status = newStatus;
    PTRACE(3, "H245\tMasterSlaveDetermination: remote is "
                  << (newStatus == e_DeterminedSlave ? "master" : "slave"));
    reply.BuildMasterSlaveDeterminationAck(newStatus == e_DeterminedMaster);
    if (!connection.WriteControlPDU(reply))
      return PFalse;
  }

  replyTimer.Stop(false);
  state = e_Idle;

  if (status != newStatus)
    return connection.OnControlProtocolError(H323Connection::e_MasterSlaveDetermination,
                                             "Master/Slave mismatch");

  return PTrue;
}


PBoolean H245NegMasterSlaveDetermination::HandleReject(const H245_MasterSlaveDeterminationReject & pdu)
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tReceived MasterSlaveDeterminationReject: state=" << state);

  switch (state) {
    case e_Idle :
      return PTrue;

    case e_Outgoing :
      if (pdu.m_cause.GetTag() == H245_MasterSlaveDeterminationReject_cause::e_identicalNumbers) {
        retryCount++;
        if (retryCount < endpoint.GetMasterSlaveDeterminationRetries())
          return Restart();
      }

    default :
      break;
  }

  replyTimer.Stop(false);
  state = e_Idle;

  return connection.OnControlProtocolError(H323Connection::e_MasterSlaveDetermination,
                                           "Retries exceeded");
}


PBoolean H245NegMasterSlaveDetermination::HandleRelease(const H245_MasterSlaveDeterminationRelease & /*pdu*/)
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tReceived MasterSlaveDeterminationRelease: state=" << state);

  if (state == e_Idle)
    return PTrue;

  replyTimer.Stop(false);
  state = e_Idle;

  return connection.OnControlProtocolError(H323Connection::e_MasterSlaveDetermination,
                                           "Aborted");
}


void H245NegMasterSlaveDetermination::HandleTimeout(PTimer &, INT)
{
  PWaitAndSignal wait(mutex);

  if (state == e_Idle)
    return;

  PTRACE(3, "H245\tTimeout on MasterSlaveDetermination: state=" << state);

  if (state == e_Outgoing) {
    H323ControlPDU reply;
    reply.Build(H245_IndicationMessage::e_masterSlaveDeterminationRelease);
    connection.WriteControlPDU(reply);
  }

  state = e_Idle;

  connection.OnControlProtocolError(H323Connection::e_MasterSlaveDetermination,
                                    "Timeout");
}


#if PTRACING
const char * H245NegMasterSlaveDetermination::GetStateName(States s)
{
  static const char * const names[] = {
    "Idle", "Outgoing", "Incoming"
  };
  return s < PARRAYSIZE(names) ? names[s] : "<Unknown>";
}


const char * H245NegMasterSlaveDetermination::GetStatusName(MasterSlaveStatus s)
{
  static const char * const names[] = {
    "Indeterminate", "DeterminedMaster", "DeterminedSlave"
  };
  return s < PARRAYSIZE(names) ? names[s] : "<Unknown>";
}
#endif


/////////////////////////////////////////////////////////////////////////////

H245NegTerminalCapabilitySet::H245NegTerminalCapabilitySet(H323EndPoint & end,
                                                           H323Connection & conn)
  : H245Negotiator(end, conn)
{
  inSequenceNumber = UINT_MAX;
  outSequenceNumber = 0;
  state = e_Idle;
  receivedCapabilites = PFalse;
}


PBoolean H245NegTerminalCapabilitySet::Start(PBoolean renegotiate, PBoolean empty)
{
  PWaitAndSignal wait(mutex);

  if (state == e_InProgress) {
    PTRACE(2, "H245\tTerminalCapabilitySet already in progress: outSeq=" << outSequenceNumber);
    return PTrue;
  }

  if (!renegotiate && state == e_Confirmed) {
    PTRACE(2, "H245\tTerminalCapabilitySet already sent.");
    return PTrue;
  }

  // Begin the capability exchange procedure
  outSequenceNumber = (outSequenceNumber+1)%256;
  replyTimer = endpoint.GetCapabilityExchangeTimeout();
  state = e_InProgress;

  PTRACE(3, "H245\tSending TerminalCapabilitySet: outSeq=" << outSequenceNumber);

  H323ControlPDU pdu;
  connection.OnSendCapabilitySet(pdu.BuildTerminalCapabilitySet(connection, outSequenceNumber, empty));
  return connection.WriteControlPDU(pdu);
}


void H245NegTerminalCapabilitySet::Stop(PBoolean dec)
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tStopping TerminalCapabilitySet: state=" << state);

  if (state == e_Idle)
    return;

  replyTimer.Stop(false);
  state = e_Idle;
  receivedCapabilites = PFalse;

  if (dec) {
    if (outSequenceNumber == 0)
      outSequenceNumber = 255;
    else
      --outSequenceNumber;
  }
}


PBoolean H245NegTerminalCapabilitySet::HandleIncoming(const H245_TerminalCapabilitySet & pdu)
{
  mutex.Wait();

  PTRACE(3, "H245\tReceived TerminalCapabilitySet:"
                     " state=" << state <<
                     " pduSeq=" << pdu.m_sequenceNumber <<
                     " inSeq=" << inSequenceNumber);

  if (pdu.m_sequenceNumber == inSequenceNumber) {
    mutex.Signal();
    PTRACE(2, "H245\tIgnoring TerminalCapabilitySet, already received sequence number");
    return PTrue;  // Already had this one
  }

  inSequenceNumber = pdu.m_sequenceNumber;

  mutex.Signal();

  H323Capabilities remoteCapabilities(connection, pdu);

  const H245_MultiplexCapability * muxCap = NULL;
  if (pdu.HasOptionalField(H245_TerminalCapabilitySet::e_multiplexCapability))
    muxCap = &pdu.m_multiplexCapability;

  H323ControlPDU reject;
  if (connection.OnReceivedCapabilitySet(remoteCapabilities, muxCap,
                    reject.BuildTerminalCapabilitySetReject(inSequenceNumber,
                            H245_TerminalCapabilitySetReject_cause::e_unspecified))) {
    receivedCapabilites = PTrue;
    H323ControlPDU ack;
    ack.BuildTerminalCapabilitySetAck(inSequenceNumber);
    return connection.WriteControlPDU(ack);
  }

  connection.WriteControlPDU(reject);
  connection.ClearCall(H323Connection::EndedByCapabilityExchange);
  return PTrue;
}


PBoolean H245NegTerminalCapabilitySet::HandleAck(const H245_TerminalCapabilitySetAck & pdu)
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tReceived TerminalCapabilitySetAck:"
                     " state=" << state <<
                     " pduSeq=" << pdu.m_sequenceNumber <<
                     " outSeq=" << (unsigned)outSequenceNumber);

  if (state != e_InProgress)
    return PTrue;

  if (pdu.m_sequenceNumber != outSequenceNumber)
    return PTrue;

  replyTimer.Stop(false);
  state = e_Confirmed;
  PTRACE(3, "H245\tTerminalCapabilitySet Sent.");
  return PTrue;
}


PBoolean H245NegTerminalCapabilitySet::HandleReject(const H245_TerminalCapabilitySetReject & pdu)
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tReceived TerminalCapabilitySetReject:"
                     " state=" << state <<
                     " pduSeq=" << pdu.m_sequenceNumber <<
                     " outSeq=" << (unsigned)outSequenceNumber);

  if (state != e_InProgress)
    return PTrue;

  if (pdu.m_sequenceNumber != outSequenceNumber)
    return PTrue;

  state = e_Idle;
  replyTimer.Stop(false);
  return connection.OnControlProtocolError(H323Connection::e_CapabilityExchange,
                                           "Rejected");
}


PBoolean H245NegTerminalCapabilitySet::HandleRelease(const H245_TerminalCapabilitySetRelease & /*pdu*/)
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tReceived TerminalCapabilityRelease: state=" << state);

  receivedCapabilites = PFalse;
  return connection.OnControlProtocolError(H323Connection::e_CapabilityExchange,
                                           "Aborted");
}


void H245NegTerminalCapabilitySet::HandleTimeout(PTimer &, INT)
{
  PWaitAndSignal wait(mutex);

  if (state == e_Idle)
    return;

  PTRACE(3, "H245\tTimeout on TerminalCapabilitySet: state=" << state);

  H323ControlPDU reply;
  reply.Build(H245_IndicationMessage::e_terminalCapabilitySetRelease);
  connection.WriteControlPDU(reply);

  connection.OnControlProtocolError(H323Connection::e_CapabilityExchange, "Timeout");
}


#if PTRACING
const char * H245NegTerminalCapabilitySet::GetStateName(States s)
{
  static const char * const names[] = {
    "Idle", "InProgress", "Sent"
  };
  return s < PARRAYSIZE(names) ? names[s] : "<Unknown>";
}
#endif


/////////////////////////////////////////////////////////////////////////////

H245NegLogicalChannel::H245NegLogicalChannel(H323EndPoint & end,
                                             H323Connection & conn,
                                             const H323ChannelNumber & chanNum)
  : H245Negotiator(end, conn),
    channelNumber(chanNum)
{
  channel = NULL;
  state = e_Released;
}


H245NegLogicalChannel::H245NegLogicalChannel(H323EndPoint & end,
                                             H323Connection & conn,
                                             H323Channel & chan)
  : H245Negotiator(end, conn),
    channelNumber(chan.GetNumber())
{
  channel = &chan;
  state = e_Established;
}


H245NegLogicalChannel::~H245NegLogicalChannel()
{
  replyTimer.Stop();
  PThread::Yield(); // Do this to avoid possible race condition with timer

  mutex.Wait();
  delete channel;
  mutex.Signal();
}


PBoolean H245NegLogicalChannel::Open(const H323Capability & capability,
                                 unsigned sessionID,
                                 unsigned replacementFor)
{
  PWaitAndSignal wait(mutex);
  return OpenWhileLocked(capability, sessionID, replacementFor);
}


PBoolean H245NegLogicalChannel::OpenWhileLocked(const H323Capability & capability,
                                            unsigned sessionID,
                                            unsigned replacementFor)
{
  if (state != e_Released && state != e_AwaitingRelease) {
    PTRACE(2, "H245\tOpen of channel currently in negotiations: " << channelNumber);
    return PFalse;
  }

  PTRACE(3, "H245\tOpening channel: " << channelNumber);

  if (channel != NULL) {
    channel->Close();
    delete channel;
    channel = NULL;
  }

  state = e_AwaitingEstablishment;

  H323ControlPDU pdu;
  H245_OpenLogicalChannel & open = pdu.BuildOpenLogicalChannel(channelNumber);

  if (!capability.OnSendingPDU(open.m_forwardLogicalChannelParameters.m_dataType)) {
    PTRACE(1, "H245\tOpening channel: " << channelNumber
           << ", capability.OnSendingPDU() failed");
    return PFalse;
  }

  channel = capability.CreateChannel(connection, H323Channel::IsTransmitter, sessionID, NULL);
  if (channel == NULL) {
    PTRACE(1, "H245\tOpening channel: " << channelNumber
           << ", capability.CreateChannel() failed");
    return PFalse;
  }

  channel->SetNumber(channelNumber);

  if (!channel->OnSendingPDU(open)) {
    PTRACE(1, "H245\tOpening channel: " << channelNumber
           << ", channel->OnSendingPDU() failed");
    return PFalse;
  }

  if (replacementFor > 0) {
    if (open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters)) {
      open.m_reverseLogicalChannelParameters.IncludeOptionalField(H245_OpenLogicalChannel_reverseLogicalChannelParameters::e_replacementFor);
      open.m_reverseLogicalChannelParameters.m_replacementFor = replacementFor;
    }
    else {
      open.m_forwardLogicalChannelParameters.IncludeOptionalField(H245_OpenLogicalChannel_forwardLogicalChannelParameters::e_replacementFor);
      open.m_forwardLogicalChannelParameters.m_replacementFor = replacementFor;
    }
  }

  if (!channel->Open())
    return PFalse;

  if (!channel->SetInitialBandwidth()) {
    PTRACE(2, "H245\tOpening channel: " << channelNumber << ", Insufficient bandwidth");
    return PFalse;
  }

  replyTimer = endpoint.GetLogicalChannelTimeout();

  return connection.WriteControlPDU(pdu);
}


PBoolean H245NegLogicalChannel::Close()
{
  PWaitAndSignal wait(mutex);
  return CloseWhileLocked();
}


PBoolean H245NegLogicalChannel::CloseWhileLocked()
{
  PTRACE(3, "H245\tClosing channel: " << channelNumber << ", state=" << state);

  if (state != e_AwaitingEstablishment && state != e_Established)
    return PTrue;

  replyTimer = endpoint.GetLogicalChannelTimeout();

  H323ControlPDU reply;

  if (channelNumber.IsFromRemote()) {
    reply.BuildRequestChannelClose(channelNumber, H245_RequestChannelClose_reason::e_normal);
    state = e_AwaitingResponse;
  }
  else {
    reply.BuildCloseLogicalChannel(channelNumber);
    state = e_AwaitingRelease;
    if (channel != NULL)
      channel->Close();
  }

  return connection.WriteControlPDU(reply);
}


PBoolean H245NegLogicalChannel::HandleOpen(const H245_OpenLogicalChannel & pdu)
{
  PTRACE(3, "H245\tReceived open channel: " << channelNumber << ", state=" << state);

  if (channel != NULL) {
    channel->Close();
    delete channel;
    channel = NULL;
  }

  state = e_AwaitingEstablishment;

  H323ControlPDU reply;
  H245_OpenLogicalChannelAck & ack = reply.BuildOpenLogicalChannelAck(channelNumber);

  PBoolean ok = PFalse;

  unsigned cause = H245_OpenLogicalChannelReject_cause::e_unspecified;
  if (connection.OnOpenLogicalChannel(pdu, ack, cause))
    channel = connection.CreateLogicalChannel(pdu, PFalse, cause);

  if (channel != NULL) {
    channel->SetNumber(channelNumber);
    channel->OnSendOpenAck(pdu, ack);
    if (channel->GetDirection() == H323Channel::IsBidirectional) {
      state = e_AwaitingConfirmation;
      replyTimer = endpoint.GetLogicalChannelTimeout(); // T103
      ok = PTrue;
    }
    else {
      ok = channel->Start();
      if (!ok) {
        // The correct protocol thing to do is reject the channel if we are
        // the master. However NetMeeting will not then reopen a channel, so
        // we act like we are a slave and close our end instead.
        if (connection.IsH245Master() &&
            connection.GetRemoteApplication().Find("NetMeeting") == P_MAX_INDEX)
          cause = H245_OpenLogicalChannelReject_cause::e_masterSlaveConflict;
        else {
          connection.OnConflictingLogicalChannel(*channel);
          ok = channel->Start();
        }
      }

      if (ok)
        state = e_Established;
    }
  }

  if (ok)
    mutex.Signal();
  else {
    reply.BuildOpenLogicalChannelReject(channelNumber, cause);
    Release();
  }

  return connection.WriteControlPDU(reply);
}


PBoolean H245NegLogicalChannel::HandleOpenAck(const H245_OpenLogicalChannelAck & pdu)
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tReceived open channel ack: " << channelNumber << ", state=" << state);

  switch (state) {
    case e_Released :
      return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                               "Ack unknown channel");
    case e_AwaitingEstablishment :
      state = e_Established;
      replyTimer.Stop(false);

      if (!channel->OnReceivedAckPDU(pdu)) {
        if (connection.GetRemoteProductInfo().name != "Cisco IOS")
          return CloseWhileLocked();
        PTRACE(4, "H245\tWorkaround for Cisco bug, cannot close channel on illegal ack or it hangs up on you.");
        return true;
      }

      if (channel->GetDirection() == H323Channel::IsBidirectional) {
        H323ControlPDU reply;
        reply.BuildOpenLogicalChannelConfirm(channelNumber);
        if (!connection.WriteControlPDU(reply))
          return PFalse;
      }

      // Channel was already opened when OLC sent, if have error here it is
      // somthing other than an asymmetric codec conflict, so close it down.
      if (!channel->Start())
        return CloseWhileLocked();

    default :
      break;
  }

  return PTrue;
}


PBoolean H245NegLogicalChannel::HandleOpenConfirm(const H245_OpenLogicalChannelConfirm & /*pdu*/)
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tReceived open channel confirm: " << channelNumber << ", state=" << state);

  switch (state) {
    case e_Released :
      return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                               "Confirm unknown channel");
    case e_AwaitingEstablishment :
      return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                               "Confirm established channel");
    case e_AwaitingConfirmation :
      replyTimer.Stop(false);
      state = e_Established;
      // Channel was already opened when OLC sent, if have error here it is
      // somthing other than an asymmetric codec conflict, so close it down.
      if (!channel->Start())
        return CloseWhileLocked();

    default :
      break;
  }

  return PTrue;
}


PBoolean H245NegLogicalChannel::HandleReject(const H245_OpenLogicalChannelReject & pdu)
{
  mutex.Wait();

  PTRACE(3, "H245\tReceived open channel reject: " << channelNumber << ", state=" << state);

  switch (state) {
    case e_Released :
      mutex.Signal();
      return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                               "Reject unknown channel");
    case e_Established :
      Release();
      return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                               "Reject established channel");
    case e_AwaitingEstablishment :
      // Master rejected our attempt to open, so try something else.
      if (pdu.m_cause.GetTag() == H245_OpenLogicalChannelReject_cause::e_masterSlaveConflict)
        connection.OnConflictingLogicalChannel(*channel);
      // Do next case

    case e_AwaitingRelease :
      Release();
      break;

    default :
      mutex.Signal();
      break;
  }

  return PTrue;
}


PBoolean H245NegLogicalChannel::HandleClose(const H245_CloseLogicalChannel & /*pdu*/)
{
  mutex.Wait();

  PTRACE(3, "H245\tReceived close channel: " << channelNumber << ", state=" << state);

  //if (pdu.m_source.GetTag() == H245_CloseLogicalChannel_source::e_user)

  H323ControlPDU reply;
  reply.BuildCloseLogicalChannelAck(channelNumber);

  Release();

  return connection.WriteControlPDU(reply);
}


PBoolean H245NegLogicalChannel::HandleCloseAck(const H245_CloseLogicalChannelAck & /*pdu*/)
{
  mutex.Wait();

  PTRACE(3, "H245\tReceived close channel ack: " << channelNumber << ", state=" << state);

  switch (state) {
    case e_Established :
      Release();
      return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                               "Close ack open channel");
    case e_AwaitingRelease :
      Release();
      break;

    default :
      mutex.Signal();
      break;
  }

  return PTrue;
}


PBoolean H245NegLogicalChannel::HandleRequestClose(const H245_RequestChannelClose & pdu)
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tReceived request close channel: " << channelNumber << ", state=" << state);

  if (state != e_Established)
    return PTrue;    // Already closed

  H323ControlPDU reply;
  if (connection.OnClosingLogicalChannel(*channel)) {
    reply.BuildRequestChannelCloseAck(channelNumber);
    if (!connection.WriteControlPDU(reply))
      return PFalse;

    // Do normal Close procedure
    replyTimer = endpoint.GetLogicalChannelTimeout();
    reply.BuildCloseLogicalChannel(channelNumber);
    state = e_AwaitingRelease;

    if (pdu.m_reason.GetTag() == H245_RequestChannelClose_reason::e_reopen) {
      PTRACE(2, "H245\tReopening channel: " << channelNumber);
      connection.OpenLogicalChannel(channel->GetCapability(),
                                    channel->GetSessionID(),
                                    channel->GetDirection());
    }
  }
  else
    reply.BuildRequestChannelCloseReject(channelNumber);

  return connection.WriteControlPDU(reply);
}


PBoolean H245NegLogicalChannel::HandleRequestCloseAck(const H245_RequestChannelCloseAck & /*pdu*/)
{
  mutex.Wait();

  PTRACE(3, "H245\tReceived request close ack channel: " << channelNumber << ", state=" << state);

  if (state == e_AwaitingResponse)
    Release();  // Other end says close OK, so do so.
  else
    mutex.Signal();

  return PTrue;
}


PBoolean H245NegLogicalChannel::HandleRequestCloseReject(const H245_RequestChannelCloseReject & /*pdu*/)
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tReceived request close reject channel: " << channelNumber << ", state=" << state);

  // Other end refused close, so go back to still having channel open
  if (state == e_AwaitingResponse)
    state = e_Established;

  return PTrue;
}


PBoolean H245NegLogicalChannel::HandleRequestCloseRelease(const H245_RequestChannelCloseRelease & /*pdu*/)
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tReceived request close release channel: " << channelNumber << ", state=" << state);

  // Other end refused close, so go back to still having channel open
  state = e_Established;

  return PTrue;
}


void H245NegLogicalChannel::HandleTimeout(PTimer &, INT)
{
  mutex.Wait();

  PTRACE(3, "H245\tTimeout on open channel: " << channelNumber << ", state=" << state);

  H323ControlPDU reply;
  switch (state) {
    case e_AwaitingEstablishment :
      reply.BuildCloseLogicalChannel(channelNumber);
      connection.WriteControlPDU(reply);
      break;

    case e_AwaitingResponse :
      reply.BuildRequestChannelCloseRelease(channelNumber);
      connection.WriteControlPDU(reply);
      break;

    case e_Released :
      mutex.Signal();
      return;

    default :
      break;
  }

  Release();
  connection.OnControlProtocolError(H323Connection::e_LogicalChannel, "Timeout");
}


void H245NegLogicalChannel::Release()
{
  state = e_Released;
  H323Channel * chan = channel;
  channel = NULL;
  mutex.Signal();

  replyTimer.Stop(false);

  if (chan != NULL) {
    chan->Close();
    delete chan;
  }
}


H323Channel * H245NegLogicalChannel::GetChannel()
{
  return channel;
}


#if PTRACING
const char * H245NegLogicalChannel::GetStateName(States s)
{
  static const char * const names[] = {
    "Released",
    "AwaitingEstablishment",
    "Established",
    "AwaitingRelease",
    "AwatingConfirmation",
    "AwaitingResponse"
  };
  return s < PARRAYSIZE(names) ? names[s] : "<Unknown>";
}
#endif


/////////////////////////////////////////////////////////////////////////////

H245NegLogicalChannels::H245NegLogicalChannels(H323EndPoint & end,
                                               H323Connection & conn)
  : H245Negotiator(end, conn),
    lastChannelNumber(100, PFalse)
{
}


void H245NegLogicalChannels::Add(H323Channel & channel)
{
  mutex.Wait();
  channels.SetAt(channel.GetNumber(), new H245NegLogicalChannel(endpoint, connection, channel));
  mutex.Signal();
}


PBoolean H245NegLogicalChannels::Open(const H323Capability & capability,
                                  unsigned sessionID,
                                  unsigned replacementFor)
{
  mutex.Wait();

  lastChannelNumber++;

  H245NegLogicalChannel * negChan = new H245NegLogicalChannel(endpoint, connection, lastChannelNumber);
  channels.SetAt(lastChannelNumber, negChan);

  mutex.Signal();

  return negChan->Open(capability, sessionID, replacementFor);
}


PBoolean H245NegLogicalChannels::Close(unsigned channelNumber, PBoolean fromRemote)
{
  H245NegLogicalChannel * chan = FindNegLogicalChannel(channelNumber, fromRemote);
  if (chan != NULL)
    return chan->Close();

  return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                           "Close unknown");
}


PBoolean H245NegLogicalChannels::HandleOpen(const H245_OpenLogicalChannel & pdu)
{
  H323ChannelNumber chanNum(pdu.m_forwardLogicalChannelNumber, PTrue);
  H245NegLogicalChannel * chan;

  mutex.Wait();

  if (channels.Contains(chanNum))
    chan = &channels[chanNum];
  else {
    chan = new H245NegLogicalChannel(endpoint, connection, chanNum);
    channels.SetAt(chanNum, chan);
  }

  chan->mutex.Wait();

  mutex.Signal();

  return chan->HandleOpen(pdu);
}


PBoolean H245NegLogicalChannels::HandleOpenAck(const H245_OpenLogicalChannelAck & pdu)
{
  H245NegLogicalChannel * chan = FindNegLogicalChannel(pdu.m_forwardLogicalChannelNumber, PFalse);
  if (chan != NULL)
    return chan->HandleOpenAck(pdu);

  return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                           "Ack unknown");
}


PBoolean H245NegLogicalChannels::HandleOpenConfirm(const H245_OpenLogicalChannelConfirm & pdu)
{
  H245NegLogicalChannel * chan = FindNegLogicalChannel(pdu.m_forwardLogicalChannelNumber, PTrue);
  if (chan != NULL)
    return chan->HandleOpenConfirm(pdu);

  return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                           "Confirm unknown");
}


PBoolean H245NegLogicalChannels::HandleReject(const H245_OpenLogicalChannelReject & pdu)
{
  H245NegLogicalChannel * chan = FindNegLogicalChannel(pdu.m_forwardLogicalChannelNumber, PFalse);
  if (chan != NULL)
    return chan->HandleReject(pdu);

  return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                           "Reject unknown");
}


PBoolean H245NegLogicalChannels::HandleClose(const H245_CloseLogicalChannel & pdu)
{
  H245NegLogicalChannel * chan = FindNegLogicalChannel(pdu.m_forwardLogicalChannelNumber, PTrue);
  if (chan != NULL)
    return chan->HandleClose(pdu);

  return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                           "Close unknown");
}


PBoolean H245NegLogicalChannels::HandleCloseAck(const H245_CloseLogicalChannelAck & pdu)
{
  H245NegLogicalChannel * chan = FindNegLogicalChannel(pdu.m_forwardLogicalChannelNumber, PFalse);
  if (chan != NULL)
    return chan->HandleCloseAck(pdu);

  return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                           "Close Ack unknown");
}


PBoolean H245NegLogicalChannels::HandleRequestClose(const H245_RequestChannelClose & pdu)
{
  H245NegLogicalChannel * chan = FindNegLogicalChannel(pdu.m_forwardLogicalChannelNumber, PFalse);
  if (chan != NULL)
    return chan->HandleRequestClose(pdu);

  return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                           "Request Close unknown");
}


PBoolean H245NegLogicalChannels::HandleRequestCloseAck(const H245_RequestChannelCloseAck & pdu)
{
  H245NegLogicalChannel * chan = FindNegLogicalChannel(pdu.m_forwardLogicalChannelNumber, PTrue);
  if (chan != NULL)
    return chan->HandleRequestCloseAck(pdu);

  return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                           "Request Close Ack unknown");
}


PBoolean H245NegLogicalChannels::HandleRequestCloseReject(const H245_RequestChannelCloseReject & pdu)
{
  H245NegLogicalChannel * chan = FindNegLogicalChannel(pdu.m_forwardLogicalChannelNumber, PTrue);
  if (chan != NULL)
    return chan->HandleRequestCloseReject(pdu);

  return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                           "Request Close Reject unknown");
}


PBoolean H245NegLogicalChannels::HandleRequestCloseRelease(const H245_RequestChannelCloseRelease & pdu)
{
  H245NegLogicalChannel * chan = FindNegLogicalChannel(pdu.m_forwardLogicalChannelNumber, PFalse);
  if (chan != NULL)
    return chan->HandleRequestCloseRelease(pdu);

  return connection.OnControlProtocolError(H323Connection::e_LogicalChannel,
                                           "Request Close Release unknown");
}


H323ChannelNumber H245NegLogicalChannels::GetNextChannelNumber()
{
  PWaitAndSignal wait(mutex);
  lastChannelNumber++;
  return lastChannelNumber;
}


H323Channel * H245NegLogicalChannels::GetChannelAt(PINDEX i)
{
  mutex.Wait();
  H323Channel * chan =  channels.GetDataAt(i).GetChannel();
  mutex.Signal();
  return chan;
}


H323Channel * H245NegLogicalChannels::FindChannel(unsigned channelNumber,
                                                  PBoolean fromRemote)
{
  PWaitAndSignal wait(mutex);

  H323ChannelNumber chanNum(channelNumber, fromRemote);

  if (channels.Contains(chanNum))
    return channels[chanNum].GetChannel();

  return NULL;
}


H245NegLogicalChannel & H245NegLogicalChannels::GetNegLogicalChannelAt(PINDEX i)
{
  PWaitAndSignal wait(mutex);
  return channels.GetDataAt(i);
}


H245NegLogicalChannel * H245NegLogicalChannels::FindNegLogicalChannel(unsigned channelNumber,
                                                                      PBoolean fromRemote)
{
  H323ChannelNumber chanNum(channelNumber, fromRemote);

  mutex.Wait();
  H245NegLogicalChannel * channel = channels.GetAt(chanNum);
  mutex.Signal();

  return channel;
}


H323Channel * H245NegLogicalChannels::FindChannelBySession(unsigned rtpSessionId,
                                                           PBoolean fromRemote)
{
  PWaitAndSignal wait(mutex);

  PINDEX i;
  H323Channel::Directions desiredDirection = fromRemote ? H323Channel::IsReceiver : H323Channel::IsTransmitter;
  for (i = 0; i < GetSize(); i++) {
    H245NegLogicalChannel & logChan = channels.GetDataAt(i);
    if (logChan.IsAwaitingEstablishment() || logChan.IsEstablished()) {
      H323Channel * channel = logChan.GetChannel();
      if (channel != NULL && channel->GetSessionID() == rtpSessionId &&
                             channel->GetDirection() == desiredDirection)
        return channel;
    }
  }

  return NULL;
}


void H245NegLogicalChannels::RemoveAll()
{
  PWaitAndSignal wait(mutex);

  for (PINDEX i = 0; i < channels.GetSize(); i++) {
    H245NegLogicalChannel & neg = channels.GetDataAt(i);
    neg.mutex.Wait();
    H323Channel * channel = neg.GetChannel();
    if (channel != NULL)
      channel->Close();
    neg.mutex.Signal();
  }

  channels.RemoveAll();
}


/////////////////////////////////////////////////////////////////////////////

H245NegRequestMode::H245NegRequestMode(H323EndPoint & end, H323Connection & conn)
  : H245Negotiator(end, conn)
{
  awaitingResponse = PFalse;
  inSequenceNumber = UINT_MAX;
  outSequenceNumber = 0;
}


PBoolean H245NegRequestMode::StartRequest(const PString & newModes)
{
  PStringArray modes = newModes.Lines();
  if (modes.IsEmpty()) {
    PTRACE(2, "H245\tNo new mode to request");
    return false;
  }

  H245_ArrayOf_ModeDescription descriptions;
  PINDEX modeCount = 0;

  const H323Capabilities & localCapabilities = connection.GetLocalCapabilities();

  for (PINDEX i = 0; i < modes.GetSize(); i++) {
    H245_ModeDescription description;
    PINDEX count = 0;

    PStringArray caps = modes[i].Tokenise('\t');
    for (PINDEX j = 0; j < caps.GetSize(); j++) {
      H323Capability * capability = localCapabilities.FindCapability(caps[j]);
      if (capability != NULL) {
        description.SetSize(count+1);
        capability->OnSendingPDU(description[count]);
        count++;
      }
    }

    if (count > 0) {
      descriptions.SetSize(modeCount+1);
      descriptions[modeCount] = description;
      modeCount++;
    }
  }

  if (modeCount == 0) {
    PTRACE(2, "H245\tUnsupported new mode to request");
    return false;
  }

  return StartRequest(descriptions);
}


PBoolean H245NegRequestMode::StartRequest(const H245_ArrayOf_ModeDescription & newModes)
{
  PTRACE(3, "H245\tStarted request mode: outSeq=" << outSequenceNumber
         << (awaitingResponse ? " awaitingResponse" : " idle"));

  if (awaitingResponse) {
    PTRACE(2, "H245\tAwaiting response to previous mode request");
    return false;
  }

  // Initiate a mode request
  outSequenceNumber = (outSequenceNumber+1)%256;
  replyTimer = endpoint.GetRequestModeTimeout();
  awaitingResponse = PTrue;

  H323ControlPDU pdu;
  H245_RequestMode & requestMode = pdu.BuildRequestMode(outSequenceNumber);
  requestMode.m_requestedModes = newModes;
  requestMode.m_requestedModes.SetConstraints(PASN_Object::FixedConstraint, 1, 256);

  return connection.WriteControlPDU(pdu);
}


PBoolean H245NegRequestMode::HandleRequest(const H245_RequestMode & pdu)
{
  inSequenceNumber = pdu.m_sequenceNumber;

  PTRACE(3, "H245\tReceived request mode: inSeq=" << inSequenceNumber);

  H323ControlPDU reply_ack;
  H245_RequestModeAck & ack = reply_ack.BuildRequestModeAck(inSequenceNumber,
                  H245_RequestModeAck_response::e_willTransmitMostPreferredMode);

  H323ControlPDU reply_reject;
  H245_RequestModeReject & reject = reply_reject.BuildRequestModeReject(inSequenceNumber,
                                        H245_RequestModeReject_cause::e_modeUnavailable);

  PINDEX selectedMode = 0;
  if (!connection.OnRequestModeChange(pdu, ack, reject, selectedMode))
    return connection.WriteControlPDU(reply_reject);

  if (selectedMode != 0)
    ack.m_response.SetTag(H245_RequestModeAck_response::e_willTransmitLessPreferredMode);

  if (!connection.WriteControlPDU(reply_ack))
    return PFalse;

  connection.OnModeChanged(pdu.m_requestedModes[selectedMode]);
  return PTrue;
}


PBoolean H245NegRequestMode::HandleAck(const H245_RequestModeAck & pdu)
{
  PTRACE(3, "H245\tReceived ack on request mode: outSeq=" << outSequenceNumber
         << (awaitingResponse ? " awaitingResponse" : " idle"));

  if (awaitingResponse && pdu.m_sequenceNumber == outSequenceNumber) {
    awaitingResponse = PFalse;
    replyTimer.Stop(false);
    connection.OnAcceptModeChange(pdu);
  }

  return PTrue;
}

PBoolean H245NegRequestMode::HandleReject(const H245_RequestModeReject & pdu)
{
  PTRACE(3, "H245\tReceived reject on request mode: outSeq=" << outSequenceNumber
         << (awaitingResponse ? " awaitingResponse" : " idle"));

  if (awaitingResponse && pdu.m_sequenceNumber == outSequenceNumber) {
    awaitingResponse = PFalse;
    replyTimer.Stop(false);
    connection.OnRefusedModeChange(&pdu);
  }

  return PTrue;
}


PBoolean H245NegRequestMode::HandleRelease(const H245_RequestModeRelease & /*pdu*/)
{
  PTRACE(3, "H245\tReceived release on request mode: inSeq=" << inSequenceNumber);
  return PTrue;
}


void H245NegRequestMode::HandleTimeout(PTimer &, INT)
{
  PTRACE(3, "H245\tTimeout on request mode: outSeq=" << outSequenceNumber
         << (awaitingResponse ? " awaitingResponse" : " idle"));

  if (awaitingResponse) {
    awaitingResponse = PFalse;
    H323ControlPDU pdu;
    pdu.Build(H245_IndicationMessage::e_requestModeRelease);
    connection.WriteControlPDU(pdu);
    connection.OnRefusedModeChange(NULL);
    connection.OnControlProtocolError(H323Connection::e_ModeRequest, "Timeout");
  }
}


/////////////////////////////////////////////////////////////////////////////

H245NegRoundTripDelay::H245NegRoundTripDelay(H323EndPoint & end, H323Connection & conn)
  : H245Negotiator(end, conn)
{
  awaitingResponse = PFalse;
  sequenceNumber = 0;

  // Temporary (ie quick) fix for strange Cisco behaviour. If keep trying to
  // do this it stops sending RTP audio data!!
  retryCount = 1;
}


PBoolean H245NegRoundTripDelay::StartRequest()
{
  PWaitAndSignal wait(mutex);

  replyTimer = endpoint.GetRoundTripDelayTimeout();
  sequenceNumber = (sequenceNumber + 1)%256;
  awaitingResponse = PTrue;

  PTRACE(3, "H245\tStarted round trip delay: seq=" << sequenceNumber
         << (awaitingResponse ? " awaitingResponse" : " idle"));

  H323ControlPDU pdu;
  pdu.BuildRoundTripDelayRequest(sequenceNumber);
  if (!connection.WriteControlPDU(pdu))
    return PFalse;

  tripStartTime = PTimer::Tick();
  return PTrue;
}


PBoolean H245NegRoundTripDelay::HandleRequest(const H245_RoundTripDelayRequest & pdu)
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tStarted round trip delay: seq=" << sequenceNumber
         << (awaitingResponse ? " awaitingResponse" : " idle"));

  H323ControlPDU reply;
  reply.BuildRoundTripDelayResponse(pdu.m_sequenceNumber);
  return connection.WriteControlPDU(reply);
}


PBoolean H245NegRoundTripDelay::HandleResponse(const H245_RoundTripDelayResponse & pdu)
{
  PWaitAndSignal wait(mutex);

  PTimeInterval tripEndTime = PTimer::Tick();

  PTRACE(3, "H245\tHandling round trip delay: seq=" << sequenceNumber
         << (awaitingResponse ? " awaitingResponse" : " idle"));

  if (awaitingResponse && pdu.m_sequenceNumber == sequenceNumber) {
    replyTimer.Stop(false);
    awaitingResponse = PFalse;
    roundTripTime = tripEndTime - tripStartTime;
    retryCount = 3;
  }

  return PTrue;
}


void H245NegRoundTripDelay::HandleTimeout(PTimer &, INT)
{
  PWaitAndSignal wait(mutex);

  PTRACE(3, "H245\tTimeout on round trip delay: seq=" << sequenceNumber
         << (awaitingResponse ? " awaitingResponse" : " idle"));

  if (awaitingResponse && retryCount > 0)
    retryCount--;
  awaitingResponse = PFalse;

  connection.OnControlProtocolError(H323Connection::e_RoundTripDelay, "Timeout");
}


#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////
