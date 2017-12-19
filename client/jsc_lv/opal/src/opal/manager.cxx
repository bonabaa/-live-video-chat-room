/*
 * manager.cxx
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
 * $Revision: 23930 $
 * $Author: rjongbloed $
 * $Date: 2010-01-13 06:50:09 +0000 (Wed, 13 Jan 2010) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "manager.h"
#endif

#include <opal/buildopts.h>

#include <opal/manager.h>
#include <opal/endpoint.h>
#include <opal/call.h>
#include <opal/patch.h>
#include <opal/mediastrm.h>
#include <codec/g711codec.h>
#include <codec/vidcodec.h>
#include <codec/rfc4175.h>
#include <codec/opalpluginmgr.h>

#if OPAL_HAS_H224
#include <h224/h224.h>
#endif

#include <ptclib/random.h>
#include <ptclib/url.h>

#include "../../version.h"


static const char * const DefaultMediaFormatOrder[] = {
#ifdef _WIN32
  YYCODE_CORE,
#endif
  OPAL_GSMAMR,
  OPAL_G7231_6k3,
  OPAL_G729B,
  OPAL_G729AB,
  OPAL_G729,
  OPAL_G729A,
  OPAL_iLBC,
  OPAL_GSM0610,
  OPAL_G728,
  OPAL_G711_ULAW_64K,
  OPAL_G711_ALAW_64K,
  "CELT-32K",
  OPAL_H264,
  OPAL_MPEG4,
  OPAL_H263,
	"H263P",
  OPAL_H261
};

// G.711 is *always* available
// Yes, it would make more sense for this to be in g711codec.cxx, but on 
// Linux it would not get loaded due to static initialisation optimisation
OPAL_REGISTER_G711();

// Same deal for RC4175 video
#if OPAL_RFC4175
OPAL_REGISTER_RFC4175();
#endif


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

PString OpalGetVersion()
{
#define AlphaCode   "alpha"
#define BetaCode    "beta"
#define ReleaseCode "."

  return psprintf("%u.%u%s%u", MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER);
}


unsigned OpalGetMajorVersion()
{
  return MAJOR_VERSION;
}

unsigned OpalGetMinorVersion()
{
  return MINOR_VERSION;
}

unsigned OpalGetBuildNumber()
{
  return BUILD_NUMBER;
}


OpalProductInfo::OpalProductInfo()
  : vendor(PProcess::Current().GetManufacturer())
  , name(PProcess::Current().GetName())
  , version(PProcess::Current().GetVersion())
  , t35CountryCode(9)     // Country code for Australia
  , t35Extension(0)       // No extension code for Australia
  , manufacturerCode(61)  // Allocated by Australian Communications Authority, Oct 2000;
{
  // Sanitise the product name to be compatible with SIP User-Agent rules
  name.Replace(' ', '-', true);
  PINDEX pos;
  while ((pos = name.FindSpan("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.!%*_+`'~")) != P_MAX_INDEX)
    name.Delete(pos, 1);
}

OpalProductInfo & OpalProductInfo::Default()
{
  static OpalProductInfo instance;
  return instance;
}


PCaselessString OpalProductInfo::AsString() const
{
  PStringStream str;
  str << name << '\t' << version << '\t';
  if (t35CountryCode != 0 && manufacturerCode != 0) {
    str << (unsigned)t35CountryCode;
    if (t35Extension != 0)
      str << '.' << (unsigned)t35Extension;
    str << '/' << manufacturerCode;
  }
  str << '\t' << vendor;
  return str;
}


/////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

OpalManager::OpalManager()
  : defaultUserName(PProcess::Current().GetUserName())
  , defaultDisplayName(defaultUserName)
  , rtpIpTypeofService(0xb8)  // New DiffServ value for Expidited Forwarding as per RFC3246
  , rtpPayloadSizeMax(576-20-16-12) // Max safe MTU size (576 bytes as per RFC879) minus IP, UDP an RTP headers
  , rtpPacketSizeMax(2048)
  , minAudioJitterDelay(50)  // milliseconds
  , maxAudioJitterDelay(250) // milliseconds
  , mediaFormatOrder(PARRAYSIZE(DefaultMediaFormatOrder), DefaultMediaFormatOrder)
  , disableDetectInBandDTMF(false)
  , noMediaTimeout(0, 0, 5)     // Minutes
  , translationAddress(0)       // Invalid address to disable
  , stun(NULL)
  , interfaceMonitor(NULL)
  , activeCalls(*this)
#ifdef OPAL_ZRTP
  , zrtpEnabled(false)
#endif
{
  rtpIpPorts.current = rtpIpPorts.base = 5000;
  rtpIpPorts.max = 5999;

  // use dynamic port allocation by default
  tcpPorts.current = tcpPorts.base = tcpPorts.max = 0;
  udpPorts.current = udpPorts.base = udpPorts.max = 0;

#if OPAL_VIDEO
  PStringArray devices = PVideoInputDevice::GetDriversDeviceNames("*"); // Get all devices on all drivers
  PINDEX i;
  for (i = 0; i < devices.GetSize(); ++i) {
    if ((devices[i] *= "*.yuv") || (devices[i] *= "fake")) 
      continue;
    videoInputDevice.deviceName = devices[i];
    break;
  }
  SetAutoStartTransmitVideo(!videoInputDevice.deviceName.IsEmpty());

  devices = PVideoOutputDevice::GetDriversDeviceNames("*"); // Get all devices on all drivers
  for (i = 0; i < devices.GetSize(); ++i) {
    if ((devices[i] *= "*.yuv") || (devices[i] *= "null"))
      continue;
    videoOutputDevice.deviceName = devices[i];
    videoPreviewDevice = videoOutputDevice;
    break;
  }
  SetAutoStartReceiveVideo(!videoOutputDevice.deviceName.IsEmpty());
#endif

  garbageCollector = PThread::Create(PCREATE_NOTIFIER(GarbageMain), "YYMM Garbage");

  PTRACE(4, "OpalMan\tCreated manager.");
}

#ifdef _MSC_VER
#pragma warning(default:4355)
#endif


OpalManager::~OpalManager()
{
  ShutDownEndpoints();

  // Shut down the cleaner thread
  garbageCollectExit.Signal();
  garbageCollector->WaitForTermination();

  // Clean up any calls that the cleaner thread missed on the way out
  GarbageCollection();

  delete garbageCollector;

  delete stun;
  delete interfaceMonitor;

  PTRACE(4, "OpalMan\tDeleted manager.");
}


PList<OpalEndPoint> OpalManager::GetEndPoints() const
{
  PList<OpalEndPoint> list;
  list.AllowDeleteObjects(false);

  PReadWaitAndSignal mutex(endpointsMutex);

  for (PList<OpalEndPoint>::const_iterator it = endpointList.begin(); it != endpointList.end(); ++it)
    list.Append((OpalEndPoint *)&*it);

  return list;
}


void OpalManager::ShutDownEndpoints()
{
  PTRACE(4, "OpalMan\tShutting down endpoints.");

  // Clear any pending calls, set flag so no calls can be received before endpoints removed
  InternalClearAllCalls(OpalConnection::EndedByLocalUser, true, m_clearingAllCallsCount++ == 0);

  // Remove (and unsubscribe) all the presentities
  m_presentities.RemoveAll();
  m_presentities.DeleteObjectsToBeRemoved();

  // Deregister the endpoints
  endpointsMutex.StartRead();
  for (PList<OpalEndPoint>::iterator ep = endpointList.begin(); ep != endpointList.end(); ++ep)
    ep->ShutDown();
  endpointsMutex.EndRead();

  endpointsMutex.StartWrite();
  endpointMap.clear();
  endpointList.RemoveAll();
  endpointsMutex.EndWrite();

  --m_clearingAllCallsCount; // Allow for endpoints to be added again.
}


void OpalManager::AttachEndPoint(OpalEndPoint * endpoint, const PString & prefix)
{
  if (PAssertNULL(endpoint) == NULL)
    return;

  PString thePrefix = prefix.IsEmpty() ? endpoint->GetPrefixName() : prefix;

  PWriteWaitAndSignal mutex(endpointsMutex);

  if (endpointMap.find(thePrefix) != endpointMap.end()) {
    PTRACE(1, "OpalMan\tCannot re-attach endpoint prefix " << thePrefix);
    return;
  }

  if (endpointList.GetObjectsIndex(endpoint) == P_MAX_INDEX)
    endpointList.Append(endpoint);
  endpointMap[thePrefix] = endpoint;
  PTRACE(3, "OpalMan\tAttached endpoint with prefix " << thePrefix);
}


void OpalManager::DetachEndPoint(OpalEndPoint * endpoint)
{ 
  if (PAssertNULL(endpoint) == NULL)
    return;

  endpoint->ShutDown();

  endpointsMutex.StartWrite();

  if (endpointList.Remove(endpoint)) {
    // Was in list, remove from map too
    std::map<PString, OpalEndPoint *>::iterator it = endpointMap.begin();
    while (it != endpointMap.end()) {
      if (it->second != endpoint)
        ++it;
      else {
        endpointMap.erase(it);
        it = endpointMap.begin();
      }
    }
  }

  endpointsMutex.EndWrite();
}


void OpalManager::DetachEndPoint(const PString & prefix)
{
  PReadWaitAndSignal mutex(endpointsMutex);

  std::map<PString, OpalEndPoint *>::iterator it = endpointMap.find(prefix);
  if (it == endpointMap.end())
    return;

  OpalEndPoint * endpoint = it->second;

  endpointsMutex.StartWrite();
  endpointMap.erase(it);
  endpointsMutex.EndWrite();

  // See if other references
  for (it = endpointMap.begin(); it != endpointMap.end(); ++it) {
    if (it->second == endpoint)
      return; // Still a reference to it
  }

  // Last copy, delete it now
  DetachEndPoint(endpoint);
}


OpalEndPoint * OpalManager::FindEndPoint(const PString & prefix)
{
  PReadWaitAndSignal mutex(endpointsMutex);
  std::map<PString, OpalEndPoint *>::iterator it = endpointMap.find(prefix);
  return it != endpointMap.end() ? it->second : NULL;
}


PBoolean OpalManager::SetUpCall(const PString & partyA,
                                const PString & partyB,
                                      PString & token,
                                         void * userData,
                                   unsigned int options,
                OpalConnection::StringOptions * stringOptions)
{
  token.MakeEmpty();

  PSafePtr<OpalCall> call = SetUpCall(partyA, partyB, userData, options, stringOptions);
  if (call == NULL)
    return false;
  
  token = call->GetToken();
  return true;
}


PSafePtr<OpalCall> OpalManager::SetUpCall(const PString & partyA,
                                          const PString & partyB,
                                                   void * userData,
                                             unsigned int options,
                          OpalConnection::StringOptions * stringOptions)
{
  PTRACE(3, "OpalMan\tSet up call from " << partyA << " to " << partyB);
  stCallUserData* pUserData = (stCallUserData*)userData;

  OpalCall * call = CreateCall(userData);
  if (call == NULL)
    return NULL;
  //chenyuan
  call->SetCallType( (em_CallType)pUserData->calltype );
  call->SetRepairSocketType( (em_rtp_type)pUserData->callrepairsockettype );
  call->m_emCallTypeCTC = (em_CallTypeForCTC)pUserData->ctccalltype;
  call->m_strOwnerUserID = pUserData->strOwnerID;
  if (stringOptions){
    if (stringOptions->Contains("conf") )
     call->SetConfToken( *stringOptions->GetAt("conf"), false, 0 );
    if (stringOptions->Contains("proxy") ){
      PString str =*stringOptions->GetAt("proxy");
      call->SetProxied(str =="1" );
    }
  }
  call->SetPartyB(partyB);

  // If we are the A-party then need to initiate a call now in this thread and
  // go through the routing engine via OnIncomingConnection. If we were the
  // B-Party then SetUpConnection() gets called in the context of the A-party
  // thread.
  PSafePtr<OpalConnection> connection = MakeConnection(*call, partyA, userData, options, stringOptions);
  if (connection != NULL && connection->SetUpConnection()) {
    PTRACE(4, "OpalMan\tSetUpCall succeeded, call=" << *call);
    return call;
  }

  PTRACE_IF(2, connection == NULL, "OpalMan\tCould not create connection for \"" << partyA << '"');

  OpalConnection::CallEndReason endReason = endReason = call->GetCallEndReason();
  if (endReason == OpalConnection::NumCallEndReasons)
    endReason = OpalConnection::EndedByTemporaryFailure;
  call->Clear(endReason);

  return NULL;
}


void OpalManager::OnEstablishedCall(OpalCall & /*call*/)
{
}


