/*
 * tlibthrd.cxx
 *
 * Routines for pre-emptive threading system
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

#include <ptlib/socket.h>
#include <sched.h>
#include <pthread.h>
#include <sys/resource.h>

#ifdef P_RTEMS
#define SUSPEND_SIG SIGALRM
#include <sched.h>
#else
#define SUSPEND_SIG SIGVTALRM
#endif

#ifdef P_MACOSX
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <sys/param.h>
#include <sys/sysctl.h>
// going to need the main thread for adjusting relative priority
static pthread_t baseThread;
#elif defined(P_LINUX)
#include <sys/syscall.h>
#endif

#ifdef P_HAS_SEMAPHORES_XPG6
#include "semaphore.h"
#endif

int PX_NewHandle(const char *, int);

#define PPThreadKill(id, sig)  PProcess::Current().PThreadKill(id, sig)


#define PAssertPTHREAD(func, args) \
  { \
    unsigned threadOpRetry = 0; \
    while (PAssertThreadOp(func args, threadOpRetry, #func, __FILE__, __LINE__)); \
  }

static PBoolean PAssertThreadOp(int retval,
                            unsigned & retry,
                            const char * funcname,
                            const char * file,
                            unsigned line)
{
  if (retval == 0) {
    PTRACE_IF(2, retry > 0, "PTLib\t" << funcname << " required " << retry << " retries!");
    return false;
  }

  if (errno == EINTR || errno == EAGAIN) {
    if (++retry < 1000) {
#if defined(P_RTEMS)
      sched_yield();
#else
      usleep(10000); // Basically just swap out thread to try and clear blockage
#endif
      return true;   // Return value to try again
    }
    // Give up and assert
  }

#if defined(_DEBUG) && defined( P_USE_ASSERTS)
  PAssertFunc(file, line, NULL, psprintf("Function %s failed", funcname));
#else
  PTRACE(1, psprintf("Function %s failed", funcname));
#endif
  return false;
}


#if defined(P_LINUX)
static int GetSchedParam(PThread::Priority priority, sched_param & param)
{
  /*
    Set realtime scheduling if our effective user id is root (only then is this
    allowed) AND our priority is Highest.
      I don't know if other UNIX OSs have SCHED_FIFO and SCHED_RR as well.

    WARNING: a misbehaving thread (one that never blocks) started with Highest
    priority can hang the entire machine. That is why root permission is 
    neccessary.
  */

  memset(&param, 0, sizeof(sched_param));

  switch (priority) {
    case PThread::HighestPriority :
      param.sched_priority = sched_get_priority_max(SCHED_RR);
      break;

    case PThread::HighPriority :
      param.sched_priority = sched_get_priority_min(SCHED_RR);
      break;

#ifdef SCHED_BATCH
    case PThread::LowestPriority :
    case PThread::LowPriority :
      return SCHED_BATCH;
#endif

    default : // PThread::NormalPriority :
      return SCHED_OTHER;
  }

  if (geteuid() == 0)
    return SCHED_RR;

  param.sched_priority = 0;
  PTRACE(2, "PTLib\tNo permission to set priority level " << priority);
  return SCHED_OTHER;
}
#endif


PDECLARE_CLASS(PHouseKeepingThread, PThread)
  public:
    PHouseKeepingThread()
      : PThread(1000, NoAutoDeleteThread, HighestPriority, "Housekeeper")
      { closing = false; Resume(); }

    void Main();
    void SetClosing() { closing = true; }

  protected:
    PBoolean closing;
};


static pthread_mutex_t MutexInitialiser = PTHREAD_MUTEX_INITIALIZER;


#define new PNEW


void PHouseKeepingThread::Main()
{
  PProcess & process = PProcess::Current();

  while (!closing) {
    PTimeInterval delay = process.timers.Process();
    if (delay > 10000)
      delay = 10000;

    process.breakBlock.Wait(delay);

    process.m_activeThreadMutex.Wait();
    PBoolean found;
    do {
      found = false;
      for (PProcess::ThreadMap::iterator it = process.m_activeThreads.begin(); it != process.m_activeThreads.end(); ++it) {
        PThread * thread = it->second;
        if ((thread != NULL) && thread->IsAutoDelete() && thread->IsTerminated()) {
          process.m_activeThreads.erase(it);

          // unlock the m_activeThreadMutex to avoid deadlocks:
          // if somewhere in the destructor a call to PTRACE() is made,
          // which itself calls PThread::Current(), deadlocks are possible
          thread->m_threadId = 0;
          process.m_activeThreadMutex.Signal();
          delete thread;
          process.m_activeThreadMutex.Wait();

          found = true;
          break;
        }
      }
    } while (found);
    process.m_activeThreadMutex.Signal();

    process.PXCheckSignals();
  }

  PTRACE(5, "Housekeeping thread ended");
}


bool PProcess::SignalTimerChange()
{
  if (!PAssert(IsInitialised(), PLogicError) || m_shuttingDown) 
    return false;

  PWaitAndSignal m(housekeepingMutex);
  if (housekeepingThread == NULL) {
#if PMEMORY_CHECK
    PBoolean oldIgnoreAllocations = PMemoryHeap::SetIgnoreAllocations(true);
#endif
    housekeepingThread = new PHouseKeepingThread;
#if PMEMORY_CHECK
    PMemoryHeap::SetIgnoreAllocations(oldIgnoreAllocations);
#endif
  }

  breakBlock.Signal();
  return true;
}


void PProcess::Construct()
{
#ifndef P_RTEMS
  // get the file descriptor limit
  struct rlimit rl;
  PAssertOS(getrlimit(RLIMIT_NOFILE, &rl) == 0);
  maxHandles = rl.rlim_cur;
  PTRACE(4, "PTLib\tMaximum per-process file handles is " << maxHandles);
#else
  maxHandles = 500; // arbitrary value
#endif

  // initialise the housekeeping thread
  housekeepingThread = NULL;

#ifdef P_MACOSX
  // records the main thread for priority adjusting
  baseThread = pthread_self();
#endif

  CommonConstruct();
}


PBoolean PProcess::SetMaxHandles(int newMax)
{
#ifndef P_RTEMS
  // get the current process limit
  struct rlimit rl;
  PAssertOS(getrlimit(RLIMIT_NOFILE, &rl) == 0);

  // set the new current limit
  rl.rlim_cur = newMax;
  if (setrlimit(RLIMIT_NOFILE, &rl) == 0) {
    PAssertOS(getrlimit(RLIMIT_NOFILE, &rl) == 0);
    maxHandles = rl.rlim_cur;
    if (maxHandles == newMax) {
      PTRACE(2, "PTLib\tNew maximum per-process file handles set to " << maxHandles);
      return true;
    }
  }
#endif // !P_RTEMS

  PTRACE(1, "PTLib\tCannot set per-process file handle limit to "
         << newMax << " (is " << maxHandles << ") - check permissions");
  return false;
}


