/*
 * mediastrm.cxx
 *
 * Media Stream classes
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
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
 * Contributor(s): ________________________________________.
 *
 * $Revision: 23955 $
 * $Author: rjongbloed $
 * $Date: 2010-01-21 02:40:15 +0000 (Thu, 21 Jan 2010) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "mediastrm.h"
#endif

#include <opal/buildopts.h>

#include <opal/mediastrm.h>

#if OPAL_VIDEO
#include <ptlib/videoio.h>
#include <codec/vidcodec.h>
#endif

#include <ptlib/sound.h>
#include <opal/patch.h>
#include <lids/lid.h>
#include <rtp/rtp.h>
#include <opal/transports.h>
#include <opal/rtpconn.h>
#include <opal/rtpep.h>
#include <opal/call.h>

#define MAX_PAYLOAD_TYPE_MISMATCHES 10


#define new PNEW


///////////////////////////////////////////////////////////////////////////////
bool OpalMediaStream::RecordRTPFrame(RTP_DataFrame& frame )
{
  if (mediaFormat.GetMediaType()== OpalMediaType::Audio()){
    connection.GetCall().OnRecordAudio("", frame);
  }else if(mediaFormat.GetMediaType()== OpalMediaType::Video()){
    connection.GetCall().OnRecordVideo("", frame);
  }
  return true;
}

OpalMediaStream::OpalMediaStream(OpalConnection & conn, const OpalMediaFormat & fmt, unsigned _sessionID, PBoolean isSourceStream)
  : connection(conn)
  , sessionID(_sessionID)
  , identifier(conn.GetCall().GetToken() + psprintf("_%u", sessionID))
  , mediaFormat(fmt)
  , paused(false)
  , isSource(isSourceStream)
  , isOpen(false)
  , defaultDataSize(mediaFormat.GetFrameSize()*mediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1))
  , timestamp(0)
  , marker(true)
  , mismatchedPayloadTypes(0)
  , mediaPatch(NULL)
  , m_uid(0)
  , m_uidbyRecord(0)
{
  connection.SafeReference();
  PTRACE(5, "Media\tCreated " << (isSource ? "Source" : "Sink") << ' ' << this);
}


OpalMediaStream::~OpalMediaStream()
{
  Close();
  connection.SafeDereference();
  PTRACE(5, "Media\tDestroyed " << (isSource ? "Source" : "Sink") << ' ' << this);
}


void OpalMediaStream::PrintOn(ostream & strm) const
{
  strm << GetClass() << '-';
  if (isSource)
    strm << "Source";
  else
    strm << "Sink";
  strm << '-' << mediaFormat;
}


OpalMediaFormat OpalMediaStream::GetMediaFormat() const
{
  return mediaFormat;
}


PBoolean OpalMediaStream::UpdateMediaFormat(const OpalMediaFormat & newMediaFormat, bool fromPatch)
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return false;

  // If we are source, then update the sink side, and vice versa
  if (mediaPatch != NULL && !fromPatch)
    return mediaPatch->UpdateMediaFormat(newMediaFormat);

  if (mediaFormat != newMediaFormat)
    return mediaFormat.Merge(newMediaFormat);

  mediaFormat = newMediaFormat;

  PTRACE(4, "Media\tMedia format updated on " << *this);

  return true;
}


PBoolean OpalMediaStream::ExecuteCommand(const OpalMediaCommand & command)
{
  PSafeLockReadOnly safeLock(*this);
  if (!safeLock.IsLocked())
    return false;

  if (mediaPatch == NULL)
    return false;

  return mediaPatch->ExecuteCommand(command, IsSink());
}


void OpalMediaStream::SetCommandNotifier(const PNotifier & notifier)
{
  if (!LockReadWrite())
    return;

  if (mediaPatch != NULL)
    mediaPatch->SetCommandNotifier(notifier, IsSink());

  commandNotifier = notifier;

  UnlockReadWrite();
}


PBoolean OpalMediaStream::Open()
{
  isOpen = true;
  return true;
}


PBoolean OpalMediaStream::Start()
{
  if (!Open())
    return false;

  if (!LockReadOnly())
    return false;

  if (mediaPatch != NULL)
    mediaPatch->Start();

  UnlockReadOnly();

  return true;
}


PBoolean OpalMediaStream::Close()
{
  if (!isOpen)
    return false;

  PTRACE(4, "Media\tClosing stream " << *this);

  if (!LockReadWrite())
    return false;

  // Allow for race condition where it is closed in another thread during the above wait
  if (!isOpen) {
    UnlockReadWrite();
    return false;
  }

  isOpen = false;

  if (mediaPatch == NULL)
    UnlockReadWrite();
  else {
#if 0
    PTRACE(4, "Media\tDisconnecting " << *this << " from patch thread " << *mediaPatch);
#else
    PTRACE(4, "Media\tDisconnecting  from patch thread ");
#endif
    OpalMediaPatch * patch = mediaPatch;
    mediaPatch = NULL;

    if (IsSink())
      patch->RemoveSink(this);
	
    UnlockReadWrite();

    if (IsSource()) {
      patch->Close();
      connection.GetEndPoint().GetManager().DestroyMediaPatch(patch);
    }
  }

  if (connection.CloseMediaStream(*this))
    return true;

  connection.OnClosedMediaStream(*this);
  connection.RemoveMediaStream(*this);
  return true;
}


PBoolean OpalMediaStream::WritePackets(RTP_DataFrameList & packets)
{
  for (RTP_DataFrameList::iterator packet = packets.begin(); packet != packets.end(); ++packet) {
    if (!WritePacket(*packet))
      return false;
  }

  return true;
}


inline static unsigned CalculateTimestamp(PINDEX size, const OpalMediaFormat & mediaFormat)
{
#if 0//chenyuan
  unsigned frameTime = mediaFormat.GetFrameTime();
  PINDEX frameSize = mediaFormat.GetFrameSize();

  if (frameSize == 0)
    return frameTime;

  unsigned frames = (size + frameSize - 1) / frameSize;
  return frames*frameTime;
#else
  time_t t;time(&t);
  return t;
  //PTime now;
 // return now.GetTimestamp();
#endif
}


PBoolean OpalMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (!isOpen)
    return false;

  unsigned oldTimestamp = timestamp;
#if 0
  if (defaultDataSize >/*< chenyuan*/ (packet.GetSize() - RTP_DataFrame::MinHeaderSize)) {
    stringstream str;
    str << "Media stream buffer " << (packet.GetSize() - RTP_DataFrame::MinHeaderSize) << " too small for media packet " << defaultDataSize;
    PAssertAlways(str.str().c_str());
  }
