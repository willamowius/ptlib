/*
 * url.cxx
 *
 * URL parsing classes.
 *
 * Portable Tools Library
 *
 * Copyright (c) 1993-2008 Equivalence Pty. Ltd.
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

#ifdef __GNUC__
#pragma implementation "url.h"
#endif

#include <ptlib.h>

#if P_URL

#include <ptclib/url.h>

#include <ptlib/sockets.h>
#include <ptclib/cypher.h>
#include <ctype.h>

#if defined(_WIN32) && !defined(_WIN32_WCE)
#include <shellapi.h>
#pragma comment(lib,"shell32.lib")
#endif


// RFC 1738
// http://host:port/path...
// https://host:port/path....
// gopher://host:port
// wais://host:port
// nntp://host:port
// prospero://host:port
// ftp://user:password@host:port/path...
// telnet://user:password@host:port
// file://hostname/path...

// mailto:user@hostname
// news:string

#define DEFAULT_FTP_PORT      21
#define DEFAULT_TELNET_PORT   23
#define DEFAULT_GOPHER_PORT   70
#define DEFAULT_HTTP_PORT     80
#define DEFAULT_NNTP_PORT     119
#define DEFAULT_WAIS_PORT     210
#define DEFAULT_HTTPS_PORT    443
#define DEFAULT_RTSP_PORT     554
#define DEFAULT_RTSPU_PORT    554
#define DEFAULT_PROSPERO_PORT 1525
#define DEFAULT_H323_PORT     1720
#define DEFAULT_H323S_PORT    1300
#define DEFAULT_H323RAS_PORT  1719
#define DEFAULT_MSRP_PORT     2855
#define DEFAULT_RTMP_PORT     1935
#define DEFAULT_SIP_PORT      5060
#define DEFAULT_SIPS_PORT     5061


//                 schemeName,user,  passwd,host,  defUser,defhost,query, params,frags, path,  rel,   port
PURL_LEGACY_SCHEME(http,      true,  true,  true,  false,  true,   true,  true,  true,  true,  true,  DEFAULT_HTTP_PORT )
PURL_LEGACY_SCHEME(file,      false, false, true,  false,  true,   false, false, false, true,  false, 0)
PURL_LEGACY_SCHEME(https,     false, false, true,  false,  true,   true,  true,  true,  true,  true,  DEFAULT_HTTPS_PORT)
PURL_LEGACY_SCHEME(gopher,    false, false, true,  false,  true,   false, false, false, true,  false, DEFAULT_GOPHER_PORT)
PURL_LEGACY_SCHEME(wais,      false, false, true,  false,  false,  false, false, false, true,  false, DEFAULT_WAIS_PORT)
PURL_LEGACY_SCHEME(nntp,      false, false, true,  false,  true,   false, false, false, true,  false, DEFAULT_NNTP_PORT)
PURL_LEGACY_SCHEME(prospero,  false, false, true,  false,  true,   false, false, false, true,  false, DEFAULT_PROSPERO_PORT)
PURL_LEGACY_SCHEME(rtsp,      false, false, true,  false,  true,   true,  false, false, true,  false, DEFAULT_RTSP_PORT)
PURL_LEGACY_SCHEME(rtspu,     false, false, true,  false,  true,   false, false, false, true,  false, DEFAULT_RTSPU_PORT)
PURL_LEGACY_SCHEME(ftp,       true,  true,  true,  false,  true,   false, false, false, true,  false, DEFAULT_FTP_PORT)
PURL_LEGACY_SCHEME(telnet,    true,  true,  true,  false,  true,   false, false, false, false, false, DEFAULT_TELNET_PORT)
PURL_LEGACY_SCHEME(mailto,    false, false, false, true,   false,  true,  false, false, false, false, 0)
PURL_LEGACY_SCHEME(news,      false, false, false, false,  true,   false, false, false, false, false, 0)
PURL_LEGACY_SCHEME(h323,      true,  false, true,  true,   false,  false, true,  false, false, false, DEFAULT_H323_PORT)
PURL_LEGACY_SCHEME(h323s,     true,  false, true,  true,   false,  false, true,  false, false, false, DEFAULT_H323S_PORT)
PURL_LEGACY_SCHEME(rtmp,      false, false, true,  false,  false,  false, false, false, true,  false, DEFAULT_RTMP_PORT)
PURL_LEGACY_SCHEME(sip,       true,  true,  true,  false,  false,  true,  true,  false, false, false, DEFAULT_SIP_PORT)
PURL_LEGACY_SCHEME(sips,      true,  true,  true,  false,  false,  true,  true,  false, false, false, DEFAULT_SIPS_PORT)
PURL_LEGACY_SCHEME(tel,       false, false, false, true,   false,  false, true,  false, false, false, 0)
PURL_LEGACY_SCHEME(fax,       false, false, false, true,   false,  false, true,  false, false, false, 0)
PURL_LEGACY_SCHEME(callto,    false, false, false, true,   false,  false, true,  false, false, false, 0)
PURL_LEGACY_SCHEME(msrp,      false, false, true,  false,  false,  true,  true,  false, true,  false, DEFAULT_MSRP_PORT)

#define DEFAULT_SCHEME "http"
#define FILE_SCHEME    "file"

//////////////////////////////////////////////////////////////////////////////
// PURL

PURL::PURL()
  : scheme(DEFAULT_SCHEME),
    port(0),
    portSupplied (PFalse),
    relativePath(PFalse)
{
}


PURL::PURL(const char * str, const char * defaultScheme)
{
  Parse(str, defaultScheme);
}


PURL::PURL(const PString & str, const char * defaultScheme)
{
  Parse(str, defaultScheme);
}


PURL::PURL(const PFilePath & filePath)
  : scheme(FILE_SCHEME),
    port(0),
    portSupplied (PFalse),
    relativePath(PFalse)
{
  PStringArray pathArray = filePath.GetDirectory().GetPath();
  if (pathArray.IsEmpty())
    return;

  if (pathArray[0].GetLength() == 2 && pathArray[0][1] == ':')
    pathArray[0][1] = '|';

  pathArray.AppendString(filePath.GetFileName());

  SetPath(pathArray);
}


PObject::Comparison PURL::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PURL), PInvalidCast);
  return urlString.Compare(((const PURL &)obj).urlString);
}

PURL::PURL(const PURL & other)
{
  CopyContents(other);
}

PURL & PURL::operator=(const PURL & other)
{
  CopyContents(other);
  return *this;
}

void PURL::CopyContents(const PURL & other)
{
  urlString    = other.urlString;
  scheme       = other.scheme;
  username     = other.username;
  password     = other.password;
  hostname     = other.hostname;
  port         = other.port;
  portSupplied = other.portSupplied;
  relativePath = other.relativePath;
  path         = other.path;
  fragment     = other.fragment;

  paramVars    = other.paramVars;
  paramVars.MakeUnique();

  queryVars    = other.queryVars;
  queryVars.MakeUnique();

  m_contents   = other.m_contents;
}

PINDEX PURL::HashFunction() const
{
  return urlString.HashFunction();
}


void PURL::PrintOn(std::ostream & stream) const
{
  stream << urlString;
}


void PURL::ReadFrom(std::istream & stream)
{
  PString s;
  stream >> s;
  Parse(s);
}


PString PURL::TranslateString(const PString & str, TranslationType type)
{
  PString xlat = str;

  /* Characters sets are from RFC2396.
     The EBNF defines lowalpha, upalpha, digit and mark which are always
     allowed. The reserved list consisting of ";/?:@&=+$," may or may not be
     allowed depending on the syntatic element being encoded.
   */
  PString safeChars = "abcdefghijklmnopqrstuvwxyz"  // lowalpha
                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"  // upalpha
                      "0123456789"                  // digit
                      "-_.!~*'()";                  // mark
  switch (type) {
    case LoginTranslation :
      safeChars += ";&=+$,";  // Section 3.2.2
      break;

    case PathTranslation :
      safeChars += ":@&=+$,|";   // Section 3.3
      break;

    case ParameterTranslation :
      /* By strict RFC2396/3.3 this should be as for PathTranslation, but many
         URI schemes have parameters of the form key=value so we don't allow
         '=' character in the allowed set. Also, including one of "@,|" is
         incompatible with some schemes, leave those out too. */
      safeChars += ":&+$";
      break;

    case QuotedParameterTranslation :
      safeChars += "[]/:@&=+$,|";
      return str.FindSpan(safeChars) != P_MAX_INDEX ? str.ToLiteral() : str;

    default :
      break;    // Section 3.4, no reserved characters may be used
  }
  PINDEX pos = (PINDEX)-1;
  while ((pos = xlat.FindSpan(safeChars, pos+1)) != P_MAX_INDEX)
    xlat.Splice(psprintf("%%%02X", (BYTE)xlat[pos]), pos, 1);

  return xlat;
}


