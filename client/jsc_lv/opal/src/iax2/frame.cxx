/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Implementation of the class that specifies frame construction.
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
 * $Revision: 22710 $
 * $Author: dereksmithies $
 * $Date: 2009-05-25 03:17:57 +0000 (Mon, 25 May 2009) $
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_IAX2

#ifdef P_USE_PRAGMA
#pragma implementation "frame.h"
#endif

#include <iax2/frame.h>

#include <iax2/iax2con.h>
#include <iax2/iax2ep.h>
#include <iax2/ies.h>
#include <iax2/receiver.h>
#include <iax2/transmit.h>

#include <ptclib/cypher.h>

#if OPAL_PTLIB_SSL_AES
#include <openssl/aes.h>
  #ifdef _MSC_VER
    #pragma comment(lib, P_SSL_LIB1)
    #pragma comment(lib, P_SSL_LIB2)
  #endif
#endif


#define new PNEW


IAX2Frame::IAX2Frame(IAX2EndPoint &_endpoint)
  : endpoint(_endpoint)
{
  ZeroAllValues();

  PTRACE(6, "Construct IAX2Frame  " << IdString());
}

IAX2Frame::~IAX2Frame()
{
  PTRACE(6, "Destructor for IAX2Frame  " << *this);
}

PString IAX2Frame::IdString() const 
{ 
  PStringStream answer;
  answer << PString("FR-ID#") << ::hex << this << ::dec;
  return answer;
}

void IAX2Frame::ZeroAllValues()
{
  data.SetSize(0);
  
  isFullFrame       = PFalse;
  isVideo           = PFalse;
  isAudio           = PFalse;
  
  currentReadIndex  = 0;
  currentWriteIndex = 0;
  timeStamp = 0;
  
  canRetransmitFrame = PFalse;
  presetTimeStamp = 0;
  
  frameType = undefType;
}

PBoolean IAX2Frame::IsVideo() const 
{ 
  return isVideo; 
}

PBoolean IAX2Frame::IsAudio() const
{ 
  return isAudio; 
}


PBoolean IAX2Frame::IsFullFrame()
{
  return isFullFrame;
}


PBoolean IAX2Frame::ReadNetworkPacket(PUDPSocket &sock)
{
  data.SetSize(4096);  //Surely no packets > 4096 bytes in length
  
  WORD     portNo;
  PIPSocket::Address addr;
  sock.GetLocalAddress(addr);
  
  PBoolean res = sock.ReadFrom(data.GetPointer(), 4096, addr, portNo);
  remote.SetRemoteAddress(addr);
  remote.SetRemotePort(portNo);
  
  if (res == PFalse) {
    PTRACE(3, "Failed in reading from socket");
    return PFalse;
  }
  
  data.SetSize(sock.GetLastReadCount());
  
  if (data.GetSize() < 4) {
    PTRACE(3, "Read a very very small packet from the network - < 4 bytes");
    return PFalse;
  }

  return PTrue;
}

PBoolean IAX2Frame::Read1Byte(BYTE & result)
{
  if (currentReadIndex >= data.GetSize())
    return PFalse;
  
  result = data[currentReadIndex];
  currentReadIndex++;
  return PTrue;
}

PBoolean IAX2Frame::Read2Bytes(PINDEX & res)
{
  BYTE a = 0;
  BYTE b = 0;
  if (Read1Byte(a) && Read1Byte(b)) {
    res = (a << 8) | b;
    return PTrue;
  }
  
  return PFalse;
}

PBoolean IAX2Frame::Read2Bytes(WORD & res)
{
  BYTE a = 0, b = 0;
  if (Read1Byte(a) && Read1Byte(b)) {
    res = (WORD)((a << 8) | b);
    return PTrue;
  }
  
  return PFalse;
}

PBoolean IAX2Frame::Read4Bytes(DWORD & res)
{
  PINDEX a = 0, b = 0;
  if (Read2Bytes(a) && Read2Bytes(b)) {
    res = (a << 16) | b;
    return PTrue;
  }
  
  return PFalse;
}

void IAX2Frame::Write1Byte(PINDEX newVal)
{
  Write1Byte((BYTE)(newVal & 0xff));
}

void IAX2Frame::Write1Byte(BYTE newVal)
{
  if (currentWriteIndex >= data.GetSize())
    data.SetSize(currentWriteIndex + 1);
  
  data[currentWriteIndex] = newVal;
  currentWriteIndex++;
}

void IAX2Frame::Write2Bytes(PINDEX newVal)
{
  BYTE a = (BYTE)(newVal >> 8);
  BYTE b = (BYTE)(newVal & 0xff);
  
  Write1Byte(a);
  Write1Byte(b);
}

void IAX2Frame::Write4Bytes(unsigned int newVal)
{
  PINDEX a = newVal >> 16;
  PINDEX b = newVal & 0xffff;
  
  Write2Bytes(a);
  Write2Bytes(b);
}


PBoolean IAX2Frame::ProcessNetworkPacket()
{
  /*We are guaranteed to have a packet > 4 bytes in size */
  PINDEX a = 0;
  Read2Bytes(a);
  remote.SetSourceCallNumber(a & 0x7fff);

  if (a != 0)
    BuildConnectionTokenId();

  if (a & 0x8000) {
    isFullFrame = PTrue;
    Read2Bytes(a);
    remote.SetDestCallNumber(a & 0x7fff);
    return PTrue;
  }
  if (a == 0) {    //We have a mini frame here, of video type.
    isVideo = PTrue;
    PINDEX b = 0;
    Read2Bytes(b);
    remote.SetSourceCallNumber(b);
    BuildConnectionTokenId();
    return PTrue;
  }

  isAudio = PTrue;
  return PTrue;
}

void IAX2Frame::BuildConnectionTokenId()
{
  connectionToken = remote.BuildConnectionTokenId();
}

void IAX2Frame::PrintOn(ostream & strm) const
{
  strm << IdString() << "      " << data.GetSize() << " bytes " << endl;
}

void IAX2Frame::BuildTimeStamp(const PTimeInterval & callStartTick)
{
  if (presetTimeStamp > 0)
    timeStamp = presetTimeStamp;
  else
    timeStamp = CalcTimeStamp(callStartTick);
}

DWORD IAX2Frame::CalcTimeStamp(const PTimeInterval & callStartTick)
{
  DWORD tVal = (DWORD)(PTimer::Tick() - callStartTick).GetMilliSeconds();
  PTRACE(6, "Calculate timestamp as " << tVal);
  return tVal;
}

PBoolean IAX2Frame::TransmitPacket(PUDPSocket &sock)
{
  if (CallMustBeActive()) {
    if (!endpoint.ConnectionForFrameIsAlive(this)) {
      PTRACE(3, "Connection not found, call has been terminated. " << IdString());
      return PFalse;   //This happens because the call has been terminated.
    }
  }
  
  //     if (PIsDescendant(this, FullFrameProtocol))
  //   	    ((FullFrameProtocol *)this)->SetRetransmissionRequired();
  //	    cout << endl;
  
  PTRACE(6, "Now transmit " << endl << *this);
  PBoolean transmitResult = sock.WriteTo(data.GetPointer(), DataSize(), remote.RemoteAddress(), 
				     (unsigned short)remote.RemotePort());
  PTRACE(6, "transmission of packet gave a " << transmitResult);
  return transmitResult;
}