PProcess::~PProcess()
{
  PreShutdown();

  // Don't wait for housekeeper to stop if Terminate() is called from it.
  {
    PWaitAndSignal m(housekeepingMutex);
    if ((housekeepingThread != NULL) && (PThread::Current() != housekeepingThread)) {
      housekeepingThread->SetClosing();
      SignalTimerChange();
      housekeepingThread->WaitForTermination();
      delete housekeepingThread;
    }
  }

  CommonDestruct();

  PostShutdown();
}

PBoolean PProcess::PThreadKill(pthread_t id, unsigned sig)
{
  PWaitAndSignal m(m_activeThreadMutex);

  if (m_activeThreads.find(id) == m_activeThreads.end()) 
    return false;

  return pthread_kill(id, sig) == 0;
}

void PProcess::PXSetThread(pthread_t id, PThread * thread)
{
  PThread * currentThread = NULL;

  m_activeThreadMutex.Wait();

  ThreadMap::iterator it = m_activeThreads.find(id);
  if (it != m_activeThreads.end() && it->second->IsAutoDelete())
    currentThread = it->second;

  m_activeThreads[id] = thread;

  m_activeThreadMutex.Signal();

  if (currentThread != NULL) 
    delete currentThread;
}

//////////////////////////////////////////////////////////////////////////////

//
//  Called to construct a PThread for either:
//
//       a) The primordial PProcesss thread
//       b) A non-PTLib thread that needs to use PTLib routines, such as PTRACE
//
//  This is always called in the context of the running thread, so naturally, the thread
//  is not paused
//

PThread::PThread(bool isProcess)
  : m_isProcess(isProcess)
  , m_autoDelete(!isProcess)
  , m_originalStackSize(0) // 0 indicates external thread
  , m_threadId(pthread_self())
  , PX_priority(NormalPriority)
#if defined(P_LINUX)
  , PX_linuxId(syscall(SYS_gettid))
#endif
  , PX_suspendMutex(MutexInitialiser)
  , PX_suspendCount(0)
  , PX_firstTimeStart(false)
#ifndef P_HAS_SEMAPHORES
  , PX_waitingSemaphore(NULL)
  , PX_WaitSemMutex(MutexInitialiser)
#endif
{
#ifdef P_RTEMS
  PAssertOS(socketpair(AF_INET,SOCK_STREAM,0,unblockPipe) == 0);
#else
  PAssertOS(::pipe(unblockPipe) == 0);
#endif

  if (isProcess)
    return;

  PProcess & process = PProcess::Current();
  process.PXSetThread(m_threadId, this);
  process.SignalTimerChange();
}


//
//  Called to construct a PThread for a normal PTLib thread.
//
//  This is always called in the context of some other thread, and
//  the PThread is always created in the paused state
//
PThread::PThread(PINDEX stackSize,
                 AutoDeleteFlag deletion,
                 Priority priorityLevel,
                 const PString & name)
  : m_isProcess(false)
  , m_autoDelete(deletion == AutoDeleteThread)
  , m_originalStackSize(stackSize) // 0 indicates PTLib created thread
  , m_threadName(name)
  , m_threadId(0)  // 0 indicates thread has not started
  , PX_priority(priorityLevel)
#if defined(P_LINUX)
  , PX_linuxId(0)
#endif
  , PX_suspendMutex(MutexInitialiser)
  , PX_suspendCount(1)
  , PX_firstTimeStart(true) // new thread is actually started the first time Resume() is called.
#ifndef P_HAS_SEMAPHORES
  , PX_waitingSemaphore(NULL)
  , PX_WaitSemMutex(MutexInitialiser)
#endif
{
  PAssert(stackSize > 0, PInvalidParameter);

#ifdef P_RTEMS
  PAssertOS(socketpair(AF_INET,SOCK_STREAM,0,unblockPipe) == 0);
#else
  PAssertOS(::pipe(unblockPipe) == 0);
#endif
  PX_NewHandle("Thread unblock pipe", PMAX(unblockPipe[0], unblockPipe[1]));

  PTRACE(3, "PTLib\tCreated thread " << this << ' ' << m_threadName);
}

//
//  Called to destruct a PThread
//
//  If not called in the context of the thread being destroyed, we need to wait
//  for that thread to stop before continuing
//

PThread::~PThread()
{
  if (PProcessInstance == NULL) {
#if PTRACING
    PTrace::Cleanup();
#endif
  } else {
    pthread_t id = m_threadId;
    PProcess & process = PProcess::Current();

    // need to terminate the thread if it was ever started and it is not us
    if ((id != 0) && (id != pthread_self()))
      Terminate();

    // cause the housekeeping thread to be created, if not already running
    process.SignalTimerChange();

    // last gasp tracing
    PTRACE(5, "PTLib\tDestroyed thread " << this << ' ' << m_threadName << "(id = " << ::hex << id << ::dec << ")");


    // if thread was started, remove it from the active thread list and detach it to release thread resources
    if (id != 0) {
      process.m_activeThreadMutex.Wait();
      if (m_originalStackSize != 0)
        pthread_detach(id);
      process.m_activeThreads.erase(id);
      process.m_activeThreadMutex.Signal();
    }

    // cause the housekeeping thread to wake up (we know it must be running)
    process.SignalTimerChange();
  }

  // close I/O unblock pipes
  ::close(unblockPipe[0]);
  ::close(unblockPipe[1]);

#ifndef P_HAS_SEMAPHORES
  pthread_mutex_destroy(&PX_WaitSemMutex);
#endif

  // If the mutex was not locked, the unlock will fail */
  pthread_mutex_trylock(&PX_suspendMutex);
  pthread_mutex_unlock(&PX_suspendMutex);
  pthread_mutex_destroy(&PX_suspendMutex);
}


void * PThread::PX_ThreadStart(void * arg)
{ 
  PThread * thread = (PThread *)arg;
  // Added this to guarantee that the thread creation (PThread::Restart)
  // has completed before we start the thread. Then the m_threadId has
  // been set.
  pthread_mutex_lock(&thread->PX_suspendMutex);
  thread->SetThreadName(thread->GetThreadName());
#if defined(P_LINUX)
  thread->PX_linuxId = syscall(SYS_gettid);
  thread->PX_startTick = PTimer::Tick();
#endif
  pthread_mutex_unlock(&thread->PX_suspendMutex);

  // make sure the cleanup routine is called when the thread exits
  //pthread_cleanup_push(&PThread::PX_ThreadEnd, arg);

#if defined(P_LINUX)
  PTRACE(5, "PTLib\tStarted thread " << thread << " (" << thread->PX_linuxId << ") " << thread->GetThreadName());
#else
  PTRACE(5, "PTLib\tStarted thread " << thread << ' ' << thread->GetThreadName());
#endif

  PProcess::Current().OnThreadStart(*thread);

  // now call the the thread main routine
  thread->Main();

  // execute the cleanup routine
  //pthread_cleanup_pop(1);
  PX_ThreadEnd(arg);

  // clean up tracing 
#if PTRACING
  PTrace::Cleanup();
#endif

  // Inform the helgrind finite state machine that this thread has finished
  // Commented out as on some platforms it causes a crash, no idea why!
  // pthread_exit(0);

  return NULL;
}


