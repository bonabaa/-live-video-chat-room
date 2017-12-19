/*
 * patch.cxx
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

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "patch.h"
#endif


#include <opal/buildopts.h>

#include <opal/patch.h>
#include <opal/mediastrm.h>
#include <opal/transcoders.h>

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif

#define new PNEW
#ifdef _WIN32
//#define D_AUDIO_YY
extern     OpalMediaStream* m_pSourceIVoiceStream;
#endif
/////////////////////////////////////////////////////////////////////////////
extern PAtomicInteger g_nDelayMicsec;
OpalMediaPatch::OpalMediaPatch(OpalMediaStream & src)
  : source(src)
#if OPAL_VIDEO
  , m_videoDecoder(src.GetMediaFormat().IsTransportable() && src.GetMediaFormat().GetMediaType() == OpalMediaType::Video())
#endif
  , m_bypassActive(false)
  , m_bypassToPatch(NULL)
  , m_bypassFromPatch(NULL)
  , patchThread(NULL)
{
  src.SetPatch(this);
  PTRACE(5, "Patch\tCreated media patch " << this);
}


OpalMediaPatch::~OpalMediaPatch()
{
  PWaitAndSignal m(patchThreadMutex);
  inUse.StartWrite();
  if (patchThread != NULL) {
    PAssert(patchThread->WaitForTermination(10000), "Media patch thread not terminated.");
    delete patchThread;
    patchThread = NULL;
  }
  PTRACE(5, "Patch\tDestroyed media patch " << this);
}


void OpalMediaPatch::PrintOn(ostream & strm) const
{
  strm << "Patch[" << this << "] " << source;

  inUse.StartRead();

  if (sinks.GetSize() > 0) {
    strm << " -> ";
    if (sinks.GetSize() == 1)
      strm << *sinks.front().stream;
    else {
      PINDEX i = 0;
      for (PList<Sink>::const_iterator s = sinks.begin(); s != sinks.end(); ++s,++i) {
        if (i > 0)
          strm << ", ";
        strm << "sink[" << i << "]=" << *s->stream;
      }
    }
  }

  inUse.EndRead();
}

void OpalMediaPatch::Start()
{
  PWaitAndSignal m(patchThreadMutex);
	
  if(patchThread != NULL) 
    return;
	
  patchThread = new Thread(*this);
  patchThread->Resume();
  PThread::Yield();
  PTRACE(4, "Media\tStarting thread " << patchThread->GetThreadName());
}


void OpalMediaPatch::Close()
{
  PTRACE(3, "Patch\tClosing media patch " << *this);

  inUse.StartWrite();

  if (m_bypassFromPatch != NULL)
    m_bypassFromPatch->SetBypassPatch(NULL);
  else
    SetBypassPatch(NULL);

  filters.RemoveAll();
  if (source.GetPatch() == this)
    source.Close();

  while (sinks.GetSize() > 0) {
    OpalMediaStreamPtr stream = sinks.front().stream;
    inUse.EndWrite();
    if (!stream->Close()) {
      // The only way we can get here is if the sink is in the proccess of being closed
      // but is blocked on the inUse mutex waiting to remove the sink from this patch.
      // Se we unlock it, and wait for it to do it in the other thread.
      PThread::Sleep(10);
    }
    inUse.StartWrite();
  }

  PTRACE(4, "Patch\tWaiting for media patch thread to stop " << *this);
  {
    PWaitAndSignal m(patchThreadMutex);
    if (patchThread != NULL && !patchThread->IsSuspended()) {
      inUse.EndWrite();
#ifdef _DEBUG
      PAssert(patchThread->WaitForTermination(10000), "Media patch thread not terminated.");
#else
      patchThread->WaitForTermination(10000);
#endif
      return;
    }
  }

  inUse.EndWrite();
}


PBoolean OpalMediaPatch::AddSink(const OpalMediaStreamPtr & sinkStream)
{
  PWriteWaitAndSignal mutex(inUse);

  if (PAssertNULL(sinkStream) == NULL)
    return false;

  PAssert(sinkStream->IsSink(), "Attempt to set source stream as sink!");

  if (!sinkStream->SetPatch(this)) {
    PTRACE(2, "Patch\tCould not set patch in stream " << *sinkStream);
    return false;
  }

  Sink * sink = new Sink(*this, sinkStream);
  sinks.Append(sink);

  // Find the media formats than can be used to get from source to sink
  OpalMediaFormat destinationFormat = sinkStream->GetMediaFormat();
#ifdef _WIN32
  if (destinationFormat.GetName() == YYCODE_CORE && this->GetSource().GetSessionID() == 0 ) return true;
#endif
  OpalMediaFormat sourceFormat = source.GetMediaFormat();
  PTRACE(5, "Patch\tAddSink\n"
            "Source format:\n" << setw(-1) << sourceFormat << "\n"
            "Destination format:\n" << setw(-1) << destinationFormat);

  if (sourceFormat == destinationFormat) {
    PINDEX framesPerPacket = destinationFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(),
                                  sourceFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1));
    PINDEX packetSize = sourceFormat.GetFrameSize()*framesPerPacket;
    PINDEX packetTime = sourceFormat.GetFrameTime()*framesPerPacket;
    source.SetDataSize(packetSize, packetTime);
    sinkStream->SetDataSize(packetSize, packetTime);
    PTRACE(3, "Patch\tAdded direct media stream sink " << *sinkStream);
    return true;
  }

  PString id = sinkStream->GetID();
  sink->primaryCodec = OpalTranscoder::Create(sourceFormat, destinationFormat, (const BYTE *)id, id.GetLength());
  if (sink->primaryCodec != NULL) {
    PTRACE(4, "Patch\tCreated primary codec " << sourceFormat << "->" << destinationFormat << " with ID " << id);

    if (!sinkStream->SetDataSize(sink->primaryCodec->GetOptimalDataFrameSize(false), sourceFormat.GetFrameTime())) {
      PTRACE(1, "Patch\tSink stream " << *sinkStream << " cannot support data size "
              << sink->primaryCodec->GetOptimalDataFrameSize(PFalse));
      return false;
    }
    sink->primaryCodec->SetMaxOutputSize(sinkStream->GetDataSize());

    PTRACE(3, "Patch\tAdded media stream sink " << *sinkStream
           << " using transcoder " << *sink->primaryCodec << ", data size=" << sinkStream->GetDataSize());
  }
  else {
    OpalMediaFormat intermediateFormat;
    if (!OpalTranscoder::FindIntermediateFormat(sourceFormat, destinationFormat,
                                                intermediateFormat)) {
      PTRACE(1, "Patch\tCould find compatible media format for " << *sinkStream);
      return false;
    }

    if (intermediateFormat.GetMediaType() == OpalMediaType::Audio()) {
      // try prepare intermediateFormat for correct frames to frames transcoding
      // so we need make sure that tx frames time of destinationFormat be equal 
      // to tx frames time of intermediateFormat (all this does not produce during
      // Merge phase in FindIntermediateFormat)
      int destinationPacketTime = destinationFormat.GetFrameTime()*destinationFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1);
      if ((destinationPacketTime % intermediateFormat.GetFrameTime()) != 0) {
        PTRACE(1, "Patch\tCould produce without buffered media format converting (which not implemented yet) for " << *sinkStream);
        return false;
      }
      intermediateFormat.AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::TxFramesPerPacketOption(),
                                                               true,
                                                               OpalMediaOption::NoMerge,
                                                               destinationPacketTime/intermediateFormat.GetFrameTime()),
                                   true);
    }

    sink->primaryCodec = OpalTranscoder::Create(sourceFormat, intermediateFormat, (const BYTE *)id, id.GetLength());
    sink->secondaryCodec = OpalTranscoder::Create(intermediateFormat, destinationFormat, (const BYTE *)id, id.GetLength());

    PTRACE(4, "Patch\tCreated two stage codec " << sourceFormat << "/" << intermediateFormat << "/" << destinationFormat << " with ID " << id);

    sink->primaryCodec->SetMaxOutputSize(sink->secondaryCodec->GetOptimalDataFrameSize(true));

    if (!sinkStream->SetDataSize(sink->secondaryCodec->GetOptimalDataFrameSize(false), sourceFormat.GetFrameTime())) {
      PTRACE(1, "Patch\tSink stream " << *sinkStream << " cannot support data size "
              << sink->secondaryCodec->GetOptimalDataFrameSize(PFalse));
      return false;
    }
    sink->secondaryCodec->SetMaxOutputSize(sinkStream->GetDataSize());

    PTRACE(3, "Patch\tAdded media stream sink " << *sinkStream
           << " using transcoders " << *sink->primaryCodec
           << " and " << *sink->secondaryCodec << ", data size=" << sinkStream->GetDataSize());
  }

  source.SetDataSize(sink->primaryCodec->GetOptimalDataFrameSize(true), destinationFormat.GetFrameTime());
  return true;
}


void OpalMediaPatch::RemoveSink(const OpalMediaStreamPtr & stream)
{
  if (PAssertNULL(stream) == NULL)
    return;

  PTRACE(3, "Patch\tRemoving media stream sink " << *stream);

  inUse.StartWrite();

  for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s) {
    if (s->stream == stream) {
      sinks.erase(s);
      PTRACE(5, "Patch\tRemoved media stream sink " << *stream);
      break;
    }
  }

  if (m_bypassFromPatch != NULL && sinks.IsEmpty())
    m_bypassFromPatch->SetBypassPatch(NULL);

  inUse.EndWrite();
}
void OpalMediaPatch::RemoveAndCloseSinkByUID(unsigned int uid)
{
 
  PTRACE(3, "Patch\tRemoving media stream sink " << uid);

  inUse.StartWrite();

  for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s) {
    OpalMediaStreamPtr stream= s->stream;
    if (stream->m_uid == uid) {
      PTRACE(5, "Patch\tRemoved media stream sink " << s->stream << ",uid=" << uid);
      sinks.erase(s);
      inUse.EndWrite();
      stream->Close();
      inUse.StartWrite();
      break;
    }
  }

  if (m_bypassFromPatch != NULL && sinks.IsEmpty())
    m_bypassFromPatch->SetBypassPatch(NULL);

  inUse.EndWrite();
}


bool OpalMediaPatch::HasSinkByUID(unsigned int uid)
{
 
  //PTRACE(3, "Patch\tRemoving media stream sink " << uid);

   PReadWaitAndSignal mutex(inUse);


  for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s) {
    if (s->stream!=NULL&& s->stream->m_uid == uid) {
        
      return true;
    }
  }

 

   
  return false;
}
void OpalMediaPatch::RemoveSinkByUID(unsigned int uid)
{
 
  PTRACE(3, "Patch\tRemoving media stream sink " << uid);

  inUse.StartWrite();

  for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s) {
    if (s->stream!=NULL&& s->stream->m_uid == uid) {
      PTRACE(5, "Patch\tRemoved media stream sink " << s->stream << ",uid=" << uid);
      sinks.erase(s);
      break;
    }
  }

  if (m_bypassFromPatch != NULL && sinks.IsEmpty())
    m_bypassFromPatch->SetBypassPatch(NULL);

  inUse.EndWrite();
}


OpalMediaStreamPtr OpalMediaPatch::GetSink(PINDEX i) const
{
  PReadWaitAndSignal mutex(inUse);
  return i < sinks.GetSize() ? sinks[i].stream : OpalMediaStreamPtr();
}


OpalMediaFormat OpalMediaPatch::GetSinkFormat(PINDEX i) const
{
  OpalMediaFormat fmt;

  OpalTranscoder * xcoder = GetAndLockSinkTranscoder(i);
  if (xcoder != NULL) {
    fmt = xcoder->GetOutputFormat();
    UnLockSinkTranscoder();
  }

  return fmt;
}


OpalTranscoder * OpalMediaPatch::GetAndLockSinkTranscoder(PINDEX i) const
{
  inUse.StartRead();

  if (i >= sinks.GetSize()) {
    UnLockSinkTranscoder();
    return NULL;
  }

  Sink & sink = sinks[i];
  if (sink.secondaryCodec != NULL) 
    return sink.secondaryCodec;

  if (sink.primaryCodec != NULL)
    return sink.primaryCodec;

  UnLockSinkTranscoder();

  return NULL;
}


void OpalMediaPatch::UnLockSinkTranscoder() const
{
  inUse.EndRead();
}


#if OPAL_STATISTICS
void OpalMediaPatch::GetStatistics(OpalMediaStatistics & statistics, bool fromSink) const
{
  inUse.StartRead();

  if (fromSink)
    source.GetStatistics(statistics, true);

  if (!sinks.IsEmpty())
    sinks.front().GetStatistics(statistics, !fromSink);

  inUse.EndRead();
}
void OpalMediaPatch::GetStatisticsBySinkIndex(OpalMediaStatistics & statistics, PINDEX nIndex) const
{
  inUse.StartRead();
	if (nIndex < sinks.GetSize()  )
	{
    sinks[nIndex].GetStatistics(statistics, false);
	}
  inUse.EndRead();
}

void OpalMediaPatch::Sink::GetStatistics(OpalMediaStatistics & statistics, bool fromSource) const
{
  if (fromSource)
    stream->GetStatistics(statistics, true);

  if (primaryCodec != NULL)
	{
    primaryCodec->GetStatistics(statistics);
		//if (PIsDescendant(primaryCodec, OpalVideoTranscoder ))
		//{
		//	statistics.m_nBytesPerSec = ((OpalVideoTranscoder*)primaryCodec)->m_totalBytesPerSec;
		//	statistics.m_nFramesPerSec = ((OpalVideoTranscoder*)primaryCodec)->m_totalFramesPerSec;
		//}
	}
  if (secondaryCodec != NULL)
    secondaryCodec->GetStatistics(statistics);
}
#endif


OpalMediaPatch::Sink::Sink(OpalMediaPatch & p, const OpalMediaStreamPtr & s)
  : patch(p)
  , stream(s)
  , primaryCodec(NULL)
  , secondaryCodec(NULL)
  , writeSuccessful(true)
#if OPAL_VIDEO
  , rateController(NULL)
#endif
{
#if OPAL_VIDEO
  SetRateControlParameters(stream->GetMediaFormat());
#endif

  PTRACE(3, "Patch\tCreated Sink: format=" << stream->GetMediaFormat());
}


OpalMediaPatch::Sink::~Sink()
{
  delete primaryCodec;
  delete secondaryCodec;
#if OPAL_VIDEO
  delete rateController;
#endif
}


void OpalMediaPatch::AddFilter(const PNotifier & filter, const OpalMediaFormat & stage)
{
  PWriteWaitAndSignal mutex(inUse);

  if (source.GetMediaFormat().GetMediaType() != stage.GetMediaType())
    return;

  // ensures that a filter is added only once
  for (PList<Filter>::iterator f = filters.begin(); f != filters.end(); ++f) {
    if (f->notifier == filter && f->stage == stage) {
      PTRACE(3, "OpalCon\tFilter already added for stage " << stage);
      return;
    }
  }
  filters.Append(new Filter(filter, stage));
}


PBoolean OpalMediaPatch::RemoveFilter(const PNotifier & filter, const OpalMediaFormat & stage)
{
  PWriteWaitAndSignal mutex(inUse);

  for (PList<Filter>::iterator f = filters.begin(); f != filters.end(); ++f) {
    if (f->notifier == filter && f->stage == stage) {
      filters.erase(f);
      return true;
    }
  }

  PTRACE(2, "OpalCon\tNo filter to remove for stage " << stage);
  return false;
}


void OpalMediaPatch::FilterFrame(RTP_DataFrame & frame,
                                 const OpalMediaFormat & mediaFormat)
{
  inUse.StartRead();

  for (PList<Filter>::iterator f = filters.begin(); f != filters.end(); ++f) {
    if (f->stage.IsEmpty() || f->stage == mediaFormat)
      f->notifier(frame, (INT)this);
  }

  inUse.EndRead();
}


bool OpalMediaPatch::UpdateMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PReadWaitAndSignal mutex(inUse);

  bool atLeastOne = source.UpdateMediaFormat(mediaFormat, true);

  for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s)
    atLeastOne = s->UpdateMediaFormat(mediaFormat) || atLeastOne;

  PTRACE_IF(2, !atLeastOne, "Patch\tCould not update media format for any stream/transcoder in " << *this);
  return atLeastOne;
}


PBoolean OpalMediaPatch::ExecuteCommand(const OpalMediaCommand & command, PBoolean fromSink)
{
  PReadWaitAndSignal mutex(inUse);

  if (fromSink)
    return source.ExecuteCommand(command);

  PBoolean atLeastOne = PFalse;
  for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s)
    atLeastOne = s->ExecuteCommand(command) || atLeastOne;

  return atLeastOne;
}


void OpalMediaPatch::SetCommandNotifier(const PNotifier & notifier, PBoolean fromSink)
{
  PReadWaitAndSignal mutex(inUse);

  if (fromSink)
    source.SetCommandNotifier(notifier);
  else {
    for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s)
      s->SetCommandNotifier(notifier);
  }
}


bool OpalMediaPatch::OnStartMediaPatch()
{
  source.OnStartMediaPatch();

  if (source.IsSynchronous())
    return false;

  PReadWaitAndSignal mutex(inUse);

  for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s) {
    if (s->stream->IsSynchronous()) {
      source.EnableJitterBuffer();
      return false;
    }
  }
	
  return true;
}


void OpalMediaPatch::Main()
{
  PTRACE(4, "Patch\tThread started for " << *this);
	
  bool asynchronous = OnStartMediaPatch();
  PTimeInterval lastTick;
  
  /* Note the RTP frame is outside loop so that a) it is more efficient
     for memory usage, the buffer is only ever increased and not allocated
     on the heap ever time, and b) the timestamp value embedded into the
     sourceFrame is needed for correct operation of the jitter buffer and
     silence frames. It is adjusted by DispatchFrame (really Sink::WriteFrame)
     each time and passed back in to source.Read() (and eventually the JB) so
     it knows where it is up to in extracting data from the JB. */
  //RTP_DataFrame sourceFrame(0);
  //sourceFrame.SetPayloadSize(source.GetDataSize());
