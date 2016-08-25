/*
 * socks.cxx
 *
 * SOCKS protocol
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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "socks.h"
#endif

#include <ptclib/socks.h>

#define new PNEW

#define SOCKS_VERSION_4 ((BYTE)4)
#define SOCKS_VERSION_5 ((BYTE)5)

#define SOCKS_CMD_CONNECT       ((BYTE)1)
#define SOCKS_CMD_BIND          ((BYTE)2)
#define SOCKS_CMD_UDP_ASSOCIATE ((BYTE)3)

#define SOCKS_AUTH_NONE      ((BYTE)0)
#define SOCKS_AUTH_USER_PASS ((BYTE)2)
#define SOCKS_AUTH_FAILED    ((BYTE)0xff)

#define SOCKS_ADDR_IPV4       ((BYTE)1)
#define SOCKS_ADDR_DOMAINNAME ((BYTE)3)
#define SOCKS_ADDR_IPV6       ((BYTE)4)


///////////////////////////////////////////////////////////////////////////////

PSocksProtocol::PSocksProtocol(WORD port)
  : serverHost("proxy")
{
  serverPort = DefaultServerPort;
  remotePort = port;
  localPort = 0;

#if P_CONFIG_FILE
  // get proxy information
  PConfig config(PConfig::System, "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\");

  // get the proxy configuration string
  PString str = config.GetString("Internet Settings", "ProxyServer", "");
  if (str.Find('=') == P_MAX_INDEX)
    SetServer("socks");
  else {
    PStringArray tokens = str.Tokenise(";");
    PINDEX i;
    for (i = 0; i < tokens.GetSize(); i++) {
      str = tokens[i];
      PINDEX equalPos = str.Find('=');
      if (equalPos != P_MAX_INDEX && (str.Left(equalPos) *= "socks")) {
        SetServer(str.Mid(equalPos+1));
        break;
      }
    }
  }
#endif // P_CONFIG_FILE
}


PBoolean PSocksProtocol::SetServer(const PString & hostname, const char * service)
{
  return SetServer(hostname, PIPSocket::GetPortByService("tcp", service));
}


PBoolean PSocksProtocol::SetServer(const PString & hostname, WORD port)
{
  PINDEX colon = hostname.Find(':');
  if (colon == P_MAX_INDEX)
    serverHost = hostname;
  else {
    unsigned portnum = hostname.Mid(colon+1).AsUnsigned();
    if (portnum == 0)
      serverHost = hostname;
    else {
      serverHost = hostname.Left(colon);
      port = (WORD)portnum;
    }
  }

  if (port == 0)
    port = DefaultServerPort;

  serverPort = port;

  return PTrue;
}


void PSocksProtocol::SetAuthentication(const PString & username, const PString & password)
{
  PAssert(authenticationUsername.GetLength() < 256, PInvalidParameter);
  authenticationUsername = username;
  PAssert(authenticationPassword.GetLength() < 256, PInvalidParameter);
  authenticationPassword = password;
}


PBoolean PSocksProtocol::ConnectSocksServer(PTCPSocket & socket)
{
  PIPSocket::Address ipnum;
  if (!PIPSocket::GetHostAddress(serverHost, ipnum))
    return PFalse;

  remotePort = socket.GetPort();
  socket.SetPort(serverPort);
  return socket.PTCPSocket::Connect(0, ipnum);
}


PBoolean PSocksProtocol::SendSocksCommand(PTCPSocket & socket,
                                      BYTE command,
                                      const char * hostname,
                                      PIPSocket::Address addr)
{
  if (!socket.IsOpen()) {
    if (!ConnectSocksServer(socket))
      return PFalse;

    socket << SOCKS_VERSION_5
           << (authenticationUsername.IsEmpty() ? '\001' : '\002') // length
           << SOCKS_AUTH_NONE;
    if (!authenticationUsername)
      socket << SOCKS_AUTH_USER_PASS;  // Simple cleartext username/password
    socket.flush();

    BYTE auth_pdu[2];
    if (!socket.ReadBlock(auth_pdu, sizeof(auth_pdu)))  // Should get 2 byte reply
      return PFalse;

    if (auth_pdu[0] != SOCKS_VERSION_5 || auth_pdu[1] == SOCKS_AUTH_FAILED) {
      socket.Close();
      SetErrorCodes(PChannel::AccessDenied, EACCES);
      return PFalse;
    }

    if (auth_pdu[1] == SOCKS_AUTH_USER_PASS) {
      // Send username and pasword
      socket << '\x01'
             << (BYTE)authenticationUsername.GetLength()  // Username length as single byte
             << authenticationUsername
             << (BYTE)authenticationPassword.GetLength()  // Password length as single byte
             << authenticationPassword
             << ::flush;

      if (!socket.ReadBlock(auth_pdu, sizeof(auth_pdu)))  // Should get 2 byte reply
        return PFalse;

      if (/*auth_pdu[0] != SOCKS_VERSION_5 ||*/ auth_pdu[1] != 0) {
        socket.Close();
        SetErrorCodes(PChannel::AccessDenied, EACCES);
        return PFalse;
      }
    }
  }

  socket << SOCKS_VERSION_5
         << command
         << '\000'; // Reserved
  if (hostname != NULL)
    socket << SOCKS_ADDR_DOMAINNAME << (BYTE)strlen(hostname) << hostname;
