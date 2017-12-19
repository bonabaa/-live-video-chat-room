/*
 * main.cxx
 *
 * OPAL call generator
 *
 * Copyright (c) 2007 Equivalence Pty. Ltd.
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
 * The Original Code is CallGen.
 *
 * Contributor(s): Equivalence Pty. Ltd.
 *
 * $Revision: 21283 $
 * $Author: rjongbloed $
 * $Date: 2008-10-11 07:10:58 +0000 (Sat, 11 Oct 2008) $
 */

#include "precompile.h"
#include "main.h"
#include "version.h"

#include <opal/transcoders.h>


PCREATE_PROCESS(CallGen);


///////////////////////////////////////////////////////////////////////////////

CallGen::CallGen()
  : PProcess("Equivalence", "CallGen", MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER),
    console(PConsoleChannel::StandardInput)
{
  totalAttempts = 0;
  totalEstablished = 0;
}


void CallGen::Main()
{
  PArgList & args = GetArguments();
  args.Parse("a-access-token-oid:"
             "c-cdr:"
             "C-cycle."
             "D-disable:"
             "f-fast-disable."
             "g-gatekeeper:"
             "I-in-dir:"
	     "-h323-interface:"
	     "-sip-interface:"
             "l-listen."
             "m-max:"
             "n-no-gatekeeper."
             "O-out-msg:"
             "o-output:"
             "P-prefer:"
             "p-password:"
             "r-repeat:"
             "-require-gatekeeper."
             "T-h245tunneldisable."
             "t-trace."
             "-tmaxest:"
             "-tmincall:"
             "-tmaxcall:"
             "-tminwait:"
             "-tmaxwait:"
             "-tcp-base:"
             "-tcp-max:"
             "-udp-base:"
             "-udp-max:"
             "-rtp-base:"
             "-rtp-max:"
             "u-user:"
             , FALSE);
  
  if (args.GetCount() == 0 && !args.HasOption('l')) {
    cout << "Usage:\n"
            "  callgen [options] -l\n"
            "  callgen [options] destination [ destination ... ]\n"
            "where options:\n"
            "  -l                   Passive/listening mode.\n"
            "  -m --max num         Maximum number of simultaneous calls\n"
            "  -r --repeat num      Repeat calls n times\n"
            "  -C --cycle           Each simultaneous call cycles through destination list\n"
            "  -t --trace           Trace enable (use multiple times for more detail)\n"
            "  -o --output file     Specify filename for trace output [stdout]\n"
            "  --sip-interface addr   Specify IP address and port listen on for SIP [*:5060]\n"
            "  --h323-interface addr  Specify IP address and port listen on for H.323 [*:1720]\n"
            "  -g --gatekeeper host Specify gatekeeper host [auto-discover]\n"
            "  -n --no-gatekeeper   Disable gatekeeper discovery [false]\n"
            "  --require-gatekeeper Exit if gatekeeper discovery fails [false]\n"
            "  -u --user username   Specify local username [login name]\n"
            "  -p --password pwd    Specify gatekeeper H.235 password [none]\n"
            "  -P --prefer codec    Set codec preference (use multiple times) [none]\n"
            "  -D --disable codec   Disable codec (use multiple times) [none]\n"
            "  -f --fast-disable    Disable fast start\n"
            "  -T --h245tunneldisable  Disable H245 tunnelling.\n"
            "  -O --out-msg file    Specify PCM16 WAV file for outgoing message [ogm.wav]\n"
            "  -I --in-dir dir      Specify directory for incoming WAV files [disabled]\n"
            "  -c --cdr file        Specify Call Detail Record file [none]\n"
            "  --tcp-base port      Specific the base TCP port to use.\n"
            "  --tcp-max port       Specific the maximum TCP port to use.\n"
            "  --udp-base port      Specific the base UDP port to use.\n"
            "  --udp-max port       Specific the maximum UDP port to use.\n"
            "  --rtp-base port      Specific the base RTP/RTCP pair of UDP port to use.\n"
            "  --rtp-max port       Specific the maximum RTP/RTCP pair of UDP port to use.\n"
            "  --tmaxest  secs      Maximum time to wait for \"Established\" [0]\n"
            "  --tmincall secs      Minimum call duration in seconds [10]\n"
            "  --tmaxcall secs      Maximum call duration in seconds [60]\n"
            "  --tminwait secs      Minimum interval between calls in seconds [10]\n"
            "  --tmaxwait secs      Maximum interval between calls in seconds [30]\n"
            "\n"
            "Notes:\n"
            "  If --tmaxest is set a non-zero value then --tmincall is the time to leave\n"
            "  the call running once established. If zero (the default) then --tmincall\n"
            "  is the length of the call from initiation. The call may or may not be\n"
            "  \"answered\" within that time.\n"
            "\n";
    return;
  }
  
#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
		     PTrace::Blocks | PTrace::DateAndTime | PTrace::Thread | PTrace::FileAndLine);
