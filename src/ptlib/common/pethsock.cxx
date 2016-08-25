/*
 * pethsock.cxx
 *
 * Direct Ethernet socket I/O channel class.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <ptlib/sockets.h>

PEthSocket::Address::Address()
{
  memset(b, 0xff, sizeof(b));
}


PEthSocket::Address::Address(const BYTE * addr)
{
  if (addr != NULL)
    memcpy(b, addr, sizeof(b));
  else
    memset(b, 0, sizeof(b));
}


PEthSocket::Address::Address(const Address & addr)
{
  ls.l = addr.ls.l;
  ls.s = addr.ls.s;
}


PEthSocket::Address::Address(const PString & str)
{
  operator=(str);
}


PEthSocket::Address & PEthSocket::Address::operator=(const Address & addr)
{
  ls.l = addr.ls.l;
  ls.s = addr.ls.s;
  return *this;
}


PEthSocket::Address & PEthSocket::Address::operator=(const PString & str)
{
  memset(b, 0, sizeof(b));

  int shift = 0;
  PINDEX byte = 5;
  PINDEX pos = str.GetLength();
  while (pos-- > 0) {
    int c = str[pos];
    if (c != '-') {
      if (isdigit(c))
        b[byte] |= (c - '0') << shift;
      else if (isxdigit(c))
        b[byte] |= (toupper(c) - 'A' + 10) << shift;
      else {
        memset(this, 0, sizeof(*this));
        return *this;
      }
      if (shift == 0)
        shift = 4;
      else {
        shift = 0;
        byte--;
      }
    }
  }

  return *this;
}


bool PEthSocket::Address::operator==(const BYTE * eth) const
{
  if (eth != NULL)
    return memcmp(b, eth, sizeof(b)) == 0;
  else
    return ls.l == 0 && ls.s == 0;
}


bool PEthSocket::Address::operator!=(const BYTE * eth) const
{
  if (eth != NULL)
    return memcmp(b, eth, sizeof(b)) != 0;
  else
    return ls.l != 0 || ls.s != 0;
}


PEthSocket::Address::operator PString() const
{
  return psprintf("%02X-%02X-%02X-%02X-%02X-%02X", b[0], b[1], b[2], b[3], b[4], b[5]);
}


void PEthSocket::Frame::Parse(WORD & type, BYTE * & payload, PINDEX & length)
{
  WORD len_or_type = ntohs(snap.length);
  if (len_or_type > sizeof(*this)) {
    type = len_or_type;
    payload = ether.payload;
    // Subtract off the Ethernet II header
    length -= sizeof(dst_addr)+sizeof(src_addr)+sizeof(snap.length);
    return;
  }

  if (snap.dsap == 0xaa && snap.ssap == 0xaa) {
    type = ntohs(snap.type);   // SNAP header
    payload = snap.payload;
    // Subtract off the 802.2 header and SNAP data
    length = len_or_type - (sizeof(snap)-sizeof(snap.payload));
    return;
  }
  
  if (snap.dsap == 0xff && snap.ssap == 0xff) {
    type = TypeIPX;   // Special case for Novell netware's stuffed up 802.3
    payload = &snap.dsap;
    length = len_or_type;  // Whole thing is IPX payload
    return;
  }

  if (snap.dsap == 0xe0 && snap.ssap == 0xe0)
    type = TypeIPX;   // Special case for Novell netware's 802.2
  else
    type = snap.dsap;    // A pure 802.2 protocol id

  payload = snap.oui;
  // Subtract off the 802.2 header
  length = len_or_type - (sizeof(snap.dsap)+sizeof(snap.ssap)+sizeof(snap.ctrl));
}


const char * PEthSocket::GetProtocolName() const
{
  return "eth";
}


PBoolean PEthSocket::Listen(unsigned, WORD, Reusability)
{
  PAssertAlways(PUnimplementedFunction);
  return PFalse;
}


PBoolean PEthSocket::GetIpAddress(PIPSocket::Address & addr)
{
  PIPSocket::Address net_mask;
  return EnumIpAddress(0, addr, net_mask);
}


PBoolean PEthSocket::GetIpAddress(PIPSocket::Address & addr, PIPSocket::Address & net_mask)
{
  return EnumIpAddress(0, addr, net_mask);
}


PBoolean PEthSocket::ReadPacket(PBYTEArray & buffer,
                            Address & dest,
                            Address & src,
                            WORD & type,
                            PINDEX & length,
                            BYTE * & payload)
{
  Frame * frame = (Frame *)buffer.GetPointer(sizeof(Frame));
  const PINDEX MinFrameSize = sizeof(frame->dst_addr)+sizeof(frame->src_addr)+sizeof(frame->snap.length);

  do {
    if (!Read(frame, sizeof(*frame)))
      return PFalse;
  } while (lastReadCount < MinFrameSize);

  dest = frame->dst_addr;
  src = frame->src_addr;
  length = lastReadCount;
  frame->Parse(type, payload, length);

  return PTrue;
}


// End Of File ///////////////////////////////////////////////////////////////
