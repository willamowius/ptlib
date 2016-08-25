/*
 * pprocess.h
 *
 * Operating System process (running program) class.
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

PDICTIONARY(PXFdDict, POrdinalKey, PThread);

///////////////////////////////////////////////////////////////////////////////
// PProcess

  public:
    friend class PApplication;
    friend class PServiceProcess;
    friend void PXSignalHandler(int);
    friend class PHouseKeepingThread;
    friend PString PX_GetThreadName(pthread_t id);

    ~PProcess();

    PDirectory PXGetHomeDir ();

    friend void PXSigHandler(int);
    virtual void PXOnSignal(int);
    virtual void PXOnAsyncSignal(int);
    void         PXCheckSignals();

    static void PXShowSystemWarning(PINDEX code);
    static void PXShowSystemWarning(PINDEX code, const PString & str);

  protected:
    void         CommonConstruct();
    void         CommonDestruct();

    virtual void _PXShowSystemWarning(PINDEX code, const PString & str);
    int pxSignals;

  protected:
    void CreateConfigFilesDictionary();
    PAbstractDictionary * configFiles;


#if defined(P_PTHREADS) || defined(P_MAC_MPTHREADS) || defined (__BEOS__)

  public:
    bool SignalTimerChange();
    PBoolean PThreadKill(pthread_t id, unsigned signal);
    void PXSetThread(pthread_t id, PThread * thread);

  protected:
    PSyncPoint breakBlock;
    class PHouseKeepingThread * housekeepingThread;
    PMutex housekeepingMutex;

#elif defined(VX_TASKS)

  public:
    bool SignalTimerChange();

  private:
    PLIST(ThreadList, PThread);
    ThreadList autoDeleteThreads;
    PMutex deleteThreadMutex;

    PDECLARE_CLASS(HouseKeepingThread, PThread)
        public:
        HouseKeepingThread();
        void Main();
        PSyncPoint breakBlock;
    };
    friend class HouseKeepingThread;
    HouseKeepingThread * houseKeeper;
    // Thread for doing timers, thread clean up etc.

  friend PThread * PThread::Current();
  friend int PThread::ThreadFunction(void * thread);


#else

  public:
    void PXAbortIOBlock(int fd);
  protected:
    PXFdDict     ioBlocks[3];
#endif

// End Of File ////////////////////////////////////////////////////////////////
