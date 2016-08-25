/*
 * pvidfile.cxx
 *
 * Video file implementation
 *
 * Portable Windows Library
 *
 * Copyright (C) 2004 Post Increment
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
 * Craig Southeren <craigs@postincrement.com>
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
#pragma implementation "pvidfile.h"
#endif

#include <ptlib.h>

#if P_VIDEO
#if P_VIDFILE

#include <ptclib/pvidfile.h>
#include <ptlib/videoio.h>


///////////////////////////////////////////////////////////////////////////////

PVideoFile::PVideoFile()
  : m_fixedFrameSize(false)
  , m_fixedFrameRate(false)
  , m_frameBytes(CalculateFrameBytes())
  , m_headerOffset(0)
{
}


PBoolean PVideoFile::Open(const PFilePath & name, PFile::OpenMode mode, int opts)
{
  static PRegularExpression res("_(sqcif|qcif|cif|cif4|cif16|[0-9]+x[0-9]+)[^a-z0-9]", PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  static PRegularExpression fps("_[0-9]+fps[^a-z]",     PRegularExpression::Extended|PRegularExpression::IgnoreCase);

  PINDEX pos, len;

  if (name.FindRegEx(res, pos, len)) {
    m_fixedFrameSize = Parse(name.Mid(pos+1, len-2));
    if (m_fixedFrameSize)
      m_frameBytes = CalculateFrameBytes();
  }

  if ((pos = name.FindRegEx(fps)) != P_MAX_INDEX)
    m_fixedFrameRate = PVideoFrameInfo::SetFrameRate(name.Mid(pos+1).AsUnsigned());

  return m_file.Open(name, mode, opts);
}


PBoolean PVideoFile::WriteFrame(const void * frame)
{
  return m_file.Write(frame, m_frameBytes);
}


PBoolean PVideoFile::ReadFrame(void * frame)
{
  if (m_file.Read(frame, m_frameBytes) && m_file.GetLastReadCount() == m_frameBytes)
    return true;

#if PTRACING
  if (m_file.GetErrorCode(PFile::LastReadError) != PFile::NoError)
    PTRACE(2, "VidFile\tError reading file \"" << m_file.GetFilePath()
           << "\" - " << m_file.GetErrorText(PFile::LastReadError));
  else
    PTRACE(4, "VidFile\tEnd of file \"" << m_file.GetFilePath() << '"');
#endif
  return false;
}


off_t PVideoFile::GetLength() const
{
  off_t len = m_file.GetLength();
  return len < m_headerOffset ? 0 : ((len - m_headerOffset)/m_frameBytes);
}


PBoolean PVideoFile::SetLength(off_t len)
{
  return m_file.SetLength(len*m_frameBytes + m_headerOffset);
}


off_t PVideoFile::GetPosition() const
{
  off_t pos = m_file.GetPosition();
  return pos < m_headerOffset ? 0 : ((pos - m_headerOffset)/m_frameBytes);
}


PBoolean PVideoFile::SetPosition(off_t pos, PFile::FilePositionOrigin origin)
{
  pos *= m_frameBytes;
  if (origin == PFile::Start)
    pos += m_headerOffset;

  return m_file.SetPosition(pos, origin);
}


PBoolean PVideoFile::SetFrameSize(unsigned width, unsigned height)
{
  if (frameWidth == width && frameHeight == height)
    return true;

  if (m_fixedFrameSize)
    return false;

  if (!PVideoFrameInfo::SetFrameSize(width, height))
    return false;

  m_frameBytes = CalculateFrameBytes();
  return m_frameBytes > 0;
}


PBoolean PVideoFile::SetFrameRate(unsigned rate)
{
  if (frameRate == rate)
    return true;

  if (m_fixedFrameRate)
    return false;
  
  return PVideoFrameInfo::SetFrameRate(rate);
}



///////////////////////////////////////////////////////////////////////////////

PFACTORY_CREATE(PFactory<PVideoFile>, PYUVFile, "yuv", false);
static PFactory<PVideoFile>::Worker<PYUVFile> y4mFileFactory("y4m");


PYUVFile::PYUVFile()
  : m_y4mMode(false)
{
}


PBoolean PYUVFile::Open(const PFilePath & name, PFile::OpenMode mode, int opts)
{
  if (!PVideoFile::Open(name, mode, opts))
    return false;

  m_y4mMode = name.GetType() *= ".y4m";

  if (m_y4mMode) {
    int ch;
    do {
      if ((ch = m_file.ReadChar()) < 0)
        return false;
    }
    while (ch != '\n');
    m_headerOffset = m_file.GetPosition();
  }

  return true;
}


PBoolean PYUVFile::WriteFrame(const void * frame)
{
  if (m_y4mMode)
    m_file.WriteChar('\n');

  return m_file.Write(frame, m_frameBytes);
}


PBoolean PYUVFile::ReadFrame(void * frame)
{
  if (m_y4mMode) {
    PString info;
    info.ReadFrom(m_file);
    PTRACE(4, "VidFile\ty4m \"" << info << '"');
  }

  return PVideoFile::ReadFrame(frame);
}


#endif  // P_VIDFILE
#endif  // P_VIDEO
