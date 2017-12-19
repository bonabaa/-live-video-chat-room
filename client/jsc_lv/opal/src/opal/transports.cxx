/*
 * transport.cxx
 *
 * Opal transports handler
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
 * Portions Copyright (C) 2006 by Post Increment
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
 * Contributor(s): Post Increment
 *     Portions of this code were written with the assistance of funding from
 *     US Joint Forces Command Joint Concept Development & Experimentation (J9)
 *     http://www.jfcom.mil/about/abt_j9.htm
 *
 * $Revision: 23295 $
 * $Author: rjongbloed $
 * $Date: 2009-08-28 06:38:49 +0000 (Fri, 28 Aug 2009) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "transports.h"
#endif

#include <opal/buildopts.h>

#include <opal/transports.h>
#include <opal/manager.h>
#include <opal/endpoint.h>
#include <opal/call.h>
#include <opal/buildopts.h>

#include <ptclib/pstun.h>
#include <main.h>


static const char IpPrefix[]  = "ip$";   // For backward compatibility with OpenH323
static const char TcpPrefix[] = "tcp$";
static const char UdpPrefix[] = "udp$";

static PFactory<OpalInternalTransport>::Worker<OpalInternalTCPTransport> opalInternalTCPTransportFactory(TcpPrefix, true);
static PFactory<OpalInternalTransport>::Worker<OpalInternalTCPTransport>  opalInternalIPTransportFactory(IpPrefix, true);
static PFactory<OpalInternalTransport>::Worker<OpalInternalUDPTransport> opalInternalUDPTransportFactory(UdpPrefix, true);

#if OPAL_PTLIB_SSL
#include <ptclib/pssl.h>
static const char TcpsPrefix[] = "tcps$";
static PFactory<OpalInternalTransport>::Worker<OpalInternalTCPSTransport> opalInternalTCPSTransportFactory(TcpsPrefix, true);
#endif

/////////////////////////////////////////////////////////////////

#define new PNEW

/////////////////////////////////////////////////////////////////

OpalTransportAddress::OpalTransportAddress()
{
  transport = NULL;
}


OpalTransportAddress::OpalTransportAddress(const char * cstr,
                                           WORD port,
                                           const char * proto)
  : PCaselessString(cstr)
{
  SetInternalTransport(port, proto);
}


OpalTransportAddress::OpalTransportAddress(const PString & str,
                                           WORD port,
                                           const char * proto)
  : PCaselessString(str)
{
  SetInternalTransport(port, proto);
}


OpalTransportAddress::OpalTransportAddress(const PIPSocket::Address & addr, WORD port, const char * proto)
  : PCaselessString(addr.IsAny() ? PString('*') : addr.AsString(true))
{
  SetInternalTransport(port, proto);
}


PString OpalTransportAddress::GetHostName() const
{
  if (transport == NULL)
    return *this;

  return transport->GetHostName(*this);
}


PBoolean OpalTransportAddress::IsEquivalent(const OpalTransportAddress & address, bool wildcard) const
{
  if (*this == address)
    return PTrue;

  if (IsEmpty() || address.IsEmpty())
    return PFalse;

  PIPSocket::Address ip1, ip2;
  WORD port1 = 65535, port2 = 65535;
  return GetIpAndPort(ip1, port1) &&
         address.GetIpAndPort(ip2, port2) &&
         ((ip1 *= ip2) || (wildcard && (ip1.IsAny() || ip2.IsAny()))) &&
         (port1 == port2 || (wildcard && (port1 == 65535 || port2 == 65535)));
}


PBoolean OpalTransportAddress::IsCompatible(const OpalTransportAddress & address) const
{
  if (IsEmpty() || address.IsEmpty())
    return PTrue;

  PCaselessString myPrefix = GetProto();
  PCaselessString theirPrefix = address.GetProto();
  return myPrefix == theirPrefix ||
        (myPrefix    == IpPrefix && (theirPrefix == TcpPrefix || theirPrefix == UdpPrefix 
#if OPAL_PTLIB_SSL
                                     || theirPrefix == TcpsPrefix
#endif
                                    )) ||
        (theirPrefix == IpPrefix && (myPrefix    == TcpPrefix || myPrefix    == UdpPrefix 
#if OPAL_PTLIB_SSL
                                     || myPrefix    == TcpsPrefix
#endif
                                    ));
}


PBoolean OpalTransportAddress::GetIpAddress(PIPSocket::Address & ip) const
{
  if (transport == NULL)
    return PFalse;

  WORD dummy = 65535;
  return transport->GetIpAndPort(*this, ip, dummy);
}


PBoolean OpalTransportAddress::GetIpAndPort(PIPSocket::Address & ip, WORD & port) const
{
  if (transport == NULL)
    return PFalse;

  return transport->GetIpAndPort(*this, ip, port);
}


PBoolean OpalTransportAddress::GetIpAndPort(PIPSocketAddressAndPort & ipPort) const
{
  if (transport == NULL)
    return PFalse;

  PIPSocket::Address ip;
  WORD port = 0;
  if (!transport->GetIpAndPort(*this, ip, port))
    return false;

  ipPort.SetAddress(ip, port);
  return true;
}


OpalListener * OpalTransportAddress::CreateListener(OpalEndPoint & endpoint,
                                                    BindOptions option) const
{
  if (transport == NULL)
    return NULL;

  return transport->CreateListener(*this, endpoint, option);
}


OpalTransport * OpalTransportAddress::CreateTransport(OpalEndPoint & endpoint,
                                                      BindOptions option) const
{
  if (transport == NULL)
    return NULL;

  return transport->CreateTransport(*this, endpoint, option);
}


void OpalTransportAddress::SetInternalTransport(WORD port, const char * proto)
{
  transport = NULL;
  
  if (IsEmpty())
    return;

  PINDEX dollar = Find('$');
  if (dollar == P_MAX_INDEX) {
    PString prefix(proto == NULL ? TcpPrefix : proto);
    if (prefix.Find('$') == P_MAX_INDEX)
      prefix += '$';

    Splice(prefix, 0);
    dollar = prefix.GetLength()-1;
  }

  // use factory to create transport types
  transport = PFactory<OpalInternalTransport>::CreateInstance(Left(dollar+1).ToLower());
  if (transport == NULL)
    return;

  // Get port, even if string is bracketed, i.e. udp$[2001...]:1720
  PINDEX rbracket = Find(']');
  if( rbracket != P_MAX_INDEX )
	  dollar = rbracket + 1;

  if (port != 0 && Find(':', dollar) == P_MAX_INDEX) {
    PINDEX end = GetLength();
    if (GetAt(end-1) == '+')
      end--;
    Splice(psprintf(":%u", port), end);
  }
}


/////////////////////////////////////////////////////////////////

void OpalTransportAddressArray::AppendString(const char * str)
{
  AppendAddress(OpalTransportAddress(str));
}


void OpalTransportAddressArray::AppendString(const PString & str)
{
  AppendAddress(OpalTransportAddress(str));
}


void OpalTransportAddressArray::AppendAddress(const OpalTransportAddress & addr)
{
  if (!addr)
    Append(new OpalTransportAddress(addr));
}


void OpalTransportAddressArray::AppendStringCollection(const PCollection & coll)
{
  for (PINDEX i = 0; i < coll.GetSize(); i++) {
    PObject * obj = coll.GetAt(i);
    if (obj != NULL && PIsDescendant(obj, PString))
      AppendAddress(OpalTransportAddress(*(PString *)obj));
  }
}


/////////////////////////////////////////////////////////////////

PString OpalInternalTransport::GetHostName(const OpalTransportAddress & address) const
{
  // skip transport identifier
  PINDEX pos = address.Find('$');
  if (pos == P_MAX_INDEX)
    return address;

  return address.Mid(pos+1);
}


PBoolean OpalInternalTransport::GetIpAndPort(const OpalTransportAddress &,
                                         PIPSocket::Address &,
                                         WORD &) const
{
  return PFalse;
}


//////////////////////////////////////////////////////////////////////////

static PBoolean SplitAddress(const PString & addr, PString & host, PString & device, PString & service)
{
  // skip transport identifier
  PINDEX i = addr.Find('$');
  if (i == P_MAX_INDEX) {
    //PTRACE(1, "Address " << addr << " has no dollar");
    return false;
  }

  host.MakeEmpty();
  device.MakeEmpty();
  service.MakeEmpty();

  PINDEX j = ++i;

  // parse interface/host
  bool isInterface = (addr[j] == '%') || (addr[j] == '[' && addr[j+1] == '%');

  if (j == '\0') {
    //PTRACE(1, "Address " << addr << " has empty host");
    return false;
  }
  i = j;
  bool bracketed = addr[j] == '[';
  for (;;) {
    if (addr[j] == '\0')
      break;
    if (!bracketed) {
      if (addr[j] == ':')
        break;
    } else if (addr[j] == ']') {
      ++j;
      break;
    }
    j++;
  }
  if (i == j) {
    //PTRACE(1, "Address " << addr << " has invalid host " << i);
    return false;
  }

  if (!isInterface)
    host = addr(i, j-1);
  else {
    if (addr[i] == '[' && addr[i+1] == '%') {
      device = '%';
      device += addr(i+2, j-2);
    }
    else
      device = addr(i, j-1);
  }
  
  // parse optional service
  if (addr[j] == ':') {
    i = ++j;
    for (;;) {
      if (addr[j] == '\0')
        break;
      j++;
    }
    // cannot have zero length service
    if (i == j) {
      //PTRACE(1, "Address " << addr << " has invalid service " << i << " " << j);
      return false;
    }
    service = addr(i, j-1);
  }

  //PTRACE(1, "Split " << addr << " into host='" << host << "',dev='" << device << "',service='" << service << "'");

  return true;
}


PString OpalInternalIPTransport::GetHostName(const OpalTransportAddress & address) const
{
  PString host, device, service;
  if (!SplitAddress(address, host, device, service))
    return address;

  if (!device.IsEmpty())
    return host+device;

  PIPSocket::Address ip;
  if (ip.FromString(host))
    return ip.AsString(true);

  return host;
}


PBoolean OpalInternalIPTransport::GetIpAndPort(const OpalTransportAddress & address,
                                           PIPSocket::Address & ip,
                                           WORD & port) const
{
  PString host, device, service;
  if (!SplitAddress(address, host, device, service))
    return PFalse;

  if (host.IsEmpty() && device.IsEmpty()) {
    PTRACE(2, "Opal\tIllegal IP transport address: \"" << address << '"');
    return PFalse;
  }

  if (service == "*")
    port = 0;
  else {
    if (!service) {
      PCaselessString proto = address.GetProto();
      if (proto *= "ip")
        proto = "tcp";
      port = PIPSocket::GetPortByService(proto, service);
    }
    if (port == 0) {
      PTRACE(2, "Opal\tIllegal IP transport port/service: \"" << address << '"');
      return PFalse;
    }
  }

  if (host[0] == '*' || host == "0.0.0.0"
#ifdef P_HAS_IPV6
	  || host == ":" || host == "::"  || host == "[::]"
#endif
	  ) {
    ip = PIPSocket::GetDefaultIpAny();
    return PTrue;
  }

  if (device.IsEmpty()) {
    if (PIPSocket::GetHostAddress(host, ip))
      return true;
    PTRACE(1, "Opal\tCould not find host \"" << host << '"');
  }
  else {
    if (ip.FromString(device))
      return true;
    PTRACE(1, "Opal\tCould not find device \"" << device << '"');
  }

  return PFalse;
}


//////////////////////////////////////////////////////////////////////////

PBoolean OpalInternalIPTransport::GetAdjustedIpAndPort(const OpalTransportAddress & address,
                                 OpalEndPoint & endpoint,
                                 OpalTransportAddress::BindOptions option,
                                 PIPSocket::Address & ip,
                                 WORD & port,
                                 PBoolean & reuseAddr)
{
  reuseAddr = address[address.GetLength()-1] == '+';

  switch (option) {
    case OpalTransportAddress::NoBinding :
      ip = PIPSocket::GetDefaultIpAny();
      port = 0;
      return PTrue;

    case OpalTransportAddress::HostOnly :
      port = 0;
      return address.GetIpAddress(ip);

    case OpalTransportAddress::RouteInterface :
      if (address.GetIpAndPort(ip, port))
        ip = PIPSocket::GetRouteInterfaceAddress(ip);
      else
        ip = PIPSocket::GetDefaultIpAny();
      port = 0;
      return TRUE;

    default :
      port = endpoint.GetDefaultSignalPort();
      return address.GetIpAndPort(ip, port);
  }
}


//////////////////////////////////////////////////////////////////////////

OpalListener::OpalListener(OpalEndPoint & ep)
  : endpoint(ep)
{
  thread = NULL;
  threadMode = SpawnNewThreadMode;
}


void OpalListener::PrintOn(ostream & strm) const
{
  strm << GetLocalAddress();
}


void OpalListener::CloseWait()
{
  PTRACE(3, "Listen\tStopping listening thread on " << GetLocalAddress());
  Close();

  PThread * exitingThread = thread;
  thread = NULL;

  if (exitingThread != NULL) {
    if (exitingThread == PThread::Current())
      exitingThread->SetAutoDelete();
    else {
      PAssert(exitingThread->WaitForTermination(10000), "Listener thread did not terminate");
      delete exitingThread;
    }
  }
}


void OpalListener::ListenForConnections(PThread & thread, INT)
{
  PTRACE(3, "Listen\tStarted listening thread on " << GetLocalAddress());
  PAssert(!acceptHandler.IsNULL(), PLogicError);

  while (IsOpen()) {
    OpalTransport * transport = Accept(PMaxTimeInterval);
    if (transport == NULL)
      acceptHandler(*this, 0);
    else {
      switch (threadMode) {
        case SpawnNewThreadMode :
          transport->AttachThread(PThread::Create(acceptHandler,
                                                  (INT)transport,
                                                  PThread::NoAutoDeleteThread,
                                                  PThread::NormalPriority,
                                                  "Opal Answer"));
          break;

        case HandOffThreadMode :
          transport->AttachThread(&thread);
          this->thread = NULL;
          // Then do next case

        case SingleThreadMode :
          acceptHandler(*this, (INT)transport);
      }
      // Note: acceptHandler is responsible for deletion of the transport
    }
  }
}


PBoolean OpalListener::StartThread(const PNotifier & theAcceptHandler, ThreadMode mode)
{
  acceptHandler = theAcceptHandler;
  threadMode = mode;

  thread = PThread::Create(PCREATE_NOTIFIER(ListenForConnections), "YY Listener");

  return thread != NULL;
}


//////////////////////////////////////////////////////////////////////////

OpalListenerIP::OpalListenerIP(OpalEndPoint & ep,
                               PIPSocket::Address binding,
                               WORD port,
                               PBoolean exclusive)
  : OpalListener(ep),
    localAddress(binding)
{
  listenerPort = port;
  exclusiveListener = exclusive;
}


OpalListenerIP::OpalListenerIP(OpalEndPoint & endpoint,
                               const OpalTransportAddress & binding,
                               OpalTransportAddress::BindOptions option)
  : OpalListener(endpoint)
{
  OpalInternalIPTransport::GetAdjustedIpAndPort(binding, endpoint, option, localAddress, listenerPort, exclusiveListener);
}


OpalTransportAddress OpalListenerIP::GetLocalAddress(const OpalTransportAddress & remoteAddress) const
{
  PIPSocket::Address localIP = localAddress;

  PIPSocket::Address remoteIP;
  if (remoteAddress.GetIpAddress(remoteIP)) {
    OpalManager & manager = endpoint.GetManager();
    PNatMethod * natMethod = manager.GetNatMethod(remoteIP);
    if (natMethod != NULL) {
      if (localIP.IsAny())
        natMethod->GetInterfaceAddress(localIP);
      manager.TranslateIPAddress(localIP, remoteIP);
    }
  }

  return OpalTransportAddress(localIP, listenerPort, GetProtoPrefix());
}


//////////////////////////////////////////////////////////////////////////

OpalListenerTCP::OpalListenerTCP(OpalEndPoint & ep,
                                 PIPSocket::Address binding,
                                 WORD port,
                                 PBoolean exclusive)
  : OpalListenerIP(ep, binding, port, exclusive)
{
}


OpalListenerTCP::OpalListenerTCP(OpalEndPoint & endpoint,
                                 const OpalTransportAddress & binding,
                                 OpalTransportAddress::BindOptions option)
  : OpalListenerIP(endpoint, binding, option)
{
}


OpalListenerTCP::~OpalListenerTCP()
{
  CloseWait();
}


PBoolean OpalListenerTCP::Open(const PNotifier & theAcceptHandler, ThreadMode mode)
{
  if (listenerPort == 0) {
    OpalManager & manager = endpoint.GetManager();
    listenerPort = manager.GetNextTCPPort();
    WORD firstPort = listenerPort;
    while (!listener.Listen(localAddress, 1, listenerPort)) {
      listenerPort = manager.GetNextTCPPort();
      if (listenerPort == firstPort) {
        PTRACE(1, "Listen\tOpen on " << localAddress << " failed: " << listener.GetErrorText());
        break;
      }
    }
    listenerPort = listener.GetPort();
    return StartThread(theAcceptHandler, mode);
  }

  if (listener.Listen(localAddress, 10, listenerPort, exclusiveListener ? PSocket::AddressIsExclusive : PSocket::CanReuseAddress))
    return StartThread(theAcceptHandler, mode);

  PTRACE(1, "Listen\tOpen (" << (exclusiveListener ? "EXCLUSIVE" : "REUSEADDR") << ") on "
         << localAddress.AsString(true) << ':' << listener.GetPort()
         << " failed: " << listener.GetErrorText());
  return false;
}


PBoolean OpalListenerTCP::IsOpen()
{
  return listener.IsOpen();
}


void OpalListenerTCP::Close()
{
  listener.Close();
}


OpalTransport * OpalListenerTCP::Accept(const PTimeInterval & timeout)
{
  if (!listener.IsOpen())
    return NULL;

  listener.SetReadTimeout(timeout); // Wait for remote connect

  PTRACE(4, "Listen\tWaiting on socket accept on " << GetLocalAddress());
  PTCPSocket * socket = new PTCPSocket;
  if (socket->Accept(listener)) {
    OpalTransportTCP * transport = new OpalTransportTCP(endpoint);
    if (transport->Open(socket))
      return transport;

    PTRACE(1, "Listen\tFailed to open transport, connection not started.");
    delete transport;
    return NULL;
  }
  else if (socket->GetErrorCode() != PChannel::Interrupted) {
    PTRACE(1, "Listen\tAccept error:" << socket->GetErrorText());
    listener.Close();
  }

  delete socket;
  return NULL;
}


OpalTransport * OpalListenerTCP::CreateTransport(const OpalTransportAddress & localAddress,
                                                 const OpalTransportAddress & remoteAddress) const
{
  OpalTransportAddress myLocalAddress = GetLocalAddress();
  if (myLocalAddress.IsCompatible(remoteAddress)) {
    if (!localAddress.IsEmpty())
      return localAddress.CreateTransport(endpoint, OpalTransportAddress::NoBinding);
#if OPAL_PTLIB_SSL
    if (remoteAddress.NumCompare(TcpsPrefix) == EqualTo)
      return new OpalTransportTCPS(endpoint);
#endif
    return new OpalTransportTCP(endpoint);
  }
  return NULL;
}


const char * OpalListenerTCP::GetProtoPrefix() const
{
  return TcpPrefix;
}


//////////////////////////////////////////////////////////////////////////

OpalListenerUDP::OpalListenerUDP(OpalEndPoint & endpoint,
                                 PIPSocket::Address binding,
                                 WORD port,
                                 PBoolean exclusive)
  : OpalListenerIP(endpoint, binding, port, exclusive),
    listenerBundle(PMonitoredSockets::Create(binding.AsString(), !exclusive, endpoint.GetManager().GetNatMethod()))
{
}


OpalListenerUDP::OpalListenerUDP(OpalEndPoint & endpoint,
                                 const OpalTransportAddress & binding,
                                 OpalTransportAddress::BindOptions option)
  : OpalListenerIP(endpoint, binding, option),
	listenerBundle(PMonitoredSockets::Create(binding.GetHostName(), !exclusiveListener, endpoint.GetManager().GetNatMethod()  ))
{
	if ( !PIsDescendant(&endpoint.GetManager(),XMOpalManager) )
		PAssertNULL(0);
}


OpalListenerUDP::~OpalListenerUDP()
{
  CloseWait();
}


PBoolean OpalListenerUDP::Open(const PNotifier & theAcceptHandler, ThreadMode /*mode*/)
{
  if (listenerBundle->Open(listenerPort) && StartThread(theAcceptHandler, SingleThreadMode)) {
    /* UDP packets need to be handled. Not so much at high speed, but must not be
       significantly delayed by media threads which are running at HighPriority.
       This, for example, helps make sure that a SIP BYE is received and processed
       to kill a call where codecs etc in the media threads are hogging all the CPU. */
    thread->SetPriority(PThread::HighestPriority);
    return true;
  }

  PTRACE(1, "Listen\tCould not start any UDP listeners");
  return PFalse;
}


