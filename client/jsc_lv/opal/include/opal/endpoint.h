/*
 * endpoint.h
 *
 * Telephony endpoint abstraction
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23926 $
 * $Author: rjongbloed $
 * $Date: 2010-01-13 00:44:33 +0000 (Wed, 13 Jan 2010) $
 */

#ifndef OPAL_OPAL_ENDPOINT_H
#define OPAL_OPAL_ENDPOINT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/manager.h>
#include <opal/mediafmt.h>
#include <opal/transports.h>

class OpalCall;
class OpalMediaStream;

/**This class describes an endpoint base class.
   Each protocol (or psuedo-protocol) would create a descendant off this
   class to manage its particular subsystem. Typically this would involve
   listening for incoming connections and being able to set up outgoing
   connections. Depending on exact semantics it may need to spawn many threads
   to achieve this.

   An endpoint will also have a default set of media data formats that it can
   support. Connections created by it would initially have the same set, but
   according to the semantics of the underlying protocol may end up using a
   different set.

   Various call backs are provided for points in the connection management. As
   a rule these are passed straight on to the OpalManager for processing. An
   application may create descendants off this class' subclasses, eg
   H323EndPoint, to modify or monitor its behaviour but it does not have to do
   so as all basic operations are passed to the OpalManager so only that class
   need be subclassed.
 */
class OpalEndPoint : public PObject
{
    PCLASSINFO(OpalEndPoint, PObject);
  public:
    enum Attributes {
      CanTerminateCall = 1,
      HasLineInterface = 2
    };

  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalEndPoint(
      OpalManager & manager,          ///<  Manager of all endpoints.
      const PCaselessString & prefix, ///<  Prefix for URL style address strings
      unsigned attributes             ///<  Bit mask of attributes endpoint has
    );

    /**Destroy the endpoint.
     */
    ~OpalEndPoint();

    /**Shut down the endpoint, this is called by the OpalManager just before
       destroying the object and can be handy to make sure some things are
       stopped before the vtable gets clobbered.
      */
    virtual void ShutDown();
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Standard stream print function.
       The PObject class has a << operator defined that calls this function
       polymorphically.
      */
    void PrintOn(
      ostream & strm    ///<  Stream to output text representation
    ) const;
  //@}

  /**@name Listeners management */
  //@{
    /**Add a set of listeners to the endoint.
       This allows for the automatic creating of incoming call connections. 
       If the list is empty then GetDefaultListeners() is used.
      */
    PBoolean StartListeners(
      const PStringArray & interfaces ///<  Address of interface to listen on.
    );

    /**Add a listener to the endoint.
       This allows for the automatic creating of incoming call connections. /
       If the address is empty then the first entry of GetDefaultListeners() is used.
      */
    PBoolean StartListener(
      const OpalTransportAddress & iface ///<  Address of interface to listen on.
    );

    /**Add a listener to the endoint.
       This allows for the automatic creating of incoming call connections. An
       application should use OnConnectionEstablished() to monitor when calls
       have arrived and been successfully negotiated.
      */
    PBoolean StartListener(
      OpalListener * listener ///<  Transport dependent listener.
    );

    /**Get the default listeners for the endpoint type.
       Default behaviour returns empty list if defaultSignalPort is zero, else
       one entry using tcp and INADDR_ANY, eg tcp$*:1720
      */
    virtual PStringArray GetDefaultListeners() const;

    /**Find a listener given the transport address.
      */
    OpalListener * FindListener(
        const OpalTransportAddress & iface ///<  Address of interface we may be listening on.
    );

    /**Stop a listener given the transport address.
       Returns PTrue if a listener was on that interface, and was stopped.
      */
    PBoolean StopListener(
        const OpalTransportAddress & iface ///<  Address of interface we may be listening on.
    );

    /**Remove a listener from the endoint.
       If the listener parameter is NULL then all listeners are removed.
      */
    PBoolean RemoveListener(
      OpalListener * listener ///<  Transport dependent listener.
    );

    /**Return a list of the transport addresses for all listeners on this endpoint
      */
    OpalTransportAddressArray GetInterfaceAddresses(
      PBoolean excludeLocalHost = PTrue,       ///<  Flag to exclude 127.0.0.1
      const OpalTransport * associatedTransport = NULL
                          ///<  Associated transport for precedence and translation
    );

    /**Handle new incoming connection.
       This will either create a new connection object or utilise a previously
       created connection on the same transport address and reference number.
      */
    PDECLARE_NOTIFIER(PThread, OpalEndPoint, ListenerCallback);

