/*
 * jitter.cxx
 *
 * Jitter buffer support
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
 * $Revision: 23925 $
 * $Author: rjongbloed $
 * $Date: 2010-01-12 07:06:45 +0000 (Tue, 12 Jan 2010) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "jitter.h"
#endif

#include <opal/buildopts.h>

#include <rtp/jitter.h>

/*Number of consecutive attempts to add a packet to the jitter buffer while
  it is full before the system clears the jitter buffer and starts over
  again. */
#define MAX_BUFFER_OVERRUNS 20

/**How much time must elapse with lower jitter before jitter
   buffer size is reduced */
#define DECREASE_JITTER_PERIOD 5000 // milliseconds

/* Percentage of current jitter buffer size that constitutes a "genuinely" smaller
jitter */
#define LOWER_JITTER_MAX_PCNT   80

/* Minimum number of packets that constitute a reliable sample for setting a lower
jitter buffer target */
#define DECREASE_JITTER_MIN_PACKETS 50



#if !PTRACING && !defined(NO_ANALYSER)
#define NO_ANALYSER 1
#endif

#ifdef NO_ANALYSER
  #define ANALYSE(inout, time, extra)
#else
  #define ANALYSE(inout, time, extra) analyser->inout(time, jitterBuffer.GetSize(), extra)

  class RTP_JitterBufferAnalyser : public PObject
  {
      PCLASSINFO(RTP_JitterBufferAnalyser, PObject);

    protected:
      struct Info {
        Info() { }
        DWORD         time;
        PTimeInterval tick;
        int           depth;
        const char *  extra;
      } in[1000], out[1000];
      PINDEX inPos, outPos;

    public:
      RTP_JitterBufferAnalyser()
      {
        inPos = outPos = 1;
        in[0].time = out[0].time = 0;
        in[0].tick = out[0].tick = PTimer::Tick();
        in[0].depth = out[0].depth = 0;
      }

      void In(DWORD time, unsigned depth, const char * extra)
      {
        if (inPos < PARRAYSIZE(in)) {
          in[inPos].tick = PTimer::Tick();
          in[inPos].time = time;
          in[inPos].depth = depth;
          in[inPos++].extra = extra;
        }
      }

      void Out(DWORD time, unsigned depth, const char * extra)
      {
        if (outPos < PARRAYSIZE(out)) {
          out[outPos].tick = PTimer::Tick();
          if (time == 0 && outPos > 0)
            out[outPos].time = out[outPos-1].time;
          else
            out[outPos].time = time;
          out[outPos].depth = depth;
          out[outPos++].extra = extra;
        }
      }

      void PrintOn(ostream & strm) const
      {
        strm << "Input samples: " << inPos << " Output samples: " << outPos << "\n"
                "Dir\tRTPTime\tInDiff\tOutDiff\tInMode\tOutMode\t"
                "InDepth\tOutDep\tInTick\tInDelay\tOutTick\tOutDel\tIODelay\n";
        PINDEX ix = 1;
        PINDEX ox = 1;
        while (ix < inPos || ox < outPos) {
          while (ix < inPos && (ox >= outPos || in[ix].time < out[ox].time)) {
            strm << "In\t"
                 << in[ix].time << '\t'
                 << (in[ix].time - in[ix-1].time) << "\t"
                    "\t"
                 << in[ix].extra << "\t"
                    "\t"
                 << in[ix].depth << "\t"
                    "\t"
                 << (in[ix].tick - in[0].tick) << '\t'
                 << (in[ix].tick - in[ix-1].tick) << "\t"
                    "\t"
                    "\t"
                    "\n";
            ix++;
          }

          while (ox < outPos && (ix >= inPos || out[ox].time < in[ix].time)) {
            strm << "Out\t"
                 << out[ox].time << "\t"
                    "\t"
                 << (out[ox].time - out[ox-1].time) << "\t"
                    "\t"
                 << out[ox].extra << "\t"
                    "\t"
                 << out[ox].depth << "\t"
                    "\t"
                    "\t"
                 << (out[ox].tick - out[0].tick) << '\t'
                 << (out[ox].tick - out[ox-1].tick) << "\t"
                    "\n";
            ox++;
          }

          while (ix < inPos && ox < outPos && in[ix].time == out[ox].time) {
            strm << "I/O\t"
                 << in[ix].time << '\t'
                 << (in[ix].time - in[ix-1].time) << '\t'
                 << (out[ox].time - out[ox-1].time) << '\t'
                 << in[ix].extra << '\t'
                 << out[ox].extra << '\t'
                 << in[ix].depth << '\t'
                 << out[ox].depth << '\t'
                 << (in[ix].tick - in[0].tick) << '\t'
                 << (in[ix].tick - in[ix-1].tick) << '\t'
                 << (out[ox].tick - out[0].tick) << '\t'
                 << (out[ox].tick - out[ox-1].tick) << '\t'
                 << (out[ox].tick - in[ix].tick)
                 << '\n';
            ox++;
            ix++;
          }
        }
      }
  };