PBoolean OpalListenerUDP::IsOpen()
{
  return listenerBundle != NULL && listenerBundle->IsOpen();
}


void OpalListenerUDP::Close()
{
  if (listenerBundle != NULL)
    listenerBundle->Close();
}


OpalTransport * OpalListenerUDP::Accept(const PTimeInterval & timeout)
{
  if (!IsOpen())
    return NULL;

  PBYTEArray pdu;
  PIPSocket::Address remoteAddr;
  WORD remotePort;
  PString iface;
  PINDEX readCount;
  static const PINDEX SixtyFourK = 0x10000;
  switch (listenerBundle->ReadFromBundle(pdu.GetPointer(SixtyFourK), SixtyFourK, remoteAddr, remotePort, iface, readCount, timeout)) {
    case PChannel::NoError :
      pdu.SetSize(readCount);
      return new OpalTransportUDP(endpoint, pdu, listenerBundle, iface, remoteAddr, remotePort);

    case PChannel::Interrupted :
      PTRACE(4, "Listen\tInterfaces changed");
      break;

    default :
      PTRACE(1, "Listen\tUDP read error.");
  }

  return NULL;
}


OpalTransport * OpalListenerUDP::CreateTransport(const OpalTransportAddress & localAddress,
                                                 const OpalTransportAddress & remoteAddress) const
{
  if (!GetLocalAddress().IsCompatible(remoteAddress))
    return NULL;

  PIPSocket::Address addr,ipRemote ;
  if (remoteAddress.GetIpAddress(addr) && addr.IsLoopback())
    return new OpalTransportUDP(endpoint, addr);

  PString iface;
  if (localAddress.GetIpAddress(addr))
    iface = addr.AsString(true);

  WORD portRemote=0;
  remoteAddress.GetIpAndPort(ipRemote, portRemote);//chenyuan 
  
  return new OpalTransportUDP(endpoint, PBYTEArray(), listenerBundle, iface, PIPSocket::GetDefaultIpAny(), portRemote);
}


