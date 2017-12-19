/*
 * connection.cxx
 *
 * Connection abstraction
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
 * $Revision: 23954 $
 * $Author: rjongbloed $
 * $Date: 2010-01-20 23:11:47 +0000 (Wed, 20 Jan 2010) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "connection.h"
#endif

#include <opal/buildopts.h>

#include <opal/connection.h>

#include <opal/manager.h>
#include <opal/endpoint.h>
#include <opal/call.h>
#include <opal/transcoders.h>
#include <opal/patch.h>
#include <codec/silencedetect.h>
#include <codec/echocancel.h>
#include <codec/rfc2833.h>
#include <codec/vidcodec.h>
#include <rtp/rtp.h>
#include <t120/t120proto.h>
#include <t38/t38proto.h>
#include <ptclib/url.h>
#include "myclient.h"
extern CMyClient *g_MyClient;

#if OPAL_HAS_IM
#include <im/rfc4103.h>
#if OPAL_SIP
#include <im/sipim.h>
#include <sip/sipcon.h>
#endif
#endif

#define new PNEW
#ifdef _WIN32
#else
#define D_YYCODE_CORE   "IVoice"
const  char* YYCODE_CORE = D_YYCODE_CORE;
#endif

#if PTRACING
ostream & operator<<(ostream & out, OpalConnection::Phases phase)
{
  static const char * const names[OpalConnection::NumPhases] = {
    "UninitialisedPhase",
    "SetUpPhase",
    "ProceedingPhase",
    "AlertingPhase",
    "ConnectedPhase",
    "EstablishedPhase",
    "ForwardingPhase",
    "ReleasingPhase",
    "ReleasedPhase"
  };
  return out << names[phase];
}


ostream & operator<<(ostream & out, OpalConnection::CallEndReason reason)
{
  const char * const names[OpalConnection::NumCallEndReasons] = {
    "EndedByLocalUser",         /// Local endpoint application cleared call
    "EndedByNoAccept",          /// Local endpoint did not accept call OnIncomingCall()=PFalse
    "EndedByAnswerDenied",      /// Local endpoint declined to answer call
    "EndedByRemoteUser",        /// Remote endpoint application cleared call
    "EndedByRefusal",           /// Remote endpoint refused call
    "EndedByNoAnswer",          /// Remote endpoint did not answer in required time
    "EndedByCallerAbort",       /// Remote endpoint stopped calling
    "EndedByTransportFail",     /// Transport error cleared call
    "EndedByConnectFail",       /// Transport connection failed to establish call
    "EndedByGatekeeper",        /// Gatekeeper has cleared call
    "EndedByNoUser",            /// Call failed as could not find user (in GK)
    "EndedByNoBandwidth",       /// Call failed as could not get enough bandwidth
    "EndedByCapabilityExchange",/// Could not find common capabilities
    "EndedByCallForwarded",     /// Call was forwarded using FACILITY message
    "EndedBySecurityDenial",    /// Call failed a security check and was ended
    "EndedByLocalBusy",         /// Local endpoint busy
    "EndedByLocalCongestion",   /// Local endpoint congested
    "EndedByRemoteBusy",        /// Remote endpoint busy
    "EndedByRemoteCongestion",  /// Remote endpoint congested
    "EndedByUnreachable",       /// Could not reach the remote party
    "EndedByNoEndPoint",        /// The remote party is not running an endpoint
    "EndedByOffline",           /// The remote party is off line
    "EndedByTemporaryFailure",  /// The remote failed temporarily app may retry
    "EndedByQ931Cause",         /// The remote ended the call with unmapped Q.931 cause code
    "EndedByDurationLimit",     /// Call cleared due to an enforced duration limit
    "EndedByInvalidConferenceID",/// Call cleared due to invalid conference ID
    "EndedByNoDialTone",        /// Call cleared due to missing dial tone
    "EndedByNoRingBackTone",    /// Call cleared due to missing ringback tone    
    "EndedByOutOfService",      /// Call cleared because the line is out of service, 
    "EndedByAcceptingCallWaiting", /// Call cleared because another call is answered
    "EndedByGkAdmissionFailed", /// Call cleared because gatekeeper admission request failed.
  };
  PAssert((PINDEX)(reason & 0xff) < PARRAYSIZE(names), "Invalid reason");
  return out << names[reason & 0xff];
}

ostream & operator<<(ostream & o, OpalConnection::AnswerCallResponse s)
{
  static const char * const AnswerCallResponseNames[OpalConnection::NumAnswerCallResponses] = {
    "AnswerCallNow",
    "AnswerCallDenied",
    "AnswerCallPending",
    "AnswerCallDeferred",
    "AnswerCallAlertWithMedia",
    "AnswerCallDeferredWithMedia",
    "AnswerCallProgress",
    "AnswerCallNowAndReleaseCurrent"
  };
  if ((PINDEX)s >= PARRAYSIZE(AnswerCallResponseNames))
    o << "InvalidAnswerCallResponse<" << (unsigned)s << '>';
  else if (AnswerCallResponseNames[s] == NULL)
    o << "AnswerCallResponse<" << (unsigned)s << '>';
  else
    o << AnswerCallResponseNames[s];
  return o;
}

ostream & operator<<(ostream & o, OpalConnection::SendUserInputModes m)
{
  static const char * const SendUserInputModeNames[OpalConnection::NumSendUserInputModes] = {
    "SendUserInputAsQ931",
    "SendUserInputAsString",
    "SendUserInputAsTone",
    "SendUserInputAsRFC2833",
    "SendUserInputAsSeparateRFC2833",
    "SendUserInputAsProtocolDefault"
  };

  if ((PINDEX)m >= PARRAYSIZE(SendUserInputModeNames))
    o << "InvalidSendUserInputMode<" << (unsigned)m << '>';
  else if (SendUserInputModeNames[m] == NULL)
    o << "SendUserInputMode<" << (unsigned)m << '>';
  else
    o << SendUserInputModeNames[m];
  return o;
}

#endif


static POrdinalToString::Initialiser const CallEndReasonStringsInitialiser[] = {
  { OpalConnection::EndedByLocalUser,            "Local party cleared call" },
  { OpalConnection::EndedByNoAccept,             "Local party did not accept call" },
  { OpalConnection::EndedByAnswerDenied,         "Local party declined to answer call" },
  { OpalConnection::EndedByRemoteUser,           "Remote party cleared call" },
  { OpalConnection::EndedByRefusal,              "Remote party refused call" },
  { OpalConnection::EndedByNoAnswer,             "Remote party did not answer in required time" },
  { OpalConnection::EndedByCallerAbort,          "Remote party stopped calling" },
  { OpalConnection::EndedByTransportFail,        "Call failed due to a transport error" },
  { OpalConnection::EndedByConnectFail,          "Connection to remote failed" },
  { OpalConnection::EndedByGatekeeper,           "Gatekeeper has cleared call" },
  { OpalConnection::EndedByNoUser,               "Call failed as could not find user" },
  { OpalConnection::EndedByNoBandwidth,          "Call failed due to insufficient bandwidth" },
  { OpalConnection::EndedByCapabilityExchange,   "Call failed as could not find common media capabilities" },
  { OpalConnection::EndedByCallForwarded,        "Call was forwarded" },
  { OpalConnection::EndedBySecurityDenial,       "Call failed security check" },
  { OpalConnection::EndedByLocalBusy,            "Local party busy" },
  { OpalConnection::EndedByLocalCongestion,      "Local party congested" },
  { OpalConnection::EndedByRemoteBusy,           "Remote party busy" },
  { OpalConnection::EndedByRemoteCongestion,     "Remote switch congested" },
  { OpalConnection::EndedByUnreachable,          "Remote party could not be reached" },
  { OpalConnection::EndedByNoEndPoint,           "Remote party application is not running" },
  { OpalConnection::EndedByHostOffline,          "Remote party host is off line" },
  { OpalConnection::EndedByTemporaryFailure,     "Remote system failed temporarily" },
  { OpalConnection::EndedByQ931Cause,            "Call cleared with Q.931 cause code %u" },
  { OpalConnection::EndedByDurationLimit,        "Call cleared due to an enforced duration limit" },
  { OpalConnection::EndedByInvalidConferenceID,  "Call cleared due to invalid conference ID" },
  { OpalConnection::EndedByNoDialTone,           "Call cleared due to missing dial tone" },
  { OpalConnection::EndedByNoRingBackTone,       "Call cleared due to missing ringback tone" },
  { OpalConnection::EndedByOutOfService,         "Call cleared because the line is out of service" },
  { OpalConnection::EndedByAcceptingCallWaiting, "Call cleared because another call is answered" },
  { OpalConnection::EndedByGkAdmissionFailed,    "Call cleared because gatekeeper admission request failed." },
};

static POrdinalToString CallEndReasonStrings(PARRAYSIZE(CallEndReasonStringsInitialiser), CallEndReasonStringsInitialiser);


/////////////////////////////////////////////////////////////////////////////

OpalConnection::OpalConnection(OpalCall & call,
                               OpalEndPoint  & ep,
                               const PString & token,
                               unsigned int options,
                               OpalConnection::StringOptions * stringOptions)
  : PSafeObject(&call)  // Share the lock flag from the call
  , ownerCall(call)
  , endpoint(ep)
  , phase(UninitialisedPhase)
  , callToken(token)
  , originating(PFalse)
  , alertingTime(0)
  , connectedTime(0)
  , callEndTime(0)
  , productInfo(ep.GetProductInfo())
  , localPartyName(ep.GetDefaultLocalPartyName())
  , displayName(ep.GetDefaultDisplayName())
  , remotePartyName(token)
  , callEndReason(NumCallEndReasons)
  , synchronousOnRelease(true)
  , silenceDetector(NULL)
#if OPAL_AEC
  , echoCanceler(NULL)
#endif
#if OPAL_PTLIB_DTMF
  , m_dtmfScaleMultiplier(1)
  , m_dtmfScaleDivisor(1)
#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif
  , m_dtmfNotifier(PCREATE_NOTIFIER(OnDetectInBandDTMF))
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
  , m_sendInBandDTMF(true)
  , m_installedInBandDTMF(false)
  , m_emittedInBandDTMF(0)
#endif
#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif
#if OPAL_HAS_MIXER
  , m_recordAudioNotifier(PCREATE_NOTIFIER(OnRecordAudio))
#if OPAL_VIDEO
  , m_recordVideoNotifier(PCREATE_NOTIFIER(OnRecordVideo))
#endif
#endif
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
#if OPAL_STATISTICS
  , m_VideoUpdateRequestsSent(0)
#endif
#if OPAL_FAX
  , m_faxMediaStreamsSwitchState(e_NotSwitchingFaxMediaStreams)
#endif
{
  PTRACE(3, "OpalCon\tCreated connection " << *this);

  PAssert(ownerCall.SafeReference(), PLogicError);

  ownerCall.connectionsActive.Append(this);

  if (stringOptions != NULL)
    m_connStringOptions = *stringOptions;

  minAudioJitterDelay = endpoint.GetManager().GetMinAudioJitterDelay();
  maxAudioJitterDelay = endpoint.GetManager().GetMaxAudioJitterDelay();
  bandwidthAvailable = endpoint.GetInitialBandwidth();

#if OPAL_PTLIB_DTMF
  switch (options&DetectInBandDTMFOptionMask) {
    case DetectInBandDTMFOptionDisable :
      m_detectInBandDTMF = false;
      break;

    case DetectInBandDTMFOptionEnable :
      m_detectInBandDTMF = true;
      break;

    default :
      m_detectInBandDTMF = !endpoint.GetManager().DetectInBandDTMFDisabled();
      break;
  }
#endif

  switch (options & SendDTMFMask) {
    case SendDTMFAsString:
      sendUserInputMode = SendUserInputAsString;
      break;
    case SendDTMFAsTone:
      sendUserInputMode = SendUserInputAsTone;
      break;
    case SendDTMFAsRFC2833:
      sendUserInputMode = SendUserInputAsInlineRFC2833;
      break;
    case SendDTMFAsDefault:
    default:
      sendUserInputMode = ep.GetSendUserInputMode();
      break;
  }

#if OPAL_HAS_IM
  m_rfc4103Context[0].SetMediaFormat(OpalT140);
  m_rfc4103Context[1].SetMediaFormat(OpalT140);
#endif
}

OpalConnection::~OpalConnection()
{
  mediaStreams.RemoveAll();

  delete silenceDetector;
#if OPAL_AEC
  delete echoCanceler;
#endif
#if OPAL_T120DATA
  delete t120handler;
#endif

  ownerCall.connectionsActive.Remove(this);
  ownerCall.SafeDereference();

  PTRACE(3, "OpalCon\tConnection " << *this << " destroyed.");
}


void OpalConnection::PrintOn(ostream & strm) const
{
  strm << ownerCall << '-'<< endpoint << '[' << callToken << ']';
}


bool OpalConnection::GarbageCollection()
{
  return mediaStreams.DeleteObjectsToBeRemoved();
}


PBoolean OpalConnection::OnSetUpConnection()
{
  PTRACE(3, "OpalCon\tOnSetUpConnection" << *this);
  return endpoint.OnSetUpConnection(*this);
}


bool OpalConnection::HoldConnection()
{
  return false;
}


bool OpalConnection::RetrieveConnection()
{
  return false;
}


PBoolean OpalConnection::IsConnectionOnHold(bool /*fromRemote*/)
{
  return false;
}


