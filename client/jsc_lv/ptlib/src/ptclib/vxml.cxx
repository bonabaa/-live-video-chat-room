/*
 * vxml.cxx
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
 * $Revision: 23621 $
 * $Author: rjongbloed $
 * $Date: 2009-10-08 23:18:00 +0000 (Thu, 08 Oct 2009) $
 */

#ifdef __GNUC__
#pragma implementation "vxml.h"
#endif

#include <ptlib.h>

#define P_DISABLE_FACTORY_INSTANCES

#if P_VXML

#include <ptlib/pfactory.h>
#include <ptclib/vxml.h>
#include <ptclib/memfile.h>
#include <ptclib/random.h>
#include <ptclib/http.h>


class PVXMLChannelPCM : public PVXMLChannel
{
  PCLASSINFO(PVXMLChannelPCM, PVXMLChannel);

  public:
    PVXMLChannelPCM();

  protected:
    // overrides from PVXMLChannel
    virtual PBoolean WriteFrame(const void * buf, PINDEX len);
    virtual PBoolean ReadFrame(void * buffer, PINDEX amount);
    virtual PINDEX CreateSilenceFrame(void * buffer, PINDEX amount);
    virtual PBoolean IsSilenceFrame(const void * buf, PINDEX len) const;
    virtual void GetBeepData(PBYTEArray & data, unsigned ms);
};


class PVXMLChannelG7231 : public PVXMLChannel
{
  PCLASSINFO(PVXMLChannelG7231, PVXMLChannel);
  public:
    PVXMLChannelG7231();

    // overrides from PVXMLChannel
    virtual PBoolean WriteFrame(const void * buf, PINDEX len);
    virtual PBoolean ReadFrame(void * buffer, PINDEX amount);
    virtual PINDEX CreateSilenceFrame(void * buffer, PINDEX amount);
    virtual PBoolean IsSilenceFrame(const void * buf, PINDEX len) const;
};


class PVXMLChannelG729 : public PVXMLChannel
{
  PCLASSINFO(PVXMLChannelG729, PVXMLChannel);
  public:
    PVXMLChannelG729();

    // overrides from PVXMLChannel
    virtual PBoolean WriteFrame(const void * buf, PINDEX len);
    virtual PBoolean ReadFrame(void * buffer, PINDEX amount);
    virtual PINDEX CreateSilenceFrame(void * buffer, PINDEX amount);
    virtual PBoolean IsSilenceFrame(const void * buf, PINDEX len) const;
};


#define new PNEW


#define SMALL_BREAK_MSECS   1000
#define MEDIUM_BREAK_MSECS  2500
#define LARGE_BREAK_MSECS   5000

// LATER: Lookup what this value should be
#define DEFAULT_TIMEOUT     10000

//////////////////////////////////////////////////////////

static PString GetContentType(const PFilePath & fn)
{
  PString type = fn.GetType();

  if (type *= ".vxml")
    return "text/vxml";

  if (type *= ".wav")
    return "audio/x-wav";

  return PString::Empty();
}


///////////////////////////////////////////////////////////////

PBoolean PVXMLPlayable::Open(PVXMLChannel & chan, PINDEX _delay, PINDEX _repeat, PBoolean _autoDelete)
{ 
  chan.SetReadChannel(NULL); 
  delay = _delay; 
  repeat = _repeat; 
  autoDelete = _autoDelete; 
  return true; 
}

PBoolean PVXMLPlayable::Open(PVXMLChannel & chan, const PString & _arg, PINDEX _delay, PINDEX _repeat, PBoolean v)
{ 
  arg = _arg; 
  return Open(chan, _delay, _repeat, v); 
}

PBoolean PVXMLPlayable::ReadFrame(PVXMLChannel & channel, void * _buf, PINDEX origLen)
{
  BYTE * buf = (BYTE *)_buf;
  PINDEX len = origLen;

  while (len > 0) {
    PBoolean stat = channel.ReadFrame(buf, len);
    if (!stat) 
      return false;
    PINDEX readLen = channel.GetLastReadCount();
    if (readLen == 0)
      return true;
    len -= readLen;
    buf += readLen;
  }

  return true;
}

///////////////////////////////////////////////////////////////

void PVXMLPlayableStop::Play(PVXMLChannel & channel)
{
	channel.SetSilentCount(20);
}

PBoolean PVXMLPlayableStop::ReadFrame(PVXMLChannel & channel, void *, PINDEX)
{ 
  channel.Close(); 
  return FALSE; 
}

///////////////////////////////////////////////////////////////

PBoolean PVXMLPlayableFilename::Open(PVXMLChannel & chan, const PString & _fn, PINDEX _delay, PINDEX _repeat, PBoolean _autoDelete)
{ 
  fn = _fn; 
  arg = _fn;
  if (!PFile::Exists(chan.AdjustWavFilename(fn)))
    return false;

  return PVXMLPlayable::Open(chan, _delay, _repeat, _autoDelete); 
}

void PVXMLPlayableFilename::Play(PVXMLChannel & outgoingChannel)
{
  PChannel * chan = NULL;

  // check the file extension and open a .wav or a raw (.sw or .g723) file
  if ((fn.Right(4)).ToLower() == ".wav")
    chan = outgoingChannel.CreateWAVFile(fn);
  else {
    PFile * fileChan = new PFile(fn);
    if (fileChan->Open(PFile::ReadOnly))
      chan = fileChan;
    else {
      delete fileChan;
    }
  }

  if (chan == NULL)
    PTRACE(2, "VXML\tCannot open file \"" << fn << "\"");
  else {
    PTRACE(3, "VXML\tPlaying file \"" << fn << "\"");
    outgoingChannel.SetReadChannel(chan, true);
  }
}

void PVXMLPlayableFilename::OnStop() 
{
  if (autoDelete) {
    PTRACE(3, "VXML\tDeleting file \"" << fn << "\"");
    PFile::Remove(fn); 
  }
}

PBoolean PVXMLPlayableFilename::Rewind(PChannel * chan) 
{ 
  PFile * file = dynamic_cast<PFile *>(chan); 
  if (file == NULL) 
    return false;

  return file->SetPosition(0); 
}

PFactory<PVXMLPlayable>::Worker<PVXMLPlayableFilename> vxmlPlayableFilenameFactory("File");

///////////////////////////////////////////////////////////////

PBoolean PVXMLPlayableFilenameList::Open(PVXMLChannel & chan, const PStringArray & _list, PINDEX _delay, PINDEX _repeat, PBoolean _autoDelete)
{ 
  for (PINDEX i = 0; i < _list.GetSize(); ++i) {
    PString fn = chan.AdjustWavFilename(_list[i]);
    if (PFile::Exists(fn))
      filenames.AppendString(fn);
  }

  if (filenames.GetSize() == 0)
    return false;

  currentIndex = 0;

  return PVXMLPlayable::Open(chan, _delay, ((_repeat >= 0) ? _repeat : 1) * filenames.GetSize(), _autoDelete); 
}

void PVXMLPlayableFilenameList::OnRepeat(PVXMLChannel & outgoingChannel)
{
  PFilePath fn = filenames[currentIndex++ % filenames.GetSize()];

  PChannel * chan = NULL;

  // check the file extension and open a .wav or a raw (.sw or .g723) file
  if ((fn.Right(4)).ToLower() == ".wav")
    chan = outgoingChannel.CreateWAVFile(fn);
  else {
    PFile * fileChan = new PFile(fn);
    if (fileChan->Open(PFile::ReadOnly))
      chan = fileChan;
    else {
      delete fileChan;
    }
  }

  if (chan == NULL)
    PTRACE(2, "VXML\tCannot open file \"" << fn << "\"");
  else {
    PTRACE(3, "VXML\tPlaying file \"" << fn << "\"");
    outgoingChannel.SetReadChannel(chan, true);
  }
}

void PVXMLPlayableFilenameList::OnStop() 
{
  if (autoDelete)  {
    for (PINDEX i = 0; i < filenames.GetSize(); ++i) {
      PTRACE(3, "VXML\tDeleting file \"" << filenames[i] << "\"");
      PFile::Remove(filenames[i]); 
    }
  }
}

PFactory<PVXMLPlayable>::Worker<PVXMLPlayableFilenameList> vxmlPlayableFilenameListFactory("FileList");

///////////////////////////////////////////////////////////////

#if P_PIPECHAN

PVXMLPlayableCommand::PVXMLPlayableCommand()
{ 
  pipeCmd = NULL; 
}

void PVXMLPlayableCommand::Play(PVXMLChannel & outgoingChannel)
{
  arg.Replace("%s", PString(PString::Unsigned, sampleFrequency));
  arg.Replace("%f", format);

  // execute a command and send the output through the stream
  pipeCmd = new PPipeChannel;
  if (!pipeCmd->Open(arg, PPipeChannel::ReadOnly)) {
    PTRACE(2, "VXML\tCannot open command " << arg);
    delete pipeCmd;
    return;
  }

  if (pipeCmd == NULL)
    PTRACE(2, "VXML\tCannot open command \"" << arg << "\"");
  else {
    pipeCmd->Execute();
    PTRACE(3, "VXML\tPlaying command \"" << arg << "\"");
    outgoingChannel.SetReadChannel(pipeCmd, true);
  }
}

void PVXMLPlayableCommand::OnStop() 
{
  if (pipeCmd != NULL) {
    pipeCmd->WaitForTermination();
    delete pipeCmd;
  }
}

PFactory<PVXMLPlayable>::Worker<PVXMLPlayableCommand> vxmlPlayableCommandFactory("Command");

#endif

///////////////////////////////////////////////////////////////

PBoolean PVXMLPlayableData::Open(PVXMLChannel & chan, const PString & /*_fn*/, PINDEX _delay, PINDEX _repeat, PBoolean v)
{ 
  return PVXMLPlayable::Open(chan, _delay, _repeat, v); 
}

void PVXMLPlayableData::SetData(const PBYTEArray & _data)
{ 
  data = _data; 
}

void PVXMLPlayableData::Play(PVXMLChannel & outgoingChannel)
{
  PMemoryFile * chan = new PMemoryFile(data);
  PTRACE(3, "VXML\tPlaying " << data.GetSize() << " bytes");
  outgoingChannel.SetReadChannel(chan, true);
}

PBoolean PVXMLPlayableData::Rewind(PChannel * chan)
{ 
  PMemoryFile * memfile = dynamic_cast<PMemoryFile *>(chan); 
  if (memfile == NULL) 
    return false; 
  return memfile->SetPosition(0); 
}

PFactory<PVXMLPlayable>::Worker<PVXMLPlayableData> vxmlPlayableDataFactory("PCM Data");

///////////////////////////////////////////////////////////////

PBoolean PVXMLPlayableTone::Open(PVXMLChannel & chan, const PString & toneSpec, PINDEX _delay, PINDEX _repeat, PBoolean v)
{ 
  // populate the tone buffer
  PTones tones;

  if (!tones.Generate(toneSpec))
    return false;

  PINDEX len = tones.GetSize() * sizeof(short);
  memcpy(data.GetPointer(len), tones.GetPointer(), len);

  return PVXMLPlayable::Open(chan, _delay, _repeat, v); 
}

PFactory<PVXMLPlayable>::Worker<PVXMLPlayableTone> vxmlPlayableToneFactory("Tone");

///////////////////////////////////////////////////////////////

PBoolean PVXMLPlayableURL::Open(PVXMLChannel & chan, const PString & _url, PINDEX _delay, PINDEX _repeat, PBoolean autoDelete)
{ 
  url = arg = _url; 
  return PVXMLPlayable::Open(chan, _delay, _repeat, autoDelete); 
}

