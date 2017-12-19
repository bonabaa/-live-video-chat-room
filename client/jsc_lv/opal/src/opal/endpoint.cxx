/*
 * endpoint.cxx
 *
 * Media channels abstraction
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23926 $
 * $Author: rjongbloed $
 * $Date: 2010-01-13 00:44:33 +0000 (Wed, 13 Jan 2010) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "endpoint.h"
#endif

#include <opal/buildopts.h>

#include <opal/endpoint.h>
#include <opal/manager.h>
#include <opal/call.h>
#include <rtp/rtp.h>

//
// TODO find the correct value, usually the bandwidth for pure audio call is 1280 kb/sec 
// Set the bandwidth to 10Mbits, as setting the bandwith to UINT_MAX causes problems with cisco gatekeepers 
//
#define BANDWITH_DEFAULT_INITIAL 100000

#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalEndPoint::OpalEndPoint(OpalManager & mgr,
                           const PCaselessString & prefix,
                           unsigned attributes)
  : manager(mgr),
    prefixName(prefix),
    attributeBits(attributes),
    productInfo(mgr.GetProductInfo()),
    defaultLocalPartyName(manager.GetDefaultUserName()),
    defaultDisplayName(manager.GetDefaultDisplayName())
{
  manager.AttachEndPoint(this);

  defaultSignalPort = 0;

  initialBandwidth = BANDWITH_DEFAULT_INITIAL;
  defaultSendUserInputMode = OpalConnection::SendUserInputAsProtocolDefault;

  if (defaultLocalPartyName.IsEmpty())
    defaultLocalPartyName = PProcess::Current().GetName() & "User";

  PTRACE(4, "OpalEP\tCreated endpoint: " << prefixName);
}


OpalEndPoint::~OpalEndPoint()
{
  PTRACE(4, "OpalEP\t" << prefixName << " endpoint destroyed.");
}


void OpalEndPoint::ShutDown()
{
  PTRACE(3, "OpalEP\t" << prefixName << " endpoint shutting down.");

  // Shut down the listeners as soon as possible to avoid race conditions
  listeners.RemoveAll();
}


void OpalEndPoint::PrintOn(ostream & strm) const
{
  strm << "EP<" << prefixName << '>';
}


PBoolean OpalEndPoint::GarbageCollection()
{
  for (PSafePtr<OpalConnection> connection(connectionsActive, PSafeReference); connection != NULL; ++connection)
    connection->GarbageCollection();

  return connectionsActive.DeleteObjectsToBeRemoved();
}


PBoolean OpalEndPoint::StartListeners(const PStringArray & listenerAddresses)
{
  PStringArray interfaces = listenerAddresses;
  if (interfaces.IsEmpty()) {
    interfaces = GetDefaultListeners();
    if (interfaces.IsEmpty())
      return PFalse;
  }

  PBoolean startedOne = PFalse;

  for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
    if (interfaces[i].Find('$') != P_MAX_INDEX) {
      if (StartListener(interfaces[i]))
        startedOne = PTrue;
    }
    else {
      PStringArray transports = GetDefaultTransport().Tokenise(',');
      for (PINDEX j = 0; j < transports.GetSize(); j++) {
        OpalTransportAddress iface(interfaces[i], GetDefaultSignalPort(), transports[j]);
        if (StartListener(iface))
          startedOne = PTrue;
      }
    }
  }

  return startedOne;
}


PBoolean OpalEndPoint::StartListener(const OpalTransportAddress & listenerAddress)
{
  OpalListener * listener;

  OpalTransportAddress iface = listenerAddress;

  if (iface.IsEmpty()) {
    PStringArray interfaces = GetDefaultListeners();
    if (interfaces.IsEmpty())
      return PFalse;
    iface = OpalTransportAddress(interfaces[0], defaultSignalPort);
  }

  listener = iface.CreateListener(*this, OpalTransportAddress::FullTSAP);
  if (listener == NULL) {
    PTRACE(1, "OpalEP\tCould not create listener: " << iface);
    return PFalse;
  }

  if (StartListener(listener))
    return PTrue;

  PTRACE(1, "OpalEP\tCould not start listener: " << iface);
  return PFalse;
}


PBoolean OpalEndPoint::StartListener(OpalListener * listener)
{
  if (listener == NULL)
    return PFalse;

  // as the listener is not open, this will have the effect of immediately
  // stopping the listener thread. This is good - it means that the 
  // listener Close function will appear to have stopped the thread
  if (!listener->Open(PCREATE_NOTIFIER(ListenerCallback))) {
    delete listener;
    return PFalse;
  }

  listeners.Append(listener);
  return PTrue;
}

PString OpalEndPoint::GetDefaultTransport() const
{
  return "tcp$";
}

PStringArray OpalEndPoint::GetDefaultListeners() const
{
  PStringArray listenerAddresses;
  PStringArray transports = GetDefaultTransport().Tokenise(',');
  for (PINDEX i = 0; i < transports.GetSize(); i++) {
    PString t = transports[i];
    WORD port = defaultSignalPort;
    PINDEX pos = t.Find(':');
    if (pos != P_MAX_INDEX) {
      port = (WORD)t.Mid(pos+1).AsUnsigned();
      t = t.Left(pos);
    }
    PString listenerAddress = t + '*';
    if (defaultSignalPort != 0)
      listenerAddress.sprintf(":%u", port);
    listenerAddresses += listenerAddress;
  }
  return listenerAddresses;
}


OpalListener * OpalEndPoint::FindListener(const OpalTransportAddress & iface)
{
  for (OpalListenerList::iterator listener = listeners.begin(); listener != listeners.end(); ++listener) {
    if (listener->GetLocalAddress().IsEquivalent(iface, true))
      return &*listener;
  }
  return NULL;
}


PBoolean OpalEndPoint::StopListener(const OpalTransportAddress & iface)
{
  OpalListener * listener = FindListener(iface);
  return listener != NULL && RemoveListener(listener);
}


PBoolean OpalEndPoint::RemoveListener(OpalListener * listener)
{
  if (listener != NULL)
    return listeners.Remove(listener);

  listeners.RemoveAll();
  return PTrue;
}


static void AddTransportAddress(OpalTransportAddressArray & interfaceAddresses,
                                const PIPSocket::Address & natInterfaceIP,
                                const PIPSocket::Address & natExternalIP,
                                const PIPSocket::Address & ip,
                                WORD port,
                                const PString & proto)
{
  if (ip != natExternalIP && (natInterfaceIP.IsAny() || ip == natInterfaceIP))
    AddTransportAddress(interfaceAddresses, natInterfaceIP, natExternalIP, natExternalIP, port, proto);

  OpalTransportAddress addr(ip, port, proto);
  if (interfaceAddresses.GetValuesIndex(addr) == P_MAX_INDEX)
    interfaceAddresses.Append(new OpalTransportAddress(addr));
}


static void AddTransportAddresses(OpalTransportAddressArray & interfaceAddresses,
                                  bool excludeLocalHost,
                                  const PIPSocket::Address & natInterfaceIP,
                                  const PIPSocket::Address & natExternalIP,
                                  const OpalTransportAddress & preferredAddress,
                                  const OpalTransportAddress & localAddress)
{
  if (!preferredAddress.IsEmpty() && !preferredAddress.IsEquivalent(localAddress, true))
    return;

  PIPSocket::Address localIP;
  WORD port = 0;
  if (!localAddress.GetIpAndPort(localIP, port))
    return;

  PCaselessString proto = localAddress.GetProto();
  PIPSocket::InterfaceTable interfaces;
  if (!localIP.IsAny() || !PIPSocket::GetInterfaceTable(interfaces)) {
    AddTransportAddress(interfaceAddresses, natInterfaceIP, natExternalIP, localIP, port, proto);
    return;
  }


  PIPSocket::Address preferredIP;
  if (preferredAddress.GetIpAddress(preferredIP)) {
    for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
      PIPSocket::Address ip = interfaces[i].GetAddress();
      if (ip == preferredIP)
        AddTransportAddress(interfaceAddresses, natInterfaceIP, natExternalIP, ip, port, proto);
    }
  }

  for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
    PIPSocket::Address ip = interfaces[i].GetAddress();
    if (!excludeLocalHost || !ip.IsLoopback())
      AddTransportAddress(interfaceAddresses, natInterfaceIP, natExternalIP, ip, port, proto);
  }
}


OpalTransportAddressArray OpalEndPoint::GetInterfaceAddresses(PBoolean excludeLocalHost,
                                                              const OpalTransport * associatedTransport)
{
///=================softfoundry zhx--20090213===========
#if 1
  OpalTransportAddressArray ifaces;
  if(associatedTransport!=NULL)
  {
    OpalTransportAddress opalraddr = associatedTransport->GetRemoteAddress();
	OpalTransportAddress opalladdr = associatedTransport->GetLocalAddress();
	PIPSocket::Address raddr;
	if(opalraddr.GetIpAddress(raddr))
	{
///===============changed by wu hailong
		PIPSocket::Address laddr1;
		WORD port = 0;
		opalladdr.GetIpAndPort(laddr1, port);
#ifdef _WIN32
  	  PIPSocket::Address laddr = PIPSocket::GetRouteAddress(raddr);
#else
 	  PIPSocket::Address laddr =  raddr;
#endif
	  if(laddr.IsValid())
	  {
	    PString proto = opalladdr.Left(opalladdr.Find('$')+1);
		ifaces.Append(new OpalTransportAddress(laddr,port,proto));
		return ifaces;
	  }
	}
  }
#endif
//===============================================
  OpalTransportAddressArray interfaceAddresses;

  OpalTransportAddress associatedLocalAddress, associatedRemoteAddress;
  PIPSocket::Address natInterfaceIP;
  PIPSocket::Address natExternalIP;
  if (associatedTransport != NULL) {
    associatedLocalAddress = associatedTransport->GetLocalAddress();
    associatedRemoteAddress = associatedTransport->GetRemoteAddress();

    PIPSocket::Address remoteIP;
    associatedRemoteAddress.GetIpAddress(remoteIP);

    PNatMethod * natMethod = manager.GetNatMethod(remoteIP);
    if (natMethod != NULL) {
      natMethod->GetInterfaceAddress(natInterfaceIP);
      natMethod->GetExternalAddress(natExternalIP);
    }
    else if (manager.GetTranslationAddress().IsValid()) {
      natInterfaceIP = PIPSocket::GetDefaultIpAny();
      natExternalIP = manager.GetTranslationAddress();
    }
  }

  OpalListenerList::iterator listener;

  if (!associatedLocalAddress.IsEmpty()) {
    for (listener = listeners.begin(); listener != listeners.end(); ++listener)
      AddTransportAddresses(interfaceAddresses,
                            excludeLocalHost,
                            natInterfaceIP,
                            natExternalIP,
                            associatedLocalAddress,
                            listener->GetLocalAddress(associatedRemoteAddress));
  }

  for (listener = listeners.begin(); listener != listeners.end(); ++listener)
    AddTransportAddresses(interfaceAddresses,
                          excludeLocalHost,
                          natInterfaceIP,
                          natExternalIP,
                          OpalTransportAddress(),
                          listener->GetLocalAddress());

  PTRACE(4, "OpalMan\tListener interfaces: associated transport="
         << (associatedTransport != NULL ? (const char *)associatedLocalAddress : "None")
         << "\n    " << setfill(',') << interfaceAddresses);
  return interfaceAddresses;
}


void OpalEndPoint::ListenerCallback(PThread &, INT param)
{
  // Error in accept, usually means listener thread stopped, so just exit
  if (param == 0)
    return;

  OpalTransport * transport = (OpalTransport *)param;
  if (NewIncomingConnection(transport))
    delete transport;
}


PBoolean OpalEndPoint::NewIncomingConnection(OpalTransport * /*transport*/)
{
  return PTrue;
}


