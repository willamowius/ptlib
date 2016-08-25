/*
 * threadpool.h
 *
 * Generalised Thread Pooling functions
 *
 * Portable Tools Library
 *
 * Copyright (C) 2009 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Portions of this code were written with the financial assistance of 
 * Metreos Corporation (http://www.metros.com).
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */


#ifndef PTLIB_THREADPOOL_H
#define PTLIB_THREADPOOL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <map>
#include <queue>


/**

   These classes and templates implement a generic thread pooling mechanism

   There are two forms, low level and high level. For high level, it is assumed
   that there is a pool of threads each with a queue of work items to be
   processed. TO use simply decare a class containing the void Work() function
   and create the poolwith PQueuedThreadPool. e.g.

     class MyWork
     {
       void Work()
       {
         doIt();
       }
     }

     PQueuedThreadPool<MyWork> m_pool;

     m_pool.AddWork(new MyWork());


   To use low level, declare the following:

      - A class that describes a "unit" of work to be performed. 
 
      - A class that described a worker thread within the pool. This class must be a descendant of 
        PThreadPoolWorkerBase and must define the following member functions:
 
            Constructor with one parameter declared as "PThreadPoolBase & threadPool"
            unsigned GetWorkSize() const;
            void OnAddWork(work_unit *);
            void OnRemoveWork(work_unit *);

            void Shutdown();
            void Main();
 
      - A class that describes the thread pool itself. This is defined using PThreadPool template

 
   Example declarations:

      struct MyWorkUnit {
        PString work;
      };

      class MyWorkerThread : public PThreadPoolWorkerBase
      {
        public:
          MyWorkerThread(PThreadPoolBase & threadPool)
            : PThreadPoolWorkerBase(threadPool) { }

          void Main();
          void Shutdown();
          unsigned GetWorkSize() const;
          void OnAddWork(MyWorkUnit * work);
          void OnRemoveWork(MyWorkUnit * work);
      };

      
      class SIPMainThreadPool : public PThreadPool<MyWorkUnit, MyWorkerThread>
      {
        public:
          virtual PThreadPoolWorkerBase * CreateWorkerThread()
          { return new MyWorkerThread(*this); }
      };

    The worker thread member functions operate as follows:

       Constructor 
          Called whenever a new worker thread is required

       void Main()
          Called when the worker thread starts up

       unsigned GetWorkSize()
          Called whenever the thread pool wants to know how "busy" the
          thread is. This is used when deciding how to allocate new work units
             
       void OnAddWork(work_unit *)
          Called to add a new work unit to the thread

       void OnRemoveWork(work_unit *);
          Called to remove a work unit from the thread

       void Shutdown();
          Called to close down the worker thread

    The thread pool is used simply by instantiation as shown below. 

        MyThreadPool myThreadPool(10, 30);

    If the second parameter is zero, the first paramater sets the maximum number of worker threads that will be created.
    If the second parameter is not zero, this is the maximum number of work units each thread can handle. The first parameter
    is then the "quanta" in which worker threads will be allocated

    Once instantiated, the AddWork and RemoveWork member functions can be used to add and remove
    work units as required. The thread pool code will take care of starting, stopping and load balancing 
    worker threads as required.
   
 */

/** Base class for thread pools.
  */
class PThreadPoolBase : public PObject
{
  public:
    class WorkerThreadBase : public PThread
    {
      public:
        WorkerThreadBase(Priority priority = NormalPriority)
          : PThread(100, NoAutoDeleteThread, priority, "Pool")
          , m_shutdown(false)
        { }

        virtual void Shutdown() = 0;
        virtual unsigned GetWorkSize() const = 0;

        bool   m_shutdown;
        PMutex m_workerMutex;
    };

    class InternalWorkBase
    {
      public:
        InternalWorkBase(const char * group)
        { 
          if (group != NULL)
            m_group = group;
        }
        std::string m_group;
    };

    ~PThreadPoolBase();

