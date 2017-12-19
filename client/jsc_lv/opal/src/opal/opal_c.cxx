/*
 * opal_c.cxx
 *
 * "C" language interface for OPAL
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2008 Vox Lucida
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
 * The Initial Developer of the Original Code is Vox Lucida (Robert Jongbloed)
 *
 * This code was initially written with the assisance of funding from
 * Stonevoice. http://www.stonevoice.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23938 $
 * $Author: rjongbloed $
 * $Date: 2010-01-14 05:06:54 +0000 (Thu, 14 Jan 2010) $
 */

#include <ptlib.h>

#include <opal/buildopts.h>

#include <opal.h>
#include <opal/manager.h>

#if OPAL_HAS_PCSS
#include <opal/pcss.h>
#endif

#include <opal/localep.h>
#include <h323/h323ep.h>
#include <sip/sipep.h>
#include <iax2/iax2ep.h>
#include <lids/lidep.h>
#include <t38/t38proto.h>
#include <opal/ivr.h>

#include <queue>


class OpalManager_C;


ostream & operator<<(ostream & strm, OpalRegistrationStates state)
{
  static const char * States[] = { "Successful", "Removed", "Failed", "Retrying", "Restored" };
  return strm << States[state];
}


inline bool IsNullString(const char * str)
{
  return str == NULL || *str == '\0';
}


class OpalMessageBuffer
{
  public:
    OpalMessageBuffer(OpalMessageType type);
    ~OpalMessageBuffer();

    OpalMessage * operator->() const { return  (OpalMessage *)m_data; }
    OpalMessage & operator *() const { return *(OpalMessage *)m_data; }
    operator OpalMessage *() const   { return  (OpalMessage *)m_data; }

    void SetString(const char * * variable, const char * value);
    void SetError(const char * errorText);

    OpalMessage * Detach();

  private:
    size_t m_size;
    char * m_data;
    std::vector<size_t> m_strPtrOffset;
};

#define SET_MESSAGE_STRING(msg, member, str) (msg).SetString(&(msg)->member, str)


#if OPAL_PTLIB_AUDIO
#if OPAL_HAS_PCSS

class OpalPCSSEndPoint_C : public OpalPCSSEndPoint
{
  public:
    OpalPCSSEndPoint_C(OpalManager_C & manager);

    virtual PBoolean OnShowIncoming(const OpalPCSSConnection &);
    virtual PBoolean OnShowOutgoing(const OpalPCSSConnection &);

  private:
    OpalManager_C & m_manager;
};

#endif
#endif // OPAL_PTLIB_AUDIO


class OpalLocalEndPoint_C : public OpalLocalEndPoint
{
  public:
    OpalLocalEndPoint_C(OpalManager_C & manager);

    virtual bool OnOutgoingCall(const OpalLocalConnection &);
    virtual bool OnIncomingCall(OpalLocalConnection &);
    virtual bool OnReadMediaFrame(const OpalLocalConnection &, const OpalMediaStream & mediaStream, RTP_DataFrame &);
    virtual bool OnWriteMediaFrame(const OpalLocalConnection &, const OpalMediaStream &, RTP_DataFrame & frame);
    virtual bool OnReadMediaData(const OpalLocalConnection &, const OpalMediaStream &, void *, PINDEX, PINDEX &);
    virtual bool OnWriteMediaData(const OpalLocalConnection &, const OpalMediaStream &, const void *, PINDEX, PINDEX &);
    virtual bool IsSynchronous() const;

    OpalMediaDataFunction m_mediaReadData;
    OpalMediaDataFunction m_mediaWriteData;
    OpalMediaDataType     m_mediaDataHeader;
    OpalMediaTiming       m_mediaTiming;

  private:
    OpalManager_C & m_manager;
};


#if OPAL_SIP
class SIPEndPoint_C : public SIPEndPoint
{
  public:
    SIPEndPoint_C(OpalManager_C & manager);

    virtual void OnRegistrationStatus(
      const RegistrationStatus & status
    );
    virtual void OnSubscriptionStatus(
      const PString & eventPackage, ///< Event package subscribed to
      const SIPURL & uri,           ///< Target URI for the subscription.
      bool wasSubscribing,          ///< Indication the subscribing or unsubscribing
      bool reSubscribing,           ///< If subscribing then indication was refeshing subscription
      SIP_PDU::StatusCodes reason   ///< Status of subscription
    );
    virtual void OnDialogInfoReceived(
      const SIPDialogNotification & info  ///< Information on dialog state change
    );

  private:
    OpalManager_C & m_manager;
};
#endif


class OpalManager_C : public OpalManager
{
  public:
    OpalManager_C(unsigned version)
      : localEP(NULL)
#if OPAL_PTLIB_AUDIO
#if OPAL_HAS_PCSS
      , pcssEP(NULL)
#endif
#endif
      , m_apiVersion(version)
      , m_manualAlerting(false)
      , m_messagesAvailable(0, INT_MAX)
    {
    }

    ~OpalManager_C()
    {
      ShutDownEndpoints();
    }

    bool Initialise(const PCaselessString & options);

    void PostMessage(OpalMessageBuffer & message);
    OpalMessage * GetMessage(unsigned timeout);
    OpalMessage * SendMessage(const OpalMessage * message);

    virtual void OnEstablishedCall(OpalCall & call);
    virtual PBoolean OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream);
    virtual void OnClosedMediaStream(const OpalMediaStream & stream);
    virtual void OnUserInputString(OpalConnection & connection, const PString & value);
    virtual void OnUserInputTone(OpalConnection & connection, char tone, int duration);
    virtual void OnMWIReceived(const PString & party, MessageWaitingType type, const PString & extraInfo);
    virtual void OnProceeding(OpalConnection & conenction);
    virtual void OnClearedCall(OpalCall & call);

    bool IsManualAlerting() const { return m_manualAlerting; }

  private:
    void HandleSetGeneral    (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleSetProtocol   (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleRegistration  (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleSetUpCall     (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleAlerting      (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleAnswerCall    (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleUserInput     (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleClearCall     (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleHoldCall      (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleRetrieveCall  (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleTransferCall  (const OpalMessage & message, OpalMessageBuffer & response);
    void HandleMediaStream   (const OpalMessage & command, OpalMessageBuffer & response);
    void HandleSetUserData   (const OpalMessage & command, OpalMessageBuffer & response);
    void HandleStartRecording(const OpalMessage & command, OpalMessageBuffer & response);
    void HandleStopRecording (const OpalMessage & command, OpalMessageBuffer & response);

    void OnIndMediaStream(const OpalMediaStream & stream, OpalMediaStates state);

    bool FindCall(const char * token, OpalMessageBuffer & response, PSafePtr<OpalCall> & call);

    OpalLocalEndPoint_C * localEP;
#if OPAL_PTLIB_AUDIO
#if OPAL_HAS_PCSS
    OpalPCSSEndPoint_C  * pcssEP;
#endif
#endif

    unsigned                  m_apiVersion;
    bool                      m_manualAlerting;
    std::queue<OpalMessage *> m_messageQueue;
    PMutex                    m_messageMutex;
    PSemaphore                m_messagesAvailable;
    OpalMessageAvailableFunction m_messageAvailableCallback;
};


class PProcess_C : public PProcess
{
public:
  PProcess_C(const PCaselessString & options)
  {
#if PTRACING
    unsigned level = 0;
    static char const TraceLevelKey[] = "TraceLevel=";
    PINDEX pos = options.Find(TraceLevelKey);
    if (pos != P_MAX_INDEX)
      level = options.Mid(pos+sizeof(TraceLevelKey)-1).AsUnsigned();

#ifdef WIN32
    PString filename = "DEBUGSTREAM";
#else
    PString filename = "stderr";
#endif
    static char const TraceFileKey[] = "TraceFile=";
    pos = options.Find(TraceFileKey);
    if (pos != P_MAX_INDEX) {
      pos += sizeof(TraceFileKey) - 1;
      PINDEX end;
      if (options[pos] == '"')
        end = options.Find('"', ++pos);
      else
        end = options.Find(' ', pos);
      filename = options(pos, end-1);
    }

    unsigned traceOpts = PTrace::Timestamp|PTrace::Thread|PTrace::Blocks;
    if (options.Find("TraceAppend") != P_MAX_INDEX)
      traceOpts |= PTrace::AppendToFile;

    PTrace::Initialise(level, filename, traceOpts);
    PTRACE(3, "OpalC\tStart Up.");
#endif
  }

  ~PProcess_C()
  {
#if PTRACING
    PTRACE(3, "OpalC\tShut Down.");
    PTrace::SetStream(NULL);
#endif
  }

private:
  virtual void Main()
  {
  }
};

struct OpalHandleStruct
{
  OpalHandleStruct(unsigned version, const PCaselessString & options)
    : process(options)
    , manager(version)
  {
  }

  PProcess_C     process;
  OpalManager_C  manager;
};


///////////////////////////////////////////////////////////////////////////////

OpalMessageBuffer::OpalMessageBuffer(OpalMessageType type)
  : m_size(sizeof(OpalMessage))
  , m_data((char *)malloc(m_size))
{
  memset(m_data, 0, m_size);
  (*this)->m_type = type;
}


OpalMessageBuffer::~OpalMessageBuffer()
{
  if (m_data != NULL)
    free(m_data);
}


void OpalMessageBuffer::SetString(const char * * variable, const char * value)
{
  PAssert((char *)variable >= m_data && (char *)variable < m_data+m_size, PInvalidParameter);

  size_t length = strlen(value)+1;

  char * newData = (char *)realloc(m_data, m_size + length);
  if (PAssertNULL(newData) != m_data) {
    // Memory has moved, this invalidates pointer variables so recalculate them
    int delta = newData - m_data;
    char * endData = m_data + m_size;
    for (size_t i = 0; i < m_strPtrOffset.size(); ++i) {
      const char ** ptr = (const char **)(newData + m_strPtrOffset[i]);
      if (*ptr >= m_data && *ptr < endData)
        *ptr += delta;
    }
    variable += delta/sizeof(char *);
    m_data = newData;
  }

  char * stringData = m_data + m_size;
  memcpy(stringData, value, length);
  m_size += length;

  *variable = stringData;

  m_strPtrOffset.push_back((char *)variable - m_data);
}


void OpalMessageBuffer::SetError(const char * errorText)
{
  OpalMessage * message = (OpalMessage *)m_data;
  PTRACE(2, "OpalC API\tCommand " << message->m_type << " error: " << errorText);

  message->m_type = OpalIndCommandError;
  m_strPtrOffset.clear();
  SetString(&message->m_param.m_commandError, errorText);
}


OpalMessage * OpalMessageBuffer::Detach()
{
  OpalMessage * message = (OpalMessage *)m_data;
  m_data = NULL;
  return message;
}


PString BuildProductName(const OpalProductInfo & info)
{
  if (info.comments.IsEmpty())
    return info.name;
  if (info.comments[0] == '(')
    return info.name + ' ' + info.comments;
  return info.name + " (" + info.comments + ')';
}


///////////////////////////////////////

OpalLocalEndPoint_C::OpalLocalEndPoint_C(OpalManager_C & mgr)
  : OpalLocalEndPoint(mgr)
  , m_mediaReadData(NULL)
  , m_mediaWriteData(NULL)
  , m_mediaDataHeader(OpalMediaDataPayloadOnly)
  , m_mediaTiming(OpalMediaTimingSynchronous)
  , m_manager(mgr)
{
  m_deferredAlerting = mgr.IsManualAlerting();
}


static void SetOutgoingCallInfo(OpalMessageBuffer & message, const OpalConnection & connection)
{
  const OpalCall & call = connection.GetCall();
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_partyA, call.GetPartyA());
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_partyB, call.GetPartyB());
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_callToken, call.GetToken());
  PTRACE(4, "OpalC API\tOnOutgoingCall:"
            " token=\"" << message->m_param.m_callSetUp.m_callToken << "\""
            " A=\""     << message->m_param.m_callSetUp.m_partyA << "\""
            " B=\""     << message->m_param.m_callSetUp.m_partyB << '"');
}


bool OpalLocalEndPoint_C::OnOutgoingCall(const OpalLocalConnection & connection)
{
  OpalMessageBuffer message(OpalIndAlerting);
  SetOutgoingCallInfo(message, connection);
  m_manager.PostMessage(message);
  return true;
}


static void SetIncomingCallInfo(OpalMessageBuffer & message, const OpalConnection & connection)
{
  PSafePtr<OpalConnection> network = connection.GetOtherPartyConnection();
  PAssert(network != NULL, PLogicError); // Should not happen!

  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_callToken, connection.GetCall().GetToken());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_localAddress, network->GetLocalPartyURL());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_remoteAddress, network->GetRemotePartyURL());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_remotePartyNumber, network->GetRemotePartyNumber());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_remoteDisplayName, network->GetRemotePartyName());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_calledAddress, network->GetCalledPartyURL());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_calledPartyNumber, network->GetCalledPartyNumber());

  const OpalProductInfo & info = connection.GetProductInfo();
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_product.m_vendor,  info.vendor);
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_product.m_name,    BuildProductName(info));
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_product.m_version, info.version);

  message->m_param.m_incomingCall.m_product.m_t35CountryCode   = info.t35CountryCode;
  message->m_param.m_incomingCall.m_product.m_t35Extension     = info.t35Extension;
  message->m_param.m_incomingCall.m_product.m_manufacturerCode = info.manufacturerCode;

  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_alertingType,   network->GetAlertingType());
  SET_MESSAGE_STRING(message, m_param.m_incomingCall.m_protocolCallId, connection.GetIdentifier());

  PTRACE(4, "OpalC API\tOpalIndIncomingCall: token=\""  << message->m_param.m_incomingCall.m_callToken << "\"\n"
            "  Local  - URL=\"" << message->m_param.m_incomingCall.m_localAddress << "\"\n"
            "  Remote - URL=\"" << message->m_param.m_incomingCall.m_remoteAddress << "\""
                    " E.164=\"" << message->m_param.m_incomingCall.m_remotePartyNumber << "\""
                  " Display=\"" << message->m_param.m_incomingCall.m_remoteDisplayName << "\"\n"
            "  Dest.  - URL=\"" << message->m_param.m_incomingCall.m_calledAddress << "\""
                    " E.164=\"" << message->m_param.m_incomingCall.m_calledPartyNumber << "\"\n"
            "  AlertingType=\"" << message->m_param.m_incomingCall.m_alertingType << "\"\n"
            "        CallID=\"" << message->m_param.m_incomingCall.m_protocolCallId << '"');
}


