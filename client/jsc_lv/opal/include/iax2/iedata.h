/*
 *
 *
 * Inter Asterisk Exchange 2
 * 
 * defines the different types of information elements
 * 
 * Open Phone Abstraction Library (OPAL)
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

#ifndef OPAL_IAX2_IEDATA_H
#define OPAL_IAX2_IEDATA_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if OPAL_IAX2

#include <ptlib/sockets.h>

#ifdef P_USE_PRAGMA
#pragma interface
#endif

/**This class is used to contain the data read from the different ie fields.
   

     This class is not thread safe.
*/
class IAX2IeData :  public PObject
{
  PCLASSINFO(IAX2IeData, PObject);
 public:
  /**@name Construction/Destruction */
  //@{
  /**Construct IeData class
   */
  IAX2IeData();
     
     
  ~IAX2IeData();
  //@}

  /**Pretty print the varaibles in this class to the designated stream*/
  virtual void PrintOn(ostream & strm) const;


  PString            calledNumber;          /*!< Number/extension being called - string */
  PString            callingNumber;         /*!< Calling number - string    */
  PString            callingAni;            /*!< Calling number ANI for billing  - string    */
  PString            callingName;           /*!< Name of caller - string    */
  int                callingTon;            /*!< calling typeofnum    */
  int                callingTns;            /*!< calling transitnet    */
  int                callingPres;           /*!< calling presntn    */
  PString            calledContext;         /*!< Context for number - string    */
  PString            userName;              /*!< Username (peer or user) for authentication - string    */
  PString            password;              /*!< Password for authentication - string    */
  unsigned int       capability;            /*!< Actual codec capability - unsigned int    */
  unsigned int       format;                /*!< Desired codec format - unsigned int    */
  PString            codecPrefs;            /*!< codec_prefs    */
  PString            language;              /*!< Desired language - string    */
  int                version;               /*!< Protocol version - short    */
  PINDEX             adsicpe;               /*!< CPE ADSI capability - short    */
  PString            dnid;                  /*!< Originally dialed DNID - string    */
  PString            rdnis;                 /*!< Referring DNIS -- string    */
  short              authMethods;           /*!< Authentication method(s) - short    */
  unsigned int       encryptionMethods;     /*!< encryption method to us    */
  PString            challenge;             /*!< Challenge data for MD5/RSA - string    */
  PString            md5Result;             /*!< MD5 challenge result - string    */
  PString            rsaResult;             /*!< RSA challenge result - string    */
  PIPSocket::Address apparentAddr;          /*!< Apparent address of peer - struct sockaddr_in    */
  PINDEX             refresh;               /*!< When to refresh registration - short    */
  PINDEX             dpStatus;              /*!< Dialplan status - short    */
  PINDEX             callNo;                /*!< Call number of peer - short    */
  PString            cause;                 /*!< Cause - string    */
  BYTE               causeCode;             /*!< cause code    */
  BYTE               iaxUnknown;            /*!< never used    */
  int                msgCount;              /*!< How many messages waiting - short    */
  int                autoAnswer;            /*!< Request auto-answering -- none    */
  int                musicOnHold;           /*!< Request musiconhold with QUELCH -- none or string    */
  unsigned int       transferId;            /*!< Transfer Request Identifier -- int    */
  PTime              dateTime;              /*!< Date/Time    */
  PString            deviceType;            /*!< device type    */
  PString            serviceIdent;          /*!< service ident    */
  int                firmwareVer;           /*!< firmware ver    */
  unsigned int       fwBlockDesc;           /*!< fw block desc    */
  PBYTEArray         fwBlockData;           /*!< fw block data    */
  PString            encKey;                /*!< encryption key    */
  unsigned int       provVer;               /*!< provisioning ver    */
  PINDEX             samplingRate;          /*!< samplingrate    */
  int                provverPres;           /*!< provisioning ver    */
  unsigned int       receivedJitter;        /*!< received jitter (as in rfc 1889) u32 */
  unsigned int       receivedLoss;          /*!< Received loss (high byte loss pckt, low 24 bits loss count, as in rfc1889 */
  unsigned int       receivedPackets;       /*!< recevied frames (total frames received) u32 */
  unsigned short     receivedDelay;         /*!< Max playout delay for received frame (in ms) u16*/
  unsigned int       receivedDropped;       /*!< Dropped frames (presumably by jitterbuf) u32 */
  unsigned int       receivedOoo;           /*!< Frames received Out of Order u32 */
};


#endif // OPAL_IAX2

#endif // OPAL_IAX2_IEDATA_H

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */
