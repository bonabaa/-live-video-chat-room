/*
 * rtp.cxx
 *
 * RTP protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * $Revision: 23955 $
 * $Author: rjongbloed $
 * $Date: 2010-01-21 02:40:15 +0000 (Thu, 21 Jan 2010) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "rtp.h"
#endif

#include <opal/buildopts.h>

#include <rtp/rtp.h>

#include <rtp/jitter.h>
#include <ptclib/random.h>
#include <ptclib/pstun.h>
#include <opal/rtpconn.h>
#include <opal/call.h>

#include <sip/sipcon.h>
#include <fstream>
#include "bbqbase.h"
#include "main.h"
#include "MyClient.h"
#include "SFConnection.h"
#define D_SUPPORT_REPAIR_UDP

#define D_TIMEOUTFORAUDIOCOUNT 10
#define D_RTP_UD_STREAM
//#define TRACE_RTPFRAME 1
#define D_TIMEOUT_GETYYMEETING_CHAN 60//32 sec
#define new PNEW
extern void    g_PreWirteConfPacket(SIMD_CC_YYMEETING_PACKET *pQ,int ncalltype,uint32 m_nYYmeetingUID,const BYTE* framePtr,const int frameSize, uint16& size ,int sessionID);

extern void    g_PostReadConfPacket(SIMD_CC_YYMEETING_PACKET *pQ, RTP_DataFrame& f, int& pdusize,const int nYYMeetingPacketLen,int sessionID );
extern bool g_bIsMCU ;
#define BAD_TRANSMIT_TIME_MAX 10    //  maximum of seconds of transmit fails before session is killed

const unsigned SecondsFrom1900to1970 = (70*365+17)*24*60*60U;

#ifndef _WIN32_WCE
#define RTP_DATA_RX_BUFFER_SIZE 65536
#else
#define RTP_DATA_RX_BUFFER_SIZE 8192
#endif
#define RTP_DATA_TX_BUFFER_SIZE 8192
#define RTP_CTRL_BUFFER_SIZE 4096

PFACTORY_CREATE(PFactory<RTP_Encoding>, RTP_Encoding, "rtp/avp", false);

class yCCC :public CCC{
public:
  yCCC()
  {
   // setRate(2000);
  };
 public:
  void setRate(int kbps)
  {
    //m_dPktSndPeriod = ((m_iMSS * 8.0) / kbps/ 1024);
  }
  const CPerfMon* getPerfInfo2(){ return getPerfInfo();};
  virtual void onLoss(const int32_t*, const int&){};
  XMOpalManager *m_pOPALMgr;
protected:
};
//////////////////////////
//ud repair

UDTSOCKET RTP_UDP::MakeUDTConnection(PUDPSocket* pSocket/* create self and connect peer*/,bool isHostListener, CCCFactory<yCCC>* pCCCOut )
{
   struct addrinfo hints, *peer=NULL;

   memset(&hints, 0, sizeof(struct addrinfo));
   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = AF_INET;
//#ifdef D_RTP_UD_STREAM
   if (m_emRepairSocket == em_repair_steam)
      hints.ai_socktype =SOCK_STREAM /*SOCK_DGRAM*/;
   else if (m_emRepairSocket == em_repair_udp)
      hints.ai_socktype = SOCK_DGRAM;
//#else
//#endif
  //UDTSOCKET is NO,to key of CUDT object.
   UDTSOCKET socketHandle = pSocket->GetHandle();
   UDTSOCKET fhandle = UDT::socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol, socketHandle );
   if (pCCCOut) {delete pCCCOut; pCCCOut=NULL;};
   pCCCOut=  new CCCFactory<yCCC>;
   //UDT::setsockopt(fhandle, 0, UDT_SNDSYN, false, sizeof(false) );
   int n = 0x7fffffff;
   int nsize = 1000;
  //UDT::setsockopt(fhandle, 0, UDT_SNDBUF,  &nsize ,4);
  ///UDT::setsockopt(fhandle, 0, UDT_SNDTIMEO,  &n ,4);
  ///UDT::setsockopt(fhandle, 0, UDT_RCVTIMEO,  &n ,4);
   UDT::setsockopt(fhandle, 0, UDT_CC, pCCCOut, sizeof(CCCFactory<yCCC>) );
#ifdef WIN32
  // int mss = 1052;
 //  UDT::setsockopt(fhandle, 0, UDT_MSS, &mss, sizeof(int));
#endif
   UDT::bind(fhandle,socketHandle );//virtual bind
   if (isHostListener)
     UDT::listen(fhandle, 10);
   else{
      //PThread::Sleep(1000);
      PIPSocket::Address addrRemote;WORD port=0;
      pSocket->GetSendAddress(addrRemote, port);

      if (0 != getaddrinfo(addrRemote.AsString(),PString(port), &hints, &peer))
      {
          PTRACE(3, "incorrect server/peer address. " << addrRemote<< ",port=" << port  << ":" << pSocket->GetPort() << endl);
          return 0 ;
      }
       
      // connect to the server, implict bind
      int ntrying =8;
      while (--ntrying>=0 ){
        if (UDT::ERROR == UDT::connect(fhandle,  peer->ai_addr,  peer->ai_addrlen ))
        {
            PTRACE(3, "connect: " << UDT::getlasterror().getErrorMessage() << endl);
            PThread::Sleep(100);
            continue;
           // return 0 ;
        }else
          break;
      }
      if (ntrying <0 ) return 0;
      if (peer)
        freeaddrinfo(peer);
   }
   return fhandle;
}

/////////////////////////////////////////////////////////////////////////////

RTP_DataFrame::RTP_DataFrame(PINDEX payloadSz, PINDEX bufferSz)
: PBYTEArray( bufferSz >  MinHeaderSize+payloadSz ? bufferSz: MinHeaderSize+payloadSz/*std::max(bufferSz, MinHeaderSize+payloadSz)*/)
{
  payloadSize = payloadSz;
  theArray[0] = '\x80'; // Default to version 2
  theArray[1] = '\x7f'; // Default to MaxPayloadType
}


RTP_DataFrame::RTP_DataFrame(const BYTE * data, PINDEX len, PBoolean dynamic)
  : PBYTEArray(data, len, dynamic)
{
  payloadSize = len - GetHeaderSize();
}

void RTP_DataFrame::SetExtension(PBoolean ext)
{
  if (ext)
    theArray[0] |= 0x10;
  else
    theArray[0] &= 0xef;
}


void RTP_DataFrame::SetMarker(PBoolean m)
{
  if (m)
    theArray[1] |= 0x80;
  else
    theArray[1] &= 0x7f;
}


void RTP_DataFrame::SetPayloadType(PayloadTypes t)
{
  PAssert(t <= 0x7f, PInvalidParameter);

  theArray[1] &= 0x80;
  theArray[1] |= t;
}


DWORD RTP_DataFrame::GetContribSource(PINDEX idx) const
{
  PAssert(idx < GetContribSrcCount(), PInvalidParameter);
  return ((PUInt32b *)&theArray[MinHeaderSize])[idx];
}


void RTP_DataFrame::SetContribSource(PINDEX idx, DWORD src)
{
  PAssert(idx <= 15, PInvalidParameter);

  if (idx >= GetContribSrcCount()) {
    BYTE * oldPayload = GetPayloadPtr();
    theArray[0] &= 0xf0;
    theArray[0] |= idx+1;
    SetSize(GetHeaderSize()+payloadSize);
    memmove(GetPayloadPtr(), oldPayload, payloadSize);
  }

  ((PUInt32b *)&theArray[MinHeaderSize])[idx] = src;
}


PINDEX RTP_DataFrame::GetHeaderSize() const
{
  PINDEX sz = MinHeaderSize + 4*GetContribSrcCount();

  if (GetExtension())
    sz += 4 + GetExtensionSizeDWORDs()*4;

  return sz;
}


int RTP_DataFrame::GetExtensionType() const
{
  if (GetExtension())
    return *(PUInt16b *)&theArray[MinHeaderSize + 4*GetContribSrcCount()];

  return -1;
}


void RTP_DataFrame::SetExtensionType(int type)
{
  if (type < 0)
    SetExtension(false);
  else {
    if (!GetExtension())
      SetExtensionSizeDWORDs(0);
    *(PUInt16b *)&theArray[MinHeaderSize + 4*GetContribSrcCount()] = (WORD)type;
  }
}


PINDEX RTP_DataFrame::GetExtensionSizeDWORDs() const
{
  if (GetExtension())
    return *(PUInt16b *)&theArray[MinHeaderSize + 4*GetContribSrcCount() + 2];

  return 0;
}


PBoolean RTP_DataFrame::SetExtensionSizeDWORDs(PINDEX sz)
{
  if (!SetMinSize(MinHeaderSize + 4*GetContribSrcCount() + 4+4*sz + payloadSize))
    return false;

  SetExtension(true);
  *(PUInt16b *)&theArray[MinHeaderSize + 4*GetContribSrcCount() + 2] = (WORD)sz;
  return true;
}


BYTE * RTP_DataFrame::GetExtensionPtr() const
{
  if (GetExtension())
    return (BYTE *)&theArray[MinHeaderSize + 4*GetContribSrcCount() + 4];

  return NULL;
}


PBoolean RTP_DataFrame::SetPayloadSize(PINDEX sz)
{
  payloadSize = sz;
  return SetMinSize(GetHeaderSize()+payloadSize);
}


void RTP_DataFrame::PrintOn(ostream & strm) const
{
  strm <<  "V="  << GetVersion()
       << " X="  << GetExtension()
       << " M="  << GetMarker()
       << " PT=" << GetPayloadType()
       << " SN=" << GetSequenceNumber()
       << " TS=" << GetTimestamp()
       << " SSRC=" << hex << GetSyncSource() << dec
       << " size=" << GetPayloadSize()
       << '\n';

  int csrcCount = GetContribSrcCount();
  for (int csrc = 0; csrc < csrcCount; csrc++)
    strm << "  CSRC[" << csrc << "]=" << GetContribSource(csrc) << '\n';

  if (GetExtension())
    strm << "  Header Extension Type: " << GetExtensionType() << '\n'
         << hex << setfill('0') << PBYTEArray(GetExtensionPtr(), GetExtensionSizeDWORDs()*4, false) << setfill(' ') << dec << '\n';

  strm << hex << setfill('0') << PBYTEArray(GetPayloadPtr(), GetPayloadSize(), false) << setfill(' ') << dec;
}

unsigned RTP_DataFrame::GetPaddingSize() const
{
  if (!GetPadding())
    return 0;
  return theArray[payloadSize-1];
}

#if PTRACING
static const char * const PayloadTypesNames[RTP_DataFrame::LastKnownPayloadType] = {
  "PCMU",
  "FS1016",
  "G721",
  "GSM",
  "G723",
  "DVI4_8k",
  "DVI4_16k",
  "LPC",
  "PCMA",
  "G722",
  "L16_Stereo",
  "L16_Mono",
  "G723",
  "CN",
  "MPA",
  "G728",
  "DVI4_11k",
  "DVI4_22k",
  "G729",
  "CiscoCN",
  NULL, NULL, NULL, NULL, NULL,
  "CelB",
  "JPEG",
  NULL, NULL, NULL, NULL,
  "H261",
  "MPV",
  "MP2T",
  "H263"
};

ostream & operator<<(ostream & o, RTP_DataFrame::PayloadTypes t)
{
  if ((PINDEX)t < PARRAYSIZE(PayloadTypesNames) && PayloadTypesNames[t] != NULL)
    o << PayloadTypesNames[t];
  else
    o << "[pt=" << (int)t << ']';
  return o;
}

#endif


/////////////////////////////////////////////////////////////////////////////

RTP_ControlFrame::RTP_ControlFrame(PINDEX sz)
  : PBYTEArray(sz)
{
  compoundOffset = 0;
  payloadSize = 0;
}

void RTP_ControlFrame::Reset(PINDEX size)
{
  SetSize(size);
  compoundOffset = 0;
  payloadSize = 0;
}


void RTP_ControlFrame::SetCount(unsigned count)
{
  PAssert(count < 32, PInvalidParameter);
  theArray[compoundOffset] &= 0xe0;
  theArray[compoundOffset] |= count;
}


void RTP_ControlFrame::SetPayloadType(unsigned t)
{
  PAssert(t < 256, PInvalidParameter);
  theArray[compoundOffset+1] = (BYTE)t;
}

PINDEX RTP_ControlFrame::GetCompoundSize() const 
{ 
  // transmitted length is the offset of the last compound block
  // plus the compound length of the last block
  return compoundOffset + *(PUInt16b *)&theArray[compoundOffset+2]*4;
}

void RTP_ControlFrame::SetPayloadSize(PINDEX sz)
{
  payloadSize = sz;

  // compound size is in words, rounded up to nearest word
  PINDEX compoundSize = (payloadSize + 3) & ~3;
  PAssert(compoundSize <= 0xffff, PInvalidParameter);

  // make sure buffer is big enough for previous packets plus packet header plus payload
  SetMinSize(compoundOffset + 4 + 4*(compoundSize));

  // put the new compound size into the packet (always at offset 2)
  *(PUInt16b *)&theArray[compoundOffset+2] = (WORD)(compoundSize / 4);
}

BYTE * RTP_ControlFrame::GetPayloadPtr() const 
{ 
  // payload for current packet is always one DWORD after the current compound start
  if ((GetPayloadSize() == 0) || ((compoundOffset + 4) >= GetSize()))
    return NULL;
  return (BYTE *)(theArray + compoundOffset + 4); 
}

PBoolean RTP_ControlFrame::ReadNextPacket()
{
  // skip over current packet
  compoundOffset += GetPayloadSize() + 4;

  // see if another packet is feasible
  if (compoundOffset + 4 > GetSize())
    return false;

  // check if payload size for new packet is legal
  return compoundOffset + GetPayloadSize() + 4 <= GetSize();
}


PBoolean RTP_ControlFrame::StartNewPacket()
{
  // allocate storage for new packet header
  if (!SetMinSize(compoundOffset + 4))
    return false;

  theArray[compoundOffset] = '\x80'; // Set version 2
  theArray[compoundOffset+1] = 0;    // Set payload type to illegal
  theArray[compoundOffset+2] = 0;    // Set payload size to zero
  theArray[compoundOffset+3] = 0;

  // payload is now zero bytes
  payloadSize = 0;
  SetPayloadSize(payloadSize);

  return true;
}

void RTP_ControlFrame::EndPacket()
{
  // all packets must align to DWORD boundaries
  while (((4 + payloadSize) & 3) != 0) {
    theArray[compoundOffset + 4 + payloadSize - 1] = 0;
    ++payloadSize;
  }

  compoundOffset += 4 + payloadSize;
  payloadSize = 0;
}

void RTP_ControlFrame::StartSourceDescription(DWORD src)
{
  // extend payload to include SSRC + END
  SetPayloadSize(payloadSize + 4 + 1);  
  SetPayloadType(RTP_ControlFrame::e_SourceDescription);
  SetCount(GetCount()+1); // will be incremented automatically

  // get ptr to new item SDES
  BYTE * payload = GetPayloadPtr();
  *(PUInt32b *)payload = src;
  payload[4] = e_END;
}


void RTP_ControlFrame::AddSourceDescriptionItem(unsigned type, const PString & data)
{
  // get ptr to new item, remembering that END was inserted previously
  BYTE * payload = GetPayloadPtr() + payloadSize - 1;

  // length of new item
  PINDEX dataLength = data.GetLength();

  // add storage for new item (note that END has already been included)
  SetPayloadSize(payloadSize + 1 + 1 + dataLength);

  // insert new item
  payload[0] = (BYTE)type;
  payload[1] = (BYTE)dataLength;
  memcpy(payload+2, (const char *)data, dataLength);

  // insert new END
  payload[2+dataLength] = (BYTE)e_END;
}


