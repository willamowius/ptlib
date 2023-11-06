/*
 * guid.h
 *
 * Globally Unique Identifier
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_GUID_H
#define PTLIB_GUID_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

class PASN_OctetString;


///////////////////////////////////////////////////////////////////////////////

/**Globally unique ID definition.
   This implements a 128 bit globally unique ID as required by many protocols
   and software systems. The value is constructed in such a way as to make a
   duplicate anywhere in the world highly unlikely.
 */
class PGloballyUniqueID : public PBYTEArray
{
  PCLASSINFO(PGloballyUniqueID, PBYTEArray);

  public:
  /**@name Construction */
  //@{
    /**Create a new ID.
       The ID created with this will be initialised to a globally unique ID
       as per specification.
     */
    PGloballyUniqueID();

    /**Create an ID from a C string of hex (as produced by AsString()).
       A useful construct is to construct a OpalGloballyUniqueID() with
       NULL which produces an all zero GUID, detectable with the IsNULL()
       function.
     */
    PGloballyUniqueID(
      const char * cstr    ///<  C string to convert
    );
    /**Create an ID from a PString of hex (as produced by AsString()).
     */
    PGloballyUniqueID(
      const PString & str  ///<  String of hex to convert
    );
#if P_ASN
    /**Create an ID from an octet string in an ASN PDU.
     */
    PGloballyUniqueID(
      const PASN_OctetString & ostr  ///<  Octet string from ASN to convert
    );
#endif
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Standard stream print function.
       The PObject class has a << operator defined that calls this function
       polymorphically.
      */
    virtual void PrintOn(
      ostream & strm    ///<  Stream to output text representation
    ) const;

    /**Standard stream read function.
       The PObject class has a >> operator defined that calls this function
       polymorphically.
      */
    virtual void ReadFrom(
      istream & strm    ///<  Stream to output text representation
    );

    /**Create a clone of the ID.
       The duplicate ID has the same value as the source. Required for having
       this object as a key in dictionaries.
      */
    virtual PObject * Clone() const;

    /**Get the hash value for the ID.
       Creates a number based on the ID value for use in the hash table of
       a dictionary. Required for having this object as a key in dictionaries.
      */
    virtual PINDEX HashFunction() const;
  //@}

  /**@name Operations */
  //@{
    /**Convert the ID to human readable string.
      */
    PString AsString() const;

    /**Test if the GUID is null, ie consists of all zeros.
      */
    PBoolean IsNULL() const;

    PBoolean operator!() const { return !IsNULL(); }
  //@}
};


#endif // PTLIB_GUID_H


// End Of File ///////////////////////////////////////////////////////////////
