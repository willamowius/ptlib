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

#ifdef __GNUC__
#pragma implementation "vcard.h"
#endif

#include <ptlib.h>

#include <ptclib/vcard.h>


static long MaxLineLength = 72;

static PvCard::Separator Space(' ');
static PvCard::Separator Comma(',');
static PvCard::Separator Colon(':');
static PvCard::Separator Semicolon(';');
static PvCard::Separator EndOfLine('\n');

PvCard::PvCard()
  : m_version("3.0")
  , m_birthday(0)
  , m_latitude(91) // > 90 degrees is "not set"
  , m_longitude(181) // > 180 degrees is "not set"
{
}


bool PvCard::IsValid() const
{
  return !m_fullName.IsEmpty() && !m_version.IsEmpty();
}


void PvCard::Token::Validate()
{
  if (IsEmpty())
    return;

  // Must start with a letter
  while (!isalpha(GetAt(0)))
    Delete(0, 1);

  // And rest must be letters, numbers or '-'
  PINDEX i = GetLength();
  while (i-- > 0) {
    char c = GetAt(i);
    if (!isalnum(c) && c != '-')
      Delete(i, 1);
  }

  PAssert(!IsEmpty(), PInvalidParameter);
}


void PvCard::Token::PrintOn(ostream & strm) const
{
  strm.iword(0) += GetLength();
  PCaselessString::PrintOn(strm);
}


void PvCard::Token::ReadFrom(istream & strm)
{
  MakeEmpty();

  while (isspace(strm.peek())) {
    if (strm.get() == '\n' && !isspace(strm.peek())) {
      strm.putback('\n');
      strm.setstate(ios::failbit);
      return;
    }
  }

  istream::int_type c;
  while ((c = strm.get()) == '-' || isalnum(c))
    operator+=((char)c);

  strm.putback((char)c);

  if (IsEmpty())
    strm.setstate(ios::failbit);
}


void PvCard::Separator::PrintOn(ostream & strm) const
{
  strm << m_separator;
  if (m_separator == '\n')
    strm.iword(0) = 0;
  else if (++strm.iword(0) > MaxLineLength) {
    strm << "\n ";
    strm.iword(0) = 1;
  }
}


void PvCard::Separator::ReadFrom(istream & strm)
{
  do {
    strm.get(m_separator);
    switch (m_separator) {
      case '\n' :
        strm.putback('\n');
      case ':' :
      case ';' :
      case ',' :
      case '=' :
        return;
    }
  } while (m_separator < ' ' || isspace(m_separator));

  strm.setstate(ios::failbit);
}


void PvCard::ParamValue::PrintOn(ostream & strm) const
{
  if (FindOneOf("\";:,") == P_MAX_INDEX) {
    strm.iword(0) += GetLength();
    PString::PrintOn(strm);
    return;
  }

  strm << '"';
  PINDEX lastPos = 0, pos;
  while ((pos = Find('"', lastPos)) != P_MAX_INDEX) {
    strm.iword(0) += pos - lastPos + 1;
    strm << string(lastPos, pos-1) << "\\\"";
    lastPos = pos+1;
  }

  strm.iword(0) += GetLength() - lastPos + 2; // Two quotes
  strm << Mid(lastPos) << '"';
}


void PvCard::ParamValue::ReadFrom(istream & strm)
{
  MakeEmpty();

  istream::int_type c;
  do {
    c = strm.get();
    if (c == '\n' && !isspace(strm.peek())) {
      strm.putback('\n');
      return;
    }
  } while (isspace(c));

  if (c != '"') {
    while (c != '\n' && strchr("\";:,", c) == NULL) {
      if (c >= ' ')
        operator+=((char)c);
      c = strm.get();
    }
    strm.putback((char)c);
  }
  else {
    while (c != '"') {
      if (c >= ' ')
        operator+=((char)c);
      c = strm.get();
      if (c == '\n') {
        strm.putback('\n');
        return;
      }
    }
  }
}


