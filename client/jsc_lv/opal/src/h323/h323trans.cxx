/*
 * h323trans.cxx
 *
 * H.323 Transactor handler
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
 * $Revision: 22170 $
 * $Author: rjongbloed $
 * $Date: 2009-03-09 02:48:51 +0000 (Mon, 09 Mar 2009) $
 */

#include <ptlib.h>

#include <opal/buildopts.h>
#if OPAL_H323

#ifdef __GNUC__
#pragma implementation "h323trans.h"
#endif

#include <h323/h323trans.h>

#include <h323/h323ep.h>
#include <h323/h323pdu.h>

#include <ptclib/random.h>


static PTimeInterval ResponseRetirementAge(0, 30); // Seconds


#define new PNEW


static struct AuthenticatorString {
  int code;
  const char * desc;
} authenticatorStrings[] = {
  { H235Authenticator::e_OK,          "Security parameters and Msg are ok, no security attacks" },
  { H235Authenticator::e_Absent,      "Security parameters are expected but absent" },
  { H235Authenticator::e_Error,       "Security parameters are present but incorrect" },
  { H235Authenticator::e_InvalidTime, "Security parameters indicate peer has bad real time clock" },
  { H235Authenticator::e_BadPassword, "Security parameters indicate bad password in token" },
  { H235Authenticator::e_ReplyAttack, "Security parameters indicate an attack was made" },
  { H235Authenticator::e_Disabled,    "Security is disabled by local system" },
  { -1, NULL }
};

/////////////////////////////////////////////////////////////////////////////////

H323TransactionPDU::H323TransactionPDU()
{
}


H323TransactionPDU::H323TransactionPDU(const H235Authenticators & auth)
  : authenticators(auth)
{
}


ostream & operator<<(ostream & strm, const H323TransactionPDU & pdu)
{
  pdu.GetPDU().PrintOn(strm);
  return strm;
}


PBoolean H323TransactionPDU::Read(H323Transport & transport)
{
  if (!transport.ReadPDU(rawPDU)) {
    PTRACE(1, GetProtocolName() << "\tRead error ("
           << transport.GetErrorNumber(PChannel::LastReadError)
           << "): " << transport.GetErrorText(PChannel::LastReadError));
    return PFalse;
  }

  rawPDU.ResetDecoder();
  PBoolean ok = GetPDU().Decode(rawPDU);
  if (!ok) {
    PTRACE(1, GetProtocolName() << "\tRead error: PER decode failure:\n  "
           << setprecision(2) << rawPDU << "\n "  << setprecision(2) << *this);
    GetChoice().SetTag(UINT_MAX);
    return PTrue;
  }

  H323TraceDumpPDU(GetProtocolName(), PFalse, rawPDU, GetPDU(), GetChoice(), GetSequenceNumber());

  return PTrue;
}


PBoolean H323TransactionPDU::Write(H323Transport & transport)
{
  PPER_Stream strm;
  GetPDU().Encode(strm);
  strm.CompleteEncoding();

  // Finalise the security if present
  for (H235Authenticators::iterator iterAuth = authenticators.begin(); iterAuth != authenticators.end(); ++iterAuth)
    iterAuth->Finalise(strm);

  H323TraceDumpPDU("Trans", PTrue, strm, GetPDU(), GetChoice(), GetSequenceNumber());

  if (transport.WritePDU(strm))
    return PTrue;

  PTRACE(1, GetProtocolName() << "\tWrite PDU failed ("
         << transport.GetErrorNumber(PChannel::LastWriteError)
         << "): " << transport.GetErrorText(PChannel::LastWriteError));
  return PFalse;
}


/////////////////////////////////////////////////////////////////////////////////

H323Transactor::H323Transactor(H323EndPoint & ep,
                               H323Transport * trans,
                               WORD local_port,
                               WORD remote_port)
  : endpoint(ep),
    defaultLocalPort(local_port),
    defaultRemotePort(remote_port)
{
  if (trans != NULL)
    transport = trans;
  else
    transport = new H323TransportUDP(ep, PIPSocket::GetDefaultIpAny(), local_port);

  Construct();
}


