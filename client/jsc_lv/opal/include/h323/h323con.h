/*
 * h323con.h
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
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
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23728 $
 * $Author: rjongbloed $
 * $Date: 2009-10-29 23:42:22 +0000 (Thu, 29 Oct 2009) $
 */

#ifndef OPAL_H323_H323CON_H
#define OPAL_H323_H323CON_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_H323

#include <opal/rtpconn.h>
#include <opal/guid.h>
#include <opal/buildopts.h>
#include <h323/h323caps.h>
#include <ptclib/dtmf.h>


/* The following classes have forward references to avoid including the VERY
   large header files for H225 and H245. If an application requires access
   to the protocol classes they can include them, but for simple usage their
   inclusion can be avoided.
 */
class PPER_Stream;
class PASN_OctetString;

class H225_EndpointType;
class H225_TransportAddress;
class H225_ArrayOf_PASN_OctetString;
class H225_ProtocolIdentifier;
class H225_AdmissionRequest;
class H225_AdmissionConfirm;
class H225_AdmissionReject;
class H225_InfoRequestResponse;
class H225_DisengageRequest;
class H225_FeatureSet;

class H245_TerminalCapabilitySet;
class H245_TerminalCapabilitySetReject;
class H245_OpenLogicalChannel;
class H245_OpenLogicalChannelAck;
class H245_TransportAddress;
class H245_UserInputIndication;
class H245_RequestMode;
class H245_RequestModeAck;
class H245_RequestModeReject;
class H245_ModeDescription;
class H245_ArrayOf_ModeDescription;
class H245_SendTerminalCapabilitySet;
class H245_MultiplexCapability;
class H245_FlowControlCommand;
class H245_MiscellaneousCommand;
class H245_MiscellaneousIndication;
class H245_JitterIndication;
class H245_ArrayOf_GenericParameter;

class H323SignalPDU;
class H323ControlPDU;
class H323EndPoint;
class H323TransportAddress;

class H235Authenticators;

class H245NegMasterSlaveDetermination;
class H245NegTerminalCapabilitySet;
class H245NegLogicalChannels;
class H245NegRequestMode;
class H245NegRoundTripDelay;

class H450xDispatcher;
class H4502Handler;
class H4504Handler;
class H4506Handler;
class H4507Handler;
class H45011Handler;

class OpalCall;

#if OPAL_H460
class H460_FeatureSet;
#endif


///////////////////////////////////////////////////////////////////////////////

/**This class represents a particular H323 connection between two endpoints.
   There are at least two threads in use, this one to look after the
   signalling channel, an another to look after the control channel. There
   would then be additional threads created for each data channel created by
   the control channel protocol thread.
 */
class H323Connection : public OpalRTPConnection
{
  PCLASSINFO(H323Connection, OpalRTPConnection);

  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    H323Connection(
      OpalCall & call,                         ///<  Call object connection belongs to
      H323EndPoint & endpoint,                 ///<  H323 End Point object
      const PString & token,                   ///<  Token for new connection
      const PString & alias,                   ///<  Alias for outgoing call
      const H323TransportAddress & address,    ///<  Address for outgoing call
      unsigned options = 0,                    ///<  Connection option bits
      OpalConnection::StringOptions * stringOptions = NULL ///<  complex string options
    );

    /**Destroy the connection
     */
    ~H323Connection();
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Get indication of connection being to a "network".
       This indicates the if the connection may be regarded as a "network"
       connection. The distinction is about if there is a concept of a "remote"
       party being connected to and is best described by example: sip, h323,
       iax and pstn are all "network" connections as they connect to something
       "remote". While pc, pots and ivr are not as the entity being connected
       to is intrinsically local.
      */
    virtual bool IsNetworkConnection() const { return true; }

    /**Get this connections protocol prefix for URLs.
      */
    virtual PString GetPrefixName() const;

    /**Start an outgoing connection.
       This function will initiate the connection to the remote entity, for
       example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.

       The default behaviour is to send SETUP packet.
      */
    virtual PBoolean SetUpConnection();

    /**Indicate to remote endpoint an alert is in progress.
       If this is an incoming connection and it is in the Alerting phase, then
       this function is used to indicate to that endpoint that an alert is in
       progress. This is usually due to another connection which is in the
       call (the B party) has received an OnAlerting() indicating that its
       remote endpoint is "ringing".

       The default behaviour sends an ALERTING pdu.
      */
    virtual PBoolean SetAlerting(
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      PBoolean withMedia                ///<  Open media with alerting
    );

    /**Indicate to remote endpoint we are connected.

       The default behaviour sends a CONNECT pdu.
      */
    virtual PBoolean SetConnected();

    /**Indicate to remote endpoint we are sending a progress.

      The default behaviour sends a PROGRESS pdu.
     */
    virtual PBoolean SetProgressed();
    
    /** Called when a connection is established.
        This indicates that a connection to an endpoint was established. This
        usually occurs after OnConnected() and indicates that the connection
        is both connected and has media flowing.

        Default behaviour is to call H323EndPoint::OnConnectionEstablished
      */
    virtual void OnEstablished();

    /**Clean up the termination of the connection.
       This function can do any internal cleaning up and waiting on background
       threads that may be using the connection object.

       Note that there is not a one to one relationship with the
       OnEstablishedConnection() function. This function may be called without
       that function being called. For example if SetUpConnection() was used
       but the call never completed.

       Classes that override this function should make sure they call the
       ancestor version for correct operation.

       An application will not typically call this function as it is used by
       the OpalManager during a release of the connection.

       The default behaviour calls CleanUpOnCallEnd() then calls the ancestor.
      */
    virtual void OnReleased();

    /**Get the destination address of an incoming connection.
       This will, for example, collect a phone number from a POTS line, or
       get the fields from the H.225 SETUP pdu in a H.323 connection.
      */
    virtual PString GetDestinationAddress();

    /**Get alerting type information of an incoming call.
       The type of "distinctive ringing" for the call. The string is protocol
       dependent, so the caller would need to be aware of the type of call
       being made. Some protocols may ignore the field completely.

       For SIP this corresponds to the string contained in the "Alert-Info"
       header field of the INVITE. This is typically a URI for the ring file.

       For H.323 this must be a string representation of an integer from 0 to 7
       which will be contained in the Q.931 SIGNAL (0x34) Information Element.

       Default behaviour returns an empty string.
      */
    virtual PString GetAlertingType() const;

    /**Set alerting type information for outgoing call.
       The type of "distinctive ringing" for the call. The string is protocol
       dependent, so the caller would need to be aware of the type of call
       being made. Some protocols may ignore the field completely.

       For SIP this corresponds to the string contained in the "Alert-Info"
       header field of the INVITE. This is typically a URI for the ring file.

       For H.323 this must be a string representation of an integer from 0 to 7
       which will be contained in the Q.931 SIGNAL (0x34) Information Element.

       Default behaviour returns false.
      */
    virtual bool SetAlertingType(const PString & info);

    /**Get the data formats this connection is capable of operating.
       This provides a list of media data format names that an
       OpalMediaStream may be created in within this connection.

       The default behaviour returns media data format names contained in
       the remote capability table.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /**Get next available session ID for the media type.
      */
    virtual unsigned GetNextSessionID(
      const OpalMediaType & mediaType,   ///< Media type of stream being opened
      bool isSource                      ///< Stream is a source/sink
    );

#if OPAL_FAX
    /**Switch to/from FAX mode.
      */
    virtual bool SwitchFaxMediaStreams(
      bool enableFax  ///< Enable FAX or return to audio mode
    );
#endif

    /**Open source or sink media stream for session.
      */
    virtual OpalMediaStreamPtr OpenMediaStream(
      const OpalMediaFormat & mediaFormat, ///<  Media format to open
      unsigned sessionID,                  ///<  Session to start stream on
      bool isSource                        ///< Stream is a source/sink
    );
    
    /**Request close of a specific media stream.
       Note that this is usually asymchronous, the OnClosedMediaStream() function is
       called when the stream is really closed.
      */
    virtual bool CloseMediaStream(
      OpalMediaStream & stream  ///< Stream to close
    );

    /**Get information on the media channel for the connection.
       The default behaviour returns PTrue and fills the info structure if
       there is a media channel active for the sessionID.
     */
    virtual PBoolean GetMediaInformation(
      unsigned sessionID,     ///<  Session ID for media channel
      MediaInformation & info ///<  Information on media channel
    ) const;
  //@}

  /**@name Backward compatibility functions */
  //@{
    /** Called when a connection is cleared, just after CleanUpOnCallEnd()
        Default behaviour is to call H323Connection::OnConnectionCleared
      */
    virtual void OnCleared();