PString PURL::UntranslateString(const PString & str, TranslationType type)
{
  PString xlat = str;
  xlat.MakeUnique();

  PINDEX pos;
  if (type == PURL::QueryTranslation) {
    /* Even though RFC2396 never mentions this, RFC1630 does. */
    pos = (PINDEX)-1;
    while ((pos = xlat.Find('+', pos+1)) != P_MAX_INDEX)
      xlat[pos] = ' ';
  }

  pos = (PINDEX)-1;
  while ((pos = xlat.Find('%', pos+1)) != P_MAX_INDEX) {
    int digit1 = xlat[pos+1];
    int digit2 = xlat[pos+2];
    if (isxdigit(digit1) && isxdigit(digit2)) {
      xlat[pos] = (char)(
            (isdigit(digit2) ? (digit2-'0') : (toupper(digit2)-'A'+10)) +
           ((isdigit(digit1) ? (digit1-'0') : (toupper(digit1)-'A'+10)) << 4));
      xlat.Delete(pos+1, 2);
    }
  }

  return xlat;
}


void PURL::SplitVars(const PString & str, PStringToString & vars, char sep1, char sep2, TranslationType type)
{
  vars.RemoveAll();

  PINDEX sep1prev = 0;
  do {
    PINDEX sep1next = str.Find(sep1, sep1prev);
    if (sep1next == P_MAX_INDEX)
      sep1next--; // Implicit assumption string is not a couple of gigabytes long ...

    PCaselessString key, data;

    PINDEX sep2pos = str.Find(sep2, sep1prev);
    if (sep2pos > sep1next)
      key = str(sep1prev, sep1next-1);
    else {
      key = str(sep1prev, sep2pos-1);
      if (type != QuotedParameterTranslation)
        data = str(sep2pos+1, sep1next-1);
      else {
        while (isspace(str[++sep2pos]))
          ;
        if (str[sep2pos] != '"')
          data = str(sep2pos, sep1next-1);
        else {
          // find the end quote
          PINDEX endQuote = sep2pos+1;
          do {
            endQuote = str.Find('"', endQuote+1);
            if (endQuote == P_MAX_INDEX) {
              PTRACE(1, "URI\tNo closing double quote in parameter: " << str);
              endQuote = str.GetLength()-1;
              break;
            }
          } while (str[endQuote-1] == '\\');

          data = PString(PString::Literal, str(sep2pos, endQuote));

          if (sep1next < endQuote) {
            sep1next = str.Find(sep1, endQuote);
            if (sep1next == P_MAX_INDEX)
              sep1next--; // Implicit assumption string is not a couple of gigabytes long ...
          }
        }
      }
    }

    key = PURL::UntranslateString(key, type);
    if (!key) {
      data = PURL::UntranslateString(data, type);
      if (vars.Contains(key))
        vars.SetAt(key, vars[key] + '\n' + data);
      else
        vars.SetAt(key, data);
    }

    sep1prev = sep1next+1;
  } while (sep1prev != P_MAX_INDEX);
}


