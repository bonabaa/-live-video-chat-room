/*
 * lidplugins.cxx
 *
 * Line Interface Device plugins manager
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (C) 2006 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 22444 $
 * $Author: rjongbloed $
 * $Date: 2009-04-20 23:49:06 +0000 (Mon, 20 Apr 2009) $
 */

#ifdef __GNUC__
#pragma implementation "lidpluginmgr.h"
#endif

#include <ptlib.h>

#include <opal/buildopts.h>

#include <lids/lidpluginmgr.h>
#include <ptclib/dtmf.h>


PFACTORY_CREATE(PFactory<PPluginModuleManager>, OpalPluginLIDManager, "OpalPluginLIDManager", true);

///////////////////////////////////////////////////////////////////////////////

OpalPluginLIDManager::OpalPluginLIDManager(PPluginManager * mgr)
  : PPluginModuleManager(PLUGIN_LID_GET_LIDS_FN_STR, mgr)
{
  // cause the plugin manager to load all dynamic plugins
  pluginMgr->AddNotifier(PCREATE_NOTIFIER(OnLoadModule), PTrue);
}


OpalPluginLIDManager::~OpalPluginLIDManager()
{
}


void OpalPluginLIDManager::OnLoadPlugin(PDynaLink & dll, INT code)
{
  PluginLID_GetDefinitionsFunction getDefinitions;
  {
    PDynaLink::Function fn;
    if (!dll.GetFunction(PString(signatureFunctionName), fn)) {
      PTRACE(3, "LID Plugin\tDLL " << dll.GetName() << " is not a plugin LID");
      return;
    }
    getDefinitions = (PluginLID_GetDefinitionsFunction)fn;
  }

  unsigned count;
  PluginLID_Definition * lid = (*getDefinitions)(&count, PWLIB_PLUGIN_API_VERSION);
  if (lid == NULL || count == 0) {
    PTRACE(3, "LID Plugin\tDLL " << dll.GetName() << " contains no LID definitions");
    return;
  } 

  PTRACE(3, "LID Plugin\tDLL " << dll.GetName() << " loaded " << count << "LID" << (count > 1 ? "s" : ""));

  while (count-- > 0) {
    if (lid->name != NULL && *lid->name != '\0') {
      switch (code) {
        case 0 : // plugin loaded
          m_registrations.Append(new OpalPluginLIDRegistration(*lid));
          break;

        case 1 : // plugin unloaded
          for (PList<OpalPluginLIDRegistration>::iterator iterLID = m_registrations.begin(); iterLID != m_registrations.end();) {
            if (*iterLID == lid->name)
              m_registrations.erase(iterLID++);
             else
               ++iterLID;
          }
      }
    }
    lid++;
  }
}


void OpalPluginLIDManager::OnShutdown()
{
  m_registrations.RemoveAll();
}


///////////////////////////////////////////////////////////////////////////////

OpalPluginLIDRegistration::OpalPluginLIDRegistration(const PluginLID_Definition & definition)
  : OpalLIDRegistration(definition.name)
  , m_definition(definition)
{
}


OpalLineInterfaceDevice * OpalPluginLIDRegistration::Create(void *) const
{
  return new OpalPluginLID(m_definition);
}


///////////////////////////////////////////////////////////////////////////////

OpalPluginLID::OpalPluginLID(const PluginLID_Definition & definition)
  : m_definition(definition)
  , m_tonePlayer(NULL)
  , m_lockOutTones(false)
{
  if (m_definition.Create != NULL) {
    m_context = definition.Create(&m_definition);
    PTRACE_IF(1, m_context == NULL, "LID Plugin\tNo context for " << m_definition.description); \
  }
  else {
    m_context = NULL;
    PTRACE(1, "LID Plugin\tDefinition for " << m_definition.description << " invalid.");
  }
}


OpalPluginLID::~OpalPluginLID()
{
  StopTone(0);
  if (m_context != NULL && m_definition.Destroy != NULL)
    m_definition.Destroy(&m_definition, m_context);
}


#if PTRACING