H323Transactor::H323Transactor(H323EndPoint & ep,
                               const H323TransportAddress & iface,
                               WORD local_port,
                               WORD remote_port)
  : endpoint(ep),
    defaultLocalPort(local_port),
    defaultRemotePort(remote_port)
{
  if (iface.IsEmpty())
    transport = NULL;
  else {
    PIPSocket::Address addr;
    PAssert(iface.GetIpAndPort(addr, local_port), "Cannot parse address");
    transport = new H323TransportUDP(ep, addr, local_port);
  }

  Construct();
}


void H323Transactor::Construct()
{
  nextSequenceNumber = PRandom::Number()%65536;
  checkResponseCryptoTokens = PTrue;
  lastRequest = NULL;

  requests.DisallowDeleteObjects();
}


H323Transactor::~H323Transactor()
{
  StopChannel();
}


void H323Transactor::PrintOn(ostream & strm) const
{
  if (transport == NULL)
    strm << "<<no-transport>>";
  else {
    H323TransportAddress addr = transport->GetRemoteAddress();

    PIPSocket::Address ip;
    WORD port;
    if (addr.GetIpAndPort(ip, port)) {
      strm << PIPSocket::GetHostName(ip);
      if (port != defaultRemotePort)
        strm << ':' << port;
    }
    else
      strm << addr;
  }
}


PBoolean H323Transactor::SetTransport(const H323TransportAddress & iface)
{
  PWaitAndSignal mutex(pduWriteMutex);

  if (transport != NULL && transport->GetLocalAddress().IsEquivalent(iface)) {
    PTRACE(2, "Trans\tAlready have listener for " << iface);
    return PTrue;
  }

  PIPSocket::Address addr;
  WORD port = defaultLocalPort;
  if (!iface.GetIpAndPort(addr, port)) {
    PTRACE(1, "Trans\tCannot create listener for " << iface);
    return PFalse;
  }

  if (transport != NULL) {
    transport->CleanUpOnTermination();
    delete transport;
  }

  transport = new H323TransportUDP(endpoint, addr, port);
  transport->SetPromiscuous(H323Transport::AcceptFromAny);
  return StartChannel();;
}


H323TransportAddressArray H323Transactor::GetInterfaceAddresses(bool excludeLocalHost)
{
  if (transport == NULL)
    return H323TransportAddressArray();
  else
    return endpoint.GetInterfaceAddresses(excludeLocalHost, transport);
}


PBoolean H323Transactor::StartChannel()
{
  if (transport == NULL)
    return PFalse;

  transport->AttachThread(PThread::Create(PCREATE_NOTIFIER(HandleTransactions), "Transactor"));
  return PTrue;
}


void H323Transactor::StopChannel()
{
  if (transport != NULL) {
    transport->CleanUpOnTermination();
    delete transport;
    transport = NULL;
  }
}


void H323Transactor::HandleTransactions(PThread &, INT)
{
  if (PAssertNULL(transport) == NULL)
    return;

  PTRACE(3, "Trans\tStarting listener thread on " << *transport);

  transport->SetReadTimeout(PMaxTimeInterval);

  PINDEX consecutiveErrors = 0;

  PBoolean ok = PTrue;
  while (ok) {
    PTRACE(5, "Trans\tReading PDU");
    H323TransactionPDU * response = CreateTransactionPDU();
    if (response->Read(*transport)) {
      consecutiveErrors = 0;
      lastRequest = NULL;
      if (HandleTransaction(response->GetPDU()))
        lastRequest->responseHandled.Signal();
      if (lastRequest != NULL)
        lastRequest->responseMutex.Signal();
    }
    else {
      switch (transport->GetErrorCode(PChannel::LastReadError)) {
        case PChannel::Interrupted :
          if (transport->IsOpen())
            break;
          // Do NotOpen case

        case PChannel::NotOpen :
          ok = PFalse;
          break;

        default :
          switch (transport->GetErrorNumber(PChannel::LastReadError)) {
            case ECONNRESET:
            case ECONNREFUSED:
              PTRACE(2, "Trans\tCannot access remote " << transport->GetRemoteAddress());
              break;

            default:
              PTRACE(1, "Trans\tRead error: " << transport->GetErrorText(PChannel::LastReadError));
              if (++consecutiveErrors > 10)
                ok = PFalse;
          }
      }
    }

    delete response;
    AgeResponses();
  }

  PTRACE(3, "Trans\tEnded listener thread on " << *transport);
}


