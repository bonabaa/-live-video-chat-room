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

#ifndef OPAL_IAX2_PROCESSOR_H
#define OPAL_IAX2_PROCESSOR_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if OPAL_IAX2

#include <opal/connection.h>

#include <iax2/frame.h>
#include <iax2/iedata.h>
#include <iax2/remote.h>
#include <iax2/safestrings.h>
#include <iax2/sound.h>

class IAX2EndPoint;
class IAX2Connection;
class IAX2ThreadHelper;

////////////////////////////////////////////////////////////////////////////////
/**This class defines what the processor is to do on receiving an ack
   for a full frame.  Thus, the processor sends a full frame (say
   Accept) and defines an action (using this class) to carry out on
   receiving the ack for the Accept.

   Essentially, this class provides us a means of "knowing" the other
   end has acted on the sent full frame. The action to do could be as
   simple as saying, "other end has accepted our Accept packet"

   It also helps us to move on in the call setup phase. On receipt of
   one particular ack packet, we send the next packet in the call
   process.  Thus, on receipt of a ack for a timestamp of X and oseqno
   of Y, we send this packet.
   
   The outgoing IAX2FullFrame has an inSeqNo, which must match the
   outSeqNo of the received ack frame.
   
   The sub classes if appropriate will need to request a call number from the
   endpoint.  This class will automatically check if the source call number
   has been set and if so then will release that number from the endpoint. 
*/
class IAX2WaitingForAck : public PObject
{
  PCLASSINFO(IAX2WaitingForAck, PObject);

 public:
  /**The action to do on receiving the specified ack */
  enum ResponseToAck {
    RingingAcked   = 0,  /*!< Processing acknowledgment to Remote end is ringing  message */ 
    AcceptAcked    = 1,  /*!< Processing acknowledgment to accept  message                */ 
    AuthRepAcked   = 2,  /*!< Processing acknowledgment to AuthRep message                */ 
    AnswerAcked    = 3   /*!< Processing acknowledgment to Answer message                 */ 
  };
  
  /**Construct this response message with values that cannot match an incoming ack packet*/
  IAX2WaitingForAck();
  
  /**Assign this response to wait for the specified coords */
  void Assign(IAX2FullFrame *f, ResponseToAck _response);
  
  /**Return true if the supplied ack frame matches the internal coordinates */
  PBoolean MatchingAckPacket(IAX2FullFrame *f);
  
  /**Report the response to carry out */
  ResponseToAck GetResponse() { return response; }
  
  /**Report the internal response code as a string */
  PString GetResponseAsString() const;
  
  /**Pretty print this response to the designated stream*/
  virtual void PrintOn(ostream & strm) const;
  
  /**Initialise this to no response, and never to match */
  void ZeroValues();
  
 private:
  /**Timestamp of the ack packet we are looking for */
  DWORD timeStamp;
  
  /**OutSeqNo of the ack packet we are looking for */
  PINDEX seqNo;
  
  /**Do this action on finding a matching ACK packet */
  ResponseToAck response;
};

////////////////////////////////////////////////////////////////////////////////
/** This class is an abstract base class for iax2 processors.  This class is
    responsible for handling all the iax2 protocol command messages.
 
    It provides the base structure for two different processor classes,
    a)registration type commands and b)commands specific to one call.
 
    The unique source and destination call number (which are in the IAX2
    frames) are used to determine which processor will handle which incoming
    packet.
 
    Each processor runs in its own thread so as to process incoming packets in
    a timely fashion.
 */
class IAX2Processor : public PThread
{
  PCLASSINFO(IAX2Processor, PObject);
  
 public:
  /**Construct this class */
  IAX2Processor(IAX2EndPoint & ep);

  /**Destructor */
  virtual ~IAX2Processor();
  
  /**Get the sequence number info (inSeqNo and outSeqNo) */
  IAX2SequenceNumbers & GetSequenceInfo() { return sequence; }
  
  /**Get the IAX2 encryption info */
  IAX2Encryption & GetEncryptionInfo() { return encryption; }
  
  /**Handle a received IAX2 frame. This may be a mini frame or full frame */
  void IncomingEthernetFrame (IAX2Frame *frame);
  
  /**A method to cause some of the values in this class to be formatted
     into a printable stream */
  virtual void PrintOn(ostream & strm) const = 0;
  
  /**Access the endpoint class that launched this processor */
  IAX2EndPoint & GetEndPoint() { return endpoint; };
  
   /**Give the call token a value. The call token is the ipaddress of
     the remote node concatented with the remote nodes src
     number. This is guaranteed to be unique.  Sadly, if this
     connection is setting up the call, the callToken is not known
     until receipt of the first packet from the remote node.

     However, if this connection is created in response to a call,
     this connection can determine the callToken on examination of
     that incoming first packet */
  void SetCallToken(const PString & newToken);
  
