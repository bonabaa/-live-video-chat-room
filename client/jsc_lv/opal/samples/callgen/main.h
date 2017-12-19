/*
 * main.h
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
 * $Revision: 19279 $
 * $Author: rjongbloed $
 * $Date: 2008-01-17 04:08:34 +0000 (Thu, 17 Jan 2008) $
 */


///////////////////////////////////////////////////////////////////////////////

class MyManager;
class CallThread;

class MyCall : public OpalCall
{
    PCLASSINFO(MyCall, OpalCall);
  public:
    MyCall(MyManager & manager, CallThread * caller);

    virtual void OnEstablishedCall();
    virtual void OnReleased(OpalConnection & connection);
    virtual PBoolean OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream);
    virtual void OnRTPStatistics(const OpalConnection & connection, const RTP_Session & session);

    MyManager          & manager;
    unsigned             index;
    PTime                openedTransmitMedia;
    PTime                openedReceiveMedia;
    PTime                receivedMedia;
    OpalTransportAddress mediaGateway;
};


///////////////////////////////////////////////////////////////////////////////

class MyManager : public OpalManager
{
    PCLASSINFO(MyManager, OpalManager);
  public:
    MyManager()
      { }

    virtual OpalCall * CreateCall(void * userData);

    virtual PBoolean OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream);

    PINDEX GetActiveCalls() const { return activeCalls.GetSize(); }
};


///////////////////////////////////////////////////////////////////////////////

class CallGen;

struct CallParams
{
  CallParams(CallGen & app)
    : callgen(app) { }

  CallGen & callgen;

  unsigned repeat;
  PTimeInterval tmax_est;
  PTimeInterval tmin_call;
  PTimeInterval tmax_call;
  PTimeInterval tmin_wait;
  PTimeInterval tmax_wait;
};


///////////////////////////////////////////////////////////////////////////////

class CallThread : public PThread
{
  PCLASSINFO(CallThread, PThread);
  public:
    CallThread(
      unsigned index,
      const PStringArray & destinations,
      const CallParams & params
    );
    void Main();
    void Stop();

    PStringArray destinations;
    unsigned     index;
    CallParams   params;
    PSyncPoint   exit;
};

PLIST(CallThreadList, CallThread);


///////////////////////////////////////////////////////////////////////////////

class CallGen : public PProcess 
{
  PCLASSINFO(CallGen, PProcess)

  public:
    CallGen();
    void Main();
    static CallGen & Current() { return (CallGen&)PProcess::Current(); }

    MyManager  manager;
    PString    outgoingMessageFile;
    PString    incomingAudioDirectory;
    PTextFile  cdrFile;

    PSyncPoint threadEnded;
    unsigned   totalAttempts;
    unsigned   totalEstablished;
    PMutex     coutMutex;

    PDECLARE_NOTIFIER(PThread, CallGen, Cancel);
    PConsoleChannel console;
    CallThreadList threadList;
};


///////////////////////////////////////////////////////////////////////////////
