/*
 * httpsvc.cxx
 *
 * Classes for service applications using HTTP as the user interface.
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

#ifdef __GNUC__
#pragma implementation "httpsvc.h"
#endif

#include <ptlib.h>
#include <ptbuildopts.h>

#ifdef P_HTTPSVC

#include <ptclib/httpsvc.h>
#include <ptlib/sockets.h>


PSORTED_LIST(PServiceMacros_base, PServiceMacro);

class PServiceMacros_list : public PServiceMacros_base
{
  public:
    PServiceMacros_list();
};


#define new PNEW


#define HOME_PAGE   "http://www.equival.com"
#define EMAIL       "equival@equival.com.au"
#define EQUIVALENCE "Equivalence Pty. Ltd."


static const PTime ImmediateExpiryTime(0, 0, 0, 1, 1, 1980);

PHTTPServiceProcess::PHTTPServiceProcess(const Info & inf)
  : PServiceProcess(inf.manufacturerName, inf.productName,
                    inf.majorVersion, inf.minorVersion, inf.buildStatus, inf.buildNumber),
    macroKeyword("macro"),
    productKey(inf.productKey),
    securedKeys(inf.securedKeyCount, inf.securedKeys),
    signatureKey(inf.signatureKey),
    compilationDate(inf.compilationDate),
    manufacturersHomePage(inf.manufHomePage != NULL ? inf.manufHomePage : HOME_PAGE),
    manufacturersEmail(inf.email != NULL ? inf.email : EMAIL),
    productNameHTML(inf.productHTML != NULL ? inf.productHTML : inf.productName),
    gifHTML(inf.gifHTML),
    copyrightHolder(inf.copyrightHolder != NULL ? inf.copyrightHolder : inf.manufacturerName),
    copyrightHomePage(inf.copyrightHomePage != NULL ? inf.copyrightHomePage : (const char *)manufacturersHomePage),
    copyrightEmail(inf.copyrightEmail != NULL ? inf.copyrightEmail : (const char *)manufacturersEmail)
{
  ignoreSignatures = PFalse;

  if (inf.gifFilename != NULL) {
    PDirectory exeDir = GetFile().GetDirectory();
#if defined(_WIN32) && defined(_DEBUG)
    // Special check to aid in using DevStudio for debugging.
    if (exeDir.Find("\\Debug\\") != P_MAX_INDEX)
      exeDir = exeDir.GetParent();
#endif
    httpNameSpace.AddResource(new PServiceHTTPFile(inf.gifFilename, exeDir+inf.gifFilename));
    if (gifHTML.IsEmpty()) {
      gifHTML = psprintf("<img border=0 src=\"%s\" alt=\"%s!\"", inf.gifFilename, inf.productName);
      if (inf.gifWidth != 0 && inf.gifHeight != 0)
        gifHTML += psprintf(" width=%i height=%i", inf.gifWidth, inf.gifHeight);
      gifHTML += " align=absmiddle>";
    }
  }

  restartThread = NULL;
  httpListeningSocket = NULL;
  httpThreads.DisallowDeleteObjects();
}


PHTTPServiceProcess::~PHTTPServiceProcess()
{
  ShutdownListener();
}


PHTTPServiceProcess & PHTTPServiceProcess::Current() 
{
  PHTTPServiceProcess & process = (PHTTPServiceProcess &)PProcess::Current();
  PAssert(PIsDescendant(&process, PHTTPServiceProcess), "Not a HTTP service!");
  return process;
}


PBoolean PHTTPServiceProcess::OnStart()
{
  if (!Initialise("Started"))
    return PFalse;

  OnConfigChanged();
  return PTrue;
}


void PHTTPServiceProcess::OnStop()
{
  ShutdownListener();
  PSYSTEMLOG(Warning, GetName() << " stopped.");
  PServiceProcess::OnStop();
}


PBoolean PHTTPServiceProcess::OnPause()
{
  OnConfigChanged();
  return PTrue;
}


void PHTTPServiceProcess::OnContinue()
{
  if (Initialise("Restarted"))
    return;

  OnStop();
  Terminate();
}


#ifdef _WIN32
const char * PHTTPServiceProcess::GetServiceDependencies() const
{
  return "EventLog\0Tcpip\0";
}
#endif


PBoolean PHTTPServiceProcess::ListenForHTTP(WORD port,
                                        PSocket::Reusability reuse,
                                        PINDEX stackSize)
{
  if (httpListeningSocket != NULL &&
      httpListeningSocket->GetPort() == port &&
      httpListeningSocket->IsOpen())
    return PTrue;

  return ListenForHTTP(new PTCPSocket(port), reuse, stackSize);
}


PBoolean PHTTPServiceProcess::ListenForHTTP(PSocket * listener,
                                        PSocket::Reusability reuse,
                                        PINDEX stackSize)
{
  if (httpListeningSocket != NULL)
    ShutdownListener();

  httpListeningSocket = PAssertNULL(listener);
  if (!httpListeningSocket->Listen(5, 0, reuse)) {
    PSYSTEMLOG(Debug, "HTTPSVC\tListen on port " << httpListeningSocket->GetPort()
                   << " failed: " << httpListeningSocket->GetErrorText());
    return PFalse;
  }

  if (stackSize > 1000)
    new PHTTPServiceThread(stackSize, *this);

  return PTrue;
}


void PHTTPServiceProcess::ShutdownListener()
{
  if (httpListeningSocket == NULL)
    return;

  if (!httpListeningSocket->IsOpen())
    return;

  PSYSTEMLOG(Debug, "HTTPSVC\tClosing listener socket on port "
                 << httpListeningSocket->GetPort());

  httpListeningSocket->Close();

  httpThreadsMutex.Wait();
  for (ThreadList::iterator i = httpThreads.begin(); i != httpThreads.end(); i++)
    i->Close();

  while (httpThreads.GetSize() > 0) {
    httpThreadsMutex.Signal();
    Sleep(1);
    httpThreadsMutex.Wait();
  }

  httpThreadsMutex.Signal();

  delete httpListeningSocket;
  httpListeningSocket = NULL;
}


PString PHTTPServiceProcess::GetCopyrightText()
{
  PHTML html(PHTML::InBody);
  html << "Copyright &copy;"
       << compilationDate.AsString("yyyy") << " by "
       << PHTML::HotLink(copyrightHomePage)
       << copyrightHolder
       << PHTML::HotLink()
       << ", "
       << PHTML::HotLink("mailto:" + copyrightEmail)
       << copyrightEmail
       << PHTML::HotLink();
  return html;
}


PString PHTTPServiceProcess::GetPageGraphic()
{
  PHTML html(PHTML::InBody);
  html << PHTML::TableStart()
       << PHTML::TableRow()
       << PHTML::TableData()

       << PHTML::HotLink("/");
  if (gifHTML.IsEmpty())
    html << PHTML::Heading(1) << productNameHTML << "&nbsp;" << PHTML::Heading(1);
  else
    html << gifHTML;
  html << PHTML::HotLink()

       << PHTML::TableData()
       << GetOSClass() << ' ' << GetOSName()
       << " Version " << GetVersion(PTrue) << PHTML::BreakLine()
       << ' ' << GetCompilationDate().AsString("d MMMM yyyy")
       << PHTML::BreakLine()
       << "By "
       << PHTML::HotLink(manufacturersHomePage) << GetManufacturer() << PHTML::HotLink()
       << ", "
       << PHTML::HotLink("mailto:" + manufacturersEmail) << manufacturersEmail << PHTML::HotLink()
       << PHTML::TableEnd()
       << PHTML::HRule();

  return html;
}


void PHTTPServiceProcess::GetPageHeader(PHTML & html)
{
  GetPageHeader(html, GetName());
}


void PHTTPServiceProcess::GetPageHeader(PHTML & html, const PString & title)
{
  html << PHTML::Title(title) 
       << PHTML::Body()
       << GetPageGraphic();
}


PTCPSocket * PHTTPServiceProcess::AcceptHTTP()
{
  if (httpListeningSocket == NULL)
    return NULL;

  if (!httpListeningSocket->IsOpen())
    return NULL;

  // get a socket when a client connects
  PTCPSocket * socket = new PTCPSocket;
  if (socket->Accept(*httpListeningSocket))
    return socket;

  if (socket->GetErrorCode() != PChannel::Interrupted) {
    PSYSTEMLOG(Error, "Accept failed for HTTP: " << socket->GetErrorText());
  }

  if (httpListeningSocket != NULL && httpListeningSocket->IsOpen())
    return socket;

  delete socket;
  return NULL;
}


PBoolean PHTTPServiceProcess::ProcessHTTP(PTCPSocket & socket)
{
  if (!socket.IsOpen())
    return PTrue;

  PHTTPServer * server = CreateHTTPServer(socket);
  if (server == NULL) {
    PSYSTEMLOG(Error, "HTTP server creation/open failed.");
    return PTrue;
  }

  // process requests
  while (server->ProcessCommand())
    ;

  // always close after the response has been sent
  delete server;

  // if a restart was requested, then do it, but only if we are not shutting down
  if (httpListeningSocket->IsOpen())
    CompleteRestartSystem();

  return PTrue;
}


void PHTTPServiceProcess::BeginRestartSystem()
{
  if (restartThread == NULL) {
    restartThread = PThread::Current();
    OnConfigChanged();
  }
}


void PHTTPServiceProcess::CompleteRestartSystem()
{
  if (restartThread == NULL)
    return;
  
  if (restartThread != PThread::Current())
    return;

  httpNameSpace.StartWrite();

  if (Initialise("Restart\tInitialisation"))
    restartThread = NULL;

  httpNameSpace.EndWrite();

  if (restartThread != NULL)
    Terminate();
}


void PHTTPServiceProcess::AddRegisteredText(PHTML &)
{
}


void PHTTPServiceProcess::AddUnregisteredText(PHTML &)
{
}


PBoolean PHTTPServiceProcess::SubstituteEquivalSequence(PHTTPRequest &, const PString &, PString &)
{
  return PFalse;
}


PHTTPServer * PHTTPServiceProcess::CreateHTTPServer(PTCPSocket & socket)
{
#ifdef SO_LINGER
  const linger ling = { 1, 5 };
  socket.SetOption(SO_LINGER, &ling, sizeof(ling));
#endif

  PHTTPServer * server = OnCreateHTTPServer(httpNameSpace);

  if (server->Open(socket))
    return server;

  delete server;
  return NULL;
}


PHTTPServer * PHTTPServiceProcess::OnCreateHTTPServer(const PHTTPSpace & httpNameSpace)
{
  return new PHTTPServer(httpNameSpace);
}


//////////////////////////////////////////////////////////////

PHTTPServiceThread::PHTTPServiceThread(PINDEX stackSize,
                                       PHTTPServiceProcess & app)
  : PThread(stackSize, AutoDeleteThread, NormalPriority, "HTTP Service"),
    process(app)
{
  process.httpThreadsMutex.Wait();
  process.httpThreads.Append(this);
  process.httpThreadsMutex.Signal();

  myStackSize = stackSize;
  socket = NULL;
  Resume();
}


PHTTPServiceThread::~PHTTPServiceThread()
{
  process.httpThreadsMutex.Wait();
  process.httpThreads.Remove(this);
  process.httpThreadsMutex.Signal();
  delete socket;
}


void PHTTPServiceThread::Close()
{
  if (socket != NULL)
    socket->Close();
}


void PHTTPServiceThread::Main()
{
  socket = process.AcceptHTTP();
  if (socket != NULL) {
    new PHTTPServiceThread(myStackSize, process);
    process.ProcessHTTP(*socket);
  }
}


//////////////////////////////////////////////////////////////

PConfigPage::PConfigPage(PHTTPServiceProcess & app,
                         const PString & title,
                         const PString & section,
                         const PHTTPAuthority & auth)
  : PHTTPConfig(title, section, auth),
    process(app)
{
}


PConfigPage::PConfigPage(PHTTPServiceProcess & app,
                         const PString & section,
                         const PHTTPAuthority & auth)
  : PHTTPConfig(section.ToLower() + ".html", section, auth),
    process(app)
{
}


void PConfigPage::OnLoadedText(PHTTPRequest & request, PString & text)
{
  PServiceHTML::ProcessMacros(request, text,
                              GetURL().AsString(PURL::PathOnly),
                              PServiceHTML::LoadFromFile);
  PHTTPConfig::OnLoadedText(request, text);
  PServiceHTML::ProcessMacros(request, text, "", PServiceHTML::NoOptions);
}


PBoolean PConfigPage::OnPOST(PHTTPServer & server,
                         const PURL & url,
                         const PMIMEInfo & info,
                         const PStringToString & data,
                         const PHTTPConnectionInfo & connectInfo)
{
  PHTTPConfig::OnPOST(server, url, info, data, connectInfo);
  return PFalse;    // Make sure we break any persistent connections
}


PBoolean PConfigPage::Post(PHTTPRequest & request,
                       const PStringToString & data,
                       PHTML & reply)
{
  PSYSTEMLOG(Debug3, "Post to " << request.url << '\n' << data);
  PBoolean retval = PHTTPConfig::Post(request, data, reply);

  if (request.code == PHTTP::RequestOK)
    process.BeginRestartSystem();

  PServiceHTML::ProcessMacros(request, reply,
                              GetURL().AsString(PURL::PathOnly),
                              PServiceHTML::LoadFromFile);
  OnLoadedText(request, reply);

  return retval;
}


PBoolean PConfigPage::GetExpirationDate(PTime & when)
{
  // Well and truly before now....
  when = ImmediateExpiryTime;
  return PTrue;
}


//////////////////////////////////////////////////////////////

PConfigSectionsPage::PConfigSectionsPage(PHTTPServiceProcess & app,
                                         const PURL & url,
                                         const PHTTPAuthority & auth,
                                         const PString & prefix,
                                         const PString & valueName,
                                         const PURL & editSection,
                                         const PURL & newSection,
                                         const PString & newTitle,
                                         PHTML & heading)
  : PHTTPConfigSectionList(url, auth, prefix, valueName,
                           editSection, newSection, newTitle, heading),
    process(app)
{
}


void PConfigSectionsPage::OnLoadedText(PHTTPRequest & request, PString & text)
{
  PServiceHTML::ProcessMacros(request, text,
                              GetURL().AsString(PURL::PathOnly),
                              PServiceHTML::LoadFromFile);
  PHTTPConfigSectionList::OnLoadedText(request, text);
}


PBoolean PConfigSectionsPage::OnPOST(PHTTPServer & server,
                                 const PURL & url,
                                 const PMIMEInfo & info,
                                 const PStringToString & data,
                                 const PHTTPConnectionInfo & connectInfo)
{
  PHTTPConfigSectionList::OnPOST(server, url, info, data, connectInfo);
  return PFalse;    // Make sure we break any persistent connections
}


PBoolean PConfigSectionsPage::Post(PHTTPRequest & request,
                               const PStringToString & data,
                               PHTML & reply)
{
  PBoolean retval = PHTTPConfigSectionList::Post(request, data, reply);
  if (request.code == PHTTP::RequestOK)
    process.BeginRestartSystem();
  return retval;
}


PBoolean PConfigSectionsPage::GetExpirationDate(PTime & when)
{
  // Well and truly before now....
  when = ImmediateExpiryTime;
  return PTrue;
}


//////////////////////////////////////////////////////////////

PRegisterPage::PRegisterPage(PHTTPServiceProcess & app,
                             const PHTTPAuthority & auth)
  : PConfigPage(app, "register.html", "Secured Options", auth),
    process(app)
{
}


PString PRegisterPage::LoadText(PHTTPRequest & request)
{
  if (fields.GetSize() > 0)
    return PConfigPage::LoadText(request);

  PString mailURL = "mailto:" + process.GetEMailAddress();
  PString orderURL = mailURL;
  PString tempURL = mailURL;
  if (process.GetHomePage() == HOME_PAGE) {
    orderURL = "https://home.equival.com.au/purchase.html";
    tempURL = "http://www.equival.com/" + process.GetName().ToLower() + "/register.html";
    tempURL.Replace(" ", "", PTrue);
  }

  PServiceHTML regPage(process.GetName() & "Registration", NULL);
  regPage << "<!--#registration start Permanent-->"
             "Your registration key is permanent.<p>"
             "Do not change your registration details or your key will not "
             "operate correctly.<p>"
             "If you need to "
          << PHTML::HotLink(orderURL)
          << "upgrade"
          << PHTML::HotLink()
          << " or "
          << PHTML::HotLink(mailURL)
          << "change"
          << PHTML::HotLink()
          << " your registration, then you may enter the new values sent "
          << " to you from "
          << process.GetManufacturer()
          << " into the fields "
             "below, and then press the Accept button.<p>"
          << PHTML::HRule()
          << "<!--#registration end Permanent-->"
             "<!--#registration start Temporary-->"
             "Your registration key is temporary and will expire on "
             "<!--#registration ExpiryDate-->.<p>"
             "Do not change your registration details or your key will not "
             "operate correctly.<p>"
             "You may "
          << PHTML::HotLink(orderURL)
          << "order a permanent key"
          << PHTML::HotLink()
          << " and enter the new values sent to you from "
          << process.GetManufacturer()
          << " into the fields below, and then press the Accept button.<p>"
          << PHTML::HRule()
          << "<!--#registration end Temporary-->"
             "<!--#registration start Expired-->"
             "Your temporary registration key has expired.<p>"
             "You may "
          << PHTML::HotLink(orderURL)
          << "order a permanent key"
          << PHTML::HotLink()
          << " and enter the new values sent to you from "
          << process.GetManufacturer()
          << " into the fields below, and then press the Accept button.<P>"
          << PHTML::HRule()
          << "<!--#registration end Expired-->";

  PSecureConfig securedConf(process.GetProductKey(), process.GetSecuredKeys());
  PString prefix;
  if (securedConf.GetValidation() != PSecureConfig::IsValid) 
    prefix = securedConf.GetPendingPrefix();

  AddFields(prefix);

  Add(new PHTTPStringField("Validation", 40));
  BuildHTML(regPage, InsertIntoHTML);

  regPage << "<!--#registration start Invalid-->"
             "You have entered the values sent to you from "
          << process.GetManufacturer()
          << " incorrectly. Please enter them again. Note, "
          << PHTML::Emphasis() << PHTML::Strong() << "all" << PHTML::Strong() << PHTML::Emphasis()
          << "the fields must be entered "
          << PHTML::Emphasis() << PHTML::Strong() << "exactly" << PHTML::Strong() << PHTML::Emphasis()
          << " as they appear in the e-mail from "
          << process.GetManufacturer()
          << ". We strongly recommend using copy and paste of all the fields, and then "
             "press the Accept button."
             "<!--#registration end Invalid-->"
             "<!--#registration start Default-->"
             "You may "
          << PHTML::HotLink(orderURL)
          << "order a permanent key"
          << PHTML::HotLink()
          << " or "
          << PHTML::HotLink(tempURL)
          << "obtain a temporary key"
          << PHTML::HotLink()
          << " and enter the values sent to you from "
          << process.GetManufacturer()
          << " into the fields above, and then press the Accept button.<p>"
             "<!--#registration end Default-->"
          << PHTML::HRule()
          << PHTML::Heading(3) << "Disclaimer" << PHTML::Heading(3)
          << PHTML::Paragraph() << PHTML::Bold()
          << "The information and code herein is provided \"as is\" "
             "without warranty of any kind, either expressed or implied, "
             "including but not limited to the implied warrenties of "
             "merchantability and fitness for a particular purpose. In "
             "no event shall " << process.GetManufacturer() << " be liable "
             "for any damages whatsoever including direct, indirect, "
             "incidental, consequential, loss of business profits or special "
             "damages, even if " << process.GetManufacturer() << " has been "
             "advised of the possibility of such damages."
          << PHTML::Bold() << PHTML::Paragraph()
          << process.GetCopyrightText()
          << PHTML::Body();

  SetString(regPage);
  return PConfigPage::LoadText(request);
}


static PBoolean FindSpliceBlock(const PRegularExpression & regex,
                            const PString & text,
                            PINDEX & pos,
                            PINDEX & len,
                            PINDEX & start,
                            PINDEX & finish)
{
  if (!text.FindRegEx(regex, pos, len, 0))
    return PFalse;

  PINDEX endpos, endlen;
  static PRegularExpression EndBlock("<?!--#registration[ \t\n]*end[ \t\n]*[a-z]*[ \t\n]*-->?",
                                     PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  if (text.FindRegEx(EndBlock, endpos, endlen, pos)) {
    start = pos+len;
    finish = endpos-1;
    len = endpos - pos + endlen;
  }

  return PTrue;
}



void PRegisterPage::OnLoadedText(PHTTPRequest & request, PString & text)
{
  PString block;
  PINDEX pos, len, start = 0, finish = 0;
  PSecureConfig securedConf(process.GetProductKey(), process.GetSecuredKeys());
  PTime expiry = securedConf.GetTime(securedConf.GetExpiryDateKey());

  static PRegularExpression Default("<?!--#registration[ \t\n]*start[ \t\n]*Default[ \t\n]*-->?",
                                    PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  static PRegularExpression Permanent("<?!--#registration[ \t\n]*start[ \t\n]*Permanent[ \t\n]*-->?",
                                      PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  static PRegularExpression Temporary("<?!--#registration[ \t\n]*start[ \t\n]*Temporary[ \t\n]*-->?",
                                      PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  static PRegularExpression Expired("<?!--#registration[ \t\n]*start[ \t\n]*Expired[ \t\n]*-->?",
                                    PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  static PRegularExpression Invalid("<?!--#registration[ \t\n]*start[ \t\n]*Invalid[ \t\n]*-->?",
                                    PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  static PRegularExpression Pending("name[ \t\n]*=[ \t\n]*\"" +
                                    securedConf.GetPendingPrefix() +
                                    "[^\"]+\"",
                                    PRegularExpression::Extended|PRegularExpression::IgnoreCase);

  PServiceHTML::ProcessMacros(request, text,
                              GetURL().AsString(PURL::PathOnly),
                              PServiceHTML::LoadFromFile);

  switch (securedConf.GetValidation()) {
    case PSecureConfig::Defaults :
      while (FindSpliceBlock(Default, text, pos, len, start, finish))
        text.Splice(text(start, finish), pos, len);
      while (FindSpliceBlock(Permanent, text, pos, len, start, finish))
        text.Delete(pos, len);
      while (FindSpliceBlock(Temporary, text, pos, len, start, finish))
        text.Delete(pos, len);
      while (FindSpliceBlock(Expired, text, pos, len, start, finish))
        text.Delete(pos, len);
      while (FindSpliceBlock(Invalid, text, pos, len, start, finish))
        text.Delete(pos, len);
      break;

    case PSecureConfig::Invalid :
    case PSecureConfig::Pending :
      while (FindSpliceBlock(Default, text, pos, len, start, finish))
        text.Delete(pos, len);
      while (FindSpliceBlock(Permanent, text, pos, len, start, finish))
        text.Delete(pos, len);
      while (FindSpliceBlock(Temporary, text, pos, len, start, finish))
        text.Delete(pos, len);
      while (FindSpliceBlock(Expired, text, pos, len, start, finish))
        text.Delete(pos, len);
      while (FindSpliceBlock(Invalid, text, pos, len, start, finish))
        text.Splice(text(start, finish), pos, len);
      break;

    case PSecureConfig::Expired :
      while (FindSpliceBlock(Default, text, pos, len, start, finish))
        text.Delete(pos, len);
      while (FindSpliceBlock(Permanent, text, pos, len, start, finish))
        text.Delete(pos, len);
      while (FindSpliceBlock(Temporary, text, pos, len, start, finish))
        text.Delete(pos, len);
      while (FindSpliceBlock(Expired, text, pos, len, start, finish))
        text.Splice(text(start, finish), pos, len);
      while (FindSpliceBlock(Invalid, text, pos, len, start, finish))
        text.Delete(pos, len);
      break;

    case PSecureConfig::IsValid :
      while (text.FindRegEx(Pending, pos, len)) {
        static PINDEX pendingLength = securedConf.GetPendingPrefix().GetLength();
        text.Delete(text.Find('"', pos)+1, pendingLength);
        start = pos + len - pendingLength;
      }
      if (expiry.GetYear() < 2011) {
        while (FindSpliceBlock(Default, text, pos, len, start, finish))
          text.Delete(pos, len);
        while (FindSpliceBlock(Permanent, text, pos, len, start, finish))
          text.Delete(pos, len);
        while (FindSpliceBlock(Temporary, text, pos, len, start, finish))
          text.Splice(text(start, finish), pos, len);
        while (FindSpliceBlock(Expired, text, pos, len, start, finish))
          text.Delete(pos, len);
        while (FindSpliceBlock(Invalid, text, pos, len, start, finish))
          text.Delete(pos, len);
      }
      else {
        while (FindSpliceBlock(Default, text, pos, len, start, finish))
          text.Delete(pos, len);
        while (FindSpliceBlock(Permanent, text, pos, len, start, finish))
          text.Splice(text(start, finish), pos, len);
        while (FindSpliceBlock(Temporary, text, pos, len, start, finish))
          text.Delete(pos, len);
        while (FindSpliceBlock(Expired, text, pos, len, start, finish))
          text.Delete(pos, len);
        while (FindSpliceBlock(Invalid, text, pos, len, start, finish))
          text.Delete(pos, len);
      }
  }

  static PRegularExpression ExpiryDate("<?!--#registration[ \t\n]*ExpiryDate[ \t\n]*-->?",
                                       PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  while (text.FindRegEx(ExpiryDate, pos, len, 0))
    text.Splice(expiry.AsString(PTime::LongDate), pos, len);

  PHTTPConfig::OnLoadedText(request, text);
  PServiceHTML::ProcessMacros(request, text, "", PServiceHTML::NoOptions);
}


PBoolean PRegisterPage::Post(PHTTPRequest & request,
                         const PStringToString & data,
                         PHTML & reply)
{
  if (fields.GetSize() == 0)
    LoadText(request);

  PBoolean retval = PHTTPConfig::Post(request, data, reply);
  if (request.code != PHTTP::RequestOK)
    return PFalse;

  PSecureConfig sconf(process.GetProductKey(), process.GetSecuredKeys());
  switch (sconf.GetValidation()) {
    case PSecureConfig::Defaults :
      sconf.ResetPending();
      break;

    case PSecureConfig::IsValid :
      break;

    case PSecureConfig::Pending :
      sconf.ValidatePending();
      break;

    default :
      sconf.ResetPending();
  }

  RemoveAllFields();
  LoadText(request);
  OnLoadedText(request, reply);

  return retval;
}


///////////////////////////////////////////////////////////////////

static void DigestSecuredKeys(PHTTPServiceProcess & process,
                              PString & reginfo,
                              PHTML * html)
{
  const PStringArray & securedKeys = process.GetSecuredKeys();
  PSecureConfig sconf(process.GetProductKey(), securedKeys);

  PString prefix;
  if (sconf.GetValidation() != PSecureConfig::IsValid) 
    prefix = sconf.GetPendingPrefix();

  PMessageDigest5 digestor;

  PStringStream info;
  info << '"' << process.GetName() << "\" ===";

  PINDEX i;
  for (i = 0; i < securedKeys.GetSize(); i++) {
    PString val = sconf.GetString(prefix + securedKeys[i]).Trim();
    info << " \"" << val << '"';
    if (html != NULL)
      *html << PHTML::HiddenField(securedKeys[i], val);
    digestor.Process(val);
  }

  PString digest = digestor.Complete();
  if (html != NULL)
    *html << PHTML::HiddenField("digest", digest);

  info.Replace("===", digest);
  reginfo = info;
}


///////////////////////////////////////////////////////////////////

PServiceHTML::PServiceHTML(const char * title, const char * help, const char * helpGif)
{
  PHTTPServiceProcess::Current().GetPageHeader(*this, title);

  ostream & this_stream = *this;
  this_stream << PHTML::Heading(1) << title;
  
  if (help != NULL)
    this_stream << "&nbsp;"
                << PHTML::HotLink(help)
                << PHTML::Image(helpGif, "Help", 48, 23, "align=absmiddle")
                << PHTML::HotLink();

  this_stream << PHTML::Heading(1) << PHTML::Paragraph();
}


PString PServiceHTML::ExtractSignature(PString & out)
{
  return ExtractSignature(*this, out);
}


PString PServiceHTML::ExtractSignature(const PString & html,
                                       PString & out,
                                       const char * keyword)
{
  out = html;

  PRegularExpression SignatureRegEx("<?!--" + PString(keyword) + "[ \t\r\n]+"
                                     "signature[ \t\r\n]+(-?[^-])+-->?",
                                     PRegularExpression::Extended|PRegularExpression::IgnoreCase);

  PINDEX pos, len;
  if (out.FindRegEx(SignatureRegEx, pos, len)) {
    PString tag = out.Mid(pos, len);
    out.Delete(pos, len);
    return tag(tag.Find("signature")+10, tag.FindLast('-')-2).Trim();
  }

  return PString::Empty();
}


PString PServiceHTML::CalculateSignature()
{
  return CalculateSignature(*this);
}


PString PServiceHTML::CalculateSignature(const PString & out)
{
  return CalculateSignature(out, PHTTPServiceProcess::Current().GetSignatureKey());
}


PString PServiceHTML::CalculateSignature(const PString & out,
                                         const PTEACypher::Key & sig)
{
  // calculate the MD5 digest of the HTML data
  PMessageDigest5 digestor;

  PINDEX p1 = 0;
  PINDEX p2;
  while ((p2 = out.FindOneOf("\r\n", p1)) != P_MAX_INDEX) {
    if (p2 > p1)
      digestor.Process(out(p1, p2-1));
    digestor.Process("\r\n", 2);
    p1 = p2 + 1;
    if (out[p2] == '\r' && out[p1] == '\n') // CR LF pair
      p1++;
  }
  digestor.Process(out(p1, P_MAX_INDEX));

  PMessageDigest5::Code md5;
  digestor.Complete(md5);

  // encode it
  PTEACypher cypher(sig);
  BYTE buf[sizeof(md5)+7];
  memcpy(buf, &md5, sizeof(md5));
  memset(&buf[sizeof(md5)], 0, sizeof(buf)-sizeof(md5));
  return cypher.Encode(buf, sizeof(buf));
}


PBoolean PServiceHTML::CheckSignature()
{
  return CheckSignature(*this);
}


PBoolean PServiceHTML::CheckSignature(const PString & html)
{
  if (PHTTPServiceProcess::Current().ShouldIgnoreSignatures())
    return PTrue;

  // extract the signature from the file
  PString out;
  PString signature = ExtractSignature(html, out);

  // calculate the signature on the data
  PString checkSignature = CalculateSignature(out);

  // return PTrue or PFalse
  return checkSignature == signature;
}


static PBoolean FindBrackets(const PString & args, PINDEX & open, PINDEX & close)
{
  open = args.FindOneOf("[{(", close);
  if (open == P_MAX_INDEX)
    return PFalse;

  switch (args[open]) {
    case '[' :
      close = args.Find(']', open+1);
      break;
    case '{' :
      close = args.Find('}', open+1);
      break;
    case '(' :
      close = args.Find(')', open+1);
      break;
  }
  return close != P_MAX_INDEX;
}


static PBoolean ExtractVariables(const PString & args,
                             PString & variable,
                             PString & value)
{
  PINDEX open;
  PINDEX close = 0;
  if (FindBrackets(args, open, close))
    variable = args(open+1, close-1);
  else {
    variable = args.Trim();
    close = P_MAX_INDEX-1;
  }
  if (variable.IsEmpty())
    return PFalse;

  if (FindBrackets(args, open, close))
    value = args(open+1, close-1);

  return PTrue;
}


///////////////////////////////////////////////////////////////////////////////

PServiceMacro * PServiceMacro::list;


PServiceMacro::PServiceMacro(const char * name, PBoolean isBlock)
{
  macroName = name;
  isMacroBlock = isBlock;
  link = list;
  list = this;
}


PServiceMacro::PServiceMacro(const PCaselessString & name, PBoolean isBlock)
{
  macroName = name;
  isMacroBlock = isBlock;
}


PObject::Comparison PServiceMacro::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PServiceMacro), PInvalidCast);
  const PServiceMacro & other = (const PServiceMacro &)obj;

  if (isMacroBlock != other.isMacroBlock)
    return isMacroBlock ? GreaterThan : LessThan;

  int cmp = strcasecmp(macroName, other.macroName);
  if (cmp < 0)
    return LessThan;
  if (cmp > 0)
    return GreaterThan;
  return EqualTo;
}


PString PServiceMacro::Translate(PHTTPRequest &, const PString &, const PString &) const
{
  return PString::Empty();
};



PServiceMacros_list::PServiceMacros_list()
{
  DisallowDeleteObjects();
  PServiceMacro * macro = PServiceMacro::list;
  while (macro != NULL) {
    Append(macro);
    macro = macro->link;
  }
}


PCREATE_SERVICE_MACRO(Header,request,P_EMPTY)
{
  PString hdr = PHTTPServiceProcess::Current().GetPageGraphic();
  PServiceHTML::ProcessMacros(request, hdr, "header.html",
                PServiceHTML::LoadFromFile|PServiceHTML::NoURLOverride);
  return hdr;
}


PCREATE_SERVICE_MACRO(Copyright,P_EMPTY,P_EMPTY)
{
  return PHTTPServiceProcess::Current().GetCopyrightText();
}


PCREATE_SERVICE_MACRO(ProductName,P_EMPTY,P_EMPTY)
{
  return PHTTPServiceProcess::Current().GetProductName();
}


PCREATE_SERVICE_MACRO(Manufacturer,P_EMPTY,P_EMPTY)
{
  return PHTTPServiceProcess::Current().GetManufacturer();
}


PCREATE_SERVICE_MACRO(Version,P_EMPTY,P_EMPTY)
{
  return PHTTPServiceProcess::Current().GetVersion(PTrue);
}


PCREATE_SERVICE_MACRO(BuildDate,P_EMPTY,args)
{
  const PTime & date = PHTTPServiceProcess::Current().GetCompilationDate();
  if (args.IsEmpty())
    return date.AsString("d MMMM yyyy");

  return date.AsString(args);
}


PCREATE_SERVICE_MACRO(OS,P_EMPTY,P_EMPTY)
{
  return PHTTPServiceProcess::Current().GetOSClass() &
         PHTTPServiceProcess::Current().GetOSName();
}


PCREATE_SERVICE_MACRO(Machine,P_EMPTY,P_EMPTY)
{
  return PHTTPServiceProcess::Current().GetOSVersion() + '-' +
         PHTTPServiceProcess::Current().GetOSHardware();
}


PCREATE_SERVICE_MACRO(LongDateTime,P_EMPTY,P_EMPTY)
{
  return PTime().AsString(PTime::LongDateTime);
}


PCREATE_SERVICE_MACRO(LongDate,P_EMPTY,P_EMPTY)
{
  return PTime().AsString(PTime::LongDate);
}


PCREATE_SERVICE_MACRO(LongTime,P_EMPTY,P_EMPTY)
{
  return PTime().AsString(PTime::LongTime);
}


PCREATE_SERVICE_MACRO(MediumDateTime,P_EMPTY,P_EMPTY)
{
  return PTime().AsString(PTime::MediumDateTime);
}


PCREATE_SERVICE_MACRO(MediumDate,P_EMPTY,P_EMPTY)
{
  return PTime().AsString(PTime::MediumDate);
}


PCREATE_SERVICE_MACRO(ShortDateTime,P_EMPTY,P_EMPTY)
{
  return PTime().AsString(PTime::ShortDateTime);
}


PCREATE_SERVICE_MACRO(ShortDate,P_EMPTY,P_EMPTY)
{
  return PTime().AsString(PTime::ShortDate);
}


PCREATE_SERVICE_MACRO(ShortTime,P_EMPTY,P_EMPTY)
{
  return PTime().AsString(PTime::ShortTime);
}


PCREATE_SERVICE_MACRO(Time,P_EMPTY,args)
{
  PTime now;
  if (args.IsEmpty())
    return now.AsString();

  return now.AsString(args);
}


PCREATE_SERVICE_MACRO(StartTime,P_EMPTY,P_EMPTY)
{
  return PProcess::Current().GetStartTime().AsString(PTime::MediumDateTime);
}


PCREATE_SERVICE_MACRO(UpTime,P_EMPTY,P_EMPTY)
{
  PTimeInterval upTime = PTime() - PProcess::Current().GetStartTime();
  return upTime.AsString(0, PTimeInterval::IncludeDays);
}


PCREATE_SERVICE_MACRO(LocalHost,request,P_EMPTY)
{
  if (request.localAddr != 0)
    return PIPSocket::GetHostName(request.localAddr);
  else
    return PIPSocket::GetHostName();
}


PCREATE_SERVICE_MACRO(LocalIP,request,P_EMPTY)
{
  if (request.localAddr != 0)
    return request.localAddr;
  else
    return "127.0.0.1";
}


PCREATE_SERVICE_MACRO(LocalPort,request,P_EMPTY)
{
  if (request.localPort != 0)
    return psprintf("%u", request.localPort);
  else
    return "80";
}


PCREATE_SERVICE_MACRO(PeerHost,request,P_EMPTY)
{
  if (request.origin != 0)
    return PIPSocket::GetHostName(request.origin);
  else
    return "N/A";
}


PCREATE_SERVICE_MACRO(PeerIP,request,P_EMPTY)
{
  if (request.origin != 0)
    return request.origin;
  else
    return "N/A";
}

PCREATE_SERVICE_MACRO(MonitorInfo,request,P_EMPTY)
{
  const PTime & compilationDate = PHTTPServiceProcess::Current().GetCompilationDate();

  PString peerAddr = "N/A";
  if (request.origin != 0)
    peerAddr = request.origin.AsString();

  PString localAddr = "127.0.0.1";
  if (request.localAddr != 0)
    localAddr = request.localAddr.AsString();

  WORD localPort = 80;
  if (request.localPort != 0)
    localPort = request.localPort;

  PString timeFormat = "yyyyMMdd hhmmss z";

  PTime now;
  PTimeInterval upTime = now - PProcess::Current().GetStartTime();

  PStringStream monitorText; 
  monitorText << "Program: "          << PHTTPServiceProcess::Current().GetProductName() << "\n"
              << "Version: "          << PHTTPServiceProcess::Current().GetVersion(PTrue) << "\n"
              << "Manufacturer: "     << PHTTPServiceProcess::Current().GetManufacturer() << "\n"
              << "OS: "               << PHTTPServiceProcess::Current().GetOSClass() << " " << PHTTPServiceProcess::Current().GetOSName() << "\n"
              << "OS Version: "       << PHTTPServiceProcess::Current().GetOSVersion() << "\n"
              << "Hardware: "         << PHTTPServiceProcess::Current().GetOSHardware() << "\n"
              << "Compilation date: " << compilationDate.AsString(timeFormat, PTime::GMT) << "\n"
              << "Start Date: "       << PProcess::Current().GetStartTime().AsString(timeFormat, PTime::GMT) << "\n"
              << "Current Date: "     << now.AsString(timeFormat, PTime::GMT) << "\n"
              << "Up time: "          << upTime << "\n"
              << "Peer Addr: "        << peerAddr << "\n"
              << "Local Host: "       << PIPSocket::GetHostName() << "\n"
              << "Local Addr: "       << localAddr << "\n"
              << "Local Port: "       << localPort << "\n"
       ;

  return monitorText;
}


PCREATE_SERVICE_MACRO(RegInfo,P_EMPTY,P_EMPTY)
{
  PString subs;
  DigestSecuredKeys(PHTTPServiceProcess::Current(), subs, NULL);
  return subs;
}


static PString GetRegInfo(const char * info)
{
  PHTTPServiceProcess & process = PHTTPServiceProcess::Current();
  PSecureConfig sconf(process.GetProductKey(), process.GetSecuredKeys());
  PString pending = sconf.GetPendingPrefix();
  return sconf.GetString(info, sconf.GetString(pending+info));
}

PCREATE_SERVICE_MACRO(RegUser,P_EMPTY,P_EMPTY)
{
  return GetRegInfo("Name");
}


PCREATE_SERVICE_MACRO(RegCompany,P_EMPTY,P_EMPTY)
{
  return GetRegInfo("Company");
}


PCREATE_SERVICE_MACRO(RegEmail,P_EMPTY,P_EMPTY)
{
  return GetRegInfo("EMail");
}


PCREATE_SERVICE_MACRO(Registration,P_EMPTY,args)
{
  PHTTPServiceProcess & process = PHTTPServiceProcess::Current();
  PSecureConfig sconf(process.GetProductKey(), process.GetSecuredKeys());
  PString pending = sconf.GetPendingPrefix();

  PString regNow = "Register Now!";
  PString viewReg = "View Registration";
  PString demoCopy = "Unregistered Demonstration Copy";
  PINDEX open;
  PINDEX close = 0;
  if (FindBrackets(args, open, close)) {
    regNow = args(open+1, close-1);
    if (FindBrackets(args, open, close)) {
      viewReg = args(open+1, close-1);
      if (FindBrackets(args, open, close))
        demoCopy = args(open+1, close-1);
    }
  }

  PHTML out(PHTML::InBody);
  out << "<font size=5>"
      << sconf.GetString("Name", sconf.GetString(pending+"Name", "*** "+demoCopy+" ***"))
      << PHTML::BreakLine()
      << "<font size=4>"
      << sconf.GetString("Company", sconf.GetString(pending+"Company"))
      << PHTML::BreakLine()
      << PHTML::BreakLine()
      << "<font size=3>";

  if (sconf.GetString("Name").IsEmpty())
    process.AddUnregisteredText(out);
  else
    process.AddRegisteredText(out);

  out << PHTML::HotLink("/register.html")
      << (sconf.GetString("Name").IsEmpty() ? regNow : viewReg)
      << PHTML::HotLink();
  return out;
}


PCREATE_SERVICE_MACRO(InputsFromQuery,request,P_EMPTY)
{
  PStringToString vars = request.url.GetQueryVars();
  PStringStream subs;
  for (PINDEX i = 0; i < vars.GetSize(); i++)
    subs << "<INPUT TYPE=hidden NAME=\"" << vars.GetKeyAt(i)
         << "\" VALUE=\"" << vars.GetDataAt(i) << "\">\r\n";
  return subs;
}


PCREATE_SERVICE_MACRO(Query,request,args)
{
  if (args.IsEmpty())
    return request.url.GetQuery();

  PString variable, value;
  if (ExtractVariables(args, variable, value)) {
    value = request.url.GetQueryVars()(variable, value);
    if (!value)
      return value;
  }
  return PString::Empty();
}


PCREATE_SERVICE_MACRO(Get,request,args)
{
  PString variable, value;
  if (ExtractVariables(args, variable, value)) {
    PString section = request.url.GetQueryVars()("section");
    PINDEX slash = variable.FindLast('\\');
    if (slash != P_MAX_INDEX) {
      section += variable.Left(slash);
      variable = variable.Mid(slash+1);
    }
    if (!section && !variable) {
      PConfig config(section);
      return config.GetString(variable, value);
    }
  }
  return PString::Empty();
}


PCREATE_SERVICE_MACRO(URL,request,P_EMPTY)
{
  return request.url.AsString();
}


PCREATE_SERVICE_MACRO(Include,P_EMPTY,args)
{
  PString text;

  if (!args) {
    PFile file;
    if (file.Open(args, PFile::ReadOnly))
      text = file.ReadString(file.GetLength());
  }

  return text;
}


PCREATE_SERVICE_MACRO(SignedInclude,P_EMPTY,args)
{
  PString text;

  if (!args) {
    PFile file;
    if (file.Open(args, PFile::ReadOnly)) {
      text = file.ReadString(file.GetLength());
      if (!PServiceHTML::CheckSignature(text)) {
        PHTTPServiceProcess & process = PHTTPServiceProcess::Current();
        PHTML html("Invalid OEM Signature");
        html << "The HTML file \""
             << args
             << "\" contains an invalid signature for \""
             << process.GetName()
             << "\" by \""
             << process.GetManufacturer()
             << '"'
             << PHTML::Body();
        text = html;
      }
    }
  }

  return text;
}

PCREATE_SERVICE_MACRO_BLOCK(IfQuery,request,args,block)
{
  PStringToString vars = request.url.GetQueryVars();

  PINDEX space = args.FindOneOf(" \t\r\n");
  PString var = args.Left(space);
  PString value = args.Mid(space).LeftTrim();

  PBoolean ok;
  if (value.IsEmpty())
    ok = vars.Contains(var);
  else {
    PString operation;
    space = value.FindOneOf(" \t\r\n");
    if (space != P_MAX_INDEX) {
      operation = value.Left(space);
      value = value.Mid(space).LeftTrim();
    }

    PString query = vars(var);
    if (operation == "!=")
      ok = query != value;
    else if (operation == "<")
      ok = query < value;
    else if (operation == ">")
      ok = query > value;
    else if (operation == "<=")
      ok = query <= value;
    else if (operation == ">=")
      ok = query >= value;
    else if (operation == "*=")
      ok = (query *= value);
    else
      ok = query == value;
  }

  return ok ? block : PString::Empty();
}


PCREATE_SERVICE_MACRO_BLOCK(IfInURL,request,args,block)
{
  if (request.url.AsString().Find(args) != P_MAX_INDEX)
    return block;

  return PString::Empty();
}


PCREATE_SERVICE_MACRO_BLOCK(IfNotInURL,request,args,block)
{
  if (request.url.AsString().Find(args) == P_MAX_INDEX)
    return block;

  return PString::Empty();
}


static void SplitCmdAndArgs(const PString & text, PINDEX pos, PCaselessString & cmd, PString & args)
{
  static const char whitespace[] = " \t\r\n";
  PString macro = text(text.FindOneOf(whitespace, pos)+1, text.Find("--", pos+3)-1).Trim();
  PINDEX endCmd = macro.FindOneOf(whitespace);
  if (endCmd == P_MAX_INDEX) {
    cmd = macro;
    args.MakeEmpty();
  }
  else {
    cmd = macro.Left(endCmd);
    args = macro.Mid(endCmd+1).LeftTrim();
  }
}


bool PServiceHTML::ProcessMacros(PHTTPRequest & request,
                                 PString & text,
                                 const PString & defaultFile,
                                 unsigned options)
{
  PINDEX alreadyLoadedPrefixLength = 0;

  PString filename = defaultFile;
  if ((options&LoadFromFile) != 0) {
    if ((options&NoURLOverride) == 0) {
      filename = request.url.GetParameters();
      if (filename.IsEmpty())
        filename = defaultFile;
    }

    if (!filename) {
      PString alreadyLoaded = "<!--#loadedfrom " + filename + "-->\r\n";
      alreadyLoadedPrefixLength = alreadyLoaded.GetLength();

      if (text.Find(alreadyLoaded) != 0) {
        PFile file;
        if (file.Open(filename, PFile::ReadOnly)) {
          text = alreadyLoaded + file.ReadString(file.GetLength());
          if ((options&NoSignatureForFile) == 0)
            options |= NeedSignature;
        }
      }
    }
  }

  if ((options&NeedSignature) != 0) {
    if (!CheckSignature(text.Mid(alreadyLoadedPrefixLength))) {
      PHTTPServiceProcess & process = PHTTPServiceProcess::Current();
      PHTML html("Invalid OEM Signature");
      html << "The HTML file \""
           << filename
           << "\" contains an invalid signature for \""
           << process.GetName()
           << "\" by \""
           << process.GetManufacturer()
           << '"'
           << PHTML::Body();
      text = html;
      return PFalse;
    }
  }

  static PServiceMacros_list ServiceMacros;

  PHTTPServiceProcess & process = PHTTPServiceProcess::Current();

  PRegularExpression StartBlockRegEx("<?!--#(equival|" + process.GetMacroKeyword() + ")"
                                     "start[ \t\r\n]+(-?[^-])+-->?",
                                     PRegularExpression::Extended|PRegularExpression::IgnoreCase);

  PRegularExpression MacroRegEx("<?!--#(equival|" + process.GetMacroKeyword() + ")[ \t\r\n]+(-?[^-])+-->?",
                                PRegularExpression::Extended|PRegularExpression::IgnoreCase);

  PBoolean substitedMacro;
  do {
    substitedMacro = PFalse;

    PINDEX pos = 0;
    PINDEX len;
    while (text.FindRegEx(StartBlockRegEx, pos, len, pos)) {
      PString substitution;

      PCaselessString cmd;
      PString args;
      SplitCmdAndArgs(text, pos, cmd, args);

      PRegularExpression EndBlockRegEx("<?!--#(equival|" + process.GetMacroKeyword() + ")"
                                       "end[ \t\r\n]+" + cmd + "(-?[^-])*-->?",
                                       PRegularExpression::Extended|PRegularExpression::IgnoreCase);
      PINDEX endpos, endlen;
      if (text.FindRegEx(EndBlockRegEx, endpos, endlen, pos+len)) {
        PINDEX startpos = pos+len;
        len = endpos-pos + endlen;
        PINDEX idx = ServiceMacros.GetValuesIndex(PServiceMacro(cmd, PTrue));
        if (idx != P_MAX_INDEX) {
          substitution = ServiceMacros[idx].Translate(request, args, text(startpos, endpos-1));
          substitedMacro = PTrue;
        }
      }

      text.Splice(substitution, pos, len);
    }

    pos = 0;
    while (text.FindRegEx(MacroRegEx, pos, len, pos)) {
      PCaselessString cmd;
      PString args;
      SplitCmdAndArgs(text, pos, cmd, args);

      PString substitution;
      if (!process.SubstituteEquivalSequence(request, cmd & args, substitution)) {
        PINDEX idx = ServiceMacros.GetValuesIndex(PServiceMacro(cmd, PFalse));
        if (idx != P_MAX_INDEX) {
          substitution = ServiceMacros[idx].Translate(request, args, PString::Empty());
          substitedMacro = PTrue;
        }
      }

      text.Splice(substitution, pos, len);
    }
  } while (substitedMacro);

  return PTrue;
}


bool PServiceHTML::SpliceMacro(PString & text, const PString & tokens, const PString & value)
{
  PString adjustedTokens = tokens;
  adjustedTokens.Replace(" ", "[ \t\r\n]+");
  PRegularExpression RegEx("<?!--#" + adjustedTokens + "[ \t\r\n]*-->?",
                           PRegularExpression::Extended|PRegularExpression::IgnoreCase);

  bool foundOne = false;

  PINDEX pos, len;
  while (text.FindRegEx(RegEx, pos, len)) {
    text.Splice(value, pos, len);
    foundOne = true;
  }

  return foundOne;
}


///////////////////////////////////////////////////////////////////

static void ServiceOnLoadedText(PString & text)
{
  PHTTPServiceProcess & process = PHTTPServiceProcess::Current();

  PString manuf = "<!--Standard_" + process.GetManufacturer() + "_Header-->";
  if (text.Find(manuf) != P_MAX_INDEX)
    text.Replace(manuf, process.GetPageGraphic(), PTrue);

  static const char equiv[] = "<!--Standard_Equivalence_Header-->";
  if (text.Find(equiv) != P_MAX_INDEX)
    text.Replace(equiv, process.GetPageGraphic(), PTrue);

  static const char copy[] = "<!--Standard_Copyright_Header-->";
  if (text.Find(copy) != P_MAX_INDEX)
    text.Replace(copy, process.GetCopyrightText(), PTrue);
}


PString PServiceHTTPString::LoadText(PHTTPRequest & request)
{
  PString text = PHTTPString::LoadText(request);
  ServiceOnLoadedText(text);
  PServiceHTML::ProcessMacros(request, text, "", PServiceHTML::LoadFromFile);

  return text;
}

PBoolean PServiceHTTPString::GetExpirationDate(PTime & when)
{
  // Well and truly before now....
  when = ImmediateExpiryTime;
  return PTrue;
}


void PServiceHTTPFile::OnLoadedText(PHTTPRequest & request, PString & text)
{
  ServiceOnLoadedText(text);
  PServiceHTML::ProcessMacros(request, text, GetURL().AsString(PURL::PathOnly),
          needSignature ? PServiceHTML::NeedSignature : PServiceHTML::NoOptions);
}

PBoolean PServiceHTTPFile::GetExpirationDate(PTime & when)
{
  // Well and truly before now....
  when = ImmediateExpiryTime;
  return PTrue;
}


void PServiceHTTPDirectory::OnLoadedText(PHTTPRequest & request, PString & text)
{
  ServiceOnLoadedText(text);
  PServiceHTML::ProcessMacros(request, text, GetURL().AsString(PURL::PathOnly),
          needSignature ? PServiceHTML::NeedSignature : PServiceHTML::NoOptions);
}


PBoolean PServiceHTTPDirectory::GetExpirationDate(PTime & when)
{
  // Well and truly before now....
  when = ImmediateExpiryTime;
  return PTrue;
}

#endif // P_HTTPSVC

///////////////////////////////////////////////////////////////////