  /**Return the string that identifies this IAX2Connection instance */
  PString GetCallToken();
  
  /**Get information on IAX2Remote class (remote node address & port + source & dest call number.) */
  IAX2Remote & GetRemoteInfo() { return remote; }
  
  /**Get the call start tick */
  const PTimeInterval & GetCallStartTick() { return callStartTick; }
  
  /**The worker method of this thread. In here, all incoming frames (for this call)
     are handled.
  */
  void Main();
  
  /**Test to see if it is a status query type IAX2 frame (eg lagrq) and handle it. If the frame
     is a status query, and it is handled, return PTrue */
  PBoolean IsStatusQueryEthernetFrame(IAX2Frame *frame);
  
  /**Set the flag to indicate if we are handling specialPackets (those
     packets which are not sent to any particular call) */
  void SetSpecialPackets(PBoolean newValue) { specialPackets = newValue; }
  
  /**Cause this thread to die immediately */
  void Terminate();
  
  /**Cause this thread to come to life, and process events that are
   * pending at IAX2Connection. This method does not start this
   * thread. This method causes this thread to be joggled back into
   * life, after waiting on a PSyncPoint. */
  void Activate();

  /**Test the sequence number of the incoming frame. This is only
     valid for handling a call. If the message is outof order, the
     supplied fullframe is deleted.

  @return true if the frame is out of order, which deletes the supplied frame
  @return false, and does not destroy the supplied frame*/
  virtual PBoolean IncomingMessageOutOfOrder(IAX2FullFrame *ff)= 0;

  /** Report on the contents of the lists waiting for processing */
  void ReportLists(PString & answer);

 protected:
 
  /**Reference to the global variable of this program */
  IAX2EndPoint      & endpoint;
 
  /**Time this connection class was created, which is the call start
     time.  It is reported in Ticks, which is required for millisecond
     accuracy under windows.   */
  PTimeInterval callStartTick;
  
  /**Details on the address of the remote endpoint, and source/dest call numbers */
  IAX2Remote remote;
  
  /** Set the acceptable time (in milliseconds) to wait before giving
      up on this call */
  void StartNoResponseTimer(PINDEX msToWait = 60000);
   
  /** Stop the timer - we have received a reply */
  void StopNoResponseTimer() { noResponseTimer.Stop(); }
  
  /** The timer which is used to test for no reply to our outgoing call setup messages */
  PTimer noResponseTimer;
  
  /**Activate this thread to process all the lists of queued frames */
  void CleanPendingLists() { activate.Signal(); }
  
  /**Action to perform on receiving an ACK packet (which is required
     during call setup phase for receiver */
  IAX2WaitingForAck nextTask;
  
   /**Flag which is used to activate this thread, so all pending tasks/packets are processed */
  PSyncPoint activate;
  
  /**Flag to indicate, end this thread */
  PBoolean endThread;
  
  /**Status of encryption for this processor - by default, no encryption */
  IAX2Encryption encryption;
  
  /**Details on the in/out sequence numbers */
  IAX2SequenceNumbers sequence;
  
  /**Array of frames read from the Receiver for this call */
  IAX2ActiveFrameList frameList;
  
  /**The call token, which uniquely identifies this IAX2CallProcessor, and the
     associated call */
  SafeString callToken;
  
  /**A threaded pure threaded callback for the sub classes of processor */
  virtual void OnNoResponseTimeout() = 0;
  
  /** A defined value which is the maximum time we will wait for an answer to
     our packets */
  enum DefinedNoResponseTimePeriod {
    NoResponseTimePeriod = 5000 /*!< Time (in milliseconds) we will wait */
  };
  
  /**return the flag to indicate if we are handling special packets,
     which are those packets sent to the endpoint (and not related to
     any particular call). */
  PBoolean      IsHandlingSpecialPackets() { return specialPackets; };

  /**Flag to indicate we are handing the special packets, which are
     sent to the endpoint,and not related to any particular call. */
  PBoolean       specialPackets;
  
  /**Go through the three lists for incoming data (ethernet/sound/UI
     commands.  */
  virtual void ProcessLists() = 0;  
  
  /**A pure virtual method that is implemented by sub classes to
     process an incoming full frame*/
  virtual void ProcessFullFrame(IAX2FullFrame & fullFrame) = 0;
  
