/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Implementation of the IAX2 extensions to the OpalEndpoint class.
 * There is one instance of this class in the Opal environemnt.
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
 * The processor parts were abstracted out by  by Stephen Cook (sitiveni@gmail.com)
 * and this work was sponsored by the Google summer of code 
 *
 * $Revision: 22710 $
 * $Author: dereksmithies $
 * $Date: 2009-05-25 03:17:57 +0000 (Mon, 25 May 2009) $
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_IAX2

#include <typeinfo>

#ifdef P_USE_PRAGMA
#pragma implementation "processor.h"
#endif

#include <iax2/processor.h>
#include <iax2/causecode.h>
#include <iax2/frame.h>
#include <iax2/iax2con.h>
#include <iax2/iax2ep.h>
#include <iax2/iax2medstrm.h>
#include <iax2/ies.h>
#include <iax2/sound.h>
#include <iax2/transmit.h>

#define new PNEW

IAX2WaitingForAck::IAX2WaitingForAck()
{
  ZeroValues();
}

void IAX2WaitingForAck::Assign(IAX2FullFrame *f, ResponseToAck _response)
{
  timeStamp = f->GetTimeStamp();
  seqNo     = f->GetSequenceInfo().InSeqNo();
  response  = _response;
  PTRACE(4, "MatchingAck\tIs looking for " << timeStamp << " and " << seqNo << " to do " << GetResponseAsString());
}

void IAX2WaitingForAck::ZeroValues()
{
  timeStamp = 0;
  seqNo = 0;
  //     response = sendNothing;
}

PBoolean IAX2WaitingForAck::MatchingAckPacket(IAX2FullFrame *f)
{
  PTRACE(4, "MatchingAck\tCompare " << timeStamp << " and " << seqNo);
  if (f->GetTimeStamp() != timeStamp) {
    PTRACE(4, "MatchingAck\tTimstamps differ");
    return PFalse;
  }

  if (f->GetSequenceInfo().OutSeqNo() != seqNo) {
    PTRACE(4, "MatchingAck\tOut seqnos differ");    
    return PFalse;
  }

  return PTrue;
}

void IAX2WaitingForAck::PrintOn(ostream & strm) const
{
  strm << "time " << timeStamp << "    seq " << seqNo << "     " << GetResponseAsString();
}

PString IAX2WaitingForAck::GetResponseAsString() const
{
  switch(response) {
  case RingingAcked :  return PString("Received acknowledgement of a Ringing message");
  case AcceptAcked  :  return PString("Received acknowledgement of a Accept message");
  case AuthRepAcked :  return PString("Received acknowledgement of a AuthRep message");
  case AnswerAcked  :  return PString("Received acknowledgement of a Answer message");
  default:;
  }
  
  return PString("Undefined response code of ") + PString((int)response);
}

////////////////////////////////////////////////////////////////////////////////

IAX2Processor::IAX2Processor(IAX2EndPoint &ep)
  : PThread(1000, NoAutoDeleteThread, NormalPriority, "IAX2 Processor"),
    endpoint(ep)
{
  endThread = PFalse;
  
  remote.SetDestCallNumber(0);
  remote.SetRemoteAddress(0);
  remote.SetRemotePort(endpoint.ListenPortNumber());
  
  nextTask.ZeroValues();
  noResponseTimer.SetNotifier(PCREATE_NOTIFIER(OnNoResponseTimeoutStart));
  
  specialPackets = PFalse;

  currentSoundTimeStamp = 0;
}

IAX2Processor::~IAX2Processor()
{
  PTRACE(5, "IAX2CallProcessor DESTRUCTOR");

  StopNoResponseTimer();
  
  Terminate();
  WaitForTermination(10000);

  frameList.AllowDeleteObjects();
}

void IAX2Processor::SetCallToken(const PString & newToken) 
{
  if (!IsHandlingSpecialPackets())
    SetThreadName("Proc" + newToken);

  callToken = newToken;
} 

PString IAX2Processor::GetCallToken()
{ 
  return callToken;
}

void IAX2Processor::Main()
{
  PTRACE(3, "Start of iax2 processing thread");
  PString name = GetThreadName();
  if (IsHandlingSpecialPackets())
    SetThreadName("Special Iax packets");

  while(endThread == PFalse) {
    activate.Wait();
    ProcessLists();
  }
  ProcessLists();
    
  PTRACE(3, "End of iax connection processing");
}

PBoolean IAX2Processor::IsStatusQueryEthernetFrame(IAX2Frame *frame)
{
  if (!PIsDescendant(frame, IAX2FullFrame))
    return PFalse;
   
  IAX2FullFrame *f = (IAX2FullFrame *)frame;
  if (f->GetFrameType() != IAX2FullFrame::iax2ProtocolType)
    return PFalse;
   
  PINDEX subClass = f->GetSubClass();
   
  if (subClass == IAX2FullFrameProtocol::cmdLagRq) {
    PTRACE(4, "Special packet of  lagrq to process");
    return PTrue;
  }
   
  if (subClass == IAX2FullFrameProtocol::cmdPing) {
    PTRACE(4, "Special packet of Ping to process");
    return PTrue;
  }
  return PFalse;
}