    /**Determine if the call has been established.
       This can be used in combination with the GetCallEndReason() function
       to determine the three main phases of a call, call setup, call
       established and call cleared.
      */
    PBoolean IsEstablished() const { return connectionState == EstablishedConnection; }

    /**Clean up the call clearance of the connection.
       This function will do any internal cleaning up and waiting on background
       threads that may be using the connection object. After this returns it
       is then safe to delete the object.

       An application will not typically use this function as it is used by the
       H323EndPoint during a clear call.
      */
    virtual void CleanUpOnCallEnd();

    virtual void ApplyStringOptions(OpalConnection::StringOptions & stringOptions);
  //@}


  /**@name Signalling Channel */
  //@{
    /**Attach a transport to this connection as the signalling channel.
      */
    void AttachSignalChannel(
      const PString & token,    ///<  New token to use to identify connection
      H323Transport * channel,  ///<  Transport for the PDU's
      PBoolean answeringCall        ///<  Flag for if incoming/outgoing call.
    );

    /**Write a PDU to the signalling channel.
      */
    PBoolean WriteSignalPDU(
      H323SignalPDU & pdu       ///<  PDU to write.
    );

    /**Handle reading PDU's from the signalling channel.
       This is an internal function and is unlikely to be used by applications.
     */
    virtual void HandleSignallingChannel();

    /**Handle PDU from the signalling channel.
       This is an internal function and is unlikely to be used by applications.
     */
    virtual PBoolean HandleSignalPDU(
      H323SignalPDU & pdu       ///<  PDU to handle.
    );

    /**Handle Control PDU tunnelled in the signalling channel.
       This is an internal function and is unlikely to be used by applications.
     */
    virtual void HandleTunnelPDU(
      H323SignalPDU * txPDU       ///<  PDU tunnel response into.
    );

    /**Handle an incoming Q931 setup PDU.
       The default behaviour is to do the handshaking operation calling a few
       virtuals at certain moments in the sequence.

       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.
     */
    virtual PBoolean OnReceivedSignalSetup(
      const H323SignalPDU & pdu   ///<  Received setup PDU
    );

    /**Handle an incoming Q931 setup acknowledge PDU.
       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour does nothing.
     */
    virtual PBoolean OnReceivedSignalSetupAck(
      const H323SignalPDU & pdu   ///<  Received setup PDU
    );

    /**Handle an incoming Q931 information PDU.
       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour does nothing.
     */
    virtual PBoolean OnReceivedSignalInformation(
      const H323SignalPDU & pdu   ///<  Received setup PDU
    );

    /**Handle an incoming Q931 call proceeding PDU.
       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour checks for hH245Address field and if present
       starts the separate H245 channel, if successful or not present it
       returns PTrue.
     */
    virtual PBoolean OnReceivedCallProceeding(
      const H323SignalPDU & pdu   ///<  Received call proceeding PDU
    );

    /**Handle an incoming Q931 progress PDU.
       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour checks for hH245Address field and if present
       starts the separate H245 channel, if successful or not present it
       returns PTrue.
     */
    virtual PBoolean OnReceivedProgress(
      const H323SignalPDU & pdu   ///<  Received call proceeding PDU
    );

    /**Handle an incoming Q931 alerting PDU.
       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour obtains the display name and calls OnAlerting().
     */
    virtual PBoolean OnReceivedAlerting(
      const H323SignalPDU & pdu   ///<  Received connect PDU
    );

    /**Handle an incoming Q931 connect PDU.
       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour checks for hH245Address field and if present
       starts the separate H245 channel, if successful it returns PTrue.
       If not present and there is no H245Tunneling then it returns PFalse.
     */
    virtual PBoolean OnReceivedSignalConnect(
      const H323SignalPDU & pdu   ///<  Received connect PDU
    );

    /**Handle an incoming Q931 facility PDU.
       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour checks for hH245Address field and if present
       starts the separate H245 channel, if successful or not present it
       returns PTrue.
     */
    virtual PBoolean OnReceivedFacility(
      const H323SignalPDU & pdu   ///<  Received connect PDU
    );

    /**Handle an incoming Q931 Notify PDU.
       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns PTrue.
     */
    virtual PBoolean OnReceivedSignalNotify(
      const H323SignalPDU & pdu   ///<  Received connect PDU
    );

    /**Handle an incoming Q931 Status PDU.
       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns PTrue.
     */
    virtual PBoolean OnReceivedSignalStatus(
      const H323SignalPDU & pdu   ///<  Received connect PDU
    );

    /**Handle an incoming Q931 Status Enquiry PDU.
       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour sends a Q931 Status PDU back.
     */
    virtual PBoolean OnReceivedStatusEnquiry(
      const H323SignalPDU & pdu   ///<  Received connect PDU
    );

    /**Handle an incoming Q931 Release Complete PDU.
       The default behaviour calls Clear() using reason code based on the
       Release Complete Cause field and the current connection state.
     */
    virtual void OnReceivedReleaseComplete(
      const H323SignalPDU & pdu   ///<  Received connect PDU
    );

    /**This function is called from the HandleSignallingChannel() function
       for unhandled PDU types.

       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent. The default behaviour returns PTrue.
     */
    virtual PBoolean OnUnknownSignalPDU(
      const H323SignalPDU & pdu  ///<  Received PDU
    );

    /**
      * called when an ARQ needs to be sent to a gatekeeper. This allows the connection
      * to change or check fields in the ARQ before it is sent.
      *
      * By default, this calls the matching function on the endpoint
      */
    virtual void OnSendARQ(
      H225_AdmissionRequest & arq
    );

    /**
      * called when an ACF is received from a gatekeeper. 
      *
      * By default, this calls the matching function on the endpoint
      */
    virtual void OnReceivedACF(
      const H225_AdmissionConfirm & acf
    );

    /**
      * called when an ARJ is received from a gatekeeper. 
      *
      * By default, this calls the matching function on the endpoint
      */
    virtual void OnReceivedARJ(
      const H225_AdmissionReject & arj
    );

    /**
      * called when an IRR needs to be sent to a gatekeeper. This allows the connection
      * to change or check fields in the IRR before it is sent.
      *
      * By default, this does nothing
      */
    virtual void OnSendIRR(
      H225_InfoRequestResponse & irr
    ) const;

    /**
      * called when an DRQ needs to be sent to a gatekeeper. This allows the connection
      * to change or check fields in the DRQ before it is sent.
      *
      * By default, this does nothing
      */
    virtual void OnSendDRQ(
      H225_DisengageRequest & drq
    ) const;

    /**Call back for incoming call.
       This function is called from the OnReceivedSignalSetup() function
       before it sends the Alerting PDU. It gives an opportunity for an
       application to alter the reply before transmission to the other
       endpoint.

       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour calls the endpoint function of the same name.
     */
    virtual PBoolean OnIncomingCall(
      const H323SignalPDU & setupPDU,   ///<  Received setup PDU
      H323SignalPDU & alertingPDU       ///<  Alerting PDU to send
    );

    /**Forward incoming call to specified address.
       This would typically be called from within the OnIncomingCall()
       function when an application wishes to redirct an unwanted incoming
       call.

       The return value is PTrue if the call is to be forwarded, PFalse
       otherwise. Note that if the call is forwarded the current connection is
       cleared with the ended call code of EndedByCallForwarded.
      */
    virtual PBoolean ForwardCall(
      const PString & forwardParty   ///<  Party to forward call to.
    );

    /**Initiate the transfer of an existing call (connection) to a new remote 
       party.

       If remoteParty is a valid call token, then the remote party is transferred
       to that party (consultation transfer) and both calls are cleared.
     */
    virtual bool TransferConnection(
      const PString & remoteParty   ///<  Remote party to transfer the existing call to
    );

    /**Put the current connection on hold, suspending all media streams.
     * Simply calls HoldCall() which is kept for backward compatibility.
     */
    virtual bool HoldConnection();

    /**Retrieve the current connection from hold, activating all media 
     * streams.
     * Simply calls RetrieveCall() which is kept for backward compatibility.
     */
    virtual bool RetrieveConnection();

    /**Return true if the current connection is on hold.
       The bool parameter indicates if we are testing if the remote system
       has us on hold, or we have them on hold.

       Simply calls IsCallOnHold() which is kept for backward compatibility.
     */
    virtual bool IsConnectionOnHold(
      bool fromRemote  ///< Test if remote has us on hold, or we have them
    );

#if OPAL_H450

    /**Initiate the transfer of an existing call (connection) to a new remote party
       using H.450.2.  This sends a Call Transfer Initiate Invoke message from the
       A-Party (transferring endpoint) to the B-Party (transferred endpoint).
     */
    bool TransferCall(
      const PString & remoteParty,   ///<  Remote party to transfer the existing call to
      const PString & callIdentity = PString::Empty()
                                    ///<  Call Identity of secondary call if present
    );