void PURL::OutputVars(std::ostream & strm,
                      const PStringToString & vars,
                      char sep0,
                      char sep1,
                      char sep2,
                      TranslationType type)
{
  for (PINDEX i = 0; i < vars.GetSize(); i++) {
    if (i > 0)
      strm << sep1;
    else if (sep0 != '\0')
      strm << sep0;

    PString key  = TranslateString(vars.GetKeyAt (i), type);
    PString data = TranslateString(vars.GetDataAt(i), type);

    if (key.IsEmpty())
      strm << data;
    else if (data.IsEmpty())
      strm << key;
    else
      strm << key << sep2 << data;
  }
}


PBoolean PURL::InternalParse(const char * cstr, const char * defaultScheme)
{
  scheme.MakeEmpty();
  username.MakeEmpty();
  password.MakeEmpty();
  hostname.MakeEmpty();
  port = 0;
  portSupplied = PFalse;
  relativePath = PFalse;
  path.SetSize(0);
  paramVars.RemoveAll();
  fragment.MakeEmpty();
  queryVars.RemoveAll();
  m_contents.MakeEmpty();

  if (cstr == NULL)
    return false;

  // copy the string so we can take bits off it
  while (((*cstr & 0x80) == 0x00) && isspace(*cstr))
    cstr++;
  PString url = cstr;
  if (url.IsEmpty())
    return false;

  // get information which tells us how to parse URL for this
  // particular scheme
  PURLScheme * schemeInfo = NULL;

  // Character set as per RFC2396
  //    scheme        = alpha *( alpha | digit | "+" | "-" | "." )
  if (isalpha(url[0])) {
    PINDEX pos = 1;
    while (isalnum(url[pos]) || url[pos] == '+' || url[pos] == '-' || url[pos] == '.')
      ++pos;

    // Determine if the URL has an explicit scheme
    if (url[pos] == ':') {
      // get the scheme information
      schemeInfo = PURLSchemeFactory::CreateInstance(url.Left(pos));
      if (schemeInfo != NULL)
        url.Delete(0, pos+1);
    }
  }

  // if we could not match a scheme, then use the specified default scheme
  if (schemeInfo == NULL && defaultScheme != NULL) {
    schemeInfo = PURLSchemeFactory::CreateInstance(defaultScheme);
    PAssert(schemeInfo != NULL, "Default scheme " + PString(defaultScheme) + " not available");
  }

  // if that still fails, then there is nowehere to go
  if (schemeInfo == NULL)
    return false;

  scheme = schemeInfo->GetName();
  return schemeInfo->Parse(url, *this) && !IsEmpty();
}