bool OpalLocalEndPoint_C::OnIncomingCall(OpalLocalConnection & connection)
{
  OpalMessageBuffer message(OpalIndIncomingCall);
  SetIncomingCallInfo(message, connection);
  m_manager.PostMessage(message);
  return true;
}


bool OpalLocalEndPoint_C::OnReadMediaFrame(const OpalLocalConnection & connection,
                                           const OpalMediaStream & mediaStream,
                                           RTP_DataFrame & frame)
{
  if (m_mediaDataHeader != OpalMediaDataWithHeader)
    return false;

  if (m_mediaReadData == NULL)
    return false;

  int result = m_mediaReadData(connection.GetCall().GetToken(),
                               mediaStream.GetID(),
                               mediaStream.GetMediaFormat().GetName(),
                               connection.GetUserData(),
                               frame.GetPointer(),
                               frame.GetSize());
  if (result < 0)
    return false;

  frame.SetPayloadSize(result-frame.GetHeaderSize());
  return true;
}


bool OpalLocalEndPoint_C::OnWriteMediaFrame(const OpalLocalConnection & connection,
                                            const OpalMediaStream & mediaStream,
                                            RTP_DataFrame & frame)
{
  if (m_mediaDataHeader != OpalMediaDataWithHeader)
    return false;

  if (m_mediaWriteData == NULL)
    return false;

  int result = m_mediaWriteData(connection.GetCall().GetToken(),
                                mediaStream.GetID(),
                                mediaStream.GetMediaFormat().GetName(),
                                connection.GetUserData(),
                                frame.GetPointer(),
                                frame.GetHeaderSize()+frame.GetPayloadSize());
  return result >= 0;
}


bool OpalLocalEndPoint_C::OnReadMediaData(const OpalLocalConnection & connection,
                                          const OpalMediaStream & mediaStream,
                                          void * data,
                                          PINDEX size,
                                          PINDEX & length)
{
  if (m_mediaDataHeader != OpalMediaDataPayloadOnly)
    return false;

  if (m_mediaReadData == NULL)
    return false;

  int result = m_mediaReadData(connection.GetCall().GetToken(),
                               mediaStream.GetID(),
                               mediaStream.GetMediaFormat().GetName(),
                               connection.GetUserData(),
                               data,
                               size);
  if (result < 0)
    return false;

  length = result;
  return true;
}


bool OpalLocalEndPoint_C::OnWriteMediaData(const OpalLocalConnection & connection,
                                           const OpalMediaStream & mediaStream,
                                           const void * data,
                                           PINDEX length,
                                           PINDEX & written)
{
  if (m_mediaDataHeader != OpalMediaDataPayloadOnly)
    return false;

  if (m_mediaWriteData == NULL)
    return false;

  int result = m_mediaWriteData(connection.GetCall().GetToken(),
                                mediaStream.GetID(),
                                mediaStream.GetMediaFormat().GetName(),
                                connection.GetUserData(),
                                (void *)data,
                                length);
  if (result < 0)
    return false;

  written = result;
  return true;
}


bool OpalLocalEndPoint_C::IsSynchronous() const
{
  return m_mediaTiming == OpalMediaTimingSynchronous;
}


///////////////////////////////////////

#if OPAL_PTLIB_AUDIO
#if OPAL_HAS_PCSS

OpalPCSSEndPoint_C::OpalPCSSEndPoint_C(OpalManager_C & mgr)
  : OpalPCSSEndPoint(mgr)
  , m_manager(mgr)
{
  //m_deferredAlerting = mgr.IsManualAlerting();
}


PBoolean OpalPCSSEndPoint_C::OnShowIncoming(const OpalPCSSConnection & connection)
{
  OpalMessageBuffer message(OpalIndIncomingCall);
  SetIncomingCallInfo(message, connection);
  m_manager.PostMessage(message);
  return true;
}


PBoolean OpalPCSSEndPoint_C::OnShowOutgoing(const OpalPCSSConnection & connection)
{
  OpalMessageBuffer message(OpalIndAlerting);
  SetOutgoingCallInfo(message, connection);
  m_manager.PostMessage(message);
  return true;
}

#endif
#endif // OPAL_PTLIB_AUDIO


///////////////////////////////////////

#if OPAL_SIP

SIPEndPoint_C::SIPEndPoint_C(OpalManager_C & mgr)
  : SIPEndPoint(mgr)
  , m_manager(mgr)
{
}


void SIPEndPoint_C::OnRegistrationStatus(const RegistrationStatus & status)
{
  SIPEndPoint::OnRegistrationStatus(status);

  OpalMessageBuffer message(OpalIndRegistration);
  SET_MESSAGE_STRING(message, m_param.m_registrationStatus.m_protocol, OPAL_PREFIX_SIP);
  SET_MESSAGE_STRING(message, m_param.m_registrationStatus.m_serverName, status.m_addressofRecord);

  SET_MESSAGE_STRING(message, m_param.m_registrationStatus.m_product.m_vendor,  status.m_productInfo.vendor);
  SET_MESSAGE_STRING(message, m_param.m_registrationStatus.m_product.m_name,    BuildProductName(status.m_productInfo));
  SET_MESSAGE_STRING(message, m_param.m_registrationStatus.m_product.m_version, status.m_productInfo.version);

  message->m_param.m_registrationStatus.m_product.m_t35CountryCode   = status.m_productInfo.t35CountryCode;
  message->m_param.m_registrationStatus.m_product.m_t35Extension     = status.m_productInfo.t35Extension;
  message->m_param.m_registrationStatus.m_product.m_manufacturerCode = status.m_productInfo.manufacturerCode;

  if (status.m_reason == SIP_PDU::Information_Trying)
    message->m_param.m_registrationStatus.m_status = OpalRegisterRetrying;
  else if (status.m_reason/100 == 2) {
    if (status.m_wasRegistering)
      message->m_param.m_registrationStatus.m_status = status.m_reRegistering ? OpalRegisterRestored : OpalRegisterSuccessful;
    else
      message->m_param.m_registrationStatus.m_status = OpalRegisterRemoved;
  }
  else {
    PStringStream strm;
    strm << "Error " << status.m_reason << " in SIP ";
    if (!status.m_wasRegistering)
      strm << "un";
    strm << "registration.";
    SET_MESSAGE_STRING(message, m_param.m_registrationStatus.m_error, strm);
    message->m_param.m_registrationStatus.m_status = status.m_wasRegistering ? OpalRegisterFailed : OpalRegisterRemoved;
  }
  PTRACE(4, "OpalC\tOnRegistrationStatus " << status.m_addressofRecord << ", status=" << message->m_param.m_registrationStatus.m_status);
  m_manager.PostMessage(message);
}


