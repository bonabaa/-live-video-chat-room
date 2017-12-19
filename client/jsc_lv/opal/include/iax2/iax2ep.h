/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Describes the IAX2 extension of the OpalEndpoint class.
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
 * $Revision: 23008 $
 * $Author: rjongbloed $
 * $Date: 2009-06-29 04:24:23 +0000 (Mon, 29 Jun 2009) $
 */

#ifndef OPAL_IAX2_IAX2EP_H
#define OPAL_IAX2_IAX2EP_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_IAX2

#include <opal/endpoint.h>
#include <iax2/iax2con.h>
#include <iax2/processor.h>
#include <iax2/regprocessor.h>
#include <iax2/specialprocessor.h>

class IAX2Receiver;
class IAX2Transmit;
class IAX2Processor;

/** A class to take frames from the transmitter, and disperse them to
    the appropriate IAX2Connection class.  This class calls a method in
    the IAX2EndPoint to do the dispersal. */
class IAX2IncomingEthernetFrames : public PThread
{
  PCLASSINFO(IAX2IncomingEthernetFrames, PThread);
public:

  /**@name Constructors/destructors*/
  //@{
  /**Construct a distributor, to send packets on to the relevant connection */
  IAX2IncomingEthernetFrames();
   
  /**Destroy the distributor */
  ~IAX2IncomingEthernetFrames() { }

  /**@name general worker methods*/
  //@{
  /*The method which gets everythig to happen */
  virtual void Main();
   
  /**Set the endpoint variable */
  void Assign(IAX2EndPoint *ep);

  /**Activate this thread to process all frames in the lists
   */
  void ProcessList();

  /**Cause this thread to die immediately */
  void Terminate();

  //@}
 protected:
  /**Global variable which holds the application specific data */
  IAX2EndPoint *endpoint;
   
  /**Flag to activate this thread*/
  PSyncPoint activate;

  /**Flag to indicate if this receiver thread should keep listening for network data */
  PBoolean           keepGoing;
};




/** A class to manage global variables. There is one Endpoint per application. */
class IAX2EndPoint : public OpalEndPoint
{
  PCLASSINFO(IAX2EndPoint, OpalEndPoint);
 public:
  /**@name Construction */
  
  //@{
  /**Create the endpoint, and define local variables */
  IAX2EndPoint(
    OpalManager & manager
  );
  
  /**Destroy the endpoint, and all associated connections*/
  ~IAX2EndPoint();
  //@}
  
  /**@name connection Connection handling */
  //@{
  /**Handle new incoming connection from listener.

  The default behaviour does nothing.
  */
  virtual PBoolean NewIncomingConnection(
    OpalTransport * transport  /// Transport connection came in on
  );
  
  /**Set up a connection to a remote party.
     This is called from the OpalManager::MakeConnection() function once
     it has determined that this is the endpoint for the protocol.
     
     The general form for this party parameter is:
     
       [iax2:]{username@][transport$]address[/extension][+context]
     
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
			      void * userData = NULL,   ///<  Arbitrary data to pass to connection
			      unsigned int options = 0, ///<  options to pass to connection
			      OpalConnection::StringOptions * stringOptions = NULL
			      );
  
  /**Create a connection for the IAX endpoint.
     The default implementation is to create a IAX2Connection.
  */
  virtual IAX2Connection * CreateConnection(
					   OpalCall & call,            /// Owner of connection
					   const PString & token,      /// token used to identify connection
					   void * userData,             /// User data for connection
					   const PString & remoteParty, /// Url to call or is calling.
             const PString & remotePartyName = PString::Empty() /// Name to call or is calling.
					   );
  //@}
  
  /**@name worker Methods*/
  //@{
  /**Setup the Endpoint internval variables, which is called at program 
     startup.*/
  PBoolean Initialise();

  /**Handle a received IAX frame. This may be a mini frame or full frame */
  virtual void IncomingEthernetFrame (IAX2Frame *frame);
  
  /**A simple test to report if the connection associated with this
     frame is still alive. This test is used when transmitting the
     frame. If the connection is gone, don't bother transmitting the
     frame. There are exceptins to this rule, such as when a hangup
     packet is sent (which is after the connections has died. */
  PBoolean ConectionForFrameIsAlive(IAX2Frame *f);
  
  /**Request a new source call number, one that is different to 
     all other source call numbers for this program.  

     @return P_MAX_INDEX  if there is no available call number,
     or return a unique valid call number.
     */
  PINDEX NextSrcCallNumber(IAX2Processor * processor);
      
  /**Write the token of all connections in the connectionsActive
     structure to the trace file */
  void ReportStoredConnections();
  
  /**Report the port in use for IAX calls */
  WORD ListenPortNumber()  { return 4569; }
      
  /**Pointer to the transmitter class, which is always valid*/
  IAX2Transmit *transmitter;
  