void OpalConnection::OnHold(bool fromRemote, bool onHold)
{
  PTRACE(4, "OpalCon\tOnHold " << *this);
  endpoint.OnHold(*this, fromRemote, onHold);
}


PString OpalConnection::GetCallEndReasonText(CallEndReason reason)
{
  return psprintf(CallEndReasonStrings(reason.code), reason.q931);
}


void OpalConnection::SetCallEndReasonText(CallEndReasonCodes reasonCode, const PString & newText)
{
  CallEndReasonStrings.SetAt(reasonCode, newText);
}


void OpalConnection::SetCallEndReason(CallEndReason reason)
{
  // Only set reason if not already set to something
  if (callEndReason == NumCallEndReasons) {
    PTRACE(3, "OpalCon\tCall end reason for " << *this << " set to " << reason);
    callEndReason = reason;
    ownerCall.SetCallEndReason(reason);
  }
}


void OpalConnection::ClearCall(CallEndReason reason)
{
  // Now set reason for the connection close
  SetCallEndReason(reason);
  ownerCall.Clear(reason);
}


void OpalConnection::ClearCallSynchronous(PSyncPoint * sync, CallEndReason reason)
{
  // Now set reason for the connection close
  SetCallEndReason(reason);
  ownerCall.Clear(reason, sync);
}


bool OpalConnection::TransferConnection(const PString & PTRACE_PARAM(remoteParty))
{
  PTRACE(2, "OpalCon\tCan not transfer connection to " << remoteParty);
  return false;
}


