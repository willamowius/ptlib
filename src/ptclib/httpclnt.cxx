/*
 * httpclnt.cxx
 *
 * HTTP Client class.
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

#include <ptlib.h>

#if P_HTTP

#include <ptlib/sockets.h>
#include <ptclib/http.h>
#include <ptclib/guid.h>

#if P_SSL
#include <ptclib/pssl.h>
#endif

#include <ctype.h>

////////////////////////////////////////////////////////////////////////////////////

static PHTTPClientAuthenticationFactory::Worker<PHTTPClientBasicAuthentication> httpClient_basicAuthenticator("basic");
static PHTTPClientAuthenticationFactory::Worker<PHTTPClientDigestAuthentication> httpClient_md5Authenticator("digest");
static const char * const AlgorithmNames[PHTTPClientDigestAuthentication::NumAlgorithms] = {
  "MD5"
};

#define new PNEW


static __inline bool IsOK(int response) { return (response/100) == 2; }

static PINDEX MaxTraceContentSize = 1000;


//////////////////////////////////////////////////////////////////////////////
// PHTTPClient

PHTTPClient::PHTTPClient(const PString & userAgent)
  : m_userAgentName(userAgent)
  , m_persist(true)
  , m_authentication(NULL)
{
}


int PHTTPClient::ExecuteCommand(Commands cmd,
                                const PURL & url,
                                PMIMEInfo & outMIME,
                                const PString & dataBody,
                                PMIMEInfo & replyMime)
{
  return ExecuteCommand(commandNames[cmd], url, outMIME, dataBody, replyMime);
}


int PHTTPClient::ExecuteCommand(const PString & cmdName,
                                const PURL & url,
                                PMIMEInfo & outMIME,
                                const PString & dataBody,
                                PMIMEInfo & replyMIME)
{
  if (!outMIME.Contains(DateTag()))
    outMIME.SetAt(DateTag(), PTime().AsString());

  if (!m_userAgentName && !outMIME.Contains(UserAgentTag()))
    outMIME.SetAt(UserAgentTag(), m_userAgentName);

  if (m_persist)
    outMIME.SetAt(ConnectionTag(), KeepAliveTag());

  bool needAuthentication = true;
  PURL adjustableURL = url;
  for (PINDEX retry = 0; retry < 3; retry++) {
    if (!AssureConnect(adjustableURL, outMIME))
      break;

    if (!WriteCommand(cmdName, url.AsString(PURL::URIOnly), outMIME, dataBody)) {
      lastResponseCode = -1;
      lastResponseInfo = GetErrorText(LastWriteError);
      break;
    }

    // If not persisting need to shut down write so other end stops reading
    if (!m_persist)
      Shutdown(ShutdownWrite);

    // Await a response, if all OK exit loop
    if (ReadResponse(replyMIME) && (lastResponseCode != Continue || ReadResponse(replyMIME))) {
      switch (lastResponseCode) {
        case MovedPermanently:
        case MovedTemporarily:
          adjustableURL = replyMIME("Location");
          if (!adjustableURL.IsEmpty())
            return lastResponseCode;
          break;

        case UnAuthorised:
          if (needAuthentication && replyMIME.Contains("WWW-Authenticate") && !(m_userName.IsEmpty() && m_password.IsEmpty())) {
            needAuthentication = false;

            // authenticate 
            PString errorMsg;
            PHTTPClientAuthentication * newAuth = PHTTPClientAuthentication::ParseAuthenticationRequired(false, replyMIME, errorMsg);
            if (newAuth == NULL)
              return false;

            newAuth->SetUsername(m_userName);
            newAuth->SetPassword(m_password);

            delete m_authentication;
            m_authentication = newAuth;
            break;
          }
          // Do next case

        default:
          return lastResponseCode;
      }
    }
    else {
      // If not persisting, we have no oppurtunity to write again, just error out
      if (!m_persist)
        break;

      // ... we close the channel and allow AssureConnet() to reopen it.
      Close();
    }
  }

  return lastResponseCode;
}


PBoolean PHTTPClient::WriteCommand(Commands cmd,
                               const PString & url,
                               PMIMEInfo & outMIME,
                               const PString & dataBody)
{
  return WriteCommand(commandNames[cmd], url, outMIME, dataBody);
}


PBoolean PHTTPClient::WriteCommand(const PString & cmdName,
                                   const PString & url,
                                       PMIMEInfo & outMIME,
                                   const PString & dataBody)
{
  ostream & this_stream = *this;
  PINDEX len = dataBody.GetLength();
  if (!outMIME.Contains(ContentLengthTag()))
    outMIME.SetInteger(ContentLengthTag(), len);

  if (m_authentication != NULL) {
    PHTTPClientAuthenticator auth(cmdName, url, outMIME, dataBody);
    m_authentication->Authorise(auth);
  }

  PString cmd(cmdName.IsEmpty() ? "GET" : cmdName);

#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(3, __FILE__, __LINE__);
    strm << "HTTP\tSending ";
    if (PTrace::CanTrace(4))
      strm << '\n';
    strm << cmdName << ' ';
    if (url.IsEmpty())
      strm << '/';
    else
      strm << url;
    if (PTrace::CanTrace(4)) {
      strm << '\n' << outMIME;
      if (!dataBody.IsEmpty()) {
        int amt = PTrace::CanTrace(5) ? 10000 : 100;
        strm << dataBody.Left(amt);
        if (len > amt)
          strm << "\n....";
      }
    }
    strm << PTrace::End;
  } 
#endif

  this_stream << cmd << ' ' << (url.IsEmpty() ? "/" :  (const char*) url) << " HTTP/1.1\r\n"
              << setfill('\r') << outMIME;

  return Write((const char *)dataBody, len);
}


PBoolean PHTTPClient::ReadResponse(PMIMEInfo & replyMIME)
{
  PString http = ReadString(7);
  if (!http) {
    UnRead(http);

    if (http.Find("HTTP/") == P_MAX_INDEX) {
      lastResponseCode = PHTTP::RequestOK;
      lastResponseInfo = "HTTP/0.9";
      PTRACE(3, "HTTP\tRead HTTP/0.9 OK");
      return PTrue;
    }

    if (http[0] == '\n')
      ReadString(1);
    else if (http[0] == '\r' &&  http[1] == '\n')
      ReadString(2);

    if (PHTTP::ReadResponse()) {
      bool readOK = replyMIME.Read(*this);

      PString body;
      if (lastResponseCode >= 300) {
        if (replyMIME.GetInteger(ContentLengthTag(), INT_MAX) > MaxTraceContentSize)
          InternalReadContentBody(replyMIME, NULL); // Waste body
        else
          ReadContentBody(replyMIME, body);
      }

#if PTRACING
      if (PTrace::CanTrace(3)) {
        ostream & strm = PTrace::Begin(3, __FILE__, __LINE__);
        strm << "HTTP\tResponse ";
        if (PTrace::CanTrace(4))
          strm << '\n';
        strm << lastResponseCode << ' ' << lastResponseInfo;
        if (PTrace::CanTrace(4)) {
          strm << '\n' << replyMIME;
          if (!body.IsEmpty())
            strm << body;
        }
        strm << PTrace::End;
      }
#endif

      if (!body.IsEmpty())
        lastResponseInfo += '\n' + body;

      if (readOK)
        return PTrue;
    }
  }
 
  lastResponseCode = -1;
  if (GetErrorCode(LastReadError) != NoError)
    lastResponseInfo = GetErrorText(LastReadError);
  else {
    lastResponseInfo = "Premature shutdown";
    SetErrorValues(ProtocolFailure, 0, LastReadError);
  }

  return PFalse;
}


PBoolean PHTTPClient::ReadContentBody(PMIMEInfo & replyMIME, PString & body)
{
  PCharArray rawBody;
  if (!InternalReadContentBody(replyMIME, &rawBody))
    return false;

  body = PString(rawBody, rawBody.GetSize());
  return true;
}


PBoolean PHTTPClient::ReadContentBody(PMIMEInfo & replyMIME, PBYTEArray & body)
{
  return InternalReadContentBody(replyMIME, &body);
}


PBoolean PHTTPClient::InternalReadContentBody(PMIMEInfo & replyMIME, PAbstractArray * body)
{
  PCaselessString encoding = replyMIME(TransferEncodingTag());

  if (encoding != ChunkedTag()) {
    if (replyMIME.Contains(ContentLengthTag())) {
      PINDEX length = replyMIME.GetInteger(ContentLengthTag());
      if (body != NULL) {
        body->SetSize(length);
        return ReadBlock(body->GetPointer(), length);
      }
      while (length-- > 0) {
        if (ReadChar() < 0)
          return false;
      }
      return true;
    }

    if (!(encoding.IsEmpty())) {
      lastResponseCode = -1;
      lastResponseInfo = "Unknown Transfer-Encoding extension";
      return PFalse;
    }

    if (body != NULL) {
      // Must be raw, read to end file variety
      static const PINDEX ChunkSize = 2048;
      PINDEX bytesRead = 0;
      while (ReadBlock((char *)body->GetPointer(bytesRead+ChunkSize)+bytesRead, ChunkSize))
        bytesRead += GetLastReadCount();

      body->SetSize(bytesRead + GetLastReadCount());
    }
    else {
      while (ReadChar() >= 0)
        ;
    }
    return GetErrorCode(LastReadError) == NoError;
  }

  // HTTP1.1 chunked format
  PINDEX bytesRead = 0;
  for (;;) {
    // Read chunk length line
    PString chunkLengthLine;
    if (!ReadLine(chunkLengthLine))
      return PFalse;

    // A zero length chunk is end of output
    PINDEX chunkLength = chunkLengthLine.AsUnsigned(16);
    if (chunkLength == 0)
      break;

    if (body != NULL) {
      // Read the chunk
      if (!ReadBlock((char *)body->GetPointer(bytesRead+chunkLength)+bytesRead, chunkLength))
        return PFalse;
      bytesRead+= chunkLength;
    }
    else {
      while (chunkLength-- > 0) {
        if (ReadChar() < 0)
          return false;
      }
    }

    // Read the trailing CRLF
    if (!ReadLine(chunkLengthLine))
      return PFalse;
  }

  // Read the footer
  PString footer;
  do {
    if (!ReadLine(footer))
      return PFalse;
  } while (replyMIME.AddMIME(footer));

  return PTrue;
}


PBoolean PHTTPClient::GetTextDocument(const PURL & url,
                                      PString & document,
                                      const PString & requiredContentType)
{
  PMIMEInfo outMIME, replyMIME;
  if (!GetDocument(url, outMIME, replyMIME))
    return PFalse;

  PCaselessString actualContentType = replyMIME(ContentTypeTag());
  if (!requiredContentType.IsEmpty() && !actualContentType.IsEmpty() &&
        actualContentType.NumCompare(requiredContentType, requiredContentType.Find(';')) != EqualTo) {
    PTRACE(2, "HTTP\tIncorrect Content-Type for document: expecting " << requiredContentType << ", got " << actualContentType);
    InternalReadContentBody(replyMIME, NULL); // Waste body
    return false;
  }

  if (!ReadContentBody(replyMIME, document)) {
    PTRACE(2, "HTTP\tRead of body failed");
    return false;
  }

  PTRACE_IF(4, !document.IsEmpty(), "HTTP\tReceived body:\n"
            << document.Left(MaxTraceContentSize) << (document.GetLength() > MaxTraceContentSize ? "\n...." : ""));
  return true;
}


PBoolean PHTTPClient::GetDocument(const PURL & url,
                              PMIMEInfo & outMIME,
                              PMIMEInfo & replyMIME)
{
  return IsOK(ExecuteCommand(GET, url, outMIME, PString::Empty(), replyMIME));
}


PBoolean PHTTPClient::GetHeader(const PURL & url,
                            PMIMEInfo & outMIME,
                            PMIMEInfo & replyMIME)
{
  return IsOK(ExecuteCommand(HEAD, url, outMIME, PString::Empty(), replyMIME));
}


PBoolean PHTTPClient::PostData(const PURL & url,
                           PMIMEInfo & outMIME,
                           const PString & data,
                           PMIMEInfo & replyMIME)
{
  PString dataBody = data;
  if (!outMIME.Contains(ContentTypeTag())) {
    outMIME.SetAt(ContentTypeTag(), "application/x-www-form-urlencoded");
    dataBody += "\r\n"; // Add CRLF for compatibility with some CGI servers.
  }

  return IsOK(ExecuteCommand(POST, url, outMIME, data, replyMIME));
}


PBoolean PHTTPClient::PostData(const PURL & url,
                           PMIMEInfo & outMIME,
                           const PString & data,
                           PMIMEInfo & replyMIME,
                           PString & body)
{
  return PostData(url, outMIME, data, replyMIME) && ReadContentBody(replyMIME, body);
}


bool PHTTPClient::PutTextDocument(const PURL & url,
                                  const PString & document,
                                  const PString & contentType)
{
  PMIMEInfo outMIME, replyMIME;
  outMIME.SetAt(ContentTypeTag(), contentType);
  return IsOK(ExecuteCommand(PUT, url, outMIME, document, replyMIME));
}


bool PHTTPClient::PutDocument(const PURL & url,
                              PMIMEInfo & outMIME,
                              PMIMEInfo & replyMIME)
{
  return IsOK(ExecuteCommand(PUT, url, outMIME, PString::Empty(), replyMIME));
}


bool PHTTPClient::DeleteDocument(const PURL & url)
{
  PMIMEInfo outMIME, replyMIME;
  return IsOK(ExecuteCommand(DELETE, url, outMIME, PString::Empty(), replyMIME));
}


PBoolean PHTTPClient::AssureConnect(const PURL & url, PMIMEInfo & outMIME)
{
  PString host = url.GetHostName();

  // Is not open or other end shut down, restablish connection
  if (!IsOpen()) {
    if (host.IsEmpty()) {
      lastResponseCode = BadRequest;
      lastResponseInfo = "No host specified";
      return SetErrorValues(ProtocolFailure, 0, LastReadError);
    }

#if P_SSL
    if (url.GetScheme() == "https") {
      PTCPSocket * tcp = new PTCPSocket(url.GetPort());
      tcp->SetReadTimeout(readTimeout);
      if (!tcp->Connect(host)) {
        lastResponseCode = -2;
        lastResponseInfo = tcp->GetErrorText();
        delete tcp;
        return PFalse;
      }

      PSSLChannel * ssl = new PSSLChannel;
      if (!ssl->Connect(tcp)) {
        lastResponseCode = -2;
        lastResponseInfo = ssl->GetErrorText();
        delete ssl;
        return PFalse;
      }

      if (!Open(ssl)) {
        lastResponseCode = -2;
        lastResponseInfo = GetErrorText();
        return PFalse;
      }
    }
    else
#endif

    if (!Connect(host, url.GetPort())) {
      lastResponseCode = -2;
      lastResponseInfo = GetErrorText();
      return PFalse;
    }
  }

  // Have connection, so fill in the required MIME fields
  static char HostTag[] = "Host";
  if (!outMIME.Contains(HostTag)) {
    if (!host)
      outMIME.SetAt(HostTag, host);
    else {
      PIPSocket * sock = GetSocket();
      if (sock != NULL)
        outMIME.SetAt(HostTag, sock->GetHostName());
    }
  }

  return PTrue;
}


void PHTTPClient::SetAuthenticationInfo(
  const PString & userName,
  const PString & password
)
{
  m_userName = userName;
  m_password = password;
}


////////////////////////////////////////////////////////////////////////////////////

PHTTPClientAuthentication::PHTTPClientAuthentication()
{
  isProxy = PFalse;
}


PObject::Comparison PHTTPClientAuthentication::Compare(const PObject & other) const
{
  const PHTTPClientAuthentication * otherAuth = dynamic_cast<const PHTTPClientAuthentication *>(&other);
  if (otherAuth == NULL)
    return LessThan;

  Comparison result = GetUsername().Compare(otherAuth->GetUsername());
  if (result != EqualTo)
    return result;

  return GetPassword().Compare(otherAuth->GetPassword());
}


PString PHTTPClientAuthentication::GetAuthParam(const PString & auth, const char * name) const
{
  PString value;

  PINDEX pos = auth.Find(name);
  if (pos != P_MAX_INDEX)  {
    pos += (int)strlen(name);
    while (isspace(auth[pos]) || (auth[pos] == ','))
      pos++;
    if (auth[pos] == '=') {
      pos++;
      while (isspace(auth[pos]))
        pos++;
      if (auth[pos] == '"') {
        pos++;
        value = auth(pos, auth.Find('"', pos)-1);
      }
      else {
        PINDEX base = pos;
        while (auth[pos] != '\0' && !isspace(auth[pos]) && (auth[pos] != ','))
          pos++;
        value = auth(base, pos-1);
      }
    }
  }

  return value;
}

PString PHTTPClientAuthentication::AsHex(PMessageDigest5::Code & digest) const
{
  PStringStream out;
  out << hex << setfill('0');
  for (PINDEX i = 0; i < 16; i++)
    out << setw(2) << (unsigned)((BYTE *)&digest)[i];
  return out;
}

PString PHTTPClientAuthentication::AsHex(const PBYTEArray & data) const
{
  PStringStream out;
  out << hex << setfill('0');
  for (PINDEX i = 0; i < data.GetSize(); i++)
    out << setw(2) << (unsigned)data[i];
  return out;
}

////////////////////////////////////////////////////////////////////////////////////

PHTTPClientBasicAuthentication::PHTTPClientBasicAuthentication()
{
}


PObject::Comparison PHTTPClientBasicAuthentication::Compare(const PObject & other) const
{
  const PHTTPClientBasicAuthentication * otherAuth = dynamic_cast<const PHTTPClientBasicAuthentication *>(&other);
  if (otherAuth == NULL)
    return LessThan;

  return PHTTPClientAuthentication::Compare(other);
}

PBoolean PHTTPClientBasicAuthentication::Parse(const PString & /*auth*/, PBoolean /*proxy*/)
{
  return true;
}