    /**Handle new incoming connection from listener.

       A return value of PTrue indicates that the transport object should be
       deleted by the caller. PFalse indicates that something else (eg the
       connection) has taken over responsibility for deleting the transport.

       The default behaviour just returns PTrue.
      */
    virtual PBoolean NewIncomingConnection(
      OpalTransport * transport  ///<  Transport connection came in on
    );
  //@}

  /**@name Connection management */
  //@{
    /**Set up a connection to a remote party.
       This is called from the OpalManager::MakeConnection() function once
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
      OpalCall & call,          ///<  Owner of connection
      const PString & party,    ///<  Remote party to call
      void * userData = NULL,          ///<  Arbitrary data to pass to connection
      unsigned int options = 0,     ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions = NULL
    ) = 0;

    /**Callback for outgoing connection, it is invoked after OpalLineConnection::SetUpConnection
       This function allows the application to set up some parameters or to log some messages
       */
    virtual PBoolean OnSetUpConnection(OpalConnection &connection);
    
    /**Call back for answering an incoming call.
       This function is used for an application to control the answering of
       incoming calls.

       If PTrue is returned then the connection continues. If PFalse then the
       connection is aborted.

       Note this function should not block for any length of time. If the
       decision to answer the call may take some time eg waiting for a user to
       pick up the phone, then AnswerCallPending or AnswerCallDeferred should
       be returned.

       If an application overrides this function, it should generally call the
       ancestor version to complete calls. Unless the application completely
       takes over that responsibility. Generally, an application would only
       intercept this function if it wishes to do some form of logging. For
       this you can obtain the name of the caller by using the function
       OpalConnection::GetRemotePartyName().

       The default behaviour calls the OpalManager function of the same name.
     */
    virtual PBoolean OnIncomingConnection(
      OpalConnection & connection,  ///<  Connection that is calling
      unsigned options,             ///<  options for new connection (can't use default value as overrides will fail)
      OpalConnection::StringOptions * stringOptions
    );

    /**Call back for remote party is now responsible for completing the call.
       This function is called when the remote system has been contacted and it
       has accepted responsibility for completing, or failing, the call. This
       is distinct from OnAlerting() in that it is not known at this time if
       anything is ringing. This indication may be used to distinguish between
       "transport" level error, in which case another host may be tried, and
       that finalising the call has moved "upstream" and the local system has
       no more to do but await a result.

       If an application overrides this function, it should generally call the
       ancestor version for correct operation.

       The default behaviour calls the OpalManager function of the same name.
     */
    virtual void OnProceeding(
      OpalConnection & connection   ///<  Connection that is proceeeding
    );

    /**Call back for remote party being alerted.
       This function is called after the connection is informed that the
       remote endpoint is "ringing". Generally some time after the
       MakeConnection() function was called, this is function is called.

       If PFalse is returned the connection is aborted.

       If an application overrides this function, it should generally call the
       ancestor version for correct operation. An application would typically
       only intercept this function if it wishes to do some form of logging.
       For this you can obtain the name of the caller by using the function
       OpalConnection::GetRemotePartyName().

       The default behaviour calls the OpalManager function of the same name.
     */
    virtual void OnAlerting(
      OpalConnection & connection   ///<  Connection that is alerting
    );

    /**Call back for answering an incoming call.
       This function is called after the connection has been acknowledged
       but before the connection is established

       This gives the application time to wait for some event before
       signalling to the endpoint that the connection is to proceed. For
       example the user pressing an "Answer call" button.

       If AnswerCallDenied is returned the connection is aborted and the
       connetion specific end call PDU is sent. If AnswerCallNow is returned 
       then the connection proceeding, Finally if AnswerCallPending is returned then the
       protocol negotiations are paused until the AnsweringCall() function is
       called.

       The default behaviour simply returns AnswerNow.
     */
    virtual OpalConnection::AnswerCallResponse OnAnswerCall(
      OpalConnection & connection,    ///<  connection that is being answered
       const PString & caller         ///<  caller
    );

    /**A call back function whenever a connection is "connected".
       This indicates that a connection to an endpoint was connected. That
       is the endpoint received acknowledgement via whatever protocol it uses
       that the connection may now start media streams.

       In the context of H.323 this means that the CONNECT pdu has been
       received.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void OnConnected(
      OpalConnection & connection   ///<  Connection that was established
    );

    /**A call back function whenever a connection is established.
       This indicates that a connection to an endpoint was established. This
       usually occurs after OnConnected() and indicates that the connection
       is both connected and has media flowing.

       In the context of H.323 this means that the signalling and control
       channels are open and the TerminalCapabilitySet and MasterSlave
       negotiations are complete.

       The default behaviour does nothing.
      */
    virtual void OnEstablished(
      OpalConnection & connection   ///<  Connection that was established
    );

