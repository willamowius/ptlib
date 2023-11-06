/*
 * mime.h
 *
 * Multipurpose Internet Mail Extensions support classes.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2002 Equivalence Pty. Ltd.
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

#ifndef PTLIB_PMIME_H
#define PTLIB_PMIME_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptclib/inetprot.h>
#include <ptclib/cypher.h>


class PMultiPartList;


//////////////////////////////////////////////////////////////////////////////
// PMIMEInfo

/** This class contains the Multipurpose Internet Mail Extensions parameters
   and variables.
 */

class PMIMEInfo : public PStringOptions
{
  public:
    PMIMEInfo() { }

    /// Construct a MIME information dictionary from the specified source.
    PMIMEInfo(
      istream &strm   ///< Stream to read the objects contents from.
    );
    PMIMEInfo(
      PInternetProtocol & socket   ///< Application socket to read MIME info.
    );
    PMIMEInfo(
      const PStringToString & dict
    );
    PMIMEInfo(
      const PString & str
    );


  // Overrides from class PObject
    /** Output the contents of the MIME dictionary to the stream. This is
       primarily used by the standard <code>operator<<</code> function.
     */
    virtual void PrintOn(
      ostream &strm   ///< Stream to print the object into.
    ) const;

    /** Input the contents of the MIME dictionary from the stream. This is
       primarily used by the standard <code>operator>></code> function.
     */
    virtual void ReadFrom(
      istream &strm   ///< Stream to read the objects contents from.
    );


  // New functions for class.
    /** Read MIME information from the socket.

       @return
       true if the MIME information was successfully read.
     */
    PBoolean Read(
      PInternetProtocol & socket   ///< Application socket to read MIME info.
    );

    /** Write MIME information to the socket.

       @return
       true if the MIME information was successfully read.
     */
    PBoolean Write(
      PInternetProtocol & socket   ///< Application socket to write MIME info.
    ) const;

    /**Return a string presentation of the MIME.
      */
    PString AsString() const;

    /**Add a MIME field given a "name: value" format string.
       Note that if the field name was already in the MIME dictionary then
       this will append the new value after a '\\n' character to the previous
       value.

       @return
       true is a field was added.
      */
    bool AddMIME(
      const PString & line
    );
    bool AddMIME(
      const PString & fieldName, ///< MIME field name
      const PString & fieldValue ///< MIME field value
    ) { return InternalAddMIME(fieldName, fieldValue); }
    bool AddMIME(
      const PMIMEInfo & mime
    );

    virtual bool InternalAddMIME(
      const PString & fieldName, ///< MIME field name
      const PString & fieldValue ///< MIME field value
    );

    /** Get a complex MIME field.
        This will parse a complex MIME field of the general form:

           key: base-value;tag1=token;tag2="string";tag3
           key: <base-value>;tag1=token;tag2="string";tag3

        The base-value will be placed in the dictionary where the key is the
        empty string. If the base-value is quoted with '<', '>' brackets then
        the brackets are removed. Note that the string "<>" can be used to have
        an empty base-value but a field starting with a ';' is illegal and this
        function will return false.
        
        Each tag will be the key for it's entry in the dictionary, if that tag
        has no '=' sign then it will have an empty string as its value. If the
        tag value is quoted using '"', then the RFC822 rules are applied and
        the quotes and '\\' charcters removed.

        IF there are multiple "key" entries in the MIME, or there is an entry
        of the form:

           key: <base-value>;tag=token, <base-value1>, <base-value2>;tag=token

        then the first entry wil be as described above. All subsequent entries
        are includedin the dictionary with the key names having the string
        "n:" prepended, e.g. "1:" would be "base-value1", "2:tag" would be the
        tag value on the third entry.

        Returns true if the field exists and base-value is non-empty or quoted.
      */
    bool GetComplex(
      const char * key,    ///< Key into MIME dictionary to get info.
      PStringToString & info  ///< Dictionary of information from field
    ) const { return ParseComplex(GetString(key), info); }
    bool GetComplex(
      const PString & key,    ///< Key into MIME dictionary to get info.
      PStringToString & info  ///< Dictionary of information from field
    ) const { return ParseComplex(GetString(key), info); }
    bool GetComplex(
      const PCaselessString & key,  ///< Key into MIME dictionary to get info.
      PStringToString & info        ///< Dictionary of information from field
    ) const { return ParseComplex(GetString(key), info); }
    bool GetComplex(
      const PCaselessString & (*key)(), ///< Key into MIME dictionary to get info.
      PStringToString & info    ///< Dictionary of information from field
    ) const { return ParseComplex(GetString(key), info); }

