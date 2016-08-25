/*
 * memfile.h
 *
 * WAV file I/O channel class.
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
 * The Initial Developer of the Original Code is
 * Equivalence Pty Ltd
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_PMEMFILE_H
#define PTLIB_PMEMFILE_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


/**This class is used to allow a block of memory to substitute for a disk file.
 */
class PMemoryFile : public PFile
{
  PCLASSINFO(PMemoryFile, PFile);
  public:
  /**@name Construction */
  //@{
    /**Create a new, empty, memory file.
      */
    PMemoryFile();

    /**Create a new memory file initialising to the specified content.
      */
    PMemoryFile(
      const PBYTEArray & data  ///< New content filr memory file.
    );

    /**Destroy the memory file
      */
    ~PMemoryFile();
  //@}


  /**@name Overrides from class PObject */
  //@{
    /**Determine the relative rank of the two objects. This is essentially the
       string comparison of the <code>PFilePath</code> names of the files.

       @return
       relative rank of the file paths.
     */
    Comparison Compare(
      const PObject & obj   ///< Other file to compare against.
    ) const;
  //@}


  /**@name Overrides from class PChannel */
  //@{
    /**Open the current file in the specified mode and with
       the specified options. If the file object already has an open file then
       it is closed.
       
       If there has not been a filename attached to the file object (via
       <code>SetFilePath()</code>, the <code>name</code> parameter or a previous
       open) then a new unique temporary filename is generated.

       @return
       true if the file was successfully opened.
     */
    virtual PBoolean Open(
      OpenMode mode = ReadWrite,  // Mode in which to open the file.
      int opts = ModeDefault      // Options for open operation.
    );

    /**Open the specified file name in the specified mode and with
       the specified options. If the file object already has an open file then
       it is closed.
       
       Note: if <code>mode</code> is StandardInput, StandardOutput or StandardError,
       then the <code>name</code> parameter is ignored.

       @return
       true if the file was successfully opened.
     */
    virtual PBoolean Open(
      const PFilePath & name,    // Name of file to open.
      OpenMode mode = ReadWrite, // Mode in which to open the file.
      int opts = ModeDefault     // <code>OpenOptions</code> enum# for open operation.
    );
      
    /** Close the channel, shutting down the link to the data source.

       @return true if the channel successfully closed.
     */
    virtual PBoolean Close();

    /**Low level read from the memory file channel. The read timeout is
       ignored.  The GetLastReadCount() function returns the actual number
       of bytes read.

       The GetErrorCode() function should be consulted after Read() returns
       false to determine what caused the failure.

       @return
       true indicates that at least one character was read from the channel.
       false means no bytes were read due to timeout or some other I/O error.
     */
    virtual PBoolean Read(
      void * buf,   ///< Pointer to a block of memory to receive the read bytes.
      PINDEX len    ///< Maximum number of bytes to read into the buffer.
    );

    /**Low level write to the memory file channel. The write timeout is
       ignored. The GetLastWriteCount() function returns the actual number
       of bytes written.

       The GetErrorCode() function should be consulted after Write() returns
       false to determine what caused the failure.

       @return true if at least len bytes were written to the channel.
     */
    virtual PBoolean Write(
      const void * buf, ///< Pointer to a block of memory to write.
      PINDEX len        ///< Number of bytes to write.
    );
  //@}


  /**@name Overrides from class PFile */
  //@{
    /**Get the current size of the file.
       The size of the file corresponds to the size of the data array.

       @return
       length of file in bytes.
     */
    virtual off_t GetLength() const;
      
    /**Set the size of the file, padding with 0 bytes if it would require
       expanding the file, or truncating it if being made shorter.

       @return
       true if the file size was changed to the length specified.
     */
    virtual PBoolean SetLength(
      off_t len   ///< New length of file.
    );

    /**Set the current active position in the file for the next read or write
       operation. The <code>pos</code> variable is a signed number which is
       added to the specified origin. For <code>origin == PFile::Start</code>
       only positive values for <code>pos</code> are meaningful. For
       <code>origin == PFile::End</code> only negative values for
       <code>pos</code> are meaningful.

       @return
       true if the new file position was set.
     */
    virtual PBoolean SetPosition(
      off_t pos,                         ///< New position to set.
      FilePositionOrigin origin = Start  ///< Origin for position change.
    );

    /**Get the current active position in the file for the next read or write
       operation.

       @return
       current file position relative to start of file.
     */
    virtual off_t GetPosition() const;
  //@}


  /**@name Overrides from class PFile */
  //@{
    /**Get the memory data the file has operated with.
      */
    const PBYTEArray & GetData() const { return m_data; }
  //@}


  protected:
    PBYTEArray m_data;
    off_t      m_position;
};


#endif // PTLIB_PMEMFILE_H


// End of File ///////////////////////////////////////////////////////////////
