/*
 * pxml.cxx
 *
 * XML parser support
 *
 * Portable Windows Library
 *
 * Copyright (c) 2002 Equivalence Pty. Ltd.
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

// This depends on the expat XML library by Jim Clark
// See http://www.jclark.com/xml/expat.html for more information

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "pxml.h"
#endif

#include <ptclib/pxml.h>

#ifdef P_EXPAT

#define XML_STATIC 1

#include <expat.h>

#define new PNEW

#define CACHE_BUFFER_SIZE   1024
#define XMLSETTINGS_OPTIONS (NewLineAfterElement)


#ifdef P_EXPAT_LIBRARY
  #pragma comment(lib, P_EXPAT_LIBRARY)
  #pragma message("XML support (via Expat) enabled")
#endif


////////////////////////////////////////////////////

static void PXML_StartElement(void * userData, const char * name, const char ** attrs)
{
  ((PXMLParser *)userData)->StartElement(name, attrs);
}

static void PXML_EndElement(void * userData, const char * name)
{
  ((PXMLParser *)userData)->EndElement(name);
}

static void PXML_CharacterDataHandler(void * userData, const char * data, int len)
{
  ((PXMLParser *)userData)->AddCharacterData(data, len);
}

static void PXML_XmlDeclHandler(void * userData, const char * version, const char * encoding, int standalone)
{
  ((PXMLParser *)userData)->XmlDecl(version, encoding, standalone);
}

static void PXML_StartDocTypeDecl(void * userData,
                const char * docTypeName,
                const char * sysid,
                const char * pubid,
                    int hasInternalSubSet)
{
  ((PXMLParser *)userData)->StartDocTypeDecl(docTypeName, sysid, pubid, hasInternalSubSet);
}

static void PXML_EndDocTypeDecl(void * userData)
{
  ((PXMLParser *)userData)->EndDocTypeDecl();
}

static void PXML_StartNamespaceDeclHandler(void *userData,
                                 const XML_Char *prefix,
                                 const XML_Char *uri)
{
  ((PXMLParser *)userData)->StartNamespaceDeclHandler(prefix, uri);
}

static void PXML_EndNamespaceDeclHandler(void *userData, const XML_Char *prefix)
{
  ((PXMLParser *)userData)->EndNamespaceDeclHandler(prefix);
}

PXMLParser::PXMLParser(int options)
  : PXMLBase(options)
  , rootOpen(true)
{
  if ((options & WithNS) != 0)
    expat = XML_ParserCreateNS(NULL, '|');
  else
    expat = XML_ParserCreate(NULL);

  XML_SetUserData((XML_Parser)expat, this);

  XML_SetElementHandler      ((XML_Parser)expat, PXML_StartElement, PXML_EndElement);
  XML_SetCharacterDataHandler((XML_Parser)expat, PXML_CharacterDataHandler);
  XML_SetXmlDeclHandler      ((XML_Parser)expat, PXML_XmlDeclHandler);
  XML_SetDoctypeDeclHandler  ((XML_Parser)expat, PXML_StartDocTypeDecl, PXML_EndDocTypeDecl);
  XML_SetNamespaceDeclHandler((XML_Parser)expat, PXML_StartNamespaceDeclHandler, PXML_EndNamespaceDeclHandler);

  rootElement = NULL;
  currentElement = NULL;
  lastElement    = NULL;
}

PXMLParser::~PXMLParser()
{
  XML_ParserFree((XML_Parser)expat);
}

PXMLElement * PXMLParser::GetXMLTree() const
{ 
  return rootOpen ? NULL : rootElement; 
}

PXMLElement * PXMLParser::SetXMLTree(PXMLElement * newRoot)
{ 
  PXMLElement * oldRoot = rootElement;
  rootElement = newRoot;
  rootOpen = false;
  return oldRoot;
}

bool PXMLParser::Parse(const char * data, int dataLen, bool final)
{
  return XML_Parse((XML_Parser)expat, data, dataLen, final) != 0;  
}

void PXMLParser::GetErrorInfo(PString & errorString, unsigned & errorCol, unsigned & errorLine)
{
  XML_Error err = XML_GetErrorCode((XML_Parser)expat);
  errorString = PString(XML_ErrorString(err));
  errorCol    = XML_GetCurrentColumnNumber((XML_Parser)expat);
  errorLine   = XML_GetCurrentLineNumber((XML_Parser)expat);
}

void PXMLParser::StartElement(const char * name, const char **attrs)
{
  PXMLElement * newElement = new PXMLElement(currentElement, name);
  if (currentElement != NULL) {
    currentElement->AddSubObject(newElement, false);
    newElement->SetFilePosition(XML_GetCurrentColumnNumber((XML_Parser)expat) , XML_GetCurrentLineNumber((XML_Parser)expat));
  }

  while (attrs[0] != NULL) {
    newElement->SetAttribute(PString(attrs[0]), PString(attrs[1]));
    attrs += 2;
  }

  currentElement = newElement;
  lastElement    = NULL;

  if (rootElement == NULL) {
    rootElement = currentElement;
    rootOpen = true;
  }

  for (PINDEX i = 0; i < m_tempNamespaceList.GetSize(); ++i) 
    currentElement->AddNamespace(m_tempNamespaceList.GetKeyAt(i), m_tempNamespaceList.GetDataAt(i));

  m_tempNamespaceList.RemoveAll();
}

void PXMLParser::EndElement(const char * /*name*/)
{
  if (currentElement != rootElement)
    currentElement = currentElement->GetParent();
  else {
    currentElement = NULL;
    rootOpen = false;
  }
  lastElement    = NULL;
}