PBoolean PURL::LegacyParse(const PString & _url, const PURLLegacyScheme * schemeInfo)
{
  PString url = _url;
  PINDEX pos;

  // Super special case!
  if (scheme *= "callto") {

    // Actually not part of MS spec, but a lot of people put in the // into
    // the URL, so we take it out of it is there.
    if (url.GetLength() > 2 && url[0] == '/' && url[1] == '/')
      url.Delete(0, 2);

    // For some bizarre reason callto uses + instead of ; for paramters
    // We do a loop so that phone numbers of the form +61243654666 still work
    do {
      pos = url.Find('+');
    } while (pos != P_MAX_INDEX && isdigit(url[pos+1]));

    if (pos != P_MAX_INDEX) {
      SplitVars(url(pos+1, P_MAX_INDEX), paramVars, '+', '=');
      url.Delete(pos, P_MAX_INDEX);
    }

    hostname = paramVars("gateway");
    if (!hostname)
      username = UntranslateString(url, LoginTranslation);
    else {
      PCaselessString type = paramVars("type");
      if (type == "directory") {
        pos = url.Find('/');
        if (pos == P_MAX_INDEX)
          username = UntranslateString(url, LoginTranslation);
        else {
          hostname = UntranslateString(url.Left(pos), LoginTranslation);
          username = UntranslateString(url.Mid(pos+1), LoginTranslation);
        }
      }
      else {
        // Now look for an @ and split user and host
        pos = url.Find('@');
        if (pos != P_MAX_INDEX) {
          username = UntranslateString(url.Left(pos), LoginTranslation);
          hostname = UntranslateString(url.Mid(pos+1), LoginTranslation);
        }
        else {
          if (type == "ip" || type == "host")
            hostname = UntranslateString(url, LoginTranslation);
          else
            username = UntranslateString(url, LoginTranslation);
        }
      }
    }

    // Allow for [ipv6] form
    pos = hostname.Find(']');
    if (pos == P_MAX_INDEX)
      pos = 0;
    pos = hostname.Find(':', pos);
    if (pos != P_MAX_INDEX) {
      port = (WORD)hostname.Mid(pos+1).AsUnsigned();
      portSupplied = PTrue;
      hostname.Delete(pos, P_MAX_INDEX);
    }

    password = paramVars("password");
    return PTrue;
  }

  // if the URL should have leading slash, then remove it if it has one
  if (schemeInfo != NULL && schemeInfo->hasHostPort && schemeInfo->hasPath) {
    if (url.GetLength() > 2 && url[0] == '/' && url[1] == '/')
      url.Delete(0, 2);
    else
      relativePath = PTrue;
  }

  if (schemeInfo == NULL)
    return PFalse;

  // parse user/password/host/port
  if (!relativePath && schemeInfo->hasHostPort) {
    PString endHostChars;
    if (schemeInfo->hasPath)
      endHostChars += '/';
    if (schemeInfo->hasQuery)
      endHostChars += '?';
    if (schemeInfo->hasParameters)
      endHostChars += ';';
    if (schemeInfo->hasFragments)
      endHostChars += '#';
    if (endHostChars.IsEmpty())
      pos = P_MAX_INDEX;
    else if (schemeInfo->hasUsername) {
      //';' showing in the username field should be valid.
      // Looking for ';' after the '@' for the parameters.
      PINDEX posAt = url.Find('@');
      if (posAt != P_MAX_INDEX)
        pos = url.FindOneOf(endHostChars, posAt);
      else 
        pos = url.FindOneOf(endHostChars);
    }
    else
      pos = url.FindOneOf(endHostChars);

    PString uphp = url.Left(pos);
    if (pos != P_MAX_INDEX)
      url.Delete(0, pos);
    else
      url.MakeEmpty();

    // if the URL is of type UserPasswordHostPort, then parse it
    if (schemeInfo->hasUsername) {
      // extract username and password
      PINDEX pos2 = uphp.Find('@');
      PINDEX pos3 = P_MAX_INDEX;
      if (schemeInfo->hasPassword)
        pos3 = uphp.Find(':');
      switch (pos2) {
        case 0 :
          uphp.Delete(0, 1);
          break;

        case P_MAX_INDEX :
          if (schemeInfo->defaultToUserIfNoAt) {
            if (pos3 == P_MAX_INDEX)
              username = UntranslateString(uphp, LoginTranslation);
            else {
              username = UntranslateString(uphp.Left(pos3), LoginTranslation);
              password = UntranslateString(uphp.Mid(pos3+1), LoginTranslation);
            }
            uphp.MakeEmpty();
          }
          break;

        default :
          if (pos3 > pos2)
            username = UntranslateString(uphp.Left(pos2), LoginTranslation);
          else {
            username = UntranslateString(uphp.Left(pos3), LoginTranslation);
            password = UntranslateString(uphp(pos3+1, pos2-1), LoginTranslation);
          }
          uphp.Delete(0, pos2+1);
      }
    }

    // if the URL does not have a port, then this is the hostname
    if (schemeInfo->defaultPort == 0)
      hostname = UntranslateString(uphp, LoginTranslation);
    else {
      // determine if the URL has a port number
      // Allow for [ipv6] form
      pos = uphp.Find(']');
      if (pos == P_MAX_INDEX)
        pos = 0;
      pos = uphp.Find(':', pos);
      if (pos == P_MAX_INDEX)
        hostname = UntranslateString(uphp, LoginTranslation);
      else {
        hostname = UntranslateString(uphp.Left(pos), LoginTranslation);
        port = (WORD)uphp.Mid(pos+1).AsUnsigned();
        portSupplied = PTrue;
      }

      if (hostname.IsEmpty() && schemeInfo->defaultHostToLocal)
        hostname = PIPSocket::GetHostName();
    }
  }

  if (schemeInfo->hasQuery) {
    // chop off any trailing query
    pos = url.Find('?');
    if (pos != P_MAX_INDEX) {
      SplitQueryVars(url(pos+1, P_MAX_INDEX), queryVars);
      url.Delete(pos, P_MAX_INDEX);
    }
  }

  if (schemeInfo->hasParameters) {
    // chop off any trailing parameters
    pos = url.Find(';');
    if (pos != P_MAX_INDEX) {
      SplitVars(url(pos+1, P_MAX_INDEX), paramVars);
      url.Delete(pos, P_MAX_INDEX);
    }
  }

  if (schemeInfo->hasFragments) {
    // chop off any trailing fragment
    pos = url.Find('#');
    if (pos != P_MAX_INDEX) {
      fragment = UntranslateString(url(pos+1, P_MAX_INDEX), PathTranslation);
      url.Delete(pos, P_MAX_INDEX);
    }
  }

  if (schemeInfo->hasPath)
    SetPathStr(url);   // the hierarchy is what is left
  else {
    // if the rest of the URL isn't a path, then we are finished!
    m_contents = UntranslateString(url, PathTranslation);
    Recalculate();
  }

  if (port == 0 && schemeInfo->defaultPort != 0 && !relativePath) {
    // Yes another horrible, horrible special case!
    if (scheme == "h323" && paramVars("type") == "gk")
      port = DEFAULT_H323RAS_PORT;
    else
      port = schemeInfo->defaultPort;
    Recalculate();
  }

  return PTrue;
}


