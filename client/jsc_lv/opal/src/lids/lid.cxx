/*
 * lid.cxx
 *
 * Line Interface Device
 *
 * Open Phone Abstraction Library
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
 * Contributor(s): ______________________________________.
 *
 * $Revision: 22570 $
 * $Author: csoutheren $
 * $Date: 2009-05-08 04:42:36 +0000 (Fri, 08 May 2009) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "lid.cxx"
#endif

#include <opal/buildopts.h>

#include <lids/lid.h>


#define new PNEW


static OpalLIDRegistration * RegisteredLIDsListHead;


///////////////////////////////////////////////////////////////////////////////

#if PTRACING
ostream & operator<<(ostream & o, OpalLineInterfaceDevice::CallProgressTones t)
{
  static const char * const CallProgressTonesNames[OpalLineInterfaceDevice::NumTones+1] = {
    "NoTone", "DialTone", "RingTone", "BusyTone", "CongestionTone", "ClearTone", "MwiTone", "RoutingTone", "CNGTone", "CEDTone", "UserTone"
  };
  if (t+1 < PARRAYSIZE(CallProgressTonesNames) && CallProgressTonesNames[t+1] != NULL)
    return o << CallProgressTonesNames[t+1];
  else
    return o << "UnknownTone:" << (unsigned)t;
}
#endif


OpalLineInterfaceDevice::OpalLineInterfaceDevice()
  : os_handle(-1)
  , osError(0)
  , m_readDeblockingOffset(P_MAX_INDEX)
  , m_writeDeblockingOffset(0)
{
  // Unknown country, just use US tones
  countryCode = UnknownCountry;
  m_callProgressTones[DialTone] = "350+440:0.2";
  m_callProgressTones[RingTone] = "440+480:2.0-4.0";
  m_callProgressTones[BusyTone] = "480+620:0.5-0.5";
  m_callProgressTones[CongestionTone] = "480+620:0.3-0.2";
  m_callProgressTones[ClearTone] = "350+440:0.5";
  m_callProgressTones[MwiTone] = "350+440:0.2";
  m_callProgressTones[RoutingTone] = "1760:0.1-0.1-0.1-4.7";
  m_callProgressTones[CNGTone] = "1100:0.5";
  m_callProgressTones[CEDTone] = "2100:0.5";
}


PBoolean OpalLineInterfaceDevice::IsOpen() const
{
  return os_handle >= 0;
}


PBoolean OpalLineInterfaceDevice::Close()
{
  if (os_handle < 0)
    return PFalse;

  os_handle = -1;
  return PTrue;
}


PBoolean OpalLineInterfaceDevice::IsLinePresent(unsigned, PBoolean)
{
  return PTrue;
}


PBoolean OpalLineInterfaceDevice::HookFlash(unsigned line, unsigned flashTime)
{
  if (!IsLineOffHook(line))
    return PFalse;

  if (!SetLineOnHook(line))
    return PFalse;

  PThread::Sleep(flashTime);

  return SetLineOffHook(line);
}


PBoolean OpalLineInterfaceDevice::HasHookFlash(unsigned)
{
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::IsLineRinging(unsigned, DWORD *)
{
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::RingLine(unsigned, PINDEX, const unsigned *, unsigned)
{
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::SetLineConnected(unsigned line)
{
  return IsLineTerminal(line);
}


PBoolean OpalLineInterfaceDevice::IsLineConnected(unsigned line)
{
  return !IsLineTerminal(line) || IsLineOffHook(line);
}


PBoolean OpalLineInterfaceDevice::IsLineDisconnected(unsigned line, PBoolean)
{
  if (IsLineTerminal(line))
    return !IsLineOffHook(line);

  return IsToneDetected(line) == BusyTone;
}


PBoolean OpalLineInterfaceDevice::SetLineToLineDirect(unsigned, unsigned, PBoolean)
{
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::IsLineToLineDirect(unsigned, unsigned)
{
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::StopReading(unsigned)
{
  m_readDeblockingOffset = P_MAX_INDEX;
  return PTrue;
}


PBoolean OpalLineInterfaceDevice::StopWriting(unsigned)
{
  m_writeDeblockingOffset = 0;
  return PTrue;
}


bool OpalLineInterfaceDevice::UsesRTP() const
{
  return false;
}


PBoolean OpalLineInterfaceDevice::SetReadFrameSize(unsigned /*line*/, PINDEX /*frameSize*/)
{
  return false;
}


PBoolean OpalLineInterfaceDevice::SetWriteFrameSize(unsigned /*line*/, PINDEX /*frameSize*/)
{
  return false;
}


PINDEX OpalLineInterfaceDevice::GetReadFrameSize(unsigned /*line*/)
{
  return 0;
}


PINDEX OpalLineInterfaceDevice::GetWriteFrameSize(unsigned /*line*/)
{
  return 0;
}


PBoolean OpalLineInterfaceDevice::ReadBlock(unsigned line, void * buffer, PINDEX length)
{
  if (UsesRTP())
    return ReadFrame(line, buffer, length);

  // Are reblocking the hardware frame sizes to those expected by the RTP packets.
  PINDEX frameSize = GetReadFrameSize(line);

  BYTE * bufferPtr = (BYTE *)buffer;

  PINDEX readBytes;
  while (length > 0) {
    if (m_readDeblockingOffset < frameSize) {
      PINDEX left = frameSize - m_readDeblockingOffset;
      if (left > length)
        left = length;
      memcpy(bufferPtr, &m_readDeblockingBuffer[m_readDeblockingOffset], left);
      m_readDeblockingOffset += left;
      bufferPtr += left;
      length -= left;
    }
    else if (length < frameSize) {
      BYTE * deblockPtr = m_readDeblockingBuffer.GetPointer(frameSize);
      if (!ReadFrame(line, deblockPtr, readBytes))
        return PFalse;
      m_readDeblockingOffset = 0;
    }
    else {
      if (!ReadFrame(line, bufferPtr, readBytes))
        return PFalse;
      bufferPtr += readBytes;
      length -= readBytes;
    }
  }

  return PTrue;
}


