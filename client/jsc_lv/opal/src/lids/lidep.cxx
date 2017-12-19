/*
 * lidep.cxx
 *
 * Line Interface Device EndPoint
 *
 * Open Phone Abstraction Library
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23295 $
 * $Author: rjongbloed $
 * $Date: 2009-08-28 06:38:49 +0000 (Fri, 28 Aug 2009) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "lidep.cxx"
#endif

#include <opal/buildopts.h>

#include <lids/lidep.h>

#include <opal/manager.h>
#include <opal/call.h>
#include <opal/patch.h>


#define new PNEW

static const char PrefixPSTN[] = "pstn";
static const char PrefixPOTS[] = "pots";


/////////////////////////////////////////////////////////////////////////////

OpalLineEndPoint::OpalLineEndPoint(OpalManager & mgr)
  : OpalEndPoint(mgr, PrefixPOTS, CanTerminateCall),
    defaultLine("*")
{
  PTRACE(4, "LID EP\tOpalLineEndPoint created");
  manager.AttachEndPoint(this, PrefixPSTN);
  monitorThread = PThread::Create(PCREATE_NOTIFIER(MonitorLines), "Line Monitor");
}


OpalLineEndPoint::~OpalLineEndPoint()
{
  if(NULL != monitorThread)
  {
     PTRACE(4, "LID EP\tAwaiting monitor thread termination " << GetPrefixName());
     exitFlag.Signal();
     monitorThread->WaitForTermination();
     delete monitorThread;
     monitorThread = NULL;

     /* remove all lines which has been added with AddLine or AddLinesFromDevice
        RemoveAllLines can be invoked only after the monitorThread has been destroyed,
        indeed, the monitor thread may called some function such as vpb_get_event_ch_async which may
        throw an exception if the line handle is no longer valid
     */
     RemoveAllLines();
  }
  PTRACE(4, "LID EP\tOpalLineEndPoint " << GetPrefixName() << " destroyed");
}


PSafePtr<OpalConnection> OpalLineEndPoint::MakeConnection(OpalCall & call,
                                                     const PString & remoteParty,
                                                              void * userData,
                                                        unsigned int /*options*/,
                                     OpalConnection::StringOptions * /*stringOptions*/)
{
  PTRACE(3, "LID EP\tMakeConnection to " << remoteParty);  

  // Then see if there is a specific line mentioned in the prefix, e.g 123456@vpb:1/2
  PINDEX prefixLength = GetPrefixName().GetLength();
  bool terminating = (remoteParty.Left(prefixLength) *= PrefixPOTS);

  PString number, lineName;
  PINDEX at = remoteParty.Find('@');
  if (at != P_MAX_INDEX) {
    number = remoteParty(prefixLength+1, at-1);
    lineName = remoteParty.Mid(at + 1);
  }
  else {
    if (terminating)
      lineName = remoteParty.Mid(prefixLength + 1);
    else
      number = remoteParty.Mid(prefixLength + 1);
  }

  if (lineName.IsEmpty())
    lineName = defaultLine;
  
  PTRACE(3,"LID EP\tMakeConnection line = \"" << lineName << "\", number = \"" << number << '"');
  
  // Locate a line
  OpalLine * line = GetLine(lineName, true, terminating);
  if (line == NULL && lineName != defaultLine) {
    PTRACE(1,"LID EP\tMakeConnection cannot find the line \"" << lineName << '"');
    line = GetLine(defaultLine, true, terminating);
  }  
  if (line == NULL){
    PTRACE(1,"LID EP\tMakeConnection cannot find the default line " << defaultLine);
    return NULL;

  }

  return AddConnection(CreateConnection(call, *line, userData, number));
}


OpalMediaFormatList OpalLineEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList mediaFormats = manager.GetCommonMediaFormats(false, false);

  PWaitAndSignal mutex(linesMutex);

  for (OpalLineList::const_iterator line = lines.begin(); line != lines.end(); ++line)
    mediaFormats += line->GetDevice().GetMediaFormats();

  return mediaFormats;
}


OpalLineConnection * OpalLineEndPoint::CreateConnection(OpalCall & call,
                                                       OpalLine & line,
                                                       void * /*userData*/,
                                                       const PString & number)
{
  PTRACE(3, "LID EP\tCreateConnection call = " << call << " line = \"" << line << "\", number = \"" << number << '"');
  return new OpalLineConnection(call, *this, line, number);
}


