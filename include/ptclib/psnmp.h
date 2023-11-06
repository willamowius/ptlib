/*
 * psnmp.h
 *
 * Simple Network Management Protocol classes.
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

#ifndef PTLIB_PSNMP_H
#define PTLIB_PSNMP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifdef P_SNMP

#include <ptlib/sockets.h>
#include <ptclib/snmp.h>
#include <ptclib/pasn.h>

#include <list>
#include <vector>

//////////////////////////////////////////////////////////////////////////

/** A list of object IDs and their values
 */
class PSNMPVarBindingList : public PObject
{
  PCLASSINFO(PSNMPVarBindingList, PObject)
  public:

    void Append(const PString & objectID);
    void Append(const PString & objectID, PASNObject * obj);
    void AppendString(const PString & objectID, const PString & str);

    void RemoveAll();

    PINDEX GetSize() const;

    PINDEX GetIndex(const PString & objectID) const;
    PString GetObjectID(PINDEX idx) const;
    PASNObject & operator[](PINDEX idx) const;

    void PrintOn(ostream & strm) const;

  protected:
    PStringArray    objectIds;
    PASNObjectArray values;
};

//////////////////////////////////////////////////////////////////////////

/** A descendant of PUDPSocket which can perform SNMP calls
 */
class PSNMP : public PIndirectChannel
{
  PCLASSINFO(PSNMP, PIndirectChannel)
  public:
    enum ErrorType {
       // Standard RFC1157 errors
       NoError        = 0,
       TooBig         = 1,
       NoSuchName     = 2,
       BadValue       = 3,
       ReadOnly       = 4,
       GenErr         = 5,

       // Additional errors
       NoResponse,
       MalformedResponse,
       SendFailed,
       RxBufferTooSmall,
       TxDataTooBig,
       NumErrors
    };

    enum RequestType {
       GetRequest     = 0,
       GetNextRequest = 1,
       GetResponse    = 2,
       SetRequest     = 3,
       Trap           = 4,
    };

    enum { TrapPort = 162 };

    enum TrapType {
      ColdStart             = 0,
      WarmStart             = 1,
      LinkDown              = 2,
      LinkUp                = 3,
      AuthenticationFailure = 4,
      EGPNeighbourLoss      = 5,
      EnterpriseSpecific    = 6,
      NumTrapTypes
    };

    static PString GetErrorText(ErrorType err);

    static PString GetTrapTypeText(PINDEX code);

    static void SendEnterpriseTrap (
                 const PIPSocket::Address & addr,
                            const PString & community,
                            const PString & enterprise,
                                     PINDEX specificTrap,
                               PASNUnsigned timeTicks,
                                       WORD sendPort = TrapPort);

    static void SendEnterpriseTrap (
                 const PIPSocket::Address & addr,
                            const PString & community,
                            const PString & enterprise,
                                     PINDEX specificTrap,
                               PASNUnsigned timeTicks,
                const PSNMPVarBindingList & vars,
                                       WORD sendPort = TrapPort);

    static void SendTrap (
                       const PIPSocket::Address & addr,
                                  PSNMP::TrapType trapType,
                                  const PString & community,
                                  const PString & enterprise,
                                           PINDEX specificTrap,
                                     PASNUnsigned timeTicks,
                      const PSNMPVarBindingList & vars,
                                             WORD sendPort = TrapPort);

    static void SendTrap (
                      const PIPSocket::Address & addr,
                                  PSNMP::TrapType trapType,
                                  const PString & community,
                                  const PString & enterprise,
                                           PINDEX specificTrap,
                                     PASNUnsigned timeTicks,
                      const PSNMPVarBindingList & vars,
                       const PIPSocket::Address & agentAddress,
                                             WORD sendPort = TrapPort);
                            
    static void WriteTrap (           PChannel & channel,
                                  PSNMP::TrapType trapType,
                                  const PString & community,
                                  const PString & enterprise,
                                           PINDEX specificTrap,
                                     PASNUnsigned timeTicks,
                      const PSNMPVarBindingList & vars,
                       const PIPSocket::Address & agentAddress);

/*
  static PBoolean DecodeTrap(const PBYTEArray & readBuffer,
                                       PINDEX & version,
                                      PString & community,
                                      PString & enterprise,
                           PIPSocket::Address & address,
                                       PINDEX & genericTrapType,
                                      PINDEX  & specificTrapType,
                                 PASNUnsigned & timeTicks,
                          PSNMPVarBindingList & varsOut);
*/

