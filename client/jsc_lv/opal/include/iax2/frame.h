/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Define the classes that carry information over the ethernet.
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

#ifndef OPAL_IAX2_FRAME_H
#define OPAL_IAX2_FRAME_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if OPAL_IAX2

#include <iax2/ies.h>
#include <iax2/remote.h>

#include <ptlib/sockets.h>

#ifdef P_USE_PRAGMA
#pragma interface
#endif

class IAX2Frame;
class IAX2FrameList;
class IAX2FullFrame;
class IAX2FullFrameCng;
class IAX2FullFrameDtmf;
class IAX2FullFrameHtml;
class IAX2FullFrameImage;
class IAX2FullFrameNull;
class IAX2FullFrameProtocol;
class IAX2FullFrameSessionControl;
class IAX2FullFrameText;
class IAX2FullFrameVideo;
class IAX2FullFrameVoice;
class IAX2EndPoint;
class IAX2Processor;
class IAX2CallProcessor;
class IAX2IeList;
class IAX2MiniFrame;
class IAX2Transmitter;



/**Base class for holding data to be sent to a remote endpoint*/
class IAX2Frame :  public PObject
{
  PCLASSINFO(IAX2Frame, PObject);
 public:
  /**construction */
  IAX2Frame(IAX2EndPoint &_endpoint);
  
  /**Destructor - which is empty */
  virtual ~IAX2Frame();
  
  /**Wait on the designated socket for an incoming UDP packet. This
     method is only called by the receiver. This method does NO interpretation*/
  PBoolean ReadNetworkPacket(PUDPSocket &sock);
  
  /**Interpret the data from the read process*/
  virtual PBoolean ProcessNetworkPacket();
  
  /**True if this is a full frame */
  virtual PBoolean IsFullFrame();
  
  /**True if it is a video frame */
  PBoolean IsVideo() const;
  
  /**True if is is an audio frame */
  PBoolean IsAudio() const;

  /**Pointer to the beginning of the media (after the header) in this packet.
     The low level frame has no idea on headers, so just return pointer to beginning
     of data. */
  virtual BYTE *GetMediaDataPointer() { return data.GetPointer(); }
  
  /**Number of bytes in the media section of this packet. The low
     level frame has no idea on headers, so just return the number of
     bytes in the packet.*/
  virtual PINDEX GetMediaDataSize() { return DataSize();}
  
  /**Reporting function, to describe the number of bytes in this packet */
  PINDEX DataSize() { return data.GetSize(); }
  
  /**Get the value of the Remote structure */
  IAX2Remote & GetRemoteInfo() { return remote; }
  
  /**Obtain a pointer to the current position in the incoming data array */
  const BYTE * GetDataPointer() { return data + currentReadIndex; }
  
  /** Read the input frame, and create the correct IAX2MiniFrame,
      FullFrame, and set frame type variables.  If a password is
      supplied, decrypt the frame with this password

      This method will never delete the input frame.
      If the output is null, it failed to derive a result.*/
  IAX2Frame * BuildAppropriateFrameType(IAX2Encryption &encryptionInfo);

  /**Same as the preceeding function, except that encryption is off */
  IAX2Frame * BuildAppropriateFrameType();

  /** How many bytes are unread in the incoming data array */
  PINDEX   GetUnReadBytes() { return data.GetSize() - currentReadIndex; }
  
  /**Cause the header bytes for this particular frame type to be written to the internal array */
  virtual PBoolean WriteHeader() { return PFalse; }
  
  /**Send this packet on the specified socket to the remote host. This method is only
     called by the transmiter.*/
  virtual PBoolean TransmitPacket(PUDPSocket &sock);
  
  /**Pretty print this frame data to the designated stream*/
  virtual void PrintOn(ostream & strm) const;
  
  /**Calculate the timestamp value, given the call start tick*/
  static DWORD CalcTimeStamp(const PTimeInterval & callStartTick);

  /**Write the timestamp value, in preparation for writing the
     header. When sending a packet, the timestamp is written at packet
     construction.*/
  virtual void BuildTimeStamp(const PTimeInterval & callStartTick);
  
  /**Return a reference to the endpoint structure */
  IAX2EndPoint & GetEndpoint() { return endpoint; }
  
  /**Globally unique ID string for this frame, to help track frames.
   The value returned is a pretty printed address of this frame instance.*/
  PString IdString() const;
    
  /**Get the timestamp as used by this class*/
  DWORD  GetTimeStamp() { return timeStamp; }
  
  /** Report flag stating that this call must be active when this frame is transmitted*/
  virtual PBoolean CallMustBeActive() { return PTrue; }     
  
  /**Specify the type of this frame. */
  enum IAX2FrameType {
    undefType        = 0,     /*!< full frame type is  Undefined                     */
    dtmfType         = 1,     /*!< full frame type is  DTMF                          */
    voiceType        = 2,     /*!< full frame type is  Audio                         */
    videoType        = 3,     /*!< full frame type is  Video                         */
    controlType      = 4,     /*!< full frame type is  Session Control               */
    nullType         = 5,     /*!< full frame type is  NULL - frame ignored.         */
    iax2ProtocolType = 6,     /*!< full frame type is  IAX protocol specific         */
    textType         = 7,     /*!< full frame type is  text message                  */
    imageType        = 8,     /*!< full frame type is  image                         */
    htmlType         = 9,     /*!< full frame type is  HTML                          */
    cngType          = 10,    /*!< full frame type is  CNG (comfort noise generation */
    numFrameTypes    = 11     /*!< the number of defined IAX2 frame types            */
  };
  
  /**Access the current value of the variable controlling frame type,
     which is used when reading data from the network socket. */
  IAX2FrameType GetFrameType() { return frameType; }
    
