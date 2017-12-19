/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Class to hold all the possible Ie Data types in one place
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

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_IAX2

#ifdef P_USE_PRAGMA
#pragma implementation "iedata.h"
#endif

#include <iax2/iedata.h>


#define new PNEW

IAX2IeData::IAX2IeData()
{
  adsicpe      = 0;
  autoAnswer   = 0;
  callNo       = 0;
  callingPres  = -1;  
  callingTns   = -1;
  callingTon   = -1;
  causeCode    = 0;
  dpStatus     = 0;
  firmwareVer  = -1;
  iaxUnknown   = 0;
  msgCount     = -1;
  musicOnHold  = 0;
  provverPres  = 0;
  refresh      = 0;
  samplingRate = 0;
  version      = 0;
}


IAX2IeData::~IAX2IeData()
{
}


void IAX2IeData::PrintOn(ostream & strm) const
{
  strm << endl 	  
       <<   "Number/extension being called - string               " << calledNumber      << endl
       <<   "Calling number - string                              " << callingNumber     << endl
       <<   "Calling number ANI for billing  - string             " << callingAni        << endl
       <<   "Name of caller - string                              " << callingName       << endl
       <<   "calling typeofnum                                    " << callingTon        << endl
       <<   "calling transitnet                                   " << callingTns        << endl
       <<   "calling presntn                                      " << callingPres       << endl
       <<   "Context for number - string                          " << calledContext     << endl
       <<   "Username (peer or user) for authentication - string  " << userName          << endl
       <<   "Password for authentication - string                 " << password          << endl
       <<   "Actual codec capability - unsigned int               " << capability        << endl
       <<   "Desired codec format - unsigned int                  " << format            << endl
       <<   "codec_prefs                                          " << codecPrefs        << endl
       <<   "Desired language - string                            " << language          << endl
       <<   "Protocol version - short                             " << version           << endl
       <<   "CPE ADSI capability - short                          " << adsicpe           << endl
       <<   "Originally dialed DNID - string                      " << dnid              << endl
       <<   "Referring DNIS -- string                             " << rdnis             << endl
       <<   "Authentication method(s) - short                     " << authMethods       << endl
       <<   "encryption method to us                              " << encryptionMethods << endl
       <<   "Challenge data for MD5/RSA - string                  " << challenge         << endl
       <<   "MD5 challenge result - string                        " << md5Result         << endl
       <<   "RSA challenge result - string                        " << rsaResult         << endl
       <<   "Apparent address of peer - struct sockaddr_in        " << apparentAddr      << endl
       <<   "When to refresh registration - short                 " << refresh           << endl
       <<   "Dialplan status - short                              " << dpStatus          << endl
       <<   "Call number of peer - short                          " << callNo            << endl
       <<   "Cause - string                                       " << cause             << endl
       <<   "cause code                                           " << causeCode         << endl
       <<   "never used                                           " << iaxUnknown        << endl
       <<   "How many messages waiting - short                    " << msgCount          << endl
       <<   "Request auto-answering -- none                       " << autoAnswer        << endl
       <<   "Request musiconhold with QUELCH -- none or string    " << musicOnHold       << endl
       <<   "Transfer Request Identifier -- int                   " << transferId        << endl
       <<   "Date/Time                                            " << dateTime          << endl
       <<   "device type                                          " << deviceType        << endl
       <<   "service ident                                        " << serviceIdent      << endl
       <<   "firmware ver                                         " << firmwareVer       << endl
       <<   "fw block desc                                        " << fwBlockDesc       << endl
       <<   "fw block data                                        " << fwBlockData       << endl
       <<   "encryption key                                       " << encKey            << endl
       <<   "provisioning ver                                     " << provVer           << endl
       <<   "samplingrate                                         " << samplingRate      << endl
       <<   "provisioning ver                                     " << provverPres       << endl
       <<   "received jitter (as in rfc 1889) u32                 " << receivedJitter    << endl
       <<   "Received loss (high byte loss pckt, low 24 bits loss " << receivedLoss      << endl
       <<   "recevied frames (total frames received) u32          " << receivedPackets   << endl
       <<   "Max playout delay for received frame (in ms) u16     " << receivedDelay     << endl
       <<   "Dropped frames (presumably by jitterbuf) u32         " << receivedDropped   << endl
       <<   "Frames received Out of Order u32                     " << receivedOoo       << endl;
}


#endif // OPAL_IAX2

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */

