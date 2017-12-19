/*
 * vxml.h
 *
 * VXML engine for pwlib library
 *
 * Copyright (C) 2002 Equivalence Pty. Ltd.
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
 * $Revision: 23021 $
 * $Author: rjongbloed $
 * $Date: 2009-07-01 08:40:15 +0000 (Wed, 01 Jul 2009) $
 */

#ifndef PTLIB_VXML_H
#define PTLIB_VXML_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <ptclib/pxml.h>

#if P_VXML

#include <ptlib/pipechan.h>
#include <ptclib/delaychan.h>
#include <ptclib/pwavfile.h>
#include <ptclib/ptts.h>
#include <ptclib/url.h>

#include <queue>


class PVXMLSession;
class PVXMLDialog;
class PVXMLSession;

// these are the same strings as the Opal equivalents, but as this is PWLib, we can't use Opal contants
#define VXML_PCM16         "PCM-16"
#define VXML_G7231         "G.723.1"
#define VXML_G729          "G.729"

#define PVXML_HAS_FACTORY   1

class PVXMLGrammar : public PObject
{
  PCLASSINFO(PVXMLGrammar, PObject);
  public:
    PVXMLGrammar(PXMLElement * field);
    virtual PBoolean OnUserInput(const char /*ch*/) { return PTrue; }
    virtual void Stop() { }

    PString GetValue() const { return value; }
    PXMLElement * GetField() { return field; }

    enum GrammarState { 
      FILLED,       ///< got something that matched the grammar
      NOINPUT,      ///< timeout or still waiting to match
      NOMATCH,      ///< recognized something but didn't match the grammar
      HELP };       ///< help keyword

    GrammarState GetState() const { return state; }

  protected:
    PXMLElement * field;
    PString value;
    GrammarState state;
};


//////////////////////////////////////////////////////////////////

class PVXMLMenuGrammar : public PVXMLGrammar
{
  PCLASSINFO(PVXMLMenuGrammar, PVXMLGrammar);
  public:
    PVXMLMenuGrammar(PXMLElement * field);
};


//////////////////////////////////////////////////////////////////

class PVXMLDigitsGrammar : public PVXMLGrammar
{
  PCLASSINFO(PVXMLDigitsGrammar, PVXMLGrammar);
  public:
    PVXMLDigitsGrammar(PXMLElement * field, PINDEX minDigits, PINDEX maxDigits, PString terminators);
    PBoolean OnUserInput(const char ch);
    virtual void Stop();

  protected:
    PINDEX minDigits;
    PINDEX maxDigits;
    PString terminators;
};


//////////////////////////////////////////////////////////////////

class PVXMLCache : public PMutex
{
  public:
    PVXMLCache(const PDirectory & directory);

    PFilePath CreateFilename(const PString & prefix, const PString & key, const PString & fileType);

    void Put(const PString & prefix,
             const PString & key, 
             const PString & fileType, 
             const PString & contentType,       
           const PFilePath & fn, 
                 PFilePath & dataFn);

    PBoolean Get(const PString & prefix,
             const PString & key, 
             const PString & fileType, 
                   PString & contentType,       
                 PFilePath & fn);

    PFilePath GetCacheDir() const
    { return directory; }

    PFilePath GetRandomFilename(const PString & prefix, const PString & fileType);

    static PVXMLCache & GetResourceCache();

  protected:
    PDirectory directory;
};

//////////////////////////////////////////////////////////////////

class PVXMLChannel;

class PVXMLChannelInterface {
  public:
    virtual ~PVXMLChannelInterface() { }
    virtual PWAVFile * CreateWAVFile(const PFilePath & fn, PFile::OpenMode mode, int opts, unsigned fmt) = 0;
    virtual void RecordEnd() = 0;
    virtual void OnEndRecording(const PString & channelName) = 0;
    virtual void Trigger() = 0;
};

//////////////////////////////////////////////////////////////////

class PVXMLSession : public PIndirectChannel, public PVXMLChannelInterface
{
  PCLASSINFO(PVXMLSession, PIndirectChannel);
  public:
    PVXMLSession(PTextToSpeech * tts = NULL, PBoolean autoDelete = PFalse);
    virtual ~PVXMLSession();

