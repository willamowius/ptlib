/*
 * filepath.h
 *
 * File system path string abstraction class.
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

#ifndef PTLIB_FILEPATH_H
#define PTLIB_FILEPATH_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#ifdef DOC_PLUS_PLUS
/** Base string type for a file path.
    For platforms where filenames are case significant (eg Unix) this class
    is a synonym for <code>PString</code>. If it is for a platform where case is not
    significant (eg Win32, Mac) then this is a synonym for <code>PCaselessString</code>.
 */
class PFilePathString : public PString { };
#endif


///////////////////////////////////////////////////////////////////////////////
// File Specification

/**This class describes a full description for a file on the particular
   platform. This will always uniquely identify the file on currently mounted
   volumes.

   An empty string for a PFilePath indicates an illegal path.

   The ancestor class is dependent on the platform. For file systems that are
   case sensitive, eg Unix, the ancestor is <code>PString</code>. For other
   platforms, the ancestor class is <code>PCaselessString</code>.
 */
class PFilePath : public PFilePathString
{
  PCLASSINFO(PFilePath, PFilePathString);

  public:
  /**@name Construction */
  //@{
    /**Create a file specification object.
     */
    PFilePath();

    /**Create a file specification object with the specified file name.
    
       The string passed in may be a full or partial specification for a file
       as determined by the platform. It is unusual for this to be a literal
       string, unless only the file title is specified, as that would be
       platform specific.

       The partial file specification is translated into a canonical form
       which always absolutely references the file.
     */
    PFilePath(
      const char * cstr   ///< Partial C string for file name.
    );

    /**Create a file specification object with the specified file name.
    
       The string passed in may be a full or partial specification for a file
       as determined by the platform. It is unusual for this to be a literal
       string, unless only the file title is specified, as that would be
       platform specific.

       The partial file specification is translated into a canonical form
       which always absolutely references the file.
     */
    PFilePath(
      const PString & str ///< Partial PString for file name.
    );

    /**Create a file specification object with the specified file name.
     */
    PFilePath(
      const PFilePath & path ///< Previous path for file name.
    );

    /**Create a file spec object with a generated temporary name. The first
       parameter is a prefix for the filename to which a unique number is
       appended. The second parameter is the directory in which the file is to
       be placed. If this is NULL a system standard directory is used.
     */
    PFilePath(
      const char * prefix,  ///< Prefix string for file title.
      const char * dir      ///< Directory in which to place the file.
    );

    /**Change the file specification object to the specified file name.
     */
    PFilePath & operator=(
      const PFilePath & path ///< Previous path for file name.
    );
    /**Change the file specification object to the specified file name.

       The string passed in may be a full or partial specifiaction for a file
       as determined by the platform. It is unusual for this to be a literal
       string, unless only the file title is specified, as that would be
       platform specific.

       The partial file specification is translated into a canonical form
       which always absolutely references the file.
     */
    PFilePath & operator=(
      const PString & str ///< Partial PString for file name.
    );
    /**Change the file specification object to the specified file name.

       The string passed in may be a full or partial specifiaction for a file
       as determined by the platform. It is unusual for this to be a literal
       string, unless only the file title is specified, as that would be
       platform specific.

       The partial file specification is translated into a canonical form
       which always absolutely references the file.
     */
    PFilePath & operator=(
      const char * cstr ///< Partial "C" string for file name.
    );
  //@}

  /**@name Path addition functions */
  //@{
    /**Concatenate a string to the file path, modifiying that path.

       @return
       reference to string that was concatenated to.
     */
    PFilePath & operator+=(
      const PString & str   ///< String to concatenate.
    );

    /**Concatenate a C string to a path, modifiying that path. The
       <code>cstr</code> parameter is typically a literal string, eg:
<pre><code>
        myStr += "fred";
</code></pre>

       @return
       reference to string that was concatenated to.
     */
    PFilePath & operator+=(
      const char * cstr  ///< C string to concatenate.
    );

    /**Concatenate a single character to a path. The <code>ch</code>
       parameter is typically a literal, eg:
<pre><code>
        myStr += '!';
</code></pre>

       @return
       new string with concatenation of the object and parameter.
     */
    PFilePath & operator+=(
      char ch   // Character to concatenate.
    );
  //@}

  /**@name Path decoding access functions */
  //@{
    /**Get the drive/volume name component of the full file specification. This
       is very platform specific. For example in DOS & NT it is the drive
       letter followed by a colon ("C:"), for Macintosh it is the volume name
       ("Untitled") and for Unix it is empty ("").
       
       @return
       string for the volume name part of the file specification..
     */
    PFilePathString GetVolume() const;
      
    /**Get the directory path component of the full file specification. This
       will include leading and trailing directory separators. For example
       on DOS this could be "\SRC\PWLIB\", for Macintosh ":Source:PwLib:" and
       for Unix "/users/equivalence/src/pwlib/".

       @return
       string for the path part of the file specification.
     */
    PFilePathString GetPath() const;

    /**Get the title component of the full file specification, eg for the DOS
       file "C:\SRC\PWLIB\FRED.DAT" this would be "FRED".

       @return
       string for the title part of the file specification.
     */
    PFilePathString GetTitle() const;

    /**Get the file type of the file. Note that on some platforms this may
       actually be part of the full name string. eg for DOS file
       "C:\SRC\PWLIB\FRED.TXT" this would be ".TXT" but on the Macintosh this
       might be "TEXT".

       Note there are standard translations from file extensions, eg ".TXT"
       and some Macintosh file types, eg "TEXT".

       @return
       string for the type part of the file specification.
     */
    PFilePathString GetType() const;

    /**Get the actual directory entry name component of the full file
       specification. This may be identical to
       <code>GetTitle() + GetType()</code> or simply <code>GetTitle()</code>
       depending on the platform. eg for DOS file "C:\SRC\PWLIB\FRED.TXT" this
       would be "FRED.TXT".

       @return
       string for the file name part of the file specification.
     */
    PFilePathString GetFileName() const;

    /**Get the the directory that the file is contained in.  This may be 
       identical to <code>GetVolume() + GetPath()</code> depending on the 
       platform. eg for DOS file "C:\SRC\PWLIB\FRED.TXT" this would be 
       "C:\SRC\PWLIB\".

       Note that for Unix platforms, this returns the {\b physical} path
       of the directory. That is all symlinks are resolved. Thus the directory
       returned may not be the same as the value of <code>GetPath()</code>.

       @return
       Directory that the file is contained in.
     */
    PDirectory GetDirectory() const;

    /**Set the type component of the full file specification, eg for the DOS
       file "C:\SRC\PWLIB\FRED.DAT" would become "C:\SRC\PWLIB\FRED.TXT".
     */
    void SetType(
      const PFilePathString & type  ///< New type of the file.
    );
  //@}

  /**@name Miscellaneous functions */
  //@{
    /**Test if the character is valid in a filename.

       @return
       true if the character is valid for a filename.
     */
    static PBoolean IsValid(
      char c    ///< Character to test for validity.
    );

    /**Test if all the characters are valid in a filename.

       @return
       true if the character is valid for a filename.
     */
    static PBoolean IsValid(
      const PString & str   ///< String to test for validity.
    );

    /**Test if path is an absolute path or relative path.
      */
    static bool IsAbsolutePath(
      const PString & path   ///< path name
    );
  //@}


  protected:
    virtual void AssignContents(const PContainer & cont);


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/filepath.h"
#else
#include "unix/ptlib/filepath.h"
#endif
};


#endif // PTLIB_FILEPATH_H


// End Of File ///////////////////////////////////////////////////////////////