PBoolean OpalManager::IsCallEstablished(const PString & token)
{
  if (token.GetLength()> 0 ){
    PSafePtr<OpalCall> call = activeCalls.FindWithLock(token, PSafeReference);
    if (call == NULL)
      return false;


    return call->IsEstablished();
  }else   {
    return activeCalls.GetSize()> 1;
  }
}


PBoolean OpalManager::ClearCall(const PString & token,
                            OpalConnection::CallEndReason reason,
                            PSyncPoint * sync)
{
  /*The hugely multi-threaded nature of the OpalCall objects means that
    to avoid many forms of race condition, a call is cleared by moving it from
    the "active" call dictionary to a list of calls to be cleared that will be
    processed by a background thread specifically for the purpose of cleaning
    up cleared calls. So that is all that this function actually does.
    The real work is done in the OpalGarbageCollector thread.
   */

  {
    // Find the call by token, callid or conferenceid
    PSafePtr<OpalCall> call = activeCalls.FindWithLock(token, PSafeReference);
    if (call == NULL) {
      PTRACE(2, "OpalMan\tCould not find/lock call token \"" << token << '"');
      return false;
    }

    call->Clear(reason, sync);
  }

  if (sync != NULL)
    sync->Wait();

  return true;
}


PBoolean OpalManager::ClearCallSynchronous(const PString & token,
                                       OpalConnection::CallEndReason reason)
{
  PSyncPoint wait;
  return ClearCall(token, reason, &wait);
}


void OpalManager::ClearAllCalls(OpalConnection::CallEndReason reason, PBoolean wait)
{
  InternalClearAllCalls(reason, wait, m_clearingAllCallsCount++ == 0);
  --m_clearingAllCallsCount;
}