bool OpalPluginLID::BadContext() const
{
  if (m_context != NULL)
    return false;

  PTRACE(1, "LID Plugin\tNo context for " << m_definition.name);
  return true;
}

bool OpalPluginLID::BadFunction(void * fnPtr, const char * fnName) const
{
  if (fnPtr != NULL)
    return false;

  PTRACE(1, "LID Plugin\tFunction " << fnName << " not implemented in " << m_definition.name);
  return true;
}


static ostream & operator<<(ostream & stream, PluginLID_Errors error)
{
  static const char * const names[] = {
    "NoError",
    "UnimplementedFunction",
    "BadContext",
    "InvalidParameter",
    "NoSuchDevice",
    "DeviceOpenFailed",
    "UsesSoundChannel",
    "DeviceNotOpen",
    "NoSuchLine",
    "OperationNotAllowed",
    "NoMoreNames",
    "BufferTooSmall",
    "UnsupportedMediaFormat",
    "NoDialTone",
    "LineBusy",
    "NoAnswer",
    "Aborted",
    "InternalError"
  };

  if (error < PARRAYSIZE(names) && names[error] != NULL)
    stream << names[error];
  else
    stream << "Code " << (int)error;

  return stream;
}

PluginLID_Errors OpalPluginLID::CheckError(PluginLID_Errors error, const char * fnName) const
{
  if (error != PluginLID_NoError && error != PluginLID_UnimplementedFunction && error != PluginLID_NoMoreNames) {
    PTRACE(2, "LID Plugin\tFunction " << fnName << " in " << m_definition.name << " returned error " << error);
  }

  osError = error;
  return error;
}

#define BAD_FN(fn)         (BadContext() || BadFunction((void *)m_definition.fn, #fn))
#define CHECK_FN(fn, args) (BadContext() ? PluginLID_BadContext : m_definition.fn == NULL ? PluginLID_UnimplementedFunction : CheckError(m_definition.fn args, #fn))

#else // PTRACING

#define BAD_FN(fn) (m_context == NULL || m_definition.fn == NULL)
#define CHECK_FN(fn, args) (m_context == NULL ? PluginLID_BadContext : m_definition.fn == NULL ? PluginLID_UnimplementedFunction : (osError = m_definition.fn args))

#endif // PTRACING


PBoolean OpalPluginLID::Open(const PString & device)
{
  if (BAD_FN(Open))
    return PFalse;

  Close();


  switch (osError = m_definition.Open(m_context, device)) {
    case PluginLID_NoError :
      break;

    case PluginLID_UsesSoundChannel :
      {
        PString soundDevice;
        PINDEX backslash = device.Find('\\');
        if (backslash != P_MAX_INDEX)
          soundDevice = device.Mid(backslash+1);
        else
          soundDevice = device;

        if (!m_player.Open(soundDevice, PSoundChannel::Player)) {
          PTRACE(1, "LID Plugin\t" << m_definition.name << " requires sound system, but cannot open player for \"" << device << '"');
          return PFalse;
        }

        if (!m_recorder.Open(soundDevice, PSoundChannel::Recorder)) {
          PTRACE(1, "LID Plugin\t" << m_definition.name << " requires sound system, but cannot open recorder for \"" << device << '"');
          return PFalse;
        }
      }
      break;

    case PluginLID_NoSuchDevice :
      PTRACE(1, "LID Plugin\tNo such device as \"" << device << "\" in " << m_definition.name);
      return PFalse;

    default :
      PTRACE(1, "LID Plugin\tOpen of \"" << device << "\" in " << m_definition.name << " returned error " << osError);
      return PFalse;
  }

  m_deviceName = device;
  os_handle = 1;
  return PTrue;
}


PBoolean OpalPluginLID::Close()
{
  OpalLineInterfaceDevice::Close();

  StopTone(0);
  m_player.Close();
  m_recorder.Close();

  if (BAD_FN(Close))
    return PFalse;

  return m_definition.Close(m_context) == PluginLID_NoError;
}


PString OpalPluginLID::GetDeviceType() const
{
  return m_definition.name;
}


PString OpalPluginLID::GetDeviceName() const
{
  return m_deviceName;
}


