/*
 * channels.h
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
 * $Revision: 23943 $
 * $Author: rjongbloed $
 * $Date: 2010-01-18 01:22:38 +0000 (Mon, 18 Jan 2010) $
 */

#ifndef OPAL_H323_CHANNELS_H
#define OPAL_H323_CHANNELS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_H323

#include <rtp/rtp.h>
#include <h323/transaddr.h>
#include <opal/mediastrm.h>


class H245_OpenLogicalChannel;
class H245_OpenLogicalChannelAck;
class H245_OpenLogicalChannel_forwardLogicalChannelParameters;
class H245_OpenLogicalChannel_reverseLogicalChannelParameters;
class H245_H2250LogicalChannelParameters;
class H245_H2250LogicalChannelAckParameters;
class H245_MiscellaneousCommand_type;
class H245_MiscellaneousIndication_type;

class H323EndPoint;
class H323Connection;
class H323Capability;
class H323_RTP_Session;


///////////////////////////////////////////////////////////////////////////////

/**Description of a Logical Channel Number.
   This is used as index into dictionary of logical channels.
 */
class H323ChannelNumber : public PObject
{
  PCLASSINFO(H323ChannelNumber, PObject);

  public:
    H323ChannelNumber() { number = 0; fromRemote = PFalse; }
    H323ChannelNumber(unsigned number, PBoolean fromRemote);

    virtual PObject * Clone() const;
    virtual PINDEX HashFunction() const;
    virtual void PrintOn(ostream & strm) const;
    virtual Comparison Compare(const PObject & obj) const;

    H323ChannelNumber & operator++(int);
    operator unsigned() const { return number; }
    PBoolean IsFromRemote() const { return fromRemote; }
    
  protected:
    unsigned number;
    PBoolean     fromRemote;
};


/**This class describes a logical channel between the two endpoints. They may
   be created and deleted as required in the H245 protocol.

   An application may create a descendent off this class and override
   functions as required for operating the channel protocol.
 */
class H323Channel : public PObject
{
  PCLASSINFO(H323Channel, PObject);

  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323Channel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability   ///<  Capability channel is using
    );

    /**Destroy new channel.
       To avoid usage of deleted objects in background threads, this waits
       for the H323LogicalChannelThread to terminate before continuing.
     */
    ~H323Channel();
  //@}

  /**@name Overrides from PObject */
  //@{
    virtual void PrintOn(
      ostream & strm
    ) const;
  //@}

  /**@name Operations */
  //@{
    enum Directions {
      IsBidirectional,
      IsTransmitter,
      IsReceiver,
      NumDirections
    };
#if PTRACING
    friend ostream & operator<<(ostream & out, Directions dir);
