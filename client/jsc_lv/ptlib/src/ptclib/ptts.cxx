/*
 * ptts.cxx
 *
 * Text To Speech classes
 *
 * Portable Windows Library
 *
 * Copyright (c) 2002 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23589 $
 * $Author: rjongbloed $
 * $Date: 2009-10-06 00:11:03 +0000 (Tue, 06 Oct 2009) $
 */

#ifdef __GNUC__
#pragma implementation "ptts.h"
#endif

#include "ptbuildopts.h"

////////////////////////////////////////////////////////////
#include <ptlib/pfactory.h>
#include <ptclib/ptts.h>


// WIN32 COM stuff must be first in file to compile properly

#if P_SAPI

#if defined(P_SAPI_LIBRARY)
#pragma comment(lib, P_SAPI_LIBRARY)
#endif

#ifndef _WIN32_DCOM
#define _WIN32_DCOM 1
#endif

#include <objbase.h>
#include <atlbase.h>
#include <windows.h>
#include <windowsx.h>
#include <sphelper.h>

#endif

////////////////////////////////////////////////////////////

// this disables the winsock2 stuff in the Windows contain.h, to avoid header file problems
#define P_KNOCKOUT_WINSOCK2

#include <ptlib.h>
#include <ptlib/pipechan.h>
#include <ptclib/ptts.h>


////////////////////////////////////////////////////////////
//
// Text to speech using Microsoft's Speech API (SAPI)
// Can be downloaded from http://www.microsoft.com/speech/download/sdk51
//

#if P_SAPI

#define MAX_FN_SIZE 1024

class PTextToSpeech_SAPI : public PTextToSpeech
{
  PCLASSINFO(PTextToSpeech_SAPI, PTextToSpeech);
  public:
    PTextToSpeech_SAPI();
    ~PTextToSpeech_SAPI();

    // overrides
    PStringArray GetVoiceList();
    PBoolean SetVoice(const PString & voice);

    PBoolean SetRate(unsigned rate);
    unsigned GetRate();

    PBoolean SetVolume(unsigned volume);
    unsigned GetVolume();

    PBoolean OpenFile   (const PFilePath & fn);
    PBoolean OpenChannel(PChannel * channel);
    PBoolean IsOpen()     { return opened; }

    PBoolean Close      ();
    PBoolean Speak      (const PString & str, TextType hint);

  protected:
    PBoolean OpenVoice();

    static PMutex refMutex;
    static int * refCount;

    PMutex mutex;
    CComPtr<ISpVoice> m_cpVoice;
    CComPtr<ISpStream> cpWavStream;
    PBoolean opened;
    PBoolean usingFile;
    unsigned rate, volume;
    PString voice;
};

PFACTORY_CREATE(PTextToSpeech, PTextToSpeech_SAPI, "Microsoft SAPI", false);

int * PTextToSpeech_SAPI::refCount;
PMutex PTextToSpeech_SAPI::refMutex;


#define new PNEW


PTextToSpeech_SAPI::PTextToSpeech_SAPI()
{
  PWaitAndSignal m(refMutex);

  if (refCount == NULL) {
    refCount = new int;
    *refCount = 1;
    ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
  } else {
    (*refCount)++;
  }

  usingFile = opened = PFalse;
}


PTextToSpeech_SAPI::~PTextToSpeech_SAPI()
{
  PWaitAndSignal m(refMutex);

  if ((--(*refCount)) == 0) {
    ::CoUninitialize();
    delete refCount;
    refCount = NULL;
  }
}

PBoolean PTextToSpeech_SAPI::OpenVoice()
{
  PWaitAndSignal m(mutex);

  HRESULT hr = m_cpVoice.CoCreateInstance(CLSID_SpVoice);
  return (opened = SUCCEEDED(hr));
}

PBoolean PTextToSpeech_SAPI::OpenChannel(PChannel *)
{
  PWaitAndSignal m(mutex);

  Close();
  usingFile = PFalse;
  return (opened = PFalse);
}