void SIPEndPoint_C::OnSubscriptionStatus(const PString & eventPackage,
                                         const SIPURL & uri,
                                         bool wasSubscribing,
                                         bool reSubscribing,
                                         SIP_PDU::StatusCodes reason)
{
  SIPEndPoint::OnSubscriptionStatus(eventPackage, uri, wasSubscribing, reSubscribing, reason);

  if (reason == SIP_PDU::Successful_OK && !reSubscribing) {
    if (SIPEventPackage(SIPSubscribe::MessageSummary) == eventPackage) {
      OpalMessageBuffer message(OpalIndMessageWaiting);
      SET_MESSAGE_STRING(message, m_param.m_messageWaiting.m_party, uri.AsString());
      SET_MESSAGE_STRING(message, m_param.m_messageWaiting.m_extraInfo, wasSubscribing ? "SUBSCRIBED" : "UNSUBSCRIBED");
      PTRACE(4, "OpalC API\tOnSubscriptionStatus - MWI: party=\"" << message->m_param.m_messageWaiting.m_party
                                              << "\" info=" << message->m_param.m_messageWaiting.m_extraInfo);
      m_manager.PostMessage(message);
    }
    else if (SIPEventPackage(SIPSubscribe::Dialog) == eventPackage) {
      OpalMessageBuffer message(OpalIndLineAppearance);
      SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_line, uri.AsString());
      message->m_param.m_lineAppearance.m_state = wasSubscribing ? OpalLineSubcribed : OpalLineUnsubcribed;
      PTRACE(4, "OpalC API\tOnSubscriptionStatus - LineAppearance: line=\"" << message->m_param.m_lineAppearance.m_line);
      m_manager.PostMessage(message);
    }
  }
}


static PString GetParticipantName(const SIPDialogNotification::Participant & participant)
{
  PStringStream strm;
  strm << '"' << participant.m_display << "\" <" << participant.m_URI << '>';
  return strm;
}


void SIPEndPoint_C::OnDialogInfoReceived(const SIPDialogNotification & info)
{
  SIPEndPoint::OnDialogInfoReceived(info);

  OpalMessageBuffer message(OpalIndLineAppearance);
  SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_line, info.m_entity);
  message->m_param.m_lineAppearance.m_state = (OpalLineAppearanceStates)info.m_state;
  message->m_param.m_lineAppearance.m_appearance = info.m_local.m_appearance;

  if (info.m_initiator) {
    SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_callId, info.m_callId+";to-tag="+info.m_remote.m_dialogTag+";from-tag="+info.m_local.m_dialogTag);
    SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_partyA, GetParticipantName(info.m_local));
    SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_partyB, GetParticipantName(info.m_remote));
  }
  else {
    SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_callId, info.m_callId+";to-tag="+info.m_local.m_dialogTag+";from-tag="+info.m_remote.m_dialogTag);
    SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_partyA, GetParticipantName(info.m_remote));
    SET_MESSAGE_STRING(message, m_param.m_lineAppearance.m_partyB, GetParticipantName(info.m_local));
  }

  PTRACE(4, "OpalC API\tOnDialogInfoReceived: entity=\"" << message->m_param.m_lineAppearance.m_line
                                          << "\" callId=" << message->m_param.m_lineAppearance.m_callId);
  m_manager.PostMessage(message);
}


#endif

///////////////////////////////////////

bool OpalManager_C::Initialise(const PCaselessString & options)
{
  PString defProto, defUser;
  PINDEX  defProtoPos = P_MAX_INDEX, defUserPos = P_MAX_INDEX;

#if OPAL_H323
  PINDEX h323Pos = options.Find("h323");
  if (h323Pos < defProtoPos) {
    defProto = "h323";
    defProtoPos = h323Pos;
  }
#endif

#if OPAL_SIP
  PINDEX sipPos = options.Find("sip");
  if (sipPos < defProtoPos) {
    defProto = "sip";
    defProtoPos = sipPos;
  }
#endif

#if OPAL_IAX2
  PINDEX iaxPos = options.Find("iax2");
  if (iaxPos < defProtoPos) {
    defProto = "iax2:<da>";
    defProtoPos = iaxPos;
  }
#endif

#if OPAL_LID
  PINDEX potsPos = options.Find("pots");
  if (potsPos < defUserPos) {
    defUser = "pots:<dn>";
    defUserPos = potsPos;
  }

  PINDEX pstnPos = options.Find("pstn");
  if (pstnPos < defProtoPos) {
    defProto = "pstn:<dn>";
    defProtoPos = pstnPos;
  }
#endif

#if OPAL_FAX
  PINDEX faxPos = options.Find("fax");
  if (faxPos < defUserPos) {
    defUser = "fax:";
    defUserPos = faxPos;
  }

  PINDEX t38Pos = options.Find("t38");
  if (t38Pos < defProtoPos) {
    defUser = "t38:";
    defProtoPos = t38Pos;
  }
#endif

  PINDEX pcPos = options.Find("pc");
  if (pcPos < defUserPos) {
    defUser = "pc:*";
    defUserPos = pcPos;
  }

  PINDEX localPos = options.Find("local");
  if (localPos < defUserPos) {
    defUser = "local:<du>";
    defUserPos = localPos;
  }


#if OPAL_IVR
  if (options.Find("ivr") != P_MAX_INDEX) {
    new OpalIVREndPoint(*this);
    AddRouteEntry(".*:#=ivr:"); // A hash from anywhere goes to IVR
  }
#endif

#if OPAL_H323
  if (h323Pos != P_MAX_INDEX) {
    new H323EndPoint(*this);
    AddRouteEntry("h323:.*=" + defUser);
  }
#endif

#if OPAL_SIP
  if (sipPos != P_MAX_INDEX) {
    new SIPEndPoint_C(*this);
    AddRouteEntry("sip:.*=" + defUser);
  }
#endif

#if OPAL_IAX2
  if (options.Find("iax2") != P_MAX_INDEX) {
    new IAX2EndPoint(*this);
    AddRouteEntry("iax2:.*=" + defUser);
  }
#endif

#if OPAL_LID
  if (potsPos != P_MAX_INDEX || pstnPos != P_MAX_INDEX) {
    new OpalLineEndPoint(*this);

    if (potsPos != P_MAX_INDEX)
      AddRouteEntry("pots:.*=" + defProto + ":<da>");
    if (pstnPos != P_MAX_INDEX)
      AddRouteEntry("pstn:.*=" + defUser + ":<da>");
  }
#endif

#if OPAL_FAX
  if (faxPos != P_MAX_INDEX || t38Pos != P_MAX_INDEX) {
    new OpalFaxEndPoint(*this);

    if (faxPos != P_MAX_INDEX)
      AddRouteEntry("fax:.*=" + defProto + ":<da>");
    if (t38Pos != P_MAX_INDEX)
      AddRouteEntry("t38:.*=" + defUser + ":<da>");
  }
#endif

#if OPAL_PTLIB_AUDIO
#if OPAL_HAS_PCSS
  if (pcPos != P_MAX_INDEX) {
    pcssEP = new OpalPCSSEndPoint_C(*this);
    AddRouteEntry("pc:.*=" + defProto + ":<da>");
  }
#endif
#endif

  if (localPos != P_MAX_INDEX) {
    localEP = new OpalLocalEndPoint_C(*this);
    AddRouteEntry("local:.*=" + defProto + ":<da>");
  }

  return true;
}


void OpalManager_C::PostMessage(OpalMessageBuffer & message)
{
  m_messageMutex.Wait();
  if (m_messageAvailableCallback == NULL || m_messageAvailableCallback(message)) {
    m_messageQueue.push(message.Detach());
    m_messagesAvailable.Signal();
  }
  m_messageMutex.Signal();
}


OpalMessage * OpalManager_C::GetMessage(unsigned timeout)
{
  OpalMessage * msg = NULL;

  if (m_messagesAvailable.Wait(timeout)) {
    m_messageMutex.Wait();

    if (!m_messageQueue.empty()) {
      msg = m_messageQueue.front();
      m_messageQueue.pop();
    }

    m_messageMutex.Signal();
  }

  return msg;
}


OpalMessage * OpalManager_C::SendMessage(const OpalMessage * message)
{
  if (message == NULL)
    return NULL;

  OpalMessageBuffer response(message->m_type);

  switch (message->m_type) {
    case OpalCmdSetGeneralParameters :
      HandleSetGeneral(*message, response);
      break;
    case OpalCmdSetProtocolParameters :
      HandleSetProtocol(*message, response);
      break;
    case OpalCmdRegistration :
      HandleRegistration(*message, response);
      break;
    case OpalCmdSetUpCall :
      HandleSetUpCall(*message, response);
      break;
    case OpalCmdAnswerCall :
      HandleAnswerCall(*message, response);
      break;
    case OpalCmdUserInput :
      HandleUserInput(*message, response);
      break;
    case OpalCmdClearCall :
      HandleClearCall(*message, response);
      break;
    case OpalCmdHoldCall :
      HandleHoldCall(*message, response);
      break;
    case OpalCmdRetrieveCall :
      HandleRetrieveCall(*message, response);
      break;
    case OpalCmdTransferCall :
      HandleTransferCall(*message, response);
      break;
    case OpalCmdMediaStream :
      HandleMediaStream(*message, response);
      break;
    case OpalCmdSetUserData :
      HandleSetUserData(*message, response);
      break;
    case OpalCmdStartRecording :
      HandleStartRecording(*message, response);
      break;
    case OpalCmdStopRecording :
      HandleStopRecording(*message, response);
      break;
    default :
      return NULL;
  }

  return response.Detach();
}