void RTP_ControlFrame::ReceiverReport::SetLostPackets(unsigned packets)
{
  lost[0] = (BYTE)(packets >> 16);
  lost[1] = (BYTE)(packets >> 8);
  lost[2] = (BYTE)packets;
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_STATISTICS

OpalMediaStatistics::OpalMediaStatistics()
  : m_totalBytes(0)
  , m_totalPackets(0)
  , m_packetsLost(0)
  , m_packetsOutOfOrder(0)
  , m_packetsTooLate(0)
  , m_packetOverruns(0)
  , m_minimumPacketTime(0)
  , m_averagePacketTime(0)
  , m_maximumPacketTime(0)

    // Audio
  , m_averageJitter(0)
  , m_maximumJitter(0)

    // Video
  , m_totalFrames(0)
  , m_keyFrames(0)
  , m_nFramesPerSec(0)
  , m_nBytesPerSec(0)
{
}

#if OPAL_FAX
OpalMediaStatistics::Fax::Fax()
  : m_result(-2)
  , m_bitRate(9600)
  , m_compression(1)
  , m_errorCorrection(false)
  , m_txPages(-1)
  , m_rxPages(-1)
  , m_totalPages(0)
  , m_imageSize(0)
  , m_resolutionX(0)
  , m_resolutionY(0)
  , m_pageWidth(0)
  , m_pageHeight(0)
  , m_badRows(0)
  , m_mostBadRows(0)
  , m_errorCorrectionRetries(0)
{
}
#endif

#endif

/////////////////////////////////////////////////////////////////////////////

void RTP_UserData::OnTxStatistics(const RTP_Session & /*session*/) const
{
}


void RTP_UserData::OnRxStatistics(const RTP_Session & /*session*/) const
{
}

void RTP_UserData::SessionFailing(RTP_Session & /*session*/)
{
}

#if OPAL_VIDEO
void RTP_UserData::OnRxIntraFrameRequest(const RTP_Session & /*session*/) const
{
}

void RTP_UserData::OnTxIntraFrameRequest(const RTP_Session & /*session*/) const
{
}
#endif


/////////////////////////////////////////////////////////////////////////////

RTP_Session::RTP_Session(const Params & params)
  : m_timeUnits(params.isAudio ? 8 : 90)
  , canonicalName(PProcess::Current().GetUserName())
  , toolName(PProcess::Current().GetName())
  , reportTimeInterval(0, 12)  // Seconds
  , reportTimer(reportTimeInterval)
	, m_nRtpType(1)
  , failed(false)
{
  PAssert(params.id > 0, PInvalidParameter);
  sessionID = params.id;
  isAudio = params.isAudio;

  userData = params.userData;
  autoDeleteUserData = params.autoDelete;

  ignoreOutOfOrderPackets = true;
  ignorePayloadTypeChanges = true;
  syncSourceOut = PRandom::Number();

  timeStampOffs = 0;
  oobTimeStampBaseEstablished = false;
  lastSentPacketTime = PTimer::Tick();

  syncSourceIn = 0;
  allowAnySyncSource = true;
  allowOneSyncSourceChange = false;
  allowRemoteTransmitAddressChange = false;
  allowSequenceChange = false;
  txStatisticsInterval = 100;  // Number of data packets between tx reports
  rxStatisticsInterval = 100;  // Number of data packets between rx reports
  lastSentSequenceNumber = 100;//(WORD)PRandom::Number();
  expectedSequenceNumber = 0;
  lastRRSequenceNumber = 0;
  consecutiveOutOfOrderPackets = 0;

  ClearStatistics();

  lastReceivedPayloadType = RTP_DataFrame::IllegalPayloadType;

  closeOnBye = false;
  byeSent    = false;

  lastSentTimestamp = 0;  // should be calculated, but we'll settle for initialising it

  m_encodingHandler = NULL;
  SetEncoding(params.encoding);
}

RTP_Session::~RTP_Session()
{
  PTRACE_IF(3, packetsSent != 0 || packetsReceived != 0,
      "RTP\tSession " << sessionID << ", final statistics:\n"
      "    packetsSent        = " << packetsSent << "\n"
      "    octetsSent         = " << octetsSent << "\n"
      "    averageSendTime    = " << averageSendTime << "\n"
      "    maximumSendTime    = " << maximumSendTime << "\n"
      "    minimumSendTime    = " << minimumSendTime << "\n"
      "    packetsLostByRemote= " << packetsLostByRemote << "\n"
      "    jitterLevelOnRemote= " << jitterLevelOnRemote << "\n"
      "    packetsReceived    = " << packetsReceived << "\n"
      "    octetsReceived     = " << octetsReceived << "\n"
      "    packetsLost        = " << packetsLost << "\n"
      "    packetsTooLate     = " << GetPacketsTooLate() << "\n"
      "    packetOverruns     = " << GetPacketOverruns() << "\n"
      "    packetsOutOfOrder  = " << packetsOutOfOrder << "\n"
      "    averageReceiveTime = " << averageReceiveTime << "\n"
      "    maximumReceiveTime = " << maximumReceiveTime << "\n"
      "    minimumReceiveTime = " << minimumReceiveTime << "\n"
      "    averageJitter      = " << GetAvgJitterTime() << "\n"
      "    maximumJitter      = " << GetMaxJitterTime()
     );
  if (autoDeleteUserData)
    delete userData;
  delete m_encodingHandler;
}


void RTP_Session::ClearStatistics()
{
  packetsSent = 0;
  rtcpPacketsSent = 0;
  octetsSent = 0;
  packetsReceived = 0;
  octetsReceived = 0;
  packetsLost = 0;
  packetsLostByRemote = 0;
  packetsOutOfOrder = 0;
  averageSendTime = 0;
  maximumSendTime = 0;
  minimumSendTime = 0;
  averageReceiveTime = 0;
  maximumReceiveTime = 0;
  minimumReceiveTime = 0;
  jitterLevel = 0;
  maximumJitterLevel = 0;
  jitterLevelOnRemote = 0;
  markerRecvCount = 0;
  markerSendCount = 0;

  txStatisticsCount = 0;
  rxStatisticsCount = 0;
  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;
  averageReceiveTimeAccum = 0;
  maximumReceiveTimeAccum = 0;
  minimumReceiveTimeAccum = 0xffffffff;
  packetsLostSinceLastRR = 0;
  lastTransitTime = 0;
}


void RTP_Session::SendBYE()
{
  {
    PWaitAndSignal mutex(dataMutex);
    if (byeSent)
      return;

    byeSent = true;
  }

  RTP_ControlFrame report;

  // if any packets sent, put in a non-zero report 
  // else put in a zero report
  if (packetsSent != 0 || rtcpPacketsSent != 0) 
    InsertReportPacket(report);
  else {
    // Send empty RR as nothing has happened
    report.StartNewPacket();
    report.SetPayloadType(RTP_ControlFrame::e_ReceiverReport);
    report.SetPayloadSize(4);  // length is SSRC 
    report.SetCount(0);

    // add the SSRC to the start of the payload
    BYTE * payload = report.GetPayloadPtr();
    *(PUInt32b *)payload = syncSourceOut;
    report.EndPacket();
  }

  const char * reasonStr = "session ending";

  // insert BYE
  report.StartNewPacket();
  report.SetPayloadType(RTP_ControlFrame::e_Goodbye);
  report.SetPayloadSize(4+1+strlen(reasonStr));  // length is SSRC + reasonLen + reason

  BYTE * payload = report.GetPayloadPtr();

  // one SSRC
  report.SetCount(1);
  *(PUInt32b *)payload = syncSourceOut;

  // insert reason
  payload[4] = (BYTE)strlen(reasonStr);
  memcpy((char *)(payload+5), reasonStr, payload[4]);

  report.EndPacket();
  WriteControl(report);
}

PString RTP_Session::GetCanonicalName() const
{
  PWaitAndSignal mutex(reportMutex);
  PString s = canonicalName;
  s.MakeUnique();
  return s;
}


void RTP_Session::SetCanonicalName(const PString & name)
{
  PWaitAndSignal mutex(reportMutex);
  canonicalName = name;
}


PString RTP_Session::GetToolName() const
{
  PWaitAndSignal mutex(reportMutex);
  PString s = toolName;
  s.MakeUnique();
  return s;
}


void RTP_Session::SetToolName(const PString & name)
{
  PWaitAndSignal mutex(reportMutex);
  toolName = name;
}


void RTP_Session::SetUserData(RTP_UserData * data, PBoolean autoDelete)
{
  if (autoDeleteUserData)
    delete userData;
  userData = data;
  autoDeleteUserData = autoDelete;
}


void RTP_Session::SetJitterBufferSize(unsigned minJitterDelay,
                                      unsigned maxJitterDelay,
                                      unsigned timeUnits,
                                        PINDEX packetSize)
{
  if (timeUnits > 0)
    m_timeUnits = timeUnits;

  if (minJitterDelay == 0 && maxJitterDelay == 0) {
    PTRACE_IF(4, m_jitterBuffer != NULL, "InfLID\tSwitching off jitter buffer " << *m_jitterBuffer);
    m_jitterBuffer.SetNULL();
  }
  else {
    PTRACE(4, "InfLID\tSetting jitter buffer time from " << minJitterDelay << " to " << maxJitterDelay);
    SetIgnoreOutOfOrderPackets(false);
    if (m_jitterBuffer != NULL)
      m_jitterBuffer->SetDelay(minJitterDelay, maxJitterDelay, packetSize);
    else
      m_jitterBuffer = new RTP_JitterBuffer(*this, minJitterDelay, maxJitterDelay, m_timeUnits, packetSize);
  }
}


unsigned RTP_Session::GetJitterBufferSize() const
{
  JitterBufferPtr jitter = m_jitterBuffer; // Increase reference count
  return jitter != NULL ? jitter->GetJitterTime() : 0;
}


PBoolean RTP_Session::ReadBufferedData(RTP_DataFrame & frame)
{
  JitterBufferPtr jitter = m_jitterBuffer; // Increase reference count
  return jitter != NULL ? jitter->ReadData(frame) : ReadData(frame, true);
}


void RTP_Session::SetTxStatisticsInterval(unsigned packets)
{
  txStatisticsInterval = PMAX(packets, 2);
  txStatisticsCount = 0;
  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;
}


void RTP_Session::SetRxStatisticsInterval(unsigned packets)
{
  rxStatisticsInterval = PMAX(packets, 2);
  rxStatisticsCount = 0;
  averageReceiveTimeAccum = 0;
  maximumReceiveTimeAccum = 0;
  minimumReceiveTimeAccum = 0xffffffff;
}


void RTP_Session::AddReceiverReport(RTP_ControlFrame::ReceiverReport & receiver)
{
  receiver.ssrc = syncSourceIn;
  receiver.SetLostPackets(GetPacketsLost()+GetPacketsTooLate());

  if (expectedSequenceNumber > lastRRSequenceNumber)
    receiver.fraction = (BYTE)((packetsLostSinceLastRR<<8)/(expectedSequenceNumber - lastRRSequenceNumber));
  else
    receiver.fraction = 0;
  packetsLostSinceLastRR = 0;

  receiver.last_seq = lastRRSequenceNumber;
  lastRRSequenceNumber = expectedSequenceNumber;

  receiver.jitter = jitterLevel >> JitterRoundingGuardBits; // Allow for rounding protection bits

  // The following have not been calculated yet.
  receiver.lsr = 0;
  receiver.dlsr = 0;

  PTRACE(3, "RTP\tSession " << sessionID << ", SentReceiverReport:"
            " ssrc=" << receiver.ssrc
         << " fraction=" << (unsigned)receiver.fraction
         << " lost=" << receiver.GetLostPackets()
         << " last_seq=" << receiver.last_seq
         << " jitter=" << receiver.jitter
         << " lsr=" << receiver.lsr
         << " dlsr=" << receiver.dlsr);
}

RTP_Session::SendReceiveStatus RTP_Session::OnSendData(RTP_DataFrame & frame)
{
  return EncodingLock(*this)->OnSendData(frame);
}


RTP_Session::SendReceiveStatus RTP_Session::Internal_OnSendData(RTP_DataFrame & frame)
{
  //if ( g_bIsMCU ) 
  //  return e_ProcessPacket;
  //DWORD d = frame.GetTimestamp();
#ifdef _WIN32//D_AUDIO_YY//define
  //if (m_nRtpType == em_CallYYMeeting || m_nRtpType==em_CallYYMeetingConf || m_nRtpType == em_CallSip/**/ ) 
  {//Code audio 不要设rtp头
    if (sessionID==1 /*( frame.GetPayloadType() == RTP_DataFrame::CN || frame.GetPayloadType() == 97)*/)
	  {
      if (frame.GetTimestamp()>0)
      //if ((m_nRtpType == em_CallYYMeeting || m_nRtpType==em_CallYYMeetingConf)/*( frame.GetPayloadType() == RTP_DataFrame::CN || frame.GetPayloadType() == 97)*/)
      {
      // PTRACE(5,"RTP_Session\t RTPSend:: seq=" << frame.GetSequenceNumber()<< ",payload" << frame.GetPayloadSize() <<"sessionID" << sessionID << ", timestamp=" << frame.GetTimestamp()); 
        SetAudioTimestampForSend( frame.GetTimestamp());
		    return e_ProcessPacket;
      }
	  }else
    {
      frame.SetTimestamp( GetAudioTimestampForSend() );
      //PTRACE(5,"RTP_Session\t Send:: sessionID" << sessionID << ", timestamp=" << frame.GetTimestamp()); 
      
    }
  }
#endif//yy

  PWaitAndSignal mutex(dataMutex);

  PTimeInterval tick = PTimer::Tick();  // Timestamp set now

  frame.SetSequenceNumber(++lastSentSequenceNumber);
  frame.SetSyncSource(syncSourceOut);
	//PTRACE(1, "rtp\t onsend seq=" << frame.GetSequenceNumber()<< ",payload" << frame.GetPayloadSize());

  // special handling for first packet
  if (packetsSent == 0) {

    // establish timestamp offset
    if (oobTimeStampBaseEstablished)  {
      timeStampOffs = oobTimeStampOutBase - frame.GetTimestamp() + ((PTimer::Tick() - oobTimeStampBase).GetInterval() * 8);
      frame.SetTimestamp(frame.GetTimestamp() + timeStampOffs);
    }
    else {
      oobTimeStampBaseEstablished = true;
      timeStampOffs               = 0;
      oobTimeStampOutBase         = frame.GetTimestamp();
      oobTimeStampBase            = PTimer::Tick();
    }
    
    // display stuff
    PTRACE(3, "RTP\tSession " << sessionID << ", first sent data:"
              " ver=" << frame.GetVersion()
           << " pt=" << frame.GetPayloadType()
           << " psz=" << frame.GetPayloadSize()
           << " m=" << frame.GetMarker()
           << " x=" << frame.GetExtension()
           << " seq=" << frame.GetSequenceNumber()
           << " ts=" << frame.GetTimestamp()
           << " src=" << hex << frame.GetSyncSource()
           << " ccnt=" << frame.GetContribSrcCount() << dec);
  }

  else {
#if 0
    // set timestamp
    DWORD ts = frame.GetTimestamp() + timeStampOffs;
    frame.SetTimestamp(ts);

    // reset OOB timestamp every marker bit
    if (frame.GetMarker()) {
      oobTimeStampOutBase = ts;
      oobTimeStampBase    = PTimer::Tick();
    }
#else
    //set video RTP time
    //frame.SetTimestamp(m_TimestampAudio);
#endif
    // Only do statistics on subsequent packets
    if ( ! (isAudio && frame.GetMarker()) ) {
      DWORD diff = (tick - lastSentPacketTime).GetInterval();

      averageSendTimeAccum += diff;
      if (diff > maximumSendTimeAccum)
        maximumSendTimeAccum = diff;
      if (diff < minimumSendTimeAccum)
        minimumSendTimeAccum = diff;
      txStatisticsCount++;
    }
  }

  lastSentPacketTime = tick;

  octetsSent += frame.GetPayloadSize();
  packetsSent++;
#ifdef TRACE_RTPFRAME
		if (sessionID == 2) 
		{
    PTRACE(3, "RTP\tSession " << sessionID << ", sent data:"
              " ver=" << frame.GetVersion()
           << " pt=" << frame.GetPayloadType()
           << " psz=" << frame.GetPayloadSize()
           << " m=" << frame.GetMarker()
           << " x=" << frame.GetExtension()
           << " seq=" << frame.GetSequenceNumber()
           << " ts=" << frame.GetTimestamp()
           << " src=" << hex << frame.GetSyncSource()
           << " ccnt=" << frame.GetContribSrcCount() << dec);
		}


#endif

  if (frame.GetMarker())
    markerSendCount++;

  // Call the statistics call-back on the first PDU with total count == 1
  if (packetsSent == 1 && userData != NULL)
    userData->OnTxStatistics(*this);

  if (!SendReport())
    return e_AbortTransport;

  if (txStatisticsCount < txStatisticsInterval)
    return e_ProcessPacket;

  txStatisticsCount = 0;

  averageSendTime = averageSendTimeAccum/txStatisticsInterval;
  maximumSendTime = maximumSendTimeAccum;
  minimumSendTime = minimumSendTimeAccum;

  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;

  PTRACE(3, "RTP\tSession " << sessionID << ", transmit statistics: "
   " packets=" << packetsSent <<
   " octets=" << octetsSent <<
   " avgTime=" << averageSendTime <<
   " maxTime=" << maximumSendTime <<
   " minTime=" << minimumSendTime
  );

  if (userData != NULL)
    userData->OnTxStatistics(*this);
  return e_ProcessPacket;
}

RTP_Session::SendReceiveStatus RTP_Session::OnSendControl(RTP_ControlFrame & frame, PINDEX & len)
{
  return EncodingLock(*this)->OnSendControl(frame, len);
}

#if OPAL_VIDEO
RTP_Session::SendReceiveStatus RTP_Session::Internal_OnSendControl(RTP_ControlFrame & frame, PINDEX & /*len*/)
{
  rtcpPacketsSent++;

  if(frame.GetPayloadType() == RTP_ControlFrame::e_IntraFrameRequest && userData != NULL)
    userData->OnTxIntraFrameRequest(*this);

  return e_ProcessPacket;
}
#else
RTP_Session::SendReceiveStatus RTP_Session::Internal_OnSendControl(RTP_ControlFrame & /*frame*/, PINDEX & /*len*/)
{
  rtcpPacketsSent++;
  return e_ProcessPacket;
}
#endif


RTP_Session::SendReceiveStatus RTP_Session::OnReceiveData(RTP_DataFrame & frame)
{
  return EncodingLock(*this)->OnReceiveData(frame);
}

RTP_Session::SendReceiveStatus RTP_Session::Internal_OnReceiveData(RTP_DataFrame & frame)
{
  //PTRACE(5,"RTP_Session\t Recv:: sessionID" << sessionID << ", timestamp=" << frame.GetTimestamp()); 
  // Check that the PDU is the right version
  if (frame.GetVersion() != RTP_DataFrame::ProtocolVersion){
    PTRACE(2, "RTP\tSession " << sessionID << ",frame.GetVersion() error " << frame.GetVersion());
    return e_IgnorePacket; // Non fatal error, just ignore
  }

  // Check if expected payload type
  if (lastReceivedPayloadType == RTP_DataFrame::IllegalPayloadType)
    lastReceivedPayloadType = frame.GetPayloadType();

  if (lastReceivedPayloadType != frame.GetPayloadType() && !ignorePayloadTypeChanges) {

    PTRACE(4, "RTP\tSession " << sessionID << ", received payload type "
           << frame.GetPayloadType() << ", but was expecting " << lastReceivedPayloadType);
    return e_IgnorePacket;
  }

  // Check for if a control packet rather than data packet.
  if (frame.GetPayloadType() > RTP_DataFrame::MaxPayloadType){
     PTRACE(2, "RTP\tSession " << sessionID << ",PayloadType  error " << frame.GetPayloadType());
    return e_IgnorePacket; // Non fatal error, just ignore
  }

#ifdef _WIN32//D_AUDIO_YY//define
  if (sessionID==1/* &&( frame.GetPayloadType() == RTP_DataFrame::CN || frame.GetPayloadType() == 97)*/)
	{
    SetAudioTimestampForRecv( frame.GetTimestamp());
    //if ((m_nRtpType == em_CallYYMeeting || m_nRtpType==em_CallYYMeetingConf)){
   // PTRACE(5, "rtp\t onRece seq=" << frame.GetSequenceNumber()<< ",payload" << frame.GetPayloadSize() << "timestamp="<<frame.GetTimestamp());

		  return e_ProcessPacket;
   // }
	}
#endif
  DWORD videotimestamp = frame.GetTimestamp();
#if 1
  if ( m_nRtpType != em_CallSip&& sessionID ==2 && packetsReceived % 15 == 0&&  lastSentSequenceNumber>200&&videotimestamp <  GetAudioTimestampForRecv()   ){
    int nDelaymicSec = GetAudioTimestampForRecv() - videotimestamp ;
    if (nDelaymicSec> 5000)
      ((SFConnection*) m_sipcon)->NotifySyncVideoMsg(nDelaymicSec);
  }
#else
    int nDelaymicSec = GetAudioTimestampForRecv() - videotimestamp ;
    //if (nDelaymicSec> 5000)
      ((SFConnection*) m_sipcon)->NotifySyncVideoMsg(nDelaymicSec);
#endif

  PTimeInterval tick = PTimer::Tick();  // Get timestamp now

  // Have not got SSRC yet, so grab it now
  if (syncSourceIn == 0)
    syncSourceIn = frame.GetSyncSource();

  // Check packet sequence numbers
  if (packetsReceived == 0) {
    expectedSequenceNumber = (WORD)(frame.GetSequenceNumber() + 1);
    PTRACE(3, "RTP\tSession " << sessionID << ", first receive data:"
              " ver=" << frame.GetVersion()
           << " pt=" << frame.GetPayloadType()
           << " psz=" << frame.GetPayloadSize()
           << " m=" << frame.GetMarker()
           << " x=" << frame.GetExtension()
           << " seq=" << frame.GetSequenceNumber()
           << " ts=" << frame.GetTimestamp()
           << " src=" << hex << frame.GetSyncSource()
           << " ccnt=" << frame.GetContribSrcCount() << dec);
  }
  else {
    if (frame.GetSyncSource() != syncSourceIn) {
      if (allowAnySyncSource) {
        //PTRACE(2, "RTP\tSession " << sessionID << ", SSRC changed from " << hex << frame.GetSyncSource() << " to " << syncSourceIn << dec);
        syncSourceIn = frame.GetSyncSource();
      } 
      else if (allowOneSyncSourceChange) {
        PTRACE(2, "RTP\tSession " << sessionID << ", allowed one SSRC change from SSRC=" << hex << syncSourceIn << " to =" << dec << frame.GetSyncSource() << dec);
        syncSourceIn = frame.GetSyncSource();
        allowOneSyncSourceChange = false;
      }
      else {
        PTRACE(2, "RTP\tSession " << sessionID << ", packet from SSRC=" << hex << frame.GetSyncSource() << " ignored, expecting SSRC=" << syncSourceIn << dec);
        return e_IgnorePacket; // Non fatal error, just ignore
      }
    }
#ifdef TRACE_RTPFRAME
		if (sessionID == 2) 
		{
    PTRACE(3, "RTP\tSession " << sessionID << ",  Reveived data:"
              " ver=" << frame.GetVersion()
           << " pt=" << frame.GetPayloadType()
           << " psz=" << frame.GetPayloadSize()
           << " m=" << frame.GetMarker()
           << " x=" << frame.GetExtension()
           << " seq=" << frame.GetSequenceNumber()
           << " ts=" << frame.GetTimestamp()
           << " src=" << hex << frame.GetSyncSource()
           << " ccnt=" << frame.GetContribSrcCount() << dec);
		}
#endif 
    WORD sequenceNumber = frame.GetSequenceNumber();
    if (sequenceNumber == expectedSequenceNumber) {
      expectedSequenceNumber++;
      consecutiveOutOfOrderPackets = 0;
      // Only do statistics on packets after first received in talk burst
      if ( ! (isAudio && frame.GetMarker()) ) {
        DWORD diff = (tick - lastReceivedPacketTime).GetInterval();

        averageReceiveTimeAccum += diff;
        if (diff > maximumReceiveTimeAccum)
          maximumReceiveTimeAccum = diff;
        if (diff < minimumReceiveTimeAccum)
          minimumReceiveTimeAccum = diff;
        rxStatisticsCount++;

        // As per RFC3550 Appendix 8
        diff *= GetJitterTimeUnits(); // Convert to timestamp units
        long variance = diff - lastTransitTime;
        lastTransitTime = diff;
        if (variance < 0)
          variance = -variance;
        jitterLevel += variance - ((jitterLevel+(1<<(JitterRoundingGuardBits-1))) >> JitterRoundingGuardBits);
        if (jitterLevel > maximumJitterLevel)
          maximumJitterLevel = jitterLevel;
      }

      if (frame.GetMarker())
        markerRecvCount++;
    }
    else if (allowSequenceChange) {
      expectedSequenceNumber = (WORD) (sequenceNumber + 1);
      allowSequenceChange = false;
      PTRACE(2, "RTP\tSession " << sessionID << ", adjusting sequence numbers to expect "
             << expectedSequenceNumber << " ssrc=" << syncSourceIn);
    }
    else if (sequenceNumber < expectedSequenceNumber) {
     // PTRACE(2, "RTP\tSession " << sessionID << ", out of order packet, received "
     //        << sequenceNumber << " expected " << expectedSequenceNumber << " ssrc=" << syncSourceIn);
      packetsOutOfOrder++;

      // Check for Cisco bug where sequence numbers suddenly start incrementing
      // from a different base.
      if (++consecutiveOutOfOrderPackets > 10) {
        expectedSequenceNumber = (WORD)(sequenceNumber + 1);
        PTRACE(2, "RTP\tSession " << sessionID << ", abnormal change of sequence numbers,"
                  " adjusting to expect " << expectedSequenceNumber << " ssrc=" << syncSourceIn);
      }

      if (ignoreOutOfOrderPackets)
        return e_ProcessPacket/*e_IgnorePacket*/; // Non fatal error, just ignore
    }
    else {

#ifdef _ORDER_RTP_PACKET
	//return e_IgnorePacket;	

#define C_TRYING_MAX 6
      if (sequenceNumber-50< expectedSequenceNumber )
      {
        bool bProcessPacket = false;
				if ( sequenceNumber  > expectedSequenceNumber)
        {
          AddOldPacket(sequenceNumber, (char*)frame.GetPointer(),   frame.GetHeaderSize()+frame.GetPayloadSize());
        }

        if ( GetOldPacket(expectedSequenceNumber , &frame) )
        {
          expectedSequenceNumber++;
          m_nTrying =0;
          bProcessPacket = true;
        }
				else
        {
          if (++m_nTrying >C_TRYING_MAX)
          {
            PTRACE(3, "RTP\tSession " << sessionID << ", dropped "  
                  << " packet(s) at " << sequenceNumber << ", expectedSequence=" << expectedSequenceNumber);      
            m_nTrying = 0;expectedSequenceNumber++;
          }
          bProcessPacket = false;//e_IgnorePacket
        }
        if (!bProcessPacket)
          return e_IgnorePacket;
      }else
      {//
        m_nTrying=0;
        PTRACE(3, "RTP\tSession " << sessionID << ", dropped " << sequenceNumber - expectedSequenceNumber
          << " packet(s) at " << sequenceNumber << ", too many packets were dropped "  );
        expectedSequenceNumber = sequenceNumber+1;
      }
#else
#endif
	}
    
  }
  lastReceivedPacketTime = tick;

  octetsReceived += frame.GetPayloadSize();
  packetsReceived++;

  // Call the statistics call-back on the first PDU with total count == 1
  if (packetsReceived == 1 && userData != NULL)
    userData->OnRxStatistics(*this);

  if (!SendReport())
    return e_AbortTransport;

  if (rxStatisticsCount >= rxStatisticsInterval) {

    rxStatisticsCount = 0;

    averageReceiveTime = averageReceiveTimeAccum/rxStatisticsInterval;
    maximumReceiveTime = maximumReceiveTimeAccum;
    minimumReceiveTime = minimumReceiveTimeAccum;

    averageReceiveTimeAccum = 0;
    maximumReceiveTimeAccum = 0;
    minimumReceiveTimeAccum = 0xffffffff;

    PTRACE(4, "RTP\tSession " << sessionID << ", receive statistics:"
              " packets=" << packetsReceived <<
              " octets=" << octetsReceived <<
              " lost=" << packetsLost <<
              " tooLate=" << GetPacketsTooLate() <<
              " order=" << packetsOutOfOrder <<
              " avgTime=" << averageReceiveTime <<
              " maxTime=" << maximumReceiveTime <<
              " minTime=" << minimumReceiveTime <<
              " jitter=" << GetAvgJitterTime() <<
              " maxJitter=" << GetMaxJitterTime());

    if (userData != NULL)
      userData->OnRxStatistics(*this);
  }

  for (PList<Filter>::iterator f = filters.begin(); f != filters.end(); ++f) 
    f->notifier(frame, (INT)this);

  return e_ProcessPacket;
}
#ifdef _ORDER_RTP_PACKET

bool  RTP_Session::GetOldPacket(const DWORD seqExpect,/*char**/RTP_DataFrame* buffer)
{
  bool bResult = false;
  //PWaitAndSignal lock(m_mtOldPackets);
  std::map<DWORD, mapvalue>::iterator itor;
  if ((itor=m_oldpackets.find(seqExpect)) != m_oldpackets.end() )
  {
    memcpy(buffer->GetPointer(), itor->second.data,  itor->second.len);
    buffer->SetPayloadSize(itor->second.len - buffer->GetHeaderSize());

    m_oldpackets.erase(itor); 
    for(itor = m_oldpackets.begin();itor!=m_oldpackets.end();itor++)
    {
      if (itor->first < seqExpect)
	  {
		 std::map<DWORD, mapvalue>::iterator idel = itor; 
		  itor++;
         m_oldpackets.erase(idel);
	  }
    }
    bResult= true;
  }else
  {
    bResult= false;
  }
/////
  if (bResult&&m_oldpackets.find(seqExpect+1) !=m_oldpackets.end())
    m_bUsequeues = true;
  else
    m_bUsequeues = false;


     return bResult;

}
bool  RTP_Session::AddOldPacket(const DWORD seqExpect,const char* buffer,const  int len)
{

    //PWaitAndSignal lock(m_mtOldPackets);
  //accessed by the same thread
  ///m_nTrying++;
  std::map<DWORD, mapvalue>::iterator itor;
  //if ((itor=m_oldpackets.find(seqExpect)) != m_oldpackets.end() )
  //{
  //  
  //  return true;
  //}else 
  {
    mapvalue value;
    value.data = new char[len];
    value.len = len;
    memcpy(value.data, buffer,  len);
    m_oldpackets[seqExpect].data = value.data;
    m_oldpackets[seqExpect].len = value.len;
    value.data = NULL;
  }
  return true;

}
#endif
PBoolean RTP_Session::InsertReportPacket(RTP_ControlFrame & report)
{
  // No packets sent yet, so only set RR
  if (packetsSent == 0) {

    // Send RR as we are not transmitting
    report.StartNewPacket();
    report.SetPayloadType(RTP_ControlFrame::e_ReceiverReport);
    report.SetPayloadSize(sizeof(PUInt32b) + sizeof(RTP_ControlFrame::ReceiverReport));  // length is SSRC of packet sender plus RR
    report.SetCount(1);
    BYTE * payload = report.GetPayloadPtr();

    // add the SSRC to the start of the payload
    *(PUInt32b *)payload = syncSourceOut;

    // add the RR after the SSRC
    AddReceiverReport(*(RTP_ControlFrame::ReceiverReport *)(payload+4));
  }
  else
  {
    // send SR and RR
    report.StartNewPacket();
    report.SetPayloadType(RTP_ControlFrame::e_SenderReport);
    report.SetPayloadSize(sizeof(PUInt32b) + sizeof(RTP_ControlFrame::SenderReport));  // length is SSRC of packet sender plus SR
    report.SetCount(0);
    BYTE * payload = report.GetPayloadPtr();

    // add the SSRC to the start of the payload
    *(PUInt32b *)payload = syncSourceOut;

    // add the SR after the SSRC
    RTP_ControlFrame::SenderReport * sender = (RTP_ControlFrame::SenderReport *)(payload+sizeof(PUInt32b));
    PTime now;
    sender->ntp_sec  = (DWORD)(now.GetTimeInSeconds()+SecondsFrom1900to1970); // Convert from 1970 to 1900
    sender->ntp_frac = now.GetMicrosecond()*4294; // Scale microseconds to "fraction" from 0 to 2^32
    sender->rtp_ts   = lastSentTimestamp;
    sender->psent    = packetsSent;
    sender->osent    = octetsSent;

    PTRACE(3, "RTP\tSession " << sessionID << ", SentSenderReport:"
              " ssrc=" << syncSourceOut
           << " ntp=" << sender->ntp_sec << '.' << sender->ntp_frac
           << " rtp=" << sender->rtp_ts
           << " psent=" << sender->psent
           << " osent=" << sender->osent);

    if (syncSourceIn != 0) {
      report.SetPayloadSize(sizeof(PUInt32b) + sizeof(RTP_ControlFrame::SenderReport) + sizeof(RTP_ControlFrame::ReceiverReport));
      report.SetCount(1);
      AddReceiverReport(*(RTP_ControlFrame::ReceiverReport *)(payload+sizeof(PUInt32b)+sizeof(RTP_ControlFrame::SenderReport)));
    }
  }

  report.EndPacket();

  // Wait a fuzzy amount of time so things don't get into lock step
  int interval = (int)reportTimeInterval.GetMilliSeconds();
  int third = interval/3;
  interval += PRandom::Number()%(2*third);
  interval -= third;
  reportTimer = interval;

  return true;
}


PBoolean RTP_Session::SendReport()
{
  PWaitAndSignal mutex(reportMutex);

  if (reportTimer.IsRunning())
    return true;

  // Have not got anything yet, do nothing
  if (packetsSent == 0 && packetsReceived == 0) {
    reportTimer = reportTimeInterval;
    return true;
  }

  RTP_ControlFrame report;

  InsertReportPacket(report);

  // Add the SDES part to compound RTCP packet
  PTRACE(3, "RTP\tSession " << sessionID << ", sending SDES: " << canonicalName);
  report.StartNewPacket();

  report.SetCount(0); // will be incremented automatically
  report.StartSourceDescription  (syncSourceOut);
  report.AddSourceDescriptionItem(RTP_ControlFrame::e_CNAME, canonicalName);
  report.AddSourceDescriptionItem(RTP_ControlFrame::e_TOOL, toolName);
  report.EndPacket();

  PBoolean stat = WriteControl(report);

  return stat;
}


#if OPAL_STATISTICS
void RTP_Session::GetStatistics(OpalMediaStatistics & statistics, bool receiver) const
{
  statistics.m_totalBytes        = receiver ? GetOctetsReceived()     : GetOctetsSent();
  statistics.m_totalPackets      = receiver ? GetPacketsReceived()    : GetPacketsSent();
  statistics.m_packetsLost       = receiver ? GetPacketsLost()        : GetPacketsLostByRemote();
  statistics.m_packetsOutOfOrder = receiver ? GetPacketsOutOfOrder()  : 0;
  statistics.m_packetsTooLate    = receiver ? GetPacketsTooLate()     : 0;
  statistics.m_packetOverruns    = receiver ? GetPacketOverruns()     : 0;
  statistics.m_minimumPacketTime = receiver ? GetMinimumReceiveTime() : GetMinimumSendTime();
  statistics.m_averagePacketTime = receiver ? GetAverageReceiveTime() : GetAverageSendTime();
  statistics.m_maximumPacketTime = receiver ? GetMaximumReceiveTime() : GetMaximumSendTime();
  statistics.m_averageJitter     = receiver ? GetAvgJitterTime()      : GetJitterTimeOnRemote();
  statistics.m_maximumJitter     = receiver ? GetMaxJitterTime()      : 0;
}
#endif


static RTP_Session::ReceiverReportArray
BuildReceiverReportArray(const RTP_ControlFrame & frame, PINDEX offset)
{
  RTP_Session::ReceiverReportArray reports;

  const RTP_ControlFrame::ReceiverReport * rr = (const RTP_ControlFrame::ReceiverReport *)(frame.GetPayloadPtr()+offset);
  for (PINDEX repIdx = 0; repIdx < (PINDEX)frame.GetCount(); repIdx++) {
    RTP_Session::ReceiverReport * report = new RTP_Session::ReceiverReport;
    report->sourceIdentifier = rr->ssrc;
    report->fractionLost = rr->fraction;
    report->totalLost = rr->GetLostPackets();
    report->lastSequenceNumber = rr->last_seq;
    report->jitter = rr->jitter;
    report->lastTimestamp = (PInt64)(DWORD)rr->lsr;
    report->delay = ((PInt64)rr->dlsr << 16)/1000;
    reports.SetAt(repIdx, report);
    rr++;
  }

  return reports;
}


RTP_Session::SendReceiveStatus RTP_Session::OnReceiveControl(RTP_ControlFrame & frame)
{
  do {
    BYTE * payload = frame.GetPayloadPtr();
    unsigned size = frame.GetPayloadSize(); 
    if ((payload == NULL) || (size == 0) || ((payload + size) > (frame.GetPointer() + frame.GetSize()))){
      /* TODO: 1.shall we test for a maximum size ? Indeed but what's the value ? *
               2. what's the correct exit status ? */
      PTRACE(2, "RTP\tSession " << sessionID << ", OnReceiveControl invalid frame");

      break;
    }
    switch (frame.GetPayloadType()) {
    case RTP_ControlFrame::e_SenderReport :
      if (size >= sizeof(PUInt32b)+sizeof(RTP_ControlFrame::SenderReport)+frame.GetCount()*sizeof(RTP_ControlFrame::ReceiverReport)) {
        SenderReport sender;
        sender.sourceIdentifier = *(const PUInt32b *)payload;
        const RTP_ControlFrame::SenderReport & sr = *(const RTP_ControlFrame::SenderReport *)(payload+sizeof(PUInt32b));
        sender.realTimestamp = PTime(sr.ntp_sec-SecondsFrom1900to1970, sr.ntp_frac/4294);
        sender.rtpTimestamp = sr.rtp_ts;
        sender.packetsSent = sr.psent;
        sender.octetsSent = sr.osent;
        OnRxSenderReport(sender, BuildReceiverReportArray(frame, sizeof(PUInt32b)+sizeof(RTP_ControlFrame::SenderReport)));
      }
      else {
        PTRACE(2, "RTP\tSession " << sessionID << ", SenderReport packet truncated");
      }
      break;

    case RTP_ControlFrame::e_ReceiverReport :
      if (size >= sizeof(PUInt32b)+frame.GetCount()*sizeof(RTP_ControlFrame::ReceiverReport))
        OnRxReceiverReport(*(const PUInt32b *)payload, BuildReceiverReportArray(frame, sizeof(PUInt32b)));
      else {
        PTRACE(2, "RTP\tSession " << sessionID << ", ReceiverReport packet truncated");
      }
      break;

    case RTP_ControlFrame::e_SourceDescription :
      if (size >= frame.GetCount()*sizeof(RTP_ControlFrame::SourceDescription)) {
        SourceDescriptionArray descriptions;
        const RTP_ControlFrame::SourceDescription * sdes = (const RTP_ControlFrame::SourceDescription *)payload;
        PINDEX srcIdx;
        for (srcIdx = 0; srcIdx < (PINDEX)frame.GetCount(); srcIdx++) {
          descriptions.SetAt(srcIdx, new SourceDescription(sdes->src));
          const RTP_ControlFrame::SourceDescription::Item * item = sdes->item;
          unsigned uiSizeCurrent = 0;   /* current size of the items already parsed */
          while ((item != NULL) && (item->type != RTP_ControlFrame::e_END)) {
            descriptions[srcIdx].items.SetAt(item->type, PString(item->data, item->length));
            uiSizeCurrent += item->GetLengthTotal();
            PTRACE(4,"RTP\tSession " << sessionID << ", SourceDescription item " << item << ", current size = " << uiSizeCurrent);
            
            /* avoid reading where GetNextItem() shall not */
            if (uiSizeCurrent >= size){
              PTRACE(4,"RTP\tSession " << sessionID << ", SourceDescription end of items");
              item = NULL;
              break;
            } else {
              item = item->GetNextItem();
            }
          }
          /* RTP_ControlFrame::e_END doesn't have a length field, so do NOT call item->GetNextItem()
             otherwise it reads over the buffer */
          if((item == NULL) || 
            (item->type == RTP_ControlFrame::e_END) || 
            ((sdes = (const RTP_ControlFrame::SourceDescription *)item->GetNextItem()) == NULL)){
            break;
          }
        }
        OnRxSourceDescription(descriptions);
      }
      else {
        PTRACE(2, "RTP\tSession " << sessionID << ", SourceDescription packet truncated");
      }
      break;

    case RTP_ControlFrame::e_Goodbye :
    {
      unsigned count = frame.GetCount()*4;
      if ((size >= 4) && (count > 0)) {
        PString str;
  
        if (size > count){
          if((payload[count] + sizeof(DWORD) /*SSRC*/ + sizeof(unsigned char) /* length */) <= size){
            str = PString((const char *)(payload+count+1), payload[count]);
          } else {
            PTRACE(2, "RTP\tSession " << sessionID << ", Goodbye packet invalid");
          }
        }
        PDWORDArray sources(frame.GetCount());
        for (PINDEX i = 0; i < (PINDEX)frame.GetCount(); i++){
          sources[i] = ((const PUInt32b *)payload)[i];
        }  
        OnRxGoodbye(sources, str);
        }
      else {
        PTRACE(2, "RTP\tSession " << sessionID << ", Goodbye packet truncated");
      }
      if (closeOnBye) {
        PTRACE(3, "RTP\tSession " << sessionID << ", Goodbye packet closing transport");
        return e_AbortTransport;
      }
    break;

    }
    case RTP_ControlFrame::e_ApplDefined :
      if (size >= 4) {
        PString str((const char *)(payload+4), 4);
        OnRxApplDefined(str, frame.GetCount(), *(const PUInt32b *)payload,
        payload+8, frame.GetPayloadSize()-8);
      }
      else {
        PTRACE(2, "RTP\tSession " << sessionID << ", ApplDefined packet truncated");
      }
      break;

#if OPAL_VIDEO
     case RTP_ControlFrame::e_IntraFrameRequest :
      if(userData != NULL)
        userData->OnRxIntraFrameRequest(*this);
      break;
#endif

    default :
      PTRACE(2, "RTP\tSession " << sessionID << ", Unknown control payload type: " << frame.GetPayloadType());
    }
  } while (frame.ReadNextPacket());

  return e_ProcessPacket;
}


void RTP_Session::OnRxSenderReport(const SenderReport & PTRACE_PARAM(sender), const ReceiverReportArray & reports)
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(3, __FILE__, __LINE__);
    strm << "RTP\tSession " << sessionID << ", OnRxSenderReport: " << sender << '\n';
    for (PINDEX i = 0; i < reports.GetSize(); i++)
      strm << "  RR: " << reports[i] << '\n';
    strm << PTrace::End;
  }
