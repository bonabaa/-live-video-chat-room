/*
 * sound.cxx
 *
 * Code for pluigns sound device
 *
 * Portable Windows Library
 *
 * Copyright (c) 2003 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): Craig Southeren
 *                 Snark at GnomeMeeting
 *
 * $Revision: 22443 $
 * $Author: rjongbloed $
 * $Date: 2009-04-20 23:47:22 +0000 (Mon, 20 Apr 2009) $
 */

#ifdef __GNUC__
#pragma implementation "sound.h"
#endif

#include <ptlib.h>

#include <ptlib/sound.h>
#include <ptlib/pluginmgr.h>

static const char soundPluginBaseClass[] = "PSoundChannel";


template <> PSoundChannel * PDevicePluginFactory<PSoundChannel>::Worker::Create(const PString & type) const
{
  return PSoundChannel::CreateChannel(type);
}

typedef PDevicePluginAdapter<PSoundChannel> PDevicePluginSoundChannel;
PFACTORY_CREATE(PFactory<PDevicePluginAdapterBase>, PDevicePluginSoundChannel, "PSoundChannel", true);


PStringArray PSoundChannel::GetDriverNames(PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return pluginMgr->GetPluginsProviding(soundPluginBaseClass);
}


PStringArray PSoundChannel::GetDriversDeviceNames(const PString & driverName,
                                                  PSoundChannel::Directions dir,
                                                  PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return pluginMgr->GetPluginsDeviceNames(driverName, soundPluginBaseClass, dir);
}


PSoundChannel * PSoundChannel::CreateChannel(const PString & driverName, PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return (PSoundChannel *)pluginMgr->CreatePluginsDevice(driverName, soundPluginBaseClass, 0);
}


PSoundChannel * PSoundChannel::CreateChannelByName(const PString & deviceName,
                                                   PSoundChannel::Directions dir,
                                                   PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return (PSoundChannel *)pluginMgr->CreatePluginsDeviceByName(deviceName, soundPluginBaseClass, dir);
}


PSoundChannel * PSoundChannel::CreateOpenedChannel(const PString & driverName,
                                                   const PString & deviceName,
                                                   PSoundChannel::Directions dir,
                                                   unsigned numChannels,
                                                   unsigned sampleRate,
                                                   unsigned bitsPerSample,
                                                   PPluginManager * pluginMgr)
{
  PString adjustedDeviceName = deviceName;
  PSoundChannel * sndChan;
  if (driverName.IsEmpty() || driverName == "*") {
    if (deviceName.IsEmpty() || deviceName == "*")
      adjustedDeviceName = PSoundChannel::GetDefaultDevice(dir);
    sndChan = CreateChannelByName(adjustedDeviceName, dir, pluginMgr);
  }
  else {
    if (deviceName.IsEmpty() || deviceName == "*") {
      PStringArray devices = PSoundChannel::GetDriversDeviceNames(driverName, PSoundChannel::Player);
      if (devices.IsEmpty())
        return NULL;
      adjustedDeviceName = devices[0];
    }
    sndChan = CreateChannel(driverName, pluginMgr);
  }

  if (sndChan != NULL && sndChan->Open(adjustedDeviceName, dir, numChannels, sampleRate, bitsPerSample))
    return sndChan;

  delete sndChan;
  return NULL;
}


PStringArray PSoundChannel::GetDeviceNames(PSoundChannel::Directions dir, PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return pluginMgr->GetPluginsDeviceNames("*", soundPluginBaseClass, dir);
}


PString PSoundChannel::GetDefaultDevice(Directions dir)
{
#ifdef _WIN32
  RegistryKey registry("HKEY_CURRENT_USER\\Software\\Microsoft\\Multimedia\\Sound Mapper",
                       RegistryKey::ReadOnly);

  PString str;

  if (dir == Player) {
    if (registry.QueryValue("ConsoleVoiceComPlayback", str) )
      return str;
    if (registry.QueryValue("Playback", str))
      return str;
  }
  else {
    if (registry.QueryValue("ConsoleVoiceComRecord", str))
      return str;
    if (registry.QueryValue("Record", str))
      return str;
  }
#endif

  PStringArray devices = GetDeviceNames(dir);
  if (devices.GetSize() > 0)
    return devices[0];

  return PString::Empty();
}