void PXMLParser::AddCharacterData(const char * data, int len)
{
  PString str(data, len);

  if (lastElement != NULL) {
    PAssert(!lastElement->IsElement(), "lastElement set by non-data element");
    lastElement->SetString(lastElement->GetString() + str, false);
  } else {
    PXMLData * newElement = new PXMLData(currentElement, str);
    if (currentElement != NULL)
      currentElement->AddSubObject(newElement, false);
    lastElement = newElement;
  } 
}


void PXMLParser::XmlDecl(const char * _version, const char * _encoding, int standAlone)
{
  version    = _version;
  encoding   = _encoding;
  m_standAlone = (StandAloneType)standAlone;
}

void PXMLParser::StartDocTypeDecl(const char * /*docTypeName*/,
                                  const char * /*sysid*/,
                                  const char * /*pubid*/,
                                  int /*hasInternalSubSet*/)
{
}

void PXMLParser::EndDocTypeDecl()
{
}

void PXMLParser::StartNamespaceDeclHandler(const XML_Char * prefix, 
                                           const XML_Char * uri)
{
  m_tempNamespaceList.SetAt(PString(prefix == NULL ? "" : prefix), uri);
}

void PXMLParser::EndNamespaceDeclHandler(const XML_Char * /*prefix*/)
{
}


///////////////////////////////////////////////////////////////////////////////////////////////



PXML::PXML(int options, const char * noIndentElements)
 : PXMLBase(options) 
{
  Construct(options, noIndentElements);
}

PXML::PXML(const PString & data, int options, const char * noIndentElements)
  : PXMLBase(options) 
{
  Construct(options, noIndentElements);
  Load(data);
}

PXML::~PXML()
{
  RemoveAll();
}

PXML::PXML(const PXML & xml)
  : noIndentElements(xml.noIndentElements)
{
  Construct(xml.m_options, NULL);

  loadFromFile       = xml.loadFromFile;
  loadFilename       = xml.loadFilename;
  version            = xml.version;
  encoding           = xml.encoding;
  m_standAlone       = xml.m_standAlone;
  m_defaultNameSpace = xml.m_defaultNameSpace;

  PWaitAndSignal m(xml.rootMutex);

  PXMLElement * oldRootElement = xml.rootElement;
  if (oldRootElement != NULL)
    rootElement = (PXMLElement *)oldRootElement->Clone(NULL);
}

void PXML::Construct(int options, const char * _noIndentElements)
{
  rootElement    = NULL;
  m_options      = options;
  loadFromFile   = false;
  m_standAlone   = UninitialisedStandAlone;
  m_errorLine    = 0;
  m_errorColumn  = 0;

  if (_noIndentElements != NULL)
    noIndentElements = PString(_noIndentElements).Tokenise(' ', false);
}

PXMLElement * PXML::SetRootElement(const PString & documentType)
{
  return SetRootElement(new PXMLElement(NULL, documentType));
}

PXMLElement * PXML::SetRootElement(PXMLElement * element)
{
  PWaitAndSignal m(rootMutex);

  if (rootElement != NULL)
    delete rootElement;

  rootElement = element;
  m_errorString.MakeEmpty();
  m_errorLine = m_errorColumn = 0;

  return rootElement;
}

bool PXML::IsDirty() const
{
  PWaitAndSignal m(rootMutex);

  if (rootElement == NULL)
    return false;

  return rootElement->IsDirty();
}


PCaselessString PXML::GetDocumentType() const
{ 
  PWaitAndSignal m(rootMutex);

  if (rootElement == NULL)
    return PCaselessString();
  return rootElement->GetName();
}


bool PXML::LoadFile(const PFilePath & fn, PXMLParser::Options options)
{
  PTRACE(4, "XML\tLoading file " << fn);

  PWaitAndSignal m(rootMutex);

  m_options = options;

  loadFilename = fn;
  loadFromFile = true;

  PFile file;
  if (!file.Open(fn, PFile::ReadOnly)) {
    m_errorString << "File open error " << file.GetErrorText();
    return false;
  }

  off_t len = file.GetLength();
  PString data;
  if (!file.Read(data.GetPointer(len + 1), len)) {
    m_errorString << "File read error " << file.GetErrorText();
    return false;
  }

  data[(PINDEX)len] = '\0';

  return Load(data);
}


bool PXML::Load(const PString & data, PXMLParser::Options options)
{
  m_options = options;
  m_errorString.MakeEmpty();
  m_errorLine = m_errorColumn = 0;

  bool stat = false;
  PXMLElement * loadingRootElement = NULL;

  {
    PXMLParser parser(options);
    int done = 1;
    stat = parser.Parse(data, data.GetLength(), done) != 0;
  
    if (!stat)
      parser.GetErrorInfo(m_errorString, m_errorColumn, m_errorLine);

    version      = parser.GetVersion();
    encoding     = parser.GetEncoding();
    m_standAlone = parser.GetStandAlone();

    loadingRootElement = parser.GetXMLTree();
  }

  if (!stat)
    return false;

  if (loadingRootElement == NULL) {
    m_errorString << "Failed to create root node in XML!";
    return false;
  }

  PWaitAndSignal m(rootMutex);
  if (rootElement != NULL) {
    delete rootElement;
    rootElement = NULL;
  }
  rootElement = loadingRootElement;
  PTRACE(4, "XML\tLoaded XML <" << rootElement->GetName() << '>');

  OnLoaded();

  return true;
}

bool PXML::Save(PXMLParser::Options options)
{
  m_options = options;

  if (!loadFromFile || !IsDirty())
    return false;

  return SaveFile(loadFilename);
}