    // new functions
    PTextToSpeech * SetTextToSpeech(PTextToSpeech * tts, PBoolean autoDelete = false);
    PTextToSpeech * SetTextToSpeech(const PString & ttsName);
    PTextToSpeech * GetTextToSpeech() { return textToSpeech; }

    virtual PBoolean Load(const PString & source);
    virtual PBoolean LoadFile(const PFilePath & file);
    virtual PBoolean LoadURL(const PURL & url);
    virtual PBoolean LoadVXML(const PString & xml);
    virtual PBoolean IsLoaded() const { return loaded; }

    virtual PBoolean Open(const PString & mediaFormat);
    virtual PBoolean Close();

    PBoolean Execute();

    PVXMLChannel * GetAndLockVXMLChannel() 
    { 
      sessionMutex.Wait(); 
      if (vxmlChannel != NULL) 
        return vxmlChannel; 
      sessionMutex.Signal();
      return NULL;
    }
    void UnLockVXMLChannel() { sessionMutex.Signal(); }
    PMutex & GetSessionMutex() { return sessionMutex; }

    PBoolean LoadGrammar(PVXMLGrammar * grammar);

    virtual PBoolean PlayText(const PString & text, PTextToSpeech::TextType type = PTextToSpeech::Default, PINDEX repeat = 1, PINDEX delay = 0);
    PBoolean ConvertTextToFilenameList(const PString & text, PTextToSpeech::TextType type, PStringArray & list, PBoolean useCacheing);

    virtual PBoolean PlayFile(const PString & fn, PINDEX repeat = 1, PINDEX delay = 0, PBoolean autoDelete = PFalse);
    virtual PBoolean PlayData(const PBYTEArray & data, PINDEX repeat = 1, PINDEX delay = 0);
    virtual PBoolean PlayCommand(const PString & data, PINDEX repeat = 1, PINDEX delay = 0);
    virtual PBoolean PlayResource(const PURL & url, PINDEX repeat = 1, PINDEX delay = 0);
    virtual PBoolean PlayTone(const PString & toneSpec, PINDEX repeat = 1, PINDEX delay = 0);

    //virtual PBoolean PlayMedia(const PURL & url, PINDEX repeat = 1, PINDEX delay = 0);
    virtual PBoolean PlaySilence(PINDEX msecs = 0);
    virtual PBoolean PlaySilence(const PTimeInterval & timeout);

    virtual PBoolean PlayStop();

    virtual void SetPause(PBoolean pause);
    virtual void GetBeepData(PBYTEArray & data, unsigned ms);

    virtual PBoolean StartRecording(const PFilePath & fn, PBoolean recordDTMFTerm, const PTimeInterval & recordMaxTime, const PTimeInterval & recordFinalSilence);
    virtual PBoolean EndRecording();
    virtual PBoolean IsPlaying() const;
    virtual PBoolean IsRecording() const;

    virtual PBoolean OnUserInput(const PString & str);

    PString GetXMLError() const;

    virtual void OnEndSession()         { }
    virtual void OnTransfer(const PString & /*destination*/, bool /*bridged*/) { }

    virtual PString GetVar(const PString & str) const;
    virtual void SetVar(const PString & ostr, const PString & val);
    virtual PString EvaluateExpr(const PString & oexpr);

    virtual PBoolean RetreiveResource(const PURL & url, PString & contentType, PFilePath & fn, PBoolean useCache = PTrue);

    PDECLARE_NOTIFIER(PThread, PVXMLSession, VXMLExecute);

    void SetCallingToken( PString& token ) { callingCallToken = token; }

    PXMLElement * FindHandler(const PString & event);

    // overrides from VXMLChannelInterface
    PWAVFile * CreateWAVFile(const PFilePath & fn, PFile::OpenMode mode, int opts, unsigned fmt);
    void OnEndRecording(const PString & channelName);
    void RecordEnd();
    void Trigger();

    PStringToString & GetSessionVars() { return sessionVars; }

  protected:
    void Initialise();

    void AllowClearCall();
    void ProcessUserInput();
    void ProcessNode();
    void ProcessGrammar();