  /**Method supplied here to provide basis for descendant classes.

   Whenever a frame is transmitted, this method will be called.*/
  virtual void InitialiseHeader(IAX2Processor * /*processor*/) { }
  
  /**Return true if this frame should be retransmitted. Acks are never
     retransmitted. cmdNew are retransmitted.*/
  PBoolean CanRetransmitFrame() const {return canRetransmitFrame; } 
  
  /**Get the string which uniquely identifies the IAXConnection that
     sent this frame */
  PString GetConnectionToken() const { return connectionToken; }

  /**Set the string which uniquely identifies the IAXConnection that
     is responsible for this frame */
  void SetConnectionToken(PString newToken) { connectionToken = newToken; } 

  /**Create the connection token id, which uniquely identifies the
     connection to process this call */
  void BuildConnectionTokenId();

  /**Write the data in the variables to this frame's data array. If
     encryption is on, the data will be encrypted */
  PBoolean EncryptContents(IAX2Encryption &encData);

  /**Get the offset to the beginning of the encrypted region */
  virtual PINDEX GetEncryptionOffset();

 protected:

  /**Use the supplied encryptionKey, and data in storage, to decrypt this frame.
   
  Return False if the decryption fails, PTrue if the decryption works.*/
  PBoolean DecryptContents(IAX2Encryption & encryption);

  /**Specification of the location (address, call number etc) of the
     far endpoint */
  IAX2Remote  remote;
  
  /**Variable specifying the IAX type of frame that this is. Used only
     in reading from the network */
  IAX2FrameType frameType;
  
  /** Read 1 byte from the internal area, (Internal area is filled
      when reading the packet in). Big Endian.*/
  PBoolean          Read1Byte(BYTE & res);
  
  /** Read 2 bytes from the internal area, (Internal area is filled
      when reading the packet in) Big Endian.*/
  PBoolean          Read2Bytes(PINDEX & res);
  
  /** Read 2 bytes from the internal area, (Internal area is filled
      when reading the packet in) Big Endian.*/
  PBoolean          Read2Bytes(WORD & res);
  
  /** Read 4 bytes from the internal area, (Internal area is filled
      when reading the packet in) Big Endian.*/
  PBoolean          Read4Bytes(DWORD & res);
  
  /** Write 1 byte to the internal area, as part of writing the header
      info */
  void          Write1Byte(BYTE newVal);
  
  /** Write 1 byte to the internal area, as part of writing the header
      info. Send only the lowest 8 bits of source*/
  void          Write1Byte(PINDEX newVal);
  
  /** Write 2 bytes to the internal area, as part of writing the
      header info Big Endian.*/
  void          Write2Bytes(PINDEX newVal);
  
  /** Write 4 bytes to the internal area, as part of writing the
      header info Big Endian.*/
  void          Write4Bytes(unsigned int newVal);
  
  /** Initialise all internal storage in this structrue */
  void          ZeroAllValues();
  
  /**Reference to the global variable of this program */
  IAX2EndPoint      & endpoint;
  
  /**Internal storage array, ready for sending to remote node, or
     ready for receiving from remote node*/
  PBYTEArray         data;
  
  /**Flag to indicate if this is a MiniFrame or FullFrame */
  PBoolean               isFullFrame;
  
  /**Flag to indicate if this is a MiniFrame with video */
  PBoolean               isVideo;
  
  /**Flag to indicate if this is a MiniFrame with audio */
  PBoolean               isAudio;
  
  /**Index of where we are reading from the internal data area */
  PINDEX               currentReadIndex;  
  
  /**Index of where we are writing to  the internal data area */  
  PINDEX               currentWriteIndex;  
  
  /**unsigned 32 bit representaiton of the time of this day */
  DWORD                timeStamp;  
  
  /**Indicate if this frame can be retransmitted*/
  PBoolean               canRetransmitFrame;

  /**Connection Token, which uniquely identifies the IAXConnection
     that sent this frame. The token will (except for the first setup
     packet) uniquely identify the IAXConnection that is to receive
     this incoming frame.  */
  PString            connectionToken;

  /**The time stamp to use, for those cases when the user demands a
   * particular timestamp on construction. */
  DWORD presetTimeStamp;
};

/////////////////////////////////////////////////////////////////////////////    
/**Class to manage a mini frame, which is sent unreliable to the remote endpoint*/
class IAX2MiniFrame : public IAX2Frame
{
  PCLASSINFO(IAX2MiniFrame, IAX2Frame);
 public:
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2MiniFrame(const IAX2Frame & srcFrame);
  
  /** Construction from an endpoint, to create an empty frame. */
  IAX2MiniFrame(IAX2EndPoint & _endpoint); 
  
  /**Construction from an encoded audio array (stored in a
     PBYTEArray), in preparation to sending to remote node.  The
     constructor will not delete the supplied PBYTEArray structure.

     TimeStamp will be calculated from time since call started, if the
     users timestamp is 0.  If the users timeStamp is non zero, the
     frames timestamp will be this.
  */
  IAX2MiniFrame(IAX2Processor * con, PBYTEArray &sound, PBoolean isAudio, DWORD usersTimeStamp = 0);

  /**Destructor*/
  virtual ~IAX2MiniFrame();
  
  /** Process the incoming frame some more, but process it as this
      frame type demands*/
  virtual PBoolean ProcessNetworkPacket();
  
  /**Write the header to the internal data area */
  virtual PBoolean WriteHeader();
  
  /**Pretty print this frame data to the designated stream*/
  virtual void PrintOn(ostream & strm) const;
  
  /**Pointer to the beginning of the media (after the header) in this
     packet */
  virtual BYTE *GetMediaDataPointer();
  