    /**Transfer the call through consultation so the remote party in the primary call is connected to
       the called party in the second call using H.450.2.  This sends a Call Transfer Identify Invoke 
       message from the A-Party (transferring endpoint) to the C-Party (transferred-to endpoint).
     */
    void ConsultationTransfer(
      const PString & primaryCallToken  ///<  Primary call
    );

    /**Handle the reception of a callTransferSetupInvoke APDU whilst a secondary call exists.  This 
       method checks whether the secondary call is still waiting for a callTransferSetupInvoke APDU and 
       proceeds to clear the call if the call identies match.
       This is an internal function and it is not expected the user will call
       it directly.
     */
    virtual void HandleConsultationTransfer(
      const PString & callIdentity, /**Call Identity of secondary call 
                                       received in SETUP Message. */
      H323Connection & incoming     ///<  Connection upon which SETUP PDU was received.
    );

    /**Determine whether this connection is being transferred.
     */
    PBoolean IsTransferringCall() const;

    /**Determine whether this connection is the result of a transferred call.
     */
    PBoolean IsTransferredCall() const;

    /**Handle the transfer of an existing connection to a new remote.
       This is an internal function and it is not expected the user will call
       it directly.
     */
    virtual void HandleTransferCall(
      const PString & token,
      const PString & identity
    );

    /**Get transfer invoke ID dureing trasfer.
       This is an internal function and it is not expected the user will call
       it directly.
      */
    int GetCallTransferInvokeId();

    /**Handle the failure of a call transfer operation at the Transferred Endpoint.  This method is
       used to handle the following transfer failure cases that can occur at the Transferred Endpoint. 
       The cases are:
       Reception of an Admission Reject
       Reception of a callTransferSetup return error APDU.
       Expiry of Call Transfer timer CT-T4.
     */
    virtual void HandleCallTransferFailure(
      const int returnError    ///<  Failure reason code
    );

    /**Store the passed token on the current connection's H4502Handler.
       This is an internal function and it is not expected the user will call
       it directly.
     */
    void SetAssociatedCallToken(
      const PString & token  ///<  Associated token
    );

    /**Callback to indicate a successful transfer through consultation.  The paramter passed is a
       reference to the existing connection between the Transferring endpoint and Transferred-to 
       endpoint.
     */
    virtual void OnConsultationTransferSuccess(
      H323Connection & secondaryCall  ///<  Secondary call for consultation
    );

    /**Place the call on hold, suspending all media channels (H.450.4).  Note it is the responsibility 
       of the application layer to delete the MOH Channel if music on hold is provided to the remote
       endpoint.  So far only Local Hold has been implemented. 
     */
    bool HoldCall(
      PBoolean localHold   ///<  true for Local Hold, false for Remote Hold
    );

    /**Retrieve the call from hold, activating all media channels (H.450.4).
       This method examines the call hold state and performs the necessary
       actions required to retrieve a Near-end or Remote-end call on hold.
       NOTE: Only Local Hold is implemented so far. 
    */
    bool RetrieveCall();

    /**Set the alternative media channel.  This channel can be used to provide
       Media On Hold (MOH) for a near end call hold operation or to provide
       Recorded Voice Anouncements (RVAs).  If this method is not called before
       a call hold operation is attempted, no media on hold will be provided
       for the held endpoint.
      */
    void SetHoldMedia(
      PChannel * audioChannel
    );

    /**Determine if Meadia On Hold is enabled.
      */
    PBoolean IsMediaOnHold() const;

    /**Determine if held.
      */
    PBoolean IsLocalHold() const;

    /**Determine if held.
      */
    PBoolean IsRemoteHold() const;

    /**Determine if the current call is held or in the process of being held.
      */
    PBoolean IsCallOnHold() const;

    /**Begin a call intrusion request.
       Calls h45011handler->IntrudeCall where SS pdu is added to Call Setup
       message.
      */
    virtual void IntrudeCall(
      unsigned capabilityLevel
    );

    /**Handle an incoming call instrusion request.
       Calls h45011handler->AwaitSetupResponse where we set Handler state to
       CI-Wait-Ack
      */
    virtual void HandleIntrudeCall(
      const PString & token,
      const PString & identity
    );

    /**Set flag indicating call intrusion.
       Used to set a flag when intrusion occurs and to determine if
       connection is created for Call Intrusion. This flag is used when we
       should decide whether to Answer the call or to Close it.
      */
    void SetCallIntrusion() { isCallIntrusion = PTrue; }

    PBoolean IsCallIntrusion() { return isCallIntrusion; }

    /**Get Call Intrusion Protection Level of the local endpoint.
      */
    unsigned GetLocalCallIntrusionProtectionLevel() { return callIntrusionProtectionLevel; }

    /**Get Call Intrusion Protection Level of other endpoints that we are in
       connection with.
      */
    virtual PBoolean GetRemoteCallIntrusionProtectionLevel(
      const PString & callToken,
      unsigned callIntrusionProtectionLevel
    );

    virtual void SetIntrusionImpending();

    virtual void SetForcedReleaseAccepted();

    virtual void SetIntrusionNotAuthorized();

    /**Send a Call Waiting indication message to the remote endpoint using
       H.450.6.  The second paramter is used to indicate to the calling user
       how many additional users are "camped on" the called user. A value of
       zero indicates to the calling user that he/she is the only user
       attempting to reach the busy called user.
     */
    void SendCallWaitingIndication(
      const unsigned nbOfAddWaitingCalls = 0   ///<  number of additional waiting calls at the served user
    );

#endif

    /**Call back for answering an incoming call.
       This function is used for an application to control the answering of
       incoming calls. It is usually used to indicate the immediate action to
       be taken in answering the call.

       It is called from the OnReceivedSignalSetup() function before it sends
       the Alerting or Connect PDUs. It also gives an opportunity for an
       application to alter the Connect PDU reply before transmission to the
       remote endpoint.

       If AnswerCallNow is returned then the H.323 protocol proceeds with the
       connection. If AnswerCallDenied is returned the connection is aborted
       and a Release Complete PDU is sent. If AnswerCallPending is returned
       then the Alerting PDU is sent and the protocol negotiations are paused
       until the AnsweringCall() function is called. Finally, if
       AnswerCallDeferred is returned then no Alerting PDU is sent, but the
       system still waits as in the AnswerCallPending response.

       Note this function should not block for any length of time. If the
       decision to answer the call may take some time eg waiting for a user to
       pick up the phone, then AnswerCallPending or AnswerCallDeferred should
       be returned.

       The default behaviour calls the endpoint function of the same name
       which in turn will return AnswerCallNow.
     */
    virtual AnswerCallResponse OnAnswerCall(
      const PString & callerName,       ///< Name of caller
      const H323SignalPDU & setupPDU,   ///< Received setup PDU
      H323SignalPDU & connectPDU,       ///< Connect PDU to send. 
      H323SignalPDU & progressPDU       ///< Progress PDU to send. 
    );
    
    virtual AnswerCallResponse OnAnswerCall(
      const PString & callerName        ///<  Name of caller
    );

    /**Indicate the result of answering an incoming call.
       This should only be called if the OnAnswerCall() callback function has
       returned a AnswerCallPending or AnswerCallDeferred response.

       Note sending further AnswerCallPending responses via this function will
       have the result of an Alerting PDU being sent to the remote endpoint.
       In this way multiple Alerting PDUs may be sent.

       Sending a AnswerCallDeferred response would have no effect.
      */
    void AnsweringCall(
      AnswerCallResponse response ///<  Answer response to incoming call
    );

    /**Send first PDU in signalling channel.
       This function does the signalling handshaking for establishing a
       connection to a remote endpoint. The transport (TCP/IP) for the
       signalling channel is assumed to be already created. This function
       will then do the SetRemoteAddress() and Connect() calls o establish
       the transport.

       Returns the error code for the call failure reason or NumCallEndReasons
       if the call was successful to that point in the protocol.
     */
    virtual CallEndReason SendSignalSetup(
      const PString & alias,                ///<  Name of remote party
      const H323TransportAddress & address  ///<  Address of destination
    );

    /**Adjust setup PDU being sent on initialisation of signal channel.
       This function is called from the SendSignalSetup() function before it
       sends the Setup PDU. It gives an opportunity for an application to
       alter the request before transmission to the other endpoint.

       The default behaviour simply returns PTrue. Note that this is usually
       overridden by the transport dependent descendent class, eg the
       H323ConnectionTCP descendent fills in the destCallSignalAddress field
       with the TCP/IP data. Therefore if you override this in your
       application make sure you call the ancestor function.
     */
    virtual PBoolean OnSendSignalSetup(
      H323SignalPDU & setupPDU   ///<  Setup PDU to send
    );