void PVXMLPlayableURL::Play(PVXMLChannel & outgoingChannel)
{
  // open the resource
  PHTTPClient * client = new PHTTPClient;
  client->SetPersistent(false);
  PMIMEInfo outMIME, replyMIME;
  int code = client->GetDocument(url, outMIME, replyMIME);
  if ((code != 200) || (replyMIME(PHTTP::TransferEncodingTag()) *= PHTTP::ChunkedTag()))
    delete client;
  else {
    outgoingChannel.SetReadChannel(client, true);
  }
}

PFactory<PVXMLPlayable>::Worker<PVXMLPlayableURL> vxmlPlayableURLFactory("URL");

///////////////////////////////////////////////////////////////

PBoolean PVXMLRecordableFilename::Open(const PString & _arg)
{ 
  fn = _arg;
  consecutiveSilence = 0;
  return true;
}

void PVXMLRecordableFilename::Record(PVXMLChannel & outgoingChannel)
{
  PChannel * chan = NULL;

  // check the file extension and open a .wav or a raw (.sw or .g723) file
  if ((fn.Right(4)).ToLower() == ".wav")
    chan = outgoingChannel.CreateWAVFile(fn, true);
  else {
    PFile * fileChan = new PFile(fn);
    if (fileChan->Open(PFile::WriteOnly))
      chan = fileChan;
    else {
      delete fileChan;
    }
  }

  if (chan == NULL)
    PTRACE(2, "VXML\tCannot open file \"" << fn << "\"");
  else {
    PTRACE(3, "VXML\tRecording to file \"" << fn << "\"");
    outgoingChannel.SetWriteChannel(chan, true);
  }

  recordStart  = PTime();
  silenceStart = PTime();
  consecutiveSilence = 0;
}

PBoolean PVXMLRecordableFilename::OnFrame(PBoolean isSilence)
{
  if (!isSilence) {
    silenceStart = PTime();
    consecutiveSilence = 0;
  } else {
    consecutiveSilence++;
    if ( ((consecutiveSilence % 20) == 0) &&
        (
          ((finalSilence > 0) && ((PTime() - silenceStart).GetMilliSeconds() >= finalSilence)) || 
          ((maxDuration  > 0) && ((PTime() - recordStart).GetMilliSeconds() >= maxDuration))
          )
       )
      return true;
  }

  return false;
}

///////////////////////////////////////////////////////////////

PVXMLCache::PVXMLCache(const PDirectory & _directory)
  : directory(_directory)
{
  if (!directory.Exists())
    directory.Create();
}

static PString MD5AsHex(const PString & str)
{
  PMessageDigest::Result digest;
  PMessageDigest5::Encode(str, digest);

  PString hexStr;
  const BYTE * data = digest.GetPointer();
  for (PINDEX i = 0; i < digest.GetSize(); ++i)
    hexStr.sprintf("%02x", (unsigned)data[i]);
  return hexStr;
}


PFilePath PVXMLCache::CreateFilename(const PString & prefix, const PString & key, const PString & fileType)
{
  PString md5   = MD5AsHex(key);

  return directory + ((prefix + "_") + md5 + fileType);
}

PBoolean PVXMLCache::Get(const PString & prefix,
                     const PString & key, 
                     const PString & fileType, 
                           PString & contentType, 
                         PFilePath & dataFn)
{
  PWaitAndSignal m(*this);

  dataFn = CreateFilename(prefix, key, "." + fileType);
  PFilePath typeFn = CreateFilename(prefix, key, "_type.txt");
  if (!PFile::Exists(dataFn) || !PFile::Exists(typeFn)) {
    PTRACE(4, "VXML\tKey \"" << key << "\" not found in cache");
    return false;
  }

  {
    PFile file(dataFn, PFile::ReadOnly);
    if (!file.IsOpen() || (file.GetLength() == 0)) {
      PTRACE(4, "VXML\tDeleting empty cache file for key " << key);
      PFile::Remove(dataFn, true);
      PFile::Remove(typeFn, true);
      return false;
    }
  }

  PTextFile typeFile(typeFn, PFile::ReadOnly);
  if (!typeFile.IsOpen()) {
    PTRACE(4, "VXML\tCannot find type for cached key " << key << " in cache");
    PFile::Remove(dataFn, true);
    PFile::Remove(typeFn, true);
    return false;
  }

  typeFile.ReadLine(contentType);
  contentType.Trim();
  if (contentType.IsEmpty())
    contentType = GetContentType(dataFn);

  return true;
}

void PVXMLCache::Put(const PString & prefix,
                     const PString & key, 
                     const PString & fileType, 
                     const PString & contentType,       
                   const PFilePath & fn, 
                         PFilePath & dataFn)
{
  PWaitAndSignal m(*this);

  // create the filename for the cache files
  dataFn = CreateFilename(prefix, key, "." + fileType);
  PFilePath typeFn = CreateFilename(prefix, key, "_type.txt");

  // write the content type file
  PTextFile typeFile(typeFn, PFile::WriteOnly);
  if (contentType.IsEmpty())
    typeFile.WriteLine(GetContentType(fn));
  else
    typeFile.WriteLine(contentType);

  // rename the file to the correct name
  PFile::Rename(fn, dataFn.GetFileName(), true);
}

PVXMLCache & PVXMLCache::GetResourceCache()
{
  static PVXMLCache cache(PDirectory() + "cache");
  return cache;
}


PFilePath PVXMLCache::GetRandomFilename(const PString & prefix, const PString & fileType)
{
  PFilePath fn;

  // create a random temporary filename
  PRandom r;
  for (;;) {
    fn = directory + psprintf("%s_%i.%s", (const char *)prefix, r.Generate() % 1000000, (const char *)fileType);
    if (!PFile::Exists(fn))
      break;
  }

  return fn;
}

//////////////////////////////////////////////////////////

PVXMLSession::PVXMLSession(PTextToSpeech * _tts, PBoolean autoDelete)
  : vxmlChannel((PVXMLChannel * &)readChannel)
{
  vxmlThread       = NULL;
  threadRunning    = false;
  textToSpeech     = NULL;
  loaded           = false;
  emptyAction      = false;
  recordDTMFTerm   = false;
  defaultDTMF      = 0;
  timeout          = DEFAULT_TIMEOUT;

  autoDeleteTextToSpeech = false;
  SetTextToSpeech(_tts, autoDelete);

  Initialise();
}

void PVXMLSession::Initialise()
{
  recording        = false;
  listening        = false;
  activeGrammar    = NULL;
  listening        = false;
  forceEnd         = false;
  currentForm      = NULL;
  currentField     = NULL;
  currentNode      = NULL;
  m_speakNodeData  = true;
}


PVXMLSession::~PVXMLSession()
{
  Close();

  if ((textToSpeech != NULL) && autoDeleteTextToSpeech)
    delete textToSpeech;
}

PTextToSpeech * PVXMLSession::SetTextToSpeech(PTextToSpeech * _tts, PBoolean autoDelete)
{
  PWaitAndSignal m(sessionMutex);

  if (autoDeleteTextToSpeech && (textToSpeech != NULL))
    delete textToSpeech;

  autoDeleteTextToSpeech = autoDelete;
  textToSpeech = _tts;
  return textToSpeech;
}

PTextToSpeech * PVXMLSession::SetTextToSpeech(const PString & ttsName)
{
  PWaitAndSignal m(sessionMutex);

  if (autoDeleteTextToSpeech && (textToSpeech != NULL))
    delete textToSpeech;

  autoDeleteTextToSpeech = true;
  textToSpeech = PFactory<PTextToSpeech>::CreateInstance(ttsName);
  return textToSpeech;
}

PBoolean PVXMLSession::Load(const PString & source)
{
  // Lets try and guess what was passed, if file exists then is file
  PFilePath file = source;
  if (PFile::Exists(file))
    return LoadFile(file);

  // see if looks like URL
  PINDEX pos = source.Find(':');
  if (pos != P_MAX_INDEX) {
    PString scheme = source.Left(pos);
    if ((scheme *= "http") || (scheme *= "https") || (scheme *= "file"))
      return LoadURL(source);
  }

  // See if is actual VXML
  if (PCaselessString(source).Find("<vxml") != P_MAX_INDEX)
    return LoadVXML(source);

  return false;
}


PBoolean PVXMLSession::LoadFile(const PFilePath & filename)
{
  PTRACE(4, "VXML\tLoading file: " << filename);

  PTextFile file(filename, PFile::ReadOnly);
  if (!file.IsOpen()) {
    PTRACE(1, "VXML\tCannot open " << filename);
    return false;
  }

  return LoadVXML(file.ReadString(P_MAX_INDEX));
}


PBoolean PVXMLSession::LoadURL(const PURL & url)
{
  PTRACE(4, "VXML\tLoading URL: " << url);

  // retreive the document (may be a HTTP get)
  PFilePath fn;
  PString contentType;
  if (!RetreiveResource(url, contentType, fn, false)) {
    PTRACE(1, "VXML\tCannot load document " << url);
    return false;
  }

  if (!LoadFile(fn)) {
    PTRACE(1, "VXML\tCannot load VXML from " << url);
    return false;
  }

  rootURL = url;
  return true;
}

PBoolean PVXMLSession::LoadVXML(const PString & xmlText)
{
  PWaitAndSignal m(sessionMutex);

  rootURL = PString::Empty();

  // parse the XML
  xmlFile.RemoveAll();
  if (!xmlFile.Load(xmlText)) {
    PTRACE(1, "VXML\tCannot parse root document: " << GetXMLError());
    return false;
  }

  PXMLElement * root = xmlFile.GetRootElement();
  if (root == NULL) {
    PTRACE(1, "VXML\tNo root element");
    return false;
  }

  // reset interpeter state
  Initialise();

  // find the first form
  if ((currentForm = FindForm(PString::Empty())) == NULL) {
    PTRACE(1, "VXML\tNo form element");
    return false;
  }

  // start processing with this <form> element
  currentNode = currentForm;

  loaded = true;

  Trigger();
  return true;
}

PURL PVXMLSession::NormaliseResourceName(const PString & src)
{
  // if resource name has a scheme, then use as is
  PINDEX pos = src.Find(':');
  if ((pos != P_MAX_INDEX) && (pos < 5))
    return src;

  if (rootURL.IsEmpty())
    return "file:" + src;

  // else use scheme and path from root document
  PURL url = rootURL;
  PStringArray path = url.GetPath();
  PString pathStr;
  if (path.GetSize() > 0) {
    pathStr += path[0];
    PINDEX i;
    for (i = 1; i < path.GetSize()-1; i++)
      pathStr += "/" + path[i];
    pathStr += "/" + src;
    url.SetPathStr(pathStr);
  }

  return url;
}