    virtual WorkerThreadBase * CreateWorkerThread() = 0;
    virtual WorkerThreadBase * AllocateWorker();
    virtual WorkerThreadBase * NewWorker();

    unsigned GetMaxWorkers() const { return m_maxWorkerCount; }

    void SetMaxWorkers(
      unsigned count
    ) { m_maxWorkerCount = count; }

    unsigned GetMaxUnits() const { return m_maxWorkUnitCount; }

    void SetMaxUnits(
      unsigned count
    ) { m_maxWorkUnitCount = count; }

  protected:
    PThreadPoolBase(unsigned maxWorkerCount = 10, unsigned maxWorkUnitCount = 0);

    virtual bool CheckWorker(WorkerThreadBase * worker);
    void StopWorker(WorkerThreadBase * worker);
    PMutex m_listMutex;

    typedef std::vector<WorkerThreadBase *> WorkerList_t;
    WorkerList_t m_workers;

    unsigned m_maxWorkerCount;
    unsigned m_maxWorkUnitCount;
};


/** Low Level thread pool.
  */
template <class Work_T>
class PThreadPool : public PThreadPoolBase
{
  PCLASSINFO(PThreadPool, PThreadPoolBase);
  public:
    //
    //  constructor
    //
    PThreadPool(unsigned maxWorkers = 10, unsigned maxWorkUnits = 0)
      : PThreadPoolBase(maxWorkers, maxWorkUnits) 
    { }

    //
    // define the ancestor of the worker thread
    //
    class WorkerThread : public WorkerThreadBase
    {
      public:
        WorkerThread(PThreadPool & pool, Priority priority = NormalPriority)
          : WorkerThreadBase(priority)
          , m_pool(pool)
        {
        }

        virtual void AddWork(Work_T * work) = 0;
        virtual void RemoveWork(Work_T * work) = 0;
        virtual void Main() = 0;
  
      protected:
        PThreadPool & m_pool;
    };

    //
    // define internal worker wrapper class
    //
    class InternalWork : public InternalWorkBase
    {
      public:
        InternalWork(WorkerThread * worker, Work_T * work, const char * group)
          : InternalWorkBase(group)
          , m_worker(worker)
          , m_work(work)
        { 
        }

        WorkerThread * m_worker;
        Work_T * m_work;
    };

    //
    // define map for external work units to internal work
    //
    typedef std::map<Work_T *, InternalWork> ExternalToInternalWorkMap_T;
    ExternalToInternalWorkMap_T m_externalToInternalWorkMap;


    //
    // define class for storing group informationm
    //
    struct GroupInfo {
      unsigned m_count;
      WorkerThread * m_worker;
    };


    //
    //  define map for group ID to group information
    //
    typedef std::map<std::string, GroupInfo> GroupInfoMap_t;
    GroupInfoMap_t m_groupInfoMap;


    //
    //  add a new unit of work to a worker thread
    //
    bool AddWork(Work_T * work, const char * group = NULL)
    {
      PWaitAndSignal m(m_listMutex);

      // allocate by group if specified
      // else allocate to least busy
      WorkerThread * worker;
      if ((group == NULL) || (strlen(group) == 0)) {
        worker = (WorkerThread *)AllocateWorker();
      }
      else {

        // find the worker thread with the matching group ID
        // if no matching Id, then create a new thread
        typename GroupInfoMap_t::iterator g = m_groupInfoMap.find(group);
        if (g == m_groupInfoMap.end()) 
          worker = (WorkerThread *)AllocateWorker();
        else {
          worker = g->second.m_worker;
          PTRACE(4, "ThreadPool\tAllocated worker thread by group Id " << group);
        }
      }

      // if cannot allocate worker, return
      if (worker == NULL) 
        return false;

      // create internal work structure
      InternalWork internalWork(worker, work, group);

      // add work to external to internal map
      m_externalToInternalWorkMap.insert(typename ExternalToInternalWorkMap_T::value_type(work, internalWork));

      // add group ID to map
      if (!internalWork.m_group.empty()) {
        typename GroupInfoMap_t::iterator r = m_groupInfoMap.find(internalWork.m_group);
        if (r != m_groupInfoMap.end())
          ++r->second.m_count;
        else {
          GroupInfo info;
          info.m_count  = 1;
          info.m_worker = worker;
          m_groupInfoMap.insert(typename GroupInfoMap_t::value_type(internalWork.m_group, info));
        }
      }
      
      // give the work to the worker
      worker->AddWork(work);
    
      return true;
    }