    /**A call back function whenever a connection is broken.
       This function can do any internal cleaning up and waiting on background
       threads that may be using the connection object.

       Note that there is not a one to one relationship with the
       OnEstablishedConnection() function. This function may be called without
       that function being called. For example if MakeConnection() was used
       but the call never completed.

       Classes that override this function should make sure they call the
       ancestor version for correct operation.

       An application will not typically call this function as it is used by
       the OpalManager during a release of the connection.

       The default behaviour removes the connection from the internal database
       and calls the OpalManager function of the same name.
      */
    virtual void OnReleased(
      OpalConnection & connection   ///<  Connection that was established
    );

    /**A call back function whenever a connection is "held" or "retrieved".
       This indicates that a connection to an endpoint was held, or
       retrieved, either locally or by the remote endpoint.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void OnHold(
      OpalConnection & connection,   ///<  Connection that was held/retrieved
      bool fromRemote,               ///<  Indicates remote has held local connection
      bool onHold                    ///<  Indicates have just been held/retrieved.
    );
    virtual void OnHold(OpalConnection & connection); // For backward compatibility

    /**A call back function whenever a connection is forwarded.

       The default behaviour does nothing.
      */
    virtual PBoolean OnForwarded(
      OpalConnection & connection,  ///<  Connection that was held
      const PString & remoteParty         ///<  The new remote party
    );

    /**Clear a call.
       This finds the call by using the token then calls the OpalCall::Clear()
       function on it. All connections are released, and the conenctions and
       call disposed of. Note that this function returns quickly and the
       disposal happens at some later time by a background thread. This it is
       safe to call this function from anywhere.
      */
    virtual PBoolean ClearCall(
      const PString & token,    ///<  Token for identifying connection
      OpalConnection::CallEndReason reason = OpalConnection::EndedByLocalUser, ///<  Reason for call clearing
      PSyncPoint * sync = NULL  ///<  Sync point to wait on.
    );

    /**Clear a current connection.
       This hangs up the connection to a remote endpoint. Note that these functions
       are synchronous
      */
    virtual PBoolean ClearCallSynchronous(
      const PString & token,    ///<  Token for identifying connection
      OpalConnection::CallEndReason reason = OpalConnection::EndedByLocalUser, ///<  Reason for call clearing
      PSyncPoint * sync = NULL  ///<  Sync point to wait on.
    );

    /**Clear all current connections.
       This hangs up all the connections to remote endpoints. The wait
       parameter is used to wait for all the calls to be cleared and their
       memory usage cleaned up before returning. This is typically used in
       the destructor for your descendant of H323EndPoint.
      */
    virtual void ClearAllCalls(
      OpalConnection::CallEndReason reason = OpalConnection::EndedByLocalUser, ///<  Reason for call clearing
      PBoolean wait = PTrue   ///<  Flag for wait for calls to e cleared.
    );

    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeConnection().
      */
    PSafePtr<OpalConnection> GetConnectionWithLock(
      const PString & token,     ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite
    ) { return connectionsActive.FindWithLock(token, mode); }

    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeConnection(). If not then it
       attempts to use the token as a OpalCall token and find a connection
       of the same class.
      */
    template <class ConnClass>
    PSafePtr<ConnClass> GetConnectionWithLockAs(
      const PString & token,     ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite
    )
    {
      PSafePtr<ConnClass> connection = PSafePtrCast<OpalConnection, ConnClass>(GetConnectionWithLock(token, mode));
      if (connection == NULL) {
        PSafePtr<OpalCall> call = manager.FindCallWithLock(token, PSafeReadOnly);
        if (call != NULL) {
          connection = PSafePtrCast<OpalConnection, ConnClass>(call->GetConnection(0, mode));
          if (connection == NULL)
            connection = PSafePtrCast<OpalConnection, ConnClass>(call->GetConnection(1, mode));
        }
      }
      return connection;
    }

    /**Get all calls current on the endpoint.
      */
    PStringList GetAllConnections();
    
    /** Get calls count on the endpoint
      */
    PINDEX GetConnectionCount() const { return connectionsActive.GetSize(); }

    /**Determine if a connection is active.
      */
    virtual PBoolean HasConnection(
      const PString & token   ///<  Token for identifying connection
    );

    /**Destroy the connection.
      */
    virtual void DestroyConnection(
      OpalConnection * connection  ///<  Connection to destroy
    );
  //@}

  /**@name Media Streams management */
  //@{
    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour is pure.
      */
    virtual OpalMediaFormatList GetMediaFormats() const = 0;

