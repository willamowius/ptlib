/*
 * thread.h
 *
 * Executable thread encapsulation class (pre-emptive if OS allows).
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

#ifndef PTLIB_THREAD_H
#define PTLIB_THREAD_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifdef Priority
#undef Priority
#endif

#include <ptlib/mutex.h>

class PSemaphore;


///////////////////////////////////////////////////////////////////////////////
// PThread

/** This class defines a thread of execution in the system. A <i>thread</i> is
   an independent flow of processor instructions. This differs from a
   <i>process</i> which also embodies a program address space and resource
   allocation. So threads can share memory and resources as they run in the
   context of a given process. A process always contains at least one thread.
   This is reflected in this library by the <code>PProcess</code> class being
   descended from the <code>PThread</code> class.

   The implementation of a thread is platform dependent, but it is
   assumed that the platform has some support for native threads.
   Previous versions of PTLib/PWLib have some support for co-operative
   threads, but this has been removed.
 */
class PThread : public PObject
{
  PCLASSINFO(PThread, PObject);

  public:
  /**@name Construction */
  //@{
    /// Codes for thread priorities.
    enum Priority {
      LowestPriority,  ///< Will only run if all other threads are blocked.

      LowPriority,     ///< Runs approximately half as often as normal.

      NormalPriority,  ///< Normal priority for a thread.

      HighPriority,    ///< Runs approximately twice as often as normal.

      HighestPriority, ///< Is only thread that will run, unless blocked.

      NumPriorities
    };

    /// Codes for thread autodelete flag
    enum AutoDeleteFlag {
      /// Automatically delete thread object on termination.
      AutoDeleteThread,   

      /// Don't delete thread as it may not be on heap.
      NoAutoDeleteThread  
    };

    /** Create a new thread instance. Unless the <code>startSuspended</code>
       parameter is <code>true</code>, the threads <code>Main()</code> function is called to
       execute the code for the thread.
       
       Note that the exact timing of the execution of code in threads can
       never be predicted. Thus you you can get a race condition on
       intialising a descendent class. To avoid this problem a thread is
       always started suspended. You must call the <code>Resume()</code> function after
       your descendent class construction is complete.

       If synchronisation is required between threads then the use of
       semaphores is essential.

       If the <code>deletion</code> is set to <code>AutoDeleteThread</code>
       then the <code>PThread</code> is assumed to be allocated with the new operator and
       may be freed using the delete operator as soon as the thread is
       terminated or executes to completion (usually the latter).

       The stack size argument retained only for source code compatibility for
       previous implementations. It is not used in the current code and
       may be removed in subsequent versions.
     */
    PThread(
      PINDEX,                 ///< Not used - previously stack size
      AutoDeleteFlag deletion = AutoDeleteThread,
        ///< Automatically delete PThread instance on termination of thread.
      Priority priorityLevel = NormalPriority,  ///< Initial priority of thread.
      const PString & threadName = PString::Empty() ///< The name of the thread (for Debug/Trace)
    );

    /** Destroy the thread, this simply calls the <code>Terminate()</code> function
       with all its restrictions and penalties. See that function for more
       information.

       Note that the correct way for a thread to terminate is to return from
       the <code>Main()</code> function.
     */
    ~PThread();
  //@}

  /**@name Overrides from <code>PObject</code> */
  //@{
    /**Standard stream print function.
       The <code>PObject</code> class has a << operator defined that calls this function
       polymorphically.
      */
    void PrintOn(
      ostream & strm    ///< Stream to output text representation
    ) const;
  //@}

  /**@name Control functions */
  //@{
    /** Restart a terminated thread using the same stack priority etc that
       was current when the thread terminated.
       
       If the thread is still running then this function is ignored.
     */
    virtual void Restart();

    /** Terminate the thread. It is highly recommended that this is not used
       except in abnormal abort situations as not all clean up of resources
       allocated to the thread will be executed. This is especially true in
       C++ as the destructors of objects that are automatic variables are not
       called causing at the very least the possiblity of memory leaks.

       Note that the correct way for a thread to terminate is to return from
       the <code>Main()</code> function or self terminate by calling
       <code>Terminate()</code> within the context of the thread which can then
       assure that all resources are cleaned up.
     */
    virtual void Terminate();

    /** Determine if the thread has been terminated or ran to completion.

       @return
       <code>true</code> if the thread has been terminated.
     */
    virtual PBoolean IsTerminated() const;