static bool InitialiseLine(OpalLine * line)
{
  PTRACE(3, "LID EP\tInitialiseLine " << *line);
  line->Ring(0, NULL);
  line->StopTone();
  line->StopReading();
  line->StopWriting();

  if (!line->DisableAudio())
    return false;

  for (unsigned lnum = 0; lnum < line->GetDevice().GetLineCount(); lnum++) {
    if (lnum != line->GetLineNumber())
      line->GetDevice().SetLineToLineDirect(lnum, line->GetLineNumber(), false);
  }

  return true;
}


PBoolean OpalLineEndPoint::AddLine(OpalLine * line)
{
  if (PAssertNULL(line) == NULL)
    return false;

  if (!line->GetDevice().IsOpen())
    return false;

  if (!InitialiseLine(line))
    return false;

  linesMutex.Wait();
  lines.Append(line);
  linesMutex.Signal();

  return true;
}


void OpalLineEndPoint::RemoveLine(OpalLine * line)
{
  linesMutex.Wait();
  lines.Remove(line);
  linesMutex.Signal();
}


void OpalLineEndPoint::RemoveLine(const PString & token)
{
  linesMutex.Wait();
  OpalLineList::iterator line = lines.begin();
  while (line != lines.end()) {
    if (line->GetToken() *= token)
      lines.erase(line++);
    else
      ++line;
  }
  linesMutex.Signal();
}


void OpalLineEndPoint::RemoveAllLines()
{
  linesMutex.Wait();
  lines.RemoveAll();
  devices.RemoveAll();
  linesMutex.Signal();
}   
    
PBoolean OpalLineEndPoint::AddLinesFromDevice(OpalLineInterfaceDevice & device)
{
  if (!device.IsOpen()){
    PTRACE(1, "LID EP\tAddLinesFromDevice device " << device.GetDeviceName() << "is not opened");
    return false;
  }  

  unsigned lineCount = device.GetLineCount();
  PTRACE(3, "LID EP\tAddLinesFromDevice device " << device.GetDeviceName() << " has " << lineCount << " lines");
  if (lineCount == 0)
    return false;

  bool atLeastOne = false;

  for (unsigned line = 0; line < lineCount; line++) {
    OpalLine * newLine = new OpalLine(device, line);
    if (InitialiseLine(newLine)) {
      atLeastOne = true;
      linesMutex.Wait();
      lines.Append(newLine);
      linesMutex.Signal();
      PTRACE(3, "LID EP\tAdded line  " << line << ", " << (device.IsLineTerminal(line) ? "terminal" : "network"));
    }
    else {
      delete newLine;
      PTRACE(3, "LID EP\tCould not add line  " << line << ", " << (device.IsLineTerminal(line) ? "terminal" : "network"));
    }
  }

  return atLeastOne;
}


void OpalLineEndPoint::RemoveLinesFromDevice(OpalLineInterfaceDevice & device)
{
  linesMutex.Wait();
  OpalLineList::iterator line = lines.begin();
  while (line != lines.end()) {
    if (line->GetToken().Find(device.GetDeviceName()) == 0)
      lines.erase(line++);
    else
      ++line;
  }
  linesMutex.Signal();
}


PBoolean OpalLineEndPoint::AddDeviceNames(const PStringArray & descriptors)
{
  PBoolean ok = false;

  for (PINDEX i = 0; i < descriptors.GetSize(); i++) {
    if (AddDeviceName(descriptors[i]))
      ok = true;
  }

  return ok;
}


const OpalLineInterfaceDevice * OpalLineEndPoint::GetDeviceByName(const PString & descriptor)
{
  PString deviceType, deviceName;

  PINDEX colon = descriptor.Find(':');
  if (colon != P_MAX_INDEX) {
    deviceType = descriptor.Left(colon).Trim();
    deviceName = descriptor.Mid(colon+1).Trim();
  }

  if (deviceType.IsEmpty() || deviceName.IsEmpty()) {
    PTRACE(1, "LID EP\tInvalid device description \"" << descriptor << '"');
    return NULL;
  }

  // Make sure not already there.
  PWaitAndSignal mutex(linesMutex);
  for (OpalLIDList::iterator iterDev = devices.begin(); iterDev != devices.end(); ++iterDev) {
    if (iterDev->GetDeviceType() == deviceType && iterDev->GetDeviceName() == deviceName) {
      PTRACE(3, "LID EP\tDevice " << deviceType << ':' << deviceName << " is loaded.");
      return &*iterDev;
    }
  }

  return NULL;
}