#else //orangal
  if (this->GetSessionID()!= 0 && defaultDataSize > (packet.GetSize() - RTP_DataFrame::MinHeaderSize)) {
    stringstream str;
    str << "Media stream buffer " << (packet.GetSize() - RTP_DataFrame::MinHeaderSize) << " too small for media packet " << defaultDataSize;
    PAssertAlways(str.str().c_str());
  }

#endif
  PINDEX lastReadCount;
#if 0
if (!ReadData(packet.GetPayloadPtr(), packet.GetPayloadSize()/* chenyuanpacket.GetSize()*/, lastReadCount))
    return false;
#else
  if (!ReadData(packet.GetPayloadPtr(), defaultDataSize, lastReadCount))//oringnal
  {
    PTRACE(5, "OpalMediaStream\t Failed to read data defaultDatasize is " << defaultDataSize << ", packet.size is " << packet.GetSize()); 
    return false;
  }
#endif
  // If the ReadData() function did not change the timestamp then use the default
  // method or fixed frame times and sizes.
  if (oldTimestamp == timestamp)
    timestamp += CalculateTimestamp(lastReadCount, mediaFormat);

  packet.SetPayloadType(mediaFormat.GetPayloadType());
  packet.SetPayloadSize(lastReadCount);
  packet.SetTimestamp(oldTimestamp); // Beginning of frame
  packet.SetMarker(marker);
  marker = false;

  if (paused)
    packet.SetPayloadSize(0);
  
  return true;
}


PBoolean OpalMediaStream::WritePacket(RTP_DataFrame & packet)
{
  if (!isOpen)
    return false;

  if (paused)
    packet.SetPayloadSize(0);

  timestamp = packet.GetTimestamp();

  int size = packet.GetPayloadSize();
  if (size > 0 && mediaFormat.IsTransportable()) {
    if (packet.GetPayloadType() == mediaFormat.GetPayloadType()) {
      PTRACE_IF(2, mismatchedPayloadTypes > 0,
                "H323RTP\tPayload type matched again " << mediaFormat.GetPayloadType());
      mismatchedPayloadTypes = 0;
    }
    else {
      mismatchedPayloadTypes++;
      if (mismatchedPayloadTypes < MAX_PAYLOAD_TYPE_MISMATCHES) {
        PTRACE(2, "Media\tRTP data with mismatched payload type,"
                  " is " << packet.GetPayloadType() << 
                  " expected " << mediaFormat.GetPayloadType() <<
                  ", ignoring packet.");
        size = 0;
      }
      else {
        PTRACE_IF(2, mismatchedPayloadTypes == MAX_PAYLOAD_TYPE_MISMATCHES,
                  "Media\tRTP data with consecutive mismatched payload types,"
                  " is " << packet.GetPayloadType() << 
                  " expected " << mediaFormat.GetPayloadType() <<
                  ", ignoring payload type from now on.");
      }
    }
  }

  if (size == 0) {
    timestamp += CalculateTimestamp(1, mediaFormat);
    packet.SetTimestamp(timestamp);
    PINDEX dummy;
    return WriteData(NULL, 0, dummy);
  }

  marker = packet.GetMarker();
  const BYTE * ptr = packet.GetPayloadPtr();

  while (size > 0) {
    unsigned oldTimestamp = timestamp;

    PINDEX written;
    if (!WriteData(ptr, size, written) || (written == 0)) {
      PTRACE(2, "Media\tWritePacket failed with written " << written);
      return false;
    }

    // If the Write() function did not change the timestamp then use the default
    // method of fixed frame times and sizes.
    if (oldTimestamp == timestamp)
      timestamp += CalculateTimestamp(size, mediaFormat);

    size -= written;
    ptr += written;
  }

  PTRACE_IF(1, size < 0, "Media\tRTP payload size too small, short " << -size << " bytes.");

  packet.SetTimestamp(timestamp);

  return true;
}


