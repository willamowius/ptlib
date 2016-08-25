/*
 * memfile.cxx
 *
 * memory file I/O channel class.
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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "memfile.h"
#endif

#include <ptclib/memfile.h>



//////////////////////////////////////////////////////////////////////////////

PMemoryFile::PMemoryFile()
  : m_position(0)
{
  os_handle = INT_MAX; // Start open
}


PMemoryFile::PMemoryFile(const PBYTEArray & data)
  : m_data(data)
  , m_position(0)
{
  os_handle = INT_MAX; // Start open
}


PMemoryFile::~PMemoryFile()
{
  Close();
}


PObject::Comparison PMemoryFile::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PMemoryFile), PInvalidCast);
  return m_data.Compare(((const PMemoryFile &)obj).m_data);
}


PBoolean PMemoryFile::Open(OpenMode, int)
{
  os_handle = INT_MAX;
  m_position = 0;
  return true;
}


PBoolean PMemoryFile::Open(const PFilePath &, OpenMode mode, int opts)
{
  return Open(mode, opts);
}

      
PBoolean PMemoryFile::Close()
{
  os_handle = -1;
  return true;
}


PBoolean PMemoryFile::Read(void * buf, PINDEX len)
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);

  if (m_position > m_data.GetSize()) {
    lastReadCount = 0;
    return true;
  }

  if ((m_position + len) > m_data.GetSize())
    len = m_data.GetSize() - m_position;

  memcpy(buf, m_position + (const BYTE * )m_data, len);
  m_position += len;
  lastReadCount = len;

  return lastReadCount > 0;
}


PBoolean PMemoryFile::Write(const void * buf, PINDEX len)
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);

  BYTE * ptr = m_data.GetPointer(m_position+len);
  if (ptr == NULL)
    return SetErrorValues(DiskFull, ENOMEM);

  memcpy(ptr + m_position, buf, len);
  m_position += len;
  lastWriteCount = len;
  return true;
}


off_t PMemoryFile::GetLength() const
{
  return m_data.GetSize();
}
      

PBoolean PMemoryFile::SetLength(off_t len)
{
  return m_data.SetSize(len);
}


PBoolean PMemoryFile::SetPosition(off_t pos, FilePositionOrigin origin)
{
  switch (origin) {
    case Start:
      if (pos > m_data.GetSize())
        return false;
      m_position = pos;
      break;

    case Current:
      if (pos < -m_position || pos > (m_data.GetSize() - m_position))
        return false;
      m_position += pos;
      break;

    case End:
      if (-pos > m_data.GetSize())
        return false;
      m_position = m_data.GetSize() - pos;
      break;
  }
  return true;
}


off_t PMemoryFile::GetPosition() const
{
  return m_position;
}


// End of File ///////////////////////////////////////////////////////////////

