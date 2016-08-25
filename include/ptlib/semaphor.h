/*
 * semaphor.h
 *
 * Thread synchronisation semaphore class.
 *
 * Portable Tools Library
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

#ifndef PTLIB_SEMAPHORE_H
#define PTLIB_SEMAPHORE_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/psync.h>
#include <limits.h>
#include <ptlib/critsec.h>

/**This class defines a thread synchronisation object. This is in the form of a
   integer semaphore. The semaphore has a count and a maximum value. The
   various combinations of count and usage of the Wait() and
   Signal() functions determine the type of synchronisation mechanism
   to be employed.

   The Wait() operation is that if the semaphore count is > 0,
   decrement the semaphore and return. If it is = 0 then wait (block).

   The Signal() operation is that if there are waiting threads then
   unblock the first one that was blocked. If no waiting threads and the count
   is less than the maximum then increment the semaphore.

   The most common is to create a mutual exclusion zone. A mutex is where a
   piece of code or data cannot be accessed by more than one thread at a time.
   To prevent this the PSemaphore is used in the following manner:
<pre><code>
      PSemaphore mutex(1, 1);  // Maximum value of 1 and initial value of 1.

      ...

      mutex.Wait();

      ... critical section - only one thread at a time here.

      mutex.Signal();

      ...
</code></pre>
    The first thread will pass through the Wait() function, a second
    thread will block on that function until the first calls the
    Signal() function, releasing the second thread.
 */
class PSemaphore : public PSync
{
  PCLASSINFO(PSemaphore, PSync);

  public:
  /**@name Construction */
  //@{
    /**Create a new semaphore with maximum count and initial value specified.
       If the initial value is larger than the maximum value then is is set to
       the maximum value.
     */
    PSemaphore(
      unsigned initial, ///< Initial value for semaphore count.
      unsigned maximum  ///< Maximum value for semaphore count.
    );

    /** Create a new semaphore with the same initial and maximum values as the original.
     */
    PSemaphore(const PSemaphore &);

    /**Destroy the semaphore. This will assert if there are still waiting
       threads on the semaphore.
     */
    ~PSemaphore();
  //@}

  /**@name Operations */
  //@{
    /**If the semaphore count is > 0, decrement the semaphore and return. If
       if is = 0 then wait (block).
     */
    virtual void Wait();

    /**If the semaphore count is > 0, decrement the semaphore and return. If
       if is = 0 then wait (block) for the specified amount of time.

       @return
       true if semaphore was signalled, false if timed out.
     */
    virtual PBoolean Wait(
      const PTimeInterval & timeout // Amount of time to wait for semaphore.
    );

    /**If there are waiting (blocked) threads then unblock the first one that
       was blocked. If no waiting threads and the count is less than the
       maximum then increment the semaphore.
     */
    virtual void Signal();

    /**Determine if the semaphore would block if the Wait() function
       were called.

       @return
       true if semaphore will block when Wait() is called.
     */
    virtual PBoolean WillBlock() const;
  //@}

  private:
    PSemaphore & operator=(const PSemaphore &) { return *this; }


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/semaphor.h"
#else
#include "unix/ptlib/semaphor.h"
#endif
};


#endif // PTLIB_SEMAPHORE_H


// End Of File ///////////////////////////////////////////////////////////////
