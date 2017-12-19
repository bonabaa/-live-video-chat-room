/*
 * maccoreaudio.cxx
 *
 * Copyright (c) 2004 Network for Educational Technology ETH
 *
 * Written by Hannes Friederich, Andreas Fenkart.
 * Based on work of Shawn Pai-Hsiang Hsiao
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
 */

#pragma implementation "maccoreaudioex.h" 

#include <ptlib/unix/ptlib/maccoreaudioex.h>
#include <iostream>  // used for Volume Listener
//#include "CAXException.h"

#define D_CORE_AUDIO 1
///////////////////////////////////////////////////////////////////////////////
#define kBufferDurationSeconds .5

PWaveBuffer::PWaveBuffer(PINDEX sz)
: PBYTEArray(sz)
{
	m_dwBufferLength=0;
}


PWaveBuffer::~PWaveBuffer()
{
	//Release();
}




void PWaveBuffer::PrepareCommon(PINDEX count)
{
	m_dwBufferLength = count;
  
}

///
/*
 * circularbuffer.inl
 *
 * Copyright (c) 2004 Network for Educational Technology ETH
 *
 * Written by Hannes Friederich, Andreas Fenkart.
 * Based on work of Shawn Pai-Hsiang Hsiao
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
 * maccoreaudio.h
 *
 */


#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#ifndef MIN
#define MIN(a,b) (a < b ? a : b)
#endif


//#define PTRACE_CIRC(level, str) PTRACE(level, str) 
#define PTRACE_CIRC(level, str) 

/**
 * Simple circular buffer for one producer and consumer with a head and a 
 * tail that chase each other. The capacity is 1 byte bigger than necessary 
 * due to a sentinel, to tell apart full from empty by the following equations:
 *
 * full  := head_next == tail
 * empty := head      == tail
 *
 * Inspired by CircularBuffer from beaudio.
 * We need a lock when updating the tail index, due to the overwrite mechanism
 * in case of buffer overflow. The head index needs no locking because 
 * overwrite does not make sense in case of buffer underrun. 
 *
 * Keep in mind that this buffer does no know about frames or packets, it's up
 * to you to make sure you get the right number of bytes. In doubt use size() 
 * to compute how many frames/packets it is save to drain/fill.
 *
 */

class CircularBuffer
{
public:
  
  explicit CircularBuffer(PINDEX len)
  :   capacity(len + 1),  /* plus sentinel */ head(0), tail(0)
  {
    buffer = (char *)malloc(capacity*sizeof(char));
    if(!buffer) {
      PTRACE(1, "Could not allocate circular buffer");
      init = false;
    } else {
      init = true;
    }
    
    /*
     * Mutex to block write thread when buffer is full
     */
    int rval;
    rval = pthread_mutex_init(&mutex, NULL);
    if (rval) {
      PTRACE(1, __func__ << " can not init mutex");
      init = false;
      //return PFalse;
    }
    rval = pthread_cond_init(&cond, NULL);
    if (rval) {
      PTRACE(1, __func__ << " can not init cond");
      init = false;
      //return PFalse;
    }
  }
  
  ~CircularBuffer()
  {
    pthread_cond_signal(&cond);
    if(buffer){
      std::free(buffer);
      buffer = NULL;
    }
    
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
  }
  
  
  PBoolean Full()
  {
    /* head + 1 == tail */
    return head_next() == tail;
  }
  
  PBoolean Empty()
  {
    return head == tail;
  }
  
  PINDEX size(){
    /* sentinel is outside of occupied area */
    return (head < tail ) ? head + capacity - tail : head - tail; 
  }
  
  PINDEX free(){
    return (capacity - size() -1 /*sentinel */);
  }
  
  /** 
   * Fill inserts data into the circular buffer. 
   * Remind that lock and overwrite are mutually exclusive. If you set lock,
   * overwrite will be ignored 
   */
  PINDEX Fill(const char* inbuf, PINDEX len, Boolean lock = true, 
              Boolean overwrite = false);
  
  
  /** See also Fill */
  PINDEX Drain(char* outbuf, PINDEX len, Boolean lock = true);
  
private:
  PINDEX head_next()
  {
    return (head + 1 == capacity) ? 0 : (head + 1);
    //return (head + 1 % capacity);
  }
  
  void increment_index(PINDEX &index, PINDEX inc)
  {
    index += inc;
    index %= capacity;
  }
  
  void increment_head(PINDEX inc)
  {
    increment_index(head, inc);
  }     
  
  void increment_tail(PINDEX inc)
  {
    increment_index(tail, inc);
  }
  
  
  
private:
  char* buffer;
  const PINDEX capacity;
  PINDEX head, tail;
  Boolean init;
  
  pthread_mutex_t mutex;
  pthread_cond_t cond;
};