PStringArray OpalPluginLID::GetAllNames() const
{
  PStringArray devices;

  char buffer[200];
  unsigned index = 0;
  while (CHECK_FN(GetDeviceName, (m_context, index++, buffer, sizeof(buffer))) == PluginLID_NoError)
    devices.AppendString(buffer);

  return devices;
}


PString OpalPluginLID::GetDescription() const
{
  return m_definition.description;
}


unsigned OpalPluginLID::GetLineCount() const
{
  unsigned count = 0;
  CHECK_FN(GetLineCount, (m_context, &count));
  return count;
}


PBoolean OpalPluginLID::IsLineTerminal(unsigned line)
{
  PluginLID_Boolean isTerminal = FALSE;
  CHECK_FN(IsLineTerminal, (m_context, line, &isTerminal));
  return isTerminal != FALSE;
}


PBoolean OpalPluginLID::IsLinePresent(unsigned line, PBoolean force)
{
  PluginLID_Boolean isPresent = FALSE;
  CHECK_FN(IsLinePresent, (m_context, line, force, &isPresent));
  return isPresent != FALSE;
}


PBoolean OpalPluginLID::IsLineOffHook(unsigned line)
{
  PluginLID_Boolean offHook = FALSE;
  CHECK_FN(IsLineOffHook, (m_context, line, &offHook));
  return offHook != FALSE;
}


PBoolean OpalPluginLID::SetLineOffHook(unsigned line, PBoolean newState)
{
  return CHECK_FN(SetLineOffHook, (m_context, line, newState)) == PluginLID_NoError;
}


PBoolean OpalPluginLID::HookFlash(unsigned line, unsigned flashTime)
{
  switch (CHECK_FN(HookFlash, (m_context, line, flashTime))) {
    case PluginLID_UnimplementedFunction :
      return OpalLineInterfaceDevice::HookFlash(line, flashTime);

    case PluginLID_NoError :
      return PTrue;

    default : ;
  }
  return PFalse;
}


PBoolean OpalPluginLID::HasHookFlash(unsigned line)
{
  PluginLID_Boolean flashed = FALSE;
  CHECK_FN(HasHookFlash, (m_context, line, &flashed));
  return flashed != FALSE;
}


PBoolean OpalPluginLID::IsLineRinging(unsigned line, DWORD * cadence)
{
  DWORD localCadence;
  if (cadence == NULL)
    cadence = &localCadence;

  if (CHECK_FN(IsLineRinging, (m_context, line, (unsigned long *)cadence)) == PluginLID_NoError)
    return *cadence != 0;

  return PFalse;
}


PBoolean OpalPluginLID::RingLine(unsigned line, PINDEX nCadence, const unsigned * pattern, unsigned frequency)
{
  PUnsignedArray cadence;

  if (nCadence > 0 && pattern == NULL) {
    PString description = m_callProgressTones[RingTone];
    PINDEX colon = description.Find(':');
    if (colon != P_MAX_INDEX) {
      unsigned newFrequency = description.Left(colon).AsUnsigned();
      if (newFrequency > 5 && newFrequency < 3000) {
        PStringArray times = description.Mid(colon+1).Tokenise('-');
        if (times.GetSize() > 1) {
          cadence.SetSize(times.GetSize());
          for (PINDEX i = 0; i < cadence.GetSize(); i++)
            cadence[i] = (unsigned)(times[i].AsReal()*1000);
          nCadence = cadence.GetSize();
          pattern = cadence;
          frequency = newFrequency;
        }
      }
    }
  }

  switch (CHECK_FN(RingLine, (m_context, line, nCadence, pattern, frequency))) {
    case PluginLID_UnimplementedFunction :
      if (nCadence > 0)
        return StartTonePlayerThread(RingTone+NumTones);
      StopTonePlayerThread();
      return true;

    case PluginLID_NoError :
      return true;

    default : ;
  }
  return PFalse;
}


PBoolean OpalPluginLID::SetLineConnected(unsigned line)
{
  switch (CHECK_FN(SetLineConnected, (m_context, line))) {
    case PluginLID_UnimplementedFunction :
      return OpalLineInterfaceDevice::SetLineConnected(line);

    case PluginLID_NoError :
      return true;

    default : ;
  }
  return PFalse;
}


