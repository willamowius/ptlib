/*
 * file.h
 *
 * Operating System file I/O channel class.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */


#ifndef PTLIB_FILE_H
#define PTLIB_FILE_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _WIN32
#include <sys/stat.h>
#endif



///////////////////////////////////////////////////////////////////////////////
// Binary Files

/**This class represents a disk file. This is a particular type of I/O channel
   that has certain attributes. All platforms have a disk file, though exact
   details of naming convertions etc may be different.

   The basic model for files is that they are a named sequence of bytes that
   persists within a directory structure. The transfer of data to and from
   the file is made at a current position in the file. This may be set to
   random locations within the file.
 */
class PFile : public PChannel
{
  PCLASSINFO(PFile, PChannel);

  public:
  /**@name Construction */
  //@{
    /**Create a file object but do not open it. It does not initially have a
       valid file name. However, an attempt to open the file using the
       <code>Open()</code> function will generate a unique temporary file.
     */
    PFile();

    /**When a file is opened, it may restrict the access available to
       operations on the object instance. A value from this enum is passed to
       the <code>Open()</code> function to set the mode.
     */
    enum OpenMode {
      ReadOnly,   ///< File can be read but not written.
      WriteOnly,  ///< File can be written but not read.
      ReadWrite   ///< File can be both read and written.
    };

    /**When a file is opened, a number of options may be associated with the
       open file. These describe what action to take on opening the file and
       what to do on closure. A value from this enum is passed to the
       <code>Open()</code> function to set the options.

       The <code>ModeDefault</code> option will use the following values:
          \arg \c ReadOnly  <code>MustExist</code>
          \arg \c WriteOnly <code>Create | Truncate</code>
          \arg \c ReadWrite <code>Create</code>
     */
    enum OpenOptions {
      /// File options depend on the OpenMode parameter.
      ModeDefault = -1, 
      /// File open fails if file does not exist.
      MustExist = 0,    
      /// File is created if it does not exist.
      Create = 1,       
      /// File is set to zero length if it already exists.
      Truncate = 2,     
      /// File open fails if file already exists.
      Exclusive = 4,    
      /// File is temporary and is to be deleted when closed.
      Temporary = 8,
      /// File may not be read by another process.
      DenySharedRead = 16,
      /// File may not be written by another process.
      DenySharedWrite = 32
    };

    /**Create a unique temporary file name, and open the file in the specified
       mode and using the specified options. Note that opening a new, unique,
       temporary file name in ReadOnly mode will always fail. This would only
       be usefull in a mode and options that will create the file.

       The <code>PChannel::IsOpen()</code> function may be used after object
       construction to determine if the file was successfully opened.
     */
    PFile(
      OpenMode mode,          ///< Mode in which to open the file.
      int opts = ModeDefault  ///< <code>OpenOptions</code> enum# for open operation.
    );

    /**Create a file object with the specified name and open it in the
       specified mode and with the specified options.

       The <code>PChannel::IsOpen()</code> function may be used after object
       construction to determine if the file was successfully opened.
     */
    PFile(
      const PFilePath & name,    ///< Name of file to open.
      OpenMode mode = ReadWrite, ///< Mode in which to open the file.
      int opts = ModeDefault     ///< <code>OpenOptions</code> enum# for open operation.
    );

    /// Close the file on destruction.
    ~PFile();
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
    /**Get the platform and I/O channel type name of the channel. For example,
       it would return the filename in <code>PFile</code> type channels.

       @return
       the name of the channel.
     */
    virtual PString GetName() const;

    /**Low level read from the file channel. The read timeout is ignored for
       file I/O. The GetLastReadCount() function returns the actual number
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

    /**Low level write to the file channel. The write timeout is ignored for
       file I/O. The GetLastWriteCount() function returns the actual number
       of bytes written.

       The GetErrorCode() function should be consulted after Write() returns
       false to determine what caused the failure.

       @return true if at least len bytes were written to the channel.
     */
    virtual PBoolean Write(
      const void * buf, ///< Pointer to a block of memory to write.
      PINDEX len        ///< Number of bytes to write.
    );