int CircularBuffer::Fill(const char *inbuf, PINDEX len, 
                         Boolean lock, Boolean overwrite)
{
  int done = 0, todo = 0;
  
  PTRACE_CIRC(1, "Fill " << len << " bytes, contains " << size() << "head="  <<head <<",tail=" <<tail <<",capacity=" <<capacity);
  
  if(inbuf== NULL){
    PTRACE(1, __func__ << "Input buffer is empty");
    return 0;
  }
  
  while(done != len && !Full()) {
    if(head >= tail) {
      // head unwrapped, fill from head till end of buffer
      if(tail == 0) /* buffer[capacity] == sentinel */
        todo = MIN(capacity -1 /*sentinel*/ - head, len - done);
      else
        todo = MIN(capacity - head, len - done);
    } else {
      // fill from head till tail 
      todo = MIN(tail -1 /*sentinel*/ - head, len - done);
    }
    memcpy(buffer + head, inbuf + done, todo);
    done += todo;
    increment_head(todo);
    PTRACE_CIRC(1, todo <<" copied " << done << " from input buffer,head="  <<head <<",tail=" <<tail
                << " available " << size());
  }
  
  // What to do if buffer is full and more bytes
  // need to be copied ?  
  if(Full() && done != len && (overwrite || lock)) {
    PTRACE_CIRC(1, __func__ << "Circular buffer is full, while Fill " 
                << len); 
    if(lock) {
      pthread_mutex_lock(&mutex);
      PTRACE_CIRC(1, "Fill going to sleep");
      pthread_cond_wait(&cond, &mutex);
      // pthread_cond_timedwait
      PTRACE_CIRC(1, "Fill woke up");
      pthread_mutex_unlock(&mutex);
    } else if(overwrite){
      pthread_mutex_lock(&mutex);
      if(Full()){
        tail += len - done; // also shifts sentinel
        tail %= capacity; // wrap around
      }
      pthread_mutex_unlock(&mutex);
    }
    // try again
    done += Fill(inbuf + done, len - done, lock, overwrite);
  }
  
  // wake up read thread if necessary
  if(!Empty())
    pthread_cond_signal(&cond);
  
  
  PTRACE_CIRC(1, __func__ << " " << len << " bytes, stored " << done 
              << " available " << size());
  return done;
}


PINDEX CircularBuffer::Drain(char *outbuf, PINDEX len, Boolean lock) {
  PINDEX done = 0, todo = 0;
  
  PTRACE_CIRC(6, __func__ << " " << len << " bytes, available " << size() );
  
  if(outbuf == NULL){
    PTRACE(1, __func__ << " Out buffer is NULL"); 
    return PINDEX(0);
  }
  
  /* protect agains buffer corruption when write thread
   * is overwriting */
  pthread_mutex_lock(&mutex);
  PTRACE(6, "aquired lock");
  
  while(done != len && !Empty()) {
    if(head >= tail) {
      // head unwrapped 
      todo = MIN(head - tail, len - done);
    } else {
      // head wrapped 
      todo = MIN(capacity - tail, len - done);
    }        
    memcpy(outbuf + done, buffer + tail, todo);
    done += todo;
    increment_tail(todo);
    PTRACE_CIRC(1, __func__ << " for " << len <<" bytes, copied " << done 
                << " to output buffer,  available in buffer " << size());
  }
  
  
  if(done != len && (lock)) /* && Empty() */ {
    PTRACE(3, "Buffer underrun for Drain " << len << " bytes");
  }
  
  // what to do if not as many bytes are available then
  // requested ?
  if(done != len && (lock)) /* && Empty() */ {
    if (lock){
      pthread_cond_wait(&cond, &mutex);
      PTRACE_CIRC(2, "Read thread woke up, calling Drain");
      pthread_mutex_unlock(&mutex); // race with write thread...
      done += Drain(outbuf + done, len - done, lock);
      PTRACE_CIRC(2, "Returned from recursive");
    }
  }
  
  pthread_mutex_unlock(&mutex);
  
  if(!Full())
    pthread_cond_signal(&cond);
  
  PTRACE_CIRC(2, "End Drain " << len << " bytes, fetched " << done
              << " buffer fill " << size() );
  return done;
  
}
#endif

/////////////////////////////////////////









///////////////////////////////////////////////////////////////////////////////

namespace PWLibStupidLinkerHacks
{
	int loadCoreAudioStuff;
}
///////////////////////////////////////////////////////////////////////////////


void __handle_input_buffer (void *userdata, AudioQueueRef queue, AudioQueueBufferRef bufferQu, const AudioTimeStamp *start_time, UInt32 number_packet_descriptions, const AudioStreamPacketDescription *packet_descriptions ) 
{
  OSStatus ret;
  int nReturned = bufferQu->mAudioDataByteSize;
	PSoundChannelCoreAudio* producer = (PSoundChannelCoreAudio*)userdata;
	{
    if (!producer->opened) {
      PTRACE(5, "PSoundChannelCoreAudio\t __handle_input_buffer audio device closed ,ignore it ,bufferQu->mAudioDataByteSize=" << bufferQu->mAudioDataByteSize); 
      return;
    }
		// Alert the session that there is new data to send
    //PTRACE(5, "PSoundChannelCoreAudio\t __handle_input_buffer,handle a audio data begin bufferQu->mAudioDataByteSize=" << bufferQu->mAudioDataByteSize ); 
#ifdef D_WAVE_BUF
		PWaitAndSignal mutex(producer->bufferMutex);   
    int nSize = bufferQu->mAudioDataByteSize;
    int offset =0;
    while (offset < bufferQu->mAudioDataByteSize) {
      PWaveBuffer & bufoout = producer->buffers[producer->bufferIndex];
      int nCopysize = bufoout.GetSize();
      if (nCopysize <= nSize )
      {
        
      }else {
        nCopysize = nSize;
      }
      
		  memcpy(bufoout.GetPointer(),(const char*)bufferQu->mAudioData+offset, nCopysize/*bufferQu->mAudioDataByteSize*/);
      offset+= nCopysize;
      nSize-=nCopysize;
      producer->bufferIndex++;
    }
#else
		//PWaitAndSignal mutex(producer->bufferMutex);   
    nReturned = producer->m_pAudionBuf->Fill((const char*)bufferQu->mAudioData, bufferQu->mAudioDataByteSize, false, true);
#endif
		// Re-enqueue the buffer
		if ((ret = AudioQueueEnqueueBuffer(producer->queue, bufferQu, 0, NULL))!=0)
      PTRACE(5, "PSoundChannelCoreAudio __handle_input_buffer\t end,failed call AudioQueueEnqueueBuffer bufferQu->mAudioDataByteSize=" << nReturned); 
    else
    {
      if (producer->bCanTrace)
      { PTRACE(5, "PSoundChannelCoreAudio __handle_input_buffer\t end bufferQu->mAudioDataByteSize=" <<nReturned);
        producer->bCanTrace = false;
      }
    }
	}
	//producer->m_syncThread.Signal();
}

