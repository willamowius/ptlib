/*
 * vcard.h
 *
 * Class to represent and parse a vCard as per RFC2426
 *
 * Portable Windows Library
 *
 * Copyright (c) 2010 Vox Lucida Pty Ltd
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
 * The Original Code is Portable Tools Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida Pty Ltd
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_VCARD_H
#define PTLIB_VCARD_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <ptclib/url.h>


/**Class to represent a vCard as per RFC2426.
  */
class PvCard : public PObject
{
    PCLASSINFO(PvCard, PObject);

  public:
    PvCard();

    bool IsValid() const;

    virtual void PrintOn(
      ostream & strm
    ) const;
    virtual void ReadFrom(
      istream & strm
    );
    bool Parse(
      const PString & str
    );

    /** Output string formats.
        If operator<< or PrintOn() is used the stream width() parameter
        may be set to this to indicate the output format. e.g.
        <CODE>
           stream << setw(PvCard::e_XML_XMPP) << card;
        </CODE>
      */
    enum Format {
      e_Standard, ///< As per RFC2425
      e_XML_XMPP, ///< Jabber XML
      e_XML_RDF,  ///< W3C version
      e_XML_RFC   ///< Draft RFC
    };
    PString AsString(
      Format fmt = e_Standard
    );

    /// Representation of token (EBNF group, name, iana-token or x-name)
    class Token : public PCaselessString
    {
      public:
        Token(const char * str = NULL) : PCaselessString(str) { Validate(); }
        Token(const PString & str) : PCaselessString(str) { Validate(); }
        Token & operator=(const char * str) { PCaselessString::operator=(str); Validate(); return *this; }
        Token & operator=(const PString & str) { PCaselessString::operator=(str); Validate(); return *this; }
        virtual void PrintOn(ostream & strm) const;
        virtual void ReadFrom(istream & strm);
      private:
        void Validate();
    };

    class Separator : public PObject
    {
      public:
        Separator(char c = '\0') : m_separator(c) { }
        virtual void PrintOn(ostream & strm) const;
        virtual void ReadFrom(istream & strm);
        bool operator==(char c) const { return m_separator == c; }
        bool operator!=(char c) const { return m_separator != c; }
        char m_separator;
    };

    /// Representation of EBNF param-value
    class ParamValue : public PString
    {
      public:
        ParamValue(const char * str = NULL) : PString(str) { }
        ParamValue(const PString & str) : PString(str) { }
        virtual void PrintOn(ostream & strm) const;
        virtual void ReadFrom(istream & strm);
    };
    /// Comma separated list of param-value's
    class ParamValues : public PArray<ParamValue>
    {
      public:
        virtual void PrintOn(ostream & strm) const;
        virtual void ReadFrom(istream & strm);
    };

    typedef std::map<Token, ParamValues> ParamMap;

    class TypeValues : public ParamValues
    {
      public:
        TypeValues() { }
        TypeValues(const ParamValues & values) : ParamValues(values) { }
        virtual void PrintOn(ostream & strm) const;
    };

    /// Representation of EBNF text-value
    class TextValue : public PString
    {
      public:
        TextValue(const char * str = NULL) : PString(str) { }
        TextValue(const PString & str) : PString(str) { }
        virtual void PrintOn(ostream & strm) const;
        virtual void ReadFrom(istream & strm);
    };

    /// Comma separated list of text-value's
    class TextValues : public PArray<TextValue>
    {
      public:
        virtual void PrintOn(ostream & strm) const;
        virtual void ReadFrom(istream & strm);
    };

    class URIValue : public PURL
    {
      public:
        URIValue(const char * str = NULL) : PURL(str) { }
        URIValue(const PString & str) : PURL(str) { }
        virtual void PrintOn(ostream & strm) const;
        virtual void ReadFrom(istream & strm);
    };

    /// Representation of EBNF img-inline-value/snd-inline-value
    class InlineValue : public URIValue
    {
      public:
        InlineValue(const char * str = NULL) : URIValue(str), m_params(NULL) { }
        InlineValue(const PString & str) : URIValue(str), m_params(NULL) { }
        virtual void PrintOn(ostream & strm) const;
        virtual void ReadFrom(istream & strm);
        InlineValue & ReadFromParam(const ParamMap & params);
      private:
        const ParamMap * m_params;
    };

    Token       m_group;
    TextValue   m_fullName; // Mandatory
    TextValue   m_version;  // Mandatory

    TextValue   m_familyName;
    TextValue   m_givenName;
    TextValues  m_additionalNames;
    TextValue   m_honorificPrefixes;
    TextValue   m_honorificSuffixes;
    TextValues  m_nickNames;
    TextValue   m_sortString; // Form of name for sorting, e.g. family name;

    PTime       m_birthday;
    URIValue    m_url;
    InlineValue m_photo;   // Possibly embedded via data: scheme
    InlineValue m_sound;   // Possibly embedded via data: scheme
    TextValue   m_timeZone;
    double      m_latitude;
    double      m_longitude;

    TextValue   m_title;
    TextValue   m_role;
    InlineValue m_logo;   // Possibly embedded via data: scheme
    TextValue   m_agent;
    TextValue   m_organisationName;
    TextValue   m_organisationUnit;

    TextValue   m_mailer;
    TextValues  m_categories;
    TextValue   m_note;

    TextValue   m_productId;
    TextValue   m_guid;
    TextValue   m_revision;
    TextValue   m_class;
    TextValue   m_publicKey;

    struct MultiValue : public PObject {
      MultiValue() { }
      MultiValue(const PString & type) { m_types.Append(new ParamValue(type)); }

      TypeValues m_types;     // e.g. "home", "work", "pref" etc
      void SetTypes(const ParamMap & params);
    };

    struct Address : public MultiValue {
      Address(bool label = false) : m_label(label) { }
      virtual void PrintOn(ostream & strm) const;
      virtual void ReadFrom(istream & strm);

      bool        m_label;
      TextValue   m_postOfficeBox;
      TextValue   m_extendedAddress;
      TextValue   m_street;    // Including number "123 Main Street"
      TextValue   m_locality;  // Suburb/city
      TextValue   m_region;    // State/province
      TextValue   m_postCode;
      TextValue   m_country;
    };
    PArray<Address> m_addresses;
    PArray<Address> m_labels;

    struct Telephone : public MultiValue {
      Telephone() { }
      Telephone(const PString & number, const PString & type = PString::Empty())
        : MultiValue(type)
        , m_number(number)
      { }
      virtual void PrintOn(ostream & strm) const;

      TextValue m_number;
    };
    PArray<Telephone> m_telephoneNumbers;

    struct EMail : public MultiValue {
      EMail() { }
      EMail(const PString & address, const PString & type = PString::Empty())
        : MultiValue(type)
        , m_address(address)
      { }
      virtual void PrintOn(ostream & strm) const;
      TextValue   m_address;
    };
    PArray<EMail> m_emailAddresses;

    struct ExtendedType {
      ParamMap  m_parameters;
      TextValue m_value;
    };

    typedef std::map<Token, ExtendedType> ExtendedTypeMap;
    ExtendedTypeMap m_extensions;
};


#endif  // PTLIB_VCARD_H


// End of File ///////////////////////////////////////////////////////////////
