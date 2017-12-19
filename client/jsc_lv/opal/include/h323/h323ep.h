/*
 * h323ep.h
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
 * $Revision: 23525 $
 * $Author: csoutheren $
 * $Date: 2009-09-24 06:02:24 +0000 (Thu, 24 Sep 2009) $
 */

#ifndef OPAL_H323_H323EP_H
#define OPAL_H323_H323EP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_H323

#include <opal/rtpep.h>
#include <opal/manager.h>
#include <opal/call.h>
#include <opal/transports.h>
#include <h323/h323con.h>
#include <h323/h323caps.h>
#include <h323/h235auth.h>
#include <asn/h225.h>

#if OPAL_H460
#include <h460/h4601.h>
#endif


class H225_EndpointType;
class H225_VendorIdentifier;
class H225_H221NonStandard;
class H225_ServiceControlDescriptor;
class H225_FeatureSet;

class H235SecurityInfo;

class H323Gatekeeper;
class H323SignalPDU;
class H323ServiceControlSession;

///////////////////////////////////////////////////////////////////////////////

/**This class manages the H323 endpoint.
   An endpoint may have zero or more listeners to create incoming connections
   or zero or more outgoing connections initiated via the MakeCall() function.
   Once a conection exists it is managed by this class instance.

   The main thing this class embodies is the capabilities of the application,
   that is the codecs and protocols it is capable of.

   An application may create a descendent off this class and overide the
   CreateConnection() function, if they require a descendent of H323Connection
   to be created. This would be quite likely in most applications.
 */
class H323EndPoint : public OpalRTPEndPoint
{
  PCLASSINFO(H323EndPoint, OpalRTPEndPoint);

  public:
    enum {
      DefaultTcpSignalPort = 1720
    };

  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    H323EndPoint(
      OpalManager & manager
    );

    /**Destroy endpoint.
     */
    ~H323EndPoint();
  //@}

  /**@name Overrides from OpalEndPoint */
  //@{
    /**Shut down the endpoint, this is called by the OpalManager just before
       destroying the object and can be handy to make sure some things are
       stopped before the vtable gets clobbered.
      */
    virtual void ShutDown();

    /**Set up a connection to a remote party.
       This is called from the OpalManager::SetUpConnection() function once
       it has determined that this is the endpoint for the protocol.

       The general form for this party parameter is:

            [proto:][alias@][transport$]address[:port]

       where the various fields will have meanings specific to the endpoint
       type. For example, with H.323 it could be "h323:Fred@site.com" which
       indicates a user Fred at gatekeeper size.com. Whereas for the PSTN
       endpoint it could be "pstn:5551234" which is to call 5551234 on the
       first available PSTN line.

       The proto field is optional when passed to a specific endpoint. If it
       is present, however, it must agree with the endpoints protocol name or
       PFalse is returned.

       This function usually returns almost immediately with the connection
       continuing to occur in a new background thread.

       If PFalse is returned then the connection could not be established. For
       example if a PSTN endpoint is used and the assiciated line is engaged
       then it may return immediately. Returning a non-NULL value does not
       mean that the connection will succeed, only that an attempt is being
       made.

       The default behaviour is pure.
     */
    virtual PSafePtr<OpalConnection> MakeConnection(
      OpalCall & call,                  ///<  Owner of connection
      const PString & party,            ///<  Remote party to call
      void * userData  = NULL,          ///<  Arbitrary data to pass to connection
      unsigned int options = NULL,      ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions = NULL
    );
  //@}

  /**@name Set up functions */
  //@{
    /**Set the endpoint information in H225 PDU's.
      */
    virtual void SetEndpointTypeInfo(
      H225_EndpointType & info
    ) const;

    /**Set the vendor information in H225 PDU's.
      */
    virtual void SetVendorIdentifierInfo(
      H225_VendorIdentifier & info
    ) const;

    /**Set the H221NonStandard information in H225 PDU's.
      */
    virtual void SetH221NonStandardInfo(
      H225_H221NonStandard & info
    ) const;

    /**Set the Gateway supported protocol default always H.323
      */
    virtual bool SetGatewaySupportedProtocol(
      H225_ArrayOf_SupportedProtocols & protocols
    ) const;

    /**Set the gateway prefixes 
       Override this to set the acceptable prefixes to the gatekeeper
      */
    virtual bool OnSetGatewayPrefixes(
      PStringList & prefixes
    ) const;
  //@}


  /**@name Capabilities */
  //@{
    /**Add a codec to the capabilities table. This will assure that the
       assignedCapabilityNumber field in the codec is unique for all codecs
       installed on this endpoint.

       If the specific instnace of the capability is already in the table, it
       is not added again. Ther can be multiple instances of the same
       capability class however.
     */
    void AddCapability(
      H323Capability * capability   ///<  New codec specification
    );

    /**Set the capability descriptor lists. This is three tier set of
       codecs. The top most level is a list of particular capabilities. Each
       of these consists of a list of alternatives that can operate
       simultaneously. The lowest level is a list of codecs that cannot
       operate together. See H323 section 6.2.8.1 and H245 section 7.2 for
       details.

       If descriptorNum is P_MAX_INDEX, the the next available index in the
       array of descriptors is used. Similarly if simultaneous is P_MAX_INDEX
       the the next available SimultaneousCapabilitySet is used. The return
       value is the index used for the new entry. Note if both are P_MAX_INDEX
       then the return value is the descriptor index as the simultaneous index
       must be zero.

       Note that the capability specified here is automatically added to the
       capability table using the AddCapability() function. A specific
       instance of a capability is only ever added once, so multiple
       SetCapability() calls with the same H323Capability pointer will only
       add that capability once.
     */
    PINDEX SetCapability(
      PINDEX descriptorNum, ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,  ///<  The member of the SimultaneousCapabilitySet to add
      H323Capability * cap  ///<  New capability specification
    );

    /**Add all matching capabilities in list.
       All capabilities that match the specified name are added. See the
       capabilities code for details on the matching algorithm.
      */
    PINDEX AddAllCapabilities(
      PINDEX descriptorNum, ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,  ///<  The member of the SimultaneousCapabilitySet to add
      const PString & name  ///<  New capabilities name, if using "known" one.
    );