PBoolean PVXMLSession::RetreiveResource(const PURL & url, 
                                       PString & contentType, 
                                     PFilePath & dataFn,
                                            PBoolean useCache)
{
  PBoolean stat = false;

  // files on the local file system get loaded locally
  if (url.GetScheme() *= "file") {
    dataFn = url.AsFilePath();
    if (contentType.IsEmpty())
      contentType = GetContentType(dataFn);
    stat = true;
  }

  // do a HTTP get when appropriate
  else if ((url.GetScheme() *= "http") || (url.GetScheme() *= "https")) {

    PFilePath fn;
    PString fileType = url.AsFilePath().GetType();

    PBoolean inCache = false;
    if (useCache)
      inCache = PVXMLCache::GetResourceCache().Get("url", url.AsString(), fileType, contentType, dataFn);

    if (!inCache) {

      // get a random filename
      fn = PVXMLCache::GetResourceCache().GetRandomFilename("url", fileType);

      // get the resource header information
      PHTTPClient client;
      PMIMEInfo outMIME, replyMIME;
      if (!client.GetDocument(url, outMIME, replyMIME)) {
        PTRACE(2, "VXML\tCannot load resource " << url);
        stat =false;
      } 
      
      else {

        // Get the body of the response in a PBYTEArray (might be binary data)
        PBYTEArray incomingData;
        client.ReadContentBody(replyMIME, incomingData);
        contentType = replyMIME(PHTTPClient::ContentTypeTag());

        // write the data in the file
        PFile cacheFile(fn, PFile::WriteOnly);
        cacheFile.Write(incomingData.GetPointer(), incomingData.GetSize() );

        // if we have a cache and we are using it, then save the data
        if (useCache) 
          PVXMLCache::GetResourceCache().Put("url", url.AsString(), fileType, contentType, fn, dataFn);

        // data is loaded
        stat = true;
      }
    }
  }

  // files on the local file system get loaded locally
  else if (url.GetScheme() *= "file") {
    dataFn = url.AsFilePath();
    stat = true;
  }

  // unknown schemes give an error
  else 
    stat = false;

  return stat;
}


PXMLElement * PVXMLSession::FindForm(const PString & id)
{
  // NOTE: should have some flag to know if it is loaded
  PXMLElement * root = xmlFile.GetRootElement();
  if (root == NULL)
    return NULL;
  
  // Only handle search of top level nodes for <form> element
  PINDEX i;
  for (i = 0; i < root->GetSize(); i++) {
    PXMLObject * xmlObject = root->GetElement(i); 
    if (xmlObject->IsElement()) {
      PXMLElement * xmlElement = (PXMLElement*)xmlObject;
      if (
          (xmlElement->GetName() *= "form") && 
          (id.IsEmpty() || (xmlElement->GetAttribute("id") *= id))
         )
        return xmlElement;
    }
  }
  return NULL;
}

PBoolean PVXMLSession::Open(const PString & _mediaFormat)
{
  mediaFormat = _mediaFormat;

  PVXMLChannel * chan = PFactory<PVXMLChannel>::CreateInstance(mediaFormat);
  if (chan == NULL) {
    PTRACE(1, "VXML\tCannot create VXML channel with format " << mediaFormat);
    return false;
  }

  if (!chan->Open(this)) {
    delete chan;
    return false;
  }

  // set the underlying channel
  if (!PIndirectChannel::Open(chan, chan))
    return false;

  return Execute();
}


PBoolean PVXMLSession::Execute()
{
  PWaitAndSignal m(sessionMutex);

  if (vxmlThread == NULL) {
    threadRunning = true;
    vxmlThread = PThread::Create(PCREATE_NOTIFIER(VXMLExecute), "VXML");
  }

  return true;
}


PBoolean PVXMLSession::Close()
{
  PThread * thread = NULL;

  sessionMutex.Wait();
  if (PThread::Current() != vxmlThread) {
    thread = vxmlThread;
    vxmlThread = NULL;
  }
  sessionMutex.Signal();

  if (thread != NULL) {
    PTRACE(3, "VXML\tClosing session");

    // Stop condition for thread
    threadRunning = false;
    forceEnd      = true;
    waitForEvent.Signal();

    // Signal all syncpoints that could be waiting for things
    answerSync.Signal();

    PAssert(thread->WaitForTermination(10000), "VXML thread did not exit in time.");
    delete thread;
  }

  return PIndirectChannel::Close();
}


void PVXMLSession::VXMLExecute(PThread &, INT)
{
  PTRACE(4, "VXML\tExecution thread started");

  while (!forceEnd && threadRunning) {
      
      // process current node in the VXML script
      if (currentNode != NULL)            
        ExecuteDialog();

    // wait for something to happen
    if (currentNode == NULL || IsPlaying())
      waitForEvent.Wait();
  }

  // Make sure the script has been run to the end so
  // submit actions etc. can be performed
  // record and audio and other user interaction commands should be skipped
  if (forceEnd) {
    PTRACE(2, "VXML\tFast forwarding through script because of forceEnd" );
    while (currentNode != NULL)
      ExecuteDialog();
  }

  OnEndSession();

  PTRACE(4, "VXML\tExecution thread ended");
}


void PVXMLSession::ProcessUserInput()
{
  // without this initialisation, gcc 4.1 gives a warning
  char ch = 0;
  {
    PWaitAndSignal m(userInputMutex);
    if (userInputQueue.size() == 0)
      return;
    ch = userInputQueue.front();
    userInputQueue.pop();
    PTRACE(3, "VXML\tHandling user input " << ch);
  }


  // recording
  if (recording) {
    if (recordDTMFTerm)
      RecordEnd();
  } 

  // playback
  else {
    if (activeGrammar != NULL)
      activeGrammar->OnUserInput(ch);
  }
}

void PVXMLSession::ExecuteDialog()
{
  // check for user input
  ProcessUserInput();

  // process any active grammars
  ProcessGrammar();

  // process current node in the VXML script
  ProcessNode();

  // Wait for the buffer to complete before continuing to the next node
  if (currentNode != NULL) {

    // if the current node has children, then process the first child
    if (currentNode->IsElement() && (((PXMLElement *)currentNode)->GetElement(0) != NULL))
      currentNode = ((PXMLElement *)currentNode)->GetElement(0);

    // else process the next sibling
    else {
      // Keep moving up the parents until we find a next sibling
      while ((currentNode != NULL) && currentNode->GetNextObject() == NULL) {
        currentNode = currentNode->GetParent();
        // if we are on the backwards traversal through a <field> then wait
        // for a grammar recognition and throw events if necessary
        if (currentNode != NULL && currentNode->IsElement() && (((PXMLElement*)currentNode)->GetName() *= "field")) {
          listening = true;
          PlaySilence(timeout);
        }
      }

      if (currentNode != NULL)
        currentNode = currentNode->GetNextObject();
    }
  }

  // Determine if we should quit
  if ((currentNode == NULL) && (activeGrammar == NULL) && !IsPlaying() && !IsRecording()) {
    PTRACE(3, "VXML\tEnd of VoiceXML elements, exiting.");
    threadRunning = false;
    waitForEvent.Signal();
  }
}


void PVXMLSession::ProcessGrammar()
{
  if (activeGrammar == NULL)
    return;

  PBoolean processGrammar(false);

  // Stop if we've matched a grammar or have a failed recognition
  if (activeGrammar->GetState() == PVXMLGrammar::FILLED || activeGrammar->GetState() == PVXMLGrammar::NOMATCH)
    processGrammar = true;

  // Stop the grammar if we've timed out
  else if (listening && !IsPlaying())   {
    activeGrammar->Stop();
    processGrammar = true;
  }

  // Let the loop run again if we're still waiting to time out and haven't resolved the grammar one way or the other
  if (!processGrammar && listening)
    return;

  if (processGrammar)
  {
    PVXMLGrammar::GrammarState state = activeGrammar->GetState();
    grammarResult = activeGrammar->GetValue();

    PXMLElement * field = activeGrammar->GetField();
    if (field != NULL && field->HasAttribute("name"))
      SetVar(field->GetAttribute("name"), grammarResult);

    LoadGrammar(NULL);
    listening = false;

    // Stop any playback
    if (IsOpen()) {
      vxmlChannel->FlushQueue();
      vxmlChannel->EndRecording();
    }

    // Check we're not in a menu
    if (eventName.IsEmpty()) {

      // Figure out what happened
      switch (state)
      {
      case PVXMLGrammar::FILLED:
        eventName = "filled";
        break;
      case PVXMLGrammar::NOINPUT:
        eventName = "noinput";
        break;
      case PVXMLGrammar::NOMATCH:
        eventName = "nomatch";
        break;
      default:
        ; //ERROR - unexpected grammar state
      }

      // Find the handler and move there
      PXMLElement * handler = FindHandler(eventName);
      if (handler != NULL)
        currentNode = handler;
    }
  }
}


void PVXMLSession::ProcessNode()
{
  if (currentNode == NULL)
    return;

  if (!currentNode->IsElement()) {
    if (m_speakNodeData) {
      if (forceEnd)
        currentNode = NULL;
      else
        TraverseAudio();
    }
    return;
  }

  m_speakNodeData = true;

  PXMLElement * element = (PXMLElement*)currentNode;
  PCaselessString nodeType = element->GetName();
  PTRACE(3, "VXML\t**** Processing VoiceXML element: <" << nodeType << "> ***");

  if (nodeType *= "audio") {
    if (!forceEnd)
      TraverseAudio();
  }

  else if (nodeType *= "block") {
    // check 'cond' attribute to see if this element's children are to be skipped
    // go on and process the children
  }

  else if (nodeType *= "break")
    TraverseAudio();

  else if (nodeType *= "disconnect")
    currentNode = NULL;

  else if (nodeType *= "field") {
    currentField = (PXMLElement*)currentNode;
    timeout = DEFAULT_TIMEOUT;
  }

  else if (nodeType *= "form") {
    // this is now the current element - go on
    currentForm = element;
    currentField = NULL;  // no active field in a new form
  }

  else if (nodeType *= "goto")
    TraverseGoto();

  else if (nodeType *= "grammar") {
    TraverseGrammar();  // this will set activeGrammar
    m_speakNodeData = false;
  }

  else if (nodeType *= "record") {
    if (!forceEnd)
      TraverseRecord();
  }

  else if (nodeType *= "prompt") {
    if (!forceEnd) {
      // LATER:
      // check 'cond' attribute to see if the children of this node should be processed
      // check 'count' attribute to see if this node should be processed
      // flush all prompts if 'bargein' attribute is set to false

      // Update timeout of current recognition (if 'timeout' attribute is set)
      if (element->HasAttribute("timeout")) {
        PTimeInterval timeout = StringToTime(element->GetAttribute("timeout"));
      }
    }
  }

  else if (nodeType *= "say-as") {
    if (!forceEnd) {
    }
  }

  else if (nodeType *= "value") {
    if (!forceEnd)
      TraverseAudio();
  }

  else if (nodeType *= "var")
    TraverseVar();

  else if (nodeType *= "if") 
    TraverseIf();

  else if (nodeType *= "exit") 
    TraverseExit();

  else if (nodeType *= "menu")  {
    if (!forceEnd) {
      TraverseMenu();
      eventName = "menu";
    }
  }

  else if (nodeType *= "choice") {
    if (!TraverseChoice())
      defaultDTMF++;
    else {
      // If the correct choice has been found, 
      /// make sure everything is reset correctly
      eventName.MakeEmpty();
      grammarResult.MakeEmpty();
      defaultDTMF = 1;
    }
  }

  else if (nodeType *= "submit")
    TraverseSubmit();

  else if (nodeType *= "property")
    TraverseProperty();

  else if (nodeType *= "transfer")
    TraverseTransfer();
}

PBoolean PVXMLSession::OnUserInput(const PString & str)
{
  {
    PWaitAndSignal m(userInputMutex);
    for (PINDEX i = 0; i < str.GetLength(); i++)
      userInputQueue.push(str[i]);
  }
  waitForEvent.Signal();
  return true;
}

