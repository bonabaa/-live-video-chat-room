/*
 * ixjunix.cxx
 *
 * QuickNet Internet Phone/Line JACK codec interface
 *
 * Open H323 Library
 *
 * Copyright (c) 1999-2000 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions of this code were written with the assisance of funding from 
 * Quicknet Technologies, Inc. http://www.quicknet.net.
 * 
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21004 $
 * $Author: rjongbloed $
 * $Date: 2008-09-16 07:08:56 +0000 (Tue, 16 Sep 2008) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "ixjlid.h"
#endif

#include <opal/buildopts.h>

#include <lids/ixjlid.h>

#ifdef HAS_IXJ

#include <sys/time.h>
#include <math.h>

#define new PNEW

#ifndef TELEPHONY_VERSION
#if   !defined(PHONE_VAD)
#warning Using extremely old telephony.h, please upgrade!
#define TELEPHONY_VERSION 1000  // Version in 2.2.16 kernel
#elif !defined(IXJCTL_VMWI)
#warning Using very old telephony.h, please upgrade!
#define TELEPHONY_VERSION 2000  // Version in 2.2.18 kernel
#else
#warning Using old telephony.h, please upgrade!
#define TELEPHONY_VERSION 3000  // Version in CVS before addition of TELEPHONY_VERSION
#endif
#endif


#define	IsLineJACK()	(dwCardType == LineJACK)

#define	FLASH_TIME	1000
#define	MANUAL_FLASH		// undefine to use FLASH exception

#ifdef P_LINUX

#ifdef _DEBUG

static int traced_ioctl(const char * str, int fd, int code)
{
  PTRACE(6,"IXJ\tIOCTL(" << fd << ", " << str << ")");
  int val = ::ioctl(fd,code);
  PTRACE(6,"IXJ\tIOCTL value = " << val);
  return val;
}

static int traced_ioctl(const char * str, int fd, int code , unsigned long arg)
{
  PTRACE(6,"IXJ\tIOCTL(" << fd << ", " << str << ", " << (void *)arg << ")");
  int val = ::ioctl(fd,code,arg);
  PTRACE(6,"IXJ\tIOCTL value = " << val);
  return val;
}

#define	IOCTL(fd,code)		traced_ioctl(#code, fd, code)
#define	IOCTL2(fd,code,arg)	traced_ioctl(#code, fd, code, (unsigned long)(arg))
#define	IOCTLP(fd,code,arg)	::ioctl(fd,code,arg)

#else

#define	IOCTL(fd,code)		::ioctl(fd,code)
#define	IOCTL2(fd,code,arg)	::ioctl(fd,code,arg)
#define	IOCTLP(fd,code,arg)	::ioctl(fd,code,arg)

#endif
#endif  // P_LINUX

#if TELEPHONY_VERSION < 3013
#define G729B 13
#endif


#ifdef P_FREEBSD
// BSD does not support return values from the ioctl() call
// except via the 3rd parameter.
// Also the 3rd parameter must be the 'address' of the data.

#ifdef _DEBUG

static int traced_bsd_ioctl(const char * str, int fd, int code , unsigned long arg = 0)
{
  int val = arg;
  int ret;
  PTRACE(6,"IXJ\tIOCTL(" << fd << ", " << str << ", " << (void *)arg << ")");
  ret = ::ioctl(fd,code, &arg);
  PTRACE(6,"IXJ\tIOCTL value = " << val);
  return ret;
}

#define	IOCTL(fd,code)		traced_bsd_ioctl(#code, fd, code)
#define	IOCTL2(fd,code,arg)	traced_bsd_ioctl(#code, fd, code, (unsigned long)(arg))
#define	IOCTLP(fd,code,arg)	::ioctl(fd,code,arg)

#else

static int bsd_ioctl(int fd, int code , unsigned long arg = 0)
{
  int val = arg;
  int ret;
  ret = ::ioctl(fd,code, &val);
  return ret;
}

#define	IOCTL(fd,code)		bsd_ioctl(fd,code,0)
#define	IOCTL2(fd,code,arg)	bsd_ioctl(fd,code,(unsigned long)(arg))
#define	IOCTLP(fd,code,arg)	::ioctl(fd,code,arg)

#endif
#endif  // P_FREEBSD

OpalIxJDevice::ExceptionInfo OpalIxJDevice::exceptionInfo[OpalIxJDevice::MaxIxjDevices];
PMutex                       OpalIxJDevice::exceptionMutex;
PBoolean                         OpalIxJDevice::exceptionInit = PFalse;

/////////////////////////////////////////////////////////////////////////////

/*

struct phone_except {
        unsigned int dtmf_ready:1;
        unsigned int hookstate:1;
        unsigned int flash:1;
        unsigned int pstn_ring:1;
        unsigned int caller_id:1;
        unsigned int pstn_wink:1;
        unsigned int f0:1;
        unsigned int f1:1;
        unsigned int f2:1;
        unsigned int f3:1;
        unsigned int fc0:1;
        unsigned int fc1:1;
        unsigned int fc2:1;
        unsigned int fc3:1;
        unsigned int reserved:18;
};

union telephony_exception {
        struct phone_except bits;
        unsigned int bytes;
};

*/

void OpalIxJDevice::SignalHandler(int sig)
{
  // construct list of fds to check
  fd_set  efds;
  FD_ZERO(&efds);
  PINDEX i;
  int maxHandle = 0;
  for (i = 0; i < MaxIxjDevices; i++) 
    if (exceptionInfo[i].fd >= 0) {
      FD_SET(exceptionInfo[i].fd, &efds);
      if (exceptionInfo[i].fd > maxHandle)
        maxHandle = exceptionInfo[i].fd;
    }

  // do not delay
  struct timeval  tv;
  tv.tv_sec = tv.tv_usec = 0;

  // get exception status
  int stat = select(maxHandle+1, NULL, NULL, &efds, &tv);

  // check for exceptions
  if (stat > 0) {
    for (i = 0; i < MaxIxjDevices; i++) {
      if ((exceptionInfo[i].fd >= 0) && FD_ISSET(exceptionInfo[i].fd, &efds)) {

        ExceptionInfo & info = exceptionInfo[i];
        int fd                     = info.fd;
        telephony_exception & data = info.data;
        data.bytes = IOCTL(fd, PHONE_EXCEPTION);

        if (data.bits.dtmf_ready) {
          //printf("dtmf\n");
          char ch = IOCTL(fd, PHONE_GET_DTMF_ASCII);
          int p = info.dtmfIn;
          info.dtmf[p] = ch;
          p = (p + 1) % 16;
          if (p != info.dtmfOut)
            info.dtmfIn = p;
        }

        if (data.bits.pstn_ring) 
          info.hasRing = PTrue;

        if (data.bits.hookstate) {
          PBoolean newHookState = (IOCTL(fd, PHONE_HOOKSTATE) & 1) != 0;
#ifdef MANUAL_FLASH
          if (newHookState != info.hookState) {
            timeval now;
            gettimeofday(&now, NULL);
            long diff = (now.tv_sec - info.lastHookChange.tv_sec) * 1000000;
            diff += now.tv_usec - info.lastHookChange.tv_usec;
            diff = (diff + 500) / 1000;
            if (newHookState && (diff < FLASH_TIME))
              info.hasFlash = PTrue;
            info.lastHookChange = now;
          }
#endif
          info.hookState = newHookState;
        }

#ifndef MANUAL_FLASH
        if (data.bits.flash) {
          info.hasFlash = PTrue;
          //printf("flash detected\n");
        }
#endif

        if (data.bits.pstn_wink)
          info.hasWink = PTrue;

        if (data.bits.f0) {
          //printf("Filter 0 trigger\n");
          info.filter[0] = PTrue;
        }
        if (data.bits.f1) {
          //printf("Filter 0 trigger\n");
          info.filter[1] = PTrue;
        }
        if (data.bits.f2) {
          //printf("Filter 0 trigger\n");
          info.filter[2] = PTrue;
        }
        if (data.bits.f3) {
          //printf("Filter 0 trigger\n");
          info.filter[3] = PTrue;
        }

#if TELEPHONY_VERSION >= 2000
        if (data.bits.fc0) {
          //printf("Cadence 0 trigger\n");
          info.cadence[0] = PTrue;
        }
        if (data.bits.fc1) {
          //printf("Cadence 1 trigger\n");
          info.cadence[1] = PTrue;
        }
        if (data.bits.fc2) {
          //printf("Cadence 2 trigger\n");
          info.cadence[2] = PTrue;
        }
        if (data.bits.fc3) {
          //printf("Cadence 3 trigger\n");
          info.cadence[3] = PTrue;
        }
#endif

#if TELEPHONY_VERSION >= 3000
        if (data.bits.caller_id) {
          ::ioctl(fd, IXJCTL_CID, &exceptionInfo[i].cid);
          info.hasCid = PTrue;
          //printf("caller ID signal\n");
        }
#endif
      }
    }
  }

  signal(SIGIO, &OpalIxJDevice::SignalHandler);
}

