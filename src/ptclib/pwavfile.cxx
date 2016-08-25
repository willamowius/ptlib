/*
 * pwavfile.cxx
 *
 * WAV file I/O channel class.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is
 * Roger Hardiman <roger@freebsd.org>
 * and Shawn Pai-Hsiang Hsiao <shawn@eecs.harvard.edu>
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifdef __GNUC__
#pragma implementation "pwavfile.h"
#endif

#include <ptlib.h>
#include <ptlib/pfactory.h>
#include <ptclib/pwavfile.h>

#define new PNEW


const char WAVLabelRIFF[4] = { 'R', 'I', 'F', 'F' };
const char WAVLabelWAVE[4] = { 'W', 'A', 'V', 'E' };
const char WAVLabelFMT_[4] = { 'f', 'm', 't', ' ' };
const char WAVLabelFACT[4] = { 'F', 'A', 'C', 'T' };
const char WAVLabelDATA[4] = { 'd', 'a', 't', 'a' };


inline PBoolean ReadAndCheck(PWAVFile & file, void * buf, PINDEX len)
{
  return file.FileRead(buf, len) && (file.PFile::GetLastReadCount() == len);
}


#if PBYTE_ORDER==PBIG_ENDIAN
#  if defined(USE_SYSTEM_SWAB)
#    define SWAB(a,b,c) ::swab(a,b,c)
#  else
static void SWAB(const void * void_from, void * void_to, register size_t len)
{
  register const char * from = (const char *)void_from;
  register char * to = (char *)void_to;

  while (len > 1) {
    char b = *from++;
    *to++ = *from++;
    *to++ = b;
    len -= 2;
  }
}
#  endif
#else
#  define SWAB(a,b,c) {}
#endif

///////////////////////////////////////////////////////////////////////////////
// PWAVFile

PWAVFile::PWAVFile(unsigned fmt)
  : origFmt(fmt)
{
  Construct();
  SelectFormat(fmt);
}

PWAVFile::PWAVFile(OpenMode mode, int opts, unsigned fmt)
  : PFile(mode, opts), origFmt(fmt)
{
  Construct();
  SelectFormat(fmt);
}

PWAVFile::PWAVFile(const PFilePath & name, OpenMode mode, int opts, unsigned fmt)
  : origFmt(fmt)
{
  Construct();
  SelectFormat(fmt);
  Open(name, mode, opts);
}

PWAVFile::PWAVFile(
  const PString & format,  
  const PFilePath & name,  
  OpenMode mode,
  int opts 
)
{
  origFmt = 0xffffffff;
  Construct();
  SelectFormat(format);
  Open(name, mode, opts);
}

PWAVFile::~PWAVFile()
{ 
  Close(); 

  delete formatHandler;
  delete autoConverter;
}


void PWAVFile::Construct()
{
  lenData                 = 0;
  lenHeader               = 0;
  isValidWAV              = PFalse;
  header_needs_updating   = PFalse;
  autoConvert             = PFalse;
  autoConverter           = NULL;

  formatHandler           = NULL;
  wavFmtChunk.hdr.len     = sizeof(wavFmtChunk) - sizeof(wavFmtChunk.hdr);
}


PWAVFile * PWAVFile::format(const PString & format)
{
  PWAVFile * file = new PWAVFile;
  file->origFmt = 0xffffffff;
  file->SelectFormat(format);
  return file;
}

PWAVFile * PWAVFile::format(const PString & format, PFile::OpenMode mode, int opts)
{
  PWAVFile * file = new PWAVFile(mode, opts);
  file->origFmt = 0xffffffff;
  file->SelectFormat(format);
  return file;
}


bool PWAVFile::SelectFormat(unsigned fmt)
{
  delete formatHandler;
  formatHandler = NULL;

  if (fmt == fmt_NotKnown)
    return true;

  formatHandler = PWAVFileFormatByIDFactory::CreateInstance(fmt);
  if (formatHandler == NULL)
    return false;

  wavFmtChunk.format  = (WORD)fmt;
  return true;
}

bool PWAVFile::SelectFormat(const PString & format)
{
  delete formatHandler;
  formatHandler = NULL;

  if (format.IsEmpty())
    return false;

  formatHandler = PWAVFileFormatByFormatFactory::CreateInstance(format);
  if (formatHandler == NULL)
    return SelectFormat(format.AsUnsigned());

  wavFmtChunk.format = (WORD)formatHandler->GetFormat();
  if (origFmt == 0xffffffff)
    origFmt = wavFmtChunk.format;
  return true;
}

PBoolean PWAVFile::Open(OpenMode mode, int opts)
{
  if (!(PFile::Open(mode, opts)))
    return PFalse;

  isValidWAV = PFalse;

  // Try and process the WAV file header information.
  // Either ProcessHeader() or GenerateHeader() must be called.

  if (PFile::GetLength() > 0) {

    // try and process the WAV file header information
    if (mode == ReadOnly || mode == ReadWrite) {
      isValidWAV = ProcessHeader();
    }
    if (mode == WriteOnly) {
      lenData = -1;
      GenerateHeader();
    }
  }
  else {

    // generate header
    if (mode == ReadWrite || mode == WriteOnly) {
      lenData = -1;
      GenerateHeader();
    }
    if (mode == ReadOnly) {
      isValidWAV = PFalse; // ReadOnly on a zero length file
    }
  }

  // if we did not know the format when we opened, then we had better know it now
  if (formatHandler == NULL) {
    Close();
    SetErrorValues(BadParameter, EINVAL);
    return PFalse;
  }

  return PTrue;
}


PBoolean PWAVFile::Open(const PFilePath & name, OpenMode mode, int opts)
{
  if (IsOpen())
    Close();
  SetFilePath(name);
  return Open(mode, opts);
}


PBoolean PWAVFile::Close()
{
  delete autoConverter;
  autoConverter = NULL;

  if (!PFile::IsOpen())
    return PTrue;

  if (header_needs_updating)
    UpdateHeader();

  if (formatHandler != NULL) 
    formatHandler->OnStop();

  delete formatHandler;
  formatHandler = NULL;

  if (origFmt != 0xffffffff)
    SelectFormat(origFmt);

  return PFile::Close();
}

void PWAVFile::SetAutoconvert()
{ 
  autoConvert = PTrue; 
}


// Performs necessary byte-order swapping on for big-endian platforms.
PBoolean PWAVFile::Read(void * buf, PINDEX len)
{
  if (!IsOpen())
    return PFalse;

  if (autoConverter != NULL)
    return autoConverter->Read(*this, buf, len);

  return RawRead(buf, len);
}

PBoolean PWAVFile::RawRead(void * buf, PINDEX len)
{
  // Some wav files have extra data after the sound samples in a LIST chunk.
  // e.g. WAV files made in GoldWave have a copyright and a URL in this chunk.
  // We do not want to return this data by mistake.
  PINDEX readlen = len;
  off_t pos = PFile::GetPosition();

  // indicate eof (return false, but error=0, lastReadCount=0)
  if (pos >= (lenHeader + lenData)) {
    lastReadCount = 0;
    ConvertOSError(0, LastReadError);
    return PFalse;
  }

  if ((pos + len) > (lenHeader + lenData))
    readlen = (lenHeader+lenData) - pos;

  if (formatHandler != NULL)
    return formatHandler->Read(*this, buf, readlen);

  return FileRead(buf, readlen);
}

PBoolean PWAVFile::FileRead(void * buf, PINDEX len)
{
  return PFile::Read(buf, len);
}

// Performs necessary byte-order swapping on for big-endian platforms.
PBoolean PWAVFile::Write(const void * buf, PINDEX len)
{
  if (!IsOpen())
    return PFalse;

  // Needs to update header on close.
  header_needs_updating = PTrue;

  if (autoConverter != NULL)
    return autoConverter->Write(*this, buf, len);

  return RawWrite(buf, len);
}

PBoolean PWAVFile::RawWrite(const void * buf, PINDEX len)
{
  // Needs to update header on close.
  header_needs_updating = PTrue;

  if (formatHandler != NULL)
    return formatHandler->Write(*this, buf, len);

  return FileWrite(buf, len);
}

PBoolean PWAVFile::FileWrite(const void * buf, PINDEX len)
{
  return PFile::Write(buf, len);
}

// Both SetPosition() and GetPosition() are offset by lenHeader.
PBoolean PWAVFile::SetPosition(off_t pos, FilePositionOrigin origin)
{
  if (autoConverter != NULL)
    return autoConverter->SetPosition(*this, pos, origin);

  return RawSetPosition(pos, origin);
}

PBoolean PWAVFile::RawSetPosition(off_t pos, FilePositionOrigin origin)
{
  if (isValidWAV) {
    pos += lenHeader;
  }

  return PFile::SetPosition(pos, origin);
}


off_t PWAVFile::GetPosition() const
{
  if (autoConverter != NULL)
    return autoConverter->GetPosition(*this);

  return RawGetPosition();
}

off_t PWAVFile::RawGetPosition() const
{
  off_t pos = PFile::GetPosition();

  if (isValidWAV) {
    if (pos >= lenHeader) {
      pos -= lenHeader;
    }
    else {
      pos = 0;
    }
  }

  return (pos);
}


unsigned PWAVFile::GetFormat() const
{
  if (isValidWAV)
    return wavFmtChunk.format;
  else
    return 0;
}

PString PWAVFile::GetFormatAsString() const
{
  if (isValidWAV && formatHandler != NULL)
    return formatHandler->GetFormat();
  else
    return PString::Empty();
}

unsigned PWAVFile::GetChannels() const
{
  if (isValidWAV)
    return wavFmtChunk.numChannels;
  else
    return 0;
}

void PWAVFile::SetChannels(unsigned v) 
{
  if (formatHandler == NULL || formatHandler->CanSetChannels(v)) {
    wavFmtChunk.numChannels = (WORD)v;
    wavFmtChunk.bytesPerSample = (wavFmtChunk.bitsPerSample/8) * wavFmtChunk.numChannels;
    wavFmtChunk.bytesPerSec = wavFmtChunk.sampleRate * wavFmtChunk.bytesPerSample;
    header_needs_updating = PTrue;
  }
}

unsigned PWAVFile::GetSampleRate() const
{
  if (isValidWAV)
    return wavFmtChunk.sampleRate;
  else
    return 0;
}

void PWAVFile::SetSampleRate(unsigned v) 
{
  wavFmtChunk.sampleRate = (WORD)v;
  header_needs_updating = PTrue;
}

unsigned PWAVFile::GetSampleSize() const
{
  if (isValidWAV)
    return wavFmtChunk.bitsPerSample;
  else
    return 0;
}

void PWAVFile::SetSampleSize(unsigned v) 
{
  wavFmtChunk.bitsPerSample = (WORD)v;
  header_needs_updating = PTrue;
}

unsigned PWAVFile::GetBytesPerSecond() const
{
  if (isValidWAV)
    return wavFmtChunk.bytesPerSec;
  else
    return 0;
}

void PWAVFile::SetBytesPerSecond(unsigned v)
{
  wavFmtChunk.bytesPerSec = (WORD)v;
  header_needs_updating = PTrue;
}

off_t PWAVFile::GetHeaderLength() const
{
  if (isValidWAV)
    return lenHeader;
  else
    return 0;
}


off_t PWAVFile::GetDataLength()
{
  if (autoConverter != NULL)
    return autoConverter->GetDataLength(*this);

  return RawGetDataLength();
}

off_t PWAVFile::RawGetDataLength()
{
  if (isValidWAV) {
    // Updates data length before returns.
    lenData = PFile::GetLength() - lenHeader;
    return lenData;
  }
  else
    return 0;
}


PBoolean PWAVFile::SetFormat(unsigned fmt)
{
  if (IsOpen() || isValidWAV)
    return PFalse;

  SelectFormat(fmt);

  return PTrue;
}

PBoolean PWAVFile::SetFormat(const PString & format)
{
  if (IsOpen() || isValidWAV)
    return PFalse;

  return SelectFormat(format);
}

static inline PBoolean NeedsConverter(const PWAV::FMTChunk & fmtChunk)
{
  return (fmtChunk.format != PWAVFile::fmt_PCM) || (fmtChunk.bitsPerSample != 16);
}

PBoolean PWAVFile::ProcessHeader() 
{
  delete autoConverter;
  autoConverter = NULL;

  // Process the header information
  // This comes in 3 or 4 chunks, either RIFF, FORMAT and DATA
  // or RIFF, FORMAT, FACT and DATA.

  if (!IsOpen()) {
    PTRACE(1,"WAV\tProcessHeader: Not Open");
    return (PFalse);
  }

  // go to the beginning of the file
  if (!PFile::SetPosition(0)) {
    PTRACE(1,"WAV\tProcessHeader: Cannot Set Pos");
    return (PFalse);
  }

  // Read the RIFF chunk.
  struct PWAV::RIFFChunkHeader riffChunk;
  if (!ReadAndCheck(*this, &riffChunk, sizeof(riffChunk)))
    return PFalse;

  // check if tags are correct
  if (strncmp(riffChunk.hdr.tag, WAVLabelRIFF, sizeof(WAVLabelRIFF)) != 0) {
    PTRACE(1,"WAV\tProcessHeader: Not RIFF");
    return (PFalse);
  }

  if (strncmp(riffChunk.tag, WAVLabelWAVE, sizeof(WAVLabelWAVE)) != 0) {
    PTRACE(1,"WAV\tProcessHeader: Not WAVE");
    return (PFalse);
  }

  // Read the known part of the FORMAT chunk.
  if (!ReadAndCheck(*this, &wavFmtChunk, sizeof(wavFmtChunk)))
    return PFalse;

  // check if labels are correct
  if (strncmp(wavFmtChunk.hdr.tag, WAVLabelFMT_, sizeof(WAVLabelFMT_)) != 0) {
    PTRACE(1,"WAV\tProcessHeader: Not FMT");
    return (PFalse);
  }

  // if we opened the file without knowing the format, then try and set the format now
  if (formatHandler == NULL) {
    SelectFormat(wavFmtChunk.format);
    if (formatHandler == NULL) {
      Close();
      return PFalse;
    }
  }

  // read the extended format chunk (if any)
  extendedHeader.SetSize(0);
  if ((unsigned)wavFmtChunk.hdr.len > (sizeof(wavFmtChunk) - sizeof(wavFmtChunk.hdr))) {
    extendedHeader.SetSize(wavFmtChunk.hdr.len - (sizeof(wavFmtChunk) - sizeof(wavFmtChunk.hdr)));
    if (!ReadAndCheck(*this, extendedHeader.GetPointer(), extendedHeader.GetSize()))
      return PFalse;
  }

  // give format handler a chance to read extra chunks
  if (!formatHandler->ReadExtraChunks(*this))
    return PFalse;

  PWAV::ChunkHeader chunkHeader;

  // ignore chunks until we see a DATA chunk
  for (;;) {
    if (!ReadAndCheck(*this, &chunkHeader, sizeof(chunkHeader)))
      return PFalse;
    if (strncmp(chunkHeader.tag, WAVLabelDATA, sizeof(WAVLabelDATA)) == 0) 
      break;
    if (!PFile::SetPosition(PFile::GetPosition() + + chunkHeader.len)) {
      PTRACE(1,"WAV\tProcessHeader: Cannot set new position");
      return PFalse;
    }
  }

  // calculate the size of header and data for accessing the WAV data.
  lenHeader  = PFile::GetPosition(); 
  lenData    = chunkHeader.len;

  // get ptr to data handler if in autoconvert mode
  if (autoConvert && NeedsConverter(wavFmtChunk)) {
    autoConverter = PWAVFileConverterFactory::CreateInstance(wavFmtChunk.format);
    PTRACE_IF(1, autoConverter == NULL, "PWAVFile\tNo format converter for type " << (int)wavFmtChunk.format);
  }

  formatHandler->OnStart();

  return PTrue;
}


// Generates the wave file header.
// Two types of header are supported.
// a) PCM data, set to 8000Hz, mono, 16-bit samples
// b) G.723.1 data
// When this function is called with lenData < 0, it will write the header
// as if the lenData is LONG_MAX minus header length.
// Note: If it returns PFalse, the file may be left in inconsistent state.

PBoolean PWAVFile::GenerateHeader()
{
  delete autoConverter;
  autoConverter = NULL;

  if (!IsOpen()) {
    PTRACE(1, "WAV\tGenerateHeader: Not Open");
    return (PFalse);
  }

  // length of audio data is set to a large value if lenData does not
  // contain a valid (non negative) number. We must then write out real values
  // when we close the wav file.
  int audioDataLen;
  if (lenData < 0) {
    audioDataLen = LONG_MAX - wavFmtChunk.hdr.len;
    header_needs_updating = PTrue;
  } else {
    audioDataLen = lenData;
  }

  // go to the beginning of the file
  if (!PFile::SetPosition(0)) {
    PTRACE(1,"WAV\tGenerateHeader: Cannot Set Pos");
    return (PFalse);
  }

  // write the WAV file header
  PWAV::RIFFChunkHeader riffChunk;
  memcpy(riffChunk.hdr.tag, WAVLabelRIFF, sizeof(WAVLabelRIFF));
  memcpy(riffChunk.tag,     WAVLabelWAVE, sizeof(WAVLabelWAVE));
  riffChunk.hdr.len = lenHeader + audioDataLen - sizeof(riffChunk.hdr);

  if (!FileWrite(&riffChunk, sizeof(riffChunk)))
    return PFalse;

  // populate and write the WAV header with the default data
  memcpy(wavFmtChunk.hdr.tag,  WAVLabelFMT_, sizeof(WAVLabelFMT_));
  wavFmtChunk.hdr.len = sizeof(wavFmtChunk) - sizeof(wavFmtChunk.hdr);  // set default length assuming no extra bytes

  // allow the format handler to modify the header and extra bytes
  if(formatHandler == NULL){
    PTRACE(1,"WAV\tGenerateHeader: format handler is null!");
    return PFalse;
  }
  formatHandler->CreateHeader(wavFmtChunk, extendedHeader);

  // write the basic WAV header
  if (!FileWrite(&wavFmtChunk, sizeof(wavFmtChunk)))
    return false;

  if (extendedHeader.GetSize() > 0 && !FileWrite(extendedHeader.GetPointer(), extendedHeader.GetSize()))
    return false;

  // allow the format handler to write additional chunks
  if (!formatHandler->WriteExtraChunks(*this))
    return PFalse;

  // Write the DATA chunk.
  PWAV::ChunkHeader dataChunk;
  memcpy(dataChunk.tag, WAVLabelDATA, sizeof(WAVLabelDATA));
  dataChunk.len = audioDataLen;
  if (!FileWrite(&dataChunk, sizeof(dataChunk)))
    return PFalse;

  isValidWAV = PTrue;

  // get the length of the header
  lenHeader = PFile::GetPosition();

  // get pointer to auto converter 
  if (autoConvert && NeedsConverter(wavFmtChunk)) {
    autoConverter = PWAVFileConverterFactory::CreateInstance(wavFmtChunk.format);
    if (autoConverter == NULL) {
      PTRACE(1, "PWAVFile\tNo format converter for type " << (int)wavFmtChunk.format);
      return PFalse;
    }
  }

  return (PTrue);
}

// Update the WAV header according to the file length
PBoolean PWAVFile::UpdateHeader()
{
  // Check file is still open
  if (!IsOpen()) {
    PTRACE(1,"WAV\tUpdateHeader: Not Open");
    return (PFalse);
  }

  // Check there is already a valid header
  if (!isValidWAV) {
    PTRACE(1,"WAV\tUpdateHeader: File not valid");
    return (PFalse);
  }

  // Find out the length of the audio data
  lenData = PFile::GetLength() - lenHeader;

  // rewrite the length in the RIFF chunk
  PInt32l riffChunkLen = (lenHeader - 8) + lenData; // size does not include first 8 bytes
  PFile::SetPosition(4);
  if (!FileWrite(&riffChunkLen, sizeof(riffChunkLen)))
    return PFalse;

  // rewrite the data length field in the data chunk
  PInt32l dataChunkLen = lenData;
  PFile::SetPosition(lenHeader - 4);
  if (!FileWrite(&dataChunkLen, sizeof(dataChunkLen)))
    return PFalse;

  if(formatHandler == NULL){
    PTRACE(1,"WAV\tGenerateHeader: format handler is null!");
    return PFalse;
  }
  formatHandler->UpdateHeader(wavFmtChunk, extendedHeader);

  PFile::SetPosition(12);
  if (!FileWrite(&wavFmtChunk, sizeof(wavFmtChunk)))
    return PFalse;

  if (!FileWrite(extendedHeader.GetPointer(), extendedHeader.GetSize()))
    return PFalse;

  header_needs_updating = PFalse;

  return PTrue;
}


//////////////////////////////////////////////////////////////////

PBoolean PWAVFileFormat::Read(PWAVFile & file, void * buf, PINDEX & len)
{ 
  if (!file.FileRead(buf, len))
    return PFalse;

  len = file.GetLastReadCount();
  return PTrue;
}

PBoolean PWAVFileFormat::Write(PWAVFile & file, const void * buf, PINDEX & len)
{ 
  if (!file.FileWrite(buf, len))
    return PFalse;

  len = file.GetLastWriteCount();
  return PTrue;
}

//////////////////////////////////////////////////////////////////

class PWAVFileFormatPCM : public PWAVFileFormat
{
  public:
    void CreateHeader(PWAV::FMTChunk & wavFmtChunk, PBYTEArray & extendedHeader);
    void UpdateHeader(PWAV::FMTChunk & wavFmtChunk, PBYTEArray & extendedHeader);

    unsigned GetFormat() const
    { return PWAVFile::fmt_PCM; }

    bool CanSetChannels(unsigned channels) const
    { return channels > 0 && channels < 7; }

    PString GetDescription() const
    { return "PCM"; }

    PString GetFormatString() const
    { return "PCM-16"; }

    PBoolean Read(PWAVFile & file, void * buf, PINDEX & len);
    PBoolean Write(PWAVFile & file, const void * buf, PINDEX & len);
};

PFACTORY_CREATE(PWAVFileFormatByIDFactory, PWAVFileFormatPCM, PWAVFile::fmt_PCM, false);
PWAVFileFormatByFormatFactory::Worker<PWAVFileFormatPCM> pcmFormatWAVFormat("PCM-16");

void PWAVFileFormatPCM::CreateHeader(PWAV::FMTChunk & wavFmtChunk, 
                                     PBYTEArray & /*extendedHeader*/)
{
  wavFmtChunk.hdr.len         = sizeof(wavFmtChunk) - sizeof(wavFmtChunk.hdr);  // no extended information
  wavFmtChunk.format          = PWAVFile::fmt_PCM;
  wavFmtChunk.numChannels     = 1;
  wavFmtChunk.sampleRate      = 8000;
  wavFmtChunk.bytesPerSample  = 2;
  wavFmtChunk.bitsPerSample   = 16;
  wavFmtChunk.bytesPerSec     = wavFmtChunk.sampleRate * wavFmtChunk.bytesPerSample;
}