PStringList OpalEndPoint::GetAllConnections()
{
  PStringList tokens;

  for (PSafePtr<OpalConnection> connection(connectionsActive, PSafeReadOnly); connection != NULL; ++connection)
    tokens.AppendString(connection->GetToken());

  return tokens;
}


PBoolean OpalEndPoint::HasConnection(const PString & token)
{
  PWaitAndSignal wait(inUseFlag);
  return connectionsActive.Contains(token);
}


OpalConnection * OpalEndPoint::AddConnection(OpalConnection * connection)
{
  if (connection == NULL)
    return NULL;

  connection->SetStringOptions(m_defaultStringOptions, false);

  OnNewConnection(connection->GetCall(), *connection);

  connectionsActive.SetAt(connection->GetToken(), connection);

  return connection;
}


void OpalEndPoint::DestroyConnection(OpalConnection * connection)
{
  delete connection;
}


PBoolean OpalEndPoint::OnSetUpConnection(OpalConnection & PTRACE_PARAM(connection))
{
  PTRACE(3, "OpalEP\tOnSetUpConnection " << connection);
  return PTrue;
}


PBoolean OpalEndPoint::OnIncomingConnection(OpalConnection & connection, unsigned options, OpalConnection::StringOptions * stringOptions)
{
  return manager.OnIncomingConnection(connection, options, stringOptions);
}


