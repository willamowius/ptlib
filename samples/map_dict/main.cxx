/*
 * main.cxx
 *
 * PWLib application source file for map dictionary compparison.
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
#include <ptclib/random.h>

using namespace std;

#include "main.h"


ElementMap elementMap;

ElementDict elementDict;

ElementIntMap elementIntMap;

ElementIntDict elementIntDict;


PCREATE_PROCESS(MapDictionary);

MapDictionary::MapDictionary()
  : PProcess("Derek Smithies code factory", "MapDictionary", 1, 0, AlphaCode, 1)
{
}

MapDictionary::~MapDictionary()
{
}

void MapDictionary::Main()
{
  PArgList & args = GetArguments();
  args.Parse("l-loops:"
	     "s-size:"
	     "h-help."
#if PTRACING
             "o-output:"
             "t-trace."
#endif
	     "v-version."
	     );

  if (args.HasOption('l'))
    cerr << "user has option l " << endl;

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
      cerr << " running " << t << " loops over the map dictionary " << endl;
      loops = t;
    }
  }

  if (args.HasOption('s')) {
    PINDEX t = args.GetOptionString('s').AsInteger();
    if (t != 0) {
      cerr << " running " << t << " elements in the  map dictionary " << endl;
      size = t;
    }
  }

  elementDict.DisallowDeleteObjects();
  elementIntDict.DisallowDeleteObjects();
  

  PRandom random;
  PINDEX thisIntKey;
  PStringStream thisKey, nextKey;
  thisIntKey = random.Generate();
  thisKey << ::hex << thisIntKey;
  firstKey = thisKey;
  firstIntKey = thisIntKey;
  PINDEX nextIntKey;

  for (PINDEX i = 0; i < size; i++) {
    Element *e = new Element;
    nextKey.MakeEmpty();
    PINDEX tKey = random.Generate();
    nextKey << ::hex << tKey;
    nextIntKey = tKey;

    e->thisKey = psprintf("%s", thisKey.GetPointer());;
    e->thisIntKey = thisIntKey;
    e->contents = PString(i);
    e->nextKey = psprintf("%s", nextKey.GetPointer());
    e->nextIntKey = nextIntKey;

    elementMap[thisKey] = e;
    elementIntMap[thisIntKey] = e;
    elementDict.SetAt(e->thisKey, e);
    elementIntDict.SetAt(e->thisIntKey, e);

    thisKey = nextKey;
    thisIntKey = nextIntKey;
  }

  for (PINDEX i = 0; i < 3; i++) {
    TestMap();    
    TestDict();    
    TestIntMap();    
    TestIntDict();
    cerr << " " << endl << " " << endl;
  }

}

void MapDictionary::TestMap()
{
  PString thisKey;
  PTime a;
  for (PINDEX i = 0; i < loops; i++) {

    thisKey = firstKey;
    PINDEX count = 0;
    while(count < size) {   
      Element *e = elementMap[thisKey];
      thisKey = e->nextKey;
      count++;
    }
  }
  PTime b;
  cerr << "Map test, time is " << (b -a) << endl;
}

void MapDictionary::TestDict()
{
  PString thisKey;
  
  PTime b;
  for (PINDEX i = 0; i < loops; i++) {
    thisKey = firstKey;
    PINDEX count = 0;
    while(count < size) {
      Element &e = elementDict[thisKey];
      thisKey = e.nextKey;
      count++;
    }
  }
  PTime c;

  cerr << "for ptlib PDictionary, time is " << (c - b) << endl;
}
void MapDictionary::TestIntMap()
{
  PINDEX thisKey;
  PTime a;
  for (PINDEX i = 0; i < loops; i++) {

    thisKey = firstIntKey;
    PINDEX count = 0;
    while(count < size) {   
      Element *e = elementIntMap[thisKey];
      thisKey = e->nextIntKey;
      count++;
    }
  }
  PTime b;
  cerr << "Map INT test, time is " << (b -a) << endl;
}


void MapDictionary::TestIntDict()
{
  PINDEX thisKey;
  PTime b;
  for (PINDEX i = 0; i < loops; i++) {
    thisKey = firstIntKey;
    PINDEX count = 0;
    while(count < size) {
      Element &e = elementIntDict[thisKey];
      thisKey = e.nextIntKey;
      count++;
    }
  }

  PTime c;

  cerr << "for ptlib PINDEX PDictionary, time is " << (c - b) << endl;
}


  
// End of File ///////////////////////////////////////////////////////////////