PBoolean PTextToSpeech_SAPI::OpenFile(const PFilePath & fn)
{
  PWaitAndSignal m(mutex);

  Close();
  usingFile = PTrue;

  if (!OpenVoice())
    return PFalse;

  CSpStreamFormat wavFormat;
  wavFormat.AssignFormat(SPSF_8kHz16BitMono);

  WCHAR szwWavFileName[MAX_FN_SIZE] = L"";;

  USES_CONVERSION;
  wcscpy(szwWavFileName, T2W((const char *)fn));
  HRESULT hr = SPBindToFile(szwWavFileName, SPFM_CREATE_ALWAYS, &cpWavStream, &wavFormat.FormatId(), wavFormat.WaveFormatExPtr()); 

  if (!SUCCEEDED(hr)) {
    cpWavStream.Release();
    return PFalse;
  }

  hr = m_cpVoice->SetOutput(cpWavStream, PTrue);

  return (opened = SUCCEEDED(hr));
}

PBoolean PTextToSpeech_SAPI::Close()
{
  PWaitAndSignal m(mutex);

  if (!opened)
    return PTrue;

  if (usingFile) {
    if (opened)
      m_cpVoice->WaitUntilDone(INFINITE);
    cpWavStream.Release();
  }

  if (opened)
    m_cpVoice.Release();

  opened = PFalse;

  return PTrue;
}


PBoolean PTextToSpeech_SAPI::Speak(const PString & otext, TextType hint)
{
  PWaitAndSignal m(mutex);

  if (!IsOpen())
    return PFalse;

  PString text = otext;

  // do various things to the string, depending upon the hint
  switch (hint) {
    case Digits:
      {
      }
      break;

    default:
    ;
  };

  // quick hack to calculate length of Unicode string
  WCHAR * uStr = new WCHAR[text.GetLength()+1];

  USES_CONVERSION;
  wcscpy(uStr, T2W((const char *)text));

  HRESULT hr = m_cpVoice->Speak(uStr, SPF_DEFAULT, NULL);

  delete[] uStr;

  return SUCCEEDED(hr);
}

PStringArray PTextToSpeech_SAPI::GetVoiceList()
{
  PWaitAndSignal m(mutex);

  PStringArray voiceList;

  CComPtr<ISpObjectToken> cpVoiceToken;
  CComPtr<IEnumSpObjectTokens> cpEnum;
  ULONG ulCount = 0;

  //Enumerate the available voices 
  HRESULT hr = SpEnumTokens(SPCAT_VOICES, NULL, NULL, &cpEnum);

  // Get the number of voices
  if (SUCCEEDED(hr))
    hr = cpEnum->GetCount(&ulCount);

  // Obtain a list of available voice tokens, set the voice to the token, and call Speak
  while (SUCCEEDED(hr) && ulCount--) {

    cpVoiceToken.Release();

    if (SUCCEEDED(hr))
      hr = cpEnum->Next(1, &cpVoiceToken, NULL );

    if (SUCCEEDED(hr)) {
      voiceList.AppendString("voice");
    }
  } 

  return voiceList;
}

PBoolean PTextToSpeech_SAPI::SetVoice(const PString & v)
{
  PWaitAndSignal m(mutex);
  voice = v;
  return PTrue;
}

PBoolean PTextToSpeech_SAPI::SetRate(unsigned v)
{
  rate = v;
  return PTrue;
}

unsigned PTextToSpeech_SAPI::GetRate()
{
  return rate;
}

PBoolean PTextToSpeech_SAPI::SetVolume(unsigned v)
{
  volume = v;
  return PTrue;
}

unsigned PTextToSpeech_SAPI::GetVolume()
{
  return volume;
}

#endif
// P_SAPI


#ifndef _WIN32_WCE

////////////////////////////////////////////////////////////
//
//  Generic text to speech using Festival
//

#undef new

class PTextToSpeech_Festival : public PTextToSpeech
{
  PCLASSINFO(PTextToSpeech_Festival, PTextToSpeech);
  public:
    PTextToSpeech_Festival();
    ~PTextToSpeech_Festival();

    // overrides
    PStringArray GetVoiceList();
    PBoolean SetVoice(const PString & voice);

    PBoolean SetRate(unsigned rate);
    unsigned GetRate();

    PBoolean SetVolume(unsigned volume);
    unsigned GetVolume();

    PBoolean OpenFile   (const PFilePath & fn);
    PBoolean OpenChannel(PChannel * channel);
    PBoolean IsOpen()    { return opened; }