void OpalEndPoint::OnProceeding(OpalConnection & connection)
{
  manager.OnProceeding(connection);
}

void OpalEndPoint::OnAlerting(OpalConnection & connection)
{
  manager.OnAlerting(connection);
}

OpalConnection::AnswerCallResponse
       OpalEndPoint::OnAnswerCall(OpalConnection & connection,
                                  const PString & caller)
{
  return manager.OnAnswerCall(connection, caller);
}

void OpalEndPoint::OnConnected(OpalConnection & connection)
{
  manager.OnConnected(connection);
}


void OpalEndPoint::OnEstablished(OpalConnection & connection)
{
  manager.OnEstablished(connection);
}


void OpalEndPoint::OnReleased(OpalConnection & connection)
{
  PTRACE(4, "OpalEP\tOnReleased " << connection);
  connectionsActive.RemoveAt(connection.GetToken());
  manager.OnReleased(connection);
}


void OpalEndPoint::OnHold(OpalConnection & connection, bool fromRemote, bool onHold)
{
  manager.OnHold(connection, fromRemote, onHold);
}


void OpalEndPoint::OnHold(OpalConnection & connection)
{
  manager.OnHold(connection);
}


PBoolean OpalEndPoint::OnForwarded(OpalConnection & connection,
			       const PString & forwardParty)
{
  PTRACE(4, "OpalEP\tOnForwarded " << connection);
  return manager.OnForwarded(connection, forwardParty);
}