    /**Adjust call proceeding PDU being sent. This function is called from
       the OnReceivedSignalSetup() function before it sends the Call
       Proceeding PDU. It gives an opportunity for an application to alter
       the request before transmission to the other endpoint. If this function
       returns PFalse then the Call Proceeding PDU is not sent at all.

       The default behaviour simply returns PTrue.
     */
    virtual PBoolean OnSendCallProceeding(
      H323SignalPDU & callProceedingPDU   ///<  Call Proceeding PDU to send
    );

    /**Call back for Release Complete being sent.
       This allows an application to add things to the release complete before
       it is sent to the remote endpoint.

       Returning PFalse will prevent the release complete from being sent. Note
       that this would be very unusual as this is called when the connection
       is being cleaned up. There will be no second chance to send the PDU and
       it must be sent.

       The default behaviour simply returns PTrue.
      */
    virtual PBoolean OnSendReleaseComplete(
      H323SignalPDU & releaseCompletePDU ///<  Release Complete PDU to send
    );

    /**Call back for remote party being alerted.
       This function is called from the SendSignalSetup() function after it
       receives the optional Alerting PDU from the remote endpoint. That is
       when the remote "phone" is "ringing".

       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour calls the endpoint function of the same name.
     */
    virtual PBoolean OnAlerting(
      const H323SignalPDU & alertingPDU,  ///<  Received Alerting PDU
      const PString & user                ///<  Username of remote endpoint
    );

    /**This function is called when insufficient digits have been entered.
       This supports overlapped dialling so that a call can begin when it is
       not known how many more digits are to be entered in a phone number.

       It is expected that the application will override this function. It
       should be noted that the application should not block in the function
       but only indicate to whatever other thread is gathering digits that
       more are required and that thread should call SendMoreDigits().

       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns PFalse.
     */
    virtual PBoolean OnInsufficientDigits();

    /**This function is called when sufficient digits have been entered.
       This supports overlapped dialling so that a call can begin when it is
       not known how many more digits are to be entered in a phone number.

       The digits parameter is appended to the existing remoteNumber member
       variable and the call is retried.

       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns PTrue.
     */
    virtual void SendMoreDigits(
      const PString & digits    ///<  Extra digits
    );

    /**This function is called from the SendSignalSetup() function after it
       receives the Connect PDU from the remote endpoint, but before it
       attempts to open the control channel.

       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour calls H323EndPoint::OnOutgoingCall
     */
    virtual PBoolean OnOutgoingCall(
      const H323SignalPDU & connectPDU   ///<  Received Connect PDU
    );

    /**Send an the acknowldege of a fast start.
       This function is called when the fast start channels provided to this
       connection by the original SETUP PDU have been selected and opened and
       need to be sent back to the remote endpoint.

       If PFalse is returned then no fast start has been acknowledged, possibly
       due to no common codec in fast start request.

       The default behaviour uses OnSelectLogicalChannels() to find a pair of
       channels and adds then to the provided PDU.
     */
    virtual PBoolean SendFastStartAcknowledge(
      H225_ArrayOf_PASN_OctetString & array   ///<  Array of H245_OpenLogicalChannel
    );

    /**Handle the acknowldege of a fast start.
       This function is called from one of a number of functions after it
       receives a PDU from the remote endpoint that has a fastStart field. It
       is in response to a request for a fast strart from the local endpoint.

       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour parses the provided array and starts the channels
       acknowledged in it.
     */
    virtual PBoolean HandleFastStartAcknowledge(
      const H225_ArrayOf_PASN_OctetString & array   ///<  Array of H245_OpenLogicalChannel
    );
  //@}

  /**@name Control Channel */
  //@{
    /**Start a separate H245 channel.
       This function is called from one of a number of functions after it
       receives a PDU from the remote endpoint that has a h245Address field.

       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour checks to see if it is a known transport and
       creates a corresponding H323Transport decendent for the control
       channel.
     */
    virtual PBoolean CreateOutgoingControlChannel(
      const H225_TransportAddress & h245Address   ///<  H245 address
    );

    /**Start a separate control channel.
       This function is called from one of a number of functions when it needs
       to listen for an incoming H.245 connection.


       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour checks to see if it is a known transport and
       creates a corresponding OpalTransport decendent for the control
       channel.
      */
    virtual PBoolean CreateIncomingControlChannel(
      H225_TransportAddress & h245Address  ///<  PDU transport address to set
    );

    /**Write a PDU to the control channel.
       If there is no control channel open then this will tunnel the PDU
       into the signalling channel.
      */
    virtual PBoolean WriteControlPDU(
      const H323ControlPDU & pdu
    );

    /**Start control channel negotiations.
      */
    virtual PBoolean StartControlNegotiations();

    /**Handle reading data on the control channel.
     */
    virtual void HandleControlChannel();

    /**Handle incoming data on the control channel.
       This decodes the data stream into a PDU and calls HandleControlPDU().

       If PFalse is returned the connection is aborted. The default behaviour
       returns PTrue.
     */
    virtual PBoolean HandleControlData(
      PPER_Stream & strm
    );

    /**Handle incoming PDU's on the control channel. Dispatches them to the
       various virtuals off this class.

       If PFalse is returned the connection is aborted. The default behaviour
       returns PTrue.
     */
    virtual PBoolean HandleControlPDU(
      const H323ControlPDU & pdu
    );

    /**This function is called from the HandleControlPDU() function
       for unhandled PDU types.

       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent. The default behaviour returns PTrue.

       The default behaviour send a FunctioNotUnderstood indication back to
       the sender, and returns PTrue to continue operation.
     */
    virtual PBoolean OnUnknownControlPDU(
      const H323ControlPDU & pdu  ///<  Received PDU
    );

    /**Handle incoming request PDU's on the control channel.
       Dispatches them to the various virtuals off this class.
     */
    virtual PBoolean OnH245Request(
      const H323ControlPDU & pdu  ///<  Received PDU
    );

    /**Handle incoming response PDU's on the control channel.
       Dispatches them to the various virtuals off this class.
     */
    virtual PBoolean OnH245Response(
      const H323ControlPDU & pdu  ///<  Received PDU
    );

    /**Handle incoming command PDU's on the control channel.
       Dispatches them to the various virtuals off this class.
     */
    virtual PBoolean OnH245Command(
      const H323ControlPDU & pdu  ///<  Received PDU
    );

    /**Handle incoming indication PDU's on the control channel.
       Dispatches them to the various virtuals off this class.
     */
    virtual PBoolean OnH245Indication(
      const H323ControlPDU & pdu  ///<  Received PDU
    );

    /**Handle H245 command to send terminal capability set.
     */
    virtual PBoolean OnH245_SendTerminalCapabilitySet(
      const H245_SendTerminalCapabilitySet & pdu  ///<  Received PDU
    );

    /**Handle H245 command to control flow control.
       This function calls OnLogicalChannelFlowControl() with the channel and
       bit rate restriction.
     */
    virtual PBoolean OnH245_FlowControlCommand(
      const H245_FlowControlCommand & pdu  ///<  Received PDU
    );

    /**Handle H245 miscellaneous command.
       This function passes the miscellaneous command on to the channel
       defined by the pdu.
     */
    virtual PBoolean OnH245_MiscellaneousCommand(
      const H245_MiscellaneousCommand & pdu  ///<  Received PDU
    );

    /**Handle H245 miscellaneous indication.
       This function passes the miscellaneous indication on to the channel
       defined by the pdu.
     */
    virtual PBoolean OnH245_MiscellaneousIndication(
      const H245_MiscellaneousIndication & pdu  ///<  Received PDU
    );

    /**Handle H245 indication of received jitter.
       This function calls OnLogicalChannelJitter() with the channel and
       estimated jitter.
     */
    virtual PBoolean OnH245_JitterIndication(
      const H245_JitterIndication & pdu  ///<  Received PDU
    );

#if OPAL_H239
    /**Handle a H.239 message from remote.
     */
    virtual bool OnH239Message(
      unsigned subMessage,
      const H245_ArrayOf_GenericParameter & params
    );

    /**Handle a H.239 flow control request.
       Default behaviour simply sends an acknowedge response.
      */
    virtual bool OnH239FlowControlRequest(
      unsigned logicalChannel,
      unsigned bitRate
    );

    /**Handle a H.239 flow control ack/reject response.
       Default behaviour does nothing
      */
    virtual bool OnH239FlowControlResponse(
      unsigned logicalChannel,
      bool rejected
    );