PBoolean OpalMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  if (!isOpen) {
    length = 0;
    return false;
  }

  RTP_DataFrame packet(size);
  if (!ReadPacket(packet)) {
    length = 0;
    return false;
  }

  length = packet.GetPayloadSize();
  if (length > size)
    length = size;
  memcpy(buffer, packet.GetPayloadPtr(), length);
  timestamp = packet.GetTimestamp();
  marker = packet.GetMarker();
  return true;
}


PBoolean OpalMediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  if (!isOpen) {
    written = 0;
    return false;
  }

  written = length;
  RTP_DataFrame packet(length);
  memcpy(packet.GetPayloadPtr(), buffer, length);
  packet.SetPayloadType(mediaFormat.GetPayloadType());
  packet.SetTimestamp(timestamp);
  packet.SetMarker(marker);
  return WritePacket(packet);
}


PBoolean OpalMediaStream::PushPacket(RTP_DataFrame & packet)
{
  OpalMediaPatch * patch = NULL;
  if (LockReadOnly()) {
    patch = mediaPatch;
    UnlockReadOnly();
  }

  // OpalMediaPatch::PushFrame() might block, do outside of mutex
  return patch != NULL && patch->PushFrame(packet);
}


PBoolean OpalMediaStream::SetDataSize(PINDEX dataSize, PINDEX /*frameTime*/)
{
  if (dataSize <= 0)
    return false;

  if (defaultDataSize< dataSize) defaultDataSize = dataSize;
	else return true;
  PTRACE_IF(4, defaultDataSize != dataSize, "Media\tSet data size from " << defaultDataSize << " to " << dataSize);
  return true;
}


PBoolean OpalMediaStream::RequiresPatchThread(OpalMediaStream * /*sinkStream*/) const
{
  return RequiresPatchThread();
}


PBoolean OpalMediaStream::RequiresPatchThread() const
{
  return true;
}


void OpalMediaStream::EnableJitterBuffer() const
{
}


void OpalMediaStream::SetPaused(bool p)
{
  PTRACE_IF(3, paused != p, "Media\t" << (p ? "Paused" : "Resumed") << " stream " << *this);
  paused = p;
}


PBoolean OpalMediaStream::SetPatch(OpalMediaPatch * patch)
{
#if PTRACING
  if (PTrace::CanTrace(4) && (patch != NULL || mediaPatch != NULL)) {
    ostream & trace = PTrace::Begin(4, __FILE__, __LINE__);
    if (patch == NULL)
      trace << "Removing patch " << *mediaPatch;
    else if (mediaPatch == NULL)
      trace << "Adding patch " << *patch;
    else
      trace << "Overwriting patch " << *mediaPatch << " with " << *patch;
    trace << " on stream " << *this << PTrace::End;
  }
#endif

  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return false;

  if (mediaPatch == NULL) {
    mediaPatch = patch;
    return true;
  }

  OpalMediaPatch * oldPatch = mediaPatch;
  mediaPatch = patch;

  if (IsSink())
    oldPatch->RemoveSink(this);
  else {
    oldPatch->Close();
    connection.GetEndPoint().GetManager().DestroyMediaPatch(oldPatch);
  }

  return true;
}


void OpalMediaStream::AddFilter(const PNotifier & Filter, const OpalMediaFormat & Stage)
{
  PSafeLockReadWrite safeLock(*this);
  if (safeLock.IsLocked() && mediaPatch != NULL)
    mediaPatch->AddFilter(Filter, Stage);
}


PBoolean OpalMediaStream::RemoveFilter(const PNotifier & Filter, const OpalMediaFormat & Stage)
{
  PSafeLockReadWrite safeLock(*this);
  return safeLock.IsLocked() && mediaPatch != NULL && mediaPatch->RemoveFilter(Filter, Stage);
}


#if OPAL_STATISTICS
void OpalMediaStream::GetStatistics(OpalMediaStatistics & statistics, bool fromPatch) const
{
  PSafeLockReadOnly safeLock(*this);
  if (safeLock.IsLocked() && mediaPatch != NULL && !fromPatch)
    mediaPatch->GetStatistics(statistics, IsSink());
}
#endif


void OpalMediaStream::RemovePatch(OpalMediaPatch * /*patch*/ ) 
{ 
  SetPatch(NULL); 
}


void OpalMediaStream::OnStartMediaPatch() 
{ 
  connection.OnStartMediaPatch(*mediaPatch);
}

