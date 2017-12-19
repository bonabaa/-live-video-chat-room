/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Implementation of the class to handle the management of the protocol.
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2005 Indranet Technologies Ltd.
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
 * The Initial Developer of the Original Code is Indranet Technologies Ltd.
 *
 * The author of this code is Derek J Smithies
 *
 * $Revision: 23632 $
 * $Author: csoutheren $
 * $Date: 2009-10-09 06:02:18 +0000 (Fri, 09 Oct 2009) $
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_IAX2

#ifdef P_USE_PRAGMA
#pragma implementation "callprocessor.h"
#endif

#include <iax2/causecode.h>
#include <iax2/frame.h>
#include <iax2/iax2con.h>
#include <iax2/iax2ep.h>
#include <iax2/iax2medstrm.h>
#include <iax2/ies.h>
#include <iax2/processor.h>
#include <iax2/callprocessor.h>
#include <iax2/sound.h>
#include <iax2/transmit.h>

#include <typeinfo>

#define new PNEW

/********************************************************************************
 * A Call has several distinct steps. These steps are defined in OPAL, and they are
 *
 *  UninitialisedPhase,
 *  SetUpPhase,      
 *  AlertingPhase,   
 *  ConnectedPhase,  
 *  EstablishedPhase,
 *  ReleasingPhase, 
 *  ReleasedPhase,   
 *
 ********************************************************************************/

IAX2CallProcessor::IAX2CallProcessor(IAX2EndPoint &ep)
  : IAX2Processor(ep)
{
  callStatus = 0;
  
  soundWaitingForTransmission.Initialise();
  
  selectedCodec = 0;
  
  audioCanFlow = PFalse;
  audioFramesNotStarted = PTrue;

  con = NULL;

  firstMediaFrame = PTrue;
  answerCallNow = PFalse;
  audioFrameDuration = 0;
  audioCompressedBytes = 0;
  
  holdCall = PFalse;
  holdReleaseCall = PFalse;
  
  doTransfer = PFalse;
  
  statusCheckTimer.SetNotifier(PCREATE_NOTIFIER(OnStatusCheck));
  statusCheckOtherEnd = PFalse;
  
  soundBufferState = BufferToSmall;
  callStartTick = PTimer::Tick();
}


IAX2CallProcessor::~IAX2CallProcessor()
{
}

void IAX2CallProcessor::AssignConnection(IAX2Connection * _con)
{
  con = _con;

  PINDEX newCallNumber = con->GetEndPoint().NextSrcCallNumber(this);
  if (newCallNumber == P_MAX_INDEX) {
      /* This call cannot be started, cause we could not get a 
	 src call number to use.                               */
      return;
  }

  remote.SetSourceCallNumber(newCallNumber);
  
  Resume();
}

void IAX2CallProcessor::PrintOn(ostream & strm) const
{
  strm << "In call with " << con->GetRemotePartyAddress() << "  " << remotePhoneNumber << " " << callToken  << endl
       << "  Call has been up for " << setprecision(0) << setw(8)
       << (PTimer::Tick() - callStartTick) << " milliseconds" << endl
       << "  Control frames sent " << controlFramesSent    << endl
       << "  Control frames rcvd " << controlFramesRcvd    << endl
       << "  Audio frames sent   " << audioFramesSent      << endl
       << "  Audio frames rcvd   " << audioFramesRcvd      << endl
       << "  Video frames sent   " << videoFramesSent      << endl
       << "  Video frames rcvd   " << videoFramesRcvd      << endl;
}

void IAX2CallProcessor::Release(OpalConnection::CallEndReason reason)
{
  PTRACE(3, "Processor\tRelease(" << reason << ")");
  PStringStream str;
  str << reason;
  Hangup(str);
}

void IAX2CallProcessor::ClearCall(OpalConnection::CallEndReason reason)
{
  statusCheckTimer.Stop();
  PTRACE(3, "ListProcesser runs     =====ClearCall(" << reason << ")");
  
  PStringStream str;
  str << reason;
  Hangup(str);

  con->EndCallNow(reason);
}

void IAX2CallProcessor::OnReleased()
{
  Terminate();
}

void IAX2CallProcessor::PutSoundPacketToNetwork(PBYTEArray *sound)
{
  /*This thread does not send the audio frame. 
    The IAX2CallProcessor thread sends the audio frame */
  soundWaitingForTransmission.AddNewEntry(sound);
  
  CleanPendingLists();
} 

void IAX2CallProcessor::CallStopSounds()
{
  //cout << "We have received a call stop sounds packet " << endl;
}

void IAX2CallProcessor::ReceivedHookFlash() 
{ 
} 

void IAX2CallProcessor::RemoteNodeIsBusy() 
{ 
} 

void IAX2CallProcessor::RemoteNodeHasAnswered()
{
  //Remote node is proceeeding, and quite likes our call.
  //Move call to the ConnectedPhase
  if (IsCallAnswered()) {
    PTRACE(3, "Second Answer Yes packet received. Ignore it");
    return;
  }
  
  SetCallAnswered();
  PTRACE(3, "Processor\tRemote node has answered");
  con->OnConnectedInternal();
}

void IAX2CallProcessor::Hangup(PString dieMessage)
{
  PTRACE(3, "Hangup request " << dieMessage);
  hangList.AppendString(dieMessage);   //send this text to remote endpoint 
  
  activate.Signal();
}