void OpalManager::InternalClearAllCalls(OpalConnection::CallEndReason reason, bool wait, bool firstThread)
{
  PTRACE(3, "OpalMan\tClearing all calls " << (wait ? "and waiting" : "asynchronously")
                      << ", " << (firstThread ? "primary" : "secondary") << " thread.");

  if (firstThread) {
    // Clear all the currentyl active calls
    for (PSafePtr<OpalCall> call = activeCalls; call != NULL; ++call)
      call->Clear(reason);
  }

  if (wait) {
    /* This is done this way as PSyncPoint only works for one thread at a time,
       all subsequent threads will wait on the mutex for the first one to be
       released from the PSyncPoint wait. */
    m_clearingAllCallsMutex.Wait();
    if (firstThread)
      m_allCallsCleared.Wait();
    m_clearingAllCallsMutex.Signal();
  }

  PTRACE(3, "OpalMan\tAll calls cleared.");
}


void OpalManager::OnClearedCall(OpalCall & PTRACE_PARAM(call))
{
  PTRACE(3, "OpalMan\tOnClearedCall " << call << " from \"" << call.GetPartyA() << "\" to \"" << call.GetPartyB() << '"');
}


OpalCall * OpalManager::InternalCreateCall()
{
  if (m_clearingAllCallsCount != 0) {
    PTRACE(2, "OpalMan\tCreate call not performed as currently clearing all calls.");
    return NULL;
  }

  return CreateCall(NULL);
}


OpalCall * OpalManager::CreateCall(void * /*userData*/)
{
  return new OpalCall(*this);
}


void OpalManager::DestroyCall(OpalCall * call)
{
  delete call;
}


PString OpalManager::GetNextToken(char prefix)
{
  return psprintf("%c%08x%u", prefix, PRandom::Number(), ++lastCallTokenID);
}

PSafePtr<OpalConnection> OpalManager::MakeConnection(OpalCall & call,
                                                const PString & remoteParty,
                                                         void * userData,
                                                   unsigned int options,
                                OpalConnection::StringOptions * stringOptions)
{
  PTRACE(3, "OpalMan\tSet up connection to \"" << remoteParty << '"');

  if (remoteParty.IsEmpty())
    return NULL;

  PCaselessString epname = remoteParty.Left(remoteParty.Find(':'));

  PReadWaitAndSignal mutex(endpointsMutex);

  OpalEndPoint * ep = NULL;
  if (epname.IsEmpty()) {
    if (endpointMap.size() > 0)
      ep = endpointMap.begin()->second;
  }
  else
    ep = FindEndPoint(epname);

  if (ep != NULL)
    return ep->MakeConnection(call, remoteParty, userData, options, stringOptions);

  PTRACE(1, "OpalMan\tCould not find endpoint to handle protocol \"" << epname << '"');
  return NULL;
}


PBoolean OpalManager::OnIncomingConnection(OpalConnection & connection, unsigned options, OpalConnection::StringOptions * stringOptions)
{
  PTRACE(3, "OpalMan\tOnIncoming connection " << connection);

  connection.OnApplyStringOptions();

  // See if we already have a B-Party in the call. If not, make one.
  if (connection.GetOtherPartyConnection() != NULL)
    return true;

  OpalCall & call = connection.GetCall();

  // See if have pre-allocated B party address, otherwise
  // get destination from incoming connection
  PString destination = call.GetPartyB();
  if (destination.IsEmpty()) {
    destination = connection.GetDestinationAddress();
    if (destination.IsEmpty()) {
      PTRACE(3, "OpalMan\tCannot complete call, no destination address from connection " << connection);
      return false;
    }
  }

  OpalConnection::StringOptions mergedOptions = connection.GetStringOptions() ;
  if (stringOptions != NULL) {
    for (PINDEX i = 0; i < stringOptions->GetSize(); ++i)
      mergedOptions.SetAt(stringOptions->GetKeyAt(i), stringOptions->GetDataAt(i));
  }

  // Use a routing algorithm to figure out who the B-Party is, and make second connection
  PStringSet routesTried;
  return OnRouteConnection(routesTried, connection.GetLocalPartyURL(), destination, call, options, &mergedOptions);
}


bool OpalManager::OnRouteConnection(PStringSet & routesTried,
                                    const PString & a_party,
                                    const PString & b_party,
                                    OpalCall & call,
                                    unsigned options,
                                    OpalConnection::StringOptions * stringOptions)
{
  PINDEX tableEntry = 0;
  for (;;) {
    PString route = ApplyRouteTable(a_party, b_party, tableEntry);
    if (route.IsEmpty()) {
      // Check for if B-Party is an explicit address
      if (FindEndPoint(b_party.Left(b_party.Find(':'))) != NULL)
        return MakeConnection(call, b_party, NULL, options, stringOptions) != NULL;

      PTRACE(3, "OpalMan\tCould not route a=\"" << a_party << "\", b=\"" << b_party << ", call=" << call);
      return false;
    }

    // See if already tried, keep searching if this route has already failed
    if (routesTried[route])
      continue;
    routesTried += route;

    // See if this route can be connected
    if (MakeConnection(call, route, NULL, options, stringOptions) != NULL)
      return true;

    // Recursively call with translated route
    if (OnRouteConnection(routesTried, a_party, route, call, options, stringOptions))
      return true;
  }
}


void OpalManager::OnProceeding(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnProceeding " << connection);

  connection.GetCall().OnProceeding(connection);
}


void OpalManager::OnAlerting(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnAlerting " << connection);

  connection.GetCall().OnAlerting(connection);
}


OpalConnection::AnswerCallResponse OpalManager::OnAnswerCall(OpalConnection & connection,
                                                             const PString & caller)
{
  return connection.GetCall().OnAnswerCall(connection, caller);
}


void OpalManager::OnConnected(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnConnected " << connection);

  connection.GetCall().OnConnected(connection);
}


void OpalManager::OnEstablished(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnEstablished " << connection);

  connection.GetCall().OnEstablished(connection);
}


void OpalManager::OnReleased(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnReleased " << connection);

  connection.GetCall().OnReleased(connection);
}


void OpalManager::OnHold(OpalConnection & connection, bool fromRemote, bool onHold)
{
  PTRACE(3, "OpalMan\t" << (onHold ? "On" : "Off") << " Hold "
         << (fromRemote ? "from remote" : "request succeeded") << " on " << connection);
  connection.GetEndPoint().OnHold(connection);
  connection.GetCall().OnHold(connection, fromRemote, onHold);
}


void OpalManager::OnHold(OpalConnection & /*connection*/)
{
}


PBoolean OpalManager::OnForwarded(OpalConnection & PTRACE_PARAM(connection),
			      const PString & /*forwardParty*/)
{
  PTRACE(4, "OpalEP\tOnForwarded " << connection);
  return true;
}