void OpalMediaStream::OnStopMediaPatch(OpalMediaPatch & patch)
{ 
  connection.OnStopMediaPatch(patch);
}


///////////////////////////////////////////////////////////////////////////////

OpalMediaStreamPacing::OpalMediaStreamPacing(const OpalMediaFormat & mediaFormat)
  : m_isAudio(mediaFormat.GetMediaType() == OpalMediaType::Audio())
  , m_frameTime(mediaFormat.GetFrameTime())
  , m_frameSize(mediaFormat.GetFrameSize())
  , m_timeUnits(mediaFormat.GetTimeUnits())
{
  PAssert(!(m_isAudio && m_frameSize == 0), PInvalidParameter);
}


void OpalMediaStreamPacing::Pace(bool reading, PINDEX bytes, bool & marker)
{
  unsigned timeToWait = m_frameTime;

  if (m_isAudio)
    timeToWait *= (bytes + m_frameSize - 1) / m_frameSize;
  else {
    if (reading)
      marker = true;
    else if (!marker)
      return;
  }

  m_delay.Delay(timeToWait/m_timeUnits);
}


///////////////////////////////////////////////////////////////////////////////

OpalNullMediaStream::OpalNullMediaStream(OpalConnection & conn,
                                         const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         bool isSource,
                                         bool isSyncronous)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
  , OpalMediaStreamPacing(mediaFormat)
  , m_isSynchronous(isSyncronous)
  , m_requiresPatchThread(isSyncronous)
{
}


OpalNullMediaStream::OpalNullMediaStream(OpalConnection & conn,
                                         const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         bool isSource,
                                         bool usePacingDelay,
                                         bool requiresPatchThread)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
  , OpalMediaStreamPacing(mediaFormat)
  , m_isSynchronous(usePacingDelay)
  , m_requiresPatchThread(requiresPatchThread)
{
}


PBoolean OpalNullMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  if (!isOpen)
    return false;

  memset(buffer, 0, size);
  length = size;

  if (m_isSynchronous)
    Pace(true, size, marker);
  return true;
}


PBoolean OpalNullMediaStream::WriteData(const BYTE * /*buffer*/, PINDEX length, PINDEX & written)
{
  if (!isOpen)
    return false;

  written = length != 0 ? length : defaultDataSize;

  if (m_isSynchronous)
    Pace(false, written, marker);
  return true;
}


PBoolean OpalNullMediaStream::RequiresPatchThread() const
{
  return m_requiresPatchThread;
}


PBoolean OpalNullMediaStream::IsSynchronous() const
{
  return m_isSynchronous;
}


///////////////////////////////////////////////////////////////////////////////

OpalRTPMediaStream::OpalRTPMediaStream(OpalRTPConnection & conn,
                                   const OpalMediaFormat & mediaFormat,
                                                  PBoolean isSource,
                                             RTP_Session & rtp,
                                                  unsigned minJitter,
                                                  unsigned maxJitter)
  : OpalMediaStream(conn, mediaFormat, rtp.GetSessionID(), isSource),
    rtpSession(rtp),
    minAudioJitterDelay(minJitter),
    maxAudioJitterDelay(maxJitter)
{
  if (!mediaFormat.NeedsJitterBuffer())
    minAudioJitterDelay = maxAudioJitterDelay = 0;

  /* If we are a source then we should set our buffer size to the max
     practical UDP packet size. This means we have a buffer that can accept
     whatever the RTP sender throws at us. For sink, we set it to the
     maximum size based on MTU (or other criteria). */
  defaultDataSize = isSource ? conn.GetEndPoint().GetManager().GetMaxRtpPacketSize() : conn.GetMaxRtpPayloadSize();
}


OpalRTPMediaStream::~OpalRTPMediaStream()
{
  Close();
}


PBoolean OpalRTPMediaStream::Open()
{
  if (isOpen)
    return true;

  rtpSession.SetEncoding(mediaFormat.GetMediaType().GetDefinition()->GetRTPEncoding());
  rtpSession.Reopen(IsSource());

  return OpalMediaStream::Open();
}

bool OpalRTPMediaStream::UpdateMediaFormat(
      const OpalMediaFormat & mediaFormat,  ///<  New media format
      bool fromPatch                 ///<  Is being called from OpalMediaPatch
    )
{
  bool bRet = OpalMediaStream::UpdateMediaFormat(mediaFormat,fromPatch );
   
 // int nbitRate= 	mediaFormat.GetOptionInteger(OpalVideoFormat::TargetBitRateOption(), 0);
 // if (nbitRate>0)
 //   rtpSession.yUDPSetRTPbitRate(nbitRate);
  return bRet;
}

PBoolean OpalRTPMediaStream::Close()
{
  if (!isOpen)
    return false;
    
  PTRACE(3, "Media\tClosing RTP for " << *this);

  // Break any I/O blocks and wait for the thread that uses this object to
  // terminate before we allow it to be deleted.
  rtpSession.Close(IsSource());
  return OpalMediaStream::Close();
}


