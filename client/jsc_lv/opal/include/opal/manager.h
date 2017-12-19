/*
 * manager.h
 *
 * OPAL system manager.
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
 * $Revision: 23988 $
 * $Author: rjongbloed $
 * $Date: 2010-01-29 01:28:07 +0000 (Fri, 29 Jan 2010) $
 */

#ifndef OPAL_OPAL_MANAGER_H
#define OPAL_OPAL_MANAGER_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/pres_ent.h>
#include <opal/call.h>
#include <opal/connection.h> //OpalConnection::AnswerCallResponse
#include <opal/guid.h>
#include <codec/silencedetect.h>
#include <codec/echocancel.h>
#include <ptclib/pstun.h>
#include <ptclib/url.h>

#if OPAL_VIDEO
#include <ptlib/videoio.h>
#endif

class OpalEndPoint;
class OpalMediaPatch;


/**This class is the central manager for OPAL.
   The OpalManager embodies the root of the tree of objects that constitute an
   OPAL system. It contains all of the endpoints that make up the system. Other
   entities such as media streams etc are in turn contained in these objects.
   It is expected that an application would only ever have one instance of
   this class, and also descend from it to override call back functions.

   The manager is the eventual destination for call back indications from
   various other objects. It is possible, for instance, to get an indication
   of a completed call by creating a descendant of the OpalCall and overriding
   the OnClearedCall() virtual. However, this could quite unwieldy for all of
   the possible classes, so the default behaviour is to call the equivalent
   function on the OpalManager. This allows most applications to only have to
   create a descendant of the OpalManager and override virtual functions there
   to get all the indications it needs.
 */
struct stCallUserData
{
  int calltype;
  int callrepairsockettype;
  int ctccalltype;
  PString strOwnerID;
};
class OpalManager : public PObject
{
    PCLASSINFO(OpalManager, PObject);
  public:
  virtual bool IsinClosedVideoUser(unsigned int uid){return false;};
	virtual  void UnloadLocalVideo(){};
  /**@name Construction */
  //@{
    /**Create a new manager.
     */
    OpalManager();

    /**Destroy the manager.
       This will clear all calls, then delete all endpoints still attached to
       the manager.
     */
    ~OpalManager();
  //@}

  /**@name Endpoint management */
  //@{
    /**Attach a new endpoint to the manager.
       This is an internal function called by the OpalEndPoint constructor.

       Note that usually the endpoint is automatically "owned" by the manager.
       They should not be deleted directly. The DetachEndPoint() command
       should be used to do this.
      */
    void AttachEndPoint(
      OpalEndPoint * endpoint,    ///< EndPoint to add to the manager
      const PString & prefix = PString::Empty()  ///< Prefix to use, if empty uses endpoint->GetPrefixName()
    );

    /**Remove an endpoint from the manager.
       This will delete the endpoint object.
      */
    void DetachEndPoint(
      const PString & prefix
    );
    void DetachEndPoint(
      OpalEndPoint * endpoint
    );

    /**Find an endpoint instance that is using the specified prefix.
      */
    OpalEndPoint * FindEndPoint(
      const PString & prefix
    );

    /**Get the endpoints attached to this manager.
      */
    PList<OpalEndPoint> GetEndPoints() const;

    /**Shut down all of the endpoints, clearing all calls.
       This is synchonous and will wait till everything is shut down.
       This will also assure no new calls come in whilein the process of
       shutting down.
      */
    void ShutDownEndpoints();
  //@}

  /**@name Call management */
  //@{
    /**Set up a call between two parties.
       This is used to initiate a call. Incoming calls are "answered" using a
       different mechanism.

       The A party and B party strings indicate the protocol and address of
       the party to call in the style of a URL. The A party is the initiator
       of the call and the B party is the remote system being called. See the
       MakeConnection() function for more details on the format of these
       strings.

       The token returned is a unique identifier for the call that allows an
       application to gain access to the call at later time. This is necesary
       as any pointer being returned could become invalid (due to being
       deleted) at any time due to the multithreaded nature of the OPAL
       system. 
     */
    virtual PSafePtr<OpalCall> SetUpCall(
      const PString & partyA,       ///<  The A party of call
      const PString & partyB,       ///<  The B party of call
      void * userData = NULL,       ///<  user data passed to Call and Connection
      unsigned options = 0,         ///<  options passed to connection
      OpalConnection::StringOptions * stringOptions = NULL   ///<  complex string options passed to call
    );
    virtual PBoolean SetUpCall(
      const PString & partyA,       ///<  The A party of call
      const PString & partyB,       ///<  The B party of call
      PString & token,              ///<  Token for call
      void * userData = NULL,       ///<  user data passed to Call and Connection
      unsigned options = 0,         ///<  options passed to connection
      OpalConnection::StringOptions * stringOptions = NULL   ///<  complex string options passed to call
    );

    /**A call back function whenever a call is completed.
       In telephony terminology a completed call is one where there is an
       established link between two parties.

       This called from the OpalCall::OnEstablished() function.

       The default behaviour does nothing.
      */
    virtual void OnEstablishedCall(
      OpalCall & call   ///<  Call that was completed
    );

    /**Determine if a call is active.
       Return PTrue if there is an active call with the specified token. Note
       that the call could clear any time (even milliseconds) after this
       function returns PTrue.
      */
    virtual PBoolean HasCall(
      const PString & token  ///<  Token for identifying call
    ) { return activeCalls.FindWithLock(token, PSafeReference) != NULL; }

    /**Return the number of active calls.
      */
    PINDEX GetCallCount() const { return activeCalls.GetSize(); }


    /**Determine if a call is established.
       Return PTrue if there is an active call with the specified token and
       that call has at least two parties with media flowing between them.
       Note that the call could clear any time (even milliseconds) after this
       function returns PTrue.
      */
    virtual PBoolean IsCallEstablished(
      const PString & token  ///<  Token for identifying call
    );