PBoolean OpalPluginLID::IsLineConnected(unsigned line)
{
  PluginLID_Boolean connected = FALSE;
  switch (CHECK_FN(IsLineConnected, (m_context, line, &connected))) {
    case PluginLID_UnimplementedFunction :
      return OpalLineInterfaceDevice::IsLineConnected(line);

    case PluginLID_NoError :
      return connected != FALSE;

    default : ;
  }
  return PFalse;
}


PBoolean OpalPluginLID::IsLineDisconnected(unsigned line, PBoolean checkForWink)
{
  PluginLID_Boolean disconnected = FALSE;
  switch (CHECK_FN(IsLineDisconnected, (m_context, line, checkForWink, &disconnected))) {
    case PluginLID_UnimplementedFunction :
      return OpalLineInterfaceDevice::IsLineDisconnected(line, checkForWink);

    case PluginLID_NoError :
      return disconnected != FALSE;

    default : ;
  }
  return PFalse;
}


PBoolean OpalPluginLID::SetLineToLineDirect(unsigned line1, unsigned line2, PBoolean connect)
{
  return CHECK_FN(SetLineToLineDirect, (m_context, line1, line2, connect)) == PluginLID_NoError;
}


PBoolean OpalPluginLID::IsLineToLineDirect(unsigned line1, unsigned line2)
{
  PluginLID_Boolean connected = FALSE;
  CHECK_FN(IsLineToLineDirect, (m_context, line1, line2, &connected));
  return connected != FALSE;
}


OpalMediaFormatList OpalPluginLID::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  char buffer[100];
  unsigned index = 0;
  for (;;) {
    switch (CHECK_FN(GetSupportedFormat, (m_context, index++, buffer, sizeof(buffer)))) {
      case PluginLID_NoMoreNames :
        return formats;

      case PluginLID_UnimplementedFunction :
        formats += OPAL_PCM16;
        return formats;

      case PluginLID_NoError :
      {
        OpalMediaFormat format = buffer;
        if (format.IsEmpty()) {
          PTRACE(2, "LID Plugin\tCodec format \"" << buffer << "\" in " << m_definition.name << " is not supported by OPAL.");
        }
        else
          formats += format;
      }
      break;

      default : ;
    }
  }
}


PBoolean OpalPluginLID::SetReadFormat(unsigned line, const OpalMediaFormat & mediaFormat)
{
  switch (CHECK_FN(SetReadFormat, (m_context, line, mediaFormat))) {
    case PluginLID_UnimplementedFunction :
      return mediaFormat == OPAL_PCM16;

    case PluginLID_NoError :
      return PTrue;

    default : ;
  }
  return PFalse;
}


PBoolean OpalPluginLID::SetWriteFormat(unsigned line, const OpalMediaFormat & mediaFormat)
{
  switch (CHECK_FN(SetWriteFormat, (m_context, line, mediaFormat))) {
    case PluginLID_UnimplementedFunction :
      return mediaFormat == OPAL_PCM16;

    case PluginLID_NoError :
      return PTrue;

    default : ;
  }
  return PFalse;
}


OpalMediaFormat OpalPluginLID::GetReadFormat(unsigned line)
{
  char buffer[100];
  switch (CHECK_FN(GetReadFormat, (m_context, line, buffer, sizeof(buffer)))) {
    case PluginLID_UnimplementedFunction :
      return OPAL_PCM16;

    case PluginLID_NoError :
      return buffer;

    default : ;
  }
  return OpalMediaFormat();
}


OpalMediaFormat OpalPluginLID::GetWriteFormat(unsigned line)
{
  char buffer[100];
  switch (CHECK_FN(GetWriteFormat, (m_context, line, buffer, sizeof(buffer)))) {
    case PluginLID_UnimplementedFunction :
      return OPAL_PCM16;

    case PluginLID_NoError :
      return buffer;

    default : ;
  }
  return OpalMediaFormat();
}