void OpalManager_C::HandleSetGeneral(const OpalMessage & command, OpalMessageBuffer & response)
{
#if OPAL_PTLIB_AUDIO
#if OPAL_HAS_PCSS
  if (pcssEP != NULL) {
    SET_MESSAGE_STRING(response, m_param.m_general.m_audioRecordDevice, pcssEP->GetSoundChannelRecordDevice());
    if (!IsNullString(command.m_param.m_general.m_audioRecordDevice))
      pcssEP->SetSoundChannelRecordDevice(command.m_param.m_general.m_audioRecordDevice);

    SET_MESSAGE_STRING(response, m_param.m_general.m_audioPlayerDevice, pcssEP->GetSoundChannelPlayDevice());
    if (!IsNullString(command.m_param.m_general.m_audioPlayerDevice))
      pcssEP->SetSoundChannelPlayDevice(command.m_param.m_general.m_audioPlayerDevice);
  }
#endif
#endif // OPAL_PTLIB_AUDIO

#if OPAL_VIDEO
  PVideoDevice::OpenArgs video = GetVideoInputDevice();
  SET_MESSAGE_STRING(response, m_param.m_general.m_videoInputDevice, video.deviceName);
  if (!IsNullString(command.m_param.m_general.m_videoInputDevice)) {
    video.deviceName = command.m_param.m_general.m_videoInputDevice;
    SetVideoInputDevice(video);
  }

  video = GetVideoOutputDevice();
  SET_MESSAGE_STRING(response, m_param.m_general.m_videoOutputDevice, video.deviceName);
  if (!IsNullString(command.m_param.m_general.m_videoOutputDevice)) {
    video.deviceName = command.m_param.m_general.m_videoOutputDevice;
    SetVideoOutputDevice(video);
  }

  video = GetVideoPreviewDevice();
  SET_MESSAGE_STRING(response, m_param.m_general.m_videoPreviewDevice, video.deviceName);
  if (!IsNullString(command.m_param.m_general.m_videoPreviewDevice)) {
    video.deviceName = command.m_param.m_general.m_videoPreviewDevice;
    SetVideoPreviewDevice(video);
  }
#endif // OPAL_VIDEO

  PStringStream strm;
  strm << setfill('\n') << GetMediaFormatOrder();
  SET_MESSAGE_STRING(response, m_param.m_general.m_mediaOrder, strm);
  if (!IsNullString(command.m_param.m_general.m_mediaOrder))
    SetMediaFormatOrder(PString(command.m_param.m_general.m_mediaOrder).Lines());

  strm.flush();
  strm << setfill('\n') << GetMediaFormatMask();
  SET_MESSAGE_STRING(response, m_param.m_general.m_mediaMask, strm);
  if (!IsNullString(command.m_param.m_general.m_mediaMask))
    SetMediaFormatMask(PString(command.m_param.m_general.m_mediaMask).Lines());

  OpalMediaTypeFactory::KeyList_T allMediaTypes = OpalMediaType::GetList();

  for (OpalMediaType::AutoStartMode autoStart = OpalMediaType::Receive; autoStart < OpalMediaType::ReceiveTransmit; ++autoStart) {
    strm.MakeEmpty();

    OpalMediaTypeFactory::KeyList_T::iterator iterMediaType;
    for (iterMediaType = allMediaTypes.begin(); iterMediaType != allMediaTypes.end(); ++iterMediaType) {
      OpalMediaTypeDefinition * definition = OpalMediaType::GetDefinition(*iterMediaType);
      if ((definition->GetAutoStart()&autoStart) != 0) {
        if (!strm.IsEmpty())
          strm << ' ';
        strm << *iterMediaType;
        definition->SetAutoStart(autoStart, false);
      }
    }

    PString autoXxMedia;
    if (autoStart == OpalMediaType::Receive) {
      SET_MESSAGE_STRING(response, m_param.m_general.m_autoRxMedia, strm);
      autoXxMedia = command.m_param.m_general.m_autoRxMedia;
    }
    else {
      SET_MESSAGE_STRING(response, m_param.m_general.m_autoTxMedia, strm);
      autoXxMedia = command.m_param.m_general.m_autoTxMedia;
    }

    PStringArray enabledMediaTypes = autoXxMedia.Tokenise(" \t\n", false);
    for (PINDEX i = 0; i < enabledMediaTypes.GetSize(); ++i) {
      OpalMediaTypeDefinition * definition = OpalMediaType::GetDefinition(enabledMediaTypes[0]);
      if (definition != NULL)
        definition->SetAutoStart(autoStart, true);
    }
  }

  SET_MESSAGE_STRING(response, m_param.m_general.m_natRouter, GetTranslationHost());
  if (!IsNullString(command.m_param.m_general.m_natRouter)) {
    if (!SetTranslationHost(command.m_param.m_general.m_natRouter)) {
      response.SetError("Could not set NAT router address.");
      return;
    }
  }

  SET_MESSAGE_STRING(response, m_param.m_general.m_stunServer, GetSTUNServer());
  if (!IsNullString(command.m_param.m_general.m_stunServer)) {
    if (!SetSTUNServer(command.m_param.m_general.m_stunServer)) {
      response.SetError("Could not set STUN server address.");
      return;
    }
    if (GetSTUNClient()->GetNatType() == PSTUNClient::BlockedNat)
      response.SetError("STUN indicates Blocked NAT.");
  }

  response->m_param.m_general.m_tcpPortBase = GetTCPPortBase();
  response->m_param.m_general.m_tcpPortMax = GetTCPPortMax();
  if (command.m_param.m_general.m_tcpPortBase != 0)
    SetTCPPorts(command.m_param.m_general.m_tcpPortBase, command.m_param.m_general.m_tcpPortMax);

  response->m_param.m_general.m_udpPortBase = GetUDPPortBase();
  response->m_param.m_general.m_udpPortMax = GetUDPPortMax();
  if (command.m_param.m_general.m_udpPortBase != 0)
    SetUDPPorts(command.m_param.m_general.m_udpPortBase, command.m_param.m_general.m_udpPortMax);

  response->m_param.m_general.m_rtpPortBase = GetRtpIpPortBase();
  response->m_param.m_general.m_rtpPortMax = GetRtpIpPortMax();
  if (command.m_param.m_general.m_rtpPortBase != 0)
    SetRtpIpPorts(command.m_param.m_general.m_rtpPortBase, command.m_param.m_general.m_rtpPortMax);

  response->m_param.m_general.m_rtpTypeOfService = GetRtpIpTypeofService();
  if (command.m_param.m_general.m_rtpTypeOfService != 0)
    SetRtpIpTypeofService(command.m_param.m_general.m_rtpTypeOfService);

  response->m_param.m_general.m_rtpMaxPayloadSize = GetMaxRtpPayloadSize();
  if (command.m_param.m_general.m_rtpMaxPayloadSize != 0)
    SetMaxRtpPayloadSize(command.m_param.m_general.m_rtpMaxPayloadSize);

  response->m_param.m_general.m_minAudioJitter = GetMinAudioJitterDelay();
  response->m_param.m_general.m_maxAudioJitter = GetMaxAudioJitterDelay();
  if (command.m_param.m_general.m_minAudioJitter != 0 && command.m_param.m_general.m_maxAudioJitter != 0)
    SetAudioJitterDelay(command.m_param.m_general.m_minAudioJitter, command.m_param.m_general.m_maxAudioJitter);

  if (m_apiVersion < 2)
    return;

  OpalSilenceDetector::Params silenceDetectParams = GetSilenceDetectParams();
  response->m_param.m_general.m_silenceDetectMode = (OpalSilenceDetectMode)(silenceDetectParams.m_mode+1);
  if (command.m_param.m_general.m_silenceDetectMode != 0)
    silenceDetectParams.m_mode = (OpalSilenceDetector::Mode)(command.m_param.m_general.m_silenceDetectMode-1);
  response->m_param.m_general.m_silenceThreshold = silenceDetectParams.m_threshold;
  if (command.m_param.m_general.m_silenceThreshold != 0)
    silenceDetectParams.m_threshold = command.m_param.m_general.m_silenceThreshold;
  response->m_param.m_general.m_signalDeadband = silenceDetectParams.m_signalDeadband;
  if (command.m_param.m_general.m_signalDeadband != 0)
    silenceDetectParams.m_signalDeadband = command.m_param.m_general.m_signalDeadband;
  response->m_param.m_general.m_silenceDeadband = silenceDetectParams.m_silenceDeadband;
  if (command.m_param.m_general.m_silenceDeadband != 0)
    silenceDetectParams.m_silenceDeadband = command.m_param.m_general.m_silenceDeadband;
  response->m_param.m_general.m_silenceAdaptPeriod = silenceDetectParams.m_adaptivePeriod;
  if (command.m_param.m_general.m_silenceAdaptPeriod != 0)
    silenceDetectParams.m_adaptivePeriod = command.m_param.m_general.m_silenceAdaptPeriod;
  SetSilenceDetectParams(silenceDetectParams);

#if OPAL_AEC
  OpalEchoCanceler::Params echoCancelParams = GetEchoCancelParams();
  response->m_param.m_general.m_echoCancellation = (OpalEchoCancelMode)(echoCancelParams.m_mode+1);
  if (command.m_param.m_general.m_echoCancellation != 0)
    echoCancelParams.m_mode = (OpalEchoCanceler::Mode)(command.m_param.m_general.m_echoCancellation-1);
  SetEchoCancelParams(echoCancelParams);
#endif

  if (m_apiVersion < 3)
    return;

#if OPAL_PTLIB_AUDIO
#if OPAL_HAS_PCSS
  if (pcssEP != NULL) {
    response->m_param.m_general.m_audioBuffers = pcssEP->GetSoundChannelBufferDepth();
    if (command.m_param.m_general.m_audioBuffers != 0)
      pcssEP->SetSoundChannelBufferDepth(command.m_param.m_general.m_audioBuffers);
  }
#endif
#endif

  if (m_apiVersion < 5)
    return;

  if (localEP != NULL) {
    response->m_param.m_general.m_mediaReadData = localEP->m_mediaReadData;
    if (command.m_param.m_general.m_mediaReadData != NULL)
      localEP->m_mediaReadData = command.m_param.m_general.m_mediaReadData;

    response->m_param.m_general.m_mediaWriteData = localEP->m_mediaWriteData;
    if (command.m_param.m_general.m_mediaWriteData != NULL)
      localEP->m_mediaWriteData = command.m_param.m_general.m_mediaWriteData;

    response->m_param.m_general.m_mediaDataHeader = localEP->m_mediaDataHeader;
    if (command.m_param.m_general.m_mediaDataHeader != 0)
      localEP->m_mediaDataHeader = command.m_param.m_general.m_mediaDataHeader;

    if (m_apiVersion >= 20) {
      response->m_param.m_general.m_mediaTiming = localEP->m_mediaTiming;
      if (command.m_param.m_general.m_mediaTiming != 0)
        localEP->m_mediaTiming = command.m_param.m_general.m_mediaTiming;
    }
  }

  if (m_apiVersion < 8)
    return;

  m_messageMutex.Wait();
  response->m_param.m_general.m_messageAvailable = m_messageAvailableCallback;
  m_messageAvailableCallback = command.m_param.m_general.m_messageAvailable;
  m_messageMutex.Signal();

  if (m_apiVersion < 14)
    return;

  OpalMediaFormatList allCodecs = OpalMediaFormat::GetAllRegisteredMediaFormats();

  PStringStream mediaOptions;
  for (OpalMediaFormatList::iterator itMediaFormat = allCodecs.begin(); itMediaFormat != allCodecs.end(); ++itMediaFormat) {
    if (itMediaFormat->IsTransportable()) {
      mediaOptions << *itMediaFormat << ":Media Type#" << itMediaFormat->GetMediaType() << '\n';
      for (PINDEX i = 0; i < itMediaFormat->GetOptionCount(); ++i) {
        const OpalMediaOption & option = itMediaFormat->GetOption(i);
        mediaOptions << *itMediaFormat << ':' << option.GetName() << (option.IsReadOnly() ? '#' : '=') << option << '\n';
      }
    }
  }
  SET_MESSAGE_STRING(response, m_param.m_general.m_mediaOptions, mediaOptions);

  PStringArray options = PString(command.m_param.m_general.m_mediaOptions).Lines();
  for (PINDEX i = 0; i < options.GetSize(); ++i) {
    PString optionSpec = options[i];
    PINDEX colon = optionSpec.Find(':');
    PINDEX equal = optionSpec.Find('=', colon);
    PString mediaName = optionSpec.Left(colon);
    PString optionName = optionSpec(colon+1, equal-1);
    PString optionValue = optionSpec.Mid(equal+1);

    if (mediaName.IsEmpty() || optionName.IsEmpty()) {
      PTRACE(2, "OpalC API\tInvalid syntax for media format option: \"" << optionSpec << '"');
    }
    else {
      OpalMediaType mediaType = mediaName.ToLower();
      if (OpalMediaTypeFactory::CreateInstance(mediaType) != NULL) {
        // Known media type name, change all codecs of that type
        for (OpalMediaFormatList::iterator it = allCodecs.begin(); it != allCodecs.end(); ++it) {
          if (it->GetMediaType() == mediaType) {
            if (it->SetOptionValue(optionName, optionValue)) {
              OpalMediaFormat::SetRegisteredMediaFormat(*it);
              PTRACE(4, "OpalC API\tSet " << mediaType << " media format \"" << *it
                     << "\" option \"" << optionName << "\" to \"" << optionValue << '"');
            }
            else {
              PTRACE(2, "OpalC API\tCould not set " << mediaType
                     << " media format option \"" << optionName << "\" to \"" << optionValue << '"');
            }
          }
        }
      }
      else {
        OpalMediaFormat mediaFormat = mediaName;
        if (mediaFormat.IsValid()) {
          if (mediaFormat.SetOptionValue(optionName, optionValue)) {
            OpalMediaFormat::SetRegisteredMediaFormat(mediaFormat);
            PTRACE(2, "OpalC API\tSet media format \"" << mediaFormat
                   << "\" option \"" << optionName << "\" to \"" << optionValue << '"');
          }
          else {
            PTRACE(2, "OpalC API\tCould not set media format \"" << mediaFormat
                   << "\" option \"" << optionName << "\" to \"" << optionValue << '"');
          }
        }
        else {
          PTRACE(2, "OpalC API\tTried to set option for unknown media format: \"" << mediaName << '"');
        }
      }
    }
  }

  if (m_apiVersion < 17)
    return;

#if OPAL_PTLIB_AUDIO
#if OPAL_HAS_PCSS
  if (pcssEP != NULL) {
    response->m_param.m_general.m_audioBufferTime = pcssEP->GetSoundChannelBufferTime();
    if (command.m_param.m_general.m_audioBufferTime != 0)
      pcssEP->SetSoundChannelBufferTime(command.m_param.m_general.m_audioBufferTime);
  }
#endif
#endif

  if (m_apiVersion < 19)
    return;

  response->m_param.m_general.m_audioBufferTime = m_manualAlerting ? 2 : 1;
  if (command.m_param.m_general.m_manualAlerting != 0)
    m_manualAlerting = command.m_param.m_general.m_manualAlerting != 1;
}