/////////////////////////////////////////////////////////////////////////////

OpalIxJDevice::OpalIxJDevice()
{
  os_handle = -1;
  readStopped = writeStopped = PTrue;
  readFrameSize = writeFrameSize = 480;  // 30 milliseconds of 16 bit PCM data
  readCodecType = writeCodecType = P_MAX_INDEX;
  currentHookState = lastHookState = PFalse;
  inRawMode = PFalse;
  enabledAudioLine = UINT_MAX;
  exclusiveAudioMode = PTrue;
  aecLevel = AECOff;
  tonePlaying = PFalse;
  removeDTMF = PFalse;
#if TELEPHONY_VERSION >= 3000
  memset(&callerIdInfo, 0, sizeof(callerIdInfo));
#endif
}


PBoolean OpalIxJDevice::Open(const PString & device)
{
  Close();

  // initialise the exception information, if required
  {
    PWaitAndSignal m(exceptionMutex);
    if (!exceptionInit) {
      PINDEX i;
      for (i = 0; i < MaxIxjDevices; i++)
        exceptionInfo[i].fd = -1;
      exceptionInit = PTrue;
    }
  }

  if (isdigit(device[0])) 
    deviceName = psprintf("/dev/phone%u", device.AsUnsigned());
  else {
    PINDEX pos = device.FindLast(' ');
    if (pos == P_MAX_INDEX)
      deviceName = device;
    else
      deviceName = device.Mid(pos+1).Trim();
  }

  int new_handle = os_handle = ::open(deviceName, O_RDWR);
  if (!ConvertOSError(new_handle))
    return PFalse;
  currentHookState = lastHookState = 
    (IOCTL(os_handle, PHONE_HOOKSTATE) != 0);

  // add the new handle to the exception info
  {
    PWaitAndSignal m(exceptionMutex);
    PINDEX i;
    for (i = 0; i < MaxIxjDevices; i++) 
      if (exceptionInfo[i].fd < 0) 
        break;
    PAssert(i < MaxIxjDevices, "too many IXJ devices open");

    ExceptionInfo & info = exceptionInfo[i];
    memset(&info, 0, sizeof(info));

    info.fd  = os_handle;
    info.hookState  = currentHookState;
    info.hasRing    = PFalse;
    info.hasWink    = PFalse;
    info.hasFlash   = PFalse;
    timerclear(&info.lastHookChange);

#if TELEPHONY_VERSION >= 3000
    info.hasCid     = PFalse;
#endif
    for (i = 0; i < 4; i++) {
      info.cadence[i] = PFalse;
      info.filter[i]  = PFalse;
    }

#ifdef IXJCTL_SIGCTL
    // enable all events except read/write
    IXJ_SIGDEF sigdef;

    sigdef.signal = SIGIO;
    sigdef.event = SIG_DTMF_READY; IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
    sigdef.event = SIG_HOOKSTATE;  IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
    sigdef.event = SIG_PSTN_RING;  IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
    sigdef.event = SIG_CALLER_ID;  IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
    sigdef.event = SIG_PSTN_WINK;  IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
    sigdef.event = SIG_F0;         IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
    sigdef.event = SIG_F1;         IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
    sigdef.event = SIG_F2;         IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
    sigdef.event = SIG_F3;         IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
    sigdef.event = SIG_FC0;        IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
    sigdef.event = SIG_FC1;        IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
    sigdef.event = SIG_FC2;        IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
    sigdef.event = SIG_FC3;        IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
#ifndef MANUAL_FLASH
    sigdef.event = SIG_FLASH;       IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
#endif

    sigdef.signal = 0;
    sigdef.event  = SIG_READ_READY;  IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
    sigdef.event  = SIG_WRITE_READY; IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
#ifdef MANUAL_FLASH
    sigdef.event = SIG_FLASH;       IOCTLP(os_handle, IXJCTL_SIGCTL, &sigdef);
#endif
#endif

    fcntl(os_handle, F_SETOWN, getpid());
    int f = fcntl(os_handle, F_GETFL);
    fcntl(os_handle, F_SETFL, f | FASYNC);
    signal(SIGIO, &OpalIxJDevice::SignalHandler);
  }

  os_handle = new_handle;

  // determine if the card is a phonejack or linejack
  dwCardType = IOCTL(os_handle, IXJCTL_CARDTYPE);
  switch (dwCardType) {
  case 3:
    dwCardType = PhoneJACK_PCI_TJ;
    break;
  case 0x100: 
  case 100:
    dwCardType = PhoneJACK;
    break;
  case 0x300:
  case 300:
    dwCardType = LineJACK;
    break;
  case 0x400:
  case 400:
    dwCardType = PhoneJACK_Lite;
    break;
  case 0x500:
  case 500: 
    dwCardType = PhoneJACK_PCI;
    break;
  case 0x600:
  case 600:
    dwCardType = PhoneCARD;
  }

  char * str = ::getenv("IXJ_COUNTRY");
  if (str != NULL) {
    if (isdigit(*str))
      SetCountryCode((T35CountryCodes)atoi(str));
    else
      SetCountryCodeName(PString(str));
  }

  // make sure the PSTN line is on-hook
  pstnIsOffHook = PFalse;
  gotWink       = PFalse;
  IOCTL2(os_handle, PHONE_PSTN_SET_STATE, PSTN_ON_HOOK);

  inRawMode     = PFalse;

  SetAEC         (0, AECOff);
  SetRecordVolume(0, 100);
  SetPlayVolume  (0, 100);

  return PTrue;
}

PBoolean OpalIxJDevice::Close()
{
  if (!IsOpen())
    return PFalse;

  StopReadCodec(0);
  StopWriteCodec(0);
  RingLine(0, 0);
  SetLineToLineDirect(0, 1, PTrue);
  deviceName = PString();

  // close the device
  int stat = ::close(os_handle);

  // remove the device from the exception information
  {
    PWaitAndSignal m(exceptionMutex);
    ExceptionInfo * info = GetException();
    info->fd = -1;
  }

  os_handle = -1;

  return ConvertOSError(stat);
}


PString OpalIxJDevice::GetDeviceType() const
{
  return OPAL_IXJ_TYPE_NAME;
}


PString OpalIxJDevice::GetDeviceName() const
{
  return deviceName;
}

