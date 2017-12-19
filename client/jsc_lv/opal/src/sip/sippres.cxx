/*
 * sippres.cxx
 *
 * SIP Presence classes for Opal
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2009 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 22858 $
 * $Author: csoutheren $
 * $Date: 2009-06-12 22:50:19 +1000 (Fri, 12 Jun 2009) $
 */

/*
 * This code implements all or part of the following RFCs
 *
 * RFC 3856 "A Presence Event Package for the Session Initiation Protocol (SIP)"
 * RFC 3857 "A Watcher Information Event Template-Package for the Session Initiation Protocol (SIP)"
 * RFC 3858 "An Extensible Markup Language (XML) Based Format for Watcher Information"
 * RFC 3863 "Presence Information Data Format (PIDF)"
 * RFC 4825 "The Extensible Markup Language (XML) Configuration Access Protocol (XCAP)"
 * RFC 4827 "An Extensible Markup Language (XML) Configuration Access Protocol (XCAP) Usage for Manipulating Presence Document Contents"
 * RFC 5025 "Presence Authorization Rules"
 *
 * This code does not implement the following RFCs
 *
 * RFC 5263 "Session Initiation Protocol (SIP) Extension for Partial Notification of Presence Information"
 *
 */


#include <ptlib.h>
#include <opal/buildopts.h>

#if P_EXPAT

#include <sip/sippres.h>
#include <ptclib/pdns.h>
#include <ptclib/pxml.h>
#include <ptclib/random.h>

const PString & SIP_Presentity::DefaultPresenceServerKey() { static const PString s = "default_presence_server"; return s; }
const PString & SIP_Presentity::PresenceServerKey()        { static const PString s = "presence_server";         return s; }

PFACTORY_CREATE(PFactory<OpalPresentity>, SIPLocal_Presentity, "sip-local", false);
PFACTORY_CREATE(PFactory<OpalPresentity>, SIPXCAP_Presentity,  "sip-xcap", false);
PFACTORY_CREATE(PFactory<OpalPresentity>, SIPOMA_Presentity,   "sip-oma", false);

static bool sip_default_PresentityWorker = PFactory<OpalPresentity>::RegisterAs("sip", "sip-oma");

static const char * const AuthNames[OpalPresentity::NumAuthorisations] = { "allow", "block", "polite-block", "confirm", "remove" };


//////////////////////////////////////////////////////////////////////////////////////

static bool ParseAndValidateXML(SIPSubscribe::NotifyCallbackInfo & status, PXML & xml, const PXML::ValidationInfo * validator)
{
  PString body = status.m_notify.GetEntityBody();

  // Check for empty body, if so then is OK, just a ping ...
  if (body.IsEmpty()) {
    PTRACE(4, "SIPPres\tEmpty body on presence watcher NOTIFY, ignoring");
    status.m_response.SetStatusCode(SIP_PDU::Successful_OK);
    return false;
  }

  PStringStream err;

  // load the XML
  if (!xml.Load(body, PXML::WithNS))
    err << "XML parse";
  else if (!xml.Validate(validator))
    err << "XML validation";
  else
    return true;

  err << " error\n"
         "Error at line " << xml.GetErrorLine() << ", column " << xml.GetErrorColumn() << '\n'
      << xml.GetErrorString() << '\n';
  status.m_response.SetEntityBody(err);
  PTRACE(3, "SIPPres\tError parsing XML in presence notify: " << err);
  return false;
}


//////////////////////////////////////////////////////////////////////////////////////

SIP_Presentity::SIP_Presentity()
  : m_endpoint(NULL)
  , m_watcherInfoVersion(-1)
{
}


SIP_Presentity::~SIP_Presentity()
{
}


bool SIP_Presentity::SetDefaultPresentity(const PString & prefix)
{
  if (PFactory<OpalPresentity>::RegisterAs("sip", prefix))
    return true;

  PTRACE(1, "SIPPres\tCannot set default 'sip' presentity handler to '" << prefix << '\'');
  return false;
}


bool SIP_Presentity::Open()
{
  Close();

  // find the endpoint
  m_endpoint = dynamic_cast<SIPEndPoint *>(m_manager->FindEndPoint("sip"));
  if (m_endpoint == NULL) {
    PTRACE(1, "SIPPres\tCannot open SIP_Presentity without sip endpoint");
    return false;
  }

  m_watcherInfoVersion = -1;

  return true;
}


bool SIP_Presentity::IsOpen() const
{
  return m_endpoint != NULL;
}


bool SIP_Presentity::Close()
{
  m_endpoint = NULL;
  return true;
}


//////////////////////////////////////////////////////////////

SIPLocal_Presentity::~SIPLocal_Presentity()
{
  Close();
}


//////////////////////////////////////////////////////////////

const PString & SIPXCAP_Presentity::XcapRootKey()      { static const PString s = "xcap_root";      return s; }
const PString & SIPXCAP_Presentity::XcapAuthIdKey()    { static const PString s = "xcap_auth_id";   return s; }
const PString & SIPXCAP_Presentity::XcapPasswordKey()  { static const PString s = "xcap_password";  return s; }
const PString & SIPXCAP_Presentity::XcapAuthAuidKey()  { static const PString s = "xcap_auid";      return s; }
const PString & SIPXCAP_Presentity::XcapAuthFileKey()  { static const PString s = "xcap_authfile";  return s; }
const PString & SIPXCAP_Presentity::XcapBuddyListKey() { static const PString s = "xcap_buddylist"; return s; }

SIPXCAP_Presentity::SIPXCAP_Presentity()
{
  m_attributes.Set(SIPXCAP_Presentity::XcapAuthAuidKey,  "pres-rules");
  m_attributes.Set(SIPXCAP_Presentity::XcapAuthFileKey,  "index");
  m_attributes.Set(SIPXCAP_Presentity::XcapBuddyListKey, "buddylist");
}


SIPXCAP_Presentity::~SIPXCAP_Presentity()
{
  Close();
}


bool SIPXCAP_Presentity::Open()
{
  if (!SIP_Presentity::Open())
    return false;

  // find presence server for Presentity as per RFC 3861
  // if not found, look for default presence server setting
  // if none, use hostname portion of domain name
  if (!m_attributes.Has(SIP_Presentity::PresenceServerKey)) {
#if P_DNS
    PIPSocketAddressAndPortVector addrs;
    if (PDNS::LookupSRV(m_aor.GetHostName(), "_pres._sip", m_aor.GetPort(), addrs) && addrs.size() > 0) {
      PTRACE(1, "SIPPres\tSRV lookup for '" << m_aor.GetHostName() << "_pres._sip' succeeded");
      m_presenceServer = addrs[0];
    }
    else
#endif
      if (m_attributes.Has(SIP_Presentity::DefaultPresenceServerKey)) 
        m_presenceServer.Parse(m_attributes.Get(SIP_Presentity::DefaultPresenceServerKey), m_endpoint->GetDefaultSignalPort());
      else
        m_presenceServer.Parse(m_aor.GetHostName(), m_aor.GetPort());

    // set presence server
    m_attributes.Set(SIP_Presentity::PresenceServerKey, m_presenceServer.AsString());
  }

  if (!m_presenceServer.GetAddress().IsValid()) {
    PTRACE(1, "SIPPres\tUnable to lookup hostname for '" << m_presenceServer.GetAddress() << '\'');
    return false;
  }

  m_watcherSubscriptionAOR.MakeEmpty();

  StartThread();

  // subscribe to presence watcher infoformation
  SendCommand(CreateCommand<SIPWatcherInfoCommand>());

  return true;
}


bool SIPXCAP_Presentity::Close()
{
  if (!IsOpen())
    return false;

  StopThread();

  m_notificationMutex.Wait();

  if (!m_watcherSubscriptionAOR.IsEmpty()) {
    PTRACE(3, "SIPPres\t'" << m_aor << "' sending unsubscribe for own presence watcher");
    m_endpoint->Unsubscribe(SIPSubscribe::Presence | SIPSubscribe::Watcher, m_watcherSubscriptionAOR);
  }

  for (StringMap::iterator subs = m_presenceIdByAor.begin(); subs != m_presenceIdByAor.end(); ++subs) {
    PTRACE(3, "SIPPres\t'" << m_aor << "' sending unsubscribe to " << subs->first);
    m_endpoint->Unsubscribe(SIPSubscribe::Presence, subs->second);
  }

  const PTimeInterval LoopSleepTime(100);
  const PTimeInterval LoopWaitTime(0, 10); // Seconds
  int count = LoopWaitTime/LoopSleepTime;
  while (!m_watcherSubscriptionAOR.IsEmpty() || !m_presenceIdByAor.empty()) {
    if (--count <= 0) {
      PTRACE(1, "SIPPres\t'" << m_aor << "' did not unsubscribe to everything.");
      break;
    }
    m_notificationMutex.Signal();
    PThread::Sleep(LoopSleepTime);
    m_notificationMutex.Wait();
  }

  m_notificationMutex.Signal();

  return SIP_Presentity::Close();
}


OPAL_DEFINE_COMMAND(OpalSetLocalPresenceCommand,     SIPXCAP_Presentity, Internal_SendLocalPresence);
OPAL_DEFINE_COMMAND(OpalSubscribeToPresenceCommand,  SIPXCAP_Presentity, Internal_SubscribeToPresence);
OPAL_DEFINE_COMMAND(OpalAuthorisationRequestCommand, SIPXCAP_Presentity, Internal_AuthorisationRequest);
OPAL_DEFINE_COMMAND(SIPWatcherInfoCommand,           SIPXCAP_Presentity, Internal_SubscribeToWatcherInfo);


void SIPXCAP_Presentity::Internal_SubscribeToWatcherInfo(const SIPWatcherInfoCommand & cmd)
{

  if (cmd.m_unsubscribe) {
    if (m_watcherSubscriptionAOR.IsEmpty()) {
      PTRACE(3, "SIPPres\tAlredy unsubscribed presence watcher for " << m_aor);
      return;
    }

    PTRACE(3, "SIPPres\t'" << m_aor << "' sending unsubscribe for own presence watcher");
    m_endpoint->Unsubscribe(SIPSubscribe::Presence | SIPSubscribe::Watcher, m_watcherSubscriptionAOR);
    return;
  }

  PString aorStr = m_aor.AsString();
  PTRACE(3, "SIPPres\t'" << aorStr << "' sending subscribe for own presence.watcherinfo");

  // subscribe to the presence.winfo event on the presence server
  SIPSubscribe::Params param(SIPSubscribe::Presence | SIPSubscribe::Watcher);
  param.m_contentType      = "application/watcherinfo+xml";
  param.m_localAddress     = aorStr;
  param.m_addressOfRecord  = aorStr;
  param.m_remoteAddress    = m_presenceServer.AsString() + ";transport=tcp";
  param.m_authID           = m_attributes.Get(OpalPresentity::AuthNameKey, aorStr);
  param.m_password         = m_attributes.Get(OpalPresentity::AuthPasswordKey);
  param.m_expire           = GetExpiryTime();
  param.m_onSubcribeStatus = PCREATE_NOTIFIER2(OnWatcherInfoSubscriptionStatus, const SIPSubscribe::SubscriptionStatus &);
  param.m_onNotify         = PCREATE_NOTIFIER2(OnWatcherInfoNotify, SIPSubscribe::NotifyCallbackInfo &);

  m_endpoint->Subscribe(param, m_watcherSubscriptionAOR);
}


void SIPXCAP_Presentity::OnWatcherInfoSubscriptionStatus(SIPSubscribeHandler &, const SIPSubscribe::SubscriptionStatus & status)
{
   if (status.m_reason == SIP_PDU::Information_Trying)
    return;

  OpalPresenceInfo info(status.m_wasSubscribing ? OpalPresenceInfo::Unchanged : OpalPresenceInfo::NoPresence);
  info.m_target = info.m_entity = m_aor.AsString();

  m_notificationMutex.Wait();

  if (status.m_reason/100 == 4)
    info.m_state = OpalPresenceInfo::Forbidden;
  else if (status.m_reason/100 != 2) 
    info.m_state = OpalPresenceInfo::InternalError;

  OnPresenceChange(info);

  if (!status.m_wasSubscribing)
    m_watcherSubscriptionAOR.MakeEmpty();

  m_notificationMutex.Signal();
}