PBoolean PHTTPClientBasicAuthentication::Authorise(AuthObject & authObject) const
{
  PBase64 digestor;
  digestor.StartEncoding();
  digestor.ProcessEncoding(username + ":" + password);
  PString result = digestor.GetEncodedString();

  PStringStream auth;
  auth << "Basic " << result;

  authObject.GetMIME().SetAt(isProxy ? "Proxy-Authorization" : "Authorization", auth);
  return true;
}

////////////////////////////////////////////////////////////////////////////////////

PHTTPClientDigestAuthentication::PHTTPClientDigestAuthentication()
  : algorithm(NumAlgorithms)
  , qopAuth(false)
  , qopAuthInt(false)
  , stale(false)
{
}

PHTTPClientDigestAuthentication & PHTTPClientDigestAuthentication::operator =(const PHTTPClientDigestAuthentication & auth)
{
  isProxy   = auth.isProxy;
  authRealm = auth.authRealm;
  username  = auth.username;
  password  = auth.password;
  nonce     = auth.nonce;
  opaque    = auth.opaque;
          
  qopAuth    = auth.qopAuth;
  qopAuthInt = auth.qopAuthInt;
  stale      = auth.stale;
  cnonce     = auth.cnonce;
  nonceCount.SetValue(auth.nonceCount);

  return *this;
}