void IAX2CallProcessor::CheckForHangupMessages()
{
  if (hangList.IsEmpty()) 
    return;

  if (!IsCallTerminating()) {
    IAX2FullFrameProtocol * f = 
      new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdHangup, IAX2FullFrame::callIrrelevant);
    PTRACE(3, "Send a hangup frame to the remote endpoint");
    
    f->AppendIe(new IAX2IeCause(hangList.GetFirstDeleteAll()));
    f->AppendIe(new IAX2IeCauseCode(IAX2IeCauseCode::NormalClearing));
    TransmitFrameToRemoteEndpoint(f);  
    PThread::Sleep(2);
  } else {
    PTRACE(3, "hangup message required. Not sending, cause already have a hangup message in queue");
  }
  
  Terminate();
}

void IAX2CallProcessor::ConnectToRemoteNode(PString & newRemoteNode)
{      // Parse a string like guest@node.name.com/23234
  
  PTRACE(2, "Connect to remote node " << newRemoteNode);
  PStringArray res = IAX2EndPoint::DissectRemoteParty(newRemoteNode);

  if (res[IAX2EndPoint::addressIndex].IsEmpty()) {
    PTRACE(3, "Opal\tremote node to call is not specified correctly iax2:" << newRemoteNode);
    PTRACE(3, "Opal\tExample format is iax2:guest@misery.digium.com/s");
    PTRACE(3, "Opal\tYou must supply (as a minimum iax2:address)");
    PTRACE(3, "Opal\tYou supplied " <<
     "iax2:" <<
     (res[IAX2EndPoint::userIndex].IsEmpty()      ? "" : res[IAX2EndPoint::userIndex])     << "@" <<
     (res[IAX2EndPoint::addressIndex].IsEmpty()   ? "" : res[IAX2EndPoint::addressIndex])  << "/" <<
     (res[IAX2EndPoint::extensionIndex].IsEmpty() ? "" : res[IAX2EndPoint::extensionIndex])            );
    return;
  }
    
  PIPSocket::Address ip;
  if (!PIPSocket::GetHostAddress(res[IAX2EndPoint::addressIndex], ip)) {
    PTRACE(1, "Conection\tFailed to make call to " << res[IAX2EndPoint::addressIndex]);
    return;
  }
  PTRACE(4, "Resolve " << res[IAX2EndPoint::addressIndex]  << " as ip address " << ip);
    
  remote.SetRemotePort(con->GetEndPoint().ListenPortNumber());
  remote.SetRemoteAddress(ip);
    
  IAX2FullFrameProtocol * f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdNew);
  f->AppendIe(new IAX2IeVersion());
  f->AppendIe(new IAX2IeFormat(con->GetPreferredCodec()));
  f->AppendIe(new IAX2IeCapability(con->GetSupportedCodecs()));
  
  if (!endpoint.GetLocalNumber().IsEmpty())
    f->AppendIe(new IAX2IeCallingNumber(endpoint.GetLocalNumber()));

  if (!res[IAX2EndPoint::userIndex].IsEmpty()) {
    f->AppendIe(new IAX2IeCallingName(res[IAX2EndPoint::userIndex]));
    f->AppendIe(new IAX2IeUserName(res[IAX2EndPoint::userIndex]));
  } else if (!GetUserName().IsEmpty()) {
    f->AppendIe(new IAX2IeCallingName(GetUserName()));
    f->AppendIe(new IAX2IeUserName(GetUserName()));
  }

  if (!res[IAX2EndPoint::extensionIndex].IsEmpty())
    f->AppendIe(new IAX2IeCalledNumber(res[IAX2EndPoint::extensionIndex] ));

  if (!res[IAX2EndPoint::extensionIndex].IsEmpty())
    f->AppendIe(new IAX2IeDnid(res[IAX2EndPoint::extensionIndex]));

  if (!res[IAX2EndPoint::contextIndex].IsEmpty())
    f->AppendIe(new IAX2IeCalledContext(res[IAX2EndPoint::contextIndex]));

#if OPAL_PTLIB_SSL_AES
  f->AppendIe(new IAX2IeEncryption());
#endif

  TransmitFrameToRemoteEndpoint(f);
  StartNoResponseTimer();
  return;
}

void IAX2CallProcessor::ReportStatistics()
{
#if 0
  cout << "  Call has been up for " << setprecision(0) << setw(8)
       << (PTimer::Tick() - callStartTick) << " milliseconds" << endl
       << "  Control frames sent " << controlFramesSent  << endl
       << "  Control frames rcvd " << controlFramesRcvd  << endl
       << "  Audio frames sent   " << audioFramesSent      << endl
       << "  Audio frames rcvd   " << audioFramesRcvd      << endl
       << "  Video frames sent   " << videoFramesSent      << endl
       << "  Video frames rcvd   " << videoFramesRcvd      << endl;
#endif
}