    PBoolean Close      ();
    PBoolean Speak      (const PString & str, TextType hint);

  protected:
    PBoolean Invoke(const PString & str, const PFilePath & fn);

    PMutex mutex;
    PBoolean opened;
    PBoolean usingFile;
    PString text;
    PFilePath path;
    unsigned volume, rate;
    PString voice;
};

#define new PNEW

PFACTORY_CREATE(PFactory<PTextToSpeech>, PTextToSpeech_Festival, "Festival", false);

PTextToSpeech_Festival::PTextToSpeech_Festival()
{
  PWaitAndSignal m(mutex);
  usingFile = opened = PFalse;
  rate = 8000;
  volume = 100;
}


PTextToSpeech_Festival::~PTextToSpeech_Festival()
{
  PWaitAndSignal m(mutex);
}

PBoolean PTextToSpeech_Festival::OpenChannel(PChannel *)
{
  PWaitAndSignal m(mutex);

  Close();
  usingFile = PFalse;
  opened = PFalse;

  return PTrue;
}


PBoolean PTextToSpeech_Festival::OpenFile(const PFilePath & fn)
{
  PWaitAndSignal m(mutex);

  Close();
  usingFile = PTrue;
  path = fn;
  opened = PTrue;

  PTRACE(3, "TTS\tWriting speech to " << fn);

  return PTrue;
}

PBoolean PTextToSpeech_Festival::Close()
{
  PWaitAndSignal m(mutex);

  if (!opened)
    return PTrue;

  PBoolean stat = PFalse;

  if (usingFile)
    stat = Invoke(text, path);

  text = PString();

  opened = PFalse;

  return stat;
}


PBoolean PTextToSpeech_Festival::Speak(const PString & ostr, TextType hint)
{
  PWaitAndSignal m(mutex);

  if (!IsOpen()) {
    PTRACE(2, "TTS\tAttempt to speak whilst engine not open");
    return PFalse;
  }

  PString str = ostr;

  // do various things to the string, depending upon the hint
  switch (hint) {
    case Digits:
    default:
    ;
  };

  if (usingFile) {
    PTRACE(3, "TTS\tSpeaking " << ostr);
    text = text & str;
    return PTrue;
  }

  PTRACE(1, "TTS\tStream mode not supported for Festival");

  return PFalse;
}

PStringArray PTextToSpeech_Festival::GetVoiceList()
{
  PWaitAndSignal m(mutex);

  PStringArray voiceList;

  voiceList.AppendString("default");

  return voiceList;
}

PBoolean PTextToSpeech_Festival::SetVoice(const PString & v)
{
  PWaitAndSignal m(mutex);
  voice = v;
  return PTrue;
}

PBoolean PTextToSpeech_Festival::SetRate(unsigned v)
{
  rate = v;
  return PTrue;
}

unsigned PTextToSpeech_Festival::GetRate()
{
  return rate;
}

PBoolean PTextToSpeech_Festival::SetVolume(unsigned v)
{
  volume = v;
  return PTrue;
}

unsigned PTextToSpeech_Festival::GetVolume()
{
  return volume;
}

PBoolean PTextToSpeech_Festival::Invoke(const PString & otext, const PFilePath & fname)
{
  PString text = otext;
  text.Replace('\n', ' ', PTrue);
  text.Replace('\"', '\'', PTrue);
  text.Replace('\\', ' ', PTrue);
  text = "\"" + text + "\"";

  PString cmdLine = "echo " + text + " | ./text2wave -F " + PString(PString::Unsigned, rate) + " -otype riff > " + fname;

#if 1

  return system(cmdLine) != -1;

#else

  PPipeChannel cmd;
  int code = -1;
  if (!cmd.Open(cmdLine, PPipeChannel::ReadWriteStd)) {
    PTRACE(1, "TTS\tCannot execute command " << cmd);
  } else {
    PTRACE(3, "TTS\tCreating " << fname << " using " << cmdLine);
    cmd.Execute();
    code = cmd.WaitForTermination();
    if (code >= 0) {
      PTRACE(4, "TTS\tdata generated");
    } else {
      PTRACE(1, "TTS\tgeneration failed");
    }
  }

  return code == 0;

#endif
}


#endif


// End Of File ///////////////////////////////////////////////////////////////
