/*
 * sound.cxx
 *
 * Sound driver implementation.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): Derek Smithies - from a modification of oss module.
 *
 * $Revision: 20949 $
 * $Author: ms30002000 $
 * $Date: 2008-09-14 21:02:41 +1200 (Sun, 14 Sep 2008) $
 */
   

#pragma implementation "sound_pulse.h"

#include <pulse/simple.h>
#include <pulse/error.h>
//#include <pulse/gccmacro.h>
#include <pulse/sample.h>
#include <ptlib.h>
#include <ptclib/random.h>

#include "sound_pulse.h"



PCREATE_SOUND_PLUGIN(Pulse, PSoundChannelPulse);

///////////////////////////////////////////////////////////////////////////////
PSoundChannelPulse::PSoundChannelPulse()
{
  PTRACE(6, "Pulse\tConstructor for no args");
  PSoundChannelPulse::Construct();
  s = NULL;
  setenv ("PULSE_PROP_media.role", "phone", true);
}


PSoundChannelPulse::PSoundChannelPulse(const PString & device,
				       Directions dir,
				       unsigned numChannels,
				       unsigned sampleRate,
				       unsigned bitsPerSample)
{
  PTRACE(6, "Pulse\tConstructor with many args\n");

  PAssert((bitsPerSample == 16), PInvalidParameter);
  Construct();
  ss.rate = sampleRate;
  ss.channels = numChannels;
  Open(device, dir, numChannels, sampleRate, bitsPerSample);
}


void PSoundChannelPulse::Construct()
{
  PTRACE(6, "Pulse\tConstruct "); 
  os_handle = -1;
  s = NULL;
  ss.format =  PA_SAMPLE_S16LE;
}


PSoundChannelPulse::~PSoundChannelPulse()
{
  PTRACE(6, "Pulse\tDestructor ");
  Close();
}



PStringArray PSoundChannelPulse::GetDeviceNames(Directions /*dir*/)
{
  // First locate sound cards. On Linux with devfs and on the other platforms
  // (eg FreeBSD), we search for filenames with dspN or mixerN.
  // On linux without devfs we scan all of the devices and look for ones
  // with major device numbers corresponding to Pulse compatible drivers.

  PTRACE(6, "Pulse\tReport devicenames as \"ptlib pulse plugin\"");
  PStringArray devices;
  devices.AppendString("ptlib pulse plugin");

  return devices;
}


PString PSoundChannelPulse::GetDefaultDevice(Directions dir)
{
  PTRACE(6, "Pulse\t report default device as \"ptlib pulse plugin\"");
  PStringArray devicenames;
  devicenames = PSoundChannelPulse::GetDeviceNames(dir);

  return devicenames[0];
}

PBoolean PSoundChannelPulse::Open(const PString & _device,
				  Directions _dir,
				  unsigned _numChannels,
				  unsigned _sampleRate,
				  unsigned _bitsPerSample)
{
  PTRACE(6, "Pulse\t Open on device name of " << _device);
  Close();
  direction = _dir;
  mNumChannels = _numChannels;
  mSampleRate = _sampleRate;
  mBitsPerSample = _bitsPerSample;
  Construct();

  PWaitAndSignal m(deviceMutex);
  int error;
  char *app = getenv ("PULSE_PROP_application.name");
  PStringStream appName, streamName;
  if (app != NULL)
    appName << app;
  else
    appName << "PTLib plugin ";
  if (_dir == Player) 
    streamName << ::hex << PRandom::Number();
  else
    streamName << ::hex << PRandom::Number();

  ss.rate = _sampleRate;
  ss.channels = _numChannels;
  ss.format =  PA_SAMPLE_S16LE;  

  if (_dir == Player) {
    s = pa_simple_new(NULL, appName.GetPointer(), PA_STREAM_PLAYBACK, NULL, 
		      streamName.GetPointer(), &ss, NULL, NULL, &error);
  } else {
    s = pa_simple_new(NULL, appName.GetPointer(), PA_STREAM_RECORD, NULL, 
		      streamName.GetPointer(), &ss, NULL, NULL, &error);
  }

  if (s == NULL) {
    PTRACE(2, ": pa_simple_new() failed: " << pa_strerror(error));
    PTRACE(2, ": pa_simple_new() uses stream " << streamName);
    PTRACE(2, ": pa_simple_new() uses rate " << PINDEX(ss.rate));
    PTRACE(2, ": pa_simple_new() uses channels " << PINDEX(ss.channels));
    return PFalse;
  }
  
  os_handle = 1;
  return PTrue;
}

PBoolean PSoundChannelPulse::Close()
{
  PTRACE(6, "Pulse\tClose");
  int error;
  PWaitAndSignal m(deviceMutex);

  if (s == NULL)
    return PTrue;

  /* Make sure that every single sample was played. We don't care about
     errors here - we are closing it and want all the sound out. */

  pa_simple_drain(s, &error);
  
  if (s)
    pa_simple_free(s);
  
  s = NULL;

  os_handle = -1;
  return PTrue;
}