    /**Adjust media formats available on a connection.
       This is called by a connection after it has called
       OpalCall::GetMediaFormats() to get all media formats that it can use so
       that an application may remove or reorder the media formats before they
       are used to open media streams.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void AdjustMediaFormats(
      bool local,                         ///<  Media formats a local ones to be presented to remote
      const OpalConnection & connection,  ///<  Connection that is about to use formats
      OpalMediaFormatList & mediaFormats  ///<  Media formats to use
    ) const;

    /**Call back when opening a media stream.
       This function is called when a connection has created a new media
       stream according to the logic of its underlying protocol.

       The usual requirement is that media streams are created on all other
       connections participating in the call and all of the media streams are
       attached to an instance of an OpalMediaPatch object that will read from
       one of the media streams passing data to the other media streams.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual PBoolean OnOpenMediaStream(
      OpalConnection & connection,  ///<  Connection that owns the media stream
      OpalMediaStream & stream      ///<  New media stream being opened
    );

    /**Call back for closed a media stream.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void OnClosedMediaStream(
      const OpalMediaStream & stream     ///<  Media stream being closed
    );

#if OPAL_VIDEO
    /**Create an PVideoInputDevice for a source media stream.
      */
    virtual PBoolean CreateVideoInputDevice(
      const OpalConnection & connection,    ///<  Connection needing created video device
      const OpalMediaFormat & mediaFormat,  ///<  Media format for stream
      PVideoInputDevice * & device,         ///<  Created device
      PBoolean & autoDelete                     ///<  Flag for auto delete device
    );

    /**Create an PVideoOutputDevice for a sink media stream or the preview
       display for a source media stream.
      */
    virtual PBoolean CreateVideoOutputDevice(
      const OpalConnection & connection,    ///<  Connection needing created video device
      const OpalMediaFormat & mediaFormat,  ///<  Media format for stream
      PBoolean preview,                         ///<  Flag indicating is a preview output
      PVideoOutputDevice * & device,        ///<  Created device
      PBoolean & autoDelete                     ///<  Flag for auto delete device
    );
#endif
  //@}

  /**@name User indications */
  //@{
    /**Call back for remote enpoint has sent user input as a string.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void OnUserInputString(
      OpalConnection & connection,  ///<  Connection input has come from
      const PString & value   ///<  String value of indication
    );

    /**Call back for remote enpoint has sent user input.
       If duration is zero then this indicates the beginning of the tone. If
       duration is non-zero then it indicates the end of the tone output.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void OnUserInputTone(
      OpalConnection & connection,  ///<  Connection input has come from
      char tone,                    ///<  Tone received
      int duration                  ///<  Duration of tone
    );

    /**Read a sequence of user indications from connection with timeouts.
      */
    virtual PString ReadUserInput(
      OpalConnection & connection,        ///<  Connection to read input from
      const char * terminators = "#\r\n", ///<  Characters that can terminte input
      unsigned lastDigitTimeout = 4,      ///<  Timeout on last digit in string
      unsigned firstDigitTimeout = 30     ///<  Timeout on receiving any digits
    );
  //@}

  /**@name Other services */
  //@{
    /**Callback called when Message Waiting Indication (MWI) is received
     */
    virtual void OnMWIReceived (
      const PString & party,                ///< Name of party MWI is for
      OpalManager::MessageWaitingType type, ///< Type of message that is waiting
      const PString & extraInfo             ///< Addition information on the MWI
    );

    /** Execute garbage collection for endpoint.
        Returns PTrue if all garbage has been collected.
        Default behaviour deletes the objects in the connectionsActive list.
      */
    virtual PBoolean GarbageCollection();
  //@}

  /**@name Member variable access */
  //@{
    /**Get the manager for this endpoint.
     */
    OpalManager & GetManager() const { return manager; }

    /**Get the protocol prefix name for the endpoint.
      */
    const PString & GetPrefixName() const { return prefixName; }

    /**Get an indication of if this endpoint has particular option.
      */
    PBoolean HasAttribute(Attributes opt) const { return (attributeBits&opt) != 0; }

    /**Get the default signal port for this endpoint.
     */
    WORD GetDefaultSignalPort() const { return defaultSignalPort; }

    /**Get the product info for all endpoints.
      */
    const OpalProductInfo & GetProductInfo() const { return productInfo; }

    /**Set the product info for all endpoints.
      */
    void SetProductInfo(
      const OpalProductInfo & info
    ) { productInfo = info; }

