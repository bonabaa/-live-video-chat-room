/*
 * h323t38.h
 *
 * H.323 T.38 logical channel establishment
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
 * $Revision: 22645 $
 * $Author: rjongbloed $
 * $Date: 2009-05-19 04:02:17 +0000 (Tue, 19 May 2009) $
 */

#ifndef OPAL_T38_H323T38_H
#define OPAL_T38_H323T38_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_T38_CAPABILITY

#include <h323/h323caps.h>


class H245_T38FaxProfile;


///////////////////////////////////////////////////////////////////////////////

/**This class describes the T.38 standard capability.
 */
class H323_T38Capability : public H323DataCapability
{
    PCLASSINFO(H323_T38Capability, H323DataCapability);
  public:
  /**@name Construction */
  //@{
    enum TransportMode {
      e_UDP,
      e_DualTCP,
      e_SingleTCP,
      NumTransportModes
    };

    /**Create a new capability.
     */
    H323_T38Capability(
      TransportMode mode = e_UDP
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;

    /**Create a copy of the object.
      */
    virtual PObject * Clone() const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns the e_t38fax enum value from the protocol ASN
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

       The default behaviour sets tcp or udp as required.
     */
    virtual PBoolean OnSendingPDU(
      H245_DataProtocolCapability & proto,  ///<  PDU to set information on
      H245_T38FaxProfile & profile          ///<  PDU to set information on
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

    TransportMode GetTransportMode() const { return mode; }

  protected:
    TransportMode mode;
};


/**This class describes the T.38 non-standard capability.
 */
class H323_T38NonStandardCapability : public H323NonStandardDataCapability
{
    PCLASSINFO(H323_T38NonStandardCapability, H323NonStandardDataCapability);
  public:
  /**@name Construction */
  //@{
    /**Create a new capability.
     */
    H323_T38NonStandardCapability(
      BYTE country = 181,            ///<  t35 information
      BYTE extension = 0,            ///<  t35 information
      WORD maufacturer = 18          ///<  t35 information
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Create a copy of the object.
      */
    virtual PObject * Clone() const;
  //@}

  /**@name Identification functions */
  //@{
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
};


#endif //OPAL_T38_CAPABILITY

#endif // OPAL_T38_H323T38_H


/////////////////////////////////////////////////////////////////////////////