void IAX2CallProcessor::ProcessFullFrame(IAX2FullFrame & fullFrame)
{
  switch(fullFrame.GetFrameType()) {
  case IAX2FullFrame::dtmfType:        
    PTRACE(5, "Build matching full frame    dtmfType");
    ProcessNetworkFrame(new IAX2FullFrameDtmf(fullFrame));
    break;
  case IAX2FullFrame::voiceType:       
    PTRACE(5, "Build matching full frame    voiceType");
    ProcessNetworkFrame(new IAX2FullFrameVoice(fullFrame));
    break;
  case IAX2FullFrame::videoType:       
    PTRACE(5, "Build matching full frame    videoType");
    ProcessNetworkFrame(new IAX2FullFrameVideo(fullFrame));
    break;
  case IAX2FullFrame::controlType:     
    PTRACE(5, "Build matching full frame    controlType");
    ProcessNetworkFrame(new IAX2FullFrameSessionControl(fullFrame));
    break;
  case IAX2FullFrame::nullType:        
    PTRACE(5, "Build matching full frame    nullType");
    ProcessNetworkFrame(new IAX2FullFrameNull(fullFrame));
    break;
  case IAX2FullFrame::iax2ProtocolType: 
    PTRACE(5, "Build matching full frame    iax2ProtocolType");
    ProcessNetworkFrame(new IAX2FullFrameProtocol(fullFrame));
    break;
  case IAX2FullFrame::textType:        
    PTRACE(5, "Build matching full frame    textType");
    ProcessNetworkFrame(new IAX2FullFrameText(fullFrame));
    break;
  case IAX2FullFrame::imageType:       
    PTRACE(5, "Build matching full frame    imageType");
    ProcessNetworkFrame(new IAX2FullFrameImage(fullFrame));
    break;
  case IAX2FullFrame::htmlType:        
    PTRACE(5, "Build matching full frame    htmlType");
    ProcessNetworkFrame(new IAX2FullFrameHtml(fullFrame));
    break;
  case IAX2FullFrame::cngType:        
    PTRACE(5, "Build matching full frame    cngType");
    ProcessNetworkFrame(new IAX2FullFrameCng(fullFrame));
    break;
  default: 
    PTRACE(5, "Build matching full frame, Type not understood");
  };
  
}

void IAX2CallProcessor::ProcessLists()
{
  while (ProcessOneIncomingEthernetFrame()) {
    ;
  }
  
  PBYTEArray *oneSound;
  do {
    oneSound = soundWaitingForTransmission.GetLastEntry();
    SendSoundMessage(oneSound);
  } while (oneSound != NULL);
   
  PString nodeToCall = callList.GetFirstDeleteAll();
  if (!nodeToCall.IsEmpty()) {
    PTRACE(4, "make a call to " << nodeToCall);
    ConnectToRemoteNode(nodeToCall);
  }
  
  if (!dtmfText.IsEmpty()) {
    PString dtmfs = dtmfText.GetAndDelete();
    PTRACE(4, "Have " << dtmfs << " DTMF chars to send");
    for (PINDEX i = 0; i < dtmfs.GetLength(); i++)
      SendDtmfMessage(dtmfs[i]);
  }  

  if (textList.StringsAvailable()) {
    PStringArray sendList; // text messages
    textList.GetAllDeleteAll(sendList);
    PTRACE(4, "Have " << sendList.GetSize() << " text strings to send");
    for (PINDEX i = 0; i < sendList.GetSize(); i++)
      SendTextMessage(sendList[i]);    
  }
   
  //This method will send a transfer message if it has been requested
  SendTransferMessage();   

  if (answerCallNow) {
    PTRACE(4, "Processor\tUser answer call with yes");
    //We have accepted an incoming iax2 call.
    PTRACE(4, "Processor\tUser accepted call, initiate media streams");
    SendAnswerMessageToRemoteNode();
    con->SetConnected();
  }    

  if (holdCall)
    SendQuelchMessage();
  
  if (holdReleaseCall)
    SendUnQuelchMessage();

  if (statusCheckOtherEnd)
    DoStatusCheck();

  CheckForHangupMessages();
}

void IAX2CallProcessor::ProcessIncomingAudioFrame(IAX2Frame *newFrame)
{
  PTRACE(5, "Processor\tProcessIncomingAudioframe " << newFrame->IdString());
  //cout << PThread::Current()->GetThreadName() << " ProcessIncomingAudioFrame(cp) "<< PTimer::Tick() << endl;
  IncAudioFramesRcvd();
  
  con->ReceivedSoundPacketFromNetwork(newFrame);
}

void IAX2CallProcessor::ProcessIncomingVideoFrame(IAX2Frame *newFrame)
{
  PTRACE(3, "Incoming video frame ignored, cause we don't handle it");
  IncVideoFramesRcvd();
  delete newFrame;
}


void IAX2CallProcessor::SendSoundMessage(PBYTEArray *sound)
{
  if (sound == NULL)
    return;

  if (sound->GetSize() == 0) {
    delete sound;
    return;
  }

  IncAudioFramesSent();   


  DWORD lastTimeStamp = currentSoundTimeStamp;
  DWORD thisTimeStamp = currentSoundTimeStamp + audioFrameDuration;

  PBoolean sendFullFrame =  (((thisTimeStamp & 0xffff) < (lastTimeStamp & 0xffff))
			 || audioFramesNotStarted);

  currentSoundTimeStamp = thisTimeStamp;

  if (sendFullFrame) {
    audioFramesNotStarted = PFalse;
    IAX2FullFrameVoice *f = new IAX2FullFrameVoice(this, *sound, thisTimeStamp);
    PTRACE(5, "Send a full audio frame" << thisTimeStamp << " On " << f->IdString());
    TransmitFrameToRemoteEndpoint(f);
  } else {
    IAX2MiniFrame *f = new IAX2MiniFrame(this, *sound, PTrue, thisTimeStamp & 0xffff);
    TransmitFrameToRemoteEndpoint(f);
  }
  
  delete sound;
}