    /** Block and wait for the thread to terminate.
     */
    void WaitForTermination() const;

    /** Block and wait for the thread to terminate.

       @return
       <code>false</code> if the thread has not terminated and the timeout has expired, <code>true</code> otherwise.
     */
    PBoolean WaitForTermination(
      const PTimeInterval & maxWait  ///< Maximum time to wait for termination.
    ) const;

    /** Suspend or resume the thread.
    
       If <code>susp</code> is <code>true</code> this increments an internal count of
       suspensions that must be matched by an equal number of calls to
       <code>Resume()</code> or <code>Suspend(false)</code> before the
       thread actually executes again.

       If <code>susp</code> is <code>false</code> then this decrements the internal count of
       suspensions. If the count is <= 0 then the thread will run. Note that
       the thread will not be suspended until an equal number of
       <code>Suspend(true)</code> calls are made.
     */
    virtual void Suspend(
      PBoolean susp = true    ///< Flag to suspend or resume a thread.
    );

    /** Resume thread execution, this is identical to
       <code>Suspend(false)</code>.

      The Resume() method may be called from within the constructor of a
      PThread descendant.  However, the <code>Resume()</code> should be in the
      constructor of the most descendant class. So, if you have a
      class B (which is descended of PThread), and a class C (which is
      descended of B), placing the call to <code>Resume()</code> in the constructor of B is
      unwise.

      If you do place a call to <code>Resume()</code> in the constructor, it
      should be at the end of the constructor, after all the other
      initialisation in the constructor.

      The reason the call to <code>Resume()</code> should be at the end of the
      construction process is simple - you want the thread to start
      when all the variables in the class have been correctly
      initialised.
     */
    virtual void Resume();

    /** Determine if the thread is currently suspended. This checks the
       suspension count and if greater than zero returns <code>true</code> for a suspended
       thread.

       @return
       <code>true</code> if thread is suspended.
     */
    virtual PBoolean IsSuspended() const;

    /// Suspend the current thread for the specified amount of time.
    static void Sleep(
      const PTimeInterval & delay   ///< Time interval to sleep for.
    );

    /** Set the priority of the thread relative to other threads in the current
       process.
     */
    virtual void SetPriority(
      Priority priorityLevel    ///< New priority for thread.
    );

    /** Get the current priority of the thread in the current process.

       @return
       current thread priority.
     */
    virtual Priority GetPriority() const;

    /** Set the flag indicating thread object is to be automatically deleted
        when the thread ends.
     */
    virtual void SetAutoDelete(
      AutoDeleteFlag deletion = AutoDeleteThread  ///< New auto delete setting.
    );

    /** Reet the flag indicating thread object is to be automatically deleted
        when the thread ends.
     */
    void SetNoAutoDelete() { SetAutoDelete(NoAutoDeleteThread); }

    /** Get the name of the thread. Thread names are a optional debugging aid.

       @return
       current thread name.
     */
    virtual PString GetThreadName() const;

    /** Change the name of the thread. Thread names are a optional debugging aid.

       @return
       current thread name.
     */
    virtual void SetThreadName(
      const PString & name        ///< New name for the thread.
    );
  //@}

  /**@name Miscellaneous */
  //@{
    /** Get operating system specific thread identifier for this thread.
      * Note that the return value from these functions is only valid
      * if called by the owning thread. Calling this function for another
      * thread that may be terminating is a very bad idea.
      */
    virtual PThreadIdentifier GetThreadId() const { return m_threadId; }
    static PThreadIdentifier GetCurrentThreadId();

    /// Times for execution of the thread.
    struct Times
    {
      PTimeInterval m_real;     ///< Total real time since thread start in milliseconds.
      PTimeInterval m_kernel;   ///< Total kernel CPU time in milliseconds.
      PTimeInterval m_user;     ///< Total user CPU time in milliseconds.
      friend ostream & operator<<(ostream & strm, const Times & times);
    };

    /** Get the thread execution times.
     */
    bool GetTimes(
      Times & times   ///< Times for thread execution.
    );

    /** User override function for the main execution routine of the thread. A
       descendent class must provide the code that will be executed in the
       thread within this function.
       
       Note that the correct way for a thread to terminate is to return from
       this function.
     */
    virtual void Main() = 0;

