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
 * $Revision: 22839 $
 * $Author: rjongbloed $
 * $Date: 2009-06-11 01:37:03 +0000 (Thu, 11 Jun 2009) $
 */

#ifndef OPAL_RATE_CONTROL_H
#define OPAL_RATE_CONTROL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_VIDEO

#include <rtp/rtp.h>

extern double OpalCalcSNR(const BYTE * src1, const BYTE * src2, PINDEX dataLen);

/**  This class is used to calculate the instantaneous bit rate of a data stream
  *  using a one second sliding window 
 */

class OpalBitRateCalculator
{
  public:
    /**  Create the calculator
    */
    OpalBitRateCalculator();

    /** Reset the statistics
    */
    void Reset();

    /** Set the quanta (usually the frame time)
    */
    void SetQuanta(
      unsigned quanta_
    );

    /** Get the quanta (usually the frame time)
    */
    unsigned GetQuanta() const
    { return m_quanta; }

    /** Add a new packet to the bit rate calculations
    */
    void AddPacket(PINDEX size, bool marker);

    /** Get the instantaneous bit rate
    */
    unsigned GetBitRate();

    /** Get the average bit rate since SetQuanta was called
    */
    unsigned GetAverageBitRate();

    /** Get the average bit rate since SetQuanta was called
    */
    unsigned GetAveragePacketSize();

    /** return the bit rate if the specific data size was transmitted
    */
    unsigned GetTrialBitRate(PINDEX size);

    /** Return total bytes sent since SetQuanta was called
    */
    PInt64 GetTotalSize() const;

    /** Return total miliseconds since SetQuanta was called
    */
    PInt64 GetTotalTime() const;

    /** Return number of frames in history
    */
    size_t GetHistoryCount() const
    { return m_history.size(); }

    /** Return number of bytes in history
    */
    unsigned GetHistorySize() const
    { return m_historySize; }

    /** Return earlist timestamp in history
    */
    PInt64 GetEarliestHistoryTime() const
    { if (m_history.size() == 0) return 0; return m_history.begin()->m_timeStamp; }

    /** Return number of marker bits in history
    */
    unsigned GetHistoryFrames() const;

    // flush old data from history
    void Flush();

    // used to get "now"
    static PInt64 GetNow();

  protected:

    void Flush(PInt64 now);

    struct History {
      History(PINDEX size_, PInt64 timeStamp_, bool marker_)
        : m_size(size_), m_timeStamp(timeStamp_), m_marker(marker_)
      { }

      PINDEX m_size;
      PInt64 m_timeStamp;
      bool m_marker;
    };

    std::deque<History> m_history;

    PINDEX m_historySize;
    PInt64 m_totalSize;
    PINDEX m_historyFrames;

    unsigned m_quanta;
    unsigned m_bitRate;
    bool m_first;
    PInt64 m_baseTimeStamp;
};

//
//  Declare a generic video rate controller class.
//  A rate controller seeks to maintain a constant bit rate by manipulating
//  the parameters of the video stream
//
//  Before encoding a potential output frame, use the SkipFrame function to determine if the 
//  frame should be skipped. 
//
//  If the frame is not skipped, encode the frame and call PushFrame to add the frame to the rate controller queue
//  PopFrame can then be called to retreive frames to transmit
//
//  PushFrame must always be called with packets from a single video frame, but PopFrame may return packets 
//  from multiple video frames
//

class OpalMediaFormat;

class OpalVideoRateController
{
  public:
    OpalVideoRateController();

    virtual ~OpalVideoRateController();

    /** Open the rate controller with the specific parameters
      */
    virtual void Open(
      const OpalMediaFormat & mediaFormat
    );

    /** Determine if the next frame should be skipped.
      * The rate controller can also indicate whether the next frame should
      * be encoded as an I-frame, which is useful if many frames have been skipped
      */
    virtual bool SkipFrame(
      bool & forceIFrame
    ) = 0;

    /** push encoded frames into the rate controller queue
      */
    virtual void Push(
      RTP_DataFrameList & inputFrames, 
      bool iFrame
    );

    /** retreive encoded frames from the rate controller queue
      */
    virtual bool Pop(
      RTP_DataFrameList & outputPackets, 
      bool & iFrame, 
      bool force
    );

    /** Bit rate calculator used by rate controller
      */
    OpalBitRateCalculator m_bitRateCalc;

  protected:
    unsigned m_targetBitRate;
    unsigned m_outputFrameTime;
    PInt64   m_inputFrameCount;
    PInt64   m_outputFrameCount;

    struct PacketEntry {
      PacketEntry(RTP_DataFrame * rtp_, bool iFrame_)
        : m_rtp(rtp_), m_iFrame(iFrame_)
      { }

      RTP_DataFrame * m_rtp;
      bool m_iFrame;
    };
    std::deque<PacketEntry> m_packets;
};

namespace PWLibStupidLinkerHacks {
  extern int rateControlKickerVal;
//  static class RateControlKicker { public: RateControlKicker() { rateControlKickerVal = 1; } } rateControlKicker;
};

#endif // OPAL_VIDEO

#endif // OPAL_RATE_CONTROL_H