IAX2Frame * IAX2Frame::BuildAppropriateFrameType(IAX2Encryption & encryptionInfo)
{
  DecryptContents(encryptionInfo);

  return BuildAppropriateFrameType();
}

IAX2Frame *IAX2Frame::BuildAppropriateFrameType()
{
  if (isFullFrame) {
    IAX2FullFrame *ff = new IAX2FullFrame(*this);
    if (!ff->ProcessNetworkPacket()) {
      delete ff;
      return NULL;
    }

    return ff;
  }

  IAX2MiniFrame *mf = new IAX2MiniFrame(*this);
  if (!mf->ProcessNetworkPacket()) {
    delete mf;
    return NULL;
  }
  
  return mf;
}


PBoolean IAX2Frame::DecryptContents(IAX2Encryption &encryption)
{
  if (!encryption.IsEncrypted())
    return PTrue;

#if OPAL_PTLIB_SSL_AES
  PINDEX headerSize = GetEncryptionOffset();
  PTRACE(4, "Decryption\tUnEncrypted headerSize for " << IdString() << " is " << headerSize);

  if ((headerSize + 32) > data.GetSize())         //Make certain packet larger than minimum size.
    return PFalse;

  PTRACE(6, "DATA Raw is " << endl << ::hex << data << ::dec);
  PINDEX encDataSize = data.GetSize() - headerSize;
  PTRACE(4, "Decryption\tEncoded data size is " << encDataSize);
  if ((encDataSize % 16) != 0) {
    PTRACE(2, "Decryption\tData size is not a multiple of 16.. Error. ");    
    return PFalse;
  }

  unsigned char lastblock[16];
  memset(lastblock, 0, 16);
  PBYTEArray working(encDataSize);

  for (PINDEX i = 0; i < encDataSize; i+= 16) {
    AES_decrypt(data.GetPointer() + headerSize + i, working.GetPointer() + i, encryption.AesDecryptKey());
    for (int x = 0; x < 16; x++)
      working[x + i] ^= lastblock[x];
    memcpy(lastblock, data.GetPointer() + headerSize + i, 16);
  }

  PINDEX padding = 16 + (working[15] & 0x0f);
  PTRACE(6, "padding is " << padding);

  PINDEX encryptedSize = encDataSize - padding;
  data.SetSize(encryptedSize + headerSize);

  PTRACE(6, "DATA should have a size of " << data.GetSize());
  PTRACE(6, "UNENCRYPTED DATA is " << endl << ::hex << working << ::dec);

  memcpy(data.GetPointer() + headerSize, working.GetPointer() + padding, encryptedSize);
  PTRACE(6, "Entire frame unencrypted is " << endl << ::hex << data << ::dec);
  return PTrue;
#else
  return PFalse;
#endif
}

PBoolean IAX2Frame::EncryptContents(IAX2Encryption &encryption)
{
  if (!encryption.IsEncrypted())
    return PTrue;

#if OPAL_PTLIB_SSL_AES
  PINDEX headerSize = GetEncryptionOffset();
  PINDEX eDataSize = data.GetSize() - headerSize;
  PINDEX padding = 16 + ((16 - (eDataSize % 16)) & 0x0f);
  PTRACE(6, "Frame\tEncryption, Size of encrypted region is changed from " 
	 << eDataSize << "  to " << (padding + eDataSize));
  
  PBYTEArray working(eDataSize + padding);
  memset(working.GetPointer(), 0, 16);
  working[15] = (BYTE)(0x0f & padding);
  memcpy(working.GetPointer() + padding, data.GetPointer() + headerSize, eDataSize);
  
  PBYTEArray result(headerSize + eDataSize + padding);
  memcpy(result.GetPointer(), data.GetPointer(), headerSize);

  unsigned char curblock[16];
  memset(curblock, 0, 16);
  for (PINDEX i = 0; i < (eDataSize + padding); i+= 16) {
    for (int x = 0; x < 16; x++)
      curblock[x] ^= working[x + i];
    AES_encrypt(curblock, result.GetPointer() + i + headerSize, encryption.AesEncryptKey());
    memcpy(curblock, result.GetPointer() + i + headerSize, 16);
  }

  data = result;
  return PTrue;
#else
  PTRACE(1, "Frame\tEncryption is Flagged on, but AES routines in openssl are not available");
  return PFalse;
#endif
}

PINDEX IAX2Frame::GetEncryptionOffset()
{
  if (isFullFrame)
    return 4;
  
  return 2;
}

////////////////////////////////////////////////////////////////////////////////  

IAX2MiniFrame::IAX2MiniFrame(const IAX2Frame & srcFrame)
  : IAX2Frame(srcFrame)
{
  ZeroAllValues();
  isAudio = (data[0] != 0) || (data[1] != 0);
  isVideo = !isAudio;
  PTRACE(6, "Build this IAX2MiniFrame " << IdString());
}

IAX2MiniFrame::IAX2MiniFrame(IAX2EndPoint &_endpoint)
  : IAX2Frame(_endpoint)
{
  ZeroAllValues();
  PTRACE(6, "Build this IAX2MiniFrame " << IdString());
}

IAX2MiniFrame::IAX2MiniFrame(IAX2Processor * iax2Processor, PBYTEArray &sound, 
		     PBoolean _isAudio, DWORD usersTimeStamp) 
  : IAX2Frame(iax2Processor->GetEndPoint())
{
  isAudio = _isAudio;
  presetTimeStamp = usersTimeStamp;
  InitialiseHeader(iax2Processor);  
  
  PINDEX headerSize = data.GetSize();
  data.SetSize(sound.GetSize() + headerSize);
  memcpy(data.GetPointer() + headerSize, sound.GetPointer(), sound.GetSize());
  PTRACE(6, "Build this IAX2MiniFrame " << IdString());
}

IAX2MiniFrame::~IAX2MiniFrame()
{
  PTRACE(6, "Destroy this IAX2MiniFrame " << IdString());
}

void IAX2MiniFrame::InitialiseHeader(IAX2Processor *iax2Processor)
{
  if (iax2Processor != NULL) {
    remote   = iax2Processor->GetRemoteInfo();
    BuildTimeStamp(iax2Processor->GetCallStartTick());
    SetConnectionToken(iax2Processor->GetCallToken());
  }
  WriteHeader();
}

PBoolean IAX2MiniFrame::ProcessNetworkPacket() 
{
  WORD dataWord;
  Read2Bytes(dataWord);
  timeStamp = dataWord;

  PTRACE(5, "Mini frame, header processed. frame is " 
	 << PString(isAudio ? " audio" : "video"));
  return PTrue;
}

void IAX2MiniFrame::ZeroAllValues()
{
}

void IAX2MiniFrame::AlterTimeStamp(PINDEX newValue)
{
  timeStamp = (newValue & (0xffff << 16)) | (timeStamp & 0xffff);
}

PBoolean IAX2MiniFrame::WriteHeader()
{
  currentWriteIndex = 0;   //Probably not needed, but this makes everything "obvious"
  
  if(IsVideo()) {
    data.SetSize(6);
    Write2Bytes(0);
  } else 
    data.SetSize(4);
  
  Write2Bytes(remote.SourceCallNumber() & 0x7fff);
  Write2Bytes(timeStamp & 0xffff);
  
  return PTrue;
}

