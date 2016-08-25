/*
 * shttpsvc.h
 *
 * Class for secure service applications using HTTPS as the user interface.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2001 Equivalence Pty. Ltd.
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

#ifndef PTLIB_SHTTPSVC_H
#define PTLIB_SHTTPSVC_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <ptclib/httpsvc.h>
#include <ptclib/pssl.h>


/////////////////////////////////////////////////////////////////////

class PSecureHTTPServiceProcess : public PHTTPServiceProcess
{
  PCLASSINFO(PSecureHTTPServiceProcess, PHTTPServiceProcess)

  public:
    PSecureHTTPServiceProcess(const Info & inf);
    ~PSecureHTTPServiceProcess();

    virtual PHTTPServer * CreateHTTPServer(PTCPSocket & socket);

    PBoolean SetServerCertificate(
      const PFilePath & certFile,
      PBoolean create = false,
      const char * dn = NULL
    );

    virtual PBoolean OnDetectedNonSSLConnection(PChannel * chan, const PString & line);

    virtual PString CreateNonSSLMessage(const PString & url);
    virtual PString CreateRedirectMessage(const PString & url);

  protected:
    PSSLContext * sslContext;
    PBoolean          disableSSL;
};


#endif // PTLIB_SHTTPSVC_H


// End Of File ///////////////////////////////////////////////////////////////