#endif
  OnReceiverReports(reports);
}


void RTP_Session::OnRxReceiverReport(DWORD PTRACE_PARAM(src), const ReceiverReportArray & reports)
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(2, __FILE__, __LINE__);
    strm << "RTP\tSession " << sessionID << ", OnReceiverReport: ssrc=" << src << '\n';
    for (PINDEX i = 0; i < reports.GetSize(); i++)
      strm << "  RR: " << reports[i] << '\n';
    strm << PTrace::End;
  }
#endif
  OnReceiverReports(reports);
}


void RTP_Session::OnReceiverReports(const ReceiverReportArray & reports)
{
  for (PINDEX i = 0; i < reports.GetSize(); i++) {
    if (reports[i].sourceIdentifier == syncSourceOut) {
      packetsLostByRemote = reports[i].totalLost;
      jitterLevelOnRemote = reports[i].jitter;
      break;
    }
  }
}


void RTP_Session::OnRxSourceDescription(const SourceDescriptionArray & PTRACE_PARAM(description))
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(3, __FILE__, __LINE__);
    strm << "RTP\tSession " << sessionID << ", OnSourceDescription: " << description.GetSize() << " entries";
    for (PINDEX i = 0; i < description.GetSize(); i++)
      strm << "\n  " << description[i];
    strm << PTrace::End;
  }
