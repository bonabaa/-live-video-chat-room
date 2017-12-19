/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Describes the IAX2 extension of the OpalConnection class.
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
 * $Revision: 23414 $
 * $Author: rjongbloed $
 * $Date: 2009-09-10 08:53:29 +0000 (Thu, 10 Sep 2009) $
 */

#ifndef OPAL_IAX2_IAX2CON_H
#define OPAL_IAX2_IAX2CON_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if OPAL_IAX2

#include <opal/connection.h>
#include <rtp/jitter.h>

#include <iax2/frame.h>
#include <iax2/iedata.h>
#include <iax2/processor.h>
#include <iax2/callprocessor.h>
#include <iax2/safestrings.h>
#include <iax2/sound.h>

class IAX2EndPoint;


////////////////////////////////////////////////////////////////////////////////
/**This class handles all data associated with a call to one remote computer.
   It runs a separate thread, which wakes in response to an incoming sound 
   block from a sound device, or inresponse to an incoming ethernet packet, or in
   response from the UI for action (like sending dtmf)
   
   It is a thread, and runs when activated by outside events.*/
class IAX2Connection : public OpalConnection
{ 
  PCLASSINFO(IAX2Connection, OpalConnection);
  
 public:  
  /**@name Construction/Destruction functions*/
  //@{
  
  /**Construct a connection given the endpoint. 
   */
  IAX2Connection(
    OpalCall & call,             ///Owner call for connection
    IAX2EndPoint & endpoint,     ///Owner iax endpoint for connection
    const PString & token,       ///Token to identify the connection
    void *userData,              ///Specific user data for this call
    const PString & remoteParty, ///Url we are calling or getting called by
    const PString & remotePartyName = PString::Empty() ///The name of the remote party
  );
  
  /**Destroy this connection, but do it nicely and let attached sound objects
     close first.
  */
  ~IAX2Connection();
  //@}

  /**this method starts the callprocessor of a connection, and maves
     the phase to setup. Essentially, we start the entire call
     handling process here. */
  void StartOperation();

    /**Get indication of connection being to a "network".
       This indicates the if the connection may be regarded as a "network"
       connection. The distinction is about if there is a concept of a "remote"
       party being connected to and is best described by example: sip, h323,
       iax and pstn are all "network" connections as they connect to something
       "remote". While pc, pots and ivr are not as the entity being connected
       to is intrinsically local.
      */
    virtual bool IsNetworkConnection() const { return true; }

    /**Initiate the transfer of an existing call (connection) to a new
       remote party.

       If remoteParty is a valid call token, then the remote party is
       transferred to that party (consultation transfer) and both
       calls are cleared.
     */
    virtual bool TransferConnection(
      const PString & remoteParty ///<  Remote party to transfer the existing call to
    );

  /**Clean up the termination of the connection.  This function can
     do any internal cleaning up and waiting on background threads
     that may be using the connection object.
     
     Note that there is not a one to one relationship with the
     OnEstablishedConnection() function. This function may be called
     without that function being called. For example if
     SetUpConnection() was used but the call never completed.
     
     Classes that override this function should make sure they call
     the ancestor version for correct operation.
     
     An application will not typically call this function as it is
     used by the OpalManager during a release of the connection.
     
     The default behaviour calls the OpalEndPoint function of the
     same name.
  */
  virtual void OnReleased();

  /**Get the data formats this connection is capable of operating.
     This provides a list of media data format names that a
     OpalMediaStream may be created in within this connection.
     
     This method returns media formats that were decided through the initial
     exchange of packets when setting up a call (the cmdAccept and cmdNew
     packets). 

     The OpalConnection has this method as pure, so it is defined for IAX2.
  */
  OpalMediaFormatList GetMediaFormats() const { return remoteMediaFormats; }

  /**Cause the call to end now, but do not send any iax hangup frames etc */
  void EndCallNow(
      CallEndReason reason = EndedByLocalUser /// Reason for call clearing
      );