void PThread::PX_ThreadEnd(void * arg)
{
  PThread * thread = (PThread *)arg;
  PProcess & process = PProcess::Current();

#if defined(P_LINUX)
  thread->PX_endTick = PTimer::Tick();
#endif

  process.OnThreadEnded(*thread);
}


void PThread::Restart()
{
  if (!IsTerminated())
    return;

  pthread_attr_t threadAttr;
  pthread_attr_init(&threadAttr);

#if defined(P_LINUX)

  // Set a decent (256K) stack size that won't eat all virtual memory
  pthread_attr_setstacksize(&threadAttr, 16*PTHREAD_STACK_MIN);

  struct sched_param sched_params;
  PAssertPTHREAD(pthread_attr_setschedpolicy, (&threadAttr, GetSchedParam(PX_priority, sched_params)));
  PAssertPTHREAD(pthread_attr_setschedparam,  (&threadAttr, &sched_params));

#elif defined(P_RTEMS)
  pthread_attr_setstacksize(&threadAttr, 2*PTHREAD_MINIMUM_STACK_SIZE);
  pthread_attr_setinheritsched(&threadAttr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&threadAttr, SCHED_OTHER);
  struct sched_param sched_param;
  sched_param.sched_priority = 125; /* set medium priority */
  pthread_attr_setschedparam(&threadAttr, &sched_param);
#endif

  PProcess & process = PProcess::Current();

  // lock the thread list
  process.m_activeThreadMutex.Wait();

  // create the thread
  PAssertPTHREAD(pthread_create, (&m_threadId, &threadAttr, PX_ThreadStart, this));

  // put the thread into the thread list
  process.PXSetThread(m_threadId, this);

  // Inside process.m_activeThreadMutex so simple static is OK
  size_t newHighWaterMark = 0;
  static size_t highWaterMark = 0;
  if (process.m_activeThreads.size() > highWaterMark)
    newHighWaterMark = highWaterMark = process.m_activeThreads.size();

  // unlock the thread list
  process.m_activeThreadMutex.Signal();

  pthread_attr_destroy(&threadAttr);

  PTRACE_IF(newHighWaterMark%100 == 0 ? 2 : 4, newHighWaterMark > 0,
            "PTLib\tThread high water mark set: " << newHighWaterMark);

#ifdef P_MACOSX
  if (PX_priority == HighestPriority) {
    PTRACE(1, "set thread to have the highest priority (MACOSX)");
    SetPriority(HighestPriority);
  }
#endif
}


void PX_SuspendSignalHandler(int)
{
  PThread * thread = PThread::Current();
  if (thread == NULL)
    return;

  PBoolean notResumed = true;
  while (notResumed) {
    BYTE ch;
    notResumed = ::read(thread->unblockPipe[0], &ch, 1) < 0 && errno == EINTR;
#if !( defined(P_NETBSD) && defined(P_NO_CANCEL) )
    pthread_testcancel();
#endif
  }
}


void PThread::Suspend(PBoolean susp)
{
  PAssertPTHREAD(pthread_mutex_lock, (&PX_suspendMutex));

  // Check for start up condition, first time Resume() is called
  if (PX_firstTimeStart) {
    if (susp)
      PX_suspendCount++;
    else {
      if (PX_suspendCount > 0)
        PX_suspendCount--;
      if (PX_suspendCount == 0) {
        PX_firstTimeStart = false;
        Restart();
      }
    }

    PAssertPTHREAD(pthread_mutex_unlock, (&PX_suspendMutex));
    return;
  }

#if defined(P_MACOSX) && (P_MACOSX <= 55)
  // Suspend - warn the user with an Assertion
  PAssertAlways("Cannot suspend threads on Mac OS X due to lack of pthread_kill()");
#else
  if (PPThreadKill(m_threadId, 0)) {

    // if suspending, then see if already suspended
    if (susp) {
      PX_suspendCount++;
      if (PX_suspendCount == 1) {
        if (m_threadId != pthread_self()) {
          signal(SUSPEND_SIG, PX_SuspendSignalHandler);
          PPThreadKill(m_threadId, SUSPEND_SIG);
        }
        else {
          PAssertPTHREAD(pthread_mutex_unlock, (&PX_suspendMutex));
          PX_SuspendSignalHandler(SUSPEND_SIG);
          return;  // Mutex already unlocked
        }
      }
    }

    // if resuming, then see if to really resume
    else if (PX_suspendCount > 0) {
      PX_suspendCount--;
      if (PX_suspendCount == 0) 
        PXAbortBlock();
    }
  }

  PAssertPTHREAD(pthread_mutex_unlock, (&PX_suspendMutex));
#endif // P_MACOSX
}


void PThread::Resume()
{
  Suspend(false);
}


PBoolean PThread::IsSuspended() const
{
  if (PX_firstTimeStart)
    return true;

  if (IsTerminated())
    return false;

  PAssertPTHREAD(pthread_mutex_lock, ((pthread_mutex_t *)&PX_suspendMutex));
  PBoolean suspended = PX_suspendCount != 0;
  PAssertPTHREAD(pthread_mutex_unlock, ((pthread_mutex_t *)&PX_suspendMutex));
  return suspended;
}


void PThread::SetAutoDelete(AutoDeleteFlag deletion)
{
  PAssert(deletion != AutoDeleteThread || (!m_isProcess && this != &PProcess::Current()), PLogicError);
  m_autoDelete = deletion == AutoDeleteThread;
}

