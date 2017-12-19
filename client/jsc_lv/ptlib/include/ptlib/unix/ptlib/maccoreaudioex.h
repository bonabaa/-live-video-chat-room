/*
 * maccoreaudio.h
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

#pragma interface

// workaround to remove warnings, when including OSServiceProviders
#ifndef __MAC_CORE_AUDIO
#define __MAC_CORE_AUDIO
#define __OPENTRANSPORTPROVIDERS__
#define D_IPHONE "iphone"
//#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>;
#include <AudioUnit/AudioUnit.h>

// needed by lists.h of pwlib, unfortunately also defined in previous
// includes from Apple
#undef nil

typedef UInt32      AudioDeviceID;
// static loading of plugins
//#define P_FORCE_STATIC_PLUGIN

#include <ptlib.h>
#include <ptlib/sound.h>
#include <ptlib/plugin.h>
#include <ptlib/pluginmgr.h>


#define CA_DUMMY_DEVICE_NAME "Null"
#define kAudioDeviceDummy kAudioDeviceUnknown

#ifndef _WIN32
typedef unsigned int DWORD;
#endif

class PWaveBuffer : public PBYTEArray
{
	PCLASSINFO(PWaveBuffer, PBYTEArray);
public:
  PWaveBuffer(PINDEX sz = 0);
  ~PWaveBuffer();
	
  //PWaveBuffer & operator=(const PSound & sound);
	
  //DWORD Prepare(HWAVEOUT hWaveOut, PINDEX & count);
  //DWORD Prepare(HWAVEIN hWaveIn);
  //DWORD Release();
	
  void PrepareCommon(PINDEX count);
	
  //HWAVEOUT hWaveOut;
  //HWAVEIN  hWaveIn;
  //WAVEHDR  header;
	int m_dwBufferLength;
	
	//friend class PSoundChannelCoreAudio;
};

PARRAY(PWaveBufferArray, PWaveBuffer);

class CircularBuffer;
#define D_BUFFER_COUNT 4//buffers.GetSize()

class PSoundChannelCoreAudio: public PSoundChannel
{
	PCLASSINFO(PSoundChannelCoreAudio, PSoundChannel);
public:
	AudioQueueRef queue;
	AudioStreamBasicDescription m_description ;
	int m_numChannels  ;
	int m_sampleRate  ;
	int m_bitsPerSample  ;
	int buffer_size;
  //PSyncPoint m_syncThread;
  PSemaphore m_syncThread;
  void Construct();
  void SetupAudioFormat(UInt32 inFormatID, bool bChangedSample);
  int ComputeRecordBufferSize(const AudioStreamBasicDescription *format, float seconds);

  PSoundChannelCoreAudio(const PString &device,
                         PSoundChannel::Directions dir,
                         unsigned numChannels,
                         unsigned sampleRate,
                         unsigned bitsPerSample);
  PSoundChannelCoreAudio();
	~PSoundChannelCoreAudio();
  static PStringArray GetDeviceNames(PSoundChannel::Directions = Player);
  PBoolean Open(
                const PString & device,
                Directions dir,
                unsigned numChannels,
                unsigned sampleRate,
                unsigned bitsPerSample
                );
  //PBoolean Setup();
  PBoolean Close();
  PBoolean IsOpen() const;
  PBoolean Write(const void * buf, PINDEX len);
  PBoolean Read(void * buf, PINDEX len);
  PBoolean SetFormat(unsigned numChannels,
                     unsigned sampleRate,
                     unsigned bitsPerSample);
  unsigned GetChannels() const;
  unsigned GetSampleRate() const;
  unsigned GetSampleSize() const;
  PBoolean SetBuffers(PINDEX size, PINDEX count);
  PBoolean GetBuffers(PINDEX & size, PINDEX & count);
  //PBoolean PlaySound(const PSound & sound, PBoolean wait);
  //PBoolean PlayFile(const PFilePath & filename, PBoolean wait);
  //PBoolean HasPlayCompleted();
  //PBoolean WaitForPlayCompletion();
  //PBoolean RecordSound(PSound & sound);
  //PBoolean RecordFile(const PFilePath & filename);
  //PBoolean StartRecording();
  PBoolean isRecordBufferFull();
  PBoolean AreAllRecordBuffersFull();
  PBoolean WaitForRecordBufferFull();
  PBoolean WaitForAllRecordBuffersFull();
  PBoolean Abort();
  PBoolean SetVolume(unsigned newVal);
  PBoolean GetVolume(unsigned &devVol);
	
public:
	void AddlastWriteCount(int v){lastWriteCount += v;};
  // Overrides from class PChannel
  virtual PString GetName() const;
	// Return the name of the channel.
	
	
  PString GetErrorText(ErrorGroup group = NumErrorGroups) const;
  // Get a text form of the last error encountered.
	
  //PBoolean SetFormat(const PWaveFormat & format);
	
  PBoolean Open(const PString & device, Directions dir);//,const PWaveFormat & format);
  // Open with format other than PCM
	
protected:
  PString      deviceName;
  Directions   direction;
	//PWaveFormat  waveFormat;
	
public:
  bool bCanTrace;
  bool         opened;
  PMutex           bufferMutex;
  //PWaveBufferArray buffers;
  //PINDEX           bufferIndex;
  //PINDEX           bufferByteOffset;
	AudioQueueBufferRef m_buffers[D_BUFFER_COUNT];
  CircularBuffer*   m_pAudionBuf;
  int buffers_GetSize;
private:
  PBoolean OpenDevice(unsigned id);
  PBoolean GetDeviceID(const PString & device, Directions dir, unsigned& id);
};

#endif