PBoolean OpalLineEndPoint::AddDeviceName(const PString & descriptor)
{
  if (GetDeviceByName(descriptor) != NULL)
    return true;

  // Not there so add it.
  OpalLineInterfaceDevice * device = OpalLineInterfaceDevice::CreateAndOpen(descriptor);
  if (device != NULL)
    return AddDevice(device);

  PTRACE(1, "LID EP\tDevice " << descriptor << " could not be created or opened.");
  return false;
}


PBoolean OpalLineEndPoint::AddDevice(OpalLineInterfaceDevice * device)
{
  if (PAssertNULL(device) == NULL)
    return false;

  linesMutex.Wait();
  devices.Append(device);
  linesMutex.Signal();
  return AddLinesFromDevice(*device);
}


void OpalLineEndPoint::RemoveDevice(OpalLineInterfaceDevice * device)
{
  if (PAssertNULL(device) == NULL)
    return;

  RemoveLinesFromDevice(*device);
  linesMutex.Wait();
  devices.Remove(device);
  linesMutex.Signal();
}


OpalLine * OpalLineEndPoint::GetLine(const PString & lineName, bool enableAudio, bool terminating)
{
  PWaitAndSignal mutex(linesMutex);

  PTRACE(4, "LID EP\tGetLine " << lineName << ", enableAudio=" << enableAudio << ", terminating=" << terminating);
  for (OpalLineList::iterator line = lines.begin(); line != lines.end(); ++line) {
    PString lineToken = line->GetToken();
    if (lineName != defaultLine && lineToken != lineName)
      PTRACE(4, "LID EP\tNo match to line name=\"" << lineToken << "\", default=" << defaultLine);
    else if (line->IsTerminal() != terminating)
      PTRACE(4, "LID EP\tNo match to line name=\"" << lineToken << "\", terminating=" << line->IsTerminal());
    else if (!line->IsPresent())
      PTRACE(4, "LID EP\tNo match to line name=\"" << lineToken << "\", not present");
    else if (enableAudio && (line->IsAudioEnabled() || !line->EnableAudio()))
      PTRACE(4, "LID EP\tNo match to line name=\"" << lineToken << "\", enableAudio=" << line->IsAudioEnabled());
    else {
      PTRACE(3, "LID EP\tGetLine found the line \"" << lineName << '"');
      return &*line;
    }  
  }

  PTRACE(3, "LID EP\tGetLine could not find/enable \"" << lineName << '"');
  return NULL;
}


void OpalLineEndPoint::SetDefaultLine(const PString & lineName)
{
  PTRACE(3, "LID EP\tSetDefaultLine " << lineName);
  linesMutex.Wait();
  defaultLine = lineName;
  linesMutex.Signal();
}


void OpalLineEndPoint::MonitorLines(PThread &, INT)
{
  PTRACE(4, "LID EP\tMonitor thread started for " << GetPrefixName());

  while (!exitFlag.Wait(100)) {
    linesMutex.Wait();
    for (OpalLineList::iterator line = lines.begin(); line != lines.end(); ++line)
      MonitorLine(*line);
    linesMutex.Signal();
  }

  PTRACE(4, "LID EP\tMonitor thread stopped for " << GetPrefixName());
}


void OpalLineEndPoint::MonitorLine(OpalLine & line)
{
  PSafePtr<OpalLineConnection> connection = GetLIDConnectionWithLock(line.GetToken(), PSafeReference);
  if (connection != NULL) {
    // Are still in a call, pass hook state it to the connection object for handling
    connection->Monitor();
    return;
  }

  if (line.IsAudioEnabled()) {
    // Are still in previous call, wait for them to hang up
    if (line.IsDisconnected()) {
      PTRACE(3, "LID EP\tLine " << line << " has disconnected.");
      line.StopTone();
      line.DisableAudio();
    }
    return;
  }

  if (line.IsTerminal()) {
    // Not off hook, so nothing happening, just return
    if (!line.IsOffHook())
      return;
    PTRACE(3, "LID EP\tLine " << line << " has gone off hook.");
  }
  else {
    // Not ringing, so nothing happening, just return
    if (!line.IsRinging())
      return;
    PTRACE(3, "LID EP\tLine " << line << " is ringing.");
  }

  // See if we can get exlusive use of the line. With something like a LineJACK
  // enabling audio on the PSTN line the POTS line will no longer be enable-able
  // so this will fail and the ringing will be ignored
  if (!line.EnableAudio())
    return;

  // Get new instance of a call, abort if none created
  OpalCall * call = manager.InternalCreateCall();
  if (call == NULL) {
    line.DisableAudio();
    return;
  }

  // Have incoming ring, create a new LID connection and let it handle it
  connection = CreateConnection(*call, line, NULL, "Unknown");
  if (AddConnection(connection))
    connection->StartIncoming();
}


