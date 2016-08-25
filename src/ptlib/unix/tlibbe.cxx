/* 
 * tlibbe.cxx
 *
 * Thread library implementation for BeOS
 *
 * Portable Windows Library
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
 * Portions are Copyright (c) 1993-1998 Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): Yuri Kiryanov, ykiryanov at users.sourceforge.net
 *
 * $Revision$
 * $Author$
 * $Date$
 */

class PThread;
class PProcess;
class PSemaphore;
class PSyncPoint;

class PMutex; 

#include <ptlib.h>
#include <ptlib/socket.h>

#ifdef B_ZETA_VERSION 
#include <posix/rlimit.h>
#endif // Zeta

// For class BLocker
#include <be/support/Locker.h>

int PX_NewHandle(const char *, int);

#define DEBUG_SEMAPHORES1 1

//////////////////////////////////////////////////////////////////////////////
// Threads

static int const priorities[] = {
  1, // Lowest priority is 1. 0 is not
  B_LOW_PRIORITY,
  B_NORMAL_PRIORITY,
  B_DISPLAY_PRIORITY,
  B_URGENT_DISPLAY_PRIORITY,
};

int32 PThread::ThreadFunction(void * threadPtr)
{
  PThread * thread = (PThread *)PAssertNULL(threadPtr);

  PProcess & process = PProcess::Current();

  process.activeThreadMutex.Wait();
  process.activeThreads.SetAt((unsigned) thread->mId, thread);
  process.activeThreadMutex.Signal();

  process.OnThreadStart(*thread);

  thread->Main();

  process.OnThreadEnded(*thread);

  return 0;
}

PThread::PThread()
 : autoDelete(false)
 , mId(find_thread(NULL))
 , mPriority(B_NORMAL_PRIORITY)
 , mStackSize(0)
 , mSuspendCount(1)
{
  PAssert(::pipe(unblockPipe) == 0, "Pipe creation failed in PThread::PThread()!");
  PAssertOS(unblockPipe[0]);
  PAssertOS(unblockPipe[1]);

  if (!PProcess::IsInitialised())
    return;

  autoDelete = true;

  PProcess & process = PProcess::Current();

  process.activeThreadMutex.Wait();
  process.activeThreads.SetAt(PX_threadId, this);
  process.activeThreadMutex.Signal();

  process.SignalTimerChange();
}

PThread::PThread(PINDEX stackSize,
                 AutoDeleteFlag deletion,
                 Priority priorityLevel,
                 const PString & name)
 : mId(B_BAD_THREAD_ID),
   mPriority(B_NORMAL_PRIORITY),
   mStackSize(0),
   mSuspendCount(0)
{
  PAssert(stackSize > 0, PInvalidParameter);
  autoDelete = deletion == AutoDeleteThread;
 
  mId =  ::spawn_thread(ThreadFunction, // Function 
         (const char*) name, // Name
         priorities[priorityLevel], // Priority 
         (void *) this); // Pass this as cookie

  PAssertOS(mId >= B_NO_ERROR);
    
  mSuspendCount = 1;
  mStackSize = stackSize;
  mPriority = priorities[priorityLevel];

  threadName.sprintf(name, mId);
  ::rename_thread(mId, (const char*) threadName); // real, unique name - with id

  PAssert(::pipe(unblockPipe) == 0, "Pipe creation failed in PThread constructor");
  PX_NewHandle("Thread unblock pipe", PMAX(unblockPipe[0], unblockPipe[1]));
}

PThread::~PThread()
{
  // if we are not process, remove this thread from the active thread list
  PProcess & process = PProcess::Current();
  if(process.GetThreadId() != GetThreadId())
  {
    process.activeThreadMutex.Wait();
    process.activeThreads.RemoveAt((unsigned) mId);
    process.activeThreadMutex.Signal();
  }

  if (!IsTerminated())
    Terminate();

  ::close(unblockPipe[0]);
  ::close(unblockPipe[1]);
}


void PThread::Restart()
{
  if(!IsTerminated())
    return;

  mId =  ::spawn_thread(ThreadFunction, // Function 
         "PWLT", // Name
          mPriority, 
          (void *) this); // Pass this as cookie

  PAssertOS(mId >= B_NO_ERROR);

  threadName.sprintf("PWLib Thread %d", mId);
  ::rename_thread(mId, (const char*) threadName); // real, unique name - with id
}

void PThread::Terminate()
{
  if(mStackSize <=0)
    return;

  if(mId == find_thread(NULL))
  {
    ::exit_thread(0);
    return;
  }

  if(IsTerminated())
    return;

  PXAbortBlock();
  WaitForTermination(20);

 if(mId > B_BAD_THREAD_ID)
   ::kill_thread(0);
}

