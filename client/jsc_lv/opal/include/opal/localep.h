/*
 * localep.h
 *
 * Local EndPoint/Connection.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2008 Vox Lucida Pty. Ltd.
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
 * $Revision: 23008 $
 * $Author: rjongbloed $
 * $Date: 2009-06-29 04:24:23 +0000 (Mon, 29 Jun 2009) $
 */

#ifndef OPAL_OPAL_LOCALEP_H
#define OPAL_OPAL_LOCALEP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/endpoint.h>

class OpalLocalConnection;


/** Local EndPoint.
    This class represents an endpoint on the local machine that can receive
    media via virtual functions.
 */
class OpalLocalEndPoint : public OpalEndPoint
{
    PCLASSINFO(OpalLocalEndPoint, OpalEndPoint);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalLocalEndPoint(
      OpalManager & manager,        ///<  Manager of all endpoints.
      const char * prefix = "local" ///<  Prefix for URL style address strings
    );

    /**Destroy endpoint.
     */
    ~OpalLocalEndPoint();
  //@}

  /**@name Overrides from OpalEndPoint */
  //@{
    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour returns the most basic media formats, PCM audio
       and YUV420P video.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

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
       false is returned.

       This function usually returns almost immediately with the connection
       continuing to occur in a new background thread.

       If false is returned then the connection could not be established. For
       example if a PSTN endpoint is used and the assiciated line is engaged
       then it may return immediately. Returning a non-NULL value does not
       mean that the connection will succeed, only that an attempt is being
       made.

       The default behaviour is pure.
     */
    virtual PSafePtr<OpalConnection> MakeConnection(
      OpalCall & call,           ///<  Owner of connection
      const PString & party,     ///<  Remote party to call
      void * userData = NULL,    ///<  Arbitrary data to pass to connection
      unsigned int options = 0,  ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions  = NULL
    );
  //@}

  /**@name Customisation call backs */
  //@{
    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeConnection(). If not then it
       attempts to use the token as a OpalCall token and find a connection
       of the same class.
      */
    PSafePtr<OpalLocalConnection> GetLocalConnectionWithLock(
      const PString & token,     ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite
    ) { return GetConnectionWithLockAs<OpalLocalConnection>(token, mode); }

    /**Create a connection for the PCSS endpoint.
       The default implementation is to create a OpalLocalConnection.
      */
    virtual OpalLocalConnection * CreateConnection(
      OpalCall & call,    ///<  Owner of connection
      void * userData,    ///<  Arbitrary data to pass to connection
      unsigned options,
      OpalConnection::StringOptions * stringOptions
    );

    /**Call back to indicate that remote is ringing.
       If false is returned the call is aborted.

       The default implementation does nothing.
      */
    virtual bool OnOutgoingCall(
      const OpalLocalConnection & connection ///<  Connection having event
    );

    /**Call back to indicate that remote is ringing.
       If false is returned the call is aborted.

       The default implementation answers the call immediately.
      */
    virtual bool OnIncomingCall(
      OpalLocalConnection & connection ///<  Connection having event
    );

    /**Indicate alerting for the incoming connection.
       Returns false if the connection token does not correspond to a valid
       connection.
      */
    virtual bool AlertingIncomingCall(
      const PString & token ///<  Token of connection to accept call
    );

    /**Accept the incoming connection.
       Returns false if the connection token does not correspond to a valid
       connection.
      */
    virtual bool AcceptIncomingCall(
      const PString & token ///<  Token of connection to accept call
    );

    /**Reject the incoming connection.
       Returns false if the connection token does not correspond to a valid
       connection.
      */
    virtual bool RejectIncomingCall(
      const PString & token ///<  Token of connection to accept call
    );

    /**Call back to indicate that the remote user has indicated something.
       If false is returned the call is aborted.

       The default implementation does nothing.
      */
    virtual bool OnUserInput(
      const OpalLocalConnection & connection, ///<  Connection having event
      const PString & indication
    );