PFilePath PURL::AsFilePath() const
{
  /* While it is never explicitly stated anywhere in RFC1798, there is an
     implication RFC 1808 that the path is absolute unless the relative
     path rules of that RFC apply. We follow that logic. */

  if (path.IsEmpty() || scheme != FILE_SCHEME || (!hostname.IsEmpty() && hostname != "localhost"))
    return PString::Empty();

  PStringStream str;

  if (path[0].GetLength() == 2 && path[0][1] == '|')
    str << path[0][0] << ':' << PDIR_SEPARATOR; // Special case for Windows paths with drive letter
  else {
    if (!relativePath)
      str << PDIR_SEPARATOR;
    str << path[0];
  }

  for (PINDEX i = 1; i < path.GetSize(); i++)
    str << PDIR_SEPARATOR << path[i];

  return str;
}


PString PURL::AsString(UrlFormat fmt) const
{
  if (fmt == FullURL)
    return urlString;

  if (scheme.IsEmpty())
    return PString::Empty();

  const PURLScheme * schemeInfo = PURLSchemeFactory::CreateInstance(scheme);
  if (schemeInfo == NULL)
    schemeInfo = PURLSchemeFactory::CreateInstance(DEFAULT_SCHEME);

  return schemeInfo->AsString(fmt, *this);
}