void OpalEndPoint::ConnectionDict::DeleteObject(PObject * object) const
{
  OpalConnection * connection = PDownCast(OpalConnection, object);
  if (connection != NULL)
    connection->GetEndPoint().DestroyConnection(connection);
}


PBoolean OpalEndPoint::ClearCall(const PString & token,
                             OpalConnection::CallEndReason reason,
                             PSyncPoint * sync)
{
  return manager.ClearCall(token, reason, sync);
}


PBoolean OpalEndPoint::ClearCallSynchronous(const PString & token,
                                        OpalConnection::CallEndReason reason,
                                        PSyncPoint * sync)
{
  PSyncPoint syncPoint;
  if (sync == NULL)
    sync = &syncPoint;
  return manager.ClearCall(token, reason, sync);
}


void OpalEndPoint::ClearAllCalls(OpalConnection::CallEndReason reason, PBoolean wait)
{
  manager.ClearAllCalls(reason, wait);
}


void OpalEndPoint::AdjustMediaFormats(bool local,
                                      const OpalConnection & connection,
                                      OpalMediaFormatList & mediaFormats) const
{
  manager.AdjustMediaFormats(local, connection, mediaFormats);
}


PBoolean OpalEndPoint::OnOpenMediaStream(OpalConnection & connection,
                                     OpalMediaStream & stream)
{
  return manager.OnOpenMediaStream(connection, stream);
}


