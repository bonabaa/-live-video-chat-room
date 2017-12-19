/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * List of the defines which enumerate the reason why calls end.
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
 * $Revision: 21293 $
 * $Author: rjongbloed $
 * $Date: 2008-10-12 23:24:41 +0000 (Sun, 12 Oct 2008) $
 */

#ifndef OPAL_IAX2_CAUSECODE_H
#define OPAL_IAX2_CAUSECODE_H

#include <opal/buildopts.h>

#if OPAL_IAX2

#include <iax2/ies.h>

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the hangup cause code*/
class IAX2IeCauseCode : public IAX2IeByte
{
  PCLASSINFO(IAX2IeCauseCode, IAX2IeByte);
 public:
  /**Enums to specify why this call failed */
  enum CauseCodes {
    Unallocated                      =   1,               /*!<    */
    NoRouteTransitNet                =   2,               /*!<    */
    NoRouteDestination               =   3,               /*!<    */
    ChannelUnacceptable              =   6,               /*!<    */
    CallAwardedDelivered             =   7,               /*!<    */
    NormalClearing                   =  16,               /*!<    */
    UserBusy                         =  17,               /*!<    */
    NoUserResponse                   =  18,               /*!<    */
    NoAnswer                         =  19,               /*!<    */
    CallRejected                     =  21,               /*!<    */
    NumberChanged                    =  22,               /*!<    */
    DestinationOutOfOrder            =  27,               /*!<    */
    InvalidNumberFormat              =  28,               /*!<    */
    FacilityRejected                 =  29,               /*!<    */
    ResponseToStatusEnquiry          =  30,               /*!<    */
    NormalUnspecified                =  31,               /*!<    */
    NormalCircuitCongestion          =  34,               /*!<    */
    NetworkOutOfOrder                =  38,               /*!<    */
    NormalTemporaryFailure           =  41,               /*!<    */
    SwitchCongestion                 =  42,               /*!<    */
    AccessInfoDiscarded              =  43,               /*!<    */
    RequestedChanUnavail             =  44,               /*!<    */
    PreEmpted                        =  45,               /*!<    */
    FacilityNotSubscribed            =  50,               /*!<    */
    OutgoingCallBarred               =  52,               /*!<    */
    IncomingCallBarred               =  54,               /*!<    */
    BearerCapabilityNotauth          =  57,               /*!<    */
    BearerCapabilityNotAvail         =  58,               /*!< No agreement  on a common codec to use. Happens at call setup. */
    BearerCapabilityNotimpl          =  65,               /*!<    */
    ChanNotImplemented               =  66,               /*!<    */
    FacilityNotImplemented           =  69,               /*!<    */
    InvalidCallReference             =  81,               /*!<    */
    IncompatibleDestination          =  88,               /*!<    */
    InvalidMsgUnspecified            =  95,               /*!<    */
    MandatoryIeMissing               =  96,               /*!<    */
    MessageTypeNonexist              =  97,               /*!<    */
    WrongMessage                     =  98,               /*!<    */
    IeNonexist                       =  99,               /*!<    */
    InvalidIeContents                = 100,               /*!<    */
    WrpngCallState                   = 101,               /*!<    */
    RecoveryOnTimerExpire            = 102,               /*!<    */
    MandatoryIeLengthError           = 103,               /*!<    */
    ProtocolError                    = 111,               /*!<    */
    Interworking                     = 127,               /*!<    */
              
    /* Special Asterisk aliases */
    Busy                              =  UserBusy,                         /*!<    */
    Failure                           =  NetworkOutOfOrder,                /*!<    */
    Normal                            =  NormalClearing,                   /*!<    */
    Congestion                        =  NormalCircuitCongestion,          /*!<    */
    Unregistered                      =  NoRouteDestination,               /*!<    */
    NotDefined                        =  0,                                /*!<    */
    NoSuchDriver                      =  ChanNotImplemented,               /*!<    */
  };

  /** Constructor from data read from the network.
	 
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeCauseCode(BYTE length, BYTE *srcData) : IAX2IeByte(length, srcData) { };
     
  /**Construct with a predefined value (Typically used prior to transmission)*/
  IAX2IeCauseCode(BYTE newValue) : IAX2IeByte(newValue) { }

  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const { return ie_causeCode; }

  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;

  /** Take the data from this Ie, and copy it into the IeData structure.
      This is done on processing an incoming frame which contains Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.causeCode = dataValue; }     

};

#endif // OPAL_IAX2

#endif // OPAL_IAX2_CAUSECODE_H

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */
