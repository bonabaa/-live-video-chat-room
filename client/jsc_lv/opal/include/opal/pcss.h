/*
 * pcss.h
 *
 * PC Sound System support.
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
 * $Revision: 23741 $
 * $Author: rjongbloed $
 * $Date: 2009-11-02 02:27:28 +0000 (Mon, 02 Nov 2009) $
 */

#ifndef OPAL_OPAL_PCSS_H
#define OPAL_OPAL_PCSS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal/buildopts.h>

#if OPAL_HAS_PCSS

#if OPAL_PTLIB_AUDIO

#include <ptlib/sound.h>
#include <opal/localep.h>


class OpalPCSSConnection;


/** PC Sound System endpoint.
 */
class OpalPCSSEndPoint : public OpalEndPoint
{
    PCLASSINFO(OpalPCSSEndPoint, OpalEndPoint);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalPCSSEndPoint(
      OpalManager & manager,  ///<  Manager of all endpoints.
      const char * prefix = "pc" ///<  Prefix for URL style address strings
    );

    /**Destroy endpoint.
     */
    ~OpalPCSSEndPoint();
  //@}

  /**@name Overrides from OpalEndPoint */
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
      OpalCall & call,           ///<  Owner of connection
      const PString & party,     ///<  Remote party to call
      void * userData = NULL,    ///<  Arbitrary data to pass to connection
      unsigned int options = 0,  ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions  = NULL
    );
  //@}

  /**@name Overrides from OpalLocalEndPoint */
  //@{
    /**Call back to indicate that remote is ringing.
       If false is returned the call is aborted.

       The default implementation does nothing.
      */
    virtual bool OnOutgoingCall(
      const OpalConnection & connection ///<  Connection having event
    );

    /**Call back to indicate that remote is ringing.
       If false is returned the call is aborted.

       The default implementation answers the call immediately.
      */
    virtual bool OnIncomingCall(
      OpalConnection & connection ///<  Connection having event
    );

    /**Call back to indicate that the remote user has indicated something.
       If false is returned the call is aborted.

       The default implementation does nothing.
      */
    virtual bool OnUserInput(
      const OpalConnection & connection, ///<  Connection having event
      const PString & indication
    );
  //@}

  /**@name Customisation call backs */
  //@{
    /**Create a connection for the PCSS endpoint.
       The default implementation is to create a OpalPCSSConnection.
      */
    virtual OpalPCSSConnection * CreateConnection(
      OpalCall & call,    ///<  Owner of connection
      const PString & playDevice, ///<  Sound channel play device
      const PString & recordDevice, ///<  Sound channel record device
      void * userData,    ///<  Arbitrary data to pass to connection
      unsigned options,
      OpalConnection::StringOptions * stringOptions
    );

    /**Create an PSoundChannel based media stream.
      */
    virtual PSoundChannel * CreateSoundChannel(
      const OpalPCSSConnection & connection, ///<  Connection needing created sound channel
      const OpalMediaFormat & mediaFormat,   ///<  Media format for the connection
      PBoolean isSource                          ///<  Direction for channel
    );
  //@}

  /**@name User Interface operations */
  //@{
    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeConnection(). If not then it
       attempts to use the token as a OpalCall token and find a connection
       of the same class.
      */
    PSafePtr<OpalPCSSConnection> GetPCSSConnectionWithLock(
      const PString & token,     ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite
    ) { return GetConnectionWithLockAs<OpalPCSSConnection>(token, mode); }

    /**Call back to indicate that remote is ringing.
       If PFalse is returned the call is aborted.

       The default implementation is pure.
      */
    virtual PBoolean OnShowIncoming(
      const OpalPCSSConnection & connection ///<  Connection having event
    ) = 0;

    /**Accept the incoming connection.
       Returns PFalse if the connection token does not correspond to a valid
       connection.
      */
    virtual PBoolean AcceptIncomingConnection(
      const PString & connectionToken ///<  Token of connection to accept call
    );

    /**Reject the incoming connection.
       Returns PFalse if the connection token does not correspond to a valid
       connection.
      */
    virtual PBoolean RejectIncomingConnection(
      const PString & connectionToken ///<  Token of connection to accept call
    );

    /**Call back to indicate that remote is ringing.
       If PFalse is returned the call is aborted.

       The default implementation is pure.
      */
    virtual PBoolean OnShowOutgoing(
      const OpalPCSSConnection & connection ///<  Connection having event
    ) = 0;

    /**Call back to indicate that the remote user has indicated something.
       If PFalse is returned the call is aborted.

       The default implementation does nothing.
      */
    virtual PBoolean OnShowUserInput(
      const OpalPCSSConnection & connection, ///<  Connection having event
      const PString & indication
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Set the name for the sound channel to be used for output.
       If the name is not suitable for use with the PSoundChannel class then
       the function will return PFalse and not change the device.

       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    virtual PBoolean SetSoundChannelPlayDevice(const PString & name);

    /**Get the name for the sound channel to be used for output.
       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    const PString & GetSoundChannelPlayDevice() const { return soundChannelPlayDevice; }

    /**Set the name for the sound channel to be used for input.
       If the name is not suitable for use with the PSoundChannel class then
       the function will return PFalse and not change the device.

       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    virtual PBoolean SetSoundChannelRecordDevice(const PString & name);

    /**Get the name for the sound channel to be used for input.
       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    const PString & GetSoundChannelRecordDevice() const { return soundChannelRecordDevice; }

    /**Get default the sound channel buffer depth.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    unsigned GetSoundChannelBufferDepth() const { return soundChannelBuffers; }

    /**Set the default sound channel buffer depth.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    void SetSoundChannelBufferDepth(
      unsigned depth    ///<  New depth
    );

    /**Get default the sound channel buffer time in milliseconds.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    unsigned GetSoundChannelBufferTime() const { return m_soundChannelBufferTime; }

    /**Set the default sound channel buffer time in milliseconds.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    void SetSoundChannelBufferTime(
      unsigned depth    ///<  New depth in milliseconds
    );
  //@}
		 //   virtual OpalMediaFormatList GetMediaFormats() const;
		 OpalMediaFormatList GetMediaFormats() const;
	public:
    PString  soundChannelPlayDevice;
    PString  soundChannelRecordDevice;
    unsigned soundChannelBuffers;
    unsigned m_soundChannelBufferTime;

  private:
    P_REMOVE_VIRTUAL(OpalPCSSConnection *, CreateConnection(OpalCall &, const PString &, const PString &, void *), 0)
};


/** PC Sound System connection.
 */
class OpalPCSSConnection : public OpalConnection
{
    PCLASSINFO(OpalPCSSConnection, OpalConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalPCSSConnection(
      OpalCall & call,              ///<  Owner calll for connection
      OpalPCSSEndPoint & endpoint,  ///<  Owner endpoint for connection
      const PString & playDevice,   ///<  Sound channel play device
      const PString & recordDevice,  ///<  Sound channel record device
      unsigned options = 0,
      OpalConnection::StringOptions * stringOptions = NULL
    );
    virtual PBoolean IsNetworkConnection() const { return false; }

    /**Destroy endpoint.
     */
    ~OpalPCSSConnection();
		virtual PBoolean SetUpConnection();
		    virtual PBoolean SetAlerting(
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      PBoolean withMedia                ///<  Open media with alerting
    ) ;

  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Initiate the transfer of an existing call (connection) to a new remote 
       party.

       If remoteParty is a valid call token, then the remote party is transferred
       to that party (consultation transfer) and both calls are cleared.
     */
    virtual bool TransferConnection(
      const PString & remoteParty   ///<  Remote party to transfer the existing call to
    );

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

    /**Set  the volume (gain) for the audio media channel to the specified percentage.
      */
    virtual PBoolean SetAudioVolume(
      PBoolean source,                  ///< true for source (microphone), false for sink (speaker)
      unsigned percentage           ///< Gain, 0=silent, 100=maximun
    );

    /**Get the average signal level (0..32767) for the audio media channel.
       A return value of UINT_MAX indicates no valid signal, eg no audio channel opened.
      */
    virtual unsigned GetAudioSignalLevel(
      PBoolean source                   ///< true for source (microphone), false for sink (speaker)
    );
  //@}

  /**@name New operations */
  //@{
    /**Create an PSoundChannel based media stream.
      */
    virtual PSoundChannel * CreateSoundChannel(
      const OpalMediaFormat & mediaFormat, ///<  Media format for the connection
      PBoolean isSource                        ///<  Direction for channel
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Get the name for the sound channel to be used for output.
       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    const PString & GetSoundChannelPlayDevice() const { return soundChannelPlayDevice; }

    /**Get the name for the sound channel to be used for input.
       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    const PString & GetSoundChannelRecordDevice() const { return soundChannelRecordDevice; }

    /**Get default the sound channel buffer depth.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    unsigned GetSoundChannelBufferDepth() const { return soundChannelBuffers; }

    /**Get default the sound channel buffer time in milliseconds.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    unsigned GetSoundChannelBufferTime() const { return m_soundChannelBufferTime; }
  //@}

	void AcceptIncoming();

  protected:
    OpalPCSSEndPoint & endpoint;
    PString            soundChannelPlayDevice;
    PString            soundChannelRecordDevice;
    unsigned           soundChannelBuffers;
    unsigned           m_soundChannelBufferTime;
};

#else

#ifdef _MSC_VER
#pragma message("PTLib soundcard support not available")
#else
#warning "PTLib soundcard support not available"
#endif

#endif // OPAL_PTLIB_AUDIO

#endif // OPAL_HAS_PCSS

#endif // OPAL_OPAL_PCSS_H


// End of File ///////////////////////////////////////////////////////////////
