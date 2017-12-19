/*
 * h323t120.h
 *
 * H.323 T.120 logical channel establishment
 *
 * Open H323 Library
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21293 $
 * $Author: rjongbloed $
 * $Date: 2008-10-12 23:24:41 +0000 (Sun, 12 Oct 2008) $
 */

#ifndef OPAL_T120_H323T120_H
#define OPAL_T120_H323T120_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <h323/h323caps.h>


class OpalT120Protocol;


///////////////////////////////////////////////////////////////////////////////

/**This class describes the T.120 logical channel.
 */
class H323_T120Capability : public H323DataCapability
{
    PCLASSINFO(H323_T120Capability, H323DataCapability);
  public:
  /**@name Construction */
  //@{
    /**Create capability.
      */
    H323_T120Capability();
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Create a copy of the object.
      */
    virtual PObject * Clone() const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns the e_t120 enum value from the protocol ASN
       H245_DataApplicationCapability_application class.
     */
    virtual unsigned GetSubType() const;

    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const;
  //@}

  /**@name Operations */
  //@{
    /**Create the channel instance, allocating resources as required.
     */
    virtual H323Channel * CreateChannel(
      H323Connection & connection,    ///<  Owner connection for channel
      H323Channel::Directions dir,    ///<  Direction of channel
      unsigned sessionID,             ///<  Session ID for RTP channel
      const H245_H2250LogicalChannelParameters * param
                                      ///<  Parameters for channel
    ) const;
  //@}

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour sets the pdu and calls OnSendingPDU with a
       H245_DataProtocolCapability parameter.
     */
    virtual PBoolean OnSendingPDU(
      H245_DataApplicationCapability & pdu
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour sets the pdu and calls OnSendingPDU with a
       H245_DataProtocolCapability parameter.
     */
    virtual PBoolean OnSendingPDU(
      H245_DataMode & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing PDU is being constructed
       for the control channel. It allows the capability to set the PDU fields
       from information in members specific to the class.

       The default behaviour sets separate LAN stack.
     */
    virtual PBoolean OnSendingPDU(
      H245_DataProtocolCapability & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour gets the data rate field from the PDU.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_DataApplicationCapability & pdu  ///<  PDU to set information on
    );
  //@}

  /**@name Member access */
  //@{
    /**Get the dynamic port capability.
       Indicates endpoint can use something other than port 1503.
      */
    PBoolean GetDynamicPortCapability() const { return dynamicPortCapability; }

    /**Set the dynamic port capability.
       Indicates endpoint can use something other than port 1503.
      */
    void SetDynamicPortCapability(PBoolean dynamic) { dynamicPortCapability = dynamic; }
  //@}

  protected:
    PBoolean dynamicPortCapability;
};


/**This class describes the T.120 logical channel.
 */
class H323_T120Channel : public H323DataChannel
{
    PCLASSINFO(H323_T120Channel, H323DataChannel);
  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323_T120Channel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction,               ///<  Direction of channel
      unsigned sessionID                  ///<  Session ID for channel
    );
  //@}

  /**@name Overrides from class H323Channel */
  //@{
    /**Handle channel data reception.

       This is called by the thread started by the Start() function and is
       typically a loop reading  from the transport and handling PDU's.

       The default behaviour here is to call HandleChannel()
      */
    virtual void Receive();

    /**Handle channel data transmission.

       This is called by the thread started by the Start() function and is
       typically a loop reading from the codec and writing to the transport
       (eg an RTP_session).

       The default behaviour here is to call HandleChannel()
      */
    virtual void Transmit();

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

    virtual void HandleChannel();

  protected:
    OpalT120Protocol * t120handler;
};


#endif // OPAL_T120_H323T120_H


/////////////////////////////////////////////////////////////////////////////