PObject::Comparison PHTTPClientDigestAuthentication::Compare(const PObject & other) const
{
  const PHTTPClientDigestAuthentication * otherAuth = dynamic_cast<const PHTTPClientDigestAuthentication *>(&other);
  if (otherAuth == NULL)
    return LessThan;

  if (stale || otherAuth->stale)
    return LessThan;

  if (algorithm < otherAuth->algorithm)
    return LessThan;
  if (algorithm > otherAuth->algorithm)
    return GreaterThan;

  Comparison result = authRealm.Compare(otherAuth->authRealm);
  if (result != EqualTo)
    return result;

  return PHTTPClientAuthentication::Compare(other);
}

PBoolean PHTTPClientDigestAuthentication::Parse(const PString & p_auth, PBoolean proxy)
{
  PCaselessString auth = p_auth;

  authRealm.MakeEmpty();
  nonce.MakeEmpty();
  opaque.MakeEmpty();
  algorithm = NumAlgorithms;

  qopAuth = qopAuthInt = PFalse;
  cnonce.MakeEmpty();
  nonceCount.SetValue(1);

  if (auth.Find("digest") == P_MAX_INDEX) {
    PTRACE(1, "HTTP\tDigest auth does not contian digest keyword");
    return false;
  }

  algorithm = Algorithm_MD5;  // default
  PCaselessString str = GetAuthParam(auth, "algorithm");
  if (!str.IsEmpty()) {
    while (str != AlgorithmNames[algorithm]) {
      algorithm = (Algorithm)(algorithm+1);
      if (algorithm >= PHTTPClientDigestAuthentication::NumAlgorithms) {
        PTRACE(1, "HTTP\tUnknown digest algorithm " << str);
        return PFalse;
      }
    }
  }

  authRealm = GetAuthParam(auth, "realm");
  if (authRealm.IsEmpty()) {
    PTRACE(1, "HTTP\tNo realm in authentication");
    return PFalse;
  }

  nonce = GetAuthParam(auth, "nonce");
  if (nonce.IsEmpty()) {
    PTRACE(1, "HTTP\tNo nonce in authentication");
    return PFalse;
  }

  opaque = GetAuthParam(auth, "opaque");
  if (!opaque.IsEmpty()) {
    PTRACE(2, "HTTP\tAuthentication contains opaque data");
  }

  PString qopStr = GetAuthParam(auth, "qop");
  if (!qopStr.IsEmpty()) {
    PTRACE(3, "HTTP\tAuthentication contains qop-options " << qopStr);
    PStringList options = qopStr.Tokenise(',', PTrue);
    qopAuth    = options.GetStringsIndex("auth") != P_MAX_INDEX;
    qopAuthInt = options.GetStringsIndex("auth-int") != P_MAX_INDEX;
    cnonce = PGloballyUniqueID().AsString();
  }

  PCaselessString staleStr = GetAuthParam(auth, "stale");
  PTRACE_IF(3, !staleStr.IsEmpty(), "HTTP\tAuthentication contains stale flag \"" << staleStr << '"');
  stale = staleStr.Find("true") != P_MAX_INDEX;

  isProxy = proxy;
  return PTrue;
}


