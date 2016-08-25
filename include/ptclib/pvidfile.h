/*
 * pvidfile.cxx
 *
 * Video file declaration
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

#ifndef PTLIB_PVIDFILE_H
#define PTLIB_PVIDFILE_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>

#if P_VIDEO
#if P_VIDFILE

#include <ptlib/videoio.h>


/**
 * Abstract class for a file containing a sequence of video frames
 */
class PVideoFile : public PVideoFrameInfo
{
  PCLASSINFO(PVideoFile, PVideoFrameInfo);
  protected:
    PVideoFile();

  public:
    virtual PBoolean Open(
      const PFilePath & name,    // Name of file to open.
      PFile::OpenMode mode = PFile::ReadWrite, // Mode in which to open the file.
      int opts = PFile::ModeDefault     // <code>OpenOptions</code> enum# for open operation.
    );

    virtual PBoolean IsOpen() const { return m_file.IsOpen(); }
    virtual PBoolean Close() { return m_file.Close(); }

    virtual PBoolean WriteFrame(const void * frame);
    virtual PBoolean ReadFrame(void * frame);

    virtual off_t GetLength() const;
    virtual PBoolean SetLength(
      off_t len   // New length of file in frames.
    );

    virtual off_t GetPosition() const;
    virtual PBoolean SetPosition(
      off_t pos,                                       ///< New position to set.
      PFile::FilePositionOrigin origin = PFile::Start  ///< Origin for position change.
    );

    virtual PBoolean SetFrameSize(
      unsigned width,   ///< New width of frame
      unsigned height   ///< New height of frame
    );
    virtual PBoolean SetFrameRate(
      unsigned rate  ///< Frames  per second
    );

    const PFilePath & GetFilePath() const { return m_file.GetFilePath(); }
    PINDEX GetFrameBytes() const { return m_frameBytes; }


  protected:
    bool   m_fixedFrameSize;
    bool   m_fixedFrameRate;
    PINDEX m_frameBytes;
    off_t  m_headerOffset;
    PFile  m_file;
};

/**
 * A file containing a sequence of raw YUV files in planar 4:2:0 format
 * Example files can be found at http://media.xiph.org/video/derf/
 */

class PYUVFile : public PVideoFile
{
  PCLASSINFO(PYUVFile, PVideoFile);
  public:
    PYUVFile();

    virtual PBoolean Open(
      const PFilePath & name,    // Name of file to open.
      PFile::OpenMode mode = PFile::ReadWrite, // Mode in which to open the file.
      int opts = PFile::ModeDefault     // <code>OpenOptions</code> enum# for open operation.
    );

    virtual PBoolean WriteFrame(const void * frame);
    virtual PBoolean ReadFrame(void * frame);

  protected:
    bool m_y4mMode;
};

PFACTORY_LOAD(PYUVFile);

#endif
#endif

#endif // PTLIB_PVIDFILE_H


// End Of File ///////////////////////////////////////////////////////////////