PBoolean H323Transactor::SetUpCallSignalAddresses(H225_ArrayOf_TransportAddress & addresses)
{
  if (PAssertNULL(transport) == NULL)
    return PFalse;

  H323SetTransportAddresses(*transport,
                            endpoint.GetInterfaceAddresses(PFalse, transport),
                            addresses);

  return addresses.GetSize() > 0;
}


unsigned H323Transactor::GetNextSequenceNumber()
{
  PWaitAndSignal mutex(nextSequenceNumberMutex);
  nextSequenceNumber++;
  if (nextSequenceNumber >= 65536)
    nextSequenceNumber = 1;
  return nextSequenceNumber;
}


void H323Transactor::AgeResponses()
{
  PTime now;

  PWaitAndSignal mutex(pduWriteMutex);

  for (PINDEX i = 0; i < responses.GetSize(); i++) {
    const Response & response = responses[i];
    if ((now - response.lastUsedTime) > response.retirementAge) {
      PTRACE(4, "Trans\tRemoving cached response: " << response);
      responses.RemoveAt(i--);
    }
  }
}


PBoolean H323Transactor::SendCachedResponse(const H323TransactionPDU & pdu)
{
  if (PAssertNULL(transport) == NULL)
    return PFalse;

  Response key(transport->GetLastReceivedAddress(), pdu.GetSequenceNumber());

  PWaitAndSignal mutex(pduWriteMutex);

  PINDEX idx = responses.GetValuesIndex(key);
  if (idx != P_MAX_INDEX)
    return responses[idx].SendCachedResponse(*transport);

  responses.Append(new Response(key));
  return PFalse;
}


PBoolean H323Transactor::WritePDU(H323TransactionPDU & pdu)
{
  if (PAssertNULL(transport) == NULL)
    return PFalse;

  OnSendingPDU(pdu.GetPDU());

  PWaitAndSignal mutex(pduWriteMutex);

  Response key(transport->GetLastReceivedAddress(), pdu.GetSequenceNumber());
  PINDEX idx = responses.GetValuesIndex(key);
  if (idx != P_MAX_INDEX)
    responses[idx].SetPDU(pdu);

  return pdu.Write(*transport);
}


PBoolean H323Transactor::WriteTo(H323TransactionPDU & pdu,
                             const H323TransportAddressArray & addresses,
                             PBoolean callback)
{
  if (PAssertNULL(transport) == NULL)
    return PFalse;

  if (addresses.IsEmpty()) {
    if (callback)
      return WritePDU(pdu);

    return pdu.Write(*transport);
  }

  pduWriteMutex.Wait();

  H323TransportAddress oldAddress = transport->GetRemoteAddress();

  PBoolean ok = PFalse;
  for (PINDEX i = 0; i < addresses.GetSize(); i++) {
    if (transport->SetRemoteAddress(addresses[i])) {
      PTRACE(3, "Trans\tWrite address set to " << addresses[i]);
      if (callback)
        ok = WritePDU(pdu);
      else
        ok = pdu.Write(*transport);
    }
  }

  transport->SetRemoteAddress(oldAddress);

  pduWriteMutex.Signal();

  return ok;
}


