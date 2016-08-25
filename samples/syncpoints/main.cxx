/*
 * main.cxx
 *
 * PTLib application source file for testing speed of PSyncPoint class.
 *
 * Main program entry point.
 *
 * Copyright 2009 Derek J Smithies
 *
 * $Revision$
 * 
 *  You can read the comments to this program in the file main.h
 *
 *  In linux, you can get nice html docs by doing "make docs"
 *
 */

#include <ptlib.h>

using namespace std;

#include "main.h"


PCREATE_PROCESS(SyncPoints);

SyncPoints::SyncPoints()
  : PProcess("Derek Smithies code factory", "SyncPoints", 1, 0, AlphaCode, 1)
{
}

SyncPoints::~SyncPoints()
{
}

void SyncPoints::ListFinished()
{
  ++syncPointsSignalled;
  allDone.Signal(); 
}

void SyncPoints::Main()
{
  PArgList & args = GetArguments();
  args.Parse("l-loops:     "
	     "s-size:    "
	     "h-help.    "
#if PTRACING
             "o-output:"             "-no-output."
             "t-trace."              "-no-trace."
#endif
	     "v-version.    "
	     );

  if (args.HasOption('h')) {
    cout << "usage: map_dict "
         << endl
         << "     -l --loops #   : count of loops to run over the map/dicts  (1000)" << endl
	 << "     -s --size  #   : number of elements to pu in map/dict (200) " << endl
	 << "     -h --help      : Get this help message" << endl
	 << "     -v --version  : Get version information " << endl
#if PTRACING
         << "  -t --trace   : Enable trace, use multiple times for more detail" << endl
         << "  -o --output  : File for trace output, default is stderr" << endl
#endif
         << endl;
    return;
  }


  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);

  if (args.HasOption('v')) {
    cout << endl
         << "Product Name: " <<  (const char *)GetName() << endl
         << "Manufacturer: " <<  (const char *)GetManufacturer() << endl
         << "Version     : " <<  (const char *)GetVersion(PTrue) << endl
         << "System      : " <<  (const char *)GetOSName() << '-'
         <<  (const char *)GetOSHardware() << ' '
         <<  (const char *)GetOSVersion() << endl
         << endl;
    return;
  }

  loops = 1000;
  size  = 200;

  if (args.HasOption('l')) {
    PINDEX t = args.GetOptionString('l').AsInteger();
    if (t != 0) {
      cerr << " running " << t << " loops over the list of syncPoints " << endl;
      loops = t;
    }
  }

  if (args.HasOption('s')) {
    PINDEX t = args.GetOptionString('s').AsInteger();
    if (t != 0) {
      cerr << " running " << t << " Sync Point tests in a row " << endl;
      size = t;
    }
  }

  Runner *lastRunner = NULL;
  for (PINDEX i = 0; i < size; i++) {
    Runner * thisRunner = new Runner(*this, lastRunner, i);
    
    list.push_back(thisRunner);
    lastRunner = thisRunner;    
  }

  PTime a;
  for(PINDEX i = 0; i < loops; i++) {
    lastRunner->RunNow();
    allDone.Wait();
  }
  PTime b;
  PInt64 c = b.GetTimestamp() - a.GetTimestamp();
  PTimeInterval g = b-a;

  cerr << "Number of SyncPoint operations is " << syncPointsSignalled << endl << endl;

  cerr << "Elapsed time for " 
       << size << " elements, done "
       << loops << "x  is " 
       << c << " micseconds" << endl << endl;

  cerr << "Elapsed wall time for the test is " << g << " seconds" << endl << endl;

  cerr << "Time per syncpoint is " 
       << (c/(syncPointsSignalled * 1.00)) << " microseconds" << endl;
}

/////////////////////////////////////////////////////////////////////////////
Runner::Runner(SyncPoints & _app, Runner * _nextThread, PINDEX _id)
  :PThread(1000, NoAutoDeleteThread),
   app(_app),
   nextThread(_nextThread),
   id(_id)
{
  Resume();
}

void Runner::Main()
{
  for (;;) {
    syncPoint.Wait();

    //    cerr << id << endl;
    if (nextThread == NULL) 
      app.ListFinished();
    else
      nextThread->RunNow();
  }
}   

void Runner::RunNow() 
{ 
  syncPoint.Signal();   
  ++app.syncPointsSignalled; 
}
// End of File ///////////////////////////////////////////////////////////////