void __handle_output_buffer(void *userdata, AudioQueueRef queue, AudioQueueBufferRef bufferQu)
{
  int nReturned=0;
  OSStatus ret;
	//void *data;
  PSoundChannelCoreAudio* consumer = (PSoundChannelCoreAudio*)userdata;
	
  if (!consumer->opened) {
    PTRACE(5, "PSoundChannelCoreAudio\t __handle_output_buffer audio device closed ,ignore it ,bufferQu->mAudioDataByteSize=" << bufferQu->mAudioDataByteSize); 
    return;
  }
  PTRACE(5, "PSoundChannelCoreAudio\t __handle_output_buffer Preparing output the data to audio device bufferQu->mAudioDataByteSize=" << bufferQu->mAudioDataByteSize); 
  //return ;
#ifdef D_WAVE_BUF
	{
    // If we can get audio to play, then copy in the buffer
    PWaitAndSignal mutex(consumer->bufferMutex);   
		PWaveBuffer & buffer = consumer->buffers[consumer->bufferIndex];
		memcpy(bufferQu->mAudioData, buffer.GetPointer(), buffer.m_dwBufferLength);
		memset(buffer.GetPointer(), 0, buffer.m_dwBufferLength);
    PTRACE(5, "__handle_output_buffer\t buffer.m_dwBufferLength=" << buffer.m_dwBufferLength); 
		//free(data);
	} 
  consumer->bufferIndex = (consumer->bufferIndex+1)% consumer->buffers.GetSize();
  //consumer->AddlastWriteCount(count);
#else
  {
    //consumer->m_syncThread.Wait();
    //PWaitAndSignal mutex(consumer->bufferMutex);   
    nReturned = consumer->m_pAudionBuf->Drain((char*)bufferQu->mAudioData, bufferQu->mAudioDataBytesCapacity, true);
    bufferQu->mAudioDataByteSize = nReturned;
  }
#endif
 
  // Re-enqueue the buffer
  if ((ret = AudioQueueEnqueueBuffer(consumer->queue, bufferQu, 0, NULL))!=0)
    PTRACE(5, "PSoundChannelCoreAudio\t __handle_output_buffer end,failed call AudioQueueEnqueueBuffer bufferQu->mAudioDataByteSize=" << nReturned); 
  else
  {
    if (consumer->bCanTrace)
    {
      PTRACE(5, "PSoundChannelCoreAudio\t __handle_output_buffer end bufferQu->mAudioDataByteSize=" <<nReturned); 
      consumer->bCanTrace = false;
    }
  }//if (ret = AudioQueueEnqueueBuffer(consumer->queue, bufferQu, 0, NULL);

}
#define D_packetperbuffer 960
#define GETDIR PString(direction == PSoundChannel::Recorder? "Recorder" :"Player") 

PSoundChannelCoreAudio::PSoundChannelCoreAudio()
:m_syncThread(0,20)
{
  Construct();
}


PSoundChannelCoreAudio::PSoundChannelCoreAudio(const PString & device,
                                               Directions dir,
                                               unsigned numChannels,
                                               unsigned sampleRate,
                                               unsigned bitsPerSample)
:m_syncThread(0,20)
{
  bCanTrace = true;
  Construct();
  Open(device, dir, numChannels, sampleRate, bitsPerSample);
}


void PSoundChannelCoreAudio::Construct()
{
  OSStatus ok =  0;//AudioSessionInitialize(NULL,NULL,NULL,NULL);
  //if (ok)
  //  PTRACE(5, "PSoundChannelCoreAudio\t failed to call AudioSessionInitialize." << ok);
  //else
  //  PTRACE(5, "PSoundChannelCoreAudio\t succeed to call AudioSessionInitialize." );
  
  opened = false;
  direction = Player;
  //waveFormat.SetFormat(1, 8000, 16);
  m_numChannels =1;
  m_sampleRate = 8000;
  m_bitsPerSample  = 16;
  
  SetFormat(1, 8000, 16);
  buffers_GetSize =0;
#ifdef D_WAVE_BUF
  bufferByteOffset = P_MAX_INDEX;
  bufferIndex = 0;
  buffer_size =0;
#endif
  for(int i=0;  i< sizeof(m_buffers)/ sizeof(AudioQueueBufferRef); i++){
    m_buffers[i]=NULL;     
  }
  PTRACE(5, "PSoundChannelCoreAudio\tCreated core audio");
  
  //SetBuffers(32768, 3);
  //this->queue = nil;
}


