/*
 *
 * Inter Asterisk Exchange 2
 * 
 * The core routine which determines the processing of packets for one call.
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
 * $Revision: 21293 $
 * $Author: rjongbloed $
 * $Date: 2008-10-12 23:24:41 +0000 (Sun, 12 Oct 2008) $
 */

#ifndef OPAL_IAX2_CALLPROCESSOR_H
#define OPAL_IAX2_CALLPROCESSOR_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if OPAL_IAX2

#include <opal/connection.h>

#include <iax2/processor.h>
#include <iax2/frame.h>
#include <iax2/iedata.h>
#include <iax2/remote.h>
#include <iax2/safestrings.h>
#include <iax2/sound.h>

class IAX2Connection;

/**This class does the work of processing the lists of IAX packets (in and out) 
   that are associated with each call. There is one IAX2CallProcessor per connection.
   */
class IAX2CallProcessor : public IAX2Processor
{
  PCLASSINFO(IAX2CallProcessor, IAX2Processor);
  
 public:
  
  /**Construct this class */
  IAX2CallProcessor(IAX2EndPoint & ep);

  /**Destructor */
  virtual ~IAX2CallProcessor(); 

  /**Assign a pointer to the connection class to process, and starts thread */
  void AssignConnection(IAX2Connection * _con);
  
  /**Handle a sound packet received from the sound device.     
     Now onsend this to the remote endpoint. */
  void PutSoundPacketToNetwork(PBYTEArray *sund);
  
  /**Get the iax2 encryption info */
  IAX2Encryption & GetEncryptionInfo() { return encryption; }

  /**Call back from the IAX2Connection class */
  virtual void Release(OpalConnection::CallEndReason releaseReason = OpalConnection::EndedByLocalUser);

  /**From the IAX2Connection class. CAlling this sends a hangup frame */
  void ClearCall(OpalConnection::CallEndReason releaseReason = OpalConnection::EndedByLocalUser);

  /**Call back from the IAX2Connection class. This indicates the
   IAX2Connection class is in the final stages of destruction. At this
   point, we can terminate the thread that processors all iax
   packets */
  virtual void OnReleased();
  
  /** Ask this IAX2CallProcessor to send dtmf to the remote
      endpoint. The dtmf is placed on a queue, ready for transmission
      in fullframes of type dtmf. */
  void SendDtmf(const PString & dtmfs);

  /** Ask this IAX2CallProcessor to send text to the remote endpoint. The text
      is placed on a queue, ready for transmission in fullframes of type
      text. */
  void SendText(const PString & text);

  /**Start an outgoing connection.
     This function will initiate the connection to the remote entity, for
     example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.
     
     The behaviour at the opal level is pure. Here, the method is defined.
  */
  virtual PBoolean SetUpConnection();

  /**Return PTrue if the remote info in the frame matches the remote info in
     this connection */
  PBoolean Matches(IAX2Frame *frame) { return remote == (frame->GetRemoteInfo()); }
  
  /**A method to cause some of the values in this class to be formatted
     into a printable stream */
  virtual void PrintOn(ostream & strm) const;
  
  /**Invoked by the User interface, which causes the statistics (count of
     in/out packets) to be printed*/
  void ReportStatistics();  

  /**Return PTrue if the arg matches the source call number for this connection */
  PBoolean MatchingLocalCallNumber(PINDEX compare) { return (compare == remote.SourceCallNumber()); }  
  
  /**Get the bit pattern of the selected codec */
  unsigned short GetSelectedCodec() { return (unsigned short) selectedCodec; }
  
  /**Send appropriate packets to the remote node to indicate we will accept
     this call.  Note that this method is called from the endpoint thread,
     (not this IAX2Connection's thread). In other words, the application calls
     this method.*/
  void AcceptIncomingCall();