PBoolean H323Transactor::MakeRequest(Request & request)
{
  PTRACE(3, "Trans\tMaking request: " << request.requestPDU.GetChoice().GetTagName());

  OnSendingPDU(request.requestPDU.GetPDU());

  requestsMutex.Wait();
  requests.SetAt(request.sequenceNumber, &request);
  requestsMutex.Signal();

  PBoolean ok = request.Poll(*this);

  requestsMutex.Wait();
  requests.SetAt(request.sequenceNumber, NULL);
  requestsMutex.Signal();

  return ok;
}


PBoolean H323Transactor::CheckForResponse(unsigned reqTag, unsigned seqNum, const PASN_Choice * reason)
{
  requestsMutex.Wait();
  lastRequest = requests.GetAt(seqNum);
  requestsMutex.Signal();

  if (lastRequest == NULL) {
    PTRACE(2, "Trans\tTimed out or received sequence number (" << seqNum << ") for PDU we never requested");
    return PFalse;
  }

  lastRequest->responseMutex.Wait();
  lastRequest->CheckResponse(reqTag, reason);
  return PTrue;
}


PBoolean H323Transactor::HandleRequestInProgress(const H323TransactionPDU & pdu,
                                             unsigned delay)
{
  unsigned seqNum = pdu.GetSequenceNumber();

  requestsMutex.Wait();
  lastRequest = requests.GetAt(seqNum);
  requestsMutex.Signal();

  if (lastRequest == NULL) {
    PTRACE(2, "Trans\tTimed out or received sequence number (" << seqNum << ") for PDU we never requested");
    return PFalse;
  }

  lastRequest->responseMutex.Wait();

  PTRACE(3, "Trans\tReceived RIP on sequence number " << seqNum);
  lastRequest->OnReceiveRIP(delay);
  return PTrue;
}


PBoolean H323Transactor::CheckCryptoTokens(const H323TransactionPDU & pdu,
                                       const PASN_Array & clearTokens,
                                       unsigned clearOptionalField,
                                       const PASN_Array & cryptoTokens,
                                       unsigned cryptoOptionalField)
{
  // If cypto token checking disabled, just return PTrue.
  if (!GetCheckResponseCryptoTokens())
    return PTrue;

  if (lastRequest != NULL && pdu.GetAuthenticators().IsEmpty()) {
    ((H323TransactionPDU &)pdu).SetAuthenticators(lastRequest->requestPDU.GetAuthenticators());
    PTRACE(4, "Trans\tUsing credentials from request: "
           << setfill(',') << pdu.GetAuthenticators() << setfill(' '));
  }

  if (pdu.Validate(clearTokens, clearOptionalField,
                   cryptoTokens, cryptoOptionalField) == H235Authenticator::e_OK)
    return PTrue;

  /* Note that a crypto tokens error is flagged to the requestor in the
     responseResult field but the other thread is NOT signalled. This is so
     it can wait for the full timeout for any other packets that might have
     the correct tokens, preventing a possible DOS attack.
   */
  if (lastRequest != NULL) {
    lastRequest->responseResult = Request::BadCryptoTokens;
    lastRequest->responseHandled.Signal();
    lastRequest->responseMutex.Signal();
    lastRequest = NULL;
  }

  return PFalse;
}


/////////////////////////////////////////////////////////////////////////////

H323Transactor::Request::Request(unsigned seqNum, H323TransactionPDU & pdu)
  : requestPDU(pdu)
{
  sequenceNumber = seqNum;
  responseInfo   = NULL;
}


H323Transactor::Request::Request(unsigned seqNum,
                                 H323TransactionPDU & pdu,
                                 const H323TransportAddressArray & addresses)
  : requestAddresses(addresses),
    requestPDU(pdu)
{
  sequenceNumber = seqNum;
  responseInfo   = NULL;
}