void OpalConnection::Release(CallEndReason reason)
{
  {
    PWaitAndSignal m(phaseMutex);
    if (phase >= ReleasingPhase) {
      PTRACE(2, "OpalCon\tAlready released " << *this);
      return;
    }
    SetPhase(ReleasingPhase);
  }

  {
    PSafeLockReadWrite safeLock(*this);
    if (!safeLock.IsLocked()) {
      PTRACE(2, "OpalCon\tAlready released " << *this);
      return;
    }

    PTRACE(3, "OpalCon\tReleasing " << *this);

    // Now set reason for the connection close
    SetCallEndReason(reason);

    if (synchronousOnRelease) {
      OnReleased();
      return;
    }

    // Add a reference for the thread we are about to start
    SafeReference();
  }

  PThread::Create(PCREATE_NOTIFIER(OnReleaseThreadMain), 0,
                  PThread::AutoDeleteThread,
                  PThread::NormalPriority,
                  "OnRelease");
}


void OpalConnection::OnReleaseThreadMain(PThread &, INT)
{
  OnReleased();

  PTRACE(4, "OpalCon\tOnRelease thread completed for " << *this);

  // Dereference on the way out of the thread
  SafeDereference();
}


void OpalConnection::OnReleased()
{
  PTRACE(3, "OpalCon\tOnReleased " << *this);

  endpoint.OnReleased(*this);

  CloseMediaStreams();
}


PBoolean OpalConnection::OnIncomingConnection(unsigned options, OpalConnection::StringOptions * stringOptions)
{
  return endpoint.OnIncomingConnection(*this, options, stringOptions);
}


PString OpalConnection::GetDestinationAddress()
{
  return '*';
}


PBoolean OpalConnection::ForwardCall(const PString & /*forwardParty*/)
{
  return PFalse;
}


PSafePtr<OpalConnection> OpalConnection::GetOtherPartyConnection() const
{
  return GetCall().GetOtherPartyConnection(*this);
}


void OpalConnection::OnProceeding()
{
  endpoint.OnProceeding(*this);
}

void OpalConnection::OnAlerting()
{
  endpoint.OnAlerting(*this);
}

OpalConnection::AnswerCallResponse OpalConnection::OnAnswerCall(const PString & callerName)
{
  return endpoint.OnAnswerCall(*this, callerName);
}


void OpalConnection::AnsweringCall(AnswerCallResponse response)
{
  PTRACE(3, "OpalCon\tAnswering call: " << response);

  PSafeLockReadWrite safeLock(*this);
  // Can only answer call in UninitialisedPhase, SetUpPhase and AlertingPhase phases
  if (!safeLock.IsLocked() || GetPhase() > AlertingPhase)
    return;

  switch (response) {
    case AnswerCallDenied :
      // If response is denied, abort the call
      Release(EndedByAnswerDenied);
      break;

    case AnswerCallAlertWithMedia :
      SetAlerting(GetLocalPartyName(), PTrue);
      break;

    case AnswerCallPending :
      SetAlerting(GetLocalPartyName(), PFalse);
      break;

    case AnswerCallNow: 
      PTRACE(3, "OpalCon\tApplication has answered incoming call");
      GetOtherPartyConnection()->OnConnectedInternal();
      break;

    default : // AnswerCallDeferred etc
      break;
  }
}


PBoolean OpalConnection::SetConnected()
{
  PTRACE(3, "OpalCon\tSetConnected for " << *this);

  if (GetPhase() < ConnectedPhase) {
    SetPhase(ConnectedPhase);
    connectedTime = PTime();
  }

  if (!mediaStreams.IsEmpty() && GetPhase() < EstablishedPhase) {
    SetPhase(EstablishedPhase);
    OnEstablished();
  }

  return true;
}


void OpalConnection::OnConnectedInternal()
{
  if (GetPhase() < ConnectedPhase) {
    connectedTime = PTime();
    SetPhase(ConnectedPhase);
    OnConnected();
  }

  if (!mediaStreams.IsEmpty() && GetPhase() < EstablishedPhase) {
    SetPhase(EstablishedPhase);
    OnEstablished();
  }
}


void OpalConnection::OnConnected()
{
  PTRACE(3, "OpalCon\tOnConnected for " << *this);
  endpoint.OnConnected(*this);
}


void OpalConnection::OnEstablished()
{
  PTRACE(3, "OpalCon\tOnEstablished " << *this);
  StartMediaStreams();
  endpoint.OnEstablished(*this);
}


void OpalConnection::AdjustMediaFormats(bool local, OpalMediaFormatList & mediaFormats, OpalConnection * otherConnection) const
{
  if (otherConnection != NULL && otherConnection != this)
    otherConnection->AdjustMediaFormats(local, mediaFormats, NULL);
  else {
    mediaFormats.Remove(m_connStringOptions(OPAL_OPT_REMOVE_CODEC).Lines());
    endpoint.AdjustMediaFormats(local, *this, mediaFormats);
  }
}


unsigned OpalConnection::GetNextSessionID(const OpalMediaType & /*mediaType*/, bool /*isSource*/)
{
  return 0;
}


void OpalConnection::AutoStartMediaStreams(bool force)
{
  OpalMediaTypeFactory::KeyList_T mediaTypes = OpalMediaType::GetList();
  for (OpalMediaTypeFactory::KeyList_T::iterator iter = mediaTypes.begin(); iter != mediaTypes.end(); ++iter) {
    OpalMediaType mediaType = *iter;
    if ((GetAutoStart(mediaType)&OpalMediaType::Transmit) != 0 && (force || GetMediaStream(mediaType, true) == NULL))
      ownerCall.OpenSourceMediaStreams(*this, mediaType, mediaType.GetDefinition()->GetDefaultSessionId());
  }
  StartMediaStreams();
}


#if OPAL_FAX
bool OpalConnection::SwitchFaxMediaStreams(bool enableFax)
{
  if (m_faxMediaStreamsSwitchState != e_NotSwitchingFaxMediaStreams) {
    PTRACE(2, "OpalCon\tNested call to SwitchFaxMediaStreams on " << *this);
    return false;
  }

  PTRACE(3, "OpalCon\tSwitchFaxMediaStreams to " << (enableFax ? "fax" : "audio") << " on " << *this);
  OpalMediaFormat format = enableFax ? OpalT38 : OpalG711uLaw;
  if (!ownerCall.OpenSourceMediaStreams(*this, format.GetMediaType(), 1, format))
    return false;

  m_faxMediaStreamsSwitchState = enableFax ? e_SwitchingToFaxMediaStreams : e_SwitchingFromFaxMediaStreams;
  return true;
}


void OpalConnection::OnSwitchedFaxMediaStreams(bool enabledFax)
{
  if (m_faxMediaStreamsSwitchState != e_NotSwitchingFaxMediaStreams) {
    PTRACE(3, "OpalCon\tSwitch of media streams to "
           << (m_faxMediaStreamsSwitchState == e_SwitchingToFaxMediaStreams ? "fax" : "audio") << ' '
           << (enabledFax != (m_faxMediaStreamsSwitchState == e_SwitchingToFaxMediaStreams) ? "failed" : "succeeded")
           << " on " << *this);

    m_faxMediaStreamsSwitchState = e_NotSwitchingFaxMediaStreams;

    PSafePtr<OpalConnection> other = GetOtherPartyConnection();
    if (other != NULL)
      other->OnSwitchedFaxMediaStreams(enabledFax);
  }
}
#endif