    /**Handle a H.239 presentation token request.
       Default behaviour simply sends an acknowedge response.
      */
    virtual bool OnH239PresentationRequest(
      unsigned logicalChannel,
      unsigned symmetryBreaking,
      unsigned terminalLabel
    );

    /**Handle a H.239 presentation token ack/reject response.
       Default behaviour simply sends a release command.
      */
    virtual bool OnH239PresentationResponse(
      unsigned logicalChannel,
      unsigned terminalLabel,
      bool rejected
    );

    /**Handle a H.239 presentation token release command.
       Default behaviour does nothing.
      */
    virtual bool OnH239PresentationRelease(
      unsigned logicalChannel,
      unsigned terminalLabel
    );

    /**Handle a H.239 presentation token owner indication.
       Default behaviour does nothing.
      */
    virtual bool OnH239PresentationIndication(
      unsigned logicalChannel,
      unsigned terminalLabel
    );
#endif

    /**Error discriminator for the OnControlProtocolError() function.
      */
    enum ControlProtocolErrors {
      e_MasterSlaveDetermination,
      e_CapabilityExchange,
      e_LogicalChannel,
      e_ModeRequest,
      e_RoundTripDelay
    };

    /**This function is called from the HandleControlPDU() function or
       any of its sub-functions for protocol errors, eg unhandled PDU types.

       The errorData field may be a string or PDU or some other data depending
       on the value of the errorSource parameter. These are:
          e_UnhandledPDU                    &H323ControlPDU
          e_MasterSlaveDetermination        const char *

       If PFalse is returned the connection is aborted. The default behaviour
       returns PTrue.
     */
    virtual PBoolean OnControlProtocolError(
      ControlProtocolErrors errorSource,  ///<  Source of the proptoerror
      const void * errorData = NULL       ///<  Data associated with error
    );

    /**This function is called from the HandleControlPDU() function when
       it is about to send the Capabilities Set to the remote endpoint. This
       gives the application an oppurtunity to alter the PDU to be sent.

       The default behaviour will make "adjustments" for compatibility with
       some broken remote endpoints.
     */
    virtual void OnSendCapabilitySet(
      H245_TerminalCapabilitySet & pdu  ///<  PDU to send
    );

    /**This function is called when the remote endpoint sends its capability
       set. This gives the application an opportunity to determine what codecs
       are available and if it supports any of the combinations of codecs.

       Note any codec types that are the remote system supports that are not in
       the codecs list member variable for the endpoint are ignored and not
       included in the remoteCodecs list.

       The default behaviour assigns the table and set to member variables and
       returns PTrue if the remoteCodecs list is not empty.
     */
    virtual PBoolean OnReceivedCapabilitySet(
      const H323Capabilities & remoteCaps,      ///<  Capability combinations remote supports
      const H245_MultiplexCapability * muxCap,  ///<  Transport capability, if present
      H245_TerminalCapabilitySetReject & reject ///<  Rejection PDU (if return PFalse)
    );

    /**Send a new capability set.
      */
    virtual bool SendCapabilitySet(
      PBoolean empty  ///<  Send an empty set.
    );

    /**check if TCS procedure in progress states.
      */
    virtual bool IsSendingCapabilitySet();

    /**Call back to set the local capabilities.
       This is called just before the capabilties are required when a call
       is begun. It is called when a SETUP PDU is received or when one is
       about to be sent, so that the capabilities may be adjusted for correct
       fast start operation.

       The default behaviour adds all media formats.
      */
    virtual void OnSetLocalCapabilities();

    /**Return if this H245 connection is a master or slave
     */
    PBoolean IsH245Master() const;

    /**Start the round trip delay calculation over the control channel.
     */
    void StartRoundTripDelay();

    /**Get the round trip delay over the control channel.
     */
    PTimeInterval GetRoundTripDelay() const;
  //@}

  /**@name Logical Channel Management */
  //@{
    /**Call back to select logical channels to start.

       This function must be defined by the descendent class. It is used
       to select the logical channels to be opened between the two endpoints.
       There are three ways in which this may be called: when a "fast start"
       has been initiated by the local endpoint (via SendSignalSetup()
       function), when a "fast start" has been requested from the remote
       endpoint (via the OnReceivedSignalSetup() function) or when the H245
       capability set (and master/slave) negotiations have completed (via the
       OnControlChannelOpen() function.

       The function would typically examine several member variable to decide
       which mode it is being called in and what to do. If fastStartState is
       FastStartDisabled then non-fast start semantics should be used. The
       H245 capabilities in the remoteCapabilities members should be
       examined, and appropriate transmit channels started using
       OpenLogicalChannel().

       If fastStartState is FastStartInitiate, then the local endpoint has
       initiated a call and is asking the application if fast start semantics
       are to be used. If so it is expected that the function call 
       OpenLogicalChannel() for all the channels that it wishes to be able to
       be use. A subset (possibly none!) of these would actually be started
       when the remote endpoint replies.

       If fastStartState is FastStartResponse, then this indicates the remote
       endpoint is attempting a fast start. The fastStartChannels member
       contains a list of possible channels from the remote that the local
       endpoint is to select which to accept. For each accepted channel it
       simply necessary to call the Start() function on that channel eg
       fastStartChannels[0].Start();

       The default behaviour selects the first codec of each session number
       that is available. This is according to the order of the capabilities
       in the remoteCapabilities, the local capability table or of the
       fastStartChannels list respectively for each of the above scenarios.
      */
    virtual void OnSelectLogicalChannels();

    /**Select default logical channel for normal start.
      */
    virtual void SelectDefaultLogicalChannel(
      const OpalMediaType & mediaType,  ///<  media type of channel
      unsigned sessionID                ///<  Session ID to find default logical channel.
    );

    /**Select default logical channel for fast start.
       Internal function, not for normal use.
      */
    virtual void SelectFastStartChannels(
      unsigned sessionID,   ///<  Session ID to find default logical channel.
      PBoolean transmitter,     ///<  Whether to open transmitters
      PBoolean receiver         ///<  Whether to open receivers
    );

    /**Start a logical channel for fast start.
       Internal function, not for normal use.
      */
    virtual void StartFastStartChannel(
      unsigned sessionID,               ///<  Session ID to find logical channel.
      H323Channel::Directions direction ///<  Direction of channel to start
    );

    /**Open a new logical channel.
       This function will open a channel between the endpoints for the
       specified capability.

       If this function is called while there is not yet a conenction
       established, eg from the OnFastStartLogicalChannels() function, then
       a "trial" receiver/transmitter channel is created. This channel is not
       started until the remote enpoint has confirmed that they are to start.
       Any channels not confirmed are deleted.

       If this function is called later in the call sequence, eg from
       OnSelectLogicalChannels(), then it may only establish a transmit
       channel, ie fromRemote must be PFalse.
      */
    virtual PBoolean OpenLogicalChannel(
      const H323Capability & capability,  ///<  Capability to open channel with
      unsigned sessionID,                 ///<  Session for the channel
      H323Channel::Directions dir         ///<  Direction of channel
    );
    
    virtual void SendFlowControlCommand(
      unsigned channelNumber,
      unsigned newBitRate
    );
    
    /**This function is called when the remote endpoint want's to open
       a new channel.

       If the return value is PFalse then the open is rejected using the
       errorCode as the cause, this would be a value from the enum
       H245_OpenLogicalChannelReject_cause::Choices.

       The default behaviour simply returns PTrue.
     */
    virtual PBoolean OnOpenLogicalChannel(
      const H245_OpenLogicalChannel & openPDU,  ///<  Received PDU for the channel open
      H245_OpenLogicalChannelAck & ackPDU,      ///<  PDU to send for acknowledgement
      unsigned & errorCode                      ///<  Error to return if refused
    );

    /**Callback for when a logical channel conflict has occurred.
       This is called when the remote endpoint, which is a master, rejects
       our transmitter channel due to a resource conflict. Typically an
       inability to do asymmetric codecs. The local (slave) endpoint must then
       try and open a new transmitter channel using the same codec as the
       receiver that is being opened.
      */
    virtual PBoolean OnConflictingLogicalChannel(
      H323Channel & channel    ///<  Channel that conflicted
    );

    /**Create a new logical channel object.
       This is in response to a request from the remote endpoint to open a
       logical channel.
      */
    virtual H323Channel * CreateLogicalChannel(
      const H245_OpenLogicalChannel & open, ///<  Parameters for opening channel
      PBoolean startingFast,                    ///<  Flag for fast/slow starting.
      unsigned & errorCode                  ///<  Reason for create failure
    );