PBoolean H323Transactor::Request::Poll(H323Transactor & rasChannel, unsigned numRetries, PTimeInterval timeout)
{
  H323EndPoint & endpoint = rasChannel.GetEndPoint();

  responseResult = AwaitingResponse;
  
  if (numRetries == 0)
    numRetries = endpoint.GetRasRequestRetries();
  
  if (timeout == 0)
    timeout = endpoint.GetRasRequestTimeout();

  for (unsigned retry = 1; retry <= numRetries; retry++) {
    // To avoid race condition with RIP must set timeout before sending the packet
    whenResponseExpected = PTimer::Tick() + timeout;

    if (!rasChannel.WriteTo(requestPDU, requestAddresses, PFalse))
      break;

    PTRACE(3, "Trans\tWaiting on response to seqnum=" << requestPDU.GetSequenceNumber()
           << " for " << setprecision(1) << timeout << " seconds");

    do {
      // Wait for a response
      responseHandled.Wait(whenResponseExpected - PTimer::Tick());

      PWaitAndSignal mutex(responseMutex); // Wait till lastRequest goes out of scope

      switch (responseResult) {
        case AwaitingResponse :  // Was a timeout
          responseResult = NoResponseReceived;
          break;

        case ConfirmReceived :
          return PTrue;

        case RejectReceived :
        case TryAlternate :
          return PFalse;

        case BadCryptoTokens :
          PTRACE(1, "Trans\tResponse to seqnum=" << requestPDU.GetSequenceNumber()
                 << " had invalid crypto tokens.");
          return PFalse;

        default : // RequestInProgress
          responseResult = AwaitingResponse; // Keep waiting
      }

      PTRACE_IF(3, responseResult == AwaitingResponse,
                "Trans\tWaiting again on response to seqnum=" << requestPDU.GetSequenceNumber() <<
                " for " << setprecision(1) << (whenResponseExpected - PTimer::Tick()) << " seconds");
    } while (responseResult == AwaitingResponse);

    PTRACE(1, "Trans\tTimeout on request seqnum=" << requestPDU.GetSequenceNumber()
           << ", try #" << retry << " of " << numRetries);
  }

  return PFalse;
}


void H323Transactor::Request::CheckResponse(unsigned reqTag, const PASN_Choice * reason)
{
  if (requestPDU.GetChoice().GetTag() != reqTag) {
    PTRACE(2, "Trans\tReceived reply for incorrect PDU tag.");
    responseResult = RejectReceived;
    rejectReason = UINT_MAX;
    return;
  }

  if (reason == NULL) {
    responseResult = ConfirmReceived;
    return;
  }

  PTRACE(2, "Trans\t" << requestPDU.GetChoice().GetTagName()
         << " rejected: " << reason->GetTagName());
  responseResult = RejectReceived;
  rejectReason = reason->GetTag();

  switch(reqTag) {
    case H225_RasMessage::e_admissionRequest:
      if (rejectReason == H225_AdmissionRejectReason::e_callerNotRegistered)
        responseResult = TryAlternate;
      break;

    case H225_RasMessage::e_gatekeeperRequest:
      if (rejectReason == H225_GatekeeperRejectReason::e_resourceUnavailable)
        responseResult = TryAlternate;
      break;

    case H225_RasMessage::e_disengageRequest:
      if (rejectReason == H225_DisengageRejectReason::e_notRegistered)
        responseResult = TryAlternate;
      break;

    case H225_RasMessage::e_registrationRequest:
      if (rejectReason == H225_RegistrationRejectReason::e_resourceUnavailable)
        responseResult = TryAlternate;
      break;

    case H225_RasMessage::e_infoRequestResponse:
      if (rejectReason == H225_InfoRequestNakReason::e_notRegistered)
        responseResult = TryAlternate;
      break;
  }
}


void H323Transactor::Request::OnReceiveRIP(unsigned milliseconds)
{
  responseResult = RequestInProgress;
  whenResponseExpected = PTimer::Tick() + PTimeInterval(milliseconds);
}


/////////////////////////////////////////////////////////////////////////////

H323Transactor::Response::Response(const H323TransportAddress & addr, unsigned seqNum)
  : PString(addr),
    retirementAge(ResponseRetirementAge)
{
  sprintf("#%u", seqNum);
  replyPDU = NULL;
}


