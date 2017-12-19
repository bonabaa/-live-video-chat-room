/*
 * ivr.cxx
 *
 * Interactive Voice Response support.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2000 Equivalence Pty. Ltd.
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
 * $Revision: 23978 $
 * $Author: rjongbloed $
 * $Date: 2010-01-25 01:28:06 +0000 (Mon, 25 Jan 2010) $
 */

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "ivr.h"
#endif

#include <opal/buildopts.h>

#include <opal/ivr.h>
#include <opal/call.h>
#include <opal/patch.h>


#define new PNEW


#if OPAL_IVR

/////////////////////////////////////////////////////////////////////////////

OpalIVREndPoint::OpalIVREndPoint(OpalManager & mgr, const char * prefix)
  : OpalLocalEndPoint(mgr, prefix),
    defaultVXML("<?xml version=\"1.0\"?>\n"
                "<vxml version=\"1.0\">\n"
                "  <form id=\"root\">\n"
                "    <audio src=\"file:welcome.wav\">\n"
                "      This is the OPAL, V X M L test program, please speak after the tone.\n"
                "    </audio>\n"
                "    <record name=\"msg\" beep=\"true\" dtmfterm=\"true\" dest=\"file:recording.wav\" maxtime=\"10s\"/>\n"
                "  </form>\n"
                "</vxml>\n")
{
  defaultMediaFormats += OpalPCM16;
  defaultMediaFormats += OpalRFC2833;
#if OPAL_T38_CAPABILITY
  defaultMediaFormats += OpalCiscoNSE;
#endif

  PTRACE(4, "IVR\tCreated endpoint.");
}


OpalIVREndPoint::~OpalIVREndPoint()
{
  PTRACE(4, "IVR\tDeleted endpoint.");
}


PSafePtr<OpalConnection> OpalIVREndPoint::MakeConnection(OpalCall & call,
                                                    const PString & remoteParty,
                                                             void * userData,
                                                       unsigned int options,
                                    OpalConnection::StringOptions * stringOptions)
{
  PString ivrString = remoteParty;

  // First strip of the prefix if present
  PINDEX prefixLength = 0;
  if (ivrString.Find(GetPrefixName()+":") == 0)
    prefixLength = GetPrefixName().GetLength()+1;

  PString vxml = ivrString.Mid(prefixLength);
  if (vxml.Left(2) == "//")
    vxml = vxml.Mid(2);
  if (vxml.IsEmpty() || vxml == "*")
    vxml = defaultVXML;

  return AddConnection(CreateConnection(call, userData, vxml, options, stringOptions));
}


OpalMediaFormatList OpalIVREndPoint::GetMediaFormats() const
{
  PWaitAndSignal mutex(inUseFlag);
  return defaultMediaFormats;
}


OpalIVRConnection * OpalIVREndPoint::CreateConnection(OpalCall & call,
                                                      void * userData,
                                                      const PString & vxml,
                                                      unsigned int options,
                                                      OpalConnection::StringOptions * stringOptions)
{
  return new OpalIVRConnection(call, *this, userData, vxml, options, stringOptions);
}


void OpalIVREndPoint::SetDefaultVXML(const PString & vxml)
{
  inUseFlag.Wait();
  defaultVXML = vxml;
  inUseFlag.Signal();
}


void OpalIVREndPoint::SetDefaultMediaFormats(const OpalMediaFormatList & formats)
{
  inUseFlag.Wait();
  defaultMediaFormats = formats;
  inUseFlag.Signal();
}

PBoolean OpalIVREndPoint::StartVXML()
{
  return PFalse;
}


/////////////////////////////////////////////////////////////////////////////

OpalIVRConnection::OpalIVRConnection(OpalCall & call,
                                     OpalIVREndPoint & ep,
                                     void * userData,
                                     const PString & vxml,
                                     unsigned int options,
                                     OpalConnection::StringOptions * stringOptions)
  : OpalLocalConnection(call, ep, userData, options, stringOptions, 'I'),
    endpoint(ep),
    vxmlToLoad(vxml),
    vxmlMediaFormats(ep.GetMediaFormats()),
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4355)
#endif
    vxmlSession(this, PFactory<PTextToSpeech>::CreateInstance(ep.GetDefaultTextToSpeech()), true)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
{
  PTRACE(4, "IVR\tConstructed");
}