PBoolean OpalPluginLID::StopReading(unsigned line)
{
  OpalLineInterfaceDevice::StopReading(line);

  switch (CHECK_FN(StopReading, (m_context, line))) {
    case PluginLID_UnimplementedFunction :
      return m_recorder.Abort();

    case PluginLID_NoError :
      return PTrue;

    default : ;
  }
  return PFalse;
}


PBoolean OpalPluginLID::StopWriting(unsigned line)
{
  OpalLineInterfaceDevice::StopWriting(line);

  m_lockOutTones = false;

  switch (CHECK_FN(StopWriting, (m_context, line))) {
    case PluginLID_UnimplementedFunction :
      return m_player.Abort();

    case PluginLID_NoError :
      return PTrue;

    default : ;
  }
  return PFalse;
}


PBoolean OpalPluginLID::SetReadFrameSize(unsigned line, PINDEX frameSize)
{
  switch (CHECK_FN(SetReadFrameSize, (m_context, line, frameSize))) {
    case PluginLID_UnimplementedFunction :
      return m_recorder.SetBuffers(frameSize, 2000/frameSize+2); // Want about 250ms of buffering

    case PluginLID_NoError :
      return PTrue;

    default : ;
  }
  return PFalse;
}


PBoolean OpalPluginLID::SetWriteFrameSize(unsigned line, PINDEX frameSize)
{
  switch (CHECK_FN(SetWriteFrameSize, (m_context, line, frameSize))) {
    case PluginLID_UnimplementedFunction :
      m_lockOutTones = true;
      StopTone(line);
      return m_player.SetBuffers(frameSize, 1000/frameSize+2); // Want about 125ms of buffering

    case PluginLID_NoError :
      return PTrue;

   default : ;
  }
  return PFalse;
}


PINDEX OpalPluginLID::GetReadFrameSize(unsigned line)
{
  unsigned frameSize = 0;
  switch (CHECK_FN(GetReadFrameSize, (m_context, line, &frameSize))) {
    case PluginLID_NoError :
      return frameSize;

    case PluginLID_UnimplementedFunction :
      PINDEX size, buffers;
      return m_recorder.GetBuffers(size, buffers) ? size : 0;

      default : ;
  }
  return 0;
}


PINDEX OpalPluginLID::GetWriteFrameSize(unsigned line)
{
  unsigned frameSize = 0;
  switch (CHECK_FN(GetWriteFrameSize, (m_context, line, &frameSize))) {
    case PluginLID_NoError :
      return frameSize;

    case PluginLID_UnimplementedFunction :
      PINDEX size, buffers;
      return m_player.GetBuffers(size, buffers) ? size : 0;

      default : ;
  }
  return 0;
}


PBoolean OpalPluginLID::ReadFrame(unsigned line, void * buffer, PINDEX & count)
{
  unsigned uiCount = 0;
  switch (CHECK_FN(ReadFrame, (m_context, line, buffer, &uiCount))) {
    case PluginLID_UnimplementedFunction :
      count = GetReadFrameSize(line);
      if (!m_recorder.Read(buffer, count))
        return PFalse;
      count = m_recorder.GetLastReadCount();
      return PTrue;

    case PluginLID_NoError :
      count = uiCount;
      return PTrue;

   default : ;
  }
  return PFalse;
}


PBoolean OpalPluginLID::WriteFrame(unsigned line, const void * buffer, PINDEX count, PINDEX & written)
{
  StopTone(line);
  m_lockOutTones = true;

  unsigned uiCount = 0;
  switch (CHECK_FN(WriteFrame, (m_context, line, buffer, count, &uiCount))) {
    case PluginLID_UnimplementedFunction :
      if (!m_player.Write(buffer, count))
        return PFalse;
      written = m_player.GetLastWriteCount();
      return PTrue;

    case PluginLID_NoError :
      written = uiCount;
      return PTrue;

    default : ;
  }
  return PFalse;
}


unsigned OpalPluginLID::GetAverageSignalLevel(unsigned line, PBoolean playback)
{
  unsigned signal = UINT_MAX;
  CHECK_FN(GetAverageSignalLevel, (m_context, line, playback, &signal));
  return signal;
}


