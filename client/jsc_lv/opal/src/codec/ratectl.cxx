/*
 * ratectl.h
 *
 * Video rate controller
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2007 Matthias Schneider
 * Copyright (C) 2008 Post Increment
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
 * The Initial Developer of the Original Code is Matthias Schneider 
 *
 * Contributor(s): Post Increment
 *
 * $Revision: 23237 $
 * $Author: rjongbloed $
 * $Date: 2009-08-21 00:34:05 +0000 (Fri, 21 Aug 2009) $
 */

#include <ptlib.h>
#include <list>
using namespace std;

#ifdef __GNUC__
#pragma implementation "ratectl.h"
#endif

#include <opal/buildopts.h>
#include <codec/ratectl.h>

#if OPAL_VIDEO

#include <opal/mediafmt.h>


//
// 20 bytes for nominal TCP header
// 8 bytes for nominal UDP header
// 
#define UDP_OVERHEAD  (20 + 8)

//
// size of history used for calcluating average packet size
//
#define PACKET_HISTORY_SIZE   5

#define new PNEW


/////////////////////////////////////////////////////////////////////////////
//
//
//

OpalBitRateCalculator::OpalBitRateCalculator()
{ 
  Reset(); 
}

PInt64 OpalBitRateCalculator::GetNow()  
{ 
  return (PTime().GetTimestamp()+500)/1000; 
}

void OpalBitRateCalculator::Reset()
{
  m_first        = true;
  m_bitRate      = 0;

  m_historySize    = 0;
  m_totalSize      = 0;
  m_historyFrames  = 0;

  m_history.clear();
}

void OpalBitRateCalculator::SetQuanta(unsigned quanta_)
{ 
  m_quanta = quanta_; 
}

void OpalBitRateCalculator::AddPacket(PINDEX size, bool marker)
{
  PInt64 now = GetNow();
  if (m_first) {
    m_baseTimeStamp = now;
    m_first = false;
  }

  m_history.push_back(History(size, now, marker));
  m_historySize    += size;
  m_totalSize      += size;

  if (marker)
    ++m_historyFrames;

  Flush();
}

unsigned OpalBitRateCalculator::GetTrialBitRate(PINDEX size)
{
  PInt64 now = GetNow();
  Flush(now);

  unsigned trialBitRate = 0;

  if (m_history.size() > 0) 
    trialBitRate = (unsigned)((size + (PInt64)m_historySize) * 8 * 1000 / (m_quanta + (now - m_history.front().m_timeStamp)));

  return trialBitRate;
}

unsigned OpalBitRateCalculator::GetBitRate() 
{
  PInt64 now = GetNow();
  Flush(now);

  if (m_history.size() > 0) 
    m_bitRate = (unsigned)(((PInt64)m_historySize * 8 * 1000) / (m_quanta + (now - m_history.front().m_timeStamp)));

  return m_bitRate;
}

unsigned OpalBitRateCalculator::GetAverageBitRate() 
{
  if (m_first)
    return 0;

  return (unsigned)(((m_totalSize * 8 * 1000) / (m_quanta + GetNow() - m_baseTimeStamp)));
}

PInt64 OpalBitRateCalculator::GetTotalSize() const
{ 
  return m_totalSize; 
}

void OpalBitRateCalculator::Flush()
{ 
  Flush(GetNow()); 
}

void OpalBitRateCalculator::Flush(PInt64 now)
{
  // flush history
  while ((m_history.size() > 0) && ((now - m_history.front().m_timeStamp) > 1000)) {
    m_historySize -= m_history.front().m_size;
    if (m_history.front().m_marker)
      --m_historyFrames;
    m_history.pop_front();
  }
}

PInt64 OpalBitRateCalculator::GetTotalTime() const
{
  return GetNow() - m_baseTimeStamp;
}

unsigned OpalBitRateCalculator::GetHistoryFrames() const
{ 
  if (m_history.size() == 0)
    return 0;

  return m_historyFrames + (m_history.back().m_marker ? 0 : 1); 
}

