/*
 * sasl.h
 *
 * Simple Authentication Security Layer interface classes
 *
 * Portable Windows Library
 *
 * Copyright (c) 2004 Reitek S.p.A.
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_PSASL_H
#define PTLIB_PSASL_H

#if P_SASL

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>

class PSASLClient : public PObject
{
    PCLASSINFO(PSASLClient, PObject);

public:
    enum  PSASLResult {
        Continue = 1,
        OK = 0,
        Fail = -1
    };

protected:
    static PString  s_Realm;
    static PString  s_Path;

    void *          m_CallBacks;
    void *          m_ConnState;
    const PString   m_Service;
    const PString   m_UserID;
    const PString   m_AuthID;
    const PString   m_Password;

    PBoolean            Start(const PString& mechanism, const char ** output, unsigned& len);
    PSASLResult     Negotiate(const char * input, const char ** output);

public:
    PSASLClient(const PString& service, const PString& uid, const PString& auth, const PString& pwd);
    ~PSASLClient();

    static void     SetRealm(const PString& realm)  { s_Realm = realm; }
    static void     SetPath(const PString& path)    { s_Path = path; }

    static const PString&  GetRealm()               { return s_Realm; }
    static const PString&  GetPath()                { return s_Path; }

    const PString&  GetService() const  { return m_Service; }
    const PString&  GetUserID() const   { return m_UserID; }
    const PString&  GetAuthID() const   { return m_AuthID; }
    const PString&  GetPassword() const { return m_Password; }

    PBoolean            Init(const PString& fqdn, PStringSet& supportedMechanisms);
    PBoolean            Start(const PString& mechanism, PString& output);
    PSASLResult     Negotiate(const PString& input, PString& output);
    PBoolean            End();
};

#endif  // P_SASL

#endif  // PTLIB_PSASL_H


// End of File ///////////////////////////////////////////////////////////////
