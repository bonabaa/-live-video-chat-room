/*
 * jitter.h
 *
 * Jitter buffer support
 *
 * Open H323 Library
 *
 * Copyright (c) 1999-2001 Equivalence Pty. Ltd.
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
 * $Revision: 23901 $
 * $Author: rjongbloed $
 * $Date: 2010-01-06 00:23:31 +0000 (Wed, 06 Jan 2010) $
 */

#ifndef OPAL_RTP_JITTER_H
#define OPAL_RTP_JITTER_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <rtp/rtp.h>


class RTP_JitterBuffer;
class RTP_JitterBufferAnalyser;


///////////////////////////////////////////////////////////////////////////////
/**This is an Abstract jitter buffer, which can be used simply in any
   application. The user is required to use a descendant of this class, and
   provide a "OnReadPacket" method, so that network packets can be placed in
   this class instance */
class OpalJitterBuffer : public PSafeObject
{
  PCLASSINFO(OpalJitterBuffer, PSafeObject);

  public:
  /**@name Construction */
  //@{
    /**Constructor for this jitter buffer. The size of this buffer can be
       altered later with the SetDelay method
      */
    OpalJitterBuffer(
      unsigned minJitterDelay, ///<  Minimum delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum delay in RTP timestamp units
      unsigned timeUnits = 8,  ///<  Time units, usually 8 or 16
      PINDEX packetSize = 2048 ///<  Max RTP packet size
    );
    
    /** Destructor, which closes this down and deletes the internal list of frames
      */
    virtual ~OpalJitterBuffer();
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Report the statistics for this jitter instance */
    void PrintOn(
      ostream & strm
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Set the maximum delay the jitter buffer will operate to.
      */
    void SetDelay(
      unsigned minJitterDelay, ///<  Minimum delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum delay in RTP timestamp units
      PINDEX packetSize = 2048 ///<  Max RTP packet size
    );

    /** Reset jitter buffer.
        Jitter buffer is cleared and "restocked" from input data.
      */
    void Reset() { m_resetJitterBufferNow = true; }

    /**Write data frame from the RTP channel.
      */
    virtual PBoolean WriteData(
      const RTP_DataFrame & frame   ///< Frame to feed into jitter buffer
    );

    /**Read a data frame from the jitter buffer.
       This function never blocks. If no data is available, an RTP packet
       with zero payload size is returned.
      */
    virtual PBoolean ReadData(
      RTP_DataFrame & frame   ///<  Frame to extract from jitter buffer
    );

    /**Get current delay for jitter buffer.
      */
    DWORD GetJitterTime() const { return currentJitterTime; }

    /**Get time units.
      */
    unsigned GetTimeUnits() const { return timeUnits; }
    
    /**Get total number received packets too late to go into jitter buffer.
      */
    DWORD GetPacketsTooLate() const { return packetsTooLate; }

    /**Get total number received packets that overran the jitter buffer.
      */
    DWORD GetBufferOverruns() const { return bufferOverruns; }

    /**Get maximum consecutive marker bits before buffer starts to ignore them.
      */
    DWORD GetMaxConsecutiveMarkerBits() const { return maxConsecutiveMarkerBits; }

    /**Set maximum consecutive marker bits before buffer starts to ignore them.
      */
    void SetMaxConsecutiveMarkerBits(DWORD max) { maxConsecutiveMarkerBits = max; }
  //@}

  protected:
    class Entry : public RTP_DataFrame
    {
      public:
        Entry(PINDEX sz) : RTP_DataFrame(0, sz) { }
        PTimeInterval tick;
    };
    OpalJitterBuffer::Entry * GetAvailableEntry();
    void InternalWriteData(OpalJitterBuffer::Entry * availableEntry);

    DWORD         minJitterTime;
    DWORD         maxJitterTime;
    unsigned      timeUnits;
    PINDEX        bufferSize;
    DWORD         maxConsecutiveMarkerBits;

    DWORD         currentJitterTime;
    DWORD         packetsTooLate;
    unsigned      bufferOverruns;
    unsigned      consecutiveBufferOverruns;
    DWORD         consecutiveMarkerBits;
    bool          markerWarning;
    PTimeInterval consecutiveEarlyPacketStartTime;
    DWORD         lastWriteTimestamp;
    PTimeInterval lastWriteTick;
    DWORD         jitterCalc;
    DWORD         targetJitterTime;
    unsigned      jitterCalcPacketCount;
    bool          m_resetJitterBufferNow;

    struct FrameQueue : public PList<Entry>
    {
      FrameQueue() { DisallowDeleteObjects(); }
      ~FrameQueue() { AllowDeleteObjects(); }
    };
    FrameQueue freeFrames;
    FrameQueue jitterBuffer;
    Entry * GetNewest(bool pop);
    Entry * GetOldest(bool pop);

    Entry * currentFrame;    // storage of current frame

    PMutex bufferMutex;
    bool   preBuffering;
    bool   firstReadData;

    RTP_JitterBufferAnalyser * analyser;
};


/**A descendant of the OpalJitterBuffer that starts a thread to read
   from something continuously and feed it into the jitter buffer.
  */
class OpalJitterBufferThread : public OpalJitterBuffer
{
    PCLASSINFO(OpalJitterBufferThread, OpalJitterBuffer);
 public:
    OpalJitterBufferThread(
      unsigned minJitterDelay, ///<  Minimum delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum delay in RTP timestamp units
      unsigned timeUnits = 8,  ///<  Time units, usually 8 or 16
      PINDEX packetSize = 2048 ///<  Max RTP packet size
    );
    ~OpalJitterBufferThread();