void PvCard::ParamValues::PrintOn(ostream & strm) const
{
  for (PINDEX i = 0; i < GetSize(); ++i) {
    if (i > 0)
      strm << Comma;
    strm << (*this)[i];
  }
}


void PvCard::ParamValues::ReadFrom(istream & strm)
{
  PvCard::ParamValue * paramValue = new PvCard::ParamValue;
  strm >> *paramValue;
  while (strm.peek() == ',') {
    strm.ignore();
    Append(paramValue);
    paramValue = new PvCard::ParamValue;
    strm >> *paramValue;
  }
  Append(paramValue);
}


void PvCard::TypeValues::PrintOn(ostream & strm) const
{
  if (IsEmpty())
    return;

  strm << Semicolon << Token("TYPE") << Separator('=');
  ParamValues::PrintOn(strm);
}


void PvCard::TextValue::PrintOn(ostream & strm) const
{
  PINDEX len = GetLength();
  PINDEX lastPos = 0, pos;
  while ((pos = FindOneOf("\n\t ,;", lastPos)) != P_MAX_INDEX) {
    PINDEX chunkSize = pos - lastPos;
    PINDEX lineLeft = MaxLineLength - strm.iword(0);
    if (chunkSize > lineLeft)
      pos = lastPos + lineLeft;

    strm.iword(0) += pos - lastPos;
    strm << operator()(lastPos, pos-1);

    char c = GetAt(pos);
    switch (c) {
      case '\t' :
        strm << Mid(lastPos, MaxLineLength) << EndOfLine << Space;
        break;

      case ',' :
      case ';' :
        strm << '\\';

      default :
        strm << Separator(c);
    }
    lastPos = pos+1;
  }

  PINDEX lineLeft = MaxLineLength - strm.iword(0);
  while (len - lastPos > lineLeft) {
    strm.iword(0) += MaxLineLength;
    strm << Mid(lastPos, lineLeft) << EndOfLine << Space;
    lastPos += MaxLineLength;
    lineLeft = MaxLineLength-1;
  }

  strm.iword(0) += len - lastPos;
  strm << Mid(lastPos);
}


void PvCard::TextValue::ReadFrom(istream & strm)
{
  MakeEmpty();

  bool escaped = false;

  for (;;) {
    istream::int_type c = strm.get();
    switch (c) {
      case EOF :
        return;

      case '\n' :
        if (!isspace(strm.peek())) {
          strm.putback('\n');
          return;
        }
        strm >> ws;
        c = ' ';
        break;

      case 0x7f : // Ignore DEL
        continue;

      case ';' :
      case ',' :
        if (!escaped) {
          strm.putback((char)c);
          return;
        }
        break;

      case '\\' :
        if (escaped)
          break;
        escaped = true;
        break;

      case 'n' :
      case 'N' :
        if (escaped)
          c = '\n';
        break;

      default :
        if (c < ' ')
          continue;
    }

    operator+=((char)c);
    escaped = false;
  }
}


void PvCard::TextValues::PrintOn(ostream & strm) const
{
  for (PINDEX i = 0; i < GetSize(); ++i) {
    if (i > 0)
      strm << Comma;
    strm << (*this)[i];
  }
}


void PvCard::TextValues::ReadFrom(istream & strm)
{
  PvCard::TextValue * textValue = new PvCard::TextValue;
  strm >> *textValue;
  while (strm.peek() == ',') {
    strm.ignore();
    Append(textValue);
    textValue = new PvCard::TextValue;
    strm >> *textValue;
  }
  Append(textValue);
}


void PvCard::URIValue::PrintOn(ostream & strm) const
{
  PString str = AsString();
  strm.iword(0) += str.GetLength();
  strm << str;
}


void PvCard::URIValue::ReadFrom(istream & strm)
{
  PvCard::TextValue value;
  strm >> value;
  if (!Parse(value))
    strm.setstate(ios::failbit);
}


