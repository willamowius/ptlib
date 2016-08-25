/*
 * syncpoint.h
 *
 * Single thread synchronisation point (event) class.
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

#ifndef PTLIB_SYNCPOINT_H
#define PTLIB_SYNCPOINT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/semaphor.h>


/** This class defines a thread synchronisation object.
  This form of semaphore is used to indicate an <i>event</i> has occurred. A
  thread may block on the sync point and wait until another thread signals
  that it may continue. eg:
<pre><code>
    ... thread one
    while (condition) {
      sync.Wait();
      do_something();
    }

    ... thread 2
    do_something_else();
    sync.Signal();    // At this point thread 1 wake up and does something.
    do_yet_more();
</code></pre>

  Note that events are boolean in nature. If "thread one" is not waiting on the
  event, multiple calls to Signal() are ignored, thread one will only exit the
  Wait() call a single time once it gets there. Similarly, if multiple threads
  are waiting on the event, only one thread will be released. As you cannot know
  which thread will be released this mode of use is not recommended.
 */
class PSyncPoint : public PSemaphore
{
  PCLASSINFO(PSyncPoint, PSemaphore);

  public:
    /** Create a new sync point.
     */
    PSyncPoint();
    PSyncPoint(const PSyncPoint &);


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/syncpoint.h"
#else
#include "unix/ptlib/syncpoint.h"
#endif
};


#endif // PTLIB_SYNCPOINT_H


// End Of File ///////////////////////////////////////////////////////////////