  OpalConnection::SendUserInputModes GetRealSendUserInputMode() const;

  /**Provided as a link between the iax endpoint and the iax processor */
  void SendDtmf(const PString & dtmf);

  /** sending text fullframes **/
  virtual PBoolean SendUserInputString(const PString & value );
  
  /** sending dtmf  - which is 1 char per IAX2FullFrameDtmf on the frame.**/
  virtual PBoolean SendUserInputTone(char tone, unsigned duration );

  /**Report if this Connection is still active */
  PBoolean IsCallTerminating() { return iax2Processor.IsCallTerminating(); }
  
  /**Indicate the result of answering an incoming call.
     This should only be called if the OnAnswerCall() callback function has
     returned a AnswerCallPending or AnswerCallDeferred response.

     IAX2 traps this call for no "real" reason, except to know for
     sure when the audio streams start. This can also be regarded as a
     convenience function, so we know when the call media beings.
  */
  virtual void AnsweringCall(
			     AnswerCallResponse response ///< response to incoming call
			     );

  /**Capture an Opal generated call back, which tells us the media
     streams are about to start. In this method, we start the jitter
     buffer for the IAX2 audio packets, and then call the OPAL
     OnConnected method to ensure the proper handling takes place.

  That this method is called is an indication that the remote endpoint
  has answered our call and is happy for the call to proceed.*/
  void OnConnected();   
    
  /**Indicate to remote endpoint we are connected.  In other words,
     tell the remote endpoint we accept the call and things proceed
     with the call eventuallly moving to established phase.
     
     In addition to the OpalConnection::SetConnected method, we have
     to do the iax2 connection stuff, which is to send an iax2
     answer packet to the remote host.
     
     The IAX2 answer packet is sent by the iax2 call processor, and
     indicates we have agreed to the incoming call.
  */
  virtual PBoolean SetConnected();

  /**A call back function whenever a connection is established.
       This indicates that a connection to an endpoint was established. This
       usually occurs after OnConnected() and indicates that the connection
       is both connected and has media flowing.

       In the context of IAX2 this means we have received the first
       full frame of media from the remote endpoint

       This method runs when a media stream in OpalConnection is opened.  This
       callback is used to indicate we need to start the "every 10 second do
       an iax2 lagrq/lagrp+ping/pong exhange process".  

       This exchange proces is invoked in the IAX2CallProcessor.
      */
  void OnEstablished();

  /**Release the current connection.
     This removes the connection from the current call. The call may
     continue if there are other connections still active on it. If this was
     the last connection for the call then the call is disposed of as well.
     
     Note that this function will return quickly as the release and
     disposal of the connections is done by another thread.
  
     This sends an IAX2 hangup frame to the remote endpoint.
 
     The ConnectionRun that manages packets going into/out of the
     IAX2Connection will continue to run, and will send the appropriate
     IAX2 hangup messages. Death of the Connection thread will happen when the 
     OnReleased thread of the OpalConnection runs */
  virtual void Release( CallEndReason reason = EndedByLocalUser ///<Reason for call release
		);

  /**Indicate to remote endpoint an alert is in progress.  If this is
     an incoming connection and the AnswerCallResponse is in a
     AnswerCallDeferred or AnswerCallPending state, then this function
     is used to indicate to that endpoint that an alert is in
     progress. This is usually due to another connection which is in
     the call (the B party) has received an OnAlerting() indicating
     that its remoteendpoint is "ringing".

     The OpalConnection has this method as pure, so it is defined for
     IAX2.
  */
  PBoolean SetAlerting(   
		       const PString & calleeName,///< Name of endpoint being alerted.
		       PBoolean withMedia  ///< Open media with alerting
			  ); 
     
  /**Open a new media stream.  This will create a media stream of 
     class OpalIAX2MediaStream.
     
     Note that media streams may be created internally to the
     underlying protocol. This function is not the only way a stream
     can come into existance.
  */
  virtual OpalMediaStream * CreateMediaStream(
				      const OpalMediaFormat & mediaFormat, ///< Media format for stream
				      unsigned sessionID,///< Session number for stream
				      PBoolean isSource  ///< Is a source stream
				      );