void IAX2CallProcessor::SendDtmf(const PString & dtmfs)
{
  PTRACE(4, "Activate the iax2 processeor, DTMF of  " << dtmfs << " to send");
  dtmfText += dtmfs;
  activate.Signal();
}

void IAX2CallProcessor::SendText(const PString & text)
{
  PTRACE(4, "Activate the iax2 processeor, text of " << text << " to send");
  textList.AppendString(text);
  activate.Signal();
}

void IAX2CallProcessor::SendHold()
{
  holdCall = PTrue;
}

void IAX2CallProcessor::SendHoldRelease()
{
  holdReleaseCall = PTrue;
}

void IAX2CallProcessor::SendTransfer(
  const PString & calledNumber,
  const PString & calledContext)
{
  {
    PWaitAndSignal m(transferMutex);
    doTransfer = PTrue;
    transferCalledNumber = calledNumber;
    transferCalledContext = calledContext;
  }
  
  activate.Signal();
}


void IAX2CallProcessor::SendDtmfMessage(char  message)
{
  IAX2FullFrameDtmf *f = new IAX2FullFrameDtmf(this, message);
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2CallProcessor::SendTransferMessage()
{
  PWaitAndSignal m(transferMutex);
  if (doTransfer) {
    IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdTransfer);
    f->AppendIe(new IAX2IeCalledNumber(transferCalledNumber));
    
    if (!transferCalledContext.IsEmpty())
      f->AppendIe(new IAX2IeCalledContext(transferCalledContext));
    TransmitFrameToRemoteEndpoint(f);
    
    doTransfer = PFalse;
  }
}


void IAX2CallProcessor::SendTextMessage(PString & message)
{
  IAX2FullFrameText *f = new IAX2FullFrameText(this, message);
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2CallProcessor::SendQuelchMessage()
{
  holdCall = PFalse;
  
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdQuelch);
  //we'll request that music is played to them because everyone likes music when they're on hold ;)
  f->AppendIe(new IAX2IeMusicOnHold());
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2CallProcessor::SendUnQuelchMessage()
{
  holdReleaseCall = PFalse;
  
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdUnquelch); 
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2MiniFrame * src)
{
  src->AlterTimeStamp(lastFullFrameTimeStamp);
  
  if (src->IsVideo()) {
    PTRACE(3, "Incoming mini video frame");
    ProcessIncomingVideoFrame(src);
    return;
  }
  
  if (src->IsAudio()) {
    PTRACE(5, "Incoming mini audio frame");
    ProcessIncomingAudioFrame(src);
    return;
  }
  
  PTRACE(1, "ERROR - mini frame is not marked as audio or video");
  delete src;
  return;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrame * src)
{
  PTRACE(5, "ProcessNetworkFrame(IAX2FullFrame * src)");
  PStringStream message;
  message << PString("Do not know how to process networks packets of \"Full Frame\" type ") << *src;
  PAssertAlways(message);
  return;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2Frame * src)
{
  PTRACE(5, "ProcessNetworkFrame(IAX2Frame * src)");
  PStringStream message;
  message << PString("Do not know how to process networks packets of \"Frame\" type ") << *src;
  PTRACE(3, message);
  PTRACE(3, message);
  PAssertAlways(message);
  return;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameDtmf * src)
{
  PTRACE(5, "ProcessNetworkFrame(IAX2FullFrameDtmf * src)");
  SendAckFrame(src);
  con->OnUserInputTone((char)src->GetSubClass(), 1);

  delete src;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameVoice * src)
{
  if (firstMediaFrame) {
    PTRACE(5, "Processor\tReceived first voice media frame " << src->IdString());
    firstMediaFrame = PFalse;
  }
  PTRACE(5, "ProcessNetworkFrame(IAX2FullFrameVoice * src)" << src->IdString());
  SendAckFrame(src);
  ProcessIncomingAudioFrame(src);
  
  return;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameVideo * src)
{
  if (firstMediaFrame) {
    PTRACE(5, "Processor\tReceived first video media frame ");
    firstMediaFrame = PFalse;
  }

  PTRACE(5, "ProcessNetworkFrame(IAX2FullFrameVideo * src)");
  SendAckFrame(src);
  ProcessIncomingVideoFrame(src);
  
  return;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameSessionControl * src)
{ /* these frames are labelled as AST_FRAME_CONTROL in the asterisk souces.
     We could get an Answer message from here., or a hangup., or...congestion control... */
  
  PTRACE(4, "ProcessNetworkFrame(IAX2FullFrameSessionControl * src)");
  SendAckFrame(src);  
  
  switch(src->GetSubClass()) {
  case IAX2FullFrameSessionControl::hangup:          // Other end has hungup
    SetCallTerminating(PTrue);
    //cout << "Other end has hungup, so exit" << endl;
    con->EndCallNow();
    break;
    
  case IAX2FullFrameSessionControl::ring:            // Local ring
    break;
    
  case IAX2FullFrameSessionControl::ringing:         // Remote end is ringing
    RemoteNodeIsRinging();
    break;
    
  case IAX2FullFrameSessionControl::answer:          // Remote end has answered
    PTRACE(3, "Have received answer packet from remote endpoint ");    
    RemoteNodeHasAnswered(); //Moves OpalConnection phase to Connected
    break;
    
  case IAX2FullFrameSessionControl::busy:            // Remote end is busy
    RemoteNodeIsBusy();
    break;
    
  case IAX2FullFrameSessionControl::tkoffhk:         // Make it go off hook
    break;
    
  case IAX2FullFrameSessionControl::offhook:         // Line is off hook
    break;
    
  case IAX2FullFrameSessionControl::congestion:      // Congestion (circuits busy)
    break;
    
  case IAX2FullFrameSessionControl::flashhook:       // Flash hook
    ReceivedHookFlash();
    break;
    
  case IAX2FullFrameSessionControl::wink:            // Wink
    break;
    
  case IAX2FullFrameSessionControl::option:          // Set a low-level option
    break;
    
  case IAX2FullFrameSessionControl::keyRadio:        // Key Radio
    break;
    
  case IAX2FullFrameSessionControl::unkeyRadio:      // Un-Key Radio
    break;
    
  case IAX2FullFrameSessionControl::callProgress:    // Indicate PROGRESS
    /**We definately do nothing here */
    break;
    
  case IAX2FullFrameSessionControl::callProceeding:  // Indicate CALL PROCEEDING
    /**We definately do nothing here */
    break;
    
  case IAX2FullFrameSessionControl::callOnHold:      // Call has been placed on hold
    con->RemoteHoldConnection();
    break;
    
  case IAX2FullFrameSessionControl::callHoldRelease: // Call is no longer on hold
    con->RemoteRetrieveConnection();
    break;
    
  case IAX2FullFrameSessionControl::stopSounds:      // Moving from call setup to media flowing
    CallStopSounds();
    break;
    
  default:
    break;
  };
  
  delete(src);
  return;
}