    /** Get the currently running thread object instance. It is possible, even
       likely, that the smae code may be executed in the context of differenct
       threads. Under some circumstances it may be necessary to know what the
       current codes thread is and this static function provides that
       information.

       @return
       pointer to current thread.
     */
    static PThread * Current();

    /** Yield to another thread without blocking.
        This duplicates the implicit thread yield that may occur on some
        I/O operations or system calls.

        This may not be implemented on all platforms.
     */
    static void Yield();

    /**Create a simple thread executing the specified notifier.
       This creates a simple <code>PThread</code> class that automatically executes the
       function defined by the <code>PNotifier</code> in the context of a new thread.
      */
    static PThread * Create(
      const PNotifier & notifier,     ///< Function to execute in thread.
      INT parameter = 0,              ///< Parameter value to pass to notifier.
      AutoDeleteFlag deletion = AutoDeleteThread,
        ///< Automatically delete PThread instance on termination of thread.
      Priority priorityLevel = NormalPriority,  ///< Initial priority of thread.
      const PString & threadName = PString::Empty(), ///< The name of the thread (for Debug/Trace)
      PINDEX stackSize = 65536         ///< Stack size on some platforms
    );
    static PThread * Create(
      const PNotifier & notifier,     ///< Function to execute in thread.
      const PString & threadName      ///< The name of the thread (for Debug/Trace)
    ) { return Create(notifier, 0, NoAutoDeleteThread, NormalPriority, threadName); }
  //@}
  
    bool IsAutoDelete() const { return m_autoDelete; }

  private:
    PThread(bool isProcess);
    // Create a new thread instance as part of a <code>PProcess</code> class.

    friend class PProcess;
    friend class PExternalThread;
    friend class PHouseKeepingThread;
    // So a PProcess can get at PThread() constructor but nothing else.

    PThread(const PThread &) : PObject () { }
    // Empty constructor to prevent copying of thread instances.

    PThread & operator=(const PThread &) { return *this; }
    // Empty assignment operator to prevent copying of thread instances.

  protected:
    bool   m_isProcess;
    bool   m_autoDelete; // Automatically delete the thread on completion.
    PINDEX m_originalStackSize;

    PString m_threadName; // Give the thread a name for debugging purposes.
    PMutex  m_threadNameMutex;

    PThreadIdentifier m_threadId;
    PBoolean m_threadIdValid;

#if PTRACING
  public:
    struct TraceInfo {
      TraceInfo()
      { traceLevel = 0; traceBlockIndentLevel = 0; }

      PStack<PStringStream> traceStreams;
      unsigned traceLevel;
      unsigned traceBlockIndentLevel;
    };

#ifndef P_HAS_THREADLOCAL_STORAGE
  private:
    friend class PTrace;
    TraceInfo traceInfo;
#endif // P_HAS_THREADLOCAL_STORAGE
#endif // PTRACING

// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/thread.h"
#else
#include "unix/ptlib/thread.h"
#endif
};

// Include definition of platform dependent thread ID format
#if defined(_WIN32) && !defined(_WIN32_WCE)
  #define PTHREAD_ID_FMT "%u"
#else
  #define PTHREAD_ID_FMT "0x%lx"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4355)
#endif

/** Define some templates to simplify the declaration
  * of simple <code>PThread</code> descendants with one or two paramaters 
  */

/*
   This class automates calling a global function with no arguments within it's own thread.
   It is used as follows:

   void GlobalFunction()
   {
   }

   ...
   PString arg;
   new PThreadMain(&GlobalFunction)
 */
class PThreadMain : public PThread
{
  PCLASSINFO(PThreadMain, PThread);
  public:
    typedef void (*FnType)(); 
    PThreadMain(FnType function, bool autoDel = false)
      : PThread(10000, autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread)
      , m_function(function)
      { PThread::Resume(); }
    PThreadMain(const char * file, int line, FnType function, bool autoDel = false)
      : PThread(10000, autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread,  NormalPriority,
                psprintf("%s:%08x-%s:%i", GetClass(), (void *)this, file, line))
      , m_function(function)
      { PThread::Resume(); }
    virtual void Main()
      { (*m_function)(); }

  protected:
    FnType m_function;
};

/*
   This template automates calling a global function with one argument within it's own thread.
   It is used as follows:

   void GlobalFunction(PString arg)
   {
   }

   ...
   PString arg;
   new PThread1Arg<PString>(arg, &GlobalFunction)
 */