    /**Add all user input capabilities to this endpoints capability table.
      */
    void AddAllUserInputCapabilities(
      PINDEX descriptorNum, ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous   ///<  The member of the SimultaneousCapabilitySet to add
    );

    /**Remove capabilites in table.
      */
    void RemoveCapabilities(
      const PStringArray & codecNames
    );

    /**Reorder capabilites in table.
      */
    void ReorderCapabilities(
      const PStringArray & preferenceOrder
    );

    /**Find a capability that has been registered.
     */
    H323Capability * FindCapability(
      const H245_Capability & cap  ///<  H245 capability table entry
    ) const;

    /**Find a capability that has been registered.
     */
    H323Capability * FindCapability(
      const H245_DataType & dataType  ///<  H245 data type of codec
    ) const;

    /**Find a capability that has been registered.
     */
    H323Capability * FindCapability(
      H323Capability::MainTypes mainType,   ///<  Main type of codec
      unsigned subType                      ///<  Subtype of codec
    ) const;
  //@}

  /**@name Gatekeeper management */
  //@{
    /**Use and register with an explicit gatekeeper.
       This will call other functions according to the following table:

           address    identifier   function
           empty      empty        DiscoverGatekeeper()
           non-empty  empty        SetGatekeeper()
           empty      non-empty    LocateGatekeeper()
           non-empty  non-empty    SetGatekeeperZone()

       The localAddress field, if non-empty, indicates the interface on which
       to look for the gatekeeper. An empty string is equivalent to "ip$*:*"
       which is any interface or port.

       If the endpoint is already registered with a gatekeeper that meets
       the same criteria then the gatekeeper is not changed, otherwise it is
       deleted (with unregistration) and new one created and registered to.

       Note that a gatekeeper address of "*" is treated like an empty string
       resulting in gatekeeper discovery.
     */
    PBoolean UseGatekeeper(
      const PString & address = PString::Empty(),     ///<  Address of gatekeeper to use.
      const PString & identifier = PString::Empty(),  ///<  Identifier of gatekeeper to use.
      const PString & localAddress = PString::Empty() ///<  Local interface to use.
    );

    /**Select and register with an explicit gatekeeper.
       This will use the specified transport and a string giving a transport
       dependent address to locate a specific gatekeeper. The endpoint will
       register with that gatekeeper and, if successful, set it as the current
       gatekeeper used by this endpoint.

       Note the transport being passed in will be deleted by this function or
       the H323Gatekeeper object it becomes associated with. Also if transport
       is NULL then a H323TransportUDP is created.
     */
    PBoolean SetGatekeeper(
      const PString & address,          ///<  Address of gatekeeper to use.
      H323Transport * transport = NULL  ///<  Transport over which to talk to gatekeeper.
    );

    /**Select and register with an explicit gatekeeper and zone.
       This will use the specified transport and a string giving a transport
       dependent address to locate a specific gatekeeper. The endpoint will
       register with that gatekeeper and, if successful, set it as the current
       gatekeeper used by this endpoint.

       The gatekeeper identifier is set to the spplied parameter to allow the
       gatekeeper to either allocate a zone or sub-zone, or refuse to register
       if the zones do not match.

       Note the transport being passed in will be deleted by this function or
       the H323Gatekeeper object it becomes associated with. Also if transport
       is NULL then a H323TransportUDP is created.
     */
    PBoolean SetGatekeeperZone(
      const PString & address,          ///<  Address of gatekeeper to use.
      const PString & identifier,       ///<  Identifier of gatekeeper to use.
      H323Transport * transport = NULL  ///<  Transport over which to talk to gatekeeper.
    );

    /**Locate and select gatekeeper.
       This function will use the automatic gatekeeper discovery methods to
       locate the gatekeeper on the particular transport that has the specified
       gatekeeper identifier name. This is often the "Zone" for the gatekeeper.

       Note the transport being passed in will be deleted becomes owned by the
       H323Gatekeeper created by this function and will be deleted by it. Also
       if transport is NULL then a H323TransportUDP is created.
     */
    PBoolean LocateGatekeeper(
      const PString & identifier,       ///<  Identifier of gatekeeper to locate.
      H323Transport * transport = NULL  ///<  Transport over which to talk to gatekeeper.
    );

    /**Discover and select gatekeeper.
       This function will use the automatic gatekeeper discovery methods to
       locate the first gatekeeper on a particular transport.

       Note the transport being passed in will be deleted becomes owned by the
       H323Gatekeeper created by this function and will be deleted by it. Also
       if transport is NULL then a H323TransportUDP is created.
     */
    PBoolean DiscoverGatekeeper(
      H323Transport * transport = NULL  ///<  Transport over which to talk to gatekeeper.
    );

    /**Create a gatekeeper.
       This allows the application writer to have the gatekeeper as a
       descendent of the H323Gatekeeper in order to add functionality to the
       base capabilities in the library.

       The default creates an instance of the H323Gatekeeper class.
     */
    virtual H323Gatekeeper * CreateGatekeeper(
      H323Transport * transport  ///<  Transport over which gatekeepers communicates.
    );

    /**Get the gatekeeper we are registered with.
     */
    H323Gatekeeper * GetGatekeeper() const { return gatekeeper; }

    /**Return if endpoint is registered with gatekeeper.
      */
    PBoolean IsRegisteredWithGatekeeper() const;

    /**Unregister and delete the gatekeeper we are registered with.
       The return value indicates PFalse if there was an error during the
       unregistration. However the gatekeeper is still removed and its
       instance deleted regardless of this error.
     */
    PBoolean RemoveGatekeeper(
      int reason = -1    ///<  Reason for gatekeeper removal
    );

    /**Set the H.235 password for the gatekeeper.
      */
    virtual void SetGatekeeperPassword(
      const PString & password,
      const PString & username = PString::Empty()
    );