OpalMediaFormatList OpalManager::GetCommonMediaFormats(bool transportable, bool pcmAudio) const
{
  OpalMediaFormatList formats;

  if (transportable) {
    OpalMediaFormatList allFormats = OpalMediaFormat::GetAllRegisteredMediaFormats();
    for (OpalMediaFormatList::iterator iter = allFormats.begin(); iter != allFormats.end(); ++iter) {
      if (iter->IsTransportable())
        formats += *iter;
    }
  }

  if (pcmAudio) {
    // Sound cards can only do 16 bit PCM, but at various sample rates
    // The following will be in order of preference, so lets do wideband first
    formats += OpalPCM16_48KHZ;
    formats += OpalPCM16_32KHZ;
    formats += OpalPCM16_16KHZ;
    formats += OpalPCM16;
    formats += OpalRFC2833;
#if OPAL_T38_CAPABILITY
    formats += OpalCiscoNSE;
#endif
  }

#if OPAL_VIDEO
  if (!videoInputDevice.deviceName.IsEmpty())
    formats += OpalYUV420P;
#endif

#if OPAL_HAS_MSRP
  formats += OpalMSRP;
#endif

#if OPAL_HAS_SIPIM
  formats += OpalSIPIM;
#endif

#if OPAL_HAS_RFC4103
  formats += OpalT140;
#endif

#if OPAL_HAS_H224
  formats += OpalH224AnnexQ;
  formats += OpalH224Tunnelled;
#endif

  return formats;
}


void OpalManager::AdjustMediaFormats(bool local,
                                     const OpalConnection & /*connection*/,
                                     OpalMediaFormatList & mediaFormats) const
{
  mediaFormats.Remove(mediaFormatMask);
  if (local)
    mediaFormats.Reorder(mediaFormatOrder);
}


PBoolean OpalManager::IsMediaBypassPossible(const OpalConnection & source,
                                        const OpalConnection & destination,
                                        unsigned sessionID) const
{
  PTRACE(3, "OpalMan\tIsMediaBypassPossible: session " << sessionID);

  return source.IsMediaBypassPossible(sessionID) &&
         destination.IsMediaBypassPossible(sessionID);
}


PBoolean OpalManager::OnOpenMediaStream(OpalConnection & PTRACE_PARAM(connection),
                                        OpalMediaStream & PTRACE_PARAM(stream))
{
  PTRACE(3, "OpalMan\tOnOpenMediaStream " << connection << ',' << stream);
  return true;
}


RTP_UDP * OpalManager::CreateRTPSession (const RTP_Session::Params & params)
{
  return new RTP_UDP(params);
}


void OpalManager::OnRTPStatistics(const OpalConnection & connection, const RTP_Session & session)
{
  connection.GetCall().OnRTPStatistics(connection, session);
}


bool OpalManager::OnLocalRTP(OpalConnection & PTRACE_PARAM(connection1),
                             OpalConnection & PTRACE_PARAM(connection2),
                             unsigned         PTRACE_PARAM(sessionID),
                             bool             PTRACE_PARAM(started)) const
{
  PTRACE(3, "OpalMan\tOnLocalRTP(" << connection1 << ',' << connection2 << ',' << sessionID << ',' << started);
  return false;
}


static bool BypassOne(OpalMediaStreamPtr source,
                      OpalMediaStreamPtr sink,
                      bool bypass)
{
  OpalMediaPatch * sourcePatch = source != NULL ? source->GetPatch() : NULL;
  OpalMediaPatch * sinkPatch = sink != NULL ? sink->GetPatch() : NULL;
  if (sourcePatch == NULL || sinkPatch == NULL) {
    PTRACE(2, "OpalMan\tSetMediaBypass could not complete as one patch does not exist");
    return false;
  }

  if (source->GetMediaFormat() != sink->GetMediaFormat()) {
    PTRACE(3, "OpalMan\tSetMediaBypass could not complete as different formats: "
           << source->GetMediaFormat() << "!=" << sink->GetMediaFormat());
    return false;
  }

  // Note SetBypassPatch() will do PTRACE() on status.
  return sourcePatch->SetBypassPatch(bypass ? sinkPatch : NULL);
}


bool OpalManager::SetMediaBypass(OpalConnection & connection1,
                                 OpalConnection & connection2,
                                 bool bypass,
                                 unsigned sessionID)
{
  bool gotOne = false;

  if (sessionID != 0) {
    // Do not use || as McCarthy will not execute the second bypass
    if (BypassOne(connection1.GetMediaStream(sessionID, true), connection2.GetMediaStream(sessionID, false), bypass))
      gotOne = true;
    if (BypassOne(connection2.GetMediaStream(sessionID, true), connection1.GetMediaStream(sessionID, false), bypass))
      gotOne = true;
  }
  else {
    OpalMediaStreamPtr stream;
    while ((stream = connection1.GetMediaStream(OpalMediaType(), true , stream)) != NULL) {
      if (BypassOne(stream, connection2.GetMediaStream(stream->GetSessionID(), false), bypass))
        gotOne = true;
    }
    while ((stream = connection2.GetMediaStream(OpalMediaType(), true, stream)) != NULL) {
      if (BypassOne(stream, connection1.GetMediaStream(stream->GetSessionID(), false), bypass))
        gotOne = true;
    }
  }

  return gotOne;
}


bool OpalManager::SetMediaBypass(const PString & token1,
                                 const PString & token2,
                                 bool bypass,
                                 unsigned sessionID,
                                 bool network)
{
  PSafePtr<OpalCall> call1 = FindCallWithLock(token1);
  PSafePtr<OpalCall> call2 = FindCallWithLock(token2);

  if (call1 == NULL || call2 == NULL) {
    PTRACE(2, "OpalMan\tSetMediaBypass could not complete as one call does not exist");
    return false;
  }

  PSafePtr<OpalConnection> connection1 = call1->GetConnection(0, PSafeReadOnly);
  while (connection1 != NULL && connection1->IsNetworkConnection() == network)
    ++connection1;

  PSafePtr<OpalConnection> connection2 = call2->GetConnection(0, PSafeReadOnly);
  while (connection2 != NULL && connection2->IsNetworkConnection() == network)
    ++connection2;

  if (connection1 == NULL || connection2 == NULL) {
    PTRACE(2, "OpalMan\tSetMediaBypass could not complete as network connection not present in calls");
    return false;
  }

  return OpalManager::SetMediaBypass(*connection1, *connection2, sessionID, bypass);
}


void OpalManager::OnClosedMediaStream(const OpalMediaStream & /*channel*/)
{
}

#if OPAL_VIDEO

PBoolean OpalManager::CreateVideoInputDevice(const OpalConnection & /*connection*/,
                                         const OpalMediaFormat & mediaFormat,
                                         PVideoInputDevice * & device,
                                         PBoolean & autoDelete)
{
  // Make copy so we can adjust the size
  PVideoDevice::OpenArgs args = videoInputDevice;
  mediaFormat.AdjustVideoArgs(args);

  autoDelete = true;
  device = PVideoInputDevice::CreateOpenedDevice(args, false);
  PTRACE_IF(2, device == NULL, "OpalCon\tCould not open video device \"" << args.deviceName << '"');
  return device != NULL;
}


PBoolean OpalManager::CreateVideoOutputDevice(const OpalConnection & connection,
                                          const OpalMediaFormat & mediaFormat,
                                          PBoolean preview,
                                          PVideoOutputDevice * & device,
                                          PBoolean & autoDelete)
{
  // Donot use our one and only SDL window, if we need it for the video output.
  if (preview && (
      (videoPreviewDevice.driverName == "SDL" && videoOutputDevice.driverName == "SDL") ||
      (videoPreviewDevice.deviceName == "SDL" && videoOutputDevice.deviceName == "SDL")
      ))
    return false;

  // Make copy so we can adjust the size
  PVideoDevice::OpenArgs args = preview ? videoPreviewDevice : videoOutputDevice;
  mediaFormat.AdjustVideoArgs(args);

  PINDEX start = args.deviceName.Find("TITLE=\"");
  if (start != P_MAX_INDEX) {
    start += 7;
    args.deviceName.Splice(preview ? "Local Preview" : connection.GetRemotePartyName(), start, args.deviceName.Find('"', start)-start);
  }

  autoDelete = true;
  device = PVideoOutputDevice::CreateOpenedDevice(args, false);
  return device != NULL;
}