void FillOpalProductInfo(const OpalMessage & command, OpalMessageBuffer & response, OpalProductInfo & info)
{
  SET_MESSAGE_STRING(response, m_param.m_protocol.m_product.m_vendor,  info.vendor);
  SET_MESSAGE_STRING(response, m_param.m_protocol.m_product.m_name,    BuildProductName(info));
  SET_MESSAGE_STRING(response, m_param.m_protocol.m_product.m_version, info.version);

  response->m_param.m_protocol.m_product.m_t35CountryCode   = info.t35CountryCode;
  response->m_param.m_protocol.m_product.m_t35Extension     = info.t35Extension;
  response->m_param.m_protocol.m_product.m_manufacturerCode = info.manufacturerCode;

  if (command.m_param.m_protocol.m_product.m_vendor != NULL)
    info.vendor = command.m_param.m_protocol.m_product.m_vendor;

  if (command.m_param.m_protocol.m_product.m_name != NULL) {
    PString str = command.m_param.m_protocol.m_product.m_name;
    PINDEX paren = str.Find('(');
    if (paren == P_MAX_INDEX)
      info.name = str;
    else {
      info.name = str.Left(paren).Trim();
      info.comments = str.Mid(paren);
    }
  }

  if (command.m_param.m_protocol.m_product.m_version != NULL)
    info.version = command.m_param.m_protocol.m_product.m_version;

  if (command.m_param.m_protocol.m_product.m_t35CountryCode != 0 && command.m_param.m_protocol.m_product.m_manufacturerCode != 0) {
    info.t35CountryCode   = (BYTE)command.m_param.m_protocol.m_product.m_t35CountryCode;
    info.t35Extension     = (BYTE)command.m_param.m_protocol.m_product.m_t35Extension;
    info.manufacturerCode = (WORD)command.m_param.m_protocol.m_product.m_manufacturerCode;
  }
}


static void StartStopListeners(OpalEndPoint * ep, const PString & interfaces, OpalMessageBuffer & response)
{
  if (ep == NULL)
    return;

  ep->RemoveListener(NULL);
  if (interfaces.IsEmpty())
    return;

  PStringArray interfaceArray;
  if (interfaces != "*")
    interfaceArray = interfaces.Lines();
  if (!ep->StartListeners(interfaceArray))
    response.SetError("Could not start listener(s).");
}


void OpalManager_C::HandleSetProtocol(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_protocol.m_prefix)) {
    SET_MESSAGE_STRING(response, m_param.m_protocol.m_userName, GetDefaultUserName());
    if (!IsNullString(command.m_param.m_protocol.m_userName))
      SetDefaultUserName(command.m_param.m_protocol.m_userName);

    SET_MESSAGE_STRING(response, m_param.m_protocol.m_displayName, GetDefaultUserName());
    if (!IsNullString(command.m_param.m_protocol.m_displayName))
      SetDefaultDisplayName(command.m_param.m_protocol.m_displayName);

    OpalProductInfo product = GetProductInfo();
    FillOpalProductInfo(command, response, product);
    SetProductInfo(product);

    if (command.m_param.m_protocol.m_interfaceAddresses != NULL) {
#if OPAL_H323
      StartStopListeners(FindEndPoint(OPAL_PREFIX_H323), command.m_param.m_protocol.m_interfaceAddresses, response);
#endif
#if OPAL_SIP
      StartStopListeners(FindEndPoint(OPAL_PREFIX_SIP),  command.m_param.m_protocol.m_interfaceAddresses, response);
#endif
#if OPAL_IAX2
      StartStopListeners(FindEndPoint(OPAL_PREFIX_IAX2),  command.m_param.m_protocol.m_interfaceAddresses, response);
#endif
    }

    return;
  }

  OpalEndPoint * ep = FindEndPoint(command.m_param.m_protocol.m_prefix);
  if (ep == NULL) {
    response.SetError("No such protocol prefix");
    return;
  }

  SET_MESSAGE_STRING(response, m_param.m_protocol.m_userName, ep->GetDefaultLocalPartyName());
  if (!IsNullString(command.m_param.m_protocol.m_userName))
    ep->SetDefaultLocalPartyName(command.m_param.m_protocol.m_userName);

  SET_MESSAGE_STRING(response, m_param.m_protocol.m_displayName, ep->GetDefaultDisplayName());
  if (!IsNullString(command.m_param.m_protocol.m_displayName))
    ep->SetDefaultDisplayName(command.m_param.m_protocol.m_displayName);

  OpalProductInfo product = ep->GetProductInfo();
  FillOpalProductInfo(command, response, product);
  ep->SetProductInfo(product);

  if (command.m_param.m_protocol.m_interfaceAddresses != NULL)
    StartStopListeners(ep, command.m_param.m_protocol.m_interfaceAddresses, response);
}


