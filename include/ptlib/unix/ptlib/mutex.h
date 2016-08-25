/*
 * mutex.h
 *
 * Mutual exclusion thread synchronisation class.
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
 * basis, WITHOUT WARRANTY OF ANY KIND, eitF ANY KIND, either express or implied. See
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

///////////////////////////////////////////////////////////////////////////////
// PMutex

#if defined(P_PTHREADS) || defined(VX_TASKS)
  public:
    virtual ~PTimedMutex();
  protected:
    mutable pthread_mutex_t m_mutex;
    void Construct();
#endif

#if defined(P_PTHREADS) || defined(__BEOS__) || defined(P_MAC_MPTHREADS) || defined(VX_TASKS)
  public:
    virtual void Wait();
    virtual PBoolean Wait(const PTimeInterval & timeout);
    virtual void Signal();
    virtual PBoolean WillBlock() const;
#endif

#if !P_HAS_RECURSIVE_MUTEX
  protected:
     mutable PAtomicInteger m_lockCount;
     mutable pthread_t      m_lockerId;
#endif


// End Of File ////////////////////////////////////////////////////////////////