    /**Find a call with the specified token.
       This searches the manager database for the call that contains the token
       as provided by functions such as SetUpCall().

       Note the caller of this function MUST call the OpalCall::Unlock()
       function if this function returns a non-NULL pointer. If it does not
       then a deadlock can occur.
      */
    PSafePtr<OpalCall> FindCallWithLock(
      const PString & token,  ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite
    ) { return activeCalls.FindWithLock(token, mode); }

    /**Clear a call.
       This finds the call by using the token then calls the OpalCall::Clear()
       function on it. All connections are released, and the connections and
       call are disposed of. Note that this function returns quickly and the
       disposal happens at some later time in a background thread. It is safe
       to call this function from anywhere.
      */
    virtual PBoolean ClearCall(
      const PString & token,    ///<  Token for identifying connection
      OpalConnection::CallEndReason reason = OpalConnection::EndedByLocalUser, ///<  Reason for call clearing
      PSyncPoint * sync = NULL  ///<  Sync point to wait on.
    );

    /**Clear a call.
       This finds the call by using the token then calls the OpalCall::Clear()
       function on it. All connections are released, and the connections and
       caller disposed of. Note that this function waits until the call has
       been cleared and all responses timeouts etc completed. Care must be
       used as to when it is called as deadlocks may result.
      */
    virtual PBoolean ClearCallSynchronous(
      const PString & token,    ///<  Token for identifying connection
      OpalConnection::CallEndReason reason = OpalConnection::EndedByLocalUser ///<  Reason for call clearing
    );

    /**Clear all current calls.
       This effectively executes OpalCall::Clear() on every call that the
       manager has active.
       This function can not be called from several threads at the same time.
      */
    virtual void ClearAllCalls(
      OpalConnection::CallEndReason reason = OpalConnection::EndedByLocalUser, ///<  Reason for call clearing
      PBoolean wait = PTrue   ///<  Flag to wait for calls to e cleared.
    );

    /**A call back function whenever a call is cleared.
       A call is cleared whenever there is no longer any connections attached
       to it. This function is called just before the call is deleted.
       However, it may be used to display information on the call after
       completion, eg the call parties and duration.

       Note that there is not a one to one relationship with the
       OnEstablishedCall() function. This function may be called without that
       function being called. For example if MakeConnection() was used but
       the call never completed.

       The default behaviour removes the call from the activeCalls dictionary.
      */
    virtual void OnClearedCall(
      OpalCall & call   ///<  Connection that was established
    );

    /**Create a call object.
       This function allows an application to have the system create
       desccendants of the OpalCall class instead of instances of that class
       directly. The application can thus override call backs or add extra
       information that it wishes to maintain on a call by call basis.

       The default behavious returns an instance of OpalCall.
      */
    virtual OpalCall * CreateCall(
      void * userData            ///<  user data passed to SetUpCall
    );
    OpalCall * InternalCreateCall();

    /**Destroy a call object.
       This gets called from background thread that garbage collects all calls
       and connections. If an application has object lifetime issues with the
       threading, it can override this function and take responsibility for
       deleting the object at some later time.

       The default behaviour simply calls "delete call".
      */
    virtual void DestroyCall(
      OpalCall * call
    );

    /**Get next unique token ID for calls or connections.
       This is an internal function called by the OpalCall and other
       constructors.
      */
    virtual PString GetNextToken(char prefix);
  //@}

  /**@name Presence & Instant Messaging management */
  //@{
    /**Add a presentity.
       Returns a Read/Write locked pointer to presentity.
      */
    virtual PSafePtr<OpalPresentity> AddPresentity(
      const PString & presentity  ///< Presentity URI
    );

    /**Get a presentity.
      */
    virtual PSafePtr<OpalPresentity> GetPresentity(
      const PString & presentity,         ///< Presentity URI
      PSafetyMode mode = PSafeReference   ///< Safety mode for presentity
    );

    /**Remove a presentity.
      */
    virtual bool RemovePresentity(
      const PString & presentity  ///< Presentity URI
    );

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
  //@}

  /**@name Connection management */
  //@{
    /**Set up a connection to a remote party.
       An appropriate protocol (endpoint) is determined from the party
       parameter. That endpoint is then called to create a connection and that
       connection is attached to the call provided.
       
       If the endpoint is already occupied in a call then the endpoints list
       is further searched for additional endpoints that support the protocol.
       For example multiple pstn endpoints may be present for multiple LID's.
       
       The general form for this party parameter is:

            [proto:][alias@][transport$]address[:port]

       where the various fields will have meanings specific to the endpoint
       type. For example, with H.323 it could be "h323:Fred@site.com" which
       indicates a user Fred at gatekeeper size.com. Whereas for the PSTN
       endpoint it could be "pstn:5551234" which is to call 5551234 on the
       first available PSTN line.

       The default for the proto is the name of the protocol for the first
       endpoint attached to the manager. Other fields default to values on an
       endpoint basis.

       This function usually returns almost immediately with the connection
       continuing to occur in a new background thread.

       If PFalse is returned then the connection could not be established. For
       example if a PSTN endpoint is used and the associated line is engaged
       then it may return immediately. Returning a non-NULL value does not
       mean that the connection will succeed, only that an attempt is being
       made.

       The default behaviour is pure.
     */
    virtual PSafePtr<OpalConnection> MakeConnection(
      OpalCall & call,                   ///<  Owner of connection
      const PString & party,             ///<  Party to call
      void * userData = NULL,            ///<  user data to pass to connections
      unsigned int options = 0,          ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions = NULL
    );

    /**Call back for a new connection has been constructed.
       This is called after CreateConnection has returned a new connection.
       It allows an application to make any custom adjustments to the
       connection before it begins to process the protocol. behind it.
      */
    virtual void OnNewConnection(
      OpalConnection & connection   ///< New connection just created
    );

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

       The default behaviour is to call OnRouteConnection to determine a
       B party for the connection.