#endif

    /**Indicate the direction of the channel.
       Return if the channel is bidirectional, or unidirectional, and which
       direction for the latter case.
     */
    virtual Directions GetDirection() const = 0;

    /**Indicate the session number of the channel.
       Return session for channel. This is primarily for use by RTP based
       channels, for channels for which the concept of a session is not
       meaningfull, the default simply returns 0.
     */
    virtual unsigned GetSessionID() const;

    /**Set the session number of the channel.
       During OLC negotations teh master may change the session number being
       used for the logical channel.

       Returns false if the sesion could not be renumbered.
      */
    virtual bool SetSessionID(
      unsigned sessionID   ///< New session ID
    );

    /**Get the media transport address for the connection.
       This is primarily used to determine if media bypass is possible for the
       call between two connections.

       The default behaviour returns PFalse.
     */
    virtual PBoolean GetMediaTransportAddress(
      OpalTransportAddress & data,        ///<  Data channel address
      OpalTransportAddress & control      ///<  Control channel address
    ) const;

    /**Set the initial bandwidth for the channel.
       This calculates the initial bandwidth required by the channel and
       returns PTrue if the connection can support this bandwidth.

       The default behaviour gets the bandwidth requirement from the codec
       object created by the channel.
     */
    virtual PBoolean SetInitialBandwidth() = 0;

    /**Open the channel.
       The default behaviour just calls connection.OnStartLogicalChannel() and
       if successful sets the opened member variable.
      */
    virtual PBoolean Open();

    /**This is called when the channel can start transferring data.
     */
    virtual PBoolean Start() = 0;

    /**This is called to clean up any threads on connection termination.
     */
    virtual void Close();

    /**Indicate if has been opened.
     */
    PBoolean IsOpen() const { return opened && m_terminating == 0; }

    /**Get the media stream associated with this logical channel.

       If the argument is set to PTrue, the mediaStream is about to be deleted
       so all internal references to the mediaStream must be removed.

       The default behaviour returns NULL.
      */
    virtual OpalMediaStreamPtr GetMediaStream() const;


    /**Fill out the OpenLogicalChannel PDU for the particular channel type.
     */
    virtual PBoolean OnSendingPDU(
      H245_OpenLogicalChannel & openPDU  ///<  Open PDU to send. 
    ) const = 0;

    /**This is called when request to create a channel is received from a
       remote machine and is about to be acknowledged.

       The default behaviour does nothing.
     */
    virtual void OnSendOpenAck(
      const H245_OpenLogicalChannel & open,   ///<  Open PDU
      H245_OpenLogicalChannelAck & ack        ///<  Acknowledgement PDU
    ) const;

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default behaviour just returns PTrue.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_OpenLogicalChannel & pdu,    ///<  Open PDU
      unsigned & errorCode                    ///<  Error code on failure
    );

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default behaviour just returns PTrue.
     */
    virtual PBoolean OnReceivedAckPDU(
      const H245_OpenLogicalChannelAck & pdu  ///<  Acknowledgement PDU
    );

    /**Limit bit flow for the logical channel.
       The default behaviour passes this on to the codec if not NULL.
     */
    virtual void OnFlowControl(
      long bitRateRestriction   ///<  Bit rate limitation
    );

    /**Process a miscellaneous command on the logical channel.
       The default behaviour passes this on to the codec if not NULL.
     */
    virtual void OnMiscellaneousCommand(
      const H245_MiscellaneousCommand_type & type  ///<  Command to process
    );

    /**Process a miscellaneous indication on the logical channel.
       The default behaviour passes this on to the codec if not NULL.
     */
    virtual void OnMiscellaneousIndication(
      const H245_MiscellaneousIndication_type & type  ///<  Indication to process
    );

    /**Limit bit flow for the logical channel.
       The default behaviour does nothing.
     */
    virtual void OnJitterIndication(
      DWORD jitter,           ///<  Estimated received jitter in microseconds
      int skippedFrameCount,  ///<  Frames skipped by decodec
      int additionalBuffer    ///<  Additional size of video decoder buffer
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Get the number of the channel.
     */
    const H323ChannelNumber & GetNumber() const { return number; }

    /**Set the number of the channel.
     */
    void SetNumber(const H323ChannelNumber & num) { number = num; }

    /**Get the number of the reverse channel (if present).
     */
    const H323ChannelNumber & GetReverseChannel() const { return reverseChannel; }

    /**Set the number of the reverse channel (if present).
     */
    void SetReverseChannel(const H323ChannelNumber & num) { reverseChannel = num; }

    /**Get the bandwidth used by the channel in 100's of bits/sec.
     */
    unsigned GetBandwidthUsed() const { return bandwidthUsed; }

    /**Get the bandwidth used by the channel in 100's of bits/sec.
     */
    PBoolean SetBandwidthUsed(
      unsigned bandwidth  ///<  New bandwidth
    );

    /**Get the capability that created this channel.
      */
    const H323Capability & GetCapability() const { return *capability; }

    /**Get the "pause" flag.
       A paused channel is one that prevents the annunciation of the channels
       data. For example for audio this would mute the data, for video it would
       still frame.

       Note that channel is not stopped, and may continue to actually receive
       data, it is just that nothing is done with it.
      */
    PBoolean IsPaused() const { return paused; }

    /**Set the "pause" flag.
       A paused channel is one that prevents the annunciation of the channels
       data. For example for audio this would mute the data, for video it would
       still frame.

       Note that channel is not stopped, and may continue to actually receive
       data, it is just that nothing is done with it.
      */
    void SetPause(
      PBoolean pause   ///<  New pause flag
    ) { paused = pause; }
  //@}

    virtual void OnMediaCommand(OpalMediaCommand &) { }

  protected:
    virtual void InternalClose();

    H323EndPoint         & endpoint;
    H323Connection       & connection;
    H323Capability       * capability;
    H323ChannelNumber      number;
    H323ChannelNumber      reverseChannel;
    bool                   opened;
    bool                   paused;
    PAtomicInteger         m_terminating;

  private:
    unsigned bandwidthUsed;
};


PLIST(H323LogicalChannelList, H323Channel);



/**This class describes a unidirectional logical channel between the two
   endpoints. They may be created and deleted as required in the H245 protocol.

   An application may create a descendent off this class and override
   functions as required for operating the channel protocol.
 */
