/*
 * h323caps.h
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
 * $Revision: 23115 $
 * $Author: rjongbloed $
 * $Date: 2009-07-17 13:13:11 +0000 (Fri, 17 Jul 2009) $
 */

#ifndef OPAL_H323_H323CAPS_H
#define OPAL_H323_H323CAPS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_H323

#include <opal/mediafmt.h>
#include <h323/channels.h>


/* The following classes have forward references to avoid including the VERY
   large header files for H225 and H245. If an application requires access
   to the protocol classes they can include them, but for simple usage their
   inclusion can be avoided.
 */
class PASN_Choice;
class H245_Capability;
class H245_DataType;
class H245_ModeElement;
class H245_AudioCapability;
class H245_AudioMode;
class H245_VideoCapability;
class H245_VideoMode;
class H245_DataApplicationCapability;
class H245_DataMode;
class H245_DataProtocolCapability;
class H245_H2250LogicalChannelParameters;
class H245_TerminalCapabilitySet;
class H245_NonStandardParameter;
class H323Connection;
class H323Capabilities;
class H245_CapabilityIdentifier;
class H245_GenericCapability;
class H245_GenericParameter;

///////////////////////////////////////////////////////////////////////////////

/**This class describes the interface to a capability of the endpoint, usually
   a codec, used to transfer data via the logical channels opened and managed
   by the H323 control channel.

   Note that this is not an instance of the codec itself. Merely the
   description of that codec. There is typically only one instance of this
   class contained in the capability tables of the endpoint. There may be
   several instances of the actualy codec managing the conversion of an
   individual stream of data.

   An application may create a descendent off this class and override
   functions as required for describing a codec that it implements.
 */
class H323Capability : public PObject
{
  PCLASSINFO(H323Capability, PObject);

  public:
  /**@name Construction */
  //@{
    /**Create a new capability specification.
     */
    H323Capability();
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;

    /**Print out the object to the stream, virtual version of << operator.
     */
    void PrintOn(ostream & strm) const;
  //@}

  /**@name Identification functions */
  //@{
    enum MainTypes {
      /// Audio codec capability
      e_Audio,
      /// Video codec capability
      e_Video,
      /// Arbitrary data capability
      e_Data,
      /// User Input capability
      e_UserInput,
      /// Generic Control
      e_GenericControl,
      /// Count of main types
      e_NumMainTypes
    };

    /**Get the main type of the capability.

       This function is overridden by one of the three main sub-classes off
       which real capabilities would be descendend.
     */
    virtual MainTypes GetMainType() const = 0;

    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.
     */
    virtual unsigned  GetSubType()  const = 0;

    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const = 0;
  //@}

  /**@name Operations */
  //@{
    /**Create an H323Capability descendant given a string name.
       This uses the registration system to create the capability.
      */
    static H323Capability * Create(
      const PString & name     ///<  Name of capability
    );

    enum {
      DefaultAudioSessionID = 1,
      DefaultVideoSessionID = 2,
      DefaultDataSessionID  = 3,
      DefaultH224SessionID  = 4
    };

    /**Get the default RTP session.
       This function gets the default RTP session ID for the capability
       type. For example audio capabilities return the value
       H323Capability::DefaultAudioSessionID etc.

       The default behaviour returns zero, indicating it is not an RTP
       based capability.
      */
    virtual unsigned GetDefaultSessionID() const;

    /**Set the maximum size (in frames) of data that will be transmitted in a
       single PDU.

       This will also be the desired number that will be sent by most codec
       implemetations.

       The default behaviour does nothing.
     */
    virtual void SetTxFramesInPacket(
      unsigned frames   ///<  Number of frames per packet
    );

    /**Get the maximum size (in frames) of data that will be transmitted in a single PDU.

       The default behaviour returns the value 1.
     */
    virtual unsigned GetTxFramesInPacket() const;

    /**Get the maximum size (in frames) of data that can be received in a single PDU.

       The default behaviour returns the value 1.
     */
    virtual unsigned GetRxFramesInPacket() const;

    /**Create the channel instance, allocating resources as required.
       This creates a logical channel object appropriate for the parameters
       provided. Not if param is NULL, sessionID must be provided, otherwise
       this is taken from the fields in param.
     */
    virtual H323Channel * CreateChannel(
      H323Connection & connection,    ///<  Owner connection for channel
      H323Channel::Directions dir,    ///<  Direction of channel
      unsigned sessionID,             ///<  Session ID for RTP channel
      const H245_H2250LogicalChannelParameters * param
                                      ///<  Parameters for channel
    ) const = 0;
  //@}

  /**@name Protocol manipulation */
  //@{
    enum CommandType {
      e_TCS,
      e_OLC,
      e_ReqMode
    };

    /**This function is called whenever and outgoing TerminalCapabilitySet
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual PBoolean OnSendingPDU(
      H245_Capability & pdu  ///<  PDU to set information on
    ) const = 0;

    /**This function is called whenever and outgoing OpenLogicalChannel
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual PBoolean OnSendingPDU(
      H245_DataType & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual PBoolean OnSendingPDU(
      H245_ModeElement & pdu  ///<  PDU to set information on
    ) const = 0;

    /**This function is called whenever and incoming TerminalCapabilitySet
       PDU is received on the control channel, and a new H323Capability
       descendent was created. This completes reading fields from the PDU
       into the classes members.

       If the function returns PFalse then the received PDU codec description
       is not supported, so will be ignored.
       
         The default behaviour sets the capabilityDirection member variable
         from the PDU and then returns PTrue. Note that this means it is very
         important to call the ancestor function when overriding.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_Capability & pdu ///<  PDU to get information from
    );

    /**This function is called whenever and incoming OpenLogicalChannel
       PDU has been used to construct the control channel. It allows the
       capability to set from the PDU fields, information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_DataType & pdu,  ///<  PDU to get information from
      PBoolean receiver               ///<  Is receiver OLC
    );

    /**Compare the PDU part of the capability.
      */
    virtual PBoolean IsMatch(
      const PASN_Choice & subTypePDU  ///<  sub-type PDU of H323Capability
    ) const;