PSoundChannel::PSoundChannel()
{
  baseChannel = NULL;
}

PSoundChannel::~PSoundChannel()
{
  delete baseChannel;
}

PSoundChannel::PSoundChannel(const PString & device,
                             Directions dir,
                             unsigned numChannels,
                             unsigned sampleRate,
                             unsigned bitsPerSample)
{
  baseChannel = NULL;
  Open(device, dir, numChannels, sampleRate, bitsPerSample);
}

PBoolean PSoundChannel::Open(const PString & devSpec,
                         Directions dir,
                         unsigned numChannels,
                         unsigned sampleRate,
                         unsigned bitsPerSample)
{
  PString driver, device;
  PINDEX colon = devSpec.Find(':');
  if (colon == P_MAX_INDEX)
    device = devSpec;
  else {
    driver = devSpec.Left(colon);
    device = devSpec.Mid(colon+1).Trim();
  }

  delete baseChannel;
  activeDirection = dir;

  baseChannel = CreateOpenedChannel(driver, device, dir, numChannels, sampleRate, bitsPerSample);
  if (baseChannel == NULL && !driver.IsEmpty())
    baseChannel = CreateOpenedChannel(PString::Empty(), devSpec, dir, numChannels, sampleRate, bitsPerSample);

  return baseChannel != NULL;
}

PString PSoundChannel::GetName() const
{
  return baseChannel != NULL ? baseChannel->GetName() : PString::Empty();
}


PBoolean PSoundChannel::IsOpen() const
{
  return baseChannel != NULL && baseChannel->PChannel::IsOpen();
}


PBoolean PSoundChannel::Close()
{
  return baseChannel == NULL || baseChannel->Close();
}


int PSoundChannel::GetHandle() const
{
  return baseChannel == NULL ? -1 : baseChannel->PChannel::GetHandle();
}


PBoolean PSoundChannel::Abort()
{
  PAssert(IsOpen(), PLogicError); 
  return baseChannel->Abort();
}


PBoolean PSoundChannel::SetFormat(unsigned numChannels, unsigned sampleRate, unsigned bitsPerSample)
{
  return baseChannel != NULL && baseChannel->SetFormat(numChannels, sampleRate, bitsPerSample);
}


unsigned PSoundChannel::GetChannels() const
{
  return baseChannel == NULL ? 0 : baseChannel->GetChannels();
}


unsigned PSoundChannel::GetSampleRate() const
{
  return baseChannel == NULL ? 0 : baseChannel->GetSampleRate();
}


unsigned PSoundChannel::GetSampleSize() const 
{
  return baseChannel == NULL ? 0 : baseChannel->GetSampleSize();
}


PBoolean PSoundChannel::SetBuffers(PINDEX size, PINDEX count)
{
  return baseChannel != NULL && baseChannel->SetBuffers(size, count);
}


PBoolean PSoundChannel::GetBuffers(PINDEX & size, PINDEX & count)
{
  return baseChannel != NULL && baseChannel->GetBuffers(size, count);
}


PBoolean PSoundChannel::SetVolume(unsigned volume)
{
  return baseChannel != NULL && baseChannel->SetVolume(volume);
}


PBoolean PSoundChannel::GetVolume(unsigned & volume)
{
  return baseChannel != NULL && baseChannel->GetVolume(volume);
}


PBoolean PSoundChannel::Write(const void * buf, PINDEX len)
{
  PAssert(activeDirection == Player, PLogicError);

  if (len == 0)
    return IsOpen();

  return baseChannel != NULL && baseChannel->Write(buf, len);
}


PINDEX PSoundChannel::GetLastWriteCount() const
{
  return baseChannel != NULL ? baseChannel->GetLastWriteCount() : PChannel::GetLastWriteCount();
}


PBoolean PSoundChannel::PlaySound(const PSound & sound, PBoolean wait)
{
  PAssert(activeDirection == Player, PLogicError);
  return baseChannel != NULL && baseChannel->PlaySound(sound, wait);
}


PBoolean PSoundChannel::PlayFile(const PFilePath & file, PBoolean wait)
{
  PAssert(activeDirection == Player, PLogicError);
  return baseChannel != NULL && baseChannel->PlayFile(file, wait);
}


PBoolean PSoundChannel::HasPlayCompleted()
{
  PAssert(activeDirection == Player, PLogicError);
  return baseChannel != NULL && baseChannel->HasPlayCompleted();
}