#endif

#if OPAL_H323
  H323EndPoint * h323 = new H323EndPoint(manager);
#endif

  outgoingMessageFile = args.GetOptionString('O', "ogm.wav");
  if (outgoingMessageFile.IsEmpty())
    cout << "Not using outgoing message file." << endl;
  else if (PFile::Exists(outgoingMessageFile))
    cout << "Using outgoing message file: " << outgoingMessageFile << endl;
  else {
    cout << "Outgoing message file  \"" << outgoingMessageFile << "\" does not exist!" << endl;
    PTRACE(1, "CallGen\tOutgoing message file \"" << outgoingMessageFile << "\" does not exist");
    outgoingMessageFile = PString::Empty();
  }

  incomingAudioDirectory = args.GetOptionString('I');
  if (incomingAudioDirectory.IsEmpty())
    cout << "Not saving incoming audio data." << endl;
  else if (PDirectory::Exists(incomingAudioDirectory) ||
           PDirectory::Create(incomingAudioDirectory)) {
    incomingAudioDirectory = PDirectory(incomingAudioDirectory);
    cout << "Using incoming audio directory: " << incomingAudioDirectory << endl;
  }
  else {
    cout << "Could not create incoming audio directory \"" << incomingAudioDirectory << "\"!" << endl;
    PTRACE(1, "CallGen\tCould not create incoming audio directory \"" << incomingAudioDirectory << '"');
    incomingAudioDirectory = PString::Empty();
  }

#if OPAL_H323
  {
    PStringArray interfaces = args.GetOptionString("h323-interface").Lines();
    if (!h323->StartListeners(interfaces)) {
      cout << "Couldn't start any listeners on interfaces/ports:\n"
           << setfill('\n') << interfaces << setfill(' ') << endl;
      return;
    }
    cout << "H.323 listening on: " << setfill(',') << h323->GetListeners() << setfill(' ') << endl;
  }
#endif // OPAL_H323

#if OPAL_SIP
  {
    PStringArray interfaces = args.GetOptionString("sip-interface").Lines();
    SIPEndPoint * sip = new SIPEndPoint(manager);
    if (!sip->StartListeners(interfaces)) {
      cout << "Couldn't start any listeners on interfaces/ports:\n"
           << setfill('\n') << interfaces << setfill(' ') << endl;
      return;
    }
    cout << "SIP listening on: " << setfill(',') << sip->GetListeners() << setfill(' ') << endl;
  }
#endif // OPAL_SIP

#if OPAL_IVR
  OpalIVREndPoint * ivr = new OpalIVREndPoint(manager);
#if 0
  PStringStream vxml;
  vxml << "<?xml version=\"1.0\"?>"
          "<vxml version=\"1.0\">"
            "<form id=\"root\">"
              "<break msecs=\"1500\"/>"
              "<audio src=\"" + outgoingMessageFile + "\">"
                "This is the OPAL call generator";

  if (incomingAudioDirectory.IsEmpty())
    vxml <<     "."
              "</audio>"
              "<break msecs=\"1000\"/>";
  else
    vxml <<     ", please speak after the tone."
              "</audio>"
              "<record name=\"msg\" beep=\"true\" dtmfterm=\"true\" dest=\"" + incomingAudioDirectory + "msg%05u.wav\" maxtime=\"10s\"/>";

  vxml <<   "</form>"
          "</vxml>";

  ivr->SetDefaultVXML(vxml);
#else
  ivr->SetDefaultVXML("file://" + outgoingMessageFile);