PBoolean PVXMLSession::TraverseRecord()
{
  if (currentNode->IsElement()) {
    
    PString strName;
    PXMLElement * element = (PXMLElement *)currentNode;
    
    // Get the name (name)
    if (element->HasAttribute("name"))
      strName = element->GetAttribute("name");
    else if (element->HasAttribute("id"))
      strName = element->GetAttribute("id");
    
    // see if we need a beep
    if (element->GetAttribute("beep").ToLower() *= "true") {
      PBYTEArray beepData;
      GetBeepData(beepData, 1000);
      if (beepData.GetSize() != 0)
        PlayData(beepData);
    }

    // Get the destination filename (dest)
    PURL destURL;
    if (element->HasAttribute("dest")) 
      destURL = element->GetAttribute("dest");

    if (destURL.IsEmpty()) {
      PTime now;
      destURL = GetVar("session.telephone.dnis" ) + "_" + GetVar( "session.telephone.ani" ) + "_" + now.AsString( "yyyyMMdd_hhmmss") + ".wav";
    }

    // For some reason, if the file is there the create 
    // seems to fail. 
    PFilePath file = destURL.AsFilePath();
    PFile::Remove(file);
    
    // Get max record time (maxtime)
    PTimeInterval maxTime = PMaxTimeInterval;
    if (element->HasAttribute("maxtime")) 
      maxTime = StringToTime(element->GetAttribute("maxtime"));
    
    // Get terminating silence duration (finalsilence)
    PTimeInterval termTime(3000);
    if (element->HasAttribute("finalsilence")) 
      termTime = StringToTime(element->GetAttribute("finalsilence"));
    
    // Get dtmf term (dtmfterm)
    PBoolean dtmfTerm = true;
    if (element->HasAttribute("dtmfterm"))
      dtmfTerm = !(element->GetAttribute("dtmfterm").ToLower() *= "false");
    
    // create a semaphore, and then wait for the recording to terminate
    StartRecording(file, dtmfTerm, maxTime, termTime);
    
    if (!recordSync.Wait(maxTime)) {
      // The Wait() has timed out, to signal that the record timed out.
      // This is VXML version 2 property, but nice.
      // So it's possible to detect if the record timed out from within the 
      // VXML script
      SetVar(strName + "$.maxtime", "true");
    }
    else {
      // Normal hangup before timeout
      SetVar( strName + "$.maxtime", "false");
    }

    // when this returns, we are done
    EndRecording();
  }
  
  return true;
}

PString PVXMLSession::GetXMLError() const
{
  return psprintf("(%i:%i) ", xmlFile.GetErrorLine(), xmlFile.GetErrorColumn()) + xmlFile.GetErrorString();
}

PString PVXMLSession::EvaluateExpr(const PString & expr)
{
  // Should be full ECMAScript but ...
  // We only support expressions of the form 'literal'+variable or all digits

  PString result;

  PINDEX pos = 0;
  while (pos < expr.GetLength()) {
    if (expr[pos] == '\'') {
      PINDEX quote = expr.Find('\'', ++pos);
      PTRACE_IF(2, quote == P_MAX_INDEX, "VXML\tMismatched quote, ignoring transfer");
      result += expr(pos, quote-1);
      pos = quote+1;
    }
    else if (isalpha(expr[pos])) {
      PINDEX span = expr.FindSpan("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.$", pos);
      result += GetVar(expr(pos, span-1));
      pos = span;
    }
    else if (isdigit(expr[pos])) {
      PINDEX span = expr.FindSpan("0123456789", pos);
      result += GetVar(expr(pos, span-1));
      pos = span;
    }
    else if (expr[pos] == '+' || isspace(expr[pos]))
      pos++;
    else {
      PTRACE(2, "VXML\tOnly '+' operator supported.");
      break;
    }
  }

  return result;
}

PString PVXMLSession::GetVar(const PString & ostr) const
{
  PString str = ostr;
  PString scope;

  // get scope
  PINDEX pos = str.Find('.');
  if (pos != P_MAX_INDEX) {
    scope = str.Left(pos);
    str   = str.Mid(pos+1);
  }

  // process session scope
  if (scope.IsEmpty() || (scope *= "session")) {
    if (sessionVars.Contains(str))
      return sessionVars(str);
  }

  // assume any other scope is actually document or application
  return documentVars(str);
}

void PVXMLSession::SetVar(const PString & ostr, const PString & val)
{
  PString str = ostr;
  PString scope;

  // get scope
  PINDEX pos = str.Find('.');
  if (pos != P_MAX_INDEX) {
    scope = str.Left(pos);
    str   = str.Mid(pos+1);
  }

  // do session scope
  if (scope.IsEmpty() || (scope *= "session")) {
    sessionVars.SetAt(str, val);
    return;
  }

  PTRACE(3, "VXML\tDocument: " << str << " = \"" << val << "\"");

  // assume any other scope is actually document or application
  documentVars.SetAt(str, val);
}

PBoolean PVXMLSession::PlayFile(const PString & fn, PINDEX repeat, PINDEX delay, PBoolean autoDelete)
{
  return IsOpen() && vxmlChannel->QueueFile(fn, repeat, delay, autoDelete);
}

PBoolean PVXMLSession::PlayCommand(const PString & cmd, PINDEX repeat, PINDEX delay)
{
  return IsOpen() && vxmlChannel->QueueCommand(cmd, repeat, delay);
}

PBoolean PVXMLSession::PlayData(const PBYTEArray & data, PINDEX repeat, PINDEX delay)
{
  return IsOpen() && vxmlChannel->QueueData(data, repeat, delay);
}

PBoolean PVXMLSession::PlayTone(const PString & toneSpec, PINDEX repeat, PINDEX delay)
{
  return IsOpen() && vxmlChannel->QueuePlayable("Tone", toneSpec, repeat, delay, true);
}

void PVXMLSession::GetBeepData(PBYTEArray & data, unsigned ms)
{
  if (IsOpen())
    vxmlChannel->GetBeepData(data, ms);
}

PBoolean PVXMLSession::PlaySilence(const PTimeInterval & timeout)
{
  return PlaySilence((PINDEX)timeout.GetMilliSeconds());
}

PBoolean PVXMLSession::PlaySilence(PINDEX msecs)
{
  PBYTEArray nothing;
  return IsOpen() && vxmlChannel->QueueData(nothing, 1, msecs);
}

PBoolean PVXMLSession::PlayStop()
{
  return IsOpen() && vxmlChannel->QueuePlayable(new PVXMLPlayableStop());
}

PBoolean PVXMLSession::PlayResource(const PURL & url, PINDEX repeat, PINDEX delay)
{
  return IsOpen() && vxmlChannel->QueueResource(url, repeat, delay);
}

PBoolean PVXMLSession::LoadGrammar(PVXMLGrammar * grammar)
{
  if (activeGrammar != NULL) {
    delete activeGrammar;
    activeGrammar = false;
  }

  activeGrammar = grammar;

  return true;
}

PBoolean PVXMLSession::PlayText(const PString & textToPlay, 
                    PTextToSpeech::TextType type, 
                                     PINDEX repeat, 
                                     PINDEX delay)
{
  if (!IsOpen() || textToPlay.IsEmpty())
    return false;

  PTRACE(2, "VXML\tConverting \"" << textToPlay << "\" to speech");

  PStringArray list;
  PBoolean useCache = !(GetVar("caching") *= "safe");
  if (!ConvertTextToFilenameList(textToPlay, type, list, useCache) || (list.GetSize() == 0)) {
    PTRACE(1, "VXML\tCannot convert text to speech");
    return false;
  }

  PVXMLPlayableFilenameList * playable = new PVXMLPlayableFilenameList;
  if (!playable->Open(*vxmlChannel, list, delay, repeat, !useCache)) {
    delete playable;
    PTRACE(1, "VXML\tCannot create playable for filename list");
    return false;
  }

  if (!vxmlChannel->QueuePlayable(playable))
    return false;

  PTRACE(2, "VXML\tQueued filename list for playing");

  return true;
}

PBoolean PVXMLSession::ConvertTextToFilenameList(const PString & _text, PTextToSpeech::TextType type, PStringArray & filenameList, PBoolean useCache)
{
  PString prefix = psprintf("tts%i", type);

  PStringArray lines = _text.Trim().Lines();
  for (PINDEX i = 0; i < lines.GetSize(); i++) {

    PString text = lines[i].Trim();
    if (text.IsEmpty())
      continue;

    PBoolean spoken = false;
    PFilePath dataFn;

    // see if we have converted this text before
    PString contentType = "audio/x-wav";
    if (useCache)
      spoken = PVXMLCache::GetResourceCache().Get(prefix, contentType + "\n" + text, "wav", contentType, dataFn);

    // if not cached, then use the text to speech converter
    if (spoken) {
     PTRACE(3, "VXML\tUsing cached WAV file for " << _text);
    } else {
      PFilePath tmpfname;
      if (textToSpeech != NULL) {
        tmpfname = PVXMLCache::GetResourceCache().GetRandomFilename("tts", "wav");
        if (!textToSpeech->OpenFile(tmpfname)) {
          PTRACE(2, "VXML\tcannot open file " << tmpfname);
        } else {
          spoken = textToSpeech->Speak(text, type);
          PTRACE(3, "VXML\tCreated new WAV file for " << _text);
          if (!textToSpeech->Close()) {
            PTRACE(2, "VXML\tcannot close TTS engine");
          }
        }
        textToSpeech->Close();
        if (useCache)
          PVXMLCache::GetResourceCache().Put(prefix, text, "wav", contentType, tmpfname, dataFn);
        else
          dataFn = tmpfname;
      }
    }

    if (!spoken) {
      PTRACE(2, "VXML\tcannot speak text using TTS engine");
    } else 
      filenameList.AppendString(dataFn);
  }

  return filenameList.GetSize() > 0;
}

void PVXMLSession::SetPause(PBoolean pause)
{
  if (IsOpen())
    vxmlChannel->SetPause(pause);
}


PBoolean PVXMLSession::IsPlaying() const
{
  return IsOpen() && vxmlChannel->IsPlaying();
}

PBoolean PVXMLSession::StartRecording(const PFilePath & p_recordFn, 
                                               PBoolean p_recordDTMFTerm, 
                                  const PTimeInterval & p_recordMaxTime, 
                                  const PTimeInterval & p_recordFinalSilence)
{
  recording          = true;
  recordFn           = p_recordFn;
  recordDTMFTerm     = p_recordDTMFTerm;
  recordMaxTime      = p_recordMaxTime;
  recordFinalSilence = p_recordFinalSilence;

  return IsOpen() && vxmlChannel->StartRecording(recordFn,
                                                 (unsigned)recordFinalSilence.GetMilliSeconds(), 
                                                 (unsigned)recordMaxTime.GetMilliSeconds());
}

void PVXMLSession::RecordEnd()
{
  if (recording)
    recordSync.Signal();
}

PBoolean PVXMLSession::EndRecording()
{
  if (recording) {
    recording = false;
    if (IsOpen())
      return vxmlChannel->EndRecording();
  }

  return false;
}


PBoolean PVXMLSession::IsRecording() const
{
  return IsOpen() && vxmlChannel->IsRecording();
}

PWAVFile * PVXMLSession::CreateWAVFile(const PFilePath & fn, PFile::OpenMode mode, int opts, unsigned fmt)
{ 
  if (!fn.IsEmpty())
    return new PWAVFile(fn, mode, opts, fmt);

  return new PWAVFile(mode, opts, fmt); 
}