#if P_HAS_IPV6
  else if ( addr.GetVersion() == 6 )
  {
    socket << SOCKS_ADDR_IPV6;
    /* Should be 16 bytes */
    for ( PINDEX i = 0; i < addr.GetSize(); i++ )    
    {
        socket << addr[i];
    }
  }
#endif 
  else
    socket << SOCKS_ADDR_IPV4
           << addr.Byte1() << addr.Byte2() << addr.Byte3() << addr.Byte4();

  socket << (BYTE)(remotePort >> 8) << (BYTE)remotePort
         << ::flush;

  return ReceiveSocksResponse(socket, localAddress, localPort);
}


PBoolean PSocksProtocol::ReceiveSocksResponse(PTCPSocket & socket,
                                          PIPSocket::Address & addr,
                                          WORD & port)
{
  int reply;
  if ((reply = socket.ReadChar()) < 0)
    return PFalse;

  if (reply != SOCKS_VERSION_5) {
    SetErrorCodes(PChannel::Miscellaneous, EINVAL);
    return PFalse;
  }

  if ((reply = socket.ReadChar()) < 0)
    return PFalse;

  switch (reply) {
    case 0 :  // No error
      break;

    case 2 :  // Refused permission
      SetErrorCodes(PChannel::AccessDenied, EACCES);
      return PFalse;

    case 3 : // Network unreachable
      SetErrorCodes(PChannel::NotFound, ENETUNREACH);
      return PFalse;

    case 4 : // Host unreachable
      SetErrorCodes(PChannel::NotFound, EHOSTUNREACH);
      return PFalse;

    case 5 : // Connection refused
      SetErrorCodes(PChannel::NotFound, EHOSTUNREACH);
      return PFalse;

    default :
      SetErrorCodes(PChannel::Miscellaneous, EINVAL);
      return PFalse;
  }

  // Ignore next byte (reserved)
  if ((reply = socket.ReadChar()) < 0)
    return PFalse;

  // Get type byte for bound address
  if ((reply = socket.ReadChar()) < 0)
    return PFalse;

  switch (reply) {
    case SOCKS_ADDR_DOMAINNAME :
      // Get length
      if ((reply = socket.ReadChar()) < 0)
        return PFalse;

      if (!PIPSocket::GetHostAddress(socket.ReadString(reply), addr))
        return PFalse;
      break;

    case SOCKS_ADDR_IPV4 :
      {
          in_addr add;
          if (!socket.ReadBlock(&add, sizeof(add)))
              return PFalse;
          addr = add;
      }
      break;

#if P_HAS_IPV6    
    case SOCKS_ADDR_IPV6 :
      {
        in6_addr add;
        if (!socket.ReadBlock(&add, sizeof(add)))
            return PFalse;
        addr = add;
      }
      break;
#endif

    default :
      SetErrorCodes(PChannel::Miscellaneous, EINVAL);
      return PFalse;
  }

  WORD rxPort;
  if (!socket.ReadBlock(&rxPort, sizeof(rxPort)))
    return PFalse;

  port = PSocket::Net2Host(rxPort);
  return PTrue;
}


///////////////////////////////////////////////////////////////////////////////

PSocksSocket::PSocksSocket(WORD port)
  : PSocksProtocol(port)
{
}


PBoolean PSocksSocket::Connect(const PString & address)
{
  if (!SendSocksCommand(*this, SOCKS_CMD_CONNECT, address, 0))
    return PFalse;

  port = remotePort;
  return PTrue;
}


PBoolean PSocksSocket::Connect(const Address & addr)
{
  if (!SendSocksCommand(*this, SOCKS_CMD_CONNECT, NULL, addr))
    return PFalse;

  port = remotePort;
  return PTrue;
}


PBoolean PSocksSocket::Connect(WORD, const Address &)
{
  PAssertAlways(PUnsupportedFeature);
  return PFalse;
}


PBoolean PSocksSocket::Listen(unsigned, WORD newPort, Reusability reuse)
{
  PAssert(newPort == 0 && port == 0, PUnsupportedFeature);
  PAssert(reuse, PUnsupportedFeature);

  if (!SendSocksCommand(*this, SOCKS_CMD_BIND, NULL, 0))
    return PFalse;

  port = localPort;
  return PTrue;
}