#endif
#endif // OPAL_IVR


  unsigned simultaneous = args.GetOptionString('m').AsUnsigned();
  if (simultaneous == 0)
    simultaneous = 1;

  if (args.HasOption('c')) {
    if (cdrFile.Open(args.GetOptionString('c'), PFile::WriteOnly, PFile::Create)) {
      cdrFile.SetPosition(0, PFile::End);
      PTRACE(1, "CallGen\tSetting CDR to \"" << cdrFile.GetFilePath() << '"');
      cout << "Sending Call Detail Records to \"" << cdrFile.GetFilePath() << '"' << endl;
    }
    else {
      cout << "Could not open \"" << cdrFile.GetFilePath() << "\"!" << endl;
    }
  }

  if (args.HasOption("tcp-base"))
    manager.SetTCPPorts(args.GetOptionString("tcp-base").AsUnsigned(),
                        args.GetOptionString("tcp-max").AsUnsigned());
  if (args.HasOption("udp-base"))
    manager.SetUDPPorts(args.GetOptionString("udp-base").AsUnsigned(),
                        args.GetOptionString("udp-max").AsUnsigned());
  if (args.HasOption("rtp-base"))
    manager.SetRtpIpPorts(args.GetOptionString("rtp-base").AsUnsigned(),
                          args.GetOptionString("rtp-max").AsUnsigned());
  else {
    // Make sure that there are enough RTP ports for the simultaneous calls
    unsigned availablePorts = manager.GetRtpIpPortMax() - manager.GetRtpIpPortBase();
    if (availablePorts < simultaneous*4) {
      manager.SetRtpIpPorts(manager.GetRtpIpPortBase(), manager.GetRtpIpPortBase()+simultaneous*5);
      cout << "Increasing RTP ports available from " << availablePorts << " to " << simultaneous*5 << endl;
    }
  }

  if (args.HasOption('D'))
    manager.SetMediaFormatMask(args.GetOptionString('D').Lines());
  if (args.HasOption('P'))
    manager.SetMediaFormatOrder(args.GetOptionString('P').Lines());

  OpalMediaFormatList allMediaFormats = OpalTranscoder::GetPossibleFormats(ivr->GetMediaFormats()); // Get transcoders

#if PTRACING
  ostream & traceStream = PTrace::Begin(3, __FILE__, __LINE__);
  traceStream << "Simple\tAvailable media formats:\n";
  for (PINDEX i = 0; i < allMediaFormats.GetSize(); i++)
    allMediaFormats[i].PrintOptions(traceStream);
  traceStream << PTrace::End;
#endif

  for (PINDEX i = 0; i < allMediaFormats.GetSize(); i++) {
    if (!allMediaFormats[i].IsTransportable())
      allMediaFormats.RemoveAt(i--); // Don't show media formats that are not used over the wire
  }
  allMediaFormats.Remove(manager.GetMediaFormatMask());
  allMediaFormats.Reorder(manager.GetMediaFormatOrder());

  cout << "Codecs removed  : " << setfill(',') << manager.GetMediaFormatMask() << "\n"
          "Codec order     : " << setfill(',') << manager.GetMediaFormatOrder() << "\n"
          "Available codecs: " << allMediaFormats << setfill(' ') << endl;

  // set local username, is necessary
  if (args.HasOption('u')) {
    PStringArray aliases = args.GetOptionString('u').Lines();
    manager.SetDefaultUserName(aliases[0]);
#if OPAL_H323
    for (PINDEX i = 1; i < aliases.GetSize(); ++i)
      h323->AddAliasName(aliases[i]);
#endif // OPAL_H323
  }
  cout << "Local username: \"" << manager.GetDefaultUserName() << '"' << endl;
  
#if OPAL_H323
  if (args.HasOption('p')) {
    h323->SetGatekeeperPassword(args.GetOptionString('p'));
    cout << "Using H.235 security." << endl;
  }

  if (args.HasOption('a')) {
    h323->SetGkAccessTokenOID(args.GetOptionString('a'));
    cout << "Set Access Token OID to \"" << h323->GetGkAccessTokenOID() << '"' << endl;
  }

  // process gatekeeper registration options
  if (args.HasOption('g')) {
    PString gkAddr = args.GetOptionString('g');
    cout << "Registering with gatekeeper \"" << gkAddr << "\" ..." << flush;
    if (h323->UseGatekeeper(gkAddr))
      cout << "\nGatekeeper set to \"" << *h323->GetGatekeeper() << '"' << endl;
    else {
      cout << "\nError registering with gatekeeper at \"" << gkAddr << '"' << endl;
      return;
    }
  }
  else if (!args.HasOption('n')) {
    cout << "Searching for gatekeeper ..." << flush;
    if (h323->UseGatekeeper())
      cout << "\nGatekeeper found: " << *h323->GetGatekeeper() << endl;
    else {
      cout << "\nNo gatekeeper found." << endl;
      if (args.HasOption("require-gatekeeper")) 
        return;
    }
  }

  if (args.HasOption('f'))
    h323->DisableFastStart(TRUE);
  if (args.HasOption('T'))
    h323->DisableH245Tunneling(TRUE);
