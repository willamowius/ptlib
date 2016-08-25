/*
 * svcproc.cxx
 *
 * Service process (daemon) implementation.
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

#include <ptlib.h>

#pragma implementation "svcproc.h"
#include <ptlib/svcproc.h>

#ifdef P_VXWORKS
#include <logLib.h>
#define LOG_EMERG      0
#define LOG_ALERT      1
#define LOG_CRIT      2
#define LOG_ERR        3
#define LOG_WARNING      4
#define  LOG_NOTICE      5
#define LOG_INFO      6
#define LOG_DEBUG      7
#else
#include <syslog.h>
#include <pwd.h>
#include <grp.h>
#endif

#include <stdarg.h>
#if (__GNUC__ >= 3)
#include <fstream>
#else
#include <fstream.h>
#endif
#include <signal.h>

#include "uerror.h"

#ifdef P_LINUX
#include <sys/resource.h>
#endif

#define new PNEW

extern void PXSignalHandler(int);

#define  MAX_LOG_LINE_LEN  1024

static const char * const PLevelName[PSystemLog::NumLogLevels+1] = {
  "Message",
  "Fatal error",
  "Error",
  "Warning",
  "Info",
  "Debug",
  "Debug2",
  "Debug3",
  "Debug4",
  "Debug5",
  "Debug6",
};

PServiceProcess::PServiceProcess(const char * manuf,
                                 const char * name,
                                         WORD majorVersion,
                                         WORD minorVersion,
                                   CodeStatus status,
                                         WORD buildNumber)
  : PProcess(manuf, name, majorVersion, minorVersion, status, buildNumber)
{
  isTerminating = PFalse;
}


PServiceProcess::~PServiceProcess()
{
  PSetErrorStream(NULL);
#if PTRACING
  PTrace::SetStream(NULL);
  PTrace::ClearOptions(PTrace::SystemLogStream);
#endif

  if (!pidFileToRemove)
    PFile::Remove(pidFileToRemove);
}


PServiceProcess & PServiceProcess::Current()
{
  PProcess & process = PProcess::Current();
  PAssert(PIsDescendant(&process, PServiceProcess), "Not a service process!");
  return (PServiceProcess &)process;
}


#ifndef P_VXWORKS
static int KillProcess(int pid, int sig)
{
  if (kill(pid, sig) != 0)
    return -1;

  cout << "Sent SIG";
  if (sig == SIGTERM)
    cout << "TERM";
  else
    cout << "KILL";
  cout << " to daemon at pid " << pid << ' ' << flush;

  for (PINDEX retry = 1; retry <= 10; retry++) {
    PThread::Sleep(1000);
    if (kill(pid, 0) != 0) {
      cout << "\nDaemon stopped." << endl;
      return 0;
    }
    cout << '.' << flush;
  }
  cout << "\nDaemon has not stopped." << endl;

  return 1;
}
#endif // !P_VXWORKS


void PServiceProcess::_PXShowSystemWarning(PINDEX code, const PString & str)
{
  PSYSTEMLOG(Warning, "PWLib\t" << GetOSClass() << " error #" << code << '-' << str);
}


int PServiceProcess::InitialiseService()
{
#ifndef P_VXWORKS
#if PMEMORY_CHECK
  PMemoryHeap::SetIgnoreAllocations(PTrue);
#endif
  PSetErrorStream(new PSystemLog(PSystemLog::StdError));
#if PTRACING
  PTrace::SetStream(new PSystemLog(PSystemLog::Debug3));
  PTrace::ClearOptions(PTrace::FileAndLine);
  PTrace::SetOptions(PTrace::SystemLogStream);
  PTrace::SetLevel(4);
#endif
#if PMEMORY_CHECK
  PMemoryHeap::SetIgnoreAllocations(PFalse);
#endif
  debugMode = PFalse;

  // parse arguments so we can grab what we want
  PArgList & args = GetArguments();

  args.Parse("v-version."
             "d-daemon."
             "c-console."
             "h-help."
             "x-execute."
             "p-pid-file:"
       "H-handlemax:"
             "i-ini-file:"
             "k-kill."
             "t-terminate."
             "s-status."
             "l-log-file:"
             "u-uid:"
             "g-gid:"
             "C-core-size:"
           , false);

  // if only displaying version information, do it and finish
  if (args.HasOption('v')) {
    cout << "Product Name: " << productName << endl
         << "Manufacturer: " << manufacturer << endl
         << "Version     : " << GetVersion(PTrue) << endl
         << "System      : " << GetOSName() << '-'
                             << GetOSHardware() << ' '
                             << GetOSVersion() << endl;
    return 0;
  }

  PString pidfilename;
  if (args.HasOption('p'))
    pidfilename = args.GetOptionString('p');
#ifdef _PATH_VARRUN
  else
    pidfilename =  _PATH_VARRUN;
#endif

  if (!pidfilename && PDirectory::Exists(pidfilename))
    pidfilename = PDirectory(pidfilename) + PProcess::Current().GetFile().GetFileName() + ".pid";

  if (args.HasOption('k') || args.HasOption('t') || args.HasOption('s')) {
    pid_t pid;

    {
      ifstream pidfile(pidfilename);
      if (!pidfile.is_open()) {
        cout << "Could not open pid file: \"" << pidfilename << "\""
                " - " << strerror(errno) << endl;
        return 1;
      }

      pidfile >> pid;
      if (pid == 0) {
        cout << "Illegal format pid file \"" << pidfilename << '"' << endl;
        return 1;
      }
    }

    if (args.HasOption('s')) {
      cout << "Process at " << pid << ' ';
      if (kill(pid, 0) == 0)
        cout << "is running.";
      else if (errno == ESRCH)
        cout << "does not exist.";
      else
        cout << " status could not be determined, error: " << strerror(errno);
      cout << endl;
      return 0;
    }

    int sig = args.HasOption('t') ? SIGTERM : SIGKILL;
    switch (KillProcess(pid, sig)) {
      case -1 :
        break;
      case 0 :
        PFile::Remove(pidfilename);
        return 0;
      case 1 :
        if (args.HasOption('t') && args.HasOption('k')) {
          switch (KillProcess(pid, SIGKILL)) {
            case -1 :
              break;
            case 0 :
              PFile::Remove(pidfilename);
              return 0;
            case 1 :
              return 2;
          }
        }
        else
          return 2;
    }

    cout << "Could not stop process " << pid <<
            " - " << strerror(errno) << endl;
    return 1;
  }

  // Set the gid we are running under
  if (args.HasOption('g')) {
    PString gidstr = args.GetOptionString('g');
    if (!SetGroupName(gidstr)) {
      cout << "Could not set GID to \"" << gidstr << "\" - " << strerror(errno) << endl;
      return 1;
    }
  }

  // Set the uid we are running under
  if (args.HasOption('u')) {
    PString uidstr = args.GetOptionString('u');
    if (!SetUserName(uidstr)) {
      cout << "Could not set UID to \"" << uidstr << "\" - " << strerror(errno) << endl;
      return 1;
    }
  }

  PBoolean helpAndExit = PFalse;

  // if displaying help, then do it
  if (args.HasOption('h')) 
    helpAndExit = PTrue;
  else if (!args.HasOption('d') && !args.HasOption('x')) {
    cout << "error: must specify one of -v, -h, -t, -k, -d or -x" << endl;
    helpAndExit = PTrue;
  }

  // set flag for console messages
  if (args.HasOption('c')) {
    PSystemLog::SetTarget(new PSystemLogToStderr());
    debugMode = PTrue;
  }

  if (args.HasOption('l')) {
    PFilePath fileName = args.GetOptionString('l');
    if (fileName.IsEmpty()) {
      cout << "error: must specify file name for -l" << endl;
      helpAndExit = PTrue;
    }
    else if (PDirectory::Exists(fileName))
      fileName = PDirectory(fileName) + PProcess::Current().GetFile().GetFileName() + ".log";
    PSystemLog::SetTarget(new PSystemLogToFile(fileName));
  }

  if (helpAndExit) {
    cout << "usage: [-c] -v|-d|-h|-x\n"
            "  -h --help           output this help message and exit\n"
            "  -v --version        display version information and exit\n"
#if !defined(BE_THREADS) && !defined(P_RTEMS)
            "  -d --daemon         run as a daemon\n"
#endif
            "  -u --uid uid        set user id to run as\n"
            "  -g --gid gid        set group id to run as\n"
            "  -p --pid-file       name or directory for pid file\n"
            "  -t --terminate      orderly terminate process in pid file\n"
            "  -k --kill           preemptively kill process in pid file\n"
            "  -s --status         check to see if daemon is running\n"
            "  -c --console        output messages to stdout rather than syslog\n"
            "  -l --log-file file  output messages to file or directory instead of syslog\n"
            "  -x --execute        execute as a normal program\n"
            "  -i --ini-file       set the ini file to use, may be explicit file or\n"
            "                      a ':' separated set of directories to search.\n"
            "  -H --handlemax n    set maximum number of file handles (set before uid/gid)\n"
#ifdef P_LINUX
            "  -C --core-size      set the maximum core file size\n"
#endif
         << endl;
    return 0;
  }

  // open the system logger for this program
  PSYSTEMLOG(StdError, "Starting service process \"" << GetName() << "\" v" << GetVersion(PTrue));

  if (args.HasOption('i'))
    SetConfigurationPath(PFilePath(args.GetOptionString('i')));

  if (args.HasOption('H')) {
    int uid = geteuid();
    seteuid(getuid()); // Switch back to starting uid for next call
    SetMaxHandles(args.GetOptionString('H').AsInteger());
    seteuid(uid);
  }

  // set the core file size
  if (args.HasOption('C')) {
#ifdef P_LINUX
    struct rlimit rlim;
    if (getrlimit(RLIMIT_CORE, &rlim) != 0) 
      cout << "Could not get current core file size : error = " << errno << endl;
    else {
      int uid = geteuid();
      seteuid(getuid()); // Switch back to starting uid for next call
      int v = args.GetOptionString('C').AsInteger();
      rlim.rlim_cur = v;
      if (setrlimit(RLIMIT_CORE, &rlim) != 0) 
        cout << "Could not set current core file size to " << v << " : error = " << errno << endl;
      else {
        getrlimit(RLIMIT_CORE, &rlim);
        cout << "Core file size set to " << rlim.rlim_cur << "/" << rlim.rlim_max << endl;
      }
      seteuid(uid);
    }
#endif
  }

#if !defined(BE_THREADS) && !defined(P_RTEMS)
  if (!args.HasOption('d'))
    return -1;

  // Run as a daemon, ie fork

  if (!pidfilename) {
    ifstream pidfile(pidfilename);
    if (pidfile.is_open()) {
      pid_t pid;
      pidfile >> pid;
      if (pid != 0 && kill(pid, 0) == 0) {
        cout << "Already have daemon running with pid " << pid << endl;
        return 2;
      }
    }
  }

  // Need to get rid of the config write thread before fork, as on
  // pthreads platforms the forked process does not have the thread
  CommonDestruct();

  pid_t pid = fork();
  switch (pid) {
    case 0 : // The forked process
      break;

    case -1 : // Failed
      cout << "Fork failed creating daemon process." << endl;
      return 1;

    default : // Parent process
      cout << "Daemon started with pid " << pid << endl;
      if (!pidfilename) {
        // Write out the child pid to magic file in /var/run (at least for linux)
        ofstream pidfile(pidfilename);
        if (pidfile.is_open())
          pidfile << pid;
        else
          cout << "Could not write pid to file \"" << pidfilename << "\""
                  " - " << strerror(errno) << endl;
      }
      return 0;
  }

  // Set ourselves as out own process group so we don't get signals
  // from our parent's terminal (hopefully!)
  PSETPGRP();

  CommonConstruct();

  pidFileToRemove = pidfilename;

  // Only if we are running in the background as a daemon, we intercept
  // the core dumping signals so get a message in the log file.
  signal(SIGSEGV, PXSignalHandler);
  signal(SIGFPE, PXSignalHandler);
  signal(SIGBUS, PXSignalHandler);

  // Also if in background, don't want to get blocked on input from stdin
  ::close(STDIN_FILENO);

#endif // !BE_THREADS && !P_RTEMS
#endif // !P_VXWORKS
  return -1;
}

int PServiceProcess::InternalMain(void *)
{
  if ((terminationValue = InitialiseService()) < 0) {
    // Make sure housekeeping thread is going so signals are handled.
    SignalTimerChange();

    terminationValue = 1;
    if (OnStart()) {
      terminationValue = 0;
      Main();
      Terminate();
    }
  }

  return terminationValue;
}


PBoolean PServiceProcess::OnPause()
{
  return PTrue;
}

void PServiceProcess::OnContinue()
{
}

void PServiceProcess::OnStop()
{
}


void PServiceProcess::Terminate()
{
  if (isTerminating) {
    // If we are the process itself and another thread is terminating us,
    // just stop and wait forever for us to go away
    if (PThread::Current() == this)
      Sleep(PMaxTimeInterval);
    PSYSTEMLOG(Error, "Nested call to process termination!");
    return;
  }

  isTerminating = PTrue;

  PSYSTEMLOG(Warning, "Stopping service process \"" << GetName() << "\" v" << GetVersion(PTrue));

  // Avoid strange errors caused by threads (and the process itself!) being destoyed 
  // before they have EVER been scheduled
  Yield();

  // Do the services stop code
  OnStop();

  PSystemLog::SetTarget(NULL);

  // Now end the program
  _exit(terminationValue);
}

void PServiceProcess::PXOnAsyncSignal(int sig)
{
  const char * sigmsg;

  // Override the default behavious for these signals as that just
  // summarily exits the program. Allow PXOnSignal() to do orderly exit.

  switch (sig) {
    case SIGINT :
    case SIGTERM :
    case SIGHUP :
      return;

    case SIGSEGV :
      sigmsg = "segmentation fault (SIGSEGV)";
      break;

    case SIGFPE :
      sigmsg = "floating point exception (SIGFPE)";
      break;

#ifndef __BEOS__ // In BeOS, SIGBUS is the same value as SIGSEGV
    case SIGBUS :
      sigmsg = "bus error (SIGBUS)";
      break;
#endif
    default :
      PProcess::PXOnAsyncSignal(sig);
      return;
  }

  signal(SIGSEGV, SIG_DFL);
  signal(SIGFPE, SIG_DFL);
  signal(SIGBUS, SIG_DFL);

  static PBoolean inHandler = PFalse;
  if (inHandler) {
    raise(SIGQUIT); // Dump core
    _exit(-1); // Fail safe if raise() didn't dump core and exit
  }

  inHandler = PTrue;

  PThreadIdentifier tid = GetCurrentThreadId();
  ThreadMap::iterator thread = m_activeThreads.find(tid);

  char msg[200];
  sprintf(msg, "\nCaught %s, thread_id=" PTHREAD_ID_FMT, sigmsg, tid);

  if (thread != m_activeThreads.end()) {
    PString thread_name = thread->second->GetThreadName();
    if (thread_name.IsEmpty())
      sprintf(&msg[strlen(msg)], " obj_ptr=%p", thread->second);
    else {
      strcat(msg, " name=");
      strcat(msg, thread_name);
    }
  }

  strcat(msg, ", aborting.\n");

  PSYSTEMLOG(Fatal, msg);

  raise(SIGQUIT); // Dump core
  _exit(-1); // Fail safe if raise() didn't dump core and exit
}


void PServiceProcess::PXOnSignal(int sig)
{
  PProcess::PXOnSignal(sig);
  switch (sig) {
    case SIGINT :
    case SIGTERM :
      Terminate();
      break;

    case SIGUSR1 :
      OnPause();
      break;

    case SIGUSR2 :
      OnContinue();
      break;

#if 0
    case SIGHUP :
      if (currentLogLevel < PSystemLog::NumLogLevels-1) {
        currentLogLevel = (PSystemLog::Level)(currentLogLevel+1);
        PSystemLog s(PSystemLog::StdError);
        s << "Log level increased to " << PLevelName[currentLogLevel+1];
      }
      break;

#ifdef SIGWINCH
    case SIGWINCH :
      if (currentLogLevel > PSystemLog::Fatal) {
        currentLogLevel = (PSystemLog::Level)(currentLogLevel-1);
        PSystemLog s(PSystemLog::StdError);
        s << "Log level decreased to " << PLevelName[currentLogLevel+1];
      }
      break;
#endif
#endif
  }
}