#endif

#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalJitterBuffer::OpalJitterBuffer(unsigned minJitter,
                                   unsigned maxJitter,
                                   unsigned units,
                                     PINDEX packetSize)
  : timeUnits(units)
  , maxConsecutiveMarkerBits(10)
  , lastWriteTimestamp(0)
  , jitterCalc(0)
  , jitterCalcPacketCount(0)
  , m_resetJitterBufferNow(false)
  , currentFrame(NULL)
#ifdef NO_ANALYSER
  , analyser(NULL)
#else
  , analyser(new RTP_JitterBufferAnalyser)
#endif
{
  SetDelay(minJitter, maxJitter, packetSize);

  PTRACE(4, "RTP\tJitter buffer created:" << *this);
}


OpalJitterBuffer::~OpalJitterBuffer()
{
  // Free up all the memory allocated (jitterBuffer and freeFrames will self delete)
  delete currentFrame;
  currentFrame = NULL;

#ifndef NO_ANALYSER
  PTRACE(5, "RTP\tJitter buffer analysis: size=" << bufferSize
         << " time=" << currentJitterTime << '\n' << *analyser);
  delete analyser;
#endif

  PTRACE(4, "RTP\tJitter buffer destroyed:" << *this);
}


void OpalJitterBuffer::PrintOn(ostream & strm) const
{
  strm << this
       << " size=" << bufferSize
       << " delay=" << (minJitterTime/timeUnits) << '-'
                    << (currentJitterTime/timeUnits) << '-'
                    << (maxJitterTime/timeUnits) << "ms";
}


void OpalJitterBuffer::SetDelay(unsigned minJitterDelay, unsigned maxJitterDelay, PINDEX packetSize)
{
  bufferMutex.Wait();

  minJitterTime     = minJitterDelay;
  maxJitterTime     = maxJitterDelay;
  currentJitterTime = minJitterDelay;
  targetJitterTime  = minJitterDelay;

  preBuffering  = true;
  firstReadData = true;
  markerWarning = false;

  packetsTooLate                  = 0;
  bufferOverruns                  = 0;
  consecutiveBufferOverruns       = 0;
  consecutiveMarkerBits           = 0;

  // retreive all of the frames in use
  if (currentFrame != NULL) {
    PAssertNULL(currentFrame);
    freeFrames.Append(currentFrame);
    currentFrame = NULL;
  }

  while (jitterBuffer.GetSize() > 0) {
    Entry * entry = GetNewest(true);
    freeFrames.Append(PAssertNULL(entry));
  }

  // Calculate number of frames to allocate, we make the assumption that the
  // smallest packet we can possibly get is 5ms long.
  bufferSize = maxJitterDelay/(5*timeUnits)+1;

  // must be able to handle at least the max number of overruns we accept
  bufferSize = PMAX(MAX_BUFFER_OVERRUNS, bufferSize);

  // resize the free queue for the new buffer size
  while (freeFrames.GetSize() < bufferSize)
    freeFrames.Append(new Entry(packetSize));

  while (freeFrames.GetSize() > bufferSize)
    delete freeFrames.RemoveAt(0);

  PTRACE(3, "RTP\tDelays set for jitter buffer " << *this);

  bufferMutex.Signal();
}


