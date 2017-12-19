/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Class to manage information information about the remote node.
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

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_IAX2

#ifdef P_USE_PRAGMA
#pragma implementation "remote.h"
#endif

#include <iax2/remote.h>
#include <iax2/frame.h>

#define new PNEW


IAX2Remote::IAX2Remote()
{
  sourceCallNumber  = callNumberUndefined;
  destCallNumber    = callNumberUndefined;
  
  remotePort        = 0;
}



void IAX2Remote::Assign(IAX2Remote & source)
{
  destCallNumber   = source.SourceCallNumber();
  sourceCallNumber = source.DestCallNumber();
  remoteAddress    = source.RemoteAddress();
  remotePort       = source.RemotePort();
}

PBoolean IAX2Remote::operator==(IAX2Remote & other)
{
  if (remoteAddress != other.RemoteAddress()) {
    PTRACE(5, "Comparison of two remotes " << endl << other << endl << (*this) );
    PTRACE(5, "comparison of two remotes  Addresses are different");
    return PFalse;
  }
  
  if (remotePort != other.RemotePort()) {
    PTRACE(5, "Comparison of two remotes " << endl << other << endl << (*this) );
    PTRACE(5, "comparison of two remotes  remote ports are different");
    return PFalse;
  }
  
  if (destCallNumber != other.DestCallNumber()) {
    PTRACE(5, "Comparison of two remotes " << endl << other << endl << (*this) );
    PTRACE(5, "comparison of two remotes. Dest call numbers differ");	
    return PFalse;
  }
  
  if (sourceCallNumber != other.SourceCallNumber()) {
    PTRACE(5, "Comparison of two remotes " << endl << other << endl << (*this) );
    PTRACE(5, "comparison of two remotes. Source call numbers differ");	
    return PFalse;
  }
  
  return PTrue;
}


PBoolean IAX2Remote::operator*=(IAX2Remote & other)
{
  PTRACE(6, "Incoming ethernet frame. Compare" << endl << other << endl << (*this) );
  
  if (remoteAddress != other.RemoteAddress()) {
    PTRACE(3, "comparison of two remotes  Addresses are different");
    return PFalse;
  }
  
  if (remotePort != other.RemotePort()) {
    PTRACE(5, "comparison of two remotes  remote ports are different");
    return PFalse;
  }
  
  if (sourceCallNumber != other.DestCallNumber()) {
    PTRACE(5, "comparison of two remotes. Local source number differs to incoming dest call number");
    PTRACE(5, " local sourceCallNumber " << sourceCallNumber 
	   << "        incoming Dest " << other.DestCallNumber());
    return PFalse;
  }
  
  PTRACE(6, "comparison of two remotes  They are the same  ");
  return PTrue;
}


PBoolean IAX2Remote::operator!=(IAX2Remote & other)
{
  return !(*this == other);
}

void IAX2Remote::PrintOn(ostream & strm) const
{
  strm << "src call number" << sourceCallNumber 
       << "        Dest call number" << destCallNumber 
       << "        remote address" << remoteAddress 
       << "        Remote port" << remotePort ;
}

PString IAX2Remote::BuildConnectionTokenId()
{
  return PString("iax2:") 
    + RemoteAddress().AsString() 
    + PString("-") 
    + PString(SourceCallNumber());  
}

PString IAX2Remote::BuildOurConnectionTokenId()
{
  return PString("iax2:") 
    + RemoteAddress().AsString() 
    + PString("-") 
    + PString(DestCallNumber());  
}

////////////////////////////////////////////////////////////////////////////////
 //paranoia here. Use brackets to guarantee the order of calculation.
IAX2FrameIdValue::IAX2FrameIdValue(PINDEX timeStamp, PINDEX seqVal)
{
  value = (timeStamp << 8) + (seqVal & 0xff);
}

IAX2FrameIdValue::IAX2FrameIdValue(PINDEX val)
{
  value = val;
}


PINDEX IAX2FrameIdValue::GetPlainSequence() const
{
  return (PINDEX)(value & P_MAX_INDEX);
}

PINDEX IAX2FrameIdValue::GetTimeStamp() const
{
  return (PINDEX)(value >> 8);            
}

PINDEX IAX2FrameIdValue::GetSequenceVal() const 
{
  return (PINDEX)(value & 0xff);         
}

void IAX2FrameIdValue::PrintOn(ostream & strm) const 
{
  strm << setw(8) << GetTimeStamp() << " --" << setw(4) << GetSequenceVal(); 
}


PObject::Comparison IAX2FrameIdValue::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, IAX2FrameIdValue), PInvalidCast);
  const IAX2FrameIdValue & other = (const IAX2FrameIdValue &)obj;
 
  if ((value > 224) && (other.value < 32))
    return LessThan;   //value has wrapped around 256, other has not.

  if ((value < 32) && (other.value > 224))
    return GreaterThan; //value has wrapped around 256, other has not.

  if (value < other.value)
    return LessThan;
 
  if (value > other.value)
    return GreaterThan;
 
  return EqualTo;
} 