    /** Close the file channel.
        @return true if close was OK.
      */
    virtual PBoolean Close();
  //@}


  /**@name File manipulation functions */
  //@{
    /**Check for file existance. 
       Determine if the file specified actually exists within the platforms
       file system.

       @return
       true if the file exists.
     */
    static PBoolean Exists(
      const PFilePath & name  ///< Name of file to see if exists.
    );

    /**Check for file existance.
       Determine if the file path specification associated with the instance
       of the object actually exists within the platforms file system.

       @return
       true if the file exists.
     */
    PBoolean Exists() const;

    /**Check for file access modes.
       Determine if the file specified may be opened in the specified mode. This would
       check the current access rights to the file for the mode. For example,
       for a file that is read only, using mode == ReadWrite would return
       false but mode == ReadOnly would return true.

       @return
       true if a file open would succeed.
     */
    static PBoolean Access(
      const PFilePath & name, ///< Name of file to have its access checked.
      OpenMode mode         ///< Mode in which the file open would be done.
    );

    /**Check for file access modes.
       Determine if the file path specification associated with the
       instance of the object may be opened in the specified mode. This would
       check the current access rights to the file for the mode. For example,
       for a file that is read only, using mode == ReadWrite would return
       false but mode == ReadOnly would return true.

       @return
       true if a file open would succeed.
     */
    PBoolean Access(
      OpenMode mode         ///< Mode in which the file open would be done.
    );

    /**Delete the specified file. If <code>force</code> is false and the file
       is protected against being deleted then the function fails. If
       <code>force</code> is true then the protection is ignored. What
       constitutes file deletion protection is platform dependent, eg on DOS
       is the Read Only attribute and on a Novell network it is a Delete
       trustee right. Some protection may not be able to overridden with the
       <code>force</code> parameter at all, eg on a Unix system and you are
       not the owner of the file.

       @return
       true if the file was deleted.
     */
    static PBoolean Remove(
      const PFilePath & name,   // Name of file to delete.
      PBoolean force = false      // Force deletion even if file is protected.
    );
    static PBoolean Remove(
      const PString & name,   // Name of file to delete.
      PBoolean force = false      // Force deletion even if file is protected.
    );

    /**Delete the current file. If <code>force</code> is false and the file
       is protected against being deleted then the function fails. If
       <code>force</code> is true then the protection is ignored. What
       constitutes file deletion protection is platform dependent, eg on DOS
       is the Read Only attribute and on a Novell network it is a Delete
       trustee right. Some protection may not be able to overridden with the
       <code>force</code> parameter at all, eg on a Unix system and you are
       not the owner of the file.

       @return
       true if the file was deleted.
     */
    PBoolean Remove(
      PBoolean force = false      // Force deletion even if file is protected.
    );

    /**Change the specified files name. This does not move the file in the
       directory hierarchy, it only changes the name of the directory entry.

       The <code>newname</code> parameter must consist only of the file name
       part, as returned by the <code>PFilePath::GetFileName()</code> function. Any
       other file path parts will cause an error.

       The first form uses the file path specification associated with the
       instance of the object. The name within the instance is changed to the
       new name if the function succeeds. The second static function uses an
       arbitrary file specified by name.

       @return
       true if the file was renamed.
     */
    static PBoolean Rename(
      const PFilePath & oldname,  ///< Old name of the file.
      const PString & newname,    ///< New name for the file.
      PBoolean force = false
        ///< Delete file if a destination exists with the same name.
    );

    /**Change the current files name.
       This does not move the file in the
       directory hierarchy, it only changes the name of the directory entry.

       The <code>newname</code> parameter must consist only of the file name
       part, as returned by the <code>PFilePath::GetFileName()</code> function. Any
       other file path parts will cause an error.

       The first form uses the file path specification associated with the
       instance of the object. The name within the instance is changed to the
       new name if the function succeeds. The second static function uses an
       arbitrary file specified by name.

       @return
       true if the file was renamed.
     */
    PBoolean Rename(
      const PString & newname,  ///< New name for the file.
      PBoolean force = false
        ///< Delete file if a destination exists with the same name.
    );

