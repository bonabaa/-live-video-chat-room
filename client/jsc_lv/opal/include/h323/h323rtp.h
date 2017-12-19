/*
 * h323rtp.h
 *
 * H.323 RTP protocol handler
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
 * $Revision: 21293 $
 * $Author: rjongbloed $
 * $Date: 2008-10-12 23:24:41 +0000 (Sun, 12 Oct 2008) $
 */

#ifndef OPAL_H323_H323RTP_H
#define OPAL_H323_H323RTP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_H323

#include <rtp/rtp.h>


class H225_RTPSession;

class H245_TransportAddress;
class H245_H2250LogicalChannelParameters;
class H245_H2250LogicalChannelAckParameters;

class H323Connection;
class H323_RTPChannel;


///////////////////////////////////////////////////////////////////////////////

/**This class is for encpsulating the IETF Real Time Protocol interface.
 */
class H323_RTP_Session : public RTP_UserData
{
  PCLASSINFO(H323_RTP_Session, RTP_UserData);

  /**@name Overrides from RTP_UserData */
  //@{
    /**Callback from the RTP session for transmit statistics monitoring.
       This is called every RTP_Session::senderReportInterval packets on the
       transmitter indicating that the statistics have been updated.

       The default behaviour calls H323Connection::OnRTPStatistics().
      */
    virtual void OnTxStatistics(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Callback from the RTP session for receive statistics monitoring.
       This is called every RTP_Session::receiverReportInterval packets on the
       receiver indicating that the statistics have been updated.

       The default behaviour calls H323Connection::OnRTPStatistics().
      */
    virtual void OnRxStatistics(
      const RTP_Session & session   ///<  Session with statistics
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Fill out the OpenLogicalChannel PDU for the particular channel type.
     */
    virtual PBoolean OnSendingPDU(
      const H323_RTPChannel & channel,            ///<  Channel using this session.
      H245_H2250LogicalChannelParameters & param  ///<  Open PDU to send.
    ) const = 0;

    /**This is called when request to create a channel is received from a
       remote machine and is about to be acknowledged.
     */
    virtual void OnSendingAckPDU(
      const H323_RTPChannel & channel,              ///<  Channel using this session.
      H245_H2250LogicalChannelAckParameters & param ///<  Acknowledgement PDU
    ) const = 0;

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.
     */
    virtual PBoolean OnReceivedPDU(
      H323_RTPChannel & channel,                  ///<  Channel using this session.
      const H245_H2250LogicalChannelParameters & param, ///<  Acknowledgement PDU
      unsigned & errorCode                              ///<  Error on failure
    ) = 0;

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.
     */
    virtual PBoolean OnReceivedAckPDU(
      H323_RTPChannel & channel,                  ///<  Channel using this session.
      const H245_H2250LogicalChannelAckParameters & param ///<  Acknowledgement PDU
    ) = 0;

    /**This is called when a gatekeeper wants to get status information from
       the endpoint.

       The default behaviour fills in the session ID's and SSRC parameters
       but does not do anything with the transport fields.
     */
    virtual void OnSendRasInfo(
      H225_RTPSession & info  ///<  RTP session info PDU
    ) = 0;
  //@}


  protected:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323_RTP_Session(
      const H323Connection & connection  ///<  Owner of the RTP session
    );
  //@}

    const H323Connection & connection; ///<  Owner of the RTP session
};


/**This class is for the IETF Real Time Protocol interface on UDP/IP.
 */
class H323_RTP_UDP : public H323_RTP_Session
{
  PCLASSINFO(H323_RTP_UDP, H323_RTP_Session);

  public:
  /**@name Construction */
  //@{
    /**Create a new RTP session H323 info.
     */
    H323_RTP_UDP(
      const H323Connection & connection, ///<  Owner of the RTP session
      RTP_UDP & rtp                      ///<  RTP session
    );
  //@}

  /**@name Operations */
  //@{
    /**Fill out the OpenLogicalChannel PDU for the particular channel type.
     */
    virtual PBoolean OnSendingPDU(
      const H323_RTPChannel & channel,            ///<  Channel using this session.
      H245_H2250LogicalChannelParameters & param  ///<  Open PDU to send.
    ) const;

    /**This is called when request to create a channel is received from a
       remote machine and is about to be acknowledged.
     */
    virtual void OnSendingAckPDU(
      const H323_RTPChannel & channel,              ///<  Channel using this session.
      H245_H2250LogicalChannelAckParameters & param ///<  Acknowledgement PDU
    ) const;

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default behaviour sets the remote ports to send UDP packets to.
     */
    virtual PBoolean OnReceivedPDU(
      H323_RTPChannel & channel,                  ///<  Channel using this session.
      const H245_H2250LogicalChannelParameters & param, ///<  Acknowledgement PDU
      unsigned & errorCode                              ///<  Error on failure
    );

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default behaviour sets the remote ports to send UDP packets to.
     */
    virtual PBoolean OnReceivedAckPDU(
      H323_RTPChannel & channel,                  ///<  Channel using this session.
      const H245_H2250LogicalChannelAckParameters & param ///<  Acknowledgement PDU
    );

    /**This is called when a gatekeeper wants to get status information from
       the endpoint.

       The default behaviour calls the ancestor functon and then fills in the 
       transport fields.
     */
    virtual void OnSendRasInfo(
      H225_RTPSession & info  ///<  RTP session info PDU
    );
  //@}

  protected:
    virtual PBoolean ExtractTransport(
      const H245_TransportAddress & pdu,
      PBoolean isDataPort,
      unsigned & errorCode
    );

    RTP_UDP & rtp;
};


#endif // OPAL_H323

#endif // OPAL_H323_H323RTP_H


/////////////////////////////////////////////////////////////////////////////