OpalMediaStreamPtr OpalConnection::OpenMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return NULL;

  // See if already opened
  OpalMediaStreamPtr stream = GetMediaStream(sessionID, isSource);
  if (stream != NULL && stream->IsOpen()) {
    if (stream->GetMediaFormat() == mediaFormat) {
      PTRACE(3, "OpalCon\tOpenMediaStream (already opened) for session " << sessionID << " on " << *this);
      return stream;
    }
    // Changing the media format, needs to close and re-open the stream
    stream->Close();
    stream.SetNULL();
  }

  if (stream == NULL) {
    stream = CreateMediaStream(mediaFormat, sessionID, isSource);
    if (stream == NULL) {
      PTRACE(1, "OpalCon\tCreateMediaStream returned NULL for session " << sessionID << " on " << *this);
      return NULL;
    }
    mediaStreams.Append(stream);
  }

  if (stream->Open()) {
    if (OnOpenMediaStream(*stream)) {
      PTRACE(3, "OpalCon\tOpened " << (isSource ? "source" : "sink") << " stream " << stream->GetID() << " with format " << mediaFormat);
      return stream;
    }
    PTRACE(2, "OpalCon\tOnOpenMediaStream failed for " << mediaFormat << ", closing " << *stream);
    stream->Close();
  }
  else {
    PTRACE(2, "OpalCon\tSource media stream open failed for " << *stream << " (" << mediaFormat << ')');
  }

  mediaStreams.Remove(stream);

  return NULL;
}

OpalMediaStreamPtr OpalConnection::GetMediaStreamByUid(const OpalMediaType & mediaType,
                                                      bool source, unsigned int uid 
                                                       ) 
{
  
  OpalMediaStreamPtr  mediaStream = OpalMediaStreamPtr(mediaStreams, PSafeReference);
 
  while (mediaStream != NULL) {
    if ((mediaType.empty() || mediaStream->GetMediaFormat().GetMediaType() == mediaType) && mediaStream->IsSource() == source && mediaStream->m_uid == uid)
      return mediaStream;
    ++mediaStream;
  }

  return NULL;
}
bool OpalConnection::CloseMediaStream(unsigned sessionId, bool source)
{
  OpalMediaStreamPtr stream = GetMediaStream(sessionId, source);
  return stream != NULL && stream->IsOpen() && CloseMediaStream(*stream);
}


bool OpalConnection::CloseMediaStream(OpalMediaStream & stream)
{
  if (!stream.Close())
    return false;

  if (stream.IsSource())
    return true;

  PSafePtr<OpalConnection> otherConnection = GetOtherPartyConnection();
  if (otherConnection == NULL)
    return true;

  OpalMediaStreamPtr otherStream = otherConnection->GetMediaStream(stream.GetSessionID(), true);
  if (otherStream == NULL )
    return true;
  if ( GetCall().GetCallType() == em_CallYYMeetingConf)
    return true;
  return otherStream->Close();
}


PBoolean OpalConnection::RemoveMediaStream(OpalMediaStream & stream)
{
  stream.Close();
  PTRACE(3, "OpalCon\tRemoved media stream " << stream);
  return mediaStreams.Remove(&stream);
}


void OpalConnection::StartMediaStreams()
{
  for (OpalMediaStreamPtr mediaStream = mediaStreams; mediaStream != NULL; ++mediaStream)
    mediaStream->Start();

  PTRACE(3, "OpalCon\tMedia stream threads started.");
}


void OpalConnection::CloseMediaStreams()
{
  // Do this double loop as while closing streams, the instance may disappear from the
  // mediaStreams list, prematurely stopping the for loop.
  bool someOpen = true;
  while (someOpen) {
    someOpen = false;
    for (OpalMediaStreamPtr mediaStream(mediaStreams, PSafeReference); mediaStream != NULL; ++mediaStream) {
      if (mediaStream->IsOpen()) {
        someOpen = true;
        CloseMediaStream(*mediaStream);
      }
    }
  }

  PTRACE(3, "OpalCon\tMedia streams closed.");
}


void OpalConnection::PauseMediaStreams(PBoolean paused)
{
  for (OpalMediaStreamPtr mediaStream = mediaStreams; mediaStream != NULL; ++mediaStream)
    mediaStream->SetPaused(paused);
}


OpalMediaStream * OpalConnection::CreateMediaStream(
#if OPAL_VIDEO
  const OpalMediaFormat & mediaFormat,
  unsigned sessionID,
  PBoolean isSource
#else
  const OpalMediaFormat & ,
  unsigned ,
  PBoolean 
#endif
  )
{
#if OPAL_VIDEO
  if (mediaFormat.GetMediaType() == OpalMediaType::Video()) {
    if (isSource) {
      PVideoInputDevice * videoDevice=NULL;
      PBoolean autoDelete;
      if (CreateVideoInputDevice(mediaFormat, videoDevice, autoDelete)) {
        PVideoOutputDevice * previewDevice=NULL;
        if (!CreateVideoOutputDevice(mediaFormat, PTrue, previewDevice, autoDelete))
          previewDevice = NULL;
        return new OpalVideoMediaStream(*this, mediaFormat, sessionID, videoDevice, previewDevice, autoDelete);
      }
    }
    else {
      PVideoOutputDevice * videoDevice;
      PBoolean autoDelete;
      if (CreateVideoOutputDevice(mediaFormat, PFalse, videoDevice, autoDelete))
        return new OpalVideoMediaStream(*this, mediaFormat, sessionID, NULL, videoDevice, autoDelete);
    }
  }
#endif

  return NULL;
}


PBoolean OpalConnection::OnOpenMediaStream(OpalMediaStream & stream)
{
  if (!endpoint.OnOpenMediaStream(*this, stream))
    return PFalse;

  if (!LockReadWrite())
    return PFalse;

  if (GetPhase() == ConnectedPhase) {
    SetPhase(EstablishedPhase);
    OnEstablished();
  }

  UnlockReadWrite();

  return PTrue;
}


void OpalConnection::OnClosedMediaStream(const OpalMediaStream & stream)
{
#if OPAL_HAS_MIXER
  OnStopRecording(stream.GetPatch());
#endif
  endpoint.OnClosedMediaStream(stream);
}

#if OPAL_HAS_MIXER
extern PString GetUserNameFromRemoteParty(const PString & str);
 PString MakeRecordingKey(const OpalConnection & con,const OpalMediaPatch * patch)
{
  unsigned int Caller= con.GetLocalPartyName().AsUnsigned();

  unsigned int Callee= con.GetRemotePartyNumber().AsUnsigned();
  if ( Callee == 0) {
    Callee = g_MyClient->GetID();
  }
  if (Caller == 0){
    Caller = g_MyClient->GetID();
  }
  return psprintf("%u_%u_%s_%u",Caller ,Callee, (const char*)patch->GetSource().GetMediaFormat().GetName(), (unsigned int ) patch);

  //return psprintf("%s_%s_%s", (const char*)GetUserNameFromRemoteParty(con.GetLocalPartyURL())  , (const char*)con.GetRemotePartyNumber() , (const char*)patch->GetSource().GetMediaFormat().GetName());
}

//void yStartRecording(SafeList<OpalMediaStream> mediaStreams )
//{
//  for (PSafePtr<OpalMediaStream> mediaStream(mediaStreams, PSafeReference); mediaStream != NULL; ++mediaStream) {
//    OpalMediaStream *p=   mediaStream;
//     patch->AddFilter(m_recordAudioNotifier, p-/*OPAL_PCM16*/);
//    //if ((streamID.IsEmpty() || mediaStream->GetID() == streamID) && mediaStream->IsSource() == source)
//    //  return mediaStream;
//  }
//}
void OpalConnection::OnStartRecording(OpalMediaPatch * patch)
{
  if (patch == NULL)
    return;
#ifdef D_DISABLE_OPAL_RECORD

  patch->AddFilter(m_recordAudioNotifier, YYCODE_CORE/*OPAL_PCM16*/);
  #if OPAL_VIDEO
    patch->AddFilter(m_recordVideoNotifier, OPAL_H264/*OPAL_YUV420P*/);
  #endif
#else
  patch->SetRecording(true);
#endif

  if (!ownerCall.OnStartRecording(MakeRecordingKey(*this, patch), patch->GetSource().GetMediaFormat())) {
    PTRACE(4, "OpalCon\tNo record filter added on connection " << *this << ", patch " << *patch);
    return;
  }
  PTRACE(4, "OpalCon\tAdded record filter on connection " << *this << ", patch " << *patch);
}