H323Transactor::Response::~Response()
{
  if (replyPDU != NULL)
    replyPDU->DeletePDU();
}


void H323Transactor::Response::SetPDU(const H323TransactionPDU & pdu)
{
  PTRACE(4, "Trans\tAdding cached response: " << *this);

  if (replyPDU != NULL)
    replyPDU->DeletePDU();
  replyPDU = pdu.ClonePDU();
  lastUsedTime = PTime();

  unsigned delay = pdu.GetRequestInProgressDelay();
  if (delay > 0)
    retirementAge = ResponseRetirementAge + delay;
}


PBoolean H323Transactor::Response::SendCachedResponse(H323Transport & transport)
{
  PTRACE(3, "Trans\tSending cached response: " << *this);

  if (replyPDU != NULL) {
    H323TransportAddress oldAddress = transport.GetRemoteAddress();
    transport.ConnectTo(Left(FindLast('#')));
    replyPDU->Write(transport);
    transport.ConnectTo(oldAddress);
  }
  else {
    PTRACE(2, "Trans\tRetry made by remote before sending response: " << *this);
  }

  lastUsedTime = PTime();
  return PTrue;
}


/////////////////////////////////////////////////////////////////////////////////

H323Transaction::H323Transaction(H323Transactor & trans,
                                 const H323TransactionPDU & requestToCopy,
                                 H323TransactionPDU * conf,
                                 H323TransactionPDU * rej)
  : transactor(trans),
    replyAddresses(trans.GetTransport().GetLastReceivedAddress()),
    request(requestToCopy.ClonePDU())
{
  confirm = conf;
  reject = rej;
  authenticatorResult = H235Authenticator::e_Disabled;
  fastResponseRequired = PTrue;
  isBehindNAT = PFalse;
  canSendRIP  = PFalse;
}


H323Transaction::~H323Transaction()
{
  delete request;
  delete confirm;
  delete reject;
}


PBoolean H323Transaction::HandlePDU()
{
  int response = OnHandlePDU();
  switch (response) {
    case Ignore :
      return PFalse;

    case Confirm :
      if (confirm != NULL)
        WritePDU(*confirm);
      return PFalse;

    case Reject :
      if (reject != NULL)
        WritePDU(*reject);
      return PFalse;
  }

  H323TransactionPDU * rip = CreateRIP(request->GetSequenceNumber(), response);
  PBoolean ok = WritePDU(*rip);
  delete rip;

  if (!ok)
    return PFalse;

  if (fastResponseRequired) {
    fastResponseRequired = PFalse;
    PThread::Create(PCREATE_NOTIFIER(SlowHandler), 0,
                                     PThread::AutoDeleteThread,
                                     PThread::NormalPriority,
                                     "Transaction");
  }

  return PTrue;
}


void H323Transaction::SlowHandler(PThread &, INT)
{
  PTRACE(4, "Trans\tStarted slow PDU handler thread.");

  while (HandlePDU())
    ;

  delete this;

  PTRACE(4, "Trans\tEnded slow PDU handler thread.");
}


PBoolean H323Transaction::WritePDU(H323TransactionPDU & pdu)
{
  pdu.SetAuthenticators(authenticators);
  return transactor.WriteTo(pdu, replyAddresses, PTrue);
}


PBoolean H323Transaction::CheckCryptoTokens(const H235Authenticators & auth)
{
  authenticators = auth;

  request->SetAuthenticators(authenticators);

  authenticatorResult = ValidatePDU();

  if (authenticatorResult == H235Authenticator::e_OK)
    return PTrue;

  PINDEX i;
  for (i = 0; (authenticatorStrings[i].code >= 0) && (authenticatorStrings[i].code != authenticatorResult); ++i) 
    ;

  const char * desc = "Unknown error";
  if (authenticatorStrings[i].code >= 0)
    desc = authenticatorStrings[i].desc;

  PTRACE(2, "Trans\t" << GetName() << " rejected - " << desc);
  return PFalse;
}