OpalJitterBuffer::Entry * OpalJitterBuffer::GetAvailableEntry()
{
  OpalJitterBuffer::Entry * availableEntry;

  // Get the next free frame available for use for reading from the RTP
  // transport. Place it into a parking spot.
  if (freeFrames.GetSize() > 0) {

    // Take the next free frame and make it the current for reading
    availableEntry = &freeFrames.front();
    freeFrames.RemoveAt(0);
    PTRACE_IF(2, consecutiveBufferOverruns > 1,
                "RTP\tJitter buffer full, threw away "
                << consecutiveBufferOverruns << " oldest frames");
    consecutiveBufferOverruns = 0;
  }
  else {
    // We have a full jitter buffer, need a new frame so take the oldest one
    PAssert(jitterBuffer.GetSize() > 0, "Cannot find free frame in jitter buffer");
    availableEntry = GetOldest(true);

    bufferOverruns++;
    consecutiveBufferOverruns++;
    if (consecutiveBufferOverruns > MAX_BUFFER_OVERRUNS) {
      PTRACE(2, "RTP\tJitter buffer continuously full, throwing away entire buffer.");
      while (jitterBuffer.GetSize() > 0) 
        freeFrames.InsertAt(0, GetOldest(true));
      preBuffering = true;
    }
    else {
      PTRACE_IF(2, (consecutiveBufferOverruns == 1) && (availableEntry != NULL),
                "RTP\tJitter buffer full, throwing away oldest frame ("
                << availableEntry->GetTimestamp() << ')');
    }
  }

  return availableEntry;
}


PBoolean OpalJitterBuffer::WriteData(const RTP_DataFrame & frame)
{
  PWaitAndSignal mutex(bufferMutex);

  OpalJitterBuffer::Entry * availableEntry = GetAvailableEntry();
  if (availableEntry == NULL)
    return false;

  *dynamic_cast<RTP_DataFrame *>(availableEntry) = frame;
  InternalWriteData(availableEntry);
  return true;
}


void OpalJitterBuffer::InternalWriteData(OpalJitterBuffer::Entry * availableEntry)
{
  PAssertNULL(availableEntry);
  availableEntry->tick = PTimer::Tick();

  if (consecutiveMarkerBits < maxConsecutiveMarkerBits) {
    if (availableEntry != NULL && availableEntry->GetMarker()) {
      PTRACE(3, "RTP\tReceived start of talk burst: " << availableEntry->GetTimestamp());
      //preBuffering = true;
      consecutiveMarkerBits++;
    }
    else
      consecutiveMarkerBits = 0;
  }
  else {
    if (availableEntry != NULL && availableEntry->GetMarker())
      availableEntry->SetMarker(false);
    if (!markerWarning && consecutiveMarkerBits == maxConsecutiveMarkerBits) {
      markerWarning = true;
      PTRACE(2, "RTP\tEvery packet has Marker bit, ignoring them from this client!");
    }
  }

  // Have been reading a frame, put it into the queue now, at correct position
  DWORD time = availableEntry->GetTimestamp();

  // add the entry to the jitter buffer
  for (FrameQueue::iterator r = jitterBuffer.begin(); r != jitterBuffer.end(); ++r) {
    if (time < r->GetTimestamp() || ((time == r->GetTimestamp()) && (availableEntry->GetSequenceNumber() < r->GetSequenceNumber()))) {
      PAssertNULL(availableEntry);
      jitterBuffer.Insert(*r, availableEntry);
      ANALYSE(In, time, "OutOfOrder");
      return;
    }
  }

  jitterBuffer.Append(availableEntry);
  ANALYSE(In, time, preBuffering ? "PreBuf" : "");
}


