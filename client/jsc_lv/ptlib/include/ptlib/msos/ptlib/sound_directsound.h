/*
 * sound_directsound.h
 *
 * DirectX Sound driver implementation.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2006-2007 Novacom, a division of IT-Optics
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original DirectSound Code is 
 * Vincent Luba <vincent.luba@novacom.be>
 *
 * Contributor(s): /
 *
 * $Revision: 21788 $
 * $Author: rjongbloed $
 * $Date: 2008-12-12 05:42:13 +0000 (Fri, 12 Dec 2008) $
 */

#ifndef __DIRECTSOUND_H__
#define __DIRECTSOUND_H__


#include <ptlib.h>
#include <ptbuildopts.h>

#if defined(P_DIRECTSOUND) && ! defined(P_DIRECTSOUND_WINCE) 

#include <ptlib/sound.h>

#include <dsound.h>

#ifdef _WIN32_WCE
#define LPDIRECTSOUND8 LPDIRECTSOUND
#define LPDIRECTSOUNDBUFFER8 LPDIRECTSOUNDBUFFER
#define LPDIRECTSOUNDCAPTURE8 LPDIRECTSOUNDCAPTURE
#define LPDIRECTSOUNDCAPTUREBUFFER8 LPDIRECTSOUNDCAPTUREBUFFER
#define DirectSoundCreate8 DirectSoundCreate
#define IID_IDirectSoundBuffer8 IID_IDirectSoundBuffer
#define DirectSoundCaptureCreate8 DirectSoundCaptureCreate
#define IID_IDirectSoundCaptureBuffer8 IID_IDirectSoundCaptureBuffer
#endif

class PSoundChannelDirectSound: public PSoundChannel
{
public:
    PSoundChannelDirectSound();
    void Construct();
    PSoundChannelDirectSound(const PString &device,
			     PSoundChannel::Directions dir,
			     unsigned numChannels,
			     unsigned sampleRate,
			     unsigned bitsPerSample);
    ~PSoundChannelDirectSound();
    static PStringArray GetDeviceNames(PSoundChannel::Directions);
    PBoolean Open(const PString & device,
              Directions dir,
              unsigned numChannels,
              unsigned sampleRate,
              unsigned bitsPerSample);
    PBoolean Setup();
    PBoolean Close();
    PString GetName() const;
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
    PBoolean PlaySound(const PSound & sound, PBoolean wait);
    PBoolean PlayFile(const PFilePath & filename, PBoolean wait);
    PBoolean HasPlayCompleted();
    PBoolean WaitForPlayCompletion();
    PBoolean RecordSound(PSound & sound);
    PBoolean RecordFile(const PFilePath & filename);
    PBoolean StartRecording();
    PBoolean IsRecordBufferFull();
    PBoolean AreAllRecordBuffersFull();
    PBoolean WaitForRecordBufferFull();
    PBoolean WaitForAllRecordBuffersFull();
    PBoolean Abort();
    PBoolean SetVolume (unsigned);
    PBoolean GetVolume (unsigned &);
    PBoolean IsOpen() const;

private:

    unsigned mNumChannels;
    unsigned mSampleRate;
    unsigned mBitsPerSample;
    
    PBoolean isInitialised;
    PBoolean isOpen;

    PString deviceName;
    Directions mDirection;

    LPDIRECTSOUNDCAPTURE8 sAudioCaptureDevice;
    LPDIRECTSOUNDCAPTUREBUFFER8 mAudioCaptureBuffer;

    LPDIRECTSOUND8 sAudioPlaybackDevice;
    LPDIRECTSOUNDBUFFER8 mAudioPlaybackBuffer;
    LPDIRECTSOUNDBUFFER mAudioPrimaryPlaybackBuffer;
    
    PBoolean InitPlaybackBuffer();
    PBoolean InitPlaybackDevice(GUID *pGUID);
    
    PBoolean InitCaptureBuffer();
    PBoolean InitCaptureDevice(GUID *pGUID);
    
    PBoolean GetDeviceID (PString deviceName, GUID *pGUID);

    PINDEX WriteToDXBuffer(const void * buf, PINDEX len, DWORD position);
    PINDEX ReadFromDXBuffer(const void * buf, PINDEX len, DWORD position);
    
    PBoolean SetFormat ();

    PBoolean mStreaming;
    PINDEX mBufferSize;
    PINDEX mDXBufferSize;
    PINDEX mBufferCount;

    PINDEX mVolume;

    WAVEFORMATEX mWFX;
    
    PMutex           bufferMutex;
};

#endif // P_DIRECTSOUND

#endif  /* __DIRECTSOUND_H__ */
