/*
 * pxml.h
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

#ifndef PTLIB_PXML_H
#define PTLIB_PXML_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>

#include <ptbuildopts.h>

#ifndef P_EXPAT

namespace PXML {
extern PString EscapeSpecialChars(const PString & str);
};

#else

#include <ptclib/http.h>

////////////////////////////////////////////////////////////

class PXMLElement;
class PXMLData;


class PXMLObject;
class PXMLElement;
class PXMLData;

////////////////////////////////////////////////////////////

class PXMLBase : public PObject
{
  public:
    enum Options {
      NoOptions           = 0x0000,
      Indent              = 0x0001,
      NewLineAfterElement = 0x0002,
      NoIgnoreWhiteSpace  = 0x0004,   ///< ignored
      CloseExtended       = 0x0008,   ///< ignored
      WithNS              = 0x0010,
      FragmentOnly        = 0x0020,   ///< XML fragment, not complete document.
      AllOptions          = 0xffff
    };
    __inline friend Options operator|(Options o1, Options o2) { return (Options)(((unsigned)o1) | ((unsigned)o2)); }
    __inline friend Options operator&(Options o1, Options o2) { return (Options)(((unsigned)o1) & ((unsigned)o2)); }

    enum StandAloneType {
      UninitialisedStandAlone = -2,
      UnknownStandAlone = -1,
      NotStandAlone,
      IsStandAlone
    };

    PXMLBase(int opts = NoOptions)
      : m_options(opts) { }

    void SetOptions(int opts)
      { m_options = opts; }

    int GetOptions() const { return m_options; }

    virtual PBoolean IsNoIndentElement(
      const PString & /*elementName*/
    ) const
    {
      return false;
    }

  protected:
    int m_options;
};


class PXML : public PXMLBase
{
    PCLASSINFO(PXML, PObject);
  public:
    PXML(
      int options = NoOptions,
      const char * noIndentElements = NULL
    );
    PXML(
      const PString & data,
      int options = NoOptions,
      const char * noIndentElements = NULL
    );

    PXML(const PXML & xml);

    ~PXML();

    bool IsLoaded() const { return rootElement != NULL; }
    bool IsDirty() const;

    bool Load(const PString & data, Options options = NoOptions);
    bool LoadFile(const PFilePath & fn, Options options = NoOptions);

    virtual void OnLoaded() { }

    bool Save(Options options = NoOptions);
    bool Save(PString & data, Options options = NoOptions);
    bool SaveFile(const PFilePath & fn, Options options = NoOptions);

    void RemoveAll();

    PBoolean IsNoIndentElement(
      const PString & elementName
    ) const;

    PString AsString() const;
    void PrintOn(ostream & strm) const;
    void ReadFrom(istream & strm);


    PXMLElement * GetElement(const PCaselessString & name, const PCaselessString & attr, const PString & attrval) const;
    PXMLElement * GetElement(const PCaselessString & name, PINDEX idx = 0) const;
    PXMLElement * GetElement(PINDEX idx) const;
    PINDEX        GetNumElements() const; 
    PXMLElement * GetRootElement() const { return rootElement; }
    PXMLElement * SetRootElement(PXMLElement * p);
    PXMLElement * SetRootElement(const PString & documentType);
    bool          RemoveElement(PINDEX idx);

    PCaselessString GetDocumentType() const;


    enum ValidationOp {
      EndOfValidationList,
      DocType,
      ElementName,
      RequiredAttribute,
      RequiredNonEmptyAttribute,
      RequiredAttributeWithValue,
      RequiredElement,
      Subtree,
      RequiredAttributeWithValueMatching,
      RequiredElementWithBodyMatching,
      OptionalElement,
      OptionalAttribute,
      OptionalNonEmptyAttribute,
      OptionalAttributeWithValue,
      OptionalAttributeWithValueMatching,
      OptionalElementWithBodyMatching,
      SetDefaultNamespace,
      SetNamespace,

      RequiredAttributeWithValueMatchingEx = RequiredAttributeWithValueMatching + 0x8000,
      OptionalAttributeWithValueMatchingEx = OptionalAttributeWithValueMatching + 0x8000,
      RequiredElementWithBodyMatchingEx    = RequiredElementWithBodyMatching    + 0x8000,
      OptionalElementWithBodyMatchingEx    = OptionalElementWithBodyMatching    + 0x8000
    };

    struct ValidationContext {
      PString m_defaultNameSpace;
      PStringToString m_nameSpaces;
    };

    struct ValidationInfo {
      ValidationOp m_op;
      const char * m_name;

      union {
        const void     * m_placeHolder;
        const char     * m_attributeValues;
        ValidationInfo * m_subElement;
        const char     * m_namespace;
      };

      PINDEX m_minCount;
      PINDEX m_maxCount;
    };

    bool Validate(const ValidationInfo * validator);
    bool ValidateElements(ValidationContext & context, PXMLElement * baseElement, const ValidationInfo * elements);
    bool ValidateElement(ValidationContext & context, PXMLElement * element, const ValidationInfo * elements);
    bool LoadAndValidate(const PString & body, const PXML::ValidationInfo * validator, PString & error, int options = NoOptions);

