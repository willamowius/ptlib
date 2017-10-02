/*
 * pdns.h
 *
 * PWLib library for DNS lookup services
 *
 * Portable Windows Library
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

#ifndef PTLIB_PDNS_H
#define PTLIB_PDNS_H

#if P_DNS

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/sockets.h>

#include <ptclib/random.h>
#include <ptclib/url.h>

#if defined(_WIN32)

  #include <windns.h>
  #include <ntverp.h>

  // Accommodate spelling error in windns.h
  enum { DnsSectionAdditional = DnsSectionAddtional };

  #if VER_PRODUCTBUILD < 6000
    typedef struct
    {
        WORD            wOrder;
        WORD            wPreference;
        PSTR            pFlags;
        PSTR            pService;
        PSTR            pRegularExpression;
        PSTR            pReplacement;
    }
    DNS_NAPTR_DATA;
  #endif

#else /* _WIN32 */

  #define  P_HAS_RESOLVER 1         // set if using Unix-style DNS routines
  #include <arpa/nameser.h>
  #include <resolv.h>
  #if defined(P_MACOSX) && (P_MACOSX >= 700)
    #include <arpa/nameser_compat.h>
  #endif

#endif  // _WIN32


#ifdef P_HAS_RESOLVER

//////////////////////////////////////////////////////////////////////////
//
// these classes provide an emulation of the Microsoft DNS API 
// on non-Window systems
//

#ifndef T_SRV
#define T_SRV   33
#endif

#ifndef T_NAPTR
#define T_NAPTR   35
#endif


#define DNS_STATUS  int
#define DNS_TYPE_SRV  T_SRV
#define DNS_TYPE_MX  T_MX
#define DNS_TYPE_A  T_A
#define DNS_TYPE_AAAA  T_AAAA
#define DNS_TYPE_NAPTR  T_NAPTR
#define DnsFreeRecordList 1
#define DNS_QUERY_STANDARD 0
#define DNS_QUERY_BYPASS_CACHE 0

typedef struct _DnsAData {
  DWORD IpAddress;
} DNS_A_DATA;

typedef struct _DnsAAAAData {
  DWORD Ip6Address[4];
} DNS_AAAA_DATA;

typedef struct {
  char   pNameExchange[MAXDNAME];
  WORD   wPreference;
} DNS_MX_DATA;

typedef struct {
  char pNameHost[MAXDNAME];
} DNS_PTR_DATA;

typedef struct _DnsSRVData {
  char   pNameTarget[MAXDNAME];
  WORD   wPriority;
  WORD   wWeight;
  WORD   wPort;
} DNS_SRV_DATA;

typedef struct _DnsNULLData {
  DWORD  dwByteCount;
  char   data[1];
} DNS_NULL_DATA;

typedef struct _DnsRecordFlags
{
  unsigned   Section     : 2;
  unsigned   Delete      : 1;
  unsigned   CharSet     : 2;
  unsigned   Unused      : 3;
  unsigned   Reserved    : 24;
} DNS_RECORD_FLAGS;

typedef enum _DnsSection
{
  DnsSectionQuestion,
  DnsSectionAnswer,
  DnsSectionAuthority,
  DnsSectionAdditional,
} DNS_SECTION;


class DnsRecord {
  public:
    DnsRecord * pNext;
    char        pName[MAXDNAME];
    WORD        wType;
    WORD        wDataLength;

    union {
      DWORD               DW;     ///< flags as DWORD
      DNS_RECORD_FLAGS    S;      ///< flags as structure
    } Flags;

    union {
      DNS_A_DATA     A;
      DNS_AAAA_DATA  AAAA;
      DNS_MX_DATA    MX;
      DNS_PTR_DATA   NS;
      DNS_SRV_DATA   SRV;
      DNS_NULL_DATA  Null;
    } Data;
};

typedef DnsRecord DNS_RECORD;
typedef DnsRecord * PDNS_RECORD;


extern void DnsRecordListFree(PDNS_RECORD rec, int FreeType);
extern PDNS_RECORD DnsRecordSetCopy(PDNS_RECORD src);

extern DNS_STATUS DnsQuery_A(const char * service,
          WORD requestType,
          DWORD options,
          void *,
          PDNS_RECORD * results,
          void *);


#endif // P_HAS_RESOLVER