#endif // OPAL_H323
  
  if (args.HasOption('l')) {
    manager.AddRouteEntry(".*\t.* = ivr:"); // Everything goes to IVR
    cout << "Endpoint is listening for incoming calls, press ENTER to exit.\n";
    console.ReadChar();
    manager.ClearAllCalls();
  }
  else {
    CallParams params(*this);
    params.tmax_est .SetInterval(0, args.GetOptionString("tmaxest",  "0" ).AsUnsigned());
    params.tmin_call.SetInterval(0, args.GetOptionString("tmincall", "10").AsUnsigned());
    params.tmax_call.SetInterval(0, args.GetOptionString("tmaxcall", "60").AsUnsigned());
    params.tmin_wait.SetInterval(0, args.GetOptionString("tminwait", "10").AsUnsigned());
    params.tmax_wait.SetInterval(0, args.GetOptionString("tmaxwait", "30").AsUnsigned());

    if (params.tmin_call == 0 ||
        params.tmin_wait == 0 ||
        params.tmin_call > params.tmax_call ||
        params.tmin_wait > params.tmax_wait) {
      cerr << "Invalid times entered!\n";
      return;
    }

    cout << "Endpoint starting " << simultaneous << " simultaneous call";
    if (simultaneous > 1)
      cout << 's';
    cout << ' ';

    params.repeat = args.GetOptionString('r', "10").AsUnsigned();
    if (params.repeat != 0)
      cout << params.repeat;
    else
      cout << "infinite";
    cout << " time";
    if (params.repeat != 1)
      cout << 's';
    if (params.repeat != 0)
      cout << ", grand total of " << simultaneous*params.repeat << " calls";
    cout << ".\n\n"
            "Press ENTER at any time to quit.\n\n"
         << endl;

    // create some threads to do calls, but start them randomly
    for (unsigned idx = 0; idx < simultaneous; idx++) {
      if (args.HasOption('C'))
        threadList.Append(new CallThread(idx+1, args.GetParameters(), params));
      else {
        PINDEX arg = idx%args.GetCount();
        threadList.Append(new CallThread(idx+1, args.GetParameters(arg, arg), params));
      }
    }

    PThread::Create(PCREATE_NOTIFIER(Cancel), 0);

    for (;;) {
      threadEnded.Wait();
      PThread::Sleep(100);

      bool finished = TRUE;
      for (PINDEX i = 0; i < threadList.GetSize(); i++) {
        if (!threadList[i].IsTerminated()) {
          finished = FALSE;
          break;
        }
      }

      if (finished) {
        cout << "\nAll call sets completed." << endl;
        console.Close();
        break;
      }
    }
  }

  if (totalAttempts > 0)
    cout << "Total calls: " << totalAttempts
         << " attempted, " << totalEstablished << " established\n";
}


void CallGen::Cancel(PThread &, INT)
{
  PTRACE(3, "CallGen\tCancel thread started.");

  // wait for a keypress
  while (console.ReadChar() != '\n') {
    if (!console.IsOpen()) {
      PTRACE(3, "CallGen\tCancel thread ended.");
      return;
    }
  }

  PTRACE(2, "CallGen\tCancelling calls.");

  coutMutex.Wait();
  cout << "\nAborting all calls ..." << endl;
  coutMutex.Signal();
  
  // stop threads
  for (PINDEX i = 0; i < threadList.GetSize(); i++)
    threadList[i].Stop();

  // stop all calls
  CallGen::Current().manager.ClearAllCalls();

  PTRACE(1, "CallGen\tCancelled calls.");
}


///////////////////////////////////////////////////////////////////////////////