PBoolean PVXMLSession::TraverseAudio()
{
  if (!currentNode->IsElement()) {
    PlayText(((PXMLData *)currentNode)->GetString().Trim());
  }

  else {
    PXMLElement * element = (PXMLElement *)currentNode;

    if (element->GetName() *= "value") {
      PString className = element->GetAttribute("class");
      PString value = EvaluateExpr(element->GetAttribute("expr"));
      PString voice = element->GetAttribute("voice");
      if (voice.IsEmpty())
        GetVar("voice");
      SayAs(className, value, voice);
    }

    else if (element->GetName() *= "sayas") {
      PString className = element->GetAttribute("class");
      PXMLObject * object = element->GetElement();
      if (!object->IsElement()) {
        PString text = ((PXMLData *)object)->GetString();
        SayAs(className, text);
      }
    }

    else if (element->GetName() *= "break") {

      // msecs is VXML 1.0
      if (element->HasAttribute("msecs"))
        PlaySilence(element->GetAttribute("msecs").AsInteger());

      // time is VXML 2.0
      else if (element->HasAttribute("time")) {
        PTimeInterval time = StringToTime(element->GetAttribute("time"));
        PlaySilence(time);
      }
      
      else if (element->HasAttribute("size")) {
        PString size = element->GetAttribute("size");
        if (size *= "none")
          ;
        else if (size *= "small")
          PlaySilence(SMALL_BREAK_MSECS);
        else if (size *= "large")
          PlaySilence(LARGE_BREAK_MSECS);
        else 
          PlaySilence(MEDIUM_BREAK_MSECS);
      } 
      
      // default to medium pause
      else {
        PlaySilence(MEDIUM_BREAK_MSECS);
      }
    }

    else if (element->GetName() *= "audio") {
      PBoolean loaded = false;

      if (element->HasAttribute("src")) {

        PString str = element->GetAttribute("src").Trim();
        if (!str.IsEmpty() && (str[0] == '|')) {
          loaded = true;
          PlayCommand(str.Mid(1));
        } 
        
        else {
          // get a normalised name for the resource
          PFilePath fn; 
          PURL url = NormaliseResourceName(str);

          // load the resource from the cache
          PString contentType;
          PBoolean useCache = !(GetVar("caching") *= "safe") && !(element->GetAttribute("caching") *= "safe");
          if (RetreiveResource(url, contentType, fn, useCache)) {
            PWAVFile * wavFile = vxmlChannel->CreateWAVFile(fn);
            if (wavFile == NULL)
              PTRACE(2, "VXML\tCannot create audio file " + fn);
            else if (!wavFile->IsOpen())
              delete wavFile;
            else {
              loaded = true;
              PlayFile(fn, 0, 0, !useCache);   // make sure we delete the file if not cacheing
            }
          }
        }

        if (loaded) {
          // skip to the next node
          if (element->HasSubObjects())
            currentNode = element->GetElement(element->GetSize() - 1);
        }
      }
    }

    else 
      PTRACE(2, "VXML\tUnknown audio tag " << element->GetName() << " encountered");
  }

  return true;
}


PBoolean PVXMLSession::TraverseGoto()   // <goto>
{
  PAssert(currentNode != NULL, "ProcessGotoElement(): Expected valid node");
  if (currentNode == NULL)
    return false;
  
  // LATER: handle expr, expritem, fetchaudio, fetchhint, fetchtimeout, maxage, maxstale
  
  PAssert(currentNode->IsElement(), "ProcessGotoElement(): Expected element");
  
  // nextitem
  PString nextitem = ((PXMLElement*)currentNode)->GetAttribute("nextitem");
  if (!nextitem.IsEmpty()) {
    // LATER: Take out the optional #
    currentForm = FindForm(nextitem);
    currentNode = currentForm;
    if (currentForm == NULL) {
      // LATER: throw "error.semantic" or "error.badfetch" -- lookup which
      return false;
    }
    return true;
  }
  
  // next
  PString next = ((PXMLElement*)currentNode)->GetAttribute("next");
  // LATER: fixup filename to prepend path
  if (!next.IsEmpty()) { 
    if (next[0] == '#') {
      next = next.Right( next.GetLength() -1 );
      currentForm = FindForm(next);
      currentNode = currentForm;
      // LATER: throw "error.semantic" or "error.badfetch" -- lookup which
      return currentForm != NULL;
    }
    else {
      PURL url = NormaliseResourceName(next);
      return LoadURL(url) && (currentForm != NULL);
    }
  }
  return false;
}

PBoolean PVXMLSession::TraverseGrammar()   // <grammar>
{
  // LATER: A bunch of work to do here!

  // For now we only support the builtin digits type and do not parse any grammars.

  // NOTE: For now we will process both <grammar> and <field> here.
  // NOTE: Later there needs to be a check for <grammar> which will pull
  //       out the text and process a grammar like '1 | 2'

  // Right now we only support one active grammar.
  if (activeGrammar != NULL) {
    PTRACE(2, "VXML\tWarning: can only process one grammar at a time, ignoring previous grammar");
    LoadGrammar(NULL);
  }

  PXMLElement * element = (PXMLElement*)currentNode;
  PCaselessString attrib = element->GetAttribute("mode");
  if (!attrib.IsEmpty() && attrib != "dtmf") {
    PTRACE(2, "VXML\tOnly DTMF mode supported for grammar");
    return false;
  }

  attrib = element->GetAttribute("type");
  if (!attrib.IsEmpty() && attrib != "X-OPAL/digits") {
    PTRACE(2, "VXML\tOnly \"digits\" type supported for grammar");
    return false;
  }

  PTRACE(4, "VXML\tLoading new grammar");
  PStringToString tokens;
  PURL::SplitVars(element->GetData(), tokens, ';', '=');
  return LoadGrammar(new PVXMLDigitsGrammar(element->GetParent(),
                                            tokens("minDigits", "1").AsUnsigned(),
                                            tokens("maxDigits", "10").AsUnsigned(),
                                            tokens("terminators", "#")));
}

// Finds the proper event hander for 'noinput', 'filled', 'nomatch' and 'error'
// by searching the scope hiearchy from the current from
PXMLElement * PVXMLSession::FindHandler(const PString & event)
{
  PAssert(currentNode->IsElement(), "Expected 'PXMLElement' in PVXMLSession::FindHandler");
  PXMLElement * tmp = (PXMLElement *)currentNode;
  PXMLElement * handler = NULL;

  // Look in all the way up the tree for a handler either explicitly or in a catch
  while (tmp != NULL) {
    // Check for an explicit hander - i.e. <error>, <filled>, <noinput>, <nomatch>, <help>
    if ((handler = tmp->GetElement(event)) != NULL)
      return handler;

    // Check for a <catch>
    if ((handler = tmp->GetElement("catch")) != NULL) {
      PString strCond = handler->GetAttribute("cond");
      if (strCond.Find(event))
        return handler;
    }

    tmp = tmp->GetParent();
  }

  return NULL;
}

void PVXMLSession::SayAs(const PString & className, const PString & _text)
{
  SayAs(className, _text, GetVar("voice"));
}


void PVXMLSession::SayAs(const PString & className, const PString & _text, const PString & voice)
{
  if (textToSpeech != NULL)
    textToSpeech->SetVoice(voice);

  PString text = _text.Trim();
  if (!text.IsEmpty()) {
    PTextToSpeech::TextType type = PTextToSpeech::Literal;

    if (className *= "digits")
      type = PTextToSpeech::Digits;

    else if (className *= "literal")
      type = PTextToSpeech::Literal;

    else if (className *= "number")
      type = PTextToSpeech::Number;

    else if (className *= "currency")
      type = PTextToSpeech::Currency;

    else if (className *= "time")
      type = PTextToSpeech::Time;

    else if (className *= "date")
      type = PTextToSpeech::Date;

    else if (className *= "phone")
      type = PTextToSpeech::Phone;

    else if (className *= "ipaddress")
      type = PTextToSpeech::IPAddress;

    else if (className *= "duration")
      type = PTextToSpeech::Duration;

    PlayText(text, type);
  }
}

PTimeInterval PVXMLSession::StringToTime(const PString & str)
{
  PTimeInterval timeout;

  long msecs = str.AsInteger();
  if (str.Find("ms") != P_MAX_INDEX)
    ;
  else if (str.Find("s") != P_MAX_INDEX)
    msecs = msecs * 1000;

  return PTimeInterval(msecs);
}

PBoolean PVXMLSession::TraverseIf()
{
  // If 'cond' parameter evaluates to true, enter child entities, else
  // go to next element.

  PString condition = ((PXMLElement*)currentNode)->GetAttribute("cond");

  // Find comparison type
  PINDEX location = condition.Find("==");
  PBoolean isEqual = (location < condition.GetSize());

  if (isEqual) {
    // Find var name
    PString varname = condition.Left(location);

    // Find value, skip '=' signs
    PString cond_value = condition.Right(condition.GetSize() - (location + 3));
    
    // check if var value equals value from condition and if not skip child elements
    PString value = GetVar(varname);
    if (cond_value == value) {
      PTRACE( 3, "VXMLSess\t\tCondition matched \"" << condition << "\"" );
    } else {
      PTRACE( 3, "VXMLSess\t\tCondition \"" << condition << "\"did not match, " << varname << " == " << value );
      if (currentNode->IsElement()) {
        PXMLElement* element = (PXMLElement*) currentNode;
        if (element->HasSubObjects()) {
          // Step to last child element (really last element is NULL?)
          currentNode = element->GetElement(element->GetSize() - 1);
        }
      }
    }
  }

  else {
    PTRACE( 1, "\tPVXMLSession, <if> element contains condition with operator other than ==, not implemented" );
    return false;
  }

  return true;
}

PBoolean PVXMLSession::TraverseExit()
{
  currentNode = NULL;
  forceEnd    = true;
  waitForEvent.Signal();
  return true;
}