void OpalConnection::OnStopRecording(OpalMediaPatch * patch)
{
  if (patch == NULL)
    return;

  ownerCall.OnStopRecording(MakeRecordingKey(*this, patch));
#ifdef D_DISABLE_OPAL_RECORD

  patch->RemoveFilter(m_recordAudioNotifier, YYCODE_CORE);
#if OPAL_VIDEO
  patch->RemoveFilter(m_recordVideoNotifier, OPAL_H264);
#endif
#else
  patch->SetRecording(false);
#endif
  PTRACE(4, "OpalCon\tRemoved record filter on " << *patch);
}

#endif

void OpalConnection::OnPatchMediaStream(PBoolean isSource, OpalMediaPatch & patch)
{
  patch.SetCommandNotifier(PCREATE_NOTIFIER(OnMediaCommand), !isSource);

  OpalMediaFormat mediaFormat = patch.GetSource().GetMediaFormat();
  if (mediaFormat.GetMediaType() == OpalMediaType::Audio()) {
    PTRACE(3, "OpalCon\tAdding audio filters to patch " << patch);

    if (isSource && silenceDetector != NULL) {
      silenceDetector->SetParameters(endpoint.GetManager().GetSilenceDetectParams(), mediaFormat.GetClockRate());
      patch.AddFilter(silenceDetector->GetReceiveHandler(), mediaFormat);
    }
#if OPAL_AEC
    if (echoCanceler) {
      echoCanceler->SetParameters(endpoint.GetManager().GetEchoCancelParams());
      echoCanceler->SetClockRate(mediaFormat.GetClockRate());
      patch.AddFilter(isSource ? echoCanceler->GetReceiveHandler() : echoCanceler->GetSendHandler(), OpalPCM16);
    }
#endif

#if OPAL_PTLIB_DTMF
    if (m_detectInBandDTMF && isSource) {
      patch.AddFilter(m_dtmfNotifier, OPAL_PCM16);
      PTRACE(4, "OpalCon\tAdded detect DTMF filter on connection " << *this << ", patch " << patch);
    }

    if (m_sendInBandDTMF && !isSource) {
      m_installedInBandDTMF = true;
      patch.AddFilter(PCREATE_NOTIFIER(OnSendInBandDTMF), OPAL_PCM16);
      PTRACE(4, "OpalCon\tAdded send DTMF filter on connection " << *this << ", patch " << patch);
    }
#endif
  }

#if OPAL_HAS_MIXER
  if (!m_recordingFilename.IsEmpty())
    ownerCall.StartRecording(m_recordingFilename);
  else if (ownerCall.IsRecording())
    OnStartRecording(&patch);
#endif

  PTRACE(3, "OpalCon\t" << (isSource ? "Source" : "Sink") << " stream of connection " << *this << " uses patch " << patch);
}


#if OPAL_HAS_MIXER
void OpalConnection::EnableRecording()
{
  OpalMediaStreamPtr stream = GetMediaStream(OpalMediaType::Audio(), true);
  if (stream != NULL)
    OnStartRecording(stream->GetPatch());

  stream = GetMediaStream(OpalMediaType::Video(), true);
  if (stream != NULL)
    OnStartRecording(stream->GetPatch());
}


void OpalConnection::DisableRecording()
{
  OpalMediaStreamPtr stream = GetMediaStream(OpalMediaType::Audio(), true);
  if (stream != NULL)
    OnStopRecording(stream->GetPatch());

  stream = GetMediaStream(OpalMediaType::Video(), true);
  if (stream != NULL)
    OnStopRecording(stream->GetPatch());
}

void OpalConnection::OnRecordAudio(RTP_DataFrame & frame_o, INT param)
{
#if 0
  //PTimeInterval tick = PTimer::Tick();
  //RTP_DataFrame frame_o = frame_oo;
  //frame_o.SetTimestamp(tick.GetInterval());
  int nOragnalRTPLen = frame_o.GetPayloadSize()+ RTP_DataFrame::MinHeaderSize;
  const OpalMediaPatch * patch = (const OpalMediaPatch *)param;
  RTP_DataFrame frame;frame.SetPayloadSize(nOragnalRTPLen +D_YYM_RTP_HEADER_LEN - RTP_DataFrame::MinHeaderSize);
  memcpy(frame.GetPointer()+D_YYM_RTP_HEADER_LEN , frame_o.GetPointer(),nOragnalRTPLen );

  stRecordRTP* pHeader =(stRecordRTP*) (frame.GetPointer() );
  memset(pHeader,0, D_YYM_RTP_HEADER_LEN);
  pHeader->mediatype = em_audio;
  pHeader->rtpLen = nOragnalRTPLen;
  pHeader->timestamp = PTimer::Tick().GetInterval();

  pHeader->Caller= GetLocalPartyName().AsUnsigned();
  pHeader->Callee= GetRemotePartyNumber().AsUnsigned();
  //if (pHeader->Callee == 0) {
  //  pHeader->Callee= pHeader->Caller = g_MyClient->GetID();
  //}

  if (m_strAliasCaller.IsEmpty() ||m_strAliasCaller.GetLength() <=0 ){
    BBQUserInfo user={0};
    g_MyClient->ViewUserData(pHeader->Caller, user, false);
    m_strAliasCaller = user.basic.alias;
    if (pHeader->Callee==0)  m_strAliasCallee= m_strAliasCaller;
  }
  if (m_strAliasCallee.IsEmpty() || m_strAliasCallee.GetLength() <=0 ){
    BBQUserInfo user={0};
    g_MyClient->ViewUserData(pHeader->Callee, user, false);
    m_strAliasCallee = user.basic.alias;
  } 
  strncpy(pHeader->strAliasCaller, (const char*)m_strAliasCaller, sizeof(pHeader->strAliasCaller));
  strncpy(pHeader->strAliasCallee, (const char*)m_strAliasCallee, sizeof(pHeader->strAliasCallee));
  ownerCall.OnRecordAudio(MakeRecordingKey(*this, patch), frame);
#endif
}


#if OPAL_VIDEO