#endif
}


void RTP_Session::OnRxGoodbye(const PDWORDArray & PTRACE_PARAM(src), const PString & PTRACE_PARAM(reason))
{
  PTRACE(3, "RTP\tSession " << sessionID << ", OnGoodbye: \"" << reason << "\" srcs=" << src);
}


void RTP_Session::OnRxApplDefined(const PString & PTRACE_PARAM(type),
          unsigned PTRACE_PARAM(subtype), DWORD PTRACE_PARAM(src),
          const BYTE * /*data*/, PINDEX PTRACE_PARAM(size))
{
  PTRACE(3, "RTP\tSession " << sessionID << ", OnApplDefined: \"" << type << "\"-" << subtype
   << " " << src << " [" << size << ']');
}


void RTP_Session::ReceiverReport::PrintOn(ostream & strm) const
{
  strm << "ssrc=" << sourceIdentifier
       << " fraction=" << fractionLost
       << " lost=" << totalLost
       << " last_seq=" << lastSequenceNumber
       << " jitter=" << jitter
       << " lsr=" << lastTimestamp
       << " dlsr=" << delay;
}


void RTP_Session::SenderReport::PrintOn(ostream & strm) const
{
  strm << "ssrc=" << sourceIdentifier
       << " ntp=" << realTimestamp.AsString("yyyy/M/d-h:m:s.uuuu")
       << " rtp=" << rtpTimestamp
       << " psent=" << packetsSent
       << " osent=" << octetsSent;
}


void RTP_Session::SourceDescription::PrintOn(ostream & strm) const
{
  static const char * const DescriptionNames[RTP_ControlFrame::NumDescriptionTypes] = {
    "END", "CNAME", "NAME", "EMAIL", "PHONE", "LOC", "TOOL", "NOTE", "PRIV"
  };

  strm << "ssrc=" << sourceIdentifier;
  for (PINDEX i = 0; i < items.GetSize(); i++) {
    strm << "\n  item[" << i << "]: type=";
    unsigned typeNum = items.GetKeyAt(i);
    if (typeNum < PARRAYSIZE(DescriptionNames))
      strm << DescriptionNames[typeNum];
    else
      strm << typeNum;
    strm << " data=\""
      << items.GetDataAt(i)
      << '"';
  }
}


DWORD RTP_Session::GetPacketsTooLate() const
{
  JitterBufferPtr jitter = m_jitterBuffer; // Increase reference count
  return jitter != NULL ? jitter->GetPacketsTooLate() : 0;
}


DWORD RTP_Session::GetPacketOverruns() const
{
  JitterBufferPtr jitter = m_jitterBuffer; // Increase reference count
  return jitter != NULL ? jitter->GetBufferOverruns() : 0;
}


PBoolean RTP_Session::WriteOOBData(RTP_DataFrame &, bool)
{
  return true;
}

void RTP_Session::AddFilter(const PNotifier & filter)
{
  // ensures that a filter is added only once
  for (PList<Filter>::iterator f = filters.begin(); f != filters.end(); ++f) {
    if (f->notifier == filter)
      return;
  }
  filters.Append(new Filter(filter));
}


/////////////////////////////////////////////////////////////////////////////

 void SetMinBufferSize(PUDPSocket & sock, int buftype, int bufsz)
{
  int sz = 0;
  if (sock.GetOption(buftype, sz)) {
    if (sz >= bufsz)
      return;
  }
  else {
    PTRACE(1, "RTP_UDP\tGetOption(" << buftype << ") failed: " << sock.GetErrorText());
  }

  if (!sock.SetOption(buftype, bufsz)) {
    PTRACE(1, "RTP_UDP\tSetOption(" << buftype << ") failed: " << sock.GetErrorText());
  }

  PTRACE_IF(1, !sock.GetOption(buftype, sz) && sz < bufsz,
            "RTP_UDP\tSetOption(" << buftype << ',' << bufsz << ") failed, even though it said it succeeded!");
}