PBoolean PVXMLSession::TraverseSubmit()
{
  PBoolean result = false;

  // Do HTTP client stuff here

  // Find out what to submit, for now, only support a WAV file
  PXMLElement * element = (PXMLElement *)currentNode;

  if (!element->HasAttribute("namelist")){
    PTRACE(1, "VXMLSess\t<submit> does not contain \"namelist\" parameter");
    return false;
  }

  PString name = element->GetAttribute("namelist");

  if (name.Find(" ") < name.GetSize()) {
    PTRACE(1, "VXMLSess\t<submit> does not support more than one value in \"namelist\" parameter");
    return false;
  }

  if (!element->HasAttribute("next")) {
    PTRACE(1, "VXMLSess\t<submit> does not contain \"next\" parameter");
    return false;
  }

  PString url = element->GetAttribute("next");

  if (url.Find( "http://" ) > url.GetSize()) {
    PTRACE(1, "VXMLSess\t<submit> needs a full url as the \"next\" parameter");
    return false;
  }

  if (!(GetVar(name + ".type") == "audio/x-wav" )) {
    PTRACE(1, "VXMLSess\t<submit> does not (yet) support submissions of types other than \"audio/x-wav\"");
    return false;
  }

  PString fileName = GetVar(name + ".filename");

  if (!(element->HasAttribute("method"))) {
    PTRACE(1, "VXMLSess\t<submit> does not (yet) support default method type \"get\"");
    return false;
  }

  if ( !PFile::Exists(fileName )) {
    PTRACE(1, "VXMLSess\t<submit> cannot find file " << fileName);
    return false;
  }

  PString fileNameOnly;
  int pos = fileName.FindLast( "/" );
  if (pos < fileName.GetLength()) {
    fileNameOnly = fileName.Right( ( fileName.GetLength() - pos ) - 1 );
  }
  else {
    pos = fileName.FindLast("\\");
    if (pos < fileName.GetSize()) {
      fileNameOnly = fileName.Right((fileName.GetLength() - pos) - 1);
    }
    else {
      fileNameOnly = fileName;
    }
  }

  PHTTPClient client;
  PMIMEInfo sendMIME, replyMIME;

  if (element->GetAttribute("method") *= "post") {

    //                            1         2         3        4123
    PString boundary = "--------012345678901234567890123458VXML";

    sendMIME.SetAt( PHTTP::ContentTypeTag(), "multipart/form-data; boundary=" + boundary);
    sendMIME.SetAt( PHTTP::UserAgentTag(), "PVXML TraverseSubmit" );
    sendMIME.SetAt( "Accept", "text/html" );

    // After this all boundaries have a "--" prepended
    boundary = "--" + boundary;

    // Create the mime header
    // First set the primary boundary
    PString mimeHeader = boundary + "\r\n";

    // Add content disposition
    mimeHeader += "Content-Disposition: form-data; name=\"voicemail\"; filename=\"" + fileNameOnly + "\"\r\n";

    // Add content type
    mimeHeader += "Content-Type: audio/wav\r\n\r\n";

    // Create the footer and add the closing of the content with a CR/LF
    PString mimeFooter = "\r\n";

    // Copy the header, buffer and footer together in one PString

    // Load the WAV file into memory
    PFile file( fileName, PFile::ReadOnly );
    int size = file.GetLength();
    PString mimeThing;

    // Make PHP happy?
    // Anyway, this shows how to add more variables, for when namelist containes more elements
    PString mimeMaxFileSize = boundary + "\r\nContent-Disposition: form-data; name=\"MAX_FILE_SIZE\"\r\n\r\n3000000\r\n";

    // Finally close the body with the boundary again, but also add "--"
    // to show this is the final boundary
    boundary = boundary + "--";
    mimeFooter += boundary + "\r\n";
    mimeHeader = mimeMaxFileSize + mimeHeader;
    mimeThing.SetSize( mimeHeader.GetSize() + size + mimeFooter.GetSize() );

    // Copy the header to the result
    memcpy( mimeThing.GetPointer(), mimeHeader.GetPointer(), mimeHeader.GetLength());

    // Copy the contents of the file to the mime result
    file.Read( mimeThing.GetPointer() + mimeHeader.GetLength(), size );

    // Copy the footer to the result
    memcpy( mimeThing.GetPointer() + mimeHeader.GetLength() + size, mimeFooter.GetPointer(), mimeFooter.GetLength());

    // Send the POST request to the server
    result = client.PostData( url, sendMIME, mimeThing, replyMIME );

    // TODO, Later:
    // Remove file?
    // Load reply from server as new VXML docuemnt ala <goto>
  }

  else {
    if (element->GetAttribute("method") != "get") {
      PTRACE(1, "VXMLSess\t<submit> does not (yet) support method type \"" << element->GetAttribute( "method" ) << "\"");
      return false;
    }

    PString getURL = url + "?" + name + "=" + GetVar( name );

    client.GetDocument( url, sendMIME, replyMIME );
    // TODO, Later:
    // Load reply from server as new VXML document ala <goto>
  }

  if (!result) {
    PTRACE( 1, "VXMLSess\t<submit> to server failed with "
        << client.GetLastResponseCode() << " "
        << client.GetLastResponseInfo() );
  }

  return result;
}

PBoolean PVXMLSession::TraverseProperty()
{
  PXMLElement* element = (PXMLElement *) currentNode;
  if (element->HasAttribute("name"))
    SetVar(element->GetAttribute("name"), element->GetAttribute("value"));

  return true;
}


PBoolean PVXMLSession::TraverseTransfer()
{
  PXMLElement* element = (PXMLElement *) currentNode;
  if (element->HasSubObjects()) {
    currentNode = element->GetElement(element->GetSize() - 1);
    ProcessNode();
    // If queued up something to play, wait for it finish before we do the transfer.
    while (IsPlaying() && !forceEnd)
      waitForEvent.Wait();
  }

  bool bridged = (element->GetAttribute("bridge") *= "true");

  if (element->HasAttribute("dest"))
    OnTransfer(element->GetAttribute("dest"), bridged);
  else if (element->HasAttribute("destexpr"))
    OnTransfer(EvaluateExpr(element->GetAttribute("destexpr")), bridged);

  return true;
}


PBoolean PVXMLSession::TraverseMenu()
{
  PBoolean result = false;
  PVXMLGrammar * newGrammar = new PVXMLDigitsGrammar((PXMLElement*) currentNode, 1, 1, "" );
  LoadGrammar(newGrammar);
  result = true;
  return result;
}

PBoolean PVXMLSession::TraverseChoice()
{
  // Iterate over all choice elements starting at currentnode
  PBoolean result = false;

  PXMLElement* element = (PXMLElement *) currentNode;
  // Current node is a choice element

  PString dtmf = element->GetAttribute( "dtmf" );

  if (dtmf.IsEmpty())
    dtmf = PString(PString::Unsigned, defaultDTMF);

  // Check if DTMF value for grammarResult matches the DTMF value for the choice
  if (dtmf == grammarResult) {

    // Find the form at next parameter
    PString formID = element->GetAttribute( "next" );

    PTRACE(3, "VXMLsess\tFound form id " << formID );

    if (!formID.IsEmpty()) {
      formID = formID.Right( formID.GetLength() - 1 );
      currentNode = FindForm( formID );
      if (currentNode != NULL)
        result = true;
    }
  }
  return result;
}

PBoolean PVXMLSession::TraverseVar()
{
  PBoolean result = false;

  PXMLElement* element = (PXMLElement *) currentNode;

  PString name = element->GetAttribute( "name" );
  PString expr = element->GetAttribute( "expr" );

  if (name.IsEmpty() || expr.IsEmpty()) {
    PTRACE( 1, "VXMLSess\t<var> has a problem with its parameters, name=\"" << name << "\", expr=\"" << expr << "\"" );
  }
  else {
    SetVar(name, EvaluateExpr(expr));
    result = true;
  }

  return result;
}


void PVXMLSession::OnEndRecording(const PString & /*channelName*/)
{
  //SetVar(channelName + ".size", PString(incomingChannel->GetWAVFile()->GetDataLength() ) );
  //SetVar(channelName + ".type", "audio/x-wav" );
  //SetVar(channelName + ".filename", incomingChannel->GetWAVFile()->GetName() );
}


void PVXMLSession::Trigger()
{
  waitForEvent.Signal();
}



/////////////////////////////////////////////////////////////////////////////////////////

PVXMLGrammar::PVXMLGrammar(PXMLElement * _field)
  : field(_field), state(PVXMLGrammar::NOINPUT)
{
}

//////////////////////////////////////////////////////////////////

PVXMLMenuGrammar::PVXMLMenuGrammar(PXMLElement * _field)
  : PVXMLGrammar(_field)
{
}

//////////////////////////////////////////////////////////////////

PVXMLDigitsGrammar::PVXMLDigitsGrammar(PXMLElement * _field, PINDEX _minDigits, PINDEX _maxDigits, PString _terminators)
  : PVXMLGrammar(_field),
  minDigits(_minDigits),
  maxDigits(_maxDigits),
  terminators(_terminators)
{
  PAssert(_minDigits <= _maxDigits, "Error - invalid grammar parameter");
}

PBoolean PVXMLDigitsGrammar::OnUserInput(const char ch)
{
  // Ignore any other keys if we've already filled the grammar
  if (state == PVXMLGrammar::FILLED || state == PVXMLGrammar::NOMATCH)
    return true;

  // is this char the terminator?
  if (terminators.Find(ch) != P_MAX_INDEX) {
    state = (value.GetLength() >= minDigits && value.GetLength() <= maxDigits) ? 
      PVXMLGrammar::FILLED : 
      PVXMLGrammar::NOMATCH;
    return true;
  }

  // Otherwise add to the grammar and check to see if we're done
  value += ch;
  if (value.GetLength() == maxDigits) {
    state = PVXMLGrammar::FILLED;   // the grammar is filled!
    return true;
  }

  return false;
}


void PVXMLDigitsGrammar::Stop()
{
  // Stopping recognition here may change the state if something was
  // recognized but it didn't fill the number of digits requested
  if (!value.IsEmpty())
    state = PVXMLGrammar::NOMATCH;
  // otherwise the state will stay as NOINPUT
}

//////////////////////////////////////////////////////////////////

PVXMLChannel::PVXMLChannel(unsigned _frameDelay, PINDEX frameSize)
  : PDelayChannel(DelayReadsAndWrites, _frameDelay, frameSize)
{
  vxmlInterface = NULL; 

  sampleFrequency = 8000;
  closed          = false;

  recording       = false;
  recordable      = NULL;

  silentCount     = 20;         // wait 20 frames before playing the OGM
  paused          = false;

  currentPlayItem = NULL;
}

PBoolean PVXMLChannel::Open(PVXMLChannelInterface * _vxmlInterface)
{
  currentPlayItem = NULL;
  vxmlInterface = _vxmlInterface;
  PTRACE(4, "VXML\tOpening channel " << this);
  return true;
}

PVXMLChannel::~PVXMLChannel()
{
  Close();
}

PBoolean PVXMLChannel::IsOpen() const
{
  return !closed;
}

PBoolean PVXMLChannel::Close()
{ 
  if (!closed) {
    PTRACE(4, "VXML\tClosing channel " << this);

    EndRecording();
    FlushQueue();

    closed = true; 

    PDelayChannel::Close(); 
  }

  return true; 
}

PString PVXMLChannel::AdjustWavFilename(const PString & ofn)
{
  if (wavFilePrefix.IsEmpty())
    return ofn;

  PString fn = ofn;

  // add in suffix required for channel format, if any
  PINDEX pos = ofn.FindLast('.');
  if (pos == P_MAX_INDEX) {
    if (fn.Right(wavFilePrefix.GetLength()) != wavFilePrefix)
      fn += wavFilePrefix;
  }
  else {
    PString basename = ofn.Left(pos);
    PString ext      = ofn.Mid(pos+1);
    if (basename.Right(wavFilePrefix.GetLength()) != wavFilePrefix)
      basename += wavFilePrefix;
    fn = basename + "." + ext;
  }
  return fn;
}

PWAVFile * PVXMLChannel::CreateWAVFile(const PFilePath & fn, PBoolean recording)
{ 
  PWAVFile * wav = PWAVFile::format(mediaFormat);
  if (wav == NULL) {
    PTRACE(1, "VXML\tWAV file format " << mediaFormat << " not known");
    return NULL;
  }

  wav->SetAutoconvert();
  if (!wav->Open(AdjustWavFilename(fn), 
                 recording ? PFile::WriteOnly : PFile::ReadOnly,
                 PFile::ModeDefault))
    PTRACE(2, "VXML\tCould not open WAV file " << wav->GetName());

  else if (recording) {
    wav->SetChannels(1);
    wav->SetSampleRate(8000);
    wav->SetSampleSize(16);
    return wav;
  } 
  
  else if (!wav->IsValid())
    PTRACE(2, "VXML\tWAV file header invalid for " << wav->GetName());

  else if (wav->GetSampleRate() != sampleFrequency)
    PTRACE(2, "VXML\tWAV file has unsupported sample frequency " << wav->GetSampleRate());

  else if (wav->GetChannels() != 1)
    PTRACE(2, "VXML\tWAV file has unsupported channel count " << wav->GetChannels());

  else {
    wav->SetAutoconvert();   /// enable autoconvert
    PTRACE(3, "VXML\tOpened WAV file " << wav->GetName());
    return wav;
  }

  delete wav;
  return NULL;
}