//
//  This file implements a video rate controller that seeks to maintain a constant bit rate 
//  by indicating when encoded video frames should be dropped
//
//  The instantaneous bit rate is monitored by calculating the total number of bytes that have been 
//  transmitted over the past few seconds. This decision to drop a frame is based on whether the 
//  the actual transmitted count is less, or more, then the number of bytes that would have been 
//  transmitted if the target bit rate had been maintained. 
//
//  The size of this history used to calculate the current bit rate is set when the rate 
//  controller is opened. Experience shows that a history of 5000ms seems to work well.
//
//  The decision to drop a frame is made before the frame is encoded. The rate controller predicts
//  the probable size of the encoded frame by looking at the previous 5 encoded frames. Experiments
//  were done with longer histories (for example the same history used for the bit rate calculation)
//  but it was found that this tended to over-estimate the probable frame size due to the inclusion 
//  of occasional I-frames.
//
//  The maximum number of consecutive dropped frames is also set when the rate controller is opened. 
//
//  The bit rate calculations take into account the 28 bytes of IP and UDP overhead on every RTP packet
//  This can make a big difference when small video packets are being transmitted
//
//  Additionally, this code can also enforce an output frame rate
//

/////////////////////////////////////////////////////////////////////////////

static int udiff(unsigned int const subtrahend, unsigned int const subtractor)
{
  return subtrahend - subtractor;
}


static double square(double const arg)
{
  return(arg*arg);
}


double OpalCalcSNR(const BYTE * src1, const BYTE * src2, PINDEX dataLen)
{
  double diff2 = 0.0;
  for (PINDEX i = 0; i < dataLen; ++i)
    diff2 += square(udiff(*src1++, *src2++));

  double const snr = diff2 / dataLen / 255;

  return snr;
}


/////////////////////////////////////////////////////////////////////////////

OpalVideoRateController::OpalVideoRateController()
{
  m_targetBitRate    = 0;
  m_outputFrameTime  = 0;
}

OpalVideoRateController::~OpalVideoRateController()
{
}

void OpalVideoRateController::Open(const OpalMediaFormat & mediaFormat)
{
  m_targetBitRate    = mediaFormat.GetOptionInteger(OpalVideoFormat::TargetBitRateOption());
  m_outputFrameTime  = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameTimeOption()) / 90;
  m_inputFrameCount  = 0;
  m_outputFrameCount = 0;

  PTRACE(4, "RateController\tOpened with rate " << m_targetBitRate << " and frame rate " << 1000 / m_outputFrameTime);

  m_bitRateCalc.SetQuanta(m_outputFrameTime);
}


void OpalVideoRateController::Push(RTP_DataFrameList & inputFrames, bool iFrame)
{ 
  if (inputFrames.GetSize() == 0)
    return;

  inputFrames.DisallowDeleteObjects();

  // add the new data to the unsent frame list
  DWORD startTimeStamp = inputFrames[0].GetTimestamp();
  for (PINDEX i = 0; i < inputFrames.GetSize(); ++i) {
    PAssert(inputFrames[0].GetTimestamp() == startTimeStamp, "Packet pacer input cannot span frames");
    m_packets.push_back(PacketEntry(&inputFrames[i], iFrame));
  }

  inputFrames.RemoveAll();
  inputFrames.AllowDeleteObjects();

  ++m_inputFrameCount;
}

bool OpalVideoRateController::Pop(RTP_DataFrameList & outputPackets, bool & iFrame, bool /*force*/)
{
  while (m_packets.size() > 0) {
    outputPackets.Append(m_packets[0].m_rtp);
    iFrame = m_packets[0].m_iFrame;
    m_bitRateCalc.AddPacket(m_packets[0].m_rtp->GetPayloadSize(), m_packets[0].m_rtp->GetMarker());
    m_packets.pop_front();
  }
  return outputPackets.GetSize() != 0;
}

/////////////////////////////////////////////////////////////////////////////

//
//  This file implements a video rate controller that seeks to maintain a constant bit rate 
//  by indicating when encoded video frames should be dropped
//
//  To use the rate controller, open it with the appropriate parameters. 
//
//  Before encoding a potential output frame, use the SkipFrame function to determine if the 
//  frame should be skipped. If the frame is not skipped, encode the frame and then call AddFrame
//  with the parameters of the final data.
//

class OpalStandardVideoRateController : public OpalVideoRateController
{
  public:
    OpalStandardVideoRateController();

    /** Open the rate controller with the specific parameters
      */
    void Open(
      const OpalMediaFormat & mediaFormat    // media format for video 
    );

    /** Determine if the next frame should be skipped.
      * The rate controller can also indicate whether the next frame should
      * be encoded as an I-frame, which is useful if many frames have been skipped
      */
    virtual bool SkipFrame(
      bool & forceIFrame
    );

    bool Pop(RTP_DataFrameList & outputPackets, bool & iFrame, bool force);

  protected:
    bool CheckFrameRate(bool reporting);
    bool CheckBitRate(bool reporting, unsigned currentBitRate);
    PInt64  startTime;
    PInt64 now;
    PInt64 lastReport;
};


PFACTORY_CREATE(PFactory<OpalVideoRateController>, OpalStandardVideoRateController, "Standard", false);