    /**Get the H.235 username for the gatekeeper.
      */
    virtual const PString & GetGatekeeperUsername() const { return gatekeeperUsername; }

    /**Get the H.235 password for the gatekeeper.
      */
    virtual const PString & GetGatekeeperPassword() const { return gatekeeperPassword; }

    /**Create a list of authenticators for gatekeeper.
      */
    virtual H235Authenticators CreateAuthenticators();

    /**Called when the gatekeeper sends a GatekeeperConfirm
      */
    virtual void  OnGatekeeperConfirm();

    /**Called when the gatekeeper sends a GatekeeperReject
      */
    virtual void  OnGatekeeperReject();

    /**Called when the gatekeeper sends a RegistrationConfirm
      */
    virtual void OnRegistrationConfirm();

    /**Called when the gatekeeper sends a RegistrationReject
      */
    virtual void  OnRegistrationReject();
  //@}

  /**@name Connection management */
  //@{
    /**Handle new incoming connetion from listener.
      */
    virtual PBoolean NewIncomingConnection(
      OpalTransport * transport  ///<  Transport connection came in on
    );

    /**Create a connection that uses the specified call.
      */
    virtual H323Connection * CreateConnection(
      OpalCall & call,                         ///<  Call object to attach the connection to
      const PString & token,                   ///<  Call token for new connection
      void * userData,                         ///<  Arbitrary user data from MakeConnection
      OpalTransport & transport,               ///<  Transport for connection
      const PString & alias,                   ///<  Alias for outgoing call
      const H323TransportAddress & address,    ///<  Address for outgoing call
      H323SignalPDU * setupPDU,                ///<  Setup PDU for incoming call
      unsigned options = 0,
      OpalConnection::StringOptions * stringOptions = NULL ///<  complex string options
    );

    /**Setup the transfer of an existing call (connection) to a new remote party
       using H.450.2.  This sends a Call Transfer Setup Invoke message from the
       B-Party (transferred endpoint) to the C-Party (transferred-to endpoint).

       If the transport parameter is NULL the transport is determined from the
       remoteParty description. The general form for this parameter is
       [alias@][transport$]host[:port] where the default alias is the same as
       the host, the default transport is "ip" and the default port is 1720.

       This function returns almost immediately with the transfer occurring in a
       new background thread.

       This function is declared virtual to allow an application to override
       the function and get the new call token of the forwarded call.
     */
    virtual PBoolean SetupTransfer(
      const PString & token,        ///<  Existing connection to be transferred
      const PString & callIdentity, ///<  Call identity of the secondary call (if it exists)
      const PString & remoteParty,  ///<  Remote party to transfer the existing call to
      void * userData = NULL        ///<  user data to pass to CreateConnection
    );

    /**Initiate the transfer of an existing call (connection) to a new remote
       party using H.450.2.  This sends a Call Transfer Initiate Invoke
       message from the A-Party (transferring endpoint) to the B-Party
       (transferred endpoint).
     */
    void TransferCall(
      const PString & token,        ///<  Existing connection to be transferred
      const PString & remoteParty,  ///<  Remote party to transfer the existing call to
      const PString & callIdentity = PString::Empty()
                                    ///<  Call Identity of secondary call if present
    );

    /**Transfer the call through consultation so the remote party in the
       primary call is connected to the called party in the second call
       using H.450.2.  This sends a Call Transfer Identify Invoke message
       from the A-Party (transferring endpoint) to the C-Party
       (transferred-to endpoint).
     */
    void ConsultationTransfer(
      const PString & primaryCallToken,   ///<  Token of primary call
      const PString & secondaryCallToken  ///<  Token of secondary call
    );

    /**Place the call on hold, suspending all media channels (H.450.4)
    * NOTE: Only Local Hold is implemented so far. 
    */
    void HoldCall(
      const PString & token,        ///<  Existing connection to be transferred
      PBoolean localHold   ///<  true for Local Hold, false for Remote Hold
    );

    /** Initiate Call intrusion
        Designed similar to MakeCall function
      */
    PBoolean IntrudeCall(
      const PString & remoteParty,  ///<  Remote party to intrude call
      unsigned capabilityLevel,     ///<  Capability level
      void * userData = NULL        ///<  user data to pass to CreateConnection
    );

    /**Parse a party address into alias and transport components.
       An appropriate transport is determined from the remoteParty parameter.
       The general form for this parameter is [alias@][transport$]host[:port]
       where the default alias is the same as the host, the default transport
       is "ip" and the default port is 1720.
      */
    PBoolean ParsePartyName(
      const PString & party,          ///<  Party name string.
      PString & alias,                ///<  Parsed alias name
      H323TransportAddress & address, ///<  Parsed transport address
      OpalConnection::StringOptions * stringOptions = NULL ///< String options parsed from party name
    );

    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeCall(). if not found it will then search for
       the string representation of the CallIdentifier for the connection, and
       finally try for the string representation of the ConferenceIdentifier.

       Note the caller of this function MUSt call the H323Connection::Unlock()
       function if this function returns a non-NULL pointer. If it does not
       then a deadlock can occur.
      */
    PSafePtr<H323Connection> FindConnectionWithLock(
      const PString & token,     ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite
    );

    /** OnSendSignalSetup is a hook for the appliation to attach H450 info in setup, 
        for instance, H450.7 Activate or Deactivate 
        @param connection the connection associated to the setup
        @param pduSetup the setup message to modify
        @return if false, do no send the setup pdu
      */
      
    virtual PBoolean OnSendSignalSetup(H323Connection & connection,
                                   H323SignalPDU & setupPDU);

    /**Adjust call proceeding PDU being sent. This function is called from
       the OnReceivedSignalSetup() function before it sends the Call
       Proceeding PDU. It gives an opportunity for an application to alter
       the request before transmission to the other endpoint. If this function
       returns PFalse then the Call Proceeding PDU is not sent at all.

       The default behaviour simply returns PTrue.
       @param connection the connection associated to the call proceeding
       @param callProceedingPDU the call processding to modify
       @return if false, do no send the connect pdu  
     */
    virtual PBoolean OnSendCallProceeding(
      H323Connection & connection,
      H323SignalPDU & callProceedingPDU
    );