bool PXML::SaveFile(const PFilePath & fn, PXMLParser::Options options)
{
  PWaitAndSignal m(rootMutex);

  PFile file;
  if (!file.Open(fn, PFile::WriteOnly)) 
    return false;

  PString data;
  if (!Save(data, options))
    return false;

  return file.Write((const char *)data, data.GetLength());
}


bool PXML::Save(PString & data, PXMLParser::Options options)
{
  PWaitAndSignal m(rootMutex);

  m_options = options;

  PStringStream strm;
  strm << *this;
  data = strm;
  return true;
}


void PXML::RemoveAll()
{
  PWaitAndSignal m(rootMutex);

  if (rootElement != NULL) {
    delete rootElement;
    rootElement = NULL;
  }
}


PXMLElement * PXML::GetElement(const PCaselessString & name, const PCaselessString & attr, const PString & attrval) const
{
  return rootElement != NULL ? rootElement->GetElement(name, attr, attrval) : NULL;
}


PXMLElement * PXML::GetElement(const PCaselessString & name, PINDEX idx) const
{
  return rootElement != NULL ? rootElement->GetElement(name, idx) : NULL;
}


PXMLElement * PXML::GetElement(PINDEX idx) const
{
  return rootElement != NULL ? (PXMLElement *)rootElement->GetElement(idx) : NULL;
}


bool PXML::RemoveElement(PINDEX idx)
{
  return rootElement != NULL && rootElement->RemoveElement(idx);
}


PINDEX PXML::GetNumElements() const
{
  if (rootElement == NULL) 
    return 0;
  else 
    return rootElement->GetSize();
}


PBoolean PXML::IsNoIndentElement(const PString & elementName) const
{
  return noIndentElements.GetValuesIndex(elementName) != P_MAX_INDEX;
}


PString PXML::AsString() const
{
  PStringStream strm;
  PrintOn(strm);
  return strm;
}


void PXML::PrintOn(ostream & strm) const
{
//<?xml version="1.0" encoding="UTF-8" standalone="yes"?>

  if ((m_options & PXMLParser::FragmentOnly) == 0) {
    strm << "<?xml version=\"";

    if (version.IsEmpty())
      strm << "1.0";
    else
      strm << version;

    strm << "\" encoding=\"";

    if (encoding.IsEmpty())
      strm << "UTF-8";
    else
      strm << encoding;

    strm << "\"";

    switch (m_standAlone) {
      case 0:
        strm << " standalone=\"no\"";
        break;
      case 1:
        strm << " standalone=\"yes\"";
        break;
      default:
        break;
    }

    strm << "?>";
    if ((m_options & PXMLParser::NewLineAfterElement) != 0)
      strm << '\n';
  }

  if (rootElement != NULL) {
    if (!docType.IsEmpty())
      strm << "<!DOCTYPE " << docType << '>' << endl;

    rootElement->Output(strm, *this, 2);
  }
}


void PXML::ReadFrom(istream & strm)
{
  rootMutex.Wait();
  delete rootElement;
  rootElement = NULL;
  rootMutex.Signal();

  PXMLParser parser(m_options);
  while (strm.good()) {
    PString line;
    strm >> line;

    if (!parser.Parse(line, line.GetLength(), false)) {
      parser.GetErrorInfo(m_errorString, m_errorColumn, m_errorLine);
      break;
    }

    if (parser.GetXMLTree() != NULL) {
      rootMutex.Wait();

      version            = parser.GetVersion();
      encoding           = parser.GetEncoding();
      m_standAlone       = parser.GetStandAlone();
      rootElement        = parser.GetXMLTree();

      rootMutex.Signal();

      PTRACE(4, "XML\tRead XML <" << rootElement->GetName() << '>');
      break;
    }
  }
}


PString PXML::CreateStartTag(const PString & text)
{
  return '<' + text + '>';
}


PString PXML::CreateEndTag(const PString & text)
{
  return "</" + text + '>';
}


PString PXML::CreateTagNoData(const PString & text)
{
  return '<' + text + "/>";
}


PString PXML::CreateTag(const PString & text, const PString & data)
{
  return CreateStartTag(text) + data + CreateEndTag(text);
}


bool PXML::Validate(const ValidationInfo * validator)
{
  if (PAssertNULL(validator) == NULL)
    return false;

  m_errorString.MakeEmpty();

  ValidationContext context;

  bool s;
  if (rootElement != NULL)
    s = ValidateElements(context, rootElement, validator);
  else {
    m_errorString << "No root element";
    s = false;
  }

  return s;
}


bool PXML::ValidateElements(ValidationContext & context, PXMLElement * baseElement, const ValidationInfo * validator)
{
  if (PAssertNULL(validator) == NULL)
    return false;

  while (validator->m_op != EndOfValidationList) {
    if (!ValidateElement(context, baseElement, validator))
      return false;
    ++validator;
  }
  return true;
}