  /**Number of bytes in the media section of this packet. */
  virtual PINDEX GetMediaDataSize();
  
  /**Fix the timestamp in this class, after being shrunk in the MiniFrames */
  void AlterTimeStamp(PINDEX newValue);
  
  /**Given the supplied Connection class, write the first 12 bytes of the frame.
     This method is called by the frame constructors, in preparation for transmission.
     This method is never called when processing a received frame.

     Whenever a frame is transmitted, this method will be called.*/ 
  virtual void InitialiseHeader(IAX2Processor *processor);
  
  /**Get the offset to the beginning of the encrypted region */
  virtual PINDEX GetEncryptionOffset();

 protected:
  /**Initialise valus in this class to some preset value */
  void ZeroAllValues();
};

/////////////////////////////////////////////////////////////////////////////    
/////////////////////////////////////////////////////////////////////////////    
/**Class to handle a full frame, which is sent reliably to the remote endpoint */
class IAX2FullFrame : public IAX2Frame
{
  PCLASSINFO(IAX2FullFrame, IAX2Frame);
 public:
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrame(const IAX2Frame & srcFrame);
  
  /** Construction from an endpoint, to create an empty frame. 
      In this case, the class is filled with the various methods*/
  IAX2FullFrame(IAX2EndPoint  &_endpoint);
  
  /**Delete this frame now, but first we have to delete every timer on it.
   */
  virtual ~IAX2FullFrame();
  
  /**Return True if this an ack frame */
  PBoolean IsAckFrame() { return isAckFrame; }
  
  /**Return True if this is a PING frame */
  PBoolean IsPingFrame();

  /**Return True if this is a NEW frame */
  PBoolean IsNewFrame();

  /**Return True if this is a LAGRQ frame */
  PBoolean IsLagRqFrame();

  /**Return True if this is a LAGRP frame */
  PBoolean IsLagRpFrame();

  /**Return True if this is a PONG frame */
  PBoolean IsPongFrame();

  /**Return True if this is a AuthReq frame */
  PBoolean IsAuthReqFrame();

  /**Return True if this is a VNAK frame */
  PBoolean IsVnakFrame();
  
  /**Return True if this is a REGREQ frame */
  PBoolean IsRegReqFrame();
  
  /**Return True if this is a REGAUTH frame */
  PBoolean IsRegAuthFrame();
  
  /**Return True if this is a REGACK frame */
  PBoolean IsRegAckFrame();  
  
  /**Return True if this is a REGREL frame */
  PBoolean IsRegRelFrame();
  
  /**Return True if this is a REGREJ frame */
  PBoolean IsRegRejFrame();

  /**Return True if this FullFrame is of a type that increments the
     InSeqNo */
  PBoolean FrameIncrementsInSeqNo();

  /**True if this is a full frame - always returns true as this is a
     full frame. */
  virtual PBoolean IsFullFrame() { return PTrue; }  
  
  /**Report PTrue if this is a hangup frame. We need this information
     for processing incoming frames, before fully dissection of the
     frame has completed */
  PBoolean IsHangupFrame();

  /** Initialise to zero all the members of this particular class */
  void ZeroAllValues();
  
  /** Process the incoming frame some more, but process it as a full
      frame */
  virtual PBoolean ProcessNetworkPacket();
  
  /**Send this packet on the specified socket to the remote host. This
     method is only called by the transmiter.*/
  virtual PBoolean TransmitPacket(PUDPSocket &sock);
  
  /**Get text descrption of this frame type*/
  PString GetFullFrameName() const;
  
  /**Get text description of the subclass contents*/
  virtual PString GetSubClassName() const
    { return PString(" subclass=") + PString(subClass); }
  
  /**Stop the timer, so this packet is not retransmitted. Mark packet
     as dead. This happens when a packet has been received that
     matches one of the previously sent packets. */
  void MarkDeleteNow();
  
  /**A Vnak frame has been received. This Vnak frame is demanding that
     we resend this particular frame. Given it is to be resent by
     vnak, we reset the countdown variables. E.G. it gets the full
     amount of retries again. */
  void MarkVnakSendNow();

  /**Pointer to the beginning of the media (after the header) in this
     packet */
  virtual BYTE *GetMediaDataPointer();
  
  /**Number of bytes in the media section of this packet. */
  virtual PINDEX GetMediaDataSize();
  
  /**Determine the current value of the subClass variable */
  PINDEX GetSubClass() const { return subClass; }
  
  /**Dry the current value of the subClass variable */
  void SetSubClass(PINDEX newValue) { subClass = newValue;}
  
  /**Write the header for this class to the internal data array. 12
     bytes of data are writen.  The application developer must write
     the remaining bytes, before transmiting this frame. */
  virtual PBoolean WriteHeader();
  
  /**Alter the two bytes for in and out sequence values. (in the
     header)*/
  void ModifyFrameHeaderSequenceNumbers(PINDEX inNo, PINDEX outNo);

  /**Alter the four bytes for this frames timestamp. It is required,
     when transmitting full frames, that there is a 3ms interval to
     last full frame in the timestamps. This is required by
     limitations in the handline of time in asterisk. */
  void ModifyFrameTimeStamp(PINDEX newTimeStamp);

  /**Mark this frame as having (or not having) information elements*/
  virtual PBoolean InformationElementsPresent() { return PFalse; }  
  
  /**Get flag to see if this frame is ready to be sent (or resent). In
     other words, has the timer expired?*/
  PBoolean  SendFrameNow() { return sendFrameNow; }
  
  /**Get flag to see if this frame is ready for deletion. In other
     words. Has it been sent too many times? */
  PBoolean  DeleteFrameNow() { return deleteFrameNow; }
  