const char * OpalListenerUDP::GetProtoPrefix() const
{
  return UdpPrefix;
}


OpalTransportAddress OpalListenerUDP::GetLocalAddress(const OpalTransportAddress & remoteAddress) const
{
  PIPSocket::Address localIP = PIPSocket::GetDefaultIpAny();
  WORD port = listenerPort;

  PIPSocket::Address remoteIP;
  if (remoteAddress.GetIpAddress(remoteIP)) {
    PNatMethod * natMethod = endpoint.GetManager().GetNatMethod(remoteIP);
    if (natMethod != NULL) {
      natMethod->GetInterfaceAddress(localIP);
      listenerBundle->GetAddress(localIP.AsString(), localIP, port, true);
    }
  }

  if (localIP.IsAny())
    listenerBundle->GetAddress(PString::Empty(), localIP, port, false);

  return OpalTransportAddress(localIP, port, GetProtoPrefix());
}


//////////////////////////////////////////////////////////////////////////

OpalTransport::OpalTransport(OpalEndPoint & end)
  : endpoint(end)
{
  thread = NULL;
}


OpalTransport::~OpalTransport()
{
  PAssert(thread == NULL, PLogicError);
}


void OpalTransport::PrintOn(ostream & strm) const
{
  strm << GetRemoteAddress() << "<if=" << GetLocalAddress() << '>';
}


