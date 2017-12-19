/*
 *
 *
 * Inter Asterisk Exchange 2
 * 
 * A class to describe the node we are talking to.
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
 * $Revision: 21421 $
 * $Author: dereksmithies $
 * $Date: 2008-10-30 00:36:33 +0000 (Thu, 30 Oct 2008) $
 */

#ifndef OPAL_IAX2_REMOTE_H
#define OPAL_IAX2_REMOTE_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if OPAL_IAX2

#include <ptlib/sockets.h>

#if OPAL_PTLIB_SSL_AES
#include <openssl/aes.h>
#endif

#ifdef P_USE_PRAGMA
#pragma interface
#endif

class IAX2FullFrame;


/**A simple class which contains the different source and dest call
   numbers, and the remote address + remote port */
class IAX2Remote : public PObject
{ 
  PCLASSINFO(IAX2Remote, PObject);
  
 public:
  
  /**Constructor*/
  IAX2Remote();
  
  virtual ~IAX2Remote() { };
  
  /**Call number as used at the destination of this data frame. If we are
     receiving this packet, it refers to our call number. */
  PINDEX DestCallNumber() { return destCallNumber; }
  
  /**Call number as used at the source of this data frame. If we are
     receiving this packet, it refers to the call number at the remote
     host. */ 
  PINDEX SourceCallNumber() { return sourceCallNumber; }
 
  /**Pretty print this remote structure (address & port) to the designated stream*/
  virtual void PrintOn(ostream & strm) const; 
  
  /**Define which is used to indicate the call number is undefined */
  enum {
    callNumberUndefined = 0xffff 
  };
  
  /** Return the current value of the ip address used by the other end of this call */
  PIPSocket::Address RemoteAddress() { return remoteAddress; }
  
  /**the connection token can be derived from the information in this
     class. Consequently, get this class to create the connection
     token */
  PString BuildConnectionTokenId();

  /**Similar to BuildConnectionTokenId, but build it with our source call  number, not remote call number */
  PString BuildOurConnectionTokenId();

  /** return the current value of the port at the other end of this call */
  PINDEX   RemotePort() { return remotePort; }
  
  /**Copy data from supplied Remote structure to this class */
  void Assign(IAX2Remote &);
  
  /**Set the remote address as used by this class */
  void SetRemoteAddress(PIPSocket::Address &newVal) { remoteAddress = newVal; }
  
  /**Set the remote address as used by this class */
  void SetRemoteAddress(int newVal) { remoteAddress = newVal; }
  
  /**Set the remote port, as used by this class */
  void SetRemotePort (PINDEX newVal) { remotePort = newVal; }
  
  /**Set the Source Call Number, as used by this class */
  void SetSourceCallNumber(PINDEX newVal) { sourceCallNumber = newVal; }
  
  /**Set the Dest Call Number, as used by this class */
  void SetDestCallNumber(PINDEX newVal) { destCallNumber = newVal; }
  
  /**Return true if remote port & address & destCallNumber & source
     call number match up.  This is used when finding a Connection
     that generated an ethernet frame which is to be transmitted*/
  PBoolean operator == (IAX2Remote & other);
  
  /**Return true if remote port & address & destCallNumber==sourceCallNumber  match up.
     This is used when finding a Connection to process an incoming ethernet frame */
  PBoolean operator *= (IAX2Remote & other);
  
  
  /**Returns true if these are are different */
  PBoolean operator != (IAX2Remote & other);
  
  
 protected:
  /**Call number at the local computer.*/
  PINDEX       sourceCallNumber;    
  
  /**Call number at the remote computer.*/
  PINDEX       destCallNumber;      
  
  /**Ip address used by the remote endpoint*/
  PIPSocket::Address remoteAddress; 
  
  /**Port number used by the remote endpoint.*/
  PINDEX               remotePort;    

};

////////////////////////////////////////////////////////////////////////////////
/**A class to store the timestamp and sequence number in a indexable
  fashion.  

  This class will be used as a key into the sorted list, which is
  declared below  (PacketIdList).  This class is required because 
  pwlib's dictionaries requires the key to be a descendant from a PObject

  The 32 bit timestamp is left shifted by 8 bits, and the
  result is added to the 8 bit seqno value */