    PString  GetErrorString() const { return m_errorString; }
    unsigned GetErrorColumn() const { return m_errorColumn; }
    unsigned GetErrorLine() const   { return m_errorLine; }

    PString GetDocType() const         { return docType; }
    void SetDocType(const PString & v) { docType = v; }

    PMutex & GetMutex() { return rootMutex; }

    // static methods to create XML tags
    static PString CreateStartTag (const PString & text);
    static PString CreateEndTag (const PString & text);
    static PString CreateTagNoData (const PString & text);
    static PString CreateTag (const PString & text, const PString & data);

    static PString EscapeSpecialChars(const PString & string);

  protected:
    void Construct(int options, const char * noIndentElements);
    PXMLElement * rootElement;
    PMutex rootMutex;

    bool loadFromFile;
    PFilePath loadFilename;
    PString version, encoding;
    StandAloneType m_standAlone;

    PStringStream m_errorString;
    unsigned      m_errorLine;
    unsigned      m_errorColumn;

    PSortedStringList noIndentElements;

    PString docType;
    PString m_defaultNameSpace;
};


#if P_HTTP
class PXML_HTTP : public PXML
{
    PCLASSINFO(PXML_HTTP, PXML);
  public:
    PXML_HTTP(
      int options = NoOptions,
      const char * noIndentElements = NULL
    );

    bool StartAutoReloadURL(
      const PURL & url, 
      const PTimeInterval & timeout, 
      const PTimeInterval & refreshTime,
      Options options = NoOptions
    );
    bool StopAutoReloadURL();
    PString GetAutoReloadStatus() { PWaitAndSignal m(autoLoadMutex); PString str = autoLoadError; return str; }
    bool AutoLoadURL();
    virtual void OnAutoLoad(PBoolean ok);

    bool LoadURL(const PURL & url);
    bool LoadURL(const PURL & url, const PTimeInterval & timeout, Options options = NoOptions);

  protected:
    PDECLARE_NOTIFIER(PTimer,  PXML_HTTP, AutoReloadTimeout);
    PDECLARE_NOTIFIER(PThread, PXML_HTTP, AutoReloadThread);

    PTimer autoLoadTimer;
    PURL autoloadURL;
    PTimeInterval autoLoadWaitTime;
    PMutex autoLoadMutex;
    PString autoLoadError;
};
#endif // P_HTTP

////////////////////////////////////////////////////////////

PARRAY(PXMLObjectArray, PXMLObject);

class PXMLObject : public PObject {
  PCLASSINFO(PXMLObject, PObject);
  public:
    PXMLObject(PXMLElement * par)
      : parent(par) { dirty = false; }

    PXMLElement * GetParent() const
      { return parent; }

    PXMLObject * GetNextObject() const;

    void SetParent(PXMLElement * newParent)
    { 
      PAssert(parent == NULL, "Cannot reparent PXMLElement");
      parent = newParent;
    }

    PString AsString() const;

    virtual void Output(ostream & strm, const PXMLBase & xml, int indent) const = 0;

    virtual PBoolean IsElement() const = 0;

    void SetDirty();
    bool IsDirty() const { return dirty; }

    virtual PXMLObject * Clone(PXMLElement * parent) const = 0;

  protected:
    PXMLElement * parent;
    bool dirty;
};

////////////////////////////////////////////////////////////

class PXMLData : public PXMLObject {
  PCLASSINFO(PXMLData, PXMLObject);
  public:
    PXMLData(PXMLElement * parent, const PString & data);
    PXMLData(PXMLElement * parent, const char * data, int len);

    PBoolean IsElement() const    { return false; }

    void SetString(const PString & str, bool dirty = true);

    PString GetString() const           { return value; }

    void Output(ostream & strm, const PXMLBase & xml, int indent) const;

    PXMLObject * Clone(PXMLElement * parent) const;

  protected:
    PString value;
};

////////////////////////////////////////////////////////////

class PXMLElement : public PXMLObject {
  PCLASSINFO(PXMLElement, PXMLObject);
  public:
    PXMLElement(PXMLElement * parent, const char * name = NULL);
    PXMLElement(PXMLElement * parent, const PString & name, const PString & data);

    PBoolean IsElement() const { return true; }

    void PrintOn(ostream & strm) const;
    void Output(ostream & strm, const PXMLBase & xml, int indent) const;

    PCaselessString GetName() const
      { return name; }

    /**
        Get the completely qualified name for the element inside the
        XML tree, for example "root:trunk:branch:subbranch:leaf".
     */
    PCaselessString GetPathName() const;

    void SetName(const PString & v)
    { name = v; }

    PINDEX GetSize() const
      { return subObjects.GetSize(); }

    PXMLObject  * AddSubObject(PXMLObject * elem, bool dirty = true);

    PXMLElement * AddChild    (PXMLElement * elem, bool dirty = true);
    PXMLData    * AddChild    (PXMLData    * elem, bool dirty = true);