PString OpalIxJDevice::GetDescription() const
{
  PStringStream name;

  name << "Internet ";

  switch (dwCardType) {
    case 0 :
    case 1 :
      name << "PhoneJACK";
      break;
    case 3 :
      name << "LineJACK";
      break;
    case 4 :
      name << "PhoneJACK-Lite";
      break;
    case 5 :
      name << "PhoneJACK-PCI";
      break;
    case 6 :
      name << "PhoneCARD";
      break;
    default :
      name << "xJACK";
  }

  name << " (" << deviceName << ')';

  return name;
}

PStringArray OpalIxJDevice::GetAllNames() const
{
  return GetDeviceNames();
}

unsigned OpalIxJDevice::GetLineCount()
{
  return 1;     
  /*Each card has just one line that can be active. 
    Consequently, return (IsLineJACK() ? NumLines : 1); is wrong.*/
}

PBoolean OpalIxJDevice::IsLinePresent(unsigned line, PBoolean /* force */)
{
  if (line != PSTNLine)
    return PFalse;

  PBoolean stat = IOCTL(os_handle, IXJCTL_PSTN_LINETEST) == 1;
  PThread::Sleep(2000);

  // clear ring signal status
  IsLineRinging(line);

  return stat;
}


PBoolean OpalIxJDevice::IsLineOffHook(unsigned line)
{
  if (line == PSTNLine) 
    return pstnIsOffHook;
  
  PWaitAndSignal m(exceptionMutex);
  ExceptionInfo * info = GetException();

#ifdef MANUAL_FLASH
  if (info->hookState != lastHookState) {
    lastHookState = info->hookState;
    if (lastHookState) {
	currentHookState = lastHookState;
    } else {
	hookTimeout = FLASH_TIME;
    }
  } else if (!hookTimeout.IsRunning() && (currentHookState != info->hookState)) 
    currentHookState = info->hookState;

  return currentHookState;
#else
  return info->hookState;
#endif
}

PBoolean OpalIxJDevice::HasHookFlash(unsigned line)
{ 
  if (line != POTSLine)
    return PFalse;
  
  PWaitAndSignal m(exceptionMutex);
  ExceptionInfo * info = GetException();
  
  PBoolean flash = info->hasFlash;
  info->hasFlash = PFalse;
  return flash;
}


PBoolean OpalIxJDevice::SetLineOffHook(unsigned line, PBoolean newState)
{
  if (line == POTSLine) {
#ifdef PHONE_WINK
    IOCTL(os_handle, PHONE_WINK);
    return PTrue;
#else
    return PFalse;
#endif
  }

  pstnIsOffHook = newState;

  if (!pstnIsOffHook) {
    StopReadCodec(line);
    StopWriteCodec(line);
  }

  // reset wink detected state going on or off hook 
  gotWink = PFalse;

  IOCTL2(os_handle, PHONE_PSTN_SET_STATE, pstnIsOffHook ? PSTN_OFF_HOOK : PSTN_ON_HOOK);

  return PTrue;
}

OpalIxJDevice::ExceptionInfo * OpalIxJDevice::GetException()
{
  PINDEX i;
  for (i = 0; i < MaxIxjDevices; i++) 
    if (exceptionInfo[i].fd == os_handle) 
      return &exceptionInfo[i];

  PAssertAlways("Cannot find open device in exception list");
  return NULL;
}


PBoolean OpalIxJDevice::IsLineRinging(unsigned line, DWORD * /*cadence*/)
{
  if (line != PSTNLine)
    return PFalse;

  PWaitAndSignal m(exceptionMutex);
  ExceptionInfo * info = GetException();

  PBoolean ring = info->hasRing;
  info->hasRing = PFalse;
  return ring;
}


PBoolean OpalIxJDevice::RingLine(unsigned line, DWORD cadence)
{
  if (line != POTSLine)
    return PFalse;

  if (cadence == 0)
    return ConvertOSError(IOCTL(os_handle, PHONE_RING_STOP));

  //if (!ConvertOSError(IOCTL2(os_handle, PHONE_RING_CADENCE, cadence)))
  //  return PFalse;

  int stat;
 
  // Need to add something to set caller ID here
#if TELEPHONY_VERSION >= 3000
  if (callerIdInfo.name[0] != '\0') {
    stat = IOCTLP(os_handle, PHONE_RING_START, &callerIdInfo);
    SetCallerID(line, "");
  } else
#endif
    stat = IOCTL2(os_handle, PHONE_RING_START, 0);

  return ConvertOSError(stat);
}


PBoolean OpalIxJDevice::RingLine(unsigned line, PINDEX nCadence, unsigned * pattern)
{
  if (line >= GetLineCount())
    return PFalse;

  if (line != POTSLine)
    return PFalse;

  return RingLine(line, nCadence != 0 ? 0xaaa : 0);
}


PBoolean OpalIxJDevice::IsLineDisconnected(unsigned line, PBoolean checkForWink)
{
  if (line >= GetLineCount())
    return PFalse;

  if (line != PSTNLine)
    return !IsLineOffHook(line);

  if (checkForWink) {

    // if we got a wink previously, hangup
    if (gotWink)
      return PTrue;

    // if we have not got a wink, then check for one
    PWaitAndSignal m(exceptionMutex);
    ExceptionInfo * info = GetException();

    gotWink = info->hasWink;
    info->hasWink = PFalse;
    if (gotWink) {
      PTRACE(3, "xJack\tDetected wink");
      return PTrue;
    }
  }


  if (IsToneDetected(line) & (BusyTone)) {
    PTRACE(3, "xJack\tDetected end of call tone");
    return PTrue;
  }

  return PFalse;
}

PBoolean OpalIxJDevice::SetLineToLineDirect(unsigned line1, unsigned line2, PBoolean connect)
{
  if (connect && (line1 != line2)) 
    IOCTL2(os_handle, IXJCTL_POTS_PSTN, 1);
  else 
    IOCTL2(os_handle, IXJCTL_POTS_PSTN, 0);

  return PTrue;
}


PBoolean OpalIxJDevice::IsLineToLineDirect(unsigned line1, unsigned line2)
{
  return PFalse;
}

PBoolean OpalIxJDevice::ConvertOSError(int err) 
{
  PChannel::Errors normalisedError;
  return PChannel::ConvertOSError(err, normalisedError, osError);
}


static const struct {
  const char * mediaFormat;
  PINDEX writeFrameSize;
  PINDEX readFrameSize;
  int mode;
  int frameTime;
  PBoolean vad;
} CodecInfo[] = {
  /* NOTE: These are enumerated in reverse order. */
  { OPAL_PCM16,         480, 480, LINEAR16, 30, PFalse },   // 480 bytes = 240 samples = 30ms
  { OPAL_G711_ULAW_64K, 240, 240, ULAW,     30, PFalse },   // 240 bytes = 240 samples = 30ms
  { OPAL_G711_ALAW_64K, 240, 240, ALAW,     30, PFalse },   // 240 bytes = 240 samples = 30ms
  { OPAL_G728,           60,  60, G728,     30, PFalse },   // 60 bytes  = 12 frames   = 30ms
  { OPAL_G729A,          10,  10, G729,     10, PFalse },   // 10 bytes = 1 frame = 10 ms
  { OPAL_G729AB,         10,  10, G729B,    10,  PTrue },   // 10 bytes = 1 frame = 10 ms
  { OPAL_G7231_5k3 ,     24,  20, G723_53,  30, PFalse },   // 20 bytes = 1 frame = 30 ms
  { OPAL_G7231_6k3,      24,  24, G723_63,  30, PFalse },   // 24 bytes = 1 frame = 30 ms
  { OPAL_G7231A_5k3 ,    24,  20, G723_53,  30,  PTrue },   // 20 bytes = 1 frame = 30 ms
  { OPAL_G7231A_6k3,     24,  24, G723_63,  30,  PTrue }    // 24 bytes = 1 frame = 30 ms
};