void PvCard::InlineValue::PrintOn(ostream & strm) const
{
  if (GetScheme() != "data")
    strm << Semicolon << TextValue("VALUE=url") << Colon << AsString();
  else {
    strm << Semicolon << TextValue("ENCODING=b");
    PCaselessString contentType = GetParamVars()("type");
    if (contentType.NumCompare("image/") == EqualTo)
      strm << Semicolon << TextValue("TYPE=" + contentType.Mid(6).ToUpper());
    strm << Colon << TextValue(GetContents());
  }
}


void PvCard::InlineValue::ReadFrom(istream & strm)
{
  if (m_params == NULL) {
    strm.setstate(ios::failbit);
    return;
  }

  ParamMap::const_iterator it = m_params->find("VALUE");
  if (it != m_params->end() && it->second.GetValuesIndex(ParamValue("uri")) != P_MAX_INDEX)
    URIValue::ReadFrom(strm);
  else {
    it = m_params->find("ENCODING");
    if (it != m_params->end() && it->second.GetValuesIndex(ParamValue("b")) != P_MAX_INDEX) {
      PvCard::TextValue data;
      strm >> data;
      Parse("data:," + data);
    }
  }

  it = m_params->find("TYPE");
  if (it != m_params->end() && !it->second.IsEmpty())
    SetParamVar("type", "image/" + it->second[0]);

  m_params = NULL;
}


PvCard::InlineValue & PvCard::InlineValue::ReadFromParam(const ParamMap & params)
{
  m_params = &params;
  return *this;
}


void PvCard::MultiValue::SetTypes(const ParamMap & params)
{
  ParamMap::const_iterator it = params.find("TYPE");
  if (it != params.end())
    m_types = it->second;
}


void PvCard::Address::PrintOn(ostream & strm) const
{
  strm << Token(m_label ? "LABEL" : "ADR") << m_types << Colon
       << m_postOfficeBox << Semicolon
       << m_extendedAddress << Semicolon
       << m_street << Semicolon
       << m_locality << Semicolon
       << m_region << Semicolon
       << m_postCode << Semicolon
       << m_country << EndOfLine;
}


void PvCard::Address::ReadFrom(istream & strm)
{
  PvCard::Separator separator;
  strm >> m_postOfficeBox >> separator
       >> m_extendedAddress >> separator
       >> m_street >> separator
       >> m_locality >> separator
       >> m_region >> separator
       >> m_postCode >> separator
       >> m_country;
}


void PvCard::Telephone::PrintOn(ostream & strm) const
{
  strm << Token("TEL") << m_types << Colon << m_number << EndOfLine;
}


void PvCard::EMail::PrintOn(ostream & strm) const
{
  strm << Token("EMAIL") << m_types << Colon << m_address << EndOfLine;
}


