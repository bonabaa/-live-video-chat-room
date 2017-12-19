/*
 * overview.cxx
 *
 * Inter Asterisk Exchange 2
 * 
 * documentation overview.
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
 *
 * $Revision: 19786 $
 * $Author: dereksmithies $
 * $Date: 2008-03-19 03:52:22 +0000 (Wed, 19 Mar 2008) $
 */

/*! \page pageIAX2Protocol IAX2 section of the OPAL library

\section secOverview Overview

    This is the IAX2 section of the OPAL library.
    
    It is an implementation of the IAX2 voip protocol.


\version 1.0
\author  Derek J Smithies



	  
\section secArchitecture Architecture


There is one instance of a IAX2Receiver class, which listens on port 4569 for incoming IAX2
packets. Incoming packets (from all calls) go via the one instance of the IAX2Receiver class.

Incoming packets may be used to generate a new IAX2Connection class (if
it is a request to open a call).  Incoming packets are then passed on to the
IAX2Connection class for handling. Note that all connections listen to the same
IAX2Receiver.

There is one instance of a IAX2Transmit class, which sends data out port 4569 to the remote
endpoint. Outgoing packets (from all calls) go via the one instance of a IAX2Transmit class.
Note that all connections send data to the same IAX2Transmit.


\section secClassListings Available classes

\subsection subsecKeyClasses The following classes are key to the implementation of IAX2 in the opal library

\li IAX2FrameIdValue           - Element of an internal list, used to keep track of incoming IAX2FullFrame out sequence numbers.
\li IAX2FrameList              - A list of frames, which can be accessed in a thread safe fashion.
\li IAX2Connection          - Manage the IAX2 protocol for one call, and connect to Opal
\li IAX2EndPoint            - Manage the IAX2 protocol specific issues which are common to all calls, and connect to Opal.
\li IAX2Processor           - Separate thread to handle all IAX2 protocol requests, and transfer media frames.
\li IAX2IncomingEthernetFrames - Separate thread to transfer all frames from the IAX2Receiver to the 
                                  appropriate IAX2Connection.
\li IAX2OpalMediaStream     - Transfer media frames between IAX2Connection to Opal.
\li IAX2PacketIdList           - A list of IAX2FrameIdValue elements, which is used to keep track of incoming IAX2FullFrame out sequence numbers
\li IAX2Receiver               - Handles the reception of data from the remote node
\li IAX2Remote                 - contain information about the remote node. 
\li SafeString                 - handle a PString in a thread safe fashion.
\li SafeStrings                - Handle a PStringArray in a thread safe fashion.
\li IAX2SoundList              - Maintain a list of the sound packets as read from the microphone.
\li IAX2Transmit               - Send IAX2 packets to the remote node, and handle retransmit issues.
\li IAX2WaitingForAck          - A predefined action, that is to carried out (by one particular IAX2Processor) on receiving an ack.


\subsection subsecFrame classes for holding the UDP data about to be sent to (or received from) the network.

\li IAX2Frame - the parent class of all frames.
\li IAX2MiniFrame - a UDP frame of data for sending voice/audio. Not as large as a IAX2FullFrame
\li IAX2FullFrame - the parent class of all full frames.
\li IAX2FullFrameCng - Transfers Cng (comfort noise generation)
\li IAX2FullFrameDtmf
\li IAX2FullFrameHtml 
\li IAX2FullFrameImage 
\li IAX2FullFrameNull 
\li IAX2FullFrameProtocol - managing the session (and doesn't really do anything in IAX2)
\li IAX2FullFrameSessionControl - carries information about call control, registration, authentication etc.
\li IAX2FullFrameText 
\li IAX2FullFrameVideo 
\li IAX2FullFrameVoice 


\subsection subsecIe  classes for carrying information in the IAX2FullFrameSessionControl UDP frames

\li IAX2Ie - the parent of all information element types

\subsection subsecIeData classes for the different data types expressed in an Ie
\li IAX2IeByte        - Byte of data in data field.
\li IAX2IeChar        - character of data in the data field
\li IAX2IeDateAndTime - data field contains a 32 bit number with date and time (2 sec resolution)
\li IAX2IeInt         - data field contains an integer.
\li IAX2IeNone        - there is no data field.
\li IAX2IeShort       - data field contains a short
\li IAX2IeSockaddrIn  - data field contains a sockaddr_in,  which is address and port
\li IAX2IeString      - data field contains a string - there is no zero byte at the end.
\li IAX2IeUInt        - data field contains an unsigned int.
\li IAX2IeUShort      - data field contains an unsigned short.

\subsection subsecIeAllTypes Classes for each of the  possible Ie types

\li IAX2IeAdsicpe 
\li IAX2IeAesProvisioning 
\li IAX2IeApparentAddr 
\li IAX2IeAuthMethods 
\li IAX2IeAutoAnswer 
\li IAX2IeBlockOfData 
\li IAX2IeCallNo 
\li IAX2IeCalledContext 
\li IAX2IeCalledNumber 
\li IAX2IeCallingAni 
\li IAX2IeCallingName 
\li IAX2IeCallingNumber 
\li IAX2IeCallingPres 
\li IAX2IeCallingTns 
\li IAX2IeCallingTon 
\li IAX2IeCapability 
\li IAX2IeCause         - text description of what happened (typically used at call completion, explaining why the call was finished)
\li IAX2IeCauseCode     - numeric value describing why the call was completed.
\li IAX2IeChallenge 
\li IAX2IeCodecPrefs 
\li IAX2IeData
\li IAX2IeDateAndTime
\li IAX2IeDateTime       - 32 bit value (2 sec accuracy) of the date and time. Year is in range of 2000-2127.
\li IAX2IeDeviceType 
\li IAX2IeDnid 
\li IAX2IeDpStatus 
\li IAX2IeDroppedFrames
\li IAX2IeEncKey 
\li IAX2IeEncryption 
\li IAX2IeFirmwareVer 
\li IAX2IeFormat 
\li IAX2IeFwBlockData 
\li IAX2IeFwBlockDesc 
\li IAX2IeIaxUnknown 
\li IAX2IeInvalidElement 
\li IAX2IeLanguage 
\li IAX2IeList 
\li IAX2IeMd5Result 
\li IAX2IeMsgCount 
\li IAX2IeMusicOnHold 
\li IAX2IePassword 
\li IAX2IeProvVer 
\li IAX2IeProvisioning 
\li IAX2IeRdnis 
\li IAX2IeReceivedDelay
\li IAX2IeReceivedFrames
\li IAX2IeReceivedJitter
\li IAX2IeReceivedLoss
\li IAX2IeReceivedOoo
\li IAX2IeRefresh
\li IAX2IeRsaResult
\li IAX2IeSamplingRate    - sampling rate in hertz. Typically, the value is 8000
\li IAX2IeServiceIdent
\li IAX2IeTransferId
\li IAX2IeUserName         - data field contains the username of the transmitting node
\li IAX2IeVersion          - version of IAX in opertion, typically the value is 2






*/