void SIPXCAP_Presentity::OnWatcherInfoNotify(SIPSubscribeHandler &, SIPSubscribe::NotifyCallbackInfo & status)
{
  static PXML::ValidationInfo const WatcherValidation[] = {
    { PXML::RequiredNonEmptyAttribute,  "id"  },
    { PXML::RequiredAttributeWithValue, "status",  { "pending\nactive\nwaiting\nterminated" } },
    { PXML::RequiredAttributeWithValue, "event",   { "subscribe\napproved\ndeactivated\nprobation\nrejected\ntimeout\ngiveup" } },
    { PXML::EndOfValidationList }
  };

  static PXML::ValidationInfo const WatcherListValidation[] = {
    { PXML::RequiredNonEmptyAttribute,  "resource" },
    { PXML::RequiredAttributeWithValue, "package", { "presence" } },

    { PXML::Subtree,                    "watcher", { WatcherValidation } , 0 },
    { PXML::EndOfValidationList }
  };

  static PXML::ValidationInfo const WatcherInfoValidation[] = {
    { PXML::SetDefaultNamespace,        "urn:ietf:params:xml:ns:watcherinfo" },
    { PXML::ElementName,                "watcherinfo", },
    { PXML::RequiredNonEmptyAttribute,  "version"},
    { PXML::RequiredAttributeWithValue, "state",   { "full\npartial" } },

    { PXML::Subtree,                    "watcher-list", { WatcherListValidation }, 0 },
    { PXML::EndOfValidationList }
  };

  PXML xml;
  if (!ParseAndValidateXML(status, xml, WatcherInfoValidation))
    return;

  // send 200 OK now, and flag caller NOT to send the response
  status.SendResponse(SIP_PDU::Successful_OK);

  PTRACE(3, "SIPPres\t'" << m_aor << "' received NOTIFY for own presence.watcherinfo");

  PXMLElement * rootElement = xml.GetRootElement();

  int version = rootElement->GetAttribute("version").AsUnsigned();

  PWaitAndSignal mutex(m_notificationMutex);

  // check version number
  bool sendRefresh = false;
  if (m_watcherInfoVersion < 0)
    m_watcherInfoVersion = version;
  else {
    sendRefresh = version != m_watcherInfoVersion+1;
    version = m_watcherInfoVersion;
  }

  // if this is a full list of watcher info, we can empty out our pending lists
  if (rootElement->GetAttribute("state") *= "full") {
    PTRACE(3, "SIPPres\t'" << m_aor << "' received full watcher list");
    m_watcherAorById.clear();
  }

  // go through list of watcher information
  PINDEX watcherListIndex = 0;
  PXMLElement * watcherList;
  while ((watcherList = rootElement->GetElement("watcher-list", watcherListIndex++)) != NULL) {
    PINDEX watcherIndex = 0;
    PXMLElement * watcher;
    while ((watcher = watcherList->GetElement("watcher", watcherIndex++)) != NULL)
      OnReceivedWatcherStatus(watcher);
  }

  // send refresh, if needed
  if (sendRefresh) {
    PTRACE(3, "SIPPres\t'" << m_aor << "' received NOTIFY for own presence.watcherinfo without out of sequence version");
    // TODO
  }
}


void SIPXCAP_Presentity::OnReceivedWatcherStatus(PXMLElement * watcher)
{
  PString id       = watcher->GetAttribute("id");
  PString status   = watcher->GetAttribute("status");
  PURL otherAOR    = watcher->GetData().Trim();

  StringMap::iterator existingAOR = m_watcherAorById.find(id);

  // save pending subscription status from this user
  if (status == "pending") {
    if (existingAOR != m_watcherAorById.end()) {
      PTRACE(3, "SIPPres\t'" << m_aor << "' received followup to request from '" << otherAOR << "' for access to presence information");
    } 
    else {
      m_watcherAorById[id] = otherAOR;
      PTRACE(3, "SIPPres\t'" << otherAOR << "' has requested access to presence information of '" << m_aor << '\'');
      OnAuthorisationRequest(otherAOR);
    }
  }
  else {
    PTRACE(3, "SIPPres\t'" << m_aor << "' has received event '" << watcher->GetAttribute("event")
           << "', status '" << status << "', for '" << otherAOR << '\'');
  }
}


void SIPXCAP_Presentity::Internal_SendLocalPresence(const OpalSetLocalPresenceCommand & cmd)
{
  PTRACE(3, "SIPPres\t'" << m_aor << "' sending own presence " << cmd.m_state << "/" << cmd.m_note);

  SIPPresenceInfo sipPresence(GetID());
  sipPresence.m_entity = m_aor.AsString();
  sipPresence.m_presenceAgent = m_presenceServer.AsString();
  sipPresence.m_state = cmd.m_state;
  sipPresence.m_note = cmd.m_note;

  if (m_publishedTupleId.IsEmpty())
    m_publishedTupleId = sipPresence.m_tupleId;
  else
    sipPresence.m_tupleId = m_publishedTupleId;

  m_endpoint->PublishPresence(sipPresence,
            cmd.m_state == OpalPresenceInfo::NoPresence ? 0 : GetExpiryTime());
}


unsigned SIPXCAP_Presentity::GetExpiryTime() const
{
  int ttl = m_attributes.Get(OpalPresentity::TimeToLiveKey, "300").AsInteger();
  return ttl > 0 ? ttl : 300;
}