void IAX2Processor::IncomingEthernetFrame(IAX2Frame *frame)
{
  if (endThread) {
    PTRACE(3, "IAX2Con\t***** incoming frame during termination " << frame->IdString());
    // snuck in here during termination. may be an ack for hangup or other re-transmitted frames
    IAX2Frame *af = frame->BuildAppropriateFrameType(GetEncryptionInfo());
    if (af != NULL) {
      endpoint.transmitter->PurgeMatchingFullFrames(af);
      delete af;
    }
    delete frame;
  } else {
    frameList.AddNewFrame(frame);  
    CleanPendingLists();    
  } 
} 

void IAX2Processor::StartNoResponseTimer(PINDEX msToWait) 
{
  if (msToWait == 0)
    msToWait = NoResponseTimePeriod;
  
  noResponseTimer = PTimeInterval(msToWait); 
}

void IAX2Processor::OnNoResponseTimeoutStart(PTimer &, INT)
{
  //call sub class to alert that there was a timeout for a response from the server
  OnNoResponseTimeout();
}

void IAX2Processor::Activate()
{
  activate.Signal();
}

void IAX2Processor::Terminate()
{
  endThread = PTrue;
  if (IsSuspended())
    Resume();

  PTRACE(4, "IAX2\tProcessor has been directed to end. " 
	 << (IsTerminated() ? "Has already ended" : "So end now."));
  
  Activate();
}

PBoolean IAX2Processor::ProcessOneIncomingEthernetFrame()
{  
  IAX2Frame *frame = frameList.GetLastFrame();
  if (frame == NULL) {
    return PFalse;
  }
  
  //check the frame has not already been built
  if (!PIsDescendant(frame, IAX2MiniFrame) && !PIsDescendant(frame, IAX2FullFrame)) {
    PTRACE(5, "Procesor\tUnknown  incoming frame " << frame->IdString());
    IAX2Frame *af = frame->BuildAppropriateFrameType(encryption);
    delete frame;
    
    if (af == NULL)
      return PTrue;  
      
    frame = af;  
  }  
  
  if (PIsDescendant(frame, IAX2MiniFrame)) {
    PTRACE(5, "IaxConnection\tIncoming mini frame" << frame->IdString());
    ProcessNetworkFrame((IAX2MiniFrame *)frame);
    return PTrue;
  }
  
  IAX2FullFrame *f = (IAX2FullFrame *) frame;
  PTRACE(5, "Processor\tFullFrame incoming frame " << frame->IdString());

  if (IncomingMessageOutOfOrder(f))
    return PTrue;

  // sequence numbers are Ok. Good.
  endpoint.transmitter->PurgeMatchingFullFrames(f);
  
  IncControlFramesRcvd();

  if (remote.DestCallNumber() == 0) {
    PTRACE(3, "Processor\tSet Destination call number to " << frame->GetRemoteInfo().SourceCallNumber());
    remote.SetDestCallNumber(frame->GetRemoteInfo().SourceCallNumber());
  }
  
  ProcessFullFrame(*f);
 
  if (f != NULL)
    delete f;

  return PTrue;   /*There could be more frames to process. */
}

void IAX2Processor::TransmitFrameNow(IAX2Frame *src)
{
  if (!src->EncryptContents(encryption)) {
    PTRACE(3, "Processor\tEncryption failed. Delete this frame " << *src);
    delete src;
    return;
  }
  endpoint.transmitter->SendFrame(src);
}

void IAX2Processor::TransmitFrameToRemoteEndpoint(IAX2FullFrameProtocol *src)
{
  src->WriteIeAsBinaryData();
  TransmitFrameToRemoteEndpoint((IAX2Frame *)src);
}

void IAX2Processor::TransmitFrameToRemoteEndpoint(IAX2Frame *src)
{
  PTRACE(5, "Processor\tSend frame " << src->GetClass() << " " << src->IdString() << " to remote endpoint");
  if (src->IsFullFrame()) {
    sequence.MassageSequenceForSending(*(IAX2FullFrame*)src);
    IncControlFramesSent();
  }
  TransmitFrameNow(src);
}

void IAX2Processor::TransmitFrameToRemoteEndpoint(IAX2FullFrame *src, 
						  IAX2WaitingForAck::ResponseToAck response)
{
  sequence.MassageSequenceForSending(*(IAX2FullFrame*)src);
  IncControlFramesSent();
  nextTask.Assign(src, response);
  TransmitFrameNow(src);
}