void IAX2MiniFrame::PrintOn(ostream & strm) const
{
  strm << "IAX2MiniFrame of " << PString(IsVideo() ? "video" : "audio") << " " 
       << IdString() << " \"" << GetConnectionToken() << "\"  " << endl;
  
  IAX2Frame::PrintOn(strm);
}

BYTE *IAX2MiniFrame::GetMediaDataPointer()
{
  if (IsVideo())
    return data.GetPointer() + 6;
  else
    return data.GetPointer() + 4;
}

PINDEX IAX2MiniFrame::GetMediaDataSize()
{
  int thisSize;
  if (IsVideo()) 
    thisSize = data.GetSize() - 6;
  else
    thisSize = data.GetSize() - 4;
  
  if (thisSize < 0)
    return 0;
  else
    return thisSize;
}

PINDEX IAX2MiniFrame::GetEncryptionOffset() 
{ 
  if (IsAudio())
    return 2; 

  return 4;
}

////////////////////////////////////////////////////////////////////////////////  

IAX2FullFrame::IAX2FullFrame(IAX2EndPoint &_newEndpoint)
  : IAX2Frame(_newEndpoint)
{
  ZeroAllValues();
}

/*This is built from an incoming frame (from the network*/
IAX2FullFrame::IAX2FullFrame(const IAX2Frame & srcFrame)
  : IAX2Frame(srcFrame)
{
  PTRACE(5, "START Constructor for a full frame");
  ZeroAllValues();
  PTRACE(5, "END Constructor for a full frame");
}

IAX2FullFrame::~IAX2FullFrame()
{
  PTRACE(6, "Destructor IAX2FullFrame:: " << IdString());
}

PBoolean IAX2FullFrame::operator*=(IAX2FullFrame & /*other*/)
{
  PAssertAlways("Sorry, IAX2FullFrame comparison operator is Not implemented");
  return PTrue;
}

void IAX2FullFrame::MarkAsResent()
{
  if (data.GetSize() > 2)
    data[2] |= 0x80;
}

void IAX2FullFrame::ZeroAllValues()
{
  subClass = 0;     
  timeStamp = 0;
  sequence.ZeroAllValues();
  canRetransmitFrame = PTrue;
  
  transmissionTimer.SetNotifier(PCREATE_NOTIFIER(OnTransmissionTimeout));
  
  retryDelta = PTimeInterval(minRetryTime);
  retries = maxRetries;
  
  callMustBeActive = PTrue;
  packetResent = PFalse;
  ClearListFlags();
  
  isFullFrame = PTrue;
  isAckFrame = PFalse;
}

void IAX2FullFrame::ClearListFlags()
{
  deleteFrameNow = PFalse;
  sendFrameNow   = PFalse;
}

PBoolean IAX2FullFrame::TransmitPacket(PUDPSocket &sock)
{
  PTRACE(6, "Send network packet on " << IdString() << " " << connectionToken);
  if (packetResent) {
    MarkAsResent();      /* Set Retry flag.*/
  }
  
  if (retries == P_MAX_INDEX) {
    PTRACE(4, "Retries count is now negative on. " << IdString());
    return PFalse;    //Give up on this packet, it has exceeded the allowed number of retries.
  }
  
  PTRACE(6, "Start timer running for " << IdString() << connectionToken);
  transmissionTimer.SetInterval(retryDelta.GetMilliSeconds());
  transmissionTimer.Reset();   //*This causes the timer to start running.
  ClearListFlags();
  
  return IAX2Frame::TransmitPacket(sock);
}

PBoolean IAX2FullFrame::ProcessNetworkPacket()
{
  PTRACE(5, "ProcessNetworkPacket - read the frame header");
  if (data.GetSize() < 12) {
    PTRACE(2, "Incoming full frame is undersize - should have 12 bytes, but only read " << data.GetSize());
    return PFalse;
  }
  
  Read4Bytes(timeStamp);
  PTRACE(5, "Remote timestamp is " << timeStamp  << " milliseconds");
  
  BYTE a = 0;
  Read1Byte(a);
  sequence.SetOutSeqNo(a);
  Read1Byte(a);
  sequence.SetInSeqNo(a);
  PTRACE(6, "Sequence is " << sequence.AsString());
  
  Read1Byte(a);

  if ((a >= numFrameTypes) || (a == undefType)) {
    PTRACE(3, "Incoming packet has invalid frame type of " << a);
    return PFalse;
  }
  
  frameType = (IAX2FrameType)a;
  isAudio = frameType == voiceType;
  isVideo = frameType == videoType;
  
  /*Do subclass value (which might be compressed) */
  Read1Byte(a);
  
  UnCompressSubClass(a);
  isAckFrame = (subClass == IAX2FullFrameProtocol::cmdAck) && (frameType == iax2ProtocolType);     
  return PTrue;
}

PBoolean IAX2FullFrame::IsPingFrame()
{
  return (subClass == IAX2FullFrameProtocol::cmdPing) && (frameType == iax2ProtocolType);
}

PBoolean IAX2FullFrame::IsNewFrame()
{
  return (subClass == IAX2FullFrameProtocol::cmdNew) && (frameType == iax2ProtocolType);
}

PBoolean IAX2FullFrame::IsLagRqFrame()
{
  return (subClass == IAX2FullFrameProtocol::cmdLagRq) && (frameType == iax2ProtocolType);
}

PBoolean IAX2FullFrame::IsLagRpFrame()
{
  return (subClass == IAX2FullFrameProtocol::cmdLagRp) && (frameType == iax2ProtocolType);     
}

PBoolean IAX2FullFrame::IsPongFrame()
{
  return (subClass == IAX2FullFrameProtocol::cmdPong) && (frameType == iax2ProtocolType);
}

PBoolean IAX2FullFrame::IsAuthReqFrame()
{
  return (subClass == IAX2FullFrameProtocol::cmdAuthReq) && (frameType == iax2ProtocolType);
}

PBoolean IAX2FullFrame::IsVnakFrame()
{
  return (subClass == IAX2FullFrameProtocol::cmdVnak) && (frameType == iax2ProtocolType);
}

PBoolean IAX2FullFrame::IsRegReqFrame()
{
  return (subClass == IAX2FullFrameProtocol::cmdRegReq) && (frameType == iax2ProtocolType);
}

PBoolean IAX2FullFrame::IsRegAuthFrame()
{
  return (subClass == IAX2FullFrameProtocol::cmdRegAuth) && (frameType == iax2ProtocolType);
}

PBoolean IAX2FullFrame::IsRegAckFrame()
{
  return (subClass == IAX2FullFrameProtocol::cmdRegAck) && (frameType == iax2ProtocolType);
}

PBoolean IAX2FullFrame::IsRegRelFrame()
{
  return (subClass == IAX2FullFrameProtocol::cmdRegRel) && (frameType == iax2ProtocolType);
}

PBoolean IAX2FullFrame::IsRegRejFrame()
{
  return (subClass == IAX2FullFrameProtocol::cmdRegRej) && (frameType == iax2ProtocolType);
}

PBoolean IAX2FullFrame::IsHangupFrame()
{
  return (subClass == IAX2FullFrameProtocol::cmdHangup) && (frameType == iax2ProtocolType);
}