  /**Get the sequence number info (inSeqNo and outSeqNo) */
  IAX2SequenceNumbers & GetSequenceInfo() { return sequence; }
  
  /**Pretty print this frame data to the designated stream*/
  virtual void PrintOn(ostream & strm) const;
  
  /**Mark this frame as having been resent (set bit 7 of data[2])*/
  void MarkAsResent();
  
  /**Compare this FullFrame with another full frame, which is used when determining if we are
     dealing with a frame we have already processed */
  PBoolean operator *= (IAX2FullFrame & other);
  
  /**enum to define if the call must be active when sending this
     frame*/
  enum ConnectionRequired {
    callActive,      /*!< the call must be active when sending frame*/
    callIrrelevant   /*!< the call may, or may not be, active when sending frame*/
  };
  
  /**Return the FullFrame type represented here (voice, protocol, session etc*/
  virtual BYTE GetFullFrameType() { return 0; }

  /**Get the offset to the beginning of the encrypted region */
  virtual PINDEX GetEncryptionOffset() { return 4; }
  
 protected:
  /** Report flag stating that this call must be active when this frame is transmitted*/
  virtual PBoolean CallMustBeActive() { return callMustBeActive; }
  
  /**Turn the 8 bit subClass value into a 16 bit representation */
  void UnCompressSubClass(BYTE a);
  
  /**Turn the 16 bit subClass value into a 8 bit representation */
  int  CompressSubClass();
  
  /**Mark this frame as not to be sent, and not to be deleted */
  void ClearListFlags();
  
  /**Given the supplied Connection class, write the first 12 bytes of the frame.
     This method is called by the frame construcors, in preparation for transmission.
     This method is never called when processing a received frame.

     Whenever a frame is transmitted, this method will be called.*/
  virtual void InitialiseHeader(IAX2Processor *processor);
  
#ifdef DOC_PLUS_PLUS
  /** pwlib constructs to cope with timeout, when transmitting a full frame.  This
      happens when a full frame has not been acknowledged in the
      required time period. This frame will be resent.*/
  void OnTransmissionTimeout(PTimer &, INT);
#else
  PDECLARE_NOTIFIER(PTimer, IAX2FullFrame, OnTransmissionTimeout);
#endif
  /** The timer which is used to test for no reply to this frame (on transmission) */
  PTimer transmissionTimer;
  
  /** integer variable specifying the uncompressed subClass value for this particular frame */
  PINDEX subClass;
  
  /**Time to wait between retries */
  PTimeInterval retryDelta;     
  
  /**Time delta between call start and sending (or receiving)*/
  PTimeInterval timeOffset;     
  
  /**Number of retries this frame has undergone*/
  PINDEX       retries;        
  
  /** Internal variables specifying the retry time periods (in milliseconds) */
  enum RetryTime {
    minRetryTime = 500,    /*!< 500 milliseconds     */
    maxRetryTime = 010000, /*!< 10 seconds           */
    maxRetries   = 3       /*!< number of retries    */
  };
  
  /**Class holding the sequence numbers, which is used by all classes which have a FullFrame ancestor. */
  IAX2SequenceNumbers sequence;
  
  /**List flag, indicating if this frame ready for sending*/
  PBoolean         sendFrameNow;   
  
  /**List flag, this frame is ready for deletion (too many retries)*/
  PBoolean         deleteFrameNow; 
  
  /**A tracking flag to indicate this fame has been resent*/
  PBoolean         packetResent;   
  
  /** Flag stating that this call must be active when this frame is transmitted  */
  PBoolean callMustBeActive;
  
  /** flag to indicate if this is an ack frame */
  PBoolean isAckFrame;  
};

/////////////////////////////////////////////////////////////////////////////    

/**Used for transmitting dtmf characters in a reliable fashion. One
   frame per dtmf character.  No data is carried in the data
   section */

class IAX2FullFrameDtmf : public IAX2FullFrame
{
  PCLASSINFO(IAX2FullFrameDtmf, IAX2FullFrame);
 public:
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameDtmf(const IAX2Frame & srcFrame);

  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameDtmf(const IAX2FullFrame & srcFrame);
  
  /**Construction from a Connection class. 
     Classes generated from this are then on sent to a remote endpoint. */
  IAX2FullFrameDtmf(IAX2Processor *processor, /*!< Iax Processor from which this frame originates      */ 
		    char  subClassValue       /*!< IAX protocol command for remote end to process   */
		    );
  
  /**Construction from a Connection class. 
     Classes generated from this are then on sent to a remote endpoint. */
  IAX2FullFrameDtmf(IAX2Processor *processor, /*!< Iax Processor from which this frame originates      */ 
		PString  subClassValue    /*!< IAX protocol command for remote end to process   */
		);
  
  
  /**Get text description of the subclass contents*/
  virtual PString GetSubClassName() const; 
  
  /**enum comtaining the possible subclass value for these dtmf frames */
  enum DtmfSc {
    dtmf0 = 48,     /*!< DTMF character 0     */
    dtmf1 = 49,     /*!< DTMF character 1     */
    dtmf2 = 50,     /*!< DTMF character 2     */
    dtmf3 = 51,     /*!< DTMF character 3     */
    dtmf4 = 52,     /*!< DTMF character 4     */
    dtmf5 = 53,     /*!< DTMF character 5     */
    dtmf6 = 54,     /*!< DTMF character 6     */
    dtmf7 = 55,     /*!< DTMF character 7     */
    dtmf8 = 56,     /*!< DTMF character 8     */
    dtmf9 = 57,     /*!< DTMF character 9     */
    dtmfA = 65,     /*!< DTMF character A     */
    dtmfB = 66,     /*!< DTMF character B     */
    dtmfC = 67,     /*!< DTMF character C     */
    dtmfD = 68,     /*!< DTMF character D     */
    dtmfStar = 42,  /*!< DTMF character *     */
    dtmfHash = 35   /*!< DTMF character #     */
  };
  
