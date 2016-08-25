/*
 * main.cxx
 *
 * PWLib application source file for DNSTest
 *
 * Main program entry point.
 *
 * Copyright 2003 Equivalence
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <ptclib/pdns.h>
#include <ptclib/enum.h>
#include <ptclib/url.h>


#if !P_DNS
#error Must have DNS support for this application
#endif


class DNSTest : public PProcess
{
  PCLASSINFO(DNSTest, PProcess)

  public:
    DNSTest();
    void Main();
};

PCREATE_PROCESS(DNSTest);


DNSTest::DNSTest()
  : PProcess("Equivalence", "DNSTest", 1, 0, AlphaCode, 1)
{
}

void Usage()
{
  PError << "usage: dnstest -t MX hostname\n"
            "       dnstest -t SRV service            (i.e. _ras._udp._example.com)\n"
            "       dnstest -t SRV service url        (i.e. _sip._udp sip:fred@example.com)\n"
            "       dnstest -t RDS url service        (i.e. mydomain.com H323+D2U)\n"
            "       dnstest -t NAPTR resource         (i.e. 2.1.2.1.5.5.5.0.0.8.1.e164.org)\n"
            "       dnstest -t NAPTR resource service (i.e. 2.1.2.1.5.5.5.0.0.8.1.e164.org E2U+SIP)\n"
            "       dnstest -t ENUM service           (i.e. +18005551212 E2U+SIP)\n"
            "       dnstest -u url                    (i.e. http://craigs@postincrement.com)\n"
  ;
}

template <class RecordListType>
void GetAndDisplayRecords(const PString & name)
{
  RecordListType records;
  if (!PDNS::GetRecords(name, records))
    PError << "Lookup for " << name << " failed" << endl;
  else
    cout << "Lookup for " << name << " returned" << endl << records << endl;
}

struct LookupRecord {
  PIPSocket::Address addr;
  WORD port;
  PString type;
  PString source;
};

ostream & operator << (ostream & strm, const LookupRecord & rec) 
{
  strm << rec.type << " " << rec.addr << ":" << rec.port << " from " << rec.source;
  return strm;
}

PBoolean FindSRVRecords(std::vector<LookupRecord> & recs,
                    const PString & domain,
                    const PString & type,
                    const PString & srv)
{
  PDNS::SRVRecordList srvRecords;
  PString srvLookupStr = srv + domain;
  PBoolean found = PDNS::GetRecords(srvLookupStr, srvRecords);
  if (found) {
    PDNS::SRVRecord * recPtr = srvRecords.GetFirst();
    while (recPtr != NULL) {
      LookupRecord rec;
      rec.addr = recPtr->hostAddress;
      rec.port = recPtr->port;
      rec.type = type;
      rec.source = srv;
      recs.push_back(rec);
      recPtr = srvRecords.GetNext();
    }
  } 
  return found;
}

void LookupSRVURL(const PString & url, const PString & service)
{
  PStringList addrs;
  if (!PDNS::LookupSRV(url, service, addrs)) {
    cout << "no records returned by SRV lookup of " << url << " with service " << service << endl;
  } else {
    cout << setfill('\n') << addrs << setfill(' ');
  }
}

void LookupRDSURL(const PString & url, const PString & service)
{
  PStringList addrs;
  if (!PDNS::RDSLookup(url, service, addrs)) {
    cout << "no records returned by RDS lookup of " << url << " with service " << service << endl;
  } else {
    cout << setfill('\n') << addrs << setfill(' ');
  }
}

void DNSTest::Main()
{
  PArgList & args = GetArguments();

  args.Parse("r:t:u.");

  if (args.GetCount() < 1) {
    Usage();
    return;
  }

  bool showCount = false;
  int repeat = args.GetOptionString('r').AsInteger();
  if (repeat < 1)
    repeat = 1;
  else
    showCount = true;

  int count = 1;

  while (repeat-- > 0) {

    if (showCount)
      cout << "#" << (int)count++ << " ";

    if (args.HasOption('u')) {
      if (args.GetCount() < 0) {
        Usage();
        return;
      }

      PURL url(args[0]);
      if (url.GetScheme() *= "h323") {
        PString user   = url.GetUserName();
        PString domain = url.GetHostName();
        WORD    port   = url.GetPort();
        cout << "user = " << user << ", domain = " << domain << ", port = " << port << endl;

        std::vector<LookupRecord> found;

        PBoolean hasGK = FindSRVRecords(found, domain, "LRQ",         "_h323ls._udp.");
        hasGK = hasGK || FindSRVRecords(found, domain, "LRQ",         "_h323rs._udp.");
        FindSRVRecords(found, domain, "Call direct", "_h323cs._tcp.");

        // if no entries so far, see if the domain is actually a host
        if (found.size() == 0) {
          PIPSocket::Address addr;
          if (PIPSocket::GetHostAddress(domain, addr)) {
            LookupRecord rec;
            rec.addr = addr;
            rec.port = 1720;
            rec.type = "Call direct";
            rec.source = "DNS";
            found.push_back(rec);
          }
        }

        if (!hasGK) {
          PDNS::MXRecordList mxRecords;
          if (PDNS::GetRecords(domain, mxRecords)) {
            PDNS::MXRecord * recPtr = mxRecords.GetFirst();
            while (recPtr != NULL) {
              LookupRecord rec;
              rec.addr = recPtr->hostAddress;
              rec.port = 1719;
              rec.type = "LRQ";
              rec.source = "MX";
              found.push_back(rec);
              recPtr = mxRecords.GetNext();
            }
          } 
        }

        if (found.size() == 0) {
          PError << "Cannot find match" << endl;
        }
        else
        {
          std::vector<LookupRecord>::const_iterator r;
          cout << "Found\n";
          for (r = found.begin(); r != found.end(); ++r) {
            cout << *r << endl;
          }
        }
      }
      else {
        PError << "error: unsupported scheme " << url.GetScheme() << endl;
      }
    }

    else if (args.HasOption('t')) {
      PString type = args.GetOptionString('t');
      if ((type *= "SRV") && (args.GetCount() == 1)) 
        GetAndDisplayRecords<PDNS::SRVRecordList>(args[0]);

      else if ((type *= "SRV") && (args.GetCount() == 2)) 
        LookupSRVURL(args[1], args[0]);

      else if ((type *= "RDS") && (args.GetCount() == 2)) 
        LookupRDSURL(args[1], args[0]);

      else if (type *= "MX")
        GetAndDisplayRecords<PDNS::MXRecordList>(args[0]);

      else if (type *= "NAPTR") {
        if (args.GetCount() == 1)
          GetAndDisplayRecords<PDNS::NAPTRRecordList>(args[0]);
        else {
          PDNS::NAPTRRecordList records;
          if (!PDNS::GetRecords(args[0], records))
            PError << "Lookup for " << args[0] << " failed" << endl;
          else {
            cout << "Returned " << endl;
            PDNS::NAPTRRecord * rec = records.GetFirst(args[1]);
            while (rec != NULL) {
              cout << *rec;
              rec = records.GetNext(args[1]);
            }
          }
        }
      }

      else if (type *= "enum") {
        if (args.GetCount() < 2)
          Usage();
        else {
          PString e164    = args[0];
          PString service = args[1];
          PString str;
          if (!PDNS::ENUMLookup(e164, service, str))
            cout << "Could not resolve E164 number " << e164 << " with service " << service << endl;
          else
            cout << "E164 number " << e164 << " with service " << service << " resolved to " << str << endl;
        }
      }
    }
    Sleep(1000);
  }
}
  
// End of File ///////////////////////////////////////////////////////////////