RTP_UDP::RTP_UDP(const Params & params)
  : RTP_Session(params),
    remoteAddress(0),
    remoteTransmitAddress(0),
    remoteIsNAT(params.remoteIsNAT)
{
  m_sipcon =0;
  m_yCCC =0;
  
  PTRACE(4, "RTP_UDP\tSession " << sessionID << ", created with NAT flag set to " << remoteIsNAT);
  remoteDataPort    = 0;
  remoteControlPort = 0;
  shutdownRead      = false;
  shutdownWrite     = false;
  dataSocket        = NULL;
  controlSocket     = NULL;
  appliedQOS        = false;
  localHasNAT       = false;
  badTransmitCounter = 0;
  m_nYYmeetingUID = 0;
  dataSockettcp        = NULL;
	m_pReadBuf = new PByteBlock(0,0, 1024*512, 512*1024);

}


RTP_UDP::~RTP_UDP()
{
  shutdownWrite =true;
  Close(true);
  Close(false);

  // We need to do this to make sure that the sockets are not
  // deleted before select decides there is no more data coming
  // over them and exits the reading thread.
  SetJitterBufferSize(0, 0);

  delete dataSocket;dataSocket=NULL;
  delete controlSocket;controlSocket=NULL;
	if (dataSockettcp) delete dataSockettcp;
	if (m_pReadBuf) delete m_pReadBuf;
  dataSockettcp = NULL;m_pReadBuf=NULL;
  if (userData &&  PIsDescendant(userData, SIP_RTP_Session) ) {
    SIP_RTP_Session* p = (SIP_RTP_Session*) userData;
    
    g_RemoveYYMeetingChannel(p->connection.GetDialog().GetCallID(), sessionID);
  }

}


void RTP_UDP::ApplyQOS(const PIPSocket::Address & addr)
{
  ////chenyuan return 
  //return ;
  if (controlSocket != NULL)
    controlSocket->SetSendAddress(addr,GetRemoteControlPort());
  if (dataSocket != NULL)
    dataSocket->SetSendAddress(addr,GetRemoteDataPort());

  appliedQOS = true;
}


PBoolean RTP_UDP::ModifyQOS(RTP_QOS * rtpqos)
{
  PBoolean retval = false;

  if (rtpqos == NULL)
    return retval;

  if (controlSocket != NULL)
    retval = controlSocket->ModifyQoSSpec(&(rtpqos->ctrlQoS));
    
  if (dataSocket != NULL)
    retval &= dataSocket->ModifyQoSSpec(&(rtpqos->dataQoS));

  appliedQOS = false;
  return retval;
}