    /**Read a data frame from the jitter buffer.
       This function never blocks. If no data is available, an RTP packet
       with zero payload size is returned.

       Override of base class so can terminate caller when shutting down.
      */
    virtual PBoolean ReadData(
      RTP_DataFrame & frame   ///<  Frame to extract from jitter buffer
    );

    /**This class instance collects data from the outside world in this
       method.

       @return true on successful read, false on faulty read. */
    virtual PBoolean OnReadPacket(
      RTP_DataFrame & frame,  ///<  Frame read from the RTP session
      PBoolean loop           ///<  If true, loop as long as data is available, if false, only process once
    ) = 0;

  protected:
    PDECLARE_NOTIFIER(PThread, OpalJitterBufferThread, JitterThreadMain);

    /// Internal function to be called from derived class constructor
    void StartThread();

    /// Internal function to be called from derived class destructor
    void WaitForThreadTermination();

    PThread * m_jitterThread;
    bool      m_running;
};


/////////////////////////////////////////////////////////////////////////////
/**A descendant of the OpalJitterBuffer that reads RTP_DataFrame instances
   from the RTP_Sessions
  */
class RTP_JitterBuffer : public OpalJitterBufferThread
{
    PCLASSINFO(RTP_JitterBuffer, OpalJitterBufferThread);
 public:
    RTP_JitterBuffer(
      RTP_Session & session,   ///<  Associated RTP session tor ead data from
      unsigned minJitterDelay, ///<  Minimum delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum delay in RTP timestamp units
      unsigned timeUnits = 8,  ///<  Time units, usually 8 or 16
      PINDEX packetSize = 2048 ///<  Max RTP packet size
    );
    ~RTP_JitterBuffer();

    /**This class instance collects data from the outside world in this
       method.

       @return true on successful read, false on faulty read. */
    virtual PBoolean OnReadPacket(
      RTP_DataFrame & frame,  ///<  Frame read from the RTP session
      PBoolean loop           ///<  If true, loop as long as data is available, if false, only process once
    );

 protected:
   /**This class extracts data from the outside world by reading from this session variable */
   RTP_Session & session;
};

#endif // OPAL_RTP_JITTER_H


/////////////////////////////////////////////////////////////////////////////