#if 0
  RTP_DataFrame sourceFrame(source.GetDataSize());
#else
	//int datasize= (PIsDescendant(& source, OpalVideoMediaStream)?1920*1600 :(source.GetDataSize()  ));//if audio source see OpalRawMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
	int datasize= (PIsDescendant(& source, OpalVideoMediaStream)?1920*1600 :(source.GetDataSize()  ));//if audio source see OpalRawMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)

	RTP_DataFrame sourceFrame(0);
  sourceFrame.SetPayloadSize(datasize/*source.GetDataSize()*/);

#endif
  while (source.IsOpen()) {
#if 0
    sourceFrame.SetPayloadType(source.GetMediaFormat().GetPayloadType());
#endif
		if (source.IsPaused())
		{ PThread::Sleep(10);continue;}
    if (g_nDelayMicsec>0&&source.GetSessionID() == 0  
      && source.GetMediaFormat().GetMediaType()== OpalMediaType::Video() 
      )
    {
#define D_DELAY_PER_SEC 200
#define D_DELAY_PER_SEC_2 250
      long n = g_nDelayMicsec.operator long();
      n-= D_DELAY_PER_SEC_2;
      if (n> 10000)
        n%=10000;
      

      g_nDelayMicsec.SetValue(n);
      //if (n> 10000)
      //  PThread::Sleep(2000);
      //else
        PThread::Sleep(D_DELAY_PER_SEC);

    }

    // We do the following to make sure that the buffer size is large enough,
    // in case something in previous loop adjusted it
    //if ( )
    //sourceFrame.SetPayloadSize(0);//chenayuan

    if (!source.ReadPacket(sourceFrame)) {
      PTRACE(4, "Patch\tThread ended because source read failed");
      break;
    }
    if (sourceFrame.GetPayloadSize() <= 0 ){
      
      PThread::Sleep(1);
   
      continue;
    }
    inUse.StartRead();
    bool written = DispatchFrame(sourceFrame);
    inUse.EndRead();

    if (!written) {
      PTRACE(4, "Patch\tThread ended because all sink writes failed");
      break;
    }
    /* Don't starve the CPU if we have idle frames and the no source or
       destination is synchronous. Note that performing a Yield is not good
       enough, as the media patch threads are high priority and will consume
       all available CPU if allowed. Also just doing a sleep each time around
       the loop slows down video where you get clusters of packets thrown at
       us, want to clear them as quickly as possible out of the UDP OS buffers
       or we overflow and lose some. Best compromise is to every 100ms, sleep
       for 10ms so can not use more than about 90% of CPU. */
#if 1
    //if (asynchronous && PTimer::Tick() - lastTick > 100)
    {
      PThread::Sleep(1);
      lastTick = PTimer::Tick();
    }
#else
    if (asynchronous/* && PTimer::Tick() - lastTick > 100*/) {
      PThread::Sleep(5/*10*/);
      lastTick = PTimer::Tick();
    }else
		{
      PThread::Sleep(5/*10*/);
      lastTick = PTimer::Tick();
		}
#endif
  }

  source.OnStopMediaPatch(*this);

  PTRACE(4, "Patch\tThread ended for " << *this);
}