PBoolean PHTTPClientDigestAuthentication::Authorise(AuthObject & authObject) const
{
  PTRACE(3, "HTTP\tAdding authentication information");

  PMessageDigest5 digestor;
  PMessageDigest5::Code a1, a2, entityBodyCode, response;

  PString uriText = authObject.GetURI();
  PINDEX pos = uriText.Find(";");
  if (pos != P_MAX_INDEX)
    uriText = uriText.Left(pos);

  digestor.Start();
  digestor.Process(username);
  digestor.Process(":");
  digestor.Process(authRealm);
  digestor.Process(":");
  digestor.Process(password);
  digestor.Complete(a1);

  if (qopAuthInt) {
    digestor.Start();
    digestor.Process(authObject.GetEntityBody());
    digestor.Complete(entityBodyCode);
  }

  digestor.Start();
  digestor.Process(authObject.GetMethod());
  digestor.Process(":");
  digestor.Process(uriText);
  if (qopAuthInt) {
    digestor.Process(":");
    digestor.Process(AsHex(entityBodyCode));
  }
  digestor.Complete(a2);

  PStringStream auth;
  auth << "Digest "
          "username=\"" << username << "\", "
          "realm=\"" << authRealm << "\", "
          "nonce=\"" << nonce << "\", "
          "uri=\"" << uriText << "\", "
          "algorithm=" << AlgorithmNames[algorithm];

  digestor.Start();
  digestor.Process(AsHex(a1));
  digestor.Process(":");
  digestor.Process(nonce);
  digestor.Process(":");

  if (qopAuthInt || qopAuth) {
    PString nc(psprintf("%08x", (unsigned int)nonceCount));
    ++nonceCount;
    PString qop;
    if (qopAuthInt)
      qop = "auth-int";
    else
      qop = "auth";
    digestor.Process(nc);
    digestor.Process(":");
    digestor.Process(cnonce);
    digestor.Process(":");
    digestor.Process(qop);
    digestor.Process(":");
    digestor.Process(AsHex(a2));
    digestor.Complete(response);
    auth << ", "
         << "response=\"" << AsHex(response) << "\", "
         << "cnonce=\"" << cnonce << "\", "
         << "nc=" << nc << ", "
         << "qop=" << qop;
  }
  else {
    digestor.Process(AsHex(a2));
    digestor.Complete(response);
    auth << ", response=\"" << AsHex(response) << "\"";
  }

  if (!opaque.IsEmpty())
    auth << ", opaque=\"" << opaque << "\"";

  authObject.GetMIME().SetAt(isProxy ? "Proxy-Authorization" : "Authorization", auth);
  return PTrue;
}