PBoolean PVXMLChannel::Write(const void * buf, PINDEX len)
{
  if (closed)
    return false;

  channelWriteMutex.Wait();

  // let the recordable do silence detection
  if (recordable != NULL && recordable->OnFrame(IsSilenceFrame(buf, len))) {
    PTRACE(3, "VXML\tRecording finished due to silence");
    EndRecording();
  }

  // if nothing is capturing incoming data, then fake the timing and return
  if ((recordable == NULL) && (GetBaseWriteChannel() == NULL)) {
    lastWriteCount = len;
    channelWriteMutex.Signal();
    PDelayChannel::Wait(len, nextWriteTick);
    return true;
  }

  // write the data and do the correct delay
  if (!WriteFrame(buf, len)) 
    EndRecording();
  else
    totalData += lastWriteCount;

  channelWriteMutex.Signal();

  return true;
}

PBoolean PVXMLChannel::StartRecording(const PFilePath & fn, unsigned _finalSilence, unsigned _maxDuration)
{
  PVXMLRecordableFilename * recordable = new PVXMLRecordableFilename();
  if (!recordable->Open(fn)) {
    delete recordable;
    return false;
  }

  recordable->SetFinalSilence(_finalSilence);
  recordable->SetMaxDuration(_maxDuration);
  return QueueRecordable(recordable);
}

PBoolean PVXMLChannel::QueueRecordable(PVXMLRecordable * newItem)
{  
  totalData = 0;

  // shutdown any existing recording
  EndRecording();

  // insert the new recordable
  PWaitAndSignal mutex(channelWriteMutex);
  recordable = newItem;
  recording = true;
  totalData = 0;
  newItem->OnStart();
  newItem->Record(*this);
  SetReadTimeout(frameDelay);
  return true;
}


PBoolean PVXMLChannel::EndRecording()
{
  PWaitAndSignal mutex(channelWriteMutex);

  if (recordable != NULL) {
    PTRACE(3, "VXML\tFinished recording " << totalData << " bytes");

    PDelayChannel::Close();
    recordable->OnStop();
    delete recordable;
    recordable = NULL;
    PTRACE(4, "VXML\tRecording finished");
  }

  return true;
}

PBoolean PVXMLChannel::Read(void * buffer, PINDEX amount)
{
  // assume we are returning silence
  PBoolean done         = false;
  PBoolean silenceStuff = false;
  PBoolean delayDone    = false;

  while (!done && !silenceStuff) {

    if (closed)
      return false;

    {
      PWaitAndSignal m(channelReadMutex);

      // if we are paused or in a delay, then do return silence
      if (paused || delayTimer.IsRunning()) {
        silenceStuff = true;
        break;
      }

      // if we are returning silence frames, then decrement the frame count
      // and continue returning silence
      if (silentCount > 0) {
        silentCount--;
        silenceStuff = true;
        break;
      }

      // try and read data from the underlying channel
      else if (currentPlayItem != NULL) {

        PWaitAndSignal m(queueMutex);

        // if the read succeeds, we are done
        if (currentPlayItem->ReadFrame(*this, buffer, amount)) {
          totalData += amount;
          delayDone = true;
          done = true;
          break;
        } 

        // if a timeout, send silence
        if (GetErrorCode(LastReadError) == Timeout) {
          silenceStuff = true;
          break;
        }

        PTRACE(3, "VXML\tFinished playing " << totalData << " bytes");

        // if current item still active, check for trailing actions
        if (currentPlayItem != NULL) {
          if (currentPlayItem->GetRepeat() > 1) {
            if (currentPlayItem->Rewind(GetBaseReadChannel())) {
              currentPlayItem->SetRepeat(currentPlayItem->GetRepeat()-1);
              currentPlayItem->OnRepeat(*this);
              continue;
            }
            PTRACE(3, "VXML\tCannot rewind item - cancelling repeat");
          } 

          // see if end of queue delay specified
          PINDEX delay = 0;
          if (!currentPlayItem->delayDone) {
            delay = currentPlayItem->GetDelay();
            if (delay != 0) {
              PTRACE(3, "VXML\tDelaying for " << delay);
              delayTimer = delay;
              currentPlayItem->delayDone = true;
              continue;
            }
          }

          // stop the current item
          currentPlayItem->OnStop();
          delete currentPlayItem;
          currentPlayItem = NULL;
        }

        PDelayChannel::Close();
      }

      // check the queue for the next action
      {
        PWaitAndSignal m(queueMutex);

        // if nothing in the queue (which is weird as something just stopped playing)
        // then trigger the VXML and send silence
        currentPlayItem = playQueue.Dequeue();
        if (currentPlayItem == NULL) {
          vxmlInterface->Trigger();
          silenceStuff = true;
          break;
        }

        // start the new item
        currentPlayItem->OnStart();
        currentPlayItem->Play(*this);
        SetReadTimeout(frameDelay);
        totalData = 0;
      }
    }
  }
  
  // start silence frame if required
  // note that this always requires a delay
  if (silenceStuff) {
    lastReadCount = CreateSilenceFrame(buffer, amount);
  }

  // make sure we always do the correct delay
  if (!delayDone)
    Wait(amount, nextReadTick);

  return true;
}

PBoolean PVXMLChannel::QueuePlayable(const PString & type,
                                 const PString & arg, 
                                 PINDEX repeat, 
                                 PINDEX delay, 
                                 PBoolean autoDelete)
{
  PTRACE(3, "VXML\tEnqueueing playable " << type << " with arg " << arg << " for playing");
  PVXMLPlayable * item = PFactory<PVXMLPlayable>::CreateInstance(type);
  if (item == NULL) {
    PTRACE(2, "VXML\tCannot find playable of type " << type);
    delete item;
    return false;
  }

  if (!item->Open(*this, arg, delay, repeat, autoDelete)) {
    PTRACE(2, "VXML\tCannot open playable of type " << type << " with arg " << arg);
    delete item;
    return false;
  }

  if (QueuePlayable(item))
    return true;

  delete item;
  return false;
}

PBoolean PVXMLChannel::QueuePlayable(PVXMLPlayable * newItem)
{
  newItem->SetSampleFrequency(sampleFrequency);
  PWaitAndSignal mutex(queueMutex);
  playQueue.Enqueue(newItem);
  return true;
}

PBoolean PVXMLChannel::QueueResource(const PURL & url, PINDEX repeat, PINDEX delay)
{
  if (url.GetScheme() *= "file")
    return QueuePlayable("File", url.AsFilePath(), repeat, delay, false);
  else
    return QueuePlayable("URL", url.AsString(), repeat, delay);
}

PBoolean PVXMLChannel::QueueData(const PBYTEArray & data, PINDEX repeat, PINDEX delay)
{
  PTRACE(3, "VXML\tEnqueueing " << data.GetSize() << " bytes for playing");
  PVXMLPlayableData * item = PFactory<PVXMLPlayable>::CreateInstanceAs<PVXMLPlayableData>("PCM Data");
  if (item == NULL) {
    PTRACE(2, "VXML\tCannot find playable of type 'PCM Data'");
    delete item;
    return false;
  }

  if (!item->Open(*this, "", delay, repeat, true)) {
    PTRACE(2, "VXML\tCannot open playable of type 'PCM Data'");
    delete item;
    return false;
  }

  item->SetData(data);

  if (QueuePlayable(item))
    return true;

  delete item;
  return false;
}

void PVXMLChannel::FlushQueue()
{
  PWaitAndSignal mutex(channelReadMutex);

  if (GetBaseReadChannel() != NULL)
    PDelayChannel::Close();

  PWaitAndSignal m(queueMutex);

  PVXMLPlayable * qItem;
  while ((qItem = playQueue.Dequeue()) != NULL) {
    qItem->OnStop();
    delete qItem;
  }

  if (currentPlayItem != NULL) {
    currentPlayItem->OnStop();
    delete currentPlayItem;
    currentPlayItem = NULL;
  }
}

///////////////////////////////////////////////////////////////

PFactory<PVXMLChannel>::Worker<PVXMLChannelPCM> pcmVXMLChannelFactory(VXML_PCM16);

PVXMLChannelPCM::PVXMLChannelPCM()
  : PVXMLChannel(30, 480)
{
  mediaFormat    = VXML_PCM16;
  wavFilePrefix  = PString::Empty();
}

PBoolean PVXMLChannelPCM::WriteFrame(const void * buf, PINDEX len)
{
  return PDelayChannel::Write(buf, len);
}

PBoolean PVXMLChannelPCM::ReadFrame(void * buffer, PINDEX amount)
{
  PINDEX len = 0;
  while (len < amount)  {
    if (!PDelayChannel::Read(len + (char *)buffer, amount-len))
      return false;
    len += GetLastReadCount();
  }

  return true;
}

PINDEX PVXMLChannelPCM::CreateSilenceFrame(void * buffer, PINDEX amount)
{
  memset(buffer, 0, amount);
  return amount;
}

PBoolean PVXMLChannelPCM::IsSilenceFrame(const void * buf, PINDEX len) const
{
  // Calculate the average signal level of this frame
  int sum = 0;

  const short * pcm = (const short *)buf;
  const short * end = pcm + len/2;
  while (pcm != end) {
    if (*pcm < 0)
      sum -= *pcm++;
    else
      sum += *pcm++;
  }

  // calc average
  unsigned level = sum / (len / 2);

  return level < 500; // arbitrary level
}

static short beepData[] = { 0, 18784, 30432, 30400, 18784, 0, -18784, -30432, -30400, -18784 };


void PVXMLChannelPCM::GetBeepData(PBYTEArray & data, unsigned ms)
{
  data.SetSize(0);
  while (data.GetSize() < (PINDEX)(ms * 16)) {
    PINDEX len = data.GetSize();
    data.SetSize(len + sizeof(beepData));
    memcpy(len + data.GetPointer(), beepData, sizeof(beepData));
  }
}

///////////////////////////////////////////////////////////////

PFactory<PVXMLChannel>::Worker<PVXMLChannelG7231> g7231VXMLChannelFactory(VXML_G7231);

PVXMLChannelG7231::PVXMLChannelG7231()
  : PVXMLChannel(30, 0)
{
  mediaFormat     = VXML_G7231;
  wavFilePrefix  = "_g7231";
}

static const PINDEX g7231Lens[] = { 24, 20, 4, 1 };

PBoolean PVXMLChannelG7231::WriteFrame(const void * buffer, PINDEX actualLen)
{
  PINDEX len = g7231Lens[(*(BYTE *)buffer)&3];
  if (len > actualLen)
    return false;

  return PDelayChannel::Write(buffer, len);
}

PBoolean PVXMLChannelG7231::ReadFrame(void * buffer, PINDEX /*amount*/)
{
  if (!PDelayChannel::Read(buffer, 1))
    return false;

  PINDEX len = g7231Lens[(*(BYTE *)buffer)&3];
  if (len != 1) {
    if (!PIndirectChannel::Read(1+(BYTE *)buffer, len-1))
      return false;
    lastReadCount++;
  } 

  return true;
}

PINDEX PVXMLChannelG7231::CreateSilenceFrame(void * buffer, PINDEX /* len */)
{


  ((BYTE *)buffer)[0] = 2;
  memset(((BYTE *)buffer)+1, 0, 3);
  return 4;
}

PBoolean PVXMLChannelG7231::IsSilenceFrame(const void * buf, PINDEX len) const
{
  if (len == 4)
    return true;
  if (buf == NULL)
    return false;
  return ((*(const BYTE *)buf)&3) == 2;
}

///////////////////////////////////////////////////////////////

PFactory<PVXMLChannel>::Worker<PVXMLChannelG729> g729VXMLChannelFactory(VXML_G729);