    /**Adjust call connect PDU being sent. This function is called from
       the H323Connection::SetConnected function before it sends the connect  PDU. 
       It gives an opportunity for an application to alter
       the request before transmission to the other endpoint. If this function
       returns PFalse then the connect PDU is not sent at all.

       The default behaviour simply returns PTrue.
       @param connection the connection associated to the connect
       @param connectPDU the connect to modify
       @return if false, do no send the connect pdu  
     */
    virtual PBoolean OnSendConnect(
      H323Connection & connection,
      H323SignalPDU & connectPDU
    );
    
    /**Call back for incoming call.
       This function is called from the OnReceivedSignalSetup() function
       before it sends the Alerting PDU. It gives an opportunity for an
       application to alter the reply before transmission to the other
       endpoint.

       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns PTrue.
     */
    virtual PBoolean OnIncomingCall(
      H323Connection & connection,    ///<  Connection that was established
      const H323SignalPDU & setupPDU,   ///<  Received setup PDU
      H323SignalPDU & alertingPDU       ///<  Alerting PDU to send
    );

    /**Called when an outgoing call connects
       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns PTrue.
      */
    virtual PBoolean OnOutgoingCall(
      H323Connection & conn, 
      const H323SignalPDU & connectPDU
    );

    /**Handle a connection transfer.
       This gives the application an opportunity to abort the transfer.
       The default behaviour just returns PTrue.
      */
    virtual PBoolean OnCallTransferInitiate(
      H323Connection & connection,    ///<  Connection to transfer
      const PString & remoteParty     ///<  Party transferring to.
    );

    /**Handle a transfer via consultation.
       This gives the transferred-to user an opportunity to abort the transfer.
       The default behaviour just returns PTrue.
      */
    virtual PBoolean OnCallTransferIdentify(
      H323Connection & connection    ///<  Connection to transfer
    );

    /**
      * Callback for ARQ send to a gatekeeper. This allows the endpoint
      * to change or check fields in the ARQ before it is sent.
      */
    virtual void OnSendARQ(
      H323Connection & conn,
      H225_AdmissionRequest & arq
    );

    /**Call back for answering an incoming call.
       This function is a H.323 specific version of OpalEndPoint::OnAnswerCall
       that contains additional information that applies only to H.323.

       By default this calls OpalEndPoint::OnAnswerCall, which returns
     */
    virtual OpalConnection::AnswerCallResponse OnAnswerCall(
      H323Connection & connection,    ///< Connection that was established
      const PString & callerName,       ///< Name of caller
      const H323SignalPDU & setupPDU,   ///< Received setup PDU
      H323SignalPDU & connectPDU,       ///< Connect PDU to send. 
      H323SignalPDU & progressPDU        ///< Progress PDU to send. 
    );
    virtual OpalConnection::AnswerCallResponse OnAnswerCall(
       OpalConnection & connection,
       const PString & caller
    );

    /**Call back for remote party being alerted.
       This function is called from the SendSignalSetup() function after it
       receives the optional Alerting PDU from the remote endpoint. That is
       when the remote "phone" is "ringing".

       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns PTrue.
     */
    virtual PBoolean OnAlerting(
      H323Connection & connection,    ///<  Connection that was established
      const H323SignalPDU & alertingPDU,  ///<  Received Alerting PDU
      const PString & user                ///<  Username of remote endpoint
    );

    /** A call back function when the alerting is about to be sent, 
        can be used by the application to alter the alerting Pdu
        @return if PFalse, then the alerting is not sent
     */
    virtual PBoolean OnSendAlerting(
      H323Connection & connection,  ///< onnection that was established
      H323SignalPDU & alerting,     ///< Alerting PDU to modify
      const PString & calleeName,   ///< Name of endpoint being alerted.
      PBoolean withMedia                ///< Open media with alerting
    );

    /** A call back function when the alerting has been sent, can be used by the application 
        to send the connect as soon as the alerting has been sent.
     */
    virtual PBoolean OnSentAlerting(
      H323Connection & connection
    );

    /**A call back function when a connection indicates it is to be forwarded.
       An H323 application may handle this call back so it can make
       complicated decisions on if the call forward ius to take place. If it
       decides to do so it must call MakeCall() and return PTrue.

       The default behaviour simply returns PFalse and that the automatic
       call forwarding should take place. See ForwardConnection()
      */
    virtual PBoolean OnConnectionForwarded(
      H323Connection & connection,    ///<  Connection to be forwarded
      const PString & forwardParty,   ///<  Remote party to forward to
      const H323SignalPDU & pdu       ///<  Full PDU initiating forwarding
    );

    /**Forward the call using the same token as the specified connection.
       Return PTrue if the call is being redirected.

       The default behaviour will replace the current call in the endpoints
       call list using the same token as the call being redirected. Not that
       even though the same token is being used the actual object is
       completely mad anew.
      */
    virtual PBoolean ForwardConnection(
      H323Connection & connection,    ///<  Connection to be forwarded
      const PString & forwardParty,   ///<  Remote party to forward to
      const H323SignalPDU & pdu       ///<  Full PDU initiating forwarding
    );

    /**A call back function whenever a connection is established.
       This indicates that a connection to a remote endpoint was established
       with a control channel and zero or more logical channels.

       The default behaviour does nothing.
      */
    virtual void OnConnectionEstablished(
      H323Connection & connection,    ///<  Connection that was established
      const PString & token           ///<  Token for identifying connection
    );

    /**Determine if a connection is established.
      */
    virtual PBoolean IsConnectionEstablished(
      const PString & token   ///<  Token for identifying connection
    );

    /**A call back function whenever a connection is broken.
       This indicates that a connection to a remote endpoint is no longer
       available.

       The default behaviour does nothing.
      */
    virtual void OnConnectionCleared(
      H323Connection & connection,    ///<  Connection that was established
      const PString & token           ///<  Token for identifying connection
    );
  //@}