    /**Validate that the capability is usable given the connection.
       This checks agains the negotiated protocol version number and remote
       application to determine if this capability should be used in TCS or
       OLC pdus.

       The default behaviour returns PTrue.
      */
    virtual PBoolean IsUsable(
      const H323Connection & connection
    ) const;
  //@}

  /**@name Member variable access */
  //@{
    enum CapabilityDirection {
      e_Unknown,
      e_Receive,
      e_Transmit,
      e_ReceiveAndTransmit,
      e_NoDirection,
      NumCapabilityDirections
    };

    /**Get the direction for this capability.
      */ 
    CapabilityDirection GetCapabilityDirection() const { return capabilityDirection; }

    /**Set the direction for this capability.
      */
    void SetCapabilityDirection(
      CapabilityDirection dir   ///<  New direction code
    ) { capabilityDirection = dir; }

    /// Get unique capability number.
    unsigned GetCapabilityNumber() const { return assignedCapabilityNumber; }

    /// Set unique capability number.
    void SetCapabilityNumber(unsigned num) { assignedCapabilityNumber = num; }

    /**Get media format of the media data this class represents.
      */
    OpalMediaFormat GetMediaFormat() const;

    /** Set media format option for the media data this class represents.
      */
    bool UpdateMediaFormat(
      const OpalMediaFormat & format
    );

    /// Get the payload type for the capaibility
    RTP_DataFrame::PayloadTypes GetPayloadType() const { return rtpPayloadType; }

    /// Set the payload type for the capaibility
    void SetPayloadType(RTP_DataFrame::PayloadTypes pt) { rtpPayloadType = pt; }

    /// Attach a QoS specification to this channel
    virtual void AttachQoS(RTP_QOS *) { }
  //@}

#if PTRACING
    friend ostream & operator<<(ostream & o , MainTypes t);
    friend ostream & operator<<(ostream & o , CapabilityDirection d);
#endif

  protected:
    OpalMediaFormat & GetWritableMediaFormat() const;

    unsigned            assignedCapabilityNumber;  /// Unique ID assigned to capability
    CapabilityDirection capabilityDirection;
    RTP_DataFrame::PayloadTypes rtpPayloadType;

  private:
    mutable OpalMediaFormat m_mediaFormat;

  friend class H323Capabilities;
};



/**This class describes the interface to a non-standard codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   It is expected that an application makes a descendent off
   H323NonStandardAudioCapability or H323NonStandardVideoCapability which
   multiply inherit from this class.
 */
class H323NonStandardCapabilityInfo
{
  public:
    typedef PObject::Comparison (*CompareFuncType)(struct PluginCodec_H323NonStandardCodecData *);

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardCapabilityInfo(
      CompareFuncType compareFunc,
      const BYTE * dataBlock,         ///< Non-Standard data for codec type
      PINDEX dataSize                 ///< Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
    );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardCapabilityInfo(
      const BYTE * dataBlock,         ///<  Non-Standard data for codec type
      PINDEX dataSize,                ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardCapabilityInfo(
      const PString & oid,
      const BYTE * dataBlock,         ///<  Non-Standard data for codec type
      PINDEX dataSize,                ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,
      PINDEX comparisonLength = P_MAX_INDEX
    );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardCapabilityInfo(
      BYTE country,                  ///<  t35 information
      BYTE extension,                ///<  t35 information
      WORD maufacturer,              ///<  t35 information
      const BYTE * dataBlock,         ///<  Non-Standard data for codec type
      PINDEX dataSize,                ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );

    /**Destroy the capability information
     */
    virtual ~H323NonStandardCapabilityInfo();

    /**This function gets the non-standard data field.

       The default behaviour sets data to fixedData.
      */
    virtual PBoolean OnSendingPDU(
      PBYTEArray & data  ///<  Data field in PDU to send
    ) const;

    /**This function validates and uses the non-standard data field.

       The default behaviour returns PTrue if data is equal to fixedData.
      */
    virtual PBoolean OnReceivedPDU(
      const PBYTEArray & data  ///<  Data field in PDU received
    );

    PBoolean IsMatch(const H245_NonStandardParameter & param) const;

    PObject::Comparison CompareParam(
      const H245_NonStandardParameter & param
    ) const;

  protected:
    PBoolean OnSendingNonStandardPDU(
      PASN_Choice & pdu,
      unsigned nonStandardTag
    ) const;
    PBoolean OnReceivedNonStandardPDU(
      const PASN_Choice & pdu,
      unsigned nonStandardTag
    );

    PObject::Comparison CompareInfo(
      const H323NonStandardCapabilityInfo & obj
    ) const;
    PObject::Comparison CompareData(
      const PBYTEArray & data  ///<  Data field in PDU received
    ) const;

    PString    oid;
    BYTE       t35CountryCode;
    BYTE       t35Extension;
    WORD       manufacturerCode;
    PBYTEArray nonStandardData;
    PINDEX     comparisonOffset;
    PINDEX     comparisonLength;
    CompareFuncType compareFunc;
};

/**This class describes the interface to a generic codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   It is expected that an application makes a descendent off
   H323GenericAudioCapability or H323GenericVideoCapability which
   multiply inherit from this class.
 */

class H323GenericCapabilityInfo
{
  public:
    H323GenericCapabilityInfo(
      const PString & id,     ///< generic codec identifier
      unsigned maxBitRate = 0   ///< maxBitRate parameter for the GenericCapability
    );

    virtual ~H323GenericCapabilityInfo() { }

  protected:
    virtual PBoolean OnSendingGenericPDU(
      H245_GenericCapability & pdu,
      const OpalMediaFormat & mediaFormat,
      H323Capability::CommandType type
    ) const;
    virtual PBoolean OnReceivedGenericPDU(
      OpalMediaFormat & mediaFormat,
      const H245_GenericCapability & pdu,
      H323Capability::CommandType type
    );