PString PURL::LegacyAsString(PURL::UrlFormat fmt, const PURLLegacyScheme * schemeInfo) const
{
  PStringStream str;

  if (fmt == HostPortOnly) {
    str << scheme << ':';

    if (relativePath) {
      if (schemeInfo->relativeImpliesScheme)
        return PString::Empty();
      return str;
    }

    if (schemeInfo->hasPath && schemeInfo->hasHostPort)
      str << "//";

    if (schemeInfo->hasUsername) {
      if (!username) {
        str << TranslateString(username, LoginTranslation);
        if (schemeInfo->hasPassword && !password)
          str << ':' << TranslateString(password, LoginTranslation);
        str << '@';
      }
    }

    if (schemeInfo->hasHostPort) {
      if (hostname.Find(':') != P_MAX_INDEX && hostname[0] != '[')
        str << '[' << hostname << ']';
      else
        str << hostname;
    }

    if (schemeInfo->defaultPort != 0) {
      if (port != schemeInfo->defaultPort || portSupplied)
        str << ':' << port;
    }

    // Problem was fixed for handling legacy schema like tel URI.
    // HostPortOnly format: if there is no default user and host fields, only the schema itself is being returned.
    // URIOnly only format: the pathStr will be retruned.
    // The Recalculate() will merge both HostPortOnly and URIOnly formats for the completed uri string creation.
    if (schemeInfo->defaultToUserIfNoAt)
      return str;

    if (str.GetLength() > scheme.GetLength()+1)
      return str;

    // Cannot JUST have the scheme: ....
    return PString::Empty();
  }

  // URIOnly and PathOnly
  if (schemeInfo->hasPath)
    str << GetPathStr();
  else
    str << TranslateString(m_contents, PathTranslation);

  if (fmt == URIOnly) {
    if (!fragment)
      str << "#" << TranslateString(fragment, PathTranslation);

    OutputVars(str, paramVars, ';', ';', '=', ParameterTranslation);
    OutputVars(str, queryVars, '?', '&', '=', QueryTranslation);
  }

  return str;
}


void PURL::SetScheme(const PString & s)
{
  scheme = s;
  Recalculate();
}


void PURL::SetUserName(const PString & u)
{
  username = u;
  Recalculate();
}


void PURL::SetPassword(const PString & p)
{
  password = p;
  Recalculate();
}


void PURL::SetHostName(const PString & h)
{
  hostname = h;
  Recalculate();
}


void PURL::SetPort(WORD newPort)
{
  port = newPort;
  portSupplied = true;
  Recalculate();
}


void PURL::SetPathStr(const PString & pathStr)
{
  path = pathStr.Tokenise("/", PTrue);

  if (path.GetSize() > 0 && path[0].IsEmpty()) 
    path.RemoveAt(0);

  for (PINDEX i = 0; i < path.GetSize(); i++) {
    path[i] = UntranslateString(path[i], PathTranslation);
    if (i > 0 && path[i] == ".." && path[i-1] != "..") {
      path.RemoveAt(i--);
      path.RemoveAt(i--);
    }
  }

  Recalculate();
}


PString PURL::GetPathStr() const
{
  PStringStream strm;
  for (PINDEX i = 0; i < path.GetSize(); i++) {
    if (i > 0 || !relativePath)
      strm << '/';
    strm << TranslateString(path[i], PathTranslation);
  }
  return strm;
}