PSoundChannelCoreAudio::~PSoundChannelCoreAudio()
{
#ifdef D_WAVE_BUF
  int count = buffers.GetSize();
#else
  int count = buffers_GetSize ;
#endif
  for(int i=0; i<count && i<sizeof(m_buffers) /sizeof(AudioQueueBufferRef); i++){
    if (m_buffers[i]!=NULL)
    {
      AudioQueueFreeBuffer( queue,  m_buffers[i]);m_buffers[i] =NULL;
    }
  }
  /*for (int i=0; i< count; i++) {
   if (buffers.GetAt(i)!=NULL)
   delete buffers.GetAt(i);
   }*/
  Close();
  
  //#ifndef D_CORE_AUDIO
  //if (queue->isValid())
  //AudioQueueDispose( queue, true);
  //#endif		
  PTRACE(5, "PSoundChannelCoreAudio\t Destroy core audio end, buffer_size ="  << buffer_size <<",buffer_count="<< count);
  
}


PString PSoundChannelCoreAudio::GetName() const
{
  return deviceName;
}


static bool GetWaveOutDeviceName(UINT id, PString & name)
{
  
  return true;
}


static bool GetWaveInDeviceName(UINT id, PString & name)
{
  name=D_IPHONE;
  return true;
}


PStringArray PSoundChannelCoreAudio::GetDeviceNames(Directions dir)
{
  PStringArray devices;
  /*
   UINT numDevs;
   UINT id = WAVE_MAPPER;
   
   switch (dir) {
   case Player :
   numDevs = waveOutGetNumDevs();
   do {
   PCaselessString dev;
   if (GetWaveOutDeviceName(id, dev))
   devices.AppendString(dev);
   } while (++id < numDevs);
   break;
   
   case Recorder :
   numDevs = waveInGetNumDevs();
   do {
   PCaselessString dev;
   if (GetWaveInDeviceName(id, dev))
   devices.AppendString(dev);
   } while (++id < numDevs);
   break;
   }
   */
  devices.AppendString(D_IPHONE);
  return devices;
}


PBoolean PSoundChannelCoreAudio::GetDeviceID(const PString & device, Directions dir, unsigned& id)
{
  id =0;
  PINDEX offset = device.Find(PDevicePluginServiceDescriptor::SeparatorChar);
  if (offset == P_MAX_INDEX)
    offset = 0;
  else
    offset++;
  
  if (device[offset] == '#') {
    id = device.Mid(offset+1).AsUnsigned();
    switch (dir) {
      case Player :
        //if (id < waveOutGetNumDevs())
        //  GetWaveOutDeviceName(id, deviceName);
        break;
        
      case Recorder :
        //if (id < waveInGetNumDevs())
        //  GetWaveInDeviceName(id, deviceName);
        break;
    }
  }
  else {
    //id = WAVE_MAPPER;
    UINT numDevs;
    switch (dir) {
      case Player :
        //numDevs = waveOutGetNumDevs();
        //do {
        //  PCaselessString str;
        //  if (GetWaveOutDeviceName(id, str) && str == device.Mid(offset)) {
        //    deviceName = str;
        //    break;
        //  }
        //} while (++id < numDevs);
        break;
        
      case Recorder :
        //numDevs = waveInGetNumDevs();
        //do {
        //  PCaselessString str;
        //  if (GetWaveInDeviceName(id, str) && str == device.Mid(offset)) {
        //    deviceName = str;
        //    break;
        //  }
        //} while (++id < numDevs);
        break;
    }
  }
  
  //if (deviceName.IsEmpty())
  //  return SetErrorValues(NotFound, MMSYSERR_BADDEVICEID|PWIN32ErrorFlag);
  
  return PTrue;
}

PBoolean PSoundChannelCoreAudio::Open(const PString & device,
                                      Directions dir,
                                      unsigned numChannels,
                                      unsigned sampleRate,
                                      unsigned bitsPerSample)
{
  Close();
  unsigned id = 0;
  
  if( !GetDeviceID(device, dir, id) )
    return PFalse;
  m_numChannels = numChannels;
  m_sampleRate = sampleRate;
  m_bitsPerSample = bitsPerSample;
  //waveFormat.SetFormat(numChannels, sampleRate, bitsPerSample);
  m_pAudionBuf = new CircularBuffer(m_sampleRate*6);
  direction = dir;
  return OpenDevice(id);
}