PVXMLChannelG729::PVXMLChannelG729()
  : PVXMLChannel(10, 0)
{
  mediaFormat    = VXML_G729;
  wavFilePrefix  = "_g729";
}

PBoolean PVXMLChannelG729::WriteFrame(const void * buf, PINDEX /*len*/)
{
  return PDelayChannel::Write(buf, 10);
}

PBoolean PVXMLChannelG729::ReadFrame(void * buffer, PINDEX /*amount*/)
{
  return PDelayChannel::Read(buffer, 10);
}

PINDEX PVXMLChannelG729::CreateSilenceFrame(void * buffer, PINDEX /* len */)
{
  memset(buffer, 0, 10);
  return 10;
}

PBoolean PVXMLChannelG729::IsSilenceFrame(const void * /*buf*/, PINDEX /*len*/) const
{
  return false;
}

//////////////////////////////////////////////////////////////////////////////////

class TextToSpeech_Sample : public PTextToSpeech
{
  public:
    TextToSpeech_Sample();
    PStringArray GetVoiceList();
    PBoolean SetVoice(const PString & voice);
    PBoolean SetRate(unsigned rate);
    unsigned GetRate();
    PBoolean SetVolume(unsigned volume);
    unsigned GetVolume();
    PBoolean OpenFile   (const PFilePath & fn);
    PBoolean OpenChannel(PChannel * chanel);
    PBoolean IsOpen()    { return opened; }
    PBoolean Close();
    PBoolean Speak(const PString & text, TextType hint = Default);
    PBoolean SpeakNumber(unsigned number);

    PBoolean SpeakFile(const PString & text);

  protected:
    //PTextToSpeech * defaultEngine;

    PMutex mutex;
    PBoolean opened;
    PBoolean usingFile;
    PString text;
    PFilePath path;
    unsigned volume, rate;
    PString voice;

    std::vector<PFilePath> filenames;
};

TextToSpeech_Sample::TextToSpeech_Sample()
{
  PWaitAndSignal m(mutex);
  usingFile = opened = false;
  rate = 8000;
  volume = 100;
}

PStringArray TextToSpeech_Sample::GetVoiceList()
{
  PStringArray r;
  return r;
}

PBoolean TextToSpeech_Sample::SetVoice(const PString & v)
{
  voice = v;
  return true;
}

PBoolean TextToSpeech_Sample::SetRate(unsigned v)
{
  rate = v;
  return true;
}

unsigned TextToSpeech_Sample::GetRate()
{
  return rate;
}

PBoolean TextToSpeech_Sample::SetVolume(unsigned v)
{
  volume = v;
  return true;
}

unsigned TextToSpeech_Sample::GetVolume()
{
  return volume;
}

PBoolean TextToSpeech_Sample::OpenFile(const PFilePath & fn)
{
  PWaitAndSignal m(mutex);

  Close();
  usingFile = true;
  path = fn;
  opened = true;

  PTRACE(3, "TTS\tWriting speech to " << fn);

  return true;
}

PBoolean TextToSpeech_Sample::OpenChannel(PChannel * /*chanel*/)
{
  PWaitAndSignal m(mutex);

  Close();
  usingFile = false;
  opened = false;

  return true;
}

PBoolean TextToSpeech_Sample::Close()
{
  PWaitAndSignal m(mutex);

  if (!opened)
    return true;

  PBoolean stat = true;

  if (usingFile) {
    PWAVFile outputFile("PCM-16", path, PFile::WriteOnly);
    if (!outputFile.IsOpen()) {
      PTRACE(1, "TTS\tCannot create output file " << path);
      stat = false;
    }
    else {
      std::vector<PFilePath>::const_iterator r;
      for (r = filenames.begin(); r != filenames.end(); ++r) {
        PFilePath f = *r;
        PWAVFile file;
        file.SetAutoconvert();
        if (!file.Open(f, PFile::ReadOnly)) {
          PTRACE(1, "TTS\tCannot open input file " << f);
          stat = false;
        } else {
          PTRACE(1, "TTS\tReading from " << f);
          BYTE buffer[1024];
          for (;;) {
            if (!file.Read(buffer, 1024))
              break;
            outputFile.Write(buffer, file.GetLastReadCount());
          }
        }
      }
    }
    filenames.erase(filenames.begin(), filenames.end());
  }

  opened = false;
  return stat;
}

PBoolean TextToSpeech_Sample::SpeakNumber(unsigned number)
{
  return Speak(PString(PString::Signed, number), Number);
}


PBoolean TextToSpeech_Sample::Speak(const PString & text, TextType hint)
{
  // break into lines
  PStringArray lines = text.Lines();
  PINDEX i;
  for (i = 0; i < lines.GetSize(); ++i) {

    PString line = lines[i].Trim();
    if (line.IsEmpty())
      continue;

    PTRACE(3, "TTS\tAsked to speak " << text << " with type " << hint);

    if (hint == DateAndTime) {
      PTRACE(3, "TTS\tSpeaking date and time");
      Speak(text, Date);
      Speak(text, Time);
      continue;
    }

    if (hint == Date) {
      PTime time(line);
      if (time.IsValid()) {
        PTRACE(4, "TTS\tSpeaking date " << time);
        SpeakFile(time.GetDayName(time.GetDayOfWeek(), PTime::FullName));
        SpeakNumber(time.GetDay());
        SpeakFile(time.GetMonthName(time.GetMonth(), PTime::FullName));
        SpeakNumber(time.GetYear());
      }
      continue;
    }

    if (hint == Time) {
      PTime time(line);
      if (time.IsValid()) {
        PTRACE(4, "TTS\tSpeaking time " << time);
        int hour = time.GetHour();
        if (hour < 13) {
          SpeakNumber(hour);
          SpeakNumber(time.GetMinute());
          SpeakFile(PTime::GetTimeAM());
        }
        else {
          SpeakNumber(hour-12);
          SpeakNumber(time.GetMinute());
          SpeakFile(PTime::GetTimePM());
        }
      }
      continue;
    }

    if (hint == Default) {
      PBoolean isTime = false;
      PBoolean isDate = false;

      for (i = 0; !isDate && i < 7; ++i) {
        isDate = isDate || (line.Find(PTime::GetDayName((PTime::Weekdays)i, PTime::FullName)) != P_MAX_INDEX);
        isDate = isDate || (line.Find(PTime::GetDayName((PTime::Weekdays)i, PTime::Abbreviated)) != P_MAX_INDEX);
        PTRACE(4, "TTS\t " << isDate << " - " << PTime::GetDayName((PTime::Weekdays)i, PTime::FullName) << "," << PTime::GetDayName((PTime::Weekdays)i, PTime::Abbreviated));
      }
      for (i = 1; !isDate && i <= 12; ++i) {
        isDate = isDate || (line.Find(PTime::GetMonthName((PTime::Months)i, PTime::FullName)) != P_MAX_INDEX);
        isDate = isDate || (line.Find(PTime::GetMonthName((PTime::Months)i, PTime::Abbreviated)) != P_MAX_INDEX);
        PTRACE(4, "TTS\t " << isDate << " - " << PTime::GetMonthName((PTime::Months)i, PTime::FullName) << "," << PTime::GetMonthName((PTime::Months)i, PTime::Abbreviated));
      }

      if (!isTime)
        isTime = line.Find(PTime::GetTimeSeparator()) != P_MAX_INDEX;
      if (!isDate)
        isDate = line.Find(PTime::GetDateSeparator()) != P_MAX_INDEX;

      if (isDate && isTime) {
        PTRACE(4, "TTS\tDefault changed to DateAndTime");
        Speak(line, DateAndTime);
        continue;
      }
      if (isDate) {
        PTRACE(4, "TTS\tDefault changed to Date");
        Speak(line, Date);
        continue;
      }
      else if (isTime) {
        PTRACE(4, "TTS\tDefault changed to Time");
        Speak(line, Time);
        continue;
      }
    }
      
    PStringArray tokens = line.Tokenise("\t ", false);
    for (PINDEX j = 0; j < tokens.GetSize(); ++j) {
      PString word = tokens[j].Trim();
      if (word.IsEmpty())
        continue;
      PTRACE(4, "TTS\tSpeaking word " << word << " as " << hint);
      switch (hint) {

        case Time:
        case Date:
        case DateAndTime:
          PAssertAlways("Logic error");
          break;

        default:
        case Default:
        case Literal:
          {
            PBoolean isDigits = true;
            PBoolean isIpAddress = true;

            PINDEX k;
            for (k = 0; k < word.GetLength(); ++k) {
              if (word[k] == '.')
                isDigits = false;
              else if (!isdigit(word[k]))
                isDigits = isIpAddress = false;
            }

            if (isIpAddress) {
              PTRACE(4, "TTS\tDefault changed to IPAddress");
              Speak(word, IPAddress);
            } else if (isDigits) {
              PTRACE(4, "TTS\tDefault changed to Number");
              Speak(word, Number);
            } else {
              PTRACE(4, "TTS\tDefault changed to Spell");
              Speak(word, Spell);
            }
          }
          break;

        case Spell:
          PTRACE(4, "TTS\tSpelling " << text);
          for (PINDEX i = 0; i < text.GetLength(); ++i) 
            SpeakFile(PString(text[i]));
          break;

        case Phone:
        case Digits:
          PTRACE(4, "TTS\tSpeaking digits " << text);
          for (PINDEX i = 0; i < text.GetLength(); ++i) {
            if (isdigit(text[i]))
              SpeakFile(PString(text[i]));
          }
          break;

        case Duration:
        case Currency:
        case Number:
          {
            int number = atoi(line);
            PTRACE(4, "TTS\tSpeaking number " << number);
            if (number < 0) {
              SpeakFile("negative");
              number = -number;
            } 
            else if (number == 0) {
              SpeakFile("0");
            } 
            else {
              if (number >= 1000000) {
                int millions = number / 1000000;
                number = number % 1000000;
                SpeakNumber(millions);
                SpeakFile("million");
              }
              if (number >= 1000) {
                int thousands = number / 1000;
                number = number % 1000;
                SpeakNumber(thousands);
                SpeakFile("thousand");
              }
              if (number >= 100) {
                int hundreds = number / 100;
                number = number % 100;
                SpeakNumber(hundreds);
                SpeakFile("hundred");
              }
              if (!SpeakFile(PString(PString::Signed, number))) {
                int tens = number / 10;
                number = number % 10;
                if (tens > 0)
                  SpeakFile(PString(PString::Signed, tens*10));
                if (number > 0)
                  SpeakFile(PString(PString::Signed, number));
              }
            }
          }
          break;

        case IPAddress:
          {
            PIPSocket::Address addr(line);
            PTRACE(4, "TTS\tSpeaking IP address " << addr);
            for (PINDEX i = 0; i < 4; ++i) {
              int octet = addr[i];
              if (octet < 100)
                SpeakNumber(octet);
              else
                Speak(octet, Digits);
              if (i != 3)
                SpeakFile("dot");
            }
          }
          break;
      }
    }
  }

  return true;
}

PBoolean TextToSpeech_Sample::SpeakFile(const PString & text)
{
  PFilePath f = PDirectory(voice) + (text.ToLower() + ".wav");
  if (!PFile::Exists(f)) {
    PTRACE(2, "TTS\tUnable to find explicit file for " << text);
    return false;
  }
  filenames.push_back(f);
  return true;
}

PFactory<PTextToSpeech>::Worker<TextToSpeech_Sample> sampleTTSFactory("sampler", false);

#endif   // P_VXML


///////////////////////////////////////////////////////////////
