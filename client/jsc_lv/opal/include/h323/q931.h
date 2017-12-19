/*
 * q931.h
 *
 * Q.931 protocol handler
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
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23923 $
 * $Author: rjongbloed $
 * $Date: 2010-01-12 01:49:45 +0000 (Tue, 12 Jan 2010) $
 */

#ifndef OPAL_H323_Q931_H
#define OPAL_H323_Q931_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

///////////////////////////////////////////////////////////////////////////////

/**This class embodies a Q.931 Protocol Data Unit.
  */
class Q931 : public PObject
{
  PCLASSINFO(Q931, PObject)
  public:
    enum MsgTypes {
      NationalEscapeMsg  = 0x00,
      AlertingMsg        = 0x01,
      CallProceedingMsg  = 0x02,
      ConnectMsg         = 0x07,
      ConnectAckMsg      = 0x0f,
      ProgressMsg        = 0x03,
      SetupMsg           = 0x05,
      SetupAckMsg        = 0x0d,
      ResumeMsg          = 0x26,
      ResumeAckMsg       = 0x2e,
      ResumeRejectMsg    = 0x22,
      SuspendMsg         = 0x25,
      SuspendAckMsg      = 0x2d,
      SuspendRejectMsg   = 0x21,
      UserInformationMsg = 0x20,
      DisconnectMsg      = 0x45,
      ReleaseMsg         = 0x4d,
      ReleaseCompleteMsg = 0x5a,
      RestartMsg         = 0x46,
      RestartAckMsg      = 0x4e,
      SegmentMsg         = 0x60,
      CongestionCtrlMsg  = 0x79,
      InformationMsg     = 0x7b,
      NotifyMsg          = 0x6e,
      StatusMsg          = 0x7d,
      StatusEnquiryMsg   = 0x75,
      FacilityMsg        = 0x62
    };

    Q931();
    Q931(const Q931 & other);
    Q931 & operator=(const Q931 & other);

    void BuildFacility(int callRef, PBoolean fromDest);
    void BuildInformation(int callRef, PBoolean fromDest);
    void BuildProgress(
      int callRef,
      PBoolean fromDest,
      unsigned description,
      unsigned codingStandard = 0,
      unsigned location = 0
    );
    void BuildNotify(int callRef, PBoolean fromDest);
    void BuildCallProceeding(int callRef);
    void BuildSetupAcknowledge(int callRef);
    void BuildAlerting(int callRef);
    void BuildSetup(int callRef = -1);
    void BuildConnect(int callRef);
    void BuildStatus(int callRef, PBoolean fromDest);
    void BuildStatusEnquiry(int callRef, PBoolean fromDest);
    void BuildReleaseComplete(int callRef, PBoolean fromDest);

    PBoolean Decode(const PBYTEArray & data);
    PBoolean Encode(PBYTEArray & data) const;

    void PrintOn(ostream & strm) const;
    PString GetMessageTypeName() const;

    static unsigned GenerateCallReference();
    unsigned GetCallReference() const { return callReference; }
    PBoolean IsFromDestination() const { return fromDestination; }
    MsgTypes GetMessageType() const { return messageType; }

    enum InformationElementCodes {
      BearerCapabilityIE      = 0x04,
      CauseIE                 = 0x08,
      ChannelIdentificationIE = 0x18,
      FacilityIE              = 0x1c,
      ProgressIndicatorIE     = 0x1e,
      CallStateIE             = 0x14,
      DisplayIE               = 0x28,
      KeypadIE                = 0x2c,
      SignalIE                = 0x34,
      ConnectedNumberIE       = 0x4c,
      CallingPartyNumberIE    = 0x6c,
      CalledPartyNumberIE     = 0x70,
      RedirectingNumberIE     = 0x74,
      UserUserIE              = 0x7e
    };
    friend ostream & operator<<(ostream & strm, InformationElementCodes ie);