PBoolean PSoundChannelCoreAudio::OpenDevice(unsigned id)
{
#ifndef D_CORE_AUDIO
  return PTrue;
#endif
  Close();
  
  PWaitAndSignal mutex(bufferMutex);
  
  OSStatus ret;
  //int i;
  //PString strDir = GETDIR;
  /* codec should have ptime */
  this->buffer_size =640;
  
  if (direction == PSoundChannel::Recorder )
  {
    /* codec should have ptime */
    
    
    // Set audio category
    UInt32 category = kAudioSessionCategory_PlayAndRecord;
     AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(category), &category);
     /*
     // Create the audio stream description
     AudioStreamBasicDescription *description = &(this->m_description);
     description->mSampleRate = m_sampleRate;
     description->mFormatID = kAudioFormatLinearPCM;
     description->mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
     description->mChannelsPerFrame = m_numChannels;
     description->mFramesPerPacket = 1;
     description->mBitsPerChannel = m_bitsPerSample;
     description->mBytesPerPacket = description->mBitsPerChannel / 8 * description->mChannelsPerFrame;
     description->mBytesPerFrame = description->mBytesPerPacket;
     description->mReserved = 0;
     */
    SetupAudioFormat(kAudioFormatLinearPCM, true);
    //int packetperbuffer = 1000 / TDAV_PRODUCER_AUDIO(producer)->ptime;
    //producer->buffer_size = description->mSampleRate * description->mBytesPerFrame / packetperbuffer;
    
    // Create the record audio queue
    ret = AudioQueueNewInput(&(this->m_description),
                             __handle_input_buffer,
                             this,
                             NULL, 
                             kCFRunLoopCommonModes,
                             0,
                             &(this->queue));
    if (ret )
    {
      PTRACE(3, "PSoundChannelCoreAudio\t Call AudioQueueNewInput failed, error is " << ret); 
    }
    buffer_size = ComputeRecordBufferSize(&m_description, kBufferDurationSeconds);	// enough bytes for half a second
    SetBuffers(buffer_size, D_BUFFER_COUNT); 
    for(int i = 0; i < D_BUFFER_COUNT  && i< sizeof(m_buffers)/ sizeof(AudioQueueBufferRef); i++) {
      // Create the buffer for the queue
      OSStatus ret = AudioQueueAllocateBuffer(this->queue, this->buffer_size, &(this->m_buffers[i]));
      if (ret) {
        PTRACE(3, "PSoundChannelCoreAudio\t Call AudioQueueAllocateBuffer for input device failed, error is " << ret); 
        break;
      }
      
      // Clear the data
      //memset(this->m_buffers[i]->mAudioData, 0, this->buffer_size);
      //this->m_buffers[i]->mAudioDataByteSize = this->buffer_size;
      
      // Enqueue the buffer
      ret = AudioQueueEnqueueBuffer(this->queue, this->m_buffers[i], 0, NULL);
      if (ret) {
        PTRACE(3, "PSoundChannelCoreAudio\t Call setbuffer(input)in AudioQueueEnqueueBuffer failed, error is " << ret); 
        break;
      }
      else {
        PTRACE(3, "PSoundChannelCoreAudio\t Call  AudioQueueEnqueueBuffer(input) succeed, mAudioDataBytesCapacity=" << this->m_buffers[i]->mAudioDataBytesCapacity << ",mAudioDataByteSize"<<this->m_buffers[i]->mAudioDataByteSize); 
      }
      
    }    
  }else {
    // Set audio category
    UInt32 category = kAudioSessionCategory_PlayAndRecord; 
     AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(category), &category);
     
     // Create the audio stream description
     /*AudioStreamBasicDescription *description = &(this->m_description);
     description->mSampleRate = m_sampleRate;
     
     description->mFormatID = kAudioFormatLinearPCM;
     description->mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
     description->mChannelsPerFrame = m_numChannels;//TDAV_CONSUMER_AUDIO(consumer)->channels;
     description->mFramesPerPacket = 1;
     description->mBitsPerChannel =  m_bitsPerSample;//TDAV_CONSUMER_AUDIO(consumer)->bits_per_sample;
     description->mBytesPerPacket = description->mBitsPerChannel / 8 * description->mChannelsPerFrame;
     description->mBytesPerFrame = description->mBytesPerPacket;
     description->mReserved = 0;
     */
    SetupAudioFormat(kAudioFormatLinearPCM, true);
    // Create the playback audio queue
    ret = AudioQueueNewOutput(&(this->m_description),
                              __handle_output_buffer,
                              this,
                              NULL, 
                              NULL,
                              0,
                              &(this->queue));    
    if (ret )
    {
      PTRACE(3, "PSoundChannelCoreAudio\t Call AudioQueueNewOutput failed, error is " << ret); 
    }
    buffer_size =ComputeRecordBufferSize(&m_description, kBufferDurationSeconds);	// enough bytes for half a second
    PTRACE(3, "PSoundChannelCoreAudio\tComputeRecordBufferSize ="<<buffer_size);
    SetBuffers(buffer_size, D_BUFFER_COUNT); 
    for(int i = 0; i < D_BUFFER_COUNT  && i< sizeof(m_buffers)/ sizeof(AudioQueueBufferRef); i++) {
      // Create the buffer for the queue
      OSStatus ret = AudioQueueAllocateBuffer(this->queue, this->buffer_size, &(this->m_buffers[i]));
      if (ret) {
        PTRACE(3, "PSoundChannelCoreAudio\t Call setbuffer(output) in AudioQueueAllocateBuffer failed, error is " << ret << ",buffer_size=" << buffer_size); 
        break;
      }else {
        PTRACE(3, "PSoundChannelCoreAudio\t Call AudioQueueAllocateBuffer(output) OK,buffer_size=" << buffer_size); 
      }

      
      // Clear the data
     // memset(this->m_buffers[i]->mAudioData, 0, this->buffer_size);
      this->m_buffers[i]->mAudioDataByteSize = this->buffer_size;
      
      // Enqueue the buffer
      ret = AudioQueueEnqueueBuffer(this->queue, this->m_buffers[i], 0, NULL);
      if (ret) {
        PTRACE(3, "PSoundChannelCoreAudio\t Call setbuffer(output) in AudioQueueEnqueueBuffer failed, error is " << ret); 
        break;
      }else {
        PTRACE(3, "PSoundChannelCoreAudio\t Call  AudioQueueEnqueueBuffer(output) succeed, mAudioDataBytesCapacity=" << this->m_buffers[i]->mAudioDataBytesCapacity << ",mAudioDataByteSize"<<this->m_buffers[i]->mAudioDataByteSize); 
      } 
    }
  }

  opened = true;
  os_handle = 1;
  