PString OpalTransport::GetInterface() const
{
  return GetLocalAddress().GetHostName();
}


bool OpalTransport::SetInterface(const PString &)
{
  return true;
}


PBoolean OpalTransport::Close()
{
  PTRACE(4, "Opal\tTransport Close");

  /* Do not use PIndirectChannel::Close() as this deletes the sub-channel
     member field crashing the background thread. Just close the base
     sub-channel so breaks the threads I/O block.
   */
  if (IsOpen())
    return GetBaseWriteChannel()->Close();

  return PTrue;
}


void OpalTransport::CloseWait()
{
  PTRACE(3, "Opal\tTransport clean up on termination");

  Close();

  channelPointerMutex.StartWrite();
  PThread * exitingThread = thread;
  thread = NULL;
  channelPointerMutex.EndWrite();

  if (exitingThread != NULL) {
    if (exitingThread == PThread::Current())
      exitingThread->SetAutoDelete();
    else {
      PAssert(exitingThread->WaitForTermination(10000), "Transport thread did not terminate");
      delete exitingThread;
    }
  }
}


PBoolean OpalTransport::IsCompatibleTransport(const OpalTransportAddress &) const
{
  PAssertAlways(PUnimplementedFunction);
  return PFalse;
}


void OpalTransport::SetPromiscuous(PromisciousModes /*promiscuous*/)
{
}