class H323UnidirectionalChannel : public H323Channel
{
  PCLASSINFO(H323UnidirectionalChannel, H323Channel);

  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323UnidirectionalChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction                ///<  Direction of channel
    );

    /**Destroy the channel, deleting the associated media stream.
      */
    ~H323UnidirectionalChannel();
  //@}

  /**@name Overrides from class H323Channel */
  //@{
    /**Indicate the direction of the channel.
       Return if the channel is bidirectional, or unidirectional, and which
       direction for th latter case.
     */
    virtual Directions GetDirection() const;

    /**Set the initial bandwidth for the channel.
       This calculates the initial bandwidth required by the channel and
       returns PTrue if the connection can support this bandwidth.

       The default behaviour gets the bandwidth requirement from the codec
       object created by the channel.
     */
    virtual PBoolean SetInitialBandwidth();

    /**Open the channel.
      */
    virtual PBoolean Open();

    /**This is called when the channel can start transferring data.
       The default action is to start one threads, with it either calling
       Receive() or Transmit() depending on the receiver member variable.
     */
    virtual PBoolean Start();

    /**Process a miscellaneous command on the logical channel.
       The default behaviour passes this on to the codec if not NULL.
     */
    virtual void OnMiscellaneousCommand(
      const H245_MiscellaneousCommand_type & type  ///<  Command to process
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Get the media stream associated with this logical channel.
       The default behaviour returns NULL.
      */
    virtual OpalMediaStreamPtr GetMediaStream() const;
  //@}

    void OnMediaCommand(OpalMediaCommand & command);

  protected:
    virtual void InternalClose();

    bool               receiver;
    OpalMediaStreamPtr mediaStream;
};


/**This class describes a bidirectional logical channel between the two
   endpoints. They may be created and deleted as required in the H245 protocol.

   An application may create a descendent off this class and override
   functions as required for operating the channel protocol.
 */
class H323BidirectionalChannel : public H323Channel
{
  PCLASSINFO(H323BidirectionalChannel, H323Channel);

  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323BidirectionalChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability   ///<  Capability channel is using
    );
  //@}

  /**@name Overrides from class H323Channel */
  //@{
    /**Indicate the direction of the channel.
       Return if the channel is bidirectional, or unidirectional, and which
       direction for th latter case.
     */
    virtual Directions GetDirection() const;

    /**This is called when the channel can start transferring data.
       The default action is to start two threads, one calls Receive() and the
       other calls Transmit().
     */
    virtual PBoolean Start();
  //@}
};


///////////////////////////////////////////////////////////////////////////////

/**This class is for encpsulating the IETF Real Time Protocol interface.
 */
class H323_RealTimeChannel : public H323UnidirectionalChannel
{
  PCLASSINFO(H323_RealTimeChannel, H323UnidirectionalChannel);

  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323_RealTimeChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction                ///<  Direction of channel
    );
  //@}

  /**@name Overrides from class H323Channel */
  //@{
    /**Fill out the OpenLogicalChannel PDU for the particular channel type.
     */
    virtual PBoolean OnSendingPDU(
      H245_OpenLogicalChannel & openPDU  ///<  Open PDU to send. 
    ) const;

    /**This is called when request to create a channel is received from a
       remote machine and is about to be acknowledged.
     */
    virtual void OnSendOpenAck(
      const H245_OpenLogicalChannel & open,   ///<  Open PDU
      H245_OpenLogicalChannelAck & ack        ///<  Acknowledgement PDU
    ) const;

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default makes sure the parameters are compatible and passes on
       the PDU to the rtp session.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_OpenLogicalChannel & pdu,    ///<  Open PDU
      unsigned & errorCode                    ///<  Error code on failure
    );

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default makes sure the parameters are compatible and passes on
       the PDU to the rtp session.
     */
    virtual PBoolean OnReceivedAckPDU(
      const H245_OpenLogicalChannelAck & pdu ///<  Acknowledgement PDU
    );
  //@}

  /**@name Operations */
  //@{
    /**Fill out the OpenLogicalChannel PDU for the particular channel type.
     */
    virtual PBoolean OnSendingPDU(
      H245_H2250LogicalChannelParameters & param  ///<  Open PDU to send.
    ) const;

    /**This is called when request to create a channel is received from a
       remote machine and is about to be acknowledged.
     */
    virtual void OnSendOpenAck(
      H245_H2250LogicalChannelAckParameters & param ///<  Acknowledgement PDU
    ) const;

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default behaviour sets the remote ports to send UDP packets to.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_H2250LogicalChannelParameters & param, ///<  Acknowledgement PDU
      unsigned & errorCode                              ///<  Error on failure
    );

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default behaviour sets the remote ports to send UDP packets to.
     */
    virtual PBoolean OnReceivedAckPDU(
      const H245_H2250LogicalChannelAckParameters & param ///<  Acknowledgement PDU
    );

    /**Set the dynamic payload type used by this channel.
      */
    virtual PBoolean SetDynamicRTPPayloadType(
      int newType  ///<  New RTP payload type number
    );

    RTP_DataFrame::PayloadTypes GetDynamicRTPPayloadType() const { return rtpPayloadType; }
  //@}

  protected:
    RTP_DataFrame::PayloadTypes rtpPayloadType;
};