  /**@name Logical Channels management */
  //@{
    /**Call back for opening a logical channel.

       The default behaviour simply returns PTrue.
      */
    virtual PBoolean OnStartLogicalChannel(
      H323Connection & connection,    ///<  Connection for the channel
      H323Channel & channel           ///<  Channel being started
    );

    /**Call back for closed a logical channel.

       The default behaviour does nothing.
      */
    virtual void OnClosedLogicalChannel(
      H323Connection & connection,    ///<  Connection for the channel
      const H323Channel & channel     ///<  Channel being started
    );

    /**Callback from the RTP session for statistics monitoring.
       This is called every so many packets on the transmitter and receiver
       threads of the RTP session indicating that the statistics have been
       updated.

       The default behaviour does nothing.
      */
    virtual void OnRTPStatistics(
      const H323Connection & connection,  ///<  Connection for the channel
      const RTP_Session & session         ///<  Session with statistics
    ) const;

    /**Call back from GK admission confirm to notify the 
     * Endpoint it is behind a NAT (GNUGK Gatekeeper).
     * The default does nothing. 
     * Override this to notify the user they are behind a NAT.
     */
    virtual void OnGatekeeperNATDetect(
      PIPSocket::Address publicAddr,         ///> Public address as returned by the Gatekeeper
      PString & gkIdentifier,                ///> Identifier at the gatekeeper
      H323TransportAddress & gkRouteAddress  ///> Gatekeeper Route Address
    );
  //@}

  /**@name Service Control */
  //@{
    /**Call back for HTTP based Service Control.
       An application may override this to use an HTTP based channel using a
       resource designated by the session ID. For example the session ID can
       correspond to a browser window and the 

       The default behaviour does nothing.
      */
    virtual void OnHTTPServiceControl(
      unsigned operation,  ///<  Control operation
      unsigned sessionId,  ///<  Session ID for HTTP page
      const PString & url  ///<  URL to use.
    );

    /**Call back for call credit information.
       An application may override this to display call credit information
       on registration, or when a call is started.

       The canDisplayAmountString member variable must also be set to PTrue
       for this to operate.

       The default behaviour does nothing.
      */
    virtual void OnCallCreditServiceControl(
      const PString & amount,  ///<  UTF-8 string for amount, including currency.
      PBoolean mode          ///<  Flag indicating that calls will debit the account.
    );

    /**Handle incoming service control session information.
       Default behaviour calls session.OnChange()
     */
    virtual void OnServiceControlSession(
      unsigned type,
      unsigned sessionid,
      const H323ServiceControlSession & session,
      H323Connection * connection
    );

    /**Create the service control session object.
     */
    virtual H323ServiceControlSession * CreateServiceControlSession(
      const H225_ServiceControlDescriptor & contents
    );
  //@}

  /**@name Additional call services */
  //@{
    /** Called when an endpoint receives a SETUP PDU with a
        conference goal of "invite"
      
        The default behaviour is to return PFalse, which will close the connection
     */
    virtual PBoolean OnConferenceInvite(
      const H323SignalPDU & setupPDU
    );

    /** Called when an endpoint receives a SETUP PDU with a
        conference goal of "callIndependentSupplementaryService"
      
        The default behaviour is to return PFalse, which will close the connection
     */
    virtual PBoolean OnCallIndependentSupplementaryService(
      const H323SignalPDU & setupPDU
    );