    PBoolean TraverseAudio();
    PBoolean TraverseGoto();
    PBoolean TraverseGrammar();
    PBoolean TraverseRecord();

    PBoolean TraverseIf();
    PBoolean TraverseExit();
    PBoolean TraverseVar();
    PBoolean TraverseSubmit();
    PBoolean TraverseMenu();
    PBoolean TraverseChoice();
    PBoolean TraverseProperty();
    PBoolean TraverseTransfer();

    void SayAs(const PString & className, const PString & text);
    void SayAs(const PString & className, const PString & text, const PString & voice);

    static PTimeInterval StringToTime(const PString & str);

    PURL NormaliseResourceName(const PString & src);

    PXMLElement * FindForm(const PString & id);

    PSyncPoint waitForEvent;

    PMutex sessionMutex;

    PXML xmlFile;

    PVXMLGrammar * activeGrammar;
    PBoolean listening;                 // PTrue if waiting for recognition events
    int timeout;                    // timeout in msecs for the current recognition

    PStringToString sessionVars;
    PStringToString documentVars;

    PMutex userInputMutex;
    std::queue<char> userInputQueue;

    PBoolean recording;
    PFilePath recordFn;
    PBoolean recordDTMFTerm;
    PTimeInterval recordMaxTime;
    PTimeInterval recordFinalSilence;
    PSyncPoint    recordSync;

    PBoolean loaded;
    PURL rootURL;
    PBoolean emptyAction;

    PThread * vxmlThread;
    PBoolean threadRunning;
    PBoolean forceEnd;

    PString mediaFormat;
    PVXMLChannel * & vxmlChannel;

    PTextToSpeech * textToSpeech;
    PBoolean autoDeleteTextToSpeech;

    PXMLElement * currentForm;
    PXMLElement * currentField;
    PXMLObject  * currentNode;
    bool          m_speakNodeData;

  private:
    void      ExecuteDialog();

    PString       callingCallToken;
    PSyncPoint    answerSync;
    PString       grammarResult;
    PString       eventName;
    PINDEX        defaultDTMF;
};


//////////////////////////////////////////////////////////////////

class PVXMLRecordable : public PObject
{
  PCLASSINFO(PVXMLRecordable, PObject);
  public:
    PVXMLRecordable()
    { consecutiveSilence = 0; finalSilence = 3000; maxDuration = 30000; }

    virtual PBoolean Open(const PString & arg) = 0;

    virtual void Record(PVXMLChannel & incomingChannel) = 0;

    virtual void OnStart() { }

    virtual PBoolean OnFrame(PBoolean /*isSilence*/) { return PFalse; }

    virtual void OnStop() { }

    void SetFinalSilence(unsigned v)
    { finalSilence = v; }

    unsigned GetFinalSilence()
    { return finalSilence; }

    void SetMaxDuration(unsigned v)
    { maxDuration = v; }

    unsigned GetMaxDuration()
    { return maxDuration; }

  protected:
    PTime silenceStart;
    PTime recordStart;
    unsigned finalSilence;
    unsigned maxDuration;
    unsigned consecutiveSilence;
};

//////////////////////////////////////////////////////////////////

class PVXMLPlayable : public PObject
{
  PCLASSINFO(PVXMLPlayable, PObject);
  public:
    PVXMLPlayable()
    { repeat = 1; delay = 0; sampleFrequency = 8000; autoDelete = PFalse; delayDone = PFalse; }

    virtual PBoolean Open(PVXMLChannel & /*chan*/, PINDEX delay, PINDEX repeat, PBoolean autoDelete);

    virtual PBoolean Open(PVXMLChannel & chan, const PString & arg, PINDEX delay, PINDEX repeat, PBoolean v);

    virtual void Play(PVXMLChannel & outgoingChannel) = 0;

    virtual void OnRepeat(PVXMLChannel & /*outgoingChannel*/)
    { }

    virtual void OnStart() { }

    virtual void OnStop() { }

    virtual void SetRepeat(PINDEX v) 
    { repeat = v; }

    virtual PINDEX GetRepeat() const
    { return repeat; }

    virtual PINDEX GetDelay() const
    { return delay; }

    void SetFormat(const PString & fmt)
    { format = fmt; }