void PWAVFileFormatPCM::UpdateHeader(PWAV::FMTChunk & wavFmtChunk, 
                                     PBYTEArray & /*extendedHeader*/)
{
  wavFmtChunk.bytesPerSample  = 2 * wavFmtChunk.numChannels;
  wavFmtChunk.bytesPerSec     = wavFmtChunk.sampleRate * 2 * wavFmtChunk.numChannels;
}

PBoolean PWAVFileFormatPCM::Read(PWAVFile & file, void * buf, PINDEX & len)
{
  if (!file.FileRead(buf, len))
    return PFalse;

  len = file.GetLastReadCount();

  // WAV files are little-endian. So swap the bytes if this is
  // a big endian machine and we have 16 bit samples
  // Note: swab only works on even length buffers.
  if (file.GetSampleSize() == 16) {
    SWAB(buf, buf, len);
  }

  return PTrue;
}

PBoolean PWAVFileFormatPCM::Write(PWAVFile & file, const void * buf, PINDEX & len)
{
  // WAV files are little-endian. So swap the bytes if this is
  // a big endian machine and we have 16 bit samples
  // Note: swab only works on even length buffers.
  if (file.GetSampleSize() == 16) {
    SWAB(buf, (void *)buf, len);
  }

  if (!file.FileWrite(buf, len))
    return PFalse;

  len = file.GetLastWriteCount();
  return PTrue;
}

