/*
 * Inter Asterisk Exchange 2
 * 
 * The entity which receives all manages weirdo iax2 packets that are 
 * sent outside of a regular call.
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2006 Stephen Cook 
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
 * The Initial Developer of the Original Code is Indranet Technologies Ltd
 *
 * $Revision: 21283 $
 * $Author: rjongbloed $
 * $Date: 2008-10-11 07:10:58 +0000 (Sat, 11 Oct 2008) $
 */

/////////////////////////////////////////////////////////////////////////////

#ifndef OPAL_IAX2_JITTER_H
#define OPAL_IAX2_JITTER_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if OPAL_IAX2

#include <rtp/rtp.h>
#include <rtp/jitter.h>

//class RTP_DataFrame;

PDECLARE_LIST(RTP_DataFrameQueue, RTP_DataFrame *)
#ifdef DOC_PLUS_PLUS     //This makes emacs bracket matching code happy.
/** A list of all RTP_DataFrame that have beeen read from the network. Actually, these frames have derived from IAX2 Mini frames. 
 
    Note please, this class is thread safe.

   You do not need to protect acces to this class.
*/
class RTP_DataFrameQueue : public RTP_DataFrame *
{
#endif
 public:
};

/////////////////////////////////////////////////////////////////////////////
/**A descendant of RTP_DataFrameList that does automatic initialisation*/
class PendingRtpDataFrames : public RTP_DataFrameQueue
{
    PCLASSINFO(PendingRtpDataFrames, RTP_DataFrameQueue);
 public:
    /**Constructor, which turns off deletion on removing an item */
    PendingRtpDataFrames();

    /**Destructor */
    ~PendingRtpDataFrames();

    /**Close this structrue down, so it cannot run again. This method is only
       called immediately prior to destruction. */
    void CloseDown();

     /**Get pointer to last frame in the list. Remove this frame from the list.
      This is a blocking call, and will wait until a frame is received*/
    RTP_DataFrame *GetLastFrame();

    /**Have received a new frame from the frame translater */
    void AddNewFrame(RTP_DataFrame *newFrame);

 protected:
     /**Get pointer to last frame in the list. Remove this frame from the list.
	This is the implementation of GetLastFrame, and does not block */
    RTP_DataFrame *InternalGetLastFrame();


  /**Flag to activate the thread waiting on data*/
  PSyncPoint activate;
 
  /**Local variable which protects access. */
  PMutex mutex;

  /**Flag to indicate this class is running and operational */
  PBoolean keepGoing;
};

/////////////////////////////////////////////////////////////////////////////
/**This class maintains a list of RTP_DataFrame instances, that is filled from
   the IAX2MediaStream class. The Jitter thread periodically grabs on frame
   and jitter corrects the frame */
class IAX2JitterBuffer : public OpalJitterBuffer
{
    PCLASSINFO(IAX2JitterBuffer, OpalJitterBuffer);

 public:
    /**Build a jitter buffer for this connection, which defaults to 8khz*/
    IAX2JitterBuffer();

    /**Stop the jitter buffer thread in this class, which allows the jitter buffer
       to close down nicely */
    ~IAX2JitterBuffer();

    /**This class instance collects data from the outside world in this
       method.

    @return PTrue on successful read, PFalse on faulty read. */
    virtual PBoolean OnReadPacket    (
        RTP_DataFrame & frame,  ///<  Frame read from the RTP session
        PBoolean loop               ///<  If PTrue, loop as long as data is available, if PFalse, only process once
        ) ;

    /**Have receive a new frame from the network. Place it on the internal list */
    void NewFrameFromNetwork(RTP_DataFrame *newFrame)
	{ receivedFrames.AddNewFrame(newFrame); }

    /**Terminate this intance of the jitter buffer permanently, which is
       required prior to destruction. This closedown mechanism is a bit
       different to that used in this libraries SIP/H.323 rtp code. However,
       that code has one socket per media stream. IAX2 and its use of one
       socket for all streams makes life difficult - we cannot just close that
       one main socket and close this stream. */
    void CloseDown() { receivedFrames.CloseDown(); }

 protected:

    /**The list of frames just read from the network*/
    PendingRtpDataFrames receivedFrames;
};


#endif // OPAL_IAX2

#endif // OPAL_IAX2_JITTER_H