PBoolean PThread::IsTerminated() const
{
  return mId == B_BAD_THREAD_ID;
}


void PThread::WaitForTermination() const
{
  WaitForTermination(PMaxTimeInterval);
}


PBoolean PThread::WaitForTermination(const PTimeInterval & /*maxWait*/) const // Fix timeout
{
  status_t result = B_NO_ERROR;
  status_t exit_value = B_NO_ERROR;

  result = ::wait_for_thread(mId, &exit_value);
  if ( result == B_INTERRUPTED ) { // thread was killed.
    return PTrue;
  }

  if ( result == B_OK ) { // thread is dead
    #ifdef DEBUG_THREADS
    PError << "B_OK" << endl;
    #endif
    return PTrue;
  }

  if ( result == B_BAD_THREAD_ID ) { // thread has invalid id
    return PTrue;
  }

  return PFalse;
}


void PThread::Suspend(PBoolean susp)
{

  PAssert(!IsTerminated(), "Operation on terminated thread");
  if (susp)
  {
    status_t result = ::suspend_thread(mId);
    if(B_OK == result)
	::atomic_add(&mSuspendCount, 1);

    PAssert(result == B_OK, "Thread don't want to be suspended");
  }
  else
    Resume();
}


void PThread::Resume()
{
  PAssert(!IsTerminated(), "Operation on terminated thread");
  status_t result = ::resume_thread(mId);
  if(B_OK == result)
    ::atomic_add(&mSuspendCount, -1);

  PAssert(result == B_NO_ERROR, "Thread doesn't want to resume");
}


PBoolean PThread::IsSuspended() const
{
  return (mSuspendCount > 0);
}

void PThread::SetAutoDelete(AutoDeleteFlag deletion)
{
  PAssert(deletion != AutoDeleteThread || this != &PProcess::Current(), PLogicError);
  autoDelete = deletion == AutoDeleteThread;
}

void PThread::SetPriority(Priority priorityLevel)
{
  PAssert(!IsTerminated(), "Operation on terminated thread");

  mPriority = priorities[priorityLevel];
  status_t result = ::set_thread_priority(mId, mPriority );
  if(result != B_OK)
    PTRACE(0, "Changing thread priority failed, error " << strerror(result) << endl);

}


PThread::Priority PThread::GetPriority() const
{
  if(!IsTerminated())
  {

  switch (mPriority) {
    case 0 :
      return LowestPriority;
    case B_LOW_PRIORITY :
      return LowPriority;
    case B_NORMAL_PRIORITY :
      return NormalPriority;
    case B_DISPLAY_PRIORITY :
      return HighPriority;
    case B_URGENT_DISPLAY_PRIORITY :
      return HighestPriority;
  }
  PAssertAlways(POperatingSystemError);
  
  }
  return LowestPriority;
}

void PThread::Yield()
{
  // we just sleep for long enough to cause a reschedule (100 microsec)
  ::snooze(100);
}

void PThread::Sleep( const PTimeInterval & delay ) // Time interval to sleep for.
{
  bigtime_t microseconds = 
		delay == PMaxTimeInterval ? B_INFINITE_TIMEOUT : (delay.GetMilliSeconds() * 1000 );
 
  status_t result = ::snooze( microseconds ) ; // delay in ms, snooze in microsec
  PAssert(result == B_OK, "Thread has insomnia");
}

int PThread::PXBlockOnChildTerminate(int pid, const PTimeInterval & /*timeout*/) // Fix timeout
{
  status_t result = B_NO_ERROR;
  status_t exit_value = B_NO_ERROR;

  result = ::wait_for_thread(pid, &exit_value);
  if ( result == B_INTERRUPTED ) 
  { 
    // thread was killed.
    #ifdef DEBUG_THREADS
    PError << "B_INTERRUPTED" << endl;
    #endif
    return 1;
  }

  if ( result == B_OK ) 
  { 
    // thread is dead
     return 1;
  }

  if ( result == B_BAD_THREAD_ID ) 
  { 
    // thread has invalid id
    return 1;
  }

  return 0; // ???
}

PThreadIdentifier PThread::GetCurrentThreadId(void)
{
  return ::find_thread(NULL);
}

int PThread::PXBlockOnIO(int handle, int type, const PTimeInterval & timeout)
{
  PTRACE(7, "PWLib\tPThread::PXBlockOnIO(" << handle << ',' << type << ')');

  if ((handle < 0) || (handle >= PProcess::Current().GetMaxHandles())) {
    PTRACE(2, "PWLib\tAttempt to use illegal handle in PThread::PXBlockOnIO, handle=" << handle);
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
    ::read(unblockPipe[0], &ch, 1);
    errno = EINTR;
    retval =  -1;
    PTRACE(6, "PWLib\tUnblocked I/O");
  }

  return retval;
}