CallThread::CallThread(unsigned _index,
                       const PStringArray & _destinations,
                       const CallParams & _params)
  : PThread(1000, NoAutoDeleteThread, NormalPriority, psprintf("CallGen %u", _index)),
    destinations(_destinations),
    index(_index),
    params(_params)
{
  Resume();
}


static unsigned RandomRange(PRandom & rand,
                            const PTimeInterval & tmin,
                            const PTimeInterval & tmax)
{
  unsigned umax = tmax.GetInterval();
  unsigned umin = tmin.GetInterval();
  return rand.Generate() % (umax - umin + 1) + umin;
}


#define START_OUTPUT(index, token) \
{ \
  CallGen::Current().coutMutex.Wait(); \
  cout << setw(3) << index << ": " << setw(10) << token.Left(10) << ": "

#define END_OUTPUT() \
  cout << endl; \
  CallGen::Current().coutMutex.Signal(); \
}

static bool generateOutput = false;

#define OUTPUT(index, token, info) \
  if (generateOutput) { \
  START_OUTPUT(index, token) << info; END_OUTPUT() \
  } \


void CallThread::Main()
{
  PTRACE(2, "CallGen\tStarted thread " << index);

  CallGen & callgen = CallGen::Current();
  PRandom rand(PRandom::Number());

  PTimeInterval delay = RandomRange(rand, (index-1)*500, (index+1)*500);
  OUTPUT(index, PString::Empty(), "Initial delay of " << delay << " seconds");

  if (exit.Wait(delay)) {
    PTRACE(2, "CallGen\tAborted thread " << index);
    callgen.threadEnded.Signal();
    return;
  }

  // Loop "repeat" times for (repeat > 0), or loop forever for (repeat == 0)
  unsigned count = 1;
  do {
    PString destination = destinations[(index-1 + count-1)%destinations.GetSize()];

    // trigger a call
    PString token;
    PTRACE(1, "CallGen\tMaking call to " << destination);
    unsigned totalAttempts = ++callgen.totalAttempts;
    if (!callgen.manager.SetUpCall("ivr:*", destination, token, this))
      OUTPUT(index, token, "Failed to start call to " << destination)
    else {
      bool stopping = FALSE;

      delay = RandomRange(rand, params.tmin_call, params.tmax_call);

      START_OUTPUT(index, token) << "Making call " << count;
      if (params.repeat)
        cout << " of " << params.repeat;
      cout << " (total=" << totalAttempts
           << ") for " << delay << " seconds to "
           << destination;
      END_OUTPUT();

      if (params.tmax_est > 0) {
        OUTPUT(index, token, "Waiting " << params.tmax_est << " seconds for establishment");

        PTimer timeout = params.tmax_est;
        while (!callgen.manager.IsCallEstablished(token)) {
          stopping = exit.Wait(100);
          if (stopping || !timeout.IsRunning() || !callgen.manager.HasCall(token)) {
            delay = 0;
            break;
          }
        }
      }

      if (delay > 0) {
        // wait for a random time
        PTRACE(1, "CallGen\tWaiting for " << delay);
        stopping = exit.Wait(delay);
      }

      // end the call
      OUTPUT(index, token, "Clearing call");

      callgen.manager.ClearCallSynchronous(token);

      if (stopping)
        break;
    }

    count++;
    if (params.repeat > 0 && count > params.repeat)
      break;

    // wait for a random delay
    delay = RandomRange(rand, params.tmin_wait, params.tmax_wait);
    OUTPUT(index, PString::Empty(), "Delaying for " << delay << " seconds");

    PTRACE(1, "CallGen\tDelaying for " << delay);
    // wait for a random time
  } while (!exit.Wait(delay));

  OUTPUT(index, PString::Empty(), "Completed call set.");
  PTRACE(2, "CallGen\tFinished thread " << index);

  callgen.threadEnded.Signal();
}


void CallThread::Stop()
{
  if (!IsTerminated())
    OUTPUT(index, PString::Empty(), "Stopping.");

  exit.Signal();
}


///////////////////////////////////////////////////////////////////////////////

OpalCall * MyManager:: CreateCall(void * userData)
{
  return new MyCall(*this, (CallThread *)userData);
}