//////////////////////////////////////////////////////////////////

#ifdef __GNUC__
#define P_PACKED    __attribute__ ((packed));
#else
#define P_PACKED
#pragma pack(1)
#endif

struct G7231ExtendedInfo {
  PInt16l data1      P_PACKED;      // 1
  PInt16l data2      P_PACKED;      // 480
};

struct G7231FACTChunk {
  PWAV::ChunkHeader hdr;
  PInt32l data1      P_PACKED;      // 0   Should be number of samples.
};

#ifdef __GNUC__
#undef P_PACKED
#else
#pragma pack()
#endif


class PWAVFileFormatG7231 : public PWAVFileFormat
{
  public:
    PWAVFileFormatG7231(unsigned short _g7231)
      : g7231(_g7231) { }

    virtual ~PWAVFileFormatG7231() {}

    void CreateHeader(PWAV::FMTChunk & wavFmtChunk, PBYTEArray & extendedHeader);
    PBoolean WriteExtraChunks(PWAVFile & file);

    bool CanSetChannels(unsigned channels) const
    { return channels == 1; }

    PString GetFormatString() const
    { return "G.723.1"; }   // must match string in mediafmt.h

    void OnStart();
    PBoolean Read(PWAVFile & file, void * buf, PINDEX & len);
    PBoolean Write(PWAVFile & file, const void * buf, PINDEX & len);