class IAX2FrameIdValue : public PObject
{
  PCLASSINFO(IAX2FrameIdValue, PObject);
 public:
  /**Constructor. to timestamp<<8   +  sequenceNumber*/
  IAX2FrameIdValue (PINDEX timeStamp, PINDEX seqVal);

  /**Constructor to some value */
  IAX2FrameIdValue (PINDEX val);

  /**Retrieve this timestamp */
  PINDEX GetTimeStamp() const;

  /**Retrieve this sequence number.*/
  PINDEX GetSequenceVal() const;

  /**Retrieve the bottom 32 bits.  In this call, the data is assumed
     to be no timestamp, and sequence number could be > 255. This is
     used for the iseqno of transmitted packets */
  PINDEX GetPlainSequence() const;

  /**Pretty print this data to the designated stream*/
  virtual void PrintOn(ostream & strm) const;

  /**Declare this method so that all comparisons (as used in sorted
     lists) work. correctly*/
  virtual Comparison Compare(const PObject & obj) const;

 protected:

  /**The combination of time and sequence number is stored in this
     element, which is a pwlib construct of 64 bits */
  PUInt64 value;
};

////////////////////////////////////////////////////////////////////////////////

/**A class to store a unique identifing number, so we can keep track
of all frames we have sent and received. This will be sued to keep the
iseqno correct (for use in sent frames), and to ensure that we never
send two frames with the same iseqno and timestamp pair.*/
PDECLARE_SORTED_LIST(IAX2PacketIdList, IAX2FrameIdValue)
#ifdef DOC_PLUS_PLUS
class IAX2PacketIdList : public PSortedList
{
#endif
  
  /**Return true if a FrameIdValue object is found in the list that
   * matches the value in the supplied arguement*/
  PBoolean Contains(IAX2FrameIdValue &src);
  
  /**Return the value at the beginning of the list. Use this value as
     the InSeqNo of this endpoint.*/
  PINDEX GetFirstValue();
  
  /**For the supplied frame, append to this list */
  void AppendNewFrame(IAX2FullFrame &src);
  
  /**Pretty print this listto the designated stream*/
  virtual void PrintOn(ostream & strm) const;	
  
 protected:
  /**Remove all the contiguous oldest values, which is used for
     determining the correct iseqno.  This endpoints iseqno is
     determined from::: iseqno is�always
     highest_contiguously_recieved_oseq+1�
  */
  void RemoveOldContiguousValues();
};

////////////////////////////////////////////////////////////////////////////////
/**A structure to hold incoming and outgoing sequence numbers */
class IAX2SequenceNumbers
{
 public:
/**An enum to describe incoming frame. The incoming frame may be on time
   (perfect sequence numbers) repeated (we have already seen it before) or out
   of order (a frame is skipped). */
  enum IncomingOrder {
    InSequence,  ///<  perfect sequence number
    SkippedFrame, ///< there is a missing frame, a VNAK condition
    RepeatedFrame ///< we have already seen this frame...
  };



  /**Constructor, which sets the in and out sequence numbers to zero*/
  IAX2SequenceNumbers() 
    { ZeroAllValues();   };
  
  /**Destructor, which is provided as this class contains virtual methods*/
  virtual ~IAX2SequenceNumbers() { }
  
  /**Initialise to Zero values */
  void ZeroAllValues();
  
  /**Read the incoming sequence number */
  PINDEX InSeqNo();
  
  /**Read the outgoing sequence number */
  PINDEX OutSeqNo();
  
  /**Report true if the frame has inSeqNo and outSeqNo of 1 and 0 respectively, 
     indicating this is a reply to a new frame (could be an ack, accept frame) */
  PBoolean IsFirstReplyFrame();

  /**Report if the sequences numbers (in and out) are both Zero. This is the case for
     some frames (new, invalid) */
  PBoolean IsSequenceNosZero();

  /**Assign new value to the in (or expected) seq number */
  void SetInSeqNo(PINDEX newVal);
  
  /**Assign a new value to the sequence number used for outgoing frames */
  void SetOutSeqNo(PINDEX newVal);

  /**Assign a new value to the seq.in and seq.out, which is used prior
     to sending a frame */
  void SetInOutSeqNo(PINDEX inVal, PINDEX outVal);
  
  /**Assign the sequence nos as appropropriate for when we are sending a
   * sequence set in a ack frame */
  void SetAckSequenceInfo(IAX2SequenceNumbers & other);
  