void SIPXCAP_Presentity::Internal_SubscribeToPresence(const OpalSubscribeToPresenceCommand & cmd)
{
  if (cmd.m_subscribe) {
    if (m_presenceIdByAor.find(cmd.m_presentity) != m_presenceIdByAor.end()) {
      PTRACE(3, "SIPPres\t'" << m_aor << "' already subscribed to presence of '" << cmd.m_presentity << '\'');
      return;
    }

    PTRACE(3, "SIPPres\t'" << m_aor << "' subscribing to presence of '" << cmd.m_presentity << '\'');

    // subscribe to the presence event on the presence server
    SIPSubscribe::Params param(SIPSubscribe::Presence);

    param.m_localAddress    = m_aor.AsString();
    param.m_addressOfRecord = cmd.m_presentity;
    param.m_remoteAddress   = m_presenceServer.AsString()+";transport=tcp";
    param.m_authID          = m_attributes.Get(OpalPresentity::AuthNameKey, m_aor.GetUserName());
    param.m_password        = m_attributes.Get(OpalPresentity::AuthPasswordKey);
    param.m_expire          = GetExpiryTime();
    param.m_contentType     = "application/pidf+xml";
    param.m_eventList       = true;

    param.m_onSubcribeStatus = PCREATE_NOTIFIER2(OnPresenceSubscriptionStatus, const SIPSubscribe::SubscriptionStatus &);
    param.m_onNotify         = PCREATE_NOTIFIER2(OnPresenceNotify, SIPSubscribe::NotifyCallbackInfo &);

    PString id;
    if (m_endpoint->Subscribe(param, id, false)) {
      m_presenceIdByAor[cmd.m_presentity] = id;
      m_presenceAorById[id] = cmd.m_presentity;
    }
  }
  else {
    StringMap::iterator id = m_presenceIdByAor.find(cmd.m_presentity);
    if (id == m_presenceIdByAor.end()) {
      PTRACE(3, "SIPPres\t'" << m_aor << "' already unsubscribed to presence of '" << cmd.m_presentity << '\'');
      return;
    }

    PTRACE(3, "SIPPres\t'" << m_aor << "' unsubscribing to presence of '" << cmd.m_presentity << '\'');
    m_endpoint->Unsubscribe(SIPSubscribe::Presence, id->second);
  }
}


void SIPXCAP_Presentity::OnPresenceSubscriptionStatus(SIPSubscribeHandler &, const SIPSubscribe::SubscriptionStatus & status)
{
  if (status.m_reason == SIP_PDU::Information_Trying)
    return;

  m_notificationMutex.Wait();
  if (!status.m_wasSubscribing || status.m_reason >= 400) {
    PString id = status.m_handler->GetCallID();
    StringMap::iterator aor = m_presenceAorById.find(id);
    if (aor != m_presenceAorById.end()) {
      PTRACE(status.m_reason >= 400 ? 2 : 3, "SIPPres\t'" << m_aor << "' "
             << (status.m_wasSubscribing ? "error " : "un")
             << "subscribing to presence of '" << aor->second << '\'');
      m_presenceIdByAor.erase(aor->second);
      m_presenceAorById.erase(aor);
    }
  }
  m_notificationMutex.Signal();
}


void SIPXCAP_Presentity::OnPresenceNotify(SIPSubscribeHandler &, SIPSubscribe::NotifyCallbackInfo & status)
{
  static PXML::ValidationInfo const StatusValidation[] = {
    { PXML::RequiredElement,            "basic" },
    { PXML::EndOfValidationList }
  };

  static PXML::ValidationInfo const TupleValidation[] = {
    { PXML::RequiredNonEmptyAttribute,  "id" },
    { PXML::Subtree,                    "status", { StatusValidation }, 1 },
    { PXML::EndOfValidationList }
  };

  static PXML::ValidationInfo const ActivitiesValidation[] = {
    { PXML::OptionalElement,            "rpid:busy" },
    { PXML::EndOfValidationList }
  };

  static PXML::ValidationInfo const PersonValidation[] = {
    { PXML::Subtree,                    "rpid:activities", { ActivitiesValidation }, 0, 1 },
    { PXML::EndOfValidationList }
  };

  static PXML::ValidationInfo const PresenceValidation[] = {
    { PXML::SetDefaultNamespace,        "urn:ietf:params:xml:ns:pidf" },
    { PXML::SetNamespace,               "dm",   { "urn:ietf:params:xml:ns:pidf:data-model" } },
    { PXML::SetNamespace,               "rpid", { "urn:ietf:params:xml:ns:pidf:rpid" } },
    { PXML::ElementName,                "presence" },
    { PXML::RequiredNonEmptyAttribute,  "entity" },
    { PXML::Subtree,                    "tuple",  { TupleValidation }, 0 },
    { PXML::Subtree,                    "dm:person", { PersonValidation }, 0, 1 },
    { PXML::EndOfValidationList }
  };

  PXML xml;
  if (!ParseAndValidateXML(status, xml, PresenceValidation)) 
    return;

  // send 200 OK now, and flag caller NOT to send the response
  status.SendResponse(SIP_PDU::Successful_OK);

  SIPPresenceInfo info(SIPPresenceInfo::Unchanged);

  info.m_target = m_aor.AsString();

  PXMLElement * rootElement = xml.GetRootElement();
  info.m_entity = rootElement->GetAttribute("entity");

  PCaselessString pidf;
  PXMLElement * tupleElement = rootElement->GetElement("tuple");
  if (tupleElement != NULL) {
    PXMLElement * statusElement = tupleElement->GetElement("status");

    PCaselessString value = statusElement->GetElement("basic")->GetData();
    if (value == "open")
      info.m_state = SIPPresenceInfo::Available;
    else if (value == "closed")
      info.m_state = SIPPresenceInfo::NoPresence;

    PXMLElement * noteElement;
    if ((noteElement = statusElement->GetElement("note")) != NULL ||
        (noteElement = rootElement->GetElement("note")) != NULL ||
        (noteElement = tupleElement->GetElement("note")) != NULL)
      info.m_note = noteElement->GetData();

    PXMLElement * contactElement = tupleElement->GetElement("contact");
    if (contactElement != NULL)
      info.m_contact = contactElement->GetData();
  }

  static PCaselessString rpid("urn:ietf:params:xml:ns:pidf:rpid|");
  static PCaselessString dm  ("urn:ietf:params:xml:ns:pidf:data-model|");
  static const int rpidLen = rpid.GetLength();

  PXMLElement * person = xml.GetElement(dm + "person");
  if (person != NULL) {
    PXMLElement * activities = person->GetElement(rpid + "activities");
    if (activities != NULL) {
      for (PINDEX i = 0; i < activities->GetSize(); ++i) {
        if (!activities->GetElement(i)->IsElement())
          continue;
        PString name(((PXMLElement *)activities->GetElement(i))->GetName());
        if (name.GetLength() < (rpidLen+1) || !(name.Left(rpidLen) *= rpid) || (name[rpidLen] != '|'))
          continue;
        SIPPresenceInfo::State state = SIPPresenceInfo::FromSIPActivityString(name.Mid(rpidLen+1));
        if (state == SIPPresenceInfo::NoPresence)
          continue;
        if ((info.m_activities.GetSize() == 0) && (info.m_state == SIPPresenceInfo::Available))
          info.m_state = state;
        else
          info.m_activities.AppendString(OpalPresenceInfo::AsString(state));
      }
    }
  }

  PTRACE(3, "SIPPres\t'" << info.m_entity << "' request for presence of '" << m_aor << "' is " << info.m_state);

  m_notificationMutex.Wait();
  OnPresenceChange(info);
  m_notificationMutex.Signal();
}