#ifdef P_MACOSX
// obtain thread priority of the main thread
static unsigned long
GetThreadBasePriority ()
{
    thread_basic_info_data_t threadInfo;
    policy_info_data_t       thePolicyInfo;
    unsigned int             count;

    if (baseThread == 0) {
      return 0;
    }

    // get basic info
    count = THREAD_BASIC_INFO_COUNT;
    thread_info (pthread_mach_thread_np (baseThread), THREAD_BASIC_INFO,
                 (integer_t*)&threadInfo, &count);

    switch (threadInfo.policy) {
    case POLICY_TIMESHARE:
      count = POLICY_TIMESHARE_INFO_COUNT;
      thread_info(pthread_mach_thread_np (baseThread),
                  THREAD_SCHED_TIMESHARE_INFO,
                  (integer_t*)&(thePolicyInfo.ts), &count);
      return thePolicyInfo.ts.base_priority;

    case POLICY_FIFO:
      count = POLICY_FIFO_INFO_COUNT;
      thread_info(pthread_mach_thread_np (baseThread),
                  THREAD_SCHED_FIFO_INFO,
                  (integer_t*)&(thePolicyInfo.fifo), &count);
      if (thePolicyInfo.fifo.depressed) 
        return thePolicyInfo.fifo.depress_priority;
      return thePolicyInfo.fifo.base_priority;

    case POLICY_RR:
      count = POLICY_RR_INFO_COUNT;
      thread_info(pthread_mach_thread_np (baseThread),
                  THREAD_SCHED_RR_INFO,
                  (integer_t*)&(thePolicyInfo.rr), &count);
      if (thePolicyInfo.rr.depressed) 
        return thePolicyInfo.rr.depress_priority;
      return thePolicyInfo.rr.base_priority;
    }

    return 0;
}
#endif

void PThread::SetPriority(Priority priorityLevel)
{
  PX_priority = priorityLevel;

  if (IsTerminated())
    return;

#if defined(P_LINUX)
  struct sched_param params;
  PAssertPTHREAD(pthread_setschedparam, (m_threadId, GetSchedParam(priorityLevel, params), &params));

#elif defined(P_MACOSX)
  if (priorityLevel == HighestPriority) {
    /* get fixed priority */
    {
      int result;

      thread_extended_policy_data_t   theFixedPolicy;
      thread_precedence_policy_data_t thePrecedencePolicy;
      long                            relativePriority;

      theFixedPolicy.timeshare = false; // set to true for a non-fixed thread
      result = thread_policy_set (pthread_mach_thread_np(m_threadId),
                                  THREAD_EXTENDED_POLICY,
                                  (thread_policy_t)&theFixedPolicy,
                                  THREAD_EXTENDED_POLICY_COUNT);
      if (result != KERN_SUCCESS) {
        PTRACE(1, "thread_policy - Couldn't set thread as fixed priority.");
      }

      // set priority

      // precedency policy's "importance" value is relative to
      // spawning thread's priority
      
      relativePriority = 62 - GetThreadBasePriority();
      PTRACE(3,  "relativePriority is " << relativePriority << " base priority is " << GetThreadBasePriority());
      
      thePrecedencePolicy.importance = relativePriority;
      result = thread_policy_set (pthread_mach_thread_np(m_threadId),
                                  THREAD_PRECEDENCE_POLICY,
                                  (thread_policy_t)&thePrecedencePolicy, 
                                  THREAD_PRECEDENCE_POLICY_COUNT);
      if (result != KERN_SUCCESS) {
        PTRACE(1, "thread_policy - Couldn't set thread priority.");
      }
    }
  }
#endif
}


PThread::Priority PThread::GetPriority() const
{
#if defined(LINUX)
  int policy;
  struct sched_param params;
  
  PAssertPTHREAD(pthread_getschedparam, (m_threadId, &policy, &params));
  
  switch (policy)
  {
    case SCHED_OTHER:
      break;

    case SCHED_FIFO:
    case SCHED_RR:
      return params.sched_priority > sched_get_priority_min(policy) ? HighestPriority : HighPriority;

#ifdef SCHED_BATCH
    case SCHED_BATCH :
      return LowPriority;
#endif

    default:
      /* Unknown scheduler. We don't know what priority this thread has. */
      PTRACE(1, "PTLib\tPThread::GetPriority: unknown scheduling policy #" << policy);
  }
#endif

  return NormalPriority; /* as good a guess as any */
}


#ifndef P_HAS_SEMAPHORES
void PThread::PXSetWaitingSemaphore(PSemaphore * sem)
{
  PAssertPTHREAD(pthread_mutex_lock, (&PX_WaitSemMutex));
  PX_waitingSemaphore = sem;
  PAssertPTHREAD(pthread_mutex_unlock, (&PX_WaitSemMutex));
}
#endif


#ifdef P_GNU_PTH
// GNU PTH threads version (used by NetBSD)
// Taken from NetBSD pkg patches
void PThread::Sleep(const PTimeInterval & timeout)
{
  PTime lastTime;
  PTime targetTime = PTime() + timeout;

  sched_yield();
  lastTime = PTime();

  while (lastTime < targetTime) {
    P_timeval tval = targetTime - lastTime;
    if (select(0, NULL, NULL, NULL, tval) < 0 && errno != EINTR)
      break;

    pthread_testcancel();

    lastTime = PTime();
  }
}

#else
// Normal Posix threads version
void PThread::Sleep(const PTimeInterval & timeout)
{
  PTime lastTime;
  PTime targetTime = lastTime + timeout;
  do {
    P_timeval tval = targetTime - lastTime;
    if (select(0, NULL, NULL, NULL, tval) < 0 && errno != EINTR)
      break;

#if !( defined(P_NETBSD) && defined(P_NO_CANCEL) )
    pthread_testcancel();
#endif

    lastTime = PTime();
  } while (lastTime < targetTime);
}
#endif

void PThread::Yield()
{
  sched_yield();
}


//
//  Terminate the specified thread
//
void PThread::Terminate()
{
  // if thread was not created by PTLib, then don't terminate it
  if (m_originalStackSize <= 0)
    return;

  // if thread calls Terminate on itself, then do it
  // don't use PThread::Current, as the thread may already not be in the
  // active threads list
  if (m_threadId == pthread_self()) {
    pthread_exit(0);
    return;   // keeps compiler happy
  }

  // if the thread is already terminated, then nothing to do
  if (IsTerminated())
    return;

  // otherwise force thread to die
  PTRACE(2, "PTLib\tForcing termination of thread " << (void *)this);

  PXAbortBlock();
  WaitForTermination(20);

#if !defined(P_HAS_SEMAPHORES) && !defined(P_HAS_NAMED_SEMAPHORES)
  PAssertPTHREAD(pthread_mutex_lock, (&PX_WaitSemMutex));
  if (PX_waitingSemaphore != NULL) {
    PAssertPTHREAD(pthread_mutex_lock, (&PX_waitingSemaphore->mutex));
    PX_waitingSemaphore->queuedLocks--;
    PAssertPTHREAD(pthread_mutex_unlock, (&PX_waitingSemaphore->mutex));
    PX_waitingSemaphore = NULL;
  }
  PAssertPTHREAD(pthread_mutex_unlock, (&PX_WaitSemMutex));
#endif

#if ( defined(P_NETBSD) && defined(P_NO_CANCEL) )
  PPThreadKill(m_threadId, SIGKILL);
#else
  if (m_threadId) {
    pthread_cancel(m_threadId);
  }
#endif
}


PBoolean PThread::IsTerminated() const
{
  if (m_isProcess)
    return false; // Process is always still running

  // See if thread is still running
  pthread_t id = m_threadId;
  return id == 0 || pthread_kill(id, 0) != 0;
}