#endif // OPAL_VIDEO


OpalMediaPatch * OpalManager::CreateMediaPatch(OpalMediaStream & source,
                                               PBoolean requiresPatchThread)
{
  if (requiresPatchThread) {
    return new OpalMediaPatch(source);
  } else {
    return new OpalPassiveMediaPatch(source);
  }
}


void OpalManager::DestroyMediaPatch(OpalMediaPatch * patch)
{
  delete patch;
}


void OpalManager::OnStartMediaPatch(OpalConnection & /*connection*/, OpalMediaPatch & /*patch*/)
{
}


void OpalManager::OnStopMediaPatch(OpalConnection & /*connection*/, OpalMediaPatch & /*patch*/)
{
}


void OpalManager::OnUserInputString(OpalConnection & connection,
                                    const PString & value)
{
  connection.GetCall().OnUserInputString(connection, value);
}


void OpalManager::OnUserInputTone(OpalConnection & connection,
                                  char tone,
                                  int duration)
{
  connection.GetCall().OnUserInputTone(connection, tone, duration);
}


PString OpalManager::ReadUserInput(OpalConnection & connection,
                                  const char * terminators,
                                  unsigned lastDigitTimeout,
                                  unsigned firstDigitTimeout)
{
  PTRACE(3, "OpalMan\tReadUserInput from " << connection);

  connection.PromptUserInput(true);
  PString digit = connection.GetUserInput(firstDigitTimeout);
  connection.PromptUserInput(false);

  if (digit.IsEmpty()) {
    PTRACE(2, "OpalMan\tReadUserInput first character timeout (" << firstDigitTimeout << " seconds) on " << *this);
    return PString::Empty();
  }

  PString input;
  while (digit.FindOneOf(terminators) == P_MAX_INDEX) {
    input += digit;

    digit = connection.GetUserInput(lastDigitTimeout);
    if (digit.IsEmpty()) {
      PTRACE(2, "OpalMan\tReadUserInput last character timeout (" << lastDigitTimeout << " seconds) on " << *this);
      return input; // Input so far will have to do
    }
  }

  return input.IsEmpty() ? digit : input;
}


void OpalManager::OnMWIReceived(const PString & PTRACE_PARAM(party),
                                MessageWaitingType PTRACE_PARAM(type),
                                const PString & PTRACE_PARAM(extraInfo))
{
  PTRACE(3, "OpalMan\tOnMWIReceived(" << party << ',' << type << ',' << extraInfo << ')');
}


OpalManager::RouteEntry::RouteEntry(const PString & pat, const PString & dest)
  : pattern(pat),
    destination(dest)
{
  PString adjustedPattern = '^';

  // Test for backward compatibility format
  PINDEX colon = pattern.Find(':');
  if (colon != P_MAX_INDEX && pattern.Find('\t', colon) == P_MAX_INDEX)
    adjustedPattern += pattern.Left(colon+1) + ".*\t" + pattern.Mid(colon+1);
  else
    adjustedPattern += pattern;

  adjustedPattern += '$';

  if (!regex.Compile(adjustedPattern, PRegularExpression::IgnoreCase|PRegularExpression::Extended)) {
    PTRACE(1, "OpalMan\tCould not compile route regular expression \"" << adjustedPattern << '"');
  }
}


void OpalManager::RouteEntry::PrintOn(ostream & strm) const
{
  strm << pattern << '=' << destination;
}


PBoolean OpalManager::AddRouteEntry(const PString & spec)
{
  if (spec[0] == '#') // Comment
    return false;

  if (spec[0] == '@') { // Load from file
    PTextFile file;
    if (!file.Open(spec.Mid(1), PFile::ReadOnly)) {
      PTRACE(1, "OpalMan\tCould not open route file \"" << file.GetFilePath() << '"');
      return false;
    }
    PTRACE(4, "OpalMan\tAdding routes from file \"" << file.GetFilePath() << '"');
    PBoolean ok = false;
    PString line;
    while (file.good()) {
      file >> line;
      if (AddRouteEntry(line))
        ok = true;
    }
    return ok;
  }

  PINDEX equal = spec.Find('=');
  if (equal == P_MAX_INDEX) {
    PTRACE(2, "OpalMan\tInvalid route table entry: \"" << spec << '"');
    return false;
  }

  RouteEntry * entry = new RouteEntry(spec.Left(equal).Trim(), spec.Mid(equal+1).Trim());
  if (entry->regex.GetErrorCode() != PRegularExpression::NoError) {
    PTRACE(2, "OpalMan\tIllegal regular expression in route table entry: \"" << spec << '"');
    delete entry;
    return false;
  }

  PTRACE(4, "OpalMan\tAdded route \"" << *entry << '"');
  m_routeMutex.Wait();
  m_routeTable.Append(entry);
  m_routeMutex.Signal();
  return true;
}


PBoolean OpalManager::SetRouteTable(const PStringArray & specs)
{
  PBoolean ok = false;

  m_routeMutex.Wait();
  m_routeTable.RemoveAll();

  for (PINDEX i = 0; i < specs.GetSize(); i++) {
    if (AddRouteEntry(specs[i].Trim()))
      ok = true;
  }

  m_routeMutex.Signal();

  return ok;
}


void OpalManager::SetRouteTable(const RouteTable & table)
{
  m_routeMutex.Wait();
  m_routeTable = table;
  m_routeTable.MakeUnique();
  m_routeMutex.Signal();
}


static void ReplaceNDU(PString & destination, const PString & subst)
{
  if (subst.Find('@') != P_MAX_INDEX) {
    PINDEX at = destination.Find('@');
    if (at != P_MAX_INDEX) {
      PINDEX du = destination.Find("<!du>", at);
      if (du != P_MAX_INDEX)
        destination.Delete(at, du-at);
    }
  }
  destination.Replace("<!du>", subst, true);
}