bool SIPXCAP_Presentity::ChangeAuthNode(XCAPClient & xcap, const OpalAuthorisationRequestCommand & cmd)
{
  PString ruleId = m_authorisationIdByAor[cmd.m_presentity];

  XCAPClient::NodeSelector node;
  node.SetNamespace("urn:ietf:params:xml:ns:pres-rules", "pr");
  node.SetNamespace("urn:ietf:params:xml:ns:common-policy", "cr");
  node.AddElement("cr:ruleset");
  node.AddElement("cr:rule", "id", ruleId);
  xcap.SetNode(node);

  if (cmd.m_authorisation == AuthorisationRemove) {
    if (xcap.DeleteXml()) {
      PTRACE(3, "SIPPres\tRule id=" << ruleId << " removed for '" << cmd.m_presentity << "' at '" << m_aor << '\'');
    }
    else {
      PTRACE(3, "SIPPres\tCould not remove rule id=" << ruleId
             << " for '" << cmd.m_presentity << "' at '" << m_aor << "\'\n"
             << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
    }
    return true;
  }

  PXML xml;
  if (!xcap.GetXml(xml)) {
    PTRACE(3, "SIPPres\tCould not locate existing rule id=" << ruleId
           << " for '" << cmd.m_presentity << "' at '" << m_aor << '\'');
    return false;
  }

  PXMLElement * root, * actions, * subHandling = NULL;
  if ((root = xml.GetRootElement()) == NULL ||
      (actions = root->GetElement("cr:actions")) == NULL ||
      (subHandling = actions->GetElement("pr:sub-handling")) == NULL) {
    PTRACE(2, "SIPPres\tInvalid XML in existing rule id=" << ruleId
           << " for '" << cmd.m_presentity << "' at '" << m_aor << '\'');
    return false;
  }

  if (subHandling->GetData() *= AuthNames[cmd.m_authorisation]) {
    PTRACE(3, "SIPPres\tRule id=" << ruleId << " already set to "
           << AuthNames[cmd.m_authorisation] << " for '" << cmd.m_presentity << "' at '" << m_aor << '\'');
    return true;
  }

  // Adjust the authorisation
  subHandling->SetData(AuthNames[cmd.m_authorisation]);

  if (xcap.PutXml(xml)) {
    PTRACE(3, "SIPPres\tRule id=" << ruleId << " changed to"
           << AuthNames[cmd.m_authorisation] << " for '" << cmd.m_presentity << "' at '" << m_aor << '\'');
    return true;
  }

  PTRACE(3, "SIPPres\tCould not change existing rule id=" << ruleId
         << " for '" << cmd.m_presentity << "' at '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
  return false;
}


void SIPXCAP_Presentity::Internal_AuthorisationRequest(const OpalAuthorisationRequestCommand & cmd)
{
  XCAPClient xcap;
  xcap.SetRoot(m_attributes.Get(SIPXCAP_Presentity::XcapRootKey));
  xcap.SetApplicationUniqueID(m_attributes.Get(SIPXCAP_Presentity::XcapAuthAuidKey)); // As per RFC5025/9.1
  xcap.SetContentType("application/auth-policy+xml");   // As per RFC5025/9.4
  xcap.SetUserIdentifier(m_aor.AsString());             // As per RFC5025/9.7
  xcap.SetAuthenticationInfo(m_attributes.Get(SIPXCAP_Presentity::XcapAuthIdKey, m_attributes.Get(OpalPresentity::AuthNameKey, xcap.GetUserIdentifier())),
                             m_attributes.Get(SIPXCAP_Presentity::XcapPasswordKey, m_attributes.Get(OpalPresentity::AuthPasswordKey)));
  xcap.SetFilename(m_attributes.Get(SIPXCAP_Presentity::XcapAuthFileKey));

  // See if we can use teh quick method
  if (m_authorisationIdByAor.find(cmd.m_presentity) != m_authorisationIdByAor.end()) {
    if (ChangeAuthNode(xcap, cmd))
      return;
  }

  PXML xml;
  PXMLElement * element;

  // Could not find rule element quickly, get the whole document
  if (!xcap.GetXml(xml)) {
    if (xcap.GetLastResponseCode() != PHTTP::NotFound) {
      PTRACE(2, "SIPPres\tUnexpected error getting rule file for '"
             << cmd.m_presentity << "' at '" << m_aor << "\'\n"
             << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
      return;
    }

    // No whole document, create a fresh one
    InitRuleSet(xml);

    if (!xcap.PutXml(xml)) {
      PTRACE(2, "SIPPres\tCould not add new rule file for '" << m_aor << "\'\n"
             << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
      return;
    }

    PTRACE(3, "SIPPres\tNew rule file created for '" << m_aor << '\'');
  }

  // Extract all the rules from it
  m_authorisationIdByAor.clear();

  bool existingRuleForWatcher = false;

  PINDEX ruleIndex = 0;
  while ((element = xml.GetElement("cr:rule", ruleIndex++)) != NULL) {
    PString ruleId = element->GetAttribute("id");
    if ((element = element->GetElement("cr:conditions")) != NULL &&
        (element = element->GetElement("cr:identity")) != NULL &&
        (element = element->GetElement("cr:one")) != NULL) {
      PString watcher = element->GetAttribute("id");

      m_authorisationIdByAor[watcher] = ruleId;
      PTRACE(4, "SIPPres\tGetting rule id=" << ruleId << " for '" << watcher << "' at '" << m_aor << '\'');

      if (watcher == cmd.m_presentity)
        existingRuleForWatcher = true;
    }
  }

  if (existingRuleForWatcher) {
    ChangeAuthNode(xcap, cmd);
    return;
  }

  // Create new rule with id as per http://www.w3.org/TR/1999/REC-xml-names-19990114/#NT-NCName
  static PAtomicInteger NextRuleId(PRandom::Number());
  PString newRuleId(PString::Printf, "wp_prs%s_one_%lu",
                    cmd.m_authorisation == AuthorisationPermitted ? "_allow" : "",
                    ++NextRuleId);

  xml.SetOptions(PXML::FragmentOnly);
  element = xml.SetRootElement("cr:rule");
  element->SetAttribute("xmlns:pr", "urn:ietf:params:xml:ns:pres-rules");
  element->SetAttribute("xmlns:cr", "urn:ietf:params:xml:ns:common-policy");
  element->SetAttribute("id", newRuleId);

  element->AddElement("cr:conditions")->AddElement("cr:identity")->AddElement("cr:one")->SetAttribute("id", cmd.m_presentity);
  element->AddElement("cr:actions")->AddElement("pr:sub-handling")->SetData(AuthNames[cmd.m_authorisation]);
  element = element->AddElement("cr:transformations");
  element->AddElement("pr:provide-services")->AddElement("pr:all-services");
  element->AddElement("pr:provide-persons")->AddElement("pr:all-persons");
  element->AddElement("pr:provide-all-attributes");

  XCAPClient::NodeSelector node;
  node.SetNamespace("urn:ietf:params:xml:ns:pres-rules", "pr");
  node.SetNamespace("urn:ietf:params:xml:ns:common-policy", "cr");
  node.AddElement("cr:ruleset");
  node.AddElement("cr:rule", "id", newRuleId);
  xcap.SetNode(node);

  if (xcap.PutXml(xml)) {
    PTRACE(3, "SIPPres\tNew rule set to" << AuthNames[cmd.m_authorisation] << " for '" << cmd.m_presentity << "' at '" << m_aor << '\'');
    m_authorisationIdByAor[cmd.m_presentity] = newRuleId;
    return;
  }

  PTRACE(2, "SIPPres\tCould not add new rule for '" << cmd.m_presentity << "' at '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
}


void SIPXCAP_Presentity::InitRuleSet(PXML & xml)
{
  xml.SetOptions(PXML::NoOptions);
  PXMLElement * element = xml.SetRootElement("cr:ruleset");
  element->SetAttribute("xmlns:pr", "urn:ietf:params:xml:ns:pres-rules");
  element->SetAttribute("xmlns:cr", "urn:ietf:params:xml:ns:common-policy");

  element = element->AddElement("rule", "id", "presence_allow_own");

  element->AddElement("cr:conditions")->AddElement("cr:identity")->AddElement("cr:one")->SetAttribute("id", m_aor.AsString());
  element->AddElement("cr:actions")->AddElement("pr:sub-handling", AuthNames[AuthorisationPermitted]);
  element = element->AddElement("cr:transformations");
  element->AddElement("pr:provide-services")->AddElement("pr:all-services");
  element->AddElement("pr:provide-persons")->AddElement("pr:all-persons");
  element->AddElement("pr:provide-all-attributes");
}


void SIPXCAP_Presentity::InitBuddyXcap(XCAPClient & xcap, const PString & entryName, const PString & listName)
{
  xcap.SetRoot(m_attributes.Get(SIPXCAP_Presentity::XcapRootKey));
  xcap.SetApplicationUniqueID("resource-lists");            // As per RFC5025/9.1
  xcap.SetContentType("application/resource-lists+xml");    // As per RFC5025/9.4
  xcap.SetUserIdentifier(m_aor.AsString());                 // As per RFC5025/9.7
  xcap.SetAuthenticationInfo(m_attributes.Get(SIPXCAP_Presentity::XcapAuthIdKey, m_attributes.Get(OpalPresentity::AuthNameKey, xcap.GetUserIdentifier())),
                             m_attributes.Get(SIPXCAP_Presentity::XcapPasswordKey, m_attributes.Get(OpalPresentity::AuthPasswordKey)));
  xcap.SetFilename("index");

  XCAPClient::NodeSelector node;
  node.SetNamespace("urn:ietf:params:xml:ns:resource-lists");
  node.AddElement("resource-lists");
  node.AddElement("list", "name", listName.IsEmpty() ? m_attributes.Get(SIPXCAP_Presentity::XcapBuddyListKey) : listName);

  if (!entryName.IsEmpty())
    node.AddElement("entry", "uri", entryName);

  xcap.SetNode(node);
}


static PXMLElement * BuddyInfoToXML(const OpalPresentity::BuddyInfo & buddy, PXMLElement * parent)
{
  PXMLElement * element = new PXMLElement(parent, "entry");
  element->SetAttribute("uri", buddy.m_presentity);

  if (!buddy.m_displayName.IsEmpty())
    element->AddElement("display-name", buddy.m_displayName);

  return element;
}


static bool XMLToBuddyInfo(const PXMLElement * element, OpalPresentity::BuddyInfo & buddy)
{
  if (element == NULL || element->GetName() != "entry")
    return false;

  buddy.m_presentity = element->GetAttribute("uri");

  PXMLElement * displayName = element->GetElement("display-name");
  if (displayName != NULL)
    buddy.m_displayName = displayName->GetData();

  buddy.m_contentType = "application/resource-lists+xml";
  buddy.m_rawXML = element->AsString();
  return true;
}


static bool RecursiveGetBuddyList(OpalPresentity::BuddyList & buddies, XCAPClient & xcap, const PURL & url)
{
  if (url.IsEmpty())
    return false;

  PXML xml;
  if (!xcap.GetXml(url, xml))
    return false;

  PXMLElement * element;
  PINDEX idx = 0;
  while ((element = xml.GetElement("entry", idx++)) != NULL) {
    OpalPresentity::BuddyInfo buddy;
    if (XMLToBuddyInfo(element, buddy))
      buddies.push_back(buddy);
  }

  idx = 0;
  while ((element = xml.GetElement("external", idx++)) != NULL)
    RecursiveGetBuddyList(buddies, xcap, element->GetAttribute("anchor"));

  idx = 0;
  while ((element = xml.GetElement("entry-ref", idx++)) != NULL) {
    PURL url(xcap.GetRoot());
    url.SetPathStr(url.GetPathStr() + element->GetAttribute("ref"));
    RecursiveGetBuddyList(buddies, xcap, url);
  }

  return true;
}


bool SIPXCAP_Presentity::GetBuddyList(BuddyList & buddies)
{
  XCAPClient xcap;
  InitBuddyXcap(xcap);
  return RecursiveGetBuddyList(buddies, xcap, xcap.BuildURL()) ||
         !buddies.empty() ||
         xcap.GetLastResponseCode() == PHTTP::NotFound;
}


bool SIPXCAP_Presentity::SetBuddyList(const BuddyList & buddies)
{
  PXML xml(PXML::FragmentOnly);

  PXMLElement * root = xml.SetRootElement("list");
  root->SetAttribute("xmlns", "urn:ietf:params:xml:ns:resource-lists");
  root->SetAttribute("name", m_attributes.Get(SIPXCAP_Presentity::XcapBuddyListKey));

  for (BuddyList::const_iterator it = buddies.begin(); it != buddies.end(); ++it)
    root->AddChild(BuddyInfoToXML(*it, root));

  XCAPClient xcap;
  InitBuddyXcap(xcap);

  if (xcap.PutXml(xml))
    return true;

  if (xcap.GetLastResponseCode() == PHTTP::Conflict && xcap.GetLastResponseInfo().Find("Parent") != P_MAX_INDEX) {
    // Got "Parent does not exist" error, so need to add whole file
    xml.SetOptions(PXML::NoOptions);
    root = xml.SetRootElement("resource-lists");
    root->SetAttribute("xmlns", "urn:ietf:params:xml:ns:resource-lists");

    PXMLElement * listElement = root->AddElement("list", "name", m_attributes.Get(SIPXCAP_Presentity::XcapBuddyListKey));

    for (BuddyList::const_iterator it = buddies.begin(); it != buddies.end(); ++it)
      listElement->AddChild(BuddyInfoToXML(*it, listElement));

    xcap.ClearNode();
    if (xcap.PutXml(xml))
      return true;
  }

  PTRACE(2, "SIPPres\tError setting buddy list of '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
  return false;
}


bool SIPXCAP_Presentity::DeleteBuddyList()
{
  XCAPClient xcap;
  InitBuddyXcap(xcap);

  if (xcap.DeleteXml())
    return true;

  PTRACE(2, "SIPPres\tError deleting buddy list of '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
  return false;
}


bool SIPXCAP_Presentity::GetBuddy(BuddyInfo & buddy)
{
  XCAPClient xcap;
  InitBuddyXcap(xcap, buddy.m_presentity);

  PXML xml;
  return xcap.GetXml(xml) && XMLToBuddyInfo(xml.GetRootElement(), buddy);
}


bool SIPXCAP_Presentity::SetBuddy(const BuddyInfo & buddy)
{
  if (buddy.m_presentity.IsEmpty())
    return false;

  XCAPClient xcap;
  InitBuddyXcap(xcap, buddy.m_presentity);

  PXML xml(PXML::FragmentOnly);
  xml.SetRootElement(BuddyInfoToXML(buddy, NULL));

  if (xcap.PutXml(xml))
    return true;

  if (xcap.GetLastResponseCode() != PHTTP::Conflict || xcap.GetLastResponseInfo().Find("Parent") == P_MAX_INDEX) {
    PTRACE(2, "SIPPres\tError setting buddy '" << buddy.m_presentity << "' of '" << m_aor << "\'\n"
           << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
    return false;
  }

  // Got "Parent does not exist" error, so need to add whole list
  BuddyList buddies;
  buddies.push_back(buddy);
  return SetBuddyList(buddies);
}


bool SIPXCAP_Presentity::DeleteBuddy(const PURL & presentity)
{
  XCAPClient xcap;
  InitBuddyXcap(xcap, presentity);

  if (xcap.DeleteXml())
    return true;

  PTRACE(2, "SIPPres\tError deleting buddy '" << presentity << "' of '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
  return false;
}


bool SIPXCAP_Presentity::SubscribeBuddyList(bool subscribe)
{
  PXML xml;
  XCAPClient xcap;
  xcap.SetRoot(m_attributes.Get(SIPXCAP_Presentity::XcapRootKey));
  xcap.SetApplicationUniqueID("rls-services");
  xcap.SetContentType("application/rls-services+xml");
  xcap.SetUserIdentifier(m_aor.AsString());
  xcap.SetAuthenticationInfo(m_attributes.Get(SIPXCAP_Presentity::XcapAuthIdKey, m_attributes.Get(OpalPresentity::AuthNameKey, xcap.GetUserIdentifier())),
                             m_attributes.Get(SIPXCAP_Presentity::XcapPasswordKey, m_attributes.Get(OpalPresentity::AuthPasswordKey)));
  xcap.SetFilename("index");

  PString serviceURI = xcap.GetUserIdentifier() + ";pres-list=oma_buddylist";

  if (!xcap.GetXml(xml)) {
    if (xcap.GetLastResponseCode() != PHTTP::NotFound) {
      PTRACE(2, "SIPPres\tUnexpected error getting rls-services file for at '" << m_aor << "\'\n"
             << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
      return OpalPresentity::SubscribeBuddyList(subscribe); // Do individual subscribes
    }

    // No file at all, add the root element.
    xml.SetRootElement("rls-services")->SetAttribute("xmlns", "urn:ietf:params:xml:ns:rls-services");
  }
  else {
    // Have file, see if have specific service
    PXMLElement * element = xml.GetElement("service", "uri", serviceURI);
    if (element != NULL) {
      PTRACE(4, "SIPPres\tConfirmed rls-services entry for '" << serviceURI << "\' is\n" << xml);
      return SubscribeToPresence(serviceURI, subscribe);
    }

    // Nope, so add it
  }

  // Added service to existing XML or newly rooted one
  PXMLElement * element = xml.GetRootElement()->AddElement("service");
  element->SetAttribute("uri", serviceURI);

  // Calculate the horrible URL using a temporary XCAP client
  XCAPClient xcapTemp;
  InitBuddyXcap(xcapTemp);
  element->AddElement("resource-list")->SetData(xcapTemp.BuildURL().AsString());

  element->AddElement("packages")->AddElement("package")->SetData("presence");

  if (xcap.PutXml(xml))
    return SubscribeToPresence(serviceURI, subscribe);

  PTRACE(2, "SIPPres\tCould not add new rls-services entry for '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());

  return OpalPresentity::SubscribeBuddyList(subscribe); // Do individual subscribes
}


//////////////////////////////////////////////////////////////

SIPOMA_Presentity::SIPOMA_Presentity()
{
  m_attributes.Set(SIPXCAP_Presentity::XcapAuthAuidKey,  "org.openmobilealliance.pres-rules");
  m_attributes.Set(SIPXCAP_Presentity::XcapAuthFileKey,  "pres-rules");
  m_attributes.Set(SIPXCAP_Presentity::XcapBuddyListKey, "oma_buddylist");
}


void SIPOMA_Presentity::InitRuleSet(PXML & xml)
{
  SIPXCAP_Presentity::InitRuleSet(xml);

  PXMLElement * root = xml.GetRootElement();
  root->SetAttribute("xmlns:ocp", "urn:oma:xml:xdm:common-policy");

  PXMLElement * element = root->AddElement("cr:rule", "id", "wp_prs_block_anonymous");
  element->AddElement("cr:conditions")->AddElement("ocp:anonymous-request");
  element->AddElement("cr:actions")->AddElement("pr:sub-handling", AuthNames[AuthorisationDenied]);

  element = root->AddElement("cr:rule", "id", "wp_prs_unlisted");
  element->AddElement("cr:conditions")->AddElement("ocp:other-identity");
  element->AddElement("cr:actions")->AddElement("pr:sub-handling", AuthNames[AuthorisationConfirming]);

  // Use an xcap object to calculate horrible URL
  XCAPClient xcap;
  InitBuddyXcap(xcap, PString::Empty(), "oma_grantedcontacts");

  element = root->AddElement("cr:rule", "id", "wp_prs_grantedcontacts");
  element->AddElement("cr:conditions")->AddElement("ocp:external-list")->AddElement("ocp:entry", "anc", xcap.BuildURL().AsString());
  element->AddElement("cr:actions")->AddElement("pr:sub-handling", AuthNames[AuthorisationPermitted]);
  element = element->AddElement("cr:transformations");
  element->AddElement("pr:provide-services")->AddElement("pr:all-services");
  element->AddElement("pr:provide-persons")->AddElement("pr:all-persons");
  element->AddElement("pr:provide-all-attributes");

  InitBuddyXcap(xcap, PString::Empty(), "oma_blockedcontacts");

  element = root->AddElement("cr:rule", "id", "wp_prs_blockedcontacts");
  element->AddElement("cr:conditions")->AddElement("ocp:external-list")->AddElement("ocp:entry", "anc", xcap.BuildURL().AsString());
  element->AddElement("cr:actions")->AddElement("pr:sub-handling", AuthNames[AuthorisationDenied]);
}


//////////////////////////////////////////////////////////////

XCAPClient::XCAPClient()
  : m_global(false)
  , m_filename("index")
{
}


PURL XCAPClient::BuildURL()
{
  PURL uri(m_root);                              // XCAP root

  uri.AppendPath(m_auid);                        // Application Unique ID

  uri.AppendPath(m_global ? "global" : "users"); // RFC4825/6.2, The path segment after the AUID MUST either be "global" or "users".

  if (!m_global)
    uri.AppendPath(m_xui);                       // XCAP User Identifier

  if (!m_filename.IsEmpty()) {
    uri.AppendPath(m_filename);                        // Final resource name
    m_node.AddToURL(uri);
  }

  return uri;
}


static bool HasNode(const PURL & url)
{
  const PStringArray & path = url.GetPath();
  for (PINDEX i = 0; i < path.GetSize(); ++i) {
    if (path[i] == "~~")
      return true;
  }

  return false;
}


bool XCAPClient::GetXml(const PURL & url, PXML & xml)
{
  bool hasNode = HasNode(url);

  PString body;
  if (!GetTextDocument(url, body, hasNode ? "application/xcap-el+xml" : m_contentType)) {
    PTRACE(3, "SIPPres\tError getting buddy list at '" << url << "\'\n"
           << GetLastResponseCode() << ' '  << GetLastResponseInfo());
    return false;
  }

  if (xml.Load(body, hasNode ? PXML::FragmentOnly : PXML::NoOptions))
    return true;

  PTRACE(2, "XCAP\tError parsing XML for '" << url << "\'\n"
            "Line " << xml.GetErrorLine() << ", Column " << xml.GetErrorColumn() << ": " << xml.GetErrorString());
  return false;
}


bool XCAPClient::PutXml(const PURL & url, const PXML & xml)
{
  PStringStream strm;
  strm << xml;
  return PutTextDocument(url, strm, HasNode(url) ? "application/xcap-el+xml" : m_contentType);
}


PString XCAPClient::ElementSelector::AsString() const
{
  PStringStream strm;

  strm << m_name;

  if (!m_position.IsEmpty())
    strm << '[' << m_position << ']';

  if (!m_attribute.IsEmpty())
    strm << "[@" << m_attribute << "=\"" << m_value << "\"]";

  return strm;
}


void XCAPClient::NodeSelector::AddToURL(PURL & uri) const
{
  if (empty())
    return;

  uri.AppendPath("~~");                      // Node selector

  for (const_iterator it = begin(); it != end(); ++it)
    uri.AppendPath(it->AsString());

  if (m_namespaces.empty())
    return;

  PStringStream query;
  for (std::map<PString, PString>::const_iterator it = m_namespaces.begin(); it != m_namespaces.end(); ++it) {
    query << "xmlns(";
    if (!it->first.IsEmpty())
      query << it->first << '=';
    query << it->second << ')';
  }

  // Non-standard format query parameter, we fake that by having one
  // dictionary element at key value empty string
  uri.SetQueryVar(PString::Empty(), query);
}


#endif // P_EXPAT
