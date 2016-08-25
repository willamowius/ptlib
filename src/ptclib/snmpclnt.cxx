/*
 * snmpclnt.cxx
 *
 * SNMP Client class
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

#include <ptlib.h>
#include <ptbuildopts.h>

#ifdef P_SNMP

#include <ptclib/psnmp.h>

#define new PNEW


#define SNMP_VERSION 0
#define SNMP_PORT    "snmp 161"

static const char defaultCommunity[] = "public";


///////////////////////////////////////////////////////////////
//
//  PSNMPClient
//

PSNMPClient::PSNMPClient(PINDEX retry, PINDEX timeout,
                         PINDEX rxSize, PINDEX txSize)
 : community(defaultCommunity),
   version(SNMP_VERSION),
   retryMax(retry),
   maxRxSize(rxSize),
   maxTxSize(txSize)
{
  SetReadTimeout(PTimeInterval(0, timeout));
  requestId = rand() % 0x7fffffff;
}


PSNMPClient::PSNMPClient(const PString & host, PINDEX retry,
                         PINDEX timeout, PINDEX rxSize, PINDEX txSize)
 : hostName(host),
   community(defaultCommunity),
   version(SNMP_VERSION),
   retryMax(retry),
   maxRxSize(rxSize),
   maxTxSize(txSize)
{
  SetReadTimeout(PTimeInterval(0, timeout));
  Open(new PUDPSocket(host, SNMP_PORT));
  requestId = rand() % 0x7fffffff;
}


void PSNMPClient::SetVersion(PASNInt newVersion)
{
  version = newVersion;
}


PASNInt PSNMPClient::GetVersion() const
{
  return version;
}


void PSNMPClient::SetCommunity(const PString & str)
{
  community = str;
}


PString PSNMPClient::GetCommunity() const
{
  return community;
}


void PSNMPClient::SetRequestID(PASNInt newRequestID)
{
  requestId = newRequestID;
}


PASNInt PSNMPClient::GetRequestID() const
{
  return requestId;
}


PBoolean PSNMPClient::WriteGetRequest(PSNMPVarBindingList & varsIn,
                                  PSNMPVarBindingList & varsOut)
{
  return WriteRequest(GetRequest, varsIn, varsOut);
}


PBoolean PSNMPClient::WriteGetNextRequest(PSNMPVarBindingList & varsIn,
                                      PSNMPVarBindingList & varsOut)
{
  return WriteRequest(GetNextRequest, varsIn, varsOut);
}


PBoolean PSNMPClient::WriteSetRequest(PSNMPVarBindingList & varsIn,
                                  PSNMPVarBindingList & varsOut)
{
  return WriteRequest(SetRequest, varsIn, varsOut);
}


PSNMP::ErrorType PSNMPClient::GetLastErrorCode() const
{
  return lastErrorCode;
}


PINDEX PSNMPClient::GetLastErrorIndex() const
{
  return lastErrorIndex;
}


PString PSNMPClient::GetLastErrorText() const
{
  return PSNMP::GetErrorText(lastErrorCode);
}

PBoolean PSNMPClient::ReadRequest(PBYTEArray & readBuffer)
{
  readBuffer.SetSize(maxRxSize);
  PINDEX rxSize = 0;

  for (;;) {

    if (!Read(readBuffer.GetPointer()+rxSize, maxRxSize - rxSize)) {

      // if the buffer was too small, then we are receiving datagrams
      // and the datagram was too big
      if (PChannel::GetErrorCode() == PChannel::BufferTooSmall) 
        lastErrorCode = RxBufferTooSmall;
      else
        lastErrorCode = NoResponse;
      return PFalse;

    } else if ((rxSize + GetLastReadCount()) >= 10)
      break;

    else 
      rxSize += GetLastReadCount();
  }

  rxSize += GetLastReadCount();

  PINDEX hdrLen = 1;

  // if not a valid sequence header, then stop reading
  WORD len;
  if ((readBuffer[0] != 0x30) ||
      !PASNObject::DecodeASNLength(readBuffer, hdrLen, len)) {
    lastErrorCode = MalformedResponse;
    return PFalse;
  }

  // length of packet is length of header + length of data
  len = (WORD)(len + hdrLen);

  // return PTrue if we have the packet, else return PFalse
  if (len <= maxRxSize) 
    return PTrue;

  lastErrorCode = RxBufferTooSmall;
  return PFalse;

#if 0
  // and get a new data ptr
  if (maxRxSize < len) 
    readBuffer.SetSize(len);

  // read the remainder of the packet
  while (rxSize < len) {
    if (!Read(readBuffer.GetPointer()+rxSize, len - rxSize)) {
      lastErrorCode = NoResponse;
      return PFalse;
    }
    rxSize += GetLastReadCount();
  }
  return PTrue;
#endif
}

PBoolean PSNMPClient::WriteRequest(PASNInt requestCode,
                               PSNMPVarBindingList & vars,
                               PSNMPVarBindingList & varsOut)
{
  PASNSequence pdu;
  PASNSequence * pduData     = new PASNSequence((BYTE)requestCode);
  PASNSequence * bindingList = new PASNSequence();

  lastErrorIndex = 0;

  // build a get request PDU
  pdu.AppendInteger(version);
  pdu.AppendString(community);
  pdu.Append(pduData);

  // build the PDU data
  PASNInt thisRequestId = requestId;
  requestId = rand() % 0x7fffffff;
  pduData->AppendInteger(thisRequestId);
  pduData->AppendInteger(0);           // error status
  pduData->AppendInteger(0);           // error index
  pduData->Append(bindingList);        // binding list

  // build the binding list
  PINDEX i;
  for (i = 0; i < vars.GetSize(); i++) {
    PASNSequence * binding = new PASNSequence();
    bindingList->Append(binding);
    binding->AppendObjectID(vars.GetObjectID(i));
    binding->Append((PASNObject *)vars[i].Clone());
  }

  // encode the PDU into a buffer
  PBYTEArray sendBuffer;
  pdu.Encode(sendBuffer);

  if (sendBuffer.GetSize() > maxTxSize) {
    lastErrorCode = TxDataTooBig;
    return PFalse;
  }

  varsOut.RemoveAll();

  PINDEX retry = retryMax;

  for (;;) {

    // send the packet
    if (!Write(sendBuffer, sendBuffer.GetSize())) {
      lastErrorCode = SendFailed;
      return PFalse;
    }

    // receive a packet
    if (ReadRequest(readBuffer))
      break;
    else if ((lastErrorCode != NoResponse) || (retry == 0))
      return PFalse;
    else
      retry--;
  }

  // parse the response
  PASNSequence response(readBuffer);
  PINDEX seqLen = response.GetSize();

  // check PDU
  if (seqLen != 3 ||
      response[0].GetType() != PASNObject::Integer ||
      response[1].GetType() != PASNObject::String ||
      response[2].GetType() != PASNObject::Choice) {
    lastErrorCode = MalformedResponse;
    return PFalse;
  }

  // check the PDU data
  const PASNSequence & rPduData = response[2].GetSequence();
  seqLen = rPduData.GetSize();
  if (seqLen != 4 ||
      rPduData.GetChoice()  != GetResponse ||
      rPduData[0].GetType() != PASNObject::Integer ||
      rPduData[1].GetType() != PASNObject::Integer ||
      rPduData[2].GetType() != PASNObject::Integer ||
      rPduData[3].GetType() != PASNObject::Sequence) {
    lastErrorCode = MalformedResponse;
    return PFalse;
  }

  // check the request ID
  PASNInt returnedRequestId = rPduData[0].GetInteger();
  if (returnedRequestId != thisRequestId) {
    lastErrorCode = MalformedResponse;
    return PFalse;
  }
  
  // check the error status and return if non-zero
  PASNInt errorStatus = rPduData[1].GetInteger();
  if (errorStatus != 0) {
    lastErrorIndex = rPduData[2].GetInteger(); 
    lastErrorCode = (ErrorType)errorStatus;
    return PFalse;
  }

  // check the variable bindings
  const PASNSequence & rBindings = rPduData[3].GetSequence();
  PINDEX bindingCount = rBindings.GetSize();

  // create the return list
  for (i = 0; i < bindingCount; i++) {
    if (rBindings[i].GetType() != PASNObject::Sequence) {
      lastErrorIndex = i+1;
      lastErrorCode  = MalformedResponse;
      return PFalse;
    }
    const PASNSequence & rVar = rBindings[i].GetSequence();
    if (rVar.GetSize() != 2 ||
        rVar[0].GetType() != PASNObject::ObjectID) {
      lastErrorIndex = i+1;
      lastErrorCode = MalformedResponse;
      return PFalse;
    }
    varsOut.Append(rVar[0].GetString(), (PASNObject *)rVar[1].Clone());
  }

  lastErrorCode = NoError;
  return PTrue;
}



///////////////////////////////////////////////////////////////
//
//  Trap routines
//

void PSNMP::SendEnterpriseTrap (
                 const PIPSocket::Address & addr,
                            const PString & community,
                            const PString & enterprise,
                                     PINDEX specificTrap,
                               PASNUnsigned timeTicks,
                                       WORD sendPort)
{
  PSNMPVarBindingList vars;
  SendTrap(addr,
           EnterpriseSpecific,
           community,
           enterprise,
           specificTrap,
           timeTicks,
           vars,
           sendPort);
}


void PSNMP::SendEnterpriseTrap (
                 const PIPSocket::Address & addr,
                            const PString & community,
                            const PString & enterprise,
                                     PINDEX specificTrap,
                               PASNUnsigned timeTicks,
                const PSNMPVarBindingList & vars,
                                       WORD sendPort)
{
  SendTrap(addr,
           EnterpriseSpecific,
           community,
           enterprise,
           specificTrap,
           timeTicks,
           vars,
           sendPort);
}


void PSNMP::SendTrap(const PIPSocket::Address & addr,
                                PSNMP::TrapType trapType,
                                const PString & community,
                                const PString & enterprise,
                                         PINDEX specificTrap,
                                   PASNUnsigned timeTicks,
                    const PSNMPVarBindingList & vars,
                                           WORD sendPort)
{
  PIPSocket::Address agentAddress;
  PIPSocket::GetHostAddress(agentAddress);
  SendTrap(addr,
           trapType,
           community,
           enterprise,
           specificTrap,
           timeTicks,
           vars,
           agentAddress,
           sendPort);
}
                            

void PSNMP::SendTrap(const PIPSocket::Address & addr,
                                PSNMP::TrapType trapType,
                                const PString & community,
                                const PString & enterprise,
                                         PINDEX specificTrap,
                                   PASNUnsigned timeTicks,
                    const PSNMPVarBindingList & vars,
                     const PIPSocket::Address & agentAddress,
                                           WORD sendPort)
                            
{
  // send the trap to specified remote host
  PUDPSocket socket(addr, sendPort);
  if (socket.IsOpen())
    WriteTrap(socket, trapType, community, enterprise,
              specificTrap, timeTicks, vars, agentAddress);
}

void PSNMP::WriteTrap(                 PChannel & channel,
                                  PSNMP::TrapType trapType,
                                  const PString & community,
                                  const PString & enterprise,
                                           PINDEX specificTrap,
                                     PASNUnsigned timeTicks,
                      const PSNMPVarBindingList & vars,
                       const PIPSocket::Address & agentAddress)
{
  PASNSequence pdu;
  PASNSequence * pduData     = new PASNSequence((BYTE)Trap);
  PASNSequence * bindingList = new PASNSequence();

  // build a trap PDU PDU
  pdu.AppendInteger(1);
  pdu.AppendString(community);
  pdu.Append(pduData);

  // build the PDU data
  pduData->AppendObjectID(enterprise);               // enterprise
  pduData->Append(new PASNIPAddress(agentAddress)); // agent address
  pduData->AppendInteger((int)trapType);             // trap type
  pduData->AppendInteger(specificTrap);              // specific trap
  pduData->Append(new PASNTimeTicks(timeTicks));    // time of event
  pduData->Append(bindingList);                      // binding list

  // build the binding list
  for (PINDEX i = 0; i < vars.GetSize(); i++) {
    PASNSequence * binding = new PASNSequence();
    bindingList->Append(binding);
    binding->AppendObjectID(vars.GetObjectID(i));
    binding->Append((PASNObject *)vars[i].Clone());
  }

  // encode the PDU into a buffer
  PBYTEArray sendBuffer;
  pdu.Encode(sendBuffer);

  // send the trap to specified remote host
  channel.Write(sendBuffer, sendBuffer.GetSize());
}

#endif // P_SNMP

// End Of File ///////////////////////////////////////////////////////////////
