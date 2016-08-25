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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifdef __GNUC__
#pragma implementation "vxml.h"
#endif

#include <ptlib.h>

#define P_DISABLE_FACTORY_INSTANCES

#if P_VXML

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


static PVXMLNodeFactory::Worker<PVXMLNodeHandler> BlockNodeHandler("Block", true);

#define TRAVERSE_NODE(name) \
  class PVXMLTraverse##name : public PVXMLNodeHandler { \
    virtual bool Start(PVXMLSession & session, PXMLElement & element) const \
    { return session.Traverse##name(element); } \
  }; \
  static PVXMLNodeFactory::Worker<PVXMLTraverse##name> name##NodeHandler(#name, true)

TRAVERSE_NODE(Audio);
TRAVERSE_NODE(Break);
TRAVERSE_NODE(Value);
TRAVERSE_NODE(SayAs);
TRAVERSE_NODE(Goto);
TRAVERSE_NODE(Grammar);
TRAVERSE_NODE(If);
TRAVERSE_NODE(Exit);
TRAVERSE_NODE(Var);
TRAVERSE_NODE(Submit);
TRAVERSE_NODE(Choice);
TRAVERSE_NODE(Property);
TRAVERSE_NODE(Disconnect);
TRAVERSE_NODE(Prompt);