  protected:
    unsigned short g7231;
    BYTE cacheBuffer[24];
    PINDEX cacheLen;
    PINDEX cachePos;
};

void PWAVFileFormatG7231::CreateHeader(PWAV::FMTChunk & wavFmtChunk, PBYTEArray & extendedHeader)
{
  wavFmtChunk.hdr.len         = sizeof(wavFmtChunk) - sizeof(wavFmtChunk.hdr) + sizeof(sizeof(G7231ExtendedInfo));
  wavFmtChunk.format          = g7231;
  wavFmtChunk.numChannels     = 1;
  wavFmtChunk.sampleRate      = 8000;
  wavFmtChunk.bytesPerSample  = 24;
  wavFmtChunk.bitsPerSample   = 0;
  wavFmtChunk.bytesPerSec     = 800;

  extendedHeader.SetSize(sizeof(G7231ExtendedInfo));
  G7231ExtendedInfo * g7231Info = (G7231ExtendedInfo *)extendedHeader.GetPointer(sizeof(G7231ExtendedInfo));

  g7231Info->data1 = 1;
  g7231Info->data2 = 480;
}

PBoolean PWAVFileFormatG7231::WriteExtraChunks(PWAVFile & file)
{
  // write the fact chunk
  G7231FACTChunk factChunk;
  memcpy(factChunk.hdr.tag, "FACT", 4);
  factChunk.hdr.len = sizeof(factChunk) - sizeof(factChunk.hdr);
  factChunk.data1 = 0;
  return file.FileWrite(&factChunk, sizeof(factChunk));
}