PBoolean OpalLineInterfaceDevice::WriteBlock(unsigned line, const void * buffer, PINDEX length)
{
  PINDEX written;
  if (UsesRTP())
    return WriteFrame(line, buffer, length, written);

  PINDEX frameSize = GetWriteFrameSize(line);

  // If zero length then flush any remaining data
  if (length == 0 && m_writeDeblockingOffset != 0) {
    SetWriteFrameSize(line, m_writeDeblockingOffset);
    PBoolean ok = WriteFrame(line,
                         m_writeDeblockingBuffer.GetPointer(),
                         GetWriteFrameSize(line),
                         written);
    SetWriteFrameSize(line, frameSize);
    m_writeDeblockingOffset = 0;
    return ok;
  }

  const BYTE * bufferPtr = (const BYTE *)buffer;

  while (length > 0) {
    // If have enough data and nothing in the reblocking buffer, just send it
    // straight on to the device.
    if (m_writeDeblockingOffset == 0 && length >= frameSize) {
      if (!WriteFrame(line, bufferPtr, frameSize, written))
        return PFalse;
      bufferPtr += written;
      length -= written;
    }
    else {
      BYTE * savedFramePtr = m_writeDeblockingBuffer.GetPointer(frameSize);

      // See if new chunk gives us enough for one frames worth
      if ((m_writeDeblockingOffset + length) < frameSize) {
        // Nope, just copy bytes into buffer and return
        memcpy(savedFramePtr + m_writeDeblockingOffset, bufferPtr, length);
        m_writeDeblockingOffset += length;
        return PTrue;
      }

      /* Calculate bytes we want from the passed in buffer to fill a frame by
         subtracting from full frame width the amount we have so far. This also
         means the lastWriteCount is set to the correct amount of buffer we are
         grabbing this time around.
       */
      PINDEX left = frameSize - m_writeDeblockingOffset;
      memcpy(savedFramePtr + m_writeDeblockingOffset, bufferPtr, left);
      m_writeDeblockingOffset = 0;

      // Write the saved frame out
      if (!WriteFrame(line, savedFramePtr, frameSize, written))
        return PFalse;

      bufferPtr += left;
      length -= left;
    }
  }

  return PTrue;
}


unsigned OpalLineInterfaceDevice::GetAverageSignalLevel(unsigned, PBoolean)
{
  return UINT_MAX;
}


PBoolean OpalLineInterfaceDevice::EnableAudio(unsigned line, PBoolean enabled)
{
  m_LineAudioEnabled.resize(GetLineCount());
  if (line >= m_LineAudioEnabled.size())
    return false;

  m_LineAudioEnabled[line] = enabled;
  return true;
}


PBoolean OpalLineInterfaceDevice::IsAudioEnabled(unsigned line) const
{
  return line < GetLineCount() && m_LineAudioEnabled[line];
}


PBoolean OpalLineInterfaceDevice::SetRecordVolume(unsigned, unsigned)
{
  return PFalse;
}