    /**Call back to get media data for transmission.
       If false is returned then OnReadMediaData() is called.

       The default implementation returns false.
      */
    virtual bool OnReadMediaFrame(
      const OpalLocalConnection & connection, ///<  Connection for media
      const OpalMediaStream & mediaStream,    ///<  Media stream data is required for
      RTP_DataFrame & frame                   ///<  RTP frame for data
    );

    /**Call back to handle received media data.
       If false is returned then OnWriteMediaData() is called.

       The default implementation .
       The default implementation returns false.
      */
    virtual bool OnWriteMediaFrame(
      const OpalLocalConnection & connection, ///<  Connection for media
      const OpalMediaStream & mediaStream,    ///<  Media stream data is required for
      RTP_DataFrame & frame                   ///<  RTP frame for data
    );

    /**Call back to get media data for transmission.
       If false is returned the media stream will be closed.

       The default implementation returns false.
      */
    virtual bool OnReadMediaData(
      const OpalLocalConnection & connection, ///<  Connection for media
      const OpalMediaStream & mediaStream,    ///<  Media stream data is required for
      void * data,                            ///<  Data to send
      PINDEX size,                            ///<  Maximum size of data buffer
      PINDEX & length                         ///<  Number of bytes placed in buffer
    );

    /**Call back to handle received media data.
       If false is returned the media stream will be closed.

       The default implementation returns false.
      */
    virtual bool OnWriteMediaData(
      const OpalLocalConnection & connection, ///<  Connection for media
      const OpalMediaStream & mediaStream,    ///<  Media stream data is required for
      const void * data,                      ///<  Data received
      PINDEX length,                          ///<  Amount of data available to write
      PINDEX & written                        ///<  Amount of data written
    );

    /**Indicate that I/O is synchronous.
       This indicates that the OnReadMediaXXX and OnWriteMediaXXX functions
       will execute blocking to the correct real time synchonisation. So if
       for example OnWriteMediaData() is sent 320 bytes of PCM data then it
       will, on average, block for 20 milliseconds per call.

       If the function returns false, then the system will try and simulate
       the correct timing using the operating system sleep function. However
       this is not desirable as this function is notoriously inaccurate.

       Default returns true.
      */
    virtual bool IsSynchronous() const;

    /**Indicate OnAlerting() is be deferred or immediate.
      */
    bool IsDeferredAlerting() const { return m_deferredAlerting; }
  //@}

  protected:
    bool m_deferredAlerting;

  private:
    P_REMOVE_VIRTUAL(OpalLocalConnection *, CreateConnection(OpalCall &, void *), 0);
};


/** Local connection.
    This class represents a connection on the local machine that can receive
    media via virtual functions.
 */
class OpalLocalConnection : public OpalConnection
{
    PCLASSINFO(OpalLocalConnection, OpalConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    OpalLocalConnection(
      OpalCall & call,              ///<  Owner call for connection
      OpalLocalEndPoint & endpoint, ///<  Owner endpoint for connection
      void * userData,              ///<  Arbitrary data to pass to connection
      unsigned options,
      OpalConnection::StringOptions * stringOptions,
      char tokenPrefix = 'L'
    );

    /**Destroy connection.
     */
    ~OpalLocalConnection();
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
    virtual PBoolean IsNetworkConnection() const { return false; }

    /**Start an outgoing connection.
       This function will initiate the connection to the remote entity, for
       example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.

       The default behaviour does.
      */
    virtual PBoolean SetUpConnection();

    /**Indicate to remote endpoint an alert is in progress.
       If this is an incoming connection and the AnswerCallResponse is in a
       AnswerCallDeferred or AnswerCallPending state, then this function is
       used to indicate to that endpoint that an alert is in progress. This is
       usually due to another connection which is in the call (the B party)
       has received an OnAlerting() indicating that its remote endpoint is
       "ringing".

       The default behaviour does nothing.
      */
    virtual PBoolean SetAlerting(
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      PBoolean withMedia            ///<  Open media with alerting
    );

    /**Indicate to remote endpoint we are connected.

       The default behaviour sets the phase to ConnectedPhase, sets the
       connection start time and then checks if there is any media channels
       opened and if so, moves on to the established phase, calling
       OnEstablished().

       In other words, this method is used to handle incoming calls,
       and is an indication that we have accepted the incoming call.
      */
    virtual PBoolean SetConnected();

    /**Open a new media stream.
       This will create a media stream of an appropriate subclass as required
       by the underlying connection protocol. For instance H.323 would create
       an OpalRTPStream.

       The sessionID parameter may not be needed by a particular media stream
       and may be ignored. In the case of an OpalRTPStream it us used.

       Note that media streams may be created internally to the underlying
       protocol. This function is not the only way a stream can come into
       existance.

       The default behaviour is pure.
     */
    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      PBoolean isSource                    ///<  Is a source stream
    );