bool OpalMediaPatch::SetBypassPatch(OpalMediaPatch * patch)
{
  PTRACE(4, "Patch\tSetting media patch bypass to " << patch << " on " << *this);

  PWriteWaitAndSignal mutex(inUse);

  if (!PAssert(m_bypassFromPatch == NULL, PLogicError))
    return false; // Can't be both!

  if (m_bypassToPatch == patch)
    return true; // Already set

  if (m_bypassToPatch != NULL) {
    if (!PAssert(m_bypassToPatch->m_bypassFromPatch == this, PLogicError))
      return false;

    m_bypassToPatch->m_bypassFromPatch = NULL;
    m_bypassToPatch->m_bypassEnded.Signal();
  }

  if (patch != NULL) {
    if (!PAssert(patch->m_bypassFromPatch == NULL, PLogicError))
      return false;

    patch->m_bypassFromPatch = this;
  }

  m_bypassToPatch = patch;

#if OPAL_VIDEO
  m_bypassActive = m_bypassToPatch != NULL && !m_videoDecoder;
#else
  m_bypassActive = m_bypassToPatch != NULL;
#endif

  return true;
}


PBoolean OpalMediaPatch::PushFrame(RTP_DataFrame & frame)
{
  PReadWaitAndSignal mutex(inUse);
  return DispatchFrame(frame);
}


