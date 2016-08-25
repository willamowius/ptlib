/*
 * thread.h
 *
 * Thread of execution control class.
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

///////////////////////////////////////////////////////////////////////////////
// PThread

  public:
    int PXBlockOnChildTerminate(int pid, const PTimeInterval & timeout);

    int PXBlockOnIO(int handle,
                    int type,
                   const PTimeInterval & timeout);

    void PXAbortBlock() const;

#ifdef P_PTHREADS

  public:
#ifndef P_HAS_SEMAPHORES
    void PXSetWaitingSemaphore(PSemaphore * sem);
#endif

  protected:
    static void * PX_ThreadStart(void *);
    static void PX_ThreadEnd(void *);

    Priority          PX_priority;
#if defined(P_LINUX)
    mutable pid_t     PX_linuxId;
    PTimeInterval     PX_startTick;
    PTimeInterval     PX_endTick;
#endif
    pthread_mutex_t   PX_suspendMutex;
    int               PX_suspendCount;
    bool              PX_firstTimeStart;

#ifndef P_HAS_SEMAPHORES
    PSemaphore      * PX_waitingSemaphore;
    pthread_mutex_t   PX_WaitSemMutex;
#endif

    int unblockPipe[2];
    friend class PSocket;
    friend void PX_SuspendSignalHandler(int);

#elif defined(__BEOS__)

  protected:
    static int32 ThreadFunction(void * threadPtr);
    thread_id mId;
    int32 mPriority;
    PINDEX mStackSize;
    int32 mSuspendCount;
  public:
    int unblockPipe[2];

#elif defined(P_MAC_MPTHREADS)
  public:
    void PXSetWaitingSemaphore(PSemaphore * sem);
    static long PX_ThreadStart(void *);
    static void PX_ThreadEnd(void *);
    MPTaskID    PX_GetThreadId() const;

  protected:
    void PX_NewThread(PBoolean startSuspended);

    PINDEX     PX_origStackSize;
    int        PX_suspendCount;
    PSemaphore *suspend_semaphore;
    long       PX_signature;
    enum { kMPThreadSig = 'THRD', kMPDeadSig = 'DEAD'};

    MPTaskID   PX_threadId;
    MPSemaphoreID PX_suspendMutex;

    int unblockPipe[2];
    friend class PSocket;

#elif defined(VX_TASKS)
  public:
    SEM_ID syncPoint;
    static void Trace(PThreadIdentifer threadId = 0);

  private:
    static int ThreadFunction(void * threadPtr);
    long PX_threadId;
    int priority;
    PINDEX originalStackSize;

#endif


// End Of File ////////////////////////////////////////////////////////////////