////////////////////////////////////////////////////////////////////////////////
PBoolean IAX2PacketIdList::Contains(IAX2FrameIdValue &src)
{
	PINDEX idex = GetValuesIndex(src);
	return idex != P_MAX_INDEX;
}

PINDEX IAX2PacketIdList::GetFirstValue()
{
  if (GetSize() == 0)
    return 255;

  return ((IAX2FrameIdValue *)GetAt(0))->GetPlainSequence();
}

void IAX2PacketIdList::RemoveOldContiguousValues()
{
  PBoolean contiguous = PTrue;
  while((GetSize() > 1) && contiguous)  {
    PINDEX first = ((IAX2FrameIdValue *)GetAt(0))->GetPlainSequence();
    PINDEX second = ((IAX2FrameIdValue *)GetAt(1))->GetPlainSequence();
    contiguous = ((first + 1) & 0xff) == second;
    if (contiguous)
      RemoveAt(0);
  }
}

void IAX2PacketIdList::PrintOn(ostream & strm) const
{
  strm << "Packet Id List Size=" << GetSize() << endl;
  for(PINDEX i = 0; i < GetSize(); i++)
    strm << (*((IAX2FrameIdValue *)GetAt(i))) << endl;
}

void IAX2PacketIdList::AppendNewFrame(IAX2FullFrame &src)
{
  IAX2FrameIdValue *f = new IAX2FrameIdValue(src.GetSequenceInfo().OutSeqNo());
  PTRACE(5, "AppendNewFrame " << (*f));

  if(GetSize() == 0) {
    PTRACE(5, "SeqNos\tList empty, so add now. " << (*f));
    Append(f);
    return;
  }

  if (Contains(*f)) {
    PTRACE(5, "SeqNos\tJustRead frame is " << (*f));
    PTRACE(5, "SeqNos\tIn queue waiting removal " << (*f));
    delete f;
    return;
  }

  if (((IAX2FrameIdValue *)GetAt(0))->Compare(*f) == GreaterThan) {
    PTRACE(5, "SeqNos\tHave already processed " << (*f));
    PTRACE(5, "SeqNos\tFirst frame in que " << (*(IAX2FrameIdValue *)GetAt(0)));
    PTRACE(5, "SeqNos\tFrame just read is " << (*f));
    delete f;
    return;
  }

  PTRACE(5, "SeqNos\tList is younger than this value. " << (*f));
  Append(f);
  RemoveOldContiguousValues();

  PTRACE(5, "SeqNos\t"  << (*this));
}

////////////////////////////////////////////////////////////////////////////////

void IAX2SequenceNumbers::ZeroAllValues()
{
  PWaitAndSignal m(mutex);

  inSeqNo  = 0;
  outSeqNo = 0;
  lastSentTimeStamp = 0;
}



PBoolean  IAX2SequenceNumbers::operator != (IAX2SequenceNumbers &other)
{
  PWaitAndSignal m(mutex);
  if (inSeqNo == other.InSeqNo())
    return PFalse;
  
  if (inSeqNo == other.OutSeqNo())
    return PFalse;
  
  if (outSeqNo == other.InSeqNo())
    return PFalse;
  
  if (outSeqNo == other.OutSeqNo())
    return PFalse;
  
  return PTrue;
}

void IAX2SequenceNumbers::SetAckSequenceInfo(IAX2SequenceNumbers & other)
{
  PWaitAndSignal m(mutex);
  outSeqNo = other.InSeqNo();
}

PBoolean IAX2SequenceNumbers::operator == (IAX2SequenceNumbers &other)
{
  PWaitAndSignal m(mutex);
  if ((inSeqNo == other.InSeqNo()) && (outSeqNo == other.OutSeqNo()))
    return PTrue;
  
  
  if ((inSeqNo == other.OutSeqNo()) && (outSeqNo == other.InSeqNo()))
    return PTrue;
  
  return PFalse;
}


void IAX2SequenceNumbers::MassageSequenceForSending(IAX2FullFrame &src)
{
  PWaitAndSignal m(mutex);

  if (src.IsAckFrame()) {
    src.ModifyFrameHeaderSequenceNumbers(inSeqNo, src.GetSequenceInfo().OutSeqNo());
    return;
  } 

  PTRACE(5, "SeqNos\tMassage - SequenceForSending(FullFrame &src) ordinary Frame");

  PINDEX timeStamp = src.GetTimeStamp();
  if ((timeStamp < (lastSentTimeStamp + minSpacing)) && !src.IsNewFrame() && 
      !src.IsPongFrame() && !src.IsLagRpFrame() &&
      !src.IsAckFrame()) {
    timeStamp = lastSentTimeStamp + minSpacing;
    src.ModifyFrameTimeStamp(timeStamp);
  }

  lastSentTimeStamp = timeStamp;

  if (src.IsVnakFrame()) {
      src.ModifyFrameHeaderSequenceNumbers(inSeqNo, inSeqNo);
      return;
    } else
      src.ModifyFrameHeaderSequenceNumbers(inSeqNo, outSeqNo);
  
  outSeqNo++;
}