    /// Parse the string as a complex field, see GetComplex()
    static bool ParseComplex(
      const PString & str,      ///< String value from MIME field.
      PStringToString & info    ///< Dictionary of information from field
    );

    /** Decode parts from a multipart body using the field value.
      */
    bool DecodeMultiPartList(
      PMultiPartList & parts,   ///< Extracted parts.
      const PString & body,     ///< Body to decode
      const PCaselessString & key       ///< MIME key for multipart info
    ) const;

    /** Decode parts from a multipart body using the field value.
      */
    bool DecodeMultiPartList(
      PMultiPartList & parts,   ///< Extracted parts.
      const PString & body,     ///< Body to decode
      const PCaselessString & (*key)() = ContentTypeTag ///< MIME key for multipart info
    ) const { return DecodeMultiPartList(parts, body, key()); }


    static const PCaselessString & ContentTypeTag();
    static const PCaselessString & ContentDispositionTag();
    static const PCaselessString & ContentTransferEncodingTag();
    static const PCaselessString & ContentDescriptionTag();
    static const PCaselessString & ContentIdTag();

    static const PCaselessString & TextPlain();


    /** Set an association between a file type and a MIME content type. The
       content type is then sent for any file in the directory sub-tree that
       has the same extension.

       Note that if the <code>merge</code> parameter if true then the
       dictionary is merged into the current association list and is not a
       simple replacement.

       The default values placed in this dictionary are:
<pre><code>

          ".txt", "text/plain"
          ".text", "text/plain"
          ".html", "text/html"
          ".htm", "text/html"
          ".aif", "audio/aiff"
          ".aiff", "audio/aiff"
          ".au", "audio/basic"
          ".snd", "audio/basic"
          ".wav", "audio/wav"
          ".gif", "image/gif"
          ".xbm", "image/x-bitmap"
          ".tif", "image/tiff"
          ".tiff", "image/tiff"
          ".jpg", "image/jpeg"
          ".jpe", "image/jpeg"
          ".jpeg", "image/jpeg"
          ".avi", "video/avi"
          ".mpg", "video/mpeg"
          ".mpeg", "video/mpeg"
          ".qt", "video/quicktime"
          ".mov", "video/quicktime"
</code></pre>


       The default content type will be "application/octet-stream".
     */
    static void SetAssociation(
      const PStringToString & allTypes,  ///< MIME content type associations.
      PBoolean merge = true                  ///< Flag for merging associations.
    );
    static void SetAssociation(
      const PString & fileType,         ///< File type (extension) to match.
      const PString & contentType       ///< MIME content type string.
    ) { GetContentTypes().SetAt(fileType, contentType); }

    /** Look up the file type to MIME content type association dictionary and
       return the MIME content type string. If the file type is not found in
       the dictionary then the string "application/octet-stream" is returned.

       @return
       MIME content type for file type.
     */
    static PString GetContentType(
      const PString & fileType   ///< File type (extension) to look up.
    );

    /** Output the contents without the trailing CRLF
     */
    virtual ostream & PrintContents(
      ostream & strm
    ) const;

  private:
    static PStringToString & GetContentTypes();
};


//////////////////////////////////////////////////////////////////////////////
// PMultiPartInfo

/** This object describes the information associated with a multi-part bodies.
  */
class PMultiPartInfo : public PObject
{
    PCLASSINFO(PMultiPartInfo, PObject);
  public:
    PMIMEInfo  m_mime;
    PString    m_textBody;
    PBYTEArray m_binaryBody;
};

class PMultiPartList : public PList<PMultiPartInfo>
{
    PCLASSINFO(PMultiPartList, PList<PMultiPartInfo>);
  public:
    PMultiPartList() { }

    bool Decode(
      const PString & body,               ///< Body to extract parts from
      const PStringToString & contentInfo ///< Content-Type info as decoded from PMIMEInfo::GetComplex()
    );
};


#endif // PTLIB_PMIME_H


// End Of File ///////////////////////////////////////////////////////////////