template<typename Arg1Type>
class PThread1Arg : public PThread
{
  PCLASSINFO(PThread1Arg, PThread);
  public:
    typedef void (*FnType)(Arg1Type arg1);

    PThread1Arg(Arg1Type arg1, FnType function, bool autoDel = false)
      : PThread(10000, autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread)
      , m_function(function)
      , m_arg1(arg1)
    { PThread::Resume(); }
    PThread1Arg(const char * file, int line, Arg1Type arg1, FnType function, bool autoDel = false)
      : PThread(10000, autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread, NormalPriority,
                psprintf("%s:%08x-%s:%i", GetClass(), (void *)this, file, line))
      , m_function(function)
      , m_arg1(arg1)
      { PThread::Resume(); }
    virtual void Main()
      { (*m_function)(m_arg1); }

  protected:
    FnType   m_function;
    Arg1Type m_arg1;
};


/*
   This template automates calling a global function with two arguments within it's own thread.
   It is used as follows:

   void GlobalFunction(PString arg1, int arg2)
   {
   }

   ...
   PString arg;
   new PThread2Arg<PString, int>(arg1, arg2, &GlobalFunction)
 */
template<typename Arg1Type, typename Arg2Type>
class PThread2Arg : public PThread
{
  PCLASSINFO(PThread2Arg, PThread);
  public:
    typedef void (*FnType)(Arg1Type arg1, Arg2Type arg2); 
    PThread2Arg(Arg1Type arg1, Arg2Type arg2, FnType function, bool autoDel = false)
      : PThread(10000, autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread)
      , m_function(function)
      , m_arg1(arg1)
      , m_arg2(arg2)
      { PThread::Resume(); }
    PThread2Arg(const char * file, int line, Arg1Type arg1, Arg2Type arg2, FnType function, bool autoDel = false)
      : PThread(10000, autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread, NormalPriority,
                psprintf("%s:%08x-%s:%i", GetClass(), (void *)this, file, line))
      , m_function(function)
      , m_arg1(arg1)
      , m_arg2(arg2)
      { PThread::Resume(); }
    virtual void Main()
      { (*m_function)(m_arg1, m_arg2); }

  protected:
    FnType   m_function;
    Arg1Type m_arg1;
    Arg2Type m_arg2;
};

/*
   This template automates calling a global function with three arguments within it's own thread.
   It is used as follows:

   void GlobalFunction(PString arg1, int arg2, int arg3)
   {
   }

   ...
   PString arg;
   new PThread3Arg<PString, int, int>(arg1, arg2, arg3, &GlobalFunction)
 */
template<typename Arg1Type, typename Arg2Type, typename Arg3Type>
class PThread3Arg : public PThread
{
  PCLASSINFO(PThread3Arg, PThread);
  public:
    typedef void (*FnType)(Arg1Type arg1, Arg2Type arg2, Arg3Type arg3); 
    PThread3Arg(Arg1Type arg1, Arg2Type arg2, Arg3Type arg3, FnType function, bool autoDel = false)
      : PThread(10000, autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread)
      , m_function(function)
      , m_arg1(arg1)
      , m_arg2(arg2)
      , m_arg3(arg3)
      { PThread::Resume(); }
    PThread3Arg(const char * file, int line, Arg1Type arg1, Arg2Type arg2, Arg3Type arg3, FnType function, bool autoDel = false)
      : PThread(10000, autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread, NormalPriority,
                psprintf("%s:%08x-%s:%i", GetClass(), (void *)this, file, line))
      , m_function(function)
      , m_arg1(arg1)
      , m_arg2(arg2)
      , m_arg3(arg3)
      { PThread::Resume(); }
    virtual void Main()
      { (*m_function)(m_arg1, m_arg2, m_arg3); }

  protected:
    FnType   m_function;
    Arg1Type m_arg1;
    Arg2Type m_arg2;
    Arg2Type m_arg3;
};

/*
   This template automates calling a member function with no arguments within it's own thread.
   It is used as follows:

   class Example {
     public:
      void Function()
      {
      }
   };

   ...
   Example ex;
   new PThreadObj<Example>(ex, &Example::Function)
 */

template <typename ObjType>
class PThreadObj : public PThread
{
    PCLASSINFO(PThreadObj, PThread);
  public:
    typedef void (ObjType::*ObjTypeFn)(); 

