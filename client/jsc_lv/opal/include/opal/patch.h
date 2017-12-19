/*
 * patch.h
 *
 * Media stream patch thread.
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
 * $Revision: 23973 $
 * $Author: rjongbloed $
 * $Date: 2010-01-25 00:33:51 +0000 (Mon, 25 Jan 2010) $
 */

#ifndef OPAL_OPAL_PATCH_H
#define OPAL_OPAL_PATCH_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/mediastrm.h>
#include <opal/mediacmd.h>
#include <codec/ratectl.h>

#include <list>

class OpalTranscoder;

/**Media stream "patch cord".
   This class is the thread of control that transfers data from one
   "source" OpalMediStream to one or more other "sink" OpalMediStream
   instances. It may use zero, one or two intermediary software codecs for
   each sink stream in order to match the media data formats the streams are
   capabile of doing natively.

   Note the thread is not actually started straight away. It is expected that
   the Start() function is called on the patch when the creator code is
   ready for it to begin. For example all sink streams have been added.
  */
class OpalMediaPatch : public PObject
{
    PCLASSINFO(OpalMediaPatch, PObject);
  public:
  /**@name Construction */
  //@{
    /**Create a new patch.
       Note the thread is not started here.
     */
    OpalMediaPatch(
      OpalMediaStream & source       ///<  Source media stream
    );