OpalMediaFormatList OpalIxJDevice::GetMediaFormats() const
{
  OpalMediaFormatList codecs;

  PINDEX idx = PARRAYSIZE(CodecInfo);
  while (idx-- > 0) {
    phone_capability cap;
    cap.captype = codec;
    cap.cap = CodecInfo[idx].mode;
    if (IOCTLP(os_handle, PHONE_CAPABILITIES_CHECK, &cap))
      codecs += CodecInfo[idx].mediaFormat;
  }

  return codecs;
}


static PINDEX FindCodec(const OpalMediaFormat & mediaFormat)
{
  for (PINDEX codecType = 0; codecType < PARRAYSIZE(CodecInfo); codecType++) {
    if (mediaFormat == CodecInfo[codecType].mediaFormat)
      return codecType;
  }

  return P_MAX_INDEX;
}


PBoolean OpalIxJDevice::SetReadFormat(unsigned line, const OpalMediaFormat & mediaFormat)
{
  {
    PWaitAndSignal mutex(toneMutex);
    if (tonePlaying) {
      tonePlaying = PFalse;
      IOCTL(os_handle, PHONE_CPT_STOP);
    }
  }

  PWaitAndSignal mutex(readMutex);

  if (!readStopped) {
    IOCTL(os_handle, PHONE_REC_STOP);
    readStopped = PTrue;
    OpalLineInterfaceDevice::StopReadCodec(line);
  }

  readCodecType = FindCodec(mediaFormat);
  if (readCodecType == P_MAX_INDEX) {
    PTRACE(1, "xJack\tUnsupported read codec requested: " << mediaFormat);
    return PFalse;
  }

  if (!writeStopped && readCodecType != writeCodecType) {
    PTRACE(1, "xJack\tAsymmectric codecs requested: "
              "read=" << CodecInfo[readCodecType].mediaFormat <<
              " write=" << CodecInfo[writeCodecType].mediaFormat);
    return PFalse;
  }

  PTRACE(2, "IXJ\tSetting read codec to "
         << CodecInfo[readCodecType].mediaFormat
         << " code=" << CodecInfo[readCodecType].mode);

  readFrameSize = CodecInfo[readCodecType].readFrameSize;

  // set frame time
  if (writeStopped)
    IOCTL2(os_handle, PHONE_FRAME, CodecInfo[readCodecType].frameTime);

  int stat = IOCTL2(os_handle, PHONE_REC_CODEC, CodecInfo[readCodecType].mode);
  if (stat != 0) {
    PTRACE(1, "IXJ\tSecond try on set record codec");
    stat = IOCTL2(os_handle, PHONE_REC_CODEC, CodecInfo[readCodecType].mode);
    if (stat != 0) {
      PTRACE(1, "IXJ\tFailed second try on set record codec");
      return PFalse;
    }
  }

  // PHONE_REC_DEPTH does not set return value
  IOCTL2(os_handle, PHONE_REC_DEPTH, 1);

  // PHONE_REC_START does not set return value
  stat = IOCTL(os_handle, PHONE_REC_START);
  if (stat != 0) {
    return PFalse;
  }

  readStopped = PFalse;

  return PTrue;
}

PBoolean OpalIxJDevice::SetWriteFormat(unsigned line, const OpalMediaFormat & mediaFormat)
{
  {
    PWaitAndSignal mutex(toneMutex);
    if (tonePlaying) {
      tonePlaying = PFalse;
      IOCTL(os_handle, PHONE_CPT_STOP);
    }
  }

  PWaitAndSignal mutex(readMutex);

  if (!writeStopped) {
    IOCTL(os_handle, PHONE_PLAY_STOP);
    writeStopped = PTrue;
    OpalLineInterfaceDevice::StopWriteCodec(line);
  }


  writeCodecType = FindCodec(mediaFormat);
  if (writeCodecType == P_MAX_INDEX) {
    PTRACE(1, "xJack\tUnsupported write codec requested: " << mediaFormat);
    return PFalse;
  }

  if (!readStopped && writeCodecType != readCodecType) {
    PTRACE(1, "xJack\tAsymmectric codecs requested: "
              "read=" << CodecInfo[readCodecType].mediaFormat <<
              " write=" << CodecInfo[writeCodecType].mediaFormat);
    return PFalse;
  }

  PTRACE(2, "IXJ\tSetting write codec to "
         << CodecInfo[writeCodecType].mediaFormat
         << " code=" << CodecInfo[writeCodecType].mode);

  writeFrameSize = CodecInfo[writeCodecType].writeFrameSize;

  // set frame time
  if (readStopped)
    IOCTL2(os_handle, PHONE_FRAME, CodecInfo[writeCodecType].frameTime);

  int stat = IOCTL2(os_handle, PHONE_PLAY_CODEC, CodecInfo[writeCodecType].mode);
  if (stat != 0) {
    PTRACE(1, "IXJ\tSecond try on set play codec");
    stat = IOCTL2(os_handle, PHONE_PLAY_CODEC, CodecInfo[writeCodecType].mode);
    if (stat != 0)
      return PFalse;
  }

  // PHONE_PLAY_DEPTH does not set return value
  IOCTL2(os_handle, PHONE_PLAY_DEPTH, 1);

  // start the codec
  stat = IOCTL(os_handle, PHONE_PLAY_START);
  if (stat != 0) {
    PTRACE(1, "IXJ\tSecond try on start play codec");
    stat = IOCTL(os_handle, PHONE_PLAY_START);
    if (stat != 0)
      return PFalse;
  }

  // wait for codec to become writable. If it doesn't happen after 100ms, give error
  fd_set wfds;
  struct timeval ts;

  for (;;) {

    FD_ZERO(&wfds);
    FD_SET(os_handle, &wfds);
    ts.tv_sec = 0;
    ts.tv_usec = 100*1000;

    stat = ::select(os_handle+1, NULL, &wfds, NULL, &ts);

    if (stat > 0)
      break;
    else if (stat == 0) {
      PTRACE(1, "IXJ\tWrite timeout on startup");
      return PFalse;
    } 

    if (errno != EINTR) {
      PTRACE(1, "IXJ\tWrite error on startup");
      return PFalse;
    }
  }

  writeStopped = PFalse;

  return PTrue;
}


OpalMediaFormat OpalIxJDevice::GetReadFormat(unsigned)
{
  if (readCodecType == P_MAX_INDEX)
    return "";
  return CodecInfo[readCodecType].mediaFormat;
}


OpalMediaFormat OpalIxJDevice::GetWriteFormat(unsigned)
{
  if (writeCodecType == P_MAX_INDEX)
    return "";
  return CodecInfo[writeCodecType].mediaFormat;
}


PBoolean OpalIxJDevice::SetRawCodec(unsigned line)
{
  if (inRawMode)
    return PFalse;

  PTRACE(2, "IXJ\tSetting raw codec mode");

  // save the current volumes
  savedPlayVol    = userPlayVol;
  savedRecVol     = userRecVol;
  savedAEC        = aecLevel;

  if (!SetReadFormat (line, CodecInfo[0].mediaFormat) ||
      !SetWriteFormat(line, CodecInfo[0].mediaFormat)) {
    PTRACE(1, "IXJ\t Failed to set raw codec");
    StopReadCodec(line);
    StopWriteCodec(line);
    return PFalse;
  }

  // set the new (maximum) volumes
  SetAEC         (line, AECOff);
  SetRecordVolume(line, 100);
  SetPlayVolume  (line, 100);

  // stop values from changing
  inRawMode = PTrue;

  return PTrue;
}


