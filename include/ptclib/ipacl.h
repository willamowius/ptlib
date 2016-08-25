/*
 * ipacl.h
 *
 * IP Access Control Lists
 *
 * Portable Windows Library
 *
 * Copyright (c) 1998-2002 Equivalence Pty. Ltd.
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

#ifndef PTLIB_IPACL_H
#define PTLIB_IPACL_H


#include <ptlib/sockets.h>


/** This class is a single IP access control specification.
 */
class PIpAccessControlEntry : public PObject
{
  PCLASSINFO(PIpAccessControlEntry, PObject)

  public:
    /** Create a new IP access control specification. See the Parse() function
       for more details on the format of the <CODE>description</CODE>
       parameter.
     */
    PIpAccessControlEntry(
      PIPSocket::Address addr,
      PIPSocket::Address msk,
      PBoolean allow
    );
    PIpAccessControlEntry(
      const PString & description
    );

    /** Set a new IP access control specification. See the Parse() function
       for more details on the format of the <CODE>pstr</CODE> and
       <CODE>cstr</CODE> parameters.
     */
    PIpAccessControlEntry & operator=(
      const PString & pstr
    );
    PIpAccessControlEntry & operator=(
      const char * cstr
    );

    /** Compare the two objects and return their relative rank.

       @return
       <CODE>LessThan</CODE>, <CODE>EqualTo</CODE> or <CODE>GreaterThan</CODE>
       according to the relative rank of the objects.
     */
    virtual Comparison Compare(
      const PObject & obj   ///< Object to compare against.
    ) const;

    /** Output the contents of the object to the stream. This outputs the same
       format as the AsString() function.
     */
    virtual void PrintOn(
      ostream &strm   ///< Stream to print the object into.
    ) const;

    /** Input the contents of the object from the stream. This expects the
       next space delimited entry in the stream to be as described in the
       Parse() function.
     */
    virtual void ReadFrom(
      istream &strm   ///< Stream to read the objects contents from.
    );

    /** Convert the specification to a string, that can be processed by the
       Parse() function.

       @return
       PString representation of the entry.
     */
    PString AsString() const;

    /** Check the internal fields of the specification for validity.

       @return
       true if entry is valid.
     */
    PBoolean IsValid();

    /** Parse the description string into this IP access control specification.
       The string may be of several forms:
          n.n.n.n         Simple IP number, this has an implicit mask of
                          255.255.255.255
          n.n.            IP with trailing dot, assumes a mask equal to the
                          number of specified octets eg 10.1. is equivalent
                          to 10.1.0.0/255.255.0.0
          n.n.n.n/b       An IP network using b bits of mask, for example
                          10.1.0.0/14 is equivalent to 10.0.1.0/255.248.0.0
          n.n.n.n/m.m.m.m An IP network using the specified mask
          hostname        A specific host name, this has an implicit mask of
                          255.255.255.255
          .domain.dom     Matches an IP number whose cannonical name (found
                          using a reverse DNS lookup) ends with the specified
                          domain.

       @return
       true if entry is valid.
     */
    PBoolean Parse(
      const PString & description   ///< Description of the specification
    );


    /** Check to see if the specified IP address match any of the conditions
       specifed in the Parse() function for this entry.

       @return
       true if entry can match the address.
     */
    PBoolean Match(
      PIPSocket::Address & address    ///< Address to search for
    );

    /**Get the domain part of entry.
      */
    const PString & GetDomain() const { return domain; }

    /**Get the address part of entry.
      */
    const PIPSocket::Address & GetAddress() const { return address; }

    /**Get the mask part of entry.
      */
    const PIPSocket::Address & GetMask() const { return mask; }

    /**Get the allowed flag of entry.
      */
    PBoolean IsAllowed() const { return allowed; }

    /**Get the hidden flag of entry.
      */
    PBoolean IsHidden()  const { return hidden; }

  protected:
    PString            domain;
    PIPSocket::Address address;
    PIPSocket::Address mask;
    PBoolean               allowed;
    PBoolean               hidden;
};

PSORTED_LIST(PIpAccessControlList_base, PIpAccessControlEntry);


/** This class is a list of IP address mask specifications used to validate if
   an address may or may not be used in a connection.

   The list may be totally internal to the application, or may use system
   wide files commonly use under Linux (hosts.allow and hosts.deny file). These
   will be used regardless of the platform.

   When a search is done using IsAllowed() function, the first entry that
   matches the specified IP address is found, and its allow flag returned. The
   list sorted so that the most specific IP number specification is first and
   the broadest onse later. The entry with the value having a mask of zero,
   that is the match all entry, is always last.
 */
class PIpAccessControlList : public PIpAccessControlList_base
{

  PCLASSINFO(PIpAccessControlList, PIpAccessControlList_base)

