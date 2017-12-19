/*
 * rfc2833.cxx
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23863 $
 * $Author: rjongbloed $
 * $Date: 2009-12-10 02:42:20 +0000 (Thu, 10 Dec 2009) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "rfc2833.h"
#endif

#include <codec/rfc2833.h>
#include <opal/rtpconn.h>
#include <rtp/rtp.h>

static const char RFC2833Table1Events[] = "0123456789*#ABCD!                Y   X";
static const char NSEEvents[] = "XY";
static PINDEX NSECodeBase = 192;

#define new PNEW


static PString GetCapability(const std::vector<bool> & capabilitySet)
{
  PStringStream str;

  PINDEX last = capabilitySet.size()-1;
  PINDEX i = 0;
  while (i < last) {
    if (!capabilitySet[i])
      i++;
    else {
      PINDEX start = i++;
      while (capabilitySet[i])
        i++;
      if (!str.IsEmpty())
        str += ",";
      str.sprintf("%u", start);
      if (i > start+1)
        str.sprintf("-%u", i-1);
    }
  }

  return str;
}


static void SetCapability(const PString & codes, std::vector<bool> & xxCapabilitySet, bool merge)
{
  std::vector<bool> capabilitySet;
  capabilitySet.resize(xxCapabilitySet.size());
 
  if (codes.IsEmpty()) {
    // RFC specified default: 0-15
    for (size_t i = 0; i < 15; ++i)
      capabilitySet[i] = true;
  }
  else if (codes != "-") {       // Allow for an empty set
    PStringArray tokens = codes.Tokenise(',');
    for (PINDEX i = 0; i < tokens.GetSize(); ++i) {
      PString token = tokens[i];
      unsigned code = token.AsUnsigned();
      if (code < capabilitySet.size()) {
        PINDEX dash = token.Find('-');
        unsigned end = dash == P_MAX_INDEX ? code : token.Mid(dash+1).AsUnsigned();
        if (end >= capabilitySet.size())
          end = capabilitySet.size()-1;
        while (code <= end)
          capabilitySet[code++] = true;
      }
    }
  }

  if (merge) {
    for (size_t i = 0; i < capabilitySet.size(); ++i)
      xxCapabilitySet[i] = xxCapabilitySet[i] && capabilitySet[i];
  }
  else
    xxCapabilitySet = capabilitySet;
}


///////////////////////////////////////////////////////////////////////////////

OpalRFC2833Info::OpalRFC2833Info(char t, unsigned d, unsigned ts)
{
  tone = t;
  duration = d;
  timestamp = ts;
}


///////////////////////////////////////////////////////////////////////////////

OpalRFC2833Proto::OpalRFC2833Proto(OpalRTPConnection & conn, const PNotifier & rx, const OpalMediaFormat & fmt)
  : m_connection(conn)
  , m_payloadType(RTP_DataFrame::IllegalPayloadType)
  , m_receiveNotifier(rx)
#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif
  , m_receiveHandler(PCREATE_NOTIFIER(ReceivedPacket))
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
  , m_receiveState(ReceiveIdle)
  , m_receivedTone('\0')
  , m_tonesReceived(0)
  , m_transmitState(TransmitIdle)
  , m_rtpSession(NULL)
  , m_transmitTimestamp(0)
  , m_rewriteTransmitTimestamp(false)
  , m_transmitCode('\0')
  , m_transmitDuration(0)
{
  PTRACE(4, "RFC2833\tHandler created");

  m_receiveTimer.SetNotifier(PCREATE_NOTIFIER(ReceiveTimeout));
  m_asyncTransmitTimer.SetNotifier(PCREATE_NOTIFIER(AsyncTimeout));
  m_asyncDurationTimer.SetNotifier(PCREATE_NOTIFIER(AsyncTimeout));

  m_rxCapabilitySet.resize(256);
  SetRxCapability(fmt.GetOptionString("FMTP", "0-15"));
  m_txCapabilitySet = m_rxCapabilitySet;
}

OpalRFC2833Proto::~OpalRFC2833Proto()
{
  if (m_rtpSession != NULL)
    m_connection.ReleaseSession(m_rtpSession->GetSessionID());
}

PBoolean OpalRFC2833Proto::SendToneAsync(char tone, unsigned duration)
{
  PWaitAndSignal mutex(m_mutex);

  // find an audio session in the current connection to send the packet on
  if (m_rtpSession == NULL) {
    OpalMediaStreamPtr stream = m_connection.GetMediaStream(OpalMediaType::Audio(), false);
    if (stream == NULL || (m_rtpSession = m_connection.GetSession(stream->GetSessionID())) == NULL) {
      PTRACE(2, "RFC2833\tNo RTP session suitable for RFC2833");
      return false;
    }
  }

  // if transmittter is ever in this state, then stop the duration timer
  if (m_payloadType == RTP_DataFrame::IllegalPayloadType) {
    PTRACE(2, "RFC2833\tNo payload type, cannot send packet.");
    return false;
  }

  // convert tone to correct code
  PINDEX code = ASCIIToRFC2833(tone, m_txCapabilitySet[NSECodeBase]);

  // if same tone as last time and still transmitting, just extend the time
  if (m_transmitState == TransmitIdle || (code != ' ' && code != m_transmitCode)) {
    if (code == P_MAX_INDEX || !m_txCapabilitySet[code]) {
      m_transmitState = TransmitIdle;
      return false;
    }

    // kick off the transmitter
    m_transmitCode             = (BYTE)code;
    m_transmitState            = TransmitActive;
    m_rewriteTransmitTimestamp = true;
    m_asyncStart               = 0;

    // Starting so cannot have zero duration
    if (duration == 0)
      duration = 90;
  }

  if (duration == 0)
    m_transmitState = TransmitEnding1;
  else {
    // reset the duration and retransmit timers
    m_asyncDurationTimer = duration;
    m_asyncTransmitTimer.RunContinuous(30);
  }

  // send the current frame
  SendAsyncFrame();

  return true;
}


void OpalRFC2833Proto::SendAsyncFrame()
{
  // be thread safe
  PWaitAndSignal mutex(m_mutex);

  if (m_rtpSession == NULL) {
    PTRACE(2, "RFC2833\tCannot send as not RTP session attached");
    m_transmitState = TransmitIdle;
  }

  // if transmittter is ever in this state, then stop the duration timer
  if (m_payloadType == RTP_DataFrame::IllegalPayloadType) {
    PTRACE(2, "RFC2833\tNo payload type for sent packet.");
    m_transmitState = TransmitIdle;
  }

  if (m_transmitState == TransmitIdle) {
    m_asyncDurationTimer.Stop(false);
    return;
  }

  RTP_DataFrame frame(4);
  frame.SetPayloadType(m_payloadType);

  BYTE * payload = frame.GetPayloadPtr();
  payload[0] = m_transmitCode; // tone
  payload[1] = (7 & 0x3f);     // Volume

  // set end bit if sending last three packets
  switch (m_transmitState) {
    case TransmitActive:
      // if the duration has ended, then go into ending state
      if (m_asyncDurationTimer.IsRunning()) {
        // set duration to time since start of time
        if (m_asyncStart != PTimeInterval(0)) 
          m_transmitDuration = (PTimer::Tick() - m_asyncStart).GetInterval() * 8;
        else {
          m_transmitDuration = 0;
          frame.SetMarker(true);
          m_asyncStart = PTimer::Tick();
        }
        break;
      }

      m_transmitState = TransmitEnding1;
      m_asyncTransmitTimer.RunContinuous(5); // Output the three end packets a bit quicker.
      // Do next case

    case TransmitEnding1:
      payload[1] |= 0x80;
      m_transmitDuration = (PTimer::Tick() - m_asyncStart).GetInterval() * 8;
      m_transmitState = TransmitEnding2;
      break;

    case TransmitEnding2:
      payload[1] |= 0x80;
      m_transmitState = TransmitEnding3;
      break;

    case TransmitEnding3:
      payload[1] |= 0x80;
      m_transmitState = TransmitIdle;
      m_asyncTransmitTimer.Stop();
      break;

    default:
      PAssertAlways("RFC2833\tunknown transmit state");
      return;
  }

  // set tone duration
  payload[2] = (BYTE)(m_transmitDuration >> 8);
  payload[3] = (BYTE) m_transmitDuration;

  if (!m_rewriteTransmitTimestamp)
    frame.SetTimestamp(m_transmitTimestamp);

  if (!m_rtpSession->WriteOOBData(frame, m_rewriteTransmitTimestamp)) {
    PTRACE(3, "RFC2833\tTransmission stopped by RTP session");
    // Abort further transmission
    m_transmitState = TransmitIdle;
    m_asyncDurationTimer.Stop(false);
  }

  if (m_rewriteTransmitTimestamp) {
    m_transmitTimestamp        = frame.GetTimestamp();
    m_rewriteTransmitTimestamp = false;
  } 

  PTRACE(frame.GetMarker() ? 3 : 4,
         "RFC2833\tSent " << ((payload[1] & 0x80) ? "end" : "tone") << ": code=" << (unsigned)m_transmitCode <<
         ", dur=" << m_transmitDuration << ", ts=" << frame.GetTimestamp() << ", mkr=" << frame.GetMarker());
}


PString OpalRFC2833Proto::GetTxCapability() const
{
  return GetCapability(m_txCapabilitySet);
}


PString OpalRFC2833Proto::GetRxCapability() const
{
  return GetCapability(m_rxCapabilitySet);
}


void OpalRFC2833Proto::SetTxCapability(const PString & codes, bool merge)
{
  PTRACE(4, "RFC2833\tTx capability " << (merge ? "merged with" : "set to") << " \"" << codes << '"');
  SetCapability(codes, m_txCapabilitySet, merge);
}


void OpalRFC2833Proto::SetRxCapability(const PString & codes)
{
  PTRACE(4, "RFC2833\tRx capability set to \"" << codes << '"');
  SetCapability(codes, m_rxCapabilitySet, false);
}


PINDEX OpalRFC2833Proto::ASCIIToRFC2833(char tone, bool hasNSE)
{
  const char * theChar;
  int upperTone = toupper(tone);

  if (hasNSE && (theChar = strchr(NSEEvents, upperTone)) != NULL)
    return (PINDEX)(NSECodeBase+theChar-NSEEvents);

  if ((theChar = strchr(RFC2833Table1Events, upperTone)) != NULL) 
    return (PINDEX)(theChar-RFC2833Table1Events);

  PTRACE(1, "RFC2833\tInvalid tone character '" << tone << "'.");
  return P_MAX_INDEX;
}


char OpalRFC2833Proto::RFC2833ToASCII(PINDEX rfc2833, bool hasNSE)
{
  PASSERTINDEX(rfc2833);

  if (hasNSE && rfc2833 >= NSECodeBase && rfc2833 < NSECodeBase+(PINDEX)sizeof(NSEEvents)-1)
    return NSEEvents[rfc2833-NSECodeBase];

  if (rfc2833 >= 0 && rfc2833 < (PINDEX)sizeof(RFC2833Table1Events)-1)
    return RFC2833Table1Events[rfc2833];

  return '\0';
}


void OpalRFC2833Proto::AsyncTimeout(PTimer &, INT)
{
  SendAsyncFrame();
}


void OpalRFC2833Proto::OnStartReceive(char tone, unsigned timestamp)
{
  ++m_tonesReceived;
  m_previousReceivedTimestamp = timestamp;
  OnStartReceive(tone);
  OpalRFC2833Info info(tone, 0, timestamp);
  m_receiveNotifier(info, 0);
}


void OpalRFC2833Proto::OnStartReceive(char)
{
}


void OpalRFC2833Proto::OnEndReceive(char tone, unsigned duration, unsigned timestamp)
{
  m_receiveState = ReceiveIdle;
  m_receiveTimer.Stop(false);
  OpalRFC2833Info info(tone, duration, timestamp);
  m_receiveNotifier(info, 1);
}


void OpalRFC2833Proto::ReceivedPacket(RTP_DataFrame & frame, INT)
{
  if (frame.GetPayloadType() != m_payloadType || frame.GetPayloadSize() == 0)
    return;

  PWaitAndSignal mutex(m_mutex);

  if (frame.GetPayloadSize() < 4) {
    PTRACE(2, "RFC2833\tIgnoring packet size " << frame.GetPayloadSize() << " - too small.");
    return;
  }

  const BYTE * payload = frame.GetPayloadPtr();

  char tone = RFC2833ToASCII(payload[0], m_rxCapabilitySet[NSECodeBase]);
  if (tone == '\0') {
    PTRACE(2, "RFC2833\tIgnoring packet with code " << payload[0] << " - unsupported event.");
    return;
  }
  unsigned duration  = ((payload[2] <<8) + payload[3]) / 8;
  unsigned timeStamp = frame.GetTimestamp();
  unsigned volume    = (payload[1] & 0x3f);

  // RFC 2833 says to ignore below -55db
  if (volume > 55) {
    PTRACE(2, "RFC2833\tIgnoring packet " << (unsigned)payload[0] << " with volume -" << volume << "db");
    return;
  }

  PTRACE(4, "RFC2833\tReceived " << ((payload[1] & 0x80) ? "end" : "tone") << ": code='" << (unsigned)payload[0]
         << "', dur=" << duration << ", vol=" << volume << ", ts=" << timeStamp << ", mkr=" << frame.GetMarker());

  // the only safe way to detect a new tone is the timestamp
  // because the packet with the marker bit could go missing and 
  // because some endpoints (*cough* Kapanga *cough*) send multiple marker bits
  bool newTone = (m_tonesReceived == 0) || (timeStamp != m_previousReceivedTimestamp);

  // if new tone, end any current tone and start new one
  if (!newTone) {
    if (m_receiveState == ReceiveActive)
      m_receiveTimer = 200;
    else
      m_receiveTimer.Stop();
  }
  else {
    m_receiveTimer.Stop();

    // finish any existing tone
    if (m_receiveState == ReceiveActive) 
      OnEndReceive(m_receivedTone, duration, m_previousReceivedTimestamp);

    // do callback for new tone
    OnStartReceive(tone, timeStamp);

    // setup for new tone
    m_receivedTone = tone;
    m_receiveTimer = 200;
    m_receiveState = ReceiveActive;
  }

  // if end of active tone, do callback and change to idle 
  // no else, so this works for single packet tones too
  if ((m_receiveState == ReceiveActive) && ((payload[1]&0x80) != 0)) 
    OnEndReceive(m_receivedTone, duration, timeStamp);
}


void OpalRFC2833Proto::ReceiveTimeout(PTimer &, INT)
{
  PTRACE(3, "RFC2833\tTimeout occurred while receiving " << (unsigned)m_receivedTone);

  PWaitAndSignal mutex(m_mutex);

  if (m_receiveState != ReceiveIdle) {
    m_receiveState = ReceiveIdle;
    //OnEndReceive(receivedTone, 0, 0);
  }

  m_receiveTimer.Stop(false);
}


/////////////////////////////////////////////////////////////////////////////