    /**Create a new real time logical channel object.
       This creates a logical channel for handling RTP data. It is primarily
       used to allow an application to redirect the RTP media streams to other
       hosts to the local one. In that case it would create an instance of
       the H323_ExternalRTPChannel class with the appropriate address. eg:

         H323Channel * MyConnection::CreateRealTimeLogicalChannel(
                                        const H323Capability & capability,
                                        H323Channel::Directions dir,
                                        unsigned sessionID,
                                        const H245_H2250LogicalChannelParameters * param)
         {
           return new H323_ExternalRTPChannel(*this, capability, dir, sessionID,
                                              externalIpAddress, externalPort);
         }

       An application would typically also override the OnStartLogicalChannel()
       function to obtain from the H323_ExternalRTPChannel instance the address
       of the remote endpoints media server RTP addresses to complete the
       setting up of the external RTP stack. eg:

         PBoolean OnStartLogicalChannel(H323Channel & channel)
         {
           H323_ExternalRTPChannel & external = (H323_ExternalRTPChannel &)channel;
           external.GetRemoteAddress(remoteIpAddress, remotePort);
         }

       Note that the port in the above example is always the data port, the
       control port is assumed to be data+1.

       The default behaviour assures there is an RTP session for the session ID,
       and if not creates one, then creates a H323_RTPChannel which will do RTP
       media to the local host.
      */
    virtual H323Channel * CreateRealTimeLogicalChannel(
      const H323Capability & capability, ///<  Capability creating channel
      H323Channel::Directions dir,       ///<  Direction of channel
      unsigned sessionID,                ///<  Session ID for RTP channel
      const H245_H2250LogicalChannelParameters * param,
                                         ///<  Parameters for channel
      RTP_QOS * rtpqos = NULL            ///<  QoS for RTP
    );
    
    /**Creates a new instance of an RTP channel.
       Allows subclasses to return a custom instance
      */
    virtual H323_RTPChannel * CreateRTPChannel(
      const H323Capability & capability,
      H323Channel::Directions direction,
      RTP_Session & rtp
    );

    /**This function is called when the remote endpoint want's to create
       a new channel.

       If the return value is PFalse then the open is rejected using the
       errorCode as the cause, this would be a value from the enum
       H245_OpenLogicalChannelReject_cause::Choices.

       The default behaviour checks the capability set for if this capability
       is allowed to be opened with other channels that may already be open.
     */
    virtual PBoolean OnCreateLogicalChannel(
      const H323Capability & capability,  ///<  Capability for the channel open
      H323Channel::Directions dir,        ///<  Direction of channel
      unsigned & errorCode                ///<  Error to return if refused
    );

    /**Call back function when a logical channel thread begins.

       The default behaviour does nothing and returns PTrue.
      */
    virtual PBoolean OnStartLogicalChannel(
      H323Channel & channel    ///<  Channel that has been started.
    );

    /**Close a logical channel.
      */
    virtual void CloseLogicalChannel(
      unsigned number,    ///<  Channel number to close.
      PBoolean fromRemote     ///<  Indicates close request of remote channel
    );

    /**Close a logical channel by number.
      */
    virtual void CloseLogicalChannelNumber(
      const H323ChannelNumber & number    ///<  Channel number to close.
    );

    /**Close a logical channel.
      */
    virtual void CloseAllLogicalChannels(
      PBoolean fromRemote     ///<  Indicates close request of remote channel
    );

    /**This function is called when the remote endpoint has closed down
       a logical channel.

       The default behaviour does nothing.
     */
    virtual void OnClosedLogicalChannel(
      const H323Channel & channel   ///<  Channel that was closed
    );

    /**This function is called when the remote endpoint request the close of
       a logical channel.

       The application may get an opportunity to refuse to close the channel by
       returning PFalse from this function.

       The default behaviour returns PTrue.
     */
    virtual PBoolean OnClosingLogicalChannel(
      H323Channel & channel   ///<  Channel that is to be closed
    );

    /**This function is called when the remote endpoint wishes to limit the
       bit rate being sent on a channel.

       If channel is NULL, then the bit rate limit applies to all channels.

       The default behaviour does nothing if channel is NULL, otherwise calls
       H323Channel::OnFlowControl() on the specific channel.
     */
    virtual void OnLogicalChannelFlowControl(
      H323Channel * channel,   ///<  Channel that is to be limited
      long bitRateRestriction  ///<  Limit for channel
    );

    /**This function is called when the remote endpoint indicates the level
       of jitter estimated by the receiver.

       If channel is NULL, then the jitter applies to all channels.

       The default behaviour does nothing if channel is NULL, otherwise calls
       H323Channel::OnJitter() on the specific channel.
     */
    virtual void OnLogicalChannelJitter(
      H323Channel * channel,   ///<  Channel that is to be limited
      DWORD jitter,            ///<  Estimated received jitter in microseconds
      int skippedFrameCount,   ///<  Frames skipped by decodec
      int additionalBuffer     ///<  Additional size of video decoder buffer
    );

    /**Get a logical channel.
       Locates the specified channel number and returns a pointer to it.
      */
    H323Channel * GetLogicalChannel(
      unsigned number,    ///<  Channel number to get.
      PBoolean fromRemote     ///<  Indicates get a remote channel
    ) const;

    /**Find a logical channel.
       Locates a channel give a RTP session ID. Each session would usually
       have two logical channels associated with it, so the fromRemote flag
       bay be used to distinguish which channel to return.
      */
    H323Channel * FindChannel(
      unsigned sessionId,   ///<  Session ID to search for.
      PBoolean fromRemote       ///<  Indicates the direction of RTP data.
    ) const;
  //@}

  /**@name Bandwidth Management */
  //@{
    /**Set the available bandwidth in 100's of bits/sec.
       Note if the force parameter is PTrue this function will close down
       active logical channels to meet the new bandwidth requirement.
      */
    virtual PBoolean SetBandwidthAvailable(
      unsigned newBandwidth,    ///<  New bandwidth limit
      PBoolean force = PFalse        ///<  Force bandwidth limit
    );

    /**Get the bandwidth currently used.
       This totals the open channels and returns the total bandwidth used in
       100's of bits/sec
      */
    virtual unsigned GetBandwidthUsed() const;
  //@}

  /**@name Indications */
  //@{
    /**Get the real user input indication transmission mode.
       This will return the user input mode that will actually be used for
       transmissions. It will be the value of GetSendUserInputMode() provided
       the remote endpoint is capable of that mode.
      */
    virtual SendUserInputModes GetRealSendUserInputMode() const;

    /**Send a user input indication to the remote endpoint.
       This is for sending arbitrary strings as user indications.

       The user indication is sent according to the sendUserInputMode member
       variable. If SendUserInputAsString then this uses an H.245 "string"
       UserInputIndication pdu sending the entire string in one go. If
       SendUserInputAsTone then a separate H.245 "signal" UserInputIndication
       pdu is sent for each character. If SendUserInputAsInlineRFC2833 then
       the indication is inserted into the outgoing audio stream as an RFC2833
       RTP data pdu.

       SendUserInputAsSeparateRFC2833 is not yet supported.
      */
    virtual PBoolean SendUserInputString(
      const PString & value                   ///<  String value of indication
    );

    /**Send a user input indication to the remote endpoint.
       This sends DTMF emulation user input. If something more sophisticated
       than the simple tones that can be sent using the SendUserInput()
       function.

       A duration of zero indicates that no duration is to be indicated.
       A non-zero logical channel indicates that the tone is to be syncronised
       with the logical channel at the rtpTimestamp value specified.

       The tone parameter must be one of "0123456789#*ABCD!" where '!'
       indicates a hook flash. If tone is a ' ' character then a
       signalUpdate PDU is sent that updates the last tone indication
       sent. See the H.245 specifcation for more details on this.

       The user indication is sent according to the sendUserInputMode member
       variable. If SendUserInputAsString then this uses an H.245 "string"
       UserInputIndication pdu sending the entire string in one go. If
       SendUserInputAsTone then a separate H.245 "signal" UserInputIndication
       pdu is sent for each character. If SendUserInputAsInlineRFC2833 then
       the indication is inserted into the outgoing audio stream as an RFC2833
       RTP data pdu.

       SendUserInputAsSeparateRFC2833 is not yet supported.
      */
    virtual PBoolean SendUserInputTone(
      char tone,             ///<  DTMF tone code
      unsigned duration = 0  ///<  Duration of tone in milliseconds
    );

    /**Send a user input indication to the remote endpoint.
       This is for sending arbitrary strings as user indications.

       This always uses a Q.931 Keypad Information Element in a Information
       pdu sending the entire string in one go.
      */
    virtual PBoolean SendUserInputIndicationQ931(
      const PString & value                   ///<  String value of indication
    );