PBoolean PSocksSocket::Accept()
{
  if (!IsOpen())
    return PFalse;

  return ReceiveSocksResponse(*this, remoteAddress, remotePort);
}


PBoolean PSocksSocket::Accept(PSocket & socket)
{
  // If is right class, transfer the SOCKS socket to class to receive the accept
  // The "listener" socket is implicitly closed as there is really only one
  // handle in a SOCKS BIND operation.
  PAssert(PIsDescendant(&socket, PSocksSocket), PUnsupportedFeature);
  os_handle = ((PSocksSocket &)socket).TransferHandle(*this);
  return Accept();
}


int PSocksSocket::TransferHandle(PSocksSocket & destination)
{
  // This "transfers" the socket from one onstance to another.

  int the_handle = os_handle;
  destination.SetReadTimeout(readTimeout);
  destination.SetWriteTimeout(writeTimeout);

  // Close the instance of the socket but don't actually close handle.
  os_handle = -1;

  return the_handle;
}


PBoolean PSocksSocket::GetLocalAddress(Address & addr)
{
  if (!IsOpen())
    return PFalse;

  addr = localAddress;
  return PTrue;
}


PBoolean PSocksSocket::GetLocalAddress(Address & addr, WORD & port)
{
  if (!IsOpen())
    return PFalse;

  addr = localAddress;
  port = localPort;
  return PTrue;
}


PBoolean PSocksSocket::GetPeerAddress(Address & addr)
{
  if (!IsOpen())
    return PFalse;

  addr = remoteAddress;
  return PTrue;
}


PBoolean PSocksSocket::GetPeerAddress(Address & addr, WORD & port)
{
  if (!IsOpen())
    return PFalse;

  addr = remoteAddress;
  port = remotePort;
  return PTrue;
}


void PSocksSocket::SetErrorCodes(PChannel::Errors errCode, int osErr)
{
  SetErrorValues(errCode, osErr);
}


///////////////////////////////////////////////////////////////////////////////

PSocks4Socket::PSocks4Socket(WORD port)
  : PSocksSocket(port)
{
}


PSocks4Socket::PSocks4Socket(const PString & host, WORD port)
  : PSocksSocket(port)
{
  Connect(host);
}


PObject * PSocks4Socket::Clone() const
{
  return new PSocks4Socket(remotePort);
}


PBoolean PSocks4Socket::SendSocksCommand(PTCPSocket & socket,
                                     BYTE command,
                                     const char * hostname,
                                     Address addr)
{
  if (hostname != NULL) {
    if (!GetHostAddress(hostname, addr))
      return PFalse;
  }

  if (!IsOpen()) {
    if (!ConnectSocksServer(*this))
      return PFalse;
  }

  PString user = PProcess::Current().GetUserName();
  socket << SOCKS_VERSION_4
         << command
         << (BYTE)(remotePort >> 8) << (BYTE)remotePort
         << addr.Byte1() << addr.Byte2() << addr.Byte3() << addr.Byte4()
         << user << ((BYTE)0)
         << ::flush;

  return ReceiveSocksResponse(socket, localAddress, localPort);
}


PBoolean PSocks4Socket::ReceiveSocksResponse(PTCPSocket & socket,
                                         Address & addr,
                                         WORD & port)
{
  int reply;
  if ((reply = socket.ReadChar()) < 0)
    return PFalse;

  if (reply != 0 /*!= SOCKS_VERSION_4*/) {
    SetErrorCodes(PChannel::Miscellaneous, EINVAL);
    return PFalse;
  }

  if ((reply = socket.ReadChar()) < 0)
    return PFalse;

  switch (reply) {
    case 90 :  // No error
      break;

    case 91 : // Connection refused
      SetErrorCodes(PChannel::NotFound, EHOSTUNREACH);
      return PFalse;

    case 92 :  // Refused permission
      SetErrorCodes(PChannel::AccessDenied, EACCES);
      return PFalse;

    default :
      SetErrorCodes(PChannel::Miscellaneous, EINVAL);
      return PFalse;
  }

  WORD rxPort;
  if (!socket.ReadBlock(&rxPort, sizeof(rxPort)))
    return PFalse;

  port = PSocket::Net2Host(rxPort);

  in_addr add;
  if ( socket.ReadBlock(&add, sizeof(add)) )
  {
    addr = add;
    return PTrue;
  }

  return PFalse;
}


///////////////////////////////////////////////////////////////////////////////

PSocks5Socket::PSocks5Socket(WORD port)
  : PSocksSocket(port)
{
}


PSocks5Socket::PSocks5Socket(const PString & host, WORD port)
  : PSocksSocket(port)
{
  Connect(host);
}