  public:
    /** Create a new, empty, access control list.
     */
    PIpAccessControlList(
      PBoolean defaultAllowance = true
    );

    /** Load the system wide files commonly use under Linux (hosts.allow and
       hosts.deny file) for IP access. See the Linux man entries on these
       files for more information. Note, these files will be loaded regardless
       of the actual platform used. The directory returned by the
       PProcess::GetOSConfigDir() function is searched for the files.

       The <CODE>daemonName</CODE> parameter is used as the search argument in
       the hosts.allow/hosts.deny file. If this is NULL then the
       PProcess::GetName() function is used.

       @return
       true if all the entries in the file were added, if any failed then
       false is returned.
     */
    PBoolean LoadHostsAccess(
      const char * daemonName = NULL    ///< Name of "daemon" application
    );

#ifdef P_CONFIG_FILE

    /** Load entries in the list from the configuration file specified. This is
       equivalent to Load(cfg, "IP Access Control List").

       @return
       true if all the entries in the file were added, if any failed then
       false is returned.
     */
    PBoolean Load(
      PConfig & cfg   ///< Configuration file to load entries from.
    );

    /** Load entries in the list from the configuration file specified, using
       the base name for the array of configuration file values. The format of
       entries in the configuration file are suitable for use with the
       PHTTPConfig classes.

       @return
       true if all the entries in the file were added, if any failed then
       false is returned.
     */
    PBoolean Load(
      PConfig & cfg,            ///< Configuration file to load entries from.
      const PString & baseName  ///< Base name string for each entry in file.
    );

    /** Save entries in the list to the configuration file specified. This is
       equivalent to Save(cfg, "IP Access Control List").
     */
    void Save(
      PConfig & cfg   ///< Configuration file to save entries to.
    );

    /** Save entries in the list to the configuration file specified, using
       the base name for the array of configuration file values. The format of
       entries in the configuration file are suitable for use with the
       PHTTPConfig classes.
     */
    void Save(
      PConfig & cfg,            ///< Configuration file to save entries to.
      const PString & baseName  ///< Base name string for each entry in file.
    );

#endif // P_CONFIG_FILE

    /** Add the specified entry into the list. See the PIpAccessControlEntry
       class for more details on the format of the <CODE>description</CODE>
       field.

       @return
       true if the entries was successfully added.
     */
    PBoolean Add(
      PIpAccessControlEntry * entry ///< Entry for IP match parameters
    );
    PBoolean Add(
      const PString & description   ///< Description of the IP match parameters
    );
    PBoolean Add(
      PIPSocket::Address address,   ///< IP network address
      PIPSocket::Address mask,      ///< Mask for IP network
      PBoolean allow                    ///< Flag for if network is allowed or not
    );

    /** Remove the specified entry into the list. See the PIpAccessControlEntry
       class for more details on the format of the <CODE>description</CODE>
       field.

       @return
       true if the entries was successfully removed.
     */
    PBoolean Remove(
      const PString & description   ///< Description of the IP match parameters
    );
    PBoolean Remove(
      PIPSocket::Address address,   ///< IP network address
      PIPSocket::Address mask       ///< Mask for IP network
    );


    /**Create a new PIpAccessControl specification entry object.
       This may be used by an application to create descendents of
       PIpAccessControlEntry when extra information/functionality is required.

       The default behaviour creates a PIpAccessControlEntry.
      */
    virtual PIpAccessControlEntry * CreateControlEntry(
      const PString & description
    );

    /**Find the PIpAccessControl specification for the address.
      */
    PIpAccessControlEntry * Find(
      PIPSocket::Address address    ///< IP Address to find
    ) const;

    /** Test the address/connection for if it is allowed within this access
       control list. If the <CODE>socket</CODE> form is used the peer address
       of the connection is tested.

       If the list is empty then true is returned. If the list is not empty,
       but the IP address does not match any entries in the list, then false
       is returned. If a match is made then the allow state of that entry is
       returned.

       @return
       true if the remote host address is allowed.
     */
    PBoolean IsAllowed(
      PTCPSocket & socket           ///< Socket to test
    ) const;
    PBoolean IsAllowed(
      PIPSocket::Address address    ///< IP Address to test
    ) const;


    /**Get the default state for allowed access if the list is empty.
      */
    PBoolean GetDefaultAllowance() const { return defaultAllowance; }

    /**Set the default state for allowed access if the list is empty.
      */
    void SetDefaultAllowance(PBoolean defAllow) { defaultAllowance = defAllow; }

  private:
    PBoolean InternalLoadHostsAccess(const PString & daemon, const char * file, PBoolean allow);
    PBoolean InternalRemoveEntry(PIpAccessControlEntry & entry);

  protected:
    PBoolean defaultAllowance;
};


#endif  // PTLIB_IPACL_H


// End of File ///////////////////////////////////////////////////////////////