PBoolean IAX2Processor::Authenticate(IAX2FullFrameProtocol *reply, PString & password)
{
  PBoolean processed = PFalse;
  IAX2IeAuthMethods ie(ieData.authMethods);
  
  if (ie.IsMd5Authentication()) {
    PTRACE(5, "Processor\tMD5 Authentiction yes, make reply up");
    IAX2IeMd5Result *res = new IAX2IeMd5Result(ieData.challenge, password);
    reply->AppendIe(res);
    processed = PTrue;
    encryption.SetChallengeKey(ieData.challenge);
    encryption.SetEncryptionKey(password);
  } else  {
    if (ie.IsPlainTextAuthentication()) {
      /*TODO: in the future we might want a policy of only
	allowing md5 passwords.  This would make
	injecting plain auth IEs useless.*/
      reply->AppendIe(new IAX2IePassword(password));
      processed = PTrue;
    } else 
      if (ie.IsRsaAuthentication()) {
	PTRACE(4, "Processor\tDO NOT handle RSA authentication ");
	reply->SetSubClass(IAX2FullFrameProtocol::cmdInval);
	processed = PTrue;
      }
  }
  
  if (ieData.encryptionMethods == IAX2IeEncryption::encryptAes128) {
    PTRACE(4, "Processor\tEnable AES 128 encryption");
    encryption.SetEncryptionOn();
    reply->AppendIe(new IAX2IeEncryption);
  }

  return processed;
}

void IAX2Processor::SendAckFrame(IAX2FullFrame *inReplyTo)
{
  PTRACE(5, "Processor\tSend an ack frame in reply" );
  PTRACE(5, "Processor\tIn reply to " << *inReplyTo);
  
  //callIrrelevant is used because if a call ends we still want the remote
  //endpoint to get the acknowledgment of the call ending!
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdAck, inReplyTo, 
    IAX2FullFrame::callIrrelevant);
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2Processor::SendVnakFrame(IAX2FullFrame *inReplyTo)
{
  PTRACE(4, "Processor\tSend Vnak frame in reply" );
  PTRACE(4, "Processor\tIn reply to " << *inReplyTo);
  
  //callIrrelevant is used because if a call ends we still want the remote
  //endpoint to get the acknowledgment of the call ending!
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdVnak, inReplyTo, 
    IAX2FullFrame::callIrrelevant);
  TransmitFrameToRemoteEndpoint(f);
}


void IAX2Processor::SendUnsupportedFrame(IAX2FullFrame *inReplyTo)
{
  PTRACE(4, "Processor\tSend an unsupported frame in reply" );
  PTRACE(4, "Processor\tIn reply to " << *inReplyTo);
  
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdUnsupport, inReplyTo, 
    IAX2FullFrame::callIrrelevant);
  f->AppendIe(new IAX2IeIaxUnknown((BYTE)inReplyTo->GetSubClass()));
  TransmitFrameToRemoteEndpoint(f);
  delete inReplyTo;
}

PBoolean IAX2Processor::ProcessNetworkFrame(IAX2FullFrameProtocol * src)
{
  switch(src->GetSubClass()) {
  case IAX2FullFrameProtocol::cmdLagRq:
    ProcessIaxCmdLagRq(src);
    break;
  case IAX2FullFrameProtocol::cmdLagRp:
    ProcessIaxCmdLagRp(src);
    break;
  case IAX2FullFrameProtocol::cmdVnak:
    ProcessIaxCmdVnak(src);
    break;
  case IAX2FullFrameProtocol::cmdPing:
    ProcessIaxCmdPing(src);
    break;
  case IAX2FullFrameProtocol::cmdPong:
    ProcessIaxCmdPong(src);
    break;
  default:
    return PFalse;
  }
  
  return PTrue;
}

void IAX2Processor::ProcessIaxCmdPing(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "ProcessIaxCmdPing(IAX2FullFrameProtocol *src)");
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdPong, src, IAX2FullFrame::callIrrelevant);
  delete src;
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2Processor::ProcessIaxCmdPong(IAX2FullFrameProtocol *src)
{
  SendAckFrame(src);  
  delete src;
  PTRACE(4, "ProcessIaxCmdPong(IAX2FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdLagRq(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "ProcessIaxCmdLagRq(IAX2FullFrameProtocol *src)");
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdLagRp, src, IAX2FullFrame::callIrrelevant);
  delete src;
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2Processor::ProcessIaxCmdLagRp(IAX2FullFrameProtocol *src)
{
  PTRACE(4, "ProcessIaxCmdLagRp(IAX2FullFrameProtocol *src)");
  SendAckFrame(src);
  delete src;
  PTRACE(4, "Process\tRound trip lag time is " << (IAX2Frame::CalcTimeStamp(callStartTick) - src->GetTimeStamp()));
}

void IAX2Processor::ProcessIaxCmdVnak(IAX2FullFrameProtocol *src)
{
    PTRACE(4, "ProcessIaxCmdVnak\tFrames recieved out of order.");
    endpoint.transmitter->SendVnakRequestedFrames(*src);
    delete src;
}


void IAX2Processor::ReportLists(PString & answer)
{
  answer= PString(" Incoming size ") + PString(frameList.GetSize());
}


#endif // OPAL_IAX2