    PBoolean HasIE(InformationElementCodes ie) const;
    PBYTEArray GetIE(
      InformationElementCodes ie,
      PINDEX idx = 0 // Index of duplicate IE entry
    ) const;
    void SetIE(
      InformationElementCodes ie,
      const PBYTEArray & userData,
      bool append = FALSE
    );
    void RemoveIE(InformationElementCodes ie);

    enum InformationTransferCapability {
      TransferSpeech,
      TransferUnrestrictedDigital = 8,
      TransferRestrictedDigital = 9,
      Transfer3_1kHzAudio = 16,
      TrasnferUnrestrictedDigitalWithTones = 17,
      TransferVideo = 24
    };

    void SetBearerCapabilities(
      InformationTransferCapability capability,
      unsigned transferRate,        ///<  Number of 64k B channels
      unsigned codingStandard = 0,  ///<  0 = ITU-T standardized coding
      unsigned userInfoLayer1 = 5   ///<  5 = Recommendations H.221 and H.242
    );

    PBoolean GetBearerCapabilities(
      InformationTransferCapability & capability,
      unsigned & transferRate,        ///<  Number of 64k B channels
      unsigned * codingStandard = NULL,
      unsigned * userInfoLayer1 = NULL
    );

    enum CauseValues {
      UnknownCauseIE               =  0,
      UnallocatedNumber            =  1,
      NoRouteToNetwork             =  2,
      NoRouteToDestination         =  3,
      SendSpecialTone              =  4,
      MisdialledTrunkPrefix        =  5,
      ChannelUnacceptable          =  6,
      NormalCallClearing           = 16,
      UserBusy                     = 17,
      NoResponse                   = 18,
      NoAnswer                     = 19,
      SubscriberAbsent             = 20,
      CallRejected                 = 21,
      NumberChanged                = 22,
      Redirection                  = 23,
      ExchangeRoutingError         = 25,
      NonSelectedUserClearing      = 26,
      DestinationOutOfOrder        = 27,
      InvalidNumberFormat          = 28,
      FacilityRejected             = 29,
      StatusEnquiryResponse        = 30,
      NormalUnspecified            = 31,
      NoCircuitChannelAvailable    = 34,
      NetworkOutOfOrder            = 38,
      TemporaryFailure             = 41,
      Congestion                   = 42,
      RequestedCircuitNotAvailable = 44,
      ResourceUnavailable          = 47,
      ServiceOptionNotAvailable    = 63,
      InvalidCallReference         = 81,
      ClearedRequestedCallIdentity = 86,
      IncompatibleDestination      = 88,
      IENonExistantOrNotImplemented= 99,
      TimerExpiry                  = 102,
      ProtocolErrorUnspecified     = 111,
      InterworkingUnspecified      = 127,
      ErrorInCauseIE               = 0x100
    };
    friend ostream & operator<<(ostream & strm, CauseValues cause);

    void SetCause(
      CauseValues value,
      unsigned standard = 0,  ///<  0 = ITU-T standardized coding
      unsigned location = 0   ///<  0 = User
    );
    CauseValues GetCause(
      unsigned * standard = NULL,  ///<  0 = ITU-T standardized coding
      unsigned * location = NULL   ///<  0 = User
    ) const;

    enum CallStates {
      CallState_Null                  = 0,
      CallState_CallInitiated         = 1,
      CallState_OverlapSending        = 2,
      CallState_OutgoingCallProceeding= 3,
      CallState_CallDelivered         = 4,
      CallState_CallPresent           = 6,
      CallState_CallReceived          = 7,
      CallState_ConnectRequest        = 8,
      CallState_IncomingCallProceeding= 9,
      CallState_Active                = 10,
      CallState_DisconnectRequest     = 11,
      CallState_DisconnectIndication  = 12,
      CallState_SuspendRequest        = 15,
      CallState_ResumeRequest         = 17,
      CallState_ReleaseRequest        = 19,
      CallState_OverlapReceiving      = 25,
      CallState_ErrorInIE             = 0x100
    };
    void SetCallState(
      CallStates value,
      unsigned standard = 0  ///<  0 = ITU-T standardized coding
    );
    CallStates GetCallState(
      unsigned * standard = NULL  ///<  0 = ITU-T standardized coding
    ) const;