PBoolean IAX2CallProcessor::SetUpConnection()
{
  PTRACE(3, "IAX2\tSet Up Connection to remote node " 
	 << con->GetRemotePartyName() << " " 
	 << con->GetRemotePartyAddress());
   
  callList.AppendString(con->GetRemotePartyAddress());
  
  CleanPendingLists();
  return PTrue;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameNull * src)
{
  PTRACE(4, "ProcessNetworkFrame(IAX2FullFrameNull * src)");
  delete src;
  return;
}

PBoolean IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameProtocol * src)
{ /* these frames are labelled as AST_FRAME_IAX in the asterisk souces.
     These frames contain Information Elements in the data field.*/  
  PTRACE(4, "ProcessNetworkFrame " << *src);
  StopNoResponseTimer();

  CheckForRemoteCapabilities(src);
  src->CopyDataFromIeListTo(ieData);
  
  //check if the common method can process it?
  if (IAX2Processor::ProcessNetworkFrame(src)) {
    return PTrue;
  }

  switch(src->GetSubClass()) {
  case IAX2FullFrameProtocol::cmdNew:
    ProcessIaxCmdNew(src);
    break;
  case IAX2FullFrameProtocol::cmdAck:
    ProcessIaxCmdAck(src);
    break;
  case IAX2FullFrameProtocol::cmdHangup:
    ProcessIaxCmdHangup(src);
    break;
  case IAX2FullFrameProtocol::cmdReject:
    ProcessIaxCmdReject(src);
    break;
  case IAX2FullFrameProtocol::cmdAccept:
    ProcessIaxCmdAccept(src);
    break;
  case IAX2FullFrameProtocol::cmdAuthReq:
    ProcessIaxCmdAuthReq(src);
    break;
  case IAX2FullFrameProtocol::cmdAuthRep:
    ProcessIaxCmdAuthRep(src);
    break;
  case IAX2FullFrameProtocol::cmdInval:
    ProcessIaxCmdInval(src);
    break; 
  case IAX2FullFrameProtocol::cmdDpReq:
    ProcessIaxCmdDpReq(src);
    break;
  case IAX2FullFrameProtocol::cmdDpRep:
    ProcessIaxCmdDpRep(src);
    break;
  case IAX2FullFrameProtocol::cmdDial:
    ProcessIaxCmdDial(src);
    break;
  /*case IAX2FullFrameProtocol::cmdTxreq:
    ProcessIaxCmdTxreq(src);
    break;
  case IAX2FullFrameProtocol::cmdTxcnt:
    ProcessIaxCmdTxcnt(src);
    break;
  case IAX2FullFrameProtocol::cmdTxacc:
    ProcessIaxCmdTxacc(src);
    break;
  case IAX2FullFrameProtocol::cmdTxready:
    ProcessIaxCmdTxready(src);
    break;
  case IAX2FullFrameProtocol::cmdTxrel:
    ProcessIaxCmdTxrel(src);
    break;
  case IAX2FullFrameProtocol::cmdTxrej:
    ProcessIaxCmdTxrej(src);
    break;*/
  case IAX2FullFrameProtocol::cmdQuelch:
    ProcessIaxCmdQuelch(src);
    break;
  case IAX2FullFrameProtocol::cmdUnquelch:
    ProcessIaxCmdUnquelch(src);
    break;
  case IAX2FullFrameProtocol::cmdPage:
    ProcessIaxCmdPage(src);
    break;/*
  case IAX2FullFrameProtocol::cmdMwi:
    ProcessIaxCmdMwi(src);
    break;
  case IAX2FullFrameProtocol::cmdUnsupport:
    ProcessIaxCmdUnsupport(src);
    break;
  case IAX2FullFrameProtocol::cmdTransfer:
    ProcessIaxCmdTransfer(src);
    break;
  case IAX2FullFrameProtocol::cmdProvision:
    ProcessIaxCmdProvision(src);
    break;
  case IAX2FullFrameProtocol::cmdFwDownl:
    ProcessIaxCmdFwDownl(src);
    break;
  case IAX2FullFrameProtocol::cmdFwData:
    ProcessIaxCmdFwData(src);
    break;*/
  default:
    PTRACE(1, "Process Full Frame Protocol, Type not expected");
    SendUnsupportedFrame(src);   //This method will delete the frame src.
  };
  
  return PFalse;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameText * src)
{
  PTRACE(4, "ProcessNetworkFrame(IAX2FullFrameText * src)");
  delete src;
  return;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameImage * src)
{
  PTRACE(4, "ProcessNetworkFrame(IAX2FullFrameImage * src)");
  delete src;
  return;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameHtml * src)
{
  PTRACE(4, "ProcessNetworkFrame(IAX2FullFrameHtml * src)");
  delete src;
  return;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameCng * src)
{
  PTRACE(4, "ProcessNetworkFrame(IAX2FullFrameCng * src)");
  delete src;
  return;
}