PBoolean OpalJitterBuffer::ReadData(RTP_DataFrame & frame)
{
  // Default response is an empty frame, ie silence
  frame.SetPayloadSize(0);

  PWaitAndSignal mutex(bufferMutex);

  /*Free the frame just written to codec, putting it back into
    the free list and clearing the parking spot for it.
   */
  if (currentFrame != NULL) {
    PAssertNULL(currentFrame);
    freeFrames.Append(currentFrame);
    currentFrame = NULL;
  }

  /*Get the next frame to send to the codec. Takes it from the oldest
    position in the queue, if it is time to do so, and parks it in the
    special member so can unlock the mutex while the writer thread has its
    way with the buffer.
   */
  if (jitterBuffer.GetSize() == 0) {
    /*No data to play! We ran the buffer down to empty, restart buffer by
      setting flag that will fill it again before returning any data.
     */
    preBuffering = true;
    currentJitterTime = targetJitterTime;

    ANALYSE(Out, 0, "Empty");
    return true;
  }

  DWORD requestedTimestamp = frame.GetTimestamp();
  DWORD oldestTimestamp    = GetOldest(false)->GetTimestamp();
  DWORD newestTimestamp    = GetNewest(false)->GetTimestamp();

  /* If there is an opportunity (due to silence in the buffer) to implement a desired 
  reduction in the size of the jitter buffer, effect it */

  if ((targetJitterTime < currentJitterTime) && ((newestTimestamp - oldestTimestamp) < currentJitterTime)) {
    currentJitterTime = ( targetJitterTime > (newestTimestamp - oldestTimestamp)) ?
                          targetJitterTime : (newestTimestamp - oldestTimestamp);

    PTRACE(3, "RTP\tJitter buffer size decreased to "
           << currentJitterTime << " (" << (currentJitterTime/timeUnits) << "ms)");
  }

  /* See if time for this packet, if our oldest frame is older than the
     required age, then use it. If it is not time yet, make sure that the
     writer thread isn't falling behind (not enough MIPS). If the time
     between the oldest and the newest entries in the jitter buffer is
     greater than the size specified for the buffer, then return the oldest
     entry regardless, making the writer thread catch up.
  */

  if (preBuffering) {

    // Reset jitter baseline (should be handled by GetMarker() condition, but just in case...)
    lastWriteTimestamp = 0;
    lastWriteTick = 0;

    // If oldest frame has not been in the buffer long enough, don't return anything yet
    if ((PTimer::Tick() - GetOldest(false)->tick).GetInterval() * timeUnits < currentJitterTime/2) {
      ANALYSE(Out, oldestTimestamp, "PreBuf");
      return true;
    }

    preBuffering = false;
  }


  // Handle short silence bursts in the middle of the buffer
  // If we think we're getting marker bit information, use that
  PBoolean shortSilence = false;
  if (consecutiveMarkerBits < maxConsecutiveMarkerBits) {
    if (GetOldest(false)->GetMarker() && (PTimer::Tick() - GetOldest(false)->tick).GetInterval()* timeUnits < currentJitterTime/2)
      shortSilence = true;
  }
  else if (requestedTimestamp < oldestTimestamp && requestedTimestamp > (newestTimestamp - currentJitterTime))
    shortSilence = true;
  
  if (shortSilence) {
    // It is not yet time for something in the buffer
    ANALYSE(Out, oldestTimestamp, "Wait");
    lastWriteTimestamp = 0;
    lastWriteTick = 0;
    return true;
  }

  // Detach oldest packet from the list, put into parking space
  currentFrame = GetOldest(true);

  // Calculate the jitter contribution of this frame
  // Don't count if start of a talk burst
  if (currentFrame->GetMarker()) {
    lastWriteTimestamp = 0;
    lastWriteTick = 0;
  }

  // calculate jitter
  if (lastWriteTimestamp != 0 && lastWriteTick != 0) {
    int thisJitter = 0;

    if (currentFrame->GetTimestamp() < lastWriteTimestamp) {
      //Not too sure how to handle this situation...
      thisJitter = 0;
    }
    else if (currentFrame->tick < lastWriteTick) {
      //Not too sure how to handle this situation either!
      thisJitter = 0;
    }
    else {  
      thisJitter = (currentFrame->tick - lastWriteTick).GetInterval()*timeUnits +
                   lastWriteTimestamp - currentFrame->GetTimestamp();
    }

    if (thisJitter < 0) thisJitter *=(-1);
    thisJitter *=2; //currentJitterTime needs to be at least TWICE the maximum jitter

    if (thisJitter > (int) currentJitterTime * LOWER_JITTER_MAX_PCNT / 100) {
      targetJitterTime = currentJitterTime;
      PTRACE(5, "RTP\tJitter buffer target realigned to current jitter buffer");
      consecutiveEarlyPacketStartTime = PTimer::Tick();
      jitterCalcPacketCount = 0;
      jitterCalc = 0;
    }
    else {
      if (thisJitter > (int) jitterCalc)
        jitterCalc = thisJitter;
      jitterCalcPacketCount++;

      //If it's bigger than the target we're currently trying to set, adapt that target.
      //Note: this will never make targetJitterTime larger than currentJitterTime due to
      //previous if condition
      DWORD newJitterTime = thisJitter * 100 / LOWER_JITTER_MAX_PCNT;
      if (newJitterTime > maxJitterTime)
        newJitterTime = maxJitterTime;
      if (newJitterTime > targetJitterTime) {
        targetJitterTime = newJitterTime;
        PTRACE(3, "RTP\tJitter buffer target size increased to "
                   << targetJitterTime << " (" << (targetJitterTime/timeUnits) << "ms)");
      }
    }
  }

  lastWriteTimestamp = currentFrame->GetTimestamp();
  lastWriteTick      = currentFrame->tick;

  if (jitterBuffer.GetSize() != 0) {

    // If exceeded current jitter buffer time delay:
    DWORD currentJitterLen = newestTimestamp - currentFrame->GetTimestamp();
    if (currentJitterLen > currentJitterTime) {
      PTRACE(4, "RTP\tJitter buffer length exceeded");
      consecutiveEarlyPacketStartTime = PTimer::Tick();
      jitterCalcPacketCount = 0;
      jitterCalc = 0;
      lastWriteTimestamp = 0;
      lastWriteTick = 0;
      
      // If we haven't yet written a frame, we get one free overrun
      if (firstReadData) {
        PTRACE(4, "RTP\tJitter buffer length exceed was prior to first write. Not increasing buffer size");
        while ((newestTimestamp - currentFrame->GetTimestamp()) > currentJitterTime) {
          ANALYSE(Out, currentFrame->GetTimestamp(), "Overrun");
          freeFrames.Append(currentFrame);
          currentFrame = GetOldest(true);
        }

        firstReadData = false;
        frame = *currentFrame;
        ANALYSE(Out, currentFrame->GetTimestamp(), "");
        return true;
      }


      // See if exceeded maximum jitter buffer time delay, waste them if so
      while ((newestTimestamp - currentFrame->GetTimestamp()) > maxJitterTime) {
        PTRACE(4, "RTP\tJitter buffer oldest packet ("
               << currentFrame->GetTimestamp() << " < "
               << (newestTimestamp - maxJitterTime)
               << ") too late, throwing away");

        packetsTooLate++;

        currentJitterTime = maxJitterTime;
      
        // Throw away the oldest frame and move everything up
        ANALYSE(Out, currentFrame->GetTimestamp(), "Late");
        freeFrames.Append(currentFrame);
        currentFrame = &jitterBuffer.front();
        jitterBuffer.RemoveAt(0);
      }

      // Now change the jitter time to cope with the new size
      // unless already set to maxJitterTime
      if (newestTimestamp - currentFrame->GetTimestamp() > currentJitterTime) 
          currentJitterTime = newestTimestamp - currentFrame->GetTimestamp();

      targetJitterTime = currentJitterTime;
      PTRACE(3, "RTP\tJitter buffer size increased to "
             << currentJitterTime << " (" << (currentJitterTime/timeUnits) << "ms)");
    }
  }

  if ((PTimer::Tick() - consecutiveEarlyPacketStartTime).GetInterval() > DECREASE_JITTER_PERIOD &&
       jitterCalcPacketCount >= DECREASE_JITTER_MIN_PACKETS){
    jitterCalc = jitterCalc * 100 / LOWER_JITTER_MAX_PCNT;
    if (jitterCalc < targetJitterTime / 2) jitterCalc = targetJitterTime / 2;
    if (jitterCalc < minJitterTime) jitterCalc = minJitterTime;
    targetJitterTime = jitterCalc;
    PTRACE(3, "RTP\tJitter buffer target size decreased to "
               << targetJitterTime << " (" << (targetJitterTime/timeUnits) << "ms)");
    jitterCalc = 0;
    jitterCalcPacketCount = 0;
    consecutiveEarlyPacketStartTime = PTimer::Tick();
  }

  /* If using immediate jitter reduction (rather than waiting for silence opportunities)
  then trash oldest frames as necessary to reduce the size of the jitter buffer */
  if (targetJitterTime < currentJitterTime && m_resetJitterBufferNow && jitterBuffer.GetSize() != 0) {
    while ((GetNewest(false)->GetTimestamp() - currentFrame->GetTimestamp()) > targetJitterTime){

      // Throw away the newest entries
      PAssertNULL(GetNewest(false));
      freeFrames.Append(GetNewest(true));

      // Reset jitter calculation baseline
      lastWriteTimestamp = 0;
      lastWriteTick = 0;
    }

    currentJitterTime = targetJitterTime;
    PTRACE(3, "RTP\tJitter buffer size decreased to "
        << currentJitterTime << " (" << (currentJitterTime/timeUnits) << "ms)");
    m_resetJitterBufferNow = false;
  }

  firstReadData = false;
  frame = *currentFrame;
  ANALYSE(Out, currentFrame->GetTimestamp(), "");
  return true;
}