static PINDEX G7231FrameSizes[4] = { 24, 20, 4, 1 };

void PWAVFileFormatG7231::OnStart()
{
  cacheLen = cachePos = 0;
}

PBoolean PWAVFileFormatG7231::Read(PWAVFile & file, void * origData, PINDEX & origLen)
{
  // Note that Microsoft && VivoActive G.2723.1 codec cannot do SID frames, so
  // we must parse the data and remove SID frames
  // also note that frames are always written as 24 byte frames, so each frame must be unpadded

  PINDEX bytesRead = 0;
  while (bytesRead < origLen) {

    // keep reading until we find a 20 or 24 byte frame
    while (cachePos == cacheLen) {
      if (!file.FileRead(cacheBuffer, 24))
        return PFalse;

      // calculate actual length of frame
      PINDEX frameLen = G7231FrameSizes[cacheBuffer[0] & 3];
      if (frameLen == 20 || frameLen == 24) {
        cacheLen = frameLen;
        cachePos = 0;
      }
    }

    // copy data to requested buffer
    PINDEX copyLen = PMIN(origLen-bytesRead, cacheLen-cachePos);
    memcpy(origData, cacheBuffer+cachePos, copyLen);
    origData = copyLen + (char *)origData;
    cachePos += copyLen;
    bytesRead += copyLen;
  }

  origLen = bytesRead;

  return PTrue;
}