PBoolean PSoundChannel::WaitForPlayCompletion() 
{
  PAssert(activeDirection == Player, PLogicError);
  return baseChannel != NULL && baseChannel->WaitForPlayCompletion();
}


PBoolean PSoundChannel::Read(void * buf, PINDEX len)
{
  PAssert(activeDirection == Recorder, PLogicError);

  if (len == 0)
    return IsOpen();

  return baseChannel != NULL && baseChannel->Read(buf, len);
}


PINDEX PSoundChannel::GetLastReadCount() const
{
  return baseChannel != NULL ? baseChannel->GetLastReadCount() : PChannel::GetLastReadCount();
}


PBoolean PSoundChannel::RecordSound(PSound & sound)
{
  PAssert(activeDirection == Recorder, PLogicError);
  return baseChannel != NULL && baseChannel->RecordSound(sound);
}


PBoolean PSoundChannel::RecordFile(const PFilePath & file)
{
  PAssert(activeDirection == Recorder, PLogicError);
  return baseChannel != NULL && baseChannel->RecordFile(file);
}


PBoolean PSoundChannel::StartRecording()
{
  PAssert(activeDirection == Recorder, PLogicError);
  return baseChannel != NULL && baseChannel->StartRecording();
}


PBoolean PSoundChannel::IsRecordBufferFull() 
{
  PAssert(activeDirection == Recorder, PLogicError);
  return baseChannel != NULL && baseChannel->IsRecordBufferFull();
}


PBoolean PSoundChannel::AreAllRecordBuffersFull() 
{
  PAssert(activeDirection == Recorder, PLogicError);
  return baseChannel != NULL && baseChannel->AreAllRecordBuffersFull();
}


PBoolean PSoundChannel::WaitForRecordBufferFull() 
{
  PAssert(activeDirection == Recorder, PLogicError);
  return baseChannel != NULL && baseChannel->WaitForRecordBufferFull();
}


PBoolean PSoundChannel::WaitForAllRecordBuffersFull() 
{
  PAssert(activeDirection == Recorder, PLogicError);
  return baseChannel != NULL && baseChannel->WaitForAllRecordBuffersFull();
}


///////////////////////////////////////////////////////////////////////////

#if !defined(_WIN32) && !defined(__BEOS__) && !defined(__APPLE__)

PSound::PSound(unsigned channels,
               unsigned samplesPerSecond,
               unsigned bitsPerSample,
               PINDEX   bufferSize,
               const BYTE * buffer)
{
  encoding = 0;
  numChannels = channels;
  sampleRate = samplesPerSecond;
  sampleSize = bitsPerSample;
  SetSize(bufferSize);
  if (buffer != NULL)
    memcpy(GetPointer(), buffer, bufferSize);
}


PSound::PSound(const PFilePath & filename)
{
  encoding = 0;
  numChannels = 1;
  sampleRate = 8000;
  sampleSize = 16;
  Load(filename);
}


PSound & PSound::operator=(const PBYTEArray & data)
{
  PBYTEArray::operator=(data);
  return *this;
}


void PSound::SetFormat(unsigned channels,
                       unsigned samplesPerSecond,
                       unsigned bitsPerSample)
{
  encoding = 0;
  numChannels = channels;
  sampleRate = samplesPerSecond;
  sampleSize = bitsPerSample;
  formatInfo.SetSize(0);
}


PBoolean PSound::Load(const PFilePath & /*filename*/)
{
  return PFalse;
}


PBoolean PSound::Save(const PFilePath & /*filename*/)
{
  return PFalse;
}


PBoolean PSound::Play()
{
  return Play(PSoundChannel::GetDefaultDevice(PSoundChannel::Player));
}


PBoolean PSound::Play(const PString & device)
{

  PSoundChannel channel(device, PSoundChannel::Player);
  if (!channel.IsOpen())
    return PFalse;

  return channel.PlaySound(*this, PTrue);
}


PBoolean PSound::PlayFile(const PFilePath & file, PBoolean wait)
{
  PSoundChannel channel(PSoundChannel::GetDefaultDevice(PSoundChannel::Player),
                        PSoundChannel::Player);
  if (!channel.IsOpen())
    return PFalse;

  return channel.PlayFile(file, wait);
}


#endif //_WIN32