///////////////////////////////////////////////////////////////////////////////

/**This class is for encpsulating the IETF Real Time Protocol interface.
 */
class H323_RTPChannel : public H323_RealTimeChannel
{
  PCLASSINFO(H323_RTPChannel, H323_RealTimeChannel);

  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323_RTPChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction,               /// Direction of channel
      RTP_Session & rtp                   /// RTP session for channel
    );

    /// Destroy the channel
    ~H323_RTPChannel();
  //@}

  /**@name Overrides from class H323Channel */
  //@{
    /**Indicate the session number of the channel.
       Return session for channel. This returns the session ID of the
       RTP_Session member variable.
     */
    virtual unsigned GetSessionID() const;

    /**Set the session number of the channel.
       During OLC negotations teh master may change the session number being
       used for the logical channel.

       Returns false if the sesion could not be renumbered.
      */
    virtual bool SetSessionID(
      unsigned sessionID   ///< New session ID
    );
  //@}

  /**@name Overrides from class H323_RealTimeChannel */
  //@{
    /**Fill out the OpenLogicalChannel PDU for the particular channel type.
     */
    virtual PBoolean OnSendingPDU(
      H245_H2250LogicalChannelParameters & param  ///<  Open PDU to send.
    ) const;

    /**This is called when request to create a channel is received from a
       remote machine and is about to be acknowledged.
     */
    virtual void OnSendOpenAck(
      H245_H2250LogicalChannelAckParameters & param ///<  Acknowledgement PDU
    ) const;

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default behaviour sets the remote ports to send UDP packets to.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_H2250LogicalChannelParameters & param, ///<  Acknowledgement PDU
      unsigned & errorCode                              ///<  Error on failure
    );

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default behaviour sets the remote ports to send UDP packets to.
     */
    virtual PBoolean OnReceivedAckPDU(
      const H245_H2250LogicalChannelAckParameters & param ///<  Acknowledgement PDU
    );
  //@}

  protected:
    RTP_Session      & rtpSession;
    H323_RTP_Session & rtpCallbacks;
};


///////////////////////////////////////////////////////////////////////////////

/**This class is for encpsulating the IETF Real Time Protocol interface as used
by a remote host.
 */
class H323_ExternalRTPChannel : public H323_RealTimeChannel
{
  PCLASSINFO(H323_ExternalRTPChannel, H323_RealTimeChannel);

  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323_ExternalRTPChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction,               ///<  Direction of channel
      unsigned sessionID                  ///<  Session ID for channel
    );
    /**Create a new channel.
     */
    H323_ExternalRTPChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction,               ///<  Direction of channel
      unsigned sessionID,                 ///<  Session ID for channel
      const H323TransportAddress & data,  ///<  Data address
      const H323TransportAddress & control///<  Control address
    );
    /**Create a new channel.
     */
    H323_ExternalRTPChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction,               ///<  Direction of channel
      unsigned sessionID,                 ///<  Session ID for channel
      const PIPSocket::Address & ip,      ///<  IP address of media server
      WORD dataPort                       ///<  Data port (control is dataPort+1)
    );
  //@}

  /**@name Overrides from class H323Channel */
  //@{
    /**Indicate the session number of the channel.
       Return session for channel. This returns the session ID of the
       RTP_Session member variable.
     */
    virtual unsigned GetSessionID() const;

    /**Get the media transport address for the connection.
       This is primarily used to determine if media bypass is possible for the
       call between two connections.

       The default behaviour returns PFalse.
     */
    virtual PBoolean GetMediaTransportAddress(
      OpalTransportAddress & data,        ///<  Data channel address
      OpalTransportAddress & control      ///<  Control channel address
    ) const;

    /**Start the channel.
      */
    virtual PBoolean Start();

    /**Handle channel data reception.

       This is called by the thread started by the Start() function and is
       typically a loop writing to the codec and reading from the transport
       (eg RTP_session).
      */
    virtual void Receive();

    /**Handle channel data transmission.

       This is called by the thread started by the Start() function and is
       typically a loop reading from the codec and writing to the transport
       (eg an RTP_session).
      */
    virtual void Transmit();
  //@}

  /**@name Overrides from class H323_RealTimeChannel */
  //@{
    /**Fill out the OpenLogicalChannel PDU for the particular channel type.
     */
    virtual PBoolean OnSendingPDU(
      H245_H2250LogicalChannelParameters & param  ///<  Open PDU to send.
    ) const;

    /**This is called when request to create a channel is received from a
       remote machine and is about to be acknowledged.
     */
    virtual void OnSendOpenAck(
      H245_H2250LogicalChannelAckParameters & param ///<  Acknowledgement PDU
    ) const;

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default behaviour sets the remote ports to send UDP packets to.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_H2250LogicalChannelParameters & param, ///<  Acknowledgement PDU
      unsigned & errorCode                              ///<  Error on failure
    );

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default behaviour sets the remote ports to send UDP packets to.
     */
    virtual PBoolean OnReceivedAckPDU(
      const H245_H2250LogicalChannelAckParameters & param ///<  Acknowledgement PDU
    );
  //@}

    void SetExternalAddress(
      const H323TransportAddress & data,  ///<  Data address
      const H323TransportAddress & control///<  Control address
    );

    const H323TransportAddress & GetRemoteMediaAddress()        const { return remoteMediaAddress; }
    const H323TransportAddress & GetRemoteMediaControlAddress() const { return remoteMediaControlAddress; }

    PBoolean GetRemoteAddress(
      PIPSocket::Address & ip,
      WORD & dataPort
    ) const;

  protected:
    void Construct(H323Connection & conn, unsigned id);

    unsigned             sessionID;
    H323TransportAddress externalMediaAddress;
    H323TransportAddress externalMediaControlAddress;
    H323TransportAddress remoteMediaAddress;
    H323TransportAddress remoteMediaControlAddress;
};