/////////////////////////////////////////////////////////////////////////////

OpalLineConnection::OpalLineConnection(OpalCall & call,
                                       OpalLineEndPoint & ep,
                                       OpalLine & ln,
                                       const PString & number)
  : OpalConnection(call, ep, ln.GetToken())
  , endpoint(ep)
  , line(ln)
  , wasOffHook(false)
  , minimumRingCount(2)
  , m_promptTone(OpalLineInterfaceDevice::DialTone)
  , handlerThread(NULL)
{
  localPartyName = ln.GetToken();
  remotePartyNumber = number.Right(number.FindSpan("0123456789*#,"));
  remotePartyName = number;
  if (remotePartyName.IsEmpty())
    remotePartyName = "Unknown";
  else
    remotePartyAddress = remotePartyName + '@';
  remotePartyAddress += GetToken();

  silenceDetector = new OpalLineSilenceDetector(line, (endpoint.GetManager().GetSilenceDetectParams()));

  PTRACE(3, "LID Con\tConnection " << callToken << " created to " << (number.IsEmpty() ? "local" : number));
  
}


void OpalLineConnection::OnReleased()
{
  PTRACE(3, "LID Con\tOnReleased " << *this);

  if (handlerThread != NULL && PThread::Current() != handlerThread) {
    PTRACE(4, "LID Con\tAwaiting handler thread termination " << *this);
    // Stop the signalling handler thread
    SetUserInput(PString()); // Break out of ReadUserInput
    handlerThread->WaitForTermination();
    delete handlerThread;
    handlerThread = NULL;
  }

  if (line.IsTerminal()) {
    if (line.IsOffHook()) {
      if (line.PlayTone(OpalLineInterfaceDevice::ClearTone))
        PTRACE(3, "LID Con\tPlaying clear tone until handset onhook");
      else
        PTRACE(2, "LID Con\tCould not play clear tone!");
    }
    line.Ring(0, NULL);
  }
  else
    line.SetOnHook();

  SetPhase(ReleasedPhase);

  OpalConnection::OnReleased();
}


PString OpalLineConnection::GetDestinationAddress()
{
  return line.IsTerminal() ? ReadUserInput() : GetLocalPartyName();
}


PBoolean OpalLineConnection::SetAlerting(const PString & /*calleeName*/, PBoolean /*withMedia*/)
{
  PTRACE(3, "LID Con\tSetAlerting " << *this);

  if (GetPhase() >= AlertingPhase) 
    return false;

  // switch phase 
  SetPhase(AlertingPhase);
  alertingTime = PTime();

  if (line.IsTerminal() && GetMediaStream(OpalMediaType::Audio(), false) == NULL) {
    // Start ringing if we don't have an audio media stream
    if (line.PlayTone(OpalLineInterfaceDevice::RingTone))
      PTRACE(3, "LID Con\tPlaying ring tone");
    else
      PTRACE(2, "LID Con\tCould not play ring tone");
  }

  return true;
}


PBoolean OpalLineConnection::SetConnected()
{
  PTRACE(3, "LID Con\tSetConnected " << *this);

  if (!line.StopTone()) {
    PTRACE(1, "LID Con\tCould not stop tone on " << *this);
    return false;
  }

  if (line.IsTerminal()) {
    if (!line.SetConnected()) {
      PTRACE(1, "LID Con\tCould not set line to connected mode on " << *this);
      return false;
    }
  }
  else {
    if (!line.SetOffHook()) {
      PTRACE(1, "LID Con\tCould not set line off hook on " << *this);
      return false;
    }
    PTRACE(4, "LID Con\tAnswered call - gone off hook.");
    wasOffHook = true;
  }

  AutoStartMediaStreams();
  return OpalConnection::SetConnected();
}


OpalMediaFormatList OpalLineConnection::GetMediaFormats() const
{
  OpalMediaFormatList formats = endpoint.GetManager().GetCommonMediaFormats(false, false);
  formats += line.GetDevice().GetMediaFormats();
  return formats;
}