#ifdef _DEBUG
  //ret = AudioQueueStart(queue, NULL);
#else
  ret = AudioQueueStart(queue, NULL);
#endif
  PTRACE(5, "PSoundChannelCoreAudio\t Opened a core audio device for dir " << GETDIR);
  
  if (ret )
  {
    PTRACE(3, "PSoundChannelCoreAudio\t Call AudioQueueStart failed, error is " << ret << GETDIR); 
  }else
    PTRACE(3, "PSoundChannelCoreAudio\t Call AudioQueueStart succeed, dir is " << GETDIR); 
  //bufferByteOffset=0;
  return PTrue;
}

PBoolean PSoundChannelCoreAudio::IsOpen() const
{ 
  return opened ? PTrue : PFalse;
}

PBoolean PSoundChannelCoreAudio::SetFormat(unsigned numChannels,
                                           unsigned sampleRate,
                                           unsigned bitsPerSample)
{
  Abort();
  
  //waveFormat.SetFormat(numChannels, sampleRate, bitsPerSample);
  m_numChannels = numChannels;
  m_sampleRate = sampleRate;
  m_bitsPerSample = bitsPerSample;
  //PTRACE(5, "PSoundChannelCoreAudio\tSetFormat numChannels=" <<numChannels<<",sampleRate="<<sampleRate<<",bitsPerSample"<<bitsPerSample);
  return PTrue;//OpenDevice(os_handle);
}




unsigned PSoundChannelCoreAudio::GetChannels() const
{
  return m_numChannels ;
}


unsigned PSoundChannelCoreAudio::GetSampleRate() const
{
  return m_sampleRate;
}


unsigned PSoundChannelCoreAudio::GetSampleSize() const
{
  return m_bitsPerSample;
}


PBoolean PSoundChannelCoreAudio::Close()
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);
  opened = false;
  //PString strTmp =GETDIR;
  PTRACE(5, "SoundChannelCoreAudio\t Closing core audio for dir " << GETDIR );
  
  OSStatus ret;
  
  ret = AudioQueueStop(this->queue, false);
  if (ret)
  {
    PTRACE(5, "PSoundChannelCoreAudio\t Call AudioQueueStop failed");
  }else {
    PTRACE(5, "PSoundChannelCoreAudio\t Call AudioQueueStop ok");
  }
  m_syncThread.Signal();
  
  PWaitAndSignal mutex(bufferMutex);
  delete m_pAudionBuf;
  Abort();
  
  
  
  if(!opened){
    
    return false;
  }
  
  
  
  opened = false;
  os_handle = -1;
  return PTrue;
  PTRACE(5, "SoundChannelCoreAudio\t Closed core audio for dir " << GETDIR );
}


PBoolean PSoundChannelCoreAudio::SetBuffers(PINDEX size, PINDEX count)
{
  
#ifdef D_WAVE_BUF
  if ( count == buffers.GetSize() && buffers[0].GetSize() == size)
    return PTrue;
  //m_buffersHeight = count;
  //m_buffersWidth = size;
  Close();
  /*if (size > 0 )
   {
   //PTRACE(3, "PSoundChannelCoreAudio\t Already allocated the buffers."
   for(int i=0; i<buffer_size && i< sizeof(m_buffers)/ sizeof(AudioQueueBufferRef); i++){
   if (m_buffers[i]!=NULL){
   AudioQueueFreeBuffer( queue,  m_buffers[i]);
   m_buffers[i]=NULL;
   }
   }
   
   //AudioQueueDispose( queue, true);
   
   
   }*/
  this->buffer_size = size;
  PBoolean ok= PTrue;
  if (!buffers.SetSize(count))
    ok = PFalse;
  else {
    for (PINDEX i = 0; i < count; i++) {
      if (buffers.GetAt(i) == NULL)
        buffers.SetAt(i, new PWaveBuffer(size));
      if (!buffers[i].SetSize(size))
        ok = PFalse;
    }
  }
  if (ok == PFalse ) return ok;
  PTRACE(3, "PSoundChannelCoreAudio\t allocated the buffers.buffer_size=" << size <<",buffer_Count" << count);
  
  /* 
   bufferByteOffset = P_MAX_INDEX;
   bufferIndex = 0;  
   int packetperbuffer = size;//1000 / TDAV_CONSUMER_AUDIO(consumer)->ptime;
   this->buffer_size = m_description.mSampleRate * m_description.mBytesPerFrame / packetperbuffer;
   
   
   for(int i = 0; i < count; i++) {
   // Create the buffer for the queue
   OSStatus ret = AudioQueueAllocateBuffer(this->queue, this->buffer_size, &(this->m_buffers[i]));
   if (ret) {
   PTRACE(3, "Macaudio\t Call setbuffer in AudioQueueAllocateBuffer failed, error is " << ret); 
   break;
   }
   
   // Clear the data
   memset(this->m_buffers[i]->mAudioData, 0, this->buffer_size);
   this->m_buffers[i]->mAudioDataByteSize = this->buffer_size;
   
   // Enqueue the buffer
   ret = AudioQueueEnqueueBuffer(this->queue, this->m_buffers[i], 0, NULL);
   if (ret) {
   PTRACE(3, "Macaudio\t Call setbuffer in AudioQueueEnqueueBuffer failed, error is " << ret); 
   break;
   }
   }*/
#else
  buffer_size = size;
  buffers_GetSize = count;
#endif
  return true;
}