OpalJitterBuffer::Entry * OpalJitterBuffer::GetNewest(bool pop)
{
  Entry * e = &jitterBuffer.back();
  if (pop)
    jitterBuffer.RemoveAt(jitterBuffer.GetSize()-1);
  return e;
}


OpalJitterBuffer::Entry * OpalJitterBuffer::GetOldest(bool pop)
{
  Entry * e = &jitterBuffer.front();
  if (pop)
    jitterBuffer.RemoveAt(0);
  return e;
}


/////////////////////////////////////////////////////////////////////////////

OpalJitterBufferThread::OpalJitterBufferThread(unsigned minJitterDelay,
                                               unsigned maxJitterDelay,
                                               unsigned timeUnits,
                                                 PINDEX packetSize)
  : OpalJitterBuffer(minJitterDelay, maxJitterDelay, timeUnits, packetSize)
  , m_jitterThread(NULL)
  , m_running(true)
{
}


OpalJitterBufferThread::~OpalJitterBufferThread()
{
  WaitForThreadTermination(); // Failsafe, should be called from derived class dtor
}


void OpalJitterBufferThread::StartThread()
{
  bufferMutex.Wait();

  if (m_jitterThread == NULL) {
    m_jitterThread = PThread::Create(PCREATE_NOTIFIER(JitterThreadMain), "RTP Jitter");
    m_jitterThread->Resume();
  }

  bufferMutex.Signal();
}