void PThread::WaitForTermination() const
{
  WaitForTermination(PMaxTimeInterval);
}


PBoolean PThread::WaitForTermination(const PTimeInterval & maxWait) const
{
  pthread_t id = m_threadId;
  if (id == 0 || this == Current()) {
    PTRACE(2, "WaitForTermination on 0x" << hex << id << dec << " short circuited");
    return true;
  }
  
  PTRACE(6, "WaitForTermination on 0x" << hex << id << dec << " for " << maxWait);

  PXAbortBlock();   // this assist in clean shutdowns on some systems

  PSimpleTimer timeout(maxWait);
  while (!IsTerminated()) {
    if (timeout.HasExpired())
      return false;

    Sleep(10); // sleep for 10ms. This slows down the busy loop removing 100%
               // CPU usage and also yeilds so other threads can run.
  }

  PTRACE(6, "WaitForTermination on 0x" << hex << id << dec << " finished");
  return true;
}


#if defined(P_LINUX)

static inline unsigned long long jiffies_to_msecs(const unsigned long j)
{
  static long sysconf_HZ = sysconf(_SC_CLK_TCK);
  return (j * 1000LL) / sysconf_HZ;
}


static bool LinuxGetTimes(const PString & statFileName,
#if PTRACING
                          PString & error,
#endif
                          PThread::Times & times)
{
  /* From the man page on the "stat" file
      Status information about the process. This is used by ps(1). It is defined in /usr/src/linux/fs/proc/array.c.
      The fields, in order, with their proper scanf(3) format specifiers, are:
         pid         %d   The process ID.
         comm        %s   The filename of the executable, in parentheses. This is visible
                          whether or not the executable is swapped out.
         state       %c   One character from the string "RSDZTW" where R is running, S is
                          sleeping in an interruptible wait, D is waiting in uninterruptible
                          disk sleep, Z is zombie, T is traced or stopped (on a signal), and
                          W is paging.
         ppid        %d   The PID of the parent.
         pgrp        %d   The process group ID of the process.
         session     %d   The session ID of the process.
         tty_nr      %d   The tty the process uses.
         tpgid       %d   The process group ID of the process which currently owns the tty
                          that the process is connected to.
         flags       %lu  The kernel flags word of the process. For bit meanings, see the
                          PF_* defines in <linux/sched.h>. Details depend on the kernel
                          version.
         minflt      %lu  The number of minor faults the process has made which have not
                          required loading a memory page from disk.
         cminflt     %lu  The number of minor faults that the process's waited-for children
                          have made.
         majflt      %lu  The number of major faults the process has made which have required
                          loading a memory page from disk.
         cmajflt     %lu  The number of major faults that the process's waited-for children
                          have made.
         utime       %lu  The number of jiffies that this process has been scheduled in user
                          mode.
         stime       %lu  The number of jiffies that this process has been scheduled in kernel
                          mode.
         cutime      %ld  The number of jiffies that this process's waited-for children have
                          been scheduled in user mode. (See also times(2).)
         cstime      %ld  The number of jiffies that this process's waited-for children have
                          been scheduled in kernel mode.
         priority    %ld  The standard nice value, plus fifteen. The value is never negative
                          in the kernel.
         nice        %ld  The nice value ranges from 19 (nicest) to -19 (not nice to others).
         num_threads %ld  Number of threads.
         itrealvalue %ld  The time in jiffies before the next SIGALRM is sent to the process
                          due to an interval timer.
         starttime   %lu  The time in jiffies the process started after system boot.
         vsize       %lu  Virtual memory size in bytes.
         rss         %ld  Resident Set Size: number of pages the process has in real memory,
                          minus 3 for administrative purposes. This is just the pages which
                          count towards text, data, or stack space. This does not include
                          pages which have not been demand-loaded in, or which are swapped out.
         rlim        %lu  Current limit in bytes on the rss of the process
                          (usually 4294967295 on i386).
         startcode   %lu  The address above which program text can run.
         endcode     %lu  The address below which program text can run.
         startstack  %lu  The address of the start of the stack.
         kstkesp     %lu  The current value of esp (stack pointer), as found in the kernel
                          stack page for the process.
         kstkeip     %lu  The current EIP (instruction pointer).
         signal      %lu  The bitmap of pending signals.
         blocked     %lu  The bitmap of blocked signals.
         sigignore   %lu  The bitmap of ignored signals.
         sigcatch    %lu  The bitmap of caught signals.
         wchan       %lu  This is the "channel" in which the process is waiting. It is the
                          address of a system call, and can be looked up in a namelist if you
                          need a textual name. (If you have an up-to-date /etc/psdatabase, then
                          try ps -l to see the WCHAN field in action.)
         nswap       %lu  Number of pages swapped (not maintained).
         cnswap      %lu  Cumulative nswap for child processes (not maintained).
         exit_signal %d   Signal to be sent to parent when we die.
         processor   %d   CPU number last executed on.
         rt_priority %lu  (since kernel 2.5.19) Real-time scheduling priority (see sched_setscheduler(2)).
         policy      %lu  (since kernel 2.5.19) Scheduling policy (see sched_setscheduler(2)).
         delayacct_blkio_ticks %llu (since Linux 2.6.18) Aggregated block I/O delays, measured in
                          clock ticks (centiseconds).
  */

  PTextFile statfile(statFileName, PFile::ReadOnly);
  if (!statfile.IsOpen()) {
    PTRACE_PARAM(error = "Could not find thread stat file");
    return false;
  }

  char line[1000];
  statfile.getline(line, sizeof(line));
  if (!statfile.good()) {
    PTRACE_PARAM(error = "Could not read thread stat file");
    return false;
  }

  int pid;
  char comm[100];
  char state;
  int ppid, pgrp, session, tty_nr, tpgid;
  unsigned long flags, minflt, cminflt, majflt, cmajflt, utime, stime;
  long cutime, cstime, priority, nice, num_threads, itrealvalue;
  unsigned long starttime, vsize;
  long rss;
  unsigned long rlim, startcode, endcode, startstack, kstkesp, kstkeip, signal, blocked, sigignore, sigcatch, wchan, nswap, cnswap;
  int exit_signal, processor;
  unsigned long rt_priority, policy;
  unsigned long long delayacct_blkio_ticks;

  // 17698 (maxmcu) R 1 17033 8586 34833 17467 4202560 7
  // 0 0 0 0 0 0 0 -100 0 16
  // 0 55172504 258756608 6741 4294967295 134512640 137352760 3217892976 8185700 15991824
  // 0 0 4 201349635 0 0 0 -1 7 99
  // 2 0

  int count = sscanf(line,
         "%d%s %c%d%d%d%d%d%lu%lu"
         "%lu%lu%lu%lu%lu%ld%ld%ld%ld%ld"
         "%ld%lu%lu%ld%lu%lu%lu%lu%lu%lu"
         "%lu%lu%lu%lu%lu%lu%lu%d%d%lu"
         "%lu%llu",
         &pid, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags, &minflt,
         &cminflt, &majflt, &cmajflt, &utime, &stime, &cutime, &cstime, &priority, &nice, &num_threads,
         &itrealvalue, &starttime, &vsize, &rss, &rlim, &startcode, &endcode, &startstack, &kstkesp, &kstkeip,
         &signal, &blocked, &sigignore, &sigcatch, &wchan, &nswap, &cnswap, &exit_signal, &processor, &rt_priority,
         &policy, &delayacct_blkio_ticks);
  if (count != 42) {
    PTRACE_PARAM(error = psprintf("Not enough values (%d)\n%s", count, line));
    return false;
  }

  times.m_kernel = jiffies_to_msecs(stime);
  times.m_user = jiffies_to_msecs(utime);
  return true;
}