PString OpalManager::ApplyRouteTable(const PString & a_party, const PString & b_party, PINDEX & routeIndex)
{
  PWaitAndSignal mutex(m_routeMutex);

  if (m_routeTable.IsEmpty())
    return routeIndex++ == 0 ? b_party : PString::Empty();

  PString search = a_party + '\t' + b_party;
  PTRACE(4, "OpalMan\tSearching for route \"" << search << '"');

  /* Examples:
        Call from UI       pc:USB Audio Device\USB Audio Device      sip:fred@boggs.com
                           pc:USB Audio Device\USB Audio Device      h323:fred@boggs.com
                           pc:USB Audio Device\USB Audio Device      fred
        Call from handset  pots:TigerJet:USB Audio Device            123
        Call from SIP      sip:me@here.net                           sip:you@there.com
                           sip:me@here.net:5061                      sip:you@there.com
        Call from H.323    h323:me@here.net                          h323:there.com
                           h323:me@here.net:1721                     h323:fred

     Table:
        .*:#  = ivr:
        pots:.*\\*.*\\*.* = sip:<dn2ip>
        pots:.*           = sip:<da>
        pc:.*             = sip:<da>
        h323:.*           = pots:<dn>
        sip:.*            = pots:<dn>
        h323:.*           = pc:
        sip:.*            = pc:
   */

  PString destination;
  while (routeIndex < m_routeTable.GetSize()) {
    RouteEntry & entry = m_routeTable[routeIndex++];
    PINDEX pos;
    if (entry.regex.Execute(search, pos)) {
      PTRACE(4, "OpalMan\tMatched regex \"" << entry.pattern << '"');
      if (entry.destination.NumCompare("label:") != EqualTo) {
        destination = entry.destination;
        break;
      }

      // restart search in table using label.
      search = entry.destination;
      routeIndex = 0;
    }
    else {
      PTRACE(4, "OpalMan\tDid not match regex \"" << entry.pattern << '"');
    }
  }

  // No route found
  if (destination.IsEmpty())
    return PString::Empty();

  // We are backward compatibility mode and the supplied address can be called
  PINDEX colon = b_party.Find(':');
  if (colon == P_MAX_INDEX)
    colon = 0;
  else if (b_party.NumCompare("tel", colon) == EqualTo) // Small cheat for tel: URI (RFC3966)
    colon++;
  else if (FindEndPoint(b_party.Left(colon)) == NULL)
    colon = 0;
  else {
    if (destination.Find("<da>") != P_MAX_INDEX)
      return b_party;
    colon++;
  }

  PINDEX nonDigitPos = b_party.FindSpan("0123456789*#-.()", colon + (b_party[colon] == '+'));
  PString digits = b_party(colon, nonDigitPos-1);

  PINDEX at = b_party.Find('@', colon);

  // Another tel: URI hack
  static const char PhoneContext[] = ";phone-context=";
  PINDEX pos = b_party.Find(PhoneContext);
  if (pos != P_MAX_INDEX) {
    pos += sizeof(PhoneContext)-1;
    PINDEX end = b_party.Find(';', pos)-1;
    if (b_party[pos] == '+') // Phone context is a prefix
      digits.Splice(b_party(pos+1, end), 0);
    else // Otherwise phone context is a domain name
      ReplaceNDU(destination, '@'+b_party(pos, end));
  }

  // Filter out the non E.164 digits, mainly for tel: URI support.
  while ((pos = digits.FindOneOf("+-.()")) != P_MAX_INDEX)
    digits.Delete(pos, 1);

  PString user = b_party(colon, at-1);

  destination.Replace("<da>", b_party, true);
  destination.Replace("<db>", user, true);

  if (at != P_MAX_INDEX) {
    destination.Replace("<du>", user, true);
    ReplaceNDU(destination, b_party.Mid(at));
  }
  else if (PIPSocket::IsLocalHost(user.Left(user.Find(':')))) {
    destination.Replace("<du>", "", true);
    ReplaceNDU(destination, user);
  }
  else {
    destination.Replace("<du>", user, true);
    ReplaceNDU(destination, "");
  }

  destination.Replace("<dn>", digits, true);
  destination.Replace("<!dn>", b_party.Mid(nonDigitPos), true);

  while ((pos = destination.FindRegEx("<dn[1-9]>")) != P_MAX_INDEX)
    destination.Splice(digits.Mid(destination[pos+3]-'0'), pos, 5);

  // Do meta character substitutions
  while ((pos = destination.Find("<dn2ip>")) != P_MAX_INDEX) {
    PStringStream route;
    PStringArray stars = digits.Tokenise('*');
    switch (stars.GetSize()) {
      case 0 :
      case 1 :
      case 2 :
      case 3 :
        route << digits;
        break;

      case 4 :
        route << stars[0] << '.' << stars[1] << '.'<< stars[2] << '.'<< stars[3];
        break;

      case 5 :
        route << stars[0] << '@'
              << stars[1] << '.' << stars[2] << '.'<< stars[3] << '.'<< stars[4];
        break;

      default :
        route << stars[0] << '@'
              << stars[1] << '.' << stars[2] << '.'<< stars[3] << '.'<< stars[4]
              << ':' << stars[5];
        break;
    }
    destination.Splice(route, pos, 7);
  }

  return destination;
}


void OpalManager::SetProductInfo(const OpalProductInfo & info, bool updateAll)
{
  productInfo = info;

  if (updateAll) {
    endpointsMutex.StartWrite();
    for (PList<OpalEndPoint>::iterator ep = endpointList.begin(); ep != endpointList.end(); ++ep)
      ep->SetProductInfo(info);
    endpointsMutex.EndWrite();
  }
}


void OpalManager::SetDefaultUserName(const PString & name, bool updateAll)
{
  defaultUserName = name;

  if (updateAll) {
    endpointsMutex.StartWrite();
    for (PList<OpalEndPoint>::iterator ep = endpointList.begin(); ep != endpointList.end(); ++ep)
      ep->SetDefaultLocalPartyName(name);
    endpointsMutex.EndWrite();
  }
}


void OpalManager::SetDefaultDisplayName(const PString & name, bool updateAll)
{
  defaultDisplayName = name;

  if (updateAll) {
    endpointsMutex.StartWrite();
    for (PList<OpalEndPoint>::iterator ep = endpointList.begin(); ep != endpointList.end(); ++ep)
      ep->SetDefaultDisplayName(name);
    endpointsMutex.EndWrite();
  }
}


PBoolean OpalManager::IsLocalAddress(const PIPSocket::Address & ip) const
{
  /* Check if the remote address is a private IP, broadcast, or us */
  return ip.IsAny() || ip.IsBroadcast() || ip.IsRFC1918() || PIPSocket::IsLocalHost(ip);
}


PBoolean OpalManager::IsRTPNATEnabled(OpalConnection & /*conn*/, 
                                      const PIPSocket::Address & localAddr,
                                      const PIPSocket::Address & peerAddr,
                                      const PIPSocket::Address & sigAddr,
                                      PBoolean PTRACE_PARAM(incoming))
{
  PTRACE(4, "OPAL\tChecking " << (incoming ? "incoming" : "outgoing") << " call for NAT: local=" << localAddr << ", peer=" << peerAddr << ", sig=" << sigAddr);

  /* The peer endpoint may be on a public address, the local network (LAN) or
     NATed. If the last, it either knows it is behind the NAT or is blissfully
     unaware of it.

     If the remote is public or on a LAN/VPN then no special treatment of RTP
     is needed. We do make the assumption that the remote will indicate correct
     addresses everywhere, in SETUP/OLC and INVITE/SDP.

     Now if the endpoint is NATed and knows it is behind a NAT, and is doing
     it correctly, it is indistinguishable from being on a public address.

     If the remote endpoint is unaware of it's NAT status then there will be a
     discrepency between the physical address of the connection and the
     signaling adddress indicated in the protocol, the H.323 SETUP
     sourceCallSignalAddress or SIP "Contact" field.

     So this is the first test to make: if those addresses the same, we will
     assume the other guy is public or LAN/VPN and either no NAT is involved,
     or we leave them in charge of any NAT traversal as he has the ability to
     do it. In either case we don't do anything.
   */

  if (peerAddr == sigAddr)
    return false;

  /* Next test is to see if BOTH addresses are "public", non RFC1918. There are
     some cases with proxies, particularly with SIP, where this is possible. We
     will assume that NAT never occurs between two public addresses though it
     could occur between two private addresses */

  if (!peerAddr.IsRFC1918() && !sigAddr.IsRFC1918())
    return false;

  /* So now we have a remote that is confused in some way, so needs help. Our
     next test is for cases of where we are on a multi-homed machine and we
     ended up with a call from interface to another. No NAT needed.
   */
  if (PIPSocket::IsLocalHost(peerAddr))
    return false;

  /* So, call is from a remote host somewhere and is still confused. We now
     need to check if we are actually ABLE to help. We test if the local end
     of the connection is public, i.e. no NAT at this end so we can help.
   */
  if (!localAddr.IsRFC1918())
    return true;

  /* Another test for if we can help, we are behind a NAT too, but the user has
     provided information so we can compensate for it, i.e. we "know" about the
     NAT. We determine this by translating the localAddr and seing if it gets
     changed to the NAT router address. If so, we can help.
   */
  PIPSocket::Address natAddr = localAddr;
  if (TranslateIPAddress(natAddr, peerAddr))
    return true;

  /* If we get here, we appear to be in a situation which, if we tried to do the
     NAT translation, we could end up in a staring match as the NAT traversal
     technique does not send anything till it receives something. If both side
     do that then .....

     So, we do nothing and hope for the best. This means that either the call
     gets no media, or there is some other magic entity (smart router, proxy,
     etc) between the two endpoints we know nothing about that will do NAT
     translations for us.

     Are there any other cases?
  */
  return false;
}