OpalStandardVideoRateController::OpalStandardVideoRateController()
{
}

void OpalStandardVideoRateController::Open(const OpalMediaFormat & fmt)
{
  OpalVideoRateController::Open(fmt);

  int scaler = fmt.GetOptionInteger("Bit Rate Scaler", 100);
  m_targetBitRate = m_targetBitRate * scaler / 100;

  PTRACE(4, "StandardRateController\tOpened with rate " << m_targetBitRate << " and frame rate " << 1000 / m_outputFrameTime);

  startTime  = PTimer::Tick().GetMilliSeconds();
  lastReport = 0;
}

bool OpalStandardVideoRateController::SkipFrame(bool & iFrame)
{
  // increment incoming frame count
  ++m_inputFrameCount;

  // never force I-frames
  iFrame = false;

  // get "now"
  now = PTimer::Tick().GetMilliSeconds();

  // set flag for reporting every second
  bool reporting = (now - lastReport) > 1000;
  if (reporting)
    lastReport = now;

  // get bit rate now to avoid flushing history multiple times
  unsigned currentBitRate = m_bitRateCalc.GetBitRate();

  // check frame rate
  if (CheckFrameRate(reporting))
    return true;

  // checl bit rate
  return CheckBitRate(reporting, currentBitRate);
}

bool OpalStandardVideoRateController::CheckFrameRate(bool PTRACE_PARAM(reporting))
{
  m_bitRateCalc.Flush();

  unsigned frameHistoryCount = m_bitRateCalc.GetHistoryCount();

  // the first one is always free
  if (frameHistoryCount == 0) {
    PTRACE(5, "RateController\tHistory too small for frame rate control");
    return false;
  }

  PTRACE_IF(3, reporting, "RateController\tReport:Total frames:in=" << m_inputFrameCount
                          << ",out=" << m_outputFrameCount
                          << ",dropped=" << (m_inputFrameCount - m_outputFrameCount)
                          << "(" << (m_inputFrameCount < 1 ? 0 : ((m_inputFrameCount - m_outputFrameCount) * 100 / m_inputFrameCount)) << "%)");

  unsigned outputFrameTime = m_bitRateCalc.GetQuanta();

  // if maintaining a frame rate, check to see if frame should be dropped
  // to make this decision, check what the rate would be if this frame was not dropped
  if (outputFrameTime > 0) {
  
    // if the frame history covers zero time, it is doubtful that outputting this
    // frame will result in an invalid frame rate
    PInt64 frameRateHistoryDuration = now - m_bitRateCalc.GetEarliestHistoryTime();
    if (frameRateHistoryDuration == 0) 
      return false;
  
    // output report now we have useful statistics
    PTRACE_IF(3, reporting, "RateController\tReport:"
                 "in="      << ((m_inputFrameCount-0)  * 1000) / (now - startTime) << " fps,"
                 "out="     << ((m_outputFrameCount-0) * 1000) / (now - startTime) << " fps,"
                 "target="  << (1000 / m_outputFrameTime) << " fps");

    if ((frameRateHistoryDuration + outputFrameTime) > (m_bitRateCalc.GetHistoryFrames()+1) * outputFrameTime) {
      PTRACE(3, "RateController\tSkipping frame to enforce frame rate");
      return true;
    }
  }

  return false;
}

bool OpalStandardVideoRateController::CheckBitRate(bool PTRACE_PARAM(reporting), unsigned PTRACE_PARAM(currentBitRate))
{
  if (m_bitRateCalc.GetHistoryCount() == 0)
    return false;

  // calculate average payload size
  unsigned averagePayloadSize = (int)(m_bitRateCalc.GetHistorySize() / m_bitRateCalc.GetHistoryCount());

  // show some statistics
  PTRACE_IF(3, reporting, "RateController\tReport:"
                          "current=" << currentBitRate << " bps,"
                          "target="  << m_targetBitRate << " bps");

  // allow the packet if the expected history size with this packet is less 
  // than the target history size 
  unsigned trialBitRate = m_bitRateCalc.GetTrialBitRate(averagePayloadSize);
  if (trialBitRate <= m_targetBitRate) 
    return false;

  PTRACE(3, "RateController\tSkipping frame to enforce bit rate");
  return true;
}


bool OpalStandardVideoRateController::Pop(RTP_DataFrameList & outputPackets, bool & iFrame, bool force)
{
  if (!OpalVideoRateController::Pop(outputPackets, iFrame, force))
    return false;

  ++m_outputFrameCount;
  return true;
}



#endif // OPAL_VIDEO
