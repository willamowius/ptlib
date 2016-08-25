/*
 * main.h
 *
 * PWLib application header file for SyncPoints
 *
 * Copyright 2009 Derek J Smithies
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _SyncPoints_MAIN_H
#define _SyncPoints_MAIN_H

#include <ptlib/pprocess.h>

#include <list>
class Runner;

/*! \mainpage  SyncPoints 

The purpose of this program is to find out which is faster:
\li windows
\li linux

when dealing with waiting on SyncPoints. PTLib provides the structure,
PSyncPoint. One programming approach is to have a thread wait on a
PSyncPoint instance. When the PSyncPoint instance is signalled, the
waiting thread should be activated to do things.

This program creates a PThread derivative, that contains a PSyncPoint instance.
There are N instances of the derivative. 

The first instance signals the second. The second instance signals the
third. And so on. The time taken to move the signalling along the
queue to end is record. Since this program runs on windows and linux,
we can compare the two times. Maybe, issues in linux scheduling can be
exposed..


Results so far indicate that a linux AMD dual core box will take 6
microseconds for a thread to respond to a syncpoint signal. this time
was recorded when the box was running at 100% cpu load, with both
processors doing a compile. When the machine was idle, the average
time per syncpoint was 4.7 microseconds.

If there are far fewer syncpoints made (just 1, loops of 10000) the
time was (unloaded box) 2.2 microseconds per sync point. With this
test set, and loading the box, the time was between 2 and 35
microseconds.

*/

typedef std::list<Runner *> RunnerList;

/**This is where all the activity happens. This class is launched on
   program startup, and does timing runs */
class SyncPoints : public PProcess
{
  PCLASSINFO(SyncPoints, PProcess)

  public:
  /**Constructor */
    SyncPoints();

    /**Destructor */
    ~SyncPoints();

    /**Program execution starts here */
    void Main();

    /**When all the elements have been processed, it calls this */
    void ListFinished();
    
    /**Simple counter of the number of signalled PSyncPoint classes
       there are.  This is used as a simple verification of the
       code. 

      The count in here is higher than expected. For 200 threads, 10
       loops, the count is 2010, which is correct. The reason is there
       are 201 sync points signalled per loop - the 200 threads and 1
       (allDone) in this class. */
    PAtomicInteger syncPointsSignalled;
 protected:
    
    /**The list of Runner Elements that do all the signaling */
    RunnerList list;
    
    /**The number of times we go over all the elements */
    PINDEX loops;
    
    /**The number of elements to put in the list of Runners */
    PINDEX size;
    
    /**The terminating sync point, which indicates all the threads
       were processed */
    PSyncPoint allDone;
};




/**This class is the core of the thing. It is a derivative of PThread,
   and contains 2 things - a pointer to the next thread, and and
   PSyncPoint. */
class Runner : public PThread
{
  PCLASSINFO(Runner, PThread);

 public:
  Runner(SyncPoints & app, Runner * nextThread, PINDEX id);

  /**Where the work in this thread happens; */
  virtual void Main();

  /**Where the other thread calls to activate us */
  void RunNow();
  
 protected:

  /**reference back to the master class */
  SyncPoints & app;

  /**The next thread, which we push to life when we are pushed to life */
  Runner *nextThread;

  /**The sync point we wait on */
  PSyncPoint syncPoint;
  
  /**The numerical ID of us */
  PINDEX id;
};



#endif  // _SyncPoints_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