PBoolean OpalPluginLID::EnableAudio(unsigned line, PBoolean enable)
{
  switch (CHECK_FN(EnableAudio, (m_context, line, enable))) {
    case PluginLID_UnimplementedFunction :
      return OpalLineInterfaceDevice::EnableAudio(line, enable);

    case PluginLID_NoError :
      return PTrue;

    default : ;
  }
  return PFalse;
}


PBoolean OpalPluginLID::IsAudioEnabled(unsigned line) const
{
  PluginLID_Boolean enabled = FALSE;
  if (CHECK_FN(IsAudioEnabled, (m_context, line, &enabled)) == PluginLID_UnimplementedFunction)
    return OpalLineInterfaceDevice::IsAudioEnabled(line);
  return enabled != FALSE;
}


PBoolean OpalPluginLID::SetRecordVolume(unsigned line, unsigned volume)
{
  switch (CHECK_FN(SetRecordVolume, (m_context, line, volume))) {
    case PluginLID_UnimplementedFunction :
      return m_recorder.SetVolume(volume);

    case PluginLID_NoError :
      return PTrue;

    default : ;
  }
  return PFalse;
}


PBoolean OpalPluginLID::SetPlayVolume(unsigned line, unsigned volume)
{
  switch (CHECK_FN(SetPlayVolume, (m_context, line, volume))) {
    case PluginLID_UnimplementedFunction :
      return m_player.SetVolume(volume);

    case PluginLID_NoError :
      return PTrue;

    default : ;
  }
  return PFalse;
}


PBoolean OpalPluginLID::GetRecordVolume(unsigned line, unsigned & volume)
{
  switch (CHECK_FN(GetRecordVolume, (m_context, line, &volume))) {
    case PluginLID_UnimplementedFunction :
      return m_recorder.GetVolume(volume);

    case PluginLID_NoError :
      return PTrue;

    default : ;
  }
  return PFalse;
}


PBoolean OpalPluginLID::GetPlayVolume(unsigned line, unsigned & volume)
{
  switch (CHECK_FN(GetPlayVolume, (m_context, line, &volume))) {
    case PluginLID_UnimplementedFunction :
      return m_player.GetVolume(volume);

    case PluginLID_NoError :
      return PTrue;

      default:;

  }
  return PFalse;
}


OpalLineInterfaceDevice::AECLevels OpalPluginLID::GetAEC(unsigned line) const
{
  unsigned level = AECError;
  CHECK_FN(GetAEC, (m_context, line, &level));
  return (AECLevels)level;
}


PBoolean OpalPluginLID::SetAEC(unsigned line, AECLevels level)
{
  return CHECK_FN(SetAEC, (m_context, line, level)) == PluginLID_NoError;
}


PBoolean OpalPluginLID::GetVAD(unsigned line) const
{
  PluginLID_Boolean vad = FALSE;
  CHECK_FN(GetVAD, (m_context, line, &vad));
  return vad != FALSE;
}


PBoolean OpalPluginLID::SetVAD(unsigned line, PBoolean enable)
{
  return CHECK_FN(SetVAD, (m_context, line, enable)) == PluginLID_NoError;
}


PBoolean OpalPluginLID::GetCallerID(unsigned line, PString & idString, PBoolean full)
{
  return CHECK_FN(GetCallerID, (m_context, line, idString.GetPointer(500), 500, full)) == PluginLID_NoError;
}


PBoolean OpalPluginLID::SetCallerID(unsigned line, const PString & idString)
{
  if (idString.IsEmpty())
    return PFalse;

  return CHECK_FN(SetCallerID, (m_context, line, idString)) == PluginLID_NoError;
}


PBoolean OpalPluginLID::SendVisualMessageWaitingIndicator(unsigned line, PBoolean on)
{
  return CHECK_FN(SendVisualMessageWaitingIndicator, (m_context, line, on)) == PluginLID_NoError;
}


PBoolean OpalPluginLID::PlayDTMF(unsigned line, const char * digits, DWORD onTime, DWORD offTime)
{
  return CHECK_FN(PlayDTMF, (m_context, line, digits, onTime, offTime)) == PluginLID_NoError;
}


