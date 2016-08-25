/*
 * sfile.h
 *
 * Structured file I/O channel class.
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

#ifndef PTLIB_STRUCTUREDFILE_H
#define PTLIB_STRUCTUREDFILE_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


/**A class representing a a structured file that is portable accross CPU
   architectures (as in the XDR protocol).
   
   This differs from object serialisation in that the access is always to a
   disk file and is random access. It would primarily be used for database
   type applications.
 */
class PStructuredFile : public PFile
{
  PCLASSINFO(PStructuredFile, PFile);

  private:
    PBoolean Read(void * buf, PINDEX len) { return PFile::Read(buf, len); }
    PBoolean Write(const void * buf, PINDEX len) { return PFile::Write(buf, len); }

  public:
  /**@name Construction */
  //@{
    /**Create a structured file object but do not open it. It does not
       initially have a valid file name. However, an attempt to open the file
       using the <code>PFile::Open()</code> function will generate a unique
       temporary file.
       
       The initial structure size is one byte.
     */
    PStructuredFile();

    /**Create a unique temporary file name, and open the file in the specified
       mode and using the specified options. Note that opening a new, unique,
       temporary file name in ReadOnly mode will always fail. This would only
       be usefull in a mode and options that will create the file.

       The <code>PChannel::IsOpen()</code> function may be used after object
       construction to determine if the file was successfully opened.
     */
    PStructuredFile(
      OpenMode mode,          ///< Mode in which to open the file.
      int opts = ModeDefault  ///< <code>OpenOptions</code> enum# for open operation.
    );
      
    /**Create a structured file object with the specified name and open it in
       the specified mode and with the specified options.

       The <code>PChannel::IsOpen()</code> function may be used after object
       construction to determine if the file was successfully opened.
     */
    PStructuredFile(
      const PFilePath & name,    ///< Name of file to open.
      OpenMode mode = ReadWrite, ///< Mode in which to open the file.
      int opts = ModeDefault     ///< <code>OpenOptions</code> enum# for open operation.
    );
  //@}

  /**@name Structured I/O functions */
  //@{
    /**Read a sequence of bytes into the specified buffer, translating the
       structure according to the specification made in the
       <code>SetStructure()</code> function.

       @return
       true if the structure was successfully read.
     */
    PBoolean Read(
      void * buffer   ///< Pointer to structure to receive data.
    );
      
    /**Write a sequence of bytes into the specified buffer, translating the
       structure according to the specification made in the
       <code>SetStructure()</code> function.

       @return
       true if the structure was successfully written.
     */
    PBoolean Write(
      const void * buffer   ///< Pointer to structure to write data from.
    );
  //@}

  /**@name Structure definition functions */
  //@{
    /**Get the size of each structure in the file.

       @return
       number of bytes in a structure.
     */
    PINDEX GetStructureSize() { return structureSize; }

    /// All element types in a structure
    enum ElementType {
      /// Element is a single character.
      Character,    
      /// Element is a 16 bit integer.
      Integer16,    
      /// Element is a 32 bit integer.
      Integer32,    
      /// Element is a 64 bit integer.
      Integer64,    
      /// Element is a 32 bit IEE floating point number.
      Float32,      
      /// Element is a 64 bit IEE floating point number.
      Float64,      
      /// Element is a 80 bit IEE floating point number.
      Float80,      
      NumElementTypes
    };

    /// Elements in the structure definition.
    struct Element {
      /// Type of element in structure.
      ElementType type;   
      /// Count of elements of this type.
      PINDEX      count;  
    };

    /** Set the structure of each record in the file. */
    void SetStructure(
      Element * structure,  ///< Array of structure elements
      PINDEX numElements    ///< Number of structure elements in structure.
    );
  //@}

  protected:
  // Member variables
    /// Number of bytes in structure.
    PINDEX structureSize;

    /// Array of elements in the structure.
    Element * structure;

    /// Number of elements in the array.
    PINDEX numElements;


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/sfile.h"
#else
#include "unix/ptlib/sfile.h"
#endif
};


#endif // PTLIB_STRUCTUREDFILE_H


// End Of File ///////////////////////////////////////////////////////////////