PBoolean MyManager::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  dynamic_cast<MyCall &>(connection.GetCall()).OnOpenMediaStream(connection, stream);
  return OpalManager::OnOpenMediaStream(connection, stream);
}


///////////////////////////////////////////////////////////////////////////////

MyCall::MyCall(MyManager & mgr, CallThread * caller)
  : OpalCall(mgr)
  , manager(mgr)
  , index(caller != NULL ? caller->index : 0)
  , openedTransmitMedia(0)
  , openedReceiveMedia(0)
  , receivedMedia(0)
{
}


void MyCall::OnEstablishedCall()
{
  PSafePtr<OpalConnection> connection = GetConnection(GetPartyA().NumCompare("IVR/") == EqualTo ? 1 : 0, PSafeReadOnly);

  OUTPUT(index, GetToken(), "Established \"" << connection->GetRemotePartyName() << "\""
                                  " " << connection->GetRemotePartyAddress() <<
                                    " active=" << manager.GetActiveCalls() <<
                                    " total=" << ++CallGen::Current().totalEstablished);
  OpalCall::OnEstablishedCall();
}


void MyCall::OnReleased(OpalConnection & connection)
{
  if (connection.GetRemotePartyName().NumCompare("IVR/") != EqualTo) {

    OUTPUT(index, GetToken(), "Cleared \"" << connection.GetRemotePartyName() << "\""
                                       " " << connection.GetRemotePartyAddress() <<
                                " reason=" << connection.GetCallEndReason());

    PTextFile & cdrFile = CallGen::Current().cdrFile;

    if (cdrFile.IsOpen()) {
      static PMutex cdrMutex;
      cdrMutex.Wait();

      if (cdrFile.GetLength() == 0)
        cdrFile << "Call Start Time,"
                   "Total duration,"
                   "Media open transmit time,"
                   "Media open received time,"
                   "Media received time,"
                   "ALERTING time,"
                   "CONNECT time,"
                   "Call End Reason,"
                   "Remote party,"
                   "Signalling gateway,"
                   "Media gateway,"
                   "Call Id,"
                   "Call Token\n";

      PTime setupTime = connection.GetSetupUpTime();

      cdrFile << setupTime.AsString("yyyy/M/d hh:mm:ss") << ','
              << setprecision(1) << (connection.GetConnectionEndTime() - setupTime) << ',';

      if (openedTransmitMedia.IsValid())
        cdrFile << (openedTransmitMedia - setupTime);
      cdrFile << ',';

      if (openedReceiveMedia.IsValid())
        cdrFile << (openedReceiveMedia - setupTime);
      cdrFile << ',';

      if (receivedMedia.IsValid())
        cdrFile << (receivedMedia - setupTime);
      cdrFile << ',';

      if (connection.GetAlertingTime().IsValid())
        cdrFile << (connection.GetAlertingTime() - setupTime);
      cdrFile << ',';

      if (connection.GetConnectionStartTime().IsValid())
        cdrFile << (connection.GetConnectionStartTime() - setupTime);
      cdrFile << ',';

      cdrFile << connection.GetCallEndReason() << ','
              << connection.GetRemotePartyName() << ','
              << connection.GetRemotePartyAddress() << ','
              << mediaGateway << ','
              << connection.GetIdentifier() << ','
              << connection.GetToken()
              << endl;

      cdrMutex.Signal();
    }
  }

  OpalCall::OnReleased(connection);
}


PBoolean MyCall::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  (stream.IsSink() ? openedTransmitMedia : openedReceiveMedia) = PTime();

  OUTPUT(index, connection.GetCall().GetToken(),
         "Opened " << (stream.IsSink() ? "transmitter" : "receiver")
                   << " for " << stream.GetMediaFormat());

  return TRUE;
}


void MyCall::OnRTPStatistics(const OpalConnection & connection, const RTP_Session & session)
{
  if (receivedMedia.GetTimeInSeconds() == 0 && session.GetPacketsReceived() > 0) {
    receivedMedia = PTime();
    OUTPUT(index, connection.GetCall().GetToken(), "Received media");

    const RTP_UDP * udpSess = dynamic_cast<const RTP_UDP *>(&session);
    if (udpSess != NULL) 
      mediaGateway = OpalTransportAddress(udpSess->GetRemoteAddress(), udpSess->GetRemoteDataPort());
  }
}


// End of file ////////////////////////////////////////////////////////////////
