/*
 * pxmlrpcs.h
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

#ifndef PTLIB_XMLRPCSRVR_H
#define PTLIB_XMLRPCSRVR_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptclib/pxmlrpc.h>
#include <ptclib/http.h>


class PXMLRPCServerMethod : public PString
{
  PCLASSINFO(PXMLRPCServerMethod, PString);
  public:
    PXMLRPCServerMethod(const PString & name)
      : PString(name) { }

    PNotifier methodFunc;
};


PSORTED_LIST(PXMLRPCServerMethodList, PXMLRPCServerMethod);


class PXMLRPCServerResource : public PHTTPResource
{
  PCLASSINFO(PXMLRPCServerResource, PHTTPResource);
  public:
    PXMLRPCServerResource();
    PXMLRPCServerResource(
      const PHTTPAuthority & auth    ///< Authorisation for the resource.
    );
    PXMLRPCServerResource(
      const PURL & url               ///< Name of the resource in URL space.
    );
    PXMLRPCServerResource(
      const PURL & url,              ///< Name of the resource in URL space.
      const PHTTPAuthority & auth    ///< Authorisation for the resource.
    );

    // overrides from PHTTPResource
    PBoolean LoadHeaders(PHTTPRequest & request);
    PBoolean OnPOSTData(PHTTPRequest & request, const PStringToString & data);

    // new functions
    virtual void OnXMLRPCRequest(const PString & body, PString & reply);
    virtual PBoolean SetMethod(const PString & methodName, const PNotifier & func);
    void OnXMLRPCRequest(const PString & methodName, PXMLRPCBlock & request, PString & reply);

    virtual PString FormatFault(
      PINDEX code,
      const PString & str
    );

  protected:
    PMutex methodMutex;
    PXMLRPCServerMethodList methodList;
};


class PXMLRPCServerParms : public PObject 
{
  PCLASSINFO(PXMLRPCServerParms, PObject);
  public:
    PXMLRPCServerParms(
      PXMLRPCServerResource & res,
      PXMLRPCBlock & req
    ) : resource(res), request(req) { }

    void SetFault(
      PINDEX code,
      const PString & text
    ) { request.SetFault(code, resource.FormatFault(code, text)); }

    PXMLRPCServerResource & resource;
    PXMLRPCBlock & request;
    PXMLRPCBlock response;
};


#endif // PTLIB_XMLRPCSRVR_H


// End Of File ///////////////////////////////////////////////////////////////
