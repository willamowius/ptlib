/*
 * httpsvc.h
 *
 * Common classes for service applications using HTTP as the user interface.
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

#ifndef PTLIB_HTTPSVC_H
#define PTLIB_HTTPSVC_H

#include <ptlib/svcproc.h>
#include <ptlib/sockets.h>
#include <ptclib/httpform.h>
#include <ptclib/cypher.h>


class PHTTPServiceProcess;


/////////////////////////////////////////////////////////////////////

class PHTTPServiceThread : public PThread
{
  PCLASSINFO(PHTTPServiceThread, PThread)
  public:
    PHTTPServiceThread(PINDEX stackSize,
                       PHTTPServiceProcess & app);
    ~PHTTPServiceThread();

    void Main();
    void Close();

  protected:
    PINDEX                myStackSize;
    PHTTPServiceProcess & process;
    PTCPSocket          * socket;
};


/////////////////////////////////////////////////////////////////////

class PHTTPServiceProcess : public PServiceProcess
{
  PCLASSINFO(PHTTPServiceProcess, PServiceProcess)

  public:
    enum {
      MaxSecuredKeys = 10
    };
    struct Info {
      const char * productName;
      const char * manufacturerName;

      WORD majorVersion;
      WORD minorVersion;
      CodeStatus buildStatus;    ///< AlphaCode, BetaCode or ReleaseCode
      WORD buildNumber;
      const char * compilationDate;

      PTEACypher::Key productKey;  ///< Poduct key for registration
      const char * securedKeys[MaxSecuredKeys]; ///< Product secured keys for registration
      PINDEX securedKeyCount;

      PTEACypher::Key signatureKey;   ///< Signature key for encryption of HTML files

      const char * manufHomePage; ///< WWW address of manufacturers home page
      const char * email;         ///< contact email for manufacturer
      const char * productHTML;   ///< HTML for the product name, if NULL defaults to
                                  ///<   the productName variable.
      const char * gifHTML;       ///< HTML to show GIF image in page headers, if NULL
                                  ///<   then the following are used to build one
      const char * gifFilename;   ///< File name for the products GIF file
      int gifWidth;               ///< Size of GIF image, if zero then none is used
      int gifHeight;              ///<   in the generated HTML.

      const char * copyrightHolder;   ///< Name of copyright holder
      const char * copyrightHomePage; ///< Home page for copyright holder
      const char * copyrightEmail;    ///< E-Mail address for copyright holder
    };

    PHTTPServiceProcess(const Info & inf);
    ~PHTTPServiceProcess();

    PBoolean OnStart();
    void OnStop();
    PBoolean OnPause();
    void OnContinue();
    const char * GetServiceDependencies() const;

    virtual void OnConfigChanged() = 0;
    virtual PBoolean Initialise(const char * initMsg) = 0;

    PBoolean ListenForHTTP(
      WORD port,
      PSocket::Reusability reuse = PSocket::CanReuseAddress,
      PINDEX stackSize = 0x4000
    );
    PBoolean ListenForHTTP(
      PSocket * listener,
      PSocket::Reusability reuse = PSocket::CanReuseAddress,
      PINDEX stackSize = 0x4000
    );

    virtual PString GetPageGraphic();
    void GetPageHeader(PHTML &);
    void GetPageHeader(PHTML &, const PString & title);

    virtual PString GetCopyrightText();

    const PString & GetMacroKeyword() const { return macroKeyword; }
    const PTime & GetCompilationDate() const { return compilationDate; }
    const PString & GetHomePage() const { return manufacturersHomePage; }
    const PString & GetEMailAddress() const { return manufacturersEmail; }
    const PString & GetProductName() const { return productNameHTML; }
    const PTEACypher::Key & GetProductKey() const { return productKey; }
    const PStringArray & GetSecuredKeys() const { return securedKeys; }
    const PTEACypher::Key & GetSignatureKey() const { return signatureKey; }
    PBoolean ShouldIgnoreSignatures() const { return ignoreSignatures; }
    void SetIgnoreSignatures(PBoolean ig) { ignoreSignatures = ig; }

    static PHTTPServiceProcess & Current();

    virtual void AddRegisteredText(PHTML & html);
    virtual void AddUnregisteredText(PHTML & html);
    virtual PBoolean SubstituteEquivalSequence(PHTTPRequest & request, const PString &, PString &);
    virtual PHTTPServer * CreateHTTPServer(PTCPSocket & socket);
    virtual PHTTPServer * OnCreateHTTPServer(const PHTTPSpace & urlSpace);
    PTCPSocket * AcceptHTTP();
    PBoolean ProcessHTTP(PTCPSocket & socket);

  protected:
    PSocket  * httpListeningSocket;
    PHTTPSpace httpNameSpace;
    PString    macroKeyword;

    PTEACypher::Key productKey;
    PStringArray    securedKeys;
    PTEACypher::Key signatureKey;
    PBoolean            ignoreSignatures;

    PTime      compilationDate;
    PString    manufacturersHomePage;
    PString    manufacturersEmail;
    PString    productNameHTML;
    PString    gifHTML;
    PString    copyrightHolder;
    PString    copyrightHomePage;
    PString    copyrightEmail;

    void ShutdownListener();
    void BeginRestartSystem();
    void CompleteRestartSystem();

    PThread *  restartThread;

    PLIST(ThreadList, PHTTPServiceThread);
    ThreadList httpThreads;
    PMutex     httpThreadsMutex;

  friend class PConfigPage;
  friend class PConfigSectionsPage;
  friend class PHTTPServiceThread;
};


/////////////////////////////////////////////////////////////////////

class PConfigPage : public PHTTPConfig
{
  PCLASSINFO(PConfigPage, PHTTPConfig)
  public:
    PConfigPage(
      PHTTPServiceProcess & app,
      const PString & section,
      const PHTTPAuthority & auth
    );
    PConfigPage(
      PHTTPServiceProcess & app,
      const PString & title,
      const PString & section,
      const PHTTPAuthority & auth
    );

    void OnLoadedText(PHTTPRequest &, PString & text);

    PBoolean OnPOST(
      PHTTPServer & server,
      const PURL & url,
      const PMIMEInfo & info,
      const PStringToString & data,
      const PHTTPConnectionInfo & connectInfo
    );

    virtual PBoolean Post(
      PHTTPRequest & request,       ///< Information on this request.
      const PStringToString & data, ///< Variables in the POST data.
      PHTML & replyMessage          ///< Reply message for post.
    );

  protected:
    virtual PBoolean GetExpirationDate(
      PTime & when          ///< Time that the resource expires
    );

    PHTTPServiceProcess & process;
};


/////////////////////////////////////////////////////////////////////

class PConfigSectionsPage : public PHTTPConfigSectionList
{
  PCLASSINFO(PConfigSectionsPage, PHTTPConfigSectionList)
  public:
    PConfigSectionsPage(
      PHTTPServiceProcess & app,
      const PURL & url,
      const PHTTPAuthority & auth,
      const PString & prefix,
      const PString & valueName,
      const PURL & editSection,
      const PURL & newSection,
      const PString & newTitle,
      PHTML & heading
    );

    void OnLoadedText(PHTTPRequest &, PString & text);

    PBoolean OnPOST(
      PHTTPServer & server,
      const PURL & url,
      const PMIMEInfo & info,
      const PStringToString & data,
      const PHTTPConnectionInfo & connectInfo
    );

    virtual PBoolean Post(
      PHTTPRequest & request,       ///< Information on this request.
      const PStringToString & data, ///< Variables in the POST data.
      PHTML & replyMessage          ///< Reply message for post.
    );

  protected:
    virtual PBoolean GetExpirationDate(
      PTime & when          ///< Time that the resource expires
    );

    PHTTPServiceProcess & process;
};


/////////////////////////////////////////////////////////////////////

class PRegisterPage : public PConfigPage
{
  PCLASSINFO(PRegisterPage, PConfigPage)
  public:
    PRegisterPage(
      PHTTPServiceProcess & app,
      const PHTTPAuthority & auth
    );

    PString LoadText(
      PHTTPRequest & request        ///< Information on this request.
    );
    void OnLoadedText(PHTTPRequest & request, PString & text);

    virtual PBoolean Post(
      PHTTPRequest & request,       ///< Information on this request.
      const PStringToString & data, ///< Variables in the POST data.
      PHTML & replyMessage          ///< Reply message for post.
    );

    virtual void AddFields(
      const PString & prefix        ///< Prefix on field names
    ) = 0;

  protected:
    PHTTPServiceProcess & process;
};


/////////////////////////////////////////////////////////////////////S

class PServiceHTML : public PHTML
{
  PCLASSINFO(PServiceHTML, PHTML)
  public:
    PServiceHTML(const char * title,
                 const char * help = NULL,
                 const char * helpGif = "help.gif");

    PString ExtractSignature(PString & out);
    static PString ExtractSignature(const PString & html,
                                    PString & out,
                                    const char * keyword = "#equival");

    PString CalculateSignature();
    static PString CalculateSignature(const PString & out);
    static PString CalculateSignature(const PString & out, const PTEACypher::Key & sig);

    PBoolean CheckSignature();
    static PBoolean CheckSignature(const PString & html);

    enum MacroOptions {
      NoOptions           = 0,
      NeedSignature       = 1,
      LoadFromFile        = 2,
      NoURLOverride       = 4,
      NoSignatureForFile  = 8
    };
    static bool ProcessMacros(
      PHTTPRequest & request,
      PString & text,
      const PString & filename,
      unsigned options
    );
    static bool SpliceMacro(
      PString & text,
      const PString & tokens,
      const PString & value
    );
};


///////////////////////////////////////////////////////////////

class PServiceMacro : public PObject
{
  public:
    PServiceMacro(const char * name, PBoolean isBlock);
    PServiceMacro(const PCaselessString & name, PBoolean isBlock);
    Comparison Compare(const PObject & obj) const;
    virtual PString Translate(
      PHTTPRequest & request,
      const PString & args,
      const PString & block
    ) const;
  protected:
    const char * macroName;
    PBoolean isMacroBlock;
    PServiceMacro * link;
    static PServiceMacro * list;
  friend class PServiceMacros_list;
};


#define P_EMPTY

#define PCREATE_SERVICE_MACRO(name, request, args) \
  class PServiceMacro_##name : public PServiceMacro { \
    public: \
      PServiceMacro_##name() : PServiceMacro(#name, false) { } \
      PString Translate(PHTTPRequest &, const PString &, const PString &) const; \
  }; \
  static const PServiceMacro_##name serviceMacro_##name; \
  PString PServiceMacro_##name::Translate(PHTTPRequest & request, const PString & args, const PString &) const



#define PCREATE_SERVICE_MACRO_BLOCK(name, request, args, block) \
  class PServiceMacro_##name : public PServiceMacro { \
    public: \
      PServiceMacro_##name() : PServiceMacro(#name, true) { } \
      PString Translate(PHTTPRequest &, const PString &, const PString &) const; \
  }; \
  static const PServiceMacro_##name serviceMacro_##name; \
  PString PServiceMacro_##name::Translate(PHTTPRequest & request, const PString & args, const PString & block) const



///////////////////////////////////////////////////////////////

class PServiceHTTPString : public PHTTPString
{
  PCLASSINFO(PServiceHTTPString, PHTTPString)
  public:
    PServiceHTTPString(const PURL & url, const PString & string)
      : PHTTPString(url, string) { }

    PServiceHTTPString(const PURL & url, const PHTTPAuthority & auth)
      : PHTTPString(url, auth) { }

    PServiceHTTPString(const PURL & url, const PString & string, const PHTTPAuthority & auth)
      : PHTTPString(url, string, auth) { }

    PServiceHTTPString(const PURL & url, const PString & string, const PString & contentType)
      : PHTTPString(url, string, contentType) { }

    PServiceHTTPString(const PURL & url, const PString & string, const PString & contentType, const PHTTPAuthority & auth)
      : PHTTPString(url, string, contentType, auth) { }

    PString LoadText(PHTTPRequest &);

  protected:
    virtual PBoolean GetExpirationDate(
      PTime & when          ///< Time that the resource expires
    );
};


class PServiceHTTPFile : public PHTTPFile
{
  PCLASSINFO(PServiceHTTPFile, PHTTPFile)
  public:
    PServiceHTTPFile(const PString & filename, PBoolean needSig = false)
      : PHTTPFile(filename) { needSignature = needSig; }
    PServiceHTTPFile(const PString & filename, const PFilePath & file, PBoolean needSig = false)
      : PHTTPFile(filename, file) { needSignature = needSig; }
    PServiceHTTPFile(const PString & filename, const PString & file, PBoolean needSig = false)
      : PHTTPFile(filename, file) { needSignature = needSig; }
    PServiceHTTPFile(const PString & filename, const PHTTPAuthority & auth, PBoolean needSig = false)
      : PHTTPFile(filename, auth) { needSignature = needSig; }
    PServiceHTTPFile(const PString & filename, const PFilePath & file, const PHTTPAuthority & auth, PBoolean needSig = false)
      : PHTTPFile(filename, file, auth) { needSignature = needSig; }

    void OnLoadedText(PHTTPRequest &, PString & text);

  protected:
    virtual PBoolean GetExpirationDate(
      PTime & when          ///< Time that the resource expires
    );

    PBoolean needSignature;
};

class PServiceHTTPDirectory : public PHTTPDirectory
{
  PCLASSINFO(PServiceHTTPDirectory, PHTTPDirectory)
  public:
    PServiceHTTPDirectory(const PURL & url, const PDirectory & dirname, PBoolean needSig = false)
      : PHTTPDirectory(url, dirname) { needSignature = needSig; }

    PServiceHTTPDirectory(const PURL & url, const PDirectory & dirname, const PHTTPAuthority & auth, PBoolean needSig = false)
      : PHTTPDirectory(url, dirname, auth) { needSignature = needSig; }

    void OnLoadedText(PHTTPRequest &, PString & text);

  protected:
    virtual PBoolean GetExpirationDate(
      PTime & when          ///< Time that the resource expires
    );

    PBoolean needSignature;
};


#endif // PTLIB_HTTPSVC_H


// End Of File ///////////////////////////////////////////////////////////////
