/*
 * dynalink.h
 *
 * Dynamic Link Library abstraction class.
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

#ifndef PTLIB_DYNALINK_H
#define PTLIB_DYNALINK_H

#if !defined(P_RTEMS)

#ifdef P_USE_PRAGMA
#pragma interface
#endif

/**A dynamic link library. This allows the loading at run time of code
   modules for use by an application.
   MacOS X/darwin supports plugins linked as object file image (linked with the -bundle arg to ld) or
   dynamic libraries (-dynamic).
   On all Unix platforms the file name should end in ".so". 
   On Windows the filename should end in ".dll"
*/

class PDynaLink : public PObject
{
  PCLASSINFO(PDynaLink, PObject);

  public:
  /**@name Construction */
  //@{
    /**Create a new dyna-link, loading the specified module. The first,
       parameterless, form does load a library.
     */
    PDynaLink();
    /**Create a new dyna-link, loading the specified module. The first,
       parameterless, form does load a library.
     */
    PDynaLink(
      const PString & name    ///< Name of the dynamically loadable module.
    );

    /**Destroy the dyna-link, freeing the module.
     */
    ~PDynaLink();
  //@}

  /**@name Load/Unload function */
  //@{
    /* Open a new dyna-link, loading the specified module.

       @return
       true if the library was loaded.
     */
    virtual PBoolean Open(
      const PString & name    ///< Name of the dynamically loadable module.
    );

    /**Close the dyna-link library.
     */
    virtual void Close();

    /**Dyna-link module is loaded and may be accessed.
     */
    virtual PBoolean IsLoaded() const;

    /**Get the name of the loaded library. If the library is not loaded
       this may return an empty string.

       If <code>full</code> is true then the full pathname of the library
       is returned otherwise only the name part is returned.

       @return
       String for the library name.
     */
    virtual PString GetName(
      PBoolean full = false  ///< Flag for full or short path name
    ) const;

    /**Get the extension used by this platform for dynamic link libraries.

       @return
       String for file extension.
     */
    static PString GetExtension();
  //@}

  /**@name DLL entry point functions */
  //@{
    /// Primitive pointer to a function for a dynamic link module.
    typedef void (*Function)();


    /**Get a pointer to the function in the dynamically loadable module.

       @return
       true if function was found.
     */
    PBoolean GetFunction(
      PINDEX index,    ///< Ordinal number of the function to get.
      Function & func  ///< Refrence to point to function to get.
    );

    /**Get a pointer to the function in the dynamically loadable module.

       @return
       true if function was found.
     */
    PBoolean GetFunction(
      const PString & name,  ///< Name of the function to get.
      Function & func        ///< Refrence to point to function to get.
    );

    ///< Return OS error code for last operation
    const PString & GetLastError() const { return m_lastError; }
  //@}

  protected:
    PString m_lastError;

// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/dynalink.h"
#else
#include "unix/ptlib/dynalink.h"
#endif
};

#endif // !defined(P_RTEMS)


#endif //PTLIB_DYNALINK_H


// End Of File ///////////////////////////////////////////////////////////////