PBoolean OpalIxJDevice::StopReadCodec(unsigned line)
{
  PTRACE(3, "xJack\tStopping read codec");

  PWaitAndSignal mutex(readMutex);

  if (!readStopped) {
    IOCTL(os_handle, PHONE_REC_STOP);
    readStopped = PTrue;
  }

  return OpalLineInterfaceDevice::StopReadCodec(line);
}


PBoolean OpalIxJDevice::StopWriteCodec(unsigned line)
{
  PTRACE(3, "xJack\tStopping write codec");

  PWaitAndSignal mutex(readMutex);

  if (!writeStopped) {
    IOCTL(os_handle, PHONE_PLAY_STOP);
    writeStopped = PTrue;
  }

  return OpalLineInterfaceDevice::StopWriteCodec(line);
}


PBoolean OpalIxJDevice::StopRawCodec(unsigned line)
{
  if (!inRawMode)
    return PFalse;

  StopReadCodec(line);
  StopWriteCodec(line);

  // allow values to change again
  inRawMode = PFalse;

  SetPlayVolume  (line, savedPlayVol);
  SetRecordVolume(line, savedRecVol);
  SetAEC         (line, savedAEC);

  OpalLineInterfaceDevice::StopReadCodec(line);
  OpalLineInterfaceDevice::StopWriteCodec(line);
  return PTrue;
}


PINDEX OpalIxJDevice::GetReadFrameSize(unsigned)
{
  return readFrameSize;
}

PBoolean OpalIxJDevice::SetReadFrameSize(unsigned, PINDEX)
{
  return PFalse;
}

static void G728_Pack(const unsigned short * unpacked, BYTE * packed)
{
  packed[0] =                                ((unpacked[0] & 0x3fc) >> 2);
  packed[1] = ((unpacked[0] & 0x003) << 6) | ((unpacked[1] & 0x3f0) >> 4);
  packed[2] = ((unpacked[1] & 0x00f) << 4) | ((unpacked[2] & 0x3c0) >> 6);
  packed[3] = ((unpacked[2] & 0x03f) << 2) | ((unpacked[3] & 0x300) >> 8);
  packed[4] =  (unpacked[3] & 0x0ff);
}

static const PINDEX G723count[4] = { 24, 20, 4, 1 };


PBoolean OpalIxJDevice::ReadFrame(unsigned, void * buffer, PINDEX & count)
{
  {
    PWaitAndSignal rmutex(readMutex);

    count = 0;

    if (readStopped) {
        PTRACE(1, "IXJ\tRead stopped, so ReadFrame returns false");    
        return PFalse;
    }

    if (writeStopped) {
      PThread::Sleep(30);
      memset(buffer, 0, readFrameSize);
      switch (CodecInfo[readCodecType].mode) {
        case G723_63:
        case G723_53:
          *((DWORD *)buffer) = 0x02;
          count = 4;
          break;
        case G729B:
          *((WORD *)buffer) = 0;
          count = 2;
          break;
        default:
          memset(buffer, 0, readFrameSize);
          count = readFrameSize;
          break;
      }
      return PTrue;
    }
  
    WORD temp_frame_buffer[48];   // 30ms = 12 frames = 48 vectors = 48 WORDS for unpacked vectors
    void * readBuf;
    int    readLen;
    switch (CodecInfo[readCodecType].mode) {
      case G728 :
        readBuf = temp_frame_buffer;
        readLen = sizeof(temp_frame_buffer);
        break;
      case G729B :
        readBuf = temp_frame_buffer;
        readLen = 12;
        break;
      default :
        readBuf = buffer;
        readLen = readFrameSize;
    }
  
    for (;;) {
      fd_set rfds;
      FD_ZERO(&rfds);
      FD_SET(os_handle, &rfds);
      struct timeval ts;
      ts.tv_sec = 30;
      ts.tv_usec = 0;
  #if PTRACING
      PTime then;
  #endif
      int stat = ::select(os_handle+1, &rfds, NULL, NULL, &ts);
      if (stat == 0) {
        PTRACE(1, "IXJ\tRead timeout:" << (PTime() - then));
        return PFalse;
      }
      
      if (stat > 0) {
        stat = ::read(os_handle, readBuf, readLen);
        if (stat == (int)readLen)
          break;
      }
  
      if ((stat >= 0) || (errno != EINTR)) {
        PTRACE(1, "IXJ\tRead error = " << errno);
        return PFalse;
      }
  
      PTRACE(1, "IXJ\tRead EINTR");
    }
  
    switch (CodecInfo[readCodecType].mode) {
      case G723_63:
      case G723_53:
        count = G723count[(*(BYTE *)readBuf)&3];
        break;
  
      case G728 :
        // for G728, pack four x 4 vectors
        PINDEX i;
        for (i = 0; i < 12; i++)
          G728_Pack(temp_frame_buffer+i*4, ((BYTE *)buffer)+i*5);
        count = readFrameSize;
        break;
  
      case G729B :
        switch (temp_frame_buffer[0]) {
          case 0 : // Silence
            memset(buffer, 0, 10);
            count = 10;
            break;
          case 1 : // Signal
            memcpy(buffer, &temp_frame_buffer[1], 10);
            count = 10;
            break;
          case 2 : // VAD
            memcpy(buffer, &temp_frame_buffer[1], 2);
            count = 2;
            break;
          default : // error
            PTRACE(1, "IXJ\tIllegal value from codec in G729");
            return PFalse;
        }
        break;
  
      default :
        count = readFrameSize;
    }
  }

  PThread::Yield();
  
  return PTrue;
}

PINDEX OpalIxJDevice::GetWriteFrameSize(unsigned)
{
  return writeFrameSize;
}

PBoolean OpalIxJDevice::SetWriteFrameSize(unsigned, PINDEX)
{
  return PFalse;
}

static void G728_Unpack(const BYTE * packed, unsigned short * unpacked)
{
  unpacked[0] = ( packed[0]         << 2) | ((packed[1] & 0xc0) >> 6);
  unpacked[1] = ((packed[1] & 0x3f) << 4) | ((packed[2] & 0xf0) >> 4);
  unpacked[2] = ((packed[2] & 0x0f) << 6) | ((packed[3] & 0xfc) >> 2);
  unpacked[3] = ((packed[3] & 0x03) << 8) |   packed[4];
}

