/*
 * assert.cxx
 *
 * Function to implement assert clauses.
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

#define P_DISABLE_FACTORY_INSTANCES

#include <ptlib.h>
#include <ptlib/svcproc.h>


///////////////////////////////////////////////////////////////////////////////
// PProcess

#if defined(_WIN32)
#ifndef _WIN32_WCE
#include <imagehlp.h>

static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM thisProcess)
{
  char wndClassName[100];
  GetClassName(hWnd, wndClassName, sizeof(wndClassName));
  if (strcmp(wndClassName, "ConsoleWindowClass") != 0)
    return PTrue;

  DWORD wndProcess;
  GetWindowThreadProcessId(hWnd, &wndProcess);
  if (wndProcess != (DWORD)thisProcess)
    return PTrue;

  PTRACE(4, "PTLib\tAwaiting key press on exit.");
  cerr << "\nPress a key to continue . . .";
  cerr.flush();

  HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
  SetConsoleMode(in, ENABLE_PROCESSED_INPUT);
  FlushConsoleInputBuffer(in);
  char dummy;
  DWORD readBytes;
  ReadConsole(in, &dummy, 1, &readBytes, NULL);
  return PFalse;
}
#endif // _WIN32_WCE


void PProcess::WaitOnExitConsoleWindow()
{
#ifndef _WIN32_WCE
  if (!m_library)
    EnumWindows(EnumWindowsProc, GetCurrentProcessId());
#endif // _WIN32_WCE
}


#ifndef _WIN32_WCE
class PImageDLL : public PDynaLink
{
  PCLASSINFO(PImageDLL, PDynaLink)
  public:
    PImageDLL();

  PBoolean (__stdcall *SymInitialize)(
    IN HANDLE   hProcess,
    IN LPSTR    UserSearchPath,
    IN PBoolean     fInvadeProcess
    );
  PBoolean (__stdcall *SymCleanup)(
    IN HANDLE hProcess
    );
  DWORD (__stdcall *SymGetOptions)();
  DWORD (__stdcall *SymSetOptions)(
    DWORD options
    );
  DWORD (__stdcall *SymLoadModule)(
    HANDLE hProcess,
    HANDLE hFile,     
    PSTR   ImageName,  
    PSTR   ModuleName, 
    DWORD  BaseOfDll,  
    DWORD  SizeOfDll   
    );
  PBoolean (__stdcall *StackWalk)(
    DWORD                             MachineType,
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME                      StackFrame,
    LPVOID                            ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE          GetModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE        TranslateAddress
    );
  PBoolean (__stdcall *SymGetSymFromAddr)(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr,
    OUT PDWORD              pdwDisplacement,
    OUT PIMAGEHLP_SYMBOL    Symbol
    );

  PFUNCTION_TABLE_ACCESS_ROUTINE SymFunctionTableAccess;
  PGET_MODULE_BASE_ROUTINE       SymGetModuleBase;

  PBoolean (__stdcall *SymGetModuleInfo)(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr,
    OUT PIMAGEHLP_MODULE    ModuleInfo
    );
};


PImageDLL::PImageDLL()
  : PDynaLink("IMAGEHLP.DLL")
{
  if (!GetFunction("SymInitialize", (Function &)SymInitialize) ||
      !GetFunction("SymCleanup", (Function &)SymCleanup) ||
      !GetFunction("SymGetOptions", (Function &)SymGetOptions) ||
      !GetFunction("SymSetOptions", (Function &)SymSetOptions) ||
      !GetFunction("SymLoadModule", (Function &)SymLoadModule) ||
      !GetFunction("StackWalk", (Function &)StackWalk) ||
      !GetFunction("SymGetSymFromAddr", (Function &)SymGetSymFromAddr) ||
      !GetFunction("SymFunctionTableAccess", (Function &)SymFunctionTableAccess) ||
      !GetFunction("SymGetModuleBase", (Function &)SymGetModuleBase) ||
      !GetFunction("SymGetModuleInfo", (Function &)SymGetModuleInfo))
    Close();
}


#endif
#endif

void PAssertFunc(const char * msg)
{
  ostringstream str;
  str << msg;

#if defined(_WIN32) && defined(_M_IX86)
  PImageDLL imagehlp;
  if (imagehlp.IsLoaded()) {
    // Turn on load lines.
    imagehlp.SymSetOptions(imagehlp.SymGetOptions()|SYMOPT_LOAD_LINES);
    HANDLE hProcess;
    OSVERSIONINFO ver;
    ver.dwOSVersionInfoSize = sizeof(ver);
    ::GetVersionEx(&ver);
    if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT)
      hProcess = GetCurrentProcess();
    else
      hProcess = (HANDLE)GetCurrentProcessId();
    if (imagehlp.SymInitialize(hProcess, NULL, PTrue)) {
      HANDLE hThread = GetCurrentThread();
      // The thread information.
      CONTEXT threadContext;
      threadContext.ContextFlags = CONTEXT_FULL ;
      if (GetThreadContext(hThread, &threadContext)) {
        STACKFRAME frame;
        memset(&frame, 0, sizeof(frame));

#if defined (_M_IX86)
#define IMAGE_FILE_MACHINE IMAGE_FILE_MACHINE_I386
        frame.AddrPC.Offset    = threadContext.Eip;
        frame.AddrPC.Mode      = AddrModeFlat;
        frame.AddrStack.Offset = threadContext.Esp;
        frame.AddrStack.Mode   = AddrModeFlat;
        frame.AddrFrame.Offset = threadContext.Ebp;
        frame.AddrFrame.Mode   = AddrModeFlat;

#elif defined (_M_ALPHA)
#define IMAGE_FILE_MACHINE IMAGE_FILE_MACHINE_ALPHA
        frame.AddrPC.Offset = (unsigned long)threadContext.Fir;
        frame.AddrPC.Mode   = AddrModeFlat;
#else
#error ( "Unknown machine!" )
#endif

        int frameCount = 0;
        while (frameCount++ < 16 &&
               imagehlp.StackWalk(IMAGE_FILE_MACHINE,
                                  hProcess,
                                  hThread,
                                  &frame,
                                  &threadContext,
                                  NULL, // ReadMemoryRoutine
                                  imagehlp.SymFunctionTableAccess,
                                  imagehlp.SymGetModuleBase,
                                  NULL)) {
          if (frameCount > 1 && frame.AddrPC.Offset != 0) {
            char buffer[sizeof(IMAGEHLP_SYMBOL)+100];
            PIMAGEHLP_SYMBOL symbol = (PIMAGEHLP_SYMBOL)buffer;
            symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
            symbol->MaxNameLength = sizeof(buffer)-sizeof(IMAGEHLP_SYMBOL);
            DWORD displacement = 0;
            if (imagehlp.SymGetSymFromAddr(hProcess,
                                           frame.AddrPC.Offset,
                                           &displacement,
                                           symbol)) {
              str << "\n    " << symbol->Name;
            }
            else {
              str << "\n    0x"
                  << hex << setfill('0')
                  << setw(8) << frame.AddrPC.Offset
                  << dec << setfill(' ');
            }
            str << '(' << hex << setfill('0');
            for (PINDEX i = 0; i < PARRAYSIZE(frame.Params); i++) {
              if (i > 0)
                str << ", ";
              if (frame.Params[i] != 0)
                str << "0x";
              str << frame.Params[i];
            }
            str << setfill(' ') << ')';
            if (displacement != 0)
              str << " + 0x" << displacement;
          }
        }

        if (frameCount <= 2) {
          DWORD e = ::GetLastError();
          str << "\n    No stack dump: IMAGEHLP.DLL StackWalk failed: error=" << e;
        }
      }
      else {
        DWORD e = ::GetLastError();
        str << "\n    No stack dump: IMAGEHLP.DLL GetThreadContext failed: error=" << e;
      }

      imagehlp.SymCleanup(hProcess);
    }
    else {
      DWORD e = ::GetLastError();
      str << "\n    No stack dump: IMAGEHLP.DLL SymInitialise failed: error=" << e;
    }
  }
  else {
    DWORD e = ::GetLastError();
    str << "\n    No stack dump: IMAGEHLP.DLL could not be loaded: error=" << e;
  }
#endif

  str << ends;
  // Copy to local variable so char ptr does not become invalidated
  std::string sstr = str.str();

  if (PProcess::Current().IsServiceProcess()) {
#ifndef _WIN32_WCE
    PSYSTEMLOG(Fatal, sstr);
#if defined(_MSC_VER) && defined(_DEBUG) && !defined(_WIN64)
    if (PServiceProcess::Current().debugMode)
      __asm int 3;
#endif
#endif // !_WIN32_WCE
    return;
  }

  PTRACE(0, sstr);

#if defined(_WIN32)
  static HANDLE mutex = CreateSemaphore(NULL, 1, 1, NULL);
  WaitForSingleObject(mutex, INFINITE);
#endif

  if (PProcess::Current().IsGUIProcess()) {
    PVarString msg = sstr.c_str();
    PVarString name = PProcess::Current().GetName();
    switch (MessageBox(NULL, msg, name, MB_ABORTRETRYIGNORE|MB_ICONHAND|MB_TASKMODAL)) {
      case IDABORT :
#if !defined(_WIN32_WCE)
		  FatalExit(1);  // Never returns
#else
		  ExitProcess(1);
#endif // !_WIN32_WCE
      case IDRETRY :
        DebugBreak();
    }
#if defined(_WIN32)
    ReleaseSemaphore(mutex, 1, NULL);
#endif
    return;
  }

  for (;;) {
    cerr << sstr << "\n<A>bort, <B>reak, <I>gnore? ";
    cerr.flush();
    switch (cin.get()) {
      case 'A' :
      case 'a' :
        cerr << "Aborted" << endl;
        _exit(100);
        
      case 'B' :
      case 'b' :
        cerr << "Break" << endl;
#if defined(_WIN32)
        ReleaseSemaphore(mutex, 1, NULL);
#endif
#if defined(_MSC_VER) && !defined(_WIN32_WCE) && !defined(_WIN64)
        __asm int 3;
#endif

      case 'I' :
      case 'i' :
      case EOF :
        cerr << "Ignored" << endl;
#if defined(_WIN32)
        ReleaseSemaphore(mutex, 1, NULL);
#endif
        return;
    }
  }
}


// End Of File ///////////////////////////////////////////////////////////////