bool PXML::ValidateElement(ValidationContext & context, PXMLElement * baseElement, const ValidationInfo * validator)
{
  if (PAssertNULL(validator) == NULL)
    return false;

  PCaselessString elementNameWithNs(validator->m_name);
  {
    PINDEX pos;
    if ((pos = elementNameWithNs.FindLast(':')) == P_MAX_INDEX) {
      if (!context.m_defaultNameSpace.IsEmpty())
        elementNameWithNs = context.m_defaultNameSpace + "|" + elementNameWithNs.Right(pos);
    }
    else {
      PString * uri = context.m_nameSpaces.GetAt(elementNameWithNs.Left(pos));
      if (uri != NULL)
        elementNameWithNs = *uri + "|" + elementNameWithNs.Right(pos);
    }
  }

  bool checkValue = false;
  bool extendedRegex = false;

  switch (validator->m_op) {

    case SetDefaultNamespace:
      context.m_defaultNameSpace = validator->m_name;
      break;

    case SetNamespace:
      context.m_nameSpaces.SetAt(validator->m_name, validator->m_namespace);;
      break;

    case ElementName:
      {
        if (elementNameWithNs != baseElement->GetName()) {
          m_errorString << "Expected element with name \"" << elementNameWithNs << '"';
          baseElement->GetFilePosition(m_errorColumn, m_errorLine);
          return false;
        }
      }
      break;

    case Subtree:
      {
        if (baseElement->GetElement(elementNameWithNs) == NULL) {
          if (validator->m_minCount == 0)
            break;

          m_errorString << "Must have at least " << validator->m_minCount << " instances of '" << elementNameWithNs << "'";
          baseElement->GetFilePosition(m_errorColumn, m_errorLine);
          return false;
        }

        // verify each matching element
        PINDEX index = 0;
        PXMLElement * subElement;
        while ((subElement = baseElement->GetElement(elementNameWithNs, index)) != NULL) {
          if (validator->m_maxCount > 0 && index > validator->m_maxCount) {
            m_errorString << "Must have at no more than " << validator->m_maxCount << " instances of '" << elementNameWithNs << "'";
            baseElement->GetFilePosition(m_errorColumn, m_errorLine);
            return false;
          }

          if (!ValidateElement(context, subElement, validator->m_subElement))
            return false;

          ++index;
        }
      }
      break;

    case RequiredElementWithBodyMatching:
      extendedRegex = true;
    case RequiredElementWithBodyMatchingEx:
      checkValue = true;
    case RequiredElement:
      if (baseElement->GetElement(elementNameWithNs) == NULL) {
        m_errorString << "Element \"" << baseElement->GetName() << "\" missing required subelement \"" << elementNameWithNs << '"';
        baseElement->GetFilePosition(m_errorColumn, m_errorLine);
        return false;
      }
      // fall through

    case OptionalElementWithBodyMatchingEx:
      extendedRegex = extendedRegex || validator->m_op == OptionalElementWithBodyMatchingEx;
      checkValue    = checkValue    || validator->m_op == OptionalElementWithBodyMatching;
    case OptionalElementWithBodyMatching:
      checkValue    = checkValue    || validator->m_op == OptionalElementWithBodyMatching;
    case OptionalElement:
      {
        if (baseElement->GetElement(validator->m_name) == NULL) 
          break;

        // verify each matching element
        PINDEX index = 0;
        PXMLElement * subElement;
        while ((subElement = baseElement->GetElement(elementNameWithNs, index)) != NULL) {
          if (validator->m_maxCount > 0 && index > validator->m_maxCount) {
            m_errorString << "Must have at no more than " << validator->m_maxCount << " instances of '" << elementNameWithNs << "'";
            baseElement->GetFilePosition(m_errorColumn, m_errorLine);
            return false;
          }
          if (validator->m_op == RequiredElementWithBodyMatching) {
            PString toMatch(subElement->GetData());
            PRegularExpression regex(PString(validator->m_attributeValues), extendedRegex ? PRegularExpression::Extended : 0);
            if (!toMatch.MatchesRegEx(regex)) {
              m_errorString << "Element \"" << subElement->GetName() << "\" has body with value \"" << toMatch.Trim() << "\" that does not match regex \"" << PString(validator->m_attributeValues) << '"';
              return false;
            }
          }
          ++index;
        }
      }
      break;

    case OptionalAttributeWithValueMatchingEx:
      extendedRegex = true;
    case OptionalAttributeWithValueMatching:
    case OptionalAttributeWithValue:
    case OptionalNonEmptyAttribute:
    case OptionalAttribute:
      if (!baseElement->HasAttribute(validator->m_name)) 
        break;
      // fall through
    case RequiredAttributeWithValueMatchingEx:
      extendedRegex = extendedRegex || validator->m_op == RequiredAttributeWithValueMatchingEx;
    case RequiredAttributeWithValueMatching:
    case RequiredAttributeWithValue:
    case RequiredNonEmptyAttribute:
    case RequiredAttribute:
      if (!baseElement->HasAttribute(validator->m_name)) {
        m_errorString << "Element \"" << baseElement->GetName() << "\" missing required attribute \"" << validator->m_name << '"';
        baseElement->GetFilePosition(m_errorColumn, m_errorLine);
        return false;
      }

      switch (validator->m_op) {
        case RequiredNonEmptyAttribute:
        case OptionalNonEmptyAttribute:
          if (baseElement->GetAttribute(validator->m_name).IsEmpty()) {
            m_errorString << "Element \"" << baseElement->GetName() << "\" has attribute \"" << validator->m_name << "\" which cannot be empty";
            baseElement->GetFilePosition(m_errorColumn, m_errorLine);
            return false;
          }
          break;

        case RequiredAttributeWithValue:
        case OptionalAttributeWithValue:
          {
            PString toMatch(baseElement->GetAttribute(validator->m_name));
            PStringArray values = PString(validator->m_attributeValues).Lines();
            PINDEX i = 0;
            for (i = 0; i < values.GetSize(); ++i) {
              if (toMatch *= values[i])
                break;
            }
            if (i == values.GetSize()) {
              m_errorString << "Element \"" << baseElement->GetName() << "\" has attribute \"" << validator->m_name << "\" which is not one of required values ";
              for (i = 0; i < values.GetSize(); ++i) {
                if (i != 0)
                  m_errorString << " | ";
                m_errorString << "'" << values[i] << "'";
              }
              baseElement->GetFilePosition(m_errorColumn, m_errorLine);
              return false;
            }
          }
          break;

        case RequiredAttributeWithValueMatching:
        case OptionalAttributeWithValueMatching:
        case RequiredAttributeWithValueMatchingEx:
        case OptionalAttributeWithValueMatchingEx:
          {
            PString toMatch(baseElement->GetAttribute(validator->m_name));
            PRegularExpression regex(PString(validator->m_attributeValues), extendedRegex ? PRegularExpression::Extended : 0);
            if (!toMatch.MatchesRegEx(regex)) {
              m_errorString << "Element \"" << baseElement->GetName() << "\" has attribute \"" << validator->m_name << "\" with value \"" << baseElement->GetAttribute(validator->m_name) << "\" that does not match regex \"" << PString(validator->m_attributeValues) << '"';
              return false;
            }
          }
          break;
        default:
          break;
      }
      break;

    default:
      break;
  }

  return true;
}


