/*
 * pdns.h
 *
 * PWLib library for ENUM lookup
 *
 * Portable Windows Library
 *
 * Copyright (C) 2004 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_ENUM_H
#define PTLIB_ENUM_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptclib/pdns.h>

#if P_DNS

namespace PDNS {

#ifndef NAPTR_SRV
#define NAPTR_SRV 35
#endif

///////////////////////////////////////////////////////////////////////////

class NAPTRRecord : public PObject
{
  PCLASSINFO(NAPTRRecord, PObject);
  public:
    Comparison Compare(const PObject & obj) const;
    void PrintOn(ostream & strm) const;

    WORD order;
    WORD preference;
    PString flags;
    PString service;
    PString regex;
    PString replacement;
};

PDECLARE_SORTED_LIST(NAPTRRecordList, PDNS::NAPTRRecord)
  public:
    void PrintOn(ostream & strm) const;

    NAPTRRecord * GetFirst(const char * service = NULL);
    NAPTRRecord * GetNext(const char * service = NULL);

    PDNS::NAPTRRecord * HandleDNSRecord(PDNS_RECORD dnsRecord, PDNS_RECORD results);

    void UnlockOrder()
    { orderLocked = false; }

  protected:
    PINDEX     currentPos;
    int        lastOrder;
    PBoolean       orderLocked;
};

inline PBoolean GetRecords(const PString & domain, NAPTRRecordList & recordList)
{ return Lookup<NAPTR_SRV, NAPTRRecordList, NAPTRRecord>(domain, recordList); }

/**
  * Set the default ENUM domain search list 
  */
void SetENUMServers(const PStringArray & serverlist);

/**
  * Perform a lookup of the specified DN using the specified service
  * and domain list. Returns the resultant URL
  *
  * @return true if the DN could be resolved, else false
  */
PBoolean ENUMLookup(
                const PString & dn,             ///< DN to lookup
                const PString & service,        ///< ENUM service to use
                const PStringArray & domains,   ///< list of ENUM domains to search
                PString & URL                   ///< resolved URL, if return value is true
);

/**
  * Perform a lookup of the specified DN using the specified service
  * using the default domain list. Returns the resultant URL.
  *
  * This function uses the default domain list, which can be set by the SetENUMServers function
  *
  * @return true if the DN could be resolved, else false
  */
PBoolean ENUMLookup(const PString & dn,             ///< DN to lookup
                const PString & service,        ///< ENUM service to use
                PString & URL                   ///< resolved URL, if return value is true
);   


//////////////////////////////////////////////////////////////
/* Uniform Resource Name Resolver Discovery System URN RDS
   This can be used to Host URI domains on hosting servers.
    This implementation follows RFC 2915 sect 7.1 Example 2:
    Example
	Question: find h323:me@a.com by looking up mydomain.com
	Query the top most NAPTR record of mydomain.com for h323:me@a.com
	  IN NAPTR 100   10   ""  ""  ^h323:(.+)@([a-z0-9\-\.]*);*(.*)$/\2.subs.mydomain.com/i 
	   this converts a.com to a.com.subs.mydomain.com
	Query H323+D2U NAPTR record for a.com.subs.mydomain.com
      IN NAPTR 100  50  "s"  "H323+D2U"     ""  _h323ls._udp.host.com
    Query SRV records for host.com
	  _h323ls._udp.host.com  	172800  	IN  	SRV  	0  	0  	1719  	gk.host.com

	Answer: find h323:me@a.com by LRQ to gk.host.com:1719
*/

/**
  * Set the Default NAPTR server to lookup
  */
void SetRDSServers(const PStringArray & servers);

/**
  * Perform a lookup of the specified url on the NAPTR
  * server list (set by SetNAPTRServers) that uses the 
  * specified service. Returns the resultant corresponding 
  * signalling addresses
  *
  * @return true if the url could be resolved, else false
  */

PBoolean RDSLookup(const PURL & url,           ///< URL to lookup
            const PString & service,       ///< Service to use
			  PStringList & dn             ///< resolved addresses, if return value is true
);

/**
  * Perform a lookup of the specified url on the NAPTR
  * on the specified server list that uses the specified service
  * Returns the resultant corresponding signalling addresses
  *
  * @return true if the url could be resolved, else false
  */

PBoolean RDSLookup(const PURL & url,            ///< URL to lookup
            const PString & service,        ///< Service to use
       const PStringArray & naptrSpaces,    ///< RDS Servers to lookup
              PStringList & returnStr       ///< resolved addresses, if return value is true
);

}; // namespace PDNS

#endif // P_DNS

#endif // PTLIB_ENUM_H


// End Of File ///////////////////////////////////////////////////////////////