PObject * PSocks5Socket::Clone() const
{
  return new PSocks5Socket(remotePort);
}


///////////////////////////////////////////////////////////////////////////////

PSocksUDPSocket::PSocksUDPSocket(WORD port)
  : PSocksProtocol(port)
{
}


PSocksUDPSocket::PSocksUDPSocket(const PString & host, WORD port)
  : PSocksProtocol(port)
{
  Connect(host);
}


PObject * PSocksUDPSocket::Clone() const
{
  return new PSocksUDPSocket(port);
}


PBoolean PSocksUDPSocket::Connect(const PString & address)
{
  if (!SendSocksCommand(socksControl, SOCKS_CMD_UDP_ASSOCIATE, address, 0))
    return PFalse;

  socksControl.GetPeerAddress(serverAddress);
  return PTrue;
}


PBoolean PSocksUDPSocket::Connect(const Address & addr)
{
  if (!SendSocksCommand(socksControl, SOCKS_CMD_UDP_ASSOCIATE, NULL, addr))
    return PFalse;

  socksControl.GetPeerAddress(serverAddress);
  return PTrue;
}


PBoolean PSocksUDPSocket::Connect(WORD, const Address &)
{
  PAssertAlways(PUnsupportedFeature);
  return PFalse;
}


PBoolean PSocksUDPSocket::Listen(unsigned, WORD newPort, Reusability reuse)
{
  PAssert(newPort == 0 && port == 0, PUnsupportedFeature);
  PAssert(reuse, PUnsupportedFeature);

  if (!SendSocksCommand(socksControl, SOCKS_CMD_UDP_ASSOCIATE, NULL, 0))
    return PFalse;

  socksControl.GetPeerAddress(serverAddress);
  port = localPort;
  return PTrue;
}


PBoolean PSocksUDPSocket::GetLocalAddress(Address & addr)
{
  if (!IsOpen())
    return PFalse;

  addr = localAddress;
  return PTrue;
}


PBoolean PSocksUDPSocket::GetLocalAddress(Address & addr, WORD & port)
{
  if (!IsOpen())
    return PFalse;

  addr = localAddress;
  port = localPort;
  return PTrue;
}


PBoolean PSocksUDPSocket::GetPeerAddress(Address & addr)
{
  if (!IsOpen())
    return PFalse;

  addr = remoteAddress;
  return PTrue;
}


PBoolean PSocksUDPSocket::GetPeerAddress(Address & addr, WORD & port)
{
  if (!IsOpen())
    return PFalse;

  addr = remoteAddress;
  port = remotePort;
  return PTrue;
}


PBoolean PSocksUDPSocket::ReadFrom(void * buf, PINDEX len, Address & addr, WORD & port)
{
  PBYTEArray newbuf(len+262);
  Address rx_addr;
  WORD rx_port;
  if (!PUDPSocket::ReadFrom(newbuf.GetPointer(), newbuf.GetSize(), rx_addr, rx_port))
    return PFalse;

  if (rx_addr != serverAddress || rx_port != serverPort)
    return PFalse;

  PINDEX port_pos;
  switch (newbuf[3]) {
    case SOCKS_ADDR_DOMAINNAME :
      if (!PIPSocket::GetHostAddress(PString((const char *)&newbuf[5], (PINDEX)newbuf[4]), addr))
        return PFalse;

      port_pos = newbuf[4]+5;
      break;

    case SOCKS_ADDR_IPV4 :
      memcpy(&addr, &newbuf[4], 4);
      port_pos = 4;
      break;

    default :
      SetErrorCodes(PChannel::Miscellaneous, EINVAL);
      return PFalse;
  }

  port = (WORD)((newbuf[port_pos] << 8)|newbuf[port_pos+1]);
  memcpy(buf, &newbuf[port_pos+2], len);

  return PTrue;
}


PBoolean PSocksUDPSocket::WriteTo(const void * buf, PINDEX len, const Address & addr, WORD port)
{
  PBYTEArray newbuf(len+10);
  BYTE * bufptr = newbuf.GetPointer();

  // Build header, bytes 0, 1 & 2 are zero

  bufptr[3] = SOCKS_ADDR_IPV4;
  memcpy(bufptr+4, &addr, 4);
  bufptr[8] = (BYTE)(port >> 8);
  bufptr[9] = (BYTE)port;
  memcpy(bufptr+10, buf, len);

  return PUDPSocket::WriteTo(newbuf, newbuf.GetSize(), serverAddress, serverPort);
}


void PSocksUDPSocket::SetErrorCodes(PChannel::Errors errCode, int osErr)
{
  SetErrorValues(errCode, osErr);
}


// End of File ///////////////////////////////////////////////////////////////