    /**Make a copy of the specified file.

       @return
       true if the file was renamed.
     */
    static PBoolean Copy(
      const PFilePath & oldname,  ///< Old name of the file.
      const PFilePath & newname,  ///< New name for the file.
      PBoolean force = false
        ///< Delete file if a destination exists with the same name.
    );

    /**Make a copy of the current file.

       @return
       true if the file was renamed.
     */
    PBoolean Copy(
      const PFilePath & newname,  ///< New name for the file.
      PBoolean force = false
        ///< Delete file if a destination exists with the same name.
    );

    /**Move the specified file. This will move the file from one position in
       the directory hierarchy to another position. The actual operation is
       platform dependent but  the reslt is the same. For instance, for Unix,
       if the move is within a file system then a simple rename is done, if
       it is across file systems then a copy and a delete is performed.

       @return
       true if the file was moved.
     */
    static PBoolean Move(
      const PFilePath & oldname,  ///< Old path and name of the file.
      const PFilePath & newname,  ///< New path and name for the file.
      PBoolean force = false
        ///< Delete file if a destination exists with the same name.
    );

    /**Move the current file. This will move the file from one position in
       the directory hierarchy to another position. The actual operation is
       platform dependent but  the reslt is the same. For instance, for Unix,
       if the move is within a file system then a simple rename is done, if
       it is across file systems then a copy and a delete is performed.

       @return
       true if the file was moved.
     */
    PBoolean Move(
      const PFilePath & newname,  ///< New path and name for the file.
      PBoolean force = false
        ///< Delete file if a destination exists with the same name.
    );
  //@}

  /**@name File channel functions */
  //@{
    /**Get the full path name of the file. The <code>PFilePath</code> object
       describes the full file name specification for the particular platform.

       @return
       the name of the file.
     */
    const PFilePath & GetFilePath() const;

    /**Set the full path name of the file. The <code>PFilePath</code> object
       describes the full file name specification for the particular platform.
     */
    void SetFilePath(
      const PString & path    ///< New file path.
    );


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
      
    /**Get the current size of the file.

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
      off_t len   // New length of file.
    );

    /// Options for the origin in setting the file position.
    enum FilePositionOrigin {
      /// Set position relative to start of file.
      Start = SEEK_SET,   
      /// Set position relative to current file position.
      Current = SEEK_CUR, 
      /// Set position relative to end of file.
      End = SEEK_END      
    };

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

    /**Determine if the current file position is at the end of the file. If
       this is true then any read operation will fail.

       @return
       true if at end of file.
     */
    PBoolean IsEndOfFile() const;
      
    /**Get information (eg protection, timestamps) on the specified file.

       @return
       true if the file info was retrieved.
     */
    static PBoolean GetInfo(
      const PFilePath & name,  // Name of file to get the information on.
      PFileInfo & info
      // <code>PFileInfo</code> structure to receive the information.
    );

    /**Get information (eg protection, timestamps) on the current file.

       @return
       true if the file info was retrieved.
     */
    PBoolean GetInfo(
      PFileInfo & info
      // <code>PFileInfo</code> structure to receive the information.
    );

    /**Set permissions on the specified file.

       @return
       true if the file was renamed.
     */
    static PBoolean SetPermissions(
      const PFilePath & name,   // Name of file to change the permission of.
      int permissions           // New permissions mask for the file.
    );
    /**Set permissions on the current file.

       @return
       true if the file was renamed.
     */
    PBoolean SetPermissions(
      int permissions           // New permissions mask for the file.
    );
  //@}

  protected:
    // Member variables

    PFilePath path;         ///< The fully qualified path name for the file.
    PBoolean removeOnClose; ///< File is to be removed when closed.


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/file.h"
#else
#include "unix/ptlib/file.h"
#endif
};


#endif // PTLIB_FILE_H


// End Of File ///////////////////////////////////////////////////////////////