    PBoolean IsMatch(
      const H245_GenericCapability & param  ///< Non standard field in PDU received
    ) const;
    PObject::Comparison CompareInfo(
      const H323GenericCapabilityInfo & obj
    ) const;


    PString  m_identifier;
    unsigned maxBitRate;
};

/**This class describes the interface to a codec that has channels based on
   the RTP protocol.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323RealTimeCapability : public H323Capability
{
  PCLASSINFO(H323RealTimeCapability, H323Capability);

  public:
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

    H323RealTimeCapability();
    H323RealTimeCapability(const H323RealTimeCapability &rtc);
    virtual ~H323RealTimeCapability();
    void AttachQoS(RTP_QOS * _rtpqos);

  protected:
    RTP_QOS * rtpqos;
  //@}
};


/**This class describes the interface to an audio codec used to transfer data
   via the logical channels opened and managed by the H323 control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323AudioCapability : public H323RealTimeCapability
{
  PCLASSINFO(H323AudioCapability, H323RealTimeCapability);

  public:
  /**@name Construction */
  //@{
    /**Create an audio based capability.
      */
    H323AudioCapability();
  //@}

  /**@name Identification functions */
  //@{
    /**Get the main type of the capability.
       Always returns e_Audio.
     */
    virtual MainTypes GetMainType() const;
  //@}

  /**@name Operations */
  //@{
    /**Get the default RTP session.
       This function gets the default RTP session ID for the capability
       type. For example audio capabilities return the value
       RTP_Session::DefaultAudioSessionID etc.

       The default behaviour returns zero, indicating it is not an RTP
       based capability.
      */
    virtual unsigned GetDefaultSessionID() const;

    /**Set the maximum size (in frames) of data that will be transmitted in a
       single PDU.

       This will also be the desired number that will be sent by most codec
       implemetations.

       The default behaviour sets the txFramesInPacket variable.
     */
    virtual void SetTxFramesInPacket(
      unsigned frames   ///<  Number of frames per packet
    );

    /**Get the maximum size (in frames) of data that will be transmitted in a
       single PDU.

       The default behaviour sends the txFramesInPacket variable.
     */
    virtual unsigned GetTxFramesInPacket() const;