  /**Return the FullFrame type represented here (voice, protocol, session etc*/
  virtual BYTE GetFullFrameType() { return dtmfType; }
  
 protected:
};

/////////////////////////////////////////////////////////////////////////////    
/**Used for transmitting voice packets in a relaible
   fashion. Typically, one is sent at the start of the call, and then
   at regular intervals in the call to keep the timestamps in sync.
   
   This class has the ability to build audio codecs, and report on available formats.
   
   The contents the data section is the compressed audio frame. */
class IAX2FullFrameVoice : public IAX2FullFrame
{
  PCLASSINFO(IAX2FullFrameVoice, IAX2FullFrame);
 public:
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameVoice(const IAX2Frame & srcFrame);
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameVoice(const IAX2FullFrame & srcFrame);
  
  /**Construction from an encoded audio array (stored in a
     PBYTEArray), in preparation to sending to remote node.  The
     constructor will not delete the supplied PBYTEArray structure.

     The full frame will use the specified timeStamp, if it is > 0.  If the
     specified timeStamp == 0, the timeStamp will be calculated from when the
     call started.
  */
  IAX2FullFrameVoice(IAX2CallProcessor *processor, PBYTEArray &sound, 
		 PINDEX usersTimeStamp = 0);
  
  /**Declare an empty destructor */
  virtual ~IAX2FullFrameVoice();

  /**Get text description of the subclass contents*/
  virtual PString GetSubClassName() const;
  
  /**Get text description of the subclass contents, given the supplied
     argument*/
  static PString GetSubClassName(unsigned short testValue)
     { return GetSubClassName((unsigned int) testValue); }

  /**Get text description of the subclass contents, given the supplied
     argument*/
  static PString GetSubClassName(unsigned int testValue);
  
  /**Get text description of the subclass contents, given the supplied
   argument.  The name returned is that recoginised by the OPAL
   library. */
  static PString GetOpalNameOfCodec(PINDEX testValue);
  
  /**Get text description of the subclass contents, given the supplied argument*/
  static PString GetSubClassName(int testValue) 
    { return GetSubClassName((unsigned short) testValue); }
  
  /**Turn the OPAL string (which describes the codec) into a AudioSc value. If there is no conversion
     found, return 0. */
  static unsigned short OpalNameToIax2Value(const PString opalName);

  /**enum comtaining the possible (uncompressed) subclass value for these voice frames */
  enum AudioSc {
    g7231    = 1,         /*!< G.723.1 audio format in this frame        */
    gsm      = 2,         /*!< GSM audio format in this frame            */
    g711ulaw = 4,         /*!< G.711 uLaw audio format in this frame     */
    g711alaw = 8,         /*!< G.711 ALaw audio format in this frame     */
    mp3      = 0x10,      /*!< MPeg 3 audio format in this frame         */
    adpcm    = 0x20,      /*!< ADPCM audio format in this frame          */
    pcm      = 0x40,      /*!< PCM audio format in this frame            */
    lpc10    = 0x80,      /*!< LPC10 audio format in this frame          */
    g729     = 0x100,     /*!< G.729 audio format in this frame          */
    speex    = 0x200,     /*!< Speex audio format in this frame          */
    ilbc     = 0x400,     /*!< ILBC audio format in this frame           */
    supportedCodecs = 11  /*!< The number of codecs defined by this enum */
  };
  
  /**Return the IAX2FullFrame type represented here (voice, protocol, session etc*/
  virtual BYTE GetFullFrameType() { return voiceType; }
};
/////////////////////////////////////////////////////////////////////////////    
/**Used for transmitting video packets in a relaible
   fashion. Typically, one is sent at the start of the call, and then
   at regular intervals in the call to keep the timestamps in sync.
   
   The contents the data section is compressed video. */
class IAX2FullFrameVideo : public IAX2FullFrame
{
  PCLASSINFO(IAX2FullFrameVideo, IAX2FullFrame);
 public:
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameVideo(const IAX2Frame & srcFrame);
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameVideo(const IAX2FullFrame & srcFrame);
  
  /**Get text description of the subclass contents*/
  virtual PString GetSubClassName() const;
  
  /**enum comtaining the possible (uncompressed) subclass value for these video frames */
  enum VideoSc {
    jpeg  = 0x10000,   /*!< Jpeg video format in this frame     */
    png   = 0x20000,   /*!< PNG video format in this frame      */
    h261  = 0x40000,   /*!< H.261 video format in this frame    */
    h263  = 0x80000    /*!< H.263 video format in this frame    */
  };
  
  /**Return the FullFrame type represented here (voice, protocol, session etc*/
  virtual BYTE GetFullFrameType() { return videoType; }
 protected:
};

/////////////////////////////////////////////////////////////////////////////    
/**Used for sending  Control Frames. These are used to manipulate the session. 
   
Asterisk calls these AST_FRAME_CONTROLs

No data is carried in the data section */
class IAX2FullFrameSessionControl : public IAX2FullFrame
{
  PCLASSINFO(IAX2FullFrameSessionControl, IAX2FullFrame);
 public:
  