void IAX2CallProcessor::CheckForRemoteCapabilities(IAX2FullFrameProtocol *src)
{
  unsigned int remoteCapability, format;
  
  src->GetRemoteCapability(remoteCapability, format);

  PTRACE(4, "Connection\tRemote capabilities are " << remoteCapability << "   codec preferred " << format);
  if ((remoteCapability == 0) && (format == 0))
    return;

  con->BuildRemoteCapabilityTable(remoteCapability, format);
}

PBoolean IAX2CallProcessor::RemoteSelectedCodecOk()
{
  selectedCodec = con->ChooseCodec();
  
  if (selectedCodec == 0) {
    IAX2FullFrameProtocol * reply;
    reply = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdReject, IAX2FullFrame::callIrrelevant);
    reply->AppendIe(new IAX2IeCause("Unable to negotiate codec"));
    reply->AppendIe(new IAX2IeCauseCode(IAX2IeCauseCode::BearerCapabilityNotAvail));
    TransmitFrameToRemoteEndpoint(reply);
    con->ClearCall(OpalConnection::EndedByCapabilityExchange);
    return PFalse;
  }
  
  return PTrue;
}

void IAX2CallProcessor::ProcessIaxCmdNew(IAX2FullFrameProtocol *src)
{ /*That we are here indicates this connection is already in place */
  PTRACE(3, "ProcessIaxCmdNew(IAX2FullFrameProtocol *src)");
  remote.SetRemoteAddress(src->GetRemoteInfo().RemoteAddress());
  remote.SetRemotePort(src->GetRemoteInfo().RemotePort());

  if (IsCallHappening()) {
    PTRACE(3, "Remote node has sent us a second new message. ignore");
    delete src;
    return;
  }

  IAX2FullFrameProtocol * reply;
  
  
  if (!RemoteSelectedCodecOk()) {
    PTRACE(3, "Remote node sected a bad codec, hangup call ");
    reply = new  IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdInval, src, IAX2FullFrame::callIrrelevant);
    TransmitFrameToRemoteEndpoint(reply);
    
    reply= new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdHangup, IAX2FullFrame::callIrrelevant);
    PTRACE(3, "Send a hangup frame to the remote endpoint as there is no codec available");    
    reply->AppendIe(new IAX2IeCause("No matching codec"));
    SetCallTerminating();
    //              f->AppendIe(new IeCauseCode(IeCauseCode::NormalClearing));
    TransmitFrameToRemoteEndpoint(reply);
    
    con->EndCallNow(OpalConnection::EndedByCapabilityExchange);
    delete src;
    return;
  }
  
  SetCallNewed();
  PTRACE(3, "ProcessIaxCmdNew have an incoming call to manage");
  {
    unsigned options = 0;
    OpalConnection::StringOptions stringOptions;
    con->OnIncomingConnection(options, &stringOptions);
  }
  con->OnSetUp();   //The person receiving call does ownerCall.OnSetUp();

  con->GetEndPoint().GetCodecLengths(selectedCodec, audioCompressedBytes, 
				     audioFrameDuration);


  /*At this point, we have selected a codec to use. */
  reply = new  IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdAccept);
  reply->AppendIe(new IAX2IeFormat(selectedCodec));
  TransmitFrameToRemoteEndpoint(reply);
  SetCallAccepted();

  /*We could send an AuthReq frame at this point */

  IAX2FullFrameSessionControl *r;
  r = new IAX2FullFrameSessionControl(this, 
				      IAX2FullFrameSessionControl::ringing);
  TransmitFrameToRemoteEndpoint(r, IAX2WaitingForAck::RingingAcked);
  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdAck(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "ProcessIaxCmdAck(IAX2FullFrameProtocol * /*src*/)");
  /* The corresponding IAX2FullFrame has already been marked as acknowledged */
  
  
  if (!nextTask.MatchingAckPacket(src)) {
    PTRACE(3, "ack packet does not match a pending response");
    delete src;
    return;
  }
  
  IAX2WaitingForAck::ResponseToAck action = nextTask.GetResponse();
  nextTask.ZeroValues();
  switch(action) {
  case IAX2WaitingForAck::RingingAcked : 
    RingingWasAcked();
    break;
  case IAX2WaitingForAck::AcceptAcked  : 
    break;;
  case IAX2WaitingForAck::AuthRepAcked : 
    break;
  case IAX2WaitingForAck::AnswerAcked  : 
    AnswerWasAcked();
    break;
  }

  delete src;  
}

