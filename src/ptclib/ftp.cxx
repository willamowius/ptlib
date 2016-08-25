/*
 * ftp.cxx
 *
 * FTP ancestor class.
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

#ifdef __GNUC__
#pragma implementation "ftp.h"
#endif

#include <ptlib.h>
#include <ptlib/sockets.h>
#include <ptclib/ftp.h>


/////////////////////////////////////////////////////////
//  File Transfer Protocol

static const char * const FTPCommands[PFTP::NumCommands] = 
{
  "USER", "PASS", "ACCT", "CWD", "CDUP", "SMNT", "QUIT", "REIN", "PORT", "PASV",
  "TYPE", "STRU", "MODE", "RETR", "STOR", "STOU", "APPE", "ALLO", "REST", "RNFR",
  "RNTO", "ABOR", "DELE", "RMD", "MKD", "PWD", "LIST", "NLST", "SITE", "SYST",
  "STAT", "HELP", "NOOP"
};

PFTP::PFTP()
  : PInternetProtocol("ftp 21", NumCommands, FTPCommands)
{
}


PBoolean PFTP::SendPORT(const PIPSocket::Address & addr, WORD port)
{
  PString str(PString::Printf,
              "%i,%i,%i,%i,%i,%i",
              addr.Byte1(),
              addr.Byte2(),
              addr.Byte3(),
              addr.Byte4(),
              port/256,
              port%256);
  return ExecuteCommand(PORT, str)/100 == 2;
}


// End of File ///////////////////////////////////////////////////////////////