void PURL::SetPath(const PStringArray & p)
{
  path = p;
  Recalculate();
}


void PURL::AppendPath(const PString & segment)
{
  path.AppendString(segment);
  Recalculate();
}


PString PURL::GetParameters() const
{
  PStringStream strm;
  OutputVars(strm, paramVars, '\0', ';', '=', ParameterTranslation);
  return strm;
}


void PURL::SetParameters(const PString & parameters)
{
  SplitVars(parameters, paramVars);
  Recalculate();
}


void PURL::SetParamVars(const PStringToString & p)
{
  paramVars = p;
  Recalculate();
}


void PURL::SetParamVar(const PString & key, const PString & data, bool emptyDataDeletes)
{
  if (emptyDataDeletes && data.IsEmpty())
    paramVars.RemoveAt(key);
  else
    paramVars.SetAt(key, data);
  Recalculate();
}


PString PURL::GetQuery() const
{
  PStringStream strm;
  OutputVars(strm, queryVars, '\0', '&', '=', QueryTranslation);
  return strm;
}


void PURL::SetQuery(const PString & queryStr)
{
  SplitQueryVars(queryStr, queryVars);
  Recalculate();
}


void PURL::SetQueryVars(const PStringToString & q)
{
  queryVars = q;
  Recalculate();
}


void PURL::SetQueryVar(const PString & key, const PString & data)
{
  if (data.IsEmpty())
    queryVars.RemoveAt(key);
  else
    queryVars.SetAt(key, data);
  Recalculate();
}


void PURL::SetContents(const PString & str)
{
  m_contents = str;
  Recalculate();
}


bool PURL::LoadResource(PString & str, const PString & requiredContentType) const
{
  PURLLoader * loader = PURLLoaderFactory::CreateInstance(GetScheme());
  return loader != NULL && loader->Load(*this, str, requiredContentType);
}


bool PURL::LoadResource(PBYTEArray & data, const PString & requiredContentType) const
{
  PURLLoader * loader = PURLLoaderFactory::CreateInstance(GetScheme());
  return loader != NULL && loader->Load(*this, data, requiredContentType);
}


bool PURL::OpenBrowser(const PString & url)
{
#ifdef _WIN32
  SHELLEXECUTEINFO sei;
  ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
  sei.cbSize = sizeof(SHELLEXECUTEINFO);
  sei.lpVerb = TEXT("open");
  PVarString file = url;
  sei.lpFile = file;

  if (ShellExecuteEx(&sei) != 0)
    return true;

  PVarString msg = "Unable to open page" & url;
  PVarString name = PProcess::Current().GetName();
  MessageBox(NULL, msg, name, MB_TASKMODAL);

#endif // WIN32
  return false;
}


void PURL::Recalculate()
{
  if (scheme.IsEmpty())
    scheme = DEFAULT_SCHEME;

  urlString = AsString(HostPortOnly) + AsString(URIOnly);
}


///////////////////////////////////////////////////////////////////////////////

// RFC3966 tel URI

class PURL_TelScheme : public PURLScheme
{
    PCLASSINFO(PURL_TelScheme, PURLScheme);
  public:
    virtual PString GetName() const
    {
      return "tel";
    }

    virtual PBoolean Parse(const PString & str, PURL & url) const
    {
      PINDEX pos = str.FindSpan("0123456789*#", str[0] != '+' ? 0 : 1);
      if (pos == P_MAX_INDEX)
        url.SetUserName(str);
      else {
        if (str[pos] != ';')
          return false;

        url.SetUserName(str.Left(pos));

        PStringToString paramVars;
        PURL::SplitVars(str(pos+1, P_MAX_INDEX), paramVars);
        url.SetParamVars(paramVars);

        PString phoneContext = paramVars("phone-context");
        if (phoneContext.IsEmpty()) {
          if (str[0] != '+')
            return false;
        }
        else if (phoneContext[0] != '+')
          url.SetHostName(phoneContext);
        else if (str[0] != '+')
          url.SetUserName(phoneContext+url.GetUserName());
        else
          return false;
      }

      return url.GetUserName() != "+";
    }

    virtual PString AsString(PURL::UrlFormat fmt, const PURL & url) const
    {
      if (fmt == PURL::HostPortOnly)
        return PString::Empty();

      PStringStream strm;
      strm << "tel:" + url.GetUserName();
      PURL::OutputVars(strm, url.GetParamVars(), ';', ';', '=', PURL::ParameterTranslation);
      return strm;
    }
};