void IAX2CallProcessor::RingingWasAcked()
{
  PTRACE(4, "Processor\t Remote node " << con->GetRemotePartyAddress() << " knows our phone is ringing");
}

void IAX2CallProcessor::AnswerWasAcked()
{
  PTRACE(4, "Answer was acked");
}

void IAX2CallProcessor::AcceptIncomingCall()
{
  PTRACE(4, "AcceptIncomingCall()");

  answerCallNow = PTrue;
  CleanPendingLists();
}

void IAX2CallProcessor::SendAnswerMessageToRemoteNode()
{
  answerCallNow = PFalse;
  if (!IsCallAnswered()) {
    SetCallAnswered(true);
    PTRACE(4, "Processor\tSend Answer message");
    IAX2FullFrameSessionControl * reply;
    reply = new IAX2FullFrameSessionControl(this, IAX2FullFrameSessionControl::answer);
    TransmitFrameToRemoteEndpoint(reply, IAX2WaitingForAck::AnswerAcked);
  }
}

PBoolean IAX2CallProcessor::SetAlerting(const PString & PTRACE_PARAM(calleeName), PBoolean /*withMedia*/)
{
  PTRACE(3, "Processor\tSetAlerting from " << calleeName);
  return PTrue;
}

/*The remote node has told us to hangup and go away */
void IAX2CallProcessor::ProcessIaxCmdHangup(IAX2FullFrameProtocol *src)
{ 
  SetCallTerminating(PTrue);

  PTRACE(3, "Processor\tProcessIaxCmdHangup(IAX2FullFrameProtocol *src)");
  SendAckFrame(src);

  PTRACE(1, "The remote node (" << con->GetRemotePartyAddress()  << ") has closed the call");

  con->EndCallNow(OpalConnection::EndedByRemoteUser);

  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdReject(IAX2FullFrameProtocol *src)
{
  PTRACE(3, "Processor\tProcessIaxCmdReject(IAX2FullFrameProtocol *src)");
  //cout << "Remote endpoint has rejected our call " << endl;
  //cout << "Cause \"" << ieData.cause << "\"" << endl;
  SendAckFrame(src);
  con->EndCallNow(OpalConnection::EndedByRefusal);

  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdAccept(IAX2FullFrameProtocol *src)
{
  if (IsCallAccepted()) {
    PTRACE(3, "Second accept packet received. Ack, delete, & do nothing.");
    SendAckFrame(src);
    delete src;
    return;
  }

  PTRACE(4, "Processor\tProcessIaxCmdAccept(IAX2FullFrameProtocol *src)");
      
  /* Other end has acknowledged our request to make a call.
     Tell our connection that the phone is ringing at the other end. */
  PString calleeName = con->GetRemotePartyName();
  con->SetAlerting(calleeName, PTrue);


  SendAckFrame(src);
  SetCallAccepted();
  
  PTRACE(4, "Now check codecs");
  
  if (!RemoteSelectedCodecOk()) {
    PTRACE(3, "Remote node sected a bad codec, hangup call ");
    Release();
    return;
  }
  PString codecName = IAX2FullFrameVoice::GetOpalNameOfCodec((unsigned short)selectedCodec);
  PTRACE(4, "Processor\tRemote endpoint has accepted our call on codec " << codecName);
  con->GetEndPoint().GetCodecLengths(selectedCodec, audioCompressedBytes, audioFrameDuration);

  PTRACE(4, "Processor\tcodec frame play duration is " << audioFrameDuration << " ms, which compressed to " 
   << audioCompressedBytes << " bytes of data");

  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdAuthReq(IAX2FullFrameProtocol * src)
{
  PTRACE(4, "Processor\tProcessIaxCmdAuthReq(IAX2FullFrameProtocol *src)");

  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdAuthRep);
  
  if (!GetPassword().IsEmpty())
    Authenticate(f, password);
  else
    Authenticate(f, endpoint.GetPassword());
  
  TransmitFrameToRemoteEndpoint(f);
  StartNoResponseTimer();

  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdAuthRep(IAX2FullFrameProtocol * src)
{
  PTRACE(4, "Processor\tProcessIaxCmdAuthRep(IAX2FullFrameProtocol *src)");
  delete src;
  /** When this packet has been acked, we send an accept */     
}

void IAX2CallProcessor::ProcessIaxCmdInval(IAX2FullFrameProtocol * src)
{
  PTRACE(4, "Processor\tProcessIaxCmdInval(IAX2FullFrameProtocol *src) " << src->IdString());
  PTRACE(4, "Processor\tProcessIaxCmdInval(IAX2FullFrameProtocol *src) " 
	 << src->GetSequenceInfo().AsString());
  PTRACE(4, "Processor\tProcessIaxCmdInval(IAX2FullFrameProtocol *src) " << src->GetTimeStamp());

  if (src->GetSequenceInfo().IsSequenceNosZero() && (src->GetTimeStamp() == 0)) {
    PTRACE(3, "Processor\tProcessIaxCmdInval - remote end does not like us, and nuked the call");
    con->ClearCall(OpalConnection::EndedByRemoteUser);
  }
  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdDpReq(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdDpReq(IAX2FullFrameProtocol *src)");
  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdDpRep(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdDpRep(IAX2FullFrameProtocol *src)");
  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdDial(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdDial(IAX2FullFrameProtocol *src)");
  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdTxreq(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdTxreq(IAX2FullFrameProtocol */*src*/)");
  delete src;
  //Not implemented.
}

void IAX2CallProcessor::ProcessIaxCmdTxcnt(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdTxcnt(IAX2FullFrameProtocol * /*src*/)");
  delete src;
//Not implemented.
}

void IAX2CallProcessor::ProcessIaxCmdTxacc(IAX2FullFrameProtocol *src)
{ /*Transfer has been accepted */
  PTRACE(4, "Processor\tProcessIaxCmdTxacc(IAX2FullFrameProtocol * /*src*/)");
  delete src;
  //Not implemented.
}

void IAX2CallProcessor::ProcessIaxCmdTxready(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdTxready(IAX2FullFrameProtocol */*src*/)");
  delete src;
  //Not implemented.
}

void IAX2CallProcessor::ProcessIaxCmdTxrel(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdTxrel(IAX2FullFrameProtocol */*src*/)");
  delete src;
  //Not implemented.
}

void IAX2CallProcessor::ProcessIaxCmdTxrej(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdTxrej(IAX2FullFrameProtocol */*src*/)");
  delete src;
  //Not implemented. (transfer rejected)
}

void IAX2CallProcessor::ProcessIaxCmdQuelch(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdQuelch(IAX2FullFrameProtocol */*src*/)");
  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdUnquelch(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdUnquelch(IAX2FullFrameProtocol */*src*/)");
  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdPage(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdPage(IAX2FullFrameProtocol *src)");
  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdMwi(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdMwi(IAX2FullFrameProtocol *src)");
  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdUnsupport(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdUnsupport(IAX2FullFrameProtocol *src)");
  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdTransfer(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdTransfer(IAX2FullFrameProtocol *src)");
  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdProvision(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdProvision(IAX2FullFrameProtocol *src)");
  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdFwDownl(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdFwDownl(IAX2FullFrameProtocol *src)");
  delete src;
}

void IAX2CallProcessor::ProcessIaxCmdFwData(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "Processor\tProcessIaxCmdFwData(IAX2FullFrameProtocol *src)");
  delete src;
}

void IAX2CallProcessor::StartStatusCheckTimer(PINDEX msToWait)
{
  PTRACE(4, "Processor\tStatusCheck time. Now set flag to send a ping+lagrq packets");
  PTRACE(4, "Processor\tStatusCheck timer set to " << msToWait << "  ms");
  statusCheckTimer = PTimeInterval(msToWait); 
  statusCheckOtherEnd = PTrue;
  CleanPendingLists();
}

void IAX2CallProcessor::OnStatusCheck(PTimer &, INT)
{
  StartStatusCheckTimer();
}

void IAX2CallProcessor::RemoteNodeIsRinging()
{
  StopNoResponseTimer();
  
  StartNoResponseTimer(60 * 1000); /* The remote end has one minute to answer their phone */
}

void IAX2CallProcessor::DoStatusCheck()
{
  statusCheckOtherEnd = PFalse;
  if (IsCallTerminating())
    return;

  IAX2FullFrame *p = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdPing);
  TransmitFrameToRemoteEndpoint(p);

  p = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdLagRq);
  TransmitFrameToRemoteEndpoint(p);
}

void IAX2CallProcessor::OnNoResponseTimeout()
{
  PTRACE(3, "hangup now, as we have had no response from the remote node in the specified time ");
  
  con->ClearCall(OpalConnection::EndedByNoAnswer);
}

PString IAX2CallProcessor::GetUserName() const
{
  //this returns the most important username that is set.
  //ie: Username > endpointUsername.
  if (!userName.IsEmpty())
    return userName;
  else if (!endpoint.GetLocalUserName().IsEmpty())
    return endpoint.GetLocalUserName();
  else
    return "";
}

PBoolean IAX2CallProcessor::IncomingMessageOutOfOrder(IAX2FullFrame *f)
{
  /*Check to see if this frame is legitimate - that sequence numbers match. If it is out of
    sequence, we have to drop it, and send a vnak frame */
  switch(sequence.IncomingMessageInOrder(*f)) {
      case IAX2SequenceNumbers::InSequence:
	  break;
      case IAX2SequenceNumbers::SkippedFrame: {
	  PTRACE(4, "Skipped frame, received frame is " << f->GetSequenceInfo().AsString());
	  SendVnakFrame(f);
	  delete f;
	  return PTrue;
      } 
	  break;
      case IAX2SequenceNumbers::RepeatedFrame: {
	  SendAckFrame(f);
	  delete f;
	  return PTrue;
      }
  }
  
  /*The frame is ok, and the order is "good". so leave it alone for later processing */
  return PFalse;
}


#endif // OPAL_IAX2

////////////////////////////////////////////////////////////////////////////////

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 2 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-basic-offset:2
 * End:
 */