void OpalConnection::OnRecordVideo(RTP_DataFrame & frame_o, INT param)
{
#if 0
  int nOragnalRTPLen = frame_o.GetPayloadSize()+ RTP_DataFrame::MinHeaderSize;
  const OpalMediaPatch * patch = (const OpalMediaPatch *)param;
  RTP_DataFrame frame;frame.SetPayloadSize(nOragnalRTPLen +D_YYM_RTP_HEADER_LEN - RTP_DataFrame::MinHeaderSize);
  memcpy(frame.GetPointer()+D_YYM_RTP_HEADER_LEN , frame_o.GetPointer(),nOragnalRTPLen );
   
  stRecordRTP* pHeader =(stRecordRTP*) (frame.GetPointer() );
  memset(pHeader,0, D_YYM_RTP_HEADER_LEN);
  pHeader->mediatype = em_video;
  pHeader->rtpLen = nOragnalRTPLen;
  pHeader->timestamp = PTimer::Tick().GetInterval();

  pHeader->Caller=  GetLocalPartyName().AsUnsigned();
  pHeader->Callee= GetRemotePartyNumber().AsUnsigned();
  //if ( pHeader->Callee == 0) {
  //  pHeader->Caller = pHeader->Callee = g_MyClient->GetID();
  //}

  if (m_strAliasCaller.GetLength() <=0 ){
    BBQUserInfo user={0};
    g_MyClient->ViewUserData(pHeader->Caller, user, false);
    m_strAliasCaller = user.basic.alias;
    if (pHeader->Callee==0)  m_strAliasCallee= m_strAliasCaller;
  }
  if (m_strAliasCallee.GetLength() <=0 ){
    BBQUserInfo user={0};
    g_MyClient->ViewUserData(pHeader->Callee, user, false);
    m_strAliasCallee = user.basic.alias;
  } 
  strncpy(pHeader->strAliasCaller, (const char*)m_strAliasCaller, sizeof(pHeader->strAliasCaller));
  strncpy(pHeader->strAliasCallee, (const char*)m_strAliasCallee, sizeof(pHeader->strAliasCallee));
  ownerCall.OnRecordVideo(MakeRecordingKey(*this, patch), frame);

#endif
//  PTRACE(3, "yRecordMgr\t seq=" << frame_o.GetSequenceNumber() << ",payload=" <<frame_o.GetPayloadSize()<< ", mediatype=" <<( pHeader->mediatype == em_video?"video":"audio") <<",caller=" <<pHeader->Caller << ",callee=" <<pHeader->Callee  );
}

#endif

#endif


void OpalConnection::AttachRFC2833HandlerToPatch(PBoolean /*isSource*/, OpalMediaPatch & /*patch*/)
{
}


OpalMediaStreamPtr OpalConnection::GetMediaStream(const PString & streamID, bool source) const
{
  for (PSafePtr<OpalMediaStream> mediaStream(mediaStreams, PSafeReference); mediaStream != NULL; ++mediaStream) {
    if ((streamID.IsEmpty() || mediaStream->GetID() == streamID) && mediaStream->IsSource() == source)
      return mediaStream;
  }

  return NULL;
}


OpalMediaStreamPtr OpalConnection::GetMediaStream(unsigned sessionId, bool source) const
{
  for (OpalMediaStreamPtr mediaStream(mediaStreams, PSafeReference); mediaStream != NULL; ++mediaStream) {
    if (mediaStream->GetSessionID() == sessionId && mediaStream->IsSource() == source)
      return mediaStream;
  }

  return NULL;
}


OpalMediaStreamPtr OpalConnection::GetMediaStream(const OpalMediaType & mediaType,
                                                  bool source,
                                                  OpalMediaStreamPtr mediaStream) const
{
  if (mediaStream == NULL)
    mediaStream = OpalMediaStreamPtr(mediaStreams, PSafeReference);
  else
    ++mediaStream;

  while (mediaStream != NULL) {
    if ((mediaType.empty() || mediaStream->GetMediaFormat().GetMediaType() == mediaType) && mediaStream->IsSource() == source)
      return mediaStream;
    ++mediaStream;
  }

  return NULL;
}


PBoolean OpalConnection::IsMediaBypassPossible(unsigned /*sessionID*/) const
{
  PTRACE(4, "OpalCon\tIsMediaBypassPossible: default returns false");
  return false;
}


#if OPAL_VIDEO

PBoolean OpalConnection::CreateVideoInputDevice(const OpalMediaFormat & mediaFormat,
                                            PVideoInputDevice * & device,
                                            PBoolean & autoDelete)
{
  return endpoint.CreateVideoInputDevice(*this, mediaFormat, device, autoDelete);
}


PBoolean OpalConnection::CreateVideoOutputDevice(const OpalMediaFormat & mediaFormat,
                                             PBoolean preview,
                                             PVideoOutputDevice * & device,
                                             PBoolean & autoDelete)
{
  return endpoint.CreateVideoOutputDevice(*this, mediaFormat, preview, device, autoDelete);
}

#endif // OPAL_VIDEO


PBoolean OpalConnection::SetAudioVolume(PBoolean /*source*/, unsigned /*percentage*/)
{
  return PFalse;
}


unsigned OpalConnection::GetAudioSignalLevel(PBoolean /*source*/)
{
    return UINT_MAX;
}

PBoolean OpalConnection::SetBandwidthAvailable(unsigned newBandwidth, PBoolean force)
{
  PTRACE(3, "OpalCon\tSetting bandwidth to " << newBandwidth << "00b/s on connection " << *this);

  unsigned used = GetBandwidthUsed();
  if (used > newBandwidth) {
    if (!force)
      return PFalse;

#if 0
    // Go through media channels and close down some.
    PINDEX chanIdx = GetmediaStreams->GetSize();
    while (used > newBandwidth && chanIdx-- > 0) {
      OpalChannel * channel = logicalChannels->GetChannelAt(chanIdx);
      if (channel != NULL) {
        used -= channel->GetBandwidthUsed();
        const H323ChannelNumber & number = channel->GetNumber();
        CloseLogicalChannel(number, number.IsFromRemote());
      }
    }
#endif
  }

  bandwidthAvailable = newBandwidth - used;
  return PTrue;
}


unsigned OpalConnection::GetBandwidthUsed() const
{
  unsigned used = 0;

#if 0
  for (PINDEX i = 0; i < logicalChannels->GetSize(); i++) {
    OpalChannel * channel = logicalChannels->GetChannelAt(i);
    if (channel != NULL)
      used += channel->GetBandwidthUsed();
  }
#endif

  PTRACE(3, "OpalCon\tBandwidth used is "
         << used << "00b/s for " << *this);

  return used;
}


PBoolean OpalConnection::SetBandwidthUsed(unsigned releasedBandwidth,
                                      unsigned requiredBandwidth)
{
  PTRACE_IF(3, releasedBandwidth > 0, "OpalCon\tBandwidth release of "
            << releasedBandwidth/10 << '.' << releasedBandwidth%10 << "kb/s");

  bandwidthAvailable += releasedBandwidth;

  PTRACE_IF(3, requiredBandwidth > 0, "OpalCon\tBandwidth request of "
            << requiredBandwidth/10 << '.' << requiredBandwidth%10
            << "kb/s, available: "
            << bandwidthAvailable/10 << '.' << bandwidthAvailable%10
            << "kb/s");

  if (requiredBandwidth > bandwidthAvailable) {
    PTRACE(2, "OpalCon\tAvailable bandwidth exceeded on " << *this);
    return PFalse;
  }

  bandwidthAvailable -= requiredBandwidth;

  return PTrue;
}

void OpalConnection::SetSendUserInputMode(SendUserInputModes mode)
{
  PTRACE(3, "OPAL\tSetting default User Input send mode to " << mode);
  sendUserInputMode = mode;
}

PBoolean OpalConnection::SendUserInputString(const PString & value)
{
  for (const char * c = value; *c != '\0'; c++) {
    if (!SendUserInputTone(*c, 0))
      return PFalse;
  }
  return PTrue;
}


#if OPAL_PTLIB_DTMF
PBoolean OpalConnection::SendUserInputTone(char tone, unsigned duration)
{
  if (!m_installedInBandDTMF)
    return false;

  if (duration <= 0)
    duration = PDTMFEncoder::DefaultToneLen;

  PTRACE(3, "OPAL\tSending in-band DTMF tone '" << tone << "', duration=" << duration);
  m_inBandMutex.Wait();
  m_inBandDTMF.AddTone(tone, duration);
  m_inBandMutex.Signal();

  return true;
}
#else
PBoolean OpalConnection::SendUserInputTone(char /*tone*/, unsigned /*duration*/)
{
  return false;
}
#endif


