/*
 * svcproc.h
 *
 * Service Process (daemon) class.
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

#ifndef PTLIB_SERVICEPROCESS_H
#define PTLIB_SERVICEPROCESS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/pprocess.h>
#include <ptlib/syslog.h>



/** A process type that runs as a "background" service.
    This may be a service under the Windows NT operating system, or a "daemon" under Unix, or a hidden application under Windows.
 */
class PServiceProcess : public PProcess
{
  PCLASSINFO(PServiceProcess, PProcess);

  public:
  /**@name Construction */
  //@{
    /** Create a new service process.
     */
    PServiceProcess(
      const char * manuf,   ///< Name of manufacturer
      const char * name,    ///< Name of product
      WORD majorVersion,    ///< Major version number of the product
      WORD minorVersion,    ///< Minor version number of the product
      CodeStatus status,    ///< Development status of the product
      WORD buildNumber      ///< Build number of the product
    );
  //@}

  /**@name Callback functions */
  //@{
    /** Called when the service is started. This typically initialises the
       service and returns true if the service is ready to run. The
       <code>Main()</code> function is then executed.

       @return
       true if service may start, false if an initialisation failure occurred.
     */
    virtual PBoolean OnStart() = 0;

    /** Called by the system when the service is stopped. One return from this
       function there is no guarentee that any more user code will be executed.
       Any cleaning up or closing of resource must be done in here.
     */
    virtual void OnStop();

    /** Called by the system when the service is to be paused. This will
       suspend any actions that the service may be executing. Usually this is
       less expensive in resource allocation etc than stopping and starting
       the service.

       @return
       true if the service was successfully paused.
     */
    virtual PBoolean OnPause();

    /** Resume after the service was paused.
     */
    virtual void OnContinue();

    /** The Control menu option was used in the SysTray menu.
     */
    virtual void OnControl() = 0;
  //@}

  /**@name Miscellaneous functions */
  //@{
    /** Get the current service process object.

       @return
       Pointer to service process.
     */
    static PServiceProcess & Current();


    /** Set the level at which errors are logged. Only messages higher than or
       equal to the specified level will be logged.
    
       The default is PSystemLog::Error allowing fatal errors and ordinary\
       errors to be logged and warning and information to be ignored.

       If in debug mode then the default is PSystemLog::Info allowing all
       messages to be displayed.
     */
    void SetLogLevel(
      PSystemLog::Level level  ///< New log level
    ) { PSystemLog::GetTarget().SetThresholdLevel(level); }

    /** Get the current level for logging.

       @return
       Log level.
     */
    PSystemLog::Level GetLogLevel() const { return PSystemLog::GetTarget().GetThresholdLevel(); }
  //@}


    /* Internal initialisation function called directly from
       <code>main()</code>. The user should never call this function.
     */
    virtual int InternalMain(void * arg = NULL);


  protected:
  // Member variables
    /// Flag to indicate service is run in simulation mode.
    PBoolean debugMode;

// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/svcproc.h"
#else
#include "unix/ptlib/svcproc.h"
#endif
};


#endif // PTLIB_SERVICEPROCESS_H


// End Of File ///////////////////////////////////////////////////////////////