    enum SignalInfo {
      SignalDialToneOn,
      SignalRingBackToneOn,
      SignalInterceptToneOn,
      SignalNetworkCongestionToneOn,
      SignalBusyToneOn,
      SignalConfirmToneOn,
      SignalAnswerToneOn,
      SignalCallWaitingTone,
      SignalOffhookWarningTone,
      SignalPreemptionToneOn,
      SignalTonesOff = 0x3f,
      SignalAlertingPattern0 = 0x40,
      SignalAlertingPattern1,
      SignalAlertingPattern2,
      SignalAlertingPattern3,
      SignalAlertingPattern4,
      SignalAlertingPattern5,
      SignalAlertingPattern6,
      SignalAlertingPattern7,
      SignalAlertingOff = 0x4f,
      SignalErrorInIE = 0x100
    };
    void SetSignalInfo(SignalInfo value);
    SignalInfo GetSignalInfo() const;

    void SetKeypad(const PString & digits);
    PString GetKeypad() const;

    enum ProgressIndication {
       ProgressNotEndToEndISDN      = 1,      // Call is not end-to-end ISDN; 
                                              // further call progress information may be available in-band  
       ProgressDestinationNonISDN   = 2,      // Destination address is non ISDN  
       ProgressOriginNotISDN        = 3,      // Origination address is non ISDN  
       ProgressReturnedToISDN       = 4,      // Call has returned to the ISDN 
       ProgressServiceChange        = 5,      // Interworking has occurred and has 
                                              // resulted in a telecommunication service change
       ProgressInbandInformationAvailable = 8 // In-band information or an appropriate pattern is now available.   
    };

    void SetProgressIndicator(
      unsigned description,
      unsigned codingStandard = 0,
      unsigned location = 0
    );
    PBoolean GetProgressIndicator(
      unsigned & description,
      unsigned * codingStandard = NULL,
      unsigned * location = NULL
    ) const;

    void SetDisplayName(const PString & name);
    PString GetDisplayName() const;

    enum NumberingPlanCodes {
      UnknownPlan          = 0x00,
      ISDNPlan             = 0x01,
      DataPlan             = 0x03,
      TelexPlan            = 0x04,
      NationalStandardPlan = 0x08,
      PrivatePlan          = 0x09,
      ReservedPlan         = 0x0f
    };

    enum TypeOfNumberCodes {
      UnknownType          = 0x00,
      InternationalType    = 0x01,
      NationalType         = 0x02,
      NetworkSpecificType  = 0x03,
      SubscriberType       = 0x04,
      AbbreviatedType      = 0x06,
      ReservedType         = 0x07
    };

    void SetCallingPartyNumber(
      const PString & number, ///<  Number string
      unsigned plan = 1,      ///<  1 = ISDN/Telephony numbering system, see Q.931 Table 4-11 for more
      unsigned type = 0,      ///<  0 = Unknown number type
      int presentation = -1,  ///<  0 = presentation allowed, 1 = presentation restricted, -1 = no octet3a
      int screening = -1      ///<  0 = user provided, not screened, -1 = no octet3a
    );
    PBoolean GetCallingPartyNumber(
      PString & number,               ///<  Number string
      unsigned * plan = NULL,         ///<  ISDN/Telephony numbering system
      unsigned * type = NULL,         ///<  Number type
      unsigned * presentation = NULL, ///<  Presentation indicator
      unsigned * screening = NULL,    ///<  Screening indicator
      unsigned defPresentation = 0,   ///<  Default value if octet3a not present
      unsigned defScreening = 0       ///<  Default value if octet3a not present
    ) const;

