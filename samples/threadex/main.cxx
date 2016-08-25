/*
 * main.cxx
 *
 * PWLib application source file for threadex
 *
 * Main program entry point.
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

#include "precompile.h"
#include "main.h"
#include "version.h"


PCREATE_PROCESS(Threadex);

#include  <ptclib/dtmf.h>
#include  <ptclib/random.h>

#ifdef _WIN32
#include <process.h>
#endif

#include <set>

PAtomicInteger autoDeleteThreadsStarted;
PAtomicInteger autoDeleteThreadsFinished;

PMutex setMutex;
typedef std::set<PThreadIdentifer> ThreadSet;
ThreadSet threadSet;

Threadex::Threadex()
  : PProcess("Derek Smithies code factory", "threadex", MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
}

/*Note: This program uses either a busy wait system to check for
   thread termination, or the WaitForTermination method supplied by
   pwlib. It was found that with sufficient number of iterations,
   busywaiting, and a delay of 20ms on a SMP machine that segfaults
   occurred. This program was used to verify the correct close down
   and creation process of a thread */

void Threadex::Main()
{
  PArgList & args = GetArguments();

  args.Parse(
             "a-autodelete."         "-no-autodelete."
             "c-create."             "-no-create."
             "h-help."               "-no-help."
             "d-delay:"              "-no-delay."
             "b-busywait."           "-no-busywait."
#if PTRACING
             "o-output:"             "-no-output."
             "t-trace."              "-no-trace."
#endif
             "v-version."
  );

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  if (args.HasOption('v')) {
    cout << "Product Name: " << GetName() << endl
         << "Manufacturer: " << GetManufacturer() << endl
         << "Version     : " << GetVersion(PTrue) << endl
         << "System      : " << GetOSName() << '-'
         << GetOSHardware() << ' '
         << GetOSVersion() << endl;
    return;
  }

  if (args.HasOption('h')) {
    PError << "Available options are: " << endl         
           << "-h  or --help        :print this help" << endl
           << "-v  or --version      print version info" << endl
           << "-d  or --delay ##     where ## specifies how many milliseconds the created thread waits for" << endl
           << "-b  or --busywait     where if specified will cause the created thread to be tested for termination using a busy wait." << endl
           << "-a  or --autodelete   where the pwlib methods for auto deleting a thread are used" << endl
           << "-c  or --create       Use the pwlib PThread::Create method to create a thread of the reqired type" << endl
#if PTRACING
           << "o-output              output file name for trace" << endl
           << "t-trace.              trace level to use." << endl
#endif
           << endl
           << endl << endl;
    return;
  }

  delay = 2000;
  if (args.HasOption('d'))
    delay = args.GetOptionString('d').AsInteger();

  delay = PMIN(1000000, PMAX(0, delay));
  cout << "Created thread will wait for " << delay << " milliseconds before ending" << endl;
 
  doBusyWait = args.HasOption('b');

  doAutoDelete = args.HasOption('a');

  doCreate = args.HasOption('c');

  UserInterfaceThread ui;
  ui.Resume();
  ui.WaitForTermination();
}

/////////////////////////////////////////////////////////////////////////////

void AddToThreadSet(PThreadIdentifer id)
{
  PWaitAndSignal m(setMutex);
  if (threadSet.find(id) != threadSet.end()) {
    cout << "WARNING: duplicate thread id " << id << " in use" << endl;
    PTRACE(1, "Duplicate thread id " << id);
  }
  threadSet.insert(id);
}

void RemoveFromThreadSet(PThreadIdentifer id)
{
  PWaitAndSignal m(setMutex);
  if (threadSet.find(id) == threadSet.end()) {
    cout << "WARNING: unknown thread id " << id << " in use" << endl;
    PTRACE(1, "Unknown thread id " << id);
  }
  else {
    threadSet.erase(id);
  }
}

DelayThread::DelayThread(PINDEX _delay)
  : PThread(10000, NoAutoDeleteThread), delay(_delay)
{
  PTRACE(5, "ThreadEx\tConstructor for a non auto deleted delay thread");
}    

DelayThread::DelayThread(PINDEX _delay, PBoolean)
  : PThread(10000, AutoDeleteThread), delay(_delay)
{
  PTRACE(5, "ThreadEx\tConstructor for an auto deleted  delay thread");

  ++autoDeleteThreadsStarted;
}


DelayThread::~DelayThread()
{
  RemoveFromThreadSet(id);
  if (IsAutoDelete())
    ++autoDeleteThreadsFinished;
  //This thread must not have a PTRACE statement in the debugger, if it is an autodeleted thread.
  //If a PTRACE statement is here, the PTRACE will fail as the PThread::Current() returns empty.
}