void PvCard::PrintOn(ostream & strm) const
{
  if (!IsValid())
    return;

  long previous_iword = strm.iword(0);
  strm.iword(0) = 0;

  switch (strm.width()) {
    case e_XML_XMPP :
    case e_XML_RDF :
    case e_XML_RFC :
      break;

    default :
      if (!m_group.IsEmpty())
        strm << m_group << '.';
      strm << "BEGIN:vCard\n"
           << Token("VERSION") << Colon << m_version << EndOfLine
           << Token("FN") << Colon << m_fullName << EndOfLine;

      if (!m_familyName.IsEmpty() ||
          !m_givenName.IsEmpty() ||
          !m_additionalNames.IsEmpty() ||
          !m_honorificPrefixes.IsEmpty() ||
          !m_honorificSuffixes.IsEmpty())
        strm << Token("N") << Colon
             << m_familyName << Semicolon
             << m_givenName << Semicolon
             << m_additionalNames << Semicolon
             << m_honorificPrefixes << Semicolon
             << m_honorificSuffixes << EndOfLine;

      if (!m_nickNames.IsEmpty())
        strm << Token("NICKNAME") << Colon << m_nickNames << EndOfLine;
      if (!m_sortString.IsEmpty())
        strm << Token("SORT-STRING") << Colon << m_sortString << EndOfLine;
      if (m_birthday.IsValid())
        strm << Token("BDAY") << Colon << m_birthday.AsString("yyyy-MM-dd") << EndOfLine;
      if (!m_url.IsEmpty())
        strm << Token("URL") << Colon << m_url << EndOfLine;
      if (!m_photo.IsEmpty())
        strm << Token("PHOTO") << m_photo << EndOfLine; // Note no ':' is correct
      if (!m_sound.IsEmpty())
        strm << Token("SOUND") << m_sound << EndOfLine; // Note no ':' is correct
      if (!m_timeZone.IsEmpty())
        strm << Token("TZ") << Colon << m_timeZone << EndOfLine;
      if (m_latitude >= -90 && m_latitude <= 90 && m_longitude >= -180 && m_longitude <= 180)
        strm << Token("GEO") << Colon << m_latitude << Semicolon << m_longitude << EndOfLine;
      if (!m_title.IsEmpty())
        strm << Token("TITLE") << Colon << m_title << EndOfLine;
      if (!m_role.IsEmpty())
        strm << Token("ROLE") << Colon << m_role << EndOfLine;
      if (!m_logo.IsEmpty())
        strm << Token("LOGO") << m_logo << EndOfLine; // Note no ':' is correct
      if (!m_agent.IsEmpty())
        strm << Token("AGENT") << Colon << m_agent << EndOfLine;
      if (!m_organisationName.IsEmpty() || !m_organisationUnit.IsEmpty())
        strm << Token("ORG") << Colon << m_organisationName << Semicolon << m_organisationUnit << EndOfLine;
      if (!m_mailer.IsEmpty())
        strm << Token("MAILER") << Colon << m_mailer << EndOfLine;
      if (!m_categories.IsEmpty())
        strm << Token("CATEGORIES") << Colon << m_categories << EndOfLine;
      if (!m_note.IsEmpty())
        strm << Token("NOTE") << Colon << m_note << EndOfLine;
      if (!m_productId.IsEmpty())
        strm << Token("PRODID") << Colon << m_productId << EndOfLine;
      if (!m_guid.IsEmpty())
        strm << Token("UID") << Colon << m_guid << EndOfLine;
      if (!m_revision.IsEmpty())
        strm << Token("REV") << Colon << m_revision << EndOfLine;
      if (!m_class.IsEmpty())
        strm << Token("CLASS") << Colon << m_class << EndOfLine;
      if (!m_publicKey.IsEmpty())
        strm << Token("KEY") << Colon << m_publicKey << EndOfLine;

      strm << m_addresses << m_labels << m_telephoneNumbers << m_emailAddresses;

      for (ExtendedTypeMap::const_iterator it = m_extensions.begin(); it != m_extensions.end(); ++it) {
        strm << it->first;
        for (std::map<Token, ParamValues>::const_iterator ip = it->second.m_parameters.begin();
                                                          ip != it->second.m_parameters.end(); ++ip)
          strm << Semicolon << ip->first << '=' << ip->second;
        strm << Colon << it->second.m_value << EndOfLine;
      }

      if (!m_group.IsEmpty())
        strm << m_group << '.';
      strm << "END:vCard\n";
  }

  strm.iword(previous_iword);
}