    void SetSampleFrequency(unsigned rate)
    { sampleFrequency = rate; }

    virtual PBoolean ReadFrame(PVXMLChannel & channel, void * buf, PINDEX len);

    virtual PBoolean Rewind(PChannel *) 
    { return PFalse; }

    friend class PVXMLChannel;

  protected:
    PString arg;
    PINDEX repeat;
    PINDEX delay;
    PString format;
    unsigned sampleFrequency;
    PBoolean autoDelete;
    PBoolean delayDone; // very tacky flag used to indicate when the post-play delay has been done
};

//////////////////////////////////////////////////////////////////

class PVXMLPlayableStop : public PVXMLPlayable
{
  PCLASSINFO(PVXMLPlayableStop, PVXMLPlayable);
  public:
    virtual void Play(PVXMLChannel & outgoingChannel);
    virtual PBoolean ReadFrame(PVXMLChannel & channel, void *, PINDEX);
};

//////////////////////////////////////////////////////////////////

class PVXMLPlayableURL : public PVXMLPlayable
{
  PCLASSINFO(PVXMLPlayableURL, PVXMLPlayable);
  public:
    PBoolean Open(PVXMLChannel & chan, const PString & url, PINDEX delay, PINDEX repeat, PBoolean v);
    void Play(PVXMLChannel & outgoingChannel);
  protected:
    PURL url;
};

//////////////////////////////////////////////////////////////////

class PVXMLPlayableData : public PVXMLPlayable
{
  PCLASSINFO(PVXMLPlayableData, PVXMLPlayable);
  public:
    PBoolean Open(PVXMLChannel & chan, const PString & fn, PINDEX delay, PINDEX repeat, PBoolean v);
    void SetData(const PBYTEArray & data);
    void Play(PVXMLChannel & outgoingChannel);
    PBoolean Rewind(PChannel * chan);
  protected:
    PBYTEArray data;
};

//////////////////////////////////////////////////////////////////

#include <ptclib/dtmf.h>

class PVXMLPlayableTone : public PVXMLPlayableData
{
  PCLASSINFO(PVXMLPlayableTone, PVXMLPlayableData);
  public:
    PBoolean Open(PVXMLChannel & chan, const PString & toneSpec, PINDEX delay, PINDEX repeat, PBoolean v);
  protected:
    PTones tones;
};

//////////////////////////////////////////////////////////////////

class PVXMLPlayableCommand : public PVXMLPlayable
{
  PCLASSINFO(PVXMLPlayableCommand, PVXMLPlayable);
  public:
    PVXMLPlayableCommand();
    void Play(PVXMLChannel & outgoingChannel);
    void OnStop();

  protected:
    PPipeChannel * pipeCmd;
};

//////////////////////////////////////////////////////////////////

class PVXMLPlayableFilename : public PVXMLPlayable
{
  PCLASSINFO(PVXMLPlayableFilename, PVXMLPlayable);
  public:
    PBoolean Open(PVXMLChannel & chan, const PString & fn, PINDEX delay, PINDEX repeat, PBoolean autoDelete);
    void Play(PVXMLChannel & outgoingChannel);
    void OnStop();
    virtual PBoolean Rewind(PChannel * chan);
  protected:
    PFilePath fn;
};

//////////////////////////////////////////////////////////////////

class PVXMLPlayableFilenameList : public PVXMLPlayable
{
  PCLASSINFO(PVXMLPlayableFilenameList, PVXMLPlayable);
  public:
    PBoolean Open(PVXMLChannel & chan, const PStringArray & filenames, PINDEX delay, PINDEX repeat, PBoolean autoDelete);
    void Play(PVXMLChannel & outgoingChannel)
    { OnRepeat(outgoingChannel); }
    void OnRepeat(PVXMLChannel & outgoingChannel);
    void OnStop();
  protected:
    PINDEX currentIndex;
    PStringArray filenames;
};

//////////////////////////////////////////////////////////////////

class PVXMLRecordableFilename : public PVXMLRecordable
{
  PCLASSINFO(PVXMLRecordableFilename, PVXMLRecordable);
  public:
    PBoolean Open(const PString & arg);
    void Record(PVXMLChannel & incomingChannel);
    PBoolean OnFrame(PBoolean isSilence);