void DelayThread::Main()  
{
  id = GetThreadId();
  AddToThreadSet(id);

  PTRACE(5, "ThreadEx\tDelayThread started");
  PThread::Sleep(delay);
  PTRACE(5, "ThreadEx\tDelayThread finished");
}


#ifdef _WIN32
unsigned _stdcall ExternalThreadMain(void *)
#else
void * ExternalThreadMain(void *)
#endif
{
  PTRACE(5, "ThreadEx\tExternal started");
  PThread::Sleep(5000);
  PTRACE(5, "ThreadEx\tExternal finished");
  return 0;
}


///////////////////////////////////////////////////////////////////////////

void LauncherThread::AutoCreatedMain(PThread &, INT param)
{
  PThread::Sleep(param);
}

void LauncherThread::Main()
{
  PINDEX delay = Threadex::Current().Delay();
  PBoolean   doCreate = Threadex::Current().Create();

  PThread *thread = NULL;
  if (Threadex::Current().AutoDelete()) {
    while (keepGoing) {
      if (doCreate) {
        thread = PThread::Create(PCREATE_NOTIFIER(AutoCreatedMain), delay,
                                     PThread::AutoDeleteThread,
                                     PThread::NormalPriority,
                                     "auto deleted %X");
      } else {
        thread = new DelayThread(delay, PTrue);
        thread->Resume();
      }
      //     PThread::Sleep(1);
      iteration++;
    }
    return;
  }

  if (Threadex::Current().BusyWait()) {
    while (keepGoing) {
      if (doCreate) {
        thread = PThread::Create(PCREATE_NOTIFIER(AutoCreatedMain), delay,
                                 PThread::NoAutoDeleteThread,
                                 PThread::NormalPriority,
                                 "auto BusyWaited %X");
      } else {
        thread = new DelayThread(delay);
        thread->Resume();
      }
      
      while (!thread->IsTerminated());
      delete thread;
      iteration++;
    }
    return;
  }

  while (keepGoing) {
    if (doCreate) {
      thread = PThread::Create(PCREATE_NOTIFIER(AutoCreatedMain), delay,
                               PThread::NoAutoDeleteThread,
                               PThread::NormalPriority,
                               "auto WaitForTermination %X");
    } else {
      thread = new DelayThread(delay);
      thread->Resume();
    }
    thread->WaitForTermination();
    delete thread;
    iteration++;
  }
}

void UserInterfaceThread::Main()
{
  cout << "This program will repeatedly create and destroy a thread until terminated from the console" << endl;

  static const char help[] = "Press : \n"
                             "         D      average Delay time of each thread\n"
                             "         H or ? help\n"
                             "         R      report count of threads done\n"
                             "         T      time elapsed\n"
                             "         C      create non-PTLib thread\n"
                             "         X or Q exit\n";
  cout << endl << help;

  LauncherThread launch;
  launch.Resume();

  while (!cin.eof()) {
    cout << "Command ? " << flush;
    PString cmd;
    cin >> cmd;

    switch (tolower(cmd[0])) {
    case 'd' :
      {
        int i = launch.GetIteration();
        if (i == 0) {
          cout << "Have not completed an iteration yet, so time per iteration is unavailable" << endl;
        } else {
          cout << "Average time per iteration is " << (launch.GetElapsedTime().GetMilliSeconds()/((double) i)) 
               << " milliseconds" << endl;
        }
      }
      break;

    case 'r' :
      cout << "\nHave completed " << launch.GetIteration() << " iterations" << endl;
      {
        int a = autoDeleteThreadsStarted;
        int b = autoDeleteThreadsFinished;
        cout << "Auto-delete threads : " << a << " - " << b << " = " << a-b << endl;
      }
      break;

    case 't' :
      cout << "\nElapsed time is " << launch.GetElapsedTime() << " (Hours:mins:seconds.millseconds)" << endl;
      break;

    case 'c' :
      {
#ifdef _WIN32
        unsigned threadId;
        _beginthreadex(NULL, 10000, ExternalThreadMain, NULL, 0, &threadId);
#elif P_PTHREADS
        pthread_t threadId;
        pthread_create(&threadId, NULL, ExternalThreadMain, NULL);
#else
        cout << "\nCreate external thread unsupported" << endl;
#endif
      }
      break;

    case 'x' :
    case 'q' :
      cout << "Exiting." << endl;
      launch.Terminate();
      launch.WaitForTermination();
      return;

    case '?' :
    case 'h' :
      cout << help << endl;

    default:
      break;
    } // end switch
  } // end for
}


// End of File ///////////////////////////////////////////////////////////////