  /** Process a FullFrameProtocol class, where the sub Class value is Initial
     message, used to measure the round trip time  */
  virtual void ProcessIaxCmdLagRq(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Reply to
     cmdLagrq, which tells us the round trip time  */
  virtual void ProcessIaxCmdLagRp(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is If we
     receive voice before valid first voice frame, send this  */
  virtual void ProcessIaxCmdVnak(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Ping
     request,  */
  virtual void ProcessIaxCmdPing(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is reply to
     a Ping  */
  virtual void ProcessIaxCmdPong(IAX2FullFrameProtocol *src); 
  
  /**remove one frame on the incoming ethernet frame list. If there
     may be more to process, return PTrue. If there are no more to
     process, return PFalse. */
  PBoolean ProcessOneIncomingEthernetFrame();
  
  /**Count of the number of control frames sent */
  PAtomicInteger controlFramesSent;
  
  /**Count of the number of control frames received */
  PAtomicInteger controlFramesRcvd;
  
  /**Increment the count of full frames sent*/
  void IncControlFramesSent() { ++controlFramesSent; }
  
  /**Increment the count of full frames received*/
  void IncControlFramesRcvd() { ++controlFramesRcvd; }
  
  /**A pure virtual method that is implmented by to process 
   * an incoming network frame of type  IAX2MiniFrame */
  virtual void ProcessNetworkFrame(IAX2MiniFrame * src) = 0;  

  /* A frame of FullFrameProtocol type is labelled as AST_FRAME_IAX in the
     asterisk souces, It will contain 0, 1, 2 or more Information Elements
     (Ie) in the data section. This processor is used to do things that are
     common to registration and call handling.

  This method will eat/delete the supplied frame. Return PTrue on success,
  PFalse on failure.*/
  virtual PBoolean ProcessNetworkFrame(IAX2FullFrameProtocol * src);
  
  /**Transmit IAX2Frame to remote endpoint, and then increment send
     count. This calls a method in the Transmitter class. .It is only called
     by the this IAX2CallProcessor class.  */
  void TransmitFrameToRemoteEndpoint(IAX2Frame *src);

  /**Transmit IAX2Frame to remote endpoint, and then increment send
     count. This calls a method in the Transmitter class. .It is only
     called by the this IAX2CallProcessor class. The second parameter
     determines what to do when an ack frame is received for the sent
     frame.  */
  void TransmitFrameToRemoteEndpoint(IAX2FullFrame *src,
             IAX2WaitingForAck::ResponseToAck response  ///<action to do on getting Ack
             );

  /**Transmit IAX2Frame to remote endpoint,. This calls a method in the
     Transmitter class. .It is only called by the this Connection
     class. There is no stats change when this method is called. */
  void TransmitFrameNow(IAX2Frame *src);
  
  /**FullFrameProtocol class needs to have the IE's correctly appended prior to transmission */
  void TransmitFrameToRemoteEndpoint(IAX2FullFrameProtocol *src);
  
  /** Do the md5/rsa authentication. Return True if successful. Has the side
      effect of appending the appropriate Ie class to the "reply" parameter.*/
  PBoolean Authenticate(IAX2FullFrameProtocol *reply, /*!< this frame contains the result of authenticating the internal data*/
                    PString & password /*!< the password to authenticate with */
        );
        
  /**Hold each of the possible values from an Ie class */
  IAX2IeData ieData;
  
  /**Transmit an IAX2 protocol frame with subclass type ack immediately to
     remote endpoint */
  void SendAckFrame(IAX2FullFrame *inReplyTo);

  /**Transmit an IAX2 protocol frame with subclass type VNAK immediately to
     remote endpoint. This message indicates we have received some full frames
     out of order, and want the interim ones retransmitted. */
  void SendVnakFrame(IAX2FullFrame *inReplyTo);
  
  /**Transmit an unsupported frame to the remote endpoint*/
  void SendUnsupportedFrame(IAX2FullFrame *inReplyTo);
  
 private:
#ifdef DOC_PLUS_PLUS
  /** pwlib constructs to cope with no response to an outgoing
      message. This is used to handle the output of the
      noResponseTimer
       
  This method runs in a separate thread to the heart of the
  Connection.  It is threadsafe, cause each of the elements in
  Connection (that are touched) are thread safe */
  void OnNoResponseTimeoutStart(PTimer &, INT);
#else
  PDECLARE_NOTIFIER(PTimer, IAX2Processor, OnNoResponseTimeoutStart);
#endif 

 protected:
  /**The timestamp we will put on the next mini frame out of here 

     This timestamp is monotonically increasing, and bears "some" relation to
     actuality.  We generate the timestamp uniformly - this instance of an
     iax2 call could be driven from a slightly non uniform packet source. */

  DWORD currentSoundTimeStamp;
};


#endif // OPAL_IAX2

#endif // OPAL_IAX2_PROCESSOR_H