void OpalRTPMediaStream::SetPaused(bool pause)
{
  if (paused == pause)
    return;

  OpalMediaStream::SetPaused(pause);

  // If coming out of pause, reopen the RTP session, even though it is probably
  // still open, to make sure any pending error/statistic conditions are reset.
  if (!paused)
    rtpSession.Reopen(IsSource());

  if (IsSource()) {
    if (pause)
      rtpSession.SetJitterBufferSize(0, 0);
    else
      EnableJitterBuffer();
  }
}


PBoolean OpalRTPMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return false;
  }

  if (paused)
    packet.SetPayloadSize(0);
  else {
    if (!rtpSession.ReadBufferedData(packet))
      return false;
  }

  timestamp = packet.GetTimestamp();
  return true;
}

PBoolean OpalRTPMediaStream::WritePacket(RTP_DataFrame & packet)
{
  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return false;
  }

  timestamp = packet.GetTimestamp();

  if (paused || packet.GetPayloadSize() == 0)
    return true;

  if ( packet.GetPayloadType() == RTP_DataFrame::MaxPayloadType)
	  packet.SetPayloadType(mediaFormat.GetPayloadType());

  return rtpSession.WriteData(packet);
}


PBoolean OpalRTPMediaStream::SetDataSize(PINDEX PTRACE_PARAM(dataSize), PINDEX /*frameTime*/)
{
  PTRACE(3, "Media\tRTP data size cannot be changed to " << dataSize << ", fixed at " << defaultDataSize);
  return true;
}


PBoolean OpalRTPMediaStream::IsSynchronous() const
{
  return false;
}


PBoolean OpalRTPMediaStream::RequiresPatchThread() const
{
  return !dynamic_cast<OpalRTPEndPoint &>(connection.GetEndPoint()).CheckForLocalRTP(*this);
}


void OpalRTPMediaStream::EnableJitterBuffer() const
{
  unsigned minJitter, maxJitter;
  if (mediaFormat.NeedsJitterBuffer()) {
    minJitter = minAudioJitterDelay*mediaFormat.GetTimeUnits();
    maxJitter = maxAudioJitterDelay*mediaFormat.GetTimeUnits();
  }
  else
    minJitter = maxJitter = 0;

  rtpSession.SetJitterBufferSize(minJitter, maxJitter,
                                 mediaFormat.GetTimeUnits(),
                                 connection.GetEndPoint().GetManager().GetMaxRtpPacketSize());
}


PBoolean OpalRTPMediaStream::SetPatch(OpalMediaPatch * patch)
{
  if (!isOpen || IsSink())
    return OpalMediaStream::SetPatch(patch);

  rtpSession.Close(true);
  bool ok = OpalMediaStream::SetPatch(patch);
  rtpSession.Reopen(true);
  return ok;
}


#if OPAL_STATISTICS
void OpalRTPMediaStream::GetStatistics(OpalMediaStatistics & statistics, bool fromPatch) const
{
  rtpSession.GetStatistics(statistics, IsSource());
  OpalMediaStream::GetStatistics(statistics, fromPatch);
}
#endif

///////////////////////////////////////////////////////////////////////////////

OpalRawMediaStream::OpalRawMediaStream(OpalConnection & conn,
                                       const OpalMediaFormat & mediaFormat,
                                       unsigned sessionID,
                                       PBoolean isSource,
                                       PChannel * chan, PBoolean autoDelete)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
  , m_channel(chan)
  , m_autoDelete(autoDelete)
  , m_silence(160) // At least 10ms
  , m_averageSignalSum(0)
  , m_averageSignalSamples(0)
{
}


OpalRawMediaStream::~OpalRawMediaStream()
{
  Close();

  if (m_autoDelete)
    delete m_channel;
  m_channel = NULL;
}


PBoolean OpalRawMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  if (!isOpen)
  {
    PTRACE(1, "OpalRawMediaStream\t Media is not open.");
    return false;
  }
  length = 0;

  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return false;
  }

  PWaitAndSignal mutex(m_channelMutex);

  if (!IsOpen() || m_channel == NULL)
  {
    PTRACE(1, "OpalRawMediaStream\t m_channel is NULL.");

    return false;
  }
  if (buffer == NULL || size == 0)
    return m_channel->Read(buffer, size);

  while (size > 0) {
    if (!m_channel->Read(buffer, size))
      return false;

    PINDEX lastReadCount = m_channel->GetLastReadCount();
    CollectAverage(buffer, lastReadCount);

    buffer += lastReadCount;
    length += lastReadCount;
    size -= lastReadCount;
  }


  return true;
}