PBoolean PSoundChannelCoreAudio::GetBuffers(PINDEX & size, PINDEX & count)
{
  PWaitAndSignal mutex(bufferMutex);
  
  count = buffers_GetSize;
  
  if (count == 0)
    size = 0;
  else
    size = buffer_size;
  
  
  return PTrue;
}


PBoolean PSoundChannelCoreAudio::Write(const void * data, PINDEX size)
{
#ifdef D_WAVE_BUF
  lastWriteCount = size;
  
  const BYTE * ptr = (const BYTE *)data;
  
  bufferMutex.Wait();
  
  DWORD osError = 0;
  int dataoffset=0;
  while (size > 0 && buffers.GetSize() >0 && bufferIndex<  buffers.GetSize()) {
    PWaveBuffer & buffer = buffers[bufferIndex];
    /*while ((buffer.header.dwFlags&WHDR_DONE) == 0) {
     bufferMutex.Signal();
     // No free buffers, so wait for one
     //if (WaitForSingleObject(hEventDone, INFINITE) != WAIT_OBJECT_0)
     //  return SetErrorValues(Miscellaneous, ::GetLastError()|PWIN32ErrorFlag, LastWriteError);
     //bufferMutex.Wait();
     size =0;//we drop it ,chy
     return PTrue;
     }*/
    
    // Can't write more than a buffer full
    PINDEX count = buffer.GetSize();
    if (count <= size ){
      count = buffer.GetSize();
      buffer.PrepareCommon( count) ;
    }
    else {
      count = size;
      buffer.PrepareCommon( count) ;
    }
    
    memcpy(buffer.GetPointer(), ptr+dataoffset, count);
    dataoffset+=count;
    
    //PTRACE(5, "SoundChannelCoreAudio\t Preparing Playing data len=" << lastWriteCount << ",Played datalen=" <<count <<",bufferIndex=" <<bufferIndex << ",size" << size); 
    
    
    //if ((osError = waveOutWrite(hWaveOut, &buffer.header, sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
    //  break;
    
    size -= count;
    //ptr += count;
    bufferIndex++;
    if (bufferIndex >=buffers.GetSize() ) bufferIndex=0;
  }
  
  bufferMutex.Signal();
  //if (size != 0)
  //  return SetErrorValues(Miscellaneous, osError|PWIN32ErrorFlag, LastWriteError);
  //lastWriteCount = size;already changed
#else
  lastWriteCount = 0;
  
  //const BYTE * ptr = (const BYTE *)data;
  //PTRACE(5, "SoundChannelCoreAudio\t Preparing Played begin datalen="    << size); 
  
  PWaitAndSignal mutex(bufferMutex);
  lastWriteCount = m_pAudionBuf->Fill((const char*)data, size, false, true);
  
  //PTRACE(5, "SoundChannelCoreAudio\t Preparing Played end datalen="  << lastWriteCount); 
  //m_syncThread.Signal();
#endif
  return PTrue;
}

PBoolean PSoundChannelCoreAudio::Abort()
{
  
  return PTrue;
}


PString PSoundChannelCoreAudio::GetErrorText(ErrorGroup group) const
{
  PString str;
  
  return str;
}


PBoolean PSoundChannelCoreAudio::SetVolume(unsigned newVolume)
{
  if (!IsOpen()  )
    return SetErrorValues(NotOpen, EBADF);
  
  
  
  return true;
}



PBoolean PSoundChannelCoreAudio::GetVolume(unsigned & oldVolume)
{
  if (!IsOpen()  )
    return SetErrorValues(NotOpen, EBADF);
  
  return true;
}
PBoolean PSoundChannelCoreAudio::Read(void * data, PINDEX size)
{
#ifdef D_WAVE_BUF
  lastReadCount = 0;
  m_syncThread.Wait();//wait until taskthread exited.
  size = 0;
  PWaitAndSignal mutex(bufferMutex);
  
  // Check to see if Abort() was called in another thread
  if (bufferByteOffset == P_MAX_INDEX)
  {
    PTRACE(5, "SoundChannelCoreAudio\tbufferByteOffset is P_MAX_INDEX"   );
    //size =0;
    return PTrue;
  }
  PTRACE(5, "SoundChannelCoreAudio\tRead buffers bufferIndex="  << bufferIndex << ",bufferByteOffset=" <<bufferByteOffset) ;
  PWaveBuffer & buffer = buffers[bufferIndex];
  PTRACE(5, "SoundChannelCoreAudio\tRead buffers buffer.size="  << buffer.GetSize()  <<",buffer.m_dwBufferLength="<< buffer.m_dwBufferLength) ;
  
  lastReadCount = buffer.m_dwBufferLength- bufferByteOffset;
  if (lastReadCount == 0 )
    return PTrue;
  if (lastReadCount > size)
    lastReadCount = size;
  
  memcpy(data, &buffer[bufferByteOffset], lastReadCount);
  
  bufferByteOffset += lastReadCount;
  if (bufferByteOffset >= (PINDEX)buffer.m_dwBufferLength) {
    
    bufferIndex = (bufferIndex+1)%buffers.GetSize();
    bufferByteOffset = 0;
  }
#else
  lastReadCount = 0;
  //PTRACE(5, "SoundChannelCoreAudio\t record begin datalen="  << size); 
  //m_syncThread.Wait();//wait until taskthread exited.
  while (IsOpen() && m_pAudionBuf->size() <=0) {
    PThread::Sleep(1);
  }
  PWaitAndSignal mutex(bufferMutex);
  lastReadCount = m_pAudionBuf->Drain((char*)data, size);
  //if (lastReadCount>0)
  //  PTRACE(5, "SoundChannelCoreAudio\t recorded end datalen="  << size); 
#endif
  //prebufferIndex= bufferIndex;
  return PTrue;
}