    void SetCalledPartyNumber(
      const PString & number, ///<  Number string
      unsigned plan = 1,      ///<  1 = ISDN/Telephony numbering system
      unsigned type = 0       ///<  0 = Unknown number type
    );
    PBoolean GetCalledPartyNumber(
      PString & number,       ///<  Number string
      unsigned * plan = NULL, ///<  ISDN/Telephony numbering system
      unsigned * type = NULL  ///<  Number type
    ) const;

    void SetRedirectingNumber(
      const PString & number, ///<  Number string
      unsigned plan = 1,      ///<  1 = ISDN/Telephony numbering system
      unsigned type = 0,      ///<  0 = Unknown number type
      int presentation = -1,  ///<  0 = presentation allowed, -1 = no octet3a
      int screening = -1,     ///<  0 = user provided, not screened
      int reason = -1         ///<  0 = Unknown reason , -1 = no octet 3b
    );
    PBoolean GetRedirectingNumber(
      PString & number,               ///<  Number string
      unsigned * plan = NULL,         ///<  ISDN/Telephony numbering system
      unsigned * type = NULL,         ///<  Number type
      unsigned * presentation = NULL, ///<  Presentation indicator
      unsigned * screening = NULL,    ///<  Screening indicator
      unsigned * reason = NULL,       ///<  Reason for redirection
      unsigned defPresentation = 0,   ///<  Default value if octet3a not present
      unsigned defScreening = 0,      ///<  Default value if octet3a not present
      unsigned defReason =0           ///<  Default value if octet 3b not present
    ) const;

    void SetConnectedNumber(
      const PString & number, ///<  Number string
      unsigned plan = 1,      ///<  1 = ISDN/Telephony numbering system
      unsigned type = 0,      ///<  0 = Unknown number type
      int presentation = -1,  ///<  0 = presentation allowed, -1 = no octet3a
      int screening = -1,     ///<  0 = user provided, not screened
      int reason = -1         ///<  0 = Unknown reason , -1 = no octet 3b
    );
    PBoolean GetConnectedNumber(
      PString & number,               ///<  Number string
      unsigned * plan = NULL,         ///<  ISDN/Telephony numbering system
      unsigned * type = NULL,         ///<  Number type
      unsigned * presentation = NULL, ///<  Presentation indicator
      unsigned * screening = NULL,    ///<  Screening indicator
      unsigned * reason = NULL,       ///<  Reason for redirection
      unsigned defPresentation = 0,   ///<  Default value if octet3a not present
      unsigned defScreening = 0,      ///<  Default value if octet3a not present
      unsigned defReason =0           ///<  Default value if octet 3b not present
    ) const;

    /**Set the limitations to ChannelIdentification.
        - the interface identifier cannot be specified
        - channel in PRI can only be indicated by number and cannot be indicated by map
        - one and only one channel can be indicated
        - the coding standard is always ITU Q.931
      */
    void SetChannelIdentification(
      unsigned interfaceType = 0,        ///<   0 = basic,     1 = other (e.g. primary)
      unsigned preferredOrExclusive = 0, ///<   0 = preferred, 1 = exclusive
      int      channelNumber = 1         ///<  -1 = any,       0 = none/D, 1 = channel 1/B1, etc. 1-15,17-31
    );

    /**Get the limitations to ChannelIdentification.
      */
    PBoolean GetChannelIdentification(
      unsigned * interfaceType = NULL,        ///<  Interface type
      unsigned * preferredOrExclusive = NULL, ///<  Channel negotiation preference
      int      * channelNumber = NULL         ///<  Channel number
    ) const;

  protected:
    unsigned callReference;
    PBoolean fromDestination;
    unsigned protocolDiscriminator;
    MsgTypes messageType;

    PARRAY(InternalInformationElement, PBYTEArray);
    PDICTIONARY(InternalInformationElements, POrdinalKey, InternalInformationElement);
    InternalInformationElements informationElements;
};


#endif // OPAL_H323_Q931_H


/////////////////////////////////////////////////////////////////////////////