bool OpalMediaPatch::DispatchFrame(RTP_DataFrame & frame)
{
  if (m_bypassFromPatch != NULL) {
    PTRACE(3, "Patch\tMedia patch bypass started by " << m_bypassFromPatch << " on " << *this);
    inUse.EndRead();
    m_bypassEnded.Wait();
    PTRACE(4, "Patch\tMedia patch bypass ended on " << *this);
    inUse.StartRead();
    return true;
  }

	//bool bWrited=false
//#define C_OLD_LOGIC
#ifdef C_OLD_LOGIC
  FilterFrame(frame, source.GetMediaFormat());    
#endif
  bool written = false;
	RTP_DataFrameList  s_intermediateFrames ;
	OpalMediaFormat  PreSinkoutput;
	int i=0;
//#ifdef D_DELETE
//  std::list<PList<Sink>::iterator > lstNeedDelete;
  for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s,i++) {
#ifdef C_OLD_LOGIC
#else
		//we buffer the first of sinked packet, if yyaudio, check the RTP from with SinkMediaformat, if equaled then send it else skip
		if (source.GetSessionID() ==0)//我们自己的patch
		{
      if (i==1||i==0)		{
        //only audio to be filter, for video use recording
        //if ( source.GetMediaFormat().GetMediaType() == OpalMediaType::Audio() )		FilterFrame(frame, source.GetMediaFormat());    //循环到第一encode sink 才filter,避免多次filter
      }

			if (/*s == (sinks.begin()+1)*/i<=1)//第0个是 yuv420p->->yuv420p,第1个是 yuv420p-> mpeg4,第3次就不用编码了
			{
#ifdef _WIN32
        if (s->stream->GetMediaFormat().GetName() == YYCODE_CORE)
        {
					  if (i == 1)
					  {//for iVoice codec, we read a packet from encode pool
              RTP_DataFrame*  pTtmpEncode = new RTP_DataFrame(0);
              if (m_pSourceIVoiceStream){
                m_pSourceIVoiceStream->ReadPacket(*pTtmpEncode);
                if (pTtmpEncode->GetPayloadSize() >0){// had to get valid data
                  s_intermediateFrames.Append(pTtmpEncode);
                  //  PTRACE(5,"RTP_Session\t Patch:: sessionID" << 0 << ", timestamp=" << pTtmpEncode->GetTimestamp()); 
                  s->stream->WritePacket(*pTtmpEncode);
                  FilterFrame(*pTtmpEncode,s->stream->GetMediaFormat()/*YYCODE_CORE*/); //for audio record filter
                }else{
                  delete pTtmpEncode;
                  PThread::Sleep(1);
                }
              }
              else
              {
                PTRACE(3, "MediaPath\t Failed to call m_pSourceIVoiceStream." );
                return false;
              }
              //s->WriteFrame(frame);
						  //s_intermediateFrames = (s->intermediateFrames);
					  } 
					  written = true;

				  
        }else// prepare video encode
#endif
        {
          
				  if (s->WriteFrame(frame))
				  {
           
					  if (i == 1)
					  {
						  //if (s->stream)
						  //	sinkoutput = s->stream->GetMediaFormat(); 
             //for video record filter
						  s_intermediateFrames = (s->intermediateFrames);
              OpalMediaFormat mf= s->stream->GetMediaFormat();
              for (RTP_DataFrameList::iterator interFrame = s_intermediateFrames.begin(); interFrame != s_intermediateFrames.end(); ++interFrame) {
                 FilterFrame(*interFrame,mf); 
              }
					  }
					  written = true;

				  }else {
					  PTRACE(2, "Patch\tWritePacket failed");
					  s_intermediateFrames.RemoveAll();
            bool bNULL = (s->stream!=NULL);
            inUse.EndRead();
            if (bNULL/*s->stream!=NULL*/)
              s->stream->Close();
            inUse.StartRead(); 
					  written = true;//session=0  返回true
            break;
				  }
        }
 
			}else
			{//ENCODE::for sink stream index >1 we try to send the packet to peer from previous transcodeer(sink stream index =1)
				if (PreSinkoutput.GetName() == s->stream->GetMediaFormat().GetName() )
				{
					//直接发送第一次编码的结果
					for (RTP_DataFrameList::iterator interFrame = s_intermediateFrames.begin(); interFrame != s_intermediateFrames.end(); ++interFrame) {
						if (s->secondaryCodec == NULL) {
							if (!s->stream->WritePacket(*interFrame))
								return (s->writeSuccessful = false);
              //else  write OK
						}
					}
					written = true;
				}
				else
				{// ENCODE::if difflent history RTP ,we continue to write frame for transcode RTP 
#ifdef _WIN32
          if (source.GetSessionID() ==1 &&source.GetMediaFormat().GetName() == YYCODE_CORE){
            s->stream->WritePacket(frame);	written = true;
          }else{
            if (s->WriteFrame(frame)){
			       written = true;
             //FilterFrame(frame, s->stream->GetMediaFormat() );//for video recording () do not call here
            }
          }
#else
					s->WriteFrame(frame);
					written = true;
#endif

				}
			}
		}else{//decode audio and video here
      //session id is not equal zero
#ifdef _WIN32
    bool bResult = false;
    if (source.GetSessionID() ==1 &&source.GetMediaFormat().GetName() == YYCODE_CORE){
      bResult = s->stream->WritePacket(frame);written = true;
      FilterFrame(frame, source.GetMediaFormat()/*YYCODE_CORE*/);//for audio recording ()
    }else{
      if ((bResult=s->WriteFrame(frame))== true ){
			  written = true;
        FilterFrame(frame, source.GetMediaFormat() );//for video recording 
      }
    }
    if (bResult ==false) {
      PTRACE(2, "Patch\tWritePacket failed");
       bool bNULL = s->stream!=NULL;
      inUse.EndRead();
      if (bNULL)
        s->stream->Close();
      inUse.StartRead();
      break;
    }
#else
		if (s->WriteFrame(frame))
			written = true;
		else {
        bool bNULL =  (s->stream!=NULL);
        inUse.EndRead();
        if (bNULL)
          s->stream->Close();
        inUse.StartRead();
				PTRACE(2, "Patch\tWritePacket failed");
        break;
			}
#endif
		}
#endif
  }

 // return written;

  if (m_bypassActive) {
    written = false;
    for (PList<Sink>::iterator s = m_bypassToPatch->sinks.begin(); s != m_bypassToPatch->sinks.end(); ++s) {
      if (s->stream->WritePacket(frame))
        written = true;
    }
  }

  return written;
}