PBoolean OpalRawMediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  if (!isOpen) {
    PTRACE(1, "Media\tTried to write to closed media stream");
    return false;
  }

  written = 0;

  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return false;
  }
  
  PWaitAndSignal mutex(m_channelMutex);

  if (!IsOpen() || m_channel == NULL) {
    PTRACE(1, "Media\tTried to write to media stream with no channel");
    return false;
  }

  /* We make the assumption that the remote is sending the same sized packets
     all the time and does not suddenly switch from 30ms to 20ms, even though
     this is technically legal. We have never yet seen it, and even if it did
     it doesn't hurt the silence insertion algorithm very much.

     So, silence buffer is set to be the largest chunk of audio the remote has
     ever sent to us. Then when they stop sending, (we get length==0) we just
     keep outputting that number of bytes to the raw channel until the remote
     starts up again.
     */

  if (buffer != NULL && length != 0)
    m_silence.SetMinSize(length);
  else {
    length = m_silence.GetSize();
    buffer = m_silence;
  }

  if (!m_channel->Write(buffer, length))
    return false;

  written = m_channel->GetLastWriteCount();
  CollectAverage(buffer, written);
  return true;
}


bool OpalRawMediaStream::SetChannel(PChannel * chan, bool autoDelete)
{
  if (chan == NULL || !chan->IsOpen()) {
    if (autoDelete)
      delete chan;
    return false;
  }

  m_channelMutex.Wait();

  PChannel * channelToDelete = m_autoDelete ? m_channel : NULL;
  m_channel = chan;
  m_autoDelete = autoDelete;

  SetDataSize(GetDataSize(), 1);

  m_channelMutex.Signal();

  delete channelToDelete; // Outside mutex

  PTRACE(4, "Media\tSet raw media channel to \"" << m_channel->GetName() << '"');
  return true;
}


PBoolean OpalRawMediaStream::Close()
{
  if (!isOpen)
    return false;

  if (m_channel != NULL)
    m_channel->Close();

  return OpalMediaStream::Close();
}


unsigned OpalRawMediaStream::GetAverageSignalLevel()
{
  PWaitAndSignal mutex(m_averagingMutex);

  if (m_averageSignalSamples == 0)
    return UINT_MAX;

  unsigned average = (unsigned)(m_averageSignalSum/m_averageSignalSamples);
  m_averageSignalSum = average;
  m_averageSignalSamples = 1;
  return average;
}


void OpalRawMediaStream::CollectAverage(const BYTE * buffer, PINDEX size)
{
  PWaitAndSignal mutex(m_averagingMutex);

  size = size/2;
  m_averageSignalSamples += size;
  const short * pcm = (const short *)buffer;
  while (size-- > 0) {
    m_averageSignalSum += PABS(*pcm);
    pcm++;
  }
}


///////////////////////////////////////////////////////////////////////////////

OpalFileMediaStream::OpalFileMediaStream(OpalConnection & conn,
                                         const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         PBoolean isSource,
                                         PFile * file,
                                         PBoolean autoDel)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource, file, autoDel)
  , OpalMediaStreamPacing(mediaFormat)
{
}


OpalFileMediaStream::OpalFileMediaStream(OpalConnection & conn,
                                         const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         PBoolean isSource,
                                         const PFilePath & path)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource,
                       new PFile(path, isSource ? PFile::ReadOnly : PFile::WriteOnly),
                       true)
  , OpalMediaStreamPacing(mediaFormat)
{
}


PBoolean OpalFileMediaStream::IsSynchronous() const
{
  return false;
}


PBoolean OpalFileMediaStream::ReadData(BYTE * data, PINDEX size, PINDEX & length)
{
  if (!OpalRawMediaStream::ReadData(data, size, length))
    return false;

  Pace(true, size, marker);
  return true;
}


/**Write raw media data to the sink media stream.
   The default behaviour writes to the PChannel object.
  */
PBoolean OpalFileMediaStream::WriteData(const BYTE * data, PINDEX length, PINDEX & written)
{
  if (!OpalRawMediaStream::WriteData(data, length, written))
    return false;

  Pace(false, written, marker);
  return true;
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_PTLIB_AUDIO

OpalAudioMediaStream::OpalAudioMediaStream(OpalConnection & conn,
                                           const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           PBoolean isSource,
                                           PINDEX buffers,
                                           unsigned bufferTime,
                                           PSoundChannel * channel,
                                           PBoolean autoDel)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource, channel, autoDel)
  , m_soundChannelBuffers(buffers)
  , m_soundChannelBufferTime(bufferTime)
{
}


OpalAudioMediaStream::OpalAudioMediaStream(OpalConnection & conn,
                                           const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           PBoolean isSource,
                                           PINDEX buffers,
                                           unsigned bufferTime,
                                           const PString & deviceName)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource,
                       PSoundChannel::CreateOpenedChannel(PString::Empty(),
                                                          deviceName,
                                                          isSource ? PSoundChannel::Recorder
                                                                   : PSoundChannel::Player,
                                                          1, mediaFormat.GetClockRate(), 16),
                       true)
  , m_soundChannelBuffers(buffers)
  , m_soundChannelBufferTime(bufferTime)
{
}