  /**enum comtaining the possible subclass value for these Session Control frames */
  enum SessionSc {
    hangup          = 1,     /*!< Other end has hungup    */
    ring            = 2,     /*!< Local ring    */
    ringing         = 3,     /*!< Remote end is ringing    */
    answer          = 4,     /*!< Remote end has answered    */
    busy            = 5,     /*!< Remote end is busy    */
    tkoffhk         = 6,     /*!< Make it go off hook    */
    offhook         = 7,     /*!< Line is off hook    */
    congestion      = 8,     /*!< Congestion (circuits busy)    */
    flashhook       = 9,     /*!< Flash hook    */
    wink            = 10,    /*!< Wink    */
    option          = 11,    /*!< Set a low-level option    */
    keyRadio        = 12,    /*!< Key Radio    */
    unkeyRadio      = 13,    /*!< Un-Key Radio    */
    callProgress    = 14,    /*!< Indicate PROGRESS    */
    callProceeding  = 15,    /*!< Indicate CALL PROCEEDING    */
    callOnHold      = 16,    /*!< Call has been placed on hold    */
    callHoldRelease = 17,    /*!< Call is no longer on hold    */
    stopSounds      = 255    /*!< Indicates the transition from ringback to bidirectional audio */
  };
  
  
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameSessionControl(const IAX2Frame & srcFrame);
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameSessionControl(const IAX2FullFrame & srcFrame);
  
  /**Construction from a Connection class. 
     Classes generated from this are then on sent to a remote endpoint. */
  IAX2FullFrameSessionControl(IAX2Processor *processor, /*!<Iax Processor from which this frame originates    */ 
			  PINDEX subClassValue/*!<IAX protocol command for remote end to process   */
			  );
  
  /**Construction from a Connection class. 
     Classes generated from this are then on sent to a remote endpoint. */
  IAX2FullFrameSessionControl(IAX2Processor *processor,     /*!< Iax Processor from which this frame originates */
			  SessionSc subClassValue /*!< IAX protocol command for remote end to process*/
			  );
  
  /**Declare an empty destructor*/
  virtual ~IAX2FullFrameSessionControl() { }

  /**Get text description of the subclass contents*/
  virtual PString GetSubClassName() const;
  
  /**Return the IAX2FullFrame type represented here (voice, protocol, session etc*/
  virtual BYTE GetFullFrameType() { return controlType; }
  
 protected:
};

/////////////////////////////////////////////////////////////////////////////    
/**Class for transfering Nothing. It is essentially a NO Op 
   It is used internally to indicate that the sound card should play a silence frame.
   
   These frames are sent reliably.
   There is no data in the subclass section.
   There is no data in the data section */
class IAX2FullFrameNull : public IAX2FullFrame
{
  PCLASSINFO(IAX2FullFrameNull, IAX2FullFrame);
 public:
  /**Construct an empty Full Frame.
     Classes generated like this are used to handle transmitted information */
  IAX2FullFrameNull(IAX2EndPoint & endpoint) : IAX2FullFrame(endpoint)   { }
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet.
     Classes generated like this are used to handle received data. */
  IAX2FullFrameNull(const IAX2Frame & srcFrame);
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet.
     Classes generated like this are used to handle received data. */
  IAX2FullFrameNull(const IAX2FullFrame & srcFrame);
  
  /**Get text description of the subclass contents*/
  virtual PString GetSubClassName() const { return  PString(""); }
  
  /**Return the IAX2FullFrame type represented here (voice, protocol, session etc*/
  virtual BYTE GetFullFrameType() { return nullType; }
  
 protected:
};

/////////////////////////////////////////////////////////////////////////////    
/**Handle IAX specific protocol issues. Used for initiating a call,
   closing a call, registration, reject a call etc.. These are used to
   manipulate the session.
   
   The data section contains information elements, or type Ie classes. */

class IAX2FullFrameProtocol : public IAX2FullFrame
{
  PCLASSINFO(IAX2FullFrameProtocol, IAX2FullFrame);
 public:
  
  /**enum comtaining the possible subclass value for these IAX protocol control frames */
  enum ProtocolSc {
    cmdNew       =  1,       /*!< Create a new call    */
    cmdPing      =  2,       /*!< Ping request, which is done on an open call. It is "Are you Alive    */
    cmdPong      =  3,       /*!< reply to a Ping    */
    cmdAck       =  4,       /*!< Acknowledge a Reliably sent full frame    */
    cmdHangup    =  5,       /*!< Request to terminate this call    */
    cmdReject    =  6,       /*!< Refuse to accept this call. May happen if authentication faile    */
    cmdAccept    =  7,       /*!< Allow this call to procee    */
    cmdAuthReq   =  8,       /*!< Ask remote end to supply authenticatio    */
    cmdAuthRep   =  9,       /*!< A reply, that contains authenticatio    */
    cmdInval     =  10,      /*!< Destroy this call immediatly    */
    cmdLagRq     =  11,      /*!< Initial message, used to measure the round trip time    */
    cmdLagRp     =  12,      /*!< Reply to cmdLagrq, which tells us the round trip time    */
    cmdRegReq    =  13,      /*!< Request for Registration    */
    cmdRegAuth   =  14,      /*!< Registration requires for authentication    */
    cmdRegAck    =  15,      /*!< Registration has been accepted    */
    cmdRegRej    =  16,      /*!< Registration has been rejected    */
    cmdRegRel    =  17,      /*!< Force the release of the current registration    */
    cmdVnak      =  18,      /*!< This indicates out of order frames, and can be read as voice not acknowledged */
    cmdDpReq     =  19,      /*!< Request the status of an entry for dialplan     */
    cmdDpRep     =  20,      /*!< Request status of an entry for  dialplan     */
    cmdDial      =  21,      /*!< Request that there is a dial (TBD) on a channel    */
    cmdTxreq     =  22,      /*!< Request a Transfer    */
    cmdTxcnt     =  23,      /*!< Connect up a  Transfer     */
    cmdTxacc     =  24,      /*!< Transfer has been accepted    */
    cmdTxready   =  25,      /*!< Transfer is ready to happen   */
    cmdTxrel     =  26,      /*!< Release a Transfer    */
    cmdTxrej     =  27,      /*!< Reject a Transfer   */
    cmdQuelch    =  28,      /*!< Stop media transmission    */
    cmdUnquelch  =  29,      /*!< Resume media transmission    */
    cmdPoke      =  30,      /*!< Query the remote endpoint (there is no open connection) */
    cmdPage      =  31,      /*!< Do a Page */
    cmdMwi       =  32,      /*!< Indicate : message waiting */
    cmdUnsupport =  33,      /*!< We have received an unsupported message */
    cmdTransfer  =  34,      /*!< Initiate the remote end to do a transfer */
    cmdProvision =  35,      /*!< Provision the remote end    */
    cmdFwDownl   =  36,      /*!< The remote end must download some firmware    */
    cmdFwData    =  37       /*!< This message contains firmware.    */
  };
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet.
     Classes generated like this are used to handle received data. */
  IAX2FullFrameProtocol(const IAX2Frame & srcFrame);
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet.
     Classes generated like this are used to handle received data. */
  IAX2FullFrameProtocol(const IAX2FullFrame & srcFrame);
  