PBoolean OpalIxJDevice::WriteFrame(unsigned, const void * buffer, PINDEX count, PINDEX & written)
{
  {
    PWaitAndSignal rmutex(readMutex);
  
    written = 0;
  
    if (writeStopped) 
      return PFalse;
  
    if (readStopped) {
      PThread::Sleep(30);
      written = writeFrameSize;
      return PTrue;
    }
  
    WORD temp_frame_buffer[48];
    const void * writeBuf;
    int writeLen;
  
    switch (CodecInfo[writeCodecType].mode) {
      case G723_63:
      case G723_53:
        writeBuf = buffer;
        writeLen = 24;
        written = G723count[(*(BYTE *)buffer)&3];
        break;
  
      case G728 :
        writeBuf = temp_frame_buffer;
        writeLen = sizeof(temp_frame_buffer);
  
        // for G728, unpack twelve x 4 vectors
        PINDEX i;
        for (i = 0; i < 12; i++) 
          G728_Unpack(((const BYTE *)buffer)+i*5, temp_frame_buffer+i*4);
        written = 60;
        break;
  
      case G729B :
        writeBuf = temp_frame_buffer;
        writeLen = 12;
  
        if (count == 2) {
          temp_frame_buffer[0] = 2;
          temp_frame_buffer[1] = *(const WORD *)buffer;
          memset(&temp_frame_buffer[2], 0, 8);
          written = 2;
        }
        else {
          if (memcmp(buffer, "\0\0\0\0\0\0\0\0\0", 10) != 0)
            temp_frame_buffer[0] = 1;
          else
            temp_frame_buffer[0] = 0;
          memcpy(&temp_frame_buffer[1], buffer, 10);
          written = 10;
        }
        break;
  
      default :
        writeBuf = buffer;
        writeLen = writeFrameSize;
        written = writeFrameSize;
    }
  
    if (count < written) {
      osError = EINVAL;
      PTRACE(1, "xJack\tWrite of too small a buffer : " << count << " vs " << written);
      return PFalse;
    }
  
    for (;;) {
  
      fd_set wfds;
      FD_ZERO(&wfds);
      FD_SET(os_handle, &wfds);
      struct timeval ts;
      ts.tv_sec = 5;
      ts.tv_usec = 0;
      int stat = ::select(os_handle+1, NULL, &wfds, NULL, &ts);
  
      if (stat == 0) {
        PTRACE(1, "IXJ\tWrite timeout");
        return PFalse;
      }
  
      if (stat > 0) {
        stat = ::write(os_handle, writeBuf, writeLen);
        if (stat == (int)writeLen)
          break;
      }
  
      if ((stat >= 0) || (errno != EINTR)) {
        PTRACE(1, "IXJ\tWrite error = " << errno);
        return PFalse;
      }

      PTRACE(1, "IXJ\tWrite EINTR");
    }
  }

//  PTRACE(4, "IXJ\tWrote " << writeLen << " bytes to codec");

  PThread::Yield();

  return PTrue;
}


unsigned OpalIxJDevice::GetAverageSignalLevel(unsigned, PBoolean playback)
{
  return IOCTL(os_handle, playback ? PHONE_PLAY_LEVEL : PHONE_REC_LEVEL);
}


PBoolean OpalIxJDevice::EnableAudio(unsigned line, PBoolean enable)
{
  if (line >= GetLineCount())
    return PFalse;

  int port = PORT_SPEAKER;

  if (enable) {
    if (enabledAudioLine != line) {
      if (enabledAudioLine != UINT_MAX && exclusiveAudioMode) {
        PTRACE(3, "xJack\tEnableAudio on port when already enabled other port.");
        return PFalse;
      }
      enabledAudioLine = line;
    }
    port = (line == POTSLine) ? PORT_POTS : PORT_PSTN;
  }
  else
    enabledAudioLine = UINT_MAX;

  return ConvertOSError(IOCTL2(os_handle, IXJCTL_PORT, port));
}


PBoolean OpalIxJDevice::IsAudioEnabled(unsigned line)
{
  return enabledAudioLine == line;
}

PINDEX OpalIxJDevice::LogScaleVolume(unsigned line, PINDEX volume, PBoolean isPlay)
{
  PINDEX dspMax = isPlay ? 0x100 : 0x200;

  switch (dwCardType) {
  case 0:
  case PhoneJACK:
    dspMax = isPlay ? 0x100 : 0x200;
    break;
  case LineJACK:
    dspMax = 0x200;
    break;
  case PhoneJACK_Lite:
    dspMax = 0x200;
    break;
  case PhoneJACK_PCI:
    dspMax = 0x100;
    break;
  case PhoneCARD:
    dspMax = 0x200;
    break;    
  case PhoneJACK_PCI_TJ:
    if (line == POTSLine)
      dspMax = 0x100;
    else
      dspMax = 0x60;
  }

/* The dsp volume is exponential in nature.
   You can plot this exponential function with gnuplot.
   gnuplot
   plot [t=0:100] exp((t/20) -1) / exp(4) */
  PINDEX res = (PINDEX) (dspMax * exp((((double)volume) / 20.0) - 1) / exp(4.0));

  return res;
}
 

PBoolean OpalIxJDevice::SetRecordVolume(unsigned line, unsigned volume)
{
  PWaitAndSignal mutex1(readMutex);
  userRecVol = volume;
  if ((aecLevel == AECAGC) || inRawMode)
    return PTrue;

  return IOCTL2(os_handle, IXJCTL_REC_VOLUME, LogScaleVolume(line, volume, PFalse));
}

PBoolean OpalIxJDevice::GetRecordVolume(unsigned, unsigned & volume)
{
  volume = userRecVol;
  return PTrue;
}

PBoolean OpalIxJDevice::SetPlayVolume(unsigned line, unsigned volume)
{
  PWaitAndSignal mutex1(readMutex);
  userPlayVol = volume;
  if (inRawMode)
    return PTrue;

  return IOCTL2(os_handle, IXJCTL_PLAY_VOLUME, LogScaleVolume(line, volume, PTrue));
}

PBoolean OpalIxJDevice::GetPlayVolume(unsigned, unsigned & volume)
{
  volume = userPlayVol;
  return PTrue;
}

OpalLineInterfaceDevice::AECLevels OpalIxJDevice::GetAEC(unsigned)
{
  return aecLevel;
}


PBoolean OpalIxJDevice::SetAEC(unsigned line, AECLevels level)
{
  aecLevel = level;

  if (inRawMode)
    return PTrue;

  // IXJCTL_AEC_START does not set return code
  IOCTL2(os_handle, IXJCTL_AEC_START, aecLevel);

  // if coming out of AGC mode, then set record volume just in case
  if (aecLevel == AECAGC)
    SetRecordVolume(line, userRecVol);

  return PTrue;
}


unsigned OpalIxJDevice::GetWinkDuration(unsigned)
{
  if (!IsOpen())
    return 0;

  return IOCTL2(os_handle, IXJCTL_WINK_DURATION, 0);
}


PBoolean OpalIxJDevice::SetWinkDuration(unsigned, unsigned winkDuration)
{
  if (!IsOpen())
    return PFalse;  

  return IOCTL2(os_handle, IXJCTL_WINK_DURATION, winkDuration);
}


PBoolean OpalIxJDevice::GetVAD(unsigned)
{
  return PFalse;
}


PBoolean OpalIxJDevice::SetVAD(unsigned, PBoolean)
{
  return PFalse;
}


PBoolean OpalIxJDevice::GetCallerID(unsigned line, PString & callerId, PBoolean /*full*/)
{
#if TELEPHONY_VERSION < 3000
  return PFalse;
#else

  if (line != PSTNLine)
    return PFalse;

  // string is "number <TAB> time <TAB> name"

  PWaitAndSignal m(exceptionMutex);
  ExceptionInfo * info = GetException();

  if (info->hasCid) {
    PHONE_CID cid = info->cid;
    callerId  = PString(cid.number, cid.numlen) + '\t';
    callerId += PString(cid.hour, 3) + ':' + PString(cid.min, 3) + ' ' + PString(cid.month, 3) + '/' + PString(cid.day, 3) + '\t';
    callerId += PString(cid.name, cid.namelen);
    info->hasCid = PFalse;
    return PTrue;
  }

  return PFalse;
#endif
}

#if TELEPHONY_VERSION >= 3000

static PBoolean IsPhoneDigits(const PString & str)
{
  PINDEX i;
  for (i = 0; i < str.GetLength(); i++) 
    if (!isdigit(str[i]) && str[i] != '*' && str[i] != '#')
      return PFalse;
  return PTrue;
}