bool OpalMediaPatch::Sink::UpdateMediaFormat(const OpalMediaFormat & mediaFormat)
{
  bool ok;

  if (primaryCodec == NULL)
    ok = stream->UpdateMediaFormat(mediaFormat, true);
  else if (secondaryCodec != NULL && secondaryCodec->GetOutputFormat() == mediaFormat)
    ok = secondaryCodec->UpdateMediaFormats(OpalMediaFormat(), mediaFormat) &&
         stream->UpdateMediaFormat(secondaryCodec->GetOutputFormat(), true);
  else if (primaryCodec->GetOutputFormat() == mediaFormat)
    ok = primaryCodec->UpdateMediaFormats(OpalMediaFormat(), mediaFormat) &&
         stream->UpdateMediaFormat(primaryCodec->GetOutputFormat(), true);
  else
    ok = primaryCodec->UpdateMediaFormats(mediaFormat, OpalMediaFormat()) &&
         stream->UpdateMediaFormat(primaryCodec->GetInputFormat(), true);

#if OPAL_VIDEO
    SetRateControlParameters(stream->GetMediaFormat());
#endif

  PTRACE(3, "Patch\tUpdated Sink: format=" << mediaFormat << " ok=" << ok);
  return ok;
}


bool OpalMediaPatch::Sink::ExecuteCommand(const OpalMediaCommand & command)
{
  PBoolean atLeastOne = PFalse;

  if (secondaryCodec != NULL)
    atLeastOne = secondaryCodec->ExecuteCommand(command) || atLeastOne;

  if (primaryCodec != NULL)
    atLeastOne = primaryCodec->ExecuteCommand(command) || atLeastOne;

  return atLeastOne;
}