    /**Send a user input indication to the remote endpoint.
       This is for sending arbitrary strings as user indications.

       This always uses an H.245 "string" UserInputIndication pdu sending the
       entire string in one go.
      */
    virtual PBoolean SendUserInputIndicationString(
      const PString & value                   ///<  String value of indication
    );

    /**Send a user input indication to the remote endpoint.
       This sends DTMF emulation user input.This uses an H.245 "signal"
       UserInputIndication pdu.
      */
    virtual PBoolean SendUserInputIndicationTone(
      char tone,                   ///<  DTMF tone code
      unsigned duration = 0,       ///<  Duration of tone in milliseconds
      unsigned logicalChannel = 0, ///<  Logical channel number for RTP sync.
      unsigned rtpTimestamp = 0    ///<  RTP timestamp in logical channel sync.
    );

    /**Send a user input indication to the remote endpoint.
       The two forms are for basic user input of a simple string using the
       SendUserInput() function or a full DTMF emulation user input using the
       SendUserInputTone() function.

       An application could do more sophisticated usage by filling in the 
       H245_UserInputIndication structure directly ans using this function.
      */
    virtual PBoolean SendUserInputIndication(
      const H245_UserInputIndication & pdu    ///<  Full user indication PDU
    );

    /**Call back for remote enpoint has sent user input.
       The default behaviour calls OnUserInputString() if the PDU is of the
       alphanumeric type, or OnUserInputTone() if of a tone type.
      */
    virtual void OnUserInputIndication(
      const H245_UserInputIndication & pdu  ///<  Full user indication PDU
    );
  //@}

  /**@name RTP Session Management */
  //@{
    /**Get an H323 RTP session for the specified ID.
       If there is no session of the specified ID, NULL is returned.
      */
    virtual H323_RTP_Session * GetSessionCallbacks(
      unsigned sessionID
    ) const;

    /**Use an RTP session for the specified ID and for the given direction.
       If there is no session of the specified ID, a new one is created using
       the information provided in the tranport parameter. If the system
       does not support the specified transport, NULL is returned.

       If this function is used, then the ReleaseSession() function MUST be
       called or the session is never deleted for the lifetime of the H323
       connection.
      */
    virtual RTP_Session * UseSession(
      const OpalTransport & transport,
                   unsigned sessionID,
      const OpalMediaType & mediatype,  ///<  media type
                  RTP_QOS * rtpqos = NULL
    );

    /**Release the session. If the session ID is not being used any more any
       clients via the UseSession() function, then the session is deleted.
     */
    virtual void ReleaseSession(
      unsigned sessionID
    );