bool PThread::GetTimes(Times & times)
{
  PStringStream statFileName;
  statFileName << "/proc/" << getpid() << "/task/" << PX_linuxId << "/stat";

  PTRACE_PARAM(PString error);

  for (int retry = 0; retry < 3; ++retry) {
    if (LinuxGetTimes(statFileName,
#if PTRACING
                      error,
#endif
                      times)) {
      if (PX_endTick != 0)
        times.m_real = PX_endTick - PX_startTick;
      else
        times.m_real = PTimer::Tick() - PX_startTick;
      return true;
    }
  }

  PTRACE(2, "PTLib\tError reading " << statFileName << ", " << error);
  return false;
}
#else
bool PThread::GetTimes(Times & times)
{
  return false;
}
#endif


int PThread::PXBlockOnIO(int handle, int type, const PTimeInterval & timeout)
{
  PTRACE(7, "PTLib\tPThread::PXBlockOnIO(" << handle << ',' << type << ')');

  if ((handle < 0) || (handle >= PProcess::Current().GetMaxHandles())) {
    PTRACE(2, "PTLib\tAttempt to use illegal handle in PThread::PXBlockOnIO, handle=" << handle);
    errno = EBADF;
    return -1;
  }

  // make sure we flush the buffer before doing a write
  P_fd_set read_fds;
  P_fd_set write_fds;
  P_fd_set exception_fds;

  int retval;
  do {
    switch (type) {
      case PChannel::PXReadBlock:
      case PChannel::PXAcceptBlock:
        read_fds = handle;
        write_fds.Zero();
        exception_fds.Zero();
        break;
      case PChannel::PXWriteBlock:
        read_fds.Zero();
        write_fds = handle;
        exception_fds.Zero();
        break;
      case PChannel::PXConnectBlock:
        read_fds.Zero();
        write_fds = handle;
        exception_fds = handle;
        break;
      default:
        PAssertAlways(PLogicError);
        return 0;
    }

    // include the termination pipe into all blocking I/O functions
    read_fds += unblockPipe[0];

    P_timeval tval = timeout;
    retval = ::select(PMAX(handle, unblockPipe[0])+1,
                      read_fds, write_fds, exception_fds, tval);
  } while (retval < 0 && errno == EINTR);

  if ((retval == 1) && read_fds.IsPresent(unblockPipe[0])) {
    BYTE ch;
    PAssertOS(::read(unblockPipe[0], &ch, 1) != -1);
    errno = EINTR;
    retval =  -1;
    PTRACE(6, "PTLib\tUnblocked I/O fd=" << unblockPipe[0]);
  }

  return retval;
}

void PThread::PXAbortBlock() const
{
  static BYTE ch = 0;
  PAssertOS(::write(unblockPipe[1], &ch, 1) != -1);
  PTRACE(6, "PTLib\tUnblocking I/O fd=" << unblockPipe[0] << " thread=" << GetThreadName());
}


///////////////////////////////////////////////////////////////////////////////

PSemaphore::PSemaphore(PXClass pxc)
{
  pxClass = pxc;

  // these should never be used, as this constructor is
  // only used for PMutex and PSyncPoint and they have their
  // own copy constructors
  
  initialVar = maxCountVar = 0;
  
  if(pxClass == PXSemaphore) {
#if defined(P_HAS_SEMAPHORES)
    /* call sem_init, otherwise sem_destroy fails*/
    PAssertPTHREAD(sem_init, (&semId, 0, 0));
#elif defined(P_HAS_NAMED_SEMAPHORES)
    semId = CreateSem(0);
#else
    currentCount = maximumCount = 0;
    queuedLocks = 0;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condVar, NULL);
#endif
  }
}


PSemaphore::PSemaphore(unsigned initial, unsigned maxCount)
{
  pxClass = PXSemaphore;

  initialVar  = initial;
  maxCountVar = maxCount;

#if defined(P_HAS_SEMAPHORES)
  PAssertPTHREAD(sem_init, (&semId, 0, initial));
#elif defined(P_HAS_NAMED_SEMAPHORES)
  semId = CreateSem(initialVar);
#else
  PAssertPTHREAD(pthread_mutex_init, (&mutex, NULL));
  PAssertPTHREAD(pthread_cond_init, (&condVar, NULL));
  
  PAssert(maxCount > 0, "Invalid semaphore maximum.");
  if (initial > maxCount)
    initial = maxCount;

  currentCount = initial;
  maximumCount = maxCount;
  queuedLocks  = 0;
#endif
}


PSemaphore::PSemaphore(const PSemaphore & sem) 
{
  pxClass = sem.GetSemClass();

  initialVar  = sem.GetInitial();
  maxCountVar = sem.GetMaxCount();

  if(pxClass == PXSemaphore) {
#if defined(P_HAS_SEMAPHORES)
    PAssertPTHREAD(sem_init, (&semId, 0, initialVar));
#elif defined(P_HAS_NAMED_SEMAPHORES)
    semId = CreateSem(initialVar);
#else
    PAssertPTHREAD(pthread_mutex_init, (&mutex, NULL));
    PAssertPTHREAD(pthread_cond_init, (&condVar, NULL));
  
    PAssert(maxCountVar > 0, "Invalid semaphore maximum.");
    if (initialVar > maxCountVar)
      initialVar = maxCountVar;

    currentCount = initialVar;
    maximumCount = maxCountVar;
    queuedLocks  = 0;
#endif
  }
}

PSemaphore::~PSemaphore()
{
  if(pxClass == PXSemaphore) {
#if defined(P_HAS_SEMAPHORES)
    PAssertPTHREAD(sem_destroy, (&semId));
#elif defined(P_HAS_NAMED_SEMAPHORES)
    PAssertPTHREAD(sem_close, (semId));
#else
    PAssert(queuedLocks == 0, "Semaphore destroyed with queued locks");
    PAssertPTHREAD(pthread_mutex_destroy, (&mutex));
    PAssertPTHREAD(pthread_cond_destroy, (&condVar));
#endif
  }
}