void OpalMediaPatch::Sink::SetCommandNotifier(const PNotifier & notifier)
{
  if (secondaryCodec != NULL)
    secondaryCodec->SetCommandNotifier(notifier);

  if (primaryCodec != NULL)
    primaryCodec->SetCommandNotifier(notifier);
}


static bool CannotTranscodeFrame(const OpalTranscoder & codec, RTP_DataFrame & frame)
{
  RTP_DataFrame::PayloadTypes pt = frame.GetPayloadType();

  if (!codec.AcceptComfortNoise()) {
    if (pt == RTP_DataFrame::CN || pt == RTP_DataFrame::Cisco_CN) {
      PTRACE(4, "Patch\tRemoving comfort noise frame with payload type " << pt);
      frame.SetPayloadSize(0);   // remove the payload because the transcoder has indicated it won't understand it
      frame.SetPayloadType(codec.GetPayloadType(true));
      return true;
    }
  }

  if ((pt != codec.GetPayloadType(true)) && !codec.AcceptOtherPayloads()) {
    PTRACE(4, "Patch\tRemoving frame with mismatched payload type " << pt << " - should be " << codec.GetPayloadType(true));
    frame.SetPayloadSize(0);   // remove the payload because the transcoder has indicated it won't understand it
    frame.SetPayloadType(codec.GetPayloadType(true)); // Reset pt so if get silence frames from jitter buffer, they don't cause errors
    return true;
  }

  if (!codec.AcceptEmptyPayload() && frame.GetPayloadSize() == 0) {
    frame.SetPayloadType(codec.GetPayloadType(false));
    return true;
  }

  return false;
}