OpalMediaStream * OpalLineConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                        unsigned sessionID,
                                                        PBoolean isSource)
{
  OpalMediaFormatList formats = line.GetDevice().GetMediaFormats();
  if (formats.FindFormat(mediaFormat) == formats.end())
    return OpalConnection::CreateMediaStream(mediaFormat, sessionID, isSource);

  return new OpalLineMediaStream(*this, mediaFormat, sessionID, isSource, line);
}


PBoolean OpalLineConnection::OnOpenMediaStream(OpalMediaStream & mediaStream)
{
  if (!OpalConnection::OnOpenMediaStream(mediaStream))
    return false;

  if (mediaStream.IsSource()) {
    OpalMediaPatch * patch = mediaStream.GetPatch();
    if (patch != NULL)
      patch->AddFilter(silenceDetector->GetReceiveHandler(), line.GetReadFormat());
  }

  line.StopTone(); // In case a RoutingTone or RingTone is going
  return true;
}


PBoolean OpalLineConnection::SetAudioVolume(PBoolean source, unsigned percentage)
{
  PSafePtr<OpalLineMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalLineMediaStream>(GetMediaStream(OpalMediaType::Audio(), source));
  if (stream == NULL)
    return false;

  OpalLine & line = stream->GetLine();
  return source ? line.SetRecordVolume(percentage) : line.SetPlayVolume(percentage);
}


unsigned OpalLineConnection::GetAudioSignalLevel(PBoolean source)
{
  PSafePtr<OpalLineMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalLineMediaStream>(GetMediaStream(OpalMediaType::Audio(), source));
  if (stream == NULL)
    return UINT_MAX;

  OpalLine & line = stream->GetLine();
  return line.GetAverageSignalLevel(!source);
}


PBoolean OpalLineConnection::SendUserInputString(const PString & value)
{
  return line.PlayDTMF(value);
}


PBoolean OpalLineConnection::SendUserInputTone(char tone, int duration)
{
  if (duration <= 0)
    return line.PlayDTMF(&tone);
  else
    return line.PlayDTMF(&tone, duration);
}


PBoolean OpalLineConnection::PromptUserInput(PBoolean play)
{
  PTRACE(3, "LID Con\tConnection " << callToken << " dial tone " << (play ? "started" : "stopped"));

  if (play) {
    if (line.PlayTone(m_promptTone)) {
      PTRACE(3, "LID Con\tPlaying dial tone");
      return true;
    }
    PTRACE(2, "LID Con\tCould not dial ring tone");
    return false;
  }

  line.StopTone();
  return true;
}


void OpalLineConnection::StartIncoming()
{
  if (handlerThread == NULL)
    handlerThread = PThread::Create(PCREATE_NOTIFIER(HandleIncoming), "Line Connection");
}


void OpalLineConnection::Monitor()
{
  bool offHook = !line.IsDisconnected();
  if (wasOffHook != offHook) {
    PSafeLockReadWrite mutex(*this);

    wasOffHook = offHook;
    PTRACE(3, "LID Con\tConnection " << callToken << " " << (offHook ? "off" : "on") << " hook: phase=" << GetPhase());

    if (!offHook) {
      Release(EndedByRemoteUser);
      return;
    }

    if (IsOriginating() && line.IsTerminal()) {
      // Ok, they went off hook, stop ringing
      line.Ring(0, NULL);

      if (GetPhase() != AlertingPhase)
        StartIncoming(); // We are A-party
      else {
        // If we are in alerting state then we are B-Party
        OnConnectedInternal();
        AutoStartMediaStreams();
      }
    }
  }

  // If we are off hook, we continually suck out DTMF tones plus various other tones and signals and pass them up
  if (offHook) {
    switch (line.IsToneDetected()) {
      case OpalLineInterfaceDevice::CNGTone :
        OnUserInputTone('X', 100);
        break;

      case OpalLineInterfaceDevice::CEDTone :
        OnUserInputTone('Y', 100);
        break;

      default :
        break;
    }

    if (line.HasHookFlash())
      OnUserInputTone('!', 100);

    char tone;
    while ((tone = line.ReadDTMF()) != '\0')
      OnUserInputTone(tone, 180);
  }
  else {
    // Check for incoming PSTN ring stopping
    if (GetPhase() == AlertingPhase && !line.IsTerminal() && line.GetRingCount() == 0)
      Release(EndedByCallerAbort);
  }
}


