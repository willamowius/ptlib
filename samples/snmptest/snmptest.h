/*
# snmptest.h
#
# header file for sample application snmptest
#
# Copyright (c) 2008 Tuyan Ozipek
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and limitations
# under the License.
#
# The Original Code is Open Phone Abstraction Library.
#
# The Initial Developer of the Original Code is Equivalence Pty. Ltd.
#
# Contributor(s): ______________________________________.
*/

#include <ptlib.h>
#include <ptlib/pprocess.h>
#include <ptclib/psnmp.h>

class MySNMPServer : public PSNMPServer
{
  PCLASSINFO(MySNMPServer, PSNMPServer)
  public:
    MySNMPServer();
    ~MySNMPServer();

    virtual PBoolean Authorise(const PIPSocket::Address & received);
	virtual PBoolean ConfirmCommunity(PASN_OctetString & community);
	virtual PBoolean OnGetRequest(PINDEX reqID, PSNMP::BindingList & vars, PSNMP::ErrorType & errCode);
    virtual PBoolean ConfirmVersion(PASN_Integer vers);

  protected:
    PRFC1155_SimpleSyntax sys_description;

};

class SNMPSrv : public PProcess
{
  PCLASSINFO(SNMPSrv, PProcess)
  public:
    SNMPSrv();
    void Main();

  protected:
    MySNMPServer srv;
};