  /**Construction from a Connection class. 
     Classes generated from this are then on sent to a remote endpoint. */
  IAX2FullFrameProtocol(IAX2Processor *processor,         /*!< Iax Processor from which this frame originates            */ 
		    PINDEX subClassValue,            /*!< IAX protocol command for remote end to process         */
		    ConnectionRequired needCon = IAX2FullFrame::callActive
		                                     /*!< this frame is only sent if the Connection class exists */
		    );
  
  /**Construction from an Connection class. 
     Classes generated from this are then on sent to a remote endpoint. */
  IAX2FullFrameProtocol(IAX2Processor *processor,         /*!< Iax Processor from which this frame originates            */ 
		    ProtocolSc  subClassValue,       /*!< IAX protocol command for remote end to process         */
		    ConnectionRequired needCon=IAX2FullFrame::callActive 
		                                     /*!< this frame is only sent if the Connection class exists */
		    );
  
  /**Construction from a Connection class. 
     Classes generated from this are then on sent to a remote endpoint.
     
     We have received a IAX2FullFrameProtocol, and this constructor is used to create a reply.
     Use the iseqno and time stamp from the supplied frame to construct the reply */
  IAX2FullFrameProtocol(IAX2Processor *processor,         /*!< Iax Processor from which this frame originates            */ 
		    ProtocolSc  subClassValue,       /*!< IAX protocol command for remote end to process         */
		    IAX2FullFrame *inReplyTo,            /*!< this message was sent in reply to this frame           */
		    ConnectionRequired needCon = IAX2FullFrame::callActive
		                                     /*!< this frame is only sent if the Connection class exists */
		    );
  
  /**Destructor, which deletes all current Information Elements */
  virtual ~IAX2FullFrameProtocol();
  
  /**Set internal variable to say that this frame does not need to be retransmitted*/
  void SetRetransmissionRequired();
  
  /**Mark this frame as having (or not having) information elements*/
  virtual PBoolean InformationElementsPresent() { return !ieElements.IsEmpty(); }
  
  /**Report the current value of the subClass variable */
  ProtocolSc GetSubClass() const { return (ProtocolSc) subClass; }

  /**Get text description of the subclass contents*/
  virtual PString GetSubClassName() const; 

  /**Get text description of the subclass contents*/
  static PString GetSubClassName(PINDEX t);
  
  /**Return a pointer to the n'th Ie in the internal list. If it is not
     there, a NULL is returned */
  IAX2Ie *GetIeAt(PINDEX i) {      return ieElements.GetIeAt(i); }
  
  /**Add a new Information Element (Ie) to the intenral list */
  void AppendIe(IAX2Ie *newElement) { ieElements.AppendIe(newElement); }
  
  /**Write the contents of the Ie internal list to the frame data array.
     This is usually done in preparation to transmitting this frame */
  void WriteIeAsBinaryData();
  
  /**Transfer the data (stored in the IeList) and place it in into
     the IeData class.  This is done when precessing a frame we
     have received from an external node, which has to be stored in the IeData class*/
  void CopyDataFromIeListTo(IAX2IeData &res);
  
  /**Look through the list of IEs, and look for remote capabability
     and preferred codec */
  void GetRemoteCapability(unsigned int & capability, unsigned int & preferred);

  /**Return the IAX2FullFrame type represented here (voice, protocol, session etc*/
  virtual BYTE GetFullFrameType() { return iax2ProtocolType; }
  
  /**Pretty print this frame data to the designated stream*/
  virtual void PrintOn(ostream & strm) const;

 protected:
  
  /**Read the information elements from the incoming data array 
     to generate a list of information element classes*/
  PBoolean ReadInformationElements();
  
  /**A list of the IEs read from/(or  written to) the data section of this frame,*/
  IAX2IeList ieElements;

  /**Pretty print the protocol value with an English word. */
#if PTRACING
    friend ostream & operator<<(ostream & o, ProtocolSc t);
#endif
};

/////////////////////////////////////////////////////////////////////////////    
/**Class for transfering text. These frames are sent reliably.
   There is no data in the subclass section.
   
   The text is carried in the data section.
*/
class IAX2FullFrameText : public IAX2FullFrame
{
  PCLASSINFO(IAX2FullFrameText, IAX2FullFrame);
 public:

