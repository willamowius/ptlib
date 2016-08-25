/*
 * pstun.cxx
 *
 * STUN Client
 *
 * Portable Windows Library
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
#pragma implementation "pstun.h"
#endif

#include <ptlib.h>
#include <ptclib/pstun.h>
#include <ptclib/random.h>

#define new PNEW


// Sample server is at larry.gloo.net

#define DEFAULT_REPLY_TIMEOUT 800
#define DEFAULT_POLL_RETRIES  3
#define DEFAULT_NUM_SOCKETS_FOR_PAIRING 4


typedef PSTUNClient PNatMethod_STUN;
PCREATE_NAT_PLUGIN(STUN);

PFACTORY_CREATE(PFactory<PNatMethod>, PSTUNClient, "STUN");


///////////////////////////////////////////////////////////////////////

PSTUNClient::PSTUNClient()
  : serverPort(DefaultPort),
    replyTimeout(DEFAULT_REPLY_TIMEOUT),
    pollRetries(DEFAULT_POLL_RETRIES),
    numSocketsForPairing(DEFAULT_NUM_SOCKETS_FOR_PAIRING),
    natType(UnknownNat),
    cachedExternalAddress(0),
    timeAddressObtained(0)
{
}

PSTUNClient::PSTUNClient(const PString & server,
                         WORD portBase, WORD portMax,
                         WORD portPairBase, WORD portPairMax)
  : serverPort(DefaultPort),
    replyTimeout(DEFAULT_REPLY_TIMEOUT),
    pollRetries(DEFAULT_POLL_RETRIES),
    numSocketsForPairing(DEFAULT_NUM_SOCKETS_FOR_PAIRING),
    natType(UnknownNat),
    cachedExternalAddress(0),
    timeAddressObtained(0)
{
  SetServer(server);
  SetPortRanges(portBase, portMax, portPairBase, portPairMax);
}


PSTUNClient::PSTUNClient(const PIPSocket::Address & address, WORD port,
                         WORD portBase, WORD portMax,
                         WORD portPairBase, WORD portPairMax)
  : serverHost(address.AsString()),
    serverPort(port),
    replyTimeout(DEFAULT_REPLY_TIMEOUT),
    pollRetries(DEFAULT_POLL_RETRIES),
    numSocketsForPairing(DEFAULT_NUM_SOCKETS_FOR_PAIRING),
    natType(UnknownNat),
    cachedExternalAddress(0),
    timeAddressObtained(0)
{
  SetPortRanges(portBase, portMax, portPairBase, portPairMax);
}


void PSTUNClient::Initialise(const PString & server,
                             WORD portBase, WORD portMax,
                             WORD portPairBase, WORD portPairMax)
{
  SetServer(server);
  SetPortRanges(portBase, portMax, portPairBase, portPairMax);
}


bool PSTUNClient::GetServerAddress(PIPSocket::Address & address, WORD & port) const
{
  if (serverPort == 0)
    return false;

  port = serverPort;

  if (cachedServerAddress.IsValid()) {
    address = cachedServerAddress;
    return true;
  }

  return PIPSocket::GetHostAddress(serverHost, address);
}


PBoolean PSTUNClient::SetServer(const PString & server)
{
  PString host;
  WORD port = serverPort;

  PINDEX colon = server.Find(':');
  if (colon == P_MAX_INDEX)
    host = server;
  else {
    host = server.Left(colon);
    PString service = server.Mid(colon+1);
    if ((port = PIPSocket::GetPortByService("udp", service)) == 0) {
      PTRACE(2, "STUN\tCould not find service \"" << service << "\".");
      return false;
    }
  }

  if (host.IsEmpty() || port == 0)
    return false;

  if (serverHost == host && serverPort == port)
    return true;

  serverHost = host;
  serverPort = port;
  InvalidateCache();
  return true;
}


PBoolean PSTUNClient::SetServer(const PIPSocket::Address & address, WORD port)
{
  if (!address.IsValid() || port == 0)
    return false;

  serverHost = address.AsString();
  cachedServerAddress = address;
  serverPort = port;
  return true;
}

#pragma pack(1)

struct PSTUNAttribute
{
  enum Types {
    MAPPED_ADDRESS = 0x0001,
    RESPONSE_ADDRESS = 0x0002,
    CHANGE_REQUEST = 0x0003,
    SOURCE_ADDRESS = 0x0004,
    CHANGED_ADDRESS = 0x0005,
    USERNAME = 0x0006,
    PASSWORD = 0x0007,
    MESSAGE_INTEGRITY = 0x0008,
    ERROR_CODE = 0x0009,
    UNKNOWN_ATTRIBUTES = 0x000a,
    REFLECTED_FROM = 0x000b,
    MaxValidCode
  };
  
  PUInt16b type;
  PUInt16b length;
  
  PSTUNAttribute * GetNext() const { return (PSTUNAttribute *)(((const BYTE *)this)+length+4); }
};

class PSTUNAddressAttribute : public PSTUNAttribute
{
public:
  BYTE     pad;
  BYTE     family;
  PUInt16b port;
  BYTE     ip[4];

  PIPSocket::Address GetIP() const { return PIPSocket::Address(4, ip); }

protected:
  enum { SizeofAddressAttribute = sizeof(BYTE)+sizeof(BYTE)+sizeof(WORD)+sizeof(PIPSocket::Address) };
  void InitAddrAttr(Types newType)
  {
    type = (WORD)newType;
    length = SizeofAddressAttribute;
    pad = 0;
    family = 1;
  }
  bool IsValidAddrAttr(Types checkType) const
  {
    return type == checkType && length == SizeofAddressAttribute;
  }
};

class PSTUNMappedAddress : public PSTUNAddressAttribute
{
public:
  void Initialise() { InitAddrAttr(MAPPED_ADDRESS); }
  bool IsValid() const { return IsValidAddrAttr(MAPPED_ADDRESS); }
};

class PSTUNChangedAddress : public PSTUNAddressAttribute
{
public:
  void Initialise() { InitAddrAttr(CHANGED_ADDRESS); }
  bool IsValid() const { return IsValidAddrAttr(CHANGED_ADDRESS); }
};

class PSTUNChangeRequest : public PSTUNAttribute
{
public:
  BYTE flags[4];
  
  PSTUNChangeRequest() { }

  PSTUNChangeRequest(bool changeIP, bool changePort)
  {
    Initialise();
    SetChangeIP(changeIP);
    SetChangePort(changePort);
  }

  void Initialise()
  {
    type = CHANGE_REQUEST;
    length = sizeof(flags);
    memset(flags, 0, sizeof(flags));
  }
  bool IsValid() const { return type == CHANGE_REQUEST && length == sizeof(flags); }
  
  bool GetChangeIP() const { return (flags[3]&4) != 0; }
  void SetChangeIP(bool on) { if (on) flags[3] |= 4; else flags[3] &= ~4; }
  
  bool GetChangePort() const { return (flags[3]&2) != 0; }
  void SetChangePort(bool on) { if (on) flags[3] |= 2; else flags[3] &= ~2; }
};

class PSTUNMessageIntegrity : public PSTUNAttribute
{
public:
  BYTE hmac[20];
  
  void Initialise()
  {
    type = MESSAGE_INTEGRITY;
    length = sizeof(hmac);
    memset(hmac, 0, sizeof(hmac));
  }
  bool IsValid() const { return type == MESSAGE_INTEGRITY && length == sizeof(hmac); }
};

struct PSTUNMessageHeader
{
  PUInt16b       msgType;
  PUInt16b       msgLength;
  BYTE           transactionId[16];
};


#pragma pack()


class PSTUNMessage : public PBYTEArray
{
public:
  enum MsgType {
    BindingRequest  = 0x0001,
    BindingResponse = 0x0101,
    BindingError    = 0x0111,
      
    SharedSecretRequest  = 0x0002,
    SharedSecretResponse = 0x0102,
    SharedSecretError    = 0x0112,
  };
  
  PSTUNMessage()
  { }
  
  PSTUNMessage(MsgType newType, const BYTE * id = NULL)
    : PBYTEArray(sizeof(PSTUNMessageHeader))
  {
    SetType(newType, id);
  }

  void SetType(MsgType newType, const BYTE * id = NULL)
  {
    SetMinSize(sizeof(PSTUNMessageHeader));
    PSTUNMessageHeader * hdr = (PSTUNMessageHeader *)theArray;
    hdr->msgType = (WORD)newType;
    for (PINDEX i = 0; i < ((PINDEX)sizeof(hdr->transactionId)); i++)
      hdr->transactionId[i] = id != NULL ? id[i] : (BYTE)PRandom::Number();
  }

  const PSTUNMessageHeader * operator->() const { return (PSTUNMessageHeader *)theArray; }
  
  PSTUNAttribute * GetFirstAttribute() { 

    int length = ((PSTUNMessageHeader *)theArray)->msgLength;
    if (theArray == NULL || length < (int) sizeof(PSTUNMessageHeader))
      return NULL;

    PSTUNAttribute * attr = (PSTUNAttribute *)(theArray+sizeof(PSTUNMessageHeader)); 
    PSTUNAttribute * ptr = attr;

    if (attr->length > GetSize() || attr->type >= PSTUNAttribute::MaxValidCode)
      return NULL;

    while (ptr && (BYTE*) ptr < (BYTE*)(theArray+GetSize()) && length >= (int) ptr->length+4) {

      length -= ptr->length + 4;
      ptr = ptr->GetNext();
    }

    if (length != 0)
      return NULL;

    return attr; 
  }

  bool Validate(const PSTUNMessage & request)
  {
    int length = ((PSTUNMessageHeader *)theArray)->msgLength;
    PSTUNAttribute * attrib = GetFirstAttribute();
    while (attrib && length > 0) {
      length -= attrib->length + 4;
      attrib = attrib->GetNext();
    }

    if (length != 0) {
      PTRACE(2, "STUN\tInvalid reply packet received, incorrect attribute length.");
      return false;
    }

    if (memcmp(request->transactionId, (*this)->transactionId, sizeof(request->transactionId)) != 0) {
      PTRACE(2, "STUN\tInvalid reply packet received, transaction ID does not match.");
      return false;
    }

    return true;
  }

  void AddAttribute(const PSTUNAttribute & attribute)
  {
    PSTUNMessageHeader * hdr = (PSTUNMessageHeader *)theArray;
    int oldLength = hdr->msgLength;
    int attrSize = attribute.length + 4;
    int newLength = oldLength + attrSize;
    hdr->msgLength = (WORD)newLength;
    // hdr pointer may be invalidated by next statement
    SetMinSize(newLength+sizeof(PSTUNMessageHeader));
    memcpy(theArray+sizeof(PSTUNMessageHeader)+oldLength, &attribute, attrSize);
  }

  void SetAttribute(const PSTUNAttribute & attribute)
  {
    int length = ((PSTUNMessageHeader *)theArray)->msgLength;
    PSTUNAttribute * attrib = GetFirstAttribute();
    while (length > 0) {
      if (attrib->type == attribute.type) {
        if (attrib->length == attribute.length)
          *attrib = attribute;
        else {
          // More here
        }
        return;
      }

      length -= attrib->length + 4;
      attrib = attrib->GetNext();
    }

    AddAttribute(attribute);
  }

  PSTUNAttribute * FindAttribute(PSTUNAttribute::Types type)
  {
    int length = ((PSTUNMessageHeader *)theArray)->msgLength;
    PSTUNAttribute * attrib = GetFirstAttribute();
    while (length > 0) {
      if (attrib->type == type)
        return attrib;

      length -= attrib->length + 4;
      attrib = attrib->GetNext();
    }
    return NULL;
  }


  bool Read(PUDPSocket & socket)
  {
    if (!socket.Read(GetPointer(1000), 1000))
      return false;

    SetSize(socket.GetLastReadCount());
    return true;
  }
  
  bool Write(PUDPSocket & socket) const
  {
    if (socket.Write(theArray, ((PSTUNMessageHeader *)theArray)->msgLength+sizeof(PSTUNMessageHeader)))
      return true;

    PTRACE(1, "STUN\tError writing to " << socket.GetSendAddress()
           << " - " << socket.GetErrorText(PChannel::LastWriteError));
    return false;
  }

  bool Poll(PUDPSocket & socket, const PSTUNMessage & request, PINDEX pollRetries)
  {
    for (PINDEX retry = 0; retry < pollRetries; retry++) {
      if (!request.Write(socket))
        return false;

      if (Read(socket) && Validate(request))
        return true;
    }

    PTRACE(5, "STUN\tNo response from " << socket.GetSendAddress() << " after " << pollRetries << " retries.");
    return false;
  }
};


bool PSTUNClient::OpenSocket(PUDPSocket & socket, PortInfo & portInfo, const PIPSocket::Address & binding)
{
  if (serverPort == 0) {
    PTRACE(1, "STUN\tServer port not set.");
    return false;
  }

  if (!PIPSocket::GetHostAddress(serverHost, cachedServerAddress) || !cachedServerAddress.IsValid()) {
    PTRACE(2, "STUN\tCould not find host \"" << serverHost << "\".");
    return false;
  }

  PWaitAndSignal mutex(portInfo.mutex);

  WORD startPort = portInfo.currentPort;

  do {
    portInfo.currentPort++;
    if (portInfo.currentPort > portInfo.maxPort)
      portInfo.currentPort = portInfo.basePort;

    if (socket.Listen(binding, 1, portInfo.currentPort)) {
      socket.SetSendAddress(cachedServerAddress, serverPort);
      socket.SetReadTimeout(replyTimeout);
      return true;
    }

  } while (portInfo.currentPort != startPort);

  PTRACE(1, "STUN\tFailed to bind to local UDP port in range "
         << portInfo.currentPort << '-' << portInfo.maxPort);
  return false;
}


PSTUNClient::NatTypes PSTUNClient::GetNatType(PBoolean force)
{
  if (!force && natType != UnknownNat)
    return natType;

  PList<PUDPSocket> sockets;

  PIPSocket::InterfaceTable interfaces;
  if (PIPSocket::GetInterfaceTable(interfaces)) {
    for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
      PIPSocket::Address binding = interfaces[i].GetAddress();
      if (!binding.IsLoopback() && binding.GetVersion() == 4) {
        PUDPSocket * socket = new PUDPSocket;
        if (OpenSocket(*socket, singlePortInfo, binding))
          sockets.Append(socket);
        else
          delete socket;
      }
    }
    if (interfaces.IsEmpty()) {
      PTRACE(1, "STUN\tNo interfaces available to find STUN server.");
      return natType = UnknownNat;
    }
  }
  else {
    PUDPSocket * socket = new PUDPSocket;
    sockets.Append(socket);
    if (!OpenSocket(*socket, singlePortInfo, PIPSocket::GetDefaultIpAny()))
      return natType = UnknownNat;
  }

  // RFC3489 discovery

  /* test I - the client sends a STUN Binding Request to a server, without
     any flags set in the CHANGE-REQUEST attribute, and without the
     RESPONSE-ADDRESS attribute. This causes the server to send the response
     back to the address and port that the request came from. */
  PSTUNMessage requestI(PSTUNMessage::BindingRequest);
  requestI.AddAttribute(PSTUNChangeRequest(false, false));
  PSTUNMessage responseI;

  PUDPSocket * replySocket = NULL;

  for (PINDEX retry = 0; retry < pollRetries; ++retry) {
    PSocket::SelectList selectList;
    for (PList<PUDPSocket>::iterator socket = sockets.begin(); socket != sockets.end(); ++socket) {
      if (requestI.Write(*socket))
        selectList += *socket;
    }

    if (selectList.IsEmpty())
      return natType = UnknownNat; // Could not send on any interface!

    PChannel::Errors error = PIPSocket::Select(selectList, replyTimeout);
    if (error != PChannel::NoError) {
      PTRACE(1, "STUN\tError in select - " << PChannel::GetErrorText(error));
      return natType = UnknownNat;
    }

    if (!selectList.IsEmpty()) {
      PUDPSocket & udp = (PUDPSocket &)selectList.front();
      if (responseI.Read(udp) && responseI.Validate(requestI)) {
        replySocket = &udp;
        break;
      }
    }
  }

  if (replySocket == NULL) {
    PTRACE(3, "STUN\tNo response to " << *this);
    return natType = BlockedNat; // No response usually means blocked
  }

  replySocket->GetLocalAddress(interfaceAddress);

  PSTUNMappedAddress * mappedAddress = (PSTUNMappedAddress *)responseI.FindAttribute(PSTUNAttribute::MAPPED_ADDRESS);
  if (mappedAddress == NULL) {
    PTRACE(2, "STUN\tExpected mapped address attribute from " << *this);
    return natType = UnknownNat; // Protocol error
  }

  PIPSocket::Address mappedAddressI = mappedAddress->GetIP();
  WORD mappedPortI = mappedAddress->port;
  bool notNAT = replySocket->GetPort() == mappedPortI && PIPSocket::IsLocalHost(mappedAddressI);

  /* Test II - the client sends a Binding Request with both the "change IP"
     and "change port" flags from the CHANGE-REQUEST attribute set. */
  PSTUNMessage requestII(PSTUNMessage::BindingRequest);
  requestII.AddAttribute(PSTUNChangeRequest(true, true));
  PSTUNMessage responseII;
  bool testII = responseII.Poll(*replySocket, requestII, pollRetries);

  if (notNAT) {
    // Is not NAT or symmetric firewall
    return natType = (testII ? OpenNat : SymmetricFirewall);
  }

  cachedExternalAddress = mappedAddressI;
  timeAddressObtained.SetCurrentTime();

  if (testII)
    return natType = ConeNat;

  PSTUNChangedAddress * changedAddress = (PSTUNChangedAddress *)responseI.FindAttribute(PSTUNAttribute::CHANGED_ADDRESS);
  if (changedAddress == NULL)
    return natType = UnknownNat; // Protocol error

  // Send test I to another server, to see if restricted or symmetric
  PIPSocket::Address secondaryServer = changedAddress->GetIP();
  WORD secondaryPort = changedAddress->port;
  replySocket->SetSendAddress(secondaryServer, secondaryPort);
  PSTUNMessage requestI2(PSTUNMessage::BindingRequest);
  requestI2.AddAttribute(PSTUNChangeRequest(false, false));
  PSTUNMessage responseI2;
  if (!responseI2.Poll(*replySocket, requestI2, pollRetries)) {
    PTRACE(2, "STUN\tPoll of secondary server " << secondaryServer << ':' << secondaryPort
           << " failed, NAT partially blocked by firwall rules.");
    return natType = PartialBlockedNat;
  }

  mappedAddress = (PSTUNMappedAddress *)responseI2.FindAttribute(PSTUNAttribute::MAPPED_ADDRESS);
  if (mappedAddress == NULL) {
    PTRACE(2, "STUN\tExpected mapped address attribute from " << *this);
    return UnknownNat; // Protocol error
  }

  if (mappedAddress->port != mappedPortI || mappedAddress->GetIP() != mappedAddressI)
    return natType = SymmetricNat;

  replySocket->SetSendAddress(cachedServerAddress, serverPort);
  PSTUNMessage requestIII(PSTUNMessage::BindingRequest);
  requestIII.SetAttribute(PSTUNChangeRequest(false, true));
  PSTUNMessage responseIII;
  return natType = (responseIII.Poll(*replySocket, requestIII, pollRetries) ? RestrictedNat : PortRestrictedNat);
}