PBoolean PWAVFileFormatG7231::Write(PWAVFile & file, const void * origData, PINDEX & len)
{
  // Note that Microsoft && VivoActive G.2723.1 codec cannot do SID frames, so
  // we must parse the data and remove SID frames
  // also note that frames are always written as 24 byte frames, so each frame must be padded
  PINDEX written = 0;

  BYTE frameBuffer[24];
  while (len > 0) {

    // calculate actual length of frame
    PINDEX frameLen = G7231FrameSizes[(*(char *)origData) & 3];
    if (len < frameLen)
      return PFalse;

    // we can write 24 byte frame straight out, 
    // 20 byte frames need to be reblocked
    // we ignore any other frames

    const void * buf = NULL;
    switch (frameLen) {
      case 24:
        buf = origData;
        break;
      case 20:
        memcpy(frameBuffer, origData, 20);
        buf = frameBuffer;
        break;
      default:
        break;
    }

    if (buf != NULL && !file.FileWrite(buf, 24))
      return PFalse;
    else
      written += 24;

    origData = (char *)origData + frameLen;
    len -= frameLen;
  }

  len = written;

  return PTrue;
}

class PWAVFileFormatG7231_vivo : public PWAVFileFormatG7231
{
  public:
    PWAVFileFormatG7231_vivo()
      : PWAVFileFormatG7231(PWAVFile::fmt_VivoG7231) { }
    virtual ~PWAVFileFormatG7231_vivo() {}
    unsigned GetFormat() const
    { return PWAVFile::fmt_VivoG7231; }
    PString GetDescription() const
    { return GetFormatString() & "Vivo"; }
};