  /**Construction from a Connection class.
     Classes generated from this are then on sent to a remote endpoint. */
  IAX2FullFrameText(IAX2Processor *processor,       /*!< Iax Processor from which this frame originates      */
		const PString&  textValue/*!< text to send to remote end   */
		);
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameText(const IAX2Frame & srcFrame);
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameText(const IAX2FullFrame & srcFrame);
  
  /**Get text description of the subclass contents*/
  virtual PString GetSubClassName() const;
  
  /**Return the IAX2FullFrame type represented here (voice, protocol, session etc*/
  virtual BYTE GetFullFrameType() { return textType; }

  /**Return the text data*/
  PString GetTextString() const;

 protected:

  /**The text string that will be placed in the body of this message */
  PString internalText;
};
/////////////////////////////////////////////////////////////////////////////    

/**Class for transfering images. These frames are sent reliably.
   The subclass describes the image compression format.
   
   The contents of the data section is the raw image data */
class IAX2FullFrameImage : public IAX2FullFrame
{
  PCLASSINFO(IAX2FullFrameImage, IAX2FullFrame);
 public:
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameImage(const IAX2Frame & srcFrame);
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameImage(const IAX2FullFrame & srcFrame);
  
  /**Get text description of the subclass contents*/
  virtual PString GetSubClassName() const;
  
  /**Return the IAX2FullFrame type represented here (voice, protocol, session etc*/
  virtual BYTE GetFullFrameType() { return imageType; }
 protected:
};

/////////////////////////////////////////////////////////////////////////////    

/**Class for transfering html. These frames are sent reliably.
   The subclass describes the html frame type.
   
   The contents of the data section is message specific */
class IAX2FullFrameHtml : public IAX2FullFrame
{
  PCLASSINFO(IAX2FullFrameHtml, IAX2FullFrame);
 public:
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameHtml(const IAX2Frame & srcFrame);
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameHtml(const IAX2FullFrame & srcFrame);
  
  /**Get text description of the subclass contents*/
  virtual PString GetSubClassName() const;
  
  /**Return the IAX2FullFrame type represented here (voice, protocol, session etc*/
  virtual BYTE GetFullFrameType() { return htmlType; }
 protected:
};

/////////////////////////////////////////////////////////////////////////////    
/**Class for transfering Cng (comfort noise generation). These frames are sent reliably.
   

The contents of the data section is message specific */
class IAX2FullFrameCng : public IAX2FullFrame
{
  PCLASSINFO(IAX2FullFrameCng, IAX2FullFrame);
 public:
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameCng(const IAX2Frame & srcFrame);
  
  /**Construction from a supplied dataframe.
     In this case, this class is filled from an incoming data packet*/
  IAX2FullFrameCng(const IAX2FullFrame & srcFrame);
  
  /**Get text description of the subclass contents*/
  virtual PString GetSubClassName() const;
  
  /**Return the IAX2FullFrame type represented here (voice, protocol, session etc*/
  virtual BYTE GetFullFrameType() { return cngType; }
 protected:
};

/////////////////////////////////////////////////////////////////////////////    

PDECLARE_LIST (IAX2FrameList, IAX2Frame *)
#ifdef DOC_PLUS_PLUS     //This makes emacs bracket matching code happy.
/** A list of all frames waiting for processing
	 
    Note please, this class is thread safe.
     
   You do not need to protect acces to this class.
*/     
class IAX2FrameList : public IAX2Frame *  
{
#endif
 public:
  ~IAX2FrameList();
  
  /**Report the frames queued in this list*/
  void ReportList(PString & answer);
  
  /**Get pointer to last frame in the list. Remove this frame from the list */
  IAX2Frame *GetLastFrame();
  
  /**Removing item from list will not automatically delete it */
  void Initialise();
    
  /**True if this frame list is empty*/
  PBoolean Empty() { return GetSize() == 0; }
  
  /**Copy to this frame the contents of the frameList pointed to by src*/
  void GrabContents(IAX2FrameList &src);
  
  /**Delete the frame that has been sent, which is waiting for this
     reply. The reply is the argument. */
  void DeleteMatchingSendFrame(IAX2FullFrame *reply);

  /** A Vnak frame has been received (voice not acknowledged) which actually
      means, retransmit all those frames you have on this particular call
      number from the oseqno specified in the supplied frame */
  void SendVnakRequestedFrames(IAX2FullFrameProtocol &src);

  /**Add the frame (supplied as an argument) to the end of this list*/
  void AddNewFrame(IAX2Frame *src);
  
  /**Get a list of frames to send, and delete the timed out frames */
  void GetResendFramesDeleteOldFrames(IAX2FrameList & framesToSend);
  
  /**Thread safe read of the number of elements on this list. */
  virtual PINDEX GetSize() { PWaitAndSignal m(mutex); return PAbstractList::GetSize(); }
  
  /**Mark every frame on this list as having been resent*/
  void MarkAllAsResent();
  
 protected:
  /**NON Thread safe read of the number of elements on this list. */
  virtual PINDEX GetEntries() { return PAbstractList::GetSize(); }
  
  /**Local variable which protects access. */
  PMutex mutex;
};

/////////////////////////////////////////////////////////////////////////////
/**The class IAX2ActiveFrameList is essentially the same as
   IAX2FrameList, except that it is initialised (by default) and the
   user is not required to use the Initialise method. This class will
   not ever automatically delete members when they are removed */
class IAX2ActiveFrameList : public IAX2FrameList
{
  PCLASSINFO(IAX2ActiveFrameList, IAX2FrameList);
 public:
  IAX2ActiveFrameList() { Initialise(); }
};
/////////////////////////////////////////////////////////////////////////////    


#endif // OPAL_IAX2

#endif // OPAL_IAX2_FRAME_H

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 2 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-basic-offset:2
 * End:
 */