  /**Indicate to remote endpoint an alert is in progress.  If this is
     an incoming connection and the AnswerCallResponse is in a
     AnswerCallDeferred or AnswerCallPending state, then this function
     is used to indicate to that endpoint that an alert is in
     progress. This is usually due to another connection which is in
     the call (the B party) has received an OnAlerting() indicating
     that its remoteendpoint is "ringing".

     The default behaviour is pure.
  */
  virtual PBoolean SetAlerting(
         const PString & calleeName,   /// Name of endpoint being alerted.
         PBoolean withMedia                /// Open media with alerting
         ) ;

  /**Cause this thread to hangup the current call, but not die. Death
     will come soon though. The argument is placed in the iax2 hangup
     packet as the cause string.*/
  void Hangup(PString messageToSend);

  /**Report the status of the flag callEndingNow, which indicates if
     the call is terminating under iax2 control  */
  PBoolean IsCallTerminating() { return callStatus & callTerminating; }
  
  /**Put the remote connection on hold*/
  void SendHold();

  /**Take the remote connection of hold*/
  void SendHoldRelease();
  
  /**Set the username for when we connect to a remote node
     we use it as authentication.  Note this must only be
     used before the main thread is started.  This is optional
     because some servers do not required authentication, also
     if it is not set then the default iax2Ep username 
     will be used instead.*/
  void SetUserName(PString & inUserName) { userName = inUserName; };
  
  /**Get the username*/
  PString GetUserName() const;
  
  /**Set the password for when we connect to a remote node
     we use it as authentication.  Note this must only be
     used before the main thread is started.  This is optional
     because some servers do not required authentication, also
     if it is not set then the default iax2Ep password 
     will be used instead.*/
  void SetPassword(PString & inPassword) { password = inPassword; };
  
  /**Get the password*/
  PString GetPassword() const { return password; };
  
  /**Send a transfer request, to transfer the remote party to the specified
     number and optionally with a context*/
  void SendTransfer(
    const PString & calledNumber,
    const PString & calledContext = PString::Empty());  

  /**Set up the acceptable time (in milliseconds) to wait between
     doing status checks. */
  void StartStatusCheckTimer(PINDEX msToWait = 10000 /*!< time between 
				      status checks, default = 10 seconds*/);
  
  /**Test the sequence number of the incoming frame. This is only
     valid for handling a call. If the message is outof order, the
     supplied fullframe is deleted.

  @return true if the frame is out of order, which deletes the supplied frame
  @return false, and does not destroy the supplied frame*/
  virtual PBoolean IncomingMessageOutOfOrder(IAX2FullFrame *ff);

  /** Advise the other end that we have picked up the phone. This
      method is public, as it is called by the IAX2Connection class,
      in response to the user accepting an incoming call. */
  void SendAnswerMessageToRemoteNode();

 protected:
  
  /**The connection class we are charged with running. */
  IAX2Connection * con;

  /**@name Internal, protected methods, which are invoked only by this
     thread*/
  //@{
  /** Test the value supplied in the format Ie is compatible.*/
  PBoolean RemoteSelectedCodecOk();
 
  /** Check to see if there is an outstanding request to send a hangup
      frame. This needs to be done in two places, so we use a routine to see
      if need to send a hanup frame.*/
  void CheckForHangupMessages();
 
  /**Internal method to process an incoming network frame of type IAX2Frame */
  void ProcessNetworkFrame(IAX2Frame * src);
  
  /**Internal method to process an incoming network frame of type
     IAX2MiniFrame */
  void ProcessNetworkFrame(IAX2MiniFrame * src);
  
  /**Internal method to process an incoming network frame of type
    IAX2FullFrame */
  void ProcessNetworkFrame(IAX2FullFrame * src);
  
  /**Internal method to process an incoming network frame of type
    IAX2FullFrameDtmf */
  void ProcessNetworkFrame(IAX2FullFrameDtmf * src);
  
  /**Internal method to process an incoming network frame of type
     IAX2FullFrameVoice */
  void ProcessNetworkFrame(IAX2FullFrameVoice * src);
  