PWAVFileFormatByIDFactory::Worker<PWAVFileFormatG7231_vivo> g7231VivoWAVFormat(PWAVFile::fmt_VivoG7231);
PWAVFileFormatByFormatFactory::Worker<PWAVFileFormatG7231_vivo> g7231FormatWAVFormat("G.723.1");

class PWAVFileFormatG7231_ms : public PWAVFileFormatG7231
{
  public:
    PWAVFileFormatG7231_ms()
      : PWAVFileFormatG7231(PWAVFile::fmt_MSG7231) { }
    virtual ~PWAVFileFormatG7231_ms() {}
    unsigned GetFormat() const
    { return PWAVFile::fmt_MSG7231; }
    PString GetDescription() const
    { return GetFormatString() & "MS"; }
};

PWAVFileFormatByIDFactory::Worker<PWAVFileFormatG7231_ms> g7231MSWAVFormat(PWAVFile::fmt_MSG7231);

//////////////////////////////////////////////////////////////////

class PWAVFileConverterPCM : public PWAVFileConverter
{
  public:
    virtual ~PWAVFileConverterPCM() {}
    unsigned GetFormat    (const PWAVFile & file) const;
    off_t GetPosition     (const PWAVFile & file) const;
    PBoolean SetPosition      (PWAVFile & file, off_t pos, PFile::FilePositionOrigin origin);
    unsigned GetSampleSize(const PWAVFile & file) const;
    off_t GetDataLength   (PWAVFile & file);
    PBoolean Read             (PWAVFile & file, void * buf, PINDEX len);
    PBoolean Write            (PWAVFile & file, const void * buf, PINDEX len);
};