void OpalLineConnection::HandleIncoming(PThread &, INT)
{
  PTRACE(3, "LID Con\tHandling incoming call on " << *this);

  SetPhase(SetUpPhase);

  if (line.IsTerminal())
    wasOffHook = true;
  else {
    PTRACE(4, "LID Con\tCounting rings.");
    // Count incoming rings
    unsigned count;
    do {
      count = line.GetRingCount();
      if (count == 0) {
        PTRACE(3, "LID Con\tIncoming PSTN call stopped.");
        Release(EndedByCallerAbort);
        return;
      }
      PThread::Sleep(100);
      if (GetPhase() >= ReleasingPhase)
        return;
    } while (count < minimumRingCount); // Wait till we have CLID

    // Get caller ID
    PString callerId;
    if (line.GetCallerID(callerId, true)) {
      PStringArray words = callerId.Tokenise('\t', true);
      if (words.GetSize() < 3) {
        PTRACE(2, "LID Con\tMalformed caller ID \"" << callerId << '"');
      }
      else {
        PTRACE(3, "LID Con\tDetected Caller ID \"" << callerId << '"');
        remotePartyNumber = words[0].Trim();
        remotePartyName = words[2].Trim();
        if (remotePartyName.IsEmpty())
          remotePartyName = remotePartyNumber;
      }
    }
    else {
      PTRACE(3, "LID Con\tNo caller ID available.");
    }
    if (remotePartyName.IsEmpty())
      remotePartyName = "Unknown";

    // switch phase 
    SetPhase(AlertingPhase);
    alertingTime = PTime();
  }

  if (!OnIncomingConnection(0, NULL)) {
    PTRACE(3, "LID\tWaiting for RING to stop on " << *this);
    while (line.GetRingCount() > 0) {
      if (GetPhase() >= ReleasingPhase)
        return;
      PThread::Sleep(100);
    }
    Release(EndedByCallerAbort);
    return;
  }

  PTRACE(3, "LID Con\tRouted to \"" << ownerCall.GetPartyB() << "\", "
         << (IsOriginating() ? "outgo" : "incom") << "ing connection " << *this);
  if (ownerCall.OnSetUp(*this) && line.IsTerminal() && GetPhase() < AlertingPhase)
    line.PlayTone(OpalLineInterfaceDevice::RoutingTone);
}


PString OpalLineConnection::GetPrefixName() const
{
  return line.IsTerminal() ? PrefixPOTS : PrefixPSTN;
}


PBoolean OpalLineConnection::SetUpConnection()
{
  PTRACE(3, "LID Con\tSetUpConnection call on " << *this << " to \"" << remotePartyNumber << '"');

  SetPhase(SetUpPhase);
  originating = true;

  if (line.IsTerminal()) {
    PSafePtr<OpalConnection> partyA = ownerCall.GetConnection(0);
    if (partyA != this) {
      // Are B-Party, so set caller ID and move to alerting state
      line.SetCallerID(partyA->GetRemotePartyNumber() + "\t\t" + partyA->GetRemotePartyName());
      SetPhase(AlertingPhase);
      OnAlerting();
    }
    return line.Ring(1, NULL);
  }

  if (remotePartyNumber.IsEmpty()) {
    if (!line.SetOffHook()) {
      PTRACE(1, "LID Con\tCould not go off hook");
      return false;
    }

    PTRACE(3, "LID Con\tNo remote party indicated, going off hook without dialing.");
    OnConnectedInternal();
    AutoStartMediaStreams();
    return true;
  }

  switch (line.DialOut(remotePartyNumber, m_dialParams)) {
    case OpalLineInterfaceDevice::RingTone :
      break;

    case OpalLineInterfaceDevice::DialTone :
      PTRACE(3, "LID Con\tNo dial tone on " << line);
      return false;

    case OpalLineInterfaceDevice::BusyTone :
      PTRACE(3, "LID Con\tBusy tone on " << line);
      Release(OpalConnection::EndedByRemoteBusy);
      return false;

    default :
      PTRACE(1, "LID Con\tError dialling " << remotePartyNumber << " on " << line);
      Release(OpalConnection::EndedByConnectFail);
      return false;
  }

  PTRACE(3, "LID Con\tGot ring back on " << line);
  // Start media before the OnAlerting to get progress tones (e.g. SIP 183 response)
  AutoStartMediaStreams();
  SetPhase(AlertingPhase);
  OnAlerting();

  // Wait for connection
  if (m_dialParams.m_progressTimeout == 0) {
    OnConnectedInternal();
    return true;
  }

  PTRACE(3, "LID Con\tWaiting " << m_dialParams.m_progressTimeout << "ms for connection on line " << line);
  PTimer timeout(m_dialParams.m_progressTimeout);
  while (timeout.IsRunning()) {
    if (GetPhase() != AlertingPhase) // Check for external connection release
      return false;

    if (line.IsConnected()) {
      OnConnectedInternal();
      return true;
    }

    if (line.IsToneDetected() == OpalLineInterfaceDevice::BusyTone) {
      Release(OpalConnection::EndedByRemoteBusy);
      return false;
    }

    PThread::Sleep(100);
  }

  PTRACE(2, "LID Con\tConnection not detected ("
         << (m_dialParams.m_requireTones ? "required" : "optional")
         << ") on line " << line);

  if (m_dialParams.m_requireTones) {
    Release(OpalConnection::EndedByRemoteBusy);
    return false;
  }

  // Connect anyway
  OnConnectedInternal();
  return true;
}