  protected:
    PFilePath fn;
};

//////////////////////////////////////////////////////////////////

PQUEUE(PVXMLQueue, PVXMLPlayable);

//////////////////////////////////////////////////////////////////

class PVXMLChannel : public PDelayChannel
{
  PCLASSINFO(PVXMLChannel, PDelayChannel);
  public:
    PVXMLChannel(unsigned frameDelay, PINDEX frameSize);
    ~PVXMLChannel();

    virtual PBoolean Open(PVXMLChannelInterface * vxml);

    // overrides from PIndirectChannel
    virtual PBoolean IsOpen() const;
    virtual PBoolean Close();
    virtual PBoolean Read(void * buffer, PINDEX amount);
    virtual PBoolean Write(const void * buf, PINDEX len);

    // new functions
    virtual PWAVFile * CreateWAVFile(const PFilePath & fn, PBoolean recording = PFalse);

    const PString & GetMediaFormat() const { return mediaFormat; }
    PBoolean IsMediaPCM() const { return mediaFormat == "PCM-16"; }
    virtual PString AdjustWavFilename(const PString & fn);

    // Incoming channel functions
    virtual PBoolean WriteFrame(const void * buf, PINDEX len) = 0;
    virtual PBoolean IsSilenceFrame(const void * buf, PINDEX len) const = 0;

    virtual PBoolean QueueRecordable(PVXMLRecordable * newItem);

    PBoolean StartRecording(const PFilePath & fn, unsigned finalSilence = 3000, unsigned maxDuration = 30000);
    PBoolean EndRecording();
    PBoolean IsRecording() const { return recording; }

    // Outgoing channel functions
    virtual PBoolean ReadFrame(void * buffer, PINDEX amount) = 0;
    virtual PINDEX CreateSilenceFrame(void * buffer, PINDEX amount) = 0;
    virtual void GetBeepData(PBYTEArray &, unsigned) { }

    virtual PBoolean QueueResource(const PURL & url, PINDEX repeat= 1, PINDEX delay = 0);

    virtual PBoolean QueuePlayable(const PString & type, const PString & str, PINDEX repeat = 1, PINDEX delay = 0, PBoolean autoDelete = PFalse);
    virtual PBoolean QueuePlayable(PVXMLPlayable * newItem);
    virtual PBoolean QueueData(const PBYTEArray & data, PINDEX repeat = 1, PINDEX delay = 0);

    virtual PBoolean QueueFile(const PString & fn, PINDEX repeat = 1, PINDEX delay = 0, PBoolean autoDelete = PFalse)
    { return QueuePlayable("File", fn, repeat, delay, autoDelete); }

    virtual PBoolean QueueCommand(const PString & cmd, PINDEX repeat = 1, PINDEX delay = 0)
    { return QueuePlayable("Command", cmd, repeat, delay, PTrue); }

    virtual void FlushQueue();
    virtual PBoolean IsPlaying() const { return currentPlayItem != NULL || playQueue.GetSize() > 0; }

    void SetPause(PBoolean pause) { paused = pause; }

    void SetName(const PString & name) { channelName = name; }

    unsigned GetSampleFrequency() const
    { return sampleFrequency; }

	  void SetSilentCount(int v) { silentCount = v; }

  protected:
    PVXMLChannelInterface * vxmlInterface;

    unsigned sampleFrequency;
    PString mediaFormat;
    PString wavFilePrefix;

    PMutex channelWriteMutex;
    PMutex channelReadMutex;
    PBoolean closed;

    // Incoming audio variables
    PBoolean recording;
    PVXMLRecordable * recordable;
    unsigned finalSilence;
    unsigned silenceRun;

    // Outgoing audio variables
    PMutex queueMutex;
    PVXMLQueue playQueue;
    PVXMLPlayable * currentPlayItem;

    PBoolean paused;
    int silentCount;
    int totalData;
    PTimer delayTimer;

    // "channelname" (which is the name of the <record> tag) so
    // results can be saved in vxml session variable
    PString channelName;
};


//////////////////////////////////////////////////////////////////


#endif // P_VXML

#endif // PTLIB_VXML_H


// End of file ////////////////////////////////////////////////////////////////