PBoolean OpalManager::TranslateIPAddress(PIPSocket::Address & localAddress,
                                     const PIPSocket::Address & remoteAddress)
{
  if (!IsLocalAddress(localAddress))
    return false; // Is already translated

  if (IsLocalAddress(remoteAddress))
    return false; // Does not need to be translated

  if (translationAddress.IsValid()) {
    localAddress = translationAddress; // Translate it!
    return true;
  }

  PIPSocket::Address stunInterface;
  if (stun != NULL &&
      stun->GetNatType() != PSTUNClient::BlockedNat &&
      stun->GetInterfaceAddress(stunInterface) &&
      stunInterface == localAddress)
    return stun->GetExternalAddress(localAddress); // Translate it!

  return false; // Have nothing to translate it to
}


bool OpalManager::SetTranslationHost(const PString & host)
{
  if (PIPSocket::GetHostAddress(host, translationAddress)) {
    translationHost = host;
    return true;
  }

  translationHost = PString::Empty();
  translationAddress = PIPSocket::GetDefaultIpAny();
  return false;
}


void OpalManager::SetTranslationAddress(const PIPSocket::Address & address)
{
  translationAddress = address;
  translationHost = PIPSocket::GetHostName(address);
}


PNatMethod * OpalManager::GetNatMethod(const PIPSocket::Address & ip) const
{
  if (ip.IsValid() && IsLocalAddress(ip))
    return NULL;

  return stun;
}


PSTUNClient::NatTypes OpalManager::SetSTUNServer(const PString & server)
{
  stunServer = server;

  if (server.IsEmpty()) {
    if (stun)
      PInterfaceMonitor::GetInstance().OnRemoveNatMethod(stun);

    delete stun;
    delete interfaceMonitor;
    stun = NULL;
    interfaceMonitor = NULL;
    return PSTUNClient::UnknownNat;
  }

  if (stun != NULL)
    stun->SetServer(server);
  else {
    stun = new PSTUNClient(server,
                                         GetUDPPortBase(), GetUDPPortMax(),
                                         GetRtpIpPortBase(), GetRtpIpPortMax());
    interfaceMonitor = new InterfaceMonitor(*this);
  }

  PSTUNClient::NatTypes type = stun->GetNatType();
  PIPSocket::Address stunExternalAddress;
  if (type != PSTUNClient::BlockedNat)
    stun->GetExternalAddress(stunExternalAddress);

  PTRACE(3, "OPAL\tSTUN server \"" << server << "\" replies " << type << ", external IP " << stunExternalAddress);

  return type;
}


void OpalManager::PortInfo::Set(unsigned newBase,
                                unsigned newMax,
                                unsigned range,
                                unsigned dflt)
{
  if (newBase == 0) {
    newBase = dflt;
    newMax = dflt;
    if (dflt > 0)
      newMax += range;
  }
  else {
    if (newBase < 1024)
      newBase = 1024;
    else if (newBase > 65500)
      newBase = 65500;

    if (newMax <= newBase)
      newMax = newBase + range;
    if (newMax > 65535)
      newMax = 65535;
  }

  mutex.Wait();

  current = base = (WORD)newBase;
  max = (WORD)newMax;

  mutex.Signal();
}


WORD OpalManager::PortInfo::GetNext(unsigned increment)
{
  PWaitAndSignal m(mutex);

  if (current < base || current >= (max-increment))
    current = base;

  if (current == 0)
    return 0;

  WORD p = current;
  current = (WORD)(current + increment);
  return p;
}


void OpalManager::SetTCPPorts(unsigned tcpBase, unsigned tcpMax)
{
  tcpPorts.Set(tcpBase, tcpMax, 49, 0);
}


WORD OpalManager::GetNextTCPPort()
{
  return tcpPorts.GetNext(1);
}


void OpalManager::SetUDPPorts(unsigned udpBase, unsigned udpMax)
{
  udpPorts.Set(udpBase, udpMax, 99, 0);

  if (stun != NULL)
    stun->SetPortRanges(GetUDPPortBase(), GetUDPPortMax(), GetRtpIpPortBase(), GetRtpIpPortMax());
}


WORD OpalManager::GetNextUDPPort()
{
  return udpPorts.GetNext(1);
}


void OpalManager::SetRtpIpPorts(unsigned rtpIpBase, unsigned rtpIpMax)
{
  rtpIpPorts.Set((rtpIpBase+1)&0xfffe, rtpIpMax&0xfffe, 199, 5000);

  if (stun != NULL)
    stun->SetPortRanges(GetUDPPortBase(), GetUDPPortMax(), GetRtpIpPortBase(), GetRtpIpPortMax());
}


WORD OpalManager::GetRtpIpPortPair()
{
  return rtpIpPorts.GetNext(2);
}


void OpalManager::SetAudioJitterDelay(unsigned minDelay, unsigned maxDelay)
{
  if (minDelay == 0) {
    // Disable jitter buffer completely if minimum is zero.
    minAudioJitterDelay = maxAudioJitterDelay = 0;
    return;
  }

  PAssert(minDelay <= 10000 && maxDelay <= 10000, PInvalidParameter);

  if (minDelay < 10)
    minDelay = 10;
  minAudioJitterDelay = minDelay;

  if (maxDelay < minDelay)
    maxDelay = minDelay;
  maxAudioJitterDelay = maxDelay;
}


void OpalManager::SetMediaFormatOrder(const PStringArray & order)
{
  mediaFormatOrder = order;
  PTRACE(3, "OPAL\tSetMediaFormatOrder(" << setfill(',') << order << ')');
}


void OpalManager::SetMediaFormatMask(const PStringArray & mask)
{
  mediaFormatMask = mask;
  PTRACE(3, "OPAL\tSetMediaFormatMask(" << setfill(',') << mask << ')');
}