#define TRAVERSE_NODE2(name) \
  class PVXMLTraverse##name : public PVXMLNodeHandler { \
    virtual bool Start(PVXMLSession & session, PXMLElement & element) const \
    { return session.Traverse##name(element); } \
    virtual bool Finish(PVXMLSession & session, PXMLElement & element) const \
    { return session.Traversed##name(element); } \
  }; \
  static PVXMLNodeFactory::Worker<PVXMLTraverse##name> name##NodeHandler(#name, true)

TRAVERSE_NODE2(Menu);
TRAVERSE_NODE2(Form);
TRAVERSE_NODE2(Field);
TRAVERSE_NODE2(Transfer);
TRAVERSE_NODE2(Record);

class PVXMLTraverseEvent : public PVXMLNodeHandler
{
  virtual bool Start(PVXMLSession &, PXMLElement & element) const
  {
    return element.GetAttribute("fired") == "true";
  }

  virtual bool Finish(PVXMLSession &, PXMLElement & element) const
  {
    element.SetAttribute("fired", "false");
    return true;
  }
};
static PVXMLNodeFactory::Worker<PVXMLTraverseEvent> FilledNodeHandler("Filled", true);
static PVXMLNodeFactory::Worker<PVXMLTraverseEvent> NoInputNodeHandler("NoInput", true);
static PVXMLNodeFactory::Worker<PVXMLTraverseEvent> NoMatchNodeHandler("NoMatch", true);
static PVXMLNodeFactory::Worker<PVXMLTraverseEvent> ErrorNodeHandler("Error", true);
static PVXMLNodeFactory::Worker<PVXMLTraverseEvent> CatchNodeHandler("Catch", true);

#if PTRACING
class PVXMLTraverseLog : public PVXMLNodeHandler {
  virtual bool Start(PVXMLSession & session, PXMLElement & node) const
  {
    unsigned level = node.GetAttribute("level").AsUnsigned();
    if (level == 0)
      level = 3;
    PTRACE(level, "VXML-Log\t" + session.EvaluateExpr(node.GetAttribute("expr")));
    return true;
  }
};
static PVXMLNodeFactory::Worker<PVXMLTraverseLog> LogNodeHandler("Log", true);
#endif


#define new PNEW


#define SMALL_BREAK_MSECS   1000
#define MEDIUM_BREAK_MSECS  2500
#define LARGE_BREAK_MSECS   5000


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

PVXMLPlayable::PVXMLPlayable()
  : m_vxmlChannel(NULL)
  , m_subChannel(NULL)
  , m_repeat(1)
  , m_delay(0)
  , m_sampleFrequency(8000)
  , m_autoDelete(false)
  , m_delayDone(false)
{
}


PBoolean PVXMLPlayable::Open(PVXMLChannel & channel, const PString &, PINDEX delay, PINDEX repeat, PBoolean autoDelete)
{
  m_vxmlChannel = &channel;
  m_delay = delay;
  m_repeat = repeat;
  m_autoDelete = autoDelete;
  return true;
}


bool PVXMLPlayable::OnRepeat()
{
  if (PAssertNULL(m_vxmlChannel) == NULL)
    return false;

  if (m_repeat <= 1)
    return false;

  --m_repeat;
  return true;
}


bool PVXMLPlayable::OnDelay()
{
  if (m_delayDone)
    return false;

  m_delayDone = true;
  if (m_delay == 0)
    return false;

  if (PAssertNULL(m_vxmlChannel) == NULL)
    return false;

  m_vxmlChannel->SetSilence(m_delay);
  return true;
}


void PVXMLPlayable::OnStop()
{
  if (m_vxmlChannel == NULL || m_subChannel == NULL)
    return;

  if (m_vxmlChannel->GetReadChannel() == m_subChannel)
    m_vxmlChannel->SetReadChannel(NULL, false, true);

  delete m_subChannel;
}


///////////////////////////////////////////////////////////////

bool PVXMLPlayableStop::OnStart()
{
  if (m_vxmlChannel == NULL)
    return false;

  m_vxmlChannel->SetSilence(500);
  return false; // Return false so always stops
}


///////////////////////////////////////////////////////////////

PBoolean PVXMLPlayableFile::Open(PVXMLChannel & chan, const PString & fn, PINDEX delay, PINDEX repeat, PBoolean autoDelete)
{
  m_filePath = chan.AdjustWavFilename(fn);
  if (!PFile::Exists(m_filePath)) {
    PTRACE(2, "VXML\tPlayable file \"" << m_filePath << "\" not found.");
    return false;
  }

  return PVXMLPlayable::Open(chan, fn, delay, repeat, autoDelete);
}


bool PVXMLPlayableFile::OnStart()
{
  if (PAssertNULL(m_vxmlChannel) == NULL)
    return false;

  PFile * file = NULL;

  // check the file extension and open a .wav or a raw (.sw or .g723) file
  if (m_filePath.GetType() == ".wav") {
    file = m_vxmlChannel->CreateWAVFile(m_filePath);
    if (file == NULL) {
      PTRACE(2, "VXML\tCannot open WAV file \"" << m_filePath << '"');
      return false;
    }
  }
  else {
    // Assume file just has bytes of correct media format
    file = new PFile(m_filePath);
    if (!file->Open(PFile::ReadOnly)) {
      PTRACE(2, "VXML\tCould not open audio file \"" << m_filePath << '"');
      delete file;
      return false;
    }
  }

  PTRACE(3, "VXML\tPlaying file \"" << m_filePath << "\", " << file->GetLength() << " bytes");
  m_subChannel = file;
  return m_vxmlChannel->SetReadChannel(file, false);
}


bool PVXMLPlayableFile::OnRepeat()
{
  if (!PVXMLPlayable::OnRepeat())
    return false;

  PFile * file = dynamic_cast<PFile *>(m_subChannel);
  return PAssert(file != NULL, PLogicError) && PAssertOS(file->SetPosition(0));
}


void PVXMLPlayableFile::OnStop()
{
  PVXMLPlayable::OnStop();

  if (m_autoDelete && !m_filePath.IsEmpty()) {
    PTRACE(3, "VXML\tDeleting file \"" << m_filePath << "\"");
    PFile::Remove(m_filePath);
  }
}


PFactory<PVXMLPlayable>::Worker<PVXMLPlayableFile> vxmlPlayableFilenameFactory("File");


///////////////////////////////////////////////////////////////

PVXMLPlayableFileList::PVXMLPlayableFileList()
  : m_currentIndex(0)
{
}


PBoolean PVXMLPlayableFileList::Open(PVXMLChannel & chan, const PString & list, PINDEX delay, PINDEX repeat, PBoolean autoDelete)
{
  return Open(chan, list.Lines(), delay, repeat, autoDelete);
}


PBoolean PVXMLPlayableFileList::Open(PVXMLChannel & chan, const PStringArray & list, PINDEX delay, PINDEX repeat, PBoolean autoDelete)
{
  for (PINDEX i = 0; i < list.GetSize(); ++i) {
    PString fn = chan.AdjustWavFilename(list[i]);
    if (PFile::Exists(fn))
      m_fileNames.AppendString(fn);
    else {
      PTRACE(2, "VXML\tAudio file \"" << fn << "\" does not exist.");
    }
  }

  if (m_fileNames.GetSize() == 0) {
    PTRACE(2, "VXML\tNo files in list exist.");
    return false;
  }

  m_currentIndex = 0;

  return PVXMLPlayable::Open(chan, PString::Empty(), delay, ((repeat >= 0) ? repeat : 1) * m_fileNames.GetSize(), autoDelete);
}


bool PVXMLPlayableFileList::OnStart()
{
  if (!PAssert(!m_fileNames.IsEmpty(), PLogicError))
    return false;

  m_filePath = m_fileNames[m_currentIndex++ % m_fileNames.GetSize()];
  return PVXMLPlayableFile::OnStart();
}


bool PVXMLPlayableFileList::OnRepeat()
{
  return PVXMLPlayable::OnRepeat() && OnStart();

}


void PVXMLPlayableFileList::OnStop()
{
  m_filePath.MakeEmpty();

  PVXMLPlayableFile::OnStop();

  if (m_autoDelete)  {
    for (PINDEX i = 0; i < m_fileNames.GetSize(); ++i) {
      PTRACE(3, "VXML\tDeleting file \"" << m_fileNames[i] << "\"");
      PFile::Remove(m_fileNames[i]);
    }
  }
}

PFactory<PVXMLPlayable>::Worker<PVXMLPlayableFileList> vxmlPlayableFilenameListFactory("FileList");


///////////////////////////////////////////////////////////////

#if P_PIPECHAN

PBoolean PVXMLPlayableCommand::Open(PVXMLChannel & chan, const PString & cmd, PINDEX delay, PINDEX repeat, PBoolean autoDelete)
{
  if (cmd.IsEmpty()) {
    PTRACE(2, "VXML\tEmpty command line.");
    return false;
  }

  m_command = cmd;
  return PVXMLPlayable::Open(chan, cmd, delay, repeat, autoDelete);
}


bool PVXMLPlayableCommand::OnStart()
{
  if (PAssertNULL(m_vxmlChannel) == NULL)
    return false;

  PString cmd = m_command;
  cmd.Replace("%s", PString(PString::Unsigned, m_sampleFrequency));
  cmd.Replace("%f", m_format);

  // execute a command and send the output through the stream
  PPipeChannel * pipe = new PPipeChannel;
  if (!pipe->Open(cmd, PPipeChannel::ReadOnly)) {
    PTRACE(2, "VXML\tCannot open command \"" << cmd << '"');
    delete pipe;
    return false;
  }

  if (!pipe->Execute()) {
    PTRACE(2, "VXML\tCannot start command \"" << cmd << '"');
    return false;
  }

  PTRACE(3, "VXML\tPlaying command \"" << cmd << '"');
  m_subChannel = pipe;
  return m_vxmlChannel->SetReadChannel(pipe, false);
}


void PVXMLPlayableCommand::OnStop()
{
  PPipeChannel * pipe = dynamic_cast<PPipeChannel *>(m_subChannel);
  if (PAssert(pipe != NULL, PLogicError))
    pipe->WaitForTermination();

  PVXMLPlayable::OnStop();
}

PFactory<PVXMLPlayable>::Worker<PVXMLPlayableCommand> vxmlPlayableCommandFactory("Command");

#endif


///////////////////////////////////////////////////////////////

PBoolean PVXMLPlayableData::Open(PVXMLChannel & chan, const PString & hex, PINDEX delay, PINDEX repeat, PBoolean autoDelete)
{
  return PVXMLPlayable::Open(chan, hex, delay, repeat, autoDelete);
}


void PVXMLPlayableData::SetData(const PBYTEArray & data)
{
  m_data = data;
}


bool PVXMLPlayableData::OnStart()
{
  if (PAssertNULL(m_vxmlChannel) == NULL)
    return false;

  m_subChannel = new PMemoryFile(m_data);
  PTRACE(3, "VXML\tPlaying " << m_data.GetSize() << " bytes of memory");
  return m_vxmlChannel->SetReadChannel(m_subChannel, false);
}


bool PVXMLPlayableData::OnRepeat()
{
  if (!PVXMLPlayable::OnRepeat())
    return false;

  PMemoryFile * memfile = dynamic_cast<PMemoryFile *>(m_subChannel);
  return PAssert(memfile != NULL, PLogicError) && PAssertOS(memfile->SetPosition(0));
}

PFactory<PVXMLPlayable>::Worker<PVXMLPlayableData> vxmlPlayableDataFactory("PCM Data");


///////////////////////////////////////////////////////////////

PBoolean PVXMLPlayableTone::Open(PVXMLChannel & chan, const PString & toneSpec, PINDEX delay, PINDEX repeat, PBoolean autoDelete)
{
  // populate the tone buffer
  PTones tones;

  if (!tones.Generate(toneSpec)) {
    PTRACE(2, "VXML\tCOuld not generate tones with \"" << toneSpec << '"');
    return false;
  }

  PINDEX len = tones.GetSize() * sizeof(short);
  memcpy(m_data.GetPointer(len), tones.GetPointer(), len);

  return PVXMLPlayable::Open(chan, toneSpec, delay, repeat, autoDelete);
}

PFactory<PVXMLPlayable>::Worker<PVXMLPlayableTone> vxmlPlayableToneFactory("Tone");


///////////////////////////////////////////////////////////////

PBoolean PVXMLPlayableURL::Open(PVXMLChannel & chan, const PString & url, PINDEX delay, PINDEX repeat, PBoolean autoDelete)
{
  if (!m_url.Parse(url)) {
    PTRACE(2, "VXML\tInvalid URL \"" << url << '"');
    return false;
  }

  return PVXMLPlayable::Open(chan, url, delay, repeat, autoDelete);
}


bool PVXMLPlayableURL::OnStart()
{
  if (PAssertNULL(m_vxmlChannel) == NULL)
    return false;

  // open the resource
  PHTTPClient * client = new PHTTPClient;
  client->SetPersistent(false);
  PMIMEInfo outMIME, replyMIME;
  int code = client->GetDocument(m_url, outMIME, replyMIME);
  if ((code != 200) || (replyMIME(PHTTP::TransferEncodingTag()) *= PHTTP::ChunkedTag())) {
    delete client;
    return false;
  }

  m_subChannel = client;
  return m_vxmlChannel->SetReadChannel(client, false);
}

PFactory<PVXMLPlayable>::Worker<PVXMLPlayableURL> vxmlPlayableURLFactory("URL");


///////////////////////////////////////////////////////////////

PVXMLRecordable::PVXMLRecordable()
  : m_finalSilence(3000)
  , m_maxDuration(30000)
{
}


///////////////////////////////////////////////////////////////

PBoolean PVXMLRecordableFilename::Open(const PString & arg)
{
  m_fileName = arg;
  return true;
}


bool PVXMLRecordableFilename::OnStart(PVXMLChannel & outgoingChannel)
{
  PFile * file = NULL;

  // check the file extension and open a .wav or a raw (.sw or .g723) file
  if (m_fileName.GetType() == ".wav") {
    file = outgoingChannel.CreateWAVFile(m_fileName, true);
    if (file == NULL) {
      PTRACE(2, "VXML\tCannot open WAV file \"" << m_fileName << '"');
      return false;
    }
  }
  else {
    file = new PFile(m_fileName);
    if (!file->Open(PFile::WriteOnly)) {
      PTRACE(2, "VXML\tCannot open audio file \"" << m_fileName << '"');
      delete file;
      return false;
    }
  }

  PTRACE(3, "VXML\tRecording to file \"" << m_fileName << '"');
  outgoingChannel.SetWriteChannel(file, true);

  m_silenceTimer = m_finalSilence;
  m_recordTimer = m_maxDuration;
  return true;
}


PBoolean PVXMLRecordableFilename::OnFrame(PBoolean isSilence)
{
  if (isSilence) {
    if (m_silenceTimer.HasExpired()) {
      PTRACE(4, "VXML\tRecording silence detected.");
      return true;
    }
  }
  else
    m_silenceTimer = m_finalSilence;

  if (m_recordTimer.HasExpired()) {
    PTRACE(3, "VXML\tRecording finished due to max time exceeded.");
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

PVXMLSession::PVXMLSession(PTextToSpeech * tts, PBoolean autoDelete)
  : m_textToSpeech(tts)
  , m_autoDeleteTextToSpeech(autoDelete)
  , m_vxmlThread(NULL)
  , m_abortVXML(false)
  , m_currentNode(NULL)
  , m_xmlChanged(false)
  , m_speakNodeData(true)
  , m_grammar(NULL)
  , m_defaultMenuDTMF('N') /// Disabled
  , m_recordingStatus(NotRecording)
  , m_recordStopOnDTMF(false)
  , m_transferStatus(NotTransfering)
  , m_transferStartTime(0)
{
  SetVar("property.timeout" , "10s");
}


PVXMLSession::~PVXMLSession()
{
  Close();

  if (m_autoDeleteTextToSpeech)
    delete m_textToSpeech;
}


PTextToSpeech * PVXMLSession::SetTextToSpeech(PTextToSpeech * tts, PBoolean autoDelete)
{
  PWaitAndSignal mutex(m_sessionMutex);

  if (m_autoDeleteTextToSpeech)
    delete m_textToSpeech;

  m_autoDeleteTextToSpeech = autoDelete;
  return m_textToSpeech = tts;
}


PTextToSpeech * PVXMLSession::SetTextToSpeech(const PString & ttsName)
{
  PFactory<PTextToSpeech>::Key_T name = ttsName;
  if (ttsName.IsEmpty()) {
    PFactory<PTextToSpeech>::KeyList_T engines = PFactory<PTextToSpeech>::GetKeyList();
    if (engines.empty())
      return SetTextToSpeech(NULL, false);

#ifdef _WIN32
    name = "Microsoft SAPI";
    if (std::find(engines.begin(), engines.end(), name) == engines.end())
#endif
      name = engines[0];
  }

  return SetTextToSpeech(PFactory<PTextToSpeech>::CreateInstance(name), true);
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


PBoolean PVXMLSession::LoadFile(const PFilePath & filename, const PString & firstForm)
{
  PTRACE(4, "VXML\tLoading file: " << filename);

  PTextFile file(filename, PFile::ReadOnly);
  if (!file.IsOpen()) {
    PTRACE(1, "VXML\tCannot open " << filename);
    return false;
  }

  return LoadVXML(file.ReadString(P_MAX_INDEX), firstForm);
}


PBoolean PVXMLSession::LoadURL(const PURL & url)
{
  PTRACE(4, "VXML\tLoading URL " << url);

  // retreive the document (may be a HTTP get)

  PString xmlStr;
  if (url.LoadResource(xmlStr))
    return LoadVXML(xmlStr, url.GetFragment());

  PTRACE(1, "VXML\tCannot load document " << url);
  return false;
}


PBoolean PVXMLSession::LoadVXML(const PString & xmlText, const PString & firstForm)
{
  {
    PWaitAndSignal mutex(m_sessionMutex);

    m_xmlChanged = true;
    m_rootURL = PString::Empty();
    LoadGrammar(NULL);

    // parse the XML
    m_xml.RemoveAll();
    if (!m_xml.Load(xmlText)) {
      PTRACE(1, "VXML\tCannot parse root document: " << GetXMLError());
      return false;
    }

    PXMLElement * root = m_xml.GetRootElement();
    if (root == NULL) {
      PTRACE(1, "VXML\tNo root element");
      return false;
    }

    // find the first form
    if (!SetCurrentForm(firstForm, false)) {
      PTRACE(1, "VXML\tNo form element");
      m_xml.RemoveAll();
      return false;
    }

    m_variableScope = m_variableScope.IsEmpty() ? "application" : "document";
  }

  return Execute();
}


PURL PVXMLSession::NormaliseResourceName(const PString & src)
{
  // if resource name has a scheme, then use as is
  PINDEX pos = src.Find(':');
  if ((pos != P_MAX_INDEX) && (pos < 5))
    return src;

  if (m_rootURL.IsEmpty())
    return "file:" + src;

  // else use scheme and path from root document
  PURL url = m_rootURL;
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
  // files on the local file system get loaded locally
  if (url.GetScheme() == "file" && url.GetHostName().IsEmpty()) {
    dataFn = url.AsFilePath();
    if (contentType.IsEmpty())
      contentType = GetContentType(dataFn);
    return true;
  }

  PString fileType;
  {
    const PStringArray & path = url.GetPath();
    if (!path.IsEmpty())
      fileType = PFilePath(path[path.GetSize()-1]).GetType();
  }

  if (useCache && PVXMLCache::GetResourceCache().Get("url", url.AsString(), fileType, contentType, dataFn))
    return true;

  // get a random filename
  PFilePath newFn = PVXMLCache::GetResourceCache().GetRandomFilename("url", fileType);

  // get the resource header information
  PHTTPClient client;
  PMIMEInfo outMIME, replyMIME;
  if (!client.GetDocument(url, outMIME, replyMIME)) {
    PTRACE(2, "VXML\tCannot load resource " << url);
    return false;
  }

  // Get the body of the response in a PBYTEArray (might be binary data)
  PBYTEArray incomingData;
  client.ReadContentBody(replyMIME, incomingData);
  contentType = replyMIME(PHTTPClient::ContentTypeTag());

  // write the data in the file
  PFile cacheFile(newFn, PFile::WriteOnly);
  cacheFile.Write(incomingData.GetPointer(), incomingData.GetSize());

  // if we have a cache and we are using it, then save the data
  if (useCache)
    PVXMLCache::GetResourceCache().Put("url", url.AsString(), fileType, contentType, newFn, dataFn);

  // data is loaded
  return true;
}


bool PVXMLSession::SetCurrentForm(const PString & searchId, bool fullURI)
{
  PString id = searchId;

  if (fullURI) {
    if (searchId.IsEmpty()) {
      PTRACE(3, "VXML\tFull URI required for this form/menu search");
      return false;
    }

    if (searchId[0] != '#') {
      PTRACE(4, "VXML\tSearching form/menu \"" << searchId << '"');
      return LoadURL(NormaliseResourceName(searchId));
    }

    id = searchId.Mid(1);
  }

  // Only handle search of top level nodes for <form>/<menu> element
  // NOTE: should have some flag to know if it is loaded
  PXMLElement * root = m_xml.GetRootElement();
  if (root != NULL) {
    for (PINDEX i = 0; i < root->GetSize(); i++) {
      PXMLObject * xmlObject = root->GetElement(i);
      if (xmlObject->IsElement()) {
        PXMLElement * xmlElement = (PXMLElement*)xmlObject;
        if (
              (xmlElement->GetName() == "form" || xmlElement->GetName() == "menu") &&
              (id.IsEmpty() || (xmlElement->GetAttribute("id") *= id))
           ) {
          PTRACE(3, "VXML\tFound <" << xmlElement->GetName() << " id=\"" << xmlElement->GetAttribute("id") << "\">");

          if (m_currentNode != NULL) {
            PXMLElement * element = m_currentNode->GetParent();
            while (element != NULL) {
              PCaselessString nodeType = element->GetName();
              PVXMLNodeHandler * handler = PVXMLNodeFactory::CreateInstance(nodeType);
              if (handler != NULL) {
                handler->Finish(*this, *element);
                PTRACE(4, "VXML\tProcessed VoiceXML element: <" << nodeType << '>');
              }
              element = element->GetParent();
            }
          }

          m_currentNode = xmlObject;
          return true;
        }
      }
    }
  }

  PTRACE(3, "VXML\tNo form/menu with id \"" << searchId << '"');
  return false;
}


PVXMLChannel * PVXMLSession::GetAndLockVXMLChannel()
{
  m_sessionMutex.Wait();
  if (IsOpen())
    return GetVXMLChannel();

  m_sessionMutex.Signal();
  return NULL;
}


PBoolean PVXMLSession::Open(const PString & mediaFormat)
{
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
  PWaitAndSignal mutex(m_sessionMutex);

  if (IsLoaded()) {
    if (m_vxmlThread == NULL)
      m_vxmlThread = PThread::Create(PCREATE_NOTIFIER(VXMLExecute), "VXML");
    else
      Trigger();
  }

  return true;
}


PBoolean PVXMLSession::Close()
{
  PThread * thread = NULL;

  m_sessionMutex.Wait();

  LoadGrammar(NULL);

  if (PThread::Current() != m_vxmlThread) {
    thread = m_vxmlThread;
    m_vxmlThread = NULL;
  }

  m_sessionMutex.Signal();

  if (thread != NULL) {
    PTRACE(3, "VXML\tClosing session, fast forwarding through script");

    // Stop condition for thread
    m_abortVXML = true;
    Trigger();

    PAssert(thread->WaitForTermination(10000), "VXML thread did not exit in time.");
    delete thread;
  }

  return PIndirectChannel::Close();
}


void PVXMLSession::VXMLExecute(PThread &, INT)
{
  PTRACE(4, "VXML\tExecution thread started");

  m_sessionMutex.Wait();

  while (!m_abortVXML) {
    // process current node in the VXML script
    if (ProcessNode()) {
      /* wait for something to happen, usually output of some audio. But under
         some circumstances we want to abort the script, but we  have to make
         sure the script has been run to the end so submit actions etc. can be
         performed. Record and audio and other user interaction commands can
         be skipped, so we don't wait for them */
      do {
        while (ProcessEvents())
          ;
      } while (NextNode(true));
    }
    else {
      // Wait till node finishes
      while (ProcessEvents())
        ;

      NextNode(false);
    }

    // Determine if we should quit
    if (m_currentNode != NULL)
      continue;

    PTRACE(3, "VXML\tEnd of VoiceXML elements.");

    m_sessionMutex.Signal();
    OnEndDialog();
    m_sessionMutex.Wait();

    // Wait for anything OnEndDialog plays to complete.
    while (ProcessEvents())
      ;

    if (m_currentNode == NULL)
      m_abortVXML = true;
  }

  m_sessionMutex.Signal();

  OnEndSession();

  PTRACE(4, "VXML\tExecution thread ended");
}


void PVXMLSession::OnEndDialog()
{
}


void PVXMLSession::OnEndSession()
{
}


bool PVXMLSession::ProcessEvents()
{
  // m_sessionMutex already locked

  if (m_abortVXML)
    return false;

  char ch;

  m_userInputMutex.Wait();
  if (m_userInputQueue.empty())
    ch = '\0';
  else {
    ch = m_userInputQueue.front();
    m_userInputQueue.pop();
    PTRACE(3, "VXML\tHandling user input " << ch);
  }
  m_userInputMutex.Signal();

  if (ch != '\0') {
    if (m_recordStopOnDTMF)
      EndRecording();

    if (m_grammar != NULL)
      m_grammar->OnUserInput(ch);
  }

  if (IsOpen() && GetVXMLChannel()->IsPlaying()) {
    PTRACE(4, "VXML\tIs playing, awaiting event");
  }
  else if (IsOpen() && GetVXMLChannel()->IsRecording()) {
    PTRACE(4, "VXML\tIs recording, awaiting event");
  }
  else if (m_grammar != NULL && m_grammar->GetState() == PVXMLGrammar::Started) {
    PTRACE(4, "VXML\tAwaiting input, awaiting event");
  }
  else if (m_transferStatus == TransferInProgress) {
    PTRACE(4, "VXML\tTransfer in progress, awaiting event");
  }
  else {
    PTRACE(4, "VXML\tNothing happening, processing next node");
    return false;
  }

  m_sessionMutex.Signal();
  m_waitForEvent.Wait();
  m_sessionMutex.Wait();

  if (!m_xmlChanged)
    return true;

  PTRACE(4, "VXML\tXML changed, flushing queue");

  // Clear out any audio being output, so can start fresh on new VXML.
  if (IsOpen())
    GetVXMLChannel()->FlushQueue();

  return false;
}


bool PVXMLSession::NextNode(bool processChildren)
{
  // m_sessionMutex already locked

  if (m_abortVXML)
    return false;

  // No more nodes
  if (m_currentNode == NULL)
    return false;

  if (m_xmlChanged)
    return false;

  PXMLElement * element;

  if (m_currentNode->IsElement()) {
    element = (PXMLElement*)m_currentNode;

    // if the current node has children, then process the first child
    if (processChildren && (m_currentNode = element->GetElement(0)) != NULL)
      return false;
  }
  else {
    // Data node
    PXMLObject * sibling = m_currentNode->GetNextObject();
    if (sibling != NULL) {
      m_currentNode = sibling;
      return false;
    }
    if ((element = m_currentNode->GetParent()) == NULL) {
      m_currentNode = NULL;
      return false;
    }
  }

  // No children, move to sibling
  do {
    PCaselessString nodeType = element->GetName();
    PVXMLNodeHandler * handler = PVXMLNodeFactory::CreateInstance(nodeType);
    if (handler != NULL) {
      if (!handler->Finish(*this, *element)) {
        PTRACE(4, "VXML\tContinue processing VoiceXML element: <" << nodeType << '>');
        return true;
      }
      PTRACE(4, "VXML\tProcessed VoiceXML element: <" << nodeType << '>');
    }

    if ((m_currentNode = element->GetNextObject()) != NULL)
      break;

  } while ((element = element->GetParent()) != NULL);

  return false;
}


bool PVXMLSession::ProcessGrammar()
{
  if (m_grammar == NULL) {
    PTRACE(4, "VXML\tNo grammar was created!");
    return true;
  }

  switch (m_grammar->GetState()) {
    case PVXMLGrammar::Idle :
      m_grammar->Start();
      return false;

    case PVXMLGrammar::Started :
      return false;

    default :
      break;
  }

  bool nextNode = m_grammar->Process();
  LoadGrammar(NULL);
  return nextNode;
}


bool PVXMLSession::ProcessNode()
{
  // m_sessionMutex already locked

  if (m_abortVXML)
    return false;

  if (m_currentNode == NULL)
    return false;

  m_xmlChanged = false;

  if (!m_currentNode->IsElement()) {
    if (m_speakNodeData)
      PlayText(((PXMLData *)m_currentNode)->GetString().Trim());
    return true;
  }

  m_speakNodeData = true;

  PXMLElement * element = (PXMLElement*)m_currentNode;
  PCaselessString nodeType = element->GetName();
  PVXMLNodeHandler * handler = PVXMLNodeFactory::CreateInstance(nodeType);
  if (handler == NULL) {
    PTRACE(2, "VXML\tUnknown/unimplemented VoiceXML element: <" << nodeType << '>');
    return false;
  }

  PTRACE(3, "VXML\tProcessing VoiceXML element: <" << nodeType << '>');
  bool started = handler->Start(*this, *element);
  PTRACE_IF(4, !started, "VXML\tSkipping VoiceXML element: <" << nodeType << '>');
  return started;
}


void PVXMLSession::OnUserInput(const PString & str)
{
  {
    PWaitAndSignal mutex(m_userInputMutex);
    for (PINDEX i = 0; i < str.GetLength(); i++)
      m_userInputQueue.push(str[i]);
  }
  Trigger();
}


PBoolean PVXMLSession::TraverseRecord(PXMLElement &)
{
  m_recordingStatus = NotRecording;
  return true;
}


PBoolean PVXMLSession::TraversedRecord(PXMLElement & element)
{
  if (m_abortVXML)
    return true;

  switch (m_recordingStatus) {
    case RecordingInProgress :
      return false;

    case RecordingComplete :
      return GoToEventHandler(element, "filled");

    default :
      break;
  }

  // see if we need a beep
  if (element.GetAttribute("beep").ToLower() *= "true") {
    PBYTEArray beepData;
    GetBeepData(beepData, 1000);
    if (beepData.GetSize() != 0)
      PlayData(beepData);
  }

  // Get the destination filename (dest)
  PURL destURL;
  if (element.HasAttribute("dest"))
    destURL = element.GetAttribute("dest");

  if (destURL.IsEmpty())
    destURL.Parse("recording_" + PTime().AsString("yyyyMMdd_hhmmss") + ".wav", "file");

  // Get max record time (maxtime)
  PTimeInterval maxTime = PMaxTimeInterval;
  if (element.HasAttribute("maxtime"))
    maxTime = StringToTime(element.GetAttribute("maxtime"));

  // Get terminating silence duration (finalsilence)
  PTimeInterval termTime(3000);
  if (element.HasAttribute("finalsilence"))
    termTime = StringToTime(element.GetAttribute("finalsilence"));

  // Get dtmf term (dtmfterm)
  PBoolean dtmfTerm = true;
  if (element.HasAttribute("dtmfterm"))
    dtmfTerm = !(element.GetAttribute("dtmfterm").ToLower() *= "false");

  // create a semaphore, and then wait for the recording to terminate
  return !StartRecording(destURL.AsFilePath(), dtmfTerm, maxTime, termTime);
}


PString PVXMLSession::GetXMLError() const
{
  return psprintf("(%i:%i) ", m_xml.GetErrorLine(), m_xml.GetErrorColumn()) + m_xml.GetErrorString();
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


PCaselessString PVXMLSession::GetVar(const PString & varName) const
{
  PString fullVarName = varName;
  if (varName.Find('.') == P_MAX_INDEX)
    fullVarName = m_variableScope+'.'+varName;

  return m_variables(fullVarName);
}


void PVXMLSession::SetVar(const PString & varName, const PString & value)
{
  PString fullVarName = varName;
  if (varName.Find('.') == P_MAX_INDEX)
    fullVarName = m_variableScope+'.'+varName;

  m_variables.SetAt(fullVarName, value);
}


PBoolean PVXMLSession::PlayFile(const PString & fn, PINDEX repeat, PINDEX delay, PBoolean autoDelete)
{
  return IsOpen() && GetVXMLChannel()->QueueFile(fn, repeat, delay, autoDelete);
}


PBoolean PVXMLSession::PlayCommand(const PString & cmd, PINDEX repeat, PINDEX delay)
{
  return IsOpen() && GetVXMLChannel()->QueueCommand(cmd, repeat, delay);
}


PBoolean PVXMLSession::PlayData(const PBYTEArray & data, PINDEX repeat, PINDEX delay)
{
  return IsOpen() && GetVXMLChannel()->QueueData(data, repeat, delay);
}


PBoolean PVXMLSession::PlayTone(const PString & toneSpec, PINDEX repeat, PINDEX delay)
{
  return IsOpen() && GetVXMLChannel()->QueuePlayable("Tone", toneSpec, repeat, delay, true);
}


PBoolean PVXMLSession::PlayElement(PXMLElement & element)
{
  PString str = element.GetAttribute("src").Trim();
  if (str.IsEmpty()) {
    PTRACE(2, "VXML\tNo src attribute to play element.");
    return false;
  }

  if (str[0] == '|')
    return PlayCommand(str.Mid(1));

  // get a normalised name for the resource
  bool safe = GetVar("caching") == "safe" || (element.GetAttribute("caching") *= "safe");

  // load the resource from the cache
  PString contentType;
  PFilePath fn;
  if (RetreiveResource(NormaliseResourceName(str), contentType, fn, !safe))
    return PlayFile(fn, 0, 0, safe);   // make sure we delete the file if not cacheing

  return false;
}


void PVXMLSession::GetBeepData(PBYTEArray & data, unsigned ms)
{
  if (IsOpen())
    GetVXMLChannel()->GetBeepData(data, ms);
}


PBoolean PVXMLSession::PlaySilence(const PTimeInterval & timeout)
{
  return PlaySilence((PINDEX)timeout.GetMilliSeconds());
}


PBoolean PVXMLSession::PlaySilence(PINDEX msecs)
{
  PBYTEArray nothing;
  return IsOpen() && GetVXMLChannel()->QueueData(nothing, 1, msecs);
}


PBoolean PVXMLSession::PlayStop()
{
  return IsOpen() && GetVXMLChannel()->QueuePlayable(new PVXMLPlayableStop());
}


PBoolean PVXMLSession::PlayResource(const PURL & url, PINDEX repeat, PINDEX delay)
{
  return IsOpen() && GetVXMLChannel()->QueueResource(url, repeat, delay);
}


PBoolean PVXMLSession::LoadGrammar(PVXMLGrammar * grammar)
{
  PTRACE_IF(2, m_grammar != NULL && grammar == NULL, "VXML\tGrammar cleared from " << *m_grammar);

  delete m_grammar;
  m_grammar = grammar;

  PTRACE_IF(2, grammar != NULL, "VXML\tGrammar set to " << *grammar);
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
  bool useCache = GetVar("caching") != "safe";
  if (!ConvertTextToFilenameList(textToPlay, type, list, useCache) || (list.GetSize() == 0)) {
    PTRACE(1, "VXML\tCannot convert text to speech");
    return false;
  }

  PVXMLPlayableFileList * playable = new PVXMLPlayableFileList;
  if (!playable->Open(*GetVXMLChannel(), list, delay, repeat, !useCache)) {
    delete playable;
    PTRACE(1, "VXML\tCannot create playable for filename list");
    return false;
  }

  if (!GetVXMLChannel()->QueuePlayable(playable))
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
      spoken = PVXMLCache::GetResourceCache().Get(prefix, contentType + '\t' + text, "wav", contentType, dataFn);

    // if not cached, then use the text to speech converter
    if (spoken) {
     PTRACE(3, "VXML\tUsing cached WAV file for " << _text);
    } else {
      PFilePath tmpfname;
      if (m_textToSpeech != NULL) {
        tmpfname = PVXMLCache::GetResourceCache().GetRandomFilename("tts", "wav");
        if (m_textToSpeech->OpenFile(tmpfname)) {
          spoken = m_textToSpeech->Speak(text, type);
          PTRACE(3, "VXML\tCreated new WAV file for " << _text);
        }
        else {
          PTRACE(2, "VXML\tcannot open file " << tmpfname);
        }
        m_textToSpeech->Close();
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
    GetVXMLChannel()->SetPause(pause);
}


PBoolean PVXMLSession::StartRecording(const PFilePath & recordFn,
                                               PBoolean recordDTMFTerm,
                                  const PTimeInterval & recordMaxTime,
                                  const PTimeInterval & recordFinalSilence)
{
  if (!IsOpen())
    return false;

  if (recordFn.IsEmpty()) {
    PTRACE(1, "VXML\tNo destination file location");
    return true;
  }

  PFile::Remove(recordFn);

  m_recordStopOnDTMF = recordDTMFTerm;

  if (!GetVXMLChannel()->StartRecording(recordFn,
                                        (unsigned)recordFinalSilence.GetMilliSeconds(),
                                        (unsigned)recordMaxTime.GetMilliSeconds()))
    return false;

  m_recordingStatus = RecordingInProgress;
  return true;
}


PBoolean PVXMLSession::EndRecording()
{
  return m_recordingStatus == RecordingInProgress && IsOpen() && GetVXMLChannel()->EndRecording();
}


PBoolean PVXMLSession::TraverseAudio(PXMLElement & element)
{
  return !PlayElement(element);
}


PBoolean PVXMLSession::TraverseBreak(PXMLElement & element)
{
  // msecs is VXML 1.0
  if (element.HasAttribute("msecs"))
    return PlaySilence(element.GetAttribute("msecs").AsInteger());

  // time is VXML 2.0
  if (element.HasAttribute("time")) {
    PTimeInterval time = StringToTime(element.GetAttribute("time"));
    return PlaySilence(time);
  }

  if (element.HasAttribute("size")) {
    PString size = element.GetAttribute("size");
    if (size *= "none")
      return true;
    if (size *= "small")
      return PlaySilence(SMALL_BREAK_MSECS);
    if (size *= "large")
      return PlaySilence(LARGE_BREAK_MSECS);
    return PlaySilence(MEDIUM_BREAK_MSECS);
  }

  // default to medium pause
  return PlaySilence(MEDIUM_BREAK_MSECS);
}


PBoolean PVXMLSession::TraverseValue(PXMLElement & element)
{
  PString className = element.GetAttribute("class");
  PString value = EvaluateExpr(element.GetAttribute("expr"));
  PString voice = element.GetAttribute("voice");
  if (voice.IsEmpty())
    voice = GetVar("voice");
  SayAs(className, value, voice);
  return true;
}


PBoolean PVXMLSession::TraverseSayAs(PXMLElement & element)
{
  PString className = element.GetAttribute("class");
  PXMLObject * object = element.GetElement();
  if (!object->IsElement()) {
    PString text = ((PXMLData *)object)->GetString();
    SayAs(className, text);
  }
  return true;
}


PBoolean PVXMLSession::TraverseGoto(PXMLElement & element)
{
  bool fullURI = element.HasAttribute("next");
  if (SetCurrentForm(element.GetAttribute(fullURI ? "next" : "nextitem"), fullURI))
    return ProcessNode();

  // LATER: throw "error.semantic" or "error.badfetch" -- lookup which
  return false;
}


PBoolean PVXMLSession::TraverseGrammar(PXMLElement & element)
{
  // LATER: A bunch of work to do here!

  // For now we only support the builtin digits type and do not parse any grammars.

  // NOTE: For now we will process both <grammar> and <field> here.
  // NOTE: Later there needs to be a check for <grammar> which will pull
  //       out the text and process a grammar like '1 | 2'

  // Right now we only support one active grammar.
  if (m_grammar != NULL) {
    PTRACE(2, "VXML\tWarning: can only process one grammar at a time, ignoring previous grammar");
    LoadGrammar(NULL);
  }

  m_speakNodeData = false;

  PCaselessString attrib = element.GetAttribute("mode");
  if (!attrib.IsEmpty() && attrib != "dtmf") {
    PTRACE(2, "VXML\tOnly DTMF mode supported for grammar");
    return false;
  }

  attrib = element.GetAttribute("type");
  if (!attrib.IsEmpty() && attrib != "X-OPAL/digits") {
    PTRACE(2, "VXML\tOnly \"digits\" type supported for grammar");
    return false;
  }

  PTRACE(4, "VXML\tLoading new grammar");
  PStringToString tokens;
  PURL::SplitVars(element.GetData(), tokens, ';', '=');
  return LoadGrammar(new PVXMLDigitsGrammar(*this,
                                            *element.GetParent(),
                                            tokens("minDigits", "1").AsUnsigned(),
                                            tokens("maxDigits", "10").AsUnsigned(),
                                            tokens("terminators", "#")));
}


// Finds the proper event hander for 'noinput', 'filled', 'nomatch' and 'error'
// by searching the scope hiearchy from the current from
bool PVXMLSession::GoToEventHandler(PXMLElement & element, const PString & eventName)
{
  PXMLElement * level = &element;
  PXMLElement * handler = NULL;

  int actualCount = 1; // Need to increment this with state stored ... somewhere

  // Look in all the way up the tree for a handler either explicitly or in a catch
  for (;;) {
    for (int testCount = actualCount; testCount >= 0; --testCount) {
      // Check for an explicit hander - i.e. <error>, <filled>, <noinput>, <nomatch>, <help>
      PINDEX index = 0;
      if ((handler = level->GetElement(eventName)) != NULL &&
              handler->GetAttribute("count").AsInteger() == testCount)
        goto gotHandler;

      // Check for a <catch>
      index = 0;
      while ((handler = level->GetElement("catch", index++)) != NULL) {
        if ((handler->GetAttribute("event") *= eventName) &&
                handler->GetAttribute("count").AsInteger() == testCount)
          goto gotHandler;
      }
    }

    level = level->GetParent();
    if (level == NULL) {
      PTRACE(4, "VXML\tNo event handler found for \"" << eventName << '"');
      return true;
    }
  }

gotHandler:
  handler->SetAttribute("fired", "true");
  m_currentNode = handler;
  PTRACE(4, "VXML\tSetting event handler to node " << handler << " for \"" << eventName << '"');
  return false;
}


void PVXMLSession::SayAs(const PString & className, const PString & text)
{
  SayAs(className, text, GetVar("voice"));
}


void PVXMLSession::SayAs(const PString & className, const PString & textToSay, const PString & voice)
{
  if (m_textToSpeech != NULL)
    m_textToSpeech->SetVoice(voice);

  PString text = textToSay.Trim();
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
  PTimeInterval time(str.AsUnsigned64());

  if (str.Find("ms") == P_MAX_INDEX && str.Find('s') != P_MAX_INDEX)
    time *= 1000;

  return time;
}


PBoolean PVXMLSession::TraverseIf(PXMLElement & element)
{
  // If 'cond' parameter evaluates to true, enter child entities, else
  // go to next element.

  PString condition = element.GetAttribute("cond");

  // Find comparison type
  PINDEX location = condition.Find("==");
  if (location == P_MAX_INDEX) {
    PTRACE(1, "VXML\t<if> element contains condition with operator other than ==, not implemented" );
    return false;
  }

  // Find var name
  PString varname = condition.Left(location);

  // Find value, skip '=' signs
  PString cond_value = condition.Mid(location + 3);

  // check if var value equals value from condition and if not skip child elements
  PCaselessString value = GetVar(varname);
  if (value == cond_value) {
    PTRACE(3, "VXML\tCondition matched \"" << condition << '"');
  }
  else {
    PTRACE(3, "VXMLSess\t\tCondition \"" << condition << "\"did not match, " << varname << " == " << value);
    if (element.HasSubObjects())
      // Step to last child element (really last element is NULL?)
      m_currentNode = element.GetElement(element.GetSize() - 1);
  }

  return true;
}


PBoolean PVXMLSession::TraverseExit(PXMLElement &)
{
  PTRACE(2, "VXML\tExiting, fast forwarding through script");
  m_abortVXML = true;
  Trigger();
  return true;
}


PBoolean PVXMLSession::TraverseSubmit(PXMLElement & element)
{
  PBoolean result = false;

  // Do HTTP client stuff here

  // Find out what to submit, for now, only support a WAV file
  if (!element.HasAttribute("namelist")){
    PTRACE(1, "VXML\t<submit> does not contain \"namelist\" parameter");
    return false;
  }

  PString name = element.GetAttribute("namelist");

  if (name.Find(" ") < name.GetSize()) {
    PTRACE(1, "VXML\t<submit> does not support more than one value in \"namelist\" parameter");
    return false;
  }

  if (!element.HasAttribute("next")) {
    PTRACE(1, "VXML\t<submit> does not contain \"next\" parameter");
    return false;
  }

  PString url = element.GetAttribute("next");

  if (url.Find( "http://" ) > url.GetSize()) {
    PTRACE(1, "VXML\t<submit> needs a full url as the \"next\" parameter");
    return false;
  }

  if (GetVar(name + ".type") != "audio/x-wav" ) {
    PTRACE(1, "VXML\t<submit> does not (yet) support submissions of types other than \"audio/x-wav\"");
    return false;
  }

  PFilePath fileName = GetVar(name + ".filename");

  if (!(element.HasAttribute("method"))) {
    PTRACE(1, "VXML\t<submit> does not (yet) support default method type \"get\"");
    return false;
  }

  if ( !PFile::Exists(fileName )) {
    PTRACE(1, "VXML\t<submit> cannot find file " << fileName);
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

  if (element.GetAttribute("method") *= "post") {

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
    if (element.GetAttribute("method") != "get") {
      PTRACE(1, "VXML\t<submit> does not (yet) support method type \"" << element.GetAttribute("method") << "\"");
      return false;
    }

    PString getURL = url + "?" + name + "=" + GetVar( name );

    client.GetDocument( url, sendMIME, replyMIME );
    // TODO, Later:
    // Load reply from server as new VXML document ala <goto>
  }

  if (!result) {
    PTRACE(1, "VXML\t<submit> to server failed with "
        << client.GetLastResponseCode() << " "
        << client.GetLastResponseInfo() );
  }

  return result;
}


PBoolean PVXMLSession::TraverseProperty(PXMLElement & element)
{
  if (element.HasAttribute("name"))
    SetVar("property." + element.GetAttribute("name"), element.GetAttribute("value"));

  return true;
}


PBoolean PVXMLSession::TraverseTransfer(PXMLElement &)
{
  m_transferStatus = NotTransfering;
  return true;
}


PBoolean PVXMLSession::TraversedTransfer(PXMLElement & element)
{
  if (m_transferStatus == NotTransfering) {
    TransferType type = BridgedTransfer;
    if (element.GetAttribute("bridge") *= "false")
      type = BlindTransfer;
    else {
      PCaselessString typeStr = element.GetAttribute("type");
      if (typeStr == "blind")
        type = BlindTransfer;
      else if (typeStr == "consultation")
        type = ConsultationTransfer;
    }

    m_transferStartTime.SetCurrentTime();

    bool started = false;
    if (element.HasAttribute("dest"))
      started = OnTransfer(element.GetAttribute("dest"), type);
    else if (element.HasAttribute("destexpr"))
      started = OnTransfer(EvaluateExpr(element.GetAttribute("destexpr")), type);

    if (started) {
      m_transferStatus = TransferInProgress;
      return false;
    }

    m_transferStatus = TransferFailed;
  }
  else {
    PString name = element.GetAttribute("name");
    if (!name.IsEmpty())
      SetVar(name + "$.duration", PString(PString::Unsigned, (PTime() - m_transferStartTime).GetSeconds()));
  }

  return GoToEventHandler(element, m_transferStatus == TransferSuccessful ? "filled" : "error");
}


void PVXMLSession::SetTransferComplete(bool state)
{
  PTRACE(3, "VXML\tTransfer " << (state ? "completed" : "failed"));
  m_transferStatus = state ? TransferSuccessful : TransferFailed;
  Trigger();
}


PBoolean PVXMLSession::TraverseMenu(PXMLElement & element)
{
  LoadGrammar(new PVXMLMenuGrammar(*this, element));
  m_defaultMenuDTMF = (element.GetAttribute("dtmf") *= "true") ? '1' : 'N';
  return true;
}


PBoolean PVXMLSession::TraversedMenu(PXMLElement &)
{
  return ProcessGrammar();
}


PBoolean PVXMLSession::TraverseChoice(PXMLElement & element)
{
  if (!element.HasAttribute("dtmf") && m_defaultMenuDTMF <= '9')
    element.SetAttribute("dtmf", PString(m_defaultMenuDTMF++));

  return true;
}


PBoolean PVXMLSession::TraverseVar(PXMLElement & element)
{
  PString name = element.GetAttribute("name");
  PString expr = element.GetAttribute("expr");

  if (name.IsEmpty() || expr.IsEmpty()) {
    PTRACE(1, "VXML\t<var> has a problem with its parameters, name=\"" << name << "\", expr=\"" << expr << "\"" );
    return false;
  }

  SetVar(name, EvaluateExpr(expr));
  return true;
}


PBoolean PVXMLSession::TraverseDisconnect(PXMLElement &)
{
  m_currentNode = NULL;
  return true;
}


PBoolean PVXMLSession::TraverseForm(PXMLElement &)
{
  m_variableScope = "dialog";
  return true;
}


PBoolean PVXMLSession::TraversedForm(PXMLElement &)
{
  m_variableScope = "application";
  return true;
}


PBoolean PVXMLSession::TraversePrompt(PXMLElement & element)
{
  // LATER:
  // check 'cond' attribute to see if the children of this node should be processed
  // check 'count' attribute to see if this node should be processed
  // flush all prompts if 'bargein' attribute is set to false

  // Update timeout of current recognition (if 'timeout' attribute is set)
  if (element.HasAttribute("timeout")) {
    PTimeInterval timeout = StringToTime(element.GetAttribute("timeout"));
  }

  return !PlayElement(element);
}


PBoolean PVXMLSession::TraverseField(PXMLElement &)
{
  return true;
}


PBoolean PVXMLSession::TraversedField(PXMLElement &)
{
  return ProcessGrammar();
}


void PVXMLSession::OnEndRecording()
{
  //SetVar(channelName + ".size", PString(incomingChannel->GetWAVFile()->GetDataLength() ) );
  //SetVar(channelName + ".type", "audio/x-wav" );
  //SetVar(channelName + ".filename", incomingChannel->GetWAVFile()->GetName() );
  m_recordingStatus = RecordingComplete;
  Trigger();
}


void PVXMLSession::Trigger()
{
  PTRACE(4, "VXML\tEvent triggered");
  m_waitForEvent.Signal();
}



/////////////////////////////////////////////////////////////////////////////////////////

PVXMLGrammar::PVXMLGrammar(PVXMLSession & session, PXMLElement & field)
  : m_session(session)
  , m_field(field)
  , m_state(Idle)
{
  m_timer.SetNotifier(PCREATE_NOTIFIER(OnTimeout));
}


void PVXMLGrammar::Start()
{
  m_state = Started;
  m_timer = PVXMLSession::StringToTime(m_session.GetVar("property.timeout"));
  PTRACE(3, "VXML\tStarted grammar " << *this << ", timeout=" << m_timer);
}


void PVXMLGrammar::OnTimeout(PTimer &, INT)
{
  PTRACE(3, "VXML\tTimeout for grammar " << *this);
  m_mutex.Wait();

  if (m_state == Started) {
    m_state = NoInput;
    m_session.Trigger();
  }

  m_mutex.Signal();
}


bool PVXMLGrammar::Process()
{
  // Figure out what happened
  switch (m_state) {
    case Filled:
      if (m_field.HasAttribute("name"))
        m_session.SetVar(m_field.GetAttribute("name"), m_value);
      return m_session.GoToEventHandler(m_field, "filled");

    case PVXMLGrammar::NoInput:
      return m_session.GoToEventHandler(m_field, "noinput");

    case PVXMLGrammar::NoMatch:
      return m_session.GoToEventHandler(m_field, "nomatch");

    default:
      break; //ERROR - unexpected grammar state
  }

  return true; // Next node
}


//////////////////////////////////////////////////////////////////

PVXMLMenuGrammar::PVXMLMenuGrammar(PVXMLSession & session, PXMLElement & field)
  : PVXMLGrammar(session, field)
{
}


void PVXMLMenuGrammar::OnUserInput(const char ch)
{
  m_mutex.Wait();

  m_value = ch;
  m_state = PVXMLGrammar::Filled;

  m_mutex.Signal();
}


bool PVXMLMenuGrammar::Process()
{
  if (m_state == Filled) {
    PXMLElement * choice;
    PINDEX index = 0;
    while ((choice = m_field.GetElement("choice", index++)) != NULL) {
      // Check if DTMF value for grammarResult matches the DTMF value for the choice
      if (choice->GetAttribute("dtmf") == m_value) {
        PTRACE(3, "VXML\tMatched menu choice: " << m_value);
        if (m_session.SetCurrentForm(choice->GetAttribute("next"), true))
          return false;

        return m_session.GoToEventHandler(m_field, choice->GetAttribute("event"));
      }
    }

    m_state = NoMatch;
  }

  return PVXMLGrammar::Process();
}


//////////////////////////////////////////////////////////////////

PVXMLDigitsGrammar::PVXMLDigitsGrammar(PVXMLSession & session,
                                       PXMLElement & field,
                                       PINDEX minDigits,
                                       PINDEX maxDigits,
                                       PString terminators)
  : PVXMLGrammar(session, field)
  , m_minDigits(minDigits)
  , m_maxDigits(maxDigits)
  , m_terminators(terminators)
{
  PAssert(minDigits <= maxDigits, PInvalidParameter);
}


void PVXMLDigitsGrammar::OnUserInput(const char ch)
{
  PWaitAndSignal mutex(m_mutex);

  // Ignore any keys until we are running
  if (m_state != Started)
    return;

  PINDEX len = m_value.GetLength();

  // is this char the terminator?
  if (m_terminators.Find(ch) != P_MAX_INDEX) {
    m_state = (len >= m_minDigits && len <= m_maxDigits) ? Filled : NoMatch;
    return;
  }

  // Otherwise add to the grammar and check to see if we're done
  m_value += ch;
  if (++len >= m_maxDigits)
    m_state = PVXMLGrammar::Filled;   // the grammar is filled!
}


//////////////////////////////////////////////////////////////////

PVXMLChannel::PVXMLChannel(unsigned frameDelay, PINDEX frameSize)
  : PDelayChannel(DelayReadsAndWrites, frameDelay, frameSize)
  , m_vxmlSession(NULL)
  , m_sampleFrequency(8000)
  , m_closed(false)
  , m_paused(false)
  , m_totalData(0)
  , m_recordable(NULL)
  , m_currentPlayItem(NULL)
{
}


PBoolean PVXMLChannel::Open(PVXMLSession * session)
{
  m_currentPlayItem = NULL;
  m_vxmlSession = session;
  m_silenceTimer.SetInterval(500); // 1/2 a second delay before we start outputting stuff
  PTRACE(4, "VXML\tOpening channel " << this);
  return true;
}


PVXMLChannel::~PVXMLChannel()
{
  Close();
}


PBoolean PVXMLChannel::IsOpen() const
{
  return !m_closed;
}


PBoolean PVXMLChannel::Close()
{
  if (!m_closed) {
    PTRACE(4, "VXML\tClosing channel " << this);

    EndRecording();
    FlushQueue();

    m_closed = true;

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
  PWAVFile * wav = new PWAVFile;
  if (!wav->SetFormat(mediaFormat)) {
    PTRACE(1, "VXML\tWAV file format " << mediaFormat << " not known");
    delete wav;
    return NULL;
  }

  wav->SetAutoconvert();
  if (!wav->Open(fn,
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

  else if (wav->GetSampleRate() != GetSampleFrequency())
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
  if (m_closed)
    return false;

  m_channelWriteMutex.Wait();

  // let the recordable do silence detection
  if (m_recordable != NULL && m_recordable->OnFrame(IsSilenceFrame(buf, len)))
    EndRecording();

  m_channelWriteMutex.Signal();

  // write the data and do the correct delay
  if (WriteFrame(buf, len))
    m_totalData += lastWriteCount;
  else {
    EndRecording();
    lastWriteCount = len;
    Wait(len, nextWriteTick);
  }

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
  m_totalData = 0;

  // shutdown any existing recording
  EndRecording();

  // insert the new recordable
  PWaitAndSignal mutex(m_channelWriteMutex);
  m_recordable = newItem;
  m_totalData = 0;
  SetReadTimeout(frameDelay);
  return newItem->OnStart(*this);
}


PBoolean PVXMLChannel::EndRecording()
{
  PWaitAndSignal mutex(m_channelWriteMutex);

  if (m_recordable == NULL)
    return false;

  PTRACE(3, "VXML\tFinished recording " << m_totalData << " bytes");
  m_recordable->OnStop();
  delete m_recordable;
  m_recordable = NULL;
  m_vxmlSession->OnEndRecording();

  return true;
}


PBoolean PVXMLChannel::Read(void * buffer, PINDEX amount)
{
  for (;;) {
    if (m_closed)
      return false;

    if (m_paused || m_silenceTimer.IsRunning())
      break;

    // if the read succeeds, we are done
    if (ReadFrame(buffer, amount)) {
      m_totalData += lastReadCount;
      return true; // Already done real time delay
    }

    // if a timeout, send silence, try again in a bit
    if (GetErrorCode(LastReadError) == Timeout)
      break;

    // Other errors mean end of the playable
    PWaitAndSignal mutex(m_channelReadMutex);

    // if current item still active, check for trailing actions
    if (m_currentPlayItem != NULL) {
      PTRACE(3, "VXML\tFinished playing " << *m_currentPlayItem << ", " << m_totalData << " bytes");

      if (m_currentPlayItem->OnRepeat())
        continue;

      // see if end of queue delay specified
      if (m_currentPlayItem->OnDelay()) 
        break;

      // stop the current item
      m_currentPlayItem->OnStop();
      delete m_currentPlayItem;
      m_currentPlayItem = NULL;
      m_vxmlSession->Trigger();
    }

    for (;;) {
      // check the queue for the next action, if none, send silence
      m_currentPlayItem = m_playQueue.Dequeue();
      if (m_currentPlayItem == NULL)
        goto double_break;

      // start the new item
      if (m_currentPlayItem->OnStart())
        break;

      delete m_currentPlayItem;
    }

    PTRACE(4, "VXML\tStarted playing " << *m_currentPlayItem);
    SetReadTimeout(frameDelay);
    m_totalData = 0;
  }

double_break:
  lastReadCount = CreateSilenceFrame(buffer, amount);
  Wait(lastReadCount, nextReadTick);
  return true;
}


void PVXMLChannel::SetSilence(unsigned msecs)
{
  PTRACE(3, "VXML\tPlaying silence for " << msecs << "ms");
  m_silenceTimer.SetInterval(msecs);
}


PBoolean PVXMLChannel::QueuePlayable(const PString & type,
                                 const PString & arg,
                                 PINDEX repeat,
                                 PINDEX delay,
                                 PBoolean autoDelete)
{
  if (repeat <= 0)
    repeat = 1;

  PVXMLPlayable * item = PFactory<PVXMLPlayable>::CreateInstance(type);
  if (item == NULL) {
    PTRACE(2, "VXML\tCannot find playable of type " << type);
    return false;
  }

  if (item->Open(*this, arg, delay, repeat, autoDelete)) {
    PTRACE(3, "VXML\tEnqueueing playable " << type << " with arg \"" << arg
           << "\" for playing " << repeat << " times, followed by " << delay << "ms silence");
    return QueuePlayable(item);
  }

  delete item;
  return false;
}


PBoolean PVXMLChannel::QueuePlayable(PVXMLPlayable * newItem)
{
  if (!IsOpen()) {
    delete newItem;
    return false;
  }

  newItem->SetSampleFrequency(GetSampleFrequency());
  m_channelReadMutex.Wait();
  m_playQueue.Enqueue(newItem);
  m_channelReadMutex.Signal();
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
  PTRACE(3, "VXML\tEnqueueing " << data.GetSize() << " bytes for playing, followed by " << delay << "ms silence");
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

  return QueuePlayable(item);
}


void PVXMLChannel::FlushQueue()
{
  PTRACE(4, "VXML\tFlushing playable queue");

  PWaitAndSignal mutex(m_channelReadMutex);

  PVXMLPlayable * qItem;
  while ((qItem = m_playQueue.Dequeue()) != NULL) {
    qItem->OnStop();
    delete qItem;
  }

  if (m_currentPlayItem != NULL) {
    m_currentPlayItem->OnStop();
    delete m_currentPlayItem;
    m_currentPlayItem = NULL;
  }

  m_silenceTimer.Stop();

  PTRACE(4, "VXML\tFlushed playable queue");
}


///////////////////////////////////////////////////////////////

PFactory<PVXMLChannel>::Worker<PVXMLChannelPCM> pcmVXMLChannelFactory(VXML_PCM16);

PVXMLChannelPCM::PVXMLChannelPCM()
  : PVXMLChannel(10, 160)
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
  return PDelayChannel::Read(buffer, 10); // No silence frames so always 10 bytes
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