/////////////////////////////////////////////////////////////////////////////////

H323TransactionServer::H323TransactionServer(H323EndPoint & ep)
  : ownerEndPoint(ep)
{
}


H323TransactionServer::~H323TransactionServer()
{
}


PBoolean H323TransactionServer::AddListeners(const H323TransportAddressArray & ifaces)
{
  if (ifaces.IsEmpty())
    return AddListener("udp$*");

  PINDEX i;

  mutex.Wait();
  ListenerList::iterator iterListener = listeners.begin();
  while (iterListener != listeners.end()) {
    PBoolean remove = PTrue;
    for (PINDEX j = 0; j < ifaces.GetSize(); j++) {
      if (iterListener->GetTransport().GetLocalAddress().IsEquivalent(ifaces[j], true)) {
        remove = PFalse;
       break;
      }
    }
    if (remove) {
      PTRACE(3, "Trans\tRemoving listener " << *iterListener);
      listeners.erase(iterListener++);
    }
    else
      ++iterListener;
  }
  mutex.Signal();

  for (i = 0; i < ifaces.GetSize(); i++) {
    if (!ifaces[i])
      AddListener(ifaces[i]);
  }

  return listeners.GetSize() > 0;
}


PBoolean H323TransactionServer::AddListener(const H323TransportAddress & interfaceName)
{
  PWaitAndSignal wait(mutex);

  PINDEX i;
  for (ListenerList::iterator iterListener = listeners.begin(); iterListener != listeners.end(); ++iterListener) {
    if (iterListener->GetTransport().GetLocalAddress().IsEquivalent(interfaceName, true)) {
      PTRACE(2, "H323\tAlready have listener for " << interfaceName);
      return PTrue;
    }
  }

  PIPSocket::Address addr;
  WORD port = GetDefaultUdpPort();
  if (!interfaceName.GetIpAndPort(addr, port))
    return AddListener(interfaceName.CreateTransport(ownerEndPoint));

  if (!addr.IsAny())
    return AddListener(new H323TransportUDP(ownerEndPoint, addr, port));

  PIPSocket::InterfaceTable interfaces;
  if (!PIPSocket::GetInterfaceTable(interfaces)) {
    PTRACE(1, "Trans\tNo interfaces on system!");
    if (!PIPSocket::GetHostAddress(addr))
      return PFalse;
    return AddListener(new H323TransportUDP(ownerEndPoint, addr, port));
  }

  PTRACE(4, "Trans\tAdding interfaces:\n" << setfill('\n') << interfaces << setfill(' '));

  PBoolean atLeastOne = PFalse;

  for (i = 0; i < interfaces.GetSize(); i++) {
    addr = interfaces[i].GetAddress();
    if (addr != 0) {
      if (AddListener(new H323TransportUDP(ownerEndPoint, addr, port, false, true)))
        atLeastOne = PTrue;
    }
  }

  return atLeastOne;
}


PBoolean H323TransactionServer::AddListener(H323Transport * transport)
{
  if (transport == NULL)
    return PFalse;

  if (!transport->IsOpen()) {
    delete transport;
    return PFalse;
  }

  return AddListener(CreateListener(transport));
}


PBoolean H323TransactionServer::AddListener(H323Transactor * listener)
{
  if (listener == NULL)
    return PFalse;

  PTRACE(3, "Trans\tStarted listener " << *listener);

  mutex.Wait();
  listeners.Append(listener);
  mutex.Signal();

  listener->StartChannel();

  return PTrue;
}


PBoolean H323TransactionServer::RemoveListener(H323Transactor * listener)
{
  PBoolean ok = PTrue;

  mutex.Wait();
  if (listener != NULL) {
    PTRACE(3, "Trans\tRemoving listener " << *listener);
    ok = listeners.Remove(listener);
  }
  else {
    PTRACE(3, "Trans\tRemoving all listeners");
    listeners.RemoveAll();
  }
  mutex.Signal();

  return ok;
}


#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////////