       If the call associated with the incoming call already had two parties
       and this connection is a third party for a conference call then
       AnswerCallNow is returned as a B party is not required.
     */
    virtual PBoolean OnIncomingConnection(
      OpalConnection & connection,   ///<  Connection that is calling
      unsigned options,              ///<  options for new connection (can't use default as overrides will fail)
      OpalConnection::StringOptions * stringOptions
    );

    /**Route a connection to another connection from an endpoint.

       The default behaviour gets the destination address from the connection
       and translates it into an address by using the routeTable member
       variable and uses MakeConnection() to start the B-party connection.
      */
    virtual bool OnRouteConnection(
      PStringSet & routesTried,     ///< Set of routes already tried
      const PString & a_party,      ///< Source local address
      const PString & b_party,      ///< Destination indicated by source
      OpalCall & call,              ///< Call for new connection
      unsigned options,             ///< Options for new connection (can't use default as overrides will fail)
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

       The default behaviour calls the OnProceeding() on the connection's
       associated OpalCall object.
     */
    virtual void OnProceeding(
      OpalConnection & connection   ///<  Connection that is proceeeding
    );

    /**Call back for remote party being alerted on outgoing call.
       This function is called after the connection is informed that the
       remote endpoint is "ringing". This function is generally called
       some time after the MakeConnection() function was called.

       If PFalse is returned the connection is aborted.

       If an application overrides this function, it should generally call the
       ancestor version for correct operation. An application would typically
       only intercept this function if it wishes to do some form of logging.
       For this you can obtain the name of the caller by using the function
       OpalConnection::GetRemotePartyName().

       The default behaviour calls the OnAlerting() on the connection's
       associated OpalCall object.
     */
    virtual void OnAlerting(
      OpalConnection & connection   ///<  Connection that was established
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

       The default behaviour calls the OnConnected() on the connections
       associated OpalCall object.
      */
    virtual void OnConnected(
      OpalConnection & connection   ///<  Connection that was established
    );

    /**A call back function whenever a connection is "established".
       This indicates that a connection to an endpoint was established. This
       usually occurs after OnConnected() and indicates that the connection
       is both connected and has media flowing.

       In the context of H.323 this means that the CONNECT pdu has been
       received and either fast start was in operation or the subsequent Open
       Logical Channels have occurred. For SIP it indicates the INVITE/OK/ACK
       sequence is complete.

       The default behaviour calls the OnEstablished() on the connection's
       associated OpalCall object.
      */
    virtual void OnEstablished(
      OpalConnection & connection   ///<  Connection that was established
    );

    /**A call back function whenever a connection is released.
       This function can do any internal cleaning up and waiting on background
       threads that may be using the connection object.

       Classes that override this function should make sure they call the
       ancestor version for correct operation.

       An application will not typically call this function as it is used by
       the OpalManager during a release of the connection.

       The default behaviour calls OnReleased() on the connection's
       associated OpalCall object. This indicates to the call that the
       connection has been released so it can release the last remaining
       connection and then returns PTrue.
      */
    virtual void OnReleased(
      OpalConnection & connection   ///<  Connection that was established
    );
    
    /**A call back function whenever a connection is "held" or "retrieved".
       This indicates that a connection to an endpoint was held, or
       retrieved, either locally or by the remote endpoint.

       The default behaviour does nothing.
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
  //@}

  /**@name Media Streams management */
  //@{
    /**Get common media formats.
       This is called by various places to get common media formats for the
       basic connection classes.

       The default behaviour uses the mediaFormatOrder and mediaFormatMask
       member variables to adjust the mediaFormats list.
      */
    virtual OpalMediaFormatList GetCommonMediaFormats(
      bool transportable,  ///< Include transportable media formats
      bool pcmAudio        ///< Include raw PCM audio media formats
    ) const;

    /**Adjust media formats available on a connection.
       This is called by a connection after it has called
       OpalCall::GetMediaFormats() to get all media formats that it can use so
       that an application may remove or reorder the media formats before they
       are used to open media streams.

       The default behaviour uses the mediaFormatOrder and mediaFormatMask
       member variables to adjust the mediaFormats list.
      */
    virtual void AdjustMediaFormats(
      bool local,                         ///<  Media formats a local ones to be presented to remote
      const OpalConnection & connection,  ///<  Connection that is about to use formats
      OpalMediaFormatList & mediaFormats  ///<  Media formats to use
    ) const;

    /**See if the media can bypass the local host.
     */
    virtual PBoolean IsMediaBypassPossible(
      const OpalConnection & source,      ///<  Source connection
      const OpalConnection & destination, ///<  Destination connection
      unsigned sessionID                  ///<  Session ID for media channel
    ) const;

    /**Call back when opening a media stream.
       This function is called when a connection has created a new media
       stream according to the logic of its underlying protocol.

       The usual requirement is that media streams are created on all other
       connections participating in the call and all of the media streams are
       attached to an instance of an OpalMediaPatch object that will read from
       one of the media streams passing data to the other media streams.

       The default behaviour achieves the above using the FindMatchingCodecs()
       to determine what (if any) software codecs are required, the
       OpalConnection::CreateMediaStream() function to open streams and the
       CreateMediaPatch() function to create a patch for all of the streams
       and codecs just produced.
      */
    virtual PBoolean OnOpenMediaStream(
      OpalConnection & connection,  ///<  Connection that owns the media stream
      OpalMediaStream & stream    ///<  New media stream being opened
    );

    /**Create an RTP session.
       This function allows an application to have the system create descendant
       class versions of the RTP_UDP class. The application could use this to 
	   modify the default RTP transfer behaviour.
	   This is called when a connection determines that RTP is required for 
	   transporting media.
       The default behaviour returns a new RTP_UDP.
      */
	virtual RTP_UDP * CreateRTPSession (const RTP_Session::Params & params);

	/**Callback from the RTP session for statistics monitoring.
       This is called every so many packets on the transmitter and receiver
       threads of the RTP session indicating that the statistics have been
       updated.

       The default behaviour does nothing.
      */
    virtual void OnRTPStatistics(
      const OpalConnection & connection,  ///<  Connection for the channel
      const RTP_Session & session         ///<  Session with statistics
    );

    /**Indicate is a local RTP connection.
       This is called when a new media stream has been created and it has been
       detected that media will be flowing between two RTP sessions within the
       same process. An application could take advantage of this by optimising
       the transfer in some way, rather than the full media path of codecs and
       sockets which might not be necessary.

       Note this is the complement to SetMediaBypass() as this function stops
       RTP data from being sent/received, while SetMediaBypass() transfers
       RTP data between the two endpoints.

       The default behaviour returns false.

       @return true if the application is going to execute some form of
               bypass, and the media patch threads should not be started.
      */
    virtual bool OnLocalRTP(
      OpalConnection & connection1, ///< First connection
      OpalConnection & connection2, ///< Second connection
      unsigned sessionID,           ///< Session ID of RTP session
      bool opened                   ///< Media streams are opened/closed
    ) const;

    /**Set bypass mode for media.

       Bypass the internal media handling, passing RTP data directly from
       one call/connection to another.

       This can be useful for back to back calls that happen to be the same
       media format and you wish to avoid double decoding and encoding of
       media. Note this scenario is not the same as two OpalConnections within
       the same OpalCall, but two completely independent OpalCall where one
       connection is to be bypassed. For example, two OpalCall instances might
       have two SIPConnection instances and two OpalMixerConnection instances
       connected via a single OpalMixerNode. Now while there are ONLY two
       calls in the node, it is a waste to decode the audio, add to mixer and
       re-encode it again. In practice this is identical to just bypassing the
       mixer node completely, until a third party is added, then we need to
       switch back to normal (non-bypassed) operation.

       Note this is the complement to OnLocalRTP() as this function transfers
       RTP data directly between the two endpoints, while OnLocalRTP() stops
       the RTP data from being sent/received.

       @return true if bypass is started/stopped, false if there was no such
               call/connection/stream, the streams are incompatible formats
               or a conflicting bypass is already in place.
      */
    bool SetMediaBypass(
      const PString & token1, ///< First calls token
      const PString & token2, ///< Second calls token
      bool bypass,            ///< Bypass the media
      unsigned sessionID = 0, ///< Session ID of media stream, 0 indicates all
      bool network  = true    ///< Bypass the network connections of the calls
    );
    static bool SetMediaBypass(
      OpalConnection & connection1, ///< First connection
      OpalConnection & connection2, ///< Second connection
      bool bypass,                  ///< Bypass the media
      unsigned sessionID = 0        ///< Session ID of media stream, 0 indicates all
    );

    /**Call back for closed a media stream.

       The default behaviour does nothing.
      */
    virtual void OnClosedMediaStream(
      const OpalMediaStream & stream     ///<  Stream being closed
    );

#if OPAL_VIDEO
    /**Create a PVideoInputDevice for a source media stream.
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

    /**Create a OpalMediaPatch instance.
       This function allows an application to have the system create descendant
       class versions of the OpalMediPatch class. The application could use
       this to modify the default behaviour of a patch.

       The default behaviour returns an instance of OpalMediaPatch.
      */
    virtual OpalMediaPatch * CreateMediaPatch(
      OpalMediaStream & source,         ///<  Source media stream
      PBoolean requiresPatchThread = PTrue
    );

    /**Destroy a OpalMediaPatch instance.

       The default behaviour simply calls delete patch.
      */
    virtual void DestroyMediaPatch(
      OpalMediaPatch * patch
    );

    /**Call back for a media patch thread starting.
       This function is called within the context of the thread associated
       with the media patch.

       The default behaviour does nothing
      */
    virtual void OnStartMediaPatch(
      OpalConnection & connection,  ///< Connection patch is in
      OpalMediaPatch & patch        ///< Media patch being started
    );

    /**Call back when media stream patch thread stops.
      */
    virtual void OnStopMediaPatch(
      OpalConnection & connection,  ///< Connection patch is in
      OpalMediaPatch & patch        ///< Media Patch being stopped
    );
  //@}

  /**@name User indications */
  //@{
    /**Call back for remote endpoint has sent user input as a string.

       The default behaviour call OpalConnection::SetUserInput() which
       saves the value so the GetUserInput() function can return it.
      */
    virtual void OnUserInputString(
      OpalConnection & connection,  ///<  Connection input has come from
      const PString & value         ///<  String value of indication
    );

    /**Call back for remote enpoint has sent user input as tones.
       If duration is zero then this indicates the beginning of the tone. If
       duration is non-zero then it indicates the end of the tone output.

       The default behaviour calls the OpalCall function of the same name.
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
    enum MessageWaitingType { 
      NoMessageWaiting,
      VoiceMessageWaiting, 
      FaxMessageWaiting,
      PagerMessageWaiting,
      MultimediaMessageWaiting,
      TextMessageWaiting,
      NumMessageWaitingTypes
    };

    /**Callback called when Message Waiting Indication (MWI) is received
     */
    virtual void OnMWIReceived(
      const PString & party,    ///< Name of party MWI is for
      MessageWaitingType type,  ///< Type of message that is waiting
      const PString & extraInfo ///< Addition information on the MWI
    );
    
    
    class RouteEntry : public PObject
    {
        PCLASSINFO(RouteEntry, PObject);
      public:
        RouteEntry(const PString & pat, const PString & dest);
        void PrintOn(ostream & strm) const;
        PString            pattern;
        PString            destination;
        PRegularExpression regex;
    };
    PARRAY(RouteTable, RouteEntry);

    /**Add a route entry to the route table.

       The specification string is of the form:

                 pattern '=' destination 
       where:

            pattern      regular expression used to select route

            destination  destination for the call

       The "pattern" string regex is compared against routing strings that are built 
       as follows:
       
                 a_party '\\t' b_party

       where:

            a_party      name associated with a local connection i.e. "pots:vpb:1/2" or
                         "h323:myname@myhost.com". 

            b_party      destination specified by the call, which may be a full URI
                         or a simple digit string

       Note that all "pattern" strings have an implied '^' at the beginning and
       a '$' at the end. This forces the "pattern" to match the entire source string. 
       For convenience, the sub-expression ".*\\t" is inserted immediately after
       any ':' character if no '\\t' is present.

       Route entries are stored and searched in the route table in the order they are added. 
       
       The "destination" string is determines the endpoint used for the outbound
       leg of the route, when a match to the "pattern" is found. It can be a literal string, 
       or can be constructed using various meta-strings that correspond to parts of the source.
       See below for a list of the available meta-strings

       A "destination" starting with the string 'label:' causes the router to restart 
       searching from the beginning of the route table using the new string as the "a_party". 
       Thus, a route table with the folllwing entries:
       
                  "label:speeddial=h323:10.0.1.1" 
                  "pots:26=label:speeddial" 

       will produce the same result as the single entry "pots:26=h323:10.0.1.1".

       If the "destination" parameter is of the form @filename, then the file
       is read with each line consisting of a pattern=destination route
       specification. 
       
       "destination" strings without an equal sign or beginning with '#' are ignored.

       Pattern Regex Examples:
       -----------------------

       1) A local H.323 endpoint with with name of "myname@myhost.com" that receives a 
          call with a destination h323Id of "boris" would generate:
          
                          "h323:myname@myhost.com\\tboris"

       2) A local SIP endpoint with with name of "fred@nurk.com" that receives a 
          call with a destination of "sip:fred@nurk.com" would generate:
          
                          "sip:fred@nurk.com\\tsip:fred@nurk.com"

       3) Using line 0 of a PhoneJACK handset with a serial # of 01AB3F4 to dial
          the digits 2, 6 and # would generate:

                          "pots:Quicknet:01AB3F4:0\\t26"


       Destination meta-strings:
       -------------------------

       The available meta-strings are:
       
         <da>    Replaced by the "b_party" string. For example
                 "pc:.*\\t.* = sip:<da>" directs calls to the SIP protocol. In
                 this case there is a special condition where if the original
                 destination had a valid protocol, eg h323:fred.com, then
                 the entire string is replaced not just the <da> part.

         <db>    Same as <da>, but without the special condtion.

         <du>    Copy the "user" part of the "b_party" string. This is
                 essentially the component after the : and before the '@', or
                 the whole "b_party" string if these are not present.

         <!du>   The rest of the "b_party" string after the <du> section. The 
                 protocol is still omitted. This is usually the '@' and onward.
                 Note if there is already an '@' in the destination before the
                 <!du> and what is abour to replace it also has an '@' then
                 everything between the @ and the <!du> (inclusive) is deleted,
                 then the substitution is made so a legal URL can result.

         <dn>    Copy all valid consecutive E.164 digits from the "b_party" so
                 pots:0061298765@vpb:1/2 becomes sip:0061298765@carrier.com

         <dnX>   As above but skip X digits, eg <dn2> skips 2 digits, so
                 pots:00612198765 becomes sip:61298765@carrier.com

         <!dn>   The rest of the "b_party" after the <dn> or <dnX> sections.

         <dn2ip> Translate digits separated by '*' characters to an IP
                 address. e.g. 10*0*1*1 becomes 10.0.1.1, also
                 1234*10*0*1*1 becomes 1234@10.0.1.1 and
                 1234*10*0*1*1*1722 becomes 1234@10.0.1.1:1722.


       Returns true if an entry was added.
      */
    virtual PBoolean AddRouteEntry(
      const PString & spec  ///<  Specification string to add
    );

    /**Parse a route table specification list for the manager.
       This removes the current routeTable and calls AddRouteEntry for every
       string in the array.

       Returns PTrue if at least one entry was added.
      */
    PBoolean SetRouteTable(
      const PStringArray & specs  ///<  Array of specification strings.
    );

    /**Set a route table for the manager.
       Note that this will make a copy of the table and not maintain a
       reference.
      */
    void SetRouteTable(
      const RouteTable & table  ///<  New table to set for routing
    );

    /**Get the active route table for the manager.
      */
    const RouteTable & GetRouteTable() const { return m_routeTable; }

    /**Route the source address to a destination using the route table.
       The source parameter may be something like pots:vpb:1/2 or
       sip:fred@nurk.com.

       The destination parameter is a partial URL, it does not include the
       protocol, but may be of the form user@host, or simply digits.
      */
    virtual PString ApplyRouteTable(
      const PString & source,      /// Source address, including endpoint protocol
      const PString & destination, /// Destination address read from source protocol
      PINDEX & entry
    );
  //@}

#if OPAL_HAS_MIXER
  /**@name Call recording */
  //@{
    /**Start recording a call.
       Current version saves to a WAV file. It may either mix the receive and
       transmit audio stream to a single mono file, or the streams are placed
       into the left and right channels of a stereo WAV file.

       Returns true if the call exists and there is no recording in progress
               for the call.
      */
    virtual PBoolean StartRecording(
      const PString & callToken,  ///< Call token for call to record
      const PFilePath & filename, ///< File into which to record
      const OpalRecordManager::Options & options = false ///< Record mixing options
    );

    /**Indicate if recording is currently active on call.
      */
    virtual bool IsRecording(
      const PString & callToken   ///< Call token for call to check if recording
    );

    /** Stop a recording.
        Returns true if the call does exists, that recording is active is
                not indicated.
      */
    virtual bool StopRecording(
      const PString & callToken   ///< Call token for call to stop recording
    );

  //@}
#endif

  /**@name Member variable access */
  //@{
    /**Get the product info for all endpoints.
      */
    const OpalProductInfo & GetProductInfo() const { return productInfo; }

    /**Set the product info for all endpoints.
      */
    void SetProductInfo(
      const OpalProductInfo & info, ///< New information
      bool updateAll = true         ///< Update all registered endpoints
    );

    /**Get the default username for all endpoints.
      */
    const PString & GetDefaultUserName() const { return defaultUserName; }

    /**Set the default username for all endpoints.
      */
    void SetDefaultUserName(
      const PString & name,   ///< New name
      bool updateAll = true   ///< Update all registered endpoints
    );

    /**Get the default display name for all endpoints.
      */
    const PString & GetDefaultDisplayName() const { return defaultDisplayName; }

    /**Set the default display name for all endpoints.
      */
    void SetDefaultDisplayName(
      const PString & name,   ///< New name
      bool updateAll = true   ///< Update all registered endpoints
    );

#if OPAL_VIDEO

    //
    // these functions are deprecated and used only for backwards compatibility
    // applications should use OpalConnection::GetAutoStart() to check whether
    // a specific media type can be auto-started
    //

    /**See if should auto-start receive video channels on connection.
     */
    bool CanAutoStartReceiveVideo() const { return (OpalMediaType::Video().GetAutoStart()&OpalMediaType::Receive) != 0; }

    /**Set if should auto-start receive video channels on connection.
     */
    void SetAutoStartReceiveVideo(bool can) { OpalMediaType::Video().GetDefinition()->SetAutoStart(OpalMediaType::Receive, can); }

    /**See if should auto-start transmit video channels on connection.
     */
    bool CanAutoStartTransmitVideo() const { return (OpalMediaType::Video().GetAutoStart()&OpalMediaType::Transmit) != 0; }

    /**Set if should auto-start transmit video channels on connection.
     */
    void SetAutoStartTransmitVideo(bool can) { OpalMediaType::Video().GetDefinition()->SetAutoStart(OpalMediaType::Transmit, can); }
    void SetAutoStartTransmitAudio(bool can) { OpalMediaType::Audio().GetDefinition()->SetAutoStart(OpalMediaType::Transmit, can); }

#endif

    /**Determine if the address is "local", ie does not need any address
       translation (fixed or via STUN) to access.

       The default behaviour checks if remoteAddress is a RFC1918 private
       IP address: 10.x.x.x, 172.16.x.x or 192.168.x.x.
     */
    virtual PBoolean IsLocalAddress(
      const PIPSocket::Address & remoteAddress
    ) const;

    /**Determine if the RTP session needs to accommodate a NAT router.
       For endpoints that do not use STUN or something similar to set up all the
       correct protocol embeddded addresses correctly when a NAT router is between
       the endpoints, it is possible to still accommodate the call, with some
       restrictions. This function determines if the RTP can proceed with special
       NAT allowances.

       The special allowance is that the RTP code will ignore whatever the remote
       indicates in the protocol for the address to send RTP data and wait for
       the first packet to arrive from the remote and will then proceed to send
       all RTP data back to that address AND port.

       The default behaviour checks the values of the physical link
       (localAddr/peerAddr) against the signaling address the remote indicated in
       the protocol, eg H.323 SETUP sourceCallSignalAddress or SIP "To" or
       "Contact" fields, and makes a guess that the remote is behind a NAT router.
     */
    virtual PBoolean IsRTPNATEnabled(
      OpalConnection & connection,            ///< Connection being checked
      const PIPSocket::Address & localAddr,   ///< Local physical address of connection
      const PIPSocket::Address & peerAddr,    ///< Remote physical address of connection
      const PIPSocket::Address & signalAddr,  ///< Remotes signaling address as indicated by protocol of connection
      PBoolean incoming                       ///< Incoming/outgoing connection
    );

    /**Provide address translation hook.
       This will check to see that remoteAddress is NOT a local address by
       using IsLocalAddress() and if not, set localAddress to the
       translationAddress (if valid) which would normally be the router
       address of a NAT system.
     */
    virtual PBoolean TranslateIPAddress(
      PIPSocket::Address & localAddress,
      const PIPSocket::Address & remoteAddress
    );

    /**Get the translation host to use for TranslateIPAddress().
      */
    const PString & GetTranslationHost() const { return translationHost; }

    /**Set the translation host to use for TranslateIPAddress().
      */
    bool SetTranslationHost(
      const PString & host
    );

    /**Get the translation address to use for TranslateIPAddress().
      */
    const PIPSocket::Address & GetTranslationAddress() const { return translationAddress; }

    /**Set the translation address to use for TranslateIPAddress().
      */
    void SetTranslationAddress(
      const PIPSocket::Address & address
    );

    /**Return the NAT method to use.
       Returns NULL if address is a local address as per IsLocalAddress().
       Always returns the NAT method if address is zero.
       Note, the pointer is NOT to be deleted by the user.
      */
    virtual PNatMethod * GetNatMethod(
      const PIPSocket::Address & remoteAddress = PIPSocket::GetDefaultIpAny()
    ) const;

    /**Set the STUN server address, is of the form host[:port]
       Note that if the STUN server is found then the translationAddress
       is automatically set to the router address as determined by STUN.
      */
    PSTUNClient::NatTypes SetSTUNServer(
      const PString & server
    );

    /**Get the current host name and optional port for the STUN server.
      */
    const PString & GetSTUNServer() const { return stunServer; }

    /**Return the STUN client instance in use.
      */
    PSTUNClient * GetSTUNClient() const { return stun; }

    /**Get the TCP port number base for H.245 channels
     */
    WORD GetTCPPortBase() const { return tcpPorts.base; }

    /**Get the TCP port number base for H.245 channels.
     */
    WORD GetTCPPortMax() const { return tcpPorts.max; }

    /**Set the TCP port number base and max for H.245 channels.
     */
    void SetTCPPorts(unsigned tcpBase, unsigned tcpMax);

    /**Get the next TCP port number for H.245 channels
     */
    WORD GetNextTCPPort();

    /**Get the UDP port number base for RAS channels
     */
    WORD GetUDPPortBase() const { return udpPorts.base; }

    /**Get the UDP port number base for RAS channels.
     */
    WORD GetUDPPortMax() const { return udpPorts.max; }

    /**Set the TCP port number base and max for H.245 channels.
     */
    void SetUDPPorts(unsigned udpBase, unsigned udpMax);

    /**Get the next UDP port number for RAS channels
     */
    WORD GetNextUDPPort();

    /**Get the UDP port number base for RTP channels.
     */
    WORD GetRtpIpPortBase() const { return rtpIpPorts.base; }

    /**Get the max UDP port number for RTP channels.
     */
    WORD GetRtpIpPortMax() const { return rtpIpPorts.max; }

    /**Set the UDP port number base and max for RTP channels.
     */
    void SetRtpIpPorts(unsigned udpBase, unsigned udpMax);

    /**Get the UDP port number pair for RTP channels.
     */
    WORD GetRtpIpPortPair();

    /**Get the IP Type Of Service byte for media (eg RTP) channels.
     */
    BYTE GetRtpIpTypeofService() const { return rtpIpTypeofService; }

    /**Set the IP Type Of Service byte for media (eg RTP) channels.
     */
    void SetRtpIpTypeofService(unsigned tos) { rtpIpTypeofService = (BYTE)tos; }

    /**Get the maximum transmitted RTP payload size.
       Defaults to maximum safe MTU size (576 bytes as per RFC879) minus the
       typical size of the IP, UDP an RTP headers.
      */
    PINDEX GetMaxRtpPayloadSize() const { return rtpPayloadSizeMax; }

    /**Get the maximum transmitted RTP payload size.
       Defaults to maximum safe MTU size (576 bytes as per RFC879) minus the
       typical size of the IP, UDP an RTP headers.
      */
    void SetMaxRtpPayloadSize(
      PINDEX size,
      bool mtu = false
    ) { rtpPayloadSizeMax = size - (mtu ? (20+16+12) : 0); }

    /**Get the maximum received RTP packet size.
       Defaults to 2048.
      */
    PINDEX GetMaxRtpPacketSize() const { return rtpPacketSizeMax; }

    /**Get the maximum transmitted RTP payload size.
       Defaults to 2048.
      */
    void SetMaxRtpPacketSize(
      PINDEX size
    ) { rtpPacketSizeMax = size; }

    /**Get the default maximum audio jitter delay parameter.
       Defaults to 50ms
     */
    unsigned GetMinAudioJitterDelay() const { return minAudioJitterDelay; }

    /**Get the default maximum audio jitter delay parameter.
       Defaults to 250ms.
     */
    unsigned GetMaxAudioJitterDelay() const { return maxAudioJitterDelay; }

    /**Set the maximum audio jitter delay parameter.

       If minDelay is set to zero then both the minimum and maximum will be
       set to zero which will disable the jitter buffer entirely.

       If maxDelay is zero, or just less that minDelay, then the maximum
       jitter is set to the minimum and this disables the adaptive jitter, a
       fixed value is used.
     */
    void SetAudioJitterDelay(
      unsigned minDelay,   ///<  New minimum jitter buffer delay in milliseconds
      unsigned maxDelay    ///<  New maximum jitter buffer delay in milliseconds
    );

    /**Get the default media format order.
     */
    const PStringArray & GetMediaFormatOrder() const { return mediaFormatOrder; }

    /**Set the default media format order.
     */
    void SetMediaFormatOrder(
      const PStringArray & order   ///< New order
    );

    /**Get the default media format mask.
       The is the default list of media format names to be removed from media
       format lists bfeore use by a connection.
       See OpalMediaFormatList::Remove() for more information.
     */
    const PStringArray & GetMediaFormatMask() const { return mediaFormatMask; }

    /**Set the default media format mask.
       The is the default list of media format names to be removed from media
       format lists bfeore use by a connection.
       See OpalMediaFormatList::Remove() for more information.
     */
    void SetMediaFormatMask(
      const PStringArray & mask   //< New mask
    );

    /**Set the default parameters for the silence detector.
     */
    virtual void SetSilenceDetectParams(
      const OpalSilenceDetector::Params & params
    ) { silenceDetectParams = params; }

    /**Get the default parameters for the silence detector.
     */
    const OpalSilenceDetector::Params & GetSilenceDetectParams() const { return silenceDetectParams; }
    
#if OPAL_AEC
    /**Set the default parameters for the echo cancelation.
     */
    virtual void SetEchoCancelParams(
      const OpalEchoCanceler::Params & params
    ) { echoCancelParams = params; }

    /**Get the default parameters for the silence detector.
     */
    const OpalEchoCanceler::Params & GetEchoCancelParams() const { return echoCancelParams; }
#endif

#if OPAL_VIDEO

    /**Set the parameters for the video device to be used for input.
       If the name is not suitable for use with the PVideoInputDevice class
       then the function will return PFalse and not change the device.

       This defaults to the value of the PVideoInputDevice::GetInputDeviceNames()
       function.
     */
    virtual PBoolean SetVideoInputDevice(
      const PVideoDevice::OpenArgs & deviceArgs ///<  Full description of device
    );

    /**Get the parameters for the video device to be used for input.
       This defaults to the value of the PSoundChannel::GetInputDeviceNames()[0].
     */
    const PVideoDevice::OpenArgs & GetVideoInputDevice() const { return videoInputDevice; }

    /**Set the parameters for the video device to be used to preview input.
       If the name is not suitable for use with the PVideoOutputDevice class
       then the function will return PFalse and not change the device.

       This defaults to the value of the PVideoInputDevice::GetOutputDeviceNames()
       function.
     */
    virtual PBoolean SetVideoPreviewDevice(
      const PVideoDevice::OpenArgs & deviceArgs ///<  Full description of device
    );

    /**Get the parameters for the video device to be used for input.
       This defaults to the value of the PSoundChannel::GetInputDeviceNames()[0].
     */
    const PVideoDevice::OpenArgs & GetVideoPreviewDevice() const { return videoPreviewDevice; }

    /**Set the parameters for the video device to be used for output.
       If the name is not suitable for use with the PVideoOutputDevice class
       then the function will return PFalse and not change the device.

       This defaults to the value of the PVideoInputDevice::GetOutputDeviceNames()
       function.
     */
    virtual PBoolean SetVideoOutputDevice(
      const PVideoDevice::OpenArgs & deviceArgs ///<  Full description of device
    );

    /**Get the parameters for the video device to be used for input.
       This defaults to the value of the PSoundChannel::GetOutputDeviceNames()[0].
     */
    const PVideoDevice::OpenArgs & GetVideoOutputDevice() const { return videoOutputDevice; }

#endif

    PBoolean DetectInBandDTMFDisabled() const
      { return disableDetectInBandDTMF; }

    /**Set the default H.245 tunneling mode.
      */
    void DisableDetectInBandDTMF(
      PBoolean mode ///<  New default mode
    ) { disableDetectInBandDTMF = mode; } 

    /**Get the amount of time with no media that should cause a call to clear
     */
    const PTimeInterval & GetNoMediaTimeout() const { return noMediaTimeout; }

    /**Set the amount of time with no media that should cause a call to clear
     */
    PBoolean SetNoMediaTimeout(
      const PTimeInterval & newInterval  ///<  New timeout for media
    );

    /**Get the default ILS server to use for user lookup.
      */
    const PString & GetDefaultILSServer() const { return ilsServer; }

    /**Set the default ILS server to use for user lookup.
      */
    void SetDefaultILSServer(
      const PString & server
    ) { ilsServer = server; }
  //@}

    // needs to be public for gcc 3.4
    void GarbageCollection();

#ifdef OPAL_ZRTP
    virtual bool GetZRTPEnabled() const;
#endif

    virtual void OnApplyStringOptions(
      OpalConnection & conn,
      OpalConnection::StringOptions & stringOptions
    );

  protected:
    // Configuration variables
    OpalProductInfo productInfo;

    PString       defaultUserName;
    PString       defaultDisplayName;

    BYTE          rtpIpTypeofService;
    PINDEX        rtpPayloadSizeMax;
    PINDEX        rtpPacketSizeMax;
    unsigned      minAudioJitterDelay;
    unsigned      maxAudioJitterDelay;
    PStringArray  mediaFormatOrder;
    PStringArray  mediaFormatMask;
    PBoolean          disableDetectInBandDTMF;
    PTimeInterval noMediaTimeout;
    PString       ilsServer;

    OpalSilenceDetector::Params silenceDetectParams;
#if OPAL_AEC
    OpalEchoCanceler::Params echoCancelParams;
#endif

#if OPAL_VIDEO
    PVideoDevice::OpenArgs videoInputDevice;
    PVideoDevice::OpenArgs videoPreviewDevice;
    PVideoDevice::OpenArgs videoOutputDevice;
#endif

    struct PortInfo {
      void Set(
        unsigned base,
        unsigned max,
        unsigned range,
        unsigned dflt
      );
      WORD GetNext(
        unsigned increment
      );

      PMutex mutex;
      WORD   base;
      WORD   max;
      WORD   current;
    } tcpPorts, udpPorts, rtpIpPorts;
    
    class InterfaceMonitor : public PInterfaceMonitorClient
    {
      PCLASSINFO(InterfaceMonitor, PInterfaceMonitorClient);
      
      enum {
        OpalManagerInterfaceMonitorClientPriority = 100,
      };
      public:
        InterfaceMonitor(OpalManager & manager);
        
      protected:
        virtual void OnAddInterface(const PIPSocket::InterfaceEntry & entry);
        virtual void OnRemoveInterface(const PIPSocket::InterfaceEntry & entry);
        
        OpalManager & m_manager;
    };

    PString            translationHost;
    PIPSocket::Address translationAddress;
    PString            stunServer;
    PSTUNClient      * stun;
    InterfaceMonitor * interfaceMonitor;

    RouteTable m_routeTable;
    PMutex     m_routeMutex;

    // Dynamic variables
    PReadWriteMutex     endpointsMutex;
    PList<OpalEndPoint> endpointList;
    std::map<PString, OpalEndPoint *> endpointMap;

    PAtomicInteger lastCallTokenID;

    class CallDict : public PSafeDictionary<PString, OpalCall>
    {
      public:
        CallDict(OpalManager & mgr) : manager(mgr) { }
        virtual void DeleteObject(PObject * object) const;
        OpalManager & manager;
    } activeCalls;

    PSafeDictionary<PString, OpalPresentity> m_presentities;

    PAtomicInteger m_clearingAllCallsCount;
    PMutex         m_clearingAllCallsMutex;
    PSyncPoint     m_allCallsCleared;
    void InternalClearAllCalls(OpalConnection::CallEndReason reason, bool wait, bool first);

    PThread    * garbageCollector;
    PSyncPoint   garbageCollectExit;
    PDECLARE_NOTIFIER(PThread, OpalManager, GarbageMain);

#ifdef OPAL_ZRTP
    bool zrtpEnabled;
#endif

    friend OpalCall::OpalCall(OpalManager & mgr);
    friend void OpalCall::InternalOnClear();

  private:
    P_REMOVE_VIRTUAL(OpalCall *,CreateCall(), 0);
    P_REMOVE_VIRTUAL(PBoolean, OnIncomingConnection(OpalConnection &, unsigned), false);
    P_REMOVE_VIRTUAL(PBoolean, OnIncomingConnection(OpalConnection &), false);
    P_REMOVE_VIRTUAL(PBoolean, OnStartMediaPatch(const OpalMediaPatch &), false);
    P_REMOVE_VIRTUAL_VOID(AdjustMediaFormats(const OpalConnection &, OpalMediaFormatList &) const);
};


PString  OpalGetVersion();
unsigned OpalGetMajorVersion();
unsigned OpalGetMinorVersion();
unsigned OpalGetBuildNumber();


#endif // OPAL_OPAL_MANAGER_H


// End of File ///////////////////////////////////////////////////////////////