static void FormatCallerIdString(const PString & idString, PHONE_CID & callerIdInfo)
{
  memset(&callerIdInfo, 0, sizeof(callerIdInfo));

  if (idString.IsEmpty())
    return;

  PString name, number;
  PTime theTime;

// string is "number <TAB> time <TAB> name"

  PStringArray fields = idString.Tokenise('\t', PTrue);
  int len = fields.GetSize();

  // if the name is specified, then use it
  if (len > 2)
    name = fields[2];

  // if the time is specified, then use it
  if (len > 1 && !fields[1].IsEmpty())
    theTime = PTime(fields[1]);

  // if the number is specified, then only use it if it is legal
  // otherwise put it into the name field
  if (len > 0) {
    if (IsPhoneDigits(fields[0]))
      number = fields[0];
    else if (name.IsEmpty())
      name = fields[0];
  }

  // truncate name and number fields
  if (name.GetLength() > (PINDEX)sizeof(callerIdInfo.name))
    name = name.Left(sizeof(callerIdInfo.name));
  if (number.GetLength() > (PINDEX)sizeof(callerIdInfo.number))
    number = number.Left(sizeof(callerIdInfo.number));

  sprintf(callerIdInfo.month, "%02i", theTime.GetMonth());
  sprintf(callerIdInfo.day,   "%02i", theTime.GetDay());
  sprintf(callerIdInfo.hour,  "%02i", theTime.GetHour());
  sprintf(callerIdInfo.min,   "%02i", theTime.GetMinute());
  strncpy(callerIdInfo.name,    (const char *)name,   sizeof(callerIdInfo.name)-1);
  callerIdInfo.namelen = name.GetLength();
  strncpy(callerIdInfo.number,  (const char *)number, sizeof(callerIdInfo.number)-1);
  callerIdInfo.numlen = number.GetLength();
}
#endif

PBoolean OpalIxJDevice::SetCallerID(unsigned line, const PString & idString)
{
#if TELEPHONY_VERSION < 3000
  return PFalse;
#else
  if (line != POTSLine)
    return PFalse;

  FormatCallerIdString(idString, callerIdInfo);
#endif

  return PTrue;
}

PBoolean OpalIxJDevice::SendCallerIDOnCallWaiting(unsigned line, const PString & idString)
{
#if TELEPHONY_VERSION < 3000
  return PFalse;
#else
  if (line != POTSLine)
    return PFalse;

  PHONE_CID callerInfo;
  FormatCallerIdString(idString, callerInfo);
  IOCTLP(os_handle, IXJCTL_CIDCW, &callerInfo);
  return PTrue;
#endif
}


PBoolean OpalIxJDevice::SendVisualMessageWaitingIndicator(unsigned line, PBoolean on)
{
#if TELEPHONY_VERSION < 3000
  return PFalse;
#else
  if (line != POTSLine)
    return PFalse;

  IOCTL2(os_handle, IXJCTL_VMWI, on);

  return PTrue;
#endif
}


PBoolean OpalIxJDevice::PlayDTMF(unsigned, const char * tones, DWORD onTime, DWORD offTime)
{
  PWaitAndSignal mutex(toneMutex);

  if (tonePlaying)
    return PFalse;

  // not really needed, as we have the tone mutex locked
  tonePlaying = PTrue;

  IOCTL2(os_handle, PHONE_SET_TONE_ON_TIME,  onTime  * 4);
  IOCTL2(os_handle, PHONE_SET_TONE_OFF_TIME, offTime * 4);

  while (*tones != '\0') {

    char tone = toupper(*tones++);

    int code = -1;
    if ('1' <= tone && tone <= '9')
      code = tone - '0';

    else if (tone == '*')
      code = 10;

    else if (tone == '0')
      code = 11;

    else if (tone == '#')
      code = 12;
    
    else if ('A' <= tone && tone <= 'D')
      code = tone - 'A' + 28;

    else if ('E' <= tone && tone <= ('E' + 11))
      code = (tone - 'E') + 13;

    PTRACE(4, "IXJ\tPlaying tone " << tone);

    IOCTL2(os_handle, PHONE_PLAY_TONE, code);

    PThread::Sleep(onTime + offTime);

    long countDown = 200;  // a tone longer than 2 seconds? I don't think so...
    while ((countDown > 0) && IOCTL(os_handle, PHONE_GET_TONE_STATE) != 0) {
      PThread::Sleep(10);
      countDown--;
    }
    if (countDown == 0)
      cerr << "Timeout whilst waiting for DTMF tone to end" << endl;
  }

  // "Realize the truth....There is no tone."
  tonePlaying = PFalse;

  return PTrue;
}


char OpalIxJDevice::ReadDTMF(unsigned)
{
  PWaitAndSignal m(exceptionMutex);
  ExceptionInfo * info = GetException();

  int p = info->dtmfOut;

  if (info->dtmfIn == p)
    return '\0';

  char ch = info->dtmf[p];
  p = (p + 1) % 16;
  info->dtmfOut = p;

  return ch;
}


PBoolean OpalIxJDevice::GetRemoveDTMF(unsigned)
{
  return removeDTMF;
}


PBoolean OpalIxJDevice::SetRemoveDTMF(unsigned, PBoolean state)
{
  removeDTMF = state;
  return IOCTL2(os_handle, PHONE_DTMF_OOB, state);
}


OpalLineInterfaceDevice::CallProgressTones OpalIxJDevice::IsToneDetected(unsigned)
{
  PWaitAndSignal m(exceptionMutex);
  ExceptionInfo * info = GetException();

  int tones = NoTone;

  if (info->cadence[0] != 0) {
    info->cadence[0] = 0;
    tones |= DialTone;
  }

  if (info->cadence[1] != 0) {
    info->cadence[1] = 0;
    tones |= RingTone;
  }

  if (info->cadence[2] != 0) {
    info->cadence[2] = 0;
    tones |= BusyTone;
  }

  if (info->cadence[3] != 0) {
    info->cadence[3] = 0;
    tones |= CNGTone;
  }

  return (OpalLineInterfaceDevice::CallProgressTones)tones;
}