    /**Open source or sink media stream for session.
      */
    virtual OpalMediaStreamPtr OpenMediaStream(
      const OpalMediaFormat & mediaFormat, ///<  Media format to open
      unsigned sessionID,                  ///<  Session to start stream on
      bool isSource                        ///< Stream is a source/sink
    );

    /**Send a user input indication to the remote endpoint.
       This sends an arbitrary string as a user indication. If DTMF tones in
       particular are required to be sent then the SendIndicationTone()
       function should be used.

       The default behaviour plays the DTMF tones on the line.
      */
    virtual PBoolean SendUserInputString(
      const PString & value                   ///<  String value of indication
    );
  //@}

  /**@name New operations */
  //@{
    /**Indicate alerting for the incoming connection.
      */
    virtual void AlertingIncoming();

    /**Accept the incoming connection.
      */
    virtual void AcceptIncoming();
  //@}

  /**@name Member variable access */
  //@{
    /// Get user data pointer.
    void * GetUserData() const  { return userData; }

    /// Set user data pointer.
    void SetUserData(void * v)  { userData = v; }
  //@}

  protected:
    OpalLocalEndPoint & endpoint;
    void * userData;
};


/**Local media stream.
    This class represents a media stream on the local machine that can receive
    media via virtual functions.
  */
class OpalLocalMediaStream : public OpalMediaStream, public OpalMediaStreamPacing
{
    PCLASSINFO(OpalLocalMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for local system.
      */
    OpalLocalMediaStream(
      OpalLocalConnection & conn,          ///<  Connection for media stream
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      bool isSynchronous                   ///<  Can accept data and block accordingly
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Read an RTP frame of data from the source media stream.
       The default behaviour simply calls ReadData() on the data portion of the
       RTP_DataFrame and sets the frames timestamp and marker from the internal
       member variables of the media stream class.
      */
    virtual PBoolean ReadPacket(
      RTP_DataFrame & packet
    );

    /**Write an RTP frame of data to the sink media stream.
       The default behaviour simply calls WriteData() on the data portion of the
       RTP_DataFrame and and sets the internal timestamp and marker from the
       member variables of the media stream class.
      */
    virtual PBoolean WritePacket(
      RTP_DataFrame & packet
    );

    /**Read raw media data from the source media stream.
       The default behaviour reads from the OpalLine object.
      */
    virtual PBoolean ReadData(
      BYTE * data,      ///<  Data buffer to read to
      PINDEX size,      ///<  Size of buffer
      PINDEX & length   ///<  Length of data actually read
    );

    /**Write raw media data to the sink media stream.
       The default behaviour writes to the OpalLine object.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );

    /**Indicate if the media stream is synchronous.
       Returns PTrue for LID streams.
      */
    virtual PBoolean IsSynchronous() const;
  //@}

  protected:
    bool m_isSynchronous;
};


#endif // OPAL_OPAL_LOCALEP_H


// End of File ///////////////////////////////////////////////////////////////