    PXMLElement * AddElement(const char * name);
    PXMLElement * AddElement(const PString & name, const PString & data);
    PXMLElement * AddElement(const PString & name, const PString & attrName, const PString & attrVal);

    void SetAttribute(const PCaselessString & key,
                      const PString & value,
                      bool setDirty = true);

    PString GetAttribute(const PCaselessString & key) const;
    PString GetKeyAttribute(PINDEX idx) const;
    PString GetDataAttribute(PINDEX idx) const;
    bool HasAttribute(const PCaselessString & key) const;
    bool HasAttributes() const      { return attributes.GetSize() > 0; }
    PINDEX GetNumAttributes() const { return attributes.GetSize(); }

    PXMLElement * GetElement(const PCaselessString & name, const PCaselessString & attr, const PString & attrval) const;
    PXMLElement * GetElement(const PCaselessString & name, PINDEX idx = 0) const;
    PXMLObject  * GetElement(PINDEX idx = 0) const;
    bool          RemoveElement(PINDEX idx);

    PINDEX FindObject(const PXMLObject * ptr) const;

    bool HasSubObjects() const
      { return subObjects.GetSize() != 0; }

    PXMLObjectArray  GetSubObjects() const
      { return subObjects; }

    PString GetData() const;
    void SetData(const PString & data);
    void AddData(const PString & data);

    PXMLObject * Clone(PXMLElement * parent) const;

    void GetFilePosition(unsigned & col, unsigned & line) const { col = column; line = lineNumber; }
    void SetFilePosition(unsigned col,   unsigned line)         { column = col; lineNumber = line; }

    void AddNamespace(const PString & prefix, const PString & uri);
    void RemoveNamespace(const PString & prefix);

    bool GetDefaultNamespace(PCaselessString & str) const;
    bool GetNamespace(const PCaselessString & prefix, PCaselessString & str) const;
    PCaselessString PrependNamespace(const PCaselessString & name) const;
    bool GetURIForNamespace(const PCaselessString & prefix, PCaselessString & uri);

  protected:
    PCaselessString name;
    PStringToString attributes;
    PXMLObjectArray subObjects;
    bool dirty;
    unsigned column;
    unsigned lineNumber;
    PStringToString m_nameSpaces;
    PCaselessString m_defaultNamespace;
};

////////////////////////////////////////////////////////////

class PConfig;      // stupid gcc 4 does not recognize PConfig as a class

class PXMLSettings : public PXML
{
  PCLASSINFO(PXMLSettings, PXML);
  public:
    PXMLSettings(Options options = NewLineAfterElement);
    PXMLSettings(const PString & data, Options options = NewLineAfterElement);
    PXMLSettings(const PConfig & data, Options options = NewLineAfterElement);

    bool Load(const PString & data);
    bool LoadFile(const PFilePath & fn);

    bool Save();
    bool Save(PString & data);
    bool SaveFile(const PFilePath & fn);

    void SetAttribute(const PCaselessString & section, const PString & key, const PString & value);

    PString GetAttribute(const PCaselessString & section, const PString & key) const;
    bool    HasAttribute(const PCaselessString & section, const PString & key) const;

    void ToConfig(PConfig & cfg) const;
};


////////////////////////////////////////////////////////////

class PXMLParser : public PXMLBase
{
  PCLASSINFO(PXMLParser, PXMLBase);
  public:
    PXMLParser(int options = NoOptions);
    ~PXMLParser();
    bool Parse(const char * data, int dataLen, bool final);
    void GetErrorInfo(PString & errorString, unsigned & errorCol, unsigned & errorLine);

    virtual void StartElement(const char * name, const char **attrs);
    virtual void EndElement(const char * name);
    virtual void AddCharacterData(const char * data, int len);
    virtual void XmlDecl(const char * version, const char * encoding, int standAlone);
    virtual void StartDocTypeDecl(const char * docTypeName,
                                  const char * sysid,
                                  const char * pubid,
                                  int hasInternalSubSet);
    virtual void EndDocTypeDecl();
    virtual void StartNamespaceDeclHandler(const char * prefix, const char * uri);
    virtual void EndNamespaceDeclHandler(const char * prefix);

    PString GetVersion() const  { return version; }
    PString GetEncoding() const { return encoding; }

    StandAloneType GetStandAlone() const { return m_standAlone; }

    PXMLElement * GetXMLTree() const;
    PXMLElement * SetXMLTree(PXMLElement * newRoot);

  protected:
    void * expat;
    PXMLElement * rootElement;
    bool rootOpen;
    PXMLElement * currentElement;
    PXMLData * lastElement;
    PString version, encoding;
    StandAloneType m_standAlone;
    PStringToString m_tempNamespaceList;
};

////////////////////////////////////////////////////////////

class PXMLStreamParser : public PXMLParser
{
  PCLASSINFO(PXMLStreamParser, PXMLParser);
  public:
    PXMLStreamParser();

    virtual void EndElement(const char * name);
    virtual PXML * Read(PChannel * channel);

  protected:
    PQueue<PXML> messages;
};


#endif // P_EXPAT

#endif // PTLIB_PXML_H


// End Of File ///////////////////////////////////////////////////////////////