PBoolean IAX2FullFrame::FrameIncrementsInSeqNo()
{
  if (frameType != iax2ProtocolType) {
    PTRACE(5, "SeqNos\tFrameType is not iaxProtocol, so we do increment inseqno. FrameType is " << frameType);
    return PTrue;
  }

  IAX2FullFrameProtocol::ProtocolSc cmdType = (IAX2FullFrameProtocol::ProtocolSc)subClass;
  PTRACE(5, "SeqNos\tThe cmd type (or subclass of IAX2FullFrameProtocol) is " << cmdType);
  if ((cmdType == IAX2FullFrameProtocol::cmdAck)     ||
      //  (cmdType == IAX2FullFrameProtocol::cmdLagRq)   ||
      //  (cmdType == IAX2FullFrameProtocol::cmdLagRp)   ||
      //  (cmdType == IAX2FullFrameProtocol::cmdAuthReq) ||
      //  (cmdType == IAX2FullFrameProtocol::cmdAuthRep) ||
      (cmdType == IAX2FullFrameProtocol::cmdVnak))   {
    PTRACE(3, "SeqNos\tThis is a iaxProtocol cmd type that does not increment inseqno");
    return PFalse;
  } else {
    PTRACE(5, "SeqNos\tThis is a iaxProtocol cmd type that increments inseqno");
  }
  return PTrue;
}

void IAX2FullFrame::UnCompressSubClass(BYTE a)
{
  if (a & 0x80) {
    if (a == 0xff)
      subClass = -1;
    else
      subClass = 1 << (a & 0x1f);
  } else
    subClass = a;
}

int IAX2FullFrame::CompressSubClass()
{
  if (subClass < 0x80)
    return subClass;
  
  for(PINDEX i = 0; i <  0x1f; i++) {
    if (subClass & (1 << i)) 
      return i | 0x80;
  }
  
  return -1;
}

PBoolean IAX2FullFrame::WriteHeader()
{
  data.SetSize(12);
  PTRACE(6, "Write a source call number of " << remote.SourceCallNumber());
  Write2Bytes(remote.SourceCallNumber() + 0x8000);
  PTRACE(6, "Write a dest call number of " << remote.DestCallNumber());
  Write2Bytes(remote.DestCallNumber() + (packetResent ? 0x8000 : 0));
  
  PTRACE(6, "Write a timestamp of " << timeStamp);
  Write4Bytes(timeStamp);
  
  PTRACE(6, "Write in seq no " << sequence.InSeqNo() << " and out seq no of " << sequence.OutSeqNo());
  Write1Byte(sequence.OutSeqNo());
  Write1Byte(sequence.InSeqNo());
  
  PTRACE(6, "FrameType is " << ((int)GetFullFrameType()));
  Write1Byte(GetFullFrameType());
  
  int a = CompressSubClass();
  if (a < 0)
    Write1Byte(0xff);
  else
    Write1Byte((BYTE)a);     
  PTRACE(6, "Comppressed sub class is " << a << " from " << subClass);
  
  return PTrue;
}

void IAX2FullFrame::MarkVnakSendNow()
{
  transmissionTimer.Stop();
  sendFrameNow = PTrue;
  deleteFrameNow = PFalse;    
  retryDelta = PTimeInterval(minRetryTime);
  retries = maxRetries;
}

void IAX2FullFrame::MarkDeleteNow()
{
  PTRACE(5, "MarkDeleteNow() method on " << IdString());
  transmissionTimer.Stop();
  deleteFrameNow = PTrue;
  retries = P_MAX_INDEX;
}

void IAX2FullFrame::OnTransmissionTimeout(PTimer &, INT)
{
  PTRACE(4, "Has had a TX timeout " << IdString() << " " << connectionToken);
  retryDelta = 4 * retryDelta.GetMilliSeconds();
  if (retryDelta > maxRetryTime)
    retryDelta = maxRetryTime;
  
  packetResent = PTrue;
  if ((retries == P_MAX_INDEX) || (retries == 0)) {
    retries = P_MAX_INDEX;
    PTRACE(5, "Retries are " << PString(retries) 
	   << " NowMarkDeleteNow " << IdString());
    MarkDeleteNow();
  } else {
    retries--;
    sendFrameNow = PTrue;
    PTRACE(5, "Tx timeout, so Mark as Send now " << IdString() << " " << connectionToken);
  }

  endpoint.transmitter->ProcessLists();
}

PString IAX2FullFrame::GetFullFrameName() const
{
  switch(frameType)  {
      case undefType       : return PString("(0?)      ");
      case dtmfType        : return PString("Dtmf      ");
      case voiceType       : return PString("Voice     ");
      case videoType       : return PString("Video     ");
      case controlType     : return PString("Session   ");
      case nullType        : return PString("Null      ");
      case iax2ProtocolType: return PString("Protocol  ");
      case textType        : return PString("Text      ");
      case imageType       : return PString("Image     ");
      case htmlType        : return PString("Html      ");
      case cngType         : return PString("Cng       ");
      case numFrameTypes   : return PString("# F types ");
  }
  
  return PString("Frame name is undefined for value of ") + PString(frameType);
}

BYTE *IAX2FullFrame::GetMediaDataPointer()
{
  return data.GetPointer() + 12;
}

PINDEX IAX2FullFrame::GetMediaDataSize()
{
  return data.GetSize() - 12;
}

void IAX2FullFrame::InitialiseHeader(IAX2Processor *iax2Processor)
{
  if (iax2Processor != NULL) {
    SetConnectionToken(iax2Processor->GetCallToken());
    BuildTimeStamp(iax2Processor->GetCallStartTick());    
    remote   = iax2Processor->GetRemoteInfo();
  }
  PTRACE(5, "source timestamp is " << timeStamp);
  frameType = (IAX2FrameType)GetFullFrameType();  
  WriteHeader();
}

void IAX2FullFrame::PrintOn(ostream & strm) const
{
  strm << IdString() << " ++  " << GetFullFrameName() << " -- " 
       << GetSubClassName() << " \"" << connectionToken << "\"" << endl
       << remote << endl;
}

void IAX2FullFrame::ModifyFrameHeaderSequenceNumbers(PINDEX inNo, PINDEX outNo)
{
  data[8] = (BYTE) (outNo & 0xff);
  data[9] = (BYTE) (inNo & 0xff);
  GetSequenceInfo().SetInOutSeqNo(inNo, outNo);
}

void IAX2FullFrame::ModifyFrameTimeStamp(PINDEX newTimeStamp)
{
  timeStamp = newTimeStamp;
  PINDEX oldWriteIndex = currentWriteIndex;
  currentWriteIndex = 4;
  Write4Bytes(timeStamp);
  currentWriteIndex = oldWriteIndex;
}
////////////////////////////////////////////////////////////////////////////////  

IAX2FullFrameDtmf::IAX2FullFrameDtmf(const IAX2Frame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
}

IAX2FullFrameDtmf::IAX2FullFrameDtmf(const IAX2FullFrame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
}

IAX2FullFrameDtmf::IAX2FullFrameDtmf(IAX2Processor *iax2Processor, PString  subClassValue)
  : IAX2FullFrame(iax2Processor->GetEndPoint())
{
  SetSubClass(subClassValue.ToUpper()[0]);
  InitialiseHeader(iax2Processor);
}