#if OPAL_VIDEO
void OpalMediaPatch::Sink::SetRateControlParameters(const OpalMediaFormat & mediaFormat)
{
  if ((mediaFormat.GetMediaType() == OpalMediaType::Video()) && mediaFormat != OpalYUV420P) {
    rateController = NULL;
    PString rc = mediaFormat.GetOptionString(OpalVideoFormat::RateControllerOption());
    if (rc.IsEmpty() && mediaFormat.GetOptionBoolean(OpalVideoFormat::RateControlEnableOption()))
      rc = "Standard";
    if (!rc.IsEmpty()) {
      rateController = PFactory<OpalVideoRateController>::CreateInstance(rc);
      if (rateController != NULL) {   
        PTRACE(3, "Patch\tCreated " << rc << " rate controller");
      }
      else {
        PTRACE(3, "Patch\tCould not create " << rc << " rate controller");
      }
    }
  }

  if (rateController != NULL) 
    rateController->Open(mediaFormat);
}


bool OpalMediaPatch::Sink::RateControlExceeded(bool & forceIFrame)
{
  if ((rateController == NULL) || !rateController->SkipFrame(forceIFrame)) 
    return false;

  PTRACE(4, "Patch\tRate controller skipping frame.");
  return true;
}

#endif

bool OpalMediaPatch::Sink::WriteFrame(RTP_DataFrame & sourceFrame)
{
 // if (!writeSuccessful)
 //   return false;
  
  if (stream->IsPaused())
    return true;
  //chenyuan

#if OPAL_VIDEO
  if (rateController != NULL) {
    bool forceIFrame = false;
    bool s = RateControlExceeded(forceIFrame);
    if (forceIFrame)
      stream->ExecuteCommand(OpalVideoUpdatePicture2(sourceFrame.GetSequenceNumber(), sourceFrame.GetTimestamp()));
    if (s) {
      if (secondaryCodec == NULL) {
        bool wasIFrame = false;
        if (rateController->Pop(intermediateFrames, wasIFrame, false)) {
        PTRACE(3, "RC returned " << intermediateFrames.GetSize() << " packets");
          for (RTP_DataFrameList::iterator interFrame = intermediateFrames.begin(); interFrame != intermediateFrames.end(); ++interFrame) {
            patch.FilterFrame(*interFrame, primaryCodec->GetOutputFormat());
            if (!stream->WritePacket(*interFrame))
              return (writeSuccessful = false);
            sourceFrame.SetTimestamp(interFrame->GetTimestamp());
            continue;
          }
          intermediateFrames.RemoveAll();
        }
      }
      return true;
    }
  }
#endif

  if (primaryCodec == NULL)
    return (writeSuccessful = stream->WritePacket(sourceFrame));

  if (CannotTranscodeFrame(*primaryCodec, sourceFrame))
    return (writeSuccessful = stream->WritePacket(sourceFrame));

  if (!primaryCodec->ConvertFrames(sourceFrame, intermediateFrames)) {
    PTRACE(1, "Patch\tMedia conversion (primary) failed, convert " << primaryCodec->GetInputFormat().GetName() << " to "<< primaryCodec->GetOutputFormat().GetName());
    return false;
  }

#if OPAL_VIDEO
  if (!patch.m_bypassActive && patch.m_videoDecoder && patch.m_bypassToPatch != NULL &&
                dynamic_cast<OpalVideoTranscoder *>(primaryCodec)->WasLastFrameIFrame()) {
    PTRACE(3, "Patch\tActivating video patch bypass to " << patch.m_bypassToPatch << " on " << patch);
    patch.m_bypassActive = true;
  }

  if (secondaryCodec == NULL && rateController != NULL) {
    PTRACE(4, "Patch\tPushing " << intermediateFrames.GetSize() << " packet into RC");
    rateController->Push(intermediateFrames, ((OpalVideoTranscoder *)primaryCodec)->WasLastFrameIFrame());
    bool wasIFrame = false;
    if (rateController->Pop(intermediateFrames, wasIFrame, false)) {
      PTRACE(4, "Patch\tPulled " << intermediateFrames.GetSize() << " frames from RC");
      for (RTP_DataFrameList::iterator interFrame = intermediateFrames.begin(); interFrame != intermediateFrames.end(); ++interFrame) {
        patch.FilterFrame(*interFrame, primaryCodec->GetOutputFormat());
        if (!stream->WritePacket(*interFrame))
          return (writeSuccessful = false);
        sourceFrame.SetTimestamp(interFrame->GetTimestamp());
        continue;
      }
      intermediateFrames.RemoveAll();
    }
  }
  else 
#endif
  for (RTP_DataFrameList::iterator interFrame = intermediateFrames.begin(); interFrame != intermediateFrames.end(); ++interFrame) {
    //patch.FilterFrame(*interFrame, primaryCodec->GetOutputFormat()); for change the time and seq,we call it after write OK   
 
    if (secondaryCodec == NULL) {
      if (!stream->WritePacket(*interFrame))
        return (writeSuccessful = false);
      sourceFrame.SetTimestamp(interFrame->GetTimestamp());
      continue;
    }
    patch.FilterFrame(*interFrame, primaryCodec->GetOutputFormat());
    if (CannotTranscodeFrame(*secondaryCodec, *interFrame)) {
      if (!stream->WritePacket(*interFrame))
        return (writeSuccessful = false);
      continue;
    }

    if (!secondaryCodec->ConvertFrames(*interFrame, finalFrames)) {
      PTRACE(1, "Patch\tMedia conversion (secondary) failed");
      return false;
    }

    for (RTP_DataFrameList::iterator finalFrame = finalFrames.begin(); finalFrame != finalFrames.end(); ++finalFrame) {
      patch.FilterFrame(*finalFrame, secondaryCodec->GetOutputFormat());
      if (!stream->WritePacket(*finalFrame))
        return (writeSuccessful = false);
      sourceFrame.SetTimestamp(finalFrame->GetTimestamp());
    }
  }

#if OPAL_VIDEO
  //if (rcEnabled)
  //  rateController.AddFrame(totalPayloadSize, frameCount);
#endif

  return true;
}