#if defined(P_HAS_NAMED_SEMAPHORES)
sem_t * PSemaphore::CreateSem(unsigned initialValue)
{
  sem_t *sem;

  // Since sem_open and sem_unlink are two operations, there is a small
  // window of opportunity that two simultaneous accesses may return
  // the same semaphore. Therefore, the static mutex is used to
  // prevent this.
  static pthread_mutex_t semCreationMutex = PTHREAD_MUTEX_INITIALIZER;
  PAssertPTHREAD(pthread_mutex_lock, (&semCreationMutex));
  
  sem_unlink("/ptlib_sem");
  sem = sem_open("/ptlib_sem", (O_CREAT | O_EXCL), 700, initialValue);
  
  PAssertPTHREAD(pthread_mutex_unlock, (&semCreationMutex));
  
  PAssert(sem != SEM_FAILED, "Couldn't create named semaphore");
  return sem;
}
#endif

void PSemaphore::Wait() 
{
#if defined(P_HAS_SEMAPHORES)
  PAssertPTHREAD(sem_wait, (&semId));
#elif defined(P_HAS_NAMED_SEMAPHORES)
  PAssertPTHREAD(sem_wait, (semId));
#else
  PAssertPTHREAD(pthread_mutex_lock, (&mutex));

  queuedLocks++;
  PThread::Current()->PXSetWaitingSemaphore(this);

  while (currentCount == 0) {
    int err = pthread_cond_wait(&condVar, &mutex);
    PAssert(err == 0 || err == EINTR, psprintf("wait error = %i", err));
  }

  PThread::Current()->PXSetWaitingSemaphore(NULL);
  queuedLocks--;

  currentCount--;

  PAssertPTHREAD(pthread_mutex_unlock, (&mutex));
#endif
}


PBoolean PSemaphore::Wait(const PTimeInterval & waitTime) 
{
  if (waitTime == PMaxTimeInterval) {
    Wait();
    return true;
  }

  // create absolute finish time 
  PTime finishTime;
  finishTime += waitTime;

#if defined(P_HAS_SEMAPHORES)
#ifdef P_HAS_SEMAPHORES_XPG6
  // use proper timed spinlocks if supported.
  // http://www.opengroup.org/onlinepubs/007904975/functions/sem_timedwait.html

  struct timespec absTime;
  absTime.tv_sec  = finishTime.GetTimeInSeconds();
  absTime.tv_nsec = finishTime.GetMicrosecond() * 1000;

  do {
    if (sem_timedwait(&semId, &absTime) == 0)
      return true;
  } while (errno == EINTR);

  PAssert(errno == ETIMEDOUT, strerror(errno));
  return false;

#else
  // loop until timeout, or semaphore becomes available
  // don't use a PTimer, as this causes the housekeeping
  // thread to get very busy
  do {
    if (sem_trywait(&semId) == 0)
      return true;

#if defined(P_LINUX)
  // sched_yield in a tight loop is bad karma
  // for the linux scheduler: http://www.ussg.iu.edu/hypermail/linux/kernel/0312.2/1127.html
    PThread::Current()->Sleep(10);
#else
    PThread::Yield();
#endif
  } while (PTime() < finishTime);

  return false;

#endif
#elif defined(P_HAS_NAMED_SEMAPHORES)
  do {
    if(sem_trywait(semId) == 0)
      return true;
    PThread::Current()->Sleep(10);
  } while (PTime() < finishTime);
  
  return false;
#else

  struct timespec absTime;
  absTime.tv_sec  = finishTime.GetTimeInSeconds();
  absTime.tv_nsec = finishTime.GetMicrosecond() * 1000;

  PAssertPTHREAD(pthread_mutex_lock, (&mutex));

  PThread * thread = PThread::Current();
  thread->PXSetWaitingSemaphore(this);
  queuedLocks++;

  PBoolean ok = true;
  while (currentCount == 0) {
    int err = pthread_cond_timedwait(&condVar, &mutex, &absTime);
    if (err == ETIMEDOUT) {
      ok = false;
      break;
    }
    else
      PAssert(err == 0 || err == EINTR, psprintf("timed wait error = %i", err));
  }

  thread->PXSetWaitingSemaphore(NULL);
  queuedLocks--;

  if (ok)
    currentCount--;

  PAssertPTHREAD(pthread_mutex_unlock, ((pthread_mutex_t *)&mutex));

  return ok;
#endif
}


void PSemaphore::Signal()
{
#if defined(P_HAS_SEMAPHORES)
  PAssertPTHREAD(sem_post, (&semId));
#elif defined(P_HAS_NAMED_SEMAPHORES)
  PAssertPTHREAD(sem_post, (semId));
#else
  PAssertPTHREAD(pthread_mutex_lock, (&mutex));

  if (currentCount < maximumCount)
    currentCount++;

  if (queuedLocks > 0) 
    PAssertPTHREAD(pthread_cond_signal, (&condVar));

  PAssertPTHREAD(pthread_mutex_unlock, (&mutex));
#endif
}


PBoolean PSemaphore::WillBlock() const
{
#if defined(P_HAS_SEMAPHORES)
  if (sem_trywait((sem_t *)&semId) != 0) {
    PAssertOS(errno == EAGAIN || errno == EINTR);
    return true;
  }
  PAssertPTHREAD(sem_post, ((sem_t *)&semId));
  return false;
#elif defined(P_HAS_NAMED_SEMAPHORES)
  if (sem_trywait(semId) != 0) {
    PAssertOS(errno == EAGAIN || errno == EINTR);
    return true;
  }
  PAssertPTHREAD(sem_post, (semId));
  return false;
#else
  return currentCount == 0;
#endif
}


///////////////////////////////////////////////////////////////////////////////

PTimedMutex::PTimedMutex()
{
  Construct();
}

PTimedMutex::PTimedMutex(const PTimedMutex &)
{
  Construct();
}


void PTimedMutex::Construct()
{
#if P_HAS_RECURSIVE_MUTEX

  pthread_mutexattr_t attr;
  PAssertPTHREAD(pthread_mutexattr_init, (&attr));

#if (P_HAS_RECURSIVE_MUTEX == 2)
  PAssertPTHREAD(pthread_mutexattr_settype, (&attr, PTHREAD_MUTEX_RECURSIVE));
#else
  PAssertPTHREAD(pthread_mutexattr_settype, (&attr, PTHREAD_MUTEX_RECURSIVE_NP));
#endif

  PAssertPTHREAD(pthread_mutex_init, (&m_mutex, &attr));
  PAssertPTHREAD(pthread_mutexattr_destroy, (&attr));

#else // P_HAS_RECURSIVE_MUTEX

  m_lockerId = (pthread_id)-1;
  PAssertPTHREAD(pthread_mutex_init, (&m_mutex, NULL));

#endif // P_HAS_RECURSIVE_MUTEX
}