PString PSTUNClient::GetNatTypeString(NatTypes type)
{
  static const char * const Names[NumNatTypes] = {
    "Unknown NAT",
    "Open NAT",
    "Cone NAT",
    "Restricted NAT",
    "Port Restricted NAT",
    "Symmetric NAT",
    "Symmetric Firewall",
    "Blocked",
    "Partially Blocked"
  };

  if (type < NumNatTypes)
    return Names[type];
  
  return psprintf("<NATType %u>", type);
}


PSTUNClient::RTPSupportTypes PSTUNClient::GetRTPSupport(PBoolean force)
{
  switch (GetNatType(force)) {
    // types that do support RTP 
    case OpenNat:
    case ConeNat:
      return RTPSupported;

    // types that support RTP if media sent first
    case SymmetricFirewall:
    case RestrictedNat:
    case PortRestrictedNat:
      return RTPIfSendMedia;

    // types that do not support RTP
    case BlockedNat:
    case SymmetricNat:
      return RTPUnsupported;

    // types that have unknown RTP support
    default:
      return RTPUnknown;
  }
}

PBoolean PSTUNClient::GetExternalAddress(PIPSocket::Address & externalAddress,
                                     const PTimeInterval & maxAge)
{
  if (cachedExternalAddress.IsValid() && (PTime() - timeAddressObtained < maxAge)) {
    externalAddress = cachedExternalAddress;
    return PTrue;
  }

  externalAddress = 0; // Set to invalid address

  PUDPSocket socket;
  if (!OpenSocket(socket, singlePortInfo, PIPSocket::GetDefaultIpAny()))
    return false;

  PSTUNMessage request(PSTUNMessage::BindingRequest);
  request.AddAttribute(PSTUNChangeRequest(false, false));
  PSTUNMessage response;
  if (!response.Poll(socket, request, pollRetries))
  {
    PTRACE(1, "STUN\t" << *this << " unexpectedly went offline getting external address.");
    return false;
  }

  PSTUNMappedAddress * mappedAddress = (PSTUNMappedAddress *)response.FindAttribute(PSTUNAttribute::MAPPED_ADDRESS);
  if (mappedAddress == NULL)
  {
    PTRACE(2, "STUN\tExpected mapped address attribute from " << *this);
    return false;
  }

  
  externalAddress = cachedExternalAddress = mappedAddress->GetIP();
  timeAddressObtained.SetCurrentTime();
  return true;
}