  /**Internal method to process an incoming network frame of type
     IAX2FullFrameVideo */
  void ProcessNetworkFrame(IAX2FullFrameVideo * src);
  
  /**Internal method to process an incoming network frame of type
    IAX2FullFrameSessionControl */
  void ProcessNetworkFrame(IAX2FullFrameSessionControl * src);
  
  /**Internal method to process an incoming network frame of type
    IAX2FullFrameNull */
  void ProcessNetworkFrame(IAX2FullFrameNull * src);
  
  /**Internal method to process an incoming network frame of type
     IAX2FullFrameProtocol.
     
  A frame of FullFrameProtocol type is labelled as AST_FRAME_IAX in the asterisk souces,
  It will contain 0, 1, 2 or more Information Elements (Ie) in the data section.*/
  virtual PBoolean ProcessNetworkFrame(IAX2FullFrameProtocol * src);
  
  /**Internal method to process an incoming network frame of type
     IAX2FullFrameText */
  void ProcessNetworkFrame(IAX2FullFrameText * src);
  
  /**Internal method to process an incoming network frame of type
     IAX2FullFrameImage */
  void ProcessNetworkFrame(IAX2FullFrameImage * src);
  
  /**Internal method to process an incoming network frame of type
     IAX2FullFrameHtml */
  void ProcessNetworkFrame(IAX2FullFrameHtml * src);
  
  /**Internal method to process an incoming network frame of type
     IAX2FullFrameCng */
  void ProcessNetworkFrame(IAX2FullFrameCng * src);
  
  /**Go through the three lists for incoming data (ethernet/sound/UI
     commands.  */
  virtual void ProcessLists();
    
  /**Make a call to a remote node*/
  void ConnectToRemoteNode(PString & destination);
  
  /**Cause the dtmf full frames to go out for this dtmf character*/
  void SendDtmfMessage(char message);
  
  /**Cause the text full frames to go out for this text message*/
  void SendTextMessage(PString & message);

  /**Cause a sound frame (which is full or mini) to be sent. 
     The data in the array is already compressed. */
  void SendSoundMessage(PBYTEArray *sound);
  
  /**Sends a transfer message ONLY if doTransfer is PTrue*/
  void SendTransferMessage();
  
  /**Send a message to put the remote connection on hold*/
  void SendQuelchMessage();
  
  /**Send a message to take the remote connection off hold*/
  void SendUnQuelchMessage();
  
  /**Increment the count of audio frames sent*/
  void IncAudioFramesSent()   { ++audioFramesSent; }
  
  /**Increment the count of audio frames received*/
  void IncAudioFramesRcvd()   { ++audioFramesRcvd; }
  
  /**Increment the count of video frames sent*/
  void IncVideoFramesSent()   { ++videoFramesSent; }
  
  /**Increment the count of video frames received*/
  void IncVideoFramesRcvd()   { ++videoFramesRcvd; }
  
  /**A callback which is used to indicate the remote party 
     has accepted our call. Media can flow now*/
  void RemoteNodeHasAnswered();
  
  /**A stop sounds packet has been received, which means we have moved from
     waiting for the remote person to answer, to they have answered and media
     can flow in both directions*/
  void CallStopSounds();
  
  /**A callback which is used to indicate that the remote party has sent us
     a hook flash message (i.e, their hook was flashed) */
  void ReceivedHookFlash();
  
  /**A callback which is used to indicate that the remote party has sent us
     a message stating they are busy. */
  void RemoteNodeIsBusy();
  
  /**Process the audio data portions of the Frame argument, which may be a
     MiniFrame or FullFrame */
  void ProcessIncomingAudioFrame(IAX2Frame *newFrame);
  