bool PXML::LoadAndValidate(const PString & body, const PXML::ValidationInfo * validator, PString & error, int options)
{
  PStringStream err;

  // load the XML
  if (!Load(body, (Options)options))
    err << "XML parse";
  else if (!Validate(validator))
    err << "XML validation";
  else
    return true;

  err << " error\n"
         "Error at line " << GetErrorLine() << ", column " << GetErrorColumn() << '\n'
      << GetErrorString() << '\n';
  error = err;
  return false;
}


///////////////////////////////////////////////////////

#if P_HTTP

PXML_HTTP::PXML_HTTP(int options, const char * noIndentElements)
  : PXML(options, noIndentElements)
{
}


bool PXML_HTTP::LoadURL(const PURL & url)
{
  return LoadURL(url, PMaxTimeInterval, PXMLParser::NoOptions);
}


bool PXML_HTTP::LoadURL(const PURL & url, const PTimeInterval & timeout, PXMLParser::Options options)
{
  if (url.IsEmpty()) {
    m_errorString = "Cannot load empty URL";
    m_errorLine = m_errorColumn = 0;
    return false;
  }

  PTRACE(4, "XML\tLoading URL " << url);

  PString data;
  if (url.GetScheme() == "file") 
    return LoadFile(url.AsFilePath());

  PHTTPClient client;
  PINDEX contentLength;
  PMIMEInfo outMIME, replyMIME;

  // make sure we do not hang around for ever
  client.SetReadTimeout(timeout);

  // get the resource header information
  if (!client.GetDocument(url, outMIME, replyMIME)) {
    m_errorString = "Cannot load URL ";
    m_errorLine = m_errorColumn = 0;
    m_errorString << '"' << url << '"';
    return false;
  }

  // get the length of the data
  if (replyMIME.Contains(PHTTPClient::ContentLengthTag()))
    contentLength = (PINDEX)replyMIME[PHTTPClient::ContentLengthTag()].AsUnsigned();
  else
    contentLength = P_MAX_INDEX;

  // download the resource into memory
  PINDEX offs = 0;
  for (;;) {
    PINDEX len;
    if (contentLength == P_MAX_INDEX)
      len = CACHE_BUFFER_SIZE;
    else if (offs == contentLength)
      break;
    else
      len = PMIN(contentLength = offs, CACHE_BUFFER_SIZE);

    if (!client.Read(offs + data.GetPointer(offs + len), len))
      break;

    len = client.GetLastReadCount();

    offs += len;
  }

  return Load(data, options);
}


bool PXML_HTTP::StartAutoReloadURL(const PURL & url, 
                                   const PTimeInterval & timeout, 
                                   const PTimeInterval & refreshTime,
                                   PXMLParser::Options options)
{
  if (url.IsEmpty()) {
    autoLoadError = "Cannot auto-load empty URL";
    return false;
  }

  PWaitAndSignal m(autoLoadMutex);
  autoLoadTimer.Stop();

  SetOptions(options);
  autoloadURL      = url;
  autoLoadWaitTime = timeout;
  autoLoadError.MakeEmpty();
  autoLoadTimer.SetNotifier(PCREATE_NOTIFIER(AutoReloadTimeout));

  bool stat = AutoLoadURL();

  autoLoadTimer = refreshTime;

  return stat;
}


void PXML_HTTP::AutoReloadTimeout(PTimer &, INT)
{
  PThread::Create(PCREATE_NOTIFIER(AutoReloadThread), "XmlReload");
}


void PXML_HTTP::AutoReloadThread(PThread &, INT)
{
  PWaitAndSignal m(autoLoadMutex);
  OnAutoLoad(AutoLoadURL());
  autoLoadTimer.Reset();
}


void PXML_HTTP::OnAutoLoad(bool PTRACE_PARAM(ok))
{
  PTRACE_IF(3, !ok, "XML\tFailed to load XML: " << GetErrorString());
}


bool PXML_HTTP::AutoLoadURL()
{
  bool stat = LoadURL(autoloadURL, autoLoadWaitTime);
  if (stat)
    autoLoadError.MakeEmpty();
  else 
    autoLoadError = GetErrorString() + psprintf(" at line %i, column %i", GetErrorLine(), GetErrorColumn());
  return stat;
}


bool PXML_HTTP::StopAutoReloadURL()
{
  PWaitAndSignal m(autoLoadMutex);
  autoLoadTimer.Stop();
  return true;
}

#endif // P_HTTP


///////////////////////////////////////////////////////
//
void PXMLObject::SetDirty()
{
  dirty = true;
  if (parent != NULL)
    parent->SetDirty();
}

