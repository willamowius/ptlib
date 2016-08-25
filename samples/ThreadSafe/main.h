/*
 * main.h
 *
 * PWLib application header file for ThreadSafe
 *
 * Copyright 2002 Equivalence
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _ThreadSafe_MAIN_H
#define _ThreadSafe_MAIN_H

#include <ptlib/pprocess.h>
#include <ptlib/safecoll.h>


class ThreadSafe;

class TestObject : public PSafeObject
{
    PCLASSINFO(TestObject, PSafeObject);
  public:
    TestObject(ThreadSafe & process, unsigned val);
    ~TestObject();

    Comparison Compare(const PObject & obj) const;
    void PrintOn(ostream & strm) const;

    ThreadSafe & process;
    unsigned value;
};


class ThreadSafe : public PProcess
{
  PCLASSINFO(ThreadSafe, PProcess)

  public:
    ThreadSafe();
    ~ThreadSafe();
    void Main();

  private:
    void Usage();

    void Test1(PArgList & args);
    void Test1Output();
    void Test1OutputEnd();
    PDECLARE_NOTIFIER(PThread, ThreadSafe, Test1Thread);

    void Test2(PArgList & args);
    PDECLARE_NOTIFIER(PThread, ThreadSafe, Test2Thread1);
    PDECLARE_NOTIFIER(PThread, ThreadSafe, Test2Thread2);

    void Test3(PArgList & args);
    PDECLARE_NOTIFIER(PThread, ThreadSafe, Test3Thread1);
    PDECLARE_NOTIFIER(PThread, ThreadSafe, Test3Thread2);

    PSafeList<TestObject> unsorted;
    PSafeSortedList<TestObject> sorted;
    PSafeDictionary<POrdinalKey, TestObject> sparse;

    PINDEX        threadCount;
    PTimeInterval startTick;
    PMutex        mutexObjects;
    unsigned      totalObjects;
    unsigned      currentObjects;

  friend class TestObject;
};


#endif  // _ThreadSafe_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