void OpalJitterBufferThread::WaitForThreadTermination()
{
  m_running = false;

  bufferMutex.Wait();
  PThread * jitterThread = m_jitterThread;
  m_jitterThread = NULL;
  bufferMutex.Signal();

  if (jitterThread != NULL) {
    PTRACE(3, "RTP\tWaiting for thread " << jitterThread->GetThreadName() << " on jitter buffer " << *this);
    PAssert(jitterThread->WaitForTermination(10000), "Jitter buffer thread did not terminate");
    delete jitterThread;
  } 
}


PBoolean OpalJitterBufferThread::ReadData(RTP_DataFrame & frame)
{
  if (m_running)
    return OpalJitterBuffer::ReadData(frame);

  PTRACE(3, "Jitter\tShutting down " << *this);
  return false;
} 


void OpalJitterBufferThread::JitterThreadMain(PThread &, INT)
{
  PTRACE(4, "RTP\tJitter receive thread started: " << *this);

  bufferMutex.Wait();

  while (m_running) {
    OpalJitterBuffer::Entry * availableEntry = GetAvailableEntry();

    do {
      bufferMutex.Signal();

      // Keep reading from the RTP transport frames
      if (!OnReadPacket(*availableEntry, true)) {
        delete availableEntry;  // Destructor won't delete this one, so do it here.
        m_running = false; // Flag to stop the reading side thread
        goto exitThread;
      }
    } while (availableEntry->GetSize() == 0);

    bufferMutex.Wait();
    InternalWriteData(availableEntry);
  }

  bufferMutex.Signal();

exitThread:
  PTRACE(4, "RTP\tJitter receive thread finished: " << *this);
}


/////////////////////////////////////////////////////////////////////////////

RTP_JitterBuffer::RTP_JitterBuffer(RTP_Session & sess,
                                        unsigned minJitterDelay,
                                        unsigned maxJitterDelay,
                                        unsigned time,
                                          PINDEX packetSize)
  : OpalJitterBufferThread(minJitterDelay, maxJitterDelay, time, packetSize),
    session(sess)
{
  StartThread();
}


RTP_JitterBuffer::~RTP_JitterBuffer()
{
  PTRACE(4, "RTP\tDestroying jitter buffer " << *this);

  m_running = false;
  bool reopen = session.Close(true);

  WaitForThreadTermination();

  if (reopen)
    session.Reopen(true);
}


PBoolean RTP_JitterBuffer::OnReadPacket(RTP_DataFrame & frame, PBoolean loop)
{
  PBoolean success = session.ReadData(frame, loop);
  PTRACE(8, "RTP\tOnReadPacket: Frame from network, timestamp " << frame.GetTimestamp());
  return success;
}


/////////////////////////////////////////////////////////////////////////////