IAX2SequenceNumbers::IncomingOrder IAX2SequenceNumbers::IncomingMessageInOrder(IAX2FullFrame &src)
{
  if (src.IsAckFrame())
    return InSequence;

  if (src.IsHangupFrame())
    return InSequence;

  PINDEX newFrameOutSeqNo = src.GetSequenceInfo().OutSeqNo();

  PWaitAndSignal m(mutex);
  if ((inSeqNo & 0xff) == newFrameOutSeqNo) {
    PTRACE(5, "SeqNos\treceivedoseqno is " << newFrameOutSeqNo << " and in order");
    inSeqNo ++;
    return InSequence;
  }

  if ((inSeqNo & 0xff) > newFrameOutSeqNo) {
    //WE have already seen this frame. drop it.
    PTRACE(5, "SeqNos\treceivedoseqno is " << newFrameOutSeqNo << " We have already seen this frame");
    return RepeatedFrame;
  }

  PTRACE(5, "SeqNos\treceivedoseqno is " << newFrameOutSeqNo << " is out of order.  " << inSeqNo);
  return SkippedFrame;
}



void IAX2SequenceNumbers::CopyContents(IAX2SequenceNumbers &src)
{
  PWaitAndSignal m(mutex);
  inSeqNo = src.InSeqNo();
  outSeqNo = src.OutSeqNo();
}

PString IAX2SequenceNumbers::AsString() const
{
  PWaitAndSignal m(mutex);
  PStringStream  res;
  res << "   in" << inSeqNo << "   out" << outSeqNo ;
  
  return res;
}

void IAX2SequenceNumbers::PrintOn(ostream & strm) const
{
  strm << AsString();
}

PINDEX IAX2SequenceNumbers::InSeqNo() 
{ 
  PWaitAndSignal m(mutex);
  return inSeqNo; 
}
  
PINDEX IAX2SequenceNumbers::OutSeqNo() 
{
  PWaitAndSignal m(mutex);
  return outSeqNo; 
}
  
PBoolean IAX2SequenceNumbers::IsFirstReplyFrame() 
{ 
  PWaitAndSignal m(mutex);
  return (inSeqNo == 1) && (outSeqNo == 0); 
}

PBoolean IAX2SequenceNumbers::IsSequenceNosZero()
{
  PWaitAndSignal m(mutex);
  return ((inSeqNo && 0xff) == 0) && ((outSeqNo && 0xff) == 0); 
}

void IAX2SequenceNumbers::SetInSeqNo(PINDEX newVal) 
{
  PWaitAndSignal m(mutex);
  inSeqNo = newVal; 
}
  
void IAX2SequenceNumbers::SetOutSeqNo(PINDEX newVal) 
{
  PWaitAndSignal m(mutex);
  outSeqNo = newVal;
}

void IAX2SequenceNumbers::SetInOutSeqNo(PINDEX inVal, PINDEX outVal)
{
  PWaitAndSignal m(mutex);
  
  inSeqNo = inVal;
  outSeqNo = outVal;
}

////////////////////////////////////////////////////////////////////////////////
IAX2Encryption::IAX2Encryption() 
{ 
  encryptionEnabled = PFalse; 
}


void IAX2Encryption::SetEncryptionOn (PBoolean newState) 
{ 
  encryptionEnabled = newState; 
  PTRACE(3, "Set encryption to " << PString(encryptionEnabled ? "On" : "Off"));
}

void IAX2Encryption::SetEncryptionKey(PString & newKey) 
{ 
  encryptionKey = newKey; 
  CalculateAesKeys();
}

void IAX2Encryption::SetChallengeKey(PString & newKey) 
{ 
  challengeKey = newKey; 
  CalculateAesKeys();
}

const PString & IAX2Encryption::EncryptionKey() const 
{ 
  return encryptionKey; 
}

const PString & IAX2Encryption::ChallengeKey() const 
{ 
  return challengeKey; 
}

PBoolean IAX2Encryption::IsEncrypted() const
{
  return encryptionEnabled;
}

#if OPAL_PTLIB_SSL_AES
AES_KEY *IAX2Encryption::AesEncryptKey()
{
  return &aesEncryptKey; 
}

AES_KEY *IAX2Encryption::AesDecryptKey()
{
  return &aesDecryptKey;
}
#endif

void IAX2Encryption::CalculateAesKeys()
{
  if (encryptionKey.IsEmpty())
    return;

  if (challengeKey.IsEmpty())
    return;


#if OPAL_PTLIB_SSL_AES
  IAX2IeMd5Result ie(*this);
  PBYTEArray context = ie.GetDataBlock();

  PTRACE(6, "Decryption\tContext has a size of " << context.GetSize());

  AES_set_encrypt_key(context.GetPointer(), 128, &aesEncryptKey);
  AES_set_decrypt_key(context.GetPointer(), 128, &aesDecryptKey);
#endif
}
  
#endif // OPAL_IAX2

////////////////////////////////////////////////////////////////////////////////

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 2 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-basic-offset:2
 * End:
 */