unsigned PWAVFileConverterPCM::GetFormat(const PWAVFile &) const
{
  return PWAVFile::fmt_PCM;
}

off_t PWAVFileConverterPCM::GetPosition(const PWAVFile & file) const
{
  off_t pos = file.RawGetPosition();
  return pos * 2;
}

PBoolean PWAVFileConverterPCM::SetPosition(PWAVFile & file, off_t pos, PFile::FilePositionOrigin origin)
{
  pos /= 2;
  return file.SetPosition(pos, origin);
}

unsigned PWAVFileConverterPCM::GetSampleSize(const PWAVFile &) const
{
  return 16;
}

off_t PWAVFileConverterPCM::GetDataLength(PWAVFile & file)
{
  return file.RawGetDataLength() * 2;
}

PBoolean PWAVFileConverterPCM::Read(PWAVFile & file, void * buf, PINDEX len)
{
  if (file.GetSampleSize() == 16)
    return file.PWAVFile::RawRead(buf, len);

  if (file.GetSampleSize() != 8) {
    PTRACE(1, "PWAVFile\tAttempt to read autoconvert PCM data with unsupported number of bits per sample " << file.GetSampleSize());
    return PFalse;
  }

  // read the PCM data with 8 bits per sample
  PINDEX samples = (len / 2);
  PBYTEArray pcm8;
  if (!file.PWAVFile::RawRead(pcm8.GetPointer(samples), samples))
    return PFalse;

  // convert to PCM-16
  PINDEX i;
  short * pcmPtr = (short *)buf;
  for (i = 0; i < samples; i++)
    *pcmPtr++ = (unsigned short)((pcm8[i] << 8) - 0x8000);

  // fake the lastReadCount
  file.SetLastReadCount(len);

  return PTrue;
}


PBoolean PWAVFileConverterPCM::Write(PWAVFile & file, const void * buf, PINDEX len)
{
  if (file.GetSampleSize() == 16)
    return file.PWAVFile::RawWrite(buf, len);

  PTRACE(1, "PWAVFile\tAttempt to write autoconvert PCM data with unsupported number of bits per sample " << file.GetSampleSize());
  return PFalse;
}

PWAVFileConverterFactory::Worker<PWAVFileConverterPCM> pcmConverter(PWAVFile::fmt_PCM);

//////////////////////////////////////////////////////////////////