void PSoundChannelCoreAudio::SetupAudioFormat(UInt32 inFormatID, bool bChangeSample)
{
	memset(&m_description, 0, sizeof(m_description));
  
  UInt32 size = sizeof(m_description.mSampleRate) ;
  Float64 nPreferredHardwareSampleRate=0.0;
  OSStatus ret =0;
  if ((ret = AudioSessionGetProperty(	kAudioSessionProperty_PreferredHardwareSampleRate,
                                     &size, 
                                     &nPreferredHardwareSampleRate) ) !=0)
  {
    PTRACE(5, "couldn't get hardware sample rate, erro" << ret);
  }else {
    PTRACE(5, "PSoundChannelCoreAudio\t Preferred sample rate is " << nPreferredHardwareSampleRate );
    //m_description.mSampleRate = 8000;
  }

  if ((ret = AudioSessionGetProperty(	kAudioSessionProperty_CurrentHardwareSampleRate,
                                     &size, 
                                     &m_description.mSampleRate) ) !=0)
  {
    PTRACE(5, "couldn't get hardware sample rate, erro" << ret);
  }else {
    if (bChangeSample){
       m_description.mSampleRate = m_sampleRate;
      PTRACE(5, "PSoundChannelCoreAudio\t Currect sample rate is " << m_description.mSampleRate << ",we changed it to" << m_sampleRate);
    }else {
      PTRACE(5, "PSoundChannelCoreAudio\t Currect sample rate is " << m_description.mSampleRate << ",we donot changed it.");
   }

   
  }

  
  size = sizeof(m_description.mChannelsPerFrame);
  if ((ret =AudioSessionGetProperty(	kAudioSessionProperty_CurrentHardwareInputNumberChannels, 
                                    &size, 
                                    &m_description.mChannelsPerFrame)) !=0)
  {
    PTRACE(5, "couldn't get input channel count,err " << ret);
  }else {
    PTRACE(5, "PSoundChannelCoreAudio\t Got input channel count that is " << m_description.mChannelsPerFrame);
  }

  
  m_description.mFormatID = inFormatID;
  if (inFormatID == kAudioFormatLinearPCM)
  {
    // if we want pcm, default to signed 16-bit little-endian
    m_description.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    m_description.mBytesPerFrame = 80*2;
    m_description.mBitsPerChannel = m_bitsPerSample;
    m_description.mBytesPerPacket = m_description.mBytesPerFrame = (m_description.mBitsPerChannel / 8) * m_description.mChannelsPerFrame;
    m_description.mFramesPerPacket = m_numChannels;
  }
}
int PSoundChannelCoreAudio::ComputeRecordBufferSize(const AudioStreamBasicDescription *format, float seconds)
{
	int packets, frames, bytes = 0;
  OSStatus ret=0;
  
  frames = (int)ceil(seconds * format->mSampleRate);
  
  if (format->mBytesPerFrame > 0)
    bytes = frames * format->mBytesPerFrame;
  else {
    UInt32 maxPacketSize;
    if (format->mBytesPerPacket > 0)
      maxPacketSize = format->mBytesPerPacket;	// constant packet size
    else {
      UInt32 propertySize = sizeof(maxPacketSize);
      if (0!=AudioQueueGetProperty(this->queue, kAudioQueueProperty_MaximumOutputPacketSize, &maxPacketSize,
                                   &propertySize))
      {
        PTRACE(5, "couldn't get queue's maximum output packet size");
      }
    }
    if (format->mFramesPerPacket > 0)
      packets = frames / format->mFramesPerPacket;
    else
      packets = frames;	// worst-case scenario: 1 frame in a packet
    if (packets == 0)		// sanity check
      packets = 1;
    bytes = packets * maxPacketSize;
  }
  
	return bytes;
}
PBoolean PSoundChannelCoreAudio::isRecordBufferFull()
{
  PAssert(direction == Recorder, PInvalidParameter);
  //if(state != setbuffer_){
  //  PTRACE(1, __func__ << " Initialize the device first");
  //  return PFalse;
  //}
  
  return (m_pAudionBuf->size() > buffer_size/*bufferSizeBytes*/);
}

PBoolean PSoundChannelCoreAudio::AreAllRecordBuffersFull()
{
  PAssert(direction == Recorder, PInvalidParameter);
  //if(state != setbuffer_){
  //  PTRACE(1, __func__ << " Initialize the device first");
  //  return PFalse;
  //}
  
  return (m_pAudionBuf->Full());
}

PBoolean PSoundChannelCoreAudio::WaitForRecordBufferFull()
{
  PTRACE(1, __func__ );
  PAssert(0, PUnimplementedFunction);
  if (os_handle < 0) {
    return PFalse;
  }
  
  return PTrue;//PXSetIOBlock(PXReadBlock, readTimeout);
}

PBoolean PSoundChannelCoreAudio::WaitForAllRecordBuffersFull()
{
  PTRACE(1, __func__ );
  PAssert(0, PUnimplementedFunction);
  return false;
}

// End of file
