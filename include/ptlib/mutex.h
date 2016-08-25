/*
 * mutex.h
 *
 * Mutual exclusion thread synchonisation class.
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

#ifndef PTLIB_MUTEX_H
#define PTLIB_MUTEX_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/critsec.h>
#include <ptlib/semaphor.h>

/**This class defines a thread mutual exclusion object. A mutex is where a
   piece of code or data cannot be accessed by more than one thread at a time.
   To prevent this the PMutex is used in the following manner:
<pre><code>
      PMutex mutex;

      ...

      mutex.Wait();

      ... critical section - only one thread at a time here.

      mutex.Signal();

      ...
</code></pre>
    The first thread will pass through the <code>Wait()</code> function, a second
    thread will block on that function until the first calls the
    <code>Signal()</code> function, releasing the second thread.
 */

/*
 * On Windows, It is convenient for PTimedMutex to be an ancestor of PSemaphore
 * But that is the only platform where it is - every other platform (i.e. Unix)
 * uses different constructs for these objects, so there is no need for a PTimedMute
 * to carry around all of the PSemaphore members
 */

#ifdef _WIN32
class PTimedMutex : public PSemaphore
{
  PCLASSINFO(PTimedMutex, PSemaphore);
#else
class PTimedMutex : public PSync
{
  PCLASSINFO(PTimedMutex, PSync)
#endif

  public:
    /* Create a new mutex.
       Initially the mutex will not be "set", so the first call to Wait() will
       never wait.
     */
    PTimedMutex();
    PTimedMutex(const PTimedMutex & mutex);

    /** Try to enter the critical section for exlusive access. Does not wait.
        @return true if cirical section entered, leave/Signal must be called.
      */
    PINLINE bool Try() { return Wait(0); }

// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/mutex.h"
#else
#include "unix/ptlib/mutex.h"
#endif
};

// On Windows, critical sections are recursive and so we can use them for mutexes
// The only Posix mutex that is recursive is pthread_mutex, so we have to use that
#ifdef _WIN32
/** \class PMutex
    Synonym for PCriticalSection
  */
typedef PCriticalSection PMutex;
#else
/** \class PMutex
    Synonym for PTimedMutex
  */
typedef PTimedMutex PMutex;
#define	PCriticalSection PTimedMutex
#endif


#endif // PTLIB_MUTEX_H


// End Of File ///////////////////////////////////////////////////////////////