PHTTPClientAuthentication * PHTTPClientAuthentication::ParseAuthenticationRequired(bool isProxy, const PMIMEInfo & replyMIME, PString & errorMsg)
{
  PString line = replyMIME(isProxy ? "Proxy-Authenticate" : "WWW-Authenticate");

  // find authentication
  PINDEX pos = line.Find(' ');
  PString scheme = line.Left(pos).Trim().ToLower();
  PHTTPClientAuthentication * newAuth = PHTTPClientAuthenticationFactory::CreateInstance(scheme);
  if (newAuth == NULL) {
    delete newAuth;
    errorMsg = "Unknown authentication scheme " + scheme;
    return NULL;
  }

  // parse the new authentication scheme
  if (!newAuth->Parse(line, isProxy)) {
    delete newAuth;
    errorMsg = "Failed to parse authentication for scheme " + scheme;
    return NULL;
  }

  // switch authentication schemes
  return newAuth;
}


////////////////////////////////////////////////////////////////////////////////////

PHTTPClientAuthenticator::PHTTPClientAuthenticator(const PString & method, const PString & uri, PMIMEInfo & mime, const PString & body)
  : m_method(method)
  , m_uri(uri)
  , m_mime(mime)
  , m_body(body)
{
}

PMIMEInfo & PHTTPClientAuthenticator::GetMIME()
{
  return m_mime;
}