  /**Give the call token a value. The call token is the ipaddress of
     the remote node concatented with the remote nodes src
     number. This is guaranteed to be unique.  Sadly, if this
     connection is setting up the call, the callToken is not known
     until receipt of the first packet from the remote node.

     However, if this connection is created in response to a call,
     this connection can determine the callToken on examination of
     that incoming first packet */

  void SetCallToken(PString newToken);

  /**Return the string that identifies this IAX2Connection instance */
  PString GetCallToken() { return iax2Processor.GetCallToken(); }

  /**Transmit IAX2Frame to remote endpoint,
    It is only called by the the IAXProcessor class. */
  void TransmitFrameToRemoteEndpoint(IAX2Frame *src);
 
  /**Handle a sound packet received from the sound device. 
     
  Now onsend this to the remote endpoint. */
  void PutSoundPacketToNetwork(PBYTEArray *sund);

  /**We have received an iax2 sound frame from the network. This method
     places it in the jitter buffer (a member of this class) */
  void ReceivedSoundPacketFromNetwork(IAX2Frame *soundFrame);

  /**Grab a sound packet from the jitterBuffer in this class. Return it to the
     requesting process, which is in an instance of the OpalIAX2MediaStream
     class. Note that this class returns RTP_DataFrame instances. These
     RTP_DataFrame instances were generated by the IAX2CallProcessor class.*/  
  PBoolean ReadSoundPacket(RTP_DataFrame & packet);

  /**Get information on Remote class (remote node address & port + source & dest call number.) */
  IAX2Remote & GetRemoteInfo() { return iax2Processor.GetRemoteInfo(); }

  /**Get the sequence number info (inSeqNo and outSeqNo) */
  IAX2SequenceNumbers & GetSequenceInfo() { return iax2Processor.GetSequenceInfo(); }
  
  /**Get the call start time */
  const PTimeInterval & GetCallStartTick() { return iax2Processor.GetCallStartTick(); } 

  /**We have received a packet from the remote iax endpoint, requeting a call.
     Now, we use this method to invoke the opal components to do their bit.
     
     This method is called after OnIncomingConnection().*/
  void OnSetUp();


  /**Start an outgoing connection.
     This function will initiate the connection to the remote entity, for
     example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.
     In IAX2, it sends a New packet..

     The behaviour at the opal level is pure. Here, the method is defined.
  */
  PBoolean SetUpConnection();


  /**Return the bitmask which specifies the possible codecs we
     support.  The supported codecs are a bitmask of values defined by
     FullFrameVoice::AudioSc */
  PINDEX GetSupportedCodecs();
  
  /**Return the bitmask which specifies the preferred codec. The
     selected codec is in the binary value defined by
     FullFrameVoice::AudioSc */
  PINDEX GetPreferredCodec();

  /**Fill the OpalMediaFormatList which describes the remote nodes
     capabilities */
  void BuildRemoteCapabilityTable(unsigned int remoteCapability, unsigned int format);

     
  /** The local capabilites and remote capabilites are in
      OpalMediaFormatList classes, stored in this class.  The local
      capbilities have already been ordered by the users
      preferences. The first entry in the remote capabilities is the
      remote endpoints preferred codec. Now, we have to select a codec
      to use for this connection. The selected codec is in the binary
      value defined by FullFrameVoice::AudioSc */
  unsigned int ChooseCodec();
  
    /**Return true if the current connection is on hold.
       The bool parameter indicates if we are testing if the remote system
       has us on hold, or we have them on hold.
     */
    virtual bool IsConnectionOnHold(
      bool fromRemote  ///< Test if remote has us on hold, or we have them
    );
  
  /**Take the current connection off hold*/
  virtual bool RetrieveConnection();
  