///////////////////////////////////////////////////////////////////////////////

OpalLineMediaStream::OpalLineMediaStream(OpalLineConnection & conn, 
                                  const OpalMediaFormat & mediaFormat,
                                                 unsigned sessionID,
                                                     PBoolean isSource,
                                               OpalLine & ln)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
  , line(ln)
  , notUsingRTP(!ln.GetDevice().UsesRTP())
  , useDeblocking(false)
  , missedCount(0)
  , lastFrameWasSignal(true)
  , directLineNumber(UINT_MAX)
{
  lastSID[0] = 2;
}

OpalLineMediaStream::~OpalLineMediaStream()
{
  Close();
}


PBoolean OpalLineMediaStream::Open()
{
  if (isOpen)
    return true;

  if (IsSource()) {
    if (!line.SetReadFormat(mediaFormat))
      return false;
  }
  else {
    if (!line.SetWriteFormat(mediaFormat))
      return false;
  }

  SetDataSize(GetDataSize(), GetDataSize()/2);
  PTRACE(3, "LineMedia\tStream opened for " << mediaFormat << ", using "
         << (notUsingRTP ? (useDeblocking ? "reblocked audio" : "audio frames") : "direct RTP"));

  return OpalMediaStream::Open();
}


PBoolean OpalLineMediaStream::Close()
{
  if (directLineNumber != UINT_MAX)
    line.GetDevice().SetLineToLineDirect(line.GetLineNumber(), directLineNumber, false);
  else if (IsSource())
    line.StopReading();
  else
    line.StopWriting();

  return OpalMediaStream::Close();
}


PBoolean OpalLineMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (notUsingRTP)
    return OpalMediaStream::ReadPacket(packet);

  if (!packet.SetMinSize(RTP_DataFrame::MinHeaderSize+defaultDataSize))
    return false;

  PINDEX count = packet.GetSize();
  if (!line.ReadFrame(packet.GetPointer(), count))
    return false;

  packet.SetPayloadSize(count-packet.GetHeaderSize());
  return true;
}


PBoolean OpalLineMediaStream::WritePacket(RTP_DataFrame & packet)
{
  if (notUsingRTP)
    return OpalMediaStream::WritePacket(packet);

  PINDEX written = 0;
  return line.WriteFrame(packet.GetPointer(), packet.GetHeaderSize()+packet.GetPayloadSize(), written);
}


PBoolean OpalLineMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  PAssert(notUsingRTP, PLogicError);

  length = 0;

  if (IsSink()) {
    PTRACE(1, "LineMedia\tTried to read from sink media stream");
    return false;
  }

  if (useDeblocking) {
    line.SetReadFrameSize(size);
    if (line.ReadBlock(buffer, size)) {
      length = size;
      return true;
    }
  }
  else {
    if (line.ReadFrame(buffer, length)) {
      // In the case of G.723.1 remember the last SID frame we sent and send
      // it again if the hardware sends us a DTX frame.
      if (mediaFormat.GetPayloadType() == RTP_DataFrame::G7231) {
        switch (length) {
          case 1 : // DTX frame
            memcpy(buffer, lastSID, 4);
            length = 4;
            lastFrameWasSignal = false;
            break;
          case 4 :
            if ((*(BYTE *)buffer&3) == 2)
              memcpy(lastSID, buffer, 4);
            lastFrameWasSignal = false;
            break;
          default :
            lastFrameWasSignal = true;
        }
      }
      return true;
    }
  }

  PTRACE_IF(1, line.GetDevice().GetErrorNumber() != 0,
            "LineMedia\tDevice read frame error: " << line.GetDevice().GetErrorText());

  return false;
}