bool OpalMediaPatch::Sink::WriteFrameEx(RTP_DataFrame & sourceFrame)
{
 // if (!writeSuccessful)
 //   return false;
  
  if (stream->IsPaused())
    return true;
  //chenyuan

#if OPAL_VIDEO
  if (rateController != NULL) {
    bool forceIFrame = false;
    bool s = RateControlExceeded(forceIFrame);
    if (forceIFrame)
      stream->ExecuteCommand(OpalVideoUpdatePicture2(sourceFrame.GetSequenceNumber(), sourceFrame.GetTimestamp()));
    if (s) {
      if (secondaryCodec == NULL) {
        bool wasIFrame = false;
        if (rateController->Pop(intermediateFrames, wasIFrame, false)) {
        PTRACE(3, "RC returned " << intermediateFrames.GetSize() << " packets");
          for (RTP_DataFrameList::iterator interFrame = intermediateFrames.begin(); interFrame != intermediateFrames.end(); ++interFrame) {
            patch.FilterFrame(*interFrame, primaryCodec->GetOutputFormat());
            if (!stream->WritePacket(*interFrame))
              return (writeSuccessful = false);
            sourceFrame.SetTimestamp(interFrame->GetTimestamp());
            continue;
          }
          intermediateFrames.RemoveAll();
        }
      }
      return true;
    }
  }
#endif

  if (primaryCodec == NULL)
    return (writeSuccessful = stream->WritePacket(sourceFrame));

  //if (CannotTranscodeFrame(*primaryCodec, sourceFrame))
  //  return (writeSuccessful = stream->WritePacket(sourceFrame));

  if (!primaryCodec->ConvertFrames(sourceFrame, intermediateFrames)) {
    PTRACE(1, "Patch\tMedia conversion (primary) failed, convert " << primaryCodec->GetInputFormat().GetName() << " to "<< primaryCodec->GetOutputFormat().GetName());
    return false;
  }

#if OPAL_VIDEO
  if (!patch.m_bypassActive && patch.m_videoDecoder && patch.m_bypassToPatch != NULL &&
                dynamic_cast<OpalVideoTranscoder *>(primaryCodec)->WasLastFrameIFrame()) {
    PTRACE(3, "Patch\tActivating video patch bypass to " << patch.m_bypassToPatch << " on " << patch);
    patch.m_bypassActive = true;
  }

  if (secondaryCodec == NULL && rateController != NULL) {
    PTRACE(4, "Patch\tPushing " << intermediateFrames.GetSize() << " packet into RC");
    rateController->Push(intermediateFrames, ((OpalVideoTranscoder *)primaryCodec)->WasLastFrameIFrame());
    bool wasIFrame = false;
    if (rateController->Pop(intermediateFrames, wasIFrame, false)) {
      PTRACE(4, "Patch\tPulled " << intermediateFrames.GetSize() << " frames from RC");
      for (RTP_DataFrameList::iterator interFrame = intermediateFrames.begin(); interFrame != intermediateFrames.end(); ++interFrame) {
        patch.FilterFrame(*interFrame, primaryCodec->GetOutputFormat());
        if (!stream->WritePacket(*interFrame))
          return (writeSuccessful = false);
        sourceFrame.SetTimestamp(interFrame->GetTimestamp());
        continue;
      }
      intermediateFrames.RemoveAll();
    }
  }
  else 
#endif
  for (RTP_DataFrameList::iterator interFrame = intermediateFrames.begin(); interFrame != intermediateFrames.end(); ++interFrame) {
    patch.FilterFrame(*interFrame, primaryCodec->GetOutputFormat());

    if (secondaryCodec == NULL) {
      if (!stream->WritePacket(*interFrame))
        return (writeSuccessful = false);
      sourceFrame.SetTimestamp(interFrame->GetTimestamp());
      continue;
    }

    if (CannotTranscodeFrame(*secondaryCodec, *interFrame)) {
      if (!stream->WritePacket(*interFrame))
        return (writeSuccessful = false);
      continue;
    }

    if (!secondaryCodec->ConvertFrames(*interFrame, finalFrames)) {
      PTRACE(1, "Patch\tMedia conversion (secondary) failed");
      return false;
    }

    for (RTP_DataFrameList::iterator finalFrame = finalFrames.begin(); finalFrame != finalFrames.end(); ++finalFrame) {
      patch.FilterFrame(*finalFrame, secondaryCodec->GetOutputFormat());
      if (!stream->WritePacket(*finalFrame))
        return (writeSuccessful = false);
      sourceFrame.SetTimestamp(finalFrame->GetTimestamp());
    }
  }

#if OPAL_VIDEO
  //if (rcEnabled)
  //  rateController.AddFrame(totalPayloadSize, frameCount);
#endif

  return true;
}


OpalMediaPatch::Thread::Thread(OpalMediaPatch & p)
  : PThread(65536,  //16*4kpage size
            NoAutoDeleteThread,
            HighPriority,
            "Media Patch"),
    patch(p)
{
}


/////////////////////////////////////////////////////////////////////////////


OpalPassiveMediaPatch::OpalPassiveMediaPatch(OpalMediaStream & source)
  : OpalMediaPatch(source)
{
}


void OpalPassiveMediaPatch::Start()
{
  OnStartMediaPatch();
}