IAX2FullFrameDtmf::IAX2FullFrameDtmf(IAX2Processor *iax2Processor, char  subClassValue)
  : IAX2FullFrame(iax2Processor->GetEndPoint())
{
  SetSubClass(toupper(subClassValue));
  InitialiseHeader(iax2Processor);
}

PString IAX2FullFrameDtmf::GetSubClassName() const {
  switch (GetSubClass()) {    
  case dtmf0:    return PString("0"); 
  case dtmf1:    return PString("1"); 
  case dtmf2:    return PString("2"); 
  case dtmf3:    return PString("3"); 
  case dtmf4:    return PString("4"); 
  case dtmf5:    return PString("5"); 
  case dtmf6:    return PString("6"); 
  case dtmf7:    return PString("7"); 
  case dtmf8:    return PString("8"); 
  case dtmf9:    return PString("9"); 
  case dtmfA:    return PString("A"); 
  case dtmfB:    return PString("B"); 
  case dtmfC:    return PString("C"); 
  case dtmfD:    return PString("D"); 
  case dtmfStar: return PString("*"); 
  case dtmfHash: return PString("#"); 
  };
  return PString("Undefined dtmf subclass value of ") + PString(GetSubClass());
}

////////////////////////////////////////////////////////////////////////////////
IAX2FullFrameVoice::IAX2FullFrameVoice(const IAX2Frame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
  PTRACE(6, "Construct a full frame voice from a Frame" << IdString());
}

IAX2FullFrameVoice::IAX2FullFrameVoice(const IAX2FullFrame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
  PTRACE(6, "Construct a full frame voice from a IAX2FullFrame" << IdString());
}

IAX2FullFrameVoice::IAX2FullFrameVoice(IAX2CallProcessor *iax2Processor, PBYTEArray & sound, PINDEX usersTimeStamp)
  : IAX2FullFrame(iax2Processor->GetEndPoint())
{
  if (iax2Processor != NULL) {
    SetSubClass((PINDEX)iax2Processor->GetSelectedCodec());
  }

  presetTimeStamp = usersTimeStamp;
  InitialiseHeader(iax2Processor);  

  PINDEX headerSize = data.GetSize();
  data.SetSize(sound.GetSize() + headerSize);
  memcpy(data.GetPointer() + headerSize, sound.GetPointer(), sound.GetSize());
  PTRACE(6, "Construct a full frame voice from a processor, sound, and codec" << IdString());
}


IAX2FullFrameVoice::~IAX2FullFrameVoice()
{
  PTRACE(6, "Destroy this IAX2FullFrameVoice" << IdString());
}

PString IAX2FullFrameVoice::GetSubClassName(unsigned int testValue) 
{
  switch (testValue) {
  case g7231:     return PString("G.723.1");
  case gsm:       return PString("GSM-06.10");
  case g711ulaw:  return PString("G.711-uLaw-64k");
  case g711alaw:  return PString("G.711-ALaw-64k");
  case mp3:       return PString("mp3");
  case adpcm:     return PString("adpcm");
  case pcm:       return PString("pcm");
  case lpc10:     return PString("LPC-10");
  case g729:      return PString("G.729");
  case speex:     return PString("speex");
  case ilbc:      return PString("iLBC-13k3");
  default: ;
  };
  
  PStringStream res;
  res << "The value 0x" << ::hex << testValue << ::dec << " could not be identified as a codec";
  return res;
}

unsigned short IAX2FullFrameVoice::OpalNameToIax2Value(const PString opalName)
{
  if (opalName.Find("uLaw") != P_MAX_INDEX) {
    return g711ulaw;
  }
  
  if (opalName.Find("ALaw") != P_MAX_INDEX) {
    return  g711alaw;
  }
  
  if (opalName.Find("GSM-06.10") != P_MAX_INDEX) {
    return gsm;
  }

  if (opalName.Find("iLBC-13k3") != P_MAX_INDEX) {
    return ilbc; 
    }
  PTRACE(6, "Codec " << opalName << " is not supported in IAX2");
  return 0;
}

PString IAX2FullFrameVoice::GetSubClassName() const 
{
  return GetSubClassName(GetSubClass());
}

PString IAX2FullFrameVoice::GetOpalNameOfCodec(PINDEX testValue)
{
  switch (testValue) {
  case g7231:     return PString("G.723.1");
  case gsm:       return PString("GSM-06.10");
  case g711ulaw:  return PString("G.711-uLaw-64k");
  case g711alaw:  return PString("G.711-ALaw-64k");
  case mp3:       return PString("mp3");
  case adpcm:     return PString("adpcm");
  case pcm:       return PString("Linear-16-Mono-8kHz");
  case lpc10:     return PString("LPC10");
  case g729:      return PString("G.729");
  case speex:     return PString("speex");
  case ilbc:      return PString("ilbc");
  default: ;
  };
  
  PStringStream res;
  res << "The value 0x" << ::hex << testValue << ::dec << " could not be identified as a codec";
  return res;
}

////////////////////////////////////////////////////////////////////////////////

IAX2FullFrameVideo::IAX2FullFrameVideo(const IAX2Frame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
}

IAX2FullFrameVideo::IAX2FullFrameVideo(const IAX2FullFrame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
}

PString IAX2FullFrameVideo::GetSubClassName() const
{
  switch (GetSubClass()) {
  case jpeg:  return PString("jpeg");
  case png:  return PString("png");
  case h261:  return PString("H.261");
  case h263:  return PString("H.263");
  };
  return PString("Undefined IAX2FullFrameVideo subclass value of ") + PString(GetSubClass());
}
////////////////////////////////////////////////////////////////////////////////

IAX2FullFrameSessionControl::IAX2FullFrameSessionControl(const IAX2Frame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
}

IAX2FullFrameSessionControl::IAX2FullFrameSessionControl(const IAX2FullFrame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
}

IAX2FullFrameSessionControl::IAX2FullFrameSessionControl(IAX2Processor *iax2Processor,
						 SessionSc session)  
  :  IAX2FullFrame(iax2Processor->GetEndPoint())          
{
  SetSubClass((PINDEX)session);
  isAckFrame = PFalse;
  InitialiseHeader(iax2Processor);
  callMustBeActive = PTrue;
}

IAX2FullFrameSessionControl::IAX2FullFrameSessionControl(IAX2Processor *iax2Processor,
						 PINDEX subClassValue)   
  :  IAX2FullFrame(iax2Processor->GetEndPoint())          
{
  SetSubClass(subClassValue);
  isAckFrame = PFalse;
  InitialiseHeader(iax2Processor);
  callMustBeActive = PTrue;
}

PString IAX2FullFrameSessionControl::GetSubClassName() const {
  switch (GetSubClass()) {
  case hangup:          return PString("hangup");
  case ring:            return PString("ring");
  case ringing:         return PString("ringing");
  case answer:          return PString("answer");
  case busy:            return PString("busy");
  case tkoffhk:         return PString("tkoffhk");
  case offhook:         return PString("offhook");
  case congestion:      return PString("congestion");
  case flashhook:       return PString("flashhook");
  case wink:            return PString("wink");
  case option:          return PString("option");
  case keyRadio:        return PString("keyRadio");
  case unkeyRadio:      return PString("unkeyRadio");
  case callProgress:    return PString("callProgress");
  case callProceeding:  return PString("callProceeding");
  };
  
  return PString("Undefined IAX2FullFrameSessionControl subclass value of ") 
    + PString(GetSubClass());
}
////////////////////////////////////////////////////////////////////////////////

