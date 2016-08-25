//
// main.cxx
//
// String Tests
//
// Copyright 2011 Vox Lucida Pty. Ltd.
//


#include <ptlib.h>
#include <ptlib/pprocess.h>
#include <string>

////////////////////////////////////////////////
//
// test #1 - PString test
//

void Test1()
{
  {
    PString pstring1("hello world");
    PString pstring2(pstring1);

    strcpy((char *)(const char *)pstring2, "overwrite");

    cout << pstring1 << endl;
    cout << pstring2 << endl;
  }
  {
    PString pstring1("hello world");
    PString pstring2(pstring1);

    strcpy(pstring2.GetPointer(), "overwrite");

    cout << pstring1 << endl;
    cout << pstring2 << endl;
  }
}


////////////////////////////////////////////////
//
// test #2 - String stream test
//

void Test2()
{
  for (PINDEX i = 0; i < 2000; ++i) 
  {
    PStringStream str;
    str << "Test #" << 2;
  }

  PStringStream s;
  s << "This is a test of the string stream, integer: " << 5 << ", real: " << 5.6;
  cout << '"' << s << '"' << endl;
}


////////////////////////////////////////////////
//
// test #3 - PBYTEArray test
//

void Test3()
{
  {
    PBYTEArray buffer1(1024);
    PBYTEArray buffer2(buffer1);

    cout << "base address of PBYTEArray 1 = " << (void *)(buffer1.GetPointer()) << endl;
    cout << "base address of PBYTEArray 2 = " << (void *)(buffer1.GetPointer()) << endl;
  }

  {
    PString str1("hello");
    PString str2(str1);

    str2 = "world";

    cout << "base address of PString 1 = " << (void *)(str1.GetPointer()) << endl;
    cout << "base address of PString 2 = " << (void *)(str2.GetPointer()) << endl;
  }
}


////////////////////////////////////////////////
//
// test #4 - string concurrency test
//

#define SPECIALNAME     "openH323"
#define COUNT_MAX       2000000

PBoolean finishFlag;

template <class S>
struct StringConv {
  static const char * ToConstCharStar(const S &) { return NULL; }
};

template <class S, class C>
class StringHolder
{
  public:
    StringHolder(const S & _str)
      : str(_str) { }
    S GetString() const { return str; }
    S str;

    void TestString(int count, const char * label)
    {
      if (finishFlag)
        return;

      S s = GetString();
      const char * ptr = C::ToConstCharStar(s);
      //const char * ptr = s.c_str();
      char buffer[20];
      strncpy(buffer, ptr, 20);

      if (strcmp((const char *)buffer, SPECIALNAME)) {
        finishFlag = PTrue;
        cerr << "String compare failed at " << count << " in " << label << " thread" << endl;
        return;
      }
      if (count % 10000 == 0)
        cout << "tested " << count << " in " << label << " thread" << endl;
    }

    class TestThread : public PThread
    {
      PCLASSINFO(TestThread, PThread);
      public:
        TestThread(StringHolder & _holder) 
        : PThread(1000,NoAutoDeleteThread), holder(_holder)
        { Resume(); }

        void Main() 
        { int count = 0; while (!finishFlag && count < COUNT_MAX) holder.TestString(count++, "sub"); }

        StringHolder & holder;
    };

    PThread * StartThread()
    {
      return new TestThread(*this);
    }

};

struct PStringConv : public StringConv<PString> {
  static const char * ToConstCharStar(const PString & s) { return (const char *)s; }
};

struct StdStringConv : public StringConv<std::string> {
  static const char * ToConstCharStar(const std::string & s) { return s.c_str(); }
};

void Test4()
{
  // uncomment this to test std::string
  //StringHolder<std::string, StdStringConv> holder(SPECIALNAME);
  
  // uncomment this to test PString
  StringHolder<PString, PStringConv> holder(SPECIALNAME);

  PThread * thread = holder.StartThread();
  finishFlag = PFalse;
  int count = 0;
  while (!finishFlag && count < COUNT_MAX) 
    holder.TestString(count++, "main");
  finishFlag = PTrue;
  thread->WaitForTermination(9000);
  cerr << "finish" << endl;
  delete thread;
}

////////////////////////////////////////////////
//
// main
//

class StringTest : public PProcess
{
  PCLASSINFO(StringTest, PProcess)
  public:
    void Main();
};

PCREATE_PROCESS(StringTest);

void StringTest::Main()
{
  //PMEMORY_ALLOCATION_BREAKPOINT(16314);
  Test1(); cout << "End of test #1\n" << endl;
  Test2(); cout << "End of test #2\n" << endl;
  Test3(); cout << "End of test #3\n" << endl;
  Test4(); cout << "End of test #4\n" << endl;
}