    /** Called when an endpoint receives a SETUP PDU with a
        conference goal of "capability_negotiation"
      
        The default behaviour is to return PFalse, which will close the connection
     */
    virtual PBoolean OnNegotiateConferenceCapabilities(
      const H323SignalPDU & setupPDU
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Set the default local party name for all connections on this endpoint.
      */
    virtual void SetDefaultLocalPartyName(
      const PString & name  /// Name for local party
    );

    /**Set the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.

       Note that this name is technically the first alias for the endpoint.
       Additional aliases may be added by the use of the AddAliasName()
       function, however that list will be cleared when this function is used.
     */
    virtual void SetLocalUserName(
      const PString & name  ///<  Local name of endpoint (prime alias)
    );

    /**Get the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.
     */
    virtual const PString & GetLocalUserName() const { return localAliasNames.front(); }

    /**Add an alias name to be used for the local end of any connections. If
       the alias name already exists in the list then is is not added again.

       The list defaults to the value set in the SetLocalUserName() function.
       Note that calling SetLocalUserName() will clear the alias list.
     */
    PBoolean AddAliasName(
      const PString & name  ///<  New alias name to add
    );

    /**Remove an alias name used for the local end of any connections. 
       defaults to an empty list.
     */
    PBoolean RemoveAliasName(
      const PString & name  ///<  New alias namer to add
    );

    /**Get the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.
     */
    const PStringList & GetAliasNames() const { return localAliasNames; }

    /**Get the alias patterns, might be used in terminalAliasPattern.
     */
    const PStringList & GetAliasNamePatterns() const { return localAliasPatterns; }

    /**Add an alias name pattern to localAliasPatterns. If
       the pattern already exists in the list then is is not added again.
     */
    PBoolean AddAliasNamePattern(
      const PString & pattern  
    );

    /**Get the default ILS server to use for user lookup.
      */
    const PString & GetDefaultILSServer() const { return manager.GetDefaultILSServer(); }

    /**Set the default ILS server to use for user lookup.
      */
    void SetDefaultILSServer(
      const PString & server
    ) { manager.SetDefaultILSServer(server); }

    /**Get the default fast start mode.
      */
    PBoolean IsFastStartDisabled() const
      { return disableFastStart; }

    /**Set the default fast start mode.
      */
    void DisableFastStart(
      PBoolean mode ///<  New default mode
    ) { disableFastStart = mode; } 

    /**Get the default H.245 tunneling mode.
      */
    PBoolean IsH245TunnelingDisabled() const
      { return disableH245Tunneling; }

    /**Set the default H.245 tunneling mode.
      */
    void DisableH245Tunneling(
      PBoolean mode ///<  New default mode
    ) { disableH245Tunneling = mode; } 

    /**Get the default H.245 tunneling mode.
      */
    PBoolean IsH245inSetupDisabled() const
      { return disableH245inSetup; }

    /**Set the default H.245 tunneling mode.
      */
    void DisableH245inSetup(
      PBoolean mode ///<  New default mode
    ) { disableH245inSetup = mode; } 

    /** find out if h245 is disabled or enabled 
      * @return PTrue if h245 is disabled 
      */
    PBoolean IsH245Disabled() const
    { return m_bH245Disabled; }

    /**Disable/Enable H.245, used at least for h450.7 calls
     * @param  bH245Disabled PTrue if h245 has to be disabled 
     */
    void DisableH245(PBoolean bH245Disabled) { m_bH245Disabled = bH245Disabled; } 
    
    /**Get the flag indicating the endpoint can display an amount string.
      */
    PBoolean CanDisplayAmountString() const
      { return canDisplayAmountString; }

    /**Set the flag indicating the endpoint can display an amount string.
      */
    void SetCanDisplayAmountString(
      PBoolean mode ///<  New default mode
    ) { canDisplayAmountString = mode; } 

    /**Get the flag indicating the call will automatically clear after a time.
      */
    PBoolean CanEnforceDurationLimit() const
      { return canEnforceDurationLimit; }

    /**Set the flag indicating the call will automatically clear after a time.
      */
    void SetCanEnforceDurationLimit(
      PBoolean mode ///<  New default mode
    ) { canEnforceDurationLimit = mode; } 

#if OPAL_H450
    /**Get Call Intrusion Protection Level of the end point.
      */
    unsigned GetCallIntrusionProtectionLevel() const { return callIntrusionProtectionLevel; }

    /**Set Call Intrusion Protection Level of the end point.
      */
    void SetCallIntrusionProtectionLevel(
      unsigned level  ///<  New level from 0 to 3
    ) { PAssert(level<=3, PInvalidParameter); callIntrusionProtectionLevel = level; }
#endif

    /**Called from H.450 OnReceivedInitiateReturnError
      */
    virtual void OnReceivedInitiateReturnError();

    /**See if should automatically do call forward of connection.
     */
    PBoolean CanAutoCallForward() const { return autoCallForward; }

    /**Get the current capability table for this endpoint.
     */
    const H323Capabilities & GetCapabilities() const;

    /**Endpoint types.
     */
    enum TerminalTypes {
      e_TerminalOnly = 50,
      e_TerminalAndMC = 70,
      e_GatewayOnly = 60,
      e_GatewayAndMC = 80,
      e_GatewayAndMCWithDataMP = 90,
      e_GatewayAndMCWithAudioMP = 100,
      e_GatewayAndMCWithAVMP = 110,
      e_GatekeeperOnly = 120,
      e_GatekeeperWithDataMP = 130,
      e_GatekeeperWithAudioMP = 140,
      e_GatekeeperWithAVMP = 150,
      e_MCUOnly = 160,
      e_MCUWithDataMP = 170,
      e_MCUWithAudioMP = 180,
      e_MCUWithAVMP = 190
    };

    /**Get the endpoint terminal type.
     */
    TerminalTypes GetTerminalType() const { return terminalType; }

    /**Determine if endpoint is terminal type.
     */
    PBoolean IsTerminal() const;

    /**Determine if endpoint is gateway type.
     */
    PBoolean IsGateway() const;

    /**Determine if endpoint is gatekeeper type.
     */
    PBoolean IsGatekeeper() const;

    /**Determine if endpoint is gatekeeper type.
     */
    PBoolean IsMCU() const;

    /**Get the default maximum audio jitter delay parameter.
       Defaults to 50ms
     */
    unsigned GetMinAudioJitterDelay() const { return manager.GetMinAudioJitterDelay(); }

    /**Get the default maximum audio delay jitter parameter.
       Defaults to 250ms.
     */
    unsigned GetMaxAudioJitterDelay() const { return manager.GetMaxAudioJitterDelay(); }

    /**Set the maximum audio delay jitter parameter.
     */
    void SetAudioJitterDelay(
      unsigned minDelay,   ///<  New minimum jitter buffer delay in milliseconds
      unsigned maxDelay    ///<  New maximum jitter buffer delay in milliseconds
    ) { manager.SetAudioJitterDelay(minDelay, maxDelay); }

    /**Get the initial bandwidth parameter.
     */
    unsigned GetInitialBandwidth() const { return initialBandwidth; }

    /**Get the initial bandwidth parameter.
     */
    void SetInitialBandwidth(unsigned bandwidth) { initialBandwidth = bandwidth; }

#if OPAL_H239
    /**Get the default H.239 control capability.
     */
    bool GetDefaultH239Control() const { return m_defaultH239Control; }

    /**Set the default H.239 control capability.
     */
    void SetDefaultH239Control(
      bool on   ///< H.239 control capability is to be sent to remote
    ) { m_defaultH239Control = on; }
#endif

    /**Called when an outgoing PDU requires a feature set
     */
    virtual PBoolean OnSendFeatureSet(unsigned, H225_FeatureSet &);

    /**Called when an incoming PDU contains a feature set
     */
    virtual void OnReceiveFeatureSet(unsigned, const H225_FeatureSet &);
	
    /**Load the Base FeatureSet usually called when you initialise the endpoint prior to 
       registering with a gatekeeper.
      */
    virtual void LoadBaseFeatureSet();

    /**Callback when creating Feature Instance. This can be used to disable features on
       a case by case basis by returning FALSE
       Default returns TRUE
      */
    virtual bool OnFeatureInstance(
      int instType,
      const PString & identifer
    );

#if OPAL_H460
    /** Is the FeatureSet disabled
      */
    bool FeatureSetDisabled() const { return disableH460; }

    /** Disable all FeatureSets. Use this for pre H323v4 interoperability
      */
    void FeatureSetDisable() { disableH460 = true; }

    /** Get the Endpoint FeatureSet
        This creates a new instance of the featureSet
      */
    H460_FeatureSet * GetFeatureSet() { return features.DeriveNewFeatureSet(); };
#endif

    /**Determine if the address is "local", ie does not need STUN
     */
    virtual PBoolean IsLocalAddress(
      const PIPSocket::Address & remoteAddress
    ) const { return manager.IsLocalAddress(remoteAddress); }

    /**Provide TCP address translation hook
     */
    virtual void TranslateTCPAddress(
      PIPSocket::Address & localAddr,
      const PIPSocket::Address & remoteAddr
    );

    /**Get the TCP port number base for H.245 channels
     */
    WORD GetTCPPortBase() const { return manager.GetTCPPortBase(); }

    /**Get the TCP port number base for H.245 channels.
     */
    WORD GetTCPPortMax() const { return manager.GetTCPPortMax(); }

    /**Set the TCP port number base and max for H.245 channels.
     */
    void SetTCPPorts(unsigned tcpBase, unsigned tcpMax) { manager.SetTCPPorts(tcpBase, tcpMax); }

    /**Get the next TCP port number for H.245 channels
     */
    WORD GetNextTCPPort() { return manager.GetNextTCPPort(); }

    /**Get the UDP port number base for RAS channels
     */
    WORD GetUDPPortBase() const { return manager.GetUDPPortBase(); }

    /**Get the UDP port number base for RAS channels.
     */
    WORD GetUDPPortMax() const { return manager.GetUDPPortMax(); }

    /**Set the TCP port number base and max for H.245 channels.
     */
    void SetUDPPorts(unsigned udpBase, unsigned udpMax) { manager.SetUDPPorts(udpBase, udpMax); }

    /**Get the next UDP port number for RAS channels
     */
    WORD GetNextUDPPort() { return manager.GetNextUDPPort(); }

    /**Get the UDP port number base for RTP channels.
     */
    WORD GetRtpIpPortBase() const { return manager.GetRtpIpPortBase(); }

    /**Get the max UDP port number for RTP channels.
     */
    WORD GetRtpIpPortMax() const { return manager.GetRtpIpPortMax(); }

    /**Set the UDP port number base and max for RTP channels.
     */
    void SetRtpIpPorts(unsigned udpBase, unsigned udpMax) { manager.SetRtpIpPorts(udpBase, udpMax); }

    /**Get the UDP port number pair for RTP channels.
     */
    WORD GetRtpIpPortPair() { return manager.GetRtpIpPortPair(); }

    /**Get the IP Type Of Service byte for RTP channels.
     */
    BYTE GetRtpIpTypeofService() const { return manager.GetRtpIpTypeofService(); }

    /**Set the IP Type Of Service byte for RTP channels.
     */
    void SetRtpIpTypeofService(unsigned tos) { manager.SetRtpIpTypeofService(tos); }

    /**Get the default timeout for calling another endpoint.
     */
    const PTimeInterval & GetSignallingChannelCallTimeout() const { return signallingChannelCallTimeout; }

    /**Get the default timeout for incoming H.245 connection.
     */
    const PTimeInterval & GetControlChannelStartTimeout() const { return controlChannelStartTimeout; }

    /**Get the default timeout for waiting on an end session.
     */
    const PTimeInterval & GetEndSessionTimeout() const { return endSessionTimeout; }

    /**Get the default timeout for master slave negotiations.
     */
    const PTimeInterval & GetMasterSlaveDeterminationTimeout() const { return masterSlaveDeterminationTimeout; }

    /**Get the default retries for H245 master slave negotiations.
     */
    unsigned GetMasterSlaveDeterminationRetries() const { return masterSlaveDeterminationRetries; }

    /**Get the default timeout for H245 capability exchange negotiations.
     */
    const PTimeInterval & GetCapabilityExchangeTimeout() const { return capabilityExchangeTimeout; }

    /**Get the default timeout for H245 logical channel negotiations.
     */
    const PTimeInterval & GetLogicalChannelTimeout() const { return logicalChannelTimeout; }

    /**Get the default timeout for H245 request mode negotiations.
     */
    const PTimeInterval & GetRequestModeTimeout() const { return logicalChannelTimeout; }

    /**Get the default timeout for H245 round trip delay negotiations.
     */
    const PTimeInterval & GetRoundTripDelayTimeout() const { return roundTripDelayTimeout; }

    /**Get the default rate H245 round trip delay is calculated by connection.
     */
    const PTimeInterval & GetRoundTripDelayRate() const { return roundTripDelayRate; }

    /**Get the flag for clearing a call if the round trip delay calculation fails.
     */
    PBoolean ShouldClearCallOnRoundTripFail() const { return clearCallOnRoundTripFail; }

    /**Get the amount of time with no media that should cause call to clear
     */
    const PTimeInterval & GetNoMediaTimeout() const { return manager.GetNoMediaTimeout(); }

    /**Set the amount of time with no media that should cause call to clear
     */
    PBoolean SetNoMediaTimeout(
      const PTimeInterval & newInterval  ///<  New timeout for media
    ) { return manager.SetNoMediaTimeout(newInterval); }

    /**Get the default timeout for GatekeeperRequest and Gatekeeper discovery.
     */
    const PTimeInterval & GetGatekeeperRequestTimeout() const { return gatekeeperRequestTimeout; }

    /**Get the default retries for GatekeeperRequest and Gatekeeper discovery.
     */
    unsigned GetGatekeeperRequestRetries() const { return gatekeeperRequestRetries; }

    /**Get the default timeout for RAS protocol transactions.
     */
    const PTimeInterval & GetRasRequestTimeout() const { return rasRequestTimeout; }

    /**Get the default retries for RAS protocol transations.
     */
    unsigned GetRasRequestRetries() const { return rasRequestRetries; }

    /**Get the default time for gatekeeper to reregister.
       A value of zero disables the keep alive facility.
     */
    const PTimeInterval & GetGatekeeperTimeToLive() const { return registrationTimeToLive; }

    /**Set the default time for gatekeeper to reregister.
       A value of zero disables the keep alive facility.
     */
    void SetGatekeeperTimeToLive(const PTimeInterval & ttl) { registrationTimeToLive = ttl; }

    /**Get the iNow Gatekeeper Access Token OID.
     */
    const PString & GetGkAccessTokenOID() const { return gkAccessTokenOID; }

    /**Set the iNow Gatekeeper Access Token OID.
     */
    void SetGkAccessTokenOID(const PString & token) { gkAccessTokenOID = token; }

    /**Get flag to indicate whether to send GRQ on gatekeeper registration
     */
    PBoolean GetSendGRQ() const
    { return sendGRQ; }

    /**Sent flag to indicate whether to send GRQ on gatekeeper registration
     */
    void SetSendGRQ(PBoolean v) 
    { sendGRQ = v; }

    /**Get the default timeout for Call Transfer Timer CT-T1.
     */
    const PTimeInterval & GetCallTransferT1() const { return callTransferT1; }

    /**Get the default timeout for Call Transfer Timer CT-T2.
     */
    const PTimeInterval & GetCallTransferT2() const { return callTransferT2; }

    /**Get the default timeout for Call Transfer Timer CT-T3.
     */
    const PTimeInterval & GetCallTransferT3() const { return callTransferT3; }

    /**Get the default timeout for Call Transfer Timer CT-T4.
     */
    const PTimeInterval & GetCallTransferT4() const { return callTransferT4; }

    /** Get Call Intrusion timers timeout */
    const PTimeInterval & GetCallIntrusionT1() const { return callIntrusionT1; }
    const PTimeInterval & GetCallIntrusionT2() const { return callIntrusionT2; }
    const PTimeInterval & GetCallIntrusionT3() const { return callIntrusionT3; }
    const PTimeInterval & GetCallIntrusionT4() const { return callIntrusionT4; }
    const PTimeInterval & GetCallIntrusionT5() const { return callIntrusionT5; }
    const PTimeInterval & GetCallIntrusionT6() const { return callIntrusionT6; }

    /**Get the dictionary of <callIdentities, connections>
     */
    H323CallIdentityDict& GetCallIdentityDictionary() { return secondaryConnectionsActive; }

    /**Get the next available invoke Id for H450 operations
      */
#if OPAL_H450
    unsigned GetNextH450CallIdentityValue() const { return ++nextH450CallIdentity; }
#endif

    PString GetDefaultTransport() const;
  //@}

  protected:
    bool InternalCreateGatekeeper(H323Transport * transport);
    H323Connection * InternalMakeCall(
      OpalCall & call,
      const PString & existingToken,           ///<  Existing connection to be transferred
      const PString & callIdentity,            ///<  Call identity of the secondary call (if it exists)
      unsigned capabilityLevel,                ///<  Intrusion capability level
      const PString & remoteParty,             ///<  Remote party to call
      void * userData,                         ///<  user data to pass to CreateConnection
      unsigned int options = 0,                ///<  options to pass to connection
      OpalConnection::StringOptions * stringOptions = NULL ///<  complex string options
    );

    // Configuration variables, commonly changed
    PStringList     localAliasNames;
    PStringList     localAliasPatterns;
    PBoolean        autoCallForward;
    PBoolean        disableFastStart;
    PBoolean        disableH245Tunneling;
    PBoolean        disableH245inSetup;
    PBoolean        m_bH245Disabled; /* enabled or disabled h245 */
    PBoolean        canDisplayAmountString;
    PBoolean        canEnforceDurationLimit;
#if OPAL_H450
    unsigned        callIntrusionProtectionLevel;
#endif

    TerminalTypes   terminalType;

#if OPAL_H239
    bool            m_defaultH239Control;
#endif

    PBoolean        clearCallOnRoundTripFail;

    // Some more configuration variables, rarely changed.
    PTimeInterval signallingChannelCallTimeout;
    PTimeInterval controlChannelStartTimeout;
    PTimeInterval endSessionTimeout;
    PTimeInterval masterSlaveDeterminationTimeout;
    unsigned      masterSlaveDeterminationRetries;
    PTimeInterval capabilityExchangeTimeout;
    PTimeInterval logicalChannelTimeout;
    PTimeInterval requestModeTimeout;
    PTimeInterval roundTripDelayTimeout;
    PTimeInterval roundTripDelayRate;
    PTimeInterval gatekeeperRequestTimeout;
    unsigned      gatekeeperRequestRetries;
    PTimeInterval rasRequestTimeout;
    unsigned      rasRequestRetries;
    PTimeInterval registrationTimeToLive;

    PString       gkAccessTokenOID;
    PBoolean          sendGRQ;

    /* Protect against absence of a response to the ctIdentify reqest
       (Transferring Endpoint - Call Transfer with a secondary Call) */
    PTimeInterval callTransferT1;
    /* Protect against failure of completion of the call transfer operation
       involving a secondary Call (Transferred-to Endpoint) */
    PTimeInterval callTransferT2;
    /* Protect against failure of the Transferred Endpoint not responding
       within sufficient time to the ctInitiate APDU (Transferring Endpoint) */
    PTimeInterval callTransferT3;
    /* May optionally operate - protects against absence of a response to the
       ctSetup request (Transferred Endpoint) */
    PTimeInterval callTransferT4;

    /** Call Intrusion Timers */
    PTimeInterval callIntrusionT1;
    PTimeInterval callIntrusionT2;
    PTimeInterval callIntrusionT3;
    PTimeInterval callIntrusionT4;
    PTimeInterval callIntrusionT5;
    PTimeInterval callIntrusionT6;

    // Dynamic variables
    mutable H323Capabilities capabilities;
    H323Gatekeeper *     gatekeeper;
    PString              gatekeeperUsername;
    PString              gatekeeperPassword;
    H323CallIdentityDict secondaryConnectionsActive;

#if OPAL_H450
    mutable PAtomicInteger nextH450CallIdentity;
            /// Next available callIdentity for H450 Transfer operations via consultation.
#endif

#if OPAL_H460
    bool            disableH460;
    H460_FeatureSet features;
#endif

};

#endif // OPAL_H323

#endif // OPAL_H323_H323EP_H


/////////////////////////////////////////////////////////////////////////////