  /**Pointer to the receiver class, which is always valid*/
  IAX2Receiver    *receiver;
  
  /**Report the local username*/
  PString GetLocalUserName() { return localUserName; }
  
  /**Report the number used by the computer running this program*/
  PString GetLocalNumber() { return localNumber; }
  
  /**Set the username to some value */
  void SetLocalUserName(PString newValue); 
  
  /**Set the local (on this host) number to some value */
  void SetLocalNumber(PString newValue);
  
  /**Report the password*/
  PString & GetPassword() { return password; }
  
  /**Set the password to some value */
  void SetPassword(PString newValue);

  /**It is possible that a retransmitted frame has been in the transmit queue,
     and while sitting there that frames sending connection has died.  Thus,
     prior to transmission, call tis method.

     @return True if a connection (which matches this Frame ) can be
     found. */
  PBoolean ConnectionForFrameIsAlive(IAX2Frame *f);
 
  /**Get out sequence number to use on status query frames*/
  PINDEX GetOutSequenceNumberForStatusQuery();
  
  /**We have an incoming call. Do we accept ? */
  void StartRinging(PString remoteCaller);
  
    /**Handle new incoming connection from listener.
       
    A return value of PTrue indicates that the transport object should be
    deleted by the caller. PFalse indicates that something else (eg the
    connection) has taken over responsibility for deleting the transport.
    
    Well, that is true of Opal. In iax2, we do all the work of creating a new
    connection etc.  The transport arguement is ignore. In Iax2, this method
    is void, as no value is returned.  Further, in iax2, we process the
    incoming frame.
    */
    void NewIncomingConnection(
      IAX2Frame *f  /// Frame carrying the new request.
		);

    /**A call back function whenever a connection is established.
       This indicates that a connection to an endpoint was established. This
       usually occurs after OnConnected() and indicates that the connection
       is both connected and has media flowing.
      */
    void OnEstablished(
       OpalConnection & con
    );

    /**Called whenever a connection instance is finished being used to
       manage a call. We trap this callback to remove the connection
       from this endpoints token table. Once we are done the token
       table, we then call the generic OpalEndpoint::OnReleased
       method. */
    virtual void OnReleased(
      OpalConnection & connection   ///<  Connection that was established
    );

   /**Get the data formats this endpoint is capable of operating.  This
       provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour is pure.
      */
  virtual OpalMediaFormatList GetMediaFormats() const;

  /**Return the bitmask which specifies the possible codecs we support */
  PINDEX GetSupportedCodecs(OpalMediaFormatList & list);
  
  /**Return the bitmask which specifies the preferred codec */
  PINDEX GetPreferredCodec(OpalMediaFormatList & list);

  /**Get the frame size (bytes) and frame duration (ms) for compressed
     data from this codec */
  void GetCodecLengths(PINDEX src, PINDEX &compressedBytes, PINDEX &duration);
  
  /**enum of the components from a remote party address string 
     These fields are from the address,
  
      [iax2:]{username@][transport$]address[/extension][+context]
  */
  enum IAX2RemoteAddressFields {
    protoIndex     = 0,     /*!< the protocol, or iax2: field            */
    userIndex      = 1,     /*!< the username, or alias field            */
    transportIndex = 2,     /*!< the transport, or transport field       */
    addressIndex   = 3,     /*!< the address, or 192.168.1.1 field       */
    extensionIndex = 4,     /*!< the extension, or "extension"" field    */
    contextIndex   = 5,     /*!< the context,   or "+context" field      */
    maximumIndex   = 6      /*!< the number of possible fields           */
  };

  /**Given a remote party name of the format:

     [proto:][alias@][transport$]address[/extension]

     pull the string apart and get the components. The compoents are stored
     in a PStringList, indexed by the enum RemoteAddressFields */
  static PStringArray DissectRemoteParty(const PString & other);

  /**Pull frames off the incoming list, and pass on to the relevant
     connection. If no matching connection found, delete the frame.
     Repeat the process until no frames are left. */
  void ProcessReceivedEthernetFrames();

  /**Report on the frames in the current transmitter class, which are
     pending transmission*/
  void ReportTransmitterLists(PString & answer, bool getFullReport = false);

  /**Copy to the supplied OpalMediaList the media formats we support*/
  void CopyLocalMediaFormats(OpalMediaFormatList & list);
  
  /**Register with a remote iax2 server.  The host can either be a 
     hostname or ip address.  The password is optional as some servers
     may not require it to register.  The requested refresh time is the
     time that the registration should be refreshed in seconds.  The time
     must be more than 10 seconds.*/
  void Register(
      const PString & host,
      const PString & username,
      const PString & password = PString::Empty(),
      PINDEX requestedRefreshTime = 60
    );
  
  enum RegisteredError {
    RegisteredFailureUnknown
  };
  
