/*
 * psync.h
 *
 * Abstract synchronisation semaphore class.
 *
 * Portable Tools Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
 * Copyright (c) 2005 Post Increment
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

#ifndef PTLIB_SYNC_H
#define PTLIB_SYNC_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/contain.h>
#include <ptlib/object.h>

class PSync : public PObject
{
  public:
  /**@name Operations */
  //@{
    /**Block until the synchronisation object is available.
     */
    virtual void Wait() = 0;

    /**Signal that the synchronisation object is available.
     */
    virtual void Signal() = 0;
  //@}
};

class PSyncNULL : public PSync
{
  public:
    virtual void Wait() { }
    virtual void Signal() { }
};

/**This class waits for the semaphore on construction and automatically
   signals the semaphore on destruction. Any descendent of PSemaphore
   may be used.

  This is very useful for constructs such as:
<pre><code>
    void func()
    {
      PWaitAndSignal mutexWait(myMutex);
      if (condition)
        return;
      do_something();
      if (other_condition)
        return;
      do_something_else();
    }
</code></pre>
 */

class PWaitAndSignal {
  public:
    /**Create the semaphore wait instance.
       This will wait on the specified semaphore using the Wait() function
       before returning.
      */
    inline PWaitAndSignal(
      const PSync & sem,   ///< Semaphore descendent to wait/signal.
      PBoolean wait = true    ///< Wait for semaphore before returning.
    ) : sync((PSync &)sem)
    { if (wait) sync.Wait(); }

    /** Signal the semaphore.
        This will execute the Signal() function on the semaphore that was used
        in the construction of this instance.
     */
    ~PWaitAndSignal()
    { sync.Signal(); }

  protected:
    PSync & sync;
};


#endif // PTLIB_SYNC_H


// End Of File ///////////////////////////////////////////////////////////////