    typedef list<pair<PString,PRFC1155_ObjectSyntax> > BindingList;
};


//////////////////////////////////////////////////////////////////////////

/** Class which gets SNMP data
 */
class PSNMPClient : public PSNMP
{
  PCLASSINFO(PSNMPClient, PSNMP)
  public:
    PSNMPClient(const PString & host,
                PINDEX retryMax = 5,
                PINDEX timeoutMax = 5,
                PINDEX rxBufferSize = 1500,
                PINDEX txSize = 484);

    PSNMPClient(PINDEX retryMax = 5,
                PINDEX timeoutMax = 5,
                PINDEX rxBufferSize = 1500,
                PINDEX txSize = 484);

    void SetVersion(PASNInt version);
    PASNInt GetVersion() const;

    void SetCommunity(const PString & str);
    PString GetCommunity() const;

    void SetRequestID(PASNInt requestID);
    PASNInt GetRequestID() const;

    PBoolean WriteGetRequest (PSNMPVarBindingList & varsIn,
                          PSNMPVarBindingList & varsOut);

    PBoolean WriteGetNextRequest (PSNMPVarBindingList & varsIn,
                              PSNMPVarBindingList & varsOut);

    PBoolean WriteSetRequest (PSNMPVarBindingList & varsIn,
                          PSNMPVarBindingList & varsOut);

    ErrorType GetLastErrorCode() const;
    PINDEX    GetLastErrorIndex() const;
    PString   GetLastErrorText() const;

  protected:
    PBoolean WriteRequest (PASNInt requestCode,
                       PSNMPVarBindingList & varsIn,
                       PSNMPVarBindingList & varsOut);


    PBoolean ReadRequest(PBYTEArray & readBuffer);

    PString   hostName;
    PString   community;
    PASNInt   requestId;
    PASNInt   version;
    PINDEX    retryMax;
    PINDEX    lastErrorIndex;
    ErrorType lastErrorCode;
    PBYTEArray readBuffer;
    PINDEX     maxRxSize;
    PINDEX     maxTxSize;
};


//////////////////////////////////////////////////////////////////////////

/** Class which supplies SNMP data
 */
class PSNMPServer : public PSNMP
{
  PCLASSINFO(PSNMPServer, PSNMP)
  public:

    PSNMPServer(PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), 
                WORD localPort = 161,   
                PINDEX timeout = 5000, 
                PINDEX rxSize = 10000, 
                PINDEX txSize = 10000);

    ~PSNMPServer();

	void Main();

    void SetVersion(PASNInt newVersion);
    PBoolean HandleChannel();
    PBoolean ProcessPDU(const PBYTEArray & readBuffer, PBYTEArray & writeBuffer);

    virtual PBoolean Authorise(const PIPSocket::Address & received);
    virtual PBoolean ConfirmVersion(PASN_Integer vers);
    virtual PBoolean ConfirmCommunity(PASN_OctetString & community);

    virtual PBoolean MIB_LocalMatch(PSNMP_PDU & pdu);

    virtual PBoolean OnGetRequest     (PINDEX reqID, PSNMP::BindingList & vars, PSNMP::ErrorType & errCode);
    virtual PBoolean OnGetNextRequest (PINDEX reqID, PSNMP::BindingList & vars, PSNMP::ErrorType & errCode);
    virtual PBoolean OnSetRequest     (PINDEX reqID, PSNMP::BindingList & vars, PSNMP::ErrorType & errCode);

    PSNMP::ErrorType SendGetResponse  (PSNMPVarBindingList & vars);
  
  protected:
    PThreadObj<PSNMPServer> m_thread;
    PString       community;
    PASN_Integer  version;
    PINDEX        lastErrorIndex;
    ErrorType     lastErrorCode;
    PBYTEArray    readBuffer;
    PINDEX        maxRxSize;
    PINDEX        maxTxSize;
    PUDPSocket   *baseSocket;
    PDictionary<PRFC1155_ObjectName, PRFC1155_ObjectSyntax>  objList;
};

#endif // P_SNMP

#endif // PTLIB_PSNMP_H


// End Of File ///////////////////////////////////////////////////////////////