PTimedMutex::~PTimedMutex()
{
  int result = pthread_mutex_destroy(&m_mutex);
  if (result == EBUSY) {
    // In case it is us
    while (pthread_mutex_unlock(&m_mutex) == 0)
      ;

    // Wait a bit for someone else to unlock it
    for (PINDEX i = 0; i < 100; ++i) {
      if ((result = pthread_mutex_destroy(&m_mutex)) != EBUSY)
        break;
      pthread_yield();
    }
  }

#ifdef _DEBUG
  PAssert(result == 0, "Error destroying mutex");
#endif
}


void PTimedMutex::Wait() 
{
#if P_HAS_RECURSIVE_MUTEX

  PAssertPTHREAD(pthread_mutex_lock, (&m_mutex));

#else //P_HAS_RECURSIVE_MUTEX

  pthread_t currentThreadId = pthread_self();

  // if the mutex is already acquired by this thread,
  // then just increment the lock count
  if (pthread_equal(m_lockerId, currentThreadId)) {
    // Note this does not need a lock as it can only be touched by the thread
    // which already has the mutex locked.
    ++m_lockCount;
    return;
  }

  // acquire the lock for real
  PAssertPTHREAD(pthread_mutex_lock, (&m_mutex));

  PAssert(m_lockerId == (pthread_t)-1 && m_lockCount.IsZero(),
          "PMutex acquired whilst locked by another thread");

  // Note this is protected by the mutex itself only the thread with
  // the lock can alter it.
  m_lockerId = currentThreadId;

#endif // P_HAS_RECURSIVE_MUTEX
}


PBoolean PTimedMutex::Wait(const PTimeInterval & waitTime) 
{
  // if waiting indefinitely, then do so
  if (waitTime == PMaxTimeInterval) {
    Wait();
    return true;
  }

#if !P_HAS_RECURSIVE_MUTEX
  pthread_t currentThreadId = pthread_self();

  // if we already have the mutex, return immediately
  if (pthread_equal(m_lockerId, currentThreadId)) {
    // Note this does not need a lock as it can only be touched by the thread
    // which already has the mutex locked.
    ++m_lockCount;
    return true;
  }
#endif

  // create absolute finish time
  PTime finishTime;
  finishTime += waitTime;

#if P_PTHREADS_XPG6
  
  struct timespec absTime;
  absTime.tv_sec  = finishTime.GetTimeInSeconds();
  absTime.tv_nsec = finishTime.GetMicrosecond() * 1000;

  if (pthread_mutex_timedlock(&m_mutex, &absTime) != 0)
    return false;

#else // P_PTHREADS_XPG6

  while (pthread_mutex_trylock(&m_mutex) != 0) {
    if (PTime() >= finishTime)
      return false;
    usleep(10000);
  }

#endif // P_PTHREADS_XPG6

#if !P_HAS_RECURSIVE_MUTEX
  PAssert((lockerId == (pthread_t)-1) && m_lockCount.IsZero(),
          "PMutex acquired whilst locked by another thread");

  // Note this is protected by the mutex itself only the thread with
  // the lock can alter it.
  m_lockerId = currentThreadId;
#endif

  return true;
}


void PTimedMutex::Signal()
{
#if !P_HAS_RECURSIVE_MUTEX
  if (!pthread_equal(m_lockerId, pthread_self())) {
    PTRACE(1, "PMutex signal failed - no matching wait or signal by wrong thread");
    //PAssertAlways("PMutex signal failed - no matching wait or signal by wrong thread");
    return;
  }

  // if lock was recursively acquired, then decrement the counter
  // Note this does not need a separate lock as it can only be touched by the thread
  // which already has the mutex locked.
  if (!m_lockCount.IsZero()) {
    --m_lockCount;
    return;
  }

  // otherwise mark mutex as available
  m_lockerId = (pthread_t)-1;

#endif

  PAssertPTHREAD(pthread_mutex_unlock, (&m_mutex));
}


PBoolean PTimedMutex::WillBlock() const
{
#if !P_HAS_RECURSIVE_MUTEX
  if (pthread_equal(m_lockerId, pthread_self()))
    return false;
#endif

  if (pthread_mutex_trylock(&m_mutex) != 0)
    return true;

  PAssertPTHREAD(pthread_mutex_unlock, (&m_mutex));
  return false;
}


///////////////////////////////////////////////////////////////////////////////

PSyncPoint::PSyncPoint()
  : PSemaphore(PXSyncPoint)
{
  PAssertPTHREAD(pthread_mutex_init, (&mutex, NULL));
  PAssertPTHREAD(pthread_cond_init, (&condVar, NULL));
  signalled = false;
}

PSyncPoint::PSyncPoint(const PSyncPoint &)
  : PSemaphore(PXSyncPoint)
{
  PAssertPTHREAD(pthread_mutex_init, (&mutex, NULL));
  PAssertPTHREAD(pthread_cond_init, (&condVar, NULL));
  signalled = false;
}

PSyncPoint::~PSyncPoint()
{
  PAssertPTHREAD(pthread_mutex_destroy, (&mutex));
  PAssertPTHREAD(pthread_cond_destroy, (&condVar));
}

void PSyncPoint::Wait()
{
  PAssertPTHREAD(pthread_mutex_lock, (&mutex));
  while (!signalled)
    pthread_cond_wait(&condVar, &mutex);
  signalled = false;
  PAssertPTHREAD(pthread_mutex_unlock, (&mutex));
}


PBoolean PSyncPoint::Wait(const PTimeInterval & waitTime)
{
  PAssertPTHREAD(pthread_mutex_lock, (&mutex));

  PTime finishTime;
  finishTime += waitTime;
  struct timespec absTime;
  absTime.tv_sec  = finishTime.GetTimeInSeconds();
  absTime.tv_nsec = finishTime.GetMicrosecond() * 1000;

  int err = 0;
  while (!signalled) {
    err = pthread_cond_timedwait(&condVar, &mutex, &absTime);
    if (err == 0 || err == ETIMEDOUT)
      break;

    PAssertOS(err == EINTR && errno == EINTR);
  }

  if (err == 0)
    signalled = false;

  PAssertPTHREAD(pthread_mutex_unlock, (&mutex));

  return err == 0;
}


void PSyncPoint::Signal()
{
  PAssertPTHREAD(pthread_mutex_lock, (&mutex));
  signalled = true;
  PAssertPTHREAD(pthread_cond_signal, (&condVar));
  PAssertPTHREAD(pthread_mutex_unlock, (&mutex));
}


PBoolean PSyncPoint::WillBlock() const
{
  return !signalled;
}