static PURLSchemeFactory::Worker<PURL_TelScheme> telScheme("tel", true);


///////////////////////////////////////////////////////////////////////////////

// RFC2397 data URI

class PURL_DataScheme : public PURLScheme
{
    PCLASSINFO(PURL_DataScheme, PURLScheme);
  public:
    virtual PString GetName() const
    {
      return "data";
    }

    virtual PBoolean Parse(const PString & url, PURL & purl) const
    {
      PINDEX comma = url.Find(',');
      if (comma == P_MAX_INDEX)
        return false;

      PINDEX semi = url.Find(';');
      if (semi > comma)
        purl.SetParamVar("type", url.Left(comma));
      else {
        purl.SetParameters(url(semi, comma-1));
        purl.SetParamVar("type", url.Left(semi));
      }

      purl.SetContents(url.Mid(comma+1));

      return true;
    }

    virtual PString AsString(PURL::UrlFormat fmt, const PURL & purl) const
    {
      if (fmt == PURL::HostPortOnly)
        return PString::Empty();

      const PStringToString & params = purl.GetParamVars();
      PStringStream strm;

      strm << "data:" + params("type", "text/plain");

      bool base64 = false;
      for (PINDEX i = 0; i < params.GetSize(); i++) {
        PCaselessString key = params.GetKeyAt(i);
        if (key == "type")
          continue;
        if (key == "base64") {
          base64 = true;
          continue;
        }

        strm << ';' << PURL::TranslateString(key, PURL::ParameterTranslation);

        PString data = params.GetDataAt(i);
        if (!data)
          strm << '=' << PURL::TranslateString(data, PURL::ParameterTranslation);
      }

      // This must always be last according to EBNF
      if (base64)
        strm << ";base64";

      strm << ',' << PURL::TranslateString(purl.GetContents(), PURL::ParameterTranslation);

      return strm;
    }
};

static PURLSchemeFactory::Worker<PURL_DataScheme> dataScheme("data", true);


///////////////////////////////////////////////////////////////////////////////

class PURL_FileLoader : public PURLLoader
{
    PCLASSINFO(PURL_FileLoader, PURLLoader);
  public:
    virtual bool Load(const PURL & url, PString & str, const PString &)
    {
      PTextFile file;
      if (!file.Open(url.AsFilePath()))
        return false;
      if (!str.SetSize(file.GetLength()+1))
        return false;
      return file.Read(str.GetPointer(), str.GetSize()-1);
    }

    virtual bool Load(const PURL & url, PBYTEArray & data, const PString &)
    {
      PFile file;
      if (!file.Open(url.AsFilePath()))
        return false;
      if (!data.SetSize(file.GetLength()))
        return false;
      return file.Read(data.GetPointer(), data.GetSize());
    }
};

PFACTORY_CREATE(PURLLoaderFactory, PURL_FileLoader, "file", true);


///////////////////////////////////////////////////////////////////////////////

class PURL_DataLoader : public PURLLoader
{
    PCLASSINFO(PURL_FileLoader, PURLLoader);
  public:
    virtual bool Load(const PURL & url, PString & str, const PString & requiredContentType)
    {
      if (!requiredContentType.IsEmpty()) {
        PCaselessString actualContentType = url.GetParamVars()("type");
        if (!actualContentType.IsEmpty() && requiredContentType != requiredContentType)
          return false;
      }

      str = url.GetContents();
      return true;
    }

    virtual bool Load(const PURL & url, PBYTEArray & data, const PString & requiredContentType)
    {
      if (!requiredContentType.IsEmpty()) {
        PCaselessString actualContentType = url.GetParamVars()("type");
        if (!actualContentType.IsEmpty() && requiredContentType != requiredContentType)
          return false;
      }

      if (url.GetParamVars().Contains("base64"))
        return PBase64::Decode(url.GetContents(), data);

      PString str = url.GetContents();
      PINDEX len = str.GetLength();
      if (!data.SetSize(len))
        return false;

      memcpy(data.GetPointer(), (const char *)str, len);
      return true;
    }
};

PFACTORY_CREATE(PURLLoaderFactory, PURL_DataLoader, "data", true);

#endif // P_URL


// End Of File ///////////////////////////////////////////////////////////////