PBoolean OpalAudioMediaStream::SetDataSize(PINDEX dataSize, PINDEX frameTime)
{
  /* For efficiency reasons we will not accept a packet size that is too small.
     We move it up to the next even multiple, which has a danger of the remote not
     sending an even number of our multiplier. */
  const unsigned BufferTimeMilliseconds = 10;
  PINDEX bufferSize = BufferTimeMilliseconds*mediaFormat.GetClockRate()/1000*sizeof(short);

  // Calculate sound buffers from global system settings
  PINDEX soundChannelBuffers = (m_soundChannelBufferTime+BufferTimeMilliseconds-1)/BufferTimeMilliseconds;
  if (soundChannelBuffers < m_soundChannelBuffers)
    soundChannelBuffers = m_soundChannelBuffers;

  // Quantise dataSize up to multiple of the frame time
  PINDEX frameSize = frameTime*sizeof(short);
  dataSize = (dataSize+frameSize-1)/frameSize * frameSize;

  // Calculte buffers needed for the data we require and increase buffers if required
  PINDEX dataBuffersNeeded = (dataSize+bufferSize-1)/bufferSize;
  if (soundChannelBuffers < dataBuffersNeeded)
    soundChannelBuffers = dataBuffersNeeded;

  PTRACE(3, "Media\tAudio " << (IsSource() ? "source" : "sink") << " data size set to "
         << dataSize << ", buffer size set to " << bufferSize << " and " << soundChannelBuffers << " buffers.");
  return OpalMediaStream::SetDataSize(dataSize, frameTime) &&
         ((PSoundChannel *)m_channel)->SetBuffers(bufferSize, soundChannelBuffers);
}


PBoolean OpalAudioMediaStream::IsSynchronous() const
{
  return true;
}

#endif // OPAL_PTLIB_AUDIO


///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

OpalVideoMediaStream::OpalVideoMediaStream(OpalConnection & conn,
                                          const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           PVideoInputDevice * in,
                                           PVideoOutputDevice * out,
                                           PBoolean del)
  : OpalMediaStream(conn, mediaFormat, sessionID, in != NULL),
    inputDevice(in),
    outputDevice(out),
    autoDelete(del)
{
  PAssert(in != NULL || out != NULL, PInvalidParameter);
}


OpalVideoMediaStream::~OpalVideoMediaStream()
{
  Close();
  if (autoDelete) {
    delete inputDevice;
    inputDevice=0;
    delete outputDevice;
    outputDevice=0;
  }
}


PBoolean OpalVideoMediaStream::SetDataSize(PINDEX dataSize, PINDEX frameTime)
{
  if (inputDevice != NULL) {
    PINDEX minDataSize = inputDevice->GetMaxFrameBytes();
    if (dataSize < minDataSize)
      dataSize = minDataSize;
  }
  if (outputDevice != NULL) {
    PINDEX minDataSize = outputDevice->GetMaxFrameBytes();
    if (dataSize < minDataSize)
      dataSize = minDataSize;
  }

  return OpalMediaStream::SetDataSize(sizeof(PluginCodec_Video_FrameHeader) + dataSize, frameTime);
}


PBoolean OpalVideoMediaStream::Open()
{
  if (isOpen)
    return true;

  unsigned width = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption(), PVideoFrameInfo::QCIFWidth);
  unsigned height = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption(), PVideoFrameInfo::QCIFHeight);

  if (inputDevice != NULL) {
    if (!inputDevice->SetColourFormatConverter(mediaFormat)) {
      PTRACE(1, "Media\tCould not set colour format in grabber to " << mediaFormat);
      return false;
    }
    if (!inputDevice->SetFrameSizeConverter(width, height)) {
      PTRACE(1, "Media\tCould not set frame size in grabber to " << width << 'x' << height << " in " << mediaFormat);
      return false;
    }
   /* if (!inputDevice->Start()) {
      PTRACE(1, "Media\tCould not start video grabber");
      return false;
    }*/
    lastGrabTime = PTimer::Tick();
  }

  if (outputDevice != NULL) {
    if (!outputDevice->SetColourFormatConverter(mediaFormat)) {
      PTRACE(1, "Media\tCould not set colour format in video display to " << mediaFormat);
      return false;
    }
    if (!outputDevice->SetFrameSizeConverter(width, height)) {
      PTRACE(1, "Media\tCould not set frame size in video display to " << width << 'x' << height << " in " << mediaFormat);
      return false;
    }
  }

  SetDataSize(1, 1); // Gets set to minimum of device buffer requirements

  return OpalMediaStream::Open();
}


PBoolean OpalVideoMediaStream::Close()
{
  if (!OpalMediaStream::Close())
    return false;

  if (inputDevice != NULL)
    inputDevice->Close();

  if (outputDevice != NULL)
    outputDevice->Close();

  return true;
}


PBoolean OpalVideoMediaStream::ReadData(BYTE * data, PINDEX size, PINDEX & length)
{
  if (!isOpen)
    return false;

  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return false;
  }

  if (inputDevice == NULL) {
    PTRACE(1, "Media\tTried to read from video display device");
    return false;
  }
  PWaitAndSignal lockit(m_mutexDevice);