void PThread::PXAbortBlock(void) const
{
  BYTE ch;
  ::write(unblockPipe[1], &ch, 1);
}

///////////////////////////////////////////////////////////////////////////////
// PProcess
PDECLARE_CLASS(PHouseKeepingThread, PThread)
  public:
    PHouseKeepingThread()
      : PThread(1000, NoAutoDeleteThread, HighestPriority, "Housekeeper")
      { closing = PFalse; Resume(); }

    void Main();
    void SetClosing() { closing = PTrue; }

  protected:
    PBoolean closing;
};

void PProcess::Construct()
{
  maxHandles = FOPEN_MAX;
  PTRACE(4, "PWLib\tMaximum per-process file handles is " << maxHandles);

  // initialise the housekeeping thread
  housekeepingThread = NULL;

  CommonConstruct();
}

void PHouseKeepingThread::Main()
{
  PProcess & process = PProcess::Current();

  while (!closing) {
    PTimeInterval delay = process.timers.Process();

    globalBreakBlock.Wait(delay);

    process.PXCheckSignals();
  }    
}

void PProcess::SignalTimerChange()
{
  if (!PAssert(IsInitialised(), PLogicError) || m_shuttingDown) 
    return false;

  if (housekeepingThread == NULL)
  {  
    housekeepingThread = new PHouseKeepingThread;
  }

  globalBreakBlock.Signal();
}

PBoolean PProcess::SetMaxHandles(int newMax)
{
  return PFalse;
}

PProcess::~PProcess()
{
  PreShutdown();

  // Don't wait for housekeeper to stop if Terminate() is called from it.
  if (housekeepingThread != NULL && PThread::Current() != housekeepingThread) {
    housekeepingThread->SetClosing();
    SignalTimerChange();
    housekeepingThread->WaitForTermination();
    delete housekeepingThread;
  }

  CommonDestruct();
  PostShutdown();
}

///////////////////////////////////////////////////////////////////////////////
// PSemaphore
PSemaphore::PSemaphore(PBoolean fNested) : mfNested(fNested)
{
}

PSemaphore::PSemaphore(unsigned initial, unsigned)
{
  Create(initial);
}
 
void PSemaphore::Create(unsigned initial)
{
  mOwner = ::find_thread(NULL);
  PAssertOS(mOwner != B_BAD_THREAD_ID);
  if(!mfNested)
  {
    mCount = initial;
    semId = ::create_sem(initial, "PWLS"); 

    PAssertOS(semId >= B_NO_ERROR);

    #ifdef DEBUG_SEMAPHORES
    sem_info info;
    get_sem_info(semId, &info);
    PError << "::create_sem (PSemaphore()), id: " << semId << ", this: " << this << ", count:" << info.count << endl;
    #endif
  }
  else // Use BLocker
  {
    semId = (sem_id) new BLocker("PWLN", true); // PWLib use recursive locks. true for benaphore style, false for not
  }
}

PSemaphore::~PSemaphore()
{
  if(!mfNested)
  {
    status_t result = B_NO_ERROR;
    PAssertOS(semId >= B_NO_ERROR);
  
    // Transmit ownership of the semaphore to our thread
    thread_id curThread = ::find_thread(NULL);
    if(mOwner != curThread)
    {
     thread_info tinfo;
     ::get_thread_info(curThread, &tinfo);
     ::set_sem_owner(semId, tinfo.team);
      mOwner = curThread; 
    } 
 
    #ifdef DEBUG_SEMAPHORES
    sem_info info;
    get_sem_info(semId, &info);
    PError << "::delete_sem, id: " << semId << ", this: " << this << ", name: " << info.name << ", count:" << info.count;
    #endif 

    // Deleting the semaphore id
    result = ::delete_sem(semId);

    #ifdef DEBUG_SEMAPHORES
    if( result != B_NO_ERROR )
      PError << "...delete_sem failed, error: " << strerror(result) << endl;
    #endif
  }
  else // Use BLocker
  {
    delete (BLocker*) semId; // Thanks!
  }
}

void PSemaphore::Wait()
{
  if(!mfNested)
  {
    PAssertOS(semId >= B_NO_ERROR);
 
    status_t result = B_NO_ERROR;

    #ifdef DEBUG_SEMAPHORES
    sem_info info;
    get_sem_info(semId, &info);
    PError << "::acquire_sem, id: " << semId << ", name: " << info.name << ", count:" << info.count << endl;
    #endif 

    while((B_BAD_THREAD_ID != mOwner) 
      && ((result = ::acquire_sem(semId)) == B_INTERRUPTED))
    {
    }
  }
  else
  {
    ((BLocker*)semId)->Lock(); // Using class to support recursive locks 
  }
}