void OpalManager_C::HandleRegistration(const OpalMessage & command, OpalMessageBuffer & response)
{
  OpalEndPoint * ep = FindEndPoint(command.m_param.m_registrationInfo.m_protocol);
  if (ep == NULL) {
    response.SetError("No such protocol prefix");
    return;
  }

#if OPAL_H323
  H323EndPoint * h323 = dynamic_cast<H323EndPoint *>(ep);
  if (h323 != NULL) {
    if (command.m_param.m_registrationInfo.m_timeToLive == 0) {
      if (!h323->RemoveGatekeeper())
        response.SetError("Failed to initiate H.323 gatekeeper unregistration.");
    }
    else {
      if (!IsNullString(command.m_param.m_registrationInfo.m_identifier))
        h323->AddAliasName(command.m_param.m_registrationInfo.m_identifier);
      h323->SetGatekeeperPassword(command.m_param.m_registrationInfo.m_password, command.m_param.m_registrationInfo.m_authUserName);
      if (!h323->UseGatekeeper(command.m_param.m_registrationInfo.m_hostName, command.m_param.m_registrationInfo.m_adminEntity))
        response.SetError("Failed to initiate H.323 gatekeeper registration.");
    }
    return;
  }
#endif

#if OPAL_SIP
  SIPEndPoint * sip = dynamic_cast<SIPEndPoint *>(ep);
  if (sip != NULL) {
    if (IsNullString(command.m_param.m_registrationInfo.m_hostName) &&
          (IsNullString(command.m_param.m_registrationInfo.m_identifier) ||
                 strchr(command.m_param.m_registrationInfo.m_identifier, '@') == NULL)) {
      response.SetError("No domain specified for SIP registration.");
      return;
    }

    if (command.m_param.m_registrationInfo.m_timeToLive == 0) {
      if (!sip->Unregister(command.m_param.m_registrationInfo.m_identifier))
        response.SetError("Failed to initiate SIP unregistration.");
    }
    else {
      PString aor;

      if (m_apiVersion < 13 || command.m_param.m_registrationInfo.m_eventPackage == NULL) {
        SIPRegister::Params regParams;
        regParams.m_addressOfRecord = command.m_param.m_registrationInfo.m_identifier;
        regParams.m_registrarAddress = command.m_param.m_registrationInfo.m_hostName;
        regParams.m_authID = command.m_param.m_registrationInfo.m_authUserName;
        regParams.m_password = command.m_param.m_registrationInfo.m_password;
        regParams.m_realm = command.m_param.m_registrationInfo.m_adminEntity;
        regParams.m_expire = command.m_param.m_registrationInfo.m_timeToLive;
        if (m_apiVersion >= 7 && command.m_param.m_registrationInfo.m_restoreTime > 0)
          regParams.m_restoreTime = command.m_param.m_registrationInfo.m_restoreTime;

        if (sip->Register(regParams, aor))
          SET_MESSAGE_STRING(response, m_param.m_registrationInfo.m_identifier, aor);
        else
          response.SetError("Failed to initiate SIP registration.");
      }

      if (m_apiVersion >= 10) {
        SIPSubscribe::Params subParams;
        if (m_apiVersion < 13)
          subParams.m_eventPackage = SIPSubscribe::MessageSummary;

        else {
          if (command.m_param.m_registrationInfo.m_eventPackage == NULL)
            return;
          subParams.m_eventPackage = command.m_param.m_registrationInfo.m_eventPackage;
        }

        subParams.m_addressOfRecord = command.m_param.m_registrationInfo.m_identifier;
        subParams.m_agentAddress = command.m_param.m_registrationInfo.m_hostName;
        subParams.m_authID = command.m_param.m_registrationInfo.m_authUserName;
        subParams.m_password = command.m_param.m_registrationInfo.m_password;
        subParams.m_realm = command.m_param.m_registrationInfo.m_adminEntity;
#if P_64BIT
        subParams.m_expire = m_apiVersion = command.m_param.m_registrationInfo.m_timeToLive;
#else
        subParams.m_expire = m_apiVersion >= 13 ? command.m_param.m_registrationInfo.m_timeToLive
                                               : (unsigned)command.m_param.m_registrationInfo.m_eventPackage; // Backward compatibility
#endif
        subParams.m_restoreTime = command.m_param.m_registrationInfo.m_restoreTime;
        bool ok = sip->Subscribe(subParams, aor);
        if (m_apiVersion >= 13) {
          if (ok)
            SET_MESSAGE_STRING(response, m_param.m_registrationInfo.m_identifier, aor);
          else
            response.SetError("Failed to initiate SIP subscription.");
        }
      }
    }
    return;
  }
#endif

  response.SetError("Protocol prefix does not support registration.");
}


void OpalManager_C::HandleSetUpCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_callSetUp.m_partyB)) {
    response.SetError("No destination address provided.");
    return;
  }

  PString partyA = command.m_param.m_callSetUp.m_partyA;
  if (partyA.IsEmpty()) {
#if OPAL_PTLIB_AUDIO
#if OPAL_HAS_PCSS
    if (pcssEP != NULL)
      partyA = "pc:*";
    else
#endif
#endif
    if (localEP != NULL)
      partyA = "local:*";
    else
      partyA = "pots:*";
  }

  OpalConnection::StringOptions options;
  if (!IsNullString(command.m_param.m_callSetUp.m_alertingType))
    options.SetAt(OPAL_OPT_ALERTING_TYPE, command.m_param.m_callSetUp.m_alertingType);

  PString token;
  if (SetUpCall(partyA, command.m_param.m_callSetUp.m_partyB, token, NULL, 0, &options)) {
    SET_MESSAGE_STRING(response, m_param.m_callSetUp.m_partyA, partyA);
    SET_MESSAGE_STRING(response, m_param.m_callSetUp.m_partyB, command.m_param.m_callSetUp.m_partyB);
    SET_MESSAGE_STRING(response, m_param.m_callSetUp.m_callToken, token);
    PSafePtr<OpalCall> call = FindCallWithLock(token);
    if (call != NULL) {
      PSafePtr<OpalConnection> other = call->GetConnection(1);
      if (other != NULL)
        SET_MESSAGE_STRING(response, m_param.m_callSetUp.m_protocolCallId, other->GetIdentifier());
    }
  }
  else
    response.SetError("Call set up failed.");
}


void OpalManager_C::HandleAlerting(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_callToken)) {
    response.SetError("No call token provided.");
    return;
  }

  if (
#if OPAL_PTLIB_AUDIO
#if OPAL_HAS_PCSS
      pcssEP == NULL &&
#endif
#endif
      localEP == NULL) {
    response.SetError("Can only control alerting from PC.");
    return;
  }

#if OPAL_PTLIB_AUDIO
#if OPAL_HAS_PCSS
  //if (pcssEP != NULL && pcssEP->AlertingIncomingCall(command.m_param.m_callToken))
  //  return;
#endif
#endif

  if (localEP != NULL && localEP->AlertingIncomingCall(command.m_param.m_callToken))
    return;

  response.SetError("No call found by the token provided.");
}


void OpalManager_C::HandleAnswerCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_callToken)) {
    response.SetError("No call token provided.");
    return;
  }

  if (
#if OPAL_PTLIB_AUDIO
#if OPAL_HAS_PCSS
      pcssEP == NULL &&
#endif
#endif
      localEP == NULL) {
    response.SetError("Can only answer calls to PC.");
    return;
  }

#if OPAL_PTLIB_AUDIO
#if OPAL_HAS_PCSS
  //if (pcssEP != NULL && pcssEP->AlertingIncomingCall(command.m_param.m_callToken))
  //  return;
#endif
#endif

  if (localEP != NULL && localEP->AcceptIncomingCall(command.m_param.m_callToken))
    return;

  response.SetError("No call found by the token provided.");
}


bool OpalManager_C::FindCall(const char * token, OpalMessageBuffer & response, PSafePtr<OpalCall> & call)
{
  if (IsNullString(token)) {
    response.SetError("No call token provided.");
    return false;
  }

  call = FindCallWithLock(token);
  if (call == NULL) {
    response.SetError("No call found by the token provided.");
    return false;
  }

  return true;
}


void OpalManager_C::HandleUserInput(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_userInput.m_userInput)) {
    response.SetError("No user input provided.");
    return;
  }

  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_userInput.m_callToken, response, call))
    return;

  PSafePtr<OpalConnection> connection = call->GetConnection(0, PSafeReadOnly);
  while (connection->IsNetworkConnection()) {
    ++connection;
    if (connection == NULL) {
      response.SetError("No suitable connection for user input.");
      return;
    }
  }

  if (command.m_param.m_userInput.m_duration == 0)
    connection->OnUserInputString(command.m_param.m_userInput.m_userInput);
  else
    connection->OnUserInputTone(command.m_param.m_userInput.m_userInput[0], command.m_param.m_userInput.m_duration);
}


void OpalManager_C::HandleClearCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  const char * callToken;
  OpalConnection::CallEndReason reason;

  if (m_apiVersion < 9) {
    callToken = command.m_param.m_callToken;
    reason = OpalConnection::EndedByLocalUser;
  }
  else {
    callToken = command.m_param.m_clearCall.m_callToken;
    reason.code = (OpalConnection::CallEndReasonCodes)command.m_param.m_clearCall.m_reason;
  }

  if (IsNullString(callToken)) {
    response.SetError("No call token provided.");
    return;
  }

  if (!ClearCall(callToken, reason))
    response.SetError("No call found by the token provided.");
}


void OpalManager_C::HandleHoldCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_userInput.m_callToken, response, call))
    return;

  if (call->IsOnHold()) {
    response.SetError("Call is already on hold.");
    return;
  }

  call->Hold();
}


void OpalManager_C::HandleRetrieveCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_userInput.m_callToken, response, call))
    return;

  if (!call->IsOnHold()) {
    response.SetError("Call is not on hold.");
    return;
  }

  call->Retrieve();
}


void OpalManager_C::HandleTransferCall(const OpalMessage & command, OpalMessageBuffer & response)
{
  if (IsNullString(command.m_param.m_callSetUp.m_partyB)) {
    response.SetError("No destination address provided.");
    return;
  }

  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_callSetUp.m_callToken, response, call))
    return;

  PString search = command.m_param.m_callSetUp.m_partyA;
  if (search.IsEmpty()) {
    search = command.m_param.m_callSetUp.m_partyB;
    search.Delete(search.Find(':'), P_MAX_INDEX);
  }

  PSafePtr<OpalConnection> connection = call->GetConnection(0, PSafeReadOnly);
  while (connection->GetLocalPartyURL().NumCompare(search) != EqualTo) {
    if (++connection == NULL) {
      response.SetError("Call does not have suitable connection to transfer.");
      return;
    }
  }

  connection->TransferConnection(command.m_param.m_callSetUp.m_partyB);
}


