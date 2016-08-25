/*
 * main.h
 *
 * PWLib application header file for threadex
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _Threadex_MAIN_H
#define _Threadex_MAIN_H

#include <ptlib/pprocess.h>

/**This class is a simple simple thread that just creates, waits a
   period of time, and exits.It is designed to test the PwLib methods
   for reporting the status of a thread. This class will be created
   over and over- millions of times is possible if left long
   enough. If the pwlib thread status functions are broken, a segfault
   will result. Past enxperience has found a fault in pwlib with the
   BusyWait option on, with SMP machines and a delay period of 20ms */
class DelayThread : public PThread
{
  PCLASSINFO(DelayThread, PThread);
  
public:
  DelayThread(PINDEX _delay);
    
  DelayThread(PINDEX _delay, PBoolean);
  
  ~DelayThread();

  void Main();
    
 protected:
  PINDEX delay;
  PThreadIdentifer id;
};

////////////////////////////////////////////////////////////////////////////////
/**This thread handles the Users console requests to query the status of 
   the launcher thread. It provides a means for the user to close down this
   program - without having to use Ctrl-C*/
class UserInterfaceThread : public PThread
{
  PCLASSINFO(UserInterfaceThread, PThread);
  
public:
  UserInterfaceThread()
    : PThread(10000, NoAutoDeleteThread)
    { }

  void Main();
    
 protected:
};

///////////////////////////////////////////////////////////////////////////////
/**This thread launches multiple instances of the BusyWaitThread. Each
   thread launched is busy monitored for termination. When the thread
   terminates, the thread is deleted, and a new one is created. This
   process repeats until segfault or termination by the user */
class LauncherThread : public PThread
{
  PCLASSINFO(LauncherThread, PThread);
  
public:
  LauncherThread()
    : PThread(10000, NoAutoDeleteThread)
    { iteration = 0; keepGoing = PTrue; }
  
  void Main();
    
  PINDEX GetIteration() { return iteration; }

  virtual void Terminate() { keepGoing = PFalse; }

  PTimeInterval GetElapsedTime() { return PTime() - startTime; }

  PDECLARE_NOTIFIER(PThread, LauncherThread, AutoCreatedMain);

 protected:
  PINDEX iteration;
  PTime startTime;
  PBoolean  keepGoing;
};

////////////////////////////////////////////////////////////////////////////////


class Threadex : public PProcess
{
  PCLASSINFO(Threadex, PProcess)

  public:
    Threadex();
    virtual void Main();

    PINDEX Delay()    { return delay; }

    PBoolean AutoDelete() { return doAutoDelete; }

    PBoolean BusyWait()   { return doBusyWait; }

    PBoolean Create()     { return doCreate; }

   static Threadex & Current()
      { return (Threadex &)PProcess::Current(); }

 protected:

    PINDEX delay;

    PBoolean doAutoDelete;

    PBoolean doBusyWait;

    PBoolean doCreate;
};



#endif  // _Threadex_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