#if 0//chenyuan
  if (size < inputDevice->GetMaxFrameBytes()) {
    PTRACE(1, "Media\tTried to read with insufficient buffer size - " << size << " < " << inputDevice->GetMaxFrameBytes());
    return false;
  }
#endif
  unsigned width, height;
  inputDevice->GetFrameSize(width, height);
  OpalVideoTranscoder::FrameHeader * frame = (OpalVideoTranscoder::FrameHeader *)PAssertNULL(data);
  frame->x = frame->y = 0;
  frame->width = width;
  frame->height = height;

  PINDEX bytesReturned = size - sizeof(OpalVideoTranscoder::FrameHeader);
  unsigned flags = 0;
  if (!inputDevice->GetFrameData((BYTE *)OPAL_VIDEO_FRAME_DATA_PTR(frame), &bytesReturned, flags))
    return false;

  PTimeInterval currentGrabTime = PTimer::Tick();
  timestamp += (int)((currentGrabTime - lastGrabTime).GetMilliSeconds()*OpalMediaFormat::VideoClockRate/1000);
  lastGrabTime = currentGrabTime;

  if ((flags & PluginCodec_ReturnCoderRequestIFrame) != 0)
    ExecuteCommand(OpalVideoUpdatePicture());

  marker = true;
  length = bytesReturned;
  if (length > 0)
    length += sizeof(PluginCodec_Video_FrameHeader);

  if (outputDevice == NULL)
    return true;
  outputDevice->SetFrameSize( width, height);
  if (outputDevice->Start())
    return outputDevice->SetFrameData(0, 0, width, height, OPAL_VIDEO_FRAME_DATA_PTR(frame), true, flags);

  PTRACE(1, "Media\tCould not start video display device");
  delete outputDevice;
  outputDevice = NULL;
  return true;
}


PBoolean OpalVideoMediaStream::WriteData(const BYTE * data, PINDEX length, PINDEX & written)
{
  if (!isOpen)
    return false;

  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return false;
  }

  if (outputDevice == NULL) {
    PTRACE(1, "Media\tTried to write to video capture device");
    return false;
  }

  // Assume we are writing the exact amount (check?)
  written = length;

  // Check for missing packets, we do nothing at this level, just ignore it
  if (data == NULL)
    return true;

  const OpalVideoTranscoder::FrameHeader * frame = (const OpalVideoTranscoder::FrameHeader *)data;

  if (!outputDevice->SetFrameSize(frame->width, frame->height)) {
    PTRACE(1, "Media\tCould not resize video display device to " << frame->width << 'x' << frame->height);
    return false;
  }

  if (!outputDevice->Start()) {
    PTRACE(1, "Media\tCould not start video display device");
    return false;
  }

  return outputDevice->SetFrameData(frame->x & 0xFFFF, frame->y & 0xFFFF,/*frame->x, frame->y,*/
                                    frame->width, frame->height,
                                    OPAL_VIDEO_FRAME_DATA_PTR(frame), marker);
}


PBoolean OpalVideoMediaStream::IsSynchronous() const
{
  return IsSource();
}

#endif // OPAL_VIDEO


///////////////////////////////////////////////////////////////////////////////

OpalUDPMediaStream::OpalUDPMediaStream(OpalConnection & conn,
                                      const OpalMediaFormat & mediaFormat,
                                       unsigned sessionID,
                                       PBoolean isSource,
                                       OpalTransportUDP & transport)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource),
    udpTransport(transport)
{
}

OpalUDPMediaStream::~OpalUDPMediaStream()
{
  Close();
}


PBoolean OpalUDPMediaStream::ReadPacket(RTP_DataFrame & Packet)
{
  Packet.SetPayloadType(mediaFormat.GetPayloadType());
  Packet.SetPayloadSize(0);

  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return false;
  }

  PBYTEArray rawData;
  if (!udpTransport.ReadPDU(rawData)) {
    PTRACE(2, "Media\tRead on UDP transport failed: "
       << udpTransport.GetErrorText() << " transport: " << udpTransport);
    return false;
  }

  if (rawData.GetSize() > 0) {
    Packet.SetPayloadSize(rawData.GetSize());
    memcpy(Packet.GetPayloadPtr(), rawData.GetPointer(), rawData.GetSize());
  }

  return true;
}


PBoolean OpalUDPMediaStream::WritePacket(RTP_DataFrame & Packet)
{
  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return false;
  }

  if (Packet.GetPayloadSize() > 0) {
    if (!udpTransport.Write(Packet.GetPayloadPtr(), Packet.GetPayloadSize())) {
      PTRACE(2, "Media\tWrite on UDP transport failed: "
         << udpTransport.GetErrorText() << " transport: " << udpTransport);
      return false;
    }
  }

  return true;
}


PBoolean OpalUDPMediaStream::IsSynchronous() const
{
  return false;
}


PBoolean OpalUDPMediaStream::Close()
{
  if (!isOpen)
    return false;
    
  udpTransport.Close();
  return OpalMediaStream::Close();
}


// End of file ////////////////////////////////////////////////////////////////