IAX2FullFrameNull::IAX2FullFrameNull(const IAX2Frame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
}

IAX2FullFrameNull::IAX2FullFrameNull(const IAX2FullFrame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
}

////////////////////////////////////////////////////////////////////////////////

IAX2FullFrameProtocol::IAX2FullFrameProtocol(IAX2Processor *iax2Processor, PINDEX subClassValue, ConnectionRequired needCon)
  :  IAX2FullFrame(iax2Processor->GetEndPoint())
{
  SetSubClass(subClassValue);
  isAckFrame = (subClassValue == cmdAck);
  if (isAckFrame) {
    PTRACE(5, "Sending an ack frame now");
  }
  InitialiseHeader(iax2Processor);
  callMustBeActive = (needCon == callActive);
  PTRACE(5, "Construct a fullframeprotocol from a processor, subclass value    and a connectionrequired. " << IdString());

}

IAX2FullFrameProtocol::IAX2FullFrameProtocol(IAX2Processor *iax2Processor,  ProtocolSc  subClassValue, ConnectionRequired needCon)
  : IAX2FullFrame(iax2Processor->GetEndPoint())
{
  SetSubClass(subClassValue);
  isAckFrame = (subClassValue == cmdAck);
  InitialiseHeader(iax2Processor);
  callMustBeActive = (needCon == callActive);

  PTRACE(5, "Construct a fullframeprotocol from a processor subclass value and connection required " << IdString());
}

IAX2FullFrameProtocol::IAX2FullFrameProtocol(IAX2Processor *iax2Processor,  ProtocolSc  subClassValue, IAX2FullFrame *inReplyTo, ConnectionRequired needCon)
  : IAX2FullFrame(iax2Processor->GetEndPoint())
{
  SetSubClass(subClassValue);     
  timeStamp = inReplyTo->GetTimeStamp();     
  isAckFrame = (subClassValue == cmdAck);
  if (isAckFrame) {
    sequence.SetAckSequenceInfo(inReplyTo->GetSequenceInfo());
  }
  if (iax2Processor == NULL) {
    IAX2Remote rem = inReplyTo->GetRemoteInfo();
    remote = rem;
    ///	  remote.SetSourceCallNumber(rem.GetDestCallNumber());
    ///       remote.SetDestCallNumber(rem.GetSourceCallNumber());	  
  } else {
    remote = iax2Processor->GetRemoteInfo();
    SetConnectionToken(iax2Processor->GetCallToken());
  }
  //     processor->MassageSequenceForReply(sequence, inReplyTo->GetSequenceInfo());
  frameType = iax2ProtocolType;       
  callMustBeActive = (needCon == callActive);
  WriteHeader();
  PTRACE(5, "Construct a fullframeprotocol from a  processor, subclass value and a connection required" << IdString());
}

IAX2FullFrameProtocol::IAX2FullFrameProtocol(const IAX2Frame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
  ReadInformationElements();
  PTRACE(5, "Construct a fullframeprotocol from a Frame" << IdString());
}

IAX2FullFrameProtocol::IAX2FullFrameProtocol(const IAX2FullFrame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
  ReadInformationElements();
  PTRACE(5, "Construct a fullframeprotocol from a Full Frame" << IdString());
}

IAX2FullFrameProtocol::~IAX2FullFrameProtocol()
{
  ieElements.AllowDeleteObjects(PTrue);
  PTRACE(6, "Destroy this IAX2FullFrameProtocol(" << GetSubClassName() << ") " << IdString());
}

void IAX2FullFrameProtocol::SetRetransmissionRequired()
{
  if (GetSubClass() == IAX2FullFrameProtocol::cmdAck)
    canRetransmitFrame = PFalse;
  
  if (GetSubClass() == IAX2FullFrameProtocol::cmdLagRp)
    canRetransmitFrame = PFalse;
  
  if (GetSubClass() == IAX2FullFrameProtocol::cmdPong)
    canRetransmitFrame = PFalse;
}

void IAX2FullFrameProtocol::WriteIeAsBinaryData()
{
  PTRACE(6, "Write the IE data (" << ieElements.GetSize() 
	 << " elements) as binary data to frame");
  PINDEX headerSize = data.GetSize();
  data.SetSize(headerSize + ieElements.GetBinaryDataSize());
  
  for(PINDEX i = 0; i < ieElements.GetSize(); i++) {
    PTRACE(5, "Append to outgoing frame " << *ieElements.GetIeAt(i));
    ieElements.GetIeAt(i)->WriteBinary(data.GetPointer(), headerSize);
  }    
}

PBoolean IAX2FullFrameProtocol::ReadInformationElements()
{
  IAX2Ie *elem = NULL;
  
  while (GetUnReadBytes() >= 2) {
    BYTE thisType = 0, thisLength = 0;
    Read1Byte(thisType);
    Read1Byte(thisLength);
    if (thisLength <= GetUnReadBytes()){
      elem = IAX2Ie::BuildInformationElement(thisType, thisLength, data.GetPointer() + currentReadIndex);
      currentReadIndex += thisLength;
      if (elem != NULL)
	if (elem->IsValid()) {
	  ieElements.Append(elem);
	  //	  PTRACE(3, "Read information element " << *elem);
	}
    } else {
      PTRACE(6, "Unread bytes is " << GetUnReadBytes() << " This length is " << thisLength);
      break;
    }	  
  }
  
  if (elem == NULL)
    return PFalse;
  
  if (!elem->IsValid())
    return PFalse;
  
  return GetUnReadBytes() == 0;
}

void IAX2FullFrameProtocol::GetRemoteCapability(unsigned int & capability, unsigned int & preferred)
{
  capability = 0;
  preferred = 0;
  IAX2Ie * p;
  PINDEX i = 0;
  for(;;) {
    p = GetIeAt(i);
    if (p == NULL)
      break;
    
    i++;
    if (p->IsValid()) {
      if (PIsDescendant(p, IAX2IeCapability)) {
	capability = ((IAX2IeCapability *)p)->ReadData();
	PTRACE(5, "IAX2FullFrameProtocol\tCapability codecs are " << capability);
      }
      if (PIsDescendant(p, IAX2IeFormat)) {
	preferred = ((IAX2IeFormat *)p)->ReadData();
	PTRACE(4, "IAX2FullFrameProtocol\tPreferred codec is " << preferred);
      }
    } else {
      PTRACE(3, "Invalid data in IE. ");
    }
  }
}

void IAX2FullFrameProtocol::CopyDataFromIeListTo(IAX2IeData &res)
{
  IAX2Ie * p;
  PINDEX i = 0;
  for(;;) {
    p = GetIeAt(i);
    if (p == NULL)
      break;
    
    i++;
    PTRACE(4, "From IAX2FullFrameProtocol, handle IAX2Ie of type " << *p);
    if (p->IsValid()) 
      p->StoreDataIn(res);
    else {
      PTRACE(3, "Invalid data in IE. " << *p);
    }
  }
}

PString IAX2FullFrameProtocol::GetSubClassName() const
{
  return GetSubClassName(GetSubClass());
}

