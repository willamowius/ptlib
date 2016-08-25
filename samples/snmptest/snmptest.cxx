/*
# snmptest.cxx
#
# C++ source file for sample application snmptest
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

/*
Simple Snmpserver application for ptlib.

Starts a server on 127.0.0.1 port 34500, and creates oid
"1.3.6.1.2.1.1.1.0" # system.Description

usage on linux with snmpget:

snmpget -v1 -c public 127.0.0.1:34500 1.3.6.1.2.1.1.1.0

will print PTLIB_VERSION 
*/



#include "snmptest.h"

/** Start SNMPServer on 127.0.0.1 port 34500
*/
MySNMPServer::MySNMPServer()
  :PSNMPServer(PIPSocket::Address::Address(), 34500)
  ,sys_description(PRFC1155_SimpleSyntax::e_string)
{
  // Create a PRFC1155_ObjectName and set value
  // to '1.3.6.1.2.1.1.1.0' # system Description
  PRFC1155_ObjectName obj_name;
  obj_name.SetValue("1.3.6.1.2.1.1.1.0");

  // Get sys_description object as ASN_OctetString and 
  // Set it to 'PTLIB Version: PTLIB_VERSION'
  PASN_OctetString *str = (PASN_OctetString *) &(sys_description.GetObject());
  str->SetValue(PString("PTLIB Version : ") + PTLIB_VERSION);

  // objList is the dictionary where PSNMPServer stores the 
  // object_name--object mappings.
  // On every request, PSNMPServer::MIB_LocalMatch function matches
  // the oid to object in objList. if found, matched object is added to the result,
  // else, SNMPServer responds with error (noSuchName).
  // since PSNMPServer::MIB_LocalMatch is virtual, you can change this behaviour

  objList.SetAt(obj_name, (PRFC1155_ObjectSyntax *) &sys_description);

  PTrace::SetLevel(1);
  PTRACE(1, "SNMPServer\tWaiting for requests");
}


/** Authorise on ip address of the request
    we are just printing the ip address and accepting it
*/
PBoolean MySNMPServer::Authorise(const PIPSocket::Address & received)
{
  PTRACE(1, "SNMPServer\tReceived request from " << received);
  return PTrue;
}


/** Confirm Community String. We print community string and accept it
*/
PBoolean MySNMPServer::ConfirmCommunity(PASN_OctetString & community)
{
  PTRACE(1, "SNMPServer\tReceived community : " << community);
  return PTrue;
}

/** This is called on every get request
    We are just going to return true for now 
*/
PBoolean MySNMPServer::OnGetRequest(PINDEX reqID, PSNMP::BindingList & vars, PSNMP::ErrorType & errCode)
{
  return PTrue;
}

/* We can confirm the version of the snmp request here
   default is 0 which is snmp version 1 
*/
PBoolean MySNMPServer::ConfirmVersion(PASN_Integer vers)
{
  PTRACE(1,"SNMPServer\tReceived Request version " << vers);
  return PTrue;
}

MySNMPServer::~MySNMPServer()
{
}

PCREATE_PROCESS(SNMPSrv)

void SNMPSrv::Main()
{
  // You can set level to 4 for more debug info
  PTrace::SetLevel(1);
  PThread::Suspend();
}

SNMPSrv::SNMPSrv()
{
	
}