    //
    //  remove a unit of work from a worker thread
    //
    bool RemoveWork(Work_T * work, bool removeFromWorker = true)
    {
      PWaitAndSignal m(m_listMutex);

      // find worker with work unit to remove
      typename ExternalToInternalWorkMap_T::iterator iterWork = m_externalToInternalWorkMap.find(work);
      if (iterWork == m_externalToInternalWorkMap.end())
        return false;

      InternalWork & internalWork = iterWork->second;

      // tell worker to stop processing work
      if (removeFromWorker)
        internalWork.m_worker->RemoveWork(work);

      // update group information
      if (!internalWork.m_group.empty()) {
        typename GroupInfoMap_t::iterator iterGroup = m_groupInfoMap.find(internalWork.m_group);
        PAssert(iterGroup != m_groupInfoMap.end(), "Attempt to find thread from unknown work group");
        if (iterGroup != m_groupInfoMap.end()) {
          if (--iterGroup->second.m_count == 0)
            m_groupInfoMap.erase(iterGroup);
        }
      }

      // see if workers need reorganising
      CheckWorker(internalWork.m_worker);

      // remove element from work unit map
      m_externalToInternalWorkMap.erase(iterWork);

      return true;
    }
};


/** High Level (queued work item) thread pool.
  */
template <class Work_T>
class PQueuedThreadPool : public PThreadPool<Work_T>
{
  public:
    //
    //  constructor
    //
    PQueuedThreadPool(unsigned maxWorkers = 10, unsigned maxWorkUnits = 0)
      : PThreadPool<Work_T>(maxWorkers, maxWorkUnits) 
    { }

    class QueuedWorkerThread : public PThreadPool<Work_T>::WorkerThread
    {
      public:
        QueuedWorkerThread(PThreadPool<Work_T> & pool, PThread::Priority priority = PThread::NormalPriority)
          : PThreadPool<Work_T>::WorkerThread(pool, priority)
          , m_available(0, INT_MAX)
        {
        }

        void AddWork(Work_T * work)
        {
          m_mutex.Wait();
          m_queue.push(work);
          m_available.Signal();
          m_mutex.Signal();
        }

        void RemoveWork(Work_T * )
        {
          m_mutex.Wait();
          Work_T * work = m_queue.front();
          m_queue.pop();
          m_mutex.Signal();
          delete work;
        }

        unsigned GetWorkSize() const
        {
          return (unsigned)m_queue.size();
        }

        void Main()
        {
          for (;;) {
            m_available.Wait();
            if (PThreadPool<Work_T>::WorkerThread::m_shutdown)
              break;

            m_mutex.Wait();
            Work_T * work = m_queue.empty() ? NULL : m_queue.front();
            m_mutex.Signal();

            if (work != NULL) {
              work->Work();
              PThreadPool<Work_T>::WorkerThread::m_pool.RemoveWork(work);
            }
          }
        }

        void Shutdown()
        {
          PThreadPool<Work_T>::WorkerThread::m_shutdown = true;
          m_available.Signal();
        }

      protected:
        typedef std::queue<Work_T *> Queue;
        Queue      m_queue;
        PMutex     m_mutex;
        PSemaphore m_available;
    };


    virtual PThreadPoolBase::WorkerThreadBase * CreateWorkerThread()
    { 
      return new QueuedWorkerThread(*this); 
    }
};


#endif // PTLIB_THREADPOOL_H


// End Of File ///////////////////////////////////////////////////////////////