PString IAX2FullFrameProtocol::GetSubClassName(PINDEX t)
{
  switch (t) {
  case cmdNew:        return PString("new");
  case cmdPing:       return PString("ping");
  case cmdPong:       return PString("pong");
  case cmdAck:        return PString("ack");
  case cmdHangup:     return PString("hangup");
  case cmdReject:     return PString("reject");
  case cmdAccept:     return PString("accept");
  case cmdAuthReq:    return PString("authreq");
  case cmdAuthRep:    return PString("authrep");
  case cmdInval:      return PString("inval");
  case cmdLagRq:      return PString("lagrq");
  case cmdLagRp:      return PString("lagrp");
  case cmdRegReq:     return PString("regreq");
  case cmdRegAuth:    return PString("regauth");
  case cmdRegAck:     return PString("regack");
  case cmdRegRej:     return PString("regrej");
  case cmdRegRel:     return PString("regrel");
  case cmdVnak:       return PString("vnak");
  case cmdDpReq:      return PString("dpreq");
  case cmdDpRep:      return PString("dprep");
  case cmdDial:       return PString("dial");
  case cmdTxreq:      return PString("txreq");
  case cmdTxcnt:      return PString("txcnt");
  case cmdTxacc:      return PString("txacc");
  case cmdTxready:    return PString("txready");
  case cmdTxrel:      return PString("txrel");
  case cmdTxrej:      return PString("txrej");
  case cmdQuelch:     return PString("quelch");
  case cmdUnquelch:   return PString("unquelch");
  case cmdPoke:       return PString("poke");
  case cmdPage:       return PString("page");
  case cmdMwi:        return PString("mwi");
  case cmdUnsupport:  return PString("unsupport");
  case cmdTransfer:   return PString("transfer");
  case cmdProvision:  return PString("provision");
  case cmdFwDownl:    return PString("fwDownl");
  case cmdFwData:     return PString("fwData");
  };
  return PString("Undefined FullFrameProtocol subclass value of ") + PString(t);
}

#if PTRACING
ostream & operator << (ostream & o, IAX2FullFrameProtocol::ProtocolSc t)
{
  PString answer = IAX2FullFrameProtocol::GetSubClassName(t);
  o << answer;
  return o;
}
#endif

void IAX2FullFrameProtocol::PrintOn(ostream & strm) const
{
  strm << "IAX2FullFrameProtocol(" << GetSubClassName() << ") " 
       << IdString() << " -- " 
       << " \"" << connectionToken << "\"" << endl
       << remote << endl;
}
////////////////////////////////////////////////////////////////////////////////

IAX2FullFrameText::IAX2FullFrameText(const IAX2Frame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
   if (GetMediaDataSize() > 0)
     internalText = PString((const char *)GetMediaDataPointer(),
			    GetMediaDataSize());

}

IAX2FullFrameText::IAX2FullFrameText(const IAX2FullFrame & srcFrame)
  : IAX2FullFrame(srcFrame)
{ 
  if (GetMediaDataSize() > 0)
    internalText = PString((const char *)GetMediaDataPointer(),
			   GetMediaDataSize());

}

PString IAX2FullFrameText::GetSubClassName() const 
{
  return PString("IAX2FullFrameText never has a valid sub class");
}

IAX2FullFrameText::IAX2FullFrameText(IAX2Processor *iaxProcessor, const PString&  text )
  : IAX2FullFrame(iaxProcessor->GetEndPoint())
{
//  presetTimeStamp = usersTimeStamp;
  InitialiseHeader(iaxProcessor);

  internalText = text;

  PINDEX headerSize = data.GetSize();
  data.SetSize(text.GetLength() + headerSize);
  memcpy(data.GetPointer() + headerSize, 
	 internalText.GetPointer(), internalText.GetLength());

  PTRACE(4, "Construct a full frame text" << IdString() << " for text " << text);
}

PString IAX2FullFrameText::GetTextString() const
{
  return internalText;
}

////////////////////////////////////////////////////////////////////////////////

IAX2FullFrameImage::IAX2FullFrameImage(const IAX2Frame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
}

IAX2FullFrameImage::IAX2FullFrameImage(const IAX2FullFrame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
}

PString IAX2FullFrameImage::GetSubClassName() const 
{
  return PString("IAX2FullFrameImage never has a valid sub class");
}
////////////////////////////////////////////////////////////////////////////////

IAX2FullFrameHtml::IAX2FullFrameHtml(const IAX2Frame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
}

IAX2FullFrameHtml::IAX2FullFrameHtml(const IAX2FullFrame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
}

PString IAX2FullFrameHtml::GetSubClassName() const 
{
  return PString("IAX2FullFrameHtml has a sub class of ") + PString(GetSubClass());
}

////////////////////////////////////////////////////////////////////////////////

IAX2FullFrameCng::IAX2FullFrameCng(const IAX2Frame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
}

IAX2FullFrameCng::IAX2FullFrameCng(const IAX2FullFrame & srcFrame)
  : IAX2FullFrame(srcFrame)
{
}

PString IAX2FullFrameCng::GetSubClassName() const 
{
  return PString("IAX2FullFrameCng has a sub class of ") + PString(GetSubClass());
}


////////////////////////////////////////////////////////////////////////////////

IAX2FrameList::~IAX2FrameList()
{   
}

void IAX2FrameList::ReportList(PString & answer)
{
  PStringStream reply;
  {
    PWaitAndSignal m(mutex);
    
    for(PINDEX i = 0; i < PAbstractList::GetSize(); i++) {
       IAX2Frame *frame = (IAX2Frame *)GetAt(i);
       reply << "     #" << (i + 1) << " of " 
	     << PAbstractList::GetSize() << "   "
	     << frame->GetConnectionToken() << " " 
	     << frame->GetTimeStamp();
       if (frame->IsFullFrame()) {
	 IAX2FullFrame *ff = (IAX2FullFrame *)frame;
	 reply << " " << ff->GetSequenceInfo().AsString() << " " 
	       << ff->GetFullFrameName() << endl;
       }
    }
  }
  answer = reply;
}

void IAX2FrameList::AddNewFrame(IAX2Frame *newFrame)
{
  if (newFrame == NULL)
    return;
  PTRACE(5, "AddNewFrame " << newFrame->IdString());
  PWaitAndSignal m(mutex);
  PAbstractList::Append(newFrame);
}

void IAX2FrameList::GrabContents(IAX2FrameList &src)
{
  IAX2Frame *current;
  do {
    current = src.GetLastFrame();
    AddNewFrame(current);
  } while (current != NULL);
}

IAX2Frame *IAX2FrameList::GetLastFrame()
{
  PWaitAndSignal m(mutex);
  PINDEX elems = GetEntries();
  if (elems ==  0) {
    return NULL;
  }
  
  return (IAX2Frame *)PAbstractList::RemoveAt(0);
}