    /**Get the default local party name for all connections on this endpoint.
      */
    const PString & GetDefaultLocalPartyName() const { return defaultLocalPartyName; }

    /**Set the default local party name for all connections on this endpoint.
      */
    virtual void SetDefaultLocalPartyName(
      const PString & name  /// Name for local party
    ) { defaultLocalPartyName = name; }

    /**Get the default local display name for all connections on this endpoint.
      */
    const PString & GetDefaultDisplayName() const { return defaultDisplayName; }

    /**Set the default local display name for all connections on this endpoint.
      */
    void SetDefaultDisplayName(const PString & name) { defaultDisplayName = name; }

    /**Get the initial bandwidth parameter.
     */
    unsigned GetInitialBandwidth() const { return initialBandwidth; }

    /**Get the initial bandwidth parameter.
     */
    void SetInitialBandwidth(unsigned bandwidth) { initialBandwidth = bandwidth; }

    /**Get the set of listeners (incoming call transports) for this endpoint.
     */
    const OpalListenerList & GetListeners() const { return listeners; }

    /**Get the default options for created connections.
      */
    const OpalConnection::StringOptions & GetDefaultStringOptions() const { return m_defaultStringOptions; }

    /**Set the default options for created connections.
      */
    void SetDefaultStringOptions(const OpalConnection::StringOptions & opts) { m_defaultStringOptions = opts; }

    /**Set the default option for created connections.
      */
    void SetDefaultStringOption(const PCaselessString & key, const PString & data) { m_defaultStringOptions.SetAt(key, data); }

    /**Get the default mode for sending User Input Indications.
      */
    OpalConnection::SendUserInputModes GetSendUserInputMode() const { return defaultSendUserInputMode; }

    /**Set the default mode for sending User Input Indications.
      */
    void SetSendUserInputMode(OpalConnection::SendUserInputModes mode) { defaultSendUserInputMode = mode; }
  //@}

    virtual PString GetDefaultTransport() const;

    /**Call back for a new connection has been constructed.
       This is called after CreateConnection has returned a new connection.
       It allows an application to make any custom adjustments to the
       connection before it begins to process the protocol. behind it.
      */
    virtual void OnNewConnection(
      OpalCall & call,              ///< Call that owns the newly created connection.
      OpalConnection & connection   ///< New connection just created
    );

#if OPAL_PTLIB_SSL
    PString GetSSLCertificate() const;
#endif

    /** Find a listener that is compatible with the specified protocol
     */
    bool FindListenerForProtocol(const char * protoPrefix, OpalTransportAddress & addr);

    /**Send text message
     */
    virtual PBoolean Message(
      const PString & to, 
      const PString & body
    );
    virtual PBoolean Message(
      const PURL & to, 
      const PString & type,
      const PString & body,
      PURL & from, 
      PString & conversationId
    );

    /**Called when text message received
     */
    virtual void OnMessageReceived(
      const PURL & from, 
      const PString & fromName,
      const PURL & to, 
      const PString & type,
      const PString & body,
      const PString & conversationId
    );

#if OPAL_HAS_IM
    /** Called when text message to be sent to remote end of a connection
      */
    virtual bool TransmitExternalIM(
      OpalConnection & conn, 
      const OpalMediaFormat & format, 
      RTP_IMFrame & frame
    );
#endif
    OpalConnection * AddConnection(OpalConnection * connection);

  protected:
    OpalManager   & manager;
    PCaselessString prefixName;
    unsigned        attributeBits;
    WORD            defaultSignalPort;
    OpalProductInfo productInfo;
    PString         defaultLocalPartyName;
    PString         defaultDisplayName;

    unsigned initialBandwidth;  // in 100s of bits/sev
    OpalConnection::StringOptions      m_defaultStringOptions;
    OpalConnection::SendUserInputModes defaultSendUserInputMode;

    OpalListenerList   listeners;

    class ConnectionDict : public PSafeDictionary<PString, OpalConnection>
    {
        virtual void DeleteObject(PObject * object) const;
    } connectionsActive;

    PMutex inUseFlag;

    friend void OpalManager::GarbageCollection();
    friend void OpalConnection::Release(CallEndReason reason);

  private:
    P_REMOVE_VIRTUAL(PBoolean, OnIncomingConnection(OpalConnection &, unsigned), false);
    P_REMOVE_VIRTUAL(PBoolean, OnIncomingConnection(OpalConnection &), false);
    P_REMOVE_VIRTUAL_VOID(AdjustMediaFormats(const OpalConnection &, OpalMediaFormatList &) const);
};


#endif // OPAL_OPAL_ENDPOINT_H


// End of File ///////////////////////////////////////////////////////////////