char OpalPluginLID::ReadDTMF(unsigned line)
{
  char dtmf = '\0';
  CHECK_FN(ReadDTMF, (m_context, line, &dtmf));
  return dtmf;
}


PBoolean OpalPluginLID::GetRemoveDTMF(unsigned line)
{
  PluginLID_Boolean remove = FALSE;
  CHECK_FN(GetRemoveDTMF, (m_context, line, &remove));
  return remove != FALSE;
}


PBoolean OpalPluginLID::SetRemoveDTMF(unsigned line, PBoolean removeTones)
{
  return CHECK_FN(SetRemoveDTMF, (m_context, line, removeTones)) == PluginLID_NoError;
}


OpalLineInterfaceDevice::CallProgressTones OpalPluginLID::IsToneDetected(unsigned line)
{
  int tone = NoTone;
  CHECK_FN(IsToneDetected, (m_context, line, &tone));
  return (CallProgressTones)tone;
}


OpalLineInterfaceDevice::CallProgressTones OpalPluginLID::WaitForToneDetect(unsigned line, unsigned timeout)
{
  int tone = NoTone;
  if (CHECK_FN(WaitForToneDetect, (m_context, line, timeout, &tone)) == PluginLID_UnimplementedFunction)
    return OpalLineInterfaceDevice::WaitForToneDetect(line, timeout);
  return (CallProgressTones)tone;
}


PBoolean OpalPluginLID::WaitForTone(unsigned line, CallProgressTones tone, unsigned timeout)
{
  switch (CHECK_FN(WaitForTone, (m_context, line, tone, timeout))) {
    case PluginLID_UnimplementedFunction :
      return OpalLineInterfaceDevice::WaitForTone(line, tone, timeout);

    case PluginLID_NoError :
      return PTrue;

    default:
      break;
  }

  return PFalse;
}


bool OpalPluginLID::SetToneParameters(unsigned line,
                                      CallProgressTones tone,
                                      unsigned frequency1,
                                      unsigned frequency2,
                                      ToneMixingModes mode,
                                      PINDEX numCadences,
                                      const unsigned * onTimes,
                                      const unsigned * offTimes)
{
  return CHECK_FN(SetToneParameters, (m_context, line, tone, frequency1, frequency2, mode, numCadences, onTimes, offTimes)) == PluginLID_NoError;
}


void OpalPluginLID::TonePlayer(PThread &, INT tone)
{
  // CHeck if we have NumTones added to value which means high volume output
  // typically if handset has no ringer, then just hammer the speaker.
  bool highVolume = tone > NumTones;
  if (highVolume)
    tone -= NumTones;

  if (!PAssert(tone < NumTones, PInvalidParameter))
    return;

  PTRACE(4, "LID Plugin\tStarting manual tone generation for \"" << m_callProgressTones[tone] << '"');

  // Get old volume level, if can't then do not do high volume
  unsigned oldVolume;
  if (!m_player.GetVolume(oldVolume))
    highVolume = false;

  // Max out the volume level for player
  if (highVolume)
    m_player.SetVolume(100);

  PTones toneData;
  if (toneData.Generate(m_callProgressTones[tone])) {
    while (!m_stopTone.Wait(0)) {
      if (!m_player.Write(toneData, toneData.GetSize()*2)) {
        PTRACE(2, "LID Plugin\tTone generation write failed.");
        break;
      }
    }
  }
  else {
    PTRACE(2, "LID Plugin\tTone generation for \"" << m_callProgressTones[tone] << "\"failed.");
  }

  m_player.Abort();

  // If we adjusted the volume, then put it back
  if (highVolume)
    m_player.SetVolume(oldVolume);

  PTRACE(4, "LID Plugin\tEnded manual tone generation for \"" << m_callProgressTones[tone] << '"');
}


bool OpalPluginLID::StartTonePlayerThread(int tone)
{
  StopTonePlayerThread();

  // Clear out extraneous signals
  while (m_stopTone.Wait(0))
    ;

  // Start new tone thread
  m_tonePlayer = PThread::Create(PCREATE_NOTIFIER(TonePlayer), tone, PThread::NoAutoDeleteThread, PThread::NormalPriority, "TonePlayer");
  return m_tonePlayer != NULL;
}