PBoolean OpalLineInterfaceDevice::GetRecordVolume(unsigned, unsigned &)
{
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::SetPlayVolume(unsigned, unsigned)
{
  return PFalse;
}

PBoolean OpalLineInterfaceDevice::GetPlayVolume(unsigned, unsigned &)
{
  return PFalse;
}


OpalLineInterfaceDevice::AECLevels OpalLineInterfaceDevice::GetAEC(unsigned) const
{
  return AECError;
}


PBoolean OpalLineInterfaceDevice::SetAEC(unsigned, AECLevels)
{
  return PFalse;
}

unsigned OpalLineInterfaceDevice::GetWinkDuration(unsigned)
{
  return 0;
}


PBoolean OpalLineInterfaceDevice::SetWinkDuration(unsigned, unsigned)
{
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::GetVAD(unsigned) const
{
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::SetVAD(unsigned, PBoolean)
{
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::GetCallerID(unsigned, PString & id, PBoolean)
{
  id = PString();
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::SetCallerID(unsigned, const PString &)
{
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::SendVisualMessageWaitingIndicator(unsigned, PBoolean)
{
  return PFalse;
}

PBoolean OpalLineInterfaceDevice::PlayDTMF(unsigned, const char *, DWORD, DWORD)
{
  return PFalse;
}


char OpalLineInterfaceDevice::ReadDTMF(unsigned)
{
  return '\0';
}


PBoolean OpalLineInterfaceDevice::GetRemoveDTMF(unsigned)
{
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::SetRemoveDTMF(unsigned, PBoolean)
{
  return PFalse;
}


OpalLineInterfaceDevice::CallProgressTones OpalLineInterfaceDevice::IsToneDetected(unsigned)
{
  return NoTone;
}


OpalLineInterfaceDevice::CallProgressTones OpalLineInterfaceDevice::WaitForToneDetect(unsigned line, unsigned timeout)
{
  PTRACE(3, "LID\tWaitForToneDetect line = " << line << ", timeout = " << timeout);

  static const unsigned sampleRate = 25;

  timeout = (timeout+sampleRate-1)/sampleRate;

  unsigned retry = 0;
  do {
    CallProgressTones tones = IsToneDetected(line);
    if (tones != NoTone) {
      PTRACE(3, "LID\tTone " << tones << " detected after " << (retry*sampleRate) << " ms");
      return tones;
    }

    PThread::Sleep(sampleRate);
    retry++;
  } while (retry < timeout);

  PTRACE(3, "LID\tTone detection timeout " << (retry*sampleRate) << " ms");
  return NoTone;
}


PBoolean OpalLineInterfaceDevice::WaitForTone(unsigned line,
                                          CallProgressTones tone,
                                          unsigned timeout)
{
  PTRACE(3, "LID\tWaitFor the tone " << tone );
  PBoolean res = WaitForToneDetect(line, timeout) & tone;
  PTRACE(3, "LID\tWaitFor the tone " << tone << 
	 " is successfull-" << (res ? "YES" : "No"));
  return res;
}


bool OpalLineInterfaceDevice::SetToneDescription(unsigned line,
                                            CallProgressTones tone,
                                            const PString & description)
{
  if (description.IsEmpty())
    return true;

  PString freqDesc, cadenceDesc;
  PINDEX colon = description.Find(':');
  if (colon == P_MAX_INDEX)
    freqDesc = description;
  else {
    freqDesc = description.Left(colon);
    cadenceDesc = description.Mid(colon+1);
  }

  ToneMixingModes mode = SimpleTone;
  unsigned low_freq, high_freq;
  PINDEX dash = freqDesc.FindOneOf("-+x");
  if (dash == P_MAX_INDEX)
    low_freq = high_freq = freqDesc.AsUnsigned();
  else {
    low_freq = freqDesc.Left(dash).AsUnsigned();
    high_freq = freqDesc.Mid(dash+1).AsUnsigned();
    switch (freqDesc[dash]) {
      case '+' :
        mode = AddedTone;
        break;
      case 'x' :
        mode = ModulatedTone;
        break;
    }
  }
  if (low_freq  < 100 || low_freq  > 3000 ||
      (mode != ModulatedTone ? (high_freq < 100 || high_freq > 3000 || low_freq > high_freq)
                             : (high_freq < 5 || high_freq > 100))) {
    PTRACE(1, "LID\tIllegal frequency specified: " << description);
    return PFalse;
  }

  PStringArray times = cadenceDesc.Tokenise("-");
  PINDEX numCadences = (times.GetSize()+1)/2;
  
  PUnsignedArray onTimes(numCadences), offTimes(numCadences);
  for (PINDEX i = 0; i < times.GetSize(); i++) {
    double time = times[i].AsReal();
    if (time <= 0.01 || time > 10) {
      PTRACE(1, "LID\tIllegal cadence time specified: " << description);
      return PFalse;
    }

    if ((i&1) == 0)
      onTimes[i/2] = (unsigned)(time*1000);
    else
      offTimes[i/2] = (unsigned)(time*1000);
  }

  return SetToneParameters(line, tone, low_freq, high_freq, mode, numCadences, onTimes, offTimes);
}


bool OpalLineInterfaceDevice::SetToneParameters(unsigned /*line*/,
                                                CallProgressTones /*tone*/,
                                                unsigned /*frequency1*/,
                                                unsigned /*frequency2*/,
                                                ToneMixingModes /*mode*/,
                                                PINDEX /*numCadences*/,
                                                const unsigned * /*onTimes*/,
                                                const unsigned * /*offTimes*/)
{
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::PlayTone(unsigned, CallProgressTones)
{
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::IsTonePlaying(unsigned)
{
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::StopTone(unsigned)
{
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::PlayAudio(unsigned /*line*/, const PString & /*filename*/)
{
  PTRACE(2, "LID\tBase Class PlayAudio method called, exiting with PFalse");
  return PFalse;
}


PBoolean OpalLineInterfaceDevice::StopAudio(unsigned /*line*/)
{
  PTRACE(2, "LID\tBase Class StopAudio method called, exiting with PFalse");
  return PFalse;
}
	
PBoolean OpalLineInterfaceDevice::RecordAudioStart(unsigned /*line*/, const PString & /*fn*/)
{
  PTRACE(2, "LID\tRecordAudioStart must be implemented in concrete class");
  return PFalse;
}

PBoolean OpalLineInterfaceDevice::RecordAudioStop(unsigned /*line*/)
{
  PTRACE(2, "LID\tRecordAudioStop must be implemented in concrete class");
  return PFalse;
}

OpalLineInterfaceDevice::CallProgressTones
        OpalLineInterfaceDevice::DialOut(unsigned line, const PString & number, const DialParams & params)
{
  PAssert(!number.IsEmpty(), PInvalidParameter);

  PTRACE(3, "LID\tDialOut to " << number << " on line " << line);

  if (IsLineTerminal(line)) {
    PTRACE(2, "LID\tDialOut line is a terminal, do nothing");
    return NoTone;

  }
  if (!SetLineOffHook(line)) {
    PTRACE(1, "LID\tDialOut cannot set the line off hook");
    return NoTone;
  }  

  /* Wait for dial tone or Message waiting tone */
  CallProgressTones tone = WaitForToneDetect(line, params.m_dialToneTimeout);
  if (tone != DialTone  && tone != MwiTone) {
    PTRACE(2, "LID\tDialOut dial tone or mwi tone not detected");
    if (params.m_requireTones) {
      SetLineOnHook(line);
      return DialTone;
    }
  }

  if (params.m_dialStartDelay > 0) {
    /* wait before dialing*/
    PTRACE(3, "LID\tDialOut wait " << params.m_dialStartDelay << "msec before dialing");
    PThread::Sleep(params.m_dialStartDelay);
  }

  // Dial the string
  PINDEX lastPos = 0;
  PINDEX nextPos;
  while ((nextPos = number.FindOneOf("!@,")) != P_MAX_INDEX) {
    PlayDTMF(line, number(lastPos, nextPos-1), params.m_dialDigitTime, params.m_dialInterDigitTime);
    lastPos = nextPos+1;
    switch (number[nextPos]) {
      case '!' :
        HookFlash(line);
        break;

      case '@' :
        if (!WaitForTone(line, DialTone, params.m_dialToneTimeout)) {
          if (params.m_requireTones) {
            SetLineOnHook(line);
            return DialTone;
          }
        }
        break;

      case ',' :
        PThread::Sleep(params.m_commaDelay);
        break;
    }
  }

  PlayDTMF(line, number.Mid(lastPos), params.m_dialDigitTime, params.m_dialInterDigitTime);

  if (params.m_requireTones)
    return WaitForToneDetect(line, params.m_progressTimeout); // Wait for busy or ring back

  return RingTone;
}


static struct {
  const char * isoName;
  unsigned dialCode;
  OpalLineInterfaceDevice::T35CountryCodes t35Code;
  const char * fullName;
  const char * tone[OpalLineInterfaceDevice::NumTones];
} const CountryInfo[] = {
  { "AF", 93,   OpalLineInterfaceDevice::Afghanistan,           "Afghanistan" },
  { "AL", 355,  OpalLineInterfaceDevice::Albania,               "Albania" },
  { "DZ", 213,  OpalLineInterfaceDevice::Algeria,               "Algeria" },
  { "AS", 684,  OpalLineInterfaceDevice::AmericanSamoa,         "American Samoa" },
  { "AO", 244,  OpalLineInterfaceDevice::Angola,                "Angola" },
  { "AI", 1264, OpalLineInterfaceDevice::Anguilla,              "Anguilla" },
  { "AG", 1268, OpalLineInterfaceDevice::AntiguaAndBarbuda,     "Antigua and Barbuda" },
  { "AR", 54,   OpalLineInterfaceDevice::Argentina,             "Argentina" },
  { "AC", 247,  OpalLineInterfaceDevice::Ascension,             "Ascension Island" },
  { "AU", 61,   OpalLineInterfaceDevice::Australia,             "Australia",            { "425x25:0.2", "400+450:0.4-0.2-0.4-2", "425:0.375-0.375", "425:0.375-0.375/50%425:0.375-0.375", "425:0.375-0.375", "425x25:0.1-0.04" } },
  { "AT", 43,   OpalLineInterfaceDevice::Austria,               "Austria" },
  { "BS", 1242, OpalLineInterfaceDevice::Bahamas,               "Bahamas" },
  { "BH", 973,  OpalLineInterfaceDevice::Bahrain,               "Bahrain" },
  { "BD", 880,  OpalLineInterfaceDevice::Bangladesh,            "Bangladesh" },
  { "BB", 1246, OpalLineInterfaceDevice::Barbados,              "Barbados" },
  { "BE", 32,   OpalLineInterfaceDevice::Belgium,               "Belgium",              { "425:0.2", "425:1.0-3.0", "425:0.5-0.5", "425:0.167-0.167", "425:0.5-0.5", "425:1-0.25" } },
  { "BZ", 501,  OpalLineInterfaceDevice::Belize,                "Belize" },
  { "BJ", 229,  OpalLineInterfaceDevice::Benin,                 "Benin" },
  { "BM", 1441, OpalLineInterfaceDevice::Bermudas,              "Bermudas" },
  { "BT", 975,  OpalLineInterfaceDevice::Bhutan,                "Bhutan" },
  { "BO", 591,  OpalLineInterfaceDevice::Bolivia,               "Bolivia" },
  { "BW", 267,  OpalLineInterfaceDevice::Botswana,              "Botswana" },
  { "BR", 55,   OpalLineInterfaceDevice::Brazil,                "Brazil" },
  { "xx", 0,    OpalLineInterfaceDevice::BritishAntarcticTerritory, "British Antarctic Territory" },
  { "IO", 246,  OpalLineInterfaceDevice::BritishIndianOceanTerritory, "British IndianOcean Territory" },
  { "VG", 1284, OpalLineInterfaceDevice::BritishVirginIslands,  "British Virgin Islands" },
  { "BN", 673,  OpalLineInterfaceDevice::BruneiDarussalam,      "Brunei Darussalam" },
  { "BG", 359,  OpalLineInterfaceDevice::Bulgaria,              "Bulgaria" },
  { "BF", 226,  OpalLineInterfaceDevice::BurkinaFaso,           "Burkina Faso" },
  { "BI", 257,  OpalLineInterfaceDevice::Burundi,               "Burundi" },
  { "xx", 0,    OpalLineInterfaceDevice::Byelorussia,           "Byelorussia" },
  { "KH", 855,  OpalLineInterfaceDevice::Cambodia,              "Cambodia" },
  { "CM", 237,  OpalLineInterfaceDevice::Cameroon,              "Cameroon" },
  { "CA", 1,    OpalLineInterfaceDevice::Canada,                "Canada",               { "350+440:0.2", "440+480:2-4", "480+620:0.5-0.5", "480+620:0.25-0.25", "480+620:0.5-0.5" } },
  { "CV", 238,  OpalLineInterfaceDevice::CapeVerde,             "Cape Verde" },
  { "KY", 1345, OpalLineInterfaceDevice::CaymanIslands,         "Cayman Islands" },
  { "CF", 236,  OpalLineInterfaceDevice::CentralAfricanRepublic,"Central African Republic" },
  { "TD", 235,  OpalLineInterfaceDevice::Chad,                  "Chad" },
  { "CL", 56,   OpalLineInterfaceDevice::Chile,                 "Chile" },
  { "CN", 86,   OpalLineInterfaceDevice::China,                 "China",                { "450:0.2", "450:1-4", "450:0.35-0.35", "450:0.7-0.7", "450:0.35-0.35" } },
  { "CO", 57,   OpalLineInterfaceDevice::Colombia,              "Colombia" },
  { "KM", 269,  OpalLineInterfaceDevice::Comoros,               "Comoros" },
  { "CG", 242,  OpalLineInterfaceDevice::Congo,                 "Congo" },
  { "CK", 682,  OpalLineInterfaceDevice::CookIslands,           "Cook Islands" },
  { "CR", 506,  OpalLineInterfaceDevice::CostaRica,             "Costa Rica" },
  { "CI", 225,  OpalLineInterfaceDevice::CotedIvoire,           "Cote dIvoire" },
  { "CU", 53,   OpalLineInterfaceDevice::Cuba,                  "Cuba" },
  { "CY", 357,  OpalLineInterfaceDevice::Cyprus,                "Cyprus" },
  { "CZ", 420,  OpalLineInterfaceDevice::Czechoslovakia,        "Czech Republic" },
  { "DK", 45,   OpalLineInterfaceDevice::Denmark,               "Denmark" },
  { "DJ", 253,  OpalLineInterfaceDevice::Djibouti,              "Djibouti" },
  { "DM", 1767, OpalLineInterfaceDevice::Dominica,              "Dominica" },
  { "DO", 1809, OpalLineInterfaceDevice::DominicanRepublic,     "Dominican Republic" },
  { "EC", 593,  OpalLineInterfaceDevice::Ecuador,               "Ecuador" },
  { "EG", 20,   OpalLineInterfaceDevice::Egypt,                 "Egypt" },
  { "SV", 503,  OpalLineInterfaceDevice::ElSalvador,            "El Salvador" },
  { "GQ", 240,  OpalLineInterfaceDevice::EquatorialGuinea,      "Equatorial Guinea" },
  { "ET", 251,  OpalLineInterfaceDevice::Ethiopia,              "Ethiopia" },
  { "FK", 500,  OpalLineInterfaceDevice::FalklandIslands,       "Falkland Islands" },
  { "FJ", 679,  OpalLineInterfaceDevice::Fiji,                  "Fiji" },
  { "FI", 358,  OpalLineInterfaceDevice::Finland,               "Finland" },
  { "FR", 33,   OpalLineInterfaceDevice::France,                "France",               { "440:0.2", "440:1.5-3.5", "440:0.5-0.5", "440:0.5-0.5", "440:0.5-0.5" } },
  { "PF", 689,  OpalLineInterfaceDevice::FrenchPolynesia,       "French Polynesia" },
  { "TF", 0,    OpalLineInterfaceDevice::FrenchSouthernAndAntarcticLands, "French Southern and Antarctic Lands" },
  { "GA", 241,  OpalLineInterfaceDevice::Gabon,                 "Gabon" },
  { "GM", 220,  OpalLineInterfaceDevice::Gambia,                "Gambia" },
  { "DE", 49,   OpalLineInterfaceDevice::Germany,               "Germany",              { "425:0.2", "425:1-4", "425:0.48-0.48", "425:0.24-0.24", "425:0.48-0.48", "425+400:0.2" } },
  { "GH", 233,  OpalLineInterfaceDevice::Ghana,                 "Ghana" },
  { "GI", 350,  OpalLineInterfaceDevice::Gibraltar,             "Gibraltar" },
  { "GR", 30,   OpalLineInterfaceDevice::Greece,                "Greece" },
  { "GD", 1473, OpalLineInterfaceDevice::Grenada,               "Grenada" },
  { "GU", 1671, OpalLineInterfaceDevice::Guam,                  "Guam" },
  { "GT", 502,  OpalLineInterfaceDevice::Guatemala,             "Guatemala" },
  { "GY", 592,  OpalLineInterfaceDevice::Guayana,               "Guayana" },
  { "GG", 441,  OpalLineInterfaceDevice::Guernsey,              "Guernsey" },
  { "GN", 224,  OpalLineInterfaceDevice::Guinea,                "Guinea" },
  { "GW", 245,  OpalLineInterfaceDevice::GuineaBissau,          "Guinea Bissau" },
  { "HT", 509,  OpalLineInterfaceDevice::Haiti,                 "Haiti" },
  { "HN", 504,  OpalLineInterfaceDevice::Honduras,              "Honduras" },
  { "HK", 852,  OpalLineInterfaceDevice::Hongkong,              "Hong Kong" },
  { "HU", 36,   OpalLineInterfaceDevice::Hungary,               "Hungary" },
  { "IS", 354,  OpalLineInterfaceDevice::Iceland,               "Iceland" },
  { "IN", 91,   OpalLineInterfaceDevice::India,                 "India" },
  { "ID", 62,   OpalLineInterfaceDevice::Indonesia,             "Indonesia" },
  { "IR", 98,   OpalLineInterfaceDevice::Iran,                  "Iran" },
  { "IQ", 964,  OpalLineInterfaceDevice::Iraq,                  "Iraq" },
  { "IE", 353,  OpalLineInterfaceDevice::Ireland,               "Ireland" },
  { "IL", 972,  OpalLineInterfaceDevice::Israel,                "Israel" },
  { "IT", 39,   OpalLineInterfaceDevice::Italy,                 "Italy",                { "425:0.2-0.2-0.6-1", "425:1-4", "425:0.5-0.5", "425:0.2-0.2", "425:0.5-0.5", "425:0.4" } },
  { "JM", 1876, OpalLineInterfaceDevice::Jamaica,               "Jamaica" },
  { "JP", 81,   OpalLineInterfaceDevice::Japan,                 "Japan",                { "400:0.2", "400x18:1-2", "400:0.5-0.5", "425:0.2-0.2", "400:0.5-0.5" } },
  { "JE", 442,  OpalLineInterfaceDevice::Jersey,                "Jersey" },
  { "JO", 962,  OpalLineInterfaceDevice::Jordan,                "Jordan" },
  { "KE", 254,  OpalLineInterfaceDevice::Kenya,                 "Kenya" },
  { "KI", 686,  OpalLineInterfaceDevice::Kiribati,              "Kiribati" },
  { "KR", 82,   OpalLineInterfaceDevice::KoreaRepublic,         "Korea, Republic of" },
  { "KP", 850,  OpalLineInterfaceDevice::DemocraticPeoplesRepublicOfKorea, "Korea, Democratic Peoples Republic of" },
  { "KW", 965,  OpalLineInterfaceDevice::Kuwait,                "Kuwait" },
  { "LA", 856,  OpalLineInterfaceDevice::Lao,                   "Lao" },
  { "LB", 961,  OpalLineInterfaceDevice::Lebanon,               "Lebanon" },
  { "LS", 266,  OpalLineInterfaceDevice::Lesotho,               "Lesotho" },
  { "LR", 231,  OpalLineInterfaceDevice::Liberia,               "Liberia" },
  { "LY", 218,  OpalLineInterfaceDevice::Libya,                 "Libya" },
  { "LI", 423,  OpalLineInterfaceDevice::Liechtenstein,         "Liechtenstein" },
  { "LU", 352,  OpalLineInterfaceDevice::Luxemborg,             "Luxemborg" },
  { "MO", 853,  OpalLineInterfaceDevice::Macao,                 "Macao" },
  { "MG", 261,  OpalLineInterfaceDevice::Madagascar,            "Madagascar" },
  { "MY", 60,   OpalLineInterfaceDevice::Malaysia,              "Malaysia" },
  { "MW", 265,  OpalLineInterfaceDevice::Malawi,                "Malawi" },
  { "MV", 960,  OpalLineInterfaceDevice::Maldives,              "Maldives" },
  { "ML", 223,  OpalLineInterfaceDevice::Mali,                  "Mali" },
  { "MT", 356,  OpalLineInterfaceDevice::Malta,                 "Malta" },
  { "MR", 222,  OpalLineInterfaceDevice::Mauritania,            "Mauritania" },
  { "MU", 230,  OpalLineInterfaceDevice::Mauritius,             "Mauritius" },
  { "MX", 52,   OpalLineInterfaceDevice::Mexico,                "Mexico" },
  { "MC", 377,  OpalLineInterfaceDevice::Monaco,                "Monaco" },
  { "MN", 976,  OpalLineInterfaceDevice::Mongolia,              "Mongolia" },
  { "MS", 1664, OpalLineInterfaceDevice::Montserrat,            "Montserrat" },
  { "MA", 212,  OpalLineInterfaceDevice::Morocco,               "Morocco" },
  { "MZ", 258,  OpalLineInterfaceDevice::Mozambique,            "Mozambique" },
  { "MM", 95,   OpalLineInterfaceDevice::Myanmar,               "Myanmar" },
  { "NR", 674,  OpalLineInterfaceDevice::Nauru,                 "Nauru" },
  { "NP", 977,  OpalLineInterfaceDevice::Nepal,                 "Nepal" },
  { "NL", 31,   OpalLineInterfaceDevice::Netherlands,           "Netherlands",          { "425:0.2", "425:1-4", "425:0.5-0.5", "425:0.25-0.25", "425:0.5-0.5", "425:0.5-0.05"  } },
  { "AN", 599,  OpalLineInterfaceDevice::NetherlandsAntilles,   "Netherlands Antilles" },
  { "NC", 687,  OpalLineInterfaceDevice::NewCaledonia,          "New Caledonia" },
  { "NZ", 64,   OpalLineInterfaceDevice::NewZealand,            "New Zealand",          { "400:0.2", "400x17:0.4-0.2-0.4-2", "400:0.5-0.5", "400:0.25-0.25", "900:0.25-0.25", "400:0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1-0.1" } },
  { "NI", 505,  OpalLineInterfaceDevice::Nicaragua,             "Nicaragua" },
  { "NE", 227,  OpalLineInterfaceDevice::Niger,                 "Niger" },
  { "NG", 234,  OpalLineInterfaceDevice::Nigeria,               "Nigeria" },
  { "NO", 47,   OpalLineInterfaceDevice::Norway,                "Norway" },
  { "OM", 968,  OpalLineInterfaceDevice::Oman,                  "Oman" },
  { "PK", 92,   OpalLineInterfaceDevice::Pakistan,              "Pakistan" },
  { "PA", 507,  OpalLineInterfaceDevice::Panama,                "Panama" },
  { "PG", 675,  OpalLineInterfaceDevice::PapuaNewGuinea,        "Papua New Guinea" },
  { "PY", 595,  OpalLineInterfaceDevice::Paraguay,              "Paraguay" },
  { "PE", 51,   OpalLineInterfaceDevice::Peru,                  "Peru" },
  { "PH", 63,   OpalLineInterfaceDevice::Philippines,           "Philippines" },
  { "PL", 48,   OpalLineInterfaceDevice::Poland,                "Poland" },
  { "PT", 351,  OpalLineInterfaceDevice::Portugal,              "Portugal" },
  { "PR", 1787, OpalLineInterfaceDevice::PuertoRico,            "Puerto Rico" },
  { "QA", 974,  OpalLineInterfaceDevice::Qatar,                 "Qatar" },
  { "RO", 40,   OpalLineInterfaceDevice::Romania,               "Romania" },
  { "RU", 7,    OpalLineInterfaceDevice::USSR,                  "Russia" },
  { "RW", 250,  OpalLineInterfaceDevice::Rwanda,                "Rwanda" },
  { "xx", 0,    OpalLineInterfaceDevice::SaintCroix,            "Saint Croix" },
  { "SH", 290,  OpalLineInterfaceDevice::SaintHelenaAndAscension, "Saint Helena and Ascension" },
  { "KN", 1869, OpalLineInterfaceDevice::SaintKittsAndNevis,    "Saint Kitts and Nevis" },
  { "LC", 1758, OpalLineInterfaceDevice::SaintLucia,            "Saint Lucia" },
  { "xx", 0,    OpalLineInterfaceDevice::SaintThomas,           "Saint Thomas" },
  { "VC", 1784, OpalLineInterfaceDevice::SaintVicentAndTheGrenadines, "Saint Vicent and the Grenadines" },
  { "SM", 378,  OpalLineInterfaceDevice::SanMarino,             "San Marino" },
  { "ST", 239,  OpalLineInterfaceDevice::SaoTomeAndPrincipe,    "Sao Tome and Principe" },
  { "SA", 966,  OpalLineInterfaceDevice::SaudiArabia,           "Saudi Arabia" },
  { "SN", 221,  OpalLineInterfaceDevice::Senegal,               "Senegal" },
  { "SC", 248,  OpalLineInterfaceDevice::Seychelles,            "Seychelles" },
  { "SL", 232,  OpalLineInterfaceDevice::SierraLeone,           "Sierra Leone" },
  { "SG", 65,   OpalLineInterfaceDevice::Singapore,             "Singapore",            { "425:0.2", "425x24:0.4-0.2-0.4-2", "425:0.75-0.75", "425:0.25-0.25", "425:0.75-0.75" } },
  { "SB", 677,  OpalLineInterfaceDevice::SolomonIslands,        "Solomon Islands" },
  { "SO", 252,  OpalLineInterfaceDevice::Somalia,               "Somalia" },
  { "ZA", 27,   OpalLineInterfaceDevice::SouthAfrica,           "South Africa" },
  { "ES", 34,   OpalLineInterfaceDevice::Spain,                 "Spain" },
  { "LK", 94,   OpalLineInterfaceDevice::SriLanka,              "Sri Lanka" },
  { "SD", 249,  OpalLineInterfaceDevice::Sudan,                 "Sudan" },
  { "SR", 597,  OpalLineInterfaceDevice::Suriname,              "Suriname" },
  { "SZ", 268,  OpalLineInterfaceDevice::Swaziland,             "Swaziland" },
  { "SE", 46,   OpalLineInterfaceDevice::Sweden,                "Sweden" },
  { "CH", 41,   OpalLineInterfaceDevice::Switzerland,           "Switzerland" },
  { "SY", 963,  OpalLineInterfaceDevice::Syria,                 "Syria" },
  { "TZ", 255,  OpalLineInterfaceDevice::Tanzania,              "Tanzania" },
  { "TH", 66,   OpalLineInterfaceDevice::Thailand,              "Thailand" },
  { "TG", 228,  OpalLineInterfaceDevice::Togo,                  "Togo" },
  { "TO", 676,  OpalLineInterfaceDevice::Tonga,                 "Tonga" },
  { "TT", 1868, OpalLineInterfaceDevice::TrinidadAndTobago,     "Trinidad and Tobago" },
  { "TN", 216,  OpalLineInterfaceDevice::Tunisia,               "Tunisia" },
  { "TR", 90,   OpalLineInterfaceDevice::Turkey,                "Turkey",               { "450:0.2", "450:2.0-4.0", "450:0.5-0.5", "450:0.2-0.2", "450:0.2-0.2-0.2-0.2-0.2-0.2-0.6-0.2", "450:1.0-0.25" } },
  { "TC", 1649, OpalLineInterfaceDevice::TurksAndCaicosIslands, "Turks and Caicos Islands" },
  { "TV", 688,  OpalLineInterfaceDevice::Tuvalu,                "Tuvalu" },
  { "UG", 256,  OpalLineInterfaceDevice::Uganda,                "Uganda" },
  { "UA", 380,  OpalLineInterfaceDevice::Ukraine,               "Ukraine" },
  { "AE", 971,  OpalLineInterfaceDevice::UnitedArabEmirates,    "United Arab Emirates" },
  { "GB", 44,   OpalLineInterfaceDevice::UnitedKingdom,         "United Kingdom",       { "350+440:0.2", "400+450:0.4-0.2-0.4-2", "400:0.375-0.375", "400:0.35-0.225-0.525", "400:6-2",     "350+440:0.75/440:0.75" } },
  { "US", 1,    OpalLineInterfaceDevice::UnitedStates,          "United States",        { "350+440:0.2", "440+480:2.0-4.0",       "480+620:0.5-0.5", "480+620:0.3-0.2",      "350+440:0.5" } },
  { "UY", 598,  OpalLineInterfaceDevice::Uruguay,               "Uruguay" },
  { "VU", 678,  OpalLineInterfaceDevice::Vanuatu,               "Vanuatu" },
  { "VA", 379,  OpalLineInterfaceDevice::VaticanCityState,      "Vatican City State" },
  { "VE", 58,   OpalLineInterfaceDevice::Venezuela,             "Venezuela" },
  { "VN", 84,   OpalLineInterfaceDevice::VietNam,               "Viet Nam" },
  { "WF", 681,  OpalLineInterfaceDevice::WallisAndFutuna,       "Wallis and Futuna" },
  { "WS", 685,  OpalLineInterfaceDevice::WesternSamoa,          "Western Samoa" },
  { "YE", 967,  OpalLineInterfaceDevice::Yemen,                 "Yemen" },
  { "YU", 381,  OpalLineInterfaceDevice::Yugoslavia,            "Yugoslavia" },
  { "xx", 0,    OpalLineInterfaceDevice::Zaire,                 "Zaire" },
  { "ZM", 260,  OpalLineInterfaceDevice::Zambia,                "Zambia" },
  { "ZW", 263,  OpalLineInterfaceDevice::Zimbabwe,              "Zimbabwe" }
};

OpalLineInterfaceDevice::T35CountryCodes OpalLineInterfaceDevice::GetCountryCode(const PString & str)
{
  for (PINDEX i = 0; i < PARRAYSIZE(CountryInfo); i++) {
    if (str *= CountryInfo[i].fullName)
      return CountryInfo[i].t35Code;
  }

  return OpalLineInterfaceDevice::UnknownCountry;
}


PString OpalLineInterfaceDevice::GetCountryCodeName(T35CountryCodes c) 
{
  for (PINDEX i = 0; i < PARRAYSIZE(CountryInfo); i++) {
    if (CountryInfo[i].t35Code == c)
      return CountryInfo[i].fullName;
  }

  return "<Unknown>";
}


PString OpalLineInterfaceDevice::GetCountryCodeName() const
{ 
  return GetCountryCodeName(countryCode);
}


PBoolean OpalLineInterfaceDevice::SetCountryCode(T35CountryCodes country)
{
  for (PINDEX i = 0; i < PARRAYSIZE(CountryInfo); i++) {
    if (CountryInfo[i].t35Code == country) {
      PTRACE(3, "LID\tCountry set to \"" << CountryInfo[i].fullName << '"');
      for (unsigned line = 0; line < GetLineCount(); line++) {
        for (int tone = 0; tone < NumTones; tone++) {
          const char * toneStr = CountryInfo[i].tone[tone];
          if (toneStr == NULL) {
            toneStr = CountryInfo[UnitedStates].tone[tone];
            if (toneStr == NULL)
              toneStr = m_callProgressTones[tone];
          }
          SetToneDescription(line, (CallProgressTones)tone, toneStr);
          m_callProgressTones[tone] = toneStr;
        }
      }
      countryCode = country;
      return true;
    }
  }

  PTRACE(2, "LID\tCountry could not be set to \"" << GetCountryCodeName(country) <<"\", leaving as \"" << GetCountryCodeName() << '"');
  return false;
}


PStringList OpalLineInterfaceDevice::GetCountryCodeNameList() const
{
  PStringList list;
  for (PINDEX i = 0; i < PARRAYSIZE(CountryInfo); i++) {
    if (CountryInfo[i].tone[DialTone] != NULL)
      list.AppendString(CountryInfo[i].fullName);
  }
  return list;
}


static PCaselessString DeSpaced(const PString & orig)
{
  PString str = orig.Trim();

  PINDEX space = 0;
  while ((space = str.Find(' ')) != P_MAX_INDEX)
    str.Delete(space, 1);

  return str;
}


PBoolean OpalLineInterfaceDevice::SetCountryCodeName(const PString & countryName)
{
  PTRACE(4, "LID\tSetting country code name to \"" << countryName << '"');
  PCaselessString spacelessAndCaseless = DeSpaced(countryName);
  if (spacelessAndCaseless.IsEmpty())
    return PFalse;

  if (isdigit(spacelessAndCaseless[0]))
    return SetCountryCode((T35CountryCodes)spacelessAndCaseless.AsUnsigned());

  PINDEX i;
  if (spacelessAndCaseless[0] == '+') {
    unsigned code = spacelessAndCaseless.AsUnsigned();
    for (i = 0; i < PARRAYSIZE(CountryInfo); i++)
      if (code == CountryInfo[i].dialCode)
        return SetCountryCode(CountryInfo[i].t35Code);
  }
  else if (spacelessAndCaseless.GetLength() == 2) {
    for (i = 0; i < PARRAYSIZE(CountryInfo); i++)
      if (spacelessAndCaseless == CountryInfo[i].isoName)
        return SetCountryCode(CountryInfo[i].t35Code);
  }
  else {
    for (i = 0; i < PARRAYSIZE(CountryInfo); i++)
      if (spacelessAndCaseless == DeSpaced(CountryInfo[i].fullName))
        return SetCountryCode(CountryInfo[i].t35Code);
  }

  SetCountryCode(UnknownCountry);
  return PFalse;
}


PString OpalLineInterfaceDevice::GetErrorText() const
{
  return PChannel::GetErrorText(PChannel::Miscellaneous, osError);
}


void OpalLineInterfaceDevice::PrintOn(ostream & strm) const
{
  strm << GetDescription();
}


OpalLineInterfaceDevice * OpalLineInterfaceDevice::Create(const PString & newType,
                                                          void * parameters)
{
  OpalLIDRegistration * type = RegisteredLIDsListHead;
  while (type != NULL) {
    if (*type == newType)
      return type->Create(parameters);
    type = type->link;
  }

  return NULL;
}


OpalLineInterfaceDevice * OpalLineInterfaceDevice::CreateAndOpen(const PString & descriptor,
                                                                 void * parameters)
{
  PString deviceType, deviceName;

  PINDEX colon = descriptor.Find(':');
  if (colon != P_MAX_INDEX) {
    deviceType = descriptor.Left(colon).Trim();
    deviceName = descriptor.Mid(colon+1).Trim();
  }

  if (deviceType.IsEmpty() || deviceName.IsEmpty()) {
    PTRACE(1, "LID\tInvalid device description \"" << descriptor << '"');
    return NULL;
  }

  OpalLineInterfaceDevice * device = Create(deviceType, parameters);
  if (device == NULL)
    return NULL;

  if (device->Open(deviceName))
    return device;

  delete device;
  return NULL;
}


PStringList OpalLineInterfaceDevice::GetAllTypes()
{
  PStringList types;

  OpalLIDRegistration * type = RegisteredLIDsListHead;
  while (type != NULL) {
    types.AppendString(*type);
    type = type->link;
  }

  return types;
}


PStringList OpalLineInterfaceDevice::GetAllDevices()
{
  PStringList devices;

  OpalLIDRegistration * type = RegisteredLIDsListHead;
  while (type != NULL) {
    OpalLineInterfaceDevice * device = type->Create(NULL);
    PStringArray names = device->GetAllNames();
    for (PINDEX i = 0; i < names.GetSize(); i++)
      devices.AppendString(*type + ": " + names[i]);
    delete device;
    type = type->link;
  }

  return devices;
}


/////////////////////////////////////////////////////////////////////////////

OpalLine::OpalLine(OpalLineInterfaceDevice & dev, unsigned num, const char * userToken)
  : device(dev)
  , lineNumber(num)
  , token(userToken)
  , ringStoppedTime(0, 6)      // 6 seconds
  , ringInterCadenceTime(1500)  // 1.5 seconds
  , ringCount(0)
  , lastRingState(false)
{
  if (token.IsEmpty())
    token.sprintf("%s:%s:%u", (const char *)device.GetDeviceType(), (const char *)device.GetDeviceName(), lineNumber);
  
  PTRACE(4, "LID\tOpalLine constructed: device=" << dev.GetDeviceName() << ", num=" << num << ", token=" << token);
  
  ringCount = 0;
}


void OpalLine::PrintOn(ostream & strm) const
{
  strm << token;
}


PBoolean OpalLine::IsRinging(DWORD * cadence)
{
  PTimeInterval tick = PTimer::Tick();
  PTimeInterval delta = tick - ringTick;
  if (ringCount > 0 && delta > ringStoppedTime) {
    PTRACE(4, "LID\tRing count reset on line " << lineNumber);
    lastRingState = false;
    ringCount = 0;
  }

  if (device.IsLineRinging(lineNumber, cadence)) {
    ringTick = tick;
    if (lastRingState)
      return true;

    PTRACE_IF(4, ringCount == 0, "LID\tRing start detected on line " << lineNumber);
    ringCount++;
    lastRingState = true;
    return true;
  }

  if (lastRingState && delta > ringInterCadenceTime) {
    PTRACE(4, "LID\tRing cadence incremented on line " << lineNumber);
    lastRingState = false;
  }

  return lastRingState;
}


unsigned OpalLine::GetRingCount(DWORD * cadence)
{
  IsRinging(cadence);
  return ringCount;
}


/////////////////////////////////////////////////////////////////////////////

OpalLIDRegistration::OpalLIDRegistration(const char * name)
  : PCaselessString(name)
{
  OpalLIDRegistration * test = RegisteredLIDsListHead;
  while (test != NULL) {
    if (*test == *this) {
      duplicate = true;
      return;
    }
    test = test->link;
  }

  link = RegisteredLIDsListHead;
  RegisteredLIDsListHead = this;
  duplicate = false;
}


OpalLIDRegistration::~OpalLIDRegistration()
{
  if (duplicate)
    return;

  if (PAssertNULL(RegisteredLIDsListHead) == NULL)
    return;

  if (this == RegisteredLIDsListHead)
    RegisteredLIDsListHead = link;
  else {
    OpalLIDRegistration * previous = RegisteredLIDsListHead;
    while (this != previous->link) {
      previous = previous->link;
      if (PAssertNULL(previous) == NULL)
        return;
    }
    previous->link = link;
  }
}


/////////////////////////////////////////////////////////////////////////////