    /**Destroy patch.
     */
    ~OpalMediaPatch();
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Standard stream print function.
       The PObject class has a << operator defined that calls this function
       polymorphically.
      */
    void PrintOn(
      ostream & strm    ///<  Stream to output text representation
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Start the patch. The default implementation simply starts the
       patch thread, which in turn calls Main()
      */
    virtual void Start();

    /**Indicate the patch has started. Typically called from the beginning
       of the patch thread.

       Default behaviour make sure jitter buffer is created.

       Returns bool indicating all media streams are asynchronous so the patch
               should assure no thread startvation
      */
    virtual bool OnStartMediaPatch();

    /**Close the patch.
      This is an internal function that closes all of the sink streams and
       waits for the the thread to terminate. It is called when the source
       stream is called.
      */
    virtual void Close();

    /**Add another "sink" OpalMediaStream to patch.
       The stream must not be a ReadOnly media stream for the patch to be
       able to write to it.
      */
    PBoolean AddSink(
      const OpalMediaStreamPtr & stream            ///< Media stream to add.
      ,bool bOnlyAppend= false
    );

    /**Add existing "sink" OpalMediaStream to patch.
       If the stream is not a sink of this patch then this function does
       nothing.
      */
    void RemoveSink(
      const OpalMediaStreamPtr & stream  ///<  Media stream to remove
    );
    void RemoveSinkByUID(unsigned int uid);
    void RemoveAndCloseSinkByUID(unsigned int uid);
    bool HasSinkByUID(unsigned int uid);


    /**Get the current source stream for patch.
      */
    OpalMediaStream & GetSource() const { return source; }

    /**Get the media stream for a sink stream
      */
    OpalMediaStreamPtr GetSink(PINDEX i = 0) const;

    /**Get the media format for a sink stream
      */
    OpalMediaFormat GetSinkFormat(PINDEX i = 0) const;

    /**Add a filter to the media pipeline.
       Use PDECLARE_NOTIFIER(RTP_DataFrame, YourClass, YourFunction) for the
       filter function notifier.
      */
    void AddFilter(
      const PNotifier & filter,
      const OpalMediaFormat & stage = OpalMediaFormat()
    );

    /**Remove a filter from the media pipeline.
      */
    PBoolean RemoveFilter(
      const PNotifier & filter,
      const OpalMediaFormat & stage = OpalMediaFormat()
    );

    /**Filter a frame. Calls all filter functions.
      */
    virtual void FilterFrame(
      RTP_DataFrame & frame,
      const OpalMediaFormat & mediaFormat
    );

    /**Update the source/sink media format. This can be used to adjust the
       parameters of a codec at run time. Note you cannot change the basic
       media format, eg change GSM0610 to G.711, only options for that
       format, eg 6k3 mode to 5k3 mode in G.723.1. If the formats are
       different then a OpalMediaFormat::Merge() is performed.

       The default behaviour updates the source/sink media stream and the
       output side of any relevant transcoders.
      */
    virtual bool UpdateMediaFormat(
      const OpalMediaFormat & mediaFormat  ///<  New media format
    );

    /**Execute the command specified to the transcoder. The commands are
       highly context sensitive, for example VideoFastUpdate would only apply
       to a video transcoder.

       The default behaviour passes the command on to the source or sinks
       and the sinks transcoders.
      */
    virtual PBoolean ExecuteCommand(
      const OpalMediaCommand & command,   ///<  Command to execute.
      PBoolean fromSink                       ///<  Flag for source or sink
    );

    /**Set a notifier to receive commands generated by the transcoder. The
       commands are highly context sensitive, for example VideoFastUpdate
       would only apply to a video transcoder.

       The default behaviour passes the command on to the source or sinks
       and the sinks transcoders.
      */
    virtual void SetCommandNotifier(
      const PNotifier & notifier,   ///<  Command to execute.
      PBoolean fromSink                 ///<  Flag for source or sink
    );

    /**Push a frame out to all the sink streams, transcoding as necessary.
      */
    virtual PBoolean PushFrame(
      RTP_DataFrame & frame   ///< Frame to push
    );

    /**Set bypass patch instance.

       This can be useful for back to back calls that happen to be the same
       media format and you wish to avoid double decoding and encoding of
       media. Note this scenario is not the same as two OpalConnections within
       the same OpalCall, but two completely independent OpalCall where one
       connection is to be bypassed. For example, two OpalCall instances might
       have two SIPConnection instances and two OpalMixerConnection instances
       connected via a single OpalMixerNode. Now while there are ONLY two
       calls in the node, it is a waste to decode the audio, add to mixer and
       re-encode it again. In practice this is identical to just bypassing the
       mixer node completely, until a third party is added, then we need to
       switch back to normal (non-bypassed) operation.

       Note it is up to the caller to assure the patch instance lifetimes are
       correct for the bypass.

       @return true if bypass was set, false if conflict with another bypass.
      */
    bool SetBypassPatch(
      OpalMediaPatch * patch
    );

    /**Get the transcoder used within a sink stream
      */
    virtual OpalTranscoder * GetAndLockSinkTranscoder(PINDEX i = 0) const;
    virtual void UnLockSinkTranscoder() const;

#if OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics, bool fromSink) const;
		void GetStatisticsBySinkIndex(OpalMediaStatistics & statistics, PINDEX nSinkIndex) const;
		int GetStatisticsAll(  )  ;

#endif
  //@}
    bool GetRecording(){return m_bRecording;};
    void SetRecording(bool bRecord){m_bRecording =bRecord;
    };
    bool RecordRTPFrame(OpalMediaStreamPtr& recordmedia,  RTP_DataFrame& frame_o, unsigned nCaller, unsigned nCallee );
    int m_nDisableVideoInternel;//how many time to disable send video
  protected:
                
    /**Called from the associated patch thread */
    virtual void Main();

    virtual bool DispatchFrame(RTP_DataFrame & frame);
    bool m_bRecording; 
    OpalMediaStream & source;
    PTime m_timeLastReadPacket;
    class Sink : public PObject {
        PCLASSINFO(Sink, PObject);
      public:
        Sink(OpalMediaPatch & p, const OpalMediaStreamPtr & s);
        ~Sink();
        bool UpdateMediaFormat(const OpalMediaFormat & mediaFormat);
        bool ExecuteCommand(const OpalMediaCommand & command);
        void SetCommandNotifier(const PNotifier & notifier);
        bool WriteFrame(RTP_DataFrame & sourceFrame);
        bool WriteFrameEx(RTP_DataFrame & sourceFrame);
#if OPAL_STATISTICS
        void GetStatistics(OpalMediaStatistics & statistics, bool fromSource) const;
#endif

        OpalMediaPatch  &  patch;
        OpalMediaStreamPtr stream;
        OpalTranscoder  * primaryCodec;
        OpalTranscoder  * secondaryCodec;
        RTP_DataFrameList intermediateFrames;
        RTP_DataFrameList finalFrames;
        bool              writeSuccessful;

#if OPAL_VIDEO
        void SetRateControlParameters(const OpalMediaFormat & mediaFormat);
        bool RateControlExceeded(bool & forceIFrame);
        OpalVideoRateController * rateController;
#endif
    };
    PList<Sink> sinks;

    class Filter : public PObject {
        PCLASSINFO(Filter, PObject);
      public:
        Filter(const PNotifier & n, const OpalMediaFormat & s) : notifier(n), stage(s) { }
        PNotifier notifier;
        OpalMediaFormat stage;
    };
    PList<Filter> filters;

#if OPAL_VIDEO
    bool             m_videoDecoder;
#endif
    bool             m_bypassActive;
    OpalMediaPatch * m_bypassToPatch;
    OpalMediaPatch * m_bypassFromPatch;
    PSyncPoint       m_bypassEnded;

    class Thread : public PThread {
        PCLASSINFO(Thread, PThread);
      public:
        Thread(OpalMediaPatch & p);
        virtual void Main() { patch.Main(); };
        OpalMediaPatch & patch;
    };

    Thread * patchThread;
    PMutex patchThreadMutex;
 public:
    mutable PReadWriteMutex inUse;

    bool  EraseSinkNoLock( PList<Sink>::iterator itor );
  private:
    P_REMOVE_VIRTUAL(bool, OnPatchStart(), false);

};

/**Passive Media Patch
   In contrast to the 'default' media patch does this instance not run
   it's own thread. Instead, the source stream may push data to the sinks
   whenever needed. Useful for implementations where the source is not
   a continuous data stream (e.g. H.224/H.281 FECC, where data is sent
   on user input) or the source stream is driven by an external thread loop.
*/
class OpalPassiveMediaPatch : public OpalMediaPatch
{
    PCLASSINFO(OpalPassiveMediaPatch, OpalMediaPatch);
  public:

    OpalPassiveMediaPatch(
      OpalMediaStream & source       ///<  Source media stream
    );

    virtual void Start();
};


#endif // OPAL_OPAL_PATCH_H


// End of File ///////////////////////////////////////////////////////////////