  /**This is a call back if an event related to registration occurs.
     This callback should return as soon as possible.*/
  virtual void OnRegistered(
      const PString & host,
      const PString & userName,
      PBoolean isFailure,
      RegisteredError reason = RegisteredFailureUnknown);
   
   /**Unregister from a registrar. This function is synchronous so it
      will block.*/
  void Unregister(
      const PString & host,
      const PString & username);
      
  enum UnregisteredError {
    UnregisteredFailureUnknown
  };
      
  /**This is a call back if an event related to unregistration occurs.
     This callback should return as soon as possible.  Generally even if
     a failure occurs when unregistering it should be ignored because it
     does not matter to much that it couldn't unregister.*/
  virtual void OnUnregistered(
      const PString & host,
      const PString & userName,
      PBoolean isFailure,
      UnregisteredError reason = UnregisteredFailureUnknown);
      
  
  /**Check if an account is registered or being registered*/
  PBoolean IsRegistered(const PString & host, const PString & username);
  
  /**Get the number of accounts that are being registered*/
  PINDEX GetRegistrationsCount();
  
  /**Builds a url*/
  PString BuildUrl(
    const PString & host,
    const PString & userName = PString::Empty(),
    const PString & extension = PString::Empty(),
    const PString & context = PString::Empty(),
    const PString & transport = PString::Empty()
  );
  
  /**Report if this iax2 endpoint class is correctly initialised */
  PBoolean InitialisedOK() { return (transmitter != NULL) && (receiver != NULL); }
  //@}
  
 protected:
  /**Thread which transfers frames from the Receiver to the
     appropriate connection.  It momentarily locks the connection
     list, searches through, and then completes the trasnsfer. If need
     be, this thread will create a new conneciton (to cope with a new
     incoming call) and add the new connections to the internal
     list. */
  IAX2IncomingEthernetFrames incomingFrameHandler;

  /**List of iax packets which has been read from the ethernet, and
     is to be sent to the matching IAX2Connection */
  IAX2FrameList   packetsReadFromEthernet;
  
  /**The socket on which all data is sent/received.*/
  PUDPSocket  *sock;
  
  /**Number of active calls */
  int callnumbs;
  
  /** lock on access to call numbers variable */
  PMutex callNumbLock;
  
  /**Time when a call was started */
  PTime callStartTime;
  
  /**Name of this user, which is used as the IeCallingNumber */
  PString localUserName;
  
  /**Number, as used by the computer on the host running this program*/
  PString localNumber;
  
  /**Password for this user, which is used when processing an authentication request */
  PString password;
  
  /**Counter to use for sending on status query frames */
  PINDEX statusQueryCounter;
  
  /**Mutex for the statusQueryCounter */
  PMutex statusQueryMutex;
  
  /**Pointer to the Processor class which handles special packets (eg lagrq) that have no 
     destination call to handle them. */
  IAX2SpecialProcessor * specialPacketHandler;
    
  /**For the supplied IAX2Frame, pass it to a connection in the
     connectionsActive structure.  If no matching connection is found, return
     PFalse;
     
     If a matching connections is found, give the frame to the
     connection (for the connection to process) and return PTrue;
  */
  PBoolean ProcessInMatchingConnection(IAX2Frame *f);  
    
  /**The TokenTranslationDict may need a new entry. Examine
     the list of active connections, to see if any match this frame.
     If any do, then we add a new translation entry in tokenTable;
     
     If a matching connection is found in connectionsActive, create a
     new translation entry and return PTrue. The connection, after
     processing the frame, will then delete the frame. */
  PBoolean AddNewTranslationEntry(IAX2Frame *f);
  
  /**tokenTable is a hack to allow IAX2 to fit in with one of the
     demands of the opal library. 
     
     Opal demands that at connection setup, we know the unique ID which 
     this call will use. 

     Since the unique ID is remote ip adress + remote's Source Call
     number, this is unknown if we are initiating the
     call. Consequently, this table is needed, as it provides a
     translation between the initial (or psuedo) token and the
     token that is later adopted */

  PStringToString    tokenTable;
  
  /**Threading mutex on the variable tokenTable. We can now safely
     read/write to this table, with the minimum of interference between
     threads.  */
  PReadWriteMutex    mutexTokenTable;

  /**Thread safe counter which keeps track of the calls created by this endpoint.
     This value is used when giving outgoing calls a unique ID */
  PAtomicInteger callsEstablished;

  /**Local copy of the media types we can handle*/
  OpalMediaFormatList localMediaFormats;
  
   /**A mutex to protect the registerProcessors collection*/
  PMutex regProcessorsMutex;
  
  /**An array of register processors.  These are created when
     another class calls register and deleted when another class
     calls unregister or class destructor is called.  This collection
     must be protected by the regProcessorsMutex*/
  PArrayObjects regProcessors;
  
};


#endif // OPAL_IAX2

#endif // OPAL_IAX2_IAX2EP_H

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 2 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-basic-offset:2
 * End:
 */