OpalIVRConnection::~OpalIVRConnection()
{
  PTRACE(4, "IVR\tDestroyed.");
}


PString OpalIVRConnection::GetLocalPartyURL() const
{
  return GetPrefixName() + ':' + vxmlToLoad.Left(vxmlToLoad.FindOneOf("\r\n"));
}


void OpalIVRConnection::OnEstablished()
{
  OpalConnection::OnEstablished();
  StartVXML();
}


PBoolean OpalIVRConnection::StartVXML()
{
  if (vxmlSession.IsPlaying()) {
    PTRACE(4, "IVR\tStartVXML, already playing");
    return true;
  }

  PStringToString & vars = vxmlSession.GetSessionVars();

  PString originator = m_connStringOptions(OPAL_OPT_ORIGINATOR_ADDRESS);
  if (originator.IsEmpty())
    originator = m_connStringOptions("Remote-Address");
  if (!originator.IsEmpty()) {
    PIPSocketAddressAndPort ap(originator);
    vars.SetAt("Source-IP-Address", ap.GetAddress().AsString());
  }

  vars.SetAt("Time", PTime().AsString());

  if (vxmlToLoad.IsEmpty()) 
    vxmlToLoad = endpoint.GetDefaultVXML();

  if (vxmlToLoad.IsEmpty())
    return endpoint.StartVXML();

  PCaselessString vxmlHead = vxmlToLoad.LeftTrim().Left(5);
  if (vxmlHead == "<?xml" || vxmlHead == "<vxml") {
    PTRACE(4, "IVR\tStartVXML:\n" << vxmlToLoad);
    return vxmlSession.LoadVXML(vxmlToLoad);
  }

  PURL vxmlURL(vxmlToLoad, NULL);
  if (vxmlURL.IsEmpty()) {
    PFilePath vxmlFile = vxmlToLoad;
    if (vxmlFile.GetType() *= ".vxml")
      return vxmlSession.LoadFile(vxmlFile);
  }
  else {
    if (vxmlURL.GetScheme() != "file" || (vxmlURL.AsFilePath().GetType() *= ".vxml"))
      return vxmlSession.LoadURL(vxmlURL);
  }

  ////////////////////////////////

  PINDEX repeat = 1;
  PINDEX delay = 0;
  PString voice;

  PINDEX i;
  PStringArray tokens = vxmlToLoad.Tokenise(';', PFalse);
  for (i = 0; i < tokens.GetSize(); ++i) {
    PString str(tokens[i]);

    if (str.Find("file:") == 0) {
      PURL url = str;
      if (url.IsEmpty()) {
        PTRACE(2, "IVR\tInvalid URL \"" << str << '"');
        continue;
      }

      PFilePath fn = url.AsFilePath();
      if (fn.IsEmpty()) {
        PTRACE(2, "IVR\tUnsupported host in URL " << url);
        continue;
      }

      if (!voice.IsEmpty())
        fn = fn.GetDirectory() + voice + PDIR_SEPARATOR + fn.GetFileName();

      PTRACE(3, "IVR\tPlaying file " << fn << ' ' << repeat << " times, " << delay << "ms");
      vxmlSession.PlayFile(fn, repeat, delay);
      continue;
    }

    PINDEX pos = str.Find("=");
    PString key(str);
    PString val;
    if (pos != P_MAX_INDEX) {
      key = str.Left(pos);
      val = str.Mid(pos+1);
    }

    if (key *= "repeat") {
      if (!val.IsEmpty())
        repeat = val.AsInteger();
    }
    else if (key *= "delay") {
      if (!val.IsEmpty())
        delay = val.AsInteger();
    }
    else if (key *= "voice") {
      if (!val.IsEmpty()) {
        voice = val;
        PTextToSpeech * tts = vxmlSession.GetTextToSpeech();
        if (tts != NULL)
          tts->SetVoice(voice);
      }
    }

    else if (key *= "tone") {
      PTRACE(3, "IVR\tPlaying tone " << val);
      vxmlSession.PlayTone(val, repeat, delay);
   }

    else if (key *= "speak") {
      if (!val.IsEmpty() && (val[0] == '$'))
        val = vars(val.Mid(1));
      PTRACE(3, "IVR\tSpeaking text '" << val << "'");
      vxmlSession.PlayText(val, PTextToSpeech::Default, repeat, delay);
    }

    else if (key *= "silence") {
      unsigned msecs;
      if (val.IsEmpty() && (val[0] == '$'))
        msecs = vars(val.Mid(1)).AsUnsigned();
      else
        msecs = val.AsUnsigned();
      PTRACE(3, "IVR\tSpeaking silence for " << msecs << " msecs");
      vxmlSession.PlaySilence(msecs);
    }
  }

  vxmlSession.PlayStop();
  vxmlSession.Trigger();

  return PTrue;
}