bool PSTUNClient::GetInterfaceAddress(PIPSocket::Address & internalAddress) const
{
  if (!interfaceAddress.IsValid())
    return false;

  internalAddress = interfaceAddress;
  return true;
}


void PSTUNClient::InvalidateCache()
{
  cachedServerAddress = 0;
  cachedExternalAddress = 0;
  interfaceAddress = 0;
  natType = UnknownNat;
}


PBoolean PSTUNClient::CreateSocket(PUDPSocket * & socket, const PIPSocket::Address & binding, WORD localPort)
{
  socket = NULL;

  switch (GetNatType(PFalse)) {
    case OpenNat :
    case ConeNat :
    case RestrictedNat :
    case PortRestrictedNat :
      break;

    case SymmetricNat :
      if (localPort == 0 && (singlePortInfo.basePort == 0 || singlePortInfo.basePort > singlePortInfo.maxPort))
      {
        PTRACE(1, "STUN\tInvalid local UDP port range "
               << singlePortInfo.currentPort << '-' << singlePortInfo.maxPort);
        return PFalse;
      }
      break;

    default : // UnknownNet, SymmetricFirewall, BlockedNat
      PTRACE(1, "STUN\tCannot create socket using NAT type " << GetNatTypeName());
      return PFalse;
  }

  if (!IsAvailable(binding)) {
    PTRACE(1, "STUN\tCannot create socket using binding " << binding);
    return PFalse;
  }

  PSTUNUDPSocket * stunSocket = new PSTUNUDPSocket;

  PBoolean opened;
  if (localPort == 0)
    opened = OpenSocket(*stunSocket, singlePortInfo, interfaceAddress);
  else {
    PortInfo portInfo = localPort;
    opened = OpenSocket(*stunSocket, portInfo, interfaceAddress);
  }

  if (opened)
  {
    PSTUNMessage request(PSTUNMessage::BindingRequest);
    request.AddAttribute(PSTUNChangeRequest(false, false));
    PSTUNMessage response;

    if (response.Poll(*stunSocket, request, pollRetries))
    {
      PSTUNMappedAddress * mappedAddress = (PSTUNMappedAddress *)response.FindAttribute(PSTUNAttribute::MAPPED_ADDRESS);
      if (mappedAddress != NULL)
      {
        stunSocket->externalIP = mappedAddress->GetIP();
        if (GetNatType(PFalse) != SymmetricNat)
          stunSocket->port = mappedAddress->port;
        stunSocket->SetSendAddress(0, 0);
        stunSocket->SetReadTimeout(PMaxTimeInterval);
        socket = stunSocket;
        return true;
      }

      PTRACE(2, "STUN\tExpected mapped address attribute from " << *this);
    }
    else
      PTRACE(1, "STUN\t" << *this << " unexpectedly went offline creating socket.");
  }

  delete stunSocket;
  return false;
}