#if OPAL_VIDEO
template<class PVideoXxxDevice>
static PBoolean SetVideoDevice(const PVideoDevice::OpenArgs & args, PVideoDevice::OpenArgs & member)
{
  // Check that the input device is legal
  PVideoXxxDevice * pDevice = PVideoXxxDevice::CreateDeviceByName(args.deviceName, args.driverName, args.pluginMgr);
  if (pDevice != NULL) {
    delete pDevice;
    member = args;
    return true;
  }

  if (args.deviceName[0] != '#')
    return false;

  // Selected device by ordinal
  PStringArray devices = PVideoXxxDevice::GetDriversDeviceNames(args.driverName, args.pluginMgr);
  if (devices.IsEmpty())
    return false;

  PINDEX id = args.deviceName.Mid(1).AsUnsigned();
  if (id <= 0 || id > devices.GetSize())
    return false;

  member = args;
  member.deviceName = devices[id-1];
  return true;
}


PBoolean OpalManager::SetVideoInputDevice(const PVideoDevice::OpenArgs & args)
{
  return SetVideoDevice<PVideoInputDevice>(args, videoInputDevice);
}


PBoolean OpalManager::SetVideoPreviewDevice(const PVideoDevice::OpenArgs & args)
{
  return SetVideoDevice<PVideoOutputDevice>(args, videoPreviewDevice);
}


PBoolean OpalManager::SetVideoOutputDevice(const PVideoDevice::OpenArgs & args)
{
  return SetVideoDevice<PVideoOutputDevice>(args, videoOutputDevice);
}

#endif

PBoolean OpalManager::SetNoMediaTimeout(const PTimeInterval & newInterval) 
{
  if (newInterval < 10)
    return false;

  noMediaTimeout = newInterval; 
  return true; 
}


void OpalManager::GarbageCollection()
{
  PBoolean allCleared = activeCalls.DeleteObjectsToBeRemoved();

  endpointsMutex.StartRead();

  for (PList<OpalEndPoint>::iterator ep = endpointList.begin(); ep != endpointList.end(); ++ep) {
    if (!ep->GarbageCollection())
      allCleared = false;
  }

  endpointsMutex.EndRead();

  if (allCleared && m_clearingAllCallsCount != 0)
    m_allCallsCleared.Signal();
}


void OpalManager::CallDict::DeleteObject(PObject * object) const
{
  manager.DestroyCall(PDownCast(OpalCall, object));
}


void OpalManager::GarbageMain(PThread &, INT)
{
  while (!garbageCollectExit.Wait(1000))
    GarbageCollection();
}

void OpalManager::OnNewConnection(OpalConnection & /*conn*/)
{
}

#if OPAL_HAS_MIXER

bool OpalManager::StartRecording(const PString & callToken,
                                 const PFilePath & fn,
                                 const OpalRecordManager::Options & options)
{
  PSafePtr<OpalCall> call = activeCalls.FindWithLock(callToken, PSafeReadWrite);
  if (call == NULL)
    return false;

  return call->StartRecording(fn, options);
}


bool OpalManager::IsRecording(const PString & callToken)
{
  PSafePtr<OpalCall> call = FindCallWithLock(callToken, PSafeReadWrite);
  return call != NULL && call->IsRecording();
}


bool OpalManager::StopRecording(const PString & callToken)
{
  PSafePtr<OpalCall> call = activeCalls.FindWithLock(callToken, PSafeReadWrite);
  if (call == NULL)
    return false;

  call->StopRecording();
  return true;
}

#endif


void OpalManager::OnApplyStringOptions(OpalConnection & conn, OpalConnection::StringOptions & stringOptions)
{
  conn.ApplyStringOptions(stringOptions);
}


PSafePtr<OpalPresentity> OpalManager::AddPresentity(const PString & presentity)
{
  OpalPresentity * newPresentity = OpalPresentity::Create(*this, presentity);
  if (newPresentity == NULL)
    return NULL;

  m_presentities.SetAt(presentity, newPresentity);
  return PSafePtr<OpalPresentity>(newPresentity, PSafeReadWrite);
}


PSafePtr<OpalPresentity> OpalManager::GetPresentity(const PString & presentity, PSafetyMode mode)
{
  return m_presentities.FindWithLock(presentity, mode);
}


bool OpalManager::RemovePresentity(const PString & presentity)
{
  return m_presentities.RemoveAt(presentity);
}


PBoolean OpalManager::Message(const PString & to, const PString & body)
{
  PURL url(to);

  for (PList<OpalEndPoint>::iterator it = endpointList.begin(); it != endpointList.end(); ++it) {
    if (url.GetScheme() *= it->GetPrefixName())
      return it->Message(to, body);
  }

  return false;
}


PBoolean OpalManager::Message(const PURL & to, const PString & type, const PString & body, PURL & from, PString & conversationId)
{
  for (PList<OpalEndPoint>::iterator it = endpointList.begin(); it != endpointList.end(); ++it) {
    if (to.GetScheme() *= it->GetPrefixName())
      return it->Message(to, type, body, from, conversationId);
  }

  return false;
}

void OpalManager::OnMessageReceived(
  const PURL & /*from*/, 
  const PString & /*fromName*/,
  const PURL & /*to*/, 
  const PString & /*type*/,
  const PString & /*body*/,
  const PString & /*conversationId*/
)
{
}

#if OPAL_HAS_IM

bool OpalManager::TransmitExternalIM(OpalConnection & conn, const OpalMediaFormat & /*format*/, RTP_IMFrame & frame)
{
  PURL remotePartyURL, localPartyURL;
  PString remotePartyName;

  // get information about sender
  PSafePtr<OpalConnection> otherParty = conn.GetOtherPartyConnectionAs<OpalConnection>();
  if (otherParty != NULL) {
    remotePartyURL  = otherParty->GetRemotePartyCallbackURL();
    remotePartyName = otherParty->GetRemotePartyName();
    localPartyURL   = otherParty->GetLocalPartyURL();
  }

  // get ID
  PString id = conn.GetToken(); //format.GetOptionString("Path", GetIdentifier());

  // pass information up the chain
  T140String t140;
  PString str;
  frame.GetContent(t140);
  t140.AsString(str);
  OnMessageReceived(remotePartyURL, 
                    remotePartyName, 
                    localPartyURL, 
                    frame.GetContentType(),
                    str,
                    id);

  return true;
}

#endif

/////////////////////////////////////////////////////////////////////////////

OpalManager::InterfaceMonitor::InterfaceMonitor(OpalManager & manager)
  : PInterfaceMonitorClient(OpalManagerInterfaceMonitorClientPriority)
  , m_manager(manager)
{
}

void OpalManager::InterfaceMonitor::OnAddInterface(const PIPSocket::InterfaceEntry & /*entry*/)
{
  m_manager.SetSTUNServer(m_manager.GetSTUNServer());
}

void OpalManager::InterfaceMonitor::OnRemoveInterface(const PIPSocket::InterfaceEntry & entry)
{
  PSTUNClient * stun = m_manager.GetSTUNClient();
  PIPSocket::Address addr;
  if (stun != NULL && stun->GetInterfaceAddress(addr) && entry.GetAddress() == addr)
    stun->InvalidateCache();
}

#ifdef OPAL_ZRTP
bool OpalManager::GetZRTPEnabled() const
{
  return zrtpEnabled;
}
#endif


/////////////////////////////////////////////////////////////////////////////