///////////////////////////////////////////////////////////////////////////////

/**This class describes a data logical channel between the two endpoints.
   They may be created and deleted as required in the H245 protocol.

   An application may create a descendent off this class and override
   functions as required for operating the channel protocol.
 */
class H323DataChannel : public H323UnidirectionalChannel
{
  PCLASSINFO(H323DataChannel, H323UnidirectionalChannel);

  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323DataChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction,               ///<  Direction of channel
      unsigned sessionID                  ///<  Session ID for channel
    );

    /**Destroy the channel.
      */
    ~H323DataChannel();
  //@}

  /**@name Overrides from class H323Channel */
  //@{
    /**Indicate the session number of the channel.
       Return session for channel. This returns the session ID of the
       RTP_Session member variable.
     */
    virtual unsigned GetSessionID() const;

    /**Fill out the OpenLogicalChannel PDU for the particular channel type.
     */
    virtual PBoolean OnSendingPDU(
      H245_OpenLogicalChannel & openPDU  ///<  Open PDU to send. 
    ) const;

    /**This is called when request to create a channel is received from a
       remote machine and is about to be acknowledged.
     */
    virtual void OnSendOpenAck(
      const H245_OpenLogicalChannel & open,   ///<  Open PDU
      H245_OpenLogicalChannelAck & ack        ///<  Acknowledgement PDU
    ) const;

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default makes sure the parameters are compatible and passes on
       the PDU to the rtp session.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_OpenLogicalChannel & pdu,    ///<  Open PDU
      unsigned & errorCode                    ///<  Error code on failure
    );

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default makes sure the parameters are compatible and passes on
       the PDU to the rtp session.
     */
    virtual PBoolean OnReceivedAckPDU(
      const H245_OpenLogicalChannelAck & pdu ///<  Acknowledgement PDU
    );
  //@}

  /**@name Operations */
  //@{
    /**Create the H323Listener class to be used.
       This is called on receipt of an OpenLogicalChannel request.

       The default behaviour creates a compatible listener using the
       connections control channel as a basis and returns PTrue if successful.
      */
    virtual PBoolean CreateListener();

    /**Create the H323Transport class to be used.
       This is called on receipt of an OpenLogicalChannelAck response. It
       should not return PTrue unless the transport member variable is set.

       The default behaviour uses the connection signalling channel to create
       the transport and returns PTrue if successful.
      */
    virtual PBoolean CreateTransport();
  //@}

  protected:
    virtual void InternalClose();

    unsigned        sessionID;
    H323Listener  * listener;
    PBoolean            autoDeleteListener;
    H323Transport * transport;
    PBoolean            autoDeleteTransport;
    PBoolean            separateReverseChannel;
};


#endif // OPAL_H323

#endif // OPAL_H323_CHANNELS_H


/////////////////////////////////////////////////////////////////////////////