namespace PDNS {

///////////////////////////////////////////////////////////////////////////

DNS_STATUS Cached_DnsQuery(
    const char * name,
    WORD       type,
    DWORD      options,
    void *     extra,
    PDNS_RECORD * queryResults,
    void * reserved
);



//////////////////////////////////////////////////////////////////////////
//
//  this template automates the creation of a list of records for
//  a specific type of DNS lookup
//

template <unsigned type, class RecordListType, class RecordType>
PBoolean Lookup(const PString & name, RecordListType & recordList)
{
  if (name.IsEmpty())
    return false;

  recordList.RemoveAll();

  PDNS_RECORD results = NULL;
  DNS_STATUS status = Cached_DnsQuery((const char *)name, 
                                      type,
                                      DNS_QUERY_STANDARD, 
                                      NULL, 
                                      &results, 
                                      NULL);
  if (status != 0) {
    if (results != NULL)
      DnsRecordListFree(results, DnsFreeRecordList);
    return false;
  }

  // find records matching the correct type
  PDNS_RECORD dnsRecord = results;
  while (dnsRecord != NULL) {
    RecordType * record = recordList.HandleDNSRecord(dnsRecord, results);
    if (record != NULL)
      recordList.Append(record);
    dnsRecord = dnsRecord->pNext;
  }

  if (results != NULL)
    DnsRecordListFree(results, DnsFreeRecordList);

  return recordList.GetSize() != 0;
}

/////////////////////////////////////////////////////////////

class SRVRecord : public PObject
{
  PCLASSINFO(SRVRecord, PObject);
  public:
    SRVRecord()
    { used = false; port = 0; priority = 0; weight = 0; }

    Comparison Compare(const PObject & obj) const;
    void PrintOn(ostream & strm) const;

    PString            hostName;
    PIPSocket::Address hostAddress;
    PBoolean               used;
    WORD port;
    WORD priority;
    WORD weight;
};

PDECLARE_SORTED_LIST(SRVRecordList, PDNS::SRVRecord)
  public:
    void PrintOn(ostream & strm) const;

    SRVRecord * GetFirst();
    SRVRecord * GetNext();

    PDNS::SRVRecord * HandleDNSRecord(PDNS_RECORD dnsRecord, PDNS_RECORD results);

  protected:
    PINDEX     priPos;
    PWORDArray priList;
};

/**
  * return a list of DNS SRV record with the specified service type
  */

inline PBoolean GetRecords(const PString & service, SRVRecordList & serviceList)
{ return Lookup<DNS_TYPE_SRV, SRVRecordList, SRVRecord>(service, serviceList); }

/**
  * provided for backwards compatibility
  */
inline PBoolean GetSRVRecords(
      const PString & service,
      SRVRecordList & serviceList
)
{ return GetRecords(service, serviceList); }

/**
  * return a list of DNS SRV record with the specified service, type and domain
  */

PBoolean GetSRVRecords(
      const PString & service,
      const PString & type,
      const PString & domain,
      SRVRecordList & serviceList
);

/**
  * Perform a DNS lookup of the specified service
  * @return true if the service could be resolved, else false
  */

PBoolean LookupSRV(
         const PString & srvQuery,
         WORD defaultPort,
         PIPSocketAddressAndPortVector & addrList
);

PBoolean LookupSRV( 
         const PString & domain,                  ///< domain to lookup
         const PString & service,                 ///< service to use
         WORD defaultPort,                        ///< default por to use
         PIPSocketAddressAndPortVector & addrList ///< returned list of sockets and ports
); 

PBoolean LookupSRV( 
         const PURL & url,          ///< URL to lookup
         const PString & service,   ///< service to use
         PStringList & returnStr    ///< resolved addresses, if return value is true
);  

////////////////////////////////////////////////////////////////

class MXRecord : public PObject
{
  PCLASSINFO(MXRecord, PObject);
  public:
    MXRecord()
    { used = false; preference = 0; }
    Comparison Compare(const PObject & obj) const;
    void PrintOn(ostream & strm) const;

    PString            hostName;
    PIPSocket::Address hostAddress;
    PBoolean               used;
    WORD               preference;
};

PDECLARE_SORTED_LIST(MXRecordList, PDNS::MXRecord)
  public:
    void PrintOn(ostream & strm) const;

    MXRecord * GetFirst();
    MXRecord * GetNext();

    PDNS::MXRecord * HandleDNSRecord(PDNS_RECORD dnsRecord, PDNS_RECORD results);

  protected:
    PINDEX lastIndex;
};

/**
  * return a list of MX records for the specified domain
  */
inline PBoolean GetRecords(
      const PString & domain,
      MXRecordList & serviceList
)
{ return Lookup<DNS_TYPE_MX, MXRecordList, MXRecord>(domain, serviceList); }

/**
  * provided for backwards compatibility
  */
inline PBoolean GetMXRecords(
      const PString & domain,
      MXRecordList & serviceList
)
{
  return GetRecords(domain, serviceList);
}


}; // namespace PDNS

#endif // P_DNS

#endif // PTLIB_PDNS_H


// End Of File ///////////////////////////////////////////////////////////////