void IAX2FrameList::DeleteMatchingSendFrame(IAX2FullFrame *reply)
{
  PINDEX currentIndex;
  IAX2FullFrame *sent;

  PWaitAndSignal m(mutex);
  //Look for a frame that has been sent, which is waiting for a reply/ack.
  PTRACE(5, "ID# Delete matchingSendFrame start, test on " 
	 << reply->IdString());

  for (PINDEX i = 0; i < GetEntries(); i++) {
    sent = NULL;
    IAX2Frame *frame = (IAX2Frame *)GetAt(i);
    if (frame == NULL)
      continue;
    
    PTRACE(5, "ID#DeleteMatching " << frame->IdString());
    if (!frame->IsFullFrame())
      continue;

    sent = (IAX2FullFrame *)frame;
    currentIndex = i; /* If a match is found, we delete this one */

    if (sent->DeleteFrameNow()) {
      // Skip this frame, as it is marked, delete now
      continue;
    }
    
    if (!(sent->GetRemoteInfo() *= reply->GetRemoteInfo())) {
      PTRACE(5, "mismatch in remote info");
      continue;
    } 

    if (sent->IsNewFrame() &&
	reply->GetSequenceInfo().IsFirstReplyFrame()) {
      PTRACE(5, "Have a match on a new frame we sent out");
      goto foundMatch;
    }

        
    if (sent->IsRegReqFrame() && 
        (reply->IsRegAckFrame() || reply->IsRegAuthFrame() || reply->IsRegRejFrame())) {
      PTRACE(5, "have read a RegAck, RegAuth or RegRej packet for a RegReq frame we have sent, delete this RegReq");
      PTRACE(5, "reg type frame, so MarkDeleteNow on " << sent->IdString());
      goto foundMatch;
    }
    
    if (sent->IsRegRelFrame() && 
        (reply->IsRegAckFrame() || reply->IsRegAuthFrame() || reply->IsRegRejFrame())) {
      PTRACE(5, "have read a RegAck, RegAuth or RegRej packet for a RegRel frame we have sent, delete this RegRel");
      PTRACE(5, "reg rel/authoframe, so MarkDeleteNow on " << sent->IdString());
      goto foundMatch;
    }
   
    if (sent->GetTimeStamp() != reply->GetTimeStamp()) {
      PTRACE(5, "Time stamps differ, so give up on the test" << sent->IdString());
      continue;
    } else {
      PTRACE(5, "Time stamps are the same, so check in seqno vs oseqno " << sent->IdString());
    }

    PTRACE(5, "SeqNos\tSent is " << sent->GetSequenceInfo().OutSeqNo() 
	   << " " << sent->GetSequenceInfo().InSeqNo());
    PTRACE(5, "SeqNos\tRepl is " << reply->GetSequenceInfo().OutSeqNo() 
	   << " " << reply->GetSequenceInfo().InSeqNo());

    if (reply->IsLagRpFrame() && sent->IsLagRqFrame()) {
      PTRACE(5, "have read a LagRp packet for a LagRq frame  we have sent, delete this LagRq " 
	     << sent->IdString());
      PTRACE(5, "LAG frame, so MarkDeleteNow on " << sent->IdString());
      goto foundMatch;
    }

    if (reply->IsPongFrame() && sent->IsPingFrame()) {
      PTRACE(5, "have read a Pong packet for a PING frame  we have sent: delete the Pong " 
	     << sent->IdString());
      PTRACE(5, "PONG frame, so MarkDeleteNow on " << sent->IdString());
      goto foundMatch;
    }

    if (sent->GetSequenceInfo().InSeqNo() == reply->GetSequenceInfo().OutSeqNo()) {
      PTRACE(5, "Timestamp, and inseqno matches oseqno " << sent->IdString());
      if (reply->IsAckFrame()) {
        PTRACE(5, "have read an ack packet for one we have sent, so delete this one " << sent->IdString());
	PTRACE(5, "ack for existing frame, MarkDeleteNow " << sent->IdString());
        goto foundMatch;
      }    
    } else {
      PTRACE(5, "No match:: sent=" << sent->IdString() << " and reply=" << reply->IdString() 
	     << PString(reply->IsAckFrame() ? "reply is ack frame " : "reply is not ack frame ")
	     << PString("Sequence numbers are:: sentIn" ) 
	     << sent->GetSequenceInfo().InSeqNo() << "  rcvdOut" << reply->GetSequenceInfo().OutSeqNo());
    }	
	  
    PTRACE(5, " sequence " << sent->GetSequenceInfo().OutSeqNo() 
	   << " and " << reply->GetSequenceInfo().InSeqNo() << " are different");
    
  }
  // No match found, so no sent frame will be deleted 
  return;

 foundMatch:

  delete sent;
  RemoveAt(currentIndex);
  return;
}  

void IAX2FrameList::SendVnakRequestedFrames(IAX2FullFrameProtocol &src)
{
  PINDEX srcOutSeqNo = src.GetSequenceInfo().OutSeqNo();
  PWaitAndSignal m(mutex);
  PTRACE(4, "Look for a frame that has been sent, waiting to be acked etc, that matches the supplied Vnak frame");
  
  for (PINDEX i = 0; i < GetEntries(); i++) {
    IAX2Frame *frame = (IAX2Frame *)GetAt(i);
    if (frame == NULL)
      continue;
    
    if (!frame->IsFullFrame())
      continue;

    IAX2FullFrame *sent = (IAX2FullFrame *)frame;

    if (sent->DeleteFrameNow()) {
      PTRACE(4, "Skip this frame, as it is marked, delete now" << sent->IdString());
      continue;
    }
    
    if (!(sent->GetRemoteInfo() *= src.GetRemoteInfo())) {
      PTRACE(5, "mismatch in remote info");
      continue;
    }

    if (sent->GetSequenceInfo().OutSeqNo() <= srcOutSeqNo) {
      sent->MarkVnakSendNow();
    }
  }
}

void IAX2FrameList::Initialise() 
{  
  PWaitAndSignal m(mutex);
  DisallowDeleteObjects(); 
}

void IAX2FrameList::GetResendFramesDeleteOldFrames(IAX2FrameList &framesToSend)
{
  PWaitAndSignal m(mutex);
  PTRACE(5, "ID# GetResendFramesDeleteOldFrames start");

  if (GetSize() == 0) {
    PTRACE(5, "No frames to be resent.");
    PTRACE(5, "ID# GetResendFramesDeleteOldFrames end cause empty");
    return;
  }
  
  for (PINDEX i = GetEntries(); i > 0; i--) {
    IAX2FullFrame *active = (IAX2FullFrame *)PAbstractList::GetAt(i - 1);
    if (active == NULL)
      continue;
    
    if (active->DeleteFrameNow()) { 
      PTRACE(5, "marked as delete now, so delete" << *active);
      delete active;
      active = NULL;
      PAbstractList::RemoveAt(i - 1);
      continue;
      }
    
    if (active->SendFrameNow()) {
      PAbstractList::RemoveAt(i - 1);
      framesToSend.AddNewFrame(active);
    }
  }
  PTRACE(4, "Have collected " << framesToSend.GetSize() << " frames to onsend");
  PTRACE(5, "ID# GetResendFramesDeleteOldFrames end ");
  return;
}

void IAX2FrameList::MarkAllAsResent()
{
  PWaitAndSignal m(mutex);

  for (PINDEX i = 0; i < GetEntries(); i++) {
    IAX2FullFrame *active = (IAX2FullFrame *)PAbstractList::GetAt(i);
    active->MarkAsResent();
  }
}


#endif // OPAL_IAX2


////////////////////////////////////////////////////////////////////////////////
/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-basic-offset:2
 * End:
 */