void OpalManager_C::HandleMediaStream(const OpalMessage & command, OpalMessageBuffer & response)
{
  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_userInput.m_callToken, response, call))
    return;

  PSafePtr<OpalConnection> connection = call->GetConnection(0, PSafeReadOnly);
  while (connection->IsNetworkConnection()) {
    ++connection;
    if (connection == NULL) {
      response.SetError("No suitable connection for media stream control.");
      return;
    }
  }

  OpalMediaType mediaType;
  bool source = false;
  if (!IsNullString(command.m_param.m_mediaStream.m_type)) {
    PString typeStr = command.m_param.m_mediaStream.m_type;
    mediaType = typeStr.Left(typeStr.Find(' '));
    source = typeStr.Find("out") != P_MAX_INDEX;
  }

  OpalMediaStreamPtr stream;
  if (!IsNullString(command.m_param.m_mediaStream.m_identifier))
    stream = connection->GetMediaStream(PString(command.m_param.m_mediaStream.m_identifier), source);
  else if (!IsNullString(command.m_param.m_mediaStream.m_type))
    stream = connection->GetMediaStream(mediaType, source);
  else {
    response.SetError("No identifer or type provided to locate media stream.");
    return;
  }

  if (stream == NULL && command.m_param.m_mediaStream.m_state != OpalMediaStateOpen) {
    response.SetError("Could not locate media stream.");
    return;
  }

  switch (command.m_param.m_mediaStream.m_state) {
    case OpalMediaStateNoChange :
      break;

    case OpalMediaStateOpen :
      if (mediaType.empty())
        response.SetError("Must provide type and direction to open media stream.");
      else {
        OpalMediaFormat mediaFormat(command.m_param.m_mediaStream.m_format);
        unsigned sessionID = 0;
        if (stream != NULL)
          sessionID = stream->GetSessionID();
        if (source)
          call->OpenSourceMediaStreams(*connection, mediaType, sessionID, mediaFormat);
        else
          call->OpenSourceMediaStreams(*call->GetOtherPartyConnection(*connection), mediaType, sessionID, mediaFormat);
      }
      break;

    case OpalMediaStateClose :
      connection->CloseMediaStream(*stream);
      break;

    case OpalMediaStatePause :
      stream->SetPaused(true);
      break;

    case OpalMediaStateResume :
      stream->SetPaused(false);
      break;
  }
}


void OpalManager_C::HandleStartRecording(const OpalMessage & command, OpalMessageBuffer & response)
{
#if OPAL_HAS_MIXER
  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_userInput.m_callToken, response, call))
    return;

  if (IsNullString(command.m_param.m_recording.m_file)) {
    if (!call->IsRecording())
      response.SetError("No recording active for call.");
    return;
  }

  OpalRecordManager::Options options;
  options.m_stereo = command.m_param.m_recording.m_channels == 2;
  if (m_apiVersion >= 21) {
    options.m_audioFormat = command.m_param.m_recording.m_audioFormat;
#if OPAL_VIDEO
    options.m_videoFormat = command.m_param.m_recording.m_videoFormat;
    options.m_videoWidth  = command.m_param.m_recording.m_videoWidth;
    options.m_videoHeight = command.m_param.m_recording.m_videoHeight;
    options.m_videoRate   = command.m_param.m_recording.m_videoRate;
    options.m_videoMixing = (OpalRecordManager::VideoMode)command.m_param.m_recording.m_videoMixing;
#endif
  }

  if (!call->StartRecording(command.m_param.m_recording.m_file, options))
#endif
    response.SetError("Could not start recording for call.");
}


void OpalManager_C::HandleStopRecording(const OpalMessage & command, OpalMessageBuffer & response)
{
#if OPAL_HAS_MIXER
  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_userInput.m_callToken, response, call))
    return;

  call->StopRecording();
#endif
}


void OpalManager_C::HandleSetUserData(const OpalMessage & command, OpalMessageBuffer & response)
{
  PSafePtr<OpalCall> call;
  if (!FindCall(command.m_param.m_userInput.m_callToken, response, call))
    return;

  PSafePtr<OpalLocalConnection> connection = call->GetConnectionAs<OpalLocalConnection>();
  if (connection == NULL) {
    response.SetError("No suitable connection for media stream control.");
    return;
  }

  connection->SetUserData(command.m_param.m_setUserData.m_userData);
}


void OpalManager_C::OnEstablishedCall(OpalCall & call)
{
  OpalMessageBuffer message(OpalIndEstablished);
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_partyA, call.GetPartyA());
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_partyB, call.GetPartyB());
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_callToken, call.GetToken());
  PTRACE(4, "OpalC API\tOnEstablishedCall:"
            " token=\"" << message->m_param.m_callSetUp.m_callToken << "\""
            " A=\""     << message->m_param.m_callSetUp.m_partyA << "\""
            " B=\""     << message->m_param.m_callSetUp.m_partyB << '"');
  PostMessage(message);
}


void OpalManager_C::OnIndMediaStream(const OpalMediaStream & stream, OpalMediaStates state)
{
  const OpalConnection & connection = stream.GetConnection();
  if (!connection.IsNetworkConnection())
    return;

  OpalMessageBuffer message(OpalIndMediaStream);
  SET_MESSAGE_STRING(message, m_param.m_mediaStream.m_callToken, connection.GetCall().GetToken());
  SET_MESSAGE_STRING(message, m_param.m_mediaStream.m_identifier, stream.GetID());
  PStringStream type;
  type << stream.GetMediaFormat().GetMediaType() << (stream.IsSource() ? " in" : " out");
  SET_MESSAGE_STRING(message, m_param.m_mediaStream.m_type, type);
  SET_MESSAGE_STRING(message, m_param.m_mediaStream.m_format, stream.GetMediaFormat().GetName());
  message->m_param.m_mediaStream.m_state = state;
  PTRACE(4, "OpalC API\tOnIndMediaStream:"
            " token=\"" << message->m_param.m_userInput.m_callToken << "\""
            " id=\"" << message->m_param.m_mediaStream.m_identifier << '"');
  PostMessage(message);
}


PBoolean OpalManager_C::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  if (!OpalManager::OnOpenMediaStream(connection, stream))
    return false;

  OnIndMediaStream(stream, OpalMediaStateOpen);
  return true;
}


void OpalManager_C::OnClosedMediaStream(const OpalMediaStream & stream)
{
  OnIndMediaStream(stream, OpalMediaStateClose);
  OpalManager::OnClosedMediaStream(stream);
}


void OpalManager_C::OnUserInputString(OpalConnection & connection, const PString & value)
{
  OpalMessageBuffer message(OpalIndUserInput);
  SET_MESSAGE_STRING(message, m_param.m_userInput.m_callToken, connection.GetCall().GetToken());
  SET_MESSAGE_STRING(message, m_param.m_userInput.m_userInput, value);
  message->m_param.m_userInput.m_duration = 0;
  PTRACE(4, "OpalC API\tOnUserInputString:"
            " token=\"" << message->m_param.m_userInput.m_callToken << "\""
            " input=\"" << message->m_param.m_userInput.m_userInput << '"');
  PostMessage(message);

  OpalManager::OnUserInputString(connection, value);
}


void OpalManager_C::OnUserInputTone(OpalConnection & connection, char tone, int duration)
{
  char input[2];
  input[0] = tone;
  input[1] = '\0';

  OpalMessageBuffer message(OpalIndUserInput);
  SET_MESSAGE_STRING(message, m_param.m_userInput.m_callToken, connection.GetCall().GetToken());
  SET_MESSAGE_STRING(message, m_param.m_userInput.m_userInput, input);
  message->m_param.m_userInput.m_duration = duration;
  PTRACE(4, "OpalC API\tOnUserInputTone:"
            " token=\"" << message->m_param.m_userInput.m_callToken << "\""
            " input=\"" << message->m_param.m_userInput.m_userInput << '"');
  PostMessage(message);

  OpalManager::OnUserInputTone(connection, tone, duration);
}


void OpalManager_C::OnMWIReceived(const PString & party, MessageWaitingType type, const PString & extraInfo)
{
  OpalMessageBuffer message(OpalIndMessageWaiting);
  SET_MESSAGE_STRING(message, m_param.m_messageWaiting.m_party, party);
  static const char * const TypeNames[] = { "Voice", "Fax", "Pager", "Multimedia", "Text", "None" };
  if ((size_t)type < sizeof(TypeNames)/sizeof(TypeNames[0]))
    SET_MESSAGE_STRING(message, m_param.m_messageWaiting.m_type, TypeNames[type]);
  SET_MESSAGE_STRING(message, m_param.m_messageWaiting.m_extraInfo, extraInfo);
  PTRACE(4, "OpalC API\tOnMWIReceived: party=\"" << message->m_param.m_messageWaiting.m_party
                                   << "\" type=" << message->m_param.m_messageWaiting.m_type
                                   << "\" info=" << message->m_param.m_messageWaiting.m_extraInfo);
  PostMessage(message);

  OpalManager::OnMWIReceived(party, type, extraInfo);
}


void OpalManager_C::OnProceeding(OpalConnection & connection)
{
  OpalCall & call = connection.GetCall();

  OpalMessageBuffer message(OpalIndProceeding);
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_partyA, call.GetPartyA());
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_partyB, call.GetPartyB());
  SET_MESSAGE_STRING(message, m_param.m_callSetUp.m_callToken, call.GetToken());
  PTRACE(4, "OpalC API\tOnProceeding:"
            " token=\"" << message->m_param.m_callSetUp.m_callToken << "\""
            " A=\""     << message->m_param.m_callSetUp.m_partyA << "\""
            " B=\""     << message->m_param.m_callSetUp.m_partyB << '"');
  PostMessage(message);

  OpalManager::OnProceeding(connection);
}


void OpalManager_C::OnClearedCall(OpalCall & call)
{
  OpalMessageBuffer message(OpalIndCallCleared);
  SET_MESSAGE_STRING(message, m_param.m_callCleared.m_callToken, call.GetToken());


  PStringStream str;
  str << (unsigned)call.GetCallEndReason() << ": " << call.GetCallEndReasonText();

  SET_MESSAGE_STRING(message, m_param.m_callCleared.m_reason, str);
  PTRACE(4, "OpalC API\tOnClearedCall:"
            " token=\""  << message->m_param.m_callCleared.m_callToken << "\""
            " reason=\"" << message->m_param.m_callCleared.m_reason << '"');
  PostMessage(message);

  OpalManager::OnClearedCall(call);
}


///////////////////////////////////////////////////////////////////////////////