OpalMediaFormatList OpalIVRConnection::GetMediaFormats() const
{
  return vxmlMediaFormats;
}


OpalMediaStream * OpalIVRConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                       unsigned sessionID,
                                                       PBoolean isSource)
{
  return new OpalIVRMediaStream(*this, mediaFormat, sessionID, isSource, vxmlSession);
}


PBoolean OpalIVRConnection::SendUserInputString(const PString & value)
{
  PTRACE(3, "IVR\tSendUserInputString(" << value << ')');

  for (PINDEX i = 0; i < value.GetLength(); i++)
    vxmlSession.OnUserInput(value[i]);

  return PTrue;
}

void OpalIVRConnection::OnStopMediaPatch(OpalMediaPatch & patch)
{
  // lose the audio, then lose the call
  if (patch.GetSource().GetMediaFormat().GetMediaType() == OpalMediaType::Audio()) {
    synchronousOnRelease = false; // Deadlocks if try to do it synchronously ...
    Release();
  }
}



/////////////////////////////////////////////////////////////////////////////

OpalIVRMediaStream::OpalIVRMediaStream(OpalIVRConnection & _conn,
                                       const OpalMediaFormat & mediaFormat,
                                       unsigned sessionID,
                                       PBoolean isSourceStream,
                                       PVXMLSession & vxml)
  : OpalRawMediaStream(_conn, mediaFormat, sessionID, isSourceStream, &vxml, FALSE),
    conn(_conn), vxmlSession(vxml)
{
  PTRACE(3, "IVR\tOpalIVRMediaStream sessionID = " << sessionID << ", isSourceStream = " << isSourceStream);
}


PBoolean OpalIVRMediaStream::Open()
{
  PTRACE(3, "IVR\tOpen");
  if (isOpen)
    return PTrue;

  if (vxmlSession.IsOpen()) {
    PVXMLChannel * vxmlChannel = vxmlSession.GetAndLockVXMLChannel();
    PString vxmlChannelMediaFormat;
    
    if (vxmlChannel == NULL) {
      PTRACE(1, "IVR\tVXML engine not really open");
      return PFalse;
    }

    vxmlChannelMediaFormat = vxmlChannel->GetMediaFormat();
    vxmlSession.UnLockVXMLChannel();
    
    if (mediaFormat.GetName() != vxmlChannelMediaFormat) {
      PTRACE(1, "IVR\tCannot use VXML engine: asymmetrical media formats: " << mediaFormat << " <-> " << vxmlChannelMediaFormat);
      return PFalse;
    }

    return OpalMediaStream::Open();
  }

  if (vxmlSession.Open(mediaFormat))
    return OpalMediaStream::Open();

  PTRACE(1, "IVR\tCannot open VXML engine: incompatible media format");
  return PFalse;
}

PBoolean OpalIVRMediaStream::IsSynchronous() const
{
  return true;
}


#endif // OPAL_IVR


/////////////////////////////////////////////////////////////////////////////