  /**Process the video data portions of the Frame argument, which may be a
     MiniFrame or FullFrame */
  void ProcessIncomingVideoFrame(IAX2Frame *newFrame);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Create a
     new call  */
  void ProcessIaxCmdNew(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is
     Acknowledge a Reliably sent full frame  */
  void ProcessIaxCmdAck(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Request
     to terminate this call  */
  void ProcessIaxCmdHangup(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Refuse
     to accept this call. May happen if authentication faile  */
  void ProcessIaxCmdReject(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Allow
     this call to procee  */
  void ProcessIaxCmdAccept(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Ask
     remote end to supply authentication  */
  void ProcessIaxCmdAuthReq(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is A reply,
     that contains authentication  */
  void ProcessIaxCmdAuthRep(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Destroy
     this call immediately  */
  void ProcessIaxCmdInval(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Request
     status of a dialplan entry  */
  void ProcessIaxCmdDpReq(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Request
     status of a dialplan entry  */
  void ProcessIaxCmdDpRep(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Request
     a dial on channel brought up TBD  */
  void ProcessIaxCmdDial(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Transfer
     Request  */
  void ProcessIaxCmdTxreq(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Transfer
     Connect  */
  void ProcessIaxCmdTxcnt(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Transfer
     Accepted  */
  void ProcessIaxCmdTxacc(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Transfer
     ready  */
  void ProcessIaxCmdTxready(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Transfer
     release  */
  void ProcessIaxCmdTxrel(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Transfer
     reject  */
  void ProcessIaxCmdTxrej(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Stop
     audio/video transmission  */
  void ProcessIaxCmdQuelch(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Resume
     audio/video transmission  */
  void ProcessIaxCmdUnquelch(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Paging
     description  */
  void ProcessIaxCmdPage(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is
     Stand-alone message waiting indicator  */
  void ProcessIaxCmdMwi(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is
     Unsupported message received  */
  void ProcessIaxCmdUnsupport(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Request
     remote transfer  */
  void ProcessIaxCmdTransfer(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is
     Provision device  */
  void ProcessIaxCmdProvision(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Download
     firmware  */
  void ProcessIaxCmdFwDownl(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Firmware
     Data  */
  void ProcessIaxCmdFwData(IAX2FullFrameProtocol *src);
  
  /**Count of the number of sound frames sent */
  PAtomicInteger audioFramesSent;
  
  /**Count of the number of sound frames received */
  PAtomicInteger audioFramesRcvd;
  
  /**Count of the number of video frames sent */
  PAtomicInteger videoFramesSent;
  
  /**Count of the number of video frames received */
  PAtomicInteger videoFramesRcvd;
  
  /**Phone number of the remote  endpoint */
  SafeString remotePhoneNumber;
  
  /**Array of remote node we have to make a call to */
  SafeStrings callList;
  
  /**Contains the concatanation of the dtmf we have to send to the remote
   endpoint.  This string is sent a character at a time, one DTMF frame per
   character. This list is threadsafe. */
  SafeString dtmfText;

  /**Array of the text we have to send to the remote endpoint. This array is
     threadsafe. */
  SafeStrings textList;

  /**Array of received dtmf characters (These have come from the network)*/
  SafeStrings dtmfNetworkList;

  /**Array of requests to end this current call */
  SafeStrings hangList;
  
  /**Flag to indicate we have to send hold call*/
  PBoolean holdCall;
  
  /**Flag to indicate we have to send hold release*/
  PBoolean holdReleaseCall;
  
  /**Array of sound packets read from the audio device, and is about
     to be transmitted to the remote node */
  IAX2SoundList   soundWaitingForTransmission;
  
  /**
   * This is the current state of the sound buffer for the packets
   * that are being recieved.  It is used so if a bad event occurs like
   * the buffer gets to big or the buffer gets to small then we can reduce
   * the amount of audiable disturbances. */
  enum SoundBufferState {
    BufferToSmall, ///We need more sound packets to come in
    Normal, ///Everything is functioning ok
    BufferToBig ///We need the buffer size to be reduced there is too much latency
  };
  
  /**This holds the current state for the sound recieving buffer*/
  SoundBufferState soundBufferState;
  
  /**This is the timestamp of the last received full frame, which is used to
     reconstruct the timestamp of received MiniFrames */
  PINDEX lastFullFrameTimeStamp;
    
  /**Flag to indicate we are ready for audio to flow */
  PBoolean audioCanFlow;

  /** Bitmask of FullFrameVoice::AudioSc values to specify which codec is
     used*/
  unsigned int selectedCodec;
  
  /** bit mask of the different flags to indicate call status*/
  enum CallStatus {
    callNewed      =  1 << 0,   /*!< we have received a new packet to set this call up.                         */
    callSentRinging = 1 << 1,   /*!< we have sent a packet to indicate that the phone is ringing at our end     */
    callRegistered =  1 << 2,   /*!< registration with a remote asterisk server has been approved               */
    callAuthorised =  1 << 3,   /*!< we are waiting on password authentication at the remote endpoint           */
    callAccepted   =  1 << 4,   /*!< call has been accepted, which means that the new request has been approved */
    callRinging    =  1 << 5,   /*!< Remote end has sent us advice the phone is ringing, awaiting answer        */
    callAnswered   =  1 << 6,   /*!< call setup complete, now do audio                                          */
    callTerminating = 1 << 7    /*!< Flag to indicate call is closing down after iax2 call end commands         */
  };
  
  /** Contains the bits stating what is happening in the call */
  unsigned short callStatus;
  
  /** Mark call status as having sent a iaxcmdRinging packet */
  void SetCallSentRinging(PBoolean newValue = PTrue) 
    { if (newValue) callStatus |= callSentRinging; else callStatus &= ~callSentRinging; }
  
  /** Mark call status as having received a new packet */
  void SetCallNewed(PBoolean newValue = PTrue) 
    { if (newValue) callStatus |= callNewed; else callStatus &= ~callNewed; }
  
  /** Mark call status Registered (argument determines flag status) */
  void SetCallRegistered(PBoolean newValue = PTrue) 
    { if (newValue) callStatus |= callRegistered; else callStatus &= ~callRegistered; }
  
  /** Mark call status Authorised (argument determines flag status) */
  void SetCallAuthorised(PBoolean newValue = PTrue) 
    { if (newValue) callStatus |= callAuthorised; else callStatus &= ~callAuthorised; }
  
  /** Mark call status Accepted (argument determines flag status) */
  void SetCallAccepted(PBoolean newValue = PTrue) 
    { if (newValue) callStatus |= callAccepted; else callStatus &= ~callAccepted; }
  
  /** Mark call status Ringing (argument determines flag status) */
  void SetCallRinging(PBoolean newValue = PTrue) 
    { if (newValue) callStatus |= callRinging; else callStatus &= ~callRinging; }
  
  /** Mark call status Answered (argument determines flag status) */
  void SetCallAnswered(PBoolean newValue = PTrue) 
    { if (newValue) callStatus |= callAnswered; else callStatus &= ~callAnswered; }

  /** Mark call status as terminated (is processing IAX2 hangup packets etc)*/
  void SetCallTerminating(PBoolean newValue = PTrue) 
    { if (newValue) callStatus |= callTerminating; else callStatus &= ~callTerminating; }
  
  /** See if any of the flag bits are on, which indicate this call is actually active */
  PBoolean IsCallHappening() { return callStatus > 0; }
  
  /** Get marker to indicate that some packets have flowed etc for this
     call  */
  PBoolean IsCallNewed() { return callStatus & callNewed; }
  
  /** Get marker to indicate that we are waiting on the ack for the
      iaxcommandringing packet we sent  */
  PBoolean IsCallSentRinging() { return callStatus & callSentRinging; }
  
  /** Get the current value of the call status flag callRegistered */
  PBoolean IsCallRegistered() { return callStatus & callRegistered; }
  
  /** Get the current value of the call status flag callAuthorised */
  PBoolean IsCallAuthorised() { return callStatus & callAuthorised; }
  
  /** Get the current value of the call status flag callAccepted */
  PBoolean IsCallAccepted() { return callStatus & callAccepted; }
  
  /** Get the current value of the call status flag callRinging */
  PBoolean IsCallRinging() { return callStatus & callRinging; }
  
  /** Get the current value of the call status flag callAnswered */
  PBoolean IsCallAnswered() { return callStatus & callAnswered; }
       
#ifdef DOC_PLUS_PLUS
  /**A pwlib callback function to invoke another status check on the
       other endpoint 

       This method runs in a separate thread to the
       heart of the Connection.  It is threadsafe, cause each of the
       elements in Connection (that are touched) are thread safe */
  void OnStatusCheck(PTimer &, INT);
#else
  PDECLARE_NOTIFIER(PTimer, IAX2CallProcessor, OnStatusCheck);
#endif
  
  /**Code to send a PING and a LAGRQ packet to the remote endpoint */
  void DoStatusCheck();
  
  /** we have received a message that the remote node is ringing. Now
      wait for the remote user to answer. */
  void RemoteNodeIsRinging();

  /** We have told the remote node that our phone is ringing. they
      have acked this message.  Now, we advise opal that our phone
      "should" be ringing, and does opal want to accept our call. */
  void RingingWasAcked();

  /** We have received an ack message from the remote node. The ack
      message was sent in response to our Answer message. Since the
      Answer message has been acked, we have to regard this call as
      Established();. */
  void AnswerWasAcked();

  /**Flag to indicate if we are waiting on the first full frame of
     media (voice or video). The arrival of this frame causes the
     IAX2Connection::OnEstablished method to be called. */
  PBoolean firstMediaFrame;

  /**Flag to indicate we have to answer this call (i.e. send a
     FullFrameSessionControl::answer packet). */
  PBoolean answerCallNow;

  /**Flag to indicate we need to do a status query on the other
     end. this means, send a PING and a LAGRQ packet, which happens
     every 10 seconds. The timer, timeStatusCheck controls when this
     happens */
  PBoolean statusCheckOtherEnd;

  /** The timer which is used to do the status check */
  PTimer statusCheckTimer;

  /**The time period, in ms, of each audio frame. It is used when determining
     the appropriate timestamp to go on a packet. */
  PINDEX audioFrameDuration;

  /**The number of bytes from compressing one frame of audio */
  PINDEX audioCompressedBytes;

  /**A flag to indicate we have yet to send an audio frame to remote
     endpoint. If this is on, then the first audio frame sent is a
     full one.  */
  PBoolean audioFramesNotStarted;

  /**If the incoming frame has Information Elements defining remote
     capability, define the list of remote capabilities */
  void CheckForRemoteCapabilities(IAX2FullFrameProtocol *src);
  
  /**A call back when there has been no acknowledgment and the
    timeout peroid has been reached */
  virtual void OnNoResponseTimeout();
  
  /**Process a full frame and respond accordingly to it*/
  virtual void ProcessFullFrame(IAX2FullFrame & fullFrame);
  
  /**Optional username for when we connect to a remote node
     we use it as authentication.  Note this must only be
     set before the main thread is started.*/
  PString userName;
  
  /**Optional password for when we connect to a remote node
     we use it as authentication.  Note this must only be
     set before the main thread is started.*/
  PString password;
  
  /**A mutex to make the transfer related fields atomic*/
  PMutex transferMutex;
  
  /**Whether we want a transfer event to occur or not*/
  PBoolean doTransfer;
  
  /**The number to call for a transfer*/
  PString transferCalledNumber;
  
  /**The context to call for a transfer -- this is optional*/
  PString transferCalledContext;    
};

/////////////////////////////////////////////////////////////////////////

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-basic-offset:2
 * End:
 */


#endif // OPAL_IAX2

#endif // OPAL_IAX2_CALLPROCESSOR_H