PBoolean PSemaphore::Wait(const PTimeInterval & timeout)
{
  PInt64 ms = timeout.GetMilliSeconds();
  bigtime_t microseconds = ms * 1000;

  status_t result = B_NO_ERROR;
   
  if(!mfNested)
  {
    PAssertOS(semId >= B_NO_ERROR);
    PAssertOS(timeout < PMaxTimeInterval);

    #ifdef DEBUG_SEMAPHORES
    sem_info info;
    get_sem_info(semId, &info);
    PError << "::acquire_sem_etc " << semId << ",this: " << this << ", name: " << info.name << ", count:" << info.count 
      << ", ms: " << microseconds << endl;
    #endif
 
    while((B_BAD_THREAD_ID != mOwner) 
      && ((result = ::acquire_sem_etc(semId, 1, 
        B_RELATIVE_TIMEOUT, microseconds)) == B_INTERRUPTED))
    {
    }
  }
  else
  {
    result = ((BLocker*)semId)->LockWithTimeout(microseconds); // Using BLocker class to support recursive locks 
  }

  return ms == 0 ? PFalse : result == B_OK;
}

void PSemaphore::Signal()
{
  if(!mfNested)
  {
    PAssertOS(semId >= B_NO_ERROR);
 
    #ifdef DEBUG_SEMAPHORES
    sem_info info;
    get_sem_info(semId, &info);
    PError << "::release_sem " << semId << ", this: " << this << ", name: " << info.name << ", count:" << info.count << endl;
    #endif 
      ::release_sem(semId);
   }
   else
   {
     ((BLocker*)semId)->Unlock(); // Using BLocker class to support recursive locks 
   }		
}

PBoolean PSemaphore::WillBlock() const
{
  if(!mfNested)
  {
    PAssertOS(semId >= B_NO_ERROR);

    #ifdef DEBUG_SEMAPHORES
    sem_info info;
    get_sem_info(semId, &info);
    PError << "::acquire_sem_etc (WillBlock) " << semId << ", this: " << this << ", name: " << info.name << ", count:" << info.count << endl;
    #endif
	
    status_t result = ::acquire_sem_etc(semId, 0, B_RELATIVE_TIMEOUT, 0);
    return result == B_WOULD_BLOCK;
  }
  else
  {
    return mOwner == find_thread(NULL); // If we are in our own thread, we won't lock
  }
}

///////////////////////////////////////////////////////////////////////////////
// PSyncPoint

PSyncPoint::PSyncPoint()
 : PSemaphore(PFalse) // PFalse is semaphore based, PTrue means implemented through BLocker
{
   PSemaphore::Create(0);
}

void PSyncPoint::Signal()
{
  PSemaphore::Signal();
}
                                                                                                      
void PSyncPoint::Wait()
{
  PSemaphore::Wait();
}
                                                                                                      
PBoolean PSyncPoint::Wait(const PTimeInterval & timeout)
{
  return PSemaphore::Wait(timeout);
}
                                                                                                      
PBoolean PSyncPoint::WillBlock() const
{
  return PSemaphore::WillBlock();
}

//////////////////////////////////////////////////////////////////////////////
// PMutex, derived from BLightNestedLocker  

PMutex::PMutex() 
  : PSemaphore(PTrue) // PTrue means implemented through BLocker
{
  PSemaphore::Create(0);
}

PMutex::PMutex(const PMutex&) 
 : PSemaphore(PTrue)
{
  PAssertAlways("PMutex copy constructor not supported");
} 

void PMutex::Signal()
{
  PSemaphore::Signal();
}
                                                                                                      
void PMutex::Wait()
{
  PSemaphore::Wait();
}
                                                                                                      
PBoolean PMutex::Wait(const PTimeInterval & timeout)
{
  return PSemaphore::Wait(timeout);
}
                                                                                                      
PBoolean PMutex::WillBlock() const
{
  return PSemaphore::WillBlock();
}

//////////////////////////////////////////////////////////////////////////////
// Extra functionality not found in BeOS

int seteuid(uid_t uid) { return 0; }
int setegid(gid_t gid) { return 0; }

///////////////////////////////////////////////////////////////////////////////
// Toolchain dependent stuff
#if (__GNUC_MINOR__  > 9)
#warning "Using gcc 2.95.x"
    ostream& ostream::write(const char *s, streamsize n) { return write(s, (long) n); };
    istream& istream::read(char *s, streamsize n) { return read(s, (long) n); };
#endif // gcc minor  > 9  

// End Of File ///////////////////////////////////////////////////////////////