PXMLObject * PXMLObject::GetNextObject() const
{
  if (parent == NULL)
    return NULL;

  // find our index in our parent's list
  PINDEX idx = parent->FindObject(this);
  if (idx == P_MAX_INDEX)
    return NULL;

  // get the next object
  ++idx;
  if (idx >= parent->GetSize())
    return NULL;

  return (*parent).GetElement(idx);
}


PString PXMLObject::AsString() const
{
  PStringStream strm;
  PrintOn(strm);
  return strm;
}


///////////////////////////////////////////////////////

PXMLData::PXMLData(PXMLElement * _parent, const PString & _value)
 : PXMLObject(_parent)
{
  value = _value;
}

PXMLData::PXMLData(PXMLElement * _parent, const char * data, int len)
 : PXMLObject(_parent)
{
  value = PString(data, len);
}

void PXMLData::Output(ostream & strm, const PXMLBase & xml, int indent) const
{
  int options = xml.GetOptions();
  if (xml.IsNoIndentElement(parent->GetName()))
    options &= ~PXMLParser::Indent;

  if (options & PXMLParser::Indent)
    strm << setw(indent-1) << " ";

  strm << value;

  if ((options & (PXMLParser::Indent|PXMLParser::NewLineAfterElement)) != 0)
    strm << endl;
}

void PXMLData::SetString(const PString & str, bool setDirty)
{
  value = str;
  if (setDirty)
    SetDirty();
}

PXMLObject * PXMLData::Clone(PXMLElement * _parent) const
{
  return new PXMLData(_parent, value);
}

///////////////////////////////////////////////////////

PXMLElement::PXMLElement(PXMLElement * _parent, const char * _name)
 : PXMLObject(_parent)
{
  lineNumber = column = 1;
  dirty = false;
  if (_name != NULL)
    name = _name;
}

PXMLElement::PXMLElement(PXMLElement * _parent, const PString & _name, const PString & data)
 : PXMLObject(_parent), name(_name)
{
  lineNumber = column = 1;
  dirty = false;
  AddSubObject(new PXMLData(this, data));
}

PINDEX PXMLElement::FindObject(const PXMLObject * ptr) const
{
  return subObjects.GetObjectsIndex(ptr);
}

bool PXMLElement::GetDefaultNamespace(PCaselessString & str) const
{
  if (!m_defaultNamespace.IsEmpty()) {
    str = m_defaultNamespace;
    return true;
  }

  if (parent != NULL)
    return parent->GetDefaultNamespace(str);

  return false;
}

bool PXMLElement::GetNamespace(const PCaselessString & prefix, PCaselessString & str) const
{
  if (m_nameSpaces.GetValuesIndex(prefix) != P_MAX_INDEX) {
    str = m_nameSpaces[prefix];
    return true;
  }

  if (parent != NULL)
    return parent->GetNamespace(prefix, str);

  return false;
}

bool PXMLElement::GetURIForNamespace(const PCaselessString & prefix, PCaselessString & uri)
{
  if (prefix.IsEmpty()) {
    if (!m_defaultNamespace.IsEmpty()) {
      uri = m_defaultNamespace + "|"; 
      return true;
    }
  }
  else {
    PINDEX i = m_nameSpaces.GetValuesIndex(prefix);
    if (i != P_MAX_INDEX) {
      uri = m_nameSpaces.GetKeyAt(i) + "|";
      return true;
    }
  }

  if (parent != NULL)
    return parent->GetNamespace(prefix, uri);

  uri = prefix + ":";

  return false;
}

PCaselessString PXMLElement::PrependNamespace(const PCaselessString & name_) const
{
  PCaselessString name(name_);
  PCaselessString newPrefix;
  PINDEX pos;
  if ((pos = name.FindLast(':')) == P_MAX_INDEX) {
    if (GetDefaultNamespace(newPrefix))
      name = newPrefix + "|" + name.Right(pos);
  }
  else if (GetNamespace(name.Left(pos), newPrefix))
    name = newPrefix + "|" + name.Right(pos);

  return name;
}

PXMLElement * PXMLElement::GetElement(const PCaselessString & name_, const PCaselessString & attr, const PString & attrval) const
{
  PCaselessString name(PrependNamespace(name_));
  for (PINDEX i = 0; i < subObjects.GetSize(); i++) {
    if (subObjects[i].IsElement()) {
      PXMLElement & subElement = ((PXMLElement &)subObjects[i]);
      if (name == subElement.GetName() && attrval == subElement.GetAttribute(attr))
        return &subElement;
    }
  }
  return NULL;
}


PXMLElement * PXMLElement::GetElement(const PCaselessString & name_, PINDEX index) const
{
  PCaselessString name(PrependNamespace(name_));
  for (PINDEX i = 0; i < subObjects.GetSize(); i++) {
    if (subObjects[i].IsElement()) {
      PXMLElement & subElement = ((PXMLElement &)subObjects[i]);
      if (name == subElement.GetName()) {
        if (index == 0)
          return &subElement;
        --index;
      }
    }
  }
  return NULL;
}


PXMLObject * PXMLElement::GetElement(PINDEX idx) const
{
  return idx < subObjects.GetSize() ? &subObjects[idx] : NULL;
}


bool PXMLElement::RemoveElement(PINDEX idx)
{
  if (idx >= subObjects.GetSize())
    return false;

  subObjects.RemoveAt(idx);
  return true;
}


PString PXMLElement::GetAttribute(const PCaselessString & key) const
{
  return attributes(key);
}

PString PXMLElement::GetKeyAttribute(PINDEX idx) const
{
  if (idx < attributes.GetSize())
    return attributes.GetKeyAt(idx);
  else
    return PString();
}

PString PXMLElement::GetDataAttribute(PINDEX idx) const
{
  if (idx < attributes.GetSize())
    return attributes.GetDataAt(idx);
  else
    return PString();
}