void OpalPluginLID::StopTonePlayerThread()
{
  // Stop previous tone, if running
  if (m_tonePlayer != NULL) {
    m_stopTone.Signal();
    m_tonePlayer->WaitForTermination(1000);
    delete m_tonePlayer;
    m_tonePlayer = NULL;
  }
}


PBoolean OpalPluginLID::PlayTone(unsigned line, CallProgressTones tone)
{
  if (m_lockOutTones)
    return StopTone(line);

  switch (CHECK_FN(PlayTone, (m_context, line, tone))) {
    case PluginLID_UnimplementedFunction :
      return StartTonePlayerThread(tone);

    case PluginLID_NoError :
      return PTrue;

    default:
      break;
  }

  return PFalse;
}


PBoolean OpalPluginLID::IsTonePlaying(unsigned line)
{
  PluginLID_Boolean playing = FALSE;
  if (m_tonePlayer == NULL || m_tonePlayer->IsTerminated())
    CHECK_FN(IsTonePlaying, (m_context, line, &playing));
  return playing != FALSE;
}


PBoolean OpalPluginLID::StopTone(unsigned line)
{
  StopTonePlayerThread();

  switch (CHECK_FN(StopTone, (m_context, line))) {
    case PluginLID_UnimplementedFunction :
    case PluginLID_NoError :
      return PTrue;
    default:
      break;
  }

  return false;
}


OpalLineInterfaceDevice::CallProgressTones OpalPluginLID::DialOut(unsigned line, const PString & number, const DialParams & params)
{
  if (m_definition.DialOut == NULL)
    return OpalLineInterfaceDevice::DialOut(line, number, params);

  if (BAD_FN(DialOut))
    return NoTone;

  struct PluginLID_DialParams pparams;
  pparams.m_requireTones = params.m_requireTones;
  pparams.m_dialToneTimeout = params.m_dialToneTimeout;
  pparams.m_dialStartDelay = params.m_dialStartDelay;
  pparams.m_progressTimeout = params.m_progressTimeout;
  pparams.m_commaDelay = params.m_commaDelay;

  osError = m_definition.DialOut(m_context, line, number, &pparams);
  switch (osError)
  {
    case PluginLID_NoError :
      return RingTone;
    case PluginLID_NoDialTone :
      return DialTone;
    case PluginLID_LineBusy :
      return BusyTone;
    case PluginLID_NoAnswer :
      return ClearTone;
#if PTRACING
    default :
      CheckError((PluginLID_Errors)osError, "DialOut");
#endif
  }

  return NoTone;
}


unsigned OpalPluginLID::GetWinkDuration(unsigned line)
{
  unsigned duration = 0;
  CHECK_FN(GetWinkDuration, (m_context, line, &duration));
  return duration;
}


PBoolean OpalPluginLID::SetWinkDuration(unsigned line, unsigned winkDuration)
{
  return CHECK_FN(SetWinkDuration, (m_context, line, winkDuration)) == PluginLID_NoError;
}


PBoolean OpalPluginLID::SetCountryCode(T35CountryCodes country)
{
  switch (CHECK_FN(SetCountryCode, (m_context, country))) {
    case PluginLID_UnimplementedFunction :
      return OpalLineInterfaceDevice::SetCountryCode(country);

    case PluginLID_NoError :
      return PTrue;

    default:
      break;
  }

  return PFalse;
}


PStringList OpalPluginLID::GetCountryCodeNameList() const
{
  PStringList countries;

  unsigned index = 0;
  for (;;) {
    unsigned countryCode = NumCountryCodes;
    switch (CHECK_FN(GetSupportedCountry, (m_context, index++, &countryCode))) {
      case PluginLID_UnimplementedFunction :
        return OpalLineInterfaceDevice::GetCountryCodeNameList();

      case PluginLID_NoError :
        if (countryCode < NumCountryCodes)
          countries.AppendString(GetCountryCodeName((T35CountryCodes)countryCode));
        break;

      case PluginLID_NoMoreNames :
        return countries;

      default :
        return PStringList();
    }
  }
}