extern "C" {

  OpalHandle OPAL_EXPORT OpalInitialise(unsigned * version, const char * options)
  {
    PCaselessString optionsString = IsNullString(options) ?
#if OPAL_HAS_PCSS
            "pcss "
#endif
            "h323 sip iax2 pots pstn fax t38 ivr" : options;

    unsigned callerVersion = 1;
    if (version != NULL) {
      callerVersion = *version;
      if (callerVersion > OPAL_C_API_VERSION)
        *version = OPAL_C_API_VERSION;
    }

    OpalHandle opal = new OpalHandleStruct(callerVersion, optionsString);
    if (opal->manager.Initialise(optionsString))
      return opal;

    delete opal;
    return NULL;
  }


  void OPAL_EXPORT OpalShutDown(OpalHandle handle)
  {
    delete handle;
  }


  OpalMessage * OPAL_EXPORT OpalGetMessage(OpalHandle handle, unsigned timeout)
  {
    return handle == NULL ? NULL : handle->manager.GetMessage(timeout);
  }


  OpalMessage * OPAL_EXPORT OpalSendMessage(OpalHandle handle, const OpalMessage * message)
  {
    return handle == NULL ? NULL : handle->manager.SendMessage(message);
  }


  void OPAL_EXPORT OpalFreeMessage(OpalMessage * message)
  {
    if (message != NULL)
      free(message);
  }

}; // extern "C"


///////////////////////////////////////////////////////////////////////////////

OpalContext::OpalContext()
  : m_handle(NULL)
{
}


OpalContext::~OpalContext()
{
  ShutDown();
}


unsigned OpalContext::Initialise(const char * options, unsigned version)
{
  ShutDown();

  m_handle = OpalInitialise(&version, options);
  return m_handle != NULL ? version : 0;
}


void OpalContext::ShutDown()
{
  if (m_handle != NULL) {
    OpalShutDown(m_handle);
    m_handle = NULL;
  }
}


bool OpalContext::GetMessage(OpalMessagePtr & message, unsigned timeout)
{
  if (m_handle == NULL) {
    message.SetType(OpalIndCommandError);
    message.m_message->m_param.m_commandError = "Uninitialised OPAL context.";
    return false;
  }

  message.m_message = OpalGetMessage(m_handle, timeout);
  if (message.m_message != NULL)
    return true;

  message.SetType(OpalIndCommandError);
  message.m_message->m_param.m_commandError = "Timeout getting message.";
  return false;
}


bool OpalContext::SendMessage(const OpalMessagePtr & message, OpalMessagePtr & response)
{
  if (m_handle == NULL) {
    response.SetType(OpalIndCommandError);
    response.m_message->m_param.m_commandError = "Uninitialised OPAL context.";
    return false;
  }

  response.m_message = OpalSendMessage(m_handle, message.m_message);
  if (response.m_message != NULL)
    return response.GetType() != OpalIndCommandError;

  response.SetType(OpalIndCommandError);
  response.m_message->m_param.m_commandError = "Invalid message.";
  return false;
}


bool OpalContext::SetUpCall(OpalMessagePtr & response,
                            const char * partyB,
                            const char * partyA,
                            const char * alertingType)
{
  OpalMessagePtr message(OpalCmdSetUpCall);
  OpalParamSetUpCall * param = message.GetCallSetUp();
  param->m_partyA = partyA;
  param->m_partyB = partyB;
  param->m_alertingType = alertingType;
  return SendMessage(message, response);
}


bool OpalContext::AnswerCall(const char * callToken)
{
  OpalMessagePtr message(OpalCmdAnswerCall), response;
  message.SetCallToken(callToken);
  return SendMessage(message, response);
}


bool OpalContext::ClearCall(const char * callToken, OpalCallEndReason reason)
{
  OpalMessagePtr message(OpalCmdClearCall), response;
  OpalParamCallCleared * param = message.GetClearCall();
  param->m_callToken = callToken;
  param->m_reason = reason;
  return SendMessage(message, response);
}


bool OpalContext::SendUserInput(const char * callToken, const char * userInput, unsigned duration)
{
  OpalMessagePtr message(OpalCmdClearCall), response;
  OpalParamUserInput * param = message.GetUserInput();
  param->m_callToken = callToken;
  param->m_userInput = userInput;
  param->m_duration = duration;
  return SendMessage(message, response);
}


OpalMessagePtr::OpalMessagePtr(OpalMessageType type)
  : m_message(NULL)
{
  SetType(type);
}


OpalMessagePtr::~OpalMessagePtr()
{
  OpalFreeMessage(m_message);
}


OpalMessageType OpalMessagePtr::GetType() const
{
  return m_message->m_type;
}


void OpalMessagePtr::SetType(OpalMessageType type)
{
  OpalFreeMessage(m_message);

  m_message = (OpalMessage *)malloc(sizeof(OpalMessage)); // Use malloc to be compatible with OpalFreeMessage
  memset(m_message, 0, sizeof(OpalMessage));
  m_message->m_type = type;
}


const char * OpalMessagePtr::GetCommandError() const
{
  return m_message->m_type == OpalIndCommandError ? m_message->m_param.m_commandError : NULL;
}


const char * OpalMessagePtr::GetCallToken() const
{
  switch (m_message->m_type) {
    case OpalCmdAnswerCall :
    case OpalCmdHoldCall :
    case OpalCmdRetrieveCall :
    case OpalCmdStopRecording :
    case OpalCmdAlerting :
      return m_message->m_param.m_callToken;

    case OpalCmdSetUpCall :
    case OpalIndProceeding :
    case OpalIndAlerting :
    case OpalIndEstablished :
      return m_message->m_param.m_callSetUp.m_callToken;

    case OpalIndIncomingCall :
      return m_message->m_param.m_incomingCall.m_callToken;

    case OpalIndMediaStream :
    case OpalCmdMediaStream :
      return m_message->m_param.m_mediaStream.m_callToken;

    case OpalCmdSetUserData :
      return m_message->m_param.m_setUserData.m_callToken;

    case OpalIndUserInput :
      return m_message->m_param.m_userInput.m_callToken;

    case OpalCmdStartRecording :
      return m_message->m_param.m_recording.m_callToken;

    case OpalIndCallCleared :
      return m_message->m_param.m_callCleared.m_callToken;

    case OpalCmdClearCall :
      return m_message->m_param.m_clearCall.m_callToken;

    default :
      return NULL;
  }
}


void OpalMessagePtr::SetCallToken(const char * callToken)
{
  switch (m_message->m_type) {
    case OpalCmdAnswerCall :
    case OpalCmdHoldCall :
    case OpalCmdRetrieveCall :
    case OpalCmdStopRecording :
    case OpalCmdAlerting :
      m_message->m_param.m_callToken = callToken;
      break;

    case OpalCmdSetUpCall :
    case OpalIndProceeding :
    case OpalIndAlerting :
    case OpalIndEstablished :
      m_message->m_param.m_callSetUp.m_callToken = callToken;
      break;

    case OpalIndIncomingCall :
      m_message->m_param.m_incomingCall.m_callToken = callToken;
      break;

    case OpalIndMediaStream :
    case OpalCmdMediaStream :
      m_message->m_param.m_mediaStream.m_callToken = callToken;
      break;

    case OpalCmdSetUserData :
      m_message->m_param.m_setUserData.m_callToken = callToken;
      break;

    case OpalIndUserInput :
      m_message->m_param.m_userInput.m_callToken = callToken;
      break;

    case OpalCmdStartRecording :
      m_message->m_param.m_recording.m_callToken = callToken;
      break;

    case OpalIndCallCleared :
      m_message->m_param.m_callCleared.m_callToken = callToken;
      break;

    case OpalCmdClearCall :
      m_message->m_param.m_clearCall.m_callToken = callToken;
      break;

    default :
      break;
  }
}


OpalParamGeneral * OpalMessagePtr::GetGeneralParams() const
{
  return m_message->m_type == OpalCmdSetGeneralParameters ? &m_message->m_param.m_general : NULL;
}


OpalParamProtocol * OpalMessagePtr::GetProtocolParams() const
{
  return m_message->m_type == OpalCmdSetProtocolParameters ? &m_message->m_param.m_protocol : NULL;
}


OpalParamRegistration * OpalMessagePtr::GetRegistrationInfo() const
{
  return m_message->m_type == OpalCmdRegistration ? &m_message->m_param.m_registrationInfo : NULL;
}


OpalStatusRegistration * OpalMessagePtr::GetRegistrationStatus() const
{
  return m_message->m_type == OpalIndRegistration ? &m_message->m_param.m_registrationStatus : NULL;
}


OpalParamSetUpCall * OpalMessagePtr::GetCallSetUp() const
{
  switch (m_message->m_type) {
    case OpalCmdSetUpCall :
    case OpalIndProceeding :
    case OpalIndAlerting :
    case OpalIndEstablished :
      return &m_message->m_param.m_callSetUp;

    default :
      return NULL;
  }
}


OpalStatusIncomingCall * OpalMessagePtr::GetIncomingCall() const
{
  return m_message->m_type == OpalIndIncomingCall ? &m_message->m_param.m_incomingCall : NULL;
}


OpalStatusUserInput * OpalMessagePtr::GetUserInput() const
{
  switch (m_message->m_type) {
    case OpalIndUserInput :
    case OpalCmdUserInput :
      return &m_message->m_param.m_userInput;

    default :
      return NULL;
  }
}


OpalStatusMessageWaiting * OpalMessagePtr::GetMessageWaiting() const
{
  return m_message->m_type == OpalIndMessageWaiting ? &m_message->m_param.m_messageWaiting : NULL;
}


OpalStatusLineAppearance * OpalMessagePtr::GetLineAppearance() const
{
  return m_message->m_type == OpalIndLineAppearance ? &m_message->m_param.m_lineAppearance : NULL;
}


OpalStatusCallCleared * OpalMessagePtr::GetCallCleared() const
{
  return m_message->m_type == OpalIndCallCleared ? &m_message->m_param.m_callCleared : NULL;
}


OpalParamCallCleared * OpalMessagePtr::GetClearCall() const
{
  return m_message->m_type == OpalCmdClearCall ? &m_message->m_param.m_clearCall : NULL;
}


OpalStatusMediaStream * OpalMessagePtr::GetMediaStream() const
{
  switch (m_message->m_type) {
    case OpalIndMediaStream :
    case OpalCmdMediaStream :
      return &m_message->m_param.m_mediaStream;

    default :
      return NULL;
  }
}


OpalParamSetUserData * OpalMessagePtr::GetSetUserData() const
{
  return m_message->m_type == OpalCmdSetUserData ? &m_message->m_param.m_setUserData : NULL;
}


OpalParamRecording * OpalMessagePtr::GetRecording() const
{
  return m_message->m_type == OpalCmdStartRecording ? &m_message->m_param.m_recording : NULL;
}


///////////////////////////////////////////////////////////////////////////////