    /**Get the maximum size (in frames) of data that can be received in a
       single PDU.

       The default behaviour sends the rxFramesInPacket variable.
     */
    virtual unsigned GetRxFramesInPacket() const;
  //@}

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual PBoolean OnSendingPDU(
      H245_Capability & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing OpenLogicalChannel
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual PBoolean OnSendingPDU(
      H245_DataType & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual PBoolean OnSendingPDU(
      H245_ModeElement & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour assumes the pdu is an integer number of frames
       per packet.
     */
    virtual PBoolean OnSendingPDU(
      H245_AudioCapability & pdu,  ///<  PDU to set information on
      unsigned packetSize          ///<  Packet size to use in capability
    ) const;
    virtual PBoolean OnSendingPDU(
      H245_AudioCapability & pdu,  ///<  PDU to set information on
      unsigned packetSize,         ///<  Packet size to use in capability
      CommandType type             ///<  Type of PDU to send in
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour sets the PDUs tag according to the GetSubType()
       function (translated to different enum).
     */
    virtual PBoolean OnSendingPDU(
      H245_AudioMode & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       PDU is received on the control channel, and a new H323Capability
       descendent was created. This completes reading fields from the PDU
       into the classes members.

       If the function returns PFalse then the received PDU codec description
       is not supported, so will be ignored.
       
       The default behaviour calls the OnReceivedPDU() that takes a
       H245_AudioCapability and clamps the txFramesInPacket.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_Capability & pdu  ///<  PDU to get information from
    );

    /**This function is called whenever and incoming OpenLogicalChannel
       PDU has been used to construct the control channel. It allows the
       capability to set from the PDU fields, information in members specific
       to the class.
       
       The default behaviour calls the OnReceivedPDU() that takes a
       H245_AudioCapability and clamps the txFramesInPacket or
       rxFramesInPacket.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_DataType & pdu,  ///<  PDU to get information from
      PBoolean receiver               ///<  Is receiver OLC
    );

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour assumes the pdu is an integer number of frames
       per packet.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_AudioCapability & pdu,  ///<  PDU to get information from
      unsigned & packetSize              ///<  Packet size to use in capability
    );
    virtual PBoolean OnReceivedPDU(
      const H245_AudioCapability & pdu,  ///< PDU to get information from
      unsigned & packetSize,             ///< Packet size to use in capability
      CommandType type                   ///<  Type of PDU to send in
    );
  //@}
};


/**This class describes the interface to a non-standard audio codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323NonStandardAudioCapability : public H323AudioCapability,
                                       public H323NonStandardCapabilityInfo
{
  PCLASSINFO(H323NonStandardAudioCapability, H323AudioCapability);

  public:
  /**@name Construction */
  //@{
    H323NonStandardAudioCapability(
      H323NonStandardCapabilityInfo::CompareFuncType compareFunc,
      const BYTE * dataBlock,         ///< Non-Standard data for codec type
      PINDEX dataSize                 ///< Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
    );
    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardAudioCapability(
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
     );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardAudioCapability(
      const PString & oid,            ///<  OID for indentification of codec
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardAudioCapability(
      BYTE country,                   ///<  t35 information
      BYTE extension,                 ///<  t35 information
      WORD maufacturer,               ///<  t35 information
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns H245_AudioCapability::e_nonStandard.
     */
    virtual unsigned GetSubType() const;
  //@}

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual PBoolean OnSendingPDU(
      H245_AudioCapability & pdu,  ///<  PDU to set information on
      unsigned packetSize          ///<  Packet size to use in capability
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual PBoolean OnSendingPDU(
      H245_AudioMode & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnReceivedPDU()
       to handle the provided PDU.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_AudioCapability & pdu,  ///<  PDU to get information from
      unsigned & packetSize              ///<  Packet size to use in capability
    );

    /**Compare the nonStandardData part of the capability, if applicable.
      */
    virtual PBoolean IsMatch(
      const PASN_Choice & subTypePDU  ///<  sub-type PDU of H323Capability
    ) const;
  //@}
};

/**This class describes the interface to a generic audio codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323GenericAudioCapability : public H323AudioCapability,
                                   public H323GenericCapabilityInfo
{
  PCLASSINFO(H323NonStandardAudioCapability, H323AudioCapability);

  public:
  /**@name Construction */
  //@{
    /**Create a new set of information about a non-standard codec.
      */
    H323GenericAudioCapability(
      const PString & capabilityId,    ///< generic codec identifier
      PINDEX maxBitRate = 0               ///< maxBitRate parameter for the GenericCapability
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns H245_AudioCapability::e_genericCapability.
     */
    virtual unsigned GetSubType() const;
  //@}

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour calls H323GenericCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual PBoolean OnSendingPDU(
      H245_AudioCapability & pdu,  ///<  PDU to set information on
      unsigned packetSize,         ///<  Packet size to use in capability
      CommandType type             ///<  Type of PDU to send in
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour sets the PDUs tag according to the GetSubType()
       function (translated to different enum).
     */
    virtual PBoolean OnSendingPDU(
      H245_AudioMode & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour calls H323GenericCapabilityinfo::OnReceivedPDU()
       to handle the provided PDU.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_AudioCapability & pdu,  ///< PDU to get information from
      unsigned & packetSize,             ///< Packet size to use in capability
      CommandType type                   ///<  Type of PDU to send in
    );

    /**Compare the generic part of the capability, if applicable.
     */
    virtual PBoolean IsMatch(
      const PASN_Choice & subTypePDU  ///<  sub-type PDU of H323Capability
    ) const;
  //@}
};


#if OPAL_VIDEO

/**This class describes the interface to a video codec used to transfer data
   via the logical channels opened and managed by the H323 control channel.

   An application may create a descendent off this class and override
   functions as required for descibing a codec.
 */
class H323VideoCapability : public H323RealTimeCapability
{
  PCLASSINFO(H323VideoCapability, H323RealTimeCapability);

  public:
  /**@name Identification functions */
  //@{
    /**Get the main type of the capability.
       Always returns e_Video.
     */
    virtual MainTypes GetMainType() const;
  //@}

  /**@name Operations */
  //@{
    /**Get the default RTP session.
       This function gets the default RTP session ID for the capability
       type. For example audio capabilities return the value
       RTP_Session::DefaultAudioSessionID etc.

       The default behaviour returns zero, indicating it is not an RTP
       based capability.
      */
    virtual unsigned GetDefaultSessionID() const;
  //@}

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual PBoolean OnSendingPDU(
      H245_Capability & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing OpenLogicalChannel
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual PBoolean OnSendingPDU(
      H245_DataType & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual PBoolean OnSendingPDU(
      H245_ModeElement & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour is pure.
     */
    virtual PBoolean OnSendingPDU(
      H245_VideoCapability & pdu  ///<  PDU to set information on
    ) const;
    virtual PBoolean OnSendingPDU(
      H245_VideoCapability & pdu,  ///<  PDU to set information on
      CommandType type             ///<  Type of PDU to send in
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour sets the PDUs tag according to the GetSubType()
       function (translated to different enum).
     */
    virtual PBoolean OnSendingPDU(
      H245_VideoMode & pdu  ///<  PDU to set information on
    ) const = 0;

    /**This function is called whenever and incoming TerminalCapabilitySet
       PDU is received on the control channel, and a new H323Capability
       descendent was created. This completes reading fields from the PDU
       into the classes members.

       If the function returns PFalse then the received PDU codec description
       is not supported, so will be ignored. The default behaviour simply
       returns PTrue.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_Capability & pdu  ///<  PDU to get information from
    );

    /**This function is called whenever and incoming OpenLogicalChannel
       PDU has been used to construct the control channel. It allows the
       capability to set from the PDU fields, information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_DataType & pdu,  ///<  PDU to get information from
      PBoolean receiver               ///<  Is receiver OLC
    );

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour is pure.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_VideoCapability & pdu  ///<  PDU to set information on
    );
    virtual PBoolean OnReceivedPDU(
      const H245_VideoCapability & pdu,  ///< PDU to get information from
      CommandType type                   ///<  Type of PDU to send in
    );
  //@}
};


/**This class describes the interface to a non-standard video codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323NonStandardVideoCapability : public H323VideoCapability,
                                       public H323NonStandardCapabilityInfo
{
  PCLASSINFO(H323NonStandardVideoCapability, H323VideoCapability);

  public:
  /**@name Construction */
  //@{
    H323NonStandardVideoCapability(
      H323NonStandardCapabilityInfo::CompareFuncType compareFunc,
      const BYTE * dataBlock,         ///< Non-Standard data for codec type
      PINDEX dataSize                 ///< Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
    );
    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardVideoCapability(
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardVideoCapability(
      const PString & oid,            ///<  OID for indentification of codec
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardVideoCapability(
      BYTE country,                   ///<  t35 information
      BYTE extension,                 ///<  t35 information
      WORD maufacturer,               ///<  t35 information
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns one of the four possible combinations of mode and speed
       using the enum values of the protocol ASN H245_AudioCapability class.
     */
    virtual unsigned GetSubType() const;
  //@}

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual PBoolean OnSendingPDU(
      H245_VideoCapability & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual PBoolean OnSendingPDU(
      H245_VideoMode & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnReceivedPDU()
       to handle the provided PDU.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_VideoCapability & pdu  ///<  PDU to set information on
    );

    /**Compare the nonStandardData part of the capability, if applicable.
      */
    virtual PBoolean IsMatch(
      const PASN_Choice & subTypePDU  ///<  sub-type PDU of H323Capability
    ) const;
  //@}
};

/**This class describes the interface to a generic video codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323GenericVideoCapability : public H323VideoCapability,
                                   public H323GenericCapabilityInfo
{
  PCLASSINFO(H323GenericVideoCapability, H323VideoCapability);

  public:
  /**@name Construction */
  //@{
    /**Create a new set of information about a non-standard codec.
      */
    H323GenericVideoCapability(
      const PString & capabilityId,    ///< generic codec identifier (OID)
      PINDEX maxBitRate = 0           ///< maxBitRate parameter for the GenericCapability
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns H245_VideoCapability::e_genericCapability.
     */
    virtual unsigned GetSubType() const;
  //@}

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour calls H323GenericCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual PBoolean OnSendingPDU(
      H245_VideoCapability & pdu,  ///<  PDU to set information on
      CommandType type             ///<  Type of PDU to send in
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour sets the PDUs tag according to the GetSubType()
       function (translated to different enum).
     */
    virtual PBoolean OnSendingPDU(
      H245_VideoMode & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour calls H323GenericCapabilityinfo::OnReceivedPDU()
       to handle the provided PDU.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_VideoCapability & pdu,  ///< PDU to get information from
      CommandType type                   ///<  Type of PDU to send in
    );

    /**Compare the generic part of the capability, if applicable.
     */
    virtual PBoolean IsMatch(
      const PASN_Choice & subTypePDU  ///<  sub-type PDU of H323Capability
    ) const;
  //@}
};


#if OPAL_H239

/**This class descibes an extended ideo capability, as used in H.239.
  */
class H323ExtendedVideoCapability : public H323GenericVideoCapability
{
  PCLASSINFO(H323ExtendedVideoCapability, H323GenericVideoCapability);

  public:
  /**@name Construction */
  //@{
    /**Create a new Extended Video capability.
      */
    H323ExtendedVideoCapability(
      const PString & identifier   ///< generic codec identifier (OID)
    );
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns H245_VideoCapability::e_extendedVideoCapability.
     */
    virtual unsigned GetSubType() const;
  //@}
    
  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour calls H323GenericCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual PBoolean OnSendingPDU(
      H245_VideoCapability & pdu,  ///<  PDU to set information on
      CommandType type             ///<  Type of PDU to send in
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour sets the PDUs tag according to the GetSubType()
       function (translated to different enum).
     */
    virtual PBoolean OnSendingPDU(
      H245_VideoMode & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour calls H323GenericCapabilityinfo::OnReceivedPDU()
       to handle the provided PDU.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_VideoCapability & pdu,  ///< PDU to get information from
      CommandType type                   ///<  Type of PDU to send in
    );

    /**Compare the generic part of the capability, if applicable.
     */
    virtual PBoolean IsMatch(
      const PASN_Choice & subTypePDU  ///<  sub-type PDU of H323Capability
    ) const;
  //@}

  protected:
    OpalMediaFormatList m_videoFormats;
};


///////////////////////////////////////////////////////////////////////////////

/**This class describes a generic control capability, as used in H.239
 */
class H323GenericControlCapability : public H323Capability,
                                     public H323GenericCapabilityInfo
{
  PCLASSINFO(H323GenericControlCapability, H323Capability);

  public:
  /**@name Construction */
  //@{
    /**Create a new data capability.
      */
    H323GenericControlCapability(
      const PString & identifier  ///< Indentifer (OID) for generic control
    );
  //@}

  /**@name Identification functions */
  //@{
    /**Get the main type of the capability.
       Always returns e_Data.
     */
    virtual MainTypes GetMainType() const;

    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns H245_VideoCapability::e_extendedVideoCapability.
     */
    virtual unsigned GetSubType() const;
  //@}

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual PBoolean OnSendingPDU(
      H245_Capability & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual PBoolean OnSendingPDU(
      H245_ModeElement & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       PDU is received on the control channel, and a new H323Capability
       descendent was created. This completes reading fields from the PDU
       into the classes members.

       If the function returns PFalse then the received PDU codec description
       is not supported, so will be ignored. The default behaviour simply
       returns PTrue.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_Capability & pdu  ///<  PDU to get information from
    );

    /**Compare the generic part of the capability, if applicable.
     */
    virtual PBoolean IsMatch(
      const PASN_Choice & subTypePDU  ///<  sub-type PDU of H323Capability
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Create the channel instance, allocating resources as required.
       This creates a logical channel object appropriate for the parameters
       provided. Not if param is NULL, sessionID must be provided, otherwise
       this is taken from the fields in param.
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


///////////////////////////////////////////////////////////////////////////////

class H323H239VideoCapability : public H323ExtendedVideoCapability
{
  PCLASSINFO(H323H239VideoCapability, H323ExtendedVideoCapability);

  public:
  /**@name Construction */
  //@{
    /**Create a new Extended Video capability.
      */
    H323H239VideoCapability(
      const OpalMediaFormat & videoFormat
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
    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const;
  //@}

  /**@name Identification functions */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour calls H323GenericCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual PBoolean OnSendingPDU(
      H245_VideoCapability & pdu,  ///<  PDU to set information on
      CommandType type             ///<  Type of PDU to send in
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour calls H323GenericCapabilityinfo::OnReceivedPDU()
       to handle the provided PDU.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_VideoCapability & pdu,  ///< PDU to get information from
      CommandType type                   ///<  Type of PDU to send in
    );
  //@}
};


///////////////////////////////////////////////////////////////////////////////

class H323H239ControlCapability : public H323GenericControlCapability
{
  PCLASSINFO(H323H239ControlCapability, H323GenericControlCapability);

  public:
  /**@name Construction */
  //@{
    /**Create a new Extended Video capability.
      */
    H323H239ControlCapability();
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
};


#endif  // OPAL_H239

#endif  // OPAL_VIDEO


/**This class describes the interface to a data channel used to transfer data
   via the logical channels opened and managed by the H323 control channel.

   An application may create a descendent off this class and override
   functions as required for descibing a codec.
 */
class H323DataCapability : public H323Capability
{
  PCLASSINFO(H323DataCapability, H323Capability);

  public:
  /**@name Construction */
  //@{
    /**Create a new data capability.
      */
    H323DataCapability(
      unsigned maxBitRate = 0  ///<  Maximum bit rate for data in 100's b/s
    );
  //@}

  /**@name Identification functions */
  //@{
    /**Get the main type of the capability.
       Always returns e_Data.
     */
    virtual MainTypes GetMainType() const;
  //@}

  /**@name Operations */
  //@{
    /**Get the default RTP session.
       This function gets the default RTP session ID for the capability
       type. For example audio capabilities return the value
       RTP_Session::DefaultAudioSessionID etc.

       The default behaviour returns 3, indicating a data session.
      */
    virtual unsigned GetDefaultSessionID() const;
  //@}

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual PBoolean OnSendingPDU(
      H245_Capability & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing OpenLogicalChannel
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual PBoolean OnSendingPDU(
      H245_DataType & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual PBoolean OnSendingPDU(
      H245_ModeElement & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour is pure.
     */
    virtual PBoolean OnSendingPDU(
      H245_DataApplicationCapability & pdu  ///<  PDU to set information on
    ) const;
    virtual PBoolean OnSendingPDU(
      H245_DataApplicationCapability & pdu, ///<  PDU to set information on
      CommandType type                      ///<  Type of PDU to send in
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour sets the PDUs tag according to the GetSubType()
       function (translated to different enum).
     */
    virtual PBoolean OnSendingPDU(
      H245_DataMode & pdu  ///<  PDU to set information on
    ) const = 0;

    /**This function is called whenever and incoming TerminalCapabilitySet
       PDU is received on the control channel, and a new H323Capability
       descendent was created. This completes reading fields from the PDU
       into the classes members.

       If the function returns PFalse then the received PDU codec description
       is not supported, so will be ignored. The default behaviour simply
       returns PTrue.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_Capability & pdu  ///<  PDU to get information from
    );

    /**This function is called whenever and incoming OpenLogicalChannel
       PDU has been used to construct the control channel. It allows the
       capability to set from the PDU fields, information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_DataType & pdu,  ///<  PDU to get information from
      PBoolean receiver               ///<  Is receiver OLC
    );

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour is pure.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_DataApplicationCapability & pdu  ///<  PDU to set information on
    );
    virtual PBoolean OnReceivedPDU(
      const H245_DataApplicationCapability & pdu, ///<  PDU to set information on
      CommandType type                            ///<  Type of PDU to send in
    );
  //@}

  protected:
    unsigned maxBitRate;
};


/**This class describes the interface to a non-standard data codec used to
   transfer data via the logical channels opened and managed by the H323
   control channel.

   An application may create a descendent off this class and override
   functions as required for descibing the codec.
 */
class H323NonStandardDataCapability : public H323DataCapability,
                                      public H323NonStandardCapabilityInfo
{
  PCLASSINFO(H323NonStandardDataCapability, H323DataCapability);

  public:
  /**@name Construction */
  //@{
    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardDataCapability(
      unsigned maxBitRate,            ///<  Maximum bit rate for data in 100's b/s
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardDataCapability(
      unsigned maxBitRate,            ///<  Maximum bit rate for data in 100's b/s
      const PString & oid,            ///<  OID for indentification of codec
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );

    /**Create a new set of information about a non-standard codec.
      */
    H323NonStandardDataCapability(
      unsigned maxBitRate,            ///<  Maximum bit rate for data in 100's b/s
      BYTE country,                   ///<  t35 information
      BYTE extension,                 ///<  t35 information
      WORD maufacturer,               ///<  t35 information
      const BYTE * dataBlock = NULL,  ///<  Non-Standard data for codec type
      PINDEX dataSize = 0,            ///<  Size of dataBlock. If 0 and dataBlock != NULL use strlen(dataBlock)
      PINDEX comparisonOffset = 0,    ///<  Offset into dataBlock to compare
      PINDEX comparisonLength = P_MAX_INDEX  ///<  Length of bytes to compare
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns one of the four possible combinations of mode and speed
       using the enum values of the protocol ASN H245_AudioCapability class.
     */
    virtual unsigned GetSubType() const;
  //@}

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual PBoolean OnSendingPDU(
      H245_DataApplicationCapability & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnSendingPDU()
       to handle the PDU.
     */
    virtual PBoolean OnSendingPDU(
      H245_DataMode & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour calls H323NonStandardCapabilityinfo::OnReceivedPDU()
       to handle the provided PDU.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_DataApplicationCapability & pdu  ///<  PDU to set information on
    );

    /**Compare the nonStandardData part of the capability, if applicable.
      */
    virtual PBoolean IsMatch(
      const PASN_Choice & subTypePDU  ///<  sub-type PDU of H323Capability
    ) const;
  //@}
};


///////////////////////////////////////////////////////////////////////////////
// Known audio codecs

/**This class describes the G.711 codec capability.
 */
class H323_G711Capability : public H323AudioCapability
{
  PCLASSINFO(H323_G711Capability, H323AudioCapability)

  public:
    /// Specific G.711 encoding algorithm.
    enum Mode {
      /// European standard
      ALaw,
      /// American standard
      muLaw
    };
    /// Specific G.711 encoding bit rates.
    enum Speed {
      /// European standard
      At64k,
      /// American standard
      At56k
    };

  /**@name Construction */
  //@{
    /**Create a new G.711 capability.
     */
    H323_G711Capability(
      Mode mode = muLaw,    ///<  Type of encoding.
      Speed speed = At64k   ///<  Encoding bit rate.
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
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns one of the four possible combinations of mode and speed
       using the enum values of the protocol ASN H245_AudioCapability class.
     */
    virtual unsigned GetSubType() const;

    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const;
  //@}

  protected:
    Mode     mode;
    Speed    speed;
};

///////////////////////////////////////////////////////////////////////////////

/**This class describes the UserInput psuedo-channel.
 */
class H323_UserInputCapability : public H323Capability
{
  PCLASSINFO(H323_UserInputCapability, H323Capability);

  public:
  /**@name Construction */
  //@{
    enum SubTypes {
      BasicString,
      IA5String,
      GeneralString,
      SignalToneH245,
      HookFlashH245,
      SignalToneRFC2833,
      NumSubTypes
    };
    static const char * GetSubTypeName(SubTypes subType);
    friend ostream & operator<<(ostream & strm, SubTypes subType) { return strm << GetSubTypeName(subType); }

    /**Create the capability for User Input.
       The subType parameter is a value from the enum
       H245_UserInputCapability::Choices.
      */
    H323_UserInputCapability(
      SubTypes subType
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
    /**Get the main type of the capability.

       This function is overridden by one of the three main sub-classes off
       which real capabilities would be descendend.
     */
    virtual MainTypes GetMainType() const;

    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.
     */
    virtual unsigned  GetSubType()  const;

    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const;
  //@}

  /**@name Operations */
  //@{
    /**Create the channel instance, allocating resources as required.
       This creates a logical channel object appropriate for the parameters
       provided. Not if param is NULL, sessionID must be provided, otherwise
       this is taken from the fields in param.
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
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual PBoolean OnSendingPDU(
      H245_Capability & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing OpenLogicalChannel
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual PBoolean OnSendingPDU(
      H245_DataType & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    virtual PBoolean OnSendingPDU(
      H245_ModeElement & pdu  ///<  PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       PDU is received on the control channel, and a new H323Capability
       descendent was created. This completes reading fields from the PDU
       into the classes members.

       If the function returns PFalse then the received PDU codec description
       is not supported, so will be ignored. The default behaviour simply
       returns PTrue.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_Capability & pdu  ///<  PDU to get information from
    );

    /**This function is called whenever and incoming OpenLogicalChannel
       PDU has been used to construct the control channel. It allows the
       capability to set from the PDU fields, information in members specific
       to the class.

       The default behaviour is pure.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_DataType & pdu,  ///<  PDU to get information from
      PBoolean receiver               ///<  Is receiver OLC
    );

    /**Validate that the capability is usable given the connection.
       This checks agains the negotiated protocol version number and remote
       application to determine if this capability should be used in TCS or
       OLC pdus.

       The default behaviour will check for early versions and return PFalse
       for RFC2833 mode.
      */
    virtual PBoolean IsUsable(
      const H323Connection & connection
    ) const;
  //@}

    static void AddAllCapabilities(
      H323Capabilities & capabilities,        ///<  Table to add capabilities to
      PINDEX descriptorNum,   ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous     ///<  The member of the SimultaneousCapabilitySet to add
    );

  protected:
    SubTypes subType;
};



///////////////////////////////////////////////////////////////////////////////

PARRAY(H323CapabilitiesList, H323Capability);

PARRAY(H323CapabilitiesListArray, H323CapabilitiesList);

class H323SimultaneousCapabilities : public H323CapabilitiesListArray
{
  PCLASSINFO(H323SimultaneousCapabilities, H323CapabilitiesListArray);
  public:
    PBoolean SetSize(PINDEX newSize);
};


PARRAY(H323CapabilitiesSetArray, H323SimultaneousCapabilities);


class H323CapabilitiesSet : public H323CapabilitiesSetArray
{
  PCLASSINFO(H323CapabilitiesSet, H323CapabilitiesSetArray);
  public:
    /// Set the new size of the table, internal use only.
    PBoolean SetSize(PINDEX newSize);
};


/**This class contains all of the capabilities and their combinations.
  */
class H323Capabilities : public PObject
{
    PCLASSINFO(H323Capabilities, PObject);
  public:
  /**@name Construction */
  //@{
    /**Construct an empty capability set.
      */
    H323Capabilities();

    /**Construct a capability set from the H.245 PDU provided.
      */
    H323Capabilities(
      const H323Connection & connection,      ///<  Connection for capabilities
      const H245_TerminalCapabilitySet & pdu  ///<  PDU to convert to a capability set.
    );

    /**Construct a copy of a capability set.
       Note this will completely duplicate the set by making clones of every
       capability in the original set.
      */
    H323Capabilities(
      const H323Capabilities & original ///<  Original capabilities to duplicate
    );

    /**Assign a copy of a capability set.
       Note this will completely duplicate the set by making clones of every
       capability in the original set.
      */
    H323Capabilities & operator=(
      const H323Capabilities & original ///<  Original capabilities to duplicate
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Print out the object to the stream, virtual version of << operator.
     */
    void PrintOn(
      ostream & strm    ///<  Stream to print out to.
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Get the number of capabilities in the set.
      */
    PINDEX GetSize() const { return table.GetSize(); }

    /**Get the capability at the specified index.
      */
    H323Capability & operator[](PINDEX i) const { return table[i]; }

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

    /**Add matching capability to media format.
      */
    PINDEX AddMediaFormat(
      PINDEX descriptorNum,    ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,     ///<  The member of the SimultaneousCapabilitySet to add
      const OpalMediaFormat & mediaFormat ///<  Media format to add.
    );

    /**Add all matching capabilities to descriptor lists.
       All capabilities that match the specified name are added as in the other
       form of the SetCapability() function.
      */
    PINDEX AddAllCapabilities(
      PINDEX descriptorNum,    ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,     ///<  The member of the SimultaneousCapabilitySet to add
      const PString & name,    ///<  New capabilities name, if using "known" one.
      PBoolean exact = PFalse       ///<  Capability name must be exact match
    );

    // this function is retained for backwards compatibility
    PINDEX AddAllCapabilities(
      const H323EndPoint &,    ///<  The endpoint adding the capabilities.
      PINDEX descriptorNum,    ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,     ///<  The member of the SimultaneousCapabilitySet to add
      const PString & name,    ///<  New capabilities name, if using "known" one.
      PBoolean exact = PFalse       ///<  Capability name must be exact match
    )
    { return AddAllCapabilities(descriptorNum, simultaneous, name, exact); }

    /**Add a codec to the capabilities table. This will assure that the
       assignedCapabilityNumber field in the capability is unique for all
       capabilities installed on this set.

       If the specific instance of the capability is already in the table, it
       is not added again. Ther can be multiple instances of the same
       capability class however.
     */
    void Add(
      H323Capability * capability   ///<  New capability specification
    );

    /**Copy a codec to the capabilities table. This will make a clone of the
       capability and assure that the assignedCapabilityNumber field in the
       capability is unique for all capabilities installed on this set.

       Returns the copy that is put in the table.
     */
    H323Capability * Copy(
      const H323Capability & capability   ///<  New capability specification
    );

    /**Remove a capability from the table. Note that the the parameter must be
       the actual instance of the capability in the table. The instance is
       deleted when removed from the table.
      */
    void Remove(
      H323Capability * capability   ///<  Existing capability specification
    );

    /**Remove all capabilities matching the string. This uses FindCapability()
       to locate the first capability whose format name does a partial match
       for the argument.
      */
    void Remove(
      const PString & formatName   ///<  Format name to search for.
    );

    /**Remove all capabilities matching any of the strings provided. This
       simply calls Remove() for each string in the list.
      */
    void Remove(
      const PStringArray & formatNames  ///<  Array of format names to remove
    );

    /**Remove all of the capabilities.
      */
    void RemoveAll();

    /**Find the capability given the capability number. This number is
       guarenteed to be unique for a give capability table. Note that is may
       not be the same as the index into the table.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      unsigned capabilityNumber
    ) const;

    /**Find the capability given the capability format name string. This does
       a partial match for the supplied argument. If the argument matches a
       substring of the actual capabilities name, then it is returned. For
       example "GSM" or "0610" will match "GSM 0610". Note case is not
       significant.

       The user should be carefull of using short strings such as "G"!

       The direction parameter can further refine the search for specific
       receive or transmit capabilities. The default value of e_Unknown will
       wildcard that field.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      const PString & formatName, ///<  Wildcard format name to search for
      H323Capability::CapabilityDirection direction = H323Capability::e_Unknown,
            ///<  Optional direction to include into search criteria
      PBoolean exact = PFalse       ///<  Capability name must be exact match
    ) const;

    /**Find the first capability in the table of the specified direction.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      H323Capability::CapabilityDirection direction ///<  Direction to search for
    ) const;

    /**Find the capability given the capability. This does a value compare of
       the two capabilities. Usually this means the mainType and subType are
       the same.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      const H323Capability & capability ///<  Capability to search for
    ) const;

    /**Find the capability given the H.245 capability PDU.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      const H245_Capability & cap  ///<  H245 capability table entry
    ) const;

    /**Find the capability given the H.245 data type PDU.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      const H245_DataType & dataType  ///<  H245 data type of codec
    ) const;

    /**Find the capability given the H.245 data type PDU.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      const H245_ModeElement & modeElement  ///<  H245 data type of codec
    ) const;

    /**Find the capability given the type codecs.

       Returns:
       NULL if no capability meeting the criteria was found
      */
    H323Capability * FindCapability(
      H323Capability::MainTypes mainType, ///<  Main type to find
      unsigned subType = UINT_MAX         ///<  Sub-type to find (UINT_MAX=ignore)
    ) const;

    /**Build a H.245 PDU from the information in the capability set.
      */
    void BuildPDU(
      const H323Connection & connection,  ///<  Connection building PDU for
      H245_TerminalCapabilitySet & pdu    ///<  PDU to build
    ) const;

    /**Merge the capabilities into this set.
      */
    PBoolean Merge(
      const H323Capabilities & newCaps
    );

    /**Change the order of capabilities in the table to the order specified.
       Note that this does not change the unique capability numbers assigned
       when the capability is first added to the set.

       The string matching rules are as for the FindCapability() function.
      */
    void Reorder(
      const PStringArray & preferenceOrder  ///<  New order
    );

    /**Test if the capability is allowed.
      */
    PBoolean IsAllowed(
      const H323Capability & capability
    );

    /**Test if the capability is allowed.
      */
    PBoolean IsAllowed(
      unsigned capabilityNumber
    );

    /**Test if the capabilities are an allowed combination.
      */
    PBoolean IsAllowed(
      const H323Capability & capability1,
      const H323Capability & capability2
    );

    /**Test if the capabilities are an allowed combination.
      */
    PBoolean IsAllowed(
      unsigned capabilityNumber1,
      unsigned capabilityNumber2
    );

    /**Get the list of capabilities as a list of media formats.
      */
    OpalMediaFormatList GetMediaFormats() const;

    const PStringSet & GetMediaPacketizations() const { return mediaPacketizations; }
  //@}

  protected:
    void SetMediaPacketizations(const PStringSet & packetizations) { mediaPacketizations = packetizations; }
    
    H323CapabilitiesList table;
    H323CapabilitiesSet  set;
    PStringSet           mediaPacketizations;
};


///////////////////////////////////////////////////////////////////////////////

/* New capability registration macros based on abstract factories
 */
typedef PFactory<H323Capability, std::string> H323CapabilityFactory;

#define H323_REGISTER_CAPABILITY(cls, capName)   static H323CapabilityFactory::Worker<cls> cls##Factory(capName, true); \


#endif // OPAL_H323

#endif // OPAL_H323_H323CAPS_H


/////////////////////////////////////////////////////////////////////////////