PBoolean PSTUNClient::CreateSocketPair(PUDPSocket * & socket1,
                                   PUDPSocket * & socket2,
                                   const PIPSocket::Address & binding)
{
  socket1 = NULL;
  socket2 = NULL;

  switch (GetNatType(PFalse)) {
    case OpenNat :
    case ConeNat :
    case RestrictedNat :
    case PortRestrictedNat :
      break;

    case SymmetricNat :
      if (pairedPortInfo.basePort == 0 || pairedPortInfo.basePort > pairedPortInfo.maxPort)
      {
        PTRACE(1, "STUN\tInvalid local UDP port range "
               << pairedPortInfo.currentPort << '-' << pairedPortInfo.maxPort);
        return PFalse;
      }
      break;

    default : // UnknownNet, SymmetricFirewall, BlockedNat
      PTRACE(1, "STUN\tCannot create socket pair using NAT type " << GetNatTypeName());
      return PFalse;
  }

  if (!IsAvailable(binding)) {
    PTRACE(1, "STUN\tCannot create socket using binding " << binding);
    return PFalse;
  }

  PINDEX i;

  PArray<PSTUNUDPSocket> stunSocket;
  PArray<PSTUNMessage> request;
  PArray<PSTUNMessage> response;

  for (i = 0; i < numSocketsForPairing; i++)
  {
    PINDEX idx = stunSocket.Append(new PSTUNUDPSocket);
    if (!OpenSocket(stunSocket[idx], pairedPortInfo, interfaceAddress)) {
      PTRACE(1, "STUN\tUnable to open socket to " << *this);
      return false;
    }

    idx = request.Append(new PSTUNMessage(PSTUNMessage::BindingRequest));
    request[idx].AddAttribute(PSTUNChangeRequest(false, false));

    response.Append(new PSTUNMessage);
  }

  for (i = 0; i < numSocketsForPairing; i++)
  {
    if (!response[i].Poll(stunSocket[i], request[i], pollRetries))
    {
      PTRACE(1, "STUN\t" << *this << " unexpectedly went offline creating socket pair.");
      return false;
    }
  }

  for (i = 0; i < numSocketsForPairing; i++)
  {
    PSTUNMappedAddress * mappedAddress = (PSTUNMappedAddress *)response[i].FindAttribute(PSTUNAttribute::MAPPED_ADDRESS);
    if (mappedAddress == NULL)
    {
      PTRACE(2, "STUN\tExpected mapped address attribute from " << *this);
      return false;
    }
    if (GetNatType(PFalse) != SymmetricNat)
      stunSocket[i].port = mappedAddress->port;
    stunSocket[i].externalIP = mappedAddress->GetIP();
  }

  for (i = 0; i < numSocketsForPairing; i++)
  {
    for (PINDEX j = 0; j < numSocketsForPairing; j++)
    {
      if ((stunSocket[i].port&1) == 0 && (stunSocket[i].port+1) == stunSocket[j].port) {
        stunSocket[i].SetSendAddress(0, 0);
        stunSocket[i].SetReadTimeout(PMaxTimeInterval);
        stunSocket[j].SetSendAddress(0, 0);
        stunSocket[j].SetReadTimeout(PMaxTimeInterval);
        socket1 = &stunSocket[i];
        socket2 = &stunSocket[j];
        stunSocket.DisallowDeleteObjects();
        stunSocket.Remove(socket1);
        stunSocket.Remove(socket2);
        stunSocket.AllowDeleteObjects();
        return true;
      }
    }
  }

  PTRACE(2, "STUN\tCould not get a pair of adjacent port numbers from NAT");
  return false;
}

bool PSTUNClient::IsAvailable(const PIPSocket::Address & binding) 
{ 
  switch (GetNatType(PFalse)) {
    case ConeNat :
    case RestrictedNat :
    case PortRestrictedNat :
      break;

    case SymmetricNat :
      if (pairedPortInfo.basePort == 0 || pairedPortInfo.basePort > pairedPortInfo.maxPort)
        return false;
      break;

    default : // UnknownNet, SymmetricFirewall, BlockedNat
      return false;
  }

  return binding.IsAny() || binding == interfaceAddress || binding == cachedExternalAddress;
}

////////////////////////////////////////////////////////////////

PSTUNUDPSocket::PSTUNUDPSocket()
  : externalIP(0)
{
}


PBoolean PSTUNUDPSocket::GetLocalAddress(Address & addr)
{
  if (!externalIP.IsValid())
    return PUDPSocket::GetLocalAddress(addr);

  addr = externalIP;
  return true;
}


PBoolean PSTUNUDPSocket::GetLocalAddress(Address & addr, WORD & port)
{
  if (!externalIP.IsValid())
    return PUDPSocket::GetLocalAddress(addr, port);

  addr = externalIP;
  port = GetPort();
  return true;
}


// End of File ////////////////////////////////////////////////////////////////