PString PHTTPClientAuthenticator::GetURI()
{
  return m_uri;
}

PString PHTTPClientAuthenticator::GetEntityBody()
{
  return m_body;
}

PString PHTTPClientAuthenticator::GetMethod()
{
  return m_method;
}

////////////////////////////////////////////////////////////////////////////////////

#undef new

class PURL_HttpLoader : public PURLLoader
{
    PCLASSINFO(PURL_HttpLoader, PURLLoader);
  public:
    virtual bool Load(const PURL & url, PString & str, const PString & requiredContentType)
    {
      PHTTPClient http;
      return http.GetTextDocument(url, str, requiredContentType);
    }

    virtual bool Load(const PURL & url, PBYTEArray & data, const PString & requiredContentType)
    {
      PHTTPClient http;
      PMIMEInfo outMIME, replyMIME;
      if (!http.GetDocument(url, outMIME, replyMIME))
        return false;

      PCaselessString actualContentType = replyMIME(PHTTP::ContentTypeTag());
      if (!requiredContentType.IsEmpty() && !actualContentType.IsEmpty() &&
            actualContentType.NumCompare(requiredContentType, requiredContentType.Find(';')) != EqualTo) {
        PTRACE(2, "HTTP\tIncorrect Content-Type for document: expecting " << requiredContentType << ", got " << actualContentType);
        return false;
      }

      return http.ReadContentBody(replyMIME, data);
    }
};

PFACTORY_CREATE(PURLLoaderFactory, PURL_HttpLoader, "http", true);
#if P_SSL
static PURLLoaderFactory::Worker<PURL_HttpLoader> httpsLoader("https", true);
#endif

#endif // P_HTTP


// End Of File ///////////////////////////////////////////////////////////////