OpalTransportAddress OpalTransport::GetLastReceivedAddress() const
{
  return GetRemoteAddress();
}


PString OpalTransport::GetLastReceivedInterface() const
{
  return GetLocalAddress();
}


PBoolean OpalTransport::WriteConnect(WriteConnectCallback function, void * userData)
{
  return function(*this, userData);
}


void OpalTransport::AttachThread(PThread * thrd)
{
  if (thread != NULL) {
    PAssert(thread->WaitForTermination(10000), "Transport not terminated when reattaching thread");
    delete thread;
  }

  thread = thrd;
}


PBoolean OpalTransport::IsRunning() const
{
  if (thread == NULL)
    return PFalse;

  return !thread->IsTerminated();
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportIP::OpalTransportIP(OpalEndPoint & end,
                                 PIPSocket::Address binding,
                                 WORD port)
  : OpalTransport(end),
    localAddress(binding),
    remoteAddress(PIPSocket::Address::GetAny(binding.GetVersion()))
{
  localPort = port;
  remotePort = 0;
}


OpalTransportAddress OpalTransportIP::GetLocalAddress(bool /*allowNAT*/) const
{
  return OpalTransportAddress(localAddress, localPort, GetProtoPrefix());
}


PBoolean OpalTransportIP::SetLocalAddress(const OpalTransportAddress & newLocalAddress)
{
  if (!IsCompatibleTransport(newLocalAddress))
    return PFalse;

  if (!IsOpen())
    return newLocalAddress.GetIpAndPort(localAddress, localPort);

  PIPSocket::Address address;
  WORD port = 0;
  if (!newLocalAddress.GetIpAndPort(address, port))
    return PFalse;

  return localAddress == address && localPort == port;
}


OpalTransportAddress OpalTransportIP::GetRemoteAddress() const
{
  return OpalTransportAddress(remoteAddress, remotePort, GetProtoPrefix());
}


PBoolean OpalTransportIP::SetRemoteAddress(const OpalTransportAddress & address)
{
  if (IsCompatibleTransport(address))
    return address.GetIpAndPort(remoteAddress, remotePort);

  PTRACE(2, "OpalIP\tAttempt to set incompatible transport " << address);
  return PFalse;
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportTCP::OpalTransportTCP(OpalEndPoint & ep,
                                   PIPSocket::Address binding,
                                   WORD port,
                                   PBoolean reuseAddr)
  : OpalTransportIP(ep, binding, port)
{
  reuseAddressFlag = reuseAddr;
}


OpalTransportTCP::OpalTransportTCP(OpalEndPoint & ep, PTCPSocket * socket)
  : OpalTransportIP(ep, INADDR_ANY, 0)
{
  Open(socket);
}


OpalTransportTCP::~OpalTransportTCP()
{
  CloseWait();
  PTRACE(4,"Opal\tDeleted transport " << *this);
}


PBoolean OpalTransportTCP::IsReliable() const
{
  return PTrue;
}


PBoolean OpalTransportTCP::IsCompatibleTransport(const OpalTransportAddress & address) const
{
  return (address.NumCompare(TcpPrefix) == EqualTo) ||
         (address.NumCompare(IpPrefix)  == EqualTo);
}


PBoolean OpalTransportTCP::Connect()
{
  if (IsOpen())
    return PTrue;

  PTCPSocket * socket = new PTCPSocket(remotePort);
  Open(socket);

  PReadWaitAndSignal mutex(channelPointerMutex);

  socket->SetReadTimeout(10000);

  OpalManager & manager = endpoint.GetManager();
  localPort = manager.GetNextTCPPort();
  WORD firstPort = localPort;
  for (;;) {
    PTRACE(4, "OpalTCP\tConnecting to "
           << remoteAddress.AsString(true) << ':' << remotePort
           << " (local port=" << localPort << ')');
    if (socket->Connect(localPort, remoteAddress))
      break;

    int errnum = socket->GetErrorNumber();
    if (localPort == 0 || (errnum != EADDRINUSE && errnum != EADDRNOTAVAIL)) {
      PTRACE(1, "OpalTCP\tCould not connect to "
                << remoteAddress.AsString(true) << ':' << remotePort
                << " (local port=" << localPort << ") - "
                << socket->GetErrorText() << '(' << errnum << ')');
      return SetErrorValues(socket->GetErrorCode(), errnum);
    }

    localPort = manager.GetNextTCPPort();
    if (localPort == firstPort) {
      PTRACE(1, "OpalTCP\tCould not bind to any port in range " <<
                manager.GetTCPPortBase() << " to " << manager.GetTCPPortMax());
      return SetErrorValues(socket->GetErrorCode(), errnum);
    }
  }

  socket->SetReadTimeout(PMaxTimeInterval);

  return OnOpen();
}


PBoolean OpalTransportTCP::ReadPDU(PBYTEArray & pdu)
{
  // Make sure is a RFC1006 TPKT
  switch (ReadChar()) {
    case 3 :  // Only support version 3
      break;

    default :  // Unknown version number
      SetErrorValues(ProtocolFailure, 0x80000000);
      // Do case for read error

    case -1 :
      return PFalse;
  }

  // Save timeout
  PTimeInterval oldTimeout = GetReadTimeout();

  // Should get all of PDU in 5 seconds or something is seriously wrong,
  SetReadTimeout(5000);

  // Get TPKT length
  BYTE header[3];
  PBoolean ok = ReadBlock(header, sizeof(header));
  if (ok) {
    PINDEX packetLength = ((header[1] << 8)|header[2]);
    if (packetLength < 4) {
      PTRACE(2, "H323TCP\tDwarf PDU received (length " << packetLength << ")");
      ok = PFalse;
    } else {
      packetLength -= 4;
      ok = ReadBlock(pdu.GetPointer(packetLength), packetLength);
    }
  }

  SetReadTimeout(oldTimeout);

  return ok;
}


PBoolean OpalTransportTCP::WritePDU(const PBYTEArray & pdu)
{
  // We copy the data into a new buffer so we can do a single write call. This
  // is necessary as we have disabled the Nagle TCP delay algorithm to improve
  // network performance.

  int packetLength = pdu.GetSize() + 4;

  // Send RFC1006 TPKT length
  PBYTEArray tpkt(packetLength);
  tpkt[0] = 3;
  tpkt[1] = 0;
  tpkt[2] = (BYTE)(packetLength >> 8);
  tpkt[3] = (BYTE)packetLength;
  memcpy(tpkt.GetPointer()+4, (const BYTE *)pdu, pdu.GetSize());

  return Write((const BYTE *)tpkt, packetLength);
}


PBoolean OpalTransportTCP::OnOpen()
{
  PIPSocket * socket = (PIPSocket *)GetReadChannel();

  // Get name of the remote computer for information purposes
  if (!socket->GetPeerAddress(remoteAddress, remotePort)) {
    PTRACE(1, "OpalTCP\tGetPeerAddress() failed: " << socket->GetErrorText());
    return PFalse;
  }

  // get local address of incoming socket to ensure that multi-homed machines
  // use a NIC address that is guaranteed to be addressable to destination
  if (!socket->GetLocalAddress(localAddress, localPort)) {
    PTRACE(1, "OpalTCP\tGetLocalAddress() failed: " << socket->GetErrorText());
    return PFalse;
  }

#ifndef __BEOS__
  if (!socket->SetOption(TCP_NODELAY, 1, IPPROTO_TCP)) {
    PTRACE(1, "OpalTCP\tSetOption(TCP_NODELAY) failed: " << socket->GetErrorText());
  }

  // make sure do not lose outgoing packets on close
  const linger ling = { 1, 3 };
  if (!socket->SetOption(SO_LINGER, &ling, sizeof(ling))) {
    PTRACE(1, "OpalTCP\tSetOption(SO_LINGER) failed: " << socket->GetErrorText());
    return PFalse;
  }
#endif

  PTRACE(3, "OpalTCP\tStarted connection to "
         << remoteAddress.AsString(true) << ':' << remotePort
         << " (if=" << localAddress.AsString(true) << ':' << localPort << ')');

  return PTrue;
}


const char * OpalTransportTCP::GetProtoPrefix() const
{
  return TcpPrefix;
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportUDP::OpalTransportUDP(OpalEndPoint & ep,
                                   PIPSocket::Address binding,
                                   WORD localPort,
                                   bool reuseAddr,
                                   bool preOpen)
  : OpalTransportIP(ep, binding, localPort)
  , manager(ep.GetManager())
{
  PMonitoredSockets * sockets = PMonitoredSockets::Create(binding.AsString(), reuseAddr, manager.GetNatMethod());
  if (preOpen)
    sockets->Open(localPort);
  Open(new PMonitoredSocketChannel(sockets, PFalse));
}


OpalTransportUDP::OpalTransportUDP(OpalEndPoint & ep,
                                   const PBYTEArray & packet,
                                   const PMonitoredSocketsPtr & listener,
                                   const PString & iface,
                                   PIPSocket::Address remAddr,
                                   WORD remPort)
  : OpalTransportIP(ep, PIPSocket::GetDefaultIpAny(), 0)
  , manager(ep.GetManager())
  , preReadPacket(packet)
{
  remoteAddress = remAddr;
  remotePort = remPort;

  PMonitoredSocketChannel * socket = new PMonitoredSocketChannel(listener, PTrue);
  socket->SetRemote(remAddr, remPort);
  socket->SetInterface(iface);
  socket->GetLocal(localAddress, localPort, !manager.IsLocalAddress(remoteAddress));
  Open(socket);

  PTRACE(3, "OpalUDP\tBinding to interface: " << localAddress.AsString(true) << ':' << localPort);
}


OpalTransportUDP::~OpalTransportUDP()
{
  CloseWait();
  PTRACE(4,"Opal\tDeleted transport " << *this);
}


PBoolean OpalTransportUDP::IsReliable() const
{
  return PFalse;
}


PBoolean OpalTransportUDP::IsCompatibleTransport(const OpalTransportAddress & address) const
{
  return (address.NumCompare(UdpPrefix) == EqualTo) ||
         (address.NumCompare(IpPrefix)  == EqualTo);
}


PBoolean OpalTransportUDP::Connect()
{	
  if (remotePort == 0)
    return PFalse;

  if (remoteAddress.IsAny() || remoteAddress.IsBroadcast()) {
	  remoteAddress = PIPSocket::Address::GetBroadcast(remoteAddress.GetVersion());
    PTRACE(3, "OpalUDP\tBroadcast connect to port " << remotePort);
  }
  else {
    PTRACE(3, "OpalUDP\tStarted connect to " << remoteAddress.AsString(true) << ':' << remotePort);
  }

  if (PAssertNULL(writeChannel) == NULL)
    return PFalse;

  PMonitoredSocketsPtr bundle = ((PMonitoredSocketChannel *)writeChannel)->GetMonitoredSockets();
  if (bundle->IsOpen())
    return PTrue;

  OpalManager & manager = endpoint.GetManager();

  localPort = manager.GetNextUDPPort();
  WORD firstPort = localPort;
  while (!bundle->Open(localPort)) {
    localPort = manager.GetNextUDPPort();
    if (localPort == firstPort) {
      PTRACE(1, "OpalUDP\tCould not bind to any port in range " <<
	      manager.GetUDPPortBase() << " to " << manager.GetUDPPortMax());
      return PFalse;
    }
  }

  return PTrue;
}


PString OpalTransportUDP::GetInterface() const
{
  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
  if (socket != NULL)
    return socket->GetInterface();

  return OpalTransportIP::GetInterface();
}


bool OpalTransportUDP::SetInterface(const PString & iface)
{
  PTRACE(3, "OpalUDP\tSetting interface to " << iface);

  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
  if (socket == NULL)
    return false;
    
  socket->SetInterface(iface);
  return true;
}


OpalTransportAddress OpalTransportUDP::GetLocalAddress(bool allowNAT) const
{
  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
  if (socket != NULL) {
    OpalTransportUDP * thisWritable = const_cast<OpalTransportUDP *>(this);
    socket->GetLocal(thisWritable->localAddress, thisWritable->localPort, allowNAT && !manager.IsLocalAddress(remoteAddress));
  }

  return OpalTransportIP::GetLocalAddress(allowNAT);
}


PBoolean OpalTransportUDP::SetLocalAddress(const OpalTransportAddress & newLocalAddress)
{
  if (OpalTransportIP::GetLocalAddress().IsEquivalent(newLocalAddress))
    return PTrue;

  if (!IsCompatibleTransport(newLocalAddress))
    return PFalse;

  if (!newLocalAddress.GetIpAndPort(localAddress, localPort))
    return PFalse;

  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
  if (socket != NULL)
    socket->GetMonitoredSockets()->Open(localPort);

  return OpalTransportIP::SetLocalAddress(newLocalAddress);
}


PBoolean OpalTransportUDP::SetRemoteAddress(const OpalTransportAddress & address)
{
  if (!OpalTransportIP::SetRemoteAddress(address))
    return PFalse;

  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
  if (socket != NULL)
    socket->SetRemote(remoteAddress, remotePort);

  return PTrue;
}


void OpalTransportUDP::SetPromiscuous(PromisciousModes promiscuous)
{
  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
  if (socket != NULL) {
    socket->SetPromiscuous(promiscuous != AcceptFromRemoteOnly);
    if (promiscuous == AcceptFromAnyAutoSet)
      socket->SetRemote(PIPSocket::GetDefaultIpAny(), 0);
  }
}


OpalTransportAddress OpalTransportUDP::GetLastReceivedAddress() const
{
  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
  if (socket != NULL) {
    PIPSocket::Address addr;
    WORD port;
    socket->GetLastReceived(addr, port);
    if (!addr.IsAny() && port != 0)
      return OpalTransportAddress(addr, port, UdpPrefix);
  }

  return OpalTransport::GetLastReceivedAddress();
}


PString OpalTransportUDP::GetLastReceivedInterface() const
{
  PString iface;

  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
  if (socket != NULL)
    iface = socket->GetLastReceivedInterface();

  if (iface.IsEmpty())
    iface = OpalTransport::GetLastReceivedInterface();

  return iface;
}


PBoolean OpalTransportUDP::Read(void * buffer, PINDEX length)
{
  if (preReadPacket.IsEmpty())
    return OpalTransportIP::Read(buffer, length);

  lastReadCount = PMIN(length, preReadPacket.GetSize());
  memcpy(buffer, preReadPacket, lastReadCount);
  preReadPacket.SetSize(0);

  return PTrue;
}


PBoolean OpalTransportUDP::ReadPDU(PBYTEArray & packet)
{
  if (preReadPacket.GetSize() > 0) {
    packet = preReadPacket;
    preReadPacket.SetSize(0);
    return PTrue;
  }

  if (!Read(packet.GetPointer(10000), 10000)) {
    packet.SetSize(0);
    return PFalse;
  }

  packet.SetSize(GetLastReadCount());
  return PTrue;
}


PBoolean OpalTransportUDP::WritePDU(const PBYTEArray & packet)
{
  return Write((const BYTE *)packet, packet.GetSize());
}


PBoolean OpalTransportUDP::WriteConnect(WriteConnectCallback function, void * userData)
{
  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)writeChannel;
  if (socket == NULL)
    return PFalse;

  PMonitoredSocketsPtr bundle = socket->GetMonitoredSockets();
  PIPSocket::Address address;
  GetRemoteAddress().GetIpAddress(address);
  PStringArray interfaces = bundle->GetInterfaces(PFalse, address);

  PBoolean ok = PFalse;
  for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
    PTRACE(4, "OpalUDP\tWriting to interface " << i << " - \"" << interfaces[i] << '"');
    socket->SetInterface(interfaces[i]);
    if (function(*this, userData))
      ok = PTrue;
  }

  return ok;
}


const char * OpalTransportUDP::GetProtoPrefix() const
{
  return UdpPrefix;
}


//////////////////////////////////////////////////////////////////////////

#if OPAL_PTLIB_SSL

static PBoolean SetSSLCertificate(PSSLContext & sslContext,
                             const PFilePath & certificateFile,
                                        PBoolean create,
                                   const char * dn = NULL)
{
  if (create && !PFile::Exists(certificateFile)) {
    PSSLPrivateKey key(1024);
    PSSLCertificate certificate;
    PStringStream name;
    if (dn != NULL)
      name << dn;
    else {
      name << "/O=" << PProcess::Current().GetManufacturer()
           << "/CN=" << PProcess::Current().GetName() << '@' << PIPSocket::GetHostName();
    }
    if (!certificate.CreateRoot(name, key)) {
      PTRACE(1, "MTGW\tCould not create certificate");
      return PFalse;
    }
    certificate.Save(certificateFile);
    key.Save(certificateFile, PTrue);
  }

  return sslContext.UseCertificate(certificateFile) &&
         sslContext.UsePrivateKey(certificateFile);
}

OpalTransportTCPS::OpalTransportTCPS(OpalEndPoint & ep,
                                     PIPSocket::Address binding,
                                     WORD port,
                                     PBoolean reuseAddr)
  : OpalTransportTCP(ep, binding, port, reuseAddr)
{
  sslContext = new PSSLContext(PSSLContext::TLSv1);
}


OpalTransportTCPS::OpalTransportTCPS(OpalEndPoint & ep, PTCPSocket * socket)
  : OpalTransportTCP(ep, PIPSocket::GetDefaultIpAny(), 0)
{
  sslContext = new PSSLContext(PSSLContext::TLSv1);
  PSSLChannel * sslChannel = new PSSLChannel(sslContext);
  if (!sslChannel->Open(socket))
    delete sslChannel;
  else
    Open(sslChannel);
}


OpalTransportTCPS::~OpalTransportTCPS()
{
  CloseWait();
  delete sslContext;
  PTRACE(4,"Opal\tDeleted transport " << *this);
}


PBoolean OpalTransportTCPS::IsCompatibleTransport(const OpalTransportAddress & address) const
{
  return OpalTransportTCP::IsCompatibleTransport(address) || address.NumCompare(TcpsPrefix) == EqualTo;
}


PBoolean OpalTransportTCPS::Connect()
{
  if (IsOpen())
    return PTrue;

  PTCPSocket * socket = new PTCPSocket(remotePort);

  PReadWaitAndSignal mutex(channelPointerMutex);

  socket->SetReadTimeout(10000);

  OpalManager & manager = endpoint.GetManager();
  localPort = manager.GetNextTCPPort();
  WORD firstPort = localPort;
  for (;;) {
    PTRACE(4, "OpalTCPS\tConnecting to "
           << remoteAddress.AsString(true) << ':' << remotePort
           << " (local port=" << localPort << ')');
    if (socket->Connect(localPort, remoteAddress))
      break;

    int errnum = socket->GetErrorNumber();
    if (localPort == 0 || (errnum != EADDRINUSE && errnum != EADDRNOTAVAIL)) {
      PTRACE(1, "OpalTCPS\tCould not connect to "
                << remoteAddress.AsString(true) << ':' << remotePort
                << " (local port=" << localPort << ") - "
                << socket->GetErrorText() << '(' << errnum << ')');
      return SetErrorValues(socket->GetErrorCode(), errnum);
    }

    localPort = manager.GetNextTCPPort();
    if (localPort == firstPort) {
      PTRACE(1, "OpalTCP\tCould not bind to any port in range " <<
                manager.GetTCPPortBase() << " to " << manager.GetTCPPortMax());
      return SetErrorValues(socket->GetErrorCode(), errnum);
    }
  }

  socket->SetReadTimeout(PMaxTimeInterval);

  PString certificateFile = endpoint.GetSSLCertificate();
  if (!SetSSLCertificate(*sslContext, certificateFile, PTrue)) {
    PTRACE(1, "OpalTCPS\tCould not load certificate \"" << certificateFile << '"');
    return PFalse;
  }

  PSSLChannel * sslChannel = new PSSLChannel(sslContext);
  if (!sslChannel->Connect(socket)) {
    delete sslChannel;
    return PFalse;
  }

  return Open(sslChannel);
}

PBoolean OpalTransportTCPS::OnOpen()
{
  PSSLChannel * sslChannel = dynamic_cast<PSSLChannel *>(GetReadChannel());
  if (sslChannel == NULL)
    return PFalse;

  PIPSocket * socket = dynamic_cast<PIPSocket *>(sslChannel->GetReadChannel());

  // Get name of the remote computer for information purposes
  if (!socket->GetPeerAddress(remoteAddress, remotePort)) {
    PTRACE(1, "OpalTCPS\tGetPeerAddress() failed: " << socket->GetErrorText());
    return PFalse;
  }

  // get local address of incoming socket to ensure that multi-homed machines
  // use a NIC address that is guaranteed to be addressable to destination
  if (!socket->GetLocalAddress(localAddress, localPort)) {
    PTRACE(1, "OpalTCPS\tGetLocalAddress() failed: " << socket->GetErrorText());
    return PFalse;
  }

#ifndef __BEOS__
  if (!socket->SetOption(TCP_NODELAY, 1, IPPROTO_TCP)) {
    PTRACE(1, "OpalTCPS\tSetOption(TCP_NODELAY) failed: " << socket->GetErrorText());
  }

  // make sure do not lose outgoing packets on close
  const linger ling = { 1, 3 };
  if (!socket->SetOption(SO_LINGER, &ling, sizeof(ling))) {
    PTRACE(1, "OpalTCP\tSetOption(SO_LINGER) failed: " << socket->GetErrorText());
    return PFalse;
  }
#endif

  PTRACE(3, "OpalTCPS\tStarted connection to "
         << remoteAddress.AsString(true) << ':' << remotePort
         << " (if=" << localAddress.AsString(true) << ':' << localPort << ')');

  return PTrue;
}


const char * OpalTransportTCPS::GetProtoPrefix() const
{
  return TcpsPrefix;
}

//////////////////////////////////////////////////////////////////////////

OpalListenerTCPS::OpalListenerTCPS(OpalEndPoint & ep,
                                 PIPSocket::Address binding,
                                 WORD port,
                                 PBoolean exclusive)
  : OpalListenerTCP(ep, binding, port, exclusive)
{
  Construct();
}


OpalListenerTCPS::OpalListenerTCPS(OpalEndPoint & ep,
                                    const OpalTransportAddress & binding,
                                    OpalTransportAddress::BindOptions option)
  : OpalListenerTCP(ep, binding, option)
{
  Construct();
}


OpalListenerTCPS::~OpalListenerTCPS()
{
  delete sslContext;
}


void OpalListenerTCPS::Construct()
{
  sslContext = new PSSLContext();

  PString certificateFile = endpoint.GetSSLCertificate();
  if (!SetSSLCertificate(*sslContext, certificateFile, PTrue)) {
    PTRACE(1, "OpalTCPS\tCould not load certificate \"" << certificateFile << '"');
  }
}


OpalTransport * OpalListenerTCPS::Accept(const PTimeInterval & timeout)
{
  if (!listener.IsOpen())
    return NULL;

  listener.SetReadTimeout(timeout); // Wait for remote connect

  PTRACE(4, "TCPS\tWaiting on socket accept on " << GetLocalAddress());
  PTCPSocket * socket = new PTCPSocket;
  if (!socket->Accept(listener)) {
    if (socket->GetErrorCode() != PChannel::Interrupted) {
      PTRACE(1, "Listen\tAccept error:" << socket->GetErrorText());
      listener.Close();
    }
    delete socket;
    return NULL;
  }

  OpalTransportTCPS * transport = new OpalTransportTCPS(endpoint);
  PSSLChannel * ssl = new PSSLChannel(sslContext);
  if (!ssl->Accept(socket)) {
    PTRACE(1, "TCPS\tAccept failed: " << ssl->GetErrorText());
    delete transport;
    delete ssl;
    delete socket;
    return NULL;
  }

  if (transport->Open(ssl))
    return transport;

  PTRACE(1, "TCPS\tFailed to open transport, connection not started.");
  delete transport;
  delete ssl;
  delete socket;
  return NULL;
}

const char * OpalListenerTCPS::GetProtoPrefix() const
{
  return TcpsPrefix;
}


#endif

///////////////////////////////////////////////////////////////////////////////