PBoolean PSoundChannelPulse::IsOpen() const
{
  PTRACE(6, "Pulse\t report is open as " << (os_handle >= 0));
  PWaitAndSignal m(deviceMutex);
  return os_handle >= 0;
}

PBoolean PSoundChannelPulse::Write(const void * buf, PINDEX len)
{
  PTRACE(6, "Pulse\tWrite " << len << " bytes");
  int error;
  PWaitAndSignal m(deviceMutex);

  if (!IsOpen()) {
    PTRACE(4, ": Pulse audio Write() failed as device closed");
    return PFalse;
  }

  if (pa_simple_write(s, buf, (size_t) len, &error) < 0) {
    PTRACE(4, ": pa_simple_write() failed: " << pa_strerror(error));
    return PFalse;   
  }

  lastWriteCount = len;

  PTRACE(6, "Pulse\tWrite completed");
  return PTrue;
}

PBoolean PSoundChannelPulse::Read(void * buf, PINDEX len)
{
  PTRACE(6, "Pulse\tRead " << len << " bytes");
  int error;
  PWaitAndSignal m(deviceMutex);

  if (!IsOpen()) {
    PTRACE(4, ": Pulse audio Read() failed as device closed");
    return PFalse;
  }

  if (pa_simple_read(s, buf, (size_t) len, &error) < 0) {
    PTRACE(4, ": pa_simple_read() failed: " << pa_strerror(error));
    return PFalse;   
  }

  lastReadCount = len;

  PTRACE(6, "Pulse\tRead completed of " <<len << " bytes");
  return PTrue;
}


PBoolean PSoundChannelPulse::SetFormat(unsigned numChannels,
                              unsigned sampleRate,
                              unsigned bitsPerSample)
{
  PTRACE(6, "Pulse\tSet format");

  ss.rate = sampleRate;
  ss.channels = numChannels;
  PAssert((bitsPerSample == 16), PInvalidParameter);

  return PTrue;
}

// Get  the number of channels (mono/stereo) in the sound.
unsigned PSoundChannelPulse::GetChannels()   const
{
  PTRACE(6, "Pulse\tGetChannels return " 
	 << ss.channels << " channel(s)");
  return ss.channels;
}

// Get the sample rate in samples per second.
unsigned PSoundChannelPulse::GetSampleRate() const
{
  PTRACE(6, "Pulse\tGet sample rate return " 
	 << ss.rate << " samples per second");
  return ss.rate;
}

// Get the sample size in bits per sample.
unsigned PSoundChannelPulse::GetSampleSize() const
{
  return 16;
}

PBoolean PSoundChannelPulse::SetBuffers(PINDEX size, PINDEX count)
{
  PTRACE(6, "Pulse\tSet buffers to " << size << " and " << count);
  bufferSize = size;
  bufferCount = count;

  return PTrue;
}


PBoolean PSoundChannelPulse::GetBuffers(PINDEX & size, PINDEX & count)
{
  size = bufferSize;
  count = bufferCount;
  PTRACE(6, "Pulse\t report buffers as " << size << " and " << count);
  return PTrue;
}


PBoolean PSoundChannelPulse::PlaySound(const PSound & sound, PBoolean wait)
{

  return PFalse;
}


PBoolean PSoundChannelPulse::PlayFile(const PFilePath & filename, PBoolean wait)
{
  return PFalse;
}


PBoolean PSoundChannelPulse::HasPlayCompleted()
{
  return PTrue;
}


PBoolean PSoundChannelPulse::WaitForPlayCompletion()
{
  return PTrue;
}


PBoolean PSoundChannelPulse::RecordSound(PSound & sound)
{
  return PFalse;
}


PBoolean PSoundChannelPulse::RecordFile(const PFilePath & filename)
{
  return PFalse;
}


PBoolean PSoundChannelPulse::StartRecording()
{
  return PFalse;
}


PBoolean PSoundChannelPulse::IsRecordBufferFull()
{
  return PFalse;
}


PBoolean PSoundChannelPulse::AreAllRecordBuffersFull()
{
  return PFalse;
}


PBoolean PSoundChannelPulse::WaitForRecordBufferFull()
{
  return PFalse;
}


PBoolean PSoundChannelPulse::WaitForAllRecordBuffersFull()
{
  return PFalse;
}


PBoolean PSoundChannelPulse::SetVolume(unsigned newVal)
{
  PWaitAndSignal m(deviceMutex);
  return PTrue;
}

PBoolean  PSoundChannelPulse::GetVolume(unsigned &devVol)
{
  PWaitAndSignal m(deviceMutex);
  return PTrue;
}
  


// End of file