void OpalConnection::OnUserInputString(const PString & value)
{
  endpoint.OnUserInputString(*this, value);
}


void OpalConnection::OnUserInputTone(char tone, unsigned duration)
{
  endpoint.OnUserInputTone(*this, tone, duration);
}


PString OpalConnection::GetUserInput(unsigned timeout)
{
  PString reply;
  if (userInputAvailable.Wait(PTimeInterval(0, timeout)) &&
      GetPhase() < ReleasingPhase &&
      LockReadWrite()) {
    reply = userInputString;
    userInputString = PString();
    UnlockReadWrite();
  }
  return reply;
}


void OpalConnection::SetUserInput(const PString & input)
{
  if (LockReadWrite()) {
    userInputString += input;
    userInputAvailable.Signal();
    UnlockReadWrite();
  }
}


PString OpalConnection::ReadUserInput(const char * terminators,
                                      unsigned lastDigitTimeout,
                                      unsigned firstDigitTimeout)
{
  return endpoint.ReadUserInput(*this, terminators, lastDigitTimeout, firstDigitTimeout);
}


PBoolean OpalConnection::PromptUserInput(PBoolean /*play*/)
{
  return PTrue;
}


#if OPAL_PTLIB_DTMF
void OpalConnection::OnDetectInBandDTMF(RTP_DataFrame & frame, INT)
{
  // This function is set up as an 'audio filter'.
  // This allows us to access the 16 bit PCM audio (at 8Khz sample rate)
  // before the audio is passed on to the sound card (or other output device)

  // Pass the 16 bit PCM audio through the DTMF decoder   
  PString tones = m_dtmfDecoder.Decode((const short *)frame.GetPayloadPtr(),
                                       frame.GetPayloadSize()/sizeof(short),
                                       m_dtmfScaleMultiplier,
                                       m_dtmfScaleDivisor);
  if (!tones.IsEmpty()) {
    PTRACE(3, "OPAL\tDTMF detected: \"" << tones << '"');
    for (PINDEX i = 0; i < tones.GetLength(); i++)
      OnUserInputTone(tones[i], PDTMFDecoder::DetectTime);
  }
}

void OpalConnection::OnSendInBandDTMF(RTP_DataFrame & frame, INT)
{
  if (m_inBandDTMF.IsEmpty())
    return;

  // Can't do the usual LockReadWrite() as deadlocks
  m_inBandMutex.Wait();

  PINDEX bytes = (m_inBandDTMF.GetSize() - m_emittedInBandDTMF)*sizeof(short);
  if (bytes > frame.GetPayloadSize())
    bytes = frame.GetPayloadSize();
  memcpy(frame.GetPayloadPtr(), &m_inBandDTMF[m_emittedInBandDTMF], bytes);

  m_emittedInBandDTMF += bytes/sizeof(short);

  if (m_emittedInBandDTMF >= m_inBandDTMF.GetSize()) {
    PTRACE(4, "OPAL\tSent in-band DTMF tone, " << m_inBandDTMF.GetSize() << " samples");
    m_inBandDTMF.SetSize(0);
    m_emittedInBandDTMF = 0;
  }

  m_inBandMutex.Signal();
}
#endif


PString OpalConnection::GetPrefixName() const
{
  return endpoint.GetPrefixName();
}


void OpalConnection::SetLocalPartyName(const PString & name)
{
  localPartyName = name;
}


PString OpalConnection::GetLocalPartyURL() const
{
  return GetPrefixName() + ':' + PURL::TranslateString(GetLocalPartyName(), PURL::LoginTranslation);
}


bool OpalConnection::IsPresentationBlocked() const
{
  return m_connStringOptions.GetBoolean(OPAL_OPT_PRESENTATION_BLOCK);
}


static PString MakeURL(const PString & prefix, const PString & partyName)
{
  if (partyName.IsEmpty())
    return PString::Empty();

  PINDEX colon = partyName.Find(':');
  if (colon != P_MAX_INDEX && partyName.FindSpan("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") == colon)
    return partyName;

  PStringStream url;
  url << prefix << ':' << partyName;
  return url;
}


PString OpalConnection::GetRemotePartyURL() const
{
  return MakeURL(GetPrefixName(), GetRemotePartyAddress());
}


PString OpalConnection::GetCalledPartyURL()
{
  return MakeURL(GetPrefixName(), GetDestinationAddress());
}


void OpalConnection::CopyPartyNames(const OpalConnection & other)
{
  remotePartyName     = other.remotePartyName;
  remotePartyNumber   = other.remotePartyNumber;
  remotePartyAddress  = other.remotePartyAddress;
  m_calledPartyName   = other.m_calledPartyName;
  m_calledPartyNumber = other.m_calledPartyNumber;
  remoteProductInfo   = other.remoteProductInfo;
}


PString OpalConnection::GetAlertingType() const
{
  return PString::Empty();
}


bool OpalConnection::SetAlertingType(const PString & /*info*/)
{
  return false;
}


void OpalConnection::SetAudioJitterDelay(unsigned minDelay, unsigned maxDelay)
{
  maxDelay = PMAX(10, PMIN(maxDelay, 999));
  minDelay = PMAX(10, PMIN(minDelay, 999));

  if (maxDelay < minDelay)
    maxDelay = minDelay;

  minAudioJitterDelay = minDelay;
  maxAudioJitterDelay = maxDelay;
}


PString OpalConnection::GetIdentifier() const
{
  return GetToken();
}


PINDEX OpalConnection::GetMaxRtpPayloadSize() const
{ 
  return endpoint.GetManager().GetMaxRtpPayloadSize(); 
}


void OpalConnection::SetPhase(Phases phaseToSet)
{
  PTRACE(3, "OpalCon\tSetPhase from " << phase << " to " << phaseToSet << " for " << *this);

  PWaitAndSignal m(phaseMutex);

  // With next few lines we will prevent phase to ever go down when it
  // reaches ReleasingPhase - end result - once you call Release you never
  // go back.
  if (phase < ReleasingPhase || (phase == ReleasingPhase && phaseToSet == ReleasedPhase))
    phase = phaseToSet;
}


void OpalConnection::SetStringOptions(const StringOptions & options, bool overwrite)
{
  if (overwrite)
    m_connStringOptions = options;
  else {
    for (PINDEX i = 0; i < options.GetSize(); ++i)
      m_connStringOptions.SetAt(options.GetKeyAt(i), options.GetDataAt(i));
  }
}

void OpalConnection::OnApplyStringOptions()
{
  endpoint.GetManager().OnApplyStringOptions(*this, m_connStringOptions);
}

void OpalConnection::ApplyStringOptions(OpalConnection::StringOptions & stringOptions)
{
  PTRACE(4, "OpalCon\tApplying string options:\n" << stringOptions);

  if (LockReadWrite()) {
    PCaselessString str;

    m_connStringOptions = stringOptions;

#if OPAL_PTLIB_DTMF
    m_sendInBandDTMF   = stringOptions.GetBoolean(OPAL_OPT_ENABLE_INBAND_DTMF, m_sendInBandDTMF);
    m_detectInBandDTMF = stringOptions.GetBoolean(OPAL_OPT_DETECT_INBAND_DTMF, m_detectInBandDTMF);
    m_sendInBandDTMF   = stringOptions.GetBoolean(OPAL_OPT_SEND_INBAND_DTMF,   m_sendInBandDTMF);

    m_dtmfScaleMultiplier = stringOptions.GetInteger(OPAL_OPT_DTMF_MULT, m_dtmfScaleMultiplier);
    m_dtmfScaleDivisor    = stringOptions.GetInteger(OPAL_OPT_DTMF_DIV,  m_dtmfScaleDivisor);
#endif

    m_autoStartInfo.Initialise(stringOptions);

    if (stringOptions.GetBoolean(OPAL_OPT_DISABLE_JITTER))
      maxAudioJitterDelay = minAudioJitterDelay = 0;
    else {
      maxAudioJitterDelay = stringOptions.GetInteger(OPAL_OPT_MAX_JITTER, maxAudioJitterDelay);
      minAudioJitterDelay = stringOptions.GetInteger(OPAL_OPT_MIN_JITTER, minAudioJitterDelay);
    }

#if OPAL_HAS_MIXER
    if (stringOptions.Contains(OPAL_OPT_RECORD_AUDIO))
      m_recordingFilename = m_connStringOptions(OPAL_OPT_RECORD_AUDIO);
#endif

    str = stringOptions(OPAL_OPT_ALERTING_TYPE);
    if (!str.IsEmpty())
      SetAlertingType(str);

    UnlockReadWrite();
  }
}