PBoolean OpalIxJDevice::SetToneFilterParameters(unsigned /*line*/,
                                            CallProgressTones tone,
                                            unsigned   lowFrequency,
                                            unsigned   highFrequency,
                                            PINDEX     numCadences,
                                            const unsigned * onTimes,
                                            const unsigned * offTimes)
{
  int toneIndex;
  switch (tone) {
    case DialTone :
      toneIndex = 0;
      break;
    case RingTone :
      toneIndex = 1;
      break;
    case BusyTone :
      toneIndex = 2;
      break;
    case CNGTone :
      toneIndex = 3;
      break;
    default :
      PTRACE(1, "xJack\tCannot set filter for tone: " << tone);
      return PFalse;
  }

#ifdef IXJCTL_SET_FILTER
  int filterCode = -1;
  int minMatch = 0, maxMatch = 0;

  if (lowFrequency == highFrequency) {
    static struct {
      IXJ_FILTER_FREQ code;
      unsigned        hertz;
    } const FreqToIXJFreq[] = {
      { f350,   350 }, { f300,   300 }, { f330,   330 }, { f340,   340 },
      { f392,   392 }, { f400,   400 }, { f420,   420 }, { f425,   425 },
      { f435,   435 }, { f440,   440 }, { f445,   445 }, { f450,   450 },
      { f452,   452 }, { f475,   475 }, { f480,   480 }, { f494,   494 },
      { f500,   500 }, { f520,   520 }, { f523,   523 }, { f525,   525 },
      { f587,   587 }, { f590,   590 }, { f600,   600 }, { f620,   620 },
      { f660,   660 }, { f700,   700 }, { f740,   740 }, { f750,   750 },
      { f770,   770 }, { f800,   800 }, { f816,   816 }, { f850,   850 },
      { f900,   900 }, { f942,   942 }, { f950,   950 }, { f975,   975 },
      { f1000, 1000 }, { f1020, 1020 }, { f1050, 1050 }, { f1100, 1100 },
      { f1140, 1140 }, { f1200, 1200 }, { f1209, 1209 }, { f1330, 1330 },
      { f1336, 1336 }, { f1380, 1380 }, { f1400, 1400 }, { f1477, 1477 },
      { f1600, 1600 }, { f1800, 1800 }, { f1860, 1860 }
    };

    PINDEX i;
    for (i = 0; i < PARRAYSIZE(FreqToIXJFreq); i++) {
      if (lowFrequency == FreqToIXJFreq[i].hertz) { 
        filterCode = FreqToIXJFreq[i].code;
        minMatch = maxMatch = FreqToIXJFreq[i].hertz;
        break;
      }
    }

  } else {
    static struct {
      IXJ_FILTER_FREQ code;
      unsigned        minHertz;
      unsigned        maxHertz;
    } const FreqToIXJFreq2[] = {
      { f20_50,       20,   50 }, { f133_200,    133,  200 }, { f300_640,    300,  640 },
      { f300_500,    300,  500 }, { f300_425,    300,  425 }, { f350_400,    350,  400 },
      { f350_440,    350,  440 }, { f350_450,    350,  450 }, { f380_420,    380,  420 },
      { f400_425,    400,  425 }, { f400_440,    400,  440 }, { f400_450,    400,  450 },
      { f425_450,    425,  450 }, { f425_475,    425,  475 }, { f440_450,    440,  450 },
      { f440_480,    440,  480 }, { f480_620,    480,  620 }, { f540_660,    540,  660 }, 
      { f750_1450,   750, 1450 }, { f857_1645,   857, 1645 }, { f900_1300,   900, 1300 },
      { f935_1215,   935, 1215 }, { f941_1477,   941, 1477 }, { f950_1400,   950, 1400 },
      { f1100_1750, 1100, 1750 }, { f1633_1638, 1633, 1638 }
    };

    PINDEX i;

    // look for exact match
    for (i = 0; i < PARRAYSIZE(FreqToIXJFreq2); i++) {
      if ((lowFrequency == FreqToIXJFreq2[i].minHertz) && (highFrequency == FreqToIXJFreq2[i].maxHertz)) { 
        filterCode = FreqToIXJFreq2[i].code;
        minMatch = FreqToIXJFreq2[i].minHertz;
        maxMatch = FreqToIXJFreq2[i].maxHertz;
        break;
      }
    }

    // look for an approximate match
    if (filterCode == -1) {
      for (i = 0; i < PARRAYSIZE(FreqToIXJFreq2); i++) {
        if ((lowFrequency > FreqToIXJFreq2[i].minHertz) && (highFrequency < FreqToIXJFreq2[i].maxHertz)) { 
          filterCode = FreqToIXJFreq2[i].code;
          minMatch = FreqToIXJFreq2[i].minHertz;
          maxMatch = FreqToIXJFreq2[i].maxHertz;
          break;
        }
      }
    }
  }

  if (filterCode < 0) {
    PTRACE(1, "PQIXJ\tCould not find filter match for " << lowFrequency << ", " << highFrequency);
    return PFalse;
  }

  // set the filter
  IXJ_FILTER filter;
  filter.filter = toneIndex;
  filter.freq   = (IXJ_FILTER_FREQ)filterCode;
  filter.enable = 1;
  PTRACE(3, "PQIXJ\tFilter " << lowFrequency << "," << highFrequency << " matched to " << minMatch << "," << maxMatch);
  if (::ioctl(os_handle, IXJCTL_SET_FILTER, &filter) < 0)
    return PFalse;
#endif

#if defined(IXJCTL_FILTER_CADENCE)
  IXJ_FILTER_CADENCE cadence;
  memset(&cadence, 0, sizeof(cadence));
  cadence.enable    = 2;
  cadence.en_filter = 0;
  cadence.filter    = toneIndex;
  switch (numCadences) {
    default :
      PTRACE(1, "xJack\tToo many cadence entries for Linux driver!");
      break;
    case 3 :
      cadence.on3  = ( onTimes[2]+5)/10;
      cadence.off3 = (offTimes[2]+5)/10;
    case 2 :
      cadence.on2  = ( onTimes[1]+5)/10;
      cadence.off2 = (offTimes[1]+5)/10;
    case 1 :
      cadence.on1  = ( onTimes[0]+5)/10;
      cadence.off1 = (offTimes[0]+5)/10;
  }

  // set the cadence
  return ::ioctl(os_handle, IXJCTL_FILTER_CADENCE, &cadence) >= 0;
#else
  return PFalse;
#endif
}


PBoolean OpalIxJDevice::PlayTone(unsigned line, CallProgressTones tone)
{
  {
    PWaitAndSignal mutex(toneMutex);

    if (tonePlaying) {
      tonePlaying = PFalse;
      IOCTL(os_handle, PHONE_CPT_STOP);
    }

    switch (tone) {

      case DialTone :
        tonePlaying = PTrue;
        return IOCTL(os_handle, PHONE_DIALTONE);

      case RingTone :
        tonePlaying = PTrue;
        return IOCTL(os_handle, PHONE_RINGBACK);

      case BusyTone :
        tonePlaying = PTrue;
        return IOCTL(os_handle, PHONE_BUSY);

      default :
        break;
    }
  }

  PWaitAndSignal mutex(toneMutex);
  StopTone(line);

  return PFalse;
}


PBoolean OpalIxJDevice::IsTonePlaying(unsigned)
{
//  if (IOCTL(os_handle, PHONE_GET_TONE_STATE) != 0) 
//    return PTrue;
//  return PFalse;

  return tonePlaying;
}


PBoolean OpalIxJDevice::StopTone(unsigned)
{
  PWaitAndSignal mutex(toneMutex);
  if (!tonePlaying) 
    return PTrue;

  tonePlaying = PFalse;
  return IOCTL(os_handle, PHONE_CPT_STOP);
}


PBoolean OpalIxJDevice::SetCountryCode(T35CountryCodes country)
{
  OpalLineInterfaceDevice::SetCountryCode(country);

  // if a LineJack, the set the DAA coeffiecients
  if (!IsLineJACK()) {
    PTRACE(4, "IXJ\tRequest to set DAA country on non-LineJACK");
    return PFalse;
  }

  if (country == UnknownCountry) {
    PTRACE(4, "IXJ\tRequest to set DAA country to unknown country code");
  } else {
    PTRACE(4, "IXJ\tSetting DAA country code to " << (int)country);
    static int ixjCountry[NumCountryCodes] = {
      DAA_JAPAN, 0, 0, 0, DAA_GERMANY, 0, 0, 0, 0, DAA_AUSTRALIA, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      DAA_FRANCE, 0, 0, 0, 0, DAA_GERMANY, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, DAA_UK, DAA_US, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0
    };
    IOCTL2(os_handle, IXJCTL_DAA_COEFF_SET, ixjCountry[countryCode]);
  }

  return PTrue;
}


DWORD OpalIxJDevice::GetSerialNumber()
{
  return IOCTL(os_handle, IXJCTL_SERIAL);
}


PStringArray OpalIxJDevice::GetDeviceNames()
{
  PStringArray array;

  PINDEX i, j = 0;
  for (i = 0; i < 10; i++) {
    PString devName = psprintf("/dev/phone%i", i);
    int handle = ::open((const char *)devName, O_RDWR);
    if (handle < 0 && errno != EBUSY)
      continue;
    ::close(handle);
    array[j++] = devName;
  }
  return array;
}

#endif // HAS_IXJ


/////////////////////////////////////////////////////////////////////////////