  /**Put the current connection on hold, suspending all media streams.*/
  virtual bool HoldConnection();
  
  /**Signal that the remote side has put the connection on hold*/
  void RemoteHoldConnection();
  
  /**Signal that the remote side has retrieved the connection*/
  void RemoteRetrieveConnection();

  /**Set the username for when we connect to a remote node
     we use it as authentication.  Note this must only be
     used before SetUpConnection is ran.  This is optional
     because some servers do not required authentication, also
     if it is not set then the default iax2Ep username 
     will be used instead.*/
  void SetUserName(PString & inUserName) { userName = inUserName; };
  
  /**Get the username*/
  PString GetUserName() const { return userName; };
  
  /**Set the password for when we connect to a remote node
     we use it as authentication.  Note this must only be
     used before SetUpConnection is ran.  This is optional
     because some servers do not required authentication, also
     if it is not set then the default iax2Ep password 
     will be used instead.*/
  void SetPassword(PString & inPassword) { password = inPassword; };
  
  /**Get the password*/
  PString GetPassword() const { return password; };
  
    
  /**Forward incoming call to specified address.
     This would typically be called from within the OnIncomingCall()
     function when an application wishes to redirct an unwanted incoming
     call.
     
     The return value is PTrue if the call is to be forwarded, PFalse
     otherwise. Note that if the call is forwarded the current connection is
     cleared with teh ended call code of EndedByCallForwarded.
  */
  virtual PBoolean ForwardCall(
			       const PString & forwardParty   ///<  Party to forward call to.
			       );
  
  /**Handle a received IAX frame. This may be a mini frame or full frame.
   Typically, this connection instance will immediately pass the frame on to
   the CallProcessor::IncomingEthernetFrame() method.*/
  void IncomingEthernetFrame (IAX2Frame *frame);
  
  /**Test to see if it is a status query type iax frame (eg lagrq) and
     handle it. If the frame is a status query, and it is handled,
     return PTrue */
  //static PBoolean IsStatusQueryEthernetFrame(IAX2Frame *frame);
    
  /**Return reference to the endpoint class */
  IAX2EndPoint & GetEndPoint() { return endpoint; }
  
  /**Invoked by the User interface, which causes the statistics (count of in/out packets)
     to be printed*/
  void ReportStatistics();
    

 protected:
  
  /**Username for the iax2CallProcessor*/
  PString userName;
  
  /**Password for the iax2CallProcessor*/
  PString password;
  
  /**@name Internal, protected methods, which are invoked only by this
     thread*/
  //@{

  /**Global variable which specifies IAX2 protocol specific issues */
  IAX2EndPoint    &endpoint;

  /**The list of media formats (codecs) the remote enpoint can use.
     This list is defined on receiving a particular Inforation Element */
  OpalMediaFormatList remoteMediaFormats;

  /**The list of media formats (codecs) this end wants to use.
     This list is defined on constructing this IAX2Connection class */
  OpalMediaFormatList localMediaFormats;
    
  /**The thread that processes the list of pending frames on this class */
  IAX2CallProcessor & iax2Processor;
  
  /**Whether the connection is on hold locally */
  PBoolean            local_hold;
  
  /**Whether the connection is on hold remotely */
  PBoolean            remote_hold;

  //@}

  /**This jitter buffer smooths out the delivery times from the network, so
     that packets arrive in schedule at the far end. */
  OpalJitterBuffer jitterBuffer;

  /**The payload type, which we put on all RTP_DataFrame packets. This
     variable is placed on all RTP_DataFrame instances, prior to placing these
     frames into the jitter buffer.

     This variable is not used in the transmission of frames.

     Note that this variable describes the payload type as opal sees it. */
  RTP_DataFrame::PayloadTypes opalPayloadType;

  friend class IAX2CallProcessor;
};


////////////////////////////////////////////////////////////////////////////////


#endif // OPAL_IAX2

#endif // OPAL_IAX2_IAX2CON_H

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 2 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-basic-offset:2
 * End:
 */