void PXMLElement::SetAttribute(const PCaselessString & key,
                               const PString & value,
                               bool setDirty)
{
  attributes.SetAt(key, value);
  if (setDirty)
    SetDirty();
}

bool PXMLElement::HasAttribute(const PCaselessString & key) const
{
  return attributes.Contains(key);
}

void PXMLElement::PrintOn(ostream & strm) const
{
  PXMLBase xml;
  Output(strm, xml, 0);
}

void PXMLElement::Output(ostream & strm, const PXMLBase & xml, int indent) const
{
  int options = xml.GetOptions();

  bool newLine = (options & (PXMLParser::Indent|PXMLParser::NewLineAfterElement)) != 0;

  if ((options & PXMLParser::Indent) != 0)
    strm << setw(indent-1) << " ";

  strm << '<' << name;

  PINDEX i;
  if (attributes.GetSize() > 0) {
    for (i = 0; i < attributes.GetSize(); i++) {
      PCaselessString key = attributes.GetKeyAt(i);
      strm << ' ' << key << "=\"" << attributes[key] << '"';
    }
  }

  // this ensures empty elements use the shortened form
  if (subObjects.GetSize() == 0) {
    strm << "/>";
    if (newLine)
      strm << endl;
  }
  else {
    bool indenting = (options & PXMLParser::Indent) != 0 && !xml.IsNoIndentElement(name);

    strm << '>';
    if (indenting)
      strm << endl;
  
    for (i = 0; i < subObjects.GetSize(); i++) 
      subObjects[i].Output(strm, xml, indent + 2);

    if (indenting)
      strm << setw(indent-1) << " ";

    strm << "</" << name << '>';
    if (newLine)
      strm << endl;
  }
}

PXMLObject * PXMLElement::AddSubObject(PXMLObject * elem, bool setDirty)
{
  subObjects.SetAt(subObjects.GetSize(), elem);
  if (setDirty)
    SetDirty();

  return elem;
}

PXMLElement * PXMLElement::AddChild(PXMLElement * elem, bool dirty)
{
  return (PXMLElement *)AddSubObject(elem, dirty);
}

PXMLData * PXMLElement::AddChild(PXMLData * elem, bool dirty)
{
  return (PXMLData *)AddSubObject(elem, dirty);
}

PXMLElement * PXMLElement::AddElement(const char * name)
{
  return (PXMLElement *)AddSubObject(new PXMLElement(this, name));
}

PXMLElement * PXMLElement::AddElement(const PString & name, const PString & data)
{
  return (PXMLElement *)AddSubObject(new PXMLElement(this, name, data));
}

PXMLElement * PXMLElement::AddElement(const PString & name, const PString & attrName, const PString & attrVal)
{
  PXMLElement * element = (PXMLElement *)AddSubObject(new PXMLElement(this, name));
  element->SetAttribute(attrName, attrVal);
  return element;
}

PXMLObject * PXMLElement::Clone(PXMLElement * _parent) const
{
  PXMLElement * elem = new PXMLElement(_parent);

  elem->SetName(name);
  elem->attributes = attributes;
  elem->dirty      = dirty;

  PINDEX idx;
  for (idx = 0; idx < subObjects.GetSize(); idx++)
    elem->AddSubObject(subObjects[idx].Clone(elem), false);

  return elem;
}

PString PXMLElement::GetData() const
{
  PString str;
  PINDEX idx;
  for (idx = 0; idx < subObjects.GetSize(); idx++) {
    if (!subObjects[idx].IsElement()) {
      PXMLData & dataElement = ((PXMLData &)subObjects[idx]);
      PStringArray lines = dataElement.GetString().Lines();
      PINDEX j;
      for (j = 0; j < lines.GetSize(); j++)
        str = str & lines[j];
    }
  }
  return str;
}

void PXMLElement::SetData(const PString & data)
{
  for (PINDEX idx = 0; idx < subObjects.GetSize(); idx++) {
    if (!subObjects[idx].IsElement())
      subObjects.RemoveAt(idx--);
  }
  AddData(data);
}

void PXMLElement::AddData(const PString & data)
{
  AddSubObject(new PXMLData(this, data));
}

PCaselessString PXMLElement::GetPathName() const
{
    PCaselessString s;

    s = GetName();
    const PXMLElement* el = this;
    while ((el = el->GetParent()) != NULL)
        s = el->GetName() + ":" + s;
    return s;
}

void PXMLElement::AddNamespace(const PString & prefix, const PString & uri)
{
  if (prefix.IsEmpty())
    m_defaultNamespace = uri;
  else
    m_nameSpaces.SetAt(prefix, uri);
}

void PXMLElement::RemoveNamespace(const PString & prefix)
{
  if (prefix.IsEmpty())
    m_defaultNamespace.MakeEmpty();
  else
    m_nameSpaces.RemoveAt(prefix);
}

///////////////////////////////////////////////////////

PXMLSettings::PXMLSettings(PXMLParser::Options options)
  : PXML(options)
{
}

PXMLSettings::PXMLSettings(const PString & data, PXMLParser::Options options)
  : PXML(data, options) 
{
}


#if P_CONFIG_FILE
PXMLSettings::PXMLSettings(const PConfig & data, PXMLParser::Options options)
  : PXML(options) 
{
  PStringList sects = data.GetSections();

  for (PStringList::iterator i = sects.begin(); i != sects.end(); ++i) {
    PStringToString keyvals = data.GetAllKeyValues(*i);
    for (PINDEX j = 0; j < keyvals.GetSize(); ++j)
      SetAttribute(*i, keyvals.GetKeyAt(j),keyvals.GetDataAt(j));
  }
}
#endif // P_CONFIG_FILE