void OpalEndPoint::OnClosedMediaStream(const OpalMediaStream & stream)
{
  manager.OnClosedMediaStream(stream);
}

#if OPAL_VIDEO
PBoolean OpalEndPoint::CreateVideoInputDevice(const OpalConnection & connection,
                                          const OpalMediaFormat & mediaFormat,
                                          PVideoInputDevice * & device,
                                          PBoolean & autoDelete)
{
  return manager.CreateVideoInputDevice(connection, mediaFormat, device, autoDelete);
}

PBoolean OpalEndPoint::CreateVideoOutputDevice(const OpalConnection & connection,
                                           const OpalMediaFormat & mediaFormat,
                                           PBoolean preview,
                                           PVideoOutputDevice * & device,
                                           PBoolean & autoDelete)
{
  return manager.CreateVideoOutputDevice(connection, mediaFormat, preview, device, autoDelete);
}
#endif


void OpalEndPoint::OnUserInputString(OpalConnection & connection,
                                     const PString & value)
{
  manager.OnUserInputString(connection, value);
}


void OpalEndPoint::OnUserInputTone(OpalConnection & connection,
                                   char tone,
                                   int duration)
{
  manager.OnUserInputTone(connection, tone, duration);
}


PString OpalEndPoint::ReadUserInput(OpalConnection & connection,
                                    const char * terminators,
                                    unsigned lastDigitTimeout,
                                    unsigned firstDigitTimeout)
{
  return manager.ReadUserInput(connection, terminators, lastDigitTimeout, firstDigitTimeout);
}


void OpalEndPoint::OnNewConnection(OpalCall & call, OpalConnection & connection)
{
  call.OnNewConnection(connection);
}


void OpalEndPoint::OnMWIReceived(const PString & party,
                                 OpalManager::MessageWaitingType type,
                                 const PString & extraInfo)
{
  manager.OnMWIReceived(party, type, extraInfo);
}

bool OpalEndPoint::FindListenerForProtocol(const char * protoPrefix, OpalTransportAddress & addr)
{
  OpalTransportAddress compatibleTo("*", 0, protoPrefix);
  for (OpalListenerList::iterator listener = listeners.begin(); listener != listeners.end(); ++listener) {
    addr = listener->GetLocalAddress();
    if (addr.IsCompatible(compatibleTo))
      return true;
   }
  return false;
}

#if OPAL_PTLIB_SSL
PString OpalEndPoint::GetSSLCertificate() const
{
  return "server.pem";
}
#endif


bool OpalEndPoint::Message(const PString & to, const PString & body)
{
  PURL from;
  PString conversationId;
  return Message(to, "text/plain", body, from, conversationId);
}


PBoolean OpalEndPoint::Message(
  const PURL & /*to*/, 
  const PString & /*type*/,
  const PString & /*body*/,
  PURL & /*from*/, 
  PString & /*conversationId*/
)
{
  return false;
}

void OpalEndPoint::OnMessageReceived(
  const PURL & from, 
  const PString & fromName,
  const PURL & to, 
  const PString & type,
  const PString & body,
  const PString & conversationId
)
{
  manager.OnMessageReceived(from, fromName, to, type, body, conversationId);
}

#if OPAL_HAS_IM

bool OpalEndPoint::TransmitExternalIM(OpalConnection & conn, const OpalMediaFormat & format, RTP_IMFrame & frame)
{
  return manager.TransmitExternalIM(conn, format, frame);
}

#endif

/////////////////////////////////////////////////////////////////////////////