  /**Comparison operator - tests if sequence numbers are different */
  PBoolean  operator !=(IAX2SequenceNumbers &other);
  
  /**Comparison operator - tests if sequence numbers are equal */
  PBoolean operator ==(IAX2SequenceNumbers &other);
  
  /**Increment this sequences outSeqNo, and assign the results to the source arg */
  void MassageSequenceForSending(IAX2FullFrame &src /*<!src will be transmitted to the remote node */
				 );

  /**Take the incoming frame, and increment seq nos by some multiple
     of 256 to bring them into line with the current values. Use the
     wrapAound member variable to do this.*/
  void WrapAroundFrameSequence(IAX2SequenceNumbers & src);
  
  /** We have received a message from the remote node. Check sequence numbers
      are ok. reply with the appropriate enum to describe if the incoming
      frame is early, late, or on time */
  IncomingOrder IncomingMessageInOrder
    (IAX2FullFrame &src /*<!frame to be compared with current data base.*/  );
  
  /**Copy the sequence info from the source argument to this class */
  void CopyContents(IAX2SequenceNumbers &src);
  
  /**Report  the contents as a string*/
  PString AsString() const;
  
  /**Pretty print in and out sequence numbers  to the designated stream*/
  virtual void PrintOn(ostream & strm) const;

  /**Report PTrue if this sequnece info is the very first packet
     received from a remote node, where we have initiated the call */
  PBoolean IsFirstReply() { return (inSeqNo == 1) && (outSeqNo == 0); }

  /**Add an offset to the inSeqNo and outSeqNo variables */
  void AddWrapAroundValue(PINDEX newOffset);


 protected:

  /**An enum to contain the various defines required by this clsss */
  enum sequenceDefines {
    minSpacing = 3   /*!< minimum spacing in ms for the time stamp of full frames */
  };
  
  /** Packet number (next incoming expected)*/
  PINDEX inSeqNo;  
  
  /** Packet number (outgoing) */
  PINDEX outSeqNo; 

  /**Mutex to protect access to this structrue while doing seqno changes*/
  PMutex mutex;

  /**Last sent time stamp - ensure 3 ms gap between time stamps. */
  PINDEX lastSentTimeStamp;

  /**Dictionary of timestamp and OutSeqNo for frames received by  this iax device */
  IAX2PacketIdList receivedLog;
};

////////////////////////////////////////////////////////////////////////////////
/** A class that holds the state variables on encryption - is it on, and the two keys. */
class IAX2Encryption : public PObject 
{
  PCLASSINFO(IAX2Encryption, PObject);
 public:
  /**Constructor, which sets encrytpion to the default value of "OFF" */
  IAX2Encryption();

  /**Set the flag that indicates this communication session is all encrypted.. */
  void SetEncryptionOn (PBoolean newState = PTrue);

  /**Set the password/key used in encryption process */
  void SetEncryptionKey(PString & newKey);

  /**Set the challenge  used in encryption process */
  void SetChallengeKey(PString & newKey);

  /**Get the value of the encrption key  - or password key */
  const PString & EncryptionKey() const;

  /**Get the value of the challenge key */
  const PString & ChallengeKey() const;

  /**Report if the encryption is enabled  (or turned on) */
  PBoolean IsEncrypted() const;

#if OPAL_PTLIB_SSL_AES
  /**Get a pointer to a filled AES_KEY encrypt structure */
  AES_KEY *AesEncryptKey();

  /**Get a pointer to a filled AES_KEY decrypt structure */
  AES_KEY *AesDecryptKey();
#endif

 protected:
  /**Do the calculation of the encrypt and decrypt AES 128 keys. 
     If neither, or only 1 of the encrypt/challenge keys are defined, do nothing */
  void CalculateAesKeys();

  /**string to use for decryption/encryption of this frame */
  PString encryptionKey;

  /**string to use for decryption/encryption of this frame */
  PString challengeKey;

  /**Flag to specify if encryption is happening */
  PBoolean encryptionEnabled;

#if OPAL_PTLIB_SSL_AES
  /**key to be used for AES 128 encryption */
  AES_KEY aesEncryptKey;

  /**key to be used for AES 128 decoding */
  AES_KEY aesDecryptKey;
#endif
};

////////////////////////////////////////////////////////////////////////////////


#endif // OPAL_IAX2

#endif // OPAL_IAX2_REMOTE_H

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-basic-offset:2
 * End:
 */