    /**Callback from the RTP session for statistics monitoring.
       This is called every so many packets on the transmitter and receiver
       threads of the RTP session indicating that the statistics have been
       updated.

       The default behaviour calls H323EndPoint::OnRTPStatistics().
      */
    virtual void OnRTPStatistics(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Get the names of the codecs in use for the RTP session.
       If there is no session of the specified ID, an empty string is returned.
      */
    virtual PString GetSessionCodecNames(
      unsigned sessionID
    ) const;

  //@}

  /**@name Request Mode Changes */
  //@{
    /**Make a request to mode change to remote.
       This asks the remote system to stop it transmitters and start sending
       one of the combinations specifed.

       The modes are separated in the string by \n characters, and all of the
       channels (capabilities) are strings separated by \t characters. Thus a
       very simple mode change would be "T.38" which requests that the remote
       start sending T.38 data and nothing else. A more complicated example
       would be "G.723\tH.261\nG.729\tH.261\nG.728" which indicates that the
       remote should either start sending G.723 and H.261, G.729 and H.261 or
       just G.728 on its own.

       Returns PFalse if a mode change is currently in progress, only one mode
       change may be done at a time.
      */
    virtual PBoolean RequestModeChange(
      const PString & newModes  ///<  New modes to select
    );

    /**Make a request to mode change to remote.
       This asks the remote system to stop it transmitters and start sending
       one of the combinations specifed.

       Returns PFalse if a mode change is currently in progress, only one mode
       change may be done at a time.
      */
    virtual PBoolean RequestModeChange(
      const H245_ArrayOf_ModeDescription & newModes  ///<  New modes to select
    );

    /**Received request for mode change from remote.
      */
    virtual PBoolean OnRequestModeChange(
      const H245_RequestMode & pdu,     ///<  Received PDU
      H245_RequestModeAck & ack,        ///<  Ack PDU to send
      H245_RequestModeReject & reject,  ///<  Reject PDU to send
      PINDEX & selectedMode           ///<  Which mode was selected
    );

    /**Completed request for mode change from remote.
       This is a call back that accurs after the ack has been sent to the
       remote as indicated by the OnRequestModeChange() return result. This
       function is intended to actually implement the mode change after it
       had been accepted.
      */
    virtual void OnModeChanged(
      const H245_ModeDescription & newMode
    );

    /**Received acceptance of last mode change request.
       This callback indicates that the RequestModeChange() was accepted by
       the remote endpoint.
      */
    virtual void OnAcceptModeChange(
      const H245_RequestModeAck & pdu  ///<  Received PDU
    );

    /**Received reject of last mode change request.
       This callback indicates that the RequestModeChange() was accepted by
       the remote endpoint.
      */
    virtual void OnRefusedModeChange(
      const H245_RequestModeReject * pdu  ///<  Received PDU, if NULL is a timeout
    );
  //@}

  /**@name Other services */
  //@{
    /**Request a mode change to T.38 data.
       Note this function is strictly H.323 and does operate correctly in the
       OPAL media stream model. It is maintained for backward compatibility
       with older applications only.
      */
    virtual PBoolean RequestModeChangeT38(
      const char * capabilityNames = "T.38\nT38FaxUDP"
    );

    /**Get separate H.235 authentication for the connection.
       This allows an individual ARQ to override the authentical credentials
       used in H.235 based RAS for this particular connection.

       A return value of PFalse indicates to use the default credentials of the
       endpoint, while PTrue indicates that new credentials are to be used.

       The default behavour does nothing and returns PFalse.
      */
    virtual PBoolean GetAdmissionRequestAuthentication(
      const H225_AdmissionRequest & arq,  ///<  ARQ being constructed
      H235Authenticators & authenticators ///<  New authenticators for ARQ
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Get the owner endpoint for this connection.
     */
    H323EndPoint & GetEndPoint() const { return endpoint; }

    /**Get the call direction for this connection.
     */
    PBoolean HadAnsweredCall() const { return !originating; }

    /**Determined if connection is gatekeeper routed.
     */
    PBoolean IsGatekeeperRouted() const { return gatekeeperRouted; }

    /**Get the distinctive ring code for incoming call.
       This returns an integer from 0 to 7 that may indicate to an application
       that different ring cadences are to be used.
      */
    unsigned GetDistinctiveRing() const { return distinctiveRing; }

    /**Set the distinctive ring code for outgoing call.
       This sets the integer from 0 to 7 that will be used in the outgoing
       Setup PDU. Note this must be called either immediately after
       construction or during the OnSendSignalSetup() callback function so the
       member variable is set befor ethe PDU is sent.
      */
    void SetDistinctiveRing(unsigned pattern) { distinctiveRing = pattern&7; }

    /**Get the internal OpenH323 call token for this connection.
       Deprecated, only used for backward compatibility.
     */
    const PString & GetCallToken() const { return GetToken(); }

    /**Get the call reference for this connection.
     */
    unsigned GetCallReference() const { return callReference; }

    /**Get the call identifier for this connection.
     */
    inline const OpalGloballyUniqueID & GetCallIdentifier() const 
    { return callIdentifier; }

    /**Get the protocol-specific unique identifier for this connection.
     */
    virtual PString GetIdentifier() const;

    /**Get the conference identifier for this connection.
     */
    const OpalGloballyUniqueID & GetConferenceIdentifier() const { return conferenceIdentifier; }

    /**Set the local name/alias from information in the PDU.
      */
    void SetLocalPartyName(const PString & name);

    /**Get the list of all alias names this connection is using.
      */
    const PStringList & GetLocalAliasNames() const { return localAliasNames; }

    /**Set the name/alias of remote end from information in the PDU.
      */
    virtual void SetRemotePartyInfo(
      const H323SignalPDU & pdu ///<  PDU from which to extract party info.
    );

    /**Set the name/alias of remote end from information in the PDU.
      */
    void SetRemoteApplication(
      const H225_EndpointType & pdu ///<  PDU from which to extract application info.
    );
    
    /**Get the remote party address.
       This will return the "best guess" at an address to use in a
       to call the user again later.
      */
    PString GetRemotePartyURL() const;
    
    /**Get the remotes capability table for this connection.
     */
    const H323Capabilities & GetLocalCapabilities() const { return localCapabilities; }

    /**Get the remotes capability table for this connection.
     */
    const H323Capabilities & GetRemoteCapabilities() const { return remoteCapabilities; }

    /**Get the maximum audio jitter delay.
     */
    unsigned GetRemoteMaxAudioDelayJitter() const { return remoteMaxAudioDelayJitter; }

    /**Get the signalling channel being used.
      */
    const H323Transport * GetSignallingChannel() const { return signallingChannel; }

    /**Get the signalling channel protocol version number.
      */
    unsigned GetSignallingVersion() const { return h225version; }

    /**Get the control channel being used (may return signalling channel).
      */
    const H323Transport & GetControlChannel() const;

    /**Get the control channel being used (may return signalling channel).
      */
    OpalTransport & GetTransport() const;

    /**Get the control channel protocol version number.
      */
    unsigned GetControlVersion() const { return h245version; }

    /**Get the UUIE PDU monitor bit mask.
     */
    unsigned GetUUIEsRequested() const { return uuiesRequested; }

    /**Set the UUIE PDU monitor bit mask.
     */
    void SetUUIEsRequested(unsigned mask) { uuiesRequested = mask; }

    /**Get the iNow Gatekeeper Access Token OID.
     */
    const PString GetGkAccessTokenOID() const { return gkAccessTokenOID; }

    /**Set the iNow Gatekeeper Access Token OID.
     */
    void SetGkAccessTokenOID(const PString & oid) { gkAccessTokenOID = oid; }

    /**Get the iNow Gatekeeper Access Token data.
     */
    const PBYTEArray & GetGkAccessTokenData() const { return gkAccessTokenData; }

    /**Set the Destionation Extra Call Info memeber.
     */
    void SetDestExtraCallInfo(
      const PString & info
    ) { destExtraCallInfo = info; }

    /** Set the remote call waiting flag
     */
    void SetRemotCallWaiting(const unsigned value) { remoteCallWaiting = value; }

    /**How many caller's are waiting on the remote endpoint?
      -1 - None
       0 - Just this connection
       n - n plus this connection
     */
    int GetRemoteCallWaiting() const { return remoteCallWaiting; }

    /**Set the enforced duration limit for the call.
       This starts a timer that will automatically shut down the call when it
       expires.
      */
    void SetEnforcedDurationLimit(
      unsigned seconds  ///<  max duration of call in seconds
    );
  //@}
    
#if OPAL_H239
    /**Get the local H.239 control capability.
     */
    bool GetLocalH239Control() const { return m_h239Control; }

    /**Set the local H.239 control capability.
     */
    void SetLocalH239Control(
      bool on   ///< H.239 control capability is to be sent to remote
    ) { m_h239Control = on; }

    /**Get the remote H.239 control capability.
     */
    bool GetRemoteH239Control() const;

    /**Get the remote H.239 options.
     */
    OpalMediaFormatList GetRemoteH239Formats() const;
#endif

    virtual PBoolean OnSendFeatureSet(unsigned, H225_FeatureSet &) const;
    
    virtual void OnReceiveFeatureSet(unsigned, const H225_FeatureSet &) const;

#if OPAL_H460
    /** Get the connection FeatureSet
     */
    virtual H460_FeatureSet * GetFeatureSet();
#endif

    
#if OPAL_H450
    /**
     * get the H4507 handler
     * @return a reference to the  H4507 handler
     */
    H4507Handler&  getH4507handler(){return *h4507handler;}
#endif

    virtual void OnMediaCommand(OpalMediaCommand & note, INT extra);
    
  protected:
    /**Internal function to check if call established.
       This checks all the criteria for establishing a call an initiating the
       starting of media channels, if they have not already been started via
       the fast start algorithm.
    */
    virtual void InternalEstablishedConnectionCheck();
    PBoolean InternalEndSessionCheck(PPER_Stream & strm);
    void SetRemoteVersions(const H225_ProtocolIdentifier & id);
    void MonitorCallStatus();
    PDECLARE_NOTIFIER(PThread, H323Connection, StartOutgoing);
    PDECLARE_NOTIFIER(PThread, H323Connection, NewOutgoingControlChannel);
    PDECLARE_NOTIFIER(PThread, H323Connection, NewIncomingControlChannel);

    H323EndPoint & endpoint;

    H323TransportAddress m_remoteConnectAddress;
    int                  remoteCallWaiting; // Number of call's waiting at the remote endpoint
    PBoolean                 gatekeeperRouted;
    unsigned             distinctiveRing;
    unsigned             callReference;
    OpalGloballyUniqueID callIdentifier;
    OpalGloballyUniqueID conferenceIdentifier;

    PString            localDestinationAddress;
    PStringList        localAliasNames;
    H323Capabilities   localCapabilities; // Capabilities local system supports
    PString            destExtraCallInfo;
    H323Capabilities   remoteCapabilities; // Capabilities remote system supports
    unsigned           remoteMaxAudioDelayJitter;
    PTimer             roundTripDelayTimer;
    WORD               maxAudioDelayJitter;
    unsigned           uuiesRequested;
    PString            gkAccessTokenOID;
    PBYTEArray         gkAccessTokenData;
    PBoolean               addAccessTokenToSetup;

    H323Transport * signallingChannel;
    H323Transport * controlChannel;
    OpalListener  * controlListener;
    PBoolean            h245Tunneling;
    H323SignalPDU * h245TunnelRxPDU;
    H323SignalPDU * h245TunnelTxPDU;
    H323SignalPDU * setupPDU;
    H323SignalPDU * alertingPDU;
    H323SignalPDU * connectPDU;
    H323SignalPDU * progressPDU;

    enum ConnectionStates {
      NoConnectionActive,
      AwaitingGatekeeperAdmission,
      AwaitingTransportConnect,
      AwaitingSignalConnect,
      AwaitingLocalAnswer,
      HasExecutedSignalConnect,
      EstablishedConnection,
      ShuttingDownConnection,
      NumConnectionStates
    } connectionState;

    unsigned   h225version;
    unsigned   h245version;
    PBoolean       h245versionSet;
    PBoolean doH245inSETUP;
    PBoolean lastPDUWasH245inSETUP;

    PBoolean mustSendDRQ;
    PBoolean mediaWaitForConnect;
    PBoolean transmitterSidePaused;
    bool     remoteTransmitPaused;
    PBoolean earlyStart;
    PString    t38ModeChangeCapabilities;
    PSyncPoint digitsWaitFlag;
    PBoolean       endSessionNeeded;
    PSyncPoint endSessionReceived;
    PTimer     enforcedDurationLimit;

    // Used as part of a local call hold operation involving MOH
    PChannel * holdMediaChannel;
    PBoolean       isConsultationTransfer;

    /** Call Intrusion flag and parameters */
    PBoolean     isCallIntrusion;
    unsigned callIntrusionProtectionLevel;

    enum FastStartStates {
      FastStartDisabled,
      FastStartInitiate,
      FastStartResponse,
      FastStartAcknowledged,
      NumFastStartStates
    };
    FastStartStates        fastStartState;
    H323LogicalChannelList fastStartChannels;
    OpalMediaStreamPtr     fastStartMediaStream;
    
#if PTRACING
    static const char * GetConnectionStatesName(ConnectionStates s);
    friend ostream & operator<<(ostream & o, ConnectionStates s) { return o << GetConnectionStatesName(s); }
    static const char * GetFastStartStateName(FastStartStates s);
    friend ostream & operator<<(ostream & o, FastStartStates s) { return o << GetFastStartStateName(s); }
#endif


    // The following pointers are to protocol procedures, they are pointers to
    // hide their complexity from the H323Connection classes users.
    H245NegMasterSlaveDetermination  * masterSlaveDeterminationProcedure;
    H245NegTerminalCapabilitySet     * capabilityExchangeProcedure;
    H245NegLogicalChannels           * logicalChannels;
    H245NegRequestMode               * requestModeProcedure;
    H245NegRoundTripDelay            * roundTripDelayProcedure;

#if OPAL_H239
    bool m_h239Control;
#endif

#if OPAL_H450
    H450xDispatcher * h450dispatcher;
    H4502Handler    * h4502handler;
    H4504Handler    * h4504handler;
    H4506Handler    * h4506handler;
    H4507Handler    * h4507handler;
    H45011Handler   * h45011handler;
#endif

#if OPAL_H460
    H460_FeatureSet * features;
#endif

  private:
    PChannel * SwapHoldMediaChannels(PChannel * newChannel);
};


PDICTIONARY(H323CallIdentityDict, PString, H323Connection);


#endif // OPAL_H323

#endif // OPAL_H323_H323CON_H


/////////////////////////////////////////////////////////////////////////////