    PThreadObj(
      ObjType & obj,
      ObjTypeFn function,
      bool autoDel = false,
      const char * name = NULL,
      PThread::Priority priority = PThread::NormalPriority
    ) : PThread(10000,
                autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread,
                priority,
                name)
      , m_object(obj)
      , m_function(function)
    {
      PThread::Resume();
    }

    void Main()
    {
      (m_object.*m_function)();
    }

  protected:
    ObjType & m_object;
    ObjTypeFn m_function;
};


/*
   This template automates calling a member function with one argument within it's own thread.
   It is used as follows:

   class Example {
     public:
      void Function(PString arg)
      {
      }
   };

   ...
   Example ex;
   PString str;
   new PThreadObj1Arg<Example>(ex, str, &Example::Function)
 */
template <class ObjType, typename Arg1Type>
class PThreadObj1Arg : public PThread
{
    PCLASSINFO(PThreadObj1Arg, PThread);
  public:
    typedef void (ObjType::*ObjTypeFn)(Arg1Type); 

    PThreadObj1Arg(
      ObjType & obj,
      Arg1Type arg1,
      ObjTypeFn function,
      bool autoDel = false,
      const char * name = NULL,
      PThread::Priority priority = PThread::NormalPriority
    ) : PThread(10000,
                autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread,
                priority,
                name)
      , m_object(obj)
      , m_function(function)
      , m_arg1(arg1)
    {
      PThread::Resume();
    }

    void Main()
    {
      (m_object.*m_function)(m_arg1);
    }

  protected:
    ObjType & m_object;
    ObjTypeFn m_function;
    Arg1Type  m_arg1;
};

template <class ObjType, typename Arg1Type, typename Arg2Type>
class PThreadObj2Arg : public PThread
{
    PCLASSINFO(PThreadObj2Arg, PThread);
  public:
    typedef void (ObjType::*ObjTypeFn)(Arg1Type, Arg2Type);

    PThreadObj2Arg(
      ObjType & obj,
      Arg1Type arg1,
      Arg2Type arg2,
      ObjTypeFn function,
      bool autoDel = false,
      const char * name = NULL,
      PThread::Priority priority = PThread::NormalPriority
    ) : PThread(10000,
                autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread,
                priority,
                name)
      , m_object(obj)
      , m_function(function)
      , m_arg1(arg1)
      , m_arg2(arg2)
    {
      PThread::Resume();
    }

    void Main()
    {
      (m_object.*m_function)(m_arg1, m_arg2);
    }

  protected:
    ObjType & m_object;
    ObjTypeFn m_function;
    Arg1Type  m_arg1;
    Arg2Type  m_arg2;
};


///////////////////////////////////////////////////////////////////////////////
//
// PThreadLocalStorage
//

#ifdef _WIN32

#define P_HAS_THREADLOCAL_STORAGE 1

template <class Storage_T>
class PThreadLocalStorage
{
  public:
    typedef DWORD Key_T;
    typedef Storage_T value_type;

    PThreadLocalStorage()
    { key = TlsAlloc(); }

    ~PThreadLocalStorage()
    { TlsFree(key);  }

    Key_T GetKey() const
    { return key; }

    value_type * Get()
    { return (value_type *) TlsGetValue(key); }

    void Set(value_type * v)
    { TlsSetValue(key, (LPVOID)v); }

  protected:
    DWORD key;
};

#elif defined(P_PTHREADS)

#include <pthread.h>

#define P_HAS_THREADLOCAL_STORAGE 1

template <class Storage_T>
class PThreadLocalStorage
{
  public:
    typedef pthread_key_t Key_T;
    typedef Storage_T value_type;

    PThreadLocalStorage()
    { pthread_key_create(&key, NULL); }

    ~PThreadLocalStorage()
    { pthread_key_delete(key); }

    Key_T GetKey() const
    { return key; }

    value_type * Get()
    { return (value_type *)pthread_getspecific(key); }

    void Set(value_type * v)
    { pthread_setspecific(key, v); }

  private:
    Key_T key;
};

#else

#undef P_HAS_THREADLOCAL_STORAGE 1
#warning("Thread local storage not supported");

#endif


#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // PTLIB_THREAD_H

// End Of File ///////////////////////////////////////////////////////////////
