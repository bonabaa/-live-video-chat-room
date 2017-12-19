/*
 * pres_ent.cxx
 *
 * Presence Entity classes for Opal
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


#include <ptlib.h>
#include <opal/buildopts.h>

#include <opal/pres_ent.h>

#include <opal/manager.h>
#include <ptclib/url.h>
#include <sip/sipep.h>

///////////////////////////////////////////////////////////////////////

const PString & OpalPresentity::AuthNameKey()        { static const PString s = "auth_name";         return s; }
const PString & OpalPresentity::AuthPasswordKey()    { static const PString s = "auth_password";     return s; }
const PString & OpalPresentity::FullNameKey()        { static const PString s = "full_name";         return s; }
const PString & OpalPresentity::SchemeKey()          { static const PString s = "scheme";            return s; }
const PString & OpalPresentity::TimeToLiveKey()      { static const PString s = "time_to_live";      return s; }


PString OpalPresenceInfo::AsString() const
{
  return AsString(m_state);
}

ostream & operator<<(ostream & strm, OpalPresenceInfo::State state)
{
  return strm << OpalPresenceInfo::AsString(state);
}

//
//  defined in RFC 3856
//
static const char * const BasicNames[] = {
  "Internal Error",
  "Forbidden",
  "No Presence",
  "Unchanged",
  "Available",
  "Unavailable"
};

//
// defined in RFC 4480 as "activities"
//
static const char * const ExtendedNames[] = {
  "UnknownExtended",
  "Appointment",
  "Away",
  "Breakfast",
  "Busy",
  "Dinner",
  "Holiday",
  "InTransit",
  "LookingForWork",
  "Lunch",
  "Meal",
  "Meeting",
  "OnThePhone",
  "Other",
  "Performance",
  "PermanentAbsence",
  "Playing",
  "Presentation",
  "Shopping",
  "Sleeping",
  "Spectator",
  "Steering",
  "Travel",
  "TV",
  "Vacation",
  "Working",
  "Worship"
};

PString OpalPresenceInfo::AsString(State state)
{
  if (state >= OpalPresenceInfo::InternalError) {
    PINDEX index = state - OpalPresenceInfo::InternalError;
    if (index < PARRAYSIZE(BasicNames)) 
      return BasicNames[index];
  }

  if (state >= OpalPresenceInfo::ExtendedBase) {
    PINDEX index = state - OpalPresenceInfo::ExtendedBase;
    if (index < PARRAYSIZE(ExtendedNames))
      return ExtendedNames[index];
  }

  PStringStream strm;
  strm << "Presence<" << (unsigned)state << '>';
  return strm;
}


OpalPresenceInfo::State OpalPresenceInfo::FromString(const PString & stateString)
{
  if (stateString *= "Unchanged")
    return OpalPresenceInfo::Unchanged;
  if (stateString *= "Available")
    return OpalPresenceInfo::Available;
  if (stateString *= "Unavailable")
    return OpalPresenceInfo::Unavailable;

  for (size_t k = 0; k < sizeof(ExtendedNames)/sizeof(ExtendedNames[0]); ++k) {
    if (stateString *= ExtendedNames[k]) 
      return (OpalPresenceInfo::State)(ExtendedBase + k);
  }

  return NoPresence;
}

///////////////////////////////////////////////////////////////////////

static PAtomicInteger::IntegerType g_idNumber = 1;


OpalPresentity::OpalPresentity()
  : m_manager(NULL)
  , m_idNumber(g_idNumber++)
{
}


OpalPresentity * OpalPresentity::Create(OpalManager & manager, const PURL & url, const PString & scheme)
{
  OpalPresentity * presEntity = PFactory<OpalPresentity>::CreateInstance(scheme.IsEmpty() ? url.GetScheme() : scheme);
  if (presEntity == NULL) 
    return NULL;

  presEntity->m_manager = &manager;
  presEntity->m_aor = url;

  return presEntity;
}


bool OpalPresentity::SubscribeToPresence(const PURL & presentity, bool subscribe)
{
  OpalSubscribeToPresenceCommand * cmd = CreateCommand<OpalSubscribeToPresenceCommand>();
  if (cmd == NULL)
    return false;

  cmd->m_presentity = presentity;
  cmd->m_subscribe  = subscribe;
  SendCommand(cmd);
  return true;
}


bool OpalPresentity::UnsubscribeFromPresence(const PURL & presentity)
{
  return SubscribeToPresence(presentity, false);
}


bool OpalPresentity::SetPresenceAuthorisation(const PURL & presentity, Authorisation authorisation)
{
  OpalAuthorisationRequestCommand * cmd = CreateCommand<OpalAuthorisationRequestCommand>();
  if (cmd == NULL)
    return false;

  cmd->m_presentity = presentity;
  cmd->m_authorisation = authorisation;
  SendCommand(cmd);
  return true;
}


bool OpalPresentity::SetLocalPresence(OpalPresenceInfo::State state, const PString & note)
{
  OpalSetLocalPresenceCommand * cmd = CreateCommand<OpalSetLocalPresenceCommand>();
  if (cmd == NULL)
    return false;

  cmd->m_state = state;
  cmd->m_note = note;
  SendCommand(cmd);
  return true;
}


void OpalPresentity::OnAuthorisationRequest(const PURL & presentity)
{
  PWaitAndSignal mutex(m_notificationMutex);

  if (m_onAuthorisationRequestNotifier.IsNULL())
    SetPresenceAuthorisation(presentity, AuthorisationPermitted);
  else
    m_onAuthorisationRequestNotifier(*this, presentity);
}


void OpalPresentity::SetAuthorisationRequestNotifier(const AuthorisationRequestNotifier & notifier)
{
  PWaitAndSignal mutex(m_notificationMutex);

  m_onAuthorisationRequestNotifier = notifier;
}


void OpalPresentity::OnPresenceChange(const OpalPresenceInfo & info)
{
  PWaitAndSignal mutex(m_notificationMutex);

  if (!m_onPresenceChangeNotifier.IsNULL())
    m_onPresenceChangeNotifier(*this, info);
}


void OpalPresentity::SetPresenceChangeNotifier(const PresenceChangeNotifier & notifier)
{
  PWaitAndSignal mutex(m_notificationMutex);

  m_onPresenceChangeNotifier = notifier;
}


bool OpalPresentity::GetBuddyList(BuddyList &)
{
  return false;
}


bool OpalPresentity::SetBuddyList(const BuddyList &)
{
  return false;
}


bool OpalPresentity::DeleteBuddyList()
{
  return false;
}


bool OpalPresentity::GetBuddy(BuddyInfo & buddy)
{
  if (buddy.m_presentity.IsEmpty())
    return false;

  BuddyList buddies;
  if (!GetBuddyList(buddies))
    return false;

  for (BuddyList::iterator it = buddies.begin(); it != buddies.end(); ++it) {
    if (it->m_presentity == buddy.m_presentity) {
      buddy = *it;
      return true;
    }
  }

  return false;
}


bool OpalPresentity::SetBuddy(const BuddyInfo & buddy)
{
  if (buddy.m_presentity.IsEmpty())
    return false;

  BuddyList buddies;
  if (!GetBuddyList(buddies))
    return false;

  buddies.push_back(buddy);
  return SetBuddyList(buddies);
}


bool OpalPresentity::DeleteBuddy(const PURL & presentity)
{
  if (presentity.IsEmpty())
    return false;

  BuddyList buddies;
  if (!GetBuddyList(buddies))
    return false;

  for (BuddyList::iterator it = buddies.begin(); it != buddies.end(); ++it) {
    if (it->m_presentity == presentity) {
      buddies.erase(it);
      return SetBuddyList(buddies);
    }
  }

  return false;
}


bool OpalPresentity::SubscribeBuddyList(bool subscribe)
{
  BuddyList buddies;
  if (!GetBuddyList(buddies))
    return false;

  for (BuddyList::iterator it = buddies.begin(); it != buddies.end(); ++it) {
    if (!SubscribeToPresence(it->m_presentity, subscribe))
      return false;
  }

  return true;
}

bool OpalPresentity::UnsubscribeBuddyList()
{
  return SubscribeBuddyList(false);
}


bool OpalPresentity::SendCommand(OpalPresentityCommand * cmd)
{
  if (cmd == NULL)
    return false;

  cmd->Process(*this);
  delete cmd;
  return true;
}


OpalPresentityCommand * OpalPresentity::InternalCreateCommand(const char * cmdName)
{
  PDefaultPFactoryKey partialKey(cmdName);
  const char * className;

  for (unsigned ancestor = 0; *(className = GetClass(ancestor)) != '\0'; ++ancestor) {
    OpalPresentityCommand * cmd = PFactory<OpalPresentityCommand>::CreateInstance(className+partialKey);
    if (cmd != NULL)
      return cmd;
  }

  PAssertAlways(PUnimplementedFunction);
  return NULL;
}


PString OpalPresentity::GetID() const 
{ 
  PStringStream strm;
  strm << "id" << (unsigned)m_idNumber;
  return strm;
}


/////////////////////////////////////////////////////////////////////////////

OpalPresentityWithCommandThread::OpalPresentityWithCommandThread()
  : m_threadRunning(false)
  , m_thread(NULL)
{
}


OpalPresentityWithCommandThread::~OpalPresentityWithCommandThread()
{
  StopThread();

  while (!m_commandQueue.empty())
    delete m_commandQueue.front();
}


void OpalPresentityWithCommandThread::StartThread()
{
  if (m_threadRunning)
    return;

  // start handler thread
  m_threadRunning = true;
  m_thread = new PThreadObj<OpalPresentityWithCommandThread>(*this, &OpalPresentityWithCommandThread::ThreadMain);
}


void OpalPresentityWithCommandThread::StopThread()
{
  if (m_threadRunning && m_thread != NULL) {
    m_threadRunning = false;
    m_commandQueueSync.Signal();
    m_thread->WaitForTermination();
    delete m_thread;
    m_thread = NULL;
  }
}


bool OpalPresentityWithCommandThread::SendCommand(OpalPresentityCommand * cmd)
{
  if (!m_threadRunning) {
    delete cmd;
    return false;
  }

  {
    PWaitAndSignal m(m_commandQueueMutex);
    cmd->m_sequence = ++m_commandSequence;
    m_commandQueue.push(cmd);
  }

  m_commandQueueSync.Signal();

  return true;
}



void OpalPresentityWithCommandThread::ThreadMain()
{
  while (m_threadRunning) {
    OpalPresentityCommand * cmd = NULL;

    {
      PWaitAndSignal mutex(m_commandQueueMutex);
      if (!m_commandQueue.empty()) {
        cmd = m_commandQueue.front();
        m_commandQueue.pop();
      }
    }

    if (cmd != NULL) {
      cmd->Process(*this);
      delete cmd;
    }

    m_commandQueueSync.Wait(1000);
  }
}


/////////////////////////////////////////////////////////////////////////////