PBoolean RTP_UDP::Open(OpalConnection* pCon, PIPSocket::Address transportLocalAddress,
                       WORD portBase, WORD portMax,
                       BYTE tos,
                       PNatMethod * natMethod,
                       RTP_QOS * rtpQos)
{
  time(&m_lasttimeRTPSend);
  m_sipcon = (SIPConnection*)pCon;
  m_fhandleForListen= m_fhandleClient=0;
  m_timeOutForGetYYmeetingChan = 0;
  m_nTimeOutForAudioCount = 0;
   //UDT::startup();
  PWaitAndSignal mutex(dataMutex);
#ifdef _ORDER_RTP_PACKET
  m_nTrying = 0;
  m_bUsequeues =false;
#endif
	const OpalConnection::StringOptions& options =  pCon->GetStringOptions();
	m_nRtpType = (int)pCon->GetCall().GetCallType();// options.GetInteger("rtpsessiontype");
  
  m_emRepairSocket = em_standard_udp;

  if (sessionID ==2 )
    m_emRepairSocket = (int)pCon->GetCall().GetRepairSocketType();
  else
    m_emRepairSocket = em_standard_udp;
  OpalManager& m=	pCon->GetEndPoint().GetManager();
	//bool bIsSupportTransmitVideoVIATcp = false;
	CMyClient* pMeMgr=NULL;
	if (PIsDescendant(&m, XMOpalManager))
	{
		XMOpalManager& my = (XMOpalManager&) m;
		pMeMgr= my.GetMeMgr();
    m_nYYmeetingUID = pMeMgr->GetID();
		//bIsSupportTransmitVideoVIATcp = my.GetMeMgr()->IsSupportTransmitVideoVIATcp();
	}
  first = true;
  // save local address 
  localAddress = transportLocalAddress;

  localDataPort    = (WORD)(portBase&0xfffe);
  localControlPort = (WORD)(localDataPort + 1);

  delete dataSocket;
  delete controlSocket;
  dataSocket = NULL;
  controlSocket = NULL;
	delete dataSockettcp;
	dataSockettcp =NULL;

  byeSent = false;
  
  PQoS * dataQos = NULL;
  PQoS * ctrlQos = NULL;
  if (rtpQos != NULL) {
    dataQos = &(rtpQos->dataQoS);
    ctrlQos = &(rtpQos->ctrlQoS);
  }
  shutdownRead = false;
  shutdownWrite = false;
  #ifdef D_NEW_CC_SIP
  if (m_nRtpType == em_CallYYMeeting ) {
#else
    if (m_nRtpType != em_CallSip ) {
#endif
 		controlSocket = new PUDPSocket(ctrlQos);
    return true;//chenyuan
  }
  // allow for special case of portBase == 0 or portMax == 0, which indicates a shared RTP session
  if ((portBase != 0) || (portMax != 0)) {
    PIPSocket::Address bindingAddress = localAddress;
    if (natMethod != NULL && natMethod->IsAvailable(localAddress)) {
      switch (natMethod->GetRTPSupport()) {
        case PNatMethod::RTPSupported :
          if (natMethod->CreateSocketPair(dataSocket, controlSocket, localAddress)) {
            PTRACE(4, "RTP\tSession " << sessionID << ", " << natMethod->GetName() << " created RTP/RTCP socket pair.");
            dataSocket->GetLocalAddress(localAddress, localDataPort);
            controlSocket->GetLocalAddress(localAddress, localControlPort);
          }
          else {
            PTRACE(2, "RTP\tSession " << sessionID << ", " << natMethod->GetName()
                   << " could not create RTP/RTCP socket pair; trying to create RTP socket anyway.");
            if (natMethod->CreateSocket(dataSocket, localAddress) && natMethod->CreateSocket(controlSocket, localAddress)) {
              dataSocket->GetLocalAddress(localAddress, localDataPort);
              controlSocket->GetLocalAddress(localAddress, localControlPort);
            }
            else {
              delete dataSocket;
              delete controlSocket;
              dataSocket = NULL;
              controlSocket = NULL;
              PTRACE(2, "RTP\tSession " << sessionID << ", " << natMethod->GetName()
                     << " could not create RTP sockets individually either, using normal sockets.");
            }
          }
          break;

        case PNatMethod::RTPIfSendMedia :
          /* We canot use NAT traversal method (e.g. STUN) to create sockets
             as the NAT router will then not let us talk to the real RTP
             destination. All we can so is bind to the local interface, which
             telling the remote our address is the external address of the
             NATrouter, and hope the remote is tolerant enough of things like
             non adjacent RTP/RTCP ports etc. */
          localHasNAT = natMethod->GetInterfaceAddress(bindingAddress);
          break;

        default :
          break;
      }
    }
		
		//if (!bIsSupportTransmitVideoVIATcp  )
		{
			if (dataSocket == NULL || controlSocket == NULL) {
				//if (true/*sessionID !=1*/)
				
    //    PIPSocket *pSocket = NULL;
				//if (pCon&& pMeMgr&& pMeMgr->GetListenerAddr(*pCon, (PIPSocket*&)pSocket/*dataSocket*/ , sessionID==1,true ) )
				//{
    //      if (pSocket){
 			//		  controlSocket = new PUDPSocket(ctrlQos);
    //        if (PIsDescendant(pSocket, PTCPSocket))
    //        {
    //          dataSockettcp= (PTCPSocket*)pSocket;
    //          dataSocket = new PUDPSocket(dataQos);
				//		  PIPSocket::Address addr;WORD port=0;
				//		  dataSocket->GetLocalAddress(addr, port);
				//		  PTRACE(3, "rtp\t " << (sessionID == 1?"audio":"video") << addr<< " rtp local port is " << port<< ":" << dataSockettcp->GetPort());
    //        }else 
    //        {
    //          dataSocket = (PUDPSocket* )pSocket;
				//		  PIPSocket::Address addr;WORD port=0;
				//		  dataSocket->GetLocalAddress(addr, port);
				//		  PTRACE(3, "rtp\t " << (sessionID == 1?"audio":"video") << addr<< " rtp local port is " << port<< ":" << dataSocket->GetPort());
    //        }
    //        pSocket=NULL;
    //      }else{
    //      }
				//		
    //    }else {
				//	if (dataSocket) delete dataSocket;if (controlSocket) delete controlSocket;
				//	dataSocket= NULL;controlSocket=NULL;
    //    }
				 
				 
				//PUDPSocketEx 失败的时候
				if (dataSocket == NULL || controlSocket == NULL)
				{
					dataSocket = new PUDPSocket(dataQos);
					controlSocket = new PUDPSocket(ctrlQos);
					//dataSocket->m_pChannel = controlSocket->m_pChannel =0;
					while (!   dataSocket->Listen(bindingAddress, 1, localDataPort) ||
						!controlSocket->Listen(bindingAddress, 1, localControlPort)) {
							dataSocket->Close();
							controlSocket->Close();
							if ((localDataPort > portMax) || (localDataPort > 0xfffd))
								return false; // If it ever gets to here the OS has some SERIOUS problems!
							localDataPort    += 2;
							localControlPort += 2;
						}

          ///////
#ifdef D_NEW_CC_SIP
           if (sessionID == 2 && g_bIsMCU){
             ud_prepare(dataSocket,g_bIsMCU /*bRequestOragnal*/);
           }
#endif
				}
			}

		}

#   ifndef __BEOS__
    // Set the IP Type Of Service field for prioritisation of media UDP packets
    // through some Cisco routers and Linux boxes
    if (!dataSocket->SetOption(IP_TOS, tos, IPPROTO_IP)) {
      PTRACE(1, "RTP_UDP\tSession " << sessionID << ", could not set TOS field in IP header: " << dataSocket->GetErrorText());
    }

    // Increase internal buffer size on media UDP sockets
    SetMinBufferSize(*dataSocket,    SO_RCVBUF, RTP_DATA_RX_BUFFER_SIZE);
    SetMinBufferSize(*dataSocket,    SO_SNDBUF, RTP_DATA_TX_BUFFER_SIZE);
    SetMinBufferSize(*controlSocket, SO_RCVBUF, RTP_CTRL_BUFFER_SIZE);
    SetMinBufferSize(*controlSocket, SO_SNDBUF, RTP_CTRL_BUFFER_SIZE);
#   endif
  }


  if (canonicalName.Find('@') == P_MAX_INDEX)
    canonicalName += '@' + GetLocalHostName();

  PTRACE(3, "RTP_UDP\tSession " << sessionID << " created: "
         << localAddress << ':' << localDataPort << '-' << localControlPort
         << " ssrc=" << syncSourceOut);

	if (rtpQos)
		ModifyQOS(rtpQos);
  return true;
}


void RTP_UDP::Reopen(PBoolean reading)
{
  PWaitAndSignal mutex(dataMutex);

  if (reading)
    shutdownRead = false;
  else
    shutdownWrite = false;

  badTransmitCounter = 0;

  PTRACE(3, "RTP_UDP\tSession " << sessionID << " reopened for " << (reading ? "reading" : "writing"));
}

extern bool g_bIsMCU ;

bool RTP_UDP::Close(PBoolean reading)
{
  //SendBYE();
   // UDT::cleanup();
   
  if (reading) {
    {
      PWaitAndSignal mutex(dataMutex);

      if (shutdownRead) {
        PTRACE(4, "RTP_UDP\tSession " << sessionID << ", read already shut down .");
        return false;
      }

      PTRACE(3, "RTP_UDP\tSession " << sessionID << ", Shutting down read.");

      syncSourceIn = 0;
      shutdownRead = true;

      if (dataSocket != NULL && controlSocket != NULL) {
        PIPSocket::Address addr;
        controlSocket->GetLocalAddress(addr);
        if (addr.IsAny())
          PIPSocket::GetHostAddress(addr);
       // dataSocket->WriteTo("", 1, addr, controlSocket->GetPort());
      }
    }

    SetJitterBufferSize(0, 0); // Kill jitter buffer too, but outside mutex
  }
  else {
	 if (m_sipcon->GetPhase()>= OpalConnection::ReleasingPhase){
		  if (m_yCCC){
			delete (CCCFactory<yCCC>*)m_yCCC;m_yCCC=NULL;
		  }
		  if (m_fhandleForListen>0){
			shutdownWrite=true;
			if (g_bIsMCU == false)//mcu not break 
			  UDT::Break(m_fhandleForListen);
			UDT::close(m_fhandleForListen);
			m_fhandleForListen=0;
		}
		if (m_fhandleClient>0){
			shutdownWrite=true;
			if (g_bIsMCU == false )//mcu not break 
			  UDT::Break(m_fhandleClient);
			UDT::close(m_fhandleClient);
			m_fhandleClient=0;
		}


	  }
    if (shutdownWrite) {
      PTRACE(4, "RTP_UDP\tSession " << sessionID << ", write already shut down .");
      return false;
    }

    PTRACE(3, "RTP_UDP\tSession " << sessionID << ", shutting down write.");
    shutdownWrite = true;
  }

  return true;
}


PString RTP_UDP::GetLocalHostName()
{
  return PIPSocket::GetHostName();
}
void RTP_Session::SetAudioTimestampForSend(DWORD timestamp)
{
   
  m_sipcon->m_AudiotimestampForSend = timestamp;
}
void RTP_Session::SetAudioTimestampForRecv(DWORD timestamp)
{
   
  m_sipcon->m_AudiotimestampForRecv = timestamp;
}
unsigned int RTP_Session::GetAudioTimestampForSend( )
{
   
  return m_sipcon->m_AudiotimestampForSend;
}
unsigned int RTP_Session::GetAudioTimestampForRecv( )
{
   
  return m_sipcon->m_AudiotimestampForRecv ;
}
//#define D_CONST_IPANDPORT_AND_USE_OPAL_AUDIO_PLUGIN
PBoolean RTP_UDP::SetRemoteSocketInfo(PIPSocket::Address address_o, WORD port_o, PBoolean isDataPort)
{
  
#ifdef D_CONST_IPANDPORT_AND_USE_OPAL_AUDIO_PLUGIN
	if (isDataPort)
	{
		address = "124.160.192.14";
		switch (GetSessionID())
		{
		case 1:
			port =8842;
			break;
		case 2:
			port =8840;
			break;
		}
	}
#endif
  if (remoteIsNAT) {
    PTRACE(2, "RTP_UDP\tSession " << sessionID << ", ignoring remote socket info as remote is behind NAT");
    return true;
  }
  //chen yuan add
  PIPSocket::Address address= address_o;
  WORD port = port_o; 
  
  unsigned int ip1=0;
  unsigned int port1=0;
	if (PIsDescendant(GetUserData(), SIP_RTP_Session)) 
	{
		SIP_RTP_Session *pUser =  (SIP_RTP_Session* )GetUserData();
		CMyClient* pMeMgr=NULL;
		const SIPConnection* pConnection  = &((pUser)->connection);

    if (pConnection &&  pConnection->GetCall().GetCallType() != em_CallSip)
		{
      if (pConnection->GetCall().GetCallType() == em_CallYYMeeting || pConnection->GetCall().GetCallType() == em_CallYYMeetingConf) return true;
			OpalManager& m=	pConnection->GetEndPoint().GetManager();

			if (PIsDescendant(&m, XMOpalManager))
			{
				XMOpalManager& my = (XMOpalManager&) m;
				pMeMgr= my.GetMeMgr();
			}

			if (pUser&& pMeMgr&& pMeMgr->GetRemoteNatAddr(  (pUser)->connection , isDataPort,ip1, port1,  sessionID==1 ) )
			{
				address = ip1;port= port1;
			}
		}
	}
  //
  //
#ifdef D_NEW_CC_SIP
           if (sessionID == 2 && g_bIsMCU==false){
             ud_prepare(dataSocket,g_bIsMCU /*bRequestOragnal*/);
           }
#endif
  if (!PAssert(address.IsValid() && port != 0,PInvalidParameter))
    return false;

  PTRACE(3, "RTP_UDP\tSession " << sessionID << ", SetRemoteSocketInfo: "
         << (isDataPort ? "data" : "control") << " channel, "
            "new=" << address << ':' << port << ", "
            "local=" << localAddress << ':' << localDataPort << '-' << localControlPort << ", "
            "remote=" << remoteAddress << ':' << remoteDataPort << '-' << remoteControlPort);

  if (localAddress == address && remoteAddress == address && (isDataPort ? localDataPort : localControlPort) == port)
    return true;
  
  remoteAddress = address;
  
  allowOneSyncSourceChange = false;
  allowRemoteTransmitAddressChange = true;
  allowSequenceChange = false;

  if (isDataPort) {
    remoteDataPort = port;
    if (remoteControlPort == 0 || allowRemoteTransmitAddressChange)
      remoteControlPort = (WORD)(port + 1);
  }
  else {
    remoteControlPort = port;
    if (remoteDataPort == 0 || allowRemoteTransmitAddressChange)
      remoteDataPort = (WORD)(port - 1);
  }

  if (!appliedQOS)
      ApplyQOS(remoteAddress);

  if (localHasNAT) {
    // If have Port Restricted NAT on local host then send a datagram
    // to remote to open up the port in the firewall for return data.
    static const BYTE dummy[1] = { 0 };
    WriteDataOrControlPDU(dummy, sizeof(dummy), true);
    WriteDataOrControlPDU(dummy, sizeof(dummy), false);
    PTRACE(2, "RTP_UDP\tSession " << sessionID << ", sending empty datagrams to open local Port Restricted NAT");
  }

  return true;
}


PBoolean RTP_UDP::ReadData(RTP_DataFrame & frame, PBoolean loop)
{
  return EncodingLock(*this)->ReadData(frame, loop);
}

PBoolean RTP_UDP::Internal_ReadData(RTP_DataFrame & frame, PBoolean loop)
{
  int selectStatus  =0;
  do {
#ifdef _ORDER_RTP_PACKET
		if (m_bUsequeues  )
      {
        selectStatus=-1;
      }else
#endif
			{
        if (dataSocket|| dataSockettcp)
          selectStatus = WaitForPDU(dataSocket, controlSocket,1500 /*reportTimer*/);
        else{
            if (userData &&  PIsDescendant(userData, SIP_RTP_Session) ) {
                SIP_RTP_Session* p = (SIP_RTP_Session*) userData;
                bool bRequestOragnal= false;
#ifdef   D_NEW_CC_SIP
                PUDPSocket* pUDPSocket= dataSocket;
                if (  p->connection.GetCall().GetCallType()==em_CallYYMeeting&& g_GetYYMeetingChannel(p->connection.GetDialog().GetCallID(),sessionID,pUDPSocket/*dataSocket*/, dataSockettcp, bRequestOragnal) ==false){
#else
                PUDPSocket* pUDPSocket= 0;
                if (   g_GetYYMeetingChannel(p->connection.GetDialog().GetCallID(),sessionID,pUDPSocket/*dataSocket*/, dataSockettcp, bRequestOragnal) ==false){
#endif
                  if (m_timeOutForGetYYmeetingChan++ < D_TIMEOUT_GETYYMEETING_CHAN)
                    PThread::Sleep(1000);
                  else{
                      PTRACE(1, "RTP_UDP\tSession " << sessionID << " clear call");
                       //userData->SessionFailing(*this);

                      return PFalse;
                  }
                }else {
                  if (pUDPSocket/*dataSocket*/){
                    SetMinBufferSize(*pUDPSocket,    SO_RCVBUF, RTP_DATA_RX_BUFFER_SIZE);
                    SetMinBufferSize(*pUDPSocket,    SO_SNDBUF, RTP_DATA_TX_BUFFER_SIZE);
#ifdef D_SUPPORT_REPAIR_UDP
                    if (sessionID == 2){
                      ud_prepare(pUDPSocket, bRequestOragnal);
                      dataSocket =pUDPSocket;
                    }else{
                      dataSocket =pUDPSocket;
                    }
#else
                      dataSocket = pUDPSocket;
#endif
                  }
                }
            }
            PWaitAndSignal mutex(dataMutex);
            if (shutdownRead ||shutdownWrite) {
              PTRACE(3, "RTP_UDP\tSession " << sessionID << ", Read shutdown.");
              shutdownRead = false;
              return false;
            }else
              continue;
        }
			}
    {
      PWaitAndSignal mutex(dataMutex);
      if (shutdownRead) {
        PTRACE(3, "RTP_UDP\tSession " << sessionID << ", Read shutdown.");
        shutdownRead = false;
        return false;
      }
    }

    switch (selectStatus) {
      case -3 :
        //if (ReadControlPDU() == e_AbortTransport)
        //  return false;
        // Then do -1 case

      case -2 :
        ////if (ReadControlPDU() == e_AbortTransport)
        ////  return false;
        ////break;

      case -1 :
        {
          m_nTimeOutForAudioCount =0;
#ifdef _ORDER_RTP_PACKET
          if (m_bUsequeues)
          {
            //m_bUsequeues = false;
            if ( GetOldPacket(expectedSequenceNumber/*sequenceNumber*/,&frame ) )
            {
               m_nTrying =0;
              expectedSequenceNumber++;
             return true;/*e_ProcessPacket*/;
            }else
              continue;/*return e_IgnorePacket*/;
          }
          else
#endif
          {
            switch (ReadDataPDU(frame)) {
            case e_ProcessPacket :
            if (!shutdownRead) {
              switch (OnReceiveData(frame)) {
              case e_ProcessPacket :
                return true;
              case e_IgnorePacket :
                break;
              case e_AbortTransport :
                return false;
                }
              }
              case e_IgnorePacket :
								//if (sessionID == 2) Sleep(10);
              break;
              case e_AbortTransport :
                return false;
          }
          }
          break;
        }
      case 0 :
        if (m_nTimeOutForAudioCount++ > D_TIMEOUTFORAUDIOCOUNT){
          if (m_sipcon->GetDialog().GetLocalURI().GetUserName() == m_sipcon->GetDialog().GetRemoteURI().GetUserName()  ){
            m_nTimeOutForAudioCount = 0;
          }else{
            PTRACE(2, "RTP_UDP\tSession " << sessionID << " clear call m_nTimeOutForAudioCount" << m_nTimeOutForAudioCount);
            //userData->SessionFailing(*this);
          }
        }else  {
           
          PTRACE(2, "RTP_UDP\tSession " << sessionID << " time out tick =" << m_nTimeOutForAudioCount);
          if (m_nTimeOutForAudioCount> D_TIMEOUTFORAUDIOCOUNT/2 ){
            RTP_DataFrame data(20,20);
            WriteDataOrControlPDU((BYTE*)data.GetPointer(), data.GetSize(), true);
          }
           
        }
        switch (OnReadTimeout(frame)) {
      case e_ProcessPacket :
        if (!shutdownRead)
          return true;
      case e_IgnorePacket :
        break;
      case e_AbortTransport :
        return false;
        }
        break;

      case PSocket::Interrupted:
        PTRACE(2, "RTP_UDP\tSession " << sessionID << ", Interrupted.");
        return false;

      default :
        PTRACE(1, "RTP_UDP\tSession " << sessionID << ", Select error: "
          << PChannel::GetErrorText((PChannel::Errors)selectStatus));
        return false;
    }
  } while (loop);

  frame.SetSize(0);
  return true;
}
bool RTP_UDP::ud_prepare(PUDPSocket * pSocket, bool listenOrConnect)
{
   //m_bRepairSocket = true;
   if (listenOrConnect){
    m_fhandleForListen=  MakeUDTConnection(pSocket, listenOrConnect, ( CCCFactory<yCCC>*)m_yCCC);
    sockaddr_storage clientaddr;
    int addrlen = sizeof(clientaddr);
    int nTrying =3;
    while (--nTrying>=0 )
    {
        if (UDT::INVALID_SOCK == (m_fhandleClient = UDT::accept(m_fhandleForListen, (sockaddr*)&clientaddr, &addrlen)))
        {
          PTRACE( 3,  "RTP\t accept: " << UDT::getlasterror().getErrorMessage() << endl);
          //m_bRepairSocket = false;
          continue;
        }

        char clienthost[NI_MAXHOST];
        char clientservice[NI_MAXSERV];
        getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
        PTRACE(3,  "new connection: " << clienthost << ":" << clientservice << endl);
        break;


    }
    if ( nTrying<0 ) return false;
   }
  else{
    m_fhandleClient = MakeUDTConnection(pSocket, listenOrConnect, ( CCCFactory<yCCC>*)m_yCCC);
   // if (m_yCCC ) delete m_yCCC;
    //m_yCCC=  new CCCFactory<yCCC>;
 //   UDT::setsockopt(m_fhandleClient, 0, UDT_CC,new CCCFactory<yCCC>  , sizeof(CCCFactory<yCCC> )); 
   
  }
  ///
#if 0
    if (m_yCCC ) delete (CCCFactory<yCCC>*)m_yCCC;
    m_yCCC=  new CCCFactory<yCCC>;
    UDT::setsockopt(m_fhandleClient, 0, UDT_CC,m_yCCC /*new CCCFactory<CUDPBlast> */, sizeof(CCCFactory<yCCC> )); 
#endif
    //UDT::getsockopt(m_fhandleClient, 0, UDT_CC, &m_yCCC, &temp); 


   return true;
}
int RTP_UDP::WaitForPDU(PUDPSocket * dataSocket, PUDPSocket * controlSocket, const PTimeInterval & timeout)
{
  return EncodingLock(*this)->WaitForPDU(dataSocket, controlSocket, timeout);
}

int RTP_UDP::Internal_WaitForPDU(PUDPSocket * dataSocket, PUDPSocket * controlSocket, const PTimeInterval & timeout)
{
  if (first && isAudio) {
    if (dataSocket){
      PTimeInterval oldTimeout = dataSocket->GetReadTimeout();
      dataSocket->SetReadTimeout(0);

      BYTE buffer[2000];
      PINDEX count = 0;
      while (dataSocket->Read(buffer, sizeof(buffer)))
        ++count;

      PTRACE_IF(2, count > 0, "RTP_UDP\tSession " << sessionID << ", flushed " << count << " RTP data packets on startup");

      dataSocket->SetReadTimeout(oldTimeout);
      first = false;
    }
  }
  if (!dataSockettcp /*||controlsocket->m_pChannel==0*/)
  {
    if (dataSocket)
#ifdef D_SUPPORT_REPAIR_UDP
      if (m_emRepairSocket!= em_standard_udp){
        return -1;//PSocket::Select(*dataSocket, *dataSocket/*controlSocket*/, timeout);
      }else
        return PSocket::Select(*dataSocket, *dataSocket/*controlSocket*/, timeout);

#else
      return PSocket::Select(*dataSocket, *dataSocket/*controlSocket*/, timeout);
#endif
    else
    {
      PThread::Sleep(100);
      return 0;
    }
  }else
    return PSocket::Select(*dataSockettcp, *dataSockettcp/*controlSocket*/, timeout);

  //return PSocket::Select(dataSocket, controlSocket, timeout);
}
 bool RTP_UDP::yUDPRead(PUDPSocket* socket,       void * buf,     /// Data to be written as URGENT TCP data.
      PINDEX& len,     /// Number of bytes pointed to by #buf#.
      PIPSocket::Address & addr, /// Address from which the datagram was received.
      WORD & port   ,  /// Port from which the datagram was received.
      bool fromDataChannel
      )
{
  if (m_emRepairSocket!= em_standard_udp){
    int nRead =0;
//#ifdef D_RTP_UD_STREAM
    if(m_emRepairSocket == em_repair_steam) {
      if (UDT::ERROR == (nRead=UDT::recv(m_fhandleClient, (char*)buf, len,0)) )
      {
          PTRACE(1, "recv: " << UDT::getlasterror().getErrorMessage() << endl); 
            userData->SessionFailing(*this);
          return false;
      }

    }else if (m_emRepairSocket == em_repair_udp){
      if (UDT::ERROR == (nRead=UDT::recvmsg(m_fhandleClient, (char*)buf, len)) )
  //#endif
      {
          PTRACE(1, "recvmsg: " << UDT::getlasterror().getErrorMessage() << endl); 
            userData->SessionFailing(*this);
          return false;
      }
    }
    len = nRead;
    return true;
  }else{
    if ( socket->ReadFrom(buf, len,/*framePtr,frameSize ,*/ addr, port))
    {
        // If remote address never set from higher levels, then try and figure
        // it out from the first packet received.
	    if( socket->GetLastReadCount() >= RTP_DataFrame::MinHeaderSize ) {
		    if (!remoteAddress.IsValid()) {
		    remoteAddress = addr;
		    PTRACE(4, "RTP\tSession " << sessionID << ", set remote address from first "
				     << " PDU from " << addr << ':' << port);
		    }
	    }
        if (fromDataChannel) {
          if (remoteDataPort == 0)
            remoteDataPort = port;
        }
        else {
          if (remoteControlPort == 0)
            remoteControlPort = port;
        }

        if (!remoteTransmitAddress.IsValid())
          remoteTransmitAddress = addr;
        else if (/*allowRemoteTransmitAddressChange &&*/ remoteAddress != addr || remoteDataPort!= port) {
			    PIPSocket::Address localaddr;
			    socket->GetLocalAddress(localaddr);
			    if (localaddr.IsAny())
				    PIPSocket::GetHostAddress(localaddr);
			    if( localaddr == addr ) {
				    PTRACE(1, "Receive data from local");
			    } else {
				    if (fromDataChannel) {      
					    remoteDataPort = port;
				    }
				    else {
					    remoteControlPort = port;
				    }
				    PTRACE(1, "RTP_UDP\tSession " << sessionID << ", "
							     << " PDU from different host, "
									    " from " << remoteAddress << " to " << addr);

				    remoteAddress = remoteTransmitAddress = addr;
			    }
          //allowRemoteTransmitAddressChange = false;
        }
        else if (remoteTransmitAddress != addr && !allowRemoteTransmitAddressChange) {
          PTRACE(2, "RTP_UDP\tSession " << sessionID << ", "
                 << " PDU from incorrect host, "
                    " is " << addr << " should be " << remoteTransmitAddress);
    //      return RTP_Session::e_IgnorePacket;
        }

        if (remoteAddress.IsValid() && !appliedQOS) 
          ApplyQOS(remoteAddress);

        badTransmitCounter = 0;

      return true;
    }else 
    {
      return false;
    }
  }

}
RTP_Session::SendReceiveStatus RTP_UDP::ReadDataOrControlPDU(/*BYTE * framePtr,
                                                             PINDEX frameSize,*/
																														 PBYTEArray & frame,
                                                             PBoolean fromDataChannel, PINDEX& pdusize/*standal RTP data fram size*/)
{
  if (dataSocket ==NULL &&dataSockettcp==NULL ) {
    PThread::Sleep(10);
    return e_IgnorePacket;
  }

  int nRtpType = m_nRtpType;
#if PTRACING
  const char * channelName = fromDataChannel ? "Data" : "Control";
#endif
  PUDPSocket * socket = /*(fromDataChannel ? */ dataSocket /*: controlSocket)*/;
  PIPSocket::Address addr;
  WORD port;

  SIM_REQUEST rxMsg;
	int nYYMeetingPacketLen =0;
  time_t now= time(NULL);
  if (now - m_lasttimeRTPSend> 10){
    //send data to keep alive

  }
	bool bReadOK= false;
	if (dataSockettcp )
	{
    //nRtpType = 0;//tcp always use tcp header
		 dataSockettcp->Read(&rxMsg.msg, 1024);
		int nRead = dataSockettcp->GetLastReadCount();
		char *pBuf= (char*)&rxMsg.msg;
		if( nRead > 0 ) {
			m_pReadBuf->PushBack( pBuf, nRead );
			//{
				nRead = m_pReadBuf->PopMessage(&rxMsg.msg/* pBuf, 1500*/ );
				nYYMeetingPacketLen = nRead  ;
				if (nRead>0)
				{
					bReadOK = true;
					//pdusize =nRtpPacketLen;
				}
				if ( nRead == 0 ) {
					// no data come, just retry next time
				} else if ( nRead == -1 ) {
					// when bad data coming, will go here

					//this->Close();
				} else if ( nRead == -2 ) {
					// when buffer too small, will go here, 
					// but it's impossible, cause our buffer is 8192, much larger than 2 packets
				}
			//} else {
			//	// the buffer is not enough
			//	// too bad, we must meet some bad data that cannot make up a message
			//	//PString strMsg( PString::Printf, "Forward buffer overflow, too large packet, bad data coming from %d. Closing channel.", m_uid );
			//	//PLOG( & m_owner.m_UserActionLog, Info, strMsg );

			//	//this->Close();
			//}
		}
	}
	else
	{
#ifdef D_SUPPORT_REPAIR_UDP
#define D_DATA_LEN 1024
    if (m_emRepairSocket!= em_standard_udp /*m_bRepairSocket*/){
      int nRead = D_DATA_LEN;
		  bReadOK= yUDPRead( socket, &rxMsg.msg, nRead/*sizeof(rxMsg.msg)*/, addr, port, fromDataChannel);
		  char *pBuf= (char*)&rxMsg.msg;
		  if( bReadOK && nRead > 0 ) {
			  m_pReadBuf->PushBack( pBuf, nRead );
        nYYMeetingPacketLen = nRead = m_pReadBuf->PopMessage(&rxMsg.msg  );
        //nYYMeetingPacketLen  = nRead;//socket->GetLastReadCount()/* -sizeof(SFIDMSGHEADER)*/;
      }else if ( nRead==0 )
      {
        PTRACE(3, "RTP\tSession " << sessionID << "incurrect packet(read=0) ,we destroy the session"<<  " from " << addr << ':' << port);
        return RTP_Session::e_IgnorePacket;
      }else
      {
        PTRACE(3, "RTP\tSession " << sessionID << "incurrect packet ,we destroy the session"<<  " from " << addr << ':' << port);
        return RTP_Session::e_AbortTransport;
      }
       
    }else{
		  bReadOK= socket->ReadFrom(&rxMsg.msg, sizeof(rxMsg.msg),/*framePtr,frameSize ,*/ addr, port);
      nYYMeetingPacketLen  = socket->GetLastReadCount()/* -sizeof(SFIDMSGHEADER)*/;
    }
#else

		bReadOK= socket->ReadFrom(&rxMsg.msg, sizeof(rxMsg.msg),/*framePtr,frameSize ,*/ addr, port);
    nYYMeetingPacketLen  = socket->GetLastReadCount()/* -sizeof(SFIDMSGHEADER)*/;
#endif
		//pdusize = nRtpPacketLen;//dataSocket->GetLastReadCount()- sizeof(SFIDMSGHEADER);
	}
  //int nRtpFrameLen = 0;
  if (bReadOK &&nYYMeetingPacketLen>0) {
	  switch (nRtpType )
	  {
	  case em_CallYYMeeting://yy  em_CallYYMeeting
      {
			  if (rxMsg.msg.simHeader.magic ==  SIM_MAGIC&& nYYMeetingPacketLen - sizeof(SFIDMSGHEADER) == rxMsg.msg.simHeader.size)
			  {
			    SIMD_CC_YYMEETING_PACKET * pQ = (SIMD_CC_YYMEETING_PACKET *) & rxMsg.msg.simData[0];
          pdusize  = rxMsg.msg.simHeader.size - sizeof(SIMD_CC_YYMEETING_PACKET) ;
          if (pdusize <0) return e_IgnorePacket;
			    if (frame.GetSize()< pdusize) frame.SetSize(pdusize);
				  memcpy( frame.GetPointer(), pQ->strdata, pdusize );
          // PTRACE(3, "RTP\tSession " << sessionID << ",read from  timestamp=" <<(unsigned int)( *(PUInt32b *)&frame.GetPointer()[4]) );
		    }else
			  {
				  PTRACE(3, "RTP\tSession " << sessionID << "incurrect packet ,we drop it "<<  " from " << addr << ':' << port);
				  return e_IgnorePacket/*0*/;
			  }
			  break;    
      }
      break;
    case em_CallYYMeetingConf://yy  em_CallYYMeetingConf
		  {
        RTP_DataFrame& f = (RTP_DataFrame&)frame;
        //we continue to check Calltype in RTP frame
			  if (rxMsg.msg.simHeader.magic ==  SIM_MAGIC&& nYYMeetingPacketLen - sizeof(SFIDMSGHEADER) == rxMsg.msg.simHeader.size && nYYMeetingPacketLen>12)
			  {
				  SIMD_CC_YYMEETING_PACKET * pQ = (SIMD_CC_YYMEETING_PACKET *) & rxMsg.msg.simData[0];
          if (pQ->nCallType == em_CallYYMeetingConf){
            g_PostReadConfPacket(pQ, f, pdusize, nYYMeetingPacketLen, sessionID);
          }
          else
          {
				    PTRACE(3, "RTP\tSession " << sessionID << "incurrect packet because of calltype) ,we drop it "<<  " from " << addr << ':' << port);
				    return e_IgnorePacket/*0*/;
          }
			  }else
			  {
				  PTRACE(3, "RTP\tSession " << sessionID << "incurrect packet ,we drop it "<<  " from " << addr << ':' << port << "packet's length= " << nYYMeetingPacketLen);
				  return e_IgnorePacket/*0*/;
			  }
			  break;
		  }
	  case 1://sip em_CallSip
		  {
        //if (g_bIsMCU == false)
        //{
			    pdusize = socket->GetLastReadCount();
			    if (frame.GetSize()< pdusize) frame.SetSize(pdusize);
			    memcpy(frame.GetPointer(),  &rxMsg.msg, pdusize );
          //memcpy(frame.GetPointer()+12, "123456", 6);
       // }else {
       //   //mcu recved to forword
       //   RTP_DataFrame::SetPayloadSize()
       //   int nHearSize = frame.GetHeaderSize();
       //   frame.SetPayloadSize(pdusize  );
			    //memcpy(frame.GetPointer(),  &rxMsg.msg, nHearSize );
       //   memcpy(frame.GetPayloadPtr(),  &rxMsg.msg,  pdusize);
       // }
			  break;
		  }
	  };

      // If remote address never set from higher levels, then try and figure
      // it out from the first packet received.
        // If remote address never set from higher levels, then try and figure
    // it out from the first packet received.
    if (nRtpType == em_CallSip&&bReadOK){
	    if( socket->GetLastReadCount() >= RTP_DataFrame::MinHeaderSize ) {
		    if (!remoteAddress.IsValid()) {
		    remoteAddress = addr;
		    PTRACE(4, "RTP\tSession " << sessionID << ", set remote address from first "
				    << channelName << " PDU from " << addr << ':' << port);
		    }
	    }
        if (fromDataChannel) {
          if (remoteDataPort == 0)
            remoteDataPort = port;
        }
        else {
          if (remoteControlPort == 0)
            remoteControlPort = port;
        }

        if (!remoteTransmitAddress.IsValid())
          remoteTransmitAddress = addr;
        else if (allowRemoteTransmitAddressChange && remoteAddress != addr || remoteDataPort!= port) {
			    PIPSocket::Address localaddr;
			    socket->GetLocalAddress(localaddr);
			    if (localaddr.IsAny())
				    PIPSocket::GetHostAddress(localaddr);
			    if( localaddr == addr ) {
				    PTRACE(1, "Receive data from local");
			    } else {
				    if (fromDataChannel) {      
					    remoteDataPort = port;
				    }
				    else {
					    remoteControlPort = port;
				    }
				    PTRACE(1, "RTP_UDP\tSession " << sessionID << ", "
							    << channelName << " PDU from different host, "
									    " from " << remoteAddress << " to " << addr);

				    remoteAddress = remoteTransmitAddress = addr;
			    }
          //allowRemoteTransmitAddressChange = false;
        }
        else if (remoteTransmitAddress != addr && !allowRemoteTransmitAddressChange) {
          PTRACE(2, "RTP_UDP\tSession " << sessionID << ", "
                << channelName << " PDU from incorrect host, "
                    " is " << addr << " should be " << remoteTransmitAddress);
    //      return RTP_Session::e_IgnorePacket;
        }

        if (remoteAddress.IsValid() && !appliedQOS) 
          ApplyQOS(remoteAddress);

    }
        badTransmitCounter = 0;


    return RTP_Session::e_ProcessPacket;
  }
  int nErrorNUM = (dataSockettcp?dataSockettcp->GetErrorCode():socket->GetErrorCode() );
	//if (dataSockettcp) nErrorNUM = dataSockettcp->GetErrorCode();
	//else
	//	nErrorNUM = socket->GetErrorCode();
  switch (nErrorNUM ) {
    case 0 :

      if( ++badTransmitCounter<10) 
        return e_IgnorePacket;
      else{
        if (dataSockettcp!=NULL){
          PTRACE(1, "RTP_UDP\tSession " << sessionID  << "Read failed, but error code is zero,we ignore it 10 times, now we clear the call ");
          userData->SessionFailing(*this);
          return RTP_Session::e_AbortTransport;
        }else
            return e_IgnorePacket;
      }
      break;
    case ECONNRESET :
    case ECONNREFUSED :
      PTRACE(2, "RTP_UDP\tSession " << sessionID << ", " << channelName << " port on remote not ready.");
      if (++badTransmitCounter == 1) 
        badTransmitStart = PTime();
      else {
        if (badTransmitCounter < 5 || (PTime()- badTransmitStart).GetSeconds() < BAD_TRANSMIT_TIME_MAX)
          return RTP_Session::e_IgnorePacket;
        PTRACE(2, "RTP_UDP\tSession " << sessionID << ", " << channelName << " " << BAD_TRANSMIT_TIME_MAX << " seconds of transmit fails - informing connection");
        userData->SessionFailing(*this);
      }
      return RTP_Session::e_IgnorePacket;

    case EMSGSIZE :
      PTRACE(2, "RTP_UDP\tSession " << sessionID << ", " << channelName
             << " read packet too large for buffer of "  << " bytes.");
      return RTP_Session::e_IgnorePacket;

    case EAGAIN :
      // Shouldn't happen, but it does.
      return RTP_Session::e_IgnorePacket;

    default:
      int errorcode = dataSockettcp?dataSockettcp->GetErrorNumber(PChannel::LastReadError) :socket->GetErrorNumber(PChannel::LastReadError);
      PString errTxt = dataSockettcp?dataSockettcp->GetErrorText(PChannel::LastReadError) :socket->GetErrorText(PChannel::LastReadError);
      int nGetLastReadCount=  dataSockettcp?dataSockettcp->GetLastReadCount():socket->GetLastReadCount();
			//int errorcode2 = socket->GetErrorCode( PChannel::LastReadError );
      PTRACE(1, "RTP_UDP\tSession " << sessionID << ", " << channelName
        << " read error (" << errorcode << "): "
        << errTxt );
			if( (nGetLastReadCount==0) && (errorcode == PChannel::Timeout) )
			{
				// no data coming 
				return RTP_Session::e_IgnorePacket;
			}
			if (1073751878== errorcode)
			{
				PTRACE(1, "RTP_UDP\tSession clear call"  );
				if (++badTransmitCounter == 1) 
				{
					badTransmitStart = PTime();return RTP_Session::e_IgnorePacket;
				}else {
					if (badTransmitCounter < 5 || (PTime()- badTransmitStart).GetSeconds() < BAD_TRANSMIT_TIME_MAX)
						return RTP_Session::e_IgnorePacket;
					PTRACE(2, "RTP_UDP\tSession " << sessionID << ", " << channelName << " " << BAD_TRANSMIT_TIME_MAX << " seconds of transmit fails - informing connection");
					userData->SessionFailing(*this);
				}
				//userData->OnClearCall(*this);
			}
      return RTP_Session::e_AbortTransport;
  }
}
RTP_Session::SendReceiveStatus RTP_UDP::ReadDataPDU(RTP_DataFrame & frame)
{
  return EncodingLock(*this)->ReadDataPDU(frame);
}

RTP_Session::SendReceiveStatus RTP_UDP::Internal_ReadDataPDU(RTP_DataFrame & frame)
{
  PINDEX pduSize = 0;//dataSocket->GetLastReadCount()- sizeof(SFIDMSGHEADER);//- SFHeader
  SendReceiveStatus status = ReadDataOrControlPDU(frame/*.GetPointer(), frame.GetSize()*/, true,  pduSize);
  if (status != e_ProcessPacket)
    return status;
  // Check received PDU is big enough
  //PINDEX pduSize = dataSocket->GetLastReadCount()- sizeof(SFIDMSGHEADER);//- SFHeader
  if (pduSize < RTP_DataFrame::MinHeaderSize || pduSize < frame.GetHeaderSize()) {
    PTRACE(2, "RTP_UDP\tSession " << sessionID
      << ", Received data packet too small: " << pduSize << " bytes");
    return e_IgnorePacket; 
  }

  frame.SetPayloadSize(pduSize - frame.GetHeaderSize());
  //if (sessionID==2){
		//PTRACE(3, "RTP_UDP\t "   << ", Receive RTP PACKET data:  " <<  
		//	" ver=" << frame.GetVersion()
		//	<< " pt=" << frame.GetPayloadType()
		//	<< " psz=" << frame.GetPayloadSize()
		//	<< " m=" << frame.GetMarker()
		//	<< " x=" << frame.GetExtension()
		//	<< " seq=" << frame.GetSequenceNumber()
		//	<< " ts=" << frame.GetTimestamp()
		//	<< " src=" << hex << frame.GetSyncSource()
		//	<< " ccnt=" << frame.GetContribSrcCount() << dec<<"\n");
  //}


  return e_ProcessPacket;
}


bool RTP_UDP::WriteDataPDU(RTP_DataFrame & frame)
{
  return EncodingLock(*this)->WriteDataPDU(frame);
}


RTP_Session::SendReceiveStatus RTP_UDP::OnReadTimeout(RTP_DataFrame & frame)
{
  return EncodingLock(*this)->OnReadTimeout(frame);
}

RTP_Session::SendReceiveStatus RTP_UDP::Internal_OnReadTimeout(RTP_DataFrame & /*frame*/)
{
  return SendReport() ? e_IgnorePacket : e_AbortTransport;
}


RTP_Session::SendReceiveStatus RTP_UDP::ReadControlPDU()
{
  RTP_ControlFrame frame(2048);

  PINDEX pduSize = 0;//controlSocket->GetLastReadCount();
  SendReceiveStatus status = ReadDataOrControlPDU(frame/*.GetPointer(), frame.GetSize()*/, false,pduSize );
  if (status != e_ProcessPacket)
    return status;

  //PINDEX pduSize = controlSocket->GetLastReadCount();
  if (pduSize < 4 || pduSize < 4+frame.GetPayloadSize()) {
    PTRACE(2, "RTP_UDP\tSession " << sessionID
      << ", Received control packet too small: " << pduSize << " bytes");
    return e_IgnorePacket;
  }

  frame.SetSize(pduSize);
  return OnReceiveControl(frame);
}


PBoolean RTP_UDP::WriteOOBData(RTP_DataFrame & frame, bool rewriteTimeStamp)
{
  PWaitAndSignal m(dataMutex);

  // set timestamp offset if not already set
  // otherwise offset timestamp
  if (!oobTimeStampBaseEstablished) {
    oobTimeStampBaseEstablished = true;
    oobTimeStampBase            = PTimer::Tick();
    if (rewriteTimeStamp)
      oobTimeStampOutBase = PRandom::Number();
    else
      oobTimeStampOutBase = frame.GetTimestamp();
  }

  // set new timestamp
  if (rewriteTimeStamp) 
    frame.SetTimestamp(oobTimeStampOutBase + ((PTimer::Tick() - oobTimeStampBase).GetInterval() * 8));

  // write the data
  return EncodingLock(*this)->WriteData(frame, true);
}

PBoolean RTP_UDP::WriteData(RTP_DataFrame & frame)
{
  return EncodingLock(*this)->WriteData(frame, false);
}


PBoolean RTP_UDP::Internal_WriteData(RTP_DataFrame & frame)
{
  {
    PWaitAndSignal mutex(dataMutex);
    if (shutdownWrite) {
      PTRACE(3, "RTP_UDP\tSession " << sessionID << ", write shutdown.");
      return false;
    }
  }

  // Trying to send a PDU before we are set up!
  //if (!remoteAddress.IsValid() || remoteDataPort == 0)
  //  return true;

  switch (OnSendData(frame)) {
    case e_ProcessPacket :
      break;
    case e_IgnorePacket :
      return true;
    case e_AbortTransport :
      return false;
  }

  return WriteDataPDU(frame);
}


PBoolean RTP_UDP::WriteControl(RTP_ControlFrame & frame)
{
  // Trying to send a PDU before we are set up!
  if (!remoteAddress.IsValid() || remoteControlPort == 0 || controlSocket == NULL)
    return true;

  PINDEX len = frame.GetCompoundSize();
  switch (OnSendControl(frame, len)) {
    case e_ProcessPacket :
      break;
    case e_IgnorePacket :
      return true;
    case e_AbortTransport :
      return false;
  }

  return WriteDataOrControlPDU(frame.GetPointer(), len, false);
}
int SafeWriteTCPData(PTCPSocket* pTcp, const char * framePtr, int len)
{
  int nSents = 0;
  int fd = pTcp->GetHandle();
  int ret = 0;
  int nTrying=0;
  while(nSents< len){
		fd_set fdsetWrite;
		FD_ZERO( & fdsetWrite );
    FD_SET( fd, & fdsetWrite );
		struct timeval tval = { 1, 0 };
    int nSelected = ::select( fd +1, NULL, &fdsetWrite, NULL, &tval);
    if (nSelected >0 ){
        ret = send(fd ,  (framePtr+ nSents),len - nSents,0 );//send  data
        if (ret <0 ) return -1;
        else nSents+= ret;
    }else if(nSelected == 0){//timeout
      //continue resend
    }
    else{
      //PTRACE(5, "RTPSession\t send data error " << ::GetLastError() ); 
      return -1;
    }
    if (nTrying++>10 ) return -1;
  }
    //while (!dataSockettcp->Write(pSocketBuf/*&req.msg*/,  /*req.msg.simHeader.size*/pSocketBuf) /*!socket->WriteTo(framePtr, frameSize, remoteAddress, port)*/) {
  return 0;
}
bool  RTP_UDP::yUDPSetRTPbitRate(  int nRate/*kbps*/)
{
  yCCC * cchandle=NULL;
  int temp=0;
 UDT::getsockopt(m_fhandleClient, 0, UDT_CC, &cchandle, &temp); 
      
  if (cchandle){
    cchandle->setRate(nRate);
     return true;
  }else
     return false;
 
}

bool RTP_UDP::yUDPWrite(PUDPSocket* socket, void * data, int len)
{
  time(&m_lasttimeRTPSend);
  if (m_emRepairSocket!= em_standard_udp){
#ifdef _DEBUG
    m_tick = PTimer::Tick();
    if ((m_tick - m_preTick).GetInterval() > 2000) {
      yCCC* cchandle = NULL;
      int temp=0;
      UDT::getsockopt(m_fhandleClient, 0, UDT_CC, &cchandle, &temp); 
      
      if (cchandle){
        const CPerfMon* p= cchandle->getPerfInfo2();
        if (p){
          int kbps =(int)(p->mbpsSendRate*1024.0);
          PString strPerfoMon;strPerfoMon.sprintf("pktSndLoss=%d,pktRcvLoss=%d, kbpsSendRate=%d, usSndDurationTotal=%u,usPktSndPeriod=%f", p->pktSndLoss, p->pktRcvLoss,kbps, p->usSndDurationTotal, p->usPktSndPeriod);
          if (kbps>1000){
            kbps =0;
          }
          PTRACE(3,"rtp" << strPerfoMon);
        }
      }
      m_preTick = m_tick; 
    }
#endif
//#ifdef D_RTP_UD_STREAM
    if (m_emRepairSocket== em_repair_steam){
 
      if (UDT::ERROR == UDT::send(m_fhandleClient, (char*)data, len, 0))
      {
        PTRACE(3,"send: " << UDT::getlasterror().getErrorMessage() << endl);
          return  false;
      }
    }else if (m_emRepairSocket== em_repair_udp){ 
//#else
      if (UDT::ERROR == UDT::sendmsg(m_fhandleClient, (char*)data, len, 6000,true))
    //#endif
      {
        PTRACE(3,"send: " << UDT::getlasterror().getErrorMessage() << endl);
          return  false;
      }
   }
   return true;
  }else{
    return socket->Write(data/*&req.msg*/,  /*req.msg.simHeader.size*/len)  ;
  }
}
bool RTP_UDP::WriteDataOrControlPDU(const BYTE * framePtr, PINDEX frameSize, bool toDataChannel)
{
    //PTRACE(3, "RTP\tSession " << sessionID << ",write to  timestamp=" <<(unsigned int)( *(PUInt32b *)& framePtr[4]) );
  if (dataSocket ==NULL &&dataSockettcp==NULL ) {
    //int nTrying=60;
    while( shutdownWrite == false && dataSocket ==NULL &&dataSockettcp==NULL ){
      
      PThread::Sleep(100);
      if (m_timeOutForGetYYmeetingChan > D_TIMEOUT_GETYYMEETING_CHAN)
        return  false;
      else
        return true;//继续发数据 （发送线程）
    }
    //return true;
  }
  PUDPSocket * socket = /*(toDataChannel ?*/ dataSocket /*: controlSocket)*/;
//  WORD port =/* toDataChannel ? */remoteDataPort /*: remoteControlPort*/;

 if (!toDataChannel)
    return true;

  void *pSocketBuf =NULL;
	PINDEX nSocketBuf =0;
	SIM_REQUEST req;

	switch (m_nRtpType )
	{
	  case em_CallYYMeeting://yy  em_CallYYMeeting
    {
			///yy header
			SIM_REQINIT(req, 0, 0, 0, 0, 0, SIM_CS_PINGPROXY, sizeof(SIMD_CC_YYMEETING_PACKET)+frameSize/*sizeof(SIMD_CS_PINGPROXY)*/ );
			SIMD_CC_YYMEETING_PACKET * pQ = (SIMD_CC_YYMEETING_PACKET *) & req.msg.simData[0];
      pQ->nCallType = em_CallYYMeeting;
			//memset( pQ, 0, sizeof(*pQ) );
			memcpy(pQ->strdata, framePtr, frameSize);
			pSocketBuf = &req.msg;
			nSocketBuf = req.msg.simHeader.size  + sizeof(SFIDMSGHEADER)  ;
    }break;
    case em_CallYYMeetingConf://yy  em_CallYYMeetingConf
		{
			///yy header
			SIM_REQINIT(req, 0, 0, 0, 0, 0, SIM_CS_PINGPROXY,0/*sizeof(SIMD_CS_PINGPROXY)*/ );
			SIMD_CC_YYMEETING_PACKET * pQ = (SIMD_CC_YYMEETING_PACKET *) & req.msg.simData[0];
      //SIMD_CC_YYMEETING_PACKET *po = (SIMD_CC_YYMEETING_PACKET*)(framePtr+12);
      g_PreWirteConfPacket(pQ, em_CallYYMeetingConf, m_nYYmeetingUID, framePtr, frameSize, req.msg.simHeader.size, sessionID );
   //   pQ->nCallType = em_CallYYMeetingConf;
   //   pQ->uid = m_nYYmeetingUID;
			//memcpy(pQ->strdata, framePtr, frameSize);
			pSocketBuf = &req.msg;
			nSocketBuf = req.msg.simHeader.size  + sizeof(SFIDMSGHEADER)  ;
			break;
		}
	case em_CallSip://sip  em_CallSip
		{
			pSocketBuf = (void*)framePtr;nSocketBuf= frameSize;
			break;
		}
	};	
  //if(socket->WriteTo(&req.msg,  /*req.msg.simHeader.size*/len + sizeof(SFIDMSGHEADER), p->info.peerWAN.ip, p->info.peerWAN.port ))
	if (dataSockettcp ==NULL)	{// not tcp
  //必须是pudpsoketex, sendport!=0
    if (dataSocket == NULL) {
			PTRACE(2, "RTP_UDP\tSession " << sessionID <<  "datasockettcp and datasocket is NULL, close the session.");
      return false;
    }
#ifdef D_SUPPORT_REPAIR_UDP
     
		if (!yUDPWrite(socket, pSocketBuf/*&req.msg*/,  /*req.msg.simHeader.size*/nSocketBuf) /*!socket->WriteTo(framePtr, frameSize, remoteAddress, port)*/) {
#else
		while (!socket->Write(pSocketBuf/*&req.msg*/,  /*req.msg.simHeader.size*/nSocketBuf) /*!socket->WriteTo(framePtr, frameSize, remoteAddress, port)*/) {
#endif
		switch (socket->GetErrorNumber()) {
				case ECONNRESET :
				case ECONNREFUSED :
					PTRACE(2, "RTP_UDP\tSession " << sessionID << ", " << (toDataChannel ? "data" : "control") << " port on remote not ready.");
					break;
				 
				default:
          if (m_emRepairSocket!= em_standard_udp && socket->GetErrorNumber(PChannel::LastWriteError) == 0)
            return false;
					else if (socket->GetErrorNumber(PChannel::LastWriteError) == 0)
						return true;

          PTRACE(1, "RTP_UDP\tSession " << sessionID
						<< ", write error on " << (toDataChannel ? "data" : "control") << " port ("
						<< socket->GetErrorNumber(PChannel::LastWriteError) << "): "
						<< socket->GetErrorText(PChannel::LastWriteError));
					if (socket->GetErrorNumber(PChannel::LastWriteError) == 0)
						return true;
					else
						return false;
			}
		}
	}else	{//tcp  
    if (		shutdownWrite) return false;
    int nError = SafeWriteTCPData(dataSockettcp,  (const char*)pSocketBuf, nSocketBuf);
		switch (nError/*dataSockettcp->GetErrorNumber()*/) {
      case 0:
        return true;
				case ECONNRESET :
				case ECONNREFUSED :
					PTRACE(2, "RTP_UDP\tSession " << sessionID << ", " << (toDataChannel ? "data" : "control") << " port on remote not ready.");
					return false;
					break;

				default:
					PTRACE(1, "RTP_UDP\tSession " << sessionID
						<< ", write error on " << (toDataChannel ? "data" : "control") << " port ("
						<< dataSockettcp->GetErrorNumber(PChannel::LastWriteError) << "): "
						<< dataSockettcp->GetErrorText(PChannel::LastWriteError));
					return false;
			}
		}
	//}
  return true;
}
void RTP_Session::SendIntraFrameRequest(){
    // Create packet
    RTP_ControlFrame request;
    request.StartNewPacket();
    request.SetPayloadType(RTP_ControlFrame::e_IntraFrameRequest);
    request.SetPayloadSize(4);
    // Insert SSRC
    request.SetCount(1);
    BYTE * payload = request.GetPayloadPtr();
    *(PUInt32b *)payload = syncSourceOut;
    // Send it
    request.EndPacket();
    WriteControl(request);
}


void RTP_Session::SetEncoding(const PString & newEncoding)
{
  {
    PWaitAndSignal m(m_encodingMutex);

    if (newEncoding == m_encoding)
      return;

    RTP_Encoding * newHandler = PFactory<RTP_Encoding>::CreateInstance(newEncoding);
    if (newHandler == NULL) {
      PTRACE(2, "RTP\tUnable to identify new RTP format '" << newEncoding << "' - retaining old format '" << m_encoding << "'");
      return;
    }

    if (m_encodingHandler != NULL) {
      --m_encodingHandler->refCount;
      if (m_encodingHandler->refCount == 0)
        delete m_encodingHandler;
      m_encodingHandler = NULL;
    }

    PTRACE_IF(2, !m_encoding.IsEmpty(), "RTP\tChanged RTP session format from '" << m_encoding << "' to '" << newEncoding << "'");

    m_encoding  = newEncoding;
    m_encodingHandler = newHandler;
  }

  ClearStatistics();

  EncodingLock(*this)->OnStart(*this);
}

/////////////////////////////////////////////////////////////////////////////

RTP_Session::EncodingLock::EncodingLock(RTP_Session & _session)
  : session(_session)
{
  PWaitAndSignal m(session.m_encodingMutex);

  m_encodingHandler = session.m_encodingHandler;
  ++m_encodingHandler->refCount;
}

RTP_Session::EncodingLock::~EncodingLock()
{
  PWaitAndSignal m(session.m_encodingMutex);

  --m_encodingHandler->refCount;
  if (m_encodingHandler->refCount == 0)
    delete m_encodingHandler;
}


/////////////////////////////////////////////////////////////////////////////

RTP_Encoding::RTP_Encoding()
{
  refCount = 1;
}

RTP_Encoding::~RTP_Encoding()
{
  OnFinish();
}


void RTP_Encoding::OnStart(RTP_Session & _rtpSession)
{
  //rtpSession = &_rtpSession;
  rtpUDP = (RTP_UDP *)&_rtpSession;
}

void RTP_Encoding::OnFinish()
{
}

RTP_Session::SendReceiveStatus RTP_Encoding::OnSendData(RTP_DataFrame & frame)
{
  return rtpUDP->Internal_OnSendData(frame);
}

PBoolean RTP_Encoding::WriteData(RTP_DataFrame & frame, bool)
{
  return rtpUDP->Internal_WriteData(frame);
}

RTP_Session::SendReceiveStatus RTP_Encoding::OnSendControl(RTP_ControlFrame & frame, PINDEX & len)
{
  return rtpUDP->Internal_OnSendControl(frame, len);
}

bool RTP_Encoding::WriteDataPDU(RTP_DataFrame & frame)
{
  return rtpUDP->WriteDataOrControlPDU(frame.GetPointer(), frame.GetHeaderSize()+frame.GetPayloadSize(), true);
}

RTP_Session::SendReceiveStatus RTP_Encoding::ReadDataPDU(RTP_DataFrame & frame)
{
  return rtpUDP->Internal_ReadDataPDU(frame);
}

RTP_Session::SendReceiveStatus RTP_Encoding::OnReceiveData(RTP_DataFrame & frame)
{
  return rtpUDP->Internal_OnReceiveData(frame);
}

RTP_Session::SendReceiveStatus RTP_Encoding::OnReadTimeout(RTP_DataFrame & frame)
{
  return rtpUDP->Internal_OnReadTimeout(frame);
}

PBoolean RTP_Encoding::ReadData(RTP_DataFrame & frame, PBoolean loop)
{
  return rtpUDP->Internal_ReadData(frame, loop);
}

int RTP_Encoding::WaitForPDU(PUDPSocket * dataSocket, PUDPSocket * controlSocket, const PTimeInterval & t)
{
  return rtpUDP->Internal_WaitForPDU(dataSocket, controlSocket, t);
}

/////////////////////////////////////////////////////////////////////////////

SecureRTP_UDP::SecureRTP_UDP(const Params & params)
  : RTP_UDP(params)
{
  securityParms = NULL;
}

SecureRTP_UDP::~SecureRTP_UDP()
{
  delete securityParms;
}

void SecureRTP_UDP::SetSecurityMode(OpalSecurityMode * newParms)
{ 
  if (securityParms != NULL)
    delete securityParms;
  securityParms = newParms; 
}
  
OpalSecurityMode * SecureRTP_UDP::GetSecurityParms() const
{ 
  return securityParms; 
}

/////////////////////////////////////////////////////////////////////////////
