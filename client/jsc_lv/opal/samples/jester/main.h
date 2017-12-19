/*
 * main.h
 *
 * Jester - a tester of the jitter buffer
 *
 * Copyright (c) 2006 Derek J Smithies
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
 * The Original Code is Jester
 *
 * The Initial Developer of the Original Code is Derek J Smithies
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21283 $
 * $Author: rjongbloed $
 * $Date: 2008-10-11 07:10:58 +0000 (Sat, 11 Oct 2008) $
 */

#ifndef _Jester_MAIN_H
#define _Jester_MAIN_H


#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <ptclib/pwavfile.h>
#include <ptlib/sound.h>

#include <rtp/jitter.h>
#include <rtp/rtp.h>

/** We use the IAX2 jitter buffer, as it is close to what we need, and has no
    RTP session stuff to worry about. The iax2 jitter buffer is derived from
    the OpalJitterBuffer, and demands OPAL library code newer than Jan 2007 */

/**** Note, this code will only compile if you use an OPAL library code newer
    than January 2007 */
#include <iax2/iax2jitter.h>   
/**** Note, this code will only compile if you use an OPAL library code newer
    than January 2007 */



#include <ptclib/delaychan.h>
#include <ptclib/random.h>

#if !OPAL_IAX2
#error Cannot compile without IAX2
#endif


/////////////////////////////////////////////////////////////////////////////
/**we use this class primarily to access variables in the OpalJitterBuffer*/
class JesterJitterBuffer : public IAX2JitterBuffer
{
    PCLASSINFO(JesterJitterBuffer, IAX2JitterBuffer);
 public:
    JesterJitterBuffer();

    ~JesterJitterBuffer();

    /**Report the target jitter time, which is the current delay */
    DWORD GetTargetJitterTime() { return targetJitterTime; }

    /**Report the current jitter depth */
    unsigned GetCurrentDepth() { return jitterBuffer.size(); }

    /** repot on an internal variable */
    DWORD GetCurrentJitterTime() { return currentJitterTime; }


    virtual void Close(
	PBoolean /*reading */   ///<  Closing the read side of the session
	);

   /**Reopens an existing session in the given direction.
      */
    virtual void Reopen(
	PBoolean /*isReading */
	) { }

    /**Get the local host name as used in SDES packes.
      */
    virtual PString GetLocalHostName() { return PString("Jester"); }



 protected:
    /**psuedo sequence number that we will put into the packets */
    WORD psuedoSequenceNo;

    /**psuedo timestamp that we will put into the packets. Timestamp goes up
     * by the number of samples placed in the packet. So, 8khz, 30ms duration,
     * means 240 increment for each packet. */
    DWORD psuedoTimestamp;

    /**Flag to indicate we have closed down */
    PBoolean closedDown;

    /**time at which this all started */
    PTime startJester;

    /**Number of times we have read a packet. This is required to determine the required time period
       to sleep */
    PINDEX readCount;
};

/////////////////////////////////////////////////////////////////////////////

/** The main class that is instantiated to do things */
class JesterProcess : public PProcess
{
  PCLASSINFO(JesterProcess, PProcess)

  public:
    JesterProcess();

    void Main();

  protected:

#ifdef DOC_PLUS_PLUS
    /**Generate the Udp packets that we could have read from the internet. In
       other words, place packets in the jitter buffer. */
    virtual void GenerateUdpPackets(PThread &, INT);
#else
    PDECLARE_NOTIFIER(PThread, JesterProcess, GenerateUdpPackets);
#endif

#ifdef DOC_PLUS_PLUS
    /**Read in the Udp packets (from the output of the jitter buffer), that we
       could have read from the internet. In other words, extract packets from
       the jitter buffer. */
    virtual void ConsumeUdpPackets(PThread &, INT);
#else
    PDECLARE_NOTIFIER(PThread, JesterProcess, ConsumeUdpPackets);
#endif

    /**Handle user input, which is keys to describe the status of the program,
       while the different loops run. The program will not finish until this
       method is completed.*/
    void ManageUserInput();
    
    /**Name of the sound device we will write audio to. If not specified, this
       program will write to
       PSoundChannel::GetDefaultDevice(PSoundChannel::Player) */
    PString audioDevice;
    
    /**The number of bytes of data in each simulated RTP_DataFrame */
    PINDEX bytesPerBlock;

    /**Flag to indicate if we do, or do not, simulate silence suppression. If
       PTrue, we do silence suppresion and send packets in bursts of onnnn,
       nothing, onnn, nothing..*/
    PBoolean silenceSuppression;

    /**Flag to indicate if we do, or do not fiddle with the operaiton of
       silence suppression function. When doing silence suppression, the start
       of each talk burst has the marker bit turned on. If this flag is set
       PTrue, then some of those marker bits (half of them) are
       suppressed. This flag therefore tests the operation of the jitter
       buffer, to see if it copes with the dropping of the first packet in
       each voice stream */
    PBoolean markerSuppression;

    /**min size of the jitter buffer in ms */
    PINDEX minJitterSize;

    /**max size of the jitter buffer - time units is ms */
    PINDEX maxJitterSize;

    /**A descendant of the OpalJitterBuffer, which means we have the minimum
       of code to write to test OpalJitterBuffer. Further, we can now access
       variables in the OpalJitterBuffer */
    JesterJitterBuffer jitterBuffer;

    /**The sound channel that we will write audio to*/
    PSoundChannel player;

    /**Name of the wavfile containing the audio we will read in */
    PString wavFile;

    /**the current index that the generate thread is on (or iteration count) */
    PINDEX generateIndex;

    /**the current index that the consume thread is on (or iteration count) */
    PINDEX consumeIndex;

    /**the timestamp, as used by the generate thread */
    DWORD generateTimestamp;

    /**the timestamp, as used by the consume thread */
    DWORD consumeTimestamp;

};


#endif  // _Jester_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