void PvCard::ReadFrom(istream & strm)
{
  Token token;
  Separator separator;

  // Look for [group "."] "BEGIN" ":" "VCARD" 1*CRLF
  strm >> token >> separator;
  if (separator == '.') {
    m_group = token;
    strm >> token >> separator;
  }
  if (separator != ':' || token != "BEGIN") {
    strm.setstate(ios::failbit);
    return;
  }

  strm >> token;
  if (token != "VCARD") {
    strm.setstate(ios::failbit);
    return;
  }

  // [group "."] name *(";" param ) ":" value CRLF
  while (!strm.fail()) {
    while (strm.get() != '\n' || isspace(strm.peek())) {
      if (strm.eof())
        return;
    }

    strm >> token >> separator;
    if (separator == '.') {
      if (token != m_group) {
        strm.setstate(ios::failbit);
        return;
      }
      strm >> token >> separator;
      if (strm.fail())
        return;
    }
    Token name = token;
    ExtendedType info;

    while (separator == ';') {
      strm >> token >> separator;
      if (strm.fail())
        return;
      if (separator != '=')
        info.m_parameters[token];
      else
        strm >> info.m_parameters[token] >> separator;
    }

    if (separator != ':' || strm.fail()) {
      strm.setstate(ios::failbit);
      return;
    }

    if (name == "VERSION")
      strm >> m_version;
    else if (name == "FN")
      strm >> m_fullName;
    else  if (name == "N")
      strm >> m_familyName >> separator
           >> m_givenName >> separator
           >> m_additionalNames >> separator
           >> m_honorificPrefixes >> separator
           >> m_honorificSuffixes;
    else  if (name == "NICKNAME")
      strm >> m_nickNames;
    else  if (name == "SORT-STRING")
      strm >> m_sortString;
    else  if (name == "BDAY")
      strm >> m_birthday;
    else  if (name == "URL")
      strm >> m_url;
    else  if (name == "PHOTO")
      strm >> m_photo.ReadFromParam(info.m_parameters);
    else  if (name == "SOUND")
      strm >> m_sound.ReadFromParam(info.m_parameters);
    else  if (name == "TZ")
      strm >> m_timeZone;
    else  if (name == "GEO")
      strm >> m_latitude >> separator >> m_longitude;
    else  if (name == "TITLE")
      strm >> m_title;
    else  if (name == "ROLE")
      strm >> m_role;
    else  if (name == "LOGO")
      strm >> m_logo.ReadFromParam(info.m_parameters);
    else  if (name == "AGENT")
      strm >> m_agent;
    else  if (name == "ORG")
      strm >> m_organisationName >> separator >> m_organisationUnit;
    else  if (name == "MAILER")
      strm >> m_mailer;
    else  if (name == "CATEGORIES")
      strm >> m_categories;
    else  if (name == "NOTE")
      strm >> m_note;
    else  if (name == "PRODID")
      strm >> m_productId;
    else  if (name == "UID")
      strm >> m_guid;
    else  if (name == "REV")
      strm >> m_revision;
    else  if (name == "CLASS")
      strm >> m_class;
    else  if (name == "KEY")
      strm >> m_publicKey;
    else  if (name == "ADR") {
      Address addr(false);
      strm >> addr;
      if (strm.fail())
        return;
      addr.SetTypes(info.m_parameters);
      m_addresses.Append(new Address(addr));
    }
    else  if (name == "LABEL") {
      Address label(true);
      strm >> label;
      if (strm.fail())
        return;
      label.SetTypes(info.m_parameters);
      m_labels.Append(new Address(label));
    }
    else  if (name == "TEL") {
      Telephone tel;
      strm >> tel.m_number;
      if (strm.fail())
        return;
      tel.SetTypes(info.m_parameters);
      m_telephoneNumbers.Append(new Telephone(tel));
    }
    else  if (name == "EMAIL") {
      EMail email;
      strm >> email.m_address;
      if (strm.fail())
        return;
      email.SetTypes(info.m_parameters);
      m_emailAddresses.Append(new EMail(email));
    }
    else if (name == "END") {
      strm >> token;
      if (token == "VCARD") {
        strm.clear(strm.rdstate()&~ios::failbit);
        return;
      }
      strm.setstate(ios::failbit);
    }
    else
      m_extensions[token] = info;
  }
}


bool PvCard::Parse(const PString & str)
{
  PStringStream strm(str);
  strm >> *this;
  return !strm.fail();
}


PString PvCard::AsString(Format fmt)
{
  PStringStream strm;
  strm << setw(fmt) << *this;
  return strm;
}


// End of File ///////////////////////////////////////////////////////////////