OpalMediaFormatList OpalConnection::GetMediaFormats() const
{
  return endpoint.GetMediaFormats();
}


OpalMediaFormatList OpalConnection::GetLocalMediaFormats()
{
  return ownerCall.GetMediaFormats(*this, FALSE);
}


void OpalConnection::OnStartMediaPatch(OpalMediaPatch & patch)
{
  GetEndPoint().GetManager().OnStartMediaPatch(*this, patch);
}


void OpalConnection::OnStopMediaPatch(OpalMediaPatch & patch)
{
  GetEndPoint().GetManager().OnStopMediaPatch(*this, patch);
}


void OpalConnection::OnMediaCommand(OpalMediaCommand & /*command*/, INT /*extra*/)
{
}


OpalMediaType::AutoStartMode OpalConnection::GetAutoStart(const OpalMediaType & mediaType) const
{
  return m_autoStartInfo.GetAutoStart(mediaType);
}


OpalConnection::AutoStartMap::AutoStartMap()
{
  m_initialised = false;
}


void OpalConnection::AutoStartMap::Initialise(const OpalConnection::StringOptions & stringOptions)
{
  PWaitAndSignal m(m_mutex);

  // make function idempotent
  if (m_initialised)
    return;

  m_initialised = true;

  // get autostart option as lines
  PStringArray lines = stringOptions(OPAL_OPT_AUTO_START).Lines();
  for (PINDEX i = 0; i < lines.GetSize(); ++i) {
    PString line = lines[i];
    PINDEX colon = line.Find(':');
    OpalMediaType mediaType = line.Left(colon);

    // see if media type is known, and if it is, enable it
    OpalMediaTypeDefinition * def = mediaType.GetDefinition();
    if (def != NULL) {
      if (colon == P_MAX_INDEX) 
        SetAutoStart(mediaType, OpalMediaType::ReceiveTransmit);
      else {
        PStringArray tokens = line.Mid(colon+1).Tokenise(";", FALSE);
        PINDEX j;
        for (j = 0; j < tokens.GetSize(); ++j) {
          if ((tokens[i] *= "no") || (tokens[i] *= "false") || (tokens[i] *= "0"))
            SetAutoStart(mediaType, OpalMediaType::DontOffer);
          else if ((tokens[i] *= "yes") || (tokens[i] *= "true") || (tokens[i] *= "1") || (tokens[i] *= "sendrecv"))
            SetAutoStart(mediaType, OpalMediaType::ReceiveTransmit);
          else if (tokens[i] *= "recvonly")
            SetAutoStart(mediaType, OpalMediaType::Receive);
          else if (tokens[i] *= "sendonly")
            SetAutoStart(mediaType, OpalMediaType::Transmit);
          else if (tokens[i] *= "offer")
            SetAutoStart(mediaType, OpalMediaType::OfferInactive);
          else if (tokens[i] *= "exclusive") {
            OpalMediaTypeFactory::KeyList_T types = OpalMediaType::GetList();
            for (OpalMediaTypeFactory::KeyList_T::iterator r = types.begin(); r != types.end(); ++r)
              SetAutoStart(*r, *r == mediaType ? OpalMediaType::ReceiveTransmit : OpalMediaType::DontOffer);
          }
        }
      }
    }
  }
}


void OpalConnection::AutoStartMap::SetAutoStart(const OpalMediaType & mediaType, OpalMediaType::AutoStartMode autoStart)
{
  PWaitAndSignal m(m_mutex);
  m_initialised = true;

  // deconflict session ID
  unsigned sessionID = mediaType.GetDefinition()->GetDefaultSessionId();
  if (size() == 0) {
    if (sessionID == 0) 
      sessionID = 1;
  }
  else {
    iterator r = begin();
    while (r != end()) {
      if (r->second.preferredSessionId != sessionID) 
        ++r;
      else {
        ++sessionID;
        r = begin();
      }
    }
  }

  AutoStartInfo info;
  info.autoStart = autoStart;
  info.preferredSessionId = sessionID;

  insert(value_type(mediaType, info));
}


OpalMediaType::AutoStartMode OpalConnection::AutoStartMap::GetAutoStart(const OpalMediaType & mediaType) const
{
  PWaitAndSignal m(m_mutex);
  const_iterator r = find(mediaType);
  return r == end() ? mediaType.GetAutoStart() : r->second.autoStart;
}


#if OPAL_HAS_IM

bool OpalConnection::TransmitInternalIM(const OpalMediaFormat & format, RTP_IMFrame & frame)
{
  // if the call contains any compatible IM stream, use it
  for (OpalMediaStreamPtr mediaStream(mediaStreams, PSafeReference); mediaStream != NULL; ++mediaStream) {
    if (mediaStream->IsSource() && (mediaStream->GetMediaFormat() == format)) {
      PTRACE(3, "OpalCon\tSending " << format << " IM message within call");
      return mediaStream->GetPatch()->PushFrame(frame);
    }
  }

  // otherwise push the frame directly to the other connection
  PSafePtr<OpalConnection> other = GetOtherPartyConnection();
  if (other != NULL) {
    PTRACE(3, "OpalCon\tSending " << format << " IM message directly to other connection");
    other->OnReceiveInternalIM(format, frame);
  }
  return true;
}


void OpalConnection::OnReceiveInternalIM(const OpalMediaFormat & format, RTP_IMFrame & rtp)
{
  TransmitExternalIM(format, rtp);
}


bool OpalConnection::TransmitExternalIM(const OpalMediaFormat & format, RTP_IMFrame & frame)
{
  return endpoint.TransmitExternalIM(*this, format, frame);
}

bool OpalConnection::OnReceiveExternalIM(const OpalMediaFormat & format, RTP_IMFrame & body)
{
  return TransmitInternalIM(format, body);
}

#endif


bool OpalConnection::StringOptions::GetBoolean(const char * key, bool dflt) const
{
  PString * value = GetAt(key);
  if (value == NULL)
    return dflt;

  return value->IsEmpty() || (*value *= "true") || (*value *= "yes") || value->AsInteger() != 0;
}


long OpalConnection::StringOptions::GetInteger(const char * key, long dflt) const
{
  PString * value = GetAt(key);
  if (value == NULL)
    return dflt;

  return value->AsInteger();
}


void OpalConnection::StringOptions::ExtractFromURL(PURL & url)
{
  PStringToString params = url.GetParamVars();
  params.MakeUnique();
  for (PINDEX i = 0; i < params.GetSize(); ++i) {
    PCaselessString key = params.GetKeyAt(i);
    if (key.NumCompare("OPAL-") == EqualTo) {
      SetAt(key.Mid(5), params.GetDataAt(i));
      url.SetParamVar(key, PString::Empty());
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