PBoolean OpalLineMediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  PAssert(notUsingRTP, PLogicError);

  written = 0;

  if (IsSource()) {
    PTRACE(1, "LineMedia\tTried to write to source media stream");
    return false;
  }

  // Check for writing silence
  PBYTEArray silenceBuffer;
  if (length != 0)
    missedCount = 0;
  else {
    switch (mediaFormat.GetPayloadType()) {
      case RTP_DataFrame::G7231 :
        if (missedCount++ < 4) {
          static const BYTE g723_erasure_frame[24] = { 0xff, 0xff, 0xff, 0xff };
          buffer = g723_erasure_frame;
          length = 24;
        }
        else {
          static const BYTE g723_cng_frame[4] = { 3 };
          buffer = g723_cng_frame;
          length = 1;
        }
        break;

      case RTP_DataFrame::PCMU :
      case RTP_DataFrame::PCMA :
        buffer = silenceBuffer.GetPointer(line.GetWriteFrameSize());
        length = silenceBuffer.GetSize();
        memset((void *)buffer, 0xff, length);
        break;

      case RTP_DataFrame::G729 :
        if (mediaFormat.GetName().Find('B') != P_MAX_INDEX) {
          static const BYTE g729_sid_frame[2] = { 1 };
          buffer = g729_sid_frame;
          length = 2;
          break;
        }
        // Else fall into default case

      default :
        buffer = silenceBuffer.GetPointer(line.GetWriteFrameSize()); // Fills with zeros
        length = silenceBuffer.GetSize();
        break;
    }
  }

  if (useDeblocking) {
    line.SetWriteFrameSize(length);
    if (line.WriteBlock(buffer, length)) {
      written = length;
      return true;
    }
  }
  else {
    if (line.WriteFrame(buffer, length, written))
      return true;
  }

  PTRACE_IF(1, line.GetDevice().GetErrorNumber() != 0,
            "LineMedia\tLID write frame error: " << line.GetDevice().GetErrorText());

  return false;
}


PBoolean OpalLineMediaStream::SetDataSize(PINDEX dataSize, PINDEX frameTime)
{
  if (notUsingRTP) {
    if (IsSource())
      useDeblocking = !line.SetReadFrameSize(dataSize) || line.GetReadFrameSize() != dataSize;
    else
      useDeblocking = !line.SetWriteFrameSize(dataSize) || line.GetWriteFrameSize() != dataSize;

    PTRACE(3, "LineMedia\tStream frame size: rd="
           << line.GetReadFrameSize() << " wr="
           << line.GetWriteFrameSize() << ", "
           << (useDeblocking ? "needs" : "no") << " reblocking.");
  }

  // Yes we set BOTH to frameSize, and ignore the dataSize parameter
  return OpalMediaStream::SetDataSize(dataSize, frameTime);
}


PBoolean OpalLineMediaStream::IsSynchronous() const
{
  return true;
}


PBoolean OpalLineMediaStream::RequiresPatchThread(OpalMediaStream * stream) const
{
  OpalLineMediaStream * lineStream = dynamic_cast<OpalLineMediaStream *>(stream);
  if (lineStream != NULL && &line.GetDevice() == &lineStream->line.GetDevice()) {
    if (line.GetDevice().SetLineToLineDirect(line.GetLineNumber(), lineStream->line.GetLineNumber(), true)) {
      PTRACE(3, "LineMedia\tDirect line connection between "
             << line.GetLineNumber() << " and " << lineStream->line.GetLineNumber()
             << " on device " << line.GetDevice());
      const_cast<OpalLineMediaStream *>(this)->directLineNumber = lineStream->line.GetLineNumber();
      lineStream->directLineNumber = line.GetLineNumber();
      return false; // Do not start threads
    }
    PTRACE(2, "LineMedia\tCould not do direct line connection between "
           << line.GetLineNumber() << " and " << lineStream->line.GetLineNumber()
           << " on device " << line.GetDevice());
  }
  return OpalMediaStream::RequiresPatchThread(stream);
}


/////////////////////////////////////////////////////////////////////////////

OpalLineSilenceDetector::OpalLineSilenceDetector(OpalLine & theLine, const Params & theParam)
  : OpalSilenceDetector(theParam)
  , line(theLine)
{
}


unsigned OpalLineSilenceDetector::GetAverageSignalLevel(const BYTE *, PINDEX)
{
  return line.GetAverageSignalLevel(true);
}


/////////////////////////////////////////////////////////////////////////////
