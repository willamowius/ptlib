//
// httptest.cxx
//
// Copyright 2011 Vox Lucida Pty. Ltd.
//

#include <ptlib.h>
#include <ptlib/pprocess.h>
#include <ptlib/sockets.h>
#include <ptclib/http.h>
#include <ptclib/threadpool.h>


class HTTPConnection
{
  public:
    HTTPConnection(PHTTPSpace & httpNameSpace)
      :  m_httpNameSpace(httpNameSpace)
    {
    }

    void Work();

    PHTTPSpace & m_httpNameSpace;
    PTCPSocket m_socket;
};


class HTTPTest : public PProcess
{
    PCLASSINFO(HTTPTest, PProcess)
  public:
    void Main();

    PQueuedThreadPool<HTTPConnection> m_pool;
};

PCREATE_PROCESS(HTTPTest)


void HTTPTest::Main()
{
  PArgList & args = GetArguments();
  args.Parse("h-help."
             "p-port:"
             "T-theads:"
             "Q-queue:"
#if PTRACING
             "o-output:"
             "t-trace."
#endif
       );

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  if (args.HasOption('h')) {
    PError << "usage: " << GetFile().GetTitle() << "[options]\n"
              "\n"
              "Available options are:\n"
              "   -h --help             : print this help message.\n"
              "   -p --port n           : port number to listen on (default 80).\n"
              "   -T --threads n        : max number of threads in pool (default 10)\n"
              "   -Q --queue n          : max queue size for listening sockets (default 100).\n"
#if PTRACING
              "   -o or --output file   : file name for output of log messages\n"       
              "   -t or --trace         : degree of verbosity in log (more times for more detail)\n"     
#endif
           << endl;
    return;
  }

  m_pool.SetMaxWorkers(args.GetOptionString('T', "10").AsUnsigned());

  PTCPSocket listener((WORD)args.GetOptionString('p', "80").AsUnsigned());
  if (!listener.Listen(args.GetOptionString('Q', "100").AsUnsigned())) {
    cerr << "Could not listen on port " << listener.GetPort() << endl;
    return;
  }

  PHTTPSpace httpNameSpace;
  httpNameSpace.AddResource(new PHTTPString("index.html", "Hello", "text/plain"));

  cout << "Listening for HTTP on port " << listener.GetPort() << endl;

  for (;;) {
    HTTPConnection * connection = new HTTPConnection(httpNameSpace);
    if (connection->m_socket.Accept(listener))
      m_pool.AddWork(connection);
    else {
      delete connection;
      cerr << "Error in accept: " << listener.GetErrorText() << endl;
      break;
    }
  }

  cout << "Exiting HTTP test" << endl;
}


void HTTPConnection::Work()
{
  PTRACE(3, "HTTPTest\tStarted work on " << m_socket.GetPeerAddress());

  PHTTPServer httpServer(m_httpNameSpace);
  if (!httpServer.Open(m_socket))
    return;

  unsigned count = 0;
  while (httpServer.ProcessCommand())
    ++count;

  PTRACE(3, "HTTPTest\tEnded work on " << m_socket.GetPeerAddress() << ", " << count << " transactions.");
}


// End of hello.cxx