bool PXMLSettings::Load(const PString & data)
{
  return PXML::Load(data);
}

bool PXMLSettings::LoadFile(const PFilePath & fn)
{
  return PXML::LoadFile(fn);
}

bool PXMLSettings::Save()
{
  return PXML::Save();
}

bool PXMLSettings::Save(PString & data)
{
  return PXML::Save(data);
}

bool PXMLSettings::SaveFile(const PFilePath & fn)
{
  return PXML::SaveFile(fn);
}

PString PXMLSettings::GetAttribute(const PCaselessString & section, const PString & key) const
{
  if (rootElement == NULL)
    return PString();

  PXMLElement * element = rootElement->GetElement(section);
  if (element == NULL)
    return PString();

  return element->GetAttribute(key);
}

void PXMLSettings::SetAttribute(const PCaselessString & section, const PString & key, const PString & value)
{
  if (rootElement == NULL) 
    rootElement = new PXMLElement(NULL, "settings");

  PXMLElement * element = rootElement->GetElement(section);
  if (element == NULL) {
    element = new PXMLElement(rootElement, section);
    rootElement->AddSubObject(element);
  }
  element->SetAttribute(key, value);
}

bool PXMLSettings::HasAttribute(const PCaselessString & section, const PString & key) const
{
  if (rootElement == NULL)
    return false;

  PXMLElement * element = rootElement->GetElement(section);
  if (element == NULL)
    return false;

  return element->HasAttribute(key);
}


#if P_CONFIG_FILE
void PXMLSettings::ToConfig(PConfig & cfg) const
{
  for (PINDEX i = 0;i < (PINDEX)GetNumElements();++i) {
    PXMLElement * el = GetElement(i);
    PString sectionName = el->GetName();
    for (PINDEX j = 0; j < (PINDEX)el->GetNumAttributes(); ++j) {
      PString key = el->GetKeyAttribute(j);
      PString dat = el->GetDataAttribute(j);
      if (!key && !dat)
        cfg.SetString(sectionName, key, dat);
    }
  }
}
#endif // P_CONFIG_FILE


///////////////////////////////////////////////////////

PXMLStreamParser::PXMLStreamParser()
{
}


void PXMLStreamParser::EndElement(const char * name)
{
  PXMLElement * element = currentElement;

  PXMLParser::EndElement(name);

  if (rootOpen) {
    PINDEX i = rootElement->FindObject(element);

    if (i != P_MAX_INDEX) {
      PXML tmp;
      element = (PXMLElement *)element->Clone(0);
      rootElement->RemoveElement(i);

      PXML * msg = new PXML;
      msg->SetRootElement(element);
      messages.Enqueue(msg);
    }
  }
}


PXML * PXMLStreamParser::Read(PChannel * channel)
{
  char buf[256];

  channel->SetReadTimeout(1000);

  while (rootOpen) {
    if (messages.GetSize() != 0)
      return messages.Dequeue();

    if (!channel->Read(buf, sizeof(buf) - 1) || !channel->IsOpen())
      return 0;

    buf[channel->GetLastReadCount()] = 0;

    if (!Parse(buf, channel->GetLastReadCount(), false))
      return 0;
  }

  channel->Close();
  return 0;
}

///////////////////////////////////////////////////////

#else

  #ifdef P_EXPAT_LIBRARY
    #pragma message("XML support (via Expat) DISABLED")
  #endif

#endif 


#ifdef P_EXPAT
PString PXML::EscapeSpecialChars(const PString & str)
#else
namespace PXML {
PString EscapeSpecialChars(const PString & str)
#endif
{
  // code based on appendix from http://www.hdfgroup.org/HDF5/XML/xml_escape_chars.htm
  static const char * quote = "&quot;";
  static const char * apos  = "&apos;";
  static const char * amp   = "&amp;";
  static const char * lt    = "&lt;";
  static const char * gt    = "&gt;";

  if (str.IsEmpty())
    return str;

  // calculate the extra length needed for the returned strng
  int len = str.GetLength();
  const char * cp = (const char *)str;
  int extra = 0;
  int i;
  for (i = 0; i < len; i++) {
    if (*cp == '\"')
      extra += (strlen(quote) - 1);
    else if (*cp == '\'')
      extra += (strlen(apos) - 1);
    else if (*cp == '<')
      extra += (strlen(lt) - 1);
    else if (*cp == '>')
      extra += (strlen(gt) - 1);
    else if (*cp == '&')
      extra += (strlen(amp) - 1);
    cp++;
  }

  if (extra == 0)
    return str;

  PString rstring;
  char * ncp = rstring.GetPointer(len+extra+1);

  cp = (const char *)str;
  for (i = 0; i < len; i++) {
    if (*cp == '\'') {
      strncpy(ncp,apos,strlen(apos));
      ncp += strlen(apos);
      cp++;
    } else if (*cp == '<') {
      strncpy(ncp,lt,strlen(lt));
      ncp += strlen(lt);
      cp++;
    } else if (*cp == '>') {
      strncpy(ncp,gt,strlen(gt));
      ncp += strlen(gt);
      cp++;
    } else if (*cp == '\"') {
      strncpy(ncp,quote,strlen(quote));
      ncp += strlen(quote);
      cp++;
    } else if (*cp == '&') {
      strncpy(ncp,amp,strlen(amp));
      ncp += strlen(amp);
      cp++;
    } else {
      *ncp++ = *cp++;
    }
  }
  *ncp = '\0';

  return rstring;
}

#ifndef P_EXPAT
}; // namespace PXML {
#endif
